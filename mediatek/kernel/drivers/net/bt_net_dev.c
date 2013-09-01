#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/idr.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/poll.h>
//#include <linux/filter.h>
#include <linux/skbuff.h>
//#include <linux/rtnetlink.h>
#include <linux/if_arp.h>
#include <linux/ip.h>
#include <linux/tcp.h>
//#include <linux/smp_lock.h>
#include <linux/spinlock.h>
//#include <linux/rwsem.h>
#include <linux/stddef.h>
#include <linux/device.h>
#include <linux/mutex.h>
//#include <net/slhc_vj.h>
#include <asm/atomic.h>

#include <linux/nsproxy.h>
#include <net/net_namespace.h>
#include <net/netns/generic.h>

#include <linux/bt_net_dev.h>

static DEFINE_MUTEX(btn_mutex);
static struct class *btn_class;
static dev_t btn_dev_num;
static atomic_t btn_unit_count = ATOMIC_INIT(0);
static u8 *dev_mac;

enum btn_dev_type
{
    BTN_IP_DEV = 0,
    BTN_ETH_DEV    
};


struct btn_file
{
    struct sk_buff_head xq;	/* transmit queue */
    struct sk_buff_head rq;	/* receive queue */
    wait_queue_head_t rwait;	/* for poll on reading /dev/btn */
    atomic_t	refcnt;		/* # refs counter */
    int	 		index;		/* interface unit number */
    int	 		dead;		/* unit has been shut down */
};

struct btn
{
    struct btn_file file;	/* stuff for read/write */
    struct net_device *dev;		/* network interface device */
    spinlock_t	rlock;		/* lock for receive side */
    spinlock_t	wlock;		/* lock for transmit side */
    enum btn_dev_type dev_type;

    struct net *btn_net;		/* the net we belong to */
};


#define PF_TO_BTN(pf)		container_of(pf, struct btn, file)


/* per-net private data for this module */
static int bt_net_id;
struct btn_net
{
    struct idr units_idr;
    struct mutex all_btn_mutex;	
};

/* We limit the length of btn->file.rq to this (arbitrary) value */
#define BTN_MAX_RQLEN	32

static struct btn *btn_create_interface(struct net *net, enum btn_dev_type dev_type, int *retp);
static void btn_destroy_interface(struct btn *btn);
static int btn_get_unit(struct idr *p, void *ptr);
static void btn_unit_put(struct idr *p, int n);
//static void btn_make_etherframe(void *_eth_hdr, u8 *src_mac_addr, u8 *dst_mac_addr);


static __net_init int btn_init_net(struct net *net)
{
	struct btn_net *btn_net = net_generic(net, bt_net_id);

	idr_init(&btn_net->units_idr);
	mutex_init(&btn_net->all_btn_mutex);

	return 0;
}

static __net_exit void btn_exit_net(struct net *net)
{
	struct btn_net *btn_net;

	btn_net = net_generic(net, bt_net_id);
	idr_destroy(&btn_net->units_idr);
}

/* per net-namespace data */
static inline struct btn_net *btn_pernet(struct net *net)
{
	BUG_ON(!net);

	return net_generic(net, bt_net_id);
}

static struct pernet_operations bt_net_ops = {
	.init = btn_init_net,
	.exit = btn_exit_net,
	.id   = &bt_net_id,
	.size = sizeof(struct btn_net),
};

static ssize_t btn_read(struct file *file, char __user *buf,
			size_t count, loff_t *ppos)
{
    struct btn_file *pf = file->private_data;
    DECLARE_WAITQUEUE(wait, current);
    ssize_t ret;
    struct sk_buff *skb = NULL;
    
    
    if (!pf)
        return -ENXIO;
    
    add_wait_queue(&pf->rwait, &wait);
    for (;;) {
        set_current_state(TASK_INTERRUPTIBLE);
        skb = skb_dequeue(&pf->rq);
        if (skb)
            break;
        ret = 0;
        if (pf->dead)
            break;
        ret = -EAGAIN;
        if (file->f_flags & O_NONBLOCK)
            break;
        ret = -ERESTARTSYS;
        if (signal_pending(current))
            break;
        schedule();
    }
    set_current_state(TASK_RUNNING);
    remove_wait_queue(&pf->rwait, &wait);
    
    if (!skb)
        goto out;
    
    ret = -EOVERFLOW;
    if (skb->len > count)
        goto outf;
    ret = -EFAULT;
    if (copy_to_user(buf, skb->data, skb->len))
        goto outf;
    ret = skb->len;
    
outf:
    kfree_skb(skb);
out:
    return ret;
}

