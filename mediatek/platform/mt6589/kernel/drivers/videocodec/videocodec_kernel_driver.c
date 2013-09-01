#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/mm_types.h>
#include <linux/mm.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <asm/page.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
#include <mach/irqs.h>
#include <linux/wait.h>
#include <linux/proc_fs.h>
#include <linux/semaphore.h>
#include <linux/android_pmem.h>
#include <mach/dma.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include "mach/sync_write.h"
#include "mach/mt_reg_base.h"
#include "mach/mt_clkmgr.h"
#ifdef CONFIG_MTK_HIBERNATION
#include "mach/mtk_hibernate_dpm.h"
#endif

#include "videocodec_kernel_driver.h"

#include <asm/cacheflush.h>
#include <asm/io.h>
#include "val_types.h"
#include "hal_types.h"
#include "val_api.h"
#include "val_log.h"
#include "drv_api.h"

#define VDO_HW_WRITE(ptr,data)     mt65xx_reg_sync_writel(data,ptr)
#define VDO_HW_READ(ptr)           (*((volatile unsigned int * const)(ptr)))

#define VCODEC_DEVNAME     "Vcodec"
#define MT6589_VCODEC_DEV_MAJOR_NUMBER 160   //189
#define VENC_IRQ_RET_REG_NUM   3  //mpeg4 enc
#define VDEC_IRQ_RET_REG_NUM   0  //h264 dec, vp8 dec

static dev_t vcodec_devno = MKDEV(MT6589_VCODEC_DEV_MAJOR_NUMBER,0);
static struct cdev *vcodec_cdev;

static DEFINE_MUTEX(IsOpenedLock);
static DEFINE_MUTEX(PWRLock);
static DEFINE_MUTEX(VdecHWLock);
static DEFINE_MUTEX(VencHWLock);
static DEFINE_MUTEX(EncEMILock);
static DEFINE_MUTEX(DecEMILock);
static DEFINE_MUTEX(DriverOpenCountLock);
static DEFINE_MUTEX(NonCacheMemoryListLock);
static DEFINE_MUTEX(DecHWLockEventTimeoutLock);
static DEFINE_MUTEX(EncHWLockEventTimeoutLock);

static DEFINE_SPINLOCK(OalHWContextLock);
static DEFINE_SPINLOCK(DecIsrLock);
static DEFINE_SPINLOCK(EncIsrLock);
static DEFINE_SPINLOCK(LockDecHWCountLock);
static DEFINE_SPINLOCK(LockEncHWCountLock);
static DEFINE_SPINLOCK(DecISRCountLock);
static DEFINE_SPINLOCK(EncISRCountLock);


static VAL_EVENT_T DecHWLockEvent;    //mutex : HWLockEventTimeoutLock
static VAL_EVENT_T EncHWLockEvent;    //mutex : HWLockEventTimeoutLock
static VAL_EVENT_T DecIsrEvent;    //mutex : HWLockEventTimeoutLock
static VAL_EVENT_T EncIsrEvent;    //mutex : HWLockEventTimeoutLock
static int MT6589Driver_Open_Count;         //mutex : DriverOpenCountLock
static VAL_UINT32_T gu4PWRCounter = 0;      //mutex : PWRLock
static VAL_UINT32_T gu4EncEMICounter = 0;   //mutex : EncEMILock
static VAL_UINT32_T gu4DecEMICounter = 0;   //mutex : DecEMILock
static VAL_BOOL_T bIsOpened = VAL_FALSE;    //mutex : IsOpenedLock
static VAL_UINT32_T gu4HwVencIrqStatus = 0; //hardware VENC IRQ status (VP8/H264)

//#define MT6589_MFV_DEBUG
#ifdef MT6589_MFV_DEBUG
#undef MFV_DEBUG
#define MFV_DEBUG MFV_LOGE
#undef MFV_LOGD
#define MFV_LOGD  MFV_LOGE
#else
#define MFV_DEBUG(...)
#undef MFV_LOGD
#define MFV_LOGD(...)
#endif

// VENC physical base address
#undef VENC_BASE
#define VENC_BASE       0x17002000
#define VENC_IRQ_STATUS_addr        VENC_BASE + 0x05C
#define VENC_IRQ_ACK_addr           VENC_BASE + 0x060
#define VENC_MP4_IRQ_ACK_addr       VENC_BASE + 0x678
#define VENC_MP4_IRQ_STATUS_addr    VENC_BASE + 0x67C
#define VENC_ZERO_COEF_COUNT_addr   VENC_BASE + 0x688
#define VENC_BYTE_COUNT_addr        VENC_BASE + 0x680
#define VENC_MP4_IRQ_ENABLE_addr    VENC_BASE + 0x668

#define VENC_MP4_STATUS_addr        VENC_BASE + 0x664
#define VENC_MP4_MVQP_STATUS_addr   VENC_BASE + 0x6E4

#define VENC_IRQ_STATUS_SPS         0x1
#define VENC_IRQ_STATUS_PPS         0x2
#define VENC_IRQ_STATUS_FRM         0x4
#define VENC_IRQ_STATUS_DRAM        0x8
#define VENC_IRQ_STATUS_PAUSE       0x10
#define VENC_IRQ_STATUS_DRAM_VP8    0x20

//#define VENC_PWR_FPGA
// Cheng-Jung 20120621 VENC power physical base address (FPGA only, should use API) [
#ifdef VENC_PWR_FPGA
#define CLK_CFG_0_addr      0x10000140
#define CLK_CFG_4_addr      0x10000150
#define VENC_PWR_addr       0x10006230
#define VENCSYS_CG_SET_addr 0x17000004

#define PWR_ONS_1_D     3
#define PWR_CKD_1_D     4
#define PWR_ONN_1_D     2
#define PWR_ISO_1_D     1
#define PWR_RST_0_D     0

#define PWR_ON_SEQ_0    ((0x1 << PWR_ONS_1_D) | (0x1 << PWR_CKD_1_D) | (0x1 << PWR_ONN_1_D) | (0x1 << PWR_ISO_1_D) | (0x0 << PWR_RST_0_D))
#define PWR_ON_SEQ_1    ((0x1 << PWR_ONS_1_D) | (0x0 << PWR_CKD_1_D) | (0x1 << PWR_ONN_1_D) | (0x1 << PWR_ISO_1_D) | (0x0 << PWR_RST_0_D))
#define PWR_ON_SEQ_2    ((0x1 << PWR_ONS_1_D) | (0x0 << PWR_CKD_1_D) | (0x1 << PWR_ONN_1_D) | (0x0 << PWR_ISO_1_D) | (0x0 << PWR_RST_0_D))
#define PWR_ON_SEQ_3    ((0x1 << PWR_ONS_1_D) | (0x0 << PWR_CKD_1_D) | (0x1 << PWR_ONN_1_D) | (0x0 << PWR_ISO_1_D) | (0x1 << PWR_RST_0_D))
// ]
#endif

// VDEC virtual base address
#define VDEC_MISC_BASE  VDEC_BASE + 0x0000
#define VDEC_VLD_BASE   VDEC_BASE + 0x1000

int KVA_VENC_IRQ_ACK_ADDR, KVA_VENC_MP4_IRQ_ACK_ADDR;
int KVA_VENC_IRQ_STATUS_ADDR, KVA_VENC_MP4_IRQ_STATUS_ADDR, KVA_VENC_ZERO_COEF_COUNT_ADDR, KVA_VENC_BYTE_COUNT_ADDR;
int KVA_VENC_MP4_IRQ_ENABLE_ADDR;
#ifdef VENC_PWR_FPGA
// Cheng-Jung 20120621 VENC power physical base address (FPGA only, should use API) [
int KVA_VENC_CLK_CFG_0_ADDR, KVA_VENC_CLK_CFG_4_ADDR, KVA_VENC_PWR_ADDR, KVA_VENCSYS_CG_SET_ADDR;
// ]
#endif

// DEBUG
//#define DEBUG_MP4_ENC
#ifdef DEBUG_MP4_ENC
int KVA_VENC_MP4_STATUS_ADDR, KVA_VENC_MP4_MVQP_STATUS_ADDR;
#endif

//#define DEBUG_INFRA_POWER
#ifdef DEBUG_INFRA_POWER
#define INFRA_addr  0x1000015C
#define CHECK_1_addr 0x10006234
#define CHECK_2_addr 0x10006210

int KVA_INFRA_ADDR;
int KVA_CHECK_1_ADDR, KVA_CHECK_2_ADDR;
#endif

extern unsigned int pmem_user_v2p_video(unsigned int va);


void vdec_power_on(void)
{
    // Central power on
    enable_clock(MT_CG_VDEC0_VDE, "VDEC");
    enable_clock(MT_CG_VDEC1_SMI, "VDEC");
}

void vdec_power_off(void)
{
    // Central power off
    disable_clock(MT_CG_VDEC0_VDE, "VDEC");
    disable_clock(MT_CG_VDEC1_SMI, "VDEC");
}

void venc_power_on(void)
{
    MFV_LOGD("venc_power_on +\n");
#ifdef VENC_PWR_FPGA
    // Cheng-Jung 20120621 VENC power physical base address (FPGA only, should use API) [
    //VDO_HW_WRITE(KVA_VENC_CLK_CFG_0_ADDR, ((VDO_HW_READ(KVA_VENC_CLK_CFG_0_ADDR) & 0xfffffff8) | 0x00000001));
    //VDO_HW_WRITE(KVA_VENC_CLK_CFG_0_ADDR, ((VDO_HW_READ(KVA_VENC_CLK_CFG_0_ADDR) & 0xfffff8ff) | 0x00000100));
    //VDO_HW_WRITE(KVA_VENC_CLK_CFG_4_ADDR, ((VDO_HW_READ(KVA_VENC_CLK_CFG_4_ADDR) & 0xffff00ff) | 0x00000600));

    // MTCMOS on
    VDO_HW_WRITE(KVA_VENC_PWR_ADDR, ((VDO_HW_READ(KVA_VENC_PWR_ADDR) & 0xffffffc0) | PWR_ON_SEQ_0));
    VDO_HW_WRITE(KVA_VENC_PWR_ADDR, ((VDO_HW_READ(KVA_VENC_PWR_ADDR) & 0xffffffc0) | PWR_ON_SEQ_1));
    VDO_HW_WRITE(KVA_VENC_PWR_ADDR, ((VDO_HW_READ(KVA_VENC_PWR_ADDR) & 0xffffffc0) | PWR_ON_SEQ_2));
    VDO_HW_WRITE(KVA_VENC_PWR_ADDR, ((VDO_HW_READ(KVA_VENC_PWR_ADDR) & 0xffffffc0) | PWR_ON_SEQ_3));

    // CG (clock gate) on
    VDO_HW_WRITE(KVA_VENCSYS_CG_SET_ADDR, 0x00000001);
    // ]
#else
    enable_clock(MT_CG_VENC_VEN , "VENC");
#endif
    // Enable MPEG4 IRQ
    MFV_LOGD("Enable MP4 IRQ\n");
    VDO_HW_WRITE(KVA_VENC_MP4_IRQ_ENABLE_ADDR, 0x1);

    MFV_LOGD("venc_power_on -\n");
}

void venc_power_off(void)
{
    MFV_LOGD("venc_power_off +\n");
    disable_clock(MT_CG_VENC_VEN, "VENC");
    MFV_LOGD("venc_power_off -\n");
}


