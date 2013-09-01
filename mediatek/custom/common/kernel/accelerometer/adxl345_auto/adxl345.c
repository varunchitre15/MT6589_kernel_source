/* ADXL345 motion sensor driver
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

#include <cust_acc.h>
#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#include "adxl345.h"
#include <linux/hwmsen_helper.h>

#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>


/*-------------------------MT6516&MT6573 define-------------------------------*/

#define POWER_NONE_MACRO MT65XX_POWER_NONE

/*----------------------------------------------------------------------------*/
#define I2C_DRIVERID_ADXL345 345
/*----------------------------------------------------------------------------*/
#define DEBUG 1
/*----------------------------------------------------------------------------*/
#define CONFIG_ADXL345_LOWPASS   /*apply low pass filter on output*/       
/*----------------------------------------------------------------------------*/
#define ADXL345_AXIS_X          0
#define ADXL345_AXIS_Y          1
#define ADXL345_AXIS_Z          2
#define ADXL345_AXES_NUM        3
#define ADXL345_DATA_LEN        6
#define ADXL345_DEV_NAME        "ADXL345"
/*----------------------------------------------------------------------------*/
static const struct i2c_device_id adxl345_i2c_id[] = {{ADXL345_DEV_NAME,0},{}};
/*the adapter id will be available in customization*/
static unsigned short adxl345_force[] = {0x00, ADXL345_I2C_SLAVE_ADDR, I2C_CLIENT_END, I2C_CLIENT_END};
static const unsigned short *const adxl345_forces[] = { adxl345_force, NULL };
static struct i2c_client_address_data adxl345_addr_data = { .forces = adxl345_forces,};

/*----------------------------------------------------------------------------*/
static int adxl345_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id); 
static int adxl345_i2c_remove(struct i2c_client *client);
static int adxl345_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);
static int adxl345_suspend(struct i2c_client *client, pm_message_t msg) ;
static int adxl345_resume(struct i2c_client *client);

static int  adxl345_local_init(void);
static int  adxl345_remove(void);

static int adxl345_init_flag =-1; // 0<==>OK -1 <==> fail

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
    s16 raw[C_MAX_FIR_LENGTH][ADXL345_AXES_NUM];
    int sum[ADXL345_AXES_NUM];
    int num;
    int idx;
};
/*----------------------------------------------------------------------------*/

static struct sensor_init_info adxl345_init_info = {
		.name = "adxl345",
		.init = adxl345_local_init,
		.uninit = adxl345_remove,
	
};



struct adxl345_i2c_data {
    struct i2c_client *client;
    struct acc_hw *hw;
    struct hwmsen_convert   cvt;
    
    /*misc*/
    struct data_resolution *reso;
    atomic_t                trace;
    atomic_t                suspend;
    atomic_t                selftest;
	atomic_t				filter;
    s16                     cali_sw[ADXL345_AXES_NUM+1];

