

#include <linux/types.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <asm/io.h>
//#include <asm/tcm.h>
#include <linux/proc_fs.h>  //proc file use
//
#include <linux/spinlock.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <asm/atomic.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/xlog.h> // For xlog_printk().
//
#include <mach/hardware.h>
//#include <mach/mt6589_pll.h>
#include <mach/camera_isp.h>
#include <mach/mt_reg_base.h>
#include <mach/mt_clkmgr.h>     // For clock mgr APIS. enable_clock()/disable_clock().
#include <mach/sync_write.h>    // For mt65xx_reg_sync_writel().
#include <mach/mt_spm_idle.h>    // For spm_enable_sodi()/spm_disable_sodi().

#include <mach/m4u.h>

//
#include "../smi/smi_common.h"

typedef unsigned char           MUINT8;
typedef unsigned int            MUINT32;
//
typedef signed char             MINT8;
typedef signed int              MINT32;
//
typedef bool                    MBOOL;
//
#ifndef MTRUE
    #define MTRUE               1
#endif
#ifndef MFALSE
    #define MFALSE              0
#endif
//----------------------------------------------------------------------------
//#define LOG_MSG(fmt, arg...)    printk(KERN_ERR "[ISP][%s]" fmt,__FUNCTION__, ##arg)
//#define LOG_DBG(fmt, arg...)    printk(KERN_ERR  "[ISP][%s]" fmt,__FUNCTION__, ##arg)
//#define LOG_WRN(fmt, arg...)    printk(KERN_ERR "[ISP][%s]Warning" fmt,__FUNCTION__, ##arg)
//#define LOG_ERR(fmt, arg...)    printk(KERN_ERR   "[ISP][%s]Err(%5d):" fmt, __FUNCTION__,__LINE__, ##arg)

#define LOG_VRB(format, args...)    xlog_printk(ANDROID_LOG_VERBOSE, "ISP", "[%s] " format, __FUNCTION__, ##args)
#define LOG_DBG(format, args...)    xlog_printk(ANDROID_LOG_DEBUG  , "ISP", "[%s] " format, __FUNCTION__, ##args)
#define LOG_INF(format, args...)    xlog_printk(ANDROID_LOG_INFO   , "ISP", "[%s] " format, __FUNCTION__, ##args)
#define LOG_WRN(format, args...)    xlog_printk(ANDROID_LOG_WARN   , "ISP", "[%s] WARNING: " format, __FUNCTION__, ##args)
#define LOG_ERR(format, args...)    xlog_printk(ANDROID_LOG_ERROR  , "ISP", "[%s, line%04d] ERROR: " format, __FUNCTION__, __LINE__, ##args)
#define LOG_AST(format, args...)    xlog_printk(ANDROID_LOG_ASSERT , "ISP", "[%s, line%04d] ASSERT: " format, __FUNCTION__, __LINE__, ##args)

/*******************************************************************************
*
********************************************************************************/
//#define ISP_WR32(addr, data)    iowrite32(data, addr) // For other projects.
#define ISP_WR32(addr, data)    mt65xx_reg_sync_writel(data, addr)    // For 89 Only.   // NEED_TUNING_BY_PROJECT
#define ISP_RD32(addr)          ioread32(addr)
#define ISP_SET_BIT(reg, bit)   ((*(volatile MUINT32*)(reg)) |= (MUINT32)(1 << (bit)))
#define ISP_CLR_BIT(reg, bit)   ((*(volatile MUINT32*)(reg)) &= ~((MUINT32)(1 << (bit))))
/*******************************************************************************
*
********************************************************************************/
#define ISP_DEV_NAME                "camera-isp"
#define ISP_IRQ_POLLING             (0)

/*******************************************************************************
*
********************************************************************************/
#define ISP_DBG_INT                 (0x00000001)
#define ISP_DBG_HOLD_REG            (0x00000002)
#define ISP_DBG_READ_REG            (0x00000004)
#define ISP_DBG_WRITE_REG           (0x00000008)
#define ISP_DBG_CLK                 (0x00000010)
#define ISP_DBG_TASKLET             (0x00000020)
#define ISP_DBG_SCHEDULE_WORK       (0x00000040)
#define ISP_DBG_BUF_WRITE           (0x00000080)
#define ISP_DBG_BUF_CTRL            (0x00000100)
#define ISP_DBG_REF_CNT_CTRL        (0x00000200)


/*******************************************************************************
*
********************************************************************************/
#define ISP_ADDR                        (CAMINF_BASE + 0x4000)
#define ISP_ADDR_CAMINF                 CAMINF_BASE
#define ISP_REG_ADDR_EN1                (ISP_ADDR + 0x4)
#define ISP_REG_ADDR_INT_STATUS         (ISP_ADDR + 0x24)
#define ISP_REG_ADDR_DMA_INT            (ISP_ADDR + 0x28)
#define ISP_REG_ADDR_INTB_STATUS        (ISP_ADDR + 0x30)
#define ISP_REG_ADDR_DMAB_INT           (ISP_ADDR + 0x34)
#define ISP_REG_ADDR_INTC_STATUS        (ISP_ADDR + 0x3C)
#define ISP_REG_ADDR_DMAC_INT           (ISP_ADDR + 0x40)
#define ISP_REG_ADDR_INT_STATUSX        (ISP_ADDR + 0x44)
#define ISP_REG_ADDR_DMA_INTX           (ISP_ADDR + 0x48)
#define ISP_REG_ADDR_SW_CTL             (ISP_ADDR + 0x5C)
#define ISP_REG_ADDR_IMGO_FBC           (ISP_ADDR + 0xF4)
#define ISP_REG_ADDR_IMG2O_FBC          (ISP_ADDR + 0xF8)
#define ISP_REG_ADDR_IMGO_BASE_ADDR     (ISP_ADDR + 0x300)
#define ISP_REG_ADDR_IMG2O_BASE_ADDR    (ISP_ADDR + 0x320)
#define ISP_REG_ADDR_TG_VF_CON          (ISP_ADDR + 0x414)
#define ISP_REG_ADDR_TG2_VF_CON         (ISP_ADDR + 0x4B4)

#define ISP_TPIPE_ADDR                  (0x15004000)


//CAM_CTL_SW_CTL
#define ISP_REG_SW_CTL_SW_RST_TRIG      (0x00000001)
#define ISP_REG_SW_CTL_SW_RST_STATUS    (0x00000002)
#define ISP_REG_SW_CTL_HW_RST           (0x00000004)
//CAM_CTL_INT_STATUS
//IRQ  Mask
#define ISP_REG_MASK_INT_STATUS         (   ISP_IRQ_INT_STATUS_VS1_ST |\
                                            ISP_IRQ_INT_STATUS_TG1_ST1 |\
                                            ISP_IRQ_INT_STATUS_TG1_ST2 |\
                                            ISP_IRQ_INT_STATUS_EXPDON1_ST |\
                                            ISP_IRQ_INT_STATUS_VS2_ST |\
                                            ISP_IRQ_INT_STATUS_TG2_ST1 |\
                                            ISP_IRQ_INT_STATUS_TG2_ST2 |\
                                            ISP_IRQ_INT_STATUS_EXPDON2_ST |\
                                            ISP_IRQ_INT_STATUS_PASS1_TG1_DON_ST |\
                                            ISP_IRQ_INT_STATUS_PASS1_TG2_DON_ST |\
                                            ISP_IRQ_INT_STATUS_SOF1_INT_ST |\
                                            ISP_IRQ_INT_STATUS_PASS2_DON_ST |\
                                            ISP_IRQ_INT_STATUS_TPIPE_DON_ST |\
                                            ISP_IRQ_INT_STATUS_AF_DON_ST |\
                                            ISP_IRQ_INT_STATUS_FLK_DON_ST |\
                                            ISP_IRQ_INT_STATUS_FMT_DON_ST |\
                                            ISP_IRQ_INT_STATUS_CQ_DON_ST)
//IRQ Error Mask
#define ISP_REG_MASK_INT_STATUS_ERR     (   ISP_IRQ_INT_STATUS_TG1_ERR_ST |\
                                            ISP_IRQ_INT_STATUS_TG2_ERR_ST |\
                                            ISP_IRQ_INT_STATUS_CQ_ERR_ST |\
                                            ISP_IRQ_INT_STATUS_IMGO_ERR_ST |\
                                            ISP_IRQ_INT_STATUS_AAO_ERR_ST |\
                                            ISP_IRQ_INT_STATUS_LCSO_ERR_ST |\
                                            ISP_IRQ_INT_STATUS_IMG2O_ERR_ST |\
                                            ISP_IRQ_INT_STATUS_ESFKO_ERR_ST |\
                                            ISP_IRQ_INT_STATUS_FLK_ERR_ST |\
                                            ISP_IRQ_INT_STATUS_LSC_ERR_ST |\
                                            ISP_IRQ_INT_STATUS_LSC2_ERR_ST |\
                                            ISP_IRQ_INT_STATUS_BPC_ERR_ST |\
                                            ISP_IRQ_INT_STATUS_LCE_ERR_ST |\
                                            ISP_IRQ_INT_STATUS_DMA_ERR_ST)
//CAM_CTL_DMA_INT
//IRQ  Mask
#define ISP_REG_MASK_DMA_INT            (   ISP_IRQ_DMA_INT_IMGO_DONE_ST |\
                                            ISP_IRQ_DMA_INT_IMG2O_DONE_ST |\
                                            ISP_IRQ_DMA_INT_AAO_DONE_ST |\
                                            ISP_IRQ_DMA_INT_LCSO_DONE_ST |\
                                            ISP_IRQ_DMA_INT_ESFKO_DONE_ST |\
                                            ISP_IRQ_DMA_INT_DISPO_DONE_ST |\
                                            ISP_IRQ_DMA_INT_VIDO_DONE_ST |\
                                            ISP_IRQ_DMA_INT_VRZO_DONE_ST |\
                                            ISP_IRQ_DMA_INT_BUF_OVL_ST)
//IRQ Error Mask
#define ISP_REG_MASK_DMA_INT_ERR        (   ISP_IRQ_DMA_INT_CQ0_ERR_ST |\
                                            ISP_IRQ_DMA_INT_TG1_GBERR_ST |\
                                            ISP_IRQ_DMA_INT_TG2_GBERR_ST)

//CAM_CTL_INTB_STATUS
//IRQ  Mask
#define ISP_REG_MASK_INTB_STATUS        (   ISP_IRQ_INTB_STATUS_PASS2_DON_ST |\
                                            ISP_IRQ_INTB_STATUS_TPIPE_DON_ST |\
                                            ISP_IRQ_INTB_STATUS_CQ_DON_ST)
//IRQ Error Mask
#define ISP_REG_MASK_INTB_STATUS_ERR    (   ISP_IRQ_INTB_STATUS_CQ_ERR_ST |\
                                            ISP_IRQ_INTB_STATUS_IMGO_ERR_ST |\
                                            ISP_IRQ_INTB_STATUS_LCSO_ERR_ST |\
                                            ISP_IRQ_INTB_STATUS_IMG2O_ERR_ST |\
                                            ISP_IRQ_INTB_STATUS_LSC_ERR_ST |\
                                            ISP_IRQ_INTB_STATUS_LCE_ERR_ST |\
                                            ISP_IRQ_INTB_STATUS_DMA_ERR_ST)
//CAM_CTL_DMAB_INT
//IRQ  Mask
#define ISP_REG_MASK_DMAB_INT           (   ISP_IRQ_DMAB_INT_IMGO_DONE_ST |\
                                            ISP_IRQ_DMAB_INT_IMG2O_DONE_ST |\
                                            ISP_IRQ_DMAB_INT_AAO_DONE_ST |\
                                            ISP_IRQ_DMAB_INT_LCSO_DONE_ST |\
                                            ISP_IRQ_DMAB_INT_ESFKO_DONE_ST |\
                                            ISP_IRQ_DMAB_INT_DISPO_DONE_ST |\
                                            ISP_IRQ_DMAB_INT_VIDO_DONE_ST |\
                                            ISP_IRQ_DMAB_INT_VRZO_DONE_ST |\
                                            ISP_IRQ_DMAB_INT_NR3O_DONE_ST)
//IRQ Error Mask
#define ISP_REG_MASK_DMAB_INT_ERR       (   ISP_IRQ_DMAB_INT_NR3O_ERR_ST)
//CAM_CTL_INTC_STATUS
//IRQ  Mask
#define ISP_REG_MASK_INTC_STATUS        (   ISP_IRQ_INTC_STATUS_PASS2_DON_ST |\
                                            ISP_IRQ_INTC_STATUS_TPIPE_DON_ST |\
                                            ISP_IRQ_INTC_STATUS_CQ_DON_ST)
//IRQ Error Mask
#define ISP_REG_MASK_INTC_STATUS_ERR    (   ISP_IRQ_INTC_STATUS_CQ_ERR_ST |\
                                            ISP_IRQ_INTC_STATUS_IMGO_ERR_ST |\
                                            ISP_IRQ_INTC_STATUS_LCSO_ERR_ST |\
                                            ISP_IRQ_INTC_STATUS_IMG2O_ERR_ST |\
                                            ISP_IRQ_INTC_STATUS_LSC_ERR_ST |\
                                            ISP_IRQ_INTC_STATUS_BPC_ERR_ST |\
                                            ISP_IRQ_INTC_STATUS_LCE_ERR_ST |\
                                            ISP_IRQ_INTC_STATUS_DMA_ERR_ST)
//CAM_CTL_DMAC_INT
//IRQ  Mask
#define ISP_REG_MASK_DMAC_INT           (   ISP_IRQ_DMAC_INT_IMGO_DONE_ST |\
                                            ISP_IRQ_DMAC_INT_IMG2O_DONE_ST |\
                                            ISP_IRQ_DMAC_INT_AAO_DONE_ST |\
                                            ISP_IRQ_DMAC_INT_LCSO_DONE_ST |\
                                            ISP_IRQ_DMAC_INT_ESFKO_DONE_ST |\
                                            ISP_IRQ_DMAC_INT_DISPO_DONE_ST |\
                                            ISP_IRQ_DMAC_INT_VIDO_DONE_ST |\
                                            ISP_IRQ_DMAC_INT_VRZO_DONE_ST |\
                                            ISP_IRQ_DMAC_INT_NR3O_DONE_ST)
//IRQ Error Mask
#define ISP_REG_MASK_DMAC_INT_ERR       (   ISP_IRQ_DMAC_INT_NR3O_ERR_ST)
//CAM_CTL_INT_STATUSX
//IRQ  Mask
#define ISP_REG_MASK_INTX_STATUS        (   ISP_IRQ_INTX_STATUS_VS1_ST |\
                                            ISP_IRQ_INTX_STATUS_TG1_ST1 |\
                                            ISP_IRQ_INTX_STATUS_TG1_ST2 |\
                                            ISP_IRQ_INTX_STATUS_EXPDON1_ST |\
                                            ISP_IRQ_INTX_STATUS_VS2_ST |\
                                            ISP_IRQ_INTX_STATUS_TG2_ST1 |\
                                            ISP_IRQ_INTX_STATUS_TG2_ST2 |\
                                            ISP_IRQ_INTX_STATUS_EXPDON2_ST |\
                                            ISP_IRQ_INTX_STATUS_PASS1_TG1_DON_ST |\
                                            ISP_IRQ_INTX_STATUS_PASS1_TG2_DON_ST |\
                                            ISP_IRQ_INTX_STATUS_VEC_DON_ST |\
                                            ISP_IRQ_INTX_STATUS_PASS2_DON_ST |\
                                            ISP_IRQ_INTX_STATUS_TPIPE_DON_ST |\
                                            ISP_IRQ_INTX_STATUS_AF_DON_ST |\
                                            ISP_IRQ_INTX_STATUS_FLK_DON_ST |\
                                            ISP_IRQ_INTX_STATUS_FMT_DON_ST |\
                                            ISP_IRQ_INTX_STATUS_CQ_DON_ST)
//IRQ Error Mask
#define ISP_REG_MASK_INTX_STATUS_ERR    (   ISP_IRQ_INTX_STATUS_TG1_ERR_ST |\
                                            ISP_IRQ_INTX_STATUS_TG2_ERR_ST |\
                                            ISP_IRQ_INTX_STATUS_CQ_ERR_ST |\
                                            ISP_IRQ_INTX_STATUS_AAO_ERR_ST |\
                                            ISP_IRQ_INTX_STATUS_LCSO_ERR_ST |\
                                            ISP_IRQ_INTX_STATUS_LCSO_ERR_ST |\
                                            ISP_IRQ_INTX_STATUS_ESFKO_ERR_ST |\
                                            ISP_IRQ_INTX_STATUS_FLK_ERR_ST |\
                                            ISP_IRQ_INTX_STATUS_LSC_ERR_ST |\
                                            ISP_IRQ_INTX_STATUS_LSC2_ERR_ST |\
                                            ISP_IRQ_INTX_STATUS_LCE_ERR_ST)
//                                            ISP_IRQ_INTX_STATUS_IMGO_ERR_ST |\
//                                            ISP_IRQ_INTX_STATUS_DMA_ERR_ST)      //DMA_ERR rootcause not clear. it caused resume problem with busy log print out.
//                                            ISP_IRQ_INTX_STATUS_BPC_ERR_ST |\    //Vent@20121025: Remove ISP_IRQ_INTX_STATUS_BPC_ERR_ST. From TH Wu's explanation, this bit is not used as an error state anymore.


//CAM_CTL_DMA_INTX
//IRQ  Mask
#define ISP_REG_MASK_DMAX_INT           (   ISP_IRQ_DMAX_INT_IMGO_DONE_ST |\
                                            ISP_IRQ_DMAX_INT_IMG2O_DONE_ST |\
                                            ISP_IRQ_DMAX_INT_AAO_DONE_ST |\
                                            ISP_IRQ_DMAX_INT_LCSO_DONE_ST |\
                                            ISP_IRQ_DMAX_INT_ESFKO_DONE_ST |\
                                            ISP_IRQ_DMAX_INT_DISPO_DONE_ST |\
                                            ISP_IRQ_DMAX_INT_VIDO_DONE_ST |\
                                            ISP_IRQ_DMAX_INT_VRZO_DONE_ST |\
                                            ISP_IRQ_DMAX_INT_NR3O_DONE_ST |\
                                            ISP_IRQ_DMAX_INT_BUF_OVL_ST)
//IRQ Error Mask
#define ISP_REG_MASK_DMAX_INT_ERR       (   ISP_IRQ_DMAX_INT_NR3O_ERR_ST |\
                                            ISP_IRQ_DMAX_INT_CQ_ERR_ST |\
                                            ISP_IRQ_DMAX_INT_TG1_GBERR_ST |\
                                            ISP_IRQ_DMAX_INT_TG2_GBERR_ST)

/*******************************************************************************
*
********************************************************************************/
// internal data
// pointer to the kmalloc'd area, rounded up to a page boundary
static int* pTbl_RTBuf = NULL;
// original pointer for kmalloc'd area as returned by kmalloc
static void* pBuf_kmalloc = NULL;
//
static ISP_RT_BUF_STRUCT* pstRTBuf = NULL;

//static ISP_DEQUE_BUF_INFO_STRUCT g_deque_buf = {0,{}};    // Marked to remove build warning.

unsigned long g_Flash_SpinLock;


unsigned int G_u4EnableClockCount = 0;
volatile MUINT32 TempAddr = 0;


/*******************************************************************************
*
********************************************************************************/
typedef struct
{
    pid_t   Pid;
    pid_t   Tid;
}ISP_USER_INFO_STRUCT;

/*******************************************************************************
*
********************************************************************************/
#define ISP_BUF_SIZE            (4096)
#define ISP_BUF_SIZE_WRITE      1024
#define ISP_BUF_WRITE_AMOUNT    6

typedef enum
{
    ISP_BUF_STATUS_EMPTY,
    ISP_BUF_STATUS_HOLD,
    ISP_BUF_STATUS_READY
}ISP_BUF_STATUS_ENUM;

typedef struct
{
    volatile ISP_BUF_STATUS_ENUM    Status;
    volatile MUINT32                Size;
    MUINT8*                         pData;
}ISP_BUF_STRUCT;

typedef struct
{
    ISP_BUF_STRUCT      Read;
    ISP_BUF_STRUCT      Write[ISP_BUF_WRITE_AMOUNT];
}ISP_BUF_INFO_STRUCT;


/*******************************************************************************
*
********************************************************************************/
typedef struct
{
    atomic_t            HoldEnable;
    atomic_t            WriteEnable;
    ISP_HOLD_TIME_ENUM  Time;
}ISP_HOLD_INFO_STRUCT;

typedef struct
{
    MUINT32     Status[ISP_IRQ_TYPE_AMOUNT];
    MUINT32     Mask[ISP_IRQ_TYPE_AMOUNT];
    MUINT32     ErrMask[ISP_IRQ_TYPE_AMOUNT];
}ISP_IRQ_INFO_STRUCT;

typedef struct
{
    MUINT32     Vd;
    MUINT32     Expdone;
    MUINT32     WorkQueueVd;
    MUINT32     WorkQueueExpdone;
    MUINT32     TaskletVd;
    MUINT32     TaskletExpdone;
}ISP_TIME_LOG_STRUCT;

typedef struct
{
    spinlock_t              SpinLockIspRef;
    spinlock_t              SpinLockIsp;
    spinlock_t              SpinLockIrq;
    spinlock_t              SpinLockHold;
    spinlock_t              SpinLockRTBC;
    wait_queue_head_t       WaitQueueHead;
    struct work_struct      ScheduleWorkVD;
    struct work_struct      ScheduleWorkEXPDONE;
    MUINT32                 UserCount;
    MUINT32                 DebugMask;
    MINT32                  IrqNum;
    ISP_IRQ_INFO_STRUCT     IrqInfo;
    ISP_HOLD_INFO_STRUCT    HoldInfo;
    ISP_BUF_INFO_STRUCT     BufInfo;
    ISP_TIME_LOG_STRUCT     TimeLog;
    ISP_CALLBACK_STRUCT     Callback[ISP_CALLBACK_AMOUNT];
}ISP_INFO_STRUCT;

static ISP_INFO_STRUCT IspInfo;


/*******************************************************************************
*
********************************************************************************/
//test flag
#define ISP_KERNEL_MOTIFY_SINGAL_TEST
#ifdef ISP_KERNEL_MOTIFY_SINGAL_TEST
/*** Linux signal test ***/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/siginfo.h>	//siginfo
#include <linux/rcupdate.h>	//rcu_read_lock
#include <linux/sched.h>	//find_task_by_pid_type
#include <linux/debugfs.h>
#include <linux/uaccess.h>

//js_test
#define __tcmfunc


#define SIG_TEST 44	// we choose 44 as our signal number (real-time signals are in the range of 33 to 64)

struct siginfo info;
struct task_struct *t;


int getTaskInfo( pid_t pid )
{
	//<<JB
	LOG_DBG("getTaskInfo");
	//>>JB
	/* send the signal */
	memset(&info, 0, sizeof(struct siginfo));
	info.si_signo = SIG_TEST;
	info.si_code = SI_QUEUE;	// this is bit of a trickery: SI_QUEUE is normally used by sigqueue from user space,
					            // and kernel space should use SI_KERNEL. But if SI_KERNEL is used the real_time data
					            // is not delivered to the user space signal handler function.
	info.si_int = 1234;  		// real time signals may have 32 bits of data.

	rcu_read_lock();

	t = find_task_by_vpid(pid);
	//t = find_task_by_pid_type(PIDTYPE_PID, g_pid);  //find the task_struct associated with this pid
	if(t == NULL){
		LOG_DBG("no such pid");
		rcu_read_unlock();
		return -ENODEV;
	}
	rcu_read_unlock();

    return 0;
}

