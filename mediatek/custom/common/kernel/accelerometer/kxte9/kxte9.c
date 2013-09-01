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
#include "kxte9.h"
#include <linux/hwmsen_helper.h>

#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>


/******************************************************************************
 * structure/enumeration
*******************************************************************************/
struct kxte9_object{
	struct i2c_client	    client;
    struct acc_hw *hw;
    struct hwmsen_convert   cvt;
  

    /*software calibration: the unit follows standard format*/
    s16                     cali_sw[KXTE9_AXES_NUM+1];     
    
    /*misc*/
    atomic_t                enable;
    atomic_t                suspend;
    atomic_t                trace;

    /*data*/
    u8                      data[KXTE9_DATA_LEN+1];
    
};

/*-------------------------MT6516&MT6573 define-------------------------------*/
#define POWER_NONE_MACRO MT65XX_POWER_NONE


/*----------------------------------------------------------------------------*/
#define KXTE9_TRC_GET_DATA  0x0001

/*----------------------------------------------------------------------------*/
#if defined(KXTE9_TEST_MODE) 
/*----------------------------------------------------------------------------*/
static struct hwmsen_reg kxte9_regs[] = {
    /*0x0C ~ 0x1F*/
    {"ST_RESP",         KXTE9_REG_ST_RESP,        REG_RO, 0xFF, 1},
    {"WHO_AM_I",        KXTE9_REG_WHO_AM_I,       REG_RO, 0xFF, 1},
    {"TILE_POS_CUR",    KXTE9_REG_TILE_POS_CUR,   REG_RO, 0x3F, 1},
    {"TILT_POS_PRE",    KXTE9_REG_TILT_POS_PRE,   REG_RO, 0x3F, 1},
    {"XOUT",            KXTE9_REG_XOUT,           REG_RO, 0xFC, 1},
    {"YOUT",            KXTE9_REG_YOUT,           REG_RO, 0xFC, 1},
    {"ZOUT",            KXTE9_REG_ZOUT,           REG_RO, 0xFC, 1},
    {"INT_SRC_REG1",    KXTE9_REG_INT_SRC_REG1,   REG_RO, 0x07, 1},
    {"INT_SRC_REG2",    KXTE9_REG_INT_SRC_REG2,   REG_RO, 0x3F, 1},
    {"STATUS_REG",      KXTE9_REG_STATUS_REG,     REG_RO, 0x3C, 1},
    {"INT_REL",         KXTE9_REG_INT_REL,        REG_RO, 0xFF, 1},
    {"CTRL_REG1",       KXTE9_REG_CTRL_REG1,      REG_RW, 0x9F, 1},
    {"CTRL_REG2",       KXTE9_REG_CTRL_REG2,      REG_RW, 0x3F, 1},        
    {"CTRL_REG3",       KXTE9_REG_CTRL_REG3,      REG_RW, 0x1F, 1}, /*transfer timeout if writing SRST as 1*/
    {"INT_CTRL_REG1",   KXTE9_REG_INT_CTRL_REG1,  REG_RW, 0x1C, 1},
    {"INT_CTRL_REG2",   KXTE9_REG_INT_CTRL_REG2,  REG_RW, 0xE0, 1},
    {"TILE_TIMER",      KXTE9_REG_TILT_TIMER,     REG_RW, 0xFF, 1},
    {"WUF_TIMER",       KXTE9_REG_WUF_TIMER,      REG_RW, 0xFF, 1},
    {"B2S_TIMER",       KXTE9_REG_B2S_TIMER,      REG_RW, 0xFF, 1},
    {"WUF_THRESH",      KXTE9_REG_WUF_THRESH,     REG_RW, 0xFF, 1},
    {"B2S_THRESH",      KXTE9_REG_B2S_THRESH,     REG_RW, 0xFF, 1},
    {"TILE_ANGLE",      KXTE9_REG_TILE_ANGLE,     REG_RW, 0xFF, 1},
    {"HYST_SET",        KXTE9_REG_HYST_SET,       REG_RW, 0xCF, 1},
 };
/*----------------------------------------------------------------------------*/
#endif

