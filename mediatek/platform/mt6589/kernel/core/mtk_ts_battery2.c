

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/dmi.h>
#include <linux/acpi.h>
#include <linux/thermal.h>
#include <linux/platform_device.h>
#include <linux/aee.h>
#include <linux/xlog.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/writeback.h>
#include <asm/uaccess.h>

#include <mach/system.h>
#include "mach/mtk_thermal_monitor.h"
#include "mach/mt_typedefs.h"
#include "mach/mt_thermal.h"


//#include <cust_battery.h>  
#include "mt6320_battery.h"

static unsigned int interval = 0; /* seconds, 0 : no auto polling */
static unsigned int trip_temp[10] = {120000,110000,100000,90000,80000,70000,65000,60000,55000,50000};
//static unsigned int cl_dev_dis_charge_state = 0;
//static unsigned int cl_dev_sysrst_state = 0;
static struct thermal_zone_device *thz_dev;
//static struct thermal_cooling_device *cl_dev_dis_charge;
//static struct thermal_cooling_device *cl_dev_sysrst;
static int mtktsbattery2_debug_log = 0;
static int kernelmode = 0;
static int g_THERMAL_TRIP[10] = {0,0,0,0,0,0,0,0,0,0};
static int num_trip=0;
static char g_bind0[20]={0};
static char g_bind1[20]={0};
static char g_bind2[20]={0};
static char g_bind3[20]={0};
static char g_bind4[20]={0};
static char g_bind5[20]={0};
static char g_bind6[20]={0};
static char g_bind7[20]={0};
static char g_bind8[20]={0};
static char g_bind9[20]={0};

extern int MA_len;
extern int read_tbat_value(void);
//static int battery2_write_flag=0;

#define mtktsbattery2_TEMP_CRIT 60000 /* 60.000 degree Celsius */

#define mtktsbattery2_dprintk(fmt, args...)   \
do {                                    \
	if (mtktsbattery2_debug_log) {                \
		xlog_printk(ANDROID_LOG_INFO, "Power/battery2_Thermal", fmt, ##args); \
	}                                   \
} while(0)

/*
 * kernel fopen/fclose
 */
/* 
static mm_segment_t oldfs;

static void my_close(int fd)
{
	set_fs(oldfs);
	sys_close(fd);
}

static int my_open(char *fname, int flag)
{
	oldfs = get_fs();
    set_fs(KERNEL_DS);
    return sys_open(fname, flag, 0);
}
*/
extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int* rawdata);
extern int IMM_IsAdcInitReady(void);
typedef struct{
    INT32 BatteryTemp;
    INT32 TemperatureR;
}BATT_TEMPERATURE;

#define BAT_NTC_10 1
#define BAT_NTC_47 0
#define RBAT_PULL_UP_R             39000
#define TBAT_OVER_CRITICAL_LOW     68237
#define RBAT_PULL_UP_VOLT          1800

static int g_BAT_TemperatureR = 0;