static ssize_t btn_write(struct file *file, const char __user *buf,
			 size_t count, loff_t *ppos)
{
    ssize_t ret;
    //char *eth_packet;

	struct btn_file *pf = file->private_data;
	struct sk_buff *skb;
	struct btn *btn = PF_TO_BTN(pf);
	int result;

	if (!pf)
		return -ENXIO;
	ret = -ENOMEM;
	//skb = dev_alloc_skb(count+14);
	skb = dev_alloc_skb(count);
	if (!skb)
		goto out;
	ret = -EFAULT;

/*
	eth_packet = kzalloc(count+14, GFP_KERNEL);
	random_ether_addr((u8 *) (eth_packet+6));
	memcpy(eth_packet, btn->dev->dev_addr, 6);
	*(eth_packet+12) = 0x08;
	*(eth_packet+13) = 0x00;
	copy_from_user(eth_packet+14, buf, count);

	memcpy(skb_put(skb, count+14), eth_packet, count+14);
*/
	if (copy_from_user(skb_put(skb, count), buf, count)) {
		kfree_skb(skb);
		goto out;
	}

	//random_ether_addr((u8 *) src_mac);
	//btn_make_etherframe(skb->data - ETH_HLEN, src_mac, btn->dev->dev_addr); 
	//skb_set_mac_header(skb, -ETH_HLEN);
	//skb_reset_mac_header(skb);
	
	skb->dev = btn->dev;
	if (btn->dev_type == BTN_IP_DEV)
	{
		skb->protocol  = htons(ETH_P_IP);
	}
	else
	{
		skb->protocol = eth_type_trans(skb, btn->dev); //htons(ETH_P_IP);
	}
	skb->ip_summed = CHECKSUM_UNNECESSARY;
	btn->dev->stats.rx_packets++;
	btn->dev->stats.rx_bytes += count+14;


	//src_mac = eth_packet+6;

	result = netif_rx(skb);
	printk(KERN_INFO "BT netif_rx result: %d", result); 

	ret = count;

	//kfree(eth_packet);
out:
	   return ret;

}

static unsigned int btn_poll(struct file *file, poll_table *wait)
{
    unsigned int mask;
    struct btn_file *pf = file->private_data;

    if (!pf)
        return 0;

    poll_wait(file, &pf->rwait, wait);		
    mask = POLLOUT | POLLWRNORM;
    if (skb_peek(&pf->rq))	
        mask |= POLLIN | POLLRDNORM;
    if (pf->dead)
        mask |= POLLHUP;

    return mask;
}

static long btn_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int err = -EFAULT;

    struct btn *btn;
    int __user *p = (int __user *)arg;

    switch (cmd) {
        case BTNIOCNEWUNIT_IP:
            mutex_lock(&btn_mutex);
            /* Create a new ip protocol btn unit */
            btn = btn_create_interface(current->nsproxy->net_ns, BTN_IP_DEV, &err);
            if (!btn)
            	break;
            file->private_data = &btn->file;
            //btn->owner = file;
            err = -EFAULT;
            if (put_user(btn->file.index, p))
            	break;
            err = 0;
            mutex_unlock(&btn_mutex);
            break;

        case BTNIOCNEWUNIT_ETH:
            mutex_lock(&btn_mutex);
            /* Create a new btn unit */
            btn = btn_create_interface(current->nsproxy->net_ns, BTN_ETH_DEV, &err);
            if (!btn)
            	break;
            file->private_data = &btn->file;
            //btn->owner = file;
            err = -EFAULT;
            if (put_user(btn->file.index, p))
            	break;
            err = 0;
            mutex_unlock(&btn_mutex);
            break;

        case BTNIOCSETMAC:
            dev_mac = kzalloc(ETH_ALEN, GFP_KERNEL);
            if (copy_from_user(dev_mac, (u8 __user *)arg, 6)) {
                kfree(dev_mac);
                dev_mac = NULL;
                break;
            }
            err = 0;
            break;

        default:
            err = -ENOTTY;
    }


    return err;
}

static int btn_open(struct inode *inode, struct file *file)
{
    /*
    * This could (should?) be enforced by the permissions on /dev/btn.
    */

    return 0;
}

