/*****************************************************************************
 *
 * Filename:
 * ---------
 *   ccci_tty.c
 *
 * Project:
 * --------
 *   ALPS
 *
 * Description:
 * ------------
 *   MT65XX CCCI Virtual TTY Driver
 *
 ****************************************************************************/

#include <linux/sched.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/wakelock.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/dma-mapping.h>
#include <linux/poll.h>
#include <asm/dma-mapping.h>

#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>

#include "ccci.h"
#include "ccci_tty.h"
#include <ccci_common.h>

typedef struct
{
	int			m_md_id;
	int			count;
	int			ready;
	int			need_reset;
	int			reset_handle;
	int			channel;
	int			uart_tx;
	int			uart_rx_ack;
	struct tty_struct	*tty;
	struct wake_lock	wake_lock;
	//unsigned char		flip_string[CCCI1_TTY_BUF_SIZE];
	wait_queue_head_t	*write_waitq;
	shared_mem_tty_t	*shared_mem;
	//struct semaphore	ccci_tty_mutex;
	struct mutex		ccci_tty_mutex;
	int					has_pending_read;
  // sync ccci_tty_read and ccci_tty_close in SMP.  
	rwlock_t            ccci_tty_rwlock; 
} tty_instance_t;

typedef struct _tty_ctl_block{
	int					m_md_id;
	struct tty_driver	*ccci_tty_driver;
	tty_instance_t		ccci_tty_modem;
	tty_instance_t		ccci_tty_meta;
	tty_instance_t		ccci_tty_ipc;
	shared_mem_tty_t	*uart1_shared_mem;
	shared_mem_tty_t	*uart2_shared_mem;
	shared_mem_tty_t	*uart3_shared_mem;
	int					tty_buf_size;
	wait_queue_head_t	ccci_tty_modem_write_waitq;
	wait_queue_head_t	ccci_tty_meta_write_waitq;
	wait_queue_head_t	ccci_tty_ipc_write_waitq;
	MD_CALL_BACK_QUEUE	tty_notifier;
	char				drv_name[32];
	char				node_name[16];
	struct tasklet_struct	ccci_tty_meta_read_tasklet;
	struct tasklet_struct	ccci_tty_modem_read_tasklet;
	struct tasklet_struct	ccci_tty_ipc_read_tasklet;
}tty_ctl_block_t;

static tty_ctl_block_t	*tty_ctlb[MAX_MD_NUM];


unsigned int tty_debug_enable[MAX_MD_NUM] = {0}; 
//1UL<<0, tty_modem; 1UL<<1, tty_meta; 1UL<<2, tty_rpc

static void ccci_tty_read(unsigned long arg)
{
	int				part, size, accept, ret;
	unsigned		read, write;
	tty_instance_t	*tty_instance = (tty_instance_t *) arg;
	ccci_msg_t		msg;
	char			*rx_buffer;
	int				md_id = tty_instance->m_md_id;
  read_lock_bh(&tty_instance->ccci_tty_rwlock);
	if (tty_instance->tty == NULL) {
		tty_instance->has_pending_read = 1;
    read_unlock_bh(&tty_instance->ccci_tty_rwlock);
		CCCI_DBG_MSG(md_id, "tty", "NULL tty @ read\n");
		return;
	}
	else if ((tty_instance->tty->index == CCCI_TTY_MODEM) && (is_meta_mode()||is_advanced_meta_mode())) {
		//  Do not allow writes to the modem when in Meta Mode.
		//  Otherwise, the modem firmware will crash.
    read_unlock_bh(&tty_instance->ccci_tty_rwlock);
		CCCI_DBG_MSG(md_id, "tty", "Attempted read from modem while in meta mode\n");     
		return;
	}

	read  = tty_instance->shared_mem->rx_control.read;
	write = tty_instance->shared_mem->rx_control.write; 
	size  = write - read;
	rx_buffer = tty_instance->shared_mem->buffer;

	/*ALPS00241537: if there is no data in share memory, not copy and send message to MD*/
	/*because total size is (length-1) which is handled in MD write API, size=0 only indicates memory is empty*/
	if(size == 0) {
    read_unlock_bh(&tty_instance->ccci_tty_rwlock);
		//CCCI_MSG_INF(md_id, 0, "tty", "ttyC%d share memory is empty! \n", tty_instance->tty->index);
		return;
	}

	if (size < 0) {
		size += tty_instance->shared_mem->rx_control.length;
	}

	if(tty_debug_enable[md_id] & (1UL << tty_instance->tty->index))
		CCCI_DBG_MSG(md_id, "tty", "[before Read]:[RX] tty=%04d data_len=%04d write=%04d read=%04d \n",
				tty_instance->tty->index, size, write, read); 

	if (read > write) {
		part = tty_instance->shared_mem->rx_control.length - read;      
		accept = tty_insert_flip_string(tty_instance->tty, &rx_buffer[read], part);

		if (accept < part) {
			size -= accept;
			read += accept;
			goto __ccci_read_ack;
		}
		else {
			size -= part;
			read  = 0;
		}
	}

	accept = tty_insert_flip_string(tty_instance->tty, &rx_buffer[read], size);

	if (accept < size) {
		size -= accept;
		read += accept;
	}
	else {
		size  = 0;
		read += accept;
	}

__ccci_read_ack:

	tty_instance->shared_mem->rx_control.read = read;
	msg.magic = 0xFFFFFFFF;
	msg.id = tty_instance->channel;
	msg.channel = tty_instance->uart_rx_ack;
	msg.reserved = 0;
	mb();
	ret = ccci_message_send(md_id, &msg, 1);

	if( ret != sizeof(msg)) {
		CCCI_DBG_MSG(md_id, "tty", "ccci_write_mailbox for %d fail: %d\n",
				tty_instance->tty->index, ret);
		//ccci_channel_status(tty_instance->uart_rx_ack);
	}

	if(tty_debug_enable[md_id] & (1UL << tty_instance->tty->index))
		CCCI_DBG_MSG(md_id, "tty", "[after  Read]:[RX] tty=%04d data_len=%04d write=%04d read=%4d\n",
			tty_instance->tty->index, accept, tty_instance->shared_mem->rx_control.write, 
			tty_instance->shared_mem->rx_control.read);        

	wake_lock_timeout(&tty_instance->wake_lock, HZ / 2);
	tty_flip_buffer_push(tty_instance->tty);
  read_unlock_bh(&tty_instance->ccci_tty_rwlock);
}


