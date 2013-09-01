/* drivers/i2c/chips/mma8453q.c - MMA8453Q motion sensor driver
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
#include "mma8453q.h"
#include <linux/hwmsen_helper.h>

#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>

#define POWER_NONE_MACRO MT65XX_POWER_NONE


/*----------------------------------------------------------------------------*/
#define I2C_DRIVERID_MMA8453Q 345
/*----------------------------------------------------------------------------*/
#define DEBUG 1
/*----------------------------------------------------------------------------*/
#define CONFIG_MMA8453Q_LOWPASS   /*apply low pass filter on output*/       
/*----------------------------------------------------------------------------*/
#define MMA8453Q_AXIS_X          0
#define MMA8453Q_AXIS_Y          1
#define MMA8453Q_AXIS_Z          2
#define MMA8453Q_AXES_NUM        3
#define MMA8453Q_DATA_LEN        6
#define MMA8453Q_DEV_NAME        "MMA8453Q"
/*----------------------------------------------------------------------------*/
static const struct i2c_device_id mma8453q_i2c_id[] = {{MMA8453Q_DEV_NAME,0},{}};
/*the adapter id will be available in customization*/
static unsigned short mma8453q_force[] = {0x00, MMA8453Q_I2C_SLAVE_ADDR, I2C_CLIENT_END, I2C_CLIENT_END};
static const unsigned short *const mma8453q_forces[] = { mma8453q_force, NULL };
static struct i2c_client_address_data mma8453q_addr_data = { .forces = mma8453q_forces,};

/*----------------------------------------------------------------------------*/
static int mma8453q_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id); 
static int mma8453q_i2c_remove(struct i2c_client *client);
static int mma8453q_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);

/*----------------------------------------------------------------------------*/
static int MMA8453Q_SetPowerMode(struct i2c_client *client, bool enable);
static int  mma8453q_local_init(void);
static int mma8453q_remove(void);

static int mma8453q_init_flag =-1; // 0<==>OK -1 <==> fail



static struct sensor_init_info mma8453q_init_info = {
		.name = "mma8453q",
		.init = mma8453q_local_init,
		.uninit = mma8453q_remove,
	
};

/*------------------------------------------------------------------------------*/
typedef enum {
    ADX_TRC_FILTER  = 0x01,
    ADX_TRC_RAWDATA = 0x02,
    ADX_TRC_IOCTL   = 0x04,
    ADX_TRC_CALI	= 0X08,
    ADX_TRC_INFO	= 0X10,
    ADX_TRC_REGXYZ	= 0X20,
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
    s16 raw[C_MAX_FIR_LENGTH][MMA8453Q_AXES_NUM];
    int sum[MMA8453Q_AXES_NUM];
    int num;
    int idx;
};
/*----------------------------------------------------------------------------*/
struct mma8453q_i2c_data {
    struct i2c_client *client;
    struct acc_hw *hw;
    struct hwmsen_convert   cvt;
    
    /*misc*/
    struct data_resolution *reso;
    atomic_t                trace;
    atomic_t                suspend;
    atomic_t                selftest;
	atomic_t				filter;
    s16                     cali_sw[MMA8453Q_AXES_NUM+1];

