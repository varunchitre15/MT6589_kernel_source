/* BMA150 motion sensor driver
 *
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

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



#include <cust_acc.h>
#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#include "bma150.h"
#include <linux/hwmsen_helper.h>
/*----------------------------------------------------------------------------*/
#define I2C_DRIVERID_BMA150 150
/*----------------------------------------------------------------------------*/
#define DEBUG 1
/*----------------------------------------------------------------------------*/
//#define CONFIG_BMA150_LOWPASS   /*apply low pass filter on output*/       
#define SW_CALIBRATION

/*----------------------------------------------------------------------------*/
#define BMA150_AXIS_X          0
#define BMA150_AXIS_Y          1
#define BMA150_AXIS_Z          2
#define BMA150_AXES_NUM        3
#define BMA150_DATA_LEN        6
#define BMA150_DEV_NAME        "BMA150"
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static const struct i2c_device_id bma150_i2c_id[] = {{BMA150_DEV_NAME,0},{}};
/*the adapter id will be available in customization*/
static struct i2c_board_info __initdata i2c_bma150={ I2C_BOARD_INFO("BMA150", BMA150_I2C_SLAVE_WRITE_ADDR>>1)};

//static unsigned short bma150_force[] = {0x00, BMA150_I2C_SLAVE_WRITE_ADDR, I2C_CLIENT_END, I2C_CLIENT_END};
//static const unsigned short *const bma150_forces[] = { bma150_force, NULL };
//static struct i2c_client_address_data bma150_addr_data = { .forces = bma150_forces,};

/*----------------------------------------------------------------------------*/
static int bma150_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id); 
static int bma150_i2c_remove(struct i2c_client *client);
static int bma150_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);

/*----------------------------------------------------------------------------*/
typedef enum {
    ADX_TRC_FILTER  = 0x01,
    ADX_TRC_RAWDATA = 0x02,
    ADX_TRC_IOCTL   = 0x04,
    ADX_TRC_CALI	= 0X08,
    ADX_TRC_INFO	= 0X10,
} ADX_TRC;
/*----------------------------------------------------------------------------*/
struct scale_factor{
    u8  whole;
    u8  fraction;
};
/*----------------------------------------------------------------------------*/
struct data_resolution {
    struct scale_factor scalefactor;
    int                 sensitivity;
};
/*----------------------------------------------------------------------------*/
#define C_MAX_FIR_LENGTH (32)
/*----------------------------------------------------------------------------*/
struct data_filter {
    s16 raw[C_MAX_FIR_LENGTH][BMA150_AXES_NUM];
    int sum[BMA150_AXES_NUM];
    int num;
    int idx;
};
/*----------------------------------------------------------------------------*/
struct bma150_i2c_data {
    struct i2c_client *client;
    struct acc_hw *hw;
    struct hwmsen_convert   cvt;
    
    /*misc*/
    struct data_resolution *reso;
    atomic_t                trace;
    atomic_t                suspend;
    atomic_t                selftest;
	atomic_t				filter;
    s16                     cali_sw[BMA150_AXES_NUM+1];