    /*data*/
    s8                      offset[ADXL345_AXES_NUM+1];  /*+1: for 4-byte alignment*/
    s16                     data[ADXL345_AXES_NUM+1];

#if defined(CONFIG_ADXL345_LOWPASS)
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
static struct i2c_driver adxl345_i2c_driver = {
    .driver = {
        .owner          = THIS_MODULE,
        .name           = ADXL345_DEV_NAME,
    },
	.probe      		= adxl345_i2c_probe,
	.remove    			= adxl345_i2c_remove,
	.detect				= adxl345_i2c_detect,
//#if !defined(CONFIG_HAS_EARLYSUSPEND)    
    .suspend            = adxl345_suspend,
    .resume             = adxl345_resume,
//#endif
	.id_table = adxl345_i2c_id,
	.address_data = &adxl345_addr_data,
};

/*----------------------------------------------------------------------------*/
static struct i2c_client *adxl345_i2c_client = NULL;
//static struct platform_driver adxl345_gsensor_driver;
static struct adxl345_i2c_data *obj_i2c_data = NULL;
static bool sensor_power = false;
static GSENSOR_VECTOR3D gsensor_gain;
static char selftestRes[8]= {0}; 


/*----------------------------------------------------------------------------*/
#define GSE_TAG                  "[Gsensor] "
#define GSE_FUN(f)               printk(KERN_INFO GSE_TAG"%s\n", __FUNCTION__)
#define GSE_ERR(fmt, args...)    printk(KERN_ERR GSE_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define GSE_LOG(fmt, args...)    printk(KERN_INFO GSE_TAG fmt, ##args)
/*----------------------------------------------------------------------------*/
static struct data_resolution adxl345_data_resolution[] = {
 /*8 combination by {FULL_RES,RANGE}*/
    {{ 3, 9}, 256},   /*+/-2g  in 10-bit resolution:  3.9 mg/LSB*/
    {{ 7, 8}, 128},   /*+/-4g  in 10-bit resolution:  7.8 mg/LSB*/
    {{15, 6},  64},   /*+/-8g  in 10-bit resolution: 15.6 mg/LSB*/
    {{31, 2},  32},   /*+/-16g in 10-bit resolution: 31.2 mg/LSB*/
    {{ 3, 9}, 256},   /*+/-2g  in 10-bit resolution:  3.9 mg/LSB (full-resolution)*/
    {{ 3, 9}, 256},   /*+/-4g  in 11-bit resolution:  3.9 mg/LSB (full-resolution)*/
    {{ 3, 9}, 256},   /*+/-8g  in 12-bit resolution:  3.9 mg/LSB (full-resolution)*/
    {{ 3, 9}, 256},   /*+/-16g in 13-bit resolution:  3.9 mg/LSB (full-resolution)*/            
};
/*----------------------------------------------------------------------------*/
static struct data_resolution adxl345_offset_resolution = {{15, 6}, 64};

/*--------------------ADXL power control function----------------------------------*/
static void ADXL345_power(struct acc_hw *hw, unsigned int on) 
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
			if(!hwPowerOn(hw->power_id, hw->power_vol, "ADXL345"))
			{
				GSE_ERR("power on fails!!\n");
			}
		}
		else	// power off
		{
			if (!hwPowerDown(hw->power_id, "ADXL345"))
			{
				GSE_ERR("power off fail!!\n");
			}			  
		}
	}
	power_on = on;    
}
/*----------------------------------------------------------------------------*/
static int ADXL345_SetDataResolution(struct adxl345_i2c_data *obj)
{
	int err;
	u8  dat, reso;

	if(err = hwmsen_read_byte(obj->client, ADXL345_REG_DATA_FORMAT, &dat))
	{
		GSE_ERR("write data format fail!!\n");
		return err;
	}

	/*the data_reso is combined by 3 bits: {FULL_RES, DATA_RANGE}*/
	reso  = (dat & ADXL345_FULL_RES) ? (0x04) : (0x00);
	reso |= (dat & ADXL345_RANGE_16G); 

	if(reso < sizeof(adxl345_data_resolution)/sizeof(adxl345_data_resolution[0]))
	{        
		obj->reso = &adxl345_data_resolution[reso];
		return 0;
	}
	else
	{
		return -EINVAL;
	}
}
/*----------------------------------------------------------------------------*/
static int ADXL345_ReadData(struct i2c_client *client, s16 data[ADXL345_AXES_NUM])
{
	struct adxl345_i2c_data *priv = i2c_get_clientdata(client);        
	u8 addr = ADXL345_REG_DATAX0;
	u8 buf[ADXL345_DATA_LEN] = {0};
	int err = 0;

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
		data[ADXL345_AXIS_X] = (s16)((buf[ADXL345_AXIS_X*2]) |
		         (buf[ADXL345_AXIS_X*2+1] << 8));
		data[ADXL345_AXIS_Y] = (s16)((buf[ADXL345_AXIS_Y*2]) |
		         (buf[ADXL345_AXIS_Y*2+1] << 8));
		data[ADXL345_AXIS_Z] = (s16)((buf[ADXL345_AXIS_Z*2]) |
		         (buf[ADXL345_AXIS_Z*2+1] << 8));
		data[ADXL345_AXIS_X] += priv->cali_sw[ADXL345_AXIS_X];
		data[ADXL345_AXIS_Y] += priv->cali_sw[ADXL345_AXIS_Y];
		data[ADXL345_AXIS_Z] += priv->cali_sw[ADXL345_AXIS_Z];

		if(atomic_read(&priv->trace) & ADX_TRC_RAWDATA)
		{
			GSE_LOG("[%08X %08X %08X] => [%5d %5d %5d]\n", data[ADXL345_AXIS_X], data[ADXL345_AXIS_Y], data[ADXL345_AXIS_Z],
		                               data[ADXL345_AXIS_X], data[ADXL345_AXIS_Y], data[ADXL345_AXIS_Z]);
		}
#ifdef CONFIG_ADXL345_LOWPASS
		if(atomic_read(&priv->filter))
		{
			if(atomic_read(&priv->fir_en) && !atomic_read(&priv->suspend))
			{
				int idx, firlen = atomic_read(&priv->firlen);   
				if(priv->fir.num < firlen)
				{                
					priv->fir.raw[priv->fir.num][ADXL345_AXIS_X] = data[ADXL345_AXIS_X];
					priv->fir.raw[priv->fir.num][ADXL345_AXIS_Y] = data[ADXL345_AXIS_Y];
					priv->fir.raw[priv->fir.num][ADXL345_AXIS_Z] = data[ADXL345_AXIS_Z];
					priv->fir.sum[ADXL345_AXIS_X] += data[ADXL345_AXIS_X];
					priv->fir.sum[ADXL345_AXIS_Y] += data[ADXL345_AXIS_Y];
					priv->fir.sum[ADXL345_AXIS_Z] += data[ADXL345_AXIS_Z];
					if(atomic_read(&priv->trace) & ADX_TRC_FILTER)
					{
						GSE_LOG("add [%2d] [%5d %5d %5d] => [%5d %5d %5d]\n", priv->fir.num,
							priv->fir.raw[priv->fir.num][ADXL345_AXIS_X], priv->fir.raw[priv->fir.num][ADXL345_AXIS_Y], priv->fir.raw[priv->fir.num][ADXL345_AXIS_Z],
							priv->fir.sum[ADXL345_AXIS_X], priv->fir.sum[ADXL345_AXIS_Y], priv->fir.sum[ADXL345_AXIS_Z]);
					}
					priv->fir.num++;
					priv->fir.idx++;
				}
				else
				{
					idx = priv->fir.idx % firlen;
					priv->fir.sum[ADXL345_AXIS_X] -= priv->fir.raw[idx][ADXL345_AXIS_X];
					priv->fir.sum[ADXL345_AXIS_Y] -= priv->fir.raw[idx][ADXL345_AXIS_Y];
					priv->fir.sum[ADXL345_AXIS_Z] -= priv->fir.raw[idx][ADXL345_AXIS_Z];
					priv->fir.raw[idx][ADXL345_AXIS_X] = data[ADXL345_AXIS_X];
					priv->fir.raw[idx][ADXL345_AXIS_Y] = data[ADXL345_AXIS_Y];
					priv->fir.raw[idx][ADXL345_AXIS_Z] = data[ADXL345_AXIS_Z];
					priv->fir.sum[ADXL345_AXIS_X] += data[ADXL345_AXIS_X];
					priv->fir.sum[ADXL345_AXIS_Y] += data[ADXL345_AXIS_Y];
					priv->fir.sum[ADXL345_AXIS_Z] += data[ADXL345_AXIS_Z];
					priv->fir.idx++;
					data[ADXL345_AXIS_X] = priv->fir.sum[ADXL345_AXIS_X]/firlen;
					data[ADXL345_AXIS_Y] = priv->fir.sum[ADXL345_AXIS_Y]/firlen;
					data[ADXL345_AXIS_Z] = priv->fir.sum[ADXL345_AXIS_Z]/firlen;
					if(atomic_read(&priv->trace) & ADX_TRC_FILTER)
					{
						GSE_LOG("add [%2d] [%5d %5d %5d] => [%5d %5d %5d] : [%5d %5d %5d]\n", idx,
						priv->fir.raw[idx][ADXL345_AXIS_X], priv->fir.raw[idx][ADXL345_AXIS_Y], priv->fir.raw[idx][ADXL345_AXIS_Z],
						priv->fir.sum[ADXL345_AXIS_X], priv->fir.sum[ADXL345_AXIS_Y], priv->fir.sum[ADXL345_AXIS_Z],
						data[ADXL345_AXIS_X], data[ADXL345_AXIS_Y], data[ADXL345_AXIS_Z]);
					}
				}
			}
		}	
#endif         
	}
	return err;
}
/*----------------------------------------------------------------------------*/
static int ADXL345_ReadOffset(struct i2c_client *client, s8 ofs[ADXL345_AXES_NUM])
{    
	int err;

	if(err = hwmsen_read_block(client, ADXL345_REG_OFSX, ofs, ADXL345_AXES_NUM))
	{
		GSE_ERR("error: %d\n", err);
	}
	
	return err;    
}
/*----------------------------------------------------------------------------*/
static int ADXL345_ResetCalibration(struct i2c_client *client)
{
	struct adxl345_i2c_data *obj = i2c_get_clientdata(client);
	s8 ofs[ADXL345_AXES_NUM] = {0x00, 0x00, 0x00};
	int err;

	if(err = hwmsen_write_block(client, ADXL345_REG_OFSX, ofs, ADXL345_AXES_NUM))
	{
		GSE_ERR("error: %d\n", err);
	}

	memset(obj->cali_sw, 0x00, sizeof(obj->cali_sw));
	return err;    
}
/*----------------------------------------------------------------------------*/
static int ADXL345_ReadCalibration(struct i2c_client *client, int dat[ADXL345_AXES_NUM])
{
    struct adxl345_i2c_data *obj = i2c_get_clientdata(client);
    int err;
    int mul;

    if ((err = ADXL345_ReadOffset(client, obj->offset))) {
        GSE_ERR("read offset fail, %d\n", err);
        return err;
    }    
    
    mul = obj->reso->sensitivity/adxl345_offset_resolution.sensitivity;

    dat[obj->cvt.map[ADXL345_AXIS_X]] = obj->cvt.sign[ADXL345_AXIS_X]*(obj->offset[ADXL345_AXIS_X]*mul + obj->cali_sw[ADXL345_AXIS_X]);
    dat[obj->cvt.map[ADXL345_AXIS_Y]] = obj->cvt.sign[ADXL345_AXIS_Y]*(obj->offset[ADXL345_AXIS_Y]*mul + obj->cali_sw[ADXL345_AXIS_Y]);
    dat[obj->cvt.map[ADXL345_AXIS_Z]] = obj->cvt.sign[ADXL345_AXIS_Z]*(obj->offset[ADXL345_AXIS_Z]*mul + obj->cali_sw[ADXL345_AXIS_Z]);                        
                                       
    return 0;
}
/*----------------------------------------------------------------------------*/
static int ADXL345_ReadCalibrationEx(struct i2c_client *client, int act[ADXL345_AXES_NUM], int raw[ADXL345_AXES_NUM])
{  
	/*raw: the raw calibration data; act: the actual calibration data*/
	struct adxl345_i2c_data *obj = i2c_get_clientdata(client);
	int err;
	int mul;

	if(err = ADXL345_ReadOffset(client, obj->offset))
	{
		GSE_ERR("read offset fail, %d\n", err);
		return err;
	}    

	mul = obj->reso->sensitivity/adxl345_offset_resolution.sensitivity;
	raw[ADXL345_AXIS_X] = obj->offset[ADXL345_AXIS_X]*mul + obj->cali_sw[ADXL345_AXIS_X];
	raw[ADXL345_AXIS_Y] = obj->offset[ADXL345_AXIS_Y]*mul + obj->cali_sw[ADXL345_AXIS_Y];
	raw[ADXL345_AXIS_Z] = obj->offset[ADXL345_AXIS_Z]*mul + obj->cali_sw[ADXL345_AXIS_Z];

	act[obj->cvt.map[ADXL345_AXIS_X]] = obj->cvt.sign[ADXL345_AXIS_X]*raw[ADXL345_AXIS_X];
	act[obj->cvt.map[ADXL345_AXIS_Y]] = obj->cvt.sign[ADXL345_AXIS_Y]*raw[ADXL345_AXIS_Y];
	act[obj->cvt.map[ADXL345_AXIS_Z]] = obj->cvt.sign[ADXL345_AXIS_Z]*raw[ADXL345_AXIS_Z];                        
	                       
	return 0;
}
/*----------------------------------------------------------------------------*/
static int ADXL345_WriteCalibration(struct i2c_client *client, int dat[ADXL345_AXES_NUM])
{
	struct adxl345_i2c_data *obj = i2c_get_clientdata(client);
	int err;
	int cali[ADXL345_AXES_NUM], raw[ADXL345_AXES_NUM];
	int lsb = adxl345_offset_resolution.sensitivity;
	int divisor = obj->reso->sensitivity/lsb;

	if(err = ADXL345_ReadCalibrationEx(client, cali, raw))	/*offset will be updated in obj->offset*/
	{ 
		GSE_ERR("read offset fail, %d\n", err);
		return err;
	}

	GSE_LOG("OLDOFF: (%+3d %+3d %+3d): (%+3d %+3d %+3d) / (%+3d %+3d %+3d)\n", 
		raw[ADXL345_AXIS_X], raw[ADXL345_AXIS_Y], raw[ADXL345_AXIS_Z],
		obj->offset[ADXL345_AXIS_X], obj->offset[ADXL345_AXIS_Y], obj->offset[ADXL345_AXIS_Z],
		obj->cali_sw[ADXL345_AXIS_X], obj->cali_sw[ADXL345_AXIS_Y], obj->cali_sw[ADXL345_AXIS_Z]);

	/*calculate the real offset expected by caller*/
	cali[ADXL345_AXIS_X] += dat[ADXL345_AXIS_X];
	cali[ADXL345_AXIS_Y] += dat[ADXL345_AXIS_Y];
	cali[ADXL345_AXIS_Z] += dat[ADXL345_AXIS_Z];

	GSE_LOG("UPDATE: (%+3d %+3d %+3d)\n", 
		dat[ADXL345_AXIS_X], dat[ADXL345_AXIS_Y], dat[ADXL345_AXIS_Z]);

	obj->offset[ADXL345_AXIS_X] = (s8)(obj->cvt.sign[ADXL345_AXIS_X]*(cali[obj->cvt.map[ADXL345_AXIS_X]])/(divisor));
	obj->offset[ADXL345_AXIS_Y] = (s8)(obj->cvt.sign[ADXL345_AXIS_Y]*(cali[obj->cvt.map[ADXL345_AXIS_Y]])/(divisor));
	obj->offset[ADXL345_AXIS_Z] = (s8)(obj->cvt.sign[ADXL345_AXIS_Z]*(cali[obj->cvt.map[ADXL345_AXIS_Z]])/(divisor));

	/*convert software calibration using standard calibration*/
	obj->cali_sw[ADXL345_AXIS_X] = obj->cvt.sign[ADXL345_AXIS_X]*(cali[obj->cvt.map[ADXL345_AXIS_X]])%(divisor);
	obj->cali_sw[ADXL345_AXIS_Y] = obj->cvt.sign[ADXL345_AXIS_Y]*(cali[obj->cvt.map[ADXL345_AXIS_Y]])%(divisor);
	obj->cali_sw[ADXL345_AXIS_Z] = obj->cvt.sign[ADXL345_AXIS_Z]*(cali[obj->cvt.map[ADXL345_AXIS_Z]])%(divisor);

	GSE_LOG("NEWOFF: (%+3d %+3d %+3d): (%+3d %+3d %+3d) / (%+3d %+3d %+3d)\n", 
		obj->offset[ADXL345_AXIS_X]*divisor + obj->cali_sw[ADXL345_AXIS_X], 
		obj->offset[ADXL345_AXIS_Y]*divisor + obj->cali_sw[ADXL345_AXIS_Y], 
		obj->offset[ADXL345_AXIS_Z]*divisor + obj->cali_sw[ADXL345_AXIS_Z], 
		obj->offset[ADXL345_AXIS_X], obj->offset[ADXL345_AXIS_Y], obj->offset[ADXL345_AXIS_Z],
		obj->cali_sw[ADXL345_AXIS_X], obj->cali_sw[ADXL345_AXIS_Y], obj->cali_sw[ADXL345_AXIS_Z]);

	if(err = hwmsen_write_block(obj->client, ADXL345_REG_OFSX, obj->offset, ADXL345_AXES_NUM))
	{
		GSE_ERR("write offset fail: %d\n", err);
		return err;
	}

	return err;
}
/*----------------------------------------------------------------------------*/
static int ADXL345_CheckDeviceID(struct i2c_client *client)
{
	u8 databuf[10];    
	int res = 0;

	memset(databuf, 0, sizeof(u8)*10);    
	databuf[0] = ADXL345_REG_DEVID;    

	res = i2c_master_send(client, databuf, 0x1);
	if(res <= 0)
	{
		goto exit_ADXL345_CheckDeviceID;
	}
	
	udelay(500);

	databuf[0] = 0x0;        
	res = i2c_master_recv(client, databuf, 0x01);
	if(res <= 0)
	{
		goto exit_ADXL345_CheckDeviceID;
	}
	

	if(databuf[0]!=ADXL345_FIXED_DEVID)
	{
		return ADXL345_ERR_IDENTIFICATION;
	}

	exit_ADXL345_CheckDeviceID:
	if (res <= 0)
	{
		return ADXL345_ERR_I2C;
	}
	
	return ADXL345_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int ADXL345_SetPowerMode(struct i2c_client *client, bool enable)
{
	u8 databuf[2];    
	int res = 0;
	u8 addr = ADXL345_REG_POWER_CTL;
	struct adxl345_i2c_data *obj = i2c_get_clientdata(client);
	
	
	if(enable == sensor_power)
	{
		GSE_LOG("Sensor power status is newest!\n");
		return ADXL345_SUCCESS;
	}

	if(hwmsen_read_block(client, addr, databuf, 0x01))
	{
		GSE_ERR("read power ctl register err!\n");
		return ADXL345_ERR_I2C;
	}

	databuf[0] &= ~ADXL345_MEASURE_MODE;
	
	if(enable == TRUE)
	{
		databuf[0] |= ADXL345_MEASURE_MODE;
	}
	else
	{
		// do nothing
	}
	databuf[1] = databuf[0];
	databuf[0] = ADXL345_REG_POWER_CTL;
	

	res = i2c_master_send(client, databuf, 0x2);

	if(res <= 0)
	{
		GSE_LOG("set power mode failed!\n");
		return ADXL345_ERR_I2C;
	}
	else if(atomic_read(&obj->trace) & ADX_TRC_INFO)
	{
		GSE_LOG("set power mode ok %d!\n", databuf[1]);
	}

	sensor_power = enable;
	
	return ADXL345_SUCCESS;    
}
/*----------------------------------------------------------------------------*/
static int ADXL345_SetDataFormat(struct i2c_client *client, u8 dataformat)
{
	struct adxl345_i2c_data *obj = i2c_get_clientdata(client);
	u8 databuf[10];    
	int res = 0;

	memset(databuf, 0, sizeof(u8)*10);    
	databuf[0] = ADXL345_REG_DATA_FORMAT;    
	databuf[1] = dataformat;

	res = i2c_master_send(client, databuf, 0x2);

	if(res <= 0)
	{
		return ADXL345_ERR_I2C;
	}
	

	return ADXL345_SetDataResolution(obj);    
}
/*----------------------------------------------------------------------------*/
static int ADXL345_SetBWRate(struct i2c_client *client, u8 bwrate)
{
	u8 databuf[10];    
	int res = 0;

	memset(databuf, 0, sizeof(u8)*10);    
	databuf[0] = ADXL345_REG_BW_RATE;    
	databuf[1] = bwrate;

	res = i2c_master_send(client, databuf, 0x2);

	if(res <= 0)
	{
		return ADXL345_ERR_I2C;
	}
	
	return ADXL345_SUCCESS;    
}
/*----------------------------------------------------------------------------*/
static int ADXL345_SetIntEnable(struct i2c_client *client, u8 intenable)
{
	u8 databuf[10];    
	int res = 0;

	memset(databuf, 0, sizeof(u8)*10);    
	databuf[0] = ADXL345_REG_INT_ENABLE;    
	databuf[1] = intenable;

	res = i2c_master_send(client, databuf, 0x2);

	if(res <= 0)
	{
		return ADXL345_ERR_I2C;
	}
	
	return ADXL345_SUCCESS;    
}
/*----------------------------------------------------------------------------*/
static int adxl345_init_client(struct i2c_client *client, int reset_cali)
{
	struct adxl345_i2c_data *obj = i2c_get_clientdata(client);
	int res = 0;

	res = ADXL345_CheckDeviceID(client); 
	if(res != ADXL345_SUCCESS)
	{
		return res;
	}	

	res = ADXL345_SetPowerMode(client, false);
	if(res != ADXL345_SUCCESS)
	{
		return res;
	}
	

	res = ADXL345_SetBWRate(client, ADXL345_BW_100HZ);
	if(res != ADXL345_SUCCESS ) //0x2C->BW=100Hz
	{
		return res;
	}

	res = ADXL345_SetDataFormat(client, ADXL345_FULL_RES|ADXL345_RANGE_2G);
	if(res != ADXL345_SUCCESS) //0x2C->BW=100Hz
	{
		return res;
	}

	gsensor_gain.x = gsensor_gain.y = gsensor_gain.z = obj->reso->sensitivity;
/*
	res = ADXL345_SetIntEnable(client, ADXL345_DATA_READY);        
	if(res != ADXL345_SUCCESS)//0x2E->0x80
	{
		return res;
	}
*/
	if(0 != reset_cali)
	{ 
		/*reset calibration only in power on*/
		res = ADXL345_ResetCalibration(client);
		if(res != ADXL345_SUCCESS)
		{
			return res;
		}
	}

#ifdef CONFIG_ADXL345_LOWPASS
	memset(&obj->fir, 0x00, sizeof(obj->fir));  
#endif

	return ADXL345_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int ADXL345_ReadChipInfo(struct i2c_client *client, char *buf, int bufsize)
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

	sprintf(buf, "ADXL345 Chip");
	return 0;
}
/*----------------------------------------------------------------------------*/
static int ADXL345_ReadSensorData(struct i2c_client *client, char *buf, int bufsize)
{
	struct adxl345_i2c_data *obj = (struct adxl345_i2c_data*)i2c_get_clientdata(client);
	u8 databuf[20];
	int acc[ADXL345_AXES_NUM];
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
		res = ADXL345_SetPowerMode(client, true);
		if(res)
		{
			GSE_ERR("Power on adxl345 error %d!\n", res);
		}
		msleep(20);
	}

	if(res = ADXL345_ReadData(client, obj->data))
	{        
		GSE_ERR("I2C error: ret value=%d", res);
		return -3;
	}
	else
	{
		obj->data[ADXL345_AXIS_X] += obj->cali_sw[ADXL345_AXIS_X];
		obj->data[ADXL345_AXIS_Y] += obj->cali_sw[ADXL345_AXIS_Y];
		obj->data[ADXL345_AXIS_Z] += obj->cali_sw[ADXL345_AXIS_Z];
		
		/*remap coordinate*/
		acc[obj->cvt.map[ADXL345_AXIS_X]] = obj->cvt.sign[ADXL345_AXIS_X]*obj->data[ADXL345_AXIS_X];
		acc[obj->cvt.map[ADXL345_AXIS_Y]] = obj->cvt.sign[ADXL345_AXIS_Y]*obj->data[ADXL345_AXIS_Y];
		acc[obj->cvt.map[ADXL345_AXIS_Z]] = obj->cvt.sign[ADXL345_AXIS_Z]*obj->data[ADXL345_AXIS_Z];

		//GSE_LOG("Mapped gsensor data: %d, %d, %d!\n", acc[ADXL345_AXIS_X], acc[ADXL345_AXIS_Y], acc[ADXL345_AXIS_Z]);

		//Out put the mg
		acc[ADXL345_AXIS_X] = acc[ADXL345_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		acc[ADXL345_AXIS_Y] = acc[ADXL345_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		acc[ADXL345_AXIS_Z] = acc[ADXL345_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;		
		

		sprintf(buf, "%04x %04x %04x", acc[ADXL345_AXIS_X], acc[ADXL345_AXIS_Y], acc[ADXL345_AXIS_Z]);
		if(atomic_read(&obj->trace) & ADX_TRC_IOCTL)
		{
			GSE_LOG("gsensor data: %s!\n", buf);
		}
	}
	
	return 0;
}
/*----------------------------------------------------------------------------*/
static int ADXL345_ReadRawData(struct i2c_client *client, char *buf)
{
	struct adxl345_i2c_data *obj = (struct adxl345_i2c_data*)i2c_get_clientdata(client);
	int res = 0;

	if (!buf || !client)
	{
		return EINVAL;
	}
	
	if(res = ADXL345_ReadData(client, obj->data))
	{        
		GSE_ERR("I2C error: ret value=%d", res);
		return EIO;
	}
	else
	{
		sprintf(buf, "%04x %04x %04x", obj->data[ADXL345_AXIS_X], 
			obj->data[ADXL345_AXIS_Y], obj->data[ADXL345_AXIS_Z]);
	
	}
	
	return 0;
}
/*----------------------------------------------------------------------------*/
static int ADXL345_InitSelfTest(struct i2c_client *client)
{
	int res = 0;
	u8  data;

	res = ADXL345_SetBWRate(client, ADXL345_BW_100HZ);
	if(res != ADXL345_SUCCESS ) //0x2C->BW=100Hz
	{
		return res;
	}
	
	res = hwmsen_read_byte(client, ADXL345_REG_DATA_FORMAT, &data);
	if(res != ADXL345_SUCCESS)
	{
		return res;
	}

	res = ADXL345_SetDataFormat(client, ADXL345_SELF_TEST|data);
	if(res != ADXL345_SUCCESS) //0x2C->BW=100Hz
	{
		return res;
	}
	
	return ADXL345_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int ADXL345_JudgeTestResult(struct i2c_client *client, s32 prv[ADXL345_AXES_NUM], s32 nxt[ADXL345_AXES_NUM])
{
    struct criteria {
        int min;
        int max;
    };
	
    struct criteria self[4][3] = {
        {{50, 540}, {-540, -50}, {75, 875}},
        {{25, 270}, {-270, -25}, {38, 438}},
        {{12, 135}, {-135, -12}, {19, 219}},            
        {{ 6,  67}, {-67,  -6},  {10, 110}},            
    };
    struct criteria (*ptr)[3] = NULL;
    u8 format;
    int res;
    if(res = hwmsen_read_byte(client, ADXL345_REG_DATA_FORMAT, &format))
        return res;
    if(format & ADXL345_FULL_RES)
        ptr = &self[0];
    else if ((format & ADXL345_RANGE_4G))
        ptr = &self[1];
    else if ((format & ADXL345_RANGE_8G))
        ptr = &self[2];
    else if ((format & ADXL345_RANGE_16G))
        ptr = &self[3];

    if (!ptr) {
        GSE_ERR("null pointer\n");
        return -EINVAL;
    }
	GSE_LOG("format=%x\n",format);

    if (((nxt[ADXL345_AXIS_X] - prv[ADXL345_AXIS_X]) > (*ptr)[ADXL345_AXIS_X].max) ||
        ((nxt[ADXL345_AXIS_X] - prv[ADXL345_AXIS_X]) < (*ptr)[ADXL345_AXIS_X].min)) {
        GSE_ERR("X is over range\n");
        res = -EINVAL;
    }
    if (((nxt[ADXL345_AXIS_Y] - prv[ADXL345_AXIS_Y]) > (*ptr)[ADXL345_AXIS_Y].max) ||
        ((nxt[ADXL345_AXIS_Y] - prv[ADXL345_AXIS_Y]) < (*ptr)[ADXL345_AXIS_Y].min)) {
        GSE_ERR("Y is over range\n");
        res = -EINVAL;
    }
    if (((nxt[ADXL345_AXIS_Z] - prv[ADXL345_AXIS_Z]) > (*ptr)[ADXL345_AXIS_Z].max) ||
        ((nxt[ADXL345_AXIS_Z] - prv[ADXL345_AXIS_Z]) < (*ptr)[ADXL345_AXIS_Z].min)) {
        GSE_ERR("Z is over range\n");
        res = -EINVAL;
    }
    return res;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = adxl345_i2c_client;
	char strbuf[ADXL345_BUFSIZE];
	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	
	ADXL345_ReadChipInfo(client, strbuf, ADXL345_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);        
}
/*----------------------------------------------------------------------------*/
static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = adxl345_i2c_client;
	char strbuf[ADXL345_BUFSIZE];
	
	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	ADXL345_ReadSensorData(client, strbuf, ADXL345_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);            
}
/*----------------------------------------------------------------------------*/
static ssize_t show_cali_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = adxl345_i2c_client;
	struct adxl345_i2c_data *obj;
	int err, len = 0, mul;
	int tmp[ADXL345_AXES_NUM];

	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}

	obj = i2c_get_clientdata(client);



	if(err = ADXL345_ReadOffset(client, obj->offset))
	{
		return -EINVAL;
	}
	else if(err = ADXL345_ReadCalibration(client, tmp))
	{
		return -EINVAL;
	}
	else
	{    
		mul = obj->reso->sensitivity/adxl345_offset_resolution.sensitivity;
		len += snprintf(buf+len, PAGE_SIZE-len, "[HW ][%d] (%+3d, %+3d, %+3d) : (0x%02X, 0x%02X, 0x%02X)\n", mul,                        
			obj->offset[ADXL345_AXIS_X], obj->offset[ADXL345_AXIS_Y], obj->offset[ADXL345_AXIS_Z],
			obj->offset[ADXL345_AXIS_X], obj->offset[ADXL345_AXIS_Y], obj->offset[ADXL345_AXIS_Z]);
		len += snprintf(buf+len, PAGE_SIZE-len, "[SW ][%d] (%+3d, %+3d, %+3d)\n", 1, 
			obj->cali_sw[ADXL345_AXIS_X], obj->cali_sw[ADXL345_AXIS_Y], obj->cali_sw[ADXL345_AXIS_Z]);

		len += snprintf(buf+len, PAGE_SIZE-len, "[ALL]    (%+3d, %+3d, %+3d) : (%+3d, %+3d, %+3d)\n", 
			obj->offset[ADXL345_AXIS_X]*mul + obj->cali_sw[ADXL345_AXIS_X],
			obj->offset[ADXL345_AXIS_Y]*mul + obj->cali_sw[ADXL345_AXIS_Y],
			obj->offset[ADXL345_AXIS_Z]*mul + obj->cali_sw[ADXL345_AXIS_Z],
			tmp[ADXL345_AXIS_X], tmp[ADXL345_AXIS_Y], tmp[ADXL345_AXIS_Z]);
		
		return len;
    }
}
/*----------------------------------------------------------------------------*/
static ssize_t store_cali_value(struct device_driver *ddri, char *buf, size_t count)
{
	struct i2c_client *client = adxl345_i2c_client;  
	int err, x, y, z;
	int dat[ADXL345_AXES_NUM];

	if(!strncmp(buf, "rst", 3))
	{
		if(err = ADXL345_ResetCalibration(client))
		{
			GSE_ERR("reset offset err = %d\n", err);
		}	
	}
	else if(3 == sscanf(buf, "0x%02X 0x%02X 0x%02X", &x, &y, &z))
	{
		dat[ADXL345_AXIS_X] = x;
		dat[ADXL345_AXIS_Y] = y;
		dat[ADXL345_AXIS_Z] = z;
		if(err = ADXL345_WriteCalibration(client, dat))
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
	struct i2c_client *client = adxl345_i2c_client;
	struct adxl345_i2c_data *obj;

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
	s16 raw[ADXL345_AXES_NUM];
	};
	
	struct i2c_client *client = adxl345_i2c_client;  
	int idx, res, num;
	struct item *prv = NULL, *nxt = NULL;
	s32 avg_prv[ADXL345_AXES_NUM] = {0, 0, 0};
	s32 avg_nxt[ADXL345_AXES_NUM] = {0, 0, 0};


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
	ADXL345_SetPowerMode(client,true);
	for(idx = 0; idx < num; idx++)
	{
		if(res = ADXL345_ReadData(client, prv[idx].raw))
		{            
			GSE_ERR("read data fail: %d\n", res);
			goto exit;
		}
		
		avg_prv[ADXL345_AXIS_X] += prv[idx].raw[ADXL345_AXIS_X];
		avg_prv[ADXL345_AXIS_Y] += prv[idx].raw[ADXL345_AXIS_Y];
		avg_prv[ADXL345_AXIS_Z] += prv[idx].raw[ADXL345_AXIS_Z];        
		GSE_LOG("[%5d %5d %5d]\n", prv[idx].raw[ADXL345_AXIS_X], prv[idx].raw[ADXL345_AXIS_Y], prv[idx].raw[ADXL345_AXIS_Z]);
	}
	
	avg_prv[ADXL345_AXIS_X] /= num;
	avg_prv[ADXL345_AXIS_Y] /= num;
	avg_prv[ADXL345_AXIS_Z] /= num;    

	/*initial setting for self test*/
	ADXL345_InitSelfTest(client);
	GSE_LOG("SELFTEST:\n");    
	for(idx = 0; idx < num; idx++)
	{
		if(res = ADXL345_ReadData(client, nxt[idx].raw))
		{            
			GSE_ERR("read data fail: %d\n", res);
			goto exit;
		}
		avg_nxt[ADXL345_AXIS_X] += nxt[idx].raw[ADXL345_AXIS_X];
		avg_nxt[ADXL345_AXIS_Y] += nxt[idx].raw[ADXL345_AXIS_Y];
		avg_nxt[ADXL345_AXIS_Z] += nxt[idx].raw[ADXL345_AXIS_Z];        
		GSE_LOG("[%5d %5d %5d]\n", nxt[idx].raw[ADXL345_AXIS_X], nxt[idx].raw[ADXL345_AXIS_Y], nxt[idx].raw[ADXL345_AXIS_Z]);
	}
	
	avg_nxt[ADXL345_AXIS_X] /= num;
	avg_nxt[ADXL345_AXIS_Y] /= num;
	avg_nxt[ADXL345_AXIS_Z] /= num;    

	GSE_LOG("X: %5d - %5d = %5d \n", avg_nxt[ADXL345_AXIS_X], avg_prv[ADXL345_AXIS_X], avg_nxt[ADXL345_AXIS_X] - avg_prv[ADXL345_AXIS_X]);
	GSE_LOG("Y: %5d - %5d = %5d \n", avg_nxt[ADXL345_AXIS_Y], avg_prv[ADXL345_AXIS_Y], avg_nxt[ADXL345_AXIS_Y] - avg_prv[ADXL345_AXIS_Y]);
	GSE_LOG("Z: %5d - %5d = %5d \n", avg_nxt[ADXL345_AXIS_Z], avg_prv[ADXL345_AXIS_Z], avg_nxt[ADXL345_AXIS_Z] - avg_prv[ADXL345_AXIS_Z]); 

	if(!ADXL345_JudgeTestResult(client, avg_prv, avg_nxt))
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
	adxl345_init_client(client, 0);
	kfree(prv);
	kfree(nxt);
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_selftest_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = adxl345_i2c_client;
	struct adxl345_i2c_data *obj;

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
	struct adxl345_i2c_data *obj = obj_i2c_data;
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
			adxl345_init_client(obj->client, 0);
		}
		else if(!atomic_read(&obj->selftest) && tmp)
		{
			/*disable -> enable*/
			ADXL345_InitSelfTest(obj->client);            
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
#ifdef CONFIG_ADXL345_LOWPASS
	struct i2c_client *client = adxl345_i2c_client;
	struct adxl345_i2c_data *obj = i2c_get_clientdata(client);
	if(atomic_read(&obj->firlen))
	{
		int idx, len = atomic_read(&obj->firlen);
		GSE_LOG("len = %2d, idx = %2d\n", obj->fir.num, obj->fir.idx);

		for(idx = 0; idx < len; idx++)
		{
			GSE_LOG("[%5d %5d %5d]\n", obj->fir.raw[idx][ADXL345_AXIS_X], obj->fir.raw[idx][ADXL345_AXIS_Y], obj->fir.raw[idx][ADXL345_AXIS_Z]);
		}
		
		GSE_LOG("sum = [%5d %5d %5d]\n", obj->fir.sum[ADXL345_AXIS_X], obj->fir.sum[ADXL345_AXIS_Y], obj->fir.sum[ADXL345_AXIS_Z]);
		GSE_LOG("avg = [%5d %5d %5d]\n", obj->fir.sum[ADXL345_AXIS_X]/len, obj->fir.sum[ADXL345_AXIS_Y]/len, obj->fir.sum[ADXL345_AXIS_Z]/len);
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&obj->firlen));
#else
	return snprintf(buf, PAGE_SIZE, "not support\n");
#endif
}
/*----------------------------------------------------------------------------*/
static ssize_t store_firlen_value(struct device_driver *ddri, char *buf, size_t count)
{
#ifdef CONFIG_ADXL345_LOWPASS
	struct i2c_client *client = adxl345_i2c_client;  
	struct adxl345_i2c_data *obj = i2c_get_clientdata(client);
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
	struct adxl345_i2c_data *obj = obj_i2c_data;
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
	struct adxl345_i2c_data *obj = obj_i2c_data;
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
	struct adxl345_i2c_data *obj = obj_i2c_data;
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
static DRIVER_ATTR(chipinfo,             S_IRUGO, show_chipinfo_value,      NULL);
static DRIVER_ATTR(sensordata,           S_IRUGO, show_sensordata_value,    NULL);
static DRIVER_ATTR(cali,       S_IWUSR | S_IRUGO, show_cali_value,          store_cali_value);
static DRIVER_ATTR(self,       S_IWUSR | S_IRUGO, show_selftest_value,          store_selftest_value);
static DRIVER_ATTR(selftest,   S_IWUSR | S_IRUGO, show_self_value ,      store_self_value );
static DRIVER_ATTR(firlen,     S_IWUSR | S_IRUGO, show_firlen_value,        store_firlen_value);
static DRIVER_ATTR(trace,      S_IWUSR | S_IRUGO, show_trace_value,         store_trace_value);
static DRIVER_ATTR(status,               S_IRUGO, show_status_value,        NULL);
/*----------------------------------------------------------------------------*/
static struct driver_attribute *adxl345_attr_list[] = {
	&driver_attr_chipinfo,     /*chip information*/
	&driver_attr_sensordata,   /*dump sensor data*/
	&driver_attr_cali,         /*show calibration data*/
	&driver_attr_self,         /*self test demo*/
	&driver_attr_selftest,     /*self control: 0: disable, 1: enable*/
	&driver_attr_firlen,       /*filter length: 0: disable, others: enable*/
	&driver_attr_trace,        /*trace log*/
	&driver_attr_status,        
};
/*----------------------------------------------------------------------------*/
static int adxl345_create_attr(struct device_driver *driver) 
{
	int idx, err = 0;
	int num = (int)(sizeof(adxl345_attr_list)/sizeof(adxl345_attr_list[0]));
	if (driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
		if(err = driver_create_file(driver, adxl345_attr_list[idx]))
		{            
			GSE_ERR("driver_create_file (%s) = %d\n", adxl345_attr_list[idx]->attr.name, err);
			break;
		}
	}    
	return err;
}
/*----------------------------------------------------------------------------*/
static int adxl345_delete_attr(struct device_driver *driver)
{
	int idx ,err = 0;
	int num = (int)(sizeof(adxl345_attr_list)/sizeof(adxl345_attr_list[0]));

	if(driver == NULL)
	{
		return -EINVAL;
	}
	

	for(idx = 0; idx < num; idx++)
	{
		driver_remove_file(driver, adxl345_attr_list[idx]);
	}
	

	return err;
}

/*----------------------------------------------------------------------------*/
int adxl345_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value, sample_delay;	
	struct adxl345_i2c_data *priv = (struct adxl345_i2c_data*)self;
	hwm_sensor_data* gsensor_data;
	char buff[ADXL345_BUFSIZE];
	
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
					sample_delay = ADXL345_BW_200HZ;
				}
				else if(value <= 10)
				{
					sample_delay = ADXL345_BW_100HZ;
				}
				else
				{
					sample_delay = ADXL345_BW_50HZ;
				}
				
				err = ADXL345_SetBWRate(priv->client, sample_delay);
				if(err != ADXL345_SUCCESS ) //0x2C->BW=100Hz
				{
					GSE_ERR("Set delay parameter error!\n");
				}

				if(value >= 50)
				{
					atomic_set(&priv->filter, 0);
				}
				else
				{					
					priv->fir.num = 0;
					priv->fir.idx = 0;
					priv->fir.sum[ADXL345_AXIS_X] = 0;
					priv->fir.sum[ADXL345_AXIS_Y] = 0;
					priv->fir.sum[ADXL345_AXIS_Z] = 0;
					atomic_set(&priv->filter, 1);
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
					err = ADXL345_SetPowerMode( priv->client, !sensor_power);
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
				ADXL345_ReadSensorData(priv->client, buff, ADXL345_BUFSIZE);
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
static int adxl345_open(struct inode *inode, struct file *file)
{
	file->private_data = adxl345_i2c_client;

	if(file->private_data == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return -EINVAL;
	}
	return nonseekable_open(inode, file);
}
/*----------------------------------------------------------------------------*/
static int adxl345_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}
/*----------------------------------------------------------------------------*/
static int adxl345_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
       unsigned long arg)
{
	struct i2c_client *client = (struct i2c_client*)file->private_data;
	struct adxl345_i2c_data *obj = (struct adxl345_i2c_data*)i2c_get_clientdata(client);	
	char strbuf[ADXL345_BUFSIZE];
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
			adxl345_init_client(client, 0);			
			break;

		case GSENSOR_IOCTL_READ_CHIPINFO:
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			
			ADXL345_ReadChipInfo(client, strbuf, ADXL345_BUFSIZE);
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
			
			ADXL345_ReadSensorData(client, strbuf, ADXL345_BUFSIZE);
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
			ADXL345_ReadRawData(client, strbuf);
			if(copy_to_user(data, strbuf, strlen(strbuf)+1))
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
				cali[ADXL345_AXIS_X] = sensor_data.x * obj->reso->sensitivity / GRAVITY_EARTH_1000;
				cali[ADXL345_AXIS_Y] = sensor_data.y * obj->reso->sensitivity / GRAVITY_EARTH_1000;
				cali[ADXL345_AXIS_Z] = sensor_data.z * obj->reso->sensitivity / GRAVITY_EARTH_1000;			  
				err = ADXL345_WriteCalibration(client, cali);			 
			}
			break;

		case GSENSOR_IOCTL_CLR_CALI:
			err = ADXL345_ResetCalibration(client);
			break;

		case GSENSOR_IOCTL_GET_CALI:
			data = (void __user*)arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			if(err = ADXL345_ReadCalibration(client, cali))
			{
				break;
			}
			
			sensor_data.x = cali[ADXL345_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
			sensor_data.y = cali[ADXL345_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
			sensor_data.z = cali[ADXL345_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
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
static struct file_operations adxl345_fops = {
	.owner = THIS_MODULE,
	.open = adxl345_open,
	.release = adxl345_release,
	.ioctl = adxl345_ioctl,
};
/*----------------------------------------------------------------------------*/
static struct miscdevice adxl345_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "gsensor",
	.fops = &adxl345_fops,
};
/*----------------------------------------------------------------------------*/
//#ifndef CONFIG_HAS_EARLYSUSPEND
/*----------------------------------------------------------------------------*/
static int adxl345_suspend(struct i2c_client *client, pm_message_t msg) 
{
	struct adxl345_i2c_data *obj = i2c_get_clientdata(client);    
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
		if(err = ADXL345_SetPowerMode(obj->client, false))
		{
			GSE_ERR("write power control fail!!\n");
			return err;
		}        
		ADXL345_power(obj->hw, 0);
		GSE_LOG("adxl345_suspend ok\n");
	}
	return err;
}
/*----------------------------------------------------------------------------*/
static int adxl345_resume(struct i2c_client *client)
{
	struct adxl345_i2c_data *obj = i2c_get_clientdata(client);        
	int err;
	GSE_FUN();

	if(obj == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return -EINVAL;
	}

	ADXL345_power(obj->hw, 1);
	if(err = adxl345_init_client(client, 0))
	{
		GSE_ERR("initialize client fail!!\n");
		return err;        
	}
	atomic_set(&obj->suspend, 0);
	GSE_LOG("adxl345_resume ok\n");

	return 0;
}
/*----------------------------------------------------------------------------*/
//#else /*CONFIG_HAS_EARLY_SUSPEND is defined*/
/*----------------------------------------------------------------------------*/
static void adxl345_early_suspend(struct early_suspend *h) 
{
	struct adxl345_i2c_data *obj = container_of(h, struct adxl345_i2c_data, early_drv);   
	int err;
	GSE_FUN();    

	if(obj == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return;
	}
	atomic_set(&obj->suspend, 1); 
	if(err = ADXL345_SetPowerMode(obj->client, false))
	{
		GSE_ERR("write power control fail!!\n");
		return;
	}

	sensor_power = false;
	
	ADXL345_power(obj->hw, 0);
}
/*----------------------------------------------------------------------------*/
static void adxl345_late_resume(struct early_suspend *h)
{
	struct adxl345_i2c_data *obj = container_of(h, struct adxl345_i2c_data, early_drv);         
	int err;
	GSE_FUN();

	if(obj == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return;
	}

	ADXL345_power(obj->hw, 1);
	if(err = adxl345_init_client(obj->client, 0))
	{
		GSE_ERR("initialize client fail!!\n");
		return;        
	}
	atomic_set(&obj->suspend, 0);    
}
/*----------------------------------------------------------------------------*/
//#endif /*CONFIG_HAS_EARLYSUSPEND*/
/*----------------------------------------------------------------------------*/
static int adxl345_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info) 
{    
	strcpy(info->type, ADXL345_DEV_NAME);
	return 0;
}