/*----------------------------------------------------------------------------*/
#define DEBUG 1
/*----------------------------------------------------------------------------*/
#define KXTE9_AXIS_X          0
#define KXTE9_AXIS_Y          1
#define KXTE9_AXIS_Z          2
#define KXTE9_AXES_NUM        3
#define KXTE9_DEV_NAME        "KXTE9"
/*----------------------------------------------------------------------------*/
static const struct i2c_device_id kxte9_i2c_id[] = {{KXTE9_DEV_NAME,0},{}};
static struct i2c_board_info __initdata i2c_kxte9={ I2C_BOARD_INFO(KXTE9_DEV_NAME, (KXTE9_WR_SLAVE_ADDR>>1))};
/*the adapter id will be available in customization*/
//static unsigned short kxte9_force[] = {0x00, KXTE9_WR_SLAVE_ADDR, I2C_CLIENT_END, I2C_CLIENT_END};
//static const unsigned short *const kxte9_forces[] = { kxte9_force, NULL };
//static struct i2c_client_address_data kxte9_addr_data = { .forces = kxte9_forces,};

/*----------------------------------------------------------------------------*/
static int kxte9_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id); 
static int kxte9_i2c_remove(struct i2c_client *client);
static int kxte9_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);


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
#define C_MAX_FIR_LENGTH (32)
/*----------------------------------------------------------------------------*/
struct data_filter {
    s16 raw[C_MAX_FIR_LENGTH][KXTE9_AXES_NUM];
    int sum[KXTE9_AXES_NUM];
    int num;
    int idx;
};
/*----------------------------------------------------------------------------*/
struct kxte9_i2c_data {
    struct i2c_client *client;
    struct acc_hw *hw;
    struct hwmsen_convert   cvt;
    
    /*misc*/
    struct data_resolution *reso;
    atomic_t                trace;
    atomic_t                suspend;
    atomic_t                selftest;
	atomic_t				filter;
    s16                     cali_sw[KXTE9_AXES_NUM+1];

    /*data*/
    s8                      offset[KXTE9_AXES_NUM+1];  /*+1: for 4-byte alignment*/
    s16                     data[KXTE9_AXES_NUM+1];

    /*early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
    struct early_suspend    early_drv;
#endif     
};
/*----------------------------------------------------------------------------*/
static struct i2c_driver kxte9_i2c_driver = {
    .driver = {
//      .owner          = THIS_MODULE,
        .name           = KXTE9_DEV_NAME,
    },
	.probe      		= kxte9_i2c_probe,
	.remove    			= kxte9_i2c_remove,
	.detect				= kxte9_i2c_detect,
#if !defined(CONFIG_HAS_EARLYSUSPEND)    
    .suspend            = kxte9_suspend,
    .resume             = kxte9_resume,
#endif
	.id_table = kxte9_i2c_id,
//	.address_data = &kxte9_addr_data,
};

/*----------------------------------------------------------------------------*/
static struct i2c_client *kxte9_i2c_client = NULL;
static struct platform_driver kxte9_gsensor_driver;
static struct kxte9_i2c_data *obj_i2c_data = NULL;
static bool sensor_power = false;
static GSENSOR_VECTOR3D gsensor_gain, gsensor_offset;


/*----------------------------------------------------------------------------*/
#define GSE_TAG                  "[Gsensor] "
#define GSE_FUN(f)               printk(KERN_INFO GSE_TAG"%s\n", __FUNCTION__)
#define GSE_ERR(fmt, args...)    printk(KERN_ERR GSE_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define GSE_LOG(fmt, args...)    printk(KERN_INFO GSE_TAG fmt, ##args)



/*--------------------Gsensor power control function----------------------------------*/
static void KXTE9_power(struct acc_hw *hw, unsigned int on) 
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
			if(!hwPowerOn(hw->power_id, hw->power_vol, "KXTE9"))
			{
				GSE_ERR("power on fails!!\n");
			}
		}
		else	// power off
		{
			if (!hwPowerDown(hw->power_id, "KXTE9"))
			{
				GSE_ERR("power off fail!!\n");
			}			  
		}
	}
	power_on = on;    
}

/*----------------------------------------------------------------------------*/
static int kxte9_write_calibration(struct kxte9_i2c_data *obj, s16 dat[KXTE9_AXES_NUM])
{
    obj->cali_sw[KXTE9_AXIS_X] = obj->cvt.sign[KXTE9_AXIS_X]*dat[obj->cvt.map[KXTE9_AXIS_X]];
    obj->cali_sw[KXTE9_AXIS_Y] = obj->cvt.sign[KXTE9_AXIS_Y]*dat[obj->cvt.map[KXTE9_AXIS_Y]];
    obj->cali_sw[KXTE9_AXIS_Z] = obj->cvt.sign[KXTE9_AXIS_Z]*dat[obj->cvt.map[KXTE9_AXIS_Z]];
    return 0;
}


