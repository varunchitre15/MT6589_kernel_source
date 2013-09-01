/*****************************************************************************
 *
 * Filename:
 * ---------
 *   ccci_ipc.c
 *
 * 
 * Author:
 * -------
 *   Yalin wang (mtk80678)
 *
 ****************************************************************************/

#include <linux/module.h>
#include <linux/poll.h>
#include <ccci.h>
#include <linux/uaccess.h>
#include <asm/io.h>

#if 0
#undef WARN_ON 
#ifdef CCCI_DEBUG_ON
#define DBG(format, ...) printk(KERN_WARNING "CCCI_IPC_DEBUG:%s()," format,__func__, ##__VA_ARGS__)

#define WARN_ON(cond,fmt,arg...)   ({if (!!(cond))\
	printk(KERN_WARNING "CCCI_IPC_WARNING:file(%s) line(%d):\n" fmt ,__FILE__,__LINE__,##arg);})
#else
#define DBG(format, ...)
#define WARN_ON(cond,fmt,arg...)
#endif
#endif

#define local_AP_id_2_unify_id(id) local_xx_id_2_unify_id(id,1)
#define local_MD_id_2_unify_id(id) local_xx_id_2_unify_id(id,0)
#define unify_AP_id_2_local_id(id)   unify_xx_id_2_local_id(id,1)
#define unify_MD_id_2_local_id(id)   unify_xx_id_2_local_id(id,0)
static DEFINE_SPINLOCK(ccci_ipc_wr_lock);
static DEFINE_SPINLOCK(ccci_ipc_rd_lock);
// static uint32 bit_map[ALIGN(MAX_NUM_IPC_TASKS,BITS_PER_LONG)/BITS_PER_LONG];
//static DEFINE_MUTEX(ipc_smem_mutex);
static CCCI_IPC_MEM *ipc_mem;
static uint32 ccci_ipc_smem_base_phy;
static uint32 ccci_ipc_buffer_phy_wr;
static uint32 ccci_ipc_buffer_phy_rd;
static uint32 ccci_ipc_smem_size;
static struct cdev *ccci_ipc_cdev;
static void *ccci_ipc_mapping_addr;
static DECLARE_WAIT_QUEUE_HEAD(poll_md_queue_head);
IPC_TASK ipc_task[MAX_NUM_IPC_TASKS] ;


static void release_recv_item(CCCI_RECV_ITEM *item);
static void ipc_call_back_func(MD_CALL_BACK_QUEUE*,unsigned long);



static int hard_ware_ok=0;
MD_CALL_BACK_QUEUE ipc_call_back={
	.call=ipc_call_back_func,
	.next=NULL,
};

static void ipc_call_back_func(MD_CALL_BACK_QUEUE*queue __always_unused,unsigned long data)
{
	switch (data)
	{	
		case CCCI_MD_EXCEPTION:
		case CCCI_MD_RESET :
			if (hard_ware_ok)
			{
				IPC_TASK *tsk;
				int i;
				CCCI_RECV_ITEM *item,*n;
				hard_ware_ok=0;
				CCCI_IPC_MSG("MD exception call chain !\n");
				for (i=0;i<MAX_NUM_IPC_TASKS;i++)
				{
					tsk=ipc_task+i;
					spin_lock(&tsk->lock);
					list_for_each_entry_safe(item,n,&tsk->recv_list,list)
					{
						release_recv_item(item);
					}
					spin_unlock(&tsk->lock);
					__wake_up(&tsk->write_wait_queue, TASK_NORMAL, 0, (void*)POLLERR);
					__wake_up(&tsk->read_wait_queue, TASK_NORMAL, 0, (void*)POLLERR);
				}
					spin_lock(&ccci_ipc_wr_lock);
					ipc_mem->buffer.buff_wr.tx_offset=0;
					ipc_mem->buffer.buff_wr.rx_offset=0;
					spin_unlock(&ccci_ipc_wr_lock);

					spin_lock(&ccci_ipc_rd_lock);
					ipc_mem->buffer.buff_rd.tx_offset=0;
					ipc_mem->buffer.buff_rd.rx_offset=0;
					spin_unlock(&ccci_ipc_rd_lock);
                 		
			}
              
			break;
		case CCCI_MD_BOOTUP:
			hard_ware_ok=1;	
			wake_up_all(&poll_md_queue_head);
			CCCI_IPC_MSG("MD boot up successfully.\n");
			break;
		
	}

}


static IPC_MSGSVC_TASKMAP_T ipc_msgsvc_maptbl[] =
{
	//	X_IPC_MODULE_CONF(1,M_SSDBG1,0,1)     //TASK_ID_1
	//	X_IPC_MODULE_CONF(1,AP_SSDBG2,1,1)     //TASK_ID_2
	#define __IPC_ID_TABLE
	#include "ccci_ipc_task_ID.h"
	#undef __IPC_ID_TABLE
    
};

void find_task_to_clear(uint32 to_id)
{
	IPC_TASK *task=NULL;
	int i;
	for (i=0;i<MAX_NUM_IPC_TASKS;i++)
	{
		if (ipc_task[i].to_id==to_id)
		{
			if (task==NULL)  
			{	
				task=ipc_task+i;
				continue ;
			}
			if (time_after(task->jiffies,ipc_task[i].jiffies))
			{
				task=ipc_task+i;
			}
			else if  (task->jiffies==ipc_task[i].jiffies)
			{
				CCCI_MSG_INF("ipc", "Wrong time stamp, it's a BUG ?? .\n");
			}
		}
	}

	if (task==NULL)
	{
		CCCI_MSG_INF("ipc", "Wrong MD  ID(%d) to clear for next recv.\n",to_id);
		return ;
	}
	CCCI_IPC_MSG("wake up task:%d \n",task-ipc_task);
	clear_bit(CCCI_TASK_PENDING,&task->flag);
	wake_up_poll(&task->write_wait_queue,POLLOUT);
	
}

static IPC_MSGSVC_TASKMAP_T *local_xx_id_2_unify_id(uint32 local_id,int AP)
{
	int i;
	for (i=0;i<sizeof(ipc_msgsvc_maptbl)/sizeof(ipc_msgsvc_maptbl[0]);i++)
	{
		if (ipc_msgsvc_maptbl[i].task_id==local_id &&
			 (AP?(ipc_msgsvc_maptbl[i].extq_id&AP_UNIFY_ID_FLAG):!(ipc_msgsvc_maptbl[i].extq_id&AP_UNIFY_ID_FLAG)))
	
			return  ipc_msgsvc_maptbl+i;
			
	
	}	
	return NULL;

}

static IPC_MSGSVC_TASKMAP_T *unify_xx_id_2_local_id(uint32 unify_id,int AP)
{
	int i;
	if (!(AP?(unify_id&AP_UNIFY_ID_FLAG):!(unify_id&AP_UNIFY_ID_FLAG)))
	 return NULL;
	
	for (i=0;i<sizeof(ipc_msgsvc_maptbl)/sizeof(ipc_msgsvc_maptbl[0]);i++)
	{
		if (ipc_msgsvc_maptbl[i].extq_id==unify_id)
			return 	ipc_msgsvc_maptbl+i;
	
	}
	return NULL;
}

/*
static void dump(void *data,size_t size)
{
	uint32 i=0;
	printk("Dump data %d bytes.\n",size);
	
	for (i=0;i<size;i+=4)
	{
		if (i%16==0)printk("\n%d----%d :",i,i+16);
		printk("%#x\t",*((uint32*)(data+i)));
	}
	printk("\n");

}
*/
static int ccci_ipc_write_stream(CCCI_CHANNEL_T channel,int addr,int len,uint32 reserved)
{
	CCCI_BUFF_T buff;
	memset(&buff, 0, sizeof(CCCI_BUFF_T));
    	CCCI_INIT_STREAM(&buff, addr, len);
	buff.reserved=reserved;
	CCCI_IPC_MSG("write to task:%d addr:%#x len:%d.\n",reserved,addr,len);
	return ccci_write(channel, &buff);
}
static int ccci_ipc_ack(CCCI_CHANNEL_T channel,int id,uint32 reserved)
{
    CCCI_BUFF_T buff;
    memset(&buff, 0, sizeof(CCCI_BUFF_T));
    CCCI_INIT_MAILBOX(&buff, id);
    buff.reserved=reserved;	
    return ccci_write(channel, &buff);

}

static void release_recv_item(CCCI_RECV_ITEM *item)
{
	if (item)
	{	
		if (!list_empty(&item->list))
		list_del_init(&item->list);
		if (item->data) kfree(item->data);
		kfree(item);
	}
}


void *read_from_ring_buffer(ipc_ilm_t *ilm,BUFF *buff_rd,int *len)
{
	int size;
	int write;
	int read;
	int data_size;
	void *data;
	void *ret=NULL;
	int over=0;
	int copy=0;
	int real_size=0;
	spin_lock_irq(&ccci_ipc_rd_lock);
	size=buff_rd->size;
	write=buff_rd->tx_offset;
	read=buff_rd->rx_offset;
	data_size=(write-read)>0?(write-read):(size-(read-write));
	//WARN_ON(data_size<=0,"wrong data_size");
	if(data_size<=0)
		CCCI_MSG_INF("ipc", "wrong data_size");
	CCCI_IPC_MSG("tx_offset=%d rx_offset=%d .\n",write,read);
	data=kmalloc(data_size+sizeof(ipc_ilm_t),GFP_ATOMIC);
	if (data==NULL) 
	{
		CCCI_MSG_INF("ipc", "kmalloc for read ilm failed !\n");
		ret=NULL;
		goto out;
	}
	*((ipc_ilm_t *)data)=*ilm;
	ilm=(ipc_ilm_t *)data;
	data +=sizeof(ipc_ilm_t) ;

	if (write<read) over=size-read;
	if (over)
	{
		if (data_size<over)  over=data_size;
		memcpy(data,buff_rd->buffer+read,over);
		copy+=over;
		read=(read+over)&(size-1);
	}
	if (copy<data_size)
	{
		memcpy(data+copy,buff_rd->buffer+read,data_size-copy);
	}
	real_size +=(ilm->local_para_ptr)?((local_para_struct *)data)->msg_len:0;
	data +=real_size;
	real_size +=(ilm->peer_buff_ptr)?((peer_buff_struct *)data)->pdu_len:0;

	buff_rd->rx_offset += real_size;
	buff_rd->rx_offset &= size-1;
	ret=ilm;
	*len=real_size+sizeof(ipc_ilm_t);
	//WARN_ON(real_size>data_size,"wrong real_size>data_size");
	if(real_size>data_size)
		CCCI_MSG_INF("ipc", "wrong real_size>data_size");
	CCCI_IPC_MSG("recv real_size=%#8x data_size=%#8x.\n",real_size,data_size);
out:
	spin_unlock_irq(&ccci_ipc_rd_lock);
	return ret;
}
static void recv_item(uint32 addr,uint32 len,IPC_TASK *task, BUFF *buff_rd)
{
	ipc_ilm_t *ilm=(ipc_ilm_t *)((uint32)ccci_ipc_mapping_addr+(addr-MODEM_REGION_BASE));
	CCCI_RECV_ITEM *item ;
	unsigned long flags;
	//WARN_ON(len!=sizeof(ipc_ilm_t),"Wrong len ipc_ilm_t(sizeof(ipc_ilm_t)=%d len=%d) .\n",sizeof(ipc_ilm_t),len);
	if(len!=sizeof(ipc_ilm_t))
		CCCI_MSG_INF("ipc", "Wrong len ipc_ilm_t(sizeof(ipc_ilm_t)=%d len=%d) .\n",sizeof(ipc_ilm_t),len);
	CCCI_IPC_MSG("Recv item Physical_Addr:%#x Virtual_Addr:%p Len:%d.\n",addr,ilm,len);
	if (addr>MODEM_REGION_BASE+CCCI_MD_IMAGE_MAPPING_SIZE)
	{
		CCCI_MSG_INF("ipc", "Wrong physical address %#x .\n",addr);
		return ;
	}
	
	item=kmalloc(sizeof(CCCI_RECV_ITEM),GFP_ATOMIC);
	if (item==NULL)
	{
		CCCI_MSG_INF("ipc", "kmalloc for recv_item failed !\n");
		goto out;
	}
	if (ilm->local_para_ptr){
	
		//WARN_ON((uint32)ilm->local_para_ptr<(uint32)ccci_ipc_buffer_phy_rd ||
		//	 (uint32)ilm->local_para_ptr>=(uint32)ccci_ipc_buffer_phy_rd+CCCI_IPC_BUFFER_SIZE,
		//		"wrong ilm->local_para_ptr address.");
		if((uint32)ilm->local_para_ptr<(uint32)ccci_ipc_buffer_phy_rd ||
		   (uint32)ilm->local_para_ptr>=(uint32)ccci_ipc_buffer_phy_rd+CCCI_IPC_BUFFER_SIZE)
			CCCI_MSG_INF("ipc", "wrong ilm->local_para_ptr address.");
	}
	
	if (ilm->peer_buff_ptr){
		//WARN_ON((uint32)ilm->peer_buff_ptr<(uint32)ccci_ipc_buffer_phy_rd ||
		//	 (uint32)ilm->peer_buff_ptr>=(uint32)ccci_ipc_buffer_phy_rd+CCCI_IPC_BUFFER_SIZE,
		//		"wrong ilm->peer_buff_ptr address.");
		if((uint32)ilm->peer_buff_ptr<(uint32)ccci_ipc_buffer_phy_rd ||
		   (uint32)ilm->peer_buff_ptr>=(uint32)ccci_ipc_buffer_phy_rd+CCCI_IPC_BUFFER_SIZE)
			CCCI_MSG_INF("ipc", "wrong ilm->peer_buff_ptr address.");
	}
	CCCI_IPC_MSG("recv ilm->local_para_ptr: %p ilm->peer_buff_ptr: %p .\n",ilm->local_para_ptr,ilm->peer_buff_ptr);
	
	INIT_LIST_HEAD(&item->list);
	item->data=read_from_ring_buffer(ilm,buff_rd,&item->len);
	if (item->data==NULL)
	{
		CCCI_MSG_INF("ipc", "item->data==NULL , recv failed .\n");
		goto out1;
	}
//	dump(item->data,item->len);
	spin_lock_irqsave(&task->lock,flags);
	list_add_tail(&item->list,&task->recv_list);
	spin_unlock_irqrestore(&task->lock,flags);
	kill_fasync(&task->fasync, SIGIO, POLL_IN);
	wake_up_poll(&task->read_wait_queue,POLLIN);
	goto out;
out1:
	kfree(item);
out: 
	return;
}



static int write_to_ring_buffer(void *data,int count,IPC_TASK *task, BUFF *ipc_buffer)
{
	int ret=0;
	int free;
	int write,read,over,copy;
	int size;
	int write_begin;
	ipc_ilm_t *ilm=task->ilm_p;
	local_para_struct *local_para=ilm->local_para_ptr?data:NULL;
	peer_buff_struct  *peer_buff=ilm->peer_buff_ptr?
			(peer_buff_struct*)((uint32)data+(local_para?local_para->msg_len:0)):NULL;
	//WARN_ON(((local_para?local_para->msg_len:0)+(peer_buff?peer_buff->pdu_len:0))!=count,"Count is not equal !\n");
	if(((local_para?local_para->msg_len:0)+(peer_buff?peer_buff->pdu_len:0))!=count)
		CCCI_MSG_INF("ipc", "Count is not equal !\n");
	CCCI_IPC_MSG("local_para_struct addr=%p peer_buff_struct addr=%p.\n",local_para,peer_buff);
	if ((local_para?local_para->msg_len:0)+(peer_buff?peer_buff->pdu_len:0)!=count)
	{
		CCCI_MSG_INF("ipc", "Count is not equal(%#x != %#x ) !\n",
			(local_para?local_para->msg_len:0)+(peer_buff?peer_buff->pdu_len:0),count);
		return -EINVAL;
	}
	if ((local_para?local_para->ref_count!=1:0)||(peer_buff?peer_buff->ref_count!=1:0))
	{
		CCCI_MSG_INF("ipc", "ref count !=1 .\n");
		return -EINVAL;
	}
	spin_lock_irq(&ccci_ipc_wr_lock);
	write_begin=write=ipc_buffer->tx_offset;
	read=ipc_buffer->rx_offset;
	size=ipc_buffer->size;
	copy=0;
	CCCI_IPC_MSG("tx_offset=%d rx_offset=%d .\n",write,read);
	if (read<=write)
	{
		free=size-(write-read);
		over=size-write;
	}
	else 
	{	
		free=read-write;
		over=0;
	}
	if (count>free)
	{
		CCCI_MSG_INF("ipc", "Too big count(%#x) free=%#x!\n",count,free);
		ret=-E2BIG;
		goto out;
	}
	if (over)
	{
		if (count<over)  over=count;
		memcpy(ipc_buffer->buffer+write,data,over);
		copy+=over;
		write=(write+over)&(size-1);
	}
	if (copy<count)
	{
		memcpy(ipc_buffer->buffer+write,data+copy,count-copy);
	}
	/*ALPS00233095(CMUX FCS error issue): write buffer will out of order for optimization, */
	/* update write pointer->memcpy, so need add dsb() between them*/
	//dsb();
	mb();
	ipc_buffer->tx_offset += count;
	ipc_buffer->tx_offset &= size-1;
	ret=count;
//	WARN_ON(count<sizeof(local_para_struct),"count<sizeof(local_para_struct).\n");
	ilm->local_para_ptr=local_para?(local_para_struct *)(ccci_ipc_buffer_phy_wr+write_begin):NULL;
	ilm->peer_buff_ptr=peer_buff?
		(peer_buff_struct *)(ccci_ipc_buffer_phy_wr+((write_begin+(local_para?local_para->msg_len:0))&(size-1))):NULL;



out:
	spin_unlock_irq(&ccci_ipc_wr_lock);
	return ret;
}
static void ccci_ipc_callback(CCCI_BUFF_T *buff, void *data)
{
	IPC_TASK *task;
//	WARN_ON(buff->reserved>=MAX_NUM_IPC_TASKS,"Error Channel Num!\n");
	IPC_MSGSVC_TASKMAP_T *id_map;
//	task=(IPC_TASK *)data+buff->reserved;
	if (buff->channel==CCCI_IPC_RX_ACK||buff->channel==CCCI_IPC_TX)
	{
		CCCI_MSG_INF("ipc", "callback channel bug (channel:%d)!\n",buff->channel);
	}
	if (buff->channel==CCCI_IPC_RX)
	{
		CCCI_IPC_MSG("CCCI_IPC_RX callback,Unify AP ID:%#x \n",buff->reserved);
		if ((id_map=unify_AP_id_2_local_id(buff->reserved))==NULL)
		{
			CCCI_MSG_INF("ipc", "Wrong AP Unify id (%#x)@RX.\n",buff->reserved);
			return;
		}
		task=(IPC_TASK *)data+id_map->task_id;
		recv_item(((CCCI_STREAM_T*)buff)->addr,((CCCI_STREAM_T*)buff)->len,task,&ipc_mem->buffer.buff_rd);
		ccci_ipc_ack(CCCI_IPC_RX_ACK,IPC_MSGSVC_RVC_DONE,buff->reserved);
		
	}
	if (buff->channel==CCCI_IPC_TX_ACK)
	{
		CCCI_IPC_MSG("CCCI_IPC_TX_ACK callback  Unify MD ID:%d.\n",buff->reserved);
		if ((id_map=unify_MD_id_2_local_id(buff->reserved))==NULL)
		{
			CCCI_MSG_INF("ipc", "Wrong AP Unify id (%d)@Tx ack.\n",buff->reserved);
			return;
		}
		find_task_to_clear(id_map->task_id);
//		clear_bit(CCCI_TASK_PENDING,&task->flag);
//		wake_up(&task->write_wait_queue);
		
		//WARN_ON(((CCCI_MAILBOX_T*)buff)->id!=IPC_MSGSVC_RVC_DONE,"Not write mailbox id.\n");
		if(((CCCI_MAILBOX_T*)buff)->id!=IPC_MSGSVC_RVC_DONE)
			CCCI_IPC_MSG("Not write mailbox id.\n");
	}
}

/*
static void ccci_ipc_timer_func(unsigned long data)
{
	


}
*/
static void ipc_task_init(IPC_TASK *task,ipc_ilm_t *ilm)
{
	spin_lock_init(&task->lock);
	task->flag=0;
	task->user=(atomic_t)ATOMIC_INIT(0);
//	setup_timer(&task->timer,ccci_ipc_timer_func,(unsigned long)task);
	task->jiffies=-1UL;
	task->fasync=NULL;
	task->ilm_p=ilm;
	task->time_out=-1;
	task->ilm_phy_addr=ccci_ipc_smem_base_phy+(uint32)ilm-(uint32)(ipc_mem->ilm);

	init_waitqueue_head(&task->read_wait_queue);
	init_waitqueue_head(&task->write_wait_queue);
	INIT_LIST_HEAD(&task->recv_list);
}

static int ccci_ipc_open(struct inode *inode, struct file *file)
{
	int index=iminor(inode);
	if (index>=MAX_NUM_IPC_TASKS)
	{
		CCCI_MSG_INF("ipc", "Wrong minor num %d.\n",index);
		return -EINVAL;
	}
	CCCI_IPC_MSG("register task:%d.\n",index);
	nonseekable_open(inode,file);
	file->private_data=ipc_task+index;
	atomic_inc(&((ipc_task+index)->user));
	return 0;

}

static ssize_t ccci_ipc_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	int ret=0;
	IPC_TASK *task=file->private_data;
	CCCI_RECV_ITEM *recv_data;
retry:
	spin_lock_irq(&task->lock);
	if (hard_ware_ok==0)
	{
		ret=-EIO;
		goto out_unlock;
	}
	if (list_empty(&task->recv_list))
	{
		if (file->f_flags & O_NONBLOCK)
		{
			ret=-EAGAIN;
			goto out_unlock;

		}
		spin_unlock_irq(&task->lock);
		interruptible_sleep_on(&task->read_wait_queue);
		if (signal_pending(current))
		{
			CCCI_IPC_MSG("Interrupt read sys_call : task:%s pid:%d tgid:%d SIGPEND:%#llx. GROUP_SIGPEND:%#llx .\n",
				current->comm,current->pid,current->tgid,
				*(unsigned long long *)current->pending.signal.sig,
					*(unsigned long long *)current->signal->shared_pending.signal.sig);
			ret=-EINTR;
			goto out;
		}
		goto retry;
	}
	recv_data=container_of(task->recv_list.next,CCCI_RECV_ITEM,list);
	
	if (recv_data->len>count)
	{
		CCCI_MSG_INF("ipc", "Recv buff is too small(count=%d data_len=%d)!\n",count,recv_data->len);
		ret=-E2BIG;
		goto out_unlock;	
	}
	list_del_init(&recv_data->list);
	spin_unlock_irq(&task->lock);
	
	if (copy_to_user(buf,recv_data->data,recv_data->len))
	{
		ret=-EFAULT;
		release_recv_item(recv_data);
		goto out;
	}
	ret=recv_data->len;
	release_recv_item(recv_data);
	goto out;


out_unlock:
	spin_unlock_irq(&task->lock);

out:
	return ret;

}

