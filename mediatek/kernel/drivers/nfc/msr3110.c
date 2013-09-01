

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/spinlock.h>
#include <linux/dma-mapping.h>

#include <cust_gpio_usage.h>
#include <cust_eint_ext.h>

#include <mach/mt_gpio.h>
#include <mach/eint.h>



#define TRACE_THIS_MODULE   0
#define MY_DEBUG    0


#if ( TRACE_THIS_MODULE == 1)

#define ENTER() \
		printk(KERN_ERR "[%s]start\n", __FUNCTION__)
#define FUNC_START() \
		printk(KERN_ERR "[%s]START\n", __FUNCTION__)
#define FUNC_END() \
		printk(KERN_ERR "[%s]END\n", __FUNCTION__)

#else /* TRACE_THIS_MODULE */

#define ENTER()
#define FUNC_START()
#define FUNC_END()

#endif /* TRACE_THIS_MODULE */

#if ( MY_DEBUG == 1)

#define my_pr_info      pr_err
#define my_pr_debug     pr_err
#define my_pr_err       pr_err
#define my_pr_warning   pr_err

#else

#define my_pr_info      pr_info
#define my_pr_debug     pr_debug
#define my_pr_err       pr_err
#define my_pr_warning   pr_warning

#endif


//****************************************
// Global
//****************************************

// I2C Bus
static struct i2c_client *nfc_client = NULL;
static struct i2c_adapter *adapter = NULL;


// global structure for internal control
typedef struct _msr3110_dev
{
	struct mutex    mutex_irq_handler;	
	struct mutex    mutex_ioctl_irq;
	wait_queue_head_t irq_wait_queue;
	int irq_flag;
		
} msr3110_dev;

msr3110_dev *g_msr3110_dev = NULL;

//For DMA
#define MAX_BUFFER_SIZE 255
static char *I2CDMAWriteBuf = NULL;
static unsigned int I2CDMAWriteBuf_pa;// = NULL;
static char *I2CDMAReadBuf = NULL;
static unsigned int I2CDMAReadBuf_pa;// = NULL;

#define NFC_CLIENT_I2C_BUS_LOG_OFF 1
#if ( NFC_CLIENT_I2C_BUS_LOG_OFF == 1)
	#define SET_NFC_CLIENT_BUS_LOG() { my_pr_debug( "%s SET_NFC_CLIENT_BUS_LOG\n", __func__); nfc_client->addr |=I2C_A_FILTER_MSG;} 
	//0x4000 not I2C_A_FILTER_MSG.
	//it's bus driver bug, comment by i2c owner, ranran
#else
	#define SET_NFC_CLIENT_BUS_LOG()
#endif


static int nfc_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int nfc_remove(struct i2c_client *client);

void msr3110_dev_irq_handler(void);


static int nfc_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	FUNC_START();
	
    my_pr_debug( "%s clinet 0x%p id %s", __func__, client, id->name);
    adapter = to_i2c_adapter( client->dev.parent);
    my_pr_debug( "%s master tx 0x%p id %s", __func__, client->adapter->algo->master_xfer, id->name);
    my_pr_debug( "%s master tx 0x%p id %s", __func__, adapter->algo->master_xfer, id->name);
    nfc_client = client;

	FUNC_END();
    return 0;
}

static int nfc_remove(struct i2c_client *client)
{
	FUNC_START();

	nfc_client = NULL;

	FUNC_END();
    return 0;
}

#define NFC_I2C_BUSNUM  5
#define I2C_ID_NAME "msr3110"
static struct i2c_board_info __initdata nfc_board_info = { I2C_BOARD_INFO(I2C_ID_NAME, 0x59)};

static const struct i2c_device_id nfc_id[] = {
	{ I2C_ID_NAME,  0x59, },  //Mstar MSR3110 NFC driver
	{},
};

static struct i2c_driver nfc_driver = {
	.driver = {
		.name	= I2C_ID_NAME,
	},
	.probe		= nfc_probe,
	.remove		= nfc_remove,
	.id_table	= nfc_id,
};


// misc driver 
static int msr3110_dev_open( struct inode *inode, struct file *filp)
{
	FUNC_START()

	FUNC_END();
	return 0;
}

static int msr3110_dev_release( struct inode *inode, struct file *filp)
{
	FUNC_START();

	FUNC_END();	
	return 0;
}

#define MSR3110_DEV_WRITE_MAX 256
#define MSR3110_CMD_I2C_ID 0x51

#define NFC_CLIENT_TIMING_ON 1
#if ( NFC_CLIENT_TIMING_ON == 1)
	#define NFC_CLIENT_TIMING 250
	#define SET_NFC_CLIENT_TIMING() { nfc_client->timing = NFC_CLIENT_TIMING;}
#else
	#define NFC_CLIENT_TIMING
	#define SET_NFC_CLIENT_TIMING()
#endif
static ssize_t msr3110_dev_write(struct file *filp, const char __user *buf,	size_t count, loff_t *offset)
{
	ssize_t retVal = 0; 

	FUNC_START();
	
	my_pr_debug( "%s count: %d \n", __func__, count);

	if( count > MSR3110_DEV_WRITE_MAX)
	{
		count = MSR3110_DEV_WRITE_MAX;
	}
	
	if( copy_from_user( I2CDMAWriteBuf, buf, count))
	{	
		my_pr_err( "%s fail to copy from user \n", __func__);
		retVal = -EFAULT;
		goto end;
	}
	
	nfc_client->addr = ( MSR3110_CMD_I2C_ID & I2C_MASK_FLAG);
	SET_NFC_CLIENT_TIMING();
	SET_NFC_CLIENT_BUS_LOG();
	
	nfc_client->ext_flag |= I2C_DMA_FLAG;
	nfc_client->ext_flag |= I2C_DIRECTION_FLAG;
	
	retVal = i2c_master_send( nfc_client, (unsigned char *)I2CDMAWriteBuf_pa, count);
	my_pr_debug( "%s rcv count: %d \n", __func__, retVal);
	if( retVal < 0)
	{
		//my_pr_err( "%s i2c_master_send fail. retVal: %d ", __func__, retVal);
		goto end;		
	}
	if( retVal != count)
	{
		//my_pr_err( "%s i2c_master_send retVal != count. retVal: %d ", __func__, retVal);
		retVal = -EIO;
		goto end;
	}
	
end:

	FUNC_END();
	return retVal;
}

