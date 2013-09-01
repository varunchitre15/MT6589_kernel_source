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

#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>



#define POWER_NONE_MACRO MT65XX_POWER_NONE

#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#include <asm/io.h>
#include <cust_eint.h>
#include <cust_alsps.h>
#include "ltr558.h"
/******************************************************************************
 * configuration
*******************************************************************************/
/*----------------------------------------------------------------------------*/

#define LTR558_DEV_NAME   "LTR_558ALS"

/*----------------------------------------------------------------------------*/
#define APS_TAG                  "[ALS/PS] "
#define APS_FUN(f)               printk(KERN_INFO APS_TAG"%s\n", __FUNCTION__)
#define APS_ERR(fmt, args...)    printk(KERN_ERR  APS_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define APS_LOG(fmt, args...)    printk(KERN_INFO APS_TAG fmt, ##args)
#define APS_DBG(fmt, args...)    printk(KERN_INFO APS_TAG fmt, ##args)                 
/******************************************************************************
 * extern functions
*******************************************************************************/
	
	
extern void mt65xx_eint_unmask(unsigned int line);
extern void mt65xx_eint_mask(unsigned int line);
extern void mt65xx_eint_set_polarity(unsigned int eint_num, unsigned int pol);
extern void mt65xx_eint_set_hw_debounce(unsigned int eint_num, unsigned int ms);
extern unsigned int mt65xx_eint_set_sens(unsigned int eint_num, unsigned int sens);
extern void mt65xx_eint_registration(unsigned int eint_num, unsigned int is_deb_en, unsigned int pol, void (EINT_FUNC_PTR)(void), unsigned int is_auto_umask);


/*----------------------------------------------------------------------------*/

static struct i2c_client *ltr558_i2c_client = NULL;

/*----------------------------------------------------------------------------*/
static const struct i2c_device_id ltr558_i2c_id[] = {{LTR558_DEV_NAME,0},{}};
/*the adapter id & i2c address will be available in customization*/
static struct i2c_board_info __initdata i2c_ltr558={ I2C_BOARD_INFO("LTR_558ALS", (0x46>>1))};

//static unsigned short ltr558_force[] = {0x00, 0x46, I2C_CLIENT_END, I2C_CLIENT_END};
//static const unsigned short *const ltr558_forces[] = { ltr558_force, NULL };
//static struct i2c_client_address_data ltr558_addr_data = { .forces = ltr558_forces,};
/*----------------------------------------------------------------------------*/
static int ltr558_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id); 
static int ltr558_i2c_remove(struct i2c_client *client);
static int ltr558_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);
/*----------------------------------------------------------------------------*/
static int ltr558_i2c_suspend(struct i2c_client *client, pm_message_t msg);
static int ltr558_i2c_resume(struct i2c_client *client);


static int ps_gainrange;
static int als_gainrange;

static int final_prox_val;
static int final_lux_val;

/*----------------------------------------------------------------------------*/
typedef enum {
    CMC_BIT_ALS    = 1,
    CMC_BIT_PS     = 2,
} CMC_BIT;

/*----------------------------------------------------------------------------*/
struct ltr558_i2c_addr {    /*define a series of i2c slave address*/
    u8  write_addr;  
    u8  ps_thd;     /*PS INT threshold*/
};

/*----------------------------------------------------------------------------*/

struct ltr558_priv {
    struct alsps_hw  *hw;
    struct i2c_client *client;
    struct work_struct  eint_work;
    struct mutex lock;
	/*i2c address group*/
    struct ltr558_i2c_addr  addr;

     /*misc*/
    u16		    als_modulus;
    atomic_t    i2c_retry;
    atomic_t    als_debounce;   /*debounce time after enabling als*/
    atomic_t    als_deb_on;     /*indicates if the debounce is on*/
    atomic_t    als_deb_end;    /*the jiffies representing the end of debounce*/
    atomic_t    ps_mask;        /*mask ps: always return far away*/
    atomic_t    ps_debounce;    /*debounce time after enabling ps*/
    atomic_t    ps_deb_on;      /*indicates if the debounce is on*/
    atomic_t    ps_deb_end;     /*the jiffies representing the end of debounce*/
    atomic_t    ps_suspend;
    atomic_t    als_suspend;

