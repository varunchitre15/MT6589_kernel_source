#include <ccci.h>
#include <mach/emi_mpu.h>
#include <linux/delay.h>
#include <mach/sec_osal.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/kfifo.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/semaphore.h>
#include <linux/version.h>
#include <ccci_cfg.h>
#include <ccci_rpc.h>
#include <ccci_common.h>
#include <linux/fs.h>
#include <mach/mtk_rtc.h>
#include <mach/bus_fabric.h>


/* -------------ccci initial status define----------------------*/
#define CCCI_ALLOC_SMEM_DONE		(1<<0)
#define CCCI_MAP_MD_CODE_DONE		(1<<1)
#define CCCI_MAP_CTL_REG_DONE		(1<<2)
#define CCCI_WDT_IRQ_REG_DONE		(1<<3)
#define CCCI_SEC_INIT_DONE			(1<<4)
#define CCCI_SEC_CHECK_DONE			(1<<5)

static unsigned int md_init_stage_flag[MAX_MD_NUM];
static int	smem_remapped = 0;

/* -------------ccci memory layout define----------------------*/
extern ccci_mem_layout_t md_mem_layout_tab[];

int emi_mpu_set_region_protection(unsigned int start, unsigned int end, int region, unsigned int access_permission);


/* -------------ccci load md&dsp image define----------------*/
static char * product_str[] = {[INVALID_VARSION]=INVALID_STR, 
							   [DEBUG_VERSION]=DEBUG_STR, 
							   [RELEASE_VERSION]=RELEASE_STR};

static char * type_str[] = {[AP_IMG_INVALID]=VER_INVALID_STR, 
							[AP_IMG_2G]=VER_2G_STR, 
							[AP_IMG_3G]=VER_3G_STR};
static char					ap_platform[16]="";
static MD_CHECK_HEADER		md_sys_head[MAX_MD_NUM];
static GFH_CHECK_CFG_v1		dsp_sys_head[MAX_MD_NUM]; //<<<< Can be delete

static struct image_info    md_img_info[MAX_MD_NUM][2];

char md_img_info_str[MAX_MD_NUM][256];

static int	img_is_dbg_ver[MAX_MD_NUM];


//===============================================
// modem hardware control section
//===============================================
/* -------------physical&virtual address define-----------------*/
#define KERNEL_PHY_BASE             (0x80000000)

static unsigned int ap_infra_base;
static unsigned int ap_mcu_reg_base;

static unsigned int md1_rgu_base;
static unsigned int md1_ccif_base;
static unsigned int md1_boot_slave_Vector;
static unsigned int md1_boot_slave_Key;
static unsigned int md1_boot_slave_En;


static unsigned int md2_rgu_base;
static unsigned int md2_ccif_base;
static unsigned int md2_boot_slave_Vector;
static unsigned int md2_boot_slave_Key;
static unsigned int md2_boot_slave_En;

/* -------------md&dsp watchdog define----------------*/
typedef int (*int_func_int_t)(int);
static int_func_int_t	wdt_notify_array[MAX_MD_NUM];
static atomic_t			wdt_irq_en_count[MAX_MD_NUM];

/* -------------md gate&ungate function----------------*/


/* -------------power on/off md function----------------*/
static DEFINE_SPINLOCK(md1_power_change_lock);
static int		md1_power_on_cnt;
static DEFINE_SPINLOCK(md2_power_change_lock);
static int		md2_power_on_cnt;
static unsigned int	md2_hw_mode = 0;

/*--------------MD WDT recover ----------------------*/
static DEFINE_SPINLOCK(md1_wdt_mon_lock);
static DEFINE_SPINLOCK(md2_wdt_mon_lock);
static void recover_md_wdt_irq(unsigned long data);
static DEFINE_TIMER(md1_wdt_recover_timer,recover_md_wdt_irq,0,MD_SYS1);
static DEFINE_TIMER(md2_wdt_recover_timer,recover_md_wdt_irq,0,MD_SYS2);
static volatile unsigned int	md_wdt_has_processed[MAX_MD_NUM] = {0, 0};


extern int md_power_on(int);
extern int md_power_off(int,unsigned int);

/* -------------ccci log filter define---------------------*/
unsigned int ccci_msg_mask[MAX_MD_NUM] = {0x0, 0x0};
EXPORT_SYMBOL(ccci_msg_mask);




/*********************************************************************************/
/*  API about getting md information                                                                                   */
/*                                                                                                                                   */
/*********************************************************************************/
static unsigned int get_chip_version(void)
{
	#ifdef ENABLE_CHIP_VER_CHECK
	if(get_chip_sw_ver() == MD_SW_V1)
		return CHIP_SW_VER_01;
	else if(get_chip_sw_ver() == MD_SW_V2)
		return CHIP_SW_VER_02;
	else
		return CHIP_SW_VER_02; // For Fix warning ,should not enter here
	#else
	return CHIP_SW_VER_01;
	#endif
}

static void ccci_get_platform_ver(char * ver)
{
	#ifdef ENABLE_CHIP_VER_CHECK
	sprintf(ver, "MT%04x_S%02x", get_chip_code(), (get_chip_sw_ver_code()&0xFF));
	#else
	sprintf(ver, "MT0000_S00");
	#endif
}


int is_modem_debug_ver(int md_id)
{
	return img_is_dbg_ver[md_id];
}
EXPORT_SYMBOL(is_modem_debug_ver);


char * get_md_info_str(int md_id)
{
	return md_img_info_str[md_id];
}
EXPORT_SYMBOL(get_md_info_str);


int get_ccif_hw_info(int md_id, ccif_hw_info_t *ccif_hw_info)
{
	if (ccif_hw_info == NULL)
		return -1;

	switch(md_id)
	{
		case MD_SYS1:
			ccif_hw_info->reg_base = AP_CCIF0_BASE;
			ccif_hw_info->irq_id = MT_CCIF0_AP_IRQ_ID;
			ccif_hw_info->type = CCIF_STD_V1;
			ccif_hw_info->irq_attr = 0;
			ccif_hw_info->md_id = MD_SYS1; // Bind to MD Sys 1
			return 0;
			
		case MD_SYS2:
			ccif_hw_info->reg_base = AP_CCIF1_BASE;
			ccif_hw_info->irq_id = MT_CCIF1_AP_IRQ_ID;
			ccif_hw_info->type = CCIF_STD_V1;
			ccif_hw_info->irq_attr = 0;
			ccif_hw_info->md_id = MD_SYS2; // Bind to MD Sys 2
			return 0;
			
		default:
			return -1;
	}
}
EXPORT_SYMBOL(get_ccif_hw_info);


void config_misc_info(int md_id, unsigned int base[], unsigned int size)
{
	misc_info_t misc_info;
	int str[2];
	
	if(NULL != base) {
		memset(&misc_info, 0 ,sizeof(misc_info_t));
		snprintf((char*)str, 2*sizeof(int), "CCIF");
		misc_info.prefix = str[0];
		misc_info.postfix = str[0];

		//== For Index 0 =============
		misc_info.index = 0;
		misc_info.next = 0;

		//--- Feature 0, remapping address
		misc_info.feature_0_val[0] = get_md_mem_start_addr(md_id);
		misc_info.support_mask |= (FEATURE_SUPPORT<<MISC_DMA_ADDR);
		
		//--- Feature 1, 32K clock less
		if(crystal_exist_status())
			misc_info.support_mask |= (FEATURE_NOT_SUPPORT<<(MISC_32K_LESS*2));
		else
			misc_info.support_mask |= (FEATURE_SUPPORT<<(MISC_32K_LESS*2));

		memcpy(base, &misc_info, sizeof(misc_info_t));
	}
}
EXPORT_SYMBOL(config_misc_info);


/*********************************************************************************/
/*  API of AP&MD related information transfer between AP and MD                                           */
/*                                                                                                                                   */
/*********************************************************************************/
#ifdef ENABLE_LOCK_MD_SLP_FEATURE
static int md_slp_cnt = 0;
static int md_slp_lock_ack = 0;
static int md_slp_unlock_ack = 0;
static DEFINE_SPINLOCK(md_slp_lock);

static int lock_sleep_cb(int data)
{
	if(data == LOCK_MD_SLP)
		md_slp_lock_ack = 1;
	else if(data == UNLOCK_MD_SLP)
		md_slp_unlock_ack = 1;

	return 0;
}

static int lock_md_sleep(int md_id, char *buf, unsigned int len)
{
	unsigned long flag;
	int ret = 0;
	unsigned int  msg = MD_SLP_REQUEST;
	unsigned int  reserved;
	
	spin_lock_irqsave(&md_slp_lock, flag);
	if (buf[0]) {
		if (++md_slp_cnt == 1)
			md_slp_lock_ack = 0;
	}
	else {
		if (md_slp_cnt == 0) {
			CCCI_MSG_INF(md_id, "ctl", "unlock md slp mis-match lock(%s, 0)\n", current->comm);
			spin_unlock_irqrestore(&md_slp_lock, flag);
			return ret;
		}
		
		if (--md_slp_cnt == 0)
			md_slp_unlock_ack = 0;
	}
	spin_unlock_irqrestore(&md_slp_lock, flag);
	
	if (md_slp_cnt == 1) {
		reserved = LOCK_MD_SLP;
	}
	else if (md_slp_cnt == 0) {
		reserved = UNLOCK_MD_SLP;
	} else {
		return ret;
	}

	CCCI_MSG_INF(md_id,"ctl", "%s request md sleep %d (%d, %d, %d): %d\n", 
		current->comm, buf[0], md_slp_cnt, md_slp_lock_ack, md_slp_unlock_ack, ret);

	ret = notify_md_by_sys_msg(md_id, msg, reserved);	

	return ret;
}

static int ack_md_sleep(int md_id, char *buf, unsigned int len)
{
	unsigned int flag = 0;
	
	if (buf[0])
		flag = md_slp_lock_ack;
	else
		flag = md_slp_unlock_ack;

	//CCCI_MSG_INF("ctl", "ack md sleep %d (%d %d) \n", buf[0], md_slp_cnt, flag);

	return flag;
}
#endif


//For thermal driver to get modem TxPowr
static int get_txpower(int md_id, char *buf, unsigned int len)
{
	int				ret = 0;
	unsigned int	msg;
	unsigned int	resv = 0;
	
	if(buf[0] == 0) {
		msg = MD_TX_POWER;
		ret = notify_md_by_sys_msg(md_id, msg, resv);
	} else if(buf[0] == 1){
		msg = MD_RF_TEMPERATURE;
		ret = notify_md_by_sys_msg(md_id, msg, resv);
	}	else if(buf[0] == 2){
		msg = MD_RF_TEMPERATURE_3G;
		ret = notify_md_by_sys_msg(md_id, msg, resv);
	}
	
	CCCI_DBG_MSG(md_id, "ctl", "get_txpower(%d): %d\n", buf[0], ret);

	return ret;
}


void send_battery_info(int md_id)
{
	int	ret = 0;
	unsigned int para = 0;
	unsigned int resv = 0;
	unsigned int msg_id = MD_GET_BATTERY_INFO;

	resv = get_bat_info(para);
	ret = notify_md_by_sys_msg(md_id, msg_id, resv);
	
	CCCI_DBG_MSG(md_id, "ctl", "send bat vol(%d) to md: %d\n", resv, ret);

	return;
}
EXPORT_SYMBOL(send_battery_info);


/*********************************************************************************/
/* ccci rpc helper function for RPC Section                                                                             */
/*                                                                                                                                   */
/*********************************************************************************/
#ifdef ENABLE_MD_IMG_SECURITY_FEATURE
typedef enum{
    SECRO_MD1 = 0,
    SECRO_MD2,    
} SECRO_USER;
extern unsigned char sec_secro_en(void);
extern unsigned int sec_secro_md_len(SECRO_USER user);
extern unsigned int sec_secro_md_get_data(SECRO_USER user, unsigned char *buf, unsigned int offset, unsigned int len);
extern unsigned int sec_secro_blk_sz(void);
#endif

unsigned int res_len = 0; //<<KE, need check this


