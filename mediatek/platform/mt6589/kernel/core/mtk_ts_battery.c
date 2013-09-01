

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

static unsigned int interval = 0; /* seconds, 0 : no auto polling */
static unsigned int trip_temp[10] = {120000,110000,100000,90000,80000,70000,65000,60000,55000,50000};
//static unsigned int cl_dev_dis_charge_state = 0;
static unsigned int cl_dev_sysrst_state = 0;
static struct thermal_zone_device *thz_dev;
//static struct thermal_cooling_device *cl_dev_dis_charge;
static struct thermal_cooling_device *cl_dev_sysrst;
static int mtktsbattery_debug_log = 0;
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
//static int battery_write_flag=0;

#define mtktsbattery_TEMP_CRIT 60000 /* 60.000 degree Celsius */

#define mtktsbattery_dprintk(fmt, args...)   \
do {                                    \
	if (mtktsbattery_debug_log) {                \
		xlog_printk(ANDROID_LOG_INFO, "Power/Battery_Thermal", fmt, ##args); \
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
static int get_hw_battery_temp(void)
{
/*    
	int fd;
    char buf[64];
    char *pmtdbufp = NULL;
    ssize_t pmtdsize;

    char *pvalue = NULL;
    int got_value=0;

    //open file and read current value
    fd = my_open("/sys/class/power_supply/battery/batt_temp", O_RDONLY);
    if (fd < 0)
    {   
        mtktsbattery_dprintk("[get_hw_battery_temp]: open file fail");
        return 0;
    }
    mtktsbattery_dprintk("[get_hw_battery_temp]: open file ok");
    buf[sizeof(buf) - 1] = '\0';
    pmtdsize = sys_read(fd, buf, sizeof(buf) - 1);
    pmtdbufp = buf;   
    got_value = simple_strtol(pmtdbufp,&pvalue,10);

    // close file
    my_close(fd);

    // debug
    mtktsbattery_dprintk("[get_hw_battery_temp]: got_value=%d\n", got_value);

    return got_value;
*/ 
	int ret=0;
#if defined(CONFIG_POWER_EXT)
	//EVB
	ret = -1270;
#else
	//Phone
	ret = read_tbat_value();
	ret = ret * 10;
#endif	

	return ret;
}

static DEFINE_MUTEX(Battery_lock);
int ts_battery_at_boot_time=0;
static int mtktsbattery_get_hw_temp(void)
{
	int t_ret=0;
	static int battery[60]={0};
	static int counter=0, first_time=0;
	int i=0;

	if(ts_battery_at_boot_time==0)
	{
		ts_battery_at_boot_time=1;
		mtktsbattery_dprintk("[mtktsbattery_get_hw_temp] at boot time, return 25000 as default\n");
		battery[counter]=25000;
		counter++;
		return 25000;
	}

	mutex_lock(&Battery_lock);

    //get HW battery temp (TSBATTERY)
    //cat /sys/class/power_supply/battery/batt_temp
	t_ret = get_hw_battery_temp();
	t_ret = t_ret * 100;
	battery[counter]=t_ret;

	if(first_time!=0)
	{
		t_ret = 0;
		for(i=0; i<MA_len; i++)
		{
			t_ret += battery[i];
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
	mutex_unlock(&Battery_lock);

	if(t_ret)

	mtktsbattery_dprintk("[mtktsbattery_get_hw_temp] counter=%d, first_time =%d, MA_len=%d\n", counter, first_time, MA_len);
	mtktsbattery_dprintk("[mtktsbattery_get_hw_temp] T_Battery, %d\n", t_ret);
	return t_ret;
}
    
static int mtktsbattery_get_temp(struct thermal_zone_device *thermal,
			       unsigned long *t)
{
	*t = mtktsbattery_get_hw_temp();
	return 0;
}

static int mtktsbattery_bind(struct thermal_zone_device *thermal,
			struct thermal_cooling_device *cdev)
{
	int table_val=0;

	if(!strcmp(cdev->type, g_bind0))
	{
		table_val = 0;
		mtktsbattery_dprintk("[mtktsbattery_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind1))
	{
		table_val = 1;
		mtktsbattery_dprintk("[mtktsbattery_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind2))
	{
		table_val = 2;
		mtktsbattery_dprintk("[mtktsbattery_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind3))
	{
		table_val = 3;
		mtktsbattery_dprintk("[mtktsbattery_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind4))
	{
		table_val = 4;
		mtktsbattery_dprintk("[mtktsbattery_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind5))
	{
		table_val = 5;
		mtktsbattery_dprintk("[mtktsbattery_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind6))
	{
		table_val = 6;
		mtktsbattery_dprintk("[mtktsbattery_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind7))
	{
		table_val = 7;
		mtktsbattery_dprintk("[mtktsbattery_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind8))
	{
		table_val = 8;
		mtktsbattery_dprintk("[mtktsbattery_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind9))
	{
		table_val = 9;
		mtktsbattery_dprintk("[mtktsbattery_bind] %s\n", cdev->type);
	}
	else
	{	
		return 0;
	}
    
	if (mtk_thermal_zone_bind_cooling_device(thermal, table_val, cdev)) {
		mtktsbattery_dprintk("[mtktsbattery_bind] error binding cooling dev\n");
		return -EINVAL;
	} else {
		mtktsbattery_dprintk("[mtktsbattery_bind] binding OK, %d\n", table_val);
	}

	return 0;
}

static int mtktsbattery_unbind(struct thermal_zone_device *thermal,
			  struct thermal_cooling_device *cdev)
{
    int table_val=0;

	if(!strcmp(cdev->type, g_bind0))
	{
		table_val = 0;
		mtktsbattery_dprintk("[mtktsbattery_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind1))
	{
		table_val = 1;
		mtktsbattery_dprintk("[mtktsbattery_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind2))
	{
		table_val = 2;
		mtktsbattery_dprintk("[mtktsbattery_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind3))
	{
		table_val = 3;
		mtktsbattery_dprintk("[mtktsbattery_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind4))
	{
		table_val = 4;
		mtktsbattery_dprintk("[mtktsbattery_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind5))
	{
		table_val = 5;
		mtktsbattery_dprintk("[mtktsbattery_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind6))
	{
		table_val = 6;
		mtktsbattery_dprintk("[mtktsbattery_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind7))
	{
		table_val = 7;
		mtktsbattery_dprintk("[mtktsbattery_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind8))
	{
		table_val = 8;
		mtktsbattery_dprintk("[mtktsbattery_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind9))
	{
		table_val = 9;
		mtktsbattery_dprintk("[mtktsbattery_unbind] %s\n", cdev->type);
	}
	else
		return 0;
    
	if (thermal_zone_unbind_cooling_device(thermal, table_val, cdev)) {
		mtktsbattery_dprintk("[mtktsbattery_unbind] error unbinding cooling dev\n");
		return -EINVAL;
	} else {
		mtktsbattery_dprintk("[mtktsbattery_unbind] unbinding OK\n");
	}

	return 0;
}

static int mtktsbattery_get_mode(struct thermal_zone_device *thermal,
			    enum thermal_device_mode *mode)
{
	*mode = (kernelmode) ? THERMAL_DEVICE_ENABLED
			     : THERMAL_DEVICE_DISABLED;
	return 0;
}

static int mtktsbattery_set_mode(struct thermal_zone_device *thermal,
			    enum thermal_device_mode mode)
{
	kernelmode = mode;
	return 0;
}

static int mtktsbattery_get_trip_type(struct thermal_zone_device *thermal, int trip,
				 enum thermal_trip_type *type)
{
	*type = g_THERMAL_TRIP[trip];
	return 0;
}

static int mtktsbattery_get_trip_temp(struct thermal_zone_device *thermal, int trip,
				 unsigned long *temp)
{
	*temp = trip_temp[trip]; 
	return 0;
}

static int mtktsbattery_get_crit_temp(struct thermal_zone_device *thermal,
				 unsigned long *temperature)
{
	*temperature = mtktsbattery_TEMP_CRIT;
	return 0;
}

/* bind callback functions to thermalzone */
static struct thermal_zone_device_ops mtktsbattery_dev_ops = {
	.bind = mtktsbattery_bind,
	.unbind = mtktsbattery_unbind,
	.get_temp = mtktsbattery_get_temp,
	.get_mode = mtktsbattery_get_mode,
	.set_mode = mtktsbattery_set_mode,
	.get_trip_type = mtktsbattery_get_trip_type,
	.get_trip_temp = mtktsbattery_get_trip_temp,
	.get_crit_temp = mtktsbattery_get_crit_temp,
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
        mtktsbattery_dprintk("[dis_charge_set_cur_state] disable charging\n");
    }
    return 0;
}
*/

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
		printk("Power/battery_Thermal: reset, reset, reset!!!");
		printk("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
		printk("*****************************************");
		printk("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");

//<2013/5/2-24507-jessicatseng, [5860] Don't do powe-off if battery's temperature is over 60 degree in driver part
		//BUG();
//>2013/5/2-24507-jessicatseng
		//arch_reset(0,NULL);   
	} 
	return 0;
}

/*
static struct thermal_cooling_device_ops mtktsbattery_cooling_dis_charge_ops = {
	.get_max_state = dis_charge_get_max_state,
	.get_cur_state = dis_charge_get_cur_state,
	.set_cur_state = dis_charge_set_cur_state,
};*/
static struct thermal_cooling_device_ops mtktsbattery_cooling_sysrst_ops = {
	.get_max_state = sysrst_get_max_state,
	.get_cur_state = sysrst_get_cur_state,
	.set_cur_state = sysrst_set_cur_state,
};


static int mtktsbattery_read(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
	int len = 0;
	char *p = buf;
    
	p += sprintf(p, "[mtktsbattery_read] trip_0_temp=%d,trip_1_temp=%d,trip_2_temp=%d,trip_3_temp=%d,trip_4_temp=%d,\n\
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

int mtktsbattery_register_thermal(void);
void mtktsbattery_unregister_thermal(void);

static ssize_t mtktsbattery_write(struct file *file, const char *buffer, unsigned long count, void *data)
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
		mtktsbattery_dprintk("[mtktsbattery_write] mtktsbattery_unregister_thermal\n");
		mtktsbattery_unregister_thermal();
	
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

		mtktsbattery_dprintk("[mtktsbattery_write] g_THERMAL_TRIP_0=%d,g_THERMAL_TRIP_1=%d,g_THERMAL_TRIP_2=%d,g_THERMAL_TRIP_3=%d,g_THERMAL_TRIP_4=%d,\
g_THERMAL_TRIP_5=%d,g_THERMAL_TRIP_6=%d,g_THERMAL_TRIP_7=%d,g_THERMAL_TRIP_8=%d,g_THERMAL_TRIP_9=%d,\n",
				g_THERMAL_TRIP[0],g_THERMAL_TRIP[1],g_THERMAL_TRIP[2],g_THERMAL_TRIP[3],g_THERMAL_TRIP[4],
				g_THERMAL_TRIP[5],g_THERMAL_TRIP[6],g_THERMAL_TRIP[7],g_THERMAL_TRIP[8],g_THERMAL_TRIP[9]);
	mtktsbattery_dprintk("[mtktsbattery_write] cooldev0=%s,cooldev1=%s,cooldev2=%s,cooldev3=%s,cooldev4=%s,\
cooldev5=%s,cooldev6=%s,cooldev7=%s,cooldev8=%s,cooldev9=%s\n",
				g_bind0,g_bind1,g_bind2,g_bind3,g_bind4,g_bind5,g_bind6,g_bind7,g_bind8,g_bind9);

		for(i=0; i<num_trip; i++)
		{
			trip_temp[i]=trip[i];
		}

		interval=time_msec / 1000;

		mtktsbattery_dprintk("[mtktsbattery_write] trip_0_temp=%d,trip_1_temp=%d,trip_2_temp=%d,trip_3_temp=%d,trip_4_temp=%d,\
trip_5_temp=%d,trip_6_temp=%d,trip_7_temp=%d,trip_8_temp=%d,trip_9_temp=%d,time_ms=%d\n", 
				trip_temp[0],trip_temp[1],trip_temp[2],trip_temp[3],trip_temp[4],
				trip_temp[5],trip_temp[6],trip_temp[7],trip_temp[8],trip_temp[9],interval*1000);
													
		mtktsbattery_dprintk("[mtktsbattery_write] mtktsbattery_register_thermal\n");
		mtktsbattery_register_thermal();
				
		//battery_write_flag=1;
		return count;
	}
	else
	{
		mtktsbattery_dprintk("[mtktsbattery_write] bad argument\n");
	}
		
	return -EINVAL;
}


int  mtktsbattery_register_cooler(void)
{
	/* cooling devices */
	cl_dev_sysrst = mtk_thermal_cooling_device_register("mtktsbattery-sysrst", NULL,
		&mtktsbattery_cooling_sysrst_ops);
	return 0;
}

int mtktsbattery_register_thermal(void)
{
	mtktsbattery_dprintk("[mtktsbattery_register_thermal] \n");

	/* trips : trip 0~1 */
	thz_dev = mtk_thermal_zone_device_register("mtktsbattery", num_trip, NULL,
		&mtktsbattery_dev_ops, 0, 0, 0, interval*1000);

	return 0;
}

void mtktsbattery_unregister_cooler(void)
{
	if (cl_dev_sysrst) {
		mtk_thermal_cooling_device_unregister(cl_dev_sysrst);
		cl_dev_sysrst = NULL;
	}
}
void mtktsbattery_unregister_thermal(void)
{
	mtktsbattery_dprintk("[mtktsbattery_unregister_thermal] \n");

	if (thz_dev) {
		mtk_thermal_zone_device_unregister(thz_dev);
		thz_dev = NULL;
	}
}

static int __init mtktsbattery_init(void)
{
	int err = 0;
	struct proc_dir_entry *entry = NULL;
	struct proc_dir_entry *mtktsbattery_dir = NULL;

	mtktsbattery_dprintk("[mtktsbattery_init] \n");

	err = mtktsbattery_register_cooler();
	if(err)
		return err;
	
	err = mtktsbattery_register_thermal();
	if (err)
		goto err_unreg;

	mtktsbattery_dir = proc_mkdir("mtktsbattery", NULL);
	if (!mtktsbattery_dir)
	{
		mtktsbattery_dprintk("[mtktsbattery_init]: mkdir /proc/mtktsbattery failed\n");
	}
	else
	{
		entry = create_proc_entry("mtktsbattery", S_IRUGO | S_IWUSR, mtktsbattery_dir);
		if (entry)
		{
			entry->read_proc = mtktsbattery_read;
			entry->write_proc = mtktsbattery_write;
		}
	}

	return 0;

err_unreg:
	mtktsbattery_unregister_cooler();
	return err;
}

static void __exit mtktsbattery_exit(void)
{
	mtktsbattery_dprintk("[mtktsbattery_exit] \n");
	mtktsbattery_unregister_thermal();
	mtktsbattery_unregister_cooler();
}

module_init(mtktsbattery_init);
module_exit(mtktsbattery_exit);
