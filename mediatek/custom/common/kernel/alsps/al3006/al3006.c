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
#include <asm/io.h>
#include <cust_eint.h>
#include <cust_alsps.h>
#include "al3006.h"
#include <linux/hwmsen_helper.h>

#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>


extern void mt65xx_eint_unmask(unsigned int line);
extern void mt65xx_eint_mask(unsigned int line);
extern void mt65xx_eint_set_polarity(unsigned int eint_num, unsigned int pol);
extern void mt65xx_eint_set_hw_debounce(unsigned int eint_num, unsigned int ms);
extern unsigned int mt65xx_eint_set_sens(unsigned int eint_num, unsigned int sens);
extern void mt65xx_eint_registration(unsigned int eint_num, unsigned int is_deb_en, unsigned int pol, void (EINT_FUNC_PTR)(void), unsigned int is_auto_umask);
/*-------------------------MT6516&MT6573 define-------------------------------*/

#define POWER_NONE_MACRO MT65XX_POWER_NONE
/******************************************************************************
 * configuration
*******************************************************************************/
#define I2C_DRIVERID_AL3006 3006
/*----------------------------------------------------------------------------*/
#define AL3006_I2C_ADDR_RAR 0   /*!< the index in obj->hw->i2c_addr: alert response address */
#define AL3006_I2C_ADDR_ALS 1   /*!< the index in obj->hw->i2c_addr: ALS address */
#define AL3006_I2C_ADDR_PS  2   /*!< the index in obj->hw->i2c_addr: PS address */
#define AL3006_DEV_NAME     "AL3006"
/*----------------------------------------------------------------------------*/
#define APS_TAG                  "[ALS/PS] "
#define APS_FUN(f)               printk(KERN_INFO APS_TAG"%s\n", __FUNCTION__)
#define APS_ERR(fmt, args...)    printk(KERN_ERR  APS_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define APS_LOG(fmt, args...)    printk(KERN_INFO APS_TAG fmt, ##args)
#define APS_DBG(fmt, args...)    printk(KERN_INFO fmt, ##args)                 
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
static struct i2c_client *al3006_i2c_client = NULL;
/*----------------------------------------------------------------------------*/
static const struct i2c_device_id al3006_i2c_id[] = {{AL3006_DEV_NAME,0},{}};
static const struct i2c_board_info __initdata i2c_AL3006= {I2C_BOARD_INFO("AL3006",(0X38>>1))};
/*the adapter id & i2c address will be available in customization*/
//static unsigned short al3006_force[] = {0x00, AL3006_I2C_SLAVE_ADDR, I2C_CLIENT_END, I2C_CLIENT_END};
//static const unsigned short *const al3006_forces[] = { al3006_force, NULL };
//static struct i2c_client_address_data al3006_addr_data = { .forces = al3006_forces,};
/*----------------------------------------------------------------------------*/
static int al3006_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id); 
static int al3006_i2c_remove(struct i2c_client *client);
static int al3006_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);
/*----------------------------------------------------------------------------*/
static int al3006_i2c_suspend(struct i2c_client *client, pm_message_t msg);
static int al3006_i2c_resume(struct i2c_client *client);


static struct al3006_priv *g_al3006_ptr = NULL;

/*----------------------------------------------------------------------------*/
typedef enum {
    CMC_TRC_APS_DATA = 0x0002,
    CMC_TRC_EINT    = 0x0004,
    CMC_TRC_IOCTL   = 0x0008,
    CMC_TRC_I2C     = 0x0010,
    CMC_TRC_CVT_ALS = 0x0020,
    CMC_TRC_CVT_PS  = 0x0040,
    CMC_TRC_DEBUG   = 0x8000,
} CMC_TRC;
/*----------------------------------------------------------------------------*/
typedef enum {
    CMC_BIT_ALS    = 1,
    CMC_BIT_PS     = 2,
} CMC_BIT;
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
struct al3006_priv {
    struct alsps_hw  *hw;
    struct i2c_client *client;
    struct delayed_work  eint_work;
	//struct timer_list   first_read_ps_timer;
	//struct timer_list   first_read_als_timer;
    
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
    u8         als;
    u8          ps;
    u8          _align;
    u16         als_level_num;
    u16         als_value_num;
    u32         als_level[C_CUST_ALS_LEVEL-1];
    u32         als_value[C_CUST_ALS_LEVEL];

    bool    als_enable;    /*record current als status*/
	unsigned int    als_widow_loss; 
	
