

#include <asm/uaccess.h>
#include <asm/system.h>

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
#include <linux/err.h>
#include <linux/syscalls.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/bug.h>
#include <linux/workqueue.h>

#include <mach/mtk_thermal_monitor.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_storage_logger.h>
#include <mach/mtk_mdm_monitor.h>

//************************************
// Definition
//************************************

/* Number of CPU CORE */
#if defined(CONFIG_ARCH_MT6589) || defined(CONFIG_ARCH_MT6582)  
#define NUMBER_OF_CORE (4)
#else
#if defined(CONFIG_ARCH_MT6577)
#define NUMBER_OF_CORE (2)
#else
/* CONFIG_ARCH_MT6575 */
#define NUMBER_OF_CORE (1)
#endif 
#endif 

#if defined(CONFIG_ARCH_MT6589)
//<2013/3/4-22419-jessicatseng, [Pelican] Some bq24157 related files are not integrated into JB2
#if defined(MTK_FAN5405_SUPPORT) || defined(MTK_NCP1851_SUPPORT) || defined(MTK_BQ24196_SUPPORT) || defined(MTK_BQ24157_SUPPORT) // supported charger IC
#define CONFIG_MTK_THERMAL_TIME_BASE_PROTECTION
#endif
//>2013/3/4-22419-jessicatseng
#endif

#if defined(CONFIG_MTK_SMART_BATTERY)
// global variable from battery driver...
extern kal_bool gFG_Is_Charging;
#endif

#if defined(CONFIG_ARCH_MT6589)
// get MT6589 GPU loading...
extern unsigned int mt_gpufreq_cur_load(void); 
extern unsigned int mt_gpufreq_cur_freq(void);
#endif

/**
 * \def MTK_THERMAL_MONITOR_MEASURE_GET_TEMP_OVERHEAD
 * 1 to enable 
 * 0 to disable
 */
#define MTK_THERMAL_MONITOR_MEASURE_GET_TEMP_OVERHEAD (1)

#define MTK_THERMAL_MONITOR_COOLER_MAX_EXTRA_CONDITIONS (3)

#define MTK_THERMAL_MONITOR_CONDITIONAL_COOLING (1)

/**
 *  \def MTK_MAX_STEP_SMA_LEN
 *  If not defined as 1, multi-step temperature SMA len is supported.
 *  For example, MTK_MAX_STEP_SMA_LEN is defined as 4. 
 *  Users can set 4 different SMA len for a thermal zone and assign a high threshold for each.
 *  SMA len in the next step is applied if temp of the TZ reaches high threshold.
 *  Represent this in a simple figure as below: 
 *      -infinite HT(0)|<- sma_len(0) ->|HT(1)|<- sma_len(1) ->|HT(2)|<- sma_len(2) ->|HT(3)|<- sma_len(3) -> |+infinite HT(4)
 *  In temp range between HT(i) and HT(i+1), sma_len(i) is applied.
 *  HT(i) < HT(i+1), eq is not allowed since meaningless
 *  sma_len(i) in [1, 60]
 */
#define MAX_STEP_MA_LEN (4)

#define MSMA_MAX_HT     (1000000)
#define MSMA_MIN_HT     (-275000)

struct mtk_thermal_cooler_data
{
    struct thermal_zone_device *tz;
    struct thermal_cooling_device_ops *ops;
    void *devdata;
    int trip;
    char conditions[MTK_THERMAL_MONITOR_COOLER_MAX_EXTRA_CONDITIONS][THERMAL_NAME_LENGTH];
    int *condition_last_value[MTK_THERMAL_MONITOR_COOLER_MAX_EXTRA_CONDITIONS];
    int threshold[MTK_THERMAL_MONITOR_COOLER_MAX_EXTRA_CONDITIONS];
    int exit_threshold;
    int id;
};

struct mtk_thermal_tz_data
{
    struct thermal_zone_device_ops* ops;
    unsigned int ma_len; // max 60
    unsigned int ma_counter;
    long ma[60];
#if (MAX_STEP_MA_LEN > 1)
    unsigned int curr_idx_ma_len;
    unsigned int ma_lens[MAX_STEP_MA_LEN];
    long msma_ht[MAX_STEP_MA_LEN]; /**< multi-step moving avg. high threshold array. */
#endif
    long fake_temp; // to store the Tfake, range from -275000 to MAX positive of int...-275000 is a special number to turn off Tfake
    struct mutex ma_lock; // protect moving avg. vars...
};

static DEFINE_MUTEX(MTM_GET_TEMP_LOCK);
static int* tz_last_values[MTK_THERMAL_SENSOR_COUNT] = {NULL};

//************************************
// Global Variable
//************************************
static int  g_SysinfoAttachOps;
static bool enable_ThermalMonitor = false;
static bool enable_ThermalMonitorXlog = false;
static int  g_nStartRealTime;
static struct proc_dir_entry *proc_cooler_dir_entry = NULL; // lock by MTM_COOLER_PROC_DIR_LOCK
static struct proc_dir_entry *proc_tz_dir_entry = NULL; // lock by MTK_TZ_PROC_DIR_LOCK
/**
 *  write to nBattCurrentCnsmpt, nCPU0_usage, and nCPU1_usage are locked by MTM_SYSINFO_LOCK
 */
static int nBattCurrentCnsmpt = 0;
static int nCPU_loading_sum = 0;
static unsigned long g_check_sys_info_flag = 0x0; // TODO: change to use atomic bit field // write locked by MTM_COOLER_LOCK

static int nWifi_throughput = 0;
//static int nModem_TxPower = -127;  ///< Indicate invalid value

// For enabling time based thermal protection under phone call+AP suspend scenario.
static int g_mtm_phone_call_ongoing = 0;

static DEFINE_MUTEX(MTM_COOLER_LOCK);
static DEFINE_MUTEX(MTM_SYSINFO_LOCK);
static DEFINE_MUTEX(MTM_COOLER_PROC_DIR_LOCK);
static DEFINE_MUTEX(MTM_TZ_PROC_DIR_LOCK);

static struct delayed_work _mtm_sysinfo_poll_queue;

//************************************
//  Macro
//************************************
#define THRML_STORAGE_LOG(msg_id, func_name, ...) \
    do { \
        if(unlikely(is_dump_mthermal()) && enable_ThermalMonitor) { \
            AddThrmlTrace(msg_id, func_name, __VA_ARGS__); \
        } \
    }while(0)