static ssize_t ccci_ipc_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
//	int loop=0;
	int ret=0;
	IPC_TASK *task=file->private_data;
	IPC_MSGSVC_TASKMAP_T *id_map;
	ipc_ilm_t *ilm=NULL;
	if (count<sizeof(ipc_ilm_t))
	{
		CCCI_MSG_INF("ipc", "Write len < ipc_ilm_t ,wrong para.\n");
		ret=-EINVAL;
		goto out;
	}
	
	ilm=kmalloc(count,GFP_KERNEL);
	if (ilm==NULL)
	{
		CCCI_MSG_INF("ipc", "kmalloc filed!\n");
		ret=-ENOMEM;
		goto out;
	}
	if (copy_from_user(ilm,buf,count))
	{
		CCCI_MSG_INF("ipc", "Copy error!\n");
		ret=-EFAULT;
		goto out_free;
		
	}
	if ((id_map=local_MD_id_2_unify_id(ilm->dest_mod_id))==NULL)
	{
		CCCI_MSG_INF("ipc", "Invalid DEST MD id (%d).\n",ilm->dest_mod_id);
		ret=-EINVAL;
		goto out_free;
	}

	if (test_and_set_bit(CCCI_TASK_PENDING,&task->flag))
	{
		CCCI_IPC_MSG("write is busy. Task ID=%d.\n",task-ipc_task);
		if (file->f_flags & O_NONBLOCK)
		{
			ret=-EBUSY;
			goto  out_free;
		}
		else if ( wait_event_interruptible_exclusive(task->write_wait_queue,
				!test_and_set_bit(CCCI_TASK_PENDING,&task->flag)||hard_ware_ok==0)==-ERESTARTSYS)
		{
			ret=-EINTR;
			goto out_free;
		}
	}

	spin_lock_irq(&task->lock);
	if (hard_ware_ok==0)
	{
		ret=-EIO;
		goto out_unlock;
	}
	spin_unlock_irq(&task->lock);
	task->jiffies=jiffies;
	*task->ilm_p=*ilm;
	task->to_id=ilm->dest_mod_id;
	task->ilm_p->src_mod_id=task-ipc_task;
	CCCI_IPC_MSG("MSG id=%#x. len=%#x .\n",ilm->msg_id,count);
	if (count>sizeof(ipc_ilm_t))
	{
		if (write_to_ring_buffer(ilm+1,count-sizeof(ipc_ilm_t),task,&ipc_mem->buffer.buff_wr)!=count-sizeof(ipc_ilm_t))
		{
			CCCI_MSG_INF("ipc", "write_to_ring_buffer failed!\n");
			clear_bit(CCCI_TASK_PENDING,&task->flag);
			ret=-EAGAIN;
			goto out_free;
		}
		
	}
