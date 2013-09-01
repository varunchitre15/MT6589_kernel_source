/*****************************************************************************
 *
 * Filename:
 * ---------
 *   ccci_md.c
 *
 * Project:
 * --------
 *   ALPS
 *
 * Description:
 * ------------
 *   MT65XX MD initialization and handshake driver
 *
 *
 ****************************************************************************/

#include <linux/module.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <linux/firmware.h>
#include <linux/io.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/fs.h>
#include <linux/syscalls.h>
#include <linux/timer.h>
#include <asm/atomic.h>
#include <ccci.h>
#include <linux/wakelock.h>



/****************************************************************************
 * DEBUG UTILITIES
 ****************************************************************************/

char *ccci_version = "v1.4 20120618";

#ifdef CCCI_MD_DEBUG_ON
#define DUMP_CCCI_MD_DATA() dump_ccci_md_data()
#else
#define DUMP_CCCI_MD_DATA()
#endif

#define MDLOGGER_FILE_PATH "/data/mdl/mdl_config"

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36))
DECLARE_MUTEX(ccci_mb_mutex);
DECLARE_MUTEX(ccci_reset_mutex);
#else
DEFINE_SEMAPHORE(ccci_mb_mutex);
DEFINE_SEMAPHORE(ccci_reset_mutex);
#endif

MD_CALL_BACK_HEAD_T md_notifier={
	#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36))
	.lock=SPIN_LOCK_UNLOCKED,
	#else
	.lock=__SPIN_LOCK_UNLOCKED(die.lock),
	#endif
	.next=NULL,
};


static void ex_monitor_func(unsigned long);
volatile int md_boot_stage = MD_BOOT_STAGE_0;
static atomic_t md_ex=ATOMIC_INIT(0);
static atomic_t md_ex_ok=ATOMIC_INIT(0);
static atomic_t md_ex_dump_reentrant=ATOMIC_INIT(0);
atomic_t md_reset_on_going=ATOMIC_INIT(0);
static DEFINE_TIMER(md_ex_monitor,ex_monitor_func,0,0);
//extern int *md_img_vir;
static void md_boot_up_timeout_func(unsigned long);
static DEFINE_TIMER(md_boot_up_check_timer, md_boot_up_timeout_func, 0 , 0);
int lock_md_sleep(char *buf, unsigned int len);


struct ccci_cores_exch_data *cores_exch_data;
static struct ccci_reset_sta reset_sta[NR_CCCI_RESET_USER]; 
static int *md_ex_log;

int is_first_boot = 1;
unsigned int md_ex_type = 0;
volatile int md_ex_flag = 0;

// Notice:It's for MT6575/MT6577 hardware limitation, that edge irq will be lost at CPU shutdown mode
static struct wake_lock trm_wake_lock;
volatile atomic_t data_connect_sta;

volatile unsigned int md_slp_cnt = 0;
volatile unsigned int md_slp_lock_ack = 0;
volatile unsigned int md_slp_unlock_ack = 0;
static spinlock_t md_slp_lock;


//-----------external variable define-----------//
extern char image_buf[]; 
extern int  valid_image_info;
extern unsigned int ccci_msg_mask;


//-----------share memory address------------//
int ccci_smem_size = 0;
int *ccci_smem_virt = NULL;
dma_addr_t ccci_smem_phy;

// For MD log
#ifdef MTK_DSPIRDBG
#define CCCI_MD_LOG_SIZE  (137*1024*4 + 384*1024 + 64*1024)
#else
//#define CCCI_MD_LOG_SIZE  (137*1024*4)
#define CCCI_MD_LOG_SIZE  (137*1024*4+64*1024) // Add 64K share memory
#endif

static int ccci_mdlog_smem_base_virt;
static int ccci_mdlog_smem_base_phy;
static int ccci_mdlog_smem_size;
// PCM
static int ccci_pcm_smem_base_virt;
static int ccci_pcm_smem_base_phy;
static int ccci_pcm_smem_size;
// PMIC
static int ccci_pmic_smem_base_virt;
static int ccci_pmic_smem_base_phy;
static int ccci_pmic_smem_size;
// FS
static int ccci_fs_smem_base_virt;
static int ccci_fs_smem_base_phy;
static int ccci_fs_smem_size;
//RPC
static int ccci_rpc_smem_base_virt;
static int ccci_rpc_smem_base_phy;
static int ccci_rpc_smem_size;
// TTY
static int ccci_tty_smem_base_virt[CCCI_TTY_BUFF_NR];
static int ccci_tty_smem_base_phy[CCCI_TTY_BUFF_NR];
static int ccci_tty_smem_size[CCCI_TTY_BUFF_NR];
// EXP
static int ccci_exp_smem_base_virt;
static int ccci_exp_smem_base_phy;
static int ccci_exp_smem_size;
//IPC
static int ccci_ipc_smem_base_virt;
static int ccci_ipc_smem_base_phy;
static int ccci_ipc_smem_size;
// SYS
static int ccci_sys_smem_base_virt;
 int ccci_sys_smem_base_phy;
static int ccci_sys_smem_size;



/** During modem booting, it would occupy much memory bandwidth with high
    priority which would interfere with DPI display. To prevent from this
    issue, disable MD high priority bit before modem booting is started 
    and re-eanble it after modem booting is finished.
*/
void md_call_chain(MD_CALL_BACK_HEAD_T *head,unsigned long data)
{
	MD_CALL_BACK_QUEUE *queue;
	unsigned long flag;
	
	spin_lock_irqsave(&head->lock,flag);
	queue=head->next;
	while (queue)
	{
		queue->call(queue,data);
		queue=queue->next;
	}
	spin_unlock_irqrestore(&head->lock,flag);
}

int md_register_call_chain(MD_CALL_BACK_HEAD_T *head,MD_CALL_BACK_QUEUE *queue)
{
	unsigned long flag;
	spin_lock_irqsave(&head->lock,flag);
	queue->next=head->next;
	head->next=queue;
	spin_unlock_irqrestore(&head->lock,flag);
	return 0;
}

int md_unregister_call_chain(MD_CALL_BACK_HEAD_T *head,MD_CALL_BACK_QUEUE *queue)
{
	unsigned long flag;
	int ret=-1;
	MD_CALL_BACK_QUEUE **_queue=NULL;
	spin_lock_irqsave(&head->lock,flag);
	_queue=&head->next;
	while(*_queue)
	{
		if ((*_queue)->next==queue)
		{
			(*_queue)->next=queue->next;
			queue->next=NULL;
			ret=0;
			break;
		}
		_queue=&(*_queue)->next;
	}
	spin_unlock_irqrestore(&head->lock,flag);
	return ret;
}