#define THRML_LOG(fmt, args...) \
    do { \
        if (enable_ThermalMonitorXlog) { \
            xlog_printk(ANDROID_LOG_INFO, "THERMAL/MONITOR", fmt, ##args); \
        } \
    } while(0)


#define THRML_ERROR_LOG(fmt, args...) \
    do { \
        xlog_printk(ANDROID_LOG_INFO, "THERMAL/MONITOR", fmt, ##args); \
    } while(0)


//************************************
//  Define
//************************************
//thermal_zone_device * sysinfo_monitor_register(int nPollingTime);
//int sysinfo_monitor_unregister(void);
#define SYSINFO_ATTACH_DEV_NAME "mtktscpu"

//************************************
// Thermal Monitor API
//************************************
#if defined(CONFIG_MTK_THERMAL_TIME_BASE_PROTECTION)
#include <mach/mt_gpt.h>
#include <mach/mt_sleep.h>
#include <linux/wakelock.h>

extern int force_get_tbat(void);

static struct wake_lock mtm_wake_lock;
static unsigned int gpt_remaining_cnt = 0;
static int last_batt_raw_temp = 0;

static int mtk_thermal_monitor_get_battery_timeout_time(void)
{
    if (NULL != tz_last_values[MTK_THERMAL_SENSOR_BATTERY])
    {
        int batt_temp = last_batt_raw_temp; //*tz_last_values[MTK_THERMAL_SENSOR_BATTERY];

        if (batt_temp <= 25000)
        {
            return 330; // max 330
        }
        else if (batt_temp <= 35000 && batt_temp > 25000)
        {
            return 300;
        }
        else if (batt_temp <= 45000 && batt_temp > 35000)
            return 150; // 2.5 min
        else if (batt_temp <= 50000 && batt_temp > 45000)
            return 60;  // 1 min
        else
            return 30;  // 0.5 min
    }
    else
    {
        return -1; // no battery temperature, what to protect?
    }
}

static int mtk_thermal_monitor_suspend(struct platform_device *dev, pm_message_t state)
{
    // check if phone call on going...
    if (g_mtm_phone_call_ongoing)
    {
        // if yes, based on battery temperature to setup a GPT timer
        int timeout = mtk_thermal_monitor_get_battery_timeout_time();
        if (timeout > 0)
        {
            // restart a one-shot GPT timer // max 5.5 min
            if (gpt_remaining_cnt > 0 && gpt_remaining_cnt <= (timeout * 13000000))
                gpt_set_cmp(GPT5, gpt_remaining_cnt);
            else
                gpt_set_cmp(GPT5, timeout * 13000000); //compare unit is (1/13M) s 
            
            start_gpt(GPT5);
            
            THRML_ERROR_LOG("[mtk_thermal_monitor_suspend] timeout: %d, gpt_remaining_cnt: %u\n", timeout, gpt_remaining_cnt);
        }

        // make GPT able to wake up AP
        slp_set_wakesrc(WAKE_SRC_CFG_KEY | WAKE_SRC_GPT, true, false);
    }
    else
    {
        THRML_LOG("[mtk_thermal_monitor_suspend] disable GPT wakes AP.\n");
        // make GPT unable to wake up AP
        slp_set_wakesrc(WAKE_SRC_CFG_KEY | WAKE_SRC_GPT, false, false);
    }
    
    return 0;
}

static int mtk_thermal_monitor_resume(struct platform_device *dev)
{
    // take wake lock
    if (NULL != tz_last_values[MTK_THERMAL_SENSOR_BATTERY])
    {
        // check if phone call on going...if yes, we need to confirm battery temp. if not, we don't need this.
        if (g_mtm_phone_call_ongoing)
        {
            unsigned int GPT5_cmp;
            unsigned int GPT5_cnt;
            int gpt_counting;
            
            gpt_counting = gpt_is_counting(GPT5);
            gpt_get_cmp(GPT5, &GPT5_cmp);
            gpt_get_cnt(GPT5, &GPT5_cnt);
            gpt_remaining_cnt = GPT5_cmp - GPT5_cnt;
            
            // If no wake lock taken and gpt does timeout!
            if (!wake_lock_active(&mtm_wake_lock) && !gpt_counting)
            {
                THRML_ERROR_LOG("[mtk_thermal_monitor_resume] wake_lock() counting=%d, cmp=%u, cnt=%u", gpt_counting, GPT5_cmp, GPT5_cnt);
                wake_lock(&mtm_wake_lock);
            }
        }
    }
    
    // cancel my own GPT timer, ok to do it w/o pairing
    stop_gpt(GPT5);

    // release wake lock until no problem...
    
    return 0;
}

static struct platform_driver mtk_thermal_monitor_driver = 
{
    .remove     = NULL,
    .shutdown   = NULL,
    .probe      = NULL,
    .suspend    = mtk_thermal_monitor_suspend,
    .resume     = mtk_thermal_monitor_resume,
    .driver     = 
        {
            .name = "mtk-therm-mon",
        },
};
#endif

static long int _get_current_time_us(void)
{
    struct timeval t;
    do_gettimeofday(&t);
    return (t.tv_sec & 0xFFF) * 1000000 + t.tv_usec;
}

static struct proc_dir_entry * _get_proc_cooler_dir_entry(void)
{
    mutex_lock(&MTM_COOLER_PROC_DIR_LOCK);
    if (NULL == proc_cooler_dir_entry)
    {
        proc_cooler_dir_entry = proc_mkdir("mtkcooler", NULL);
        mb();
        if (NULL == proc_cooler_dir_entry)
        {
            THRML_ERROR_LOG("[mtkthermal_init]: mkdir /proc/mtkcooler failed\n");
        }
    }
    else
    {
    }
    mutex_unlock(&MTM_COOLER_PROC_DIR_LOCK);
    return proc_cooler_dir_entry;
}

static struct proc_dir_entry * _get_proc_tz_dir_entry(void)
{
    mutex_lock(&MTM_TZ_PROC_DIR_LOCK);
    if (NULL == proc_tz_dir_entry)
    {
        proc_tz_dir_entry = proc_mkdir("mtktz", NULL);
        mb();
        if (NULL == proc_tz_dir_entry)
        {
            THRML_ERROR_LOG("[mtkthermal_init]: mkdir /proc/mtktz failed\n");
        }
    }
    else
    {
    }
    mutex_unlock(&MTM_TZ_PROC_DIR_LOCK);
    return proc_tz_dir_entry;
}

static struct thermal_cooling_device_ops* recoveryClientCooler
(
    struct thermal_cooling_device *cdev,
    struct mtk_thermal_cooler_data **mcdata
)
{
    *mcdata = cdev->devdata;
    cdev->devdata = (*mcdata)->devdata;

    return (*mcdata)->ops;
}


/* Lookup List to get Client's Thermal Zone OPS */
static struct thermal_zone_device_ops* getClientZoneOps
(
    struct thermal_zone_device *zdev
)
{
    if ((NULL == zdev) || (NULL == zdev->devdata))
    {
        BUG();
        return NULL;
    }
    else
    {
        struct thermal_zone_device_ops* ret = NULL;
        struct mtk_thermal_tz_data* tzdata = zdev->devdata;
        mutex_lock(&tzdata->ma_lock);
        ret = tzdata->ops;
        mutex_unlock(&tzdata->ma_lock);
        return ret;
    }
}

//*********************************************
// System Information Monitor
//*********************************************
static mm_segment_t oldfs;

/*
 *  Read Battery Information.
 *
 *  "cat /sys/devices/platform/mt6575-battery/FG_Battery_CurrentConsumption"
 *  "cat /sys/class/power_supply/battery/batt_vol"
 *  "cat /sys/class/power_supply/battery/batt_temp"
 */
static int get_sys_battery_info(char* dev)
{
    int fd;
    int nRet;
    int nReadSize;
    char *pvalue = NULL;
    char buf[64];

    oldfs = get_fs();
    set_fs(KERNEL_DS);
    fd = sys_open(dev, O_RDONLY, 0);
    if (fd < 0)
    {
        THRML_LOG("[get_sys_battery_info] open fail dev:%s fd:%d \n", dev, fd);
        set_fs(oldfs);
        return fd;
    }

    nReadSize = sys_read(fd, buf, sizeof(buf) - 1);
    THRML_LOG("[get_sys_battery_info] nReadSize:%d\n", nReadSize);
    nRet = simple_strtol(buf, &pvalue, 10);

    set_fs(oldfs);
    sys_close(fd);

    return nRet;
}

//*********************************************
// Get Wifi Tx throughput
//*********************************************
static int get_sys_wifi_throughput(char* dev, int nRetryNr)
{
    int fd;
    int nRet;
    int nReadSize;
    int nRetryCnt=0;
    char *pvalue = NULL;
    char buf[64];

    oldfs = get_fs();
    set_fs(KERNEL_DS);

    /* If sys_open fail, it will retry "nRetryNr" times. */
    do {
        fd = sys_open(dev, O_RDONLY, 0);
        if(nRetryCnt > nRetryNr) {
            THRML_LOG("[get_sys_wifi_throughput] open fail dev:%s fd:%d \n", dev, fd);
            set_fs(oldfs);
            return fd;
        }
        nRetryCnt++;
    } while(fd < 0);

    if(nRetryCnt > 1) {
       THRML_LOG("[get_sys_wifi_throughput] open fail nRetryCnt:%d \n", nRetryCnt);
    }

    nReadSize = sys_read(fd, buf, sizeof(buf) - 1);
    THRML_LOG("[get_sys_wifi_throughput] nReadSize:%d\n", nReadSize);
    nRet = simple_strtol(buf, &pvalue, 10);

    set_fs(oldfs);
    sys_close(fd);

    return nRet;
}


//*********************************************
// For get_sys_cpu_usage_info_ex()
//*********************************************

#define CPU_USAGE_CURRENT_FIELD (0)
#define CPU_USAGE_SAVE_FIELD    (1)
#define CPU_USAGE_FRAME_FIELD   (2)

struct cpu_index_st
{
    unsigned long  u[3];
    unsigned long  s[3];
    unsigned long  n[3];
    unsigned long  i[3];
    unsigned long  w[3];
    unsigned long  q[3];
    unsigned long  sq[3]; 
    unsigned long  tot_frme;
    unsigned long  tz; 
    int  usage;
    int  freq;
};

struct gpu_index_st
{
    int  usage;
    int  freq;
};

static struct cpu_index_st cpu_index_list[4];   ///< 4-Core is maximum 
static struct gpu_index_st gpu_index;


#define SEEK_BUFF(x, c)  while(*x != c)x++; \
                            x++;

#define TRIMz_ex(tz, x)   ((tz = (unsigned long long)(x)) < 0 ? 0 : tz)

//*********************************************
// CPU Index
//*********************************************

static int get_sys_cpu_usage_info_ex(void)
{
    int fd;
    int nReadSize;
    char szTempBuf[512];
    char buf[256];
    char *pbuf;
    int nCoreIndex=0, i;


    oldfs = get_fs();
    set_fs(KERNEL_DS);
    fd = sys_open("/proc/stat", O_RDONLY, 0);

    if (fd < 0)
    {
        THRML_LOG("[get_sys_cpu_usage_info] open fail fd:%d \n", fd);
        set_fs(oldfs); 
        return -1;
    }

    nReadSize = sys_read(fd, buf, sizeof(buf) - 1);
    set_fs(oldfs);
    sys_close(fd);

    pbuf = buf;
    SEEK_BUFF(pbuf, '\n');
    
    THRML_LOG("[Read Buff]:%s \n", buf);


	for(nCoreIndex=0; nCoreIndex<NUMBER_OF_CORE ; nCoreIndex++)
	{
		sprintf(szTempBuf, "cpu%01d %%lu %%lu %%lu %%lu %%lu %%lu %%lu", nCoreIndex);

		/* Get CPU Info */
		sscanf(pbuf, szTempBuf, &cpu_index_list[nCoreIndex].u[CPU_USAGE_CURRENT_FIELD], &cpu_index_list[nCoreIndex].n[CPU_USAGE_CURRENT_FIELD], 
							    &cpu_index_list[nCoreIndex].s[CPU_USAGE_CURRENT_FIELD], &cpu_index_list[nCoreIndex].i[CPU_USAGE_CURRENT_FIELD], 
							    &cpu_index_list[nCoreIndex].w[CPU_USAGE_CURRENT_FIELD], &cpu_index_list[nCoreIndex].q[CPU_USAGE_CURRENT_FIELD], 
							    &cpu_index_list[nCoreIndex].sq[CPU_USAGE_CURRENT_FIELD]);

		SEEK_BUFF(pbuf, '\n');
        
        
		/* Frame */
	    cpu_index_list[nCoreIndex].u[CPU_USAGE_FRAME_FIELD] = cpu_index_list[nCoreIndex].u[CPU_USAGE_CURRENT_FIELD] -  
                                                                    cpu_index_list[nCoreIndex].u[CPU_USAGE_SAVE_FIELD];
		cpu_index_list[nCoreIndex].n[CPU_USAGE_FRAME_FIELD] = cpu_index_list[nCoreIndex].n[CPU_USAGE_CURRENT_FIELD] -  
                                                                    cpu_index_list[nCoreIndex].n[CPU_USAGE_SAVE_FIELD];
		cpu_index_list[nCoreIndex].s[CPU_USAGE_FRAME_FIELD] = cpu_index_list[nCoreIndex].s[CPU_USAGE_CURRENT_FIELD] -  
                                                                    cpu_index_list[nCoreIndex].s[CPU_USAGE_SAVE_FIELD];
		cpu_index_list[nCoreIndex].i[CPU_USAGE_FRAME_FIELD] = TRIMz_ex(cpu_index_list[nCoreIndex].tz, 
                                                                    (cpu_index_list[nCoreIndex].i[CPU_USAGE_CURRENT_FIELD] - 
                                                                     cpu_index_list[nCoreIndex].i[CPU_USAGE_SAVE_FIELD])) ;
		cpu_index_list[nCoreIndex].w[CPU_USAGE_FRAME_FIELD] = cpu_index_list[nCoreIndex].w[CPU_USAGE_CURRENT_FIELD] -  
                                                                    cpu_index_list[nCoreIndex].w[CPU_USAGE_SAVE_FIELD];
		cpu_index_list[nCoreIndex].q[CPU_USAGE_FRAME_FIELD] = cpu_index_list[nCoreIndex].q[CPU_USAGE_CURRENT_FIELD] -  
                                                                    cpu_index_list[nCoreIndex].q[CPU_USAGE_SAVE_FIELD] ;
		cpu_index_list[nCoreIndex].sq[CPU_USAGE_FRAME_FIELD] = cpu_index_list[nCoreIndex].sq[CPU_USAGE_CURRENT_FIELD] -  
                                                                    cpu_index_list[nCoreIndex].sq[CPU_USAGE_SAVE_FIELD];

		/* Total Frame */
		cpu_index_list[nCoreIndex].tot_frme = cpu_index_list[nCoreIndex].u[CPU_USAGE_FRAME_FIELD] +
											 cpu_index_list[nCoreIndex].n[CPU_USAGE_FRAME_FIELD] +
											 cpu_index_list[nCoreIndex].s[CPU_USAGE_FRAME_FIELD] +
											 cpu_index_list[nCoreIndex].i[CPU_USAGE_FRAME_FIELD] +
											 cpu_index_list[nCoreIndex].w[CPU_USAGE_FRAME_FIELD] +
											 cpu_index_list[nCoreIndex].q[CPU_USAGE_FRAME_FIELD] +
											 cpu_index_list[nCoreIndex].sq[CPU_USAGE_FRAME_FIELD];
		
		/* CPU Usage */
		if(cpu_index_list[nCoreIndex].tot_frme > 0)
		{
			cpu_index_list[nCoreIndex].usage = (100-(((int)cpu_index_list[nCoreIndex].i[CPU_USAGE_FRAME_FIELD]*100)/(int)cpu_index_list[nCoreIndex].tot_frme));
		}else
		{
			/* CPU unplug case */
			cpu_index_list[nCoreIndex].usage = 0;
		}

		cpu_index_list[nCoreIndex].u[CPU_USAGE_SAVE_FIELD]  = cpu_index_list[nCoreIndex].u[CPU_USAGE_CURRENT_FIELD];
	    cpu_index_list[nCoreIndex].n[CPU_USAGE_SAVE_FIELD]  = cpu_index_list[nCoreIndex].n[CPU_USAGE_CURRENT_FIELD];
	    cpu_index_list[nCoreIndex].s[CPU_USAGE_SAVE_FIELD]  = cpu_index_list[nCoreIndex].s[CPU_USAGE_CURRENT_FIELD];
		cpu_index_list[nCoreIndex].i[CPU_USAGE_SAVE_FIELD]  = cpu_index_list[nCoreIndex].i[CPU_USAGE_CURRENT_FIELD];
		cpu_index_list[nCoreIndex].w[CPU_USAGE_SAVE_FIELD]  = cpu_index_list[nCoreIndex].w[CPU_USAGE_CURRENT_FIELD];
		cpu_index_list[nCoreIndex].q[CPU_USAGE_SAVE_FIELD]  = cpu_index_list[nCoreIndex].q[CPU_USAGE_CURRENT_FIELD];
		cpu_index_list[nCoreIndex].sq[CPU_USAGE_SAVE_FIELD] = cpu_index_list[nCoreIndex].sq[CPU_USAGE_CURRENT_FIELD];
                    
		THRML_LOG("CPU%d Frame:%d USAGE:%d  \n", nCoreIndex, cpu_index_list[nCoreIndex].tot_frme, cpu_index_list[nCoreIndex].usage);

        for(i=0 ; i<3 ; i++)
        {
            THRML_LOG("Index [u:%d] [n:%d] [s:%d] [i:%d] [w:%d] [q:%d] [sq:%d] \n", i, cpu_index_list[nCoreIndex].u[i],
                                                                  cpu_index_list[nCoreIndex].n[i],
                                                                  cpu_index_list[nCoreIndex].s[i],
                                                                  cpu_index_list[nCoreIndex].i[i],
                                                                  cpu_index_list[nCoreIndex].w[i],
                                                                  cpu_index_list[nCoreIndex].q[i],
                                                                  cpu_index_list[nCoreIndex].sq[i]);

        }
	}//for

    return 0;

}

#if 0
/*
 * Read CPU Usage
 *
 * "cat /proc/stat"
 */
#define TRIMz(x)   ((tz = (unsigned long long)(x)) < 0 ? 0 : tz)
#define TRIMz1(x)  ((tz1 = (unsigned long long)(x)) < 0 ? 0 : tz1)

static int get_sys_cpu_usage_info(int *pcpu0_usage, int *pcpu1_usage)
{
    int fd;
    int nReadSize;
    char buf[256];
    char *pbuf;

    static unsigned long  u, u_sav, u_frme, s, s_sav, s_frme,
                              n, n_sav, n_frme, i, i_sav, i_frme,
                              w, w_sav, w_frme, q, q_sav, q_frme,
                              sq, sq_sav, sq_frme, tot_frme, tz ;

    static unsigned long u1, u1_sav, u1_frme, s1, s1_sav, s1_frme,
                              n1, n1_sav, n1_frme, i1, i1_sav, i1_frme,
                              w1, w1_sav, w1_frme, q1, q1_sav, q1_frme,
                              sq1, sq1_sav, sq1_frme, tot1_frme, tz1 ;


    oldfs = get_fs();
    set_fs(KERNEL_DS);
    fd = sys_open("/proc/stat", O_RDONLY, 0);

    if (fd < 0)
    {
        THRML_LOG("[get_sys_cpu_usage_info] open fail fd:%d \n", fd);
        set_fs(oldfs);
        return -1;
    }

    nReadSize = sys_read(fd, buf, sizeof(buf) - 1);
    set_fs(oldfs);
    sys_close(fd);
    /*
     *  Field Description:
     *  [user]: normal processes executing in user mode
     *  [nice]: niced processes executing in user mode
     *  [system]: processes executing in kernel mode
     *  [idle]: twiddling thumbs
     *  [iowait]: waiting for I/O to complete
     *  [irq]: servicing interrupts
     *  [softirq]: servicing softirqs
     */
    pbuf = buf;
    while(*pbuf != '\n')
    {
      pbuf++;
    }
    pbuf++;

    /* Get CPU Info */
    sscanf(pbuf, "cpu0 %lu %lu %lu %lu %lu %lu %lu",
                         &u, &n, &s, &i, &w, &q, &sq);

    THRML_LOG("[/proc/stat/] buf:%s \n", buf);

    /* Get CPU0 Info */
    while(*pbuf != '\n')
    {
      pbuf++;
    }
    pbuf++;

    sscanf(pbuf, "cpu1 %lu %lu %lu %lu %lu %lu %lu",
                         &u1, &n1, &s1, &i1, &w1, &q1, &sq1);

    THRML_LOG("[CPU0 Stat] %d %d %d %d %d %d %d \n", u, n, s, i, w, q, sq);
    THRML_LOG("[CPU Stat frame] %d %d %d %d %d %d %d \n", u_frme, n_frme, s_frme, i_frme, w_frme, q_frme, sq_frme);

    THRML_LOG("[CPU1 Stat] %d %d %d %d %d %d %d \n", u1, n1, s1, i1, w1, q1, sq1);
    THRML_LOG("[CPU1 Stat frame] %d %d %d %d %d %d %d \n", u1_frme, n1_frme, s1_frme, i1_frme, w1_frme, q1_frme, sq1_frme);


    u_frme = u - u_sav ;
    n_frme = n - n_sav ;
    s_frme = s - s_sav ;
    i_frme = TRIMz(i - i_sav) ;
    w_frme = w - w_sav ;
    q_frme = q - q_sav ;
    sq_frme = sq - sq_sav ;

    tot_frme = u_frme + s_frme + n_frme + i_frme + w_frme + q_frme + sq_frme;
    if(tot_frme > 0)
    {
        *pcpu0_usage = (100-(((int)i_frme*100)/(int)tot_frme));
    }else
    {
        /* CPU unplug case */
        *pcpu0_usage = -444; // TODO: why -444
    }

    u1_frme = u1 - u1_sav ;
    n1_frme = n1 - n1_sav ;
    s1_frme = s1 - s1_sav ;
    i1_frme = TRIMz1(i1 - i1_sav) ;
    w1_frme = w1 - w1_sav ;
    q1_frme = q1 - q1_sav ;
    sq1_frme = sq1 - sq1_sav ;
    tot1_frme = u1_frme + s1_frme + n1_frme + i1_frme + w1_frme + q1_frme + sq1_frme;
    if(tot1_frme > 0)
    {
        *pcpu1_usage = (int)(100-(((int)i1_frme*100)/(int)tot1_frme));
    }else
    {
        /* CPU unplug case */
        *pcpu1_usage = -444; // TODO: why -444
    }


    THRML_LOG("[CPU Usage] t=%d t1=%d i=%d i1=%d cu=%d c1u=%d\n", tot_frme, tot1_frme, i_frme, i1_frme, *pcpu0_usage, *pcpu1_usage);

    u_sav   = u;
    s_sav   = s;
    n_sav   = n;
    i_sav   = i;
    w_sav   = w;
    q_sav  = q;
    sq_sav = sq;

    u1_sav   = u1;
    s1_sav   = s1;
    n1_sav   = n1;
    i1_sav   = i1;
    w1_sav   = w1;
    q1_sav  = q1;
    sq1_sav = sq1;

    return 0;
}
#endif 

static int get_sys_cpu_freq_info(char* dev, int nRetryNr)
{
    int fd;
    int nRet=0;
    int nReadSize;
    int nRetryCnt=0;
    char *pvalue = NULL;
    char buf[64];

    oldfs = get_fs();
    set_fs(KERNEL_DS);


    /* If sys_open fail, it will retry three times. */
    do
    {
        fd = sys_open(dev, O_RDONLY, 0);
        if(nRetryCnt > nRetryNr)
        {
            THRML_LOG("[get_sys_cpu_freq_info] open fail dev:%s fd:%d \n", dev, fd);
            set_fs(oldfs);
            return fd;
        }
        nRetryCnt++;
    }while(fd < 0);

    if(nRetryCnt > 1)
    {
       THRML_LOG("[get_sys_cpu_freq_info] open fail nRetryCnt:%d \n", nRetryCnt);
    }

    nReadSize = sys_read(fd, buf, sizeof(buf) - 1);
    //THRML_LOG("[get_sys_cpu_freq_info] nReadSize:%d\n", nReadSize);
    nRet = simple_strtol(buf, &pvalue, 10);

    set_fs(oldfs);
    sys_close(fd);

    return nRet;
}

static int get_sys_all_cpu_freq_info(void)
{
    int nCPU_freq_temp, i;
    char szTempBuf[512];

    for(i=0 ; i<NUMBER_OF_CORE ; i++)
    {
        sprintf(szTempBuf, "/sys/devices/system/cpu/cpu%01d/cpufreq/cpuinfo_cur_freq", i);
        nCPU_freq_temp = get_sys_cpu_freq_info(szTempBuf, 3);
        if(nCPU_freq_temp > 0)
        {
            cpu_index_list[i].freq = nCPU_freq_temp/1000;
        }else
        {
            /* CPU is unplug now */
            cpu_index_list[i].freq = nCPU_freq_temp*10;
        }
    }      

    return 0;
}



static int mtk_sysinfo_get_info(void)
{
    int nBattVol, nBattTemp;
    int i;

    mutex_lock(&MTM_SYSINFO_LOCK);

    //******************
    // Battery
    //******************

    /* Read Battery Information */
#if defined(CONFIG_ARCH_MT6575) || defined(CONFIG_ARCH_MT6577)

    nBattCurrentCnsmpt = get_sys_battery_info("/sys/devices/platform/mt6329-battery/FG_Battery_CurrentConsumption");
    // the return value is 0.1mA
    if (nBattCurrentCnsmpt%10 <5)
        nBattCurrentCnsmpt /= 10;
    else
        nBattCurrentCnsmpt = 1+(nBattCurrentCnsmpt/10);
    
#if defined(CONFIG_MTK_SMART_BATTERY)
    if (KAL_TRUE == gFG_Is_Charging)
    {
        nBattCurrentCnsmpt *= -1;
    }
#endif

#elif defined(CONFIG_ARCH_MT6589)

    // MT6589 PMIC is MT6320
    nBattCurrentCnsmpt = get_sys_battery_info("/sys/devices/platform/mt6320-battery/FG_Battery_CurrentConsumption");
    // the return value is 0.1mA
    if (nBattCurrentCnsmpt%10 <5)
        nBattCurrentCnsmpt /= 10;
    else
        nBattCurrentCnsmpt = 1+(nBattCurrentCnsmpt/10);


#if defined(CONFIG_MTK_SMART_BATTERY)
    if (KAL_TRUE == gFG_Is_Charging)
    {
        nBattCurrentCnsmpt *= -1;
    }
#endif
    
#else
    nBattCurrentCnsmpt = 0;
#endif

    nBattVol = get_sys_battery_info("/sys/class/power_supply/battery/batt_vol");
    nBattTemp = get_sys_battery_info("/sys/class/power_supply/battery/batt_temp");
    
    
    THRML_LOG("[mtk_sysinfo_get_info] nBattCurrentCnsmpt=%d nBattVol=%d nBattTemp=%d \n", nBattCurrentCnsmpt, nBattVol, nBattTemp);
    THRML_STORAGE_LOG(THRML_LOGGER_MSG_BATTERY_INFO, get_battery_info, nBattCurrentCnsmpt, nBattVol, nBattTemp);

    //******************
    // CPU Usage
    //******************

    /* Read CPU Usage Information */
    get_sys_cpu_usage_info_ex();

    /* CPU loading average */
    nCPU_loading_sum = 0;
    for(i=0 ; i<NUMBER_OF_CORE ; i++)
    {
        nCPU_loading_sum += cpu_index_list[i].usage;
    }
    

    //******************
    // CPU Frequency 
    //******************
    get_sys_all_cpu_freq_info();

    //******************
    // GPU Index
    //******************
#if defined(CONFIG_ARCH_MT6589)
    gpu_index.usage = mt_gpufreq_cur_load();
    gpu_index.freq = mt_gpufreq_cur_freq()/1000; // the return value is KHz
#endif

    //******************
    // Modem Index
    //******************
    {
        struct md_info *p_info;
        int size, i;
        mtk_mdm_get_md_info(&p_info, &size);        
        for(i=0; i<size; i++) 
        {
            THRML_STORAGE_LOG(THRML_LOGGER_MSG_MISC_EX_INFO, get_misc_ex_info, 
                                    p_info[i].attribute, p_info[i].value, p_info[i].unit);   
        }
    }

    //******************
    // Wifi Index
    //******************
    /* Get Wi-Fi Tx throughput */
    nWifi_throughput = get_sys_wifi_throughput("/proc/wmt_tm/tx_thro", 3);
 
    mutex_unlock(&MTM_SYSINFO_LOCK);

    /* CPU and GPU to storage logger */
    THRML_STORAGE_LOG(THRML_LOGGER_MSG_CPU_INFO_EX, get_cpu_info_ex, 
                            cpu_index_list[0].usage, cpu_index_list[1].usage, 
                            cpu_index_list[2].usage, cpu_index_list[3].usage,
                            cpu_index_list[0].freq , cpu_index_list[1].freq,
                            cpu_index_list[2].freq , cpu_index_list[3].freq, 
                            gpu_index.usage, gpu_index.freq);
    
    /* MISC to storage logger */
    // Modem and  WIFI ....     
    THRML_STORAGE_LOG(THRML_LOGGER_MSG_MISC_EX_INFO, get_misc_ex_info, 
                            "WiFi_TP", nWifi_throughput, "index");  
    //THRML_STORAGE_LOG(THRML_LOGGER_MSG_MISC_INFO, get_misc_info, nModem_TxPower, nWifi_throughput); 
    

    THRML_LOG("[mtk_sysinfo_get_info]CPU Usage C0=%d C1=%d C2=%d C3=%d \n", 
                            cpu_index_list[0].usage, cpu_index_list[1].usage, 
                            cpu_index_list[2].usage, cpu_index_list[3].usage );

    THRML_LOG("[mtk_sysinfo_get_info]CPU Freq C0=%d C1=%d C2=%d C3=%d \n", 
                            cpu_index_list[0].freq, cpu_index_list[1].freq, 
                            cpu_index_list[2].freq, cpu_index_list[3].freq );

    return 0;
}

static void _mtm_update_sysinfo(struct work_struct *work)
{
    if ((true == enable_ThermalMonitor) || (0x0 != g_check_sys_info_flag))
        mtk_sysinfo_get_info();
    
    cancel_delayed_work(&_mtm_sysinfo_poll_queue);

    queue_delayed_work(system_freezable_wq, &_mtm_sysinfo_poll_queue, msecs_to_jiffies(1000));
}


//************************************
// Thermal Host Driver Interface
//************************************

/* Read */
static int mtkthermal_read(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    char *p = buf;

    p += sprintf(p, "\r\n[Thermal Monitor debug flag]\r\n");
    p += sprintf(p, "=========================================\r\n" );
    p += sprintf(p, "enable_ThermalMonitor = %d\r\n", enable_ThermalMonitor);
    p += sprintf(p, "enable_ThermalMonitorXlog = %d\r\n", enable_ThermalMonitorXlog);
    p += sprintf(p, "g_nStartRealTime = %d\r\n", g_nStartRealTime);

    *start = buf + off;

    xlog_printk(ANDROID_LOG_INFO, "THERMAL/MONITOR", "[mtkthermal_monitor] enable_ThermalMonitor:%d\n", enable_ThermalMonitor);

    len = p - buf;
    if (len > off)
        len -= off;
    else
        len = 0;

    return len < count ? len  : count;
}

/* Write */
static ssize_t mtkthermal_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int len=0, nCtrlCmd=0, nReadTime=0;
    char desc[32];

    len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
    if (copy_from_user(desc, buffer, len))
    {
        return 0;
    }
    desc[len] = '\0';

    if(sscanf(desc, "%d %d", &nCtrlCmd, &nReadTime) == 2)
    {
        /* Bit 0; Enable Thermal Monitor. */
        if ((nCtrlCmd>>0) & 0x01)
        {
            /* Reset Global CPU Info Variable */
            memset( &cpu_index_list, 0x00, sizeof(cpu_index_list));

            //mtk_mdm_start_query();
            enable_ThermalMonitor = true;
        } else {            
            //mtk_mdm_stop_query();
            enable_ThermalMonitor = false;
        } 

        /* Bit 1: Enable Thermal Monitor xlog */
        enable_ThermalMonitorXlog = ((nCtrlCmd>>1) & 0x01)?true:false;

        /*
         * Get Real Time from user input
         * Format: hhmmss 113901=> 11:39:01
         */
        g_nStartRealTime = nReadTime;

        THRML_STORAGE_LOG(THRML_LOGGER_MSG_DEC_NUM, get_real_time, "[realtime]",g_nStartRealTime);
        xlog_printk(ANDROID_LOG_INFO, "THERMAL/MONITOR", "[mtkthermal_monitor] nCtrlCmd=%d enable_ThermalMonitor=%d \n", nCtrlCmd, enable_ThermalMonitor);
        xlog_printk(ANDROID_LOG_INFO, "THERMAL/MONITOR", "[mtkthermal_monitor] g_nStartRealTime=%d \n", g_nStartRealTime);

        return count;
    }
    else if (sscanf(desc, "%d", &nCtrlCmd) == 1)
    {
        /* Bit 0; Enable Thermal Monitor. */
        if ((nCtrlCmd>>0) & 0x01)
        {
            /* Reset Global CPU Info Variable */
            memset( &cpu_index_list, 0x00, sizeof(cpu_index_list));

            //mtk_mdm_start_query();
            enable_ThermalMonitor = true;
        } else {            
            //mtk_mdm_stop_query();
            enable_ThermalMonitor = false;
        } 

        /* Bit 1: Enable Thermal Monitor xlog */
        enable_ThermalMonitorXlog = ((nCtrlCmd>>1) & 0x01)?true:false;

        xlog_printk(ANDROID_LOG_INFO, "THERMAL/MONITOR", "[mtkthermal_monitor] nCtrlCmd=%d enable_ThermalMonitor=%d \n", nCtrlCmd, enable_ThermalMonitor);

        return count;
    }
    else
    {
        THRML_LOG("[mtkthermal_monitor] bad argument\n");
    }

    return -EINVAL;
}

static int _mtkthermal_check_cooler_conditions(
    struct mtk_thermal_cooler_data *cldata)
{
    int ret = 0;

    if (NULL == cldata)
    {
    }
    else
    {
        int i = 0;
        for (; i < MTK_THERMAL_MONITOR_COOLER_MAX_EXTRA_CONDITIONS; i++)
        {
            if (NULL == cldata->condition_last_value[i])
            {
                ret++;
            }
            else
            {
                if (*cldata->condition_last_value[i] > cldata->threshold[i])
                {
                    ret++;
                }
            }
        }
        mb();
    }

    return ret;
}

static void _mtkthermal_clear_cooler_conditions(
    struct mtk_thermal_cooler_data *cldata)
{
    int i = 0;
    for (; i < MTK_THERMAL_MONITOR_COOLER_MAX_EXTRA_CONDITIONS; i++)
    {
        cldata->conditions[i][0] = 0x0;
        cldata->condition_last_value[i] = NULL;
        cldata->threshold[i] = 0;
    }
    g_check_sys_info_flag &= (~(0x1 << cldata->id));
}

static int _mtkthermal_cooler_read(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    char *p = buf;
    struct mtk_thermal_cooler_data *mcdata;

    THRML_LOG("[_mtkthermal_cooler_read] invoked.\n");

    /**
     * The format to print out
     * <condition_name_1> <condition_value_1> <thershold_1> <state_1>
     * ..
     * <condition_name_n> <condition_value_n> <thershold_n> <state_n>
     * PS: n is MTK_THERMAL_MONITOR_COOLER_MAX_EXTRA_CONDITIONS
     */
    if (NULL == data)
    {
        THRML_ERROR_LOG("[_mtkthermal_cooler_read] null data\n");
    }
    else
    {
        int i = 0;

        // TODO: we may not need to lock here...
        mutex_lock(&MTM_COOLER_LOCK);
        mcdata = (struct mtk_thermal_cooler_data *)data;
        mutex_unlock(&MTM_COOLER_LOCK);

        for (; i < MTK_THERMAL_MONITOR_COOLER_MAX_EXTRA_CONDITIONS; i++)
        {
            if (0x0 == mcdata->conditions[i][0]) continue; // no condition

            // TODO: consider the case that tz is unregistered...
            p += sprintf(p, "%s val=%d threshold=%d %s",
                            mcdata->conditions[i],
                            (NULL == mcdata->condition_last_value[i]) ? 0 : *(mcdata->condition_last_value[i]),
                            mcdata->threshold[i],
                            (NULL == mcdata->condition_last_value[i]) ? "error\n" : "\n");
        }
    }

    *start = buf + off;

    len = p - buf;
    if (len > off)
        len -= off;
    else
        len = 0;

    return len < count ? len  : count;
}

static ssize_t _mtkthermal_cooler_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int len = 0;
    char desc[32];
    struct mtk_thermal_cooler_data *mcdata;

    len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
    if (copy_from_user(desc, buffer, len))
    {
        return 0;
    }
    desc[len] = '\0';

    /**
     * sscanf format <condition_1> <threshold_1> ... <condition_n> <threshold_n>
     * <condition_i> is string format
     * <threshold_i> is integer format
     * n is MTK_THERMAL_MONITOR_COOLER_MAX_EXTRA_CONDITIONS
     */

    if (NULL == data)
    {
        THRML_ERROR_LOG("[_mtkthermal_cooler_write] null data\n");
        return -EINVAL;
    }

    // TODO: we may not need to lock here...
    mutex_lock(&MTM_COOLER_LOCK);
    mcdata = (struct mtk_thermal_cooler_data *)data;
    mutex_unlock(&MTM_COOLER_LOCK);

    // WARNING: Modify here if MTK_THERMAL_MONITOR_COOLER_MAX_EXTRA_CONDITIONS is changed to other than 3
#if (3 == MTK_THERMAL_MONITOR_COOLER_MAX_EXTRA_CONDITIONS)
    _mtkthermal_clear_cooler_conditions(mcdata);

    if (2 <= sscanf(desc, "%s %d %s %d %s %d",
                    &mcdata->conditions[0][0], &mcdata->threshold[0],
                    &mcdata->conditions[1][0], &mcdata->threshold[1],
                    &mcdata->conditions[2][0], &mcdata->threshold[2]))
    {
        int i = 0;

        for (; i < MTK_THERMAL_MONITOR_COOLER_MAX_EXTRA_CONDITIONS; i++)
        {
            if (0 == strncmp(mcdata->conditions[i], "CPU0", 4))
            {
                mcdata->condition_last_value[i] = &nCPU_loading_sum;
                g_check_sys_info_flag |= (0x1 << mcdata->id);
            }
            else if (0 == strncmp(mcdata->conditions[i], "BATCC", 5))
            {
                mcdata->condition_last_value[i] = &nBattCurrentCnsmpt;
                g_check_sys_info_flag |= (0x1 << mcdata->id);
            }
            else if (0 == strncmp(mcdata->conditions[i], "EXIT", 4))
            {
                mcdata->exit_threshold = mcdata->threshold[i];
            }
            else if (0 == strncmp(mcdata->conditions[i], "WIFI", 4))
            {
                mcdata->condition_last_value[i] = &nWifi_throughput;
                g_check_sys_info_flag |= (0x1 << mcdata->id);
            }
            else
            {
                // normal thermal zones
                mcdata->condition_last_value[i] = NULL;
            }
            THRML_LOG("[_mtkthermal_cooler_write] %d: %s %x %x %d.\n",
                      i, &mcdata->conditions[i][0], mcdata->conditions[i][0], mcdata->condition_last_value[i], mcdata->threshold[0]);
        }

        return count;
    }
    else
#else
#error "Change correspondent part when changing MTK_THERMAL_MONITOR_COOLER_MAX_EXTRA_CONDITIONS!"
#endif
    {
        THRML_ERROR_LOG("[_mtkthermal_cooler_write] bad argument\n");
    }

    return -EINVAL;
}

