

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
#include <asm/string.h>
#include <linux/spinlock.h>

#include <mach/system.h>

#include "mach/mtk_thermal_monitor.h"
#include "mach/mtk_mdm_monitor.h"

#if  defined(MTK_ENABLE_MD1) || defined(MTK_ENABLE_MD2)
    #include <mach/mtk_ccci_helper.h>
#endif

#define mtk_mdm_debug_log 1
#define mtk_mdm_dprintk(fmt, args...)   \
do {                                    \
    if (mtk_mdm_debug_log) {                \
        xlog_printk(ANDROID_LOG_INFO, "MDM_TxPower/PA_Thermal", fmt, ##args); \
    }                                   \
} while(0)

#define DEFINE_MDM_CB(index)	\
    static int fill_mdm_cb_##index(int md_id, int data)	\
    {	\
        if(data>500)	\
            data = -32767;	\
        g_pinfo_list[index].value = data*1000;	\
        return 0;	\
    }	

#define MDM_CB(index)	(fill_mdm_cb_##index)

#define MAX_LEN	256
#define ID_REG_TXPOWER_CB  	(MD_TX_POWER)
#define ID_REG_RFTEMP_CB	(MD_RF_TEMPERATURE)
#define ID_REG_RFTEMP_3G_CB	(MD_RF_TEMPERATURE_3G)
#define MTK_THERMAL_GET_TX_POWER	0
#define MTK_THERMAL_GET_RF_TEMP_2G 	1
#define MTK_THERMAL_GET_RF_TEMP_3G 	2

static bool mdm_sw = false;
static struct timer_list txpwr_timer;
static int mtk_mdm_enable(void);
static int mtk_mdm_disable(void);
extern bool is_meta_mode(void);
extern bool is_advanced_meta_mode(void);
static int signal_period = 60; // 1s

struct md_info g_pinfo_list[] = 
{{"TXPWR_MD1", -127, "db", -127, 0}, 
 {"TXPWR_MD2", -127, "db", -127, 1},
 {"RFTEMP_2G_MD1", -32767000, "m�XC", -32767000, 2},
 {"RFTEMP_2G_MD2", -32767000, "m�XC", -32767000, 3},
 {"RFTEMP_3G_MD1", -32767000, "m�XC", -32767000, 4},
 {"RFTEMP_3G_MD2", -32767000, "m�XC", -32767000, 5}};

#ifdef MTK_ENABLE_MD1
DEFINE_MDM_CB(0)
DEFINE_MDM_CB(2)					
DEFINE_MDM_CB(4)
#endif
#ifdef MTK_ENABLE_MD2
DEFINE_MDM_CB(1)
DEFINE_MDM_CB(3)
DEFINE_MDM_CB(5)
#endif

int mtk_mdm_get_tx_power(void)
{
    return 0;
}
EXPORT_SYMBOL(mtk_mdm_get_tx_power);

int mtk_mdm_get_rf_temp(void)
{
    return 0;
}
EXPORT_SYMBOL(mtk_mdm_get_rf_temp);

int mtk_mdm_get_md_info(struct md_info** p_inf, int *size)
{
    *p_inf = g_pinfo_list;
    *size = sizeof(g_pinfo_list)/sizeof(g_pinfo_list[0]);
    return 0;
}
EXPORT_SYMBOL(mtk_mdm_get_md_info);

int mtk_mdm_start_query(void)
{
#if  defined(MTK_ENABLE_MD1) || defined(MTK_ENABLE_MD2)
    mdm_sw = true;
    mtk_mdm_enable();
#endif
    return 0;
}
EXPORT_SYMBOL(mtk_mdm_start_query);

int mtk_mdm_stop_query(void)
{
    mdm_sw = false;
    mtk_mdm_disable();
    return 0;
}
EXPORT_SYMBOL(mtk_mdm_stop_query);

int mtk_mdm_set_signal_period(int second)
{
    signal_period = second;
    return 0;
}
EXPORT_SYMBOL(mtk_mdm_set_signal_period);

static int send_get_md_all_msg(void)
{
	char mode[1];
	if(!(is_meta_mode() | is_advanced_meta_mode())){
	mode[0] = MTK_THERMAL_GET_TX_POWER;
#ifdef  MTK_ENABLE_MD1
	exec_ccci_kern_func_by_md_id(0, ID_GET_TXPOWER, mode, 0);
#endif
#ifdef	MTK_ENABLE_MD2
	exec_ccci_kern_func_by_md_id(1, ID_GET_TXPOWER, mode, 0);
#endif
	}
	mode[0] = MTK_THERMAL_GET_RF_TEMP_2G;
#ifdef	MTK_ENABLE_MD1
	exec_ccci_kern_func_by_md_id(0, ID_GET_TXPOWER, mode, 0);
#endif
#ifdef  MTK_ENABLE_MD2
	exec_ccci_kern_func_by_md_id(1, ID_GET_TXPOWER, mode, 0);
#endif	
	mode[0] = MTK_THERMAL_GET_RF_TEMP_3G;
#ifdef	MTK_ENABLE_MD1
	exec_ccci_kern_func_by_md_id(0, ID_GET_TXPOWER, mode, 0);
#endif
#ifdef	MTK_ENABLE_MD2
	exec_ccci_kern_func_by_md_id(1, ID_GET_TXPOWER, mode, 0);
#endif	
	return 0;
}

static int mtk_stats_txpwr(unsigned long data)
{
    txpwr_timer.expires = jiffies + signal_period * HZ;
    send_get_md_all_msg();
    add_timer(&txpwr_timer);
    //mtk_mdm_dprintk("SEND GET_MDM_TXPWR_MSG");
    return 0;
}

static int mtk_mdm_enable(void)
{
    // Register the data send back function 
    // MD will receive the data by cb
#ifdef  MTK_ENABLE_MD1
    if(!(is_meta_mode() | is_advanced_meta_mode())){
        register_ccci_sys_call_back(0, ID_REG_TXPOWER_CB, MDM_CB(0));
    }
    register_ccci_sys_call_back(0, ID_REG_RFTEMP_CB, MDM_CB(2));	
    register_ccci_sys_call_back(0, ID_REG_RFTEMP_3G_CB, MDM_CB(4));
#endif
#ifdef  MTK_ENABLE_MD2
    if(!(is_meta_mode() | is_advanced_meta_mode())){
        register_ccci_sys_call_back(1, ID_REG_TXPOWER_CB, MDM_CB(1));
    }
    register_ccci_sys_call_back(1, ID_REG_RFTEMP_CB, MDM_CB(3));
    register_ccci_sys_call_back(1, ID_REG_RFTEMP_3G_CB, MDM_CB(5));
#endif
    init_timer(&txpwr_timer);
    txpwr_timer.function = (void *)&mtk_stats_txpwr;
    //txpwr_timer.data = (unsigned long) &wmt_stats_info;
    txpwr_timer.expires = jiffies + signal_period * HZ;
    add_timer(&txpwr_timer);
    mtk_mdm_dprintk("ENABLE MDM_TxPower Function\n");
    return 0;
}

static int mtk_mdm_disable(void)
{
    del_timer(&txpwr_timer);
    mtk_mdm_dprintk("DISABLE MDM_TxPower Function\n");
    return 0;
}

static int mtk_mdm_value_read(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
	
    int len = 0;
    struct md_info* p_md;
    int size;
    int i;
    char temp_buf[512];
		
    mtk_mdm_get_md_info(&p_md, &size);
    sprintf(temp_buf, "%s:%d %s\n", p_md[0].attribute, p_md[0].value, p_md[0].unit);
    strcpy(buf, temp_buf);
    for(i=1; i<size; i++){
        sprintf(temp_buf, "%s:%d %s\n", p_md[i].attribute, p_md[i].value, p_md[i].unit);
        strcat(buf, temp_buf);
    }
		
    len = strlen(buf);
    *start = buf + off;
		
    if (len > off)
        len -= off;
    else
        len = 0;
    return len < count ? len  : count;
}

static int mtk_mdm_sw_read(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;

    len += sprintf(buf, "%s\n", mdm_sw?"on":"off");
    *start = buf + off;

    if (len > off)
        len -= off;
    else
        len = 0;
    mtk_mdm_dprintk("[%s] %s", __func__, mdm_sw?"on":"off");
    return len < count ? len  : count;
}

static ssize_t mtk_mdm_sw_write(struct file *file, const char *buf, unsigned long len, void *data)
{
    
    char desc[MAX_LEN] = {0};
    char temp[MAX_LEN] = {0};
    len = (len < (sizeof(desc) - 1)) ? len : (sizeof(desc) - 1);

    /* write data to the buffer */
    if (copy_from_user(desc, buf, len)) {
        return -EFAULT;
    }

    if (sscanf(desc, "%s", temp) == 1) {
        if(strncmp(temp, "on", 2)==0 || strncmp(temp, "1", 1)==0){
            mdm_sw = true;
        } else if (strncmp(temp, "off", 3)==0 || strncmp(temp, "0", 1)==0){
            mdm_sw = false;
        } else {
        mtk_mdm_dprintk("[%s] bad argument:%s\n", __func__, temp);
        } 
                
        if(mdm_sw) {
            mtk_mdm_enable();
        } else {
            mtk_mdm_disable();
        }
                        
        return len;
    } else {
        mtk_mdm_dprintk("[%s] bad argument\n", __func__);
    }
    
    return -EINVAL;
}

static int mtk_mdm_proc_timeout_read(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;

    len += sprintf(buf, "%d\n", signal_period);
    *start = buf + off;

    if (len > off)
        len -= off;
    else
        len = 0;
    mtk_mdm_dprintk("[%s] %d", __func__, signal_period);
    return len < count ? len  : count;
}

static ssize_t mtk_mdm_proc_timeout_write(struct file *file, const char *buf, unsigned long len, void *data)
{
    
    char desc[MAX_LEN] = {0};
    int temp_value;
    len = (len < (sizeof(desc) - 1)) ? len : (sizeof(desc) - 1);

    /* write data to the buffer */
    if (copy_from_user(desc, buf, len)) {
        return -EFAULT;
    }

    if (sscanf(desc, "%d", &temp_value) == 1) {
        signal_period = temp_value;
        mtk_mdm_dprintk("[%s] Set Timeout:%d\n", __func__, temp_value);
        return len;
    } else {
        mtk_mdm_dprintk("[%s] bad argument\n", __func__);
    }
    
    return -EINVAL;
}


static int __init mtk_mdm_txpwr_init(void)
{
    struct proc_dir_entry *entry = NULL;
    struct proc_dir_entry *mdtxpwr_dir = NULL;

    mtk_mdm_dprintk("[mtk_mdm_txpwr_init] \n");
	
    mdtxpwr_dir = proc_mkdir("mtk_mdm_txpwr", NULL);
    if (!mdtxpwr_dir) {
        mtk_mdm_dprintk("[mtk_mdm_init]: mkdir /proc/mtk_mdm_txpwr failed\n");
    } else {
        entry = create_proc_entry("txpwr_sw", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, mdtxpwr_dir);
        if (entry) {
            entry->read_proc = mtk_mdm_sw_read;
            entry->write_proc = mtk_mdm_sw_write;
        }
        entry = create_proc_entry("txpwr_value", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, mdtxpwr_dir);
        if (entry) {
            entry->read_proc = mtk_mdm_value_read;
        }
        entry = create_proc_entry("timeout", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, mdtxpwr_dir);
        if (entry) {
            entry->read_proc = mtk_mdm_proc_timeout_read;
            entry->write_proc = mtk_mdm_proc_timeout_write;
        }
    }
    // Add for thermal all on scenary
    //mtk_mdm_start_query();

    return 0;
}

static void __exit mtk_mdm_txpwr_exit(void)
{
    mtk_mdm_dprintk("[mtk_mdm_txpwr_exit] \n");
}

module_init(mtk_mdm_txpwr_init);
module_exit(mtk_mdm_txpwr_exit);
