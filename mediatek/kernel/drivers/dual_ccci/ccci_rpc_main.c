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
#include <ccci_common.h>

typedef struct test
{
	int op;
	char buf[0];
}test_t;

typedef struct _rpc_ctl_block
{
	spinlock_t			rpc_fifo_lock;
	RPC_BUF				*rpc_buf_vir;
	unsigned int		rpc_buf_phy;
	unsigned int		rpc_buf_len;
	struct kfifo		rpc_fifo;
	struct work_struct	rpc_work;
	int					m_md_id;
	int					rpc_smem_instance_size;
	int					rpc_max_buf_size;
	int					rpc_ch_num;
}rpc_ctl_block_t;

static rpc_ctl_block_t	*rpc_ctl_block[MAX_MD_NUM];

extern void ccci_rpc_work_helper(int md_id, int *p_pkt_num, RPC_PKT pkt[], RPC_BUF *p_rpc_buf, unsigned int tmp_data[]);

static int get_pkt_info(int md_id, unsigned int* pktnum, RPC_PKT* pkt_info, char* pdata)
{
	unsigned int	pkt_num = *((unsigned int*)pdata);
	unsigned int	idx = 0;
	unsigned int	i = 0;
	rpc_ctl_block_t	*ctl_b = rpc_ctl_block[md_id];

	CCCI_RPC_MSG(md_id, "package number = 0x%08X\n", pkt_num);

	if(pkt_num > IPC_RPC_MAX_ARG_NUM)
		return -1;

	idx = sizeof(unsigned int);
	for(i = 0; i < pkt_num; i++)
	{
		pkt_info[i].len = *((unsigned int*)(pdata + idx));
		idx += sizeof(unsigned int);
		pkt_info[i].buf = (pdata + idx);

		CCCI_RPC_MSG(md_id, "pak[%d]: vir = 0x%08X, len = 0x%08X\n", i, \
								(unsigned int)pkt_info[i].buf, pkt_info[i].len);

		// 4 byte alignment
		idx += ((pkt_info[i].len+3)>>2)<<2;
	}

	if(idx > ctl_b->rpc_max_buf_size)
	{
		CCCI_MSG_INF(md_id, "rpc", "over flow, pdata = %p, idx = 0x%08X, max = %p\n", 
								pdata, idx, pdata + ctl_b->rpc_max_buf_size);
		return -1;
	}
	*pktnum = pkt_num;

	return 0;
}


