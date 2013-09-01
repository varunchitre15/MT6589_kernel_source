/* drivers/i2c/chips/afa750.c - afa750 motion sensor driver
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

#include <linux/hwmsen_helper.h>

#include <mach/mt_devs.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>

/*-------------------------MT6516&MT6573 define-------------------------------*/
#define POWER_NONE_MACRO MT65XX_POWER_NONE




#include "afa750.h"


/*----------------------------------------------------------------------------*/
#define I2C_DRIVERID_AFA750 750
/*----------------------------------------------------------------------------*/
#define DEBUG 1
/*----------------------------------------------------------------------------*/
//#define CONFIG_AFA750_LOWPASS   /*apply low pass filter on output*/
/*----------------------------------------------------------------------------*/
#define AFA750_AXIS_X          0
#define AFA750_AXIS_Y          1
#define AFA750_AXIS_Z          2
#define AFA750_AXES_NUM        3
#define AFA750_DATA_LEN        6
#define AFA750_DEV_NAME        "afa750"
/*----------------------------------------------------------------------------*/
static const struct i2c_device_id AFA750_i2c_id[] = {{AFA750_DEV_NAME,0},{}};
/*the adapter id will be available in customization*/
//static unsigned short AFA750_force[] = {0x00, AFA750_I2C_SLAVE_ADDR, I2C_CLIENT_END, I2C_CLIENT_END};
//static const unsigned short *const AFA750_forces[] = { AFA750_force, NULL };
//static struct i2c_client_address_data AFA750_addr_data = { .forces = AFA750_forces};
static struct i2c_board_info __initdata i2c_AFA750={ I2C_BOARD_INFO(AFA750_DEV_NAME,(AFA750_I2C_SLAVE_ADDR>>1))};
/*----------------------------------------------------------------------------*/
static int AFA750_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int AFA750_i2c_remove(struct i2c_client *client);
//static int AFA750_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);

/*----------------------------------------------------------------------------*/
static int  AFA750_local_init(void);
static int AFA750_remove(void);
/************** AFA750 **********/
#define AFA750_LIBHWM_GRAVITY_EARTH            9807

typedef struct {
    int offset[3];
	int gain[3];
}AFA750_CALI_INFO;

extern void ConsAP_OneTouchCalibration(struct i2c_client *client, void* getCali); //CUS
AFA750_CALI_INFO sensor_data;
SENSOR_DATA sensor_data2;
SENSOR_DATA sensor_data3;
/*********************************/
//static int AFA750_init_flag =0;
#if 0
static struct sensor_init_info AFA750_init_info = {
        .name = "afa750",
        .init = AFA750_local_init,
        .uninit = AFA750_remove,
};
#endif
/*------------------------------------------------------------------------------*/

typedef enum {
    ADX_TRC_FILTER  = 0x01,
    ADX_TRC_RAWDATA = 0x02,
    ADX_TRC_IOCTL   = 0x04,
    ADX_TRC_CALI    = 0X08,
    ADX_TRC_INFO    = 0X10,
    ADX_TRC_REGXYZ    = 0X20,
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
    s16 raw[C_MAX_FIR_LENGTH][AFA750_AXES_NUM];
    int sum[AFA750_AXES_NUM];
    int num;
    int idx;
};
/*----------------------------------------------------------------------------*/
struct AFA750_i2c_data {
    struct i2c_client *client;
    struct acc_hw *hw;
    struct hwmsen_convert   cvt;

    /*misc*/
    struct data_resolution *reso;
	atomic_t 				layout;
    atomic_t                trace;
    atomic_t                suspend;
    atomic_t                selftest;
    atomic_t                filter;
    s16                     cali_sw[AFA750_AXES_NUM+1];

    /*data*/
    s8                      offset[AFA750_AXES_NUM+1];  /*+1: for 4-byte alignment*/
    s16                     data[AFA750_AXES_NUM+1];