    /*data*/
    u16         als;
    u16          ps;
    u8          _align;
    u16         als_level_num;
    u16         als_value_num;
    u32         als_level[C_CUST_ALS_LEVEL-1];
    u32         als_value[C_CUST_ALS_LEVEL];

    atomic_t    als_cmd_val;    /*the cmd value can't be read, stored in ram*/
    atomic_t    ps_cmd_val;     /*the cmd value can't be read, stored in ram*/
    atomic_t    ps_thd_val;     /*the cmd value can't be read, stored in ram*/
    ulong       enable;         /*enable mask*/
    ulong       pending_intr;   /*pending interrupt*/

    /*early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
    struct early_suspend    early_drv;
#endif     
};


static struct ltr558_priv *ltr558_obj = NULL;
static struct platform_driver ltr558_alsps_driver;

/*----------------------------------------------------------------------------*/
static struct i2c_driver ltr558_i2c_driver = {	
	.probe      = ltr558_i2c_probe,
	.remove     = ltr558_i2c_remove,
	.detect     = ltr558_i2c_detect,
	.suspend    = ltr558_i2c_suspend,
	.resume     = ltr558_i2c_resume,
	.id_table   = ltr558_i2c_id,
	//.address_data = &ltr558_addr_data,
	.driver = {
		//.owner          = THIS_MODULE,
		.name           = LTR558_DEV_NAME,
	},
};


/* 
 * #########
 * ## I2C ##
 * #########
 */

// I2C Read
static int ltr558_i2c_read_reg(u8 regnum)
{
    u8 buffer[1],reg_value[1];
	int res = 0;
	
	buffer[0]= regnum;
	res = i2c_master_send(ltr558_obj->client, buffer, 0x1);
	if(res <= 0)
	{
		return res;
	}
	res = i2c_master_recv(ltr558_obj->client, reg_value, 0x1);
	if(res <= 0)
	{
		return res;
	}
	return reg_value[0];
}

// I2C Write
static int ltr558_i2c_write_reg(u8 regnum, u8 value)
{
	u8 databuf[2];    
	int res = 0;
   
	databuf[0] = regnum;   
	databuf[1] = value;
	res = i2c_master_send(ltr558_obj->client, databuf, 0x2);

	if (res < 0)
		return res;
	else
		return 0;
}

/* 
 * ###############
 * ## PS CONFIG ##
 * ###############
 */

static int ltr558_ps_enable(int gainrange)
{
	int error;
	int setgain;
    //APS_LOG("ltr558_ps_enable() ...start!\n");
	switch (gainrange) {
		case PS_RANGE1:
			setgain = MODE_PS_ON_Gain1;
			break;

		case PS_RANGE4:
			setgain = MODE_PS_ON_Gain4;
			break;

		case PS_RANGE8:
			setgain = MODE_PS_ON_Gain8;
			break;

		case PS_RANGE16:
			setgain = MODE_PS_ON_Gain16;
			break;

		default:
			setgain = MODE_PS_ON_Gain1;
			break;
	}

	//APS_LOG("LTR558_PS_CONTR==>%d!\n",setgain);

	error = ltr558_i2c_write_reg(LTR558_PS_CONTR, setgain); 
	if(error<0)
	{
	    APS_LOG("ltr558_ps_enable() error1\n");
	    return error;
	}
	
	mdelay(WAKEUP_DELAY);
    
	/* =============== 
	 * ** IMPORTANT **
	 * ===============
	 * Other settings like timing and threshold to be set here, if required.
 	 * Not set and kept as device default for now.
 	 */
   error = ltr558_i2c_write_reg(LTR558_PS_N_PULSES, 2); 
	if(error<0)
    {
        APS_LOG("ltr558_ps_enable() error2\n");
	    return error;
	} 
	/*error = ltr558_i2c_write_reg(LTR558_PS_LED, 0x63); 
	if(error<0)
    {
        APS_LOG("ltr558_ps_enable() error3...\n");
	    return error;
	}*/
		
 	APS_LOG("ltr558_ps_enable ...OK!\n");
 	
	return error;
}

// Put PS into Standby mode
static int ltr558_ps_disable(void)
{
	int error;
	error = ltr558_i2c_write_reg(LTR558_PS_CONTR, MODE_PS_StdBy); 
	if(error<0)
 	    APS_LOG("ltr558_ps_disable ...ERROR\n");
 	else
        APS_LOG("ltr558_ps_disable ...OK\n");
	return error;
}


static int ltr558_ps_read(void)
{
	int psval_lo, psval_hi, psdata;

	psval_lo = ltr558_i2c_read_reg(LTR558_PS_DATA_0);
	if (psval_lo < 0){
	    
	    APS_DBG("psval_lo error\n");
		psdata = psval_lo;
		goto out;
	}
	//APS_DBG("psval_lo = %d\n", psval_lo);	
	psval_hi = ltr558_i2c_read_reg(LTR558_PS_DATA_1);
	if (psval_hi < 0){
	    APS_DBG("psval_hi error\n");
		psdata = psval_hi;
		goto out;
	}
	//APS_DBG("psval_hi = %d\n", psval_hi);	
	
	psdata = ((psval_hi & 7)* 256) + psval_lo;
    //psdata = ((psval_hi&0x7)<<8) + psval_lo;
    //APS_DBG("ps_value = %d\n", psdata);
    
	out:
	final_prox_val = psdata;
	
	return psdata;
}

/* 
 * ################
 * ## ALS CONFIG ##
 * ################
 */

static int ltr558_als_enable(int gainrange)
{
	int error;

	if (gainrange == 1)
		error = ltr558_i2c_write_reg(LTR558_ALS_CONTR, MODE_ALS_ON_Range1);
	else if (gainrange == 2)
		error = ltr558_i2c_write_reg(LTR558_ALS_CONTR, MODE_ALS_ON_Range2);
	else
		error = -1;

	mdelay(WAKEUP_DELAY);

	/* =============== 
	 * ** IMPORTANT **
	 * ===============
	 * Other settings like timing and threshold to be set here, if required.
 	 * Not set and kept as device default for now.
 	 */
 	if(error<0)
 	    APS_LOG("ltr558_als_enable ...ERROR\n");
 	else
        APS_LOG("ltr558_als_enable ...OK\n");
        
	return error;
}


// Put ALS into Standby mode
static int ltr558_als_disable(void)
{
	int error;
	error = ltr558_i2c_write_reg(LTR558_ALS_CONTR, MODE_ALS_StdBy); 
	if(error<0)
 	    APS_LOG("ltr558_als_disable ...ERROR\n");
 	else
        APS_LOG("ltr558_als_disable ...OK\n");
	return error;
}

static int ltr558_als_read(int gainrange)
{
	int alsval_ch0_lo, alsval_ch0_hi, alsval_ch0;
	int alsval_ch1_lo, alsval_ch1_hi, alsval_ch1;
	int luxdata_int;
	int ratio;

	alsval_ch0_lo = ltr558_i2c_read_reg(LTR558_ALS_DATA_CH0_0);
	alsval_ch0_hi = ltr558_i2c_read_reg(LTR558_ALS_DATA_CH0_1);
	alsval_ch0 = (alsval_ch0_hi * 256) + alsval_ch0_lo;

	alsval_ch1_lo = ltr558_i2c_read_reg(LTR558_ALS_DATA_CH1_0);
	alsval_ch1_hi = ltr558_i2c_read_reg(LTR558_ALS_DATA_CH1_1);
	alsval_ch1 = (alsval_ch1_hi * 256) + alsval_ch1_lo;
	
    //APS_DBG("alsval_ch0=%d,  alsval_ch1=%d\n", alsval_ch0, alsval_ch1);

    if((alsval_ch1==0)||(alsval_ch0==0))
    {
        luxdata_int = 0;
        goto err;
    }
	ratio = alsval_ch1*100 / alsval_ch0;

	if (ratio < 69){
		luxdata_int = ((1361 * alsval_ch0) - (1500 * alsval_ch1))/1000;
	}
	else if ((ratio >= 69) && (ratio < 100)){
		luxdata_int = ((570 * alsval_ch0) - (345 * alsval_ch1))/1000;
	}
	else {
		luxdata_int = 0;
	}

	// For Range1
	if (gainrange == ALS_RANGE1_320)
		luxdata_int = luxdata_int / 150;

	// convert float to integer;
	//luxdata_int = luxdata_flt;
/*	if ((luxdata_flt - luxdata_int) > 0.5){
		luxdata_int = luxdata_int + 1;
	}
	else {
		luxdata_int = luxdata_flt;
	}*/
err:
	final_lux_val = luxdata_int;
	//APS_DBG("als_value_lux = 0x%x\n", luxdata_int);
	return luxdata_int;
}



/*----------------------------------------------------------------------------*/
int ltr558_get_addr(struct alsps_hw *hw, struct ltr558_i2c_addr *addr)
{
	if(!hw || !addr)
	{
		return -EFAULT;
	}
	addr->write_addr= hw->i2c_addr[0];
	return 0;
}

/*----------------------------------------------------------------------------*/
static void ltr558_power(struct alsps_hw *hw, unsigned int on) 
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
			if(!hwPowerOn(hw->power_id, hw->power_vol, "TMD2771")) 
			{
				APS_ERR("power on fails!!\n");
			}
		}
		else
		{
			if(!hwPowerDown(hw->power_id, "TMD2771")) 
			{
				APS_ERR("power off fail!!\n");   
			}
		}
	}
	power_on = on;
}