    /*data*/
    s8                      offset[BMA150_AXES_NUM+1];  /*+1: for 4-byte alignment*/
    s16                     data[BMA150_AXES_NUM+1];

#if defined(CONFIG_BMA150_LOWPASS)
    atomic_t                firlen;
    atomic_t                fir_en;
    struct data_filter      fir;
#endif 
    /*early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
    struct early_suspend    early_drv;
#endif     
};
/*----------------------------------------------------------------------------*/
static struct i2c_driver bma150_i2c_driver = {
    .driver = {
       // .owner          = THIS_MODULE,
        .name           = BMA150_DEV_NAME,
    },
	.probe      		= bma150_i2c_probe,
	.remove    			= bma150_i2c_remove,
	.detect				= bma150_i2c_detect,
#if !defined(CONFIG_HAS_EARLYSUSPEND)    
    .suspend            = bma150_suspend,
    .resume             = bma150_resume,
#endif
	.id_table = bma150_i2c_id,
	//.address_data = &bma150_addr_data,
};

/*----------------------------------------------------------------------------*/
static struct i2c_client *bma150_i2c_client = NULL;
static struct platform_driver bma150_gsensor_driver;
static struct bma150_i2c_data *obj_i2c_data = NULL;
static bool sensor_power = true;
static GSENSOR_VECTOR3D gsensor_gain;
static char selftestRes[8]= {0}; 

/*----------------------------------------------------------------------------*/
#define GSE_TAG                  "[Gsensor] "
#define GSE_FUN(f)               printk(KERN_INFO GSE_TAG"%s\n", __FUNCTION__)
#define GSE_ERR(fmt, args...)    printk(KERN_ERR GSE_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define GSE_LOG(fmt, args...)    printk(KERN_INFO GSE_TAG fmt, ##args)
/*----------------------------------------------------------------------------*/
static struct data_resolution bma150_data_resolution[1] = {
 /* combination by {FULL_RES,RANGE}*/
    {{ 3, 9}, 256},   // dataformat +/-2g  in 10-bit resolution;  { 3, 9} = 3.9 = (2*2*1000)/(2^10);  256 = (2^10)/(2*2)          
};
/*----------------------------------------------------------------------------*/
static struct data_resolution bma150_offset_resolution = {{15, 6}, 64};

/*--------------------BMA150 power control function----------------------------------*/
static void BMA150_power(struct acc_hw *hw, unsigned int on) 
{
	static unsigned int power_on = 0;

	if(hw->power_id != POWER_NONE_MACRO)		// have externel LDO
	{        
		GSE_LOG("power %s\n", on ? "on" : "off");
		if(power_on == on)	// power status not change
		{
			GSE_LOG("ignore power control: %d\n", on);
		}
		else if(on)	// power on
		{
			if(!hwPowerOn(hw->power_id, hw->power_vol, "BMA150"))
			{
				GSE_ERR("power on fails!!\n");
			}
		}
		else	// power off
		{
			if (!hwPowerDown(hw->power_id, "BMA150"))
			{
				GSE_ERR("power off fail!!\n");
			}			  
		}
	}
	power_on = on;    
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static int BMA150_SetDataResolution(struct bma150_i2c_data *obj)
{
	int err;
	u8  dat, reso;

/*set g sensor dataresolution here*/

/*BMA150 only can set to 10-bit dataresolution, so do nothing in bma150 driver here*/

/*end of set dataresolution*/


 
 /*we set measure range from -2g to +2g in BMA150_SetDataFormat(client, BMA150_RANGE_2G), 
                                                    and set 10-bit dataresolution BMA150_SetDataResolution()*/
                                                    
 /*so bma150_data_resolution[0] set value as {{ 3, 9}, 256} when declaration, and assign the value to obj->reso here*/  

 	obj->reso = &bma150_data_resolution[0];
	return 0;
	
/*if you changed the measure range, for example call: BMA150_SetDataFormat(client, BMA150_RANGE_4G), 
you must set the right value to bma150_data_resolution*/

}
/*----------------------------------------------------------------------------*/
static int BMA150_ReadData(struct i2c_client *client, s16 data[BMA150_AXES_NUM])
{
	struct bma150_i2c_data *priv = i2c_get_clientdata(client);        
	u8 addr = BMA150_REG_DATAXLOW;
	u8 buf[BMA150_DATA_LEN] = {0};
	int err = 0;
	int i;
	int tmp=0;
	u8 ofs[3];



	if(NULL == client)
	{
		err = -EINVAL;
	}
	else if(err = hwmsen_read_block(client, addr, buf, 0x06))
	{
		GSE_ERR("error: %d\n", err);
	}
	else
	{
		data[BMA150_AXIS_X] = (s16)((buf[BMA150_AXIS_X*2] >> 6) |
		         (buf[BMA150_AXIS_X*2+1] << 2));
		data[BMA150_AXIS_Y] = (s16)((buf[BMA150_AXIS_Y*2] >> 6) |
		         (buf[BMA150_AXIS_Y*2+1] << 2));
		data[BMA150_AXIS_Z] = (s16)((buf[BMA150_AXIS_Z*2] >> 6) |
		         (buf[BMA150_AXIS_Z*2+1] << 2));

		for(i=0;i<3;i++)				
		{								//because the data is store in binary complement number formation in computer system
			if ( data[i] == 0x0200 )	//so we want to calculate actual number here
				data[i]= -512;			//10bit resolution, 512= 2^(10-1)
			else if ( data[i] & 0x0200 )//transfor format
			{							//printk("data 0 step %x \n",data[i]);
				data[i] -= 0x1;			//printk("data 1 step %x \n",data[i]);
				data[i] = ~data[i];		//printk("data 2 step %x \n",data[i]);
				data[i] &= 0x01ff;		//printk("data 3 step %x \n\n",data[i]);
				data[i] = -data[i];		
			}
		}	


		if(atomic_read(&priv->trace) & ADX_TRC_RAWDATA)
		{
			GSE_LOG("[%08X %08X %08X] => [%5d %5d %5d]\n", data[BMA150_AXIS_X], data[BMA150_AXIS_Y], data[BMA150_AXIS_Z],
		                               data[BMA150_AXIS_X], data[BMA150_AXIS_Y], data[BMA150_AXIS_Z]);
		}
#ifdef CONFIG_BMA150_LOWPASS
		if(atomic_read(&priv->filter))
		{
			if(atomic_read(&priv->fir_en) && !atomic_read(&priv->suspend))
			{
				int idx, firlen = atomic_read(&priv->firlen);   
				if(priv->fir.num < firlen)
				{                
					priv->fir.raw[priv->fir.num][BMA150_AXIS_X] = data[BMA150_AXIS_X];
					priv->fir.raw[priv->fir.num][BMA150_AXIS_Y] = data[BMA150_AXIS_Y];
					priv->fir.raw[priv->fir.num][BMA150_AXIS_Z] = data[BMA150_AXIS_Z];
					priv->fir.sum[BMA150_AXIS_X] += data[BMA150_AXIS_X];
					priv->fir.sum[BMA150_AXIS_Y] += data[BMA150_AXIS_Y];
					priv->fir.sum[BMA150_AXIS_Z] += data[BMA150_AXIS_Z];
					if(atomic_read(&priv->trace) & ADX_TRC_FILTER)
					{
						GSE_LOG("add [%2d] [%5d %5d %5d] => [%5d %5d %5d]\n", priv->fir.num,
							priv->fir.raw[priv->fir.num][BMA150_AXIS_X], priv->fir.raw[priv->fir.num][BMA150_AXIS_Y], priv->fir.raw[priv->fir.num][BMA150_AXIS_Z],
							priv->fir.sum[BMA150_AXIS_X], priv->fir.sum[BMA150_AXIS_Y], priv->fir.sum[BMA150_AXIS_Z]);
					}
					priv->fir.num++;
					priv->fir.idx++;
				}
				else
				{
					idx = priv->fir.idx % firlen;
					priv->fir.sum[BMA150_AXIS_X] -= priv->fir.raw[idx][BMA150_AXIS_X];
					priv->fir.sum[BMA150_AXIS_Y] -= priv->fir.raw[idx][BMA150_AXIS_Y];
					priv->fir.sum[BMA150_AXIS_Z] -= priv->fir.raw[idx][BMA150_AXIS_Z];
					priv->fir.raw[idx][BMA150_AXIS_X] = data[BMA150_AXIS_X];
					priv->fir.raw[idx][BMA150_AXIS_Y] = data[BMA150_AXIS_Y];
					priv->fir.raw[idx][BMA150_AXIS_Z] = data[BMA150_AXIS_Z];
					priv->fir.sum[BMA150_AXIS_X] += data[BMA150_AXIS_X];
					priv->fir.sum[BMA150_AXIS_Y] += data[BMA150_AXIS_Y];
					priv->fir.sum[BMA150_AXIS_Z] += data[BMA150_AXIS_Z];
					priv->fir.idx++;
					data[BMA150_AXIS_X] = priv->fir.sum[BMA150_AXIS_X]/firlen;
					data[BMA150_AXIS_Y] = priv->fir.sum[BMA150_AXIS_Y]/firlen;
					data[BMA150_AXIS_Z] = priv->fir.sum[BMA150_AXIS_Z]/firlen;
					if(atomic_read(&priv->trace) & ADX_TRC_FILTER)
					{
						GSE_LOG("add [%2d] [%5d %5d %5d] => [%5d %5d %5d] : [%5d %5d %5d]\n", idx,
						priv->fir.raw[idx][BMA150_AXIS_X], priv->fir.raw[idx][BMA150_AXIS_Y], priv->fir.raw[idx][BMA150_AXIS_Z],
						priv->fir.sum[BMA150_AXIS_X], priv->fir.sum[BMA150_AXIS_Y], priv->fir.sum[BMA150_AXIS_Z],
						data[BMA150_AXIS_X], data[BMA150_AXIS_Y], data[BMA150_AXIS_Z]);
					}
				}
			}
		}	
#endif         
	}
	return err;
}
/*----------------------------------------------------------------------------*/
static int BMA150_ReadOffset(struct i2c_client *client, s8 ofs[BMA150_AXES_NUM])
{    
	int err;
#ifdef SW_CALIBRATION
	ofs[0]=ofs[1]=ofs[2]=0x0;
#else
	if(err = hwmsen_read_block(client, BMA150_REG_OFSX, ofs, BMA150_AXES_NUM))
	{
		GSE_ERR("error: %d\n", err);
	}
#endif
	//printk("offesx=%x, y=%x, z=%x",ofs[0],ofs[1],ofs[2]);
	
	return err;    
}
/*----------------------------------------------------------------------------*/
static int BMA150_ResetCalibration(struct i2c_client *client)
{
	struct bma150_i2c_data *obj = i2c_get_clientdata(client);
	u8 ofs[4]={0,0,0,0};
	int err;
	
	#ifdef SW_CALIBRATION
		
	#else
		if(err = hwmsen_write_block(client, BMA150_REG_OFSX, ofs, 4))
		{
			GSE_ERR("error: %d\n", err);
		}
	#endif

	memset(obj->cali_sw, 0x00, sizeof(obj->cali_sw));
	memset(obj->offset, 0x00, sizeof(obj->offset));
	return err;    
}
/*----------------------------------------------------------------------------*/
static int BMA150_ReadCalibration(struct i2c_client *client, int dat[BMA150_AXES_NUM])
{
    struct bma150_i2c_data *obj = i2c_get_clientdata(client);
    int err;
    int mul;

	#ifdef SW_CALIBRATION
		mul = 0;//only SW Calibration, disable HW Calibration
	#else
	    if ((err = BMA150_ReadOffset(client, obj->offset))) {
        GSE_ERR("read offset fail, %d\n", err);
        return err;
    	}    
    	mul = obj->reso->sensitivity/bma150_offset_resolution.sensitivity;
	#endif

    dat[obj->cvt.map[BMA150_AXIS_X]] = obj->cvt.sign[BMA150_AXIS_X]*(obj->offset[BMA150_AXIS_X]*mul + obj->cali_sw[BMA150_AXIS_X]);
    dat[obj->cvt.map[BMA150_AXIS_Y]] = obj->cvt.sign[BMA150_AXIS_Y]*(obj->offset[BMA150_AXIS_Y]*mul + obj->cali_sw[BMA150_AXIS_Y]);
    dat[obj->cvt.map[BMA150_AXIS_Z]] = obj->cvt.sign[BMA150_AXIS_Z]*(obj->offset[BMA150_AXIS_Z]*mul + obj->cali_sw[BMA150_AXIS_Z]);                        
                                       
    return 0;
}
/*----------------------------------------------------------------------------*/
static int BMA150_ReadCalibrationEx(struct i2c_client *client, int act[BMA150_AXES_NUM], int raw[BMA150_AXES_NUM])
{  
	/*raw: the raw calibration data; act: the actual calibration data*/
	struct bma150_i2c_data *obj = i2c_get_clientdata(client);
	int err;
	int mul;

 

	#ifdef SW_CALIBRATION
		mul = 0;//only SW Calibration, disable HW Calibration
	#else
		if(err = BMA150_ReadOffset(client, obj->offset))
		{
			GSE_ERR("read offset fail, %d\n", err);
			return err;
		}   
		mul = obj->reso->sensitivity/bma150_offset_resolution.sensitivity;
	#endif
	
	raw[BMA150_AXIS_X] = obj->offset[BMA150_AXIS_X]*mul + obj->cali_sw[BMA150_AXIS_X];
	raw[BMA150_AXIS_Y] = obj->offset[BMA150_AXIS_Y]*mul + obj->cali_sw[BMA150_AXIS_Y];
	raw[BMA150_AXIS_Z] = obj->offset[BMA150_AXIS_Z]*mul + obj->cali_sw[BMA150_AXIS_Z];

	act[obj->cvt.map[BMA150_AXIS_X]] = obj->cvt.sign[BMA150_AXIS_X]*raw[BMA150_AXIS_X];
	act[obj->cvt.map[BMA150_AXIS_Y]] = obj->cvt.sign[BMA150_AXIS_Y]*raw[BMA150_AXIS_Y];
	act[obj->cvt.map[BMA150_AXIS_Z]] = obj->cvt.sign[BMA150_AXIS_Z]*raw[BMA150_AXIS_Z];                        
	                       
	return 0;
}
/*----------------------------------------------------------------------------*/
static int BMA150_WriteCalibration(struct i2c_client *client, int dat[BMA150_AXES_NUM])
{
	struct bma150_i2c_data *obj = i2c_get_clientdata(client);
	int err;
	int cali[BMA150_AXES_NUM], raw[BMA150_AXES_NUM];
	int lsb = bma150_offset_resolution.sensitivity;
	int divisor = obj->reso->sensitivity/lsb;

	if(err = BMA150_ReadCalibrationEx(client, cali, raw))	/*offset will be updated in obj->offset*/
	{ 
		GSE_ERR("read offset fail, %d\n", err);
		return err;
	}

	GSE_LOG("OLDOFF: (%+3d %+3d %+3d): (%+3d %+3d %+3d) / (%+3d %+3d %+3d)\n", 
		raw[BMA150_AXIS_X], raw[BMA150_AXIS_Y], raw[BMA150_AXIS_Z],
		obj->offset[BMA150_AXIS_X], obj->offset[BMA150_AXIS_Y], obj->offset[BMA150_AXIS_Z],
		obj->cali_sw[BMA150_AXIS_X], obj->cali_sw[BMA150_AXIS_Y], obj->cali_sw[BMA150_AXIS_Z]);

	/*calculate the real offset expected by caller*/
	cali[BMA150_AXIS_X] += dat[BMA150_AXIS_X];
	cali[BMA150_AXIS_Y] += dat[BMA150_AXIS_Y];
	cali[BMA150_AXIS_Z] += dat[BMA150_AXIS_Z];

	GSE_LOG("UPDATE: (%+3d %+3d %+3d)\n", 
		dat[BMA150_AXIS_X], dat[BMA150_AXIS_Y], dat[BMA150_AXIS_Z]);

#ifdef SW_CALIBRATION
	obj->cali_sw[BMA150_AXIS_X] = obj->cvt.sign[BMA150_AXIS_X]*(cali[obj->cvt.map[BMA150_AXIS_X]]);
	obj->cali_sw[BMA150_AXIS_Y] = obj->cvt.sign[BMA150_AXIS_Y]*(cali[obj->cvt.map[BMA150_AXIS_Y]]);
	obj->cali_sw[BMA150_AXIS_Z] = obj->cvt.sign[BMA150_AXIS_Z]*(cali[obj->cvt.map[BMA150_AXIS_Z]]);	
#else
	obj->offset[BMA150_AXIS_X] = (s8)(obj->cvt.sign[BMA150_AXIS_X]*(cali[obj->cvt.map[BMA150_AXIS_X]])/(divisor));
	obj->offset[BMA150_AXIS_Y] = (s8)(obj->cvt.sign[BMA150_AXIS_Y]*(cali[obj->cvt.map[BMA150_AXIS_Y]])/(divisor));
	obj->offset[BMA150_AXIS_Z] = (s8)(obj->cvt.sign[BMA150_AXIS_Z]*(cali[obj->cvt.map[BMA150_AXIS_Z]])/(divisor));

	/*convert software calibration using standard calibration*/
	obj->cali_sw[BMA150_AXIS_X] = obj->cvt.sign[BMA150_AXIS_X]*(cali[obj->cvt.map[BMA150_AXIS_X]])%(divisor);
	obj->cali_sw[BMA150_AXIS_Y] = obj->cvt.sign[BMA150_AXIS_Y]*(cali[obj->cvt.map[BMA150_AXIS_Y]])%(divisor);
	obj->cali_sw[BMA150_AXIS_Z] = obj->cvt.sign[BMA150_AXIS_Z]*(cali[obj->cvt.map[BMA150_AXIS_Z]])%(divisor);

	GSE_LOG("NEWOFF: (%+3d %+3d %+3d): (%+3d %+3d %+3d) / (%+3d %+3d %+3d)\n", 
		obj->offset[BMA150_AXIS_X]*divisor + obj->cali_sw[BMA150_AXIS_X], 
		obj->offset[BMA150_AXIS_Y]*divisor + obj->cali_sw[BMA150_AXIS_Y], 
		obj->offset[BMA150_AXIS_Z]*divisor + obj->cali_sw[BMA150_AXIS_Z], 
		obj->offset[BMA150_AXIS_X], obj->offset[BMA150_AXIS_Y], obj->offset[BMA150_AXIS_Z],
		obj->cali_sw[BMA150_AXIS_X], obj->cali_sw[BMA150_AXIS_Y], obj->cali_sw[BMA150_AXIS_Z]);

	if(err = hwmsen_write_block(obj->client, BMA150_REG_OFSX, obj->offset, BMA150_AXES_NUM))
	{
		GSE_ERR("write offset fail: %d\n", err);
		return err;
	}
#endif

	return err;
}
/*----------------------------------------------------------------------------*/
static int BMA150_CheckDeviceID(struct i2c_client *client)
{
	u8 databuf[2];    
	int res = 0;

	memset(databuf, 0, sizeof(u8)*2);    
	databuf[0] = BMA150_REG_DEVID;    

	res = i2c_master_send(client, databuf, 0x1);
	if(res <= 0)
	{
		goto exit_BMA150_CheckDeviceID;
	}
	
	udelay(500);

	databuf[0] = 0x0;        
	res = i2c_master_recv(client, databuf, 0x01);
	if(res <= 0)
	{
		goto exit_BMA150_CheckDeviceID;
	}
	

	if(databuf[0]!=BMA150_FIXED_DEVID)
	{
		printk("BMA150_CheckDeviceID %d failt!\n ", databuf[0]);
		return BMA150_ERR_IDENTIFICATION;
	}
	else
	{
		printk("BMA150_CheckDeviceID %d pass!\n ", databuf[0]);
	}

	exit_BMA150_CheckDeviceID:
	if (res <= 0)
	{
		return BMA150_ERR_I2C;
	}
	
	return BMA150_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int BMA150_SetPowerMode(struct i2c_client *client, bool enable)
{
	u8 databuf[2];    
	int res = 0;
	u8 addr = BMA150_REG_POWER_CTL;
	struct bma150_i2c_data *obj = i2c_get_clientdata(client);
	
	
	if(enable == sensor_power )
	{
		GSE_LOG("Sensor power status is newest!\n");
		return BMA150_SUCCESS;
	}

	if(hwmsen_read_block(client, addr, databuf, 0x01))
	{
		GSE_ERR("read power ctl register err!\n");
		return BMA150_ERR_I2C;
	}

	
	if(enable == TRUE)
	{
		databuf[0] &= ~BMA150_MEASURE_MODE;
	}
	else
	{
		databuf[0] |= BMA150_MEASURE_MODE;
	}
	databuf[1] = databuf[0];
	databuf[0] = BMA150_REG_POWER_CTL;
	

	res = i2c_master_send(client, databuf, 0x2);

	if(res <= 0)
	{
		GSE_LOG("set power mode failed!\n");
		return BMA150_ERR_I2C;
	}
	else if(atomic_read(&obj->trace) & ADX_TRC_INFO)
	{
		GSE_LOG("set power mode ok %d!\n", databuf[1]);
	}

	//GSE_LOG("BMA150_SetPowerMode ok!\n");


	sensor_power = enable;

	mdelay(20);
	
	return BMA150_SUCCESS;    
}
/*----------------------------------------------------------------------------*/
static int BMA150_SetDataFormat(struct i2c_client *client, u8 dataformat)
{
	struct bma150_i2c_data *obj = i2c_get_clientdata(client);
	u8 databuf[10];    
	int res = 0;

	memset(databuf, 0, sizeof(u8)*10);    

	if(hwmsen_read_block(client, BMA150_REG_DATA_FORMAT, databuf, 0x01))
	{
		printk("bma150 read Dataformat failt \n");
		return BMA150_ERR_I2C;
	}

	databuf[0] &= ~BMA150_RANGE_MASK;
	databuf[0] |= dataformat;
	databuf[1] = databuf[0];
	databuf[0] = BMA150_REG_DATA_FORMAT;


	res = i2c_master_send(client, databuf, 0x2);

	if(res <= 0)
	{
		return BMA150_ERR_I2C;
	}
	
	//printk("BMA150_SetDataFormat OK! \n");
	

	return BMA150_SetDataResolution(obj);    
}
/*----------------------------------------------------------------------------*/
static int BMA150_SetBWRate(struct i2c_client *client, u8 bwrate)
{
	u8 databuf[10];    
	int res = 0;

	memset(databuf, 0, sizeof(u8)*10);    

	if(hwmsen_read_block(client, BMA150_REG_BW_RATE, databuf, 0x01))
	{
		printk("bma150 read rate failt \n");
		return BMA150_ERR_I2C;
	}

	databuf[0] &= ~BMA150_BW_MASK;
	databuf[0] |= bwrate;
	databuf[1] = databuf[0];
	databuf[0] = BMA150_REG_BW_RATE;


	res = i2c_master_send(client, databuf, 0x2);

	if(res <= 0)
	{
		return BMA150_ERR_I2C;
	}
	
	//printk("BMA150_SetBWRate OK! \n");
	
	return BMA150_SUCCESS;    
}
/*----------------------------------------------------------------------------*/
static int BMA150_SetIntEnable(struct i2c_client *client, u8 intenable)
{
			u8 databuf[10];    
			int res = 0;
		
			res = hwmsen_write_byte(client, BMA150_INT_REG, 0x00);
			if(res != BMA150_SUCCESS) 
			{
				return res;
			}
			printk("BMA150 disable interrupt ...\n");
		
			/*for disable interrupt function*/
			
			return BMA150_SUCCESS;	  
}

/*----------------------------------------------------------------------------*/
static int bma150_init_client(struct i2c_client *client, int reset_cali)
{
	struct bma150_i2c_data *obj = i2c_get_clientdata(client);
	int res = 0;

	res = BMA150_CheckDeviceID(client); 
	if(res != BMA150_SUCCESS)
	{
		return res;
	}	
	
	res = BMA150_SetPowerMode(client, false);
	if(res != BMA150_SUCCESS)
	{
		return res;
	}
	printk("BMA150_SetPowerMode OK!\n");
	
	res = BMA150_SetBWRate(client, BMA150_BW_100HZ);
	if(res != BMA150_SUCCESS ) 
	{
		return res;
	}
	printk("BMA150_SetBWRate OK!\n");
	
	res = BMA150_SetDataFormat(client, BMA150_RANGE_2G);
	if(res != BMA150_SUCCESS) 
	{
		return res;
	}
	printk("BMA150_SetDataFormat OK!\n");

	gsensor_gain.x = gsensor_gain.y = gsensor_gain.z = obj->reso->sensitivity;


	res = BMA150_SetIntEnable(client, 0x00);        
	if(res != BMA150_SUCCESS)
	{
		return res;
	}
	printk("BMA150 disable interrupt function!\n");


	if(0 != reset_cali)
	{ 
		/*reset calibration only in power on*/
		res = BMA150_ResetCalibration(client);
		if(res != BMA150_SUCCESS)
		{
			return res;
		}
	}
	printk("bma150_init_client OK!\n");
#ifdef CONFIG_BMA150_LOWPASS
	memset(&obj->fir, 0x00, sizeof(obj->fir));  
#endif

	mdelay(20);

	return BMA150_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int BMA150_ReadChipInfo(struct i2c_client *client, char *buf, int bufsize)
{
	u8 databuf[10];    

	memset(databuf, 0, sizeof(u8)*10);

	if((NULL == buf)||(bufsize<=30))
	{
		return -1;
	}
	
	if(NULL == client)
	{
		*buf = 0;
		return -2;
	}

	sprintf(buf, "BMA150 Chip");
	return 0;
}
/*----------------------------------------------------------------------------*/
static int BMA150_ReadSensorData(struct i2c_client *client, char *buf, int bufsize)
{
	struct bma150_i2c_data *obj = (struct bma150_i2c_data*)i2c_get_clientdata(client);
	u8 databuf[20];
	int acc[BMA150_AXES_NUM];
	int res = 0;
	memset(databuf, 0, sizeof(u8)*10);

	if(NULL == buf)
	{
		return -1;
	}
	if(NULL == client)
	{
		*buf = 0;
		return -2;
	}

	if(sensor_power == FALSE)
	{
		res = BMA150_SetPowerMode(client, true);
		if(res)
		{
			GSE_ERR("Power on bma150 error %d!\n", res);
		}
	}

	if(res = BMA150_ReadData(client, obj->data))
	{        
		GSE_ERR("I2C error: ret value=%d", res);
		return -3;
	}
	else
	{
		//printk("raw data x=%d, y=%d, z=%d \n",obj->data[BMA150_AXIS_X],obj->data[BMA150_AXIS_Y],obj->data[BMA150_AXIS_Z]);
		obj->data[BMA150_AXIS_X] += obj->cali_sw[BMA150_AXIS_X];
		obj->data[BMA150_AXIS_Y] += obj->cali_sw[BMA150_AXIS_Y];
		obj->data[BMA150_AXIS_Z] += obj->cali_sw[BMA150_AXIS_Z];
		
		//printk("cali_sw x=%d, y=%d, z=%d \n",obj->cali_sw[BMA150_AXIS_X],obj->cali_sw[BMA150_AXIS_Y],obj->cali_sw[BMA150_AXIS_Z]);
		
		/*remap coordinate*/
		acc[obj->cvt.map[BMA150_AXIS_X]] = obj->cvt.sign[BMA150_AXIS_X]*obj->data[BMA150_AXIS_X];
		acc[obj->cvt.map[BMA150_AXIS_Y]] = obj->cvt.sign[BMA150_AXIS_Y]*obj->data[BMA150_AXIS_Y];
		acc[obj->cvt.map[BMA150_AXIS_Z]] = obj->cvt.sign[BMA150_AXIS_Z]*obj->data[BMA150_AXIS_Z];
		//printk("cvt x=%d, y=%d, z=%d \n",obj->cvt.sign[BMA150_AXIS_X],obj->cvt.sign[BMA150_AXIS_Y],obj->cvt.sign[BMA150_AXIS_Z]);


		//GSE_LOG("Mapped gsensor data: %d, %d, %d!\n", acc[BMA150_AXIS_X], acc[BMA150_AXIS_Y], acc[BMA150_AXIS_Z]);

		//Out put the mg
		//printk("mg acc=%d, GRAVITY=%d, sensityvity=%d \n",acc[BMA150_AXIS_X],GRAVITY_EARTH_1000,obj->reso->sensitivity);
		acc[BMA150_AXIS_X] = acc[BMA150_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		acc[BMA150_AXIS_Y] = acc[BMA150_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		acc[BMA150_AXIS_Z] = acc[BMA150_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;		
		
	

		sprintf(buf, "%04x %04x %04x", acc[BMA150_AXIS_X], acc[BMA150_AXIS_Y], acc[BMA150_AXIS_Z]);
		if(atomic_read(&obj->trace) & ADX_TRC_IOCTL)
		{
			GSE_LOG("gsensor data: %s!\n", buf);
		}
	}
	
	return 0;
}
/*----------------------------------------------------------------------------*/
static int BMA150_ReadRawData(struct i2c_client *client, char *buf)
{
	struct bma150_i2c_data *obj = (struct bma150_i2c_data*)i2c_get_clientdata(client);
	int res = 0;

	if (!buf || !client)
	{
		return EINVAL;
	}
	
	if(res = BMA150_ReadData(client, obj->data))
	{        
		GSE_ERR("I2C error: ret value=%d", res);
		return EIO;
	}
	else
	{
		sprintf(buf, "BMA150_ReadRawData %04x %04x %04x", obj->data[BMA150_AXIS_X], 
			obj->data[BMA150_AXIS_Y], obj->data[BMA150_AXIS_Z]);
	
	}
	
	return 0;
}
/*----------------------------------------------------------------------------*/
static int BMA150_InitSelfTest(struct i2c_client *client)
{
	int res = 0;

	BMA150_SetPowerMode(client, true);

	res = BMA150_SetBWRate(client, BMA150_BW_25HZ);
	if(res != BMA150_SUCCESS ) //0x2C->BW=100Hz
	{
		return res;
	}
	
	res = BMA150_SetDataFormat(client, BMA150_RANGE_4G);
	if(res != BMA150_SUCCESS) //0x2C->BW=100Hz
	{
		return res;
	}
	mdelay(60);
	
	return BMA150_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int BMA150_JudgeTestResult(struct i2c_client *client)
{

	struct bma150_i2c_data *obj = (struct bma150_i2c_data*)i2c_get_clientdata(client);
	int res = 0;
	s16  acc[BMA150_AXES_NUM];
	int  self_result;

	
	if(res = BMA150_ReadData(client, acc))
	{        
		GSE_ERR("I2C error: ret value=%d", res);
		return EIO;
	}
	else
	{			
		printk("0 step: %d %d %d\n", acc[0],acc[1],acc[2]);

		acc[BMA150_AXIS_X] = acc[BMA150_AXIS_X] * 1000 / 128;
		acc[BMA150_AXIS_Y] = acc[BMA150_AXIS_Y] * 1000 / 128;
		acc[BMA150_AXIS_Z] = acc[BMA150_AXIS_Z] * 1000 / 128;
		
		printk("1 step: %d %d %d\n", acc[0],acc[1],acc[2]);
		
		self_result = acc[BMA150_AXIS_X]*acc[BMA150_AXIS_X] 
			+ acc[BMA150_AXIS_Y]*acc[BMA150_AXIS_Y] 
			+ acc[BMA150_AXIS_Z]*acc[BMA150_AXIS_Z];
			
		
		printk("2 step: result = %d", self_result);

	    if ( (self_result>550000) && (self_result<1700000) ) //between 0.55g and 1.7g 
	    {												 
			GSE_ERR("BMA150_JudgeTestResult successful\n");
			return BMA150_SUCCESS;
		}
		{
	        GSE_ERR("BMA150_JudgeTestResult failt\n");
	        return -EINVAL;
	    }
	
	}
	
}
/*----------------------------------------------------------------------------*/
static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = bma150_i2c_client;
	char strbuf[BMA150_BUFSIZE];
	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	
	BMA150_ReadChipInfo(client, strbuf, BMA150_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);        
}

static ssize_t gsensor_init(struct device_driver *ddri, char *buf, size_t count)
	{
		struct i2c_client *client = bma150_i2c_client;
		char strbuf[BMA150_BUFSIZE];
		
		if(NULL == client)
		{
			GSE_ERR("i2c client is null!!\n");
			return 0;
		}
		bma150_init_client(client, 1);
		return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);			
	}



/*----------------------------------------------------------------------------*/
static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = bma150_i2c_client;
	char strbuf[BMA150_BUFSIZE];
	
	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	BMA150_ReadSensorData(client, strbuf, BMA150_BUFSIZE);
	//BMA150_ReadRawData(client, strbuf);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);            
}

static ssize_t show_sensorrawdata_value(struct device_driver *ddri, char *buf, size_t count)
	{
		struct i2c_client *client = bma150_i2c_client;
		char strbuf[BMA150_BUFSIZE];
		
		if(NULL == client)
		{
			GSE_ERR("i2c client is null!!\n");
			return 0;
		}
		//BMA150_ReadSensorData(client, strbuf, BMA150_BUFSIZE);
		BMA150_ReadRawData(client, strbuf);
		return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);			
	}

/*----------------------------------------------------------------------------*/
static ssize_t show_cali_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = bma150_i2c_client;
	struct bma150_i2c_data *obj;
	int err, len = 0, mul;
	int tmp[BMA150_AXES_NUM];

	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}

	obj = i2c_get_clientdata(client);



	if(err = BMA150_ReadOffset(client, obj->offset))
	{
		return -EINVAL;
	}
	else if(err = BMA150_ReadCalibration(client, tmp))
	{
		return -EINVAL;
	}
	else
	{    
		mul = obj->reso->sensitivity/bma150_offset_resolution.sensitivity;
		len += snprintf(buf+len, PAGE_SIZE-len, "[HW ][%d] (%+3d, %+3d, %+3d) : (0x%02X, 0x%02X, 0x%02X)\n", mul,                        
			obj->offset[BMA150_AXIS_X], obj->offset[BMA150_AXIS_Y], obj->offset[BMA150_AXIS_Z],
			obj->offset[BMA150_AXIS_X], obj->offset[BMA150_AXIS_Y], obj->offset[BMA150_AXIS_Z]);
		len += snprintf(buf+len, PAGE_SIZE-len, "[SW ][%d] (%+3d, %+3d, %+3d)\n", 1, 
			obj->cali_sw[BMA150_AXIS_X], obj->cali_sw[BMA150_AXIS_Y], obj->cali_sw[BMA150_AXIS_Z]);

		len += snprintf(buf+len, PAGE_SIZE-len, "[ALL]    (%+3d, %+3d, %+3d) : (%+3d, %+3d, %+3d)\n", 
			obj->offset[BMA150_AXIS_X]*mul + obj->cali_sw[BMA150_AXIS_X],
			obj->offset[BMA150_AXIS_Y]*mul + obj->cali_sw[BMA150_AXIS_Y],
			obj->offset[BMA150_AXIS_Z]*mul + obj->cali_sw[BMA150_AXIS_Z],
			tmp[BMA150_AXIS_X], tmp[BMA150_AXIS_Y], tmp[BMA150_AXIS_Z]);
		
		return len;
    }
}
/*----------------------------------------------------------------------------*/
static ssize_t store_cali_value(struct device_driver *ddri, char *buf, size_t count)
{
	struct i2c_client *client = bma150_i2c_client;  
	int err, x, y, z;
	int dat[BMA150_AXES_NUM];

	if(!strncmp(buf, "rst", 3))
	{
		if(err = BMA150_ResetCalibration(client))
		{
			GSE_ERR("reset offset err = %d\n", err);
		}	
	}
	else if(3 == sscanf(buf, "0x%02X 0x%02X 0x%02X", &x, &y, &z))
	{
		dat[BMA150_AXIS_X] = x;
		dat[BMA150_AXIS_Y] = y;
		dat[BMA150_AXIS_Z] = z;
		if(err = BMA150_WriteCalibration(client, dat))
		{
			GSE_ERR("write calibration err = %d\n", err);
		}		
	}
	else
	{
		GSE_ERR("invalid format\n");
	}
	
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_self_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = bma150_i2c_client;
	struct bma150_i2c_data *obj;

	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}

	//obj = i2c_get_clientdata(client);
	
    return snprintf(buf, 8, "%s\n", selftestRes);
}
/*----------------------------------------------------------------------------*/
static ssize_t store_self_value(struct device_driver *ddri, char *buf, size_t count)
{   /*write anything to this register will trigger the process*/
	struct item{
	s16 raw[BMA150_AXES_NUM];
	};
	
	struct i2c_client *client = bma150_i2c_client;  
	int idx, res, num;
	struct item *prv = NULL, *nxt = NULL;


	if(1 != sscanf(buf, "%d", &num))
	{
		GSE_ERR("parse number fail\n");
		return count;
	}
	else if(num == 0)
	{
		GSE_ERR("invalid data count\n");
		return count;
	}

	prv = kzalloc(sizeof(*prv) * num, GFP_KERNEL);
	nxt = kzalloc(sizeof(*nxt) * num, GFP_KERNEL);
	if (!prv || !nxt)
	{
		goto exit;
	}


	GSE_LOG("NORMAL:\n");
	BMA150_SetPowerMode(client,true); 

	/*initial setting for self test*/
	BMA150_InitSelfTest(client);
	GSE_LOG("SELFTEST:\n");    

	if(!BMA150_JudgeTestResult(client))
	{
		GSE_LOG("SELFTEST : PASS\n");
		strcpy(selftestRes,"y");
	}	
	else
	{
		GSE_LOG("SELFTEST : FAIL\n");		
		strcpy(selftestRes,"n");
	}
	
	exit:
	/*restore the setting*/    
	bma150_init_client(client, 0);
	kfree(prv);
	kfree(nxt);
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_selftest_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = bma150_i2c_client;
	struct bma150_i2c_data *obj;

	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}