    /*data*/
    s8                      offset[MMA8453Q_AXES_NUM+1];  /*+1: for 4-byte alignment*/
    s16                     data[MMA8453Q_AXES_NUM+1];

#if defined(CONFIG_MMA8453Q_LOWPASS)
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
static struct i2c_driver mma8453q_i2c_driver = {
    .driver = {
        .owner          = THIS_MODULE,
        .name           = MMA8453Q_DEV_NAME,
    },
	.probe      		= mma8453q_i2c_probe,
	.remove    			= mma8453q_i2c_remove,
	.detect				= mma8453q_i2c_detect,
#if !defined(CONFIG_HAS_EARLYSUSPEND)    
    .suspend            = mma8453q_suspend,
    .resume             = mma8453q_resume,
#endif
	.id_table = mma8453q_i2c_id,
	.address_data = &mma8453q_addr_data,
};

/*----------------------------------------------------------------------------*/
static struct i2c_client *mma8453q_i2c_client = NULL;
//static struct platform_driver mma8453q_gsensor_driver;
static struct mma8453q_i2c_data *obj_i2c_data = NULL;
static bool sensor_power = false;
static GSENSOR_VECTOR3D gsensor_gain, gsensor_offset;
static char selftestRes[10] = {0};



/*----------------------------------------------------------------------------*/
#define GSE_TAG                  "[Gsensor] "
#define GSE_FUN(f)               printk(KERN_INFO GSE_TAG"%s\n", __FUNCTION__)
#define GSE_ERR(fmt, args...)    printk(KERN_ERR GSE_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define GSE_LOG(fmt, args...)    printk(KERN_INFO GSE_TAG fmt, ##args)
/*----------------------------------------------------------------------------*/
static struct data_resolution mma8453q_data_resolution[] = {
 /*8 combination by {FULL_RES,RANGE}*/
    {{ 3, 9}, 256},   /*+/-2g  in 10-bit resolution:  3.9 mg/LSB*/
    {{ 7, 8}, 128},   /*+/-4g  in 10-bit resolution:  7.8 mg/LSB*/
    {{15, 6},  64},   /*+/-8g  in 10-bit resolution: 15.6 mg/LSB*/
    {{ 15, 6}, 64},   /*+/-2g  in 8-bit resolution:  3.9 mg/LSB (full-resolution)*/
    {{ 31, 3}, 32},   /*+/-4g  in 8-bit resolution:  3.9 mg/LSB (full-resolution)*/
    {{ 62, 5}, 16},   /*+/-8g  in 8-bit resolution:  3.9 mg/LSB (full-resolution)*/            
};
/*----------------------------------------------------------------------------*/
static struct data_resolution mma8453q_offset_resolution = {{2, 0}, 512};

/*--------------------ADXL power control function----------------------------------*/

int hwmsen_read_byte_sr(struct i2c_client *client, u8 addr, u8 *data)
{
   u8 buf;
    int ret = 0;
	
    client->addr = client->addr& I2C_MASK_FLAG | I2C_WR_FLAG |I2C_RS_FLAG;
    buf = addr;
	ret = i2c_master_send(client, (const char*)&buf, 1<<8 | 1);
    //ret = i2c_master_send(client, (const char*)&buf, 1);
    if (ret < 0) {
        HWM_ERR("send command error!!\n");
        return -EFAULT;
    }

    *data = buf;
	client->addr = client->addr& I2C_MASK_FLAG;
    return 0;
}

void dumpReg(struct i2c_client *client)
{
  int i=0;
  u8 addr = 0x00;
  u8 regdata=0;
  for(i=0; i<49 ; i++)
  {
    //dump all
    hwmsen_read_byte_sr(client,addr,&regdata);
	HWM_LOG("Reg addr=%x regdata=%x\n",addr,regdata);
	addr++;
	if(addr ==01)
		addr=addr+0x06;
	if(addr==0x09)
		addr++;
	if(addr==0x0A)
		addr++;
  }
  
  /*
  for(i=0; i<5 ; i++)
  {
    //dump ctrol_reg1~control_reg5
    hwmsen_read_byte_sr(client,addr,regdata);
	HWM_LOG("Reg addr=%x regdata=%x\n",addr,regdata);
	addr++;
  }
  
  addr = MMA8453Q_REG_OFSX;
  for(i=0; i<5 ; i++)
  {
    //dump offset
    hwmsen_read_byte_sr(client,addr,regdata);
	HWM_LOG("Reg addr=%x regdata=%x\n",addr,regdata);
	addr++
  }
  */
}

int hwmsen_read_block_sr(struct i2c_client *client, u8 addr, u8 *data)
{
   u8 buf[10];
    int ret = 0;
	memset(buf, 0, sizeof(u8)*10); 
	
    client->addr = client->addr& I2C_MASK_FLAG | I2C_WR_FLAG |I2C_RS_FLAG;
    buf[0] = addr;
	ret = i2c_master_send(client, (const char*)&buf, 6<<8 | 1);
    //ret = i2c_master_send(client, (const char*)&buf, 1);
    if (ret < 0) {
        HWM_ERR("send command error!!\n");
        return -EFAULT;
    }

    *data = buf;
	client->addr = client->addr& I2C_MASK_FLAG;
    return 0;
}

static void MMA8453Q_power(struct acc_hw *hw, unsigned int on) 
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
			if(!hwPowerOn(hw->power_id, hw->power_vol, "MMA8453Q"))
			{
				GSE_ERR("power on fails!!\n");
			}
		}
		else	// power off
		{
			if (!hwPowerDown(hw->power_id, "MMA8453Q"))
			{
				GSE_ERR("power off fail!!\n");
			}			  
		}
	}
	power_on = on;    
}
/*----------------------------------------------------------------------------*/
//this function here use to set resolution and choose sensitivity
static int MMA8453Q_SetDataResolution(struct i2c_client *client ,u8 dataresolution)
{
    GSE_LOG("fwq set resolution  dataresolution= %d!\n", dataresolution);
	int err;
	u8  dat, reso;
    u8 databuf[10];    
    int res = 0;
	struct mma8453q_i2c_data *obj = i2c_get_clientdata(client);

	if(hwmsen_read_byte_sr(client, MMA8453Q_REG_CTL_REG2, databuf))
	{
		GSE_ERR("read power ctl register err!\n");
		return -1;
	}
	GSE_LOG("fwq read MMA8453Q_REG_CTL_REG2 =%x in %s \n",databuf[0],__FUNCTION__);
	if(dataresolution == MMA8453Q_10BIT_RES)
	{
		databuf[0] |= MMA8453Q_10BIT_RES;
	}
	else
	{
		databuf[0] &= (~MMA8453Q_10BIT_RES);//8 bit resolution
	}
	databuf[1] = databuf[0];
	databuf[0] = MMA8453Q_REG_CTL_REG2;
	

	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		GSE_LOG("set resolution  failed!\n");
		return -1;
	}
	else
	{
		GSE_LOG("set resolution mode ok %x!\n", databuf[1]);
	}
	
    //choose sensitivity depend on resolution and detect range
	//read detect range
	if(err = hwmsen_read_byte_sr(client, MMA8453Q_REG_XYZ_DATA_CFG, &dat))
	{
		GSE_ERR("read detect range  fail!!\n");
		return err;
	}
	reso  = (dataresolution & MMA8453Q_10BIT_RES) ? (0x00) : (0x03);
	
	
    if(dat & MMA8453Q_RANGE_2G)
    {
      reso = reso + MMA8453Q_RANGE_2G;
    }
	if(dat & MMA8453Q_RANGE_4G)
    {
      reso = reso + MMA8453Q_RANGE_4G;
    }
	if(dat & MMA8453Q_RANGE_8G)
    {
      reso = reso + MMA8453Q_RANGE_8G;
    }

	if(reso < sizeof(mma8453q_data_resolution)/sizeof(mma8453q_data_resolution[0]))
	{        
		obj->reso = &mma8453q_data_resolution[reso];
		GSE_LOG("reso=%x!! OK \n",reso);
		return 0;
	}
	else
	{   
	    GSE_ERR("choose sensitivity  fail!!\n");
		return -EINVAL;
	}
}
/*----------------------------------------------------------------------------*/
static int MMA8453Q_ReadData(struct i2c_client *client, s16 data[MMA8453Q_AXES_NUM])
{
	struct mma8453q_i2c_data *priv = i2c_get_clientdata(client);        
	u8 addr = MMA8453Q_REG_DATAX0;
	u8 buf[MMA8453Q_DATA_LEN] = {0};
	int err = 0;

	if(NULL == client)
	{
		err = -EINVAL;
	}
	else
		
	{
	  // hwmsen_read_block(client, addr, buf, 0x06);
       // dumpReg(client);
	
		buf[0] = MMA8453Q_REG_DATAX0;
	    client->addr = client->addr& I2C_MASK_FLAG | I2C_WR_FLAG |I2C_RS_FLAG;
        i2c_master_send(client, (const char*)&buf, 6<<8 | 1);
	    client->addr = client->addr& I2C_MASK_FLAG;


		data[MMA8453Q_AXIS_X] = (s16)((buf[MMA8453Q_AXIS_X*2] << 8) |
		         (buf[MMA8453Q_AXIS_X*2+1]));
		data[MMA8453Q_AXIS_Y] = (s16)((buf[MMA8453Q_AXIS_Y*2] << 8) |
		         (buf[MMA8453Q_AXIS_Y*2+1]));
		data[MMA8453Q_AXIS_Z] = (s16)((buf[MMA8453Q_AXIS_Z*2] << 8) |
		         (buf[MMA8453Q_AXIS_Z*2+1]));
	    

		if(atomic_read(&priv->trace) & ADX_TRC_REGXYZ)
		{
			GSE_LOG("raw from reg(SR) [%08X %08X %08X] => [%5d %5d %5d]\n", data[MMA8453Q_AXIS_X], data[MMA8453Q_AXIS_Y], data[MMA8453Q_AXIS_Z],
		                               data[MMA8453Q_AXIS_X], data[MMA8453Q_AXIS_Y], data[MMA8453Q_AXIS_Z]);
		}
		
		//add to fix data, refer to datasheet
		
		data[MMA8453Q_AXIS_X] = data[MMA8453Q_AXIS_X]>>6;
		data[MMA8453Q_AXIS_Y] = data[MMA8453Q_AXIS_Y]>>6;
		data[MMA8453Q_AXIS_Z] = data[MMA8453Q_AXIS_Z]>>6;
		
		data[MMA8453Q_AXIS_X] += priv->cali_sw[MMA8453Q_AXIS_X];
		data[MMA8453Q_AXIS_Y] += priv->cali_sw[MMA8453Q_AXIS_Y];
		data[MMA8453Q_AXIS_Z] += priv->cali_sw[MMA8453Q_AXIS_Z];

		if(atomic_read(&priv->trace) & ADX_TRC_RAWDATA)
		{
			GSE_LOG("raw >>6it:[%08X %08X %08X] => [%5d %5d %5d]\n", data[MMA8453Q_AXIS_X], data[MMA8453Q_AXIS_Y], data[MMA8453Q_AXIS_Z],
		                               data[MMA8453Q_AXIS_X], data[MMA8453Q_AXIS_Y], data[MMA8453Q_AXIS_Z]);
		}
		
#ifdef CONFIG_MMA8453Q_LOWPASS
		if(atomic_read(&priv->filter))
		{
			if(atomic_read(&priv->fir_en) && !atomic_read(&priv->suspend))
			{
				int idx, firlen = atomic_read(&priv->firlen);   
				if(priv->fir.num < firlen)
				{                
					priv->fir.raw[priv->fir.num][MMA8453Q_AXIS_X] = data[MMA8453Q_AXIS_X];
					priv->fir.raw[priv->fir.num][MMA8453Q_AXIS_Y] = data[MMA8453Q_AXIS_Y];
					priv->fir.raw[priv->fir.num][MMA8453Q_AXIS_Z] = data[MMA8453Q_AXIS_Z];
					priv->fir.sum[MMA8453Q_AXIS_X] += data[MMA8453Q_AXIS_X];
					priv->fir.sum[MMA8453Q_AXIS_Y] += data[MMA8453Q_AXIS_Y];
					priv->fir.sum[MMA8453Q_AXIS_Z] += data[MMA8453Q_AXIS_Z];
					if(atomic_read(&priv->trace) & ADX_TRC_FILTER)
					{
						GSE_LOG("add [%2d] [%5d %5d %5d] => [%5d %5d %5d]\n", priv->fir.num,
							priv->fir.raw[priv->fir.num][MMA8453Q_AXIS_X], priv->fir.raw[priv->fir.num][MMA8453Q_AXIS_Y], priv->fir.raw[priv->fir.num][MMA8453Q_AXIS_Z],
							priv->fir.sum[MMA8453Q_AXIS_X], priv->fir.sum[MMA8453Q_AXIS_Y], priv->fir.sum[MMA8453Q_AXIS_Z]);
					}
					priv->fir.num++;
					priv->fir.idx++;
				}
				else
				{
					idx = priv->fir.idx % firlen;
					priv->fir.sum[MMA8453Q_AXIS_X] -= priv->fir.raw[idx][MMA8453Q_AXIS_X];
					priv->fir.sum[MMA8453Q_AXIS_Y] -= priv->fir.raw[idx][MMA8453Q_AXIS_Y];
					priv->fir.sum[MMA8453Q_AXIS_Z] -= priv->fir.raw[idx][MMA8453Q_AXIS_Z];
					priv->fir.raw[idx][MMA8453Q_AXIS_X] = data[MMA8453Q_AXIS_X];
					priv->fir.raw[idx][MMA8453Q_AXIS_Y] = data[MMA8453Q_AXIS_Y];
					priv->fir.raw[idx][MMA8453Q_AXIS_Z] = data[MMA8453Q_AXIS_Z];
					priv->fir.sum[MMA8453Q_AXIS_X] += data[MMA8453Q_AXIS_X];
					priv->fir.sum[MMA8453Q_AXIS_Y] += data[MMA8453Q_AXIS_Y];
					priv->fir.sum[MMA8453Q_AXIS_Z] += data[MMA8453Q_AXIS_Z];
					priv->fir.idx++;
					data[MMA8453Q_AXIS_X] = priv->fir.sum[MMA8453Q_AXIS_X]/firlen;
					data[MMA8453Q_AXIS_Y] = priv->fir.sum[MMA8453Q_AXIS_Y]/firlen;
					data[MMA8453Q_AXIS_Z] = priv->fir.sum[MMA8453Q_AXIS_Z]/firlen;
					if(atomic_read(&priv->trace) & ADX_TRC_FILTER)
					{
						GSE_LOG("add [%2d] [%5d %5d %5d] => [%5d %5d %5d] : [%5d %5d %5d]\n", idx,
						priv->fir.raw[idx][MMA8453Q_AXIS_X], priv->fir.raw[idx][MMA8453Q_AXIS_Y], priv->fir.raw[idx][MMA8453Q_AXIS_Z],
						priv->fir.sum[MMA8453Q_AXIS_X], priv->fir.sum[MMA8453Q_AXIS_Y], priv->fir.sum[MMA8453Q_AXIS_Z],
						data[MMA8453Q_AXIS_X], data[MMA8453Q_AXIS_Y], data[MMA8453Q_AXIS_Z]);
					}
				}
			}
		}	
#endif         
	}
	return err;
}
/*----------------------------------------------------------------------------*/
static int MMA8453Q_ReadOffset(struct i2c_client *client, s8 ofs[MMA8453Q_AXES_NUM])
{    
	int err;
    GSE_LOG("fwq read offset+: \n");

	err = hwmsen_read_block(client,MMA8453Q_REG_OFSX,&ofs[MMA8453Q_AXIS_X],3);
	if(err < 0)
	{
	   GSE_ERR(" MMA8453Q_ReadOffset error and retry: %d\n", err);
	   err = hwmsen_read_block(client,MMA8453Q_REG_OFSX,&ofs[MMA8453Q_AXIS_X],3);
	   if(err <0 )
	   {
	      GSE_ERR(" MMA8453Q_ReadOffset retry error: %d\n", err);
	   }
	}
	GSE_LOG("fwq read off:  offX=%x ,offY=%x ,offZ=%x\n",ofs[MMA8453Q_AXIS_X],ofs[MMA8453Q_AXIS_Y],ofs[MMA8453Q_AXIS_Z]);
	
	return err;    
}
/*----------------------------------------------------------------------------*/
static int MMA8453Q_ResetCalibration(struct i2c_client *client)
{
	struct mma8453q_i2c_data *obj = i2c_get_clientdata(client);
	s8 ofs[MMA8453Q_AXES_NUM] = {0x00, 0x00, 0x00};
	int err;

	//goto standby mode to clear cali
	MMA8453Q_SetPowerMode(obj->client,false);
	if(err = hwmsen_write_block(client, MMA8453Q_REG_OFSX, ofs, MMA8453Q_AXES_NUM))
	{
		GSE_ERR("error: %d\n", err);
	}
    MMA8453Q_SetPowerMode(obj->client,true);
	memset(obj->cali_sw, 0x00, sizeof(obj->cali_sw));
	return err;    
}
/*----------------------------------------------------------------------------*/
static int MMA8453Q_ReadCalibration(struct i2c_client *client, int dat[MMA8453Q_AXES_NUM])
{
    struct mma8453q_i2c_data *obj = i2c_get_clientdata(client);
    int err;
    int mul;
    
    if ((err = MMA8453Q_ReadOffset(client, obj->offset))) {
        GSE_ERR("read offset fail, %d\n", err);
        return err;
    }    
    
    //mul = obj->reso->sensitivity/mma8453q_offset_resolution.sensitivity;
    mul = mma8453q_offset_resolution.sensitivity/obj->reso->sensitivity;
    dat[obj->cvt.map[MMA8453Q_AXIS_X]] = obj->cvt.sign[MMA8453Q_AXIS_X]*(obj->offset[MMA8453Q_AXIS_X]/mul);
    dat[obj->cvt.map[MMA8453Q_AXIS_Y]] = obj->cvt.sign[MMA8453Q_AXIS_Y]*(obj->offset[MMA8453Q_AXIS_Y]/mul);
    dat[obj->cvt.map[MMA8453Q_AXIS_Z]] = obj->cvt.sign[MMA8453Q_AXIS_Z]*(obj->offset[MMA8453Q_AXIS_Z]/mul);                        
    GSE_LOG("fwq:read cali offX=%x ,offY=%x ,offZ=%x\n",obj->offset[MMA8453Q_AXIS_X],obj->offset[MMA8453Q_AXIS_Y],obj->offset[MMA8453Q_AXIS_Z]);
	//GSE_LOG("fwq:read cali swX=%x ,swY=%x ,swZ=%x\n",obj->cali_sw[MMA8453Q_AXIS_X],obj->cali_sw[MMA8453Q_AXIS_Y],obj->cali_sw[MMA8453Q_AXIS_Z]);
    return 0;
}
/*----------------------------------------------------------------------------*/
static int MMA8453Q_ReadCalibrationEx(struct i2c_client *client, int act[MMA8453Q_AXES_NUM], int raw[MMA8453Q_AXES_NUM])
{  
	/*raw: the raw calibration data; act: the actual calibration data*/
	struct mma8453q_i2c_data *obj = i2c_get_clientdata(client);
	int err;
	int mul;

	if(err = MMA8453Q_ReadOffset(client, obj->offset))
	{
		GSE_ERR("read offset fail, %d\n", err);
		return err;
	}    

	//mul = obj->reso->sensitivity/mma8453q_offset_resolution.sensitivity;
	mul = mma8453q_offset_resolution.sensitivity/obj->reso->sensitivity;
	raw[MMA8453Q_AXIS_X] = obj->offset[MMA8453Q_AXIS_X]/mul + obj->cali_sw[MMA8453Q_AXIS_X];
	raw[MMA8453Q_AXIS_Y] = obj->offset[MMA8453Q_AXIS_Y]/mul + obj->cali_sw[MMA8453Q_AXIS_Y];
	raw[MMA8453Q_AXIS_Z] = obj->offset[MMA8453Q_AXIS_Z]/mul + obj->cali_sw[MMA8453Q_AXIS_Z];

	act[obj->cvt.map[MMA8453Q_AXIS_X]] = obj->cvt.sign[MMA8453Q_AXIS_X]*raw[MMA8453Q_AXIS_X];
	act[obj->cvt.map[MMA8453Q_AXIS_Y]] = obj->cvt.sign[MMA8453Q_AXIS_Y]*raw[MMA8453Q_AXIS_Y];
	act[obj->cvt.map[MMA8453Q_AXIS_Z]] = obj->cvt.sign[MMA8453Q_AXIS_Z]*raw[MMA8453Q_AXIS_Z];                        
	                       
	return 0;
}
/*----------------------------------------------------------------------------*/
static int MMA8453Q_WriteCalibration(struct i2c_client *client, int dat[MMA8453Q_AXES_NUM])
{
	struct mma8453q_i2c_data *obj = i2c_get_clientdata(client);
	u8 testdata=0;
	int err;
	int cali[MMA8453Q_AXES_NUM], raw[MMA8453Q_AXES_NUM];
	int lsb = mma8453q_offset_resolution.sensitivity;
	u8 databuf[2]; 
	int res = 0;
	//int divisor = obj->reso->sensitivity/lsb;
	int divisor = lsb/obj->reso->sensitivity;
	GSE_LOG("fwq obj->reso->sensitivity=%d\n", obj->reso->sensitivity);
	GSE_LOG("fwq lsb=%d\n", lsb);
	

	if(err = MMA8453Q_ReadCalibrationEx(client, cali, raw))	/*offset will be updated in obj->offset*/
	{ 
		GSE_ERR("read offset fail, %d\n", err);
		return err;
	}

	GSE_LOG("OLDOFF: (%+3d %+3d %+3d): (%+3d %+3d %+3d) / (%+3d %+3d %+3d)\n", 
		raw[MMA8453Q_AXIS_X], raw[MMA8453Q_AXIS_Y], raw[MMA8453Q_AXIS_Z],
		obj->offset[MMA8453Q_AXIS_X], obj->offset[MMA8453Q_AXIS_Y], obj->offset[MMA8453Q_AXIS_Z],
		obj->cali_sw[MMA8453Q_AXIS_X], obj->cali_sw[MMA8453Q_AXIS_Y], obj->cali_sw[MMA8453Q_AXIS_Z]);

	/*calculate the real offset expected by caller*/
	cali[MMA8453Q_AXIS_X] += dat[MMA8453Q_AXIS_X];
	cali[MMA8453Q_AXIS_Y] += dat[MMA8453Q_AXIS_Y];
	cali[MMA8453Q_AXIS_Z] += dat[MMA8453Q_AXIS_Z];

	GSE_LOG("UPDATE: (%+3d %+3d %+3d)\n", 
		dat[MMA8453Q_AXIS_X], dat[MMA8453Q_AXIS_Y], dat[MMA8453Q_AXIS_Z]);

	obj->offset[MMA8453Q_AXIS_X] = (s8)(obj->cvt.sign[MMA8453Q_AXIS_X]*(cali[obj->cvt.map[MMA8453Q_AXIS_X]])*(divisor));
	obj->offset[MMA8453Q_AXIS_Y] = (s8)(obj->cvt.sign[MMA8453Q_AXIS_Y]*(cali[obj->cvt.map[MMA8453Q_AXIS_Y]])*(divisor));
	obj->offset[MMA8453Q_AXIS_Z] = (s8)(obj->cvt.sign[MMA8453Q_AXIS_Z]*(cali[obj->cvt.map[MMA8453Q_AXIS_Z]])*(divisor));

	/*convert software calibration using standard calibration*/
	obj->cali_sw[MMA8453Q_AXIS_X] =0; //obj->cvt.sign[MMA8453Q_AXIS_X]*(cali[obj->cvt.map[MMA8453Q_AXIS_X]])%(divisor);
	obj->cali_sw[MMA8453Q_AXIS_Y] =0; //obj->cvt.sign[MMA8453Q_AXIS_Y]*(cali[obj->cvt.map[MMA8453Q_AXIS_Y]])%(divisor);
	obj->cali_sw[MMA8453Q_AXIS_Z] =0;// obj->cvt.sign[MMA8453Q_AXIS_Z]*(cali[obj->cvt.map[MMA8453Q_AXIS_Z]])%(divisor);

	GSE_LOG("NEWOFF: (%+3d %+3d %+3d): (%+3d %+3d %+3d) / (%+3d %+3d %+3d)\n", 
		obj->offset[MMA8453Q_AXIS_X] + obj->cali_sw[MMA8453Q_AXIS_X], 
		obj->offset[MMA8453Q_AXIS_Y] + obj->cali_sw[MMA8453Q_AXIS_Y], 
		obj->offset[MMA8453Q_AXIS_Z] + obj->cali_sw[MMA8453Q_AXIS_Z], 
		obj->offset[MMA8453Q_AXIS_X], obj->offset[MMA8453Q_AXIS_Y], obj->offset[MMA8453Q_AXIS_Z],
		obj->cali_sw[MMA8453Q_AXIS_X], obj->cali_sw[MMA8453Q_AXIS_Y], obj->cali_sw[MMA8453Q_AXIS_Z]);
	//
	//go to standby mode to set cali
    MMA8453Q_SetPowerMode(obj->client,false);
	if(err = hwmsen_write_block(obj->client, MMA8453Q_REG_OFSX, obj->offset, MMA8453Q_AXES_NUM))
	{
		GSE_ERR("write offset fail: %d\n", err);
		return err;
	}
	MMA8453Q_SetPowerMode(obj->client,true);
	
	//
	/*
	MMA8453Q_SetPowerMode(obj->client,false);
	msleep(20);
	if(err = hwmsen_write_byte(obj->client, MMA8453Q_REG_OFSX, obj->offset[MMA8453Q_AXIS_X]))
	{
		GSE_ERR("write offset fail: %d\n", err);
		return err;
	}
    msleep(20);
	hwmsen_read_byte_sr(obj->client,MMA8453Q_REG_OFSX,&testdata);
	GSE_LOG("write offsetX: %x\n", testdata);
	
	if(err = hwmsen_write_byte(obj->client, MMA8453Q_REG_OFSY, obj->offset[MMA8453Q_AXIS_Y]))
	{
		GSE_ERR("write offset fail: %d\n", err);
		return err;
	}
	msleep(20);
	hwmsen_read_byte_sr(obj->client,MMA8453Q_REG_OFSY,&testdata);
	GSE_LOG("write offsetY: %x\n", testdata);
	
	if(err = hwmsen_write_byte(obj->client, MMA8453Q_REG_OFSZ, obj->offset[MMA8453Q_AXIS_Z]))
	{
		GSE_ERR("write offset fail: %d\n", err);
		return err;
	}
	msleep(20);
	hwmsen_read_byte_sr(obj->client,MMA8453Q_REG_OFSZ,&testdata);
	GSE_LOG("write offsetZ: %x\n", testdata);
	MMA8453Q_SetPowerMode(obj->client,true);
*/
	return err;
}
/*----------------------------------------------------------------------------*/
static int MMA8453Q_CheckDeviceID(struct i2c_client *client)
{
	u8 databuf[10];    
	int res = 0;

	memset(databuf, 0, sizeof(u8)*10);    
	databuf[0] = MMA8453Q_REG_DEVID;    

	res = hwmsen_read_byte_sr(client,MMA8453Q_REG_DEVID,databuf);
    GSE_LOG("fwq mma8453q id %x!\n",databuf[0]);
	if(databuf[0]!=MMA8453Q_FIXED_DEVID)
	{
		return MMA8453Q_ERR_IDENTIFICATION;
	}

	exit_MMA8453Q_CheckDeviceID:
	if (res < 0)
	{
		return MMA8453Q_ERR_I2C;
	}
	
	return MMA8453Q_SUCCESS;
}
/*----------------------------------------------------------------------------*/
//normal
//High resolution
//low noise low power
//low power