static int ltr558_devinit(void)
{
	int error;
	int init_ps_gain;
	int init_als_gain;

	mdelay(PON_DELAY);

	// Enable PS to Gain1 at startup
	init_ps_gain = PS_RANGE1;
	ps_gainrange = init_ps_gain;

	error = ltr558_ps_enable(init_ps_gain);
	if (error < 0)
		goto out;


	// Enable ALS to Full Range at startup
	init_als_gain = ALS_RANGE2_64K;
	als_gainrange = init_als_gain;

	error = ltr558_als_enable(init_als_gain);
	if (error < 0)
		goto out;

	error = 0;

	out:
	return error;
}
/*----------------------------------------------------------------------------*/


static int ltr558_get_als_value(struct ltr558_priv *obj, u16 als)
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
		//APS_DBG("ALS: %05d => %05d\n", als, obj->hw->als_value[idx]);	
		return obj->hw->als_value[idx];
	}
	else
	{
		APS_ERR("ALS: %05d => %05d (-1)\n", als, obj->hw->als_value[idx]);    
		return -1;
	}
}
/*----------------------------------------------------------------------------*/
static int ltr558_get_ps_value(struct ltr558_priv *obj, u16 ps)
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
	else if (obj->als > 50000)
	{
		//invalid = 1;
		APS_DBG("ligh too high will result to failt proximiy\n");
		return 1;  /*far away*/
	}

	if(!invalid)
	{
		//APS_DBG("PS:  %05d => %05d\n", ps, val);
		return val;
	}	
	else
	{
		return -1;
	}	
}

