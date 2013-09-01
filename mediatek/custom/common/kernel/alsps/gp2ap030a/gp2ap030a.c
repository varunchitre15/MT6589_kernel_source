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

#include <mach/mt_devs.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>
#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#include <asm/io.h>
#include <cust_eint.h>
#include <cust_alsps.h>
#include "gp2ap030a.h"
#include <linux/hwmsen_helper.h>

#define POWER_NONE_MACRO MT65XX_POWER_NONE


/******************************************************************************
 * configuration
*******************************************************************************/
/*----------------------------------------------------------------------------*/

#define GP2AP030A_DEV_NAME     "GP2AP030A00F"
/*----------------------------------------------------------------------------*/
#define APS_TAG                  "[ALS/PS] "
#define APS_FUN(f)               printk(KERN_INFO APS_TAG"%s\n", __FUNCTION__)
#define APS_ERR(fmt, args...)    printk(KERN_ERR  APS_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define APS_LOG(fmt, args...)    printk(KERN_ERR APS_TAG fmt, ##args)
#define APS_DBG(fmt, args...)    printk(KERN_INFO APS_TAG fmt, ##args)                 
/******************************************************************************
* extern functions 
*******************************************************************************/

#ifdef MT6577
		extern void mt65xx_eint_unmask(unsigned int line);
		extern void mt65xx_eint_mask(unsigned int line);
		extern void mt65xx_eint_set_polarity(unsigned int eint_num, unsigned int pol);
		extern void mt65xx_eint_set_hw_debounce(unsigned int eint_num, unsigned int ms);
		extern unsigned int mt65xx_eint_set_sens(unsigned int eint_num, unsigned int sens);
		extern void mt65xx_eint_registration(unsigned int eint_num, unsigned int is_deb_en, unsigned int pol, void (EINT_FUNC_PTR)(void), unsigned int is_auto_umask);
#endif
#ifdef MT6575
		extern void mt65xx_eint_unmask(unsigned int line);
		extern void mt65xx_eint_mask(unsigned int line);
		extern void mt65xx_eint_set_polarity(kal_uint8 eintno, kal_bool ACT_Polarity);
		extern void mt65xx_eint_set_hw_debounce(kal_uint8 eintno, kal_uint32 ms);
		extern kal_uint32 mt65xx_eint_set_sens(kal_uint8 eintno, kal_bool sens);
		extern void mt65xx_eint_registration(kal_uint8 eintno, kal_bool Dbounce_En,
											 kal_bool ACT_Polarity, void (EINT_FUNC_PTR)(void),
											 kal_bool auto_umask);
		
#endif

/*----------------------------------------------------------------------------*/
static int gp2ap030a_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id); 
static int gp2ap030a_i2c_remove(struct i2c_client *client);
//static int gp2ap030a_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
static int gp2ap030a_i2c_suspend(struct i2c_client *client, pm_message_t msg);
static int gp2ap030a_i2c_resume(struct i2c_client *client);

/*----------------------------------------------------------------------------*/
static const struct i2c_device_id gp2ap030a_i2c_id[] = {{GP2AP030A_DEV_NAME,0},{}};
static struct i2c_board_info __initdata i2c_gp2ap030a={ I2C_BOARD_INFO(GP2AP030A_DEV_NAME, 0x39)};
/*----------------------------------------------------------------------------*/
struct gp2ap030a_priv {
	struct alsps_hw  *hw;
	struct i2c_client *client;
	u8					regData[12] ;
	struct work_struct	eint_work;

	/*misc*/
	u16 		als_modulus;
	atomic_t	i2c_retry;
	atomic_t	als_suspend;
	atomic_t	als_debounce;	/*debounce time after enabling als*/
	atomic_t	als_deb_on; 	/*indicates if the debounce is on*/
	atomic_t	als_deb_end;	/*the jiffies representing the end of debounce*/
	atomic_t	ps_mask;		/*mask ps: always return far away*/
	atomic_t	ps_debounce;	/*debounce time after enabling ps*/
	atomic_t	ps_deb_on;		/*indicates if the debounce is on*/
	atomic_t	ps_deb_end; 	/*the jiffies representing the end of debounce*/
	atomic_t	ps_suspend;
	atomic_t 	trace;
	
	
	/*data*/
	u16			als;
	u16			ps;
	u16			als_level_num;
	u16			als_value_num;
	u32			als_level[C_CUST_ALS_LEVEL-1];
	u32			als_value[C_CUST_ALS_LEVEL];
	int			als_mode ;
	int			als_lux_prev ;
	
	atomic_t	als_cmd_val;	/*the cmd value can't be read, stored in ram*/
	atomic_t	ps_cmd_val; 	/*the cmd value can't be read, stored in ram*/
	atomic_t	ps_thd_val_high;	 /*the cmd value can't be read, stored in ram*/
	atomic_t	ps_thd_val_low; 	/*the cmd value can't be read, stored in ram*/
	atomic_t	ps_thd_val;
	ulong		enable; 		/*enable mask*/
	ulong		pending_intr;	/*pending interrupt*/
	
	/*early suspend*/
	#if defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend	early_drv;
	#endif     
};
/*----------------------------------------------------------------------------*/

static struct i2c_driver gp2ap030a_i2c_driver = {	
	.probe      = gp2ap030a_i2c_probe,
	.remove     = gp2ap030a_i2c_remove,
	//.detect     = gp2ap030a_i2c_detect,
	.suspend    = gp2ap030a_i2c_suspend,
	.resume     = gp2ap030a_i2c_resume,
	.id_table   = gp2ap030a_i2c_id,
	.driver = {
		.name = GP2AP030A_DEV_NAME,
	},
};

/*----------------------------------------------------------------------------*/
struct PS_CALI_DATA_STRUCT
{
	int close;
	int far_away;
	int valid;
};

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static struct i2c_client *gp2ap030a_i2c_client = NULL;
static struct gp2ap030a_priv *g_gp2ap030a_ptr = NULL;
static struct gp2ap030a_priv *gp2ap030a_obj = NULL;
static struct platform_driver gp2ap030a_alsps_driver;
static struct PS_CALI_DATA_STRUCT ps_cali={0,0,0};
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
typedef enum {
	CMC_BIT_ALS    = 1,
	CMC_BIT_PS	   = 2,
}CMC_BIT;
/*-----------------------------CMC for debugging-------------------------------*/
typedef enum {
    CMC_TRC_ALS_DATA= 0x0001,
    CMC_TRC_PS_DATA = 0x0002,
    CMC_TRC_EINT    = 0x0004,
    CMC_TRC_IOCTL   = 0x0008,
    CMC_TRC_I2C     = 0x0010,
    CMC_TRC_CVT_ALS = 0x0020,
    CMC_TRC_CVT_PS  = 0x0040,
    CMC_TRC_DEBUG   = 0x8000,
} CMC_TRC;