static int _mtkthermal_tz_read(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    char *p = buf;
    struct thermal_zone_device *tz = NULL;

    THRML_LOG("[_mtkthermal_tz_read] invoked.\n");

    if (NULL == data)
    {
        THRML_ERROR_LOG("[_mtkthermal_tz_read] null data\n");
    }
    else
    {
        tz = (struct thermal_zone_device *) data;
        // TODO: consider the case that tz is unregistered...
        p += sprintf(p, "%d\n", tz->last_temperature);
        {
            struct mtk_thermal_tz_data *tzdata = NULL;
            int ma_len = 0;
            int fake_temp = 0;
            tzdata = tz->devdata;
            if (!tzdata)
            {
                BUG();
            }

#if (MAX_STEP_MA_LEN > 1)
            mutex_lock(&tzdata->ma_lock);
            ma_len = tzdata->ma_len;
            fake_temp = tzdata->fake_temp;
            p += sprintf(p, "ma_len=%d\n", ma_len);
            p += sprintf(p, "%d ", tzdata->ma_lens[0]);
            {
                int i = 1;
                for (; i<MAX_STEP_MA_LEN; i++)
                    p += sprintf(p, "(%ld,%d) ", tzdata->msma_ht[i-1], tzdata->ma_lens[i]);
            }
            mutex_unlock(&tzdata->ma_lock);
            p += sprintf(p, "\n");
#else
            mutex_lock(&tzdata->ma_lock);
            ma_len = tzdata->ma_len;
            fake_temp = tzdata->fake_temp;
            mutex_unlock(&tzdata->ma_lock);
            p += sprintf(p, "ma_len=%d\n", ma_len);
#endif
            if (-275000 < fake_temp)
            {
                // print Tfake only when fake_temp > -275000
                p += sprintf(p, "Tfake=%d\n", fake_temp);
            }
        }
    }

    *start = buf + off;

    len = p - buf;
    if (len > off)
        len -= off;
    else
        len = 0;

    return len < count ? len  : count;
}