//  will be called when modem sends us something.
//  we will then copy it to the tty's buffer.
//  this is essentially the "read" fops.
static void ccci_tty_callback(void *private)
{
	logic_channel_info_t	*ch_info = (logic_channel_info_t*)private;
	ccci_msg_t				msg;
	tty_ctl_block_t			*ctlb = (tty_ctl_block_t *)ch_info->m_owner;

	while(get_logic_ch_data(ch_info, &msg)){
		switch(msg.channel)
		{
		case CCCI_UART1_TX_ACK:
			// this should be in an interrupt,
			// so no locking required...
			//ctlb->ccci_tty_meta.ready = 1;
			wake_up_interruptible(ctlb->ccci_tty_meta.write_waitq);
			wake_up_interruptible_poll(&ctlb->ccci_tty_driver->ttys[CCCI_TTY_META]->write_wait,POLLOUT);
			break;

		case CCCI_UART1_RX:
			//ccci_tty_meta_read_tasklet.data = (unsigned long) &ccci_tty_meta;
			tasklet_schedule(&ctlb->ccci_tty_meta_read_tasklet);
			break;

		case CCCI_UART2_TX_ACK:
			// this should be in an interrupt,
			// so no locking required...
			//ccci_tty_modem.ready = 1;
			wake_up_interruptible(ctlb->ccci_tty_modem.write_waitq); 
			wake_up_interruptible_poll(&ctlb->ccci_tty_driver->ttys[CCCI_TTY_MODEM]->write_wait,POLLOUT);
			break;

		case CCCI_UART2_RX:
			//ccci_tty_modem_read_tasklet.data = (unsigned long) &ccci_tty_modem;
			tasklet_schedule(&ctlb->ccci_tty_modem_read_tasklet);
			break;

		case CCCI_IPC_UART_TX_ACK:
			//ccci_tty_ipc.ready = 1;
			wake_up_interruptible(ctlb->ccci_tty_ipc.write_waitq);
			wake_up_interruptible_poll(&ctlb->ccci_tty_driver->ttys[CCCI_TTY_IPC]->write_wait,POLLOUT);
			break;

		case CCCI_IPC_UART_RX:
			//ccci_tty_ipc_read_tasklet.data = (unsigned long) &ccci_tty_ipc;
			tasklet_schedule(&ctlb->ccci_tty_ipc_read_tasklet);
			break;

		default:
			break;
		}
	}
}