/*following GIPO should be set by DCT TOOL*/
#if 0
#define GPIO_ALS_EINT_PIN         GPIO190
#define GPIO_ALS_EINT_PIN_M_GPIO  GPIO_MODE_00
#define GPIO_ALS_EINT_PIN_M_EINT  GPIO_MODE_01
#define GPIO_ALS_EINT_PIN_M_PWM  GPIO_MODE_04
#define CUST_EINT_ALS_NUM              3
#define CUST_EINT_ALS_DEBOUNCE_CN      0
#define CUST_EINT_ALS_POLARITY         CUST_EINT_POLARITY_LOW
#define CUST_EINT_ALS_SENSITIVE        CUST_EINT_LEVEL_SENSITIVE
#define CUST_EINT_ALS_DEBOUNCE_EN      CUST_EINT_DEBOUNCE_DISABLE
#endif

/*----------------------------------------------------------------------------*/
static u8 gp2ap_init_data[12] = {
	/* Reg0 shutdown */
	0x00,
	/* Reg1 PRST:01 RES_A:100 RANGE_A:011 */
	( PRST_4 | RES_A_14 | RANGE_A_8 ),
	/* Reg2 INTTYPE:0 RES_P:011 RANGE_P:010 */
	( INTTYPE_L | RES_P_10 | RANGE_P_4 ),
	/* Reg3 INTVAL:0 IS:11 PIN:11 FREQ:0 */
	( INTVAL_0 | IS_110 | PIN_DETECT | FREQ_327_5 ),
	/* Reg.4 TL[7:0]:0x00 */
	0x00,
	/* Reg.5 TL[15:8]:0x00 */
	0x00,
	/* Reg.6 TH[7:0]:0x00 */
	0xFF,
	/* Reg.7 TH[15:8]:0x00 */
	0xFF,
	/* Reg.8 PL[7:0]:0x0B */
	0x0B,
	/* Reg.9 PL[15:8]:0x00 */
	0x00,
	/* Reg.A PH[7:0]:0x0C */
	0x0C,
	/* Reg.B PH[15:8]:0x00 */
	0x00
} ;

static void gp2ap_init_device( struct gp2ap030a_priv *data );

static int gp2ap_i2c_write( u8 reg, u8 *wbuf, struct i2c_client *client )
{
	int 		err = 0 ;

	if( client == NULL )
	{
		return -ENODEV ;
	}
	
	err = hwmsen_write_byte(client,reg,*wbuf);
	//APS_DBG("PS:  %05d => %05d =>%05d [M] \n", reg, *wbuf,err);
	return err ;
}
static int hwmsen_read_byte_sr(struct i2c_client *client, u8 addr, u8 *data)
{
   u8 buf;
    int ret = 0;
    client->addr = client->addr&(I2C_MASK_FLAG | I2C_WR_FLAG |I2C_RS_FLAG);

    buf = addr;
	ret = i2c_master_send(client, (const char*)&buf, 1<<8 | 1);
    if (ret < 0) {
        HWM_ERR("send command error!!\n");
        return -EFAULT;
    }

    *data = buf;
	client->addr = client->addr& I2C_MASK_FLAG;
    return 0;
}



static void gp2ap030a_power(struct alsps_hw *hw, unsigned int on) 
{
	static unsigned int power_on = 0;

	APS_LOG("power %s\n", on ? "on" : "off");

	if(hw->power_id != POWER_NONE_MACRO)
	{
		if(power_on == on)
		{
			APS_LOG("ignore power control: %d\n", on);
		}
		else if(on)
		{
			if(!hwPowerOn(hw->power_id, hw->power_vol, "GP2AP030A")) 
			{
				APS_ERR("power on fails!!\n");
			}
		}
		else
		{
			if(!hwPowerDown(hw->power_id, "GP2AP030A")) 
			{
				APS_ERR("power off fail!!\n");   
			}
		}
	}
	power_on = on;
}