void dec_isr(void)
{
    VAL_RESULT_T  eValRet;
    unsigned long ulFlags, ulFlagsISR, ulFlagsLockHW;

    VAL_UINT32_T u4TempDecISRCount = 0;
    VAL_UINT32_T u4TempLockDecHWCount = 0;
    VAL_UINT32_T u4CgStatus = 0;
    VAL_UINT32_T u4DecDoneStatus = 0;
#ifdef DEBUG_INFRA_POWER
    VAL_UINT32_T u4Infra = 0;
    VAL_UINT32_T u4Val = 0; 
    //----------------------
    u4Infra = VDO_HW_READ(KVA_INFRA_ADDR);
    if ((u4Infra & (0x1 << 15)) != 0)
    {
        MFV_LOGE("[MFV][ERROR] Entering ISR, but Infra is not 0x0 (0x%08x)(0x%08x)", u4Infra & ((0x1 << 15)), u4Infra);
    }    
#endif
    u4CgStatus = VDO_HW_READ(0xF6000000);
    if ((u4CgStatus & 0x10) != 0)
    {
        MFV_LOGE("[MFV][ERROR] DEC ISR, VDEC active is not 0x0 (0x%08x)", u4CgStatus);
        return;
    }
    
    u4DecDoneStatus = VDO_HW_READ(0xF60200A4);
    if ((u4DecDoneStatus & (0x1 << 16)) != 0x10000)
    {
        MFV_LOGE("[MFV][ERROR] DEC ISR, Decode done status is not 0x1 (0x%08x)", u4DecDoneStatus);
        return;
    }
    
    
    spin_lock_irqsave(&DecISRCountLock, ulFlagsISR);
    gu4DecISRCount++;
    u4TempDecISRCount = gu4DecISRCount;
    spin_unlock_irqrestore(&DecISRCountLock, ulFlagsISR);

    spin_lock_irqsave(&LockDecHWCountLock, ulFlagsLockHW);
    u4TempLockDecHWCount = gu4LockDecHWCount;
    spin_unlock_irqrestore(&LockDecHWCountLock, ulFlagsLockHW);

    if (u4TempDecISRCount != u4TempLockDecHWCount)
    {
        //MFV_LOGE("[INFO] Dec ISRCount: 0x%x, LockHWCount:0x%x\n", u4TempDecISRCount, u4TempLockDecHWCount);
    }

    // Clear interrupt 
    VDO_HW_WRITE(VDEC_MISC_BASE+41*4, VDO_HW_READ(VDEC_MISC_BASE + 41*4) | 0x11);
    VDO_HW_WRITE(VDEC_MISC_BASE+41*4, VDO_HW_READ(VDEC_MISC_BASE + 41*4) & ~0x10);
#ifdef DEBUG_INFRA_POWER
    u4CgStatus = VDO_HW_READ(0xF6000000);
    if (u4CgStatus != 0x1)
    {
        MFV_LOGE("[MFV][ERROR] Entering ISR, but CG status is not 0x1 (0x%08x), mid!...", u4CgStatus);
        //return;
    }

    u4Val = VDO_HW_READ(0xF60200C8);
    MFV_LOGE("[MFV][ERROR] Read MISC_50 0xF60200C8 = 0x%08x", u4Val);
    u4Val = VDO_HW_READ(0xF60200CC);
    MFV_LOGE("[MFV][ERROR] Read MISC_51 0xF60200CC = 0x%08x", u4Val);

    u4Val = VDO_HW_READ(0xF6024250);
    MFV_LOGE("[MFV][ERROR] Intra MV count Read 0xF6024250 = 0x%08x", u4Val);
    u4Val = VDO_HW_READ(0xF60213CC);
    MFV_LOGE("[MFV][ERROR] Error check Read 0xF60213CC = 0x%08x", u4Val);
    u4Val = VDO_HW_READ(0xF602000C);
    MFV_LOGE("[MFV][ERROR] CRC Read 0xF602000C = 0x%08x", u4Val);
#if 0 // CRC
    u4Val = VDO_HW_READ(0xF6020010);
    MFV_LOGE("[MFV][ERROR] Read 0xF6020010 = 0x%08x", u4Val);
    u4Val = VDO_HW_READ(0xF6020014);
    MFV_LOGE("[MFV][ERROR] Read 0xF6020014 = 0x%08x", u4Val);
    u4Val = VDO_HW_READ(0xF6020018);
    MFV_LOGE("[MFV][ERROR] Read 0xF6020018 = 0x%08x", u4Val);
    u4Val = VDO_HW_READ(0xF602001C);
    MFV_LOGE("[MFV][ERROR] Read 0xF602001C = 0x%08x", u4Val);
    u4Val = VDO_HW_READ(0xF6020020);
    MFV_LOGE("[MFV][ERROR] Read 0xF6020020 = 0x%08x", u4Val);
    u4Val = VDO_HW_READ(0xF6020024);
    MFV_LOGE("[MFV][ERROR] Read 0xF6020024 = 0x%08x", u4Val);
    u4Val = VDO_HW_READ(0xF6020024);
    MFV_LOGE("[MFV][ERROR] CRC end Read 0xF6020024 = 0x%08x", u4Val);
#endif    
#if 0 // Read SRAM
    VDO_HW_WRITE(0xF602293C, (960*4));
    MFV_LOGE("[MFV][ERROR] Write 0xF602293C = 0x%08x", 960*4);
    u4Val = VDO_HW_READ(0xF6022940);
    MFV_LOGE("[MFV][ERROR] Read 0xF6022940 = 0x%08x", u4Val);
#endif 
    // SRAM power
    u4Val = VDO_HW_READ(KVA_CHECK_1_ADDR);
    MFV_LOGE("[MFV][ERROR] Read 0x10006234 = 0x%08x, is_ok = %d", u4Val, (u4Val & 0xF00) == 0xF00);
    u4Val = VDO_HW_READ(KVA_CHECK_2_ADDR);
    MFV_LOGE("[MFV][ERROR] Read 0x10006210 = 0x%08x, is_ok = %d", u4Val, (u4Val & 0xF00) == 0xF00);
#endif
    spin_lock_irqsave(&DecIsrLock, ulFlags);
    eValRet = eVideoSetEvent(&DecIsrEvent, sizeof(VAL_EVENT_T));
    if(VAL_RESULT_NO_ERROR != eValRet)
    {
        MFV_LOGE("[MFV][ERROR] ISR set DecIsrEvent error\n");
    }
    spin_unlock_irqrestore(&DecIsrLock, ulFlags);

#ifdef DEBUG_INFRA_POWER
    u4Infra = VDO_HW_READ(KVA_INFRA_ADDR);
    if ((u4Infra & (0x1 << 15)) != 0)
    {
        MFV_LOGE("[MFV][ERROR] Leaving ISR, but Infra is not 0x0 (0x%08x)(0x%08x), leaving...", u4Infra & ((0x1 << 15)), u4Infra);
    }

    u4CgStatus = VDO_HW_READ(0xF6000000);
    if (u4CgStatus != 0x1)
    {
        MFV_LOGE("[MFV][ERROR] Entering ISR, but CG status is not 0x1 (0x%08x), leaving...", u4CgStatus);
        //return;
    }
#endif
    return;
}


void enc_isr(void)
{
    VAL_RESULT_T  eValRet;
    int index, i, maxnum;
    unsigned long ulFlags, ulFlagsISR, ulFlagsLockHW;
    VAL_UINT32_T u4IRQStatus = 0;


    VAL_UINT32_T u4TempEncISRCount = 0;
    VAL_UINT32_T u4TempLockEncHWCount = 0;
    //----------------------

    spin_lock_irqsave(&EncISRCountLock, ulFlagsISR);
    gu4EncISRCount++;
    u4TempEncISRCount = gu4EncISRCount;
    spin_unlock_irqrestore(&EncISRCountLock, ulFlagsISR);

    spin_lock_irqsave(&LockEncHWCountLock, ulFlagsLockHW);
    u4TempLockEncHWCount = gu4LockEncHWCount;
    spin_unlock_irqrestore(&LockEncHWCountLock, ulFlagsLockHW);

    if (u4TempEncISRCount != u4TempLockEncHWCount)
    {
        //MFV_LOGE("[INFO] Enc ISRCount: 0x%x, LockHWCount:0x%x\n", u4TempEncISRCount, u4TempLockEncHWCount);
    }

    if (grVcodecEncHWLock.pvHandle == 0)
    {
        MFV_LOGE("[ERROR] NO one Lock Enc HW, please check!!\n");

        // Clear all status
        VDO_HW_WRITE(KVA_VENC_MP4_IRQ_ACK_ADDR, 1);
        VDO_HW_WRITE(KVA_VENC_IRQ_ACK_ADDR, VENC_IRQ_STATUS_PAUSE);
        VDO_HW_WRITE(KVA_VENC_IRQ_ACK_ADDR, VENC_IRQ_STATUS_DRAM_VP8);
        VDO_HW_WRITE(KVA_VENC_IRQ_ACK_ADDR, VENC_IRQ_STATUS_DRAM);
        VDO_HW_WRITE(KVA_VENC_IRQ_ACK_ADDR, VENC_IRQ_STATUS_SPS);
        VDO_HW_WRITE(KVA_VENC_IRQ_ACK_ADDR, VENC_IRQ_STATUS_PPS);
        VDO_HW_WRITE(KVA_VENC_IRQ_ACK_ADDR, VENC_IRQ_STATUS_FRM);
        return;
    }
    
    if (grVcodecEncHWLock.eDriverType == VAL_DRIVER_TYPE_H264_ENC ||
        grVcodecEncHWLock.eDriverType == VAL_DRIVER_TYPE_VP8_ENC) // hardwire
    {
        gu4HwVencIrqStatus = VDO_HW_READ(KVA_VENC_IRQ_STATUS_ADDR);
        if (gu4HwVencIrqStatus & VENC_IRQ_STATUS_PAUSE)
        {
            VDO_HW_WRITE(KVA_VENC_IRQ_ACK_ADDR, VENC_IRQ_STATUS_PAUSE);
        }
        if (gu4HwVencIrqStatus & VENC_IRQ_STATUS_DRAM_VP8)
        {
            VDO_HW_WRITE(KVA_VENC_IRQ_ACK_ADDR, (VENC_IRQ_STATUS_DRAM_VP8 | VENC_IRQ_STATUS_DRAM));
        }
        if (gu4HwVencIrqStatus & VENC_IRQ_STATUS_DRAM)
        {
            VDO_HW_WRITE(KVA_VENC_IRQ_ACK_ADDR, VENC_IRQ_STATUS_DRAM);
        }
        if (gu4HwVencIrqStatus & VENC_IRQ_STATUS_SPS)
        {
            VDO_HW_WRITE(KVA_VENC_IRQ_ACK_ADDR, VENC_IRQ_STATUS_SPS);
        }
        if (gu4HwVencIrqStatus & VENC_IRQ_STATUS_PPS)
        {
            VDO_HW_WRITE(KVA_VENC_IRQ_ACK_ADDR, VENC_IRQ_STATUS_PPS);
        }
        if (gu4HwVencIrqStatus & VENC_IRQ_STATUS_FRM)
        {
            VDO_HW_WRITE(KVA_VENC_IRQ_ACK_ADDR, VENC_IRQ_STATUS_FRM);
        }
    }    
    else if (grVcodecEncHWLock.eDriverType == VAL_DRIVER_TYPE_MP4_ENC) // Hybrid
    {
        spin_lock_irqsave(&OalHWContextLock, ulFlags);
        index = search_HWLockSlot_ByHandle(0, (VAL_HANDLE_T)grVcodecEncHWLock.pvHandle);

        //MFV_DEBUG("index = %d\n", index);

        // in case, if the process is killed first, 
        // then receive an ISR from HW, the event information already cleared.    
        if(index == -1) // Hybrid
        {
            MFV_LOGE("[ERROR][ISR] Can't find any index in ISR\n");   	  		  
            
            VDO_HW_WRITE(KVA_VENC_MP4_IRQ_ACK_ADDR, 1);
    	    spin_unlock_irqrestore(&OalHWContextLock, ulFlags);
            
            return;
        }
    						
        // get address from context			
        //MFV_DEBUG("ISR: Total %d u4NumOfRegister\n", oal_hw_context[index].u4NumOfRegister);

        maxnum = oal_hw_context[index].u4NumOfRegister;
        if(oal_hw_context[index].u4NumOfRegister > VCODEC_MULTIPLE_INSTANCE_NUM)
        {
        	MFV_LOGE("[ERROR] oal_hw_context[index].u4NumOfRegister =%d\n", oal_hw_context[index].u4NumOfRegister);
        	maxnum = VCODEC_MULTIPLE_INSTANCE_NUM;
        }

        //MFV_DEBUG("oal_hw_context[index].kva_u4HWIsCompleted 0x%x value=%d \n", oal_hw_context[index].kva_u4HWIsCompleted, *((volatile VAL_UINT32_T*)oal_hw_context[index].kva_u4HWIsCompleted)); 
        if ((((volatile VAL_UINT32_T*)oal_hw_context[index].kva_u4HWIsCompleted) == NULL) || (((volatile VAL_UINT32_T*)oal_hw_context[index].kva_u4HWIsTimeout) == NULL))
        {
            MFV_LOGE(" @@ [ERROR][ISR] index = %d, please check!!\n", index);
            
            VDO_HW_WRITE(KVA_VENC_MP4_IRQ_ACK_ADDR, 1);
            spin_unlock_irqrestore(&OalHWContextLock, ulFlags);
    	    
            return;
        }
        *((volatile VAL_UINT32_T*)oal_hw_context[index].kva_u4HWIsCompleted) = 1;
        *((volatile VAL_UINT32_T*)oal_hw_context[index].kva_u4HWIsTimeout) = 0;

        for(i=0; i < maxnum ; i++ )
        {                   
            //MFV_DEBUG("[BEFORE] ISR read: [%d]  User_va=0x%x kva=0x%x 0x%x \n", i ,
        	//*((volatile VAL_UINT32_T*)oal_hw_context[index].kva_Oal_HW_mem_reg + i*2),
        	//oal_hw_context[index].oalmem_status[i].u4ReadAddr, 
        	//*((volatile VAL_UINT32_T*)oal_hw_context[index].kva_Oal_HW_mem_reg + i*2 + 1));
                
        	*((volatile VAL_UINT32_T*)oal_hw_context[index].kva_Oal_HW_mem_reg + i*2 + 1) = *((volatile VAL_UINT32_T*)oal_hw_context[index].oalmem_status[i].u4ReadAddr);

            if (maxnum == 3)
            {
                if (i == 0)
                {
                    u4IRQStatus = (*((volatile VAL_UINT32_T*)oal_hw_context[index].kva_Oal_HW_mem_reg + i*2 + 1));
                    if (u4IRQStatus != 2)
                    {                    
                        MFV_LOGE("[ERROR][ISR] IRQ status error u4IRQStatus = %d\n", u4IRQStatus);
                    }
                }

                if (u4IRQStatus != 2)
                {
                    MFV_LOGE("[ERROR] %d, %x, %d, %d, %d, %d\n", 
                        i, 
                        ((volatile VAL_UINT32_T*)oal_hw_context[index].oalmem_status[i].u4ReadAddr),
                        (*((volatile VAL_UINT32_T*)oal_hw_context[index].kva_Oal_HW_mem_reg + i*2 + 1)),
                        VDO_HW_READ(KVA_VENC_MP4_IRQ_STATUS_ADDR),
                        VDO_HW_READ(KVA_VENC_ZERO_COEF_COUNT_ADDR),
                        VDO_HW_READ(KVA_VENC_BYTE_COUNT_ADDR));
                }
            }
            
            //MFV_DEBUG("[AFTER] ISR read: [%d]  User_va=0x%x kva=0x%x 0x%x \n", i ,
        	//*((volatile VAL_UINT32_T*)oal_hw_context[index].kva_Oal_HW_mem_reg + i*2),
        	//oal_hw_context[index].oalmem_status[i].u4ReadAddr, 
        	//*((volatile VAL_UINT32_T*)oal_hw_context[index].kva_Oal_HW_mem_reg + i*2 + 1) /*oal_hw_context[index].oalmem_status[i].u4ReadData*/);
        }
        spin_unlock_irqrestore(&OalHWContextLock, ulFlags);

        VDO_HW_WRITE(KVA_VENC_MP4_IRQ_ACK_ADDR, 1);

        // TODO: Release HW lock
    }

    eValRet = eVideoSetEvent(&EncIsrEvent, sizeof(VAL_EVENT_T)); 
    if(VAL_RESULT_NO_ERROR != eValRet)
    {
        MFV_LOGE("[MFV][ERROR] ISR set EncIsrEvent error\n");
    }
}

