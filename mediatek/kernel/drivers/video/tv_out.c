#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <asm/cacheflush.h>
#include <linux/list.h>
#include <linux/semaphore.h>




#include <mach/irqs.h>
//#include <asm/tcm.h>
#include <asm/io.h>
//#include <mach/mtk_mau.h>
//#include <mach/sync_write.h>
#include <mach/m4u.h>
#include <mach/mt_typedefs.h>
//#include <mach/mt_boot.h>
#include <mach/mt_reg_base.h>
//#include <mach/mt_pmic_feature_api.h> //PMIC Analog switch select

#include "tv_def.h"
#include "tvc_drv.h"
#include "tve_drv.h"
#include "tvrot_drv.h"
#include "tv_out.h"
#include "disp_drv.h"
#include "lcd_drv.h"
#include "lcm_drv.h"
#include <linux/xlog.h>


#define TV_VER(fmt, arg...)      xlog_printk( ANDROID_LOG_DEBUG, "TV/OUT", "[VER]: %s() "fmt"\n", __FUNCTION__, ##arg)
#define TV_DBG(fmt, arg...)      xlog_printk( ANDROID_LOG_DEBUG, "TV/OUT", "[DBG]: %s() "fmt"\n", __FUNCTION__, ##arg)
#define TV_INFO(fmt, arg...)     xlog_printk( ANDROID_LOG_INFO, "TV/OUT", "[INFO]: %s() "fmt"\n", __FUNCTION__, ##arg)
#define TV_WARN(fmt, arg...)     xlog_printk( ANDROID_LOG_WARN, "TV/OUT", "[WARN]: %s() "fmt"\n", __FUNCTION__, ##arg)
#define TV_ERROR(fmt, arg...)    xlog_printk( ANDROID_LOG_ERROR, "TV/OUT", "[ERR]: %s(): %s@%d "fmt"\n", __FUNCTION__, __FILE__,__LINE__,##arg)
#define TV_FATAL(fmt, arg...)    xlog_printk( ANDROID_LOG_FATAL, "TV/OUT", "[FATAL]: %s(): %s@%d "fmt"\n", __FUNCTION__, __FILE__,__LINE__,##arg)

