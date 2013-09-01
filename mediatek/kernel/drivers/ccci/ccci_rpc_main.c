/*****************************************************************************
 *
 * Filename:
 * ---------
 *   ccci_fs.c
 *
 * Project:
 * --------
 *   YuSu
 *
 * Description:
 * ------------
 *   MT6516 CCCI RPC
 *
 * Author:
 * -------
 *   
 *
 ****************************************************************************/

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/kfifo.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/semaphore.h>
#include <linux/version.h>
#include <ccci.h>


#if 0
//#define ENCRYPT_DEBUG
//#define CCCI_RPC_DEBUG_ON
#ifdef CCCI_RPC_DEBUG_ON
#define RPC_DBG(format, args...)  	printk("ccci_rpc: " format, ##args)
#else
#define RPC_DBG(format, ...)
#endif
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36))
static spinlock_t       rpc_fifo_lock = SPIN_LOCK_UNLOCKED;
#else
static DEFINE_SPINLOCK(rpc_fifo_lock);
#endif

static RPC_BUF			*rpc_buf_vir;
static unsigned int     rpc_buf_phy;
static unsigned int     rpc_buf_len;
static struct kfifo     rpc_fifo;
struct work_struct      rpc_work;

extern void ccci_rpc_work_helper(int *p_pkt_num, RPC_PKT pkt[], RPC_BUF *p_rpc_buf, unsigned int tmp_data[]);

static bool get_pkt_info(unsigned int* pktnum, RPC_PKT* pkt_info, char* pdata)
{
	unsigned int pkt_num = *((unsigned int*)pdata);
	unsigned int idx = 0;
	unsigned int i = 0;

	CCCI_RPC_MSG("package number = 0x%08X\n", pkt_num);
	
	if(pkt_num > IPC_RPC_MAX_ARG_NUM)
		return false;
	
	idx = sizeof(unsigned int);
	for(i = 0; i < pkt_num; i++)
	{
		pkt_info[i].len = *((unsigned int*)(pdata + idx));
		idx += sizeof(unsigned int);
		pkt_info[i].buf = (pdata + idx);

		CCCI_RPC_MSG("pak[%d]: vir = 0x%08X, len = 0x%08X\n", i, (unsigned int)pkt_info[i].buf, pkt_info[i].len);

		// 4 byte alignment
		idx += ((pkt_info[i].len+3)>>2)<<2;
	}
	
	if(idx > IPC_RPC_MAX_BUF_SIZE)
	{
		CCCI_MSG_INF("rpc", "over flow, pdata = %p, idx = 0x%08X, max = %p\n", 
								pdata, idx, pdata + IPC_RPC_MAX_BUF_SIZE);
		return false;
	}
	*pktnum = pkt_num;
	
	return true;
}

static bool rpc_write(int buf_idx, RPC_PKT* pkt_src, unsigned int pkt_num)
{
	int ret = CCCI_FAIL;
	CCCI_BUFF_T     stream = { {0}, };
	RPC_BUF	*rpc_buf_tmp = NULL;
	unsigned char	*pdata = NULL;
	unsigned int 	data_len = 0;
	unsigned int 	i = 0;
	unsigned int	AlignLength = 0;

	rpc_buf_tmp = rpc_buf_vir + buf_idx;
	rpc_buf_tmp->op_id = IPC_RPC_API_RESP_ID | rpc_buf_tmp->op_id;
	pdata = rpc_buf_tmp->buf;
	*((unsigned int*)pdata) = pkt_num;

	pdata += sizeof(unsigned int);
	data_len += sizeof(unsigned int);

	for(i = 0; i < pkt_num; i++)
	{
		if((data_len + 2*sizeof(unsigned int) + pkt_src[i].len) > IPC_RPC_MAX_BUF_SIZE)
		{
			CCCI_MSG_INF("rpc", "Stream buffer full!!\n");
			goto _Exit;
		}
		
		*((unsigned int*)pdata) = pkt_src[i].len;
		pdata += sizeof(unsigned int);
		data_len += sizeof(unsigned int);
			
		// 4  byte aligned
		AlignLength = ((pkt_src[i].len + 3) >> 2) << 2;
		data_len += AlignLength;
			
		if(pdata != pkt_src[i].buf)
			memcpy(pdata, pkt_src[i].buf, pkt_src[i].len);
		else
			CCCI_RPC_MSG("same addr, no copy\n");
					
		pdata += AlignLength;
	}

    stream.data[0]  = rpc_buf_phy + (sizeof(RPC_BUF) * buf_idx);
    stream.data[1]  = data_len + 4;
    stream.reserved = buf_idx;

	CCCI_RPC_MSG("Write, %08X, %08X, %08X, %08X\n", 
			stream.data[0], stream.data[1], stream.channel, stream.reserved);

	ret = ccci_write(CCCI_RPC_TX, &stream);
	if(ret != CCCI_SUCCESS)
    {
		CCCI_MSG_INF("rpc", "fail send msg <%d>!!!\n", ret);
		return ret;
	}
			
_Exit:
	return ret;
}
static void ccci_rpc_work(struct work_struct *work  __always_unused)
{
	int pkt_num = 0;		
	int ret_val = 0;
	unsigned int buf_idx = 0;
	RPC_PKT pkt[IPC_RPC_MAX_ARG_NUM] = { {0}, };
	RPC_BUF *rpc_buf_tmp = NULL;
	unsigned int tmp_data[4];
	
	CCCI_RPC_MSG("ccci_rpc_work++\n");
				
	if(rpc_buf_vir == NULL)
	{
		CCCI_MSG_INF("rpc", "invalid rpc_buf_vir!!\n");
		return ;			
	}

	//RPC_DBG("rpc_buf: phy = 0x%08X, vir = 0x%08X, len = 0x%08X\n",
	//					rpc_buf_phy, rpc_buf_vir, rpc_buf_len);
	
	while(kfifo_out(&rpc_fifo, &buf_idx, sizeof(unsigned int)))
	{
//		int m = 0;
		if(buf_idx < 0 || buf_idx > IPC_RPC_REQ_BUFFER_NUM)
		{
			CCCI_MSG_INF("rpc", "invalid idx %d\n", buf_idx);
			ret_val = FS_PARAM_ERROR;
			pkt[pkt_num].len = sizeof(unsigned int);
			pkt[pkt_num++].buf = (void*) &ret_val;
			goto _Next;
		}	
		//RPC_DBG("rpc_buf: index = 0x%08X\n", buf_idx);

		pkt_num = 0;
		memset(pkt, 0x00, sizeof(RPC_PKT)*IPC_RPC_MAX_ARG_NUM);

		rpc_buf_tmp = rpc_buf_vir + buf_idx;
#if 0
		for(m = 0; m < IPC_RPC_MAX_BUF_SIZE + sizeof(unsigned int); m++)
		{
			if(m % 16 == 0)
				printk("\nSHM: ");
			printk("%02X ", *((unsigned char*)rpc_buf_tmp+m));					
			msleep(1);
		}				
		printk("\n\n");
#endif			
		if(!get_pkt_info(&pkt_num, pkt, rpc_buf_tmp->buf))
		{
			CCCI_MSG_INF("rpc", "Fail to get packet info\n");
			ret_val = FS_PARAM_ERROR;
			pkt[pkt_num].len = sizeof(unsigned int);
			pkt[pkt_num++].buf = (void*) &ret_val;
			goto _Next;
		}
				
		switch(rpc_buf_tmp->op_id)
		{
		default:
			ccci_rpc_work_helper(&pkt_num, pkt, rpc_buf_tmp, tmp_data);
			break;
		}
_Next:
		if(rpc_write(buf_idx, pkt, pkt_num) != CCCI_SUCCESS)
		{
			CCCI_MSG_INF("rpc", "fail to write packet!!\r\n");
			return ;
		}		
	} 

	CCCI_RPC_MSG("ccci_rpc_work--\n");    
}