	obj = i2c_get_clientdata(client);
	return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&obj->selftest));
}
/*----------------------------------------------------------------------------*/
static ssize_t store_selftest_value(struct device_driver *ddri, char *buf, size_t count)
{
	struct bma150_i2c_data *obj = obj_i2c_data;
	int tmp;

	if(NULL == obj)
	{
		GSE_ERR("i2c data obj is null!!\n");
		return 0;
	}
	
	
	if(1 == sscanf(buf, "%d", &tmp))
	{        
		if(atomic_read(&obj->selftest) && !tmp)
		{
			/*enable -> disable*/
			bma150_init_client(obj->client, 0);
		}
		else if(!atomic_read(&obj->selftest) && tmp)
		{
			/*disable -> enable*/
			BMA150_InitSelfTest(obj->client);            
		}
		
		GSE_LOG("selftest: %d => %d\n", atomic_read(&obj->selftest), tmp);
		atomic_set(&obj->selftest, tmp); 
	}
	else
	{ 
		GSE_ERR("invalid content: '%s', length = %d\n", buf, count);   
	}
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_firlen_value(struct device_driver *ddri, char *buf)
{
#ifdef CONFIG_BMA150_LOWPASS
	struct i2c_client *client = bma150_i2c_client;
	struct bma150_i2c_data *obj = i2c_get_clientdata(client);
	if(atomic_read(&obj->firlen))
	{
		int idx, len = atomic_read(&obj->firlen);
		GSE_LOG("len = %2d, idx = %2d\n", obj->fir.num, obj->fir.idx);

		for(idx = 0; idx < len; idx++)
		{
			GSE_LOG("[%5d %5d %5d]\n", obj->fir.raw[idx][BMA150_AXIS_X], obj->fir.raw[idx][BMA150_AXIS_Y], obj->fir.raw[idx][BMA150_AXIS_Z]);
		}
		
		GSE_LOG("sum = [%5d %5d %5d]\n", obj->fir.sum[BMA150_AXIS_X], obj->fir.sum[BMA150_AXIS_Y], obj->fir.sum[BMA150_AXIS_Z]);
		GSE_LOG("avg = [%5d %5d %5d]\n", obj->fir.sum[BMA150_AXIS_X]/len, obj->fir.sum[BMA150_AXIS_Y]/len, obj->fir.sum[BMA150_AXIS_Z]/len);
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&obj->firlen));
#else
	return snprintf(buf, PAGE_SIZE, "not support\n");
#endif
}
/*----------------------------------------------------------------------------*/
static ssize_t store_firlen_value(struct device_driver *ddri, char *buf, size_t count)
{
#ifdef CONFIG_BMA150_LOWPASS
	struct i2c_client *client = bma150_i2c_client;  
	struct bma150_i2c_data *obj = i2c_get_clientdata(client);
	int firlen;

	if(1 != sscanf(buf, "%d", &firlen))
	{
		GSE_ERR("invallid format\n");
	}
	else if(firlen > C_MAX_FIR_LENGTH)
	{
		GSE_ERR("exceeds maximum filter length\n");
	}
	else
	{ 
		atomic_set(&obj->firlen, firlen);
		if(NULL == firlen)
		{
			atomic_set(&obj->fir_en, 0);
		}
		else
		{
			memset(&obj->fir, 0x00, sizeof(obj->fir));
			atomic_set(&obj->fir_en, 1);
		}
	}
#endif    
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_trace_value(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	struct bma150_i2c_data *obj = obj_i2c_data;
	if (obj == NULL)
	{
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}
	
	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));     
	return res;    
}
/*----------------------------------------------------------------------------*/
static ssize_t store_trace_value(struct device_driver *ddri, char *buf, size_t count)
{
	struct bma150_i2c_data *obj = obj_i2c_data;
	int trace;
	if (obj == NULL)
	{
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}
	
	if(1 == sscanf(buf, "0x%x", &trace))
	{
		atomic_set(&obj->trace, trace);
	}	
	else
	{
		GSE_ERR("invalid content: '%s', length = %d\n", buf, count);
	}
	
	return count;    
}
/*----------------------------------------------------------------------------*/
static ssize_t show_status_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;    
	struct bma150_i2c_data *obj = obj_i2c_data;
	if (obj == NULL)
	{
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}	
	
	if(obj->hw)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: %d %d (%d %d)\n", 
	            obj->hw->i2c_num, obj->hw->direction, obj->hw->power_id, obj->hw->power_vol);   
	}
	else
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");
	}
	return len;    
}
/*----------------------------------------------------------------------------*/
static ssize_t show_power_status_value(struct device_driver *ddri, char *buf)
{
	if(sensor_power)
		printk("G sensor is in work mode, sensor_power = %d\n", sensor_power);
	else
		printk("G sensor is in standby mode, sensor_power = %d\n", sensor_power);

	return 0;
}
/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(chipinfo,   S_IWUSR | S_IRUGO, show_chipinfo_value,      NULL);
static DRIVER_ATTR(sensordata, S_IWUSR | S_IRUGO, show_sensordata_value,    NULL);
static DRIVER_ATTR(cali,       S_IWUSR | S_IRUGO, show_cali_value,          store_cali_value);
static DRIVER_ATTR(selftest, S_IWUSR | S_IRUGO, show_self_value,  store_self_value);
static DRIVER_ATTR(self,   S_IWUSR | S_IRUGO, show_selftest_value,      store_selftest_value);
static DRIVER_ATTR(firlen,     S_IWUSR | S_IRUGO, show_firlen_value,        store_firlen_value);
static DRIVER_ATTR(trace,      S_IWUSR | S_IRUGO, show_trace_value,         store_trace_value);
static DRIVER_ATTR(status,               S_IRUGO, show_status_value,        NULL);
static DRIVER_ATTR(powerstatus,               S_IRUGO, show_power_status_value,        NULL);

