/* drivers/i2c/chips/mma7450l.c - MMA7450L motion sensor driver
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
#include "mma7450l.h"
#include <linux/hwmsen_helper.h>
/*----------------------------------------------------------------------------*/
#define I2C_DRIVERID_MMA7450L 345
/*----------------------------------------------------------------------------*/
#define DEBUG 1
/*----------------------------------------------------------------------------*/
#define CONFIG_MMA7450L_LOWPASS   /*apply low pass filter on output*/       
/*----------------------------------------------------------------------------*/
#define MMA7450L_AXIS_X          0
#define MMA7450L_AXIS_Y          1
#define MMA7450L_AXIS_Z          2
#define MMA7450L_AXES_NUM        3
#define MMA7450L_DATA_LEN        6
#define MMA7450L_DEV_NAME        "MMA7450L"
/*----------------------------------------------------------------------------*/
static const struct i2c_device_id mma7450l_i2c_id[] = {{MMA7450L_DEV_NAME,0},{}};
/*the adapter id will be available in customization*/
static struct i2c_board_info __initdata i2c_mma7450l={ I2C_BOARD_INFO("MMA7450L", MMA7450L_I2C_SLAVE_ADDR>>1)};

//static unsigned short mma7450l_force[] = {0x00, MMA7450L_I2C_SLAVE_ADDR, I2C_CLIENT_END, I2C_CLIENT_END};
//static const unsigned short *const mma7450l_forces[] = { mma7450l_force, NULL };
//static struct i2c_client_address_data mma7450l_addr_data = { .forces = mma7450l_forces,};

/*----------------------------------------------------------------------------*/
static int mma7450l_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id); 
static int mma7450l_i2c_remove(struct i2c_client *client);
static int mma7450l_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);

/*----------------------------------------------------------------------------*/
static int MMA7450L_SetPowerMode(struct i2c_client *client, bool enable);


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
    s16 raw[C_MAX_FIR_LENGTH][MMA7450L_AXES_NUM];
    int sum[MMA7450L_AXES_NUM];
    int num;
    int idx;
};
/*----------------------------------------------------------------------------*/
struct mma7450l_i2c_data {
    struct i2c_client *client;
    struct acc_hw *hw;
    struct hwmsen_convert   cvt;
    
    /*misc*/
    struct data_resolution *reso;
    atomic_t                trace;
    atomic_t                suspend;
    atomic_t                selftest;
	atomic_t				filter;
    s16                     cali_sw[MMA7450L_AXES_NUM+1];