static int btn_release(struct inode *unused, struct file *file)
{
    struct btn_file *pf = file->private_data;
    struct btn *btn;

    if (pf) {
        file->private_data = NULL;
        btn = PF_TO_BTN(pf);
        btn_destroy_interface(btn);
    }

    if (dev_mac) {
        kfree(dev_mac);
        dev_mac = NULL;
    }
        
    return 0;
}

static const struct file_operations bt_device_fops = {
	.owner		= THIS_MODULE,
	.read		= btn_read,
	.write		= btn_write,
	.poll		= btn_poll,
	.unlocked_ioctl	= btn_ioctl,
	.open		= btn_open,
	.release	= btn_release
};

/* Called at boot time if bt net device is compiled into the kernel,
   or at module load time (from init_module) if compiled as a module. */
static int __init btn_init(void)
{
	int err;
	unsigned int btn_major;

	printk(KERN_INFO "BT network driver init \n");

	err = register_pernet_device(&bt_net_ops);
	if (err) {
		printk(KERN_ERR "failed to register BT pernet device (%d)\n", err);
		goto out;
	}

	btn_major = register_chrdev(0, "btn", &bt_device_fops);
	printk(KERN_ERR "register BT chr device (%d)\n", err);

	btn_class = class_create(THIS_MODULE, "btn");
	if (IS_ERR(btn_class)) {
		err = PTR_ERR(btn_class);
		goto out_chrdev;
	}

	device_create(btn_class, NULL, MKDEV(btn_major, 0), NULL, "btn");
	btn_dev_num = btn_major;
	printk(KERN_ERR "register BT chr device done\n");

	return 0;

out_chrdev:
	unregister_chrdev(btn_major, "btn");
	unregister_pernet_device(&bt_net_ops);
out:
	return err;
}

static void __exit btn_cleanup(void)
{
	unregister_chrdev(MAJOR(btn_dev_num), "btn");
	device_destroy(btn_class, MKDEV(MAJOR(btn_dev_num), 0));
	class_destroy(btn_class);
	unregister_pernet_device(&bt_net_ops);
	unregister_chrdev_region(btn_dev_num, 1);
}

/*
 * Network interface unit routines.
 */
static int btn_net_open(struct net_device *dev)
{
    netif_start_queue(dev);
    
    return 0;
}


static netdev_tx_t
btn_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
    struct btn *btn = netdev_priv(dev);

    netif_stop_queue(dev);
    printk(KERN_INFO "BT start xmit data, unit: %d, len: %d \n", btn->file.index, skb->len);

    if (btn->file.rq.qlen > BTN_MAX_RQLEN)
        goto drop;

    skb_queue_tail(&btn->file.rq, skb);
    wake_up_interruptible(&btn->file.rwait);

    netif_wake_queue(dev);

    return NETDEV_TX_OK;	

drop:
    printk(KERN_INFO "BT start xmit data busy, rq len: %d \n", btn->file.rq.qlen);
    kfree_skb(skb);
    ++btn->dev->stats.tx_errors;
    return NETDEV_TX_OK;	
}

static int
btn_net_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
    //  No implementation at this moment.
    //  This is a place holder.
    
    return 0;
}

static void btn_tx_timeout(struct net_device *dev)
{
    //  No implementation at this moment.
    //  This is a place holder.
    
    netif_wake_queue(dev);   
}


static const struct net_device_ops bt_netdev_ops = {
    .ndo_open       = btn_net_open,
    .ndo_start_xmit = btn_start_xmit,
    .ndo_do_ioctl   = btn_net_ioctl,
    .ndo_tx_timeout = btn_tx_timeout,
};

static void btn_setup(struct net_device *dev)
{
    ether_setup(dev);

    dev->netdev_ops = &bt_netdev_ops;
    dev->mtu = ETH_DATA_LEN;
    dev->addr_len = ETH_ALEN;
    dev->tx_queue_len = 1000;

    if (dev_mac) {
        memcpy((u8 *) dev->dev_addr, dev_mac, ETH_ALEN);    // BT MAC Address
    }
    else {
        random_ether_addr((u8 *) dev->dev_addr);
    }
    
    printk(KERN_INFO "BT btn  dev addr: %x, %x, %x, %x, %x, %x \n", dev->dev_addr[0], dev->dev_addr[1], dev->dev_addr[2], 
    dev->dev_addr[3], dev->dev_addr[4], dev->dev_addr[5]);

}