#define MSR3110_DEV_READ_MAX 255
#define MSR3110_DEV_READ_RETRY_COUNT 50
#define MSR3110_DEV_READ_RETRY_DELAY 1
static ssize_t msr3110_dev_read(struct file *filp, char __user *buf, size_t count, loff_t *offset)
{
	ssize_t retVal = 0;
	unsigned char tmp[MSR3110_DEV_READ_MAX];
	int loopIdx = 0;
	int readByteCount = 0;;

	FUNC_START();
	
	my_pr_debug( "%s count: %d\n", __func__, count);
	if( count > MSR3110_DEV_READ_MAX)
	{
		count = MSR3110_DEV_READ_MAX;
	}
	
	nfc_client->addr = ( MSR3110_CMD_I2C_ID & I2C_MASK_FLAG);    
    nfc_client->ext_flag |= I2C_DMA_FLAG;
    nfc_client->ext_flag |= I2C_DIRECTION_FLAG;    
	SET_NFC_CLIENT_TIMING();
	SET_NFC_CLIENT_BUS_LOG();

	for( loopIdx = 0; loopIdx < MSR3110_DEV_READ_RETRY_COUNT; loopIdx++)
	{
		retVal = i2c_master_recv( nfc_client, (unsigned char *)I2CDMAReadBuf_pa, count);
		my_pr_debug( "%s loopIdx: %d, recv retVal: %d \n", __func__, loopIdx, retVal);

		if( retVal == count)
		{
			my_pr_debug( "%s SUCCESS: recv. \n", __func__);
			break;			
		}
		
		if( retVal < 0)
		{
			//my_pr_err( "%s FAIL: recv. CONTINUE. \n", __func__);
			usleep_range( 900, 1000);
			continue;			
		}
		
		if( retVal > 2)
		{
			//my_pr_err( "%s FAIL: recv too many bytes. goto end. \n", __func__);
			//retVal = -EIO;
			retVal = 0;
			goto end;
		}
	}

	if( retVal < 0)
	{
		my_pr_err( "%s FAIL: recv. out of retry count, goto end. \n", __func__);
		retVal = 0;
		goto end;
	}

	readByteCount = retVal;

#if 0
	for( loopIdx = 0; loopIdx < MSR3110_DEV_READ_RETRY_COUNT; loopIdx++)
	{
		msleep(MSR3110_DEV_READ_RETRY_DELAY);
		retVal = i2c_master_recv( nfc_client, tmp, 2);
		my_pr_info( "%s loopIdx: %d, recv retVal: %d \n", __func__, loopIdx, retVal);

		if( retVal == 2)
		{
			my_pr_info( "%s SUCCESS: recv. \n", __func__);
			break;			
		}
		
		if( retVal < 0)
		{
			my_pr_err( "%s FAIL: recv. CONTINUE. \n", __func__);
			continue;			
		}
		
		if( retVal > 2)
		{
			my_pr_err( "%s FAIL: recv too many bytes. goto end. \n", __func__);
			retVal = -EIO;
			goto end;
		}
	}
	if( retVal < 0)
	{
		my_pr_err( "%s FAIL: recv. out of retry count, goto end. \n", __func__);
		goto end;
	}

	readByteCount = 2;
	
	if( tmp[1] > 0)
	{
		for( loopIdx = 0; loopIdx < MSR3110_DEV_READ_RETRY_COUNT; loopIdx++)
		{
			msleep(1);
			retVal = i2c_master_recv( nfc_client, &tmp[2], tmp[1] + 2);
			my_pr_info( "%s loopIdx: %d, recv retVal: %d", __func__, loopIdx, retVal);

			if( retVal == ( tmp[1] + 2))
			{
				my_pr_info( "%s SUCCESS: recv data.", __func__);
				break;		
			}
			
			if( retVal < 0)
			{
				my_pr_err( "%s FAIL: recv. CONTINUE.", __func__);
				continue;
			}		

			if( retVal > ( tmp[1] + 2))
			{
				my_pr_err( "%s FAIL: recv too many bytes. goto end.", __func__);
				retVal = -EIO;
				goto end;		
			}
		}
	}
	if( retVal < 0)
	{
		my_pr_err( "%s FAIL: recv. out of retry count, goto end.", __func__);
		goto end;
	}

	readByteCount += tmp[1] + 2;
	retVal = readByteCount;
#endif

	if ( copy_to_user( buf, I2CDMAReadBuf, readByteCount)) {
		my_pr_err("%s : failed to copy to user space\n", __func__);
		retVal =  0;
		goto end;
	}	
	
	
end:
	
	//for( loopIdx = 0; loopIdx < readByteCount; loopIdx++)
	//{
	//	my_pr_debug( "%s %02X", __func__, tmp[loopIdx]);		
	//}
	
	FUNC_END();
	return retVal;	
	
}

#define MSR3110_DEV_MAGIC_ID 0xCD
#define MSR3110_IOCTL_FW_UPGRADE _IOW( MSR3110_DEV_MAGIC_ID, 0x00, int)
#define MSR3110_IOCTL_SET_VEN   _IOW( MSR3110_DEV_MAGIC_ID, 0x01, int)
#define MSR3110_IOCTL_SET_RST   _IOW( MSR3110_DEV_MAGIC_ID, 0x02, int)
#define MSR3110_IOCTL_IRQ       _IOW( MSR3110_DEV_MAGIC_ID, 0x03, int)
#define MSR3110_IOCTL_IRQ_ABORT _IOW( MSR3110_DEV_MAGIC_ID, 0x04, int)
#define MSR3110_IOCTL_IRQ_REG _IOW( MSR3110_DEV_MAGIC_ID, 0x05, int)


#define MSR3110_IOCTL_ISP_READ_REG _IOW( MSR3110_DEV_MAGIC_ID, 0xA1, int)
#define MSR3110_IOCTL_ISP_WRITE_REG _IOW( MSR3110_DEV_MAGIC_ID, 0xA2, int)
#define MSR3110_IOCTL_SET_IRQ_NFC_PIN _IOW( MSR3110_DEV_MAGIC_ID, 0xA3, int)
#define MSR3110_IOCTL_SET_NFC_EINT_PIN _IOW( MSR3110_DEV_MAGIC_ID, 0xA4, int)


typedef struct _msr3110fw_upgrade_info
{
	unsigned long FwBufLen;
	void* FwBuf;

} msr3110fw_upgrade_info;

#define MSR3110_ISP_SLAVE_I2C_ID 0x59
//#define MSR3110_ISP_FLASH_I2C_ID 0x49
#define MSR3110_ISP_FLASH_I2C_ID 0x79
#define MSR3110_ISP_FLASH_I2C_ID_SHIFT 0xF2
#define Antares_SLAVE_ADDR  MSR3110_ISP_SLAVE_I2C_ID
#define I2CtoSPIFlash_SLAVE_ADDR    MSR3110_ISP_FLASH_I2C_ID

#define SERIALFLASH_CMD_DATA_WRITE  0x10
#define SERIALFLASH_CMD_DATA_READ   0x11
#define SERIALFLASH_CMD_DATA_END    0x12
#define SERIALFLASH_CMD_RESET_ISP   0x24

//#define FLASH_START 0x00010000
#define FLASH_START 0x00000000
#define FLASH_END   0x00020000

enum
{
    FLASH_UNKNOWN,
    FLASH_SST,
    FLASH_MX
};


static int nfc_i2c_writebytes(u8 addr, u32 len, u8 *buf)
{		
    int retVal;   

	memcpy( I2CDMAWriteBuf, buf, len);
	
	nfc_client->addr = ( addr & I2C_MASK_FLAG);
	SET_NFC_CLIENT_TIMING();
	SET_NFC_CLIENT_BUS_LOG();
	
	nfc_client->ext_flag |= I2C_DMA_FLAG;
	nfc_client->ext_flag |= I2C_DIRECTION_FLAG;
	
	retVal = i2c_master_send( nfc_client, (unsigned char *)I2CDMAWriteBuf_pa, len);
	if( retVal != len)
	{
		my_pr_err( "%s i2c_master_send fail\n", __func__);
		retVal = -EIO;
		goto end;
	}
	
end:
	
	return retVal;
}

