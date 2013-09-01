/* drivers/i2c/chips/stk8311.c - STK8311 motion sensor driver
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
#include "stk8311.h"
#include <linux/hwmsen_helper.h>

#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>


#define POWER_NONE_MACRO MT65XX_POWER_NONE



/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
#define DEBUG 1
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
#define STK8311_AXIS_X          0
#define STK8311_AXIS_Y          1
#define STK8311_AXIS_Z          2
#define STK8311_AXES_NUM        3
#define STK8311_DATA_LEN        6
#define STK8311_DEV_NAME        "STK8311"
/*----------------------------------------------------------------------------*/
static const struct i2c_device_id stk8311_i2c_id[] = {{STK8311_DEV_NAME,0},{}};
/*the adapter id will be available in customization*/
static struct i2c_board_info __initdata i2c_stk8311={ I2C_BOARD_INFO("STK8311", STK8311_I2C_SLAVE_ADDR>>1)};

//static unsigned short stk8311_force[] = {0x00, STK8311_I2C_SLAVE_ADDR, I2C_CLIENT_END, I2C_CLIENT_END};
//static const unsigned short *const stk8311_forces[] = { stk8311_force, NULL };
//static struct i2c_client_address_data stk8311_addr_data = { .forces = stk8311_forces,};

/*----------------------------------------------------------------------------*/
static int stk8311_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id); 
static int stk8311_i2c_remove(struct i2c_client *client);
static int stk8311_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);

/*----------------------------------------------------------------------------*/
static int STK8311_SetPowerMode(struct i2c_client *client, bool enable);


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
    s16 raw[C_MAX_FIR_LENGTH][STK8311_AXES_NUM];
    int sum[STK8311_AXES_NUM];
    int num;
    int idx;
};
/*----------------------------------------------------------------------------*/
struct stk8311_i2c_data {
    struct i2c_client *client;
    struct acc_hw *hw;
    struct hwmsen_convert   cvt;
    
    /*misc*/
    struct data_resolution *reso;
    atomic_t                trace;
    atomic_t                suspend;
    atomic_t                selftest;
	atomic_t				filter;
    s16                     cali_sw[STK8311_AXES_NUM+1];

    /*data*/
    s8                      offset[STK8311_AXES_NUM+1];  /*+1: for 4-byte alignment*/
    s16                     data[STK8311_AXES_NUM+1];


    /*early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
    struct early_suspend    early_drv;
#endif     
};
/*----------------------------------------------------------------------------*/
static struct i2c_driver stk8311_i2c_driver = {
    .driver = {
       // .owner          = THIS_MODULE,
        .name           = STK8311_DEV_NAME,
    },
	.probe      		= stk8311_i2c_probe,
	.remove    			= stk8311_i2c_remove,
	.detect				= stk8311_i2c_detect,
#if !defined(CONFIG_HAS_EARLYSUSPEND)    
    .suspend            = stk8311_suspend,
    .resume             = stk8311_resume,
#endif
	.id_table = stk8311_i2c_id,
	//.address_data = &stk8311_addr_data,
};

/*----------------------------------------------------------------------------*/
static struct i2c_client *stk8311_i2c_client = NULL;
static struct platform_driver stk8311_gsensor_driver;
static struct stk8311_i2c_data *obj_i2c_data = NULL;
static bool sensor_power = false;
static GSENSOR_VECTOR3D gsensor_gain, gsensor_offset;
static char selftestRes[10] = {0};



/*----------------------------------------------------------------------------*/
#define GSE_TAG                  "[Gsensor] "
#define GSE_FUN(f)               printk(KERN_INFO GSE_TAG"%s\n", __FUNCTION__)
#define GSE_ERR(fmt, args...)    printk(KERN_ERR GSE_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define GSE_LOG(fmt, args...)    printk(KERN_INFO GSE_TAG fmt, ##args)
/*----------------------------------------------------------------------------*/
static struct data_resolution stk8311_data_resolution[] = {
 /*8 combination by {FULL_RES,RANGE}*/
    {{ 3, 9}, 256},   /*+/-2g  in 10-bit resolution:  3.9 mg/LSB*/
    {{ 3, 9}, 256},   /*+/-4g  in 11-bit resolution:  3.9 mg/LSB*/
    {{3, 9},  256},   /*+/-8g  in 12-bit resolution: 3.9 mg/LSB*/
    {{ 7, 8}, 128},   /*+/-16g  in 12-bit resolution:  7.8 mg/LSB (full-resolution)*/       
};
/*----------------------------------------------------------------------------*/
static struct data_resolution stk8311_offset_resolution = {{3, 9}, 256};

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
  