/*----------------------------------------------------------------------------*/
static int KXTE9_ResetCalibration(struct i2c_client *client)
{
	struct kxte9_i2c_data *obj = i2c_get_clientdata(client);	

	memset(obj->cali_sw, 0x00, sizeof(obj->cali_sw));
	return 0;    
}
/*----------------------------------------------------------------------------*/
static int KXTE9_ReadCalibration(struct i2c_client *client, int dat[KXTE9_AXES_NUM])
{
    struct kxte9_i2c_data *obj = i2c_get_clientdata(client);

    dat[obj->cvt.map[KXTE9_AXIS_X]] = obj->cvt.sign[KXTE9_AXIS_X]*obj->cali_sw[KXTE9_AXIS_X];
    dat[obj->cvt.map[KXTE9_AXIS_Y]] = obj->cvt.sign[KXTE9_AXIS_Y]*obj->cali_sw[KXTE9_AXIS_Y];
    dat[obj->cvt.map[KXTE9_AXIS_Z]] = obj->cvt.sign[KXTE9_AXIS_Z]*obj->cali_sw[KXTE9_AXIS_Z];                        
                                       
    return 0;
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static int KXTE9_WriteCalibration(struct i2c_client *client, int dat[KXTE9_AXES_NUM])
{
	struct kxte9_i2c_data *obj = i2c_get_clientdata(client);
	int err = 0;
	int cali[KXTE9_AXES_NUM];


	GSE_FUN();
	if(!obj || ! dat)
	{
		GSE_ERR("null ptr!!\n");
		return -EINVAL;
	}
	else
	{        
		s16 cali[KXTE9_AXES_NUM];
		cali[obj->cvt.map[KXTE9_AXIS_X]] = obj->cvt.sign[KXTE9_AXIS_X]*obj->cali_sw[KXTE9_AXIS_X];
		cali[obj->cvt.map[KXTE9_AXIS_Y]] = obj->cvt.sign[KXTE9_AXIS_Y]*obj->cali_sw[KXTE9_AXIS_Y];
		cali[obj->cvt.map[KXTE9_AXIS_Z]] = obj->cvt.sign[KXTE9_AXIS_Z]*obj->cali_sw[KXTE9_AXIS_Z]; 
		cali[KXTE9_AXIS_X] += dat[KXTE9_AXIS_X];
		cali[KXTE9_AXIS_Y] += dat[KXTE9_AXIS_Y];
		cali[KXTE9_AXIS_Z] += dat[KXTE9_AXIS_Z];
		return kxte9_write_calibration(obj, cali);
	} 

	return err;
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static int KXTE9_SetPowerMode(struct i2c_client *client, bool enable)
{
	u8 databuf[2];    
	int res = 0;
	u8 addr = KXTE9_REG_CTRL_REG1;
	struct kxte9_i2c_data *obj = i2c_get_clientdata(client);
	
	
	if(enable == sensor_power)
	{
		GSE_LOG("Sensor power status is newest!\n");
		return 0;
	}

	if(hwmsen_read_block(client, addr, databuf, 0x01))
	{
		GSE_ERR("read power ctl register err!\n");
		return -1;
	}

	databuf[0] &= ~KXTE9_CTRL_PC1;
	
	if(enable == TRUE)
	{
		databuf[0] |= KXTE9_CTRL_PC1;
	}
	else
	{
		// do nothing
	}
	databuf[1] = databuf[0];
	databuf[0] = KXTE9_REG_CTRL_REG1;
	

	res = i2c_master_send(client, databuf, 0x2);

	if(res < 0)
	{
		GSE_ERR("set power mode failed!\n");
		return -1;
	}
	else if(atomic_read(&obj->trace) & ADX_TRC_INFO)
	{
		GSE_LOG("set power mode ok %d!\n", databuf[1]);
	}

	sensor_power = enable;
	return 0;    
}

#ifdef KXTE9_TEST_MODE
/*----------------------------------------------------------------------------*/
static void kxte9_single_rw(struct i2c_client *client) 
{
	hwmsen_single_rw(client, kxte9_regs, sizeof(kxte9_regs)/sizeof(kxte9_regs[0]));   
}
/*----------------------------------------------------------------------------*/
struct hwmsen_reg kxte9_dummy = HWMSEN_DUMMY_REG(0x00);
/*----------------------------------------------------------------------------*/
struct hwmsen_reg* kxte9_get_reg(int reg) 
{
	int idx;
	for(idx = 0; idx < sizeof(kxte9_regs)/sizeof(kxte9_regs[0]); idx++)
	{
		if (reg == kxte9_regs[idx].addr)
		return &kxte9_regs[idx];
	}
	return &kxte9_dummy;
}
/*----------------------------------------------------------------------------*/
static void kxte9_multi_rw(struct i2c_client *client) 
{
	struct hwmsen_reg_test_multi test_list[] = {
			{KXTE9_REG_XOUT,        3, REG_RO},
			{KXTE9_REG_TILT_TIMER,  3, REG_RW},            
			{KXTE9_REG_WUF_THRESH,  3, REG_RW},                        
			};
	hwmsen_multi_rw(client, kxte9_get_reg, test_list, sizeof(test_list)/sizeof(test_list[0]));
}
/*----------------------------------------------------------------------------*/
static void kxte9_test(struct i2c_client *client) 
{
	kxte9_single_rw(client);
	kxte9_multi_rw(client);
}
/*----------------------------------------------------------------------------*/
static ssize_t kxte9_show_dump(struct device *dev, 
									struct device_attribute *attr, char* buf)
{
	/*the register name starting from THRESH_TAP*/
#define REG_OFFSET      0x00 
#define REG_TABLE_LEN   0x5F 
	static u8 kxte9_reg_value[REG_TABLE_LEN] = {0};
	struct i2c_client *client = to_i2c_client(dev);    

	return hwmsen_show_dump(client, REG_OFFSET, kxte9_reg_value, REG_TABLE_LEN,
							kxte9_get_reg, buf, PAGE_SIZE);
}
/*----------------------------------------------------------------------------*/
#endif /*KXTE9_TEST_MODE*/

/*----------------------------------------------------------------------------*/
static int kxte9_init_hw(struct i2c_client* client)
{
	int err = 0;
	GSE_LOG("sensor test: kxte9_init_hw function!\n");
	if(!client)
	{
		return -EINVAL;
	}
	if(err = hwmsen_write_byte(client, KXTE9_REG_CTRL_REG2, KXTE9_TILE_POS_MASK))
	{
		GSE_ERR("write KXTE9_REG_CTRL_REG3 fail!!\n");
		return err;
	}

	/*configure output rate*/
	if(err = hwmsen_write_byte(client, KXTE9_REG_CTRL_REG3, KXTE9_ODR_ACTIVE|KXTE9_ODR_INACTIVE))
	{
		GSE_ERR("write KXTE9_REG_CTRL_REG2 fail!!\n");
		return err;
	}

	/*disable all interrupt*/
	if(err = hwmsen_write_byte(client, KXTE9_REG_INT_CTRL_REG2, 0x00))
	{
		GSE_ERR("write KXTE9_REG_INT_CTRL_REG2 fail!!\n");
		return err;
	}

	/*setup default ODR*/
	if(err = hwmsen_write_byte(client, KXTE9_REG_CTRL_REG1, KXTE9_ODR_DEFAULT))
	{
		GSE_ERR("write KXTE9_REG_CTRL_REG1 fail!!\n");
		return err;
	}
	return err;
}

/*----------------------------------------------------------------------------*/
static int KXTE9_Init_client(struct i2c_client *client, int reset_cali)
{

#ifdef KXTE9_TEST_MODE
	kxte9_test(client);
#endif
	gsensor_offset.x = gsensor_offset.y = gsensor_offset.z = 2048;
	gsensor_gain.x = gsensor_gain.y = gsensor_gain.z = KXTE9_SENSITIVITY;

	return kxte9_init_hw(client);
}
/*----------------------------------------------------------------------------*/
static int KXTE9_ReadChipInfo(struct i2c_client *client, char *buf, int bufsize)
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

	sprintf(buf, "KXTE9 Chip");
	return 0;
}

/*----------------------------------------------------------------------------*/
static int kxte9_read_data(struct i2c_client *client, u8 data[KXTE9_AXES_NUM])
{
	u8 addr = KXTE9_REG_XOUT;
	int err = 0;

	if(!client)
	{
		err = -EINVAL;
	}
	else if(err = hwmsen_read_block(client, addr, data, KXTE9_DATA_LEN))
	{
		GSE_ERR("error: %d\n", err);
	}
	
	return err;
}


/******************************************************************************
 * Functions 
******************************************************************************/
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static int kxte9_to_standard_format(struct kxte9_object *obj, u8 dat[KXTE9_AXES_NUM],
                                    s16 out[KXTE9_AXES_NUM])
{   

	/*convert original KXTE9 data t*/
	out[KXTE9_AXIS_X] = (KXTE9_DATA_COUNT(dat[KXTE9_AXIS_X]) - KXTE9_0G_OFFSET);
	out[KXTE9_AXIS_Y] = (KXTE9_DATA_COUNT(dat[KXTE9_AXIS_Y]) - KXTE9_0G_OFFSET);
	out[KXTE9_AXIS_Z] = (KXTE9_DATA_COUNT(dat[KXTE9_AXIS_Z]) - KXTE9_0G_OFFSET);
	
	return 0;
}


/*----------------------------------------------------------------------------*/
static ssize_t KXTE9_ReadSensorData(struct i2c_client *client, char *buf, int bufsize)
{
	
	struct kxte9_i2c_data *obj = (struct kxte9_i2c_data*)i2c_get_clientdata(client);
	s16 output[KXTE9_AXES_NUM];
	u8 data[KXTE9_AXES_NUM];
	int res = 0;

	if(sensor_power == FALSE)
	{
		res = KXTE9_SetPowerMode(client, true);
		if(res)
		{
			GSE_ERR("Power on kxte9 error %d!\n", res);
		}
	}
	
	if(!obj || !buf)
	{
		GSE_ERR("null pointer");
		return -EINVAL;
	}
	

	if(res = kxte9_read_data(obj->client, data))
	{
		GSE_ERR("read data failed!!");
		return -EINVAL;
	}

	/*convert to mg: 1000 = 1g*/
	if(res = kxte9_to_standard_format(obj, data, output))
	{
		GSE_ERR("convert to standard format fail:%d\n",res);
		return -EINVAL;
	}

	obj->data[KXTE9_AXIS_X] = output[KXTE9_AXIS_X] + obj->cali_sw[KXTE9_AXIS_X];
	obj->data[KXTE9_AXIS_Y] = output[KXTE9_AXIS_Y] + obj->cali_sw[KXTE9_AXIS_Y];
	obj->data[KXTE9_AXIS_Z] = output[KXTE9_AXIS_Z] + obj->cali_sw[KXTE9_AXIS_Z];
	
	/*remap coordinate*/
	output[obj->cvt.map[KXTE9_AXIS_X]] = obj->cvt.sign[KXTE9_AXIS_X]*obj->data[KXTE9_AXIS_X];
	output[obj->cvt.map[KXTE9_AXIS_Y]] = obj->cvt.sign[KXTE9_AXIS_Y]*obj->data[KXTE9_AXIS_Y];
	output[obj->cvt.map[KXTE9_AXIS_Z]] = obj->cvt.sign[KXTE9_AXIS_Z]*obj->data[KXTE9_AXIS_Z];

	//GSE_LOG("Mapped gsensor data: %d, %d, %d!\n", acc[KXTE9_AXIS_X], acc[KXTE9_AXIS_Y], acc[KXTE9_AXIS_Z]);

	//Out put the mg
	output[KXTE9_AXIS_X] = output[KXTE9_AXIS_X] * GRAVITY_EARTH_1000 / KXTE9_SENSITIVITY;
	output[KXTE9_AXIS_Y] = output[KXTE9_AXIS_Y] * GRAVITY_EARTH_1000 / KXTE9_SENSITIVITY;
	output[KXTE9_AXIS_Z] = output[KXTE9_AXIS_Z] * GRAVITY_EARTH_1000 / KXTE9_SENSITIVITY;	

	if(atomic_read(&obj->trace) & KXTE9_TRC_GET_DATA)
	{
		GSE_LOG("%d (0x%08X, 0x%08X, 0x%08X) -> (%5d, %5d, %5d)\n", KXTE9_SENSITIVITY,
			obj->data[KXTE9_AXIS_X], obj->data[KXTE9_AXIS_Y], obj->data[KXTE9_AXIS_Z],
			output[KXTE9_AXIS_X],output[KXTE9_AXIS_Y],output[KXTE9_AXIS_Z]);
	}

	sprintf(buf, "%04x %04x %04x", output[KXTE9_AXIS_X], output[KXTE9_AXIS_Y], output[KXTE9_AXIS_Z]);
    return 0;
}
/*----------------------------------------------------------------------------*/
static int KXTE9_ReadRawData(struct i2c_client *client, char *buf)
{
	char buff[KXTE9_BUFSIZE];
	s16 data[3];

	
	KXTE9_ReadSensorData(client, buff, KXTE9_BUFSIZE);
	sscanf(buff, "%x %x %x", data[0], data[1],data[2]);

	data[0] = data[0] * KXTE9_SENSITIVITY / GRAVITY_EARTH_1000;
	data[1] = data[1] * KXTE9_SENSITIVITY / GRAVITY_EARTH_1000;
	data[2] = data[2] * KXTE9_SENSITIVITY / GRAVITY_EARTH_1000;

	sprintf(buf, "%04x %04x %04x", data[0],data[1],data[2]);
	return 0;
}


/*----------------------------------------------------------------------------*/
static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = kxte9_i2c_client;
	char strbuf[KXTE9_BUFSIZE];
	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	
	KXTE9_ReadChipInfo(client, strbuf, KXTE9_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);        
}
/*----------------------------------------------------------------------------*/
static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = kxte9_i2c_client;
	char strbuf[KXTE9_BUFSIZE];
	
	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	KXTE9_ReadSensorData(client, strbuf, KXTE9_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);            
}
/*----------------------------------------------------------------------------*/
static ssize_t show_cali_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = kxte9_i2c_client;
	struct kxte9_i2c_data *obj;

	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}

	obj = i2c_get_clientdata(client);

	int err, len = 0;
	int tmp[KXTE9_AXES_NUM];

	if(err = KXTE9_ReadCalibration(client, tmp))
	{
		return -EINVAL;
	}
	else
	{		
		len += snprintf(buf+len, PAGE_SIZE-len, "[SW ][%d] (%+3d, %+3d, %+3d)\n", 1, 
			obj->cali_sw[KXTE9_AXIS_X], obj->cali_sw[KXTE9_AXIS_Y], obj->cali_sw[KXTE9_AXIS_Z]);
		
		return len;
    }
}
/*----------------------------------------------------------------------------*/
static ssize_t store_cali_value(struct device_driver *ddri, char *buf, size_t count)
{
	struct i2c_client *client = kxte9_i2c_client;  
	int err, x, y, z;
	int dat[KXTE9_AXES_NUM];

	if(!strncmp(buf, "rst", 3))
	{
		if(err = KXTE9_ResetCalibration(client))
		{
			GSE_ERR("reset offset err = %d\n", err);
		}	
	}
	else if(3 == sscanf(buf, "0x%02X 0x%02X 0x%02X", &x, &y, &z))
	{
		dat[KXTE9_AXIS_X] = x;
		dat[KXTE9_AXIS_Y] = y;
		dat[KXTE9_AXIS_Z] = z;
		if(err = KXTE9_WriteCalibration(client, dat))
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
static ssize_t show_trace_value(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	struct kxte9_i2c_data *obj = obj_i2c_data;
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
	struct kxte9_i2c_data *obj = obj_i2c_data;
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
	struct kxte9_i2c_data *obj = obj_i2c_data;
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
static DRIVER_ATTR(trace,      S_IWUSR | S_IRUGO, show_trace_value,         store_trace_value);
static DRIVER_ATTR(status,               S_IRUGO, show_status_value,        NULL);
/*----------------------------------------------------------------------------*/
static struct driver_attribute *kxte9_attr_list[] = {
	&driver_attr_chipinfo,     /*chip information*/
	&driver_attr_sensordata,   /*dump sensor data*/
	&driver_attr_cali,         /*show calibration data*/
	&driver_attr_trace,        /*trace log*/
	&driver_attr_status,        
};

/*----------------------------------------------------------------------------*/
static int kxte9_create_attr(struct device_driver *driver) 
{
	int idx, err = 0;
	int num = (int)(sizeof(kxte9_attr_list)/sizeof(kxte9_attr_list[0]));
	if (driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
		if(err = driver_create_file(driver, kxte9_attr_list[idx]))
		{            
			GSE_ERR("driver_create_file (%s) = %d\n", kxte9_attr_list[idx]->attr.name, err);
			break;
		}
	}    
	return err;
}
/*----------------------------------------------------------------------------*/
static int kxte9_delete_attr(struct device_driver *driver)
{
	int idx ,err = 0;
	int num = (int)(sizeof(kxte9_attr_list)/sizeof(kxte9_attr_list[0]));

	if(driver == NULL)
	{
		return -EINVAL;
	}
	

	for(idx = 0; idx < num; idx++)
	{
		driver_remove_file(driver, kxte9_attr_list[idx]);
	}
	

	return err;
}

/*----------------------------------------------------------------------------*/
int gsensor_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value, sample_delay;	
	struct kxte9_i2c_data *priv = (struct kxte9_i2c_data*)self;
	hwm_sensor_data* gsensor_data;
	char buff[KXTE9_BUFSIZE];
	
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
					err = KXTE9_SetPowerMode( priv->client, !sensor_power);
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
				KXTE9_ReadSensorData(priv->client, buff, KXTE9_BUFSIZE);
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
static int kxte9_open(struct inode *inode, struct file *file)
{
	file->private_data = kxte9_i2c_client;

	if(file->private_data == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return -EINVAL;
	}
	return nonseekable_open(inode, file);
}
/*----------------------------------------------------------------------------*/
static int kxte9_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}
/*----------------------------------------------------------------------------*/
//static int kxte9_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
//       unsigned long arg)
static long kxte9_unlocked_ioctl(struct file *file, unsigned int cmd,
       unsigned long arg)
{
	struct i2c_client *client = (struct i2c_client*)file->private_data;
	struct kxte9_i2c_data *obj = (struct kxte9_i2c_data*)i2c_get_clientdata(client);	
	char strbuf[KXTE9_BUFSIZE];
	void __user *data;
	SENSOR_DATA sensor_data;
	long err = 0;
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
			KXTE9_Init_client(client, 0);			
			break;

		case GSENSOR_IOCTL_READ_CHIPINFO:
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			
			KXTE9_ReadChipInfo(client, strbuf, KXTE9_BUFSIZE);
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
			
			KXTE9_ReadSensorData(client, strbuf, KXTE9_BUFSIZE);
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
			KXTE9_ReadRawData(client, &strbuf);
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
				cali[KXTE9_AXIS_X] = sensor_data.x * KXTE9_SENSITIVITY / GRAVITY_EARTH_1000;
				cali[KXTE9_AXIS_Y] = sensor_data.y * KXTE9_SENSITIVITY / GRAVITY_EARTH_1000;
				cali[KXTE9_AXIS_Z] = sensor_data.z * KXTE9_SENSITIVITY / GRAVITY_EARTH_1000;			  
				err = KXTE9_WriteCalibration(client, cali);			 
			}
			break;

		case GSENSOR_IOCTL_CLR_CALI:
			err = KXTE9_ResetCalibration(client);
			break;

		case GSENSOR_IOCTL_GET_CALI:
			data = (void __user*)arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			if(err = KXTE9_ReadCalibration(client, cali))
			{
				break;
			}
			
			sensor_data.x = cali[KXTE9_AXIS_X] * GRAVITY_EARTH_1000 / KXTE9_SENSITIVITY;
			sensor_data.y = cali[KXTE9_AXIS_Y] * GRAVITY_EARTH_1000 / KXTE9_SENSITIVITY;
			sensor_data.z = cali[KXTE9_AXIS_Z] * GRAVITY_EARTH_1000 / KXTE9_SENSITIVITY;
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
static struct file_operations kxte9_fops = {
	.owner = THIS_MODULE,
	.open = kxte9_open,
	.release = kxte9_release,
	.unlocked_ioctl = kxte9_unlocked_ioctl,
	//.ioctl = kxte9_ioctl,
};
/*----------------------------------------------------------------------------*/
static struct miscdevice kxte9_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "gsensor",
	.fops = &kxte9_fops,
};
/*----------------------------------------------------------------------------*/
#ifndef CONFIG_HAS_EARLYSUSPEND
/*----------------------------------------------------------------------------*/
static int kxte9_suspend(struct i2c_client *client, pm_message_t msg) 
{
	struct kxte9_i2c_data *obj = i2c_get_clientdata(client);    
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
		if(err = hwmsen_write_byte(client, KXTE9_REG_CTRL_REG1, 0x00))
		{
			GSE_ERR("write power control fail!!\n");
			return err;
		}        
		//KXTE9_power(obj->hw, 0);
	}
	return err;
}
/*----------------------------------------------------------------------------*/
static int kxte9_resume(struct i2c_client *client)
{
	struct kxte9_i2c_data *obj = i2c_get_clientdata(client);        
	int err;
	GSE_FUN();

	if(obj == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return -EINVAL;
	}

	//KXTE9_power(obj->hw, 1);
	if(err = KXTE9_Init_client(client, 0))
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
static void kxte9_early_suspend(struct early_suspend *h) 
{
	struct kxte9_i2c_data *obj = container_of(h, struct kxte9_i2c_data, early_drv);   
	int err;
	GSE_FUN();    

	if(obj == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return;
	}
	atomic_set(&obj->suspend, 1);
	if(err = KXTE9_SetPowerMode(obj->client, false))
	{
		GSE_ERR("write power control fail!!\n");
		return;
	}

	sensor_power = false;
	
	//KXTE9_power(obj->hw, 0);
}
/*----------------------------------------------------------------------------*/
static void kxte9_late_resume(struct early_suspend *h)
{
	struct kxte9_i2c_data *obj = container_of(h, struct kxte9_i2c_data, early_drv);         
	int err;
	GSE_FUN();

	if(obj == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return;
	}

	//KXTE9_power(obj->hw, 1);
	if(err = KXTE9_Init_client(obj->client, 0))
	{
		GSE_ERR("initialize client fail!!\n");
		return;        
	}
	atomic_set(&obj->suspend, 0);    
}
/*----------------------------------------------------------------------------*/
#endif /*CONFIG_HAS_EARLYSUSPEND*/
/*----------------------------------------------------------------------------*/
static int kxte9_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info) 
{    
	strcpy(info->type, KXTE9_DEV_NAME);
	return 0;
}

