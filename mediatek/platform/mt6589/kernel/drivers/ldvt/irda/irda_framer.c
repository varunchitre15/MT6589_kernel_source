#include <linux/timer.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <mach/irqs.h>
#include <asm/io.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <mach/dma.h>
#include <linux/dma-mapping.h>
#include <asm/uaccess.h>
#include <asm/page.h>
#include <linux/wait.h>
#include "irframer_hw.h"

static int IrClock=61440000;
DECLARE_WAIT_QUEUE_HEAD(err_wait);
static int mt_irda_net_open(struct net_device *dev);
static int mt_irda_net_close(struct net_device *dev);
static netdev_tx_t mt_irda_xmit(struct sk_buff *skb,struct net_device *dev);
static int mt_irda_ioctl(struct net_device *dev,struct ifreq *ifr,int cmd);
static void mt_irda_start_rx(struct mt_irda_framer *framer);
static void mt_irda_start_tx(struct mt_irda_framer *framer);
static ssize_t mt_irda_show_stat(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t mt_irda_store_stat(struct device *dev, struct device_attribute *attr,const char *buf, size_t count);
static int mt_irda_change_speed(struct mt_irda_framer *framer);	
struct mt_irda_framer *framer_dev;
static int rx_invert=0;
static int tx_invert=0;
static int hard_ware=1;
module_param(tx_invert,int,0664);
module_param(rx_invert,int,0664);
module_param(hard_ware,int,0664);

#ifdef MT6573
static DMA_CHAN irda_dma=
{
	.baseAddr =DMA_BASE_CH(P_DMA_IRDA),
	.chan_num =P_DMA_IRDA,
	.registered =1,
	.type =DMA_HALF_CHANNEL,
	.callback=irda_dma_callback;
};

#elif defined(MT6575)
static struct irda_dma_conf irda_dma ;	


#endif
static const struct net_device_ops mt_irda_netdev_ops=
{
		.ndo_open = mt_irda_net_open,
		.ndo_stop = mt_irda_net_close,
		.ndo_start_xmit = mt_irda_xmit,
		.ndo_do_ioctl = mt_irda_ioctl, 
	};
	
	
static struct device_attribute irda_dev_attr =
		__ATTR(mt65xx-irda,S_IRUGO|S_IWUGO,mt_irda_show_stat,mt_irda_store_stat);
		
		
static struct attribute *irda_attr[] = {&irda_dev_attr.attr,NULL};

static const struct attribute_group mt_irda_attribute_group =
{
	 .attrs=irda_attr,	
};
static const struct attribute_group *mt_irda_attribute_group_p[]=
{
	&mt_irda_attribute_group,
	NULL	
};
static  struct device_type mt_irda_dev_type = 
{
			.groups=mt_irda_attribute_group_p,
};


const char *speed_str(int speed)
{
	if (speed==115200)
		return "115200";
	if (speed==9600)
		return "9600";
	if (speed==576000)
		return "576000";
	if (speed==1152000)
		return "1152000";
	if (speed==4000000)
		return "4000000";
		return "Unknown Speed";
}
static ssize_t mt_irda_show_stat(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct net_device *net_dev=container_of(dev,struct net_device,dev);	
	struct mt_irda_framer *framer=netdev_priv(net_dev);
	ssize_t count=0;
	unsigned long flag ;
	spin_lock_irqsave(&framer->lock,flag);
	count = sprintf(buf,
"********************mt65xx-irda-status******************\n\
speed	:%s [9600 115200 576000 1152000 4000000]\n\
^(don't change if you are not sure)\n\
ir_div 	:%d \n\
rx_invert :%3s [y n]\n\
tx_invert :%3s [y n]\n\
\n\
SIR Mode (9600<--->115200):\n\
	modulation_type :%6s [3/16 1.61us]\n\
	bof_type        :%6s [C0H FFH]\n\
	sir_framing	:%6s [ignore check]\n\
\n	  ^\
	 (used for ignore or check stop bit.)\n\
\n\
MIR Mode (576000<--->1152000):\n\
	mir_576_tune	:%6s [none half quter]\n\
\n\n\n\
write to this file using the following format :\n\
speed=[...] rx_invert=[...] tx_invert=[...] modulation_type=[...] bof_type=[...] sir_framing=[...] mir_576_tune=[...]\n\n\
*********************mt65xx-irda-status**********************\n\
\n",
	speed_str(framer->speed),
	inw(IR_DIV),
	framer->rx_invert==IR_RX_EN_INVERT?"yes":"no",
	framer->tx_invert==IR_TX_EN_INVERT?"yes":"no",
	framer->sir_mode==IR_DIV_16?"3/16":"1.61us",
	framer->bof_type==BOF_TYPE_C0H?"C0H":"FFH",
	framer->sir_framing_set==SIR_FRAMING_IGN_STOP_BIT?"ignore":"check",
	framer->mir_576_tune==IR_TX_MIR576_NONE?"none":(framer->mir_576_tune==IR_TX_MIR576_RX_HALF?"half":"quter"));
	{
		unsigned int i ;
		for (i=IRDA_base;i <=IR_FIR_DIV; i+=4 )
		{
			count += sprintf(buf+count,"%#8x : %#8x\n",i,inw(i));
		}
		count +=sprintf(buf+count,"hard_ware=%d \n",hard_ware)  ;
	}
	spin_unlock_irqrestore(&framer->lock,flag);
	return count;	
}


static const char *irda_get_option(const char *str,const char *op)
{
	char *substr=strstr(str,op);
	if (substr)
	{
		const char *oop=substr+strlen(op);
		if (*oop==' '||*oop=='\t' || *oop=='\0') goto out;
		return oop;
	}
out:
	return NULL;
}
static void parse_speed(const char *op)
{
		long speed;
		if (op==NULL) return ;
		speed=simple_strtol(op,NULL,0);
		framer_dev->new_speed=speed;
		if (mt_irda_change_speed(framer_dev)!=1)
			dbg("mt65xx-irda-status: change speed failed!\n");
		else mt_irda_start_rx(framer_dev);
}
static void parse_rx_invert(const char *op)
{
		int old=framer_dev->rx_invert;
		if (op==NULL) return ;
		if (!strncmp(op,"yes",3) || !strncmp(op,"y",1))
		{
			framer_dev->rx_invert=IR_RX_EN_INVERT;
		}
		else if (!strncmp(op,"no",2) || !strncmp(op,"n",1))
			framer_dev->rx_invert=0;
		if (old != framer_dev->rx_invert)
			mt_irda_start_rx(framer_dev);
		return ;
}
static void parse_tx_invert(const char *op)
{
	if (op==NULL) return ;
	if (!strncmp(op,"yes",3) || !strncmp(op,"y",1))
		{
			framer_dev->tx_invert=IR_TX_EN_INVERT;
		}
		else if (!strncmp(op,"no",2) || !strncmp(op,"n",1))
			framer_dev->tx_invert=0;
		return ;
}
static void parse_modulation_type(const char *op)
{
		int old = framer_dev->sir_mode;
		if (op==NULL) return ;
		if (!strncmp(op,"3/16",4) )
		 framer_dev->sir_mode=IR_DIV_16;
		else if (!strncmp(op,"1.61us",6))
			framer_dev->sir_mode=IR_1_61_us;
		if (old != framer_dev->sir_mode && framer_dev->speed<=115200)
			mt_irda_start_rx(framer_dev);
		return ;
}
static void parse_bof_type(const char *op)
{
		int old=framer_dev->bof_type;
		if (op==NULL) return ;
		if (!strncmp(op,"C0H",3)||!strncmp(op,"c0h",3))	
			framer_dev->bof_type=BOF_TYPE_C0H;
		else if (!strncmp(op,"FFH",3)||!strncmp(op,"ffh",3))
			framer_dev->bof_type=BOF_TYPE_FFH;
		if (old != framer_dev->bof_type && framer_dev->speed <= 115200)
			mt_irda_start_rx(framer_dev);
			return ;
}
static void  parse_sir_framing(const char *op)
{			
			int old = framer_dev->sir_framing_set;
			if (op==NULL) return ;
			if (!strncmp(op,"ignore",6))	
				framer_dev->sir_framing_set=SIR_FRAMING_IGN_STOP_BIT;
			else if (!strncmp(op,"check",5))
				framer_dev->sir_framing_set=SIR_FRAMING_CHK_STOP_BIT;
			if(old != framer_dev->sir_framing_set && framer_dev->speed <=115200)
				mt_irda_start_rx(framer_dev);
			return ;
}
static void parse_mir_576_tune(const char *op)
{
		int old=framer_dev->mir_576_tune;
		if (op==NULL) return ;
			if (!strncmp(op,"none",4))	
				framer_dev->mir_576_tune=IR_TX_MIR576_NONE;
			else if (!strncmp(op,"half",4))	
				framer_dev->mir_576_tune=IR_TX_MIR576_RX_HALF;	
			else if (!strncmp(op,"quter",5))	
				framer_dev->mir_576_tune=IR_TX_MIR576_RX_QUTER;	
		if(old!=framer_dev->mir_576_tune && (framer_dev->speed==576000 ||framer_dev->speed==1152000))
			mt_irda_start_rx(framer_dev);
			return ;
}
static void parse_ir_div(const char *op)
{
	u16 div=(u16)simple_strtol(op,NULL,0);
	if (div && framer_dev->speed <=115200)
	{	
		outw(div,IR_DIV);
		mt_irda_start_rx(framer_dev);
	}
	return ;	
}
static struct option_array op_array[]=
{
	{.option="speed=", .parse=parse_speed,} ,
	{.option="ir_div=", .parse=parse_ir_div,},
	{.option="rx_invert=", .parse= parse_rx_invert,},
	{.option="tx_invert=", .parse= parse_tx_invert,},
	{.option="modulation_type=", .parse=parse_modulation_type,},
	{.option="bof_type=", .parse=parse_bof_type,},
	{.option="sir_framing=", .parse=parse_sir_framing,},	
	{.option="mir_576_tune=", .parse=parse_mir_576_tune,},
	{.option=NULL, .parse=NULL,},
};
static ssize_t mt_irda_store_stat(struct device *dev, struct device_attribute *attr,const char *buf, size_t count)
{
//	const char *option[]={"speed=","rx_invert=","tx_invert=","modulation_type=","bof_type=","sir_framing=","mir_576_tune=",NULL};
	struct net_device *net_dev=container_of(dev,struct net_device,dev);	
	struct mt_irda_framer *framer=netdev_priv(net_dev);
	unsigned long flag ;
	const char *op=NULL;
	spin_lock_irqsave(&framer->lock,flag);
	if (framer->direction==IO_XMIT || (inw(IR_FIFO_STA)&0x5) != 0x5 || (inw(IR_FIFO_STA)&0x10))
	{
		spin_unlock_irqrestore(&framer->lock,flag);
		return -EBUSY;
	}
	else 
	{
			int i=0;
			while (op_array[i].option )
			{
				op=irda_get_option(buf,op_array[i].option);
				if (op && op_array[i].parse )
					(*op_array[i].parse)(op);
				i++;
			}
	}
	spin_unlock_irqrestore(&framer->lock,flag);
	return count;	
}
static int irda_notifier_call(struct notifier_block *nb,unsigned long flag,void *dev)
{
	if ( (flag==NETDEV_UP) && (dev == framer_dev->netdev))
		{
			unsigned long flag;
			framer_dev->irlap=irlap_open(dev,&framer_dev->qos,"mt65xx-irda-framer");	
			if (framer_dev->irlap == NULL)
			{
				dbg("irlap_open failed !!!\n");
				goto out;
			}
		dbg("irlap open with address %#x\n",framer_dev->irlap->saddr);
		spin_lock_irqsave(&framer_dev->lock,flag);
		mt_irda_start_rx(framer_dev);
		spin_unlock_irqrestore(&framer_dev->lock,flag);
		}
		else 
		dbg("irda notifier call(flag=%#lx dev=%s)\n",flag,((struct net_device *)dev)->name);
	out:
		return NOTIFY_OK;	
}

static struct notifier_block mt_irda_notify =
{
		.notifier_call=irda_notifier_call,
	};

static void mt_irda_delay(int count)
{
		int i;
		for (i=0;i<count;i++);	
}
static void hw_switch_to_sir(void)
{
		outw(IR_POWER_OFF,IR_TRANSCEIVER_PDN);
		mt_irda_delay(20);
		outw(0x5,IR_TRANSCEIVER_PDN);
		mt_irda_delay(20);
		outw(0x4,IR_TRANSCEIVER_PDN);
		mt_irda_delay(20);
		outw(0x0,IR_TRANSCEIVER_PDN);
}
static void hw_switch_to_mir(void)
{
		hw_switch_to_sir();
}
static void hw_switch_to_fir(void)
{	

		outw(IR_POWER_OFF,IR_TRANSCEIVER_PDN);
		mt_irda_delay(20);
		outw(0x7,IR_TRANSCEIVER_PDN);
		mt_irda_delay(40);
		outw(0x6,IR_TRANSCEIVER_PDN);
		mt_irda_delay(40);
		outw(0x4,IR_TRANSCEIVER_PDN);
		mt_irda_delay(20);
		outw(0x0,IR_TRANSCEIVER_PDN);
		mt_irda_delay(20);

}
static int mt_irda_change_speed(struct mt_irda_framer *framer)  //this one 
{
				int speed=framer->new_speed;
				if (speed <=0 || (speed>115200 && speed!=1152000 && speed != 576000 && speed != 4000000) )
					return -1;
				if (framer->new_speed && (framer->speed != framer->new_speed))
					{
							dbg("%s(),change speed %d-->%d\n",__FUNCTION__,framer->speed,framer->new_speed);
							speed=framer->speed=framer->new_speed;
							framer->new_speed=0;
						//	outw(IR_TX_EN_DISABLE,IR_TX_EN);
						//	outw(IR_RX_EN_DISABLE,IR_RX_EN);
							if (speed<=115200)
							{
									int div,remainder;
									unsigned short tmp;
									div=IrClock/(16*speed);
									remainder=IrClock%(16*speed);
									if (remainder>=8*speed)
										tmp=(unsigned short)(div+1);
									else 
										tmp=div;
									outw(tmp,IR_DIV);
									outw(0|0x18,IR_MODE);
									outw(framer->sir_framing_set|IR_CRC_REPORT,IR_RATE_FIX);
									outw(IR_BUF_CLEAR_BIT,IR_BUF_CLEAR);
									hw_switch_to_sir();					
															
							}
							else if (speed==1152000)
							{
										outw(1,IR_DIV);
										outw(0x18|0x4|0x1,IR_MODE);
										outw(IR_CRC_REPORT,IR_RATE_FIX);
										hw_switch_to_mir();
										
							}
							else if (speed==576000)
								{
											outw(1,IR_DIV);
											outw(0x18|0x1,IR_MODE);
											outw(IR_CRC_REPORT|framer->mir_576_tune,IR_RATE_FIX);
											hw_switch_to_mir();
										
								}
							else if (speed==4000000)
								{
										outw(1,IR_DIV);
										hw_switch_to_fir();
										outw(0x18|0x2,IR_MODE);	
										outw((1<<5)|IR_CRC_REPORT,IR_RATE_FIX);
										
										
								}
							dbg("%s(),speed=%d IR_DIV=%d \n",__FUNCTION__,framer->speed,inw(IR_DIV));
							return 1;
					}
				return 0;
}

void irda_dma_callback(void *dev)
{
		unsigned long flag;
		int dir;
		struct mt_irda_framer *framer=(struct mt_irda_framer *)dev;
		spin_lock_irqsave(&framer->lock,flag);
		dir=framer->direction;
		dbg("\n\n\n********************DMA irq jiffies=%lu**********************\n",jiffies);
		
		if (dir==IO_RECV)
		{
				dbg("BUG recv too much(FIFO_STA=%#x) !! \n",inw(IR_FIFO_STA));
				//mt_irda_start_rx(framer);
		}
		else if (dir == IO_XMIT)
			dbg("dma XMIT ..\n");
		else 
			dbg("DMA direction err!!\n");
			dbg("********************DMA irq**********************\n\n\n");
		spin_unlock_irqrestore(&framer->lock,flag);
	}
static void mt_irda_start_rx(struct mt_irda_framer *framer)
{
#if 0	
	dbg("dma internal buffer size=%d trans_len=%d\n",
		inl(((DMA_CHAN*)(framer->dma))->baseAddr+0x38),(1<<11)+1-
		inl(((DMA_CHAN*)(framer->dma))->baseAddr+0x24));
#endif
	//outl(0,DMA_START(((DMA_CHAN*)(framer->dma))->baseAddr));
#ifdef MT6573
	mt_stop_dma(framer->dma);
	mt_reset_dma(framer->dma);
	irda_dma_rx_config(framer);
//	framer->dma->callback=irda_dma_callback;
	framer->dma->data=(void*)framer;
#elif defined(MT6575)
	irda_stop_dma();
#endif
	irda_dma_rx_config(framer);
	framer->direction=IO_RECV;
	framer->tx_size=0;
	outw(IR_TX_EN_DISABLE,IR_TX_EN);
	outw(IR_RX_EN_DISABLE,IR_RX_EN);
	outw(IR_BUF_CLEAR_BIT,IR_BUF_CLEAR);
	outw(IR_IRQ_RTX/*|IR_IRQ_ENABLE_RXTRIG*/,IR_IRQ_ENABLE);
	outw(IR_RX_EN_BIT/*|IR_RX_EN_RXONE*/|(hard_ware?(framer->speed<=115200?0:IR_RX_EN_INVERT):(framer->speed<=115200?IR_RX_EN_INVERT:0)),IR_RX_EN);
#ifdef MT6573
	mt_start_dma(framer->dma);
#elif defined(MT6575)
	irda_start_dma();
#endif
//	netif_wake_queue(framer->netdev);
}	

static void mt_irda_start_tx(struct mt_irda_framer *framer)
{

#ifdef MT6573
	mt_stop_dma(framer->dma);
	mt_reset_dma(framer->dma);
	framer->dma->data=(void*)framer
#elif defined(MT6575)
	irda_stop_dma();
#endif
	irda_dma_tx_config(framer);
//	framer->dma->callback=irda_dma_callback;
	framer->direction=IO_XMIT;
	outw(IR_TX_EN_DISABLE,IR_TX_EN);
	outw(IR_RX_EN_DISABLE,IR_RX_EN);
	outw(IR_BUF_CLEAR_BIT,IR_BUF_CLEAR);
	outw(framer->tx_size,IR_TX_FRAME_SIZE);
	outw(IR_IRQ_RTX,IR_IRQ_ENABLE);
#ifdef MT6573
	mt_start_dma(framer->dma);
#elif defined(MT6575)
	irda_start_dma();
#endif
	outw(IR_TX_EN_BIT|IR_TX_EN_ONESHIT|framer->sir_mode|framer->tx_invert,IR_TX_EN);
}

void display_skb(struct sk_buff *skb,int rx)
{

#ifdef DEBUG
		int i=0;
		unsigned char *data=skb->data;
		if (skb->len==0)
			return ;
		printk("*********%s skb display begin(size=%dbytes speed=%d id=%lu)***********",rx?"rx":"tx",skb->len,framer_dev->speed,
			rx?framer_dev->netdev->stats.rx_packets:framer_dev->netdev->stats.tx_packets);
		for (i=0;i<skb->len;i++)
		{
					if (i%8==0)
					{
						printk("\n%2d---%2d:\t",i,(i+7<skb->len)?i+7:skb->len-1);
					}
					printk("%2x ",*(data+i));
		}
		printk("\n*********%s skb display end(size=%dbytes id=%lu)***********\n\n",rx?"rx":"tx",skb->len,
			rx?framer_dev->netdev->stats.rx_packets:framer_dev->netdev->stats.tx_packets);
		return ;
#endif

}
	
		
static int mt_irda_rx_one_skb(struct mt_irda_framer *framer)	   //this one
{
	int size=framer->rx_size;
	struct sk_buff *skb;
	skb=dev_alloc_skb(size+1);
	if (skb==NULL)
		{
			dbg("%s(),skb alloc failed, dropping frame!\n",__FUNCTION__);	
			return -ENOMEM;
		}
	//printk("rx one skb------\n");
	
	skb_reserve(skb,1);
	skb->mac_header=skb->data;
	skb_put(skb,size);
	skb_copy_to_linear_data(skb,framer->buff,size);
	display_skb(skb,1);
	skb->dev=framer->netdev;
	skb->protocol=htons(ETH_P_IRDA);
	return netif_rx(skb);
	
}



static irqreturn_t mt_irda_irq(int irq,void *dev)
{
	int status;
	unsigned long flag;
	int err=0;
	int framer_1_err=0;
	int framer_2_err=0;
	int size_1=0;
	int size_2=0;
	int framer_err;
	struct  mt_irda_framer *framer=(struct  mt_irda_framer *)dev;
	spin_lock_irqsave(&framer->lock,flag);
	
	outw(0,IR_IRQ_ENABLE);
	size_1=inw(IR_RX_FRAME1_SIZE);
	size_2=inw(IR_RX_FRAME2_SIZE);
	status=inw(IR_IRQ_STA);
	framer_1_err=inw(IR_FRAME1_STA);
	framer_2_err=inw(IR_FRAME2_STA);
	err=inw(IR_LSR);
	dbg("*****************irda irq**********************\n\
IRQ_STA=%#x Err_STA=%#x IR_FRAME1_STA=%#x IR_FRAME2_STA=%#x\
\nIR_FRAME1_SIZE=%d IR_FRAME2_SIZE=%d\n",
			status,err,framer_1_err,framer_2_err,size_1,size_2);
	if(status&IR_IRQ_ENABLE_MINTOUT)
		{
			dbg("%s(),mtt timeout!\n",__FUNCTION__);
			outw(0,IR_COUNT_ENABLE);	
			mt_irda_start_tx(framer);
			goto out;
		}
	if (status&IR_IRQ_ENABLE_RXTRIG)
		{
			dbg("%s(),RXTRIG !\n ",__FUNCTION__);
			outw(IR_IRQ_RTX,IR_IRQ_ENABLE);
		#ifdef MT6573
			mt_start_dma(framer->dma);
		#elif defined(MT6575)
			irda_start_dma();
		#endif
			
				
		//	goto out;	
		}
	if (status&IR_IRQ_ENABLE_ERROR)
	{
		outw(IR_TX_EN_DISABLE,IR_TX_EN);
		outw(IR_RX_EN_DISABLE,IR_RX_EN);
		framer->err_stat=err;
		wake_up_interruptible_all(&err_wait);
		dbg("(1)err cap---->IRQ_reg=%#x Err_Reg=%#x\n IR_FRAME1_Reg=%#x IR_FRAME2_Reg=%#x \n\n ",inw(IR_IRQ_STA),inw(IR_LSR),inw(IR_FRAME1_STA),inw(IR_FRAME2_STA));
		dbg("(2)err cap---->IRQ_reg=%#x Err_Reg=%#x\n IR_FRAME1_Reg=%#x IR_FRAME2_Reg=%#x \n\n ",inw(IR_IRQ_STA),inw(IR_LSR),inw(IR_FRAME1_STA),inw(IR_FRAME2_STA));
			if(framer->direction==IO_XMIT)
				{
					dbg("%s(),xmit error Err_STA=%#x !\n",__FUNCTION__,err);	
					if (err&IR_TX_UNDERRUN)
					{
							framer->netdev->stats.tx_errors++;
							framer->netdev->stats.tx_fifo_errors++;
					}
					goto tx_out;
				}
				else if (framer->direction==IO_RECV)
				{
						framer_err=(err&IR_FRAME1_ERROR)?framer_1_err:((err&IR_FRAME2_ERROR)?framer_2_err:0);
						if (framer_err==IR_FRAME_PF) 
							{
								outw(IR_IRQ_RTX,IR_IRQ_ENABLE);
								goto out;
							}
						
					{ 
						static int n=0;	printk(KERN_WARNING"\n\n recv error(%d) . speed=%d\n\n",n++,framer->speed);
						#ifdef DEBUG
					{	static int i;
						for (i=0;i<32;i++)
						{
							
							if(i%8==0) printk("\n%2d----%2d: ",i,i+7);
							 printk("%2x  ",*((char*)framer->buff+i));
						} 
					}				
						#endif
						
					}
						dbg("%s(),recv error Err_STA=%#x !\n",__FUNCTION__,err);	
						framer->netdev->stats.rx_errors++;
						if (err&IR_LSR_RXSIZE)
							{
									framer->netdev->stats.rx_length_errors++;
							}
						if (err&IR_LSR_OVERRUN)
							{
									framer->netdev->stats.rx_over_errors++;
							}
					
						if(framer_err)
							{
								if (framer_err&(IR_FRAME_FRAMING|IR_FRAME_MIR|IR_FRAME_FIR_4PPM|IR_FRAME_FIR_STO|IR_FRAME_UNKNOW))	
									framer->netdev->stats.rx_frame_errors++;
								if (framer_err&IR_FRAME_CRCFAIL)	
									framer->netdev->stats.rx_crc_errors++;
								
							}
							goto rx_out;
				}
	}
	else if (status&IR_IRQ_ENABLE_RXABORT)
		{
				dbg("%s(),rx abort(%s) !\n",__FUNCTION__,framer->direction==IO_RECV?"rx":"rx abort with tx dir");	
				framer->netdev->stats.rx_missed_errors++;
				goto rx_out;
		}
	else if(status&IR_IRQ_ENABLE_TXCOMPLETE)
		{
				dbg("%s(),TX completed! \n",__FUNCTION__);
				if(framer->direction != IO_XMIT)
					dbg("%s(),TXCOMPLETE bug !\n",__FUNCTION__);
				framer->netdev->stats.tx_packets++;
				framer->netdev->stats.tx_bytes +=framer->tx_size;
				
				goto tx_out;
		}	
	else if(status&IR_IRQ_ENABLE_RXCOMPLETE)	
	{
		printk("rx one skb,speed=%d .\n",framer->speed);
		outw(IR_RX_EN_DISABLE,IR_RX_EN);
		framer->rx_size=size_1?:size_2;
		dbg("rx_complete, rx_size=%d \n",framer->rx_size);
		while((inw(IR_FIFO_STA)&0x1) != 1);
	#ifdef MT6573	
		mt_stop_dma(framer->dma);
	#elif defined(MT6575)
		irda_stop_dma();
	#endif

		if(mt_irda_rx_one_skb(framer)==0)
			framer->netdev->stats.rx_packets++;
		else framer->netdev->stats.rx_missed_errors++;
		framer->netdev->stats.rx_bytes +=framer->rx_size;
		framer->rx_size=0;
		goto rx_out;
	}

	goto out;
tx_out:
	mt_irda_change_speed(framer);
	netif_wake_queue(framer->netdev);
rx_out:
	mt_irda_start_rx(framer);
out:
	spin_unlock_irqrestore(&framer->lock,flag);
	dbg("*****************irda irq**********************\n\n\n");
	return IRQ_HANDLED;
}
	
	
static int __init mt_irda_init(void)
{
		struct net_device *dev;
		struct mt_irda_framer *framer;
		int err=0;
		dev=alloc_irdadev(sizeof(struct mt_irda_framer));
		if (!dev)
		{
				dbg("alloc_irdadev failed !!\n");
				return -ENOMEM;
			}
//	dev->sysfs_groups[0]=&mt_irda_attribute_group;
		dev->dev.type = &mt_irda_dev_type;
		framer_dev=framer=netdev_priv(dev);
		framer->netdev=dev;
		spin_lock_init(&framer->lock);
		irda_init_max_qos_capabilies(&framer->qos);
		framer->qos.baud_rate.bits=IR_9600|IR_19200|IR_38400|IR_57600|
					IR_115200|IR_576000|IR_1152000
					|IR_4000000<<8
					;
		framer->qos.min_turn_time.bits=0x07;
		irda_qos_bits_to_value(&framer->qos);
		framer->buff=dma_alloc_coherent(NULL,PAGE_SIZE,&framer->buff_dma,GFP_KERNEL);
		if (framer->buff == NULL)
			{
					dbg("dma alloc failed!!\n");
					err=-ENOMEM;
					goto out1;
				}
		dbg("alloc dma buffer at %p size=%lu \n",framer->buff,PAGE_SIZE);
		memset(framer->buff,0,PAGE_SIZE);
		framer->direction=0;
		dev->netdev_ops=&mt_irda_netdev_ops;
		err=register_netdev(dev);
		if(err)
			{
					dbg("register netdev failed!!\n");
					goto out2;
				}
		dbg("register netdev: %s\n",dev->name);		
		err=register_netdevice_notifier(&mt_irda_notify);
		if (err)
			{
				dbg("%s(),register_netdevice_notifier() failed(return %d)!\n",__FUNCTION__,err);
				goto out3;	
			}
		goto out;
	out3:
			unregister_netdev(framer_dev->netdev);
	out2:
			dma_free_coherent(NULL,PAGE_SIZE,framer->buff,framer->buff_dma);		
	out1:
			free_netdev(dev);
	out:
			return err;
	}
	
static void __exit mt_irda_exit(void)
{
	unregister_netdevice_notifier(&mt_irda_notify);
	dma_free_coherent(NULL,PAGE_SIZE,framer_dev->buff,framer_dev->buff_dma);
	unregister_netdev(framer_dev->netdev);
	free_netdev(framer_dev->netdev);
}

static int mt_irda_net_open(struct net_device *dev)
{
	int err;
	int status;
	struct mt_irda_framer *framer;
	framer=netdev_priv(dev);
	irda_irq_init();
	err=request_irq(IRDA_IRQ,mt_irda_irq,0,dev->name,framer);
	if(err)
	{
		dbg("request irq failed !!\n");
		goto out;		
	}
	mt_irda_dev_open();
	outw(IR_TX_EN_DISABLE,IR_TX_EN);
	outw(IR_RX_EN_DISABLE,IR_RX_EN);
	outw(0,IR_TX_FRAME_SIZE);
	outw(0,IR_IRQ_ENABLE);
	outw(3,IR_INTTRIGGER);
	outw(1,IR_RATE);
	outw(IR_CRC_REPORT,IR_RATE_FIX);
	outw(IR_CRC_REPORT|(1<<5),IR_RATE_FIX);
	outw(0,IR_MODE);
	outw(IR_POWER_ON,IR_TRANSCEIVER_PDN);
#if defined(MT6573)||defined(MT6573)
	outw(12,IR_FIR_DIV);
#endif
	outw(IR_BUF_CLEAR_BIT,IR_BUF_CLEAR);
	outw(0,IR_COUNT_ENABLE);
	outw(1<<11,IR_RX_FRAME_MAX);
	status=inw(IR_IRQ_STA);
	status=inw(IR_LSR);
	mt65xx_irq_unmask(IRDA_IRQ);	
	framer->sir_mode=IR_DIV_16;
	framer->rx_invert=rx_invert?IR_RX_EN_INVERT:0;//IR_RX_EN_INVERT
	framer->tx_invert=tx_invert?IR_TX_EN_INVERT:0;
	framer->bof_type=BOF_TYPE_FFH;
	framer->mir_576_tune=IR_TX_MIR576_RX_HALF;
	framer->sir_framing_set=SIR_FRAMING_IGN_STOP_BIT;
	framer->new_speed=0;
	framer->speed=0;
	framer->dma=
#ifdef MT6516
	mt_request_half_size_dma();
#elif defined(MT6573)
	&irda_dma.config;
#elif defined(MT6575)
	&irda_dma;
#endif

#if defined(MT6575)
//	IrClock=(mt6575_get_bus_freq()*1000)>>2;	
	dbg("mt6575_get_bus_freq=%d\n",IrClock);
#endif	
	if (framer->dma == NULL)
	{
			dbg("request half dma failed!!\n");
			err=-EBUSY;
			goto out1;
		}
//	dbg("%s(),dma channel=%d \n",__FUNCTION__,((DMA_CHAN *)framer->dma)->chan_num);
	netif_start_queue(dev);
	
	//
//	spin_lock_irqsave(&framer->lock,flag);
//	mt_irda_start_rx(framer);
//	spin_unlock_irqrestore(&framer->lock,flag);
	//mt_start_dma(framer->dma);
	
	return 0;
out1:
#ifdef MT6516
	mt_free_dma(framer->dma);
#endif	

	mt65xx_irq_mask(IRDA_IRQ);
	free_irq(IRDA_IRQ,NULL);	
out:
	return err;	
}	

static int mt_irda_net_close(struct net_device *dev)
{
	unsigned long flag;
	struct mt_irda_framer *framer=netdev_priv(dev);
	netif_stop_queue(dev);
	if (framer->irlap)
		irlap_close(framer->irlap);
	spin_lock_irqsave(&framer->lock,flag);
	outw(0,IR_IRQ_ENABLE);
	outw(0,IR_COUNT_ENABLE);
	outw(IR_TX_EN_DISABLE,IR_TX_EN);
	outw(IR_RX_EN_DISABLE,IR_RX_EN);
	outw(IR_POWER_OFF,IR_TRANSCEIVER_PDN);
#ifdef MT6573	
	mt_stop_dma(framer->dma);
#elif defined(MT6575)
	irda_stop_dma();
#endif

	
#ifdef MT6516
	mt_free_dma(framer->dma);
#endif
	spin_unlock_irqrestore(&framer->lock,flag);
	free_irq(IRDA_IRQ,framer);
	mt65xx_irq_mask(IRDA_IRQ);
	mt_irda_dev_close();
	dbg("Irda closed IR_IRQ_STA=%#x FIFO_STA=%#x\n",inw(IR_IRQ_STA),inw(IR_FIFO_STA));
	return 0;
}

static netdev_tx_t mt_irda_xmit(struct sk_buff *skb,struct net_device *dev)
{
	struct mt_irda_framer *framer=netdev_priv(dev);
	unsigned long flag;
	int speed;
	int mtt;
	spin_lock_irqsave(&framer->lock,flag);
	if ( (inw(IR_FIFO_STA)&0x5) != 0x5 || (inw(IR_FIFO_STA)&0x10) )
	{
		dbg("%s(),Net xmit busy!\n ",__FUNCTION__);	
		spin_unlock_irqrestore(&framer->lock,flag);
		return NETDEV_TX_BUSY;
	}
	
	speed=irda_get_next_speed(skb);
	if ((speed != framer->speed) && speed!=-1)
	{
		framer->new_speed=speed;
		dbg("%s(),change speed to %d\n",__FUNCTION__,speed);
		if (!skb->len)
			{
				if(mt_irda_change_speed(framer)!=1)
					dbg("%s(),change speed failed !\n",__FUNCTION__);
				mt_irda_start_rx(framer);
				goto out;
			}	 
		dbg("%s(),change speed after a skb trans!(%d-->%d)\n",__FUNCTION__,framer->speed,speed);		
	}
	if (!skb->len)
	{
		dbg("%s(),0 length skb,it's bug!\n",__FUNCTION__);
		goto out;
	}
		display_skb(skb,0);
		outw(IR_RX_EN_DISABLE,IR_RX_EN);
		netif_stop_queue(dev);
		skb_copy_from_linear_data(skb,framer->buff,skb->len);
		framer->tx_size=skb->len;
		if(framer->speed<=115200)
		{
				int xbof;
				struct irda_skb_cb *cb=(struct irda_skb_cb *)skb->cb;
				if(cb->magic != LAP_MAGIC)
					{
							dbg("%s(),wrong magic in skb!\n",__FUNCTION__);
							xbof=10;
						}
				else
				{		
					xbof=cb->xbofs+cb->xbofs_delay;
					if(xbof>127)
					{
						dbg("%s(),too many xbofs(%d) !\n",__FUNCTION__,xbof);
						xbof=127;
					}
				}
				outw(framer->bof_type|xbof,IR_BOFS);
	no_mtt:
				mt_irda_start_tx(framer);
				goto out;
				
		}
		outw(0,IR_BOFS);	
		mtt=irda_get_mtt(skb);
		if (!mtt)
		{
			dbg("%s(),mtt bug!!!!!!!!!!!~~~\n\n",__FUNCTION__);
			goto no_mtt;
		}
		dbg("%s(),mtt time=%d \n",__FUNCTION__,mtt);
		outw(mtt*7500/1000,IR_MIN_T);
		outw(2,IR_COUNT_ENABLE);
		outw(IR_IRQ_ENABLE_MINTOUT,IR_IRQ_ENABLE);
		
	out:
		dev->trans_start=jiffies;
		dev_kfree_skb(skb);
		spin_unlock_irqrestore(&framer->lock,flag);
		return NETDEV_TX_OK;
}

static int mt_irda_ioctl(struct net_device *dev,struct ifreq *ifrq,int cmd)
{	
	int ret=0;
	struct if_irda_req *ifr = (struct if_irda_req *) ifrq;
	switch (cmd)
	{
	case SIOCSBANDWIDTH:
		if (!capable(CAP_NET_ADMIN))
		{	ret=-EPERM;
			goto out;
		}
		spin_lock_irq(&framer_dev->lock);
		if ( (inw(IR_FIFO_STA) & 0x5) != 0x5)
		{
			ret =-EBUSY;
			spin_unlock_irq(&framer_dev->lock);
			goto out;
		}
		framer_dev->new_speed = ifr->ifr_baudrate;
		ret=mt_irda_change_speed(framer_dev) == -1 ? -EINVAL: 0;
		spin_unlock_irq(&framer_dev->lock);
		break;
	case SIOCSMEDIABUSY:
		if (!capable(CAP_NET_ADMIN))
		{	ret=-EPERM;
			goto out;
		}
		irda_device_set_media_busy(framer_dev->netdev,TRUE);
		break;	
	case SIOCGRECEIVING:
		spin_lock_irq(&framer_dev->lock);
		ifr->ifr_receiving=(inw(IR_FIFO_STA) & 0x1) ==0?TRUE : FALSE;
		spin_unlock_irq(&framer_dev->lock);	
		break;
	case SIOCSMODE:
		if (!capable(CAP_NET_ADMIN))
		{	ret=-EPERM;
			goto out;
		}
		if (ifr->ifr_mode & ~(IOCTL_SIR_DIV_16|IOCTL_SIR_1_61|IOCTL_BOF_C0H|IOCTL_BOF_FFH|IOCTL_HAVEHARDWARE|IOCTL_NOHARDWARE))
		{
			ret=-EINVAL;
			goto out ;
		}
		spin_lock_irq(&framer_dev->lock);
		if ( (inw(IR_FIFO_STA) & 0x5) != 0x5)
		{
			ret =-EBUSY;
			spin_unlock_irq(&framer_dev->lock);
			goto out;
		}
		if (ifr->ifr_mode & IOCTL_SIR_DIV_16 ) parse_modulation_type("3/16");
		else if (ifr->ifr_mode & IOCTL_SIR_1_61) parse_modulation_type("1.61us");
		if (ifr->ifr_mode & IOCTL_BOF_C0H) parse_bof_type("c0h");
		else if (ifr->ifr_mode & IOCTL_BOF_FFH) parse_bof_type("ffh");
		if (ifr->ifr_mode & IOCTL_HAVEHARDWARE) { hard_ware =1; mt_irda_start_rx(framer_dev);}
		else if (ifr->ifr_mode & IOCTL_NOHARDWARE) {hard_ware =0;mt_irda_start_rx(framer_dev);}
		spin_unlock_irq(&framer_dev->lock);
		break;
	case SIOCGERRSTAT :
		interruptible_sleep_on(&err_wait);
		if (signal_pending(current))
			return -EINTR;
		else 
		{
			int err;
			spin_lock_irq(&framer_dev->lock);
			err=framer_dev->err_stat;
			spin_unlock_irq(&framer_dev->lock);
			return put_user(err,(int *)ifr->ifr_mode);
		}
		
	default :
		ret=-EOPNOTSUPP;	
	}
out:
	return ret;
}
module_init(mt_irda_init);
module_exit(mt_irda_exit);
MODULE_LICENSE("Dual BSD/GPL");