/********************************************************************/
int gp2ap030a_enable_ps(struct i2c_client *client, int enable)
{
	struct gp2ap030a_priv *obj = i2c_get_clientdata(client);
	u8			value ;
	u8			rdata ;
	int 		err = -1;
	u32			th_l ;
	u32			th_h ;

	if(enable == 1)
	{
		if(0 == test_bit(CMC_BIT_ALS,  &obj->enable))
		{
				value = RST ;	// RST
				err =gp2ap_i2c_write( REG_ADR_03, &value, client ) ;
				gp2ap_init_device( obj );
				value = ( INTVAL_16 | IS_110 | PIN_DETECT | FREQ_327_5 );
				err =gp2ap_i2c_write( REG_ADR_03, &value, client ) ;
				value = ( OP_RUN | OP_CONTINUOUS | OP_PS ) ;		// PS mode
				err =gp2ap_i2c_write( REG_ADR_00, &value, client ) ;
		}
		else
		{
			value = OP_SHUTSOWN ;	// shutdown
			err =gp2ap_i2c_write( REG_ADR_00, &value, client ) ;
			value = ( u8 )( PRST_4 | RES_A_14 | RANGE_A_8 );
			err =gp2ap_i2c_write( REG_ADR_01, &value, client ) ;
			value = ( INTVAL_0 | IS_110 | PIN_DETECT | FREQ_327_5 );
			err =gp2ap_i2c_write( REG_ADR_03, &value, client ) ;
			value = ( OP_RUN | OP_CONTINUOUS | OP_PS_ALS ) ;	// ALS & PS mode
			err =gp2ap_i2c_write( REG_ADR_00, &value, client ) ;
		}
		value = (INTTYPE_L );
		err =gp2ap_i2c_write( REG_ADR_02, &value, client ) ;

		
		th_l = atomic_read(&obj->ps_thd_val_low);
		th_h = atomic_read(&obj->ps_thd_val_high);
		value = ( u8 )( th_l & 0x000000ff ) ;
		obj->regData[REG_ADR_08] = value ;
		err =gp2ap_i2c_write( REG_ADR_08, &value, client );

		value = ( u8 )( ( th_l	& 0x0000ff00 ) >> 8 ) ;
		obj->regData[REG_ADR_09] = value ;
		err =gp2ap_i2c_write( REG_ADR_09, &value, client ) ;

		value = ( u8 )( th_h & 0x000000ff ) ;
		obj->regData[REG_ADR_0A] = value ;
		err =gp2ap_i2c_write( REG_ADR_0A, &value, client );

		value = ( u8 )( ( th_h	& 0x0000ff00 ) >> 8 ) ;
		obj->regData[REG_ADR_0B] = value ;
		err =gp2ap_i2c_write( REG_ADR_0B, &value, client ) ;

		atomic_set(&obj->ps_deb_on, 1);
		atomic_set(&obj->ps_deb_end, jiffies+atomic_read(&obj->ps_debounce)/(1000/HZ));
		set_bit(CMC_BIT_PS,  &obj->pending_intr);
	}
	else
	{
		if(0 == test_bit(CMC_BIT_ALS,  &obj->enable))
		{
			value = OP_SHUTSOWN ;	// shutdown
			err =gp2ap_i2c_write( REG_ADR_00, &value, client ) ;
		}
		else
		{
			value = OP_SHUTSOWN ;	// shutdown
			err =gp2ap_i2c_write( REG_ADR_00, &value, client ) ;
			value = ( INTVAL_0 | IS_110 | PIN_DETECT | FREQ_327_5 );
			err =gp2ap_i2c_write( REG_ADR_03, &value, client ) ;
			value = ( OP_RUN | OP_CONTINUOUS | OP_ALS ) ;		// ALS mode
			err =gp2ap_i2c_write( REG_ADR_00, &value, client ) ;
		}
		atomic_set(&obj->ps_deb_on, 0);
	}
	return err;
}
/********************************************************************/
int gp2ap030a_enable_als(struct i2c_client *client, int enable)
{
	struct gp2ap030a_priv *obj = i2c_get_clientdata(client);
	u8		value ;
	u8		rdata ;

	if(enable == 1)
	{
		if(0 == test_bit(CMC_BIT_PS,  &obj->enable))
		{
			value = RST ;	// RST
			gp2ap_i2c_write( REG_ADR_03, &value, client ) ;
			
			gp2ap_init_device( obj );
			value = ( INTVAL_0 | IS_110 | PIN_DETECT | FREQ_327_5 );
			gp2ap_i2c_write( REG_ADR_03, &value, client ) ;
			value = ( OP_RUN | OP_CONTINUOUS | OP_ALS ) ;		// ALS mode
			gp2ap_i2c_write( REG_ADR_00, &value, client ) ;
		}
		else
		{
			value = OP_SHUTSOWN ; // shutdown
			gp2ap_i2c_write( REG_ADR_00, &value, client ) ;
			value = ( u8 )( PRST_4 | RES_A_14 | RANGE_A_8 );
			value = ( u8 )( RES_A_14 | RANGE_A_8 ) | ( rdata & 0xC0 ) ;
			gp2ap_i2c_write( REG_ADR_01, &value, client ) ;
			value = ( INTVAL_0 | IS_110 | PIN_DETECT | FREQ_327_5 );
			gp2ap_i2c_write( REG_ADR_03, &value, client ) ;
			value = ( OP_RUN | OP_CONTINUOUS | OP_PS_ALS ) ;	// ALS & PS mode
			gp2ap_i2c_write( REG_ADR_00, &value, client ) ;
		}
		atomic_set(&obj->als_deb_on, 1);
		atomic_set(&obj->als_deb_end, jiffies+atomic_read(&obj->als_debounce)/(1000/HZ));
		set_bit(CMC_BIT_ALS,  &obj->pending_intr);
	}
	else
	{
		if(0 == test_bit(CMC_BIT_PS,  &obj->enable))
		{
			value = OP_SHUTSOWN ; // shutdown
			gp2ap_i2c_write( REG_ADR_00, &value, client ) ;
		}
		else
		{
			value = OP_SHUTSOWN ; // shutdown
			gp2ap_i2c_write( REG_ADR_00, &value, client ) ;
			value = ( INTVAL_16 | IS_110 | PIN_DETECT | FREQ_327_5 );
			gp2ap_i2c_write( REG_ADR_03, &value, client ) ;
			value = ( OP_RUN | OP_CONTINUOUS | OP_PS ) ;		// PS mode
			gp2ap_i2c_write( REG_ADR_00, &value, client ) ;
		}
		atomic_set(&obj->als_deb_on, 0);
	}
	return 0;
}
/********************************************************************/
long gp2ap030a_read_ps(struct i2c_client *client, u16 *data)
{
	long res =-1;
	u8			rdata[2]= {0} ;

	msleep( 20 ) ;
	res = hwmsen_read_block(client,REG_ADR_10,rdata,0x02);
	if(res!= 0)
	{
		APS_ERR("i2c_master_send function err\n");
		goto READ_PS_EXIT_ERR;
	}

	*data = ( rdata[1] << 8 ) | rdata[0] ;

	return 0;
	READ_PS_EXIT_ERR:
	return res;

}

/********************************************************************/
long gp2ap030a_read_als(struct i2c_client *client, u16 *data)
{
	struct gp2ap030a_priv *obj = i2c_get_clientdata(client);	 
	u32 	data_als0 ;
	u32 	data_als1 ;
	u32 	lux ;
	u32 	ratio ;
	u8		value ;
	u8		rdata[2] ;
	long res;
	APS_FUN(f);

	res = hwmsen_read_block(client,REG_ADR_0C,rdata,0x02);
	if(res < 0)
	{
		APS_ERR("i2c_master_send function err\n");
		goto READ_ALS_EXIT_ERR;
	}

	data_als0 = ( rdata[1] << 8 ) | rdata[0] ;
	res = hwmsen_read_block(client,REG_ADR_0E,rdata,0x02);
	if(res < 0)
	{
		APS_ERR("i2c_master_send function err\n");
		goto READ_ALS_EXIT_ERR;
	}

	data_als1 = ( rdata[1] << 8 ) | rdata[0] ;

	if( obj->als_mode == HIGH_LUX_MODE )
	{
		data_als0 = 16 * data_als0 ;
		data_als1 = 16 * data_als1 ;
	}

	/////	Lux Calculation   ////////////////////////
	//	Conditions in case there is no panel.
	//	lux = c * (a * Clear - b * IR)
	//	ratio			a		b		c
	//	ratio < 0.41	1.4 	0		0.423
	//	ratio < 0.52	4.57	7.8 	0.423
	//	ratio < 0.9 	1.162	1.249	0.423
	//	ratio >= 0.9	als_lux_prev
	
	if( data_als0 == 0 )
	{
		ratio = 100 ;
	}
	else
	{
		ratio = ( data_als1 * 100 ) / data_als0 ;
	}

	if( ( ( data_als0 == 0 ) || ( data_als1 == 0 ) )
			&& ( ( data_als0 < 10 ) && ( data_als1 < 10 ) )
				&& ( obj->als_mode == LOW_LUX_MODE ) )
	{
		lux = 0 ;
	}
	else if( ratio <= 41 )	//ratio < 0.41
	{
//		lux = 0.423 * ( 1.4 * data_als0 ) ;
		lux = ( 423 * ( ( 1400 * data_als0 ) / 1000 ) ) / 1000 ;
	}
	else if( ratio <= 52 )	//ratio < 0.52
	{
//		lux = 0.423 * ( 4.57 * data_als0 - 7.8 * data_als1 ) ;
		lux = ( 423 * ( ( 4570 * data_als0 - 7800 * data_als1 ) / 1000 ) ) / 1000 ;
	}
	else if( ratio <= 90 )	//ratio < 0.9
	{
//		lux = 0.423 * ( 1.162 * data_als0 - 1.249 * data_als1 ) ;
		lux = ( 423 * ( ( 1162 * data_als0 - 1249 * data_als1 ) / 1000 ) ) / 1000 ;
	}
	else
	{
		lux = obj->als_lux_prev ;
	}
	obj->als_lux_prev = lux ;

	*data = lux;
	/////	Lux mode (Range) change    ////////////////////////
	if( ( data_als0 >= 16000 ) && ( obj->als_mode == LOW_LUX_MODE ) )
	{
		obj->als_mode = HIGH_LUX_MODE ;
		printk( KERN_INFO "change lux mode high!! \n" ) ;
		value = OP_SHUTSOWN ;
		hwmsen_read_byte_sr(obj->client,REG_ADR_00,&value);
		hwmsen_read_byte_sr(obj->client,REG_ADR_01,rdata);
		value = ( u8 )( rdata[0] & 0xC0 ) | ( RES_A_14 | RANGE_A_128 ) ;
		gp2ap_i2c_write( REG_ADR_01, &value, obj->client ) ;

		if(1 == test_bit(CMC_BIT_PS,  &obj->enable))
		{
			value = ( OP_RUN | OP_CONTINUOUS | OP_PS_ALS ) ;
		}
		else
		{
			value = ( OP_RUN | OP_CONTINUOUS | OP_ALS ) ;
		}
		gp2ap_i2c_write( REG_ADR_00, &value, obj->client ) ;
	}
	else if( ( data_als0 < 1000 ) & ( obj->als_mode == HIGH_LUX_MODE ) )
	{
		obj->als_mode = LOW_LUX_MODE ;
		printk( KERN_INFO "change lux mode low!! \n" ) ;
		value = OP_SHUTSOWN ;
		gp2ap_i2c_write( REG_ADR_00, &value, obj->client ) ;
		hwmsen_read_byte_sr(obj->client,REG_ADR_01,rdata);
		value = ( u8 )( rdata[0] & 0xC0 ) | ( RES_A_14 | RANGE_A_8 ) ;
		gp2ap_i2c_write( REG_ADR_01, &value, obj->client ) ;
		if(1 == test_bit(CMC_BIT_PS,  &obj->enable))
		{
			value = ( OP_RUN | OP_CONTINUOUS | OP_PS_ALS ) ;
		}
		else
		{
			value = ( OP_RUN | OP_CONTINUOUS | OP_ALS ) ;
		}
		gp2ap_i2c_write( REG_ADR_00, &value, obj->client ) ;
	}
	return 0;
	READ_ALS_EXIT_ERR:
	return res;

}