//	do{
//		loop++;
		
		ret=ccci_ipc_write_stream(CCCI_IPC_TX,task->ilm_phy_addr,sizeof(ipc_ilm_t),id_map->extq_id);
//		cond_resched();
//	}while(ret==CCCI_NO_PHY_CHANNEL) ;
//	dump(ilm,count);
	if (ret!=CCCI_SUCCESS)
	{	// ret==CCCI_IN_USE  this should not happened.
		CCCI_MSG_INF("ipc", "ccci_ipc_write_stream fialed (ret=%d)!\n",ret);
		clear_bit(CCCI_TASK_PENDING,&task->flag);
		ret=-EAGAIN;
		goto out_free;
	}
out_unlock:
	spin_unlock_irq(&task->lock);	

out_free:
	kfree(ilm);

out:
	return ret==0?count:ret;


}

static long ccci_ipc_ioctl( struct file *file, unsigned int cmd, unsigned long arg)
{
	IPC_TASK *task=file->private_data;
	CCCI_RECV_ITEM *item,*n;
	
	long ret=0;
	switch(cmd)
	{
		case CCCI_IPC_RESET_RECV:
			spin_lock_irq(&task->lock);
			list_for_each_entry_safe(item,n,&task->recv_list,list)
			{
				release_recv_item(item);
			}
			
			spin_unlock_irq(&task->lock);
			
			ret=0;
			break;
		case CCCI_IPC_RESET_SEND:
			clear_bit(CCCI_TASK_PENDING,&task->flag);
			wake_up(&task->write_wait_queue);
			break;
		case CCCI_IPC_WAIT_MD_READY:
			if (hard_ware_ok==0)
			{
				interruptible_sleep_on(&poll_md_queue_head);
				if (signal_pending(current)) ret=-EINTR;
			}
			break;
		default:
			ret=-EINVAL;
			break;
	}

	return ret;
}

