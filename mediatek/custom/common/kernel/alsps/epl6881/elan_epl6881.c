/* drivers/i2c/chips/elan_epl6801.c - light and proxmity sensors driver
 * Copyright (C) 2011 ELAN Corporation.
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

#include <linux/hrtimer.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <asm/mach-types.h>
#include <asm/setup.h>
#include <linux/wakelock.h>
#include <linux/jiffies.h>
#include "elan_interface.h"
#include <linux/regulator/consumer.h>
#include <linux/platform_device.h>
#include <linux/hwmsen_dev.h>
#include <cust_alsps.h>
//#include <mach/mt_devs.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>

#define TXBYTES                             2
#define RXBYTES                             2

#define PS_POLLING_RATE 		300
#define DUAL_POLLING_RATE	 	1000
#define ALS_POLLING_RATE 		1000

#define PACKAGE_SIZE 			2
#define I2C_RETRY_COUNT 		10

#define P_INTT 					1

//incandescent and fluorescent parameter
#define ALS_MAX_COUNT			60000
#define ALS_MIN_COUNT			15  			//ambient light mode dynamic integration time ADC low threshold
#define ALS_MAX_LUX				60000	  	//ambient light mode MAX lux
#define ALS_MIN_LUX				0


#define ALS_I_THRESHOLD	     0x180	// incandescent threshold (1.55859375)
#define ALS_F_THRESHOLD	     0x380	// fluorescent threshold (2.08984375)
#define ALS_M_THRESHOLD	     0x1c0	// incandescent and fluorescent mix threshold (1.59765625)

#define ALS_AX2		     0x2240
#define ALS_BX		     0xaf80
#define ALS_C		     0xb180
#define ALS_AX		     0x1494A
#define ALS_B		     0x20028
#define ALS_L                  0x1A0

// 0 for power-saving, 1 for less cpu communication
#define PS_IRQ_ENABLED         	0

//<2013/04/01-23428-kevincheng , Modify ALSPS PARA
//<2013/05/27-25335-kevincheng,Modify ALS Calibration only for 500 Lux
#define P_SENSOR_LTHD			 96//8000///1200
#define P_SENSOR_HTHD			120//96//10000///1500
//>2013/04/01-23428-kevincheng
//<2013/05/23-25250-kevincheng , Modify ALSPS APK to 830/500 Lux
int lux_per_count = 500;//1700;//380;//1000;
//>2013/05/27-25335-kevincheng
//>2013/05/23-25250-kevincheng
//<2013/05/31-25693-kevincheng , PS Saturste issue
int CH1_value = 1;
int CH1_data = 0;
int CH0_Saturate = 0;
//>2013/05/31--kevincheng

struct epl6881_i2c_addr {   
	u8  epl_addr;   
};

static void epl_sensor_irq_do_work(struct work_struct *work);
static DECLARE_WORK(epl_sensor_irq_work, epl_sensor_irq_do_work);
static void polling_do_work(struct work_struct *work);
static DECLARE_DELAYED_WORK(polling_work, polling_do_work);
static void ps_polling_do_work(struct work_struct *work);
static DECLARE_DELAYED_WORK(ps_polling_work, ps_polling_do_work);

//als integration time table (us)
static uint16_t als_intT_table[] =
{
    1,
    8,
    16,
    32,
    64,
    128,
    256,
    512,
    640,
    768,
    1024,
    2048,
    4096,
    6144,
    8192,
    10240
};

//selection integration time from als_intT_table
static uint8_t als_intT_selection[] =
{
    1,
    5,//5,//5,//5,//5,//7,
};

/* primitive raw data from I2C */
typedef struct _epl_raw_data
{
    u8 raw_bytes[PACKAGE_SIZE];
    u16 ps_raw;
    u16 als_ch0_raw;
    u16 als_ch1_raw;
    uint32_t lux;
    uint32_t ratio;
} epl_raw_data;

struct elan_epl_data
{
    struct alsps_hw	*hw;
    struct i2c_client *client;
    struct input_dev *als_input_dev;
    struct input_dev *ps_input_dev;
    struct workqueue_struct *epl_wq;
    struct delayed_work polling_work; 
    struct delayed_work epl_sensor_irq_work;
    struct early_suspend early_suspend;

    /*i2c address group*/
    struct epl6881_i2c_addr  addr;

    int intr_pin;
    int (*power)(int on);

    int ps_opened;
    int als_opened;

    int enable_pflag;
    int enable_lflag;
    int read_flag;
    int irq;

    int ps_is_first_frame ;
    uint16_t ps_irq_enabled;

    uint32_t als_i_threshold;
    uint32_t als_f_threshold;
    uint32_t als_m_threshold;

    uint32_t als_ax2;
    uint32_t als_bx;
    uint32_t als_c;
    uint32_t als_ax;
    uint32_t als_b;

    uint8_t epl_irq_pin_state;
    uint8_t epl_adc_item;
    uint8_t epl_average_item;
    uint8_t epl_integrationtime_item;
    uint16_t epl_integrationtime;
    uint8_t epl_integrationtime_index;
    uint8_t epl_intpersistency;
    uint16_t epl_intthreshold_hvalue;
    uint16_t epl_intthreshold_lvalue;
    uint8_t epl_op_mode;
    uint8_t epl_sc_mode;
    uint8_t epl_sensor_gain_item;
    uint8_t epl_return_status;
    u8 mode_flag;
} ;

//<<EricHsieh,2012/10/29 ,add the wakelock for Psensor
static struct wake_lock g_ps_wlock;
//>>EricHsieh,2012/10/29 ,add the wakelock for Psensor
struct elan_epl_data *epl_data;
static struct mutex als_enable_mutex, ps_enable_mutex,als_get_sensor_value_mutex,ps_get_sensor_value_mutex;
static epl_raw_data	gRawData;

static const char ElanPsensorName[]="proximity";
static const char ElanALsensorName[]="light";

static bool poverflag = false;
static int psensor_mode_suspend = 0;

/******************************************************************************
 * configuration
*******************************************************************************/

#define EPL6881_I2C_ADDR_EPL 0   
#define on 1
#define ELAN_LS_6881 "elan-epl6881"
#define true 1
#define false 0
int _is_ps_first_frame = 1;
int _is_als_first_frame = 1;
int _als_reinit = 0;

#define APS_TAG                  "[ELAN ALS/PS] "
#define APS_FUN(f)               printk(KERN_INFO APS_TAG"%s\n", __FUNCTION__)
#define APS_ERR(fmt, args...)    printk(KERN_ERR  APS_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define APS_LOG(fmt, args...)    printk(KERN_INFO APS_TAG fmt, ##args)
#define APS_DBG(fmt, args...)    printk(KERN_INFO fmt, ##args)                 

/****************************************************************************
 * DEBUG MACROS
 ***************************************************************************/
static int debug_enable_data = 1;
static int debug_enable_interface = 1;
#define EPL_DEBUG_DATA(format, args...) do{ \
	if(debug_enable_data) \
	{\
		printk(KERN_EMERG format,##args);\
	}\
}while(0)

#define EPL_DEBUG_INTERFACE(format, args...) do{ \
	if(debug_enable_interface) \
	{\
		printk(KERN_EMERG format,##args);\
	}\
}while(0)


static struct i2c_board_info __initdata epl6881_i2c_alsps={ I2C_BOARD_INFO("elan-epl6881", (0x92>>1))};
//<2013/05/31-25693-kevincheng,PS Saturate issue  
int crit_limit = 2200;
//>2013/05/31-25693-kevincheng
//<2013/04/01-23428-kevincheng , Modify ALSPS PARA
int threshold_base = 100;//600;
//>2013/04/01-23428-kevincheng
int Dyna_en = 0;

void epl6881_calibration_polling(void);
int calibaration_result = -2;
//>2012/09/10-13741-kevincheng
/******************************************************************************/

/*
//====================I2C write operation===============//
//regaddr: ELAN epl6801 Register Address.
//bytecount: How many bytes to be written to epl6801 register via i2c bus.
//txbyte: I2C bus transmit byte(s). Single byte(0X01) transmit only slave 
address.
//data: setting value.
//
// Example: If you want to write single byte to 0x1D register address, show 
below
//	      elan_epl6801_I2C_Write(client,0x1D,0x01,0X02,0xff);
//
*/
static int elan_epl6881_I2C_Write(struct i2c_client *client, uint8_t regaddr, 
uint8_t bytecount, uint8_t txbyte, uint8_t data)
{
    uint8_t buffer[2];
    int ret = 0;
    int retry,val;
    struct elan_epl_data *epld = epl_data;

    buffer[0] = (regaddr<<3) | bytecount ;
    buffer[1] = data;


    //EPL_DEBUG_DATA("[ELAN epl6881] ---Send regaddr(0x%x);buffer data (0x%x) (0x%x)---\n",regaddr,buffer[0],buffer[1]);

    for(retry = 0; retry < I2C_RETRY_COUNT; retry++)
    {
        ret = i2c_master_send(client, buffer, txbyte);

        if (ret == txbyte)
        {
            break;
        }

        val = gpio_get_value(epld->intr_pin);

        //EPL_DEBUG_DATA("[ELAN epl6881] %s, INTERRUPT GPIO val = %d\n", __func__, val);

        msleep(10);
    }

    if(retry>=I2C_RETRY_COUNT)
    {
        EPL_DEBUG_DATA(KERN_ERR "[ELAN epl6881 error] %s i2c write retry over %d\n",__func__, I2C_RETRY_COUNT);
        return -EINVAL;
    }

    return ret;
}


static int elan_epl6881_I2C_Read(struct i2c_client *client)
{
    uint8_t buffer[RXBYTES];
    int ret = 0, i =0;
    int retry,val;
    struct elan_epl_data *epld = epl_data;

    for(retry = 0; retry < I2C_RETRY_COUNT; retry++)
    {

        ret = i2c_master_recv(client, buffer, RXBYTES);

        if (ret == RXBYTES)
            break;

        val = gpio_get_value(epld->intr_pin);

        //EPL_DEBUG_DATA("[ELAN epl6881] %s, INTERRUPT GPIO val = %d\n", __func__, val);

        //printk("i2c read error,RXBYTES %d\r\n",ret);
        msleep(10);
    }

    if(retry>=I2C_RETRY_COUNT)
    {
        EPL_DEBUG_DATA(KERN_ERR "[ELAN epl6881 error] %s i2c read retry over %d\n",__func__, I2C_RETRY_COUNT);
        return -EINVAL;
    }

    for(i=0; i<PACKAGE_SIZE; i++)
    {
        gRawData.raw_bytes[i] = buffer[i];
    }

    //EPL_DEBUG_DATA("[ELAN epl6881]---Receive data byte1 (0x%x) byte2 (0x%x)---\n",buffer[0],buffer[1]);
//<2012/09/13 ShermanWei, Fix Error :<HWMSEN> hwmsen_enable 521 : activate sensor(4)
        if (ret == RXBYTES)
            ret = 0;
//>2012/09/13 ShermanWei

    return ret;
	
}


unsigned char cal_h[10];
unsigned char cal_l [10];
int cal_h_val=0;
int cal_l_val = 0;

int elan_calibration_atoi(char* s)
{
    int num=0,flag=0;
    int i=0;

    EPL_DEBUG_INTERFACE("[ELAN epl6881] %s , %s\n", __func__,s);
	
    for(i=0; i<=strlen(s); i++)
    {
        if(s[i] >= '0' && s[i] <= '9')
            num = num * 10 + s[i] -'0';

        else if(s[0] == '-' && i==0)
            flag =1;
        else

            break;

    }
    if(flag == 1)
        num = num * -1;
    return num;

}