#ifdef CCCI_MD_DEBUG_ON
static void dump_ccci_md_data(void)
{
	int i;
	struct modem_runtime_t *runtime = (struct modem_runtime_t *)MD_RUNTIME_ADDR;
	char ctmp[12];
	int *p;

	p = (int*)ctmp;
	*p = runtime->Prefix;
	p++;
	*p = runtime->Platform_L;
	p++;
	*p = runtime->Platform_H;

	CCCI_MSG("**********************************************\n");
	CCCI_MSG("Prefix\t\t%c%c%c%c\n", ctmp[0], ctmp[1], ctmp[2], ctmp[3]);
	CCCI_MSG("Platform_L\t\t%c%c%c%c\n", ctmp[4], ctmp[5], ctmp[6], ctmp[7]);
	CCCI_MSG("Platform_H\t\t%c%c%c%c\n", ctmp[8], ctmp[9], ctmp[10], ctmp[11]);

	CCCI_MSG("DriverVersion\t\t0x%x\n", runtime->DriverVersion);
	CCCI_MSG("BootChannel\t\t%d\n", runtime->BootChannel);
	CCCI_MSG("BootingStartID(Mode)\t\t%d\n", runtime->BootingStartID);
	CCCI_MSG("BootAttributes\t\t%d\n", runtime->BootAttributes);
	CCCI_MSG("BootReadyID\t\t%d\n", runtime->BootReadyID);
        CCCI_CTL_MSG("MdlogShareMemBase\t\t0x%x\n", runtime->MdlogShareMemBase);
	CCCI_CTL_MSG("MdlogShareMemSize\t\t%d\n", runtime->MdlogShareMemSize);
	CCCI_CTL_MSG("PcmShareMemBase\t\t0x%x\n", runtime->PcmShareMemBase);
	CCCI_CTL_MSG("PcmShareMemSize\t\t%d\n", runtime->PcmShareMemSize);

	CCCI_CTL_MSG("UartPortNum\t\t%d\n", runtime->UartPortNum);
	for (i = 0; i < UART_MAX_PORT_NUM; i++) {
		CCCI_CTL_MSG("UartShareMemBase[%d]\t0x%x\n", i, runtime->UartShareMemBase[i]);
		CCCI_CTL_MSG("UartShareMemSize[%d]\t%d\n", i, runtime->UartShareMemSize[i]);
	}

	CCCI_CTL_MSG("FileShareMemBase\t0x%x\n", runtime->FileShareMemBase);
	CCCI_CTL_MSG("FileShareMemSize\t%d\n", runtime->FileShareMemSize);
	CCCI_CTL_MSG("RpcShareMemBase\t0x%x\n", runtime->RpcShareMemBase);
	CCCI_CTL_MSG("RpcShareMemSize\t%d\n", runtime->RpcShareMemSize);
	CCCI_CTL_MSG("PmicShareMemBase\t0x%x\n", runtime->PmicShareMemBase);
	CCCI_CTL_MSG("PmicShareMemSize\t%d\n", runtime->PmicShareMemSize);
	CCCI_CTL_MSG("ExceShareMemBase\t0x%x\n", runtime->ExceShareMemBase);
	CCCI_CTL_MSG("ExceShareMemSize\t%d\n", runtime->ExceShareMemSize);
	CCCI_CTL_MSG("SysShareMemBase\t\t0x%x\n", runtime->SysShareMemBase);
	CCCI_CTL_MSG("SysShareMemSize\t\t%d\n", runtime->SysShareMemSize);
	CCCI_CTL_MSG("IPCShareMemBase\t\t0x%x\n",runtime->IPCShareMemBase);
	CCCI_CTL_MSG("IPCShareMemSize\t\t0x%x\n",runtime->IPCShareMemSize);
	CCCI_CTL_MSG("CheckSum\t\t%d\n", runtime->CheckSum);

	p = (int*)ctmp;
	*p = runtime->Postfix;
	CCCI_MSG("Postfix\t\t%c%c%c%c\n", ctmp[0], ctmp[1], ctmp[2], ctmp[3]);

	CCCI_MSG("----------------------------------------------\n");

	CCCI_MSG("ccci_smem_virt\t%x\n", (unsigned int)ccci_smem_virt);
	CCCI_MSG("ccci_smem_phy\t%x\n", (unsigned int)ccci_smem_phy);
	CCCI_MSG("ccci_smem_size\t%08x\n", ccci_smem_size);

	CCCI_MSG("ccci_pcm_smem_base_virt\t%x\n", ccci_pcm_smem_base_virt);
	CCCI_MSG("ccci_pcm_smem_base_phy\t%x\n", ccci_pcm_smem_base_phy);
	CCCI_MSG("ccci_pcm_smem_size\t%d\n", ccci_pcm_smem_size);

	CCCI_MSG("ccci_fs_smem_base_virt\t%x\n", ccci_fs_smem_base_virt);
	CCCI_MSG("ccci_fs_smem_base_phy\t%x\n", ccci_fs_smem_base_phy);
	CCCI_MSG("ccci_fs_smem_size\t%d\n", ccci_fs_smem_size);

	for (i = 0; i < UART_MAX_PORT_NUM; i++) {
		CCCI_MSG("ccci_tty_smem_base_virt[%d]\t%x\n", i, ccci_tty_smem_base_virt[i]);
		CCCI_MSG("ccci_tty_smem_base_phy[%d]\t%x\n", i, ccci_tty_smem_base_phy[i]);
		CCCI_MSG("ccci_tty_smem_size[%d]\t%d\n", i, ccci_tty_smem_size[i]);
	}

	CCCI_MSG("ccci_ipc_smem_base_virt\t%x\n", ccci_ipc_smem_base_virt);
	CCCI_MSG("ccci_ipc_smem_base_phy\t%x\n", ccci_ipc_smem_base_phy);
	CCCI_MSG("ccci_ipc_smem_size\t%d\n", ccci_ipc_smem_size);
	CCCI_MSG("**********************************************\n");
}
#endif


int ccci_mdlog_base_req(void *addr_phy, int *len)
{
    if (addr_phy == NULL) {
        return CCCI_INVALID_PARAM;
    }
    if (len == NULL) {
        return CCCI_INVALID_PARAM;
    }

    *(unsigned int *)addr_phy = (unsigned int)ccci_mdlog_smem_base_phy;
    *len = ccci_mdlog_smem_size;

    return CCCI_SUCCESS;
}

/*
 * ccci_pcm_base_reg: get PCM buffer information
 * @addr: kernel space buffer to store the address of PCM buffer
 * @len: kernel space buffer to store the length of PCM buffer
 * return 0 for success; negative value for failure
 */
int ccci_pcm_base_req(void *addr_phy, int *len)
{
    if (addr_phy == NULL) {
        return CCCI_INVALID_PARAM;
    }
    if (len == NULL) {
        return CCCI_INVALID_PARAM;
    }

    *(unsigned int *)addr_phy = (unsigned int)ccci_pcm_smem_base_phy;
    *len = ccci_pcm_smem_size;

    return CCCI_SUCCESS;
}

/*
 * ccci_uart_setup: setup TTY share buffer
 * @port: UART port to setup
 * @addr: physical address of TTY share buffer
 * @len: length of TTY share buffer
 * return 0 for success; negative value for failure
 */
int ccci_uart_setup(int port, int *addr_virt, int *addr_phy, int *len)
{
    if (port >= CCCI_TTY_BUFF_NR) {
        return CCCI_INVALID_PARAM;
    } 
	else {
	*addr_virt = ccci_tty_smem_base_virt[port];
	*addr_phy = ccci_tty_smem_base_phy[port];
	*len = ccci_tty_smem_size[port];
        return CCCI_SUCCESS;
    }
}

/*
 * ccci_uart_base_req: get TTY share buffer information
 * @port: UART port to get
 * @addr: kernel space buffer to store the address of TTY share buffer
 * @len: kernel space buffer to store the length of TTY share buffer
 * return 0 for success; negative value for failure
 */
int ccci_uart_base_req(int port, void *addr_phy, int *len)
{
    if (port >= CCCI_TTY_BUFF_NR) {
        return CCCI_INVALID_PARAM;
    }
    if (addr_phy == NULL) {
        return CCCI_INVALID_PARAM;
    }
    if (len == NULL) {
        return CCCI_INVALID_PARAM;
    }

    *(int *)addr_phy = ccci_tty_smem_base_phy[port];
    *len = ccci_tty_smem_size[port];

    return CCCI_SUCCESS;
}

/*
 * ccci_fs_setup: setup CCCI_FS share buffer
 * @addr: physical address of CCCI_FS share buffer
 * @len: length of CCCI_FS share buffer
 * return 0 for success; negative value for failure
 */
int ccci_fs_setup(int *addr_virt, int *addr_phy, int *len)
{
	*addr_virt = ccci_fs_smem_base_virt;
	*addr_phy = ccci_fs_smem_base_phy;
	*len = ccci_fs_smem_size;
	return CCCI_SUCCESS;
}

/*
 * ccci_fs_base_req: get CCCI_FS share buffer information
 * @addr: kernel space buffer to store the address of CCCI_FS share buffer
 * @len: kernel space buffer to store the length of CCCI_FS share buffer
 * return 0 for success; negative value for failure
 */
int ccci_fs_base_req(void *addr_phy, int *len)
{
    if (addr_phy == NULL) {
        return CCCI_INVALID_PARAM;
    }
    if (len == NULL) {
        return CCCI_INVALID_PARAM;
    }
    *(int *)addr_phy = ccci_fs_smem_base_phy;
    *len = ccci_fs_smem_size;

    return CCCI_SUCCESS;
}

/*
 * ccci_rpc_setup: setup CCCI_FS share buffer
 * @addr: physical address of CCCI_FS share buffer
 * @len: length of CCCI_FS share buffer
 * return 0 for success; negative value for failure
 */
int ccci_rpc_setup(int *addr_virt, int *addr_phy, int *len)
{
	*addr_virt = ccci_rpc_smem_base_virt;
	*addr_phy = ccci_rpc_smem_base_phy;
	*len = ccci_rpc_smem_size;
	return CCCI_SUCCESS;
}
/*
 * ccci_rpc_base_req: get CCCI_FS share buffer information
 * @addr: kernel space buffer to store the address of CCCI_FS share buffer
 * @len: kernel space buffer to store the length of CCCI_FS share buffer
 * return 0 for success; negative value for failure
 */
int ccci_rpc_base_req(void *addr_phy, int *len)
{
    if (addr_phy == NULL) {
        return CCCI_INVALID_PARAM;
    }
    if (len == NULL) {
        return CCCI_INVALID_PARAM;
    }
    *(int *)addr_phy = ccci_rpc_smem_base_phy;
    *len = ccci_rpc_smem_size;

    return CCCI_SUCCESS;
}



/*
 * ccci_pmic_setup: setup PMIC share buffer
 * @addr: physical address of PMIC share buffer
 * @len: length of PMIC share buffer
 * return 0 for success; negative value for failure
 */
int ccci_pmic_setup(int *addr_virt, int *addr_phy, int *len)
{
	*addr_virt = ccci_pmic_smem_base_virt;
	*addr_phy = ccci_pmic_smem_base_phy;
	*len = ccci_pmic_smem_size;
	return CCCI_SUCCESS;
}

/*
 * ccci_pmic_base_req: get PMIC share buffer information
 * @addr: kernel space buffer to store the address of PMIC share buffer
 * @len: kernel space buffer to store the length of PMIC share buffer
 * return 0 for success; negative value for failure
 */