/*----------------------------------------------------------------------------*/
static struct driver_attribute *bma150_attr_list[] = {
	&driver_attr_chipinfo,     /*chip information*/
	&driver_attr_sensordata,   /*dump sensor data*/
	&driver_attr_cali,         /*show calibration data*/
	&driver_attr_self,         /*self test demo*/
	&driver_attr_selftest,     /*self control: 0: disable, 1: enable*/
	&driver_attr_firlen,       /*filter length: 0: disable, others: enable*/
	&driver_attr_trace,        /*trace log*/
	&driver_attr_status,
	&driver_attr_powerstatus,
};
/*----------------------------------------------------------------------------*/
static int bma150_create_attr(struct device_driver *driver) 
{
	int idx, err = 0;
	int num = (int)(sizeof(bma150_attr_list)/sizeof(bma150_attr_list[0]));
	if (driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
		if(err = driver_create_file(driver, bma150_attr_list[idx]))
		{            
			GSE_ERR("driver_create_file (%s) = %d\n", bma150_attr_list[idx]->attr.name, err);
			break;
		}
	}    
	return err;
}
/*----------------------------------------------------------------------------*/
static int bma150_delete_attr(struct device_driver *driver)
{
	int idx ,err = 0;
	int num = (int)(sizeof(bma150_attr_list)/sizeof(bma150_attr_list[0]));

	if(driver == NULL)
	{
		return -EINVAL;
	}
	

	for(idx = 0; idx < num; idx++)
	{
		driver_remove_file(driver, bma150_attr_list[idx]);
	}
	

	return err;
}