static int rpc_write(int md_id, int buf_idx, RPC_PKT* pkt_src, unsigned int pkt_num)
{
	int				ret = 0;
	ccci_msg_t		msg;
	RPC_BUF			*rpc_buf_tmp = NULL;
	unsigned char	*pdata = NULL;
	unsigned int 	data_len = 0;
	unsigned int 	i = 0;
	unsigned int	AlignLength = 0;
	rpc_ctl_block_t	*ctl_b = rpc_ctl_block[md_id];

	//rpc_buf_tmp = ctl_b->rpc_buf_vir + buf_idx;
	rpc_buf_tmp = (RPC_BUF*)((unsigned int)(ctl_b->rpc_buf_vir) + ctl_b->rpc_smem_instance_size*buf_idx);
	rpc_buf_tmp->op_id = IPC_RPC_API_RESP_ID | rpc_buf_tmp->op_id;
	pdata = rpc_buf_tmp->buf;
	*((unsigned int*)pdata) = pkt_num;

	pdata += sizeof(unsigned int);
	data_len += sizeof(unsigned int);

	for(i = 0; i < pkt_num; i++)
	{
		if((data_len + 2*sizeof(unsigned int) + pkt_src[i].len) > ctl_b->rpc_max_buf_size)
		{
			CCCI_MSG_INF(md_id, "rpc", "Stream buffer full!!\n");
			ret = -CCCI_ERR_LARGE_THAN_BUF_SIZE;
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
			CCCI_RPC_MSG(md_id, "same addr, no copy\n");

		pdata += AlignLength;
	}

	//msg.data0  = ctl_b->rpc_buf_phy + (sizeof(RPC_BUF) * buf_idx);
	msg.data0 = (unsigned int)(ctl_b->rpc_buf_phy) + ctl_b->rpc_smem_instance_size*buf_idx;
	msg.data1  = data_len + 4;
	msg.reserved = buf_idx;
	msg.channel = CCCI_RPC_TX;

	CCCI_RPC_MSG(md_id, "Write, %08X, %08X, %08X, %08X\n", 
			msg.data0, msg.data1, msg.channel, msg.reserved);

	mb();
	ret = ccci_message_send(md_id, &msg, 1);
	if(ret != sizeof(ccci_msg_t))
	{
		CCCI_MSG_INF(md_id, "rpc", "fail send msg <%d>!!!\n", ret);
		return ret;
	} else 
		ret = 0;

_Exit:
	return ret;
}

static void ccci_rpc_work(struct work_struct *work)
{
	int				pkt_num = 0;
	int				ret_val = 0;
	unsigned int	buf_idx = 0;
	RPC_PKT			pkt[IPC_RPC_MAX_ARG_NUM] = { {0}, };
	RPC_BUF			*rpc_buf_tmp = NULL;
	unsigned int	tmp_data[4];
	rpc_ctl_block_t	*ctl_b = container_of(work, rpc_ctl_block_t, rpc_work);
	int				md_id= ctl_b->m_md_id;

	CCCI_RPC_MSG(md_id, "ccci_rpc_work++\n");

	if(ctl_b->rpc_buf_vir == NULL)
	{
		CCCI_MSG_INF(md_id, "rpc", "invalid rpc_buf_vir!!\n");
		return;
	}

	while(kfifo_out(&ctl_b->rpc_fifo, &buf_idx, sizeof(unsigned int)))
	{
		if(buf_idx < 0 || buf_idx > ctl_b->rpc_ch_num)
		{
			CCCI_MSG_INF(md_id, "rpc", "invalid idx %d\n", buf_idx);
			ret_val = FS_PARAM_ERROR;  // !!!!! Make more meaningful
			pkt[pkt_num].len = sizeof(unsigned int);
			pkt[pkt_num++].buf = (void*) &ret_val;
			goto _Next;
		}

		pkt_num = 0;
		memset(pkt, 0x00, sizeof(RPC_PKT)*IPC_RPC_MAX_ARG_NUM);

		//rpc_buf_tmp = ctl_b->rpc_buf_vir + buf_idx;
		rpc_buf_tmp = (RPC_BUF*)((unsigned int)(ctl_b->rpc_buf_vir) + ctl_b->rpc_smem_instance_size*buf_idx);

		if(get_pkt_info(md_id, &pkt_num, pkt, rpc_buf_tmp->buf) < 0)
		{
			CCCI_MSG_INF(md_id, "rpc", "Fail to get packet info\n");
			ret_val = FS_PARAM_ERROR;  // !!!!! Make more meaningful
			pkt[pkt_num].len = sizeof(unsigned int);
			pkt[pkt_num++].buf = (void*) &ret_val;
			goto _Next;
		}
				
		switch(rpc_buf_tmp->op_id)
		{
		default:
			ccci_rpc_work_helper(md_id, &pkt_num, pkt, rpc_buf_tmp, tmp_data);  // !!!!! Make more meaningful
			break;
		}
_Next:
		if(rpc_write(md_id, buf_idx, pkt, pkt_num) != 0)
		{
			CCCI_MSG_INF(md_id, "rpc", "fail to write packet!!\r\n");
			return ;
		}
	} 

	CCCI_RPC_MSG(md_id, "ccci_rpc_work--\n");    
}


static void ccci_rpc_callback(void *private)
{
	logic_channel_info_t	*ch_info = (logic_channel_info_t*)private;
	ccci_msg_t				msg;
	rpc_ctl_block_t			*ctl_b = (rpc_ctl_block_t *)ch_info->m_owner;
	int						md_id = ctl_b->m_md_id;

	while(get_logic_ch_data(ch_info, &msg)){
		CCCI_RPC_MSG(md_id, "callback, %08X, %08X, %08X, %08X\n", 
							msg.data0, msg.data1, msg.channel, msg.reserved);
		spin_lock_bh(&ctl_b->rpc_fifo_lock);
		kfifo_in(&ctl_b->rpc_fifo, &msg.reserved, sizeof(unsigned int));
		spin_unlock_bh(&ctl_b->rpc_fifo_lock);
	}

	schedule_work(&ctl_b->rpc_work);
	CCCI_RPC_MSG(md_id, "callback --\n");    
}

int __init ccci_rpc_init(int md_id)
{ 
	int 				ret;
	rpc_ctl_block_t		*ctl_b;
	rpc_cfg_inf_t		rpc_cfg;
	int					rpc_buf_vir, rpc_buf_phy, rpc_buf_len;

	// Allocate fs ctrl struct memory
	ctl_b = (rpc_ctl_block_t *)kmalloc(sizeof(rpc_ctl_block_t), GFP_KERNEL);
	if(ctl_b == NULL)
		return -CCCI_ERR_GET_MEM_FAIL;
	memset(ctl_b, 0, sizeof(rpc_ctl_block_t));
	rpc_ctl_block[md_id] = ctl_b;

	// Get rpc config information
	ASSERT(ccci_get_sub_module_cfg(md_id, "rpc", (char*)&rpc_cfg, sizeof(rpc_cfg_inf_t)) \
				== sizeof(rpc_cfg_inf_t) );

	ASSERT(ccci_rpc_base_req(md_id, &rpc_buf_vir, &rpc_buf_phy, &rpc_buf_len) == 0);
	ctl_b->rpc_buf_vir = (RPC_BUF*)rpc_buf_vir;
	ctl_b->rpc_buf_phy = (unsigned int)rpc_buf_phy;
	ctl_b->rpc_buf_len = rpc_buf_len;
	ctl_b->rpc_max_buf_size = rpc_cfg.rpc_max_buf_size;
	ctl_b->rpc_ch_num = rpc_cfg.rpc_ch_num;
	ctl_b->rpc_smem_instance_size = sizeof(RPC_BUF) + ctl_b->rpc_max_buf_size;
	//Note!!!!! we should check cofigure mistake

	// Init ctl_b
	ctl_b->m_md_id = md_id;
	spin_lock_init(&ctl_b->rpc_fifo_lock);


	ret=kfifo_alloc(&ctl_b->rpc_fifo,sizeof(unsigned) * ctl_b->rpc_ch_num, GFP_KERNEL);

	if (ret<0)
	{
		CCCI_MSG_INF(md_id, "rpc", "Unable to create fifo\n");
		goto _KFIFO_ALLOC_FAIL;
	}

	INIT_WORK(&ctl_b->rpc_work, ccci_rpc_work);

	// modem related channel registration.
	CCCI_RPC_MSG(md_id, "rpc_buf_vir=0x%p, rpc_buf_phy=0x%08X, rpc_buf_len=0x%08X\n", \
								ctl_b->rpc_buf_vir, ctl_b->rpc_buf_phy, ctl_b->rpc_buf_len);
	ASSERT(ctl_b->rpc_buf_vir != NULL);
	ASSERT(ctl_b->rpc_buf_len != 0);
	ASSERT(register_to_logic_ch(md_id, CCCI_RPC_RX, ccci_rpc_callback, ctl_b) == 0);

	return 0;

_KFIFO_ALLOC_FAIL:
	kfree(ctl_b);
	rpc_ctl_block[md_id] = NULL;

	return ret;
}

void __exit ccci_rpc_exit(int md_id)
{
	rpc_ctl_block_t		*ctl_b;

	CCCI_RPC_MSG(md_id, "ccci_rpc_exit\n");
	ctl_b = rpc_ctl_block[md_id];
	if (ctl_b == NULL)
		return;
	
	kfifo_free(&ctl_b->rpc_fifo);
	un_register_to_logic_ch(md_id, CCCI_RPC_RX);
	kfree(ctl_b);
	rpc_ctl_block[md_id] = NULL;
}