void ccci_rpc_work_helper(int md_id, int *p_pkt_num, RPC_PKT pkt[], RPC_BUF *p_rpc_buf, unsigned int tmp_data[])
{
	// tmp_data[] is used to make sure memory address is valid after this function return
	int pkt_num = *p_pkt_num;

	CCCI_RPC_MSG(md_id, "ccci_rpc_work_helper++\n");

	tmp_data[0] = 0;

	switch(p_rpc_buf->op_id)
	{
		case IPC_RPC_CPSVC_SECURE_ALGO_OP:
		{
			unsigned char Direction = 0;
			unsigned int  ContentAddr = 0;
			unsigned int  ContentLen = 0;
			sed_t CustomSeed = SED_INITIALIZER;
			unsigned char *ResText __always_unused= NULL;
			unsigned char *RawText __always_unused= NULL;
			unsigned int i __always_unused= 0;

			if(pkt_num < 4)
			{
				CCCI_MSG_INF(md_id, "rpc", "invalid pkt_num %d for RPC_SECURE_ALGO_OP!\n", pkt_num);
				tmp_data[0] = FS_PARAM_ERROR;
				pkt[pkt_num].len = sizeof(unsigned int);
				pkt[pkt_num++].buf = (void*) &tmp_data[0];
				break;
			}

			Direction = *(unsigned char*)pkt[0].buf;
			ContentAddr = (unsigned int)pkt[1].buf;				
			CCCI_RPC_MSG(md_id, "RPC_SECURE_ALGO_OP: Content_Addr = 0x%08X, RPC_Base = 0x%08X, RPC_Len = 0x%08X\n", 
				ContentAddr, (unsigned int)p_rpc_buf, sizeof(RPC_BUF)+RPC1_MAX_BUF_SIZE);
			if(ContentAddr < (unsigned int)p_rpc_buf || 
								ContentAddr > ((unsigned int)p_rpc_buf + sizeof(RPC_BUF)+RPC1_MAX_BUF_SIZE))
			{
				CCCI_MSG_INF(md_id, "rpc", "invalid ContentAdddr[0x%08X] for RPC_SECURE_ALGO_OP!\n", ContentAddr);
				tmp_data[0] = FS_PARAM_ERROR;
				pkt[pkt_num].len = sizeof(unsigned int);
				pkt[pkt_num++].buf = (void*) &tmp_data[0];
				break;
			}
			ContentLen = *(unsigned int*)pkt[2].buf;
			//	CustomSeed = *(sed_t*)pkt[3].buf;
			WARN_ON(sizeof(CustomSeed.sed)<pkt[3].len);
			memcpy(CustomSeed.sed,pkt[3].buf,pkt[3].len);

			#ifdef ENCRYPT_DEBUG
			unsigned char log_buf[128];

			if(Direction == TRUE)
				CCCI_MSG_INF(md_id, "rpc", "HACC_S: EnCrypt_src:\n");
			else
				CCCI_MSG_INF(md_id, "rpc", "HACC_S: DeCrypt_src:\n");
			for(i = 0; i < ContentLen; i++)
			{
				if(i % 16 == 0){
					if(i!=0){
						CCCI_RPC_MSG(md_id, "%s\n", log_buf);
					}
					curr = 0;
					curr += snprintf(log_buf, sizeof(log_buf)-curr, "HACC_S: ");
				}
				//CCCI_MSG("0x%02X ", *(unsigned char*)(ContentAddr+i));
				curr += snprintf(&log_buf[curr], sizeof(log_buf)-curr, "0x%02X ", *(unsigned char*)(ContentAddr+i));					
				//sleep(1);
			}
			CCCI_RPC_MSG(md_id, "%s\n", log_buf);
				
			RawText = kmalloc(ContentLen, GFP_KERNEL);
			if(RawText == NULL)
				CCCI_MSG_INF(md_id, "rpc", "Fail alloc Mem for RPC_SECURE_ALGO_OP!\n");
			else
				memcpy(RawText, (unsigned char*)ContentAddr, ContentLen);
			#endif

			ResText = kmalloc(ContentLen, GFP_KERNEL);
			if(ResText == NULL)
			{
				CCCI_MSG_INF(md_id, "rpc", "Fail alloc Mem for RPC_SECURE_ALGO_OP!\n");
				tmp_data[0] = FS_PARAM_ERROR;
				pkt[pkt_num].len = sizeof(unsigned int);
				pkt[pkt_num++].buf = (void*) &tmp_data[0];
				break;
			}

			#if (defined(ENABLE_MD_IMG_SECURITY_FEATURE) && defined(MTK_SEC_MODEM_NVRAM_ANTI_CLONE))
			if(!SST_Secure_Init())
			{
				CCCI_MSG_INF(md_id, "rpc", "SST_Secure_Init fail!\n");
				ASSERT(0);
			}
			
			CCCI_RPC_MSG(md_id, "RPC_SECURE_ALGO_OP: Dir=0x%08X, Addr=0x%08X, Len=0x%08X, Seed=0x%016llX\n", 
					Direction, ContentAddr, ContentLen, *(long long *)CustomSeed.sed);
			SST_Secure_Algo(Direction, ContentAddr, ContentLen, CustomSeed.sed, ResText);

			if(!SST_Secure_DeInit())
				CCCI_MSG_INF(md_id, "rpc", "SST_Secure_DeInit fail!\n");
			#endif

			pkt_num = 0;
			pkt[pkt_num].len = sizeof(unsigned int);
			pkt[pkt_num++].buf = (void*) &tmp_data[0];
			pkt[pkt_num].len = ContentLen;	
			
			#if (defined(ENABLE_MD_IMG_SECURITY_FEATURE) && defined(MTK_SEC_MODEM_NVRAM_ANTI_CLONE))
			memcpy(pkt[pkt_num++].buf, ResText, ContentLen);
			CCCI_MSG_INF(md_id, "rpc","RPC_Secure memory copy OK: %d!", ContentLen);
			#else
			memcpy(pkt[pkt_num++].buf, (void *)ContentAddr, ContentLen);
			CCCI_MSG_INF(md_id, "rpc","RPC_NORMAL memory copy OK: %d!", ContentLen);
			#endif
			
			#ifdef ENCRYPT_DEBUG
			if(Direction == TRUE)
				CCCI_RPC_MSG(md_id, "HACC_D: EnCrypt_dst:\n");
			else
				CCCI_RPC_MSG(md_id, "HACC_D: DeCrypt_dst:\n");
			for(i = 0; i < ContentLen; i++)
			{
				if(i % 16 == 0){
					if(i!=0){
						CCCI_RPC_MSG(md_id, "%s\n", log_buf);
					}
					curr = 0;
					curr += snprintf(&log_buf[curr], sizeof(log_buf)-curr, "HACC_D: ");
				}
				//CCCI_MSG("%02X ", *(ResText+i));
				curr += snprintf(&log_buf[curr], sizeof(log_buf)-curr, "0x%02X ", *(ResText+i));
				//sleep(1);
			}
			
			CCCI_RPC_MSG(md_id, "%s\n", log_buf);

			if(RawText)
				kfree(RawText);
			#endif

			kfree(ResText);
			break;
		}

		#ifdef ENABLE_MD_IMG_SECURITY_FEATURE
		case IPC_RPC_GET_SECRO_OP:
		{
			unsigned char *addr = NULL;
			unsigned int img_len = 0;
			unsigned int img_len_bak = 0;
			unsigned int blk_sz = 0;
			unsigned int tmp = 1;
			unsigned int cnt = 0;
			unsigned int req_len = 0;	
		
			if(pkt_num != 1) {
				CCCI_MSG_INF(md_id, "rpc", "RPC_GET_SECRO_OP: invalid parameter: pkt_num=%d \n", pkt_num);
				tmp_data[0] = FS_PARAM_ERROR;
				pkt_num = 0;
				pkt[pkt_num].len = sizeof(unsigned int);
				pkt[pkt_num++].buf = (void*) &tmp_data[0];
				pkt[pkt_num].len = sizeof(unsigned int);
				tmp_data[1] = img_len;
				pkt[pkt_num++].buf = (void*) &tmp_data[1];
				break;
			}
				
			req_len = *(unsigned int*)(pkt[0].buf);
			if(sec_secro_en()) {
				if(md_id == MD_SYS1) {
					img_len = sec_secro_md_len(SECRO_MD1);
				} else {
					img_len = sec_secro_md_len(SECRO_MD2);
				}

				if((img_len > RPC1_MAX_BUF_SIZE) || (req_len > RPC1_MAX_BUF_SIZE)) {
					pkt_num = 0;
					tmp_data[0] = FS_MEM_OVERFLOW;
					pkt[pkt_num].len = sizeof(unsigned int);
					pkt[pkt_num++].buf = (void*) &tmp_data[0];
					//set it as image length for modem ccci check when error happens
					pkt[pkt_num].len = img_len;
					///pkt[pkt_num].len = sizeof(unsigned int);
					tmp_data[1] = img_len;
					pkt[pkt_num++].buf = (void*) &tmp_data[1];
					CCCI_MSG_INF(md_id, "rpc", "RPC_GET_SECRO_OP: md request length is larger than rpc memory: (%d, %d) \n", 
						req_len, img_len);
					break;
				}
				
				if(img_len > req_len) {
					pkt_num = 0;
					tmp_data[0] = FS_NO_MATCH;
					pkt[pkt_num].len = sizeof(unsigned int);
					pkt[pkt_num++].buf = (void*) &tmp_data[0];
					//set it as image length for modem ccci check when error happens
					pkt[pkt_num].len = img_len;
					///pkt[pkt_num].len = sizeof(unsigned int);
					tmp_data[1] = img_len;
					pkt[pkt_num++].buf = (void*) &tmp_data[1];
					CCCI_MSG_INF(md_id, "rpc", "RPC_GET_SECRO_OP: AP mis-match MD request length: (%d, %d) \n", 
						req_len, img_len);
					break;
				}

				/* TODO : please check it */
				/* save original modem secro length */
				CCCI_MSG("<rpc>RPC_GET_SECRO_OP: save MD SECRO length: (%d) \n",img_len);
				img_len_bak = img_len;
	   
				blk_sz = sec_secro_blk_sz();
				for(cnt = 0; cnt < blk_sz; cnt++) {
					tmp = tmp*2;
					if(tmp >= blk_sz)
						break;
				}
				++cnt;
				img_len = ((img_len + (blk_sz-1)) >> cnt) << cnt;

				addr = p_rpc_buf->buf + 4*sizeof(unsigned int);
				if(md_id == MD_SYS1) {
					tmp_data[0] = sec_secro_md_get_data(SECRO_MD1, addr, 0, img_len);
				} else {
					tmp_data[0] = sec_secro_md_get_data(SECRO_MD2, addr, 0, img_len);
				}


				/* TODO : please check it */
				/* restore original modem secro length */
				img_len = img_len_bak;

				CCCI_MSG("<rpc>RPC_GET_SECRO_OP: restore MD SECRO length: (%d) \n",img_len);             

				if(tmp_data[0] != 0) {
					CCCI_MSG_INF(md_id, "rpc", "RPC_GET_SECRO_OP: get data fail:%d \n", tmp_data[0]);
					pkt_num = 0;
					pkt[pkt_num].len = sizeof(unsigned int);
					pkt[pkt_num++].buf = (void*) &tmp_data[0];
					pkt[pkt_num].len = sizeof(unsigned int);
					tmp_data[1] = img_len;
					pkt[pkt_num++].buf = (void*) &tmp_data[1];
				} else {
					CCCI_MSG_INF(md_id, "rpc", "RPC_GET_SECRO_OP: get data OK: %d,%d \n", img_len, tmp_data[0]);
					pkt_num = 0;
					pkt[pkt_num].len = sizeof(unsigned int);
					//pkt[pkt_num++].buf = (void*) &img_len;
					tmp_data[1] = img_len;
					pkt[pkt_num++].buf = (void*)&tmp_data[1];
					pkt[pkt_num].len = img_len;
					pkt[pkt_num++].buf = (void*) addr;
					//tmp_data[2] = (unsigned int)addr;
					//pkt[pkt_num++].buf = (void*) &tmp_data[2];
				}
			}else {
				CCCI_MSG_INF(md_id, "rpc", "RPC_GET_SECRO_OP: secro disable \n");
				tmp_data[0] = FS_NO_FEATURE;
				pkt_num = 0;
				pkt[pkt_num].len = sizeof(unsigned int);
				pkt[pkt_num++].buf = (void*) &tmp_data[0];
				pkt[pkt_num].len = sizeof(unsigned int);
				tmp_data[1] = img_len;
				pkt[pkt_num++].buf = (void*) &tmp_data[1];	
			}

			break;
		}
		#endif

		//call eint API to get TDD EINT configuration for modem EINT initial
		case IPC_RPC_GET_TDD_EINT_NUM_OP:
		case IPC_RPC_GET_TDD_GPIO_NUM_OP:
		case IPC_RPC_GET_TDD_ADC_NUM_OP:
		{
			int get_num = 0;
			unsigned char * name = NULL;
			unsigned int length = 0;	

			if(pkt_num < 2)	{
				CCCI_MSG_INF(md_id, "rpc", "invalid parameter for [0x%X]: pkt_num=%d!\n", 
	                                p_rpc_buf->op_id, pkt_num);
				tmp_data[0] = FS_PARAM_ERROR;
				goto err1;
			}

			if((length = pkt[0].len) < 1) {
				CCCI_MSG_INF(md_id, "rpc", "invalid parameter for [0x%X]: pkt_num=%d, name_len=%d!\n", 
					p_rpc_buf->op_id, pkt_num, length);
				tmp_data[0] = FS_PARAM_ERROR;
				goto err1;
			}

			name = kmalloc(length, GFP_KERNEL);
			if(name == NULL) {
				CCCI_MSG_INF(md_id, "rpc", "Fail alloc Mem for [0x%X]!\n", p_rpc_buf->op_id);
				tmp_data[0] = FS_ERROR_RESERVED;
				goto err1;
			} else {
				memcpy(name, (unsigned char*)(pkt[0].buf), length);

				if(p_rpc_buf->op_id == IPC_RPC_GET_TDD_EINT_NUM_OP) {
					if((get_num = get_td_eint_info(md_id, name, length)) < 0) {
						get_num = FS_FUNC_FAIL;
					}
				}else if(p_rpc_buf->op_id == IPC_RPC_GET_TDD_GPIO_NUM_OP) {
					if((get_num = get_md_gpio_info(md_id, name, length)) < 0)	{
						get_num = FS_FUNC_FAIL;
					}
				}
				else if(p_rpc_buf->op_id == IPC_RPC_GET_TDD_ADC_NUM_OP) {
					if((get_num = get_md_adc_info(md_id, name, length)) < 0)	{
						get_num = FS_FUNC_FAIL;
					}
				}
		
				CCCI_MSG_INF(md_id, "rpc", "[0x%08X]: name:%s, len=%d, get_num:%d\n",p_rpc_buf->op_id,
					name, length, get_num);	
				pkt_num = 0;

				/* NOTE: tmp_data[1] not [0] */
				tmp_data[1] = (unsigned int)get_num;	// get_num may be invalid after exit this function
				pkt[pkt_num].len = sizeof(unsigned int);
				pkt[pkt_num++].buf = (void*)(&tmp_data[1]);	//get_num);
				pkt[pkt_num].len = sizeof(unsigned int);
				pkt[pkt_num++].buf = (void*)(&tmp_data[1]);	//get_num);
				kfree(name);
			}
			break;
	   
	        err1:
				pkt_num = 0;
				pkt[pkt_num].len = sizeof(unsigned int);
				pkt[pkt_num++].buf = (void*) &tmp_data[0];
				pkt[pkt_num].len = sizeof(unsigned int);
				pkt[pkt_num++].buf = (void*) &tmp_data[0];
				break;
	    }

		case IPC_RPC_GET_EMI_CLK_TYPE_OP:
		{
			int dram_type = 0;
			int dram_clk = 0;
		
			if(pkt_num != 0) {
				CCCI_MSG_INF(md_id, "rpc", "invalid parameter for [0x%X]: pkt_num=%d!\n", 
	                                p_rpc_buf->op_id, pkt_num);
				tmp_data[0] = FS_PARAM_ERROR;
				goto err2;
			}

			if(get_dram_type_clk(&dram_clk, &dram_type)) {
				tmp_data[0] = FS_FUNC_FAIL;
				goto err2;
			}
			else {
				tmp_data[0] = 0;
				CCCI_MSG_INF(md_id, "rpc", "[0x%08X]: dram_clk: %d, dram_type:%d \n",
					p_rpc_buf->op_id, dram_clk, dram_type);	
			}
		
			tmp_data[1] = (unsigned int)dram_type;
			tmp_data[2] = (unsigned int)dram_clk;
			
			pkt_num = 0;
			pkt[pkt_num].len = sizeof(unsigned int);
			pkt[pkt_num++].buf = (void*)(&tmp_data[0]);	
			pkt[pkt_num].len = sizeof(unsigned int);
			pkt[pkt_num++].buf = (void*)(&tmp_data[1]);	
			pkt[pkt_num].len = sizeof(unsigned int);
			pkt[pkt_num++].buf = (void*)(&tmp_data[2]);	
			break;
			
			err2:
				pkt_num = 0;
				pkt[pkt_num].len = sizeof(unsigned int);
				pkt[pkt_num++].buf = (void*) &tmp_data[0];
				pkt[pkt_num].len = sizeof(unsigned int);
				pkt[pkt_num++].buf = (void*) &tmp_data[0];
				pkt[pkt_num].len = sizeof(unsigned int);
				pkt[pkt_num++].buf = (void*) &tmp_data[0];
				break;
	    }
			
		case IPC_RPC_GET_EINT_ATTR_OP:	
		{
			char * eint_name = NULL;
			unsigned int name_len = 0;
			unsigned int type = 0;
			char * res = NULL;
			//unsigned int res_len = 0;
			int ret = 0;
			
			if(pkt_num < 3)	{
				CCCI_MSG_INF(md_id, "rpc", "invalid parameter for [0x%X]: pkt_num=%d!\n",
					p_rpc_buf->op_id, pkt_num);
				tmp_data[0] = FS_PARAM_ERROR;
				goto err3;
			}
			
			if((name_len = pkt[0].len) < 1) {
				CCCI_MSG_INF(md_id, "rpc", "invalid parameter for [0x%X]: pkt_num=%d, name_len=%d!\n",
					p_rpc_buf->op_id, pkt_num, name_len);
				tmp_data[0] = FS_PARAM_ERROR;
				goto err3;
			}
			
			eint_name = kmalloc(name_len, GFP_KERNEL);
			if(eint_name == NULL) {
				CCCI_MSG_INF(md_id, "rpc", "Fail alloc Mem for [0x%X]!\n", p_rpc_buf->op_id);
				tmp_data[0] = FS_ERROR_RESERVED;
				goto err3;
			}
			else {
				memcpy(eint_name, (unsigned char*)(pkt[0].buf), name_len);
			}
			
			type = *(unsigned int*)(pkt[2].buf);
			res = p_rpc_buf->buf + 4*sizeof(unsigned int);
			ret = get_eint_attr(eint_name, name_len, type, res, &res_len);
			if (ret == 0) {
				tmp_data[0] = ret;
				pkt_num = 0;
				pkt[pkt_num].len = sizeof(unsigned int);
				pkt[pkt_num++].buf = (void*) &tmp_data[0];
				pkt[pkt_num].len = res_len;
				pkt[pkt_num++].buf = (void*) res;
				CCCI_MSG_INF(md_id, "rpc", "[0x%08X] OK: name:%s, len:%d, type:%d, res:%d, res_len:%d\n",
					p_rpc_buf->op_id, eint_name, name_len, type, *res, res_len);
				kfree(eint_name);
			}
			else {
				tmp_data[0] = ret;
				CCCI_MSG_INF(md_id, "rpc", "[0x%08X] fail: name:%s, len:%d, type:%d, ret:%d\n", p_rpc_buf->op_id,
					eint_name, name_len, type, ret);
				kfree(eint_name);
				goto err3;
			}
			break;
			
		err3:
			pkt_num = 0;
			pkt[pkt_num].len = sizeof(unsigned int);
			pkt[pkt_num++].buf = (void*) &tmp_data[0];
			pkt[pkt_num].len = sizeof(unsigned int);
			pkt[pkt_num++].buf = (void*) &tmp_data[0];
			break;
		}

		default:
			CCCI_MSG_INF(md_id, "rpc", "[error]Unknown Operation ID (0x%08X)\n", p_rpc_buf->op_id);			
			tmp_data[0] = FS_NO_OP;
			pkt_num = 0;
			pkt[pkt_num].len = sizeof(int);
			pkt[pkt_num++].buf = (void*) &tmp_data[0];
			break;
	}
	*p_pkt_num = pkt_num;

	CCCI_RPC_MSG(md_id, "ccci_rpc_work_helper--\n");
}
EXPORT_SYMBOL(ccci_rpc_work_helper);