int sendSignal( void )
{
int ret = 0;
	//<<JB
	LOG_DBG("sendSignal");
	//>>JB
	ret = send_sig_info(SIG_TEST, &info, t);    //send the signal
	if (ret < 0) {
		LOG_DBG("error sending signal");
		return ret;
	}

	return ret;
}

/*** Linux signal test ***/

#endif  // ISP_KERNEL_MOTIFY_SINGAL_TEST

/*******************************************************************************
*
********************************************************************************/
static __inline MUINT32 ISP_MsToJiffies(MUINT32 Ms)
{
	//<<JB
	LOG_DBG("ISP_MsToJiffies");
	//>>JB
    return ((Ms * HZ + 512) >> 10);
}


/*******************************************************************************
*
********************************************************************************/
static __inline MUINT32 ISP_GetIRQState(MUINT32 type, MUINT32 stus)
{
    MUINT32 ret;
    MUINT32 flags;
    //<<JB
	LOG_DBG("ISP_GetIRQState");
	//>>JB
    //
    spin_lock_irqsave(&(IspInfo.SpinLockIrq), flags);
    ret = (IspInfo.IrqInfo.Status[type] & stus);
    spin_unlock_irqrestore(&(IspInfo.SpinLockIrq), flags);
    //
    return ret;
}


/*******************************************************************************
*
********************************************************************************/
static __inline MUINT32 ISP_JiffiesToMs(MUINT32 Jiffies)
{
	//<<JB
	LOG_DBG("ISP_JiffiesToMs");
	//>>JB
    return ((Jiffies*1000)/HZ);
}


/*******************************************************************************
*
********************************************************************************/
static MINT32 ISP_DumpReg(void)
{
    MINT32 Ret = 0;
    MINT32 i;
    //
    LOG_DBG("- E.");
    //
    //spin_lock_irqsave(&(IspInfo.SpinLock), flags);

    //tile tool parse range
    //Joseph Hung (xa)#define ISP_ADDR_START  0x15004000
    //                              #define ISP_ADDR_END    0x15006000
    //

	//<<JB
	LOG_DBG("ISP_DumpReg");
	//>>JB
#if 1  //kk test 0:new tile format
    for(i = 0x0; i <= 0x20; i += 4)
    {
        //LOG_DBG("0x%08X %08X ", ISP_ADDR + i, ISP_RD32(ISP_ADDR + i));
        LOG_DBG("0x%08X %08X ", ISP_ADDR + i, ISP_RD32(ISP_ADDR + i));
    }
    //ignore read clear registers
    LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x24, 0);
    LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x28, 0);
    LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x2C, ISP_RD32(ISP_ADDR + 0x2C));
    LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x30, 0);
    LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x34, 0);
    LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x38, ISP_RD32(ISP_ADDR + 0x38));
    LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x3C, 0);
    LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x40, 0);
    LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x44, 0);
    LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x48, 0);
    for(i = 0x4C; i <= 0x5E08; i += 4)
    {
        //LOG_DBG("0x%08X %08X ", ISP_ADDR + i, ISP_RD32(ISP_ADDR + i));
        LOG_DBG("0x%08X %08X ", ISP_ADDR + i, ISP_RD32(ISP_ADDR + i));
    }

{
    int tpipePA = ISP_RD32(ISP_ADDR + 0x204);
    int ctlStart = ISP_RD32(ISP_ADDR + 0x000);
    int ctlTcm = ISP_RD32(ISP_ADDR + 0x054);
    int map_va=0, map_size;
    int i;
    int *pMapVa;
#define TPIPE_DUMP_SIZE    200

    if((ctlStart&0x01)&&(tpipePA)&&(ctlTcm&0x80000000)){ // for pass2
        map_va = 0;
        m4u_mva_map_kernel( tpipePA, TPIPE_DUMP_SIZE, 0, &map_va, &map_size);
        pMapVa = map_va;
        LOG_DBG("pMapVa(0x%x),map_size(0x%x)",pMapVa,map_size);
        LOG_DBG("ctlStart(0x%x),tpipePA(0x%x),ctlTcm(0x%x)",ctlStart,tpipePA,ctlTcm);
        if(pMapVa){
            for(i=0;i<TPIPE_DUMP_SIZE;i+=10) {
                LOG_DBG("[idx(%d)]%08X-%08X-%08X-%08X-%08X-%08X-%08X-%08X-%08X-%08X",i,pMapVa[i],pMapVa[i+1],pMapVa[i+2],pMapVa[i+3],
                    pMapVa[i+4],pMapVa[i+5],pMapVa[i+6],pMapVa[i+7],pMapVa[i+8],pMapVa[i+9]);
            }
        }
        m4u_mva_unmap_kernel(tpipePA, map_size, map_va);
    }
}

#else

    //
    ////////////// for tpipe main start //////
    LOG_DBG("start MT6589");
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x000, ISP_RD32(ISP_ADDR + 0x000));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x004, ISP_RD32(ISP_ADDR + 0x004));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x008, ISP_RD32(ISP_ADDR + 0x008));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x00C, ISP_RD32(ISP_ADDR + 0x00C));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x010, ISP_RD32(ISP_ADDR + 0x010));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x018, ISP_RD32(ISP_ADDR + 0x018));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x01C, ISP_RD32(ISP_ADDR + 0x01C));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x020, ISP_RD32(ISP_ADDR + 0x020));
#if 1 //it may touch ReadClear register
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x024, ISP_RD32(ISP_ADDR + 0x024));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x028, ISP_RD32(ISP_ADDR + 0x028));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x02C, ISP_RD32(ISP_ADDR + 0x02C));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x030, ISP_RD32(ISP_ADDR + 0x030));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x034, ISP_RD32(ISP_ADDR + 0x034));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x038, ISP_RD32(ISP_ADDR + 0x038));
#else
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x024, 0);
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x028, 0);
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x02C, 0);
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x030, 0);
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x034, 0);
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x038, 0);
#endif
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x050, ISP_RD32(ISP_ADDR + 0x050));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x054, ISP_RD32(ISP_ADDR + 0x054));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x080, ISP_RD32(ISP_ADDR + 0x080));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x084, ISP_RD32(ISP_ADDR + 0x084));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x088, ISP_RD32(ISP_ADDR + 0x088));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x08C, ISP_RD32(ISP_ADDR + 0x08C));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x090, ISP_RD32(ISP_ADDR + 0x090));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x094, ISP_RD32(ISP_ADDR + 0x094));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x0A8, ISP_RD32(ISP_ADDR + 0x0A8));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x0AC, ISP_RD32(ISP_ADDR + 0x0AC));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x0B0, ISP_RD32(ISP_ADDR + 0x0B0));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x0B4, ISP_RD32(ISP_ADDR + 0x0B4));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x0E8, ISP_RD32(ISP_ADDR + 0x0E8));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x138, ISP_RD32(ISP_ADDR + 0x138));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x13C, ISP_RD32(ISP_ADDR + 0x13C));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x144, ISP_RD32(ISP_ADDR + 0x144));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x148, ISP_RD32(ISP_ADDR + 0x148));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x14C, ISP_RD32(ISP_ADDR + 0x14C));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x204, ISP_RD32(ISP_ADDR + 0x204));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x20C, ISP_RD32(ISP_ADDR + 0x20C));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x230, ISP_RD32(ISP_ADDR + 0x230));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x234, ISP_RD32(ISP_ADDR + 0x234));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x238, ISP_RD32(ISP_ADDR + 0x238));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x23C, ISP_RD32(ISP_ADDR + 0x23C));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x240, ISP_RD32(ISP_ADDR + 0x240));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x244, ISP_RD32(ISP_ADDR + 0x244));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x258, ISP_RD32(ISP_ADDR + 0x258));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x260, ISP_RD32(ISP_ADDR + 0x260));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x26C, ISP_RD32(ISP_ADDR + 0x26C));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x270, ISP_RD32(ISP_ADDR + 0x270));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x27C, ISP_RD32(ISP_ADDR + 0x27C));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x298, ISP_RD32(ISP_ADDR + 0x298));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x2B4, ISP_RD32(ISP_ADDR + 0x2B4));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x2D0, ISP_RD32(ISP_ADDR + 0x2D0));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x2D4, ISP_RD32(ISP_ADDR + 0x2D4));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x2F0, ISP_RD32(ISP_ADDR + 0x2F0));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x2F4, ISP_RD32(ISP_ADDR + 0x2F4));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x300, ISP_RD32(ISP_ADDR + 0x300));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x304, ISP_RD32(ISP_ADDR + 0x304));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x308, ISP_RD32(ISP_ADDR + 0x308));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x30C, ISP_RD32(ISP_ADDR + 0x30C));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x310, ISP_RD32(ISP_ADDR + 0x310));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x330, ISP_RD32(ISP_ADDR + 0x330));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x350, ISP_RD32(ISP_ADDR + 0x350));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x37C, ISP_RD32(ISP_ADDR + 0x37C));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x398, ISP_RD32(ISP_ADDR + 0x398));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x410, ISP_RD32(ISP_ADDR + 0x410));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x414, ISP_RD32(ISP_ADDR + 0x414));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x418, ISP_RD32(ISP_ADDR + 0x418));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x41C, ISP_RD32(ISP_ADDR + 0x41C));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x420, ISP_RD32(ISP_ADDR + 0x420));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x448, ISP_RD32(ISP_ADDR + 0x448));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4B0, ISP_RD32(ISP_ADDR + 0x4B0));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4B4, ISP_RD32(ISP_ADDR + 0x4B4));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4B8, ISP_RD32(ISP_ADDR + 0x4B8));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4BC, ISP_RD32(ISP_ADDR + 0x4BC));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4E8, ISP_RD32(ISP_ADDR + 0x4E8));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x534, ISP_RD32(ISP_ADDR + 0x534));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x538, ISP_RD32(ISP_ADDR + 0x538));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x53C, ISP_RD32(ISP_ADDR + 0x53C));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x800, ISP_RD32(ISP_ADDR + 0x800));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x880, ISP_RD32(ISP_ADDR + 0x880));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x884, ISP_RD32(ISP_ADDR + 0x884));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x888, ISP_RD32(ISP_ADDR + 0x888));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x8A0, ISP_RD32(ISP_ADDR + 0x8A0));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x920, ISP_RD32(ISP_ADDR + 0x920));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x924, ISP_RD32(ISP_ADDR + 0x924));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x928, ISP_RD32(ISP_ADDR + 0x928));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x92C, ISP_RD32(ISP_ADDR + 0x92C));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x930, ISP_RD32(ISP_ADDR + 0x930));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x934, ISP_RD32(ISP_ADDR + 0x934));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x938, ISP_RD32(ISP_ADDR + 0x938));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x93C, ISP_RD32(ISP_ADDR + 0x93C));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x960, ISP_RD32(ISP_ADDR + 0x960));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x9C4, ISP_RD32(ISP_ADDR + 0x9C4));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x9E4, ISP_RD32(ISP_ADDR + 0x9E4));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x9E8, ISP_RD32(ISP_ADDR + 0x9E8));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x9EC, ISP_RD32(ISP_ADDR + 0x9EC));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xA00, ISP_RD32(ISP_ADDR + 0xA00));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xA04, ISP_RD32(ISP_ADDR + 0xA04));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xA08, ISP_RD32(ISP_ADDR + 0xA08));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xA0C, ISP_RD32(ISP_ADDR + 0xA0C));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xA10, ISP_RD32(ISP_ADDR + 0xA10));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xA14, ISP_RD32(ISP_ADDR + 0xA14));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xA20, ISP_RD32(ISP_ADDR + 0xA20));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xAA0, ISP_RD32(ISP_ADDR + 0xAA0));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xACC, ISP_RD32(ISP_ADDR + 0xACC));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xB00, ISP_RD32(ISP_ADDR + 0xB00));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xB04, ISP_RD32(ISP_ADDR + 0xB04));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xB08, ISP_RD32(ISP_ADDR + 0xB08));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xB0C, ISP_RD32(ISP_ADDR + 0xB0C));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xB10, ISP_RD32(ISP_ADDR + 0xB10));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xB14, ISP_RD32(ISP_ADDR + 0xB14));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xB18, ISP_RD32(ISP_ADDR + 0xB18));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xB1C, ISP_RD32(ISP_ADDR + 0xB1C));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xB20, ISP_RD32(ISP_ADDR + 0xB20));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xB44, ISP_RD32(ISP_ADDR + 0xB44));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xB48, ISP_RD32(ISP_ADDR + 0xB48));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xB4C, ISP_RD32(ISP_ADDR + 0xB4C));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xB50, ISP_RD32(ISP_ADDR + 0xB50));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xB54, ISP_RD32(ISP_ADDR + 0xB54));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xB58, ISP_RD32(ISP_ADDR + 0xB58));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xB5C, ISP_RD32(ISP_ADDR + 0xB5C));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xB60, ISP_RD32(ISP_ADDR + 0xB60));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xBA0, ISP_RD32(ISP_ADDR + 0xBA0));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xBA4, ISP_RD32(ISP_ADDR + 0xBA4));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xBA8, ISP_RD32(ISP_ADDR + 0xBA8));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xBAC, ISP_RD32(ISP_ADDR + 0xBAC));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xBB0, ISP_RD32(ISP_ADDR + 0xBB0));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xBB4, ISP_RD32(ISP_ADDR + 0xBB4));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xBB8, ISP_RD32(ISP_ADDR + 0xBB8));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xBBC, ISP_RD32(ISP_ADDR + 0xBBC));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xBC0, ISP_RD32(ISP_ADDR + 0xBC0));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xC20, ISP_RD32(ISP_ADDR + 0xC20));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xCC0, ISP_RD32(ISP_ADDR + 0xCC0));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xCE4, ISP_RD32(ISP_ADDR + 0xCE4));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xCE8, ISP_RD32(ISP_ADDR + 0xCE8));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xCEC, ISP_RD32(ISP_ADDR + 0xCEC));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xCF0, ISP_RD32(ISP_ADDR + 0xCF0));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xCF4, ISP_RD32(ISP_ADDR + 0xCF4));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xCF8, ISP_RD32(ISP_ADDR + 0xCF8));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xCFC, ISP_RD32(ISP_ADDR + 0xCFC));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xD24, ISP_RD32(ISP_ADDR + 0xD24));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xD28, ISP_RD32(ISP_ADDR + 0xD28));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xD2C, ISP_RD32(ISP_ADDR + 0xD2c));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xD40, ISP_RD32(ISP_ADDR + 0xD40));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xD64, ISP_RD32(ISP_ADDR + 0xD64));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xD68, ISP_RD32(ISP_ADDR + 0xD68));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xD6C, ISP_RD32(ISP_ADDR + 0xD6c));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xD70, ISP_RD32(ISP_ADDR + 0xD70));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xD74, ISP_RD32(ISP_ADDR + 0xD74));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xD78, ISP_RD32(ISP_ADDR + 0xD78));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xD7C, ISP_RD32(ISP_ADDR + 0xD7C));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xDA4, ISP_RD32(ISP_ADDR + 0xDA4));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xDA8, ISP_RD32(ISP_ADDR + 0xDA8));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xDAC, ISP_RD32(ISP_ADDR + 0xDAC));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4010, ISP_RD32(ISP_ADDR + 0x4010));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4014, ISP_RD32(ISP_ADDR + 0x4014));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4018, ISP_RD32(ISP_ADDR + 0x4018));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x401C, ISP_RD32(ISP_ADDR + 0x401C));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4020, ISP_RD32(ISP_ADDR + 0x4020));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4024, ISP_RD32(ISP_ADDR + 0x4024));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4028, ISP_RD32(ISP_ADDR + 0x4028));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x402C, ISP_RD32(ISP_ADDR + 0x402C));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4090, ISP_RD32(ISP_ADDR + 0x4090));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4094, ISP_RD32(ISP_ADDR + 0x4094));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4098, ISP_RD32(ISP_ADDR + 0x4098));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x409C, ISP_RD32(ISP_ADDR + 0x409C));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x40A0, ISP_RD32(ISP_ADDR + 0x40A0));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x40A4, ISP_RD32(ISP_ADDR + 0x40A4));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x40A8, ISP_RD32(ISP_ADDR + 0x40A8));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x40AC, ISP_RD32(ISP_ADDR + 0x40AC));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4100, ISP_RD32(ISP_ADDR + 0x4100));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4104, ISP_RD32(ISP_ADDR + 0x4104));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4108, ISP_RD32(ISP_ADDR + 0x4108));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x410C, ISP_RD32(ISP_ADDR + 0x410C));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4120, ISP_RD32(ISP_ADDR + 0x4120));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x412C, ISP_RD32(ISP_ADDR + 0x412C));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4180, ISP_RD32(ISP_ADDR + 0x4180));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4184, ISP_RD32(ISP_ADDR + 0x4184));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4188, ISP_RD32(ISP_ADDR + 0x4188));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x418C, ISP_RD32(ISP_ADDR + 0x418C));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x41A0, ISP_RD32(ISP_ADDR + 0x41A0));
    LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x41AC, ISP_RD32(ISP_ADDR + 0x41AC));

    LOG_DBG("end MT6589");
    ////////////// for tpipe main end //////
    //INT STX
    for( i = 0x44; i <= 0x48; i += 4)
    {
        LOG_DBG("0x%08X %08X ", ISP_TPIPE_ADDR + i, ISP_RD32(ISP_ADDR + i));
    }
    //DMA
    for( i = 0x248; i <= 0x24C; i += 4)//IMGI
    {
        LOG_DBG("0x%08X %08X ", ISP_TPIPE_ADDR + i, ISP_RD32(ISP_ADDR + i));
    }
    for( i = 0x280; i <= 0x284; i += 4)//LSCI
    {
        LOG_DBG("0x%08X %08X ", ISP_TPIPE_ADDR + i, ISP_RD32(ISP_ADDR + i));
    }
    for( i = 0x29C; i <= 0x2A0; i += 4)//FLKI
    {
        LOG_DBG("0x%08X %08X ", ISP_TPIPE_ADDR + i, ISP_RD32(ISP_ADDR + i));
    }
    for( i = 0x2D8; i <= 0x2DC; i += 4)//VIPI
    {
        LOG_DBG("0x%08X %08X ", ISP_TPIPE_ADDR + i, ISP_RD32(ISP_ADDR + i));
    }
    for( i = 0x2F8; i <= 0x2FC; i += 4)//VIP2I
    {
        LOG_DBG("0x%08X %08X ", ISP_TPIPE_ADDR + i, ISP_RD32(ISP_ADDR + i));
    }
    for( i = 0x300; i <= 0x31C; i += 4)//IMGO
    {
        LOG_DBG("0x%08X %08X ", ISP_TPIPE_ADDR + i, ISP_RD32(ISP_ADDR + i));
    }
    for( i = 0x334; i <= 0x338; i += 4)//IMG2O
    {
        LOG_DBG("0x%08X %08X ", ISP_TPIPE_ADDR + i, ISP_RD32(ISP_ADDR + i));
    }
    for( i = 0x380; i <= 0x384; i += 4)//ESFKO
    {
        LOG_DBG("0x%08X %08X ", ISP_TPIPE_ADDR + i, ISP_RD32(ISP_ADDR + i));
    }
    for( i = 0x39C; i <= 0x3A0; i += 4)//ESFKO
    {
        LOG_DBG("0x%08X %08X ", ISP_TPIPE_ADDR + i, ISP_RD32(ISP_ADDR + i));
    }
    //DMA ERR ST
    for( i = 0x3A4; i <= 0x3D8; i += 4)
    {
        LOG_DBG("0x%08X %08X ", ISP_TPIPE_ADDR + i, ISP_RD32(ISP_ADDR + i));
    }
    //TG1
    for( i = 0x410; i <= 0x44C; i += 4)
    {
        LOG_DBG("0x%08X %08X ", ISP_TPIPE_ADDR + i, ISP_RD32(ISP_ADDR + i));
    }
    //TG2
    for( i = 0x4B0; i <= 0x4EC; i += 4)
    {
        LOG_DBG("0x%08X %08X ", ISP_TPIPE_ADDR + i, ISP_RD32(ISP_ADDR + i));
    }
    //CDP
    LOG_DBG("0x%08X %08X ", ISP_TPIPE_ADDR + 0xCC0, ISP_RD32(ISP_ADDR + 0xCC0));
    LOG_DBG("0x%08X %08X ", ISP_TPIPE_ADDR + 0xD40, ISP_RD32(ISP_ADDR + 0xD40));
    //SENINF1_INT_STA
    for( i = 0x4014; i <= 0x4018; i += 4)
    {
        LOG_DBG("0x%08X %08X ", ISP_TPIPE_ADDR + i, ISP_RD32(ISP_ADDR + i));
    }
    //
    LOG_DBG("0x%08X %08X ", ISP_ADDR_CAMINF, ISP_RD32(ISP_ADDR_CAMINF));
    LOG_DBG("0x%08X %08X ", ISP_TPIPE_ADDR + 0x150, ISP_RD32(ISP_ADDR + 0x150));
    //
    //GDMA_CTL
    for( i = 0xE50; i <= 0xE84; i += 4)
    {
        LOG_DBG("0x%08X %08X ", ISP_TPIPE_ADDR + i, ISP_RD32(ISP_ADDR + i));
    }
    //GDMA_FMT
    for( i = 0xE90; i <= 0xEDC; i += 4)
    {
        LOG_DBG("0x%08X %08X ", ISP_TPIPE_ADDR + i, ISP_RD32(ISP_ADDR + i));
    }


{
    int tpipePA = ISP_RD32(ISP_ADDR + 0x204);
    int ctlStart = ISP_RD32(ISP_ADDR + 0x000);
    int ctlTcm = ISP_RD32(ISP_ADDR + 0x054);
    int map_va=0, map_size;
    int i;
    int *pMapVa;
#define TPIPE_DUMP_SIZE    200

    if((ctlStart&0x01)&&(tpipePA)&&(ctlTcm&0x80000000)){ // for pass2
        map_va = 0;
        m4u_mva_map_kernel( tpipePA, TPIPE_DUMP_SIZE, 0, &map_va, &map_size);
        pMapVa = map_va;
        LOG_DBG("pMapVa(0x%x),map_size(0x%x)",pMapVa,map_size);
        LOG_DBG("ctlStart(0x%x),tpipePA(0x%x),ctlTcm(0x%x)",ctlStart,tpipePA,ctlTcm);
        if(pMapVa){
            for(i=0;i<TPIPE_DUMP_SIZE;i+=10) {
                LOG_DBG("[idx(%d)]%08X-%08X-%08X-%08X-%08X-%08X-%08X-%08X-%08X-%08X",i,pMapVa[i],pMapVa[i+1],pMapVa[i+2],pMapVa[i+3],
                    pMapVa[i+4],pMapVa[i+5],pMapVa[i+6],pMapVa[i+7],pMapVa[i+8],pMapVa[i+9]);
            }
        }
        m4u_mva_unmap_kernel(tpipePA, map_size, map_va);
    }
}


