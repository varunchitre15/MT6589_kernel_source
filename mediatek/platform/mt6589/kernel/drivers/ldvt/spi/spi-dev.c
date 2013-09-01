#include <linux/spi/spi.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/device.h>

#include <mach/mt6575_spi.h>

#define SPIDEV_LOG(fmt, args...) printk(KERN_INFO "SPIDEV: [%s]:[%d]" fmt, __func__, __LINE__, ##args) 
#define SPIDEV_MSG(fmt, args...) printk(KERN_INFO  fmt, ##args )

static ssize_t spi_store(struct device *dev, 
	struct device_attribute *attr, 
	const char *buf, size_t count)
{
	struct spi_device *spi;

	struct mt6575_chip_conf *chip_config;
	
	u32 setuptime, holdtime, high_time, low_time;
	u32 cs_idletime, ulthgh_thrsh;
	int cpol, cpha,tx_mlsb, rx_mlsb, tx_endian;
	int rx_endian, com_mod, pause, finish_intr;
	int deassert, tckdly, ulthigh;
	
	spi = container_of(dev, struct spi_device, dev);

	SPIDEV_LOG("SPIDEV name is:%s\n", spi->modalias);

	chip_config = (struct mt6575_chip_conf *) spi->controller_data;

	if (!chip_config) {
		SPIDEV_LOG ( "chip_config is NULL.\n");
		chip_config = kzalloc ( sizeof ( struct mt6575_chip_conf ), GFP_KERNEL );
		if ( !chip_config ) 
			return -ENOMEM;
	}	

	if (!strncmp(buf, "-h", 2 ) ) {
		SPIDEV_MSG("Please input the parameters for this device.\n");
	} else if ( !strncmp(buf, "-w", 2 ) ) {
		buf += 3;
		if (!buf) {
			SPIDEV_LOG("buf is NULL.\n");
			goto out;
		}
		if (!strncmp(buf, "setuptime=",10) && (1 == sscanf(buf+10, "%d", &setuptime))) {
			SPIDEV_MSG("setuptime is:%d\n", setuptime);
			chip_config->setuptime=setuptime;
		}else if (!strncmp(buf, "holdtime=", 9)&&(1==sscanf(buf+9, "%d", &holdtime))) {
			SPIDEV_MSG("Set holdtime is:%d\n", holdtime);
			chip_config->holdtime=holdtime;	
		}else if (!strncmp(buf, "high_time=", 10)&&(1==sscanf(buf+10, "%d", &high_time))) {
			SPIDEV_MSG("Set high_time is:%d\n", high_time);
			chip_config->high_time=high_time;	
		}else if (!strncmp(buf, "low_time=", 9)&&(1==sscanf(buf+9, "%d", &low_time))) {
			SPIDEV_MSG("Set low_time is:%d\n", low_time);
			chip_config->low_time=low_time;
		}else if (!strncmp(buf, "cs_idletime=", 12)&&(1==sscanf(buf+12, "%d", &cs_idletime))) {
			SPIDEV_MSG("Set cs_idletime is:%d\n", cs_idletime);
			chip_config->cs_idletime=cs_idletime;	
		}else if (!strncmp(buf, "ulthgh_thrsh=", 13)&&(1==sscanf(buf+13, "%d", &ulthgh_thrsh))) {
			SPIDEV_MSG("Set slwdown_thrsh is:%d\n", ulthgh_thrsh);
			chip_config->ulthgh_thrsh=ulthgh_thrsh; 
		}else if (!strncmp(buf, "cpol=", 5) && (1 == sscanf(buf+5, "%d", &cpol))){
			SPIDEV_MSG("Set cpol is:%d\n", cpol);
			chip_config->cpol = cpol;
		}else if (!strncmp(buf, "cpha=", 5) && (1 == sscanf(buf+5, "%d", &cpha))) {
			SPIDEV_MSG("Set cpha is:%d\n", cpha);
			chip_config->cpha = cpha;
		}else if (!strncmp(buf, "tx_mlsb=", 8)&&(1==sscanf(buf+8, "%d", &tx_mlsb))) {
			SPIDEV_MSG("Set tx_mlsb is:%d\n", tx_mlsb);
			chip_config->tx_mlsb=tx_mlsb;	
		}else if (!strncmp(buf, "rx_mlsb=", 8)&&(1==sscanf(buf+8, "%d", &rx_mlsb))) {
			SPIDEV_MSG("Set rx_mlsb is:%d\n", rx_mlsb);
			chip_config->rx_mlsb=rx_mlsb;	
		}else if (!strncmp(buf, "tx_endian=", 10)&&(1==sscanf(buf+10, "%d", &tx_endian))) {
			SPIDEV_MSG("Set tx_endian is:%d\n", tx_endian);
			chip_config->tx_endian=tx_endian;	
		}else if (!strncmp(buf, "rx_endian=", 10)&&(1==sscanf(buf+10, "%d", &rx_endian))) {
			SPIDEV_MSG("Set rx_endian is:%d\n", rx_endian);
			chip_config->rx_endian=rx_endian;	
		}else if (!strncmp(buf, "com_mod=", 8)&&(1==sscanf(buf+8, "%d", &com_mod))) {
			chip_config->com_mod=com_mod;
			SPIDEV_MSG("Set com_mod is:%d\n", com_mod);
		}else if (!strncmp(buf, "pause=", 6)&&(1==sscanf(buf+6, "%d", &pause))) {
			SPIDEV_MSG("Set pause is:%d\n", pause);
			chip_config->pause=pause;
		}else if (!strncmp(buf, "finish_intr=", 12)&&(1==sscanf(buf+12, "%d", &finish_intr))) {
			SPIDEV_MSG("Set finish_intr is:%d\n", finish_intr);
			chip_config->finish_intr=finish_intr;
		}else if (!strncmp(buf, "deassert=", 9)&&(1==sscanf(buf+9, "%d", &deassert))) {
			SPIDEV_MSG("Set deassert is:%d\n", deassert);
			chip_config->deassert=deassert;	
		}else if (!strncmp(buf, "ulthigh=", 8 ) && ( 1 == sscanf(buf+8, "%d", &ulthigh))) {
			SPIDEV_MSG("Set ulthigh is:%d\n", ulthigh);	
			chip_config->ulthigh=ulthigh;
		}else if (!strncmp(buf, "tckdly=",7) && ( 1 == sscanf(buf+7, "%d", &tckdly))) {
			SPIDEV_MSG("Set tckdly is:%d\n", tckdly);
			chip_config->tckdly=tckdly;
		}else {
			SPIDEV_LOG("Wrong parameters.\n");
			goto out;
		}
		spi->controller_data = chip_config;
		spi_setup(spi);
			
	}
out:
	return count;
}