/*
 * Initialize a btn_file structure.
 */
static void
btn_init_file(struct btn_file *pf)
{
    skb_queue_head_init(&pf->xq);
    skb_queue_head_init(&pf->rq);
    atomic_set(&pf->refcnt, 1);
    init_waitqueue_head(&pf->rwait);
}

/*
 * Create a new btn interface unit.  Fails if it can't allocate memory
 * or if there is already a unit with the requested number.
 * unit == -1 means allocate a new number.
 */
static struct btn *
btn_create_interface(struct net *net, enum btn_dev_type dev_type, int *retp)
{
    struct btn *btn;
    struct btn_net *btn_net;
    struct net_device *dev = NULL;
    int ret = -ENOMEM;
    int unit;

    dev = alloc_netdev(sizeof(struct btn), "", btn_setup);
    if (!dev)
        goto out1;


    if (dev_type == BTN_IP_DEV)
    {
        dev->header_ops = NULL;
        dev->flags = IFF_NOARP & (~IFF_BROADCAST & ~IFF_MULTICAST);
    }

    btn_net = btn_pernet(net);

    btn = netdev_priv(dev);
    btn->dev = dev;
    spin_lock_init(&btn->rlock);
    spin_lock_init(&btn->wlock);
    btn_init_file(&btn->file);

    /*
     * drum roll: don't forget to set
     * the net device is belong to
     */
    dev_net_set(dev, net);

    ret = -EEXIST;
    mutex_lock(&btn_net->all_btn_mutex);

    unit = btn_get_unit(&btn_net->units_idr, btn);
    btn->file.index = unit;
    sprintf(dev->name, "btn%d", unit);

    ret = register_netdev(dev);
    if (ret != 0) {
        printk(KERN_ERR "BTN: couldn't register device %s (%d)\n",
               dev->name, ret);
        goto out2;
    }
    printk(KERN_INFO "BT register net dev name: %s \n", dev->name);

    netif_carrier_on(dev);
    btn->btn_net = net;
    btn->dev_type = dev_type;

    atomic_inc(&btn_unit_count);
    mutex_unlock(&btn_net->all_btn_mutex);

    return btn;

out2:
    mutex_unlock(&btn_net->all_btn_mutex);
    free_netdev(dev);
out1:
    *retp = ret;
    return NULL;
}

/*
 * Take down a btn interface unit, and free the memory used by a btn unit. 
 * - called when the owning file (the one that created the unit) is closed or detached.
 */
static void btn_destroy_interface(struct btn *btn)
{
    struct btn_net *btn_net;

    btn_net = btn_pernet(btn->btn_net);	
    mutex_lock(&btn_net->all_btn_mutex);

    /* This will call dev_close() for us. */
    unregister_netdev(btn->dev);
    
    btn_unit_put(&btn_net->units_idr, btn->file.index);
    btn->file.dead = 1;
    wake_up_interruptible(&btn->file.rwait);
    
    mutex_unlock(&btn_net->all_btn_mutex);

    atomic_dec(&btn_unit_count);
    free_netdev(btn->dev);

}

/* get new free unit number and associate pointer with it */
static int btn_get_unit(struct idr *p, void *ptr)
{
	int unit, err;

again:
	if (!idr_pre_get(p, GFP_KERNEL)) {
		printk(KERN_ERR "BTN: No free memory for idr\n");
		return -ENOMEM;
	}

	err = idr_get_new_above(p, ptr, 0, &unit);
	if (err == -EAGAIN)
		goto again;

	return unit;
}

/* put unit number back to a pool */
static void btn_unit_put(struct idr *p, int n)
{
	idr_remove(p, n);
}

/*static void btn_make_etherframe(void *_eth_hdr, u8 *src_mac_addr, u8 *dst_mac_addr)
{
    struct ethhdr *eth_hdr = _eth_hdr;

    memcpy(eth_hdr->h_dest,   dst_mac_addr, sizeof(eth_hdr->h_dest));
    memcpy(eth_hdr->h_source, src_mac_addr, sizeof(eth_hdr->h_source));
    eth_hdr->h_proto = __constant_cpu_to_be16(ETH_P_IP);
}*/

/* Module/initialization stuff */

module_init(btn_init);
module_exit(btn_cleanup);

