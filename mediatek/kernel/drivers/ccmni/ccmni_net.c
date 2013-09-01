/*****************************************************************************
 *
 * Filename:
 * ---------
 *   ccmni.c
 *
 * Project:
 * --------
 *   YuSu
 *
 * Description:
 * ------------
 *   MT6516 Cross Chip Modem Network Interface
 *
 * Author:
 * -------
 *   TL Lau (mtk02008)
 *
 ****************************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/bitops.h>
#include <linux/wakelock.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/dma-mapping.h>
#include <linux/timer.h>
#include <asm/bitops.h>
#include <asm/dma-mapping.h>
#include <ccci.h>
#include <ccci_tty.h>

#include <mach/mt_typedefs.h>

#include "ccmni_pfp.h"

#define  CCCI_NETWORK           0
#define  CCMNI_MTU              (MAX_PFP_LEN_FIELD_VALUE)
#define  CCMNI_TX_QUEUE         1000
#define  CCMNI_UART_OFFSET      2
#define  CCMNI_MAX_CHANNELS     3
#define  CCMNI_CHANNEL_OFFSET   2


#define  IPV4_VERSION 0x40
#define  IPV6_VERSION 0x60

#define CCMNI_RECV_ACK_PENDING (0)
#define CCMNI_SEND_PENDING (1)



typedef struct
{
	int			channel;
	int			ccci_channel;
	int			uart_rx;
	int 			uart_rx_ack;
	int			uart_tx;
	int			uart_tx_ack;
	int			ready;
	unsigned long		flags;
	struct timer_list	timer;
	unsigned long		send_len;
	struct net_device	*dev;
	struct wake_lock	wake_lock;
	spinlock_t		spinlock;

	shared_mem_tty_t	*shared_mem;
	int			shared_mem_phys_addr;

	unsigned char		write_buffer [CCMNI_MTU + 4];
	unsigned char		read_buffer  [CCCI_TTY_RX_BUFFER_SIZE];
	unsigned char		decode_buffer[CCCI_TTY_RX_BUFFER_SIZE];

	u8			mac_addr     [ETH_ALEN];

	struct tasklet_struct	tasklet;
    
} ccmni_instance_t;  

static int hard_ware_ok=0;
static ccmni_instance_t       *ccmni_instance[CCMNI_MAX_CHANNELS];
static struct wake_lock        ccmni_wake_lock;


static void ccmni_read        (unsigned long arg);
//static DECLARE_TASKLET        (ccmni_read_tasklet, ccmni_read, 0);


static void reset_ccmni_instance(ccmni_instance_t *ccmni_instance)
{
	
	spin_lock(&ccmni_instance->spinlock);
	netif_carrier_off(ccmni_instance->dev);	
	del_timer_sync(&ccmni_instance->timer);
	ccci_reset_buffers(ccmni_instance->shared_mem);
	spin_unlock(&ccmni_instance->spinlock);
}

static void restore_ccmni_instance(ccmni_instance_t *ccmni_instance)
{
	spin_lock(&ccmni_instance->spinlock);
	netif_carrier_on(ccmni_instance->dev);	
	spin_unlock(&ccmni_instance->spinlock);
}



static void ccmni_notifier_call(MD_CALL_BACK_QUEUE *notifier __always_unused,unsigned long val)
{
	int i=0;
	
	switch(val)
	{	
	case CCCI_MD_EXCEPTION :
	case CCCI_MD_RESET     :
		if (hard_ware_ok)
		{
			hard_ware_ok=0;
			for(i=0;i<CCMNI_MAX_CHANNELS;i++)
			{
					ccmni_instance_t *instance = ccmni_instance[i];
					if (instance)  
						reset_ccmni_instance(instance);
			}
		}	
		break;
		
	case CCCI_MD_BOOTUP:
		if (hard_ware_ok==0)
		{	
			hard_ware_ok=1;
			for(i=0;i<CCMNI_MAX_CHANNELS;i++)
			{
					ccmni_instance_t *instance = ccmni_instance[i];
					if (instance) 
						restore_ccmni_instance(instance);
			}
		}
		break;
	}

	return ;
}

static MD_CALL_BACK_QUEUE ccmni_notifier= {
	.call = ccmni_notifier_call,
	.next=NULL,
};


static void timer_func(unsigned long data)
{
	ccmni_instance_t *ccmni=(ccmni_instance_t *)data;
	int contin=0;
	int ret=0;
	
	spin_lock_bh(&ccmni->spinlock);
	
	if (hard_ware_ok == 0)  
		goto out;
	
	if (test_bit(CCMNI_RECV_ACK_PENDING,&ccmni->flags))
	{		
		ret=ccci_write_mailbox(ccmni->uart_rx_ack, CCMNI_CHANNEL_OFFSET + ccmni->channel);

		if (ret==CCCI_NO_PHY_CHANNEL)
			contin=1;
		else 
			clear_bit(CCMNI_RECV_ACK_PENDING, &ccmni->flags);
		
	}
	if (test_bit(CCMNI_SEND_PENDING,&ccmni->flags))
	{
		ret = ccci_write_stream(ccmni->uart_tx, (unsigned int) NULL, ccmni->send_len);

		if (ret==CCCI_NO_PHY_CHANNEL)
			contin=1;
		else {
			//clear_bit(CCMNI_RECV_ACK_PENDING,&ccmni->flags);
			clear_bit(CCMNI_SEND_PENDING,&ccmni->flags);
			ccmni->send_len=0;
		}		
	}
	
out:
	spin_unlock_bh(&ccmni->spinlock);
	if (contin)
		mod_timer(&ccmni->timer,jiffies+2);	

	return;

}

static void ccmni_make_etherframe(void *_eth_hdr, u8 *mac_addr, int packet_type)
{
    struct ethhdr *eth_hdr = _eth_hdr;

    memcpy(eth_hdr->h_dest,   mac_addr, sizeof(eth_hdr->h_dest));
    memset(eth_hdr->h_source, 0, sizeof(eth_hdr->h_source));
    if(packet_type == IPV6_VERSION){
        eth_hdr->h_proto = __constant_cpu_to_be16(ETH_P_IPV6);
    }else{
        eth_hdr->h_proto = __constant_cpu_to_be16(ETH_P_IP);
    }
}


static int ccmni_receive(ccmni_instance_t *ccmni, int length)
{
    int               counter, ret;
    packet_info_t     packet_info;
    complete_ippkt_t *packet;
    complete_ippkt_t *processed_packet;
    struct sk_buff   *skb;
#ifndef __SUPPORT_DYNAMIC_MULTIPLE_FRAME__
    complete_ippkt_t  last_packet = {0};
    int               offset_put_pkt = 0;
    int               offset_parse_frame = 0; 
#endif
    int               packet_type;

    CCCI_CCMNI_MSG("CCMNI%d_receive() invoke pfp_unframe()\n", ccmni->channel);
#ifndef __SUPPORT_DYNAMIC_MULTIPLE_FRAME__
    do
    {
#endif
        packet_info = pfp_unframe(ccmni->decode_buffer+offset_put_pkt, \
        	CCCI_TTY_RX_BUFFER_SIZE-offset_put_pkt, ccmni->read_buffer+offset_parse_frame, \
        	length, ccmni->channel);
        packet = packet_info.pkt_list;
        
        CCCI_CCMNI_MSG("CCMNI%d num_complete_pkt=%d after pfp_unframe \n", \
			ccmni->channel, packet_info.num_complete_packets);
        
        for(counter = 0; counter < packet_info.num_complete_packets; counter++)
        {
            skb = dev_alloc_skb(packet->pkt_size);
            if (skb)
            {
                packet_type = packet->pkt_data[0] & 0xF0;
                
                memcpy(skb_put(skb, packet->pkt_size), packet->pkt_data, packet->pkt_size);
                ccmni_make_etherframe(skb->data - ETH_HLEN, ccmni->dev->dev_addr, packet_type);
                skb_set_mac_header(skb, -ETH_HLEN);
                
                skb->dev       = ccmni->dev;
                if(packet_type == IPV6_VERSION){            
                    skb->protocol  = htons(ETH_P_IPV6);
                }
                else {
                    skb->protocol  = htons(ETH_P_IP);
                }
                //skb->ip_summed = CHECKSUM_UNNECESSARY;
		skb->ip_summed = CHECKSUM_NONE;

                ret = netif_rx(skb);

                CCCI_CCMNI_MSG("CCMNI%d invoke netif_rx()=%d\n", ccmni->channel, ret);
                       
                ccmni->dev->stats.rx_packets++;
                ccmni->dev->stats.rx_bytes  += packet->pkt_size;
                
                CCCI_CCMNI_MSG("CCMNI%d rx_pkts=%d, stats_rx_bytes=%d\n",ccmni->channel, \
					ccmni->dev->stats.rx_packets,ccmni->dev->stats.rx_bytes);
            }
            else
            {
                CCCI_MSG_INF("net", "CCMNI%d Socket buffer allocate fail\n", ccmni->channel);
            }

            processed_packet = packet;
            last_packet = *processed_packet;
            packet = packet->next;
            
#ifndef __SUPPORT_DYNAMIC_MULTIPLE_FRAME__
            /* Only clear the entry_used flag as 0 */
            release_one_used_complete_ippkt_entry(processed_packet);