#ifdef DEBUG_MP4_ENC
void dump_venc_status(void)
{
    VAL_UINT32_T vencMp4Status = VDO_HW_READ(KVA_VENC_MP4_STATUS_ADDR);
    VAL_UINT32_T vencMp4MvqpStatus = VDO_HW_READ(KVA_VENC_MP4_MVQP_STATUS_ADDR);
    VAL_UINT32_T vencIrqStatus = VDO_HW_READ(KVA_VENC_MP4_IRQ_STATUS_ADDR);

    MFV_LOGD("[MP4ENC_STAUTUS] 0x664 = 0x%x\n", vencMp4Status);
    MFV_LOGD("[MP4ENC_STAUTUS] SOFT_GUARD %d\n", (vencMp4Status & 0xF));
    MFV_LOGD("[MP4ENC_STAUTUS] NOT_BYTE_ALIGNED %d\n", ((vencMp4Status >> 12) & 0x1));
    MFV_LOGD("[MP4ENC_STAUTUS] HW_BUSY %d\n", ((vencMp4Status >> 13) & 0x1));
    MFV_LOGD("[MP4ENC_STAUTUS] SLICE_DONE %d\n", ((vencMp4Status >> 14) & 0x1));
    MFV_LOGD("[MP4ENC_STAUTUS] FRAME_DONE %d\n", ((vencMp4Status >> 15) & 0x1));
    MFV_LOGD("[MP4ENC_STAUTUS] MB_X_POSITION %d\n", ((vencMp4Status >> 16) & 0xF));
    MFV_LOGD("[MP4ENC_STAUTUS] MB_X_POSITION %d\n", ((vencMp4Status >> 24) & 0xF));

    MFV_LOGD("[MP4ENC_STAUTUS] 0x6E4 = 0x%x\n", vencMp4MvqpStatus);
    MFV_LOGD("[MP4ENC_STAUTUS] MVQP_ST %d\n", (vencMp4MvqpStatus & 0x7));
    MFV_LOGD("[MP4ENC_STAUTUS] CUR_MB_CNT %d\n", ((vencMp4MvqpStatus >> 3) & 0x3FFF));
    MFV_LOGD("[MP4ENC_STAUTUS] RD_IDX %d\n", ((vencMp4MvqpStatus >> 18) & 0x3));
    MFV_LOGD("[MP4ENC_STAUTUS] WR_IDX %d\n", ((vencMp4MvqpStatus >> 20) & 0x3));
    MFV_LOGD("[MP4ENC_STAUTUS] NEED_SKIP_SET %d\n", ((vencMp4MvqpStatus >> 29) & 0x1));

    MFV_LOGD("[MP4ENC_STAUTUS] 0x67C = 0x%x\n", vencIrqStatus);
    MFV_LOGD("[MP4ENC_STAUTUS] SLICE_IRQ = %d\n", (vencIrqStatus & 0x1));
    MFV_LOGD("[MP4ENC_STAUTUS] FRAME_IRQ = %d\n", ((vencIrqStatus >> 1) & 0x1));
    MFV_LOGD("[MP4ENC_STAUTUS] BITSTREAM_IRQ = %d\n", ((vencIrqStatus >> 4) & 0x1));
    
}
#endif

static irqreturn_t video_intr_dlr(int irq, void *dev_id)
{   
    dec_isr();
    return IRQ_HANDLED;
}

static irqreturn_t video_intr_dlr2(int irq, void *dev_id)
{   
    enc_isr();
    return IRQ_HANDLED;
}


static long vcodec_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    VAL_INT32_T ret;
    VAL_UINT8_T *user_data_addr;
    VAL_RESULT_T  eValRet;
    VAL_RESULT_T  eValHWLockRet = VAL_RESULT_INVALID_ISR;    
    VAL_MEMORY_T  rTempMem;
    VAL_VCODEC_THREAD_ID_T rTempTID;
    VAL_UINT32_T u4Index = 0xff;
    VAL_INT32_T i4Index = -1;
    VAL_UINT32_T ulFlags, ulFlagsLockHW, ulFlagsISR;
    VAL_HW_LOCK_T rHWLock;
    VAL_BOOL_T  bLockedHW = VAL_FALSE;
    VAL_UINT32_T FirstUseDecHW = 0;
    VAL_UINT32_T FirstUseEncHW = 0;
    VAL_TIME_T rCurTime;
    VAL_UINT32_T u4TimeInterval;
    VAL_ISR_T  val_isr;
    VAL_VCODEC_CORE_LOADING_T rTempCoreLoading;
    VAL_VCODEC_CPU_OPP_LIMIT_T rCpuOppLimit;
    VAL_INT32_T temp_nr_cpu_ids;
    VAL_UINT32_T u4TempVCodecThreadNum;
    VAL_UINT32_T u4TempVCodecThreadID[VCODEC_THREAD_MAX_NUM];
    VAL_UINT32_T *pu4TempKVA;
    VAL_UINT32_T u4TempKPA;
#if 0
    VCODEC_DRV_CMD_QUEUE_T rDrvCmdQueue;
    P_VCODEC_DRV_CMD_T cmd_queue = VAL_NULL;
    VAL_UINT32_T u4Size, uValue, nCount;