#define TV_LOG(fmt, arg...) \
    do { \
        if (tv_out_log_on) xlog_printk( ANDROID_LOG_INFO, "TV/OUT", "[LOG]: %s()" fmt"\n", __FUNCTION__, ##arg); \
    }while (0)

//#define TVOUT_PORTING_WAIT_OTHER_READY


#if defined (MTK_TVOUT_SUPPORT)
//--------------------------------------------Define-----------------------------------------------//


//DECLARE_MUTEX(sem_tvout_screen_update);
DEFINE_SEMAPHORE(sem_tvout_screen_update);

#if 1
#define SCREEN_UPDATE_LOCK() \
    do { if (down_interruptible(&sem_tvout_screen_update)) { \
            printk("[TV] ERROR: Can't get sem_tvout_screen_update" \
                   " in %s()\n", __func__); \
            return TVOUT_STATUS_ERROR; \
        }} while (0)
#define SCREEN_UPDATE_RELEASE() \
    do { up(&sem_tvout_screen_update); } while (0)
#else
#define SCREEN_UPDATE_LOCK() \
    do { printk("[TV]%s: lock \n",__func__ ); \
        if (down_interruptible(&sem_tvout_screen_update)) { \
            printk("[TV] ERROR: Can't get sem_tvout_screen_update" \
                   " in %s()\n", __func__); \
            return TVOUT_STATUS_ERROR; \
         } \
	} while (0)
#define SCREEN_UPDATE_RELEASE() \
    do { up(&sem_tvout_screen_update); printk("[TV]%s: unlock\n", __func__); } while (0)
#endif

// This mutex is used to reslove wait check line inter time out issue when disable TV-out
// during video playback.
// When video playback, TVC will update registers each frame,
// and wait check line inter, if we disable TV-out at this time,
// TVC will not generate any inter, so time out occurs.
// So, use this mutex to let 'disable' take effect after TVC update registers.
DEFINE_SEMAPHORE(sem_video_update);
#define VIDEO_UPDATE_LOCK() \
    do { if (down_interruptible(&sem_video_update)) { \
            printk("[TV] ERROR: Can't get sem_video_update" \
                   " in %s()\n", __func__); \
            return TVOUT_STATUS_ERROR; \
        }} while (0)
#define VIDEO_UPDATE_RELEASE() \
    do { up(&sem_video_update); } while (0)



//DECLARE_MUTEX(sem_tvout_status);
DEFINE_SEMAPHORE(sem_tvout_status);
#define TVOUT_STATUS_LOCK() \
        do { if (down_interruptible(&sem_tvout_status)) { \
                printk("[TV] ERROR: Can't get sem_tvout_status" \
                       " in %s()\n", __func__); \
                return TVOUT_STATUS_ERROR; \
            }} while (0)
#define TVOUT_STATUS_UNLOCK() \
        do { up(&sem_tvout_status); } while (0)


/* tvout working buffer cnt */
#define TVOUT_BUFFERS (4)

#define TVOUT_BUFFER_WORK_QUEUE
#define TVOUT_BUFFER_BPP (2)

#define ALIGN_TO_POW_OF_2(x, n)  \
	(((x) + ((n) - 1)) & ~((n) - 1))

typedef struct
{
    struct list_head    vd_link;
    unsigned int        vd_mva;
    unsigned int        vd_va;
    unsigned int        vd_size;
} tvout_video_buffer;

struct list_head tvout_video_buffer_list;


typedef struct
{
    UINT32 pa;
    UINT32 pitchInBytes;
} TempBuffer;

typedef struct
{
    BOOL         	isEnabled;
    TVOUT_MODE   	tvMode;
    TVOUT_SYSTEM 	tvSystem;
    TVOUT_ROT    	orientation;

    UINT32 			targetWidth, targetHeight;

    TempBuffer     	tmpBuffers[TVOUT_BUFFERS];
    UINT32         	activeTmpBufferIdx;

    // Video playback mode parameters
    UINT32 			videoPa;
    TVOUT_SRC_FORMAT 	videoFormat;
    bool            isVideoM4uEnabled;
    UINT32 			videoWidth, videoHeight;
} _TVOUT_CONTEXT;

static _TVOUT_CONTEXT tvout_context = {0};


#if defined(MTK_TVOUT_ENABLE_M4U)
	//static bool is_m4u_tvc_port_closed = false;
	static unsigned char * tmpBufVAForM4U;
	static UINT32 mva_tvc=0;
	static UINT32 mva_tvr=0;
    static UINT32 mva_size=0;
#endif


//static bool is_tv_cable_plug_in = false;
static bool is_tv_enabled = false;
static bool is_tv_enabled_before_suspend = false;
static bool is_engine_in_suspend_mode = false;
static bool is_tv_out_turned_on    = false;
static bool is_tv_cable_plugged_in = false;
static bool is_tv_display_colorbar = false;
static bool is_tv_initialized_done = false;
static bool is_tv_out_enable_failed = false;

static bool is_tv_out_closed = false;
static bool is_tv_enable_time_profiling = false;

static int is_disable_video_mode = 0;
static int is_tv_out_in_hqa_mode = false;
static bool tv_out_log_on = false;
static TVOUT_SYSTEM tv_system_backup = TVOUT_SYSTEM_NTSC;


static struct work_struct tvout_work;
static struct workqueue_struct * tvout_workqueue = NULL;

/*for video mode memcpy buffer*/
static unsigned char* video_buffer_addr = NULL;
static unsigned int video_buffer_size = 0;
static unsigned int video_buffer_mva;
//static int video_buffer_active_idx = 0;
//static int video_buffer_ready_idx = 0;
//static int video_buffer_busy_idx = 0;

typedef struct {
    unsigned int idx;
    unsigned int stats; //0:active 1:ready 2:busy
} video_buffer;
#define VIDEO_BUFFER_CNT 2

#define VIDEO_BUFIDX_INC(x) x = (x == (VIDEO_BUFFER_CNT-1) ? 0 : x+1)

static unsigned int _GetTime(void)
{
	struct timeval time;
	do_gettimeofday(&time);
	return (time.tv_sec*1000 + time.tv_usec/1000);
}


// ---------------------------------
//  TVOut Time profiling-start
// ---------------------------------

static unsigned int tv_time_start;
static unsigned int tv_time_end;


#define TV_TIME_PROFILE_START()     \
    do {                            \
        if (is_tv_enable_time_profiling) \
            tv_time_start = _GetTime(); \
    } while (0)

#define TV_TIME_PROFILE_END()       \
    do {                            \
        if (is_tv_enable_time_profiling) {\
            tv_time_end = _GetTime();   \
            TV_INFO("Time: %u\n", tv_time_end-tv_time_start); \
        } \
    } while (0)

UINT32 tvout_get_vram_size(void)
{
	UINT32 vramsize;
	vramsize = DISP_GetScreenWidth() *
			   DISP_GetScreenHeight() *
			   TVOUT_BUFFER_BPP * TVOUT_BUFFERS;
#if !defined(MTK_TVOUT_ENABLE_M4U)
	vramsize = ALIGN_TO_POW_OF_2(vramsize, 0x100000);
#else
	vramsize += 1;
#endif
	TV_INFO(" %u bytes", vramsize);

	return vramsize;
}


// ---------------------------------
//  TVOut Buffer Control-start
// ---------------------------------

static UINT32 tvr_buf_offset = 0;

//DECLARE_MUTEX(sem_tvout_buffer);
DEFINE_SEMAPHORE(sem_tvout_buffer);

static inline UINT32 TVBUF_LOCK(void)
{
    if (down_interruptible(&sem_tvout_buffer))
    {
        TV_WARN("Can't get sem_tvout_buffer");
        return -1;
    }
    return 0;
}

static inline UINT32 TVBUF_TRY_LOCK(void)
{
    if (down_trylock(&sem_tvout_buffer) != 0)
    {
        TV_WARN("Can't get sem_tvout_buffer");
        return -1;
    }
    return 0;

}

static inline UINT32 TVBUF_UNLOCK(void)
{
    up(&sem_tvout_buffer);
    return 0;
}



typedef enum
{
    TV_BUF_IDLE = 0,
    TV_BUF_EMPTY, //init status of buffers
    TV_BUF_FULL,  //buffer can be shown on TV
    TV_BUF_BUSY,  //buffer is shown on TV(woring on by TVC)
    TV_BUF_IN_QUEUE, //The buffer is in TVR descrpiter queue
    TV_BUF_STAT_ALL
}   TV_BUF_STAT;

/*
    TVR: EMPTY->IN_QUEUE->FULL->IN_QUEUE
    TVC: FULL->BUSY->FULL_>BUSY
*/


typedef struct
{
    UINT32          buf_id;
    UINT32          buf_va;
    UINT32          buf_pa;
#if defined MTK_TVOUT_ENABLE_M4U
    UINT32          buf_mva_tvc;
    UINT32          buf_mva_tvr;
#endif
    TV_BUF_STAT     buf_sts;
    UINT32          buf_timestamp;
} TV_Buffer;

/*  for image mode */
static TV_Buffer tvBuf[TVOUT_BUFFERS];

static bool     is_tv_buffer_inited = false;
static UINT32   tvr_active_buf_addr;

/* for video mode */
static TV_Buffer tvVdoBuf[TVOUT_BUFFERS];
static bool     is_tv_vdo_buffer_inited = false;



char const* const bufStatName[TV_BUF_STAT_ALL] =
{
    [TV_BUF_IDLE] = "IDLE",
    [TV_BUF_EMPTY] = "Empty",
    [TV_BUF_FULL] = "Full",
    [TV_BUF_BUSY] = "Busy",
    [TV_BUF_IN_QUEUE] = "In Queue"
};




UINT32 _cal_tvr_buf_offset(TVOUT_ROT rot, UINT32 srcWidth, UINT32 srcHeight)
{
    UINT32 dstPitchInBytes = (rot & 0x1) ? \
          (srcHeight * 2) : (srcWidth * 2);
    switch(rot)
    {
        case TVOUT_ROT_180 :
            return dstPitchInBytes * (srcHeight - 1);
        case TVOUT_ROT_270 :
            return dstPitchInBytes * (srcWidth - 1);
        default :
            return 0;
    }
}
extern unsigned int m4u_user_v2p(unsigned int va);
#if 0
static unsigned int _user_v2p(unsigned int va)
{
    unsigned int pmdOffset = (va & (PMD_SIZE - 1));
    unsigned int pageOffset = (va & (PAGE_SIZE - 1));
    pgd_t *pgd;
    pmd_t *pmd;
    pte_t *pte;
    unsigned int pa;

    if(NULL==current)
    {
    	  TV_ERROR("error: m4u_user_v2p, current is NULL! \n");
    	  return 0;
    }
    if(NULL==current->mm)
    {
    	  TV_ERROR("error: m4u_user_v2p, current->mm is NULL! tgid=0x%x, name=%s \n", current->tgid, current->comm);
    	  return 0;
    }

    pgd = pgd_offset(current->mm, va); /* what is tsk->mm */

    if(pgd_none(*pgd)||pgd_bad(*pgd))
    {
        TV_ERROR("warning: m4u_user_v2p(), va=0x%x, pgd invalid! \n", va);
        return 0;
    }

    pmd = pmd_offset(pgd, va);

    /* If this is a page table entry, keep on walking to the next level */
    if (( (unsigned int)pmd_val(*pmd) & PMD_TYPE_MASK) == PMD_TYPE_TABLE)
    {
        if(pmd_none(*pmd)||pmd_bad(*pmd))
        {
            TV_ERROR("warning: m4u_user_v2p(), va=0x%x, pmd invalid! \n", va);
            return 0;
        }

        // we encounter some pte not preset issue, do not know why
        pte = pte_offset_map(pmd, va);
        if(pte_present(*pte))
        {
            pa=(pte_val(*pte) & (PAGE_MASK)) | pageOffset;
            //TV_ERROR("PA = 0x%8x\n", pa);
            return pa;
        }
    }
    else /* Only 1 level page table */
    {
       if(pmd_none(*pmd))
       {
          TV_ERROR("Error: m4u_user_v2p(), virtual addr 0x%x, pmd invalid! \n", va);
          return 0;
       }
       pa=(pte_val(*pmd) & (PMD_MASK)) | pmdOffset;
       return pa;
    }

    TV_LOG("warning: m4u_user_v2p(), pte invalid! ");

    return 0;
}
#endif




#if defined MTK_TVOUT_ENABLE_M4U
TVOUT_STATUS tvbuf_Init(void)
{
    M4U_PORT_STRUCT M4uPort;
    UINT32 _mva_tvc, _mva_tvr;
    UINT32 _buf_va;
    UINT32 tmpFbPitchInBytes, tmpFbSizeInBytes;
    int i;

    if (is_tv_buffer_inited)
    {
        TV_WARN("TV Buffer has been inited, but init again!!");
        return TVOUT_STATUS_OK;
    }
    memset(tvBuf, 0, sizeof(TV_Buffer)*TVOUT_BUFFERS);

    TV_INFO("using M4U!");


    mva_size = tvout_get_vram_size();
	tmpBufVAForM4U = (unsigned char*)vmalloc(mva_size);
	if (tmpBufVAForM4U == NULL)
    {
		TV_ERROR("vmalloc failed\n");
		return TVOUT_STATUS_ERROR;
	}

	TV_LOG("%0x",  (UINT32)tmpBufVAForM4U );
	memset(tmpBufVAForM4U, 0, mva_size);


    //Allocate MVA for TVROT
    if (TVR_AllocMva((unsigned int)tmpBufVAForM4U, mva_size, &mva_tvr) != TVR_STATUS_OK)
        return TVOUT_STATUS_ERROR;

    _mva_tvr = mva_tvr;
	M4uPort.ePortID = M4U_PORT_TV_ROT_OUT0;
	M4uPort.Virtuality = 1;
	M4uPort.Security = 0;
	M4uPort.Distance = 1;
	M4uPort.Direction = 0;
	m4u_config_port(&M4uPort);
	//_m4u_tvout_func.m4u_dump_reg();

    //Allocate MVA for TVC
    if (TVC_AllocMva((unsigned int)tmpBufVAForM4U, mva_size, &mva_tvc) != TVC_STATUS_OK)
        return TVOUT_STATUS_ERROR;

	TV_LOG("MVA for TVC: 0x%x", mva_tvc);

    _mva_tvc = mva_tvc;
    M4uPort.ePortID = M4U_PORT_TVC;
	M4uPort.Virtuality = 1;
	M4uPort.Security = 0;
	M4uPort.Distance = 1;
	M4uPort.Direction = 0;
	m4u_config_port(&M4uPort);


    tmpFbPitchInBytes = DISP_GetScreenWidth() * TVOUT_BUFFER_BPP;
    tmpFbSizeInBytes = tmpFbPitchInBytes * DISP_GetScreenHeight();

    _buf_va = (UINT32)tmpBufVAForM4U;

    for ( i=0; i < TVOUT_BUFFERS; i++)
    {
        TV_Buffer* b    = &tvBuf[i];
        b->buf_mva_tvr  = _mva_tvr;
        b->buf_mva_tvc  = _mva_tvc;
        b->buf_va       = _buf_va;
        b->buf_id       = i;
        b->buf_sts      = TV_BUF_EMPTY;
        b->buf_timestamp = 0;
        ASSERT((b->buf_mva_tvr & 0x7) == 0);
        ASSERT((b->buf_mva_tvc & 0x7) == 0);
        _mva_tvr += tmpFbSizeInBytes;
        _mva_tvc += tmpFbSizeInBytes;
        _buf_va += tmpFbSizeInBytes;
        TV_LOG("buf id:%d MVA TVR:0x%x MVA TVC:0x%x va:0x%x sts:%d time:%u",
                tvBuf[i].buf_id,
                tvBuf[i].buf_mva_tvr,
                tvBuf[i].buf_mva_tvc,
                tvBuf[i].buf_va,
                tvBuf[i].buf_sts,
                tvBuf[i].buf_timestamp);
    }

    is_tv_buffer_inited = true;
    return TVOUT_STATUS_OK;


}

TVOUT_STATUS tvbuf_Deinit(void)
{
    M4U_PORT_STRUCT M4uPort;

    if (!is_tv_buffer_inited)
    {
        TV_WARN("TV Buffer is not inited, no need to deinit");
        return TVOUT_STATUS_OK;
    }


	TV_LOG("Disable M4U for TVC");
    M4uPort.ePortID = M4U_PORT_TV_ROT_OUT0;
	M4uPort.Virtuality = 0;
	M4uPort.Security = 0;
	M4uPort.Distance = 1;
	M4uPort.Direction = 0;
	m4u_config_port(&M4uPort);

	TV_LOG("Disable M4U for TVR");
	M4uPort.ePortID = M4U_PORT_TVC;
	M4uPort.Virtuality = 0;
	M4uPort.Security = 0;
	M4uPort.Distance = 1;
	M4uPort.Direction = 0;
	m4u_config_port(&M4uPort);


    TVC_DeallocMva((UINT32)tmpBufVAForM4U, tvout_get_vram_size(), mva_tvc);
    TVR_DeallocMva((UINT32)tmpBufVAForM4U, tvout_get_vram_size(), mva_tvr);

	if (tmpBufVAForM4U) {vfree(tmpBufVAForM4U);}

    is_tv_buffer_inited = false;
    return TVOUT_STATUS_OK;
}

TVOUT_STATUS tv_video_buf_init(void)
{
    UINT32 _mva_tvc, _mva_tvr;
    UINT32 _buf_va;
    int i;

    if (is_tv_vdo_buffer_inited)
    {
        TV_WARN("TV Buffer has been inited, but init again!!");
        return TVOUT_STATUS_OK;
    }
    memset(tvVdoBuf, 0, sizeof(TV_Buffer)*TVOUT_BUFFERS);



	video_buffer_addr = (unsigned char*)vmalloc(video_buffer_size*TVOUT_BUFFERS);
	if (video_buffer_addr == NULL)
    {
		TV_ERROR("vmalloc failed\n");
		return TVOUT_STATUS_ERROR;
	}

	TV_LOG("%0x",  (UINT32)video_buffer_addr );

    //Allocate MVA for TVC
    if (TVC_AllocMva((unsigned int)video_buffer_addr, video_buffer_size * TVOUT_BUFFERS, &video_buffer_mva) != TVC_STATUS_OK)
        return TVOUT_STATUS_ERROR;

	TV_LOG("MVA for TVC: 0x%x", video_buffer_mva);


    _buf_va = (UINT32)video_buffer_addr;
    _mva_tvc = video_buffer_mva;

    for ( i=0; i < TVOUT_BUFFERS; i++)
    {
        TV_Buffer* b    = &tvVdoBuf[i];
        b->buf_mva_tvc  = _mva_tvc;
        b->buf_va       = _buf_va;
        b->buf_id       = i;
        b->buf_sts      = TV_BUF_EMPTY;
        b->buf_timestamp = 0;
        ASSERT((b->buf_mva_tvr & 0x7) == 0);
        ASSERT((b->buf_mva_tvc & 0x7) == 0);
        _mva_tvr += video_buffer_size;
        _mva_tvc += video_buffer_size;
        _buf_va += video_buffer_size;
        TV_LOG("buf id:%d MVA TVR:0x%x MVA TVC:0x%x va:0x%x sts:%d time:%u",
                tvVdoBuf[i].buf_id,
                tvVdoBuf[i].buf_mva_tvr,
                tvVdoBuf[i].buf_mva_tvc,
                tvVdoBuf[i].buf_va,
                tvVdoBuf[i].buf_sts,
                tvVdoBuf[i].buf_timestamp);
    }

    is_tv_vdo_buffer_inited = true;
    return TVOUT_STATUS_OK;


}

TVOUT_STATUS tv_video_buf_release(void)
{

    if (!is_tv_vdo_buffer_inited)
    {
        TV_WARN("TV Buffer is not inited, no need to deinit");
        return TVOUT_STATUS_OK;
    }

	if (video_buffer_addr) {
        TV_INFO("free video buffer");

        TVC_DeallocMva((UINT32)video_buffer_addr, video_buffer_size * TVOUT_BUFFERS, video_buffer_mva);
        vfree(video_buffer_addr);
        video_buffer_addr = NULL;
        memset(tvVdoBuf, 0, sizeof(TV_Buffer)*TVOUT_BUFFERS);
    }

    is_tv_vdo_buffer_inited = false;
    return TVOUT_STATUS_OK;
}

#else
TVOUT_STATUS tvbuf_Init(void)
{
    unsigned char* bufVa;
    UINT32 bufPa;
    UINT32 bufSize;
    UINT32 tmpFbPitchInBytes;
    UINT32 tmpFbSizeInBytes;
    int i;

    if (is_tv_buffer_inited)
    {
        TV_WARN("TV Buffer has been inited, but init again!!");
        return TVOUT_STATUS_OK;
    }
    memset(tvBuf, 0, sizeof(TV_Buffer)*TVOUT_BUFFERS);

    TV_INFO("using Kmalloc!");


    bufSize = DISP_GetScreenWidth() *
			  DISP_GetScreenHeight() *
			  TVOUT_BUFFER_BPP * TVOUT_BUFFERS;

    bufVa = kmalloc(bufSize, GFP_KERNEL);
    if (bufVa == NULL)
    {
        TV_ERROR("unable to allocate memory for TV-out\n");
        return TVOUT_STATUS_ERROR;
    }
    bufPa = virt_to_phys(bufVa);
    memset(bufVa, 0, bufSize);

    __cpuc_flush_kern_all();
    outer_clean_all();

    tmpFbPitchInBytes = DISP_GetScreenWidth() * TVOUT_BUFFER_BPP;
    tmpFbSizeInBytes = tmpFbPitchInBytes * DISP_GetScreenHeight();

    for ( i=0; i < TVOUT_BUFFERS; i++)
    {
        TV_Buffer* b    = &tvBuf[i];
        b->buf_pa       = (UINT32)bufPa;
        b->buf_va       = (UINT32)bufVa;
        b->buf_id       = i;
        b->buf_sts      = TV_BUF_EMPTY;
        b->buf_timestamp = 0;
        ASSERT((bufPa & 0x7) == 0);
        bufPa += tmpFbSizeInBytes;
        bufVa += tmpFbSizeInBytes;
        TV_LOG("buf id:%d pa:%d va:%d sts:%d time:%u",
                tvBuf[i].buf_id,
                tvBuf[i].buf_pa,
                tvBuf[i].buf_va,
                tvBuf[i].buf_sts,
                tvBuf[i].buf_timestamp);
    }


    is_tv_buffer_inited = true;
    return TVOUT_STATUS_OK;
}

TVOUT_STATUS tvbuf_Deinit(void)
{
    if (!is_tv_buffer_inited)
    {
        TV_WARN("TV Buffer is not inited, no need to deinit");
        return TVOUT_STATUS_OK;
    }
    kfree((unsigned char *)tvBuf[0].buf_va);



    is_tv_buffer_inited = false;
    return TVOUT_STATUS_OK;
}
#endif



void tvbuf_DumpInfo(void* buf)
{
    int i;
    UINT32 curTime = _GetTime();
    TV_Buffer * tvBuf = buf;
    if (buf == NULL) {
        TV_ERROR("NULL pointer");
        return;
    }

    TV_INFO("-----Dump TV Buf Info Start-----");

#if defined MTK_TVOUT_ENABLE_M4U
    for (i=0; i<TVOUT_BUFFERS; i++)
    {
        TV_INFO("ID:%d MVA TVR:0x%x MVA TVC:0x%x Status:%s time: %u",
            tvBuf[i].buf_id, tvBuf[i].buf_mva_tvr,
            tvBuf[i].buf_mva_tvc, bufStatName[tvBuf[i].buf_sts],
            curTime-tvBuf[i].buf_timestamp);
    }
#else
    for (i=0; i<TVOUT_BUFFERS; i++)
    {
        TV_INFO("ID:%d VA:0x%x PA:0x%x Status:%s time: %u",
            tvBuf[i].buf_id, tvBuf[i].buf_va,
            tvBuf[i].buf_pa, bufStatName[tvBuf[i].buf_sts],
            curTime-tvBuf[i].buf_timestamp);
    }
#endif
    TV_INFO("-----Dump TV Buf Info End-----");
}

inline UINT32 tvbuf_DumpStatus(void* buf)
{
    int i;
    UINT32 status = 0;

    TV_Buffer * tvBuf = buf;
    if (buf == NULL) {
        TV_ERROR("NULL pointer");
        return 0xf;
    }

    for (i=0; i<TVOUT_BUFFERS; i++)
    {
        status += tvBuf[i].buf_sts << (TVOUT_BUFFERS-i-1)*4;
    }

    return status;
}




inline void tvbuf_SetTimeStamp(void* buf, int ID)
{
    UINT32 curTime;

    TV_Buffer * tvBuf = buf;
    if (buf == NULL) {
        TV_ERROR("NULL pointer");
        return;
    }

    if (ID > (TVOUT_BUFFERS - 1) || ID < 0 )
    {
        TV_ERROR("invalid para %d\n", ID);
        return;
    }
    curTime = _GetTime();
    if (curTime < tvBuf[ID].buf_timestamp)
    {
        TV_ERROR("buffer time error %d %d\n", ID, curTime);
        tvbuf_DumpInfo(tvBuf);
        return;
    }

    tvBuf[ID].buf_timestamp = curTime;

    return;
}

//only reset not busy buffer(busy: TVC workiong on)
inline UINT32 tvbuf_Reset(void* buf)
{
    int i;
    TV_Buffer * tvBuf = buf;
    if (buf == NULL) {
        TV_ERROR("NULL pointer");
        return 0;
    }

    //TVBUF_LOCK();
    for (i = 0; i < TVOUT_BUFFERS; i++)
    {
        if (tvBuf[i].buf_sts != TV_BUF_BUSY)
        {
            TV_INFO("reset buf id:%d sts:%s", tvBuf[i].buf_id, bufStatName[tvBuf[i].buf_sts]);
            tvBuf[i].buf_sts = TV_BUF_EMPTY;
        }
    }
    tvbuf_DumpInfo(tvBuf);
    //TVBUF_UNLOCK();
    return 0;
}


inline UINT32 tvbuf_GetEmptyBuf(void* buf)
{
    int i;
    TV_Buffer * tvBuf = buf;
    if (buf == NULL) {
        TV_ERROR("NULL pointer");
        return -1;
    }

    for (i = 0; i < TVOUT_BUFFERS; i++)
    {
        if (tvBuf[i].buf_sts == TV_BUF_EMPTY)
        {
            tvBuf[i].buf_sts = TV_BUF_IN_QUEUE;
            tvbuf_SetTimeStamp(tvBuf, i);
            TV_LOG("TVR id:%d Sts: %x", tvBuf[i].buf_id, tvbuf_DumpStatus(tvBuf));
            return tvBuf[i].buf_id;
        }
    }

    return -1;
}

inline UINT32 tvbuf_GetFullBuf(void* buf)
{
    int i;
    TV_Buffer * tvBuf = buf;
    if (buf == NULL) {
        TV_ERROR("NULL pointer");
        return -1;
    }

    for (i = 0; i < TVOUT_BUFFERS; i++)
    {
        if (tvBuf[i].buf_sts == TV_BUF_FULL)
        {
            tvBuf[i].buf_sts = TV_BUF_IN_QUEUE;
            tvbuf_SetTimeStamp(tvBuf, i);
            TV_LOG("TVR id:%d Sts: %x", tvBuf[i].buf_id, tvbuf_DumpStatus(tvBuf));
            return tvBuf[i].buf_id;
        }
    }

    TV_ERROR("Can not Find full buffer");
    tvbuf_DumpInfo(tvBuf);

    return -1;
}


inline TVOUT_STATUS tvbuf_SetBufReady(void* buf, UINT32 buf_addr)
{
    int i;

    TV_Buffer * tvBuf = buf;
    if (buf == NULL) {
        TV_ERROR("NULL pointer");
        return TVOUT_STATUS_ERROR;
    }

    if (buf_addr == 0)
    {
        TV_ERROR("invalid buffer addr");
        return TVOUT_STATUS_ERROR;
    }
#if defined MTK_TVOUT_ENABLE_M4U
    for (i=0; i < TVOUT_BUFFERS; i++)
    {
        if (tvBuf[i].buf_mva_tvr == buf_addr)
        {
            tvBuf[i].buf_sts = TV_BUF_FULL;
            tvbuf_SetTimeStamp(tvBuf, i);
            TV_LOG("TVR id:%d Sts: %x", tvBuf[i].buf_id, tvbuf_DumpStatus(tvBuf));
            return TVOUT_STATUS_OK;
        }
    }
#else
    for (i=0; i < TVOUT_BUFFERS; i++)
    {
        if (tvBuf[i].buf_pa == buf_addr)
        {
            tvBuf[i].buf_sts = TV_BUF_FULL;
            tvbuf_SetTimeStamp(tvBuf, i);
            TV_LOG("TVR id:%d Sts: %x", tvBuf[i].buf_id, tvbuf_DumpStatus(tvBuf));
            return TVOUT_STATUS_OK;
        }
    }
#endif

    TV_WARN("can not find buf, addr 0x%x", buf_addr);
    tvbuf_DumpInfo(tvBuf);
    return TVOUT_STATUS_ERROR;

}

inline TVOUT_STATUS tvbuf_SetBufReady_ByIdx(void* buf, UINT32 ID)
{


    TV_Buffer * tvBuf = buf;
    if (buf == NULL) {
        TV_ERROR("NULL pointer");
        return TVOUT_STATUS_ERROR;
    }

    if (ID > (TVOUT_BUFFERS - 1) || ID < 0 )
    {
        TV_ERROR("invalid para %d\n", ID);
        return TVOUT_STATUS_ERROR;
    }

    tvBuf[ID].buf_sts = TV_BUF_FULL;
    tvbuf_SetTimeStamp(tvBuf, ID);

    return TVOUT_STATUS_OK;

}


inline UINT32 tvbuf_FindEmptyBuf(void* buf)
{
    int i;
    TV_Buffer * tvBuf = buf;
    if (buf == NULL) {
        TV_ERROR("NULL pointer");
        return -1;
    }

    for (i = 0; i < TVOUT_BUFFERS; i++)
    {
        if (tvBuf[i].buf_sts == TV_BUF_EMPTY)
        {
            tvBuf[i].buf_sts = TV_BUF_BUSY;
            tvbuf_SetTimeStamp(tvBuf, i);
            TV_LOG("TVC id:%d Sts: %x", tvBuf[i].buf_id, tvbuf_DumpStatus(tvBuf));
            return tvBuf[i].buf_id;
        }
    }

    TV_ERROR("Can not Find empty buffer");
    tvbuf_DumpInfo(tvBuf);

    return -1;
}


inline UINT32 tvbuf_FindFullBuf(void* buf)
{
    int i;
    TV_Buffer * tvBuf = buf;
    if (buf == NULL) {
        TV_ERROR("NULL pointer");
        return -1;
    }

    for (i=0; i<TVOUT_BUFFERS; i++)
    {
        if (tvBuf[i].buf_sts == TV_BUF_FULL)
        {
            tvBuf[i].buf_sts = TV_BUF_BUSY;
            tvbuf_SetTimeStamp(tvBuf, i);
            TV_LOG("TVC id:%d Sts: %x", tvBuf[i].buf_id, tvbuf_DumpStatus(tvBuf));
            return tvBuf[i].buf_id;
        }
    }

    return -1;
}

inline UINT32 tvbuf_FindLatestFullBuf(void* buf, UINT32 curID)
{
    int i;
    TV_Buffer * tvBuf = buf;
    if (buf == NULL) {
        TV_ERROR("NULL pointer");
        return -1;
    }
	
    for (i=0; i<TVOUT_BUFFERS; i++)
    {
        if (tvBuf[i].buf_sts == TV_BUF_FULL)
        {
			if (tvBuf[i].buf_timestamp < tvBuf[curID].buf_timestamp)
			{
				//TV_INFO("not latest");
				continue;
			}
            tvBuf[i].buf_sts = TV_BUF_BUSY;
            //tvbuf_SetTimeStamp(tvBuf, i);
            TV_LOG("TVC id:%d Sts: %x", tvBuf[i].buf_id, tvbuf_DumpStatus(tvBuf));
            return tvBuf[i].buf_id;
        }
    }

    return -1;
}

inline TVOUT_STATUS tvbuf_ReleaseBuf(void* buf, UINT32 buf_addr)
{
    int i;
    TV_Buffer * tvBuf = buf;
    if (buf == NULL) {
        TV_ERROR("NULL pointer");
        return TVOUT_STATUS_ERROR;
    }


    if (buf_addr == 0)
    {
        TV_ERROR("invalid buffer addr");
        return TVOUT_STATUS_ERROR;
    }
#if defined MTK_TVOUT_ENABLE_M4U
    for (i=0; i < TVOUT_BUFFERS; i++)
    {
        if (tvBuf[i].buf_mva_tvc== buf_addr)
        {
            tvBuf[i].buf_sts = TV_BUF_EMPTY;
            tvbuf_SetTimeStamp(tvBuf, i);
            TV_LOG("TVC id:%d Sts: %x", tvBuf[i].buf_id, tvbuf_DumpStatus(tvBuf));
            return TVOUT_STATUS_OK;
        }
    }
#else
    for (i=0; i < TVOUT_BUFFERS; i++)
    {
        if (tvBuf[i].buf_pa == buf_addr)
        {
            tvBuf[i].buf_sts = TV_BUF_EMPTY;
            tvbuf_SetTimeStamp(tvBuf, i);
            TV_LOG("TVC id:%d Sts: %x", tvBuf[i].buf_id, tvbuf_DumpStatus(tvBuf));
            return TVOUT_STATUS_OK;
        }
    }
#endif

    TV_WARN("can not find buf, addr 0x%x", buf_addr);
    tvbuf_DumpInfo(tvBuf);
    return TVOUT_STATUS_OK;
}

inline UINT32 tvbuf_GetBufID(void* buf, UINT32 buf_addr)
{
    int i;
    TV_Buffer * tvBuf = buf;
    if (buf == NULL) {
        TV_ERROR("NULL pointer");
        return TVOUT_STATUS_ERROR;
    }

    if (buf_addr == 0)
    {
        TV_ERROR("invalid buffer addr");
        return -1;
    }
#if defined MTK_TVOUT_ENABLE_M4U
    for (i=0; i < TVOUT_BUFFERS; i++)
    {
        if (tvBuf[i].buf_mva_tvc== buf_addr)
        {
            TV_LOG("%d:TVC MVA %x", tvBuf[i].buf_id, tvBuf[i].buf_mva_tvc);
            return tvBuf[i].buf_id;
        }

        if (tvBuf[i].buf_mva_tvr== buf_addr)
        {
            TV_LOG("%d:TVR MVA %x", tvBuf[i].buf_id, tvBuf[i].buf_mva_tvr);
            return tvBuf[i].buf_id;
        }
    }
#else
    for (i=0; i < TVOUT_BUFFERS; i++)
    {
        if (tvBuf[i].buf_pa == buf_addr)
        {
            TV_LOG("%d:%x", tvBuf[i].buf_id, tvBuf[i].buf_pa);
            return tvBuf[i].buf_id;
        }
    }
#endif

    TV_WARN("can not find buf, addr 0x%x", buf_addr);
    tvbuf_DumpInfo(tvBuf);
    return -1;
}



// ---------------------------------
//  TVOut Buffer Control-end
// ---------------------------------





// ---------------------------------------------------------------------------
//  TVOut Driver Private Functions
// ---------------------------------------------------------------------------

static TVOUT_STATUS Config_TVR(TVOUT_ROT rot, BOOL lockScreenUpdateMutex)
{

	TV_INFO("rot(%d), %s",  rot, (lockScreenUpdateMutex?"Block":"Nonblock"));
    // Prevent from race condition with screen update process
    if (lockScreenUpdateMutex) {
        SCREEN_UPDATE_LOCK();
    }

   	tvout_context.orientation = rot;

    //if (!tvout_context.isEnabled || TVOUT_MODE_MIRROR != tvout_context.tvMode)
    if (!tvout_context.isEnabled)
        goto End;

	{
        TVR_PARAM param;
        UINT32 i;

		TVR_Stop();

        param.srcWidth = DISP_GetScreenWidth();
        param.srcHeight = DISP_GetScreenHeight();

#if defined TV_BUFFER_FMT_UYVY
        param.outputFormat = TVR_YUYV422;
#else
        param.outputFormat = TVR_RGB565;
#endif

        param.rotation = (TVR_ROT)tvout_context.orientation;
        param.flip = FALSE;
        param.dstBufOffset = _cal_tvr_buf_offset(tvout_context.orientation, param.srcWidth, param.srcHeight);
        tvr_buf_offset = param.dstBufOffset;
        TV_INFO("%d", tvr_buf_offset);

        TVBUF_LOCK();
        tvbuf_Reset(&tvBuf);
#if defined MTK_TVOUT_ENABLE_M4U
        {
            for (i = 0; i < TVR_BUFFERS; ++ i)
            {
                UINT32 ID = tvbuf_GetEmptyBuf(&tvBuf);
                if (ID == -1)
                {
                    TV_ERROR("No Empty buffer for TVR");
                    return TVOUT_STATUS_ERROR;
                }
                param.dstBufAddr[i] = tvBuf[ID].buf_mva_tvr;
            }

            tvr_active_buf_addr = param.dstBufAddr[0] + tvr_buf_offset;
            TV_INFO("tvr_active_buf_addr %x", tvr_active_buf_addr);
        }
#else
        {
            for (i = 0; i < TVR_BUFFERS; ++ i)
            {
                UINT32 ID = tvbuf_GetEmptyBuf(&tvBuf);
                if (ID == -1)
                {
                    TV_ERROR("No Empty buffer for TVR");
                    return TVOUT_STATUS_ERROR;
                }
                param.dstBufAddr[i] = tvBuf[ID].buf_pa;
            }

            tvr_active_buf_addr = param.dstBufAddr[0] + tvr_buf_offset;
            TV_INFO("tvr_active_buf_addr %x", tvr_active_buf_addr);
        }
#endif
        TVBUF_UNLOCK();

        TVR_Config(&param);
		TVR_Start();
    }

    // Update tvout target size

    switch(rot)
    {
    case TVOUT_ROT_90  :
    case TVOUT_ROT_270 :
        tvout_context.targetWidth = DISP_GetScreenHeight();
        tvout_context.targetHeight = DISP_GetScreenWidth();
        break;

    case TVOUT_ROT_0   :
    case TVOUT_ROT_180 :
    default :
        tvout_context.targetWidth = DISP_GetScreenWidth();
        tvout_context.targetHeight = DISP_GetScreenHeight();
        break;
    }

    TV_INFO("TV context tar width %d height %d", tvout_context.targetWidth, tvout_context.targetHeight);

End:
    if (lockScreenUpdateMutex) {
        SCREEN_UPDATE_RELEASE();
    }

    return TVOUT_STATUS_OK;
}


static __inline TVOUT_STATUS tvout_enterMirrorMode(void)
{
	LCD_OUTPUT_MODE outputMode;
    UINT32 nextBuf;

	TV_INFO();

	SCREEN_UPDATE_LOCK();

    outputMode = LCD_GetOutputMode();
    LCD_CHECK_RET(LCD_SetOutputMode(outputMode & ~LCD_OUTPUT_TO_TVROT));
    TV_INFO("Set LCD output mode : (0x%x) -> (0x%x)", outputMode, LCD_GetOutputMode());

    // 1. Config TV Rotator
    Config_TVR(tvout_context.orientation, FALSE);
    // 2. Set LCD output mode
	outputMode = LCD_GetOutputMode();
    //LCD_CHECK_RET(LCD_WaitForNotBusy());
	LCD_CHECK_RET(LCD_SetOutputMode(outputMode | LCD_OUTPUT_TO_TVROT));
	TV_INFO("Set LCD output mode : (0x%x) -> (0x%x)", outputMode, LCD_GetOutputMode());

	SCREEN_UPDATE_RELEASE();

 	// 3. Config TV controller

#if defined TV_BUFFER_FMT_UYVY
	TVC_CHECK_RET(TVC_SetSrcFormat(TVC_UYVY422));
#else
    TVC_CHECK_RET(TVC_SetSrcFormat(TVC_RGB565));
#endif

    TVC_CHECK_RET(TVC_SetSrcSize(tvout_context.targetWidth, tvout_context.targetHeight));

    TVBUF_LOCK();
    nextBuf = tvbuf_FindEmptyBuf(&tvBuf);
    if (nextBuf == -1)
    {
        TV_WARN("no emtpy buf, try to find full buf");
        nextBuf = tvbuf_FindFullBuf(&tvBuf);
        if (nextBuf == -1)
        {
            TV_ERROR("no emtpy & full buf, can not init TVC");
            return TVOUT_STATUS_ERROR;
        }
    }
#if defined MTK_TVOUT_ENABLE_M4U
    TVC_CHECK_RET(TVC_SetSrcRGBAddr(tvBuf[nextBuf].buf_mva_tvc));
#else
    TVC_CHECK_RET(TVC_SetSrcRGBAddr(tvBuf[nextBuf].buf_pa));
#endif
    TVBUF_UNLOCK();

	return TVOUT_STATUS_OK;

}

static TVOUT_STATUS tvout_leaveMirrorMode(void)
{
	TV_INFO();

    if (tvout_context.isEnabled)
    {
        // when leave mirror mode, we need to release TVC working buffer
        TVBUF_LOCK();
        tvbuf_ReleaseBuf(&tvBuf,TVC_GetWorkingAddr());
        TVBUF_UNLOCK();
    }
	return TVOUT_STATUS_OK;
}


static __inline TVOUT_STATUS tvout_enterVideoPlaybackMode(void)
{
	if (tvout_context.videoPa == 0)
	{
		TV_ERROR("null pointer");
		return TVOUT_STATUS_ERROR;
	}

	if (TVOUT_FMT_YUV420_PLANAR == tvout_context.videoFormat ||
        TVOUT_FMT_YUV420_BLK == tvout_context.videoFormat)
	{
		UINT32 Y = tvout_context.videoPa;
    	UINT32 U = Y + tvout_context.videoWidth * tvout_context.videoHeight;
    	UINT32 V = U + tvout_context.videoWidth * tvout_context.videoHeight / 4;

    	TV_LOG("Y: %x, U: %x, V: %x", Y, U, V);

    	TVC_SetSrcYUVAddr(Y, U, V);
    	TVC_SetSrcFormat((TVC_SRC_FORMAT)tvout_context.videoFormat);
    	TVC_SetSrcSize(tvout_context.videoWidth, tvout_context.videoHeight);
    }
	else if (TVOUT_FMT_RGB565 == tvout_context.videoFormat ||
             TVOUT_FMT_UYVY == tvout_context.videoFormat)
	{
    	TV_LOG("RGB addr: %x",tvout_context.videoPa);
        TVC_SetSrcRGBAddr(tvout_context.videoPa);
    	TVC_SetSrcFormat((TVC_SRC_FORMAT)tvout_context.videoFormat);
    	TVC_SetSrcSize(tvout_context.videoWidth, tvout_context.videoHeight);
	}

    VIDEO_UPDATE_LOCK();
    TVC_CHECK_RET(TVC_CommitChanges(TRUE));
    VIDEO_UPDATE_RELEASE();

	return TVOUT_STATUS_OK;
}

static __inline void tvout_leaveVideoPlaybackMode(void)
{
	TV_INFO();

    if (is_tv_out_in_hqa_mode) {
        TVC_SetTarSizeForHQA(false);
    }
}


static void tvout_enable_power(BOOL enable)
{
	TV_INFO("TVout Power (%s)",(enable?"on":"off"));
    if (enable) {
#if defined(CONFIG_MT6575_EVB_BOARD)
    if( get_chip_eco_ver() == CHIP_E1) {
        TV_INFO("PMIC Analog swith to 0x0");
        hwSPKClassABAnalogSwitchSelect(0x0);
    } else {
        TV_INFO("PMIC Analog swith to 0x1");
        hwSPKClassABAnalogSwitchSelect(0x1);
    }
#else
        TV_INFO("PMIC Analog swith to 0x1");
        hwSPKClassABAnalogSwitchSelect(0x1);
#endif
        TVC_CHECK_RET(TVC_PowerOn());
        TVE_CHECK_RET(TVE_PowerOn());
        TVE_CHECK_RET(TVE_ResetDefaultSettings());
        TVR_CHECK_RET(TVR_PowerOn());
    } else {
        //LCD_CHECK_RET(LCD_WaitForNotBusy());
        TVR_CHECK_RET(TVR_PowerOff());
        TVE_CHECK_RET(TVE_PowerOff());
        TVC_CHECK_RET(TVC_PowerOff());

        TV_INFO("PMIC Analog swith to 0x0");
        hwSPKClassABAnalogSwitchSelect(0x0);
    }
}

TVOUT_STATUS tvout_clear_video_buffer_list(void)
{
#if 0
    struct list_head *pNode;
    struct list_head *pNext;
    tvout_video_buffer* pBufLink = NULL;
    list_for_each_safe(pNode, pNext, &tvout_video_buffer_list)
    {
        pBufLink = container_of(pNode, tvout_video_buffer, vd_link);
        if (pBufLink != NULL)
        {
            list_del(pNode);
            TV_INFO("remove 0x%x from list", pBufLink->vd_va);
            // dealloc MVA here
            TVC_DeallocMva(pBufLink->vd_va, pBufLink->vd_size, pBufLink->vd_mva);
            kfree((void *)pBufLink);
        }
    }
#else
/*
    if (video_buffer_addr) {
        TV_INFO("free video buffer");
        vfree(video_buffer_addr);
        video_buffer_addr = NULL;
        TVC_DeallocMva((UINT32)video_buffer_addr, video_buffer_size, video_buffer_mva);
    }
*/
    tv_video_buf_release();
#endif
    return TVOUT_STATUS_OK;
}

static inline unsigned int tvc_get_busy_index(void)
{
    unsigned int cur_addr = TVC_GetWorkingAddr();
    unsigned int bufSize;
    unsigned int idx;

    if (video_buffer_size == 0 ||
        video_buffer_mva == 0 ||
        cur_addr == 0) {
        return 0;
    }

    bufSize = video_buffer_size/VIDEO_BUFFER_CNT;
    idx = (cur_addr - video_buffer_mva)/bufSize;
    TV_INFO("idx %d",idx);
    return idx;
}

TVOUT_STATUS tvout_setup_video_buffer_list(unsigned int va, unsigned int bFlipUV, unsigned int bufSize, unsigned int * addr_hw_access)
{
#if 0
    tvout_video_buffer* pBufLink = NULL;
    struct list_head *pNode;
    bool    is_va_in_list = false;

    TV_LOG("Video buffer is not PMEM, use M4U");
    /* 1, iterate all list, to find a saved va*/

    list_for_each(pNode, &tvout_video_buffer_list)
    {
        pBufLink = container_of(pNode, tvout_video_buffer, vd_link);

        if (pBufLink->vd_va == va)
        {
            TV_LOG("va 0x%x is in List", va);
            is_va_in_list = true;
            *addr_hw_access = pBufLink->vd_mva;
            break;
        }
    }

    /* 2, va is a new one, allocate MVA for it*/
    if (is_va_in_list == false)
    {
        //_m4u_tvout_func.m4u_dump_pagetable(M4U_CLNTMOD_TVC);
        pBufLink = (tvout_video_buffer*)kmalloc(sizeof(tvout_video_buffer), GFP_KERNEL);
        if (pBufLink == NULL) {
            TV_ERROR("kmalloc error");
            return TVOUT_STATUS_ERROR;
        }
        if (TVC_AllocMva(va, bufSize, &pBufLink->vd_mva) != TVC_STATUS_OK) {
            TV_ERROR("can not alloc MVA for TVC");
            kfree((void *)pBufLink);
            return TVOUT_STATUS_ERROR;
        }

        //_m4u_tvout_func.m4u_dump_pagetable(M4U_CLNTMOD_TVC);
        *addr_hw_access = pBufLink->vd_mva;
        pBufLink->vd_va = va;
        pBufLink->vd_size = bufSize;
        list_add(&(pBufLink->vd_link), &(tvout_video_buffer_list));
        TV_INFO("add va 0x%x mva 0x%x into list", pBufLink->vd_va, pBufLink->vd_mva);
    }

    //tvout_context.isVideoM4uEnabled = true;
#else
    UINT32 nextBuf;

    /*if video size changed, allocate a new buffer*/
    if (video_buffer_addr != NULL && video_buffer_size < bufSize)
    {
        TV_INFO("buffer is not suitable 0x%x < 0x%x", video_buffer_size,  bufSize);
        if (tvout_context.isEnabled) {
            TV_ERROR();
            return TVOUT_STATUS_ERROR;
        } else {
            tv_video_buf_release();
            video_buffer_size = bufSize;
            tv_video_buf_init();
        }
    }

    video_buffer_size = bufSize;

    if (video_buffer_addr == NULL)
    {
        tv_video_buf_init();
        TV_INFO("allocate video buffer 0x%x, size 0x%x", video_buffer_addr, video_buffer_size);
    }


    //UINT32 nextBuf;

    TVBUF_LOCK();
    // 1.Get the empty buffer

    nextBuf = tvbuf_GetEmptyBuf(&tvVdoBuf);

    // 2.if no empty, get the old full buffer, show waring message
    if (nextBuf == -1)
    {
        nextBuf = tvbuf_GetFullBuf(&tvVdoBuf);
        if (nextBuf == -1)
        {
            TV_WARN("can not get empty & full buf");
            TVBUF_UNLOCK();
            return TVOUT_STATUS_OK;
        }
    }
    //TV_INFO("%x %x %x", tvVdoBuf[nextBuf].buf_va, va, video_buffer_size);

    // 3.write the buffer
	if (bFlipUV == 1){
    	memcpy((void*)tvVdoBuf[nextBuf].buf_va, (void*)va, video_buffer_size*2/3);
    	memcpy((void*)tvVdoBuf[nextBuf].buf_va + video_buffer_size*2/3, (void*)va + video_buffer_size*5/6, video_buffer_size/6);
    	memcpy((void*)tvVdoBuf[nextBuf].buf_va + video_buffer_size*5/6, (void*)va + video_buffer_size*2/3, video_buffer_size/6);
	}
	else{
    	memcpy((void*)tvVdoBuf[nextBuf].buf_va, (void*)va, video_buffer_size);
	}
	__cpuc_flush_kern_all();
    outer_clean_all();
    // 4.set the buffer to ready
    tvbuf_SetBufReady_ByIdx(&tvVdoBuf, nextBuf);
    TVBUF_UNLOCK();
#endif

    return TVOUT_STATUS_OK;


}

TVOUT_STATUS init_tvc_video_addr(void)
{
    UINT32 nextBuf;
    TVBUF_LOCK();
    nextBuf = tvbuf_FindEmptyBuf(&tvVdoBuf);
    if (nextBuf == -1)
    {
        TV_WARN("no emtpy buf, try to find full buf");
        nextBuf = tvbuf_FindFullBuf(&tvVdoBuf);
        if (nextBuf == -1)
        {
            TV_ERROR("no emtpy & full buf, can not init TVC");
            return TVOUT_STATUS_ERROR;
        }
    }
    tvout_context.videoPa = tvVdoBuf[nextBuf].buf_mva_tvc;

    TVBUF_UNLOCK();
    return TVOUT_STATUS_OK;
}


#ifndef TVOUT_PORTING_WAIT_OTHER_READY
extern void switch_NTSC_to_PAL(int mode);
#endif
TVOUT_STATUS tvout_enable_mode(BOOL enable)
{
	TV_INFO("TvOut mode (%s) (%s)",
        (enable?"Enable":"Disable"),
        (TVOUT_MODE_MIRROR == tvout_context.tvMode?"MIRROR":"VIDEO"));

    if (tvout_context.isEnabled == enable)
        return TVOUT_STATUS_ALREADY_SET;

    tvout_context.isEnabled = enable;

    if (enable)
    {
        if (TVOUT_STATUS_ERROR == tvbuf_Init())
        {
            TV_ERROR("BUFFER INIT ERROR");
            return TVOUT_STATUS_ERROR;
        }
        if (TVOUT_MODE_MIRROR == tvout_context.tvMode) {
            tvout_enterMirrorMode();
        } else {
            init_tvc_video_addr();
            tvout_enterVideoPlaybackMode();
        }

#ifndef TVOUT_PORTING_WAIT_OTHER_READY
		switch_NTSC_to_PAL(tvout_context.tvSystem);
#endif
		TVC_CHECK_RET(TVC_SetTvType(tvout_context.tvSystem));
        TVE_CHECK_RET(TVE_SetTvType(tvout_context.tvSystem));

        TVE_CHECK_RET(TVE_Enable());
        TVC_CHECK_RET(TVC_Enable());
        TVC_CHECK_RET(TVC_CommitChanges(TRUE));

        is_tv_initialized_done = true;

        DISP_UpdateScreen(0, 0, DISP_GetScreenWidth(), DISP_GetScreenHeight());
    }
    else
    {
		LCD_OUTPUT_MODE outputMode = LCD_GetOutputMode();
		if (outputMode & LCD_OUTPUT_TO_TVROT) {
			SCREEN_UPDATE_LOCK();
    		//LCD_CHECK_RET(LCD_WaitForNotBusy());
    		LCD_CHECK_RET(LCD_SetOutputMode(outputMode & ~LCD_OUTPUT_TO_TVROT));
			TV_INFO("Set LCD output mode : (0x%x) -> (0x%x)", outputMode, LCD_GetOutputMode());
			SCREEN_UPDATE_RELEASE();
		}

		TVR_Stop();

		VIDEO_UPDATE_LOCK();
        TVC_CHECK_RET(TVC_Disable());
		VIDEO_UPDATE_RELEASE();

        TVC_CHECK_RET(TVC_CommitChanges(TRUE));

		TVE_CHECK_RET(TVE_Disable());


        if (TVOUT_MODE_MIRROR == tvout_context.tvMode) {
            tvout_leaveMirrorMode();
        } else {
            tvout_leaveVideoPlaybackMode();
#if defined(MTK_TVOUT_ENABLE_M4U)
            // Cable has been plug out, clear video buffer if
            // LeaveVideoBuffer has not been called
            if (is_tv_cable_plugged_in == false) {
                tvout_clear_video_buffer_list();
        		tvout_context.tvMode = TVOUT_MODE_MIRROR;
            }
#endif
        }

        tvbuf_Deinit();

		is_tv_initialized_done = false;
    }

	return TVOUT_STATUS_OK;
}

static TVOUT_STATUS tvout_enable(BOOL enable)
{
    TVOUT_STATUS r = TVOUT_STATUS_OK;

	TV_INFO("[TV] TvOut: (%s)", (enable?"Enable":"Disable"));

    if (is_tv_enabled == enable) {
		TV_ERROR("TvOut already set ");
        return TVOUT_STATUS_ALREADY_SET;
	}


    if (enable) tvout_enable_power(TRUE);
    r = tvout_enable_mode(enable);
    if (!enable) tvout_enable_power(FALSE);

	is_tv_enabled = enable;

    return r;
}

// ---------------------------------------------------------------------------
//  TVOut Driver Public Functions
// ---------------------------------------------------------------------------
TVOUT_STATUS TVOUT_Capture_Tvrotbuffer(unsigned int pvbuf, unsigned int bpp)
{
    unsigned int i = 0;
    unsigned char *fbv = NULL;
    unsigned int fbsize = 0;
    unsigned int fb_bpp = 16;
    unsigned int w,h;
    int budId;

    TV_INFO();

    // TV-out is not enabled, return

    if ( !tvout_context.isEnabled )
    {
        TV_WARN("TVOUT_Capture_Tvrotbuffer, TVout is not enabled");
        return TVOUT_STATUS_OK;
    }

    if(pvbuf == 0 || bpp == 0)
    {
        TV_ERROR("TVOUT_Capture_Tvrotbuffer, ERROR, parameters wrong: pvbuf=0x%08x, bpp=%d", pvbuf, bpp);
        return TVOUT_STATUS_ERROR;
    }

    w = DISP_GetScreenWidth();
    h = DISP_GetScreenHeight();
    fbsize = w*h*fb_bpp/8;
    for (budId =0; budId< TVOUT_BUFFERS; budId++)
    {
        if (tvBuf[budId].buf_sts == TV_BUF_BUSY)
        {
            fbv = (unsigned char*)tvBuf[budId].buf_va;
        }
    }


    if (!fbv)
    {
        TV_ERROR("TVOUT_Capture_Tvrotbuffer, fbv is NULL");
        return TVOUT_STATUS_ERROR;
    }

    if(bpp == 32 && fb_bpp == 16)
    {
        unsigned int t;
	    unsigned short* fbvt = (unsigned short*)fbv;
        for(i = 0;i < w*h; i++)
    	{
	        t = fbvt[i];
            *(unsigned int*)(pvbuf+i*4) = 0xff000000|((t&0x001F)<<3)|((t&0x07E0)<<5)|((t&0xF800)<<8);
    	}
    }
    else if(bpp == 16 && fb_bpp == 16)
    {
    	memcpy((void*)pvbuf, (void*)fbv, fbsize);
    }
    else
    {
    	TV_ERROR("TVOUT_Capture_Tvrotbuffer, bpp:%d & fb_bpp:%d is not supported now", bpp, fb_bpp);
    }

    return TVOUT_STATUS_OK;
}

void TVOUT_EnableLog(bool enable)
{
    tv_out_log_on = enable;
    TV_INFO("debug log is %s", (enable?"ON":"OFF"));
}

void TVOUT_EnableTimeProfiling(bool enable)
{
    is_tv_enable_time_profiling = enable;
    TV_INFO("Time profiling is %s", (enable?"ON":"OFF"));
}


bool TVOUT_IsCablePlugIn(void)
{
    return is_tv_cable_plugged_in;
}

bool TVOUT_IsTvoutEnabled(void)
{
	return tvout_context.isEnabled;
}

bool TVOUT_IsUserEnabled(void)
{
    return is_tv_out_turned_on;
}
EXPORT_SYMBOL(TVOUT_IsUserEnabled);

void TVOUT_TurnOn(bool en)
{
    TV_INFO("%d", en);
    is_tv_out_turned_on = en;
}


TVOUT_STATUS TVOUT_PowerEnable(bool enable)
{
	TV_INFO("TvOut Power: (%s)", (enable?"Resume":"Suspend"));

    TVOUT_STATUS_LOCK();

	if (!enable && (is_tv_enabled || is_tv_display_colorbar)) {
		is_tv_enabled_before_suspend = TRUE;
        //if (is_tv_out_turned_on) {
        if (is_tv_out_turned_on && !is_tv_display_colorbar) {
            tvout_enable(FALSE);
        }else{
            TVOUT_EnableColorBar(false);
        }
	}

	is_engine_in_suspend_mode = enable ? false : true;

	if (enable && is_tv_enabled_before_suspend) {
		is_tv_enabled_before_suspend = FALSE;
		//if (is_tv_out_turned_on) {
        if (is_tv_out_turned_on && !is_tv_out_closed) {
            tvout_enable(TRUE);
        }else{
            TVOUT_EnableColorBar(true);
        }
	}

    TVOUT_STATUS_UNLOCK();
	return TVOUT_STATUS_OK;
}

TVOUT_STATUS TVOUT_EnableColorBar(bool enable)
{
	TV_INFO("ColorBar: (%s) pluged_in: (%d)", (enable?"Enable":"Disable"),
			is_tv_cable_plugged_in);

    if (is_tv_display_colorbar == enable) {
		TV_WARN("TvOut already set");
        return TVOUT_STATUS_ALREADY_SET;
	}
	is_tv_display_colorbar = enable;


    if (enable) {
        tvout_enable_power(TRUE);
        TVE_CHECK_RET(TVE_Enable());
        TVE_CHECK_RET(TVE_EnableColorBar(TRUE));
#ifndef TVOUT_PORTING_WAIT_OTHER_READY
    	switch_NTSC_to_PAL(tvout_context.tvSystem);
        TVE_CHECK_RET(TVE_SetTvType((TVE_TV_TYPE)tvout_context.tvSystem));
#endif
    } else {
        TVE_CHECK_RET(TVE_EnableColorBar(FALSE));
        TVE_CHECK_RET(TVE_Disable());
        tvout_enable_power(FALSE);
    }

    //is_tv_display_colorbar = enable;

	return TVOUT_STATUS_OK;
}

TVOUT_STATUS TVOUT_SetOrientation(TVOUT_ROT rot)
{

 	LCD_OUTPUT_MODE outputMode;
    if(0 == strncmp(MTK_LCM_PHYSICAL_ROTATION, "90", 2))
	{
    	rot = (rot + 3) % 4;
    	TV_INFO("MTK_LCM_PHYSICAL_ROTATION (%d)", rot);
	}
    else if(0 == strncmp(MTK_LCM_PHYSICAL_ROTATION, "180", 3))
	{
    	rot = (rot + 2) % 4;
    	TV_INFO("MTK_LCM_PHYSICAL_ROTATION (%d)", rot);
	}
    else if(0 == strncmp(MTK_LCM_PHYSICAL_ROTATION, "270", 3))
	{
    	rot = (rot + 1) % 4;
    	TV_INFO("MTK_LCM_PHYSICAL_ROTATION (%d)", rot);
	}

   	tvout_context.orientation = rot;

    TV_INFO("rot %d", rot);

    if (tvout_context.isEnabled && TVOUT_MODE_MIRROR == tvout_context.tvMode)
    {
        //SCREEN_UPDATE_LOCK();
        //Disable LCD output mode to TVROT
	    outputMode = LCD_GetOutputMode();
	    LCD_CHECK_RET(LCD_SetOutputMode(outputMode & ~LCD_OUTPUT_TO_TVROT));
        TV_INFO("Set LCD output mode : (0x%x) -> (0x%x)", outputMode, LCD_GetOutputMode());

        Config_TVR(rot, FALSE);

        //Enable LCD output mode to TVROT
	    outputMode = LCD_GetOutputMode();
	    LCD_CHECK_RET(LCD_SetOutputMode(outputMode | LCD_OUTPUT_TO_TVROT));
        TV_INFO("Set LCD output mode : (0x%x) -> (0x%x)", outputMode, LCD_GetOutputMode());
        //SCREEN_UPDATE_RELEASE();
    }

	return TVOUT_STATUS_OK;
}


TVOUT_STATUS TVOUT_SetTvSystem(TVOUT_SYSTEM tvSystem)
{
	TV_INFO("[TV]System(%d)", tvSystem);

    tvout_context.tvSystem = tvSystem;

    if (tvout_context.isEnabled) {
#ifndef TVOUT_PORTING_WAIT_OTHER_READY
        switch_NTSC_to_PAL(tvSystem);
#endif
        TVC_CHECK_RET(TVC_SetTvType((TVC_TV_TYPE)tvout_context.tvSystem));
        TVE_CHECK_RET(TVE_SetTvType((TVE_TV_TYPE)tvout_context.tvSystem));
        TVC_CHECK_RET(TVC_CommitChanges(TRUE));
    }

    return TVOUT_STATUS_OK;
}

TVOUT_STATUS TVOUT_DisableVideoMode(bool disable)
{
    TV_INFO("%s video mode (%d)", (disable?"DISABLED":"ENABLED"), is_disable_video_mode);
    if (disable) is_disable_video_mode++;
    else is_disable_video_mode--;
    if (is_disable_video_mode < 0)
    {
        TV_INFO("disable video mode");
        is_disable_video_mode = 0;
    }
    return TVOUT_STATUS_OK;
}



extern int is_pmem_range(unsigned long* base, unsigned long size);
TVOUT_STATUS TVOUT_PostVideoBuffer(UINT32 va, TVOUT_SRC_FORMAT format,
		UINT32 width, UINT32 height)
{

    unsigned int pa;
    unsigned int bufSize;
    unsigned int addr_hw_access;
    char temp;
    unsigned int t0 =0;
    unsigned int t1 =0;
    unsigned int t2 =0;
	unsigned int bFlipUV = 0;

    if (!tvout_context.isEnabled) {
        TV_LOG("tv-out is not enabled");
        return TVOUT_STATUS_OK;
    }

    if (is_tv_out_closed) {
        TV_LOG("tv-out is force closed");
        return TVOUT_STATUS_ERROR;
    }

    if (is_disable_video_mode) {
        TV_LOG("video mode is disabled, will not enter video mode");
        return TVOUT_STATUS_ERROR;
    }

    if ((char*)va == NULL) {
        TV_ERROR("Why SF post a NULL buffer!");
        return TVOUT_STATUS_ERROR;
    }


	if (format == TVOUT_FMT_YV12)
	{
		format = TVOUT_FMT_YUV420_PLANAR;
		bFlipUV = 1;
		if (width%32 != 0) {
			TV_ERROR("YV12 not align to 32");
			return TVOUT_STATUS_ERROR;
		}
	}

	if (TVC_CheckFormat(format) != TVC_STATUS_OK) {
        TV_LOG("Unsupported format, will not enter video mode");
        return TVOUT_STATUS_ERROR;
    }

	pa = m4u_user_v2p(va);

	if (pa == 0) {
		TV_WARN("Can not get available pa!");
		return TVOUT_STATUS_ERROR;
	}
    temp = *(char *)va;
    TVOUT_STATUS_LOCK();

    // change mirror mode to video mode
    if (tvout_context.tvMode == TVOUT_MODE_MIRROR)
    {
        if (tvout_context.isEnabled) {
            TVC_CHECK_RET(TVC_Disable());
            TVC_CHECK_RET(TVC_CommitChanges(TRUE));
        }
        tvout_context.tvMode = TVOUT_MODE_VIDEO;
        tvout_leaveMirrorMode();
        TV_INFO("Mirror mode->Video mode");

        // Cofig tar size for HQA mode if needed
        if (is_tv_out_in_hqa_mode)
            TVC_SetTarSizeForHQA(true);
        if (tvout_context.isEnabled) {
            TVC_CHECK_RET(TVC_Enable());
            TVC_CHECK_RET(TVC_CommitChanges(TRUE));
        }
    }

	switch (format)
	{
		case TVOUT_FMT_RGB565:
        case TVOUT_FMT_UYVY:
			bufSize = width * height * 2;
			break;
		case TVOUT_FMT_YUV420_PLANAR:
		case TVOUT_FMT_YUV420_BLK:
			bufSize = width * height * 3 / 2;
			break;
		default:
			TV_ERROR(" wrong format");
            TVOUT_STATUS_UNLOCK();
			return TVOUT_STATUS_ERROR;
	}

    TV_LOG("va 0x%x, pa 0x%x, W %d, H %d,size 0x%x,fmt %d 1st %x",va, pa, width, height, bufSize, format, temp);
    addr_hw_access = pa;
    //tvout_context.isVideoM4uEnabled = false;

    SCREEN_UPDATE_LOCK();

    if(!is_pmem_range((unsigned long*)pa, bufSize))
    {
#if defined MTK_TVOUT_ENABLE_M4U
        t0 = _GetTime();

        if (TVOUT_STATUS_OK != tvout_setup_video_buffer_list(va, bFlipUV, bufSize, &addr_hw_access)) {
            TV_ERROR("fail to set up video buffer");
            goto End;
        }
        t1 = _GetTime();

        tvout_context.isVideoM4uEnabled = true;
        //tvout_context.videoPa = addr_hw_access;
        tvout_context.tvMode = TVOUT_MODE_VIDEO;

        tvout_context.videoFormat = format;
        tvout_context.videoWidth = width;
        tvout_context.videoHeight = height;

        // Force align video width to 16-pixels if format is YUV420
        if (TVOUT_FMT_YUV420_PLANAR == format ||
            TVOUT_FMT_YUV420_BLK == format) {
            tvout_context.videoWidth = (tvout_context.videoWidth + 15) & 0xFFFFFFF0;
        }

        t2 = _GetTime();

        goto End;

#else
    TV_ERROR("M4U is not enabled, va is not in PMEM");
    goto End;
#endif
    }
    else {
        TV_LOG("The address is in PMEM, pa %d, va %d", pa, va);
    }

    tvout_context.tvMode = TVOUT_MODE_VIDEO;
    tvout_context.videoPa = addr_hw_access;
    tvout_context.videoFormat = format;
    tvout_context.videoWidth = width;
    tvout_context.videoHeight = height;

    // Force align video width to 16-pixels if format is YUV420
    if (TVOUT_FMT_YUV420_PLANAR == format ||
        TVOUT_FMT_YUV420_BLK == format) {
        tvout_context.videoWidth = (tvout_context.videoWidth + 15) & 0xFFFFFFF0;
    }

    if (!tvout_context.isEnabled || !is_tv_initialized_done) goto End;

    tvout_enterVideoPlaybackMode();

    t2 = _GetTime();
End:

    SCREEN_UPDATE_RELEASE();
    TVOUT_STATUS_UNLOCK();
    TV_LOG("1:%u 2:%u a:%u", t1-t0, t2-t1, t2-t0);

	return TVOUT_STATUS_OK;
}


TVOUT_STATUS TVOUT_LeaveVideoBuffer(void)
{
    TV_INFO();
    TVOUT_STATUS_LOCK();

	if (tvout_context.isEnabled && tvout_context.tvMode == TVOUT_MODE_VIDEO )
    {
        TVC_CHECK_RET(TVC_Disable());

        tvout_leaveVideoPlaybackMode();
    	tvout_enterMirrorMode();
	}

    //SCREEN_UPDATE_LOCK();
#if defined(MTK_TVOUT_ENABLE_M4U)
    tvout_clear_video_buffer_list();
#endif
    TV_INFO("Video mode->Mirror mode");
    tvout_context.tvMode = TVOUT_MODE_MIRROR;
    if (tvout_context.isEnabled) {
        TVC_CHECK_RET(TVC_Enable());
        TVC_CHECK_RET(TVC_CommitChanges(TRUE));
    }

    //SCREEN_UPDATE_RELEASE();
    TVOUT_STATUS_UNLOCK();
	return TVOUT_STATUS_OK;
}

#ifndef TVOUT_PORTING_WAIT_OTHER_READY
extern void accdet_detect(void);
#endif
TVOUT_STATUS TVOUT_TvTurnOn(bool on)
{

    TV_INFO("%d", on);

    if (is_tv_out_turned_on == on) {
        TV_WARN("already set");
        return TVOUT_STATUS_ALREADY_SET;
    }
    TVOUT_STATUS_LOCK();
    is_tv_out_turned_on = on;

    if (is_tv_out_turned_on && !is_tv_cable_plugged_in) {
        TV_INFO("Turn On, Let Accdet detect cable");
#ifndef TVOUT_PORTING_WAIT_OTHER_READY
        accdet_detect();
#endif
    } else if (is_tv_out_turned_on &&
			   is_tv_display_colorbar &&
			   !is_tv_out_closed) {
        TVOUT_EnableColorBar(false);
        if (tvout_enable(true) == TVOUT_STATUS_ERROR) {
            TV_WARN("TV out enable failed, enable CB instead");
            is_tv_out_enable_failed = true;
            tvout_enable(false);
            TVOUT_EnableColorBar(true);
        }
    } /*else if (is_tv_out_closed && !is_tv_out_turned_on) {
        TV_INFO("TVout is force closed, turn off colorbar");
        TVOUT_EnableColorBar(false);
    } */else if (is_tv_out_closed && is_tv_out_turned_on) {
        TV_INFO("TVout is force closed, turn on colorbar");
        TVOUT_EnableColorBar(true);
    } else if (!is_tv_out_turned_on && is_tv_cable_plugged_in) {
        TV_INFO("Turn Off, close TV-out, enable colorbar");
        if (!is_tv_out_enable_failed) {
		    tvout_enable(false);
		    TVOUT_EnableColorBar(true);
        } else {
            TV_WARN("TV out enable has been failed, still show CB");
            is_tv_out_enable_failed = false;
        }

    }  else{
        TV_ERROR("Unkown status");
        TV_ERROR("status: %d %d %d %d %d %d %d %d %d",
            is_tv_enabled,
            is_tv_enabled_before_suspend,
            is_engine_in_suspend_mode,
            is_tv_out_turned_on,
            is_tv_cable_plugged_in,
            is_tv_display_colorbar,
            is_tv_initialized_done,
            is_tv_out_closed,
            is_disable_video_mode);
    }

    TVOUT_STATUS_UNLOCK();

    return TVOUT_STATUS_OK;
}

extern void accdet_state_reset(void);

TVOUT_STATUS TVOUT_IPOPowerOff(void)
{

    TV_INFO();

    TVOUT_STATUS_LOCK();

	if ( is_tv_display_colorbar ) {
        TVOUT_EnableColorBar(false);
	}
	else if (tvout_context.isEnabled) {
		tvout_enable(false);
	}


     is_tv_enabled = 0;
     is_tv_enabled_before_suspend = 0;
     is_engine_in_suspend_mode = 0;
     is_tv_out_turned_on = 0;
     is_tv_cable_plugged_in = 0;
     is_tv_out_closed = 0;

#ifndef TVOUT_PORTING_WAIT_OTHER_READY
    accdet_state_reset();
#endif
    TVOUT_STATUS_UNLOCK();

    return TVOUT_STATUS_OK;
}
TVOUT_STATUS TVOUT_TvCablePlugIn_Directly(void)
{

    TV_INFO();

    is_tv_out_turned_on = true;
	TVOUT_TvCablePlugIn();


	return TVOUT_STATUS_OK;
}

TVOUT_STATUS TVOUT_TvCablePlugOut_Directly(void)
{
	TV_INFO();

	TVOUT_TvCablePlugOut();
	is_tv_out_turned_on = false;


	return TVOUT_STATUS_OK;

}



TVOUT_STATUS TVOUT_TvCablePlugIn(void)
{

    TV_INFO();

#ifndef TVOUT_PORTING_WAIT_OTHER_READY
    if (FACTORY_BOOT == get_boot_mode()) {
        TV_INFO("Factory mode: enable color bar");
        is_tv_cable_plugged_in = TRUE;
        return TVOUT_EnableColorBar(TRUE);
    }
#endif
	if (is_tv_cable_plugged_in) {
		TV_WARN("TV cable has pluged in.");
		return TVOUT_STATUS_ALREADY_SET;
	}

    if (!is_tv_out_turned_on) {
        TV_INFO("TV out is not turn on by user");
        return TVOUT_STATUS_OK;
    }

    TVOUT_STATUS_LOCK();

	is_tv_cable_plugged_in = TRUE;

	if (is_engine_in_suspend_mode) {
		is_tv_enabled_before_suspend = TRUE;
	} else {
	    if (is_tv_out_closed) {
            TV_INFO("tv out is closed, enable color bar");
            TVOUT_EnableColorBar(true);
        } else {
		    if (tvout_enable(true) == TVOUT_STATUS_ERROR) {
			    TV_WARN("TV-out enable failed. Enable CB instead");
                is_tv_out_enable_failed = true;
                tvout_enable(false);
                TVOUT_EnableColorBar(true);

                TVOUT_STATUS_UNLOCK();
			    return TVOUT_STATUS_ERROR;
	        }// tvout_enable
        }//is_tv_out_closed
    }//is_engine_in_suspend_mode
    TVOUT_STATUS_UNLOCK();

	return TVOUT_STATUS_OK;
}

TVOUT_STATUS TVOUT_TvCablePlugOut(void)
{
	TV_INFO();
#ifndef TVOUT_PORTING_WAIT_OTHER_READY
    if (FACTORY_BOOT == get_boot_mode()) {
        is_tv_cable_plugged_in = FALSE;
        return TVOUT_EnableColorBar(FALSE);
    }
#endif
	if (!is_tv_cable_plugged_in)
		return TVOUT_STATUS_ALREADY_SET;
    TVOUT_STATUS_LOCK();

	is_tv_cable_plugged_in = FALSE;

	if (is_tv_out_turned_on) {
		if (is_engine_in_suspend_mode) {
		    is_tv_enabled_before_suspend = FALSE;
		} else {
            if (is_tv_out_closed) {
                TV_INFO("tv out is closed, disable color bar");
                TVOUT_EnableColorBar(false);
            } else {
                if (is_tv_out_enable_failed) {
                    TV_WARN("TV out is not enabled successfully when plug in, so disable CB here");
                    is_tv_out_enable_failed = false;
                    TVOUT_EnableColorBar(false);
                } else {
                    tvout_enable(false);
                }
            }
        }
	} else {
        TVOUT_EnableColorBar(false);
	}
    TVOUT_STATUS_UNLOCK();

	return TVOUT_STATUS_OK;

}



TVOUT_STATUS TVOUT_ScreenUpdateLock(void)
{
	// Release semaphore in dbi_tvout_on_lcd_done()
    //SCREEN_UPDATE_LOCK();
	return TVOUT_STATUS_OK;
}

TVOUT_STATUS TVOUT_On_LCD_Done(void)
{
    // Software control LCD/TVC frame buffer addresses
    //SCREEN_UPDATE_RELEASE();
    return TVOUT_STATUS_OK;
}

static inline void _tvout_on_tvc_done(void)
{
    TV_TIME_PROFILE_START();


    if (tvout_context.isEnabled && TVOUT_MODE_MIRROR == tvout_context.tvMode)
    {
        UINT32 nextBuf;

        if (TVBUF_TRY_LOCK() == -1) {
           TV_WARN("lock fail");
           return;
        }
        // 1.check if there is a new full buffer
        nextBuf = tvbuf_FindFullBuf(&tvBuf);

        if (nextBuf == -1) {
            TVBUF_UNLOCK();
            return;
        }

        // 2. set prev busy buffer to empty
        tvbuf_ReleaseBuf(&tvBuf,TVC_GetWorkingAddr());
        TVBUF_UNLOCK();

        // 3. config new buffer to TVC
        TVC_CHECK_RET(TVC_SetSrcSize(tvout_context.targetWidth, tvout_context.targetHeight));
#if defined MTK_TVOUT_ENABLE_M4U
        if (TVC_GetWorkingAddr() == tvBuf[nextBuf].buf_mva_tvc) {
            TV_WARN("Tearing may occurs, get the same buffer %x", tvbuf_DumpStatus(&tvBuf));
        }
        TVC_CHECK_RET(TVC_SetSrcRGBAddr(tvBuf[nextBuf].buf_mva_tvc));
#else
        if (TVC_GetWorkingAddr() == tvBuf[nextBuf].buf_pa) {
            TV_WARN("Tearing may occurs, get the same buffer %x", tvbuf_DumpStatus(&tvBuf));
        }
        TVC_CHECK_RET(TVC_SetSrcRGBAddr(tvBuf[nextBuf].buf_pa));
#endif
        TVC_CHECK_RET(TVC_CommitChanges(TRUE));
   }
   else if (tvout_context.isEnabled && TVOUT_MODE_VIDEO == tvout_context.tvMode)
   {
       UINT32 nextBuf;
       static UINT32 curBuf = 0;
       UINT32 workingAddr;
/*
       if (TVBUF_TRY_LOCK() == -1) {
          TV_WARN("lock fail");
          return;
       }
*/
       TVBUF_LOCK();
       // 1.check if there is a new full buffer
       //nextBuf = tvbuf_FindFullBuf(&tvVdoBuf);
       nextBuf = tvbuf_FindLatestFullBuf(&tvVdoBuf, curBuf);
       if (nextBuf == -1) {
           TVBUF_UNLOCK();
           return;
       }

       // 2. set prev busy buffer to empty
       workingAddr = TVC_GetWorkingAddr();

       TVBUF_UNLOCK();

       if (TVC_GetWorkingAddr() == tvVdoBuf[nextBuf].buf_mva_tvc) {
           TV_WARN("Tearing may occurs, get the same buffer %x", tvbuf_DumpStatus(&tvVdoBuf));
       }
       tvout_context.videoPa = tvVdoBuf[nextBuf].buf_mva_tvc;

       tvout_enterVideoPlaybackMode();

       TVBUF_LOCK();
       tvbuf_ReleaseBuf(&tvVdoBuf,workingAddr);
       TVBUF_UNLOCK();
       curBuf = nextBuf;
   }
   TV_TIME_PROFILE_END();
}

void tvout_work_callback(struct work_struct *work)
{
    _tvout_on_tvc_done();
}


inline void TVOUT_On_TVC_Done(void)
{
    TV_TIME_PROFILE_START();
#if defined TVOUT_BUFFER_WORK_QUEUE
    queue_work(tvout_workqueue, &tvout_work);
#else
    _tvout_on_tvc_done();
#endif

    TV_TIME_PROFILE_END();
}
EXPORT_SYMBOL(TVOUT_On_TVC_Done);

inline void  TVOUT_On_TVR_Done(void)
{
    TV_TIME_PROFILE_START();

    if (tvout_context.isEnabled && tvout_context.tvMode == TVOUT_MODE_MIRROR)
    {
                UINT32 nextBuf;
        static  UINT32 pre_active_addr = 0;
        static  UINT32 pre_inqueue_addr = 0;


        if (TVBUF_TRY_LOCK() == -1)
        {
            TV_WARN("lock fail");
            //Can not lock semphone, so only update active buf index
            tvr_active_buf_addr = TVR_GetWorkingAddr();
            return;
        }

        // 1.Get the empty buffer

        nextBuf = tvbuf_GetEmptyBuf(&tvBuf);

        // 2.if no empty, get the old full buffer, show waring message
        if (nextBuf == -1)
        {
            nextBuf = tvbuf_GetFullBuf(&tvBuf);
            if (nextBuf == -1)
            {
                TV_WARN("can not get empty & full buf");
                TVBUF_UNLOCK();
                return;
            }
        }

        // 3.Enqueue the buffer
#if defined MTK_TVOUT_ENABLE_M4U
        TVR_EnqueueBuffer(tvBuf[nextBuf].buf_mva_tvr, &pre_inqueue_addr);
#else
        TVR_EnqueueBuffer(tvBuf[nextBuf].buf_pa, &pre_inqueue_addr);
#endif

        // 4.Dequeue the buffer, set it to ready

        tvbuf_SetBufReady(&tvBuf, pre_inqueue_addr - tvr_buf_offset);


        if (pre_inqueue_addr == pre_active_addr)
        {
            TV_WARN("pre_inqueue_addr = pre_active_addr 0x%x", tvr_active_buf_addr);
        }
        pre_active_addr = pre_inqueue_addr;


        TVBUF_UNLOCK();

    }
    TV_TIME_PROFILE_END();
}
EXPORT_SYMBOL(TVOUT_On_TVR_Done);


TVOUT_STATUS TVOUT_ForceClose(void)
{
    TV_INFO("cable: %s ",
         (is_tv_cable_plugged_in?"plug-in":"plug-out"));

    if (is_tv_out_closed)
    {
        TV_WARN("tv out has been closed, but close again");
        return TVOUT_STATUS_ERROR;
    }

    if (is_engine_in_suspend_mode)
    {
        TV_WARN("force close after suspend");
        return TVOUT_STATUS_ERROR;
    }

    TVOUT_STATUS_LOCK();
    if (is_tv_cable_plugged_in && is_tv_out_turned_on)
    {
        TV_INFO("cable plug-in, close tv out, enable color bar ");
        if (!is_tv_out_enable_failed) {
            tvout_enable(false);
            TVOUT_EnableColorBar(true);
        } else {
            TV_WARN("TV out has been enabled failed, still show CB here");
            is_tv_out_enable_failed = false;
        }
    }

    is_tv_out_closed = true;
    TVOUT_STATUS_UNLOCK();

    return TVOUT_STATUS_OK;
}

TVOUT_STATUS TVOUT_RestoreOpen(void)
{
    TV_INFO("cable: %s ",
         (is_tv_cable_plugged_in?"plug-in":"plug-out"));

    if (!is_tv_out_closed)
    {
        TV_WARN("tv out has been opened, but open again");
        return TVOUT_STATUS_ERROR;
    }

    if (is_engine_in_suspend_mode)
    {
        TV_WARN("restore open after suspend");
        return TVOUT_STATUS_ERROR;
    }

    TVOUT_STATUS_LOCK();

    if (is_tv_cable_plugged_in && is_tv_out_turned_on)
    {
        TV_INFO("cable plug-in, open tv out, enable tv out ");
        TVOUT_EnableColorBar(false);
        if (tvout_enable(true) == TVOUT_STATUS_ERROR) {
            TV_WARN("TV out enable failed, enable CB instead");
            is_tv_out_enable_failed = true;
            tvout_enable(false);
            TVOUT_EnableColorBar(true);
        }
    }

    is_tv_out_closed = false;

    TVOUT_STATUS_UNLOCK();

    return TVOUT_STATUS_OK;
}


/*****************************************************************************/

//file operations
#if 0
static int TVout_Drv_Ioctl(struct inode * a_pstInode,
						 struct file * a_pstFile,
						 unsigned int a_u4Command,
						 unsigned long a_u4Param)
#else
static long TVout_Drv_Ioctl(struct file * a_pstFile,
								unsigned int a_Command,
								unsigned long a_Param)
#endif
{
    int ret = 0;
    void __user *argp = (void __user *)a_Param;
    unsigned int ctrlCmd = a_Command;

	switch (ctrlCmd)
	{
        case TVOUT_IS_TV_CABLE_PLUG_IN:
        {
            BOOL isTvCableEnabled = TVOUT_IsCablePlugIn();
            return copy_to_user(argp, &isTvCableEnabled,
                            sizeof(isTvCableEnabled)) ? -EFAULT : 0;
        }

		case TVOUT_TURN_ON:
    	{
        	TV_INFO("Ioctl: set TV Enable: %u", a_Param);
            TVOUT_TvTurnOn(a_Param);
        	return 0;
    	}

    	case TVOUT_SET_TV_SYSTEM:
    	{
        	TV_INFO("Ioctl: set TV System: %u", a_Param);
        	TVOUT_SetTvSystem((TVOUT_SYSTEM)a_Param);
        	return 0;
    	}

    	case TVOUT_ISSHOW_TVOUTBUTTON:
    	{
            bool isShow = true;
#if defined TVOUT_USER_ENABLE_BUTTON
            isShow = true;
#else
            isShow = false;
#endif

        	TV_INFO("Ioctl: show tvout button(%d)", isShow);
        	return isShow;
    	}

		case TVOUT_CTL_SWITCH_TO_HQA_MODE:
		{
		    TV_INFO("Ioctl: swith to HQA mode(%d)", (UINT32)a_Param);

			if (a_Param & 0x1)
				tvout_context.tvMode = TVOUT_MODE_VIDEO;
			else
				tvout_context.tvMode = TVOUT_MODE_VIDEO;
       	 	return 0;
		}

        case TVOUT_POST_VIDEO_BUFFER:
        {
            TVOUT_HQA_BUF b;

       		if (copy_from_user(&b, (void __user *)a_Param, sizeof(b)))
        	{
            	TV_ERROR("copy_from_user failed! ");
            	ret = -EFAULT;
        	} else {
            	TVOUT_PostVideoBuffer((UINT32)b.vir_addr,
                                      (TVOUT_SRC_FORMAT)b.format,
                                      b.width, b.height);
			}
        	return ret;

        }

        case TVOUT_LEAVE_VIDEO_BUFFER:
        {
			TVOUT_LeaveVideoBuffer();
            return 0;
        }

	    case TVOUT_CTL_POST_HQA_BUFFER:
    	{
        	TVOUT_HQA_BUF b;
        	TV_INFO("Ioctl: post HQA buffer");

       		if (copy_from_user(&b, (void __user *)a_Param, sizeof(b)))
        	{
            	TV_ERROR("copy_from_user failed! ");
            	ret = -EFAULT;
        	} else {

				//TVC_SetTarSizeForHQA(TRUE);
				is_tv_out_in_hqa_mode = true;
				tv_system_backup = tvout_context.tvSystem;
                TVOUT_SetTvSystem(TVOUT_SYSTEM_PAL);
            	TVOUT_PostVideoBuffer((UINT32)b.vir_addr,
                                      (TVOUT_SRC_FORMAT)b.format,
                                      b.width, b.height);
			}
        	return ret;
    	}

    	case TVOUT_CTL_LEAVE_HQA_MODE:
    	{
        	TV_INFO("Ioctl: leave HQA mode");
            TVOUT_SetTvSystem(tv_system_backup);
			//tvout_dumpreg();
			//TVC_SetTarSizeForHQA(FALSE);

			TVOUT_LeaveVideoBuffer();
            is_tv_out_in_hqa_mode = false;
            DISP_UpdateScreen(0, 0, DISP_GetScreenWidth(), DISP_GetScreenHeight());
			return 0;
    	}

    	case TVOUT_FORCE_CLOSE:
    	{
        	TV_INFO("Ioctl: force close tv-out");
            TVOUT_ForceClose();
    		return 0;
    	}

        case TVOUT_RESTORE_OPEN:
    	{
        	TV_INFO("Ioctl: restore open tv-out");
            TVOUT_RestoreOpen();
			return 0;
    	}

        case TVOUT_DISABLE_VIDEO_MODE:
        {
            TV_INFO("Ioctl: video mode is %s", (a_Param?"DISABLED":"ENABLED"));
            TVOUT_DisableVideoMode(a_Param);
            return 0;
        }

        case TVOUT_ENABLE_COLOR_BAR:
        {
            TV_INFO("Ioctl: colorbar %s", (a_Param?"ENABLED":"DISABLED"));
            TVOUT_EnableColorBar(a_Param);
            return 0;
        }

		case TVOUT_PLUG_IN_DIRECTLY:
		{
			TVOUT_TvCablePlugIn_Directly();
			return 0;
		}

		case TVOUT_PLUG_OUT_DIRECTLY:
		{
			TVOUT_TvCablePlugOut_Directly();
			return 0;
		}

        case TVOUT_POWER_ENABLE:
        {
            TV_INFO("Ioctl: power %s", (a_Param?"ENABLED":"DISABLED"));
            TVOUT_PowerEnable(a_Param);
            return 0;
        }

		case TVOUT_IPO_POWER_OFF:
		{
            TV_INFO("Ioctl: IPO Power off");
			TVOUT_IPOPowerOff();
			return 0;
		}

        default:
        {
            return -1;
        }

	}

	return ret;
}


static const struct file_operations g_stMTKTVout_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = TVout_Drv_Ioctl
};