#else
            /* Free the memory space allocated in pfp_unframe() */
            vfree(processed_packet);
#endif
        };
#ifdef __SUPPORT_DYNAMIC_MULTIPLE_FRAME__
        return packet_info.consumed_length;
#else
        /* It must to check if it is necessary to invoke the pfp_unframe() again due to no available complete_ippkt entry */
        if (packet_info.try_decode_again == 1)
        {
            offset_put_pkt += (last_packet.pkt_data - ccmni->decode_buffer + last_packet.pkt_size);
            offset_parse_frame += packet_info.consumed_length;
        }
	
    } while (packet_info.try_decode_again == 1); 

    offset_parse_frame += packet_info.consumed_length;
    return offset_parse_frame;
#endif
        
}

static void ccmni_read(unsigned long arg)
{
    int                part, size;
    int ret;
    int                read, write, consumed;
    unsigned char     *string;
    ccmni_instance_t  *ccmni = (ccmni_instance_t *) arg;

    if (ccmni == NULL)
    {
        CCCI_MSG_INF("net", "CCMNI%d_read: invalid private data\n", ccmni->channel);
        return;
    }
  
    spin_lock_bh(&ccmni->spinlock);
	
    if (hard_ware_ok==0)  
    {
        CCCI_MSG_INF("net", "CCMNI%d_read fail when modem not ready\n", ccmni->channel);
        goto out;
    }
	
    string = ccmni->read_buffer;
    read   = ccmni->shared_mem->rx_control.read;
    write  = ccmni->shared_mem->rx_control.write; 
    size   = write - read;
    part   = 0;

    if (size < 0)
    {
        size += ccmni->shared_mem->rx_control.length;
    }
    
    if (read > write)
    {
        part = ccmni->shared_mem->rx_control.length - read;       
        memcpy(string, &ccmni->shared_mem->rx_buffer[read], part);
        
        size   -= part;
        string += part;
        read    = 0;
    }

    memcpy(string, &ccmni->shared_mem->rx_buffer[read], size);
	
    CCCI_CCMNI_MSG("CCMNI%d_receive[Before]: size=%d, read=%d\n", \
		ccmni->channel, (size+part), read);
    consumed = ccmni_receive(ccmni, size + part);
    CCCI_CCMNI_MSG("CCMNI%d_receive[After]: consume=%d\n", ccmni->channel, consumed);
	
    //  Calculate the new position of the read pointer.
    //  Take into consideration the number of bytes actually consumed;
    //  i.e. number of bytes taken up by complete IP packets.   
    read += size;
    if (read >= ccmni->shared_mem->rx_control.length)
    {
        read -= ccmni->shared_mem->rx_control.length;
    }
    
    if (consumed < (size + part))
    {
        read -= ((size + part) - consumed);
        if (read < 0)
        {
            read += ccmni->shared_mem->rx_control.length;
        }
    }
    
    ccmni->shared_mem->rx_control.read = read;
        
    //  Send an acknowledgement back to modem side.
    CCCI_CCMNI_MSG("CCMNI%d_read to write mailbox(ch%d, tty%d)\n", ccmni->channel,
		ccmni->uart_rx_ack, CCMNI_CHANNEL_OFFSET + ccmni->channel);
    ret = ccci_write_mailbox(ccmni->uart_rx_ack, CCMNI_CHANNEL_OFFSET + ccmni->channel); 
    if (ret==CCCI_NO_PHY_CHANNEL)	
    {
        set_bit(CCMNI_RECV_ACK_PENDING,&ccmni->flags);
        mod_timer(&ccmni->timer,jiffies);
    }
    else if (ret==CCCI_SUCCESS)
        clear_bit(CCMNI_RECV_ACK_PENDING,&ccmni->flags);
out:
	spin_unlock_bh(&ccmni->spinlock);
	
#if 0
    do {
		
    	
	}while(ret==CCCI_NO_PHY_CHANNEL&&jiffies<=start+5);

	
    CCMNI_LOG(ccmni, KERN_DEBUG, "return ccmni_write_mailbox(%d, %d) = %d\n", ccmni->uart_rx_ack, CCMNI_CHANNEL_OFFSET + ccmni->channel, ret);
    
    if (ret != CCCI_SUCCESS)
    {
        CCMNI_LOG(ccmni, KERN_ERR, "ccci_write_mailbox failed. Error code is %d\n", ret);
        ccci_channel_status(ccmni->uart_rx_ack);
    //    ASSERT(0);
    }
#endif

    CCCI_CCMNI_MSG("CCMNI%d_read invoke wake_lock_timeout(1s)\n", ccmni->channel);
    wake_lock_timeout(&ccmni_wake_lock, HZ);

	return;
}