#endif
    VAL_INT32_T i4I;

    switch(cmd) {
        case VCODEC_SET_THREAD_ID:
            MFV_LOGD("[MT6589] VCODEC_SET_THREAD_ID + tid = %d\n", current->pid);
            user_data_addr = (VAL_UINT8_T *)arg;
            ret = copy_from_user(&rTempTID, user_data_addr, sizeof(VAL_VCODEC_THREAD_ID_T));
            if (ret) {
                MFV_LOGE("[ERROR] VCODEC_SET_THREAD_ID, copy_from_user failed: %d\n", ret);
                return -EFAULT;
            }

            spin_lock_irqsave(&OalHWContextLock, ulFlags);
            setCurr_HWLockSlot_Thread_ID(rTempTID, &u4Index);
            spin_unlock_irqrestore(&OalHWContextLock, ulFlags);

            if (u4Index == 0xff) {
                MFV_LOGE("[ERROR] MT6589_VCODEC_SET_THREAD_ID error, u4Index = %d\n", u4Index);
            }
            
            MFV_LOGD("[MT6589] VCODEC_SET_THREAD_ID - tid = %d\n", current->pid);
        break;

        case VCODEC_ALLOC_NON_CACHE_BUFFER:
            MFV_LOGD("[MT6589][M4U]! VCODEC_ALLOC_NON_CACHE_BUFFER + tid = %d\n", current->pid);
            	
            user_data_addr = (VAL_UINT8_T *)arg;
            ret = copy_from_user(&rTempMem, user_data_addr, sizeof(VAL_MEMORY_T));
            if (ret)
            {
            	MFV_LOGE("[ERROR] VCODEC_ALLOC_NON_CACHE_BUFFER, copy_from_user failed: %d\n", ret);
            	return -EFAULT;
            } 

            rTempMem.u4ReservedSize /*kernel va*/ = (unsigned int)dma_alloc_coherent(0, rTempMem.u4MemSize, (dma_addr_t *)&rTempMem.pvMemPa, GFP_KERNEL);
            if((0 == rTempMem.u4ReservedSize) || (0 == rTempMem.pvMemPa))
            {
            	  MFV_LOGE("[ERROR] dma_alloc_coherent fail in VCODEC_ALLOC_NON_CACHE_BUFFER\n");
            	  return -EFAULT;
            }   

            MFV_LOGD("kernel va = 0x%x, kernel pa = 0x%x, memory size = %d\n", 
                (unsigned int)rTempMem.u4ReservedSize, (unsigned int)rTempMem.pvMemPa, (unsigned int)rTempMem.u4MemSize);            

            spin_lock_irqsave(&OalHWContextLock, ulFlags);
            i4Index = search_HWLockSlot_ByTID(0, current->pid);
            if (i4Index == -1)
            {
                MFV_LOGE("[ERROR] Add_NonCacheMemoryList error, u4Index = -1\n");
                break;
            }

            u4TempVCodecThreadNum = oal_hw_context[i4Index].u4VCodecThreadNum;
            for (i4I = 0; i4I < u4TempVCodecThreadNum; i4I++)
            {
                u4TempVCodecThreadID[i4I] = oal_hw_context[i4Index].u4VCodecThreadID[i4I];
            }
            spin_unlock_irqrestore(&OalHWContextLock, ulFlags);

            // TODO: Enable this if it is expected that no one should allocate non cache buffer before init hw lock
            /*
            if (u4Index == -1)
            {
                dma_free_coherent(0, rTempMem.u4MemSize, rTempMem.u4ReservedSize, (dma_addr_t)rTempMem.pvMemPa);
                return -EFAULT;
            }
            */

            mutex_lock(&NonCacheMemoryListLock); 
            Add_NonCacheMemoryList(rTempMem.u4ReservedSize, (VAL_UINT32_T)rTempMem.pvMemPa, (VAL_UINT32_T)rTempMem.u4MemSize, u4TempVCodecThreadNum, u4TempVCodecThreadID);
            mutex_unlock(&NonCacheMemoryListLock);
            
            ret = copy_to_user(user_data_addr, &rTempMem, sizeof(VAL_MEMORY_T));
            if(ret)
            {
            	MFV_LOGE("[ERROR] VCODEC_ALLOC_NON_CACHE_BUFFER, copy_to_user failed: %d\n", ret);
            	return -EFAULT;
            }  

            MFV_LOGD("[MT6589][M4U]! VCODEC_ALLOC_NON_CACHE_BUFFER - tid = %d\n", current->pid);
        break;

        case VCODEC_FREE_NON_CACHE_BUFFER:
            MFV_LOGD("[MT6589][M4U]! VCODEC_FREE_NON_CACHE_BUFFER + tid = %d\n", current->pid);
            	
            user_data_addr = (VAL_UINT8_T *)arg;
            ret = copy_from_user(&rTempMem, user_data_addr, sizeof(VAL_MEMORY_T));
            if (ret)
            {
            	MFV_LOGE("[ERROR] VCODEC_FREE_NON_CACHE_BUFFER, copy_from_user failed: %d\n", ret);
            	return -EFAULT;
            } 

            dma_free_coherent(0, rTempMem.u4MemSize, (void *)rTempMem.u4ReservedSize, (dma_addr_t)rTempMem.pvMemPa);

            mutex_lock(&NonCacheMemoryListLock);
            Free_NonCacheMemoryList(rTempMem.u4ReservedSize, (VAL_UINT32_T)rTempMem.pvMemPa);
            mutex_unlock(&NonCacheMemoryListLock);

            rTempMem.u4ReservedSize = 0;
            rTempMem.pvMemPa = NULL;

            ret = copy_to_user(user_data_addr, &rTempMem, sizeof(VAL_MEMORY_T));
            if(ret)
            {
            	MFV_LOGE("[ERROR] VCODEC_FREE_NON_CACHE_BUFFER, copy_to_user failed: %d\n", ret);
            	return -EFAULT;
            }  

            MFV_LOGD("[MT6589][M4U]! VCODEC_FREE_NON_CACHE_BUFFER - tid = %d\n", current->pid);
        break;

        case VCODEC_INC_DEC_EMI_USER:
            MFV_LOGD("[MT6589] VCODEC_INC_DEC_EMI_USER + tid = %d\n", current->pid);

            mutex_lock(&DecEMILock);
            gu4DecEMICounter++;
            MFV_LOGE("DEC_EMI_USER = %d\n", gu4DecEMICounter);
            user_data_addr = (VAL_UINT8_T *)arg;
            ret = copy_to_user(user_data_addr, &gu4DecEMICounter, sizeof(VAL_UINT32_T));
            if (ret)
            {
            	MFV_LOGE("[ERROR] VCODEC_INC_DEC_EMI_USER, copy_to_user failed: %d\n", ret);
                mutex_unlock(&DecEMILock);
            	return -EFAULT;
            }
            mutex_unlock(&DecEMILock);

            MFV_LOGD("[MT6589] VCODEC_INC_DEC_EMI_USER - tid = %d\n", current->pid);            
        break;

        case VCODEC_DEC_DEC_EMI_USER:
            MFV_LOGD("[MT6589] VCODEC_DEC_DEC_EMI_USER + tid = %d\n", current->pid);

            mutex_lock(&DecEMILock);
            gu4DecEMICounter--;
            MFV_LOGE("DEC_EMI_USER = %d\n", gu4DecEMICounter);
            user_data_addr = (VAL_UINT8_T *)arg;
            ret = copy_to_user(user_data_addr, &gu4DecEMICounter, sizeof(VAL_UINT32_T));
            if (ret)
            {
            	MFV_LOGE("[ERROR] VCODEC_DEC_DEC_EMI_USER, copy_to_user failed: %d\n", ret);
                mutex_unlock(&DecEMILock);
            	return -EFAULT;
            }
            mutex_unlock(&DecEMILock);

            MFV_LOGD("[MT6589] VCODEC_DEC_DEC_EMI_USER - tid = %d\n", current->pid);            
        break;

        case VCODEC_INC_ENC_EMI_USER:
            MFV_LOGD("[MT6589] VCODEC_INC_ENC_EMI_USER + tid = %d\n", current->pid);

            mutex_lock(&EncEMILock);
            gu4EncEMICounter++;
            MFV_LOGE("ENC_EMI_USER = %d\n", gu4EncEMICounter);
            user_data_addr = (VAL_UINT8_T *)arg;
            ret = copy_to_user(user_data_addr, &gu4EncEMICounter, sizeof(VAL_UINT32_T));
            if (ret) {
                MFV_LOGE("[ERROR] VCODEC_INC_ENC_EMI_USER, copy_to_user failed: %d\n", ret);
                mutex_unlock(&EncEMILock);
                return -EFAULT;
            }
            mutex_unlock(&EncEMILock);

            MFV_LOGD("[MT6589] VCODEC_INC_ENC_EMI_USER - tid = %d\n", current->pid);
        break;

        case VCODEC_DEC_ENC_EMI_USER:
            MFV_LOGD("[MT6589] VCODEC_DEC_ENC_EMI_USER + tid = %d\n", current->pid);

            mutex_lock(&EncEMILock);
            gu4EncEMICounter--;
            MFV_LOGE("ENC_EMI_USER = %d\n", gu4EncEMICounter);
            user_data_addr = (VAL_UINT8_T *)arg;
            ret = copy_to_user(user_data_addr, &gu4EncEMICounter, sizeof(VAL_UINT32_T));
            if (ret) {
                MFV_LOGE("[ERROR] VCODEC_DEC_ENC_EMI_USER, copy_to_user failed: %d\n", ret);
                mutex_unlock(&EncEMILock);
                return -EFAULT;
            }
            mutex_unlock(&EncEMILock);

            MFV_LOGD("[MT6589] VCODEC_DEC_ENC_EMI_USER - tid = %d\n", current->pid);
        break;

        case VCODEC_LOCKHW:
            MFV_LOGD("[MT6589] VCODEC_LOCKHW + tid = %d\n", current->pid);
            user_data_addr = (VAL_UINT8_T *)arg;
            ret = copy_from_user(&rHWLock, user_data_addr, sizeof(VAL_HW_LOCK_T));
            if (ret) {
                MFV_LOGE("[ERROR] VCODEC_LOCKHW, copy_from_user failed: %d\n", ret);
                return -EFAULT;
            }

            MFV_LOGD("LOCKHW eDriverType = %d\n", rHWLock.eDriverType);
            eValRet = VAL_RESULT_INVALID_ISR;
            if (rHWLock.eDriverType == VAL_DRIVER_TYPE_MP4_DEC ||
                rHWLock.eDriverType == VAL_DRIVER_TYPE_MP1_MP2_DEC ||
                rHWLock.eDriverType == VAL_DRIVER_TYPE_H264_DEC ||
                rHWLock.eDriverType == VAL_DRIVER_TYPE_VC1_DEC ||
                rHWLock.eDriverType == VAL_DRIVER_TYPE_VC1_ADV_DEC ||
                rHWLock.eDriverType == VAL_DRIVER_TYPE_RV9_DEC ||
                rHWLock.eDriverType == VAL_DRIVER_TYPE_VP8_DEC)
            {
                while (bLockedHW == VAL_FALSE)
                {
                    mutex_lock(&DecHWLockEventTimeoutLock);
                    if (DecHWLockEvent.u4TimeoutMs == 1) {
                        MFV_LOGE("[NOT ERROR][VCODEC_LOCKHW] First Use Dec HW!!\n");
                        FirstUseDecHW = 1;
                    }
                    else {
                        FirstUseDecHW = 0;
                    }                    
                    mutex_unlock(&DecHWLockEventTimeoutLock);
                    if (FirstUseDecHW == 1)
                    {
                        eValRet = eVideoWaitEvent(&DecHWLockEvent, sizeof(VAL_EVENT_T));
                    }
                    mutex_lock(&DecHWLockEventTimeoutLock);
                    if (DecHWLockEvent.u4TimeoutMs != 1000)
                    {
                        DecHWLockEvent.u4TimeoutMs = 1000;
                        FirstUseDecHW = 1;
                    }
                    else
                    {
                        FirstUseDecHW = 0;
                    }
                    mutex_unlock(&DecHWLockEventTimeoutLock);

                    mutex_lock(&VdecHWLock);
                    // one process try to lock twice
                    if (grVcodecDecHWLock.pvHandle == (VAL_VOID_T*)pmem_user_v2p_video((unsigned int)rHWLock.pvHandle)) {
                        MFV_LOGE("[WARNING] one decoder instance try to lock twice, may cause lock HW timeout!! instance = 0x%x, CurrentTID = %d\n",
                            grVcodecDecHWLock.pvHandle, current->pid);
                    }
                    mutex_unlock(&VdecHWLock);

                    if (FirstUseDecHW == 0) {
                        eValRet = eVideoWaitEvent(&DecHWLockEvent, sizeof(VAL_EVENT_T));
                    }

                    if (VAL_RESULT_INVALID_ISR == eValRet) {
                        MFV_LOGE("[ERROR][VCODEC_LOCKHW] DecHWLockEvent TimeOut, CurrentTID = %d\n", current->pid);
                        if (FirstUseDecHW != 1) {
                            mutex_lock(&VdecHWLock);
                            if (grVcodecDecHWLock.pvHandle == 0) {
                                MFV_LOGE("[WARNING] maybe mediaserver restart before, please check!!\n");
                            }
                            else {
                                MFV_LOGE("[WARNING] someone use HW, and check timeout value!!\n");
                            }
                            mutex_unlock(&VdecHWLock);
                        }
                    }

                    mutex_lock(&VdecHWLock);
                    if (grVcodecDecHWLock.pvHandle == 0) // No one holds dec hw lock now
                    {
                        grVcodecDecHWLock.pvHandle = (VAL_VOID_T*)pmem_user_v2p_video((unsigned int)rHWLock.pvHandle);
                        grVcodecDecHWLock.eDriverType = rHWLock.eDriverType;
                        eVideoGetTimeOfDay(&grVcodecDecHWLock.rLockedTime, sizeof(VAL_TIME_T));

                        MFV_LOGD("No process use dec HW, so current process can use HW\n");
                        MFV_LOGD("LockInstance = 0x%x CurrentTID = %d, rLockedTime(s, us) = %d, %d\n",
                            grVcodecDecHWLock.pvHandle, current->pid, grVcodecDecHWLock.rLockedTime.u4Sec, grVcodecDecHWLock.rLockedTime.u4uSec);
                    
                        bLockedHW = VAL_TRUE;
                        vdec_power_on();
                        enable_irq(MT_VDEC_IRQ_ID);
                    }
                    else // Another one holding dec hw now
                    {
                    MFV_LOGE("[NOT ERROR][VCODEC_LOCKHW] E\n");                    
                        eVideoGetTimeOfDay(&rCurTime, sizeof(VAL_TIME_T));
                        u4TimeInterval = (((((rCurTime.u4Sec - grVcodecDecHWLock.rLockedTime.u4Sec) * 1000000) + rCurTime.u4uSec) 
                            - grVcodecDecHWLock.rLockedTime.u4uSec) / 1000);

                        MFV_LOGD("someone use dec HW, and check timeout value\n");
                        MFV_LOGD("Instance = 0x%x CurrentTID = %d, TimeInterval(ms) = %d, TimeOutValue(ms)) = %d\n",
                            grVcodecDecHWLock.pvHandle, current->pid, u4TimeInterval, rHWLock.u4TimeoutMs);

                        MFV_LOGD("Instance = 0x%x, CurrentTID = %d, rLockedTime(s, us) = %d, %d, rCurTime(s, us) = %d, %d\n",
                            grVcodecDecHWLock.pvHandle, current->pid, 
                            grVcodecDecHWLock.rLockedTime.u4Sec, grVcodecDecHWLock.rLockedTime.u4uSec,
                            rCurTime.u4Sec, rCurTime.u4uSec
                            );

                        // 2012/12/16. Cheng-Jung Never steal hardware lock
                        if (0)
                        //if (u4TimeInterval >= rHWLock.u4TimeoutMs)
                        {
                            grVcodecDecHWLock.pvHandle = (VAL_VOID_T*)pmem_user_v2p_video((unsigned int)rHWLock.pvHandle);
                            grVcodecDecHWLock.eDriverType = rHWLock.eDriverType;
                            eVideoGetTimeOfDay(&grVcodecDecHWLock.rLockedTime, sizeof(VAL_TIME_T));
                            bLockedHW = VAL_TRUE;
                            vdec_power_on();
                            // TODO: Error handling, VDEC break, reset?
                        }
                    }
                    mutex_unlock(&VdecHWLock);
                    spin_lock_irqsave(&LockDecHWCountLock, ulFlagsLockHW);
                    gu4LockDecHWCount++;
                    spin_unlock_irqrestore(&LockDecHWCountLock, ulFlagsLockHW);
                }
            }
            else if (rHWLock.eDriverType == VAL_DRIVER_TYPE_MP4_ENC ||
                     rHWLock.eDriverType == VAL_DRIVER_TYPE_H264_ENC ||
                     rHWLock.eDriverType == VAL_DRIVER_TYPE_VP8_ENC)
            {
                while (bLockedHW == VAL_FALSE)
                {
                    // Wait to acquire Enc HW lock
                    mutex_lock(&EncHWLockEventTimeoutLock);
                    if (EncHWLockEvent.u4TimeoutMs == 1) {
                        MFV_LOGE("[NOT ERROR][VCODEC_LOCKHW] First Use Enc HW!!\n");
                        FirstUseEncHW = 1;
                    }
                    else {
                        FirstUseEncHW = 0;
                    }                    
                    mutex_unlock(&EncHWLockEventTimeoutLock);
                    if (FirstUseEncHW == 1)
                    {
                        eValRet = eVideoWaitEvent(&EncHWLockEvent, sizeof(VAL_EVENT_T));
                    }
                    mutex_lock(&EncHWLockEventTimeoutLock);
                    if (EncHWLockEvent.u4TimeoutMs == 1)
                    {
                        EncHWLockEvent.u4TimeoutMs = 1000;
                        FirstUseEncHW = 1;
                    }
                    else
                    {
                        FirstUseEncHW = 0;
                    }
                    mutex_unlock(&EncHWLockEventTimeoutLock);

                    mutex_lock(&VencHWLock);
                    // one process try to lock twice
                    if (grVcodecEncHWLock.pvHandle == (VAL_VOID_T*)pmem_user_v2p_video((unsigned int)rHWLock.pvHandle)) {
                        MFV_LOGE("[WARNING] one decoder instance try to lock twice, may cause lock HW timeout!! instance = 0x%x, CurrentTID = %d\n",
                            grVcodecEncHWLock.pvHandle, current->pid);
                    }
                    mutex_unlock(&VencHWLock);

                    if (FirstUseEncHW == 0) {
                        eValRet = eVideoWaitEvent(&EncHWLockEvent, sizeof(VAL_EVENT_T));
                    }

                    if (VAL_RESULT_INVALID_ISR == eValRet) {
                        MFV_LOGE("[ERROR][VCODEC_LOCKHW] EncHWLockEvent TimeOut, CurrentTID = %d\n", current->pid);
                        if (FirstUseEncHW != 1) {
                            mutex_lock(&VencHWLock);
                            if (grVcodecEncHWLock.pvHandle == 0) {
                                MFV_LOGE("[WARNING] maybe mediaserver restart before, please check!!\n");
                            }
                            else {
                                MFV_LOGE("[WARNING] someone use HW, and check timeout value!!\n");
                            }
                            mutex_unlock(&VencHWLock);
                        }
                    }

                    mutex_lock(&VencHWLock);
                    if (grVcodecEncHWLock.pvHandle == 0)   //No process use HW, so current process can use HW
                    {
                        if (rHWLock.eDriverType == VAL_DRIVER_TYPE_MP4_ENC)
                        {
                            spin_lock_irqsave(&OalHWContextLock, ulFlags);
                            u4Index = search_HWLockSlot_ByTID(0, current->pid);
                            //Index = search_HWLockSlot_ByHandle(0, pmem_user_v2p_video((unsigned int)rHWLock.pvHandle));
                            spin_unlock_irqrestore(&OalHWContextLock, ulFlags);
                            
                            if (u4Index == -1)
                            {
                                MFV_LOGE("[ERROR][VCODEC_LOCKHW] No process use enc HW, so current process can use HW, u4Index = -1\n");
                                mutex_unlock(&VencHWLock);
                                return -EFAULT;
                            }
                    
                            grVcodecEncHWLock.pvHandle = (VAL_VOID_T*)pmem_user_v2p_video((unsigned int)rHWLock.pvHandle);
                            MFV_LOGD("[LOG][VCODEC_LOCKHW] No process use enc HW, so current process can use HW, handle = 0x%x\n", grVcodecEncHWLock.pvHandle);                        
                            grVcodecEncHWLock.eDriverType = rHWLock.eDriverType;
                            spin_lock_irqsave(&OalHWContextLock, ulFlags);
                            oal_hw_context[u4Index].pvHandle = (VAL_HANDLE_T)grVcodecEncHWLock.pvHandle;
                            spin_unlock_irqrestore(&OalHWContextLock, ulFlags);

                            eVideoGetTimeOfDay(&grVcodecEncHWLock.rLockedTime, sizeof(VAL_TIME_T));
                    
                            MFV_LOGD("No process use enc HW, so current process can use HW\n");
                            MFV_LOGD("LockInstance = 0x%x CurrentTID = %d, rLockedTime(s, us) = %d, %d\n",
                                grVcodecEncHWLock.pvHandle, current->pid, grVcodecEncHWLock.rLockedTime.u4Sec, grVcodecEncHWLock.rLockedTime.u4uSec);
                            
                            bLockedHW = VAL_TRUE;
                            venc_power_on();
                            enable_irq(MT_VENC_IRQ_ID);
                        }
                        else if (rHWLock.eDriverType == VAL_DRIVER_TYPE_H264_ENC ||
                                 rHWLock.eDriverType == VAL_DRIVER_TYPE_VP8_ENC)
                        {
                            grVcodecEncHWLock.pvHandle = (VAL_VOID_T*)pmem_user_v2p_video((unsigned int)rHWLock.pvHandle);
                            MFV_LOGD("[LOG][VCODEC_LOCKHW] No process use HW, so current process can use HW, handle = 0x%x\n", grVcodecEncHWLock.pvHandle);
                            grVcodecEncHWLock.eDriverType = rHWLock.eDriverType;
                            eVideoGetTimeOfDay(&grVcodecEncHWLock.rLockedTime, sizeof(VAL_TIME_T));
                    
                            MFV_LOGD("No process use HW, so current process can use HW\n");
                            MFV_LOGD("LockInstance = 0x%x CurrentTID = %d, rLockedTime(s, us) = %d, %d\n",
                                grVcodecEncHWLock.pvHandle, current->pid, grVcodecEncHWLock.rLockedTime.u4Sec, grVcodecEncHWLock.rLockedTime.u4uSec);
                            
                            bLockedHW = VAL_TRUE;
                            venc_power_on();
                            enable_irq(MT_VENC_IRQ_ID);                        
                        }
                    }
                    else    //someone use HW, and check timeout value
                    {
                        eVideoGetTimeOfDay(&rCurTime, sizeof(VAL_TIME_T));
                        u4TimeInterval = (((((rCurTime.u4Sec - grVcodecEncHWLock.rLockedTime.u4Sec) * 1000000) + rCurTime.u4uSec) 
                            - grVcodecEncHWLock.rLockedTime.u4uSec) / 1000);
                
                        MFV_LOGD("someone use enc HW, and check timeout value\n");
                        MFV_LOGD("LockInstance = 0x%x, CurrentInstance = 0x%x, CurrentTID = %d, TimeInterval(ms) = %d, TimeOutValue(ms)) = %d\n",
                            grVcodecEncHWLock.pvHandle, pmem_user_v2p_video((unsigned int)rHWLock.pvHandle), current->pid, u4TimeInterval, rHWLock.u4TimeoutMs);
                
                        MFV_LOGD("LockInstance = 0x%x, CurrentInstance = 0x%x, CurrentTID = %d, rLockedTime(s, us) = %d, %d, rCurTime(s, us) = %d, %d\n",
                            grVcodecEncHWLock.pvHandle, pmem_user_v2p_video((unsigned int)rHWLock.pvHandle), current->pid, 
                            grVcodecEncHWLock.rLockedTime.u4Sec, grVcodecEncHWLock.rLockedTime.u4uSec,
                            rCurTime.u4Sec, rCurTime.u4uSec
                            );
                
                        if (u4TimeInterval >= rHWLock.u4TimeoutMs)  
                        //Locked process(A) timeout, so release HW and let currnet process(B) to use HW 
                        {
                            if (rHWLock.eDriverType == VAL_DRIVER_TYPE_MP4_ENC)
                            {
                                MFV_LOGE("[INFO][VCODEC_LOCKHW] Locked process(A) timeout, so release HW and let currnet process(B) to use HW\n");

                                // Set timeout status for previous lock owner.
                                spin_lock_irqsave(&OalHWContextLock, ulFlags);
                                ret = search_HWLockSlot_ByHandle(0, (VAL_HANDLE_T)grVcodecEncHWLock.pvHandle);
                                spin_unlock_irqrestore(&OalHWContextLock, ulFlags);
                    
                                if (ret == -1)
                                {
                                    MFV_LOGE("[ERROR] Locked process (A) - Instance 0x%x fail, didn't call InitHWLock \n", 
                                    grVcodecEncHWLock.pvHandle);
                                    mutex_unlock(&VencHWLock);
                                    return -EFAULT;
                                }
                                else
                                {
                                    spin_lock_irqsave(&OalHWContextLock, ulFlags);
                                    *((volatile VAL_UINT32_T*)oal_hw_context[ret].kva_u4HWIsCompleted) = 1;
                                    *((volatile VAL_UINT32_T*)oal_hw_context[ret].kva_u4HWIsTimeout) = 1;
                                    spin_unlock_irqrestore(&OalHWContextLock, ulFlags);
                    
                                    spin_lock_irqsave(&EncISRCountLock, ulFlagsISR);
                                    gu4EncISRCount++;
                                    spin_unlock_irqrestore(&EncISRCountLock, ulFlagsISR);
                                }

                                // Update lock owner to current 
                                spin_lock_irqsave(&OalHWContextLock, ulFlags);
                                u4Index = search_HWLockSlot_ByTID(0, current->pid);
                                spin_unlock_irqrestore(&OalHWContextLock, ulFlags);
                                
                                if (u4Index == -1)
                                {
                                    MFV_LOGE("[ERROR] Locked process - ID %d fail, didn't call InitHWLock\n", current->pid);
                                    mutex_unlock(&VencHWLock);
                                    return -EFAULT;
                                }
                    
                                grVcodecEncHWLock.pvHandle = (VAL_VOID_T*)pmem_user_v2p_video((unsigned int)rHWLock.pvHandle);
                                grVcodecEncHWLock.eDriverType = rHWLock.eDriverType;
                                spin_lock_irqsave(&OalHWContextLock, ulFlags);
                                oal_hw_context[u4Index].pvHandle = (VAL_HANDLE_T)grVcodecEncHWLock.pvHandle;
                                spin_unlock_irqrestore(&OalHWContextLock, ulFlags);
                                
                                eVideoGetTimeOfDay(&grVcodecEncHWLock.rLockedTime, sizeof(VAL_TIME_T));
                    
                                MFV_LOGD("LockInstance = 0x%x, CurrentTID = %d, rLockedTime(s, us) = %d, %d\n",
                                    grVcodecEncHWLock.pvHandle, current->pid, grVcodecEncHWLock.rLockedTime.u4Sec, grVcodecEncHWLock.rLockedTime.u4uSec);
                    
                                bLockedHW = VAL_TRUE;
                                venc_power_on();
                            }                            
                            else if (rHWLock.eDriverType == VAL_DRIVER_TYPE_H264_ENC ||
                                     rHWLock.eDriverType == VAL_DRIVER_TYPE_VP8_ENC)
                            {
                                grVcodecEncHWLock.pvHandle = (VAL_VOID_T*)pmem_user_v2p_video((unsigned int)rHWLock.pvHandle);
                                grVcodecEncHWLock.eDriverType = rHWLock.eDriverType;                                
                                eVideoGetTimeOfDay(&grVcodecEncHWLock.rLockedTime, sizeof(VAL_TIME_T));
                    
                                MFV_LOGD("LockInstance = 0x%x, CurrentTID = %d, rLockedTime(s, us) = %d, %d\n",
                                    grVcodecEncHWLock.pvHandle, current->pid, grVcodecEncHWLock.rLockedTime.u4Sec, grVcodecEncHWLock.rLockedTime.u4uSec);
                    
                                bLockedHW = VAL_TRUE;
                                venc_power_on();
                            }
                        }
                    }
                
                    if (bLockedHW == VAL_TRUE)
                    {
                        MFV_LOGD("grVcodecEncHWLock.pvHandle = 0x%x", grVcodecEncHWLock.pvHandle);
                    }
                    mutex_unlock(&VencHWLock);
                }
                
                spin_lock_irqsave(&LockEncHWCountLock, ulFlagsLockHW);
                gu4LockEncHWCount++;
                spin_unlock_irqrestore(&LockEncHWCountLock, ulFlagsLockHW);
                
                MFV_LOGD("get locked - ObjId =%d\n", current->pid);
                
                MFV_LOGD("VCODEC_LOCKHWed - tid = %d\n", current->pid); 

                if (rHWLock.eDriverType == VAL_DRIVER_TYPE_MP4_ENC) 
                {            
                    // add for debugging checking
                    spin_lock_irqsave(&OalHWContextLock, ulFlags);
                    ret = search_HWLockSlot_ByTID(0, current->pid);
                    spin_unlock_irqrestore(&OalHWContextLock, ulFlags);
                    
                    if(ret == -1)
                    {
                        MFV_LOGE("VCODEC_LOCKHW - ID %d  fail, didn't call InitHWLock \n", current->pid); 
                        return -EFAULT;
                    }
                }
            }
            else
            {
                MFV_LOGE("[WARNING] VCODEC_LOCKHW Unknown instance\n");
                return -EFAULT;
            }
            MFV_LOGD("[MT6589] VCODEC_LOCKHW - tid = %d\n", current->pid);
        break;

        case VCODEC_UNLOCKHW:
            MFV_LOGD("[MT6589] VCODEC_UNLOCKHW + tid = %d\n", current->pid);
            user_data_addr = (VAL_UINT8_T *)arg;
            ret = copy_from_user(&rHWLock, user_data_addr, sizeof(VAL_HW_LOCK_T));
            if (ret) {
                MFV_LOGE("[ERROR] VCODEC_UNLOCKHW, copy_from_user failed: %d\n", ret);
                return -EFAULT;
            }
            
            MFV_LOGD("UNLOCKHW eDriverType = %d\n", rHWLock.eDriverType);
            eValRet = VAL_RESULT_INVALID_ISR;
            if (rHWLock.eDriverType == VAL_DRIVER_TYPE_MP4_DEC ||
                rHWLock.eDriverType == VAL_DRIVER_TYPE_MP1_MP2_DEC ||
                rHWLock.eDriverType == VAL_DRIVER_TYPE_H264_DEC ||
                rHWLock.eDriverType == VAL_DRIVER_TYPE_VC1_DEC ||
                rHWLock.eDriverType == VAL_DRIVER_TYPE_VC1_ADV_DEC ||
                rHWLock.eDriverType == VAL_DRIVER_TYPE_RV9_DEC ||
                rHWLock.eDriverType == VAL_DRIVER_TYPE_VP8_DEC)
            {
                mutex_lock(&VdecHWLock);
                if (grVcodecDecHWLock.pvHandle == (VAL_VOID_T*)pmem_user_v2p_video((unsigned int)rHWLock.pvHandle)) // Current owner give up hw lock
                {
                    grVcodecDecHWLock.pvHandle = 0;
                    grVcodecDecHWLock.eDriverType = VAL_DRIVER_TYPE_NONE;
                    disable_irq(MT_VDEC_IRQ_ID);                    
                    // TODO: check if turning power off is ok
                    vdec_power_off();
                }
                else // Not current owner
                {
                    MFV_LOGD("[ERROR] Not owner trying to unlock dec hardware 0x%x\n", pmem_user_v2p_video((unsigned int)rHWLock.pvHandle));
                    mutex_unlock(&VdecHWLock);
                    return -EFAULT;
                }
                mutex_unlock(&VdecHWLock);
                eValRet = eVideoSetEvent(&DecHWLockEvent, sizeof(VAL_EVENT_T));
            }
            else if (rHWLock.eDriverType == VAL_DRIVER_TYPE_MP4_ENC ||
                     rHWLock.eDriverType == VAL_DRIVER_TYPE_H264_ENC ||
                     rHWLock.eDriverType == VAL_DRIVER_TYPE_VP8_ENC)
            {
                if (rHWLock.eDriverType == VAL_DRIVER_TYPE_MP4_ENC)
                {
                    // Debug
                    MFV_LOGE("Hybrid UNLOCKHW \n");
                }
                else if (rHWLock.eDriverType == VAL_DRIVER_TYPE_H264_ENC ||
                         rHWLock.eDriverType == VAL_DRIVER_TYPE_VP8_ENC)
                {
                    mutex_lock(&VencHWLock);
                    if (grVcodecEncHWLock.pvHandle == (VAL_VOID_T*)pmem_user_v2p_video((unsigned int)rHWLock.pvHandle)) // Current owner give up hw lock
                    {
                        grVcodecEncHWLock.pvHandle = 0;
                        grVcodecEncHWLock.eDriverType = VAL_DRIVER_TYPE_NONE;
                        disable_irq(MT_VENC_IRQ_ID);
                        // turn venc power off
                        venc_power_off();
                    }
                    else // Not current owner
                    {
                        // [TODO] error handling
                        MFV_LOGD("[ERROR] Not owner trying to unlock enc hardware 0x%x\n", pmem_user_v2p_video((unsigned int)rHWLock.pvHandle));
                        mutex_unlock(&VencHWLock);
                        return -EFAULT;                        
                    }
                    mutex_unlock(&VencHWLock);
                    eValRet = eVideoSetEvent(&EncHWLockEvent, sizeof(VAL_EVENT_T));
                }
            }
            else
            {
                MFV_LOGE("[WARNING] VCODEC_UNLOCKHW Unknown instance\n");
                return -EFAULT;            
            }
            MFV_LOGD("[MT6589] VCODEC_UNLOCKHW - tid = %d\n", current->pid);
        break;

        case VCODEC_INC_PWR_USER:
            MFV_LOGD("[MT6589] VCODEC_INC_PWR_USER + tid = %d\n", current->pid);
#if 0
            mutex_lock(&PWRLock);
            gu4PWRCounter++;
            MFV_LOGE("PWR_USER = %d\n", gu4PWRCounter);
            if (gu4PWRCounter == 1) {
                MFV_LOGE("[VCODEC_INC_PWR_USER] First Use HW, Enable Power!\n");
            }
            mutex_unlock(&PWRLock);
#endif
            MFV_LOGD("[MT6589] VCODEC_INC_PWR_USER - tid = %d\n", current->pid);
        break;

        case VCODEC_DEC_PWR_USER:
            MFV_LOGD("[MT6589] VCODEC_DEC_PWR_USER + tid = %d\n", current->pid);
#if 0
            mutex_lock(&PWRLock);
            gu4PWRCounter--;
            MFV_LOGE("PWR_USER = %d\n", gu4PWRCounter);
            if (gu4PWRCounter == 0) {
                MFV_LOGE("[VCODEC_DEC_PWR_USER] No One Use HW, Disable Power!\n");
            }
            mutex_unlock(&PWRLock);
#endif
            MFV_LOGD("[MT6589] VCODEC_DEC_PWR_USER - tid = %d\n", current->pid);
        break;

        case VCODEC_WAITISR:
            
            MFV_LOGD("[MT6589] VCODEC_WAITISR + tid = %d\n", current->pid);
            user_data_addr = (VAL_UINT8_T *)arg;
            ret = copy_from_user(&val_isr, user_data_addr, sizeof(VAL_ISR_T));
            if (ret) {
                MFV_LOGE("[ERROR] VCODEC_WAITISR, copy_from_user failed: %d\n", ret);
                return -EFAULT;
            }

            if (val_isr.eDriverType == VAL_DRIVER_TYPE_MP4_DEC ||
                val_isr.eDriverType == VAL_DRIVER_TYPE_MP1_MP2_DEC ||
                val_isr.eDriverType == VAL_DRIVER_TYPE_H264_DEC ||
                val_isr.eDriverType == VAL_DRIVER_TYPE_VC1_DEC ||
                val_isr.eDriverType == VAL_DRIVER_TYPE_VC1_ADV_DEC ||
                val_isr.eDriverType == VAL_DRIVER_TYPE_RV9_DEC ||
                val_isr.eDriverType == VAL_DRIVER_TYPE_VP8_DEC)
            {
                mutex_lock(&VdecHWLock);
                if (grVcodecDecHWLock.pvHandle == (VAL_VOID_T*)pmem_user_v2p_video((unsigned int)val_isr.pvHandle))
                {
                    bLockedHW = VAL_TRUE;
                }
                else
                {
                }
                mutex_unlock(&VdecHWLock);

                if (bLockedHW == VAL_FALSE)
                {
                    MFV_LOGE("[ERROR] DO NOT have HWLock, so return fail\n");
                    break;
                }

                spin_lock_irqsave(&DecIsrLock, ulFlags);
                DecIsrEvent.u4TimeoutMs = val_isr.u4TimeoutMs;
                spin_unlock_irqrestore(&DecIsrLock, ulFlags);
                                
                eValRet = eVideoWaitEvent(&DecIsrEvent, sizeof(VAL_EVENT_T));                
                if(VAL_RESULT_INVALID_ISR == eValRet)
                {
                    return -2;
                }
            } 
            else if (val_isr.eDriverType == VAL_DRIVER_TYPE_MP4_ENC ||
                     val_isr.eDriverType == VAL_DRIVER_TYPE_H264_ENC ||
                     val_isr.eDriverType == VAL_DRIVER_TYPE_VP8_ENC)
            {
                if (val_isr.eDriverType == VAL_DRIVER_TYPE_MP4_ENC) // Hybrid
                {
                    MFV_LOGD("[MT6589] VCODEC_WAITISR_MP4_ENC + tid = %d\n", current->pid);
                    spin_lock_irqsave(&OalHWContextLock, ulFlags);
                    u4Index = search_HWLockSlot_ByHandle(0, pmem_user_v2p_video((unsigned int)val_isr.pvHandle));
                    //u4Index = search_HWLockSlot_ByTID(0, current->pid);
                    spin_unlock_irqrestore(&OalHWContextLock, ulFlags);

                    if (u4Index == -1)
                    {
                        MFV_LOGE("[ERROR] VCODEC_WAITISR Fail, handle = 0x%x, tid = %d, index = -1\n", pmem_user_v2p_video((unsigned int)val_isr.pvHandle), current->pid); 
                        return -EFAULT;                    
                    }

                    MFV_LOGD("index = %d, start wait VCODEC_WAITISR handle 0x%x \n", u4Index, pmem_user_v2p_video((unsigned int)val_isr.pvHandle));
                    
                    mutex_lock(&VencHWLock);                    
                    MFV_LOGD("grVcodecEncHWLock.pvHandle = 0x%x\n", grVcodecEncHWLock.pvHandle);                    
                    if (grVcodecEncHWLock.pvHandle == (VAL_VOID_T*)pmem_user_v2p_video((unsigned int)val_isr.pvHandle)) //normal case
                    {
                        bLockedHW = VAL_TRUE;
                    }
                    else    //current process can not use HW
                    {
                        //do not disable irq, because other process is using irq 
                        MFV_LOGE("grVcodecEncHWLock.pvHandle = 0x%x, current handle = 0x%x\n", grVcodecEncHWLock.pvHandle, pmem_user_v2p_video((unsigned int)val_isr.pvHandle));  
                    }
                    mutex_unlock(&VencHWLock);

                    if (bLockedHW == VAL_FALSE)
                    {
                        MFV_LOGE("[ERROR] DO NOT have HWLock, so return fail\n");
                        break;
                    }
                
                    spin_lock_irqsave(&EncIsrLock, ulFlags);
                    EncIsrEvent.u4TimeoutMs = val_isr.u4TimeoutMs;
                    spin_unlock_irqrestore(&EncIsrLock, ulFlags);

                    eValRet = eVideoWaitEvent(&EncIsrEvent, sizeof(VAL_EVENT_T));
                    MFV_LOGD("waitdone VCODEC_WAITISR handle 0x%x \n", pmem_user_v2p_video((unsigned int)val_isr.pvHandle));
                    
                    mutex_lock(&VencHWLock);                    
                    if (grVcodecEncHWLock.pvHandle == (VAL_VOID_T*)pmem_user_v2p_video((unsigned int)val_isr.pvHandle)) //normal case
                    {
                        grVcodecEncHWLock.pvHandle = 0;
                        grVcodecEncHWLock.rLockedTime.u4Sec = 0;
                        grVcodecEncHWLock.rLockedTime.u4uSec = 0;
                        disable_irq_nosync(MT_VENC_IRQ_ID); 
                        // turn venc power off
                        venc_power_off();
 
                        eValHWLockRet = eVideoSetEvent(&EncHWLockEvent, sizeof(VAL_EVENT_T));
                        if(VAL_RESULT_NO_ERROR != eValHWLockRet)
                        {
                            MFV_LOGE("[MFV][ERROR] ISR set EncHWLockEvent error\n");
                        }
                    }                    
                    mutex_unlock(&VencHWLock);
                    
                    if(VAL_RESULT_INVALID_ISR == eValRet)
                    {
                        MFV_LOGE("[ERROR] WAIT_ISR_CMD TimeOut\n");
                    
                        spin_lock_irqsave(&OalHWContextLock, ulFlags);
                        *((volatile VAL_UINT32_T*)oal_hw_context[u4Index].kva_u4HWIsCompleted) = 0;
                        *((volatile VAL_UINT32_T*)oal_hw_context[u4Index].kva_u4HWIsTimeout) = 1;
                        spin_unlock_irqrestore(&OalHWContextLock, ulFlags);
                        
                        spin_lock_irqsave(&EncISRCountLock, ulFlagsISR);
                        gu4EncISRCount++;
                        spin_unlock_irqrestore(&EncISRCountLock, ulFlagsISR);

                        // Cheng-Jung 20120614 Dump status register
                        #ifdef DEBUG_MP4_ENC
                        dump_venc_status();
                        #endif

                        // TODO: power down hw?
                        
                        return -2;
                    }
                }
                else if (val_isr.eDriverType == VAL_DRIVER_TYPE_H264_ENC ||
                         val_isr.eDriverType == VAL_DRIVER_TYPE_VP8_ENC) // Pure HW
                {
                    mutex_lock(&VencHWLock);
                    if (grVcodecEncHWLock.pvHandle == (VAL_VOID_T*)pmem_user_v2p_video((unsigned int)val_isr.pvHandle))
                    {
                        bLockedHW = VAL_TRUE;
                    }
                    else
                    {
                    }
                    mutex_unlock(&VencHWLock);
                    
                    if (bLockedHW == VAL_FALSE)
                    {
                        MFV_LOGE("[ERROR] DO NOT have HWLock, so return fail\n");
                        break;
                    }          
                    
                    spin_lock_irqsave(&EncIsrLock, ulFlags);
                    EncIsrEvent.u4TimeoutMs = val_isr.u4TimeoutMs;
                    spin_unlock_irqrestore(&EncIsrLock, ulFlags);
                                    
                    eValRet = eVideoWaitEvent(&EncIsrEvent, sizeof(VAL_EVENT_T));                
                    if(VAL_RESULT_INVALID_ISR == eValRet)
                    {
                        return -2;
                    }

                    if (val_isr.u4IrqStatusNum > 0)
                    {
                        val_isr.u4IrqStatus[0] = gu4HwVencIrqStatus;
                        ret = copy_to_user(user_data_addr, &val_isr, sizeof(VAL_ISR_T));
                        if (ret) {
                            MFV_LOGE("[ERROR] VCODEC_WAITISR, copy_to_user failed: %d\n", ret);
                            return -EFAULT;
                        }
                    }
                }
            }
            else
            {
                MFV_LOGE("[WARNING] VCODEC_WAITISR Unknown instance\n");
                return -EFAULT;
            }
            MFV_LOGD("[MT6589] VCODEC_WAITISR - tid = %d\n", current->pid);
        break;

        case VCODEC_INITHWLOCK:
        {
            VAL_VCODEC_OAL_HW_CONTEXT_T *context;
            VAL_VCODEC_OAL_HW_REGISTER_T  hwoal_reg;
            VAL_VCODEC_OAL_HW_REGISTER_T  *kva_TempReg;
            VAL_VCODEC_OAL_MEM_STAUTS_T  oal_mem_status[OALMEM_STATUS_NUM];
            unsigned int addr_pa, ret, i, pa_u4HWIsCompleted, pa_u4HWIsTimeout;

            MFV_LOGD("[MT6589] VCODEC_INITHWLOCK + - tid = %d\n", current->pid);

            ////////////// Start to get content
            ////////////// take VAL_VCODEC_OAL_HW_REGISTER_T content
            user_data_addr = (VAL_UINT8_T *)arg;
            ret = copy_from_user(&hwoal_reg, user_data_addr, sizeof(VAL_VCODEC_OAL_HW_REGISTER_T));

            // TODO:
            addr_pa = pmem_user_v2p_video((unsigned int )user_data_addr);

            spin_lock_irqsave(&OalHWContextLock, ulFlags);            
            context = setCurr_HWLockSlot(addr_pa, current->pid);
            context->Oal_HW_reg = (VAL_VCODEC_OAL_HW_REGISTER_T *)arg;
            context->Oal_HW_mem_reg = (VAL_UINT32_T*)(((VAL_VCODEC_OAL_HW_REGISTER_T *)user_data_addr)->pHWStatus);
            if (hwoal_reg.u4NumOfRegister != 0) 
            {
                context->pa_Oal_HW_mem_reg =  pmem_user_v2p_video( (int)( ((VAL_VCODEC_OAL_HW_REGISTER_T *)user_data_addr)->pHWStatus ) );
            }
            pa_u4HWIsCompleted =  pmem_user_v2p_video( (int)&( ((VAL_VCODEC_OAL_HW_REGISTER_T *)user_data_addr)->u4HWIsCompleted ) );
            pa_u4HWIsTimeout =  pmem_user_v2p_video( (int)&( ((VAL_VCODEC_OAL_HW_REGISTER_T *)user_data_addr)->u4HWIsTimeout ) );
            MFV_LOGD("user_data_addr->u4HWIsCompleted ua = 0x%x pa= 0x%x\n", (int)&( ((VAL_VCODEC_OAL_HW_REGISTER_T *)user_data_addr)->u4HWIsCompleted ), pa_u4HWIsCompleted );
            MFV_LOGD("user_data_addr->u4HWIsTimeout ua = 0x%x pa= 0x%x\n", (int)&( ((VAL_VCODEC_OAL_HW_REGISTER_T *)user_data_addr)->u4HWIsTimeout ), pa_u4HWIsTimeout );


    	    //ret = copy_from_user(&oal_mem_status[0], ((VAL_VCODEC_OAL_HW_REGISTER_T *)user_data_addr)->pHWStatus, hwoal_reg.u4NumOfRegister*sizeof(VAL_VCODEC_OAL_MEM_STAUTS_T));
    	    ret = copy_from_user(&oal_mem_status[0], hwoal_reg.pHWStatus, hwoal_reg.u4NumOfRegister*sizeof(VAL_VCODEC_OAL_MEM_STAUTS_T));
            context->u4NumOfRegister = hwoal_reg.u4NumOfRegister;
            MFV_LOGW("[VCODEC_INITHWLOCK] ToTal %d u4NumOfRegister\n", hwoal_reg.u4NumOfRegister);

            if (hwoal_reg.u4NumOfRegister != 0) 
            {
                u4TempKPA = context->pa_Oal_HW_mem_reg;
                spin_unlock_irqrestore(&OalHWContextLock, ulFlags);
                mutex_lock(&NonCacheMemoryListLock);
                pu4TempKVA = (VAL_UINT32_T *)Search_NonCacheMemoryList_By_KPA(u4TempKPA);
                mutex_unlock(&NonCacheMemoryListLock);
                spin_lock_irqsave(&OalHWContextLock, ulFlags);
                context->kva_Oal_HW_mem_reg = pu4TempKVA;
                MFV_LOGD("context->ua = 0x%x  pa_Oal_HW_mem_reg = 0x%x \n", (int)( ((VAL_VCODEC_OAL_HW_REGISTER_T *)user_data_addr)->pHWStatus ), context->pa_Oal_HW_mem_reg);
            }
            spin_unlock_irqrestore(&OalHWContextLock, ulFlags);
            mutex_lock(&NonCacheMemoryListLock);
            kva_TempReg = (VAL_VCODEC_OAL_HW_REGISTER_T *)Search_NonCacheMemoryList_By_KPA(addr_pa);
            mutex_unlock(&NonCacheMemoryListLock);
            spin_lock_irqsave(&OalHWContextLock, ulFlags);
            context->kva_u4HWIsCompleted = (VAL_UINT32_T)(&(kva_TempReg->u4HWIsCompleted));
            context->kva_u4HWIsTimeout = (VAL_UINT32_T)(&(kva_TempReg->u4HWIsTimeout));
            MFV_LOGD("kva_TempReg = 0x%x, kva_u4HWIsCompleted = 0x%x, kva_u4HWIsTimeout = 0x%x\n",
                (VAL_UINT32_T)kva_TempReg, context->kva_u4HWIsCompleted, context->kva_u4HWIsTimeout);

            for (i = 0; i < hwoal_reg.u4NumOfRegister ; i++ ) 
            {
                int kva;
                MFV_LOGE("[REG_INFO_1] [%d] 0x%x 0x%x \n", i ,
                oal_mem_status[i].u4ReadAddr, oal_mem_status[i].u4ReadData);

                // TODO:
                addr_pa = pmem_user_v2p_video((unsigned int )oal_mem_status[i].u4ReadAddr);
                kva = (VAL_UINT32_T)ioremap(addr_pa, 8); // need to remap addr + data addr
                MFV_LOGE("[REG_INFO_2] [%d] pa = 0x%x  kva = 0x%x \n", i , addr_pa, kva);
                context->oalmem_status[i].u4ReadAddr = kva; //oal_mem_status[i].u4ReadAddr;
            }
            spin_unlock_irqrestore(&OalHWContextLock, ulFlags);

            MFV_DEBUG(" VCODEC_INITHWLOCK addr1 0x%x addr2 0x%x \n",
            (unsigned int )arg, (unsigned int ) ((VAL_VCODEC_OAL_HW_REGISTER_T *)arg)->pHWStatus);
            MFV_LOGD("[MT6589] VCODEC_INITHWLOCK - - tid = %d\n", current->pid);
        }
        break;

        case VCODEC_DEINITHWLOCK:
        { 
            VAL_UINT8_T *user_data_addr;
            int addr_pa;

            MFV_LOGD("[MT6589] VCODEC_DEINITHWLOCK + - tid = %d\n", current->pid);

            user_data_addr = (VAL_UINT8_T *)arg;
            // TODO:
            addr_pa = pmem_user_v2p_video((unsigned int )user_data_addr);

            MFV_DEBUG("VCODEC_DEINITHWLOCK ObjId=%d\n", current->pid);
            spin_lock_irqsave(&OalHWContextLock, ulFlags);
            freeCurr_HWLockSlot(addr_pa);
            spin_unlock_irqrestore(&OalHWContextLock, ulFlags);
            MFV_LOGD("[MT6589] VCODEC_DEINITHWLOCK - - tid = %d\n", current->pid);
        }
        break;

        case VCODEC_GET_CPU_LOADING_INFO:
        {
            VAL_UINT8_T *user_data_addr;
            VAL_VCODEC_CPU_LOADING_INFO_T _temp;

            MFV_LOGD("[MT6589] VCODEC_GET_CPU_LOADING_INFO +\n");
            user_data_addr = (VAL_UINT8_T *)arg;
            // TODO:
#if 0 // Morris Yang 20120112 mark temporarily
            _temp._cpu_idle_time = mt_get_cpu_idle(0);
            _temp._thread_cpu_time = mt_get_thread_cputime(0);
            spin_lock_irqsave(&OalHWContextLock, ulFlags);
            _temp._inst_count = getCurInstanceCount();
            spin_unlock_irqrestore(&OalHWContextLock, ulFlags);
            _temp._sched_clock = mt_sched_clock();
#endif
            ret = copy_to_user(user_data_addr, &_temp, sizeof(VAL_VCODEC_CPU_LOADING_INFO_T));
            if (ret) {
                MFV_LOGE("[ERROR] VCODEC_GET_CPU_LOADING_INFO, copy_to_user failed: %d\n", ret);
                return -EFAULT;
            }

            MFV_LOGD("[MT6589] VCODEC_GET_CPU_LOADING_INFO -\n");
            break;
        }

        case VCODEC_GET_CORE_LOADING:
        {
            MFV_LOGD("[MT6589] VCODEC_GET_CORE_LOADING + - tid = %d\n", current->pid);

            user_data_addr = (VAL_UINT8_T *)arg;
            ret = copy_from_user(&rTempCoreLoading, user_data_addr, sizeof(VAL_VCODEC_CORE_LOADING_T));
            if (ret)
            {
                MFV_LOGE("[ERROR] VCODEC_GET_CORE_LOADING, copy_from_user failed: %d\n", ret);
                return -EFAULT;
            }

            // TODO: Check if get_cpu_load is still available
            // rTempCoreLoading.Loading = get_cpu_load(rTempCoreLoading.CPUid);

            ret = copy_to_user(user_data_addr, &rTempCoreLoading, sizeof(VAL_VCODEC_CORE_LOADING_T));
            if (ret) {
                MFV_LOGE("[ERROR] VCODEC_GET_CORE_LOADING, copy_to_user failed: %d\n", ret);
                return -EFAULT;
            }

            MFV_LOGD("[MT6589] VCODEC_GET_CORE_LOADING - - tid = %d\n", current->pid);
            break;
        }

        case VCODEC_GET_CORE_NUMBER:
        {
            MFV_LOGD("[MT6589] VCODEC_GET_CORE_NUMBER + - tid = %d\n", current->pid);

            user_data_addr = (VAL_UINT8_T *)arg;
            // TODO: Check if nr_cpu_ids is available
            //temp_nr_cpu_ids = nr_cpu_ids;
            ret = copy_to_user(user_data_addr, &temp_nr_cpu_ids, sizeof(int));
            if (ret) {
                MFV_LOGE("[ERROR] VCODEC_GET_CORE_NUMBER, copy_to_user failed: %d\n", ret);
                return -EFAULT;
            }

            MFV_LOGD("[MT6589] VCODEC_GET_CORE_NUMBER - - tid = %d\n", current->pid);
            break;
        }
        case VCODEC_SET_CPU_OPP_LIMIT:
            MFV_LOGD("[MT6589] VCODEC_SET_CPU_OPP_LIMIT + - tid = %d\n", current->pid);
            user_data_addr = (VAL_UINT8_T *)arg;
            ret = copy_from_user(&rCpuOppLimit, user_data_addr, sizeof(VAL_VCODEC_CPU_OPP_LIMIT_T));
            if (ret) {
                MFV_LOGE("[ERROR] VCODEC_SET_CPU_OPP_LIMIT, copy_from_user failed: %d\n", ret);
                return -EFAULT;
            }
            MFV_LOGE("+VCODEC_SET_CPU_OPP_LIMIT (%d, %d, %d), tid = %d\n", rCpuOppLimit.limited_freq, rCpuOppLimit.limited_cpu, rCpuOppLimit.enable, current->pid);
            // TODO: Check if cpu_opp_limit is available
            //ret = cpu_opp_limit(EVENT_VIDEO, rCpuOppLimit.limited_freq, rCpuOppLimit.limited_cpu, rCpuOppLimit.enable); // 0: PASS, other: FAIL
            if (ret) {
                MFV_LOGE("[ERROR] cpu_opp_limit failed: %d\n", ret);
                return -EFAULT;
            }
            MFV_LOGE("-VCODEC_SET_CPU_OPP_LIMIT tid = %d, ret = %d\n", current->pid, ret);
            MFV_LOGD("[MT6589] VCODEC_SET_CPU_OPP_LIMIT - - tid = %d\n", current->pid);
            break;
        case VCODEC_MB:
            mb();
            break;
#if 0
        case MFV_SET_CMD_CMD:
            MFV_LOGD("[MFV] MFV_SET_CMD_CMD\n");
            MFV_LOGD("[MFV] Arg = %x\n",arg);
            user_data_addr = (VAL_UINT8_T *)arg;
            ret = copy_from_user(&rDrvCmdQueue, user_data_addr, sizeof(VCODEC_DRV_CMD_QUEUE_T));
            MFV_LOGD("[MFV] CmdNum = %d\n",rDrvCmdQueue.CmdNum);
            u4Size = (rDrvCmdQueue.CmdNum)*sizeof(VCODEC_DRV_CMD_T);

            cmd_queue = (P_VCODEC_DRV_CMD_T)kmalloc(u4Size,GFP_ATOMIC);
            if (cmd_queue != VAL_NULL && rDrvCmdQueue.pCmd != VAL_NULL) {
                ret = copy_from_user(cmd_queue, rDrvCmdQueue.pCmd, u4Size);
                while (cmd_queue->type != END_CMD) {
                    switch (cmd_queue->type)
                    {
                        case ENABLE_HW_CMD:
                            break;
                        case DISABLE_HW_CMD:
                            break;
                        case WRITE_REG_CMD:
                            VDO_HW_WRITE(cmd_queue->address + cmd_queue->offset, cmd_queue->value);
                            break;
                        case READ_REG_CMD:
                            uValue = VDO_HW_READ(cmd_queue->address + cmd_queue->offset);
                            copy_to_user((void *)cmd_queue->value, &uValue, sizeof(VAL_UINT32_T));
                            break;
                        case WRITE_SYSRAM_CMD:
                            VDO_HW_WRITE(cmd_queue->address + cmd_queue->offset, cmd_queue->value);
                            break;
                        case READ_SYSRAM_CMD:
                            uValue = VDO_HW_READ(cmd_queue->address + cmd_queue->offset);
                            copy_to_user((void *)cmd_queue->value, &uValue, sizeof(VAL_UINT32_T));
                            break;
                        case MASTER_WRITE_CMD:
                            uValue = VDO_HW_READ(cmd_queue->address + cmd_queue->offset);
                            VDO_HW_WRITE(cmd_queue->address + cmd_queue->offset, cmd_queue->value | (uValue & cmd_queue->mask));
                            break;
                        case SETUP_ISR_CMD:
                            break;
                        case WAIT_ISR_CMD:
                            MFV_LOGD("HAL_CMD_SET_CMD_QUEUE: WAIT_ISR_CMD+\n"); 
                            
                            MFV_LOGD("HAL_CMD_SET_CMD_QUEUE: WAIT_ISR_CMD-\n"); 
                            break;
                        case TIMEOUT_CMD:
                            break;
                        case WRITE_SYSRAM_RANGE_CMD:
                            break;
                        case READ_SYSRAM_RANGE_CMD:
                            break;
                        case POLL_REG_STATUS_CMD:
                            uValue = VDO_HW_READ(cmd_queue->address + cmd_queue->offset);
                            nCount = 0;
                            while ((uValue & cmd_queue->mask) != 0) {
                                nCount++;
                                if (nCount > 1000) {
                                    break;
                                }
                                uValue = VDO_HW_READ(cmd_queue->address + cmd_queue->offset);
                            }
                            break;
                        default:
                            break;
                    }
                    cmd_queue++;
                }

            }
            break;
#endif
        default:
            MFV_LOGE("========[ERROR] vcodec_ioctl default case======== %u\n", cmd);
        break;
    }
    return 0xFF;
}