static void nfc_i2c_readbytes(u8 addr, u32 wlen, u32 rlen, u8 *wbuf, u8 *rbuf)
{
	int retVal;
    struct i2c_msg msgs[] = {
	{
            .addr   = addr,
            .flags  = 0,
            .len    = wlen,
            .buf    = wbuf,
        },
        {
            .addr   = addr,
            .flags  = I2C_M_RD,
            .len    = rlen,
            .buf    = rbuf,
        }

    };

    retVal = i2c_transfer( nfc_client->adapter, msgs, 2);
	
    if( retVal <= 0)
    {
		my_pr_err( "%s ERROR: i2c_transfer, retVal: %d", __func__, retVal);
    }

}

static void SerialFlash_EnterSerialDebug(void)
{	
    u8 cmd[5] = {0x53, 0x45, 0x52, 0x44, 0x42};

	FUNC_START();
	
    nfc_i2c_writebytes(Antares_SLAVE_ADDR, sizeof(cmd), cmd);

	FUNC_END();
}

static void SerialFlash_EnterSingleStep(void)
{
    u8 cmd1[4] = {SERIALFLASH_CMD_DATA_WRITE, 0xC0, 0xC1, 0x53};
    u8 cmd2[4] = {SERIALFLASH_CMD_DATA_WRITE, 0x1F, 0xC1, 0x53};

	FUNC_START();
	
    nfc_i2c_writebytes(Antares_SLAVE_ADDR, sizeof(cmd1), cmd1);
    nfc_i2c_writebytes(Antares_SLAVE_ADDR, sizeof(cmd2), cmd2);

	FUNC_END();
}

static void SerialFlash_ExitSingleStep(void)
{	
    u8 cmd1[4] = {SERIALFLASH_CMD_DATA_WRITE, 0xC0, 0xC1, 0xFF};
    u8 cmd2[4] = {SERIALFLASH_CMD_DATA_WRITE, 0x1F, 0xC1, 0xFF};

	FUNC_START();
	
    nfc_i2c_writebytes(Antares_SLAVE_ADDR, sizeof(cmd1), cmd1);
    nfc_i2c_writebytes(Antares_SLAVE_ADDR, sizeof(cmd2), cmd2);

	FUNC_END();
}

static void SerialFlash_ExitSerialDebug(void)
{
    u8 cmd[1] = {0x45};
	
	FUNC_START();

	nfc_i2c_writebytes(Antares_SLAVE_ADDR, sizeof(cmd), cmd);

	FUNC_END();
}

u8 SerialFlash_DisableWDT(void)
{
    u8 cmd1[4] = {SERIALFLASH_CMD_DATA_WRITE, 0x09, 0x08, 0x55};
    u8 cmd2[4] = {SERIALFLASH_CMD_DATA_WRITE, 0x09, 0x09, 0xAA};
    u8 result = false;
    u8 retry = 0;
    u8 res1 = 0;
    u8 res2 = 0;

	FUNC_START();
	
    while(1)
    {
        SerialFlash_EnterSerialDebug();
        SerialFlash_EnterSingleStep();

        nfc_i2c_writebytes(Antares_SLAVE_ADDR, sizeof(cmd1), cmd1);
        nfc_i2c_writebytes(Antares_SLAVE_ADDR, sizeof(cmd2), cmd2);
        nfc_i2c_readbytes(Antares_SLAVE_ADDR, 3, 1, cmd1, &res1);
        nfc_i2c_readbytes(Antares_SLAVE_ADDR, 3, 1, cmd2, &res2);

        SerialFlash_ExitSingleStep();
        SerialFlash_ExitSerialDebug();

        if (res1 == cmd1[3] && res2 == cmd2[3])
        {
            result = true;
            break;
        }
        else if (++retry == 10)
        {
            result = false;
            break;
        }
        break;
    }

	FUNC_END();

    return result;
}

u8 SerialFlash_Chg_Flash_I2C_Addrs(void)
{
	
    u8 cmd1[4] = {SERIALFLASH_CMD_DATA_WRITE, 0x08, 0x00, MSR3110_ISP_FLASH_I2C_ID_SHIFT};
    u8 result = false;
    u8 retry = 0;
    u8 res1 = 0;

	FUNC_START();

    while(1)
    {
        SerialFlash_EnterSerialDebug();
        SerialFlash_EnterSingleStep();

        nfc_i2c_writebytes(Antares_SLAVE_ADDR, sizeof(cmd1), cmd1);    
        nfc_i2c_readbytes(Antares_SLAVE_ADDR, 3, 1, cmd1, &res1);

        SerialFlash_ExitSingleStep();
        SerialFlash_ExitSerialDebug();

        if ( res1 == cmd1[3])
        {
            result = true;
            break;
        }
        else if (++retry == 10)
        {
            result = false;
            break;
        }
        break;
    }

	FUNC_END();

    return result;
}



static void SerialFlash_EntryIspMode(void)
{
    u8 cmd[5] = {0x4D, 0x53, 0x54, 0x41, 0x52};

	FUNC_START();
	
    nfc_i2c_writebytes(I2CtoSPIFlash_SLAVE_ADDR, sizeof(cmd), cmd);

	FUNC_END();
}

static void SerialFlash_ExitIspMode(void)
{
    u8 cmd[1] = {0x24};

	FUNC_START();
	
    nfc_i2c_writebytes(I2CtoSPIFlash_SLAVE_ADDR, sizeof(cmd), cmd);

	FUNC_END();
}

static void SerialFlash_ReadChipID(u8* id)
{
    u8 cmd1[2] = {SERIALFLASH_CMD_DATA_WRITE, 0x9F};
    u8 cmd2[1] = {SERIALFLASH_CMD_DATA_READ};
    u8 cmd3[1] = {SERIALFLASH_CMD_DATA_END};

	FUNC_START();
	
    nfc_i2c_writebytes(I2CtoSPIFlash_SLAVE_ADDR, sizeof(cmd1), cmd1);
    nfc_i2c_readbytes(I2CtoSPIFlash_SLAVE_ADDR, 1, 3, cmd2, id);
    nfc_i2c_writebytes(I2CtoSPIFlash_SLAVE_ADDR, sizeof(cmd3), cmd3);

	FUNC_END();
}

static u8 SerialFlash_ReadStatus(void)
{	
    u8 cmd1[2] = {SERIALFLASH_CMD_DATA_WRITE, 0x05};
    u8 cmd2[1] = {SERIALFLASH_CMD_DATA_READ};
    u8 cmd3[1] = {SERIALFLASH_CMD_DATA_END};
    u8 status = 0;

    nfc_i2c_writebytes(I2CtoSPIFlash_SLAVE_ADDR, sizeof(cmd1), cmd1);
    nfc_i2c_readbytes(I2CtoSPIFlash_SLAVE_ADDR, 1, 1, cmd2, &status);
    nfc_i2c_writebytes(I2CtoSPIFlash_SLAVE_ADDR, sizeof(cmd3), cmd3);

    return status;
}

static void SerialFlash_WriteEnable(void)
{	
    u8 cmd1[2] = {SERIALFLASH_CMD_DATA_WRITE, 0x06};
    u8 cmd2[1] = {SERIALFLASH_CMD_DATA_END};

    nfc_i2c_writebytes(I2CtoSPIFlash_SLAVE_ADDR, sizeof(cmd1), cmd1);
    nfc_i2c_writebytes(I2CtoSPIFlash_SLAVE_ADDR, sizeof(cmd2), cmd2);
}