//  will be called when modem sends us something.
//  we will then copy it to the tty's buffer.
//  this is essentially the "read" fops.
static void ccmni_callback(CCCI_BUFF_T *buff, void *private_data)
{
	ccmni_instance_t  *ccmni = (ccmni_instance_t *) private_data;

	switch(buff->channel)
	{
	case CCCI_CCMNI1_TX_ACK:
	case CCCI_CCMNI2_TX_ACK:
	case CCCI_CCMNI3_TX_ACK:
		// this should be in an interrupt,
		// so no locking required...
		ccmni->ready = 1;
		netif_wake_queue(ccmni->dev);

		break;

	case CCCI_CCMNI1_RX:
	case CCCI_CCMNI2_RX:
	case CCCI_CCMNI3_RX:
		//ccmni_read_tasklet2.data = (unsigned long) private_data;
		//tasklet_schedule(&ccmni_read_tasklet);
		tasklet_schedule(&ccmni->tasklet);

		break;

	default:
		break;
	}
}


static void ccmni_write(ccmni_instance_t *ccmni, frame_info_t *frame_info)
{
	int		size, over, total;
	int		ret;
	unsigned	read, write, length, len;
	unsigned	tmp_write;
	unsigned char	*ptr;
//	unsigned long start=jiffies;
	size = 0;
	ptr  = (unsigned char *) frame_info->frame_list[0].frame_data;
	len  =                   frame_info->frame_list[0].frame_size;


	read   = ccmni->shared_mem->tx_control.read;
	write  = ccmni->shared_mem->tx_control.write;
	length = ccmni->shared_mem->tx_control.length;
	over   = length - write;

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

	if (len > size)
	{
		len   = size;
		total = size;
	}

	total = len;

	if (over < len)
	{
		memcpy(&ccmni->shared_mem->tx_buffer[write], (void *) ptr, over);
		len   -= over;
		ptr   += over;
		write  = 0;
	}

	memcpy(&ccmni->shared_mem->tx_buffer[write], (void *) ptr, len);
	/*ALPS00233095(CMUX FCS error issue): write buffer will out of order for optimization, */
	/* update write pointer->memcpy, so need add dsb() between them*/
	//dsb();
	mb();
	tmp_write = write + len;
	if (tmp_write >= length)
	{
		tmp_write -= length;
	}
	ccmni->shared_mem->tx_control.write = tmp_write;

	// ccmni->ready = 0;
	len = total;
	ret = ccci_write_stream(ccmni->uart_tx, (unsigned int) NULL, len);
	if (ret==CCCI_NO_PHY_CHANNEL)
	{
		set_bit(CCMNI_SEND_PENDING,&ccmni->flags);
		ccmni->send_len +=len;
		mod_timer(&ccmni->timer,jiffies);
	}
	else if (ret==CCCI_SUCCESS)
		clear_bit(CCMNI_SEND_PENDING,&ccmni->flags);
#if 0
   //   do {
		
  //    	}while (ret==CCCI_NO_PHY_CHANNEL&&jiffies<=start+5);

        if (ret != CCCI_SUCCESS)
        {
            CCCI_CCMNI_MSG("ch%d, ccci_write_stream failed. Error code is %d\n", ccmni->channel, ret);
            ccci_channel_status(ccmni->uart_tx);
           // ASSERT(0);
        }
#endif

	return;
}