/*----------------------------------------------------------------------------*/

/****************************************************************************** 
 * Function Configuration
******************************************************************************/
static int ltr558_open(struct inode *inode, struct file *file)
{
	file->private_data = ltr558_i2c_client;

	if (!file->private_data)
	{
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}
	
	return nonseekable_open(inode, file);
}
/*----------------------------------------------------------------------------*/
static int ltr558_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}
/*----------------------------------------------------------------------------*/


//static int ltr558_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
  //     unsigned long arg)
static int ltr558_unlocked_ioctl(struct file *file, unsigned int cmd,
       unsigned long arg)       
{
	struct i2c_client *client = (struct i2c_client*)file->private_data;
	struct ltr558_priv *obj = i2c_get_clientdata(client);  
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
			    err = ltr558_ps_enable(ps_gainrange);
				if(err < 0)
				{
					APS_ERR("enable ps fail: %d\n", err); 
					goto err_out;
				}
				set_bit(CMC_BIT_PS, &obj->enable);
			}
			else
			{
			    err = ltr558_ps_disable();
				if(err < 0)
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
		    obj->ps = ltr558_ps_read();
			if(obj->ps < 0)
			{
				goto err_out;
			}
			
			dat = ltr558_get_ps_value(obj, obj->ps);
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}  
			break;

		case ALSPS_GET_PS_RAW_DATA:    
			obj->ps = ltr558_ps_read();
			if(obj->ps < 0)
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
			    err = ltr558_als_enable(als_gainrange);
				if(err < 0)
				{
					APS_ERR("enable als fail: %d\n", err); 
					goto err_out;
				}
				set_bit(CMC_BIT_ALS, &obj->enable);
			}
			else
			{
			    err = ltr558_als_disable();
				if(err < 0)
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
		    obj->als = ltr558_als_read(als_gainrange);
			if(obj->als < 0)
			{
				goto err_out;
			}

			dat = ltr558_get_als_value(obj, obj->als);
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}              
			break;

		case ALSPS_GET_ALS_RAW_DATA:    
			obj->als = ltr558_als_read(als_gainrange);
			if(obj->als < 0)
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
static struct file_operations ltr558_fops = {
	//.owner = THIS_MODULE,
	.open = ltr558_open,
	.release = ltr558_release,
	.unlocked_ioctl = ltr558_unlocked_ioctl,
};
/*----------------------------------------------------------------------------*/
static struct miscdevice ltr558_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "als_ps",
	.fops = &ltr558_fops,
};

