/* drivers/i2c/chips/lis33de.c - LIS33DE motion sensor driver
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
#include "lis33de.h"
#include <linux/hwmsen_helper.h>

#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>


#define POWER_NONE_MACRO MT65XX_POWER_NONE



/*----------------------------------------------------------------------------*/
//#define I2C_DRIVERID_LIS33DE 345
/*----------------------------------------------------------------------------*/
#define DEBUG 1
/*----------------------------------------------------------------------------*/
#define CONFIG_LIS33DE_LOWPASS   /*apply low pass filter on output*/       
/*----------------------------------------------------------------------------*/
#define LIS33DE_AXIS_X          0
#define LIS33DE_AXIS_Y          1
#define LIS33DE_AXIS_Z          2
#define LIS33DE_AXES_NUM        3
#define LIS33DE_DATA_LEN        6
#define LIS33DE_DEV_NAME        "LIS33DE"
/*----------------------------------------------------------------------------*/
static const struct i2c_device_id lis33de_i2c_id[] = {{LIS33DE_DEV_NAME,0},{}};
/*the adapter id will be available in customization*/
static struct i2c_board_info __initdata i2c_lis33de={ I2C_BOARD_INFO("LIS33DE", LIS33DE_I2C_SLAVE_ADDR>>1)};

//static unsigned short lis33de_force[] = {0x00, LIS33DE_I2C_SLAVE_ADDR, I2C_CLIENT_END, I2C_CLIENT_END};
//static const unsigned short *const lis33de_forces[] = { lis33de_force, NULL };
//static struct i2c_client_address_data lis33de_addr_data = { .forces = lis33de_forces,};

/*----------------------------------------------------------------------------*/
static int lis33de_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id); 
static int lis33de_i2c_remove(struct i2c_client *client);
static int lis33de_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);
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
    s16 raw[C_MAX_FIR_LENGTH][LIS33DE_AXES_NUM];
    int sum[LIS33DE_AXES_NUM];
    int num;
    int idx;
};
/*----------------------------------------------------------------------------*/
struct lis33de_i2c_data {
    struct i2c_client *client;
    struct acc_hw *hw;
    struct hwmsen_convert   cvt;
    
    /*misc*/
    struct data_resolution *reso;
    atomic_t                trace;
    atomic_t                suspend;
    atomic_t                selftest;
	atomic_t				filter;
    s16                     cali_sw[LIS33DE_AXES_NUM+1];