static int ccci_ipc_release(struct inode *inode, struct file *file)
{

	CCCI_RECV_ITEM *item,*n;
	IPC_TASK *task=file->private_data;
	if (atomic_dec_and_test(&task->user))
	{
		spin_lock_irq(&task->lock);
			list_for_each_entry_safe(item,n,&task->recv_list,list)
			{
				release_recv_item(item);
			}
		spin_unlock_irq(&task->lock);
	}
	clear_bit(CCCI_TASK_PENDING,&task->flag);
//	return fasync_helper(-1, file, 0,&task->fasync);
	return 0;

}

static int ccci_ipc_fasync(int fd, struct file *file, int on)
{
	IPC_TASK *task=file->private_data;
	return fasync_helper(fd,file,on,&task->fasync);


}
static uint32 ccci_ipc_poll(struct file *file,poll_table *wait)
{
	IPC_TASK *task=file->private_data;
	int ret=0;
	poll_wait(file,&task->read_wait_queue,wait);
	poll_wait(file,&task->write_wait_queue,wait);
	spin_lock_irq(&task->lock);
    if (hard_ware_ok==0) { ret |= POLLERR; goto out;}
	if (!list_empty(&task->recv_list)) ret |= POLLIN|POLLRDNORM;
	if (!test_bit(CCCI_TASK_PENDING,&task->flag)) ret |= POLLOUT|POLLWRNORM;
out:
	spin_unlock_irq(&task->lock);
	return ret;
}