int elan_calibaration_read(void)
{

    struct file *fp_h;
    struct file *fp_l;
    mm_segment_t fs;
    loff_t pos;
    cal_h_val = 0;
    cal_l_val = 0;

    EPL_DEBUG_INTERFACE("[ELAN epl6881] %s\n", __func__);

    fp_h = filp_open("/data/nrstdata/H-threshold.dat", O_RDONLY, S_IRUSR);
    ///fp_h = filp_open("/data/nrstdata/H-threshold.dat", O_RDWR | O_CREAT, 0777);
    if (IS_ERR(fp_h))
    {
        EPL_DEBUG_INTERFACE("[ELAN]create file_h error\n");
        return -1;
    }

    fp_l = filp_open("/data/nrstdata/L-threshold.dat", O_RDONLY, S_IRUSR);
    ///fp_l = filp_open("/data/nrstdata/L-threshold.dat", O_RDWR | O_CREAT, 0777);
    if (IS_ERR(fp_l))
    {
        EPL_DEBUG_INTERFACE("[ELAN]create file_l error\n");
        return -1;
    }

    fs = get_fs();
    set_fs(KERNEL_DS);
    pos = 0;
    vfs_read(fp_h, cal_h, sizeof(cal_h), &pos);
    pos = 0;
    vfs_read(fp_l, cal_l, sizeof(cal_l), &pos);

    cal_h_val = elan_calibration_atoi(cal_h);
    cal_l_val = elan_calibration_atoi(cal_l);

    EPL_DEBUG_INTERFACE("[ELAN] read cal_h: %s , cal_l : %s\n", cal_h,cal_l);
    EPL_DEBUG_INTERFACE("[ELAN] read cal_h_val: %d , cal_l_value : %d\n", cal_h_val,cal_l_val);

    filp_close(fp_h, NULL);
    filp_close(fp_l, NULL);
    set_fs(fs);
    return 0;

}

static int elan_epl6881_psensor_enable(struct elan_epl_data *epld)
{
    int ret;
    uint8_t regdata = 0;
    struct i2c_client *client = epld->client;

    EPL_DEBUG_INTERFACE("[ELAN epl6881]--- Proximity sensor Enable --- \n");
    
    //set register 0
    mutex_lock(&ps_enable_mutex);

//<2012/10/31 ShermanWei ,PCBA version without Dynamic Calibration
    #if defined ARIMA_PCBA_RELEASE
        calibaration_result=elan_calibaration_read();
        EPL_DEBUG_INTERFACE("[ELAN]read check : %d , cal_h_val : %d\n", calibaration_result,cal_h_val);
	if(cal_h_val != 0) {
            epld->epl_intthreshold_lvalue = cal_l_val;
            epld->epl_intthreshold_hvalue = cal_h_val;
        }
    #endif
//>2012/10/31 ShermanWei
    epl_data->ps_is_first_frame = 1;
    epl_data->epl_average_item = EPL_SENSING_1_TIME;
    epl_data->epl_sc_mode = EPL_C_SENSING_MODE;//epld->ps_irq_enabled ? EPL_C_SENSING_MODE : EPL_S_SENSING_MODE;
    epl_data->epl_op_mode = EPL_PS_MODE;
    epl_data->epl_sensor_gain_item = EPL_M_GAIN;
    regdata = epl_data->epl_average_item | epl_data->epl_sc_mode | epl_data->epl_op_mode | epl_data->epl_sensor_gain_item;
    ret = elan_epl6881_I2C_Write(client,REG_0,W_SINGLE_BYTE,0X02,regdata);

    //set register 1
    epl_data->epl_intpersistency = EPL_PST_1_TIME;
    epl_data->epl_adc_item = EPL_10BIT_ADC;
    regdata = P_INTT << 4 | epl_data->epl_intpersistency | epl_data->epl_adc_item;
    ret = elan_epl6881_I2C_Write(client,REG_1,W_SINGLE_BYTE,0X02,regdata);

    //set register 9
//<2012/11/12-16731-kevincheng,Fix light sensor issue
    //elan_epl6881_I2C_Write(client,REG_9,W_SINGLE_BYTE,0x02,EPL_INT_FRAME_ENABLE);  //setting interrupt pin = binary
    elan_epl6881_I2C_Write(client,REG_9,W_SINGLE_BYTE,0x02,EPL_INT_DISABLE);  //setting interrupt pin = disable modify by Robert 2012/11/12
//>2012/11/12-16731-kevincheng

    /*restart sensor*/
    elan_epl6881_I2C_Write(client,REG_7,W_SINGLE_BYTE,0X02,EPL_C_RESET);
    ret = elan_epl6881_I2C_Write(client,REG_7,W_SINGLE_BYTE,0x02,EPL_C_START_RUN);
    EPL_DEBUG_INTERFACE("[ELAN]--- Proximity sensor setting finish --- \n");
    msleep(2);
#if 1
    if (ret != 0x02)
    {
        epld->enable_pflag = 0;
        EPL_DEBUG_INTERFACE("[ELAN EPL6801] P-sensor i2c err\n");
    }
    else
    {
//<2012/10/26 ShermanWei, for power consumption, reduce 20mA in phone call mode
        ////wake_lock(&g_ps_wlock);
//>2012/10/26 ShermanWei
    }
#endif
   mutex_unlock(&ps_enable_mutex);

//<2012/10/25 ShermanWei, Fix Error :<HWMSEN> hwmsen_enable
    if (ret == 0x2)
	ret = 0;	
//>2012/10/25 ShermanWei

    return ret;
}

static int epl6881_get_addr(struct alsps_hw *hw, struct epl6881_i2c_addr *addr)
{
       printk("[ELAN epl6881] %s\n", __func__);
	if(!hw || !addr)
	{
		return -EFAULT;
	}
	
	addr->epl_addr = (hw->i2c_addr[0]);
       printk("[ELAN  epl6881] i2c addr : 0x%x\n",addr->epl_addr);
	   
	return 0;
}

void on_lsensor_stop(void)
{
// when light sensor stop, switch led on
}

void on_lsensor_start(void)
{
// when light sensor start, switch led off
}

static int elan_epl6881_disable(struct elan_epl_data *epld)
{
   struct i2c_client *client = epld->client;
   int ret;

   EPL_DEBUG_INTERFACE("[ELAN epl6881] %s\n",__func__);

   ret = elan_epl6881_I2C_Write(client,REG_7, W_SINGLE_BYTE, 0x02, EPL_C_P_DOWN);

   if(ret != 0x02)
    {
        epld->enable_lflag = 0;
        EPL_DEBUG_INTERFACE("[Elan epl6881] sensor disable err\n");
    }
//<2012/10/25 ShermanWei, Fix Error :<HWMSEN> hwmsen_enable
    if (ret == 0x2)
	ret = 0;	
//>2012/10/25 ShermanWei
   
   return ret; 
}

static int elan_epl6881_lsensor_enable(struct elan_epl_data *epld)
{
    int ret;
    uint8_t regdata = 0;
    struct i2c_client *client = epld->client;
    struct file *fp_als;
//<2013/03/28-23302-kevincheng , Fix ALSPS APK crash issue
    unsigned char als_val[10];

    mm_segment_t fs_als;
    loff_t posals;

    EPL_DEBUG_INTERFACE("[ELAN]--- ALS sensor Enable --- \n");

    fp_als = filp_open("/data/nrstdata/als_lux_per_count.txt", O_RDONLY, S_IRUSR);
    if (IS_ERR(fp_als))
    {
        EPL_DEBUG_INTERFACE("[epl6881]create lux_per_count file_als error\n");        
    }
    else
    {
        fs_als = get_fs();
        set_fs(KERNEL_DS);
        posals = 0;
        vfs_read(fp_als, als_val, sizeof(als_val), &posals);
	printk("epl als_val : %s\n",als_val);
        lux_per_count = elan_calibration_atoi(als_val);
        filp_close(fp_als, NULL);
        set_fs(fs_als);
    }
    printk("epl lux_per_count : %d\n",lux_per_count);
//>2013/03/28-23302-kevincheng
    mutex_lock(&als_enable_mutex);

    on_lsensor_start();

    elan_epl6881_I2C_Write(client,REG_7, W_SINGLE_BYTE, 0x02, EPL_C_P_UP);
	
    //set register 0
    //average cycle = 32 times, sensing mode = continuous, sensor mode = als,gain = auto
    epl_data->epl_average_item = EPL_SENSING_32_TIME;
    epl_data->epl_sc_mode = EPL_C_SENSING_MODE;
    epl_data->epl_op_mode = EPL_ALS_MODE;
    epl_data->epl_sensor_gain_item = EPL_AUTO_GAIN;
    regdata = epl_data->epl_average_item | epl_data->epl_sc_mode | epl_data->epl_op_mode | epl_data->epl_sensor_gain_item;
    ret = elan_epl6881_I2C_Write(client,REG_0,W_SINGLE_BYTE,0X02,regdata);

    //set register 1
    epl_data->epl_intpersistency = EPL_PST_1_TIME;
    epl_data->epl_adc_item = EPL_10BIT_ADC;
    epld->epl_integrationtime_item = als_intT_selection[epld->epl_integrationtime_index];
    regdata = epld->epl_integrationtime_item << 4 | epl_data->epl_intpersistency | epl_data->epl_adc_item;
    epld->epl_integrationtime = als_intT_table[epld->epl_integrationtime_item];
    ret = elan_epl6881_I2C_Write(client,REG_1,W_SINGLE_BYTE,0X02,regdata);
//<2012/11/12-16731-kevincheng,Fix light sensor issue
    //elan_epl6881_I2C_Write(client,REG_9,W_SINGLE_BYTE,0x02,EPL_INT_FRAME_ENABLE);  //setting interrupt pin = binary
    elan_epl6881_I2C_Write(client,REG_9,W_SINGLE_BYTE,0x02,EPL_INT_DISABLE);  //setting interrupt pin = disable modify by Robert 2012/11/12
    
    //set register 10 go-mid
    elan_epl6881_I2C_Write(client,REG_10,W_SINGLE_BYTE,0x02,0x3e); //add by Robert 2012/11/12

    //set register 11 go-low
    elan_epl6881_I2C_Write(client,REG_11,W_SINGLE_BYTE,0x02,0x3e); //add by Robert 2012/11/12
//>2012/11/12-16731-kevincheng

    /*restart sensor*/
    elan_epl6881_I2C_Write(client,REG_7,W_SINGLE_BYTE,0X02,EPL_C_RESET);
    ret = elan_epl6881_I2C_Write(client,REG_7,W_SINGLE_BYTE,0x02,EPL_C_START_RUN);
    EPL_DEBUG_INTERFACE("[ELAN]--- ALS sensor setting finish --- \n");
    msleep(2);

    if(ret != 0x02)
    {
        epld->enable_lflag = 0;
        EPL_DEBUG_INTERFACE("[EPL6881] ALS-sensor i2c err\n");
    }

    mutex_unlock(&als_enable_mutex);
//<2012/09/13 ShermanWei, Fix Error :<HWMSEN> hwmsen_enable 521 : activate sensor(4)
    if (ret == 0x2)
	ret = 0;	
//>2012/09/13 ShermanWei

    return ret;

}

static int elan_epl6881_psensor_disable(struct elan_epl_data *epld)
{
    EPL_DEBUG_INTERFACE("[ELAN]--- Proximity sensor disable --- \n");
    ////EPL_DEBUG_INTERFACE("[ELAN]--- wake_unlock --- \n");

    epld->enable_pflag=0;
//<2012/10/26 ShermanWei, for power consumption, reduce 20mA in phone call mode
    ///wake_unlock(&g_ps_wlock);
//>2012/10/26 ShermanWei
    return 0;
}

/*
//====================elan_epl_ps_poll_rawdata===============//
//polling method for proximity sensor detect. Report proximity sensor raw data.
//Report "ABS_DISTANCE" event to HAL layer.
//Variable "value" 0 and 1 to represent which distance from psensor to target(
human's face..etc).
//value: 0 represent near.
//value: 1 represent far.
*/