/*----------------------------------------------------------------------------*/
int gsensor_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value, sample_delay;	
	struct bma150_i2c_data *priv = (struct bma150_i2c_data*)self;
	hwm_sensor_data* gsensor_data;
	char buff[BMA150_BUFSIZE];
	
	//GSE_FUN(f);
	switch (command)
	{
		case SENSOR_DELAY:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				GSE_ERR("Set delay parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				value = *(int *)buff_in;
				if(value <= 5)
				{
					sample_delay = BMA150_BW_200HZ;
				}
				else if(value <= 10)
				{
					sample_delay = BMA150_BW_100HZ;
				}
				else
				{
					sample_delay = BMA150_BW_50HZ;
				}
				
				err = BMA150_SetBWRate(priv->client, sample_delay);
				if(err != BMA150_SUCCESS ) //0x2C->BW=100Hz
				{
					GSE_ERR("Set delay parameter error!\n");
				}

				if(value >= 50)
				{
					atomic_set(&priv->filter, 0);
				}
				else
				{	
				#if defined(CONFIG_BMA150_LOWPASS)
					priv->fir.num = 0;
					priv->fir.idx = 0;
					priv->fir.sum[BMA150_AXIS_X] = 0;
					priv->fir.sum[BMA150_AXIS_Y] = 0;
					priv->fir.sum[BMA150_AXIS_Z] = 0;
					atomic_set(&priv->filter, 1);
				#endif
				}
			}
			break;

		case SENSOR_ENABLE:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				GSE_ERR("Enable sensor parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				value = *(int *)buff_in;
				if(((value == 0) && (sensor_power == false)) ||((value == 1) && (sensor_power == true)))
				{
					GSE_LOG("Gsensor device have updated!\n");
				}
				else
				{
					err = BMA150_SetPowerMode( priv->client, !sensor_power);
				}
			}
			break;

		case SENSOR_GET_DATA:
			if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
			{
				GSE_ERR("get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				gsensor_data = (hwm_sensor_data *)buff_out;
				BMA150_ReadSensorData(priv->client, buff, BMA150_BUFSIZE);
				sscanf(buff, "%x %x %x", &gsensor_data->values[0], 
					&gsensor_data->values[1], &gsensor_data->values[2]);				
				gsensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;				
				gsensor_data->value_divide = 1000;
			}
			break;
		default:
			GSE_ERR("gsensor operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}
	
	return err;
}

/****************************************************************************** 
 * Function Configuration
******************************************************************************/
static int bma150_open(struct inode *inode, struct file *file)
{
	file->private_data = bma150_i2c_client;

	if(file->private_data == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return -EINVAL;
	}
	return nonseekable_open(inode, file);
}
/*----------------------------------------------------------------------------*/
static int bma150_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}
/*----------------------------------------------------------------------------*/
//static int bma150_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
    //   unsigned long arg)
static int bma150_unlocked_ioctl(struct file *file, unsigned int cmd,
       unsigned long arg)       
{
	struct i2c_client *client = (struct i2c_client*)file->private_data;
	struct bma150_i2c_data *obj = (struct bma150_i2c_data*)i2c_get_clientdata(client);	
	char strbuf[BMA150_BUFSIZE];
	void __user *data;
	SENSOR_DATA sensor_data;
	int err = 0;
	int cali[3];

	//GSE_FUN(f);
	if(_IOC_DIR(cmd) & _IOC_READ)
	{
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	}
	else if(_IOC_DIR(cmd) & _IOC_WRITE)
	{
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	}

	if(err)
	{
		GSE_ERR("access error: %08X, (%2d, %2d)\n", cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));
		return -EFAULT;
	}

	switch(cmd)
	{
		case GSENSOR_IOCTL_INIT:
			bma150_init_client(client, 0);			
			break;

		case GSENSOR_IOCTL_READ_CHIPINFO:
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			
			BMA150_ReadChipInfo(client, strbuf, BMA150_BUFSIZE);
			if(copy_to_user(data, strbuf, strlen(strbuf)+1))
			{
				err = -EFAULT;
				break;
			}				 
			break;	  

		case GSENSOR_IOCTL_READ_SENSORDATA:
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			
			BMA150_ReadSensorData(client, strbuf, BMA150_BUFSIZE);
			if(copy_to_user(data, strbuf, strlen(strbuf)+1))
			{
				err = -EFAULT;
				break;	  
			}				 
			break;

		case GSENSOR_IOCTL_READ_GAIN:
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}			
			
			if(copy_to_user(data, &gsensor_gain, sizeof(GSENSOR_VECTOR3D)))
			{
				err = -EFAULT;
				break;
			}				 
			break;

		case GSENSOR_IOCTL_READ_RAW_DATA:
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			BMA150_ReadRawData(client, strbuf);
			if(copy_to_user(data, &strbuf, strlen(strbuf)+1))
			{
				err = -EFAULT;
				break;	  
			}
			break;	  

		case GSENSOR_IOCTL_SET_CALI:
			data = (void __user*)arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			if(copy_from_user(&sensor_data, data, sizeof(sensor_data)))
			{
				err = -EFAULT;
				break;	  
			}
			if(atomic_read(&obj->suspend))
			{
				GSE_ERR("Perform calibration in suspend state!!\n");
				err = -EINVAL;
			}
			else
			{
				cali[BMA150_AXIS_X] = sensor_data.x * obj->reso->sensitivity / GRAVITY_EARTH_1000;
				cali[BMA150_AXIS_Y] = sensor_data.y * obj->reso->sensitivity / GRAVITY_EARTH_1000;
				cali[BMA150_AXIS_Z] = sensor_data.z * obj->reso->sensitivity / GRAVITY_EARTH_1000;			  
				err = BMA150_WriteCalibration(client, cali);			 
			}
			break;

		case GSENSOR_IOCTL_CLR_CALI:
			err = BMA150_ResetCalibration(client);
			break;

		case GSENSOR_IOCTL_GET_CALI:
			data = (void __user*)arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			if(err = BMA150_ReadCalibration(client, cali))
			{
				break;
			}
			
			sensor_data.x = cali[BMA150_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
			sensor_data.y = cali[BMA150_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
			sensor_data.z = cali[BMA150_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
			if(copy_to_user(data, &sensor_data, sizeof(sensor_data)))
			{
				err = -EFAULT;
				break;
			}		
			break;
		

		default:
			GSE_ERR("unknown IOCTL: 0x%08x\n", cmd);
			err = -ENOIOCTLCMD;
			break;
			
	}

	return err;
}


/*----------------------------------------------------------------------------*/
static struct file_operations bma150_fops = {
	//.owner = THIS_MODULE,
	.open = bma150_open,
	.release = bma150_release,
	.unlocked_ioctl = bma150_unlocked_ioctl,
};
/*----------------------------------------------------------------------------*/
static struct miscdevice bma150_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "gsensor",
	.fops = &bma150_fops,
};
/*----------------------------------------------------------------------------*/
#ifndef CONFIG_HAS_EARLYSUSPEND
/*----------------------------------------------------------------------------*/
static int bma150_suspend(struct i2c_client *client, pm_message_t msg) 
{
	struct bma150_i2c_data *obj = i2c_get_clientdata(client);    
	int err = 0;
	GSE_FUN();    

	if(msg.event == PM_EVENT_SUSPEND)
	{   
		if(obj == NULL)
		{
			GSE_ERR("null pointer!!\n");
			return -EINVAL;
		}
		atomic_set(&obj->suspend, 1);
		if(err = BMA150_SetPowerMode(obj->client, false))
		{
			GSE_ERR("write power control fail!!\n");
			return;
		}       
		BMA150_power(obj->hw, 0);
	}
	return err;
}
/*----------------------------------------------------------------------------*/
static int bma150_resume(struct i2c_client *client)
{
	struct bma150_i2c_data *obj = i2c_get_clientdata(client);        
	int err;
	GSE_FUN();

	if(obj == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return -EINVAL;
	}

	BMA150_power(obj->hw, 1);
	if(err = bma150_init_client(client, 0))
	{
		GSE_ERR("initialize client fail!!\n");
		return err;        
	}
	atomic_set(&obj->suspend, 0);

	return 0;
}
/*----------------------------------------------------------------------------*/
#else /*CONFIG_HAS_EARLY_SUSPEND is defined*/
/*----------------------------------------------------------------------------*/
static void bma150_early_suspend(struct early_suspend *h) 
{
	struct bma150_i2c_data *obj = container_of(h, struct bma150_i2c_data, early_drv);   
	int err;
	GSE_FUN();    

	if(obj == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return;
	}
	atomic_set(&obj->suspend, 1); 
	if(err = BMA150_SetPowerMode(obj->client, false))
	{
		GSE_ERR("write power control fail!!\n");
		return;
	}

	sensor_power = false;
	
	BMA150_power(obj->hw, 0);
}
/*----------------------------------------------------------------------------*/
static void bma150_late_resume(struct early_suspend *h)
{
	struct bma150_i2c_data *obj = container_of(h, struct bma150_i2c_data, early_drv);         
	int err;
	GSE_FUN();

	if(obj == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return;
	}

	BMA150_power(obj->hw, 1);
	if(err = bma150_init_client(obj->client, 0))
	{
		GSE_ERR("initialize client fail!!\n");
		return;        
	}
	atomic_set(&obj->suspend, 0);    
}
/*----------------------------------------------------------------------------*/
#endif /*CONFIG_HAS_EARLYSUSPEND*/
/*----------------------------------------------------------------------------*/
static int bma150_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info) 
{    
	strcpy(info->type, BMA150_DEV_NAME);
	return 0;
}