/* convert register to temperature  */
static INT16 BattThermistorConverTemp(INT32 Res)
{
    int i=0;
    INT32 RES1=0,RES2=0;
    INT32 TBatt_Value=-200,TMP1=0,TMP2=0;

#if defined(BAT_NTC_BL197)
BATT_TEMPERATURE Batt_Temperature_Table[] = {
{-20,74354},    
{-15,57626},
{-10,45068},
{ -5,35548},
{  0,28267},
{  5,22650},
{ 10,18280},
{ 15,14855},
{ 20,12151},
{ 25,10000},
{ 30,8279},
{ 35,6892},
{ 40,5768},
{ 45,4852},
{ 50,4101},
{ 55,3483},
{ 60,2970}
};
#endif

#if defined(BAT_NTC_TSM_1)
BATT_TEMPERATURE Batt_Temperature_Table[] = {
{-20,70603},    
{-15,55183},
{-10,43499},
{ -5,34569},
{  0,27680},
{  5,22316},
{ 10,18104},
{ 15,14773},
{ 20,12122},
{ 25,10000},
{ 30,8294},
{ 35,6915},
{ 40,5795},
{ 45,4882},
{ 50,4133},
{ 55,3516},
{ 60,3004}
};
#endif

#if defined(BAT_NTC_10_SEN_1)        
BATT_TEMPERATURE Batt_Temperature_Table[] = {
 {-20,74354},
 {-15,57626},
 {-10,45068},
 { -5,35548},
 {  0,28267},
 {  5,22650},
 { 10,18280},
 { 15,14855},
 { 20,12151},
 { 25,10000},
 { 30,8279},
 { 35,6892},
 { 40,5768},
 { 45,4852},
 { 50,4101},
 { 55,3483},
 { 60,2970}
};
#endif

#if (BAT_NTC_10 == 1)
    BATT_TEMPERATURE Batt_Temperature_Table[] = {
        {-20,68237},
        {-15,53650},
        {-10,42506},
        { -5,33892},
        {  0,27219},
        {  5,22021},
        { 10,17926},
        { 15,14674},
        { 20,12081},
        { 25,10000},
        { 30,8315},
        { 35,6948},
        { 40,5834},
        { 45,4917},
        { 50,4161},
        { 55,3535},
        { 60,3014}
    };
#endif

#if (BAT_NTC_47 == 1)
    BATT_TEMPERATURE Batt_Temperature_Table[] = {
        {-20,483954},
        {-15,360850},
        {-10,271697},
        { -5,206463},
        {  0,158214},
        {  5,122259},
        { 10,95227},
        { 15,74730},
        { 20,59065},
        { 25,47000},
        { 30,37643},
        { 35,30334},
        { 40,24591},
        { 45,20048},
        { 50,16433},
        { 55,13539},
        { 60,11210}        
    };
#endif

    if(Res>=Batt_Temperature_Table[0].TemperatureR)
    {
        #if 0
        xlog_printk(ANDROID_LOG_DEBUG, "Power/Battery", "Res>=%d\n", Batt_Temperature_Table[0].TemperatureR);
        #endif
        TBatt_Value = -20;
    }
    else if(Res<=Batt_Temperature_Table[16].TemperatureR)
    {
        #if 0
        xlog_printk(ANDROID_LOG_DEBUG, "Power/Battery", "Res<=%d\n", Batt_Temperature_Table[16].TemperatureR);
        #endif
        TBatt_Value = 60;
    }
    else
    {
        RES1=Batt_Temperature_Table[0].TemperatureR;
        TMP1=Batt_Temperature_Table[0].BatteryTemp;

        for(i=0;i<=16;i++)
        {
            if(Res>=Batt_Temperature_Table[i].TemperatureR)
            {
                RES2=Batt_Temperature_Table[i].TemperatureR;
                TMP2=Batt_Temperature_Table[i].BatteryTemp;
                break;
            }
            else
            {
                RES1=Batt_Temperature_Table[i].TemperatureR;
                TMP1=Batt_Temperature_Table[i].BatteryTemp;
            }
        }
        
        TBatt_Value = (((Res-RES2)*TMP1)+((RES1-Res)*TMP2))/(RES1-RES2);
    }

    #if 0
    xlog_printk(ANDROID_LOG_DEBUG, "Power/Battery", "BattThermistorConverTemp() : TBatt_Value = %d\n",TBatt_Value);
    xlog_printk(ANDROID_LOG_DEBUG, "Power/Battery", "BattThermistorConverTemp() : Res = %d\n",Res);
    xlog_printk(ANDROID_LOG_DEBUG, "Power/Battery", "BattThermistorConverTemp() : RES1 = %d\n",RES1);
    xlog_printk(ANDROID_LOG_DEBUG, "Power/Battery", "BattThermistorConverTemp() : RES2 = %d\n",RES2);
    xlog_printk(ANDROID_LOG_DEBUG, "Power/Battery", "BattThermistorConverTemp() : TMP1 = %d\n",TMP1);
    xlog_printk(ANDROID_LOG_DEBUG, "Power/Battery", "BattThermistorConverTemp() : TMP2 = %d\n",TMP2);
    #endif

    return TBatt_Value;    
}