    /*early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
    struct early_suspend    early_drv;
#endif
};
/*----------------------------------------------------------------------------*/
static struct i2c_driver AFA750_i2c_driver = {
    .driver = {
//        .owner          = THIS_MODULE,
        .name           = AFA750_DEV_NAME,
    },
    .probe              = AFA750_i2c_probe,
    .remove                = AFA750_i2c_remove,
//    .detect                = AFA750_i2c_detect,
#if !defined(CONFIG_HAS_EARLYSUSPEND)
    .suspend            = AFA750_suspend,
    .resume             = AFA750_resume,
#endif
    .id_table = AFA750_i2c_id,
   // .address_data = &AFA750_addr_data,
};

/*----------------------------------------------------------------------------*/
static struct i2c_client *AFA750_i2c_client = NULL;
static struct platform_driver AFA750_gsensor_driver;
static struct AFA750_i2c_data *obj_i2c_data = NULL;
static bool sensor_power = false;
static GSENSOR_VECTOR3D gsensor_gain, gsensor_offset;
static char selftestRes[10] = {0};

/*----------------------------------------------------------------------------*/
#define GSE_TAG                  "[Gsensor afa750] "
#define GSE_FUN(f)               printk(KERN_INFO GSE_TAG"%s\n", __FUNCTION__)
#define GSE_ERR(fmt, args...)    printk(KERN_ERR GSE_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define GSE_LOG(fmt, args...)    printk(KERN_NOTICE GSE_TAG fmt, ##args)
/*----------------------------------------------------------------------------*/

static void AFA750_power(struct acc_hw *hw, unsigned int on)
{
    static unsigned int power_on = 0;

    if(hw->power_id != POWER_NONE_MACRO)        // have externel LDO
    {
        GSE_LOG("power %s\n", on ? "on" : "off");
        if(power_on == on)    // power status not change
        {
            GSE_LOG("ignore power control: %d\n", on);
        }
        else if(on)    // power on
        {
            if(!hwPowerOn(hw->power_id, hw->power_vol, "afa750"))
            {
                GSE_ERR("power on fails!!\n");
            }
        }
        else    // power off
        {
            if (!hwPowerDown(hw->power_id, "afa750"))
            {
                GSE_ERR("power off fail!!\n");
            }
        }
    }
    power_on = on;
}

s16 AFA750_Cali_X(s16 Cnt_X){

	 GSE_LOG("Frances : OffsetX  %d\n", sensor_data2.x);

     Cnt_X -= sensor_data2.x;

	 GSE_LOG("Frances : output CNTX %f\n", Cnt_X);
	 
	 return Cnt_X;
 }
 
s16 AFA750_Cali_Y(s16 Cnt_Y){

	 GSE_LOG("Frances : OffsetY  %d\n", sensor_data2.y);

     Cnt_Y -= sensor_data2.y;

	 GSE_LOG("Frances : output CNTY %f\n", Cnt_Y);
		
	 return Cnt_Y;
 }
 