static int ccci_tty_write(struct tty_struct *tty, const unsigned char *buf, int len)
{
	int				size, over, total, ret;
	unsigned		read, write, length, write_back, tmp_write;
	unsigned char	*ptr;
	tty_instance_t	*tty_instance = (tty_instance_t *) tty->driver_data;
	int				has_free_space = 1;
	int				retry_times, time_out;
	int				xmit_retry=0;
	ccci_msg_t		msg;
	char			*tx_buffer;
	int				md_id = tty_instance->m_md_id;

	if (tty_instance == NULL)
	{
		CCCI_DBG_MSG(md_id, "tty", "NULL tty @ write\n");
		return  -EIO;
	}
	else if ((tty_instance->tty->index == CCCI_TTY_MODEM) && (is_meta_mode())) {
		//  Do not allow writes to the modem when in Meta Mode.
		//  Otherwise, the modem firmware will crash.
		CCCI_DBG_MSG(md_id, "tty", "Attempted write to modem while in meta mode\n");
		return   -EIO;
	}
	else if (len == 0) {
		CCCI_DBG_MSG(md_id, "tty", "Write Length is zero, return directly\n");
		return 0;
	}

	//mutex_lock(&ccci_tty_lock);
	mutex_lock(&tty_instance->ccci_tty_mutex);

	size = 0;
	ptr  = (unsigned char *) buf;

	//if (wait_event_interruptible(*tty_instance->write_waitq, tty_instance->ready == 1) == 0)

	/* Check free space */
	read   = tty_instance->shared_mem->tx_control.read;
	write  = tty_instance->shared_mem->tx_control.write;
	length = tty_instance->shared_mem->tx_control.length;
	tx_buffer = tty_instance->shared_mem->buffer + length;
	over   = length - write;
	write_back = write;

	if (read == write)
	{
		size = length - 1;
	}
	else if (read < write)
	{
		size  = length - write;
		size += read;
	}
	else
	{
		size = read - write -1;
	}

	if(tty_debug_enable[md_id] & (1UL << tty_instance->tty->index))
		CCCI_DBG_MSG(md_id, "tty", "[before Write]:[TX] tty=%04d data_len=%04d write=%04d read=%04d over=%04d\n",
			tty_instance->tty->index, len, write, read, size);		

	if (0 == size){
		/* No space to write, wait MD side read data out to free memory */
		has_free_space = 0;
		retry_times = 2;
		time_out = 2*HZ;

		while(1){
			if(wait_event_interruptible_timeout(*tty_instance->write_waitq, 
					    read != tty_instance->shared_mem->tx_control.read,
					    time_out) == 0) {
				if(retry_times){
					CCCI_DBG_MSG(md_id, "tty", "ttyC%d write timeout, rp=%04d wp=%04d len=%04d\n",
						tty_instance->tty->index,
						tty_instance->shared_mem->tx_control.read,
						tty_instance->shared_mem->tx_control.write,
						tty_instance->shared_mem->tx_control.length);
					retry_times--;
				}
				else {
					/* If retry counter decrease to 0, set time out time to 1 hour from 2s. */
					time_out = 60*60*HZ;
					CCCI_DBG_MSG(md_id, "tty", "ttyC%d change write timeout to 1 hour\n", tty_instance->tty->index);
				}
			}
			else {
				/* MD side has read data out or get a signal */
				if(read != tty_instance->shared_mem->tx_control.read){
					has_free_space = 1;
					/* Re-calculate free space */
					read   = tty_instance->shared_mem->tx_control.read;       
					if (read == write){
						size = length - 1;
					} 
					else if (read < write) {
						size  = length -  write;
						size += read;
					}
					else {
						size = read - write -1;
					}

					CCCI_DBG_MSG(md_id, "tty", "ttyC%d has space to write, S:%d\n", tty_instance->tty->index, size);
				}/* else{ We get a signal, be interrupted } */
				break;
			}
		}
	}

	if(has_free_space){
		if (len > size) {
			CCCI_TTY_MSG(md_id, "Really Write %dbytes for %d bytes\n",size,len);
			len   = size;
			total = size;
		}

		total = len;

		if (over < len) {
			memcpy(&tx_buffer[write], (void *) ptr, over);
			len   -= over;
			ptr   += over;
			write  = 0;
		}

		memcpy(&tx_buffer[write], (void *) ptr, len);
		mb();
		tmp_write = write + len;
		if (tmp_write >= length) {
			tmp_write -= length;
		}
		tty_instance->shared_mem->tx_control.write = tmp_write;
        
		//tty_instance->ready = 0;
		len = total;

		if(tty_debug_enable[md_id] & (1UL << tty_instance->tty->index))
			CCCI_DBG_MSG(md_id, "tty", "[after  Write]:[TX] tty=%04d data_len=%04d write=%04d read=%4d\n",
				tty_instance->tty->index, len, tty_instance->shared_mem->tx_control.write, 
				tty_instance->shared_mem->tx_control.read);

		msg.addr = 0;
		msg.len = len;
		msg.channel = tty_instance->uart_tx;
		msg.reserved = 0;
		mb();
		do{
			ret = ccci_message_send(md_id, &msg, 1);
			if(ret == sizeof(msg))
				break;

			if(ret == -CCCI_ERR_CCIF_NO_PHYSICAL_CHANNEL){
				xmit_retry++;
				msleep(10);
				if( (xmit_retry%10) == 0){
					CCCI_DBG_MSG(md_id, "tty", "[No Physical Channel]ttyC%d retry %d times fail\n", tty_instance->tty->index, xmit_retry);
				}
			} else {
				break;
			}
				
		}while(1);

		if (ret != sizeof(ccci_msg_t)) {
			CCCI_DBG_MSG(md_id, "tty", "ttyC%d write stream fail, tx write pointer will roll back[%d-->%d]\n", 
				tty_instance->tty->index, tty_instance->shared_mem->tx_control.write, write_back);

			tty_instance->ready = 1;
			//tty_instance->shared_mem->tx_control.write = write_back;

			if (ret == CCCI_MD_NOT_READY) {
				CCCI_DBG_MSG(md_id, "tty", "ttyC%d write fail when Modem not ready\n", tty_instance->tty->index);
				//mutex_unlock(&ccci_tty_lock);
				mutex_unlock(&tty_instance->ccci_tty_mutex);
				return ret;
			}

			CCCI_DBG_MSG(md_id, "tty", "ttyC%d write fail: %d\n", tty_instance->tty->index, ret);
			//ccci_channel_status(tty_instance->uart_tx);

			mutex_unlock(&tty_instance->ccci_tty_mutex);
			
			return ret;
		}

	}
	else {
		CCCI_DBG_MSG(md_id, "tty", "Fail write to %d\n", tty_instance->tty->index);
		len = 0;
		//mutex_unlock(&ccci_tty_lock);
		mutex_unlock(&tty_instance->ccci_tty_mutex);

		return -1;
	}

	//mutex_unlock(&ccci_tty_lock);
	mutex_unlock(&tty_instance->ccci_tty_mutex);

	return len;
}