int ccci_pmic_base_req(void *addr_phy, int *len)
{
    if (addr_phy == NULL) {
        return CCCI_INVALID_PARAM;
    }
    if (len == NULL) {
        return CCCI_INVALID_PARAM;
    }
    *(int *)addr_phy = (int)ccci_pmic_smem_base_phy;
    *len = ccci_pmic_smem_size;

    return CCCI_SUCCESS;
}




int ccci_ipc_setup(int *addr_virt, int *addr_phy, int *len)
{
	*addr_virt = ccci_ipc_smem_base_virt;
	*addr_phy = ccci_ipc_smem_base_phy;
	*len = ccci_ipc_smem_size;
	return CCCI_SUCCESS;
}


int ccci_ipc_base_req( void *addr_phy, int *len)
{
	if (addr_phy==NULL)
		return CCCI_INVALID_PARAM;
	if (len==NULL)
		return CCCI_INVALID_PARAM;

    *(int *)addr_phy = (int)ccci_ipc_smem_base_phy+offset_of(CCCI_IPC_MEM,buffer);
    *len = sizeof(CCCI_IPC_BUFFER);

    return CCCI_SUCCESS;
}

/*
 * read2boot: check if system is ready to boot up MODEM
 * return 1 for success; return 0 for failure
 */

static int ready2boot(void)
{
    int addr, len;

    ccci_uart_base_req(0, &addr, &len);
    if (addr == 0 || len == 0) {
        CCCI_MSG_INF("ctl", "UART base address is not setup\n");
        return 0;
    }

    ccci_fs_base_req(&addr, &len);
    if (addr == 0 || len == 0) {
        CCCI_MSG_INF("ctl", "CCCI_FS base address is not setup\n");
        return 0;
    }

    ccci_pmic_base_req(&addr, &len);
    if (addr == 0 || len == 0) {
        CCCI_MSG_INF("ctl", "PMIC base address is not setup\n");
        return 0;
    }
	
    ccci_ipc_base_req(&addr, &len);
    if (addr == 0 || len == 0) {
        CCCI_MSG_INF("ctl", "CCCI_IPC base address is not setup\n");
        return 0;
    }

   if (platform_ready2boot())
   	return 0;

    return 1;
}

#if 0
/*
static void dump_firmware(void)
{
    int i;
    printk(KERN_ERR "CCCI_MD: [Exception] Mem dump:\n");
    for(i = 0; i < (MD_IMG_DUMP_SIZE / 4); i += 4)
    {
        printk(KERN_ERR "0x%08X %08X %08X %08X %08X\n", 
	       (unsigned int)&md_img_vir[i],
               md_img_vir[i], md_img_vir[i + 1],
               md_img_vir[i + 2], md_img_vir[i + 3]);
    }
#if defined(CONFIG_ARCH_MT6573)
    printk(KERN_ERR "CCCI_MD: [Exception] DSP dump:\n");
    for(i = 0; i < (MD_IMG_DUMP_SIZE / 4); i += 4)
    {
        printk(KERN_ERR "0x%08X %08X %08X %08X %08X\n", 
	       (unsigned int)&dsp_img_vir[i],
               dsp_img_vir[i], dsp_img_vir[i + 1],
               dsp_img_vir[i + 2], dsp_img_vir[i + 3]);
    }
#endif
}
*/
#endif

/*
 * ccci_md_exception: handle modem exception
 */

static void ccci_md_exception(unsigned int trusted)
{
	DEBUG_INFO_T debug_info;
	EX_LOG_T *ex_info = (EX_LOG_T*)md_ex_log;
	memset(&debug_info,0,sizeof(DEBUG_INFO_T));
	debug_info.type = ex_info->header.ex_type;
	md_ex_type = ex_info->header.ex_type;
	CCCI_MSG_INF("ctl", "\n");
	CCCI_MSG_INF("ctl", "MODEM exception occurs\n");
	switch (debug_info.type) 
	{
	case MD_EX_TYPE_INVALID:
		debug_info.name="INVALID";
        	break;

	case MD_EX_TYPE_UNDEF:
		debug_info.name="UNDEF";
		break;

	case MD_EX_TYPE_SWI:
		debug_info.name="SWI";
		break;

	case MD_EX_TYPE_PREF_ABT:
		debug_info.name="PREFETCH ABORT";
		break;

	case MD_EX_TYPE_DATA_ABT:
		debug_info.name="DATA ABORT";
		break;

	case MD_EX_TYPE_ASSERT:
		debug_info.name="ASSERT";
		snprintf(debug_info.assert.file_name,sizeof(debug_info.assert.file_name),
				ex_info->content.assert.filename);	
		debug_info.assert.line_num = ex_info->content.assert.linenumber;
		debug_info.assert.parameters[0] = ex_info->content.assert.parameters[0];
		debug_info.assert.parameters[1] = ex_info->content.assert.parameters[1];
		debug_info.assert.parameters[2] = ex_info->content.assert.parameters[2];
		break;

	case MD_EX_TYPE_FATALERR_TASK:
		debug_info.name="Fatal error (task)";
		debug_info.fatal_error.err_code1=ex_info->content.fatalerr.error_code.code1;
		debug_info.fatal_error.err_code2=ex_info->content.fatalerr.error_code.code2;
		break;

	case MD_EX_TYPE_FATALERR_BUF:
		debug_info.name="Fatal error (buff)";
		debug_info.fatal_error.err_code1=ex_info->content.fatalerr.error_code.code1;
		debug_info.fatal_error.err_code2=ex_info->content.fatalerr.error_code.code2;
		break;

	case MD_EX_TYPE_LOCKUP:
		debug_info.name="Lockup";
		break;

	case MD_EX_TYPE_ASSERT_DUMP:
		debug_info.name="ASSERT DUMP";
		snprintf(debug_info.assert.file_name,sizeof(debug_info.assert.file_name),
				ex_info->content.assert.filename);	
		debug_info.assert.line_num=ex_info->content.assert.linenumber;
		break;

	case DSP_EX_TYPE_ASSERT:
		debug_info.name="MD DMD ASSERT";
		snprintf(debug_info.dsp_assert.file_name,sizeof(debug_info.dsp_assert.file_name),
				ex_info->content.assert.filename);	
		debug_info.dsp_assert.line_num = ex_info->content.assert.linenumber;
		snprintf(debug_info.dsp_assert.execution_unit,sizeof(debug_info.dsp_assert.execution_unit),
				ex_info->envinfo.execution_unit);	
		debug_info.dsp_assert.parameters[0] = ex_info->content.assert.parameters[0];
		debug_info.dsp_assert.parameters[1] = ex_info->content.assert.parameters[1];
		debug_info.dsp_assert.parameters[2] = ex_info->content.assert.parameters[2];
		break;
	case DSP_EX_TYPE_EXCEPTION:
		debug_info.name="MD DMD Exception";
		snprintf(debug_info.dsp_exception.execution_unit,sizeof(debug_info.dsp_exception.execution_unit),
				ex_info->envinfo.execution_unit);
		debug_info.dsp_exception.code1 = ex_info->content.fatalerr.error_code.code1;	
		break;
	case DSP_EX_FATAL_ERROR:
		debug_info.name="MD DMD FATAL ERROR";
		snprintf(debug_info.dsp_fatal_err.execution_unit,sizeof(debug_info.dsp_fatal_err.execution_unit),
				ex_info->envinfo.execution_unit);	
		debug_info.dsp_fatal_err.err_code[0] = ex_info->content.fatalerr.error_code.code1;
		debug_info.dsp_fatal_err.err_code[1] = ex_info->content.fatalerr.error_code.code2;
		break;
		
	default:
		debug_info.name= "UNKNOW Excep";
	
	}
	if(!trusted){
		strcat("(N)",debug_info.name);
	}
	debug_info.ext_mem=md_ex_log;
	debug_info.ext_size=MD_EX_LOG_SIZE;
	debug_info.md_image=md_img_vir;
	debug_info.md_size=MD_IMG_DUMP_SIZE;
	ccci_dump_debug_info(&debug_info);
}

void md_emi_check(CCCI_BUFF_T *buff)
{
	DEBUG_INFO_T debug_info;
	
        memset(&debug_info,0,sizeof(DEBUG_INFO_T));
        debug_info.type=MD_EX_TYPE_EMI_CHECK;
	debug_info.name="EMI_CHK";
	debug_info.data=*buff;
        ccci_dump_debug_info(&debug_info);
}