    /*data*/
    s8                      offset[MMA7450L_AXES_NUM+1];  /*+1: for 4-byte alignment*/
    s16                     data[MMA7450L_AXES_NUM+1];

#if defined(CONFIG_MMA7450L_LOWPASS)
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
static struct i2c_driver mma7450l_i2c_driver = {
    .driver = {
        //.owner          = THIS_MODULE,
        .name           = MMA7450L_DEV_NAME,
    },
	.probe      		= mma7450l_i2c_probe,
	.remove    			= mma7450l_i2c_remove,
	.detect				= mma7450l_i2c_detect,
#if !defined(CONFIG_HAS_EARLYSUSPEND)    
    .suspend            = mma7450l_suspend,
    .resume             = mma7450l_resume,
#endif
	.id_table = mma7450l_i2c_id,
	//.address_data = &mma7450l_addr_data,
};

/*----------------------------------------------------------------------------*/
static struct i2c_client *mma7450l_i2c_client = NULL;
static struct platform_driver mma7450l_gsensor_driver;
static struct mma7450l_i2c_data *obj_i2c_data = NULL;
static bool sensor_power = false;
static GSENSOR_VECTOR3D gsensor_gain, gsensor_offset;
static char selftestRes[10] = {0};



/*----------------------------------------------------------------------------*/
#define GSE_TAG                  "[Gsensor] "
#define GSE_FUN(f)               printk(KERN_INFO GSE_TAG"%s\n", __FUNCTION__)
#define GSE_ERR(fmt, args...)    printk(KERN_ERR GSE_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define GSE_LOG(fmt, args...)    printk(KERN_INFO GSE_TAG fmt, ##args)
/*----------------------------------------------------------------------------*/
static struct data_resolution mma7450l_data_resolution[] = {
 /*8 combination by {FULL_RES,RANGE}*/
    {{15, 6},  64},   /*+/-8g  in 10-bit resolution: 15.6 mg/LSB*/
    {{ 15, 6}, 64},   /*+/-2g  in 8-bit resolution:  3.9 mg/LSB (full-resolution)*/
    {{ 31, 3}, 32},   /*+/-4g  in 8-bit resolution:  3.9 mg/LSB (full-resolution)*/
    {{ 62, 5}, 16},   /*+/-8g  in 8-bit resolution:  3.9 mg/LSB (full-resolution)*/            
};
/*----------------------------------------------------------------------------*/
static struct data_resolution mma7450l_offset_resolution = {{7, 8}, 128};


/*--------------------ADXL power control function----------------------------------*/

static int hwmsen_read_byte_sr(struct i2c_client *client, u8 addr, u8 *data)
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

static void dumpReg(struct i2c_client *client)
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

static void MMA7450L_power(struct acc_hw *hw, unsigned int on) 
{
	static unsigned int power_on = 0;

	if(hw->power_id != MT65XX_POWER_NONE)		// have externel LDO
	{        
		GSE_LOG("power %s\n", on ? "on" : "off");
		if(power_on == on)	// power status not change
		{
			GSE_LOG("ignore power control: %d\n", on);
		}
		else if(on)	// power on
		{
			if(!hwPowerOn(hw->power_id, hw->power_vol, "MMA7450L"))
			{
				GSE_ERR("power on fails!!\n");
			}
		}
		else	// power off
		{
			if (!hwPowerDown(hw->power_id, "MMA7450L"))
			{
				GSE_ERR("power off fail!!\n");
			}			  
		}
	}
	power_on = on;    
}
/*----------------------------------------------------------------------------*/
/*
//this function here use to set resolution and choose sensitivity
static int MMA7450L_SetDataResolution(struct i2c_client *client ,u8 dataresolution)
{
    GSE_LOG("fwq set resolution  dataresolution= %d!\n", dataresolution);
	int err;
	u8  dat, reso;
    u8 databuf[10];    
    int res = 0;
	struct mma7450l_i2c_data *obj = i2c_get_clientdata(client);

	if(hwmsen_read_byte_sr(client, MMA7450L_REG_CTL_REG2, databuf))
	{
		GSE_ERR("read power ctl register err!\n");
		return -1;
	}
	GSE_LOG("fwq read MMA7450L_REG_CTL_REG2 =%x in %s \n",databuf[0],__FUNCTION__);
	if(dataresolution == MMA7450L_10BIT_RES)
	{
		databuf[0] |= MMA7450L_10BIT_RES;
	}
	else
	{
		databuf[0] &= (~MMA7450L_10BIT_RES);//8 bit resolution
	}
	databuf[1] = databuf[0];
	databuf[0] = MMA7450L_REG_CTL_REG2;
	

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
	if(err = hwmsen_read_byte_sr(client, MMA7450L_REG_XYZ_DATA_CFG, &dat))
	{
		GSE_ERR("read detect range  fail!!\n");
		return err;
	}
	reso  = (dataresolution & MMA7450L_10BIT_RES) ? (0x00) : (0x03);
	
	
    if(dat & MMA7450L_RANGE_2G)
    {
      reso = reso + MMA7450L_RANGE_2G;
    }
	if(dat & MMA7450L_RANGE_4G)
    {
      reso = reso + MMA7450L_RANGE_4G;
    }
	if(dat & MMA7450L_RANGE_8G)
    {
      reso = reso + MMA7450L_RANGE_8G;
    }

	if(reso < sizeof(mma7450l_data_resolution)/sizeof(mma7450l_data_resolution[0]))
	{        
		obj->reso = &mma7450l_data_resolution[reso];
		GSE_LOG("reso=%x!! OK \n",reso);
		return 0;
	}
	else
	{   
	    GSE_ERR("choose sensitivity  fail!!\n");
		return -EINVAL;
	}
}
*/
/*----------------------------------------------------------------------------*/
static int MMA7450L_ReadData(struct i2c_client *client, s16 data[MMA7450L_AXES_NUM])
{
	struct mma7450l_i2c_data *priv = i2c_get_clientdata(client);        
	//u8 uData;
	u8 buf[MMA7450L_DATA_LEN] = {0};
	int err = 0;

	if(NULL == client)
	{
		err = -EINVAL;
	}
	else
		
	{
	  // hwmsen_read_block(client, addr, buf, 0x06);
       // dumpReg(client);
	/*
		buf[0] = XOUT8;
	    client->addr = client->addr& I2C_MASK_FLAG | I2C_WR_FLAG |I2C_RS_FLAG;
        i2c_master_send(client, (const char*)&buf, 3<<8 | 1);
	    client->addr = client->addr& I2C_MASK_FLAG;

*/
        if ((err = hwmsen_read_block(client, XOUT8, buf, 3))) 
        {
    	   GSE_ERR("error: %d\n", err);
    	   return err;
        }
		data[MMA7450L_AXIS_X] = (s8)(buf[MMA7450L_AXIS_X]);
	    data[MMA7450L_AXIS_Y] = (s8)(buf[MMA7450L_AXIS_Y]);
	    data[MMA7450L_AXIS_Z] = (s8)(buf[MMA7450L_AXIS_Z]);
	    

		if(atomic_read(&priv->trace) & ADX_TRC_REGXYZ)
		{
		    //hwmsen_read_byte(client, MCTL, &uData);
	       // GSE_LOG("  MCTL reg =%x\n",uData);
			GSE_LOG("raw from reg(SR) [%08X %08X %08X] => [%5d %5d %5d]\n", data[MMA7450L_AXIS_X], data[MMA7450L_AXIS_Y], data[MMA7450L_AXIS_Z],
		                               data[MMA7450L_AXIS_X], data[MMA7450L_AXIS_Y], data[MMA7450L_AXIS_Z]);
		}
/*
		if(atomic_read(&priv->trace) & ADX_TRC_RAWDATA)
		{
			GSE_LOG("raw >>6it:[%08X %08X %08X] => [%5d %5d %5d]\n", data[MMA7450L_AXIS_X], data[MMA7450L_AXIS_Y], data[MMA7450L_AXIS_Z],
		                               data[MMA7450L_AXIS_X], data[MMA7450L_AXIS_Y], data[MMA7450L_AXIS_Z]);
		}
		*/
#ifdef CONFIG_MMA7450L_LOWPASS
		if(atomic_read(&priv->filter))
		{
			if(atomic_read(&priv->fir_en) && !atomic_read(&priv->suspend))
			{
				int idx, firlen = atomic_read(&priv->firlen);   
				if(priv->fir.num < firlen)
				{                
					priv->fir.raw[priv->fir.num][MMA7450L_AXIS_X] = data[MMA7450L_AXIS_X];
					priv->fir.raw[priv->fir.num][MMA7450L_AXIS_Y] = data[MMA7450L_AXIS_Y];
					priv->fir.raw[priv->fir.num][MMA7450L_AXIS_Z] = data[MMA7450L_AXIS_Z];
					priv->fir.sum[MMA7450L_AXIS_X] += data[MMA7450L_AXIS_X];
					priv->fir.sum[MMA7450L_AXIS_Y] += data[MMA7450L_AXIS_Y];
					priv->fir.sum[MMA7450L_AXIS_Z] += data[MMA7450L_AXIS_Z];
					if(atomic_read(&priv->trace) & ADX_TRC_FILTER)
					{
						GSE_LOG("add [%2d] [%5d %5d %5d] => [%5d %5d %5d]\n", priv->fir.num,
							priv->fir.raw[priv->fir.num][MMA7450L_AXIS_X], priv->fir.raw[priv->fir.num][MMA7450L_AXIS_Y], priv->fir.raw[priv->fir.num][MMA7450L_AXIS_Z],
							priv->fir.sum[MMA7450L_AXIS_X], priv->fir.sum[MMA7450L_AXIS_Y], priv->fir.sum[MMA7450L_AXIS_Z]);
					}
					priv->fir.num++;
					priv->fir.idx++;
				}
				else
				{
					idx = priv->fir.idx % firlen;
					priv->fir.sum[MMA7450L_AXIS_X] -= priv->fir.raw[idx][MMA7450L_AXIS_X];
					priv->fir.sum[MMA7450L_AXIS_Y] -= priv->fir.raw[idx][MMA7450L_AXIS_Y];
					priv->fir.sum[MMA7450L_AXIS_Z] -= priv->fir.raw[idx][MMA7450L_AXIS_Z];
					priv->fir.raw[idx][MMA7450L_AXIS_X] = data[MMA7450L_AXIS_X];
					priv->fir.raw[idx][MMA7450L_AXIS_Y] = data[MMA7450L_AXIS_Y];
					priv->fir.raw[idx][MMA7450L_AXIS_Z] = data[MMA7450L_AXIS_Z];
					priv->fir.sum[MMA7450L_AXIS_X] += data[MMA7450L_AXIS_X];
					priv->fir.sum[MMA7450L_AXIS_Y] += data[MMA7450L_AXIS_Y];
					priv->fir.sum[MMA7450L_AXIS_Z] += data[MMA7450L_AXIS_Z];
					priv->fir.idx++;
					data[MMA7450L_AXIS_X] = priv->fir.sum[MMA7450L_AXIS_X]/firlen;
					data[MMA7450L_AXIS_Y] = priv->fir.sum[MMA7450L_AXIS_Y]/firlen;
					data[MMA7450L_AXIS_Z] = priv->fir.sum[MMA7450L_AXIS_Z]/firlen;
					if(atomic_read(&priv->trace) & ADX_TRC_FILTER)
					{
						GSE_LOG("add [%2d] [%5d %5d %5d] => [%5d %5d %5d] : [%5d %5d %5d]\n", idx,
						priv->fir.raw[idx][MMA7450L_AXIS_X], priv->fir.raw[idx][MMA7450L_AXIS_Y], priv->fir.raw[idx][MMA7450L_AXIS_Z],
						priv->fir.sum[MMA7450L_AXIS_X], priv->fir.sum[MMA7450L_AXIS_Y], priv->fir.sum[MMA7450L_AXIS_Z],
						data[MMA7450L_AXIS_X], data[MMA7450L_AXIS_Y], data[MMA7450L_AXIS_Z]);
					}
				}
			}
		}	
#endif         
	}
	return err;
}
/*----------------------------------------------------------------------------*/
static int MMA7450L_ReadOffset(struct i2c_client *client, s8 ofs[MMA7450L_AXES_NUM])
{    
	int err;
	u8 off_data[6];

    if ((err = hwmsen_read_block(client, XOFFL, off_data, 6))) 
    {
    	GSE_ERR("error: %d\n", err);
    	return err;
    }
	
	ofs[MMA7450L_AXIS_X] = ((off_data[1] & 0x07)<<8)| off_data[0];
	ofs[MMA7450L_AXIS_Y] = ((off_data[3] & 0x07)<<8)| off_data[2];
	ofs[MMA7450L_AXIS_Z] = ((off_data[5] & 0x07)<<8)| off_data[4];

	if(off_data[1] & 0x04)
	{
		ofs[MMA7450L_AXIS_X] = (2048 - ofs[MMA7450L_AXIS_X]) * (-1);
	}

	if(off_data[3] & 0x04)
	{
		ofs[MMA7450L_AXIS_Y] = (2048 - ofs[MMA7450L_AXIS_Y]) * (-1);
	}

	if(off_data[5] & 0x04)
	{
		ofs[MMA7450L_AXIS_Z] = (2048 - ofs[MMA7450L_AXIS_Z]) * (-1);
	}

	GSE_LOG("read offset: (%+3d %+3d %+3d %+3d %+3d %+3d): (%+3d %+3d %+3d)\n", 
		off_data[0],off_data[1],off_data[2],off_data[3],off_data[4],off_data[5],
		ofs[MMA7450L_AXIS_X],ofs[MMA7450L_AXIS_Y],ofs[MMA7450L_AXIS_Z]);

    return 0;  
}
/*----------------------------------------------------------------------------*/
static int MMA7450L_ResetCalibration(struct i2c_client *client)
{
	struct mma7450l_i2c_data *obj = i2c_get_clientdata(client);
    u8 ofs[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    int err;

    if ((err = hwmsen_write_block(client, XOFFL, ofs, 6))) 
        GSE_ERR("error: %d\n", err);
    memset(obj->cali_sw, 0x00, sizeof(obj->cali_sw));
    return err;    

    return 0;
}
/*----------------------------------------------------------------------------*/
static int MMA7450L_ReadCalibration(struct i2c_client *client, int dat[MMA7450L_AXES_NUM])
{
    struct mma7450l_i2c_data *obj = i2c_get_clientdata(client);
    int err;
    int mul;

    if ((err = MMA7450L_ReadOffset(client, obj->offset))) {
        GSE_ERR("read offset fail, %d\n", err);
        return err;
    }    
    
    mul = mma7450l_offset_resolution.sensitivity /obj->reso->sensitivity;

    dat[obj->cvt.map[MMA7450L_AXIS_X]] = obj->cvt.sign[MMA7450L_AXIS_X]*(obj->offset[MMA7450L_AXIS_X]/mul + obj->cali_sw[MMA7450L_AXIS_X]);
    dat[obj->cvt.map[MMA7450L_AXIS_Y]] = obj->cvt.sign[MMA7450L_AXIS_Y]*(obj->offset[MMA7450L_AXIS_Y]/mul + obj->cali_sw[MMA7450L_AXIS_Y]);
    dat[obj->cvt.map[MMA7450L_AXIS_Z]] = obj->cvt.sign[MMA7450L_AXIS_Z]*(obj->offset[MMA7450L_AXIS_Z]/mul + obj->cali_sw[MMA7450L_AXIS_Z]);                        
                                      
    return 0;
}
/*----------------------------------------------------------------------------*/
static int MMA7450L_ReadCalibrationEx(struct i2c_client *client, int act[MMA7450L_AXES_NUM], int raw[MMA7450L_AXES_NUM])
{  
	/*raw: the raw calibration data; act: the actual calibration data*/
	struct mma7450l_i2c_data *obj = i2c_get_clientdata(client);
    int err;
    int mul;

    if ((err = MMA7450L_ReadOffset(client, obj->offset))) {
        GSE_ERR("read offset fail, %d\n", err);
        return err;
    }    
    
    mul = mma7450l_offset_resolution.sensitivity/obj->reso->sensitivity;
    raw[MMA7450L_AXIS_X] = obj->offset[MMA7450L_AXIS_X]/mul + obj->cali_sw[MMA7450L_AXIS_X];
    raw[MMA7450L_AXIS_Y] = obj->offset[MMA7450L_AXIS_Y]/mul + obj->cali_sw[MMA7450L_AXIS_Y];
    raw[MMA7450L_AXIS_Z] = obj->offset[MMA7450L_AXIS_Z]/mul + obj->cali_sw[MMA7450L_AXIS_Z];
    
    act[obj->cvt.map[MMA7450L_AXIS_X]] = obj->cvt.sign[MMA7450L_AXIS_X]*raw[MMA7450L_AXIS_X];
    act[obj->cvt.map[MMA7450L_AXIS_Y]] = obj->cvt.sign[MMA7450L_AXIS_Y]*raw[MMA7450L_AXIS_Y];
    act[obj->cvt.map[MMA7450L_AXIS_Z]] = obj->cvt.sign[MMA7450L_AXIS_Z]*raw[MMA7450L_AXIS_Z];                        
                                     
    return 0;
}
/*----------------------------------------------------------------------------*/
static int MMA7450L_WriteCalibration(struct i2c_client *client, int dat[MMA7450L_AXES_NUM])
{
	struct mma7450l_i2c_data *obj = i2c_get_clientdata(client);
    int err;
    int cali[MMA7450L_AXES_NUM], raw[MMA7450L_AXES_NUM];
	u16 translate;
    int divisor = mma7450l_offset_resolution.sensitivity/obj->reso->sensitivity;
	u8 off_data[6];

    if ((err = MMA7450L_ReadCalibrationEx(client, cali, raw))) { /*offset will be updated in obj->offset*/
        GSE_ERR("read offset fail, %d\n", err);
        return err;
    }
    
    GSE_LOG("OLDOFF: (%+3d %+3d %+3d): (%+3d %+3d %+3d) / (%+3d %+3d %+3d)\n", 
            raw[MMA7450L_AXIS_X], raw[MMA7450L_AXIS_Y], raw[MMA7450L_AXIS_Z],
            obj->offset[MMA7450L_AXIS_X], obj->offset[MMA7450L_AXIS_Y], obj->offset[MMA7450L_AXIS_Z],
            obj->cali_sw[MMA7450L_AXIS_X], obj->cali_sw[MMA7450L_AXIS_Y], obj->cali_sw[MMA7450L_AXIS_Z]);
    
    /*calculate the real offset expected by caller*/
    cali[MMA7450L_AXIS_X] += dat[MMA7450L_AXIS_X];
    cali[MMA7450L_AXIS_Y] += dat[MMA7450L_AXIS_Y];
    cali[MMA7450L_AXIS_Z] += dat[MMA7450L_AXIS_Z];

    GSE_LOG("UPDATE: (%+3d %+3d %+3d)\n", 
            dat[MMA7450L_AXIS_X], dat[MMA7450L_AXIS_Y], dat[MMA7450L_AXIS_Z]);
    
    obj->offset[MMA7450L_AXIS_X] = (s16)(obj->cvt.sign[MMA7450L_AXIS_X]*(cali[obj->cvt.map[MMA7450L_AXIS_X]])*(divisor));
    obj->offset[MMA7450L_AXIS_Y] = (s16)(obj->cvt.sign[MMA7450L_AXIS_Y]*(cali[obj->cvt.map[MMA7450L_AXIS_Y]])*(divisor));
    obj->offset[MMA7450L_AXIS_Z] = (s16)(obj->cvt.sign[MMA7450L_AXIS_Z]*(cali[obj->cvt.map[MMA7450L_AXIS_Z]])*(divisor));
    /*convert software calibration using standard calibration*/
	/*
    obj->cali_sw[MMA7450L_AXIS_X] = obj->cvt.sign[MMA7450L_AXIS_X]*(cali[obj->cvt.map[MMA7450L_AXIS_X]])%(divisor);
    obj->cali_sw[MMA7450L_AXIS_Y] = obj->cvt.sign[MMA7450L_AXIS_Y]*(cali[obj->cvt.map[MMA7450L_AXIS_Y]])%(divisor);
    obj->cali_sw[MMA7450L_AXIS_Z] = obj->cvt.sign[MMA7450L_AXIS_Z]*(cali[obj->cvt.map[MMA7450L_AXIS_Z]])%(divisor);
    */
    obj->cali_sw[MMA7450L_AXIS_X] = 0;
    obj->cali_sw[MMA7450L_AXIS_Y] = 0;
    obj->cali_sw[MMA7450L_AXIS_Z] = 0;
    
    GSE_LOG("NEWOFF: (%+3d %+3d %+3d): (%+3d %+3d %+3d) / (%+3d %+3d %+3d)\n", 
            obj->offset[MMA7450L_AXIS_X]/divisor + obj->cali_sw[MMA7450L_AXIS_X], 
            obj->offset[MMA7450L_AXIS_Y]/divisor + obj->cali_sw[MMA7450L_AXIS_Y], 
            obj->offset[MMA7450L_AXIS_Z]/divisor + obj->cali_sw[MMA7450L_AXIS_Z], 
            obj->offset[MMA7450L_AXIS_X], obj->offset[MMA7450L_AXIS_Y], obj->offset[MMA7450L_AXIS_Z],
            obj->cali_sw[MMA7450L_AXIS_X], obj->cali_sw[MMA7450L_AXIS_Y], obj->cali_sw[MMA7450L_AXIS_Z]);

	GSE_LOG("1>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
	if((obj->offset[MMA7450L_AXIS_X] > 255) || (obj->offset[MMA7450L_AXIS_X] < -255 )
		|| (obj->offset[MMA7450L_AXIS_Y] > 255) || (obj->offset[MMA7450L_AXIS_Y] < -255 )
		|| (obj->offset[MMA7450L_AXIS_Z] > 255) || (obj->offset[MMA7450L_AXIS_Z] < -255 ))
	{
        GSE_ERR("offset value fail!\n");
        return -1;
    }
    GSE_LOG("2>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
	if(obj->offset[MMA7450L_AXIS_X] < 0)
	{
		translate = 2048+obj->offset[MMA7450L_AXIS_X];
		off_data[0] = (u8)translate & 0xff;
		off_data[1] = (u8)(translate>>8 & 0x07);
	}
	else
	{
		off_data[0] = obj->offset[MMA7450L_AXIS_X];
		off_data[1] = 0x00;
	}
    GSE_LOG("3>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
	if(obj->offset[MMA7450L_AXIS_Y] < 0)
	{
		translate = 2048+obj->offset[MMA7450L_AXIS_Y];
		off_data[2] = (u8)translate & 0xff;
		off_data[3] = (u8)(translate>>8 & 0x07);
	}
	else
	{
		off_data[2] = obj->offset[MMA7450L_AXIS_Y];
		off_data[3] = 0x00;
	}
    GSE_LOG("4>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
	if(obj->offset[MMA7450L_AXIS_Z] < 0)
	{
		translate = 2048+obj->offset[MMA7450L_AXIS_Z];
		off_data[4] = (u8)translate & 0xff;
		off_data[5] = (u8)(translate>>8 & 0x07);
	}
	else
	{
		off_data[4] = obj->offset[MMA7450L_AXIS_Z];
		off_data[5] = 0x00;
	}
    GSE_LOG("5>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
	if ((err = hwmsen_write_block(client, XOFFL, off_data, MMA7450L_AXES_NUM * 2))) 
	{
        GSE_ERR("write offset fail: %d\n", err);
        return err;
    }
	GSE_LOG("write cali ok!\n");

    return 0;
}
/*----------------------------------------------------------------------------*/
static int MMA7450L_CheckDeviceID(struct i2c_client *client)
{
#if 1
	u8 databuf[10];    
	int res = 0;

	memset(databuf, 0, sizeof(u8)*10);    
	databuf[0] = MMA7450L_REG_DEVID;    

	res = hwmsen_read_byte(client,MMA7450L_REG_DEVID,databuf);
    GSE_LOG("fwq mma7450l id %x!\n",databuf[0]);
	
	if(databuf[0]!=MMA7450L_REG_DEVID)
	{
		return MMA7450L_ERR_IDENTIFICATION;
	}

	if (res < 0)
	{
		return MMA7450L_ERR_I2C;
	}
	
#endif	
	return MMA7450L_SUCCESS;
}
/*----------------------------------------------------------------------------*/
//normal
//High resolution
//low noise low power
//low power

/*---------------------------------------------------------------------------*/
static int MMA7450L_SetPowerMode(struct i2c_client *client, bool enable)
{
	u8 databuf[2];    
	int res = 0;
	u8 addr = MCTL;
	struct mma7450l_i2c_data *obj = i2c_get_clientdata(client);
	

	if(enable == sensor_power)
	{
		GSE_LOG("Sensor power status need not to be set again!!!\n");
		return MMA7450L_SUCCESS;
	}

   
	if(hwmsen_read_byte_sr(client, addr, databuf))
	{
	    msleep(50);
		GSE_LOG("try again read power ctl register \n");
		if(hwmsen_read_byte_sr(client, addr, databuf))
		{
		  GSE_ERR("try again read power ctl register err!\n");
		  return MMA7450L_ERR_I2C;
		}
		
	}
	GSE_LOG("set power read MCTL =%x\n",databuf[0]);

	databuf[0] &= 0xFC;

	if(enable == TRUE)
	{
		databuf[0] |= 0x01;
	}
	else
	{
		// do nothing
	}
	databuf[1] = databuf[0];
	databuf[0] = MCTL;

	res = i2c_master_send(client, databuf, 0x2);

	if(res <= 0)
	{
		GSE_LOG("fwq set power mode failed!\n");
		return MMA7450L_ERR_I2C;
	}
	else if(atomic_read(&obj->trace) & ADX_TRC_INFO)
	{
		GSE_LOG("fwq set power mode ok %d!\n", databuf[1]);
	}

	sensor_power = enable;
	return MMA7450L_SUCCESS;    
}
/*----------------------------------------------------------------------------*/
//set detect range
/*

static int MMA7450L_SetDataFormat(struct i2c_client *client, u8 dataformat)
{
    
	struct mma7450l_i2c_data *obj = i2c_get_clientdata(client);
	u8 databuf[10];    
	int res = 0;

	memset(databuf, 0, sizeof(u8)*10);    
	databuf[0] = MMA7450L_REG_XYZ_DATA_CFG;    
	databuf[1] = dataformat;

	res = i2c_master_send(client, databuf, 0x2);

	if(res <= 0)
	{
		return MMA7450L_ERR_I2C;
	}

	return 0;

	//return MMA7450L_SetDataResolution(obj,dataformat);    
}
*/
/*----------------------------------------------------------------------------*/

static int MMA7450L_SetBWRate(struct i2c_client *client, u8 bwrate)
{
	u8 databuf[10];    
	int res = 0;

	memset(databuf, 0, sizeof(u8)*10);    
	databuf[0] = CTL1;    
	//databuf[1] = bwrate;
	
	if(hwmsen_read_byte_sr(client, CTL1, databuf))
	{
		GSE_ERR("read power ctl register err!\n");
		return MMA7450L_ERR_I2C;
	}
	GSE_LOG("fwq read MMA7450L_REG_CTL_REG1 =%x in %s \n",databuf[0],__FUNCTION__);

	databuf[0] &=0x7F;//clear original  data rate 
		
	databuf[0] |= bwrate; //set data rate
	databuf[1]= databuf[0];
	databuf[0]= CTL1;

	res = i2c_master_send(client, databuf, 0x2);

	if(res <= 0)
	{
		return MMA7450L_ERR_I2C;
	}
	
	return MMA7450L_SUCCESS;    
}

/*----------------------------------------------------------------------------*/
/*
static int MMA7450L_SetIntEnable(struct i2c_client *client, u8 intenable)
{
	u8 databuf[10];    
	int res = 0;

	memset(databuf, 0, sizeof(u8)*10);    
	databuf[0] = MMA7450L_REG_CTL_REG4;    
	databuf[1] = intenable;

	res = i2c_master_send(client, databuf, 0x2);

	if(res <= 0)
	{
		return MMA7450L_ERR_I2C;
	}
	
	return MMA7450L_SUCCESS;    
}
*/
/*----------------------------------------------------------------------------*/
static int MMA7450L_Init(struct i2c_client *client, int reset_cali)
{
	struct mma7450l_i2c_data *obj = i2c_get_clientdata(client);
	int res = 0;
	u8 uData = 0;
    GSE_LOG("mma7450l addr %x!\n",client->addr);

	MMA7450L_CheckDeviceID(client);

	uData = 0x44;
	res = hwmsen_write_byte(client,MCTL,uData);	// 2g measurement mode, data ready signal not output
    //								 b01000101
	//								  ||||||||
	//								  ||||||++- MODE: 00 - Standby
	//								  ||||||		  01 - Measurement
	//								  ||||||		  10 - Level Detection
	//								  ||||||		  11 - Pulse Detection
	//								  |||||++-- GLVL: 00 - 8 G
	//								  ||||			  10 - 4 G 
	//								  ||||			  01 - 2 G 
	//								  |||+----- STON: 0 - Self Test Disabled
	//								  ||+------ SPI3W:0 - 3-wire SPI
	//								  |+------- DRPD: 1 - Data Ready NOT Output to PIN1
	//								  +-------- Reserved: 0
	if(res != MMA7450L_SUCCESS ) 
	{
	    GSE_LOG("fwq mma7450l set MCTL mode error\n");
		return res;
	}
	MMA7450L_SetPowerMode(client,false);

	hwmsen_read_byte(client, MCTL, &uData);
	GSE_LOG("  MCTL reg =%x\n",uData);

	uData = 0x00;
    res = hwmsen_write_byte(client,CTL1,uData);	//b00000000
	//								  ||||||||
	//								  |||||||+- INTPIN: 0 - INT pins and register bits do not swap
	//								  |||||++-- INTREG[1:0]: 00 - INT1 for level detection, INT2 for pulse detection
	//								  ||||+---- XDA: 0 - X axis enabled for detection
	//								  |||+----- YDA: 0 - Y axis enabled for detection
	//								  ||+------ ZDA: 0 - Z axis enabled for detection
	//								  |+------- THOPT: 0 - Level detection threshold is absolute value
	//								  +-------- DFBW: 0 - Digital filter band width is 62.5Hz
	uData = 0x00;
	hwmsen_write_byte(client, CTL2, uData);	   //b00000000
	//								  |||
	//								  ||+- LDPL: 0 - Level detection polarity is positive
	//								  |+-- PDPL: 0 - Pulse detection polarity is positive
	//								  +--- DRVO: 0 - Standard drive strength on SDA/SDO pin
	uData = 0x00;
	hwmsen_write_byte(client, LDTH, uData);	   // Level detection threshold

	uData = 0x00;
	hwmsen_write_byte(client, PDTH, uData);	   // Pulse detection threshold

	uData = 0x00;
	hwmsen_write_byte(client, PW, uData);		   // Pulse Duration, 0.5mS per count

	uData = 0x00;
	hwmsen_write_byte(client, LT, uData);		   // Latency time, max 255mS

	uData = 0x00;
	hwmsen_write_byte(client, TW, uData);		   // Time Window for 2nd Pulse, max 255mS

	// Clear interrupt flags
	uData = 0x03;
	hwmsen_write_byte(client, INTRST, uData);

	uData = 0x00;
	hwmsen_write_byte(client, INTRST, uData);

	//GSE_LOG("fwq obj->reso->sensitivity =%d\n",obj->reso->sensitivity);
	//GSE_LOG("mma7450l_data_resolution[1].sensitivity =%d\n",mma7450l_data_resolution[1].sensitivity);
	GSE_LOG("fwq*****************************************\n");	
    obj->reso = &mma7450l_data_resolution[1];
	GSE_LOG("obj->reso->sensitivity =%d\n",obj->reso->sensitivity);	
	gsensor_gain.x = gsensor_gain.y = gsensor_gain.z = obj->reso->sensitivity;

  /*  
	if(NULL != reset_cali)
	{ 
		
		GSE_LOG("fwq mma7450l  set cali\n");
		res = MMA7450L_ResetCalibration(client);
		if(res != MMA7450L_SUCCESS)
		{
		    GSE_LOG("fwq mma7450l set cali error\n");
			return res;
		}
	}
*/
#ifdef CONFIG_MMA7450L_LOWPASS
	memset(&obj->fir, 0x00, sizeof(obj->fir));  
#endif
    GSE_LOG("fwq mma7450l Init OK\n");
	return MMA7450L_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int MMA7450L_ReadChipInfo(struct i2c_client *client, char *buf, int bufsize)
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

	sprintf(buf, "MMA7450L Chip");
	return 0;
}
/*----------------------------------------------------------------------------*/
static int MMA7450L_ReadSensorData(struct i2c_client *client, char *buf, int bufsize)
{
	struct mma7450l_i2c_data *obj = (struct mma7450l_i2c_data*)i2c_get_clientdata(client);
	u8 databuf[20];
	int acc[MMA7450L_AXES_NUM];
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

	if(sensor_power == false)
	{
		res = MMA7450L_SetPowerMode(client, true);
		if(res)
		{
			GSE_ERR("Power on mma7450l error %d!\n", res);
		}
		msleep(50);
	}

	if(res = MMA7450L_ReadData(client, obj->data))
	{        
		GSE_ERR("I2C error: ret value=%d", res);
		return -3;
	}
	else
	{
		obj->data[MMA7450L_AXIS_X] += obj->cali_sw[MMA7450L_AXIS_X];
		obj->data[MMA7450L_AXIS_Y] += obj->cali_sw[MMA7450L_AXIS_Y];
		obj->data[MMA7450L_AXIS_Z] += obj->cali_sw[MMA7450L_AXIS_Z];
		
		/*remap coordinate*/
		acc[obj->cvt.map[MMA7450L_AXIS_X]] = obj->cvt.sign[MMA7450L_AXIS_X]*obj->data[MMA7450L_AXIS_X];
		acc[obj->cvt.map[MMA7450L_AXIS_Y]] = obj->cvt.sign[MMA7450L_AXIS_Y]*obj->data[MMA7450L_AXIS_Y];
		acc[obj->cvt.map[MMA7450L_AXIS_Z]] = obj->cvt.sign[MMA7450L_AXIS_Z]*obj->data[MMA7450L_AXIS_Z];

		//GSE_LOG("Mapped gsensor data: %d, %d, %d!\n", acc[MMA7450L_AXIS_X], acc[MMA7450L_AXIS_Y], acc[MMA7450L_AXIS_Z]);

		//Out put the mg
		acc[MMA7450L_AXIS_X] = acc[MMA7450L_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		acc[MMA7450L_AXIS_Y] = acc[MMA7450L_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		acc[MMA7450L_AXIS_Z] = acc[MMA7450L_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;		
		

		sprintf(buf, "%04x %04x %04x", acc[MMA7450L_AXIS_X], acc[MMA7450L_AXIS_Y], acc[MMA7450L_AXIS_Z]);
		if(atomic_read(&obj->trace) & ADX_TRC_IOCTL)
		{
			GSE_LOG("gsensor data: %s!\n", buf);
			GSE_LOG("gsensor raw data: %d %d %d!\n", acc[obj->cvt.map[MMA7450L_AXIS_X]],acc[obj->cvt.map[MMA7450L_AXIS_Y]],acc[obj->cvt.map[MMA7450L_AXIS_Z]]);
			GSE_LOG("gsensor data:  sensitivity x=%d \n",gsensor_gain.z);
			 
		}
	}
	
	return 0;
}
/*----------------------------------------------------------------------------*/
static int MMA7450L_ReadRawData(struct i2c_client *client, char *buf)
{
	struct mma7450l_i2c_data *obj = (struct mma7450l_i2c_data*)i2c_get_clientdata(client);
	int res = 0;

	if (!buf || !client)
	{
		return EINVAL;
	}

	if(sensor_power == false)
	{
		res = MMA7450L_SetPowerMode(client, true);
		if(res)
		{
			GSE_ERR("Power on mma7450l error %d!\n", res);
		}
	}
	
	if(res = MMA7450L_ReadData(client, obj->data))
	{        
		GSE_ERR("I2C error: ret value=%d", res);
		return EIO;
	}
	else
	{
		sprintf(buf, "%04x %04x %04x", obj->data[MMA7450L_AXIS_X], 
			obj->data[MMA7450L_AXIS_Y], obj->data[MMA7450L_AXIS_Z]);
	
	}
	
	return 0;
}
/*----------------------------------------------------------------------------*/
static int MMA7450L_InitSelfTest(struct i2c_client *client)
{
	int res = 0;
	u8  data;
	u8 databuf[10]; 
    GSE_LOG("fwq init self test\n");
	#if 1

	res = MMA7450L_SetPowerMode(client,true);
	if(res != MMA7450L_SUCCESS ) //
	{
		return res;
	}
	
	res = MMA7450L_SetBWRate(client, MMA7450L_BW_125HZ);
	if(res != MMA7450L_SUCCESS ) //
	{
		return res;
	}	
	

	//set self test reg
	memset(databuf, 0, sizeof(u8)*10);    
	databuf[0] = MCTL;//set self test    
	if(hwmsen_read_byte_sr(client, MCTL, databuf))
	{
		GSE_ERR("read power ctl register err!\n");
		return MMA7450L_ERR_I2C;
	}

	databuf[0] &= 0xEF;//clear original    	
	databuf[0] |= 0x10; //set self test
	
	databuf[1]= databuf[0];
	databuf[0]= MCTL;

	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
	    GSE_LOG("fwq set selftest error\n");
		return MMA7450L_ERR_I2C;
	}
	
	GSE_LOG("fwq init self test OK\n");
#endif
	return MMA7450L_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int MMA7450L_JudgeTestResult(struct i2c_client *client, s32 prv[MMA7450L_AXES_NUM], s32 nxt[MMA7450L_AXES_NUM])
{
#if 1
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
        {{ 50,  540}, {-540,  -50},  {60, 68}},
    };
    struct criteria (*ptr)[3] = NULL;
    u8 detectRage;
	u8 tmp_resolution;
    int res=0;
	GSE_LOG("fwq judge test result\n");
	/*
    if(res = hwmsen_read_byte_sr(client, MMA7450L_REG_XYZ_DATA_CFG, &detectRage))
        return res;
	if(res = hwmsen_read_byte_sr(client, MMA7450L_REG_CTL_REG2, &tmp_resolution))
        return res;
*/
	//GSE_LOG("fwq tmp_resolution=%x , detectRage=%x\n",tmp_resolution,detectRage);
	/*
	if((tmp_resolution&MMA7450L_10BIT_RES) && (detectRage==0x00))
		ptr = &self[0];
	else if((tmp_resolution&MMA7450L_10BIT_RES) && (detectRage&MMA7450L_RANGE_4G))
	{
		ptr = &self[1];
		GSE_LOG("fwq self test choose ptr1\n");
	}
	else if((tmp_resolution&MMA7450L_10BIT_RES) && (detectRage&MMA7450L_RANGE_8G))
		ptr = &self[2];
	else if(detectRage&MMA7450L_RANGE_2G)//8 bit resolution
		ptr = &self[3];
	else if(detectRage&MMA7450L_RANGE_4G)//8 bit resolution
		ptr = &self[4];
	else if(detectRage&MMA7450L_RANGE_8G)//8 bit resolution
		ptr = &self[5];
	
*/
    ptr = &self[5];
    if (!ptr) {
        GSE_ERR("null pointer\n");
		GSE_LOG("fwq ptr null\n");
        return -EINVAL;
    }
/*
    if (((nxt[MMA7450L_AXIS_X] - prv[MMA7450L_AXIS_X]) > (*ptr)[MMA7450L_AXIS_X].max) ||
        ((nxt[MMA7450L_AXIS_X] - prv[MMA7450L_AXIS_X]) < (*ptr)[MMA7450L_AXIS_X].min)) {
        GSE_ERR("X is over range\n");
        res = -EINVAL;
    }
    if (((nxt[MMA7450L_AXIS_Y] - prv[MMA7450L_AXIS_Y]) > (*ptr)[MMA7450L_AXIS_Y].max) ||
        ((nxt[MMA7450L_AXIS_Y] - prv[MMA7450L_AXIS_Y]) < (*ptr)[MMA7450L_AXIS_Y].min)) {
        GSE_ERR("Y is over range\n");
        res = -EINVAL;
    }
	*/
    if (((nxt[MMA7450L_AXIS_Z] - prv[MMA7450L_AXIS_Z]) > (*ptr)[MMA7450L_AXIS_Z].max) ||
        ((nxt[MMA7450L_AXIS_Z] - prv[MMA7450L_AXIS_Z]) < (*ptr)[MMA7450L_AXIS_Z].min)) {
        GSE_ERR("Z is over range\n");
        res = -EINVAL;
    }
#endif
    return res;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
    GSE_LOG("fwq show_chipinfo_value \n");
	struct i2c_client *client = mma7450l_i2c_client;
	char strbuf[MMA7450L_BUFSIZE];
	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	
	MMA7450L_ReadChipInfo(client, strbuf, MMA7450L_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);        
}
/*----------------------------------------------------------------------------*/
static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = mma7450l_i2c_client;
	char strbuf[MMA7450L_BUFSIZE];
	
	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	MMA7450L_ReadSensorData(client, strbuf, MMA7450L_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);            
}
/*----------------------------------------------------------------------------*/
static ssize_t show_cali_value(struct device_driver *ddri, char *buf)
{
    GSE_LOG("fwq show_cali_value \n");
	struct i2c_client *client = mma7450l_i2c_client;
	struct mma7450l_i2c_data *obj;

	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}

	obj = i2c_get_clientdata(client);

	int err, len = 0, mul;
	int tmp[MMA7450L_AXES_NUM];

	if(err = MMA7450L_ReadOffset(client, obj->offset))
	{
		return -EINVAL;
	}
	else if(err = MMA7450L_ReadCalibration(client, tmp))
	{
		return -EINVAL;
	}
	else
	{    
		mul = obj->reso->sensitivity/mma7450l_offset_resolution.sensitivity;
		len += snprintf(buf+len, PAGE_SIZE-len, "[HW ][%d] (%+3d, %+3d, %+3d) : (0x%02X, 0x%02X, 0x%02X)\n", mul,                        
			obj->offset[MMA7450L_AXIS_X], obj->offset[MMA7450L_AXIS_Y], obj->offset[MMA7450L_AXIS_Z],
			obj->offset[MMA7450L_AXIS_X], obj->offset[MMA7450L_AXIS_Y], obj->offset[MMA7450L_AXIS_Z]);
		len += snprintf(buf+len, PAGE_SIZE-len, "[SW ][%d] (%+3d, %+3d, %+3d)\n", 1, 
			obj->cali_sw[MMA7450L_AXIS_X], obj->cali_sw[MMA7450L_AXIS_Y], obj->cali_sw[MMA7450L_AXIS_Z]);

		len += snprintf(buf+len, PAGE_SIZE-len, "[ALL]    (%+3d, %+3d, %+3d) : (%+3d, %+3d, %+3d)\n", 
			obj->offset[MMA7450L_AXIS_X]*mul + obj->cali_sw[MMA7450L_AXIS_X],
			obj->offset[MMA7450L_AXIS_Y]*mul + obj->cali_sw[MMA7450L_AXIS_Y],
			obj->offset[MMA7450L_AXIS_Z]*mul + obj->cali_sw[MMA7450L_AXIS_Z],
			tmp[MMA7450L_AXIS_X], tmp[MMA7450L_AXIS_Y], tmp[MMA7450L_AXIS_Z]);
		
		return len;
    }
}
/*----------------------------------------------------------------------------*/
static ssize_t store_cali_value(struct device_driver *ddri, char *buf, size_t count)
{
	struct i2c_client *client = mma7450l_i2c_client;  
	int err, x, y, z;
	int dat[MMA7450L_AXES_NUM];

	if(!strncmp(buf, "rst", 3))
	{
		if(err = MMA7450L_ResetCalibration(client))
		{
			GSE_ERR("reset offset err = %d\n", err);
		}	
	}
	else if(3 == sscanf(buf, "0x%02X 0x%02X 0x%02X", &x, &y, &z))
	{
		dat[MMA7450L_AXIS_X] = x;
		dat[MMA7450L_AXIS_Y] = y;
		dat[MMA7450L_AXIS_Z] = z;
		if(err = MMA7450L_WriteCalibration(client, dat))
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
	struct i2c_client *client = mma7450l_i2c_client;
	struct mma7450l_i2c_data *obj;
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
#if 1

	struct item{
	s16 raw[MMA7450L_AXES_NUM];
	};
	
	struct i2c_client *client = mma7450l_i2c_client;  
	struct mma7450l_i2c_data *obj = i2c_get_clientdata(client);
	int idx, res, num;
	struct item *prv = NULL, *nxt = NULL;
	s32 avg_prv[MMA7450L_AXES_NUM] = {0, 0, 0};
	s32 avg_nxt[MMA7450L_AXES_NUM] = {0, 0, 0};
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

	res = MMA7450L_SetPowerMode(client,true);
	if(res != MMA7450L_SUCCESS ) //
	{
		return res;
	}

	GSE_LOG("NORMAL:\n");
	msleep(20);
	for(idx = 0; idx < num; idx++)
	{
		if(res = MMA7450L_ReadData(client, prv[idx].raw))
		{            
			GSE_ERR("read data fail: %d\n", res);
			goto exit;
		}
		
		avg_prv[MMA7450L_AXIS_X] += prv[idx].raw[MMA7450L_AXIS_X];
		avg_prv[MMA7450L_AXIS_Y] += prv[idx].raw[MMA7450L_AXIS_Y];
		avg_prv[MMA7450L_AXIS_Z] += prv[idx].raw[MMA7450L_AXIS_Z];        
		GSE_LOG("[%5d %5d %5d]\n", prv[idx].raw[MMA7450L_AXIS_X], prv[idx].raw[MMA7450L_AXIS_Y], prv[idx].raw[MMA7450L_AXIS_Z]);
	}
	
	avg_prv[MMA7450L_AXIS_X] /= num;
	avg_prv[MMA7450L_AXIS_Y] /= num;
	avg_prv[MMA7450L_AXIS_Z] /= num; 

	res = MMA7450L_SetPowerMode(client,false);
	if(res != MMA7450L_SUCCESS ) //
	{
		return res;
	}

	/*initial setting for self test*/
	MMA7450L_InitSelfTest(client);
	msleep(20);
	GSE_LOG("SELFTEST:\n");  
/*
	MMA7450L_ReadData(client, nxt[0].raw);
	GSE_LOG("nxt[0].raw[MMA7450L_AXIS_X]: %d\n", nxt[0].raw[MMA7450L_AXIS_X]);
	GSE_LOG("nxt[0].raw[MMA7450L_AXIS_Y]: %d\n", nxt[0].raw[MMA7450L_AXIS_Y]);
	GSE_LOG("nxt[0].raw[MMA7450L_AXIS_Z]: %d\n", nxt[0].raw[MMA7450L_AXIS_Z]);
	*/
	for(idx = 0; idx < num; idx++)
	{
		if(res = MMA7450L_ReadData(client, nxt[idx].raw))
		{            
			GSE_ERR("read data fail: %d\n", res);
			goto exit;
		}
		avg_nxt[MMA7450L_AXIS_X] += nxt[idx].raw[MMA7450L_AXIS_X];
		avg_nxt[MMA7450L_AXIS_Y] += nxt[idx].raw[MMA7450L_AXIS_Y];
		avg_nxt[MMA7450L_AXIS_Z] += nxt[idx].raw[MMA7450L_AXIS_Z];        
		GSE_LOG("[%5d %5d %5d]\n", nxt[idx].raw[MMA7450L_AXIS_X], nxt[idx].raw[MMA7450L_AXIS_Y], nxt[idx].raw[MMA7450L_AXIS_Z]);
	}

	//softrestet
/*
	memset(databuf, 0, sizeof(u8)*10);    
	databuf[0] = MMA7450L_REG_CTL_REG2;//set self test    
	if(hwmsen_read_byte_sr(client, MMA7450L_REG_CTL_REG2, databuf))
	{
		GSE_ERR("read power ctl2 register err!\n");
		return MMA7450L_ERR_I2C;
	}

	databuf[0] &=~0x40;//clear original    	
	databuf[0] |= 0x40; 
	
	databuf[1]= databuf[0];
	databuf[0]= MMA7450L_REG_CTL_REG2;

	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
	    GSE_LOG("fwq softrest error\n");
		return MMA7450L_ERR_I2C;
	}

	// 
	*/
	MMA7450L_Init(client, 0);

	avg_nxt[MMA7450L_AXIS_X] /= num;
	avg_nxt[MMA7450L_AXIS_Y] /= num;
	avg_nxt[MMA7450L_AXIS_Z] /= num;    

	GSE_LOG("X: %5d - %5d = %5d \n", avg_nxt[MMA7450L_AXIS_X], avg_prv[MMA7450L_AXIS_X], avg_nxt[MMA7450L_AXIS_X] - avg_prv[MMA7450L_AXIS_X]);
	GSE_LOG("Y: %5d - %5d = %5d \n", avg_nxt[MMA7450L_AXIS_Y], avg_prv[MMA7450L_AXIS_Y], avg_nxt[MMA7450L_AXIS_Y] - avg_prv[MMA7450L_AXIS_Y]);
	GSE_LOG("Z: %5d - %5d = %5d \n", avg_nxt[MMA7450L_AXIS_Z], avg_prv[MMA7450L_AXIS_Z], avg_nxt[MMA7450L_AXIS_Z] - avg_prv[MMA7450L_AXIS_Z]); 

	if(!MMA7450L_JudgeTestResult(client, avg_prv, avg_nxt))
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
	MMA7450L_Init(client, 0);
	kfree(prv);
	kfree(nxt);
#endif	
	return count;
}
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
static ssize_t show_firlen_value(struct device_driver *ddri, char *buf)
{
    GSE_LOG("fwq show_firlen_value \n");
#ifdef CONFIG_MMA7450L_LOWPASS
	struct i2c_client *client = mma7450l_i2c_client;
	struct mma7450l_i2c_data *obj = i2c_get_clientdata(client);
	if(atomic_read(&obj->firlen))
	{
		int idx, len = atomic_read(&obj->firlen);
		GSE_LOG("len = %2d, idx = %2d\n", obj->fir.num, obj->fir.idx);

		for(idx = 0; idx < len; idx++)
		{
			GSE_LOG("[%5d %5d %5d]\n", obj->fir.raw[idx][MMA7450L_AXIS_X], obj->fir.raw[idx][MMA7450L_AXIS_Y], obj->fir.raw[idx][MMA7450L_AXIS_Z]);
		}
		
		GSE_LOG("sum = [%5d %5d %5d]\n", obj->fir.sum[MMA7450L_AXIS_X], obj->fir.sum[MMA7450L_AXIS_Y], obj->fir.sum[MMA7450L_AXIS_Z]);
		GSE_LOG("avg = [%5d %5d %5d]\n", obj->fir.sum[MMA7450L_AXIS_X]/len, obj->fir.sum[MMA7450L_AXIS_Y]/len, obj->fir.sum[MMA7450L_AXIS_Z]/len);
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
#ifdef CONFIG_MMA7450L_LOWPASS
	struct i2c_client *client = mma7450l_i2c_client;  
	struct mma7450l_i2c_data *obj = i2c_get_clientdata(client);
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
	struct mma7450l_i2c_data *obj = obj_i2c_data;
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
	struct mma7450l_i2c_data *obj = obj_i2c_data;
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
	struct mma7450l_i2c_data *obj = obj_i2c_data;
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

static ssize_t show_power_status(struct device_driver *ddri, char *buf)
{
	
	ssize_t res;
	u8 uData;
	struct mma7450l_i2c_data *obj = obj_i2c_data;
	if (obj == NULL)
	{
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}
	hwmsen_read_byte(obj->client, MCTL, &uData);
	
	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", uData);     
	return res;   
}


/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(chipinfo,             S_IRUGO, show_chipinfo_value,      NULL);
static DRIVER_ATTR(sensordata,           S_IRUGO, show_sensordata_value,    NULL);
static DRIVER_ATTR(cali,       S_IWUSR | S_IRUGO, show_cali_value,          store_cali_value);
static DRIVER_ATTR(selftest,       S_IWUSR | S_IRUGO, show_selftest_value,          store_selftest_value);
static DRIVER_ATTR(firlen,     S_IWUSR | S_IRUGO, show_firlen_value,        store_firlen_value);
static DRIVER_ATTR(trace,      S_IWUSR | S_IRUGO, show_trace_value,         store_trace_value);
static DRIVER_ATTR(status,               S_IRUGO, show_status_value,        NULL);
static DRIVER_ATTR(power,               S_IRUGO, show_power_status,        NULL);

/*----------------------------------------------------------------------------*/
static struct driver_attribute *mma7450l_attr_list[] = {
	&driver_attr_chipinfo,     /*chip information*/
	&driver_attr_sensordata,   /*dump sensor data*/
	&driver_attr_cali,         /*show calibration data*/
	&driver_attr_selftest,         /*self test demo*/
	&driver_attr_firlen,       /*filter length: 0: disable, others: enable*/
	&driver_attr_trace,        /*trace log*/
	&driver_attr_status,
	&driver_attr_power, 
};
/*----------------------------------------------------------------------------*/
static int mma7450l_create_attr(struct device_driver *driver) 
{
	int idx, err = 0;
	int num = (int)(sizeof(mma7450l_attr_list)/sizeof(mma7450l_attr_list[0]));
	if (driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
		if(err = driver_create_file(driver, mma7450l_attr_list[idx]))
		{            
			GSE_ERR("driver_create_file (%s) = %d\n", mma7450l_attr_list[idx]->attr.name, err);
			break;
		}
	}    
	return err;
}
/*----------------------------------------------------------------------------*/
static int mma7450l_delete_attr(struct device_driver *driver)
{
	int idx ,err = 0;
	int num = (int)(sizeof(mma7450l_attr_list)/sizeof(mma7450l_attr_list[0]));

	if(driver == NULL)
	{
		return -EINVAL;
	}
	

	for(idx = 0; idx < num; idx++)
	{
		driver_remove_file(driver, mma7450l_attr_list[idx]);
	}
	

	return err;
}

/*----------------------------------------------------------------------------*/
int gsensor_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value, sample_delay;	
	struct mma7450l_i2c_data *priv = (struct mma7450l_i2c_data*)self;
	hwm_sensor_data* gsensor_data;
	char buff[MMA7450L_BUFSIZE];
	
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
					sample_delay = MMA7450L_BW_250HZ;
				}
				else if(value <= 10)
				{
					sample_delay = MMA7450L_BW_125HZ;
				}
				else
				{
					sample_delay = MMA7450L_BW_125HZ;
				}
				
				err = MMA7450L_SetBWRate(priv->client, sample_delay); //err = MMA7450L_SetBWRate(priv->client, sample_delay);
				if(err != MMA7450L_SUCCESS ) //0x2C->BW=100Hz
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
					priv->fir.sum[MMA7450L_AXIS_X] = 0;
					priv->fir.sum[MMA7450L_AXIS_Y] = 0;
					priv->fir.sum[MMA7450L_AXIS_Z] = 0;
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
					err = MMA7450L_SetPowerMode( priv->client, !sensor_power);
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
				MMA7450L_ReadSensorData(priv->client, buff, MMA7450L_BUFSIZE);
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
static int mma7450l_open(struct inode *inode, struct file *file)
{
	file->private_data = mma7450l_i2c_client;

	if(file->private_data == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return -EINVAL;
	}
	return nonseekable_open(inode, file);
}
/*----------------------------------------------------------------------------*/
static int mma7450l_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}
/*----------------------------------------------------------------------------*/
//static int mma7450l_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
      // unsigned long arg)
static int mma7450l_unlocked_ioctl(struct file *file, unsigned int cmd,
       unsigned long arg)       
{
	struct i2c_client *client = (struct i2c_client*)file->private_data;
	struct mma7450l_i2c_data *obj = (struct mma7450l_i2c_data*)i2c_get_clientdata(client);	
	char strbuf[MMA7450L_BUFSIZE];
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
			GSE_LOG("fwq GSENSOR_IOCTL_INIT\n");
			MMA7450L_Init(client, 0);			
			break;

		case GSENSOR_IOCTL_READ_CHIPINFO:
			GSE_LOG("fwq GSENSOR_IOCTL_READ_CHIPINFO\n");
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			
			MMA7450L_ReadChipInfo(client, strbuf, MMA7450L_BUFSIZE);
			if(copy_to_user(data, strbuf, strlen(strbuf)+1))
			{
				err = -EFAULT;
				break;
			}				 
			break;	  

		case GSENSOR_IOCTL_READ_SENSORDATA:
			GSE_LOG("fwq GSENSOR_IOCTL_READ_SENSORDATA\n");
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			
			MMA7450L_ReadSensorData(client, strbuf, MMA7450L_BUFSIZE);
			if(copy_to_user(data, strbuf, strlen(strbuf)+1))
			{
				err = -EFAULT;
				break;	  
			}				 
			break;

		case GSENSOR_IOCTL_READ_GAIN:
			GSE_LOG("fwq GSENSOR_IOCTL_READ_GAIN\n");
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
			GSE_LOG("fwq GSENSOR_IOCTL_READ_OFFSET\n");
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
			GSE_LOG("fwq GSENSOR_IOCTL_READ_RAW_DATA\n");
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			MMA7450L_ReadRawData(client, &strbuf);
			if(copy_to_user(data, &strbuf, strlen(strbuf)+1))
			{
				err = -EFAULT;
				break;	  
			}
			break;	  

		case GSENSOR_IOCTL_SET_CALI:
			GSE_LOG("fwq GSENSOR_IOCTL_SET_CALI!!\n");
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
				cali[MMA7450L_AXIS_X] = sensor_data.x * obj->reso->sensitivity / GRAVITY_EARTH_1000;
				cali[MMA7450L_AXIS_Y] = sensor_data.y * obj->reso->sensitivity / GRAVITY_EARTH_1000;
				cali[MMA7450L_AXIS_Z] = sensor_data.z * obj->reso->sensitivity / GRAVITY_EARTH_1000;			  
				err = MMA7450L_WriteCalibration(client, cali);			 
			}
			break;

		case GSENSOR_IOCTL_CLR_CALI:
			GSE_LOG("fwq GSENSOR_IOCTL_CLR_CALI!!\n");
			err = MMA7450L_ResetCalibration(client);
			break;

		case GSENSOR_IOCTL_GET_CALI:
			GSE_LOG("fwq GSENSOR_IOCTL_GET_CALI\n");
			data = (void __user*)arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			if(err = MMA7450L_ReadCalibration(client, cali))
			{
				break;
			}
			
			sensor_data.x = cali[MMA7450L_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
			sensor_data.y = cali[MMA7450L_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
			sensor_data.z = cali[MMA7450L_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
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
static struct file_operations mma7450l_fops = {
	//.owner = THIS_MODULE,
	.open = mma7450l_open,
	.release = mma7450l_release,
	.unlocked_ioctl = mma7450l_unlocked_ioctl,
};
/*----------------------------------------------------------------------------*/
static struct miscdevice mma7450l_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "gsensor",
	.fops = &mma7450l_fops,
};
/*----------------------------------------------------------------------------*/
#ifndef CONFIG_HAS_EARLYSUSPEND
/*----------------------------------------------------------------------------*/
static int mma7450l_suspend(struct i2c_client *client, pm_message_t msg) 
{
	struct mma7450l_i2c_data *obj = i2c_get_clientdata(client);    
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
		
		atomic_set(&obj->suspend, 1);
		if ((err = MMA7450L_SetPowerMode(client,false){
            GSE_ERR("write power control fail!!\n");
            return err;
        }     
		MMA7450L_power(obj->hw, 0);
	}
	return err;
}
/*----------------------------------------------------------------------------*/
static int mma7450l_resume(struct i2c_client *client)
{
	struct mma7450l_i2c_data *obj = i2c_get_clientdata(client);        
	int err;
	GSE_FUN();

	if(obj == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return -EINVAL;
	}

	MMA7450L_power(obj->hw, 1);
	if(err = MMA7450L_Init(client, 0))
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
static void mma7450l_early_suspend(struct early_suspend *h) 
{
	struct mma7450l_i2c_data *obj = container_of(h, struct mma7450l_i2c_data, early_drv);   
	int err;
	GSE_FUN();    

	if(obj == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return;
	}
	atomic_set(&obj->suspend, 1); 
	/*
	if(err = hwmsen_write_byte(obj->client, MMA7450L_REG_POWER_CTL, 0x00))
	{
		GSE_ERR("write power control fail!!\n");
		return;
	}  
	*/
	if(err = MMA7450L_SetPowerMode(obj->client, false))
	{
		GSE_ERR("write power control fail!!\n");
		return;
	}

	sensor_power = false;
	
	MMA7450L_power(obj->hw, 0);
}
/*----------------------------------------------------------------------------*/
static void mma7450l_late_resume(struct early_suspend *h)
{
	struct mma7450l_i2c_data *obj = container_of(h, struct mma7450l_i2c_data, early_drv);         
	int err;
	GSE_FUN();

	if(obj == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return;
	}

	MMA7450L_power(obj->hw, 1);
	if(err = MMA7450L_Init(obj->client, 0))
	{
		GSE_ERR("initialize client fail!!\n");
		return;        
	}
	atomic_set(&obj->suspend, 0);    
}
/*----------------------------------------------------------------------------*/
#endif /*CONFIG_HAS_EARLYSUSPEND*/
/*----------------------------------------------------------------------------*/
static int mma7450l_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info) 
{    
	strcpy(info->type, MMA7450L_DEV_NAME);
	return 0;
}

/*----------------------------------------------------------------------------*/
static int mma7450l_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_client *new_client;
	struct mma7450l_i2c_data *obj;
	struct hwmsen_object sobj;
	int err = 0;
	GSE_FUN();

	if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
	{
		err = -ENOMEM;
		goto exit;
	}
	
	memset(obj, 0, sizeof(struct mma7450l_i2c_data));

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
	
#ifdef CONFIG_MMA7450L_LOWPASS
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

	mma7450l_i2c_client = new_client;	

	if(err = MMA7450L_Init(new_client, 1))
	{
		goto exit_init_failed;
	}
	

	if(err = misc_register(&mma7450l_device))
	{
		GSE_ERR("mma7450l_device register failed\n");
		goto exit_misc_device_register_failed;
	}

	if(err = mma7450l_create_attr(&mma7450l_gsensor_driver.driver))
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
	obj->early_drv.suspend  = mma7450l_early_suspend,
	obj->early_drv.resume   = mma7450l_late_resume,    
	register_early_suspend(&obj->early_drv);
#endif 

	GSE_LOG("%s: OK\n", __func__);    
	return 0;

	exit_create_attr_failed:
	misc_deregister(&mma7450l_device);
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
static int mma7450l_i2c_remove(struct i2c_client *client)
{
	int err = 0;	
	
	if(err = mma7450l_delete_attr(&mma7450l_gsensor_driver.driver))
	{
		GSE_ERR("mma7450l_delete_attr fail: %d\n", err);
	}
	
	if(err = misc_deregister(&mma7450l_device))
	{
		GSE_ERR("misc_deregister fail: %d\n", err);
	}

	if(err = hwmsen_detach(ID_ACCELEROMETER))
	    

	mma7450l_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));
	return 0;
}
/*----------------------------------------------------------------------------*/
static int mma7450l_probe(struct platform_device *pdev) 
{
	struct acc_hw *hw = get_cust_acc_hw();
	GSE_FUN();

	MMA7450L_power(hw, 1);
	//mma7450l_force[0] = hw->i2c_num;
	if(i2c_add_driver(&mma7450l_i2c_driver))
	{
		GSE_ERR("add driver error\n");
		return -1;
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
static int mma7450l_remove(struct platform_device *pdev)
{
    struct acc_hw *hw = get_cust_acc_hw();

    GSE_FUN();    
    MMA7450L_power(hw, 0);    
    i2c_del_driver(&mma7450l_i2c_driver);
    return 0;
}
/*----------------------------------------------------------------------------*/
static struct platform_driver mma7450l_gsensor_driver = {
	.probe      = mma7450l_probe,
	.remove     = mma7450l_remove,    
	.driver     = {
		.name  = "gsensor",
		//.owner = THIS_MODULE,
	}
};

/*----------------------------------------------------------------------------*/
static int __init mma7450l_init(void)
{
	GSE_FUN();
	struct acc_hw *hw = get_cust_acc_hw();
	GSE_LOG("%s: i2c_number=%d\n", __func__,hw->i2c_num); 
	i2c_register_board_info(hw->i2c_num, &i2c_mma7450l, 1);
	if(platform_driver_register(&mma7450l_gsensor_driver))
	{
		GSE_ERR("failed to register driver");
		return -ENODEV;
	}
	return 0;    
}
/*----------------------------------------------------------------------------*/
static void __exit mma7450l_exit(void)
{
	GSE_FUN();
	platform_driver_unregister(&mma7450l_gsensor_driver);
}
/*----------------------------------------------------------------------------*/
module_init(mma7450l_init);
module_exit(mma7450l_exit);
/*----------------------------------------------------------------------------*/
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MMA7450L I2C driver");
MODULE_AUTHOR("Chunlei.Wang@mediatek.com");
