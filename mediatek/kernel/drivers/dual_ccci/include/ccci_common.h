/*****************************************************************************
 *
 * Filename:
 * ---------
 *   ccci_common.h
 *
 * Project:
 * --------
 *   
 *
 * Description:
 * ------------
 *
 * Author:
 * -------
 *   
 *
 ****************************************************************************/

#ifndef __CCCI_COMMON_H__
#define __CCCI_COMMON_H__
#include <linux/xlog.h>
#include <ccci_cfg.h>
#include <ccci_platform_cfg.h>
#include <ccci_err_no.h>
#include <ccci.h>
#include <ccci_md.h>
#include <ccci_layer.h>
#include <mach/mtk_ccci_helper.h>


//==================================================================================
// debug log define    
//==================================================================================
/*---------------------------alway on log-----------------------------------*/
#ifdef USING_PRINTK_LOG

#define CCCI_MSG(fmt, args...)	printk("[ccci]" fmt, ##args)
#define CCCI_RAW		printk
//#define CCCI_DEBUG_MSG(format, ...) printk("CCCI :" format, __VA_ARGS__)

#else
#if 0
#define CCCI_MSG(fmt, args...)		xlog_printk(ANDROID_LOG_ERROR, "ccci/com", "(0)"fmt, ##args)
#define CCCI_MSG_INF(idx, tag, fmt, args...)	xlog_printk(ANDROID_LOG_ERROR, "ccci/"tag, "(%d)"fmt, (idx+1), ##args)
#define CCCI_DBG_MSG(idx, tag, fmt, args...)	xlog_printk(ANDROID_LOG_VERBOSE, "ccci/"tag, "(%d)"fmt, idx, ##args)
#define CCCI_DBG_COM_MSG(fmt, args...)		xlog_printk(ANDROID_LOG_VERBOSE, "ccci/com", fmt, ##args)
#endif

#define CCCI_MSG(fmt, args...)		printk(KERN_ERR "[ccci/com] (0)" fmt, ##args)
#define CCCI_MSG_INF(idx, tag, fmt, args...)	printk(KERN_ERR "[ccci/" tag "] (%d)" fmt, (idx+1), ##args)
#define CCCI_DBG_MSG(idx, tag, fmt, args...)	printk(KERN_DEBUG "[ccci/" tag "] (%d)" fmt, (idx+1), ##args)
#define CCCI_DBG_COM_MSG(fmt, args...)		printk(KERN_DEBUG "[ccci/com] (0)" fmt, ##args)

#endif

/*---------------------------Switchable log--------------------------------*/
/* Debug message switch */
#define CCCI_DBG_NONE		(0x00000000)    /* No debug log */
#define CCCI_DBG_CTL		(0x00000001)    /* Control log */
#define CCCI_DBG_TTY		(0x00000002)    /* TTY channel log */
#define CCCI_DBG_FS			(0x00000004)    /* FS channel log */
#define CCCI_DBG_RPC		(0x00000008)    /* RPC channel log */
#define CCCI_DBG_IPC		(0x00000010)    /* IPC channel log */
#define CCCI_DBG_PMIC		(0x00000020)    /* PMIC channel log */
#define CCCI_DBG_CCMNI		(0x00000040)    /* CCMIN channel log */
#define CCCI_DBG_FUNC		(0x00000080)    /* Functiong entry log */
#define CCCI_DBG_MISC		(0x00000100)    /* Misc log */
#define CCCI_DBG_CHR		(0x00000200)	/* Char dev log */
#define CCCI_DBG_CCIF		(0x00000400)	/* Ccif log */
#define CCCI_DBG_ALL		(0xffffffff)

#define ENABLE_ALL_RX_LOG	(1ULL<<63)

/*---------------------------------------------------------------------------*/
/* Switchable messages */
extern unsigned int ccci_msg_mask[];

#ifdef USING_PRINTK_LOG