static int ccci_tty_write_room(struct tty_struct *tty)
{
	int      size;
	unsigned read, write;
	tty_instance_t *tty_instance = (tty_instance_t *) tty->driver_data;

	if (tty_instance == NULL) {
		return 0;
	}

	mutex_lock(&tty_instance->ccci_tty_mutex);

	read  = tty_instance->shared_mem->tx_control.read;
	write = tty_instance->shared_mem->tx_control.write;
	size  = read - write;

	if (size < 0) {
		size += tty_instance->shared_mem->tx_control.length;
	}
	else if (size == 0) {
		if (tty_instance->ready == 1) {
			size = tty_instance->shared_mem->tx_control.length-1;
		}
	}

	//mutex_unlock(&ccci_tty_lock);
	mutex_unlock(&tty_instance->ccci_tty_mutex);

	return size;
}


static int ccci_tty_chars_in_buffer(struct tty_struct *tty)
{
	int             size;
	unsigned        read, write;
	tty_instance_t *tty_instance = (tty_instance_t *) tty->driver_data;

	if (tty_instance == NULL) {
		return 0;
	}

	mutex_lock(&tty_instance->ccci_tty_mutex);
	read  = tty_instance->shared_mem->rx_control.read;
	write = tty_instance->shared_mem->rx_control.write;
	size  = write - read;

	if (size < 0) {
		size += tty_instance->shared_mem->rx_control.length;
	}
	else if (size == 0) {
		size = tty_instance->shared_mem->tx_control.length-1;
	}

	mutex_unlock(&tty_instance->ccci_tty_mutex);

	return size;
}