static void ex_monitor_func(unsigned long data  __always_unused)
{
	int md_ex_get, md_ex_ok_get; 
	int trusted = 0;
	volatile int reentrant_times;

	atomic_inc(&md_ex_dump_reentrant);
	
	do {
		reentrant_times = atomic_read(&md_ex_dump_reentrant);
	}while(reentrant_times != atomic_read(&md_ex_dump_reentrant));

	if(reentrant_times > 1)
		return;

	while(1){

		md_ex_get=atomic_read(&md_ex);
		md_ex_ok_get=atomic_read(&md_ex_ok);
		if (md_ex_get==1 && md_ex_ok_get==0) {
			atomic_set(&md_ex,0);
			CCCI_MSG_INF("ctl", "Only recv MD_EX, timeout trigger dump. Dump data may be not correct.\n");
		}else if (md_ex_get==1 && md_ex_ok_get==1) {
			atomic_set(&md_ex,0);
			atomic_set(&md_ex_ok,0);
			CCCI_MSG_INF("ctl", "Receive MD_EX_REC_OK\n");
			trusted = 1;
		}else  if (md_ex_get==0 && md_ex_ok_get==1){
			atomic_set(&md_ex_ok,0);
			CCCI_MSG_INF("ctl", "Only Receive MD_EX_REC_OK, this may be appear after time out\n");
		}
		else 
			CCCI_MSG_INF("ctl", "Invalid EX_Num\n");
//		md_boot_stage = MD_BOOT_STAGE_0;
//		atomic_set(&md_ex,0);
//		ccci_disable_nosync();
		ccci_md_exception(trusted);
		atomic_dec(&md_ex_dump_reentrant);
		do {
			reentrant_times = atomic_read(&md_ex_dump_reentrant);
		}while(reentrant_times != atomic_read(&md_ex_dump_reentrant));
		if(reentrant_times == 0)
			break;
	}
}

static void md_boot_up_timeout_func(unsigned long data  __always_unused)
{
	CCCI_BUFF_T sys_msg;
	CCCI_MSG_INF("ctl", "Time out! Notify Deamon\n");
	CCCI_INIT_MAILBOX(&sys_msg, CCCI_MD_MSG_BOOT_TIMEOUT);
	ccci_system_message(&sys_msg);
}

/*
 * ccci_md_ctrl_cb: CCCI_CONTROL_RX callback function for MODEM
 * @buff: pointer to a CCCI buffer
 * @private_data: pointer to private data of CCCI_CONTROL_RX
 */
extern void ccif_send_wakeup_md_msg(void);
static volatile int wakeup_md_is_safe = 0;
void ccci_md_ctrl_cb(CCCI_BUFF_T *buff, void *private_data)
{
	int ret;
	CCCI_BUFF_T sys_msg;

	if (CCCI_MAILBOX_ID(buff) == MD_INIT_START_BOOT &&
		buff->reserved == MD_INIT_CHK_ID && md_boot_stage == MD_BOOT_STAGE_0) 
	{
		del_timer(&md_boot_up_check_timer);
		CCCI_MSG_INF("ctl", "receive MD_INIT_START_BOOT\n");
		md_boot_stage = MD_BOOT_STAGE_1;

		//power on Audsys for DSP boot up
		AudSys_Power_On(TRUE);
		
		CCCI_INIT_MAILBOX(&sys_msg, CCCI_MD_MSG_BOOT_UP);
		ccci_system_message(&sys_msg);
	}
	else if (CCCI_MAILBOX_ID(buff) == NORMAL_BOOT_ID &&
	               md_boot_stage == MD_BOOT_STAGE_1) 
	{
		del_timer(&md_boot_up_check_timer);
		CCCI_MSG_INF("ctl", "receive NORMAL_BOOT_ID\n");
		ccci_after_modem_finish_boot();
		md_boot_stage = MD_BOOT_STAGE_2;
		//if (is_first_boot) 
		//	is_first_boot = 0;
		wakeup_md_is_safe = 1;
		md_call_chain(&md_notifier,CCCI_MD_BOOTUP);
		CCCI_INIT_MAILBOX(&sys_msg, CCCI_MD_MSG_BOOT_READY);
		ccci_system_message(&sys_msg);
		
		//power off Audsys after DSP boot up ready
		AudSys_Power_On(FALSE);

	}
	else if (CCCI_MAILBOX_ID(buff) == MD_EX) 
	{
		del_timer(&md_boot_up_check_timer);
		if (unlikely(buff->reserved != MD_EX_CHK_ID)) 
			CCCI_MSG_INF("ctl", "receive invalid MD_EX\n");
		else 
		{
			md_boot_stage = MD_BOOT_STAGE_EXCEPTION;
			md_ex_flag = 1;
			
			atomic_set(&md_ex, 1);
			mod_timer(&md_ex_monitor,jiffies+5*HZ);
		   	CCCI_MSG_INF("ctl", "receive MD_EX\n");
			ret = ccci_write(CCCI_CONTROL_TX, buff);
			if (ret != 0)
			{ 
				CCCI_MSG_INF("ctl", "fail to write CCCI_CONTROL_TX\n");
			}
			md_call_chain(&md_notifier,CCCI_MD_EXCEPTION);
			CCCI_INIT_MAILBOX(&sys_msg, CCCI_MD_MSG_EXCEPTION);
			ccci_system_message(&sys_msg);
		}
	}
	else if (CCCI_MAILBOX_ID(buff) == MD_EX_REC_OK) 
	{
		if (unlikely(buff->reserved != MD_EX_REC_OK_CHK_ID)) 
			CCCI_MSG_INF("ctl", "receive invalid MD_EX_REC_OK\n");
		else 
		{
			atomic_set(&md_ex_ok, 1);
			mod_timer(&md_ex_monitor,jiffies);		
		}
	} 
	else if (CCCI_MAILBOX_ID(buff) == MD_INIT_START_BOOT &&
		buff->reserved == MD_INIT_CHK_ID && !is_first_boot) 
	{
		/* reset state and notify the user process md_init */
		md_boot_stage = MD_BOOT_STAGE_0;
		CCCI_MSG_INF("ctl", "MD second bootup detected!\n");
		CCCI_INIT_MAILBOX(&sys_msg, CCCI_MD_MSG_RESET);
		ccci_system_message(&sys_msg);
	}
	else if (CCCI_MAILBOX_ID(buff) == MD_EX_RESUME_CHK_ID) 
	{
		md_emi_check(buff);
	}
	else if (CCCI_MAILBOX_ID(buff) == CCCI_DRV_VER_ERROR)
	{
		CCCI_MSG_INF("ctl", "AP CCCI driver version mis-match to MD!!\n");
	}
	else 
	{
		CCCI_MSG_INF("ctl", "receive unknow data from CCCI_CONTROL_RX = %d\n", CCCI_MAILBOX_ID(buff));
	}
}


/*
 * set_md_runtime: setup MODEM runtime data
 * return 0 for success; return negative value for failure
 */
static int set_md_runtime(void)
{
    int i, addr, len;
    struct modem_runtime_t *runtime = (struct modem_runtime_t *)MD_RUNTIME_ADDR;
    volatile int *base = (int*)(MD_RUNTIME_ADDR+sizeof(struct modem_runtime_t));
	struct file *filp = NULL;
	LOGGING_MODE mdlog_flag = MODE_IDLE;
	int ret = 0;

    do {
	base--;
	CCCI_WRITEL(base,0);
    } while (base > (int*)MD_RUNTIME_ADDR);

    CCCI_WRITEL(&runtime->Prefix, 0x46494343); // "CCIF"
    CCCI_WRITEL(&runtime->Postfix, 0x46494343); // "CCIF"


    CCCI_WRITEL(&runtime->BootChannel, CCCI_CONTROL_RX);
#if 0
    if (is_meta_mode()||is_advanced_meta_mode()) {
        CCCI_WRITEL(&runtime->BootingStartID ,META_BOOT_ID);
    } else {
        CCCI_WRITEL(&runtime->BootingStartID ,NORMAL_BOOT_ID);
    }
#endif 

#ifdef AP_MD_EINT_SHARE_DATA
    CCCI_WRITEL(&runtime->DriverVersion, 0x00000929);
    CCCI_WRITEL(&runtime->SysShareMemBase ,ccci_sys_smem_base_phy);
    CCCI_WRITEL(&runtime->SysShareMemSize ,ccci_sys_smem_size);
#endif 
    platform_set_runtime_data(runtime);

    CCCI_WRITEL(&runtime->ExceShareMemBase ,ccci_exp_smem_base_phy);
    CCCI_WRITEL(&runtime->ExceShareMemSize ,ccci_exp_smem_size);


    ccci_mdlog_base_req((void *)&addr, &len);
    CCCI_WRITEL(&runtime->MdlogShareMemBase ,addr);
    CCCI_WRITEL(&runtime->MdlogShareMemSize , len);

    ccci_pcm_base_req((void *)&addr, &len);
    CCCI_WRITEL(&runtime->PcmShareMemBase ,addr);
    CCCI_WRITEL(&runtime->PcmShareMemSize ,len);

    ccci_pmic_base_req((void *)&addr, &len);
    CCCI_WRITEL(&runtime->PmicShareMemBase ,addr);
    CCCI_WRITEL(&runtime->PmicShareMemSize ,len);

    ccci_fs_base_req((void *)&addr, &len);
    CCCI_WRITEL(&runtime->FileShareMemBase ,addr);
    CCCI_WRITEL(&runtime->FileShareMemSize ,len);
   
    ccci_rpc_base_req((void *)&addr, &len);
    CCCI_WRITEL(&runtime->RpcShareMemBase, addr);
    CCCI_WRITEL(&runtime->RpcShareMemSize,len);

    ccci_ipc_base_req((void*)&addr,&len);
    CCCI_WRITEL(&runtime->IPCShareMemBase,addr);
    CCCI_WRITEL(&runtime->IPCShareMemSize,len);

    for (i = 0; i < CCCI_TTY_BUFF_NR; i++) {
        ccci_uart_base_req(i, (void *)&addr, &len);
        if (addr == 0 || len == 0) {
            break;
        } else {
            CCCI_WRITEL(&runtime->UartShareMemBase[i] ,addr);
            CCCI_WRITEL(&runtime->UartShareMemSize[i] ,len);
        }
    }
    CCCI_WRITEL(&runtime->UartPortNum ,i);

	  //add a new attribute of mdlogger auto start info to notify md
	  filp = filp_open(MDLOGGER_FILE_PATH, O_RDONLY, 0777);
	  if (IS_ERR(filp)) {
    	CCCI_MSG_INF("ctl", "open /data/mdl/mdl_config fail:%ld\n", PTR_ERR(filp));	
		   filp=NULL;
    }
	  else {
		   ret = kernel_read(filp, 0, (char*)&mdlog_flag,sizeof(int));	
       if (ret != sizeof(int)) {
			CCCI_MSG_INF("ctl", "read /data/mdl/mdl_config fail:ret=%d!\n", ret);
			    mdlog_flag = MODE_IDLE;
       }
	  }

	  if(filp != NULL) {
		//CCCI_MSG_INF("ctl", "close /data/mdl/mdl_config!\n");
	     //filp_close(filp, current->files);
	     filp_close(filp, NULL);
	  }
	
	if (is_meta_mode() || is_advanced_meta_mode()) 
       CCCI_WRITEL(&runtime->BootingStartID, ((char)mdlog_flag <<8 | META_BOOT_ID));
	else 
       CCCI_WRITEL(&runtime->BootingStartID, ((char)mdlog_flag <<8 | NORMAL_BOOT_ID));

	  CCCI_MSG_INF("ctl", "send /data/mdl/mdl_config =%d to modem!\n", mdlog_flag);

    DUMP_CCCI_MD_DATA();

    return 0;
}