static ssize_t _mtkthermal_tz_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int len = 0;
    char desc[128];
    char trailing[128]={0};

    len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
    if (copy_from_user(desc, buffer, len))
    {
        return 0;
    }
    desc[len] = '\0';

    if (NULL == data)
    {
        THRML_ERROR_LOG("[_mtkthermal_tz_write] null data\n");
        return -EINVAL;
    }
    else
    {
        char arg_name[32] = {0};
        int arg_val = 0;
        struct thermal_zone_device *tz = NULL;

        if (2 <= sscanf(desc, "%s %d %s", arg_name, &arg_val, trailing))
        {
            if ((0 == strncmp(arg_name, "ma_len", 6)) && (arg_val >= 1) && (arg_val <= 60))
            {
                struct mtk_thermal_tz_data *tzdata = NULL;
                tz = (struct thermal_zone_device *) data;
                tzdata = tz->devdata;
                if (!tzdata)
                {
                    BUG();
                }

                THRML_ERROR_LOG("[_mtkthermal_tz_write] trailing=%s\n", trailing);

                /**
                 *  reset MA len and lock
                 */
#if (MAX_STEP_MA_LEN > 1)
                mutex_lock(&tzdata->ma_lock);
                tzdata->ma_len = arg_val;
                tzdata->ma_counter = 0;
                tzdata->curr_idx_ma_len = 0;
                tzdata->ma_lens[0] = arg_val;
                tzdata->msma_ht[0] = MSMA_MAX_HT;
                THRML_ERROR_LOG("[_mtkthermal_tz_write] %s ma_len=%d.\n", tz->type, tzdata->ma_len);

#if (MAX_STEP_MA_LEN == 4)
                // reset
                tzdata->msma_ht[1] = tzdata->msma_ht[2] = tzdata->msma_ht[3] = MSMA_MAX_HT;
                tzdata->ma_lens[1] = tzdata->ma_lens[2] = tzdata->ma_lens[3] = 1;
                sscanf(trailing, "%ld,%d;%ld,%d;%ld,%d;", &tzdata->msma_ht[0], &tzdata->ma_lens[1], 
                                                       &tzdata->msma_ht[1], &tzdata->ma_lens[2], 
                                                       &tzdata->msma_ht[2], &tzdata->ma_lens[3]);
                THRML_ERROR_LOG("[_mtkthermal_tz_write] %s (%d, %d), (%d, %d), (%d, %d)\n", tz->type, 
                                                       tzdata->msma_ht[0], tzdata->ma_lens[1], 
                                                       tzdata->msma_ht[1], tzdata->ma_lens[2], 
                                                       tzdata->msma_ht[2], tzdata->ma_lens[3]);
#else
    #error
#endif
                mutex_unlock(&tzdata->ma_lock);
#else
                mutex_lock(&tzdata->ma_lock);
                tzdata->ma_len = arg_val;
                tzdata->ma_counter = 0;
                mutex_unlock(&tzdata->ma_lock);
                THRML_ERROR_LOG("[_mtkthermal_tz_write] %s ma_len=%d.\n", tz->type, tzdata->ma_len);
#endif
            }
            else if ((0 == strncmp(arg_name, "Tfake", 5)) && (arg_val >= -275000))
            {
                // only accept for [-275000, max positive value of int]
                struct mtk_thermal_tz_data *tzdata = NULL;
                tz = (struct thermal_zone_device *) data;
                tzdata = tz->devdata;
                if (!tzdata)
                {
                    BUG();
                }
                
                mutex_lock(&tzdata->ma_lock);
                tzdata->fake_temp = (long) arg_val;
                mutex_unlock(&tzdata->ma_lock);
                THRML_ERROR_LOG("[_mtkthermal_tz_write] %s Tfake=%d.\n", tz->type, tzdata->fake_temp);
            }
            
            return count;
        }
        else
        {
            return -EINVAL;
        }
    }
}

