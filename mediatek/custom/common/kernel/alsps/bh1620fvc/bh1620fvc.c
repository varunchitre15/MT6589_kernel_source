#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>

#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#include <linux/hwmsen_helper.h>

#if (defined(MT6575) || defined(MT6577))
#include <mach/mt_clock_manager.h>
#endif

#ifdef MT6589
#include <mach/mt_clkmgr.h>
#endif

#include <asm/io.h>
#include <cust_eint.h>
#include <cust_alsps.h>
#include "bh1620fvc.h"

#if (defined(MT6575) || defined(MT6577))
#include <mach/mt_devs.h>
#endif

#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>

#define POWER_NONE_MACRO MT65XX_POWER_NONE

//extern int IMM_GetOneChannelValue(int dwChannel, int data[4]);
extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int* rawdata);

extern void mt65xx_eint_unmask(unsigned int line);
extern void mt65xx_eint_mask(unsigned int line);
extern void mt65xx_eint_set_polarity(kal_uint8 eintno, kal_bool ACT_Polarity);
extern void mt65xx_eint_set_hw_debounce(kal_uint8 eintno, kal_uint32 ms);
extern kal_uint32 mt65xx_eint_set_sens(kal_uint8 eintno, kal_bool sens);
extern void mt65xx_eint_registration(kal_uint8 eintno, kal_bool Dbounce_En,
                                     kal_bool ACT_Polarity, void (EINT_FUNC_PTR)(void),
                                     kal_bool auto_umask);

/******************************************************************************
 * configuration
*******************************************************************************/
#define BH1620FVC_NO_SUPPORT_PS

#define GET_ALS_DATA_FROM_AUXADC
//#define TEST_LOG 

/*----------------------------------------------------------------------------*/
#define BH1620FVC_DEV_NAME     "BH1620FVC"
/*----------------------------------------------------------------------------*/
#define APS_TAG                  "[ALS/PS] "
#define APS_FUN(f)               printk(KERN_INFO APS_TAG"%s\n", __FUNCTION__)
#define APS_ERR(fmt, args...)    printk(KERN_ERR  APS_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define APS_LOG(fmt, args...)    printk(KERN_INFO APS_TAG fmt, ##args)
#define APS_DBG(fmt, args...)    printk(KERN_INFO fmt, ##args)                 

#ifdef GET_ALS_DATA_FROM_AUXADC

#define FACTOR_1_6P8K     1    //6.8K
#define FACTOR_10_68K   10   //6.8*10 = 68K
#define AUXADAC_0   0
#define AUXADAC_1   1
#define AUXADAC_2   2
#define AUXADAC_3   3
#define AUXADAC_4   4

#define BH1620FVC_MAX_LUX          6450                 //max lux
#define BH1620FVC_LUX_FACTOR    FACTOR_1_6P8K  
//#define ADC_MAX_RANGE                 2500                 //mV
//MT8389
#define ADC_MAX_RANGE                 1470                 //mV
#define ALS_OUTPUT_AUXADC_PIN  AUXADAC_1     // 4 for experiment

#define BH1620FVC_HIGH_GAIN  0
#define BH1620FVC_MIDDLE_GAIN 1
#define BH1620FVC_LOW_GAIN 2
#define BH1620FVC_SHUTDOWN 3

int auxadc_pwr_enable = 0;
#endif

/******************************************************************************
 * extern functions
*******************************************************************************/
#ifdef MT6516
extern void MT6516_EINTIRQUnmask(unsigned int line);
extern void MT6516_EINTIRQMask(unsigned int line);
extern void MT6516_EINT_Set_Polarity(kal_uint8 eintno, kal_bool ACT_Polarity);
extern void MT6516_EINT_Set_HW_Debounce(kal_uint8 eintno, kal_uint32 ms);
extern kal_uint32 MT6516_EINT_Set_Sensitivity(kal_uint8 eintno, kal_bool sens);
extern void MT6516_EINT_Registration(kal_uint8 eintno, kal_bool Dbounce_En,
                                     kal_bool ACT_Polarity, void (EINT_FUNC_PTR)(void),
                                     kal_bool auto_umask);
#endif
/*----------------------------------------------------------------------------*/
#define mt6516_I2C_DATA_PORT        ((base) + 0x0000)
#define mt6516_I2C_SLAVE_ADDR       ((base) + 0x0004)
#define mt6516_I2C_INTR_MASK        ((base) + 0x0008)
#define mt6516_I2C_INTR_STAT        ((base) + 0x000c)
#define mt6516_I2C_CONTROL          ((base) + 0x0010)
#define mt6516_I2C_TRANSFER_LEN     ((base) + 0x0014)
#define mt6516_I2C_TRANSAC_LEN      ((base) + 0x0018)
#define mt6516_I2C_DELAY_LEN        ((base) + 0x001c)
#define mt6516_I2C_TIMING           ((base) + 0x0020)
#define mt6516_I2C_START            ((base) + 0x0024)
#define mt6516_I2C_FIFO_STAT        ((base) + 0x0030)
#define mt6516_I2C_FIFO_THRESH      ((base) + 0x0034)
#define mt6516_I2C_FIFO_ADDR_CLR    ((base) + 0x0038)
#define mt6516_I2C_IO_CONFIG        ((base) + 0x0040)
#define mt6516_I2C_DEBUG            ((base) + 0x0044)
#define mt6516_I2C_HS               ((base) + 0x0048)
#define mt6516_I2C_DEBUGSTAT        ((base) + 0x0064)
#define mt6516_I2C_DEBUGCTRL        ((base) + 0x0068)
/*----------------------------------------------------------------------------*/
static struct i2c_client *bh1620fvc_i2c_client = NULL;
/*----------------------------------------------------------------------------*/
static const struct i2c_device_id bh1620fvc_i2c_id[] = {{BH1620FVC_DEV_NAME,0},{}};
static struct i2c_board_info __initdata i2c_bh1620fvc={ I2C_BOARD_INFO("BH1620FVC", (0x20>>1))};
/*the adapter id & i2c address will be available in customization*/
//static unsigned short bh1620fvc_force[] = {0x00, 0x00, I2C_CLIENT_END, I2C_CLIENT_END};
//static const unsigned short *const bh1620fvc_forces[] = { bh1620fvc_force, NULL };
//static struct i2c_client_address_data bh1620fvc_addr_data = { .forces = bh1620fvc_forces,};
/*----------------------------------------------------------------------------*/
static int bh1620fvc_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id); 
static int bh1620fvc_i2c_remove(struct i2c_client *client);
//static int bh1620fvc_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);
/*----------------------------------------------------------------------------*/
static int bh1620fvc_i2c_suspend(struct i2c_client *client, pm_message_t msg);
static int bh1620fvc_i2c_resume(struct i2c_client *client);

static struct bh1620fvc_priv *g_bh1620fvc_ptr = NULL;
/*----------------------------------------------------------------------------*/
typedef enum {
    BH_TRC_ALS_DATA= 0x0001,
    BH_TRC_PS_DATA = 0x0002,
    BH_TRC_EINT    = 0x0004,
    BH_TRC_IOCTL   = 0x0008,
    BH_TRC_I2C     = 0x0010,
    BH_TRC_CVT_ALS = 0x0020,
    BH_TRC_CVT_PS  = 0x0040,
    BH_TRC_DEBUG   = 0x8000,
} BH_TRC;
/*----------------------------------------------------------------------------*/
typedef enum {
    BH_BIT_ALS    = 1,
    BH_BIT_PS     = 2,
} BH_BIT;
/*----------------------------------------------------------------------------*/
struct bh1620fvc_i2c_addr {    /*define a series of i2c slave address*/
    u8  rar;           /*Alert Response Address*/
    u8  init;           /*device initialization */
    u8  als_cmd;   /*ALS command*/
    u8  als_dat1;   /*ALS MSB*/
    u8  als_dat0;   /*ALS LSB*/
    u8  als_intr;    /*ALS INT*/    
    u8  ps_cmd;    /*PS command*/
    u8  ps_dat;      /*PS data*/
    u8  ps_thd;     /*PS INT threshold*/
};
/*----------------------------------------------------------------------------*/
struct bh1620fvc_priv {
    struct alsps_hw  *hw;
    struct i2c_client *client;
    struct delayed_work  eint_work;

    /*i2c address group*/
    struct bh1620fvc_i2c_addr  addr;
    
    /*misc*/
    atomic_t    trace;
    atomic_t    i2c_retry;
    atomic_t    als_suspend;
    atomic_t    als_debounce;   /*debounce time after enabling als*/
    atomic_t    als_deb_on;     /*indicates if the debounce is on*/
    atomic_t    als_deb_end;    /*the jiffies representing the end of debounce*/
    atomic_t    ps_mask;        /*mask ps: always return far away*/
    atomic_t    ps_debounce;    /*debounce time after enabling ps*/
    atomic_t    ps_deb_on;      /*indicates if the debounce is on*/
    atomic_t    ps_deb_end;     /*the jiffies representing the end of debounce*/
    atomic_t    ps_suspend;


    /*data*/
    u16         als;
    u8          ps;
    u8          _align;
    u16         als_level_num;
    u16         als_value_num;
    u32         als_level[C_CUST_ALS_LEVEL-1];
    u32         als_value[C_CUST_ALS_LEVEL];

    atomic_t    als_cmd_val;   /*the cmd value can't be read, stored in ram*/
    atomic_t    als_intr_val;    /*the intr  value can't be read, stored in ram*/    
    atomic_t    ps_cmd_val;    /*the cmd value can't be read, stored in ram*/
    atomic_t    ps_thd_val;     /*the cmd value can't be read, stored in ram*/
    ulong       enable;              /*enable mask*/
    ulong       pending_intr;     /*pending interrupt*/