    bool    ps_enable;     /*record current ps status*/
    unsigned int    ps_thd_val;     /*the cmd value can't be read, stored in ram*/
    ulong       enable;         /*record HAL enalbe status*/
    ulong       pending_intr;   /*pending interrupt*/
    //ulong        first_read;   // record first read ps and als
    /*early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
    struct early_suspend    early_drv;
#endif     
};
/*----------------------------------------------------------------------------*/
static struct i2c_driver al3006_i2c_driver = {	
	.probe      = al3006_i2c_probe,
	.remove     = al3006_i2c_remove,
//	.detect     = al3006_i2c_detect,
	.suspend    = al3006_i2c_suspend,
	.resume     = al3006_i2c_resume,
	.id_table   = al3006_i2c_id,
//	.address_data = &al3006_addr_data,
	.driver = {
//		.owner          = THIS_MODULE,
		.name           = AL3006_DEV_NAME,
	},
};

static struct al3006_priv *al3006_obj = NULL;
static struct platform_driver al3006_alsps_driver;

static int al3006_get_ps_value(struct al3006_priv *obj, u8 ps);
static int al3006_get_als_value(struct al3006_priv *obj, u8 als);

/*----------------------------------------------------------------------------*/
static int hwmsen_read_byte_sr(struct i2c_client *client, u8 addr, u8 *data)
{
   u8 buf;
    int ret = 0;
	
    client->addr = (client->addr & I2C_MASK_FLAG) | I2C_WR_FLAG |I2C_RS_FLAG;
    buf = addr;
	ret = i2c_master_send(client, (const char*)&buf, 1<<8 | 1);
    //ret = i2c_master_send(client, (const char*)&buf, 1);
    if (ret < 0) {
        APS_ERR("send command error!!\n");
        return -EFAULT;
    }

    *data = buf;
	client->addr = client->addr& I2C_MASK_FLAG;
    return 0;
}

static void al3006_dumpReg(struct i2c_client *client)
{
  int i=0;
  u8 addr = 0x00;
  u8 regdata=0;
  for(i=0; i<9 ; i++)
  {
    //dump all
    hwmsen_read_byte_sr(client,addr,&regdata);
	APS_LOG("Reg addr=%x regdata=%x\n",addr,regdata);
	//snprintf(buf,1,"%c",regdata);
	addr++;
	if(addr == 0x06)
		addr=addr+0x02;
	if(addr > 0x08)
		break;
  }
}
/*
void al3006_first_read_ps_func()
{
	
	if (al3006_obj != NULL)
	{
		schedule_work(&al3006_obj->eint_work);
	}
	
	if(al3006_obj->first_read& 0x04 )
	{
	  mod_timer(&al3006_obj->first_read_ps_timer, jiffies + 20); 
	}
}

void al3006_first_read_als_func()
{
	
	if (al3006_obj != NULL)
	{
		schedule_work(&al3006_obj->eint_work);
	}
	
	if(al3006_obj->first_read&0x02)
	{
	  mod_timer(&al3006_obj->first_read_als_timer, jiffies + 20); 
	}
}

*/
/*----------------------------------------------------------------------------*/
int al3006_get_timing(void)
{
return 200;
/*
	u32 base = I2C2_BASE; 
	return (__raw_readw(mt6516_I2C_HS) << 16) | (__raw_readw(mt6516_I2C_TIMING));
*/
}
/*----------------------------------------------------------------------------*/
/*
int al3006_config_timing(int sample_div, int step_div)
{
	u32 base = I2C2_BASE; 
	unsigned long tmp;

	tmp  = __raw_readw(mt6516_I2C_TIMING) & ~((0x7 << 8) | (0x1f << 0));
	tmp  = (sample_div & 0x7) << 8 | (step_div & 0x1f) << 0 | tmp;

	return (__raw_readw(mt6516_I2C_HS) << 16) | (tmp);
}*/
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
int al3006_read_data(struct i2c_client *client, u8 *data)
{
	struct al3006_priv *obj = i2c_get_clientdata(client);    
	int ret = 0;
	//u8 aps_data=0;
	//u8 addr = 0x00;

	//hwmsen_read_byte_sr(client,APS_BOTH_DATA,&aps_data);
	if(hwmsen_read_byte_sr(client,APS_BOTH_DATA,data))
	{
		APS_ERR("reads aps data = %d\n", ret);
		return -EFAULT;
	}
	
	if(atomic_read(&obj->trace) & CMC_TRC_APS_DATA)
	{
		APS_DBG("APS:  0x%04X\n", (u32)(*data));
	}
	return 0;    
}
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

int al3006_init_device(struct i2c_client *client)
{
	//struct al3006_priv *obj = i2c_get_clientdata(client);        
	APS_LOG("al3006_init_device.........\r\n");
	u8 buf =0;
	
	//refor to datasheet
	if(hwmsen_write_byte(client,APS_CONFIGUATION,0x0B))//powerdown & idle
	{
	  return -EFAULT;
	}
	if(hwmsen_write_byte(client,APS_TIMING_CTL,0x11))//integration cycle=4 &integration time =100ms
	{
	  return -EFAULT;
	}
    if(hwmsen_write_byte(client,APS_ALS_CTL,0xA0))//als 64 levels & low lux threshold
    {
      return -EFAULT;
    }
    if(hwmsen_write_byte(client,APS_PS_CTL,0x4A))//ps accuracy count=7 &ps threshold =A
    {
      return -EFAULT;
    }
    if(hwmsen_write_byte(client,APS_ALS_WINDOW,0x00)) //window loss 0;
    {
      return -EFAULT;
    }
    if(hwmsen_read_byte_sr(client,APS_BOTH_DATA,&buf))
    {
      return -EFAULT;
    }
	//power up & enalbe both
	if(hwmsen_write_byte(client,APS_CONFIGUATION,0x03)) // powerup & idle
	{
	  return -EFAULT;
	}
	return 0;
}

/*----------------------------------------------------------------------------*/
int al3006_set_als_level(struct i2c_client *client, int level)
{
    int res =0;
	u8 data =0;
	res = hwmsen_read_byte_sr(client, APS_ALS_CTL, &data);
	if(res < 0)
	{
	   return -1;
	}
	data = data & 0x1f;//0001 1111 clear 7:5
	if(64 == level)
	{
	   data |= ALS_LEVEL_64;
	}
	else if(33 == level)
	{
	   data |= ALS_LEVEL_33;
	}
	else if(17 == level)
	{
	   data |= ALS_LEVEL_17;
	}
	else if(9 == level)
	{
	   data |= ALS_LEVEL_9;
	}
	else if(5 == level)
	{
	   data |= ALS_LEVEL_5;
	}
	else if(3 == level)
	{
	   //do nothing
	}
	else
	{
	   APS_ERR("do not support this level %d\n" ,level);
	   return -1;
	}

	res = hwmsen_write_byte(client,APS_ALS_CTL,data);
	if(res<0)
	{
	   return -1;
	}

	return res;
}
/*----------------------------------------------------------------------------*/
int al3006_set_low_lux(struct i2c_client *client, u8 low_lux)
{
    int res =0;
	u8 data =0;
	if(low_lux > 0x0f)
	{
	   APS_ERR("do not support this low_lux %d\n" ,low_lux);
	   return -1;
	}
	res = hwmsen_read_byte_sr(client, APS_ALS_CTL, &data);
	if(res < 0)
	{
	   return -1;
	}
	data = data & 0xe0; //1110 0000 clear 4:0
	
    data = data | low_lux;
	res = hwmsen_write_byte(client,APS_ALS_CTL,data);
	if(res < 0)
	{
	   return -1;
	}

	return res;
}

/*----------------------------------------------------------------------------*/
//loss from 0~15
/*----------------------------------------------------------------------------*/

int al3006_set_window_loss(struct i2c_client *client, unsigned int loss)
{
	int res =0;
	u8 data =0;
/*
	switch(loss)
	{
	case 0:
		data = 0x00;
		break;
	case 1:
		data = 0x01;
		break;
	case 2:
		data = 0x02;
		break;
	case 3:
		data = 0x03;
		break;
	case 1:
		data = 0x01;
		break;
	}
	*/
	APS_LOG("loss  %x\n" ,(u8)loss);
	
	res = hwmsen_write_byte(client, APS_ALS_WINDOW, (u8)loss);
	if(res < 0)
	{
	    return -1;
	}
	
	return res;
}
/*----------------------------------------------------------------------------*/

int al3006_set_ps_threshold(struct i2c_client *client, unsigned int threshold)
{
    int res =0;
	u8 data =0;
	
	if(threshold > 32)
	{
	   APS_ERR("do not support this ps threshold %d\n" ,threshold);
	   return -1;
	}
	res = hwmsen_read_byte_sr(client, APS_PS_CTL, &data);
	if(res < 0)
	{
	   return -1;
	}
	APS_LOG("threshold  %x\n" ,(u8)threshold);

	data = data & 0xE0; //1110 0000 clear 4:0
	
	data |= (u8)threshold;
	res = hwmsen_write_byte(client,APS_PS_CTL,data);
	if(res < 0)
	{
	   return -1;
	}
	
	return res;
   
}


/*----------------------------------------------------------------------------*/
static void al3006_power(struct alsps_hw *hw, unsigned int on) 
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
			if(!hwPowerOn(hw->power_id, hw->power_vol, "AL3006")) 
			{
				APS_ERR("power on fails!!\n");
			}
		}
		else
		{
			if(!hwPowerDown(hw->power_id, "AL3006")) 
			{
				APS_ERR("power off fail!!\n");   
			}
		}
	}
	power_on = on;
}
/*----------------------------------------------------------------------------*/
static int al3006_enable_als(struct i2c_client *client, bool enable)
{
    APS_LOG(" al3006_enable_als %d \n",enable); 
	struct al3006_priv *obj = i2c_get_clientdata(client);
	int err=0;
	int trc = atomic_read(&obj->trace);
	u8 regdata=0;
	if(enable == obj->als_enable)
	{
	   return 0;
	}
	

	if(hwmsen_read_byte_sr(client, APS_CONFIGUATION, &regdata))
	{
		APS_ERR("read APS_CONFIGUATION register err!\n");
		return -1;
	}
#if 0
	//
    //APS_LOG(" al3006_enable_als 00h=%x \n",regdata); 
    al3006_enable_ps(client, true);
	//
#endif
	regdata &= 0b11111100; //first set 00 to clear bit
	
	if(enable == TRUE)//enable als
	{
	     APS_LOG("first enable als!\n");
		 if(true == obj->ps_enable)
		 {
		   APS_LOG("ALS(1): enable both \n");
		   atomic_set(&obj->ps_deb_on, 1);
		   atomic_set(&obj->ps_deb_end, jiffies+atomic_read(&obj->ps_debounce)/(1000/HZ));
		   regdata |= 0b00000010; //enable both
		 }
		 if(false == obj->ps_enable)
		 {
		   APS_LOG("ALS(1): enable als only \n");
		   regdata &= 0b11111100; //only enable als
		 }
		 atomic_set(&obj->als_deb_on, 1);
		 atomic_set(&obj->als_deb_end, jiffies+atomic_read(&obj->als_debounce)/(1000/HZ));
		 set_bit(CMC_BIT_ALS,  &obj->pending_intr);
		 schedule_delayed_work(&obj->eint_work,230); //after enable the value is not accurate
		 APS_LOG("first enalbe als set pending interrupt %d\n",obj->pending_intr);
	}
	else
	{
		if(true == obj->ps_enable)
		 {
		   APS_LOG("ALS(0):enable ps only \n");
		   atomic_set(&obj->ps_deb_on, 1);
		   atomic_set(&obj->ps_deb_end, jiffies+atomic_read(&obj->ps_debounce)/(1000/HZ));
		   regdata |= 0b00000001;//only enable ps
		   set_bit(CMC_BIT_PS,  &obj->pending_intr);
		   schedule_delayed_work(&obj->eint_work,120);
		 }
		 if(false == obj->ps_enable)
		 {
		   APS_LOG("ALS(0): disable both \n");
		   regdata |= 0b00000011;//disable both
		 }
		 //del_timer_sync(&obj->first_read_als_timer);
	}
	

	if(hwmsen_write_byte(client,APS_CONFIGUATION,regdata))
	{
		APS_LOG("al3006_enable_als failed!\n");
		return -1;
	}
    obj->als_enable = enable;

#if 0
	if(hwmsen_read_byte_sr(client, APS_CONFIGUATION, &regdata))
	{
		APS_ERR("read APS_CONFIGUATION register err!\n");
		return -1;
	}
	//
	APS_LOG(" after al3006_enable_als 00h=%x \n",regdata);
#endif

	if(trc & CMC_TRC_DEBUG)
	{
		APS_LOG("enable als (%d)\n", enable);
	}

	return err;
}
/*----------------------------------------------------------------------------*/
static int al3006_enable_ps(struct i2c_client *client, bool enable)
{
    APS_LOG(" al3006_enable_ps %d\n",enable); 
	struct al3006_priv *obj = i2c_get_clientdata(client);
	int err=0;
	int trc = atomic_read(&obj->trace);
	u8 regdata=0;
	if(enable == obj->ps_enable)
	{
	   return 0;
	}
	

	if(hwmsen_read_byte_sr(client, APS_CONFIGUATION, &regdata))
	{
		APS_ERR("read APS_CONFIGUATION register err!\n");
		return -1;
	}

#if 0
	APS_LOG(" al3006_enable_ps 00h=%x \n",regdata); 
//
#endif
	regdata &= 0b11111100; //first set 00 to clear bit
	
	if(enable == TRUE)//enable ps
	{
	     APS_LOG("first enable ps!\n");
		 if(true == obj->als_enable)
		 {
		   regdata |= 0b00000010; //enable both
		   atomic_set(&obj->als_deb_on, 1);
		   atomic_set(&obj->als_deb_end, jiffies+atomic_read(&obj->als_debounce)/(1000/HZ));
		   APS_LOG("PS(1): enable ps both !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		 }
		 if(false == obj->als_enable)
		 {
		   regdata |= 0b00000001; //only enable ps
		   APS_LOG("PS(1): enable ps only !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		 }
		 atomic_set(&obj->ps_deb_on, 1);
		 atomic_set(&obj->ps_deb_end, jiffies+atomic_read(&obj->ps_debounce)/(1000/HZ));
		 set_bit(CMC_BIT_PS,  &obj->pending_intr);
		 schedule_delayed_work(&obj->eint_work,120);
		 APS_LOG("first enalbe ps set pending interrupt %d\n",obj->pending_intr);
	}
	else//disable ps
	{
		 if(true == obj->als_enable)
		 {
		   APS_LOG("PS(0): disable ps only enalbe als !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		   regdata &= 0b11111100;//only enable als
		   atomic_set(&obj->als_deb_on, 1);//add 
		   atomic_set(&obj->als_deb_end, jiffies+atomic_read(&obj->als_debounce)/(1000/HZ));
		   set_bit(CMC_BIT_ALS,  &obj->pending_intr);
		   schedule_delayed_work(&obj->eint_work,120);
		 }
		 if(false == obj->als_enable)
		 {
		   APS_LOG("PS(0): disable both !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		   regdata |= 0b00000011;//disable both
		 }
		 //del_timer_sync(&obj->first_read_ps_timer);
	}
	

	if(hwmsen_write_byte(client,APS_CONFIGUATION,regdata))
	{
		APS_LOG("al3006_enable_als failed!\n");
		return -1;
	}
	obj->ps_enable = enable;

#if 0
	if(hwmsen_read_byte_sr(client, APS_CONFIGUATION, &regdata))
	{
		APS_ERR("read APS_CONFIGUATION register err!\n");
		return -1;
	}
	//
	APS_LOG(" after al3006_enable_ps 00h=%x \n",regdata);
#endif
	
	if(trc & CMC_TRC_DEBUG)
	{
		APS_LOG("enable ps (%d)\n", enable);
	}

	return err;
}
/*----------------------------------------------------------------------------*/

static int al3006_check_intr(struct i2c_client *client) 
{
	struct al3006_priv *obj = i2c_get_clientdata(client);
	int err;
	u8 data=0;

	err = hwmsen_read_byte_sr(client,APS_INT_STATUS,&data);
	APS_LOG("INT flage: = %x\n", data);

	if(err)
	{
		APS_ERR("WARNING: read int status: %d\n", err);
		return 0;
	}
    
	if(data & 0x01)//als
	{
		set_bit(CMC_BIT_ALS, &obj->pending_intr);
	}
	else
	{
	   clear_bit(CMC_BIT_ALS, &obj->pending_intr);
	}
	
	if(data & 0x02)//ps
	{
		set_bit(CMC_BIT_PS,  &obj->pending_intr);
	}
	else
	{
	    clear_bit(CMC_BIT_PS, &obj->pending_intr);
	}
	
	if(atomic_read(&obj->trace) & CMC_TRC_DEBUG)
	{
		APS_LOG("check intr: 0x%08X\n", obj->pending_intr);
	}

	return 0;
}

/*----------------------------------------------------------------------------*/
void al3006_eint_func(void)
{
	struct al3006_priv *obj = g_al3006_ptr;
	APS_LOG("fwq interrupt fuc\n");
	if(!obj)
	{
		return;
	}
	
	schedule_delayed_work(&obj->eint_work,0);
	if(atomic_read(&obj->trace) & CMC_TRC_EINT)
	{
		APS_LOG("eint: als/ps intrs\n");
	}
}
/*----------------------------------------------------------------------------*/
static void al3006_eint_work(struct work_struct *work)
{
	struct al3006_priv *obj = (struct al3006_priv *)container_of(work, struct al3006_priv, eint_work);
	int err;
	hwm_sensor_data sensor_data;
	
	memset(&sensor_data, 0, sizeof(sensor_data));

	APS_LOG("al3006_eint_work\n");

	if(0 == atomic_read(&obj->ps_deb_on)) // first enable do not check interrupt
	{
	   err = al3006_check_intr(obj->client);
	}
	
	if(err)
	{
		APS_ERR("check intrs: %d\n", err);
	}

    APS_LOG("al3006_eint_work &obj->pending_intr =%d\n",obj->pending_intr);
	
	if((1<<CMC_BIT_ALS) & obj->pending_intr)
	{
	  //get raw data
	  APS_LOG("fwq als INT\n");
	  if(err = al3006_read_data(obj->client, &obj->als))
	  {
		 APS_ERR("al3006 read als data: %d\n", err);;
	  }
	  //map and store data to hwm_sensor_data
	  while(-1 == al3006_get_als_value(obj, obj->als))
	  {
		 al3006_read_data(obj->client, &obj->als);
		 msleep(50);
	  }
 	  sensor_data.values[0] = al3006_get_als_value(obj, obj->als);
	  sensor_data.value_divide = 1;
	  sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
	  //let up layer to know
	  if(err = hwmsen_get_interrupt_data(ID_LIGHT, &sensor_data))
	  {
		APS_ERR("call hwmsen_get_interrupt_data fail = %d\n", err);
	  }
	  
	}
	if((1<<CMC_BIT_PS) &  obj->pending_intr)
	{
	  //get raw data
	  APS_LOG("fwq ps INT\n");
	  if(err = al3006_read_data(obj->client, &obj->ps))
	  {
		 APS_ERR("al3006 read ps data: %d\n", err);;
	  }
	  //map and store data to hwm_sensor_data
	  while(-1 == al3006_get_ps_value(obj, obj->ps))
	  {
		 al3006_read_data(obj->client, &obj->ps);
		 msleep(50);
		 APS_LOG("al3006 read ps data delay\n");;
	  }
	  sensor_data.values[0] = al3006_get_ps_value(obj, obj->ps);
	  sensor_data.value_divide = 1;
	  sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
	  //let up layer to know
	  APS_LOG("al3006 read ps data  = %d \n",sensor_data.values[0]);
	  if(err = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data))
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
	#ifdef MT6577
	mt65xx_eint_unmask(CUST_EINT_ALS_NUM);  
	#endif
}

/*----------------------------------------------------------------------------*/
int al3006_setup_eint(struct i2c_client *client)
{
	struct al3006_priv *obj = i2c_get_clientdata(client);        

	g_al3006_ptr = obj;
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
	MT6516_EINT_Registration(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_EN, CUST_EINT_ALS_POLARITY, al3006_eint_func, 0);
	MT6516_EINTIRQUnmask(CUST_EINT_ALS_NUM);  
#endif
    //
#ifdef MT6573
	
    mt65xx_eint_set_sens(CUST_EINT_ALS_NUM, CUST_EINT_ALS_SENSITIVE);
	mt65xx_eint_set_polarity(CUST_EINT_ALS_NUM, CUST_EINT_ALS_POLARITY);
	mt65xx_eint_set_hw_debounce(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_CN);
	mt65xx_eint_registration(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_EN, CUST_EINT_ALS_POLARITY, al3006_eint_func, 0);
	mt65xx_eint_unmask(CUST_EINT_ALS_NUM);  
#endif  

#ifdef MT6575
		
		mt65xx_eint_set_sens(CUST_EINT_ALS_NUM, CUST_EINT_ALS_SENSITIVE);
		mt65xx_eint_set_polarity(CUST_EINT_ALS_NUM, CUST_EINT_ALS_POLARITY);
		mt65xx_eint_set_hw_debounce(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_CN);
		mt65xx_eint_registration(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_EN, CUST_EINT_ALS_POLARITY, al3006_eint_func, 0);
		mt65xx_eint_unmask(CUST_EINT_ALS_NUM);	
#endif  
#ifdef MT6577
		
		mt65xx_eint_set_sens(CUST_EINT_ALS_NUM, CUST_EINT_ALS_SENSITIVE);
		mt65xx_eint_set_polarity(CUST_EINT_ALS_NUM, CUST_EINT_ALS_POLARITY);
		mt65xx_eint_set_hw_debounce(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_CN);
		mt65xx_eint_registration(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_EN, CUST_EINT_ALS_POLARITY, al3006_eint_func, 0);
		mt65xx_eint_unmask(CUST_EINT_ALS_NUM);	
#endif  


    return 0;
}

/*----------------------------------------------------------------------------*/
static int al3006_init_client(struct i2c_client *client)
{
	struct al3006_priv *obj = i2c_get_clientdata(client);
	int err=0;
	APS_LOG("al3006_init_client.........\r\n");

	if((err = al3006_setup_eint(client)))
	{
		APS_ERR("setup eint: %d\n", err);
		return err;
	}
	
	
	if((err = al3006_init_device(client)))
	{
		APS_ERR("init dev: %d\n", err);
		return err;
	}

	//set window loss
	if(err = al3006_set_window_loss(client,(unsigned int)(obj->als_widow_loss)))
	{
	  APS_ERR("set window loss error %d\n", err);
	  return err;
	}
	//set ps threshold
	if(err = al3006_set_ps_threshold(client,(unsigned int)(obj->ps_thd_val)))
	{
	  APS_ERR("set ps threshold error %d\n", err);
	  return err;
	}
	
	
	return err;
}
/******************************************************************************
 * Sysfs attributes
*******************************************************************************/
static ssize_t al3006_show_config(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	
	if(!al3006_obj)
	{
		APS_ERR("al3006_obj is null!!\n");
		return 0;
	}
	
	res = snprintf(buf, PAGE_SIZE, "(%d %d %d %d %d)\n", 
		atomic_read(&al3006_obj->i2c_retry), atomic_read(&al3006_obj->als_debounce), 
		atomic_read(&al3006_obj->ps_mask), al3006_obj->ps_thd_val, atomic_read(&al3006_obj->ps_debounce));     
	return res;    
}
/*----------------------------------------------------------------------------*/
static ssize_t al3006_store_config(struct device_driver *ddri, char *buf, size_t count)
{
	int retry, als_deb, ps_deb, mask, thres;
	if(!al3006_obj)
	{
		APS_ERR("al3006_obj is null!!\n");
		return 0;
	}
	
	if(5 == sscanf(buf, "%d %d %d %d %d", &retry, &als_deb, &mask, &thres, &ps_deb))
	{ 
		atomic_set(&al3006_obj->i2c_retry, retry);
		atomic_set(&al3006_obj->als_debounce, als_deb);
		atomic_set(&al3006_obj->ps_mask, mask);
		al3006_obj->ps_thd_val= thres;        
		atomic_set(&al3006_obj->ps_debounce, ps_deb);
	}
	else
	{
		APS_ERR("invalid content: '%s', length = %d\n", buf, count);
	}
	return count;    
}
/*----------------------------------------------------------------------------*/
static ssize_t al3006_show_trace(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	if(!al3006_obj)
	{
		APS_ERR("al3006_obj is null!!\n");
		return 0;
	}

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&al3006_obj->trace));     
	return res;    
}
/*----------------------------------------------------------------------------*/
static ssize_t al3006_store_trace(struct device_driver *ddri, char *buf, size_t count)
{
    int trace;
    if(!al3006_obj)
	{
		APS_ERR("al3006_obj is null!!\n");
		return 0;
	}
	
	if(1 == sscanf(buf, "0x%x", &trace))
	{
		atomic_set(&al3006_obj->trace, trace);
	}
	else 
	{
		APS_ERR("invalid content: '%s', length = %d\n", buf, count);
	}
	return count;    
}
/*----------------------------------------------------------------------------*/
static ssize_t al3006_show_als(struct device_driver *ddri, char *buf)
{
	int res;
	u8 dat = 0;
	
	if(!al3006_obj)
	{
		APS_ERR("al3006_obj is null!!\n");
		return 0;
	}
	if(res = al3006_read_data(al3006_obj->client, &al3006_obj->als))
	{
		return snprintf(buf, PAGE_SIZE, "ERROR: %d\n", res);
	}
	else
	{   dat = al3006_obj->als & 0x3f;
		return snprintf(buf, PAGE_SIZE, "0x%04X\n", dat);     
	}
}
/*----------------------------------------------------------------------------*/
static ssize_t al3006_show_ps(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	u8 dat=0;
	if(!al3006_obj)
	{
		APS_ERR("al3006_obj is null!!\n");
		return 0;
	}
	
	if(res = al3006_read_data(al3006_obj->client, &al3006_obj->ps))
	{
		return snprintf(buf, PAGE_SIZE, "ERROR: %d\n", res);
	}
	else
	{
	    dat = al3006_obj->ps & 0x80;
		return snprintf(buf, PAGE_SIZE, "0x%04X\n", dat);     
	}
}
/*----------------------------------------------------------------------------*/
static ssize_t al3006_show_reg(struct device_driver *ddri, char *buf)
{
	if(!al3006_obj)
	{
		APS_ERR("al3006_obj is null!!\n");
		return 0;
	}
	
	/*read*/
	al3006_dumpReg(al3006_obj->client);
	
	return 0;
}

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static ssize_t al3006_show_status(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	
	if(!al3006_obj)
	{
		APS_ERR("al3006_obj is null!!\n");
		return 0;
	}
	
	if(al3006_obj->hw)
	{
	
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: %d, (%d %d)\n", 
			al3006_obj->hw->i2c_num, al3006_obj->hw->power_id, al3006_obj->hw->power_vol);
		
	}
	else
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");
	}

	#ifdef MT6516
	len += snprintf(buf+len, PAGE_SIZE-len, "EINT: %d (%d %d %d %d)\n", mt_get_gpio_in(GPIO_ALS_EINT_PIN),
				CUST_EINT_ALS_NUM, CUST_EINT_ALS_POLARITY, CUST_EINT_ALS_DEBOUNCE_EN, CUST_EINT_ALS_DEBOUNCE_CN);

	len += snprintf(buf+len, PAGE_SIZE-len, "GPIO: %d (%d %d %d %d)\n",	GPIO_ALS_EINT_PIN, 
				mt_get_gpio_dir(GPIO_ALS_EINT_PIN), mt_get_gpio_mode(GPIO_ALS_EINT_PIN), 
				mt_get_gpio_pull_enable(GPIO_ALS_EINT_PIN), mt_get_gpio_pull_select(GPIO_ALS_EINT_PIN));
	#endif

	len += snprintf(buf+len, PAGE_SIZE-len, "MISC: %d %d\n", atomic_read(&al3006_obj->als_suspend), atomic_read(&al3006_obj->ps_suspend));

	return len;
}
/*----------------------------------------------------------------------------*/
static ssize_t al3006_show_i2c(struct device_driver *ddri, char *buf)
{
/*	ssize_t len = 0;
	u32 base = I2C2_BASE;

	if(!al3006_obj)
	{
		APS_ERR("al3006_obj is null!!\n");
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

	return len;*/
	return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t al3006_store_i2c(struct device_driver *ddri, char *buf, size_t count)
{
/*	int sample_div, step_div;
	unsigned long tmp;
	u32 base = I2C2_BASE;    

	if(!al3006_obj)
	{
		APS_ERR("al3006_obj is null!!\n");
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
static int read_int_from_buf(struct al3006_priv *obj, const char* buf, size_t count,
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
static ssize_t al3006_show_alslv(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	int idx;
	if(!al3006_obj)
	{
		APS_ERR("al3006_obj is null!!\n");
		return 0;
	}
	
	for(idx = 0; idx < al3006_obj->als_level_num; idx++)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "%d ", al3006_obj->hw->als_level[idx]);
	}
	len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	return len;    
}
/*----------------------------------------------------------------------------*/
static ssize_t al3006_store_alslv(struct device_driver *ddri, char *buf, size_t count)
{
	struct al3006_priv *obj;
	if(!al3006_obj)
	{
		APS_ERR("al3006_obj is null!!\n");
		return 0;
	}
	else if(!strcmp(buf, "def"))
	{
		memcpy(al3006_obj->als_level, al3006_obj->hw->als_level, sizeof(al3006_obj->als_level));
	}
	else if(al3006_obj->als_level_num != read_int_from_buf(al3006_obj, buf, count, 
			al3006_obj->hw->als_level, al3006_obj->als_level_num))
	{
		APS_ERR("invalid format: '%s'\n", buf);
	}    
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t al3006_show_alsval(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	int idx;
	if(!al3006_obj)
	{
		APS_ERR("al3006_obj is null!!\n");
		return 0;
	}
	
	for(idx = 0; idx < al3006_obj->als_value_num; idx++)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "%d ", al3006_obj->hw->als_value[idx]);
	}
	len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	return len;    
}
/*----------------------------------------------------------------------------*/
static ssize_t al3006_store_alsval(struct device_driver *ddri, char *buf, size_t count)
{
	if(!al3006_obj)
	{
		APS_ERR("al3006_obj is null!!\n");
		return 0;
	}
	else if(!strcmp(buf, "def"))
	{
		memcpy(al3006_obj->als_value, al3006_obj->hw->als_value, sizeof(al3006_obj->als_value));
	}
	else if(al3006_obj->als_value_num != read_int_from_buf(al3006_obj, buf, count, 
			al3006_obj->hw->als_value, al3006_obj->als_value_num))
	{
		APS_ERR("invalid format: '%s'\n", buf);
	}    
	return count;
}

/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(als,     S_IWUSR | S_IRUGO, al3006_show_als,   NULL);
static DRIVER_ATTR(ps,      S_IWUSR | S_IRUGO, al3006_show_ps,    NULL);
static DRIVER_ATTR(config,  S_IWUSR | S_IRUGO, al3006_show_config,al3006_store_config);
static DRIVER_ATTR(alslv,   S_IWUSR | S_IRUGO, al3006_show_alslv, al3006_store_alslv);
static DRIVER_ATTR(alsval,  S_IWUSR | S_IRUGO, al3006_show_alsval,al3006_store_alsval);
static DRIVER_ATTR(trace,   S_IWUSR | S_IRUGO, al3006_show_trace, al3006_store_trace);
static DRIVER_ATTR(status,  S_IWUSR | S_IRUGO, al3006_show_status,  NULL);
static DRIVER_ATTR(reg,     S_IWUSR | S_IRUGO, al3006_show_reg,   NULL);
static DRIVER_ATTR(i2c,     S_IWUSR | S_IRUGO, al3006_show_i2c,   al3006_store_i2c);
/*----------------------------------------------------------------------------*/
static struct device_attribute *al3006_attr_list[] = {
    &driver_attr_als,
    &driver_attr_ps,    
    &driver_attr_trace,        /*trace log*/
    &driver_attr_config,
    &driver_attr_alslv,
    &driver_attr_alsval,
    &driver_attr_status,
    &driver_attr_i2c,
    &driver_attr_reg,
};
/*----------------------------------------------------------------------------*/
static int al3006_create_attr(struct device_driver *driver) 
{
	int idx, err = 0;
	int num = (int)(sizeof(al3006_attr_list)/sizeof(al3006_attr_list[0]));
	if (driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
		if(err = driver_create_file(driver, al3006_attr_list[idx]))
		{            
			APS_ERR("driver_create_file (%s) = %d\n", al3006_attr_list[idx]->attr.name, err);
			break;
		}
	}    
	return err;
}
/*----------------------------------------------------------------------------*/
	static int al3006_delete_attr(struct device_driver *driver)
	{
	int idx ,err = 0;
	int num = (int)(sizeof(al3006_attr_list)/sizeof(al3006_attr_list[0]));

	if (!driver)
	return -EINVAL;

	for (idx = 0; idx < num; idx++) 
	{
		driver_remove_file(driver, al3006_attr_list[idx]);
	}
	
	return err;
}
/****************************************************************************** 
 * Function Configuration
******************************************************************************/
static int al3006_get_als_value(struct al3006_priv *obj, u8 als)
{
	int idx;
	int invalid = 0;
	als = als & 0x3f;
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
			//clear_bit(CMC_BIT_ALS, &obj->first_read);
		}
		
		if(1 == atomic_read(&obj->als_deb_on))
		{
			invalid = 1;
		}
	}

	if(!invalid)
	{
		if (atomic_read(&obj->trace) & CMC_TRC_CVT_ALS)
		{
			APS_DBG("ALS: %05d => %05d\n", als, obj->hw->als_value[idx]);
		}
		
		return obj->hw->als_value[idx];
	}
	else
	{
		if(atomic_read(&obj->trace) & CMC_TRC_CVT_ALS)
		{
			APS_DBG("ALS: %05d => %05d (-1)\n", als, obj->hw->als_value[idx]);    
		}
		return -1;
	}
}
/*----------------------------------------------------------------------------*/

static int al3006_get_ps_value(struct al3006_priv *obj, u8 ps)
{
  
    int val= -1;
	int invalid = 0;

	if(0x80 & ps)
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
			//clear_bit(CMC_BIT_PS, &obj->first_read);
		}
		
		if (1 == atomic_read(&obj->ps_deb_on))
		{
			invalid = 1;
		}
	}

	if(!invalid)
	{
		if(unlikely(atomic_read(&obj->trace) & CMC_TRC_CVT_PS))
		{
		   APS_DBG("PS:  %05d => %05d\n", ps, val);
		}
		return val;
		
	}	
	else
	{
		if(unlikely(atomic_read(&obj->trace) & CMC_TRC_CVT_PS))
		{
			APS_DBG("PS:  %05d => %05d (-1)\n", ps, val);    
		}
		return -1;
	}	
	
}