/********************************************************************/
static int gp2ap030a_get_ps_value(struct gp2ap030a_priv *obj, u16 ps)
{
	int val = 0;
	int mask = atomic_read(&obj->ps_mask);
	int invalid = 0;

	if(ps > atomic_read(&obj->ps_thd_val_high))
	{
		val = 0;  /*close*/
	}
	else if(ps < atomic_read(&obj->ps_thd_val_low))
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
		if(unlikely(atomic_read(&obj->trace) & CMC_TRC_CVT_PS))
		{
			
			if(mask)
			{
				APS_ERR("PS:  %05d => %05d [M] \n", ps, val);
			}
			else
			{
				APS_ERR("PS:  %05d => %05d\n", ps, val);
			}
		}
		if(0 == test_bit(CMC_BIT_PS,  &obj->enable))
		{
		  //if ps is disable do not report value
		  APS_ERR("PS: not enable and do not report this value\n");
		  return -1;
		}
		else
		{
		   return val;
		}
		
	}	
	else
	{
		if(unlikely(atomic_read(&obj->trace) & CMC_TRC_CVT_PS))
		{
			APS_DBG("PS:  %05d => %05d (-1)\n", ps, val);    
		}
		return -1;
	}	
	return val;
}
/********************************************************************/
static int gp2ap030a_get_als_value(struct gp2ap030a_priv *obj, u16 als)
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


/*-------------------------------attribute file for debugging----------------------------------*/