static struct cdev * g_pMTKTVout_CharDrv = NULL;
static dev_t g_MTKTVoutdevno = MKDEV(MTK_TVOUT_MAJOR_NUMBER,0);
static inline int TVout_Drv_Register(void)
{
    TV_INFO();
    if( alloc_chrdev_region(&g_MTKTVoutdevno, 0, 1,"TV-out") ) {
        TV_ERROR("Allocate device no failed");
        return -EAGAIN;
    }

    //Allocate driver
    g_pMTKTVout_CharDrv = cdev_alloc();

    if (NULL == g_pMTKTVout_CharDrv) {
        unregister_chrdev_region(g_MTKTVoutdevno, 1);
        TV_ERROR("Allocate mem for kobject failed");
        return -ENOMEM;
    }

    //Attatch file operation.
    cdev_init(g_pMTKTVout_CharDrv, &g_stMTKTVout_fops);
    g_pMTKTVout_CharDrv->owner = THIS_MODULE;

    //Add to system
    if (cdev_add(g_pMTKTVout_CharDrv, g_MTKTVoutdevno, 1)) {
        TV_ERROR("Attatch file operation failed");
        unregister_chrdev_region(g_MTKTVoutdevno, 1);
        return -EAGAIN;
    }

    return 0;
}

static inline void TVout_Drv_Unregister(void)
{
    //Release char driver
    cdev_del(g_pMTKTVout_CharDrv);
    unregister_chrdev_region(g_MTKTVoutdevno, 1);
}

