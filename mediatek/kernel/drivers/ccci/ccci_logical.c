#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <mach/irqs.h>
#include <linux/kallsyms.h>
#include <ccci.h>
#include <linux/delay.h>

extern unsigned long long lg_ch_tx_debug_enable;
extern unsigned long long lg_ch_rx_debug_enable;
//static unsigned int tasklet_num = 0;  // Fix tasklet_num build waring
unsigned int error[CCCI_MAX_CHANNEL][ERR_CODE_NUM];
#define ISR_ERR_THREASH 100



static void logical_layer_tasklet(unsigned long data)
{
	struct logical_layer *lg_layer=(struct logical_layer *)data;
	struct logical_channel *client;
	int i=0;
	int size;
	CCCI_BUFF_T buff;
	CCCI_CALLBACK call_back;
	void *para;
	
	for (i=0;i<lg_layer->lc_num;i++)
	{
		read_lock_irq(&lg_layer->lock);
		client=lg_layer->lg_array[i];
		read_unlock_irq(&lg_layer->lock);
		if (client)
		{
			spin_lock_irq(&client->lock);
			call_back=client->callback;
			para=client->private_data;
			while (!kfifo_is_empty(&client->fifo))
			{	
				size=kfifo_out(&client->fifo,&buff,sizeof(buff));
				spin_unlock_irq(&client->lock);
				WARN_ON(!call_back);
				
				(*call_back)(&buff,para);
				spin_lock_irq(&client->lock);
			}
			spin_unlock_irq(&client->lock);
		}
	
	}

}

static int lg_add_client(struct logical_layer *lg_layer,int num,int buf_num,struct logical_channel **lg_chptr)
{
	int ret=0;
	struct logical_channel *client;
	int i = 0;
	
	if (num>=lg_layer->lc_num)
	{
		CCCI_MSG("Invalid lg_ch Num %d.\n",num);
		*lg_chptr=NULL;
		return -EINVAL;
		
	}
	write_lock_irq(&lg_layer->lock);
	WARN_ON(!lg_layer->lg_array);
	if (lg_layer->lg_array[num])
	{
		ret=-EEXIST;
		CCCI_MSG("struct logical_channel[%s:%d] has registered by %s\n",
			lg_layer->lg_array[num]->name,num,lg_layer->lg_array[num]->owner);
		*lg_chptr=lg_layer->lg_array[num];
	}
	else {
		client=kzalloc(sizeof(struct logical_channel),GFP_ATOMIC) ;
		if (client==NULL)
		{
			ret=-ENOMEM;
			CCCI_MSG("kzalloc for struct logical_channel fialed !\n");			
		}	
		else 
		{	
			//printk("[CCCI] allocate client %x for num:%d\n", client, num);
			CCCI_CTL_MSG("allocate struct logical_channel for lg_ch:%d\n", num);	
			lg_layer->lg_array[num]=client;
			client->buf_num=buf_num;
			atomic_inc(&lg_layer->user);
		}
		*lg_chptr=client;
	}

	write_unlock_irq(&lg_layer->lock);

	for(i = 0; i < ERR_CODE_NUM; i++) 
		error[num][i] = 0;
	
	return ret;
}

static int lg_remove_client(struct logical_layer *lg_layer,int num)
{
	int ret=0;
	struct logical_channel *client=NULL;
	
	if (num>=lg_layer->lc_num)
	{
		CCCI_MSG("Invalid lg_ch Num=%d.\n",num);
		return -EINVAL;
	}
	
	write_lock_irq(&lg_layer->lock);
	WARN_ON(!lg_layer->lg_array);
	if (lg_layer->lg_array[num]==NULL)
	{
		CCCI_MSG("Remove a Null Client.\n ");
		ret=-EINVAL;
		goto out;
	}
	client=lg_layer->lg_array[num];
	lg_layer->lg_array[num]=NULL;
	atomic_dec(&lg_layer->user);
	
out:
	write_unlock_irq(&lg_layer->lock);
	if (ret==0)
	{
		while (test_bit(CCCI_RUNNING, &lg_layer->pc_channel->flags)
			||test_bit(TASKLET_STATE_RUN,&lg_layer->tasklet.state))
		{
			yield();
		}
		if (client->dtor)
			client->dtor(client);
		kfree(client);
	}
	
	return ret;
}
#ifdef __CCCI_LOG__
static inline void  lg_ch_log(CCCI_LOG_T *log,CCCI_BUFF_T *data,int rx)
{
	struct timespec ts = current_kernel_time();
	
	log->sec=ts.tv_sec;
	log->nanosec=ts.tv_nsec;
	log->rx=rx;
	//log->buff=*data;
	ccci_buff_t_assign(&log->buff,data);
}

