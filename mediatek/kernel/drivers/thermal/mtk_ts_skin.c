

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

#include <mach/system.h>
#include "mach/mtk_thermal_monitor.h"
//#include "mach/mt6577_typedefs.h"
//#include "mach/mt6577_thermal.h"

//#include <mach/upmu_common_sw.h>
//#include <mach/upmu_hw.h>

static unsigned int interval = 0; /* seconds, 0 : no auto polling */
static unsigned int trip_temp[10] = {120000,110000,100000,90000,80000,70000,65000,60000,55000,50000};

static unsigned int cl_dev_sysrst_state = 0;
static struct thermal_zone_device *thz_dev;

static struct thermal_cooling_device *cl_dev_sysrst;
static int mtktsskin_debug_log = 0;
static int kernelmode = 0;
static int mtktsskin_formula = 0;
static int mtktsskin_param[6][8] = {{0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0},
									{0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}};
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

#define mtktsskin_TEMP_CRIT 150000 /* 150.000 degree Celsius */

#define mtktsskin_dprintk(fmt, args...)   \
do {                                    \
    if (mtktsskin_debug_log) {                \
        xlog_printk(ANDROID_LOG_INFO, "Power/skin_Thermal", fmt, ##args); \
    }                                   \
} while(0)

int ts_skin_at_boot_time=0;
static int mtktsskin_get_hw_temp(void)
{
    kal_uint8 thermal_status=0;
    int t_ret=0;

    if(ts_skin_at_boot_time==0){            
        ts_skin_at_boot_time=1;
        mtktsskin_dprintk("[mtktsskin_get_hw_temp] at boot time, return 100002 as default\n");
        return 100002;
    }

    //get HW SKIN temp
    //Need formula to implement

    if(thermal_status == 0x0) {
        t_ret = 100002;
    } else if (thermal_status == 0x1) {
        t_ret = 115002;
    } else if (thermal_status == 0x3) {
        t_ret = 140002;    
    } else if (thermal_status == 0x7) {
        t_ret = 160002;
    } else {
        mtktsskin_dprintk("[mtktsskin_get_hw_temp] error register value (%d)\n", thermal_status);
    }        
    
    mtktsskin_dprintk("[mtktsskin_get_hw_temp] T_skin, %d, %d\n", t_ret, thermal_status);

    return t_ret;
}
    
static int mtktsskin_get_temp(struct thermal_zone_device *thermal,
			       unsigned long *t)
{
    *t = mtktsskin_get_hw_temp();
		return 0;
}

static int mtktsskin_bind(struct thermal_zone_device *thermal,
			struct thermal_cooling_device *cdev)
{
    int table_val=0;

		if(!strcmp(cdev->type, g_bind0))
		{
				table_val = 0;
				mtktsskin_dprintk("[mtktsskin_bind] %s\n", cdev->type);
		}
		else if(!strcmp(cdev->type, g_bind1))
		{
				table_val = 1;
				mtktsskin_dprintk("[mtktsskin_bind] %s\n", cdev->type);
		}
		else if(!strcmp(cdev->type, g_bind2))
		{
				table_val = 2;
				mtktsskin_dprintk("[mtktsskin_bind] %s\n", cdev->type);
		}
		else if(!strcmp(cdev->type, g_bind3))
		{
				table_val = 3;
				mtktsskin_dprintk("[mtktsskin_bind] %s\n", cdev->type);
		}
		else if(!strcmp(cdev->type, g_bind4))
		{
				table_val = 4;
				mtktsskin_dprintk("[mtktsskin_bind] %s\n", cdev->type);
		}
		else if(!strcmp(cdev->type, g_bind5))
		{
				table_val = 5;
				mtktsskin_dprintk("[mtktsskin_bind] %s\n", cdev->type);
		}
		else if(!strcmp(cdev->type, g_bind6))
		{
				table_val = 6;
				mtktsskin_dprintk("[mtktsskin_bind] %s\n", cdev->type);
		}
		else if(!strcmp(cdev->type, g_bind7))
		{
				table_val = 7;
				mtktsskin_dprintk("[mtktsskin_bind] %s\n", cdev->type);
		}
		else if(!strcmp(cdev->type, g_bind8))
		{
				table_val = 8;
				mtktsskin_dprintk("[mtktsskin_bind] %s\n", cdev->type);
		}
		else if(!strcmp(cdev->type, g_bind9))
		{
				table_val = 9;
				mtktsskin_dprintk("[mtktsskin_bind] %s\n", cdev->type);
		}
		else
		{	
				return 0;
		}
    
    if (mtk_thermal_zone_bind_cooling_device(thermal, table_val, cdev)) {
	    mtktsskin_dprintk("[mtktsskin_bind] error binding cooling dev\n");
		return -EINVAL;
	} else {
	    mtktsskin_dprintk("[mtktsskin_bind] binding OK, %d\n", table_val);
    }

	return 0;
}

static int mtktsskin_unbind(struct thermal_zone_device *thermal,
			  struct thermal_cooling_device *cdev)
{
    int table_val=0;

		if(!strcmp(cdev->type, g_bind0))
		{
				table_val = 0;
				mtktsskin_dprintk("[mtktsskin_unbind] %s\n", cdev->type);
		}
		else if(!strcmp(cdev->type, g_bind1))
		{
				table_val = 1;
				mtktsskin_dprintk("[mtktsskin_unbind] %s\n", cdev->type);
		}
		else if(!strcmp(cdev->type, g_bind2))
		{
				table_val = 2;
				mtktsskin_dprintk("[mtktsskin_unbind] %s\n", cdev->type);
		}
		else if(!strcmp(cdev->type, g_bind3))
		{
				table_val = 3;
				mtktsskin_dprintk("[mtktsskin_unbind] %s\n", cdev->type);
		}
		else if(!strcmp(cdev->type, g_bind4))
		{
				table_val = 4;
				mtktsskin_dprintk("[mtktsskin_unbind] %s\n", cdev->type);
		}
		else if(!strcmp(cdev->type, g_bind5))
		{
				table_val = 5;
				mtktsskin_dprintk("[mtktsskin_unbind] %s\n", cdev->type);
		}
		else if(!strcmp(cdev->type, g_bind6))
		{
				table_val = 6;
				mtktsskin_dprintk("[mtktsskin_unbind] %s\n", cdev->type);
		}
		else if(!strcmp(cdev->type, g_bind7))
		{
				table_val = 7;
				mtktsskin_dprintk("[mtktsskin_unbind] %s\n", cdev->type);
		}
		else if(!strcmp(cdev->type, g_bind8))
		{
				table_val = 8;
				mtktsskin_dprintk("[mtktsskin_unbind] %s\n", cdev->type);
		}
		else if(!strcmp(cdev->type, g_bind9))
		{
				table_val = 9;
				mtktsskin_dprintk("[mtktsskin_unbind] %s\n", cdev->type);
		}
		else
				return 0;
    
    if (thermal_zone_unbind_cooling_device(thermal, table_val, cdev)) {
	    mtktsskin_dprintk("[mtktsskin_unbind] error unbinding cooling dev\n");
		return -EINVAL;
	} else {
	    mtktsskin_dprintk("[mtktsskin_unbind] unbinding OK\n");
    }

	return 0;
}

static int mtktsskin_get_mode(struct thermal_zone_device *thermal,
			    enum thermal_device_mode *mode)
{
    *mode = (kernelmode) ? THERMAL_DEVICE_ENABLED
			     : THERMAL_DEVICE_DISABLED;
		return 0;
}

static int mtktsskin_set_mode(struct thermal_zone_device *thermal,
			    enum thermal_device_mode mode)
{
    kernelmode = mode;
		return 0;
}

static int mtktsskin_get_trip_type(struct thermal_zone_device *thermal, int trip,
				 enum thermal_trip_type *type)
{
		*type = g_THERMAL_TRIP[trip];
		return 0;
}

static int mtktsskin_get_trip_temp(struct thermal_zone_device *thermal, int trip,
				 unsigned long *temp)
{
		*temp = trip_temp[trip]; 
		return 0;
}

static int mtktsskin_get_crit_temp(struct thermal_zone_device *thermal,
				 unsigned long *temperature)
{
		*temperature = mtktsskin_TEMP_CRIT;
		return 0;
}

/* bind callback functions to thermalzone */
static struct thermal_zone_device_ops mtktsskin_dev_ops = {
	.bind = mtktsskin_bind,
	.unbind = mtktsskin_unbind,
	.get_temp = mtktsskin_get_temp,
	.get_mode = mtktsskin_get_mode,
	.set_mode = mtktsskin_set_mode,
	.get_trip_type = mtktsskin_get_trip_type,
	.get_trip_temp = mtktsskin_get_trip_temp,
	.get_crit_temp = mtktsskin_get_crit_temp,
};

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
				printk("Power/skin_Thermal: reset, reset, reset!!!");
				printk("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
				printk("*****************************************");
				printk("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");

				BUG();
				//arch_reset(0,NULL);   
		}    
		return 0;
}