static struct class *pTVoutClass = NULL;
// Called to probe if the device really exists. and create the semaphores
static int TVout_Drv_Probe(struct platform_device *pdev)
{
    struct device* tvout_device = NULL;
    TV_INFO();

    //Check platform_device parameters
    if (NULL == pdev) {
        TV_ERROR("platform data missed");
        return -ENXIO;
    }

    //register char driver
    //Allocate major no
    if(TVout_Drv_Register()) {
        dev_err(&pdev->dev,"register char failed\n");
        return -EAGAIN;
    }

    pTVoutClass = class_create(THIS_MODULE, "tvoutdrv");
    if (IS_ERR(pTVoutClass)) {
        int ret = PTR_ERR(pTVoutClass);
        TV_ERROR("Unable to create class, err = %d", ret);
        return ret;
    }
    tvout_device = device_create(pTVoutClass, NULL, g_MTKTVoutdevno, NULL, "TV-out");


	//------------------------------------------------------------------
	// 							Create workqueue
	//------------------------------------------------------------------
	tvout_workqueue = create_singlethread_workqueue("tvout");
	INIT_WORK(&tvout_work, tvout_work_callback);


#if defined MTK_M4U_SUPPORT
#endif

	//Init TV-out engines
	TVC_CHECK_RET(TVC_Init());
    TVE_CHECK_RET(TVE_Init());
    TVR_CHECK_RET(TVR_Init());

#if defined TVOUT_USER_ENABLE_BUTTON
    is_tv_out_turned_on = false;
#ifndef TVOUT_PORTING_WAIT_OTHER_READY
	if (FACTORY_BOOT == get_boot_mode()) {
        TV_INFO("In Factory mode, always set user config on");
        is_tv_out_turned_on = true;
    }
#endif
#else
    is_tv_out_turned_on = true; // auto detect, always be ture.
#endif
    return 0;
}