/*********************************************************************************/
/*  API about md ROM/RW/Share memory MPU protection                                                      */
/*                                                                                                                                   */
/*********************************************************************************/
static int clear_md_region_protection(int md_id)
{
	unsigned int rom_mem_mpu_id, rw_mem_mpu_id;

	#ifdef ENABLE_EMI_PROTECTION
	CCCI_MSG_INF(md_id, "ctl", "Clear MD%d region protect...\n", md_id+1);
	switch(md_id)
	{
		case MD_SYS1:
			rom_mem_mpu_id = 0;
			rw_mem_mpu_id = 1;
			break;
			
		case MD_SYS2:
			rom_mem_mpu_id = 2;
			rw_mem_mpu_id = 3;
			break;
			
		default:
			CCCI_MSG_INF(md_id, "ctl", "[error]md id(%d) invalid when clear MPU protect\n", md_id+1);
			return -1;
	}
	
	CCCI_MSG_INF(md_id, "ctl", "Clear MPU protect MD%d ROM region<%d>\n", md_id+1, rom_mem_mpu_id);
	emi_mpu_set_region_protection(0,	  				/*START_ADDR*/
								  0,      				/*END_ADDR*/
								  rom_mem_mpu_id,       /*region*/
								  SET_ACCESS_PERMISSON(NO_PROTECTION, NO_PROTECTION, NO_PROTECTION, NO_PROTECTION));

	CCCI_MSG_INF(md_id, "ctl", "Clear MPU protect MD%d R/W region<%d>\n", md_id+1, rom_mem_mpu_id);
	emi_mpu_set_region_protection(0,		  			/*START_ADDR*/
								  0,       				/*END_ADDR*/
								  rw_mem_mpu_id,        /*region*/
								  SET_ACCESS_PERMISSON(NO_PROTECTION, NO_PROTECTION, NO_PROTECTION, NO_PROTECTION));
	#endif
	
	return 0;
}


void enable_mem_access_protection(int md_id)
{
	unsigned int shr_mem_phy_start, shr_mem_phy_end, shr_mem_mpu_id, shr_mem_mpu_attr;
	unsigned int rom_mem_phy_start, rom_mem_phy_end, rom_mem_mpu_id, rom_mem_mpu_attr;
	unsigned int rw_mem_phy_start, rw_mem_phy_end, rw_mem_mpu_id, rw_mem_mpu_attr;
	unsigned int ap_mem_mpu_id, ap_mem_mpu_attr;
	struct image_info	*img_info;
	ccci_mem_layout_t	*md_layout;
	unsigned int kernel_base;
	unsigned int dram_size;

	// For MT6589
	//===================================================================
	//            | Region |  D0(AP)  |  D1(MD0)  |  D2(MD1)  |  D3(MM)
	//------------+------------------------------------------------------
	// MD0 ROM    |    0   |RO(S/NS)  |RO(S/NS)   |Forbidden  |Forbidden
	//------------+------------------------------------------------------
	// MD0 R/W+   |    1   |Forbidden |No protect |Forbidden  |Forbidden
	//------------+------------------------------------------------------
	// MD1 ROM    |    2   |RO(S/NS)  |Forbidden  |RO(S/NS)   |Forbidden
	//------------+------------------------------------------------------
	// MD1 R/W+   |    3   |Forbidden |Forbidden  |No protect |Forbidden
	//------------+------------------------------------------------------
	// Secure OS  |    4   |RW(S)     |Forbidden  |Forbidden  |Forbidden
	//------------+------------------------------------------------------
	// MD0 Share  |    5   |No protect|No protect |Forbidden  |Forbidden
	//------------+------------------------------------------------------
	// MD1 Share  |    6   |No protect|Forbidden  |No protect |Forbidden
	//------------+------------------------------------------------------
	// AP         |    7   |No protect|Forbidden  |Forbidden  |No protect
	//===================================================================

	#ifdef ENABLE_EMI_PROTECTION
	switch(md_id)
	{
		case MD_SYS1:
			img_info = md_img_info[md_id];
			md_layout = &md_mem_layout_tab[md_id];
			rom_mem_mpu_id = 0;
			rw_mem_mpu_id = 1;
			shr_mem_mpu_id = 5;
			rom_mem_mpu_attr = SET_ACCESS_PERMISSON(FORBIDDEN, FORBIDDEN, SEC_R_NSEC_R, SEC_R_NSEC_R);
			rw_mem_mpu_attr = SET_ACCESS_PERMISSON(FORBIDDEN, FORBIDDEN, NO_PROTECTION, FORBIDDEN);
			shr_mem_mpu_attr = SET_ACCESS_PERMISSON(FORBIDDEN, FORBIDDEN, NO_PROTECTION, NO_PROTECTION);			
			break;

		case MD_SYS2:
			img_info = md_img_info[md_id];
			md_layout = &md_mem_layout_tab[md_id];
			rom_mem_mpu_id = 2;
			rw_mem_mpu_id = 3;
			shr_mem_mpu_id = 6;
			rom_mem_mpu_attr = SET_ACCESS_PERMISSON(FORBIDDEN, SEC_R_NSEC_R, FORBIDDEN, SEC_R_NSEC_R);
			rw_mem_mpu_attr = SET_ACCESS_PERMISSON(FORBIDDEN, NO_PROTECTION, FORBIDDEN, FORBIDDEN);
			shr_mem_mpu_attr = SET_ACCESS_PERMISSON(FORBIDDEN, NO_PROTECTION, FORBIDDEN, NO_PROTECTION);
			break;

		default:
			CCCI_MSG_INF(md_id, "ctl", "[error]md id(%d) invalid when MPU protect\n", md_id+1);
			return;
	}

	kernel_base = KERNEL_PHY_BASE;
	dram_size = get_max_DRAM_size();
	ap_mem_mpu_id = 7;
	ap_mem_mpu_attr = SET_ACCESS_PERMISSON(NO_PROTECTION, FORBIDDEN, FORBIDDEN, NO_PROTECTION);
	
	shr_mem_phy_start = md_layout->smem_region_phy_before_map;
	shr_mem_phy_end   = md_layout->smem_region_phy_before_map + 0x200000;//md_layout->smem_region_size;
	rom_mem_phy_start = md_layout->md_region_phy;
	rom_mem_phy_end   = (md_layout->md_region_phy + img_info[MD_INDEX].size + 0xFFFF)&(~0xFFFF); //64KB align
	rw_mem_phy_start  = rom_mem_phy_end;
	rw_mem_phy_end	  = md_layout->md_region_phy + md_layout->md_region_size;
	
	CCCI_MSG_INF(md_id, "ctl", "MPU Start protect MD%d ROM region<%d:%08x:%08x>\n", md_id+1, 
                              	rom_mem_mpu_id, rom_mem_phy_start, rom_mem_phy_end);
	emi_mpu_set_region_protection(rom_mem_phy_start,	  /*START_ADDR*/
									rom_mem_phy_end,      /*END_ADDR*/
									rom_mem_mpu_id,       /*region*/
									rom_mem_mpu_attr);

	CCCI_MSG_INF(md_id, "ctl", "MPU Start protect MD%d R/W region<%d:%08x:%08x>\n", md_id+1, 
                              	rw_mem_mpu_id, rw_mem_phy_start, rw_mem_phy_end);
	emi_mpu_set_region_protection(rw_mem_phy_start,		  /*START_ADDR*/
									rw_mem_phy_end,       /*END_ADDR*/
									rw_mem_mpu_id,        /*region*/
									rw_mem_mpu_attr);

	CCCI_MSG_INF(md_id, "ctl", "MPU Start protect MD%d Share region<%d:%08x:%08x>\n", md_id+1, 
                              	shr_mem_mpu_id, shr_mem_phy_start, shr_mem_phy_end);
	emi_mpu_set_region_protection(shr_mem_phy_start,	  /*START_ADDR*/
									shr_mem_phy_end,      /*END_ADDR*/
									shr_mem_mpu_id,       /*region*/
									shr_mem_mpu_attr);

	CCCI_MSG_INF(md_id, "ctl", "MPU Start protect AP region<%d:%08x:%08x>\n", ap_mem_mpu_id, kernel_base, (kernel_base+dram_size)); 
		emi_mpu_set_region_protection(kernel_base,
									  (kernel_base+dram_size),
									  ap_mem_mpu_id,
									  ap_mem_mpu_attr);
	#endif

	return;
}
EXPORT_SYMBOL(enable_mem_access_protection);