//<2013/05/31-25693-kevincheng , PS Saturate issue
static int elan_epl_ps_poll_rawdata(bool israw)
{
    struct elan_epl_data *epld = epl_data;
    int value = 0;

    EPL_DEBUG_INTERFACE("[ELAN epl6881] %s \n", __func__);

if(CH0_Saturate == 0)
{
    EPL_DEBUG_INTERFACE("[ELAN epl6881] --- Polling ps rawdata --- high : %d , low : %d\n",epld->epl_intthreshold_hvalue,epld->epl_intthreshold_lvalue);

    gRawData.ps_raw = (gRawData.raw_bytes[1]<<8) | gRawData.raw_bytes[0];
    value = gRawData.ps_raw;
    CH1_data = value;

    if(gRawData.ps_raw > epld->epl_intthreshold_hvalue)
        poverflag = true;

    if(gRawData.ps_raw < epld->epl_intthreshold_lvalue && poverflag == true)
        poverflag = false;

    value = (poverflag == true) ? 0 : 1;
    CH1_value = value;

    EPL_DEBUG_INTERFACE("[ELAN epl6881]ps_raw_data (%x) (%d), value(%d , %s)\n\n",gRawData.ps_raw,gRawData.ps_raw, value,value == 0 ? "near":"far");

    input_report_abs(epld->ps_input_dev, ABS_DISTANCE, value);
    input_sync(epld->ps_input_dev);

    if(israw)
		return gRawData.ps_raw;
	else
		return value;
    	}else{
    EPL_DEBUG_INTERFACE("[ELAN epl6881] --- Polling ps rawdata --- [CH0 Saturate] , CH1 Saturate : %d",CH1_data,CH0_Saturate);
	if(israw)
		return CH1_data;
	else
		return CH1_value;
    		}
}
//>2013/05/31-25693-kevincheng

//<2012/09/10-13741-kevincheng , Calibration Improvement
static int elan_ps_get_data(bool israw)
{
    struct elan_epl_data *epld = epl_data;
    struct i2c_client *client = epld->client;
    int value = 0;
    int i = 0 , ch0 = 0 , status = 0 , ADC_err = 0 , ADC_value_total = 0 , ADC_avg = 0;

    EPL_DEBUG_INTERFACE("[ELAN epl6881] %s\n", __func__);

    mutex_lock(&ps_get_sensor_value_mutex);

//<2012/10/31 ShermanWei ,PCBA version without Dynamic Calibration
#if defined ARIMA_PCBA_RELEASE
/// Nothing To Do
#else
    if((!israw)&&(!Dyna_en))
      {
         for(i=0;i<5;i++)
    	  {
    	    //Proximity Sensor (Status) 
	    elan_epl6881_I2C_Write(client,REG_13,R_TWO_BYTE,0x01,0x00); 
	    elan_epl6881_I2C_Read(client); 
	    status = gRawData.raw_bytes[0]; 
                
	    //Proximity Sensor (Channel_0 REG) 
	    elan_epl6881_I2C_Write(client,REG_14,R_TWO_BYTE,0x01,0x00); 
	    elan_epl6881_I2C_Read(client); 
	    ch0 = (gRawData.raw_bytes[1]<<8) | gRawData.raw_bytes[0]; 
           EPL_DEBUG_INTERFACE("[ELAN_EPL6803] channel 0 = : %d , status = 0x%x\n",ch0, status);

	    //Proximity Sensor (Channel_1 REG)
    elan_epl6881_I2C_Write(client,REG_16,R_TWO_BYTE,0x01,0x00);
    elan_epl6881_I2C_Read(client);
	
           gRawData.ps_raw = (gRawData.raw_bytes[1]<<8) | gRawData.raw_bytes[0];
           value = gRawData.ps_raw;
	    EPL_DEBUG_INTERFACE("[Elan_epl6881]dyan[%d] : %d\n",i,value);

//<2013/05/31-25693-kevincheng,PS Saturate issue  
           if((value == 0) || (value > crit_limit))
//>2013/05/31-25693-kevincheng
		{
		   ADC_err = 1;
		}
		ADC_value_total +=value;
    	    }
		ADC_avg = (ADC_value_total /5);

		EPL_DEBUG_INTERFACE("ADC_avg value : %d , base : %d\n", ADC_avg,threshold_base);
		
		if(ADC_err == 1)
		{ 
		       EPL_DEBUG_INTERFACE("[Elan_epl6881]Dynamic fail\n");
                       calibaration_result=elan_calibaration_read();
                       printk("[ELAN]read check : %d , cal_h_val : %d\n", calibaration_result,cal_h_val);
                       if(cal_h_val != 0)
                         {
                          epld->epl_intthreshold_lvalue = cal_l_val;
                          epld->epl_intthreshold_hvalue = cal_h_val;
                         }
                       else
                         {
                          epld->epl_intthreshold_lvalue = P_SENSOR_LTHD;
                          epld->epl_intthreshold_hvalue = P_SENSOR_HTHD;
                         }
		}
		else
		{
			epld->epl_intthreshold_hvalue = ADC_avg + threshold_base;
                     epld->epl_intthreshold_lvalue = (epld->epl_intthreshold_hvalue)*8/10;
		}
		Dyna_en = 1;
		ADC_err = 0;
	}
#endif	
//<2013/05/31-25693-kevincheng,PS Saturate issue    
    elan_epl6881_I2C_Write(client,REG_13,R_SINGLE_BYTE,0x01,0x00); 
    elan_epl6881_I2C_Read(client); 
    CH0_Saturate = (gRawData.raw_bytes[0]>>1) & 0x01;
    EPL_DEBUG_INTERFACE("[Elan_epl6881] Reg 0xd : 0x%x , CH0_Saturate : %d\n",gRawData.raw_bytes[0],CH0_Saturate);
    
    if(!CH0_Saturate){
    elan_epl6881_I2C_Write(client,REG_16,R_TWO_BYTE,0x01,0x00);
    elan_epl6881_I2C_Read(client);}
//>2013/05/31-25693-kevincheng

    value = elan_epl_ps_poll_rawdata(israw);
    mutex_unlock(&ps_get_sensor_value_mutex);

    return value;
}

static void incandescent_lux(void)
{
    struct elan_epl_data *epld = epl_data;
    uint32_t ratio_square = (gRawData.ratio * gRawData.ratio) >> 8;

    uint32_t c = ((epld->als_bx * gRawData.ratio) >> 8)-((epld->als_ax2 * ratio_square) >> 8) - epld->als_c;

          c = (c * ALS_L) >> 8;
    if(epld->epl_integrationtime > 0 )
    {
    gRawData.lux = ((gRawData.als_ch1_raw * c) / epld->epl_integrationtime)>> 8;
    }
}

static void fluorescent_lux(void)
{
    struct elan_epl_data *epld = epl_data;
    uint32_t ratio_square = (gRawData.ratio * gRawData.ratio) >> 8;

    uint32_t c = ((epld->als_bx * gRawData.ratio) >> 8)-((epld->als_ax2 * ratio_square) >> 8) - epld->als_c;

          c = (c * ALS_L) >> 8;
    if(epld->epl_integrationtime > 0 )
    {
    gRawData.lux = ((gRawData.als_ch1_raw * c) / epld->epl_integrationtime)>> 8;
    }
}

static void sample_lux(void)
{
    struct elan_epl_data *epld = epl_data;

    if(gRawData.als_ch1_raw < 15)
       gRawData.lux = 0;
    else
       gRawData.lux = ((gRawData.als_ch1_raw * lux_per_count) / 1000 );

    if(gRawData.lux > 65535)
		gRawData.lux = 65535;
}

////====================lux equation===============//
static void elan_epl_als_equation(void)
{
    struct elan_epl_data *epld = epl_data;
    uint32_t channel1 = gRawData.als_ch1_raw << 8;
    uint16_t channel0 = gRawData.als_ch0_raw;

	sample_lux();
#if 0
	gRawData.ratio = channel1 / channel0;

    if(gRawData.ratio <= epld->als_i_threshold)
    {
        gRawData.ratio = epld->als_i_threshold;
        incandescent_lux();
    }
    else if(gRawData.ratio > epld->als_i_threshold && gRawData.ratio < epld->als_m_threshold)
    {
        incandescent_lux();
    }
    else if(gRawData.ratio >= epld->als_m_threshold && gRawData.ratio < epld->als_f_threshold)
    {
        fluorescent_lux();
    }
    else
    {
        gRawData.ratio = epld->als_f_threshold;
        fluorescent_lux();
    }
    //EPL_DEBUG_INTERFACE("[ELAN]ambient light sensor lux = %d ,ch1_raw=%x, ch0_raw=%x ,Int=%d\n\n",\
		   gRawData.lux,gRawData.als_ch1_raw ,gRawData.als_ch0_raw,epld->epl_integrationtime );
#endif
#if 1
    input_report_abs(epld->als_input_dev, ABS_MISC, gRawData.lux);
    input_sync(epld->als_input_dev);
#endif
}

/*
//====================elan_epl_als_rawdata===============//
//polling method for Light sensor detect. Report Light sensor raw data.
//Report "ABS_MISC" event to HAL layer.
*/
static void elan_epl_als_rawdata(void)
{
    uint16_t value;
    uint8_t regdata;
    uint8_t maxIndex = sizeof(als_intT_selection)-1;
    struct elan_epl_data *epld = epl_data;
    struct i2c_client *client = epld->client;

#if 1
       elan_epl_als_equation();
#else
    value = (gRawData.als_ch0_raw > gRawData.als_ch1_raw) ? gRawData.als_ch0_raw : gRawData.als_ch1_raw;
    if(value <= ALS_MAX_COUNT && value >= ALS_MIN_COUNT)
    {
        EPL_DEBUG_INTERFACE("[ELAN]---- normal value = %d\n", value);
        elan_epl_als_equation();
    }
    else
    {
        if(value > ALS_MAX_COUNT)
        {
            if(epld->epl_integrationtime_index == 0)
            {
                epld->epl_integrationtime_item=als_intT_selection[epld->epl_integrationtime_index];
                epld->epl_integrationtime = als_intT_table[epld->epl_integrationtime_item];
                EPL_DEBUG_INTERFACE("[ELAN]lgiht sensor lux max %d , value : %d\n",ALS_MAX_LUX,value);
				
                input_report_abs(epld->als_input_dev, ABS_MISC, ALS_MAX_LUX);
                input_sync(epld->als_input_dev);
                gRawData.lux = ALS_MAX_COUNT;

            }
            else
            {
                epld->epl_integrationtime_index --;
                epld->epl_integrationtime_item=als_intT_selection[epld->epl_integrationtime_index];
                epld->epl_integrationtime = als_intT_table[epld->epl_integrationtime_item];
                EPL_DEBUG_INTERFACE("---- [ELAN]light sensor integration time down (%d us)\n",epld->epl_integrationtime);
                regdata = epld->epl_integrationtime_item << 4 | epld->epl_intpersistency | epld->epl_adc_item;
                elan_epl6881_I2C_Write(client,REG_1,W_SINGLE_BYTE,0X02,regdata);
                //restart sensor
                elan_epl6881_I2C_Write(client,REG_7,W_SINGLE_BYTE,0X02,EPL_C_RESET);
                elan_epl6881_I2C_Write(client,REG_7,W_SINGLE_BYTE,0x02,EPL_C_START_RUN);
            }
        }

        if(value < ALS_MIN_COUNT)
        {
            EPL_DEBUG_INTERFACE("---- [ELAN]ADJ_Min value = %d,  intT_index = %d, intT = %d\n",value, epld->epl_integrationtime_index, epld->epl_integrationtime);
            if(epld->epl_integrationtime_index == maxIndex)
            {
                epld->epl_integrationtime_item=als_intT_selection[epld->epl_integrationtime_index];
                epld->epl_integrationtime = als_intT_table[epld->epl_integrationtime_item];
                EPL_DEBUG_INTERFACE("[ELAN]ambient light sensor min lux ,ch1_raw=%x, ch0_raw=%x ,IntTime=%d\n",\
					gRawData.als_ch1_raw ,gRawData.als_ch0_raw,epld->epl_integrationtime );
                EPL_DEBUG_INTERFACE("[ELAN]lgith sensor lux min = %d\n",ALS_MIN_LUX);

                input_report_abs(epld->als_input_dev, ABS_MISC, ALS_MIN_LUX);
                input_sync(epld->als_input_dev);
                gRawData.lux = ALS_MIN_LUX;
				
            }
            else
            {
                epld->epl_integrationtime_index ++ ;
                epld->epl_integrationtime_item=als_intT_selection[epld->epl_integrationtime_index];
                epld->epl_integrationtime = als_intT_table[epld->epl_integrationtime_item];
                EPL_DEBUG_INTERFACE("---- [ELAN]light sensor integration time up (%d us)\n", epld->epl_integrationtime);
                regdata = epld->epl_integrationtime_item << 4 | epld->epl_intpersistency | epld->epl_adc_item;
                elan_epl6881_I2C_Write(client,REG_1,W_SINGLE_BYTE,0X02,regdata);
                //restart sensor
                elan_epl6881_I2C_Write(client,REG_7,W_SINGLE_BYTE,0X02,EPL_C_RESET);
                elan_epl6881_I2C_Write(client,REG_7,W_SINGLE_BYTE,0x02,EPL_C_START_RUN);
            }
        }
    }
	#endif
}