static void SerialFlash_WriteDisable(void)
{
    u8 cmd1[2] = {SERIALFLASH_CMD_DATA_WRITE, 0x04};
    u8 cmd2[1] = {SERIALFLASH_CMD_DATA_END};

    nfc_i2c_writebytes(I2CtoSPIFlash_SLAVE_ADDR, sizeof(cmd1), cmd1);
    nfc_i2c_writebytes(I2CtoSPIFlash_SLAVE_ADDR, sizeof(cmd2), cmd2);
}

static void SerialFlash_WriteStatus(u8 flash_type, u8 status)
{
    u8 cmd1[2] = {SERIALFLASH_CMD_DATA_WRITE, 0x50};
    u8 cmd2[3] = {SERIALFLASH_CMD_DATA_WRITE, 0x01, 0x00};
    u8 cmd3[1] = {SERIALFLASH_CMD_DATA_END};

    SerialFlash_WriteEnable();

    if (flash_type == FLASH_SST)
    {
        nfc_i2c_writebytes(I2CtoSPIFlash_SLAVE_ADDR, sizeof(cmd1), cmd1);
        nfc_i2c_writebytes(I2CtoSPIFlash_SLAVE_ADDR, sizeof(cmd3), cmd3);
    }
    cmd2[2] = status;
    nfc_i2c_writebytes(I2CtoSPIFlash_SLAVE_ADDR, sizeof(cmd2), cmd2);
    nfc_i2c_writebytes(I2CtoSPIFlash_SLAVE_ADDR, sizeof(cmd3), cmd3);

    SerialFlash_WriteDisable();

}

static void SerialFlash_ChipErase(void)
{
    u8 cmd1[2] = {SERIALFLASH_CMD_DATA_WRITE, 0x60};
    u8 cmd2[1] = {SERIALFLASH_CMD_DATA_END};

	FUNC_START();
	
    SerialFlash_WriteEnable();

    nfc_i2c_writebytes(I2CtoSPIFlash_SLAVE_ADDR, sizeof(cmd1), cmd1);
    nfc_i2c_writebytes(I2CtoSPIFlash_SLAVE_ADDR, sizeof(cmd2), cmd2);

	FUNC_END();
}

static void SerialFlash_WriteData(u8 flash_type, u32 addr, u32 wsize, u8 *data)
{	
    u8 cmd1[7] = {SERIALFLASH_CMD_DATA_WRITE, 0xAD, 0x00, 0x00, 0x00, 0x00, 0x00};
    u8 cmd2[4] = {SERIALFLASH_CMD_DATA_WRITE, 0xAD, 0x00, 0x00};
    u8 cmd3[1] = {SERIALFLASH_CMD_DATA_END};
    //u8 cmd4[6] = {I2CtoSPIFlash_SLAVE_ADDR, SERIALFLASH_CMD_DATA_WRITE, 0x02, 0x00, 0x00, 0x00};
    u32 wcnt = 0;

    SerialFlash_WriteEnable();

    if (flash_type == FLASH_SST)
    {
        cmd1[2] = addr>>16;
        cmd1[3] = addr>>8;
        cmd1[4] = addr;
        cmd1[5] = data[wcnt++];
        cmd1[6] = data[wcnt];
        nfc_i2c_writebytes(I2CtoSPIFlash_SLAVE_ADDR, sizeof(cmd1), cmd1);
        nfc_i2c_writebytes(I2CtoSPIFlash_SLAVE_ADDR, sizeof(cmd3), cmd3);

        for (wcnt=2; wcnt<wsize; wcnt++)
        {
            cmd2[2] = data[wcnt++];
            cmd2[3] = data[wcnt];
            nfc_i2c_writebytes(I2CtoSPIFlash_SLAVE_ADDR, sizeof(cmd2), cmd2);
            nfc_i2c_writebytes(I2CtoSPIFlash_SLAVE_ADDR, sizeof(cmd3), cmd3);
        }
    }
#if 0
    else if (flash_type == FLASH_MX)
    {
	u8 *cmd = kmalloc(wsize+5, GFP_KERNEL);
	if(cmd)
	{
	    cmd[0] = SERIALFLASH_CMD_DATA_WRITE;
	    cmd[1] = 0x02;
	    cmd[2] = addr>>16;
	    cmd[3] = addr>>8;
	    cmd[4] = addr;
	    memcpy(&cmd[5], data, wcnt);
	    nfc_i2c_writebytes(I2CtoSPIFlash_SLAVE_ADDR, sizeof(cmd), cmd);
	    kfree(cmd);	
	}
	else
	{
		my_pr_err("[nfc-control]: *cmd malloc fail");
	}
    }
#endif
    else
    {
		my_pr_err("[nfc-control]: uknown flash type %d", flash_type);
    }

    SerialFlash_WriteDisable();

}

static void SerialFlash_ReadData(u32 addr, u32 rsize, u8 *data)
{
    u8 cmd1[5] = {SERIALFLASH_CMD_DATA_WRITE, 0x03, 0x00, 0x00, 0x00};
    u8 cmd2[1] = {SERIALFLASH_CMD_DATA_READ};
    u8 cmd3[1] = {SERIALFLASH_CMD_DATA_END};

    cmd1[2] = addr >> 16;
    cmd1[3] = addr >> 8;
    cmd1[4] = addr;
    nfc_i2c_writebytes( I2CtoSPIFlash_SLAVE_ADDR, sizeof(cmd1), cmd1);

	nfc_i2c_readbytes( I2CtoSPIFlash_SLAVE_ADDR, 1, rsize, cmd2, data);
	
    nfc_i2c_writebytes(I2CtoSPIFlash_SLAVE_ADDR, sizeof(cmd3), cmd3);
}