static int vcodec_open(struct inode *inode, struct file *file)
{
    MFV_LOGD("[MFV_DEBUG] vcodec_open\n");

    mutex_lock(&DriverOpenCountLock);
    MT6589Driver_Open_Count++;

    MFV_LOGE("vcodec_open pid = %d, MT6589Driver_Open_Count %d\n", current->pid, MT6589Driver_Open_Count);
    mutex_unlock(&DriverOpenCountLock);

    // TODO: Check upper limit of concurrent users?

    return 0;
}

static int vcodec_flush(struct file *file, fl_owner_t id)
{
    int i, j;
    unsigned long ulFlags, ulFlagsLockHW, ulFlagsISR;

    //dump_stack();
    MFV_LOGD("[MFV_DEBUG] vcodec_flush, curr_tid =%d\n", current->pid);
    mutex_lock(&DriverOpenCountLock);
    MFV_LOGE("vcodec_flush pid = %d, MT6589Driver_Open_Count %d\n", current->pid, MT6589Driver_Open_Count);

    MT6589Driver_Open_Count--;

    mutex_lock(&NonCacheMemoryListLock);
    // TODO: Check if Force_Free_NonCacheMemoryList should be called?
    //Force_Free_NonCacheMemoryList(current->pid);
    mutex_unlock(&NonCacheMemoryListLock);

    if (MT6589Driver_Open_Count == 0) {

        mutex_lock(&VdecHWLock);
        grVcodecDecHWLock.pvHandle = 0;
        grVcodecDecHWLock.eDriverType = VAL_DRIVER_TYPE_NONE;
        grVcodecDecHWLock.rLockedTime.u4Sec = 0;
        grVcodecDecHWLock.rLockedTime.u4uSec = 0;
        mutex_unlock(&VdecHWLock);

        mutex_lock(&VencHWLock);
        grVcodecEncHWLock.pvHandle = 0;
        grVcodecEncHWLock.eDriverType = VAL_DRIVER_TYPE_NONE;
        grVcodecEncHWLock.rLockedTime.u4Sec = 0;
        grVcodecEncHWLock.rLockedTime.u4uSec = 0;
        mutex_unlock(&VencHWLock);

        mutex_lock(&DecEMILock);
        gu4DecEMICounter = 0;
        mutex_unlock(&DecEMILock);

        mutex_lock(&EncEMILock);
        gu4EncEMICounter = 0;
        mutex_unlock(&EncEMILock);

        mutex_lock(&PWRLock);
        gu4PWRCounter = 0;
        mutex_unlock(&PWRLock);

        mutex_lock(&NonCacheMemoryListLock);
        for (i = 0; i < VCODEC_MULTIPLE_INSTANCE_NUM_x_10; i++) {
            grNonCacheMemoryList[i].pvHandle = 0;
            for (j = 0; j < VCODEC_THREAD_MAX_NUM; j++)
            {
                grNonCacheMemoryList[i].u4VCodecThreadID[j] = 0xffffffff;
            }            
            grNonCacheMemoryList[i].u4KVA = 0xffffffff;
            grNonCacheMemoryList[i].u4KPA = 0xffffffff;
        }
        mutex_unlock(&NonCacheMemoryListLock);

        spin_lock_irqsave(&OalHWContextLock, ulFlags);
        for (i = 0; i < VCODEC_MULTIPLE_INSTANCE_NUM; i++) {
            oal_hw_context[i].Oal_HW_reg = (VAL_VCODEC_OAL_HW_REGISTER_T  *)0;
            oal_hw_context[i].ObjId = -1;
            oal_hw_context[i].slotindex = i;
            for (j = 0; j < VCODEC_THREAD_MAX_NUM; j++)
            {
                oal_hw_context[i].u4VCodecThreadID[j] = -1;
            }
            oal_hw_context[i].pvHandle = 0;
            oal_hw_context[i].u4NumOfRegister = 0;

            for (j = 0; j < OALMEM_STATUS_NUM ; j ++) {
                oal_hw_context[i].oalmem_status[j].u4ReadAddr = 0;
                oal_hw_context[i].oalmem_status[j].u4ReadData = 0;
            }
        }
        spin_unlock_irqrestore(&OalHWContextLock, ulFlags);

        spin_lock_irqsave(&LockDecHWCountLock, ulFlagsLockHW);
        gu4LockDecHWCount = 0;
        spin_unlock_irqrestore(&LockDecHWCountLock, ulFlagsLockHW);

        spin_lock_irqsave(&LockEncHWCountLock, ulFlagsLockHW);
        gu4LockEncHWCount = 0;
        spin_unlock_irqrestore(&LockEncHWCountLock, ulFlagsLockHW);
        
        spin_lock_irqsave(&DecISRCountLock, ulFlagsISR);
        gu4DecISRCount = 0;
        spin_unlock_irqrestore(&DecISRCountLock, ulFlagsISR);

        spin_lock_irqsave(&EncISRCountLock, ulFlagsISR);
        gu4EncISRCount = 0;
        spin_unlock_irqrestore(&EncISRCountLock, ulFlagsISR);
        
    }
    mutex_unlock(&DriverOpenCountLock);

    return 0;
}