/******************************************************************************
 * Sysfs attributes
*******************************************************************************/
static ssize_t gp2ap030a_show_config(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	
	if(!gp2ap030a_obj)
	{
		APS_ERR("gp2ap030a_obj is null!!\n");
		return 0;
	}
	
	res = snprintf(buf, PAGE_SIZE, "(%d %d %d %d %d)\n", 
		atomic_read(&gp2ap030a_obj->i2c_retry), atomic_read(&gp2ap030a_obj->als_debounce), 
		atomic_read(&gp2ap030a_obj->ps_mask), atomic_read(&gp2ap030a_obj->ps_thd_val), atomic_read(&gp2ap030a_obj->ps_debounce));     
	return res;    
}
/*----------------------------------------------------------------------------*/
static ssize_t gp2ap030a_store_config(struct device_driver *ddri, const char *buf, size_t count)
{
	int retry, als_deb, ps_deb, mask, thres;
	if(!gp2ap030a_obj)
	{
		APS_ERR("gp2ap030a_obj is null!!\n");
		return 0;
	}
	
	if(5 == sscanf(buf, "%d %d %d %d %d", &retry, &als_deb, &mask, &thres, &ps_deb))
	{ 
		atomic_set(&gp2ap030a_obj->i2c_retry, retry);
		atomic_set(&gp2ap030a_obj->als_debounce, als_deb);
		atomic_set(&gp2ap030a_obj->ps_mask, mask);
		atomic_set(&gp2ap030a_obj->ps_thd_val, thres);        
		atomic_set(&gp2ap030a_obj->ps_debounce, ps_deb);
	}
	else
	{
		APS_ERR("invalid content: '%s', length = %d\n", buf, count);
	}
	return count;    
}
/*----------------------------------------------------------------------------*/
static ssize_t gp2ap030a_show_trace(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	if(!gp2ap030a_obj)
	{
		APS_ERR("gp2ap030a_obj is null!!\n");
		return 0;
	}

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&gp2ap030a_obj->trace));     
	return res;    
}
/*----------------------------------------------------------------------------*/
static ssize_t gp2ap030a_store_trace(struct device_driver *ddri, const char *buf, size_t count)
{
    int trace;
    if(!gp2ap030a_obj)
	{
		APS_ERR("gp2ap030a_obj is null!!\n");
		return 0;
	}
	
	if(1 == sscanf(buf, "0x%x", &trace))
	{
		atomic_set(&gp2ap030a_obj->trace, trace);
	}
	else 
	{
		APS_ERR("invalid content: '%s', length = %d\n", buf, count);
	}
	return count;    
}
/*----------------------------------------------------------------------------*/
static ssize_t gp2ap030a_show_als(struct device_driver *ddri, char *buf)
{
	int res;
	
	if(!gp2ap030a_obj)
	{
		APS_ERR("gp2ap030a_obj is null!!\n");
		return 0;
	}
	if((res = gp2ap030a_read_als(gp2ap030a_obj->client, &gp2ap030a_obj->als)))
	{
		return snprintf(buf, PAGE_SIZE, "ERROR: %d\n", res);
	}
	else
	{
		return snprintf(buf, PAGE_SIZE, "0x%04X\n", gp2ap030a_obj->als);     
	}
}
/*----------------------------------------------------------------------------*/
static ssize_t gp2ap030a_show_ps(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	if(!gp2ap030a_obj)
	{
		APS_ERR("gp2ap030a_obj is null!!\n");
		return 0;
	}
	
	if((res = gp2ap030a_read_ps(gp2ap030a_obj->client, &gp2ap030a_obj->ps)))
	{
		return snprintf(buf, PAGE_SIZE, "ERROR: %d\n", res);
	}
	else
	{
		return snprintf(buf, PAGE_SIZE, "0x%04X\n", gp2ap030a_obj->ps);     
	}
}
/*----------------------------------------------------------------------------*/
static ssize_t gp2ap030a_show_reg(struct device_driver *ddri, char *buf)
{
	if(!gp2ap030a_obj)
	{
		APS_ERR("gp2ap030a_obj is null!!\n");
		return 0;
	}
	
	
	return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t gp2ap030a_show_send(struct device_driver *ddri, char *buf)
{
    return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t gp2ap030a_store_send(struct device_driver *ddri, const char *buf, size_t count)
{
	int addr, cmd;
	u8 dat;

	if(!gp2ap030a_obj)
	{
		APS_ERR("gp2ap030a_obj is null!!\n");
		return 0;
	}
	else if(2 != sscanf(buf, "%x %x", &addr, &cmd))
	{
		APS_ERR("invalid format: '%s'\n", buf);
		return 0;
	}

	dat = (u8)cmd;
	//****************************
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t gp2ap030a_show_recv(struct device_driver *ddri, char *buf)
{
    return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t gp2ap030a_store_recv(struct device_driver *ddri, const char *buf, size_t count)
{
	int addr;
	//u8 dat;
	if(!gp2ap030a_obj)
	{
		APS_ERR("gp2ap030a_obj is null!!\n");
		return 0;
	}
	else if(1 != sscanf(buf, "%x", &addr))
	{
		APS_ERR("invalid format: '%s'\n", buf);
		return 0;
	}

	//****************************
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t gp2ap030a_show_status(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	
	if(!gp2ap030a_obj)
	{
		APS_ERR("gp2ap030a_obj is null!!\n");
		return 0;
	}
	
	if(gp2ap030a_obj->hw)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: %d, (%d %d)\n", 
			gp2ap030a_obj->hw->i2c_num, gp2ap030a_obj->hw->power_id, gp2ap030a_obj->hw->power_vol);
	}
	else
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");
	}
	
	len += snprintf(buf+len, PAGE_SIZE-len, "REGS: %02X %02X %02X %02lX %02lX\n", 
				atomic_read(&gp2ap030a_obj->als_cmd_val), atomic_read(&gp2ap030a_obj->ps_cmd_val), 
				atomic_read(&gp2ap030a_obj->ps_thd_val),gp2ap030a_obj->enable, gp2ap030a_obj->pending_intr);
	
	len += snprintf(buf+len, PAGE_SIZE-len, "MISC: %d %d\n", atomic_read(&gp2ap030a_obj->als_suspend), atomic_read(&gp2ap030a_obj->ps_suspend));

	return len;
}
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
#define IS_SPACE(CH) (((CH) == ' ') || ((CH) == '\n'))
/*----------------------------------------------------------------------------*/
static int read_int_from_buf(struct gp2ap030a_priv *obj, const char* buf, size_t count, u32 data[], int len)
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
static ssize_t gp2ap030a_show_alslv(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	int idx;
	if(!gp2ap030a_obj)
	{
		APS_ERR("gp2ap030a_obj is null!!\n");
		return 0;
	}
	
	for(idx = 0; idx < gp2ap030a_obj->als_level_num; idx++)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "%d ", gp2ap030a_obj->hw->als_level[idx]);
	}
	len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	return len;    
}
/*----------------------------------------------------------------------------*/
static ssize_t gp2ap030a_store_alslv(struct device_driver *ddri, const char *buf, size_t count)
{
	if(!gp2ap030a_obj)
	{
		APS_ERR("gp2ap030a_obj is null!!\n");
		return 0;
	}
	else if(!strcmp(buf, "def"))
	{
		memcpy(gp2ap030a_obj->als_level, gp2ap030a_obj->hw->als_level, sizeof(gp2ap030a_obj->als_level));
	}
	else if(gp2ap030a_obj->als_level_num != read_int_from_buf(gp2ap030a_obj, buf, count, 
			gp2ap030a_obj->hw->als_level, gp2ap030a_obj->als_level_num))
	{
		APS_ERR("invalid format: '%s'\n", buf);
	}    
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t gp2ap030a_show_alsval(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	int idx;
	if(!gp2ap030a_obj)
	{
		APS_ERR("gp2ap030a_obj is null!!\n");
		return 0;
	}
	
	for(idx = 0; idx < gp2ap030a_obj->als_value_num; idx++)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "%d ", gp2ap030a_obj->hw->als_value[idx]);
	}
	len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	return len;    
}
/*----------------------------------------------------------------------------*/
static ssize_t gp2ap030a_store_alsval(struct device_driver *ddri, const char *buf, size_t count)
{
	if(!gp2ap030a_obj)
	{
		APS_ERR("gp2ap030a_obj is null!!\n");
		return 0;
	}
	else if(!strcmp(buf, "def"))
	{
		memcpy(gp2ap030a_obj->als_value, gp2ap030a_obj->hw->als_value, sizeof(gp2ap030a_obj->als_value));
	}
	else if(gp2ap030a_obj->als_value_num != read_int_from_buf(gp2ap030a_obj, buf, count, 
			gp2ap030a_obj->hw->als_value, gp2ap030a_obj->als_value_num))
	{
		APS_ERR("invalid format: '%s'\n", buf);
	}    
	return count;
}
/*---------------------------------------------------------------------------------------*/
static DRIVER_ATTR(als,     S_IWUSR | S_IRUGO, gp2ap030a_show_als, NULL);
static DRIVER_ATTR(ps,      S_IWUSR | S_IRUGO, gp2ap030a_show_ps, NULL);
static DRIVER_ATTR(config,  S_IWUSR | S_IRUGO, gp2ap030a_show_config,	gp2ap030a_store_config);
static DRIVER_ATTR(alslv,   S_IWUSR | S_IRUGO, NULL, NULL);
static DRIVER_ATTR(alsval,  S_IWUSR | S_IRUGO, NULL, NULL);
static DRIVER_ATTR(trace,   S_IWUSR | S_IRUGO, gp2ap030a_show_trace,		gp2ap030a_store_trace);
static DRIVER_ATTR(status,  S_IWUSR | S_IRUGO, NULL, NULL);
static DRIVER_ATTR(send,    S_IWUSR | S_IRUGO, NULL, NULL);
static DRIVER_ATTR(recv,    S_IWUSR | S_IRUGO, NULL, NULL);
static DRIVER_ATTR(reg,     S_IWUSR | S_IRUGO, gp2ap030a_show_reg, NULL);
/*----------------------------------------------------------------------------*/
static struct driver_attribute *gp2ap030a_attr_list[] = {
    &driver_attr_als,
    &driver_attr_ps,    
    &driver_attr_trace,        /*trace log*/
    &driver_attr_config,
    &driver_attr_alslv,
    &driver_attr_alsval,
    &driver_attr_status,
    &driver_attr_send,
    &driver_attr_recv,
    &driver_attr_reg,
};

/*----------------------------------------------------------------------------*/
static int gp2ap030a_create_attr(struct device_driver *driver) 
{
	int idx, err = 0;
	int num = (int)(sizeof(gp2ap030a_attr_list)/sizeof(gp2ap030a_attr_list[0]));
	if (driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
		if((err = driver_create_file(driver, gp2ap030a_attr_list[idx])))
		{            
			APS_ERR("driver_create_file (%s) = %d\n", gp2ap030a_attr_list[idx]->attr.name, err);
			break;
		}
	}    
	return err;
}
/*----------------------------------------------------------------------------*/
	static int gp2ap030a_delete_attr(struct device_driver *driver)
	{
	int idx ,err = 0;
	int num = (int)(sizeof(gp2ap030a_attr_list)/sizeof(gp2ap030a_attr_list[0]));

	if (!driver)
	return -EINVAL;

	for (idx = 0; idx < num; idx++) 
	{
		driver_remove_file(driver, gp2ap030a_attr_list[idx]);
	}
	
	return err;
}
/*----------------------------------------------------------------------------*/