#if 0
    /*
    1.	Debug port information:

    [30]:ultra-high is enable
    */
        ISP_WR32(ISP_ADDR + 0x160,0x94);
        LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x160, ISP_RD32(ISP_ADDR + 0x160));
        LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x164, ISP_RD32(ISP_ADDR + 0x164));

    /*
    IMGI :
    0x15004160 ] 0x00000011
    Read 0x15004164
    Check bit 21 ->N dmadone_flag
    Check bit [9:7]  -> NFSM (idle =0)
    */
    ISP_WR32(ISP_ADDR + 0x160,0x00000011);
    LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x160, ISP_RD32(ISP_ADDR + 0x160));
    LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x164, ISP_RD32(ISP_ADDR + 0x164));
    /*
    IMGCI :
    0x15004160 ] 0x00000021
    Read 0x15004164
    Check bit 21 ->N dmadone_flag
    Check bit [9:7]  -> NFSM (idle =0)
    */
    ISP_WR32(ISP_ADDR + 0x160,0x00000021);
    LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x160, ISP_RD32(ISP_ADDR + 0x160));
    LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x164, ISP_RD32(ISP_ADDR + 0x164));
    /*
    LSCI :
    0x15004160 ] 0x00000031
    Read 0x15004164
    Check bit 21 ->N dmadone_flag
    Check bit [9:7]  -> NFSM (idle =0)
    */
    ISP_WR32(ISP_ADDR + 0x160,0x00000031);
    LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x160, ISP_RD32(ISP_ADDR + 0x160));
    LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x164, ISP_RD32(ISP_ADDR + 0x164));

    /*
    FLKI :
    0x15004160 ] 0x00000051
    Read 0x15004164
    Check bit 21 ->N dmadone_flag
    Check bit [9:7]  -> NFSM (idle =0)
    */
    ISP_WR32(ISP_ADDR + 0x160,0x00000051);
    LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x160, ISP_RD32(ISP_ADDR + 0x160));
    LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x164, ISP_RD32(ISP_ADDR + 0x164));

    /*
    LCEI :
    0x15004160 ] 0x00000061
    Read 0x15004164
    Check bit 21 ->N dmadone_flag
    Check bit [9:7]  -> NFSM (idle =0)
    */
    ISP_WR32(ISP_ADDR + 0x160,0x00000061);
    LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x160, ISP_RD32(ISP_ADDR + 0x160));
    LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x164, ISP_RD32(ISP_ADDR + 0x164));
    /*
    VIPI :
    0x15004160 ] 0x00000071
    Read 0x15004164
    Check bit 21 ->N dmadone_flag
    Check bit [9:7]  -> NFSM (idle =0)
    */
    ISP_WR32(ISP_ADDR + 0x160,0x00000071);
    LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x160, ISP_RD32(ISP_ADDR + 0x160));
    LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x164, ISP_RD32(ISP_ADDR + 0x164));
    /*
    VIP2I :
    0x15004160 ] 0x00000081
    Read 0x15004164
    Check bit 21 ->N dmadone_flag
    Check bit [9:7]  -> NFSM (idle =0)
    */
    ISP_WR32(ISP_ADDR + 0x160,0x00000081);
    LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x160, ISP_RD32(ISP_ADDR + 0x160));
    LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x164, ISP_RD32(ISP_ADDR + 0x164));
    /*
    IMGO
    0x15004160 ] 0x00000194
    Read 0x15004164
    Check bit [4:2] ->NFSM (idle =0)
    Check bit 23  -> Ndmasent_flag
    Check bit 22 -> N dmadone_flag
    */
    ISP_WR32(ISP_ADDR + 0x160,0x00000194);
    LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x160, ISP_RD32(ISP_ADDR + 0x160));
    LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x164, ISP_RD32(ISP_ADDR + 0x164));
    /*
    IMG2O
    0x15004160 ] 0x000001a4
    Read 0x15004164
    Check bit [4:2] ->NFSM (idle =0)
    Check bit 23  -> Ndmasent_flag
    Check bit 22 -> N dmadone_flag
    */
    ISP_WR32(ISP_ADDR + 0x160,0x000001a4);
    LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x160, ISP_RD32(ISP_ADDR + 0x160));
    LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x164, ISP_RD32(ISP_ADDR + 0x164));

    /*
    LCSO
    0x15004160 ] 0x000001b4
    Read 0x15004164
    Check bit [4:2] ->NFSM (idle =0)
    Check bit 23  -> Ndmasent_flag
    Check bit 22 -> N dmadone_flag
    */
    ISP_WR32(ISP_ADDR + 0x160,0x000001b4);
    LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x160, ISP_RD32(ISP_ADDR + 0x160));
    LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x164, ISP_RD32(ISP_ADDR + 0x164));

    /*
    ESFKO
    0x15004160 ] 0x000001c4
    Read 0x15004164
    Check bit [4:2] ->NFSM (idle =0)
    Check bit 23  -> Ndmasent_flag
    Check bit 22 -> N dmadone_flag
    */
    ISP_WR32(ISP_ADDR + 0x160,0x000001c4);
    LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x160, ISP_RD32(ISP_ADDR + 0x160));
    LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x164, ISP_RD32(ISP_ADDR + 0x164));
    /*
    AAO
    0x15004160 ] 0x000001d4
    Read 0x15004164
    Check bit [4:2] ->NFSM (idle =0)
    Check bit 23  -> Ndmasent_flag
    Check bit 22 -> N dmadone_flag
    */
    ISP_WR32(ISP_ADDR + 0x160,0x000001d4);
    LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x160, ISP_RD32(ISP_ADDR + 0x160));
    LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x164, ISP_RD32(ISP_ADDR + 0x164));

#endif



    //
#endif



    //
    //spin_unlock_irqrestore(&(IspInfo.SpinLock), flags);
    //
    LOG_DBG("- X.");
    //
    return Ret;
}


/*******************************************************************************
*
********************************************************************************/
static void ISP_EnableClock(MBOOL En)
{
    LOG_DBG("- E. En: %d. G_u4EnableClockCount: %d.", En, G_u4EnableClockCount);
    if (En) // Enable clock.
    {
        enable_clock(MT_CG_IMAGE_CAM_SMI, "CAMERA");
        enable_clock(MT_CG_IMAGE_CAM_CAM, "CAMERA");
        enable_clock(MT_CG_IMAGE_SEN_TG,  "CAMERA");
        enable_clock(MT_CG_IMAGE_SEN_CAM, "CAMERA");
        enable_clock(MT_CG_IMAGE_LARB3_SMI, "CAMERA");
        enable_clock(MT_CG_IMAGE_LARB4_SMI, "CAMERA");
        enable_clock(MT_CG_IMAGE_COMMN_SMI, "CAMERA");
        G_u4EnableClockCount++;
        LOG_DBG("Camera clock enbled. G_u4EnableClockCount: %d.", G_u4EnableClockCount);
    }
    else    // Disable clock.
    {
        disable_clock(MT_CG_IMAGE_CAM_SMI, "CAMERA");
        disable_clock(MT_CG_IMAGE_CAM_CAM, "CAMERA");
        disable_clock(MT_CG_IMAGE_SEN_TG,  "CAMERA");
        disable_clock(MT_CG_IMAGE_SEN_CAM, "CAMERA");
        disable_clock(MT_CG_IMAGE_LARB3_SMI, "CAMERA");
        disable_clock(MT_CG_IMAGE_LARB4_SMI, "CAMERA");
        disable_clock(MT_CG_IMAGE_COMMN_SMI, "CAMERA");
        G_u4EnableClockCount--;
        LOG_DBG("Camera clock disabled. G_u4EnableClockCount: %d.", G_u4EnableClockCount);
    }

    LOG_DBG("- X.");
}

/*******************************************************************************
*
********************************************************************************/
static inline void ISP_Reset(void)
{
    // ensure the view finder is disabe. 0: take_picture
    //ISP_CLR_BIT(ISP_REG_ADDR_EN1, 0);
    MUINT32 i, Reg;
    MUINT32 flags;
    //
    LOG_DBG("- E.");
    //TODO: MUST remove later
    //imgsys clk on
    //ISP_WR32(ISP_ADDR_CAMINF, 0);
//    ISP_EnableClock(MTRUE);
    LOG_DBG("isp gate clk(0x%x)",ISP_RD32(ISP_ADDR_CAMINF));
    //imgsys IMG_SW_RST -> reset HW register
    /*
    1500000C	IMG_SW_RST
        17	16	SENINF	SENINF	R/W	2'b00	"SENINF clock domain hw reset
                bit 0 : cam_tg hw domain hw reset,
                bit 1 : (TBD)
                0 : no hw reset
                1 : hw reset"

        14	12	CAM_RST	CAM_RST	R/W	3'b000	" CAM W reset
                bit 0 : fsmi domain HW reset
                bit 1 : fmem domain HW reset
                bit 2 : faxi(cpu bus) dom HW reset
                0 : no hw reset
                1 : hw reset"
    */
//    Reg = ISP_RD32(ISP_ADDR_CAMINF+0x0C);
//    ISP_WR32(ISP_ADDR_CAMINF+0x0C, (Reg|0x0003F000));
//    mdelay(5);
//    ISP_WR32(ISP_ADDR_CAMINF+0x0C, (Reg&(~0x0003F000)) );

//js_test, remove later once IMGSYS is working. Now it's NOT working on FPGA.
for(i=0; i<=0x54; i+=4)
{
ISP_WR32(ISP_ADDR + i, 0x00 );
}


    //bandwidth limitor for TG
    Reg = ISP_RD32((EMI_BASE+0x120));
    Reg |= 0x3F;
    ISP_WR32((EMI_BASE+0x120), Reg);
    //
    ISP_WR32(ISP_REG_ADDR_SW_CTL, ISP_REG_SW_CTL_SW_RST_TRIG);
	/*
    while(1)
    {
        Reg = ISP_RD32(ISP_REG_ADDR_SW_CTL);
        if(Reg & ISP_REG_SW_CTL_SW_RST_STATUS)
        {
            break;
        }
    }
	*/
	do {
	    Reg = ISP_RD32(ISP_REG_ADDR_SW_CTL);
	} while ((!Reg) & ISP_REG_SW_CTL_SW_RST_STATUS);
    //
    ISP_WR32(ISP_REG_ADDR_SW_CTL, ISP_REG_SW_CTL_SW_RST_TRIG|ISP_REG_SW_CTL_HW_RST); //0x5
    ISP_WR32(ISP_REG_ADDR_SW_CTL, ISP_REG_SW_CTL_HW_RST); //0x4
    ISP_WR32(ISP_REG_ADDR_SW_CTL, 0);
    //
    spin_lock_irqsave(&(IspInfo.SpinLockIrq), flags);
    for(i=0; i<ISP_IRQ_TYPE_AMOUNT; i++)
    {
        IspInfo.IrqInfo.Status[i] = 0;
    }
    spin_unlock_irqrestore(&(IspInfo.SpinLockIrq), flags);
//
TempAddr = 0;



//js_test
#if 0 //TODO:test code for SMI bandwidth control , remove later
    //larb_clock_on();
    //SMI_PowerOn();
#define SMI_LARB_NUMBER 5
        for (i=0; i<SMI_LARB_NUMBER; i++) {
                larb_clock_on(i);
        }
        LOG_DBG("+LARB in reset,BWL(0x%08X)/(0x%08X)/(0x%08X)/(0x%08X),220(0x%08X)/(0x%08X),0x14(0x%08X)/(0x%08X)/(0x%08X)/(0x%08X)/(0x%08X)", \
            ISP_RD32(0xF0202000+0x204), \
            ISP_RD32(0xF0202000+0x20c), \
            ISP_RD32(0xF0202000+0x210), \
            ISP_RD32(0xF0202000+0x214), \
            ISP_RD32(0xF0202000+0x220), \
            ISP_RD32(0xF0202000+0x230), \
            ISP_RD32(SMI_LARB0+0x10), \
            ISP_RD32(SMI_LARB1+0x10), \
            ISP_RD32(SMI_LARB2+0x10), \
            ISP_RD32(SMI_LARB3+0x10), \
            ISP_RD32(SMI_LARB4+0x10));

        //BW limit:
        //SMI_COMMON_APB_BASE+0x204=0xb92  //larb0 venc
        //SMI_COMMON_APB_BASE+0x20c=0xa4b  //larb2 disp
        //SMI_COMMON_APB_BASE+0x210=0x96d  //larb3 cdp
        //SMI_COMMON_APB_BASE+0x214=0x9a7  //larb4 isp
        ISP_WR32(0xF0202000+0x204,0xb92);
        ISP_WR32(0xF0202000+0x20c,0xa4b);
        ISP_WR32(0xF0202000+0x210,0x96d);
        ISP_WR32(0xF0202000+0x214,0xfff);//pass1


        //Allocate larb->2 smi common
        //LARB2&3 use m4u#0, else use m4u#1
        //SMI_COMMON_APB_BASE+0x220=0x1505
        ISP_WR32(0xF0202000+0x220,0x1505);

        //SMI COMMON reduce command buffer
        //ISP_WR32(SMI_COMMON_APB_BASE+0x230,0x1560);
        ISP_WR32(0xF0202000+0x230,0x1560);

        //SMI LARB reduce command buffer
        ISP_WR32(SMI_LARB0+0x14,0x400420);
        ISP_WR32(SMI_LARB1+0x14,0x400420);
        ISP_WR32(SMI_LARB2+0x14,0x400420);
        ISP_WR32(SMI_LARB3+0x14,0x400420);//pass2
        //ISP_WR32(SMI_LARB4+0x14,0x400420);//pass1

        LOG_DBG("-LARB in reset,BWL(0x%08X)/(0x%08X)/(0x%08X)/(0x%08X),220(0x%08X)/(0x%08X),0x14(0x%08X)/(0x%08X)/(0x%08X)/(0x%08X)/(0x%08X)", \
            ISP_RD32(0xF0202000+0x204), \
            ISP_RD32(0xF0202000+0x20c), \
            ISP_RD32(0xF0202000+0x210), \
            ISP_RD32(0xF0202000+0x214), \
            ISP_RD32(0xF0202000+0x220), \
            ISP_RD32(0xF0202000+0x230), \
            ISP_RD32(SMI_LARB0+0x10), \
            ISP_RD32(SMI_LARB1+0x10), \
            ISP_RD32(SMI_LARB2+0x10), \
            ISP_RD32(SMI_LARB3+0x10), \
            ISP_RD32(SMI_LARB4+0x10));

        for (i=0; i<SMI_LARB_NUMBER; i++) {
                larb_clock_off(i);
        }
#endif


    //
    LOG_DBG("- X.");
}

/*******************************************************************************
*
********************************************************************************/
static MINT32 ISP_ReadReg(ISP_REG_IO_STRUCT* pRegIo)
{
    MUINT32 i;
    MINT32 Ret = 0;
    //
    ISP_REG_STRUCT reg;
    MUINT32* pData = (MUINT32*)pRegIo->Data;
    for (i = 0; i < pRegIo->Count; i++)
    {
        if  ( 0 != get_user(reg.Addr, pData) )
        {
            LOG_ERR("get_user failed");
            Ret = -EFAULT;
            goto EXIT;
        }
        pData++;
        //
        reg.Val = ISP_RD32(ISP_ADDR_CAMINF + reg.Addr);
        //
        if  ( 0 != put_user(reg.Val, pData) )
        {
            LOG_ERR("put_user failed");
            Ret = -EFAULT;
            goto EXIT;
        }
        pData++;
        //
    }
    //
    EXIT:
    return Ret;
}


/*******************************************************************************
*
********************************************************************************/
static MINT32 ISP_WriteRegToHw(
    ISP_REG_STRUCT* pReg,
    MUINT32         Count)
{
    MINT32 Ret = 0;
    MUINT32 i;
    //
    if(IspInfo.DebugMask & ISP_DBG_WRITE_REG)
    {
        LOG_DBG("- E.");
    }
    //
    spin_lock(&(IspInfo.SpinLockIsp));
    for(i = 0; i < Count; i++)
    {
        if(IspInfo.DebugMask & ISP_DBG_WRITE_REG)
        {
            LOG_DBG("Addr(0x%08X), Val(0x%08X)", (MUINT32)(ISP_ADDR_CAMINF + pReg[i].Addr), (MUINT32)(pReg[i].Val));
        }
        ISP_WR32(ISP_ADDR_CAMINF + pReg[i].Addr, pReg[i].Val);
    }
    spin_unlock(&(IspInfo.SpinLockIsp));
    //
    return Ret;
}

/*******************************************************************************
*
********************************************************************************
static void ISP_BufWrite_Init(void)    //Vent@20121106: Marked to remove build warning: 'ISP_BufWrite_Init' defined but not used [-Wunused-function]
{
    MUINT32 i;
    //
    if(IspInfo.DebugMask & ISP_DBG_BUF_WRITE)
    {
        LOG_DBG("- E.");
    }
    //
    for(i=0; i<ISP_BUF_WRITE_AMOUNT; i++)
    {
        IspInfo.BufInfo.Write[i].Status = ISP_BUF_STATUS_EMPTY;
        IspInfo.BufInfo.Write[i].Size = 0;
        IspInfo.BufInfo.Write[i].pData = NULL;
    }
}

*******************************************************************************
*
********************************************************************************/
static void ISP_BufWrite_Dump(void)
{
    MUINT32 i;
    //
    LOG_DBG("- E.");
    //
    for(i=0; i<ISP_BUF_WRITE_AMOUNT; i++)
    {
        LOG_DBG("i=%d, Status=%d, Size=%d",i,IspInfo.BufInfo.Write[i].Status,IspInfo.BufInfo.Write[i].Size);
        IspInfo.BufInfo.Write[i].Status = ISP_BUF_STATUS_EMPTY;
        IspInfo.BufInfo.Write[i].Size = 0;
        IspInfo.BufInfo.Write[i].pData = NULL;
    }
}


/*******************************************************************************
*
********************************************************************************/
static void ISP_BufWrite_Free(void)
{
    MUINT32 i;
    //
    if(IspInfo.DebugMask & ISP_DBG_BUF_WRITE)
    {
        LOG_DBG("- E.");
    }
    //
    for(i=0; i<ISP_BUF_WRITE_AMOUNT; i++)
    {
        IspInfo.BufInfo.Write[i].Status = ISP_BUF_STATUS_EMPTY;
        IspInfo.BufInfo.Write[i].Size = 0;
        if(IspInfo.BufInfo.Write[i].pData != NULL)
        {
            kfree(IspInfo.BufInfo.Write[i].pData);
            IspInfo.BufInfo.Write[i].pData = NULL;
        }
    }
}

/*******************************************************************************
*
********************************************************************************/
static MBOOL ISP_BufWrite_Alloc(void)
{
    MUINT32 i;
    //
    if(IspInfo.DebugMask & ISP_DBG_BUF_WRITE)
    {
        LOG_DBG("- E.");
    }
    //
    for(i=0; i<ISP_BUF_WRITE_AMOUNT; i++)
    {
        IspInfo.BufInfo.Write[i].Status = ISP_BUF_STATUS_EMPTY;
        IspInfo.BufInfo.Write[i].Size = 0;
        IspInfo.BufInfo.Write[i].pData = (MUINT8*)kmalloc(ISP_BUF_SIZE_WRITE, GFP_ATOMIC);
        if(IspInfo.BufInfo.Write[i].pData == NULL)
        {
            LOG_DBG("ERROR: i = %d, pData is NULL",i);
            ISP_BufWrite_Free();
            return false;
        }
    }
    return true;
}

/*******************************************************************************
*
********************************************************************************/
static void ISP_BufWrite_Reset(void)
{
    MUINT32 i;
    //
    if(IspInfo.DebugMask & ISP_DBG_BUF_WRITE)
    {
        LOG_DBG("- E.");
    }
    //
    for(i=0; i<ISP_BUF_WRITE_AMOUNT; i++)
    {
        IspInfo.BufInfo.Write[i].Status = ISP_BUF_STATUS_EMPTY;
        IspInfo.BufInfo.Write[i].Size = 0;
    }
}

/*******************************************************************************
*
********************************************************************************/
static __inline MUINT32 ISP_BufWrite_GetAmount(void)
{
    MUINT32 i, Count = 0;
    //
    for(i=0; i<ISP_BUF_WRITE_AMOUNT; i++)
    {
        if(IspInfo.BufInfo.Write[i].Status == ISP_BUF_STATUS_READY)
        {
            Count++;
        }
    }
    //
    if(IspInfo.DebugMask & ISP_DBG_BUF_WRITE)
    {
        LOG_DBG("Count = %d",Count);
    }
    return Count;
}

/*******************************************************************************
*
********************************************************************************/
static MBOOL ISP_BufWrite_Add(
    MUINT32     Size,
    MUINT8*     pData)
{
    MUINT32 i;
    //
    //LOG_DBG("- E.");
    //
    for(i=0; i<ISP_BUF_WRITE_AMOUNT; i++)
    {
        if(IspInfo.BufInfo.Write[i].Status == ISP_BUF_STATUS_HOLD)
        {
            if((IspInfo.BufInfo.Write[i].Size+Size) > ISP_BUF_SIZE_WRITE)
            {
                LOG_ERR("i = %d, BufWriteSize(%d)+Size(%d) > %d",i,IspInfo.BufInfo.Write[i].Size,Size,ISP_BUF_SIZE_WRITE);
                return false;
            }
            //
            if(copy_from_user((MUINT8*)(IspInfo.BufInfo.Write[i].pData+IspInfo.BufInfo.Write[i].Size), (MUINT8*)pData, Size) != 0)
            {
                LOG_ERR("copy_from_user failed");
                return false;
            }
            //
            if(IspInfo.DebugMask & ISP_DBG_BUF_WRITE)
            {
                LOG_DBG("i = %d, BufSize = %d, Size = %d",i,IspInfo.BufInfo.Write[i].Size,Size);
            }
            //
            IspInfo.BufInfo.Write[i].Size += Size;
            return true;
        }
    }
    //
    for(i=0; i<ISP_BUF_WRITE_AMOUNT; i++)
    {
        if(IspInfo.BufInfo.Write[i].Status == ISP_BUF_STATUS_EMPTY)
        {
            if(Size > ISP_BUF_SIZE_WRITE)
            {
                LOG_ERR("i = %d, Size(%d) > %d",i,Size,ISP_BUF_SIZE_WRITE);
                return false;
            }
            //
            if(copy_from_user((MUINT8*)(IspInfo.BufInfo.Write[i].pData), (MUINT8*)pData, Size) != 0)
            {
                LOG_ERR("copy_from_user failed");
                return false;
            }
            //
            if(IspInfo.DebugMask & ISP_DBG_BUF_WRITE)
            {
                LOG_DBG("i = %d, Size = %d",i,Size);
            }
            //
            IspInfo.BufInfo.Write[i].Size = Size;
            //
            IspInfo.BufInfo.Write[i].Status = ISP_BUF_STATUS_HOLD;
            return true;
        }
    }

    //
    LOG_ERR("All write buffer are full of data!");
    return false;

}
/*******************************************************************************
*
********************************************************************************/
static void ISP_BufWrite_SetReady(void)
{
    MUINT32 i;
    //
    //LOG_DBG("- E.");
    //
    for(i=0; i<ISP_BUF_WRITE_AMOUNT; i++)
    {
        if(IspInfo.BufInfo.Write[i].Status == ISP_BUF_STATUS_HOLD)
        {
            if(IspInfo.DebugMask & ISP_DBG_BUF_WRITE)
            {
                LOG_DBG("i = %d, Size = %d",i,IspInfo.BufInfo.Write[i].Size);
            }
            IspInfo.BufInfo.Write[i].Status = ISP_BUF_STATUS_READY;
        }
    }
}

/*******************************************************************************
*
********************************************************************************/
static MBOOL ISP_BufWrite_Get(
    MUINT32*    pIndex,
    MUINT32*    pSize,
    MUINT8**    ppData)
{
    MUINT32 i;
    //
    //LOG_DBG("- E.");
    //
    for(i=0; i<ISP_BUF_WRITE_AMOUNT; i++)
    {
        if(IspInfo.BufInfo.Write[i].Status == ISP_BUF_STATUS_READY)
        {
            if(IspInfo.DebugMask & ISP_DBG_BUF_WRITE)
            {
                LOG_DBG("i = %d, Size = %d",i,IspInfo.BufInfo.Write[i].Size);
            }
            *pIndex = i;
            *pSize = IspInfo.BufInfo.Write[i].Size;
            *ppData = IspInfo.BufInfo.Write[i].pData;
            return true;
        }
    }
    //
    if(IspInfo.DebugMask & ISP_DBG_BUF_WRITE)
    {
        LOG_DBG("No buf is ready!");
    }
    return false;
}