static int ltr558_i2c_suspend(struct i2c_client *client, pm_message_t msg) 
{
	struct ltr558_priv *obj = i2c_get_clientdata(client);    
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
		err = ltr558_als_disable();
		if(err < 0)
		{
			APS_ERR("disable als: %d\n", err);
			return err;
		}

		atomic_set(&obj->ps_suspend, 1);
		err = ltr558_ps_disable();
		if(err < 0)
		{
			APS_ERR("disable ps:  %d\n", err);
			return err;
		}
		
		ltr558_power(obj->hw, 0);
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
static int ltr558_i2c_resume(struct i2c_client *client)
{
	struct ltr558_priv *obj = i2c_get_clientdata(client);        
	int err;
	APS_FUN();

	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}

	ltr558_power(obj->hw, 1);
/*	err = ltr558_devinit();
	if(err < 0)
	{
		APS_ERR("initialize client fail!!\n");
		return err;        
	}*/
	atomic_set(&obj->als_suspend, 0);
	if(test_bit(CMC_BIT_ALS, &obj->enable))
	{
	    err = ltr558_als_enable(als_gainrange);
	    if (err < 0)
		{
			APS_ERR("enable als fail: %d\n", err);        
		}
	}
	atomic_set(&obj->ps_suspend, 0);
	if(test_bit(CMC_BIT_PS,  &obj->enable))
	{
		err = ltr558_ps_enable(ps_gainrange);
	    if (err < 0)
		{
			APS_ERR("enable ps fail: %d\n", err);                
		}
	}

	return 0;
}

static void ltr558_early_suspend(struct early_suspend *h) 
{   /*early_suspend is only applied for ALS*/
	struct ltr558_priv *obj = container_of(h, struct ltr558_priv, early_drv);   
	int err;
	APS_FUN();    

	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return;
	}
	
	atomic_set(&obj->als_suspend, 1); 
	err = ltr558_als_disable();
	if(err < 0)
	{
		APS_ERR("disable als fail: %d\n", err); 
	}
}

static void ltr558_late_resume(struct early_suspend *h)
{   /*early_suspend is only applied for ALS*/
	struct ltr558_priv *obj = container_of(h, struct ltr558_priv, early_drv);         
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
	    err = ltr558_als_enable(als_gainrange);
		if(err < 0)
		{
			APS_ERR("enable als fail: %d\n", err);        

		}
	}
}

int ltr558_ps_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	hwm_sensor_data* sensor_data;
	struct ltr558_priv *obj = (struct ltr558_priv *)self;
	
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
				    err = ltr558_ps_enable(ps_gainrange);
					if(err < 0)
					{
						APS_ERR("enable ps fail: %d\n", err); 
						return -1;
					}
					set_bit(CMC_BIT_PS, &obj->enable);
				}
				else
				{
				    err = ltr558_ps_disable();
					if(err < 0)
					{
						APS_ERR("disable ps fail: %d\n", err); 
						return -1;
					}
					clear_bit(CMC_BIT_PS, &obj->enable);
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
				obj->ps = ltr558_ps_read();
    			if(obj->ps < 0)
    			{
    				err = -1;
    				break;
    			}
				sensor_data->values[0] = ltr558_get_ps_value(obj, obj->ps);
				sensor_data->value_divide = 1;
				sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;			
			}
			break;
		default:
			APS_ERR("proxmy sensor operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}
	
	return err;
}