/*----------------------------------interrupt functions--------------------------------*/
static int intr_flag = 0;
/*----------------------------------------------------------------------------*/
static int gp2ap030a_check_intr(struct i2c_client *client) 
{
	struct gp2ap030a_priv *obj = i2c_get_clientdata(client);
	int res;
	u8 buffer[2];
	u8			rdata[18] ;

	if (mt_get_gpio_in(GPIO_ALS_EINT_PIN) == 1) /*skip if no interrupt*/  
		return 0;
#if 0
	res = hwmsen_read_byte_sr(client,REG_ADR_00,buffer);
	if(res < 0)
	{
		goto EXIT_ERR;
	}

	APS_ERR("gp2ap030a_check_intr status=0x%x\n", buffer[0]);
#endif
//just support interrupt for ps

	if((1 == test_bit(CMC_BIT_PS,  &obj->enable))
		&&(gp2ap030a_obj->hw->polling_mode_ps == 0))
	{
		set_bit(CMC_BIT_PS, &obj->pending_intr);
	}
	else
	{
		clear_bit(CMC_BIT_PS, &obj->pending_intr);
	}
	clear_bit(CMC_BIT_ALS, &obj->pending_intr);
	#if 0
	if(buffer[0]&0x02)
	{
		set_bit(CMC_BIT_ALS, &obj->pending_intr);
	}
	else
	{
		clear_bit(CMC_BIT_ALS, &obj->pending_intr);
	}
	#endif
	if(atomic_read(&obj->trace) & CMC_TRC_DEBUG)
	{
		APS_LOG("check intr: 0x%08lX\n", obj->pending_intr);
	}
	return res;

EXIT_ERR:
	APS_ERR("gp2ap030a_check_intr fail\n");
	return 1;
}
/*----------------------------------------------------------------------------*/
static void gp2ap030a_eint_work(struct work_struct *work)
{
	struct gp2ap030a_priv *obj = (struct gp2ap030a_priv *)container_of(work, struct gp2ap030a_priv, eint_work);
	
	hwm_sensor_data sensor_data;
	int res = 0;
	
	msleep( 20 ) ;

	res = gp2ap030a_check_intr(obj->client);
	if(res != 0)
	{
		APS_ERR("check intrs: %d\n", res);
		goto EXIT_INTR_ERR;
	}

	if((1<<CMC_BIT_ALS) & obj->pending_intr)
	{
	  //get raw data
	  APS_LOG(" als change\n");
	  if((res = gp2ap030a_read_als(obj->client, &obj->als)))
	  {
		 APS_ERR("cm3623 read als data: %d\n", res);
	  }
	  //map and store data to hwm_sensor_data
	 
 	  sensor_data.values[0] = gp2ap030a_get_als_value(obj, obj->als);
	  sensor_data.value_divide = 1;
	  sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
	  //APS_LOG("als raw %x -> value %d \n", obj->als,sensor_data.values[0]);
	  //let up layer to know
	  if((res = hwmsen_get_interrupt_data(ID_LIGHT, &sensor_data)))
	  {
		APS_ERR("call hwmsen_get_interrupt_data fail = %d\n", res);
	  }
	  
	}
	if((1<<CMC_BIT_PS) &  obj->pending_intr)
	{
	  //get raw data
	  APS_LOG(" ps change\n");
	  if((res = gp2ap030a_read_ps(obj->client, &obj->ps)))
	  {
		 APS_ERR("cm3623 read ps data: %d\n", res);
		 goto EXIT_INTR_ERR;
	  }
	  //map and store data to hwm_sensor_data
	  sensor_data.values[0] = gp2ap030a_get_ps_value(obj, obj->ps);
	  sensor_data.value_divide = 1;
	  sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
	  //let up layer to know
	  if((res = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data)))
	  {
		APS_ERR("call hwmsen_get_interrupt_data fail = %d\n", res);
		goto EXIT_INTR_ERR;
	  }
	}

	
	mt65xx_eint_unmask(CUST_EINT_ALS_NUM);
	return;
	EXIT_INTR_ERR:
	mt65xx_eint_unmask(CUST_EINT_ALS_NUM);

	//APS_ERR("gp2ap030a_eint_work err: %d\n", res);

}
/*----------------------------------------------------------------------------*/
static void gp2ap030a_eint_func(void)
{
	struct gp2ap030a_priv *obj = g_gp2ap030a_ptr;
	if(!obj)
	{
		return;
	}	
	schedule_work(&obj->eint_work);
}

int gp2ap030a_setup_eint(struct i2c_client *client)
{
	struct gp2ap030a_priv *obj = i2c_get_clientdata(client);        

	g_gp2ap030a_ptr = obj;
	
	mt_set_gpio_dir(GPIO_ALS_EINT_PIN, GPIO_DIR_IN);
	mt_set_gpio_mode(GPIO_ALS_EINT_PIN, GPIO_ALS_EINT_PIN_M_EINT);
	mt_set_gpio_pull_enable(GPIO_ALS_EINT_PIN, TRUE);
	mt_set_gpio_pull_select(GPIO_ALS_EINT_PIN, GPIO_PULL_UP);

	mt65xx_eint_set_sens(CUST_EINT_ALS_NUM, CUST_EINT_ALS_SENSITIVE);
	mt65xx_eint_set_polarity(CUST_EINT_ALS_NUM, CUST_EINT_ALS_POLARITY);
	mt65xx_eint_set_hw_debounce(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_CN);
	mt65xx_eint_registration(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_EN, CUST_EINT_ALS_POLARITY, gp2ap030a_eint_func, 0);

	mt65xx_eint_unmask(CUST_EINT_ALS_NUM);  
    return 0;
}
/*-------------------------------MISC device related------------------------------------------*/