// Called when the device is being detached from the driver
static int TVout_Drv_Remove(struct platform_device *pdev)
{
    //unregister char driver.
    TVout_Drv_Unregister();
	destroy_workqueue(tvout_workqueue);
    device_destroy(pTVoutClass, g_MTKTVoutdevno);
    class_destroy(pTVoutClass);

    TV_INFO("TV-out driver is removed");
    return 0;
}


static int TVout_Drv_Suspend(struct platform_device *pdev, pm_message_t mesg)
{
    return 0;
}

static int TVout_Drv_Resume(struct platform_device *pdev)
{
    return 0;
}



static struct platform_driver g_stMTKTVout_Platform_Driver = {
    .probe	= TVout_Drv_Probe,
    .remove	= TVout_Drv_Remove,
    .suspend= TVout_Drv_Suspend,
    .resume	= TVout_Drv_Resume,
    .driver	= {
    .name	= "TV-out",
    .owner	= THIS_MODULE,
    }
};


static int __init TVout_Drv_Init(void)
{
    if (platform_driver_register(&g_stMTKTVout_Platform_Driver))
    {
        TV_ERROR("failed to register TV out driver");
        return -ENODEV;
    }
    TV_INFO();

    memset(&tvout_context, 0, sizeof(tvout_context));

    if(0 == strncmp(MTK_LCM_PHYSICAL_ROTATION, "90", 2))
	{
    	tvout_context.orientation = 3;
    }
    else if(0 == strncmp(MTK_LCM_PHYSICAL_ROTATION, "180", 3))
	{
    	tvout_context.orientation = 2;
    }
    else if(0 == strncmp(MTK_LCM_PHYSICAL_ROTATION, "270", 3))
	{
    	tvout_context.orientation = 1;
    }

    INIT_LIST_HEAD(&(tvout_video_buffer_list));

    return 0;
}

static void __exit TVout_Drv_Exit(void)
{
    TV_INFO();
    platform_driver_unregister(&g_stMTKTVout_Platform_Driver);
}

module_init(TVout_Drv_Init);
module_exit(TVout_Drv_Exit);

MODULE_DESCRIPTION("MTK TV-out driver");
MODULE_AUTHOR("Mingchen <Mingchen.gao@Mediatek.com>");
MODULE_LICENSE("GPL");

#endif