/****************************************************************************** 
 * Function Configuration
******************************************************************************/
static int al3006_open(struct inode *inode, struct file *file)
{
	file->private_data = al3006_i2c_client;

	if (!file->private_data)
	{
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}
	
	return nonseekable_open(inode, file);
}
/*----------------------------------------------------------------------------*/
static int al3006_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}
/*----------------------------------------------------------------------------*/
static int al3006_unlocked_ioctl(struct file *file, unsigned int cmd,
       unsigned long arg)
{
	struct i2c_client *client = (struct i2c_client*)file->private_data;
	struct al3006_priv *obj = i2c_get_clientdata(client);  
	int err = 0;
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
			if(enable)
			{
				if(err = al3006_enable_ps(obj->client, true))
				{
					APS_ERR("enable ps fail: %d\n", err); 
					goto err_out;
				}
				
				set_bit(CMC_BIT_PS, &obj->enable);
			}
			else
			{
				if(err = al3006_enable_ps(obj->client, false))
				{
					APS_ERR("disable ps fail: %d\n", err); 
					goto err_out;
				}
				
				clear_bit(CMC_BIT_PS, &obj->enable);
			}
			break;

		case ALSPS_GET_PS_MODE:
			enable = test_bit(CMC_BIT_PS, &obj->enable) ? (1) : (0);
			if(copy_to_user(ptr, &enable, sizeof(enable)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;

		case ALSPS_GET_PS_DATA:    
			if(err = al3006_read_data(obj->client, &obj->ps))
			{
				goto err_out;
			}
			dat = al3006_get_ps_value(obj, obj->ps);
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}  
			break;

		case ALSPS_GET_PS_RAW_DATA:    
			if(err = al3006_read_data(obj->client, &obj->ps))
			{
				goto err_out;
			}
			
			dat = obj->ps & 0x80;
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
				if(err = al3006_enable_als(obj->client, true))
				{
					APS_ERR("enable als fail: %d\n", err); 
					goto err_out;
				}
				set_bit(CMC_BIT_ALS, &obj->enable);
			}
			else
			{
				if(err = al3006_enable_als(obj->client, false))
				{
					APS_ERR("disable als fail: %d\n", err); 
					goto err_out;
				}
				clear_bit(CMC_BIT_ALS, &obj->enable);
			}
			break;

		case ALSPS_GET_ALS_MODE:
			enable = test_bit(CMC_BIT_ALS, &obj->enable) ? (1) : (0);
			if(copy_to_user(ptr, &enable, sizeof(enable)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;

		case ALSPS_GET_ALS_DATA: 
			if(err = al3006_read_data(obj->client, &obj->als))
			{
				goto err_out;
			}

			dat = al3006_get_als_value(obj, obj->als);
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}              
			break;

		case ALSPS_GET_ALS_RAW_DATA:    
			if(err = al3006_read_data(obj->client, &obj->als))
			{
				goto err_out;
			}

			dat = obj->als & 0x3f;
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
static struct file_operations al3006_fops = {
//	.owner = THIS_MODULE,
	.open = al3006_open,
	.release = al3006_release,
	.unlocked_ioctl = al3006_unlocked_ioctl,
};
/*----------------------------------------------------------------------------*/
static struct miscdevice al3006_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "als_ps",
	.fops = &al3006_fops,
};
/*----------------------------------------------------------------------------*/
static int al3006_i2c_suspend(struct i2c_client *client, pm_message_t msg) 
{
	struct al3006_priv *obj = i2c_get_clientdata(client);    
	int err;
	APS_FUN();    

	if(msg.event == PM_EVENT_SUSPEND)
	{   
		if(!obj)
		{
			APS_ERR("null pointer!!\n");
			return -EINVAL;
		}
		
		atomic_set(&obj->als_suspend, 1);
		if(err = al3006_enable_als(client, false))
		{
			APS_ERR("disable als: %d\n", err);
			return err;
		}

		atomic_set(&obj->ps_suspend, 1);
		if(err = al3006_enable_ps(client, false))
		{
			APS_ERR("disable ps:  %d\n", err);
			return err;
		}
		
		al3006_power(obj->hw, 0);
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
static int al3006_i2c_resume(struct i2c_client *client)
{
	struct al3006_priv *obj = i2c_get_clientdata(client);        
	int err;
	APS_FUN();

	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}

	al3006_power(obj->hw, 1);
	if(err = al3006_init_client(client))
	{
		APS_ERR("initialize client fail!!\n");
		return err;        
	}
	atomic_set(&obj->als_suspend, 0);
	if(test_bit(CMC_BIT_ALS, &obj->enable))
	{
		if(err = al3006_enable_als(client, true))
		{
			APS_ERR("enable als fail: %d\n", err);        
		}
	}
	atomic_set(&obj->ps_suspend, 0);
	if(test_bit(CMC_BIT_PS,  &obj->enable))
	{
		if(err = al3006_enable_ps(client, true))
		{
			APS_ERR("enable ps fail: %d\n", err);                
		}
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
static void al3006_early_suspend(struct early_suspend *h) 
{   /*early_suspend is only applied for ALS*/
	struct al3006_priv *obj = container_of(h, struct al3006_priv, early_drv);   
	int err;
	APS_FUN();    

	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return;
	}
	
	atomic_set(&obj->als_suspend, 1);    
	if(err = al3006_enable_als(obj->client, false))
	{
		APS_ERR("disable als fail: %d\n", err); 
	}
}
/*----------------------------------------------------------------------------*/
static void al3006_late_resume(struct early_suspend *h)
{   /*early_suspend is only applied for ALS*/
	struct al3006_priv *obj = container_of(h, struct al3006_priv, early_drv);         
	int err;
	APS_FUN();

	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return;
	}

	atomic_set(&obj->als_suspend, 0);
	if(test_bit(CMC_BIT_ALS, &obj->enable))
	{
		if(err = al3006_enable_als(obj->client, true))
		{
			APS_ERR("enable als fail: %d\n", err);        

		}
	}
}

int al3006_ps_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	hwm_sensor_data* sensor_data;
	struct al3006_priv *obj = (struct al3006_priv *)self;
	
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
					if(err = al3006_enable_ps(obj->client, true))
					{
						APS_ERR("enable ps fail: %d\n", err); 
						return -1;
					}
					set_bit(CMC_BIT_PS, &obj->enable);
				}
				else
				{
					if(err = al3006_enable_ps(obj->client, false))
					{
						APS_ERR("disable ps fail: %d\n", err); 
						return -1;
					}
					clear_bit(CMC_BIT_PS, &obj->enable);
				}
			}
			break;

		case SENSOR_GET_DATA:
			//APS_LOG("fwq get ps data !!!!!!\n");
			if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
			{
				APS_ERR("get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				sensor_data = (hwm_sensor_data *)buff_out;				
				
				if(err = al3006_read_data(obj->client, &obj->ps))
				{
					err = -1;;
				}
				else
				{
				    while(-1 == al3006_get_ps_value(obj, obj->ps))
				    {
				      al3006_read_data(obj->client, &obj->ps);
				      msleep(50);
				    }
				   
					sensor_data->values[0] = al3006_get_ps_value(obj, obj->ps);
					sensor_data->value_divide = 1;
					sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
					//APS_LOG("fwq get ps data =%d\n",sensor_data->values[0]);
				    
					
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

int al3006_als_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	hwm_sensor_data* sensor_data;
	struct al3006_priv *obj = (struct al3006_priv *)self;
	
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
					if(err = al3006_enable_als(obj->client, true))
					{
						APS_ERR("enable als fail: %d\n", err); 
						return -1;
					}
					set_bit(CMC_BIT_ALS, &obj->enable);
				}
				else
				{
					if(err = al3006_enable_als(obj->client, false))
					{
						APS_ERR("disable als fail: %d\n", err); 
						return -1;
					}
					clear_bit(CMC_BIT_ALS, &obj->enable);
				}
				
			}
			break;

		case SENSOR_GET_DATA:
			//APS_LOG("fwq get als data !!!!!!\n");
			if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
			{
				APS_ERR("get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				sensor_data = (hwm_sensor_data *)buff_out;
								
				if(err = al3006_read_data(obj->client, &obj->als))
				{
					err = -1;;
				}
				else
				{
				#if defined(MTK_AAL_SUPPORT)
				sensor_data->values[0] = obj->als;
				#else
				    while(-1 == al3006_get_als_value(obj, obj->als))
				    {
				      al3006_read_data(obj->client, &obj->als);
				      msleep(50);
				    }
					sensor_data->values[0] = al3006_get_als_value(obj, obj->als);
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
static int al3006_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info) 
{    
	strcpy(info->type, AL3006_DEV_NAME);
	return 0;
}

/*----------------------------------------------------------------------------*/
static int al3006_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    APS_FUN();
	struct al3006_priv *obj;
	struct hwmsen_object obj_ps, obj_als;
	int err = 0;

	if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
	{
		err = -ENOMEM;
		goto exit;
	}
	memset(obj, 0, sizeof(*obj));
	al3006_obj = obj;

	obj->hw = get_cust_alsps_hw();


	INIT_DELAYED_WORK(&obj->eint_work, al3006_eint_work);
	obj->client = client;
	i2c_set_clientdata(client, obj);	
	atomic_set(&obj->als_debounce, 1000);
	atomic_set(&obj->als_deb_on, 0);
	atomic_set(&obj->als_deb_end, 0);
	atomic_set(&obj->ps_debounce, 1000);
	atomic_set(&obj->ps_deb_on, 0);
	atomic_set(&obj->ps_deb_end, 0);
	atomic_set(&obj->ps_mask, 0);
	atomic_set(&obj->trace, 0x00);
	atomic_set(&obj->als_suspend, 0);

	obj->ps_enable = 0;
	obj->als_enable = 0;
	obj->enable = 0;
	obj->pending_intr = 0;
	obj->als_level_num = sizeof(obj->hw->als_level)/sizeof(obj->hw->als_level[0]);
	obj->als_value_num = sizeof(obj->hw->als_value)/sizeof(obj->hw->als_value[0]);   
	BUG_ON(sizeof(obj->als_level) != sizeof(obj->hw->als_level));
	memcpy(obj->als_level, obj->hw->als_level, sizeof(obj->als_level));
	BUG_ON(sizeof(obj->als_value) != sizeof(obj->hw->als_value));
	memcpy(obj->als_value, obj->hw->als_value, sizeof(obj->als_value));
	atomic_set(&obj->i2c_retry, 3);
    //pre set ps threshold
	obj->ps_thd_val = obj->hw->ps_threshold;
	//pre set window loss
    obj->als_widow_loss = obj->hw->als_window_loss;
	
	al3006_i2c_client = client;

	
	if(err = al3006_init_client(client))
	{
		goto exit_init_failed;
	}
	
	if(err = misc_register(&al3006_device))
	{
		APS_ERR("al3006_device register failed\n");
		goto exit_misc_device_register_failed;
	}

	if(err = al3006_create_attr(&al3006_alsps_driver.driver))
	{
		APS_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}
	obj_ps.self = al3006_obj;
	if(1 == obj->hw->polling_mode)
	{
	  obj_ps.polling = 1;
	}
	else
	{
	  obj_ps.polling = 0;//interrupt mode
	}
	obj_ps.sensor_operate = al3006_ps_operate;
	if(err = hwmsen_attach(ID_PROXIMITY, &obj_ps))
	{
		APS_ERR("attach fail = %d\n", err);
		goto exit_create_attr_failed;
	}
	
	obj_als.self = al3006_obj;
	if(1 == obj->hw->polling_mode)
	{
	  obj_als.polling = 1;
	  APS_LOG("polling mode\n");
	}
	else
	{
	  obj_als.polling = 0;//interrupt mode
	  APS_LOG("interrupt mode\n");
	}
	obj_als.sensor_operate = al3006_als_operate;
	if(err = hwmsen_attach(ID_LIGHT, &obj_als))
	{
		APS_ERR("attach fail = %d\n", err);
		goto exit_create_attr_failed;
	}
/*
	//init timer
	init_timer(&obj->first_read_ps_timer);
	obj->first_read_ps_timer.expires	= jiffies + 110;
	obj->first_read_ps_timer.function	= al3006_first_read_ps_func;
	obj->first_read_ps_timer.data		= 0;

	init_timer(&obj->first_read_als_timer);
	obj->first_read_als_timer.expires	= jiffies + 220;
	obj->first_read_als_timer.function	= al3006_first_read_als_func;
	obj->first_read_als_timer.data		= 0;
*/

#if defined(CONFIG_HAS_EARLYSUSPEND)
	obj->early_drv.level    = EARLY_SUSPEND_LEVEL_STOP_DRAWING - 2,
	obj->early_drv.suspend  = al3006_early_suspend,
	obj->early_drv.resume   = al3006_late_resume,    
	register_early_suspend(&obj->early_drv);
#endif

	APS_LOG("%s: OK\n", __func__);
	return 0;

	exit_create_attr_failed:
	misc_deregister(&al3006_device);
	exit_misc_device_register_failed:
	exit_init_failed:
	//i2c_detach_client(client);
	exit_kfree:
	kfree(obj);
	exit:
	al3006_i2c_client = NULL;           
	#ifdef MT6516        
	MT6516_EINTIRQMask(CUST_EINT_ALS_NUM);  /*mask interrupt if fail*/
	#endif
	APS_ERR("%s: err = %d\n", __func__, err);
	return err;
}
/*----------------------------------------------------------------------------*/
static int al3006_i2c_remove(struct i2c_client *client)
{
	int err;	
	
	if(err = al3006_delete_attr(&al3006_i2c_driver.driver))
	{
		APS_ERR("al3006_delete_attr fail: %d\n", err);
	} 

	if(err = misc_deregister(&al3006_device))
	{
		APS_ERR("misc_deregister fail: %d\n", err);    
	}
	
	al3006_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));

	return 0;
}
/*----------------------------------------------------------------------------*/
static int al3006_probe(struct platform_device *pdev) 
{
	struct alsps_hw *hw = get_cust_alsps_hw();
	//struct al3006_i2c_addr addr;

	al3006_power(hw, 1);    
	//al3006_get_addr(hw, &addr);
//	al3006_force[0] = hw->i2c_num;
	//al3006_force[1] = addr.init;
	if(i2c_add_driver(&al3006_i2c_driver))
	{
		APS_ERR("add driver error\n");
		return -1;
	} 
	return 0;
}
/*----------------------------------------------------------------------------*/
static int al3006_remove(struct platform_device *pdev)
{
	struct alsps_hw *hw = get_cust_alsps_hw();
	APS_FUN();    
	al3006_power(hw, 0);    
	i2c_del_driver(&al3006_i2c_driver);
	return 0;
}
/*----------------------------------------------------------------------------*/
static struct platform_driver al3006_alsps_driver = {
	.probe      = al3006_probe,
	.remove     = al3006_remove,    
	.driver     = {
		.name  = "als_ps",
//		.owner = THIS_MODULE,
	}
};
/*----------------------------------------------------------------------------*/
static int __init al3006_init(void)
{
	APS_FUN();
	struct alsps_hw *hw = get_cust_alsps_hw();
	APS_LOG("%s: i2c_number=%d\n", __func__,hw->i2c_num); 
	i2c_register_board_info(hw->i2c_num ,&i2c_AL3006,1);
	if(platform_driver_register(&al3006_alsps_driver))
	{
		APS_ERR("failed to register driver");
		return -ENODEV;
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit al3006_exit(void)
{
	APS_FUN();
	platform_driver_unregister(&al3006_alsps_driver);
}
/*----------------------------------------------------------------------------*/
module_init(al3006_init);
module_exit(al3006_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("MingHsien Hsieh");
MODULE_DESCRIPTION("ADXL345 accelerometer driver");
MODULE_LICENSE("GPL");