/************************************************************/
static int gp2ap030a_open(struct inode *inode, struct file *file)
{
	file->private_data = gp2ap030a_i2c_client;

	if (!file->private_data)
	{
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}
	return nonseekable_open(inode, file);
}
/************************************************************/
static int gp2ap030a_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}
/************************************************************/
static long gp2ap030a_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
		struct i2c_client *client = (struct i2c_client*)file->private_data;
		struct gp2ap030a_priv *obj = i2c_get_clientdata(client);  
		long err = 0;
		void __user *ptr = (void __user*) arg;
		int dat;
		uint32_t enable;
		//int ps_result;
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
					if((err = gp2ap030a_enable_ps(obj->client, 1)))
					{
						APS_ERR("enable ps fail: %ld\n", err); 
						goto err_out;
					}
					
					set_bit(CMC_BIT_PS, &obj->enable);
				}
				else
				{
					if((err = gp2ap030a_enable_ps(obj->client, 0)))
					{
						APS_ERR("disable ps fail: %ld\n", err); 
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
				
				if((err = gp2ap030a_read_ps(obj->client, &obj->ps)))
				{
					goto err_out;
				}
				
				dat = gp2ap030a_get_ps_value(obj, obj->ps);
				if(copy_to_user(ptr, &dat, sizeof(dat)))
				{
					err = -EFAULT;
					goto err_out;
				}  
				break;
	
			case ALSPS_GET_PS_RAW_DATA:   
				
				if((err = gp2ap030a_read_ps(obj->client, &obj->ps)))
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
					if((err = gp2ap030a_enable_als(obj->client, 1)))
					{
						APS_ERR("enable als fail: %ld\n", err); 
						goto err_out;
					}
					set_bit(CMC_BIT_ALS, &obj->enable);
				}
				else
				{
					if((err = gp2ap030a_enable_als(obj->client, 0)))
					{
						APS_ERR("disable als fail: %ld\n", err); 
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
				if((err = gp2ap030a_read_als(obj->client, &obj->als)))
				{
					goto err_out;
				}
	
				dat = gp2ap030a_get_als_value(obj, obj->als);
				if(copy_to_user(ptr, &dat, sizeof(dat)))
				{
					err = -EFAULT;
					goto err_out;
				}			   
				break;
	
			case ALSPS_GET_ALS_RAW_DATA:					
				if((err = gp2ap030a_read_als(obj->client, &obj->als)))
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

			/*----------------------------------for factory mode test---------------------------------------*/
			#if 0
			case ALSPS_GET_PS_TEST_RESULT:
				if((err = gp2ap030a_read_ps(obj->client, &obj->ps)))
				{
					goto err_out;
				}
				if(obj->ps > atomic_read(&obj->ps_thd_val_high))
					{
						ps_result = 0;
					}
				else	ps_result = 1;
				
				if(copy_to_user(ptr, &ps_result, sizeof(ps_result)))
				{
					err = -EFAULT;
					goto err_out;
				}			   
				break;
			/*------------------------------------------------------------------------------------------*/
			#endif
			default:
				APS_ERR("%s not supported = 0x%04x", __FUNCTION__, cmd);
				err = -ENOIOCTLCMD;
				break;
		}
	
		err_out:
		return err;    
	}
/********************************************************************/
/*------------------------------misc device related operation functions------------------------------------*/
static struct file_operations gp2ap030a_fops = {
	.owner = THIS_MODULE,
	.open = gp2ap030a_open,
	.release = gp2ap030a_release,
	.unlocked_ioctl = gp2ap030a_unlocked_ioctl,
};

static struct miscdevice gp2ap030a_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "als_ps",
	.fops = &gp2ap030a_fops,
};

/*--------------------------------------------------------------------------------------*/
static void gp2ap030a_early_suspend(struct early_suspend *h)
{
		struct gp2ap030a_priv *obj = container_of(h, struct gp2ap030a_priv, early_drv);	
		int err;
		APS_FUN();	  
	
		if(!obj)
		{
			APS_ERR("null pointer!!\n");
			return;
		}
		
		atomic_set(&obj->als_suspend, 1);
		if((err = gp2ap030a_enable_als(obj->client, 0)))
		{
			APS_ERR("disable als fail: %d\n", err); 
		}
}

static void gp2ap030a_late_resume(struct early_suspend *h) 
{
		struct gp2ap030a_priv *obj = container_of(h, struct gp2ap030a_priv, early_drv);		  
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
		if(test_bit(CMC_BIT_ALS, &obj->enable))
		{
			if((err = gp2ap030a_enable_als(obj->client, 1)))
			{
				APS_ERR("enable als fail: %d\n", err);		  
	
			}
		}

}
/*--------------------------------------------------------------------------------*/
static void gp2ap_init_device( struct gp2ap030a_priv *data )
{
	int		i ;

	for( i = 1 ; i < sizeof( data->regData ) ; i++ )
	{
		gp2ap_i2c_write( i, &data->regData[i], data->client ) ;
	}
}

static int gp2ap030a_init_client(struct i2c_client *client)
{
	int 	i ;
	int res = 0;

	for( i = 1 ; i < sizeof( gp2ap_init_data) ; i++ )
	{
		res = gp2ap_i2c_write( i, &gp2ap_init_data[i], client ) ;
		if(res!=0)
		{
			APS_ERR("i2c_master_send function err\n");
			goto EXIT_ERR;
		}
	}
	res = gp2ap030a_setup_eint(client);
	if(res!=0)
	{
		APS_ERR("setup eint: %d\n", res);
		return res;
	}
	
	return 0;
	
	EXIT_ERR:
	APS_ERR("init dev: %d\n", res);
	return res;

}