static int vcodec_release(struct inode *inode, struct file *file)
{
    MFV_LOGD("[MFV_DEBUG] mflexvideo_release, curr_tid =%d\n", current->pid);

    return 0;
}

void vcodec_vma_open(struct vm_area_struct *vma)
{
    MFV_LOGD("vcodec VMA open, virt %lx, phys %lx\n", vma->vm_start, vma->vm_pgoff << PAGE_SHIFT);
}

void vcodec_vma_close(struct vm_area_struct *vma)
{
     MFV_LOGD("vcodec VMA close, virt %lx, phys %lx\n", vma->vm_start, vma->vm_pgoff << PAGE_SHIFT);
}

static struct vm_operations_struct vcodec_remap_vm_ops = {
    .open = vcodec_vma_open,
    .close = vcodec_vma_close,
};

static int vcodec_mmap(struct file* file, struct vm_area_struct* vma)
{
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    MFV_LOGE("[mmap] vma->start 0x%x, vma->end 0x%x, vma->pgoff 0x%x\n", 
             (unsigned int)vma->vm_start, (unsigned int)vma->vm_end, (unsigned int)vma->vm_pgoff);
    if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff,
        vma->vm_end - vma->vm_start, vma->vm_page_prot)) {
        return -EAGAIN;
    }

    vma->vm_ops = &vcodec_remap_vm_ops;
    vcodec_vma_open(vma);

    return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void vcodec_early_suspend(struct early_suspend *h)
{
    mutex_lock(&PWRLock);
    MFV_LOGE("vcodec_early_suspend, tid = %d, PWR_USER = %d\n", current->pid, gu4PWRCounter);
    mutex_unlock(&PWRLock);
/*
    if (gu4PWRCounter != 0)
    {
        MFV_LOGE("[MT6589_VCodec_early_suspend] Someone Use HW, Disable Power!\n");
        disable_clock(MT65XX_PDN_MM_VBUF, "Video_VBUF");
        disable_clock(MT_CG_VDEC0_VDE, "VideoDec");
        disable_clock(MT_CG_VENC_VEN, "VideoEnc");
        disable_clock(MT65XX_PDN_MM_GDC_SHARE_MACRO, "VideoEnc");
    }
*/
    MFV_LOGD("vcodec_early_suspend - tid = %d\n", current->pid);
}