/*******************************************************************************
*
********************************************************************************/
static MBOOL ISP_BufWrite_Clear(MUINT32  Index)
{
    //
    //LOG_DBG("- E.");
    //
    if(IspInfo.BufInfo.Write[Index].Status == ISP_BUF_STATUS_READY)
    {
        if(IspInfo.DebugMask & ISP_DBG_BUF_WRITE)
        {
            LOG_DBG("Index = %d, Size = %d",Index,IspInfo.BufInfo.Write[Index].Size);
        }
        IspInfo.BufInfo.Write[Index].Size = 0;
        IspInfo.BufInfo.Write[Index].Status = ISP_BUF_STATUS_EMPTY;
        return true;
    }
    else
    {
        LOG_DBG("WARNING: Index(%d) is not ready! Status = %d",Index,IspInfo.BufInfo.Write[Index].Status);
        return false;
    }
}

/*******************************************************************************
*
********************************************************************************/
static void ISP_BufWrite_WriteToHw(void)
{
    MUINT8* pBuf;
    MUINT32 Index, BufSize;
    //
    spin_lock(&(IspInfo.SpinLockHold));
    //
    LOG_DBG("- E.");
    //
    while(ISP_BufWrite_Get(&Index,&BufSize,&pBuf))
    {
        if(IspInfo.DebugMask & ISP_DBG_TASKLET)
        {
            LOG_DBG("Index = %d, BufSize = %d ", Index, BufSize);
        }
        ISP_WriteRegToHw((ISP_REG_STRUCT*)pBuf, BufSize/sizeof(ISP_REG_STRUCT));
        ISP_BufWrite_Clear(Index);
    }
    //LOG_DBG("No more buf.");
    atomic_set(&(IspInfo.HoldInfo.WriteEnable), 0);
    wake_up_interruptible(&(IspInfo.WaitQueueHead));
    //
    spin_unlock(&(IspInfo.SpinLockHold));
}


/*******************************************************************************
*
********************************************************************************/
void ISP_ScheduleWork_VD(struct work_struct *data)
{
    if(IspInfo.DebugMask & ISP_DBG_SCHEDULE_WORK)
    {
        LOG_DBG("- E.");
    }
    //
    IspInfo.TimeLog.WorkQueueVd = ISP_JiffiesToMs(jiffies);
    //
    if(IspInfo.Callback[ISP_CALLBACK_WORKQUEUE_VD].Func != NULL)
    {
        IspInfo.Callback[ISP_CALLBACK_WORKQUEUE_VD].Func();
    }
}

/*******************************************************************************
*
********************************************************************************/
void ISP_ScheduleWork_EXPDONE(struct work_struct *data)
{
    if(IspInfo.DebugMask & ISP_DBG_SCHEDULE_WORK)
    {
        LOG_DBG("- E.");
    }
    //
    IspInfo.TimeLog.WorkQueueExpdone = ISP_JiffiesToMs(jiffies);
    //
    if(IspInfo.Callback[ISP_CALLBACK_WORKQUEUE_EXPDONE].Func != NULL)
    {
        IspInfo.Callback[ISP_CALLBACK_WORKQUEUE_EXPDONE].Func();
    }
}

/*******************************************************************************
*
********************************************************************************/
void ISP_Tasklet_VD(unsigned long Param)
{
    if(IspInfo.DebugMask & ISP_DBG_TASKLET)
    {
        LOG_DBG("- E.");
    }
    //
    IspInfo.TimeLog.TaskletVd = ISP_JiffiesToMs(jiffies);
    //
    if(IspInfo.Callback[ISP_CALLBACK_TASKLET_VD].Func != NULL)
    {
        IspInfo.Callback[ISP_CALLBACK_TASKLET_VD].Func();
    }
    //
    if(IspInfo.HoldInfo.Time == ISP_HOLD_TIME_VD)
    {
        ISP_BufWrite_WriteToHw();
    }
}
DECLARE_TASKLET(IspTaskletVD, ISP_Tasklet_VD, 0);

/*******************************************************************************
*
********************************************************************************/
void ISP_Tasklet_EXPDONE(unsigned long Param)
{
    if(IspInfo.DebugMask & ISP_DBG_TASKLET)
    {
        LOG_DBG("- E.");
    }
    //
    IspInfo.TimeLog.TaskletExpdone = ISP_JiffiesToMs(jiffies);
    //
    if(IspInfo.Callback[ISP_CALLBACK_TASKLET_EXPDONE].Func != NULL)
    {
        IspInfo.Callback[ISP_CALLBACK_TASKLET_EXPDONE].Func();
    }
    //
    if(IspInfo.HoldInfo.Time == ISP_HOLD_TIME_EXPDONE)
    {
        ISP_BufWrite_WriteToHw();
    }
}
DECLARE_TASKLET(IspTaskletEXPDONE, ISP_Tasklet_EXPDONE, 0);


/*******************************************************************************
*
********************************************************************************/
static MINT32 ISP_WriteReg(ISP_REG_IO_STRUCT*   pRegIo)
{
    MINT32 Ret = 0;
    MINT32 TimeVd = 0;
    MINT32 TimeExpdone = 0;
    MINT32 TimeTasklet = 0;
    MUINT8* pData = NULL;
    //
    if(IspInfo.DebugMask & ISP_DBG_WRITE_REG)
    {
        LOG_DBG("Data(0x%08X), Count(%d)", (MUINT32)(pRegIo->Data), (MUINT32)(pRegIo->Count));
    }
    //
    if(atomic_read(&(IspInfo.HoldInfo.HoldEnable)))
    {
        if(ISP_BufWrite_Add((pRegIo->Count)*sizeof(ISP_REG_STRUCT), (MUINT8*)(pRegIo->Data)))
        {
            //LOG_DBG("Add write buffer OK");
        }
        else
        {
            LOG_ERR("Add write buffer fail");
            TimeVd = ISP_JiffiesToMs(jiffies)-IspInfo.TimeLog.Vd;
            TimeExpdone = ISP_JiffiesToMs(jiffies)-IspInfo.TimeLog.Expdone;
            TimeTasklet = ISP_JiffiesToMs(jiffies)-IspInfo.TimeLog.TaskletExpdone;
            LOG_ERR("HoldTime(%d), VD(%d ms), Expdone(%d ms), Tasklet(%d ms)",IspInfo.HoldInfo.Time,TimeVd,TimeExpdone,TimeTasklet);
            ISP_BufWrite_Dump();
            ISP_DumpReg();
            Ret = -EFAULT;
            goto EXIT;
        }
    }
    else
    {
        pData = (MUINT8*)kmalloc((pRegIo->Count)*sizeof(ISP_REG_STRUCT), GFP_ATOMIC);
        if(pData == NULL)
        {
            LOG_DBG("ERROR: kmalloc failed, (process, pid, tgid)=(%s, %d, %d)", current->comm, current->pid, current->tgid);
            Ret = -ENOMEM;
        }
        if(copy_from_user(pData, (MUINT8*)(pRegIo->Data), pRegIo->Count*sizeof(ISP_REG_STRUCT)) != 0)
        {
            LOG_ERR("copy_from_user failed");
            Ret = -EFAULT;
            goto EXIT;
        }
        //
        Ret = ISP_WriteRegToHw(
                (ISP_REG_STRUCT*)pData,
                pRegIo->Count);
    }
    //
    EXIT:
    if(pData != NULL)
    {
        kfree(pData);
        pData = NULL;
    }
    return Ret;
}

/*******************************************************************************
*
********************************************************************************/
static MINT32 ISP_SetHoldTime(ISP_HOLD_TIME_ENUM HoldTime)
{
    LOG_DBG("HoldTime(%d)", HoldTime);
    IspInfo.HoldInfo.Time = HoldTime;
    //
    return 0;
}

/*******************************************************************************
*
********************************************************************************/
static MINT32 ISP_ResetBuf(void)
{
    LOG_DBG("- E. hold_reg(%d), BufAmount(%d)", atomic_read(&(IspInfo.HoldInfo.HoldEnable)), ISP_BufWrite_GetAmount());
    //
    ISP_BufWrite_Reset();
    atomic_set(&(IspInfo.HoldInfo.HoldEnable), 0);
    atomic_set(&(IspInfo.HoldInfo.WriteEnable), 0);
    LOG_DBG("- X.");
    return 0;
}


/*******************************************************************************
*
********************************************************************************/
static MINT32 ISP_EnableHoldReg(MBOOL En)
{
    MINT32 Ret = 0;
    MUINT32 BufAmount = 0;
    //
    if(IspInfo.DebugMask & ISP_DBG_HOLD_REG)
    {
        LOG_DBG("En(%d), HoldEnable(%d)", En, atomic_read(&(IspInfo.HoldInfo.HoldEnable)));
    }
    //
    if(!spin_trylock_bh(&(IspInfo.SpinLockHold)))
    {
        //  Should wait until tasklet done.
        MINT32 Timeout;
        MINT32 IsLock = 0;
        //
        if(IspInfo.DebugMask & ISP_DBG_TASKLET)
        {
            LOG_DBG("Start wait ... ");
        }
        //
        Timeout = wait_event_interruptible_timeout(
                    IspInfo.WaitQueueHead,
                    (IsLock = spin_trylock_bh(&(IspInfo.SpinLockHold))),
                    ISP_MsToJiffies(500));
        //
        if(IspInfo.DebugMask & ISP_DBG_TASKLET)
        {
            LOG_DBG("End wait ");
        }
        //
        if(IsLock == 0)
        {
            LOG_ERR("Should not happen, Timeout & IsLock is 0");
            Ret = -EFAULT;
            goto EXIT;
        }
    }
    //  Here we get the lock.
    if(En == MFALSE)
    {
        ISP_BufWrite_SetReady();
        BufAmount = ISP_BufWrite_GetAmount();
        //
        if(BufAmount)
        {
            atomic_set(&(IspInfo.HoldInfo.WriteEnable), 1);
        }
    }
    //
    if(IspInfo.DebugMask & ISP_DBG_HOLD_REG)
    {
        LOG_DBG("En(%d), HoldEnable(%d), BufAmount(%d)", En, atomic_read(&(IspInfo.HoldInfo.HoldEnable)),BufAmount);
    }
    //
    atomic_set(&(IspInfo.HoldInfo.HoldEnable), En);
    //
    spin_unlock_bh(&(IspInfo.SpinLockHold));
    //
    EXIT:
    return Ret;
}
/*******************************************************************************
*
********************************************************************************/
static atomic_t g_imem_ref_cnt[ISP_REF_CNT_ID_MAX];
//
static long ISP_REF_CNT_CTRL_FUNC(MUINT32 Param)
{
    MINT32 Ret = 0;
    ISP_REF_CNT_CTRL_STRUCT ref_cnt_ctrl;
    MINT32 imem_ref_cnt = 0;

LOG_DBG("- E.");

	////////////////////---add lock here
//	spin_lock_irq(&(IspInfo.SpinLock));
	////////////////////
    //
    if(IspInfo.DebugMask & ISP_DBG_REF_CNT_CTRL) {
        LOG_DBG("[rc]+");
    }
    //
    if (NULL == (void*)Param)  {
        LOG_ERR("[rc]NULL Param");
		////////////////////---add unlock here
//	  	spin_unlock_irqrestore(&(IspInfo.SpinLock), flags);
	    ////////////////////
        return -EFAULT;
    }
    //
    if(copy_from_user(&ref_cnt_ctrl, (void*)Param, sizeof(ISP_REF_CNT_CTRL_STRUCT)) == 0)
    {


        if(IspInfo.DebugMask & ISP_DBG_REF_CNT_CTRL) {
            LOG_DBG("[rc]ctrl(%d),id(%d)",ref_cnt_ctrl.ctrl,ref_cnt_ctrl.id);
        }
        //
        if ( ISP_REF_CNT_ID_MAX > ref_cnt_ctrl.id ) {
            ////////////////////---add lock here
            spin_lock(&(IspInfo.SpinLockIspRef));
            ////////////////////
            //
            switch(ref_cnt_ctrl.ctrl) {
                case ISP_REF_CNT_GET:
                    break;
                case ISP_REF_CNT_INC:
                    atomic_inc(&g_imem_ref_cnt[ref_cnt_ctrl.id]);
                    //g_imem_ref_cnt++;
                    break;
                case ISP_REF_CNT_DEC:
                case ISP_REF_CNT_DEC_AND_RESET_IF_LAST_ONE:
                    atomic_dec(&g_imem_ref_cnt[ref_cnt_ctrl.id]);
                    //g_imem_ref_cnt--;
                    break;
                default:
                case ISP_REF_CNT_MAX:   // Add this to remove build warning.
                    // Do nothing.
                    break;
            }
            //
            imem_ref_cnt = (MINT32)atomic_read(&g_imem_ref_cnt[ref_cnt_ctrl.id]);
            LOG_DBG("g_imem_ref_cnt[%d]: %d.", ref_cnt_ctrl.id, imem_ref_cnt);

            if ( (imem_ref_cnt == 0) && (ref_cnt_ctrl.ctrl == ISP_REF_CNT_DEC_AND_RESET_IF_LAST_ONE) )   // No user left and ctrl is RESET_IF_LAST_ONE, do ISP reset.
            {
                ISP_Reset();
                LOG_DBG("ISP_REF_CNT_DEC_AND_RESET_IF_LAST_ONE. Do ISP_Reset.");
            }
            ////////////////////---add unlock here
        	spin_unlock(&(IspInfo.SpinLockIspRef));
        	////////////////////

            //
            if(IspInfo.DebugMask & ISP_DBG_REF_CNT_CTRL) {
                LOG_DBG("[rc]ref_cnt(%d)",imem_ref_cnt);
            }
            //
            if(copy_to_user((void*)ref_cnt_ctrl.data_ptr, &imem_ref_cnt, sizeof(MINT32)) != 0)
            {
                LOG_ERR("[rc][GET]:copy_to_user failed");
                Ret = -EFAULT;
            }
        }
        else {
            LOG_ERR("[rc]:id(%d) exceed",ref_cnt_ctrl.id);
            Ret = -EFAULT;
        }


    }
    else
    {
        LOG_ERR("[rc]copy_from_user failed");
        Ret = -EFAULT;
    }

    //
    if(IspInfo.DebugMask & ISP_DBG_REF_CNT_CTRL) {
        LOG_DBG("[rc]-");
    }
    //
    ////////////////////---add unlock here
//	spin_unlock_irqrestore(&(IspInfo.SpinLock), flags);
	////////////////////
LOG_DBG("- X. Ret: %d.", Ret);
    return Ret;
}
/*******************************************************************************
*
********************************************************************************/

//js_test
//
#ifndef _rtbc_use_cq0c_
static unsigned int aaa = 0;
static MUINT32  bEnqBuf = 0;
static MUINT32  bDeqBuf = 0;
static MINT32 rtbc_enq_dma = _rt_dma_max_;
static MINT32 rtbc_deq_dma = _rt_dma_max_;
#endif

static ISP_RT_BUF_INFO_STRUCT       rt_buf_info;
static ISP_RT_BUF_INFO_STRUCT       ex_rt_buf_info;
static ISP_DEQUE_BUF_INFO_STRUCT    deque_buf;

static MUINT32 prv_tstamp_s = 0;
static MUINT32 prv_tstamp_us = 0;


static MUINT32 sof_count = 0;
static MUINT32 start_time = 0;
static MUINT32 avg_frame_time = 0;

//allan
static int sof_pass1done = 0;

//
static MINT32 ISP_RTBC_ENQUE(MINT32 dma)
{
    MINT32 Ret = 0;
    MINT32 rt_dma = dma;
    MUINT32 buffer_exist = 0;
    MUINT32 i = 0;
    MUINT32 index = 0;

    //check max
    if ( ISP_RT_BUF_SIZE == pstRTBuf->ring_buf[rt_dma].total_count ) {
        LOG_ERR("[rtbc][ENQUE]:real time buffer number FULL:rt_dma(%d)",rt_dma);
        Ret = -EFAULT;
        //break;
    }

    //
    //spin_lock_irqsave(&(IspInfo.SpinLockRTBC),g_Flash_SpinLock);
    //check if buffer exist
    for (i=0;i<ISP_RT_BUF_SIZE;i++) {
        if ( pstRTBuf->ring_buf[rt_dma].data[i].base_pAddr == rt_buf_info.base_pAddr ) {
            buffer_exist = 1;
            break;
        }
        //
        if ( pstRTBuf->ring_buf[rt_dma].data[i].base_pAddr == 0 ) {
            break;
        }
    }
    //
    if (buffer_exist) {
        //
        if ( ISP_RTBC_BUF_EMPTY != pstRTBuf->ring_buf[rt_dma].data[i].bFilled ) {
            pstRTBuf->ring_buf[rt_dma].data[i].bFilled    = ISP_RTBC_BUF_EMPTY;
            pstRTBuf->ring_buf[rt_dma].empty_count++;
            index = i;
        }
        //
        //if(IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
            LOG_DBG("[rtbc][ENQUE]::buffer_exist(%d)/i(%d)/PA(0x%x)/bFilled(%d)/empty(%d)",
                buffer_exist,
                i,
                rt_buf_info.base_pAddr,
                pstRTBuf->ring_buf[rt_dma].data[i].bFilled, \
                pstRTBuf->ring_buf[rt_dma].empty_count);
        //}

    }
    else {
        //overwrite oldest element if buffer is full
        if ( pstRTBuf->ring_buf[rt_dma].total_count == ISP_RT_BUF_SIZE) {
            LOG_ERR("[ENQUE]:[rtbc]:buffer full(%d)",pstRTBuf->ring_buf[rt_dma].total_count);
        }
        else {
            //first time add
            index = pstRTBuf->ring_buf[rt_dma].total_count % ISP_RT_BUF_SIZE;
            //
            pstRTBuf->ring_buf[rt_dma].data[index].memID      = rt_buf_info.memID;
            pstRTBuf->ring_buf[rt_dma].data[index].size       = rt_buf_info.size;
            pstRTBuf->ring_buf[rt_dma].data[index].base_vAddr = rt_buf_info.base_vAddr;
            pstRTBuf->ring_buf[rt_dma].data[index].base_pAddr = rt_buf_info.base_pAddr;
            pstRTBuf->ring_buf[rt_dma].data[index].bFilled    = ISP_RTBC_BUF_EMPTY;
            //
            pstRTBuf->ring_buf[rt_dma].total_count++;
            pstRTBuf->ring_buf[rt_dma].empty_count++;
            //
            //if(IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
                LOG_DBG("[rtbc][ENQUE]:dma(%d),index(%d),PA(0x%x)/empty(%d)/total(%d)",
                    rt_dma, \
                    index, \
                    pstRTBuf->ring_buf[rt_dma].data[index].base_pAddr, \
                    pstRTBuf->ring_buf[rt_dma].empty_count, \
                    pstRTBuf->ring_buf[rt_dma].total_count);
            //}
        }
    }
    //

    //count ==1 means DMA stalled already or NOT start yet
    if ( 1 == pstRTBuf->ring_buf[rt_dma].empty_count) {
        if (_imgo_ == rt_dma) {
            //set base_addr at beginning before VF_EN
            ISP_WR32(ISP_REG_ADDR_IMGO_BASE_ADDR,pstRTBuf->ring_buf[rt_dma].data[index].base_pAddr);
        }
        else {
            //set base_addr at beginning before VF_EN
            ISP_WR32(ISP_REG_ADDR_IMG2O_BASE_ADDR,pstRTBuf->ring_buf[rt_dma].data[index].base_pAddr);
        }
        //if(IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
            LOG_DBG("[rtbc][ENQUE]:dma(%d),base_pAddr(0x%x)/imgo(0x%x)/img2o(0x%x)/empty_count(%d) ",
                rt_dma, \
                pstRTBuf->ring_buf[rt_dma].data[index].base_pAddr,\
                ISP_RD32(ISP_REG_ADDR_IMGO_BASE_ADDR), \
                ISP_RD32(ISP_REG_ADDR_IMG2O_BASE_ADDR), \
                pstRTBuf->ring_buf[rt_dma].empty_count);
        //}

#if defined(_rtbc_use_cq0c_)
    //Do nothing
#else
        MUINT32 reg_val = 0;

        //disable FBC control to go on download
        if (_imgo_ == rt_dma) {
            reg_val = ISP_RD32(ISP_REG_ADDR_IMGO_FBC);
            reg_val &= ~0x4000;
            ISP_WR32(ISP_REG_ADDR_IMGO_FBC,reg_val);
        }
        else {
            reg_val = ISP_RD32(ISP_REG_ADDR_IMG2O_FBC);
            reg_val &= ~0x4000;
            ISP_WR32(ISP_REG_ADDR_IMG2O_FBC,reg_val);
        }
        if(IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
            LOG_DBG("[rtbc][ENQUE]:dma(%d),disable fbc:IMGO(0x%x),IMG2O(0x%x)",rt_dma,ISP_RD32(ISP_REG_ADDR_IMGO_FBC),ISP_RD32(ISP_REG_ADDR_IMG2O_FBC));
        }
#endif
        pstRTBuf->ring_buf[rt_dma].pre_empty_count = pstRTBuf->ring_buf[rt_dma].empty_count;

    }

    //
    //spin_unlock_irqrestore(&(IspInfo.SpinLockRTBC),g_Flash_SpinLock);
    //
    if(IspInfo.DebugMask & ISP_DBG_INT) {
        LOG_DBG("[rtbc][ENQUE]:dma:(%d),start(%d),index(%d),empty_count(%d),base_pAddr(0x%x)", \
            rt_dma,
            pstRTBuf->ring_buf[rt_dma].start,
            index,
            pstRTBuf->ring_buf[rt_dma].empty_count,
            pstRTBuf->ring_buf[rt_dma].data[index].base_pAddr);
    }
    //
    return Ret;
}

static MINT32 ISP_RTBC_DEQUE(MINT32 dma)
{
    MINT32 Ret = 0;
    MINT32 rt_dma = dma;
    MUINT32 i=0;
    MUINT32 index = 0;

    //spin_lock_irqsave(&(IspInfo.SpinLockRTBC),g_Flash_SpinLock);
    //
    if(IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
        LOG_DBG("[rtbc][DEQUE]+");
    }
    //
    deque_buf.count = 0;
    //in SOF, "start" is next buffer index
    for ( i=0 ; i < pstRTBuf->ring_buf[rt_dma].total_count ; i++ ) {
        //
        index = ( pstRTBuf->ring_buf[rt_dma].start + i ) % pstRTBuf->ring_buf[rt_dma].total_count;
        //
        if ( ISP_RTBC_BUF_FILLED == pstRTBuf->ring_buf[rt_dma].data[index].bFilled ) {
            pstRTBuf->ring_buf[rt_dma].data[index].bFilled = ISP_RTBC_BUF_LOCKED;
            deque_buf.count= 1;
            break;
        }
    }
    //
    if (0==deque_buf.count){
        //queue buffer status
        LOG_DBG("[rtbc][DEQUE]:dma(%d),start(%d),total(%d),empty(%d), deque_buf.count(%d) ", \
            rt_dma, \
            pstRTBuf->ring_buf[rt_dma].start, \
            pstRTBuf->ring_buf[rt_dma].total_count, \
            pstRTBuf->ring_buf[rt_dma].empty_count, \
            deque_buf.count);
        //
        for ( i=0;i<=pstRTBuf->ring_buf[rt_dma].total_count-1;i++ ) {
            LOG_DBG("[rtbc][DEQUE]Buf List:%d/%d/0x%x/0x%x/0x%x/%d/  ", \
                i, \
                pstRTBuf->ring_buf[rt_dma].data[i].memID, \
                pstRTBuf->ring_buf[rt_dma].data[i].size, \
                pstRTBuf->ring_buf[rt_dma].data[i].base_vAddr, \
                pstRTBuf->ring_buf[rt_dma].data[i].base_pAddr, \
                pstRTBuf->ring_buf[rt_dma].data[i].bFilled);
        }
    }
    //
    if ( deque_buf.count ) {
        //Fill buffer head
        //"start" is current working index
        if(IspInfo.DebugMask & ISP_DBG_INT) {
            LOG_DBG("[rtbc][DEQUE]:rt_dma(%d)/index(%d)/empty(%d)/total(%d)", \
                rt_dma, \
                index, \
                pstRTBuf->ring_buf[rt_dma].empty_count, \
                pstRTBuf->ring_buf[rt_dma].total_count);
        }
        //
        for (i=0;i<deque_buf.count;i++) {
            deque_buf.data[i].memID         = pstRTBuf->ring_buf[rt_dma].data[index+i].memID;
            deque_buf.data[i].size          = pstRTBuf->ring_buf[rt_dma].data[index+i].size;
            deque_buf.data[i].base_vAddr    = pstRTBuf->ring_buf[rt_dma].data[index+i].base_vAddr;
            deque_buf.data[i].base_pAddr    = pstRTBuf->ring_buf[rt_dma].data[index+i].base_pAddr;
            deque_buf.data[i].timeStampS    = pstRTBuf->ring_buf[rt_dma].data[index+i].timeStampS;
            deque_buf.data[i].timeStampUs   = pstRTBuf->ring_buf[rt_dma].data[index+i].timeStampUs;
            //
            if(IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
                LOG_DBG("[rtbc][DEQUE]:index(%d)/PA(0x%x)/memID(%d)/size(0x%x)/VA(0x%x)", \
                    index+i, \
                    deque_buf.data[i].base_pAddr, \
                    deque_buf.data[i].memID, \
                    deque_buf.data[i].size, \
                    deque_buf.data[i].base_vAddr);
            }

        }
        //
        if(IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
            LOG_DBG("[rtbc][DEQUE]-");
        }
        //
        //spin_unlock_irqrestore(&(IspInfo.SpinLockRTBC),g_Flash_SpinLock);
        //
    }
    else {
        //
        //spin_unlock_irqrestore(&(IspInfo.SpinLockRTBC),g_Flash_SpinLock);
        LOG_ERR("[rtbc][DEQUE]:no filled buffer");
        Ret = -EFAULT;
    }

    return Ret;
}