/*********************************************************************************/
/*  API about security check                                                                                                */
/*                                                                                                                                    */
/*********************************************************************************/
#ifdef ENABLE_MD_IMG_SECURITY_FEATURE
int sec_lib_version_check(void)
{
	int ret = 0;

	int sec_lib_ver = sec_ccci_version_info();
	if(sec_lib_ver != CURR_SEC_CCCI_SYNC_VER){
		CCCI_MSG("[Error]sec lib for ccci mismatch: sec_ver:%d, ccci_ver:%d\n", sec_lib_ver, CURR_SEC_CCCI_SYNC_VER);
		ret = -1;
	}

	return ret;
}
#endif


#ifdef ENABLE_MD_IMG_SECURITY_FEATURE
//--------------------------------------------------------------------------------------------------//
// New signature check version. 2012-2-2. 
// Change to use sec_ccci_signfmt_verify_file(char *file_path, unsigned int *data_offset, unsigned int *data_sec_len)
//  sec_ccci_signfmt_verify_file parameter description
//    @ file_path: such as etc/firmware/modem.img
//    @ data_offset: the offset address that bypass signature header
//    @ data_sec_len: length of signature header + tail
//    @ return value: 0-success;
//---------------------------------------------------------------------------------------------------//
static int signature_check_v2(int md_id, char* file_path, unsigned int *sec_tail_length)
{
	unsigned int bypass_sec_header_offset = 0;
	unsigned int sec_total_len = 0;

	if( sec_ccci_signfmt_verify_file(file_path, &bypass_sec_header_offset, &sec_total_len) == 0 ){
		//signature lib check success
		//-- check return value
		CCCI_MSG_INF(md_id, "ctl", "sign check ret value 0x%x, 0x%x!\n", bypass_sec_header_offset, sec_total_len);
		if(bypass_sec_header_offset > sec_total_len){
			CCCI_MSG_INF(md_id, "ctl", "sign check get invalid ret value 0x%x, 0x%x!\n", bypass_sec_header_offset, sec_total_len);
			return -CCCI_ERR_LOAD_IMG_SIGN_FAIL;
		} else {
			CCCI_MSG_INF(md_id, "ctl", "sign check success!\n");
			*sec_tail_length = sec_total_len - bypass_sec_header_offset;
			return (int)bypass_sec_header_offset; // Note here, offset is more than 2G is not hoped 
		}
	} else {
		CCCI_MSG_INF(md_id, "ctl", "sign lib return fail!\n");
		return -CCCI_ERR_LOAD_IMG_SIGN_FAIL;
	}
}
#endif

static struct file *open_img_file(char *name, int *sec_fp_id)
{
	#ifdef ENABLE_MD_IMG_SECURITY_FEATURE
	int fp_id = OSAL_FILE_NULL;
	fp_id = osal_filp_open_read_only(name);  
	CCCI_DBG_COM_MSG("sec_open (%d)!\n", fp_id); 

	if(sec_fp_id != NULL)
		*sec_fp_id = fp_id;
	return (struct file *)osal_get_filp_struct(fp_id);
	#else
	CCCI_DBG_COM_MSG("std_open!\n");
	return filp_open(name, O_RDONLY, 0644);// 0777
	#endif
}

static void close_img_file(struct file *filp_id, int sec_fp_id)
{
	#ifdef ENABLE_MD_IMG_SECURITY_FEATURE
	CCCI_DBG_COM_MSG("sec_close (%d)!\n", sec_fp_id);
	osal_filp_close(sec_fp_id);
	#else
	CCCI_DBG_COM_MSG("std_close!\n");
	filp_close(filp_id,current->files);
	#endif
}


/*********************************************************************************/
/* check MD&dsp header structure                                                                                       */
/* return value: 0, no dsp header; >0, dsp header check ok; <0, dsp header check fail               */
/*********************************************************************************/
static int check_dsp_header(int md_id, 
							unsigned int parse_addr, 
							struct image_info *image)
{
	int					ret = 0;
	char				*start_addr = (char *)parse_addr;
	GFH_HEADER			*gfh_head = (GFH_HEADER *)parse_addr;
	GFH_FILE_INFO_v1	*gfh_file_head = NULL;
	GFH_CHECK_CFG_v1	*gfh_check_head = &dsp_sys_head[md_id];
	unsigned int		dsp_ver = DSP_VER_INVALID;
	unsigned int		ap_ver = DSP_VER_INVALID;
	bool				file_info_check = false;
	bool				header_check = false;
	bool				ver_check = false;
	bool				image_check = false;
	bool				platform_check = false;

	if (gfh_head == NULL) {
		CCCI_MSG_INF(md_id, "ctl", "ioremap DSP image failed!\n");
		ret = -ENOMEM;
		goto out;
	}

	while((gfh_head->m_magic_ver & 0xFFFFFF) == GFH_HEADER_MAGIC_NO) {
		if(gfh_head->m_type == GFH_FILE_INFO_TYPE) {
			gfh_file_head = (GFH_FILE_INFO_v1 *)gfh_head;
			file_info_check = true;

			//check image type: DSP_ROM or DSP_BL
			if (gfh_file_head->m_file_type == DSP_ROM_TYPE) {
				image_check = true;
			}
		}
		else if(gfh_head->m_type == GFH_CHECK_HEADER_TYPE) {
			*gfh_check_head = *(GFH_CHECK_CFG_v1 *)gfh_head;
			header_check = true;

			//check image version: 2G or 3G
			if(gfh_check_head->m_image_type == get_ap_img_ver())
				ver_check = true;
			image->ap_info.image_type = type_str[get_ap_img_ver()];
			image->img_info.image_type = type_str[gfh_check_head->m_image_type];

			//get dsp product version: debug or release
			image->img_info.product_ver = product_str[gfh_check_head->m_product_ver];

			#ifdef ENABLE_CHIP_VER_CHECK
			if(!strncmp(gfh_check_head->m_platform_id, ap_platform, AP_PLATFORM_LEN)) {
				platform_check = true;
			}
			#else
			platform_check = true;
			#endif
			image->img_info.platform = gfh_check_head->m_platform_id;

			//get build version and build time
			image->img_info.build_ver = gfh_check_head->m_project_id;
			image->img_info.build_time = gfh_check_head->m_build_time;
		}

		start_addr += gfh_head->m_size;
		gfh_head = (GFH_HEADER *)start_addr;
	}

	CCCI_MSG_INF(md_id, "ctl", "\n");
	CCCI_MSG_INF(md_id, "ctl", "**********************DSP image check****************************\n");
	if(!file_info_check && !header_check) {
		CCCI_MSG_INF(md_id, "ctl", "GFH_FILE_INFO header and GFH_CHECK_HEADER not exist!\n");
		CCCI_MSG_INF(md_id, "ctl", "[Reason]No DSP_ROM, please check this image!\n");
		ret = -CCCI_ERR_LOAD_IMG_DSP_CHECK;
	}
	else if(file_info_check && !header_check) {
		CCCI_MSG_INF(md_id, "ctl", "GFH_CHECK_HEADER not exist!\n");

		//check the image version from file_info structure
		dsp_ver = (gfh_file_head->m_file_ver >> DSP_2G_BIT)& 0x1;
		dsp_ver = dsp_ver? AP_IMG_2G:AP_IMG_3G;
		ap_ver = get_ap_img_ver();

		if(dsp_ver == ap_ver)
			ver_check = true;

		image->ap_info.image_type = type_str[ap_ver];
		image->img_info.image_type = type_str[dsp_ver];

		if(image_check && ver_check) {
			CCCI_MSG_INF(md_id, "ctl", "GFH_FILE_INFO header check OK!\n");
		}
		else {
			CCCI_MSG_INF(md_id, "ctl", "[Error]GFH_FILE_INFO check fail!\n");
			if(!image_check)
				CCCI_MSG_INF(md_id, "ctl", "[Reason]not DSP_ROM image, please check this image!\n");

			if(!ver_check)
				CCCI_MSG_INF(md_id, "ctl", "[Reason]DSP type(2G/3G) mis-match to AP!\n");	

			ret = -CCCI_ERR_LOAD_IMG_DSP_CHECK;
		}

		CCCI_MSG_INF(md_id, "ctl", "(DSP)[type]=%s\n",(image_check?DSP_ROM_STR:DSP_BL_STR));
		CCCI_MSG_INF(md_id, "ctl", "(DSP)[ver]=%s, (AP)[ver]=%s\n",image->img_info.image_type,
				image->ap_info.image_type);
	}
	else if(!file_info_check && header_check) {
		CCCI_MSG_INF(md_id, "ctl", "GFH_FILE_INFO header not exist!\n");

		if(ver_check && platform_check) {
			CCCI_MSG_INF(md_id, "ctl", "GFH_CHECK_HEADER header check OK!\n");
		}
		else {
			CCCI_MSG_INF(md_id, "ctl", "[Error]GFH_CHECK_HEADER check fail!\n");

			if(!ver_check)
				CCCI_MSG_INF(md_id, "ctl", "[Reason]DSP type(2G/3G) mis-match to AP!\n");

			if(!platform_check)
				CCCI_MSG_INF(md_id, "ctl", "[Reason]DSP platform version mis-match to AP!\n");

			ret = -CCCI_ERR_LOAD_IMG_DSP_CHECK;
		}
		CCCI_MSG_INF(md_id, "ctl", "(DSP)[ver]=%s, (AP)[ver]=%s\n",image->img_info.image_type, 
			image->ap_info.image_type);
		CCCI_MSG_INF(md_id, "ctl", "(DSP)[plat]=%s, (AP)[plat]=%s\n",image->img_info.platform, 
			image->ap_info.platform);
		CCCI_MSG_INF(md_id, "ctl", "(DSP)[build_Ver]=%s, [build_time]=%s\n", image->img_info.build_ver , 
			image->img_info.build_time);
		CCCI_MSG_INF(md_id, "ctl", "(DSP)[product_ver]=%s\n", product_str[gfh_check_head->m_product_ver]);

	}
	else {
		if(image_check && ver_check && platform_check) {
			CCCI_MSG_INF(md_id, "ctl", "GFH_FILE_INFO header and GFH_CHECK_HEADER check OK!\n");
		}
		else {
			CCCI_MSG_INF(md_id, "ctl", "[Error]DSP header check fail!\n");
			if(!image_check)
				CCCI_MSG_INF(md_id, "ctl", "[Reason]No DSP_ROM, please check this image!\n");

			if(!ver_check)
				CCCI_MSG_INF(md_id, "ctl", "[Reason]DSP type(2G/3G) mis-match to AP!\n");

			if(!platform_check)
				CCCI_MSG_INF(md_id, "ctl", "[Reason]DSP platform version mis-match to AP!\n");

			ret = -CCCI_ERR_LOAD_IMG_DSP_CHECK;
		}
		CCCI_MSG_INF(md_id, "ctl", "(DSP)[type]=%s\n",(image_check?DSP_ROM_STR:DSP_BL_STR));
		CCCI_MSG_INF(md_id, "ctl", "(DSP)[ver]=%s, (AP)[ver]=%s\n",image->img_info.image_type, 
			image->ap_info.image_type);
		CCCI_MSG_INF(md_id, "ctl", "(DSP)[plat]=%s, (AP)[plat]=%s\n",image->img_info.platform, 
			image->ap_info.platform);
		CCCI_MSG_INF(md_id, "ctl", "(DSP)[build_Ver]=%s, [build_time]=%s\n", image->img_info.build_ver , 
			image->img_info.build_time);
		CCCI_MSG_INF(md_id, "ctl", "(DSP)[product_ver]=%s\n", product_str[gfh_check_head->m_product_ver]);

	}
	CCCI_MSG_INF(md_id, "ctl", "**********************DSP image check****************************\r\n");

out:
	return ret;

}