#define MIN(_a_, _b_) ((_a_) < (_b_) ? (_a_) : (_b_))

// No parameter check in this internal function
static long _mtkthermal_update_and_get_sma(struct mtk_thermal_tz_data *tzdata, long latest_val)
{
    long ret = 0;

    if (NULL == tzdata)
    {
        BUG();
        return latest_val;
    }
    
    mutex_lock(&tzdata->ma_lock);
    // Use Tfake if set...
    latest_val = (-275000 < tzdata->fake_temp) ? tzdata->fake_temp : latest_val;
    
    if (1 == tzdata->ma_len)
    {
        ret = latest_val;
    }
    else
    {
        int i = 0;
        
        tzdata->ma[(tzdata->ma_counter)%(tzdata->ma_len)] = latest_val;
        tzdata->ma_counter++;
        for (i = 0; i < MIN(tzdata->ma_counter, tzdata->ma_len); i++)
        {
            ret += tzdata->ma[i];
        }
        ret = ret / ((long) MIN(tzdata->ma_counter, tzdata->ma_len));
    }

#if (MAX_STEP_MA_LEN > 1)

    /*
     *  2. Move to correct region if ma_counter == 1
     *      a. For (i=0;SMA >= high_threshold[i];i++) ;
     *      b. if (curr_idx_sma_len != i) {ma_counter = 0; ma_len = sma_len[curr_idx_sma_len = i]; }
     *  3. Check if need to change region if ma_counter > 1
     *      a. if SMA >= high_threshold[curr_idx_sma_len] { Move upward: ma_counter = 0; ma_len = sma_len[++curr_idx_sma_len]; }
     *      b. else if curr_idx_sma_len >0 && SMA < high_threshold[curr_idx_sma_len-1] { Move downward: ma_counter =0; ma_len = sma_len[--curr_idx_sma_len]; }
     */
    if (1 == tzdata->ma_counter)
    {
        int i = 0;
        for (; ret >= tzdata->msma_ht[i];i++);
        if (tzdata->curr_idx_ma_len != i) 
        {
            tzdata->ma_counter = 0;
            tzdata->ma_len = tzdata->ma_lens[tzdata->curr_idx_ma_len = i];
            THRML_LOG("[_mtkthermal_update_and_get_sma] 2b ma_len: %d curr_idx_ma_len: %d\n", tzdata->ma_len, tzdata->curr_idx_ma_len);
        }
    }
    else
    {
        if (ret >= tzdata->msma_ht[tzdata->curr_idx_ma_len])
        {
            tzdata->ma_counter = 0;
            tzdata->ma_len = tzdata->ma_lens[++(tzdata->curr_idx_ma_len)];
            THRML_LOG("[_mtkthermal_update_and_get_sma] 3a ma_len: %d curr_idx_ma_len: %d\n", tzdata->ma_len, tzdata->curr_idx_ma_len);
        }
        else if (tzdata->curr_idx_ma_len > 0 && ret < tzdata->msma_ht[tzdata->curr_idx_ma_len-1])
        {
            tzdata->ma_counter = 0;
            tzdata->ma_len = tzdata->ma_lens[--(tzdata->curr_idx_ma_len)];
            THRML_LOG("[_mtkthermal_update_and_get_sma] 3b ma_len: %d curr_idx_ma_len: %d\n", tzdata->ma_len, tzdata->curr_idx_ma_len);
        }
    }
#endif
    
    mutex_unlock(&tzdata->ma_lock);
    return ret;
}

/**
 *  0: means please do not show thermal limit in "Show CPU Usage" panel.
 *  1: means show thermal limit and CPU temp only
 *  2: means show all all tz temp besides thermal limit and CPU temp
 */
static unsigned int g_thermal_indicator_mode = 0;

/**
 *  delay in milliseconds.
 */
static unsigned int g_thermal_indicator_delay = 0;

/* Read */
static int _mtkthermal_indicator_read(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    char *p = buf;

    p += sprintf(p, "%d\n%d\n", g_thermal_indicator_mode, g_thermal_indicator_delay);
 
    *start = buf + off;

    len = p - buf;
    if (len > off)
        len -= off;
    else
        len = 0;

    return len < count ? len  : count;
}

/* Write */
static ssize_t _mtkthermal_indicator_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int len = 0, thermal_indicator_mode = 0, thermal_indicator_delay = 0;
    char desc[32];

    len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
    if (copy_from_user(desc, buffer, len))
    {
        return 0;
    }
    desc[len] = '\0';

    if(sscanf(desc, "%d %d", &thermal_indicator_mode, &thermal_indicator_delay) == 2)
    {
        if ((thermal_indicator_mode >= 0) && (thermal_indicator_mode <= 3))
        {
            g_thermal_indicator_mode = thermal_indicator_mode;
        }
        g_thermal_indicator_delay = thermal_indicator_delay;
        return count;
    }
    else
    {
        return 0;
    }
}