static long ISP_Buf_CTRL_FUNC(MUINT32 Param)
{
    MINT32 Ret = 0;
    MINT32 rt_dma;
    MUINT32 reg_val = 0;
    MUINT32 reg_val2 = 0;
    MUINT32 i = 0;
    MUINT32 iBuf = 0;
    MUINT32 size = 0;
    MUINT32 bWaitBufRdy = 0;
    ISP_BUFFER_CTRL_STRUCT rt_buf_ctrl;
    //MUINT32 buffer_exist = 0;
#if defined(_rtbc_use_cq0c_) // fix compile warning
    CQ_RTBC_FBC imgo_fbc;
    CQ_RTBC_FBC img2o_fbc;
#else
    MINT32 Timeout = 1000; //ms
#endif
    //
    if (NULL == pstRTBuf)  {
        LOG_ERR("[rtbc]NULL pstRTBuf");
        return -EFAULT;
    }
    //
    if(copy_from_user(&rt_buf_ctrl, (void*)Param, sizeof(ISP_BUFFER_CTRL_STRUCT)) == 0)
    {
        rt_dma = rt_buf_ctrl.buf_id;
        //
        if(IspInfo.DebugMask & ISP_DBG_INT) {
            LOG_DBG("[rtbc]ctrl(0x%x)/buf_id(0x%x)/data_ptr(0x%x)/ex_data_ptr(0x%x)", \
                rt_buf_ctrl.ctrl, \
                rt_buf_ctrl.buf_id, \
                rt_buf_ctrl.data_ptr, \
                rt_buf_ctrl.ex_data_ptr);
        }
        //
        if ( _imgo_ != rt_dma && _img2o_ != rt_dma ) {
            LOG_ERR("[rtbc]invalid dma channel(%d)",rt_dma);
            return -EFAULT;
        }
        //
#if defined(_rtbc_use_cq0c_)
        //
        if ( (ISP_RT_BUF_CTRL_ENQUE==rt_buf_ctrl.ctrl) || \
             (ISP_RT_BUF_CTRL_EXCHANGE_ENQUE==rt_buf_ctrl.ctrl) || \
             (ISP_RT_BUF_CTRL_DEQUE==rt_buf_ctrl.ctrl) || \
             (ISP_RT_BUF_CTRL_IS_RDY==rt_buf_ctrl.ctrl) ) {
            //
            imgo_fbc.Reg_val= ISP_RD32(ISP_REG_ADDR_IMGO_FBC);
            img2o_fbc.Reg_val= ISP_RD32(ISP_REG_ADDR_IMG2O_FBC);
            //
            if(IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
                LOG_DBG("[rtbc]:ctrl(%d),o(0x%x),2o(0x%x)",rt_buf_ctrl.ctrl,ISP_RD32(ISP_REG_ADDR_IMGO_FBC),ISP_RD32(ISP_REG_ADDR_IMG2O_FBC));
            }
        }

#endif

        //
        switch(rt_buf_ctrl.ctrl) {
            case ISP_RT_BUF_CTRL_ENQUE:
            case ISP_RT_BUF_CTRL_EXCHANGE_ENQUE:
                //
                if(copy_from_user(&rt_buf_info, (void*)rt_buf_ctrl.data_ptr, sizeof(ISP_RT_BUF_INFO_STRUCT)) == 0) {
                    reg_val  = ISP_RD32(ISP_REG_ADDR_TG_VF_CON);
                    reg_val2 = ISP_RD32(ISP_REG_ADDR_TG2_VF_CON);

                    //VF start already
                    if ( (reg_val & 0x01) || (reg_val2 & 0x01) ) {

#if defined(_rtbc_use_cq0c_)
                        //
                        if(IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
                            LOG_DBG("[rtbc][ENQUE]:ex_data_ptr(0x%x)",rt_buf_ctrl.ex_data_ptr);
                        }
                        //
                        if ( 0 != rt_buf_ctrl.ex_data_ptr ) {
                            if(copy_from_user(&ex_rt_buf_info, (void*)rt_buf_ctrl.ex_data_ptr, sizeof(ISP_RT_BUF_INFO_STRUCT)) == 0) {
                                //
                                for (i=0;i<ISP_RT_BUF_SIZE;i++) {
                                    //
                                    if ( pstRTBuf->ring_buf[rt_dma].data[i].base_pAddr == rt_buf_info.base_pAddr ) {
                                        LOG_DBG("[rtbc]dma(%d),old(%d) PA(0x%x) VA(0x%x)",rt_dma,i,pstRTBuf->ring_buf[rt_dma].data[i].base_pAddr,pstRTBuf->ring_buf[rt_dma].data[i].base_vAddr);
                                        pstRTBuf->ring_buf[rt_dma].data[i].memID= ex_rt_buf_info.memID;
                                        pstRTBuf->ring_buf[rt_dma].data[i].size= ex_rt_buf_info.size;
                                        pstRTBuf->ring_buf[rt_dma].data[i].base_pAddr = ex_rt_buf_info.base_pAddr;
                                        pstRTBuf->ring_buf[rt_dma].data[i].base_vAddr = ex_rt_buf_info.base_vAddr;
                                        LOG_DBG("[rtbc]dma(%d),new(%d) PA(0x%x) VA(0x%x)",rt_dma,i,pstRTBuf->ring_buf[rt_dma].data[i].base_pAddr,pstRTBuf->ring_buf[rt_dma].data[i].base_vAddr);
                                        break;
                                    }
                                }
                            }
                        }

                        //set RCN_INC = 1;
                        //RCNT++
                        //FBC_CNT--
                        if ( _imgo_ == rt_dma  ) {
                            imgo_fbc.Bits.RCNT_INC = 1;
                            ISP_WR32(ISP_REG_ADDR_IMGO_FBC,imgo_fbc.Reg_val);
                        }
                        else if ( _img2o_ == rt_dma ) {
                            img2o_fbc.Bits.RCNT_INC = 1;
                            ISP_WR32(ISP_REG_ADDR_IMG2O_FBC,img2o_fbc.Reg_val);
                        }
                        //
                        //if(IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
                            LOG_DBG("[rtbc][ENQUE]:dma(%d),PA(0x%x),O(0x%x),2O(0x%x),fps(%d)us",
                                rt_dma, \
                                rt_buf_info.base_pAddr, \
                                ISP_RD32(ISP_REG_ADDR_IMGO_BASE_ADDR), \
                                ISP_RD32(ISP_REG_ADDR_IMG2O_BASE_ADDR),
                                avg_frame_time);
                        //}
#else


                        //
                        if(IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
                            LOG_DBG("[rtbc][ENQUE]+:wait enque done.(0x%x)(0x%x)",reg_val,reg_val2);
                        }
                        //wait till enq done in SOF
                        rtbc_enq_dma = rt_dma;
                        bEnqBuf = 1;
                        Timeout = wait_event_interruptible_timeout(
                                    IspInfo.WaitQueueHead,
                                    (0==bEnqBuf),
                                    ISP_MsToJiffies(Timeout));
                        //
                        if(Timeout == 0)
                        {
                            LOG_ERR("[rtbc][ENQUE]:timeout(%d)",
                                        bEnqBuf);
                            Ret = -EFAULT;
                        }
                        //
                        if(IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
                            LOG_DBG("[rtbc][ENQUE]-:enque done.");
                        }
#endif

                    }
                    else {
                        ISP_RTBC_ENQUE(rt_dma);
                    }

                }
                else {
                    LOG_ERR("[rtbc][ENQUE]:copy_from_user fail");
                    return -EFAULT;
                }
                break;

            case ISP_RT_BUF_CTRL_DEQUE:
                //
                reg_val  = ISP_RD32(ISP_REG_ADDR_TG_VF_CON);
                reg_val2 = ISP_RD32(ISP_REG_ADDR_TG2_VF_CON);
                //VF start already
                if ( (reg_val & 0x01) || (reg_val2 & 0x01) ) {
#if defined(_rtbc_use_cq0c_)
                    //
                    deque_buf.count = 1;

                    iBuf = ( (_imgo_==rt_dma)?imgo_fbc.Bits.RCNT:img2o_fbc.Bits.RCNT ) - 1; //RCNT = [1,2,3,...]

                    for (i=0;i<deque_buf.count;i++) {
                        deque_buf.data[i].memID         = pstRTBuf->ring_buf[rt_dma].data[iBuf+i].memID;
                        deque_buf.data[i].size          = pstRTBuf->ring_buf[rt_dma].data[iBuf+i].size;
                        deque_buf.data[i].base_vAddr    = pstRTBuf->ring_buf[rt_dma].data[iBuf+i].base_vAddr;
                        deque_buf.data[i].base_pAddr    = pstRTBuf->ring_buf[rt_dma].data[iBuf+i].base_pAddr;
                        deque_buf.data[i].timeStampS    = pstRTBuf->ring_buf[rt_dma].data[iBuf+i].timeStampS;
                        deque_buf.data[i].timeStampUs   = pstRTBuf->ring_buf[rt_dma].data[iBuf+i].timeStampUs;
                        //
                        //if(IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
                            LOG_DBG("[rtbc][DEQUE]:TStamp\"%d.%06d\",co(0x%08x),c2o(0x%08x),i(%d),VA(0x%x),PA(0x%x),O(0x%x),2O(0x%x)", \
                                deque_buf.data[i].timeStampS, \
                                deque_buf.data[i].timeStampUs, \
                                imgo_fbc,
                                img2o_fbc,
                                iBuf+i, \
                                deque_buf.data[i].base_vAddr, \
                                deque_buf.data[i].base_pAddr,
                                ISP_RD32(ISP_REG_ADDR_IMGO_BASE_ADDR), \
                                ISP_RD32(ISP_REG_ADDR_IMG2O_BASE_ADDR));
                        //}
                        //
                        //tstamp = deque_buf.data[i].timeStampS*1000000+deque_buf.data[i].timeStampUs;
                        //if ( (0 != prv_tstamp) && (prv_tstamp >= tstamp) ) {
                        if ( 0 != prv_tstamp_s ) {
                            if ( ( prv_tstamp_s > deque_buf.data[i].timeStampS) ||
                                 ( (prv_tstamp_s == deque_buf.data[i].timeStampS)&&(prv_tstamp_us >= deque_buf.data[i].timeStampUs) ) ) {
                                LOG_ERR("[rtbc]TS rollback,prv\"%d.%06d\",cur\"%d.%06d\"",prv_tstamp_s,prv_tstamp_us,deque_buf.data[i].timeStampS,deque_buf.data[i].timeStampUs);
                        }
                        }
                        prv_tstamp_s = deque_buf.data[i].timeStampS;
                        prv_tstamp_us = deque_buf.data[i].timeStampUs;
                    }

#if 0
LOG_DBG("+LARB in DEQUE,BWL(0x%08X)/(0x%08X)/(0x%08X)/(0x%08X),220(0x%08X)/(0x%08X),0x14(0x%08X)/(0x%08X)/(0x%08X)/(0x%08X)/(0x%08X)", \
    ISP_RD32(0xF0202000+0x204), \
    ISP_RD32(0xF0202000+0x20c), \
    ISP_RD32(0xF0202000+0x210), \
    ISP_RD32(0xF0202000+0x214), \
    ISP_RD32(0xF0202000+0x220), \
    ISP_RD32(0xF0202000+0x230), \
    ISP_RD32(SMI_LARB0+0x10), \
    ISP_RD32(SMI_LARB1+0x10), \
    ISP_RD32(SMI_LARB2+0x10), \
    ISP_RD32(SMI_LARB3+0x10), \
    ISP_RD32(SMI_LARB4+0x10));
#endif

#else
                    //
                    if(IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
                        LOG_DBG("[rtbc][DEQUE]+:wait deque done.(0x%x)(0x%x)",reg_val,reg_val2);
                    }
                    //wait till deq done in SOF
                    rtbc_deq_dma = rt_dma;
                    bDeqBuf = 1;
                    Timeout = wait_event_interruptible_timeout(
                                IspInfo.WaitQueueHead,
                                (0==bDeqBuf),
                                ISP_MsToJiffies(Timeout));
                    //
                    if(Timeout == 0)
                    {
                        LOG_ERR("[rtbc][DEQUE]:timeout(%d)",
                                    bDeqBuf);
                        LOG_ERR("ISP_IRQ_TYPE_INT:IrqStatus(0x%08X)",IspInfo.IrqInfo.Status[ISP_IRQ_TYPE_INT]);
                        ISP_DumpReg();
                        Ret = -EFAULT;
                    }

                    if(IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
                        LOG_DBG("[rtbc][DEQUE]-:deque done.");
                    }
#endif
                }
                else {
                    ISP_RTBC_DEQUE(rt_dma);
                }

                if ( deque_buf.count ) {
                    //
                    if(copy_to_user((void*)rt_buf_ctrl.data_ptr, &deque_buf, sizeof(ISP_DEQUE_BUF_INFO_STRUCT)) != 0)
                    {
                        LOG_ERR("[rtbc][DEQUE]:copy_to_user failed");
                        Ret = -EFAULT;
                    }

                }
                else {
                    //
                    //spin_unlock_irqrestore(&(IspInfo.SpinLockRTBC),g_Flash_SpinLock);
                    LOG_ERR("[rtbc][DEQUE]:no filled buffer");
                    Ret = -EFAULT;
                }

                break;
            case ISP_RT_BUF_CTRL_IS_RDY:
                //
                //spin_lock_irqsave(&(IspInfo.SpinLockRTBC),g_Flash_SpinLock);
                //
                bWaitBufRdy = 1;
#if defined(_rtbc_use_cq0c_)
                bWaitBufRdy = ( (_imgo_==rt_dma)?(imgo_fbc.Bits.FBC_CNT):(img2o_fbc.Bits.FBC_CNT) )?0:1;
#else
                for ( i=0 ; i<=pstRTBuf->ring_buf[rt_dma].total_count; i++ ) {
                    //
                    if ( ISP_RTBC_BUF_FILLED == pstRTBuf->ring_buf[rt_dma].data[i].bFilled ) {
                        bWaitBufRdy = 0;
                        break;
                    }
                }
#endif
                //
                if(IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
                    LOG_DBG("[rtbc][IS_RDY]:bWaitBufRdy(%d)",bWaitBufRdy);
                }

                //
                //spin_unlock_irqrestore(&(IspInfo.SpinLockRTBC),g_Flash_SpinLock);
                //
                if(copy_to_user((void*)rt_buf_ctrl.data_ptr, &bWaitBufRdy, sizeof(MUINT32)) != 0)
                {
                    LOG_ERR("[rtbc][IS_RDY]:copy_to_user failed");
                    Ret = -EFAULT;
                }
                //
                //spin_unlock_irqrestore(&(IspInfo.SpinLockRTBC), flags);
                //
                break;
            case ISP_RT_BUF_CTRL_GET_SIZE:
                //
                size = pstRTBuf->ring_buf[rt_dma].total_count;
                //
                if(IspInfo.DebugMask & ISP_DBG_INT) {
                    LOG_DBG("[rtbc][GET_SIZE]:rt_dma(%d)/size(%d)",rt_dma,size);
                }
                if(copy_to_user((void*)rt_buf_ctrl.data_ptr, &size, sizeof(MUINT32)) != 0)
                {
                    LOG_ERR("[rtbc][GET_SIZE]:copy_to_user failed");
                    Ret = -EFAULT;
                }
                break;
            case ISP_RT_BUF_CTRL_CLEAR:
                //
                //if(IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
                    LOG_DBG("[rtbc][CLEAR]:rt_dma(%d)",rt_dma);
                //}
                //
#if 0
                pstRTBuf->ring_buf[rt_dma].total_count= 0;
                pstRTBuf->ring_buf[rt_dma].start    = 0;
                pstRTBuf->ring_buf[rt_dma].empty_count= 0;
                pstRTBuf->ring_buf[rt_dma].active   = 0;

                for (i=0;i<ISP_RT_BUF_SIZE;i++) {
                    if ( pstRTBuf->ring_buf[rt_dma].data[i].base_pAddr == rt_buf_info.base_pAddr ) {
                        buffer_exist = 1;
                        break;
                    }
                    //
                    if ( pstRTBuf->ring_buf[rt_dma].data[i].base_pAddr == 0 ) {
                        break;
                    }
                }
#else
                memset((char*)pstRTBuf,0x00,sizeof(ISP_RT_BUF_STRUCT));
                prv_tstamp_s = 0;
                prv_tstamp_us = 0;

                sof_count = 0;
                start_time = 0;
                avg_frame_time = 0;

                TempAddr = 0;

#endif

                break;

            case ISP_RT_BUF_CTRL_MAX:   // Add this to remove build warning.
                // Do nothing.
                break;

        }
        //
    }
    else
    {
        LOG_ERR("[rtbc]copy_from_user failed");
        Ret = -EFAULT;
    }

    return Ret;
}

static MINT32 ISP_SOF_Buf_Get(unsigned long long sec,unsigned long usec)
{
#if defined(_rtbc_use_cq0c_)

    CQ_RTBC_FBC imgo_fbc;
    CQ_RTBC_FBC img2o_fbc;
    MUINT32 curr_imgo = 0;//(imgo_fbc.Bits.WCNT+imgo_fbc.Bits.FB_NUM-1)%imgo_fbc.Bits.FB_NUM; //[0,1,2,...]
    MUINT32 curr_img2o = 0;//(img2o_fbc.Bits.WCNT+img2o_fbc.Bits.FB_NUM-1)%img2o_fbc.Bits.FB_NUM; //[0,1,2,...]
    volatile MUINT32 curr_pa = 0;    
    MUINT32 i=0;
    //
    imgo_fbc.Reg_val    = ISP_RD32(ISP_REG_ADDR_IMGO_FBC);
    img2o_fbc.Reg_val   = ISP_RD32(ISP_REG_ADDR_IMG2O_FBC);

    //No drop
    if ( imgo_fbc.Bits.FB_NUM != imgo_fbc.Bits.FBC_CNT ) {
        pstRTBuf->dropCnt = 0;
    }
    //dropped
    else {
        pstRTBuf->dropCnt = 1;
    }
    //
    if(IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
        LOG_DBG("[rtbc]dropCnt(%d)",pstRTBuf->dropCnt);
    }
    //No drop
    if ( 0 == pstRTBuf->dropCnt ) {

        //verify write buffer
        curr_pa = ISP_RD32(ISP_REG_ADDR_IMGO_BASE_ADDR);
        if(TempAddr == curr_pa)
        {
            LOG_ERR("TempAddr:Last(0x%08X) == Cur(0x%08X)",TempAddr,curr_pa);
            //ISP_DumpReg();
        }
        TempAddr = curr_pa;

        //last update buffer index
        curr_imgo = imgo_fbc.Bits.WCNT - 1; //[0,1,2,...]
        //curr_img2o = img2o_fbc.Bits.WCNT - 1; //[0,1,2,...]
        curr_img2o = curr_imgo;

#if 1
        //verify write buffer,once pass1_done lost, WCNT is untrustful.
        if ( ISP_RT_CQ0C_BUF_SIZE < pstRTBuf->ring_buf[_imgo_].total_count ) {
            LOG_ERR("[rtbc]buf cnt(%d)",pstRTBuf->ring_buf[_imgo_].total_count);
            pstRTBuf->ring_buf[_imgo_].total_count = ISP_RT_CQ0C_BUF_SIZE;
        }
        //
        if ( curr_pa != pstRTBuf->ring_buf[_imgo_].data[curr_imgo].base_pAddr ) {
            //
            for( i = 0; i< pstRTBuf->ring_buf[_imgo_].total_count; i++ ) {
                //
                if ( curr_pa == pstRTBuf->ring_buf[_imgo_].data[i].base_pAddr ) {
                    //  
                    if(IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
                        LOG_DBG("[rtbc]curr:old/new(%d/%d)",curr_imgo,i);
                    }
                    //
                    curr_imgo  = i;
                    curr_img2o = i;
                    break;
                }                
            }
        }
#endif

        //
        pstRTBuf->ring_buf[_imgo_].data[curr_imgo].timeStampS = sec;
        pstRTBuf->ring_buf[_imgo_].data[curr_imgo].timeStampUs = usec;
        pstRTBuf->ring_buf[_img2o_].data[curr_img2o].timeStampS = sec;
        pstRTBuf->ring_buf[_img2o_].data[curr_img2o].timeStampUs = usec;
        //
        if(IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
            LOG_DBG("[rtbc]TStamp(%d.%06d),curr(%d),pa(0x%x/0x%x),o2o(0x%x/0x%x),fbc_o(0x%08x),fbc_2o(0x%08x),cq0c(0x%x)", \
                pstRTBuf->ring_buf[_imgo_].data[curr_imgo].timeStampS,  \
                pstRTBuf->ring_buf[_imgo_].data[curr_imgo].timeStampUs, \
                curr_imgo,
                pstRTBuf->ring_buf[_imgo_].data[curr_imgo].base_pAddr,  \
                pstRTBuf->ring_buf[_img2o_].data[curr_imgo].base_pAddr,  \                
                ISP_RD32(ISP_REG_ADDR_IMGO_BASE_ADDR), \
                ISP_RD32(ISP_REG_ADDR_IMG2O_BASE_ADDR), \
                imgo_fbc.Reg_val, \
                img2o_fbc.Reg_val, \
                ISP_RD32(ISP_ADDR+0xBC) );
        }
        //
    }


    //frame time profile
    if ( 0 == sof_count ) {
        start_time = sec*1000000+ usec; //us
    }
    //calc once per senond
    if ( 1000000 < ( (sec*1000000+ usec)- start_time) ) {
        if ( 0 != sof_count) {
            avg_frame_time = (sec*1000000+ usec)- start_time;
            avg_frame_time = (MUINT32)(avg_frame_time / sof_count) ;
        }
        sof_count = 0;
    }
    else {
        sof_count++;
    }

#else

int i;
int i_dma;
unsigned int pAddr = 0;
unsigned int dma_base_addr = 0;
unsigned int next = 0;

    if(IspInfo.DebugMask & ISP_DBG_INT) {
        LOG_DBG("[rtbc]E:fbc(0x%x/0x%x)/aaa(%d)",ISP_RD32(ISP_REG_ADDR_IMGO_FBC),ISP_RD32(ISP_REG_ADDR_IMG2O_FBC),aaa++);
    }
    //
    //spin_lock_irqsave(&(IspInfo.SpinLockRTBC),g_Flash_SpinLock);
    //
    if(IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
        LOG_DBG("+:[rtbc]aaa(%d)",aaa++);
    }
    //
    for ( i=0;i<=1;i++ ) {
        i_dma = (0==i)?_imgo_:_img2o_;
        dma_base_addr = (_imgo_==i_dma)?ISP_REG_ADDR_IMGO_BASE_ADDR:ISP_REG_ADDR_IMG2O_BASE_ADDR;

        //queue buffer status
        if(IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
            LOG_DBG("[rtbc][SOF]:dma(%d),start(%d),total(%d),empty(%d) ", \
                i_dma,
                pstRTBuf->ring_buf[i_dma].start,
                pstRTBuf->ring_buf[i_dma].total_count,
                pstRTBuf->ring_buf[i_dma].empty_count);

            for ( i=0;i<=pstRTBuf->ring_buf[i_dma].total_count-1;i++ ) {
                LOG_DBG("[rtbc][SOF]Buf List:%d/%d/0x%x/0x%x/0x%x/%d/  ", \
                    i, \
                    pstRTBuf->ring_buf[i_dma].data[i].memID, \
                    pstRTBuf->ring_buf[i_dma].data[i].size, \
                    pstRTBuf->ring_buf[i_dma].data[i].base_vAddr, \
                    pstRTBuf->ring_buf[i_dma].data[i].base_pAddr, \
                    pstRTBuf->ring_buf[i_dma].data[i].bFilled);
            }

        }

        //ring buffer get next buffer
        if ( 0 == pstRTBuf->ring_buf[i_dma].empty_count) {
            //once if buffer put into queue between SOF and ISP_DONE.
            pstRTBuf->ring_buf[i_dma].active = MFALSE;
            //
            if(IspInfo.DebugMask & ISP_DBG_INT) {
                LOG_DBG("[rtbc][SOF]:dma(%d)real time buffer number empty,start(%d) ",i_dma,pstRTBuf->ring_buf[i_dma].start);
            }
        }
        else {
            //
            if ( 2 <= pstRTBuf->ring_buf[i_dma].empty_count) {
                //next buffer
                next = (pstRTBuf->ring_buf[i_dma].start+1)%pstRTBuf->ring_buf[i_dma].total_count;
                pAddr = pstRTBuf->ring_buf[i_dma].data[next].base_pAddr;
                //
                ISP_WR32(dma_base_addr, pAddr);
                //
                if(IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
                    LOG_DBG("[rtbc][SOF]:dma(%d),start(%d),empty(%d),next(%d),next_PA(0x%x) ", \
                        i_dma,
                        pstRTBuf->ring_buf[i_dma].start,
                        pstRTBuf->ring_buf[i_dma].empty_count,
                        next,
                        pAddr);
                }
            }
            else {
                if(IspInfo.DebugMask & ISP_DBG_INT) {
                    LOG_DBG("[rtbc][SOF]:dma(%d)real time buffer number is running out ",i_dma);
                }
            }

            //once if buffer put into queue between SOF and ISP_DONE.
            pstRTBuf->ring_buf[i_dma].active = MTRUE;
        }
    }
    //
    if( bEnqBuf ) {
        ISP_RTBC_ENQUE(rtbc_enq_dma);
        bEnqBuf = 0;
        wake_up_interruptible(&IspInfo.WaitQueueHead);
    }
    //
    if (bDeqBuf) {
        ISP_RTBC_DEQUE(rtbc_deq_dma);
        bDeqBuf = 0;
        wake_up_interruptible(&IspInfo.WaitQueueHead);
    }
    //
    if(IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
        LOG_DBG("-:[rtbc]");
    }
    //
    pstRTBuf->state = ISP_RTBC_STATE_SOF;
    //spin_unlock_irqrestore(&(IspInfo.SpinLockRTBC),g_Flash_SpinLock);
#endif

    return 0;
}//

