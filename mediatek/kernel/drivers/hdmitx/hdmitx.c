/*****************************************************************************/
/* Copyright (c) 2009 NXP Semiconductors BV                                  */
/*                                                                           */
/* This program is free software; you can redistribute it and/or modify      */
/* it under the terms of the GNU General Public License as published by      */
/* the Free Software Foundation, using version 2 of the License.             */
/*                                                                           */
/* This program is distributed in the hope that it will be useful,           */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the              */
/* GNU General Public License for more details.                              */
/*                                                                           */
/* You should have received a copy of the GNU General Public License         */
/* along with this program; if not, write to the Free Software               */
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307       */
/* USA.                                                                      */
/*                                                                           */
/*****************************************************************************/
#if defined(MTK_HDMI_SUPPORT)
#define TMFL_TDA19989

#define _tx_c_
#include <linux/autoconf.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/earlysuspend.h>
#include <linux/kthread.h>
#include <linux/rtpm_prio.h>
#include <linux/vmalloc.h>
#include <linux/disp_assert_layer.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/switch.h>

#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/mach-types.h>
#include <asm/cacheflush.h>
#include <asm/io.h>
#include <mach/dma.h>
#include <mach/irqs.h>
#include <asm/tlbflush.h>
#include <asm/page.h>

#include <mach/m4u.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_reg_base.h>
#include <mach/mt_clkmgr.h>

#include "hdmitx.h"
#include "hdmi_drv.h"
#include "hdmi_utils.h"

#include "dpi_reg.h"
#include "mach/eint.h"
#include "mach/irqs.h"

#include "disp_drv_platform.h"
#include "ddp_reg.h"

#include "dpi1_drv.h"
#include <ddp_dpfd.h>



#ifdef I2C_DBG
#include "tmbslHdmiTx_types.h"
#include "tmbslTDA9989_local.h"
#endif


#define HDMI_DEVNAME "hdmitx"

#undef OUTREG32
#define OUTREG32(x, y) {/*printk("[hdmi]write 0x%08x to 0x%08x\n", (y), (x)); */__OUTREG32((x),(y))}
#define __OUTREG32(x,y) {*(unsigned int*)(x)=(y);}

#define RETIF(cond, rslt)       if ((cond)){HDMI_LOG("return in %d\n",__LINE__);return (rslt);}
#define RET_VOID_IF(cond)       if ((cond)){HDMI_LOG("return in %d\n",__LINE__);return;}
#define RETIF_NOLOG(cond, rslt)       if ((cond)){return (rslt);}
#define RET_VOID_IF_NOLOG(cond)       if ((cond)){return;}
#define RETIFNOT(cond, rslt)    if (!(cond)){HDMI_LOG("return in %d\n",__LINE__);return (rslt);}

#ifdef MTK_MT8193_HDMI_SUPPORT
#define HDMI_DPI(suffix)        DPI1 ## suffix
#define HMID_DEST_DPI    		DISP_MODULE_DPI1
static int hdmi_bpp = 3;
#else
#define HDMI_DPI(suffix)        DPI  ## suffix
#define HMID_DEST_DPI    		DISP_MODULE_DPI0
static int hdmi_bpp = 3;
#endif


#define hdmi_performance_tuning

static int wdma1_bpp = 2;
static int rmda1_bpp = 3;
static int ddp1_bpp =3;


#define ALIGN_TO(x, n)  \
    (((x) + ((n) - 1)) & ~((n) - 1))

extern const HDMI_DRIVER* HDMI_GetDriver(void);
extern void HDMI_DBG_Init(void);

extern UINT32 DISP_GetScreenHeight(void);
extern UINT32 DISP_GetScreenWidth(void);
extern BOOL DISP_IsVideoMode(void);
extern int disp_lock_mutex(void);
extern int disp_unlock_mutex(int id);
static void hdmi_update_impl(void);

static size_t hdmi_log_on = 1;
static struct switch_dev hdmi_switch_data;
HDMI_PARAMS _s_hdmi_params = {0};
HDMI_PARAMS *hdmi_params = &_s_hdmi_params;
static HDMI_DRIVER *hdmi_drv = NULL;

void hdmi_log_enable(int enable)
{
	printk("hdmi log %s\n", enable?"enabled":"disabled");
	hdmi_log_on = enable;
	hdmi_drv->log_enable(enable);
}

static DEFINE_SEMAPHORE(hdmi_update_mutex);
typedef struct{
    bool is_reconfig_needed;    // whether need to reset HDMI memory
    bool is_enabled;    // whether HDMI is enabled or disabled by user
    bool is_force_disable; 		//used for camera scenario.
    bool is_clock_on;   // DPI is running or not
    atomic_t state; // HDMI_POWER_STATE state
    int 	lcm_width;  // LCD write buffer width
    int		lcm_height; // LCD write buffer height
    bool    lcm_is_video_mode;
    int		hdmi_width; // DPI read buffer width
    int		hdmi_height; // DPI read buffer height
    HDMI_VIDEO_RESOLUTION		output_video_resolution;
    HDMI_AUDIO_FORMAT           output_audio_format;
    int		orientation;    // MDP's orientation, 0 means 0 degree, 1 means 90 degree, 2 means 180 degree, 3 means 270 degree
    HDMI_OUTPUT_MODE    output_mode;
    int     scaling_factor;
}_t_hdmi_context;

struct hdmi_video_buffer_list {
    struct hdmi_video_buffer_info buffer_info;
    pid_t  pid;
    void*  file_addr;
    unsigned int buffer_mva;
    struct list_head list;
};

static struct list_head hdmi_video_mode_buffer_list;
static struct list_head *hdmi_video_buffer_list_head = &hdmi_video_mode_buffer_list;
DEFINE_SEMAPHORE(hdmi_video_mode_mutex);
static atomic_t hdmi_video_mode_flag = ATOMIC_INIT(0);
static int hdmi_add_video_buffer(struct hdmi_video_buffer_info *buffer_info, struct file *file);
static struct hdmi_video_buffer_list* hdmi_search_video_buffer(struct hdmi_video_buffer_info *buffer_info, struct file *file);
static void hdmi_destory_video_buffer(void);
#define IS_HDMI_IN_VIDEO_MODE()        atomic_read(&hdmi_video_mode_flag)
#define SET_HDMI_TO_VIDEO_MODE()       atomic_set(&hdmi_video_mode_flag, 1)
#define SET_HDMI_LEAVE_VIDEO_MODE()    atomic_set(&hdmi_video_mode_flag, 0)
static wait_queue_head_t hdmi_video_mode_wq;
//static atomic_t hdmi_video_mode_event = ATOMIC_INIT(0);
//static atomic_t hdmi_video_mode_dpi_change_address = ATOMIC_INIT(0);
#define IS_HDMI_VIDEO_MODE_DPI_IN_CHANGING_ADDRESS()    atomic_read(&hdmi_video_mode_dpi_change_address)
#define SET_HDMI_VIDEO_MODE_DPI_CHANGE_ADDRESS()        atomic_set(&hdmi_video_mode_dpi_change_address, 1)
#define SET_HDMI_VIDEO_MODE_DPI_CHANGE_ADDRESS_DONE()   atomic_set(&hdmi_video_mode_dpi_change_address, 0)


static _t_hdmi_context hdmi_context;
static _t_hdmi_context *p = &hdmi_context;

#define IS_HDMI_ON()			(HDMI_POWER_STATE_ON == atomic_read(&p->state))
#define IS_HDMI_OFF()			(HDMI_POWER_STATE_OFF == atomic_read(&p->state))
#define IS_HDMI_STANDBY()	    (HDMI_POWER_STATE_STANDBY == atomic_read(&p->state))

#define IS_HDMI_NOT_ON()		(HDMI_POWER_STATE_ON != atomic_read(&p->state))
#define IS_HDMI_NOT_OFF()		(HDMI_POWER_STATE_OFF != atomic_read(&p->state))
#define IS_HDMI_NOT_STANDBY()	(HDMI_POWER_STATE_STANDBY != atomic_read(&p->state))

#define SET_HDMI_ON()	        atomic_set(&p->state, HDMI_POWER_STATE_ON)
#define SET_HDMI_OFF()	        atomic_set(&p->state, HDMI_POWER_STATE_OFF)
#define SET_HDMI_STANDBY()	    atomic_set(&p->state, HDMI_POWER_STATE_STANDBY)

int hdmi_allocate_hdmi_buffer(void);
int hdmi_free_hdmi_buffer(void);
int hdmi_display_path_overlay_config(bool enable);
int hdmi_dst_display_path_config(bool enable);
int hdmi_src_display_path_config(bool enable);
static int dp_mutex_src = -1, dp_mutex_dst = -1;
static unsigned int temp_mva_r, temp_mva_w, temp_va, hdmi_va, hdmi_mva_r, hdmi_mva_w;


static dev_t hdmi_devno;
static struct cdev *hdmi_cdev;
static struct class *hdmi_class = NULL;

#include <linux/mmprofile.h>
struct HDMI_MMP_Events_t
{
    MMP_Event HDMI;
    MMP_Event DDPKBitblt;
    MMP_Event OverlayDone;
    MMP_Event SwitchRDMABuffer;
    MMP_Event SwitchOverlayBuffer;
    MMP_Event StopOverlayBuffer;
    MMP_Event RDMA1RegisterUpdated;
    MMP_Event WDMA1RegisterUpdated;
} HDMI_MMP_Events;


// ---------------------------------------------------------------------------
//  Information Dump Routines
// ---------------------------------------------------------------------------

void init_hdmi_mmp_events(void)
{
    if (HDMI_MMP_Events.HDMI == 0)
    {
        HDMI_MMP_Events.HDMI = MMProfileRegisterEvent(MMP_RootEvent, "HDMI");
        HDMI_MMP_Events.OverlayDone = MMProfileRegisterEvent(HDMI_MMP_Events.HDMI, "OverlayDone");
        HDMI_MMP_Events.DDPKBitblt = MMProfileRegisterEvent(HDMI_MMP_Events.HDMI, "DDPKBitblt");
        HDMI_MMP_Events.SwitchRDMABuffer = MMProfileRegisterEvent(HDMI_MMP_Events.HDMI, "SwitchRDMABuffer");
        HDMI_MMP_Events.RDMA1RegisterUpdated = MMProfileRegisterEvent(HDMI_MMP_Events.HDMI, "RDMA1RegisterUpdated");
        HDMI_MMP_Events.SwitchOverlayBuffer = MMProfileRegisterEvent(HDMI_MMP_Events.HDMI, "SwitchOverlayBuffer");
        HDMI_MMP_Events.WDMA1RegisterUpdated = MMProfileRegisterEvent(HDMI_MMP_Events.HDMI, "WDMA1RegisterUpdated");
        HDMI_MMP_Events.StopOverlayBuffer = MMProfileRegisterEvent(HDMI_MMP_Events.HDMI, "StopOverlayBuffer");
        MMProfileEnableEventRecursive(HDMI_MMP_Events.HDMI, 1);
    }
}

//static int hdmi_default_width = 1280;
//static int hdmi_default_height = 720;

#define ENABLE_HDMI_FPS_CONTROL_LOG 1
#if ENABLE_HDMI_FPS_CONTROL_LOG
static unsigned int hdmi_fps_control_fps_wdma0 = 0;
static unsigned long hdmi_fps_control_time_base_wdma0 = 0;
static unsigned int hdmi_fps_control_fps_wdma1 = 0;
static unsigned long hdmi_fps_control_time_base_wdma1 = 0;
static unsigned int hdmi_fps_control_fps_rdma1 = 0;
static unsigned long hdmi_fps_control_time_base_rdma1 = 0;
#endif

typedef enum
{
    HDMI_OVERLAY_STATUS_STOPPED,
    HDMI_OVERLAY_STATUS_STOPPING,
    HDMI_OVERLAY_STATUS_STARTING,
    HDMI_OVERLAY_STATUS_STARTED,
}HDMI_OVERLAY_STATUS;

static unsigned int hdmi_fps_control_dpi = 0;
static unsigned int hdmi_fps_control_overlay = 0;
static HDMI_OVERLAY_STATUS hdmi_overlay_status = HDMI_OVERLAY_STATUS_STOPPED;
static unsigned int hdmi_rdma_switch_count = 0;

static int hdmi_buffer_write_id = 0;
static int hdmi_buffer_read_id = 0;
static int hdmi_buffer_read_id_tmp = 0;
static int hdmi_buffer_lcdw_id = 0;
static int hdmi_buffer_lcdw_id_tmp = 0;

static DPI_POLARITY clk_pol, de_pol, hsync_pol, vsync_pol;
static unsigned int dpi_clk_div, dpi_clk_duty, hsync_pulse_width, hsync_back_porch, hsync_front_porch, vsync_pulse_width, vsync_back_porch, vsync_front_porch, intermediat_buffer_num;
static HDMI_COLOR_ORDER rgb_order;

static struct task_struct *hdmi_update_task = NULL;
static wait_queue_head_t hdmi_update_wq;
static atomic_t hdmi_update_event = ATOMIC_INIT(0);

static struct task_struct *hdmi_overlay_config_task = NULL;
static wait_queue_head_t hdmi_overlay_config_wq;
static atomic_t hdmi_overlay_config_event = ATOMIC_INIT(0);

static struct task_struct *hdmi_rdma_config_task = NULL;
static wait_queue_head_t hdmi_rdma_config_wq;
static atomic_t hdmi_rdma_config_event = ATOMIC_INIT(0);

static wait_queue_head_t reg_update_wq;
static atomic_t reg_update_event = ATOMIC_INIT(0);

static wait_queue_head_t dst_reg_update_wq;
static atomic_t dst_reg_update_event = ATOMIC_INIT(0);

static unsigned int hdmi_resolution_param_table[][3] =
{
        #ifdef MTK_MT8193_HDMI_SUPPORT
        {720,   480,    60},
        {720,   576,    50},
        {1280,  720,    60},
        {1280,  720,    50},
        
        {1920,  1080,   60},
        {1920,  1080,   50},
        {1920,  1080,   30},
        {1920,  1080,   25},
        {1920,  1080,   24},
        {1920,  1080,   23},
        {1920,  1080,   29},
        
        {1920,  1080,   60},
        {1920,  1080,   50},
        #else
        {720,   480,    60},
        {1280,  720,    60},
        {1920,  1080,   30},
        #endif
};