int s_to_date(unsigned long seconds, unsigned long usec, unsigned int *us,
					unsigned int *sec, unsigned int *min, unsigned int *hour,
					unsigned int *day, unsigned int *month, unsigned int *year)
{
#define  DAY_PER_LEAP_YEAR		366
#define  DAY_PER_NON_LEAP_YEAR	365

	unsigned int i = 0;
	unsigned long mins, hours, days, month_t, year_t;
	unsigned char m[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	if(!sec || !min || !hour || !day || !month || !year)
	{
		//printk("%s invalid param!\n", __FUNCTION__);
		CCCI_MSG("<ctl>%s invalid param!\n", __FUNCTION__);
		return (-1);
	}

	*us = usec;
	*sec = seconds % 60;
	mins = seconds / 60;
	*min = mins % 60;
	hours = mins / 60;
	*hour = hours % 24;
	days = hours / 24;
	
	year_t = 1970;

	while(1)
	{
		if(!(year_t % 4) && (year_t % 100))
		{
			if(days >= DAY_PER_LEAP_YEAR)
			{
				days -= DAY_PER_LEAP_YEAR;
				year_t ++;  
			}
			else
				break;
		}
		else
		{
			if(days >= DAY_PER_NON_LEAP_YEAR)
			{
				days -= DAY_PER_NON_LEAP_YEAR;
				year_t ++;  
			}
			else
				break;
		}
	}

	if(!(year_t % 4) && year_t % 100)
	{
		m[1] = 29;
	}

	month_t = 1;
	for(i=0; i < 12; i++)
	{
		if(days > m[i])
		{
			days -= m[i];
			month_t++;
		}
		else
		{
			*day = days;
			break;
		}
	}	

	*year = year_t;
	*month = month_t;
	
	return 0;
}

static int lg_ch_log_dump(CCCI_LOG_T  *log)
{
	unsigned int ms, sec, min, hour, day, month, year;	
	if (log->sec==0&&log->nanosec==0)
		return 0;
	s_to_date(log->sec, log->nanosec/1000,
		&ms, &sec, &min, &hour, &day, &month, &year);
	//CCCI_MSG("\n%s (Time:%d-%02d-%02d %02d:%02d:%02d.%06d  )\n",log->rx?"RX":"TX",
	//	year, month, day, hour, min, sec, ms);
	//CCCI_MSG("Data:%08X %08X %02d %08X \n",
	//	log->buff.data[0],log->buff.data[1],log->buff.channel,log->buff.reserved);
	CCCI_MSG("%08X %08X %02d %08X   %d-%02d-%02d %02d:%02d:%02d.%06d\n",
		log->buff.data[0],log->buff.data[1],log->buff.channel,log->buff.reserved,
		year, month, day, hour, min, sec, ms);
	return 1;
}
#endif

inline bool lg_ch_is_mailbox(unsigned int ch)
{
	if((ch >= CCCI_CONTROL_RX) &&(ch <= CCCI_PCM_TX))
		return true;
	else if((ch >= CCCI_UART1_RX) && (ch <= CCCI_MD_LOG_TX))
		return false;
	else
		return false;
}


static int lg_process_data(struct logical_layer *lg_layer,CCCI_BUFF_T *data,int rx,int drop)
{
	int ret=0;
	int size;
	unsigned long flag;
	struct logical_channel *lg_ch;
	unsigned int ch = data->channel;
	read_lock_irqsave(&lg_layer->lock,flag);
	WARN_ON(!lg_layer->lg_array);
	
	if (unlikely(ch >= lg_layer->lc_num))
	{
		ret=-EINVAL;
		read_unlock_irqrestore(&lg_layer->lock,flag);
		CCCI_MSG("Invalid channel(%s): ch=%d \n", (rx?"RX":"TX"), ch);
		goto out;
	}
	
	if (lg_layer->lg_array[ch] == NULL)
	{
		ret=-EINVAL;
		read_unlock_irqrestore(&lg_layer->lock,flag);
		
		if((rx == 0) || lg_ch_is_mailbox(ch))
			CCCI_MSG("Wrong data(%s): %08x, %08x, %02d, %08x \n", (rx?"RX":"TX"),
				data->data[0], data->data[1],data->channel, data->reserved);
		else {
			error[ch][CLIENT_FAIL]++;
			if(error[ch][CLIENT_FAIL] > ISR_ERR_THREASH) {
				CCCI_MSG("Wrong data(%s): %08x, %08x, %02d, %08x \n", (rx?"RX":"TX"),
					data->data[0], data->data[1], data->channel, data->reserved);		
				error[ch][CLIENT_FAIL] = 0;
			}
		}
		
		goto out;
	}
	error[ch][CLIENT_FAIL] = 0;	
	
	lg_ch=lg_layer->lg_array[ch];
	read_unlock_irqrestore(&lg_layer->lock,flag);
	
	spin_lock_irqsave(&lg_ch->lock,flag);
	if (rx)
	{
		if (lg_ch->have_fifo==0)
		{
			ret=kfifo_alloc(&lg_ch->fifo,lg_ch->buf_num*sizeof(CCCI_BUFF_T),GFP_ATOMIC);
			if (ret)
			{
				if(unlikely(lg_ch_is_mailbox(ch)))
					CCCI_MSG("kfifo alloc fail: %d, %d \n", ch, ret);
				else {
					error[ch][KFIFO_ALLOC_FAIL]++;
					if(error[ch][KFIFO_ALLOC_FAIL] > ISR_ERR_THREASH) {
						CCCI_MSG("kfifo alloc fail: %d, %d \n", ch, ret);
						error[ch][KFIFO_ALLOC_FAIL] = 0;
					}
				}
				goto out_unlock;
			}
			lg_ch->have_fifo=1;
			error[ch][KFIFO_ALLOC_FAIL] = 0;
		}
		
		if(kfifo_is_full(&lg_ch->fifo))
		{
			ret=-ENOMEM;
			if(unlikely(lg_ch_is_mailbox(ch)))
				CCCI_MSG("kfifo full: Owner:%s size:%d (%08X %08X %02d %08X)\n",
				lg_ch->owner, kfifo_size(&lg_ch->fifo),data->data[0], 
				data->data[1], data->channel, data->reserved);
			else {
				error[ch][KFIFO_FULL]++;
				if(error[ch][KFIFO_FULL] > ISR_ERR_THREASH) {
					CCCI_MSG("kfifo full: Owner:%s size:%d (%08X %08X %02d %08X)\n",
						lg_ch->owner, kfifo_size(&lg_ch->fifo),data->data[0], 
						data->data[1], data->channel, data->reserved);
					error[ch][KFIFO_FULL] = 0;
				}
			}
			
			lg_ch->lc_statics.rx_drop_num++;
			goto out_unlock;
		}
		error[ch][KFIFO_FULL] = 0;
		
		size=kfifo_in(&lg_ch->fifo,data,sizeof(*data))	;
		lg_ch->lc_statics.rx_num++;
		WARN_ON(size!=sizeof(*data));
		ret=sizeof(*data);
	}
	else {
		if (drop)
			lg_ch->lc_statics.tx_drop_num++;
		else {
			lg_ch->lc_statics.tx_num++;	
			ret=sizeof(*data);
		}
	}

#ifdef __CCCI_LOG__
	do{
		int idx=(lg_ch->lc_statics.log_idx++)%ARRAY_SIZE(lg_ch->lc_statics.log);
		lg_ch_log(&lg_ch->lc_statics.log[idx],data,rx);		
	}while(0);
#endif

out_unlock:
	spin_unlock_irqrestore(&lg_ch->lock,flag);
	//if (rx&&ret==sizeof(*data))
	if ( rx && (0 != lg_ch->have_fifo) ) {
		tasklet_schedule(&lg_layer->tasklet);
	}

out:
	return ret;
}

static void dump_lg_ch(struct logical_channel *client,int ch)
{
	if (client)
	{	
		spin_lock_irq(&client->lock);
		//CCCI_MSG("******************Dump CH:%d****************\n",ch);
		//CCCI_MSG("MaxBufferMsg:%d Name:%15s Owner:%s\n",
		//	kfifo_size(&client->fifo)/sizeof(CCCI_BUFF_T),client->name,client->owner);
		//print_symbol("callback:%s\t",(unsigned long)client->callback);
		//CCCI_MSG("Private_data:%p\n",client->private_data);
		//CCCI_MSG("rx_num:%ld\trx_drop_num:%ld\n",client->lc_statics.rx_num,
		//	client->lc_statics.rx_drop_num);
		//CCCI_MSG("tx_num:%ld\ttx_drop_num:%ld\n",client->lc_statics.tx_num,
		//	client->lc_statics.tx_drop_num);
		
		CCCI_MSG("\n");
		CCCI_MSG("ch%02d rx:%ld\t rx_fail:%ld\t tx:%ld\t tx_fail:%ld\n", ch,
			client->lc_statics.rx_num, client->lc_statics.rx_drop_num, 
			client->lc_statics.tx_num, client->lc_statics.tx_drop_num);
		#ifdef __CCCI_LOG__
		do{
			int i=0;
			for (i=0;i<ARRAY_SIZE(client->lc_statics.log);i++)
			{
				if ( !lg_ch_log_dump(client->lc_statics.log+((client->lc_statics.log_idx-i-1)
					%ARRAY_SIZE(client->lc_statics.log)))) 
				{
					//CCCI_DEBUG("No log to dump(%d %d)\n",i,client->lc_statics.log_idx);
					 break;
				}
			}
		}while(0);
		#endif
		//CCCI_MSG("*****************End Dump CH:%d****************\n",ch);
		spin_unlock_irq(&client->lock);
	}
	mdelay(2);

}

static void lg_dump(struct logical_layer *lg_layer,unsigned int nr)
{
	int i=0;
	struct logical_channel *client;
	set_bit(CCCI_RUNNING,&lg_layer->pc_channel->flags);
	read_lock_irq(&lg_layer->lock);
	if (nr==-1U)
	{
		for (i=0;i<lg_layer->lc_num;i++)
		{
			
			client=lg_layer->lg_array[i];
			
			dump_lg_ch(client,i);
		}
	}
	else if (nr<lg_layer->lc_num)
	{
		dump_lg_ch(lg_layer->lg_array[nr],nr);
	}
	read_unlock_irq(&lg_layer->lock);	
	clear_bit(CCCI_RUNNING,&lg_layer->pc_channel->flags);
}


 int ccci_logical_layer_init(struct physical_channel *p_ch,struct logical_layer *lg_layer,int ch_num)
{
	int ret=0;
	WARN_ON(lg_layer->lg_array);
	lg_layer->lg_array=kzalloc(sizeof(struct logical_channel *)*ch_num,GFP_KERNEL);
	if (lg_layer->lg_array==NULL) 
	{
		ret=-ENOMEM;
		goto out;
	}
	lg_layer->pc_channel=p_ch;
	lg_layer->lc_num=ch_num;
	lg_layer->add_client=lg_add_client;
	lg_layer->remove_client=lg_remove_client;
	lg_layer->process_data=lg_process_data;
	lg_layer->dump=lg_dump;
	tasklet_init(&lg_layer->tasklet,logical_layer_tasklet,(unsigned long)lg_layer);
	atomic_set(&lg_layer->user,0);
	rwlock_init(&lg_layer->lock);
	
out:
	return ret;

}

int ccci_logical_layer_destroy(struct logical_layer *lg_layer)
{
	BUG_ON(lg_layer==NULL);
	if (atomic_read(&lg_layer->user))
	{
		return -EBUSY;
	}
	kfree(lg_layer->lg_array) ;
	lg_layer->lc_num=0;
	lg_layer->lg_array=NULL;
	tasklet_kill(&lg_layer->tasklet);
	return 0;
}