#ifndef _rtbc_use_cq0c_
static MINT32 ISP_DONE_Buf_Time(unsigned long long sec,unsigned long usec)
{
//int k;
int i;
int i_dma;
unsigned int curr;
unsigned int reg_fbc;
MUINT32 reg_val = 0;


    if(IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
        LOG_DBG("[rtbc]E:fbc(0x%x/0x%x)",ISP_RD32(ISP_REG_ADDR_IMGO_FBC),ISP_RD32(ISP_REG_ADDR_IMG2O_FBC));
    }
    //
#if 0
    if ( spin_trylock_bh(&(IspInfo.SpinLockRTBC)) ) {
        if(IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
            LOG_DBG("[rtbc]:unlock state");
        }
    }
    else {
        if(IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
            LOG_DBG("[rtbc]:locked state");
        }
    }
#endif
    //spin_lock_irqsave(&(IspInfo.SpinLockRTBC),g_Flash_SpinLock);
    //
    if(IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
        LOG_DBG("+:[rtbc]aaa(%d)",aaa++);
    }
    //
    for ( i=0;i<=1;i++ ) {
        //
        i_dma = (0==i)?_imgo_:_img2o_;
        reg_fbc = (_imgo_==i_dma)?ISP_REG_ADDR_IMGO_FBC:ISP_REG_ADDR_IMG2O_FBC;
        //
        if ( 0 == pstRTBuf->ring_buf[i_dma].empty_count) {
            //
            if(IspInfo.DebugMask & ISP_DBG_INT) {
                LOG_DBG("[rtbc][DONE]:dma(%d)real time buffer number empty,start(%d) ",i_dma,pstRTBuf->ring_buf[i_dma].start);
            }
            //
            continue;
        }
        //once if buffer put into queue between SOF and ISP_DONE.
        if ( MFALSE == pstRTBuf->ring_buf[i_dma].active ) {
            //
            if(IspInfo.DebugMask & ISP_DBG_INT) {
                LOG_DBG("[rtbc][DONE] ERROR: missing SOF ");
            }
            //
            continue;
        }

        while(1)
        {
            MUINT32 loopCount = 0;
            curr = pstRTBuf->ring_buf[i_dma].start;

            if(IspInfo.DebugMask & ISP_DBG_INT) {
                LOG_DBG("i_dma(%d),curr(%d),bFilled(%d)",
                        i_dma,
                        curr,
                        pstRTBuf->ring_buf[i_dma].data[curr].bFilled);
            }

            if(pstRTBuf->ring_buf[i_dma].data[curr].bFilled == ISP_RTBC_BUF_EMPTY)
            {
                pstRTBuf->ring_buf[i_dma].data[curr].bFilled = ISP_RTBC_BUF_FILLED;
                //start + 1
                pstRTBuf->ring_buf[i_dma].start = (curr+1)%pstRTBuf->ring_buf[i_dma].total_count;
                pstRTBuf->ring_buf[i_dma].empty_count--;
                break;
            }
            else
            {
                if(IspInfo.DebugMask & ISP_DBG_INT) {
                    LOG_DBG("i_dma(%d),curr(%d),bFilled(%d) != ISP_RTBC_BUF_EMPTY",
                            i_dma,
                            curr,
                            pstRTBuf->ring_buf[i_dma].data[curr].bFilled);
                }
                //start + 1
                pstRTBuf->ring_buf[i_dma].start = (curr+1)%pstRTBuf->ring_buf[i_dma].total_count;
            }

            loopCount++;
            if(loopCount > pstRTBuf->ring_buf[i_dma].total_count)
            {
                LOG_ERR("Can't find empty dma(%d) buf in total_count(%d)",
                        i_dma,
                        pstRTBuf->ring_buf[i_dma].total_count);
                break;
            }
        }

        //enable fbc to stall DMA
        if ( 0 == pstRTBuf->ring_buf[i_dma].empty_count) {
                if (_imgo_ == i_dma) {
                    reg_val = ISP_RD32(ISP_REG_ADDR_IMGO_FBC);
                    reg_val |= 0x4000;
                    //ISP_WR32(ISP_REG_ADDR_IMGO_FBC,reg_val);
                }
                else {
                    reg_val = ISP_RD32(ISP_REG_ADDR_IMG2O_FBC);
                    reg_val |= 0x4000;
                    //ISP_WR32(ISP_REG_ADDR_IMG2O_FBC,reg_val);
                }
                //
                if(IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
                LOG_DBG("[rtbc][DONE]:dma(%d),en fbc(0x%x) stalled DMA out",i_dma,ISP_RD32(reg_fbc));
            }
        }
        //
        if(IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
            LOG_DBG("[rtbc][DONE]:dma(%d),start(%d),empty(%d)", \
                i_dma,
                pstRTBuf->ring_buf[i_dma].start,
                pstRTBuf->ring_buf[i_dma].empty_count);
        }
        //
        pstRTBuf->ring_buf[i_dma].data[curr].timeStampS = sec;
        pstRTBuf->ring_buf[i_dma].data[curr].timeStampUs = usec;
        //
        if(IspInfo.DebugMask & ISP_DBG_INT) {
            LOG_DBG("[rtbc][DONE]:dma(%d),curr(%d),sec(%lld),usec(%ld) ", i_dma, curr, sec, usec);
        }

    }


    if(IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
        LOG_DBG("-:[rtbc]");
    }
    //
    pstRTBuf->state = ISP_RTBC_STATE_DONE;
    //spin_unlock_irqrestore(&(IspInfo.SpinLockRTBC),g_Flash_SpinLock);

    return 0;
}
#endif

/*******************************************************************************
*
********************************************************************************/
static MINT32 ISP_WaitIrq(ISP_WAIT_IRQ_STRUCT WaitIrq)
{
    MINT32 Ret = 0, Timeout = WaitIrq.Timeout;
    MUINT32 i;
    MUINT32 flags;
    //
    if(IspInfo.DebugMask & ISP_DBG_INT)
    {
        LOG_DBG("Clear(%d), Type(%d), Status(0x%08X), Timeout(%d)",
                    WaitIrq.Clear,
                    WaitIrq.Type,
                    WaitIrq.Status,
                    WaitIrq.Timeout);
    }
    //
    if(WaitIrq.Clear == ISP_IRQ_CLEAR_WAIT)
    {
        spin_lock_irqsave(&(IspInfo.SpinLockIrq), flags);
        if(IspInfo.IrqInfo.Status[WaitIrq.Type] & WaitIrq.Status)
        {
            LOG_DBG("WARNING: Clear(%d), Type(%d): IrqStatus(0x%08X) has been cleared",WaitIrq.Clear,WaitIrq.Type,IspInfo.IrqInfo.Status[WaitIrq.Type] & WaitIrq.Status);
            IspInfo.IrqInfo.Status[WaitIrq.Type] &= (~WaitIrq.Status);
        }
        spin_unlock_irqrestore(&(IspInfo.SpinLockIrq), flags);
    }
    else
    if(WaitIrq.Clear == ISP_IRQ_CLEAR_ALL)
    {
        spin_lock_irqsave(&(IspInfo.SpinLockIrq), flags);
        LOG_DBG("WARNING: Clear(%d), Type(%d): IrqStatus(0x%08X) has been cleared",WaitIrq.Clear,WaitIrq.Type,IspInfo.IrqInfo.Status[WaitIrq.Type]);
        IspInfo.IrqInfo.Status[WaitIrq.Type] = 0;
        spin_unlock_irqrestore(&(IspInfo.SpinLockIrq), flags);
    }
    //
    #if ISP_IRQ_POLLING
    ISP_IRQ_TYPE_ENUM IrqStatus[ISP_IRQ_TYPE_AMOUNT];
    //
    while(1)
    {
        IrqStatus[ISP_IRQ_TYPE_INT]     = (ISP_RD32(ISP_REG_ADDR_INT_STATUS)    & (IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INT]|IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_INT]));
        IrqStatus[ISP_IRQ_TYPE_DMA]     = (ISP_RD32(ISP_REG_ADDR_DMA_INT)       & (IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMA]|IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_DMA]));
        IrqStatus[ISP_IRQ_TYPE_INTB]    = (ISP_RD32(ISP_REG_ADDR_INTB_STATUS)   & (IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INTB]|IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_INTB]));
        IrqStatus[ISP_IRQ_TYPE_DMAB]    = (ISP_RD32(ISP_REG_ADDR_DMAB_INT)      & (IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMAB]|IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_DMAB]));
        IrqStatus[ISP_IRQ_TYPE_INTC]    = (ISP_RD32(ISP_REG_ADDR_INTC_STATUS)   & (IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INTC]|IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_INTC]));
        IrqStatus[ISP_IRQ_TYPE_DMAC]    = (ISP_RD32(ISP_REG_ADDR_DMAC_INT)      & (IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMAC]|IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_DMAC]));
        IrqStatus[ISP_IRQ_TYPE_INTX]    = (ISP_RD32(ISP_REG_ADDR_INT_STATUSX)   & (IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INTX]|IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_INTX]));
        IrqStatus[ISP_IRQ_TYPE_DMAX]    = (ISP_RD32(ISP_REG_ADDR_DMA_INTX)      & (IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMAX]|IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_DMAX]));
        //
        for(i=0; i<ISP_IRQ_TYPE_AMOUNT; i++)
        {
            if(IspInfo.IrqInfo.ErrMask[i] & IrqStatus[i])
            {
                //LOG_ERR("Error IRQ, Type(%d), Status(0x%08X)",i,IspInfo.IrqInfo.ErrMask[i] & IrqStatus[i]);
                //TODO: Add error handler...
            }
            //
            if(IspInfo.DebugMask & ISP_DBG_INT)
            {
                LOG_DBG("Type(%d), IrqStatus(0x%08X | 0x%08X)",i,IspInfo.IrqInfo.Status[i], IrqStatus[i]);
                //LOG_DBG("Mask(0x%08X), ErrMask(0x%08X)",IspInfo.IrqInfo.Mask[i], IspInfo.IrqInfo.ErrMask[i]);
            }
            IspInfo.IrqInfo.Status[i] |= (IrqStatus[i] & IspInfo.IrqInfo.Mask[i]);
        }
        //
        if( (IspInfo.IrqInfo.Status[WaitIrq.Type] & WaitIrq.Status) == WaitIrq.Status ||
            Timeout == 0)
        {
            break;
        }
        //
        mdelay(1);
        Timeout -= 1;
    }
    #else
    Timeout = wait_event_interruptible_timeout(
                IspInfo.WaitQueueHead,
                ISP_GetIRQState(WaitIrq.Type, WaitIrq.Status),
                ISP_MsToJiffies(WaitIrq.Timeout));
    #endif
    //
    if(Timeout == 0)
    {
        LOG_ERR("Clear(%d), Type(%d), IrqStatus(0x%08X), WaitStatus(0x%08X), Timeout(%d)",
                    WaitIrq.Clear,
                    WaitIrq.Type,
                    IspInfo.IrqInfo.Status[WaitIrq.Type],
                    WaitIrq.Status,
                    WaitIrq.Timeout);
        if((    WaitIrq.Type == ISP_IRQ_TYPE_INT &&
                WaitIrq.Status == ISP_IRQ_INT_STATUS_AF_DON_ST))
        {
            //Do nothing.
        }
        else
        {
            ISP_DumpReg();
        }
        Ret = -EFAULT;
        goto EXIT;
    }
    //
    spin_lock_irqsave(&(IspInfo.SpinLockIrq), flags);
    //
    if(IspInfo.DebugMask & ISP_DBG_INT)
    {
        for(i=0; i<ISP_IRQ_TYPE_AMOUNT; i++)
        {
            LOG_DBG("Type(%d), IrqStatus(0x%08X)",i,IspInfo.IrqInfo.Status[i]);
        }
    }
    //
    IspInfo.IrqInfo.Status[WaitIrq.Type] &= (~WaitIrq.Status);
    //
    spin_unlock_irqrestore(&(IspInfo.SpinLockIrq), flags);
    //
    //check CQ status, when pass2, pass2b, pass2c done
    if( WaitIrq.Status == ISP_IRQ_INT_STATUS_PASS2_DON_ST  ||
        WaitIrq.Status == ISP_IRQ_INTB_STATUS_PASS2_DON_ST ||
        WaitIrq.Status == ISP_IRQ_INTC_STATUS_PASS2_DON_ST )
    {
        MUINT32 CQ_status;
        ISP_WR32( CAMINF_BASE + 0x4160, 0x6000 );
        CQ_status = ISP_RD32( CAMINF_BASE + 0x4164 );
        switch( WaitIrq.Type )
        {
            case ISP_IRQ_TYPE_INT:
                if( (CQ_status&0x0000000F) != 0x001 )
                {
                    LOG_ERR("CQ1 not idle dbg(0x%08x 0x%08x)", \
                        ISP_RD32( CAMINF_BASE + 0x4160 ), CQ_status );
                }
                break;
            case ISP_IRQ_TYPE_INTB:
                if( (CQ_status&0x000000F0) != 0x010 )
                {
                    LOG_ERR("CQ2 not idle dbg(0x%08x 0x%08x)", \
                        ISP_RD32( CAMINF_BASE + 0x4160 ), CQ_status );
                }
                break;
            case ISP_IRQ_TYPE_INTC:
                if( (CQ_status&0x00000F00) != 0x100 )
                {
                    LOG_ERR("CQ3 not idle dbg(0x%08x 0x%08x)", \
                        ISP_RD32( CAMINF_BASE + 0x4160 ), CQ_status );
                }
                break;
            default:
                break;
        }
    }

    EXIT:
    return Ret;
}

//#define _debug_dma_err_
#if defined(_debug_dma_err_)
#define bit(x) (0x1<<(x))

MUINT32 DMA_ERR[3*12] = {
	bit(1) , 0xF50043A8, 0x00000011, //IMGI
	bit(2) , 0xF50043AC, 0x00000021, //IMGCI
	bit(4) , 0xF50043B0, 0x00000031, //LSCI
	bit(5) , 0xF50043B4, 0x00000051, //FLKI
	bit(6) , 0xF50043B8, 0x00000061, //LCEI
	bit(7) , 0xF50043BC, 0x00000071, //VIPI
	bit(8) , 0xF50043C0, 0x00000081, //VIP2I
	bit(9) , 0xF50043C4, 0x00000194, //IMGO
	bit(10), 0xF50043C8, 0x000001a4, //IMG2O
	bit(11), 0xF50043CC, 0x000001b4, //LCSO
	bit(12), 0xF50043D0, 0x000001c4, //ESFKO
	bit(13), 0xF50043D4, 0x000001d4, //AAO
};

static MINT32 DMAErrHandler()
{
	MUINT32 err_ctrl = ISP_RD32(0xF50043A4);
	LOG_DBG("err_ctrl(0x%08x)", err_ctrl);

	MUINT32 i = 0;

	MUINT32 *pErr = DMA_ERR;
	for( i = 0; i < 12 ; i++ )
	{
		MUINT32 addr = 0;
#if 1
		if( err_ctrl & (*pErr) )
		{
			ISP_WR32(0xF5004160, pErr[2]);
			addr = pErr[1];

			LOG_DBG("(0x%08x, 0x%08x), dbg(0x%08x, 0x%08x)",
				addr, ISP_RD32(addr),
				ISP_RD32(0xF5004160), ISP_RD32(0xF5004164));
		}
#else
		addr = pErr[1];
		MUINT32 status = ISP_RD32( addr );

		if( status & 0x0000FFFF )
		{
			ISP_WR32(0xF5004160, pErr[2]);
			addr = pErr[1];

			LOG_DBG("(0x%08x, 0x%08x), dbg(0x%08x, 0x%08x)",
				addr, status,
				ISP_RD32(0xF5004160), ISP_RD32(0xF5004164));
		}
#endif
		pErr = pErr + 3;
	}

}
#endif