static struct thermal_cooling_device_ops mtktsskin_cooling_sysrst_ops = {
	.get_max_state = sysrst_get_max_state,
	.get_cur_state = sysrst_get_cur_state,
	.set_cur_state = sysrst_set_cur_state,
};


static int mtktsskin_read(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    char *p = buf;
    
		p += sprintf(p, "[ mtktsskin_read] trip_0_temp=%d,trip_1_temp=%d,trip_2_temp=%d,trip_3_temp=%d,trip_4_temp=%d,\n\
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

int mtktsskin_register_thermal(void);
void mtktsskin_unregister_thermal(void);

static ssize_t mtktsskin_write(struct file *file, const char *buffer, unsigned long count, void *data)
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
				mtktsskin_dprintk("[mtktsskin_write] mtktsskin_unregister_thermal\n");
				mtktsskin_unregister_thermal();
	
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

				mtktsskin_dprintk("[mtktsskin_write] g_THERMAL_TRIP_0=%d,g_THERMAL_TRIP_1=%d,g_THERMAL_TRIP_2=%d,g_THERMAL_TRIP_3=%d,g_THERMAL_TRIP_4=%d,\
g_THERMAL_TRIP_5=%d,g_THERMAL_TRIP_6=%d,g_THERMAL_TRIP_7=%d,g_THERMAL_TRIP_8=%d,g_THERMAL_TRIP_9=%d,\n",
													g_THERMAL_TRIP[0],g_THERMAL_TRIP[1],g_THERMAL_TRIP[2],g_THERMAL_TRIP[3],g_THERMAL_TRIP[4],
													g_THERMAL_TRIP[5],g_THERMAL_TRIP[6],g_THERMAL_TRIP[7],g_THERMAL_TRIP[8],g_THERMAL_TRIP[9]);
				mtktsskin_dprintk("[mtktsskin_write] cooldev0=%s,cooldev1=%s,cooldev2=%s,cooldev3=%s,cooldev4=%s,\
cooldev5=%s,cooldev6=%s,cooldev7=%s,cooldev8=%s,cooldev9=%s\n",
													g_bind0,g_bind1,g_bind2,g_bind3,g_bind4,g_bind5,g_bind6,g_bind7,g_bind8,g_bind9);

				for(i=0; i<num_trip; i++)
				{
						trip_temp[i]=trip[i];
				}
				
				interval=time_msec / 1000;

				mtktsskin_dprintk("[mtktsskin_write] trip_0_temp=%d,trip_1_temp=%d,trip_2_temp=%d,trip_3_temp=%d,trip_4_temp=%d,\
trip_5_temp=%d,trip_6_temp=%d,trip_7_temp=%d,trip_8_temp=%d,trip_9_temp=%d,time_ms=%d\n", 
						trip_temp[0],trip_temp[1],trip_temp[2],trip_temp[3],trip_temp[4],
						trip_temp[5],trip_temp[6],trip_temp[7],trip_temp[8],trip_temp[9],interval*1000);
													
				mtktsskin_dprintk("[mtktsskin_write] mtktsskin_register_thermal\n");
				mtktsskin_register_thermal();

				return count;
		}
		else
		{
				mtktsskin_dprintk("[mtktsskin_write] bad argument\n");
		}
		
		return -EINVAL;
}