int ltr558_als_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	hwm_sensor_data* sensor_data;
	struct ltr558_priv *obj = (struct ltr558_priv *)self;

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
				    err = ltr558_als_enable(als_gainrange);
					if(err < 0)
					{
						APS_ERR("enable als fail: %d\n", err); 
						return -1;
					}
					set_bit(CMC_BIT_ALS, &obj->enable);
				}
				else
				{
				    err = ltr558_als_disable();
					if(err < 0)
					{
						APS_ERR("disable als fail: %d\n", err); 
						return -1;
					}
					clear_bit(CMC_BIT_ALS, &obj->enable);
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
				obj->als = ltr558_als_read(als_gainrange);
				#if defined(MTK_AAL_SUPPORT)
				sensor_data->values[0] = obj->als;
				#else				
				sensor_data->values[0] = ltr558_get_als_value(obj, obj->als);
				#endif
				sensor_data->value_divide = 1;
				sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
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
static int ltr558_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info) 
{    
	strcpy(info->type, LTR558_DEV_NAME);
	return 0;
}

/*----------------------------------------------------------------------------*/
static int ltr558_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	//struct tmd2771_priv *obj;
	struct ltr558_priv *obj;
	struct hwmsen_object obj_ps, obj_als;
	int err = 0;

	if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
	{
		err = -ENOMEM;
		goto exit;
	}
	memset(obj, 0, sizeof(*obj));
	ltr558_obj = obj;

	obj->hw = get_cust_alsps_hw();
	ltr558_get_addr(obj->hw, &obj->addr);

	//INIT_WORK(&obj->eint_work, tmd2771_eint_work);
	obj->client = client;
	i2c_set_clientdata(client, obj);	
	atomic_set(&obj->als_debounce, 300);
	atomic_set(&obj->als_deb_on, 0);
	atomic_set(&obj->als_deb_end, 0);
	atomic_set(&obj->ps_debounce, 300);
	atomic_set(&obj->ps_deb_on, 0);
	atomic_set(&obj->ps_deb_end, 0);
	atomic_set(&obj->ps_mask, 0);
	atomic_set(&obj->als_suspend, 0);
	//atomic_set(&obj->als_cmd_val, 0xDF);
	//atomic_set(&obj->ps_cmd_val,  0xC1);
	atomic_set(&obj->ps_thd_val,  obj->hw->ps_threshold);
	obj->enable = 0;
	obj->pending_intr = 0;
	obj->als_level_num = sizeof(obj->hw->als_level)/sizeof(obj->hw->als_level[0]);
	obj->als_value_num = sizeof(obj->hw->als_value)/sizeof(obj->hw->als_value[0]);   
	obj->als_modulus = (400*100)/(16*150);//(1/Gain)*(400/Tine), this value is fix after init ATIME and CONTROL register value
										//(400)/16*2.72 here is amplify *100
	BUG_ON(sizeof(obj->als_level) != sizeof(obj->hw->als_level));
	memcpy(obj->als_level, obj->hw->als_level, sizeof(obj->als_level));
	BUG_ON(sizeof(obj->als_value) != sizeof(obj->hw->als_value));
	memcpy(obj->als_value, obj->hw->als_value, sizeof(obj->als_value));
	atomic_set(&obj->i2c_retry, 3);
	set_bit(CMC_BIT_ALS, &obj->enable);
	set_bit(CMC_BIT_PS, &obj->enable);

	APS_LOG("ltr558_devinit() start...!\n");
	ltr558_i2c_client = client;

	if(err = ltr558_devinit())
	{
		goto exit_init_failed;
	}
	APS_LOG("ltr558_devinit() ...OK!\n");

	//printk("@@@@@@ manufacturer value:%x\n",ltr558_i2c_read_reg(0x87));

	if(err = misc_register(&ltr558_device))
	{
		APS_ERR("tmd2771_device register failed\n");
		goto exit_misc_device_register_failed;
	}