/* Read */
static int _mtm_scen_call_read(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    char *p = buf;

    p += sprintf(p, "%d\n", g_mtm_phone_call_ongoing);
 
    *start = buf + off;

    len = p - buf;
    if (len > off)
        len -= off;
    else
        len = 0;

    return len < count ? len  : count;
}

/* Write */
static ssize_t _mtm_scen_call_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int len = 0, mtm_phone_call_ongoing = 0;
    char desc[32];

    len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
    if (copy_from_user(desc, buffer, len))
    {
        return 0;
    }
    desc[len] = '\0';

    if(sscanf(desc, "%d", &mtm_phone_call_ongoing) == 1)
    {
        if ((mtm_phone_call_ongoing == 0) || (mtm_phone_call_ongoing == 1))
        {
            g_mtm_phone_call_ongoing = mtm_phone_call_ongoing;
        }
        return count;
    }
    else
    {
        return 0;
    }
}

/* Init */
static int __init mtkthermal_init(void)
{
    int err = 0;
    struct proc_dir_entry *entry;

    THRML_LOG("[mtkthermal_init] \n");

    entry = create_proc_entry("driver/mtk_thermal_monitor", S_IRUGO | S_IWUSR, NULL);
    if (entry)
    {
        entry->read_proc = mtkthermal_read;
        entry->write_proc = mtkthermal_write;
    }

    entry = create_proc_entry("driver/mtk_thermal_indicator", S_IRUGO | S_IWUSR, NULL);
    if (entry)
    {
        entry->read_proc = _mtkthermal_indicator_read;
        entry->write_proc = _mtkthermal_indicator_write;
    }

    entry = create_proc_entry("driver/mtm_scen_call", S_IRUGO | S_IWUSR | S_IWGRP, NULL);
    if (entry)
    {
        entry->read_proc = _mtm_scen_call_read;
        entry->write_proc = _mtm_scen_call_write;
    }

    /* create /proc/cooler folder */
    // WARNING! This is not gauranteed to be invoked before mtk_ts_cpu's functions...
    proc_cooler_dir_entry = (NULL == proc_cooler_dir_entry) ? proc_mkdir("mtkcooler", NULL) : proc_cooler_dir_entry;
    if (NULL == proc_cooler_dir_entry)
    {
        THRML_ERROR_LOG("[mtkthermal_init]: mkdir /proc/mtkcooler failed\n");
    }

    /* create /proc/tz folder */
    // WARNING! This is not gauranteed to be invoked before mtk_ts_cpu's functions...
    proc_tz_dir_entry = (NULL == proc_tz_dir_entry) ? proc_mkdir("mtktz", NULL) : proc_tz_dir_entry;
    if (NULL == proc_tz_dir_entry)
    {
        THRML_ERROR_LOG("[mtkthermal_init]: mkdir /proc/mtktz failed\n");
    }

#if defined(CONFIG_MTK_THERMAL_TIME_BASE_PROTECTION)
    wake_lock_init(&mtm_wake_lock, WAKE_LOCK_SUSPEND, "alarm");
#endif

    INIT_DELAYED_WORK(&_mtm_sysinfo_poll_queue, _mtm_update_sysinfo);
    _mtm_update_sysinfo(NULL);

    return err;
}

/* Exit */
static void __exit mtkthermal_exit(void)
{
   THRML_LOG("[mtkthermal_exit] \n");
#if defined(CONFIG_MTK_THERMAL_TIME_BASE_PROTECTION)
   wake_lock_destroy(&mtm_wake_lock);
#endif
}

#if defined(CONFIG_MTK_THERMAL_TIME_BASE_PROTECTION)
static int __init mtkthermal_late_init(void)
{
    THRML_LOG("[mtkthermal_late_init] \n");
    return platform_driver_register(&mtk_thermal_monitor_driver);
}
#endif

//************************************
// thermal_zone_device_ops Wrapper
//************************************

/*
 * .bind wrapper: bind the thermal zone device with a thermal cooling device.
 */
static int mtk_thermal_wrapper_bind
(
    struct thermal_zone_device *thermal,
    struct thermal_cooling_device *cdev
)
{
    int ret = 0;
    struct thermal_zone_device_ops *ops;

    // WARNING! bind will invoke mtk_thermal_zone_bind_cooling_device_wrapper(), so don't rollback cooler's devdata in this bind...

#if MTK_THERMAL_MONITOR_CONDITIONAL_COOLING
    {
        int i = 0;
        struct mtk_thermal_cooler_data *cldata = NULL;

        mutex_lock(&MTM_COOLER_LOCK);
        cldata = cdev->devdata;
        mutex_unlock(&MTM_COOLER_LOCK);

        for (; i < MTK_THERMAL_MONITOR_COOLER_MAX_EXTRA_CONDITIONS; i++)
        {
            if ((0x0 != cldata->conditions[i][0]) &&
                (NULL == cldata->condition_last_value[i]))
            {
                if (0 == strncmp(cldata->conditions[i], thermal->type, 20))
                {
                    cldata->condition_last_value[i] = &(thermal->last_temperature);
                    THRML_LOG("[.bind]condition+ tz: %s cdev: %s condition: %s \n",
                              thermal->type, cdev->type, cldata->conditions[i]);
                }
            }
        }
    }
#endif

    /* Bind Relationship to StoreLogger */
    THRML_LOG("[.bind]+ tz: %s cdev: %s tz_data:%d cl_data:%d \n", thermal->type, cdev->type, thermal->devdata, cdev->devdata);

    ops = getClientZoneOps(thermal);

    if (ops->bind)
        ops->bind(thermal, cdev);

    /* Bind Relationship to StoreLogger */
    THRML_LOG("[.bind]- tz: %s cdev: %s tz_data:%d cl_data:%d \n", thermal->type, cdev->type, thermal->devdata, cdev->devdata);

    /* Log in mtk_thermal_zone_bind_cooling_device_wrapper() */
    //THRML_STORAGE_LOG(THRML_LOGGER_MSG_BIND, bind, thermal->type, cdev->type);

    return ret;
}

/*
 *.unbind wrapper: unbind the thermal zone device with a thermal cooling device.
 */
static int mtk_thermal_wrapper_unbind
(
    struct thermal_zone_device *thermal,
    struct thermal_cooling_device *cdev
)
{
    int ret = 0;
    struct thermal_zone_device_ops *ops;

#if MTK_THERMAL_MONITOR_CONDITIONAL_COOLING
    {
        int i = 0;
        struct mtk_thermal_cooler_data *cldata = NULL;
        mutex_lock(&MTM_COOLER_LOCK);
        cldata = cdev->devdata;
        mutex_unlock(&MTM_COOLER_LOCK);

        // Clear cldata->tz
        if (thermal == cldata->tz)
        {
            cldata->tz = NULL;
            cldata->trip = 0;
            // clear the state of cooler bounded
            if (cdev->ops)
                cdev->ops->set_cur_state(cdev, 0);
        }

        for (; i < MTK_THERMAL_MONITOR_COOLER_MAX_EXTRA_CONDITIONS; i++)
        {
            if ((NULL != cldata->condition_last_value[i]) &&
                (&(thermal->last_temperature) == cldata->condition_last_value[i]))
            {
                cldata->condition_last_value[i] = NULL;
                THRML_LOG("[.unbind]condition- tz: %s cdev: %s condition: %s\n", thermal->type, cdev->type, cldata->conditions[i]);
            }
        }
    }
#endif

    THRML_LOG("[.unbind]+ tz: %s cdev: %s \n", thermal->type, cdev->type);

    ops = getClientZoneOps(thermal);

    // Move this down
    //THRML_LOG("[.unbind] thermal_type:%s cdev_type:%s \n", thermal->type, cdev->type);

    if (ops->unbind)
        ret = ops->unbind(thermal, cdev);

    THRML_LOG("[.unbind]- tz: %s cdev: %s \n", thermal->type, cdev->type);

    return ret;
}

/*
 * .get_temp wrapper: get the current temperature of the thermal zone.
 */
static int mtk_thermal_wrapper_get_temp
(
    struct thermal_zone_device *thermal,
    unsigned long *temperature
)
{
    int ret = 0;
    struct thermal_zone_device_ops *ops;
    int nTemperature;
    unsigned long raw_temp;
#if MTK_THERMAL_MONITOR_MEASURE_GET_TEMP_OVERHEAD
    long int t = _get_current_time_us();
    long int dur = 0;
#endif

    ops = getClientZoneOps(thermal);

    if(ops->get_temp)
        ret = ops->get_temp(thermal, &raw_temp);

    nTemperature = (int) raw_temp; ///< Long cast to INT.
    

#if defined(CONFIG_MTK_THERMAL_TIME_BASE_PROTECTION)
    // if batt temp raw data < 60C, release wake lock
    if ((tz_last_values[MTK_THERMAL_SENSOR_BATTERY] != NULL) && // batt TZ is registered
        (&(thermal->last_temperature) == tz_last_values[MTK_THERMAL_SENSOR_BATTERY])) // get batt temp this time
    {
        if (wake_lock_active(&mtm_wake_lock))
        {
            nTemperature = force_get_tbat()*1000;
            raw_temp = nTemperature;
            THRML_ERROR_LOG("[.get_temp] tz: %s wake_lock_active() batt temp=%d\n", thermal->type, nTemperature);
        }
        
        if (nTemperature < 59000 && wake_lock_active(&mtm_wake_lock)) // unlock when only batt temp below 60C
        {
            THRML_ERROR_LOG("[.get_temp] tz: %s wake_unlock()\n", thermal->type);
            wake_unlock(&mtm_wake_lock);
        }
        
        last_batt_raw_temp = nTemperature;
    }
#endif

    *temperature = _mtkthermal_update_and_get_sma(thermal->devdata, raw_temp); // No strong type cast...

    /* Monitor Temperature to StoreLogger */
    THRML_STORAGE_LOG(THRML_LOGGER_MSG_ZONE_TEMP, get_temp, thermal->type, (int) *temperature);
    THRML_LOG("[.get_temp] tz: %s raw: %d sma: %d\n", thermal->type, nTemperature, *temperature);

    /* Collect system info */
#if 0   // move to _mtm_update_sysinfo()
    if ((ops != NULL) && ((int)ops == g_SysinfoAttachOps))
    {
        if ((true == enable_ThermalMonitor) || (0x0 != g_check_sys_info_flag))
            mtk_sysinfo_get_info();
    }
#endif
    
#if MTK_THERMAL_MONITOR_MEASURE_GET_TEMP_OVERHEAD
    dur = _get_current_time_us() - t;
    if (dur > 10000) // over 10msec, log it
        THRML_ERROR_LOG("[.get_temp] tz: %s dur: %ld\n", thermal->type, dur);
#endif

    return ret;
}

/*
 * .get_mode wrapper: get the current mode (user/kernel) of the thermal zone.
 *  - "kernel" means thermal management is done in kernel.
 *  - "user" will prevent kernel thermal driver actions upon trip points
 */
static int mtk_thermal_wrapper_get_mode
(
    struct thermal_zone_device *thermal,
    enum thermal_device_mode *mode
)
{
    int ret = 0;
    struct thermal_zone_device_ops *ops;

    THRML_LOG("[.get_mode] tz: %s mode: %d \n", thermal->type, *mode);

    ops = getClientZoneOps(thermal);

    if(ops->get_mode)
        ret = ops->get_mode(thermal, mode);

    return ret;
}

/*
 *  .set_mode wrapper: set the mode (user/kernel) of the thermal zone.
 */