static int mtktsskin_read_param(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
	int len = 0;
	char *p = buf;

	p += sprintf(p, "[ mtktsskin_read_param] formula = %d\n", mtktsskin_formula);

	*start = buf + off;

	len = p - buf;
	if (len > off)
		len -= off;
	else
		len = 0;

	return len < count ? len  : count;
}

static int mtktsskin_write_param(struct file *file, const char *buffer, unsigned long count, void *data)
{
	char	desc[32];
	int 	len = 0;
			
	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
	{
			return 0;
	}
	desc[len] = '\0';

	if (sscanf(desc, "%d", &mtktsskin_formula) == 1)
	{
		mtktsskin_dprintk("[mtktsskin_write_param] mtktsskin_formula=%d\n", mtktsskin_formula);
		return count;
	}
	else
	{
		mtktsskin_dprintk("[mtktsskin_write_param] bad argument\n");
	}
	return -EINVAL;
}

static int mtktsskin_read_abb(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
	int len = 0;
	char *p = buf;
	
	p += sprintf(p, "[ mtktsskin_read_abb] param = %d %d %d %d %d %d %d %d\n", mtktsskin_param[0][0],
		mtktsskin_param[0][1], mtktsskin_param[0][2], mtktsskin_param[0][3], mtktsskin_param[0][4]
		, mtktsskin_param[0][5], mtktsskin_param[0][6], mtktsskin_param[0][7]);
	
	*start = buf + off;

	len = p - buf;
	if (len > off)
		len -= off;
	else
		len = 0;

	return len < count ? len  : count;

}