static int ccci_tty_open(struct tty_struct *tty, struct file *f)
{
	int				index = tty->index;
	int				major = tty->driver->major;
	tty_instance_t	*tty_instance;
	char			*name;
	int				ret = 0;
	int				md_id;
	char			name_str[16];
	tty_ctl_block_t	*ctlb = NULL;

	ret = get_md_id_by_dev_major(major);
	if(ret < 0) {
		CCCI_MSG("TTY: mapping to invalid md sys id(%d)\n", ret+1);
		return -ENODEV;
	}
	md_id = ret;
	ret = 0; // Reset ret value
	ctlb = tty_ctlb[md_id];
	WARN_ON(tty!=ctlb->ccci_tty_driver->ttys[index]);

	switch(index)
	{
		case CCCI_TTY_MODEM:
			name = "CCCI_MODEM";
			tty_instance = &ctlb->ccci_tty_modem;
			break;

		case CCCI_TTY_META:
			name = "CCCI_META";            
			tty_instance = &ctlb->ccci_tty_meta;
			break;

		case CCCI_TTY_IPC :
			name = "CCCI_IPC" ;
			tty_instance=&ctlb->ccci_tty_ipc;
			break;

		default:
			return -ENODEV;
	}

	mutex_lock(&tty_instance->ccci_tty_mutex);

	tty_instance->count++;
	tty_instance->m_md_id = md_id;
	if(tty_instance->count > 1){
		CCCI_MSG_INF(md_id, "tty", "[tty_open]Multi-Open! %s open %s%d, %s, count:%d\n", 
			current->comm, ctlb->node_name, index, name, tty_instance->count);
   
		mutex_unlock(&tty_instance->ccci_tty_mutex);
		return -EMFILE;
	}
	else {
		CCCI_MSG_INF(md_id, "tty", "[tty_open]%s open %s%d, %s, nb_flag:%x\n", current->comm, 
			ctlb->node_name, index, name, f->f_flags & O_NONBLOCK);
    write_lock_bh(&tty_instance->ccci_tty_rwlock);
		tty_instance->tty = tty;
		tty_instance->ready = 1;
		tty->driver_data = tty_instance;
		snprintf(name_str, 16, "%s%d", name, md_id);
		wake_lock_init(&tty_instance->wake_lock, WAKE_LOCK_SUSPEND, name_str);
    write_unlock_bh(&tty_instance->ccci_tty_rwlock);		
		mutex_unlock(&tty_instance->ccci_tty_mutex);

		//Note: reset handle must be set after make sure open tty instance successfully
		if(tty_instance->need_reset) {
			//CCCI_MSG("<tty>%s opening ttyC%d, %s\n", current->comm, index, name);
			tty_instance->reset_handle = ccci_reset_register(md_id, name);
			ASSERT(tty_instance->reset_handle >= 0);
		}

		if ((index == CCCI_TTY_MODEM) && (tty_instance->has_pending_read == 1)) {
			tty_instance->has_pending_read = 0;
			tasklet_schedule(&ctlb->ccci_tty_modem_read_tasklet);
		}
	}
    
	return ret;
}


void ccci_reset_buffers(shared_mem_tty_t *shared_mem, int size)
{
	shared_mem->tx_control.length = size;
	shared_mem->tx_control.read   = 0;
	shared_mem->tx_control.write  = 0;

	shared_mem->rx_control.length = size;
	shared_mem->rx_control.read   = 0;
	shared_mem->rx_control.write  = 0;

	memset(shared_mem->buffer, 0, size*2);

	return;
}

int ccci_uart_ipo_h_restore(int md_id)
{
	tty_ctl_block_t	*ctlb = NULL;

	ctlb = tty_ctlb[md_id];
	ccci_reset_buffers(ctlb->uart1_shared_mem, ctlb->tty_buf_size);
	ccci_reset_buffers(ctlb->uart2_shared_mem, ctlb->tty_buf_size);
	ccci_reset_buffers(ctlb->uart3_shared_mem, ctlb->tty_buf_size);
	return 0;
}


static void ccci_tty_close(struct tty_struct *tty, struct file *f)
{
	tty_instance_t	*tty_instance;
	int				md_id;
	tty_ctl_block_t	*ctlb = NULL;

	if (tty == NULL)
		return;

	tty_instance = (tty_instance_t *) tty->driver_data;
	md_id = tty_instance->m_md_id;
	ctlb = tty_ctlb[md_id];

	mutex_lock(&tty_instance->ccci_tty_mutex);
    
	tty_instance->count--;

	CCCI_MSG_INF(md_id, "tty", "[tty_close]Port%d count %d \n", tty->index, tty_instance->count);

	if (tty_instance->count == 0) {
		wake_lock_destroy(&tty_instance->wake_lock);
    // keep tty_instance->tty cannot be used by ccci_tty_read()
    write_lock_bh(&tty_instance->ccci_tty_rwlock);		
		tty->driver_data    = NULL;
		tty_instance->tty   = NULL;
		tty_instance->ready = 0;
    write_unlock_bh(&tty_instance->ccci_tty_rwlock);
		ccci_reset_buffers(tty_instance->shared_mem, ctlb->tty_buf_size);

		if(tty_instance->need_reset) {
			if (tty_instance->reset_handle >= 0) { 
				ccci_user_ready_to_reset(md_id, tty_instance->reset_handle);
			}
			else {
				CCCI_MSG_INF(md_id, "tty", "[tty_close] fail, Invalid reset handle(port%d): %d \n", 
					tty->index, tty_instance->reset_handle);
			}
		}
	}

	mutex_unlock(&tty_instance->ccci_tty_mutex);

	return;
}