/*
 * boot_md: boot-up MODEM
 * return 0 for success; return negative values for failure
 */
static int boot_md(void)
{
    int ret=0;
    //struct image_info *pImg_info=NULL;
	
    if (md_boot_stage != MD_BOOT_STAGE_0)
    {
	CCCI_MSG_INF("ctl", "MD has boot up!\n");
	return 0;
    }
   
    CCCI_MSG("booting up MODEM: start to load firmware...\n");

    if(is_first_boot) {
        //if((ret = ccci_load_firmware(pImg_info)) <0) {
        if((ret = ccci_load_firmware(LOAD_MD_DSP)) <0) {
	    CCCI_MSG_INF("ctl", "load firmware fail, so modem boot fail!\n");
	    return ret;
        }
        else {
	    //when load firmware successfully, no need to load it again when reset modem
	    is_first_boot = 0;
	    CCCI_MSG_INF("ctl", "load firmware successful!\n");
        }
    }
    else {
	CCCI_MSG_INF("ctl", "modem&dsp firmware already exist, not load again!\n");
    }

    ret = ccci_register(CCCI_CONTROL_TX, NULL, NULL);
    if (ret != 0 && ret != CCCI_IN_USE) {
        CCCI_MSG_INF("ctl", "fail to register CCCI_CONTROL_TX\n");
        return ret;
    }
	
    ret = ccci_register(CCCI_CONTROL_RX, ccci_md_ctrl_cb, NULL);
    if (ret != 0 && ret != CCCI_IN_USE) {
        CCCI_MSG_INF("ctl", "fail to register CCCI_CONTROL_RX\n");
        ccci_unregister(CCCI_CONTROL_TX);
        return ret;
    }

    ret = ccci_register(CCCI_SYSTEM_TX, NULL, NULL);
    if (ret != 0 && ret != CCCI_IN_USE) {
        CCCI_MSG_INF("ctl", "fail to register CCCI_SYSTEM_TX\n");
    }

    /* step 1: ungate modem */
    ungate_md();

    /* step 2: get start-boot command from control channel */
    CCCI_MSG_INF("ctl", "wait MD_INIT_START_BOOT\n");

    #if 0
    time_out = jiffies + 6*HZ;
    do {
        if (md_boot_stage == MD_BOOT_STAGE_1) {
            CCCI_MSG_INF("ctl", "received MD_INIT_START_BOOT\n");
            break;
        }
	yield();
	if(time_after(jiffies, time_out)){
            CCCI_MSG_INF("ctl", "wait MD_INIT_START_BOOT time out, try to reboot MD again\n");
            CCCI_INIT_MAILBOX(&sys_msg, CCCI_MD_MSG_RESET_RETRY);
            ccci_system_message(&sys_msg);
            return ret;
        }
    } while (1);

    /* step 3: set runtime data and echo start-boot command */
    CCCI_MSG_INF("ctl", "set modem runtime\n");
    ret = set_md_runtime();
    if (ret != 0) {
        CCCI_MSG("fail to set MODEM runtime data\n");
        return ret;
    }
	
    //printk("echo MD_INIT_START_BOOT\n");
    CCCI_BUFF_T buff;
    CCCI_INIT_MAILBOX(&buff, MD_INIT_START_BOOT);
    buff.reserved = MD_INIT_CHK_ID;
    ccci_before_modem_start_boot();
    ret = ccci_write(CCCI_CONTROL_TX, &buff);
    if (ret != 0) {
        CCCI_MSG("fail to write CCCI_CONTROL_TX\n");
        return ret;
    }
    CCCI_MSG_INF("ctl", "wait for NORMAL_BOOT_ID\n");
    //if (end)   end();
    //Notes:after load dsp_rom, dsp will write data back to dsp region, so set protect region at last
    start_emi_mpu_protect();
    #endif

    return ret;
}


/*
 * ccci_reset_register: register a user for ccci reset
 * @name: user name
 * return a handle if success; return negative value if failure
 */
int ccci_reset_register(char *name)
{
    int handle, i;

    CCCI_MSG_INF("ctl", "Register a reset handle\n");
	
    if (name == NULL) {
		CCCI_MSG_INF("ctl", "Invalid reset handle name registered \n");
        return CCCI_INVALID_PARAM;
    }

    if (down_interruptible(&ccci_reset_mutex)) {
		CCCI_MSG_INF("ctl", "down_interruptible fail \n");
        return -1;
    }
    		
    for (handle = 0; handle < NR_CCCI_RESET_USER; handle++) {
        if (reset_sta[handle].is_allocate == 0) {
            reset_sta[handle].is_allocate = 1;
            break;
        }
    }

    if (handle < NR_CCCI_RESET_USER) {
		reset_sta[handle].is_reset = 0;
	up(&ccci_reset_mutex);
        for (i = 0; i < NR_CCCI_RESET_USER_NAME; i++) {
            if (name[i] == '\0') {
                break;
            } else {
                reset_sta[handle].name[i] = name[i];
            }
        }
		CCCI_MSG_INF("ctl", "Register a reset handle by %s(%d)\n", current->comm, handle);
        return handle;
    } 
	else {
	up(&ccci_reset_mutex);
        ASSERT(0);
        return CCCI_FAIL;
    }
}

/*
 * reset_md: reset modem
 * return 0 if success; return negative value if failure
 */
int reset_md(void)
{
    CCCI_BUFF_T sys_msg;
	char buf = 1;
	
    CCCI_MSG_INF("ctl", "send reset modem request message\n");

    /* prevent another reset modem action from wdt timeout IRQ during modem reset */
	atomic_inc(&md_reset_on_going);
	if(atomic_read(&md_reset_on_going)>1){
		CCCI_MSG_INF("ctl", "One reset flow is on-going \n");
		return CCCI_MD_IN_RESET;
	}

	if(md_boot_stage == MD_BOOT_STAGE_2){
		lock_md_sleep(&buf, sizeof(char));
	}
	
	/* v1.3 20120601: mask CCIF IRQ to prevent md_boot_stage from updating by CCIF ISR */
    md_boot_stage = MD_BOOT_STAGE_0;
    //ccci_disable();
    ccci_mask();
    //CCCI_INIT_MAILBOX(&sys_msg, CCCI_SYS_MSG_RESET_MD);
    wake_lock_timeout(&trm_wake_lock, 10*HZ);
    CCCI_INIT_MAILBOX(&sys_msg, CCCI_MD_MSG_RESET);
    ccci_system_message(&sys_msg);

  //  CCCI_DEBUG("wait.....\n");
 //   schedule_timeout_interruptible(5*HZ);
 //  boot_md();
    return CCCI_SUCCESS;
}

/*
 * send_stop_md_request:
 * return 0 if success; return negative value if failure
 */