static int mtktsskin_write_abb(struct file *file, const char *buffer, unsigned long count, void *data)
{
	char	desc[64];
	int 	len = 0;
				
	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
	{
			return 0;
	}
	desc[len] = '\0';

	if (sscanf(desc, "%d %d %d %d %d %d %d %d", &mtktsskin_param[0][0], &mtktsskin_param[0][1]
		, &mtktsskin_param[0][2], &mtktsskin_param[0][3], &mtktsskin_param[0][4], &mtktsskin_param[0][5]
		, &mtktsskin_param[0][6], &mtktsskin_param[0][7]) == 8)
	{
		mtktsskin_dprintk("[mtktsskin_write_abb] param=%d %d %d %d %d %d %d %d\n", mtktsskin_param[0][0],
			mtktsskin_param[0][1], mtktsskin_param[0][2], mtktsskin_param[0][3], mtktsskin_param[0][4],
			mtktsskin_param[0][5], mtktsskin_param[0][6], mtktsskin_param[0][7]);
		return count;
	}
	else
	{
		mtktsskin_dprintk("[mtktsskin_write_abb] bad argument\n");
	}
	return -EINVAL;

}

static int mtktsskin_read_battery(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
	int len = 0;
	char *p = buf;
	
	p += sprintf(p, "[ mtktsskin_read_battery] param = %d %d %d %d %d %d %d %d\n", mtktsskin_param[1][0],
		mtktsskin_param[1][1], mtktsskin_param[1][2], mtktsskin_param[1][3], mtktsskin_param[1][4]
		, mtktsskin_param[1][5], mtktsskin_param[1][6], mtktsskin_param[1][7]);

	*start = buf + off;

	len = p - buf;
	if (len > off)
		len -= off;
	else
		len = 0;

	return len < count ? len  : count;

}