static u8 SerialFlash_UpgradeA3(u8 *bufptr, u32 len)
{	
    u8 id[3] = {0x00, 0x00, 0x00};
    u8 flash_type = FLASH_UNKNOWN;
    u8 rstatus = 0;
    u8 error = 0;
    u8 waitcnt = 0;
    u8 wstatus = 0;
    u8 *rdata = NULL;
    u32 offset = 0;
	int i;

	FUNC_START();
	
    rdata = kmalloc(4096, GFP_KERNEL);
    if(rdata == NULL)
    {
        my_pr_err("%s Malloc Failed", __func__);
        error = 1;
        goto LAST_END;
	
    }

    // Init
    rstatus = SerialFlash_DisableWDT();
    if (rstatus == 0)
    {
        my_pr_err("%s Disable WDT Failed", __func__);
        error = 1;
        goto LAST_END;
    }
    my_pr_debug("%s Disable WDT Success", __func__);

	rstatus = SerialFlash_Chg_Flash_I2C_Addrs();
    if (rstatus == 0)
    {
        my_pr_err("%s FAIL: SerialFlash_Chg_Flash_I2C_Addrs", __FUNCTION__);
        error = 1;
        goto LAST_END;
    }
    my_pr_debug("%s Success: SerialFlash_Chg_Flash_I2C_Addrs", __FUNCTION__);

    SerialFlash_EntryIspMode();

    SerialFlash_ReadChipID(id);
    my_pr_debug( "%s ID 0x%02X 0x%02X 0x%02X", __FUNCTION__, id[0], id[1], id[2]);

    if (id[0] == 0xBF && id[1] == 0x25 && id[2] == 0x02)
    {
        flash_type = FLASH_SST;
        my_pr_debug("%s SST Flash", __FUNCTION__);
    }
    else if (id[0] == 0xC2 && id[1] == 0x20 && id[2] == 0x11)
    {
        flash_type = FLASH_MX;
        my_pr_debug("%s MXIC Flash", __FUNCTION__);
    }

	rstatus = SerialFlash_ReadStatus();
    my_pr_debug("%s Flash Status = 0x%02X", __FUNCTION__, rstatus);

    SerialFlash_WriteStatus(flash_type, 0x00);
    
    // Erase
    error = 1;
    SerialFlash_ChipErase();
    //wait for Erase done
    for (waitcnt = 0; waitcnt<1000; waitcnt++)
    {
        udelay(50);
        wstatus = SerialFlash_ReadStatus();
        if (wstatus == 0x00)
        {
            error = 0;
            break;
        }
    }
    if (error)
    {
        my_pr_err("%s Erase Timeout", __FUNCTION__);
        goto LAST_END;
    }
    my_pr_debug("%s Erase Done %d", __FUNCTION__, waitcnt);

    // Write
    my_pr_debug("%s FLASH_START: %ul", __FUNCTION__, FLASH_START);
    for (offset=FLASH_START; offset<FLASH_END; offset+=256)
    {
        if( ( offset % 0x1000 ) == 0 )
        {
            my_pr_debug("%s Write Address = 0x%04X from 0x%04X", __FUNCTION__, offset, (unsigned int)(bufptr+offset));
        }

	//printk(KERN_DEBUG "data 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X", rdata[0], rdata[1],rdata[2],rdata[3],rdata[4],rdata[5],rdata[6],rdata[7]);
        SerialFlash_WriteData(FLASH_SST, offset, 256, bufptr+offset);
        error = 1;
        for (waitcnt = 0; waitcnt<10000; waitcnt++)
        {
	    	udelay(100);
            wstatus = SerialFlash_ReadStatus();
            if( wstatus == 0x00 )
            {
                error = 0;
                break;
            }
        }
        if( error )
        {
            my_pr_err("%s Write Timeout", __func__);
            goto LAST_END;
        }
    }

	//SerialFlash_ReadData( 0x00000000, 255, rdata);
	//SerialFlash_ReadData( 0x00010000, 128, rdata);
#if 0
    // Verify
    for (offset=FLASH_START; offset<FLASH_END; offset+=4096)
    {
		int i;
        if( ( (offset) % 0x1000 ) == 0 )
        {
            my_pr_info("%s Read Address = 0x%08X", __FUNCTION__, offset);
        }
        SerialFlash_ReadData(offset, 4096, rdata);
        udelay(1);
        error = 0;
        for (i = 0; i<4096; i++)
        {
            if( rdata[i] != bufptr[offset+i] )
            {
                error = 1;
                break;
            }
        }

        if (error)
        {
            my_pr_err("%s Verify Error at addr:0x%X", __FUNCTION__, offset+i );
            my_pr_err("%s: WData = 0x%02X", __FUNCTION__, bufptr[offset+i] );
            my_pr_err("%s RData = 0x%02X", __FUNCTION__, rdata[i] );

            goto LAST_END;
        }
    }

    my_pr_info( "%s Verify OK", __FUNCTION__);
#endif

#if 1
    // Verify
    
    for ( offset = FLASH_START; offset < FLASH_END; offset += 128)
    {
		
        //if( ( (offset) % 0x0010 ) == 0 )
        //{
        //    my_pr_info("%s Read Address = 0x%08X", __FUNCTION__, offset);
        //}

		SerialFlash_ReadData( offset, 128, rdata);

		udelay(1);
        error = 0;
        for ( i = 0; i < 128; i++)
        {
            if( rdata[i] != bufptr[offset+i] )
            {
                error = 1;
                break;
            }
        }

        if (error)
        {
            my_pr_err("%s Verify Error at addr:0x%X", __FUNCTION__, offset+i );
            my_pr_err("%s: WData = 0x%02X", __FUNCTION__, bufptr[offset+i] );
            my_pr_err("%s RData = 0x%02X", __FUNCTION__, rdata[i] );

            goto LAST_END;
        }
    }
	

    my_pr_debug( "%s Verify OK", __FUNCTION__);
#endif

    


LAST_END:
    if(rdata)
    {
		kfree(rdata);
    }
    SerialFlash_ExitIspMode();

	FUNC_END();
    return error;
}

typedef struct _msr3110_isp_info
{
	unsigned char isp_addrs;
	unsigned char reg_pos_1;
	unsigned char reg_pos_2;
	unsigned char write_val;
	unsigned char reg_val;
	
} msr3110_isp_info;

