/* drivers/i2c/chips/stk8ba22.c - STK8BA22 motion sensor driver
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
#include "stk8ba50.h"
#include <linux/hwmsen_helper.h>

#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>


#ifdef MT6516
#define POWER_NONE_MACRO MT6516_POWER_NONE
#endif

#ifdef MT6573
#define POWER_NONE_MACRO MT65XX_POWER_NONE
#endif

#ifdef MT6575
#define POWER_NONE_MACRO MT65XX_POWER_NONE
#endif
#ifdef MT6577
#define POWER_NONE_MACRO MT65XX_POWER_NONE
#endif

#define STK_ACC_POLLING_MODE	1

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
#define DEBUG 1
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
#define STK8BA22_AXIS_X          0
#define STK8BA22_AXIS_Y          1
#define STK8BA22_AXIS_Z          2
#define STK8BA22_AXES_NUM        3
#define STK8BA22_DATA_LEN        6
#define STK8BA22_DEV_NAME        "STK8BA22"
/*----------------------------------------------------------------------------*/
static const struct i2c_device_id stk8ba22_i2c_id[] = {{STK8BA22_DEV_NAME,0},{}};
/*the adapter id will be available in customization*/
static struct i2c_board_info __initdata i2c_stk8ba22={ I2C_BOARD_INFO("STK8BA22", STK8BA22_I2C_SLAVE_ADDR)};


/*----------------------------------------------------------------------------*/
static int stk8ba22_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id); 
static int stk8ba22_i2c_remove(struct i2c_client *client);
static int stk8ba22_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);

/*----------------------------------------------------------------------------*/
static int STK8BA22_SetPowerMode(struct i2c_client *client, bool enable);
static int STK8BA22_SetBWSEL(struct i2c_client *client, u8 dataformat);


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
    s16 raw[C_MAX_FIR_LENGTH][STK8BA22_AXES_NUM];
    int sum[STK8BA22_AXES_NUM];
    int num;
    int idx;
};
/*----------------------------------------------------------------------------*/
struct stk8ba22_i2c_data {
    struct i2c_client *client;
    struct acc_hw *hw;
    struct hwmsen_convert   cvt;
    
    /*misc*/
    struct data_resolution *reso;
    atomic_t                trace;
    atomic_t                suspend;
    atomic_t                selftest;
	atomic_t				filter;
    s16                     cali_sw[STK8BA22_AXES_NUM+1];

    /*data*/
    s8                      offset[STK8BA22_AXES_NUM+1];  /*+1: for 4-byte alignment*/
    s16                     data[STK8BA22_AXES_NUM+1];


    /*early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
    struct early_suspend    early_drv;
#endif     
};
/*----------------------------------------------------------------------------*/
static struct i2c_driver stk8ba22_i2c_driver = {
    .driver = {
        .name           = STK8BA22_DEV_NAME,
    },
	.probe      		= stk8ba22_i2c_probe,
	.remove    			= stk8ba22_i2c_remove,
	.detect				= stk8ba22_i2c_detect,
#if !defined(CONFIG_HAS_EARLYSUSPEND)    
    .suspend            = stk8ba22_suspend,
    .resume             = stk8ba22_resume,
#endif
	.id_table = stk8ba22_i2c_id,
	//.address_data = &stk8ba22_addr_data,
};

/*----------------------------------------------------------------------------*/
static struct i2c_client *stk8ba22_i2c_client = NULL;
static struct platform_driver stk8ba22_gsensor_driver;
static struct stk8ba22_i2c_data *obj_i2c_data = NULL;
static bool sensor_power = false;
static GSENSOR_VECTOR3D gsensor_gain, gsensor_offset;



/*----------------------------------------------------------------------------*/
#define GSE_TAG                  "[Gsensor] "
#define GSE_FUN(f)               printk(KERN_INFO GSE_TAG"%s\n", __FUNCTION__)
#define GSE_ERR(fmt, args...)    printk(KERN_ERR GSE_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define GSE_LOG(fmt, args...)    printk(KERN_INFO GSE_TAG fmt, ##args)
/*----------------------------------------------------------------------------*/

static struct data_resolution stk8ba22_data_resolution[] = {
 /* combination by {FULL_RES,RANGE}*/
	 {{ 15, 6}, 64},   // dataformat +/-2g	in 8-bit resolution;  { 15, 6} = 15.6= (2*2*1000)/(2^8);  64 = (2^8)/(2*2)			
	 {{ 31, 2}, 32},   // dataformat +/-4g	 in 8-bit resolution;  { 31, 2} = 31.2= (2*4*1000)/(2^8);  32 = (2^8)/(4*2) 		 
	 {{ 64, 4}, 16},   // dataformat +/-8g	 in 8-bit resolution;  { 62, 4} = 62.4= (2*8*1000)/(2^8);  16 = (2^8)/(8*2) 		 
	 {{ 124, 8}, 8},   // dataformat +/-16g  in 8-bit resolution;  { 124,8} = 124.8= (2*16*1000)/(2^8);  8 = (2^8)/(16*2)			 
};

/*----------------------------------------------------------------------------*/
static struct data_resolution stk8ba22_offset_resolution = {{7, 8}, 128};

#define  STK8BA22_SPTIME_NO		8
const static int STK8BA22_SAMPLE_TIME[STK8BA22_SPTIME_NO] = {213220, 106610, 53333, 26667, 13333, 6667, 3333, 1667};
uint32_t gsensor_delay = 13333;


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

static void STK8BA22_power(struct acc_hw *hw, unsigned int on) 
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
			if(!hwPowerOn(hw->power_id, hw->power_vol, "STK8BA22"))
			{
				GSE_ERR("power on fails!!\n");
			}
		}
		else	// power off
		{
			if (!hwPowerDown(hw->power_id, "STK8BA22"))
			{
				GSE_ERR("power off fail!!\n");
			}			  
		}
	}
	power_on = on;    
}
/*----------------------------------------------------------------------------*/
//this function here use to set resolution and choose sensitivity
static int STK8BA22_SetDataResolution(struct i2c_client *client ,u8 dataresolution)
{
    GSE_LOG("fwq set resolution  dataresolution= %d!\n", dataresolution);
	int err;
	u8 databuf[2];    
	int res = 0;
	u8  reso=0;
	u8  dat;

	struct stk8ba22_i2c_data *obj = i2c_get_clientdata(client);

	memset(databuf, 0, sizeof(u8)*2);    
	databuf[0] = STK8BA22_RANGESEL;    



	switch(dataresolution)
	{
	case STK8BA22_RNG_2G:
		databuf[1]= STK8BA22_RNG_2G;
		reso = 0;
		break;
	case STK8BA22_RNG_4G:
		databuf[1] = STK8BA22_RNG_4G;
		reso = 1;
		break;
	case STK8BA22_RNG_8G:	
		databuf[1] = STK8BA22_RNG_8G;
		reso = 2;
		break;
	case STK8BA22_RNG_16G:	
		databuf[1] = STK8BA22_RNG_16G;
		reso = 3;
		break;
	default:
		databuf[1] = STK8BA22_RNG_2G;
		printk(KERN_ERR "%s: unknown range, set as STK8BA22_RNG_2G\n", __func__);
	}
	if(reso < sizeof(stk8ba22_data_resolution)/sizeof(stk8ba22_data_resolution[0]))
	{        
		obj->reso = &stk8ba22_data_resolution[reso];
		GSE_LOG("reso=%x!! OK \n",reso);
		err = 0;
	}
	else
	{   
	    GSE_ERR("choose sensitivity  fail!!\n");
		err = -EINVAL;
	}
	res = i2c_master_send(client, databuf, 0x2);

	if(res <= 0)
	{
		err = STK8BA22_ERR_I2C;
	}
	udelay(500);

	hwmsen_read_byte(client,STK8BA22_POWMODE,&dat);

	return err;



}
/*----------------------------------------------------------------------------*/