static void tty_call_back_func(MD_CALL_BACK_QUEUE *notifier, unsigned long data)
{
	tty_ctl_block_t		*ctl_b = container_of(notifier, tty_ctl_block_t, tty_notifier);
	int					md_id = ctl_b->m_md_id;
	
	switch (data)
	{
		case CCCI_MD_RESET:  
			CCCI_TTY_MSG(md_id, "tty_call_back_func: reset tty buffers \n");
			ccci_reset_buffers(ctl_b->ccci_tty_meta.shared_mem, ctl_b->tty_buf_size);
			ccci_reset_buffers(ctl_b->ccci_tty_modem.shared_mem, ctl_b->tty_buf_size);
			ccci_reset_buffers(ctl_b->ccci_tty_ipc.shared_mem, ctl_b->tty_buf_size);
			break;

		default:
			break;	
	}
}

static struct tty_operations ccci_tty_ops = 
{
	.open  = ccci_tty_open,
	.close = ccci_tty_close,
	.write = ccci_tty_write,
	.write_room = ccci_tty_write_room,
	.chars_in_buffer = ccci_tty_chars_in_buffer,
};


int ccci_tty_init(int md_id)
{
	int					ret = 0;
	struct tty_driver	*tty_driver;
	int 				smem_phy = 0;
	int					smem_size = 0;
	int					tty_buf_len;
	int					major,minor;
	tty_ctl_block_t		*ctlb;

	// Create control block structure
	ctlb = (tty_ctl_block_t *)kmalloc(sizeof(tty_ctl_block_t), GFP_KERNEL);
	if(ctlb == NULL)
		return -CCCI_ERR_GET_MEM_FAIL;

	memset(ctlb, 0, sizeof(tty_ctl_block_t));
	tty_ctlb[md_id] = ctlb;

	// Init ctlb
	ctlb->m_md_id = md_id;
	ctlb->tty_notifier.call = tty_call_back_func;
	ctlb->tty_notifier.next = NULL;
	ret = md_register_call_chain(md_id ,&ctlb->tty_notifier);
    if(ret) {
		CCCI_MSG_INF(md_id, "tty", "md_register_call_chain fail: %d\n", ret);
		goto _RELEASE_CTL_MEMORY;
	}

	tty_driver = alloc_tty_driver(3);
	if (tty_driver == NULL) {
		ret = -ENOMEM;
		goto _RELEASE_CTL_MEMORY;
	}

	snprintf(ctlb->drv_name, 32, "ccci_tty_driver%d", md_id);
	if(md_id == MD_SYS1)
		snprintf(ctlb->node_name, 16, "ttyC");
	else if (md_id == MD_SYS2)
		snprintf(ctlb->node_name, 16, "ccci%d_tty", md_id+1);

	ret = get_dev_id_by_md_id(md_id, "tty", &major, &minor);
	if(ret < 0)
		goto _RELEASE_CTL_MEMORY;

	tty_driver->owner        = THIS_MODULE;
	tty_driver->driver_name  = ctlb->drv_name;
	tty_driver->name         = ctlb->node_name;
	tty_driver->major        = major;
	tty_driver->minor_start  = minor;
	tty_driver->type         = TTY_DRIVER_TYPE_SERIAL;
	tty_driver->subtype      = SERIAL_TYPE_NORMAL;
	tty_driver->init_termios = tty_std_termios;
	tty_driver->init_termios.c_iflag = 0;
	tty_driver->init_termios.c_oflag = 0;
	tty_driver->init_termios.c_cflag = B38400 | CS8 | CREAD;
	tty_driver->init_termios.c_lflag = 0;
	tty_driver->flags        = TTY_DRIVER_RESET_TERMIOS | 
							   TTY_DRIVER_REAL_RAW      ;
						//     | TTY_DRIVER_DYNAMIC_DEV;
	tty_set_operations(tty_driver, &ccci_tty_ops);

	ret = tty_register_driver(tty_driver);
	if (ret != 0) {
		CCCI_MSG_INF(md_id, "tty", "TTY driver Register fail: %d\n", ret);
		goto _DEL_TTY_DRV;
	}
	ctlb->ccci_tty_driver = tty_driver;

	ASSERT(ccci_uart_base_req(md_id, 0, (int*)&ctlb->uart1_shared_mem, &smem_phy, &smem_size) == 0);
	//CCCI_DBG_MSG(md_id, "tty", "TTY0 %x:%x:%d\n", (unsigned int)ctlb->uart1_shared_mem, 
	//			(unsigned int)smem_phy, smem_size);

	// Get tty config information
	ASSERT(ccci_get_sub_module_cfg(md_id, "tty", (char*)&tty_buf_len, sizeof(int)) == sizeof(int) );
	tty_buf_len = (tty_buf_len - sizeof(shared_mem_tty_t))/2;
	ctlb->tty_buf_size = tty_buf_len;

	// Meta section
	ctlb->uart1_shared_mem->tx_control.length = tty_buf_len;
	ctlb->uart1_shared_mem->tx_control.read   = 0;
	ctlb->uart1_shared_mem->tx_control.write  = 0;
	ctlb->uart1_shared_mem->rx_control.length = tty_buf_len;
	ctlb->uart1_shared_mem->rx_control.read   = 0;
	ctlb->uart1_shared_mem->rx_control.write  = 0;

	// meta related channel register
	ASSERT(register_to_logic_ch(md_id, CCCI_UART1_RX,     ccci_tty_callback, ctlb) == 0);
	//ASSERT(ccci_register(CCCI_UART1_RX_ACK, ccci_tty_callback, NULL) == CCCI_SUCCESS);
	//ASSERT(ccci_register(CCCI_UART1_TX,     ccci_tty_callback, NULL) == CCCI_SUCCESS);
	ASSERT(register_to_logic_ch(md_id, CCCI_UART1_TX_ACK, ccci_tty_callback, ctlb) == 0);

	ctlb->ccci_tty_meta.need_reset   = 0;
	ctlb->ccci_tty_meta.reset_handle = -1;
	ctlb->ccci_tty_meta.count        = 0;
	ctlb->ccci_tty_meta.channel      = 0;
	ctlb->ccci_tty_meta.shared_mem   = ctlb->uart1_shared_mem;
	ctlb->ccci_tty_meta.uart_tx      = CCCI_UART1_TX;
	ctlb->ccci_tty_meta.uart_rx_ack  = CCCI_UART1_RX_ACK;
	ctlb->ccci_tty_meta.write_waitq  = &ctlb->ccci_tty_meta_write_waitq;
	ctlb->ccci_tty_meta.ready        = 1;
  rwlock_init(&ctlb->ccci_tty_meta.ccci_tty_rwlock);
	mutex_init(&ctlb->ccci_tty_meta.ccci_tty_mutex);

	// MUX section
	ASSERT(ccci_uart_base_req(md_id, 1, (int*)&ctlb->uart2_shared_mem, &smem_phy, &smem_size) == 0);
	//CCCI_DBG_MSG(md_id, "tty", "TTY1 %x:%x:%d\n", (unsigned int)ctlb->uart2_shared_mem, 
	//			(unsigned int)smem_phy, smem_size);

	ctlb->uart2_shared_mem->tx_control.length = tty_buf_len;
	ctlb->uart2_shared_mem->tx_control.read   = 0;
	ctlb->uart2_shared_mem->tx_control.write  = 0;
            
	ctlb->uart2_shared_mem->rx_control.length = tty_buf_len;
	ctlb->uart2_shared_mem->rx_control.read   = 0;
	ctlb->uart2_shared_mem->rx_control.write  = 0;

	// modem related channel registration.
	ASSERT(register_to_logic_ch(md_id, CCCI_UART2_RX,     ccci_tty_callback, ctlb) == 0);
	//ASSERT(ccci_register(CCCI_UART2_RX_ACK, ccci_tty_callback, NULL) == CCCI_SUCCESS);
	//ASSERT(ccci_register(CCCI_UART2_TX,     ccci_tty_callback, NULL) == CCCI_SUCCESS);
	ASSERT(register_to_logic_ch(md_id, CCCI_UART2_TX_ACK, ccci_tty_callback, ctlb) == 0);

	// modem reset registration.
	ctlb->ccci_tty_modem.need_reset   = 1;
	ctlb->ccci_tty_modem.reset_handle = -1;
	ctlb->ccci_tty_modem.count        = 0;
	ctlb->ccci_tty_modem.channel      = 1;            
	ctlb->ccci_tty_modem.shared_mem   = ctlb->uart2_shared_mem;
	ctlb->ccci_tty_modem.uart_tx      = CCCI_UART2_TX;
	ctlb->ccci_tty_modem.uart_rx_ack  = CCCI_UART2_RX_ACK;
	ctlb->ccci_tty_modem.write_waitq  = &ctlb->ccci_tty_modem_write_waitq;
	ctlb->ccci_tty_modem.ready        = 1;
  rwlock_init(&ctlb->ccci_tty_modem.ccci_tty_rwlock); 
	mutex_init(&ctlb->ccci_tty_modem.ccci_tty_mutex);

	// for IPC uart
	ASSERT(ccci_uart_base_req(md_id, 5, (int*)&ctlb->uart3_shared_mem, &smem_phy, &smem_size) == 0);
	//CCCI_DBG_MSG(md_id, "tty", "TTY2 %x:%x:%d\n", (unsigned int)ctlb->uart3_shared_mem, 
	//			(unsigned int)smem_phy, smem_size);

	ctlb->uart3_shared_mem->tx_control.length = tty_buf_len;
	ctlb->uart3_shared_mem->tx_control.read   = 0;
	ctlb->uart3_shared_mem->tx_control.write  = 0;
	ctlb->uart3_shared_mem->rx_control.length = tty_buf_len;
	ctlb->uart3_shared_mem->rx_control.read   = 0;
	ctlb->uart3_shared_mem->rx_control.write  = 0;
	// IPC related channel register
	ASSERT(register_to_logic_ch(md_id, CCCI_IPC_UART_RX,     ccci_tty_callback, ctlb) == 0);
	//ASSERT(ccci_register(CCCI_IPC_UART_RX_ACK, ccci_tty_callback, NULL) == CCCI_SUCCESS);
	//ASSERT(ccci_register(CCCI_IPC_UART_TX,     ccci_tty_callback, NULL) == CCCI_SUCCESS);
	ASSERT(register_to_logic_ch(md_id, CCCI_IPC_UART_TX_ACK, ccci_tty_callback, ctlb) == 0);

	// IPC reset register
	ctlb->ccci_tty_ipc.need_reset   = 0;
	ctlb->ccci_tty_ipc.reset_handle = -1;
	ctlb->ccci_tty_ipc.count        = 0;
	ctlb->ccci_tty_ipc.channel      = 1;            
	ctlb->ccci_tty_ipc.shared_mem   = ctlb->uart3_shared_mem;
	ctlb->ccci_tty_ipc.uart_tx      = CCCI_IPC_UART_TX;
	ctlb->ccci_tty_ipc.uart_rx_ack  = CCCI_IPC_UART_RX_ACK;
	ctlb->ccci_tty_ipc.write_waitq  = &ctlb->ccci_tty_ipc_write_waitq;
	ctlb->ccci_tty_ipc.ready        = 1;
	rwlock_init(&ctlb->ccci_tty_ipc.ccci_tty_rwlock); 
	mutex_init(&ctlb->ccci_tty_ipc.ccci_tty_mutex);

	//init wait queue and tasklet
	init_waitqueue_head(&ctlb->ccci_tty_modem_write_waitq);
	init_waitqueue_head(&ctlb->ccci_tty_meta_write_waitq);
	init_waitqueue_head(&ctlb->ccci_tty_ipc_write_waitq);
	tasklet_init(&ctlb->ccci_tty_meta_read_tasklet, ccci_tty_read, (unsigned long)&ctlb->ccci_tty_meta);
	tasklet_init(&ctlb->ccci_tty_modem_read_tasklet, ccci_tty_read, (unsigned long)&ctlb->ccci_tty_modem);
	tasklet_init(&ctlb->ccci_tty_ipc_read_tasklet, ccci_tty_read, (unsigned long)&ctlb->ccci_tty_ipc);

	return 0;

_DEL_TTY_DRV:
_RELEASE_CTL_MEMORY:
	kfree(ctlb);
	tty_ctlb[md_id] = NULL;

	return ret;
}