/*----------------------------------------------------------------------------*/
static int kxte9_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_client *new_client;
	struct kxte9_i2c_data *obj;
	struct hwmsen_object sobj;
	int err = 0;
	GSE_FUN();

	if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
	{
		err = -ENOMEM;
		goto exit;
	}
	
	memset(obj, 0, sizeof(struct kxte9_i2c_data));

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

	kxte9_i2c_client = new_client;	

	if(err = KXTE9_Init_client(new_client, 1))
	{
		goto exit_init_failed;
	}
	

	if(err = misc_register(&kxte9_device))
	{
		GSE_ERR("kxte9_device register failed\n");
		goto exit_misc_device_register_failed;
	}

	if(err = kxte9_create_attr(&kxte9_gsensor_driver.driver))
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
	obj->early_drv.suspend  = kxte9_early_suspend,
	obj->early_drv.resume   = kxte9_late_resume,    
	register_early_suspend(&obj->early_drv);
#endif 

	GSE_LOG("%s: OK\n", __func__);    
	return 0;

	exit_create_attr_failed:
	misc_deregister(&kxte9_device);
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
static int kxte9_i2c_remove(struct i2c_client *client)
{
	int err = 0;	
	
	if(err = kxte9_delete_attr(&kxte9_gsensor_driver.driver))
	{
		GSE_ERR("kxte9_delete_attr fail: %d\n", err);
	}
	
	if(err = misc_deregister(&kxte9_device))
	{
		GSE_ERR("misc_deregister fail: %d\n", err);
	}

	if(err = hwmsen_detach(ID_ACCELEROMETER))
	    

	kxte9_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));
	return 0;
}
/*----------------------------------------------------------------------------*/
static int kxte9_probe(struct platform_device *pdev) 
{
	struct acc_hw *hw = get_cust_acc_hw();
	GSE_FUN();

	KXTE9_power(hw, 1);
	//kxte9_force[0] = hw->i2c_num;
	if(i2c_add_driver(&kxte9_i2c_driver))
	{
		GSE_ERR("add driver error\n");
		return -1;
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
static int kxte9_remove(struct platform_device *pdev)
{
    struct acc_hw *hw = get_cust_acc_hw();

    GSE_FUN();    
//    KXTE9_power(hw, 0);    
    i2c_del_driver(&kxte9_i2c_driver);
    return 0;
}
/*----------------------------------------------------------------------------*/
static struct platform_driver kxte9_gsensor_driver = {
	.probe      = kxte9_probe,
	.remove     = kxte9_remove,    
	.driver     = {
		.name  = "gsensor",
		.owner = THIS_MODULE,
	}
};

/*----------------------------------------------------------------------------*/
static int __init kxte9_init(void)
{
	GSE_FUN();
	struct acc_hw *hw = get_cust_acc_hw();
	GSE_LOG("%s: i2c_number=%d\n", __func__,hw->i2c_num); 
	i2c_register_board_info(hw->i2c_num, &i2c_kxte9, 1);
	if(platform_driver_register(&kxte9_gsensor_driver))
	{
		GSE_ERR("failed to register driver");
		return -ENODEV;
	}
	return 0;    
}
/*----------------------------------------------------------------------------*/
static void __exit kxte9_exit(void)
{
	GSE_FUN();
	platform_driver_unregister(&kxte9_gsensor_driver);
}
/*----------------------------------------------------------------------------*/
module_init(kxte9_init);
module_exit(kxte9_exit);
/*----------------------------------------------------------------------------*/
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("KXTE9 I2C driver");
MODULE_AUTHOR("Chunlei.Wang@mediatek.com");