/*----------------------------------------------------------------------------*/
static int bma150_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_client *new_client;
	struct bma150_i2c_data *obj;
	struct hwmsen_object sobj;
	int err = 0;
	GSE_FUN();

	if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
	{
		err = -ENOMEM;
		goto exit;
	}
	
	memset(obj, 0, sizeof(struct bma150_i2c_data));

	obj->hw = get_cust_acc_hw();
	
	if(err = hwmsen_get_convert(obj->hw->direction, &obj->cvt))
	{
		GSE_ERR("invalid direction: %d\n", obj->hw->direction);
		goto exit;
	}

	obj_i2c_data = obj;
	obj->client = client;
	new_client = obj->client;
	i2c_set_clientdata(new_client,obj);
	
	atomic_set(&obj->trace, 0);
	atomic_set(&obj->suspend, 0);
	
#ifdef CONFIG_BMA150_LOWPASS
	if(obj->hw->firlen > C_MAX_FIR_LENGTH)
	{
		atomic_set(&obj->firlen, C_MAX_FIR_LENGTH);
	}	
	else
	{
		atomic_set(&obj->firlen, obj->hw->firlen);
	}
	
	if(atomic_read(&obj->firlen) > 0)
	{
		atomic_set(&obj->fir_en, 1);
	}
	