static int STK8BA22_ReadData(struct i2c_client *client, s16 data[STK8BA22_AXES_NUM])
	{	
		int result;
		u8 acc_reg[6];
		u8  dat;
		
		hwmsen_read_block(client,STK8BA22_XOUT1,acc_reg,sizeof(acc_reg));
		if (result < 0) 
		{
			printk(KERN_ERR "%s: failed to read 6 bytes acc data, error=0x%x\n", __func__, result);
			return result;
		}		

		data[STK8BA22_AXIS_X] = (s16)acc_reg[STK8BA22_AXIS_X*2+1] ;
		data[STK8BA22_AXIS_Y] = (s16)acc_reg[STK8BA22_AXIS_Y*2+1];
		data[STK8BA22_AXIS_Z] = (s16)acc_reg[STK8BA22_AXIS_Z*2+1] ;

		if(data[STK8BA22_AXIS_X]&0x80)
		{
				data[STK8BA22_AXIS_X] = ~data[STK8BA22_AXIS_X];
				data[STK8BA22_AXIS_X] &= 0xff;
				data[STK8BA22_AXIS_X]+=1;
				data[STK8BA22_AXIS_X] = -data[STK8BA22_AXIS_X];
		}
		if(data[STK8BA22_AXIS_Y]&0x80)
		{
				data[STK8BA22_AXIS_Y] = ~data[STK8BA22_AXIS_Y];
				data[STK8BA22_AXIS_Y] &= 0xff;
				data[STK8BA22_AXIS_Y]+=1;
				data[STK8BA22_AXIS_Y] = -data[STK8BA22_AXIS_Y];
		}
		if(data[STK8BA22_AXIS_Z]&0x80)
		{
				data[STK8BA22_AXIS_Z] = ~data[STK8BA22_AXIS_Z];
				data[STK8BA22_AXIS_Z] &= 0xff;
				data[STK8BA22_AXIS_Z]+=1;
				data[STK8BA22_AXIS_Z] = -data[STK8BA22_AXIS_Z];
		}

		//GSE_ERR("%s: acc_reg[1] = %d\n", __func__, acc_reg[1]);
		//GSE_ERR("%s: acc_reg[3] = %d\n", __func__, acc_reg[3]);
		//GSE_ERR("%s: acc_reg[5] = %d\n", __func__, acc_reg[5]);
		//GSE_ERR("%s: acc_reg[1] = %d\n", __func__, data[STK8BA22_AXIS_X]);
		//GSE_ERR("%s: acc_reg[3] = %d\n", __func__, data[STK8BA22_AXIS_Y]);
		//GSE_ERR("%s: acc_reg[5] = %d\n", __func__, data[STK8BA22_AXIS_Z]);
		return 0;	

}
/*----------------------------------------------------------------------------*/
static int STK8BA22_ReadOffset(struct i2c_client *client, s8 ofs[STK8BA22_AXES_NUM])
{    
	int err;
    GSE_ERR("fwq read offset+: \n");


	if(err = hwmsen_write_block(client, STK8BA22_OFSTFILTX, ofs, STK8BA22_AXES_NUM))
	{
		GSE_ERR("error: %d\n", err);
	}
	return err;    
}
/*----------------------------------------------------------------------------*/
static int STK8BA22_ResetCalibration(struct i2c_client *client)
{
	struct stk8ba22_i2c_data *obj = i2c_get_clientdata(client);
	s8 ofs[STK8BA22_AXES_NUM] = {0x00, 0x00, 0x00};
	int err;
	bool old_sensorpowmode = sensor_power;

	//goto standby mode to clear cali
	STK8BA22_SetPowerMode(obj->client,false);

	if(err = STK8BA22_ReadOffset(client,ofs))
	{
		GSE_ERR("error: %d\n", err);
	}
	if(old_sensorpowmode == true)
    STK8BA22_SetPowerMode(obj->client,true);
	memset(obj->cali_sw, 0x00, sizeof(obj->cali_sw));
	return err;    
}
/*----------------------------------------------------------------------------*/
static int STK8BA22_ReadCalibration(struct i2c_client *client, int dat[STK8BA22_AXES_NUM])
{
    struct stk8ba22_i2c_data *obj = i2c_get_clientdata(client);
    int err;
    int mul;
    
    if ((err = STK8BA22_ReadOffset(client, obj->offset))) {
        GSE_ERR("read offset fail, %d\n", err);
        return err;
    }    
    
    //mul = obj->reso->sensitivity/mma8452q_offset_resolution.sensitivity;
    mul = stk8ba22_offset_resolution.sensitivity/obj->reso->sensitivity;
    dat[obj->cvt.map[STK8BA22_AXIS_X]] = obj->cvt.sign[STK8BA22_AXIS_X]*(obj->offset[STK8BA22_AXIS_X]/mul);
    dat[obj->cvt.map[STK8BA22_AXIS_Y]] = obj->cvt.sign[STK8BA22_AXIS_Y]*(obj->offset[STK8BA22_AXIS_Y]/mul);
    dat[obj->cvt.map[STK8BA22_AXIS_Z]] = obj->cvt.sign[STK8BA22_AXIS_Z]*(obj->offset[STK8BA22_AXIS_Z]/mul);                        
    GSE_LOG("fwq:read cali offX=%x ,offY=%x ,offZ=%x\n",obj->offset[STK8BA22_AXIS_X],obj->offset[STK8BA22_AXIS_Y],obj->offset[STK8BA22_AXIS_Z]);
	//GSE_LOG("fwq:read cali swX=%x ,swY=%x ,swZ=%x\n",obj->cali_sw[MMA8452Q_AXIS_X],obj->cali_sw[MMA8452Q_AXIS_Y],obj->cali_sw[MMA8452Q_AXIS_Z]);
    return 0;
}
/*----------------------------------------------------------------------------*/
static int STK8BA22_ReadCalibrationEx(struct i2c_client *client, int act[STK8BA22_AXES_NUM], int raw[STK8BA22_AXES_NUM])
{  
	/*raw: the raw calibration data; act: the actual calibration data*/
	struct stk8ba22_i2c_data *obj = i2c_get_clientdata(client);
	int err;
	int mul;

	if(err = STK8BA22_ReadOffset(client, obj->offset))
	{
		GSE_ERR("read offset fail, %d\n", err);
		return err;
	}    

	mul = stk8ba22_offset_resolution.sensitivity/obj->reso->sensitivity;
	raw[STK8BA22_AXIS_X] = obj->offset[STK8BA22_AXIS_X]/mul + obj->cali_sw[STK8BA22_AXIS_X];
	raw[STK8BA22_AXIS_Y] = obj->offset[STK8BA22_AXIS_Y]/mul + obj->cali_sw[STK8BA22_AXIS_Y];
	raw[STK8BA22_AXIS_Z] = obj->offset[STK8BA22_AXIS_Z]/mul + obj->cali_sw[STK8BA22_AXIS_Z];

	act[obj->cvt.map[STK8BA22_AXIS_X]] = obj->cvt.sign[STK8BA22_AXIS_X]*raw[STK8BA22_AXIS_X];
	act[obj->cvt.map[STK8BA22_AXIS_Y]] = obj->cvt.sign[STK8BA22_AXIS_Y]*raw[STK8BA22_AXIS_Y];
	act[obj->cvt.map[STK8BA22_AXIS_Z]] = obj->cvt.sign[STK8BA22_AXIS_Z]*raw[STK8BA22_AXIS_Z];                        
	                       
	return 0;
}
/*----------------------------------------------------------------------------*/