typedef enum 
{
  NFC_IRQ_STS_INIT 		= 0x00,
  NFC_IRQ_STS_SUCCESS 	= 0x01,
  NFC_IRQ_STS_FAIL		= 0x02,

  NFC_IRQ_STS_WAIT 		= 0x03,
  NFC_IRQ_STS_RAISE		= 0x04,
  NFC_IRQ_STS_ABORT		= 0x05
} NFC_IRQ_STS; 
static long msr3110_dev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long retVal = 0;
	
	msr3110fw_upgrade_info *fwInfo = NULL;
	u8 *fwBuf = NULL;
	int pinVal = 0;
	int irq_interrupt = 0;

	msr3110_isp_info *ispInfo = NULL;
	unsigned char isp_addrs = 0xFF;
	unsigned char pos_1 = 0xFF;
	unsigned char pos_2 = 0xFF;
	unsigned char write_val = 0xFF;
	unsigned char regVal = 0xFF;
	unsigned char write_buf[4];
	
	FUNC_START();
	
	switch( cmd)
	{
		case MSR3110_IOCTL_SET_VEN:
			my_pr_debug( "%s MSR3110_IOCTL_SET_VEN", __func__);
			if( copy_from_user( &pinVal, ( int __user *)arg, sizeof( pinVal)))
			{
				my_pr_err( "%s MSR3110_IOCTL_SET_VEN FAIL: copy_from_user", __func__);
				retVal = -EFAULT;				
			}

			my_pr_debug( "%s MSR3110_IOCTL_SET_VEN pinVal: %d", __func__, pinVal);

			switch( pinVal)
			{
				case 0:
					mt_set_gpio_out(GPIO_NFC_VENB_PIN, GPIO_OUT_ZERO);
					retVal = 0;
					goto end;
					break;
				case 1:
					mt_set_gpio_out(GPIO_NFC_VENB_PIN, GPIO_OUT_ONE);
					retVal = 0;
					goto end;
					break;
				default:
					my_pr_err( "%s MSR3110_IOCTL_SET_VEN wrong pinVal", __func__);
					retVal = -EFAULT;
					goto end;
					break;					
			}
			
			break;
			
		case MSR3110_IOCTL_SET_RST:
			my_pr_debug( "%s MSR3110_IOCTL_SET_RST", __func__);
			if( copy_from_user( &pinVal, ( int __user *)arg, sizeof( pinVal)))
			{
				my_pr_err( "%s MSR3110_IOCTL_SET_RST FAIL: copy_from_user", __func__);
				retVal = -EFAULT;				
			}

			my_pr_debug( "%s MSR3110_IOCTL_SET_RST pinVal: %d", __func__, pinVal);

			switch( pinVal)
			{
				case 0:
					mt_set_gpio_out(GPIO_NFC_RST_PIN, GPIO_OUT_ZERO);
					retVal = 0;
					goto end;
					break;
				case 1:
					mt_set_gpio_out(GPIO_NFC_RST_PIN, GPIO_OUT_ONE);
					retVal = 0;
					goto end;
					break;
				default:
					my_pr_err( "%s MSR3110_IOCTL_SET_RST wrong pinVal", __func__);
					retVal = -EFAULT;
					goto end;
					break;					
			}
			
			break;
			
		case MSR3110_IOCTL_FW_UPGRADE:
			my_pr_debug( "%s MSR3110_IOCTL_FW_UPGRADE", __func__);
			fwInfo = ( msr3110fw_upgrade_info *)arg;
			fwBuf = kmalloc( fwInfo->FwBufLen, GFP_KERNEL);
			if( fwBuf == NULL)
			{
				my_pr_err( "%s MSR3110_IOCTL_FW_UPGRADE kmalloc fail, FwBufLen: %lu \n", __func__, fwInfo->FwBufLen);
				retVal = -1;
				goto end;
			}
			
			if( copy_from_user( fwBuf, fwInfo->FwBuf, fwInfo->FwBufLen))
			{
				my_pr_err( "%s MSR3110_IOCTL_FW_UPGRADE copy from user fail \n", __func__);
				retVal = -1;
				goto end;								
			}

			my_pr_debug( "%s MSR3110_IOCTL_FW_UPGRADE FwBuf: 0x %p, size: %lu (0x %lx) \n", __func__, fwInfo->FwBuf, fwInfo->FwBufLen, fwInfo->FwBufLen);
			retVal = SerialFlash_UpgradeA3( fwBuf, fwInfo->FwBufLen);
			my_pr_debug( "%s MSR3110_IOCTL_FW_UPGRADE retVal: %ld \n", __func__, retVal);
			
			if( fwBuf)
			{
				my_pr_debug( "%s MSR3110_IOCTL_FW_UPGRADE kfree \n", __func__);
				kfree( fwBuf);
			}

			if( retVal < 0)
			{
				my_pr_err( "%s MSR3110_IOCTL_FW_UPGRADE fail \n", __func__);
				goto end;
				
			}
			
			break;

		case MSR3110_IOCTL_IRQ:
			my_pr_debug( "%s MSR3110_IOCTL_IRQ ", __func__);

			my_pr_debug( "%s 3. mt_get_gpio_mode(%d): %d", __func__, GPIO_IRQ_NFC_PIN_M_EINT, mt_get_gpio_mode(GPIO_IRQ_NFC_PIN));
			my_pr_debug( "%s 3. mt_get_gpio_dir(%d): %d", __func__, GPIO_DIR_IN, mt_get_gpio_dir(GPIO_IRQ_NFC_PIN));
			my_pr_debug( "%s 3. mt_get_gpio_pull_enable(%d): %d", __func__, GPIO_PULL_ENABLE, mt_get_gpio_pull_enable(GPIO_IRQ_NFC_PIN));
			my_pr_debug( "%s 3. mt_get_gpio_pull_select(%d): %d", __func__, GPIO_PULL_UP, mt_get_gpio_pull_select(GPIO_IRQ_NFC_PIN));		

			mutex_lock( &g_msr3110_dev->mutex_ioctl_irq);
			
			g_msr3110_dev->irq_flag = NFC_IRQ_STS_WAIT;			
			mt65xx_eint_unmask( CUST_EINT_EXT_IRQ_NFC_NUM);
			//irq_interrupt = wait_event_interruptible( g_msr3110_dev->irq_wait_queue, ( mt_get_gpio_in( GPIO_NFC_EINT_PIN) == 0));

			mutex_unlock( &g_msr3110_dev->mutex_ioctl_irq);
			
			irq_interrupt = wait_event_interruptible( g_msr3110_dev->irq_wait_queue, ( g_msr3110_dev->irq_flag  != NFC_IRQ_STS_WAIT));
			
			my_pr_debug( "%s MSR3110_IOCTL_IRQ irq_interrupt: %d", __func__, irq_interrupt); 
            
			retVal = g_msr3110_dev->irq_flag;
			
			break;

		case MSR3110_IOCTL_IRQ_ABORT:   //Aborting the blocking-wait of IRQ
			my_pr_debug( "%s MSR3110_IOCTL_IRQ_ABORT ", __func__); 

			mutex_lock( &g_msr3110_dev->mutex_ioctl_irq);

			retVal = g_msr3110_dev->irq_flag;

			if( retVal != NFC_IRQ_STS_WAIT)
			{
				my_pr_err( "%s IRQ_ABORT canceled. local irq_flag: %d", __func__, retVal);
				my_pr_err( "%s IRQ_ABORT canceled. global irq_flag: %d", __func__, g_msr3110_dev->irq_flag);
				goto end;								
			}

			g_msr3110_dev->irq_flag = NFC_IRQ_STS_ABORT;
			wake_up( &g_msr3110_dev->irq_wait_queue);
			
			my_pr_debug( "%s 4. mt_get_gpio_mode(%d): %d", __func__, GPIO_IRQ_NFC_PIN_M_EINT, mt_get_gpio_mode(GPIO_IRQ_NFC_PIN));
			my_pr_debug( "%s 4. mt_get_gpio_dir(%d): %d", __func__, GPIO_DIR_IN, mt_get_gpio_dir(GPIO_IRQ_NFC_PIN));
			my_pr_debug( "%s 4. mt_get_gpio_pull_enable(%d): %d", __func__, GPIO_PULL_ENABLE, mt_get_gpio_pull_enable(GPIO_IRQ_NFC_PIN));
			my_pr_debug( "%s 4. mt_get_gpio_pull_select(%d): %d", __func__, GPIO_PULL_UP, mt_get_gpio_pull_select(GPIO_IRQ_NFC_PIN));
			
            mutex_unlock( &g_msr3110_dev->mutex_ioctl_irq);
            
			retVal = NFC_IRQ_STS_SUCCESS;

			break;
		case MSR3110_IOCTL_IRQ_REG:   // IRQ Registration
			my_pr_debug( "%s MSR3110_IOCTL_IRQ_REG ", __func__); 
			
			mt65xx_eint_set_sens(CUST_EINT_EXT_IRQ_NFC_NUM, CUST_EINT_EXT_IRQ_NFC_SENSITIVE);
			mt65xx_eint_set_hw_debounce(CUST_EINT_EXT_IRQ_NFC_NUM, CUST_EINT_EXT_IRQ_NFC_DEBOUNCE_CN);
			mt65xx_eint_registration(CUST_EINT_EXT_IRQ_NFC_NUM, CUST_EINT_EXT_IRQ_NFC_DEBOUNCE_EN, CUST_EINT_EXT_IRQ_NFC_POLARITY, msr3110_dev_irq_handler, 0);
			mt65xx_eint_mask( CUST_EINT_EXT_IRQ_NFC_NUM);
            
			retVal = NFC_IRQ_STS_SUCCESS;

			break;
		case MSR3110_IOCTL_ISP_READ_REG:
			my_pr_debug( "%s MSR3110_IOCTL_ISP_READ_REG ", __func__); 
			ispInfo = ( msr3110_isp_info *)arg;
			isp_addrs = ispInfo->isp_addrs;
			pos_1 = ispInfo->reg_pos_1;
			pos_2 = ispInfo->reg_pos_2;
			my_pr_debug( "%s pos_1: %02x.", __func__, pos_1);
			my_pr_debug( "%s pos_2: %02x.", __func__, pos_2);
			my_pr_debug( "%s isp_addrs: %02x.", __func__, isp_addrs);

			write_buf[0] = SERIALFLASH_CMD_DATA_WRITE;
			write_buf[1] = pos_1;
			write_buf[2] = pos_2;

			SerialFlash_EnterSerialDebug();
			SerialFlash_EnterSingleStep();
			
			nfc_i2c_readbytes( isp_addrs, 3, 1, write_buf, &regVal);			

			SerialFlash_ExitSingleStep();
			SerialFlash_ExitSerialDebug();

			ispInfo->reg_val = regVal;
			my_pr_debug( "%s isp_addrs: %02x.", __func__, regVal);
			

			break;
		case MSR3110_IOCTL_ISP_WRITE_REG:
			my_pr_debug( "%s MSR3110_IOCTL_ISP_WRITE_REG ", __func__); 
			ispInfo = ( msr3110_isp_info *)arg;
			isp_addrs = ispInfo->isp_addrs;
			pos_1 = ispInfo->reg_pos_1;
			pos_2 = ispInfo->reg_pos_2;
			write_val = ispInfo->write_val;
			my_pr_debug( "%s pos_1: %02x.", __func__, pos_1);
			my_pr_debug( "%s pos_2: %02x.", __func__, pos_2);
			my_pr_debug( "%s isp_addrs: %02x.", __func__, isp_addrs);
			my_pr_debug( "%s write_val: %02x.", __func__, write_val);

			write_buf[0] = SERIALFLASH_CMD_DATA_WRITE;
			write_buf[1] = pos_1;
			write_buf[2] = pos_2;
			write_buf[3] = write_val;

			SerialFlash_EnterSerialDebug();
			SerialFlash_EnterSingleStep();
			
			nfc_i2c_writebytes( isp_addrs, 4, write_buf);	

			SerialFlash_ExitSingleStep();
			SerialFlash_ExitSerialDebug();

			break;
			
		case MSR3110_IOCTL_SET_IRQ_NFC_PIN:
			my_pr_debug( "%s MSR3110_IOCTL_SET_IRQ_NFC_PIN", __func__);
			if( copy_from_user( &pinVal, ( int __user *)arg, sizeof( pinVal)))
			{
				my_pr_err( "%s MSR3110_IOCTL_SET_IRQ_NFC_PIN FAIL: copy_from_user", __func__);
				retVal = -EFAULT;				
			}

			my_pr_debug( "%s MSR3110_IOCTL_SET_IRQ_NFC_PIN pinVal: %d", __func__, pinVal);

			switch( pinVal)
			{
				case 0:
					mt_set_gpio_out(GPIO_IRQ_NFC_PIN, GPIO_OUT_ZERO);
					retVal = 0;
					goto end;
					break;
				case 1:
					mt_set_gpio_out(GPIO_IRQ_NFC_PIN, GPIO_OUT_ONE);
					retVal = 0;
					goto end;
					break;
				default:
					my_pr_err( "%s MSR3110_IOCTL_SET_IRQ_NFC_PIN wrong pinVal", __func__);
					retVal = -EFAULT;
					goto end;
					break;					
			}
			
			break;

		case MSR3110_IOCTL_SET_NFC_EINT_PIN:
			my_pr_debug( "%s MSR3110_IOCTL_SET_NFC_EINT_PIN", __func__);
			if( copy_from_user( &pinVal, ( int __user *)arg, sizeof( pinVal)))
			{
				my_pr_err( "%s MSR3110_IOCTL_SET_NFC_EINT_PIN FAIL: copy_from_user", __func__);
				retVal = -EFAULT;				
			}

			my_pr_debug( "%s MSR3110_IOCTL_SET_NFC_EINT_PIN pinVal: %d", __func__, pinVal);

			switch( pinVal)
			{
				case 0:
					mt_set_gpio_out(GPIO_NFC_EINT_PIN, GPIO_OUT_ZERO);
					retVal = 0;
					goto end;
					break;
				case 1:
					mt_set_gpio_out(GPIO_NFC_EINT_PIN, GPIO_OUT_ONE);
					retVal = 0;
					goto end;
					break;
				default:
					my_pr_err( "%s MSR3110_IOCTL_SET_NFC_EINT_PIN wrong pinVal", __func__);
					retVal = -EFAULT;
					goto end;
					break;					
			}
			
			break;

		
		default:
			my_pr_err( "%s default: UNKNOWN COMMAND", __func__);
			retVal = -1;
			goto end;
			break;
	}