 s16 AFA750_Cali_Z(s16 Cnt_Z){

	 GSE_LOG("Frances : OffsetZ  %d\n", sensor_data2.z);

     Cnt_Z -= sensor_data2.z;
		
	 GSE_LOG("Frances : output CNTZ %f\n", Cnt_Z);
		
	 return Cnt_Z;
 }

/*----------------------------------------------------------------------------*/
static int AFA750_ReadData(struct i2c_client *client, s16 data[AFA750_AXES_NUM])
{
	printk("sz--- %s \n",__func__);
    struct AFA750_i2c_data *priv = i2c_get_clientdata(client);
    u8 addr = AFA750_REG_DATAX0;
    u8 buf[AFA750_DATA_LEN] = {0};
    int err = 0;
    int res = 0xffffffff;
		
    if(NULL == client)
    {
        err = -EINVAL;
    }
    else
    {
        buf[0] = AFA750_REG_DATAX0;
        client->addr = client->addr & I2C_MASK_FLAG | I2C_WR_FLAG |I2C_RS_FLAG;
        res = i2c_master_send(client, (const char*)&buf, 6<<8 | 1);
        client->addr = client->addr & I2C_MASK_FLAG;

        data[AFA750_AXIS_X] = (s16)((buf[AFA750_AXIS_X*2]) |
                 (buf[AFA750_AXIS_X*2+1] << 8));
        data[AFA750_AXIS_Y] = (s16)((buf[AFA750_AXIS_Y*2]) |
                 (buf[AFA750_AXIS_Y*2+1] << 8));
        data[AFA750_AXIS_Z] = (s16)((buf[AFA750_AXIS_Z*2]) |
                 (buf[AFA750_AXIS_Z*2+1] << 8));
		printk("AFA before: cnt = %d, %d, %d\n", data[AFA750_AXIS_X], data[AFA750_AXIS_Y], data[AFA750_AXIS_Z]);
        

				 
		data[AFA750_AXIS_X] = AFA750_Cali_X(data[AFA750_AXIS_X]);
		data[AFA750_AXIS_Y] = AFA750_Cali_Y(data[AFA750_AXIS_Y]);
		data[AFA750_AXIS_Z] = AFA750_Cali_Z(data[AFA750_AXIS_Z]);
        printk("AFA:AFA750_Cali--cnt 0 = %d, %d, %d\n", AFA750_Cali_X(data[AFA750_AXIS_X]), AFA750_Cali_Y(data[AFA750_AXIS_Y]), AFA750_Cali_Z(data[AFA750_AXIS_Z]));
        printk("AFA: cnt 1 = %d, %d, %d\n", data[AFA750_AXIS_X], data[AFA750_AXIS_Y], data[AFA750_AXIS_Z]);
    }
    return err;
}

/*----------------------------------------------------------------------------*/
static int AFA750_CheckDeviceID(struct i2c_client *client)
{
    //return AFA750_SUCCESS;
#if 1
    u8 databuf[10];
    int res = 0;
    int status = 0;
	
    memset(databuf, 0, sizeof(u8)*10);
    databuf[0] = AFA750_REG_WHO_AM_I;
    client->addr = client->addr & I2C_MASK_FLAG | I2C_WR_FLAG |I2C_RS_FLAG;
   res = i2c_master_send(client,(const char*)&databuf, 1<<8 |1);	
    printk(" sz---- fwq afa750 res=%d!\n", res);
   if(res<0)
   {
    printk(" sz---- fwq afa750 id %x!\n", databuf[0]);
   }
	client->addr = client->addr& I2C_MASK_FLAG;
    
    GSE_LOG(" fwq afa750 id %x!\n", databuf[0]);
    printk("afa750 client->addr = 0x%x\n",client->addr);
 
   /* if ((databuf[0]&0xFE) != (AFA750_I2C_SLAVE_ADDR &0xFE))
    {
     	GSE_LOG("afa750 check id error\n");   
        return AFA750_ERR_IDENTIFICATION;
    }

exit_AFA750_CheckDeviceID:
    if (res < 0)
    {
        return AFA750_ERR_I2C;
    }*/
#endif
    return AFA750_SUCCESS;
}

/*----------------------------------------------------------------------------*/
static int AFA750_Init(struct i2c_client *client, int reset_cali)
{
    //return AFA750_SUCCESS;
    struct AFA750_i2c_data *obj = i2c_get_clientdata(client);
    int res = 0;
    u8 buf[AFA750_DATA_LEN] = {0};
    GSE_LOG("2010-11-03-11:43 fwq AFA750 addr %x!\n",client->addr);

    res = AFA750_CheckDeviceID(client);
    if(res != AFA750_SUCCESS)
    {
        GSE_LOG("fwq AFA750 check id error\n");
        return res;
    }
/**/

	/*
    buf[0] = AFA750_REG_DATAX0;
    client->addr = client->addr & I2C_MASK_FLAG | I2C_WR_FLAG |I2C_RS_FLAG;
    res = i2c_master_send(client, (const char*)&buf, 6<<8 | 1);
    client->addr = client->addr& I2C_MASK_FLAG;

    if(buf[1] == 0 && buf[2] == 0 &&
       buf[3] == 0 && buf[4] == 0 &&
       buf[5] == 0){
       return AFA750_ERR_I2C;
    }
	*/
    gsensor_gain.x = gsensor_gain.y = gsensor_gain.z = 3568; //obj->reso->sensitivity;

    GSE_LOG("fwq AFA750 Init OK\n");
    return AFA750_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int AFA750_ReadChipInfo(struct i2c_client *client, char *buf, int bufsize)
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

    sprintf(buf, "AFA750 Chip");
    return 0;
}
/*----------------------------------------------------------------------------*/
static int AFA750_ReadSensorData(struct i2c_client *client, char *buf, int bufsize)
{
    struct AFA750_i2c_data *obj = (struct AFA750_i2c_data*)i2c_get_clientdata(client);
    u8 databuf[20];
    int acc[AFA750_AXES_NUM];
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

    if(res = AFA750_ReadData(client, obj->data))
	{        
		GSE_ERR("I2C error: ret value=%d", res);
		return -3;
	}
#if 1
    else
    {
        /*
        obj->data[AFA750_AXIS_X] += obj->cali_sw[AFA750_AXIS_X];
        obj->data[AFA750_AXIS_Y] += obj->cali_sw[AFA750_AXIS_Y];
        obj->data[AFA750_AXIS_Z] += obj->cali_sw[AFA750_AXIS_Z];
        */
        /*remap coordinate*/
#if 1
        acc[obj->cvt.map[AFA750_AXIS_X]] = obj->cvt.sign[AFA750_AXIS_X]*obj->data[AFA750_AXIS_X];
        acc[obj->cvt.map[AFA750_AXIS_Y]] = obj->cvt.sign[AFA750_AXIS_Y]*obj->data[AFA750_AXIS_Y];
        acc[obj->cvt.map[AFA750_AXIS_Z]] = obj->cvt.sign[AFA750_AXIS_Z]*obj->data[AFA750_AXIS_Z];
#else
        acc[AFA750_AXIS_X] = obj->cvt.sign[AFA750_AXIS_X]*obj->data[AFA750_AXIS_X];
        acc[AFA750_AXIS_Y] = obj->cvt.sign[AFA750_AXIS_Y]*obj->data[AFA750_AXIS_Y];
        acc[AFA750_AXIS_Z] = obj->cvt.sign[AFA750_AXIS_Z]*obj->data[AFA750_AXIS_Z];
#endif
        //GSE_LOG("Mapped gsensor data: %d, %d, %d!\n", acc[AFA750_AXIS_X], acc[AFA750_AXIS_Y], acc[AFA750_AXIS_Z]);

        //Out put the mg
		acc[AFA750_AXIS_X] = (s32)acc[AFA750_AXIS_X]/*acc[AFA750_AXIS_X]*/ * GRAVITY_EARTH_1000 / 3568; //obj->reso->sensitivity;
		acc[AFA750_AXIS_Y] = (s32)acc[AFA750_AXIS_Y]/*acc[AFA750_AXIS_Y]*/ * GRAVITY_EARTH_1000 / 3568; //obj->reso->sensitivity;
		acc[AFA750_AXIS_Z] = (s32)acc[AFA750_AXIS_Z]/*acc[AFA750_AXIS_Z]*/ * GRAVITY_EARTH_1000 / 3568; //obj->reso->sensitivity;		

        /*sprintf(buf, "%d,%d,%d,  sensitivity x=%d",
                        acc[AFA750_AXIS_X], acc[AFA750_AXIS_Y], acc[AFA750_AXIS_Z], gsensor_gain.x);
        GSE_LOG("gsensor data: %s!\n", buf);
        */
        sprintf(buf, "%04x %04x %04x", acc[AFA750_AXIS_X], acc[AFA750_AXIS_Y], acc[AFA750_AXIS_Z]);
    }
#endif
    return 0;
}
/*----------------------------------------------------------------------------*/
static int AFA750_ReadRawData(struct i2c_client *client, char *buf)
{
    //return 0;
    struct AFA750_i2c_data *obj = (struct AFA750_i2c_data*)i2c_get_clientdata(client);
    int res = 0;

    if (!buf || !client)
    {
        return EINVAL;
    }

    if(res = AFA750_ReadData(client, obj->data))
    {
        GSE_ERR("I2C error: ret value=%d", res);
        return EIO;
    }
    else
    {
        sprintf(buf, "%04x %04x %04x", obj->data[AFA750_AXIS_X],
            obj->data[AFA750_AXIS_Y], obj->data[AFA750_AXIS_Z]);

    }

    return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
    GSE_LOG("fwq show_chipinfo_value \n");
    struct i2c_client *client = AFA750_i2c_client;
    char strbuf[AFA750_BUFSIZE];
    if(NULL == client)
    {
        GSE_ERR("i2c client is null!!\n");
        return 0;
    }

    AFA750_ReadChipInfo(client, strbuf, AFA750_BUFSIZE);
    return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}
/*----------------------------------------------------------------------------*/
static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{
    struct i2c_client *client = AFA750_i2c_client;
    char strbuf[AFA750_BUFSIZE];

    if(NULL == client)
    {
        GSE_ERR("i2c client is null!!\n");
        return 0;
    }
    AFA750_ReadSensorData(client, strbuf, AFA750_BUFSIZE);
    return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}
/*----------------------------------------------------------------------------*/
static ssize_t show_layout_value(struct device_driver *ddri, char *buf)
{
        struct i2c_client *client = AFA750_i2c_client;
        struct AFA750_i2c_data *data = i2c_get_clientdata(client);

        return sprintf(buf, "(%d, %d)\n[%+2d %+2d %+2d]\n[%+2d %+2d %+2d]\n",
                       data->hw->direction, atomic_read(&data->layout), data->cvt.sign[0], data->cvt.sign[1],
                       data->cvt.sign[2],data->cvt.map[0], data->cvt.map[1], data->cvt.map[2]);
}
/*----------------------------------------------------------------------------*/
static ssize_t store_layout_value(struct device_driver *ddri, const char *buf, size_t count)
{
        struct i2c_client *client = AFA750_i2c_client;
        struct AFA750_i2c_data *data = i2c_get_clientdata(client);
        int layout = 0;

        if (sscanf(buf, "%d", &layout) == 1) {
                atomic_set(&data->layout, layout);
                if (!hwmsen_get_convert(layout, &data->cvt)) {
                        printk(KERN_ERR "HWMSEN_GET_CONVERT function error!\n");
                } else if (!hwmsen_get_convert(data->hw->direction, &data->cvt)) {
                        printk(KERN_ERR "invalid layout: %d, restore to %d\n", layout, data->hw->direction);
                } else {
                        printk(KERN_ERR "invalid layout: (%d, %d)\n", layout, data->hw->direction);
                        hwmsen_get_convert(0, &data->cvt);
                }
        } else {
                printk(KERN_ERR "invalid format = '%s'\n", buf);
        }

        return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t show_trace_value(struct device_driver *ddri, char *buf)
{
    GSE_LOG("fwq show_trace_value \n");
    ssize_t res;
    struct AFA750_i2c_data *obj = obj_i2c_data;
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
    struct AFA750_i2c_data *obj = obj_i2c_data;
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
    struct AFA750_i2c_data *obj = obj_i2c_data;
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
static DRIVER_ATTR(trace,      S_IWUSR | S_IRUGO, show_trace_value,         store_trace_value);
static DRIVER_ATTR(layout,      S_IRUGO | S_IWUSR, show_layout_value, store_layout_value);
static DRIVER_ATTR(status,               S_IRUGO, show_status_value,        NULL);
/*----------------------------------------------------------------------------*/
static struct driver_attribute *AFA750_attr_list[] = {
    &driver_attr_chipinfo,     /*chip information*/
    &driver_attr_sensordata,   /*dump sensor data*/
    &driver_attr_trace,        /*trace log*/
    &driver_attr_layout,
    &driver_attr_status,
};
/*----------------------------------------------------------------------------*/
static int AFA750_create_attr(struct device_driver *driver)
{
    int idx, err = 0;
    int num = (int)(sizeof(AFA750_attr_list)/sizeof(AFA750_attr_list[0]));
    if (driver == NULL)
    {
        return -EINVAL;
    }

    for(idx = 0; idx < num; idx++)
    {
        if(err = driver_create_file(driver, AFA750_attr_list[idx]))
        {
            GSE_ERR("driver_create_file (%s) = %d\n", AFA750_attr_list[idx]->attr.name, err);
            break;
        }
    }
    return err;
}
/*----------------------------------------------------------------------------*/
static int AFA750_delete_attr(struct device_driver *driver)
{
    int idx ,err = 0;
    int num = (int)(sizeof(AFA750_attr_list)/sizeof(AFA750_attr_list[0]));

    if(driver == NULL)
    {
        return -EINVAL;
    }

    for(idx = 0; idx < num; idx++)
    {
        driver_remove_file(driver, AFA750_attr_list[idx]);
    }

    return err;
}

/*----------------------------------------------------------------------------*/
int AFA750_operate(void* self, uint32_t command, void* buff_in, int size_in,
        void* buff_out, int size_out, int* actualout)
{
    int err = 0;
    int value, sample_delay;
    struct AFA750_i2c_data *priv = (struct AFA750_i2c_data*)self;
    hwm_sensor_data* gsensor_data;
    char buff[AFA750_BUFSIZE];

    //GSE_FUN(f);
    switch (command)
    {
        case SENSOR_DELAY:
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
                    //err = AFA750_SetPowerMode( priv->client, !sensor_power);
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
                AFA750_ReadSensorData(priv->client, buff, AFA750_BUFSIZE);
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
static int AFA750_open(struct inode *inode, struct file *file)
{
    file->private_data = AFA750_i2c_client;

    if(file->private_data == NULL)
    {
        GSE_ERR("null pointer!!\n");
        return -EINVAL;
    }
    return nonseekable_open(inode, file);
}
/*----------------------------------------------------------------------------*/
static int AFA750_release(struct inode *inode, struct file *file)
{
    file->private_data = NULL;
    return 0;
}
/*----------------------------------------------------------------------------*/

static int AFA750_unlocked_ioctl(struct file *file, unsigned int cmd,unsigned long arg)
{
    struct i2c_client *client = (struct i2c_client*)file->private_data;
printk("sz----- %s  %d\n",__func__,__LINE__);
    struct AFA750_i2c_data *obj = (struct AFA750_i2c_data*)i2c_get_clientdata(client);
    char strbuf[AFA750_BUFSIZE];
    void __user *data;

    int err = 0;
    int cali[3];

printk("sz----- %s  %d\n",__func__,__LINE__);
    //GSE_FUN(f);
    if(_IOC_DIR(cmd) & _IOC_READ)
    {
        err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    }
    else if(_IOC_DIR(cmd) & _IOC_WRITE)
    {
        err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
    }
printk("sz-----  %d\n",__LINE__);
    if(err)
    {
        GSE_ERR("access error: %08X, (%2d, %2d)\n", cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));
        return -EFAULT;
    }

    switch(cmd)
    {
        case GSENSOR_IOCTL_INIT:
            GSE_LOG("fwq GSENSOR_IOCTL_INIT\n");
            AFA750_Init(client, 0);
            break;

        case GSENSOR_IOCTL_READ_CHIPINFO:
            GSE_LOG("fwq GSENSOR_IOCTL_READ_CHIPINFO\n");
            data = (void __user *) arg;
            if(data == NULL)
            {
                err = -EINVAL;
                break;
            }

            AFA750_ReadChipInfo(client, strbuf, AFA750_BUFSIZE);
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

            AFA750_ReadSensorData(client, strbuf, AFA750_BUFSIZE);
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
            gsensor_offset.x = gsensor_offset.y = gsensor_offset.z = 0;
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
            AFA750_ReadRawData(client, &strbuf);
            if(copy_to_user(data, &strbuf, strlen(strbuf)+1))
            {
                err = -EFAULT;
                break;
            }
            break;

        case GSENSOR_IOCTL_SET_CALI:
			printk("Frances : start to call GSENSOR_IOCTL_SSSET_CALI\n");
			
		    data = (void __user*)arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			if(copy_from_user(&sensor_data2, data, sizeof(sensor_data2)))
			{
				err = -EFAULT;
				break;	  
			}

			printk("afa750 before : sensor_data2.x=%d,sensor_data2.y=%d,sensor_data2.z=%d\n",sensor_data2.x,sensor_data2.y,sensor_data2.z);

			sensor_data2.x = (sensor_data2.x*3568/AFA750_LIBHWM_GRAVITY_EARTH);
			sensor_data2.y = (sensor_data2.y*3568/AFA750_LIBHWM_GRAVITY_EARTH);
			sensor_data2.z = (sensor_data2.z*3568/AFA750_LIBHWM_GRAVITY_EARTH);
			printk("afa750: sensor_data2.x=%d,sensor_data2.y=%d,sensor_data2.z=%d\n",sensor_data2.x,sensor_data2.y,sensor_data2.z);
            break;		
            
        case GSENSOR_IOCTL_CLR_CALI:
            break;

        case GSENSOR_IOCTL_GET_CALI:
		    printk("Frances : start to call GSENSOR_IOCTL_GET_CALI\n");
			data = (void __user*)arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			
			ConsAP_OneTouchCalibration(client, (void *)&sensor_data);
		    printk("afa750 before : sensor_data.offset[0]=%d,sensor_data.offset[1]=%d,sensor_data.offset[2]=%d\n",sensor_data.offset[0],sensor_data.offset[1],sensor_data.offset[2]);
			printk("afa750:before : sensor_data3.x=%d,sensor_data3.y=%d,sensor_data3.z=%d\n",sensor_data3.x,sensor_data3.y,sensor_data3.z);

            sensor_data3.x = (sensor_data.offset[0]*AFA750_LIBHWM_GRAVITY_EARTH/3568);
		    sensor_data3.y = (sensor_data.offset[1]*AFA750_LIBHWM_GRAVITY_EARTH/3568);
		    sensor_data3.z = (sensor_data.offset[2]*AFA750_LIBHWM_GRAVITY_EARTH/3568);

		    sensor_data2.x = (sensor_data3.x*3568/AFA750_LIBHWM_GRAVITY_EARTH);
			sensor_data2.y = (sensor_data3.y*3568/AFA750_LIBHWM_GRAVITY_EARTH);
			sensor_data2.z = (sensor_data3.z*3568/AFA750_LIBHWM_GRAVITY_EARTH);
			
			if(copy_to_user(data, &sensor_data3, sizeof(sensor_data3)))
			{
				err = -EFAULT;
				break;
			}	

			printk("afa750: sensor_data3.x=%d,sensor_data3.y=%d,sensor_data3.z=%d\n",sensor_data3.x,sensor_data3.y,sensor_data3.z);
		    break;
			
        default:
            GSE_ERR("unknown IOCTL: 0x%08x\n", cmd);
            err = -ENOIOCTLCMD;
            break;

    }

    return err;
}


/*----------------------------------------------------------------------------*/
static struct file_operations AFA750_fops = {
//    .owner = THIS_MODULE,
    .open = AFA750_open,
    .release = AFA750_release,
    .unlocked_ioctl = AFA750_unlocked_ioctl,
};
/*----------------------------------------------------------------------------*/
static struct miscdevice AFA750_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "gsensor",
    .fops = &AFA750_fops,
};
/*----------------------------------------------------------------------------*/
#ifndef CONFIG_HAS_EARLYSUSPEND
/*----------------------------------------------------------------------------*/
static int AFA750_suspend(struct i2c_client *client, pm_message_t msg)
{
    struct AFA750_i2c_data *obj = i2c_get_clientdata(client);
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
        if ((err = hwmsen_read_byte_sr(client, AFA750_REG_CTL_REG1, &dat)))
        {
           GSE_ERR("read ctl_reg1  fail!!\n");
           return err;
        }
        //dat = 0x02;//stand by mode
        atomic_set(&obj->suspend, 1);

        AFA750_power(obj->hw, 0);
    }
    return err;
}
/*----------------------------------------------------------------------------*/
static int AFA750_resume(struct i2c_client *client)
{
    struct AFA750_i2c_data *obj = i2c_get_clientdata(client);
    int err;
    GSE_FUN();

    if(obj == NULL)
    {
        GSE_ERR("null pointer!!\n");
        return -EINVAL;
    }

    AFA750_power(obj->hw, 1);
    /*
    if(err = AFA750_Init(client, 0))
    {
        GSE_ERR("initialize client fail!!\n");
        return err;
    }
    */
    atomic_set(&obj->suspend, 0);

    return 0;
}
/*----------------------------------------------------------------------------*/
#else /*CONFIG_HAS_EARLY_SUSPEND is defined*/
/*----------------------------------------------------------------------------*/
static void AFA750_early_suspend(struct early_suspend *h)
{
    struct AFA750_i2c_data *obj = container_of(h, struct AFA750_i2c_data, early_drv);
    int err;
    GSE_FUN();

    if(obj == NULL)
    {
        GSE_ERR("null pointer!!\n");
        return;
    }
    atomic_set(&obj->suspend, 1);

    //sensor_power = false;

    AFA750_power(obj->hw, 0);
}
/*----------------------------------------------------------------------------*/
static void AFA750_late_resume(struct early_suspend *h)
{
    struct AFA750_i2c_data *obj = container_of(h, struct AFA750_i2c_data, early_drv);
    int err;
    GSE_FUN();

    if(obj == NULL)
    {
        GSE_ERR("null pointer!!\n");
        return;
    }

    AFA750_power(obj->hw, 1);
    if(err = AFA750_Init(obj->client, 0))
    {
        GSE_ERR("initialize client fail!!\n");
        return;
    }
    atomic_set(&obj->suspend, 0);
}
/*----------------------------------------------------------------------------*/
#endif /*CONFIG_HAS_EARLYSUSPEND*/
/*----------------------------------------------------------------------------*/
static int AFA750_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info)
{
    strcpy(info->type, AFA750_DEV_NAME);
    return 0;
}

/*----------------------------------------------------------------------------*/
static int AFA750_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct i2c_client *new_client;
    struct AFA750_i2c_data *obj;
    struct hwmsen_object sobj;
    int err = 0;
    GSE_FUN();