    /*early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
    struct early_suspend    early_drv;
#endif     
};
/*----------------------------------------------------------------------------*/
static struct i2c_driver bh1620fvc_i2c_driver = {	
	.probe      = bh1620fvc_i2c_probe,
	.remove     = bh1620fvc_i2c_remove,
//	.detect     = bh1620fvc_i2c_detect,
	.suspend    = bh1620fvc_i2c_suspend,
	.resume     = bh1620fvc_i2c_resume,
	.id_table   = bh1620fvc_i2c_id,
//	.address_data = &bh1620fvc_addr_data,
	.driver = {
//		.owner          = THIS_MODULE,
		.name           = BH1620FVC_DEV_NAME,
	},
};

static struct bh1620fvc_priv *bh1620fvc_obj = NULL;
static struct platform_driver bh1620fvc_alsps_driver;
static int bh1620fvc_get_ps_value(struct bh1620fvc_priv *obj, u16 ps);
static int bh1620fvc_get_als_value(struct bh1620fvc_priv *obj, u16 als);
static int bh1620fvc_read_als(struct i2c_client *client, u16 *data);
static int bh1620fvc_read_ps(struct i2c_client *client, u8 *data);
/*----------------------------------------------------------------------------*/
int bh1620fvc_get_addr(struct alsps_hw *hw, struct bh1620fvc_i2c_addr *addr)
{
	if(!hw || !addr)
	{
		return -EFAULT;
	}
	addr->als_cmd   = ALS_CMD;         
	addr->als_dat1  = ALS_DT1;
	addr->als_dat0  = ALS_DT2;
	addr->als_intr   = ALS_INT;         
	addr->ps_cmd    = PS_CMD;
	addr->ps_thd    = PS_THDL;
	addr->ps_dat    = PS_DT;
	return 0;
}
/*----------------------------------------------------------------------------*/
int bh1620fvc_get_timing(void)
{
return 200;
/*
	u32 base = I2C2_BASE; 
	return (__raw_readw(mt6516_I2C_HS) << 16) | (__raw_readw(mt6516_I2C_TIMING));
*/
}
/*----------------------------------------------------------------------------*/
/*
int bh1620fvc_config_timing(int sample_div, int step_div)
{
	u32 base = I2C2_BASE; 
	unsigned long tmp;

	tmp  = __raw_readw(mt6516_I2C_TIMING) & ~((0x7 << 8) | (0x1f << 0));
	tmp  = (sample_div & 0x7) << 8 | (step_div & 0x1f) << 0 | tmp;

	return (__raw_readw(mt6516_I2C_HS) << 16) | (tmp);
}
*/
/*----------------------------------------------------------------------------*/
int bh1620fvc_master_recv(struct i2c_client *client, u16 addr, u8 *buf ,int count)
{
	struct bh1620fvc_priv *obj = i2c_get_clientdata(client);        
	//struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg;
	int ret = 0, retry = 0;
	int trc = atomic_read(&obj->trace);
	int max_try = atomic_read(&obj->i2c_retry);

	#ifdef GET_ALS_DATA_FROM_AUXADC
         return 1;
       #endif

	while(retry++ < max_try)
	{
		ret = hwmsen_read_block(client, addr, buf, count);
		if(ret == 0)
            break;
		udelay(100);
	}

	if(unlikely(trc))
	{
		if(trc & BH_TRC_I2C)
		{
			APS_LOG("(recv) %x %d %d %p [%02X]\n", msg.addr, msg.flags, msg.len, msg.buf, msg.buf[0]);    
		}

		if((retry != 1) && (trc & BH_TRC_DEBUG))
		{
			APS_LOG("(recv) %d/%d\n", retry-1, max_try); 

		}
	}

	/* If everything went ok (i.e. 1 msg transmitted), return #bytes
	transmitted, else error code. */
	return (ret == 0) ? count : ret;
}
/*----------------------------------------------------------------------------*/
int bh1620fvc_master_send(struct i2c_client *client, u16 addr, u8 *buf ,int count)
{
	int ret = 0, retry = 0;
	struct bh1620fvc_priv *obj = i2c_get_clientdata(client);        
	//struct i2c_adapter *adap=client->adapter;
	struct i2c_msg msg;
	int trc = atomic_read(&obj->trace);
	int max_try = atomic_read(&obj->i2c_retry);

      #ifdef GET_ALS_DATA_FROM_AUXADC
         return 1;
      #endif

	while(retry++ < max_try)
	{
		ret = hwmsen_write_block(client, addr, buf, count);
		if (ret == 0)
		    break;
		udelay(100);
	}

	if(unlikely(trc))
	{
		if(trc & BH_TRC_I2C)
		{
			APS_LOG("(send) %x %d %d %p [%02X]\n", msg.addr, msg.flags, msg.len, msg.buf, msg.buf[0]);    
		}

		if((retry != 1) && (trc & BH_TRC_DEBUG))
		{
			APS_LOG("(send) %d/%d\n", retry-1, max_try);
		}
	}
	/* If everything went ok (i.e. 1 msg transmitted), return #bytes
	transmitted, else error code. */
	return (ret == 0) ? count : ret;
}

#ifdef GET_ALS_DATA_FROM_AUXADC
int bh1620fvc_als_get_data_from_adc(void)
{
 //======================================================
 // Bh1620fvc spec => H Gain (GC2:0  GC1:1) => Viout = 0.57*10^(-6)* lux*R1   
 // Bh1620fvc spec => M Gain (GC2:1  GC1:0) => Viout = 0.057*10^(-6)* lux*R1   
 // Bh1620fvc spec => L Gain  (GC2:1  GC1:1) => Viout = 0.0057*10^(-6)* lux*R1   

 // H Gain if max 645     lux => 6.8Kohm
 // M Gain if max 6450   lux => 6.8Kohm
 // L Gain if  max 64500 lux => 6.8Kohm

 // Max Auxadc range = 2.5V/2.8V, we assume 0~2.3V is linear
 // Max lux is 6450 lux if R1=6.8K,  factor=1 
 // 2.5V => 6450/factor     if factor =1  mean 6.8K,  if factor =10 mean 68K    
 // MT8389 
 // Max Auxadc range = 1.47V/1.5V
 // Max lux is 6450 lux if R1=3.9K,  factor=1 
 // 1.47V => 6450/factor     if factor =1  mean 3.9K,  if factor =10 mean 39K    
 
 //======================================================

	unsigned int channel[5] = {0,0,0,0,0};
	int als_voltage=0;
	int als_to_lux=0;
	int dwChannel=ALS_OUTPUT_AUXADC_PIN;  //use channel 4 for ALS AUXADC
	int data[4]={0,0,0,0};
	int res=0;

//=======================================================
	if ( auxadc_pwr_enable == 0 )
    {
    	if ( enable_clock(MT_CG_PERI1_AUXADC,"AUXADC") == FALSE )
        {
			//APS_ERR("hwEnableClock AUXADC failed.\n");
        }
	    else
	    {
			//APS_ERR("hwEnableClock AUXADC OK.\n");
			auxadc_pwr_enable = 1;
	    }	   
    }
	//res = IMM_GetOneChannelValue(dwChannel,data);
    res = IMM_GetOneChannelValue(dwChannel,data,NULL);  //for ICS
	
    if ( res < 0 )
    { 
		channel[dwChannel] = data[0]*1000+data[1]*10;  			   
    }
    else
    {
		channel[dwChannel] = data[0]*1000+data[1]*10;
    }
//=======================================================
  
	als_voltage = channel[dwChannel];     //rawdata to real voltage to mV

	if ( als_voltage < 0 )  als_voltage = 0;  //protection

	als_to_lux = (als_voltage*BH1620FVC_MAX_LUX/(BH1620FVC_LUX_FACTOR*ADC_MAX_RANGE));

    #ifdef  TEST_LOG 
	APS_ERR("Channel[%d] = %d, [Volt]= %dmV ,[Lux]= %d\n",dwChannel,channel[dwChannel],als_voltage,als_to_lux);
    #endif

	return  als_to_lux;
}

void bh1620fvc_als_gain_control(unsigned int gain)
{
  switch (gain)
  {
    case BH1620FVC_HIGH_GAIN:
       break;

    case BH1620FVC_MIDDLE_GAIN:
       break;

    case BH1620FVC_LOW_GAIN:
       break;

    case BH1620FVC_SHUTDOWN:
       break;

   default:
       break;
  }
}
#endif

/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
int bh1620fvc_read_als(struct i2c_client *client, u16 *data)
{
	struct bh1620fvc_priv *obj = i2c_get_clientdata(client);    
	int ret = 0;
	u8 buf[2];

	if(1 != (ret = bh1620fvc_master_recv(client, obj->addr.als_dat1, (char*)&buf[1], 1)))
	{
		APS_ERR("reads als data1 = %d\n", ret);
		return -EFAULT;
	}
	else if(1 != (ret = bh1620fvc_master_recv(client, obj->addr.als_dat0, (char*)&buf[0], 1)))
	{
		APS_ERR("reads als data2 = %d\n", ret);
		return -EFAULT;
	}
	
	*data = (buf[1] << 4)|(buf[0] >>4);

	
	if(atomic_read(&obj->trace) & BH_TRC_ALS_DATA)
	{
		APS_DBG("ALS: 0x%04X\n", (u32)(*data));
	}

      #ifdef GET_ALS_DATA_FROM_AUXADC
        *data = (u16)bh1620fvc_als_get_data_from_adc();
      #endif

	return 0;    
}
/*----------------------------------------------------------------------------*/
int bh1620fvc_write_als(struct i2c_client *client, u8 data)
{
	struct bh1620fvc_priv *obj = i2c_get_clientdata(client);
	int ret = 0;
    
    ret = bh1620fvc_master_send(client, obj->addr.als_cmd, &data, 1);
	if(ret < 0)
	{
		APS_ERR("write als = %d\n", ret);
		return -EFAULT;
	}
	
	return 0;    
}
/*----------------------------------------------------------------------------*/
int bh1620fvc_write_als_intr(struct i2c_client *client, u8 data)
{
	struct bh1620fvc_priv *obj = i2c_get_clientdata(client);
	int ret = 0;
    
    ret = bh1620fvc_master_send(client, obj->addr.als_intr, &data, 1);
	if(ret < 0)
	{
		APS_ERR("write als intr= %d\n", ret);
		return -EFAULT;
	}
	
	return 0;    
}
/*----------------------------------------------------------------------------*/
int bh1620fvc_read_ps(struct i2c_client *client, u8 *data)
{
    struct bh1620fvc_priv *obj = i2c_get_clientdata(client);    
	int ret = 0;

	if(sizeof(*data) != (ret = bh1620fvc_master_recv(client, obj->addr.ps_dat, (char*)data, sizeof(*data))))
	{
		APS_ERR("reads ps data = %d\n", ret);
		return -EFAULT;
	} 

	if(atomic_read(&obj->trace) & BH_TRC_PS_DATA)
	{
		APS_DBG("PS:  0x%04X\n", (u32)(*data));
	}

       #ifdef BH1620FVC_NO_SUPPORT_PS
         *data = 0x00;
       #endif

	return 0; 
}
/*----------------------------------------------------------------------------*/
int bh1620fvc_write_ps(struct i2c_client *client, u8 data)
{
	struct bh1620fvc_priv *obj = i2c_get_clientdata(client);        
	int ret = 0;

    ret = bh1620fvc_master_send(client, obj->addr.ps_cmd, &data, 1);
	if (ret < 0)
	{
		APS_ERR("write ps = %d\n", ret);
		return -EFAULT;
	} 
	return 0;    
}
/*----------------------------------------------------------------------------*/
int bh1620fvc_write_ps_thd(struct i2c_client *client, u8 thd)
{
	struct bh1620fvc_priv *obj = i2c_get_clientdata(client);        
	u8 buf = thd;
	int ret = 0;

	if(sizeof(buf) != (ret = bh1620fvc_master_send(client, obj->addr.ps_thd, (char*)&buf, sizeof(buf))))
	{
		APS_ERR("write thd = %d\n", ret);
		return -EFAULT;
	} 
	return 0;    
}

/*----------------------------------------------------------------------------*/
static void bh1620fvc_power(struct alsps_hw *hw, unsigned int on) 
{
	static unsigned int power_on = 0;

	//APS_LOG("power %s\n", on ? "on" : "off");

	if(hw->power_id != POWER_NONE_MACRO)
	{
		if(power_on == on)
		{
			APS_LOG("ignore power control: %d\n", on);
		}
		else if(on)
		{
			if(!hwPowerOn(hw->power_id, hw->power_vol, "bh1620fvc")) 
			{
				APS_ERR("power on fails!!\n");
			}
		}
		else
		{
			if(!hwPowerDown(hw->power_id, "bh1620fvc")) 
			{
				APS_ERR("power off fail!!\n");   
			}
		}
	}
	power_on = on;
}
/*----------------------------------------------------------------------------*/
static int bh1620fvc_enable_als(struct i2c_client *client, int enable)
{
	struct bh1620fvc_priv *obj = i2c_get_clientdata(client);
	int err, cur = 0, old = atomic_read(&obj->als_cmd_val);
	int trc = atomic_read(&obj->trace);

	if(enable)
	{
		cur = old & (~SD_ALS);   
	}
	else
	{
		cur = old | (SD_ALS); 
	}
	
	if(trc & BH_TRC_DEBUG)
	{
		APS_LOG("%s: %08X, %08X, %d\n", __func__, cur, old, enable);
	}
	
	if(0 == (cur ^ old))
	{
		return 0;
	}
	
	if(0 == (err = bh1620fvc_write_als(client, cur))) 
	{
		atomic_set(&obj->als_cmd_val, cur);
	}
	
	if(enable)
	{
		atomic_set(&obj->als_deb_on, 1);
		atomic_set(&obj->als_deb_end, jiffies+atomic_read(&obj->als_debounce)/(1000/HZ));
		set_bit(BH_BIT_ALS,  &obj->pending_intr);
            #ifdef GET_ALS_DATA_FROM_AUXADC
            #else       
		schedule_delayed_work(&obj->eint_work,260); //after enable the value is not accurate
            #endif 
	}

	if(trc & BH_TRC_DEBUG)
	{
		APS_LOG("enable als (%d)\n", enable);
	}

       #ifdef GET_ALS_DATA_FROM_AUXADC
         if(enable)
         {
           bh1620fvc_als_gain_control(BH1620FVC_MIDDLE_GAIN);
         } 
         else  
         {
           bh1620fvc_als_gain_control(BH1620FVC_SHUTDOWN);
         } 
      #endif 

	return err;
}
/*----------------------------------------------------------------------------*/
static int bh1620fvc_enable_ps(struct i2c_client *client, int enable)
{
	struct bh1620fvc_priv *obj = i2c_get_clientdata(client);
	int err, cur = 0, old = atomic_read(&obj->ps_cmd_val);
	int trc = atomic_read(&obj->trace);

	if(enable)
	{
		cur = old & (~SD_PS);   
	}
	else
	{
		cur = old | (SD_PS);
	}
	
	if(trc & BH_TRC_DEBUG)
	{
		APS_LOG("%s: %08X, %08X, %d\n", __func__, cur, old, enable);
	}
	
	if(0 == (cur ^ old))
	{
		return 0;
	}
	
	if(0 == (err = bh1620fvc_write_ps(client, cur))) 
	{
		atomic_set(&obj->ps_cmd_val, cur);
	}
	
	if(enable)
	{
		atomic_set(&obj->ps_deb_on, 1);
		atomic_set(&obj->ps_deb_end, jiffies+atomic_read(&obj->ps_debounce)/(1000/HZ));
		set_bit(BH_BIT_PS,  &obj->pending_intr);
            #ifdef GET_ALS_DATA_FROM_AUXADC
            #else       
              schedule_delayed_work(&obj->eint_work,120);
	    #endif 
        }

	if(trc & BH_TRC_DEBUG)
	{
		APS_LOG("enable ps  (%d)\n", enable);
	}

	return err;
}
/*----------------------------------------------------------------------------*/
static int bh1620fvc_check_and_clear_intr(struct i2c_client *client) 
{
	struct bh1620fvc_priv *obj = i2c_get_clientdata(client);
	int err;
	u8 status;

    #ifdef GET_ALS_DATA_FROM_AUXADC
           return 0;
    #else
	if (mt_get_gpio_in(GPIO_ALS_EINT_PIN) == 1) /*skip if no interrupt*/  
	    return 0;
    #endif	

    err = bh1620fvc_master_recv(client, obj->addr.rar, &status, 1);
	if (err < 0)
	{
		APS_ERR("WARNING: read status: %d\n", err);
		return 0;
	}
    
	if((status & 0x10) == (obj->addr.als_cmd ))
	{
		set_bit(BH_BIT_ALS, &obj->pending_intr);
	}
	else
	{
	   clear_bit(BH_BIT_ALS, &obj->pending_intr);
	}
	
	if((status & 0x20) == (obj->addr.ps_cmd ))
	{
		set_bit(BH_BIT_PS,  &obj->pending_intr);
	}
	else
	{
	    clear_bit(BH_BIT_PS, &obj->pending_intr);
	}
	
	if(atomic_read(&obj->trace) & BH_TRC_DEBUG)
	{
		APS_LOG("check intr: 0x%02X => 0x%08lX\n", status, obj->pending_intr);
	}

    status = 0;
    err = bh1620fvc_master_send(client, obj->addr.rar, &status, 1);
	if (err < 0)
	{
		APS_ERR("WARNING: clear intrrupt: %d\n", err);
		return 0;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
void bh1620fvc_eint_func(void)
{
	struct bh1620fvc_priv *obj = g_bh1620fvc_ptr;
	APS_LOG(" interrupt fuc\n");
	if(!obj)
	{
		return;
	}
	
	//schedule_work(&obj->eint_work);
     #ifdef GET_ALS_DATA_FROM_AUXADC
     #else
	schedule_delayed_work(&obj->eint_work,0);
     #endif 
	if(atomic_read(&obj->trace) & BH_TRC_EINT)
	{
		APS_LOG("eint: als/ps intrs\n");
	}
}
/*----------------------------------------------------------------------------*/
/*
static void bh1620fvc_eint_work(struct work_struct *work)
{
	struct bh1620fvc_priv *obj = g_bh1620fvc_ptr;
	int err;
	hwm_sensor_data sensor_data;
	
	memset(&sensor_data, 0, sizeof(sensor_data));

	APS_LOG(" eint work\n");
	
	if((err = bh1620fvc_check_and_clear_intr(obj->client)))
	{
		APS_ERR("check intrs: %d\n", err);
	}

    APS_LOG(" &obj->pending_intr =%lx\n",obj->pending_intr);
	
	if((1<<BH_BIT_ALS) & obj->pending_intr)
	{
	  //get raw data
	  APS_LOG(" als change\n");
	  if((err = bh1620fvc_read_als(obj->client, &obj->als)))
	  {
		 APS_ERR("bh1620fvc read als data: %d\n", err);;
	  }
	  //map and store data to hwm_sensor_data
	 
 	  sensor_data.values[0] = bh1620fvc_get_als_value(obj, obj->als);
	  sensor_data.value_divide = 1;
	  sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
	  APS_LOG("als raw %x -> value %d \n", obj->als,sensor_data.values[0]);
	  //let up layer to know
	  if((err = hwmsen_get_interrupt_data(ID_LIGHT, &sensor_data)))
	  {
		APS_ERR("call hwmsen_get_interrupt_data fail = %d\n", err);
	  }
	  
	}
	if((1<<BH_BIT_PS) &  obj->pending_intr)
	{
	  //get raw data
	  APS_LOG(" ps change\n");
	  if((err = bh1620fvc_read_ps(obj->client, &obj->ps)))
	  {
		 APS_ERR("bh1620fvc read ps data: %d\n", err);
	  }
	  //map and store data to hwm_sensor_data
	  sensor_data.values[0] = bh1620fvc_get_ps_value(obj, obj->ps);
	  sensor_data.value_divide = 1;
	  sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
	  //let up layer to know
	  if((err = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data)))
	  {
		APS_ERR("call hwmsen_get_interrupt_data fail = %d\n", err);
	  }
	}
	
	#ifdef MT6516
	MT6516_EINTIRQUnmask(CUST_EINT_ALS_NUM);      
	#endif     
	#ifdef MT6573
	mt65xx_eint_unmask(CUST_EINT_ALS_NUM);      
	#endif
	#ifdef MT6575
	mt65xx_eint_unmask(CUST_EINT_ALS_NUM);      
	#endif

}
*/
/*----------------------------------------------------------------------------*/
int bh1620fvc_setup_eint(struct i2c_client *client)
{
	struct bh1620fvc_priv *obj = i2c_get_clientdata(client);        

	g_bh1620fvc_ptr = obj;
	/*configure to GPIO function, external interrupt*/

	
	mt_set_gpio_mode(GPIO_ALS_EINT_PIN, GPIO_ALS_EINT_PIN_M_EINT);
       mt_set_gpio_dir(GPIO_ALS_EINT_PIN, GPIO_DIR_IN);
	mt_set_gpio_pull_enable(GPIO_ALS_EINT_PIN, GPIO_PULL_ENABLE);
	mt_set_gpio_pull_select(GPIO_ALS_EINT_PIN, GPIO_PULL_UP);
	
	//mt_set_gpio_dir(GPIO_ALS_EINT_PIN, GPIO_DIR_IN);
	//mt_set_gpio_mode(GPIO_ALS_EINT_PIN, GPIO_ALS_EINT_PIN_M_EINT);
	//mt_set_gpio_pull_enable(GPIO_ALS_EINT_PIN, GPIO_PULL_ENABLE);
	//mt_set_gpio_pull_select(GPIO_ALS_EINT_PIN, GPIO_PULL_UP);

#ifdef MT6516

	MT6516_EINT_Set_Sensitivity(CUST_EINT_ALS_NUM, CUST_EINT_ALS_SENSITIVE);
	MT6516_EINT_Set_Polarity(CUST_EINT_ALS_NUM, CUST_EINT_ALS_POLARITY);
	MT6516_EINT_Set_HW_Debounce(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_CN);
	MT6516_EINT_Registration(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_EN, CUST_EINT_ALS_POLARITY, bh1620fvc_eint_func, 0);
	MT6516_EINTIRQUnmask(CUST_EINT_ALS_NUM);  
#endif
    //
#ifdef MT6573
	
    mt65xx_eint_set_sens(CUST_EINT_ALS_NUM, CUST_EINT_ALS_SENSITIVE);
	mt65xx_eint_set_polarity(CUST_EINT_ALS_NUM, CUST_EINT_ALS_POLARITY);
	mt65xx_eint_set_hw_debounce(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_CN);
	mt65xx_eint_registration(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_EN, CUST_EINT_ALS_POLARITY, bh1620fvc_eint_func, 0);
	mt65xx_eint_unmask(CUST_EINT_ALS_NUM);  
#endif  

#ifdef MT6575
	
    mt65xx_eint_set_sens(CUST_EINT_ALS_NUM, CUST_EINT_ALS_SENSITIVE);
	mt65xx_eint_set_polarity(CUST_EINT_ALS_NUM, CUST_EINT_ALS_POLARITY);
	mt65xx_eint_set_hw_debounce(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_CN);
	mt65xx_eint_registration(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_EN, CUST_EINT_ALS_POLARITY, bh1620fvc_eint_func, 0);
	mt65xx_eint_unmask(CUST_EINT_ALS_NUM);  
#endif  
#ifdef MT6577
	
    mt65xx_eint_set_sens(CUST_EINT_ALS_NUM, CUST_EINT_ALS_SENSITIVE);
	mt65xx_eint_set_polarity(CUST_EINT_ALS_NUM, CUST_EINT_ALS_POLARITY);
	mt65xx_eint_set_hw_debounce(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_CN);
	mt65xx_eint_registration(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_EN, CUST_EINT_ALS_POLARITY, bh1620fvc_eint_func, 0);
	mt65xx_eint_unmask(CUST_EINT_ALS_NUM);  
#endif 	
    return 0;
	
}
/*----------------------------------------------------------------------------*/
static int bh1620fvc_init_client(struct i2c_client *client)
{
	struct bh1620fvc_priv *obj = i2c_get_clientdata(client);
	int err;

	 //client->addr |= I2C_ENEXT_FLAG; //for EVB Borad
     #ifdef GET_ALS_DATA_FROM_AUXADC
     #else
	if((err = bh1620fvc_setup_eint(client)))
	{
		APS_ERR("setup eint: %d\n", err);
		return err;
	}
     #endif	
	if((err = bh1620fvc_check_and_clear_intr(client)))
	{
		APS_ERR("check/clear intr: %d\n", err);
		//    return err;
	}
	
	if((err = bh1620fvc_write_als(client, atomic_read(&obj->als_cmd_val))))
	{
		APS_ERR("write als: %d\n", err);
		return err;
	}

	if((err = bh1620fvc_write_als_intr(client, atomic_read(&obj->als_intr_val))))
	{
		APS_ERR("write als intr: %d\n", err);
		return err;
	}
	
	if((err = bh1620fvc_write_ps(client, atomic_read(&obj->ps_cmd_val))))
	{
		APS_ERR("write ps: %d\n", err);
		return err;        
	}
	
	if((err = bh1620fvc_write_ps_thd(client, atomic_read(&obj->ps_thd_val))))
	{
		APS_ERR("write thd: %d\n", err);
		return err;        
	}
	return 0;
}
/******************************************************************************
 * Sysfs attributes
*******************************************************************************/
static ssize_t bh1620fvc_show_config(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	
	if(!bh1620fvc_obj)
	{
		APS_ERR("bh1620fvc_obj is null!!\n");
		return 0;
	}
	
	res = snprintf(buf, PAGE_SIZE, "(%d %d %d %d %d)\n", 
		atomic_read(&bh1620fvc_obj->i2c_retry), atomic_read(&bh1620fvc_obj->als_debounce), 
		atomic_read(&bh1620fvc_obj->ps_mask), atomic_read(&bh1620fvc_obj->ps_thd_val), atomic_read(&bh1620fvc_obj->ps_debounce));     
	return res;    
}
/*----------------------------------------------------------------------------*/
static ssize_t bh1620fvc_store_config(struct device_driver *ddri, const char *buf, size_t count)
{
	int retry, als_deb, ps_deb, mask, thres;
	if(!bh1620fvc_obj)
	{
		APS_ERR("bh1620fvc_obj is null!!\n");
		return 0;
	}
	
	if(5 == sscanf(buf, "%d %d %d %d %d", &retry, &als_deb, &mask, &thres, &ps_deb))
	{ 
		atomic_set(&bh1620fvc_obj->i2c_retry, retry);
		atomic_set(&bh1620fvc_obj->als_debounce, als_deb);
		atomic_set(&bh1620fvc_obj->ps_mask, mask);
		atomic_set(&bh1620fvc_obj->ps_thd_val, thres);        
		atomic_set(&bh1620fvc_obj->ps_debounce, ps_deb);
	}
	else
	{
		APS_ERR("invalid content: '%s', length = %d\n", buf, count);
	}
	return count;    
}
/*----------------------------------------------------------------------------*/
static ssize_t bh1620fvc_show_trace(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	if(!bh1620fvc_obj)
	{
		APS_ERR("bh1620fvc_obj is null!!\n");
		return 0;
	}

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&bh1620fvc_obj->trace));     
	return res;    
}
/*----------------------------------------------------------------------------*/
static ssize_t bh1620fvc_store_trace(struct device_driver *ddri, const char *buf, size_t count)
{
    int trace;
    if(!bh1620fvc_obj)
	{
		APS_ERR("bh1620fvc_obj is null!!\n");
		return 0;
	}
	
	if(1 == sscanf(buf, "0x%x", &trace))
	{
		atomic_set(&bh1620fvc_obj->trace, trace);
	}
	else 
	{
		APS_ERR("invalid content: '%s', length = %d\n", buf, count);
	}
	return count;    
}
/*----------------------------------------------------------------------------*/
static ssize_t bh1620fvc_show_als(struct device_driver *ddri, char *buf)
{
	int res;
	
	if(!bh1620fvc_obj)
	{
		APS_ERR("bh1620fvc_obj is null!!\n");
		return 0;
	}
	if((res = bh1620fvc_read_als(bh1620fvc_obj->client, &bh1620fvc_obj->als)))
	{
		return snprintf(buf, PAGE_SIZE, "ERROR: %d\n", res);
	}
	else
	{
		return snprintf(buf, PAGE_SIZE, "0x%04X\n", bh1620fvc_obj->als);     
	}
}
/*----------------------------------------------------------------------------*/
static ssize_t bh1620fvc_show_ps(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	if(!bh1620fvc_obj)
	{
		APS_ERR("bh1620fvc_obj is null!!\n");
		return 0;
	}
	
	if((res = bh1620fvc_read_ps(bh1620fvc_obj->client, &bh1620fvc_obj->ps)))
	{
		return snprintf(buf, PAGE_SIZE, "ERROR: %d\n", res);
	}
	else
	{
		return snprintf(buf, PAGE_SIZE, "0x%04X\n", bh1620fvc_obj->ps);     
	}
}
/*----------------------------------------------------------------------------*/
static ssize_t bh1620fvc_show_reg(struct device_driver *ddri, char *buf)
{
	if(!bh1620fvc_obj)
	{
		APS_ERR("bh1620fvc_obj is null!!\n");
		return 0;
	}
	
	/*read*/
	bh1620fvc_check_and_clear_intr(bh1620fvc_obj->client);
	bh1620fvc_read_ps(bh1620fvc_obj->client, &bh1620fvc_obj->ps);
	bh1620fvc_read_als(bh1620fvc_obj->client, &bh1620fvc_obj->als);
	/*write*/
	bh1620fvc_write_als(bh1620fvc_obj->client, atomic_read(&bh1620fvc_obj->als_cmd_val));
	bh1620fvc_write_als_intr(bh1620fvc_obj->client, atomic_read(&bh1620fvc_obj->als_intr_val));
	bh1620fvc_write_ps(bh1620fvc_obj->client, atomic_read(&bh1620fvc_obj->ps_cmd_val)); 
	bh1620fvc_write_ps_thd(bh1620fvc_obj->client, atomic_read(&bh1620fvc_obj->ps_thd_val));
	return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t bh1620fvc_show_send(struct device_driver *ddri, char *buf)
{
    return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t bh1620fvc_store_send(struct device_driver *ddri, const char *buf, size_t count)
{
	int addr, cmd;
	u8 dat;

	if(!bh1620fvc_obj)
	{
		APS_ERR("bh1620fvc_obj is null!!\n");
		return 0;
	}
	else if(2 != sscanf(buf, "%x %x", &addr, &cmd))
	{
		APS_ERR("invalid format: '%s'\n", buf);
		return 0;
	}

	dat = (u8)cmd;
	APS_LOG("send(%02X, %02X) = %d\n", addr, cmd, 
	bh1620fvc_master_send(bh1620fvc_obj->client, (u16)addr, &dat, sizeof(dat)));
	
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t bh1620fvc_show_recv(struct device_driver *ddri, char *buf)
{
    return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t bh1620fvc_store_recv(struct device_driver *ddri, const char *buf, size_t count)
{
	int addr;
	u8 dat = 0;
	if(!bh1620fvc_obj)
	{
		APS_ERR("bh1620fvc_obj is null!!\n");
		return 0;
	}
	else if(1 != sscanf(buf, "%x", &addr))
	{
		APS_ERR("invalid format: '%s'\n", buf);
		return 0;
	}

	APS_LOG("recv(%02X) = %d, 0x%02X\n", addr, 
	bh1620fvc_master_recv(bh1620fvc_obj->client, (u16)addr, (char*)&dat, sizeof(dat)), dat);
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t bh1620fvc_show_status(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	
	if(!bh1620fvc_obj)
	{
		APS_ERR("bh1620fvc_obj is null!!\n");
		return 0;
	}
	
	if(bh1620fvc_obj->hw)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: %d, (%d %d) (%02X %02X) (%02X %02X %02X) (%02X %02X %02X)\n", 
			bh1620fvc_obj->hw->i2c_num, bh1620fvc_obj->hw->power_id, bh1620fvc_obj->hw->power_vol, bh1620fvc_obj->addr.init, 
			bh1620fvc_obj->addr.rar,bh1620fvc_obj->addr.als_cmd, bh1620fvc_obj->addr.als_dat0, bh1620fvc_obj->addr.als_dat1,
			bh1620fvc_obj->addr.ps_cmd, bh1620fvc_obj->addr.ps_dat, bh1620fvc_obj->addr.ps_thd);
	}
	else
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");
	}
	
	len += snprintf(buf+len, PAGE_SIZE-len, "REGS: %02X %02X %02X %02lX %02lX\n", 
				atomic_read(&bh1620fvc_obj->als_cmd_val), atomic_read(&bh1620fvc_obj->ps_cmd_val), 
				atomic_read(&bh1620fvc_obj->ps_thd_val),bh1620fvc_obj->enable, bh1620fvc_obj->pending_intr);
	#ifdef MT6516
	len += snprintf(buf+len, PAGE_SIZE-len, "EINT: %d (%d %d %d %d)\n", mt_get_gpio_in(GPIO_ALS_EINT_PIN),
				CUST_EINT_ALS_NUM, CUST_EINT_ALS_POLARITY, CUST_EINT_ALS_DEBOUNCE_EN, CUST_EINT_ALS_DEBOUNCE_CN);

	len += snprintf(buf+len, PAGE_SIZE-len, "GPIO: %d (%d %d %d %d)\n",	GPIO_ALS_EINT_PIN, 
				mt_get_gpio_dir(GPIO_ALS_EINT_PIN), mt_get_gpio_mode(GPIO_ALS_EINT_PIN), 
				mt_get_gpio_pull_enable(GPIO_ALS_EINT_PIN), mt_get_gpio_pull_select(GPIO_ALS_EINT_PIN));
	#endif

	len += snprintf(buf+len, PAGE_SIZE-len, "MISC: %d %d\n", atomic_read(&bh1620fvc_obj->als_suspend), atomic_read(&bh1620fvc_obj->ps_suspend));

	return len;
}
/*----------------------------------------------------------------------------*/
static ssize_t bh1620fvc_show_i2c(struct device_driver *ddri, char *buf)
{
/*
	ssize_t len = 0;
	u32 base = I2C2_BASE;

	if(!bh1620fvc_obj)
	{
		APS_ERR("bh1620fvc_obj is null!!\n");
		return 0;
	}
	
	len += snprintf(buf+len, PAGE_SIZE-len, "DATA_PORT      = 0x%08X\n", __raw_readl(mt6516_I2C_DATA_PORT    ));
	len += snprintf(buf+len, PAGE_SIZE-len, "SLAVE_ADDR     = 0x%08X\n", __raw_readl(mt6516_I2C_SLAVE_ADDR));
	len += snprintf(buf+len, PAGE_SIZE-len, "INTR_MASK      = 0x%08X\n", __raw_readl(mt6516_I2C_INTR_MASK));
	len += snprintf(buf+len, PAGE_SIZE-len, "INTR_STAT      = 0x%08X\n", __raw_readl(mt6516_I2C_INTR_STAT));
	len += snprintf(buf+len, PAGE_SIZE-len, "CONTROL        = 0x%08X\n", __raw_readl(mt6516_I2C_CONTROL));
	len += snprintf(buf+len, PAGE_SIZE-len, "TRANSFER_LEN   = 0x%08X\n", __raw_readl(mt6516_I2C_TRANSFER_LEN));
	len += snprintf(buf+len, PAGE_SIZE-len, "TRANSAC_LEN    = 0x%08X\n", __raw_readl(mt6516_I2C_TRANSAC_LEN));
	len += snprintf(buf+len, PAGE_SIZE-len, "DELAY_LEN      = 0x%08X\n", __raw_readl(mt6516_I2C_DELAY_LEN));
	len += snprintf(buf+len, PAGE_SIZE-len, "TIMING         = 0x%08X\n", __raw_readl(mt6516_I2C_TIMING));
	len += snprintf(buf+len, PAGE_SIZE-len, "START          = 0x%08X\n", __raw_readl(mt6516_I2C_START));
	len += snprintf(buf+len, PAGE_SIZE-len, "FIFO_STAT      = 0x%08X\n", __raw_readl(mt6516_I2C_FIFO_STAT));
	len += snprintf(buf+len, PAGE_SIZE-len, "FIFO_THRESH    = 0x%08X\n", __raw_readl(mt6516_I2C_FIFO_THRESH));
	len += snprintf(buf+len, PAGE_SIZE-len, "FIFO_ADDR_CLR  = 0x%08X\n", __raw_readl(mt6516_I2C_FIFO_ADDR_CLR));
	len += snprintf(buf+len, PAGE_SIZE-len, "IO_CONFIG      = 0x%08X\n", __raw_readl(mt6516_I2C_IO_CONFIG));
	len += snprintf(buf+len, PAGE_SIZE-len, "DEBUG          = 0x%08X\n", __raw_readl(mt6516_I2C_DEBUG));
	len += snprintf(buf+len, PAGE_SIZE-len, "HS             = 0x%08X\n", __raw_readl(mt6516_I2C_HS));
	len += snprintf(buf+len, PAGE_SIZE-len, "DEBUGSTAT      = 0x%08X\n", __raw_readl(mt6516_I2C_DEBUGSTAT));
	len += snprintf(buf+len, PAGE_SIZE-len, "DEBUGCTRL      = 0x%08X\n", __raw_readl(mt6516_I2C_DEBUGCTRL));    

	return len;
*/
	return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t bh1620fvc_store_i2c(struct device_driver *ddri, const char *buf, size_t count)
{
/*
	int sample_div, step_div;
	unsigned long tmp;
	u32 base = I2C2_BASE;    

	if(!bh1620fvc_obj)
	{
		APS_ERR("bh1620fvc_obj is null!!\n");
		return 0;
	}
	else if(2 != sscanf(buf, "%d %d", &sample_div, &step_div))
	{
		APS_ERR("invalid format: '%s'\n", buf);
		return 0;
	}
	tmp  = __raw_readw(mt6516_I2C_TIMING) & ~((0x7 << 8) | (0x1f << 0));
	tmp  = (sample_div & 0x7) << 8 | (step_div & 0x1f) << 0 | tmp;
	__raw_writew(tmp, mt6516_I2C_TIMING);        

	return count; 
*/
       return 0;
}
/*----------------------------------------------------------------------------*/
#define IS_SPACE(CH) (((CH) == ' ') || ((CH) == '\n'))
/*----------------------------------------------------------------------------*/
static int read_int_from_buf(struct bh1620fvc_priv *obj, const char* buf, size_t count,
                             u32 data[], int len)
{
	int idx = 0;
	char *cur = (char*)buf, *end = (char*)(buf+count);

	while(idx < len)
	{
		while((cur < end) && IS_SPACE(*cur))
		{
			cur++;        
		}

		if(1 != sscanf(cur, "%d", &data[idx]))
		{
			break;
		}

		idx++; 
		while((cur < end) && !IS_SPACE(*cur))
		{
			cur++;
		}
	}
	return idx;
}
/*----------------------------------------------------------------------------*/
static ssize_t bh1620fvc_show_alslv(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	int idx;
	if(!bh1620fvc_obj)
	{
		APS_ERR("bh1620fvc_obj is null!!\n");
		return 0;
	}
	
	for(idx = 0; idx < bh1620fvc_obj->als_level_num; idx++)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "%d ", bh1620fvc_obj->hw->als_level[idx]);
	}
	len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	return len;    
}
/*----------------------------------------------------------------------------*/
static ssize_t bh1620fvc_store_alslv(struct device_driver *ddri, const char *buf, size_t count)
{
//	struct bh1620fvc_priv *obj;
	if(!bh1620fvc_obj)
	{
		APS_ERR("bh1620fvc_obj is null!!\n");
		return 0;
	}
	else if(!strcmp(buf, "def"))
	{
		memcpy(bh1620fvc_obj->als_level, bh1620fvc_obj->hw->als_level, sizeof(bh1620fvc_obj->als_level));
	}
	else if(bh1620fvc_obj->als_level_num != read_int_from_buf(bh1620fvc_obj, buf, count, 
			bh1620fvc_obj->hw->als_level, bh1620fvc_obj->als_level_num))
	{
		APS_ERR("invalid format: '%s'\n", buf);
	}    
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t bh1620fvc_show_alsval(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	int idx;
	if(!bh1620fvc_obj)
	{
		APS_ERR("bh1620fvc_obj is null!!\n");
		return 0;
	}
	
	for(idx = 0; idx < bh1620fvc_obj->als_value_num; idx++)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "%d ", bh1620fvc_obj->hw->als_value[idx]);
	}
	len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	return len;    
}
/*----------------------------------------------------------------------------*/
static ssize_t bh1620fvc_store_alsval(struct device_driver *ddri, const char *buf, size_t count)
{
	if(!bh1620fvc_obj)
	{
		APS_ERR("bh1620fvc_obj is null!!\n");
		return 0;
	}
	else if(!strcmp(buf, "def"))
	{
		memcpy(bh1620fvc_obj->als_value, bh1620fvc_obj->hw->als_value, sizeof(bh1620fvc_obj->als_value));
	}
	else if(bh1620fvc_obj->als_value_num != read_int_from_buf(bh1620fvc_obj, buf, count, 
			bh1620fvc_obj->hw->als_value, bh1620fvc_obj->als_value_num))
	{
		APS_ERR("invalid format: '%s'\n", buf);
	}    
	return count;
}
/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(als,     S_IWUSR | S_IRUGO, bh1620fvc_show_als,   NULL);
static DRIVER_ATTR(ps,      S_IWUSR | S_IRUGO, bh1620fvc_show_ps,    NULL);
static DRIVER_ATTR(config,  S_IWUSR | S_IRUGO, bh1620fvc_show_config,bh1620fvc_store_config);
static DRIVER_ATTR(alslv,   S_IWUSR | S_IRUGO, bh1620fvc_show_alslv, bh1620fvc_store_alslv);
static DRIVER_ATTR(alsval,  S_IWUSR | S_IRUGO, bh1620fvc_show_alsval,bh1620fvc_store_alsval);
static DRIVER_ATTR(trace,   S_IWUSR | S_IRUGO, bh1620fvc_show_trace, bh1620fvc_store_trace);
static DRIVER_ATTR(status,  S_IWUSR | S_IRUGO, bh1620fvc_show_status,  NULL);
static DRIVER_ATTR(send,    S_IWUSR | S_IRUGO, bh1620fvc_show_send,  bh1620fvc_store_send);
static DRIVER_ATTR(recv,    S_IWUSR | S_IRUGO, bh1620fvc_show_recv,  bh1620fvc_store_recv);
static DRIVER_ATTR(reg,     S_IWUSR | S_IRUGO, bh1620fvc_show_reg,   NULL);
static DRIVER_ATTR(i2c,     S_IWUSR | S_IRUGO, bh1620fvc_show_i2c,   bh1620fvc_store_i2c);
/*----------------------------------------------------------------------------*/
static struct driver_attribute *bh1620fvc_attr_list[] = {
    &driver_attr_als,
    &driver_attr_ps,    
    &driver_attr_trace,        /*trace log*/
    &driver_attr_config,
    &driver_attr_alslv,
    &driver_attr_alsval,
    &driver_attr_status,
    &driver_attr_send,
    &driver_attr_recv,
    &driver_attr_i2c,
    &driver_attr_reg,
};

/*----------------------------------------------------------------------------*/
static int bh1620fvc_create_attr(struct device_driver *driver) 
{
	int idx, err = 0;
	int num = (int)(sizeof(bh1620fvc_attr_list)/sizeof(bh1620fvc_attr_list[0]));
	if (driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
		if((err = driver_create_file(driver, bh1620fvc_attr_list[idx])))
		{            
			APS_ERR("driver_create_file (%s) = %d\n", bh1620fvc_attr_list[idx]->attr.name, err);
			break;
		}
	}    
	return err;
}
/*----------------------------------------------------------------------------*/
	static int bh1620fvc_delete_attr(struct device_driver *driver)
	{
	int idx ,err = 0;
	int num = (int)(sizeof(bh1620fvc_attr_list)/sizeof(bh1620fvc_attr_list[0]));

	if (!driver)
	return -EINVAL;

	for (idx = 0; idx < num; idx++) 
	{
		driver_remove_file(driver, bh1620fvc_attr_list[idx]);
	}
	
	return err;
}
/****************************************************************************** 
 * Function Configuration
******************************************************************************/
static int bh1620fvc_get_als_value(struct bh1620fvc_priv *obj, u16 als)
{
	int idx;
	int invalid = 0;
	for(idx = 0; idx < obj->als_level_num; idx++)
	{
		if(als < obj->hw->als_level[idx])
		{
			break;
		}
	}
	
	if(idx >= obj->als_value_num)
	{
		APS_ERR("exceed range\n"); 
		idx = obj->als_value_num - 1;
	}
	
	if(1 == atomic_read(&obj->als_deb_on))
	{
		unsigned long endt = atomic_read(&obj->als_deb_end);
		if(time_after(jiffies, endt))
		{
			atomic_set(&obj->als_deb_on, 0);
		}
		
		if(1 == atomic_read(&obj->als_deb_on))
		{
			invalid = 1;
		}
	}

	if(!invalid)
	{
		if (atomic_read(&obj->trace) & BH_TRC_CVT_ALS)
		{
			APS_DBG("ALS: %05d => %05d\n", als, obj->hw->als_value[idx]);
		}
		
		return obj->hw->als_value[idx];
	}
	else
	{
		if(atomic_read(&obj->trace) & BH_TRC_CVT_ALS)
		{
			APS_DBG("ALS: %05d => %05d (-1)\n", als, obj->hw->als_value[idx]);    
		}
		return -1;
	}
}
/*----------------------------------------------------------------------------*/
static int bh1620fvc_get_ps_value(struct bh1620fvc_priv *obj, u16 ps)
{
	int val, mask = atomic_read(&obj->ps_mask);
	int invalid = 0;

	if(ps > atomic_read(&obj->ps_thd_val))
	{
		val = 0;  /*close*/
	}
	else
	{
		val = 1;  /*far away*/
	}
	
	if(atomic_read(&obj->ps_suspend))
	{
		invalid = 1;
	}
	else if(1 == atomic_read(&obj->ps_deb_on))
	{
		unsigned long endt = atomic_read(&obj->ps_deb_end);
		if(time_after(jiffies, endt))
		{
			atomic_set(&obj->ps_deb_on, 0);
		}
		
		if (1 == atomic_read(&obj->ps_deb_on))
		{
			invalid = 1;
		}
	}

	if(!invalid)
	{
		if(unlikely(atomic_read(&obj->trace) & BH_TRC_CVT_PS))
		{
			if(mask)
			{
				APS_DBG("PS:  %05d => %05d [M] \n", ps, val);
			}
			else
			{
				APS_DBG("PS:  %05d => %05d\n", ps, val);
			}
		}
		return val;
		
	}	
	else
	{
		if(unlikely(atomic_read(&obj->trace) & BH_TRC_CVT_PS))
		{
			APS_DBG("PS:  %05d => %05d (-1)\n", ps, val);    
		}
		return -1;
	}	
}
/****************************************************************************** 
 * Function Configuration
******************************************************************************/
static int bh1620fvc_open(struct inode *inode, struct file *file)
{
	file->private_data = bh1620fvc_i2c_client;

	if (!file->private_data)
	{
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}
	
	return nonseekable_open(inode, file);
}
/*----------------------------------------------------------------------------*/
static int bh1620fvc_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}
/*----------------------------------------------------------------------------*/
//static int bh1620fvc_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
//       unsigned long arg)
static long bh1620fvc_unlocked_ioctl(struct file *file, unsigned int cmd,
       unsigned long arg)
{
	struct i2c_client *client = (struct i2c_client*)file->private_data;
	struct bh1620fvc_priv *obj = i2c_get_clientdata(client);  
	long err = 0;
	void __user *ptr = (void __user*) arg;
	int dat;
	uint32_t enable;

	switch (cmd)
	{
		case ALSPS_SET_PS_MODE:
			if(copy_from_user(&enable, ptr, sizeof(enable)))
			{
				err = -EFAULT;
				goto err_out;
			}

			//Forced to close P-sensor
			//err = -EFAULT;
			//goto err_out;
			
			if(enable)
			{
				if((err = bh1620fvc_enable_ps(obj->client, 1)))
				{
					APS_ERR("enable ps fail: %1ld\n", err); 
					goto err_out;
				}
				
				set_bit(BH_BIT_PS, &obj->enable);
			}
			else
			{
				if((err = bh1620fvc_enable_ps(obj->client, 0)))
				{
					APS_ERR("disable ps fail: %1ld\n", err); 
					goto err_out;
				}
				
				clear_bit(BH_BIT_PS, &obj->enable);
			}
			break;

		case ALSPS_GET_PS_MODE:
			enable = test_bit(BH_BIT_PS, &obj->enable) ? (1) : (0);
			if(copy_to_user(ptr, &enable, sizeof(enable)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;

		case ALSPS_GET_PS_DATA:    
			if((err = bh1620fvc_read_ps(obj->client, &obj->ps)))
			{
				goto err_out;
			}
			
			dat = bh1620fvc_get_ps_value(obj, obj->ps);
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}  
			break;

		case ALSPS_GET_PS_RAW_DATA:    
			if((err = bh1620fvc_read_ps(obj->client, &obj->ps)))
			{
				goto err_out;
			}
			
			dat = obj->ps;
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}  
			break;            

		case ALSPS_SET_ALS_MODE:
			if(copy_from_user(&enable, ptr, sizeof(enable)))
			{
				err = -EFAULT;
				goto err_out;
			}
			if(enable)
			{
				if((err = bh1620fvc_enable_als(obj->client, 1)))
				{
					APS_ERR("enable als fail: %1ld\n", err); 
					goto err_out;
				}
				set_bit(BH_BIT_ALS, &obj->enable);
			}
			else
			{
				if((err = bh1620fvc_enable_als(obj->client, 0)))
				{
					APS_ERR("disable als fail: %ld\n", err); 
					goto err_out;
				}
				clear_bit(BH_BIT_ALS, &obj->enable);
			}
			break;

		case ALSPS_GET_ALS_MODE:
			enable = test_bit(BH_BIT_ALS, &obj->enable) ? (1) : (0);
			if(copy_to_user(ptr, &enable, sizeof(enable)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;

		case ALSPS_GET_ALS_DATA: 
			if((err = bh1620fvc_read_als(obj->client, &obj->als)))
			{
				goto err_out;
			}

			dat = bh1620fvc_get_als_value(obj, obj->als);
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}              
			break;

		case ALSPS_GET_ALS_RAW_DATA:    
			if((err = bh1620fvc_read_als(obj->client, &obj->als)))
			{
				goto err_out;
			}

			dat = obj->als;
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}              
			break;

		default:
			APS_ERR("%s not supported = 0x%04x", __FUNCTION__, cmd);
			err = -ENOIOCTLCMD;
			break;
	}

	err_out:
	return err;    
}
/*----------------------------------------------------------------------------*/
static struct file_operations bh1620fvc_fops = {
//	.owner = THIS_MODULE,
	.open = bh1620fvc_open,
	.release = bh1620fvc_release,
	.unlocked_ioctl = bh1620fvc_unlocked_ioctl,
};
/*----------------------------------------------------------------------------*/
static struct miscdevice bh1620fvc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "als_ps",
	.fops = &bh1620fvc_fops,
};
/*----------------------------------------------------------------------------*/
static int bh1620fvc_i2c_suspend(struct i2c_client *client, pm_message_t msg) 
{
//	struct bh1620fvc_priv *obj = i2c_get_clientdata(client);    
//	int err;
	APS_FUN();    
/*
	if(msg.event == PM_EVENT_SUSPEND)
	{   
		if(!obj)
		{
			APS_ERR("null pointer!!\n");
			return -EINVAL;
		}
		
		atomic_set(&obj->als_suspend, 1);
		if((err = bh1620fvc_enable_als(client, 0)))
		{
			APS_ERR("disable als: %d\n", err);
			return err;
		}

		atomic_set(&obj->ps_suspend, 1);
		if((err = bh1620fvc_enable_ps(client, 0)))
		{
			APS_ERR("disable ps:  %d\n", err);
			return err;
		}
		
		bh1620fvc_power(obj->hw, 0);
	}
*/
	return 0;
}
/*----------------------------------------------------------------------------*/
static int bh1620fvc_i2c_resume(struct i2c_client *client)
{
//	struct bh1620fvc_priv *obj = i2c_get_clientdata(client);        
//	int err;
	APS_FUN();
/*
	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}

	bh1620fvc_power(obj->hw, 1);
	if((err = bh1620fvc_init_client(client)))
	{
		APS_ERR("initialize client fail!!\n");
		return err;        
	}
	atomic_set(&obj->als_suspend, 0);
	if(test_bit(BH_BIT_ALS, &obj->enable))
	{
		if((err = bh1620fvc_enable_als(client, 1)))
		{
			APS_ERR("enable als fail: %d\n", err);        
		}
	}
	atomic_set(&obj->ps_suspend, 0);
	if(test_bit(BH_BIT_PS,  &obj->enable))
	{
		if((err = bh1620fvc_enable_ps(client, 1)))
		{
			APS_ERR("enable ps fail: %d\n", err);                
		}
	}
*/
	return 0;
}
/*----------------------------------------------------------------------------*/
static void bh1620fvc_early_suspend(struct early_suspend *h) 
{   /*early_suspend is only applied for ALS*/
	struct bh1620fvc_priv *obj = container_of(h, struct bh1620fvc_priv, early_drv);   
	int err;
	APS_FUN();    

	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return;
	}
	
	atomic_set(&obj->als_suspend, 1);    
	if((err = bh1620fvc_enable_als(obj->client, 0)))
	{
		APS_ERR("disable als fail: %d\n", err); 
	}
}
/*----------------------------------------------------------------------------*/
static void bh1620fvc_late_resume(struct early_suspend *h)
{   /*early_suspend is only applied for ALS*/
	struct bh1620fvc_priv *obj = container_of(h, struct bh1620fvc_priv, early_drv);         
	int err;
	hwm_sensor_data sensor_data;
	
	memset(&sensor_data, 0, sizeof(sensor_data));
	APS_FUN();

	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return;
	}

	atomic_set(&obj->als_suspend, 0);
	if(test_bit(BH_BIT_ALS, &obj->enable))
	{
		if((err = bh1620fvc_enable_als(obj->client, 1)))
		{
			APS_ERR("enable als fail: %d\n", err);        

		}
	}
}