static ssize_t spi_msg_store ( struct device *dev, 
	struct device_attribute *attr, 
	const char *buf, size_t count)
{

	struct spi_device *spi;

	struct spi_transfer *rv;
	struct spi_transfer *rv2;
	struct spi_transfer *rv3;
	struct spi_transfer transfer;
	struct spi_transfer transfer2;
	struct spi_transfer transfer3;
	struct spi_message msg;

	u32 cnt, i, len=4;
	u32 tx_buffer = 0x12345678;
	u32 rx_buffer = 0xaaaaaaaa;

	transfer.tx_buf = &tx_buffer;
	transfer.rx_buf = &rx_buffer;
	transfer.len = 4;	

	transfer2.tx_buf = &tx_buffer;
	transfer2.rx_buf = &rx_buffer;
	transfer2.len = 4;	

	transfer3.tx_buf = &tx_buffer;
	transfer3.rx_buf = &rx_buffer;
	transfer3.len = 4;	
	
	spi = container_of ( dev, struct spi_device, dev );

	if ( !spi ) {
		SPIDEV_LOG ( "spi device is invalid\n" );
		goto out;
	}

	spi_message_init ( &msg );

	if  ( !buf )
		goto out;

	if ( !strncmp ( buf, "-h", 2 ) ) {
		SPIDEV_MSG("Please input the message \
			of this device to send and receive. \n" );
	} else if ( !strncmp(buf, "-w", 2)) {
		buf += 3;
		if (!buf) {
			SPIDEV_LOG("Parameter is not enough.\n");
			goto out;
		}
		if (!strncmp ( buf, "len=", 4 ) && 1 == sscanf ( buf+4, "%d", &len ) ) {
			transfer.len=len;
			transfer.tx_buf = ( u32 * ) kzalloc ( len, GFP_KERNEL);
			transfer.rx_buf = ( u32 * ) kzalloc ( len, GFP_KERNEL);
			
			if (len%4)
				cnt = len/4 + 1;
			else
				cnt = len/4;

			for ( i = 0; i < cnt; i++ )
				*( ( u32 * ) transfer.tx_buf + i ) = tx_buffer;	

			spi_message_add_tail ( &transfer, &msg );
			spi_sync ( spi, &msg );
			rv = list_entry ( msg.transfers.next, struct spi_transfer, transfer_list );
			if (!rv) {
				SPIDEV_MSG ( "rv msg is NULL.\n" );
				goto out;
			}
			for ( i = 0; i < cnt, i++ )
				SPIDEV_LOG("rv message is:%x\n", *( ( u32 * ) rv->rx_buf + i ) );

			SPIDEV_LOG("Message length is:%d\n", len);

			kfree ( transfer.tx_buf );
			kfree ( transfer.rx_buf );
				
		}

	}else if  ( !strncmp(buf, "test_pause", 10 ) ) { 

		len = 32;
		transfer.len = len;
		transfer.tx_buf = ( u32 * ) kzalloc ( len, GFP_KERNEL );
		if ( !transfer.tx_buf ) {
			SPIDEV_LOG (" kzalloc memory for tx buf fails.\n");
			goto out;
		}
		transfer.rx_buf = ( u32 * ) kzalloc ( len, GFP_KERNEL );
		if ( !transfer.tx_buf ) {
			SPIDEV_LOG ( " kzalloc memory for rx buf fails.\n" );
			kfree(transfer.tx_buf);
			goto out;
		}

		if (len%4)
			cnt = len/4+1;
		else
			cnt = len/4;

		for ( i = 0; i < cnt; i++ )
			*( ( u32 * ) transfer.tx_buf+i ) = tx_buffer;	

		spi_message_add_tail(&transfer, &msg);

		len = 1024;
		transfer2.len=len;
		transfer2.tx_buf = ( u32 * ) kzalloc ( len, GFP_KERNEL );
		if ( !transfer2.tx_buf ) {
			SPIDEV_LOG (" kzalloc memory for tx buf fails.\n");
			kfree(transfer.tx_buf);
			kfree(transfer.rx_buf);
			goto out;
		}
		transfer2.rx_buf = ( u32 * ) kzalloc ( len, GFP_KERNEL );
		if ( !transfer2.rx_buf ) {
			SPIDEV_LOG (" kzalloc memory for rx buf fails.\n");
			kfree(transfer.tx_buf);
			kfree(transfer.rx_buf);
			kfree(transfer2.tx_buf);
			goto out;
		}

		if (len%4)
			cnt = len/4+1;
		else
			cnt = len/4;

		for ( i = 0; i < cnt; i++ )
			*( ( u32 * ) transfer2.tx_buf + i ) = tx_buffer+i;	

		spi_message_add_tail ( &transfer2, &msg );

		len = 32;
		transfer3.len=len;
		transfer3.tx_buf = ( u32 * ) kzalloc ( len, GFP_KERNEL );
		if ( !transfer3.tx_buf ) {
			SPIDEV_LOG (" kzalloc memory for tx buf fails.\n");
			kfree(transfer.tx_buf);
			kfree(transfer.rx_buf);
			kfree(transfer2.tx_buf);
			kfree(transfer2.rx_buf);
			goto out;
		}
		transfer3.rx_buf = ( u32 * ) kzalloc ( len, GFP_KERNEL );
		if ( !transfer3.rx_buf ) {
			SPIDEV_LOG (" kzalloc memory for rx buf fails.\n");
			kfree(transfer.tx_buf);
			kfree(transfer.rx_buf);
			kfree(transfer2.tx_buf);
			kfree(transfer2.rx_buf);
			kfree(transfer3.tx_buf);
			goto out;
		}

		if (len%4)
			cnt = len/4+1;
		else
			cnt = len/4;

		for ( i = 0; i < cnt; i++ )
			*( ( u32 * ) transfer3.tx_buf+i) = tx_buffer;	

		spi_message_add_tail ( &transfer3, &msg );

		spi_sync ( spi, &msg );

		rv = list_entry ( msg.transfers.next, struct spi_transfer, transfer_list );
		if ( !rv ) {
			SPIDEV_MSG ( "rv msg is NULL.\n" );
			goto out;
		}
		if ( rv->len%4 )
			cnt=rv->len/4+1;
		else
			cnt = rv->len/4;

		SPIDEV_LOG ( "CNT is :%d\n", cnt );
		for ( i = 0; i < cnt; i++ )
			SPIDEV_LOG ( "rv message is:%x\n", *((u32 *)rv->rx_buf + i ) );

		rv2 = list_entry ( rv->transfer_list.next, struct spi_transfer, transfer_list );
		if ( !rv2 ) {
			SPIDEV_MSG ( "rv msg is NULL.\n" );
			goto out;
		}
		if (rv2->len%4)
			cnt=rv2->len/4+1;
		else
			cnt = rv2->len/4;

		SPIDEV_LOG ( "CNT is :%d\n", cnt );
		for (i=0; i<cnt; i++)
			SPIDEV_LOG ( "rv message is:%x\n", *((u32 *)rv2->rx_buf + i ) );

		rv3 = list_entry( rv2->transfer_list.next,  struct spi_transfer,  transfer_list);
		if (!rv3) {
			SPIDEV_MSG("rv msg is NULL.\n");
			goto out;
		}
		if (rv3->len%4)
			cnt=rv3->len/4+1;
		else
			cnt = rv3->len/4;

		SPIDEV_LOG ( "CNT is :%d\n", cnt);
		for ( i = 0; i < cnt; i++ )
			SPIDEV_LOG("rv message is:%x\n", * ( ( u32 *)rv3->rx_buf+i ) );

		kfree(transfer.tx_buf);
		kfree(transfer.rx_buf);

		kfree(transfer2.tx_buf);
		kfree(transfer2.rx_buf);

		kfree(transfer3.tx_buf);
		kfree(transfer3.rx_buf);
		
		
	}else{
		SPIDEV_LOG("Wrong parameters.\n");
		goto out;
	}

out:
	return count;
		
}

