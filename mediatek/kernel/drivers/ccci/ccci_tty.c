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

typedef struct
{
	int			count;
	int			ready;
	int         need_reset;
	int			reset_handle;
	int			channel;
	int			uart_tx;
	int			uart_rx_ack;
	struct tty_struct	*tty;
	struct wake_lock	wake_lock;
	unsigned char		flip_string[CCCI_TTY_RX_BUFFER_SIZE];
	wait_queue_head_t	*write_waitq;
	shared_mem_tty_t	*shared_mem;
	//struct semaphore	ccci_tty_mutex;
	struct mutex        ccci_tty_mutex;
} tty_instance_t;

static struct tty_driver *ccci_tty_driver;
static tty_instance_t  ccci_tty_modem, ccci_tty_meta, ccci_tty_ipc;
static shared_mem_tty_t   *uart1_shared_mem;
static shared_mem_tty_t   *uart2_shared_mem;
static shared_mem_tty_t   *uart3_shared_mem;

static int                 has_pending_read = 0;

static void ccci_tty_read      (unsigned long arg);
static DECLARE_TASKLET         (ccci_tty_meta_read_tasklet,  ccci_tty_read, 0);
static DECLARE_TASKLET         (ccci_tty_modem_read_tasklet, ccci_tty_read, 0);
static DECLARE_TASKLET	       (ccci_tty_ipc_read_tasklet,ccci_tty_read,0);
static DECLARE_WAIT_QUEUE_HEAD (ccci_tty_modem_write_waitq);
static DECLARE_WAIT_QUEUE_HEAD (ccci_tty_meta_write_waitq );
static DECLARE_WAIT_QUEUE_HEAD (ccci_tty_ipc_write_waitq);
//static DEFINE_MUTEX            (ccci_tty_lock);

unsigned int tty_debug_enable = 0; 
//1UL<<0, tty_modem; 1UL<<1, tty_meta; 1UL<<2, tty_rpc