static int check_md_header( int md_id, 
							unsigned int parse_addr, 
							struct image_info *image)
{
	int ret;
	bool md_type_check = false;
	bool md_plat_check = false;
	bool md_sys_match  = false;
	MD_CHECK_HEADER *head = &md_sys_head[md_id];

	memcpy(head, (void*)(parse_addr - sizeof(MD_CHECK_HEADER)), sizeof(MD_CHECK_HEADER));

	CCCI_MSG_INF(md_id, "ctl", "\n");
	CCCI_MSG_INF(md_id, "ctl", "**********************MD image check***************************\n");
	ret = strncmp(head->check_header, MD_HEADER_MAGIC_NO, 12);
	if(ret) {
		CCCI_MSG_INF(md_id, "ctl", "md check header not exist!\n");
		ret = 0;
	}
	else {
		if(head->header_verno != MD_HEADER_VER_NO) {
			CCCI_MSG_INF(md_id, "ctl", "[Error]md check header version mis-match to AP:[%d]!\n", 
				head->header_verno);
		}
		else {
			#ifdef ENABLE_2G_3G_CHECK
			if((head->image_type != AP_IMG_INVALID) && (head->image_type == get_ap_img_ver())) {
				md_type_check = true;
			}
			#else
			md_type_check = true;
			#endif

			#ifdef ENABLE_CHIP_VER_CHECK
			if(!strncmp(head->platform, ap_platform, AP_PLATFORM_LEN)) {
				md_plat_check = true;
			}
			#else
			md_plat_check = true;
			#endif

			if(head->bind_sys_id == (md_id+1)) {
				md_sys_match = true;
			}

			image->ap_info.image_type = type_str[get_ap_img_ver()];
			image->img_info.image_type = type_str[head->image_type];
			image->ap_info.platform = ap_platform;
			image->img_info.platform = head->platform;
			image->img_info.build_time = head->build_time;
			image->img_info.build_ver = head->build_ver;
			image->img_info.product_ver = product_str[head->product_ver];

			if(md_type_check && md_plat_check && md_sys_match) {
				CCCI_MSG_INF(md_id, "ctl", "Modem header check OK!\n");
			}
			else {
				CCCI_MSG_INF(md_id, "ctl", "[Error]Modem header check fail!\n");
				if(!md_type_check)
					CCCI_MSG_INF(md_id, "ctl", "[Reason]MD type(2G/3G) mis-match to AP!\n");

				if(!md_plat_check)
					CCCI_MSG_INF(md_id, "ctl", "[Reason]MD platform mis-match to AP!\n");

				if(!md_sys_match)
					CCCI_MSG_INF(md_id, "ctl", "[Reason]MD image is not for MD SYS%d!\n", md_id+1);

				ret = -CCCI_ERR_LOAD_IMG_MD_CHECK;
			}

			CCCI_MSG_INF(md_id, "ctl", "(MD)[type]=%s, (AP)[type]=%s\n",image->img_info.image_type, image->ap_info.image_type);
			CCCI_MSG_INF(md_id, "ctl", "(MD)[plat]=%s, (AP)[plat]=%s\n",image->img_info.platform, image->ap_info.platform);
			CCCI_MSG_INF(md_id, "ctl", "(MD)[build_ver]=%s, [build_time]=%s\n",image->img_info.build_ver, image->img_info.build_time);
			CCCI_MSG_INF(md_id, "ctl", "(MD)[product_ver]=%s\n",image->img_info.product_ver);

		}
	}
	CCCI_MSG_INF(md_id, "ctl", "**********************MD image check***************************\r\n");

	return ret;
}


/*********************************************************************************/
/* load modem&dsp image function                                                                                     */
/*                                                                                                                                   */
/*********************************************************************************/
#ifdef ENABLE_MD_IMG_SECURITY_FEATURE
static int load_cipher_firmware_v2( int md_id, 
									int fp_id, 
									struct image_info *img,
									unsigned int cipher_img_offset, 
									unsigned int cipher_img_len)
{
	int ret;
	void *addr = ioremap_nocache(img->address,cipher_img_len);
	void *addr_bak = addr;
	unsigned int data_offset;

	if (addr==NULL) {
		CCCI_MSG_INF(md_id, "ctl", "ioremap image fialed!\n");
		ret = -CCCI_ERR_LOAD_IMG_NO_ADDR;
		goto out;
	}

	if(SEC_OK != sec_ccci_decrypt_cipherfmt(fp_id, cipher_img_offset, (char*)addr, cipher_img_len, &data_offset) ) {
		CCCI_MSG_INF(md_id, "ctl", "cipher image decrypt fail!\n");
		ret = -CCCI_ERR_LOAD_IMG_CIPHER_FAIL;
		goto unmap_out;
	}

	img->size = cipher_img_len;
	img->offset += data_offset;	
	addr+=cipher_img_len;

	ret=check_md_header(md_id, ((unsigned int)addr), img);

unmap_out:
	iounmap(addr_bak);
out:
	return ret;
}
#endif

static int load_std_firmware(int md_id, 
							 struct file *filp, 
							 struct image_info *img)
{
	void			*start;
	int				ret = 0;
	int				check_ret = 0;
	int				read_size = 0;
	mm_segment_t	curr_fs;
	unsigned long	load_addr;
	unsigned int	end_addr;
	const int		size_per_read = 1024 * 1024;
	const int		size = 1024;

	curr_fs = get_fs();
	set_fs(KERNEL_DS);

	load_addr = img->address;
	filp->f_pos = img->offset;

	while (1) {
		// Map 1M memory
		start = ioremap_nocache((load_addr + read_size), size_per_read);
		//CCCI_MSG_INF(md_id, "ctl", "map %08x --> %08x\n", (unsigned int)(load_addr+read_size), (unsigned int)start);
		if (start <= 0) {
			CCCI_MSG_INF(md_id, "ctl", "CCCI_MD: Firmware ioremap failed:%d\n", (unsigned int)start);
			set_fs(curr_fs);
			return -CCCI_ERR_LOAD_IMG_NOMEM;
		}

		ret = filp->f_op->read(filp, start, size_per_read, &filp->f_pos);
		if ((ret < 0) || (ret > size_per_read)) {
			CCCI_MSG_INF(md_id, "ctl", "image read failed=%d\n", ret);
			ret = -CCCI_ERR_LOAD_IMG_FILE_READ;
			goto error;
		}
		else if(ret == size_per_read) {
			read_size += ret;
			iounmap(start);
		}
		else {
			read_size += ret;
			img->size = read_size - img->tail_length; /* Note here, signatured image has file tail info. */
			CCCI_MSG_INF(md_id, "ctl", "%s image size=0x%x,read size:%d, tail:%d\n", 
							img->file_name, img->size, read_size, img->tail_length);
			iounmap(start);
			break;
		}
	}

	if(img->type == MD_INDEX) {
		start = ioremap_nocache(round_down(load_addr + img->size - 0x4000, 0x4000), 
					round_up(img->size, 0x4000) - round_down(img->size - 0x4000, 0x4000)); // Make sure in one scope
		end_addr = ((unsigned int)start + img->size - round_down(img->size - 0x4000, 0x4000));
		if((check_ret = check_md_header(md_id, end_addr, img)) < 0) {
			ret = check_ret;
			goto error;
		}
		iounmap(start);
	}
	else if(img->type == DSP_INDEX) {
		start = ioremap_nocache(load_addr, size);
		if((check_ret = check_dsp_header(md_id, (unsigned int)start, img))<0){
			ret = check_ret;
			goto error;	
		}
		iounmap(start);
	}

	set_fs(curr_fs);
	CCCI_MSG_INF(md_id, "ctl", "Load %s (size=%d) to 0x%lx\n", img->file_name, read_size, load_addr);

	return read_size;

error:
	iounmap(start);
	set_fs(curr_fs);
	return ret;
}


#if 0   // Debug use only
static int md_mem_copy(unsigned int src, unsigned int des, unsigned int leng)
{
	void		*src_start, *des_start;
	int		read_size = 0;
	const int	size_per_read = 1024 * 1024;
	unsigned int curr_src, curr_des;

	CCCI_MSG_INF(0, "ctl", "md_mem_copy src:%08x des:%08x\n", src, des);
	while (1) {
		// Map 1M memory
		curr_src = src+read_size;
		curr_des = des+read_size;
		if(read_size >= leng)
			break;
		src_start = ioremap_nocache((src + read_size), size_per_read);
		if (src_start <= 0) {
			CCCI_MSG_INF(0, "ctl", "md_mem_copy map src start fail:%d\n", (unsigned int)src_start);
			return -1;
		}else{
			//CCCI_MSG_INF(0, "ctl", "SRC map: %08x --> %08x\n", curr_src, (unsigned int)src_start);
		}
		des_start = ioremap_nocache((des + read_size), size_per_read);
		if (des_start <= 0) {
			CCCI_MSG_INF(0, "ctl", "md_mem_copy map des start fail:%d\n", (unsigned int)des_start);
			return -1;
		}else{
			//CCCI_MSG_INF(0, "ctl", "DES map: %08x --> %08x\n", curr_des, (unsigned int)des_start);
		}

		// Memory copy
		memcpy(des_start, src_start, size_per_read);
		read_size += size_per_read;
	
		iounmap(src_start);
		iounmap(des_start);
	}

	return 0;
}


int cpy_check(int md_id, unsigned long start_addr, unsigned int size)
{
	void			*start;
	mm_segment_t	curr_fs;
	const int		size_per_read = 1024 * 1024;
	char			data;
	unsigned long	i, check_num;
	int				has_check = 0;
	char			*check_data;
	int				need_break = 0;
	int				fp_id;
	struct file		*filp;

	curr_fs = get_fs();
	set_fs(KERNEL_DS);
	fp_id = osal_filp_open_read_only("/system/etc/firmware/modem.img");
	filp = (struct file *)osal_get_filp_struct(fp_id);

	filp->f_pos = 0;

	CCCI_MSG_INF(md_id, "ctl", "Begin check image\n");
	while (has_check < size) {
		start = ioremap_nocache((start_addr + has_check), size_per_read);
		if (start <= 0) {
			CCCI_MSG_INF(md_id, "ctl", "CCCI_MD: Check firmware ioremap failed:%d\n", (unsigned int)start);
			set_fs(curr_fs);
			return -1;
		}
		check_data = (char*)start;

		if((size-has_check) > size_per_read)
			check_num = size_per_read;
		else {
			need_break = 1;
			check_num = (size-has_check);
		}

		for(i=0; i<check_num; i++) {
			filp->f_op->read(filp, &data, sizeof(char), &filp->f_pos);
			//printk("%02x:%02x\n", data, check_data[i]);
			if(check_data[i] != data) {
				CCCI_MSG_INF(md_id, "ctl", "Check firmware failed at offset:%d\n", (unsigned int)(has_check+i));
			}
		}
		iounmap(start);
		has_check += size_per_read;

		if(need_break)
			break;
	}

	CCCI_MSG_INF(md_id, "ctl", "End check image\n");
	set_fs(curr_fs);

	return 0;

}
#endif