#endif

	bma150_i2c_client = new_client;	

	if(err = bma150_init_client(new_client, 1))
	{
		goto exit_init_failed;
	}
	

	if(err = misc_register(&bma150_device))
	{
		GSE_ERR("bma150_device register failed\n");
		goto exit_misc_device_register_failed;
	}

	if(err = bma150_create_attr(&bma150_gsensor_driver.driver))
	{
		GSE_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}

	sobj.self = obj;
    sobj.polling = 1;
    sobj.sensor_operate = gsensor_operate;
	if(err = hwmsen_attach(ID_ACCELEROMETER, &sobj))
	{
		GSE_ERR("attach fail = %d\n", err);
		goto exit_kfree;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	obj->early_drv.level    = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
	obj->early_drv.suspend  = bma150_early_suspend,
	obj->early_drv.resume   = bma150_late_resume,    
	register_early_suspend(&obj->early_drv);
#endif 

	GSE_LOG("%s: OK\n", __func__);    
	return 0;

	exit_create_attr_failed:
	misc_deregister(&bma150_device);
	exit_misc_device_register_failed:
	exit_init_failed:
	//i2c_detach_client(new_client);
	exit_kfree:
	kfree(obj);
	exit:
	GSE_ERR("%s: err = %d\n", __func__, err);        
	return err;
}