int send_stop_md_request(void)
{
	CCCI_BUFF_T sys_msg;
    char buf = 1;

	CCCI_MSG_INF("ctl", "send stop modem request message\n");
	/* prevent another reset modem action from wdt timeout IRQ during modem reset */
	atomic_inc(&md_reset_on_going);
	if(atomic_read(&md_reset_on_going)>1){
		CCCI_MSG_INF("ctl", "One stop flow is on-going \n");
		return CCCI_MD_IN_RESET;
	}

	if(md_boot_stage == MD_BOOT_STAGE_2){
		lock_md_sleep(&buf, sizeof(char));
	}
	
	/* mask CCIF IRQ to prevent md_boot_stage from updating by CCIF LISR */
	md_boot_stage = MD_BOOT_STAGE_0;
	/* v1.4 20120618: mask CCIF IRQ to prevent md_boot_stage from updating by CCIF ISR */
	//CCCI_MSG_INF("ctl", "stop request: mask ccci irq\n");
	ccci_mask();
	CCCI_INIT_MAILBOX(&sys_msg, CCCI_MD_MSG_STOP_MD_REQUEST);
	ccci_system_message(&sys_msg);

	return CCCI_SUCCESS;
}

/*
 * send_start_md_request:
 * return 0 if success; return negative value if failure
 */
int send_start_md_request(void)
{
	CCCI_BUFF_T sys_msg;

	CCCI_MSG_INF("ctl", "send start modem request message\n");
	CCCI_INIT_MAILBOX(&sys_msg, CCCI_MD_MSG_START_MD_REQUEST);
	ccci_system_message(&sys_msg);
	return CCCI_SUCCESS;
}

/*
 * ccci_reset_request: request to reset CCCI
 * @handle: a user handle gotten from ccci_reset_register()
 * return 0 if CCCI is reset; return negative value for failure
 */
int ccci_reset_request(int handle)
{
    int i;
    CCCI_BUFF_T sys_msg;
	bool reset_ready = true;
    
    if (handle >= NR_CCCI_RESET_USER) {
		CCCI_MSG_INF("ctl", "reset_request: invalid handle:%d \n", handle);
        return CCCI_INVALID_PARAM;
    }
	
    if (reset_sta[handle].is_allocate == 0) {
		CCCI_MSG_INF("ctl", "reset_request: handle(%d) not alloc: alloc=%d \n", 
			handle, reset_sta[handle].is_allocate);
        return CCCI_INVALID_PARAM;
    } 

	CCCI_MSG_INF("ctl", "%s (%d) call reset request \n",current->comm,handle);

 
    down(&ccci_reset_mutex);	
	
    reset_sta[handle].is_allocate = 0;
    reset_sta[handle].is_reset = 1;

    CCCI_MSG_INF("ctl", "Dump not ready list++++\n");
    for (i = 0; i < NR_CCCI_RESET_USER; i++) {
        if (reset_sta[i].is_allocate && (reset_sta[i].is_reset == 0)) {
			reset_ready = false;
            CCCI_MSG_INF("ctl", " ==> %s\n", reset_sta[i].name);
        }
    }
    CCCI_MSG_INF("ctl", "Dump not ready list----\n");

    up(&ccci_reset_mutex);
   
    if (reset_ready == false) 
            return CCCI_RESET_NOT_READY;
 
    #if 0
    atomic_inc(&md_reset_on_going);
    if(atomic_read(&md_reset_on_going)>1){
        CCCI_MSG_INF("ctl", "One reset flow is on-going.\n");
        return;
    }

    /* if all register users are ready to reset, reset modem */
    CCCI_MSG_INF("ctl", "Reset by %s(%d).\n",current->comm,handle);
    md_call_chain(&md_notifier,CCCI_MD_RESET);
    ccci_disable();
    gate_md();
    ccci_reset();
    ccci_enable();
    /* reset state and notify the user process md_init */
   // md_boot_stage = MD_BOOT_STAGE_0;
    boot_md();
    for (i = 0; i < NR_CCCI_RESET_USER; i++) {
        reset_sta[i].is_reset = 0;
    }
    atomic_set(&md_reset_on_going, 0);
    #endif
	
    CCCI_MSG_INF("ctl", "Reset MD by %s(%d) \n", current->comm, handle);
    CCCI_INIT_MAILBOX(&sys_msg, CCCI_MD_MSG_RESET_REQUEST);
    ccci_system_message(&sys_msg);

    return CCCI_SUCCESS;
}

/*
 * ccci_stop_modem:
 * Do stop modem operation
 */
int ccci_stop_modem(void)
{
	int i, ret;

	/*ALPS00302837: md reset happens again during reset process, this judge will cause the first reset fail*/
	#if 0
	//atomic_inc(&md_reset_on_going);
	if(atomic_read(&md_reset_on_going)>1){
		CCCI_MSG_INF("ctl", "One reset flow is on-going \n");
		return -1;
	}
	#endif

	//md_call_chain(&md_notifier,CCCI_MD_RESET);
	ccci_disable();
	md_call_chain(&md_notifier,CCCI_MD_RESET);
	gate_md();
	ccci_reset();

	for (i = 0; i < NR_CCCI_RESET_USER; i++) {
		reset_sta[i].is_reset = 0;
	}

	if(md_ex_flag) {
		if((ret = ccci_load_firmware(LOAD_MD_ONLY)) <0) {
	    	CCCI_MSG_INF("ctl", "load firmware fail, so modem boot fail:%d!\n", ret);
	    	return -1;
        }
        else {
	   	 	//when load firmware successfully, no need to load it again when reset modem
	   	 	CCCI_MSG_INF("ctl", "load firmware successful!\n");
        }
		md_ex_flag = 0;
	}
	
	md_slp_cnt = 0;
	md_boot_stage = MD_BOOT_STAGE_0;

	return CCCI_SUCCESS;
}

/*
 * ccci_stop_modem:
 * Do start modem operation
 */
int ccci_start_modem(void)
{
	ccci_enable();
	ungate_md();
	mod_timer(&md_boot_up_check_timer, jiffies+5*HZ);

	atomic_set(&md_reset_on_going, 0);
	return CCCI_SUCCESS;
}

/*
 * ccci_do_modem_reset:
 * Do reset work by operate the rgu register
 */
int ccci_do_modem_reset(void)
{
	CCCI_MSG_INF("ctl", "Begin to reset MD\n");

	if(ccci_stop_modem() != CCCI_SUCCESS)
		return -1;

	ccci_start_modem();

	return CCCI_SUCCESS;
}

/*
 * ccci_send_run_time_data
 * return 0 for success; return negative values for failure
 */
int ccci_send_run_time_data(void)
{
	int ret=0;
	CCCI_BUFF_T buff;

	/* Set runtime data and echo start-boot command */
	CCCI_MSG_INF("ctl", "set modem runtime\n");
	ret = set_md_runtime();
	if (ret != 0) {
		CCCI_MSG_INF("ctl", "fail to set MODEM runtime data\n");
		return ret;
	}

	//printk("echo MD_INIT_START_BOOT\n");
	CCCI_INIT_MAILBOX(&buff, MD_INIT_START_BOOT);
	buff.reserved = MD_INIT_CHK_ID;
	ccci_before_modem_start_boot();
	ret = ccci_write(CCCI_CONTROL_TX, &buff);
	if (ret != 0) {
		CCCI_MSG_INF("ctl", "fail to write CCCI_CONTROL_TX\n");
		return ret;
	}
	CCCI_MSG_INF("ctl", "wait for NORMAL_BOOT_ID\n");
	//if (end)   end();
	//Notes:after load dsp_rom, dsp will write data back to dsp region, so set protect region at last
	//start_emi_mpu_protect();
	enable_emi_mpu_protection(ccci_smem_phy, ccci_smem_size);
	//mod_timer(&md_boot_up_check_timer, jiffies+5*HZ);
	mod_timer(&md_boot_up_check_timer, jiffies+10*HZ);
	return ret;
}

/*
 * boot_md_show
 * @buf:
 */
static ssize_t boot_md_show(char *buf)
{
    return sprintf(buf, "%d\n", md_boot_stage);
}

/*
 * boot_md_store
 * @buf:
 * @count:
 */
static ssize_t boot_md_store(const char *buf, size_t count)
{
    if (down_interruptible(&ccci_mb_mutex)) {
        return count;
    }

    if (md_boot_stage == MD_BOOT_STAGE_0) {
        if (!ready2boot()) {
            CCCI_MSG_INF("ctl", "systme is not ready\n");
        } else {
            boot_md();
        }
    } else {
        CCCI_MSG_INF("ctl", "MD already in boot stage %d\n", md_boot_stage);
    }

    up(&ccci_mb_mutex);

    return count;
}