static int mtktsskin_write_battery(struct file *file, const char *buffer, unsigned long count, void *data)
{
	char	desc[64];
	int 	len = 0;
		
	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
	{
		return 0;
	}
	desc[len] = '\0';

	if (sscanf(desc, "%d %d %d %d %d %d %d %d", &mtktsskin_param[1][0], &mtktsskin_param[1][1]
		, &mtktsskin_param[1][2], &mtktsskin_param[1][3], &mtktsskin_param[1][4], &mtktsskin_param[1][5]
		, &mtktsskin_param[1][6], &mtktsskin_param[1][7]) == 8)
	{
		mtktsskin_dprintk("[mtktsskin_write_battery] param=%d %d %d %d %d %d %d %d\n", mtktsskin_param[1][0],
			mtktsskin_param[1][1], mtktsskin_param[1][2], mtktsskin_param[1][3], mtktsskin_param[1][4],
			mtktsskin_param[1][5], mtktsskin_param[1][6], mtktsskin_param[1][7]);
		return count;
	}
	else
	{
		mtktsskin_dprintk("[mtktsskin_write_battery] bad argument\n");
	}
	return -EINVAL;

}

static int mtktsskin_read_cpu(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
	int len = 0;
	char *p = buf;
	
	p += sprintf(p, "[ mtktsskin_read_cpu] param = %d %d %d %d %d %d %d %d\n", mtktsskin_param[2][0],
		mtktsskin_param[2][1], mtktsskin_param[2][2], mtktsskin_param[2][3], mtktsskin_param[2][4]
		, mtktsskin_param[2][5], mtktsskin_param[2][6], mtktsskin_param[2][7]);

	*start = buf + off;

	len = p - buf;
	if (len > off)
		len -= off;
	else
		len = 0;

	return len < count ? len  : count;

}

static int mtktsskin_write_cpu(struct file *file, const char *buffer, unsigned long count, void *data)
{
	char	desc[64];
	int 	len = 0;
	
	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
	{
		return 0;
	}
	desc[len] = '\0';

	if (sscanf(desc, "%d %d %d %d %d %d %d %d", &mtktsskin_param[2][0], &mtktsskin_param[2][1]
		, &mtktsskin_param[2][2], &mtktsskin_param[2][3], &mtktsskin_param[2][4], &mtktsskin_param[2][5]
		, &mtktsskin_param[2][6], &mtktsskin_param[2][7]) == 8)
	{
		mtktsskin_dprintk("[mtktsskin_write_cpu] param=%d %d %d %d %d %d %d %d\n", mtktsskin_param[2][0],
			mtktsskin_param[2][1], mtktsskin_param[2][2], mtktsskin_param[2][3], mtktsskin_param[2][4],
			mtktsskin_param[2][5], mtktsskin_param[2][6], mtktsskin_param[2][7]);
		return count;
	}
	else
	{
		mtktsskin_dprintk("[mtktsskin_write_cpu] bad argument\n");
	}
	return -EINVAL;

}

static int mtktsskin_read_cpu_up(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
	int len = 0;
	char *p = buf;
	
	p += sprintf(p, "[ mtktsskin_read_cpu_up] param = %d %d %d %d %d %d %d %d\n", mtktsskin_param[3][0],
		mtktsskin_param[3][1], mtktsskin_param[3][2], mtktsskin_param[3][3], mtktsskin_param[3][4]
		, mtktsskin_param[3][5], mtktsskin_param[3][6], mtktsskin_param[3][7]);

	*start = buf + off;

	len = p - buf;
	if (len > off)
		len -= off;
	else
		len = 0;

	return len < count ? len  : count;

}

static int mtktsskin_write_cpu_up(struct file *file, const char *buffer, unsigned long count, void *data)
{
	char	desc[64];
	int 	len = 0;
	
	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
	{
		return 0;
	}
	desc[len] = '\0';

	if (sscanf(desc, "%d %d %d %d %d %d %d %d", &mtktsskin_param[3][0], &mtktsskin_param[3][1]
		, &mtktsskin_param[3][2], &mtktsskin_param[3][3], &mtktsskin_param[3][4], &mtktsskin_param[3][5]
		, &mtktsskin_param[3][6], &mtktsskin_param[3][7]) == 8)
	{
		mtktsskin_dprintk("[mtktsskin_write_cpu_up] param=%d %d %d %d %d %d %d %d\n", mtktsskin_param[3][0],
			mtktsskin_param[3][1], mtktsskin_param[3][2], mtktsskin_param[3][3], mtktsskin_param[3][4],
			mtktsskin_param[3][5], mtktsskin_param[3][6], mtktsskin_param[3][7]);
		return count;
	}
	else
	{
		mtktsskin_dprintk("[mtktsskin_write_cpu_up] bad argument\n");
	}
	return -EINVAL;

}