static int STK8BA22_SetDelay(struct i2c_client *client, uint32_t sdelay_ns)
{
	unsigned char sr_no;	
	int result;
	
	uint32_t sdelay_us = sdelay_ns / 1000;

	for(sr_no=0;sr_no<STK8BA22_SPTIME_NO;sr_no++)
	{
		if(sdelay_us >= STK8BA22_SAMPLE_TIME[sr_no])	
			break;		
	}	
		
	if(sr_no == 0)
	{
		sdelay_ns = STK8BA22_SAMPLE_TIME[0]*1000;
	}
	else if(sr_no == (STK8BA22_SPTIME_NO-1))
	{
		sdelay_ns = STK8BA22_SAMPLE_TIME[STK8BA22_SPTIME_NO-1]*1000;		
	}
	sr_no += 0x8;//STK8BAXX_SPTIME_BASE
#ifdef STK_DEBUG_PRINT		
	printk(KERN_INFO "%s:sdelay_ns=%ud, sr_no=%d\n", __func__, sdelay_ns, sr_no);
#endif	
	result = STK8BA22_SetBWSEL(client,sr_no);

	if (result < 0) 
	{
		printk(KERN_ERR "%s:failed to write reg 0x%x, error=0x%x\n", __func__, STK8BA22_BWSEL, result);
		return result;
	}
	gsensor_delay = sdelay_us;
	return result;	
}
static s64 STK8BA22_GetDelay()
{
	return gsensor_delay;
}
/*sstate is not used now*/

static int STK8BA22_SetCali(struct i2c_client *client, char sstate)
{
	int result;
	u8 offset[3];
	int retry = 0;
	s64 ord_delay;
	
	ord_delay = STK8BA22_GetDelay();
	STK8BA22_SetDelay(client, 13333);	/*	75 Hz ODR */	
	
	if(sensor_power)
	{
	}		
	else
	{
		result = STK8BA22_SetPowerMode(client,STK8BA22_MD_NORMAL);
		
		if (result < 0) 
		{
			printk(KERN_ERR "%s:failed to write reg 0x%x, error=0x%x\n", __func__, STK8BA22_POWMODE, result);
			return result;
		}		
	}

	result = i2c_smbus_write_byte_data(client, STK8BA22_OFSTCOMP1, 0x0);
	
	if (result < 0) 
	{
		printk(KERN_ERR "%s:failed to write reg 0x%x, error=0x%x\n", __func__, STK8BA22_OFSTCOMP1, result);
		return result;
	}	
	result = i2c_smbus_write_byte_data(client, STK8BA22_OFSTCOMP2, CAL_TG_X0_Y0_ZPOS1);
	
	if (result < 0) 
	{
		printk(KERN_ERR "%s:failed to write reg 0x%x, error=0x%x\n", __func__, STK8BA22_OFSTCOMP2, result);
		return result;
	}		
	result = i2c_smbus_write_byte_data(client, STK8BA22_OFSTCOMP1, CAL_AXIS_X_EN);
	if (result < 0) 
	{
		printk(KERN_ERR "%s:failed to write reg 0x%x, error=0x%x\n", __func__, STK8BA22_OFSTCOMP1, result);
		return result;
	}	
	msleep(250);
	do {
		msleep(5);
		result = i2c_smbus_read_byte_data(client, STK8BA22_OFSTCOMP1);
		if (result < 0) 
		{
			printk(KERN_ERR "%s: failed to read reg 0x%x, error=0x%x\n", __func__, STK8BA22_OFSTCOMP1, result);
			return result;
		}	
		retry++;
	} while (!(result & STK8BA22_OF_CAL_DRY_MASK) && retry<25);
	result = i2c_smbus_write_byte_data(client, STK8BA22_OFSTCOMP1, CAL_AXIS_Y_EN);
	if (result < 0) 
	{
		printk(KERN_ERR "%s:failed to write reg 0x%x, error=0x%x\n", __func__, STK8BA22_OFSTCOMP1, result);
		return result;
	}	
	retry = 0;
	msleep(250);
	do {
		msleep(5);		
		result = i2c_smbus_read_byte_data(client, STK8BA22_OFSTCOMP1);
		if (result < 0) 
		{
			printk(KERN_ERR "%s: failed to read reg 0x%x, error=0x%x\n", __func__, STK8BA22_OFSTCOMP1, result);
			return result;
		}	
		retry++;
	} while (!(result & STK8BA22_OF_CAL_DRY_MASK) && retry<25);
	result = i2c_smbus_write_byte_data(client, STK8BA22_OFSTCOMP1, CAL_AXIS_Z_EN);
	if (result < 0) 
	{
		printk(KERN_ERR "%s:failed to write reg 0x%x, error=0x%x\n", __func__, STK8BA22_OFSTCOMP1, result);
		return result;
	}	
	retry = 0;
	msleep(250);
	do {
		msleep(5);		
		result = i2c_smbus_read_byte_data(client, STK8BA22_OFSTCOMP1);
		if (result < 0) 
		{
			printk(KERN_ERR "%s: failed to read reg 0x%x, error=0x%x\n", __func__, STK8BA22_OFSTCOMP1, result);
			return result;
		}	
		retry++;
	} while (!(result & STK8BA22_OF_CAL_DRY_MASK) && retry<25);	
	
	if(sensor_power)
	{
	}		
	else
	{		
		result = STK8BA22_SetPowerMode(client,STK8BA22_MD_SUSPEND);
		if (result < 0) 
		{
			printk(KERN_ERR "%s:failed to write reg 0x%x, error=0x%x\n", __func__, STK8BA22_POWMODE, result);
			return result;
		}		
	}	
	STK8BA22_ReadOffset(client,offset);
	STK8BA22_SetDelay(client, ord_delay);
	return 0;
}