//  The function start_xmit is called when there is one packet to transmit.
static int ccmni_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
    int      ret = NETDEV_TX_OK;
    int      size;
    unsigned int read, write, length;
    frame_info_t       frame_info;
	ccmni_instance_t  *ccmni = netdev_priv(dev);
   
    spin_lock_bh(&ccmni->spinlock);
	
    if (hard_ware_ok==0) 
    {
		CCCI_MSG_INF("net", "CCMNI%d transfer data fail when modem not ready \n", ccmni->channel);
		ret = NETDEV_TX_BUSY;
		goto _ccmni_start_xmit_busy;
    }
	
    read   = ccmni->shared_mem->tx_control.read;
    write  = ccmni->shared_mem->tx_control.write;
    length = ccmni->shared_mem->tx_control.length;
    size   = read - write;
    
    CCCI_CCMNI_MSG("CCMNI%d_start_xmit: skb_len=%d, size=%d, ccmni_ready=%d \n", \
		ccmni->channel, skb->len, size, ccmni->ready);
    
    if (size <= 0)
    {
        size += length;
    }

    if (skb->len > CCMNI_MTU)
    {
        //  Sanity check; this should not happen!
        //  Digest and return OK.
        CCCI_MSG_INF("net", "CCMNI%d packet size exceed 1500 bytes: size=%d \n", \
        	ccmni->channel, skb->len);
        dev->stats.tx_dropped++;
        goto _ccmni_start_xmit_exit;
    }

	if(size >= 1)
		size-=1;
	else
		CCCI_MSG_DBG("net", "CCMNI%d size is Zero(1) \n", ccmni->channel);
  
    if (size < (skb->len + 4))
    {
        //  The TX buffer is full, or its not ready yet,
        //  we should stop the net queue for the moment.
        CCCI_MSG_DBG("net", "CCMNI%d TX busy and stop queue: size=%d, skb->len=%d \n", \
        	ccmni->channel, size, skb->len);
        CCCI_MSG_DBG("net", "       TX read = %d  write = %d\n", \
			ccmni->shared_mem->tx_control.read, ccmni->shared_mem->tx_control.write); 
        CCCI_MSG_DBG("net", "       RX read = %d  write = %d\n", \
			ccmni->shared_mem->rx_control.read, ccmni->shared_mem->rx_control.write);
        
        netif_stop_queue(ccmni->dev);

        //  Set CCMNI ready to ZERO, and wait for the ACK from modem side.
        ccmni->ready = 0;
        ret          = NETDEV_TX_BUSY;

        goto _ccmni_start_xmit_busy;
    }


    frame_info = pfp_frame(ccmni->write_buffer, skb->data, skb->len, FRAME_START, ccmni->channel);
    ccmni_write (ccmni, &frame_info);

    dev->stats.tx_packets++;
    dev->stats.tx_bytes  += skb->len;

    
  _ccmni_start_xmit_exit:

    dev_kfree_skb(skb);

  _ccmni_start_xmit_busy:
    
    spin_unlock_bh(&ccmni->spinlock);
    
    return ret;
}