static void ccci_rpc_callback(CCCI_BUFF_T *buff, void *private_data)
{
	if (buff->channel==CCCI_RPC_TX) 
		CCCI_MSG_INF("rpc", "Wrong CH for recv.\n");
	CCCI_RPC_MSG("callback, %08X, %08X, %08X, %08X\n", 
		buff->data[0], buff->data[1], buff->channel, buff->reserved);
	spin_lock_bh(&rpc_fifo_lock);
	kfifo_in(&rpc_fifo, &buff->reserved, sizeof(unsigned int));
	spin_unlock_bh(&rpc_fifo_lock);
   	schedule_work(&rpc_work);
	CCCI_RPC_MSG("callback --\n");    
}

int __init ccci_rpc_init(void)
{ 
    int ret;	
	
    ret=kfifo_alloc(&rpc_fifo,sizeof(unsigned) * IPC_RPC_REQ_BUFFER_NUM, GFP_KERNEL);
    if (ret)
    {
        CCCI_MSG_INF("rpc", "Unable to create fifo\n");
        return ret;
    }
  
#if 0
	rpc_work_queue = create_workqueue(WORK_QUE_NAME);
	if(rpc_work_queue == NULL)
	{
        printk(KERN_ERR "Fail create rpc_work_queue!!\n");
        return -EFAULT;
	}
#endif
    INIT_WORK(&rpc_work, ccci_rpc_work);

    // modem related channel registration.
    ASSERT(ccci_rpc_setup((int*)&rpc_buf_vir, &rpc_buf_phy, &rpc_buf_len) == CCCI_SUCCESS);
    CCCI_RPC_MSG("rpc_buf_vir=0x%p, rpc_buf_phy=0x%08X, rpc_buf_len=0x%08X\n", rpc_buf_vir, rpc_buf_phy, rpc_buf_len);
	ASSERT(rpc_buf_vir != NULL);
	ASSERT(rpc_buf_len != 0);
    ASSERT(ccci_register(CCCI_RPC_TX, ccci_rpc_callback, NULL) == CCCI_SUCCESS);
    ASSERT(ccci_register(CCCI_RPC_RX, ccci_rpc_callback, NULL) == CCCI_SUCCESS);

    return 0;
}

void __exit ccci_rpc_exit(void)
{
	CCCI_RPC_MSG("deinit\n");
#if 0
    if(cancel_work_sync(&rpc_work)< 0)
    {
		printk("ccci_rpc: Cancel rpc work fail!\n");
    }
	//destroy_workqueue(rpc_work_queue);
#endif
	
	kfifo_free(&rpc_fifo);
	ccci_unregister(CCCI_RPC_RX);
	ccci_unregister(CCCI_RPC_TX);

}

//EXPORT_SYMBOL(sej_sem);
#if 0
module_init(ccci_rpc_init);
module_exit(ccci_rpc_exit);

EXPORT_SYMBOL(SST_Secure_Init);
EXPORT_SYMBOL(SST_Secure_DeInit);
MODULE_DESCRIPTION("MEDIATEK CCCI RPC");
MODULE_AUTHOR("MEDIATEK");
MODULE_LICENSE("Proprietary");
#endif 