static int STK8BA22_WriteCalibration(struct i2c_client *client, int dat[STK8BA22_AXES_NUM])
{

	struct stk8ba22_i2c_data *obj = i2c_get_clientdata(client);
	u8 testdata=0;
	int err;
	int cali[STK8BA22_AXES_NUM], raw[STK8BA22_AXES_NUM];
	int lsb = stk8ba22_offset_resolution.sensitivity;
	u8 databuf[2]; 
	int res = 0;

	//int divisor = obj->reso->sensitivity/lsb;
	int divisor = lsb/obj->reso->sensitivity;
	GSE_LOG("fwq obj->reso->sensitivity=%d\n", obj->reso->sensitivity);
	GSE_LOG("fwq lsb=%d\n", lsb);
	

	if(err = STK8BA22_ReadCalibrationEx(client, cali, raw))	/*offset will be updated in obj->offset*/
	{ 
		GSE_ERR("read offset fail, %d\n", err);
		return err;
	}

	GSE_LOG("OLDOFF: (%+3d %+3d %+3d): (%+3d %+3d %+3d) / (%+3d %+3d %+3d)\n", 
		raw[STK8BA22_AXIS_X], raw[STK8BA22_AXIS_Y], raw[STK8BA22_AXIS_Z],
		obj->offset[STK8BA22_AXIS_X], obj->offset[STK8BA22_AXIS_Y], obj->offset[STK8BA22_AXIS_Z],
		obj->cali_sw[STK8BA22_AXIS_X], obj->cali_sw[STK8BA22_AXIS_Y], obj->cali_sw[STK8BA22_AXIS_Z]);

	/*calculate the real offset expected by caller*/
	cali[STK8BA22_AXIS_X] += dat[STK8BA22_AXIS_X];
	cali[STK8BA22_AXIS_Y] += dat[STK8BA22_AXIS_Y];
	cali[STK8BA22_AXIS_Z] += dat[STK8BA22_AXIS_Z];

	GSE_LOG("UPDATE: (%+3d %+3d %+3d)\n", 
		dat[STK8BA22_AXIS_X], dat[STK8BA22_AXIS_Y], dat[STK8BA22_AXIS_Z]);

	obj->offset[STK8BA22_AXIS_X] = (s8)(obj->cvt.sign[STK8BA22_AXIS_X]*(cali[obj->cvt.map[STK8BA22_AXIS_X]])*(divisor));
	obj->offset[STK8BA22_AXIS_Y] = (s8)(obj->cvt.sign[STK8BA22_AXIS_Y]*(cali[obj->cvt.map[STK8BA22_AXIS_Y]])*(divisor));
	obj->offset[STK8BA22_AXIS_Z] = (s8)(obj->cvt.sign[STK8BA22_AXIS_Z]*(cali[obj->cvt.map[STK8BA22_AXIS_Z]])*(divisor));

	/*convert software calibration using standard calibration*/
	obj->cali_sw[STK8BA22_AXIS_X] = obj->cvt.sign[STK8BA22_AXIS_X]*(cali[obj->cvt.map[STK8BA22_AXIS_X]])%(divisor);
	obj->cali_sw[STK8BA22_AXIS_Y] = obj->cvt.sign[STK8BA22_AXIS_Y]*(cali[obj->cvt.map[STK8BA22_AXIS_Y]])%(divisor);
	obj->cali_sw[STK8BA22_AXIS_Z] = obj->cvt.sign[STK8BA22_AXIS_Z]*(cali[obj->cvt.map[STK8BA22_AXIS_Z]])%(divisor);

	GSE_LOG("NEWOFF: (%+3d %+3d %+3d): (%+3d %+3d %+3d) / (%+3d %+3d %+3d)\n", 
		obj->offset[STK8BA22_AXIS_X] + obj->cali_sw[STK8BA22_AXIS_X], 
		obj->offset[STK8BA22_AXIS_Y] + obj->cali_sw[STK8BA22_AXIS_Y], 
		obj->offset[STK8BA22_AXIS_Z] + obj->cali_sw[STK8BA22_AXIS_Z], 
		obj->offset[STK8BA22_AXIS_X], obj->offset[STK8BA22_AXIS_Y], obj->offset[STK8BA22_AXIS_Z],
		obj->cali_sw[STK8BA22_AXIS_X], obj->cali_sw[STK8BA22_AXIS_Y], obj->cali_sw[STK8BA22_AXIS_Z]);
	//
	//go to standby mode to set cali
    STK8BA22_SetPowerMode(obj->client,false);
	if(err = hwmsen_write_block(obj->client, STK8BA22_OFSTFILTX, obj->offset, STK8BA22_AXES_NUM))
	{
		GSE_ERR("write offset fail: %d\n", err);
		return err;
	}
	STK8BA22_SetPowerMode(obj->client,true);

	return err;
}

static int STK8BA22_SetReset(struct i2c_client *client)
{
	u8 databuf[2];    
	int res = 0;
	struct stk8ba22_i2c_data *obj = i2c_get_clientdata(client);
	
	GSE_FUN();

	
	memset(databuf, 0, sizeof(u8)*2);
	databuf[0] = STK8BA22_SWRST;    
	databuf[1] = STK8BA22_SWRST_VAL;

	res = i2c_master_send(client, databuf, 0x2);


	if(res <= 0)
	{
		GSE_LOG("fwq set reset failed!\n");
		return STK8BA22_ERR_I2C;
	}
	else if(atomic_read(&obj->trace) & ADX_TRC_INFO)
	{
		GSE_LOG("fwq set reset ok %d!\n", databuf[1]);
	}

	return STK8BA22_SUCCESS;    
}

/*----------------------------------------------------------------------------*/
//normal
//High resolution
//low noise low power
//low power