static DEVICE_ATTR(spi, S_IWUGO|S_IRUGO, NULL, spi_store);
static DEVICE_ATTR(spi_msg, S_IWUGO|S_IRUGO, NULL, spi_msg_store);

static struct device_attribute *spi_attribute[]={
	&dev_attr_spi,
	&dev_attr_spi_msg,
};

static int spi_create_attribute(struct device *dev)
{
	int num,idx;
	int err =0;
	num = (int)(sizeof(spi_attribute)/sizeof(spi_attribute[0]));

	for (idx = 0; idx < num; idx ++) {
		if ((err = device_create_file(dev, spi_attribute[idx])))
			break;
	}
	return err;
	
}

static void spi_remove_attribute(struct device *dev)
{
	int num, idx;
	num = (int)(sizeof(spi_attribute)/sizeof(spi_attribute[0]));

	for (idx = 0; idx < num; idx ++) {
		device_remove_file(dev, spi_attribute[idx]);
	}

	return;
}

static int spi_test_remove(struct spi_device *spi)
{
	#if 0
	struct mt6575_chip_conf *chip_config;

	chip_config = spi->controller_data;

	if (chip_config) {
		SPIDEV_LOG("kfree chip_config\n");
		kfree(chip_config);
	}
	spi->controller_data=NULL;
	#endif
	SPIDEV_LOG("spi_test_remove.\n");
	spi_remove_attribute(&spi->dev);
	return 0;
}