    /*data*/
    s8                      offset[LIS33DE_AXES_NUM+1];  /*+1: for 4-byte alignment*/
    s16                     data[LIS33DE_AXES_NUM+1];

#if defined(CONFIG_LIS33DE_LOWPASS)
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
static struct i2c_driver lis33de_i2c_driver = {
    .driver = {
       // .owner          = THIS_MODULE,
        .name           = LIS33DE_DEV_NAME,
    },
	.probe      		= lis33de_i2c_probe,
	.remove    			= lis33de_i2c_remove,
	.detect				= lis33de_i2c_detect,
#if !defined(CONFIG_HAS_EARLYSUSPEND)    
    .suspend            = lis33de_suspend,
    .resume             = lis33de_resume,
#endif
	.id_table = lis33de_i2c_id,
	//.address_data = &lis33de_addr_data,
};

/*----------------------------------------------------------------------------*/
static struct i2c_client *lis33de_i2c_client = NULL;
static struct platform_driver lis33de_gsensor_driver;
static struct lis33de_i2c_data *obj_i2c_data = NULL;
static bool sensor_power = false;
static GSENSOR_VECTOR3D gsensor_gain, gsensor_offset;
static char selftestRes[10] = {0};



/*----------------------------------------------------------------------------*/
#define GSE_TAG                  "[Gsensor] "
#define GSE_FUN(f)               printk(KERN_INFO GSE_TAG"%s\n", __FUNCTION__)
#define GSE_ERR(fmt, args...)    printk(KERN_ERR GSE_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define GSE_LOG(fmt, args...)    printk(KERN_INFO GSE_TAG fmt, ##args)
/*----------------------------------------------------------------------------*/
static struct data_resolution lis33de_data_resolution[] = {
 /*8 combination by {FULL_RES,RANGE}*/
    {{ 18, 0}, 56},  //refer to datasheet {{ 15, 6}, 64} /*+/-2g  in 7-bit resolution:  15.6 mg/LSB  1g is devided into 64 block*/ 
    {{ 62, 5}, 16},   /*+/-8g  in 7-bit resolution:  62.5 mg/LSB 1g is devided into 16 block*/
};
/*----------------------------------------------------------------------------*/
static struct data_resolution lis33de_offset_resolution = {{15, 6}, 64};

/*
static int hwmsen_read_byte_sr(struct i2c_client *client, u8 addr, u8 *data)
{
   u8 buf;
    int ret = 0;
	
    client->addr = client->addr& I2C_MASK_FLAG | I2C_WR_FLAG |I2C_RS_FLAG;
    buf = addr;
	ret = i2c_master_send(client, (const char*)&buf, 1<<8 | 1);
    //ret = i2c_master_send(client, (const char*)&buf, 1);
    if (ret < 0) {
        GSE_ERR("send command error!!\n");
        return -EFAULT;
    }

    *data = buf;
	client->addr = client->addr& I2C_MASK_FLAG;
    return 0;
}
*/
static void dumpReg(struct i2c_client *client)
{
  int i=0;
  u8 addr = 0x20;
  u8 regdata=0;
  for(i=0; i<3 ; i++)
  {
    //dump all
    hwmsen_read_byte(client,addr,&regdata);
	GSE_LOG("Reg addr=%x regdata=%x\n",addr,regdata);
	addr++;
	
	
  }
}


/*--------------------ADXL power control function----------------------------------*/
static void LIS33DE_power(struct acc_hw *hw, unsigned int on) 
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
			if(!hwPowerOn(hw->power_id, hw->power_vol, "LIS33DE"))
			{
				GSE_ERR("power on fails!!\n");
			}
		}
		else	// power off
		{
			if (!hwPowerDown(hw->power_id, "LIS33DE"))
			{
				GSE_ERR("power off fail!!\n");
			}			  
		}
	}
	power_on = on;    
}
/*----------------------------------------------------------------------------*/
static int LIS33DE_SetDataResolution(struct lis33de_i2c_data *obj)
{
	int err;
	u8  dat, reso;

	if(err = hwmsen_read_byte(obj->client, LIS33DE_REG_CTL_REG1, &dat))
	{
		GSE_ERR("write data format fail!!\n");
		return err;
	}

	/*the data_reso is combined by 3 bits: {FULL_RES, DATA_RANGE}*/
	reso  = (dat & LIS33DE_RANGE_8G) ? (0x01) : (0x00);
	
	

	if(reso < sizeof(lis33de_data_resolution)/sizeof(lis33de_data_resolution[0]))
	{        
		obj->reso = &lis33de_data_resolution[reso];
		return 0;
	}
	else
	{
		return -EINVAL;
	}
}
/*----------------------------------------------------------------------------*/
static int LIS33DE_ReadData(struct i2c_client *client, s16 data[LIS33DE_AXES_NUM])
{
	struct lis33de_i2c_data *priv = i2c_get_clientdata(client);        
	u8 addr = LIS33DE_REG_DATAX0;
	u8 buf[LIS33DE_DATA_LEN] = {0};
	int err = 0;

	if(NULL == client)
	{
		err = -EINVAL;
	}
	
	else
	{
		if(hwmsen_read_block(client, LIS33DE_REG_OUT_X, buf, 0x01))
	    {
		   GSE_ERR("read X register err!\n");
		     return -1;
	    }
		
	    data[LIS33DE_AXIS_X] = (s8)buf[0];//(s16)(buf[0]|0x0000);
	    if(hwmsen_read_block(client, LIS33DE_REG_OUT_Y, buf, 0x01))
	    {
		   GSE_ERR("read X register err!\n");
		    return -1;
	     }
	     data[LIS33DE_AXIS_Y] = (s8)buf[0];//(s16)(buf[0]|0x0000);
	 
	    if(hwmsen_read_block(client, LIS33DE_REG_OUT_Z, buf, 0x01))
	    {
		  GSE_ERR("read X register err!\n");
		   return -1;
	  
	    }
	    data[LIS33DE_AXIS_Z] = (s8)buf[0];//(s16)(buf[0]|0x0000);

		if(atomic_read(&priv->trace) & ADX_TRC_RAWDATA)
		{
			GSE_LOG("[%08X %08X %08X] => [%5d %5d %5d]\n", data[LIS33DE_AXIS_X], data[LIS33DE_AXIS_Y], data[LIS33DE_AXIS_Z],
		                               data[LIS33DE_AXIS_X], data[LIS33DE_AXIS_Y], data[LIS33DE_AXIS_Z]);
		}
#ifdef CONFIG_LIS33DE_LOWPASS
		if(atomic_read(&priv->filter))
		{
			if(atomic_read(&priv->fir_en) && !atomic_read(&priv->suspend))
			{
				int idx, firlen = atomic_read(&priv->firlen);   
				if(priv->fir.num < firlen)
				{                
					priv->fir.raw[priv->fir.num][LIS33DE_AXIS_X] = data[LIS33DE_AXIS_X];
					priv->fir.raw[priv->fir.num][LIS33DE_AXIS_Y] = data[LIS33DE_AXIS_Y];
					priv->fir.raw[priv->fir.num][LIS33DE_AXIS_Z] = data[LIS33DE_AXIS_Z];
					priv->fir.sum[LIS33DE_AXIS_X] += data[LIS33DE_AXIS_X];
					priv->fir.sum[LIS33DE_AXIS_Y] += data[LIS33DE_AXIS_Y];
					priv->fir.sum[LIS33DE_AXIS_Z] += data[LIS33DE_AXIS_Z];
					if(atomic_read(&priv->trace) & ADX_TRC_FILTER)
					{
						GSE_LOG("add [%2d] [%5d %5d %5d] => [%5d %5d %5d]\n", priv->fir.num,
							priv->fir.raw[priv->fir.num][LIS33DE_AXIS_X], priv->fir.raw[priv->fir.num][LIS33DE_AXIS_Y], priv->fir.raw[priv->fir.num][LIS33DE_AXIS_Z],
							priv->fir.sum[LIS33DE_AXIS_X], priv->fir.sum[LIS33DE_AXIS_Y], priv->fir.sum[LIS33DE_AXIS_Z]);
					}
					priv->fir.num++;
					priv->fir.idx++;
				}
				else
				{
					idx = priv->fir.idx % firlen;
					priv->fir.sum[LIS33DE_AXIS_X] -= priv->fir.raw[idx][LIS33DE_AXIS_X];
					priv->fir.sum[LIS33DE_AXIS_Y] -= priv->fir.raw[idx][LIS33DE_AXIS_Y];
					priv->fir.sum[LIS33DE_AXIS_Z] -= priv->fir.raw[idx][LIS33DE_AXIS_Z];
					priv->fir.raw[idx][LIS33DE_AXIS_X] = data[LIS33DE_AXIS_X];
					priv->fir.raw[idx][LIS33DE_AXIS_Y] = data[LIS33DE_AXIS_Y];
					priv->fir.raw[idx][LIS33DE_AXIS_Z] = data[LIS33DE_AXIS_Z];
					priv->fir.sum[LIS33DE_AXIS_X] += data[LIS33DE_AXIS_X];
					priv->fir.sum[LIS33DE_AXIS_Y] += data[LIS33DE_AXIS_Y];
					priv->fir.sum[LIS33DE_AXIS_Z] += data[LIS33DE_AXIS_Z];
					priv->fir.idx++;
					data[LIS33DE_AXIS_X] = priv->fir.sum[LIS33DE_AXIS_X]/firlen;
					data[LIS33DE_AXIS_Y] = priv->fir.sum[LIS33DE_AXIS_Y]/firlen;
					data[LIS33DE_AXIS_Z] = priv->fir.sum[LIS33DE_AXIS_Z]/firlen;
					if(atomic_read(&priv->trace) & ADX_TRC_FILTER)
					{
						GSE_LOG("add [%2d] [%5d %5d %5d] => [%5d %5d %5d] : [%5d %5d %5d]\n", idx,
						priv->fir.raw[idx][LIS33DE_AXIS_X], priv->fir.raw[idx][LIS33DE_AXIS_Y], priv->fir.raw[idx][LIS33DE_AXIS_Z],
						priv->fir.sum[LIS33DE_AXIS_X], priv->fir.sum[LIS33DE_AXIS_Y], priv->fir.sum[LIS33DE_AXIS_Z],
						data[LIS33DE_AXIS_X], data[LIS33DE_AXIS_Y], data[LIS33DE_AXIS_Z]);
					}
				}
			}
		}	
#endif         
	}
	return err;
}
/*----------------------------------------------------------------------------*/
/*
static int LIS33DE_ReadOffset(struct i2c_client *client, s8 ofs[LIS33DE_AXES_NUM])
{    
	int err;

	return err;    
}
*/
/*----------------------------------------------------------------------------*/
static int LIS33DE_ResetCalibration(struct i2c_client *client)
{
	struct lis33de_i2c_data *obj = i2c_get_clientdata(client);	

	memset(obj->cali_sw, 0x00, sizeof(obj->cali_sw));
	return 0;     
}
/*----------------------------------------------------------------------------*/
static int LIS33DE_ReadCalibration(struct i2c_client *client, int dat[LIS33DE_AXES_NUM])
{
    struct lis33de_i2c_data *obj = i2c_get_clientdata(client);

    dat[obj->cvt.map[LIS33DE_AXIS_X]] = obj->cvt.sign[LIS33DE_AXIS_X]*obj->cali_sw[LIS33DE_AXIS_X];
    dat[obj->cvt.map[LIS33DE_AXIS_Y]] = obj->cvt.sign[LIS33DE_AXIS_Y]*obj->cali_sw[LIS33DE_AXIS_Y];
    dat[obj->cvt.map[LIS33DE_AXIS_Z]] = obj->cvt.sign[LIS33DE_AXIS_Z]*obj->cali_sw[LIS33DE_AXIS_Z];                        
                                       
    return 0;
}
/*----------------------------------------------------------------------------*/
/*
static int LIS33DE_ReadCalibrationEx(struct i2c_client *client, int act[LIS33DE_AXES_NUM], int raw[LIS33DE_AXES_NUM])
{  
	
	struct lis33de_i2c_data *obj = i2c_get_clientdata(client);
	int err;
	int mul;

	if(err = LIS33DE_ReadOffset(client, obj->offset))
	{
		GSE_ERR("read offset fail, %d\n", err);
		return err;
	}    

	mul = obj->reso->sensitivity/lis33de_offset_resolution.sensitivity;
	raw[LIS33DE_AXIS_X] = obj->offset[LIS33DE_AXIS_X]*mul + obj->cali_sw[LIS33DE_AXIS_X];
	raw[LIS33DE_AXIS_Y] = obj->offset[LIS33DE_AXIS_Y]*mul + obj->cali_sw[LIS33DE_AXIS_Y];
	raw[LIS33DE_AXIS_Z] = obj->offset[LIS33DE_AXIS_Z]*mul + obj->cali_sw[LIS33DE_AXIS_Z];

	act[obj->cvt.map[LIS33DE_AXIS_X]] = obj->cvt.sign[LIS33DE_AXIS_X]*raw[LIS33DE_AXIS_X];
	act[obj->cvt.map[LIS33DE_AXIS_Y]] = obj->cvt.sign[LIS33DE_AXIS_Y]*raw[LIS33DE_AXIS_Y];
	act[obj->cvt.map[LIS33DE_AXIS_Z]] = obj->cvt.sign[LIS33DE_AXIS_Z]*raw[LIS33DE_AXIS_Z];                        
	                       
	return 0;
}
*/
/*----------------------------------------------------------------------------*/
static int LIS33DE_WriteCalibration(struct i2c_client *client, int dat[LIS33DE_AXES_NUM])
{
	struct lis33de_i2c_data *obj = i2c_get_clientdata(client);
	int err = 0;
	int cali[LIS33DE_AXES_NUM];


	GSE_FUN();
	if(!obj || ! dat)
	{
		GSE_ERR("null ptr!!\n");
		return -EINVAL;
	}
	else
	{        
		s16 cali[LIS33DE_AXES_NUM];
		cali[obj->cvt.map[LIS33DE_AXIS_X]] = obj->cvt.sign[LIS33DE_AXIS_X]*obj->cali_sw[LIS33DE_AXIS_X];
		cali[obj->cvt.map[LIS33DE_AXIS_Y]] = obj->cvt.sign[LIS33DE_AXIS_Y]*obj->cali_sw[LIS33DE_AXIS_Y];
		cali[obj->cvt.map[LIS33DE_AXIS_Z]] = obj->cvt.sign[LIS33DE_AXIS_Z]*obj->cali_sw[LIS33DE_AXIS_Z]; 
		cali[LIS33DE_AXIS_X] += dat[LIS33DE_AXIS_X];
		cali[LIS33DE_AXIS_Y] += dat[LIS33DE_AXIS_Y];
		cali[LIS33DE_AXIS_Z] += dat[LIS33DE_AXIS_Z];

		obj->cali_sw[LIS33DE_AXIS_X] += obj->cvt.sign[LIS33DE_AXIS_X]*dat[obj->cvt.map[LIS33DE_AXIS_X]];
        obj->cali_sw[LIS33DE_AXIS_Y] += obj->cvt.sign[LIS33DE_AXIS_Y]*dat[obj->cvt.map[LIS33DE_AXIS_Y]];
        obj->cali_sw[LIS33DE_AXIS_Z] += obj->cvt.sign[LIS33DE_AXIS_Z]*dat[obj->cvt.map[LIS33DE_AXIS_Z]];
	} 

	return err;
}
/*----------------------------------------------------------------------------*/
static int LIS33DE_CheckDeviceID(struct i2c_client *client)
{
	u8 databuf[10];    
	int res = 0;
/*
	memset(databuf, 0, sizeof(u8)*10);    
	databuf[0] = LIS33DE_REG_DEVID;    

	res = i2c_master_send(client, databuf, 0x1);
	if(res <= 0)
	{
		goto exit_LIS33DE_CheckDeviceID;
	}
	
	udelay(500);

	databuf[0] = 0x0;        
	res = i2c_master_recv(client, databuf, 0x01);
	if(res <= 0)
	{
		goto exit_LIS33DE_CheckDeviceID;
	}
	

	if(databuf[0]!=LIS33DE_FIXED_DEVID)
	{
		return LIS33DE_ERR_IDENTIFICATION;
	}

	exit_LIS33DE_CheckDeviceID:
	if (res <= 0)
	{
		return LIS33DE_ERR_I2C;
	}
	*/
	return LIS33DE_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int LIS33DE_SetPowerMode(struct i2c_client *client, bool enable)
{
	u8 databuf[2];    
	int res = 0;
	u8 addr = LIS33DE_REG_CTL_REG1;
	struct lis33de_i2c_data *obj = i2c_get_clientdata(client);
	
	
	if(enable == sensor_power)
	{
		GSE_LOG("Sensor power status is newest!\n");
		return LIS33DE_SUCCESS;
	}

	if(hwmsen_read_byte(client, addr, &databuf[0]))
	{
		GSE_ERR("read power ctl register err!\n");
		return LIS33DE_ERR_I2C;
	}

	databuf[0] &= ~LIS33DE_MEASURE_MODE;
	
	if(enable == TRUE)
	{
		databuf[0] |= LIS33DE_MEASURE_MODE;
	}
	else
	{
		// do nothing
	}
	databuf[1] = databuf[0];
	databuf[0] = LIS33DE_REG_CTL_REG1;
	

	res = i2c_master_send(client, databuf, 0x2);

	if(res <= 0)
	{
		GSE_LOG("set power mode failed!\n");
		return LIS33DE_ERR_I2C;
	}
	else if(atomic_read(&obj->trace) & ADX_TRC_INFO)
	{
		GSE_LOG("set power mode ok %d!\n", databuf[1]);
	}

	sensor_power = enable;
	
	return LIS33DE_SUCCESS;    
}
/*----------------------------------------------------------------------------*/
static int LIS33DE_SetDataFormat(struct i2c_client *client, u8 dataformat)
{
	struct lis33de_i2c_data *obj = i2c_get_clientdata(client);
	u8 databuf[10];
	u8 addr = LIS33DE_REG_CTL_REG1;
	int res = 0;

	memset(databuf, 0, sizeof(u8)*10);

	if(hwmsen_read_byte(client, addr, &databuf[0]))
	{
		GSE_ERR("read reg_ctl_reg1 register err!\n");
		return LIS33DE_ERR_I2C;
	}

	databuf[0] &= ~LIS33DE_RANGE_8G;
	
	if(LIS33DE_RANGE_8G == dataformat)
	{
		databuf[0] |= LIS33DE_RANGE_8G;
	}
	else
	{
	    GSE_LOG("set 2g range\n");
		// do nothing
	}
	databuf[1] = databuf[0];
	databuf[0] = LIS33DE_REG_CTL_REG1;
	

	res = i2c_master_send(client, databuf, 0x2);

	if(res <= 0)
	{
		return LIS33DE_ERR_I2C;
	}
	

	return LIS33DE_SetDataResolution(obj);    
}
/*----------------------------------------------------------------------------*/
static int LIS33DE_SetBWRate(struct i2c_client *client, u8 bwrate)
{
	u8 databuf[10];
	u8 addr = LIS33DE_REG_CTL_REG1;
	int res = 0;

	memset(databuf, 0, sizeof(u8)*10);
	
	if(hwmsen_read_byte(client, addr, &databuf[0]))
	{
		GSE_ERR("read reg_ctl_reg1 register err!\n");
		return LIS33DE_ERR_I2C;
	}

	databuf[0] &= ~LIS33DE_BW_400HZ;
	
	if(LIS33DE_BW_400HZ == bwrate)
	{
		databuf[0] |= LIS33DE_BW_400HZ;
	}
	else
	{
	     GSE_LOG("set DataRate 100Hz\n");
		// do nothing
	}
	databuf[1] = databuf[0];
	databuf[0] = LIS33DE_REG_CTL_REG1;

	res = i2c_master_send(client, databuf, 0x2);

	if(res <= 0)
	{
		return LIS33DE_ERR_I2C;
	}
	
	return LIS33DE_SUCCESS;    
}
/*----------------------------------------------------------------------------*/
//enalbe data ready interrupt
static int LIS33DE_SetIntEnable(struct i2c_client *client, u8 intenable)
{
	u8 databuf[10];
	u8 addr = LIS33DE_REG_CTL_REG1;
	int res = 0;

	memset(databuf, 0, sizeof(u8)*10); 

	if(hwmsen_read_byte(client, addr, &databuf[0]))
	{
		GSE_ERR("read reg_ctl_reg1 register err!\n");
		return LIS33DE_ERR_I2C;
	}

	databuf[0] &= ~LIS33DE_DATA_READY;
	
	if(LIS33DE_DATA_READY == intenable)
	{
		databuf[0] |= LIS33DE_DATA_READY;
	}
	else
	{
	     GSE_LOG("Disable dataready INT\n");
		// do nothing
	}
	databuf[1] = databuf[0];
	databuf[0] = LIS33DE_REG_CTL_REG1;
	
	res = i2c_master_send(client, databuf, 0x2);

	if(res <= 0)
	{
		return LIS33DE_ERR_I2C;
	}
	
	return LIS33DE_SUCCESS;    
}
/*----------------------------------------------------------------------------*/
static int LIS33DE_Init(struct i2c_client *client, int reset_cali)
{
	struct lis33de_i2c_data *obj = i2c_get_clientdata(client);
	int res = 0;
/*
	res = LIS33DE_CheckDeviceID(client); 
	if(res != LIS33DE_SUCCESS)
	{
		return res;
	}	
*/
    // first clear reg1
    res = hwmsen_write_byte(client,LIS33DE_REG_CTL_REG1,0x00);
	if(res != LIS33DE_SUCCESS)
	{
		return res;
	}


	res = LIS33DE_SetPowerMode(client, false);
	if(res != LIS33DE_SUCCESS)
	{
		return res;
	}
	

	res = LIS33DE_SetBWRate(client, ~LIS33DE_BW_400HZ);//400 or 100 no other choice
	if(res != LIS33DE_SUCCESS )
	{
		return res;
	}

	res = LIS33DE_SetDataFormat(client, ~LIS33DE_RANGE_8G);//8g or 2G no oher choise
	if(res != LIS33DE_SUCCESS) 
	{
		return res;
	}
	gsensor_gain.x = gsensor_gain.y = gsensor_gain.z = obj->reso->sensitivity;

	res = LIS33DE_SetIntEnable(client, ~LIS33DE_DATA_READY);        
	if(res != LIS33DE_SUCCESS)
	{
		return res;
	}
	

	if(NULL != reset_cali)
	{ 
		//reset calibration only in power on
		res = LIS33DE_ResetCalibration(client);
		if(res != LIS33DE_SUCCESS)
		{
			return res;
		}
	}

#ifdef CONFIG_LIS33DE_LOWPASS
	memset(&obj->fir, 0x00, sizeof(obj->fir));  
#endif

	return LIS33DE_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int LIS33DE_ReadChipInfo(struct i2c_client *client, char *buf, int bufsize)
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

	sprintf(buf, "LIS33DE Chip");
	return 0;
}
/*----------------------------------------------------------------------------*/
static int LIS33DE_ReadSensorData(struct i2c_client *client, char *buf, int bufsize)
{
	struct lis33de_i2c_data *obj = (struct lis33de_i2c_data*)i2c_get_clientdata(client);
	u8 databuf[20];
	int acc[LIS33DE_AXES_NUM];
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
		res = LIS33DE_SetPowerMode(client, true);
		if(res)
		{
			GSE_ERR("Power on lis33de error %d!\n", res);
		}
		msleep(20);
	}

	if(res = LIS33DE_ReadData(client, obj->data))
	{        
		GSE_ERR("I2C error: ret value=%d", res);
		return -3;
	}
	else
	{
		obj->data[LIS33DE_AXIS_X] += obj->cali_sw[LIS33DE_AXIS_X];
		obj->data[LIS33DE_AXIS_Y] += obj->cali_sw[LIS33DE_AXIS_Y];
		obj->data[LIS33DE_AXIS_Z] += obj->cali_sw[LIS33DE_AXIS_Z];
		
		/*remap coordinate*/
		acc[obj->cvt.map[LIS33DE_AXIS_X]] = obj->cvt.sign[LIS33DE_AXIS_X]*obj->data[LIS33DE_AXIS_X];
		acc[obj->cvt.map[LIS33DE_AXIS_Y]] = obj->cvt.sign[LIS33DE_AXIS_Y]*obj->data[LIS33DE_AXIS_Y];
		acc[obj->cvt.map[LIS33DE_AXIS_Z]] = obj->cvt.sign[LIS33DE_AXIS_Z]*obj->data[LIS33DE_AXIS_Z];

		//GSE_LOG("Mapped gsensor data: %d, %d, %d!\n", acc[LIS33DE_AXIS_X], acc[LIS33DE_AXIS_Y], acc[LIS33DE_AXIS_Z]);

		//Out put the mg
		acc[LIS33DE_AXIS_X] = acc[LIS33DE_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		acc[LIS33DE_AXIS_Y] = acc[LIS33DE_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		acc[LIS33DE_AXIS_Z] = acc[LIS33DE_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;		
		

		sprintf(buf, "%04x %04x %04x", acc[LIS33DE_AXIS_X], acc[LIS33DE_AXIS_Y], acc[LIS33DE_AXIS_Z]);
		if(atomic_read(&obj->trace) & ADX_TRC_IOCTL)//atomic_read(&obj->trace) & ADX_TRC_IOCTL
		{
			GSE_LOG("gsensor data: %s!\n", buf);
			dumpReg(client);
		}
	}
	
	return 0;
}
/*----------------------------------------------------------------------------*/
static int LIS33DE_ReadRawData(struct i2c_client *client, char *buf)
{
	struct lis33de_i2c_data *obj = (struct lis33de_i2c_data*)i2c_get_clientdata(client);
	int res = 0;

	if (!buf || !client)
	{
		return EINVAL;
	}
	
	if(res = LIS33DE_ReadData(client, obj->data))
	{        
		GSE_ERR("I2C error: ret value=%d", res);
		return EIO;
	}
	else
	{
		sprintf(buf, "%04x %04x %04x", obj->data[LIS33DE_AXIS_X], 
			obj->data[LIS33DE_AXIS_Y], obj->data[LIS33DE_AXIS_Z]);
	
	}
	
	return 0;
}
/*----------------------------------------------------------------------------*/
static int LIS33DE_InitSelfTest(struct i2c_client *client)
{
	int res = 0;
	u8  data;

	res = LIS33DE_SetBWRate(client, LIS33DE_BW_400HZ);
	if(res != LIS33DE_SUCCESS ) 
	{
		return res;
	}
	
	res = hwmsen_read_byte(client, LIS33DE_REG_CTL_REG1, &data);
	if(res != LIS33DE_SUCCESS)
	{
		return res;
	}
	

	res = LIS33DE_SetDataFormat(client, LIS33DE_SELF_TEST|data);
	if(res != LIS33DE_SUCCESS) 
	{
		return res;
	}
	
	res = LIS33DE_SetPowerMode(client,true);
	if(res != LIS33DE_SUCCESS) 
	{
		return res;
	}
	
	return LIS33DE_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int LIS33DE_JudgeTestResult(struct i2c_client *client, s32 prv[LIS33DE_AXES_NUM], s32 nxt[LIS33DE_AXES_NUM])
{
    struct criteria {
        int min;
        int max;
    };
	
    struct criteria self[4][3] = {
        {{-35, 5}, {-35, 5}, {-40, -3}},//test range 8g
        {{-35, 5}, {-35, 5}, {-40, -3}},//test range 2g
                   
    };
    struct criteria (*ptr)[3] = NULL;
    u8 format;
    int res;
    if(res = hwmsen_read_byte(client, LIS33DE_REG_CTL_REG1, &format))
        return res;
    if(format & LIS33DE_RANGE_8G)
        ptr = &self[0];
    else 
        ptr = &self[1];
    

    if (!ptr) {
        GSE_ERR("null pointer\n");
        return -EINVAL;
    }

    if (((nxt[LIS33DE_AXIS_X] - prv[LIS33DE_AXIS_X]) > (*ptr)[LIS33DE_AXIS_X].max) ||
        ((nxt[LIS33DE_AXIS_X] - prv[LIS33DE_AXIS_X]) < (*ptr)[LIS33DE_AXIS_X].min)) {
        GSE_ERR("X is over range\n");
        res = -EINVAL;
    }
    if (((nxt[LIS33DE_AXIS_Y] - prv[LIS33DE_AXIS_Y]) > (*ptr)[LIS33DE_AXIS_Y].max) ||
        ((nxt[LIS33DE_AXIS_Y] - prv[LIS33DE_AXIS_Y]) < (*ptr)[LIS33DE_AXIS_Y].min)) {
        GSE_ERR("Y is over range\n");
        res = -EINVAL;
    }
    if (((nxt[LIS33DE_AXIS_Z] - prv[LIS33DE_AXIS_Z]) > (*ptr)[LIS33DE_AXIS_Z].max) ||
        ((nxt[LIS33DE_AXIS_Z] - prv[LIS33DE_AXIS_Z]) < (*ptr)[LIS33DE_AXIS_Z].min)) {
        GSE_ERR("Z is over range\n");
        res = -EINVAL;
    }
    return res;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = lis33de_i2c_client;
	char strbuf[LIS33DE_BUFSIZE];
	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	
	LIS33DE_ReadChipInfo(client, strbuf, LIS33DE_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);        
}
/*----------------------------------------------------------------------------*/
static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = lis33de_i2c_client;
	char strbuf[LIS33DE_BUFSIZE];
	
	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	LIS33DE_ReadSensorData(client, strbuf, LIS33DE_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);            
}
/*----------------------------------------------------------------------------*/
static ssize_t show_cali_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = lis33de_i2c_client;
	struct lis33de_i2c_data *obj;

	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}

	obj = i2c_get_clientdata(client);

	int err, len = 0, mul;
	int tmp[LIS33DE_AXES_NUM];

	
	if(err = LIS33DE_ReadCalibration(client, tmp))
	{
		return -EINVAL;
	}
	else
	{    
		mul = obj->reso->sensitivity/lis33de_offset_resolution.sensitivity;
		len += snprintf(buf+len, PAGE_SIZE-len, "[HW ][%d] (%+3d, %+3d, %+3d) : (0x%02X, 0x%02X, 0x%02X)\n", mul,                        
			obj->offset[LIS33DE_AXIS_X], obj->offset[LIS33DE_AXIS_Y], obj->offset[LIS33DE_AXIS_Z],
			obj->offset[LIS33DE_AXIS_X], obj->offset[LIS33DE_AXIS_Y], obj->offset[LIS33DE_AXIS_Z]);
		len += snprintf(buf+len, PAGE_SIZE-len, "[SW ][%d] (%+3d, %+3d, %+3d)\n", 1, 
			obj->cali_sw[LIS33DE_AXIS_X], obj->cali_sw[LIS33DE_AXIS_Y], obj->cali_sw[LIS33DE_AXIS_Z]);

		len += snprintf(buf+len, PAGE_SIZE-len, "[ALL]    (%+3d, %+3d, %+3d) : (%+3d, %+3d, %+3d)\n", 
			obj->offset[LIS33DE_AXIS_X]*mul + obj->cali_sw[LIS33DE_AXIS_X],
			obj->offset[LIS33DE_AXIS_Y]*mul + obj->cali_sw[LIS33DE_AXIS_Y],
			obj->offset[LIS33DE_AXIS_Z]*mul + obj->cali_sw[LIS33DE_AXIS_Z],
			tmp[LIS33DE_AXIS_X], tmp[LIS33DE_AXIS_Y], tmp[LIS33DE_AXIS_Z]);
		
		return len;
    }
}
/*----------------------------------------------------------------------------*/
static ssize_t store_cali_value(struct device_driver *ddri, char *buf, size_t count)
{
	struct i2c_client *client = lis33de_i2c_client;  
	int err, x, y, z;
	int dat[LIS33DE_AXES_NUM];

	if(!strncmp(buf, "rst", 3))
	{
		if(err = LIS33DE_ResetCalibration(client))
		{
			GSE_ERR("reset offset err = %d\n", err);
		}	
	}
	else if(3 == sscanf(buf, "0x%02X 0x%02X 0x%02X", &x, &y, &z))
	{
		dat[LIS33DE_AXIS_X] = x;
		dat[LIS33DE_AXIS_Y] = y;
		dat[LIS33DE_AXIS_Z] = z;
		if(err = LIS33DE_WriteCalibration(client, dat))
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

static ssize_t show_power_status(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = lis33de_i2c_client;
	struct lis33de_i2c_data *obj;
	u8 data;

	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}

	obj = i2c_get_clientdata(client);
	hwmsen_read_byte(client,LIS33DE_REG_CTL_REG1,&data);

    return snprintf(buf, PAGE_SIZE, "%x\n", data);
}

/*----------------------------------------------------------------------------*/
/*
static ssize_t store_self_value(struct device_driver *ddri, char *buf, size_t count)
{   //write anything to this register will trigger the process
	struct item{
	s16 raw[LIS33DE_AXES_NUM];
	};
	
	struct i2c_client *client = lis33de_i2c_client;  
	int idx, res, num;
	struct item *prv = NULL, *nxt = NULL;
	s32 avg_prv[LIS33DE_AXES_NUM] = {0, 0, 0};
	s32 avg_nxt[LIS33DE_AXES_NUM] = {0, 0, 0};


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
	for(idx = 0; idx < num; idx++)
	{
		if(res = LIS33DE_ReadData(client, prv[idx].raw))
		{            
			GSE_ERR("read data fail: %d\n", res);
			goto exit;
		}
		
		avg_prv[LIS33DE_AXIS_X] += prv[idx].raw[LIS33DE_AXIS_X];
		avg_prv[LIS33DE_AXIS_Y] += prv[idx].raw[LIS33DE_AXIS_Y];
		avg_prv[LIS33DE_AXIS_Z] += prv[idx].raw[LIS33DE_AXIS_Z];        
		GSE_LOG("[%5d %5d %5d]\n", prv[idx].raw[LIS33DE_AXIS_X], prv[idx].raw[LIS33DE_AXIS_Y], prv[idx].raw[LIS33DE_AXIS_Z]);
	}
	
	avg_prv[LIS33DE_AXIS_X] /= num;
	avg_prv[LIS33DE_AXIS_Y] /= num;
	avg_prv[LIS33DE_AXIS_Z] /= num;    

	
	LIS33DE_InitSelfTest(client);
	GSE_LOG("SELFTEST:\n");    
	for(idx = 0; idx < num; idx++)
	{
		if(res = LIS33DE_ReadData(client, nxt[idx].raw))
		{            
			GSE_ERR("read data fail: %d\n", res);
			goto exit;
		}
		avg_nxt[LIS33DE_AXIS_X] += nxt[idx].raw[LIS33DE_AXIS_X];
		avg_nxt[LIS33DE_AXIS_Y] += nxt[idx].raw[LIS33DE_AXIS_Y];
		avg_nxt[LIS33DE_AXIS_Z] += nxt[idx].raw[LIS33DE_AXIS_Z];        
		GSE_LOG("[%5d %5d %5d]\n", nxt[idx].raw[LIS33DE_AXIS_X], nxt[idx].raw[LIS33DE_AXIS_Y], nxt[idx].raw[LIS33DE_AXIS_Z]);
	}
	
	avg_nxt[LIS33DE_AXIS_X] /= num;
	avg_nxt[LIS33DE_AXIS_Y] /= num;
	avg_nxt[LIS33DE_AXIS_Z] /= num;    

	GSE_LOG("X: %5d - %5d = %5d \n", avg_nxt[LIS33DE_AXIS_X], avg_prv[LIS33DE_AXIS_X], avg_nxt[LIS33DE_AXIS_X] - avg_prv[LIS33DE_AXIS_X]);
	GSE_LOG("Y: %5d - %5d = %5d \n", avg_nxt[LIS33DE_AXIS_Y], avg_prv[LIS33DE_AXIS_Y], avg_nxt[LIS33DE_AXIS_Y] - avg_prv[LIS33DE_AXIS_Y]);
	GSE_LOG("Z: %5d - %5d = %5d \n", avg_nxt[LIS33DE_AXIS_Z], avg_prv[LIS33DE_AXIS_Z], avg_nxt[LIS33DE_AXIS_Z] - avg_prv[LIS33DE_AXIS_Z]); 

	if(!LIS33DE_JudgeTestResult(client, avg_prv, avg_nxt))
	{
		GSE_LOG("SELFTEST : PASS\n");
	}	
	else
	{
		GSE_LOG("SELFTEST : FAIL\n");
	}
	
	exit:
	  
	LIS33DE_Init(client, 0);
	kfree(prv);
	kfree(nxt);
	return count;
}
*/
/*----------------------------------------------------------------------------*/
static ssize_t show_selftest_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = lis33de_i2c_client;
	struct lis33de_i2c_data *obj;
	int result =0;
	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	GSE_LOG("fwq  selftestRes value =%s\n",selftestRes); 
	return snprintf(buf, 10, "%s\n", selftestRes);
}

/*----------------------------------------------------------------------------*/
static ssize_t store_selftest_value(struct device_driver *ddri, char *buf, size_t count)
{
	struct item{
	s16 raw[LIS33DE_AXES_NUM];
	};
	
	struct i2c_client *client = lis33de_i2c_client;  
	int idx, res, num;
	struct item *prv = NULL, *nxt = NULL;
	s32 avg_prv[LIS33DE_AXES_NUM] = {0, 0, 0};
	s32 avg_nxt[LIS33DE_AXES_NUM] = {0, 0, 0};


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


    res = LIS33DE_SetPowerMode(client,true);
	if(res != LIS33DE_SUCCESS ) //
	{
		return res;
	}
	msleep(20);
	
	GSE_LOG("NORMAL:\n");
	for(idx = 0; idx < num; idx++)
	{
		if(res = LIS33DE_ReadData(client, prv[idx].raw))
		{            
			GSE_ERR("read data fail: %d\n", res);
			goto exit;
		}
		
		avg_prv[LIS33DE_AXIS_X] += prv[idx].raw[LIS33DE_AXIS_X];
		avg_prv[LIS33DE_AXIS_Y] += prv[idx].raw[LIS33DE_AXIS_Y];
		avg_prv[LIS33DE_AXIS_Z] += prv[idx].raw[LIS33DE_AXIS_Z];        
		GSE_LOG("[%5d %5d %5d]\n", prv[idx].raw[LIS33DE_AXIS_X], prv[idx].raw[LIS33DE_AXIS_Y], prv[idx].raw[LIS33DE_AXIS_Z]);
	}
	
	avg_prv[LIS33DE_AXIS_X] /= num;
	avg_prv[LIS33DE_AXIS_Y] /= num;
	avg_prv[LIS33DE_AXIS_Z] /= num;  

	res = LIS33DE_SetPowerMode(client,false);
	if(res != LIS33DE_SUCCESS ) //
	{
		return res;
	}

	/*initial setting for self test*/
	LIS33DE_InitSelfTest(client);
	msleep(50);
	GSE_LOG("SELFTEST:\n");    
	for(idx = 0; idx < num; idx++)
	{
		if(res = LIS33DE_ReadData(client, nxt[idx].raw))
		{            
			GSE_ERR("read data fail: %d\n", res);
			goto exit;
		}
		avg_nxt[LIS33DE_AXIS_X] += nxt[idx].raw[LIS33DE_AXIS_X];
		avg_nxt[LIS33DE_AXIS_Y] += nxt[idx].raw[LIS33DE_AXIS_Y];
		avg_nxt[LIS33DE_AXIS_Z] += nxt[idx].raw[LIS33DE_AXIS_Z];        
		GSE_LOG("[%5d %5d %5d]\n", nxt[idx].raw[LIS33DE_AXIS_X], nxt[idx].raw[LIS33DE_AXIS_Y], nxt[idx].raw[LIS33DE_AXIS_Z]);
	}
	
	avg_nxt[LIS33DE_AXIS_X] /= num;
	avg_nxt[LIS33DE_AXIS_Y] /= num;
	avg_nxt[LIS33DE_AXIS_Z] /= num;    

	GSE_LOG("X: %5d - %5d = %5d \n", avg_nxt[LIS33DE_AXIS_X], avg_prv[LIS33DE_AXIS_X], avg_nxt[LIS33DE_AXIS_X] - avg_prv[LIS33DE_AXIS_X]);
	GSE_LOG("Y: %5d - %5d = %5d \n", avg_nxt[LIS33DE_AXIS_Y], avg_prv[LIS33DE_AXIS_Y], avg_nxt[LIS33DE_AXIS_Y] - avg_prv[LIS33DE_AXIS_Y]);
	GSE_LOG("Z: %5d - %5d = %5d \n", avg_nxt[LIS33DE_AXIS_Z], avg_prv[LIS33DE_AXIS_Z], avg_nxt[LIS33DE_AXIS_Z] - avg_prv[LIS33DE_AXIS_Z]); 

	if(!LIS33DE_JudgeTestResult(client, avg_prv, avg_nxt))
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
	LIS33DE_Init(client, 0);
	kfree(prv);
	kfree(nxt);
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_firlen_value(struct device_driver *ddri, char *buf)
{
#ifdef CONFIG_LIS33DE_LOWPASS
	struct i2c_client *client = lis33de_i2c_client;
	struct lis33de_i2c_data *obj = i2c_get_clientdata(client);
	if(atomic_read(&obj->firlen))
	{
		int idx, len = atomic_read(&obj->firlen);
		GSE_LOG("len = %2d, idx = %2d\n", obj->fir.num, obj->fir.idx);

		for(idx = 0; idx < len; idx++)
		{
			GSE_LOG("[%5d %5d %5d]\n", obj->fir.raw[idx][LIS33DE_AXIS_X], obj->fir.raw[idx][LIS33DE_AXIS_Y], obj->fir.raw[idx][LIS33DE_AXIS_Z]);
		}
		
		GSE_LOG("sum = [%5d %5d %5d]\n", obj->fir.sum[LIS33DE_AXIS_X], obj->fir.sum[LIS33DE_AXIS_Y], obj->fir.sum[LIS33DE_AXIS_Z]);
		GSE_LOG("avg = [%5d %5d %5d]\n", obj->fir.sum[LIS33DE_AXIS_X]/len, obj->fir.sum[LIS33DE_AXIS_Y]/len, obj->fir.sum[LIS33DE_AXIS_Z]/len);
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&obj->firlen));
#else
	return snprintf(buf, PAGE_SIZE, "not support\n");
#endif
}
/*----------------------------------------------------------------------------*/
static ssize_t store_firlen_value(struct device_driver *ddri, char *buf, size_t count)
{
#ifdef CONFIG_LIS33DE_LOWPASS
	struct i2c_client *client = lis33de_i2c_client;  
	struct lis33de_i2c_data *obj = i2c_get_clientdata(client);
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
	struct lis33de_i2c_data *obj = obj_i2c_data;
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
	struct lis33de_i2c_data *obj = obj_i2c_data;
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
	struct lis33de_i2c_data *obj = obj_i2c_data;
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
static DRIVER_ATTR(power,                S_IRUGO, show_power_status,          NULL);
static DRIVER_ATTR(selftest,   S_IWUSR | S_IRUGO, show_selftest_value,      store_selftest_value);
static DRIVER_ATTR(firlen,     S_IWUSR | S_IRUGO, show_firlen_value,        store_firlen_value);
static DRIVER_ATTR(trace,      S_IWUSR | S_IRUGO, show_trace_value,         store_trace_value);
static DRIVER_ATTR(status,               S_IRUGO, show_status_value,        NULL);
/*----------------------------------------------------------------------------*/
static struct driver_attribute *lis33de_attr_list[] = {
	&driver_attr_chipinfo,     /*chip information*/
	&driver_attr_sensordata,   /*dump sensor data*/
	&driver_attr_cali,         /*show calibration data*/
	&driver_attr_power,         /*show power reg*/
	&driver_attr_selftest,     /*self control: 0: disable, 1: enable*/
	&driver_attr_firlen,       /*filter length: 0: disable, others: enable*/
	&driver_attr_trace,        /*trace log*/
	&driver_attr_status,        
};
/*----------------------------------------------------------------------------*/
static int lis33de_create_attr(struct device_driver *driver) 
{
	int idx, err = 0;
	int num = (int)(sizeof(lis33de_attr_list)/sizeof(lis33de_attr_list[0]));
	if (driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
		if(err = driver_create_file(driver, lis33de_attr_list[idx]))
		{            
			GSE_ERR("driver_create_file (%s) = %d\n", lis33de_attr_list[idx]->attr.name, err);
			break;
		}
	}    
	return err;
}
/*----------------------------------------------------------------------------*/
static int lis33de_delete_attr(struct device_driver *driver)
{
	int idx ,err = 0;
	int num = (int)(sizeof(lis33de_attr_list)/sizeof(lis33de_attr_list[0]));

	if(driver == NULL)
	{
		return -EINVAL;
	}
	

	for(idx = 0; idx < num; idx++)
	{
		driver_remove_file(driver, lis33de_attr_list[idx]);
	}
	

	return err;
}

/*----------------------------------------------------------------------------*/
int gsensor_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value, sample_delay;	
	struct lis33de_i2c_data *priv = (struct lis33de_i2c_data*)self;
	hwm_sensor_data* gsensor_data;
	char buff[LIS33DE_BUFSIZE];
	
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
					sample_delay = LIS33DE_BW_400HZ;
				}
				else if(value <= 10)
				{
					sample_delay = ~LIS33DE_BW_400HZ;
				}
				else
				{
					sample_delay = ~LIS33DE_BW_400HZ;
				}
				
				err = LIS33DE_SetBWRate(priv->client, sample_delay);
				if(err != LIS33DE_SUCCESS ) //0x2C->BW=100Hz
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
					priv->fir.sum[LIS33DE_AXIS_X] = 0;
					priv->fir.sum[LIS33DE_AXIS_Y] = 0;
					priv->fir.sum[LIS33DE_AXIS_Z] = 0;
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
				GSE_LOG("enable value=%d, sensor_power =%d\n",value,sensor_power);
				if(((value == 0) && (sensor_power == false)) ||((value == 1) && (sensor_power == true)))
				{
					GSE_LOG("Gsensor device have updated!\n");
				}
				else
				{
					err = LIS33DE_SetPowerMode( priv->client, !sensor_power);
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
				LIS33DE_ReadSensorData(priv->client, buff, LIS33DE_BUFSIZE);
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
static int lis33de_open(struct inode *inode, struct file *file)
{
	file->private_data = lis33de_i2c_client;

	if(file->private_data == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return -EINVAL;
	}
	return nonseekable_open(inode, file);
}
/*----------------------------------------------------------------------------*/
static int lis33de_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}
/*----------------------------------------------------------------------------*/
//static int lis33de_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
      // unsigned long arg)
static int lis33de_unlocked_ioctl(struct file *file, unsigned int cmd,
       unsigned long arg)       
{
	struct i2c_client *client = (struct i2c_client*)file->private_data;
	struct lis33de_i2c_data *obj = (struct lis33de_i2c_data*)i2c_get_clientdata(client);	
	char strbuf[LIS33DE_BUFSIZE];
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
			LIS33DE_Init(client, 0);			
			break;

		case GSENSOR_IOCTL_READ_CHIPINFO:
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			
			LIS33DE_ReadChipInfo(client, strbuf, LIS33DE_BUFSIZE);
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
			
			LIS33DE_ReadSensorData(client, strbuf, LIS33DE_BUFSIZE);
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

		case GSENSOR_IOCTL_READ_OFFSET:
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			
			if(copy_to_user(data, &gsensor_offset, sizeof(GSENSOR_VECTOR3D)))
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
			LIS33DE_ReadRawData(client, &strbuf);
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
				cali[LIS33DE_AXIS_X] = sensor_data.x * obj->reso->sensitivity / GRAVITY_EARTH_1000;
				cali[LIS33DE_AXIS_Y] = sensor_data.y * obj->reso->sensitivity / GRAVITY_EARTH_1000;
				cali[LIS33DE_AXIS_Z] = sensor_data.z * obj->reso->sensitivity / GRAVITY_EARTH_1000;			  
				err = LIS33DE_WriteCalibration(client, cali);			 
			}
			break;

		case GSENSOR_IOCTL_CLR_CALI:
			err = LIS33DE_ResetCalibration(client);
			break;

		case GSENSOR_IOCTL_GET_CALI:
			data = (void __user*)arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			if(err = LIS33DE_ReadCalibration(client, cali))
			{
				break;
			}
			
			sensor_data.x = cali[LIS33DE_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
			sensor_data.y = cali[LIS33DE_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
			sensor_data.z = cali[LIS33DE_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
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
static struct file_operations lis33de_fops = {
	//.owner = THIS_MODULE,
	.open = lis33de_open,
	.release = lis33de_release,
	.unlocked_ioctl = lis33de_unlocked_ioctl,
};
/*----------------------------------------------------------------------------*/
static struct miscdevice lis33de_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "gsensor",
	.fops = &lis33de_fops,
};
/*----------------------------------------------------------------------------*/
#ifndef CONFIG_HAS_EARLYSUSPEND
/*----------------------------------------------------------------------------*/
static int lis33de_suspend(struct i2c_client *client, pm_message_t msg) 
{
	struct lis33de_i2c_data *obj = i2c_get_clientdata(client);    
	int err = 0;
	u8 dat;
	GSE_FUN();    

	if(msg.event == PM_EVENT_SUSPEND)
	{   
		if(obj == NULL)
		{
			GSE_ERR("null pointer!!\n");
			return -EINVAL;
		}
		//read old data
		if ((err = hwmsen_read_byte(client, LIS33DE_REG_CTL_REG1, &dat))) 
		{
           GSE_ERR("write data format fail!!\n");
           return err;
        }
		dat = dat&0b10111111;
		atomic_set(&obj->suspend, 1);
		if(err = hwmsen_write_byte(client, LIS33DE_REG_CTL_REG1, dat))
		{
			GSE_ERR("write power control fail!!\n");
			return err;
		}        
		LIS33DE_power(obj->hw, 0);
	}
	return err;
}
/*----------------------------------------------------------------------------*/
static int lis33de_resume(struct i2c_client *client)
{
	struct lis33de_i2c_data *obj = i2c_get_clientdata(client);        
	int err;
	GSE_FUN();

	if(obj == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return -EINVAL;
	}

	LIS33DE_power(obj->hw, 1);
	if(err = LIS33DE_Init(client, 0))
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

static void lis33de_early_suspend(struct early_suspend *h) 
{
	struct lis33de_i2c_data *obj = container_of(h, struct lis33de_i2c_data, early_drv);   
	int err;
	GSE_FUN();    

	if(obj == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return;
	}
	atomic_set(&obj->suspend, 1); 
	/*
	if(err = hwmsen_write_byte(obj->client, LIS33DE_REG_POWER_CTL, 0x00))
	{
		GSE_ERR("write power control fail!!\n");
		return;
	}  
	*/
	if(err = LIS33DE_SetPowerMode(obj->client, false))
	{
		GSE_ERR("write power control fail!!\n");
		return;
	}

	sensor_power = false;
	
	LIS33DE_power(obj->hw, 0);
}
/*----------------------------------------------------------------------------*/
static void lis33de_late_resume(struct early_suspend *h)
{
	struct lis33de_i2c_data *obj = container_of(h, struct lis33de_i2c_data, early_drv);         
	int err;
	GSE_FUN();

	if(obj == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return;
	}

	LIS33DE_power(obj->hw, 1);
	if(err = LIS33DE_Init(obj->client, 0))
	{
		GSE_ERR("initialize client fail!!\n");
		return;        
	}
	atomic_set(&obj->suspend, 0);    
}
/*----------------------------------------------------------------------------*/
#endif /*CONFIG_HAS_EARLYSUSPEND*/
/*----------------------------------------------------------------------------*/
static int lis33de_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info) 
{    
	strcpy(info->type, LIS33DE_DEV_NAME);
	return 0;
}

/*----------------------------------------------------------------------------*/
static int lis33de_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_client *new_client;
	struct lis33de_i2c_data *obj;
	struct hwmsen_object sobj;
	int err = 0;
	GSE_FUN();

	if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
	{
		err = -ENOMEM;
		goto exit;
	}
	
	memset(obj, 0, sizeof(struct lis33de_i2c_data));

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
	
#ifdef CONFIG_LIS33DE_LOWPASS
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

	lis33de_i2c_client = new_client;	

	if(err = LIS33DE_Init(new_client, 1))
	{
		goto exit_init_failed;
	}
	

	if(err = misc_register(&lis33de_device))
	{
		GSE_ERR("lis33de_device register failed\n");
		goto exit_misc_device_register_failed;
	}

	if(err = lis33de_create_attr(&lis33de_gsensor_driver.driver))
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
	obj->early_drv.suspend  = lis33de_early_suspend,
	obj->early_drv.resume   = lis33de_late_resume,    
	register_early_suspend(&obj->early_drv);
#endif 

	GSE_LOG("%s: OK\n", __func__);    
	return 0;

	exit_create_attr_failed:
	misc_deregister(&lis33de_device);
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
static int lis33de_i2c_remove(struct i2c_client *client)
{
	int err = 0;	
	
	if(err = lis33de_delete_attr(&lis33de_gsensor_driver.driver))
	{
		GSE_ERR("lis33de_delete_attr fail: %d\n", err);
	}
	
	if(err = misc_deregister(&lis33de_device))
	{
		GSE_ERR("misc_deregister fail: %d\n", err);
	}

	if(err = hwmsen_detach(ID_ACCELEROMETER))
	    

	lis33de_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));
	return 0;
}
/*----------------------------------------------------------------------------*/
static int lis33de_probe(struct platform_device *pdev) 
{
	struct acc_hw *hw = get_cust_acc_hw();
	GSE_FUN();

	LIS33DE_power(hw, 1);
	//lis33de_force[0] = hw->i2c_num;
	if(i2c_add_driver(&lis33de_i2c_driver))
	{
		GSE_ERR("add driver error\n");
		return -1;
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
static int lis33de_remove(struct platform_device *pdev)
{
    struct acc_hw *hw = get_cust_acc_hw();

    GSE_FUN();    
    LIS33DE_power(hw, 0);    
    i2c_del_driver(&lis33de_i2c_driver);
    return 0;
}
/*----------------------------------------------------------------------------*/
static struct platform_driver lis33de_gsensor_driver = {
	.probe      = lis33de_probe,
	.remove     = lis33de_remove,    
	.driver     = {
		.name  = "gsensor",
		//.owner = THIS_MODULE,
	}
};

/*----------------------------------------------------------------------------*/
static int __init lis33de_init(void)
{
	GSE_FUN();
	struct acc_hw *hw = get_cust_acc_hw();
	GSE_LOG("%s: i2c_number=%d\n", __func__,hw->i2c_num); 
	i2c_register_board_info(hw->i2c_num, &i2c_lis33de, 1);
	if(platform_driver_register(&lis33de_gsensor_driver))
	{
		GSE_ERR("failed to register driver");
		return -ENODEV;
	}
	return 0;    
}
/*----------------------------------------------------------------------------*/
static void __exit lis33de_exit(void)
{
	GSE_FUN();
	platform_driver_unregister(&lis33de_gsensor_driver);
}
/*----------------------------------------------------------------------------*/
module_init(lis33de_init);
module_exit(lis33de_exit);
/*----------------------------------------------------------------------------*/
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("LIS33DE I2C driver");
MODULE_AUTHOR("Chunlei.Wang@mediatek.com");