  addr = STK8311_REG_OFSX;
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

static void STK8311_power(struct acc_hw *hw, unsigned int on) 
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
			if(!hwPowerOn(hw->power_id, hw->power_vol, "STK8311"))
			{
				GSE_ERR("power on fails!!\n");
			}
		}
		else	// power off
		{
			if (!hwPowerDown(hw->power_id, "STK8311"))
			{
				GSE_ERR("power off fail!!\n");
			}			  
		}
	}
	power_on = on;    
}
/*----------------------------------------------------------------------------*/
//this function here use to set resolution and choose sensitivity
static int STK8311_SetDataResolution(struct i2c_client *client ,u8 dataresolution)
{
    GSE_LOG("fwq set resolution  dataresolution= %d!\n", dataresolution);
	int err;
	u8  dat, reso=0;
    u8 databuf[10];    
    int res = 0;
	struct stk8311_i2c_data *obj = i2c_get_clientdata(client);
	
    //choose sensitivity depend on resolution and detect range
	//read detect range
	if(err = hwmsen_read_byte_sr(client, STK8311_REG_XYZ_DATA_CFG, &dat))
	{
		GSE_ERR("read detect range  fail!!\n");
		return err;
	}	
	GSE_LOG("dat=%x!! OK \n",dat);
	dat = (dat&0xc0)>>6;
	GSE_LOG("dat=%x!! OK \n",dat);
	GSE_LOG("reso=%x!! OK \n",reso);
    if(dat & STK8311_RANGE_2G)
    {
      reso = reso + STK8311_RANGE_2G;
    }
	if(dat & STK8311_RANGE_4G)
    {
      reso = reso + STK8311_RANGE_4G;
    }
	if(dat & STK8311_RANGE_8G)
    {
      reso = reso + STK8311_RANGE_8G;
    }
    if(dat & STK8311_RANGE_16G)
    {
      reso = reso + STK8311_RANGE_16G;
    }
	if(reso < sizeof(stk8311_data_resolution)/sizeof(stk8311_data_resolution[0]))
	{        
		obj->reso = &stk8311_data_resolution[reso];
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
static int STK8311_ReadData(struct i2c_client *client, s16 data[STK8311_AXES_NUM])
{
	struct stk8311_i2c_data *priv = i2c_get_clientdata(client);        
	u8 addr = STK8311_REG_DATAX0;
	u8 buf[STK8311_DATA_LEN] = {0};
	int err = 0;

	if(NULL == client)
	{
		err = -EINVAL;
	}
	else
		
	{
	  // hwmsen_read_block(client, addr, buf, 0x06);
       // dumpReg(client);	    
		buf[0] = STK8311_REG_DATAX0;
	    client->addr = client->addr& I2C_MASK_FLAG | I2C_WR_FLAG |I2C_RS_FLAG;
        i2c_master_send(client, (const char*)&buf, 6<<8 | 1);
	    client->addr = client->addr& I2C_MASK_FLAG;


		data[STK8311_AXIS_X] = (s16)((buf[STK8311_AXIS_X*2] << 8) |
		         (buf[STK8311_AXIS_X*2+1]));
		data[STK8311_AXIS_Y] = (s16)((buf[STK8311_AXIS_Y*2] << 8) |
		         (buf[STK8311_AXIS_Y*2+1]));
		data[STK8311_AXIS_Z] = (s16)((buf[STK8311_AXIS_Z*2] << 8) |
		         (buf[STK8311_AXIS_Z*2+1]));
	    
		
		if(atomic_read(&priv->trace) & ADX_TRC_REGXYZ)
		{
			GSE_LOG("raw from reg(SR) [%08X %08X %08X] => [%5d %5d %5d]\n", data[STK8311_AXIS_X], data[STK8311_AXIS_Y], data[STK8311_AXIS_Z],
		                               data[STK8311_AXIS_X], data[STK8311_AXIS_Y], data[STK8311_AXIS_Z]);
		}
		//GSE_LOG("raw from reg(SR) [%08X %08X %08X] => [%5d %5d %5d]\n", data[MMA8452Q_AXIS_X], data[MMA8452Q_AXIS_Y], data[MMA8452Q_AXIS_Z],
		  //                             data[MMA8452Q_AXIS_X], data[MMA8452Q_AXIS_Y], data[MMA8452Q_AXIS_Z]);
		//add to fix data, refer to datasheet
		
		data[STK8311_AXIS_X] = data[STK8311_AXIS_X]>>4;
		data[STK8311_AXIS_Y] = data[STK8311_AXIS_Y]>>4;
		data[STK8311_AXIS_Z] = data[STK8311_AXIS_Z]>>4;
		
		data[STK8311_AXIS_X] += priv->cali_sw[STK8311_AXIS_X];
		data[STK8311_AXIS_Y] += priv->cali_sw[STK8311_AXIS_Y];
		data[STK8311_AXIS_Z] += priv->cali_sw[STK8311_AXIS_Z];

		if(atomic_read(&priv->trace) & ADX_TRC_RAWDATA)
		{
			GSE_LOG("raw >>6it:[%08X %08X %08X] => [%5d %5d %5d]\n", data[STK8311_AXIS_X], data[STK8311_AXIS_Y], data[STK8311_AXIS_Z],
		                               data[STK8311_AXIS_X], data[STK8311_AXIS_Y], data[STK8311_AXIS_Z]);
		}
		    
	}
	return err;
}
/*----------------------------------------------------------------------------*/
static int STK8311_ReadOffset(struct i2c_client *client, s8 ofs[STK8311_AXES_NUM])
{    
	int err;
    GSE_ERR("fwq read offset+: \n");
	if(err = hwmsen_read_byte_sr(client, STK8311_REG_OFSX, &ofs[STK8311_AXIS_X]))
	{
		GSE_ERR("error: %d\n", err);
	}
	if(err = hwmsen_read_byte_sr(client, STK8311_REG_OFSY, &ofs[STK8311_AXIS_Y]))
	{
		GSE_ERR("error: %d\n", err);
	}
	if(err = hwmsen_read_byte_sr(client, STK8311_REG_OFSZ, &ofs[STK8311_AXIS_Z]))
	{
		GSE_ERR("error: %d\n", err);
	}
	GSE_LOG("fwq read off:  offX=%x ,offY=%x ,offZ=%x\n",ofs[STK8311_AXIS_X],ofs[STK8311_AXIS_Y],ofs[STK8311_AXIS_Z]);
	
	return err;    
}
/*----------------------------------------------------------------------------*/
static int STK8311_ResetCalibration(struct i2c_client *client)
{
	struct stk8311_i2c_data *obj = i2c_get_clientdata(client);
	s8 ofs[STK8311_AXES_NUM] = {0x00, 0x00, 0x00};
	int err;

	//goto standby mode to clear cali
	STK8311_SetPowerMode(obj->client,false);
	if(err = hwmsen_write_block(client, STK8311_REG_OFSX, ofs, STK8311_AXES_NUM))
	{
		GSE_ERR("error: %d\n", err);
	}
    STK8311_SetPowerMode(obj->client,true);
	memset(obj->cali_sw, 0x00, sizeof(obj->cali_sw));
	return err;    
}
/*----------------------------------------------------------------------------*/
static int STK8311_ReadCalibration(struct i2c_client *client, int dat[STK8311_AXES_NUM])
{
    struct stk8311_i2c_data *obj = i2c_get_clientdata(client);
    int err;
    int mul;
    
    if ((err = STK8311_ReadOffset(client, obj->offset))) {
        GSE_ERR("read offset fail, %d\n", err);
        return err;
    }    
    
    //mul = obj->reso->sensitivity/mma8452q_offset_resolution.sensitivity;
    mul = stk8311_offset_resolution.sensitivity/obj->reso->sensitivity;
    dat[obj->cvt.map[STK8311_AXIS_X]] = obj->cvt.sign[STK8311_AXIS_X]*(obj->offset[STK8311_AXIS_X]/mul);
    dat[obj->cvt.map[STK8311_AXIS_Y]] = obj->cvt.sign[STK8311_AXIS_Y]*(obj->offset[STK8311_AXIS_Y]/mul);
    dat[obj->cvt.map[STK8311_AXIS_Z]] = obj->cvt.sign[STK8311_AXIS_Z]*(obj->offset[STK8311_AXIS_Z]/mul);                        
    GSE_LOG("fwq:read cali offX=%x ,offY=%x ,offZ=%x\n",obj->offset[STK8311_AXIS_X],obj->offset[STK8311_AXIS_Y],obj->offset[STK8311_AXIS_Z]);
	//GSE_LOG("fwq:read cali swX=%x ,swY=%x ,swZ=%x\n",obj->cali_sw[MMA8452Q_AXIS_X],obj->cali_sw[MMA8452Q_AXIS_Y],obj->cali_sw[MMA8452Q_AXIS_Z]);
    return 0;
}
/*----------------------------------------------------------------------------*/
static int STK8311_ReadCalibrationEx(struct i2c_client *client, int act[STK8311_AXES_NUM], int raw[STK8311_AXES_NUM])
{  
	/*raw: the raw calibration data; act: the actual calibration data*/
	struct stk8311_i2c_data *obj = i2c_get_clientdata(client);
	int err;
	int mul;

	if(err = STK8311_ReadOffset(client, obj->offset))
	{
		GSE_ERR("read offset fail, %d\n", err);
		return err;
	}    

	mul = stk8311_offset_resolution.sensitivity/obj->reso->sensitivity;
	raw[STK8311_AXIS_X] = obj->offset[STK8311_AXIS_X]/mul + obj->cali_sw[STK8311_AXIS_X];
	raw[STK8311_AXIS_Y] = obj->offset[STK8311_AXIS_Y]/mul + obj->cali_sw[STK8311_AXIS_Y];
	raw[STK8311_AXIS_Z] = obj->offset[STK8311_AXIS_Z]/mul + obj->cali_sw[STK8311_AXIS_Z];

	act[obj->cvt.map[STK8311_AXIS_X]] = obj->cvt.sign[STK8311_AXIS_X]*raw[STK8311_AXIS_X];
	act[obj->cvt.map[STK8311_AXIS_Y]] = obj->cvt.sign[STK8311_AXIS_Y]*raw[STK8311_AXIS_Y];
	act[obj->cvt.map[STK8311_AXIS_Z]] = obj->cvt.sign[STK8311_AXIS_Z]*raw[STK8311_AXIS_Z];                        
	                       
	return 0;
}
/*----------------------------------------------------------------------------*/
static int STK8311_WriteCalibration(struct i2c_client *client, int dat[STK8311_AXES_NUM])
{
	struct stk8311_i2c_data *obj = i2c_get_clientdata(client);
	u8 testdata=0;
	int err;
	int cali[STK8311_AXES_NUM], raw[STK8311_AXES_NUM];
	int lsb = stk8311_offset_resolution.sensitivity;
	u8 databuf[2]; 
	int res = 0;
	//int divisor = obj->reso->sensitivity/lsb;
	int divisor = lsb/obj->reso->sensitivity;
	GSE_LOG("fwq obj->reso->sensitivity=%d\n", obj->reso->sensitivity);
	GSE_LOG("fwq lsb=%d\n", lsb);
	

	if(err = STK8311_ReadCalibrationEx(client, cali, raw))	/*offset will be updated in obj->offset*/
	{ 
		GSE_ERR("read offset fail, %d\n", err);
		return err;
	}

	GSE_LOG("OLDOFF: (%+3d %+3d %+3d): (%+3d %+3d %+3d) / (%+3d %+3d %+3d)\n", 
		raw[STK8311_AXIS_X], raw[STK8311_AXIS_Y], raw[STK8311_AXIS_Z],
		obj->offset[STK8311_AXIS_X], obj->offset[STK8311_AXIS_Y], obj->offset[STK8311_AXIS_Z],
		obj->cali_sw[STK8311_AXIS_X], obj->cali_sw[STK8311_AXIS_Y], obj->cali_sw[STK8311_AXIS_Z]);

	/*calculate the real offset expected by caller*/
	cali[STK8311_AXIS_X] += dat[STK8311_AXIS_X];
	cali[STK8311_AXIS_Y] += dat[STK8311_AXIS_Y];
	cali[STK8311_AXIS_Z] += dat[STK8311_AXIS_Z];

	GSE_LOG("UPDATE: (%+3d %+3d %+3d)\n", 
		dat[STK8311_AXIS_X], dat[STK8311_AXIS_Y], dat[STK8311_AXIS_Z]);

	obj->offset[STK8311_AXIS_X] = (s8)(obj->cvt.sign[STK8311_AXIS_X]*(cali[obj->cvt.map[STK8311_AXIS_X]])*(divisor));
	obj->offset[STK8311_AXIS_Y] = (s8)(obj->cvt.sign[STK8311_AXIS_Y]*(cali[obj->cvt.map[STK8311_AXIS_Y]])*(divisor));
	obj->offset[STK8311_AXIS_Z] = (s8)(obj->cvt.sign[STK8311_AXIS_Z]*(cali[obj->cvt.map[STK8311_AXIS_Z]])*(divisor));

	/*convert software calibration using standard calibration*/
	obj->cali_sw[STK8311_AXIS_X] =0; //obj->cvt.sign[MMA8452Q_AXIS_X]*(cali[obj->cvt.map[MMA8452Q_AXIS_X]])%(divisor);
	obj->cali_sw[STK8311_AXIS_Y] =0; //obj->cvt.sign[MMA8452Q_AXIS_Y]*(cali[obj->cvt.map[MMA8452Q_AXIS_Y]])%(divisor);
	obj->cali_sw[STK8311_AXIS_Z] =0;// obj->cvt.sign[MMA8452Q_AXIS_Z]*(cali[obj->cvt.map[MMA8452Q_AXIS_Z]])%(divisor);

	GSE_LOG("NEWOFF: (%+3d %+3d %+3d): (%+3d %+3d %+3d) / (%+3d %+3d %+3d)\n", 
		obj->offset[STK8311_AXIS_X] + obj->cali_sw[STK8311_AXIS_X], 
		obj->offset[STK8311_AXIS_Y] + obj->cali_sw[STK8311_AXIS_Y], 
		obj->offset[STK8311_AXIS_Z] + obj->cali_sw[STK8311_AXIS_Z], 
		obj->offset[STK8311_AXIS_X], obj->offset[STK8311_AXIS_Y], obj->offset[STK8311_AXIS_Z],
		obj->cali_sw[STK8311_AXIS_X], obj->cali_sw[STK8311_AXIS_Y], obj->cali_sw[STK8311_AXIS_Z]);
	//
	//go to standby mode to set cali
    STK8311_SetPowerMode(obj->client,false);
	if(err = hwmsen_write_block(obj->client, STK8311_REG_OFSX, obj->offset, STK8311_AXES_NUM))
	{
		GSE_ERR("write offset fail: %d\n", err);
		return err;
	}
	STK8311_SetPowerMode(obj->client,true);
	
	//
	/*
	STK8311_SetPowerMode(obj->client,false);
	msleep(20);
	if(err = hwmsen_write_byte(obj->client, STK8311_REG_OFSX, obj->offset[STK8311_AXIS_X]))
	{
		GSE_ERR("write offset fail: %d\n", err);
		return err;
	}
    msleep(20);
	hwmsen_read_byte_sr(obj->client,STK8311_REG_OFSX,&testdata);
	GSE_LOG("write offsetX: %x\n", testdata);
	
	if(err = hwmsen_write_byte(obj->client, STK8311_REG_OFSY, obj->offset[STK8311_AXIS_Y]))
	{
		GSE_ERR("write offset fail: %d\n", err);
		return err;
	}
	msleep(20);
	hwmsen_read_byte_sr(obj->client,STK8311_REG_OFSY,&testdata);
	GSE_LOG("write offsetY: %x\n", testdata);
	
	if(err = hwmsen_write_byte(obj->client, STK8311_REG_OFSZ, obj->offset[STK8311_AXIS_Z]))
	{
		GSE_ERR("write offset fail: %d\n", err);
		return err;
	}
	msleep(20);
	hwmsen_read_byte_sr(obj->client,STK8311_REG_OFSZ,&testdata);
	GSE_LOG("write offsetZ: %x\n", testdata);
	STK8311_SetPowerMode(obj->client,true);
*/
	return err;
}
/*----------------------------------------------------------------------------*/
static int STK8311_CheckDeviceID(struct i2c_client *client)
{
	u8 databuf[10];    
	int res = 0;

	memset(databuf, 0, sizeof(u8)*10);    
	databuf[0] = STK8311_REG_DEVID;    

	res = hwmsen_read_byte_sr(client,STK8311_REG_DEVID,databuf);
    GSE_LOG("fwq stk8311 id %x!\n",databuf[0]);
	
	
	if(databuf[0]!=STK8311_FIXED_DEVID)
	{
		return STK8311_ERR_IDENTIFICATION;
	}

	exit_STK8311_CheckDeviceID:
	if (res < 0)
	{
		return STK8311_ERR_I2C;
	}
	
	return STK8311_SUCCESS;
}

static int STK8311_SetReset(struct i2c_client *client)
{
	u8 databuf[2];    
	int res = 0;
	u8 addr = STK8311_REG_RESET;
	struct stk8311_i2c_data *obj = i2c_get_clientdata(client);
	
	GSE_FUN();
		

	databuf[0] = 0;
	
	
	databuf[1] = databuf[0];
	databuf[0] = STK8311_REG_RESET;
	

	res = i2c_master_send(client, databuf, 0x2);

	if(res <= 0)
	{
		GSE_LOG("fwq set reset failed!\n");
		return STK8311_ERR_I2C;
	}
	else if(atomic_read(&obj->trace) & ADX_TRC_INFO)
	{
		GSE_LOG("fwq set reset ok %d!\n", databuf[1]);
	}

	
	return STK8311_SUCCESS;    
}

/*----------------------------------------------------------------------------*/
//normal
//High resolution
//low noise low power
//low power

/*---------------------------------------------------------------------------*/
static int STK8311_SetPowerMode(struct i2c_client *client, bool enable)
{
	u8 databuf[2];    
	int res = 0;
	u8 addr = STK8311_REG_MODE;
	struct stk8311_i2c_data *obj = i2c_get_clientdata(client);
	
	GSE_FUN();
	if(enable == sensor_power)
	{
		GSE_LOG("Sensor power status need not to be set again!!!\n");
		return STK8311_SUCCESS;
	}

	if(hwmsen_read_byte_sr(client, addr, databuf))
	{
		GSE_ERR("read power ctl register err!\n");
		return STK8311_ERR_I2C;
	}

	databuf[0] &= ~STK8311_MEASURE_MODE;
	
	if(enable == TRUE)
	{
		databuf[0] |= STK8311_MEASURE_MODE;
	}
	else
	{
		// do nothing
	}
	databuf[1] = databuf[0];
	databuf[0] = STK8311_REG_MODE;
	

	res = i2c_master_send(client, databuf, 0x2);

	if(res <= 0)
	{
		GSE_LOG("fwq set power mode failed!\n");
		return STK8311_ERR_I2C;
	}
	else if(atomic_read(&obj->trace) & ADX_TRC_INFO)
	{
		GSE_LOG("fwq set power mode ok %d!\n", databuf[1]);
	}

	sensor_power = enable;
	
	return STK8311_SUCCESS;    
}
/*----------------------------------------------------------------------------*/
//set detect range

static int STK8311_SetDataFormat(struct i2c_client *client, u8 dataformat)
{
    
	struct stk8311_i2c_data *obj = i2c_get_clientdata(client);
	u8 databuf[10];    
	int res = 0;

	memset(databuf, 0, sizeof(u8)*10);    
	databuf[0] = STK8311_REG_XYZ_DATA_CFG;    
	databuf[1] = (dataformat<<6)|0x02;

	res = i2c_master_send(client, databuf, 0x2);

	if(res <= 0)
	{
		return STK8311_ERR_I2C;
	}

	return 0;

	//return MMA8452Q_SetDataResolution(obj,dataformat);    
}

static int STK8311_SetIntEnable(struct i2c_client *client, u8 intenable)
{
	u8 databuf[10];    
	int res = 0;

	memset(databuf, 0, sizeof(u8)*10);    
	databuf[0] = STK8311_REG_INT;    
	databuf[1] = intenable;

	res = i2c_master_send(client, databuf, 0x2);

	if(res <= 0)
	{
		return STK8311_ERR_I2C;
	}
	
	return STK8311_SUCCESS;    
}
/*----------------------------------------------------------------------------*/
static int STK8311_Init(struct i2c_client *client, int reset_cali)
{
	struct stk8311_i2c_data *obj = i2c_get_clientdata(client);
	int res = 0;
    GSE_LOG("2010-11-03-11:43 fwq stk8311 addr %x!\n",client->addr);



	res = STK8311_SetReset(client);
	if(res != STK8311_SUCCESS)
	{
	    GSE_LOG("fwq stk8311 set reset error\n");
		return res;
	}
	res = STK8311_CheckDeviceID(client); 
	if(res != STK8311_SUCCESS)
	{
	    GSE_LOG("fwq stk8311 check id error\n");
		return res;
	}	

	
	

	
	res = STK8311_SetDataFormat(client, STK8311_RANGE_2G);
	if(res != STK8311_SUCCESS)
	{
	    GSE_LOG("fwq stk8311 set data format error\n");
		return res;
	}
	//add by fwq
	res = STK8311_SetDataResolution(client, STK8311_10BIT_RES);
	if(res != STK8311_SUCCESS) 
	{
	    GSE_LOG("fwq stk8311 set data reslution error\n");
		return res;
	}
	res = STK8311_SetPowerMode(client, false);
	if(res != STK8311_SUCCESS)
	{
	    GSE_LOG("fwq stk8311 set power error\n");
		return res;
	}
	gsensor_gain.x = gsensor_gain.y = gsensor_gain.z = obj->reso->sensitivity;
/*//we do not use interrupt
	res = STK8311_SetIntEnable(client, STK8311_DATA_READY);        
	if(res != STK8311_SUCCESS)//0x2E->0x80
	{
		return res;
	}
*/
    
	if(NULL != reset_cali)
	{ 
		/*reset calibration only in power on*/
		GSE_LOG("fwq stk8311  set cali\n");
		res = STK8311_ResetCalibration(client);
		if(res != STK8311_SUCCESS)
		{
		    GSE_LOG("fwq stk8311 set cali error\n");
			return res;
		}
	}


    GSE_LOG("fwq stk8311 Init OK\n");
	return STK8311_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int STK8311_ReadChipInfo(struct i2c_client *client, char *buf, int bufsize)
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

	sprintf(buf, "STK8311 Chip");
	return 0;
}
/*----------------------------------------------------------------------------*/
static int STK8311_ReadSensorData(struct i2c_client *client, char *buf, int bufsize)
{
	struct stk8311_i2c_data *obj = (struct stk8311_i2c_data*)i2c_get_clientdata(client);
	u8 databuf[20];
	int acc[STK8311_AXES_NUM];
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
		res = STK8311_SetPowerMode(client, true);
		if(res)
		{
			GSE_ERR("Power on stk8311 error %d!\n", res);
		}
	}

	if(res = STK8311_ReadData(client, obj->data))
	{        
		GSE_ERR("I2C error: ret value=%d", res);
		return -3;
	}
	else
	{
		obj->data[STK8311_AXIS_X] += obj->cali_sw[STK8311_AXIS_X];
		obj->data[STK8311_AXIS_Y] += obj->cali_sw[STK8311_AXIS_Y];
		obj->data[STK8311_AXIS_Z] += obj->cali_sw[STK8311_AXIS_Z];
		
		/*remap coordinate*/
		acc[obj->cvt.map[STK8311_AXIS_X]] = obj->cvt.sign[STK8311_AXIS_X]*obj->data[STK8311_AXIS_X];
		acc[obj->cvt.map[STK8311_AXIS_Y]] = obj->cvt.sign[STK8311_AXIS_Y]*obj->data[STK8311_AXIS_Y];
		acc[obj->cvt.map[STK8311_AXIS_Z]] = obj->cvt.sign[STK8311_AXIS_Z]*obj->data[STK8311_AXIS_Z];

		//GSE_LOG("Mapped gsensor data: %d, %d, %d!\n", acc[MMA8452Q_AXIS_X], acc[MMA8452Q_AXIS_Y], acc[MMA8452Q_AXIS_Z]);

		//Out put the mg
		acc[STK8311_AXIS_X] = acc[STK8311_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		acc[STK8311_AXIS_Y] = acc[STK8311_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		acc[STK8311_AXIS_Z] = acc[STK8311_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;		
		

		sprintf(buf, "%04x %04x %04x", acc[STK8311_AXIS_X], acc[STK8311_AXIS_Y], acc[STK8311_AXIS_Z]);
		if(atomic_read(&obj->trace) & ADX_TRC_IOCTL)
		{
			GSE_LOG("gsensor data: %s!\n", buf);
			GSE_LOG("gsensor data:  sensitivity x=%d \n",gsensor_gain.z);
			 
		}
	}
	
	return 0;
}
/*----------------------------------------------------------------------------*/
static int STK8311_ReadRawData(struct i2c_client *client, char *buf)
{
	struct stk8311_i2c_data *obj = (struct stk8311_i2c_data*)i2c_get_clientdata(client);
	int res = 0;

	if (!buf || !client)
	{
		return EINVAL;
	}

	if(sensor_power == FALSE)
	{
		res = STK8311_SetPowerMode(client, true);
		if(res)
		{
			GSE_ERR("Power on stk8311 error %d!\n", res);
		}
	}
	
	if(res = STK8311_ReadData(client, obj->data))
	{        
		GSE_ERR("I2C error: ret value=%d", res);
		return EIO;
	}
	else
	{
		sprintf(buf, "%04x %04x %04x", obj->data[STK8311_AXIS_X], 
			obj->data[STK8311_AXIS_Y], obj->data[STK8311_AXIS_Z]);
	
	}
	GSE_LOG("gsensor data: %s!\n", buf);
	return 0;
}
/*----------------------------------------------------------------------------*/
static int STK8311_InitSelfTest(struct i2c_client *client)
{
	int res = 0;
	u8  data;
	u8 databuf[10]; 
    GSE_LOG("fwq init self test\n");
/*
	res = STK8311_SetPowerMode(client,true);
	if(res != STK8311_SUCCESS ) //
	{
		return res;
	}
	*/
	
	
	res = STK8311_SetDataFormat(client, STK8311_RANGE_2G);
	if(res != STK8311_SUCCESS) //0x2C->BW=100Hz
	{
		return res;
	}
	res = STK8311_SetDataResolution(client, STK8311_10BIT_RES);
	if(res != STK8311_SUCCESS) 
	{
	    GSE_LOG("fwq stk8311 set data reslution error\n");
		return res;
	}

	//set self test reg
	memset(databuf, 0, sizeof(u8)*10);    
	databuf[0] = STK8311_REG_MODE;//set self test    
	if(hwmsen_read_byte_sr(client, STK8311_REG_MODE, databuf))
	{
		GSE_ERR("read power ctl register err!\n");
		return STK8311_ERR_I2C;
	}

	databuf[0] &=~0x02;//clear original    	
	databuf[0] |= 0x02; //set self test
	
	databuf[1]= databuf[0];
	databuf[0]= STK8311_REG_MODE;

	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
	    GSE_LOG("fwq set selftest error\n");
		return STK8311_ERR_I2C;
	}
	
	GSE_LOG("fwq init self test OK\n");
	return STK8311_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int STK8311_JudgeTestResult(struct i2c_client *client, s32 prv[STK8311_AXES_NUM], s32 nxt[STK8311_AXES_NUM])
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
    if(res = hwmsen_read_byte_sr(client, STK8311_REG_XYZ_DATA_CFG, &detectRage))
        return res;
	switch((detectRage&0xc0)>>6)
		{
		   case 0x00:
		   	    tmp_resolution = STK8311_10BIT_RES ;
				break;
		   case 0x01:
		   	    tmp_resolution = STK8311_11BIT_RES ;		
				break;
		   case 0x02:
		   	    tmp_resolution = STK8311_12BIT_RES ;		
				break;
		   case 0x03:
		   	    tmp_resolution = STK8311_12BIT_RES ;		
				break;
				default:
					tmp_resolution = STK8311_10BIT_RES ;
					GSE_LOG("fwq judge test detectRage read error !!! \n");
					break;
		}
	GSE_LOG("fwq tmp_resolution=%x , detectRage=%x\n",tmp_resolution,detectRage);
	detectRage = detectRage >>6;
	if((tmp_resolution&STK8311_12BIT_RES) && (detectRage==0x00))
		ptr = &self[0];
	else if((tmp_resolution&STK8311_12BIT_RES) && (detectRage==STK8311_RANGE_4G))
	{
		ptr = &self[1];
		GSE_LOG("fwq self test choose ptr1\n");
	}
	else if((tmp_resolution&STK8311_12BIT_RES) && (detectRage==STK8311_RANGE_8G))
		ptr = &self[2];
	else if(detectRage==STK8311_RANGE_2G)//8 bit resolution
		ptr = &self[3];
	else if(detectRage==STK8311_RANGE_4G)//8 bit resolution
		ptr = &self[4];
	else if(detectRage==STK8311_RANGE_8G)//8 bit resolution
		ptr = &self[5];
	

    if (!ptr) {
        GSE_ERR("null pointer\n");
		GSE_LOG("fwq ptr null\n");
        return -EINVAL;
    }

    if (((nxt[STK8311_AXIS_X] - prv[STK8311_AXIS_X]) > (*ptr)[STK8311_AXIS_X].max) ||
        ((nxt[STK8311_AXIS_X] - prv[STK8311_AXIS_X]) < (*ptr)[STK8311_AXIS_X].min)) {
        GSE_ERR("X is over range\n");
        res = -EINVAL;
    }
    if (((nxt[STK8311_AXIS_Y] - prv[STK8311_AXIS_Y]) > (*ptr)[STK8311_AXIS_Y].max) ||
        ((nxt[STK8311_AXIS_Y] - prv[STK8311_AXIS_Y]) < (*ptr)[STK8311_AXIS_Y].min)) {
        GSE_ERR("Y is over range\n");
        res = -EINVAL;
    }
    if (((nxt[STK8311_AXIS_Z] - prv[STK8311_AXIS_Z]) > (*ptr)[STK8311_AXIS_Z].max) ||
        ((nxt[STK8311_AXIS_Z] - prv[STK8311_AXIS_Z]) < (*ptr)[STK8311_AXIS_Z].min)) {
        GSE_ERR("Z is over range\n");
        res = -EINVAL;
    }
    return res;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
    GSE_LOG("fwq show_chipinfo_value \n");
	struct i2c_client *client = stk8311_i2c_client;
	char strbuf[STK8311_BUFSIZE];
	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	
	STK8311_ReadChipInfo(client, strbuf, STK8311_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);        
}
/*----------------------------------------------------------------------------*/
static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = stk8311_i2c_client;
	char strbuf[STK8311_BUFSIZE];
	
	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	STK8311_ReadSensorData(client, strbuf, STK8311_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);            
}
/*----------------------------------------------------------------------------*/
static ssize_t show_cali_value(struct device_driver *ddri, char *buf)
{
    GSE_LOG("fwq show_cali_value \n");
	struct i2c_client *client = stk8311_i2c_client;
	struct stk8311_i2c_data *obj;

	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}

	obj = i2c_get_clientdata(client);

	int err, len = 0, mul;
	int tmp[STK8311_AXES_NUM];

	if(err = STK8311_ReadOffset(client, obj->offset))
	{
		return -EINVAL;
	}
	else if(err = STK8311_ReadCalibration(client, tmp))
	{
		return -EINVAL;
	}
	else
	{    
		mul = obj->reso->sensitivity/stk8311_offset_resolution.sensitivity;
		len += snprintf(buf+len, PAGE_SIZE-len, "[HW ][%d] (%+3d, %+3d, %+3d) : (0x%02X, 0x%02X, 0x%02X)\n", mul,                        
			obj->offset[STK8311_AXIS_X], obj->offset[STK8311_AXIS_Y], obj->offset[STK8311_AXIS_Z],
			obj->offset[STK8311_AXIS_X], obj->offset[STK8311_AXIS_Y], obj->offset[STK8311_AXIS_Z]);
		len += snprintf(buf+len, PAGE_SIZE-len, "[SW ][%d] (%+3d, %+3d, %+3d)\n", 1, 
			obj->cali_sw[STK8311_AXIS_X], obj->cali_sw[STK8311_AXIS_Y], obj->cali_sw[STK8311_AXIS_Z]);

		len += snprintf(buf+len, PAGE_SIZE-len, "[ALL]    (%+3d, %+3d, %+3d) : (%+3d, %+3d, %+3d)\n", 
			obj->offset[STK8311_AXIS_X]*mul + obj->cali_sw[STK8311_AXIS_X],
			obj->offset[STK8311_AXIS_Y]*mul + obj->cali_sw[STK8311_AXIS_Y],
			obj->offset[STK8311_AXIS_Z]*mul + obj->cali_sw[STK8311_AXIS_Z],
			tmp[STK8311_AXIS_X], tmp[STK8311_AXIS_Y], tmp[STK8311_AXIS_Z]);
		
		return len;
    }
}
/*----------------------------------------------------------------------------*/
static ssize_t store_cali_value(struct device_driver *ddri, char *buf, size_t count)
{
	struct i2c_client *client = stk8311_i2c_client;  
	int err, x, y, z;
	int dat[STK8311_AXES_NUM];

	if(!strncmp(buf, "rst", 3))
	{
		if(err = STK8311_ResetCalibration(client))
		{
			GSE_ERR("reset offset err = %d\n", err);
		}	
	}
	else if(3 == sscanf(buf, "0x%02X 0x%02X 0x%02X", &x, &y, &z))
	{
		dat[STK8311_AXIS_X] = x;
		dat[STK8311_AXIS_Y] = y;
		dat[STK8311_AXIS_Z] = z;
		if(err = STK8311_WriteCalibration(client, dat))
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
	struct i2c_client *client = stk8311_i2c_client;
	struct stk8311_i2c_data *obj;
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
	s16 raw[STK8311_AXES_NUM];
	};
	
	struct i2c_client *client = stk8311_i2c_client;  
	struct stk8311_i2c_data *obj = i2c_get_clientdata(client);
	int idx, res, num;
	struct item *prv = NULL, *nxt = NULL;
	s32 avg_prv[STK8311_AXES_NUM] = {0, 0, 0};
	s32 avg_nxt[STK8311_AXES_NUM] = {0, 0, 0};
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

	res = STK8311_SetPowerMode(client,true);
	if(res != STK8311_SUCCESS ) //
	{
		return res;
	}

	GSE_LOG("NORMAL:\n");
	for(idx = 0; idx < num; idx++)
	{
		if(res = STK8311_ReadData(client, prv[idx].raw))
		{            
			GSE_ERR("read data fail: %d\n", res);
			goto exit;
		}
		
		avg_prv[STK8311_AXIS_X] += prv[idx].raw[STK8311_AXIS_X];
		avg_prv[STK8311_AXIS_Y] += prv[idx].raw[STK8311_AXIS_Y];
		avg_prv[STK8311_AXIS_Z] += prv[idx].raw[STK8311_AXIS_Z];        
		GSE_LOG("[%5d %5d %5d]\n", prv[idx].raw[STK8311_AXIS_X], prv[idx].raw[STK8311_AXIS_Y], prv[idx].raw[STK8311_AXIS_Z]);
	}
	
	avg_prv[STK8311_AXIS_X] /= num;
	avg_prv[STK8311_AXIS_Y] /= num;
	avg_prv[STK8311_AXIS_Z] /= num; 

	res = STK8311_SetPowerMode(client,false);
	if(res != STK8311_SUCCESS ) //
	{
		return res;
	}

	/*initial setting for self test*/
	STK8311_InitSelfTest(client);
	GSE_LOG("SELFTEST:\n");  
/*
	STK8311_ReadData(client, nxt[0].raw);
	GSE_LOG("nxt[0].raw[STK8311_AXIS_X]: %d\n", nxt[0].raw[STK8311_AXIS_X]);
	GSE_LOG("nxt[0].raw[STK8311_AXIS_Y]: %d\n", nxt[0].raw[STK8311_AXIS_Y]);
	GSE_LOG("nxt[0].raw[STK8311_AXIS_Z]: %d\n", nxt[0].raw[STK8311_AXIS_Z]);
	*/
	for(idx = 0; idx < num; idx++)
	{
		if(res = STK8311_ReadData(client, nxt[idx].raw))
		{            
			GSE_ERR("read data fail: %d\n", res);
			goto exit;
		}
		avg_nxt[STK8311_AXIS_X] += nxt[idx].raw[STK8311_AXIS_X];
		avg_nxt[STK8311_AXIS_Y] += nxt[idx].raw[STK8311_AXIS_Y];
		avg_nxt[STK8311_AXIS_Z] += nxt[idx].raw[STK8311_AXIS_Z];        
		GSE_LOG("[%5d %5d %5d]\n", nxt[idx].raw[STK8311_AXIS_X], nxt[idx].raw[STK8311_AXIS_Y], nxt[idx].raw[STK8311_AXIS_Z]);
	}

	//softrestet

	
	// 
	STK8311_Init(client, 0);

	avg_nxt[STK8311_AXIS_X] /= num;
	avg_nxt[STK8311_AXIS_Y] /= num;
	avg_nxt[STK8311_AXIS_Z] /= num;    

	GSE_LOG("X: %5d - %5d = %5d \n", avg_nxt[STK8311_AXIS_X], avg_prv[STK8311_AXIS_X], avg_nxt[STK8311_AXIS_X] - avg_prv[STK8311_AXIS_X]);
	GSE_LOG("Y: %5d - %5d = %5d \n", avg_nxt[STK8311_AXIS_Y], avg_prv[STK8311_AXIS_Y], avg_nxt[STK8311_AXIS_Y] - avg_prv[STK8311_AXIS_Y]);
	GSE_LOG("Z: %5d - %5d = %5d \n", avg_nxt[STK8311_AXIS_Z], avg_prv[STK8311_AXIS_Z], avg_nxt[STK8311_AXIS_Z] - avg_prv[STK8311_AXIS_Z]); 

	if(!STK8311_JudgeTestResult(client, avg_prv, avg_nxt))
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
	STK8311_Init(client, 0);
	kfree(prv);
	kfree(nxt);
	return count;
}
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
static ssize_t show_firlen_value(struct device_driver *ddri, char *buf)
{
    GSE_LOG("fwq show_firlen_value \n");


	return snprintf(buf, PAGE_SIZE, "not support\n");

}
/*----------------------------------------------------------------------------*/
static ssize_t store_firlen_value(struct device_driver *ddri, char *buf, size_t count)
{
    GSE_LOG("fwq store_firlen_value \n");

	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_trace_value(struct device_driver *ddri, char *buf)
{
    GSE_LOG("fwq show_trace_value \n");
	ssize_t res;
	struct stk8311_i2c_data *obj = obj_i2c_data;
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
	struct stk8311_i2c_data *obj = obj_i2c_data;
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
	struct stk8311_i2c_data *obj = obj_i2c_data;
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
static struct driver_attribute *stk8311_attr_list[] = {
	&driver_attr_chipinfo,     /*chip information*/
	&driver_attr_sensordata,   /*dump sensor data*/
	&driver_attr_cali,         /*show calibration data*/
	&driver_attr_selftest,         /*self test demo*/
	&driver_attr_firlen,       /*filter length: 0: disable, others: enable*/
	&driver_attr_trace,        /*trace log*/
	&driver_attr_status,        
};
/*----------------------------------------------------------------------------*/
static int stk8311_create_attr(struct device_driver *driver) 
{
	int idx, err = 0;
	int num = (int)(sizeof(stk8311_attr_list)/sizeof(stk8311_attr_list[0]));
	if (driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
		if(err = driver_create_file(driver, stk8311_attr_list[idx]))
		{            
			GSE_ERR("driver_create_file (%s) = %d\n", stk8311_attr_list[idx]->attr.name, err);
			break;
		}
	}    
	return err;
}
/*----------------------------------------------------------------------------*/
static int stk8311_delete_attr(struct device_driver *driver)
{
	int idx ,err = 0;
	int num = (int)(sizeof(stk8311_attr_list)/sizeof(stk8311_attr_list[0]));

	if(driver == NULL)
	{
		return -EINVAL;
	}
	

	for(idx = 0; idx < num; idx++)
	{
		driver_remove_file(driver, stk8311_attr_list[idx]);
	}
	

	return err;
}

/*----------------------------------------------------------------------------*/
int gsensor_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value, sample_delay;	
	struct stk8311_i2c_data *priv = (struct stk8311_i2c_data*)self;
	hwm_sensor_data* gsensor_data;
	char buff[STK8311_BUFSIZE];
	
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
					//sample_delay = MMA8452Q_BW_200HZ;
				}
				else if(value <= 10)
				{
					//sample_delay = MMA8452Q_BW_100HZ;
				}
				else
				{
					//sample_delay = MMA8452Q_BW_50HZ;
				}
				
				//err = MMA8452Q_SetBWRate(priv->client, MMA8452Q_BW_100HZ); //err = MMA8452Q_SetBWRate(priv->client, sample_delay);
				if(err != STK8311_SUCCESS ) //0x2C->BW=100Hz
				{
					GSE_ERR("Set delay parameter error!\n");
				}

				if(value >= 50)
				{
					atomic_set(&priv->filter, 0);
				}
				else
				{					
					
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
					err = STK8311_SetPowerMode( priv->client, !sensor_power);
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
				STK8311_ReadSensorData(priv->client, buff, STK8311_BUFSIZE);
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
static int stk8311_open(struct inode *inode, struct file *file)
{
	file->private_data = stk8311_i2c_client;

	if(file->private_data == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return -EINVAL;
	}
	return nonseekable_open(inode, file);
}
/*----------------------------------------------------------------------------*/
static int stk8311_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}
/*----------------------------------------------------------------------------*/
//static int stk8311_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
  //     unsigned long arg)
static int stk8311_unlocked_ioctl(struct file *file, unsigned int cmd,
       unsigned long arg)  
{
	struct i2c_client *client = (struct i2c_client*)file->private_data;
	struct stk8311_i2c_data *obj = (struct stk8311_i2c_data*)i2c_get_clientdata(client);	
	char strbuf[STK8311_BUFSIZE];
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
			STK8311_Init(client, 0);			
			break;

		case GSENSOR_IOCTL_READ_CHIPINFO:
			//GSE_LOG("fwq GSENSOR_IOCTL_READ_CHIPINFO\n");
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			
			STK8311_ReadChipInfo(client, strbuf, STK8311_BUFSIZE);
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
			
			STK8311_ReadSensorData(client, strbuf, STK8311_BUFSIZE);
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
			STK8311_ReadRawData(client, &strbuf);
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
				cali[STK8311_AXIS_X] = sensor_data.x * obj->reso->sensitivity / GRAVITY_EARTH_1000;
				cali[STK8311_AXIS_Y] = sensor_data.y * obj->reso->sensitivity / GRAVITY_EARTH_1000;
				cali[STK8311_AXIS_Z] = sensor_data.z * obj->reso->sensitivity / GRAVITY_EARTH_1000;			  
				err = STK8311_WriteCalibration(client, cali);			 
			}
			break;

		case GSENSOR_IOCTL_CLR_CALI:
			//GSE_LOG("fwq GSENSOR_IOCTL_CLR_CALI!!\n");
			err = STK8311_ResetCalibration(client);
			break;

		case GSENSOR_IOCTL_GET_CALI:
			//GSE_LOG("fwq GSENSOR_IOCTL_GET_CALI\n");
			data = (void __user*)arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			if(err = STK8311_ReadCalibration(client, cali))
			{
				break;
			}
			
			sensor_data.x = cali[STK8311_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
			sensor_data.y = cali[STK8311_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
			sensor_data.z = cali[STK8311_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
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
static struct file_operations stk8311_fops = {
	//.owner = THIS_MODULE,
	.open = stk8311_open,
	.release = stk8311_release,
	.unlocked_ioctl = stk8311_unlocked_ioctl,
};
/*----------------------------------------------------------------------------*/
static struct miscdevice stk8311_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "gsensor",
	.fops = &stk8311_fops,
};
/*----------------------------------------------------------------------------*/
#ifndef CONFIG_HAS_EARLYSUSPEND
/*----------------------------------------------------------------------------*/
static int stk8311_suspend(struct i2c_client *client, pm_message_t msg) 
{
	struct stk8311_i2c_data *obj = i2c_get_clientdata(client);    
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
		if ((err = hwmsen_read_byte_sr(client, STK8311_REG_MODE, &dat))) 
		{
           GSE_ERR("read ctl_reg1  fail!!\n");
           return err;
        }
		dat = dat&0b11111110;//stand by mode
		atomic_set(&obj->suspend, 1);
		if(err = hwmsen_write_byte(client, STK8311_REG_MODE, dat))
		{
			GSE_ERR("write power control fail!!\n");
			return err;
		}        
		STK8311_power(obj->hw, 0);
	}
	return err;
}
/*----------------------------------------------------------------------------*/
static int stk8311_resume(struct i2c_client *client)
{
	struct stk8311_i2c_data *obj = i2c_get_clientdata(client);        
	int err;
	GSE_FUN();

	if(obj == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return -EINVAL;
	}

	STK8311_power(obj->hw, 1);
	if(err = STK8311_Init(client, 0))
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
static void stk8311_early_suspend(struct early_suspend *h) 
{
	struct stk8311_i2c_data *obj = container_of(h, struct stk8311_i2c_data, early_drv);   
	int err;
	GSE_FUN();    

	if(obj == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return;
	}
	atomic_set(&obj->suspend, 1); 
	/*
	if(err = hwmsen_write_byte(obj->client, STK8311_REG_POWER_CTL, 0x00))
	{
		GSE_ERR("write power control fail!!\n");
		return;
	}  
	*/
	if(err = STK8311_SetPowerMode(obj->client, false))
	{
		GSE_ERR("write power control fail!!\n");
		return;
	}

	sensor_power = false;
	
	STK8311_power(obj->hw, 0);
}
/*----------------------------------------------------------------------------*/
static void stk8311_late_resume(struct early_suspend *h)
{
	struct stk8311_i2c_data *obj = container_of(h, struct stk8311_i2c_data, early_drv);         
	int err;
	GSE_FUN();

	if(obj == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return;
	}

	STK8311_power(obj->hw, 1);
	if(err = STK8311_Init(obj->client, 0))
	{
		GSE_ERR("initialize client fail!!\n");
		return;        
	}
	atomic_set(&obj->suspend, 0);    
}
/*----------------------------------------------------------------------------*/
#endif /*CONFIG_HAS_EARLYSUSPEND*/
/*----------------------------------------------------------------------------*/
static int stk8311_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info) 
{    
	strcpy(info->type, STK8311_DEV_NAME);
	return 0;
}

/*----------------------------------------------------------------------------*/
static int stk8311_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_client *new_client;
	struct stk8311_i2c_data *obj;
	struct hwmsen_object sobj;
	int err = 0;
	GSE_FUN();

	if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
	{
		err = -ENOMEM;
		goto exit;
	}
	
	memset(obj, 0, sizeof(struct stk8311_i2c_data));

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
	


	stk8311_i2c_client = new_client;	

	if(err = STK8311_Init(new_client, 1))
	{
		goto exit_init_failed;
	}
	

	if(err = misc_register(&stk8311_device))
	{
		GSE_ERR("stk8311_device register failed\n");
		goto exit_misc_device_register_failed;
	}

	if(err = stk8311_create_attr(&stk8311_gsensor_driver.driver))
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
	obj->early_drv.suspend  = stk8311_early_suspend,
	obj->early_drv.resume   = stk8311_late_resume,    
	register_early_suspend(&obj->early_drv);
#endif 

	GSE_LOG("%s: OK\n", __func__);    
	return 0;

	exit_create_attr_failed:
	misc_deregister(&stk8311_device);
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
static int stk8311_i2c_remove(struct i2c_client *client)
{
	int err = 0;	
	
	if(err = stk8311_delete_attr(&stk8311_gsensor_driver.driver))
	{
		GSE_ERR("stk8311_delete_attr fail: %d\n", err);
	}
	
	if(err = misc_deregister(&stk8311_device))
	{
		GSE_ERR("misc_deregister fail: %d\n", err);
	}

	if(err = hwmsen_detach(ID_ACCELEROMETER))
	    

	stk8311_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));
	return 0;
}
/*----------------------------------------------------------------------------*/
static int stk8311_probe(struct platform_device *pdev) 
{
	struct acc_hw *hw = get_cust_acc_hw();
	GSE_FUN();

	STK8311_power(hw, 1);
	//stk8311_force[0] = hw->i2c_num;
	if(i2c_add_driver(&stk8311_i2c_driver))
	{
		GSE_ERR("add driver error\n");
		return -1;
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
static int stk8311_remove(struct platform_device *pdev)
{
    struct acc_hw *hw = get_cust_acc_hw();

    GSE_FUN();    
    STK8311_power(hw, 0);    
    i2c_del_driver(&stk8311_i2c_driver);
    return 0;
}
/*----------------------------------------------------------------------------*/
static struct platform_driver stk8311_gsensor_driver = {
	.probe      = stk8311_probe,
	.remove     = stk8311_remove,    
	.driver     = {
		.name  = "gsensor",
		//.owner = THIS_MODULE,
	}
};

/*----------------------------------------------------------------------------*/
static int __init stk8311_init(void)
{
	GSE_FUN();
	struct acc_hw *hw = get_cust_acc_hw();
	GSE_LOG("%s: i2c_number=%d\n", __func__,hw->i2c_num); 
	i2c_register_board_info(hw->i2c_num, &i2c_stk8311, 1);
	if(platform_driver_register(&stk8311_gsensor_driver))
	{
		GSE_ERR("failed to register driver");
		return -ENODEV;
	}
	return 0;    
}
/*----------------------------------------------------------------------------*/
static void __exit stk8311_exit(void)
{
	GSE_FUN();
	platform_driver_unregister(&stk8311_gsensor_driver);
}
/*----------------------------------------------------------------------------*/
module_init(stk8311_init);
module_exit(stk8311_exit);
/*----------------------------------------------------------------------------*/
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("STK8311 I2C driver");
MODULE_AUTHOR("Zhilin.Chen@mediatek.com");