static int __init ccci_alloc_smem(void)
{
	int ret = 0;
	int i, base_virt, base_phy;

	ccci_smem_size = CCCI_PCM_SMEM_SIZE +
		    CCCI_MD_LOG_SIZE +
		    CCCI_PMIC_SMEM_SIZE +
		    CCCI_RPC_SMEM_SIZE +
		    CCCI_FS_SMEM_SIZE +
		    (CCCI_TTY_SMEM_SIZE * CCCI_TTY_PORT_COUNT) +
#ifdef AP_MD_EINT_SHARE_DATA
		    CCCI_SYS_SMEM_SIZE +
#endif
		    MD_EX_LOG_SIZE+
		    CCCI_IPC_SMEM_SIZE;
	ccci_smem_size=round_up(ccci_smem_size,0x4000);
	
#ifdef CCCI_STATIC_SHARED_MEM

	if (CCCI_SHARED_MEM_SIZE < ccci_smem_size) {
	    CCCI_MSG_INF("ctl", "CCCI shared mem is too small\n");
	    return -ENOMEM;
	}

	ccci_smem_virt = ioremap_nocache(CCCI_SHARED_MEM_BASE, CCCI_SHARED_MEM_SIZE);
	if (!ccci_smem_virt) {
		CCCI_MSG_INF("ctl", "CCCI_MD: alloc_smem err\n");
		return -ENOMEM;
	}
	ccci_smem_phy = CCCI_SHARED_MEM_BASE;
#else // dynamic allocation shared memory

	ccci_smem_virt = dma_alloc_coherent(NULL, ccci_smem_size, &ccci_smem_phy, GFP_KERNEL);
	if (ccci_smem_virt == NULL) {
	    CCCI_MSG_INF("ctl", "ccci_alloc_smem\n");
	    return -ENOMEM;
	}
	CCCI_CTL_MSG("ccci_smem_size=%d,ccci_smem_virt=%x,ccci_smem_phy=%x\n", 
		ccci_smem_size, (unsigned int)ccci_smem_virt, (unsigned int)ccci_smem_phy);
#endif

	WARN_ON(ccci_smem_phy&(0x4000-1)||ccci_smem_size&(0x4000-1));
	base_virt = (int)ccci_smem_virt;
	base_phy = (int)ccci_smem_phy;

	//LOG
	ccci_mdlog_smem_base_virt = base_virt;
	ccci_mdlog_smem_base_phy = base_phy;
	ccci_mdlog_smem_size = CCCI_MD_LOG_SIZE;
	base_virt += CCCI_MD_LOG_SIZE;
	base_phy += CCCI_MD_LOG_SIZE;

	// PCM
	ccci_pcm_smem_base_virt = base_virt;
	ccci_pcm_smem_base_phy = base_phy;
	ccci_pcm_smem_size = CCCI_PCM_SMEM_SIZE;
	base_virt += CCCI_PCM_SMEM_SIZE;
	base_phy += CCCI_PCM_SMEM_SIZE;
	// FS
	ccci_fs_smem_base_virt = base_virt;
	ccci_fs_smem_base_phy = base_phy;
	ccci_fs_smem_size = CCCI_FS_SMEM_SIZE;
	base_virt += CCCI_FS_SMEM_SIZE;
	base_phy += CCCI_FS_SMEM_SIZE;

	// RPC
	ccci_rpc_smem_base_virt = base_virt;
	ccci_rpc_smem_base_phy = base_phy;
	ccci_rpc_smem_size = CCCI_RPC_SMEM_SIZE;
	base_virt += CCCI_RPC_SMEM_SIZE;
	base_phy += CCCI_RPC_SMEM_SIZE;	

	// TTY
	for (i = 0; i < CCCI_TTY_PORT_COUNT ; i++) {
		ccci_tty_smem_base_virt[i] = base_virt;
		ccci_tty_smem_base_phy[i] = base_phy;
		ccci_tty_smem_size[i] = CCCI_TTY_SMEM_SIZE;
		base_virt += CCCI_TTY_SMEM_SIZE;
		base_phy += CCCI_TTY_SMEM_SIZE;
	}
	for (; i < CCCI_TTY_BUFF_NR; i++) {
		ccci_tty_smem_base_virt[i] = 0;
		ccci_tty_smem_base_phy[i] = 0;
		ccci_tty_smem_size[i] = 0;
	}

	// PMIC
	ccci_pmic_smem_base_virt = base_virt;
	ccci_pmic_smem_base_phy = base_phy;
	ccci_pmic_smem_size = CCCI_PMIC_SMEM_SIZE;
	base_virt += CCCI_PMIC_SMEM_SIZE;
	base_phy += CCCI_PMIC_SMEM_SIZE;

	// SYS
#ifdef AP_MD_EINT_SHARE_DATA
	ccci_sys_smem_base_virt = base_virt;
	ccci_sys_smem_base_phy = base_phy;
	ccci_sys_smem_size = CCCI_SYS_SMEM_SIZE;
	cores_exch_data = (struct ccci_cores_exch_data*)ccci_sys_smem_base_virt;
	base_virt += CCCI_SYS_SMEM_SIZE;
	base_phy += CCCI_SYS_SMEM_SIZE;
#endif
	// EXP
	ccci_exp_smem_base_virt = base_virt;
	ccci_exp_smem_base_phy = base_phy;
	ccci_exp_smem_size = MD_EX_LOG_SIZE;
	md_ex_log = (int*)ccci_exp_smem_base_virt ;
	base_virt += MD_EX_LOG_SIZE;
	base_phy += MD_EX_LOG_SIZE;
	
	//IPC
	ccci_ipc_smem_base_virt=base_virt;
	ccci_ipc_smem_base_phy=base_phy;
	ccci_ipc_smem_size=CCCI_IPC_SMEM_SIZE;
	base_virt += CCCI_IPC_SMEM_SIZE ;
	base_phy +=  CCCI_IPC_SMEM_SIZE;

	return ret;
}

static void ccci_free_smem(void)
{
#ifdef CCCI_STATIC_SHARED_MEM
	if (ccci_smem_virt)
		iounmap(ccci_smem_virt);
#else
	if (ccci_smem_virt)
		dma_free_coherent(NULL, ccci_smem_size, ccci_smem_virt, ccci_smem_phy);
#endif
}




void lock_md_dormant(void)
{
	ccci_write_mailbox_with_resv(CCCI_SYSTEM_TX, MD_DORMANT_NOTIFY, 0x1);
	CCCI_MSG_INF("ctl", "notify modem lock dormant mode\n");
}

void unlock_md_dormant(void)
{
	ccci_write_mailbox_with_resv(CCCI_SYSTEM_TX, MD_DORMANT_NOTIFY, 0x0);
	CCCI_MSG_INF("ctl", "notify modem unlock dormant mode\n");
}

	
/*
ccci_dormancy
When "data_connect_sta" is not zero, which means there are data connection exist
So the suspend function should call this API to notify modem disconnect data 
connection before enter suspend state to save power. axs
 */
 
int ccci_dormancy(char *buf, unsigned int len)
{
	//CCCI_MSG("<ctl> ccci_dormancy\n");
	//if(atomic_read(&data_connect_sta) > 0)
	{
		//CCCI_MSG_INF("ctl", "notify enter dormancy, data_sta<%d>\n", atomic_read(&data_connect_sta));
		CCCI_MSG_INF("ctl", "notify enter fast dormancy \n");
		ccci_write_mailbox(CCCI_SYSTEM_TX, 0xE);
		//atomic_set(&data_connect_sta,0x0);
	}

	return 0;
	//unlock_md_dormant();
}
/*
When data channel coming, the "data_connect_sta" should be mark
This function called in IRQ_handler function. axs
*/
void check_data_connected(int channel)
{
	if(channel >= CCCI_CCMNI1_RX && channel <= CCCI_CCMNI3_TX_ACK)
		atomic_inc((atomic_t*)&data_connect_sta);
}


int lock_md_sleep(char *buf, unsigned int len)
{
	unsigned long flag;
	int ret = 0;

	spin_lock_irqsave(&md_slp_lock, flag);
	if (buf[0]) {
		if (++md_slp_cnt == 1)
			md_slp_lock_ack = 0;
	}
	else {
		if (md_slp_cnt == 0) {
			CCCI_MSG_INF("ctl", "unlock md slp mis-match lock(%s, 0)\n", current->comm);
			spin_unlock_irqrestore(&md_slp_lock, flag);
			return ret;
		}
		
		if (--md_slp_cnt == 0)
			md_slp_unlock_ack = 0;
	}
	spin_unlock_irqrestore(&md_slp_lock, flag);
	
	if (md_slp_cnt == 1)
		ret = ccci_write_mailbox_with_resv(CCCI_SYSTEM_TX, MD_SLP_REQUEST, LOCK_MD_SLP);
	else if (md_slp_cnt == 0)
		ret = ccci_write_mailbox_with_resv(CCCI_SYSTEM_TX, MD_SLP_REQUEST, UNLOCK_MD_SLP);	
	
	CCCI_MSG_INF("ctl", "%s request md sleep %d (%d, %d, %d): %d\n", 
		current->comm, buf[0], md_slp_cnt, md_slp_lock_ack, md_slp_unlock_ack, ret);

	return ret;
}

int ack_md_sleep(char *buf, unsigned int len)
{
	unsigned int flag = 0;
	
	if (buf[0])
		flag = md_slp_lock_ack;
	else
		flag = md_slp_unlock_ack;

	//CCCI_MSG_INF("cci", "ack md sleep %d (%d %d) \n", buf[0], md_slp_cnt, flag);

	return flag;
}