/*---------------------------------------------------------------------------*/
static int STK8BA22_SetPowerMode(struct i2c_client *client, bool enable)
{
	u8 databuf[2];	  
	int res = 0;
	u8  dat;
	struct stk8ba22_i2c_data *obj = i2c_get_clientdata(client);

	GSE_FUN();
	if(enable == sensor_power)
	{
		GSE_LOG("Sensor power status need not to be set again!!!\n");
		return STK8BA22_SUCCESS;
	}

	
	if(enable == TRUE)
	{
		databuf[0] = STK8BA22_MD_NORMAL;
	}
	else
	{
		databuf[0] = STK8BA22_MD_SUSPEND;
	}
	databuf[1] = databuf[0];
	databuf[0] = STK8BA22_POWMODE;
	

	res = i2c_master_send(client, databuf, 0x2);

	if(res <= 0)
	{
		GSE_LOG("fwq set power mode failed!\n");
		return STK8BA22_ERR_I2C;
	}
	else if(atomic_read(&obj->trace) & ADX_TRC_INFO)
	{
		GSE_LOG("fwq set power mode ok %d!\n", databuf[1]);
	}
	udelay(500);

	hwmsen_read_byte(client,STK8BA22_POWMODE,&dat);

	sensor_power = enable;
	
	return STK8BA22_SUCCESS;	

}
/*----------------------------------------------------------------------------*/
//set detect range
static int STK8BA22_SetDataRange(struct i2c_client *client, u8 dataformat)
{
    
	u8 databuf[2];    
	int res = 0;

	memset(databuf, 0, sizeof(u8)*2);    
	databuf[0] = STK8BA22_RANGESEL;    
	databuf[1] = dataformat;

	res = i2c_master_send(client, databuf, 0x2);

	if(res <= 0)
	{
		return STK8BA22_ERR_I2C;
	}

	return 0;

}
//set detect range
static int STK8BA22_SetBWSEL(struct i2c_client *client, u8 dataformat)
{
    
	u8 databuf[2];    
	int res = 0;

	memset(databuf, 0, sizeof(u8)*2);    
	databuf[0] = STK8BA22_BWSEL;    
	databuf[1] = dataformat;

	res = i2c_master_send(client, databuf, 0x2);

	if(res <= 0)
	{
		return STK8BA22_ERR_I2C;
	}

	return 0;

}


static int STK8BA22_SetDataFormat(struct i2c_client *client, u8 dataformat)
{
    
	u8 databuf[2];    
	int res = 0;

	memset(databuf, 0, sizeof(u8)*2);    
	databuf[0] = STK8BA22_RANGESEL;    
	databuf[1] = dataformat;

	res = i2c_master_send(client, databuf, 0x2);

	if(res <= 0)
	{
		return STK8BA22_ERR_I2C;
	}

	return 0;

}
static int STK8BA22_SetWatchDog(struct i2c_client *client, u8 intenable)
{
	u8 databuf[2];    
	int res = 0;

	memset(databuf, 0, sizeof(u8)*2);    
	databuf[0] = STK8BA22_INTFCFG;    
	databuf[1] = intenable;

	res = i2c_master_send(client, databuf, 0x2);

	if(res <= 0)
	{
		return STK8BA22_ERR_I2C;
	}
	
	return STK8BA22_SUCCESS;    
}

/*----------------------------------------------------------------------------*/
static int STK8BA22_Init_client(struct i2c_client *client, int reset_cali)
{
	struct stk8ba22_i2c_data *obj = i2c_get_clientdata(client);
	int res = 0;
	int result;	
	u8 databuf[2];
	u8 dat;

	res = STK8BA22_SetReset(client);

	if(res != STK8BA22_SUCCESS)
	{
	    GSE_LOG("fwq stk8ba22 set reset error\n");
		return res;
	}

	result = STK8BA22_SetPowerMode(client,false);

#if (!STK_ACC_POLLING_MODE)
		/* map new data int to int1 */
		result = i2c_smbus_write_byte_data(client, STK8BA22_INTMAP2, 0x01);
		if (result < 0) 
		{
			printk(KERN_ERR "%s:failed to write reg 0x%x, error=0x%x\n", __func__, STK8BA22_INTMAP2, result);		
			return result;
		}	
		/*	enable new data int */
		result = i2c_smbus_write_byte_data(client, STK8BA22_INTEN2, 0x10);
		if (result < 0) 
		{
			printk(KERN_ERR "%s:failed to write reg 0x%x, error=0x%x\n", __func__, STK8BA22_INTEN2, result);		
			return result;
		}			
		/*	non-latch int	*/
		result = i2c_smbus_write_byte_data(client, STK8BA22_INTCFG2, 0x00);
		if (result < 0) 
		{
			printk(KERN_ERR "%s:failed to write reg 0x%x, error=0x%x\n", __func__, STK8BA22_INTCFG2, result);		
			return result;
		}	
		/*	filtered data source for new data int	*/
		result = i2c_smbus_write_byte_data(client, STK8BA22_DATASRC, 0x00);
		if (result < 0) 
		{
			printk(KERN_ERR "%s:failed to write reg 0x%x, error=0x%x\n", __func__, STK8BA22_DATASRC, result);		
			return result;
		}		
		/*	int1, push-pull, active high	*/
		result = i2c_smbus_write_byte_data(client, STK8BA22_INTCFG1, 0x01);
		if (result < 0) 
		{
			printk(KERN_ERR "%s:failed to write reg 0x%x, error=0x%x\n", __func__, STK8BA22_INTCFG1, result);		
			return result;
		}	
#endif	

		res = STK8BA22_SetDataResolution(client, STK8BA22_RNG_2G);
		if(res != STK8BA22_SUCCESS) 
		{
			GSE_LOG("fwq stk8ba22 set data reslution error\n");
			return res;
		}

	res = STK8BA22_SetBWSEL(client, 0x0);
	if(res != STK8BA22_SUCCESS)
	{
		GSE_LOG("fwq stk8ba22 set data format error\n");
		return res;
	}

	result = STK8BA22_SetWatchDog(client,0x04);//enable I2C Watch dog
	if (result < 0) 
	{
		printk(KERN_ERR "%s:failed to write reg 0x%x, error=0x%x\n", __func__, STK8BA22_INTFCFG, result);		
		return result;
	}


	if(NULL != reset_cali)
	{ 
		/*reset calibration only in power on*/
		GSE_LOG("fwq stk8ba22  set cali\n");
		res = STK8BA22_ResetCalibration(client);
		if(res != STK8BA22_SUCCESS)
		{
		    GSE_LOG("fwq stk8ba22 set cali error\n");
			return res;
		}
		res = STK8BA22_SetCali(client,1);

		if(res != STK8BA22_SUCCESS)
		{
			GSE_LOG("fwq stk8ba22 set cali error\n");
			return res;
		}
	}


    GSE_LOG("fwq stk8ba22 Init OK\n");
	return res;
}
/*----------------------------------------------------------------------------*/
static int STK8BA22_ReadChipInfo(struct i2c_client *client, char *buf, int bufsize)
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

	sprintf(buf, "STK8BA22 Chip");
	return 0;
}