static int mtk_thermal_wrapper_set_mode
(
    struct thermal_zone_device *thermal,
    enum thermal_device_mode mode
)
{
    int ret = 0;
    struct thermal_zone_device_ops *ops;

    THRML_LOG("[.set_mode] tz: %s mode: %d \n", thermal->type, mode);

    ops = getClientZoneOps(thermal);

    if(ops->set_mode)
        ret = ops->set_mode(thermal, mode);

    return ret;
}

/*
 * .get_trip_type wrapper: get the type of certain trip point.
 */
static int mtk_thermal_wrapper_get_trip_type
(
    struct thermal_zone_device *thermal,
    int trip,
    enum thermal_trip_type *type
)
{
    int ret = 0;
    struct thermal_zone_device_ops *ops;

    ops = getClientZoneOps(thermal);

    if (ops->get_trip_type)
        ret = ops->get_trip_type(thermal, trip, type);

    THRML_LOG("[.get_trip_type] tz: %s trip: %d type: %d \n", thermal->type, trip, *type);

    return ret;
}

/*
 * .get_trip_temp wrapper: get the temperature above which the certain trip point
 *  will be fired.
 */
static int mtk_thermal_wrapper_get_trip_temp
(
    struct thermal_zone_device *thermal,
    int trip,
    unsigned long *temperature
)
{
    int ret = 0;
    struct thermal_zone_device_ops *ops;

    ops = getClientZoneOps(thermal);

    if (ops->get_trip_temp)
        ret = ops->get_trip_temp(thermal, trip, temperature);

    THRML_LOG("[.get_trip_temp] tz: %s trip: %d temp: %d \n", thermal->type, trip, *temperature);
    THRML_STORAGE_LOG(THRML_LOGGER_MSG_TRIP_POINT, get_trip_temp, thermal->type, trip, *temperature);

    return ret;
}

/*
 * .get_crit_temp wrapper:
 */
static int mtk_thermal_wrapper_get_crit_temp
(
    struct thermal_zone_device *thermal,
    unsigned long *temperature
)
{
    int ret = 0;
    struct thermal_zone_device_ops *ops;

    ops = getClientZoneOps(thermal);

    if (ops->get_crit_temp)
        ret = ops->get_crit_temp(thermal, temperature);

    THRML_LOG("[.get_crit_temp] tz: %s temp: %d \n", thermal->type, *temperature);

    return ret;
}

static int mtk_thermal_wrapper_notify
(
    struct thermal_zone_device *thermal,
    int trip,
    enum thermal_trip_type type
)
{
    int ret = 0;
    struct thermal_zone_device_ops *ops;

    ops = getClientZoneOps(thermal);

    if (ops->notify)
        ret = ops->notify(thermal, trip, type);

    return ret;
}

#if defined(CONFIG_ARCH_MT6589)
static int tdpa_polling_delay = 0;
static int pa_polling_delay = 0;
static int mdm_signal_period = 0;

static void set_mdm_signal_period(void)
{
    int new_mdm_signal_period = 0;
    if ((0 == tdpa_polling_delay) && (0 == pa_polling_delay))
    {
    }
    else if (0 == tdpa_polling_delay)
    {
        new_mdm_signal_period = pa_polling_delay;
    }
    else if (0 == pa_polling_delay)
    {
        new_mdm_signal_period = tdpa_polling_delay;
    }
    else
    {
        new_mdm_signal_period = (pa_polling_delay <= tdpa_polling_delay) ? pa_polling_delay : tdpa_polling_delay;
    }

    if (new_mdm_signal_period != mdm_signal_period)
    {
        if (0 == new_mdm_signal_period)
        {
            mtk_mdm_stop_query();
        }
        else
        {
            if (0 == mdm_signal_period)
            {
                mtk_mdm_set_signal_period(new_mdm_signal_period);
                mtk_mdm_start_query();
            }
            else
            {
                mtk_mdm_set_signal_period(new_mdm_signal_period);
            }
        }
        mdm_signal_period = new_mdm_signal_period;
    }
}
#endif

//***************************************
// MTK thermal zone register/unregister
//***************************************

/* Wrapper callback OPS */
static struct thermal_zone_device_ops mtk_thermal_wrapper_dev_ops = {
    .bind = mtk_thermal_wrapper_bind,
    .unbind = mtk_thermal_wrapper_unbind,
    .get_temp = mtk_thermal_wrapper_get_temp,
    .get_mode = mtk_thermal_wrapper_get_mode,
    .set_mode = mtk_thermal_wrapper_set_mode,
    .get_trip_type = mtk_thermal_wrapper_get_trip_type,
    .get_trip_temp = mtk_thermal_wrapper_get_trip_temp,
    .get_crit_temp = mtk_thermal_wrapper_get_crit_temp,
    .notify = mtk_thermal_wrapper_notify,
};

/*mtk thermal zone register function */
struct thermal_zone_device *mtk_thermal_zone_device_register_wrapper
(
    char *type,
    int  trips,
    void *devdata,
    const struct thermal_zone_device_ops *ops,
    int tc1,
    int tc2,
    int passive_delay,
    int polling_delay
)
{
    struct thermal_zone_device *tz = NULL;
    struct mtk_thermal_tz_data *tzdata = NULL;
    THRML_LOG("[mtk_thermal_zone_device_register_wrapper] tz: %s trips: %d passive_delay: %d polling_delay: %d \n", type, trips, passive_delay, polling_delay);

    if (strcmp(SYSINFO_ATTACH_DEV_NAME, type) ==0)
    {
        g_SysinfoAttachOps = (int)ops;
    }

    tzdata = kzalloc(sizeof(struct mtk_thermal_tz_data), GFP_KERNEL);
    if (!tzdata)
    {
        THRML_ERROR_LOG("[%s] tzdata kzalloc fail.\n", __FUNCTION__);
        return ERR_PTR(-ENOMEM);
    }
    else
    {
        mutex_init(&tzdata->ma_lock);
        mutex_lock(&tzdata->ma_lock);
        tzdata->ops = (struct thermal_zone_device_ops *) ops;
        tzdata->ma_len = 1;
        tzdata->ma_counter = 0;
        tzdata->fake_temp = -275000; // init to -275000
#if (MAX_STEP_MA_LEN > 1)
        tzdata->curr_idx_ma_len = 0;
        tzdata->ma_lens[0] = 1;
        tzdata->msma_ht[0] = MSMA_MAX_HT;
#endif
        mb();
        mutex_unlock(&tzdata->ma_lock);
    }

    tz = thermal_zone_device_register(type,
                                      trips,                         ///< total number of trip points
                                      //(void*)ops,                  ///< invoker's ops pass to devdata
                                      (void*) tzdata, 
                                      &mtk_thermal_wrapper_dev_ops,  ///< use wrapper ops.
                                      tc1, tc2,
                                      passive_delay, polling_delay);

    // registered the last_temperature to local arra
    mutex_lock(&MTM_GET_TEMP_LOCK);
    {
        if (0 == strncmp(type, "mtktscpu", 8))
            tz_last_values[MTK_THERMAL_SENSOR_CPU] = &(tz->last_temperature);
        else if (0 == strncmp(type, "mtktsabb", 8))
            tz_last_values[MTK_THERMAL_SENSOR_ABB] = &(tz->last_temperature);
        else if (0 == strncmp(type, "mtktspmic", 9))
            tz_last_values[MTK_THERMAL_SENSOR_PMIC] = &(tz->last_temperature);
        else if (0 == strncmp(type, "mtktsbattery", 12))
            tz_last_values[MTK_THERMAL_SENSOR_BATTERY] = &(tz->last_temperature);
        else if (0 == strncmp(type, "mtktspa", 7))
            tz_last_values[MTK_THERMAL_SENSOR_MD1] = &(tz->last_temperature);
        else if (0 == strncmp(type, "mtktstdpa", 9))
            tz_last_values[MTK_THERMAL_SENSOR_MD2] = &(tz->last_temperature);
        else if (0 == strncmp(type, "mtktswmt", 8))
            tz_last_values[MTK_THERMAL_SENSOR_WIFI] = &(tz->last_temperature);
    }
    mutex_unlock(&MTM_GET_TEMP_LOCK);

#if defined(CONFIG_ARCH_MT6589)
    // config mtk_mdm timeout period based on PA or TDPA polling delay for better low power
    if (0 == strncmp(type, "mtktstdpa", 9))
    {
        tdpa_polling_delay = polling_delay/1000;
        set_mdm_signal_period();
    }
    if (0 == strncmp(type, "mtktspa", 7))
    {
        pa_polling_delay = polling_delay/1000;
        set_mdm_signal_period();
    }
#endif

    /* create a proc for this tz... */
    if (NULL != _get_proc_tz_dir_entry())
    {
        struct proc_dir_entry *entry = NULL;
        entry = create_proc_entry((const char*) type, S_IRUGO | S_IWUSR | S_IWGRP, proc_tz_dir_entry);
        if (NULL != entry)
        {
            entry->read_proc = _mtkthermal_tz_read;
            entry->write_proc = _mtkthermal_tz_write;
            entry->data = tz;
            entry->gid = 1000;
        }
        THRML_LOG("[mtk_thermal_zone_device_register_wrapper] proc file created: %d \n", entry->data);
    }

    /* This interface function adds a new thermal zone device */
    return tz;

}

/*mtk thermal zone unregister function */
void mtk_thermal_zone_device_unregister_wrapper
(
    struct thermal_zone_device *tz
)
{
    char type[32] = {0};
    struct mtk_thermal_tz_data *tzdata = NULL;

    strncpy(type, tz->type, 20);
    tzdata = (struct mtk_thermal_tz_data *) tz->devdata;

    // delete the proc file entry from proc
    if (NULL != proc_tz_dir_entry)
    {
        remove_proc_entry((const char *)type, proc_tz_dir_entry);
    }

    // unregistered the last_temperature from local array
    mutex_lock(&MTM_GET_TEMP_LOCK);
    {
        if (0 == strncmp(tz->type, "mtktscpu", 8))
            tz_last_values[MTK_THERMAL_SENSOR_CPU] = NULL;
        else if (0 == strncmp(tz->type, "mtktsabb", 8))
            tz_last_values[MTK_THERMAL_SENSOR_ABB] = NULL;
        else if (0 == strncmp(tz->type, "mtktspmic", 9))
            tz_last_values[MTK_THERMAL_SENSOR_PMIC] = NULL;
        else if (0 == strncmp(tz->type, "mtktsbattery", 12))
            tz_last_values[MTK_THERMAL_SENSOR_BATTERY] = NULL;
        else if (0 == strncmp(tz->type, "mtktspa", 7))
            tz_last_values[MTK_THERMAL_SENSOR_MD1] = NULL;
        else if (0 == strncmp(tz->type, "mtktstdpa", 9))
            tz_last_values[MTK_THERMAL_SENSOR_MD2] = NULL;
        else if (0 == strncmp(tz->type, "mtktswmt", 8))
            tz_last_values[MTK_THERMAL_SENSOR_WIFI] = NULL;
    }
    mutex_unlock(&MTM_GET_TEMP_LOCK);

    THRML_LOG("[mtk_thermal_zone_device_unregister]+ tz : %s\n", type);

    thermal_zone_device_unregister(tz);

    THRML_LOG("[mtk_thermal_zone_device_unregister]- tz: %s\n", type);

    // free memory
    if (NULL != tzdata)
    {
        mutex_destroy(&tzdata->ma_lock);
        kfree(tzdata);
    }
}