/* convert ADC_bat_temp_volt to register */
static INT16 BattVoltToTemp(UINT32 dwVolt)
{
    INT32 TRes;
    INT32 dwVCriBat = 0; 
    INT32 sBaTTMP = -100;

    //SW workaround-----------------------------------------------------
    //dwVCriBat = (TBAT_OVER_CRITICAL_LOW * 1800) / (TBAT_OVER_CRITICAL_LOW + 39000);
    dwVCriBat = (TBAT_OVER_CRITICAL_LOW * RBAT_PULL_UP_VOLT) / (TBAT_OVER_CRITICAL_LOW + RBAT_PULL_UP_R);
        
    if(dwVolt > dwVCriBat)
    {
        TRes = TBAT_OVER_CRITICAL_LOW;
    }
    else
    {
        //TRes = (39000*dwVolt) / (1800-dwVolt);
        TRes = (RBAT_PULL_UP_R*dwVolt) / (RBAT_PULL_UP_VOLT-dwVolt);    
    }
    //------------------------------------------------------------------

    g_BAT_TemperatureR = TRes;

    /* convert register to temperature */
    sBaTTMP = BattThermistorConverTemp(TRes);
  
    return sBaTTMP;
}

static int get_hw_battery2_temp(void)
{

	int ret = 0, data[4], i, ret_value = 0, ret_temp = 0, output;
	int times=1, Channel=1;
	
	if( IMM_IsAdcInitReady() == 0 )
	{
        mtktsbattery2_dprintk("[thermal_auxadc_get_data]: AUXADC is not ready\n");
		return 0;
	}

	i = times;
	while (i--)
	{
		ret_value = IMM_GetOneChannelValue(Channel, data, &ret_temp);
		ret += ret_temp;
		mtktsbattery2_dprintk("[thermal_auxadc_get_data(ADCIN1)]: ret_temp=%d\n",ret_temp);        
	}

#if 0
	Channel = 0;
	ret = 0 ;
	ret_temp = 0;
	i = times;
	while (i--)
	{
		ret_value = IMM_GetOneChannelValue(Channel, data, &ret_temp);
		ret += ret_temp;
		printk("[thermal_auxadc_get_data(ADCIN %d)]: ret_temp=%d\n",Channel,ret_temp);        
	}
	
	Channel = 2;
	ret = 0 ;
	ret_temp = 0;	
	i = times;
	while (i--)
	{
		ret_value = IMM_GetOneChannelValue(Channel, data, &ret_temp);
		ret += ret_temp;
		printk("[thermal_auxadc_get_data(ADCIN %d)]: ret_temp=%d\n",Channel,ret_temp);        
	}
#endif

	ret = ret*1500/4096	;
	mtktsbattery2_dprintk("Battery output mV = %d\n",ret);
	output = BattVoltToTemp(ret);
	mtktsbattery2_dprintk("Battery output temperature = %d\n",output);
	
	return output;
}

static DEFINE_MUTEX(battery2_lock);
int ts_battery2_at_boot_time=0;
static int mtktsbattery2_get_hw_temp(void)
{
	int t_ret=0;
	static int battery2[60]={0};
	static int counter=0, first_time=0;
	int i=0;
	
	mutex_lock(&battery2_lock);

    //get HW battery2 temp (TSbattery2)
    //cat /sys/class/power_supply/battery2/batt_temp
	t_ret = get_hw_battery2_temp();
	t_ret = t_ret * 1000;
	battery2[counter]=t_ret;

	if(first_time!=0)
	{
		t_ret = 0;
		for(i=0; i<MA_len; i++)
		{
			t_ret += battery2[i];
		}
		t_ret = t_ret / MA_len;
	}

	counter++;
	if(counter==MA_len)
	{
		counter=0;
		first_time=1;
	}
	if(counter>MA_len)
	{
		counter=0;
		first_time=0;
	}
	mutex_unlock(&battery2_lock);

	mtktsbattery2_dprintk("[mtktsbattery2_get_hw_temp] counter=%d, first_time =%d, MA_len=%d\n", counter, first_time, MA_len);
	mtktsbattery2_dprintk("[mtktsbattery2_get_hw_temp] T_battery2, %d\n", t_ret);
	return t_ret;
}
    
static int mtktsbattery2_get_temp(struct thermal_zone_device *thermal,
			       unsigned long *t)
{
	*t = mtktsbattery2_get_hw_temp();
	return 0;
}