#define ENABLE_HDMI_BUFFER_LOG 1
#if ENABLE_HDMI_BUFFER_LOG
bool enable_hdmi_buffer_log = 0;
#define HDMI_BUFFER_LOG(fmt, arg...) \
    do { \
        if(enable_hdmi_buffer_log){printk("[hdmi_buffer] "); printk(fmt, ##arg);} \
    }while (0)
#else
bool enable_hdmi_buffer_log = 0;
#define HDMI_BUFFER_LOG(fmt, arg...)
#endif

int hdmi_rdma_buffer_switch_mode = 0; // 0: switch in rdma1 frame done interrupt, 1: switch after DDPK_Bitblt done
static int hdmi_buffer_num = 0;
static int* hdmi_buffer_available = 0;
static int* hdmi_buffer_queue = 0;
static int hdmi_buffer_end = 0;
static int hdmi_buffer_start = 0;
static int hdmi_buffer_fill_count = 0;
static DEFINE_SEMAPHORE(hdmi_buffer_mutex);

static void hdmi_buffer_init(int num)
{
    int i;

    if(down_interruptible(&hdmi_buffer_mutex))
    {
        HDMI_LOG("Can't get semaphore in %s()\n", __func__);
        return;
    }

    HDMI_FUNC();

    hdmi_buffer_num = num;
    hdmi_buffer_start = 0;
    hdmi_buffer_end = 0;
    hdmi_buffer_fill_count = 0;

    hdmi_buffer_write_id = 0;
    hdmi_buffer_read_id = 0;
    hdmi_buffer_lcdw_id = 0;
    //hdmi_buffer_lcdr_id = 0;

    hdmi_buffer_available = (int*)vmalloc(hdmi_buffer_num * sizeof(int));
    hdmi_buffer_queue = (int*)vmalloc(hdmi_buffer_num * sizeof(int));

    for(i=0; i<hdmi_buffer_num; i++)
    {
        hdmi_buffer_available[i] = 1;
        hdmi_buffer_queue[i] = -1;
    }

    up(&hdmi_buffer_mutex);
}

static void hdmi_buffer_deinit(void)
{
    if(down_interruptible(&hdmi_buffer_mutex))
    {
        HDMI_LOG("Can't get semaphore in %s()\n", __func__);
        return;
    }

    HDMI_FUNC();

    hdmi_buffer_start = 0;
    hdmi_buffer_end = 0;
    hdmi_buffer_fill_count = 0;

    if(hdmi_buffer_available)
    {
        vfree((const void*)hdmi_buffer_available);
        hdmi_buffer_available = 0;
    }

    if(hdmi_buffer_queue)
    {
        vfree((const void*)hdmi_buffer_queue);
        hdmi_buffer_queue = 0;
    }

    up(&hdmi_buffer_mutex);
}

static void hdmi_dump_buffer_queue(void)
{
    HDMI_BUFFER_LOG("[hdmi] available={%d,%d,%d,%d} queue={%d,%d,%d,%d}, {start,end}={%d,%d} count=%d\n",
            hdmi_buffer_available[0], hdmi_buffer_available[1], hdmi_buffer_available[2], hdmi_buffer_available[3],
            hdmi_buffer_queue[hdmi_buffer_start],
            hdmi_buffer_queue[(hdmi_buffer_start+1)%hdmi_buffer_num],
            hdmi_buffer_queue[(hdmi_buffer_start+2)%hdmi_buffer_num],
            hdmi_buffer_queue[(hdmi_buffer_start+3)%hdmi_buffer_num],
            hdmi_buffer_start, hdmi_buffer_end, hdmi_buffer_fill_count);
}

static int hdmi_is_buffer_empty(void)
{
    return hdmi_buffer_fill_count == 0;
}

static void hdmi_release_buffer(int index)
{
    //down_interruptible(&hdmi_buffer_mutex);

    HDMI_BUFFER_LOG("[hdmi] hdmi_release_buffer: %d\n", index);
    hdmi_buffer_available[index] = 1;
    hdmi_dump_buffer_queue();

    //up(&hdmi_buffer_mutex);
}

static int hdmi_acquire_buffer(void)
{
    int index = -1;

    if(down_interruptible(&hdmi_buffer_mutex))
    {
        HDMI_LOG("Can't get semaphore in %s()\n", __func__);
        return -1;
    }

    if(!hdmi_is_buffer_empty())
    {
        index = hdmi_buffer_queue[hdmi_buffer_start];

        HDMI_BUFFER_LOG("[hdmi] hdmi_acquire_buffer: %d\n", index);

        hdmi_buffer_queue[hdmi_buffer_start] = -1;
        hdmi_buffer_start = (hdmi_buffer_start + 1) % hdmi_buffer_num;

        hdmi_buffer_fill_count--;

        hdmi_dump_buffer_queue();
    }

    up(&hdmi_buffer_mutex);
    return index;
}

static int hdmi_dequeue_buffer(void)
{
    int i;
    if(down_interruptible(&hdmi_buffer_mutex))
    {
        HDMI_LOG("Can't get semaphore in %s()\n", __func__);
        return -1;
    }

    for(i=0; i<hdmi_buffer_num; i++)
    {
        if(hdmi_buffer_available[i])
        {
            hdmi_buffer_available[i] = 0;
            HDMI_BUFFER_LOG("[hdmi] hdmi_dequeue_buffer: %d\n", i);

            hdmi_dump_buffer_queue();
            up(&hdmi_buffer_mutex);
            return i;
        }
    }
#if 1
    // if no available buffer, return the last buffer in queue
    if(!hdmi_is_buffer_empty())
    {
        int index = hdmi_buffer_queue[hdmi_buffer_end];
        HDMI_BUFFER_LOG("[hdmi] hdmi_dequeue_buffer(last): %d\n", index);

        hdmi_buffer_queue[hdmi_buffer_end] = -1;
        hdmi_buffer_end = (hdmi_buffer_end + (hdmi_buffer_num - 1)) % hdmi_buffer_num;
        hdmi_buffer_fill_count--;

        hdmi_dump_buffer_queue();
        up(&hdmi_buffer_mutex);
        return index;
    }
#endif
    up(&hdmi_buffer_mutex);
    HDMI_LOG("no available buffer\n");
    return -1;
}

static void hdmi_enqueue_buffer(int index)
{
    if(down_interruptible(&hdmi_buffer_mutex))
    {
        HDMI_LOG("Can't get semaphore in %s()\n", __func__);
        hdmi_release_buffer(index);
        return;
    }

    HDMI_BUFFER_LOG("[hdmi] hdmi_enqueue_buffer: %d\n", index);

    //hdmi_buffer_available[index] = 1;
    hdmi_buffer_end = (hdmi_buffer_start + hdmi_buffer_fill_count) % hdmi_buffer_num;
    hdmi_buffer_queue[hdmi_buffer_end] = index;

    if(hdmi_buffer_fill_count == hdmi_buffer_num)
        hdmi_buffer_start = (hdmi_buffer_start + 1) % hdmi_buffer_num;
    else
        hdmi_buffer_fill_count++;

    hdmi_dump_buffer_queue();

    up(&hdmi_buffer_mutex);
}

static unsigned long get_current_time_us(void)
{
    struct timeval t;
    do_gettimeofday(&t);
    return t.tv_sec * 1000 + t.tv_usec / 1000;
}

static void hdmi_udelay(unsigned int us)
{
	udelay(us);
}

static void hdmi_mdelay(unsigned int ms)
{
	msleep(ms);
}

static unsigned int hdmi_get_width(HDMI_VIDEO_RESOLUTION r)
{
    ASSERT(r < HDMI_VIDEO_RESOLUTION_NUM);
    return hdmi_resolution_param_table[r][0];
}

static unsigned int hdmi_get_height(HDMI_VIDEO_RESOLUTION r)
{
    ASSERT(r < HDMI_VIDEO_RESOLUTION_NUM);
    return hdmi_resolution_param_table[r][1];
}


static atomic_t hdmi_fake_in = ATOMIC_INIT(false);
#define IS_HDMI_FAKE_PLUG_IN()  (true == atomic_read(&hdmi_fake_in))
#define SET_HDMI_FAKE_PLUG_IN() (atomic_set(&hdmi_fake_in, true))
#define SET_HDMI_NOT_FAKE()     (atomic_set(&hdmi_fake_in, false))

// For Debugfs
void hdmi_cable_fake_plug_in(void)
{
    SET_HDMI_FAKE_PLUG_IN();
    HDMI_LOG("[HDMIFake]Cable Plug In\n");
    if(p->is_force_disable == false)
    {
        if (IS_HDMI_STANDBY())
        {
            hdmi_resume( );
            msleep(1000);
            switch_set_state(&hdmi_switch_data, HDMI_STATE_ACTIVE);
        }
    }
}

// For Debugfs
void hdmi_cable_fake_plug_out(void)
{
    SET_HDMI_NOT_FAKE();
    HDMI_LOG("[HDMIFake]Disable\n");
    if(p->is_force_disable == false)
    {
        if (IS_HDMI_ON())
        {
            if (hdmi_drv->get_state() != HDMI_STATE_ACTIVE)
            {
                hdmi_suspend( );
                switch_set_state(&hdmi_switch_data, HDMI_STATE_NO_DEVICE);
            }
        }
    }
}

void hdmi_set_mode(unsigned char ucMode)
{
    HDMI_FUNC();

    hdmi_drv->set_mode(ucMode);

    return;
}

void hdmi_reg_dump(void)
{
	hdmi_drv->dump();
}
#ifdef MTK_MT8193_HDMI_SUPPORT
void hdmi_read_reg(unsigned char u8Reg, unsigned int *p4Data)
{
	hdmi_drv->read(u8Reg, p4Data);
}
#else
void hdmi_read_reg(unsigned char u8Reg)
{
	hdmi_drv->read(u8Reg);
}
#endif
void hdmi_write_reg(unsigned char u8Reg, unsigned char u8Data)
{
	hdmi_drv->write(u8Reg, u8Data);
}

/* Used for HDMI Driver update */
static int hdmi_update_kthread(void *data)
{
    struct sched_param param = { .sched_priority = RTPM_PRIO_SCRN_UPDATE };
    sched_setscheduler(current, SCHED_RR, &param);

    for( ;; ) {
        wait_event_interruptible(hdmi_update_wq, atomic_read(&hdmi_update_event));
        //HDMI_LOG("wq wakeup\n");
        //hdmi_update_impl();

        ///atomic_set(&hdmi_update_event,0);
        hdmi_update_impl();

        atomic_set(&hdmi_update_event, 0);

        if (kthread_should_stop())
            break;
    }

    return 0;
}

/* Will be called in LCD Interrupt handler to check whether HDMI is actived */
bool is_hdmi_active(void)
{
	return IS_HDMI_ON();
}

void hdmi_wdma1_done(void)
{
#if ENABLE_HDMI_FPS_CONTROL_LOG
    unsigned long currentTime = get_current_time_us();
    hdmi_fps_control_fps_wdma1++;

    if(currentTime - hdmi_fps_control_time_base_wdma1 >= 1000)
    {
        HDMI_LOG("overlay>wdma1 fps:%d\n", hdmi_fps_control_fps_wdma1);
        hdmi_fps_control_time_base_wdma1 = currentTime;
        hdmi_fps_control_fps_wdma1 = 0;
    }
#endif
}

void hdmi_wdma0_done(void)
{
#if ENABLE_HDMI_FPS_CONTROL_LOG
    unsigned long currentTime = get_current_time_us();
    hdmi_fps_control_fps_wdma0++;

    if(currentTime - hdmi_fps_control_time_base_wdma0 >= 1000)
    {
        HDMI_LOG("rot>scl>wdma0 fps:%d\n", hdmi_fps_control_fps_wdma0);
        hdmi_fps_control_time_base_wdma0 = currentTime;
        hdmi_fps_control_fps_wdma0 = 0;
    }
#endif
}

void hdmi_rdma1_done(void)
{
#if ENABLE_HDMI_FPS_CONTROL_LOG
    unsigned long currentTime = get_current_time_us();
    hdmi_fps_control_fps_rdma1++;

    if(currentTime - hdmi_fps_control_time_base_rdma1 >= 1000)
    {
        HDMI_LOG("rdma1>dpi fps:%d(%d)\n", hdmi_fps_control_fps_rdma1, hdmi_rdma_switch_count);
        hdmi_fps_control_time_base_rdma1 = currentTime;
        hdmi_fps_control_fps_rdma1 = 0;
        hdmi_rdma_switch_count = 0;
    }
#endif
}

/* Used for HDMI Driver update */
static int hdmi_overlay_config_kthread(void *data)
{
    unsigned int addr = 0;
    bool needStopWdma1 = false;
    struct disp_path_config_mem_out_struct wdma1Config = {0};
    struct sched_param param = { .sched_priority = RTPM_PRIO_SCRN_UPDATE };
    sched_setscheduler(current, SCHED_RR, &param);

///hdmi_performance_tuning
	if(wdma1_bpp == 2)
	    wdma1Config.outFormat = WDMA_OUTPUT_FORMAT_UYVY;////WDMA_OUTPUT_FORMAT_RGB888;
	else
	    wdma1Config.outFormat = WDMA_OUTPUT_FORMAT_RGB888;

    wdma1Config.srcROI.x = 0;
    wdma1Config.srcROI.y = 0;

    for( ;; ) {

        wait_event_interruptible(hdmi_overlay_config_wq, atomic_read(&hdmi_overlay_config_event));
        atomic_set(&hdmi_overlay_config_event, 0);

        needStopWdma1 = hdmi_fps_control_overlay > hdmi_fps_control_dpi + 1;

        if(hdmi_overlay_status != HDMI_OVERLAY_STATUS_STOPPING &&
           !(needStopWdma1 && hdmi_overlay_status == HDMI_OVERLAY_STATUS_STOPPED))
        {
            disp_path_get_mutex();

            if(needStopWdma1)
            {
                //HDMI_LOG("Stop WDMA1\n");
                unsigned int reg;

                MMProfileLogEx(HDMI_MMP_Events.StopOverlayBuffer, MMProfileFlagStart, hdmi_fps_control_overlay, 0);

                // Stop WDMA1
                wdma1Config.enable = 0;
                disp_path_config_mem_out(&wdma1Config);

                WDMAStop(1);

                hdmi_overlay_status = HDMI_OVERLAY_STATUS_STOPPING;

                MMProfileLogEx(HDMI_MMP_Events.StopOverlayBuffer, MMProfileFlagEnd, hdmi_fps_control_overlay, 0);
            }
            else
            {
                hdmi_buffer_lcdw_id_tmp = (hdmi_buffer_lcdw_id+1)%hdmi_params->intermediat_buffer_num;
                addr = temp_mva_w + hdmi_buffer_lcdw_id_tmp * p->lcm_width * p->lcm_height * hdmi_bpp;

                MMProfileLogEx(HDMI_MMP_Events.SwitchOverlayBuffer, MMProfileFlagStart, hdmi_buffer_lcdw_id_tmp, hdmi_fps_control_overlay);

                // Start WDMA1
                wdma1Config.enable = 1;
                wdma1Config.dstAddr = addr;
                wdma1Config.srcROI.width = p->lcm_width;
                wdma1Config.srcROI.height = p->lcm_height;

                if(hdmi_overlay_status == HDMI_OVERLAY_STATUS_STOPPED)
                {
                    //HDMI_LOG("Start WDMA1, ovl_w=%d\n", hdmi_buffer_lcdw_id_tmp);
                    hdmi_overlay_status = HDMI_OVERLAY_STATUS_STARTING;
                }

                disp_path_config_mem_out(&wdma1Config);

                hdmi_fps_control_overlay++;

                MMProfileLogEx(HDMI_MMP_Events.SwitchOverlayBuffer, MMProfileFlagEnd, hdmi_buffer_lcdw_id_tmp, hdmi_fps_control_overlay);
            }

            disp_path_release_mutex();
        }

        if (kthread_should_stop())
                break;
    }

    return 0;
}

/* Switch LCD write buffer, will be called in LCD Interrupt handler */
void hdmi_source_buffer_switch(void)
{
    //printk("lcd write buffer:%d\n", hdmi_buffer_lcdw_id);

    RET_VOID_IF_NOLOG(IS_HDMI_NOT_ON());
    RET_VOID_IF_NOLOG(p->output_mode == HDMI_OUTPUT_MODE_DPI_BYPASS);
    RET_VOID_IF_NOLOG(IS_HDMI_IN_VIDEO_MODE());

    if(p->lcm_is_video_mode)
    {
        atomic_set(&hdmi_overlay_config_event, 1);
        wake_up_interruptible(&hdmi_overlay_config_wq);
        //HDMI_Config_Overlay_to_Memory(temp_mva_w + p->lcm_width * p->lcm_height * 3 * hdmi_buffer_lcdw_id_tmp, 1);
    }
    else
    {
        hdmi_buffer_lcdw_id = (hdmi_buffer_lcdw_id + 1) % hdmi_params->intermediat_buffer_num;
///#ifdef hdmi_performance_tuning
		unsigned int outFormat = WDMA_OUTPUT_FORMAT_RGB888;
		if(wdma1_bpp == 2)
			outFormat = WDMA_OUTPUT_FORMAT_UYVY;
		
        HDMI_Config_Overlay_to_Memory(temp_mva_w + p->lcm_width * p->lcm_height * hdmi_bpp * hdmi_buffer_lcdw_id, 1, outFormat);
    }
}

void hdmi_set_rdma_address(int bufferIndex)
{
    unsigned int hdmiSourceAddr;
    disp_path_get_mutex_(dp_mutex_dst);
    hdmi_buffer_read_id_tmp = bufferIndex;
    hdmiSourceAddr = hdmi_mva_r + p->hdmi_width * p->hdmi_height * hdmi_bpp * bufferIndex;
    DISP_REG_SET(0x1000 + DISP_REG_RDMA_MEM_START_ADDR, hdmiSourceAddr);
    disp_path_release_mutex_(dp_mutex_dst);

    HDMI_BUFFER_LOG("rdma1 address set done, hdmi_r=%d\n", bufferIndex);
}

void hdmi_rdma_buffer_switch(void)
{
    int acquired;

#if 0
    if(hdmi_buffer_read_id == hdmi_buffer_read_id_tmp && (acquired = hdmi_acquire_buffer()) != -1)
    {
        MMProfileLogEx(HDMI_MMP_Events.SwitchRDMABuffer, MMProfileFlagStart, acquired, 0);
    }
        hdmi_set_rdma_address(acquired);
#else
    if((acquired = hdmi_acquire_buffer()) != -1)
    {
        unsigned int hdmiSourceAddr;

        disp_path_get_mutex_(dp_mutex_dst);

        if(hdmi_buffer_read_id != hdmi_buffer_read_id_tmp)
        {
            // if hdmi_buffer_read_id_tmp has not be writen to working register, drop it
            HDMI_BUFFER_LOG("drop %d\n", hdmi_buffer_read_id_tmp);
            hdmi_release_buffer(hdmi_buffer_read_id_tmp);
        }

        MMProfileLogEx(HDMI_MMP_Events.SwitchRDMABuffer, MMProfileFlagStart, acquired, hdmi_buffer_read_id_tmp);

        hdmi_buffer_read_id_tmp = acquired;
        hdmiSourceAddr = hdmi_mva_r + p->hdmi_width * p->hdmi_height * hdmi_bpp * hdmi_buffer_read_id_tmp;
        DISP_REG_SET(0x1000 + DISP_REG_RDMA_MEM_START_ADDR, hdmiSourceAddr);
        disp_path_release_mutex_(dp_mutex_dst);

        HDMI_BUFFER_LOG("rdma1 address set done, hdmi_r=%d\n", hdmi_buffer_read_id_tmp);

#endif

        hdmi_rdma_switch_count++;
    }
    else
    {
        MMProfileLogEx(HDMI_MMP_Events.SwitchRDMABuffer, MMProfileFlagStart, acquired, 0);

        HDMI_BUFFER_LOG("no available buffer, wait buffer...\n");
        hdmi_set_rdma_address(hdmi_buffer_read_id_tmp);
    }

    MMProfileLogEx(HDMI_MMP_Events.SwitchRDMABuffer, MMProfileFlagEnd, acquired, 0);
}

/* Switch DPI read buffer, will be called in DPI Interrupt handler */
void hdmi_update_buffer_switch(void)
{
    //HDMI_LOG("DPI read buffer:%d\n", hdmi_buffer_read_id);

    RET_VOID_IF_NOLOG(IS_HDMI_NOT_ON());
    RET_VOID_IF_NOLOG(p->output_mode == HDMI_OUTPUT_MODE_DPI_BYPASS);
#if 0
    if(IS_HDMI_IN_VIDEO_MODE())
    {
        if (IS_HDMI_VIDEO_MODE_DPI_IN_CHANGING_ADDRESS())
        {
            if (0 == *((volatile unsigned int*)(0xF208C020)))
            {
                SET_HDMI_VIDEO_MODE_DPI_CHANGE_ADDRESS_DONE();
                atomic_set(&hdmi_video_mode_event, 1);
                wake_up_interruptible(&hdmi_video_mode_wq);
            }
        }
    }
    else
    {
        //DPI_CHECK_RET(DPI_FBSetAddress(DPI_FB_0, hdmi_mva + p->hdmi_width*p->hdmi_height*3*hdmi_buffer_read_id));
    }
#endif

    if(p->lcm_is_video_mode && hdmi_rdma_buffer_switch_mode == 0)
    {
        if(hdmi_fps_control_dpi <= hdmi_fps_control_overlay + 1)
            hdmi_fps_control_dpi++;

        atomic_set(&hdmi_rdma_config_event, 1);
        wake_up_interruptible(&hdmi_rdma_config_wq);
    }
}

static int hdmi_rdma_config_kthread(void *data)
{
    struct sched_param param = { .sched_priority = RTPM_PRIO_SCRN_UPDATE };
    sched_setscheduler(current, SCHED_RR, &param);

    for( ;; ) {
        wait_event_interruptible(hdmi_rdma_config_wq, atomic_read(&hdmi_rdma_config_event));
        atomic_set(&hdmi_rdma_config_event, 0);

        hdmi_rdma_buffer_switch();

        if (kthread_should_stop())
            break;
    }

    return 0;
}

extern void DBG_OnTriggerHDMI(void);
extern void DBG_OnHDMIDone(void);

/* hdmi update api, will be called in LCD Interrupt handler */
void hdmi_update(void)
{
    //HDMI_FUNC();
#if 1
    RET_VOID_IF(IS_HDMI_NOT_ON());
    RET_VOID_IF_NOLOG(p->output_mode == HDMI_OUTPUT_MODE_DPI_BYPASS);

    atomic_set(&hdmi_update_event, 1);
    wake_up_interruptible(&hdmi_update_wq); //wake up hdmi_update_kthread() to do update
#else
    hdmi_update_impl();
#endif
}

static void hdmi_update_impl(void)
{
    //HDMI_LOG("hdmi_update_impl\n");
    int pixelSize =  p->hdmi_width * p->hdmi_height;
    int dataSize = pixelSize * hdmi_bpp;

    RET_VOID_IF_NOLOG(p->output_mode == HDMI_OUTPUT_MODE_DPI_BYPASS);

    if(pixelSize == 0)
    {
        HDMI_LOG("ignored[resolution is null]\n");
        return;
    }

    //HDMI_FUNC();
    if(down_interruptible(&hdmi_update_mutex))
    {
        HDMI_LOG("[HDMI] can't get semaphore in\n");
        return;
    }

    if (IS_HDMI_NOT_ON())
    {
        goto done;
    }
    if(IS_HDMI_IN_VIDEO_MODE())
    {
        goto done;
    }

    DBG_OnTriggerHDMI();
    //LCD_WaitForNotBusy();

    if(p->is_reconfig_needed && hdmi_va != 0)
    {
        memset((void*)hdmi_va, 0, dataSize * hdmi_params->intermediat_buffer_num);
        p->is_reconfig_needed = false;
    }
    if(temp_va != 0 && hdmi_va != 0)
    {
        DdpkBitbltConfig pddp;
        int dstOffset;
        int scaling = (100 - p->scaling_factor);
        int ovlReadBuffer = ((hdmi_buffer_lcdw_id+(hdmi_params->intermediat_buffer_num-1))%hdmi_params->intermediat_buffer_num);
        memset((void*)&pddp, 0, sizeof(DdpkBitbltConfig));

        pddp.srcX = pddp.srcY = 0;
        pddp.srcW = p->lcm_width;
        pddp.srcH   = p->lcm_height;
        pddp.srcWStride = p->lcm_width;
        pddp.srcHStride = p->lcm_height;
        pddp.srcAddr[0] = temp_va + p->lcm_width * p->lcm_height * hdmi_bpp * ovlReadBuffer;

///#ifdef hdmi_performance_tuning
		if(wdma1_bpp == 2)
        	pddp.srcFormat = eYUV_422_I_K;///eRGB888_K;
    	else
			pddp.srcFormat = eRGB888_K;
		
        pddp.srcBufferSize[0] = p->lcm_width * p->lcm_height * wdma1_bpp;

        pddp.srcPlaneNum = 1;

        pddp.dstX = 0;
        pddp.dstY = 0;
///#ifdef hdmi_performance_tuning
		if(rmda1_bpp == 2)
	        pddp.dstFormat = eYUY2_K;
		else
			pddp.dstFormat = eRGB888_K;

        pddp.pitch = p->hdmi_width;
        pddp.dstWStride = p->hdmi_width;
        pddp.dstHStride = p->hdmi_height;
        pddp.dstPlaneNum = 1;

#if 0
        if(p->lcm_height > p->lcm_width)
        {
            pddp.orientation = 270;
        }
        else
        {
            pddp.orientation = 0;
        }
#else
		pddp.orientation = p->orientation;
#endif
		if(((pddp.orientation == 0 || pddp.orientation == 180) && p->lcm_height > p->lcm_width) ||
                         ((pddp.orientation == 90 || pddp.orientation == 270) && p->lcm_height < p->lcm_width))
		{
			if(p->lcm_height > p->lcm_width)
			    pddp.dstW = ALIGN_TO(p->lcm_width * p->hdmi_height / p->lcm_height, 4);
			else
			    pddp.dstW = ALIGN_TO(p->lcm_height * p->hdmi_height / p->lcm_width, 4);
			pddp.dstH = ALIGN_TO(p->hdmi_height,4);
		}
		else
		{
			pddp.dstW = ALIGN_TO(p->hdmi_width * scaling / 100, 4);
			pddp.dstH = ALIGN_TO(p->hdmi_height * scaling / 100, 4);
		}

        dstOffset = (p->hdmi_height - pddp.dstH) / 2 * p->hdmi_width * rmda1_bpp +
                    (p->hdmi_width - pddp.dstW) / 2 * rmda1_bpp;

        if(p->lcm_is_video_mode)
        {
            int dequeuedBuffer = hdmi_dequeue_buffer();

            if(dequeuedBuffer != -1)
            {
                hdmi_buffer_write_id = dequeuedBuffer;

                MMProfileLogEx(HDMI_MMP_Events.DDPKBitblt, MMProfileFlagStart, ovlReadBuffer, hdmi_buffer_write_id);

                pddp.dstAddr[0] = hdmi_va + hdmi_buffer_write_id * p->hdmi_width * p->hdmi_height * hdmi_bpp + dstOffset;
                pddp.dstBufferSize[0] = p->hdmi_width * p->hdmi_height * rmda1_bpp - dstOffset;

                HDMI_BUFFER_LOG("[hdmi_buffer] hdmi_r:%d, hdmi_w: %d, hdmi_r_tmp:%d\n", hdmi_buffer_read_id, hdmi_buffer_write_id, hdmi_buffer_read_id_tmp);

                DDPK_Bitblt_Config(DDPK_CH_HDMI_0, &pddp);
                DDPK_Bitblt(DDPK_CH_HDMI_0);

                hdmi_wdma0_done();

                //HDMI_LOG("DDPK_Bitblit done\n");

                hdmi_enqueue_buffer(hdmi_buffer_write_id);

                MMProfileLogEx(HDMI_MMP_Events.DDPKBitblt, MMProfileFlagEnd, ovlReadBuffer, hdmi_buffer_write_id);
            }
        }
        else
        {
            pddp.dstAddr[0] = hdmi_va + hdmi_buffer_write_id * p->hdmi_width * p->hdmi_height * hdmi_bpp + dstOffset;
            pddp.dstBufferSize[0] = p->hdmi_width * p->hdmi_height * rmda1_bpp - dstOffset;

            MMProfileLogEx(HDMI_MMP_Events.DDPKBitblt, MMProfileFlagStart, ovlReadBuffer, hdmi_buffer_write_id);

            DDPK_Bitblt_Config(DDPK_CH_HDMI_0, &pddp);
            DDPK_Bitblt(DDPK_CH_HDMI_0);

            MMProfileLogEx(HDMI_MMP_Events.DDPKBitblt, MMProfileFlagEnd, ovlReadBuffer, hdmi_buffer_write_id);
        }
    }

    //HDMI_LOG("dstw=%d, dsth=%d, ori=%d\n", p.dstW, p.dstH, p.orientation);

    DBG_OnHDMIDone();
    //HDMI_LOG("cost %d us\n", get_current_time_us() - t);

    if(p->lcm_is_video_mode)
    {
        if(hdmi_rdma_buffer_switch_mode == 1)
        {
            hdmi_rdma_buffer_switch();
        }
    }
    else
    {
        hdmi_buffer_read_id = hdmi_buffer_write_id;

        if(p->is_clock_on)
        {
            hdmi_set_rdma_address(hdmi_buffer_read_id);
        }

        hdmi_buffer_write_id = (hdmi_buffer_write_id+1) % hdmi_params->intermediat_buffer_num;
    }

done:
    up(&hdmi_update_mutex);

    return;
}

//--------------------------FIXME-------------------------------
DPI_STATUS hdmi_config_pll(HDMI_VIDEO_RESOLUTION resolution)
{
    unsigned int con1, con0;

    switch(resolution)
    {
        case HDMI_VIDEO_720x480p_60Hz:
#ifdef MTK_MT8193_HDMI_SUPPORT
        case HDMI_VIDEO_720x576p_50Hz:
#endif
        {
            con1 = 0x80109d89;
            con0 = 0x80800081;
            break;
        }
        case HDMI_VIDEO_1920x1080p_30Hz:
#ifdef MTK_MT8193_HDMI_SUPPORT
        case HDMI_VIDEO_1280x720p_50Hz:
        case HDMI_VIDEO_1920x1080i_50Hz:
        case HDMI_VIDEO_1920x1080p_25Hz:
        case HDMI_VIDEO_1920x1080p_24Hz:
        case HDMI_VIDEO_1920x1080p_50Hz:
#endif
        {
            con1 = 0x800b6c4e;
            con0 = 0x80000081;
            break;
        }

        case HDMI_VIDEO_1280x720p_60Hz:
    #ifdef MTK_MT8193_HDMI_SUPPORT
        case HDMI_VIDEO_1920x1080i_60Hz:
        case HDMI_VIDEO_1920x1080p_23Hz:
        case HDMI_VIDEO_1920x1080p_29Hz:
        case HDMI_VIDEO_1920x1080p_60Hz:
    #endif
        {
            con1 = 0x800b6964;
            con0 = 0x80000081;
            break;
        }

        default:
        {
            printk("[hdmi] not supported format, %s, %d, format = %d\n", __func__, __LINE__, resolution);
            return DPI_STATUS_ERROR;
        }
    }

#if 0
    //DPI1 PLL
    DRV_SetReg32(TVDPLL_PWR_CON0, (0x1 << 0));  //PLL_PWR_ON
    udelay(2);
    DRV_ClrReg32(TVDPLL_PWR_CON0, (0x1 << 1));  //PLL_ISO_EN

    OUTREG32(TVDPLL_CON1, con1);
    OUTREG32(TVDPLL_CON0, con0);

    udelay(20);
#else
    if(enable_pll(TVDPLL, "hdmi_dpi"))
    {
        printk("[hdmi] enable_pll fail\n");
        return DPI_STATUS_ERROR;
    }

    //FIXME: TVDPLL_CON0 is always the same when use mt_clkmgr api
    OUTREG32(TVDPLL_CON0, con0);

    if(pll_fsel(TVDPLL, con1))
    {
        printk("[hdmi] pll_fsel fail\n");
        return DPI_STATUS_ERROR;
    }
#endif

	return DPI_STATUS_OK;
}

static void _rdma1_irq_handler(unsigned int param)
{
    RET_VOID_IF_NOLOG(!is_hdmi_active());
    RET_VOID_IF_NOLOG(!p->lcm_is_video_mode);


    if(param & 1) // rdma1 register updated
    {
        MMProfileLogEx(HDMI_MMP_Events.RDMA1RegisterUpdated, MMProfileFlagPulse, hdmi_buffer_read_id, hdmi_buffer_read_id_tmp);

        if(hdmi_buffer_read_id != hdmi_buffer_read_id_tmp)
        {
            hdmi_release_buffer(hdmi_buffer_read_id);
        }

        hdmi_buffer_read_id = hdmi_buffer_read_id_tmp;
        HDMI_BUFFER_LOG("rdma1 register updated (buffer: hdmi_r=%d)\n", hdmi_buffer_read_id);
        hdmi_update_buffer_switch();

        //MMProfileLogEx(HDMI_MMP_Events.RDMA1RegisterUpdated, MMProfileFlagEnd, hdmi_buffer_read_id, 0);
    }
}

static void _register_updated_irq_handler(unsigned int param)
{
    RET_VOID_IF_NOLOG(!is_hdmi_active());

    if(param & 1) // wdma1 register updated
    {
        atomic_set(&reg_update_event, 1);
        wake_up_interruptible(&reg_update_wq);
    }

    if(dp_mutex_dst != -1 && (param & (1 << dp_mutex_dst))) // rdma1>dpi register updated
    {
        atomic_set(&dst_reg_update_event, 1);
        wake_up_interruptible(&dst_reg_update_wq);
    }

    RET_VOID_IF_NOLOG(!p->lcm_is_video_mode);

    if(param & 1) // wdma1 register updated
    {
        MMProfileLogEx(HDMI_MMP_Events.WDMA1RegisterUpdated, MMProfileFlagPulse, hdmi_buffer_lcdw_id, hdmi_buffer_lcdw_id_tmp);

        if(hdmi_overlay_status == HDMI_OVERLAY_STATUS_STOPPING)
        {
            hdmi_overlay_status = HDMI_OVERLAY_STATUS_STOPPED;
        }
        else if(hdmi_overlay_status == HDMI_OVERLAY_STATUS_STARTING)
        {
            hdmi_overlay_status = HDMI_OVERLAY_STATUS_STARTED;

        }

        //if(hdmi_overlay_status == HDMI_OVERLAY_STATUS_STARTED)
        {
            hdmi_buffer_lcdw_id = hdmi_buffer_lcdw_id_tmp;
            HDMI_BUFFER_LOG("wdma1 register updated (buffer: ovl_w=%d)\n", hdmi_buffer_lcdw_id);
        }

        hdmi_source_buffer_switch();

        //MMProfileLogEx(HDMI_MMP_Events.WDMA1RegisterUpdated, MMProfileFlagEnd, hdmi_buffer_lcdw_id, 0);
    }
}
 
/* Allocate memory, set M4U, LCD, MDP, DPI */
/* LCD overlay to memory -> MDP resize and rotate to memory -> DPI read to HDMI */
/* Will only be used in ioctl(MTK_HDMI_AUDIO_VIDEO_ENABLE) */
static HDMI_STATUS hdmi_drv_init(void)
{
    int lcm_width, lcm_height;
    int tmpBufferSize;
    M4U_PORT_STRUCT m4uport;

    HDMI_FUNC();

    RETIF(p->output_mode == HDMI_OUTPUT_MODE_DPI_BYPASS, 0);

    p->hdmi_width = hdmi_get_width(hdmi_params->init_config.vformat);
    p->hdmi_height = hdmi_get_height(hdmi_params->init_config.vformat);

    lcm_width = DISP_GetScreenWidth();
    lcm_height = DISP_GetScreenHeight();

    //printk("[hdmi]%s, hdmi_width=%d, hdmi_height=%d\n", __func__, p->hdmi_width, p->hdmi_height);
    HDMI_LOG("lcm_width=%d, lcm_height=%d\n", lcm_width, lcm_height);

    tmpBufferSize = lcm_width * lcm_height * hdmi_bpp * hdmi_params->intermediat_buffer_num;

    temp_va = (unsigned int) vmalloc(tmpBufferSize);
    if (((void*) temp_va) == NULL)
    {
        HDMI_LOG("vmalloc %dbytes fail\n", tmpBufferSize);
        return -1;
    }

    // WDMA1
    if (m4u_alloc_mva(M4U_CLNTMOD_WDMA,
                temp_va,
                tmpBufferSize,
                0,
                0,
                &temp_mva_w))
    {
        HDMI_LOG("m4u_alloc_mva for temp_mva_w fail\n");
        return -1;
    }
    m4u_dma_cache_maint(M4U_CLNTMOD_WDMA,
            (void const *)temp_va,
            tmpBufferSize,
            DMA_BIDIRECTIONAL);

    m4uport.ePortID = M4U_PORT_WDMA1;
    m4uport.Virtuality = 1;
    m4uport.Security = 0;
    m4uport.domain = 0;     //domain : 0 1 2 3
    m4uport.Distance = 1;
    m4uport.Direction = 0;
    m4u_config_port(&m4uport);

    HDMI_LOG("temp_va=0x%08x, temp_mva_w=0x%08x\n", temp_va, temp_mva_w);


    p->lcm_width = lcm_width;
    p->lcm_height = lcm_height;
    p->lcm_is_video_mode = DISP_IsVideoMode();
    p->output_video_resolution = hdmi_params->init_config.vformat;
    p->output_audio_format = hdmi_params->init_config.aformat;
    p->scaling_factor = hdmi_params->scaling_factor < 10 ? hdmi_params->scaling_factor : 10;

    if(p->lcm_is_video_mode)
        hdmi_buffer_init(hdmi_params->intermediat_buffer_num);

    hdmi_dpi_config_clock(); // configure dpi clock

    hdmi_dpi_power_switch(false);   // but dpi power is still off

    disp_register_irq(DISP_MODULE_MUTEX, _register_updated_irq_handler);

    if(p->lcm_is_video_mode)
    {
        disp_register_irq(DISP_MODULE_RDMA1, _rdma1_irq_handler);

        if(!hdmi_rdma_config_task)
        {
            hdmi_rdma_config_task = kthread_create(hdmi_rdma_config_kthread, NULL, "hdmi_rdma_config_kthread");
            wake_up_process(hdmi_rdma_config_task);
        }
    }

    return HDMI_STATUS_OK;
}

//free IRQ
/*static*/ void hdmi_dpi_free_irq(void)
{
    RET_VOID_IF(p->output_mode == HDMI_OUTPUT_MODE_DPI_BYPASS);
    DPI_CHECK_RET(HDMI_DPI(_FreeIRQ)());
}

/* Release memory */
/* Will only be used in ioctl(MTK_HDMI_AUDIO_VIDEO_ENABLE) */
static  HDMI_STATUS hdmi_drv_deinit(void)
{
    int temp_va_size;

    HDMI_FUNC();
    RETIF(p->output_mode == HDMI_OUTPUT_MODE_DPI_BYPASS, 0);

#ifdef MTK_MT8193_HDMI_SUPPORT
    hdmi_display_path_overlay_config(false);
#endif

    disp_unregister_irq(DISP_MODULE_MUTEX, _register_updated_irq_handler);

    if(p->lcm_is_video_mode)
    {
        disp_unregister_irq(DISP_MODULE_RDMA1, _rdma1_irq_handler);
    }

    hdmi_dpi_power_switch(false);

#ifndef MTK_MT8193_HDMI_SUPPORT
    DISP_Config_Overlay_to_Memory(temp_mva_w, 0);
#endif

    if(p->lcm_is_video_mode)
        hdmi_buffer_deinit();

    //free temp_va & temp_mva
    HDMI_LOG("Free temp_va and temp_mva\n");
    temp_va_size = p->lcm_width * p->lcm_height * hdmi_bpp * hdmi_params->intermediat_buffer_num;
    if (temp_mva_w)
    {
        M4U_PORT_STRUCT m4uport;
        m4uport.ePortID =  M4U_PORT_WDMA1;
        m4uport.Virtuality = 0;
        m4uport.domain = 0;
        m4uport.Security = 0;
        m4uport.Distance = 1;
        m4uport.Direction = 0;
        m4u_config_port(&m4uport);

        m4u_dealloc_mva(M4U_CLNTMOD_WDMA,
                                temp_va,
                                temp_va_size,
                                temp_mva_w);
        temp_mva_w = 0;
    }

    if (temp_va)
    {
        vfree((void*) temp_va);
        temp_va = 0;
    }

    hdmi_free_hdmi_buffer();

    hdmi_dpi_free_irq();
    return HDMI_STATUS_OK;
}

static void hdmi_dpi_config_update(void)
{
    DPI_CHECK_RET(HDMI_DPI(_ConfigPixelClk)(clk_pol, dpi_clk_div, dpi_clk_duty));

    DPI_CHECK_RET(HDMI_DPI(_ConfigDataEnable)(de_pol)); // maybe no used

    DPI_CHECK_RET(HDMI_DPI(_ConfigHsync)(hsync_pol, hsync_pulse_width, hsync_back_porch, hsync_front_porch));

    DPI_CHECK_RET(HDMI_DPI(_ConfigVsync)(vsync_pol, vsync_pulse_width, vsync_back_porch, vsync_front_porch));

    DPI_CHECK_RET(HDMI_DPI(_FBSetSize)(p->hdmi_width, p->hdmi_height));

#ifdef MTK_MT8193_HDMI_SUPPORT
    //FIXME
    {
        // the following are sample codes
        DPI_CHECK_RET(DPI1_ESAVVTimingControlLeft(0, 0x1E, 0, 0));
        DPI_CHECK_RET(DPI1_MatrixCoef(0x1F53, 0x1EAD, 0x0200, 0x0132, 0x0259, 0x0075, 0x0200, 0x1E53, 0x1FA0));
        DPI_CHECK_RET(DPI1_MatrixPreOffset(0, 0, 0));
        DPI_CHECK_RET(DPI1_MatrixPostOffset(0x0800, 0, 0x0800));
        DPI_CHECK_RET(DPI1_CLPFSetting(0, FALSE));
        DPI_CHECK_RET(DPI1_SetChannelLimit(0x0010, 0x0FE0, 0x0010, 0x0FE0));
        DPI_CHECK_RET(DPI1_EmbeddedSyncSetting(TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE));
        DPI_CHECK_RET(DPI1_OutputSetting(DPI_OUTPUT_BIT_NUM_8BITS, FALSE, DPI_OUTPUT_CHANNEL_SWAP_RGB, DPI_OUTPUT_YC_MAP_CY));
        //DPI_CHECK_RET(DPI1_EnableColorBar());
        //DPI_CHECK_RET(DPI1_EnableBlackScreen());
    }
#endif
    {
        //DPI_CHECK_RET(DPI_FBSetAddress(DPI_FB_0, hdmi_mva));//??????????????????????
        DPI_CHECK_RET(HDMI_DPI(_FBSetPitch)(DPI_FB_0, p->hdmi_width*3)); // do nothing
        DPI_CHECK_RET(HDMI_DPI(_FBEnable)(DPI_FB_0, TRUE)); // do nothing
    }

    //OUTREG32(0xF208C090, 0x41);
    DPI_CHECK_RET(HDMI_DPI(_FBSetFormat)(DPI_FB_FORMAT_RGB888)); // do nothing

    if (HDMI_COLOR_ORDER_BGR == rgb_order)
    {
        DPI_CHECK_RET(HDMI_DPI(_SetRGBOrder)(DPI_RGB_ORDER_RGB, DPI_RGB_ORDER_BGR)); // do nothing
    }
    else
    {
        DPI_CHECK_RET(HDMI_DPI(_SetRGBOrder)(DPI_RGB_ORDER_RGB, DPI_RGB_ORDER_RGB)); // do nothing
    }
}


/* Will only be used in hdmi_drv_init(), this means that will only be use in ioctl(MTK_HDMI_AUDIO_VIDEO_ENABLE) */
/*static*/ void hdmi_dpi_config_clock(void)
{
    int ret = 0;

    RET_VOID_IF(p->output_mode == HDMI_OUTPUT_MODE_DPI_BYPASS);

    ret = enable_pll(TVDPLL, "HDMI");
	p->is_clock_on = true;
    if(ret)
    {
        HDMI_LOG("enable_pll fail!!\n");
    }

    switch(hdmi_params->init_config.vformat)
    {
        case HDMI_VIDEO_720x480p_60Hz:
        {
            printk("[hdmi]480p\n");
            //ret = pll_fsel(TVDPLL, 0x1C7204C7);
            ASSERT(!ret);

            dpi_clk_div = 2;
            dpi_clk_duty = 1;

            break;
        }
        case HDMI_VIDEO_1280x720p_60Hz:
        {
            printk("[hdmi]720p 60Hz\n");
            //ret = pll_fsel(TVDPLL, 0xDBCD0119);
            ASSERT(!ret);

            dpi_clk_div = 2;
            dpi_clk_duty = 1;

            break;
        }
#ifdef MTK_MT8193_HDMI_SUPPORT
        case HDMI_VIDEO_1280x720p_50Hz: {
            printk("[hdmi]720p 50Hz\n");
            //ret = pll_fsel(TVDPLL, 0x1C7204C7);
            ASSERT(!ret);

            dpi_clk_div = 2;
            dpi_clk_duty = 1;

            break;
        }
#endif
        default:
        {
            printk("[hdmi] not supported format, %s, %d, format = %d\n", __func__, __LINE__, hdmi_params->init_config.vformat);
            break;
        }
    }

    clk_pol     = hdmi_params->clk_pol;
    de_pol      = hdmi_params->de_pol;
    hsync_pol   = hdmi_params->hsync_pol;
    vsync_pol   = hdmi_params->vsync_pol;;

    hsync_pulse_width   = hdmi_params->hsync_pulse_width;
    vsync_pulse_width   = hdmi_params->vsync_pulse_width;
    hsync_back_porch    = hdmi_params->hsync_back_porch;
    vsync_back_porch    = hdmi_params->vsync_back_porch;
    hsync_front_porch   = hdmi_params->hsync_front_porch;
    vsync_front_porch   = hdmi_params->vsync_front_porch;

    rgb_order           = hdmi_params->rgb_order;
    intermediat_buffer_num = hdmi_params->intermediat_buffer_num;

#if 0
    //DPI_CHECK_RET(HDMI_DPI(_Init_PLL)(hdmi_params->init_config.vformat));
    hdmi_config_pll(hdmi_params->init_config.vformat);//FIXME
    
    DPI_CHECK_RET(HDMI_DPI(_Init)(FALSE));
    DPI_CHECK_RET(HDMI_DPI(_ConfigHDMI)());

    hdmi_dpi_config_update();

    DPI_CHECK_RET(HDMI_DPI(_EnableClk)());

    p->is_clock_on = true;
#else
    DPI_CHECK_RET(HDMI_DPI(_Init)(FALSE));
#endif
}


int hdmi_allocate_hdmi_buffer(void)
{
    M4U_PORT_STRUCT m4uport;
    int hdmiPixelSize = p->hdmi_width * p->hdmi_height;
    int hdmiDataSize = hdmiPixelSize * hdmi_bpp;
    int hdmiBufferSize = hdmiDataSize * hdmi_params->intermediat_buffer_num;

    HDMI_FUNC();

    hdmi_va = (unsigned int) vmalloc(hdmiBufferSize);
    if (((void*) hdmi_va) == NULL)
    {
        HDMI_LOG("vmalloc %dbytes fail!!!\n", hdmiBufferSize);
        return -1;
    }

    memset((void*) hdmi_va, 0, hdmiBufferSize);

    //RDMA1
    if (m4u_alloc_mva(M4U_CLNTMOD_RDMA, hdmi_va, hdmiBufferSize, 0, 0, &hdmi_mva_r))
    {
        HDMI_LOG("m4u_alloc_mva for hdmi_mva_r fail\n");
        return -1;
    }
    memset((void*) &m4uport, 0, sizeof(M4U_PORT_STRUCT));
    m4uport.ePortID = M4U_PORT_RDMA1;
    m4uport.Virtuality = 1;
    m4uport.domain = 0;
    m4uport.Security = 0;
    m4uport.Distance = 1;
    m4uport.Direction = 0;
    m4u_config_port(&m4uport);

    HDMI_LOG("hdmi_va=0x%08x, hdmi_mva_r=0x%08x\n", hdmi_va, hdmi_mva_r);

    return 0;
}

int hdmi_free_hdmi_buffer(void)
{
    int hdmi_va_size = p->hdmi_width * p->hdmi_height * hdmi_bpp * hdmi_params->intermediat_buffer_num;

    //free hdmi_va & hdmi_mva
    HDMI_LOG("Free hdmi_va and hdmi_mva\n");

    if (hdmi_mva_r)
    {
        M4U_PORT_STRUCT m4uport;
        m4uport.ePortID =  M4U_PORT_RDMA1;
        m4uport.Virtuality = 0;
        m4uport.domain = 0;
        m4uport.Security = 0;
        m4uport.Distance = 1;
        m4uport.Direction = 0;
        m4u_config_port(&m4uport);

        m4u_dealloc_mva(M4U_CLNTMOD_RDMA,
                                hdmi_va,
                                hdmi_va_size,
                                hdmi_mva_r);
        hdmi_mva_r = 0;
    }

    if (hdmi_va)
    {
        vfree((void*) hdmi_va);
        hdmi_va = 0;
    }

    return 0;
}

int hdmi_rdma_address_config(bool enable, void* hdmi_mva)
{
    HDMI_FUNC();
    if(enable)
    {
        int rdmaInputFormat = rmda1_bpp == 2 ? RDMA_INPUT_FORMAT_UYVY : RDMA_INPUT_FORMAT_RGB888;
        unsigned int hdmiSourceAddr = (unsigned int)hdmi_mva;

        struct disp_path_config_struct config = {0};

        // Config RDMA->DPI1
        config.srcWidth = p->hdmi_width;
        config.srcHeight = p->hdmi_height;

        config.srcModule = DISP_MODULE_RDMA1;
        config.inFormat = rdmaInputFormat;
        config.outFormat = RDMA_OUTPUT_FORMAT_ARGB;
        config.addr = hdmiSourceAddr;
        config.pitch = config.srcWidth * rmda1_bpp;

        config.dstModule = HMID_DEST_DPI;

        //if(-1 == dp_mutex_dst)
        //    dp_mutex_dst = disp_lock_mutex();
        dp_mutex_dst = 2;
        if(-1 == dp_mutex_dst)
        {
            HDMI_LOG("Lock mutex for RDMA1>DPI1 fail\n");
            return -1;
        }
#if 0
        disp_dump_reg(DISP_MODULE_RDMA0);
        disp_dump_reg(DISP_MODULE_RDMA1);
        disp_dump_reg(DISP_MODULE_CONFIG);
        HDMI_LOG("Get mutex ID %d for RDMA1>DPI1\n", dp_mutex_dst);
#endif

        disp_path_get_mutex_(dp_mutex_dst);
        disp_path_config_(&config, dp_mutex_dst);
        disp_path_release_mutex_(dp_mutex_dst);

#if 0
        disp_dump_reg(DISP_MODULE_CONFIG);
        disp_dump_reg(DISP_MODULE_RDMA0);
        disp_dump_reg(DISP_MODULE_RDMA1);
#endif
    }
    else
    {
        if(-1 != dp_mutex_dst)
        {
            //FIXME: release mutex timeout
            HDMI_LOG("Stop RDMA1>DPI1\n");
            disp_path_get_mutex_(dp_mutex_dst);

            DISP_REG_SET_FIELD(1 << dp_mutex_src , DISP_REG_CONFIG_MUTEX_INTEN,  1);
            RDMAStop(1);
            RDMAReset(1);
            disp_path_release_mutex_(dp_mutex_dst);

            //disp_unlock_mutex(dp_mutex_dst);
            dp_mutex_dst = -1;
        }
    }

    return 0;
}
int hdmi_dst_display_path_config(bool enable)
{
    HDMI_FUNC();
    if(enable)
    {
        //FIXME: now nothing can be seen on TV if output UYVY from WDMA0
        int rdmaInputFormat = rmda1_bpp == 2 ? RDMA_INPUT_FORMAT_UYVY : RDMA_INPUT_FORMAT_RGB888;
        
        unsigned int hdmiSourceAddr = hdmi_mva_r + p->hdmi_width * p->hdmi_height * hdmi_bpp * hdmi_buffer_read_id;

        struct disp_path_config_struct config = {0};

        HDMI_LOG("hdmi_buffer_read_id: %d\n", hdmi_buffer_read_id);

        // Config RDMA->DPI1
        config.srcWidth = p->hdmi_width;
        config.srcHeight = p->hdmi_height;

        config.srcModule = DISP_MODULE_RDMA1;
        config.inFormat = rdmaInputFormat;
        config.outFormat = RDMA_OUTPUT_FORMAT_ARGB;
        config.addr = hdmiSourceAddr;
        config.pitch = config.srcWidth * rmda1_bpp;

        config.dstModule = HMID_DEST_DPI;

        //if(-1 == dp_mutex_dst)
        //    dp_mutex_dst = disp_lock_mutex();
        dp_mutex_dst = 2;
        if(-1 == dp_mutex_dst)
        {
            HDMI_LOG("Lock mutex for RDMA1>DPI1 fail\n");
            return -1;
        }
#if 0
        disp_dump_reg(DISP_MODULE_RDMA0);
        disp_dump_reg(DISP_MODULE_RDMA1);
        disp_dump_reg(DISP_MODULE_CONFIG);
        HDMI_LOG("Get mutex ID %d for RDMA1>DPI1\n", dp_mutex_dst);
#endif

        disp_path_get_mutex_(dp_mutex_dst);
        disp_path_config_(&config, dp_mutex_dst);
        disp_path_release_mutex_(dp_mutex_dst);

#if 0
        disp_dump_reg(DISP_MODULE_CONFIG);
        disp_dump_reg(DISP_MODULE_RDMA0);
        disp_dump_reg(DISP_MODULE_RDMA1);
#endif
    }
    else
    {
        if(-1 != dp_mutex_dst)
        {
            //FIXME: release mutex timeout
            HDMI_LOG("Stop RDMA1>DPI1\n");
            disp_path_get_mutex_(dp_mutex_dst);
            atomic_set(&dst_reg_update_event, 0);
            DISP_REG_SET_FIELD(1 << dp_mutex_src , DISP_REG_CONFIG_MUTEX_INTEN,  1);
            RDMAStop(1);
            disp_path_release_mutex_(dp_mutex_dst);

            wait_event_interruptible_timeout(dst_reg_update_wq, atomic_read(&dst_reg_update_event), HZ);

            //disp_unlock_mutex(dp_mutex_dst);
            dp_mutex_dst = -1;
        }
    }

    return 0;
}

int hdmi_display_path_overlay_config(bool enable)
{
    HDMI_FUNC();
    if(enable)
    {
        unsigned int wdma1OutputAddr = temp_mva_w + p->lcm_width * p->lcm_height * hdmi_bpp * hdmi_buffer_lcdw_id;

        struct disp_path_config_mem_out_struct wdma1Config = {0};

        // Config OVL->WDMA1
        wdma1Config.enable = 1;
        wdma1Config.dstAddr = wdma1OutputAddr;
///#ifdef hdmi_performance_tuning
		if(wdma1_bpp == 2)
	        wdma1Config.outFormat = WDMA_OUTPUT_FORMAT_UYVY;
		else
	        wdma1Config.outFormat = WDMA_OUTPUT_FORMAT_RGB888;


        // ROI for WDMA1
        wdma1Config.srcROI.x = 0;
        wdma1Config.srcROI.y = 0;
        wdma1Config.srcROI.width = p->lcm_width;
        wdma1Config.srcROI.height = p->lcm_height;

        disp_path_get_mutex();
        disp_path_config_mem_out(&wdma1Config);
        hdmi_overlay_status = HDMI_OVERLAY_STATUS_STARTING;
        disp_path_release_mutex();
    }
    else
    {
        struct disp_path_config_mem_out_struct wdma1Config = {0};

        HDMI_LOG("Stop WDMA1 memory out\n");

        disp_path_get_mutex();
        atomic_set(&reg_update_event, 0);
        disp_path_config_mem_out(&wdma1Config);
        hdmi_overlay_status = HDMI_OVERLAY_STATUS_STOPPING;
        disp_path_release_mutex();

        wait_event_interruptible_timeout(reg_update_wq, atomic_read(&reg_update_event), HZ);
    }

    return 0;
}

/* Switch DPI Power for HDMI Driver */
/*static*/ void hdmi_dpi_power_switch(bool enable)
{
    int ret;

    HDMI_LOG("DPI clock:%d\n", enable);

    RET_VOID_IF(p->output_mode == HDMI_OUTPUT_MODE_DPI_BYPASS);

    if(enable)
    {
        if(p->is_clock_on == true)
        {
            HDMI_LOG("power on request while already powered on!\n");
            return;
        }

        ret = enable_pll(TVDPLL, "HDMI");
        if(ret)
        {
            HDMI_LOG("enable_pll fail!!\n");
            return;
        }
        HDMI_DPI(_PowerOn)();
        HDMI_DPI(_EnableIrq)();
        DPI_CHECK_RET(HDMI_DPI(_EnableClk)());

        p->is_clock_on = true;
    }
    else
    {
        if(p->is_clock_on == false)
        {
            HDMI_LOG("power off request while already powered off!\n");
            return;
        }

        p->is_clock_on = false;

        HDMI_DPI(_DisableIrq)();
        HDMI_DPI(_DisableClk)();
        HDMI_DPI(_PowerOff)();

        ret = disable_pll(TVDPLL, "HDMI");
        if(ret)
        {
            HDMI_LOG("disable_pll fail!!\n");
            //return;
        }

#if 0
        if (IS_HDMI_VIDEO_MODE_DPI_IN_CHANGING_ADDRESS())
        {
            SET_HDMI_VIDEO_MODE_DPI_CHANGE_ADDRESS_DONE();
            atomic_set(&hdmi_video_mode_event, 1);
            wake_up_interruptible(&hdmi_video_mode_wq);
        }
#endif
    }
}

/* Configure video attribute */
static int hdmi_video_config(HDMI_VIDEO_RESOLUTION vformat, HDMI_VIDEO_INPUT_FORMAT vin, HDMI_VIDEO_OUTPUT_FORMAT vout)
{
	HDMI_FUNC();
	RETIF(IS_HDMI_NOT_ON(), 0);

    hdmi_allocate_hdmi_buffer();
    //hdmi_src_display_path_config(true);
    hdmi_dst_display_path_config(true);

    hdmi_fps_control_overlay = 0;
    hdmi_fps_control_dpi = 0;

    return hdmi_drv->video_config(vformat, vin, vout);
}

/* Configure audio attribute, will be called by audio driver */
int hdmi_audio_config(int samplerate)
{
    HDMI_FUNC();
	RETIF(!p->is_enabled, 0);
	RETIF(IS_HDMI_NOT_ON(), 0);

    HDMI_LOG("sample rate=%d\n", samplerate);
    if(samplerate == 48000)
    {
        p->output_audio_format = HDMI_AUDIO_PCM_16bit_48000;
    }
    else if(samplerate == 44100)
    {
        p->output_audio_format = HDMI_AUDIO_PCM_16bit_44100;
    }
    else if(samplerate == 32000)
	{
        p->output_audio_format = HDMI_AUDIO_PCM_16bit_32000;
    }
    else
    {
        HDMI_LOG("samplerate not support:%d\n", samplerate);
    }


    hdmi_drv->audio_config(p->output_audio_format);

    return 0;
}

/* No one will use this function */
/*static*/ int hdmi_video_enable(bool enable)
{
    HDMI_FUNC();


	return hdmi_drv->video_enable(enable);
}

/* No one will use this function */
/*static*/ int hdmi_audio_enable(bool enable)
{
    HDMI_FUNC();


	return hdmi_drv->audio_enable(enable);
}

struct timer_list timer;
void __timer_isr(unsigned long n)
{
    HDMI_FUNC();
    if(hdmi_drv->audio_enable) hdmi_drv->audio_enable(true);

    del_timer(&timer);
}

int hdmi_audio_delay_mute(int latency)
{
    HDMI_FUNC();
    memset((void*)&timer, 0, sizeof(timer));
    timer.expires = jiffies +  ( latency * HZ / 1000 );
    timer.function = __timer_isr;
    init_timer(&timer);
    add_timer(&timer);
    if(hdmi_drv->audio_enable) hdmi_drv->audio_enable(false);
    return 0;
}

/* Reset HDMI Driver state */
static void hdmi_state_reset(void)
{
    HDMI_FUNC();

    if(hdmi_drv->get_state() == HDMI_STATE_ACTIVE)
    {
        switch_set_state(&hdmi_switch_data, HDMI_STATE_ACTIVE);
    }
    else
    {
        switch_set_state(&hdmi_switch_data, HDMI_STATE_NO_DEVICE);
    }
}

/* HDMI Driver state callback function */
void hdmi_state_callback(HDMI_STATE state)
{

    printk("[hdmi]%s, state = %d\n", __func__, state);

    RET_VOID_IF((p->is_force_disable == true));
    RET_VOID_IF(IS_HDMI_FAKE_PLUG_IN());

    switch(state)
    {
        case HDMI_STATE_NO_DEVICE:
        {
            hdmi_suspend();
            switch_set_state(&hdmi_switch_data, HDMI_STATE_NO_DEVICE);

            break;
        }
        case HDMI_STATE_ACTIVE:
        {
            hdmi_resume();
            //force update screen
            DISP_UpdateScreen(0, 0, DISP_GetScreenWidth(), DISP_GetScreenHeight());
            if (HDMI_OUTPUT_MODE_LCD_MIRROR == p->output_mode) 
            {
                msleep(1000);
            }
            switch_set_state(&hdmi_switch_data, HDMI_STATE_ACTIVE); 

            break;
        }
		#ifdef MTK_MT8193_HDMI_SUPPORT
		case HDMI_STATE_PLUGIN_ONLY:
        {   
            switch_set_state(&hdmi_switch_data, HDMI_STATE_PLUGIN_ONLY);

            break;
        }
		case HDMI_STATE_EDID_UPDATE:
        {
            switch_set_state(&hdmi_switch_data, HDMI_STATE_EDID_UPDATE);

            break;
        }
		case HDMI_STATE_CEC_UPDATE:
        {
            switch_set_state(&hdmi_switch_data, HDMI_STATE_CEC_UPDATE);

            break;
        }		
		#endif
        default:
        {
            printk("[hdmi]%s, state not support\n", __func__);
            break;
        }
    }

    return;
}

/*static*/ void hdmi_power_on(void)
{
    HDMI_FUNC();

    RET_VOID_IF(IS_HDMI_NOT_OFF());

	if (down_interruptible(&hdmi_update_mutex)) {
			printk("[hdmi][HDMI] can't get semaphore in %s()\n", __func__);
			return;
	}

	// Why we set power state before calling hdmi_drv->power_on()?
	// Because when power on, the hpd irq will come immediately, that means hdmi_resume will be called before hdmi_drv->power_on() retuen here.
	// So we have to ensure the power state is STANDBY before hdmi_resume() be called.
	SET_HDMI_STANDBY();

    hdmi_drv->power_on();

	// When camera is open, the state will only be changed when camera exits.
	// So we bypass state_reset here, if camera is open.
	// The related scenario is: suspend in camera with hdmi enabled.
	// Why need state_reset() here?
	// When we suspend the phone, and then plug out hdmi cable, the hdmi chip status will change immediately
	// But when we resume the phone and check hdmi status, the irq will never come again
	// So we have to reset hdmi state manually, to ensure the status is the same between the host and hdmi chip.
	#ifndef MTK_MT8193_HDMI_SUPPORT
    if(p->is_force_disable == false)
    {
        if (IS_HDMI_FAKE_PLUG_IN())
        {
            //FixMe, deadlock may happened here, due to recursive use mutex
            hdmi_resume();
            msleep(1000);
            switch_set_state(&hdmi_switch_data, HDMI_STATE_ACTIVE);
        }
        else
        {
            hdmi_state_reset();
            // this is just a ugly workaround for some tv sets...
            //if(hdmi_drv->get_state() == HDMI_STATE_ACTIVE)
            //	hdmi_resume();
        }
    }
    #endif
    up(&hdmi_update_mutex);

    return;
}

/*static*/ void hdmi_power_off(void)
{
    HDMI_FUNC();

    RET_VOID_IF(IS_HDMI_OFF());

	if (down_interruptible(&hdmi_update_mutex)) {
			printk("[hdmi][HDMI] can't get semaphore in %s()\n", __func__);
			return;
	}

	hdmi_drv->power_off();
    hdmi_dpi_power_switch(false);
	SET_HDMI_OFF();
	up(&hdmi_update_mutex);

    return;
}

/*static*/ void hdmi_suspend(void)
{
    HDMI_FUNC();

    RET_VOID_IF(IS_HDMI_NOT_ON());

    if (down_interruptible(&hdmi_update_mutex)) {
        printk("[hdmi][HDMI] can't get semaphore in %s()\n", __func__);
        return;
    }
		
#ifdef MTK_MT8193_HDMI_SUPPORT
    hdmi_display_path_overlay_config(false);
    //hdmi_src_display_path_config(false);
#else
///#ifdef hdmi_performance_tuning
	unsigned int outFormat = WDMA_OUTPUT_FORMAT_RGB888;
	if(wdma1_bpp == 2)
	 	outFormat = WDMA_OUTPUT_FORMAT_UYVY;

    HDMI_Config_Overlay_to_Memory(temp_mva_w, 0, outFormat);
#endif
    hdmi_dpi_power_switch(false);

	hdmi_dst_display_path_config(false);

    hdmi_drv->suspend();
    SET_HDMI_STANDBY();

	disp_module_clock_off(DISP_MODULE_RDMA1, "HDMI");
	disp_module_clock_off(DISP_MODULE_GAMMA, "HDMI");		
	disp_module_clock_off(DISP_MODULE_WDMA1, "HDMI");		
    up(&hdmi_update_mutex);
}

/*static*/ void hdmi_resume(void)
{
    HDMI_FUNC();

    RET_VOID_IF(IS_HDMI_NOT_STANDBY());
    SET_HDMI_ON();

    if (down_interruptible(&hdmi_update_mutex)) {
        printk("[hdmi][HDMI] can't get semaphore in %s()\n", __func__);
        return;
    }

		disp_module_clock_on(DISP_MODULE_RDMA1, "HDMI");
		disp_module_clock_on(DISP_MODULE_GAMMA, "HDMI"); 	
		disp_module_clock_on(DISP_MODULE_WDMA1, "HDMI"); 	

#ifdef MTK_MT8193_HDMI_SUPPORT
    hdmi_display_path_overlay_config(true);
#else
///#ifdef hdmi_performance_tuning
	unsigned int outFormat = WDMA_OUTPUT_FORMAT_RGB888;
	if(wdma1_bpp == 2)
	 	outFormat = WDMA_OUTPUT_FORMAT_UYVY;

    HDMI_Config_Overlay_to_Memory(temp_mva_w, 1, outFormat);
#endif

	hdmi_dst_display_path_config(true);
    hdmi_dpi_power_switch(true);
    hdmi_drv->resume();
#if 0 // !defined(MTK_MT8193_HDMI_SUPPORT)
    hdmi_video_config(p->output_video_resolution, HDMI_VIN_FORMAT_RGB888, HDMI_VOUT_FORMAT_RGB888);
    hdmi_audio_config(44100);
#endif
    up(&hdmi_update_mutex);
}

/* Set HDMI orientation, will be called in mtkfb_ioctl(SET_ORIENTATION) */
/*static*/ void hdmi_setorientation(int orientation)
{
	HDMI_FUNC();
    ///RET_VOID_IF(!p->is_enabled);

	if(down_interruptible(&hdmi_update_mutex))
	{
		printk("[hdmi][HDMI] can't get semaphore in %s\n", __func__);
		return;
	}

	p->orientation = orientation;
	p->is_reconfig_needed = true;

//done:
	up(&hdmi_update_mutex);
}

static int hdmi_release(struct inode *inode, struct file *file)
{
	return 0;
}

static int hdmi_open(struct inode *inode, struct file *file)
{
	return 0;
}

static BOOL hdmi_drv_init_context(void);

static void dpi_setting_res(u8 arg)
{
	switch(arg)
	 {
	  case HDMI_VIDEO_720x480p_60Hz:
	  {
	
		 clk_pol	 = HDMI_POLARITY_FALLING;
		 de_pol 	 = HDMI_POLARITY_RISING;
		 hsync_pol	 = HDMI_POLARITY_RISING;
		 vsync_pol	 = HDMI_POLARITY_RISING;;
		 
		 dpi_clk_div = 2;
	
		 hsync_pulse_width	 = 62;
		 hsync_back_porch	 = 60;
		 hsync_front_porch	 = 16;
		 
		 vsync_pulse_width	 = 6;
		 vsync_back_porch	 = 30;
		 vsync_front_porch	 = 9;
		
		 p->hdmi_width = 720;
		 p->hdmi_height = 480;
		 p->output_video_resolution = HDMI_VIDEO_720x480p_60Hz;
		 break;
	  }
#ifdef MTK_MT8193_HDMI_SUPPORT
	  case HDMI_VIDEO_720x576p_50Hz:
	  {
	
		 clk_pol	 = HDMI_POLARITY_FALLING;
		 de_pol 	 = HDMI_POLARITY_RISING;
		 hsync_pol	 = HDMI_POLARITY_RISING;
		 vsync_pol	 = HDMI_POLARITY_RISING;;
		 
		 dpi_clk_div = 2;
	
		 hsync_pulse_width	 = 64;
		 hsync_back_porch	 = 68;
		 hsync_front_porch	 = 12;
		 
		 vsync_pulse_width	 = 5;
		 vsync_back_porch	 = 39;
		 vsync_front_porch	 = 5;
		
		 p->hdmi_width = 720;
		 p->hdmi_height = 576;
		 p->output_video_resolution = HDMI_VIDEO_720x576p_50Hz;
		 break;
	  }
#endif
	  case HDMI_VIDEO_1280x720p_60Hz:
	  {
	
		 clk_pol	 = HDMI_POLARITY_FALLING;
		 de_pol 	 = HDMI_POLARITY_RISING;
#if defined(HDMI_TDA19989)
         hsync_pol	 = HDMI_POLARITY_FALLING;
#else
		 hsync_pol	 = HDMI_POLARITY_RISING;
#endif
		 vsync_pol	 = HDMI_POLARITY_RISING;;
		 
		 dpi_clk_div = 2;
	
		 hsync_pulse_width	 = 40;
		 hsync_back_porch	 = 220;
		 hsync_front_porch	 = 110;
		 
		 vsync_pulse_width	 = 5;
		 vsync_back_porch	 = 20;
		 vsync_front_porch	 = 5;
		
		 p->hdmi_width = 1280;
		 p->hdmi_height = 720;
		 p->output_video_resolution = HDMI_VIDEO_1280x720p_60Hz;
		 break;
	  }
#ifdef MTK_MT8193_HDMI_SUPPORT
	  case HDMI_VIDEO_1280x720p_50Hz:
	  {
	
		 clk_pol	 = HDMI_POLARITY_FALLING;
		 de_pol 	 = HDMI_POLARITY_RISING;
		 hsync_pol	 = HDMI_POLARITY_RISING;
		 vsync_pol	 = HDMI_POLARITY_RISING;;
		 
		 dpi_clk_div = 2;
	
		 hsync_pulse_width	 = 40;
		 hsync_back_porch	 = 220;
		 hsync_front_porch	 = 440;
		 
		 vsync_pulse_width	 = 5;
		 vsync_back_porch	 = 20;
		 vsync_front_porch	 = 5;
		
		 p->hdmi_width = 1280;
		 p->hdmi_height = 720;
		 p->output_video_resolution = HDMI_VIDEO_1280x720p_50Hz;
		 break;
	  }
	  case HDMI_VIDEO_1920x1080p_24Hz:
	  {
	
		 clk_pol	 = HDMI_POLARITY_FALLING;
		 de_pol 	 = HDMI_POLARITY_RISING;
		 hsync_pol	 = HDMI_POLARITY_RISING;
		 vsync_pol	 = HDMI_POLARITY_RISING;;
		 
		 dpi_clk_div = 2;
	
		 hsync_pulse_width	 = 44;
		 hsync_back_porch	 = 148;
		 hsync_front_porch	 = 638;
		 
		 vsync_pulse_width	 = 5;
		 vsync_back_porch	 = 36;
		 vsync_front_porch	 = 4;
		
		 p->hdmi_width = 1920;
		 p->hdmi_height = 1080;
		 p->output_video_resolution = HDMI_VIDEO_1920x1080p_24Hz;
		 break;
	  }
	  case HDMI_VIDEO_1920x1080p_25Hz:
	  {
	
		 clk_pol	 = HDMI_POLARITY_FALLING;
		 de_pol 	 = HDMI_POLARITY_RISING;
		 hsync_pol	 = HDMI_POLARITY_RISING;
		 vsync_pol	 = HDMI_POLARITY_RISING;;
		 
		 dpi_clk_div = 2;
	
		 hsync_pulse_width	 = 44;
		 hsync_back_porch	 = 148;
		 hsync_front_porch	 = 528;
		 
		 vsync_pulse_width	 = 5;
		 vsync_back_porch	 = 36;
		 vsync_front_porch	 = 4;
		
		 p->hdmi_width = 1920;
		 p->hdmi_height = 1080;
		 p->output_video_resolution = HDMI_VIDEO_1920x1080p_25Hz;
		 break;
	  }
#endif
	  case HDMI_VIDEO_1920x1080p_30Hz:
	  {
	
		 clk_pol	 = HDMI_POLARITY_FALLING;
		 de_pol 	 = HDMI_POLARITY_RISING;
		 hsync_pol	 = HDMI_POLARITY_RISING;
		 vsync_pol	 = HDMI_POLARITY_RISING;;
		 
		 dpi_clk_div = 2;
	
		 hsync_pulse_width	 = 44;
		 hsync_back_porch	 = 148;
		 hsync_front_porch	 = 88;
		 
		 vsync_pulse_width	 = 5;
		 vsync_back_porch	 = 36;
		 vsync_front_porch	 = 4;
		
		 p->hdmi_width = 1920;
		 p->hdmi_height = 1080;
		 p->output_video_resolution = HDMI_VIDEO_1920x1080p_30Hz;
		 break;
	  }
#ifdef MTK_MT8193_HDMI_SUPPORT
	  case HDMI_VIDEO_1920x1080p_29Hz:
	  {
	
		 clk_pol	 = HDMI_POLARITY_FALLING;
		 de_pol 	 = HDMI_POLARITY_RISING;
		 hsync_pol	 = HDMI_POLARITY_RISING;
		 vsync_pol	 = HDMI_POLARITY_RISING;;
		 
		 dpi_clk_div = 2;
	
		 hsync_pulse_width	 = 44;
		 hsync_back_porch	 = 148;
		 hsync_front_porch	 = 88;
		 
		 vsync_pulse_width	 = 5;
		 vsync_back_porch	 = 36;
		 vsync_front_porch	 = 4;
		
		 p->hdmi_width = 1920;
		 p->hdmi_height = 1080;
		 p->output_video_resolution = HDMI_VIDEO_1920x1080p_29Hz;
		 break;
	  }
	  case HDMI_VIDEO_1920x1080p_23Hz:
	  {
	
		 clk_pol	 = HDMI_POLARITY_FALLING;
		 de_pol 	 = HDMI_POLARITY_RISING;
		 hsync_pol	 = HDMI_POLARITY_RISING;
		 vsync_pol	 = HDMI_POLARITY_RISING;;
		 
		 dpi_clk_div = 2;
	
		 hsync_pulse_width	 = 44;
		 hsync_back_porch	 = 148;
		 hsync_front_porch	 = 638;
		 
		 vsync_pulse_width	 = 5;
		 vsync_back_porch	 = 36;
		 vsync_front_porch	 = 4;
		
		 p->hdmi_width = 1920;
		 p->hdmi_height = 1080;
		 p->output_video_resolution = HDMI_VIDEO_1920x1080p_23Hz;
		 break;
	  }
#endif
	  default:
	  	break;
	 }

}

void	MTK_HDMI_Set_Security_Output(int enable)
{
	RETIF(!p->is_enabled, 0);
    RETIF(IS_HDMI_OFF(), 0);

	if(enable)
	{
		if(hdmi_drv->get_state() == HDMI_STATE_ACTIVE)
		{
				hdmi_resume();
				msleep(1000);
				switch_set_state(&hdmi_switch_data, HDMI_STATE_ACTIVE);
		}
	}
	else
	{
		if(hdmi_drv->get_state() == HDMI_STATE_ACTIVE)
		{
			hdmi_suspend();
			switch_set_state(&hdmi_switch_data, HDMI_STATE_NO_DEVICE);
		}
	}
}

static long hdmi_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;

	int r = 0;
	#ifdef MTK_MT8193_HDMI_SUPPORT
    hdmi_device_write w_info;
	hdmi_hdcp_key key;
	send_slt_data send_sltdata;
	CEC_SLT_DATA get_sltdata;
    hdmi_para_setting data_info;
	HDMI_EDID_INFO_T pv_get_info;
	CEC_FRAME_DESCRIPTION cec_frame;
	CEC_ADDRESS cecaddr;
	CEC_DRV_ADDR_CFG_T cecsetAddr; 
	CEC_SEND_MSG_T cecsendframe;
	READ_REG_VALUE regval;
    #endif
	
	HDMI_LOG("[HDMI] hdmi ioctl= %s(%d), arg = %lu\n", _hdmi_ioctl_spy(cmd),cmd&0xff, arg);

    switch(cmd)
    {
       #ifdef MTK_MT8193_HDMI_SUPPORT
       case MTK_HDMI_WRITE_DEV:
       {
           if (copy_from_user(&w_info, (void __user *)arg, sizeof(w_info))) {
               HDMI_LOG("copy_from_user failed! line:%d \n", __LINE__);
               r = -EFAULT;
           } else {
               hdmi_drv->write(w_info.u4Addr & 0xFFFF, w_info.u4Data);
           }            
           break;
       }

	   case MTK_HDMI_INFOFRAME_SETTING:
       {
           if (copy_from_user(&data_info, (void __user *)arg, sizeof(data_info))) {
               HDMI_LOG("copy_from_user failed! line:%d \n", __LINE__);
               r = -EFAULT;
           } else {
               hdmi_drv->InfoframeSetting(data_info.u4Data1 & 0xFF, data_info.u4Data2 & 0xFF);
           }            
           break;
       }
	   
	   case MTK_HDMI_HDCP_KEY:
       {
           if (copy_from_user(&key, (void __user *)arg, sizeof(key))) {
               HDMI_LOG("copy_from_user failed! line:%d \n", __LINE__);
               r = -EFAULT;
           } else {
               hdmi_drv->hdcpkey((UINT8*)&key);
           }            
           break;
       }
	   
	   case MTK_HDMI_SETLA:
       {
           if (copy_from_user(&cecsetAddr, (void __user *)arg, sizeof(cecsetAddr))) {
               HDMI_LOG("copy_from_user failed! line:%d \n", __LINE__);
               r = -EFAULT;
           } else {
               hdmi_drv->setcecla(&cecsetAddr);
           }            
           break;
       }
	   
	   case MTK_HDMI_SENDSLTDATA:
       {
           if (copy_from_user(&send_sltdata, (void __user *)arg, sizeof(send_sltdata))) {
               HDMI_LOG("copy_from_user failed! line:%d \n", __LINE__);
               r = -EFAULT;
           } else {
               hdmi_drv->sendsltdata((UINT8*)&send_sltdata);
           }            
           break;
       }	
	   
	   case MTK_HDMI_SET_CECCMD:
       {
           if (copy_from_user(&cecsendframe, (void __user *)arg, sizeof(cecsendframe))) {
               HDMI_LOG("copy_from_user failed! line:%d \n", __LINE__);
               r = -EFAULT;
           } else {
               hdmi_drv->setceccmd(&cecsendframe);
           }            
           break;
       }	 

	   case MTK_HDMI_CEC_ENABLE:
	   {
		   hdmi_drv->cecenable(arg & 0xFF); 	
		   break;
	   }

	   
	   case MTK_HDMI_GET_EDID:
       {
	   	   hdmi_drv->getedid(&pv_get_info);
           if (copy_to_user((void __user *)arg, &pv_get_info, sizeof(pv_get_info))) {
               HDMI_LOG("copy_to_user failed! line:%d \n", __LINE__);
               r = -EFAULT;
           }           
           break;
       }
	   
	   case MTK_HDMI_GET_CECCMD:
       {
	   	   hdmi_drv->getceccmd(&cec_frame);
           if (copy_to_user((void __user *)arg, &cec_frame, sizeof(cec_frame))) {
               HDMI_LOG("copy_to_user failed! line:%d \n", __LINE__);
               r = -EFAULT;
           }           
           break;
       }
	   
	   case MTK_HDMI_GET_SLTDATA:
       {
	   	   hdmi_drv->getsltdata(&get_sltdata);
           if (copy_to_user((void __user *)arg, &get_sltdata, sizeof(get_sltdata))) {
               HDMI_LOG("copy_to_user failed! line:%d \n", __LINE__);
               r = -EFAULT;
           }           
           break;
       }

	   case MTK_HDMI_GET_CECADDR:
       {
	   	   hdmi_drv->getcecaddr(&cecaddr);
           if (copy_to_user((void __user *)arg, &cecaddr, sizeof(cecaddr))) {
               HDMI_LOG("copy_to_user failed! line:%d \n", __LINE__);
               r = -EFAULT;
           }       
           break;
       }

	   case MTK_HDMI_COLOR_DEEP:
       {
           if (copy_from_user(&data_info, (void __user *)arg, sizeof(data_info))) {
               HDMI_LOG("copy_from_user failed! line:%d \n", __LINE__);
               r = -EFAULT;
           } else {
               hdmi_drv->colordeep(data_info.u4Data1 & 0xFF, data_info.u4Data2 & 0xFF);
           }            
           break;
       }
	   
       case MTK_HDMI_READ_DEV:
       {
           if (copy_from_user(&regval, (void __user *)arg, sizeof(regval))) {
           HDMI_LOG("copy_from_user failed! line:%d \n", __LINE__);
           r = -EFAULT;
           } else {
              hdmi_drv->read(regval.u1adress & 0xFFFF, &regval.pu1Data);    
           }
		   
           if (copy_to_user((void __user *)arg, &regval, sizeof(regval))) {
           HDMI_LOG("copy_to_user failed! line:%d \n", __LINE__);
           r = -EFAULT;
           }   
           break;
       }

	   case MTK_HDMI_ENABLE_LOG:
       {
           hdmi_drv->log_enable(arg & 0xFFFF);     
           break;
       }
	   
	   case MTK_HDMI_ENABLE_HDCP:
       {
           hdmi_drv->enablehdcp(arg & 0xFFFF);     
           break;
       }
	   
	   case MTK_HDMI_CECRX_MODE:
       {
           hdmi_drv->setcecrxmode(arg & 0xFFFF);     
           break;
       }

	   case MTK_HDMI_STATUS:
       {
           hdmi_drv->hdmistatus();     
           break;
       }

	   case MTK_HDMI_CHECK_EDID:
       {
           hdmi_drv->checkedid(arg & 0xFF);     
           break;
       }
	   #endif
	   
       case MTK_HDMI_AUDIO_VIDEO_ENABLE:
       {
            if (arg)
            {
                if(p->is_enabled)
                    return 0;

                HDMI_CHECK_RET(hdmi_drv_init());
                if(hdmi_drv->enter) hdmi_drv->enter();
                hdmi_power_on();
                p->is_enabled = true;
            }
            else
            {
                if(!p->is_enabled)
                    return 0;

                //when disable hdmi, HPD is disabled
                switch_set_state(&hdmi_switch_data, HDMI_STATE_NO_DEVICE);

                hdmi_dst_display_path_config(false);

                hdmi_power_off();

                //wait hdmi finish update
                if (down_interruptible(&hdmi_update_mutex)) {
                    printk("[hdmi][HDMI] can't get semaphore in %s()\n", __func__);
                    return -EFAULT;
                }
                HDMI_CHECK_RET(hdmi_drv_deinit());
                up(&hdmi_update_mutex);

                if(hdmi_drv->exit) hdmi_drv->exit();
                p->is_enabled = false;
            }

            break;
        }
		case MTK_HDMI_FORCE_FULLSCREEN_ON:
        case MTK_HDMI_FORCE_CLOSE:
        {
            RETIF(p->output_mode == HDMI_OUTPUT_MODE_DPI_BYPASS, 0);
            RETIF(!p->is_enabled, 0);
            RETIF(IS_HDMI_OFF(), 0);

            if(p->is_force_disable == true)
                break;

            if (IS_HDMI_FAKE_PLUG_IN())
            {
                hdmi_suspend();
                switch_set_state(&hdmi_switch_data, HDMI_STATE_NO_DEVICE);
            }
            else
            {
                if(hdmi_drv->get_state() == HDMI_STATE_ACTIVE)
                {
                    hdmi_suspend();
                    switch_set_state(&hdmi_switch_data, HDMI_STATE_NO_DEVICE);
                }
            }

            p->is_force_disable = true;

            break;
	    }
		case MTK_HDMI_FORCE_FULLSCREEN_OFF:
        case MTK_HDMI_FORCE_OPEN:
        {
            RETIF(p->output_mode == HDMI_OUTPUT_MODE_DPI_BYPASS, 0);
            RETIF(!p->is_enabled, 0);
            RETIF(IS_HDMI_OFF(), 0);

            if(p->is_force_disable == false)
                break;

            if (IS_HDMI_FAKE_PLUG_IN())
            {
                hdmi_resume();
                msleep(1000);
                switch_set_state(&hdmi_switch_data, HDMI_STATE_ACTIVE);
            }
            else
            {
                if(hdmi_drv->get_state() == HDMI_STATE_ACTIVE)
                {
                    hdmi_resume();
                    msleep(1000);
                    switch_set_state(&hdmi_switch_data, HDMI_STATE_ACTIVE);
                }
            }
            p->is_force_disable = false;

            break;
        }
        /* Shutdown thread(No matter IPO), system suspend/resume will go this way... */
        case MTK_HDMI_POWER_ENABLE:
        {
			RETIF(!p->is_enabled, 0);

            if (arg)
            {
                hdmi_power_on();
            }
            else
            {
                #ifdef MTK_MT8193_HDMI_SUPPORT
                switch_set_state(&hdmi_switch_data, HDMI_STATE_NO_DEVICE);
				#endif
                hdmi_power_off();
            }
            break;
        }
        case MTK_HDMI_AUDIO_ENABLE:
        {
			RETIF(!p->is_enabled, 0);

            if (arg)
            {
                HDMI_CHECK_RET(hdmi_audio_enable(true));
            }
            else
            {
                HDMI_CHECK_RET(hdmi_audio_enable(false));
            }

            break;
        }
        case MTK_HDMI_VIDEO_ENABLE:
        {
			RETIF(!p->is_enabled, 0);
            break;
        }
        case MTK_HDMI_VIDEO_CONFIG:
        {
            HDMI_LOG("video resolution configuration, arg=%ld\n", arg);

#ifndef MTK_MT8193_HDMI_SUPPORT
            if (arg > 1)
            {
                arg = 1;
            }
#endif
            RETIF(!p->is_enabled, 0);

            RETIF(IS_HDMI_NOT_ON(),0);

            if(down_interruptible(&hdmi_update_mutex))
            {
                HDMI_LOG("[HDMI] can't get semaphore in\n");
                return EAGAIN;
            }

            hdmi_dst_display_path_config(false);
            //hdmi_src_display_path_config(false);
            hdmi_free_hdmi_buffer();

			//FIXME, arg => vin format
            hdmi_config_pll(arg);

            dpi_setting_res((u8)arg);
            hdmi_video_config(p->output_video_resolution, HDMI_VIN_FORMAT_RGB888, HDMI_VOUT_FORMAT_RGB888);

            DPI_CHECK_RET(HDMI_DPI(_DisableClk)());
    	    DPI_CHECK_RET(HDMI_DPI(_ConfigHDMI)());
            hdmi_dpi_config_update();
            DPI_CHECK_RET(HDMI_DPI(_EnableClk)());

            p->is_clock_on = true;

            up(&hdmi_update_mutex);
            break;
        }
        case MTK_HDMI_AUDIO_CONFIG:
        {
			RETIF(!p->is_enabled, 0);

            break;
        }
        case MTK_HDMI_IS_FORCE_AWAKE:
        {
            if (!hdmi_drv_init_context())
	        {
	            printk("[hdmi]%s, hdmi_drv_init_context fail\n", __func__);
		        return HDMI_STATUS_NOT_IMPLEMENTED;
	        }
            r = copy_to_user(argp, &hdmi_params->is_force_awake, sizeof(hdmi_params->is_force_awake)) ? -EFAULT : 0;
            break;
        }
#if 1
        case MTK_HDMI_ENTER_VIDEO_MODE:
        {
			RETIF(!p->is_enabled, 0);
			RETIF(HDMI_OUTPUT_MODE_VIDEO_MODE != p->output_mode, 0);
            //FIXME
            //hdmi_dst_display_path_config(true, NULL);
            break;
        }
        case MTK_HDMI_REGISTER_VIDEO_BUFFER:
        {
            struct hdmi_video_buffer_info video_buffer_info;
			RETIF(!p->is_enabled, 0);
			RETIF(HDMI_OUTPUT_MODE_VIDEO_MODE != p->output_mode, 0);
            if(copy_from_user(&video_buffer_info, (void __user *)argp, sizeof(video_buffer_info)))
            {
                HDMI_LOG("copy_from_user failed! line\n");
                r = -EFAULT;
                break;
            }
			if(video_buffer_info.src_vir_addr == 0)
			{
                HDMI_LOG("[Error]HDMI_REGISTER_VIDEO_BUFFER VA should not be NULL\n");
                break;
			}
            if(down_interruptible(&hdmi_video_mode_mutex))
            {
                HDMI_LOG("[HDMI] can't get semaphore in\n");
                break;
            }
            if(hdmi_add_video_buffer(&video_buffer_info, file) < 0)
            {
                r = -ENOMEM;
            }
            up(&hdmi_video_mode_mutex);

            break;

        }
        case MTK_HDMI_POST_VIDEO_BUFFER:
        {
            struct hdmi_video_buffer_info video_buffer_info;
            struct hdmi_video_buffer_list* buffer_list;
			RETIF(!p->is_enabled, 0);
			RETIF(HDMI_OUTPUT_MODE_VIDEO_MODE != p->output_mode, 0);
            RETIF(IS_HDMI_NOT_ON(),0);
            if(copy_from_user(&video_buffer_info, (void __user *)argp, sizeof(video_buffer_info)))
            {
                HDMI_LOG("copy_from_user failed! line\n");
                r = -EFAULT;
                break;
            }
            if(down_interruptible(&hdmi_update_mutex))
            {
                HDMI_LOG("[HDMI] can't get semaphore in\n");
                break;
            }
            if(down_interruptible(&hdmi_video_mode_mutex))
            {
                HDMI_LOG("[HDMI] can't get semaphore in\n");
                up(&hdmi_update_mutex);
                break;
            }

            if(NULL == (buffer_list = hdmi_search_video_buffer(&video_buffer_info, file)))
            {
                up(&hdmi_video_mode_mutex);
                up(&hdmi_update_mutex);
                break;
            }

            if(IS_HDMI_IN_VIDEO_MODE())
            {
                //FIXME
            }
            else //Enter video mode the first time
            {
                SET_HDMI_TO_VIDEO_MODE();
            }
            up(&hdmi_update_mutex);

            DBG_OnTriggerHDMI();
            if(p->is_clock_on)
            {
#if 0
                //Wait for change buffer
                DPI_CHECK_RET(DPI_FBSetAddress(DPI_FB_0, buffer_list->buffer_mva));
                *((volatile unsigned int*)(0xF208C020)) = 1;
                SET_HDMI_VIDEO_MODE_DPI_CHANGE_ADDRESS();

                wait_event_interruptible(hdmi_video_mode_wq, atomic_read(&hdmi_video_mode_event));
                atomic_set(&hdmi_video_mode_event, 0);
#else
                //hdmi_dst_display_path_config(true, buffer_list->buffer_mva);
                hdmi_rdma_address_config(true, (void*)buffer_list->buffer_mva);
#endif
            }
            DBG_OnHDMIDone();

            up(&hdmi_video_mode_mutex);
            break;
        }
        case MTK_HDMI_LEAVE_VIDEO_MODE:
        {
			RETIF(!p->is_enabled, 0);
			RETIF(HDMI_OUTPUT_MODE_VIDEO_MODE != p->output_mode, 0);
            if(down_interruptible(&hdmi_update_mutex))
            {
                HDMI_LOG("[HDMI] can't get semaphore in\n");
                break;
            }

            if(down_interruptible(&hdmi_video_mode_mutex))
            {
                HDMI_LOG("[HDMI] can't get semaphore in\n");
                up(&hdmi_update_mutex);
                break;
            }
            if(p->is_clock_on)
            {
#if 0
                //Wait for change buffer
                DPI_CHECK_RET(DPI_FBSetAddress(DPI_FB_0, hdmi_mva + p->hdmi_width*p->hdmi_height*3*hdmi_buffer_read_id));
                *((volatile unsigned int*)(0xF208C020)) = 1;
                SET_HDMI_VIDEO_MODE_DPI_CHANGE_ADDRESS();

                wait_event_interruptible(hdmi_video_mode_wq, atomic_read(&hdmi_video_mode_event));
                atomic_set(&hdmi_video_mode_event, 0);
#endif
            }

            //hdmi_dst_display_path_config(false, NULL);
            hdmi_rdma_address_config(false, NULL);
            SET_HDMI_LEAVE_VIDEO_MODE();
            //Destory video buffer mva
            hdmi_destory_video_buffer();
            up(&hdmi_video_mode_mutex);
            up(&hdmi_update_mutex);
            break;
        }
#endif
        case MTK_HDMI_FACTORY_MODE_ENABLE:
        {
			if (hdmi_drv->power_on())
            {
                r = -EAGAIN;
                HDMI_LOG("Error factory mode test fail\n");
            }
            else
            {
                HDMI_LOG("before power off\n");
                hdmi_drv->power_off();
                HDMI_LOG("after power off\n");
            }
            break;
        }
        default:
        {
            printk("[hdmi][HDMI] arguments error\n");
            break;
        }
    }

	return r;
}


//VIDEO MODE
#if 1
void dump_video_buffer(char* banner)
{
#if 0
    struct hdmi_video_buffer_list *buffer_list;
    struct list_head *temp;
    HDMI_LOG("before add a node,list node Addr:\n");
    list_for_each(temp, hdmi_video_buffer_list_head)
    {
        buffer_list = list_entry(temp, struct hdmi_video_buffer_list, list);
        HDMI_LOG("0x%08x,    ", (unsigned int)temp);
    }
    HDMI_LOG("\n");
#else
    struct hdmi_video_buffer_list *temp;
    HDMI_LOG("%s\n", banner);
    list_for_each_entry(temp, hdmi_video_buffer_list_head, list)
    {
        HDMI_LOG("0x%x,    ", (unsigned int)temp);
    }
    HDMI_LOG("\n");
#endif
}
static int hdmi_add_video_buffer(struct hdmi_video_buffer_info *buffer_info, struct file *file)
{
    struct hdmi_video_buffer_list *buffer_list;
    int ret;
    M4U_PORT_STRUCT m4uport;
    int r = 0;

    dump_video_buffer("++++++++before add video buffer");
    buffer_list = (struct hdmi_video_buffer_list*)kmalloc(sizeof(struct hdmi_video_buffer_list), GFP_KERNEL);
    if(!buffer_list)
    {
        HDMI_LOG("kmalloc failed\n");
        r = -1;
        goto EXIT;
    }
    
    ret = m4u_alloc_mva(M4U_CLNTMOD_RDMA,
            (unsigned int)(buffer_info->src_vir_addr),
            buffer_info->src_size,
            0, 0,
            &buffer_list->buffer_mva);
    if(ret)
    {
        HDMI_LOG("allocate mva for dpi fail\n");
        r = -2;
        goto EXIT;
    }

    m4uport.ePortID = M4U_PORT_RDMA1;
    m4uport.Virtuality = 1;
    m4uport.domain = 0;
    m4uport.Security = 0;
    m4uport.Distance = 1;
    m4uport.Direction = 0;
    m4u_config_port(&m4uport);
    HDMI_LOG("hdmi_va=0x%08x, hdmi_mva=0x%08x\n", (unsigned int)(buffer_info->src_vir_addr), buffer_list->buffer_mva);

    buffer_list->buffer_info = *buffer_info;
    buffer_list->pid = current->tgid;
    buffer_list->file_addr = (void*)file;
    //HDMI_LOG("[current process ID = %d, current process name:%s]\n", current->tgid, current->comm);
    list_add_tail(&buffer_list->list, hdmi_video_buffer_list_head);
 
    dump_video_buffer("-----after add video buffer");
EXIT:
    return r;
}
static struct hdmi_video_buffer_list* hdmi_search_video_buffer(struct hdmi_video_buffer_info *buffer_info, struct file *file)
{
    struct hdmi_video_buffer_list *temp;
    list_for_each_entry(temp, hdmi_video_buffer_list_head, list)
    {
        if(temp->buffer_info.src_vir_addr == buffer_info->src_vir_addr && \
           temp->pid == current->tgid \
          )
        {
            return temp;
        }
    }
    HDMI_LOG("Error: Cannot find matched VA:0x%08x in buffer list\n", (unsigned int)(buffer_info->src_vir_addr));
    return NULL;
}

static void hdmi_destory_video_buffer(void)
{
    struct hdmi_video_buffer_list *temp, *next;
    int ret;
    list_for_each_entry_safe(temp, next, hdmi_video_buffer_list_head, list)
    {
        M4U_PORT_STRUCT m4uport;
        m4uport.ePortID = M4U_PORT_RDMA1;
        m4uport.Virtuality = 0;
        m4uport.domain = 0;
        m4uport.Security = 0;
        m4uport.Distance = 1;
        m4uport.Direction = 0;
        m4u_config_port(&m4uport);

        ret = m4u_dealloc_mva(M4U_CLNTMOD_RDMA,
                                (unsigned int)temp->buffer_info.src_vir_addr, 
                                temp->buffer_info.src_size,
                                temp->buffer_mva);
        if (ret < 0)
        {
            HDMI_LOG("Invlid M4U Error, vir_addr = 0x%08x, mva = 0x%08x\n", 
                    (unsigned int)(temp->buffer_info.src_vir_addr), temp->buffer_mva);
        }

        list_del(&temp->list);
        kfree(temp);
    }
}
#endif

static int hdmi_remove(struct platform_device *pdev)
{
	return 0;
}

static BOOL hdmi_drv_init_context(void)
{
	static const HDMI_UTIL_FUNCS hdmi_utils =
	{
		.udelay             	= hdmi_udelay,
		.mdelay             	= hdmi_mdelay,
		.state_callback			= hdmi_state_callback,
	};

	if (hdmi_drv != NULL)
		return TRUE;


    hdmi_drv = (HDMI_DRIVER*)HDMI_GetDriver();

	if (NULL == hdmi_drv) return FALSE;

	hdmi_drv->set_util_funcs(&hdmi_utils);
	hdmi_drv->get_params(hdmi_params);

	return TRUE;
}

static void __exit hdmi_exit(void)
{
	device_destroy(hdmi_class, hdmi_devno);
	class_destroy(hdmi_class);
	cdev_del(hdmi_cdev);
	unregister_chrdev_region(hdmi_devno, 1);

}

struct file_operations hdmi_fops = {
	.owner   = THIS_MODULE,
	.unlocked_ioctl   = hdmi_ioctl,
	.open    = hdmi_open,
	.release = hdmi_release,
};

static int hdmi_probe(struct platform_device *pdev)
{
    int ret = 0;
	struct class_device *class_dev = NULL;

    printk("[hdmi]%s\n", __func__);

    /* Allocate device number for hdmi driver */
	ret = alloc_chrdev_region(&hdmi_devno, 0, 1, HDMI_DEVNAME);
	if(ret)
	{
		printk("[hdmi]alloc_chrdev_region fail\n");
		return -1;
	}

    /* For character driver register to system, device number binded to file operations */
	hdmi_cdev = cdev_alloc();
	hdmi_cdev->owner = THIS_MODULE;
	hdmi_cdev->ops = &hdmi_fops;
	ret = cdev_add(hdmi_cdev, hdmi_devno, 1);

	/* For device number binded to device name(hdmitx), one class is corresponeded to one node */
	hdmi_class = class_create(THIS_MODULE, HDMI_DEVNAME);
	/* mknod /dev/hdmitx */
	class_dev = (struct class_device *)device_create(hdmi_class, NULL, hdmi_devno, NULL, HDMI_DEVNAME);

	printk("[hdmi][%s] current=0x%08x\n", __func__, (unsigned int)current);

    if (!hdmi_drv_init_context())
	{
	    printk("[hdmi]%s, hdmi_drv_init_context fail\n", __func__);
		return HDMI_STATUS_NOT_IMPLEMENTED;
	}

    init_waitqueue_head(&hdmi_video_mode_wq);
    INIT_LIST_HEAD(&hdmi_video_mode_buffer_list);

    init_waitqueue_head(&hdmi_update_wq);
    hdmi_update_task = kthread_create(hdmi_update_kthread, NULL, "hdmi_update_kthread");
    wake_up_process(hdmi_update_task);

    init_waitqueue_head(&hdmi_overlay_config_wq);
    hdmi_overlay_config_task = kthread_create(hdmi_overlay_config_kthread, NULL, "hdmi_overlay_config_kthread");
    wake_up_process(hdmi_overlay_config_task);

    init_waitqueue_head(&hdmi_rdma_config_wq);
    init_waitqueue_head(&reg_update_wq);
    init_waitqueue_head(&dst_reg_update_wq);

    return 0;
}

static struct platform_driver hdmi_driver = {
    .probe  = hdmi_probe,
    .remove = hdmi_remove,
    .driver = { .name = HDMI_DEVNAME }
};

static int __init hdmi_init(void)
{
    int ret = 0;
    printk("[hdmi]%s\n", __func__);


    if (platform_driver_register(&hdmi_driver))
    {
        printk("[hdmi]failed to register mtkfb driver\n");
        return -1;
    }

    memset((void*)&hdmi_context, 0, sizeof(_t_hdmi_context));
    SET_HDMI_OFF();
			
    init_hdmi_mmp_events();

	p->lcm_is_video_mode = DISP_IsVideoMode();
	if(p->lcm_is_video_mode )
		wdma1_bpp = 2;
	else
		wdma1_bpp = 3;

    if (!hdmi_drv_init_context())
    {
        printk("[hdmi]%s, hdmi_drv_init_context fail\n", __func__);
        return HDMI_STATUS_NOT_IMPLEMENTED;
    }

    p->output_mode = hdmi_params->output_mode;
	p->orientation = 0;
    hdmi_drv->init();
    HDMI_LOG("Output mode is %s\n", (hdmi_params->output_mode==HDMI_OUTPUT_MODE_DPI_BYPASS)?"HDMI_OUTPUT_MODE_DPI_BYPASS":"HDMI_OUTPUT_MODE_LCD_MIRROR");

    if(hdmi_params->output_mode == HDMI_OUTPUT_MODE_DPI_BYPASS)
    {
        p->output_video_resolution = HDMI_VIDEO_RESOLUTION_NUM;
    }

    HDMI_DBG_Init();

    hdmi_switch_data.name = "hdmi";
    hdmi_switch_data.index = 0;
    hdmi_switch_data.state = NO_DEVICE;

    // for support hdmi hotplug, inform AP the event
    ret = switch_dev_register(&hdmi_switch_data);
    if(ret)
    {
        printk("[hdmi][HDMI]switch_dev_register returned:%d!\n", ret);
        return 1;
    }

    return 0;
}

module_init(hdmi_init);
module_exit(hdmi_exit);
MODULE_AUTHOR("Xuecheng, Zhang <xuecheng.zhang@mediatek.com>");
MODULE_DESCRIPTION("HDMI Driver");
MODULE_LICENSE("GPL");

#endif