#ifdef CONFIG_MTK_AEE_FEATURE
extern void aed_md_exception(int *, int, int *, int, char *);

#endif

void ccci_aed(unsigned int dump_flag, char *aed_str)
{
	#define AED_STR_LEN		(512)
	int *ex_log_addr = NULL;
	int ex_log_len = 0;
	int *md_img_addr = NULL;
	int md_img_len = 0;
	int info_str_len = 0;
	char buff[AED_STR_LEN];

	info_str_len = strlen(aed_str);
	info_str_len += strlen(image_buf);

	if(info_str_len > AED_STR_LEN){
		CCCI_MSG_INF("ctl", "Info string is too long, num:%d\n", info_str_len);
		buff[AED_STR_LEN-1] = '\0'; // Cut string length to AED_STR_LEN
	}

	snprintf(buff, AED_STR_LEN, "%s%s", aed_str, image_buf);

	if(dump_flag & CCCI_AED_DUMP_EX_MEM){
		ex_log_addr = md_ex_log;
		ex_log_len = MD_EX_LOG_SIZE;
	}
	if(dump_flag & CCCI_AED_DUMP_MD_IMG_MEM){
		md_img_addr = md_img_vir;
		md_img_len = MD_IMG_DUMP_SIZE;
	}

	#ifdef CONFIG_MTK_AEE_FEATURE
	aed_md_exception(ex_log_addr, ex_log_len, md_img_addr, md_img_len, buff);
	#endif
}

extern void ccci_aed_cb_register(ccci_aed_cb_t funcp);
typedef size_t (*ccci_sys_cb_func_t)(char buf[], size_t len);
extern int register_filter_func(char cmd[], ccci_sys_cb_func_t store, ccci_sys_cb_func_t show);
extern unsigned long long lg_ch_tx_debug_enable;
extern unsigned long long lg_ch_rx_debug_enable;
extern unsigned int tty_debug_enable;
extern unsigned int fs_tx_debug_enable; 
extern unsigned int fs_rx_debug_enable; 



size_t ccci_msg_filter_store(char buf[], size_t len)
{
	unsigned int msg_mask=0;
	unsigned int key;
	int ret;

	ret = sscanf(buf, "-l=0x%x 0x%x", &key, &msg_mask);
	if(ret != 2){
		CCCI_MSG("Parse msg filter fail\n");
	}else if( key != 0x20111111){
		CCCI_MSG("Wrong key\n");
	}else{
		ccci_msg_mask = msg_mask;
	}
	return len;
}

size_t ccci_msg_filter_show(char buf[], size_t len)
{
	int ret;
	ret = snprintf(buf, len, "msg mask: %x\n", ccci_msg_mask);
	return ret;
}

size_t ccci_ch_filter_store(char buf[], size_t len)
{
	unsigned long long lg_ch_tx_mask= 0;
	unsigned long long lg_ch_rx_mask= 0;
	unsigned int tty_mask= 0;
	unsigned int fs_tx_mask= 0;
	unsigned int fs_rx_mask = 0;
	unsigned int key = 0;
	int ret = 0;

	//ret = sscanf(buf, "-c=0x%x 0x%llX 0x%llX 0x%x 0x%x 0x%x", &key, &lg_ch_tx_mask, 
	//	&lg_ch_rx_mask, &fs_tx_mask, &fs_rx_mask, &tty_mask);
	ret = sscanf(buf, "-c=0x%x 0x%llx 0x%llx 0x%x 0x%x 0x%x ", &key, &lg_ch_tx_mask, &lg_ch_rx_mask, \
		&fs_tx_mask, &fs_rx_mask, &tty_mask);
	if(ret != 6){
		CCCI_MSG("Parse channel filter fail\n");
	}else if( key != 0x20111111){
		CCCI_MSG("Wrong key\n");
	}else{
		CCCI_MSG("%x %x %x %x %llx %llx\n", key, fs_tx_mask, fs_rx_mask, tty_mask, \
			lg_ch_tx_mask, lg_ch_rx_mask);
		lg_ch_tx_debug_enable = lg_ch_tx_mask;
		lg_ch_rx_debug_enable = lg_ch_rx_mask;
		fs_tx_debug_enable = fs_tx_mask;
		fs_rx_debug_enable = fs_rx_mask;
		tty_debug_enable = tty_mask;
	}
	return len;
}

size_t ccci_ch_filter_show(char buf[], size_t len)
{
	int ret;

	ret = snprintf(buf, len, "tx_mask: %llX\nrx_mask: %llX\nfs_tx_mask: %X\nfs_rx_mask: %X\ntty_msk: %X\n", 
			lg_ch_tx_debug_enable, lg_ch_rx_debug_enable, 
			fs_tx_debug_enable, fs_rx_debug_enable, tty_debug_enable);
	return ret;
}

/*
 * ccci_md_init_mod_init: module init function
 */
int __init ccci_md_init_mod_init(void)
{
    int ret;

	//<2013/05/21-25146-EricLin, Remove time stamp logs in kernel.
    	// CCCI_MSG("Ver. %s, @ %s %s\n",ccci_version, __DATE__, __TIME__); 
	//>2013/05/21-25146-EricLin
   
    ret = ccci_alloc_smem();
    if (ret)
	return ret;
	
    /*
     * IOREMAP the physical address where the modem firmware is stored.
     * We will need to dump this, when there's an exception.
     */
    //md_img_vir = (int *)ioremap_nocache(MODEM_REGION_BASE, MD_IMG_DUMP_SIZE);
//	 printk(KERN_ERR "----------->CCCI_MD: md_img_vir=%p.\n",md_img_vir);
    //if (!md_img_vir)
    //{
        //printk(KERN_ERR "[CCCI]:CCCI_MD[Exception] Mem dump ioremap failed\n");
        //return -ENOMEM;
    //}

    ret=platform_init();
    if (ret)
    {
	CCCI_MSG_INF("ctl", "platform_init failed.\n");
	//iounmap(md_img_vir);
	return ret;
    }
	
    ret=ccci_rpc_init();
    if (ret)
    {
	CCCI_MSG_INF("ctl", "rpc_init failed.\n");
	ccci_rpc_exit();
	//iounmap(md_img_vir);
	return ret;
    }
	
    // MUST register callbacks after memory is allocated
	boot_register_md_func(boot_md_show, boot_md_store);
	//slp_set_ccci_callback(ccci_dormancy);
	ccci_md_wdt_notify_register(reset_md);
	ccci_aed_cb_register(ccci_aed);
	register_resume_notify(RSM_ID_MD_LOCK_DORMANT, lock_md_dormant);
	register_suspend_notify(SLP_ID_MD_UNLOCK_DORMANT, unlock_md_dormant);
	register_ccci_kern_func(ID_CCCI_DORMANCY, ccci_dormancy);
    register_ccci_kern_func(ID_LOCK_MD_SLEEP, lock_md_sleep);
	register_ccci_kern_func(ID_ACK_MD_SLEEP, ack_md_sleep);
    
    register_filter_func("-l", ccci_msg_filter_store, ccci_msg_filter_show);
    register_filter_func("-c", ccci_ch_filter_store, ccci_ch_filter_show);
    wake_lock_init(&trm_wake_lock, WAKE_LOCK_SUSPEND, "ccci_trm");
    spin_lock_init(&md_slp_lock);
    
    return 0;
}


/*
 * ccci_md_init_mod_exit: module exit function
 */
void __exit ccci_md_init_mod_exit(void)
{
    //iounmap(md_img_vir);
    platform_deinit();
    ccci_free_smem();
}
#if 0
module_init(ccci_md_init_mod_init);
module_exit(ccci_md_init_mod_exit);
MODULE_AUTHOR("MediaTek Inc.");
MODULE_DESCRIPTION("MT6516 CCCI MD Driver");
MODULE_LICENSE("Proprietary");
#endif
EXPORT_SYMBOL(ccci_uart_setup);
EXPORT_SYMBOL(ccci_uart_base_req);
EXPORT_SYMBOL(ccci_fs_setup);
EXPORT_SYMBOL(ccci_fs_base_req);
EXPORT_SYMBOL(ccci_rpc_setup);
EXPORT_SYMBOL(ccci_rpc_base_req);
EXPORT_SYMBOL(ccci_pmic_setup);
EXPORT_SYMBOL(ccci_pmic_base_req);
EXPORT_SYMBOL(ccci_ipc_setup);
EXPORT_SYMBOL(ccci_ipc_base_req);
EXPORT_SYMBOL(ccci_reset_register);
EXPORT_SYMBOL(ccci_reset_request);
EXPORT_SYMBOL(md_notifier);
EXPORT_SYMBOL(md_call_chain);
EXPORT_SYMBOL(md_register_call_chain);
EXPORT_SYMBOL(md_unregister_call_chain);
//EXPORT_SYMBOL(ccci_msg_mask);
EXPORT_SYMBOL(ccci_aed);
EXPORT_SYMBOL(ccci_pcm_base_req);
EXPORT_SYMBOL(ccci_mdlog_base_req);
EXPORT_SYMBOL(reset_md);