static int ccmni_open(struct net_device *dev)
{
 	ccmni_instance_t  *ccmni = netdev_priv(dev);
	
 	CCCI_MSG_DBG("net", "CCMNI%d open \n", ccmni->channel); 
    if (hard_ware_ok == 0) {	
		CCCI_MSG_INF("net", "CCMNI%d open fail when modem not ready \n", ccmni->channel);
		return -EIO;
    }
    netif_start_queue(dev);
    return 0;
}

static int ccmni_close(struct net_device *dev)
{
    ccmni_instance_t  *ccmni = netdev_priv(dev);
	
	CCCI_MSG_DBG("net", "CCMNI%d close \n", ccmni->channel); 
    netif_stop_queue(dev);
    return 0;
}

static int ccmni_net_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
    //  No implementation at this moment.
    //  This is a place holder.
    
    return 0;
}


static void ccmni_tx_timeout(struct net_device *dev)
{
    //  No implementation at this moment.
    //  This is a place holder.
    
    dev->stats.tx_errors++;
    netif_wake_queue(dev);   
}


static const struct net_device_ops ccmni_netdev_ops = 
{
    .ndo_open       = ccmni_open,
     .ndo_stop      = ccmni_close,		
    .ndo_start_xmit = ccmni_start_xmit,
    .ndo_do_ioctl   = ccmni_net_ioctl,
    .ndo_tx_timeout = ccmni_tx_timeout,
};