/*----------------------------------------------------------------------------*/
static int adxl345_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_client *new_client;
	struct adxl345_i2c_data *obj;
	struct hwmsen_object sobj;
	int err = 0;
	GSE_FUN();

	if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
	{
		err = -ENOMEM;
		goto exit;
	}
	
	memset(obj, 0, sizeof(struct adxl345_i2c_data));

	obj->hw = adxl345_get_cust_acc_hw();
	
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
	
#ifdef CONFIG_ADXL345_LOWPASS
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

	adxl345_i2c_client = new_client;	

	if(err = adxl345_init_client(new_client, 1))
	{
		goto exit_init_failed;
	}
	

	if(err = misc_register(&adxl345_device))
	{
		GSE_ERR("adxl345_device register failed\n");
		goto exit_misc_device_register_failed;
	}

	if(err = adxl345_create_attr(&(adxl345_init_info.platform_diver_addr->driver)))
	{
		GSE_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}

	sobj.self = obj;
    sobj.polling = 1;
    sobj.sensor_operate = adxl345_operate;
	if(err = hwmsen_attach(ID_ACCELEROMETER, &sobj))
	{
		GSE_ERR("attach fail = %d\n", err);
		goto exit_kfree;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	obj->early_drv.level    = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
	obj->early_drv.suspend  = adxl345_early_suspend,
	obj->early_drv.resume   = adxl345_late_resume,    
	register_early_suspend(&obj->early_drv);
#endif 

	GSE_LOG("%s: OK\n", __func__);    
    adxl345_init_flag =0;
	return 0;

	exit_create_attr_failed:
	misc_deregister(&adxl345_device);
	exit_misc_device_register_failed:
	exit_init_failed:
	//i2c_detach_client(new_client);
	exit_kfree:
	kfree(obj);
	exit:
	GSE_ERR("%s: err = %d\n", __func__, err);  
	//
	//platform_driver_unregister(&adxl345_gsensor_driver);
	adxl345_init_flag = -1;
	return err;
}