/*******************************************************************************
*
********************************************************************************/
static __tcmfunc irqreturn_t ISP_Irq(
    MINT32  Irq,
    void*   DeviceId)
{
//    LOG_DBG("- E.");
    MUINT32 i;
    MUINT32 IrqStatus[ISP_IRQ_TYPE_AMOUNT];
    MUINT32 IrqStatus_fbc_int;
    // Read irq status
    IrqStatus[ISP_IRQ_TYPE_INT]     = (ISP_RD32(ISP_REG_ADDR_INT_STATUS)    & (IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INT]|IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_INT]));
    IrqStatus[ISP_IRQ_TYPE_DMA]     = (ISP_RD32(ISP_REG_ADDR_DMA_INT)       & (IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMA]|IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_DMA]));
    IrqStatus[ISP_IRQ_TYPE_INTB]    = (ISP_RD32(ISP_REG_ADDR_INTB_STATUS)   & (IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INTB]|IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_INTB]));
    IrqStatus[ISP_IRQ_TYPE_DMAB]    = (ISP_RD32(ISP_REG_ADDR_DMAB_INT)      & (IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMAB]|IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_DMAB]));
    IrqStatus[ISP_IRQ_TYPE_INTC]    = (ISP_RD32(ISP_REG_ADDR_INTC_STATUS)   & (IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INTC]|IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_INTC]));
    IrqStatus[ISP_IRQ_TYPE_DMAC]    = (ISP_RD32(ISP_REG_ADDR_DMAC_INT)      & (IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMAC]|IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_DMAC]));
    //below may need to read elsewhere
    IrqStatus[ISP_IRQ_TYPE_INTX]    = (ISP_RD32(ISP_REG_ADDR_INT_STATUSX)   & (IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INTX]|IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_INTX]));
    IrqStatus[ISP_IRQ_TYPE_DMAX]    = (ISP_RD32(ISP_REG_ADDR_DMA_INTX)      & (IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMAX]|IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_DMAX]));
    IrqStatus_fbc_int = ISP_RD32(ISP_ADDR + 0xFC);
    //
    spin_lock(&(IspInfo.SpinLockIrq));
    for(i=0; i<ISP_IRQ_TYPE_AMOUNT; i++)
    {
        if(IspInfo.IrqInfo.ErrMask[i] & IrqStatus[i])
        {
            // ISP_IRQ_INTX_STATUS_IMGO_ERR_ST: on-the-fly imgo error, not really overrun
            if( i != ISP_IRQ_TYPE_INTX || ( IspInfo.IrqInfo.ErrMask[i] & IrqStatus[i] & (~ISP_IRQ_INTX_STATUS_IMGO_ERR_ST)) )
            {
                LOG_DBG("Error IRQ, Type(%d), Status(0x%08X)",i,IspInfo.IrqInfo.ErrMask[i] & IrqStatus[i]);
                //TODO: Add error handler...
            }
        }
        //
        if(IspInfo.DebugMask & ISP_DBG_INT)
        {
            LOG_DBG("Type(%d), IrqStatus(0x%08X | 0x%08X)",i,IspInfo.IrqInfo.Status[i], IrqStatus[i]);
        }
        IspInfo.IrqInfo.Status[i] |= (IrqStatus[i] & IspInfo.IrqInfo.Mask[i]);
    }
    //
    if ( IrqStatus[ISP_IRQ_TYPE_INTX]&ISP_IRQ_INTX_STATUS_DMA_ERR_ST ) {

#if defined(_debug_dma_err_)
        LOG_ERR("[rtbc]StatusX(0x%08X), DMA_ERR",IrqStatus[ISP_IRQ_TYPE_INTX]);  //IMGO may overrun before setting smi regs

        //allan
        DMAErrHandler();

        LOG_DBG("ERR (8018 0x%08x)(8024 0x%08x)(802C 0x%08x)(4440 0x%08x) (810C 0x%08x)",
                    ISP_RD32(0xF5008018),
                    ISP_RD32(0xF5008024),
                    ISP_RD32(0xF500802C),
                    ISP_RD32(0xF5004440),
                    ISP_RD32(0xF500810C));
#endif

    }
    //
    if ( IrqStatus[ISP_IRQ_TYPE_INT]&ISP_IRQ_INT_STATUS_TG1_ERR_ST ||
         IrqStatus[ISP_IRQ_TYPE_INT]&ISP_IRQ_INT_STATUS_TG2_ERR_ST ) {
        LOG_ERR("[rtbc]Status(0x%08X), TG_ERR",IrqStatus[ISP_IRQ_TYPE_INT]);
    }
    //
    if (IrqStatus_fbc_int) {
        LOG_ERR("[rtbc]dropframe st(0x%08X),co(0x%08X),c2o(0x%08X)",IrqStatus_fbc_int,ISP_RD32(ISP_REG_ADDR_IMGO_FBC),ISP_RD32(ISP_REG_ADDR_IMG2O_FBC));
    }
    spin_unlock(&(IspInfo.SpinLockIrq));

    //service pass1_done first once if SOF/PASS1_DONE are coming together.
    //get time stamp
    //push hw filled buffer to sw list
    if(IrqStatus[ISP_IRQ_TYPE_INT] & ISP_IRQ_INT_STATUS_PASS1_TG1_DON_ST)
    {
#if defined(_rtbc_use_cq0c_)
        if(IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
            LOG_DBG("[rtbc]:PASS1_TG1_DON");
        }
#else
        //LOG_DBG("[k_js_test]Pass1_done(0x%x)",IrqStatus[ISP_IRQ_TYPE_INT]);
        unsigned long long  sec;
        unsigned long       usec;
        sec = cpu_clock(0);     //ns
        do_div( sec, 1000 );    //usec
        usec = do_div( sec, 1000000);    //sec and usec

        ISP_DONE_Buf_Time(sec,usec);
        /*Check Timesamp reverse*/
        //what's this?

        //
#endif
		sof_pass1done = 0;
    }

    //switch pass1 WDMA buffer
    //fill time stamp for cq0c
    if(IrqStatus[ISP_IRQ_TYPE_INT] & ISP_IRQ_INT_STATUS_SOF1_INT_ST)
    {
        unsigned long long  sec;
        unsigned long       usec;
        ktime_t             time;
#if 1
        time = ktime_get();     //ns
        sec = time.tv64;
#else
        sec = cpu_clock(0);     //ns
#endif
        do_div( sec, 1000 );    //usec
        usec = do_div( sec, 1000000);    //sec and usec

        ISP_SOF_Buf_Get(sec,usec);

        if( sof_pass1done == 1 ) {
			LOG_DBG("Lost pass1 done");
        }
		sof_pass1done = 1;

    }

    //
    wake_up_interruptible(&IspInfo.WaitQueueHead);
    //Work queue. It is interruptible, so there can be "Sleep" in work queue function.
    if(IrqStatus[ISP_IRQ_TYPE_INT] & (ISP_IRQ_INT_STATUS_VS1_ST|ISP_IRQ_INT_STATUS_VS2_ST))
    {
        IspInfo.TimeLog.Vd = ISP_JiffiesToMs(jiffies);
        schedule_work(&IspInfo.ScheduleWorkVD);
        tasklet_schedule(&IspTaskletVD);
    }
    //Tasklet. It is uninterrupted, so there can NOT be "Sleep" in tasklet function.
    if(IrqStatus[ISP_IRQ_TYPE_INT] & (ISP_IRQ_INT_STATUS_EXPDON1_ST|ISP_IRQ_INT_STATUS_EXPDON2_ST))
    {
        IspInfo.TimeLog.Expdone = ISP_JiffiesToMs(jiffies);
        schedule_work(&IspInfo.ScheduleWorkEXPDONE);
        tasklet_schedule(&IspTaskletEXPDONE);
    }

//    LOG_DBG("- X.");

    //
    return IRQ_HANDLED;
}

/*******************************************************************************
*
********************************************************************************/
static long ISP_ioctl(
    struct file*    pFile,
    unsigned int    Cmd,
    unsigned long   Param)
{
    MINT32 Ret = 0;
    //
    MBOOL   HoldEnable = MFALSE;
    MUINT32 DebugFlag = 0,pid = 0;
    ISP_REG_IO_STRUCT       RegIo;
    ISP_HOLD_TIME_ENUM      HoldTime;
    ISP_WAIT_IRQ_STRUCT     WaitIrq;
    ISP_READ_IRQ_STRUCT     ReadIrq;
    ISP_CLEAR_IRQ_STRUCT    ClearIrq;
    ISP_USER_INFO_STRUCT*   pUserInfo;
    MUINT32 flags;
    //
    if(pFile->private_data == NULL)
    {
        LOG_WRN("private_data is NULL,(process, pid, tgid)=(%s, %d, %d)", current->comm , current->pid, current->tgid);
        return -EFAULT;
    }
    //
    pUserInfo = (ISP_USER_INFO_STRUCT*)(pFile->private_data);
    //
    switch(Cmd)
    {
        case ISP_RESET:
        {
            spin_lock(&(IspInfo.SpinLockIsp));
            ISP_Reset();
            spin_unlock(&(IspInfo.SpinLockIsp));
            break;
        }
        case ISP_RESET_BUF:
        {
            spin_lock_bh(&(IspInfo.SpinLockHold));
            ISP_ResetBuf();
            spin_unlock_bh(&(IspInfo.SpinLockHold));
            break;
        }
        case ISP_READ_REGISTER:
        {
            if(copy_from_user(&RegIo, (void*)Param, sizeof(ISP_REG_IO_STRUCT)) == 0)
            {
                Ret = ISP_ReadReg(&RegIo);
            }
            else
            {
                LOG_ERR("copy_from_user failed");
                Ret = -EFAULT;
            }
            break;
        }
        case ISP_WRITE_REGISTER:
        {
            if(copy_from_user(&RegIo, (void*)Param, sizeof(ISP_REG_IO_STRUCT)) == 0)
            {
                Ret = ISP_WriteReg(&RegIo);
            }
            else
            {
                LOG_ERR("copy_from_user failed");
                Ret = -EFAULT;
            }
            break;
        }
        case ISP_HOLD_REG_TIME:
        {
            if(copy_from_user(&HoldTime, (void*)Param, sizeof(ISP_HOLD_TIME_ENUM)) == 0)
            {
                spin_lock(&(IspInfo.SpinLockIsp));
                Ret = ISP_SetHoldTime(HoldTime);
                spin_unlock(&(IspInfo.SpinLockIsp));
            }
            else
            {
                LOG_ERR("copy_from_user failed");
                Ret = -EFAULT;
            }
            break;
        }
        case ISP_HOLD_REG:
        {
            if(copy_from_user(&HoldEnable, (void*)Param, sizeof(MBOOL)) == 0)
            {
                Ret = ISP_EnableHoldReg(HoldEnable);
            }
            else
            {
                LOG_ERR("copy_from_user failed");
                Ret = -EFAULT;
            }
            break;
        }
        case ISP_WAIT_IRQ:
        {
            if(copy_from_user(&WaitIrq, (void*)Param, sizeof(ISP_WAIT_IRQ_STRUCT)) == 0)
            {
                Ret = ISP_WaitIrq(WaitIrq);
            }
            else
            {
                LOG_ERR("copy_from_user failed");
                Ret = -EFAULT;
            }
            break;
        }
        case ISP_READ_IRQ:
        {
            if(copy_from_user(&ReadIrq, (void*)Param, sizeof(ISP_READ_IRQ_STRUCT)) == 0)
            {
                #if ISP_IRQ_POLLING
                spin_lock_irqsave(&(IspInfo.SpinLockIrq), flags);
                //
                ISP_IRQ_TYPE_ENUM IrqStatus[ISP_IRQ_TYPE_AMOUNT];
                //
                IrqStatus[ISP_IRQ_TYPE_INT]     = (ISP_RD32(ISP_REG_ADDR_INT_STATUS)    & (IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INT]|IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_INT]));
                IrqStatus[ISP_IRQ_TYPE_DMA]     = (ISP_RD32(ISP_REG_ADDR_DMA_INT)       & (IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMA]|IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_DMA]));
                IrqStatus[ISP_IRQ_TYPE_INTB]    = (ISP_RD32(ISP_REG_ADDR_INTB_STATUS)   & (IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INTB]|IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_INTB]));
                IrqStatus[ISP_IRQ_TYPE_DMAB]    = (ISP_RD32(ISP_REG_ADDR_DMAB_INT)      & (IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMAB]|IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_DMAB]));
                IrqStatus[ISP_IRQ_TYPE_INTC]    = (ISP_RD32(ISP_REG_ADDR_INTC_STATUS)   & (IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INTC]|IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_INTC]));
                IrqStatus[ISP_IRQ_TYPE_DMAC]    = (ISP_RD32(ISP_REG_ADDR_DMAC_INT)      & (IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMAC]|IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_DMAC]));
                IrqStatus[ISP_IRQ_TYPE_INTX]    = (ISP_RD32(ISP_REG_ADDR_INT_STATUSX)   & (IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INTX]|IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_INTX]));
                IrqStatus[ISP_IRQ_TYPE_DMAX]    = (ISP_RD32(ISP_REG_ADDR_DMA_INTX)      & (IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMAX]|IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_DMAX]));
                //
                for(i=0; i<ISP_IRQ_TYPE_AMOUNT; i++)
                {
                    IspInfo.IrqInfo.Status[i] |= (IrqStatus[i] & IspInfo.IrqInfo.Mask[i]);
                }
                //
                spin_unlock_irqrestore(&(IspInfo.SpinLockIrq), flags);
                #endif
                //
                ReadIrq.Status = IspInfo.IrqInfo.Status[ReadIrq.Type];
                //
                if(copy_to_user((void*)Param, &ReadIrq, sizeof(ISP_READ_IRQ_STRUCT)) != 0)
                {
                    LOG_ERR("copy_to_user failed");
                    Ret = -EFAULT;
                }
            }
            else
            {
                LOG_ERR("copy_from_user failed");
                Ret = -EFAULT;
            }
            break;
        }
        case ISP_CLEAR_IRQ:
        {
            if(copy_from_user(&ClearIrq, (void*)Param, sizeof(ISP_CLEAR_IRQ_STRUCT)) == 0)
            {
                spin_lock_irqsave(&(IspInfo.SpinLockIrq), flags);
                //
                #if ISP_IRQ_POLLING
                ISP_IRQ_TYPE_ENUM IrqStatus[ISP_IRQ_TYPE_AMOUNT];
                //
                IrqStatus[ISP_IRQ_TYPE_INT]     = (ISP_RD32(ISP_REG_ADDR_INT_STATUS)    & (IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INT]|IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_INT]));
                IrqStatus[ISP_IRQ_TYPE_DMA]     = (ISP_RD32(ISP_REG_ADDR_DMA_INT)       & (IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMA]|IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_DMA]));
                IrqStatus[ISP_IRQ_TYPE_INTB]    = (ISP_RD32(ISP_REG_ADDR_INTB_STATUS)   & (IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INTB]|IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_INTB]));
                IrqStatus[ISP_IRQ_TYPE_DMAB]    = (ISP_RD32(ISP_REG_ADDR_DMAB_INT)      & (IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMAB]|IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_DMAB]));
                IrqStatus[ISP_IRQ_TYPE_INTC]    = (ISP_RD32(ISP_REG_ADDR_INTC_STATUS)   & (IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INTC]|IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_INTC]));
                IrqStatus[ISP_IRQ_TYPE_DMAC]    = (ISP_RD32(ISP_REG_ADDR_DMAC_INT)      & (IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMAC]|IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_DMAC]));
                IrqStatus[ISP_IRQ_TYPE_INTX]    = (ISP_RD32(ISP_REG_ADDR_INT_STATUSX)   & (IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INTX]|IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_INTX]));
                IrqStatus[ISP_IRQ_TYPE_DMAX]    = (ISP_RD32(ISP_REG_ADDR_DMA_INTX)      & (IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMAX]|IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_DMAX]));
                //
                for(i=0; i<ISP_IRQ_TYPE_AMOUNT; i++)
                {
                    IspInfo.IrqInfo.Status[i] |= (IrqStatus[i] & IspInfo.IrqInfo.Mask[i]);
                }
                #endif
                //
                LOG_DBG("ISP_CLEAR_IRQ:Type(%d),Status(0x%08X),IrqStatus(0x%08X)",ClearIrq.Type,ClearIrq.Status,IspInfo.IrqInfo.Status[ClearIrq.Type]);
                IspInfo.IrqInfo.Status[ClearIrq.Type] &= (~ClearIrq.Status);
                spin_unlock_irqrestore(&(IspInfo.SpinLockIrq), flags);
            }
            else
            {
                LOG_ERR("copy_from_user failed");
                Ret = -EFAULT;
            }
            break;
        }
        case ISP_DUMP_REG:
        {
            Ret = ISP_DumpReg();
            break;
        }
        case ISP_DEBUG_FLAG:
        {
            if(copy_from_user(&DebugFlag, (void*)Param, sizeof(MUINT32)) == 0)
            {
                spin_lock_irqsave(&(IspInfo.SpinLockIrq), flags);
                IspInfo.DebugMask = DebugFlag;
                spin_unlock_irqrestore(&(IspInfo.SpinLockIrq), flags);
            }
            else
            {
                LOG_ERR("copy_from_user failed");
                Ret = -EFAULT;
            }
            break;
        }
#ifdef ISP_KERNEL_MOTIFY_SINGAL_TEST
        case ISP_SET_USER_PID:
        {
            if(copy_from_user(&pid, (void*)Param, sizeof(MUINT32)) == 0)
            {
                spin_lock(&(IspInfo.SpinLockIsp));
                getTaskInfo( (pid_t) pid );

                sendSignal();

                LOG_DBG("[ISP_KERNEL_MOTIFY_SINGAL_TEST]:0x08%x ",pid);
                spin_unlock(&(IspInfo.SpinLockIsp));
            }
            else
            {
                LOG_ERR("copy_from_user failed");
                Ret = -EFAULT;
            }

            break;
        }
#endif
        case ISP_BUFFER_CTRL:
            Ret = ISP_Buf_CTRL_FUNC(Param);
            break;
        case ISP_REF_CNT_CTRL:
            Ret = ISP_REF_CNT_CTRL_FUNC(Param);
            break;

        default:
        {
            LOG_ERR("Unknown Cmd(%d)",Cmd);
            Ret = -EPERM;
            break;
        }
    }
    //
    if(Ret != 0)
    {
        LOG_ERR("Fail, Cmd(%d), Pid(%d), (process, pid, tgid)=(%s, %d, %d)",Cmd, pUserInfo->Pid, current->comm , current->pid, current->tgid);
    }
    //
    return Ret;
}

/*******************************************************************************
*
********************************************************************************/
static MINT32 ISP_open(
    struct inode*   pInode,
    struct file*    pFile)
{
    MINT32 Ret = 0;
    MUINT32 i;
    ISP_USER_INFO_STRUCT* pUserInfo;

    LOG_DBG("- E. UserCount: %d.", IspInfo.UserCount);
    //
    spin_lock(&(IspInfo.SpinLockIspRef));
    //
    //LOG_DBG("UserCount(%d)",IspInfo.UserCount);
    //
    pFile->private_data = NULL;
    pFile->private_data = kmalloc(sizeof(ISP_USER_INFO_STRUCT) , GFP_ATOMIC);
    if(pFile->private_data == NULL)
    {
        LOG_DBG("ERROR: kmalloc failed, (process, pid, tgid)=(%s, %d, %d)", current->comm, current->pid, current->tgid);
        Ret = -ENOMEM;
    }
    else
    {
        pUserInfo = (ISP_USER_INFO_STRUCT*)pFile->private_data;
        pUserInfo->Pid = current->pid;
        pUserInfo->Tid = current->tgid;
    }
    //
    if(IspInfo.UserCount > 0)
    {
        IspInfo.UserCount++;
        LOG_DBG("Curr UserCount(%d), (process, pid, tgid)=(%s, %d, %d), users exist",IspInfo.UserCount,current->comm, current->pid, current->tgid);
        goto EXIT;
    }
    //
    IspInfo.BufInfo.Read.pData = (MUINT8 *) kmalloc(ISP_BUF_SIZE, GFP_ATOMIC);
    IspInfo.BufInfo.Read.Size = ISP_BUF_SIZE;
    IspInfo.BufInfo.Read.Status = ISP_BUF_STATUS_EMPTY;
    if(IspInfo.BufInfo.Read.pData == NULL)
    {
        LOG_DBG("ERROR: BufRead kmalloc failed");
        Ret = -ENOMEM;
        goto EXIT;
    }
    //
    if(!ISP_BufWrite_Alloc())
    {
        LOG_DBG("ERROR: BufWrite kmalloc failed");
        Ret = -ENOMEM;
        goto EXIT;
    }
    //
    atomic_set(&(IspInfo.HoldInfo.HoldEnable), 0);
    atomic_set(&(IspInfo.HoldInfo.WriteEnable), 0);
    for (i=0;i<ISP_REF_CNT_ID_MAX;i++) {
        atomic_set(&g_imem_ref_cnt[i],0);
    }
    // Enable clock
    ISP_EnableClock(MTRUE);
    //
    for(i=0; i<ISP_IRQ_TYPE_AMOUNT; i++)
    {
        IspInfo.IrqInfo.Status[i] = 0;
    }
    for(i=0; i<ISP_CALLBACK_AMOUNT; i++)
    {
        IspInfo.Callback[i].Func = NULL;
    }
    //
    IspInfo.UserCount++;
    LOG_DBG("Curr UserCount(%d), (process, pid, tgid)=(%s, %d, %d), first user",IspInfo.UserCount,current->comm, current->pid, current->tgid);
    //


//js_test
//IspInfo.DebugMask = ISP_DBG_BUF_CTRL;

    //
    EXIT:
    if(Ret < 0)
    {
        if(IspInfo.BufInfo.Read.pData != NULL)
        {
            kfree(IspInfo.BufInfo.Read.pData);
            IspInfo.BufInfo.Read.pData = NULL;
        }
        //
        ISP_BufWrite_Free();
    }
    //
    spin_unlock(&(IspInfo.SpinLockIspRef));
    //

    //LOG_DBG("Before spm_disable_sodi().");
    // Disable sodi (Multi-Core Deep Idle).
    spm_disable_sodi();

    LOG_DBG("- X. Ret: %d. UserCount: %d.", Ret, IspInfo.UserCount);

    return Ret;

}

/*******************************************************************************
*
********************************************************************************/
static MINT32 ISP_release(
    struct inode*   pInode,
    struct file*    pFile)
{
    ISP_USER_INFO_STRUCT* pUserInfo;

    LOG_DBG("- E. UserCount: %d.", IspInfo.UserCount);
    //
    spin_lock(&(IspInfo.SpinLockIspRef));
    //
    //LOG_DBG("UserCount(%d)",IspInfo.UserCount);
    //
    if(pFile->private_data != NULL)
    {
        pUserInfo = (ISP_USER_INFO_STRUCT*)pFile->private_data;
        kfree(pFile->private_data);
        pFile->private_data = NULL;
    }
    //
    IspInfo.UserCount--;
    if(IspInfo.UserCount > 0)
    {
        LOG_DBG("Curr UserCount(%d), (process, pid, tgid)=(%s, %d, %d), users exist",IspInfo.UserCount,current->comm, current->pid, current->tgid);
        goto EXIT;
    }
    //
    LOG_DBG("Curr UserCount(%d), (process, pid, tgid)=(%s, %d, %d), last user",IspInfo.UserCount,current->comm, current->pid, current->tgid);

    // Disable clock.
    ISP_EnableClock(MFALSE);

    if(IspInfo.BufInfo.Read.pData != NULL)
    {
        kfree(IspInfo.BufInfo.Read.pData);
        IspInfo.BufInfo.Read.pData = NULL;
        IspInfo.BufInfo.Read.Size = 0;
        IspInfo.BufInfo.Read.Status = ISP_BUF_STATUS_EMPTY;
    }
    //
    ISP_BufWrite_Free();
    //
    EXIT:
    spin_unlock(&(IspInfo.SpinLockIspRef));
    //
    //LOG_DBG("Before spm_enable_sodi().");
    // Enable sodi (Multi-Core Deep Idle).
    spm_enable_sodi();

    LOG_DBG("- X. UserCount: %d.", IspInfo.UserCount);
    return 0;
}

/*******************************************************************************
*
********************************************************************************/
// helper function, mmap's the kmalloc'd area which is physically contiguous
static MINT32 mmap_kmem(struct file *filp, struct vm_area_struct *vma)
{
        int ret;
        long length = vma->vm_end - vma->vm_start;

        /* check length - do not allow larger mappings than the number of
           pages allocated */
        if (length > RT_BUF_TBL_NPAGES * PAGE_SIZE)
                return -EIO;

        /* map the whole physically contiguous area in one piece */
        if ((ret = remap_pfn_range(vma,
                                   vma->vm_start,
                                   virt_to_phys((void *)pTbl_RTBuf) >> PAGE_SHIFT,
                                   length,
                                   vma->vm_page_prot)) < 0) {
                return ret;
        }

        return 0;
}
/*******************************************************************************
*
********************************************************************************/
static MINT32 ISP_mmap(
    struct file*            pFile,
    struct vm_area_struct*  pVma)
{
    LOG_DBG("- E.");

    /* at offset RT_BUF_TBL_NPAGES we map the kmalloc'd area */
    if (pVma->vm_pgoff == RT_BUF_TBL_NPAGES) {
            return mmap_kmem(pFile, pVma);
    }
    else {
    //
    pVma->vm_page_prot = pgprot_noncached(pVma->vm_page_prot);
    if(remap_pfn_range(pVma, pVma->vm_start, pVma->vm_pgoff,pVma->vm_end - pVma->vm_start, pVma->vm_page_prot))
    {
        return -EAGAIN;
    }
    }
    //
    return 0;
}

/*******************************************************************************
*
********************************************************************************/
static dev_t IspDevNo;
static struct cdev *pIspCharDrv = NULL;
static struct class *pIspClass = NULL;

static const struct file_operations IspFileOper =
{
    .owner   = THIS_MODULE,
    .open    = ISP_open,
    .release = ISP_release,
    //.flush   = mt_isp_flush,
    .mmap    = ISP_mmap,
    .unlocked_ioctl   = ISP_ioctl
};

/*******************************************************************************
*
********************************************************************************/
inline static void ISP_UnregCharDev(void)
{
    LOG_DBG("- E.");
    //
    //Release char driver
    if(pIspCharDrv != NULL)
    {
        cdev_del(pIspCharDrv);
        pIspCharDrv = NULL;
    }
    //
    unregister_chrdev_region(IspDevNo, 1);
}

/*******************************************************************************
*
********************************************************************************/
inline static MINT32 ISP_RegCharDev(void)
{
    MINT32 Ret = 0;
    //
    LOG_DBG("- E.");
    //
    if((Ret = alloc_chrdev_region(&IspDevNo, 0, 1, ISP_DEV_NAME)) < 0)
    {
        LOG_ERR("alloc_chrdev_region failed, %d", Ret);
        return Ret;
    }
    //Allocate driver
    pIspCharDrv = cdev_alloc();
    if(pIspCharDrv == NULL)
    {
        LOG_ERR("cdev_alloc failed");
        Ret = -ENOMEM;
        goto EXIT;
    }
    //Attatch file operation.
    cdev_init(pIspCharDrv, &IspFileOper);
    //
    pIspCharDrv->owner = THIS_MODULE;
    //Add to system
    if((Ret = cdev_add(pIspCharDrv, IspDevNo, 1)) < 0)
    {
        LOG_ERR("Attatch file operation failed, %d", Ret);
        goto EXIT;
    }
    //
    EXIT:
    if(Ret < 0)
    {
        ISP_UnregCharDev();
    }
    //

    LOG_DBG("- X.");
    return Ret;
}