/*---------------------------------------------------------------------------*/
static int MMA8453Q_SetPowerMode(struct i2c_client *client, bool enable)
{
	u8 databuf[2];    
	int res = 0;
	u8 addr = MMA8453Q_REG_CTL_REG1;
	struct mma8453q_i2c_data *obj = i2c_get_clientdata(client);
	
	
	if(enable == sensor_power)
	{
		GSE_LOG("Sensor power status need not to be set again!!!\n");
		return MMA8453Q_SUCCESS;
	}

	if(hwmsen_read_byte_sr(client, addr, databuf))
	{
		GSE_ERR("read power ctl register err and retry!\n");
		if(hwmsen_read_byte_sr(client, addr, databuf))
	    {
		   GSE_ERR("read power ctl register retry err!\n");
		   return MMA8453Q_ERR_I2C;
	    }
		
	}

	databuf[0] &= ~MMA8453Q_MEASURE_MODE;
	
	if(enable == TRUE)
	{
		databuf[0] |= MMA8453Q_MEASURE_MODE;
	}
	else
	{
		// do nothing
	}
	databuf[1] = databuf[0];
	databuf[0] = MMA8453Q_REG_CTL_REG1;
	

	res = i2c_master_send(client, databuf, 0x2);

	if(res <= 0)
	{
		GSE_LOG("fwq set power mode failed!\n");
		return MMA8453Q_ERR_I2C;
	}
	else if(atomic_read(&obj->trace) & ADX_TRC_INFO)
	{
		GSE_LOG("fwq set power mode ok %d!\n", databuf[1]);
	}

	sensor_power = enable;
	
	return MMA8453Q_SUCCESS;    
}
/*----------------------------------------------------------------------------*/
//set detect range