static int mtktsbattery2_bind(struct thermal_zone_device *thermal,
			struct thermal_cooling_device *cdev)
{
	int table_val=0;

	if(!strcmp(cdev->type, g_bind0))
	{
		table_val = 0;
		mtktsbattery2_dprintk("[mtktsbattery2_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind1))
	{
		table_val = 1;
		mtktsbattery2_dprintk("[mtktsbattery2_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind2))
	{
		table_val = 2;
		mtktsbattery2_dprintk("[mtktsbattery2_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind3))
	{
		table_val = 3;
		mtktsbattery2_dprintk("[mtktsbattery2_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind4))
	{
		table_val = 4;
		mtktsbattery2_dprintk("[mtktsbattery2_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind5))
	{
		table_val = 5;
		mtktsbattery2_dprintk("[mtktsbattery2_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind6))
	{
		table_val = 6;
		mtktsbattery2_dprintk("[mtktsbattery2_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind7))
	{
		table_val = 7;
		mtktsbattery2_dprintk("[mtktsbattery2_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind8))
	{
		table_val = 8;
		mtktsbattery2_dprintk("[mtktsbattery2_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind9))
	{
		table_val = 9;
		mtktsbattery2_dprintk("[mtktsbattery2_bind] %s\n", cdev->type);
	}
	else
	{	
		return 0;
	}
    
	if (mtk_thermal_zone_bind_cooling_device(thermal, table_val, cdev)) {
		mtktsbattery2_dprintk("[mtktsbattery2_bind] error binding cooling dev\n");
		return -EINVAL;
	} else {
		mtktsbattery2_dprintk("[mtktsbattery2_bind] binding OK, %d\n", table_val);
	}

	return 0;
}

static int mtktsbattery2_unbind(struct thermal_zone_device *thermal,
			  struct thermal_cooling_device *cdev)
{
    int table_val=0;

	if(!strcmp(cdev->type, g_bind0))
	{
		table_val = 0;
		mtktsbattery2_dprintk("[mtktsbattery2_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind1))
	{
		table_val = 1;
		mtktsbattery2_dprintk("[mtktsbattery2_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind2))
	{
		table_val = 2;
		mtktsbattery2_dprintk("[mtktsbattery2_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind3))
	{
		table_val = 3;
		mtktsbattery2_dprintk("[mtktsbattery2_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind4))
	{
		table_val = 4;
		mtktsbattery2_dprintk("[mtktsbattery2_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind5))
	{
		table_val = 5;
		mtktsbattery2_dprintk("[mtktsbattery2_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind6))
	{
		table_val = 6;
		mtktsbattery2_dprintk("[mtktsbattery2_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind7))
	{
		table_val = 7;
		mtktsbattery2_dprintk("[mtktsbattery2_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind8))
	{
		table_val = 8;
		mtktsbattery2_dprintk("[mtktsbattery2_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind9))
	{
		table_val = 9;
		mtktsbattery2_dprintk("[mtktsbattery2_unbind] %s\n", cdev->type);
	}
	else
		return 0;
    
	if (thermal_zone_unbind_cooling_device(thermal, table_val, cdev)) {
		mtktsbattery2_dprintk("[mtktsbattery2_unbind] error unbinding cooling dev\n");
		return -EINVAL;
	} else {
		mtktsbattery2_dprintk("[mtktsbattery2_unbind] unbinding OK\n");
	}

	return 0;
}

static int mtktsbattery2_get_mode(struct thermal_zone_device *thermal,
			    enum thermal_device_mode *mode)
{
	*mode = (kernelmode) ? THERMAL_DEVICE_ENABLED
			     : THERMAL_DEVICE_DISABLED;
	return 0;
}

static int mtktsbattery2_set_mode(struct thermal_zone_device *thermal,
			    enum thermal_device_mode mode)
{
	kernelmode = mode;
	return 0;
}

static int mtktsbattery2_get_trip_type(struct thermal_zone_device *thermal, int trip,
				 enum thermal_trip_type *type)
{
	*type = g_THERMAL_TRIP[trip];
	return 0;
}

static int mtktsbattery2_get_trip_temp(struct thermal_zone_device *thermal, int trip,
				 unsigned long *temp)
{
	*temp = trip_temp[trip]; 
	return 0;
}

static int mtktsbattery2_get_crit_temp(struct thermal_zone_device *thermal,
				 unsigned long *temperature)
{
	*temperature = mtktsbattery2_TEMP_CRIT;
	return 0;
}

/* bind callback functions to thermalzone */
static struct thermal_zone_device_ops mtktsbattery2_dev_ops = {
	.bind = mtktsbattery2_bind,
	.unbind = mtktsbattery2_unbind,
	.get_temp = mtktsbattery2_get_temp,
	.get_mode = mtktsbattery2_get_mode,
	.set_mode = mtktsbattery2_set_mode,
	.get_trip_type = mtktsbattery2_get_trip_type,
	.get_trip_temp = mtktsbattery2_get_trip_temp,
	.get_crit_temp = mtktsbattery2_get_crit_temp,
};

/*
static int dis_charge_get_max_state(struct thermal_cooling_device *cdev,
				 unsigned long *state)
{        
		*state = 1;    
		return 0;
}
static int dis_charge_get_cur_state(struct thermal_cooling_device *cdev,
				 unsigned long *state)
{        
		*state = cl_dev_dis_charge_state;
		return 0;
}
static int dis_charge_set_cur_state(struct thermal_cooling_device *cdev,
				 unsigned long state)
{
    cl_dev_dis_charge_state = state;
    if(cl_dev_dis_charge_state == 1) {
        mtktsbattery2_dprintk("[dis_charge_set_cur_state] disable charging\n");
    }
    return 0;
}
*/
/*
static int sysrst_get_max_state(struct thermal_cooling_device *cdev,
				 unsigned long *state)
{        
	*state = 1;    
	return 0;
}
static int sysrst_get_cur_state(struct thermal_cooling_device *cdev,
				 unsigned long *state)
{        
	*state = cl_dev_sysrst_state;
	return 0;
}
static int sysrst_set_cur_state(struct thermal_cooling_device *cdev,
				 unsigned long state)
{
	cl_dev_sysrst_state = state;
	if(cl_dev_sysrst_state == 1)
	{
		printk("Power/battery2_Thermal: reset, reset, reset!!!");
		printk("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
		printk("*****************************************");
		printk("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");

		BUG();
		//arch_reset(0,NULL);   
	} 
	return 0;
}
*/
/*
static struct thermal_cooling_device_ops mtktsbattery2_cooling_dis_charge_ops = {
	.get_max_state = dis_charge_get_max_state,
	.get_cur_state = dis_charge_get_cur_state,
	.set_cur_state = dis_charge_set_cur_state,
};*/
/*static struct thermal_cooling_device_ops mtktsbattery2_cooling_sysrst_ops = {
	.get_max_state = sysrst_get_max_state,
	.get_cur_state = sysrst_get_cur_state,
	.set_cur_state = sysrst_set_cur_state,
};*/


static int mtktsbattery2_read(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
	int len = 0;
	char *p = buf;
    
	p += sprintf(p, "[mtktsbattery2_read] trip_0_temp=%d,trip_1_temp=%d,trip_2_temp=%d,trip_3_temp=%d,trip_4_temp=%d,\n\
trip_5_temp=%d,trip_6_temp=%d,trip_7_temp=%d,trip_8_temp=%d,trip_9_temp=%d,\n\
g_THERMAL_TRIP_0=%d,g_THERMAL_TRIP_1=%d,g_THERMAL_TRIP_2=%d,g_THERMAL_TRIP_3=%d,g_THERMAL_TRIP_4=%d,\n\
g_THERMAL_TRIP_5=%d,g_THERMAL_TRIP_6=%d,g_THERMAL_TRIP_7=%d,g_THERMAL_TRIP_8=%d,g_THERMAL_TRIP_9=%d,\n\
cooldev0=%s,cooldev1=%s,cooldev2=%s,cooldev3=%s,cooldev4=%s,\n\
cooldev5=%s,cooldev6=%s,cooldev7=%s,cooldev8=%s,cooldev9=%s,time_ms=%d\n",
				trip_temp[0],trip_temp[1],trip_temp[2],trip_temp[3],trip_temp[4],
				trip_temp[5],trip_temp[6],trip_temp[7],trip_temp[8],trip_temp[9],
				g_THERMAL_TRIP[0],g_THERMAL_TRIP[1],g_THERMAL_TRIP[2],g_THERMAL_TRIP[3],g_THERMAL_TRIP[4],
				g_THERMAL_TRIP[5],g_THERMAL_TRIP[6],g_THERMAL_TRIP[7],g_THERMAL_TRIP[8],g_THERMAL_TRIP[9],
				g_bind0,g_bind1,g_bind2,g_bind3,g_bind4,g_bind5,g_bind6,g_bind7,g_bind8,g_bind9,
				interval*1000);
    
    
	*start = buf + off;
    
	len = p - buf;
	if (len > off)
		len -= off;
	else
		len = 0;
    
	return len < count ? len  : count;
}

int mtktsbattery2_register_thermal(void);
void mtktsbattery2_unregister_thermal(void);

static ssize_t mtktsbattery2_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	int len=0,time_msec=0;
	int trip[10]={0};
	int t_type[10]={0};
	int i;
	char bind0[20],bind1[20],bind2[20],bind3[20],bind4[20];
	char bind5[20],bind6[20],bind7[20],bind8[20],bind9[20];
	char desc[512];


	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
	{
		return 0;
	}
	desc[len] = '\0';

	if (sscanf(desc, "%d %d %d %s %d %d %s %d %d %s %d %d %s %d %d %s %d %d %s %d %d %s %d %d %s %d %d %s %d %d %s %d",
				&num_trip, &trip[0],&t_type[0],bind0, &trip[1],&t_type[1],bind1,
				&trip[2],&t_type[2],bind2, &trip[3],&t_type[3],bind3,
				&trip[4],&t_type[4],bind4, &trip[5],&t_type[5],bind5,
				&trip[6],&t_type[6],bind6, &trip[7],&t_type[7],bind7,
				&trip[8],&t_type[8],bind8, &trip[9],&t_type[9],bind9,
				&time_msec) == 32)
	{
		mtktsbattery2_dprintk("[mtktsbattery2_write] mtktsbattery2_unregister_thermal\n");
		mtktsbattery2_unregister_thermal();
	
		for(i=0; i<num_trip; i++)
			g_THERMAL_TRIP[i] = t_type[i];	

		g_bind0[0]=g_bind1[0]=g_bind2[0]=g_bind3[0]=g_bind4[0]=g_bind5[0]=g_bind6[0]=g_bind7[0]=g_bind8[0]=g_bind9[0]='\0';
				
		for(i=0; i<20; i++)
		{
			g_bind0[i]=bind0[i]; 
			g_bind1[i]=bind1[i]; 
			g_bind2[i]=bind2[i]; 
			g_bind3[i]=bind3[i]; 
			g_bind4[i]=bind4[i];
			g_bind5[i]=bind5[i]; 
			g_bind6[i]=bind6[i]; 
			g_bind7[i]=bind7[i]; 
			g_bind8[i]=bind8[i]; 
			g_bind9[i]=bind9[i];
		}

		mtktsbattery2_dprintk("[mtktsbattery2_write] g_THERMAL_TRIP_0=%d,g_THERMAL_TRIP_1=%d,g_THERMAL_TRIP_2=%d,g_THERMAL_TRIP_3=%d,g_THERMAL_TRIP_4=%d,\
g_THERMAL_TRIP_5=%d,g_THERMAL_TRIP_6=%d,g_THERMAL_TRIP_7=%d,g_THERMAL_TRIP_8=%d,g_THERMAL_TRIP_9=%d,\n",
				g_THERMAL_TRIP[0],g_THERMAL_TRIP[1],g_THERMAL_TRIP[2],g_THERMAL_TRIP[3],g_THERMAL_TRIP[4],
				g_THERMAL_TRIP[5],g_THERMAL_TRIP[6],g_THERMAL_TRIP[7],g_THERMAL_TRIP[8],g_THERMAL_TRIP[9]);
	mtktsbattery2_dprintk("[mtktsbattery2_write] cooldev0=%s,cooldev1=%s,cooldev2=%s,cooldev3=%s,cooldev4=%s,\
cooldev5=%s,cooldev6=%s,cooldev7=%s,cooldev8=%s,cooldev9=%s\n",
				g_bind0,g_bind1,g_bind2,g_bind3,g_bind4,g_bind5,g_bind6,g_bind7,g_bind8,g_bind9);

		for(i=0; i<num_trip; i++)
		{
			trip_temp[i]=trip[i];
		}

		interval=time_msec / 1000;

		mtktsbattery2_dprintk("[mtktsbattery2_write] trip_0_temp=%d,trip_1_temp=%d,trip_2_temp=%d,trip_3_temp=%d,trip_4_temp=%d,\
trip_5_temp=%d,trip_6_temp=%d,trip_7_temp=%d,trip_8_temp=%d,trip_9_temp=%d,time_ms=%d\n", 
				trip_temp[0],trip_temp[1],trip_temp[2],trip_temp[3],trip_temp[4],
				trip_temp[5],trip_temp[6],trip_temp[7],trip_temp[8],trip_temp[9],interval*1000);
													
		mtktsbattery2_dprintk("[mtktsbattery2_write] mtktsbattery2_register_thermal\n");
		mtktsbattery2_register_thermal();
				
		//battery2_write_flag=1;
		return count;
	}
	else
	{
		mtktsbattery2_dprintk("[mtktsbattery2_write] bad argument\n");
	}
		
	return -EINVAL;
}


//int  mtktsbattery2_register_cooler(void)
//{
	/* cooling devices */
	//cl_dev_sysrst = mtk_thermal_cooling_device_register("mtktsbattery-sysrst", NULL,
	//	&mtktsbattery2_cooling_sysrst_ops);
	//return 0;
//}

int mtktsbattery2_register_thermal(void)
{
	mtktsbattery2_dprintk("[mtktsbattery2_register_thermal] \n");

	/* trips : trip 0~1 */
	thz_dev = mtk_thermal_zone_device_register("mtktsbattery2", num_trip, NULL,
		&mtktsbattery2_dev_ops, 0, 0, 0, interval*1000);

	return 0;
}

//void mtktsbattery2_unregister_cooler(void)
//{
	//if (cl_dev_sysrst) {
	//	mtk_thermal_cooling_device_unregister(cl_dev_sysrst);
	//	cl_dev_sysrst = NULL;
	//}
//}
void mtktsbattery2_unregister_thermal(void)
{
	mtktsbattery2_dprintk("[mtktsbattery2_unregister_thermal] \n");

	if (thz_dev) {
		mtk_thermal_zone_device_unregister(thz_dev);
		thz_dev = NULL;
	}
}

static int __init mtktsbattery2_init(void)
{
	int err = 0;
	struct proc_dir_entry *entry = NULL;
	struct proc_dir_entry *mtktsbattery2_dir = NULL;

	mtktsbattery2_dprintk("[mtktsbattery2_init] \n");

	//err = mtktsbattery2_register_cooler();
	//if(err)
	//	return err;
	
	err = mtktsbattery2_register_thermal();
	if (err)
		goto err_unreg;

	mtktsbattery2_dir = proc_mkdir("mtktsbattery2", NULL);
	if (!mtktsbattery2_dir)
	{
		mtktsbattery2_dprintk("[mtktsbattery2_init]: mkdir /proc/mtktsbattery2 failed\n");
	}
	else
	{
		entry = create_proc_entry("mtktsbattery2", S_IRUGO | S_IWUSR, mtktsbattery2_dir);
		if (entry)
		{
			entry->read_proc = mtktsbattery2_read;
			entry->write_proc = mtktsbattery2_write;
		}
	}

	return 0;

err_unreg:
	//mtktsbattery2_unregister_cooler();
	return err;
}

static void __exit mtktsbattery2_exit(void)
{
	mtktsbattery2_dprintk("[mtktsbattery2_exit] \n");
	mtktsbattery2_unregister_thermal();
	//mtktsbattery2_unregister_cooler();
}

module_init(mtktsbattery2_init);
module_exit(mtktsbattery2_exit);