static void vcodec_late_resume(struct early_suspend *h)
{
    mutex_lock(&PWRLock);
    MFV_LOGE("vcodec_late_resume, tid = %d, PWR_USER = %d\n", current->pid, gu4PWRCounter);
    mutex_unlock(&PWRLock);
/*
    if (gu4PWRCounter != 0)
    {
        MFV_LOGE("[vcodec_late_resume] Someone Use HW, Enable Power!\n");
        enable_clock(MT65XX_PDN_MM_VBUF, "Video_VBUF");
        enable_clock(MT_CG_VDEC0_VDE, "VideoDec");
        enable_clock(MT_CG_VENC_VEN, "VideoEnc");
        enable_clock(MT65XX_PDN_MM_GDC_SHARE_MACRO, "VideoEnc");
    }
*/
    MFV_LOGD("vcodec_late_resume - tid = %d\n", current->pid);
}

static struct early_suspend vcodec_early_suspend_handler =
{
    .level = (EARLY_SUSPEND_LEVEL_DISABLE_FB - 1),
    .suspend = vcodec_early_suspend,
    .resume = vcodec_late_resume,
};
#endif

static struct file_operations vcodec_fops = {
    .owner      = THIS_MODULE,
    .unlocked_ioctl = vcodec_unlocked_ioctl,
    .open       = vcodec_open,
    .flush      = vcodec_flush,
    .release    = vcodec_release,
    .mmap       = vcodec_mmap,
};

static int vcodec_probe(struct platform_device *dev)
{
    int ret;
    MFV_LOGD("+vcodec_probe\n");

    mutex_lock(&DecEMILock);
    gu4DecEMICounter = 0;
    mutex_unlock(&DecEMILock);

    mutex_lock(&EncEMILock);
    gu4EncEMICounter = 0;
    mutex_unlock(&EncEMILock);

    mutex_lock(&PWRLock);
    gu4PWRCounter = 0;
    mutex_unlock(&PWRLock);

    ret = register_chrdev_region(vcodec_devno, 1, VCODEC_DEVNAME);
    if (ret) {
        MFV_LOGD("[MFV_DEBUG][ERROR] Can't Get Major number for VCodec Device\n");
    }

    vcodec_cdev = cdev_alloc();
    vcodec_cdev->owner = THIS_MODULE;
    vcodec_cdev->ops = &vcodec_fops;

    ret = cdev_add(vcodec_cdev, vcodec_devno, 1);

    if (request_irq(MT_VDEC_IRQ_ID , (irq_handler_t)video_intr_dlr, IRQF_TRIGGER_HIGH, VCODEC_DEVNAME, NULL) < 0)
    {
       MFV_LOGD("[MFV_DEBUG][ERROR] error to request dec irq\n"); 
    }
    else
    {
       MFV_LOGD("[MFV_DEBUG] success to request dec irq\n");
    }

    if (request_irq(MT_VENC_IRQ_ID , (irq_handler_t)video_intr_dlr2, IRQF_TRIGGER_LOW, VCODEC_DEVNAME, NULL) < 0)
    {
       MFV_LOGD("[MFV_DEBUG][ERROR] error to request enc irq\n"); 
    }
    else
    {
       MFV_LOGD("[MFV_DEBUG] success to request enc irq\n");
    }

    disable_irq(MT_VDEC_IRQ_ID);
    disable_irq(MT_VENC_IRQ_ID);

    // [TODO] request IRQ here

    MFV_LOGD("[MFV_DEBUG] vcodec_probe Done\n");

    return 0;
}