static int load_firmware_func(int md_id, struct image_info *img)
{
	struct file		*filp = NULL;
	int				fp_id;
	int				ret=0;
	int				offset=0;
	unsigned int	sec_tail_length = 0;

	#ifdef ENABLE_MD_IMG_SECURITY_FEATURE
	unsigned int	img_len=0;
	#endif

	//get modem&dsp image name with E1&E2 
	switch(md_id)
	{
		case MD_SYS1:
			if(get_chip_version() == CHIP_SW_VER_01) {
				snprintf(img->file_name,sizeof(img->file_name),
				CONFIG_MODEM_FIRMWARE_PATH "%s",(img->type?DSP_IMAGE_E1_NAME:MOEDM_IMAGE_E1_NAME));
			}
			else if(get_chip_version() == CHIP_SW_VER_02) {		
				snprintf(img->file_name,sizeof(img->file_name),
				CONFIG_MODEM_FIRMWARE_PATH "%s",(img->type?DSP_IMAGE_E2_NAME:MOEDM_IMAGE_E2_NAME));
			}
			break;
			
		case MD_SYS2:
			if(get_chip_version() == CHIP_SW_VER_01) {
				snprintf(img->file_name,sizeof(img->file_name),
				CONFIG_MODEM_FIRMWARE_PATH "%s",(img->type?DSP_IMAGE_E1_NAME:MOEDM_SYS2_IMAGE_E1_NAME));
			}
			else if(get_chip_version() == CHIP_SW_VER_02) {		
				snprintf(img->file_name,sizeof(img->file_name),
				CONFIG_MODEM_FIRMWARE_PATH "%s",(img->type?DSP_IMAGE_E2_NAME:MOEDM_SYS2_IMAGE_E2_NAME));
			}
			break;
			
		default:
			return -1;
	}

	filp = open_img_file(img->file_name, &fp_id);
	if (IS_ERR(filp)) {
		CCCI_MSG_INF(md_id, "ctl","open %s fail(%ld), try to open modem.img\n",
			img->file_name, PTR_ERR(filp));
		goto open_file; // Try to open modem.img
	} else { // Open image success, goto header check
		CCCI_MSG_INF(md_id, "ctl", "open %s OK\n",img->file_name);
		goto check_head;
	}

open_file:
	//get default modem&dsp image name (modem.img & DSP_ROM)
	if (md_id == MD_SYS1)
		snprintf(img->file_name,sizeof(img->file_name),
				CONFIG_MODEM_FIRMWARE_PATH "%s",(img->type?DSP_IMAGE_NAME:MOEDM_IMAGE_NAME));
	else if (md_id == MD_SYS2)
		snprintf(img->file_name,sizeof(img->file_name),
				CONFIG_MODEM_FIRMWARE_PATH "%s",(img->type?DSP_IMAGE_NAME:MOEDM_SYS2_IMAGE_NAME));

	filp = open_img_file(img->file_name, &fp_id);
	if (IS_ERR(filp)) {
		CCCI_MSG_INF(md_id, "ctl", "open %s fail: %ld\n",img->file_name, PTR_ERR(filp));
		ret = -CCCI_ERR_LOAD_IMG_FILE_OPEN;
		filp = NULL;
		goto out;
	} else {
		CCCI_MSG_INF(md_id, "ctl", "open %s OK\n",img->file_name);
	}

check_head:
	//only modem.img need check signature and cipher header
	//sign_check = false;
	sec_tail_length = 0;
	if(img->type == MD_INDEX) {
		//step1:check if need to signature
		//offset=signature_check(filp);
		#ifdef ENABLE_MD_IMG_SECURITY_FEATURE
		offset = signature_check_v2(md_id, img->file_name, &sec_tail_length);
		CCCI_MSG_INF(md_id, "ctl", "signature_check offset:%d, tail:%d\n",offset, sec_tail_length);
		if (offset<0) {
			CCCI_MSG_INF(md_id, "ctl", "signature_check failed ret=%d\n",offset);
			ret=offset;
			goto out;
		}
		#endif

		img->offset=offset;
		img->tail_length = sec_tail_length;

		//step2:check if need to cipher
		#ifdef ENABLE_MD_IMG_SECURITY_FEATURE       
		if (sec_ccci_is_cipherfmt(fp_id,offset,&img_len)) {// Cipher image
			CCCI_MSG_INF(md_id, "ctl", "cipher image\n");
			//ret=load_cipher_firmware(filp,img,&cipher_header);
			ret=load_cipher_firmware_v2(md_id, fp_id,img,offset, img_len);
			if(ret<0) {
				CCCI_MSG_INF(md_id, "ctl", "load_cipher_firmware failed:ret=%d!\n",ret);
				goto out;
			}
			CCCI_MSG_INF(md_id, "ctl", "load_cipher_firmware done! (=%d)\n",ret);
		} 
		else {
		#endif
			CCCI_MSG_INF(md_id, "ctl", "Not cipher image\n");
			ret=load_std_firmware(md_id, filp, img);
			if(ret<0) {
   				CCCI_MSG_INF(md_id, "ctl", "load_firmware failed:ret=%d!\n",ret);
				goto out;
    		}
		#ifdef ENABLE_MD_IMG_SECURITY_FEATURE           
		}
		#endif        
	}
	//dsp image check signature during uboot, and ccci not need check for dsp.
	else if(img->type == DSP_INDEX) {
		ret=load_std_firmware(md_id, filp,img);
		if(ret<0) {
   			CCCI_MSG_INF(md_id, "ctl", "load_firmware for %s failed:ret=%d!\n",img->file_name,ret);
			goto out;
    	}
	}

out:
	if(filp != NULL){ 
		close_img_file(filp, fp_id);
	}

	return ret;
}


static int load_img_cfg(int md_id)
{
	ccci_mem_layout_t *layout;
	
	layout = &md_mem_layout_tab[md_id];
	md_img_info[md_id][MD_INDEX].type = MD_INDEX;
	md_img_info[md_id][MD_INDEX].address = layout->md_region_phy;
	md_img_info[md_id][MD_INDEX].offset = 0;
	md_img_info[md_id][MD_INDEX].load_firmware = load_firmware_func;
	md_img_info[md_id][MD_INDEX].flags = CAN_BE_RELOAD|LOAD_ONE_TIME;
	md_img_info[md_id][MD_INDEX].ap_info.platform = ap_platform;
	
	//md_img_info[md_id][DSP_INDEX].type = DSP_INDEX; //no dsp image on MT6589	
	//md_img_info[md_id][DSP_INDEX].address = layout->dsp_region_phy;	
	//md_img_info[md_id][DSP_INDEX].offset = 0;	
	//md_img_info[md_id][DSP_INDEX].load_firmware = load_firmware_func;			
	//md_img_info[md_id][DSP_INDEX].flags = LOAD_ONE_TIME;		
	//md_img_info[md_id][DSP_INDEX].ap_info.platform = ap_platform;

	return 0;
}


int ccci_load_firmware(int md_id, unsigned int load_flag, char img_err_str[], int len)
{
	int					i;
	int					ret = 0;
	int					ret_a[IMG_CNT] = {0};
	int					img_need_load;
	struct image_info	*img_ptr = NULL;
	char				*img_str, *str;

	img_ptr = md_img_info[md_id];
	img_str = md_img_info_str[md_id];

	//step1: clear modem protection when start to load firmware
	clear_md_region_protection(md_id);

	//step2: load image
	for(i = 0; i < IMG_CNT; i++) {
		img_need_load = load_flag&img_ptr[i].flags;
		if ( img_need_load && img_ptr[i].load_firmware) {
			if((ret_a[i]= img_ptr[i].load_firmware(md_id, &img_ptr[i])) < 0){
				CCCI_MSG_INF(md_id, "ctl", "load firmware fail for %s!\n", img_ptr[i].file_name);
			}
		}
		else if (img_need_load == 0) {
			CCCI_MSG_INF(md_id, "ctl", "Not load firmware for %s!\n", (i?DSP_IMAGE_NAME:MOEDM_IMAGE_NAME));
		}
		else {
			CCCI_MSG_INF(md_id, "ctl", "load null firmware for %s!\n", img_ptr[i].file_name);
			ret_a[i] = -CCCI_ERR_LOAD_IMG_FIRM_NULL; 
		}
	}

	// Can be delete, load image to start address 0
	//md_mem_copy(md_mem_layout_tab[md_id].md_region_phy, 0, 22*1024*1024);
	//cpy_check(0, 0, 0x4fa420);
	//need protect after load firmeware is completed
	//start_emi_mpu_protect();

	/* Construct image information string */
	sprintf(img_str, "MD:%s*%s*%s*%s\nAP:%s*%s (MD)%s\n",
			img_ptr[MD_INDEX].img_info.image_type,img_ptr[MD_INDEX].img_info.platform, 
			img_ptr[MD_INDEX].img_info.build_ver,img_ptr[MD_INDEX].img_info.build_time,
			img_ptr[MD_INDEX].ap_info.image_type,img_ptr[MD_INDEX].ap_info.platform,
			img_ptr[MD_INDEX].img_info.product_ver);

	// Image info ready
	str = img_ptr[MD_INDEX].img_info.product_ver;
	if( NULL == str)
		img_is_dbg_ver[md_id] = MD_DEBUG_REL_INFO_NOT_READY;
	else if(strcmp(str, "Debug") == 0)
		img_is_dbg_ver[md_id] = MD_IS_DEBUG_VERSION;
	else
		img_is_dbg_ver[md_id] = MD_IS_RELEASE_VERSION;

	// Prepare error string if needed
	if(img_err_str != NULL) {
		if(ret_a[MD_INDEX] == -CCCI_ERR_LOAD_IMG_SIGN_FAIL) {
			snprintf(img_err_str, len, "%s Signature check fail\n", img_ptr[i].file_name);
			CCCI_MSG_INF(md_id, "ctl", "signature check fail!\n");
		}
		else if(ret_a[MD_INDEX] == -CCCI_ERR_LOAD_IMG_CIPHER_FAIL) {
			snprintf(img_err_str, len, "%s Cipher chekc fail\n", img_ptr[i].file_name);
			CCCI_MSG_INF(md_id, "ctl", "cipher check fail!\n");
		}
		else if(ret_a[MD_INDEX] == -CCCI_ERR_LOAD_IMG_FILE_OPEN) {
			snprintf(img_err_str, len, "[ASSERT] Modem image not exist\n");
		}
		else if( ret_a[MD_INDEX] == -CCCI_ERR_LOAD_IMG_MD_CHECK) {
			snprintf(img_err_str, len, "[ASSERT] Modem mismatch to AP\n\n");
		}
	}
	if(ret_a[MD_INDEX] < 0)
		ret = -CCCI_ERR_LOAD_IMG_LOAD_FIRM;

	return ret;
}
EXPORT_SYMBOL(ccci_load_firmware);


/*********************************************************************************/
/*  hook function during modem boot up                                                                               */
/*                                                                                                                                   */
/*********************************************************************************/
void md_env_setup_before_boot(int md_id)
{
	enable_mem_access_protection(md_id);
}
EXPORT_SYMBOL(md_env_setup_before_boot);


void md_env_setup_before_ready(int md_id)
{
}
EXPORT_SYMBOL(md_env_setup_before_ready);


void md_boot_up_additional_operation(int md_id)
{
	//power on Audsys for DSP boot up
	//AudSys_Power_On(TRUE);
}
EXPORT_SYMBOL(md_boot_up_additional_operation);

void md_boot_ready_additional_operation(int md_id)
{
	//power off Audsys after DSP boot up ready
	//AudSys_Power_On(FALSE);
}
EXPORT_SYMBOL(md_boot_ready_additional_operation);

void additional_operation_before_stop_md(int md_id)
{
	//char buf = 1;
	//exec_ccci_kern_func(ID_LOCK_MD_SLEEP, &buf, sizeof(char));
	//md_slp_cnt = 0;
}
EXPORT_SYMBOL(additional_operation_before_stop_md);



/*********************************************************************************/
/* md&dsp watchdog irq handle function                                                                               */
/*                                                                                                                                    */
/*********************************************************************************/
static void md_wdt_notify(int md_id)
{
	if(md_id < MAX_MD_NUM) {
		if(wdt_notify_array[md_id] != NULL)
			wdt_notify_array[md_id](md_id);
	}
}

void ccci_md_wdt_notify_register(int md_id, int_func_int_t funcp)
{
	if(NULL == funcp){
		CCCI_MSG_INF(md_id, "ctl", "[Error]md wdt notify function pointer is NULL\n");
	} else if(wdt_notify_array[md_id]){
		CCCI_MSG_INF(md_id, "ctl", "wdt notify function pointer has registered\n");
	} else{
		wdt_notify_array[md_id] = funcp;
		CCCI_DBG_MSG(md_id, "ctl", "wdt notify func register OK\n");
	}
}
EXPORT_SYMBOL(ccci_md_wdt_notify_register);


void md_dsp_wdt_irq_en(int md_id)
{
	int irq_id;
	
	switch(md_id)
	{
		case MD_SYS1:
			irq_id = MT_MD_WDT1_IRQ_ID;
			break;
			
		case MD_SYS2:
			irq_id = MT_MD_WDT2_IRQ_ID;
			break;
			
		default:
			return;
	}

	if(atomic_read(&wdt_irq_en_count[md_id])==0) {
		enable_irq(irq_id);
		atomic_inc(&wdt_irq_en_count[md_id]);
	}

	return;
}
EXPORT_SYMBOL(md_dsp_wdt_irq_en);


void md_dsp_wdt_irq_dis(int md_id)
{
	int irq_id;
	
	switch(md_id)
	{
		case MD_SYS1:
			irq_id = MT_MD_WDT1_IRQ_ID;
			break;
			
		case MD_SYS2:
			irq_id = MT_MD_WDT2_IRQ_ID;
			break;
			
		default:
			return;
	}

	if(atomic_read(&wdt_irq_en_count[md_id]) == 1) {
		disable_irq(irq_id);
		atomic_dec(&wdt_irq_en_count[md_id]);
	}

	return;
}
EXPORT_SYMBOL(md_dsp_wdt_irq_dis);


void start_md_wdt_recov_timer(int md_id) 
{
	switch(md_id)
	{
	case MD_SYS1:
		mod_timer(&md1_wdt_recover_timer, jiffies+HZ/100);
		break;
	case MD_SYS2:
		mod_timer(&md2_wdt_recover_timer, jiffies+HZ/100);
		break;
	default:
		break;
	}
}
EXPORT_SYMBOL(start_md_wdt_recov_timer);


void stop_md_wdt_recov_timer(int md_id)
{
	switch(md_id)
	{
	case MD_SYS1:
		del_timer(&md1_wdt_recover_timer);
		break;
	case MD_SYS2:
		del_timer(&md2_wdt_recover_timer);
		break;
	default:
		break;
	}
}