static int elan_als_get_data(void)
{
    struct elan_epl_data *epld = epl_data;
    struct i2c_client *client = epld->client;

    mutex_lock(&als_get_sensor_value_mutex);
	
    elan_epl6881_I2C_Write(client,REG_14,R_TWO_BYTE,0x01,0x00);
    elan_epl6881_I2C_Read(client);
    gRawData.als_ch0_raw = (gRawData.raw_bytes[1]<<8) | gRawData.raw_bytes[0];
    elan_epl6881_I2C_Write(client,REG_16,R_TWO_BYTE,0x01,0x00);
    elan_epl6881_I2C_Read(client);
    gRawData.als_ch1_raw = (gRawData.raw_bytes[1]<<8) | gRawData.raw_bytes[0];
    //printk("epl get data : %d\n",gRawData.als_ch1_raw);
    elan_epl_als_rawdata();
	
    mutex_unlock(&als_get_sensor_value_mutex);
    EPL_DEBUG_INTERFACE("[ELAN epl6881] %s done , return gRawData.lux : %d\n", __func__, gRawData.lux);
	
    return gRawData.lux;
}

/*
//====================set_psensor_intr_threshold===============//
//low_thd: The value is psensor interrupt low threshold.
//high_thd:	The value is psensor interrupt hihg threshold.
//When psensor rawdata > hihg_threshold, interrupt pin will be pulled low.
//After interrupt occur, psensor rawdata < low_threshold, interrupt pin will 
be pulled high.
*/
static int set_psensor_intr_threshold(uint16_t low_thd, uint16_t high_thd)
{
    int ret = 0;
    struct elan_epl_data *epld = epl_data;
    struct i2c_client *client = epld->client;

    uint8_t high_msb ,high_lsb, low_msb, low_lsb;

    EPL_DEBUG_INTERFACE("[ELAN epl6881] %s , low : %d , high : %d\n", __func__,low_thd,high_thd);
	
    high_msb = (uint8_t) (high_thd >> 8);
    high_lsb = (uint8_t) (high_thd & 0x00ff);
    low_msb  = (uint8_t) (low_thd >> 8);
    low_lsb  = (uint8_t) (low_thd & 0x00ff);

    //EPL_DEBUG_INTERFACE("[ELAN epl6801] %s: low_thd = 0x%X, high_thd = 0x%x \n",__func__, low_thd, high_thd);
    mutex_lock(&ps_get_sensor_value_mutex);
    elan_epl6881_I2C_Write(client,REG_2,W_SINGLE_BYTE,0x02,high_lsb);
    elan_epl6881_I2C_Write(client,REG_3,W_SINGLE_BYTE,0x02,high_msb);
    elan_epl6881_I2C_Write(client,REG_4,W_SINGLE_BYTE,0x02,low_lsb);
    elan_epl6881_I2C_Write(client,REG_5,W_SINGLE_BYTE,0x02,low_msb);
    mutex_unlock(&ps_get_sensor_value_mutex);

    return ret;
}

static void epl_sensor_irq_do_work(struct work_struct *work)
{
    struct elan_epl_data *epld = epl_data;
    struct i2c_client *client = epld->client;
    int mode = 0;

    EPL_DEBUG_INTERFACE("[ELAN epl6881] %s\n", __func__);
	
    mutex_lock(&ps_get_sensor_value_mutex);
    mutex_lock(&als_get_sensor_value_mutex);

    elan_epl6881_I2C_Write(client,REG_13,R_SINGLE_BYTE,0x01,0);
    elan_epl6881_I2C_Read(client);
    epld->epl_return_status = gRawData.raw_bytes[0];
    mode = gRawData.raw_bytes[0]&(3<<4);

    // 0x10 is ps mode
    if(mode==0x10)
    {
        if(epld->enable_lflag && epld->enable_lflag && !epld->ps_irq_enabled)
        {
            elan_epl6881_I2C_Write(client,REG_16,R_TWO_BYTE,0x01,0x00);
            elan_epl6881_I2C_Read(client);
            elan_epl_ps_poll_rawdata(false);
        }

        // change to binary mode in first frame
        else  if(epld->ps_irq_enabled &&  epl_data->ps_is_first_frame)
        {
            // disable frame enable
//kevin compile error            set_irq_type(IRQ_EINT1,IRQ_TYPE_EDGE_FALLING |IRQ_TYPE_EDGE_RISING);
            elan_epl6881_I2C_Write(client,REG_9,W_SINGLE_BYTE,0x02,EPL_INT_BINARY);
            elan_epl6881_I2C_Write(client,REG_7,W_SINGLE_BYTE,0x02,EPL_DATA_UNLOCK);
        }

        // report value in other frame or irq trigger
        else
        {
            if(epld->ps_irq_enabled)
            {
                elan_epl6881_I2C_Write(client,REG_7,W_SINGLE_BYTE,0x02,0x01); 
//lock_only
            }

            elan_epl6881_I2C_Write(client,REG_16,R_TWO_BYTE,0x01,0x00);
            elan_epl6881_I2C_Read(client);
            elan_epl_ps_poll_rawdata(false);

            if(epld->ps_irq_enabled)
            {
                elan_epl6881_I2C_Write(client,REG_7,W_SINGLE_BYTE,0x02,EPL_DATA_UNLOCK);
            }
        }
        epl_data->ps_is_first_frame = 0;
    }

    // 0x00 is als mode
    else if(mode==0x00)
    {
        elan_epl6881_I2C_Write(client,REG_14,R_TWO_BYTE,0x01,0x00);
        elan_epl6881_I2C_Read(client);
        gRawData.als_ch0_raw = (gRawData.raw_bytes[1]<<8) | gRawData.raw_bytes[0];
        elan_epl6881_I2C_Write(client,REG_16,R_TWO_BYTE,0x01,0x00);
        elan_epl6881_I2C_Read(client);
        gRawData.als_ch1_raw = (gRawData.raw_bytes[1]<<8) | gRawData.raw_bytes[0];
        elan_epl_als_rawdata();
        epld->read_flag=1;

        if(epld->enable_pflag==1)
        {
            if(epld->ps_irq_enabled)
            {
                elan_epl6881_psensor_enable(epld);
            }
            else
            {
                queue_delayed_work(epld->epl_wq, &ps_polling_work,msecs_to_jiffies(0));
            }
        }
        on_lsensor_stop();
    }
    enable_irq(epld->irq);
    mutex_unlock(&ps_get_sensor_value_mutex);
    mutex_unlock(&als_get_sensor_value_mutex);
}

static irqreturn_t elan_epl6881_irq_handler(int irqNo, void *handle)
{
    struct elan_epl_data *epld = (struct elan_epl_data*)handle;

    EPL_DEBUG_INTERFACE("[ELAN epl6881] %s , irq_no : %d\n", __func__,irqNo);
    disable_irq_nosync(epld->irq);
    queue_work(epld->epl_wq, &epl_sensor_irq_work);

    return IRQ_HANDLED;
}

static void polling_do_work(struct work_struct *work)
{
    struct elan_epl_data *epld = epl_data;
    struct i2c_client *client = epld->client;

    disable_irq(epld->irq);
    EPL_DEBUG_INTERFACE("[ELAN]---polling do work---\n");
    //set_irq_type(IRQ_EINT1,IRQ_TYPE_EDGE_FALLING);
    cancel_delayed_work(&polling_work);
    cancel_delayed_work(&ps_polling_work);

    if(epld->enable_pflag==0 && epld->enable_lflag==0)
    {
        elan_epl6881_I2C_Write(client,REG_9,W_SINGLE_BYTE,0x02,EPL_INT_DISABLE);
        elan_epl6881_I2C_Write(client,REG_7,W_SINGLE_BYTE,0X02,EPL_C_RESET);
    }
    else if(epld->enable_pflag==1 && epld->enable_lflag==0)
    {
        mutex_lock(&ps_get_sensor_value_mutex);
        if(!epld->ps_irq_enabled)
        {
            queue_delayed_work(epld->epl_wq, &polling_work,msecs_to_jiffies(PS_POLLING_RATE));
        }
        elan_epl6881_psensor_enable(epld);
        mutex_unlock(&ps_get_sensor_value_mutex);
    }
    else if(epld->enable_pflag==0 && epld->enable_lflag==1)
    {
        mutex_lock(&als_get_sensor_value_mutex);
        queue_delayed_work(epld->epl_wq, &polling_work,msecs_to_jiffies(ALS_POLLING_RATE));
        elan_epl6881_lsensor_enable(epld);
        mutex_unlock(&als_get_sensor_value_mutex);
    }
    else if(epld->enable_pflag==1 && epld->enable_lflag==1)
    {
        mutex_lock(&als_get_sensor_value_mutex);
        queue_delayed_work(epld->epl_wq, &polling_work,msecs_to_jiffies(DUAL_POLLING_RATE ));
        elan_epl6881_lsensor_enable(epld);
        mutex_unlock(&als_get_sensor_value_mutex);
    }
    enable_irq(epld->irq);
}

static void ps_polling_do_work(struct work_struct *work)
{
    struct elan_epl_data *epld = epl_data;

    EPL_DEBUG_INTERFACE("[ELAN epl6881] %s\n", __func__);
	
//<2012/09/10-13741-kevincheng , Calibration Improvement
#if 0
    queue_delayed_work(epld->epl_wq, &ps_polling_work,msecs_to_jiffies(PS_POLLING_RATE));
#endif 
//>2012/09/10-13741-kevincheng
    elan_epl6881_psensor_enable(epld);
}

static ssize_t elan_ls_intHthreshold_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    uint16_t hthd;
    struct elan_epl_data *epld = epl_data;

    sscanf(buf,"%hu",&hthd);
    epld->epl_intthreshold_hvalue = hthd;
    EPL_DEBUG_INTERFACE("==>[ELAN][high threshold]=%d\n", epld->epl_intthreshold_hvalue);
    set_psensor_intr_threshold(epld->epl_intthreshold_lvalue,epld->epl_intthreshold_hvalue);
    return count;
}

static ssize_t elan_ls_intHthreshold_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    long *tmp = (long*)buf;
    struct elan_epl_data *epld = epl_data;
    tmp[0] = epld->epl_intthreshold_hvalue;
    EPL_DEBUG_INTERFACE("[ELAN epl6801] %s\n", __func__);
    return 4;
}

static ssize_t elan_ls_intLthreshold_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    uint16_t lthd;
    struct elan_epl_data *epld = epl_data;

    sscanf(buf,"%hu",&lthd);
    epld->epl_intthreshold_lvalue = lthd;
    EPL_DEBUG_INTERFACE("==>[ELAN][low threshold]=%d\n", epld->epl_intthreshold_lvalue);
    set_psensor_intr_threshold(epld->epl_intthreshold_lvalue,epld->epl_intthreshold_hvalue);
    return count;
}

static ssize_t elan_ls_intLthreshold_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    long *tmp = (long*)buf;
    struct elan_epl_data *epld = epl_data;
	
    EPL_DEBUG_INTERFACE("[ELAN epl6881] %s\n", __func__);
	
    tmp[0] = epld->epl_intthreshold_lvalue;
    return 4;
}