end:

	FUNC_END();
	return retVal;
}




	static const struct file_operations msr3110_dev_fops = {
		.owner	= THIS_MODULE,
		//.llseek	= no_llseek,
		.read	= msr3110_dev_read,
		.write	= msr3110_dev_write,
		.open	= msr3110_dev_open,
		.release = msr3110_dev_release,
		.unlocked_ioctl	= msr3110_dev_ioctl,
	};


	struct miscdevice nfc_dev = {
		.minor = MISC_DYNAMIC_MINOR,
		.name = "msr3110",
		.fops = &msr3110_dev_fops,
	};

void msr3110_dev_irq_handler(void)
{
	FUNC_START();
		
	// disable irq
	my_pr_debug( "%s mt_get_gpio_in( GPIO_IRQ_NFC_PIN): %d", __func__, mt_get_gpio_in( GPIO_IRQ_NFC_PIN));
	
	if( mt_get_gpio_in( GPIO_IRQ_NFC_PIN) == 0)
	{
		//mutex_lock( &g_msr3110_dev->mutex_irq_handler);

		mt65xx_eint_mask( CUST_EINT_EXT_IRQ_NFC_NUM);
		my_pr_debug( "%s g_msr3110_dev->irq_flag: %d", __func__, g_msr3110_dev->irq_flag);
		g_msr3110_dev->irq_flag = NFC_IRQ_STS_RAISE;
		wake_up( &g_msr3110_dev->irq_wait_queue);

		//mutex_unlock( &g_msr3110_dev->mutex_irq_handler);	
	}

	FUNC_END();
}