static irqreturn_t md_wdt_isr(int irq, void *data __always_unused)
{
	unsigned int	sta = 0;
	int				md_id = -1;

	#ifdef ENABLE_MD_WDT_PROCESS
	switch (irq)
	{
	case MT_MD_WDT1_IRQ_ID:	
		stop_md_wdt_recov_timer(MD_SYS1);
		md_wdt_has_processed[MD_SYS1] = 1;
		if(md1_rgu_base) {
			sta = ccci_read32(WDT_MD_STA(md1_rgu_base));
			ccci_write32(WDT_MD_STA(md1_rgu_base),WDT_MD_MODE_KEY);
			md_id = MD_SYS1;
		}
		break;
	
	case MT_MD_WDT2_IRQ_ID:
		stop_md_wdt_recov_timer(MD_SYS2);
		md_wdt_has_processed[MD_SYS2] = 1;
		if(md2_rgu_base) {
			sta = ccci_read32(WDT_MD_STA(md2_rgu_base));
	        ccci_write32(WDT_MD_STA(md2_rgu_base), WDT_MD_MODE_KEY);
			md_id = MD_SYS2;
		}		
		break;

	default:
		break;
	}
	#endif

	CCCI_MSG_INF(md_id, "ctl", "MD_WDT_STA=%04x(md%d_irq=%d)\n", sta, (md_id+1), irq);

	if(sta!=0)
		md_wdt_notify(md_id);

	return IRQ_HANDLED;
}


static int md_dsp_irq_init(int md_id)
{
	int ret = 0;

	#ifdef ENABLE_MD_WDT_PROCESS
	int irq_num = 0;
	
	switch (md_id)
	{
		case MD_SYS1:
			irq_num = MT_MD_WDT1_IRQ_ID;
			break;
		
		case MD_SYS2:
			irq_num = MT_MD_WDT2_IRQ_ID;
			break;
			
		default:
			return -1;
	}
	
	ret = request_irq(irq_num, md_wdt_isr, IRQF_TRIGGER_FALLING, "MD-WDT", NULL);
	if (ret) {
		CCCI_MSG_INF(md_id, "ctl", "register MDWDT_IRQ fail: %d\n", ret);
		return ret;
	}

	atomic_set(&wdt_irq_en_count[md_id], 1);
	#endif
	
	return ret;
}


static void md_dsp_irq_deinit(int md_id)
{
	#ifdef ENABLE_MD_WDT_PROCESS
	int irq_num = 0;

	switch (md_id)
	{
		case MD_SYS1:
			irq_num = MT_MD_WDT1_IRQ_ID;
			break;
		
		case MD_SYS2:
			irq_num = MT_MD_WDT2_IRQ_ID;
			break;
			
		default:
			return;
	}
	
	free_irq(irq_num, NULL);
	disable_irq(irq_num);

	atomic_set(&wdt_irq_en_count[md_id], 0);
	#endif
}


static void recover_md_wdt_irq(unsigned long data)
{
	unsigned int	sta = 0;
	int				md_id = -1;
	unsigned long	flags;

	#ifdef ENABLE_MD_WDT_PROCESS
	switch (data)
	{
	case MD_SYS1:
		if(md1_rgu_base && (!md_wdt_has_processed[MD_SYS1])) {
			spin_lock_irqsave(&md1_wdt_mon_lock, flags);
			sta = ccci_read32(WDT_MD_STA(md1_rgu_base));
			ccci_write32(WDT_MD_STA(md1_rgu_base),WDT_MD_MODE_KEY);
			spin_unlock_irqrestore(&md1_wdt_mon_lock, flags);
			md_id = MD_SYS1;
		}
		break;
	
	case MD_SYS2:
		if(md2_rgu_base && (!md_wdt_has_processed[MD_SYS2])) {
			spin_lock_irqsave(&md2_wdt_mon_lock, flags);
			sta = ccci_read32(WDT_MD_STA(md2_rgu_base));
	        	ccci_write32(WDT_MD_STA(md2_rgu_base), WDT_MD_MODE_KEY);
			md_wdt_has_processed[MD_SYS2] = 0;
			spin_unlock_irqrestore(&md2_wdt_mon_lock, flags);
			md_id = MD_SYS2;
		}
		break;

	default:
		break;
	}
	#endif

	CCCI_MSG_INF(md_id, "ctl", "MD_WDT_STA=%04x(%d)(R)\n", sta, (md_id+1));

	if(sta!=0)
		md_wdt_notify(md_id);
}


/*********************************************************************************/
/*  API about md hardware handle                                                                                       */
/*                                                                                                                                    */
/*********************************************************************************/
static void map_md_side_register(int md_id)
{
	switch(md_id)
	{
		case MD_SYS1:
			//md1_infra_sys = (int)ioremap_nocache(MD_INFRA_BASE, 0x1000);
			//md1_rgu_base = (int)ioremap_nocache(0xD10C0000, 0x1000);
			md1_ccif_base = MD_CCIF0_BASE; // MD CCIF Bas;
			md1_boot_slave_Vector = (unsigned int)ioremap_nocache(0x20190000, 0x4);
			md1_boot_slave_Key = (unsigned int)ioremap_nocache(0x2019379C, 0x4);
			md1_boot_slave_En = (unsigned int)ioremap_nocache(0x20195488, 0x4);
			md1_rgu_base = (unsigned int)ioremap_nocache(0x20050000, 0x40);
			break;
			
		case MD_SYS2:
			//md2_infra_sys = (int)ioremap_nocache(MD_INFRA_BASE, 0x1000);
			//md2_rgu_base = (int)ioremap_nocache(0xD10C0000, 0x1000);
			md2_ccif_base = MD_CCIF1_BASE; // MD CCIF Bas;
			md2_boot_slave_Vector = (unsigned int)ioremap_nocache(0x30190000, 0x4);
			md2_boot_slave_Key = (unsigned int)ioremap_nocache(0x3019379C, 0x4);
			md2_boot_slave_En = (unsigned int)ioremap_nocache(0x30195488, 0x4);
			md2_rgu_base = (unsigned int)ioremap_nocache(0x30050000, 0x40);
			break;
			
		default:
			break;
	}

	return;
}


//========================================================
// Enable/Disable MD clock
//========================================================
static int ccci_en_md1_clock(void)
{
	unsigned long flags;
	int ret = 0;
	int need_power_on_md  = 0;
	spin_lock_irqsave(&md1_power_change_lock, flags);
	if(md1_power_on_cnt == 0) {
		md1_power_on_cnt++;
		need_power_on_md = 1;
	}
	spin_unlock_irqrestore(&md1_power_change_lock, flags);

	if(need_power_on_md)
		ret = md_power_on(0);

	return ret;
}


static int ccci_dis_md1_clock(unsigned int timeout)
{
	unsigned long flags;
	int ret = 0;
	int need_power_off_md  = 0;
	
	spin_lock_irqsave(&md1_power_change_lock, flags);
	if(md1_power_on_cnt == 1) {
		md1_power_on_cnt--;
		need_power_off_md = 1;
	}
	spin_unlock_irqrestore(&md1_power_change_lock, flags);

	if(need_power_off_md) {
		timeout = timeout/10; // Mili-seconds to Jiffies
		CCCI_DBG_MSG(MD_SYS1, "ctl", "off+\n");
		ret = md_power_off(0, timeout);
		CCCI_DBG_MSG(MD_SYS1, "ctl", "off-%d\n", ret);
	}

	return ret;
}


static int ccci_en_md2_clock(void)
{
	unsigned long flags;
	int ret = 0;
	int need_power_on_md  = 0;
	spin_lock_irqsave(&md2_power_change_lock, flags);
	if(md2_power_on_cnt == 0) {
		md2_power_on_cnt++;
		need_power_on_md = 1;
	}
	spin_unlock_irqrestore(&md2_power_change_lock, flags);

	if(need_power_on_md)
		ret = md_power_on(1);

	return ret;
}


static int ccci_dis_md2_clock(unsigned int timeout)
{
	unsigned long flags;
	int ret = 0;
	int need_power_off_md  = 0;
	spin_lock_irqsave(&md2_power_change_lock, flags);
	if(md2_power_on_cnt == 1) {
		md2_power_on_cnt--;
		need_power_off_md = 1;
	}
	spin_unlock_irqrestore(&md2_power_change_lock, flags);

	if(need_power_off_md) {
		timeout = timeout/10; // Mili-seconds to Jiffies
		CCCI_DBG_MSG(MD_SYS2, "ctl", "off+\n");
		ret = md_power_off(1, timeout);
		CCCI_DBG_MSG(MD_SYS2, "ctl", "off-%d\n", ret);
	}

	return ret;
}


//========================================================
// power on/off modem
//========================================================
int ccci_power_on_md(int md_id)
{
	CCCI_MSG_INF(md_id, "ctl", "[ccci/cci] power on md%d to run\n", md_id+1);
	
	switch(md_id)
	{
		case MD_SYS1:// MD1
			md_power_on(MD_SYS1);
		        ccci_write32(md1_rgu_base, 0x2200);
			break;
			
		case MD_SYS2:// MD2
		       *((volatile unsigned int*)0xF0000018) |= 0x00000020;//<==================
			power_on_md_ldo(MD_SYS2);

			*((volatile unsigned int*)0xF0000018) &= ~(1<<5);
		
			break;
			
		default:
			break;
	}
	
	return 0;
}
EXPORT_SYMBOL(ccci_power_on_md);


int ccci_power_down_md(int md_id)
{
	CCCI_MSG_INF(md_id, "ctl", "power down md%d\n", md_id+1);

	return 0;
}
EXPORT_SYMBOL(ccci_power_down_md);


//========================================================
// ungate/gate md
//========================================================
static void ungate_md1(void)
{
	CCCI_MSG_INF(0, "rst", "ungate_md1\n");

	if ( (!md1_boot_slave_Vector)||(!md1_boot_slave_Key) ||(!md1_boot_slave_En)) {
		CCCI_MSG_INF(0, "rst", "fail map md boot slave base!\n");
		return;
	}

	/* Power on MD MTCMOS*/
	ccci_en_md1_clock();

	/*set the start address to let modem to run*/ 
	ccci_write32(md1_rgu_base, 0x2200);
	ccci_write32(md1_boot_slave_Key, 0x3567C766); 
	ccci_write32(md1_boot_slave_Vector, 0x0); 
	ccci_write32(md1_boot_slave_En, 0xA3B66175); 
}

static void ungate_md2(void)
{
	CCCI_MSG_INF(1, "rst", "ungate_md2\n");

	if ( (!md2_boot_slave_Vector)||(!md2_boot_slave_Key) ||(!md2_boot_slave_En)) {
		CCCI_MSG_INF(1, "rst", "fail map md boot slave base!\n");
		return;
	}

	ccci_en_md2_clock();

	if(!md2_hw_mode) {
		md2_hw_mode = 1;
		switch_md_ldo(MD_SYS2, 1);
	}

	//*((volatile unsigned int*)0xF0000018) &= ~(1<<5);
	ccci_write32(md2_rgu_base, 0x2200);

	/*set the start address to let modem to run*/ 
	//*((volatile unsigned int*)(md2_rgu_base)) = 0x2200; //???anny
	ccci_write32(md2_boot_slave_Key, 0x3567C766); 
	ccci_write32(md2_boot_slave_Vector, 0x0); 
	ccci_write32(md2_boot_slave_En, 0xA3B66175); 
}

void gate_md1(unsigned int timeout)
{
	int ret;
	if(0 == md1_ccif_base){
		CCCI_MSG_INF(0, "ctl", "md_ccif_base map fail\n");
		return;
	}
	
	ret = ccci_dis_md1_clock(timeout);
	CCCI_MSG_INF(0, "ctl", "sleep ret %d\n", ret);

	/* Write MD CCIF Ack to clear AP CCIF busy register */
	CCCI_CTL_MSG(0, "Write MD CCIF Ack to clear AP CCIF busy register\n");
	ccci_write32(MD_CCIF_ACK(md1_ccif_base), ~0U);
}

void gate_md2(unsigned int timeout)
{
	if(0 == md2_rgu_base){
		CCCI_MSG_INF(1, "ctl", "md_rgu_base map fail\n");
		return;
	}

	ccci_dis_md2_clock(timeout);
	/* Write MD CCIF Ack to clear AP CCIF busy register */
	CCCI_CTL_MSG(1, "Write MD CCIF Ack to clear AP CCIF busy register\n");
	ccci_write32(MD_CCIF_ACK(md2_ccif_base), ~0U);
}


int let_md_stop(int md_id, unsigned int timeout)
{
	switch(md_id)
	{
		case MD_SYS1:
			gate_md1(timeout);
			break;
			
		case MD_SYS2:
			gate_md2(timeout);
			break;
			
		default:
			break;
	}
	return 0;
}
EXPORT_SYMBOL(let_md_stop);


int let_md_go(int md_id)
{
	unsigned int dbg_spare = *((volatile unsigned int*)(DEBUGTOP_BASE+0x1A010));
	
	CCCI_MSG_INF(md_id, "ctl", "let md%d to run\n", md_id+1);
	if(dbg_spare&0x1) {
		CCCI_MSG_INF(md_id, "ctl", "[Error]Debug mode(%08x) for md%d\n", md_id+1, dbg_spare);
		return -1;
	}
	
	switch(md_id)
	{
		case MD_SYS1:
			md_wdt_has_processed[MD_SYS1] = 0;
			ungate_md1();
			break;
			
		case MD_SYS2:
			md_wdt_has_processed[MD_SYS2] = 0;
			ungate_md2();
			break;
			
		default:
			break;
	}
	return 0;
}
EXPORT_SYMBOL(let_md_go);