/*----------------------------------------------------------------------------*/
static int adxl345_i2c_remove(struct i2c_client *client)
{
	int err = 0;	
	
	if(err = adxl345_delete_attr(&(adxl345_init_info.platform_diver_addr->driver)))
	{
		GSE_ERR("adxl345_delete_attr fail: %d\n", err);
	}
	
	if(err = misc_deregister(&adxl345_device))
	{
		GSE_ERR("misc_deregister fail: %d\n", err);
	}

	if(err = hwmsen_detach(ID_ACCELEROMETER))
	    

	adxl345_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));
	return 0;
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static int adxl345_remove(void)
{
    struct acc_hw *hw = adxl345_get_cust_acc_hw();

    GSE_FUN();    
    ADXL345_power(hw, 0);    
    i2c_del_driver(&adxl345_i2c_driver);
    return 0;
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/

static int  adxl345_local_init(void)
{
   struct acc_hw *hw = adxl345_get_cust_acc_hw();
	GSE_FUN();

	ADXL345_power(hw, 1);
	adxl345_force[0] = hw->i2c_num;
	if(i2c_add_driver(&adxl345_i2c_driver))
	{
		GSE_ERR("add driver error\n");
		return -1;
	}
	if(-1 == adxl345_init_flag)
	{
	   return -1;
	}
	
	return 0;
}



static int __init adxl345_init(void)
{
	GSE_FUN();
	hwmsen_gsensor_add(&adxl345_init_info);

	return 0;    
}
/*----------------------------------------------------------------------------*/
static void __exit adxl345_exit(void)
{
	GSE_FUN();
	//platform_driver_unregister(&adxl345_gsensor_driver);
}
/*----------------------------------------------------------------------------*/
module_init(adxl345_init);
module_exit(adxl345_exit);
/*----------------------------------------------------------------------------*/
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ADXL345 I2C driver");
MODULE_AUTHOR("Chunlei.Wang@mediatek.com");