static int mtktsskin_read_pa(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
	int len = 0;
	char *p = buf;
	
	p += sprintf(p, "[ mtktsskin_read_pa] param = %d %d %d %d %d %d %d %d\n", mtktsskin_param[4][0],
		mtktsskin_param[4][1], mtktsskin_param[4][2], mtktsskin_param[4][3], mtktsskin_param[4][4]
		, mtktsskin_param[4][5], mtktsskin_param[4][6], mtktsskin_param[4][7]);

	*start = buf + off;

	len = p - buf;
	if (len > off)
		len -= off;
	else
		len = 0;

	return len < count ? len  : count;

}

static int mtktsskin_write_pa(struct file *file, const char *buffer, unsigned long count, void *data)
{
	char	desc[64];
	int 	len = 0;
	
	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
	{
		return 0;
	}
	desc[len] = '\0';

	if (sscanf(desc, "%d %d %d %d %d %d %d %d", &mtktsskin_param[4][0], &mtktsskin_param[4][1]
		, &mtktsskin_param[4][2], &mtktsskin_param[4][3], &mtktsskin_param[4][4], &mtktsskin_param[4][5]
		, &mtktsskin_param[4][6], &mtktsskin_param[4][7]) == 8)
	{
		mtktsskin_dprintk("[mtktsskin_write_pa] param=%d %d %d %d %d %d %d %d\n", mtktsskin_param[4][0],
			mtktsskin_param[4][1], mtktsskin_param[4][2], mtktsskin_param[4][3], mtktsskin_param[4][4],
			mtktsskin_param[4][5], mtktsskin_param[4][6], mtktsskin_param[4][7]);
		return count;
	}
	else
	{
		mtktsskin_dprintk("[mtktsskin_write_pa] bad argument\n");
	}
	return -EINVAL;

}

static int mtktsskin_read_pmic(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
	int len = 0;
	char *p = buf;
	
	p += sprintf(p, "[ mtktsskin_read_pmic] param = %d %d %d %d %d %d %d %d\n", mtktsskin_param[5][0],
		mtktsskin_param[5][1], mtktsskin_param[5][2], mtktsskin_param[5][3], mtktsskin_param[5][4]
		, mtktsskin_param[5][5], mtktsskin_param[5][6], mtktsskin_param[5][7]);

	*start = buf + off;

	len = p - buf;
	if (len > off)
		len -= off;
	else
		len = 0;

	return len < count ? len  : count;

}

static int mtktsskin_write_pmic(struct file *file, const char *buffer, unsigned long count, void *data)
{
	char	desc[64];
	int 	len = 0;
	
	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
	{
		return 0;
	}
	desc[len] = '\0';

	if (sscanf(desc, "%d %d %d %d %d %d %d %d", &mtktsskin_param[5][0], &mtktsskin_param[5][1]
		, &mtktsskin_param[5][2], &mtktsskin_param[5][3], &mtktsskin_param[5][4], &mtktsskin_param[5][5]
		, &mtktsskin_param[5][6], &mtktsskin_param[5][7]) == 8)
	{
		mtktsskin_dprintk("[mtktsskin_write_pmic] param=%d %d %d %d %d %d %d %d\n", mtktsskin_param[5][0],
			mtktsskin_param[5][1], mtktsskin_param[5][2], mtktsskin_param[5][3], mtktsskin_param[5][4],
			mtktsskin_param[5][5], mtktsskin_param[5][6], mtktsskin_param[5][7]);
		return count;
	}
	else
	{
		mtktsskin_dprintk("[mtktsskin_write_pmic] bad argument\n");
	}
	return -EINVAL;

}

int mtktsskin_register_cooler(void)
{
		cl_dev_sysrst = mtk_thermal_cooling_device_register("mtktsskin-sysrst", NULL,
    	               &mtktsskin_cooling_sysrst_ops);		
   	return 0;
}