static int STK8BA22_ReadSensorData(struct i2c_client *client, char *buf, int bufsize)
{
	struct stk8ba22_i2c_data *obj = (struct stk8ba22_i2c_data*)i2c_get_clientdata(client);
	u8 databuf[20];
	int acc[STK8BA22_AXES_NUM];
	int res = 0;
	memset(databuf, 0, sizeof(u8)*20);

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

		res = STK8BA22_SetPowerMode(client, true);
		if(res)
		{
			GSE_ERR("Power on stk8ba22 error %d!\n", res);
		}
	}

	if(res = STK8BA22_ReadData(client, obj->data))
	{        
		return -3;
	}
	else
	{
		obj->data[STK8BA22_AXIS_X] += obj->cali_sw[STK8BA22_AXIS_X];
		obj->data[STK8BA22_AXIS_Y] += obj->cali_sw[STK8BA22_AXIS_Y];
		obj->data[STK8BA22_AXIS_Z] += obj->cali_sw[STK8BA22_AXIS_Z];
		//GSE_ERR("Mapped gsensor cali_sw: %d, %d, %d!\n", obj->cali_sw[STK8BA22_AXIS_X], obj->cali_sw[STK8BA22_AXIS_Y], obj->cali_sw[STK8BA22_AXIS_Z]);
		/*remap coordinate*/
		//GSE_ERR("Mapped gsensor cali_sw: %d, %d, %d!\n", obj->cvt.sign[STK8BA22_AXIS_X], obj->cvt.sign[STK8BA22_AXIS_Y], obj->cvt.sign[STK8BA22_AXIS_Z]);
		acc[obj->cvt.map[STK8BA22_AXIS_X]] = obj->cvt.sign[STK8BA22_AXIS_X]*obj->data[STK8BA22_AXIS_X];
		acc[obj->cvt.map[STK8BA22_AXIS_Y]] = obj->cvt.sign[STK8BA22_AXIS_Y]*obj->data[STK8BA22_AXIS_Y];
		acc[obj->cvt.map[STK8BA22_AXIS_Z]] = obj->cvt.sign[STK8BA22_AXIS_Z]*obj->data[STK8BA22_AXIS_Z];

		//GSE_ERR("Mapped gsensor data: %d, %d, %d!\n", acc[STK8BA22_AXIS_X], acc[STK8BA22_AXIS_Y], acc[STK8BA22_AXIS_Z]);

		//Out put the mg
		acc[STK8BA22_AXIS_X] = acc[STK8BA22_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		acc[STK8BA22_AXIS_Y] = acc[STK8BA22_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		acc[STK8BA22_AXIS_Z] = acc[STK8BA22_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;		
		

		sprintf(buf, "%04x %04x %04x", acc[STK8BA22_AXIS_X], acc[STK8BA22_AXIS_Y], acc[STK8BA22_AXIS_Z]);
		if(atomic_read(&obj->trace) & ADX_TRC_IOCTL)
		{
			GSE_LOG("gsensor data: %s!\n", buf);
			GSE_LOG("gsensor data:  sensitivity x=%d \n",gsensor_gain.z);
			 
		}
	}
	
	return 0;
}
/*----------------------------------------------------------------------------*/
static int STK8BA22_ReadRawData(struct i2c_client *client, char *buf)
{
	struct stk8ba22_i2c_data *obj = (struct stk8ba22_i2c_data*)i2c_get_clientdata(client);
	int res = 0;

	if (!buf || !client)
	{
		return EINVAL;
	}

	if(sensor_power == FALSE)
	{
		res = STK8BA22_SetPowerMode(client, true);
		if(res)
		{
			GSE_ERR("Power on stk8ba22 error %d!\n", res);
		}
	}
	
	if(res = STK8BA22_ReadData(client, obj->data))
	{        
		GSE_ERR("I2C error: ret value=%d", res);
		return EIO;
	}
	else
	{
		sprintf(buf, "%04x %04x %04x", obj->data[STK8BA22_AXIS_X], 
			obj->data[STK8BA22_AXIS_Y], obj->data[STK8BA22_AXIS_Z]);
	
	}
	GSE_LOG("gsensor data: %s!\n", buf);
	return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
    GSE_LOG("fwq show_chipinfo_value \n");
	struct i2c_client *client = stk8ba22_i2c_client;
	char strbuf[STK8BA22_BUFSIZE];
	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	
	STK8BA22_ReadChipInfo(client, strbuf, STK8BA22_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);        
}
/*----------------------------------------------------------------------------*/
static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = stk8ba22_i2c_client;
	char strbuf[STK8BA22_BUFSIZE];
	
	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	STK8BA22_ReadSensorData(client, strbuf, STK8BA22_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);            
}
/*----------------------------------------------------------------------------*/
static ssize_t show_cali_value(struct device_driver *ddri, char *buf)
{
    GSE_LOG("fwq show_cali_value \n");
	struct i2c_client *client = stk8ba22_i2c_client;
	struct stk8ba22_i2c_data *obj;

	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}

	obj = i2c_get_clientdata(client);

	int err, len = 0, mul;
	int tmp[STK8BA22_AXES_NUM];

	if(err = STK8BA22_ReadOffset(client, obj->offset))
	{
		return -EINVAL;
	}
	else if(err = STK8BA22_ReadCalibration(client, tmp))
	{
		return -EINVAL;
	}
	else
	{    
		mul = obj->reso->sensitivity/stk8ba22_offset_resolution.sensitivity;
		len += snprintf(buf+len, PAGE_SIZE-len, "[HW ][%d] (%+3d, %+3d, %+3d) : (0x%02X, 0x%02X, 0x%02X)\n", mul,                        
			obj->offset[STK8BA22_AXIS_X], obj->offset[STK8BA22_AXIS_Y], obj->offset[STK8BA22_AXIS_Z],
			obj->offset[STK8BA22_AXIS_X], obj->offset[STK8BA22_AXIS_Y], obj->offset[STK8BA22_AXIS_Z]);
		len += snprintf(buf+len, PAGE_SIZE-len, "[SW ][%d] (%+3d, %+3d, %+3d)\n", 1, 
			obj->cali_sw[STK8BA22_AXIS_X], obj->cali_sw[STK8BA22_AXIS_Y], obj->cali_sw[STK8BA22_AXIS_Z]);

		len += snprintf(buf+len, PAGE_SIZE-len, "[ALL]    (%+3d, %+3d, %+3d) : (%+3d, %+3d, %+3d)\n", 
			obj->offset[STK8BA22_AXIS_X]*mul + obj->cali_sw[STK8BA22_AXIS_X],
			obj->offset[STK8BA22_AXIS_Y]*mul + obj->cali_sw[STK8BA22_AXIS_Y],
			obj->offset[STK8BA22_AXIS_Z]*mul + obj->cali_sw[STK8BA22_AXIS_Z],
			tmp[STK8BA22_AXIS_X], tmp[STK8BA22_AXIS_Y], tmp[STK8BA22_AXIS_Z]);
		
		return len;
    }
}
/*----------------------------------------------------------------------------*/
static ssize_t store_cali_value(struct device_driver *ddri, char *buf, size_t count)
{
	struct i2c_client *client = stk8ba22_i2c_client;  
	int err, x, y, z;
	int dat[STK8BA22_AXES_NUM];

	if(!strncmp(buf, "rst", 3))
	{
		if(err = STK8BA22_ResetCalibration(client))
		{
			GSE_ERR("reset offset err = %d\n", err);
		}	
	}
	else if(3 == sscanf(buf, "0x%02X 0x%02X 0x%02X", &x, &y, &z))
	{
		dat[STK8BA22_AXIS_X] = x;
		dat[STK8BA22_AXIS_Y] = y;
		dat[STK8BA22_AXIS_Z] = z;
		//if(err = STK8BA22_WriteCalibration(client, dat))
		if(err = STK8BA22_SetCali(client,1))
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
	struct stk8ba22_i2c_data *obj = obj_i2c_data;
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
	struct stk8ba22_i2c_data *obj = obj_i2c_data;
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
	struct stk8ba22_i2c_data *obj = obj_i2c_data;
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
//static DRIVER_ATTR(selftest,       S_IWUSR | S_IRUGO, show_selftest_value,          store_selftest_value);
static DRIVER_ATTR(firlen,     S_IWUSR | S_IRUGO, show_firlen_value,        store_firlen_value);
static DRIVER_ATTR(trace,      S_IWUSR | S_IRUGO, show_trace_value,         store_trace_value);
static DRIVER_ATTR(status,               S_IRUGO, show_status_value,        NULL);
/*----------------------------------------------------------------------------*/
static struct driver_attribute *stk8ba22_attr_list[] = {
	&driver_attr_chipinfo,     /*chip information*/
	&driver_attr_sensordata,   /*dump sensor data*/
	&driver_attr_cali,         /*show calibration data*/
	//&driver_attr_selftest,         /*self test demo*/
	&driver_attr_firlen,       /*filter length: 0: disable, others: enable*/
	&driver_attr_trace,        /*trace log*/
	&driver_attr_status,        
};
/*----------------------------------------------------------------------------*/
static int stk8ba22_create_attr(struct device_driver *driver) 
{
	int idx, err = 0;
	int num = (int)(sizeof(stk8ba22_attr_list)/sizeof(stk8ba22_attr_list[0]));
	if (driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
		if(err = driver_create_file(driver, stk8ba22_attr_list[idx]))
		{            
			GSE_ERR("driver_create_file (%s) = %d\n", stk8ba22_attr_list[idx]->attr.name, err);
			break;
		}
	}    
	return err;
}
/*----------------------------------------------------------------------------*/
static int stk8ba22_delete_attr(struct device_driver *driver)
{
	int idx ,err = 0;
	int num = (int)(sizeof(stk8ba22_attr_list)/sizeof(stk8ba22_attr_list[0]));

	if(driver == NULL)
	{
		return -EINVAL;
	}
	

	for(idx = 0; idx < num; idx++)
	{
		driver_remove_file(driver, stk8ba22_attr_list[idx]);
	}
	

	return err;
}

/*----------------------------------------------------------------------------*/
int gsensor_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value, sample_delay;	
	struct stk8ba22_i2c_data *priv = (struct stk8ba22_i2c_data*)self;
	hwm_sensor_data* gsensor_data;
	char buff[STK8BA22_BUFSIZE];
	
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
				err = STK8BA22_SetDelay(priv->client,value*1000);
				
				if(err != STK8BA22_SUCCESS ) //0x2C->BW=100Hz
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
					err = STK8BA22_SetPowerMode( priv->client, !sensor_power);
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
				STK8BA22_ReadSensorData(priv->client, buff, STK8BA22_BUFSIZE);
				sscanf(buff, "%x %x %x", &gsensor_data->values[0], 
					&gsensor_data->values[1], &gsensor_data->values[2]);				
				gsensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;				
				gsensor_data->value_divide = 1000;
				//GSE_ERR("X :%d,Y: %d, Z: %d\n",gsensor_data->values[0],gsensor_data->values[1],gsensor_data->values[2]);
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
static int stk8ba22_open(struct inode *inode, struct file *file)
{
	file->private_data = stk8ba22_i2c_client;

	if(file->private_data == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return -EINVAL;
	}
	return nonseekable_open(inode, file);
}
/*----------------------------------------------------------------------------*/
static int stk8ba22_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}
/*----------------------------------------------------------------------------*/
//static int stk8ba22_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
  //     unsigned long arg)
static int stk8ba22_unlocked_ioctl(struct file *file, unsigned int cmd,
       unsigned long arg)  
{
	struct i2c_client *client = (struct i2c_client*)file->private_data;
	struct stk8ba22_i2c_data *obj = (struct stk8ba22_i2c_data*)i2c_get_clientdata(client);	
	char strbuf[STK8BA22_BUFSIZE];
	void __user *data;
	SENSOR_DATA sensor_data;
	int err = 0;
	int cali[3];

	GSE_FUN(f);
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
			STK8BA22_Init_client(client, 0);			
			break;

		case GSENSOR_IOCTL_READ_CHIPINFO:
			GSE_LOG("fwq GSENSOR_IOCTL_READ_CHIPINFO\n");
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			
			STK8BA22_ReadChipInfo(client, strbuf, STK8BA22_BUFSIZE);
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
			
			STK8BA22_ReadSensorData(client, strbuf, STK8BA22_BUFSIZE);
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
			STK8BA22_ReadRawData(client, &strbuf);
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
				cali[STK8BA22_AXIS_X] = sensor_data.x * obj->reso->sensitivity / GRAVITY_EARTH_1000;
				cali[STK8BA22_AXIS_Y] = sensor_data.y * obj->reso->sensitivity / GRAVITY_EARTH_1000;
				cali[STK8BA22_AXIS_Z] = sensor_data.z * obj->reso->sensitivity / GRAVITY_EARTH_1000;			  
				err = STK8BA22_WriteCalibration(client, cali);			 
				GSE_LOG("fwq GSENSOR_IOCTL_SET_CALI!!sensor_data .x =%d,sensor_data .z =%d,sensor_data .z =%d \n",sensor_data.x,sensor_data.y,sensor_data.z);
			}
			break;

		case GSENSOR_IOCTL_CLR_CALI:
			GSE_LOG("fwq GSENSOR_IOCTL_CLR_CALI!!\n");
			err = STK8BA22_ResetCalibration(client);
			break;

		case GSENSOR_IOCTL_GET_CALI:
			GSE_LOG("fwq GSENSOR_IOCTL_GET_CALI\n");
			data = (void __user*)arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			if(err = STK8BA22_ReadCalibration(client, cali))
			{
				break;
			}
			
			sensor_data.x = cali[STK8BA22_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
			sensor_data.y = cali[STK8BA22_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
			sensor_data.z = cali[STK8BA22_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
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
static struct file_operations stk8ba22_fops = {
	//.owner = THIS_MODULE,
	.open = stk8ba22_open,
	.release = stk8ba22_release,
	.unlocked_ioctl = stk8ba22_unlocked_ioctl,
};
/*----------------------------------------------------------------------------*/
static struct miscdevice stk8ba22_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "gsensor",
	.fops = &stk8ba22_fops,
};
/*----------------------------------------------------------------------------*/
#ifndef CONFIG_HAS_EARLYSUSPEND
/*----------------------------------------------------------------------------*/
static int stk8ba22_suspend(struct i2c_client *client, pm_message_t msg) 
{
	struct stk8ba22_i2c_data *obj = i2c_get_clientdata(client);    
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
		if ((err = hwmsen_read_byte_sr(client, STK8BA22_REG_MODE, &dat))) 
		{
           GSE_ERR("read ctl_reg1  fail!!\n");
           return err;
        }
		dat = dat&0b11111110;//stand by mode
		atomic_set(&obj->suspend, 1);
		if(err = hwmsen_write_byte(client, STK8BA22_REG_MODE, dat))
		{
			GSE_ERR("write power control fail!!\n");
			return err;
		}        
		STK8BA22_power(obj->hw, 0);
	}
	return err;
}
/*----------------------------------------------------------------------------*/
static int stk8ba22_resume(struct i2c_client *client)
{
	struct stk8ba22_i2c_data *obj = i2c_get_clientdata(client);        
	int err;
	GSE_FUN();

	if(obj == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return -EINVAL;
	}

	STK8BA22_power(obj->hw, 1);
	if(err = STK8BA22_Init_client(client, 0))
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
static void stk8ba22_early_suspend(struct early_suspend *h) 
{
	struct stk8ba22_i2c_data *obj = container_of(h, struct stk8ba22_i2c_data, early_drv);   
	int err;
	GSE_FUN();    

	if(obj == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return;
	}
	atomic_set(&obj->suspend, 1); 
	/*
	if(err = hwmsen_write_byte(obj->client, STK8BA22_REG_POWER_CTL, 0x00))
	{
		GSE_ERR("write power control fail!!\n");
		return;
	}  
	*/
	if(err = STK8BA22_SetPowerMode(obj->client, false))
	{
		GSE_ERR("write power control fail!!\n");
		return;
	}

	sensor_power = false;
	
	STK8BA22_power(obj->hw, 0);
}
/*----------------------------------------------------------------------------*/
static void stk8ba22_late_resume(struct early_suspend *h)
{
	struct stk8ba22_i2c_data *obj = container_of(h, struct stk8ba22_i2c_data, early_drv);         
	int err;
	GSE_FUN();

	if(obj == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return;
	}

	STK8BA22_power(obj->hw, 1);
	if(err = STK8BA22_Init_client(obj->client, 0))
	{
		GSE_ERR("initialize client fail!!\n");
		return;        
	}
	atomic_set(&obj->suspend, 0);    
}
/*----------------------------------------------------------------------------*/
#endif /*CONFIG_HAS_EARLYSUSPEND*/
/*----------------------------------------------------------------------------*/
static int stk8ba22_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info) 
{    
	strcpy(info->type, STK8BA22_DEV_NAME);
	return 0;
}

/*----------------------------------------------------------------------------*/
static int stk8ba22_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_client *new_client;
	struct stk8ba22_i2c_data *obj;
	struct hwmsen_object sobj;
	int err = 0;
	GSE_FUN();

	if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
	{
		err = -ENOMEM;
		goto exit;
	}
	memset(obj, 0, sizeof(struct stk8ba22_i2c_data));

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
	


	stk8ba22_i2c_client = new_client;	

	if(err = STK8BA22_Init_client(new_client, 1))
	{
		goto exit_init_failed;
	}
	

	if(err = misc_register(&stk8ba22_device))
	{
		GSE_ERR("stk8ba22_device register failed\n");
		goto exit_misc_device_register_failed;
	}

	if(err = stk8ba22_create_attr(&stk8ba22_gsensor_driver.driver))
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
	obj->early_drv.suspend  = stk8ba22_early_suspend,
	obj->early_drv.resume   = stk8ba22_late_resume,    
	register_early_suspend(&obj->early_drv);
#endif 

	GSE_LOG("%s: OK\n", __func__);    
	return 0;

	exit_create_attr_failed:
	misc_deregister(&stk8ba22_device);
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
static int stk8ba22_i2c_remove(struct i2c_client *client)
{
	int err = 0;	
	
	if(err = stk8ba22_delete_attr(&stk8ba22_gsensor_driver.driver))
	{
		GSE_ERR("stk8ba22_delete_attr fail: %d\n", err);
	}
	
	if(err = misc_deregister(&stk8ba22_device))
	{
		GSE_ERR("misc_deregister fail: %d\n", err);
	}

	if(err = hwmsen_detach(ID_ACCELEROMETER))
	    

	stk8ba22_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));
	return 0;
}
/*----------------------------------------------------------------------------*/
static int stk8ba22_probe(struct platform_device *pdev) 
{
	struct acc_hw *hw = get_cust_acc_hw();
	GSE_FUN();

	STK8BA22_power(hw, 1);
	if(i2c_add_driver(&stk8ba22_i2c_driver))
	{
		GSE_ERR("add driver error\n");
		return -1;
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
static int stk8ba22_remove(struct platform_device *pdev)
{
    struct acc_hw *hw = get_cust_acc_hw();

    GSE_FUN();    
    STK8BA22_power(hw, 0);    
    i2c_del_driver(&stk8ba22_i2c_driver);
    return 0;
}
/*----------------------------------------------------------------------------*/
static struct platform_driver stk8ba22_gsensor_driver = {
	.probe      = stk8ba22_probe,
	.remove     = stk8ba22_remove,    
	.driver     = {
		.name  = "gsensor",
		.owner = THIS_MODULE,
	}
};

/*----------------------------------------------------------------------------*/
static int __init stk8ba22_init(void)
{
	GSE_FUN();
	i2c_register_board_info(0, &i2c_stk8ba22, 1);
	if(platform_driver_register(&stk8ba22_gsensor_driver))
	{
		GSE_ERR("failed to register driver");
		return -ENODEV;
	}
	return 0;    
}
/*----------------------------------------------------------------------------*/
static void __exit stk8ba22_exit(void)
{
	GSE_FUN();
	platform_driver_unregister(&stk8ba22_gsensor_driver);
}
/*----------------------------------------------------------------------------*/
module_init(stk8ba22_init);
module_exit(stk8ba22_exit);
/*----------------------------------------------------------------------------*/
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("STK8BA22 I2C driver");
MODULE_AUTHOR("Zhilin.Chen@mediatek.com");