static void ccmni_setup(struct net_device *dev)
{
    ccmni_instance_t *ccmni = netdev_priv(dev);
    
    ether_setup(dev);

    dev->header_ops        = NULL;
    dev->netdev_ops        = &ccmni_netdev_ops;
    dev->flags             = IFF_NOARP & (~IFF_BROADCAST & ~IFF_MULTICAST);
    dev->mtu               = CCMNI_MTU;
    dev->tx_queue_len      = CCMNI_TX_QUEUE;
    dev->addr_len          = ETH_ALEN;
    dev->destructor        = free_netdev;

    random_ether_addr((u8 *) dev->dev_addr);

    CCCI_CCMNI_MSG("CCMNI%d_setup: features=0x%08x,flags=0x%08x\n", ccmni->channel, \
		dev->features, dev->flags);

	return;
}


static int ccmni_create_instance(int channel)
{
    int  ret, size;
    int  uart_rx, uart_rx_ack;
    int  uart_tx, uart_tx_ack;
    ccmni_instance_t  *ccmni;
    struct net_device *dev = NULL;

    //  Network device creation and registration.
    dev = alloc_netdev(sizeof(ccmni_instance_t), "", ccmni_setup);
    if (dev == NULL)
    {
    	CCCI_MSG_INF("net", "CCMNI%d allocate netdev fail!\n", channel); 
        return -ENOMEM;
    }
    
    ccmni          = netdev_priv(dev);
    ccmni->dev     = dev;
    ccmni->channel = channel;
    
    sprintf(dev->name, "ccmni%d", channel);

    ret = register_netdev(dev);
    if (ret != 0)
    {
        CCCI_MSG_INF("net", "CCMNI%d register netdev fail: %d\n", ccmni->channel, ret);        
        goto _ccmni_create_instance_exit;
    }


    //  CCCI channel registration.
    ASSERT(ccci_uart_setup(CCMNI_UART_OFFSET + ccmni->channel, (int*)&ccmni->shared_mem,
                           &ccmni->shared_mem_phys_addr, &size) == CCCI_SUCCESS);

    if (ccmni->shared_mem == NULL)
    {
        CCCI_MSG_INF("net", "CCMNI%d allocate memory fail\n", ccmni->channel);
        unregister_netdev(dev);        
        ret = -ENOMEM;
        
        goto _ccmni_create_instance_exit;
    }

	CCCI_CCMNI_MSG("0x%08X:0x%08X:%d\n", (unsigned int)ccmni->shared_mem, \
		(unsigned int)ccmni->shared_mem_phys_addr, size);

    ccmni->shared_mem->tx_control.length = CCCI_TTY_TX_BUFFER_SIZE;
    ccmni->shared_mem->tx_control.read   = 0;
    ccmni->shared_mem->tx_control.write  = 0;
        
    ccmni->shared_mem->rx_control.length = CCCI_TTY_RX_BUFFER_SIZE;
    ccmni->shared_mem->rx_control.read   = 0;
    ccmni->shared_mem->rx_control.write  = 0;

    switch(ccmni->channel)
    {
        case 0:
        {
            uart_rx     = CCCI_CCMNI1_RX;
            uart_rx_ack = CCCI_CCMNI1_RX_ACK;
            uart_tx     = CCCI_CCMNI1_TX;
            uart_tx_ack = CCCI_CCMNI1_TX_ACK;

            ccmni->ccci_channel = 0;

            break;            
        }

        case 1:
        {
            uart_rx     = CCCI_CCMNI2_RX;
            uart_rx_ack = CCCI_CCMNI2_RX_ACK;
            uart_tx     = CCCI_CCMNI2_TX;
            uart_tx_ack = CCCI_CCMNI2_TX_ACK;

            ccmni->ccci_channel = 1;

            break;            
        }

        case 2:
        {
            uart_rx     = CCCI_CCMNI3_RX;
            uart_rx_ack = CCCI_CCMNI3_RX_ACK;
            uart_tx     = CCCI_CCMNI3_TX;
            uart_tx_ack = CCCI_CCMNI3_TX_ACK;

            ccmni->ccci_channel = 2;

            break;            
        }

        default:
        {
            CCCI_MSG_INF("net", "CCMNI%d, Invalid ccmni number\n", ccmni->channel);
            unregister_netdev(dev);
            ret = -ENOSYS;
            
            goto _ccmni_create_instance_exit;
        }
    }
    
    ccmni->uart_rx      = uart_rx;
    ccmni->uart_rx_ack  = uart_rx_ack;
    ccmni->uart_tx      = uart_tx;
    ccmni->uart_tx_ack  = uart_tx_ack;
    
    // Register this ccmni instance to the ccci driver.
    // pass it the notification handler.
    ASSERT(ccci_register(uart_rx,     ccmni_callback, (void *) ccmni) == CCCI_SUCCESS);
    ASSERT(ccci_register(uart_rx_ack, ccmni_callback, (void *) ccmni) == CCCI_SUCCESS);
    ASSERT(ccci_register(uart_tx,     ccmni_callback, (void *) ccmni) == CCCI_SUCCESS);
    ASSERT(ccci_register(uart_tx_ack, ccmni_callback, (void *) ccmni) == CCCI_SUCCESS);

    // Initialize the spinlock.
    spin_lock_init(&ccmni->spinlock);
    setup_timer(&ccmni->timer,timer_func,(unsigned long)ccmni);

    // Initialize the tasklet.
    tasklet_init(&ccmni->tasklet, ccmni_read, (unsigned long)ccmni);

    ccmni_instance[channel] = ccmni;
    ccmni->ready = 1;
       
    return ret;

    
  _ccmni_create_instance_exit:
    
    free_netdev(dev);
    
    return ret;
}