int bh1620fvc_ps_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	hwm_sensor_data* sensor_data;
	struct bh1620fvc_priv *obj = (struct bh1620fvc_priv *)self;
	
	//APS_FUN(f);
	switch (command)
	{
		case SENSOR_DELAY:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				APS_ERR("Set delay parameter error!\n");
				err = -EINVAL;
			}
			// Do nothing
			break;

		case SENSOR_ENABLE:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				APS_ERR("Enable sensor parameter error!\n");
				err = -EINVAL;
			}
			else
			{				
				value = *(int *)buff_in;
				if(value)
				{
					if((err = bh1620fvc_enable_ps(obj->client, 1)))
					{
						APS_ERR("enable ps fail: %d\n", err); 
						return -1;
					}
					set_bit(BH_BIT_PS, &obj->enable);
				}
				else
				{
					if((err = bh1620fvc_enable_ps(obj->client, 0)))
					{
						APS_ERR("disable ps fail: %d\n", err); 
						return -1;
					}
					clear_bit(BH_BIT_PS, &obj->enable);
				}
			}
			break;

		case SENSOR_GET_DATA:
			if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
			{
				APS_ERR("get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				sensor_data = (hwm_sensor_data *)buff_out;				
				
				if((err = bh1620fvc_read_ps(obj->client, &obj->ps)))
				{
					err = -1;;
				}
				else
				{
					sensor_data->values[0] = bh1620fvc_get_ps_value(obj, obj->ps);
					sensor_data->value_divide = 1;
					sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
				}				
			}
			break;
		default:
			APS_ERR("proxmy sensor operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}
	
	return err;
}

int bh1620fvc_als_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	hwm_sensor_data* sensor_data;
	struct bh1620fvc_priv *obj = (struct bh1620fvc_priv *)self;
	
	//APS_FUN(f);
	switch (command)
	{
		case SENSOR_DELAY:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				APS_ERR("Set delay parameter error!\n");
				err = -EINVAL;
			}
			// Do nothing
			break;

		case SENSOR_ENABLE:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				APS_ERR("Enable sensor parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				value = *(int *)buff_in;				
				if(value)
				{
					if((err = bh1620fvc_enable_als(obj->client, 1)))
					{
						APS_ERR("enable als fail: %d\n", err); 
						return -1;
					}
					set_bit(BH_BIT_ALS, &obj->enable);
				}
				else
				{
					if((err = bh1620fvc_enable_als(obj->client, 0)))
					{
						APS_ERR("disable als fail: %d\n", err); 
						return -1;
					}
					clear_bit(BH_BIT_ALS, &obj->enable);
				}
				
			}
			break;

		case SENSOR_GET_DATA:
			if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
			{
				APS_ERR("get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				sensor_data = (hwm_sensor_data *)buff_out;
								
				if((err = bh1620fvc_read_als(obj->client, &obj->als)))
				{
					err = -1;;
				}
				else
				{
					#if defined(MTK_AAL_SUPPORT)
					sensor_data->values[0] = obj->als;
					#else
					sensor_data->values[0] = bh1620fvc_get_als_value(obj, obj->als);
					#endif
					sensor_data->value_divide = 1;
					sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
				}				
			}
			break;
		default:
			APS_ERR("light sensor operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}
	
	return err;
}


/*----------------------------------------------------------------------------*/
/*
static int bh1620fvc_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info) 
{    
	strcpy(info->type, BH1620FVC_DEV_NAME);
	return 0;
}
*/
/*----------------------------------------------------------------------------*/
static int bh1620fvc_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct bh1620fvc_priv *obj;
	struct hwmsen_object obj_ps, obj_als;
	int err = 0;

	if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
	{
		err = -ENOMEM;
		goto exit;
	}
	memset(obj, 0, sizeof(*obj));
	bh1620fvc_obj = obj;

	obj->hw = get_cust_alsps_hw();
	bh1620fvc_get_addr(obj->hw, &obj->addr);

    #ifdef GET_ALS_DATA_FROM_AUXADC
    #else
	INIT_DELAYED_WORK(&obj->eint_work, bh1620fvc_eint_work);
    #endif
	obj->client = client;
	i2c_set_clientdata(client, obj);	
	atomic_set(&obj->als_debounce, 2000);
	atomic_set(&obj->als_deb_on, 0);
	atomic_set(&obj->als_deb_end, 0);
	atomic_set(&obj->ps_debounce, 1000);
	atomic_set(&obj->ps_deb_on, 0);
	atomic_set(&obj->ps_deb_end, 0);
	atomic_set(&obj->ps_mask, 0);
	atomic_set(&obj->trace, 0x00);
	atomic_set(&obj->als_suspend, 0);
	atomic_set(&obj->als_cmd_val, GAIN00_ALS|IT01_ALS|SD_ALS); //Gain:1/8     Refresh:100ms   
	atomic_set(&obj->als_intr_val, THD01_ALS|PRST01_ALS);             //THD : 0x00  Disable INTR      
	atomic_set(&obj->ps_cmd_val, 0x01);
    atomic_set(&obj->ps_thd_val,  obj->hw->ps_threshold);
	if(obj->hw->polling_mode == 0)
	{
        atomic_set(&obj->als_intr_val, THD01_ALS|PRST01_ALS|FLAG_ALS);  //THD : 0x00  Enablee INTR      
        //atomic_add(&obj->ps_cmd_val, 0x02);
        atomic_set(&obj->ps_cmd_val, 0x03);
	  APS_LOG("enable interrupt\n");
	}
	
	obj->enable = 0;
	obj->pending_intr = 0;
	obj->als_level_num = sizeof(obj->hw->als_level)/sizeof(obj->hw->als_level[0]);
	obj->als_value_num = sizeof(obj->hw->als_value)/sizeof(obj->hw->als_value[0]);   
	BUG_ON(sizeof(obj->als_level) != sizeof(obj->hw->als_level));
	memcpy(obj->als_level, obj->hw->als_level, sizeof(obj->als_level));
	BUG_ON(sizeof(obj->als_value) != sizeof(obj->hw->als_value));
	memcpy(obj->als_value, obj->hw->als_value, sizeof(obj->als_value));
	atomic_set(&obj->i2c_retry, 3);
	if(!(atomic_read(&obj->als_cmd_val) & SD_ALS))
	{
		set_bit(BH_BIT_ALS, &obj->enable);
	}
	
	if(!(atomic_read(&obj->ps_cmd_val) & SD_PS))
	{
		set_bit(BH_BIT_PS, &obj->enable);
	}
	
	bh1620fvc_i2c_client = client;

	
	if((err = bh1620fvc_init_client(client)))
	{
		goto exit_init_failed;
	}
	
	if((err = misc_register(&bh1620fvc_device)))
	{
		APS_ERR("bh1620fvc_device register failed\n");
		goto exit_misc_device_register_failed;
	}

	if((err = bh1620fvc_create_attr(&bh1620fvc_alsps_driver.driver)))
	{
		APS_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}
	obj_ps.self = bh1620fvc_obj;
	if(1 == obj->hw->polling_mode)
	{
	  obj_ps.polling = 1;
	}
	else
	{
	  obj_ps.polling = 0;//interrupt mode
	}
	obj_ps.sensor_operate = bh1620fvc_ps_operate;
	if((err = hwmsen_attach(ID_PROXIMITY, &obj_ps)))
	{
		APS_ERR("attach fail = %d\n", err);
		goto exit_create_attr_failed;
	}
	
	obj_als.self = bh1620fvc_obj;
	if(1 == obj->hw->polling_mode)
	{
	  obj_als.polling = 1;
	}
	else
	{
	  obj_als.polling = 0;//interrupt mode
	}
	obj_als.sensor_operate = bh1620fvc_als_operate;
	if((err = hwmsen_attach(ID_LIGHT, &obj_als)))
	{
		APS_ERR("attach fail = %d\n", err);
		goto exit_create_attr_failed;
	}


#if defined(CONFIG_HAS_EARLYSUSPEND)
	obj->early_drv.level    = EARLY_SUSPEND_LEVEL_STOP_DRAWING - 2,
	obj->early_drv.suspend  = bh1620fvc_early_suspend,
	obj->early_drv.resume   = bh1620fvc_late_resume,    
	register_early_suspend(&obj->early_drv);
#endif

	APS_LOG("%s: OK\n", __func__);
	return 0;

	exit_create_attr_failed:
	misc_deregister(&bh1620fvc_device);
	exit_misc_device_register_failed:
	exit_init_failed:
	//i2c_detach_client(client);
//	exit_kfree:
	kfree(obj);
	exit:
	bh1620fvc_i2c_client = NULL;           
	#ifdef MT6516        
	MT6516_EINTIRQMask(CUST_EINT_ALS_NUM);  /*mask interrupt if fail*/
	#endif
	APS_ERR("%s: err = %d\n", __func__, err);
	return err;
}
/*----------------------------------------------------------------------------*/
static int bh1620fvc_i2c_remove(struct i2c_client *client)
{
	int err;	
	
	if((err = bh1620fvc_delete_attr(&bh1620fvc_i2c_driver.driver)))
	{
		APS_ERR("bh1620fvc_delete_attr fail: %d\n", err);
	} 

	if((err = misc_deregister(&bh1620fvc_device)))
	{
		APS_ERR("misc_deregister fail: %d\n", err);    
	}
	
	bh1620fvc_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));

	return 0;
}
/*----------------------------------------------------------------------------*/
static int bh1620fvc_probe(struct platform_device *pdev) 
{
	struct alsps_hw *hw = get_cust_alsps_hw();
	struct bh1620fvc_i2c_addr addr;

	bh1620fvc_power(hw, 1);    
	bh1620fvc_get_addr(hw, &addr);
//	bh1620fvc_force[0] = hw->i2c_num;
//	bh1620fvc_force[1] = hw->i2c_addr[0];
	if(i2c_add_driver(&bh1620fvc_i2c_driver))
	{
		APS_ERR("add driver error\n");
		return -1;
	} 
	return 0;
}
/*----------------------------------------------------------------------------*/
static int bh1620fvc_remove(struct platform_device *pdev)
{
	struct alsps_hw *hw = get_cust_alsps_hw();
	APS_FUN();    
	bh1620fvc_power(hw, 0);    
	i2c_del_driver(&bh1620fvc_i2c_driver);
	return 0;
}
/*----------------------------------------------------------------------------*/
static struct platform_driver bh1620fvc_alsps_driver = {
	.probe      = bh1620fvc_probe,
	.remove     = bh1620fvc_remove,    
	.driver     = {
		.name  = "als_ps",
//		.owner = THIS_MODULE,
	}
};
/*----------------------------------------------------------------------------*/
static int __init bh1620fvc_init(void)
{
    struct alsps_hw *hw = get_cust_alsps_hw();
	APS_FUN();	
	APS_LOG("%s: i2c_number=%d\n", __func__,hw->i2c_num); 
	i2c_register_board_info(hw->i2c_num, &i2c_bh1620fvc, 1);
	if(platform_driver_register(&bh1620fvc_alsps_driver))
	{
		APS_ERR("failed to register driver");
		return -ENODEV;
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit bh1620fvc_exit(void)
{
	APS_FUN();
	platform_driver_unregister(&bh1620fvc_alsps_driver);
}
/*----------------------------------------------------------------------------*/
module_init(bh1620fvc_init);
module_exit(bh1620fvc_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("TC Chu");
MODULE_DESCRIPTION("BH1620FVC ALS driver");
MODULE_LICENSE("GPL");