static int MMA8453Q_SetDataFormat(struct i2c_client *client, u8 dataformat)
{
    
	struct mma8453q_i2c_data *obj = i2c_get_clientdata(client);
	u8 databuf[10];    
	int res = 0;

	memset(databuf, 0, sizeof(u8)*10);    
	databuf[0] = MMA8453Q_REG_XYZ_DATA_CFG;    
	databuf[1] = dataformat;

	res = i2c_master_send(client, databuf, 0x2);

	if(res <= 0)
	{
		return MMA8453Q_ERR_I2C;
	}

	return 0;

	//return MMA8453Q_SetDataResolution(obj,dataformat);    
}
/*----------------------------------------------------------------------------*/
static int MMA8453Q_SetBWRate(struct i2c_client *client, u8 bwrate)
{
	u8 databuf[10];    
	int res = 0;

	memset(databuf, 0, sizeof(u8)*10);    
	databuf[0] = MMA8453Q_REG_CTL_REG1;    
	//databuf[1] = bwrate;
	
	if(hwmsen_read_byte_sr(client, MMA8453Q_REG_CTL_REG1, databuf))
	{
		GSE_ERR("read power ctl register err and retry!\n");
		if(hwmsen_read_byte_sr(client, MMA8453Q_REG_CTL_REG1, databuf))
	    {
		   GSE_ERR("read power ctl register retry err!\n");
		   return MMA8453Q_ERR_I2C;
	    } 
	}
	GSE_LOG("fwq read MMA8453Q_REG_CTL_REG1 =%x in %s \n",databuf[0],__FUNCTION__);

	databuf[0] &=0xC7;//clear original  data rate 
		
	databuf[0] |= bwrate; //set data rate
	databuf[1]= databuf[0];
	databuf[0]= MMA8453Q_REG_CTL_REG1;

	res = i2c_master_send(client, databuf, 0x2);

	if(res <= 0)
	{
		return MMA8453Q_ERR_I2C;
	}
	
	return MMA8453Q_SUCCESS;    
}
/*----------------------------------------------------------------------------*/
static int MMA8453Q_SetIntEnable(struct i2c_client *client, u8 intenable)
{
	u8 databuf[10];    
	int res = 0;

	memset(databuf, 0, sizeof(u8)*10);    
	databuf[0] = MMA8453Q_REG_CTL_REG4;    
	databuf[1] = intenable;

	res = i2c_master_send(client, databuf, 0x2);

	if(res <= 0)
	{
		return MMA8453Q_ERR_I2C;
	}
	
	return MMA8453Q_SUCCESS;    
}
/*----------------------------------------------------------------------------*/
static int MMA8453Q_Init(struct i2c_client *client, int reset_cali)
{
	struct mma8453q_i2c_data *obj = i2c_get_clientdata(client);
	int res = 0;
    GSE_LOG("2010-11-03-11:43 fwq mma8453q addr %x!\n",client->addr);
	/*
	res = MMA8453Q_CheckDeviceID(client); 
	if(res != MMA8453Q_SUCCESS)
	{
	    GSE_LOG("fwq mma8453q check id error\n");
		return res;
	}	
*/
	res = MMA8453Q_SetPowerMode(client, false);
	if(res != MMA8453Q_SUCCESS)
	{
	    GSE_LOG("fwq mma8453q set power error\n");
		return res;
	}
	

	res = MMA8453Q_SetBWRate(client, MMA8453Q_BW_100HZ);
	if(res != MMA8453Q_SUCCESS ) 
	{
	    GSE_LOG("fwq mma8453q set BWRate error\n");
		return res;
	}

	res = MMA8453Q_SetDataFormat(client, MMA8453Q_RANGE_2G);
	if(res != MMA8453Q_SUCCESS)
	{
	    GSE_LOG("fwq mma8453q set data format error\n");
		return res;
	}
	//add by fwq
	res = MMA8453Q_SetDataResolution(client, MMA8453Q_10BIT_RES);
	if(res != MMA8453Q_SUCCESS) 
	{
	    GSE_LOG("fwq mma8453q set data reslution error\n");
		return res;
	}
	gsensor_gain.x = gsensor_gain.y = gsensor_gain.z = obj->reso->sensitivity;
/*//we do not use interrupt
	res = MMA8453Q_SetIntEnable(client, MMA8453Q_DATA_READY);        
	if(res != MMA8453Q_SUCCESS)//0x2E->0x80
	{
		return res;
	}
*/
    
	if(NULL != reset_cali)
	{ 
		/*reset calibration only in power on*/
		GSE_LOG("fwq mma8453q  set cali\n");
		res = MMA8453Q_ResetCalibration(client);
		if(res != MMA8453Q_SUCCESS)
		{
		    GSE_LOG("fwq mma8453q set cali error\n");
			return res;
		}
	}

#ifdef CONFIG_MMA8453Q_LOWPASS
	memset(&obj->fir, 0x00, sizeof(obj->fir));  
#endif
    GSE_LOG("fwq mma8453q Init OK\n");
	return MMA8453Q_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int MMA8453Q_ReadChipInfo(struct i2c_client *client, char *buf, int bufsize)
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

	sprintf(buf, "MMA8453Q Chip");
	return 0;
}
/*----------------------------------------------------------------------------*/
static int MMA8453Q_ReadSensorData(struct i2c_client *client, char *buf, int bufsize)
{
	struct mma8453q_i2c_data *obj = (struct mma8453q_i2c_data*)i2c_get_clientdata(client);
	u8 databuf[20];
	int acc[MMA8453Q_AXES_NUM];
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
		res = MMA8453Q_SetPowerMode(client, true);
		if(res)
		{
			GSE_ERR("Power on mma8453q error %d!\n", res);
		}
	}

	if(res = MMA8453Q_ReadData(client, obj->data))
	{        
		GSE_ERR("I2C error: ret value=%d", res);
		return -3;
	}
	else
	{
		obj->data[MMA8453Q_AXIS_X] += obj->cali_sw[MMA8453Q_AXIS_X];
		obj->data[MMA8453Q_AXIS_Y] += obj->cali_sw[MMA8453Q_AXIS_Y];
		obj->data[MMA8453Q_AXIS_Z] += obj->cali_sw[MMA8453Q_AXIS_Z];
		
		/*remap coordinate*/
		acc[obj->cvt.map[MMA8453Q_AXIS_X]] = obj->cvt.sign[MMA8453Q_AXIS_X]*obj->data[MMA8453Q_AXIS_X];
		acc[obj->cvt.map[MMA8453Q_AXIS_Y]] = obj->cvt.sign[MMA8453Q_AXIS_Y]*obj->data[MMA8453Q_AXIS_Y];
		acc[obj->cvt.map[MMA8453Q_AXIS_Z]] = obj->cvt.sign[MMA8453Q_AXIS_Z]*obj->data[MMA8453Q_AXIS_Z];

		//GSE_LOG("Mapped gsensor data: %d, %d, %d!\n", acc[MMA8453Q_AXIS_X], acc[MMA8453Q_AXIS_Y], acc[MMA8453Q_AXIS_Z]);

		//Out put the mg
		acc[MMA8453Q_AXIS_X] = acc[MMA8453Q_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		acc[MMA8453Q_AXIS_Y] = acc[MMA8453Q_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		acc[MMA8453Q_AXIS_Z] = acc[MMA8453Q_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;		
		

		sprintf(buf, "%04x %04x %04x", acc[MMA8453Q_AXIS_X], acc[MMA8453Q_AXIS_Y], acc[MMA8453Q_AXIS_Z]);
		if(atomic_read(&obj->trace) & ADX_TRC_IOCTL)
		{
			GSE_LOG("gsensor data: %s!\n", buf);
			GSE_LOG("gsensor data:  sensitivity x=%d \n",gsensor_gain.z);
			 
		}
	}
	
	return 0;
}
/*----------------------------------------------------------------------------*/
static int MMA8453Q_ReadRawData(struct i2c_client *client, char *buf)
{
	struct mma8453q_i2c_data *obj = (struct mma8453q_i2c_data*)i2c_get_clientdata(client);
	int res = 0;

	if (!buf || !client)
	{
		return EINVAL;
	}

	if(sensor_power == FALSE)
	{
		res = MMA8453Q_SetPowerMode(client, true);
		if(res)
		{
			GSE_ERR("Power on mma8453q error %d!\n", res);
		}
	}
	
	if(res = MMA8453Q_ReadData(client, obj->data))
	{        
		GSE_ERR("I2C error: ret value=%d", res);
		return EIO;
	}
	else
	{
		sprintf(buf, "%04x %04x %04x", obj->data[MMA8453Q_AXIS_X], 
			obj->data[MMA8453Q_AXIS_Y], obj->data[MMA8453Q_AXIS_Z]);
	
	}
	
	return 0;
}
/*----------------------------------------------------------------------------*/
static int MMA8453Q_InitSelfTest(struct i2c_client *client)
{
	int res = 0;
	u8  data;
	u8 databuf[10]; 
    GSE_LOG("fwq init self test\n");
/*
	res = MMA8453Q_SetPowerMode(client,true);
	if(res != MMA8453Q_SUCCESS ) //
	{
		return res;
	}
	*/
	res = MMA8453Q_SetBWRate(client, MMA8453Q_BW_100HZ);
	if(res != MMA8453Q_SUCCESS ) //
	{
		return res;
	}	
	
	res = MMA8453Q_SetDataFormat(client, MMA8453Q_RANGE_2G);
	if(res != MMA8453Q_SUCCESS) //0x2C->BW=100Hz
	{
		return res;
	}
	res = MMA8453Q_SetDataResolution(client, MMA8453Q_10BIT_RES);
	if(res != MMA8453Q_SUCCESS) 
	{
	    GSE_LOG("fwq mma8453q set data reslution error\n");
		return res;
	}

	//set self test reg
	memset(databuf, 0, sizeof(u8)*10);    
	databuf[0] = MMA8453Q_REG_CTL_REG2;//set self test    
	if(hwmsen_read_byte_sr(client, MMA8453Q_REG_CTL_REG2, databuf))
	{
		GSE_ERR("read power ctl register err!\n");
		return MMA8453Q_ERR_I2C;
	}

	databuf[0] &=~0x80;//clear original    	
	databuf[0] |= 0x80; //set self test
	
	databuf[1]= databuf[0];
	databuf[0]= MMA8453Q_REG_CTL_REG2;

	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
	    GSE_LOG("fwq set selftest error\n");
		return MMA8453Q_ERR_I2C;
	}
	
	GSE_LOG("fwq init self test OK\n");
	return MMA8453Q_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int MMA8453Q_JudgeTestResult(struct i2c_client *client, s32 prv[MMA8453Q_AXES_NUM], s32 nxt[MMA8453Q_AXES_NUM])
{
    struct criteria {
        int min;
        int max;
    };
	
    struct criteria self[6][3] = {
        {{-2, 11}, {-2, 16}, {-20, 105}},
        {{-10, 89}, {0, 125}, {0, 819}},
        {{12, 135}, {-135, -12}, {19, 219}},            
        {{ 6,  67}, {-67,  -6},  {10, 110}},
        {{ 6,  67}, {-67,  -6},  {10, 110}},
        {{ 50,  540}, {-540,  -50},  {75, 875}},
    };
    struct criteria (*ptr)[3] = NULL;
    u8 detectRage;
	u8 tmp_resolution;
    int res;
	GSE_LOG("fwq judge test result\n");
    if(res = hwmsen_read_byte_sr(client, MMA8453Q_REG_XYZ_DATA_CFG, &detectRage))
        return res;
	if(res = hwmsen_read_byte_sr(client, MMA8453Q_REG_CTL_REG2, &tmp_resolution))
        return res;

	GSE_LOG("fwq tmp_resolution=%x , detectRage=%x\n",tmp_resolution,detectRage);
	if((tmp_resolution&MMA8453Q_10BIT_RES) && (detectRage==0x00))
		ptr = &self[0];
	else if((tmp_resolution&MMA8453Q_10BIT_RES) && (detectRage&MMA8453Q_RANGE_4G))
	{
		ptr = &self[1];
		GSE_LOG("fwq self test choose ptr1\n");
	}
	else if((tmp_resolution&MMA8453Q_10BIT_RES) && (detectRage&MMA8453Q_RANGE_8G))
		ptr = &self[2];
	else if(detectRage&MMA8453Q_RANGE_2G)//8 bit resolution
		ptr = &self[3];
	else if(detectRage&MMA8453Q_RANGE_4G)//8 bit resolution
		ptr = &self[4];
	else if(detectRage&MMA8453Q_RANGE_8G)//8 bit resolution
		ptr = &self[5];
	

    if (!ptr) {
        GSE_ERR("null pointer\n");
		GSE_LOG("fwq ptr null\n");
        return -EINVAL;
    }

    if (((nxt[MMA8453Q_AXIS_X] - prv[MMA8453Q_AXIS_X]) > (*ptr)[MMA8453Q_AXIS_X].max) ||
        ((nxt[MMA8453Q_AXIS_X] - prv[MMA8453Q_AXIS_X]) < (*ptr)[MMA8453Q_AXIS_X].min)) {
        GSE_ERR("X is over range\n");
        res = -EINVAL;
    }
    if (((nxt[MMA8453Q_AXIS_Y] - prv[MMA8453Q_AXIS_Y]) > (*ptr)[MMA8453Q_AXIS_Y].max) ||
        ((nxt[MMA8453Q_AXIS_Y] - prv[MMA8453Q_AXIS_Y]) < (*ptr)[MMA8453Q_AXIS_Y].min)) {
        GSE_ERR("Y is over range\n");
        res = -EINVAL;
    }
    if (((nxt[MMA8453Q_AXIS_Z] - prv[MMA8453Q_AXIS_Z]) > (*ptr)[MMA8453Q_AXIS_Z].max) ||
        ((nxt[MMA8453Q_AXIS_Z] - prv[MMA8453Q_AXIS_Z]) < (*ptr)[MMA8453Q_AXIS_Z].min)) {
        GSE_ERR("Z is over range\n");
        res = -EINVAL;
    }
    return res;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
    GSE_LOG("fwq show_chipinfo_value \n");
	struct i2c_client *client = mma8453q_i2c_client;
	char strbuf[MMA8453Q_BUFSIZE];
	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	
	MMA8453Q_ReadChipInfo(client, strbuf, MMA8453Q_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);        
}
/*----------------------------------------------------------------------------*/
static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = mma8453q_i2c_client;
	char strbuf[MMA8453Q_BUFSIZE];
	
	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	MMA8453Q_ReadSensorData(client, strbuf, MMA8453Q_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);            
}
/*----------------------------------------------------------------------------*/
static ssize_t show_cali_value(struct device_driver *ddri, char *buf)
{
    GSE_LOG("fwq show_cali_value \n");
	struct i2c_client *client = mma8453q_i2c_client;
	struct mma8453q_i2c_data *obj;

	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}

	obj = i2c_get_clientdata(client);

	int err, len = 0, mul;
	int tmp[MMA8453Q_AXES_NUM];

	if(err = MMA8453Q_ReadOffset(client, obj->offset))
	{
		return -EINVAL;
	}
	else if(err = MMA8453Q_ReadCalibration(client, tmp))
	{
		return -EINVAL;
	}
	else
	{    
		mul = obj->reso->sensitivity/mma8453q_offset_resolution.sensitivity;
		len += snprintf(buf+len, PAGE_SIZE-len, "[HW ][%d] (%+3d, %+3d, %+3d) : (0x%02X, 0x%02X, 0x%02X)\n", mul,                        
			obj->offset[MMA8453Q_AXIS_X], obj->offset[MMA8453Q_AXIS_Y], obj->offset[MMA8453Q_AXIS_Z],
			obj->offset[MMA8453Q_AXIS_X], obj->offset[MMA8453Q_AXIS_Y], obj->offset[MMA8453Q_AXIS_Z]);
		len += snprintf(buf+len, PAGE_SIZE-len, "[SW ][%d] (%+3d, %+3d, %+3d)\n", 1, 
			obj->cali_sw[MMA8453Q_AXIS_X], obj->cali_sw[MMA8453Q_AXIS_Y], obj->cali_sw[MMA8453Q_AXIS_Z]);

		len += snprintf(buf+len, PAGE_SIZE-len, "[ALL]    (%+3d, %+3d, %+3d) : (%+3d, %+3d, %+3d)\n", 
			obj->offset[MMA8453Q_AXIS_X]*mul + obj->cali_sw[MMA8453Q_AXIS_X],
			obj->offset[MMA8453Q_AXIS_Y]*mul + obj->cali_sw[MMA8453Q_AXIS_Y],
			obj->offset[MMA8453Q_AXIS_Z]*mul + obj->cali_sw[MMA8453Q_AXIS_Z],
			tmp[MMA8453Q_AXIS_X], tmp[MMA8453Q_AXIS_Y], tmp[MMA8453Q_AXIS_Z]);
		
		return len;
    }
}
/*----------------------------------------------------------------------------*/
static ssize_t store_cali_value(struct device_driver *ddri, char *buf, size_t count)
{
	struct i2c_client *client = mma8453q_i2c_client;  
	int err, x, y, z;
	int dat[MMA8453Q_AXES_NUM];

	if(!strncmp(buf, "rst", 3))
	{
		if(err = MMA8453Q_ResetCalibration(client))
		{
			GSE_ERR("reset offset err = %d\n", err);
		}	
	}
	else if(3 == sscanf(buf, "0x%02X 0x%02X 0x%02X", &x, &y, &z))
	{
		dat[MMA8453Q_AXIS_X] = x;
		dat[MMA8453Q_AXIS_Y] = y;
		dat[MMA8453Q_AXIS_Z] = z;
		if(err = MMA8453Q_WriteCalibration(client, dat))
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
static ssize_t show_selftest_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = mma8453q_i2c_client;
	struct mma8453q_i2c_data *obj;
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
{   /*write anything to this register will trigger the process*/
	struct item{
	s16 raw[MMA8453Q_AXES_NUM];
	};
	
	struct i2c_client *client = mma8453q_i2c_client;  
	struct mma8453q_i2c_data *obj = i2c_get_clientdata(client);
	int idx, res, num;
	struct item *prv = NULL, *nxt = NULL;
	s32 avg_prv[MMA8453Q_AXES_NUM] = {0, 0, 0};
	s32 avg_nxt[MMA8453Q_AXES_NUM] = {0, 0, 0};
    u8 databuf[10];

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

	res = MMA8453Q_SetPowerMode(client,true);
	if(res != MMA8453Q_SUCCESS ) //
	{
		return res;
	}

	GSE_LOG("NORMAL:\n");
	for(idx = 0; idx < num; idx++)
	{
		if(res = MMA8453Q_ReadData(client, prv[idx].raw))
		{            
			GSE_ERR("read data fail: %d\n", res);
			goto exit;
		}
		
		avg_prv[MMA8453Q_AXIS_X] += prv[idx].raw[MMA8453Q_AXIS_X];
		avg_prv[MMA8453Q_AXIS_Y] += prv[idx].raw[MMA8453Q_AXIS_Y];
		avg_prv[MMA8453Q_AXIS_Z] += prv[idx].raw[MMA8453Q_AXIS_Z];        
		GSE_LOG("[%5d %5d %5d]\n", prv[idx].raw[MMA8453Q_AXIS_X], prv[idx].raw[MMA8453Q_AXIS_Y], prv[idx].raw[MMA8453Q_AXIS_Z]);
	}
	
	avg_prv[MMA8453Q_AXIS_X] /= num;
	avg_prv[MMA8453Q_AXIS_Y] /= num;
	avg_prv[MMA8453Q_AXIS_Z] /= num; 

	res = MMA8453Q_SetPowerMode(client,false);
	if(res != MMA8453Q_SUCCESS ) //
	{
		return res;
	}

	/*initial setting for self test*/
	MMA8453Q_InitSelfTest(client);
	GSE_LOG("SELFTEST:\n");  
/*
	MMA8453Q_ReadData(client, nxt[0].raw);
	GSE_LOG("nxt[0].raw[MMA8453Q_AXIS_X]: %d\n", nxt[0].raw[MMA8453Q_AXIS_X]);
	GSE_LOG("nxt[0].raw[MMA8453Q_AXIS_Y]: %d\n", nxt[0].raw[MMA8453Q_AXIS_Y]);
	GSE_LOG("nxt[0].raw[MMA8453Q_AXIS_Z]: %d\n", nxt[0].raw[MMA8453Q_AXIS_Z]);
	*/
	for(idx = 0; idx < num; idx++)
	{
		if(res = MMA8453Q_ReadData(client, nxt[idx].raw))
		{            
			GSE_ERR("read data fail: %d\n", res);
			goto exit;
		}
		avg_nxt[MMA8453Q_AXIS_X] += nxt[idx].raw[MMA8453Q_AXIS_X];
		avg_nxt[MMA8453Q_AXIS_Y] += nxt[idx].raw[MMA8453Q_AXIS_Y];
		avg_nxt[MMA8453Q_AXIS_Z] += nxt[idx].raw[MMA8453Q_AXIS_Z];        
		GSE_LOG("[%5d %5d %5d]\n", nxt[idx].raw[MMA8453Q_AXIS_X], nxt[idx].raw[MMA8453Q_AXIS_Y], nxt[idx].raw[MMA8453Q_AXIS_Z]);
	}

	//softrestet

	memset(databuf, 0, sizeof(u8)*10);    
	databuf[0] = MMA8453Q_REG_CTL_REG2;//set self test    
	if(hwmsen_read_byte_sr(client, MMA8453Q_REG_CTL_REG2, databuf))
	{
		GSE_ERR("read power ctl2 register err!\n");
		return MMA8453Q_ERR_I2C;
	}

	databuf[0] &=~0x40;//clear original    	
	databuf[0] |= 0x40; 
	
	databuf[1]= databuf[0];
	databuf[0]= MMA8453Q_REG_CTL_REG2;

	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
	    GSE_LOG("fwq softrest error\n");
		return MMA8453Q_ERR_I2C;
	}

	// 
	MMA8453Q_Init(client, 0);

	avg_nxt[MMA8453Q_AXIS_X] /= num;
	avg_nxt[MMA8453Q_AXIS_Y] /= num;
	avg_nxt[MMA8453Q_AXIS_Z] /= num;    

	GSE_LOG("X: %5d - %5d = %5d \n", avg_nxt[MMA8453Q_AXIS_X], avg_prv[MMA8453Q_AXIS_X], avg_nxt[MMA8453Q_AXIS_X] - avg_prv[MMA8453Q_AXIS_X]);
	GSE_LOG("Y: %5d - %5d = %5d \n", avg_nxt[MMA8453Q_AXIS_Y], avg_prv[MMA8453Q_AXIS_Y], avg_nxt[MMA8453Q_AXIS_Y] - avg_prv[MMA8453Q_AXIS_Y]);
	GSE_LOG("Z: %5d - %5d = %5d \n", avg_nxt[MMA8453Q_AXIS_Z], avg_prv[MMA8453Q_AXIS_Z], avg_nxt[MMA8453Q_AXIS_Z] - avg_prv[MMA8453Q_AXIS_Z]); 

	if(!MMA8453Q_JudgeTestResult(client, avg_prv, avg_nxt))
	{
		GSE_LOG("SELFTEST : PASS\n");
		atomic_set(&obj->selftest, 1); 
		strcpy(selftestRes,"y");
		
	}	
	else
	{
		GSE_LOG("SELFTEST : FAIL\n");
		atomic_set(&obj->selftest, 0);
		strcpy(selftestRes,"n");
	}
	
	exit:
	/*restore the setting*/    
	MMA8453Q_Init(client, 0);
	kfree(prv);
	kfree(nxt);
	return count;
}
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
static ssize_t show_firlen_value(struct device_driver *ddri, char *buf)
{
    GSE_LOG("fwq show_firlen_value \n");
#ifdef CONFIG_MMA8453Q_LOWPASS
	struct i2c_client *client = mma8453q_i2c_client;
	struct mma8453q_i2c_data *obj = i2c_get_clientdata(client);
	if(atomic_read(&obj->firlen))
	{
		int idx, len = atomic_read(&obj->firlen);
		GSE_LOG("len = %2d, idx = %2d\n", obj->fir.num, obj->fir.idx);

		for(idx = 0; idx < len; idx++)
		{
			GSE_LOG("[%5d %5d %5d]\n", obj->fir.raw[idx][MMA8453Q_AXIS_X], obj->fir.raw[idx][MMA8453Q_AXIS_Y], obj->fir.raw[idx][MMA8453Q_AXIS_Z]);
		}
		
		GSE_LOG("sum = [%5d %5d %5d]\n", obj->fir.sum[MMA8453Q_AXIS_X], obj->fir.sum[MMA8453Q_AXIS_Y], obj->fir.sum[MMA8453Q_AXIS_Z]);
		GSE_LOG("avg = [%5d %5d %5d]\n", obj->fir.sum[MMA8453Q_AXIS_X]/len, obj->fir.sum[MMA8453Q_AXIS_Y]/len, obj->fir.sum[MMA8453Q_AXIS_Z]/len);
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&obj->firlen));
#else
	return snprintf(buf, PAGE_SIZE, "not support\n");
#endif
}
/*----------------------------------------------------------------------------*/
static ssize_t store_firlen_value(struct device_driver *ddri, char *buf, size_t count)
{
    GSE_LOG("fwq store_firlen_value \n");
#ifdef CONFIG_MMA8453Q_LOWPASS
	struct i2c_client *client = mma8453q_i2c_client;  
	struct mma8453q_i2c_data *obj = i2c_get_clientdata(client);
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
    GSE_LOG("fwq show_trace_value \n");
	ssize_t res;
	struct mma8453q_i2c_data *obj = obj_i2c_data;
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
    GSE_LOG("fwq store_trace_value \n");
	struct mma8453q_i2c_data *obj = obj_i2c_data;
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
    GSE_LOG("fwq show_status_value \n");
	ssize_t len = 0;    
	struct mma8453q_i2c_data *obj = obj_i2c_data;
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
static DRIVER_ATTR(selftest,       S_IWUSR | S_IRUGO, show_selftest_value,          store_selftest_value);
static DRIVER_ATTR(firlen,     S_IWUSR | S_IRUGO, show_firlen_value,        store_firlen_value);
static DRIVER_ATTR(trace,      S_IWUSR | S_IRUGO, show_trace_value,         store_trace_value);
static DRIVER_ATTR(status,               S_IRUGO, show_status_value,        NULL);
/*----------------------------------------------------------------------------*/
static struct driver_attribute *mma8453q_attr_list[] = {
	&driver_attr_chipinfo,     /*chip information*/
	&driver_attr_sensordata,   /*dump sensor data*/
	&driver_attr_cali,         /*show calibration data*/
	&driver_attr_selftest,         /*self test demo*/
	&driver_attr_firlen,       /*filter length: 0: disable, others: enable*/
	&driver_attr_trace,        /*trace log*/
	&driver_attr_status,        
};
/*----------------------------------------------------------------------------*/
static int mma8453q_create_attr(struct device_driver *driver) 
{
	int idx, err = 0;
	int num = (int)(sizeof(mma8453q_attr_list)/sizeof(mma8453q_attr_list[0]));
	if (driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
		if(err = driver_create_file(driver, mma8453q_attr_list[idx]))
		{            
			GSE_ERR("driver_create_file (%s) = %d\n", mma8453q_attr_list[idx]->attr.name, err);
			break;
		}
	}    
	return err;
}
/*----------------------------------------------------------------------------*/
static int mma8453q_delete_attr(struct device_driver *driver)
{
	int idx ,err = 0;
	int num = (int)(sizeof(mma8453q_attr_list)/sizeof(mma8453q_attr_list[0]));

	if(driver == NULL)
	{
		return -EINVAL;
	}
	
	for(idx = 0; idx < num; idx++)
	{
		driver_remove_file(driver, mma8453q_attr_list[idx]);
	}
	
	return err;
}

/*----------------------------------------------------------------------------*/
int mma8453q_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value, sample_delay;	
	struct mma8453q_i2c_data *priv = (struct mma8453q_i2c_data*)self;
	hwm_sensor_data* gsensor_data;
	char buff[MMA8453Q_BUFSIZE];
	
	//GSE_FUN(f);
	switch (command)
	{
		case SENSOR_DELAY:
			//GSE_LOG("fwq set delay\n");
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
					sample_delay = MMA8453Q_BW_200HZ;
				}
				else if(value <= 10)
				{
					sample_delay = MMA8453Q_BW_100HZ;
				}
				else
				{
					sample_delay = MMA8453Q_BW_50HZ;
				}
				
				err = MMA8453Q_SetBWRate(priv->client, MMA8453Q_BW_100HZ); //err = MMA8453Q_SetBWRate(priv->client, sample_delay);
				if(err != MMA8453Q_SUCCESS ) //0x2C->BW=100Hz
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
					priv->fir.sum[MMA8453Q_AXIS_X] = 0;
					priv->fir.sum[MMA8453Q_AXIS_Y] = 0;
					priv->fir.sum[MMA8453Q_AXIS_Z] = 0;
					atomic_set(&priv->filter, 1);
				}
			}
			break;

		case SENSOR_ENABLE:
			GSE_LOG("fwq sensor enable gsensor\n");
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
					err = MMA8453Q_SetPowerMode( priv->client, !sensor_power);
				}
			}
			break;

		case SENSOR_GET_DATA:
			//GSE_LOG("fwq sensor operate get data\n");
			if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
			{
				GSE_ERR("get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				gsensor_data = (hwm_sensor_data *)buff_out;
				MMA8453Q_ReadSensorData(priv->client, buff, MMA8453Q_BUFSIZE);
				sscanf(buff, "%x %x %x", &gsensor_data->values[0], 
					&gsensor_data->values[1], &gsensor_data->values[2]);				
				gsensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;				
				gsensor_data->value_divide = 1000;
				//GSE_LOG("X :%d,Y: %d, Z: %d\n",gsensor_data->values[0],gsensor_data->values[1],gsensor_data->values[2]);
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
static int mma8453q_open(struct inode *inode, struct file *file)
{
	file->private_data = mma8453q_i2c_client;

	if(file->private_data == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return -EINVAL;
	}
	return nonseekable_open(inode, file);
}
/*----------------------------------------------------------------------------*/
static int mma8453q_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}
/*----------------------------------------------------------------------------*/
static int mma8453q_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
       unsigned long arg)
{
	struct i2c_client *client = (struct i2c_client*)file->private_data;
	struct mma8453q_i2c_data *obj = (struct mma8453q_i2c_data*)i2c_get_clientdata(client);	
	char strbuf[MMA8453Q_BUFSIZE];
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
			//GSE_LOG("fwq GSENSOR_IOCTL_INIT\n");
			MMA8453Q_Init(client, 0);			
			break;

		case GSENSOR_IOCTL_READ_CHIPINFO:
			//GSE_LOG("fwq GSENSOR_IOCTL_READ_CHIPINFO\n");
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			
			MMA8453Q_ReadChipInfo(client, strbuf, MMA8453Q_BUFSIZE);
			if(copy_to_user(data, strbuf, strlen(strbuf)+1))
			{
				err = -EFAULT;
				break;
			}				 
			break;	  

		case GSENSOR_IOCTL_READ_SENSORDATA:
			//GSE_LOG("fwq GSENSOR_IOCTL_READ_SENSORDATA\n");
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			
			MMA8453Q_ReadSensorData(client, strbuf, MMA8453Q_BUFSIZE);
			if(copy_to_user(data, strbuf, strlen(strbuf)+1))
			{
				err = -EFAULT;
				break;	  
			}				 
			break;

		case GSENSOR_IOCTL_READ_GAIN:
			//GSE_LOG("fwq GSENSOR_IOCTL_READ_GAIN\n");
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
			//GSE_LOG("fwq GSENSOR_IOCTL_READ_OFFSET\n");
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
			//GSE_LOG("fwq GSENSOR_IOCTL_READ_RAW_DATA\n");
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			MMA8453Q_ReadRawData(client, &strbuf);
			if(copy_to_user(data, &strbuf, strlen(strbuf)+1))
			{
				err = -EFAULT;
				break;	  
			}
			break;	  

		case GSENSOR_IOCTL_SET_CALI:
			//GSE_LOG("fwq GSENSOR_IOCTL_SET_CALI!!\n");
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
			    GSE_LOG("fwq going to set cali\n");
				cali[MMA8453Q_AXIS_X] = sensor_data.x * obj->reso->sensitivity / GRAVITY_EARTH_1000;
				cali[MMA8453Q_AXIS_Y] = sensor_data.y * obj->reso->sensitivity / GRAVITY_EARTH_1000;
				cali[MMA8453Q_AXIS_Z] = sensor_data.z * obj->reso->sensitivity / GRAVITY_EARTH_1000;			  
				err = MMA8453Q_WriteCalibration(client, cali);			 
			}
			break;

		case GSENSOR_IOCTL_CLR_CALI:
			//GSE_LOG("fwq GSENSOR_IOCTL_CLR_CALI!!\n");
			err = MMA8453Q_ResetCalibration(client);
			break;

		case GSENSOR_IOCTL_GET_CALI:
			//GSE_LOG("fwq GSENSOR_IOCTL_GET_CALI\n");
			data = (void __user*)arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			if(err = MMA8453Q_ReadCalibration(client, cali))
			{
				break;
			}
			
			sensor_data.x = cali[MMA8453Q_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
			sensor_data.y = cali[MMA8453Q_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
			sensor_data.z = cali[MMA8453Q_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
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
static struct file_operations mma8453q_fops = {
	.owner = THIS_MODULE,
	.open = mma8453q_open,
	.release = mma8453q_release,
	.ioctl = mma8453q_ioctl,
};
/*----------------------------------------------------------------------------*/
static struct miscdevice mma8453q_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "gsensor",
	.fops = &mma8453q_fops,
};
/*----------------------------------------------------------------------------*/
#ifndef CONFIG_HAS_EARLYSUSPEND
/*----------------------------------------------------------------------------*/
static int mma8453q_suspend(struct i2c_client *client, pm_message_t msg) 
{
	struct mma8453q_i2c_data *obj = i2c_get_clientdata(client);    
	int err = 0;
	u8  dat=0;
	GSE_FUN();    

	if(msg.event == PM_EVENT_SUSPEND)
	{   
		if(obj == NULL)
		{
			GSE_ERR("null pointer!!\n");
			return -EINVAL;
		}
		//read old data
		if ((err = hwmsen_read_byte_sr(client, MMA8453Q_REG_CTL_REG1, &dat))) 
		{
           GSE_ERR("read ctl_reg1  fail!!\n");
           return err;
        }
		dat = dat&0b11111110;//stand by mode
		atomic_set(&obj->suspend, 1);
		if(err = hwmsen_write_byte(client, MMA8453Q_REG_CTL_REG1, dat))
		{
			GSE_ERR("write power control fail!!\n");
			return err;
		}        
		MMA8453Q_power(obj->hw, 0);
	}
	return err;
}
/*----------------------------------------------------------------------------*/
static int mma8453q_resume(struct i2c_client *client)
{
	struct mma8453q_i2c_data *obj = i2c_get_clientdata(client);        
	int err;
	GSE_FUN();

	if(obj == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return -EINVAL;
	}

	MMA8453Q_power(obj->hw, 1);
	if(err = MMA8453Q_Init(client, 0))
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
static void mma8453q_early_suspend(struct early_suspend *h) 
{
	struct mma8453q_i2c_data *obj = container_of(h, struct mma8453q_i2c_data, early_drv);   
	int err;
	GSE_FUN();    

	if(obj == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return;
	}
	atomic_set(&obj->suspend, 1); 
	/*
	if(err = hwmsen_write_byte(obj->client, MMA8453Q_REG_POWER_CTL, 0x00))
	{
		GSE_ERR("write power control fail!!\n");
		return;
	}  
	*/
	if(err = MMA8453Q_SetPowerMode(obj->client, false))
	{
		GSE_ERR("write power control fail!!\n");
		return;
	}

	sensor_power = false;
	
	MMA8453Q_power(obj->hw, 0);
}
/*----------------------------------------------------------------------------*/
static void mma8453q_late_resume(struct early_suspend *h)
{
	struct mma8453q_i2c_data *obj = container_of(h, struct mma8453q_i2c_data, early_drv);         
	int err;
	GSE_FUN();

	if(obj == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return;
	}

	MMA8453Q_power(obj->hw, 1);
	if(err = MMA8453Q_Init(obj->client, 0))
	{
		GSE_ERR("initialize client fail!!\n");
		return;        
	}
	atomic_set(&obj->suspend, 0);    
}
/*----------------------------------------------------------------------------*/
#endif /*CONFIG_HAS_EARLYSUSPEND*/
/*----------------------------------------------------------------------------*/
static int mma8453q_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info) 
{    
	strcpy(info->type, MMA8453Q_DEV_NAME);
	return 0;
}

/*----------------------------------------------------------------------------*/
static int mma8453q_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_client *new_client;
	struct mma8453q_i2c_data *obj;
	struct hwmsen_object sobj;
	int err = 0;
	GSE_FUN();

	if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
	{
		err = -ENOMEM;
		goto exit;
	}
	
	memset(obj, 0, sizeof(struct mma8453q_i2c_data));

	obj->hw = mma8453q_get_cust_acc_hw();
	
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
	
#ifdef CONFIG_MMA8453Q_LOWPASS
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

	mma8453q_i2c_client = new_client;	

	if(err = MMA8453Q_Init(new_client, 1))
	{
		goto exit_init_failed;
	}
	

	if(err = misc_register(&mma8453q_device))
	{
		GSE_ERR("mma8453q_device register failed\n");
		goto exit_misc_device_register_failed;
	}

	if(err = mma8453q_create_attr(&(mma8453q_init_info.platform_diver_addr->driver)))
	{
		GSE_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}

	sobj.self = obj;
    sobj.polling = 1;
    sobj.sensor_operate = mma8453q_operate;
	if(err = hwmsen_attach(ID_ACCELEROMETER, &sobj))
	{
		GSE_ERR("attach fail = %d\n", err);
		goto exit_kfree;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	obj->early_drv.level    = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
	obj->early_drv.suspend  = mma8453q_early_suspend,
	obj->early_drv.resume   = mma8453q_late_resume,    
	register_early_suspend(&obj->early_drv);
#endif 

	GSE_LOG("%s: OK\n", __func__);
    mma8453q_init_flag = 0;
	return 0;

	exit_create_attr_failed:
	misc_deregister(&mma8453q_device);
	exit_misc_device_register_failed:
	exit_init_failed:
	//i2c_detach_client(new_client);
	exit_kfree:
	kfree(obj);
	exit:
	//
	//platform_driver_unregister(&mma8453q_gsensor_driver);
	GSE_ERR("%s: err = %d\n", __func__, err);    
	mma8453q_init_flag = -1;
	return err;
}

/*----------------------------------------------------------------------------*/
static int mma8453q_i2c_remove(struct i2c_client *client)
{
	int err = 0;	
	
	if(err = mma8453q_delete_attr(&(mma8453q_init_info.platform_diver_addr->driver)))
	{
		GSE_ERR("mma8453q_delete_attr fail: %d\n", err);
	}
	
	if(err = misc_deregister(&mma8453q_device))
	{
		GSE_ERR("misc_deregister fail: %d\n", err);
	}

	if(err = hwmsen_detach(ID_ACCELEROMETER))
	    

	mma8453q_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));
	return 0;
}
/*----------------------------------------------------------------------------*/
static int  mma8453q_local_init(void)
{
	struct acc_hw *hw = mma8453q_get_cust_acc_hw();
	GSE_FUN();

	MMA8453Q_power(hw, 1);
	mma8453q_force[0] = hw->i2c_num;
	if(i2c_add_driver(&mma8453q_i2c_driver))
	{
		GSE_ERR("add driver error\n");
		return -1;
	}
	if(-1 == mma8453q_init_flag)
	{
	   return -1;
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
static int mma8453q_remove(void)
{
    struct acc_hw *hw = mma8453q_get_cust_acc_hw();

    GSE_FUN();    
    MMA8453Q_power(hw, 0);    
    i2c_del_driver(&mma8453q_i2c_driver);
    return 0;
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static int __init mma8453q_init(void)
{
	GSE_FUN();
	hwmsen_gsensor_add(&mma8453q_init_info);
	return 0;    
}
/*----------------------------------------------------------------------------*/
static void __exit mma8453q_exit(void)
{
	GSE_FUN();
	
}
/*----------------------------------------------------------------------------*/
module_init(mma8453q_init);
module_exit(mma8453q_exit);
/*----------------------------------------------------------------------------*/
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MMA8453Q I2C driver");
MODULE_AUTHOR("Chunlei.Wang@mediatek.com");