    if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
    {
        err = -ENOMEM;
        goto exit;
    }

    memset(obj, 0, sizeof(struct AFA750_i2c_data));

    obj->hw = get_cust_acc_hw();

    if(err = hwmsen_get_convert(obj->hw->direction, &obj->cvt))
    {
        GSE_ERR("invalid direction: %d\n", obj->hw->direction);
        goto exit;
    }

    obj_i2c_data = obj;
    obj->client = client;
    new_client = obj->client;
	printk("sz---new_client=%d\n",new_client);
    i2c_set_clientdata(new_client,obj);

	atomic_set(&obj->layout, obj->hw->direction);
    atomic_set(&obj->trace, 0);
    atomic_set(&obj->suspend, 0);

    AFA750_i2c_client = new_client;

    if(err = AFA750_Init(new_client, 0))
    {
        goto exit_init_failed;
    }
    sensor_power = true;


    if(err = misc_register(&AFA750_device))
    {
        GSE_ERR("AFA750_device register failed\n");
        goto exit_misc_device_register_failed;
    }

    //if(err = AFA750_create_attr(&(AFA750_init_info.platform_diver_addr->driver)))
    if(err = AFA750_create_attr(&AFA750_gsensor_driver.driver))
    {
        GSE_ERR("create attribute err = %d\n", err);
        goto exit_create_attr_failed;
    }