void __exit ccci_tty_exit(int md_id)
{
	tty_ctl_block_t *ctlb = tty_ctlb[md_id];

	if(ctlb != NULL) {
		tty_unregister_driver(ctlb->ccci_tty_driver);
		put_tty_driver(ctlb->ccci_tty_driver);
		un_register_to_logic_ch(md_id, CCCI_UART1_RX);
		//ccci_unregister(CCCI_UART1_RX_ACK);
		//ccci_unregister(CCCI_UART1_TX);
		un_register_to_logic_ch(md_id, CCCI_UART1_TX_ACK);

		un_register_to_logic_ch(md_id, CCCI_UART2_RX);
		//ccci_unregister(CCCI_UART2_RX_ACK);
		//ccci_unregister(CCCI_UART2_TX);
		un_register_to_logic_ch(md_id, CCCI_UART2_TX_ACK);

		un_register_to_logic_ch(md_id, CCCI_IPC_UART_RX);
		//ccci_unregister(CCCI_IPC_UART_RX_ACK);
		//ccci_unregister(CCCI_IPC_UART_TX);
		un_register_to_logic_ch(md_id, CCCI_IPC_UART_TX_ACK);

		ctlb->uart1_shared_mem = NULL;
		ctlb->uart2_shared_mem = NULL;
		ctlb->uart3_shared_mem = NULL;

		kfree(ctlb);
		tty_ctlb[md_id] = NULL;
	}

	return;
}