static void ccmni_destroy_instance(int channel)
{
    ccmni_instance_t *ccmni = ccmni_instance[channel];

    if (ccmni != NULL)
    {
        ASSERT(ccci_unregister(ccmni->uart_rx    ) == CCCI_SUCCESS);
        ASSERT(ccci_unregister(ccmni->uart_rx_ack) == CCCI_SUCCESS);
        ASSERT(ccci_unregister(ccmni->uart_rx    ) == CCCI_SUCCESS);
        ASSERT(ccci_unregister(ccmni->uart_tx_ack) == CCCI_SUCCESS);
                        
        if (ccmni->shared_mem != NULL)
        {
            ccmni->shared_mem           = NULL;
            ccmni->shared_mem_phys_addr = 0;
        }
        
		if(ccmni->dev != NULL)
        unregister_netdev(ccmni->dev);
        tasklet_kill(&ccmni->tasklet);
		ccmni->ready = 0;
        ccmni_instance[channel] = NULL;
    }
}


static int __init ccmni_init(void)
{
    int count, ret;

    memset(ccmni_instance, 0, sizeof(ccmni_instance) * CCMNI_MAX_CHANNELS);
    
    for(count = 0; count < CCMNI_MAX_CHANNELS; count++)
    {
        ret = ccmni_create_instance(count);
        if (ret != 0) {
            CCCI_MSG_INF("net", "CCMNI%d create instance fail: %d\n", count, ret);
            return ret;
        }
        else {
            CCCI_MSG_DBG("net", "CCMNI%d create instance ok!\n", count);
        }
    }
	
    ret=md_register_call_chain(&md_notifier,&ccmni_notifier);
    if(ret) {
		CCCI_MSG_INF("net", "md_register_call_chain fail: %d\n", ret);
	  goto out;
    }
    
    wake_lock_init(&ccmni_wake_lock, WAKE_LOCK_SUSPEND, "ccmni wake lock");
out: 
    return ret;
}


static void __exit ccmni_exit(void)
{
	int count;
	
	for(count = 0; count < CCMNI_MAX_CHANNELS; count++)
    	ccmni_destroy_instance(count);
	
    md_unregister_call_chain(&md_notifier,&ccmni_notifier);
    wake_lock_destroy(&ccmni_wake_lock);

	return;
}


module_init(ccmni_init);
module_exit(ccmni_exit);


MODULE_DESCRIPTION("Cross Chip Modem Network Interface driver");
MODULE_AUTHOR("MTK");
MODULE_LICENSE("GPL");