//<2013/04/19-24085-kevincheng,Cover Light Sensor can't turn off issue
#if 0
static ssize_t elan_ls_operationmode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    uint16_t mode=0;
    struct elan_epl_data *epld = epl_data;
    EPL_DEBUG_INTERFACE("[ELAN epl6881] %s\n", __func__);
    sscanf(buf, "%hu",&mode);
    EPL_DEBUG_INTERFACE("==>[ELAN][operation mode]=%d\n", mode);

    mutex_lock(&als_get_sensor_value_mutex);
    mutex_lock(&ps_get_sensor_value_mutex);

    if(mode == 0)
    {
        epld->enable_lflag = 0;
        epld->enable_pflag = 0;
//<2012/10/26 ShermanWei, for power consumption, reduce 20mA in phone call mode
        ///wake_unlock(&g_ps_wlock);
//>2012/10/26 ShermanWei
    }
    else if(mode == 1)
    {
        epld->enable_lflag = 1;
        epld->enable_pflag = 0;
        elan_epl6881_lsensor_enable(epld);
//<2012/10/26 ShermanWei, for power consumption, reduce 20mA in phone call mode
        ///wake_unlock(&g_ps_wlock);
//>2012/10/26 ShermanWei
    }
    else if(mode == 2)
    {
        epld->enable_lflag = 0;
        epld->enable_pflag = 1;
    }
    else if(mode == 3)
    {
        epld->enable_lflag = 1;
        epld->enable_pflag = 1;
    }
    else
    {
        EPL_DEBUG_INTERFACE("0: none\n1: als only\n2: ps only\n3: interleaving\n");
    }

    cancel_delayed_work(&polling_work);
//<2012/09/10-13741-kevnicheng , Calibration Improvement
#if 0
    queue_delayed_work(epld->epl_wq, &polling_work,msecs_to_jiffies(0));
#endif 
//>2012/09/10-13741-kevincheng

    mutex_unlock(&als_get_sensor_value_mutex);
    mutex_unlock(&ps_get_sensor_value_mutex);
    return count;
}
#endif

static ssize_t elan_ls_operationmode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct elan_epl_data *epld = epl_data;
    long *tmp = (long*)buf;
    uint16_t mode =0;
    int ret = 0;

    if(  epld->enable_pflag==0 &&  epld->enable_lflag==0)
    {
        mode = 0;
    }
    else if(  epld->enable_pflag==0 &&  epld->enable_lflag==1)
    {
        mode = 1;
    }
    else if(  epld->enable_pflag==1 && epld->enable_lflag==0)
    {
        mode = 2;
    }
    else if(  epld->enable_pflag==1 && epld->enable_lflag==1)
    {
        mode = 3;
    }
    tmp[0] = mode;
    EPL_DEBUG_INTERFACE("[ELAN epl6881] %s : %d\n", __func__,mode);

    ret = sprintf(buf, "%d\n", tmp[0]);
    mutex_lock(&als_get_sensor_value_mutex);
    mutex_lock(&ps_get_sensor_value_mutex);

    epld->enable_lflag = 1;
    epld->enable_pflag = 0;
    elan_epl6881_lsensor_enable(epld);
	
    mutex_unlock(&als_get_sensor_value_mutex);
    mutex_unlock(&ps_get_sensor_value_mutex);
    return ret;
}
//>2013/04/19-24085-kevincheng

static ssize_t elan_ls_ps_irq_enabled_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    uint16_t enabled;
    struct elan_epl_data *epld = epl_data;

    sscanf(buf,"%hu",&enabled);
    EPL_DEBUG_INTERFACE("==>[ELAN][ps irq enabled]=%d\n",enabled);

    mutex_lock(&ps_get_sensor_value_mutex);

    epld->ps_irq_enabled = enabled;
    cancel_delayed_work(&polling_work);
//<2012/09/10-13741-kevincheng , Calibration Improvement
#if 0
    queue_delayed_work(epld->epl_wq, &polling_work,msecs_to_jiffies(0));
#endif
//>2012/09/10-13741-kevincheng

    mutex_unlock(&ps_get_sensor_value_mutex);

    return count;
}

static ssize_t elan_ls_ps_irq_enabled_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct elan_epl_data *epld = epl_data;
    long *tmp = (long*)buf;

    EPL_DEBUG_INTERFACE("[ELAN epl6881] %s\n", __func__);
	
    tmp[0] = epld->ps_irq_enabled;
    return 2;
}

static ssize_t elan_ls_sensor_als_ch0_rawdata_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    long *tmp = (long*)buf;

    EPL_DEBUG_INTERFACE("[ELAN epl6881] %s\n", __func__);
	
    tmp[0] = gRawData.als_ch0_raw;
    return 2;
}

static ssize_t elan_ls_sensor_als_ch1_rawdata_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    long *tmp = (long*)buf;

    EPL_DEBUG_INTERFACE("[ELAN epl6881] %s\n", __func__);
	
    tmp[0] = gRawData.als_ch1_raw;
    return 2;
}

static ssize_t elan_ls_incandescent_threshold_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    uint32_t value;
    struct elan_epl_data *epld = epl_data;

    EPL_DEBUG_INTERFACE("[ELAN epl6801] %s\n", __func__);

    value = buf[3]<<24 | buf[2] <<16|buf[1]<<8 | buf[0];

    EPL_DEBUG_INTERFACE("==>[ELAN][incandescent]=%d\n", value);
    mutex_lock(&als_get_sensor_value_mutex);
    epld->als_i_threshold = value;
    mutex_unlock(&als_get_sensor_value_mutex);
    return 1;
}

static ssize_t elan_ls_incandescent_threshold_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    long *tmp = (long*)buf;
    struct elan_epl_data *epld = epl_data;

    EPL_DEBUG_INTERFACE("[ELAN epl6881] %s\n", __func__);

    tmp[0] = epld->als_i_threshold;
    return 4;
}

static ssize_t elan_ls_fluorescent_threshold_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int value;
    struct elan_epl_data *epld = epl_data;

    EPL_DEBUG_INTERFACE("[ELAN epl6881] %s\n", __func__);

    value = buf[3]<<24 | buf[2] <<16|buf[1]<<8 | buf[0];
    EPL_DEBUG_INTERFACE("==>[ELAN][fluorescent]=%d\n", value);
    mutex_lock(&als_get_sensor_value_mutex);
    epld->als_f_threshold = value;
    mutex_unlock(&als_get_sensor_value_mutex);
    return 1;
}

static ssize_t elan_ls_fluorescent_threshold_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    long *tmp = (long*)buf;
    struct elan_epl_data *epld = epl_data;

    EPL_DEBUG_INTERFACE("[ELAN epl6881] %s\n", __func__);

    tmp[0] = epld->als_f_threshold;

    return 4;
}

static ssize_t elan_ls_mix_threshold_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int value;
    struct elan_epl_data *epld = epl_data;

    EPL_DEBUG_INTERFACE("[ELAN epl6881] %s\n", __func__);

    value = buf[3]<<24 | buf[2] <<16|buf[1]<<8 | buf[0];
    EPL_DEBUG_INTERFACE("==>[mix]=%d\n", value);
    mutex_lock(&als_get_sensor_value_mutex);
    epld->als_m_threshold = value;
    mutex_unlock(&als_get_sensor_value_mutex);
    return 1;
}

static ssize_t elan_ls_mix_threshold_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    long *tmp = (long*)buf;
    struct elan_epl_data *epld = epl_data;

    EPL_DEBUG_INTERFACE("[ELAN epl6881] %s\n", __func__);

    tmp[0] = epld->als_m_threshold;

    return 4;
}

static ssize_t elan_ls_als_ax2_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int value;
    struct elan_epl_data *epld = epl_data;

    EPL_DEBUG_INTERFACE("[ELAN epl6881] %s\n", __func__);

    value = buf[3]<<24 | buf[2] <<16|buf[1]<<8 | buf[0];

    mutex_lock(&als_get_sensor_value_mutex);
    epld->als_ax2= value;
    mutex_unlock(&als_get_sensor_value_mutex);
    return 1;
}

static ssize_t elan_ls_als_ax2_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct elan_epl_data *epld = epl_data;
    long *tmp = (long*)buf;

    EPL_DEBUG_INTERFACE("[ELAN epl6881] %s\n", __func__);
	
    tmp[0] = epld->als_ax2;
    return 4;
}


static ssize_t elan_ls_als_bx_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int value;
    struct elan_epl_data *epld = epl_data;

    EPL_DEBUG_INTERFACE("[ELAN epl6881] %s\n", __func__);

    value = buf[3]<<24 | buf[2] <<16|buf[1]<<8 | buf[0];

    mutex_lock(&als_get_sensor_value_mutex);
    epld->als_bx = value;
    mutex_unlock(&als_get_sensor_value_mutex);
    return 1;
}

static ssize_t elan_ls_als_bx_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct elan_epl_data *epld = epl_data;
    long *tmp = (long*)buf;

    EPL_DEBUG_INTERFACE("[ELAN epl6881] %s\n", __func__);
	
    tmp[0] = epld->als_bx;
    return 4;
}

static ssize_t elan_ls_als_c_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int value;
    struct elan_epl_data *epld = epl_data;

    EPL_DEBUG_INTERFACE("[ELAN epl6881] %s\n", __func__);

    value = buf[3]<<24 | buf[2] <<16|buf[1]<<8 | buf[0];

    mutex_lock(&als_get_sensor_value_mutex);
    epld->als_c = value;
    mutex_unlock(&als_get_sensor_value_mutex);
    return 1;
}

static ssize_t elan_ls_als_c_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct elan_epl_data *epld = epl_data;
    long *tmp = (long*)buf;

    EPL_DEBUG_INTERFACE("[ELAN]%s\n",__func__);
	
    tmp[0] = epld->als_c;
    return 4;
}

static ssize_t elan_ls_als_ax_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int value;
    struct elan_epl_data *epld = epl_data;

    EPL_DEBUG_INTERFACE("[ELAN epl6881] %s\n", __func__);

    value = buf[3]<<24 | buf[2] <<16|buf[1]<<8 | buf[0];

    mutex_lock(&als_get_sensor_value_mutex);
    epld->als_ax = value;
    mutex_unlock(&als_get_sensor_value_mutex);
    return 1;
}

static ssize_t elan_ls_als_ax_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct elan_epl_data *epld = epl_data;
    long *tmp = (long*)buf;

    EPL_DEBUG_INTERFACE("[ELAN]%s\n",__func__);
	
    tmp[0] = epld->als_ax;
    return 4;
}

static ssize_t elan_ls_als_b_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int value;
    struct elan_epl_data *epld = epl_data;

    EPL_DEBUG_INTERFACE("[ELAN epl6881] %s\n", __func__);

    value = buf[3]<<24 | buf[2] <<16|buf[1]<<8 | buf[0];

    mutex_lock(&als_get_sensor_value_mutex);
    epld->als_b = value;
    mutex_unlock(&als_get_sensor_value_mutex);
    return 1;
}

static ssize_t elan_ls_als_b_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct elan_epl_data *epld = epl_data;
    long *tmp = (long*)buf;

    EPL_DEBUG_INTERFACE("[ELAN]%s\n",__func__);
	
    tmp[0] = epld->als_b;
    return 4;
}

static ssize_t elan_ls_als_lux_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    long *tmp = (long*)buf;

    EPL_DEBUG_INTERFACE("[ELAN]%s\n",__func__);
	
    tmp[0] = gRawData.lux;
    return 4;
}

static ssize_t elan_ls_ps_ch1_rawdata_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    long *tmp = (long*)buf;

    EPL_DEBUG_INTERFACE("[ELAN]%s\n",__func__);
    epl6881_calibration_polling();
    tmp[0] = gRawData.ps_raw;
    return 2;
}

static ssize_t elan_regdata_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    long *tmp = (long*)buf;
struct elan_epl_data *epld = epl_data;
struct i2c_client *client = epld->client;

EPL_DEBUG_INTERFACE("[ELAN epl6881] %s\n", __func__);
elan_epl6881_I2C_Write(client,REG_0,R_TWO_BYTE,0x01,0x0);
elan_epl6881_I2C_Read(client);

elan_epl6881_I2C_Write(client,REG_7,R_TWO_BYTE,0x01,0x0);
elan_epl6881_I2C_Read(client);

elan_epl6881_I2C_Write(client,REG_13,R_SINGLE_BYTE,0x01,0x0);
elan_epl6881_I2C_Read(client);

	//Proximity Sensor (Channel_0 REG) 
elan_epl6881_I2C_Write(client,REG_14,R_TWO_BYTE,0x01,0x00); 
EPL_DEBUG_INTERFACE("[ELAN epl6881] REG_14\n");
elan_epl6881_I2C_Read(client); 

	//Proximity Sensor (Channel_1 REG)