#ifdef CONFIG_MTK_HIBERNATION
extern void mt_irq_set_sens(unsigned int irq, unsigned int sens);
extern void mt_irq_set_polarity(unsigned int irq, unsigned int polarity);
static int vcodec_pm_restore_noirq(struct device *device)
{
    // vdec : IRQF_TRIGGER_RISING
    mt_irq_set_sens(MT_VDEC_IRQ_ID, MT65xx_LEVEL_SENSITIVE);
    mt_irq_set_polarity(MT_VDEC_IRQ_ID, MT65xx_POLARITY_HIGH);
    // venc: IRQF_TRIGGER_LOW
    mt_irq_set_sens(MT_VENC_IRQ_ID, MT65xx_LEVEL_SENSITIVE);
    mt_irq_set_polarity(MT_VENC_IRQ_ID, MT65xx_POLARITY_LOW);

    return 0;
}
#endif

static int __init vcodec_driver_init(void)
{
    int i, j;
    VAL_RESULT_T  eValHWLockRet;
    unsigned long ulFlags, ulFlagsLockHW, ulFlagsISR;

    MFV_LOGD("+vcodec_init\n");

    MT6589Driver_Open_Count = 0;

    KVA_VENC_IRQ_STATUS_ADDR =  (int)ioremap(VENC_IRQ_STATUS_addr, 4);
    KVA_VENC_IRQ_ACK_ADDR  = (int)ioremap(VENC_IRQ_ACK_addr, 4);
    KVA_VENC_MP4_IRQ_ACK_ADDR  = (int)ioremap(VENC_MP4_IRQ_ACK_addr, 4);
    KVA_VENC_MP4_IRQ_STATUS_ADDR =  (int)ioremap(VENC_MP4_IRQ_STATUS_addr, 4);
    KVA_VENC_ZERO_COEF_COUNT_ADDR = (int)ioremap(VENC_ZERO_COEF_COUNT_addr, 4);
    KVA_VENC_BYTE_COUNT_ADDR = (int)ioremap(VENC_BYTE_COUNT_addr, 4);
    KVA_VENC_MP4_IRQ_ENABLE_ADDR = (int)ioremap(VENC_MP4_IRQ_ENABLE_addr, 4);
#ifdef VENC_PWR_FPGA
    KVA_VENC_CLK_CFG_0_ADDR = (int)ioremap(CLK_CFG_0_addr, 4);
    KVA_VENC_CLK_CFG_4_ADDR = (int)ioremap(CLK_CFG_4_addr, 4);
    KVA_VENC_PWR_ADDR = (int)ioremap(VENC_PWR_addr, 4);
    KVA_VENCSYS_CG_SET_ADDR = (int)ioremap(VENCSYS_CG_SET_addr, 4);
#endif

#ifdef DEBUG_MP4_ENC
    // DEBUG
    KVA_VENC_MP4_STATUS_ADDR = (int)ioremap(VENC_MP4_STATUS_addr, 4);
    KVA_VENC_MP4_MVQP_STATUS_ADDR = (int)ioremap(VENC_MP4_MVQP_STATUS_addr, 4);
#endif
#ifdef DEBUG_INFRA_POWER
    KVA_INFRA_ADDR = (int)ioremap(INFRA_addr, 4);
    KVA_CHECK_1_ADDR = (int)ioremap(CHECK_1_addr, 4);
    KVA_CHECK_2_ADDR = (int)ioremap(CHECK_2_addr, 4);
#endif

    spin_lock_irqsave(&LockDecHWCountLock, ulFlagsLockHW);
    gu4LockDecHWCount = 0;
    spin_unlock_irqrestore(&LockDecHWCountLock, ulFlagsLockHW);
    
    spin_lock_irqsave(&LockEncHWCountLock, ulFlagsLockHW);
    gu4LockEncHWCount = 0;
    spin_unlock_irqrestore(&LockEncHWCountLock, ulFlagsLockHW);
    
    spin_lock_irqsave(&DecISRCountLock, ulFlagsISR);
    gu4DecISRCount = 0;
    spin_unlock_irqrestore(&DecISRCountLock, ulFlagsISR);
    
    spin_lock_irqsave(&EncISRCountLock, ulFlagsISR);
    gu4EncISRCount = 0;
    spin_unlock_irqrestore(&EncISRCountLock, ulFlagsISR);

    mutex_lock(&IsOpenedLock);
    if (VAL_FALSE == bIsOpened) {
        bIsOpened = VAL_TRUE;
        vcodec_probe(NULL);
    }
    mutex_unlock(&IsOpenedLock);

    mutex_lock(&VdecHWLock);
    grVcodecDecHWLock.pvHandle = 0;
    grVcodecDecHWLock.eDriverType = VAL_DRIVER_TYPE_NONE;
    grVcodecDecHWLock.rLockedTime.u4Sec = 0;
    grVcodecDecHWLock.rLockedTime.u4uSec = 0;
    mutex_unlock(&VdecHWLock);

    mutex_lock(&VencHWLock);
    grVcodecEncHWLock.pvHandle = 0;
    grVcodecEncHWLock.eDriverType = VAL_DRIVER_TYPE_NONE;
    grVcodecEncHWLock.rLockedTime.u4Sec = 0;
    grVcodecEncHWLock.rLockedTime.u4uSec = 0;
    mutex_unlock(&VencHWLock);
    
    mutex_lock(&NonCacheMemoryListLock);
    for (i = 0; i < VCODEC_MULTIPLE_INSTANCE_NUM_x_10; i++) {
        grNonCacheMemoryList[i].pvHandle = 0x0;
        for (j = 0; j < VCODEC_THREAD_MAX_NUM; j++)
        {
            grNonCacheMemoryList[i].u4VCodecThreadID[j] = 0xffffffff;
        }
        grNonCacheMemoryList[i].u4KVA = 0xffffffff;
        grNonCacheMemoryList[i].u4KPA = 0xffffffff;
    }
    mutex_unlock(&NonCacheMemoryListLock);

    spin_lock_irqsave(&OalHWContextLock, ulFlags);
    for (i = 0; i < VCODEC_MULTIPLE_INSTANCE_NUM; i++) {
        oal_hw_context[i].Oal_HW_reg = (VAL_VCODEC_OAL_HW_REGISTER_T  *)0;
        oal_hw_context[i].ObjId = -1;
        oal_hw_context[i].slotindex = i;
        oal_hw_context[i].u4VCodecThreadNum = VCODEC_THREAD_MAX_NUM;
        for (j = 0; j < VCODEC_THREAD_MAX_NUM; j++)
        {
            oal_hw_context[i].u4VCodecThreadID[j] = -1;
        }
        oal_hw_context[i].pvHandle = 0;
        oal_hw_context[i].u4NumOfRegister = 0;

        for (j = 0; j < OALMEM_STATUS_NUM ; j ++) {
            oal_hw_context[i].oalmem_status[j].u4ReadAddr = 0;
            oal_hw_context[i].oalmem_status[j].u4ReadData = 0;
        }
    }
    spin_unlock_irqrestore(&OalHWContextLock, ulFlags);

    //MT6589_HWLockEvent part
    mutex_lock(&DecHWLockEventTimeoutLock);
    DecHWLockEvent.pvHandle = "DECHWLOCK_EVENT";
    DecHWLockEvent.u4HandleSize = sizeof("DECHWLOCK_EVENT")+1;
    DecHWLockEvent.u4TimeoutMs = 1;
    mutex_unlock(&DecHWLockEventTimeoutLock);
    eValHWLockRet = eVideoCreateEvent(&DecHWLockEvent, sizeof(VAL_EVENT_T));
    if (VAL_RESULT_NO_ERROR != eValHWLockRet) {
        MFV_LOGE("[MFV][ERROR] create dec hwlock event error\n");
    }

    mutex_lock(&EncHWLockEventTimeoutLock);
    EncHWLockEvent.pvHandle = "ENCHWLOCK_EVENT";
    EncHWLockEvent.u4HandleSize = sizeof("ENCHWLOCK_EVENT")+1;
    EncHWLockEvent.u4TimeoutMs = 1;
    mutex_unlock(&EncHWLockEventTimeoutLock);
    eValHWLockRet = eVideoCreateEvent(&EncHWLockEvent, sizeof(VAL_EVENT_T));
    if (VAL_RESULT_NO_ERROR != eValHWLockRet) {
        MFV_LOGE("[MFV][ERROR] create enc hwlock event error\n");
    }

    //MT6589_IsrEvent part
    spin_lock_irqsave(&DecIsrLock, ulFlags);
    DecIsrEvent.pvHandle = "DECISR_EVENT";
    DecIsrEvent.u4HandleSize = sizeof("DECISR_EVENT")+1;
    DecIsrEvent.u4TimeoutMs = 1;
    spin_unlock_irqrestore(&DecIsrLock, ulFlags);
    eValHWLockRet = eVideoCreateEvent(&DecIsrEvent, sizeof(VAL_EVENT_T));
    if(VAL_RESULT_NO_ERROR != eValHWLockRet)
    {
        MFV_LOGE("[MFV][ERROR] create dec isr event error\n");
    }

    spin_lock_irqsave(&EncIsrLock, ulFlags);
    EncIsrEvent.pvHandle = "ENCISR_EVENT";
    EncIsrEvent.u4HandleSize = sizeof("ENCISR_EVENT")+1;
    EncIsrEvent.u4TimeoutMs = 1;
    spin_unlock_irqrestore(&EncIsrLock, ulFlags);
    eValHWLockRet = eVideoCreateEvent(&EncIsrEvent, sizeof(VAL_EVENT_T));
    if(VAL_RESULT_NO_ERROR != eValHWLockRet)
    {
        MFV_LOGE("[MFV][ERROR] create enc isr event error\n");
    }



    MFV_LOGD("[MFV_DEBUG] mflexvideo_driver_init Done\n");

#ifdef CONFIG_HAS_EARLYSUSPEND
    register_early_suspend(&vcodec_early_suspend_handler);
#endif

#ifdef CONFIG_MTK_HIBERNATION
    register_swsusp_restore_noirq_func(ID_M_VCODEC, vcodec_pm_restore_noirq, NULL);
#endif

    return 0;
}

static void __exit vcodec_driver_exit(void)
{
    VAL_RESULT_T  eValHWLockRet;

    MFV_LOGD("[MFV_DEBUG] mflexvideo_driver_exit\n");

    mutex_lock(&IsOpenedLock);
    if (VAL_TRUE == bIsOpened) {
        bIsOpened = VAL_FALSE;
    }
    mutex_unlock(&IsOpenedLock);

    cdev_del(vcodec_cdev);
    unregister_chrdev_region(vcodec_devno, 1);

    // [TODO] iounmap the following?
    iounmap((void*)KVA_VENC_IRQ_STATUS_ADDR);
    iounmap((void*)KVA_VENC_IRQ_ACK_ADDR);   
    iounmap((void*)KVA_VENC_MP4_IRQ_ACK_ADDR);
    iounmap((void*)KVA_VENC_MP4_IRQ_STATUS_ADDR);
    iounmap((void*)KVA_VENC_ZERO_COEF_COUNT_ADDR);
    iounmap((void*)KVA_VENC_BYTE_COUNT_ADDR);
    iounmap((void*)KVA_VENC_MP4_IRQ_ENABLE_ADDR);
#ifdef VENC_PWR_FPGA
    iounmap((void*)KVA_VENC_CLK_CFG_0_ADDR);
    iounmap((void*)KVA_VENC_CLK_CFG_4_ADDR);
    iounmap((void*)KVA_VENC_PWR_ADDR);
    iounmap((void*)KVA_VENCSYS_CG_SET_ADDR);
#endif
#ifdef DEBUG_MP4_ENC
    iounmap((void*)KVA_VENC_MP4_STATUS_ADDR);
    iounmap((void*)KVA_VENC_MP4_MVQP_STATUS_ADDR);
#endif
#ifdef DEBUG_INFRA_POWER
    iounmap((void*)KVA_INFRA_ADDR);    
    iounmap((void*)KVA_CHECK_1_ADDR);    
    iounmap((void*)KVA_CHECK_2_ADDR);
#endif

    // [TODO] free IRQ here


    //MT6589_HWLockEvent part
    eValHWLockRet = eVideoCloseEvent(&DecHWLockEvent, sizeof(VAL_EVENT_T));
    if (VAL_RESULT_NO_ERROR != eValHWLockRet) {
        MFV_LOGE("[MFV][ERROR] close dec hwlock event error\n");
    }

    eValHWLockRet = eVideoCloseEvent(&EncHWLockEvent, sizeof(VAL_EVENT_T));
    if (VAL_RESULT_NO_ERROR != eValHWLockRet) {
        MFV_LOGE("[MFV][ERROR] close enc hwlock event error\n");
    }

    //MT6589_IsrEvent part
    eValHWLockRet = eVideoCloseEvent(&DecIsrEvent, sizeof(VAL_EVENT_T));
    if (VAL_RESULT_NO_ERROR != eValHWLockRet) {
        MFV_LOGE("[MFV][ERROR] close dec isr event error\n");
    }

    eValHWLockRet = eVideoCloseEvent(&EncIsrEvent, sizeof(VAL_EVENT_T));
    if (VAL_RESULT_NO_ERROR != eValHWLockRet) {
        MFV_LOGE("[MFV][ERROR] close enc isr event error\n");
    }



#ifdef CONFIG_HAS_EARLYSUSPEND
    unregister_early_suspend(&vcodec_early_suspend_handler);
#endif

#ifdef CONFIG_MTK_HIBERNATION
    unregister_swsusp_restore_noirq_func(ID_M_VCODEC);
#endif
}

module_init(vcodec_driver_init);
module_exit(vcodec_driver_exit);
MODULE_AUTHOR("Legis, Lu <legis.lu@mediatek.com>");
MODULE_DESCRIPTION("MT6589 Vcodec Driver");
MODULE_LICENSE("GPL");