/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/
long gp2ap030a_ps_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
		long err = 0;
		int value;
		hwm_sensor_data* sensor_data;
		struct gp2ap030a_priv *obj = (struct gp2ap030a_priv *)self;		
		APS_FUN(f);
		switch (command)
		{
			case SENSOR_DELAY:
				if((buff_in == NULL) || (size_in < sizeof(int)))
				{
					APS_ERR("Set delay parameter error!\n");
					err = -EINVAL;
				}
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
						if((err = gp2ap030a_enable_ps(obj->client, 1)))
						{
							APS_ERR("enable ps fail: %d\n", err); 
							return -1;
						}
						set_bit(CMC_BIT_PS, &obj->enable);
					}
					else
					{
						if((err = gp2ap030a_enable_ps(obj->client, 0)))
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
					
					if((err = gp2ap030a_read_ps(obj->client, &obj->ps)))
					{
						err = -1;;
					}
					else
					{
						sensor_data->values[0] = gp2ap030a_get_ps_value(obj, obj->ps);
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

long gp2ap030a_als_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
		long err = 0;
		int value;
		hwm_sensor_data* sensor_data;
		struct gp2ap030a_priv *obj = (struct gp2ap030a_priv *)self;
		APS_FUN(f);
		switch (command)
		{
			case SENSOR_DELAY:
				if((buff_in == NULL) || (size_in < sizeof(int)))
				{
					APS_ERR("Set delay parameter error!\n");
					err = -EINVAL;
				}
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
						if((err = gp2ap030a_enable_als(obj->client, 1)))
						{
							APS_ERR("enable als fail: %d\n", err); 
							return -1;
						}
						set_bit(CMC_BIT_ALS, &obj->enable);
					}
					else
					{
						if((err = gp2ap030a_enable_als(obj->client, 0)))
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
									
					if((err = gp2ap030a_read_als(obj->client, &obj->als)))
					{
						err = -1;;
					}
					else
					{
						sensor_data->values[0] = gp2ap030a_get_als_value(obj, obj->als);
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
/*--------------------------------------------------------------------------------*/


/*-----------------------------------i2c operations----------------------------------*/
static int gp2ap030a_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct gp2ap030a_priv *obj;
	struct hwmsen_object obj_ps, obj_als;
	int err = 0;
	int i =0;
	if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
	{
		err = -ENOMEM;
		goto exit;
	}
	
	memset(obj, 0, sizeof(*obj));
	gp2ap030a_obj = obj;
	
	obj->hw = get_cust_alsps_hw();//get custom file data struct
	
	INIT_WORK(&obj->eint_work, gp2ap030a_eint_work);

	obj->client = client;
	i2c_set_clientdata(client, obj);

	/*-----------------------------value need to be confirmed-----------------------------------------*/
	atomic_set(&obj->als_debounce, 200);
	atomic_set(&obj->als_deb_on, 0);
	atomic_set(&obj->als_deb_end, 0);
	atomic_set(&obj->ps_debounce, 200);
	atomic_set(&obj->ps_deb_on, 0);
	atomic_set(&obj->ps_deb_end, 0);
	atomic_set(&obj->ps_mask, 0);
	atomic_set(&obj->als_suspend, 0);
	atomic_set(&obj->als_cmd_val, 0xDF);
	atomic_set(&obj->ps_cmd_val,  0xC1);
	atomic_set(&obj->ps_thd_val_high,  obj->hw->ps_threshold_high);
	atomic_set(&obj->ps_thd_val_low,  obj->hw->ps_threshold_low);
	
	obj->enable = 0;
	obj->pending_intr = 0;
	obj->als_level_num = sizeof(obj->hw->als_level)/sizeof(obj->hw->als_level[0]);
	obj->als_value_num = sizeof(obj->hw->als_value)/sizeof(obj->hw->als_value[0]);
	obj->als_mode = LOW_LUX_MODE;
	/*-----------------------------value need to be confirmed-----------------------------------------*/
	
	BUG_ON(sizeof(obj->als_level) != sizeof(obj->hw->als_level));
	memcpy(obj->als_level, obj->hw->als_level, sizeof(obj->als_level));
	BUG_ON(sizeof(obj->als_value) != sizeof(obj->hw->als_value));
	memcpy(obj->als_value, obj->hw->als_value, sizeof(obj->als_value));
	atomic_set(&obj->i2c_retry, 3);
	set_bit(CMC_BIT_ALS, &obj->enable);
	set_bit(CMC_BIT_PS, &obj->enable);
	
	for( i = 0 ; i < sizeof( gp2ap_init_data ) ; i++ )
	{
		obj->regData[i] = gp2ap_init_data[i] ;
	}

	gp2ap030a_i2c_client = client;

	if((err = gp2ap030a_init_client(client)))
	{
		goto exit_init_failed;
	}
	APS_LOG("gp2ap030a_init_client() OK!\n");

	if((err = misc_register(&gp2ap030a_device)))
	{
		APS_ERR("gp2ap030a_device register failed\n");
		goto exit_misc_device_register_failed;
	}
	APS_LOG("gp2ap030a_device misc_register OK!\n");

	/*------------------------gp2ap030a attribute file for debug--------------------------------------*/
	if((err = gp2ap030a_create_attr(&gp2ap030a_alsps_driver.driver)))
	{
		APS_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}
	/*------------------------gp2ap030a attribute file for debug--------------------------------------*/

	obj_ps.self = gp2ap030a_obj;
	obj_ps.polling = obj->hw->polling_mode_ps;	
	obj_ps.sensor_operate = gp2ap030a_ps_operate;
	if((err = hwmsen_attach(ID_PROXIMITY, &obj_ps)))
	{
		APS_ERR("attach fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}
		
	obj_als.self = gp2ap030a_obj;
	obj_als.polling = obj->hw->polling_mode_als;
	obj_als.sensor_operate = gp2ap030a_als_operate;
	if((err = hwmsen_attach(ID_LIGHT, &obj_als)))
	{
		APS_ERR("attach fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

	#if defined(CONFIG_HAS_EARLYSUSPEND)
	obj->early_drv.level    = EARLY_SUSPEND_LEVEL_STOP_DRAWING - 2,
	obj->early_drv.suspend  = gp2ap030a_early_suspend,
	obj->early_drv.resume   = gp2ap030a_late_resume,    
	register_early_suspend(&obj->early_drv);
	#endif

	APS_LOG("%s: OK\n", __func__);
	return 0;

	exit_create_attr_failed:
	exit_sensor_obj_attach_fail:
	exit_misc_device_register_failed:
		misc_deregister(&gp2ap030a_device);
	exit_init_failed:
		kfree(obj);
	exit:
	gp2ap030a_i2c_client = NULL;           
	APS_ERR("%s: err = %d\n", __func__, err);
	return err;
}

static int gp2ap030a_i2c_remove(struct i2c_client *client)
{
	int err;	
	/*------------------------gp2ap030a attribute file for debug--------------------------------------*/	
	if((err = gp2ap030a_delete_attr(&gp2ap030a_i2c_driver.driver)))
	{
		APS_ERR("gp2ap030a_delete_attr fail: %d\n", err);
	} 
	/*----------------------------------------------------------------------------------------*/
	
	if((err = misc_deregister(&gp2ap030a_device)))
	{
		APS_ERR("misc_deregister fail: %d\n", err);    
	}
		
	gp2ap030a_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));
	return 0;

}
#if 0
static int gp2ap030a_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	strcpy(info->type, GP2AP030A_DEV_NAME);
	return 0;

}
#endif
static int gp2ap030a_i2c_suspend(struct i2c_client *client, pm_message_t msg)
{
	APS_FUN();
	return 0;
}

static int gp2ap030a_i2c_resume(struct i2c_client *client)
{
	APS_FUN();
	return 0;
}

/*----------------------------------------------------------------------------*/

static int gp2ap030a_probe(struct platform_device *pdev) 
{
	APS_FUN();  
	struct alsps_hw *hw = get_cust_alsps_hw();

	gp2ap030a_power(hw, 1); //*****************   
	
	if(i2c_add_driver(&gp2ap030a_i2c_driver))
	{
		APS_ERR("add driver error\n");
		return -1;
	} 
	return 0;
}
/*----------------------------------------------------------------------------*/
static int gp2ap030a_remove(struct platform_device *pdev)
{
	
	APS_FUN(); 
	struct alsps_hw *hw = get_cust_alsps_hw();
	
	gp2ap030a_power(hw, 0);//*****************  
	
	i2c_del_driver(&gp2ap030a_i2c_driver);
	return 0;
}



/*----------------------------------------------------------------------------*/
static struct platform_driver gp2ap030a_alsps_driver = {
	.probe      = gp2ap030a_probe,
	.remove     = gp2ap030a_remove,    
	.driver     = {
		.name  = "als_ps",
	}
};

/*----------------------------------------------------------------------------*/
static int __init gp2ap030a_init(void)
{
	APS_FUN();
	i2c_register_board_info(0, &i2c_gp2ap030a, 1);
	if(platform_driver_register(&gp2ap030a_alsps_driver))
	{
		APS_ERR("failed to register driver");
		return -ENODEV;
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit gp2ap030a_exit(void)
{
	APS_FUN();
	platform_driver_unregister(&gp2ap030a_alsps_driver);
}
/*----------------------------------------------------------------------------*/
module_init(gp2ap030a_init);
module_exit(gp2ap030a_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("fisher luo");
MODULE_DESCRIPTION("gp2ap030a driver");
MODULE_LICENSE("GPL");