/*********************************************************************************/
/* set ROM&Share memory address remapping register between AP and MD                               */
/*                                                                                                                                    */
/*********************************************************************************/
int set_ap_smem_remap(int md_id, unsigned int src, unsigned int des)
{
	unsigned int remap1_val = 0;
	unsigned int remap2_val = 0;
	
	if(!smem_remapped) {
		smem_remapped = 1;
		remap1_val =(((des>>24)|0x1)&0xFF)
				  + (((INVALID_ADDR>>16)|1<<8)&0xFF00)
				  + (((INVALID_ADDR>>8)|1<<16)&0xFF0000)
				  + (((INVALID_ADDR>>0)|1<<24)&0xFF000000);
		
		remap2_val =(((INVALID_ADDR>>24)|0x1)&0xFF)
				  + (((INVALID_ADDR>>16)|1<<8)&0xFF00)
				  + (((INVALID_ADDR>>8)|1<<16)&0xFF0000)
				  + (((INVALID_ADDR>>0)|1<<24)&0xFF000000);
		
		CCCI_DBG_MSG(md_id, "ctl", "AP Smem remap: [%08x]->[%08x](%08x:%08x)\n", src, des, remap1_val, remap2_val);

		#ifdef 	ENABLE_MEM_REMAP_HW
		ccci_write32(AP_BANK4_MAP0(ap_mcu_reg_base), remap1_val);
		ccci_write32(AP_BANK4_MAP1(ap_mcu_reg_base), remap2_val);
		ccci_write32(AP_BANK4_MAP_UPDATE(ap_mcu_reg_base),  ccci_read32(AP_BANK4_MAP_UPDATE(ap_mcu_reg_base))^0x80000000);
		#endif
								
	}
	return 0;
}


int set_md_smem_remap(int md_id, unsigned int src, unsigned int des)
{
	unsigned int remap1_val = 0;
	unsigned int remap2_val = 0;
	des-=KERNEL_PHY_BASE;
	
	switch(md_id)
	{
		case MD_SYS1:
			remap1_val =(((des>>24)|0x1)&0xFF)
					  + (((INVALID_ADDR>>16)|1<<8)&0xFF00)
					  + (((INVALID_ADDR>>8)|1<<16)&0xFF0000)
					  + (((INVALID_ADDR>>0)|1<<24)&0xFF000000);
			remap2_val =(((INVALID_ADDR>>24)|0x1)&0xFF)
					  + (((INVALID_ADDR>>16)|1<<8)&0xFF00)
					  + (((INVALID_ADDR>>8)|1<<16)&0xFF0000)
					  + (((INVALID_ADDR>>0)|1<<24)&0xFF000000);
			#ifdef 	ENABLE_MEM_REMAP_HW
		        ccci_write32(MD1_BANK4_MAP0(ap_infra_base), remap1_val);
		        ccci_write32(MD1_BANK4_MAP1(ap_infra_base), remap2_val);
			#endif

			break;

		case MD_SYS2:
			remap1_val =(((des>>24)|0x1)&0xFF)
					  + (((INVALID_ADDR>>16)|1<<8)&0xFF00)
					  + (((INVALID_ADDR>>8)|1<<16)&0xFF0000)
					  + (((INVALID_ADDR>>0)|1<<24)&0xFF000000);
			remap2_val =(((INVALID_ADDR>>24)|0x1)&0xFF)
					  + (((INVALID_ADDR>>16)|1<<8)&0xFF00)
					  + (((INVALID_ADDR>>8)|1<<16)&0xFF0000)
					  + (((INVALID_ADDR>>0)|1<<24)&0xFF000000);
			#ifdef 	ENABLE_MEM_REMAP_HW
		        ccci_write32(MD2_BANK4_MAP0(ap_infra_base), remap1_val);
		        ccci_write32(MD2_BANK4_MAP1(ap_infra_base), remap2_val);
			#endif

			break;

		default:
			break;
	}

	CCCI_DBG_MSG(md_id, "ctl", "MD%d smem remap:[%08x]->[%08x](%08x:%08x)\n", md_id+1, src, des, remap1_val, remap2_val);
	return 0;
}


int set_md_rom_rw_mem_remap(int md_id, unsigned int src, unsigned int des)
{
	unsigned int remap1_val = 0;
	unsigned int remap2_val = 0;
	des -= KERNEL_PHY_BASE;
	
	switch(md_id)
	{
		case MD_SYS1:
			remap1_val =(((des>>24)|0x1)&0xFF)
					  + (((INVALID_ADDR>>16)|1<<8)&0xFF00)
					  + (((INVALID_ADDR>>8)|1<<16)&0xFF0000)
					  + (((INVALID_ADDR>>0)|1<<24)&0xFF000000);
			remap2_val =(((INVALID_ADDR>>24)|0x1)&0xFF)
					  + (((INVALID_ADDR>>16)|1<<8)&0xFF00)
					  + (((INVALID_ADDR>>8)|1<<16)&0xFF0000)
					  + (((INVALID_ADDR>>0)|1<<24)&0xFF000000);
			#ifdef 	ENABLE_MEM_REMAP_HW
		        ccci_write32(MD1_BANK0_MAP0(ap_infra_base), remap1_val);
		        ccci_write32(MD1_BANK0_MAP1(ap_infra_base), remap2_val);
			#endif

			break;
			
		case MD_SYS2:
			remap1_val =(((des>>24)|0x1)&0xFF)
					  + (((INVALID_ADDR>>16)|1<<8)&0xFF00)
					  + (((INVALID_ADDR>>8)|1<<16)&0xFF0000)
					  + (((INVALID_ADDR>>0)|1<<24)&0xFF000000);
			remap2_val =(((INVALID_ADDR>>24)|0x1)&0xFF)
					  + (((INVALID_ADDR>>16)|1<<8)&0xFF00)
					  + (((INVALID_ADDR>>8)|1<<16)&0xFF0000)
					  + (((INVALID_ADDR>>0)|1<<24)&0xFF000000);
			#ifdef 	ENABLE_MEM_REMAP_HW
		        ccci_write32(MD2_BANK0_MAP0(ap_infra_base), remap1_val);
		        ccci_write32(MD2_BANK0_MAP1(ap_infra_base), remap2_val);
			#endif

			break;
			
		default:
			break;
	}

	CCCI_DBG_MSG(md_id, "ctl", "MD%d ROM mem remap:[%08x]->[%08x](%08x:%08x)\n", md_id+1, src, des, remap1_val, remap2_val);
	return 0;
}



/*********************************************************************************/
/* ccci platform initial function                                                                                             */
/*                                                                                                                                    */
/*********************************************************************************/
int ccci_ipo_h_platform_restore(int md_id)
{
	int				ret = 0;
	unsigned int	smem_addr;
	unsigned int	md_mem_addr;
	int				need_restore = 0;
	//int				wdt_irq = 0;
	
	smem_remapped = 0;

	switch(md_id)
	{
		case MD_SYS1:
			//wdt_irq = MT_MD_WDT1_IRQ_ID;
			need_restore = 1;
			break;
			
		case MD_SYS2:
			//wdt_irq = MT_MD_WDT2_IRQ_ID;
			need_restore = 1;
			md2_hw_mode = 0;
			break;
			
		default:
			CCCI_MSG("register MDWDT irq fail: invalid md id(%d)\n", md_id);
			return -1;
	}

	if(need_restore) {
		smem_addr = md_mem_layout_tab[md_id].smem_region_phy_before_map;
		md_mem_addr = md_mem_layout_tab[md_id].md_region_phy;

		ret += set_ap_smem_remap(md_id,0x40000000, smem_addr);
		ret += set_md_smem_remap(md_id, 0x40000000, smem_addr);
		ret += set_md_rom_rw_mem_remap(md_id, 0x00000000, md_mem_addr); 
	}

	if(ret)
		return -1;

	// Configure AXI slow down to 120MHz for MD 
	*TOPAXI_AXI_ASLICE_CRL(ap_infra_base) &= 0xFFFFF33F;
	// Configure TOPAXI_MD_BUS_CTRL
	ap_md_bus_config();

	//#ifdef ENABLE_MD_WDT_PROCESS
	#if 0
	ret = request_irq(wdt_irq, md_wdt_isr, IRQF_TRIGGER_FALLING, "MD-WDT", NULL);
	if (ret) {
		CCCI_MSG_INF(md_id, "ctl", "register MDWDT irq fail: %d\n",ret);
		return ret;
	}

	atomic_set(&wdt_irq_en_count[md_id], 1);
	#endif

	return 0;
}
EXPORT_SYMBOL(ccci_ipo_h_platform_restore);


int md_init(int md_id)
{
	int ret = 0;

	ret = ccci_alloc_smem(md_id);
	md_init_stage_flag[md_id] |= CCCI_ALLOC_SMEM_DONE;
	if(ret < 0) {
		CCCI_MSG_INF(md_id, "ctl", "MD memory allocate fail: %d\n", ret);
		return ret;
	}

	md_mem_layout_tab[md_id].md_region_vir = 
				(unsigned int)ioremap_nocache(md_mem_layout_tab[md_id].md_region_phy, MD_IMG_DUMP_SIZE);
	if (!md_mem_layout_tab[md_id].md_region_vir)
	{
		CCCI_MSG_INF(md_id, "ctl", "MD region ioremap fail!\n");
		return -ENOMEM;
	}
	CCCI_DBG_COM_MSG("md_region_vir:%p\n",(void*)(md_mem_layout_tab[0].md_region_vir));

	CCCI_MSG_INF(md_id, "ctl", "md%d: md_rom<P:0x%08x><V:0x%08x>, md_smem<P:0x%08x><V:0x%08x>\n", md_id+1,
			md_mem_layout_tab[md_id].md_region_phy, md_mem_layout_tab[md_id].md_region_vir,
			md_mem_layout_tab[md_id].smem_region_phy_before_map, md_mem_layout_tab[md_id].smem_region_vir);

	md_init_stage_flag[md_id] |= CCCI_MAP_MD_CODE_DONE;

	map_md_side_register(md_id);
	md_init_stage_flag[md_id] |= CCCI_MAP_CTL_REG_DONE;

	if ((ret = md_dsp_irq_init(md_id)) != 0) 
		return ret;
	md_init_stage_flag[md_id] |= CCCI_WDT_IRQ_REG_DONE;

	load_img_cfg(md_id);

	memset(md_img_info_str[md_id], 0, sizeof(md_img_info_str[md_id]));

	//register_ccci_kern_func_by_md_id(0, ID_LOCK_MD_SLEEP, lock_md_sleep);
	//register_ccci_kern_func_by_md_id(0, ID_ACK_MD_SLEEP, ack_md_sleep);
	//register_ccci_sys_call_back(0, MD_SLP_REQUEST, lock_sleep_cb);
//	register_suspend_notify(md_id, SLP_ID_MD_FAST_DROMANT, md_fast_dormancy);
	register_ccci_kern_func_by_md_id(md_id, ID_GET_TXPOWER, get_txpower);
	
	return ret;
}


void md_deinit(int md_id)
{
	if(md_init_stage_flag[md_id]&CCCI_ALLOC_SMEM_DONE)
		ccci_free_smem(md_id);

	if(md_init_stage_flag[md_id]&CCCI_MAP_MD_CODE_DONE)
		iounmap((void*)md_mem_layout_tab[md_id].md_region_vir);

	if(md_init_stage_flag[md_id]&CCCI_WDT_IRQ_REG_DONE)
		md_dsp_irq_deinit(md_id);

}


int platform_init(int md_id, int power_down)
{
	int ret = 0;
	
	ret = md_init(md_id);

	return ret;
}
EXPORT_SYMBOL(platform_init);


void platform_deinit(int md_id)
{
	md_deinit(md_id);
}
EXPORT_SYMBOL(platform_deinit);


int __init ccci_mach_init(void)
{
	int ret = 0;

	CCCI_MSG("Ver. %s, @ %s %s\n", CCCI_VERSION, __DATE__, __TIME__); 
		
	CCCI_CTL_MSG(0, "kernel base:0x%08X, dram_size:0x%08X\n", get_phys_offset(), get_max_DRAM_size());

	#ifdef ENABLE_MD_IMG_SECURITY_FEATURE
	if ((ret = sec_boot_init()) !=0) {
		CCCI_MSG("sec_boot_init fail: %d\n",ret);
		ret= -EIO;
		return ret;
	}

	if(sec_lib_version_check()!= 0) {
		CCCI_MSG("sec lib version check error\n");
		ret= -EIO;
		return ret;
	}
	#endif
	
	ccci_get_platform_ver(ap_platform);

	ap_infra_base = INFRACFG_BASE;
	ap_mcu_reg_base = MCUSYS_CFGREG_BASE;
	
	// Cofigure AXI slow down to 120MHz for MD, which change default register value request by hw designer
	ccci_write32(TOPAXI_AXI_ASLICE_CRL(ap_infra_base), ccci_read32(TOPAXI_AXI_ASLICE_CRL(ap_infra_base))&0xFFFFF33F);
	CCCI_DBG_COM_MSG("TOPAXI_AXI_ASLICE_CRL=0x%08x", ccci_read32(TOPAXI_AXI_ASLICE_CRL(ap_infra_base)));

	// Configure TOPAXI_MD_BUS_CTRL
	ap_md_bus_config();

	return ret;
}


void __exit ccci_mach_exit(void)
{
}


module_init(ccci_mach_init);
module_exit(ccci_mach_exit);

MODULE_DESCRIPTION("CCCI Plaform Driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("MTK");