/*----------------------------------------------------------------------------*/
static int bma150_i2c_remove(struct i2c_client *client)
{
	int err = 0;	
	
	if(err = bma150_delete_attr(&bma150_gsensor_driver.driver))
	{
		GSE_ERR("bma150_delete_attr fail: %d\n", err);
	}
	
	if(err = misc_deregister(&bma150_device))
	{
		GSE_ERR("misc_deregister fail: %d\n", err);
	}

	if(err = hwmsen_detach(ID_ACCELEROMETER))
	    

	bma150_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));
	return 0;
}
/*----------------------------------------------------------------------------*/
static int bma150_probe(struct platform_device *pdev) 
{
	struct acc_hw *hw = get_cust_acc_hw();
	GSE_FUN();

	BMA150_power(hw, 1);
	//bma150_force[0] = hw->i2c_num;
	if(i2c_add_driver(&bma150_i2c_driver))
	{
		GSE_ERR("add driver error\n");
		return -1;
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
static int bma150_remove(struct platform_device *pdev)
{
    struct acc_hw *hw = get_cust_acc_hw();

    GSE_FUN();    
    BMA150_power(hw, 0);    
    i2c_del_driver(&bma150_i2c_driver);
    return 0;
}
/*----------------------------------------------------------------------------*/
static struct platform_driver bma150_gsensor_driver = {
	.probe      = bma150_probe,
	.remove     = bma150_remove,    
	.driver     = {
		.name  = "gsensor",
		//.owner = THIS_MODULE,
	}
};

/*----------------------------------------------------------------------------*/
static int __init bma150_init(void)
{
	GSE_FUN();
	struct acc_hw *hw = get_cust_acc_hw();
	GSE_LOG("%s: i2c_number=%d\n", __func__,hw->i2c_num); 
	i2c_register_board_info(hw->i2c_num, &i2c_bma150, 1);
	if(platform_driver_register(&bma150_gsensor_driver))
	{
		GSE_ERR("failed to register driver");
		return -ENODEV;
	}
	return 0;    
}
/*----------------------------------------------------------------------------*/
static void __exit bma150_exit(void)
{
	GSE_FUN();
	platform_driver_unregister(&bma150_gsensor_driver);
}
/*----------------------------------------------------------------------------*/
module_init(bma150_init);
module_exit(bma150_exit);
/*----------------------------------------------------------------------------*/
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("BMA150 I2C driver");
MODULE_AUTHOR("Dexiang.Liu@mediatek.com");