int mtktsskin_register_thermal(void)
{
    mtktsskin_dprintk("[mtktsskin_register_thermal] \n");

    /* trips : trip 0~2 */
    thz_dev = mtk_thermal_zone_device_register("mtktsskin", num_trip, NULL,
                      &mtktsskin_dev_ops, 0, 0, 0, interval*1000);

    return 0;
}

void mtktsskin_unregister_cooler(void)
{
		if (cl_dev_sysrst) {
        mtk_thermal_cooling_device_unregister(cl_dev_sysrst);
        cl_dev_sysrst = NULL;
    }
}

void mtktsskin_unregister_thermal(void)
{
    mtktsskin_dprintk("[mtktsskin_unregister_thermal] \n");
    
	  if (thz_dev) {
        mtk_thermal_zone_device_unregister(thz_dev);
        thz_dev = NULL;
    }
}

static int __init mtktsskin_init(void)
{
		int err = 0;
    struct proc_dir_entry *entry = NULL;
    struct proc_dir_entry *mtktsskin_dir = NULL;

    mtktsskin_dprintk("[mtktsskin_init] \n");
		
		err = mtktsskin_register_cooler();
		if(err)
			return err;
		err = mtktsskin_register_thermal();
		if (err)
			goto err_unreg;

    mtktsskin_dir = proc_mkdir("mtktsskin", NULL);
    if (!mtktsskin_dir)
    {
        mtktsskin_dprintk("[mtktsskin_init]: mkdir /proc/mtktsskin failed\n");
    }
    else
    {
        entry = create_proc_entry("mtktsskin", S_IRUGO | S_IWUSR, mtktsskin_dir);
        if (entry)
        {
            entry->read_proc = mtktsskin_read;
            entry->write_proc = mtktsskin_write;
        }

		entry = create_proc_entry("mtktsskin_param", S_IRUGO | S_IWUSR, mtktsskin_dir);
        if (entry)
        {
            entry->read_proc = mtktsskin_read_param;
            entry->write_proc = mtktsskin_write_param;
        }

		entry = create_proc_entry("mtktsskin_abb", S_IRUGO | S_IWUSR, mtktsskin_dir);
        if (entry)
        {
            entry->read_proc = mtktsskin_read_abb;
            entry->write_proc = mtktsskin_write_abb;
        }

		entry = create_proc_entry("mtktsskin_battery", S_IRUGO | S_IWUSR, mtktsskin_dir);
        if (entry)
        {
            entry->read_proc = mtktsskin_read_battery;
            entry->write_proc = mtktsskin_write_battery;
        }

		entry = create_proc_entry("mtktsskin_cpu", S_IRUGO | S_IWUSR, mtktsskin_dir);
        if (entry)
        {
            entry->read_proc = mtktsskin_read_cpu;
            entry->write_proc = mtktsskin_write_cpu;
        }

		entry = create_proc_entry("mtktsskin_cpu_up", S_IRUGO | S_IWUSR, mtktsskin_dir);
        if (entry)
        {
            entry->read_proc = mtktsskin_read_cpu_up;
            entry->write_proc = mtktsskin_write_cpu_up;
        }

		entry = create_proc_entry("mtktsskin_pa", S_IRUGO | S_IWUSR, mtktsskin_dir);
        if (entry)
        {
            entry->read_proc = mtktsskin_read_pa;
            entry->write_proc = mtktsskin_write_pa;
        }

		entry = create_proc_entry("mtktsskin_pmic", S_IRUGO | S_IWUSR, mtktsskin_dir);
        if (entry)
        {
            entry->read_proc = mtktsskin_read_pmic;
            entry->write_proc = mtktsskin_write_pmic;
        }
    }

		return 0;

err_unreg:
		mtktsskin_unregister_cooler();
		return err;
}

static void __exit mtktsskin_exit(void)
{
    mtktsskin_dprintk("[mtktsskin_exit] \n");
		mtktsskin_unregister_thermal();
		mtktsskin_unregister_cooler();
}

module_init(mtktsskin_init);
module_exit(mtktsskin_exit);