/*
	if(err = tmd2771_create_attr(&tmd2771_alsps_driver.driver))
	{
		APS_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}
*/
	obj_ps.self = ltr558_obj;
	obj_ps.polling = 1;
	obj_ps.sensor_operate = ltr558_ps_operate;
	if(err = hwmsen_attach(ID_PROXIMITY, &obj_ps))
	{
		APS_ERR("attach fail = %d\n", err);
		goto exit_create_attr_failed;
	}
	
	obj_als.self = ltr558_obj;
	obj_als.polling = 1;
	obj_als.sensor_operate = ltr558_als_operate;
	if(err = hwmsen_attach(ID_LIGHT, &obj_als))
	{
		APS_ERR("attach fail = %d\n", err);
		goto exit_create_attr_failed;
	}


#if defined(CONFIG_HAS_EARLYSUSPEND)
	obj->early_drv.level    = EARLY_SUSPEND_LEVEL_STOP_DRAWING - 2,
	obj->early_drv.suspend  = ltr558_early_suspend,
	obj->early_drv.resume   = ltr558_late_resume,    
	register_early_suspend(&obj->early_drv);
#endif

	APS_LOG("%s: OK\n", __func__);
	return 0;

	exit_create_attr_failed:
	misc_deregister(&ltr558_device);
	exit_misc_device_register_failed:
	exit_init_failed:
	//i2c_detach_client(client);
	exit_kfree:
	kfree(obj);
	exit:
	ltr558_i2c_client = NULL;           
//	MT6516_EINTIRQMask(CUST_EINT_ALS_NUM);  /*mask interrupt if fail*/
	APS_ERR("%s: err = %d\n", __func__, err);
	return err;
}
/*----------------------------------------------------------------------------*/
static int ltr558_i2c_remove(struct i2c_client *client)
{
	int err;	

	if(err = misc_deregister(&ltr558_device))
	{
		APS_ERR("misc_deregister fail: %d\n", err);    
	}
	
	ltr558_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));

	return 0;
}
/*----------------------------------------------------------------------------*/
static int ltr558_probe(struct platform_device *pdev) 
{
	struct alsps_hw *hw = get_cust_alsps_hw();

	ltr558_power(hw, 1);
	//ltr558_force[0] = hw->i2c_num;
	//ltr558_force[1] = hw->i2c_addr[0];
	//APS_DBG("I2C = %d, addr =0x%x\n",ltr558_force[0],ltr558_force[1]);
	if(i2c_add_driver(&ltr558_i2c_driver))
	{
		APS_ERR("add driver error\n");
		return -1;
	} 
	return 0;
}
/*----------------------------------------------------------------------------*/
static int ltr558_remove(struct platform_device *pdev)
{
	struct alsps_hw *hw = get_cust_alsps_hw();
	APS_FUN();    
	ltr558_power(hw, 0);    
	i2c_del_driver(&ltr558_i2c_driver);
	return 0;
}
/*----------------------------------------------------------------------------*/
static struct platform_driver ltr558_alsps_driver = {
	.probe      = ltr558_probe,
	.remove     = ltr558_remove,    
	.driver     = {
		.name  = "als_ps",
		//.owner = THIS_MODULE,
	}
};
/*----------------------------------------------------------------------------*/
static int __init ltr558_init(void)
{
	APS_FUN();
	struct alsps_hw *hw = get_cust_alsps_hw();
	APS_LOG("%s: i2c_number=%d\n", __func__,hw->i2c_num); 
	i2c_register_board_info(hw->i2c_num, &i2c_ltr558, 1);
	if(platform_driver_register(&ltr558_alsps_driver))
	{
		APS_ERR("failed to register driver");
		return -ENODEV;
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit ltr558_exit(void)
{
	APS_FUN();
	platform_driver_unregister(&ltr558_alsps_driver);
}
/*----------------------------------------------------------------------------*/
module_init(ltr558_init);
module_exit(ltr558_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("Keqi Li");
MODULE_DESCRIPTION("LTR-558ALS Driver");
MODULE_LICENSE("GPL");