static struct file_operations ccci_ipc_fops=
{
	.owner = THIS_MODULE,
	.open= ccci_ipc_open,
	.read=ccci_ipc_read,
	.write=ccci_ipc_write,
	.release=ccci_ipc_release,
	.unlocked_ioctl=ccci_ipc_ioctl,
	.fasync=ccci_ipc_fasync,
	.poll=ccci_ipc_poll,

};
int __init ccci_ipc_init(void)
{
	int ret=0;
	int i=0;
	
	ccci_ipc_setup((int*)(&ipc_mem),&ccci_ipc_smem_base_phy,&ccci_ipc_smem_size);
	CCCI_MSG_INF("ipc", "virt:%p, phy:%#x, len:%#x\n",ipc_mem,ccci_ipc_smem_base_phy,ccci_ipc_smem_size);

	ccci_ipc_buffer_phy_wr=ccci_ipc_smem_base_phy+offset_of(CCCI_IPC_MEM,buffer.buff_wr.buffer);
	ccci_ipc_buffer_phy_rd=ccci_ipc_smem_base_phy+offset_of(CCCI_IPC_MEM,buffer.buff_rd.buffer);
	CCCI_MSG_INF("ipc", "ccci_ipc_buffer_phy_wr: %#x, ccci_ipc_buffer_phy_rd: %#x.\n",ccci_ipc_buffer_phy_wr, ccci_ipc_buffer_phy_rd);

	ccci_ipc_mapping_addr=ioremap(MODEM_REGION_BASE,CCCI_MD_IMAGE_MAPPING_SIZE);
	CCCI_MSG_INF("ipc", "ccci_ipc_mapping_addr :%p.\n",ccci_ipc_mapping_addr);
	if (ccci_ipc_mapping_addr==NULL)
	{
		CCCI_MSG_INF("ipc", "ioremap for ccci_ipc_mapping_addr failed !\n ");
		return -1;
	}
	CCCI_IPC_MSG("ioremap MODEM_REGION_BASE --->%p.\n",ccci_ipc_mapping_addr);

	ipc_mem->buffer.buff_wr.size=CCCI_IPC_BUFFER_SIZE;
	ipc_mem->buffer.buff_wr.rx_offset=0;
	ipc_mem->buffer.buff_wr.tx_offset=0;
	ipc_mem->buffer.buff_rd.size=CCCI_IPC_BUFFER_SIZE;
	ipc_mem->buffer.buff_rd.rx_offset=0;
	ipc_mem->buffer.buff_rd.tx_offset=0;

	for (i=0;i<MAX_NUM_IPC_TASKS;i++)
	{
		ipc_task_init(ipc_task+i,ipc_mem->ilm+i);
	}

	if (register_chrdev_region(MKDEV(CCCI_IPC_DEV_MAJOR,0),MAX_NUM_IPC_TASKS,"CCCI_IPC_DEV") != 0)
	{
		CCCI_MSG_INF("ipc", "Regsiter CCCI_IPC_DEV failed!\n");
		ret=-1;
		goto out_err_3;
	}

	ccci_ipc_cdev=cdev_alloc();
	if (ccci_ipc_cdev==NULL)
	{
		CCCI_MSG_INF("ipc", "cdev alloc failed!\n");
		ret=-1;
		goto out_err_2;

	}

	cdev_init(ccci_ipc_cdev,&ccci_ipc_fops);
	ccci_ipc_cdev->owner=THIS_MODULE;
	if ((ret=cdev_add(ccci_ipc_cdev,MKDEV(CCCI_IPC_DEV_MAJOR,0),MAX_NUM_IPC_TASKS))<0)
	{
		CCCI_MSG_INF("ipc", "cdev_add failed!\n");	
			
		goto out_err_1;
	}

	if (ccci_register(CCCI_IPC_RX,ccci_ipc_callback,ipc_task)||
		ccci_register(CCCI_IPC_RX_ACK,ccci_ipc_callback,ipc_task)||
		ccci_register(CCCI_IPC_TX,ccci_ipc_callback,ipc_task)||
		ccci_register(CCCI_IPC_TX_ACK,ccci_ipc_callback,ipc_task))
	{
		CCCI_MSG_INF("ipc", "ccci_ipc_register failed!\n");
		ret=-1;
		goto out_err_0;
	}

	md_register_call_chain(&md_notifier,&ipc_call_back);
	goto out;
	
out_err_0:
	ccci_unregister(CCCI_IPC_RX);
	ccci_unregister(CCCI_IPC_RX_ACK);
	ccci_unregister(CCCI_IPC_TX);
	ccci_unregister(CCCI_IPC_TX_ACK);
	
out_err_1:
	cdev_del(ccci_ipc_cdev);	
	
out_err_2:
	unregister_chrdev_region(MKDEV(CCCI_IPC_DEV_MAJOR,0),MAX_NUM_IPC_TASKS);
	
out_err_3:
	iounmap(ccci_ipc_mapping_addr);
	
out:	
	return ret;
}
 
void __exit ccci_ipc_exit(void)
{
	int i;
	for (i=0;i<MAX_NUM_IPC_TASKS;i++)
	{
		if (atomic_read(&ipc_task[i].user))
		{
			CCCI_MSG_INF("ipc", "BUG for taskID %d module exit count.\n",i);
		}
	}
	iounmap(ccci_ipc_mapping_addr);
	cdev_del(ccci_ipc_cdev);
	unregister_chrdev_region(MKDEV(CCCI_IPC_DEV_MAJOR,0),MAX_NUM_IPC_TASKS);
	md_unregister_call_chain(&md_notifier,&ipc_call_back);
	ccci_unregister(CCCI_IPC_RX);
	ccci_unregister(CCCI_IPC_RX_ACK);
	ccci_unregister(CCCI_IPC_TX);
	ccci_unregister(CCCI_IPC_TX_ACK);
}

#if 0
module_init(ccci_ipc_init);
module_exit(ccci_ipc_exit);

MODULE_AUTHOR("Yalin.wang  MTK80678");
MODULE_DESCRIPTION("CCCI AP&MD ipc  Driver");
MODULE_LICENSE("Proprietary");

#endif