static int __init msr3110_dev_init(void)
{
	int retVal = 0;

	FUNC_START();

	// global control init
	g_msr3110_dev = kmalloc( sizeof( msr3110_dev), GFP_KERNEL);
	if( g_msr3110_dev == NULL)
	{
		my_pr_err( "%s FAIL kmalloc for g_msr3110_dev", __func__);
		retVal = -1;
		goto end;
	}
	init_waitqueue_head( &g_msr3110_dev->irq_wait_queue);
	mutex_init( &g_msr3110_dev->mutex_irq_handler);
	mutex_init( &g_msr3110_dev->mutex_ioctl_irq);
	g_msr3110_dev->irq_flag = NFC_IRQ_STS_INIT;

	// DMA Related - init start

	I2CDMAWriteBuf = (char *)dma_alloc_coherent(NULL, MAX_BUFFER_SIZE, &I2CDMAWriteBuf_pa, GFP_KERNEL);
	if (I2CDMAWriteBuf == NULL) 
	{
		my_pr_err("%s : failed to allocate dma buffer\n", __func__);
		goto end;
	}
	
	I2CDMAReadBuf = (char *)dma_alloc_coherent(NULL, MAX_BUFFER_SIZE, &I2CDMAReadBuf_pa, GFP_KERNEL);
	if (I2CDMAReadBuf == NULL) 
	{
		my_pr_err("%s : failed to allocate dma buffer\n", __func__);
		goto end;
	}
    
	my_pr_debug( "%s :I2CDMAWriteBuf_pa %d, I2CDMAReadBuf_pa,%d\n", __func__, I2CDMAWriteBuf_pa, I2CDMAReadBuf_pa);

	// DMA Related - init end
	

	// device node register
	retVal = misc_register( &nfc_dev);
	if ( retVal < 0)
	{
		my_pr_err( "%s unable to register device %d", __func__, retVal);
		goto end;
	}

	// i2c bus driver regoster board
	i2c_register_board_info( NFC_I2C_BUSNUM, &nfc_board_info, 1);
	retVal = i2c_add_driver( &nfc_driver);
	if( retVal < 0)
	{
		my_pr_err( "%s unable to register device %d", __func__, retVal);
		goto end;
	}

	// GPIO Initial: IRQ	

#if 1 // 0: ROM mode 
	my_pr_debug( "%s mt_get_gpio_mode(%d): %d", __func__, GPIO_IRQ_NFC_PIN_M_EINT, mt_get_gpio_mode(GPIO_IRQ_NFC_PIN));
	my_pr_debug( "%s mt_get_gpio_dir(%d): %d", __func__, GPIO_DIR_IN, mt_get_gpio_dir(GPIO_IRQ_NFC_PIN));
	my_pr_debug( "%s mt_get_gpio_pull_enable(%d): %d", __func__, GPIO_PULL_ENABLE, mt_get_gpio_pull_enable(GPIO_IRQ_NFC_PIN));
	my_pr_debug( "%s mt_get_gpio_pull_select(%d): %d", __func__, GPIO_PULL_UP, mt_get_gpio_pull_select(GPIO_IRQ_NFC_PIN));
		


	mt_set_gpio_mode( GPIO_IRQ_NFC_PIN, GPIO_IRQ_NFC_PIN_M_EINT);
	mt_set_gpio_dir( GPIO_IRQ_NFC_PIN, GPIO_DIR_IN);
	mt_set_gpio_pull_enable( GPIO_IRQ_NFC_PIN, GPIO_PULL_ENABLE);
	mt_set_gpio_pull_select( GPIO_IRQ_NFC_PIN, GPIO_PULL_UP);

	my_pr_debug( "%s 1. mt_get_gpio_mode(%d): %d", __func__, GPIO_IRQ_NFC_PIN_M_EINT, mt_get_gpio_mode(GPIO_IRQ_NFC_PIN));
	my_pr_debug( "%s 1. mt_get_gpio_dir(%d): %d", __func__, GPIO_DIR_IN, mt_get_gpio_dir(GPIO_IRQ_NFC_PIN));
	my_pr_debug( "%s 1. mt_get_gpio_pull_enable(%d): %d", __func__, GPIO_PULL_ENABLE, mt_get_gpio_pull_enable(GPIO_IRQ_NFC_PIN));
	my_pr_debug( "%s 1. mt_get_gpio_pull_select(%d): %d", __func__, GPIO_PULL_UP, mt_get_gpio_pull_select(GPIO_IRQ_NFC_PIN));
			
	mt65xx_eint_set_sens(CUST_EINT_EXT_IRQ_NFC_NUM, CUST_EINT_EXT_IRQ_NFC_SENSITIVE);
	mt65xx_eint_set_hw_debounce(CUST_EINT_EXT_IRQ_NFC_NUM, CUST_EINT_EXT_IRQ_NFC_DEBOUNCE_CN);
	mt65xx_eint_registration(CUST_EINT_EXT_IRQ_NFC_NUM, CUST_EINT_EXT_IRQ_NFC_DEBOUNCE_EN, CUST_EINT_EXT_IRQ_NFC_POLARITY, msr3110_dev_irq_handler, 0);
	mt65xx_eint_mask( CUST_EINT_EXT_IRQ_NFC_NUM);

	my_pr_debug( "%s 2. mt_get_gpio_mode(%d): %d", __func__, GPIO_IRQ_NFC_PIN_M_EINT, mt_get_gpio_mode(GPIO_IRQ_NFC_PIN));
	my_pr_debug( "%s 2. mt_get_gpio_dir(%d): %d", __func__, GPIO_DIR_IN, mt_get_gpio_dir(GPIO_IRQ_NFC_PIN));
	my_pr_debug( "%s 2. mt_get_gpio_pull_enable(%d): %d", __func__, GPIO_PULL_ENABLE, mt_get_gpio_pull_enable(GPIO_IRQ_NFC_PIN));
	my_pr_debug( "%s 2. mt_get_gpio_pull_select(%d): %d", __func__, GPIO_PULL_UP, mt_get_gpio_pull_select(GPIO_IRQ_NFC_PIN));
	

	
#else 
	mt_set_gpio_mode( GPIO_IRQ_NFC_PIN, GPIO_IRQ_NFC_PIN_M_GPIO);
	mt_set_gpio_dir( GPIO_IRQ_NFC_PIN, GPIO_DIR_OUT);

	//mt_set_gpio_out( GPIO_IRQ_NFC_PIN, GPIO_OUT_ZERO);

	mt_set_gpio_mode( GPIO_NFC_EINT_PIN, GPIO_NFC_EINT_PIN_M_GPIO);
	mt_set_gpio_dir( GPIO_NFC_EINT_PIN, GPIO_DIR_OUT);
#endif

	// GPIO Initial: VEN	
	mt_set_gpio_mode( GPIO_NFC_VENB_PIN, GPIO_NFC_VENB_PIN_M_GPIO);
	mt_set_gpio_dir( GPIO_NFC_VENB_PIN, GPIO_DIR_OUT);

	// GPIO Initial: RST 	
	mt_set_gpio_mode( GPIO_NFC_RST_PIN, GPIO_NFC_RST_PIN_M_GPIO);
	mt_set_gpio_dir( GPIO_NFC_RST_PIN, GPIO_DIR_OUT);

	// Init MSR3110 by VEN HIGH-then-LOW
	mt_set_gpio_out( GPIO_NFC_VENB_PIN, GPIO_OUT_ONE);
	mdelay( 200);
	
	mt_set_gpio_out( GPIO_NFC_RST_PIN, GPIO_OUT_ONE);
	mdelay( 50);	
	mt_set_gpio_out( GPIO_NFC_RST_PIN, GPIO_OUT_ZERO);
	mt_set_gpio_out( GPIO_NFC_VENB_PIN, GPIO_OUT_ZERO);
	
	

end:    

	FUNC_END();
	return retVal;
}


static void __exit msr3110_dev_exit(void)
{
    FUNC_START();

	//return 0;
	
	my_pr_debug("msr3110_dev_exit\n");
	//i2c_del_driver(&pn544_driver);
	misc_deregister(&nfc_dev);
	//gpio_free( GPIO_NFC_EINT_PIN);
	gpio_free( GPIO_NFC_VENB_PIN);
	gpio_free( GPIO_NFC_RST_PIN);

	FUNC_END();
}

module_init( msr3110_dev_init);
module_exit( msr3110_dev_exit);

MODULE_AUTHOR("Alan Huang");
MODULE_DESCRIPTION("NFC MSR3110 driver");
MODULE_LICENSE("GPL");