int mtk_thermal_zone_bind_cooling_device_wrapper
(
    struct thermal_zone_device *thermal,
    int trip,
    struct thermal_cooling_device *cdev
)
{
    struct mtk_thermal_cooler_data *mcdata;
    int ret = 0;

    THRML_LOG("[mtk_thermal_zone_bind_cooling_device_wrapper] thermal_type:%s trip:%d cdev_type:%s  ret:%d\n",
                thermal->type, trip, cdev->type, ret);

    ret = thermal_zone_bind_cooling_device(thermal, trip, cdev);

    if (ret)
    {
       THRML_ERROR_LOG("thermal_zone_bind_cooling_device Fail. Code(%d)\n", ret);
    }
    else
    {
        // TODO: think of a way don't do this here...Or cannot rollback devdata in bind ops...
        /* Init mtk Cooler Data */
        mcdata = cdev->devdata;
        mcdata->trip = trip;
        mcdata->tz = thermal;
    }

    THRML_LOG("[mtk_thermal_zone_bind_cooling_device_wrapper] thermal_type:%s trip:%d cdev_type:%s  ret:%d\n",
                thermal->type, trip, cdev->type, ret);

    THRML_STORAGE_LOG(THRML_LOGGER_MSG_BIND, bind, thermal->type, trip, cdev->type);

    return ret;
}


//*********************************************
// MTK cooling dev register/unregister
//*********************************************

/* .get_max_state */
static int mtk_cooling_wrapper_get_max_state
(
    struct thermal_cooling_device *cdev,
    unsigned long *state
)
{
    int ret=0;
    struct thermal_cooling_device_ops *ops;
    struct mtk_thermal_cooler_data *mcdata;

    mutex_lock(&MTM_COOLER_LOCK);

    /* Recovery client's devdata */
    ops = recoveryClientCooler(cdev, &mcdata);

    if(ops->get_max_state)
        ret =ops->get_max_state(cdev, state);

    THRML_LOG("[.get_max_state] cdev_type:%s state:%d \n", cdev->type, *state);

    cdev->devdata = mcdata;

    mutex_unlock(&MTM_COOLER_LOCK);

    return ret;
}

/* .get_cur_state */
static int mtk_cooling_wrapper_get_cur_state
(
    struct thermal_cooling_device *cdev,
    unsigned long *state
)
{
    int ret=0;
    struct thermal_cooling_device_ops *ops;
    struct mtk_thermal_cooler_data *mcdata;

    mutex_lock(&MTM_COOLER_LOCK);

    /* Recovery client's devdata */
    ops = recoveryClientCooler(cdev, &mcdata);

    if(ops->get_cur_state)
        ret = ops->get_cur_state(cdev, state);

    THRML_LOG("[.get_cur_state] cdev_type:%s state:%d \n", cdev->type, *state);

    /* reset devdata to mcdata */
    cdev->devdata = mcdata;

    mutex_unlock(&MTM_COOLER_LOCK);

    return ret;
}

/* set_cur_state */
static int mtk_cooling_wrapper_set_cur_state
(
    struct thermal_cooling_device *cdev,
    unsigned long state
)
{
    struct thermal_cooling_device_ops *ops;
    struct mtk_thermal_cooler_data *mcdata;
    int ret = 0;

    mutex_lock(&MTM_COOLER_LOCK);

    /* Recovery client's devdata */
    ops = recoveryClientCooler(cdev, &mcdata);

// check conditions
#if MTK_THERMAL_MONITOR_CONDITIONAL_COOLING
    if (0 != state)
    {
        // here check conditions for setting the cooler...
        if (MTK_THERMAL_MONITOR_COOLER_MAX_EXTRA_CONDITIONS == _mtkthermal_check_cooler_conditions(mcdata))
        {
            // pass
        }
        else
        {
            THRML_LOG("[.set_cur_state]condition check failed tz_type:%s cdev_type:%s trip:%d state:%d \n",
                               mcdata->tz->type, cdev->type, mcdata->trip, state);
            state = 0;
        }
    }

    
    if (0 == state)
    {
        unsigned long cur_state = 0;
        int last_temp = 0;
        unsigned long trip_temp = 0;

        if ((0 < mcdata->exit_threshold) && (mcdata->tz != NULL)) // if exit point is set and if this cooler is still bound...
        {
            if (ops->get_cur_state)
                ret = ops->get_cur_state(cdev, &cur_state);

            THRML_LOG("[.set_cur_state] cur_state:%d \n", cur_state);

            if (0 < cur_state)
            {
                THRML_LOG("[.set_cur_state] tz:%x devdata:%x\n", mcdata->tz, mcdata->tz->devdata);

                if (mcdata->tz)
                    last_temp = mcdata->tz->last_temperature;

                THRML_LOG("[.set_cur_state] last_temp:%d\n", last_temp);

                // TODO: restore...
                {
                    struct thermal_zone_device_ops *tz_ops = getClientZoneOps(mcdata->tz);
                    if (tz_ops->get_trip_temp)
                    {
                        tz_ops->get_trip_temp(mcdata->tz, mcdata->trip, &trip_temp);
                        THRML_LOG("[.set_cur_state] trip_temp:%d\n", trip_temp);
                    }
                    else
                    {
                        BUG();
                    }
                }

                if ((last_temp >= (int) trip_temp) || (((int) trip_temp - last_temp) < mcdata->exit_threshold))
                {
                    THRML_LOG("[.set_cur_state]not exit yet tz_type:%s cdev_type:%s trip:%d state:%d \n",
                                mcdata->tz->type, cdev->type, mcdata->trip, state);
                    state = cur_state;
                }
            }
        }
    }
#endif

    THRML_LOG("[.set_cur_state] tz_type:%s cdev_type:%s trip:%d state:%d \n",
                               mcdata->tz->type, cdev->type, mcdata->trip, state);
    THRML_STORAGE_LOG(THRML_LOGGER_MSG_COOL_STAE, set_cur_state, mcdata->tz->type, mcdata->trip, cdev->type, state);

    if(ops->set_cur_state)
        ret =ops->set_cur_state(cdev, state);

    /* reset devdata to mcdata */
    cdev->devdata = mcdata;

    mutex_unlock(&MTM_COOLER_LOCK);

    return ret;
}

/* Cooling callbacks OPS */
static struct thermal_cooling_device_ops mtk_cooling_wrapper_dev_ops = {
    .get_max_state = mtk_cooling_wrapper_get_max_state,
    .get_cur_state = mtk_cooling_wrapper_get_cur_state,
    .set_cur_state = mtk_cooling_wrapper_set_cur_state,
};

/*
 * MTK Cooling Register
 */
struct thermal_cooling_device *mtk_thermal_cooling_device_register_wrapper
(
     char *type,
     void *devdata,
     const struct thermal_cooling_device_ops *ops
)
{
    struct mtk_thermal_cooler_data *mcdata = NULL;
    struct thermal_cooling_device *ret = NULL;

    THRML_LOG("[mtk_thermal_cooling_device_register] type:%s \n", type);

    mcdata = kzalloc(sizeof(struct mtk_thermal_cooler_data), GFP_KERNEL);
    if (!mcdata)
    {
        THRML_ERROR_LOG("[%s] mcdata kzalloc fail.\n",__FUNCTION__);
        return ERR_PTR(-ENOMEM);
    }
    else
    {
        int i = 0;

        mcdata->ops = (struct thermal_cooling_device_ops *)ops;
        mcdata->devdata = devdata;
        mcdata->exit_threshold = 0;

        for (; i < MTK_THERMAL_MONITOR_COOLER_MAX_EXTRA_CONDITIONS; i++)
        {
            mcdata->conditions[i][0] = 0x0;
            mcdata->condition_last_value[i] = NULL;
            mcdata->threshold[i] = 0;
        }
        mb();
    }

    /* create a proc for this cooler... */
    if (NULL != _get_proc_cooler_dir_entry())
    {
        struct proc_dir_entry *entry = NULL;
        entry = create_proc_entry((const char*) type, S_IRUGO | S_IWUSR | S_IWGRP, proc_cooler_dir_entry);
        if (NULL != entry)
        {
            entry->read_proc = _mtkthermal_cooler_read;
            entry->write_proc = _mtkthermal_cooler_write;
            entry->data = mcdata;
            entry->gid = 1000;
        }
        THRML_LOG("[mtk_thermal_cooling_device_register] proc file created: %d \n", entry->data);
    }

    ret = thermal_cooling_device_register(type, mcdata, &mtk_cooling_wrapper_dev_ops);
    mcdata->id = ret->id; // Used for CPU usage flag...
    return ret;
}

/*
 * MTK Cooling Unregister
 */
void mtk_thermal_cooling_device_unregister_wrapper
(
    struct thermal_cooling_device *cdev
)
{
    struct mtk_thermal_cooler_data *mcdata;
    char type[32] = {0};

    strncpy(type, cdev->type, 20);

    THRML_LOG("[mtk_thermal_cooling_device_unregister]+ cdev:0x%x devdata:0x%x cdev:%s \n", cdev, cdev->devdata, type);

    mutex_lock(&MTM_COOLER_LOCK);

    // delete the proc file entry from proc
    if (NULL != proc_cooler_dir_entry)
    {
        remove_proc_entry((const char *)type, proc_cooler_dir_entry);
    }
    // TODO: consider error handling...

    mcdata = cdev->devdata;

    //THRML_LOG("[mtk_thermal_cooling_device_unregister] cdev:0x%x devdata:0x%x cdev:%s \n", cdev, cdev->devdata, cdev->type);
    THRML_LOG("[mtk_thermal_cooling_device_unregister]~ mcdata:0x%x\n", mcdata);

    thermal_cooling_device_unregister(cdev);

    /* free mtk cooler data */
    if(mcdata)
    {
        kfree(mcdata);
    }
    mutex_unlock(&MTM_COOLER_LOCK);

    THRML_LOG("[mtk_thermal_cooling_device_unregister]- cdev: %s \n", type);
}
#if defined(CONFIG_6589_THERMAL)
int mtk_thermal_zone_bind_trigger_trip(struct thermal_zone_device *tz, int trip, int mode)
{
    printk(KERN_DEBUG"hank mtk_thermal_zone_bind_trigger_trip %d", trip);
    THRML_LOG("hank mtk_thermal_zone_bind_trigger_trip \n");
    schedule_delayed_work(&(tz->poll_queue), 0);
    return 0;
}
#endif

int mtk_thermal_get_temp(MTK_THERMAL_SENSOR_ID id)
{
    if (id < 0 || id >= MTK_THERMAL_SENSOR_COUNT)
        return -127000;
    else
    {
        mutex_lock(&MTM_GET_TEMP_LOCK);
        if (tz_last_values[id] == NULL)
        {
            mutex_unlock(&MTM_GET_TEMP_LOCK);
            return -127000;
        }
        else
        {
            int ret = *tz_last_values[id];
            mutex_unlock(&MTM_GET_TEMP_LOCK);
            return ret;
        }
    }
}

//*********************************************
// Export Interface
//*********************************************

EXPORT_SYMBOL(mtk_thermal_zone_device_register_wrapper);
EXPORT_SYMBOL(mtk_thermal_zone_device_unregister_wrapper);
EXPORT_SYMBOL(mtk_thermal_cooling_device_unregister_wrapper);
EXPORT_SYMBOL(mtk_thermal_cooling_device_register_wrapper);
EXPORT_SYMBOL(mtk_thermal_zone_bind_cooling_device_wrapper);
#if defined(CONFIG_6589_THERMAL)
EXPORT_SYMBOL(mtk_thermal_zone_bind_trigger_trip);
#endif
EXPORT_SYMBOL(mtk_thermal_get_temp);
module_init(mtkthermal_init);
module_exit(mtkthermal_exit);

#if defined(CONFIG_MTK_THERMAL_TIME_BASE_PROTECTION)
late_initcall(mtkthermal_late_init);
#endif