/*static int spidev_init(struct spi_device *spi)
{
	struct mt6575_chip_conf *chip_config;
	
	chip_config = (struct mt6575_chip_conf *) spi->controller_data;

	//SPIDEV_LOG("com_mod is:%d\n", chip_config->com_mod);

	if (!chip_config) {
		SPIDEV_LOG("chip_config is NULL.\n");
		chip_config = kzalloc(sizeof(struct mt6575_chip_conf), GFP_KERNEL);
		if (!chip_config) 
			return -ENOMEM;
	}	
	chip_config->setuptime=1;
	chip_config->holdtime=1;
	chip_config->high_time=10;
	chip_config->low_time=10;
	chip_config->cs_idletime=2;
	chip_config->tx_mlsb=0;
	chip_config->rx_mlsb=0;
	chip_config->tx_endian=0;
	chip_config->rx_endian=0;
	chip_config->com_mod=1;

	//spi_set_ctldata(spi, &chip_config);
	spi->controller_data=chip_config;
	spi_setup(spi);
	SPIDEV_LOG("spidev com_mod is:%d\n", chip_config->com_mod);
	return 0;
}*/
static int __init spi_test_probe(struct spi_device *spi)
{
	//spidev_init(spi);
	spi->mode = SPI_MODE_0;
	spi->bits_per_word = 32;
	spi_create_attribute ( &spi->dev );
	return 0;
}

//struct spi_device_id spi_id_table = {"test_spi", 0};
struct spi_device_id spi_id_table = {"spidev", 0};

static struct spi_driver spi_test_driver = {
	.driver = {
		.name = "test_spi",
		.bus = &spi_bus_type,
		.owner = THIS_MODULE,
	},
	.probe = spi_test_probe,
	.remove=spi_test_remove,
	.id_table = &spi_id_table,
};

static int __init spi_dev_init(void)
{
	SPIDEV_LOG("SPI_DEV_INIT.\n");
	return spi_register_driver(&spi_test_driver);
}

static void __exit spi_test_exit(void)
{
	SPIDEV_LOG("SPI_DEV_EXIT.\n");
	spi_unregister_driver(&spi_test_driver);
	
	return;
}

module_init(spi_dev_init);
module_exit(spi_test_exit);

MODULE_DESCRIPTION ( "MT6575 SPI test device driver" );
MODULE_AUTHOR ( "Li Zhang <cindy.zhang@mediatek.com>" );
MODULE_LICENSE("GPL");