elan_epl6881_I2C_Write(client,REG_16,R_TWO_BYTE,0x01,0x00);
EPL_DEBUG_INTERFACE("[ELAN epl6881] REG_16\n");
elan_epl6881_I2C_Read(client);
	
    
    return 2;
}

static DEVICE_ATTR(elan_ls_intHthreshold, S_IWUSR|S_IRUGO,elan_ls_intHthreshold_show, elan_ls_intHthreshold_store);
static DEVICE_ATTR(elan_ls_intLthreshold, S_IWUSR|S_IRUGO,elan_ls_intLthreshold_show, elan_ls_intLthreshold_store);
//<2013/04/19-24085-kevincheng,Cover Light Sensor can't turn off issue
static DEVICE_ATTR(elan_ls_operationmode, S_IWUSR|S_IRUGO,elan_ls_operationmode_show,NULL/*elan_ls_operationmode_store*/);
//>2013/04/19-24085-kevincheng
static DEVICE_ATTR(elan_ls_ps_irq_enabled, S_IWUSR|S_IRUGO,elan_ls_ps_irq_enabled_show,elan_ls_ps_irq_enabled_store);

static DEVICE_ATTR(elan_ls_sensor_als_ch0_rawdata, S_IWUSR|S_IRUGO,elan_ls_sensor_als_ch0_rawdata_show,NULL);
static DEVICE_ATTR(elan_ls_sensor_als_ch1_rawdata, S_IWUSR|S_IRUGO,elan_ls_sensor_als_ch1_rawdata_show,NULL);
static DEVICE_ATTR(elan_ls_als_lux, S_IWUSR|S_IRUGO, elan_ls_als_lux_show,NULL);
static DEVICE_ATTR(elan_ls_ps_ch1_rawdata, S_IWUSR|S_IRUGO, elan_ls_ps_ch1_rawdata_show, NULL);

static DEVICE_ATTR(elan_ls_incandescent_threshold, S_IWUSR|S_IRUGO,elan_ls_incandescent_threshold_show, elan_ls_incandescent_threshold_store);
static DEVICE_ATTR(elan_ls_fluorescent_threshold, S_IWUSR|S_IRUGO,elan_ls_fluorescent_threshold_show, elan_ls_fluorescent_threshold_store);
static DEVICE_ATTR(elan_ls_mix_threshold, S_IWUSR|S_IRUGO, elan_ls_mix_threshold_show,elan_ls_mix_threshold_store);

static DEVICE_ATTR(elan_ls_als_ax2, S_IWUSR|S_IRUGO, elan_ls_als_ax2_show,elan_ls_als_ax2_store);
static DEVICE_ATTR(elan_ls_als_bx, S_IWUSR|S_IRUGO, elan_ls_als_bx_show,elan_ls_als_bx_store);
static DEVICE_ATTR(elan_ls_als_c, S_IWUSR|S_IRUGO, elan_ls_als_c_show, elan_ls_als_c_store);
static DEVICE_ATTR(elan_ls_als_ax, S_IWUSR|S_IRUGO, elan_ls_als_ax_show,elan_ls_als_ax_store);
static DEVICE_ATTR(elan_ls_als_b, S_IWUSR|S_IRUGO, elan_ls_als_b_show, elan_ls_als_b_store);

static DEVICE_ATTR(elan_regdata, S_IWUSR|S_IRUGO,elan_regdata_show,NULL);

static struct attribute *ets_attributes[] =
{
    &dev_attr_elan_ls_intHthreshold.attr,
    &dev_attr_elan_ls_intLthreshold.attr,

    &dev_attr_elan_ls_operationmode.attr,
    &dev_attr_elan_ls_ps_irq_enabled.attr,

    &dev_attr_elan_ls_sensor_als_ch0_rawdata.attr,
    &dev_attr_elan_ls_sensor_als_ch1_rawdata.attr,
    &dev_attr_elan_ls_als_lux.attr,
    &dev_attr_elan_ls_ps_ch1_rawdata.attr,

    &dev_attr_elan_ls_incandescent_threshold.attr,
    &dev_attr_elan_ls_fluorescent_threshold.attr,
    &dev_attr_elan_ls_mix_threshold.attr,

    &dev_attr_elan_ls_als_ax2.attr,
    &dev_attr_elan_ls_als_bx.attr,
    &dev_attr_elan_ls_als_c.attr,
    &dev_attr_elan_ls_als_ax.attr,
    &dev_attr_elan_ls_als_b.attr,
    
    &dev_attr_elan_regdata.attr,
    NULL,
};

static struct attribute_group ets_attr_group =
{
    .attrs = ets_attributes,
};

static int elan_als_open(struct inode *inode, struct file *file)
{
    struct elan_epl_data *epld = epl_data;

    EPL_DEBUG_INTERFACE("[ELAN]%s\n",__func__);

    if (epld->als_opened)
    {
        return -EBUSY;
    }
    epld->als_opened = 1;

    return 0;
}

static int elan_als_read(struct file *file, char __user *buffer, size_t count, loff_t *ppos)
{
    struct elan_epl_data *epld = epl_data;
    int buf[3];

    EPL_DEBUG_INTERFACE("[ELAN]%s\n",__func__);	
    if(epld->read_flag ==1)
    {
        buf[0] = gRawData.als_ch0_raw;
        buf[1] = gRawData.als_ch1_raw;
        buf[2] = epl_data->epl_integrationtime;
        if(copy_to_user(buffer, &buf , sizeof(buf)))
            return 0;
        epld->read_flag = 0;
        return 12;
    }
    else
    {
        return 0;
    }
}

static int elan_als_release(struct inode *inode, struct file *file)
{
    struct elan_epl_data *epld = epl_data;

    EPL_DEBUG_INTERFACE("[ELAN] Light Sensor release\n");

    epld->als_opened = 0;

    return 0;
}


static long elan_als_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int flag;
    unsigned long buf[3];
    struct elan_epl_data *epld = epl_data;

    void __user *argp = (void __user *)arg;

    EPL_DEBUG_INTERFACE("[EPL6881] %s cmd %d\n", __func__, _IOC_NR(cmd));

    switch(cmd)
    {
        case ELAN_EPL6881_IOCTL_GET_LFLAG:

            EPL_DEBUG_INTERFACE("[ELAN] ambient-light IOCTL Sensor get lflag \n");
            flag = epld->enable_lflag;
            if (copy_to_user(argp, &flag, sizeof(flag)))
                return -EFAULT;

            EPL_DEBUG_INTERFACE("elan ambient-light Sensor get lflag %d\n",flag);

            break;

        case ELAN_EPL6881_IOCTL_ENABLE_LFLAG:

            EPL_DEBUG_INTERFACE("[ELAN] ambient-light IOCTL Sensor set lflag \n");
            if (copy_from_user(&flag, argp, sizeof(flag)))
                return -EFAULT;
            if (flag < 0 || flag > 1)
                return -EINVAL;

            mutex_lock(&als_get_sensor_value_mutex);
            mutex_lock(&ps_get_sensor_value_mutex);

            epld->enable_lflag = flag;
            cancel_delayed_work(&polling_work);
//<2012/09/10-13741-kevincheng , Calibration Improvement			
#if 0
            queue_delayed_work(epld->epl_wq, &polling_work,msecs_to_jiffies(0));
#endif 
//>2012/09/10-13741-keivncheng

            mutex_unlock(&als_get_sensor_value_mutex);
            mutex_unlock(&ps_get_sensor_value_mutex);

            EPL_DEBUG_INTERFACE("[ELAN] ambient-light Sensor IOCTL set lflag %d\n",flag);

            break;

        case ELAN_EPL6881_IOCTL_GETDATA:
            buf[0] = (unsigned long)gRawData.als_ch0_raw;
            buf[1] = (unsigned long)gRawData.als_ch1_raw;
            buf[2] = (unsigned long)epl_data->epl_integrationtime;
            if(copy_to_user(argp, &buf , sizeof(buf)))
                return -EFAULT;

//kevin compile error            printk("[ELAN] ambient-light Sensor IOCTL get data (%d) \n",value);

            break;

        default:
            pr_err("[ELAN epl6881 error]%s: invalid cmd %d\n",__func__, _IOC_NR(cmd));
            return -EINVAL;
    }

    return 0;


}


static struct file_operations elan_als_fops =
{
    .owner = THIS_MODULE,
    .open = elan_als_open,
    .read = elan_als_read,
    .release = elan_als_release,
    .unlocked_ioctl = elan_als_ioctl
};

static struct miscdevice elan_als_device =
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = "elan_als",
    .fops = &elan_als_fops
};

static int elan_ps_open(struct inode *inode, struct file *file)
{
    struct elan_epl_data *epld = epl_data;

    printk("[ELAN] Proximity Sensor open\n");

    if (epld->ps_opened)
        return -EBUSY;

    epld->ps_opened = 1;

    return 0;
}

static int elan_ps_release(struct inode *inode, struct file *file)
{
    struct elan_epl_data *epld = epl_data;

    printk("[ELAN epl6881] Proximity Sensor release\n");

    epld->ps_opened = 0;

    psensor_mode_suspend = 0;

    return 0;
}


static long elan_ps_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int value;
    int flag;
    struct elan_epl_data *epld = epl_data;

    EPL_DEBUG_INTERFACE("[ELAN epl6881]%s , cmd : %d\n",__func__,cmd);
    void __user *argp = (void __user *)arg;

    //ioctl message handle must define by android sensor library (case by case)
    switch(cmd)
    {

        case ELAN_EPL6881_IOCTL_GET_PFLAG:

            EPL_DEBUG_INTERFACE("[ELAN epl6881]Proximity Sensor IOCTL get pflag \n");
            flag = epld->enable_pflag;
            if (copy_to_user(argp, &flag, sizeof(flag)))
                return -EFAULT;

            EPL_DEBUG_INTERFACE("[ELAN epl6881] Proximity Sensor get pflag %d\n",flag);

            break;

        case ELAN_EPL6881_IOCTL_ENABLE_PFLAG:
            EPL_DEBUG_INTERFACE("[ELAN epl6881] Proximity IOCTL Sensor set pflag \n");
            if (copy_from_user(&flag, argp, sizeof(flag)))
                return -EFAULT;
            if (flag < 0 || flag > 1)
                return -EINVAL;

            mutex_lock(&ps_get_sensor_value_mutex);

            if(flag == 0)
            {
                elan_epl6881_psensor_disable(epld);
            }
            else
            {
                epld->enable_pflag = 1;
            }

            cancel_delayed_work(&polling_work);
//<2012/09/10-13741-kevincheng , Calibration improvement
#if 0			
            queue_delayed_work(epld->epl_wq, &polling_work,msecs_to_jiffies(0));
#endif
//>2012/09/10-13741-kevincheng
            mutex_unlock(&ps_get_sensor_value_mutex);

            EPL_DEBUG_INTERFACE("[ELAN] Proximity Sensor set pflag %d\n",flag);

            break;

        case ELAN_EPL6881_IOCTL_GETDATA:

            value = gRawData.ps_raw;
            if(copy_to_user(argp, &value , sizeof(value)))
                return -EFAULT;

            EPL_DEBUG_INTERFACE("[ELAN] proximity Sensor get data (%d) \n",value);

            break;

        default:
            pr_err("[ELAN epl6881 error]%s: invalid cmd %d\n",__func__,_IOC_NR(cmd));
            return -EINVAL;
    }

    return 0;

}


static struct file_operations elan_ps_fops =
{
    .owner = THIS_MODULE,
    .open = elan_ps_open,
    .release = elan_ps_release,
    .unlocked_ioctl = elan_ps_ioctl
};

static struct miscdevice elan_ps_device =
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = "elan_ps",
    .fops = &elan_ps_fops
};