/*******************************************************************************
*
********************************************************************************/
static MINT32 ISP_probe(struct platform_device* pDev)
{
    MINT32 Ret = 0;
    struct resource *pRes = NULL;
    MINT32 i;
    //
    LOG_DBG("- E.");
    // Check platform_device parameters
    if(pDev == NULL)
    {
        dev_err(&pDev->dev, "pDev is NULL");
        return -ENXIO;
    }
    // Register char driver
    if((Ret = ISP_RegCharDev()))
    {
        dev_err(&pDev->dev, "register char failed");
        return Ret;
    }
    // Mapping CAM_REGISTERS
    for(i = 0; i < 1; i++)  // NEED_TUNING_BY_CHIP. 1: Only one IORESOURCE_MEM type resource in kernel\mt_devs.c\mt_resource_isp[].
    {
        LOG_DBG("Mapping CAM_REGISTERS. i: %d.", i);
        pRes = platform_get_resource(pDev, IORESOURCE_MEM, i);
        if(pRes == NULL)
        {
            dev_err(&pDev->dev, "platform_get_resource failed");
            Ret = -ENOMEM;
            goto EXIT;
        }
        pRes = request_mem_region(pRes->start, pRes->end - pRes->start + 1, pDev->name);
        if(pRes == NULL)
        {
            dev_err(&pDev->dev, "request_mem_region failed");
            Ret = -ENOMEM;
            goto EXIT;
        }
    }
    // Create class register
    pIspClass = class_create(THIS_MODULE, "ispdrv");
    if(IS_ERR(pIspClass))
    {
        Ret = PTR_ERR(pIspClass);
        LOG_ERR("Unable to create class, err = %d", Ret);
        return Ret;
    }
    // FIXME: error handling
    device_create(pIspClass, NULL, IspDevNo, NULL, ISP_DEV_NAME);
    //
    init_waitqueue_head(&IspInfo.WaitQueueHead);
    //
    INIT_WORK(&IspInfo.ScheduleWorkVD,       ISP_ScheduleWork_VD);
    INIT_WORK(&IspInfo.ScheduleWorkEXPDONE,  ISP_ScheduleWork_EXPDONE);
    //
    spin_lock_init(&(IspInfo.SpinLockIspRef));
    spin_lock_init(&(IspInfo.SpinLockIsp));
    spin_lock_init(&(IspInfo.SpinLockIrq));
    spin_lock_init(&(IspInfo.SpinLockHold));
    spin_lock_init(&(IspInfo.SpinLockRTBC));
    //
    IspInfo.UserCount = 0;
    IspInfo.HoldInfo.Time = ISP_HOLD_TIME_EXPDONE;
    //
    IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INT] = ISP_REG_MASK_INT_STATUS;
    IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMA] = ISP_REG_MASK_DMA_INT;
    IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INTB] = ISP_REG_MASK_INTB_STATUS;
    IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMAB] = ISP_REG_MASK_DMAB_INT;
    IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INTC] = ISP_REG_MASK_INTC_STATUS;
    IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMAC] = ISP_REG_MASK_DMAC_INT;
    IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INTX] = ISP_REG_MASK_INTX_STATUS;
    IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMAX] = ISP_REG_MASK_DMAX_INT;
    //
    IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_INT] = ISP_REG_MASK_INT_STATUS_ERR;
    IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_DMA] = ISP_REG_MASK_DMA_INT_ERR;
    IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_INTB] = ISP_REG_MASK_INTB_STATUS_ERR;
    IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_DMAB] = ISP_REG_MASK_DMAB_INT_ERR;
    IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_INTC] = ISP_REG_MASK_INTC_STATUS_ERR;
    IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_DMAC] = ISP_REG_MASK_DMAC_INT_ERR;
    IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_INTX] = ISP_REG_MASK_INTX_STATUS_ERR;
    IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_DMAX] = ISP_REG_MASK_DMAX_INT_ERR;
    //

    // Request CAM_ISP IRQ
    #if 1 // FIXME
    if (request_irq(CAMERA_ISP_IRQ0_ID, (irq_handler_t)ISP_Irq, IRQF_TRIGGER_LOW , "isp", NULL))
//    if (request_irq(CAMERA_ISP_IRQ0_ID, (irq_handler_t)ISP_Irq, IRQF_TRIGGER_HIGH, "isp", NULL))
    {
        LOG_ERR("MT6589_CAM_IRQ_LINE IRQ LINE NOT AVAILABLE!!");
        goto EXIT;
    }
    //mt_irq_unmask(CAMERA_ISP_IRQ0_ID);
    #endif

    EXIT:
    if(Ret < 0)
    {
        ISP_UnregCharDev();
    }
    //
    LOG_DBG("- X.");
    //
    return Ret;
}

/*******************************************************************************
* Called when the device is being detached from the driver
********************************************************************************/
static MINT32 ISP_remove(struct platform_device *pDev)
{
    struct resource *pRes;
    MINT32 i;
    MINT32 IrqNum;
    //
    LOG_DBG("- E.");
    // unregister char driver.
    ISP_UnregCharDev();
    // unmaping ISP CAM_REGISTER registers
    for(i = 0; i < 2; i++)
    {
        pRes = platform_get_resource(pDev, IORESOURCE_MEM, 0);
        release_mem_region(pRes->start, (pRes->end - pRes->start + 1));
    }
    // Release IRQ
    disable_irq(IspInfo.IrqNum);
    IrqNum = platform_get_irq(pDev, 0);
    free_irq(IrqNum , NULL);
    //
    device_destroy(pIspClass, IspDevNo);
    //
    class_destroy(pIspClass);
    pIspClass = NULL;
    //
    return 0;
}

/*******************************************************************************
*
********************************************************************************/
static MINT32 bPass1_On_In_Resume_TG1 = 0;
static MINT32 bPass1_On_In_Resume_TG2 = 0;
static MINT32 ISP_suspend(
    struct platform_device* pDev,
    pm_message_t            Mesg
)
{
    // TG_VF_CON[0] (0x15004414[0]): VFDATA_EN. TG1 Take Picture Request.
    MUINT32 regTG1Val = ISP_RD32(ISP_ADDR + 0x414);
    // TG2_VF_CON[0] (0x150044B4[0]): VFDATA_EN. TG2 Take Picture Request.
    MUINT32 regTG2Val = ISP_RD32(ISP_ADDR + 0x4B4);

    LOG_DBG("bPass1_On_In_Resume_TG1(%d). bPass1_On_In_Resume_TG2(%d). regTG1Val(0x%08x). regTG2Val(0x%08x) \n", bPass1_On_In_Resume_TG1, bPass1_On_In_Resume_TG2, regTG1Val, regTG2Val);

    bPass1_On_In_Resume_TG1 = 0;
    if ( regTG1Val & 0x01 )    // For TG1 Main sensor.
    {
        bPass1_On_In_Resume_TG1 = 1;
        ISP_WR32(ISP_ADDR + 0x414, (regTG1Val&(~0x01)) );
    }

    bPass1_On_In_Resume_TG2 = 0;
    if ( regTG2Val & 0x01 )    // For TG2 Sub sensor.
    {
        bPass1_On_In_Resume_TG2 = 1;
        ISP_WR32(ISP_ADDR + 0x4B4, (regTG2Val&(~0x01)) );
    }

    return 0;
}

/*******************************************************************************
*
********************************************************************************/
static MINT32 ISP_resume(struct platform_device* pDev)
{
    // TG_VF_CON[0] (0x15004414[0]): VFDATA_EN. TG1 Take Picture Request.
    MUINT32 regTG1Val = ISP_RD32(ISP_ADDR + 0x414);
    // TG2_VF_CON[0] (0x150044B4[0]): VFDATA_EN. TG2 Take Picture Request.
    MUINT32 regTG2Val = ISP_RD32(ISP_ADDR + 0x4B4);

    LOG_DBG("bPass1_On_In_Resume_TG1(%d). bPass1_On_In_Resume_TG2(%d). regTG1Val(0x%x) regTG2Val(0x%x) \n", bPass1_On_In_Resume_TG1, bPass1_On_In_Resume_TG2, regTG1Val, regTG2Val);

    if ( bPass1_On_In_Resume_TG1 ) {
        bPass1_On_In_Resume_TG1 = 0;
        ISP_WR32(ISP_ADDR + 0x414, (regTG1Val|0x01) );    // For TG1 Main sensor.
    }

    if ( bPass1_On_In_Resume_TG2 ) {
        bPass1_On_In_Resume_TG2 = 0;
        ISP_WR32(ISP_ADDR + 0x4B4, (regTG2Val|0x01) );    // For TG2 Sub sensor.
    }

    return 0;
}

/*---------------------------------------------------------------------------*/
#ifdef CONFIG_PM
/*---------------------------------------------------------------------------*/
int ISP_pm_suspend(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);
    BUG_ON(pdev == NULL);

    pr_debug("calling %s()\n", __func__);

    return ISP_suspend(pdev, PMSG_SUSPEND);
}

int ISP_pm_resume(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);
    BUG_ON(pdev == NULL);

    pr_debug("calling %s()\n", __func__);

    return ISP_resume(pdev);
}

extern void mt_irq_set_sens(unsigned int irq, unsigned int sens);
extern void mt_irq_set_polarity(unsigned int irq, unsigned int polarity);
int ISP_pm_restore_noirq(struct device *device)
{
    pr_debug("calling %s()\n", __func__);

    mt_irq_set_sens(CAMERA_ISP_IRQ0_ID, MT65xx_LEVEL_SENSITIVE);
    mt_irq_set_polarity(CAMERA_ISP_IRQ0_ID, MT65xx_POLARITY_LOW);

    return 0;

}
/*---------------------------------------------------------------------------*/
#else /*CONFIG_PM*/
/*---------------------------------------------------------------------------*/
#define ISP_pm_suspend NULL
#define ISP_pm_resume  NULL
#define ISP_pm_restore_noirq NULL
/*---------------------------------------------------------------------------*/
#endif /*CONFIG_PM*/
/*---------------------------------------------------------------------------*/
struct dev_pm_ops ISP_pm_ops = {
    .suspend = ISP_pm_suspend,
    .resume = ISP_pm_resume,
    .freeze = ISP_pm_suspend,
    .thaw = ISP_pm_resume,
    .poweroff = ISP_pm_suspend,
    .restore = ISP_pm_resume,
    .restore_noirq = ISP_pm_restore_noirq,
};


/*******************************************************************************
*
********************************************************************************/
static struct platform_driver IspDriver =
{
    .probe   = ISP_probe,
    .remove  = ISP_remove,
    .suspend = ISP_suspend,
    .resume  = ISP_resume,
    .driver  = {
        .name  = ISP_DEV_NAME,
        .owner = THIS_MODULE,
#ifdef CONFIG_PM
        .pm     = &ISP_pm_ops,
#endif
    }
};

/*******************************************************************************
*
********************************************************************************/
static MINT32 ISP_DumpRegToProc(
    char*   pPage,
    char**  ppStart,
    off_t   off,
    MINT32  Count,
    MINT32* pEof,
    void*   pData)
{
    char *p = pPage;
    MINT32 Length = 0;
    MUINT32 i = 0;
    MINT32 ret = 0;
    //
    LOG_DBG("- E. pPage: 0x%08x. off: %d. Count: %d.", (unsigned int)pPage, (unsigned int)off, Count);
    //
    p += sprintf(p, " MT6589 ISP Register\n");
    p += sprintf(p,"====== top ====\n");
    for(i = 0x0; i <= 0x1AC; i += 4)
    {
        p += sprintf(p,"+0x%08x 0x%08x\n", ISP_ADDR + i, ISP_RD32(ISP_ADDR + i));
    }
    p += sprintf(p,"====== dma ====\n");
    for(i = 0x200; i <= 0x3D8; i += 4)
    {
        p += sprintf(p,"+0x%08x 0x%08x\n\r", ISP_ADDR + i, ISP_RD32(ISP_ADDR + i));
    }
    p += sprintf(p,"====== tg ====\n");
    for(i = 0x400; i <= 0x4EC; i += 4)
    {
        p += sprintf(p,"+0x%08x 0x%08x\n", ISP_ADDR + i, ISP_RD32(ISP_ADDR + i));
    }
    p += sprintf(p,"====== cdp (including EIS) ====\n");
    for(i = 0xB00; i <= 0xDE0; i += 4)
    {
        p += sprintf(p,"+0x%08x 0x%08x\n", ISP_ADDR + i, ISP_RD32(ISP_ADDR + i));
    }
    p += sprintf(p,"====== seninf ====\n");
    for(i = 0x4000; i <= 0x40C0; i += 4)
    {
        p += sprintf(p,"+0x%08x 0x%08x\n", ISP_ADDR + i, ISP_RD32(ISP_ADDR + i));
    }
    for(i = 0x4100; i <= 0x41BC; i += 4)
    {
        p += sprintf(p,"+0x%08x 0x%08x\n", ISP_ADDR + i, ISP_RD32(ISP_ADDR + i));
    }
    for(i = 0x4300; i <= 0x4310; i += 4)
    {
        p += sprintf(p,"+0x%08x 0x%08x\n", ISP_ADDR + i, ISP_RD32(ISP_ADDR + i));
    }
    for(i = 0x43A0; i <= 0x43B0; i += 4)
    {
        p += sprintf(p,"+0x%08x 0x%08x\n", ISP_ADDR + i, ISP_RD32(ISP_ADDR + i));
    }
    for(i = 0x4400; i <= 0x4424; i += 4)
    {
        p += sprintf(p,"+0x%08x 0x%08x\n", ISP_ADDR + i, ISP_RD32(ISP_ADDR + i));
    }
    for(i = 0x4500; i <= 0x4520; i += 4)
    {
        p += sprintf(p,"+0x%08x 0x%08x\n", ISP_ADDR + i, ISP_RD32(ISP_ADDR + i));
    }
     p += sprintf(p,"====== 3DNR ====\n");
    for(i = 0x4F00; i <= 0x4F38; i += 4)
    {
        p += sprintf(p,"+0x%08x 0x%08x\n", ISP_ADDR + i, ISP_RD32(ISP_ADDR + i));
    }
    //
    *ppStart = pPage + off;
    //
    Length = p - pPage;
    if(Length > off)
    {
        Length -= off;
    }
    else
    {
        Length = 0;
    }
    //

    ret = Length < Count ? Length : Count;

    LOG_DBG("- X. ret: %d.", ret);

    return ret;
}

/*******************************************************************************
*
********************************************************************************/
static MINT32  ISP_RegDebug(
    struct file*    pFile,
    const char*     pBuffer,
    unsigned long   Count,
    void*           pData)
{
    char RegBuf[64];
    MUINT32 CopyBufSize = (Count < (sizeof(RegBuf) - 1)) ? (Count) : (sizeof(RegBuf) - 1);
    MUINT32 Addr = 0;
    MUINT32 Data = 0;

    LOG_DBG("- E. pFile: 0x%08x. pBuffer: 0x%08x. Count: %d.", (unsigned int)pFile, (unsigned int)pBuffer, (int)Count);
    //
    if(copy_from_user(RegBuf, pBuffer, CopyBufSize))
    {
        LOG_ERR("copy_from_user() fail.");
        return -EFAULT;
    }

    //
    if (sscanf(RegBuf, "%x %x",  &Addr, &Data) == 2)
    {
        ISP_WR32(Data, ISP_ADDR_CAMINF + Addr);
        LOG_DBG("Write => Addr: 0x%08X, Write Data: 0x%08X. Read Data: 0x%08X.", ISP_ADDR_CAMINF + Addr, Data, ioread32(ISP_ADDR_CAMINF + Addr));
    }
    else if (sscanf(RegBuf, "%x", &Addr) == 1)
    {
        LOG_DBG("Read => Addr: 0x%08X, Read Data: 0x%08X.", ISP_ADDR_CAMINF + Addr, ioread32(ISP_ADDR_CAMINF + Addr));
    }
    //
    LOG_DBG("- X. Count: %d.", (int)Count);
    return Count;
}

static MUINT32 proc_regOfst = 0;
static MINT32 CAMIO_DumpRegToProc(
    char*   pPage,
    char**  ppStart,
    off_t   off,
    MINT32  Count,
    MINT32* pEof,
    void*   pData)
{
    char *p = pPage;
    MINT32 Length = 0;
    MINT32 ret = 0;
    //
    LOG_DBG("- E. pPage: 0x%08x. off: %d. Count: %d.", (unsigned int)pPage, (int)off, Count);
    //
    p += sprintf(p,"reg_0x%08X = 0x%X \n", ISP_ADDR_CAMINF+proc_regOfst , ioread32(ISP_ADDR_CAMINF + proc_regOfst));

    *ppStart = pPage + off;
    //
    Length = p - pPage;
    if(Length > off)
    {
        Length -= off;
    }
    else
    {
        Length = 0;
    }
    //

    ret = Length < Count ? Length : Count;

    LOG_DBG("- X. ret: %d.", ret);

    return ret;
}

/*******************************************************************************
*
********************************************************************************/
static MINT32  CAMIO_RegDebug(
    struct file*    pFile,
    const char*     pBuffer,
    unsigned long   Count,
    void*           pData)
{
    char RegBuf[64];
    MUINT32 CopyBufSize = (Count < (sizeof(RegBuf) - 1)) ? (Count) : (sizeof(RegBuf) - 1);
    MUINT32 Addr = 0;
    MUINT32 Data = 0;
    LOG_DBG("- E. pFile: 0x%08x. pBuffer: 0x%08x. Count: %d.", (unsigned int)pFile, (unsigned int)pBuffer, (int)Count);

    //
    if(copy_from_user(RegBuf, pBuffer, CopyBufSize))
    {
        LOG_ERR("copy_from_user() fail.");
        return -EFAULT;
    }

    //
    if (sscanf(RegBuf, "%x %x",  &Addr, &Data) == 2)
    {
        proc_regOfst = Addr;
        ISP_WR32(Data, GPIO_BASE + Addr);
        LOG_DBG("Write => Addr: 0x%08X, Write Data: 0x%08X. Read Data: 0x%08X.", GPIO_BASE + Addr, Data, ioread32(GPIO_BASE + Addr));
    }
    else if (sscanf(RegBuf, "%x", &Addr) == 1)
    {
        proc_regOfst = Addr;
        LOG_DBG("Read => Addr: 0x%08X, Read Data: 0x%08X.", GPIO_BASE + Addr, ioread32(GPIO_BASE + Addr));
    }
    //
    LOG_DBG("- X. Count: %d.", (int)Count);
    return Count;
}
/*******************************************************************************
*
********************************************************************************/
static MINT32 __init ISP_Init(void)
{
    MINT32 Ret = 0;
    struct proc_dir_entry*  pEntry;
    int i;
    //
    LOG_DBG("- E.");
    //
    if((Ret = platform_driver_register(&IspDriver)) < 0)
    {
        LOG_ERR("platform_driver_register fail");
        return Ret;
    }
    //
    pEntry = create_proc_entry("driver/isp_reg", 0, NULL);
    if(pEntry)
    {
        pEntry->read_proc = ISP_DumpRegToProc;
        pEntry->write_proc = ISP_RegDebug;
    }
    else
    {
        LOG_ERR("add /proc/driver/isp_reg entry fail");
    }

    pEntry = create_proc_entry("driver/camio_reg", 0, NULL);
    if(pEntry)
    {
        pEntry->read_proc = CAMIO_DumpRegToProc;
        pEntry->write_proc = CAMIO_RegDebug;
    }
    else
    {
        LOG_ERR("add /proc/driver/camio_reg entry fail");
    }
    //
    /* allocate a memory area with kmalloc. Will be rounded up to a page boundary */
    //RT_BUF_TBL_NPAGES*4096(1page) = 64k Bytes
    if ((pBuf_kmalloc = kmalloc((RT_BUF_TBL_NPAGES + 2) * PAGE_SIZE, GFP_KERNEL)) == NULL) {
            return -ENOMEM;

    }
    memset(pBuf_kmalloc,0x00,RT_BUF_TBL_NPAGES*PAGE_SIZE);
    /* round it up to the page bondary */
    pTbl_RTBuf = (int *)((((unsigned long)pBuf_kmalloc) + PAGE_SIZE - 1) & PAGE_MASK);
    pstRTBuf = (ISP_RT_BUF_STRUCT*)pTbl_RTBuf;
    pstRTBuf->state = ISP_RTBC_STATE_INIT;
    /* mark the pages reserved */
    for (i = 0; i < RT_BUF_TBL_NPAGES * PAGE_SIZE; i+= PAGE_SIZE) {
        SetPageReserved(virt_to_page(((unsigned long)pTbl_RTBuf) + i));
    }
    //
    LOG_DBG("- X. Ret: %d.", Ret);
    return Ret;
}

/*******************************************************************************
*
********************************************************************************/
static void __exit ISP_Exit(void)
{
    int i;
    LOG_DBG("- E.");
    //
    platform_driver_unregister(&IspDriver);

    //
    /* unreserve the pages */
    for (i = 0; i < RT_BUF_TBL_NPAGES * PAGE_SIZE; i+= PAGE_SIZE) {
            SetPageReserved(virt_to_page(((unsigned long)pTbl_RTBuf) + i));
    }
    /* free the memory areas */
    kfree(pBuf_kmalloc);
    //
}

/*******************************************************************************
*
********************************************************************************/
MBOOL ISP_RegCallback(ISP_CALLBACK_STRUCT* pCallback)
{
    //
    if(pCallback == NULL)
    {
        LOG_ERR("pCallback is null");
        return MFALSE;
    }
    //
    if(pCallback->Func == NULL)
    {
        LOG_ERR("Func is null");
        return MFALSE;
    }
    //
    LOG_DBG("Type(%d)",pCallback->Type);
    IspInfo.Callback[pCallback->Type].Func = pCallback->Func;
    //
    return MTRUE;
}

/*******************************************************************************
*
********************************************************************************/
MBOOL ISP_UnregCallback(ISP_CALLBACK_ENUM   Type)
{
    if(Type > ISP_CALLBACK_AMOUNT)
    {
        LOG_ERR("Type(%d) must smaller than %d",Type,ISP_CALLBACK_AMOUNT);
        return MFALSE;
    }
    //
    LOG_DBG("Type(%d)",Type);
    IspInfo.Callback[Type].Func = NULL;
    //
    return MTRUE;
}

void ISP_MCLK1_EN(MBOOL En)
{
    MUINT32 temp=0;
    temp = ISP_RD32(ISP_ADDR + 0x4300);
    if(En)
    {
        temp |= 0x20000000;
        ISP_WR32(ISP_ADDR + 0x4300,temp);       
    }
    else
    {
        temp &= 0xDFFFFFFF;
        ISP_WR32(ISP_ADDR + 0x4300,temp);
    }    

}

void ISP_MCLK2_EN(MBOOL En)
{
    MUINT32 temp=0;
    temp = ISP_RD32(ISP_ADDR + 0x43A0);
    if(En)
    {
        temp |= 0x20000000;
        ISP_WR32(ISP_ADDR + 0x43A0,temp);       
    }
    else
    {
        temp &= 0xDFFFFFFF;
        ISP_WR32(ISP_ADDR + 0x43A0,temp);
    }    

}
/*******************************************************************************
*
********************************************************************************/
module_init(ISP_Init);
module_exit(ISP_Exit);
MODULE_DESCRIPTION("Camera ISP driver");
MODULE_AUTHOR("ME3");
MODULE_LICENSE("GPL");
EXPORT_SYMBOL(ISP_RegCallback);
EXPORT_SYMBOL(ISP_UnregCallback);
EXPORT_SYMBOL(ISP_MCLK1_EN);
EXPORT_SYMBOL(ISP_MCLK2_EN);