static void ccci_tty_read(unsigned long arg)
{
    int             part, size, accept, ret;
    unsigned        read, write;
    tty_instance_t *tty_instance = (tty_instance_t *) arg;

    if (tty_instance->tty == NULL) {
        has_pending_read = 1;
        CCCI_MSG_INF("tty", "NULL tty @ read\n");
        return;
    }
    else if ((tty_instance->tty->index == CCCI_TTY_MODEM) && (is_meta_mode()||is_advanced_meta_mode())) {
        //  Do not allow writes to the modem when in Meta Mode.
        //  Otherwise, the modem firmware will crash.

        CCCI_MSG_INF("tty", "Attempted read from modem while in meta mode\n");     
        return;
    }
    

    read  = tty_instance->shared_mem->rx_control.read;
    write = tty_instance->shared_mem->rx_control.write; 
    size  = write - read;

    /*ALPS00241537: if there is no data in share memory, not copy and send message to MD*/
    /*because total size is (length-1) which is handled in MD write API, size=0 only indicates memory is empty*/
    if(size == 0) {
        //CCCI_MSG_INF("tty", "ttyC%d share memory is empty! \n", tty_instance->tty->index);
        return;
    }
    
    if (size < 0) {
        size += tty_instance->shared_mem->rx_control.length;
    }

	if(tty_debug_enable & (1UL << tty_instance->tty->index))
		CCCI_MSG_INF("tty", "[before Read]:[RX] tty=%04d data_len=%04d write=%04d read=%04d \n",
         tty_instance->tty->index, size, write, read); 
	

    if (read > write) {
        part = tty_instance->shared_mem->rx_control.length - read;
        memcpy(tty_instance->flip_string, &tty_instance->shared_mem->rx_buffer[read], part);          
        accept = tty_insert_flip_string(tty_instance->tty, tty_instance->flip_string, part);

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

    memcpy(tty_instance->flip_string, &tty_instance->shared_mem->rx_buffer[read], size);
    accept = tty_insert_flip_string(tty_instance->tty, tty_instance->flip_string, size);

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
    
    ret = ccci_write_mailbox(tty_instance->uart_rx_ack, tty_instance->channel);
    if (ret != CCCI_SUCCESS) {
        CCCI_MSG_INF("tty", "ccci_write_mailbox for %d fail: %d\n",
               tty_instance->tty->index, ret);
        ccci_channel_status(tty_instance->uart_rx_ack);
		
		// axs: mask assert which will induce device reboot
        //ASSERT(0);
		// axs: mask assert which will induce device reboot
    }

   if(tty_debug_enable & (1UL << tty_instance->tty->index))
		CCCI_MSG_INF("tty", "[after  Read]:[RX] tty=%04d data_len=%04d write=%04d read=%4d\n",
			tty_instance->tty->index, accept, tty_instance->shared_mem->rx_control.write, 
		    tty_instance->shared_mem->rx_control.read);        
    
    wake_lock_timeout(&tty_instance->wake_lock, HZ / 2);
    tty_flip_buffer_push(tty_instance->tty);
}


//  will be called when modem sends us something.
//  we will then copy it to the tty's buffer.
//  this is essentially the "read" fops.
static void ccci_tty_callback(CCCI_BUFF_T *buff, void *private_data)
{
    switch(buff->channel)
    {
        case CCCI_UART1_TX_ACK:
	{
	    // this should be in an interrupt,
	    // so no locking required...
	    ccci_tty_meta.ready = 1;
            wake_up_interruptible(ccci_tty_meta.write_waitq);
	    wake_up_interruptible_poll(&ccci_tty_driver->ttys[CCCI_TTY_META]->write_wait,POLLOUT);
        }
	break;

        case CCCI_UART1_RX:
	{
            ccci_tty_meta_read_tasklet.data = (unsigned long) &ccci_tty_meta;
            tasklet_schedule(&ccci_tty_meta_read_tasklet);
	}
	break;

        case CCCI_UART2_TX_ACK:
	{
	    // this should be in an interrupt,
	    // so no locking required...
	    ccci_tty_modem.ready = 1;
            wake_up_interruptible(ccci_tty_modem.write_waitq); 
	    wake_up_interruptible_poll(&ccci_tty_driver->ttys[CCCI_TTY_MODEM]->write_wait,POLLOUT);
        }
	break;

        case CCCI_UART2_RX:
	{
            ccci_tty_modem_read_tasklet.data = (unsigned long) &ccci_tty_modem;
            tasklet_schedule(&ccci_tty_modem_read_tasklet);
	}
	break;
		
	case CCCI_IPC_UART_TX_ACK:
	{
		ccci_tty_ipc.ready = 1;
            	wake_up_interruptible(ccci_tty_ipc.write_waitq);
		wake_up_interruptible_poll(&ccci_tty_driver->ttys[CCCI_TTY_IPC]->write_wait,POLLOUT);
	}
	break;
		
	case CCCI_IPC_UART_RX:
	{
		ccci_tty_ipc_read_tasklet.data = (unsigned long) &ccci_tty_ipc;
           	tasklet_schedule(&ccci_tty_ipc_read_tasklet);
	}
	break;
	
    default:
        break;
    }
}


static int ccci_tty_write(struct tty_struct *tty, const unsigned char *buf, int len)
{
	int		size, over, total, ret;
	unsigned	read, write, length, write_back, tmp_write;
	unsigned char	*ptr;
	tty_instance_t	*tty_instance = (tty_instance_t *) tty->driver_data;
	int		has_free_space = 1;
	int		retry_times, time_out;
	int		xmit_retry=0;

	if (tty_instance == NULL)
	{
		CCCI_MSG_INF("tty", "NULL tty @ write\n");
		return  -EIO;
	}
	else if ((tty_instance->tty->index == CCCI_TTY_MODEM) && (is_meta_mode())) {
		//  Do not allow writes to the modem when in Meta Mode.
		//  Otherwise, the modem firmware will crash.
		CCCI_MSG_INF("tty", "Attempted write to modem while in meta mode\n");
		return   -EIO;
	}
	else if (len == 0) {
		CCCI_MSG_INF("tty", "Write Length is zero, return directly\n");
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
	over   = length - write;
	write_back = write;

	if (read == write)
	{
		size = length;
	}
	else if (read < write)
	{
		size  = length -  write;
		size += read;
	}
	else
	{
		size = read - write;
	}
	
	if(size >= 1)
		size-=1;
	else
		CCCI_MSG_INF("tty", "Size is Zero 1\n");

	if(tty_debug_enable & (1UL << tty_instance->tty->index))
		CCCI_MSG_INF("tty", "[before Write]:[TX] tty=%04d data_len=%04d write=%04d read=%04d over=%04d\n",
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
					CCCI_MSG_INF("tty", "ttyC%d write timeout, rp=%04d wp=%04d len=%04d\n",
						tty_instance->tty->index,
						tty_instance->shared_mem->tx_control.read,
						tty_instance->shared_mem->tx_control.write,
						tty_instance->shared_mem->tx_control.length);
					retry_times--;
				}
				else {
					/* If retry counter decrease to 0, set time out time to 1 hour from 2s. */
					time_out = 60*60*HZ;
					CCCI_MSG_INF("tty", "ttyC%d change write timeout to 1 hour\n", tty_instance->tty->index);
				}
			}
			else {
				/* MD side has read data out or get a signal */
				if(read != tty_instance->shared_mem->tx_control.read){
					has_free_space = 1;
					/* Re-calculate free space */
					read   = tty_instance->shared_mem->tx_control.read;       
					if (read == write){
						size = length;
					} 
					else if (read < write) {
						size  = length -  write;
						size += read;
					}
					else {
						size = read - write;
					}
					
					if(size>=1)
						size-=1;
					else
						CCCI_MSG_INF("tty", "Size is Zero 2\n");
					
					CCCI_MSG_INF("tty", "ttyC%d has space to write, S:%d\n", tty_instance->tty->index, size);
				}/* else{ We get a signal, be interrupted } */
				break;
			}
		}
	}

	if(has_free_space){
		if (len > size) {
			CCCI_TTY_MSG("Really Write %dbytes for %d bytes\n",size,len);
			len   = size;
			total = size;
		}

		total = len;

		if (over < len) {
			memcpy(&tty_instance->shared_mem->tx_buffer[write], (void *) ptr, over);
			len   -= over;
			ptr   += over;
			write  = 0;
		}

		memcpy(&tty_instance->shared_mem->tx_buffer[write], (void *) ptr, len);
		/*ALPS00233095(CMUX FCS error issue): write buffer will out of order for optimization, */
		/* update write pointer->memcpy, so need add dsb() between them*/
		//dsb();
		mb();
		tmp_write = write + len;
		if (tmp_write >= length) {
			tmp_write -= length;
		}
		tty_instance->shared_mem->tx_control.write = tmp_write;
        
		tty_instance->ready = 0;
		len = total;

		if(tty_debug_enable & (1UL << tty_instance->tty->index))
			CCCI_MSG_INF("tty", "[after  Write]:[TX] tty=%04d data_len=%04d write=%04d read=%4d\n",
				tty_instance->tty->index, len, tty_instance->shared_mem->tx_control.write, 
				tty_instance->shared_mem->tx_control.read);

		do{
			ret = ccci_write_stream(tty_instance->uart_tx, (unsigned int) NULL, len);
			if(ret == CCCI_SUCCESS)
				break;

			if(ret == CCCI_NO_PHY_CHANNEL){
				xmit_retry++;
				msleep(10);
				if( (xmit_retry%10) == 0){
					CCCI_MSG_INF("tty", "[No Physical Channel]ttyC%d retry %d times fail\n", tty_instance->tty->index, xmit_retry);
				}
			}
			else {
				break;
			}
				
		}while(1);

		if (ret != CCCI_SUCCESS) {
			CCCI_MSG_INF("tty", "ttyC%d write stream fail, tx write pointer will roll back[%d-->%d]\n", 
				tty_instance->tty->index, tty_instance->shared_mem->tx_control.write, write_back);

			tty_instance->ready = 1; 			
			tty_instance->shared_mem->tx_control.write = write_back;
			
			if (ret == CCCI_MD_NOT_READY) {
				CCCI_MSG_INF("tty", "ttyC%d write fail when Modem not ready\n", tty_instance->tty->index);
				//mutex_unlock(&ccci_tty_lock);
				mutex_unlock(&tty_instance->ccci_tty_mutex);
				return ret;
			}

			CCCI_MSG_INF("tty", "ttyC%d write fail: %d\n", tty_instance->tty->index, ret);
			ccci_channel_status(tty_instance->uart_tx);

			// axs: mask assert which will induce device reboot
			//ASSERT(0);			
			// axs: mask assert which will induce device reboot
		
			//mutex_unlock(&ccci_tty_lock);
			mutex_unlock(&tty_instance->ccci_tty_mutex);
			
			return ret;
		}

	}
	else {
		CCCI_MSG_INF("tty", "Fail write to %d\n", tty_instance->tty->index);
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
    
    //mutex_lock(&ccci_tty_lock);
    mutex_lock(&tty_instance->ccci_tty_mutex);
        
    read  = tty_instance->shared_mem->tx_control.read;
    write = tty_instance->shared_mem->tx_control.write;
    size  = read - write;

    if (size < 0) {
        size += tty_instance->shared_mem->tx_control.length;
    }
    else if (size == 0) {
        if (tty_instance->ready == 1) {
            size = tty_instance->shared_mem->tx_control.length;
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
    
    //mutex_lock(&ccci_tty_lock);
    mutex_lock(&tty_instance->ccci_tty_mutex);
        
    read  = tty_instance->shared_mem->rx_control.read;
    write = tty_instance->shared_mem->rx_control.write;
    size  = write - read;

    if (size < 0) {
        size += tty_instance->shared_mem->rx_control.length;
    }
    else if (size == 0) {
        size = tty_instance->shared_mem->tx_control.length;
    }

    //mutex_unlock(&ccci_tty_lock);
    mutex_unlock(&tty_instance->ccci_tty_mutex);
    
    return size;
}


static int ccci_tty_open(struct tty_struct *tty, struct file *f)
{
    int             index = tty->index;
    tty_instance_t *tty_instance;
    char     *name;
    WARN_ON(tty!=ccci_tty_driver->ttys[index]);

    switch(index)
    {
        case CCCI_TTY_MODEM:
        {
            name = "CCCI_MODEM";
            tty_instance = &ccci_tty_modem;
        }
	break;

        case CCCI_TTY_META:
        {
            name = "CCCI_META";            
            tty_instance = &ccci_tty_meta;
        }
	break;
	
	case CCCI_TTY_IPC :
	{
		name = "CCCI_IPC" ;
		tty_instance=&ccci_tty_ipc;
	}
	break;
		
    default:
        return -ENODEV;
    }

	mutex_lock(&tty_instance->ccci_tty_mutex);
		
   	tty_instance->count++;
    if(tty_instance->count > 1){
        CCCI_MSG_INF("tty", "[tty_open]Multi-Open! %s open ttyC%d, %s, count:%d\n", 
        	current->comm, index, name, tty_instance->count);
   
		mutex_unlock(&tty_instance->ccci_tty_mutex);
		
		return -EMFILE;
    }
	else {
		CCCI_MSG_INF("tty", "[tty_open]%s open ttyC%d, %s, nb_flag:%x\n", current->comm, 
			index, name, f->f_flags & O_NONBLOCK);
 
    	tty_instance->tty = tty;
    	tty_instance->ready = 1;
    	tty->driver_data = tty_instance;
    	wake_lock_init(&tty_instance->wake_lock, WAKE_LOCK_SUSPEND, name);
		mutex_unlock(&tty_instance->ccci_tty_mutex);

		//Note: reset handle must be set after make sure open tty instance successfully
    	if(tty_instance->need_reset) {
        	//CCCI_MSG("<tty>%s opening ttyC%d, %s\n", current->comm, index, name);
        	tty_instance->reset_handle = ccci_reset_register(name);
    		ASSERT(tty_instance->reset_handle >= 0);
    	}

    	if ((index == CCCI_TTY_MODEM) && (has_pending_read == 1)) {
        	has_pending_read = 0;
        	ccci_tty_modem_read_tasklet.data = (unsigned long) &ccci_tty_modem;
        	tasklet_schedule(&ccci_tty_modem_read_tasklet);
    	}
	}
    
    return 0;
}


void ccci_reset_buffers(shared_mem_tty_t *shared_mem)
{
    shared_mem->tx_control.length = CCCI_TTY_TX_BUFFER_SIZE;
    shared_mem->tx_control.read   = 0;
    shared_mem->tx_control.write  = 0;
    memset(shared_mem->tx_buffer, 0, CCCI_TTY_TX_BUFFER_SIZE);
                
    shared_mem->rx_control.length = CCCI_TTY_RX_BUFFER_SIZE;
    shared_mem->rx_control.read   = 0;
    shared_mem->rx_control.write  = 0;
    memset(shared_mem->rx_buffer, 0, CCCI_TTY_RX_BUFFER_SIZE);

	return;
}


static void ccci_tty_close(struct tty_struct *tty, struct file *f)
{
    tty_instance_t *tty_instance;
      
    if (tty == NULL)
	return;
        
    tty_instance = (tty_instance_t *) tty->driver_data;

    mutex_lock(&tty_instance->ccci_tty_mutex);
    
    tty_instance->count--;
    
    CCCI_MSG_INF("tty", "[tty_close]Port%d count %d \n", tty->index, tty_instance->count);
    
    if (tty_instance->count == 0) {
        wake_lock_destroy(&tty_instance->wake_lock);
        tty->driver_data    = NULL;
	tty_instance->tty   = NULL;
        tty_instance->ready = 0;
	ccci_reset_buffers(tty_instance->shared_mem  );
  
		if(tty_instance->need_reset) {
        	if (tty_instance->reset_handle >= 0) { 
            	ccci_reset_request(tty_instance->reset_handle);
        	}
        	else {
            	CCCI_MSG_INF("tty", "[tty_close]Invalid reset handle(port%d): %d \n", 
					tty->index, tty_instance->reset_handle);
        	}
        }
    }

    mutex_unlock(&tty_instance->ccci_tty_mutex);
	
	return;
}


static void tty_call_back_func(MD_CALL_BACK_QUEUE*queue __always_unused,unsigned long data)
{
	switch (data)
	{	
		case CCCI_MD_RESET:  
			CCCI_TTY_MSG("tty_call_back_func: reset tty buffers \n");
			ccci_reset_buffers(ccci_tty_meta.shared_mem);
			ccci_reset_buffers(ccci_tty_modem.shared_mem);
			ccci_reset_buffers(ccci_tty_ipc.shared_mem);
			break;
			
		default:
			break;	
	}
}
	
MD_CALL_BACK_QUEUE tty_call_back={
	.call=tty_call_back_func,
	.next=NULL,
};

static struct tty_operations ccci_tty_ops = 
{
    .open  = ccci_tty_open,
    .close = ccci_tty_close,
    .write = ccci_tty_write,
    .write_room = ccci_tty_write_room,
    .chars_in_buffer = ccci_tty_chars_in_buffer,
};



int __init ccci_tty_init(void)
{
    int ret = 0;

    ccci_tty_driver = alloc_tty_driver(3);
    if (ccci_tty_driver == NULL) {
        return -ENOMEM;
    }

    ccci_tty_driver->owner        = THIS_MODULE;
    ccci_tty_driver->driver_name  = "ccci_tty_driver";
    ccci_tty_driver->name         = "ttyC";
    //ccci_tty_driver->major        = 0;
    ccci_tty_driver->major        = CCCI_TTY_DEV_MAJOR;
    ccci_tty_driver->minor_start  = 0;
    ccci_tty_driver->type         = TTY_DRIVER_TYPE_SERIAL;
    ccci_tty_driver->subtype      = SERIAL_TYPE_NORMAL;
    ccci_tty_driver->init_termios = tty_std_termios;
    ccci_tty_driver->init_termios.c_iflag = 0;
    ccci_tty_driver->init_termios.c_oflag = 0;
    ccci_tty_driver->init_termios.c_cflag = B38400 | CS8 | CREAD;
    ccci_tty_driver->init_termios.c_lflag = 0;
    ccci_tty_driver->flags        = TTY_DRIVER_RESET_TERMIOS | 
                                    TTY_DRIVER_REAL_RAW      ;
                              //     | TTY_DRIVER_DYNAMIC_DEV;
    tty_set_operations(ccci_tty_driver, &ccci_tty_ops);

    ret = tty_register_driver(ccci_tty_driver);
    if (ret != 0) {
        CCCI_MSG_INF("tty", "TTY driver Register fail: %d\n", ret);
        return ret;
    }

//    tty_register_device(ccci_tty_driver, CCCI_TTY_MODEM, 0);
//    tty_register_device(ccci_tty_driver, CCCI_TTY_META,  0);
//    tty_register_device(ccci_tty_driver, CCCI_TTY_IPC,  0);
    
        {
            int smem_phy = 0, smem_size = 0;
            ASSERT(ccci_uart_setup(0, (int*)&uart1_shared_mem, &smem_phy, &smem_size) == CCCI_SUCCESS);
            CCCI_MSG_INF("tty", "TTY0 %x:%x:%d\n", (unsigned int)uart1_shared_mem, 
				(unsigned int)smem_phy, smem_size);

            uart1_shared_mem->tx_control.length = CCCI_TTY_TX_BUFFER_SIZE;
            uart1_shared_mem->tx_control.read   = 0;
            uart1_shared_mem->tx_control.write  = 0;
            
            uart1_shared_mem->rx_control.length = CCCI_TTY_RX_BUFFER_SIZE;
            uart1_shared_mem->rx_control.read   = 0;
            uart1_shared_mem->rx_control.write  = 0;
            
            // register this tty instance to the ccci driver.
            // pass it the notification handler.

            // meta related channel register
            ASSERT(ccci_register(CCCI_UART1_RX,     ccci_tty_callback, NULL) == CCCI_SUCCESS);
            ASSERT(ccci_register(CCCI_UART1_RX_ACK, ccci_tty_callback, NULL) == CCCI_SUCCESS);
            ASSERT(ccci_register(CCCI_UART1_TX,     ccci_tty_callback, NULL) == CCCI_SUCCESS);
            ASSERT(ccci_register(CCCI_UART1_TX_ACK, ccci_tty_callback, NULL) == CCCI_SUCCESS);
            
            // we're ready to go.
            ccci_tty_meta.need_reset   = 0;
            ccci_tty_meta.reset_handle = -1;
            ccci_tty_meta.count        = 0;
            ccci_tty_meta.channel      = 0;
            ccci_tty_meta.shared_mem   = uart1_shared_mem;
            ccci_tty_meta.uart_tx      = CCCI_UART1_TX;
            ccci_tty_meta.uart_rx_ack  = CCCI_UART1_RX_ACK;
            ccci_tty_meta.write_waitq  = &ccci_tty_meta_write_waitq;
            ccci_tty_meta.ready        = 1;

			mutex_init(&ccci_tty_meta.ccci_tty_mutex);
        }

        {
            int smem_phy = 0, smem_size = 0;
            ASSERT(ccci_uart_setup(1, (int*)&uart2_shared_mem, &smem_phy, &smem_size) == CCCI_SUCCESS);
            CCCI_MSG_INF("tty", "TTY1 %x:%x:%d\n", (unsigned int)uart2_shared_mem, 
				(unsigned int)smem_phy, smem_size);
        
            uart2_shared_mem->tx_control.length = CCCI_TTY_TX_BUFFER_SIZE;
            uart2_shared_mem->tx_control.read   = 0;
            uart2_shared_mem->tx_control.write  = 0;
            
            uart2_shared_mem->rx_control.length = CCCI_TTY_RX_BUFFER_SIZE;
            uart2_shared_mem->rx_control.read   = 0;
            uart2_shared_mem->rx_control.write  = 0;
            
            // modem related channel registration.
            ASSERT(ccci_register(CCCI_UART2_RX,     ccci_tty_callback, NULL) == CCCI_SUCCESS);
            ASSERT(ccci_register(CCCI_UART2_RX_ACK, ccci_tty_callback, NULL) == CCCI_SUCCESS);
            ASSERT(ccci_register(CCCI_UART2_TX,     ccci_tty_callback, NULL) == CCCI_SUCCESS);
            ASSERT(ccci_register(CCCI_UART2_TX_ACK, ccci_tty_callback, NULL) == CCCI_SUCCESS);

            // modem reset registration.
            ccci_tty_modem.need_reset   = 1;
            ccci_tty_modem.reset_handle = -1;
            ccci_tty_modem.count        = 0;
            ccci_tty_modem.channel      = 1;            
            ccci_tty_modem.shared_mem   = uart2_shared_mem;
            ccci_tty_modem.uart_tx      = CCCI_UART2_TX;
            ccci_tty_modem.uart_rx_ack  = CCCI_UART2_RX_ACK;
            ccci_tty_modem.write_waitq  = &ccci_tty_modem_write_waitq;
            ccci_tty_modem.ready        = 1;
         
			mutex_init(&ccci_tty_modem.ccci_tty_mutex);
        }

	{
            int smem_phy = 0, smem_size = 0;
            // for IPC uart
            ASSERT(ccci_uart_setup(5, (int*)&uart3_shared_mem, &smem_phy, &smem_size) == CCCI_SUCCESS);
            CCCI_MSG_INF("tty", "TTY2 %x:%x:%d\n", (unsigned int)uart3_shared_mem, 
				(unsigned int)smem_phy, smem_size);
        
            uart3_shared_mem->tx_control.length = CCCI_TTY_TX_BUFFER_SIZE;
            uart3_shared_mem->tx_control.read   = 0;
            uart3_shared_mem->tx_control.write  = 0;
            
            uart3_shared_mem->rx_control.length = CCCI_TTY_RX_BUFFER_SIZE;
            uart3_shared_mem->rx_control.read   = 0;
            uart3_shared_mem->rx_control.write  = 0;
            
            // IPC related channel register
            ASSERT(ccci_register(CCCI_IPC_UART_RX,     ccci_tty_callback, NULL) == CCCI_SUCCESS);
            ASSERT(ccci_register(CCCI_IPC_UART_RX_ACK, ccci_tty_callback, NULL) == CCCI_SUCCESS);
            ASSERT(ccci_register(CCCI_IPC_UART_TX,     ccci_tty_callback, NULL) == CCCI_SUCCESS);
            ASSERT(ccci_register(CCCI_IPC_UART_TX_ACK, ccci_tty_callback, NULL) == CCCI_SUCCESS);

            // IPC reset register
            ccci_tty_ipc.need_reset   = 0;
            ccci_tty_ipc.reset_handle = -1;
            ccci_tty_ipc.count        = 0;
            ccci_tty_ipc.channel      = 1;            
            ccci_tty_ipc.shared_mem   = uart3_shared_mem;
            ccci_tty_ipc.uart_tx      = CCCI_IPC_UART_TX;
            ccci_tty_ipc.uart_rx_ack  = CCCI_IPC_UART_RX_ACK;
            ccci_tty_ipc.write_waitq  = &ccci_tty_ipc_write_waitq;
            ccci_tty_ipc.ready        = 1;
       
			mutex_init(&ccci_tty_ipc.ccci_tty_mutex);
        }
	
	//register tty call back function during modem boot up stage
	md_register_call_chain(&md_notifier,&tty_call_back);
	
    return ret;
}


 void __exit ccci_tty_exit(void)
{
	tty_unregister_driver(ccci_tty_driver);
	put_tty_driver(ccci_tty_driver);
	ccci_unregister(CCCI_UART1_RX);
	ccci_unregister(CCCI_UART1_RX_ACK);
	ccci_unregister(CCCI_UART1_TX);
	ccci_unregister(CCCI_UART1_TX_ACK);
	
	ccci_unregister(CCCI_UART2_RX);
	ccci_unregister(CCCI_UART2_RX_ACK);
	ccci_unregister(CCCI_UART2_TX);
	ccci_unregister(CCCI_UART2_TX_ACK);

	ccci_unregister(CCCI_IPC_UART_RX);
	ccci_unregister(CCCI_IPC_UART_RX_ACK);
	ccci_unregister(CCCI_IPC_UART_TX);
	ccci_unregister(CCCI_IPC_UART_TX_ACK);
	
	uart1_shared_mem = NULL;
	uart2_shared_mem = NULL;
	uart3_shared_mem = NULL;

	return;
}

EXPORT_SYMBOL(ccci_reset_buffers);