static int initial_epl6881(struct elan_epl_data *epld)
{
    struct i2c_client *client = epld->client;
    struct hwmsen_object obj_ps, obj_als;
    int ret = 0;

    printk("[ELAN epl6881]%s enter!\n",__func__);


    ret = elan_epl6881_I2C_Read(client);

    if(ret < 0)
        return -EINVAL;

    //Set proximity sensor High threshold and Low threshold
    set_psensor_intr_threshold(P_SENSOR_LTHD,P_SENSOR_HTHD);

	//<<EricHsieh,2012/10/29,Power consumption tuning on epl6881
    //set register 9
    elan_epl6881_I2C_Write(client,REG_9,W_SINGLE_BYTE,0x02,EPL_INT_FRAME_ENABLE);  //setting interrupt pin = binary
	//>>EricHsieh,2012/10/29,Power consumption tuning on epl6881

    /*restart sensor*/
    elan_epl6881_I2C_Write(client,REG_7,W_SINGLE_BYTE,0X02,EPL_C_RESET);
    ret = elan_epl6881_I2C_Write(client,REG_7,W_SINGLE_BYTE,0x02,EPL_C_START_RUN);

    msleep(2);

    epld->ps_irq_enabled = PS_IRQ_ENABLED;
    epld->als_ax2 = ALS_AX2;
    epld->als_bx = ALS_BX;
    epld->als_c = ALS_C;
    epld->als_ax = ALS_AX;
    epld->als_b = ALS_B;
    epld->als_f_threshold= ALS_F_THRESHOLD;
    epld->als_i_threshold = ALS_I_THRESHOLD;
    epld->als_m_threshold = ALS_M_THRESHOLD;

    epld->enable_lflag = 0;
    epld->enable_pflag = 0; // don't change it, since read calibration file can not be too early

    return ret;
}



static int lightsensor_setup(struct elan_epl_data *epld)
{
    int err = 0;
    printk("[ELAN epl6881]%s\n",__func__);

    epld->als_input_dev = input_allocate_device();
    if (!epld->als_input_dev)
    {
        pr_err("[ELAN epl6881 error]%s: could not allocate ls input device\n",__func__);
        return -ENOMEM;
    }
    epld->als_input_dev->name = ElanALsensorName;
    set_bit(EV_ABS, epld->als_input_dev->evbit);
    input_set_abs_params(epld->als_input_dev, ABS_MISC, 0, 9, 0, 0);

    err = input_register_device(epld->als_input_dev);
    if (err < 0)
    {
        pr_err("[ELAN epl6881 error]%s: can not register ls input device\n",__func__);
        goto err_free_ls_input_device;
    }

    err = misc_register(&elan_als_device);
    if (err < 0)
    {
        pr_err("[ELAN epl6881 error]%s: can not register ls misc device\n",__func__);
        goto err_unregister_ls_input_device;
    }

    return err;

err_unregister_ls_input_device:
    input_unregister_device(epld->als_input_dev);
err_free_ls_input_device:
    input_free_device(epld->als_input_dev);
    return err;
}

static int psensor_setup(struct elan_epl_data *epld)
{
    int err = 0;
    printk("[ELAN epl6881]%s\n",__func__);

    epld->ps_input_dev = input_allocate_device();
    if (!epld->ps_input_dev)
    {
        pr_err("[ELAN epl6881 error]%s: could not allocate ps input device\n",__func__);
        return -ENOMEM;
    }
    epld->ps_input_dev->name = ElanPsensorName;

    set_bit(EV_ABS, epld->ps_input_dev->evbit);
    input_set_abs_params(epld->ps_input_dev, ABS_DISTANCE, 0, 1, 0, 0);

    err = input_register_device(epld->ps_input_dev);
    if (err < 0)
    {
        pr_err("[ELAN epl6881 error]%s: could not register ps input device\n",__func__);
        goto err_free_ps_input_device;
    }

    err = misc_register(&elan_ps_device);
    if (err < 0)
    {
        pr_err(
            "[ELAN epl6881 error]%s: could not register ps misc device\n",__func__);
        goto err_unregister_ps_input_device;
    }

    return err;

err_unregister_ps_input_device:
    input_unregister_device(epld->ps_input_dev);
err_free_ps_input_device:
    input_free_device(epld->ps_input_dev);
    return err;
}


static int epl6881_setup(struct elan_epl_data *epld)
{
    struct i2c_client *client = epld->client;

    int err = 0;
    msleep(5);

    printk("[ELAN epl6881] %s\n",__func__);	

    err = initial_epl6881(epld);
    if (err < 0)
    {
        pr_err("[epl6881 error]%s: fail to initial epl6881 (%d)\n",__func__,err);
        goto initial_fail;
    }

    return err;

initial_fail:
fail_free_intr_pin:
    gpio_free(epld->intr_pin);
    return err;


}


#ifndef CONFIG_SUSPEND
static void elan_epl6881_early_suspend(struct early_suspend *h)
{

    struct elan_epl_data *epld = epl_data;
    struct i2c_client *client = epld->client;

    printk("[ELAN epl6881] %s\n", __func__);

    if(epld->enable_lflag)
    {
        cancel_delayed_work(&polling_work);
        elan_epl6881_I2C_Write(client,REG_7, W_SINGLE_BYTE, 0x02, EPL_C_P_DOWN);
    }

    if(epld->enable_pflag)
    {
        if(IRQ_ENABLE)
        {
            cancel_delayed_work(&polling_work);
            elan_epl6881_I2C_Write(client,REG_7, W_SINGLE_BYTE, 0x02,EPL_C_P_DOWN);
        }
        //printk("---psensor_mode_suspend 0---\n\n");
        psensor_mode_suspend = 1;
    }
	mt65xx_eint_mask(CUST_EINT_ALS_NUM); //EricHsieh,2012/10/17,add the EP16881 INT mask for power consumption
}

static void elan_epl6881_late_resume(struct early_suspend *h)
{

    struct elan_epl_data *epld = epl_data;
    struct i2c_client *client = epld->client;

    printk("[ELAN epl6881] %s\n", __func__);
	
	mt65xx_eint_unmask(CUST_EINT_ALS_NUM);	//EricHsieh,2012/10/17,add the EP16881 INT mask for power consumption

    if(epld->enable_lflag)
    {
        elan_epl6881_I2C_Write(client,REG_7, W_SINGLE_BYTE, 0x02, EPL_C_P_UP);
        queue_delayed_work(epld->epl_wq, &polling_work,msecs_to_jiffies(ALS_POLLING_RATE));
    }

    if(epld->enable_pflag)
    {
        if(IRQ_ENABLE)
        {
            queue_delayed_work(epld->epl_wq, &polling_work,msecs_to_jiffies(PS_POLLING_RATE));
            elan_epl6881_I2C_Write(client,REG_7, W_SINGLE_BYTE, 0x02, EPL_C_P_DOWN);
            //printk("---psensor_mode_suspend 0---\n\n");
        }
        psensor_mode_suspend = 0;
    }

}
#endif

//<2012/09/10-13741-kevincheng , Calibration Improvement
//<2013/04/19-24085-kevincheng,Cover Light Sensor can't turn off issue
void epl6881_calibration_polling(void)
{
     struct elan_epl_data *epld = epl_data;

     EPL_DEBUG_INTERFACE("[ELAN_epl6881]epld->enable_pflag : %d\n",epld->enable_pflag);
     epld->enable_pflag = 1; 
     elan_epl6881_psensor_enable(epld);
     msleep(10);
     elan_ps_get_data(true);
     elan_epl6881_disable(epld);

}
//>2013/04/19-24085-kevincheng
//>2012/09/10-13741-kevincheng
int epl6881_ps_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	hwm_sensor_data* sensor_data;
	struct elan_epl_data *epld = (struct elan_epl_data *)self;
	
	EPL_DEBUG_INTERFACE("[ELAN epl6881]%s , command : %d\n",__func__,command);
	switch (command)
	{
		case SENSOR_DELAY:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				APS_ERR("Set delay parameter error!\n");
				err = -EINVAL;
			}
			// Do nothing
			break;

		case SENSOR_ENABLE:
			_is_ps_first_frame = 1;
//<2013/05/31-25693-kevincheng , PS saturate issue
                      CH1_value = 1;
                      CH1_data = 0;