    sobj.self = obj;
    sobj.polling = 1;
    sobj.sensor_operate = AFA750_operate;
    if(err = hwmsen_attach(ID_ACCELEROMETER, &sobj))
    {
        GSE_ERR("attach fail = %d\n", err);
        goto exit_kfree;
    }

#ifdef CONFIG_HAS_EARLYSUSPEND
    obj->early_drv.level    = EARLY_SUSPEND_LEVEL_STOP_DRAWING - 2,
    obj->early_drv.suspend  = AFA750_early_suspend,
    obj->early_drv.resume   = AFA750_late_resume,
    register_early_suspend(&obj->early_drv);
#endif
    GSE_LOG("%s: OK\n", __func__);
	AFA750_ReadData(client, obj->data);
   // AFA750_init_flag = 0;
    return 0;

exit_create_attr_failed:
    misc_deregister(&AFA750_device);
exit_misc_device_register_failed:
exit_init_failed:
    //i2c_detach_client(new_client);
exit_kfree:
    kfree(obj);
exit:
    //
    //platform_driver_unregister(&AFA750_gsensor_driver);
    GSE_ERR("%s: err = %d\n", __func__, err);
    //AFA750_init_flag = -1;
    return err;
}

/*----------------------------------------------------------------------------*/
static int AFA750_i2c_remove(struct i2c_client *client)
{
    int err = 0;

 //   if(err = AFA750_delete_attr(&(AFA750_init_info.platform_diver_addr->driver)))
    if(err = AFA750_delete_attr(&(AFA750_gsensor_driver.driver)))
    {
        GSE_ERR("AFA750_delete_attr fail: %d\n", err);
    }

    if(err = misc_deregister(&AFA750_device))
    {
        GSE_ERR("misc_deregister fail: %d\n", err);
    }

    if(err = hwmsen_detach(ID_ACCELEROMETER))


    AFA750_i2c_client = NULL;
    i2c_unregister_device(client);
    kfree(i2c_get_clientdata(client));//
    return 0;
}
/*----------------------------------------------------------------------------*/
#if 0
static int  AFA750_local_init(void)
{
    struct acc_hw *hw = get_cust_acc_hw();
    GSE_FUN();

    AFA750_power(hw, 1);
//    AFA750_force[0] = hw->i2c_num;
   if(i2c_add_driver(&AFA750_i2c_driver))
    {
        GSE_ERR("add driver error\n");
        return -1;
    }
  /*  if(-1 == AFA750_init_flag)
    {
       return -1;
    }*/
    return 0;
}
#endif
static int AFA750_probe(struct platform_device *pdev) 
{
    struct acc_hw *hw = get_cust_acc_hw();
    GSE_FUN();

    AFA750_power(hw, 1);
   if(i2c_add_driver(&AFA750_i2c_driver))
    {
        GSE_ERR("add driver error\n");
        return -1;
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
static int AFA750_remove(void)
{
    struct acc_hw *hw = get_cust_acc_hw();

    GSE_FUN();
    AFA750_power(hw, 0);
    i2c_del_driver(&AFA750_i2c_driver);
    return 0;
}
/*----------------------------------------------------------------------------*/
static struct platform_driver AFA750_gsensor_driver = {
     .probe      = AFA750_probe,
	 .remove     = AFA750_remove,
	 .driver     = {
		 .name  = "gsensor",
		 //.owner = THIS_MODULE,
	 }
};
/*----------------------------------------------------------------------------*/
static int __init AFA750_init(void)
{

    GSE_FUN();
	struct acc_hw *hw = get_cust_acc_hw();
	i2c_register_board_info(hw->i2c_num, &i2c_AFA750, 1);
//	hwmsen_gsensor_add(&AFA750_init_info);
	 if(platform_driver_register(&AFA750_gsensor_driver))
	 {
		 GSE_ERR("failed to register driver");
		 return -ENODEV;
	 }

    return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit AFA750_exit(void)
{
    GSE_FUN();
  	platform_driver_unregister(&AFA750_gsensor_driver);
}
/*----------------------------------------------------------------------------*/
module_init(AFA750_init);
module_exit(AFA750_exit);
/*----------------------------------------------------------------------------*/
MODULE_LICENSE("GPL");