#define CCCI_FILTER_MSG(mask, fmt, args...) \
do {	\
	if ((CCCI_DBG_##mask) & ccci_msg_mask ) \
            printk("[ccci]" fmt , ##args); \
} while(0)

#define CCCI_CTL_MSG(fmt, args...)	CCCI_FILTER_MSG(CTL, "<ctl>"fmt, ##args)
#define CCCI_TTY_MSG(fmt, args...)	CCCI_FILTER_MSG(TTY, "<tty>"fmt, ##args)
#define CCCI_FS_MSG(fmt, args...)	CCCI_FILTER_MSG(FS, "<fs>"fmt, ##args)
#define CCCI_RPC_MSG(fmt, args...)	CCCI_FILTER_MSG(RPC, "<rpc>"fmt, ##args)
#define CCCI_IPC_MSG(fmt, args...)	CCCI_FILTER_MSG(IPC, "<ipc>"fmt, ##args)
#define CCCI_PMIC_MSG(fmt, args...)	CCCI_FILTER_MSG(PMIC, "<pmic>"fmt, ##args)
#define CCCI_FUNC_ENTRY(f)		CCCI_FILTER_MSG(FUNC, "%s\n", __FUNCTION__)
#define CCCI_MISC_MSG(fmt, args...)	CCCI_FILTER_MSG(MISC, fmt, ##args)
#define CCCI_CHR_MSG(fmt, args...)	CCCI_FILTER_MSG(CHR, "<chr>"fmt, ##args)
#define CCCI_CCIF_MSG(fmt, args...)	CCCI_FILTER_MSG(CCIF, "<chr>"fmt, ##args)
#define CCCI_CCMNI_MSG(fmt, args...)	CCCI_FILTER_MSG(CCMNI, "<ccmni>"fmt, ##args)

#else
#define CCCI_FILTER_MSG(mask, tag, idx, fmt, args...) \
do {	\
	if ((CCCI_DBG_##mask) & ccci_msg_mask[idx] ) \
            xlog_printk(ANDROID_LOG_ERROR, "ccci"tag, "(%d)"fmt , idx+1, ##args); \
} while(0)

#define CCCI_CTL_MSG(idx, fmt, args...)		CCCI_FILTER_MSG(CTL, "/ctl", idx, fmt, ##args)
#define CCCI_TTY_MSG(idx, fmt, args...)		CCCI_FILTER_MSG(TTY, "/tty", idx, fmt, ##args)
#define CCCI_FS_MSG(idx, fmt, args...)		CCCI_FILTER_MSG(FS, "/fs ", idx, fmt, ##args)
#define CCCI_RPC_MSG(idx, fmt, args...)		CCCI_FILTER_MSG(RPC, "/rpc", idx, fmt, ##args)
#define CCCI_IPC_MSG(idx, fmt, args...)		CCCI_FILTER_MSG(IPC, "/ipc", idx, fmt, ##args)
#define CCCI_PMIC_MSG(idx, fmt, args...)	CCCI_FILTER_MSG(PMIC, "/pmc", idx, fmt, ##args)
#define CCCI_FUNC_ENTRY(idx)				CCCI_FILTER_MSG(FUNC, "/fun", idx, "%s\n", __FUNCTION__)
#define CCCI_MISC_MSG(idx, fmt, args...)	CCCI_FILTER_MSG(MISC, "/mis", idx, fmt, ##args)
#define CCCI_CHR_MSG(idx, fmt, args...)		CCCI_FILTER_MSG(CHR, "/chr", idx, fmt, ##args)
#define CCCI_CCIF_MSG(idx, fmt, args...)	CCCI_FILTER_MSG(CCIF, "/cci", idx, fmt, ##args)
#define CCCI_CCMNI_MSG(idx, fmt, args...)	CCCI_FILTER_MSG(CCMNI, "/net", idx, fmt, ##args)
#endif



//==================================================================================
// AEE function and macro define  
//==================================================================================
#define CCCI_AED_DUMP_EX_MEM		(0x00000001)
#define CCCI_AED_DUMP_MD_IMG_MEM	(0x00000002)

void ccci_aed(int, unsigned int, char *);



//==================================================================================
// ccci related macro and structure define  
//==================================================================================
#define CAN_BE_RELOAD		(0x1<<1)
#define LOAD_ONE_TIME		(0x1<<0)
#define LOAD_ALL_IMG		(LOAD_ONE_TIME|CAN_BE_RELOAD)
#define RELOAD_ONLY			(CAN_BE_RELOAD)

#define CCCI_LOG_TX 0
#define CCCI_LOG_RX 1


enum {
	MD_DEBUG_REL_INFO_NOT_READY = 0,
	MD_IS_DEBUG_VERSION,
	MD_IS_RELEASE_VERSION
};

typedef enum _ccif_type
{
	CCIF_STD_V1=0,	// 16 channel ccif, tx 8, rx 8
	CCIF_VIR,		// Virtual CCIF type
}ccif_type_t;

typedef struct _ccif_hw_info
{
	unsigned long	reg_base;
	unsigned int	irq_id;
	unsigned int	irq_attr;
	ccif_type_t		type;
	unsigned int	md_id;
}ccif_hw_info_t;

typedef struct _rpc_cfg_inf
{
	int rpc_ch_num;
	int rpc_max_buf_size;
}rpc_cfg_inf_t;



//==================================================================================
// share memory layout define  
//==================================================================================
//share memory table
typedef struct _smem_alloc
{
	// Share memory
	unsigned int		ccci_smem_size;
	unsigned int		ccci_smem_vir;
	unsigned int		ccci_smem_phy;
	// -- Log
	unsigned int		ccci_mdlog_smem_base_virt;
	unsigned int		ccci_mdlog_smem_base_phy;
	unsigned int		ccci_mdlog_smem_size;
	// -- PCM
	unsigned int		ccci_pcm_smem_base_virt;
	unsigned int		ccci_pcm_smem_base_phy;
	unsigned int		ccci_pcm_smem_size;
	// -- PMIC
	unsigned int		ccci_pmic_smem_base_virt;
	unsigned int		ccci_pmic_smem_base_phy;
	unsigned int		ccci_pmic_smem_size;
	// -- FS
	unsigned int		ccci_fs_smem_base_virt;
	unsigned int		ccci_fs_smem_base_phy;
	unsigned int		ccci_fs_smem_size;
	// -- RPC
	unsigned int		ccci_rpc_smem_base_virt;
	unsigned int		ccci_rpc_smem_base_phy;
	unsigned int		ccci_rpc_smem_size;
	// -- TTY
	unsigned int		ccci_tty_smem_base_virt[CCCI_TTY_BUFF_NR];
	unsigned int		ccci_tty_smem_base_phy[CCCI_TTY_BUFF_NR];
	unsigned int		ccci_tty_smem_size[CCCI_TTY_BUFF_NR];
	// -- Exception
	unsigned int		ccci_exp_smem_base_virt;
	unsigned int		ccci_exp_smem_base_phy;
	unsigned int		ccci_exp_smem_size;
	// -- IPC
	unsigned int		ccci_ipc_smem_base_virt;
	unsigned int		ccci_ipc_smem_base_phy;
	unsigned int		ccci_ipc_smem_size;
	// -- SYS - Eint exchagne
	unsigned int		ccci_sys_smem_base_virt;
	unsigned int		ccci_sys_smem_base_phy;
	unsigned int		ccci_sys_smem_size;
	// -- CCMNI new version
	// ----- Up-link
	unsigned int		ccci_ccmni_smem_ul_base_virt;
	unsigned int		ccci_ccmni_smem_ul_base_phy;
	unsigned int		ccci_ccmni_smem_ul_size;
	// ----- Donw-link
	unsigned int		ccci_ccmni_smem_dl_base_virt;
	unsigned int		ccci_ccmni_smem_dl_base_phy;
	unsigned int		ccci_ccmni_smem_dl_size;
	unsigned int		ccci_ccmni_ctl_smem_base_virt[NET_PORT_NUM];
	unsigned int		ccci_ccmni_ctl_smem_base_phy[NET_PORT_NUM];
	unsigned int		ccci_ccmni_ctl_smem_size[NET_PORT_NUM];
	// -- EXT MD Exception
	unsigned int		ccci_md_ex_exp_info_smem_base_virt;
	unsigned int		ccci_md_ex_exp_info_smem_base_phy;
	unsigned int		ccci_md_ex_exp_info_smem_size;
	// -- MD Runtime Data
	unsigned int		ccci_md_runtime_data_smem_base_virt;
	unsigned int		ccci_md_runtime_data_smem_base_phy;
	unsigned int		ccci_md_runtime_data_smem_size;
	// -- Misc Info Data
	unsigned int		ccci_misc_info_base_virt;
	unsigned int		ccci_misc_info_base_phy;
	unsigned int		ccci_misc_info_size;
}smem_alloc_t;

// Memory layout table
typedef struct _ccci_mem_layout
{
	// MD image
	unsigned int		md_region_vir;
	unsigned int		md_region_phy;
	unsigned int		md_region_size;
	// DSP image
	unsigned int		dsp_region_vir;
	unsigned int		dsp_region_phy;
	unsigned int		dsp_region_size;
	// Share memory
	unsigned int		smem_region_vir;
	unsigned int		smem_region_phy;
	unsigned int		smem_region_size;
	unsigned int		smem_region_phy_before_map;
}ccci_mem_layout_t;


// Misc info structure
typedef struct _misc_info
{
	unsigned int prefix;		// "CCIF"
	unsigned int support_mask;
	unsigned int index;
	unsigned int next;
	unsigned int feature_0_val[4];
	unsigned int feature_1_val[4];
	unsigned int feature_2_val[4];
	unsigned int feature_3_val[4];
	unsigned int feature_4_val[4];
	unsigned int feature_5_val[4];
	unsigned int feature_6_val[4];
	unsigned int feature_7_val[4];
	unsigned int feature_8_val[4];
	unsigned int feature_9_val[4];
	unsigned int feature_10_val[4];
	unsigned int feature_11_val[4];
	unsigned int feature_12_val[4];
	unsigned int feature_13_val[4];
	unsigned int feature_14_val[4];
	unsigned int feature_15_val[4];
	unsigned int reserved_2[3];
	unsigned int postfix;		// "CCIF"
} misc_info_t;

typedef enum
{
	FEATURE_NOT_EXIST = 0,
	FEATURE_NOT_SUPPORT,
	FEATURE_SUPPORT,
	FEATURE_PARTIALLY_SUPPORT,
} misc_feature_sta_t; 

typedef enum
{
	MISC_DMA_ADDR = 0,
	MISC_32K_LESS,
} misc_feature_id_t;



//==================================================================================
// API functions exported by ccci platform
//==================================================================================
int					get_dev_major_for_md_sys(int md_id);
int					get_ccif_hw_info(int md_id, ccif_hw_info_t *ccif_hw_info);
void				md_env_setup_before_boot(int md_id);
void        		md_env_setup_before_ready(int md_id);
void				md_boot_up_additional_operation(int md_id);
void				md_boot_ready_additional_operation(int md_id);
void				additional_operation_before_stop_md(int md_id);
smem_alloc_t*		get_md_smem_layout(int md_id);
unsigned int		get_md_sys_max_num(void);
void				enable_mem_access_protection(int md_id);
int					ccci_load_firmware(int md_id, unsigned int load_flag, char img_err_str[], int len);
int					ccci_power_on_md(int md_id);
int					ccci_power_down_md(int md_id);
int					let_md_stop(int md_id, unsigned int timeout);
int					let_md_go(int md_id);
int					ccci_get_sub_module_cfg(int md_id, char name[], char out_buf[], int size);
unsigned int		is_md_enable(int md_id);
int					ccci_alloc_smem(int md_id);
void				ccci_free_smem(int md_id);
ccci_mem_layout_t*	get_md_sys_layout(int md_id);
int					is_modem_debug_ver(int md_id);
char*				get_md_info_str(int md_id);
void				platform_set_runtime_data(int md_id, modem_runtime_t *runtime);
void 				config_misc_info(int md_id, unsigned int base[], unsigned int size);
void                send_battery_info(int md_id);
void                md_fast_dormancy(int md_id);
void				start_md_wdt_recov_timer(int md_id);


//==================================================================================
// API functions exported by ccci source
//==================================================================================
int                 platform_init(int md_id, int power_down);
void                platform_deinit(int md_id);
int                 ccci_md_ctrl_init(int md_id);
void                ccci_md_ctrl_exit(int md_id);
int                 ccci_chrdev_init(int md_id);
void                ccci_chrdev_exit(int md_id);
int                 ccci_tty_init(int md_id);
void                ccci_tty_exit(int md_id);
int                 ccci_ipc_init(int md_id);
void                ccci_ipc_exit(int md_id);
int                 ccci_rpc_init(int md_id);
void                ccci_rpc_exit(int md_id);
int                 ccci_fs_init(int md_id);
void                ccci_fs_exit(int md_id);
int                 ccmni_init(int md_id);
void                ccmni_exit(int md_id);
int                 ccci_vir_chrdev_init(int md_id);
void                ccci_vir_chrdev_exit(int md_id);
int                 init_ccci_dev_node(void);
int					statistics_init(int md_id);
void				statistics_exit(int md_id);


int					get_dev_id_by_md_id(int md_id, char node_name[], int *major, int* minor);
int					get_md_id_by_dev_major(int dev_major);
int					init_ccci_dev_node(void);
int					send_md_reset_notify(int);
int					ccci_trigger_md_assert(int);
int					get_md_exception_type(int md_id);
int					send_md_stop_notify(int md_id);
int					send_md_start_notify(int md_id);
int					ccci_start_modem(int md_id);
int					ccci_stop_modem(int md_id, unsigned int timeout);
int					ccci_send_run_time_data(int md_id);
int					statistics_init_ch_dir(int md_sys_id, int ch, int dir);
void				dump_logical_layer_tx_rx_histroy(int md_id);
void				logic_layer_ch_record_dump(int md_id, int ch);
void				add_logic_layer_record(int md_id, ccci_msg_t *data, int drop);
void				ccci_dump_logic_layer_info(int md_id, int buf[], int len);
void				ccci_dump_hw_reg_val(int md_id, int buf[], int len);
int					send_enter_flight_mode_request(int md_id);
int					send_leave_flight_mode_request(int md_id);
int					send_power_on_md_request(int md_id);
int					send_power_down_md_request(int md_id);
int					mk_ccci_dev_node(int md_id);
int					ccci_md_ctrl_common_init(void);
int					bind_to_low_layer_notify(int md_id, void (*isr_func)(int), void (*send_func)(int, unsigned int));
ccif_t*             ccif_create_instance(ccif_hw_info_t *info, void* ctl_b, int md_id);



//==================================================================================
// API functions for IPO-H
//==================================================================================
int					ccci_uart_ipo_h_restore(int md_id);
int					ccci_ipc_ipo_h_restore(int md_id);
int					ccmni_ipo_h_restore(int md_id);
int					ccci_ipo_h_platform_restore(int md_id);

int					ccci_ipo_h_restore(int md_id, char buf[], unsigned int len);


//==================================================================================
// IOCTL commands
//==================================================================================

#define CCCI_IOC_MAGIC 'C'
#define CCCI_IOC_MD_RESET				_IO(CCCI_IOC_MAGIC, 0)
#define CCCI_IOC_GET_MD_STATE			_IOR(CCCI_IOC_MAGIC, 1, unsigned int)
#define CCCI_IOC_PCM_BASE_ADDR			_IOR(CCCI_IOC_MAGIC, 2, unsigned int)
#define CCCI_IOC_PCM_LEN				_IOR(CCCI_IOC_MAGIC, 3, unsigned int)
#define CCCI_IOC_FORCE_MD_ASSERT		_IO(CCCI_IOC_MAGIC, 4)
#define CCCI_IOC_ALLOC_MD_LOG_MEM		_IO(CCCI_IOC_MAGIC, 5)
#define CCCI_IOC_DO_MD_RST				_IO(CCCI_IOC_MAGIC, 6)
#define CCCI_IOC_SEND_RUN_TIME_DATA		_IO(CCCI_IOC_MAGIC, 7)
#define CCCI_IOC_GET_MD_INFO			_IOR(CCCI_IOC_MAGIC, 8, unsigned int)
#define CCCI_IOC_GET_MD_EX_TYPE			_IOR(CCCI_IOC_MAGIC, 9, unsigned int)
#define CCCI_IOC_SEND_STOP_MD_REQUEST	_IO(CCCI_IOC_MAGIC, 10)
#define CCCI_IOC_SEND_START_MD_REQUEST	_IO(CCCI_IOC_MAGIC, 11)
#define CCCI_IOC_DO_STOP_MD				_IO(CCCI_IOC_MAGIC, 12)
#define CCCI_IOC_DO_START_MD			_IO(CCCI_IOC_MAGIC, 13)
#define CCCI_IOC_ENTER_DEEP_FLIGHT		_IO(CCCI_IOC_MAGIC, 14)
#define CCCI_IOC_LEAVE_DEEP_FLIGHT		_IO(CCCI_IOC_MAGIC, 15)
#define CCCI_IOC_POWER_ON_MD			_IO(CCCI_IOC_MAGIC, 16)
#define CCCI_IOC_POWER_OFF_MD			_IO(CCCI_IOC_MAGIC, 17)
#define CCCI_IOC_POWER_ON_MD_REQUEST	_IO(CCCI_IOC_MAGIC, 18)
#define CCCI_IOC_POWER_OFF_MD_REQUEST	_IO(CCCI_IOC_MAGIC, 19)
#define CCCI_IOC_SIM_SWITCH			    _IOW(CCCI_IOC_MAGIC, 20, unsigned int)
#define CCCI_IOC_SEND_BATTERY_INFO      _IO(CCCI_IOC_MAGIC, 21)


#endif