//>2013/05/31-25693-kevincheng
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
				       EPL_DEBUG_INTERFACE("[ELAN epl6881] >> ps enable <<\n");
	                            epld->enable_pflag = 1;
					err = elan_epl6881_psensor_enable(epld);
				       if(err < 0)
					{
						APS_ERR("enable ps fail: %d\n", err); 
						return -1;
					}
				}
				else
				{
				       EPL_DEBUG_INTERFACE("[ELAN epl6881] >> ps disable <<\n");
					err = elan_epl6881_disable(epld);
					//<2012/09/10-13741-kevincheng , Calibration Improvement
					Dyna_en = 0;
					//>2012/09/10-13741-kevincheng
					epld->enable_pflag = 0;
					if(err < 0)
					{
						APS_ERR("disable ps fail: %d\n", err); 
						return -1;
					}
				}
				elan_ep16881_psensor_lock(epld->enable_pflag);	//EricHsieh,2012/10/29 ,add the wakelock for Psensor
			}
			break;

		case SENSOR_GET_DATA:
			if(_is_ps_first_frame){
			     msleep(50);
			     _is_ps_first_frame = 0;}
			
			if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
			{
				APS_ERR("get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
			       EPL_DEBUG_INTERFACE("[ELAN epl6881] --- Sensor Get Data ---\n");
				sensor_data = (hwm_sensor_data *)buff_out;				
				sensor_data->values[0] = elan_ps_get_data(false);
				sensor_data->value_divide = 1;
				sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;	
			}
			break;
		default:
			APS_ERR("proxmy sensor operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}
	
	return err;
}

//<<EricHsieh,2012/10/29 ,add the wakelock for Psensor
void elan_ep16881_psensor_lock(int enable_pflag){

	EPL_DEBUG_INTERFACE("[%s], enable_pflag=%d\n",__func__,enable_pflag);
	
	if(enable_pflag){
		wake_lock(&g_ps_wlock);
	}
	else{
		wake_unlock(&g_ps_wlock);
		}
	
}
//>>EricHsieh,2012/10/29 ,add the wakelock for Psensor


int epl6881_als_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	hwm_sensor_data* sensor_data;
       struct elan_epl_data *epld = (struct elan_epl_data *)self;
	
	EPL_DEBUG_INTERFACE("[ELAN epl6881]%s , command : %d\n",__func__,command);
	switch (command)
	{
		case SENSOR_DELAY:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				APS_ERR("Set delay parameter error!\n");
				err = -EINVAL;
			}
			// Do nothing
			break;

		case SENSOR_ENABLE:
			_is_als_first_frame = 1;
		       if(!epld->enable_pflag)
		         {
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
				       EPL_DEBUG_INTERFACE("[ELAN epl6881] >> als enable <<\n");
					epld->enable_lflag = 1;
					err = elan_epl6881_lsensor_enable(epld);
					if(err < 0)
					{
						APS_ERR("enable als fail: %d\n", err); 
						return -1;
					}
				     }
				   else
				     {
				       EPL_DEBUG_INTERFACE("[ELAN epl6881] >> als disable <<\n");
					err = elan_epl6881_disable(epld);
					epld->enable_lflag = 0;
					if(err < 0)
					{
						APS_ERR("disable als fail: %d\n", err); 
						return -1;
					}
				      }
			            }	
		 	       }
			break;

		case SENSOR_GET_DATA:
		    if(!epld->enable_pflag)
		    {
		       if(_als_reinit == true){
			   EPL_DEBUG_INTERFACE("[ELAN_epl6881] als re-init\n");
			   err = elan_epl6881_lsensor_enable(epld);
			   if(err < 0)
		             {
			        APS_ERR("enable als fail: %d\n", err); 
				 return -1;
				}
			    _als_reinit = false;
			   }
					
			if(_is_als_first_frame){
			     msleep(50);
			     _is_als_first_frame = 0;}
			    
			if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
			{
				APS_ERR("get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
		//	       EPL_DEBUG_INTERFACE("[ELAN epl6884] ALS -- SENSOR GET DATA --\n");
				sensor_data = (hwm_sensor_data *)buff_out;
				
				sensor_data->values[0] = elan_als_get_data();
				sensor_data->value_divide = 1;
				sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;				
			}
		     }
		   else
		     {
		      EPL_DEBUG_INTERFACE("[ELAN epl6881] als pending\n");
		       _als_reinit = true ;
			_is_als_first_frame = true;   
//<2013/04/19-24085-kevincheng,Cover Light Sensor can't turn off issue
			sensor_data->values[0] = gRawData.lux;
			sensor_data->value_divide = 1;
			sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
//>2013/04/19-24085-kevincheng
		      }
			break;
		default:
			APS_ERR("light sensor operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}
	
	return err;
}

static int epl6881_i2c_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
    int err = 0;
    struct elan_epl_data *epld ;
    struct hwmsen_object obj_ps, obj_als;
//<2012/09/10-13741-kevincheng , Claibration Improvement
    struct platform_device *sensor_dev;
//>2012/09/10-13741-kevincheng

    printk("[ELAN epl6881] %s\n", __func__);
    epld = kzalloc(sizeof(struct elan_epl_data), GFP_KERNEL);
    if (!epld)
        return -ENOMEM;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
    {
        dev_err(&client->dev,"No supported i2c func what we need?!!\n");
        err = -ENOTSUPP;
        goto i2c_fail;
    }

    epld->hw = get_cust_alsps_hw();
    epl6881_get_addr(epld->hw,&epld->addr);
    INIT_DELAYED_WORK(&epld->polling_work, polling_do_work);
    INIT_DELAYED_WORK(&epld->epl_sensor_irq_work, epl_sensor_irq_do_work);

    client->irq = GPIO_ALS_EINT_PIN;
//<2013/03/26-23184-kevincheng , Change ALSPS APK Para
    mt_set_gpio_mode( GPIO_ALS_EINT_PIN, GPIO_MODE_00 );
    mt_set_gpio_dir( GPIO_ALS_EINT_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out( GPIO_ALS_EINT_PIN, GPIO_OUT_ONE );
//>2013/03/26-23184-kevincheng
    printk("[ELAN epl6881] : GPIO_ALS_EINT_PIN : %d\n",GPIO_ALS_EINT_PIN);	
    client->addr |= I2C_ENEXT_FLAG;
    epld->client = client;
    epld->irq = client->irq;
    i2c_set_clientdata(client, epld);

    epld->epl_intthreshold_hvalue = P_SENSOR_HTHD; //high threshold
    epld->epl_intthreshold_lvalue = P_SENSOR_LTHD; //low threshold
    epld->epl_irq_pin_state = 0;
    epld->epl_adc_item = 0x01;  //10 bits ADC
    epld->epl_average_item = 0x05; // 32 times sensing average
    epld->epl_integrationtime_index = 1;
    epld->epl_integrationtime_item = als_intT_selection[epld->epl_integrationtime_index];  
    //integration time (proximity mode = 144us, ambient light mode = 128 us)
    epld->epl_integrationtime = als_intT_table[epld->epl_integrationtime_item];
    epld->epl_intpersistency = 0x00; //Interrupt Persistence = 1 time
    epld->epl_sc_mode = 0x00; //continuous mode
    epld->epl_sensor_gain_item = 0x02; //auto gain
    epld->epl_op_mode = 0x00; //ambient light mode
    epld->mode_flag=0x00;

    epl_data = epld;

    mutex_init(&als_enable_mutex);
    mutex_init(&ps_enable_mutex);
    mutex_init(&als_get_sensor_value_mutex);
    mutex_init(&ps_get_sensor_value_mutex);
#if 0
    epld->epl_wq = create_singlethread_workqueue("epl6881_wq");
    if (!epld->epl_wq)
    {
        pr_err("[ELAN epl6881 error]%s: can't create workqueue\n", __func__);
        err = -ENOMEM;
        goto err_create_singlethread_workqueue;
    }
#endif

    err = lightsensor_setup(epld);
    if (err < 0)
    {
        pr_err("[epl6881 error]%s: lightsensor_setup error!!\n",__func__);
        goto err_lightsensor_setup;
    }

    err = psensor_setup(epld);
    if (err < 0)
    {
        pr_err("[epl6881 error]%s: psensor_setup error!!\n",__func__);
        goto err_psensor_setup;
    }

    err = epl6881_setup(epld);
    if (err < 0)
    {
        pr_err("[ELAN epl6881 error]%s: epl6801_setup error!\n", __func__);
        goto err_epl6801_setup;
    }

	obj_ps.self = epld;
	if(1 == epld->hw->polling_mode_ps)
	{
	  obj_ps.polling = 1;
	}
	else
	{
	  obj_ps.polling = 0;//interrupt mode
	}
	obj_ps.sensor_operate = epl6881_ps_operate;
	if((err = hwmsen_attach(ID_PROXIMITY, &obj_ps)))
	{
		APS_ERR("attach fail = %d\n", err);
		goto exit_create_attr_failed;
	}
	printk("[ELAn epl6881] PS init succecc with %s mode\n",obj_ps.polling == 1?"Polling":"Interrupt");
	
	obj_als.self = epld;
	if(1 == epld->hw->polling_mode_als)
	{
	  obj_als.polling = 1;
	}
	else
	{
	  obj_als.polling = 0;//interrupt mode
	}
	obj_als.sensor_operate = epl6881_als_operate;
	if((err = hwmsen_attach(ID_LIGHT, &obj_als)))
	{
		APS_ERR("attach fail = %d\n", err);
		goto exit_create_attr_failed;
	}
	printk("[ELAn epl6881] ALS init succecc with %s mode\n",obj_als.polling == 1?"Polling":"Interrupt");
	
#ifndef CONFIG_SUSPEND
    epld->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
    epld->early_suspend.suspend = elan_epl6881_early_suspend;
    epld->early_suspend.resume = elan_epl6881_late_resume;
    register_early_suspend(&epld->early_suspend);
#endif
	//<<EricHsieh,2012/10/29 ,add the wakelock for Psensor
    wake_lock_init(&g_ps_wlock, WAKE_LOCK_SUSPEND, "ps_wakelock");
	//>>EricHsieh,2012/10/29 ,add the wakelock for Psensor

#if 0
    err = queue_delayed_work(epld->epl_wq, &polling_work,msecs_to_jiffies(5000));
    if(err<0)
    {
        pr_err("[ELAN epl6801 error]%s: queue_delayed_work %d fail\n",__func__,err);
        goto err_delayed_work;
    }
#endif

//<2012/09/10-13741-kevincheng , Calibration Improvement
    sensor_dev = platform_device_register_simple("elan_epl6801", -1, NULL, 0); 
    if (IS_ERR(sensor_dev)) 
    { 
        printk ("sensor_dev_init: error\n"); 
        goto err_fail; 
    } 

    err = sysfs_create_group(&sensor_dev->dev.kobj, &ets_attr_group);
    if (err !=0)
    {
        dev_err(&client->dev,"%s:create sysfs group error", __func__);
        goto err_fail;
    }
//>2012/09/10-13741-kevincheng

    printk(">> elan epl6801 sensor probe success.\n");

    return err;

err_fail:
err_delayed_work:
    input_unregister_device(epld->als_input_dev);
    input_unregister_device(epld->ps_input_dev);
    input_free_device(epld->als_input_dev);
    input_free_device(epld->ps_input_dev);
exit_create_attr_failed:
    misc_deregister(&elan_als_device);
    misc_deregister(&elan_ps_device);
err_lightsensor_setup:
err_psensor_setup:
err_epl6801_setup:
//<2012/10/10-14926-kevincheng,Fix Error When Psensor init fail
    //destroy_workqueue(epld->epl_wq);
//>2012/10/10-14926-kevincheng
    mutex_destroy(&als_enable_mutex);
    mutex_destroy(&ps_enable_mutex);
    mutex_destroy(&als_get_sensor_value_mutex);
    mutex_destroy(&ps_get_sensor_value_mutex);
    misc_deregister(&elan_ps_device);
    misc_deregister(&elan_als_device);
err_create_singlethread_workqueue:
i2c_fail:
//err_platform_data_null:
    kfree(epld);
    return err;
}

static int epl6881_i2c_remove(struct i2c_client *client)
{
    struct elan_epl_data *epld = i2c_get_clientdata(client);

    printk("[ELAN epl6881] %s\n", __func__);

//<2012/10/10-14926-kevincheng,Fix Error When Psensor init fail
    //unregister_early_suspend(&epld->early_suspend);
    input_unregister_device(epld->als_input_dev);
    input_unregister_device(epld->ps_input_dev);
    input_free_device(epld->als_input_dev);
    input_free_device(epld->ps_input_dev);
    misc_deregister(&elan_ps_device);
    misc_deregister(&elan_als_device);
    //free_irq(epld->irq,epld);
    //destroy_workqueue(epld->epl_wq);
    kfree(epld);
    return 0;
//>2012/10/10-14926-kevincheng
}

//<<EricHsieh,2012/10/29,Power consumption tuning on epl6881
#ifdef CONFIG_SUSPEND
static int epl6881_suspend(struct i2c_client *client, pm_message_t mesg)
{
    printk("[ELAN epl6881] [%s] [enable_lflag,enable_pflag,ps_irq_enabled]=%d,%d,%d\n", __func__,epl_data->enable_lflag,epl_data->enable_pflag,epl_data->ps_irq_enabled);

    elan_epl6881_I2C_Write(client,REG_7, W_SINGLE_BYTE, 0x02, EPL_C_P_DOWN);

    return 0;
}

static int epl6881_resume(struct i2c_client *client)
{
    printk("[ELAN epl6881] [%s] [enable_lflag,enable_pflag,ps_irq_enabled]=%d,%d,%d\n", __func__,epl_data->enable_lflag,epl_data->enable_pflag,epl_data->ps_irq_enabled);

	if(epl_data->enable_lflag || epl_data->enable_pflag)

        elan_epl6881_I2C_Write(client,REG_7, W_SINGLE_BYTE, 0x02, EPL_C_P_UP);

    return 0;
}
#endif


static const struct i2c_device_id elan_epl6881_id[] = {
	{ ELAN_LS_6881, 0 },
	{ }
};

static struct i2c_driver epl6881_i2c_driver =
{
    .probe	       = epl6881_i2c_probe,
    .remove	= epl6881_i2c_remove,
    .id_table	= elan_epl6881_id,
    .driver	= {
                   .name = "ELAN_LS_6881",
                //.owner = THIS_MODULE,
                  },
#ifdef CONFIG_SUSPEND
    .suspend = epl6881_suspend,
    .resume  = epl6881_resume,
#endif
};

static int epl6881_probe(struct platform_device *pdev) 
{
	struct alsps_hw *hw = get_cust_alsps_hw();
	   
       printk("[ELAN]%s\n",__func__);

	hwPowerOn(hw->power_id, hw->power_vol, "elan_alsps" );
      	   
	if(i2c_add_driver(&epl6881_i2c_driver))
	{
		APS_ERR("add driver error\n");
		return -1;
	} 

	return 0;
}

static int epl6881_remove(struct platform_device *pdev)
{
	APS_FUN();       
	i2c_del_driver(&epl6881_i2c_driver);
	
	return 0;
}

static struct platform_driver epl6881_alsps_driver = {
	.probe      = epl6881_probe,
	.remove     = epl6881_remove,    
	.driver     = {
		.name  = "als_ps",
//		.owner = THIS_MODULE,
	}
};

static int __init elan_epl6881_init(void)
{
    printk("[ELAN epl6881] %s\n", __func__);

    struct alsps_hw *hw = get_cust_alsps_hw();
    printk("[ELAN_epl6881] i2c_number = %d\n",hw->i2c_num);
    i2c_register_board_info(hw->i2c_num, &epl6881_i2c_alsps, 1);

    if(platform_driver_register(&epl6881_alsps_driver))
	{
		APS_ERR(">> failed to register EPL6881 <<");
		return -ENODEV;
	}

	return 0;
}

static void __exit  elan_epl6881_exit(void)
{
    printk("[ELAN epl6881] %s\n", __func__);
	
    platform_driver_unregister(&epl6881_alsps_driver);
}

module_init(elan_epl6881_init);
module_exit(elan_epl6881_exit);

MODULE_AUTHOR("Cheng-Wei Lin <dusonlin@emc.com.tw>");
MODULE_DESCRIPTION("ELAN epl6881 driver");
MODULE_LICENSE("GPL");


