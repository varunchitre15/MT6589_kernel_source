#include <linux/autoconf.h>
#include <linux/module.h>
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
#include <linux/semaphore.h>
#include <linux/xlog.h>
#include <linux/mutex.h>
#include <linux/leds-mt65xx.h>

#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/mach-types.h>
#include <asm/cacheflush.h>
#include <asm/io.h>

#include <mach/dma.h>
#include <mach/irqs.h>
#include "mach/mt_boot.h"

#include "debug.h"
#include "disp_drv.h"
#include <disp_drv_platform.h>
#include "disp_drv_log.h"
#include "dpi_drv.h"
#include "lcd_drv.h"
#if defined (MTK_TVOUT_SUPPORT)
#include "tv_out.h"
#endif

#include "mtkfb.h"
#include "mtkfb_console.h"
#include "mtkfb_info.h"
#include "ddp_ovl.h"

unsigned int EnableVSyncLog = 0;

#define INIT_FB_AS_COLOR_BAR    (0)

static u32 MTK_FB_XRES  = 0;
static u32 MTK_FB_YRES  = 0;
static u32 MTK_FB_BPP   = 0;
static u32 MTK_FB_PAGES = 0;
static u32 fb_xres_update = 0;
static u32 fb_yres_update = 0;
#define ALIGN_TO(x, n)  \
	(((x) + ((n) - 1)) & ~((n) - 1))

#define MTK_FB_XRESV (ALIGN_TO(MTK_FB_XRES, 32))
#define MTK_FB_YRESV (ALIGN_TO(MTK_FB_YRES, 32) * MTK_FB_PAGES) /* For page flipping */
#define MTK_FB_BYPP  ((MTK_FB_BPP + 7) >> 3)
#define MTK_FB_LINE  (ALIGN_TO(MTK_FB_XRES, 32) * MTK_FB_BYPP)
#define MTK_FB_SIZE  (MTK_FB_LINE * ALIGN_TO(MTK_FB_YRES, 32))

#define MTK_FB_SIZEV (MTK_FB_LINE * ALIGN_TO(MTK_FB_YRES, 32) * MTK_FB_PAGES)

#define CHECK_RET(expr)    \
    do {                   \
        int ret = (expr);  \
        ASSERT(0 == ret);  \
    } while (0)

static size_t mtkfb_log_on = false;
#define MTKFB_LOG(fmt, arg...) \
    do { \
        if (mtkfb_log_on) DISP_LOG_PRINT(ANDROID_LOG_WARN, "MTKFB", fmt, ##arg); \
    }while (0)

#define MTKFB_FUNC()	\
	do { \
		if(mtkfb_log_on) DISP_LOG_PRINT(ANDROID_LOG_INFO, "MTKFB", "[Func]%s\n", __func__); \
	}while (0)

#define PRNERR(fmt, args...)   DISP_LOG_PRINT(ANDROID_LOG_INFO, "MTKFB", fmt, ## args);

void mtkfb_log_enable(int enable)
{
    mtkfb_log_on = enable;
	MTKFB_LOG("mtkfb log %s\n", enable?"enabled":"disabled");
}

// ---------------------------------------------------------------------------
//  local variables
// ---------------------------------------------------------------------------

#if defined(MTK_M4U_SUPPORT)
#if defined(MTKFB_NO_M4U)
#undef MTK_M4U_SUPPORT
#endif
struct fb_overlay_buffer_list{
    struct fb_overlay_buffer_info buffer;
    unsigned int src_mva;
    pid_t pid;
    void* file_addr;
    struct fb_overlay_buffer_list *next;
};

static BOOL mtkfb_enable_m4u = TRUE;
unsigned int fb_pa = 0;
static unsigned int fb_va_m4u = 0;
static unsigned int fb_size_m4u = 0;
static unsigned int fb_mva_m4u = 0;

struct fb_overlay_buffer_list *overlay_buffer_head = NULL;
EXPORT_SYMBOL(overlay_buffer_head);
#endif
//#endif

static const struct timeval FRAME_INTERVAL = {0, 30000};  // 33ms

atomic_t has_pending_update = ATOMIC_INIT(0);
struct fb_overlay_layer video_layerInfo;
UINT32 dbr_backup = 0;
UINT32 dbg_backup = 0;
UINT32 dbb_backup = 0;
bool fblayer_dither_needed = false;
static unsigned int video_rotation = 0;
static UINT32 mtkfb_current_layer_type = LAYER_2D;
static UINT32 mtkfb_using_layer_type = LAYER_2D;
static bool	hwc_force_fb_enabled = true;
bool is_ipoh_bootup = false;
struct fb_info         *mtkfb_fbi;
struct fb_overlay_layer fb_layer_context;

/* This mutex is used to prevent tearing due to page flipping when adbd is
   reading the front buffer
*/
//DECLARE_MUTEX(sem_flipping);
DEFINE_SEMAPHORE(sem_flipping);
//DECLARE_MUTEX(sem_early_suspend);
DEFINE_SEMAPHORE(sem_early_suspend);
DEFINE_SEMAPHORE(sem_overlay_buffer);

extern OVL_CONFIG_STRUCT cached_layer_config[DDP_OVL_LAYER_MUN];
DEFINE_MUTEX(OverlaySettingMutex);
atomic_t OverlaySettingDirtyFlag = ATOMIC_INIT(0);
atomic_t OverlaySettingApplied = ATOMIC_INIT(0);
unsigned int PanDispSettingPending = 0;
unsigned int PanDispSettingDirty = 0;
unsigned int PanDispSettingApplied = 0;

DECLARE_WAIT_QUEUE_HEAD(reg_update_wq);

unsigned int need_esd_check = 0;
DECLARE_WAIT_QUEUE_HEAD(esd_check_wq);

extern unsigned int disp_running;
extern wait_queue_head_t disp_done_wq;

DEFINE_MUTEX(ScreenCaptureMutex);

BOOL is_early_suspended = FALSE;
static int sem_flipping_cnt = 1;
static int sem_early_suspend_cnt = 1;
static int sem_overlay_buffer_cnt = 1;
static int vsync_cnt = 0;

extern BOOL is_engine_in_suspend_mode;
extern BOOL is_lcm_in_suspend_mode;

// ---------------------------------------------------------------------------
//  local function declarations
// ---------------------------------------------------------------------------

static int init_framebuffer(struct fb_info *info);
static int mtkfb_set_overlay_layer(struct fb_info *info,
                                   struct fb_overlay_layer* layerInfo);
static int mtkfb_get_overlay_layer_info(struct fb_overlay_layer_info* layerInfo);
static int mtkfb_update_screen(struct fb_info *info);
static void mtkfb_update_screen_impl(void);
extern int is_pmem_range(unsigned long* base, unsigned long size);

static int mtkfb_set_s3d_ftm(struct fb_info *info, unsigned int mode);
#if defined(MTK_HDMI_SUPPORT) ||defined (MTK_WFD_SUPPORT)
extern void hdmi_setorientation(int orientation);
void hdmi_power_on(void);
void hdmi_power_off(void);
extern void hdmi_update_buffer_switch(void);
extern bool is_hdmi_active(void);
extern void hdmi_update(void);
extern void hdmi_source_buffer_switch(void);
extern void	MTK_HDMI_Set_Security_Output(int enable);
#endif
#if defined (MTK_WFD_SUPPORT)
extern void wfd_setorientation(int orientation);
extern void	MTK_WFD_Set_Security_Output(int enable);
#endif

// ---------------------------------------------------------------------------
//  Timer Routines
// ---------------------------------------------------------------------------

static struct task_struct *screen_update_task = NULL;
static struct task_struct *esd_recovery_task = NULL;
unsigned int lcd_fps = 6000;
wait_queue_head_t screen_update_wq;
extern BOOL dal_shown;

void mtkfb_pan_disp_test(void)
{
	MTKFB_FUNC();
	if (down_interruptible(&sem_flipping)) {
        printk("[fb driver] can't get semaphore:%d\n", __LINE__);
        return;
    }
	sem_flipping_cnt--;
	DISP_LOG_PRINT(ANDROID_LOG_WARN, "MTKFB", "wait sem_flipping\n");
	if (down_interruptible(&sem_early_suspend)) {
        printk("[fb driver] can't get semaphore:%d\n", __LINE__);
		sem_flipping_cnt++;
		up(&sem_flipping);
        return;
    }
	sem_early_suspend_cnt--;

	DISP_LOG_PRINT(ANDROID_LOG_WARN, "MTKFB", "wait sem_early_suspend\n");
	if (down_interruptible(&sem_overlay_buffer)) {
     	printk("[fb driver] can't get semaphore,%d\n", __LINE__);
		sem_early_suspend_cnt++;
		up(&sem_early_suspend);

		sem_flipping_cnt++;
		up(&sem_flipping);
     	return;
    }
	sem_overlay_buffer_cnt--;
	DISP_LOG_PRINT(ANDROID_LOG_WARN, "MTKFB", "wait sem_overlay_buffer\n");
	if (is_early_suspended) goto end;

	DISP_CHECK_RET(DISP_UpdateScreen(0, 0, fb_xres_update, fb_yres_update));
end:
	sem_overlay_buffer_cnt++;
	sem_early_suspend_cnt++;
	sem_flipping_cnt++;
    up(&sem_overlay_buffer);
    up(&sem_early_suspend);
	up(&sem_flipping);
}

void mtkfb_show_sem_cnt(void)
{
	printk("[FB driver: sem cnt = %d, %d, %d. fps = %d, vsync_cnt = %d\n", sem_overlay_buffer_cnt, sem_early_suspend_cnt, sem_flipping_cnt, lcd_fps, vsync_cnt);
	printk("[FB driver: sem cnt = %d, %d, %d\n", sem_overlay_buffer.count, sem_early_suspend.count, sem_flipping.count);
}

void mtkfb_hang_test(bool en)
{
	MTKFB_FUNC();
	if(en){
	if (down_interruptible(&sem_flipping)) {
        printk("[fb driver] can't get semaphore:%d\n", __LINE__);
        return;
    }
	sem_flipping_cnt--;
	}
	else{
	sem_flipping_cnt++;
	up(&sem_flipping);
	}
}

static int screen_update_kthread(void *data)
{
	struct sched_param param = { .sched_priority = RTPM_PRIO_SCRN_UPDATE };
	sched_setscheduler(current, SCHED_RR, &param);
    for( ;; ) {
        wait_event_interruptible(screen_update_wq, atomic_read(&has_pending_update));
		MTKFB_LOG("wq wakeup\n");
        mtkfb_update_screen_impl();

		atomic_set(&has_pending_update,0);
        if (kthread_should_stop())
            break;
    }

    return 0;
}

BOOL esd_kthread_pause = TRUE;

void esd_recovery_pause(BOOL en)
{
    esd_kthread_pause = en;
}

static int esd_recovery_kthread(void *data)
{
	//struct sched_param param = { .sched_priority = RTPM_PRIO_SCRN_UPDATE };
	//sched_setscheduler(current, SCHED_RR, &param);
    MTKFB_LOG("enter esd_recovery_kthread()\n");
    for( ;; ) {

        if (kthread_should_stop())
            break;

        MTKFB_LOG("sleep start in esd_recovery_kthread()\n");
        msleep(2000);       //2s
        MTKFB_LOG("sleep ends in esd_recovery_kthread()\n");

        if(!esd_kthread_pause)
        {
            if(is_early_suspended)
            {
                MTKFB_LOG("is_early_suspended in esd_recovery_kthread()\n");
                continue;
            }
            ///execute ESD check and recover flow
            MTKFB_LOG("DISP_EsdCheck starts\n");
            need_esd_check = 1;
            wait_event_interruptible(esd_check_wq, !need_esd_check);
            MTKFB_LOG("DISP_EsdCheck ends\n");
       }
    }


    MTKFB_LOG("exit esd_recovery_kthread()\n");
    return 0;
}


// return out = a - b
void timeval_sub(struct timeval *out,
                 const struct timeval *a,
                 const struct timeval *b)
{
    out->tv_sec  = a->tv_sec - b->tv_sec;
    out->tv_usec = a->tv_usec - b->tv_usec;

    if (out->tv_usec < 0) {
        -- out->tv_sec;
        out->tv_usec += 1000000;
    }
}

// return if a > b
static __inline BOOL timeval_larger(const struct timeval *a,
                                    const struct timeval *b)
{
    if (a->tv_sec > b->tv_sec) return TRUE;
    return (a->tv_usec > b->tv_usec) ? TRUE : FALSE;
}

// convert to 32KHz unit
static __inline time_t convert_to_32K_ticks(const struct timeval *x)
{
    return (x->tv_sec * 32768+ x->tv_usec * 32768/ 1000000);
}


/*
 * ---------------------------------------------------------------------------
 *  mtkfb_set_lcm_inited() will be called in mt6516_board_init()
 * ---------------------------------------------------------------------------
 */
static BOOL is_lcm_inited = FALSE;

void mtkfb_set_lcm_inited(BOOL inited)
{
    is_lcm_inited = inited;
}


/*
 * ---------------------------------------------------------------------------
 * called by M4U driver to init LCD port
 * ---------------------------------------------------------------------------
 */
unsigned int _m4u_lcd_init(void)
{
#if defined(MTK_M4U_SUPPORT)
    MTKFB_LOG("[FB driver] call _m4u_lcd_init()\n");
	DISP_AllocUILayerMva(fb_va_m4u, &fb_mva_m4u, fb_size_m4u);
	DISP_SetFrameBufferAddr(fb_mva_m4u);
	DISP_ConfigAssertLayerMva();
	DISP_InitM4U();
	mtkfb_enable_m4u = TRUE;
	MTKFB_LOG("[m4u_lcd_init] VA = 0x%x, MVA = 0x%x\n",fb_va_m4u, fb_mva_m4u);
#else
    MTKFB_LOG("[FB driver] call _m4u_lcd_init() but this function not implement for MTKFB not use M4U\n");
#endif
	return 0;
}
EXPORT_SYMBOL(_m4u_lcd_init);

void mtkfb_m4u_switch(bool enable)
{
#if defined(MTK_M4U_SUPPORT)
	MTKFB_FUNC();
	if (down_interruptible(&sem_flipping)) {
        MTKFB_LOG("[FB Driver] can't get semaphore in mtkfb_m4u_switch()\n");
        return;
    }
	sem_flipping_cnt--;
	if (down_interruptible(&sem_early_suspend)) {
        MTKFB_LOG("[FB Driver] can't get semaphore in mtkfb_m4u_switch()\n");
        return;
    }
	sem_early_suspend_cnt--;
    if (is_early_suspended) return;

	DISP_WaitForLCDNotBusy();
	DISP_M4U_On(enable);
	if(!enable){
		DISP_SetFrameBufferAddr(fb_va_m4u);
		mtkfb_enable_m4u = FALSE;
	}
	else{
		DISP_SetFrameBufferAddr(fb_mva_m4u);
		mtkfb_enable_m4u = TRUE;
	}
    sem_flipping_cnt++;
	sem_early_suspend_cnt++;
	up(&sem_early_suspend);
	up(&sem_flipping);
#else
    MTKFB_LOG("[FB driver] call mtkfb_m4u_switch() but this function not implement for MTKFB not use M4U\n");
#endif
	return;
}

void mtkfb_m4u_dump(void)
{
#if defined(MTK_M4U_SUPPORT)
	MTKFB_FUNC();

    if (is_early_suspended) return;

	DISP_DumpM4U();
#else
    MTKFB_LOG("[FB driver] call mtkfb_m4u_dump() but this function not implement for MTKFB not use M4U\n");
#endif
	return;
}


/*
 * ---------------------------------------------------------------------------
 * fbdev framework callbacks and the ioctl interface
 * ---------------------------------------------------------------------------
 */
/* Called each time the mtkfb device is opened */
static int mtkfb_open(struct file *file, struct fb_info *info, int user)
{
    NOT_REFERENCED(info);
    NOT_REFERENCED(user);

    MSG_FUNC_ENTER();
    MSG_FUNC_LEAVE();
    return 0;
}

#if defined (MTK_M4U_SUPPORT)
static int mtkfb_search_overlayList_ex(struct fb_overlay_buffer_list* head, void* file_addr)
{
    int ret = 0;
	struct fb_overlay_buffer_list *c, *n;
	c = head->next;
	n = head;
//	if(!c) return 0;
	while(c != NULL){
		if(c->file_addr == file_addr){

			DISP_WaitForLCDNotBusy();
        	DISP_DeallocMva(c->buffer.src_vir_addr, c->src_mva, c->buffer.size);
			{
				struct fb_overlay_buffer_list *a;
				a = overlay_buffer_head;
				MTKFB_LOG("[mtkfb_release] before delete a node,list node Addr:\n");

				while(a)
				{
					MTKFB_LOG("0x%x,  ", (unsigned int)a);
					a = a->next;
				}
			}
			n->next = c->next;
			vfree(c);
			{
				struct fb_overlay_buffer_list *b;
				b = overlay_buffer_head;
				MTKFB_LOG("\n[mtkfb_release] after delete a node,list node Addr:\n");
				while(b)
				{
					MTKFB_LOG("0x%x,  ", (unsigned int)b);
					b = b->next;
					MTKFB_LOG("\n");
				}
			}
			c = n->next;
			ret = 1;
		}
		else{
			n = c;
			c = c->next;
		}
	}
	return ret;
}
#endif

/* Called when the mtkfb device is closed. We make sure that any pending
 * gfx DMA operations are ended, before we return. */
static int mtkfb_release(struct file *file, struct fb_info *info, int user)
{
    NOT_REFERENCED(info);
    NOT_REFERENCED(user);

    MSG_FUNC_ENTER();

#if defined (MTK_M4U_SUPPORT)
	{
		int i;
        struct mtkfb_device *fbdev = (struct mtkfb_device *)info->par;
		if(mtkfb_search_overlayList_ex(overlay_buffer_head, (void*)file)){
			MTKFB_LOG("disable all video layer\n");
			for(i = 0;i < VIDEO_LAYER_COUNT+1;i++)
				LCD_LayerEnable(i,0);
		}
		fbdev->layer_enable = (1 << FB_LAYER);
	}
#endif

    MSG_FUNC_LEAVE();
    return 0;
}

/* Store a single color palette entry into a pseudo palette or the hardware
 * palette if one is available. For now we support only 16bpp and thus store
 * the entry only to the pseudo palette.
 */
static int mtkfb_setcolreg(u_int regno, u_int red, u_int green,
                           u_int blue, u_int transp,
                           struct fb_info *info)
{
    int r = 0;
    unsigned bpp, m;

    NOT_REFERENCED(transp);

    MSG_FUNC_ENTER();

    bpp = info->var.bits_per_pixel;
    m = 1 << bpp;
    if (regno >= m)
    {
        r = -EINVAL;
        goto exit;
    }

    switch (bpp)
    {
    case 16:
        /* RGB 565 */
        ((u32 *)(info->pseudo_palette))[regno] =
            ((red & 0xF800) |
            ((green & 0xFC00) >> 5) |
            ((blue & 0xF800) >> 11));
        break;
    case 32:
        /* ARGB8888 */
        ((u32 *)(info->pseudo_palette))[regno] =
             (0xff000000)           |
            ((red   & 0xFF00) << 8) |
            ((green & 0xFF00)     ) |
            ((blue  & 0xFF00) >> 8);
        break;

    // TODO: RGB888, BGR888, ABGR8888

    default:
        ASSERT(0);
    }

exit:
    MSG_FUNC_LEAVE();
    return r;
}

#if defined(MTK_HDMI_SUPPORT) ||defined(MTK_WFD_SUPPORT)
typedef void (*HDMI_POWER_ON)(bool);
HDMI_POWER_ON hdmi_power_on_callback = NULL;
void mtkfb_set_hdmi_power_on_callback(unsigned int cb)
{
	hdmi_power_on_callback = (HDMI_POWER_ON)cb;
}
EXPORT_SYMBOL(mtkfb_set_hdmi_power_on_callback);


typedef void (*HDMI_SET_ORIENTATION)(int);
HDMI_SET_ORIENTATION hdmi_set_orientation_callback = NULL;
void mtkfb_set_hdmi_set_orientation_callback(unsigned int cb)
{
	hdmi_set_orientation_callback = (HDMI_SET_ORIENTATION)cb;
}
EXPORT_SYMBOL(mtkfb_set_hdmi_set_orientation_callback);

typedef bool (*IS_HDMI_ENABLE_CB)(void);
IS_HDMI_ENABLE_CB is_hdmi_enable_callback = NULL;
void mtkfb_set_is_hdmi_enable_callback(unsigned int cb)
{
	is_hdmi_enable_callback = (IS_HDMI_ENABLE_CB)cb;
}
EXPORT_SYMBOL(mtkfb_set_is_hdmi_enable_callback);

typedef void (*HDMI_UPDATE_CB)(void);
HDMI_UPDATE_CB hdmi_update_callback = NULL;
void mtkfb_set_hdmi_update_callback(unsigned int cb)
{
	hdmi_update_callback = (HDMI_UPDATE_CB)cb;
}
EXPORT_SYMBOL(mtkfb_set_hdmi_update_callback);

#endif

#if 0
static long int get_current_time_us(void)
{
	struct timeval t;
	do_gettimeofday(&t);
	return (t.tv_sec & 0xFFF) * 1000000 + t.tv_usec;
}
#endif

void dsi_enable_dc2lcd(void);
void dsi_disable_dc2lcd(void);

#if defined(MTK_M4U_SUPPORT)
static BOOL MTKFB_SearchMVA(UINT32 mva)
{
	BOOL ret = TRUE;
	struct fb_overlay_buffer_list *c;
	c = overlay_buffer_head->next;
	if(c == NULL){
		MTKFB_LOG("[LCD Warning] mva = 0x%x, buffer list is NULL\n",mva);
		return FALSE;
	}
	while(mva != c->src_mva){
		c = c->next;
		if(!c){
			MTKFB_LOG("[LCD Warning] mva = 0x%x,Can't find matched VA in buffer list\n",mva);
			ret = FALSE;
			break;
		}
	}
    return ret;

}
#endif

static void mtkfb_update_screen_impl(void)
{
	BOOL down_sem = FALSE;
	MTKFB_FUNC();
	MMProfileLog(MTKFB_MMP_Events.UpdateScreenImpl, MMProfileFlagStart);
	if (down_interruptible(&sem_overlay_buffer)) {
       	printk("[FB Driver] can't get semaphore in mtkfb_update_screen_impl()\n");
    }
	else{
		down_sem = TRUE;
		sem_overlay_buffer_cnt--;
	}

#if defined(MTK_M4U_SUPPORT)
    {
        unsigned int i;
        /// check if the MVA address is invalid, turn off the layer
    	for(i=0;i<FB_LAYER;i++){
    		if(LCD_IsLayerEnable((LCD_LAYER_ID)i)){
    			if(!MTKFB_SearchMVA(LCD_LayerGetAddress(i))){
    				LCD_LayerEnable((LCD_LAYER_ID)i, FALSE);
    			}
    		}
    	}
    }
#endif

	DISP_CHECK_RET(DISP_UpdateScreen(0, 0, fb_xres_update, fb_yres_update));

    if(down_sem){
		sem_overlay_buffer_cnt++;
		up(&sem_overlay_buffer);
	}
	MMProfileLog(MTKFB_MMP_Events.UpdateScreenImpl, MMProfileFlagEnd);
}



int external_display_trigger_update(void)
{
	mtkfb_update_screen_impl();
	return 0;
}

static int mtkfb_update_screen(struct fb_info *info)
{
	MTKFB_FUNC();
    if (down_interruptible(&sem_early_suspend)) {
        printk("[FB Driver] can't get semaphore in mtkfb_update_screen()\n");
        return -ERESTARTSYS;
    }
	sem_early_suspend_cnt--;
    if (is_early_suspended) goto End;
#ifndef MT65XX_NEW_DISP
	if(DISP_IsInOverlayMode())
	{
		if(DISP_IsLCDBusy())
		{
			atomic_set(&has_pending_update,1);
			goto End;
		}
		else
		{
			mtkfb_update_screen_impl();
		}
	}
	else
#endif
	{
		mtkfb_update_screen_impl();
	}

End:
	sem_early_suspend_cnt++;
    up(&sem_early_suspend);
    return 0;
}
static unsigned int BL_level = 0;
static BOOL BL_set_level_resume = FALSE;
int mtkfb_set_backlight_level(unsigned int level)
{
	MTKFB_FUNC();
	printk("mtkfb_set_backlight_level:%d\n", level);
	if (down_interruptible(&sem_flipping)) {
        printk("[FB Driver] can't get semaphore:%d\n", __LINE__);
        return -ERESTARTSYS;
    }
	sem_flipping_cnt--;
	if (down_interruptible(&sem_early_suspend)) {
        printk("[FB Driver] can't get semaphore:%d\n", __LINE__);
		sem_flipping_cnt++;
		up(&sem_flipping);
        return -ERESTARTSYS;
    }

	sem_early_suspend_cnt--;
    if (is_early_suspended){
		BL_level = level;
		BL_set_level_resume = TRUE;
		printk("[FB driver] set backlight level but FB has been suspended\n");
		goto End;
    	}
	DISP_SetBacklight(level);
	BL_set_level_resume = FALSE;
End:
	sem_flipping_cnt++;
	sem_early_suspend_cnt++;
    up(&sem_early_suspend);
	up(&sem_flipping);
    return 0;
}
EXPORT_SYMBOL(mtkfb_set_backlight_level);

int mtkfb_set_backlight_mode(unsigned int mode)
{
	MTKFB_FUNC();
	if (down_interruptible(&sem_flipping)) {
        printk("[FB Driver] can't get semaphore:%d\n", __LINE__);
        return -ERESTARTSYS;
    }
	sem_flipping_cnt--;
	if (down_interruptible(&sem_early_suspend)) {
        printk("[FB Driver] can't get semaphore:%d\n", __LINE__);
		sem_flipping_cnt++;
		up(&sem_flipping);
        return -ERESTARTSYS;
    }

	sem_early_suspend_cnt--;
    if (is_early_suspended) goto End;

	DISP_SetBacklight_mode(mode);
End:
	sem_flipping_cnt++;
	sem_early_suspend_cnt++;
    up(&sem_early_suspend);
	up(&sem_flipping);
    return 0;
}
EXPORT_SYMBOL(mtkfb_set_backlight_mode);


int mtkfb_set_backlight_pwm(int div)
{
	MTKFB_FUNC();
	if (down_interruptible(&sem_flipping)) {
        printk("[FB Driver] can't get semaphore:%d\n", __LINE__);
        return -ERESTARTSYS;
    }
	sem_flipping_cnt--;
	if (down_interruptible(&sem_early_suspend)) {
        printk("[FB Driver] can't get semaphore:%d\n", __LINE__);
		sem_flipping_cnt++;
		up(&sem_flipping);
        return -ERESTARTSYS;
    }
	sem_early_suspend_cnt--;
    if (is_early_suspended) goto End;
	DISP_SetPWM(div);
End:
	sem_flipping_cnt++;
	sem_early_suspend_cnt++;
    up(&sem_early_suspend);
	up(&sem_flipping);
    return 0;
}
EXPORT_SYMBOL(mtkfb_set_backlight_pwm);

int mtkfb_get_backlight_pwm(int div, unsigned int *freq)
{
	DISP_GetPWM(div, freq);
    return 0;
}
EXPORT_SYMBOL(mtkfb_get_backlight_pwm);

void mtkfb_waitVsync(void)
{
	if(is_early_suspended){
		printk("[MTKFB_VSYNC]:mtkfb has suspend, return directly\n");
		msleep(20);
		return;
	}
	vsync_cnt++;
	DISP_WaitVSYNC();
	vsync_cnt--;
	return;
}
EXPORT_SYMBOL(mtkfb_waitVsync);
/* Used for HQA test */
/*-------------------------------------------------------------
   Note: The using scenario must be
         1. switch normal mode to factory mode when LCD screen is on
         2. switch factory mode to normal mode(optional)
-------------------------------------------------------------*/
static struct fb_var_screeninfo    fbi_var_backup;
static struct fb_fix_screeninfo    fbi_fix_backup;
static BOOL                         need_restore = FALSE;
static int mtkfb_set_par(struct fb_info *fbi);
void mtkfb_switch_normal_to_factory(void)
{
    if (down_interruptible(&sem_flipping)) {
        printk("[FB Driver] can't get semaphore:%d\n", __LINE__);
        return;
    }
	sem_flipping_cnt--;
	if (down_interruptible(&sem_early_suspend)) {
        printk("[FB Driver] can't get semaphore:%d\n", __LINE__);
		sem_flipping_cnt++;
        up(&sem_flipping);
        return;
    }
	sem_early_suspend_cnt--;
    if (is_early_suspended) {
        goto EXIT;
    }

    if (mtkfb_fbi)
    {
        memcpy(&fbi_var_backup, &mtkfb_fbi->var, sizeof(fbi_var_backup));
        memcpy(&fbi_fix_backup, &mtkfb_fbi->fix, sizeof(fbi_fix_backup));
        need_restore = TRUE;
    }

EXIT:
	sem_early_suspend_cnt++;
	sem_flipping_cnt++;
    up(&sem_early_suspend);
    up(&sem_flipping);
}

/* Used for HQA test */
void mtkfb_switch_factory_to_normal(void)
{
    BOOL need_set_par = FALSE;
    if (down_interruptible(&sem_flipping)) {
        printk("[FB Driver] can't get semaphore in mtkfb_switch_factory_to_normal()\n");
        return;
    }
	sem_flipping_cnt--;
	if (down_interruptible(&sem_early_suspend)) {
        printk("[FB Driver] can't get semaphore in mtkfb_switch_factory_to_normal()\n");
		sem_flipping_cnt++;
        up(&sem_flipping);
        return;
    }

	sem_early_suspend_cnt--;
    if (is_early_suspended) {
        goto EXIT;
    }

    if ((mtkfb_fbi) && (need_restore))
    {
        memcpy(&mtkfb_fbi->var, &fbi_var_backup, sizeof(fbi_var_backup));
        memcpy(&mtkfb_fbi->fix, &fbi_fix_backup, sizeof(fbi_fix_backup));
        need_restore = FALSE;
        need_set_par = TRUE;
    }

EXIT:
	sem_early_suspend_cnt++;
	sem_flipping_cnt++;
    up(&sem_early_suspend);
    up(&sem_flipping);
    if (need_set_par)
    {
        int ret;
        ret = mtkfb_set_par(mtkfb_fbi);
		if (ret != 0)
			PRNERR("failed to mtkfb_set_par\n");
    }
}

static LCD_LAYER_FORMAT mtkfb_var_get_format(struct fb_var_screeninfo *var)
{
	u32 bpp = var->bits_per_pixel;

	switch(bpp)
	{
	case 16 :
		return LCD_LAYER_FORMAT_RGB565;
	case 24 :
		return LCD_LAYER_FORMAT_RGB888;
	case 32 :
		return LCD_LAYER_FORMAT_ARGB8888;
	default :
		return LCD_LAYER_FORMAT_NUM;
	}

}

static bool first_update = false;
static bool first_enable_esd = true;
static int cnt=3;
static int mtkfb_pan_display_impl(struct fb_var_screeninfo *var, struct fb_info *info)
{
    UINT32 offset;
    UINT32 paStart;
    char *vaStart, *vaEnd;
    int ret = 0;

	if(first_update){
		first_update = false;
		return ret;
	}
	MMProfileLogStructure(MTKFB_MMP_Events.PanDisplay, MMProfileFlagStart, var, struct fb_var_screeninfo);
	if(0!=cnt){
		printk("LCD:%dx%d\n",MTK_FB_XRES,MTK_FB_YRES);
		cnt--;
	}
	MTKFB_FUNC();

    MSG_FUNC_ENTER();

    MSG(ARGU, "xoffset=%u, yoffset=%u, xres=%u, yres=%u, xresv=%u, yresv=%u\n",
        var->xoffset, var->yoffset,
        info->var.xres, info->var.yres,
        info->var.xres_virtual,
        info->var.yres_virtual);

    if (down_interruptible(&sem_flipping)) {
        printk("[FB Driver] can't get semaphore in mtkfb_pan_display_impl()\n");
        MMProfileLogMetaString(MTKFB_MMP_Events.PanDisplay, MMProfileFlagEnd, "Can't get semaphore in mtkfb_pan_display_impl()");
        return -ERESTARTSYS;
    }
	sem_flipping_cnt--;

    info->var.yoffset = var->yoffset;

    offset = var->yoffset * info->fix.line_length;
    paStart = fb_pa + offset;
    vaStart = info->screen_base + offset;
    vaEnd   = vaStart + info->var.yres * info->fix.line_length;
//    LCD_WaitForNotBusy();

    mutex_lock(&OverlaySettingMutex);
    DISP_CHECK_RET(DISP_SetFrameBufferAddr(paStart));
    cached_layer_config[FB_LAYER].vaddr = vaStart;
    LCD_LayerEnable(FB_LAYER, 1);
    cached_layer_config[FB_LAYER].src_x = 0;
    cached_layer_config[FB_LAYER].src_y = 0;
    cached_layer_config[FB_LAYER].src_w = var->xres;
    cached_layer_config[FB_LAYER].src_h = var->yres;
    cached_layer_config[FB_LAYER].dst_x = 0;
    cached_layer_config[FB_LAYER].dst_y = 0;
    cached_layer_config[FB_LAYER].dst_w = var->xres;
    cached_layer_config[FB_LAYER].dst_h = var->yres;
    {
        LCD_LAYER_FORMAT eFormat;
        unsigned int layerpitch;
        unsigned int src_pitch = ALIGN_TO(var->xres, 32);
        switch(var->bits_per_pixel)
        {
        case 16:
            eFormat = LCD_LAYER_FORMAT_RGB565;
            layerpitch = 2;
            LCD_LayerSetAlphaBlending(FB_LAYER, FALSE, 0xFF);
            break;
        case 24:
            eFormat = LCD_LAYER_FORMAT_RGB888;
            layerpitch = 3;
            LCD_LayerSetAlphaBlending(FB_LAYER, FALSE, 0xFF);
            break;
        case 32:
            eFormat = LCD_LAYER_FORMAT_PARGB8888;
            layerpitch = 4;
            LCD_LayerSetAlphaBlending(FB_LAYER, TRUE, 0xFF);
            break;
        default:
            PRNERR("Invalid color format bpp: 0x%d\n", var->bits_per_pixel);
            return -1;
        }
        LCD_LayerSetFormat(FB_LAYER, eFormat);
        LCD_LayerSetNextBuffIdx(FB_LAYER, -1);
        LCD_LayerSetPitch(FB_LAYER, src_pitch * layerpitch);
    }

    atomic_set(&OverlaySettingDirtyFlag, 1);
    atomic_set(&OverlaySettingApplied, 0);
    PanDispSettingPending = 1;
	PanDispSettingDirty = 1;
	PanDispSettingApplied = 0;
	is_ipoh_bootup = false;
    mutex_unlock(&OverlaySettingMutex);
#if defined(MTK_LCD_HW_3D_SUPPORT)
	//if (mtkfb_current_layer_type != mtkfb_using_layer_type)
	{
		struct fb_overlay_layer fb_layer;

		if (mtkfb_current_layer_type != mtkfb_using_layer_type)
			MTKFB_LOG("[FB Driver] layer type change %d -> %d() \n", mtkfb_using_layer_type, mtkfb_current_layer_type);
		mtkfb_using_layer_type = mtkfb_current_layer_type;

		memcpy(&fb_layer, &fb_layer_context, sizeof(fb_layer_context));

		fb_layer.src_phy_addr = (void*)paStart;

		if (mtkfb_using_layer_type == LAYER_2D)
		{
			LCD_CHECK_RET(LCD_LayerSet3D(FB_LAYER, 0, 0, 0));
			LCD_CHECK_RET(LCD_LayerSet3D((FB_LAYER+1), 0, 0, 0));

			if (!dal_shown)
				LCD_CHECK_RET(LCD_LayerEnable((FB_LAYER+1), 0));
		}
		else
		{
			bool r_1st = 0, landscape = 0;
			UINT32 offset_x, offset_y;

			LCD_CHECK_RET(LCD_Layer3D_Prepare(FB_LAYER));

			switch (mtkfb_using_layer_type)
			{
				case LAYER_3D_SBS_0 :
					r_1st=0; landscape=0;
					offset_x=fb_layer_context.src_width/2;
					offset_y=0;
					break;
				case LAYER_3D_SBS_90 :
					r_1st=0; landscape=1;
					offset_x=0;
					offset_y=fb_layer_context.src_height/2;
					break;
				case LAYER_3D_SBS_180 :
					r_1st=1; landscape=0;
					offset_x=fb_layer_context.src_width/2;
					offset_y=0;
					break;
				case LAYER_3D_SBS_270 :
					r_1st=1; landscape=1;
					offset_x=0;
					offset_y=fb_layer_context.src_height/2;
					 break;
				default :
					break;
			}

			LCD_CHECK_RET(LCD_LayerSet3D(FB_LAYER, 1, r_1st, landscape));
			LCD_CHECK_RET(LCD_LayerSet3D((FB_LAYER+1), 1, r_1st, landscape));
			if(hwc_force_fb_enabled)
				LCD_CHECK_RET(LCD_LayerEnable((FB_LAYER+1), 1));
			else
				LCD_CHECK_RET(LCD_LayerEnable((FB_LAYER+1), 0));

			LCD_CHECK_RET(LCD_LayerSetWindowOffset((FB_LAYER+1), offset_x, offset_y));

		}
	}
#endif

// Instead by LCD_LayerSetWindowOffset (source croping)
/*
	if (mtkfb_using_layer_type != LAYER_2D)
	{
//		unsigned int l_layer_addr = paStart;
		unsigned int r_layer_addr = paStart;

		//printk("[FB Driver] layer_type = %d, tgt_width = %d \n", fb_layer_context.layer_type, fb_layer_context.tgt_width);

		switch (mtkfb_using_layer_type)
		{
			case LAYER_3D_SBS_0 :
			case LAYER_3D_SBS_180 :
				//r_layer_addr += (layerInfo->src_pitch * layerpitch / 2); break;
				r_layer_addr += (fb_layer_context.tgt_width * 4 / 2); break;
			case LAYER_3D_SBS_90 :
			case LAYER_3D_SBS_270 :
				r_layer_addr += (fb_layer_context.src_pitch * fb_layer_context.tgt_height * 4 / 2); break;
			default :
				break;
		}

		{
			//printk("[FB Driver] layer %d LCD_LayerSetAddress %p %p \n", FB_LAYER + 1, l_layer_addr, r_layer_addr);
			//LCD_CHECK_RET(LCD_LayerSetAddress(FB_LAYER + 1, r_layer_addr));
		}
	}
*/

	//if(DISP_IsInOverlayMode()){
 //       DISP_WaitForLCDNotBusy();
	//}
	ret = mtkfb_update_screen(info);
    wait_event_interruptible_timeout(reg_update_wq, PanDispSettingApplied, HZ/10);
	sem_flipping_cnt++;
    up(&sem_flipping);
    if(first_enable_esd)
    {
		esd_recovery_pause(FALSE);
		first_enable_esd = false;
	}
    MMProfileLog(MTKFB_MMP_Events.PanDisplay, MMProfileFlagEnd);

    return ret;
}


static int mtkfb_pan_display_proxy(struct fb_var_screeninfo *var, struct fb_info *info)
{
#ifdef CONFIG_MTPROF_APPLAUNCH  // eng enable, user disable
	LOG_PRINT(ANDROID_LOG_INFO, "AppLaunch", "mtkfb_pan_display_proxy.\n");
#endif
    return mtkfb_pan_display_impl(var, info);
}


/* Set fb_info.fix fields and also updates fbdev.
 * When calling this fb_info.var must be set up already.
 */
static void set_fb_fix(struct mtkfb_device *fbdev)
{
    struct fb_info           *fbi   = fbdev->fb_info;
    struct fb_fix_screeninfo *fix   = &fbi->fix;
    struct fb_var_screeninfo *var   = &fbi->var;
    struct fb_ops            *fbops = fbi->fbops;

    strncpy(fix->id, MTKFB_DRIVER, sizeof(fix->id));
    fix->type = FB_TYPE_PACKED_PIXELS;

    switch (var->bits_per_pixel)
    {
    case 16:
    case 24:
    case 32:
        fix->visual = FB_VISUAL_TRUECOLOR;
        break;
    case 1:
    case 2:
    case 4:
    case 8:
        fix->visual = FB_VISUAL_PSEUDOCOLOR;
        break;
    default:
        ASSERT(0);
    }

    fix->accel       = FB_ACCEL_NONE;
    fix->line_length = ALIGN_TO(var->xres_virtual, 32) * var->bits_per_pixel / 8;
    fix->smem_len    = fbdev->fb_size_in_byte;
    fix->smem_start  = fbdev->fb_pa_base;

    fix->xpanstep = 0;
    fix->ypanstep = 1;

    fbops->fb_fillrect  = cfb_fillrect;
    fbops->fb_copyarea  = cfb_copyarea;
    fbops->fb_imageblit = cfb_imageblit;
}


/* Check values in var, try to adjust them in case of out of bound values if
 * possible, or return error.
 */
static int mtkfb_check_var(struct fb_var_screeninfo *var, struct fb_info *fbi)
{
    unsigned int bpp;
    unsigned long max_frame_size;
    unsigned long line_size;

    struct mtkfb_device *fbdev = (struct mtkfb_device *)fbi->par;

    MSG_FUNC_ENTER();

    MSG(ARGU, "xres=%u, yres=%u, xres_virtual=%u, yres_virtual=%u, "
              "xoffset=%u, yoffset=%u, bits_per_pixel=%u)\n",
        var->xres, var->yres, var->xres_virtual, var->yres_virtual,
        var->xoffset, var->yoffset, var->bits_per_pixel);

    bpp = var->bits_per_pixel;

    if (bpp != 16 && bpp != 24 && bpp != 32) {
        MTKFB_LOG("[%s]unsupported bpp: %d", __func__, bpp);
        return -1;
    }

    switch (var->rotate) {
    case 0:
    case 180:
        var->xres = MTK_FB_XRES;
        var->yres = MTK_FB_YRES;
        break;
    case 90:
    case 270:
        var->xres = MTK_FB_YRES;
        var->yres = MTK_FB_XRES;
        break;
    default:
        return -1;
    }

    if (var->xres_virtual < var->xres)
        var->xres_virtual = var->xres;
    if (var->yres_virtual < var->yres)
        var->yres_virtual = var->yres;

    max_frame_size = fbdev->fb_size_in_byte;
    line_size = var->xres_virtual * bpp / 8;

    if (line_size * var->yres_virtual > max_frame_size) {
        /* Try to keep yres_virtual first */
        line_size = max_frame_size / var->yres_virtual;
        var->xres_virtual = line_size * 8 / bpp;
        if (var->xres_virtual < var->xres) {
            /* Still doesn't fit. Shrink yres_virtual too */
            var->xres_virtual = var->xres;
            line_size = var->xres * bpp / 8;
            var->yres_virtual = max_frame_size / line_size;
        }
    }
    if (var->xres + var->xoffset > var->xres_virtual)
        var->xoffset = var->xres_virtual - var->xres;
    if (var->yres + var->yoffset > var->yres_virtual)
        var->yoffset = var->yres_virtual - var->yres;

    if (16 == bpp) {
        var->red.offset    = 11;  var->red.length    = 5;
        var->green.offset  =  5;  var->green.length  = 6;
        var->blue.offset   =  0;  var->blue.length   = 5;
        var->transp.offset =  0;  var->transp.length = 0;
    }
    else if (24 == bpp)
    {
        var->red.length = var->green.length = var->blue.length = 8;
        var->transp.length = 0;

        // Check if format is RGB565 or BGR565

        ASSERT(8 == var->green.offset);
        ASSERT(16 == var->red.offset + var->blue.offset);
        ASSERT(16 == var->red.offset || 0 == var->red.offset);
    }
    else if (32 == bpp)
    {
        var->red.length = var->green.length =
        var->blue.length = var->transp.length = 8;

        // Check if format is ARGB565 or ABGR565

        ASSERT(8 == var->green.offset && 24 == var->transp.offset);
        ASSERT(16 == var->red.offset + var->blue.offset);
        ASSERT(16 == var->red.offset || 0 == var->red.offset);
    }

    var->red.msb_right = var->green.msb_right =
    var->blue.msb_right = var->transp.msb_right = 0;

    var->activate = FB_ACTIVATE_NOW;

    var->height    = UINT_MAX;
    var->width     = UINT_MAX;
    var->grayscale = 0;
    var->nonstd    = 0;

    var->pixclock     = UINT_MAX;
    var->left_margin  = UINT_MAX;
    var->right_margin = UINT_MAX;
    var->upper_margin = UINT_MAX;
    var->lower_margin = UINT_MAX;
    var->hsync_len    = UINT_MAX;
    var->vsync_len    = UINT_MAX;

    var->vmode = FB_VMODE_NONINTERLACED;
    var->sync  = 0;

    MSG_FUNC_LEAVE();
    return 0;
}


/* Switch to a new mode. The parameters for it has been check already by
 * mtkfb_check_var.
 */
static int mtkfb_set_par(struct fb_info *fbi)
{
    struct fb_var_screeninfo *var = &fbi->var;
    struct mtkfb_device *fbdev = (struct mtkfb_device *)fbi->par;
    struct fb_overlay_layer fb_layer;
    u32 bpp = var->bits_per_pixel;

    MSG_FUNC_ENTER();
    memset(&fb_layer, 0, sizeof(struct fb_overlay_layer));
    switch(bpp)
    {
    case 16 :
        fb_layer.src_fmt = MTK_FB_FORMAT_RGB565;
        fb_layer.src_use_color_key = 1;
#ifdef MT65XX_NEW_DISP
    	fb_layer.src_color_key = 0xFF000000;
#else
    	fb_layer.src_color_key = 0;
#endif
        break;

    case 24 :
        fb_layer.src_use_color_key = 1;
        fb_layer.src_fmt = (0 == var->blue.offset) ?
                           MTK_FB_FORMAT_RGB888 :
                           MTK_FB_FORMAT_BGR888;
#ifdef MT65XX_NEW_DISP
    	fb_layer.src_color_key = 0xFF000000;
#else
    	fb_layer.src_color_key = 0;
#endif
        break;

    case 32 :
        fb_layer.src_use_color_key = 0;
        fb_layer.src_fmt = (0 == var->blue.offset) ?
                           MTK_FB_FORMAT_ARGB8888 :
                           MTK_FB_FORMAT_ABGR8888;
    	fb_layer.src_color_key = 0;
        break;

    default :
        fb_layer.src_fmt = MTK_FB_FORMAT_UNKNOWN;
        MTKFB_LOG("[%s]unsupported bpp: %d", __func__, bpp);
        return -1;
    }

    // If the framebuffer format is NOT changed, nothing to do
    //
    if (fb_layer.src_fmt == fbdev->layer_format[FB_LAYER]) {
        goto Done;
    }

    // else, begin change display mode
    //
    set_fb_fix(fbdev);

    fb_layer.layer_id = FB_LAYER;
    fb_layer.layer_enable = 1;
    fb_layer.src_base_addr = (void *)((unsigned long)fbdev->fb_va_base + var->yoffset * fbi->fix.line_length);
    fb_layer.src_phy_addr = (void *)(fb_pa + var->yoffset * fbi->fix.line_length);
    fb_layer.src_direct_link = 0;
    fb_layer.src_offset_x = fb_layer.src_offset_y = 0;
//    fb_layer.src_width = fb_layer.tgt_width = fb_layer.src_pitch = var->xres;
#if defined(HWGPU_SUPPORT)
    fb_layer.src_pitch = ALIGN_TO(var->xres, 32);
#else
		if(get_boot_mode() == META_BOOT || get_boot_mode() == FACTORY_BOOT
		|| get_boot_mode() == ADVMETA_BOOT || get_boot_mode() == RECOVERY_BOOT)
			fb_layer.src_pitch = ALIGN_TO(var->xres, 32);
		else
			fb_layer.src_pitch = var->xres;
#endif
    fb_layer.src_width = fb_layer.tgt_width = var->xres;
    fb_layer.src_height = fb_layer.tgt_height = var->yres;
    fb_layer.tgt_offset_x = fb_layer.tgt_offset_y = 0;

//    fb_layer.src_color_key = 0;
    fb_layer.layer_rotation = MTK_FB_ORIENTATION_0;
	fb_layer.layer_type = LAYER_2D;

// xuecheng.zhang
// for landsacpe mode, we need to rotate the framebuffer layer
// because there is not any middleware could be used for the UI rotation.
//#ifdef MTK_QVGA_LANDSCAPE_SUPPORT
#if 0
	if((get_boot_mode() == FACTORY_BOOT || get_boot_mode() == RECOVERY_BOOT))
	{
		if(0 == strncmp(MTK_LCM_PHYSICAL_ROTATION, "90", 2))
		{
			fb_layer.layer_rotation = MTK_FB_ORIENTATION_270;
		}
		else if(0 == strncmp(MTK_LCM_PHYSICAL_ROTATION, "270", 3)){
			fb_layer.layer_rotation = MTK_FB_ORIENTATION_90;
		}
		else
		{
		}
	}
#endif
//#endif

    mutex_lock(&OverlaySettingMutex);
    mtkfb_set_overlay_layer(fbi, &fb_layer);
    atomic_set(&OverlaySettingDirtyFlag, 1);
    atomic_set(&OverlaySettingApplied, 0);
    mutex_unlock(&OverlaySettingMutex);

	// backup fb_layer information.
	memcpy(&fb_layer_context, &fb_layer, sizeof(fb_layer));

    //memset(fbi->screen_base, 0x0, fbi->screen_size);  // clear the whole VRAM as zero
Done:
    MSG_FUNC_LEAVE();
    return 0;
}


static int mtkfb_soft_cursor(struct fb_info *info, struct fb_cursor *cursor)
{
    NOT_REFERENCED(info);
    NOT_REFERENCED(cursor);

    return 0;
}

#if defined (MTK_M4U_SUPPORT)
static struct fb_overlay_buffer_list* mtkfb_search_overlayList(struct fb_overlay_buffer_list* head, unsigned int va)
{
	struct fb_overlay_buffer_list *c;
	c = head->next;
	if(c == NULL) return c;
	while(va != c->buffer.src_vir_addr || c->pid != current->tgid){
		c = c->next;
		if(!c){
			struct fb_overlay_buffer_list *p;
			p = head->next;
//			ASSERT(p);
			MTKFB_LOG("[FB driver] current va = 0x%x, buffer_list va:\n",va);
			while(p){
				MTKFB_LOG("0x%x\n", p->buffer.src_vir_addr);
				p = p->next;
			}
			MTKFB_LOG("[FB driver] ERROR: Can't find matched VA in buffer list\n");
			break;
		}
	}
    return c;
}
#else
static unsigned int mtkfb_user_v2p(unsigned int va)
{
    unsigned int pageOffset = (va & (PAGE_SIZE - 1));
    pgd_t *pgd;
    pmd_t *pmd;
    pte_t *pte;
    unsigned int pa;

    pgd = pgd_offset(current->mm, va); /* what is tsk->mm */
    pmd = pmd_offset(pgd, va);
    pte = pte_offset_map(pmd, va);

    pa = (pte_val(*pte) & (PAGE_MASK)) | pageOffset;


    return pa;
}
#endif

static int mtkfb_set_overlay_layer(struct fb_info *info, struct fb_overlay_layer* layerInfo)
{
    struct mtkfb_device *fbdev = (struct mtkfb_device *)info->par;

    unsigned int layerpitch;
	unsigned int layerbpp;
    unsigned int u4OvlPhyAddr, layer_addr;
	LCD_LAYER_FORMAT eFormat;
    unsigned int id = layerInfo->layer_id;
    int enable = layerInfo->layer_enable ? 1 : 0;
    int ret = 0;
	bool r_1st = 0, landscape = 0;

	MTKFB_FUNC();
    MSG_FUNC_ENTER();
    MMProfileLogEx(MTKFB_MMP_Events.SetOverlayLayer, MMProfileFlagStart, (id<<16)|enable, (unsigned int)layerInfo->src_phy_addr);

    /** LCD registers can't be R/W when its clock is gated in early suspend
        mode; power on/off LCD to modify register values before/after func.
    */
    if (is_early_suspended) {
        DISP_CHECK_RET(DISP_LCDPowerEnable(TRUE));
    }
	//BUG: check layer 3 format
	if((layerInfo->layer_id == 3) && (layerInfo->src_fmt != MTK_FB_FORMAT_ARGB8888))
	{
		printk("ERROR!!!Layer 3 format error!!!\n");
	}

    MTKFB_LOG("[FB Driver] mtkfb_set_overlay_layer():layer id = %u, layer en = %u, src format = %u, direct link: %u, src vir addr = %u, src phy addr = %u, src pitch=%u, src xoff=%u, src yoff=%u, src w=%u, src h=%u\n",
        layerInfo->layer_id,
        layerInfo->layer_enable,
        layerInfo->src_fmt,
        (unsigned int)(layerInfo->src_direct_link),
        (unsigned int)(layerInfo->src_base_addr),
        (unsigned int)(layerInfo->src_phy_addr),
        layerInfo->src_pitch,
        layerInfo->src_offset_x,
        layerInfo->src_offset_y,
        layerInfo->src_width,
        layerInfo->src_height);
    MTKFB_LOG("[FB Driver] mtkfb_set_overlay_layer():target xoff=%u, target yoff=%u, target w=%u, target h=%u\n",
        layerInfo->tgt_offset_x,
        layerInfo->tgt_offset_y,
        layerInfo->tgt_width,
        layerInfo->tgt_height);

	//LCD_WaitForNotBusy();

    // Update Layer Enable Bits and Layer Config Dirty Bits

    if ((((fbdev->layer_enable >> id) & 1) ^ enable)) {
        fbdev->layer_enable ^= (1 << id);
        fbdev->layer_config_dirty |= MTKFB_LAYER_ENABLE_DIRTY;
    }

    // Update Layer Format and Layer Config Dirty Bits

    if (fbdev->layer_format[id] != layerInfo->src_fmt) {
        fbdev->layer_format[id] = layerInfo->src_fmt;
        fbdev->layer_config_dirty |= MTKFB_LAYER_FORMAT_DIRTY;
    }

    // Enter Overlay Mode if any layer is enabled except the FB layer

    if(fbdev->layer_enable & ((1 << VIDEO_LAYER_COUNT)-1)){
        if (DISP_STATUS_OK == DISP_EnterOverlayMode()) {
            MTKFB_LOG("mtkfb_ioctl(MTKFB_ENABLE_OVERLAY)\n");
        }
    }

    if (!enable || !layerInfo->src_direct_link) {
        DISP_DisableDirectLinkMode(id);
    }

    if (!enable) {
	LCD_CHECK_RET(LCD_LayerEnable(id, enable));
        ret = 0;
        goto LeaveOverlayMode;
    }

    if (layerInfo->src_direct_link) {
        DISP_EnableDirectLinkMode(id);
    }

	switch (layerInfo->src_fmt)
    {
    case MTK_FB_FORMAT_YUV422:
        cached_layer_config[id].fmt = OVL_INPUT_FORMAT_YUYV;
        eFormat = LCD_LAYER_FORMAT_YUYV422;
        layerpitch = 2;
		layerbpp = 24;
        break;

    case MTK_FB_FORMAT_RGB565:
        cached_layer_config[id].fmt = OVL_INPUT_FORMAT_RGB565;
        eFormat = LCD_LAYER_FORMAT_RGB565;
        layerpitch = 2;
		layerbpp = 16;
        break;

    case MTK_FB_FORMAT_RGB888:
        cached_layer_config[id].fmt = OVL_INPUT_FORMAT_RGB888;
        eFormat = LCD_LAYER_FORMAT_RGB888;
        layerpitch = 3;
		layerbpp = 24;
        break;
    case MTK_FB_FORMAT_BGR888:
        cached_layer_config[id].fmt = OVL_INPUT_FORMAT_BGR888;
        eFormat = LCD_LAYER_FORMAT_RGB888;
        layerpitch = 3;
		layerbpp = 24;
        break;

    case MTK_FB_FORMAT_ARGB8888:
		cached_layer_config[id].fmt = OVL_INPUT_FORMAT_PARGB8888;
        eFormat = LCD_LAYER_FORMAT_PARGB8888;
        layerpitch = 4;
		layerbpp = 32;
        break;
    case MTK_FB_FORMAT_ABGR8888:
		cached_layer_config[id].fmt = OVL_INPUT_FORMAT_PABGR8888;
        eFormat = LCD_LAYER_FORMAT_PARGB8888;
        layerpitch = 4;
		layerbpp = 32;
        break;
	case MTK_FB_FORMAT_XRGB8888:
		cached_layer_config[id].fmt = OVL_INPUT_FORMAT_xRGB8888;
        eFormat = LCD_LAYER_FORMAT_xRGB8888;
        layerpitch = 4;
		layerbpp = 32;
        break;
	case MTK_FB_FORMAT_XBGR8888:
		cached_layer_config[id].fmt = OVL_INPUT_FORMAT_xBGR8888;
		eFormat = LCD_LAYER_FORMAT_xRGB8888;
		layerpitch = 4;
		layerbpp = 32;
		break;
    default:
        PRNERR("Invalid color format: 0x%x\n", layerInfo->src_fmt);
        ret = -EFAULT;
        goto LeaveOverlayMode;
    }
    cached_layer_config[id].vaddr = layerInfo->src_base_addr;
    cached_layer_config[id].security = layerInfo->security;

{
#if defined(MTK_WFD_SUPPORT) || defined(MTK_HDMI_SUPPORT)
		int tl = 0;
		int cnt_security_layer = 0;
		for(tl=0;tl<HW_OVERLAY_COUNT;tl++)
		{
			cnt_security_layer += cached_layer_config[tl].security;
		}
		MTKFB_LOG("Totally %d security layer is set now\n", cnt_security_layer);
#endif
#if defined (MTK_WFD_SUPPORT)
	MTK_WFD_Set_Security_Output(!!cnt_security_layer);
#endif

#if defined(MTK_HDMI_SUPPORT)
	//MTK_HDMI_Set_Security_Output(!!cnt_security_layer);
#endif
}
#if defined(MTKFB_NO_M4U)
	LCD_CHECK_RET(LCD_LayerSetAddress(id, (unsigned int)layerInfo->src_phy_addr));
#else
#if defined(MTK_M4U_SUPPORT)
	if(FB_LAYER != id && (FB_LAYER + 1) != id){
		struct fb_overlay_buffer_list* searched_node;
		u4OvlPhyAddr = (unsigned int)(layerInfo->src_base_addr);
		if(u4OvlPhyAddr == 0)
		{
			MTKFB_LOG("[FB Error]overlay_va should not be 0x00000000\n");
			LCD_CHECK_RET(LCD_LayerEnable(id, 0));
			ret = -EFAULT;
        	goto LeaveOverlayMode;
		}
		searched_node = mtkfb_search_overlayList(overlay_buffer_head, u4OvlPhyAddr);
		if(searched_node == NULL){
			LCD_CHECK_RET(LCD_LayerEnable(id, 0));
        	ret = 0;
        	goto LeaveOverlayMode;
		}
		else
			layer_addr = searched_node->src_mva;
	}
	else{
		u4OvlPhyAddr = (unsigned int)(layerInfo->src_phy_addr);
		layer_addr = u4OvlPhyAddr;
	}

//	if(layerInfo->layer_type == LAYER_2D)
		LCD_CHECK_RET(LCD_LayerSetAddress(id, layer_addr));
#if defined(MTK_LCD_HW_3D_SUPPORT)
/*
	else
	{
		unsigned int l_layer_addr = layer_addr;
		unsigned int r_layer_addr = layer_addr;

		switch (layerInfo->layer_type)
		{
			case LAYER_3D_SBS_0 :
			case LAYER_3D_SBS_180 :
				//r_layer_addr += (layerInfo->src_pitch * layerpitch / 2); break;
				r_layer_addr += (layerInfo->tgt_width * layerpitch / 2); break;
			case LAYER_3D_SBS_90 :
			case LAYER_3D_SBS_270 :
				r_layer_addr += (layerInfo->src_pitch * layerInfo->tgt_height * layerpitch / 2); break;
			default :
				break;
		}
		if (!(layerInfo->layer_id % 2))
		{
			LCD_CHECK_RET(LCD_LayerSetAddress(layerInfo->layer_id, l_layer_addr));
			LCD_CHECK_RET(LCD_LayerSetAddress(layerInfo->layer_id + 1, r_layer_addr));
		}
	}
*/
#endif
#else
	if(FB_LAYER == id){
		u4OvlPhyAddr = layerInfo->src_phy_addr;
	}
	else{
		u4OvlPhyAddr = mtkfb_user_v2p(layerInfo->src_base_addr);
		if(u4OvlPhyAddr == 0)
		{
			MTKFB_LOG("[FB Error]overlay_va should not be 0x00000000\n");
			LCD_CHECK_RET(LCD_LayerEnable(id, 0));
			ret = -EFAULT;
        	goto LeaveOverlayMode;
		}
		if(!is_pmem_range(u4OvlPhyAddr, layerpitch * layerInfo->src_width * layerInfo->src_height)){
			MTKFB_LOG("error: M4U is not supported but non-PMEM is set to video-layer\n");
			ASSERT(0);
		}
	}
	LCD_CHECK_RET(LCD_LayerSetAddress(id, u4OvlPhyAddr));
#endif
#endif
    //LCD_CHECK_RET(LCD_LayerSetFormat(id, eFormat));

#ifdef MT65XX_NEW_DISP
	LCD_CHECK_RET(LCD_LayerEnableTdshp(id, layerInfo->isTdshp));
	LCD_CHECK_RET(LCD_LayerSetNextBuffIdx(id, layerInfo->next_buff_idx));
    cached_layer_config[id].identity = layerInfo->identity;
    cached_layer_config[id].connected_type = layerInfo->connected_type;
#endif

    //set Alpha blending
	if (MTK_FB_FORMAT_ARGB8888 == layerInfo->src_fmt ||
        MTK_FB_FORMAT_ABGR8888 == layerInfo->src_fmt)
    {
        LCD_CHECK_RET(LCD_LayerSetAlphaBlending(id, TRUE, 0xFF));
	} else {
		LCD_CHECK_RET(LCD_LayerSetAlphaBlending(id, FALSE, 0xFF));
	}

    //set src width, src height
    //LCD_CHECK_RET(LCD_LayerSetSize(id, layerInfo->src_width, layerInfo->src_height));
    cached_layer_config[id].src_x = layerInfo->src_offset_x;
    cached_layer_config[id].src_y = layerInfo->src_offset_y;
    cached_layer_config[id].src_w = layerInfo->src_width;
    cached_layer_config[id].src_h = layerInfo->src_height;
    cached_layer_config[id].dst_x = layerInfo->tgt_offset_x;
    cached_layer_config[id].dst_y = layerInfo->tgt_offset_y;
    cached_layer_config[id].dst_w = layerInfo->tgt_width;
    cached_layer_config[id].dst_h = layerInfo->tgt_height;
    if (cached_layer_config[id].dst_w > cached_layer_config[id].src_w)
        cached_layer_config[id].dst_w = cached_layer_config[id].src_w;
    if (cached_layer_config[id].dst_h > cached_layer_config[id].src_h)
        cached_layer_config[id].dst_h = cached_layer_config[id].src_h;
    //LCD_CHECK_RET(LCD_LayerSetWindowOffset(id, layerInfo->src_offset_x, layerInfo->src_offset_y));

	LCD_CHECK_RET(LCD_LayerSetPitch(id, layerInfo->src_pitch*layerpitch));
#if defined(MTK_LCD_HW_3D_SUPPORT)
	if(layerInfo->layer_type == LAYER_2D)
	{
		//DISP_Set3DPWM(0,0);
		LCD_CHECK_RET(LCD_LayerSet3D(id, 0, 0, 0));
	}
	else
	{
		switch (layerInfo->layer_type)
		{
			case LAYER_3D_SBS_0 :
				r_1st=0; landscape=0; break;
			case LAYER_3D_SBS_90 :
				r_1st=0; landscape=1; break;
			case LAYER_3D_SBS_180 :
				r_1st=1; landscape=0; break;
			case LAYER_3D_SBS_270 :
				r_1st=1; landscape=1; break;
			default :
				break;
		}
		//DISP_Set3DPWM(1,landscape);
		LCD_CHECK_RET(LCD_LayerSet3D(id, 1, r_1st, landscape));
	}
#endif

#if defined(DITHERING_SUPPORT)
	{
		bool ditherenabled = false;
		UINT32 ditherbpp = DISP_GetOutputBPPforDithering();
		UINT32 dbr = 0;
		UINT32 dbg = 0;
		UINT32 dbb = 0;

		if(ditherbpp < layerbpp)
		{
			if(ditherbpp == 16)
			{
				if(layerbpp == 18)
				{
					dbr = 1;
					dbg = 0;
					dbb = 1;
					ditherenabled = true;
				}
				else if(layerbpp == 24 || layerbpp == 32)
				{
					dbr = 2;
					dbg = 1;
					dbb = 2;
					ditherenabled = true;
				}
				else
				{
					MTKFB_LOG("ERROR, error dithring bpp settings\n");
				}
			}
			else if(ditherbpp == 18)
			{
				if(layerbpp == 24 || layerbpp == 32)
				{
					dbr = 1;
					dbg = 1;
					dbb = 1;
					ditherenabled = true;
				}
				else
				{
					MTKFB_LOG("ERROR, error dithring bpp settings\n");
					ASSERT(0);
				}
			}
            else if(ditherbpp == 24)
            {
                // do nothing here.
            }
			else
			{
				MTKFB_LOG("ERROR, error dithering bpp settings, diterbpp = %d\n",ditherbpp);
				ASSERT(0);
			}

			if(ditherenabled)
			{
				LCD_CHECK_RET(LCD_LayerEnableDither(id, true));
				DISP_ConfigDither(14, 14, 14, dbr, dbg, dbb);
				if(FB_LAYER == id){
					dbr_backup = dbr;dbg_backup = dbg;dbb_backup = dbb;
					fblayer_dither_needed = ditherenabled;
					MTKFB_LOG("[FB driver] dither enabled:%d, dither bit(%d,%d,%d)\n", fblayer_dither_needed, dbr_backup, dbg_backup, dbb_backup);
				}
			}
		}
		else
		{
			// no dithering needed.
		}

		// debug message
        #if 0
		MTKFB_LOG("[%s] layer %d, ditherbpp=%d, layerbpp=%d\n", __func__, id, ditherbpp, layerbpp);
		MTKFB_LOG("[%s] dithering enabled: %d, dbr=%d, dbg=%d, dbb=%d\n", __func__, ditherenabled, dbr, dbg, dbb);
        #endif
 	}
#endif

    if(0 == strncmp(MTK_LCM_PHYSICAL_ROTATION, "180", 3))
	{
		layerInfo->layer_rotation = (layerInfo->layer_rotation + MTK_FB_ORIENTATION_180) % 4;
		layerInfo->tgt_offset_x = MTK_FB_XRES - (layerInfo->tgt_offset_x + layerInfo->tgt_width);
		layerInfo->tgt_offset_y = MTK_FB_YRES - (layerInfo->tgt_offset_y + layerInfo->tgt_height);
//		layerInfo->tgt_offset_x = MTK_FB_XRES - layerInfo->tgt_offset_x - 1;
//		layerInfo->tgt_offset_y = MTK_FB_YRES - layerInfo->tgt_offset_y - 1;
	}

    //set target x, y offset
    LCD_CHECK_RET(LCD_LayerSetOffset(id, layerInfo->tgt_offset_x, layerInfo->tgt_offset_y));

	// xuecheng add, set rotation.
	LCD_CHECK_RET(LCD_LayerSetRotation(id, layerInfo->layer_rotation));
	video_rotation = layerInfo->video_rotation;
    //set target width, height
    //mt6516: target w/h must = sourc width/height
//    ASSERT((layerInfo->src_width) == (layerInfo->tgt_width));
//    ASSERT((layerInfo->src_height) == (layerInfo->tgt_height));

    //set color key
    LCD_CHECK_RET(LCD_LayerSetSourceColorKey(id, layerInfo->src_use_color_key, layerInfo->src_color_key));

    //data transferring is triggerred in MTKFB_TRIG_OVERLAY_OUT
    LCD_CHECK_RET(LCD_LayerEnable(id, enable));
	// force writing cached data into dram
#if 0
	dmac_clean_range(layerInfo->src_base_addr,
                     layerInfo->src_base_addr +
                     (layerInfo->src_pitch * layerInfo->src_height *
                     GET_MTK_FB_FORMAT_BPP(layerInfo->src_fmt)));
#endif
LeaveOverlayMode:
    // Leave Overlay Mode if only FB layer is enabled

    if ((fbdev->layer_enable & ((1 << VIDEO_LAYER_COUNT)-1)) == 0) {
        if (DISP_STATUS_OK == DISP_LeaveOverlayMode()) {
            MTKFB_LOG("mtkfb_ioctl(MTKFB_DISABLE_OVERLAY)\n");
   	        if(fblayer_dither_needed){
				LCD_CHECK_RET(LCD_LayerEnableDither(FB_LAYER, true));
	        	DISP_ConfigDither(14, 14, 14, dbr_backup, dbg_backup, dbb_backup);
	    	}
        }
    }

    if (is_early_suspended) {
        DISP_CHECK_RET(DISP_LCDPowerEnable(FALSE));
    }

    MSG_FUNC_LEAVE();
    MMProfileLog(MTKFB_MMP_Events.SetOverlayLayer, MMProfileFlagEnd);

    return ret;
}

static int mtkfb_get_overlay_layer_info(struct fb_overlay_layer_info* layerInfo)
{
#ifdef MT65XX_NEW_DISP
    DISP_LAYER_INFO layer;
    layer.id = layerInfo->layer_id;
    DISP_GetLayerInfo(&layer);
    layerInfo->curr_en = layer.curr_en;
    layerInfo->next_en = layer.next_en;
    layerInfo->hw_en = layer.hw_en;
    layerInfo->curr_idx = layer.curr_idx;
    layerInfo->next_idx = layer.next_idx;
    layerInfo->hw_idx = layer.hw_idx;
    layerInfo->curr_identity = layer.curr_identity;
    layerInfo->next_identity = layer.next_identity;
    layerInfo->hw_identity = layer.hw_identity;
    layerInfo->curr_conn_type = layer.curr_conn_type;
    layerInfo->next_conn_type = layer.next_conn_type;
    layerInfo->hw_conn_type = layer.hw_conn_type;
    MMProfileLogEx(MTKFB_MMP_Events.LayerInfo[layerInfo->layer_id], MMProfileFlagPulse, (layerInfo->next_idx<<16)+((layerInfo->curr_idx)&0xFFFF), (layerInfo->hw_idx<<16)+(layerInfo->next_en<<8)+(layerInfo->curr_en<<4)+layerInfo->hw_en);
#else
    return -1;
#endif
}

static int mtkfb_get_video_layer(struct fb_info *info, struct fb_overlay_layer *layerInfo)
{
	struct mtkfb_device *fbdev = (struct mtkfb_device *)info->par;
    int ret = 0;
	unsigned int id = 2;  //layer 2 is video layer
	MTKFB_FUNC();

	layerInfo->layer_id = id; //layer_2 is video layer
    layerInfo->layer_enable = (fbdev->layer_enable >> id) & 1;

    if(layerInfo->layer_enable == 0){
	    MTKFB_LOG("[FB Driver] Video Layer not enable when mtkfb_get_video_layer()\n");
		return -1;
	}

	if (down_interruptible(&sem_early_suspend)) {
        printk("[FB Driver] can't get semaphore in mtkfb_capture_framebuffer()\n");
        return -ERESTARTSYS;
    }

	sem_early_suspend_cnt--;
    if (is_early_suspended) {
		  sem_early_suspend_cnt++;
          up(&sem_early_suspend);
          return -1;
    }

    LCD_Get_VideoLayerSize(id, &layerInfo->src_width, &layerInfo->src_height);

	switch(video_rotation)
	{
		case 0:
		case 2:break;
		case 1:
		case 3:
		{
			unsigned int width = layerInfo->src_height;
			unsigned int height = layerInfo->src_width;
			layerInfo->src_height = height;
			layerInfo->src_width = width;
			break;
		}
	}
	sem_early_suspend_cnt++;
    up(&sem_early_suspend);

    MSG_FUNC_LEAVE();

    return ret;
}

static int mtkfb_capture_videobuffer(struct fb_info *info, unsigned int pvbuf)
{
    int ret = 0;

    MTKFB_FUNC();

    if(0 == video_layerInfo.layer_enable){// video layer not enable
	    MTKFB_LOG("[FB Driver] Video Layer not enable when mtkfb_capture_videobuffer()\n");
		return -1;
	}

    if (down_interruptible(&sem_early_suspend)) {
        printk("[FB Driver] can't get semaphore in mtkfb_capture_framebuffer()\n");
        return -ERESTARTSYS;
    }

	sem_early_suspend_cnt--;
    /** LCD registers can't be R/W when its clock is gated in early suspend
        mode; power on/off LCD to modify register values before/after func.
    */
    if (is_early_suspended) {
		  sem_early_suspend_cnt++;
          up(&sem_early_suspend);
          return -1;
    }

    DISP_Capture_Videobuffer(pvbuf, info->var.bits_per_pixel, video_rotation);

	sem_early_suspend_cnt++;
    up(&sem_early_suspend);

    MSG_FUNC_LEAVE();

    return ret;
}
#if defined(MTK_TVOUT_SUPPORT)
extern bool capture_tv_buffer;
#endif


static atomic_t capture_ui_layer_only = ATOMIC_INIT(0); /* when capturing framebuffer ,whether capture ui layer only */
void mtkfb_capture_fb_only(bool enable)
{
    atomic_set(&capture_ui_layer_only, enable);
}

static int mtkfb_capture_framebuffer(struct fb_info *info, unsigned int pvbuf)
{
    int ret = 0;
    MMProfileLogEx(MTKFB_MMP_Events.CaptureFramebuffer, MMProfileFlagStart, pvbuf, 0);
    MTKFB_FUNC();
	if (down_interruptible(&sem_flipping)) {
        printk("[FB Driver] can't get semaphore:%d\n", __LINE__);
        MMProfileLogEx(MTKFB_MMP_Events.CaptureFramebuffer, MMProfileFlagEnd, 0, 1);
        return -ERESTARTSYS;
    }
	sem_flipping_cnt--;
    mutex_lock(&ScreenCaptureMutex);

    /** LCD registers can't be R/W when its clock is gated in early suspend
        mode; power on/off LCD to modify register values before/after func.
    */
    if (is_early_suspended) {
#ifndef MT65XX_NEW_DISP
        DISP_CHECK_RET(DISP_LCDPowerEnable(TRUE));
#else
        // Turn on engine clock.
        disp_path_clock_on("mtkfb");
        //DISP_CHECK_RET(DISP_PowerEnable(TRUE));
#endif
    }

#if defined (MTK_TVOUT_SUPPORT)
    if (!capture_tv_buffer)
    {
        if (atomic_read(&capture_ui_layer_only))
        {
        		unsigned int fbv;
            unsigned int w_xres = (unsigned short)fb_layer_context.src_width;
            unsigned int h_yres = (unsigned short)fb_layer_context.src_height;
            unsigned int pixel_bpp = info->var.bits_per_pixel / 8; // bpp is either 32 or 16, can not be other value
            unsigned int w_fb = (unsigned int)fb_layer_context.src_pitch;
            unsigned int fbsize = w_fb * h_yres * pixel_bpp; // frame buffer size
            unsigned int fbaddress = info->fix.smem_start + info->var.yoffset * info->fix.line_length; //physical address
            unsigned int mem_off_x = (unsigned short)fb_layer_context.src_offset_x;
            unsigned int mem_off_y = (unsigned short)fb_layer_context.src_offset_y;
            fbaddress += (mem_off_y * w_fb + mem_off_x) * pixel_bpp;
            fbv = (unsigned int)ioremap_nocache(fbaddress, fbsize);
            MTKFB_LOG("[FB Driver], w_xres = %d, h_yres = %d, w_fb = %d, pixel_bpp = %d, fbsize = %d, fbaddress = 0x%08x\n", w_xres, h_yres, w_fb, pixel_bpp, fbsize, fbaddress);
            if (!fbv)
            {
                MTKFB_LOG("[FB Driver], Unable to allocate memory for frame buffer: address=0x%08x, size=0x%08x\n", \
                        fbaddress, fbsize);
                goto EXIT;
            }
            {
                unsigned int i;
                for(i = 0;i < h_yres; i++)
                {
                    memcpy((void *)(pvbuf + i * w_xres * pixel_bpp), (void *)(fbv + i * w_fb * pixel_bpp), w_xres * pixel_bpp);
                }
            }
            iounmap((void *)fbv);
        }
        else
            DISP_Capture_Framebuffer(pvbuf, info->var.bits_per_pixel);
    }
    else
    {
        TVOUT_Capture_Tvrotbuffer(pvbuf, info->var.bits_per_pixel);
    }
#else
    if (atomic_read(&capture_ui_layer_only))
    {
        unsigned int w_xres = (unsigned short)fb_layer_context.src_width;
        unsigned int h_yres = (unsigned short)fb_layer_context.src_height;
        unsigned int pixel_bpp = info->var.bits_per_pixel / 8; // bpp is either 32 or 16, can not be other value
        unsigned int w_fb = (unsigned int)fb_layer_context.src_pitch;
        unsigned int fbsize = w_fb * h_yres * pixel_bpp; // frame buffer size
        unsigned int fbaddress = info->fix.smem_start + info->var.yoffset * info->fix.line_length; //physical address
        unsigned int mem_off_x = (unsigned short)fb_layer_context.src_offset_x;
        unsigned int mem_off_y = (unsigned short)fb_layer_context.src_offset_y;
        unsigned int fbv = 0;
        fbaddress += (mem_off_y * w_fb + mem_off_x) * pixel_bpp;
        fbv = (unsigned int)ioremap_nocache(fbaddress, fbsize);
        MTKFB_LOG("[FB Driver], w_xres = %d, h_yres = %d, w_fb = %d, pixel_bpp = %d, fbsize = %d, fbaddress = 0x%08x\n", w_xres, h_yres, w_fb, pixel_bpp, fbsize, fbaddress);
        if (!fbv)
        {
            MTKFB_LOG("[FB Driver], Unable to allocate memory for frame buffer: address=0x%08x, size=0x%08x\n", \
                    fbaddress, fbsize);
            goto EXIT;
        }
        {
            unsigned int i;
            for(i = 0;i < h_yres; i++)
            {
                memcpy((void *)(pvbuf + i * w_xres * pixel_bpp), (void *)(fbv + i * w_fb * pixel_bpp), w_xres * pixel_bpp);
            }
        }
        iounmap((void *)fbv);
    }
    else
        DISP_Capture_Framebuffer(pvbuf, info->var.bits_per_pixel, is_early_suspended);
#endif


EXIT:
    if (is_early_suspended) {
#ifndef MT65XX_NEW_DISP
        DISP_CHECK_RET(DISP_LCDPowerEnable(FALSE));
#else
        // Turn off engine clock.
        //DISP_CHECK_RET(DISP_PowerEnable(FALSE));
        disp_path_clock_off("mtkfb");
#endif
    }

    mutex_unlock(&ScreenCaptureMutex);
    sem_flipping_cnt++;
	up(&sem_flipping);
    MSG_FUNC_LEAVE();
    MMProfileLogEx(MTKFB_MMP_Events.CaptureFramebuffer, MMProfileFlagEnd, 0, 0);

    return ret;
}

#if defined(MTK_LCD_HW_3D_SUPPORT)
static int mtkfb_set_s3d_ftm(struct fb_info *info, unsigned int mode)
{
	int ret = 0;

	struct fb_overlay_layer layerInfo;
	//uint8_t framebuffer[1080*960*3];
    uint8_t *framebuffer = (uint8_t *)vmalloc(1080*960*3);
    if (!framebuffer)
    {
        MTKFB_LOG("[FB Driver] can't allocate memory in mtkfb_set_s3d_ftm()\n");
        return -ERESTARTSYS;
    }

    MTKFB_FUNC();

	if (down_interruptible(&sem_flipping)) {
        MTKFB_LOG("[FB Driver] can't get semaphore in mtkfb_set_backlight_pwm()\n");
        return -ERESTARTSYS;
    }
	sem_flipping_cnt--;

	if (down_interruptible(&sem_early_suspend)) {
        MTKFB_LOG("[FB Driver] can't get semaphore in mtkfb_set_backlight_pwm()\n");
		sem_flipping_cnt++;
		up(&sem_flipping);
        return -ERESTARTSYS;
    }
	sem_early_suspend_cnt--;
	//memcpy(&framebuffer[0], &portrait_1080x960[0], sizeof(portrait_1080x960));

	if (mode)
	{
		layerInfo.layer_id = FB_LAYER;
		layerInfo.layer_enable = 1;
		//layerInfo.src_base_addr = (void *)&framebuffer[0];
		layerInfo.src_phy_addr = (void *)&framebuffer[0];
		layerInfo.src_direct_link = 0;
		layerInfo.src_offset_x = layerInfo.src_offset_y = 0;
		layerInfo.src_width = layerInfo.src_pitch = layerInfo.tgt_width = 540;
		layerInfo.src_height = layerInfo.tgt_height = 960;
		layerInfo.tgt_offset_x = layerInfo.tgt_offset_y = 0;
		layerInfo.src_color_key = 0;
		layerInfo.layer_rotation = MTK_FB_ORIENTATION_0;
		layerInfo.layer_type = LAYER_3D_SBS_0;

        mutex_lock(&OverlaySettingMutex);
		mtkfb_set_overlay_layer(info, &layerInfo);
		layerInfo.layer_id = FB_LAYER + 1;
		mtkfb_set_overlay_layer(info, &layerInfo);
        atomic_set(&OverlaySettingDirtyFlag, 1);
        atomic_set(&OverlaySettingApplied, 0);
        mutex_unlock(&OverlaySettingMutex);
	}
	else
	{
		ret= mtkfb_set_par(info);
		if (ret != 0)
			PRNERR("failed to mtkfb_set_par\n");
	}
	sem_early_suspend_cnt++;
	sem_flipping_cnt++;
    up(&sem_early_suspend);
	up(&sem_flipping);

    vfree((void *)framebuffer);

    MSG_FUNC_LEAVE();

	return ret;
}
#endif

mtk_dispif_info_t dispif_info[MTKFB_MAX_DISPLAY_COUNT];
extern unsigned int isAEEEnabled;
static int mtkfb_ioctl(struct file *file, struct fb_info *info, unsigned int cmd, unsigned long arg)
{
    void __user *argp = (void __user *)arg;
    struct mtkfb_device *fbdev = (struct mtkfb_device *)info->par;
    DISP_STATUS ret = 0;
    int r = 0;

	MTKFB_FUNC();

    switch (cmd) {
	case MTKFB_GET_MAX_DISPLAY_COUNT:
	{
		unsigned int count = MTKFB_MAX_DISPLAY_COUNT;
		MTKFB_FUNC();
		return copy_to_user(argp, &count,  sizeof(count)) ? -EFAULT : 0;
	}
	case MTKFB_GET_DISPLAY_IF_INFORMATION:
	{
		int displayid = 0;
		if (copy_from_user(&displayid, (void __user *)arg, sizeof(displayid))) {
			MTKFB_LOG("[FB]: copy_from_user failed! line:%d \n", __LINE__);
			return -EFAULT;
		}
		printk("%s, display_id=%d\n", __func__, displayid);
		MTKFB_FUNC();
		if (displayid > MTKFB_MAX_DISPLAY_COUNT) {
			MTKFB_LOG("[FB]: invalid display id:%d \n", displayid);
			return -EFAULT;
		}
		if (copy_to_user((void __user *)arg, &(dispif_info[displayid]),  sizeof(mtk_dispif_info_t))) {
			MTKFB_LOG("[FB]: copy_to_user failed! line:%d \n", __LINE__);
			r = -EFAULT;
		}
		return (r);
	}
	case MTKFB_POWEROFF:
   	{
		MTKFB_FUNC();
		if(is_early_suspended) return r;
    	if (down_interruptible(&sem_early_suspend))
		{
        	printk("[FB Driver] can't get semaphore in mtkfb_early_suspend()\n");
        	return -ERESTARTSYS;
    	}

    	is_early_suspended = TRUE;

		DISP_CHECK_RET(DISP_PanelEnable(FALSE));
 		DISP_CHECK_RET(DISP_PowerEnable(FALSE));

    	up(&sem_early_suspend);

		return r;
	}

	case MTKFB_POWERON:
   	{
		MTKFB_FUNC();
//		if(!is_early_suspended) return r;
		if (down_interruptible(&sem_early_suspend))
		{
        	printk("[FB Driver] can't get semaphore in mtkfb_late_resume()\n");
        	return -ERESTARTSYS;
    	}

    	DISP_CHECK_RET(DISP_PowerEnable(TRUE));
    	DISP_CHECK_RET(DISP_PanelEnable(TRUE));

		is_early_suspended = FALSE;

    	up(&sem_early_suspend);

		return r;
	}
    case MTKFB_GET_POWERSTATE:
    {
        unsigned long power_state;

        if(is_early_suspended == TRUE)
            power_state = 0;
        else
            power_state = 1;

        return copy_to_user(argp, &power_state,  sizeof(power_state)) ? -EFAULT : 0;
    }


    case MTKFB_GETVFRAMEPHYSICAL:
        return copy_to_user(argp, &fbdev->fb_pa_base,
                            sizeof(fbdev->fb_pa_base)) ? -EFAULT : 0;

    case MTKFB_CONFIG_IMMEDIATE_UPDATE:
    {
        MTKFB_LOG("[%s] MTKFB_CONFIG_IMMEDIATE_UPDATE, enable = %lu\n",
            __func__, arg);
		if (down_interruptible(&sem_early_suspend)) {
        		MTKFB_LOG("[mtkfb_ioctl] can't get semaphore:%d\n", __LINE__);
        		return -ERESTARTSYS;
    	}
		sem_early_suspend_cnt--;
        DISP_WaitForLCDNotBusy();
        ret = DISP_ConfigImmediateUpdate((BOOL)arg);
		sem_early_suspend_cnt++;
		up(&sem_early_suspend);
        return (r);
    }

    case MTKFB_CAPTURE_FRAMEBUFFER:
    {
        unsigned int pbuf = 0;
        if (copy_from_user(&pbuf, (void __user *)arg, sizeof(pbuf)))
        {
            MTKFB_LOG("[FB]: copy_from_user failed! line:%d \n", __LINE__);
            r = -EFAULT;
        }
        else
        {
            mtkfb_capture_framebuffer(info, pbuf);
        }

        return (r);
    }

#ifdef MTK_FB_OVERLAY_SUPPORT
    case MTKFB_GET_OVERLAY_LAYER_INFO:
    {
        struct fb_overlay_layer_info layerInfo;
        MTKFB_LOG(" mtkfb_ioctl():MTKFB_GET_OVERLAY_LAYER_INFO\n");

        if (copy_from_user(&layerInfo, (void __user *)arg, sizeof(layerInfo))) {
            MTKFB_LOG("[FB]: copy_from_user failed! line:%d \n", __LINE__);
            return -EFAULT;
        }
        if (mtkfb_get_overlay_layer_info(&layerInfo) < 0)
        {
            MTKFB_LOG("[FB]: Failed to get overlay layer info\n");
            return -EFAULT;
        }
        if (copy_to_user((void __user *)arg, &layerInfo, sizeof(layerInfo))) {
            MTKFB_LOG("[FB]: copy_to_user failed! line:%d \n", __LINE__);
            r = -EFAULT;
        }
        return (r);
    }
    case MTKFB_SET_OVERLAY_LAYER:
    {
        struct fb_overlay_layer layerInfo;
        MTKFB_LOG(" mtkfb_ioctl():MTKFB_SET_OVERLAY_LAYER\n");

        if (copy_from_user(&layerInfo, (void __user *)arg, sizeof(layerInfo))) {
            MTKFB_LOG("[FB]: copy_from_user failed! line:%d \n", __LINE__);
            r = -EFAULT;
        } else {
            mutex_lock(&OverlaySettingMutex);
            mtkfb_set_overlay_layer(info, &layerInfo);
            if (is_ipoh_bootup)
            {
                int i;
                for (i=0; i<DDP_OVL_LAYER_MUN; i++)
                    cached_layer_config[i].isDirty = 1;
                is_ipoh_bootup = false;
            }
            atomic_set(&OverlaySettingDirtyFlag, 1);
            atomic_set(&OverlaySettingApplied, 0);
            mutex_unlock(&OverlaySettingMutex);
        }
        return (r);
    }

	case MTKFB_GET_VIDEOLAYER_SIZE:
    {
        MTKFB_LOG(" mtkfb_ioctl():MTKFB_GET_VIDEO_LAYER\n");

        if(mtkfb_get_video_layer(info, &video_layerInfo) < 0)
		    return -EFAULT;
		if (copy_to_user((void __user *)arg, &video_layerInfo, sizeof(video_layerInfo))){
            MTKFB_LOG("[FB]: copy_to_user failed! line:%d \n", __LINE__);
            r = -EFAULT;
        }
        return (r);
    }

	case MTKFB_CAPTURE_VIDEOBUFFER:
    {
        unsigned int pbuf = 0;
		MTKFB_LOG(" mtkfb_ioctl(): MTKFB_CAPTURE_VIDEOBUFFER\n");
        if (copy_from_user(&pbuf, (void __user *)arg, sizeof(pbuf)))
        {
            MTKFB_LOG("[FB]: copy_from_user failed! line:%d \n", __LINE__);
            r = -EFAULT;
        }
        else
        {
            if(mtkfb_capture_videobuffer(info, pbuf) < 0)
			    return -EFAULT;
        }

        return (r);
    }

    case MTKFB_SET_VIDEO_LAYERS:
    {
        struct mmp_fb_overlay_layers
        {
            struct fb_overlay_layer Layer0;
            struct fb_overlay_layer Layer1;
            struct fb_overlay_layer Layer2;
            struct fb_overlay_layer Layer3;
        };
        struct fb_overlay_layer layerInfo[VIDEO_LAYER_COUNT];
        MTKFB_LOG(" mtkfb_ioctl():MTKFB_SET_VIDEO_LAYERS\n");
        MMProfileLog(MTKFB_MMP_Events.SetOverlayLayers, MMProfileFlagStart);
        if (copy_from_user(&layerInfo, (void __user *)arg, sizeof(layerInfo))) {
            MTKFB_LOG("[FB]: copy_from_user failed! line:%d \n", __LINE__);
            MMProfileLogMetaString(MTKFB_MMP_Events.SetOverlayLayers, MMProfileFlagEnd, "Copy_from_user failed!");
            r = -EFAULT;
        } else {
            int32_t i;
#if defined(MTK_LCD_HW_3D_SUPPORT)
/*
			if (layerInfo[2].layer_type != LAYER_2D)
			{
				memcpy(&layerInfo[0], &layerInfo[2], sizeof(layerInfo[2]));
				memcpy(&layerInfo[1], &layerInfo[2], sizeof(layerInfo[2]));
				layerInfo[0].layer_id=0;
				layerInfo[1].layer_id=1;
				layerInfo[2].layer_enable=0;
			}
*/
#endif
            mutex_lock(&OverlaySettingMutex);
            for (i = 0; i < VIDEO_LAYER_COUNT; ++i) {
                mtkfb_set_overlay_layer(info, &layerInfo[i]);
            }
            is_ipoh_bootup = false;
            atomic_set(&OverlaySettingDirtyFlag, 1);
            atomic_set(&OverlaySettingApplied, 0);
            mutex_unlock(&OverlaySettingMutex);
            MMProfileLogStructure(MTKFB_MMP_Events.SetOverlayLayers, MMProfileFlagEnd, layerInfo, struct mmp_fb_overlay_layers);
        }

        return (r);
    }

    case MTKFB_WAIT_OVERLAY_READY:
        DISP_WaitForLCDNotBusy();
        r = 0;
        return (r);

    case MTKFB_GET_OVERLAY_LAYER_COUNT:
    {
        int hw_layer_count = VIDEO_LAYER_COUNT;
        if (copy_to_user((void __user *)arg, &hw_layer_count, sizeof(hw_layer_count)))
        {
            return -EFAULT;
        }
        return 0;
    }

    case MTKFB_TRIG_OVERLAY_OUT:
	{
        MTKFB_LOG(" mtkfb_ioctl():MTKFB_TRIG_OVERLAY_OUT\n");
        MMProfileLog(MTKFB_MMP_Events.TrigOverlayOut, MMProfileFlagPulse);
        return mtkfb_update_screen(info);
	}

	case MTKFB_REGISTER_OVERLAYBUFFER:
    {
		struct fb_overlay_buffer_info overlay_buffer;
		printk("[mtkfb_ioctl]MTKFB_REGISTER_OVERLAYBUFFER\n");
#if defined(MTK_M4U_SUPPORT)
		if (copy_from_user(&overlay_buffer, (void __user *)arg, sizeof(overlay_buffer))) {
            printk("[FB]: copy_from_user failed! line:%d \n", __LINE__);
            r = -EFAULT;
        }
		else
		{
			struct fb_overlay_buffer_list *c,*n;
			unsigned int overlay_mva;
			if(overlay_buffer.src_vir_addr == 0)
			{
				printk("[mtkfb_ioctl Error]MTKFB_REGISTER_OVERLAYBUFFER VA should not be 0x00000000\n");
				return (r);
			}
			c = overlay_buffer_head;
			{
				struct fb_overlay_buffer_list *a;
				a = overlay_buffer_head;
				printk("[mtkfb_ioctl] before add a node,list node Addr:\n");
				while(a)
				{
					printk("0x%x,  ", (unsigned int)a);
					a = a->next;
				}
			}
			printk("\n[mtkfb_ioctl]MTKFB_REGISTER_OVERLAYBUFFER,va = 0x%x,size = %d\n", overlay_buffer.src_vir_addr, overlay_buffer.size);
			while(c->next)
			{
				c = c->next;
			}
			n = (struct fb_overlay_buffer_list*)vmalloc(sizeof(struct fb_overlay_buffer_list));
			if(!n){
				printk("[mtkfb_ioctl] vmalloc failed\n");
				return -ENOMEM;
			}

			if (down_interruptible(&sem_flipping)) {
    	    	printk("[mtkfb_ioctl] can't get semaphore:%d\n",__LINE__ );
        		return -ERESTARTSYS;
   	 		}
			sem_flipping_cnt--;

			if (down_interruptible(&sem_early_suspend)) {
        		printk("[mtkfb_ioctl] can't get semaphore:%d\n",__LINE__);
				sem_flipping_cnt++;
				up(&sem_flipping);
        		return -ERESTARTSYS;
    		}
			sem_early_suspend_cnt--;
            DISP_AllocOverlayMva(overlay_buffer.src_vir_addr, &overlay_mva, overlay_buffer.size);
			printk("[mtkfb_ioctl]MTKFB_REGISTER_OVERLAYBUFFER,allocated mva = 0x%x\n", overlay_mva);
			n->src_mva = overlay_mva;
			n->buffer.src_vir_addr = overlay_buffer.src_vir_addr;
			n->buffer.size = overlay_buffer.size;
			n->pid = current->tgid;
			n->file_addr = (void*)file;
			printk("[current process ID = %d, current process name:%s]\n", current->tgid, current->comm);
			n->next = NULL;
			c->next = n;
			{
				struct fb_overlay_buffer_list *b;
				b = overlay_buffer_head;
				printk("[mtkfb_ioctl] after add a node,list node Addr:\n");

				while(b)
				{
					printk("0x%x,  ", (unsigned int)b);
					b = b->next;
				}
				printk("\n");
			}
			sem_flipping_cnt++;
			sem_early_suspend_cnt++;
 		   	up(&sem_early_suspend);
			up(&sem_flipping);

		}
#else
        MTKFB_LOG("[FB]: not enable m4u! line:%d \n", __LINE__);
#endif
		return (r);
    }

	case MTKFB_UNREGISTER_OVERLAYBUFFER:
    {
		unsigned int overlay_bufferVA;
		printk("[mtkfb_ioctl]MTKFB_UNREGISTER_OVERLAYBUFFER\n");
#if defined(MTK_M4U_SUPPORT)
		if (copy_from_user(&overlay_bufferVA, (void __user *)arg, sizeof(overlay_bufferVA))) {
            printk("[FB]: copy_from_user failed! line:%d \n", __LINE__);
            r = -EFAULT;
        }
		else
		{
			struct fb_overlay_buffer_list *c,*n;
			printk("[mtkfb_ioctl]MTKFB_UNREGISTER_OVERLAYBUFFER,va0 = 0x%x\n", overlay_bufferVA);
			if(overlay_bufferVA == 0)
			{
				MTKFB_LOG("[mtkfb_ioctl Error]MTKFB_UNREGISTER_OVERLAYBUFFER VA should not be 0x00000000\n");
				return (r);
			}

			if (down_interruptible(&sem_flipping)) {
    	    	printk("[mtkfb_ioctl] can't get semaphore:%d\n", __LINE__);
        		return -ERESTARTSYS;
   	 		}
			sem_flipping_cnt--;

			if (down_interruptible(&sem_early_suspend)) {
        		printk("[mtkfb_ioctl] can't get semaphore:%d\n", __LINE__);
				sem_flipping_cnt++;
				up(&sem_flipping);
        		return -ERESTARTSYS;
    		}
			sem_early_suspend_cnt--;
			if (down_interruptible(&sem_overlay_buffer)) {
        		printk("[mtkfb_ioctl] can't get semaphore,%d\n", __LINE__);
				sem_flipping_cnt++;
				sem_early_suspend_cnt++;
			   	up(&sem_early_suspend);
				up(&sem_flipping);
        		return -ERESTARTSYS;
    		}

			sem_overlay_buffer_cnt--;
			c = overlay_buffer_head;
			n = c->next;
			if(n == NULL){
				sem_flipping_cnt++;
				sem_early_suspend_cnt++;
				sem_overlay_buffer_cnt++;
				up(&sem_overlay_buffer);
		   		up(&sem_early_suspend);
				up(&sem_flipping);
				return ret;
			}
			//xuecheng, add this for sem count check.
			mtkfb_show_sem_cnt();

			while(overlay_bufferVA != n->buffer.src_vir_addr || n->pid != current->tgid)
			{
				c = n;
				n = c->next;
				if(!n){
					struct fb_overlay_buffer_list *p;
					p = overlay_buffer_head->next;
//					ASSERT(p);
					printk("[FB driver] current va = 0x%x, buffer_list va:\n",overlay_bufferVA);
					while(p){
						printk("0x%x\n", p->buffer.src_vir_addr);
						p = p->next;
					}
					printk("[FB driver] ERROR: Can't find matched VA in buffer list\n");
					sem_flipping_cnt++;
					sem_early_suspend_cnt++;
					sem_overlay_buffer_cnt++;
					up(&sem_overlay_buffer);
		   			up(&sem_early_suspend);
					up(&sem_flipping);
					return ret;
				}
//				ASSERT(n); //n is NULL, but not find matched buffer
			}

			printk("[mtkfb_ioctl]MTKFB_UNREGISTER_OVERLAYBUFFER,va1 = 0x%x, size = %d\n",
					n->buffer.src_vir_addr, n->buffer.size);
			DISP_WaitForLCDNotBusy();
        	DISP_DeallocMva(n->buffer.src_vir_addr, n->src_mva, n->buffer.size);
			{
				struct fb_overlay_buffer_list *a;
				a = overlay_buffer_head;
				printk("[mtkfb_ioctl] before delete a node,list node Addr:\n");

				while(a)
				{
					printk("0x%x,  ", (unsigned int)a);
					a = a->next;
				}
			}
			c->next = n->next;
			vfree(n);
			{
				struct fb_overlay_buffer_list *b;
				b = overlay_buffer_head;
				printk("\n[mtkfb_ioctl] after delete a node,list node Addr:\n");
				while(b)
				{
					printk("0x%x,  ", (unsigned int)b);
					b = b->next;
					MTKFB_LOG("\n");
				}
			}
			sem_overlay_buffer_cnt++;
			sem_early_suspend_cnt++;
			sem_flipping_cnt++;
			up(&sem_overlay_buffer);
		   	up(&sem_early_suspend);
			up(&sem_flipping);

		}
#else
        MTKFB_LOG("[FB]: not enable m4u! line:%d \n", __LINE__);
#endif
		return (r);
    }

#endif // MTK_FB_OVERLAY_SUPPORT

    case MTKFB_SET_ORIENTATION:
    {
        MTKFB_LOG("[MTKFB] Set Orientation: %lu\n", arg);
        // surface flinger orientation definition of 90 and 270
        // is different than DISP_TV_ROT
        if (arg & 0x1) arg ^= 0x2;
	arg *=90;
#if defined(MTK_TVOUT_SUPPORT)
		TVOUT_SetOrientation((TVOUT_ROT)arg);
#endif
#if defined(MTK_HDMI_SUPPORT)
			//for MT6589, the orientation of DDPK is changed from 0123 to 0/90/180/270
			hdmi_setorientation((int)arg);
#endif
#if defined(MTK_WFD_SUPPORT)
			//for MT6589, the orientation of DDPK is changed from 0123 to 0/90/180/270
			wfd_setorientation((int)arg);
#endif
        return 0;
    }
#if defined(MTK_LCD_HW_3D_SUPPORT)
	case MTKFB_SET_COMPOSING3D:
	{
		MTKFB_LOG("[MTKFB] Set Composing 3D: %lu\n", arg);
		mtkfb_current_layer_type = arg;

		return 0;
	}

	case MTKFB_SET_S3D_FTM:
	{
		MTKFB_LOG("[MTKFB] Set S3D Factory Mode: %lu\n", arg);
		mtkfb_set_s3d_ftm(info, arg);

		return 0;
	}
#endif
    case MTKFB_TV_POST_VIDEO_BUFFER:
    {
#if defined(MTK_TVOUT_SUPPORT)
        struct fb_post_video_buffer b;
        MSG(ARGU, " mtkfb_ioctl():MTKFB_TV_POST_VIDEO_BUFFER\n");

        if (copy_from_user(&b, (void __user *)arg, sizeof(b)))
        {
            MTKFB_LOG("[FB]: copy_from_user failed! line:%d \n", __LINE__);
            r = -EFAULT;
        } else {

            TVOUT_PostVideoBuffer((UINT32)b.vir_addr,
                                      (TVOUT_SRC_FORMAT)b.format,
                                      b.width, b.height);
        }
#endif
        return (r);
    }

    case MTKFB_TV_LEAVE_VIDEO_PLAYBACK_MODE:
    {
        MSG(ARGU, " mtkfb_ioctl():MTKFB_TV_LEAVE_VIDEO_PLAYBACK_MODE\n");

#if defined(MTK_TVOUT_SUPPORT)
		TVOUT_LeaveVideoBuffer();
#endif
        return 0;
    }


    case MTKFB_META_RESTORE_SCREEN:
    {
        struct fb_var_screeninfo var;

		if (copy_from_user(&var, argp, sizeof(var)))
			return -EFAULT;

        info->var.yoffset = var.yoffset;
        init_framebuffer(info);

        return mtkfb_pan_display_impl(&var, info);
    }

    case MTKFB_LOCK_FRONT_BUFFER:
#if 0
        if (down_interruptible(&sem_flipping)) {
            printk("[FB Driver] can't get semaphore when lock front buffer\n");
            return -ERESTARTSYS;
        }
		sem_flipping_cnt--;
#endif
        return 0;

    case MTKFB_UNLOCK_FRONT_BUFFER:
#if 0
		sem_flipping_cnt++;
        up(&sem_flipping);
#endif
        return 0;
////////////////////////////////////////////FM De-sense
    case MTKFB_FM_NOTIFY_FREQ:
	{
		unsigned long freq;
		MTKFB_LOG("[MTKFB] FM set new channel\n");
		if(DISP_STATUS_OK != DISP_FMDesense_Query()){
		    MTKFB_LOG("[FB]: this project not support FM De-sense\n");
			return -EFAULT;
		}

		if (copy_from_user(&freq, (void __user *)arg, sizeof(freq))) {
            MTKFB_LOG("[FB]: copy_from_user failed! line:%d \n", __LINE__);
            r = -EFAULT;
        } else {
            MTKFB_LOG("[MTKFB] FM channel freq is %ld\n", freq);
			DISP_FM_Desense(freq);
        }
	    return (r);
	}

	case MTKFB_GET_INTERFACE_TYPE:
	{
        extern LCM_PARAMS *lcm_params;
        unsigned long lcm_type = lcm_params->type;

		MTKFB_LOG("[MTKFB] MTKFB_GET_INTERFACE_TYPE\n");

        printk("[MTKFB EM]MTKFB_GET_INTERFACE_TYPE is %ld\n", lcm_type);

        return copy_to_user(argp, &lcm_type,  sizeof(lcm_type)) ? -EFAULT : 0;
	}

	case MTKFB_RESET_UPDATESPEED:
	{
		MTKFB_LOG("[MTKFB] FM be turned off\n");
		if(DISP_STATUS_OK != DISP_FMDesense_Query()){
		    MTKFB_LOG("[FB]: this project not support FM De-sense\n");
			return -EFAULT;
		}

		DISP_Reset_Update();
        return (r);
	}

    case MTKFB_GET_DEFAULT_UPDATESPEED:
	{
	    unsigned int speed;
		MTKFB_LOG("[MTKFB] get default update speed\n");
		DISP_Get_Default_UpdateSpeed(&speed);

        printk("[MTKFB EM]MTKFB_GET_DEFAULT_UPDATESPEED is %d\n", speed);
		return copy_to_user(argp, &speed,
                            sizeof(speed)) ? -EFAULT : 0;
    }

    case MTKFB_GET_CURR_UPDATESPEED:
	{
	    unsigned int speed;
		MTKFB_LOG("[MTKFB] get current update speed\n");
		DISP_Get_Current_UpdateSpeed(&speed);

        printk("[MTKFB EM]MTKFB_GET_CURR_UPDATESPEED is %d\n", speed);
		return copy_to_user(argp, &speed,
                            sizeof(speed)) ? -EFAULT : 0;
	}

	case MTKFB_CHANGE_UPDATESPEED:
	{
	    unsigned int speed;
		MTKFB_LOG("[MTKFB] change update speed\n");

		if (copy_from_user(&speed, (void __user *)arg, sizeof(speed))) {
            MTKFB_LOG("[FB]: copy_from_user failed! line:%d \n", __LINE__);
            r = -EFAULT;
        } else {
			DISP_Change_Update(speed);

            printk("[MTKFB EM]MTKFB_CHANGE_UPDATESPEED is %d\n", speed);

        }
        return (r);
	}
	case MTKFB_BOOTANIMATION:// this interface only for 6573 bootanimation
	{
#if 0  /////only mt6573 HW limitation need do this
        static int bootanimation_cnt = 0;
		if(bootanimation_cnt > 1)return r;
		if (down_interruptible(&sem_early_suspend))
		{
        	MTKFB_LOG("[FB Driver] can't get semaphore in mtkfb_bootanimation()\n");
        	return -ERESTARTSYS;
    	}

		DISP_WaitForLCDNotBusy();
		DISP_CHECK_RET(DISP_ChangeLCDWriteCycle());
		bootanimation_cnt++;
    	up(&sem_early_suspend);
#endif
		return r;

	}

	case MTKFB_GETFPS:
	{
		printk("MTKFB_GETFPS = %d\n", lcd_fps);
		return copy_to_user(argp, &lcd_fps,
                       sizeof(lcd_fps)) ? -EFAULT : 0;
	}

	case MTKFB_VSYNC:
	{
		if(is_early_suspended){
			printk("[MTKFB_VSYNC]:mtkfb has suspend, return directly\n");
			msleep(20);
			return (r);
		}
		vsync_cnt++;
		DISP_WaitVSYNC();
		vsync_cnt--;
		return (r);
	}

	case MTKFB_FBLAYER_ENABLE:
	{
		unsigned int enable;
		if(copy_from_user(&enable,(void __user*)argp,sizeof(enable))){
		      MTKFB_LOG("[FB]: copy_from_user failed! line:%d \n", __LINE__);
                      r = -EFAULT;
		}
		else{
		MTKFB_LOG("[FB]: FDLAYER_ENABLE:%d \n",enable);

		// Consider if sem protection is necessary.
		//if (down_interruptible(&sem_flipping)) {
		//	printk("[FB Driver] can't get semaphore in mtkfb_pan_display_impl()\n");
		//	return -ERESTARTSYS;
		//}

		hwc_force_fb_enabled = (enable ? true : false);

        mutex_lock(&OverlaySettingMutex);
		LCD_CHECK_RET(LCD_LayerEnable(FB_LAYER, enable));
		if (mtkfb_using_layer_type != LAYER_2D)
			LCD_CHECK_RET(LCD_LayerEnable(FB_LAYER+1, enable));
        atomic_set(&OverlaySettingDirtyFlag, 1);
        atomic_set(&OverlaySettingApplied, 0);
        mutex_unlock(&OverlaySettingMutex);

		//up(&sem_flipping);

		}

		return (r);
	}

	case MTKFB_SET_UI_LAYER_ALPHA:
	{
		unsigned int alpha;
		if(copy_from_user(&alpha,(void __user*)argp,sizeof(alpha))){
		      MTKFB_LOG("[FB]: copy_from_user failed! line:%d \n", __LINE__);
                      r = -EFAULT;
		}
		else{
        LCD_CHECK_RET(LCD_LayerSetAlphaBlending(FB_LAYER, TRUE, alpha));
        LCD_CHECK_RET(LCD_LayerSetAlphaBlending(FB_LAYER-1, TRUE, alpha));
		}
		return (r);
	}
	case MTKFB_SET_UI_LAYER_SRCKEY:
	{
		unsigned int key_en;
		if(copy_from_user(&key_en,(void __user*)argp,sizeof(key_en))){
		      MTKFB_LOG("[FB]: copy_from_user failed! line:%d \n", __LINE__);
                      r = -EFAULT;
		}
		else{
            LCD_CHECK_RET(LCD_LayerSetSourceColorKey(FB_LAYER, key_en, 0x00000000));
		}
		return (r);
	}

    case MTKFB_AEE_LAYER_EXIST:
    {
		//printk("[MTKFB] isAEEEnabled=%d \n", isAEEEnabled);
		return copy_to_user(argp, &isAEEEnabled,
                            sizeof(isAEEEnabled)) ? -EFAULT : 0;
    }
////////////////////////////////////////////////
    default:
        return -EINVAL;
    }
}


/* Callback table for the frame buffer framework. Some of these pointers
 * will be changed according to the current setting of fb_info->accel_flags.
 */
static struct fb_ops mtkfb_ops = {
    .owner          = THIS_MODULE,
    .fb_open        = mtkfb_open,
    .fb_release     = mtkfb_release,
    .fb_setcolreg   = mtkfb_setcolreg,
    .fb_pan_display = mtkfb_pan_display_proxy,
    .fb_fillrect    = cfb_fillrect,
    .fb_copyarea    = cfb_copyarea,
    .fb_imageblit   = cfb_imageblit,
    .fb_cursor      = mtkfb_soft_cursor,
    .fb_check_var   = mtkfb_check_var,
    .fb_set_par     = mtkfb_set_par,
    .fb_ioctl       = mtkfb_ioctl,
};

/*
 * ---------------------------------------------------------------------------
 * Sysfs interface
 * ---------------------------------------------------------------------------
 */

static int mtkfb_register_sysfs(struct mtkfb_device *fbdev)
{
    NOT_REFERENCED(fbdev);

    return 0;
}

static void mtkfb_unregister_sysfs(struct mtkfb_device *fbdev)
{
    NOT_REFERENCED(fbdev);
}

/*
 * ---------------------------------------------------------------------------
 * LDM callbacks
 * ---------------------------------------------------------------------------
 */
/* Initialize system fb_info object and set the default video mode.
 * The frame buffer memory already allocated by lcddma_init
 */
static int mtkfb_fbinfo_init(struct fb_info *info)
{
    struct mtkfb_device *fbdev = (struct mtkfb_device *)info->par;
    struct fb_var_screeninfo var;
    int r = 0;

    MSG_FUNC_ENTER();

    BUG_ON(!fbdev->fb_va_base);
    info->fbops = &mtkfb_ops;
    info->flags = FBINFO_FLAG_DEFAULT;
    info->screen_base = (char *) fbdev->fb_va_base;
    info->screen_size = fbdev->fb_size_in_byte;
    info->pseudo_palette = fbdev->pseudo_palette;

    r = fb_alloc_cmap(&info->cmap, 16, 0);
    if (r != 0)
        PRNERR("unable to allocate color map memory\n");

    // setup the initial video mode (RGB565)

    memset(&var, 0, sizeof(var));

    var.xres         = MTK_FB_XRES;
    var.yres         = MTK_FB_YRES;
    var.xres_virtual = MTK_FB_XRESV;
    var.yres_virtual = MTK_FB_YRESV;

    var.bits_per_pixel = 16;

    var.red.offset   = 11; var.red.length   = 5;
    var.green.offset =  5; var.green.length = 6;
    var.blue.offset  =  0; var.blue.length  = 5;

    var.width  = DISP_GetActiveWidth();
    var.height = DISP_GetActiveHeight();

    var.activate = FB_ACTIVATE_NOW;

    r = mtkfb_check_var(&var, info);
    if (r != 0)
        PRNERR("failed to mtkfb_check_var\n");

    info->var = var;

    r = mtkfb_set_par(info);
    if (r != 0)
        PRNERR("failed to mtkfb_set_par\n");

    MSG_FUNC_LEAVE();
    return r;
}

/* Release the fb_info object */
static void mtkfb_fbinfo_cleanup(struct mtkfb_device *fbdev)
{
    MSG_FUNC_ENTER();

    fb_dealloc_cmap(&fbdev->fb_info->cmap);

    MSG_FUNC_LEAVE();
}

static void mtkfb_lcd_complete_interrupt(void *param)
{
    if(atomic_read(&has_pending_update))
    {
        wake_up_interruptible(&screen_update_wq);
    }

#if defined(MTK_HDMI_SUPPORT)
    hdmi_source_buffer_switch();
    if(is_hdmi_active())
    {
        hdmi_update();
    }
#endif

}

static void mtkfb_dpi_vsync_interrupt(void *param)
{
#if defined(MTK_HDMI_SUPPORT)
//    extern void hdmi_update_buffer_switch(void);
    hdmi_update_buffer_switch();
#endif
}

void mtkfb_disable_non_fb_layer(void)
{
    int id;
    unsigned int dirty = 0;
    for (id = 0; id < DDP_OVL_LAYER_MUN; id++)
    {
        if (cached_layer_config[id].layer_en == 0)
            continue;

        if (cached_layer_config[id].addr >= fb_pa &&
            cached_layer_config[id].addr < (fb_pa+DISP_GetVRamSize()))
            continue;

        DISP_LOG_PRINT(ANDROID_LOG_INFO, "LCD", "  disable(%d)\n", id);
        cached_layer_config[id].layer_en = 0;
        cached_layer_config[id].isDirty = true;
        dirty = 1;
    }
    if (dirty)
    {
        memset(mtkfb_fbi->screen_base, 0, DISP_GetVRamSize());
        mtkfb_pan_display_impl(&mtkfb_fbi->var, mtkfb_fbi);
    }
}

#if INIT_FB_AS_COLOR_BAR
static void fill_color(u8 *buffer,
                       u32 fillColor,
                       u8  bpp,
                       u32 linePitchInPixels,
                       u32 startX,
                       u32 startY,
                       u32 fillWidth,
                       u32 fillHeight)
{
    u32 linePitchInBytes = linePitchInPixels * bpp;
    u8 *linePtr = buffer + startY * linePitchInBytes + startX * bpp;
    s32 h;
    u32 i;
    u8 color[4];

    for(i = 0; i < bpp; ++ i)
    {
        color[i] = fillColor & 0xFF;
        fillColor >>= 8;
    }

    h = (s32)fillHeight;
    while (--h >= 0)
    {
        u8 *ptr = linePtr;
        s32 w = (s32)fillWidth;
        while (--w >= 0)
        {
            memcpy(ptr, color, bpp);
            ptr += bpp;
        }
        linePtr += linePitchInBytes;
    }
}
#endif

#define RGB565_TO_ARGB8888(x)   \
    ((((x) &   0x1F) << 3) |    \
     (((x) &  0x7E0) << 5) |    \
     (((x) & 0xF800) << 8) |    \
     (0xFF << 24)) // opaque

#if 0
static void copy_rect(u8 *dstBuffer,
                      const u8 *srcBuffer,
                      u8  dstBpp,
                      u8  srcBpp,
                      u32 dstPitchInPixels,
                      u32 srcPitchInPixels,
                      u32 dstX,
                      u32 dstY,
                      u32 copyWidth,
                      u32 copyHeight)
{
    u32 dstPitchInBytes  = dstPitchInPixels * dstBpp;
    u32 srcPitchInBytes  = srcPitchInPixels * srcBpp;
    u8 *dstPtr = dstBuffer + dstY * dstPitchInBytes + dstX * dstBpp;
    const u8 *srcPtr = srcBuffer;
    s32 h = (s32)copyHeight;

    if (dstBpp == srcBpp)
    {
        u32 copyBytesPerLine = copyWidth * dstBpp;

        while (--h >= 0)
        {
            memcpy(dstPtr, srcPtr, copyBytesPerLine);
            dstPtr += dstPitchInBytes;
            srcPtr += srcPitchInBytes;
        }
    }
    else if (srcBpp == 2 && dstBpp == 4)    // RGB565 copy to ARGB8888
    {
        while (--h >= 0)
        {
            const u16 *s = (const u16 *)srcPtr;
            u32 *d = (u32 *)dstPtr;
            s32 w = (s32)copyWidth;

            while (--w >= 0)
            {
                u16 rgb565 = *s;
                *d = RGB565_TO_ARGB8888(rgb565);
                ++ d; ++ s;
            }

            dstPtr += dstPitchInBytes;
            srcPtr += srcPitchInBytes;
        }
    }
    else
    {
        MTKFB_LOG("un-supported bpp in copy_rect(), srcBpp: %d --> dstBpp: %d\n",
               srcBpp, dstBpp);

        ASSERT(0);
    }
}
#endif

/* Init frame buffer content as 3 R/G/B color bars for debug */
static int init_framebuffer(struct fb_info *info)
{
    void *buffer = info->screen_base +
                   info->var.yoffset * info->fix.line_length;

    u32 bpp = (info->var.bits_per_pixel + 7) >> 3;

#if INIT_FB_AS_COLOR_BAR
    int i;

    u32 colorRGB565[] =
    {
        0xffff, // White
        0xf800, // Red
        0x07e0, // Green
        0x001f, // Blue
    };

    u32 xSteps[ARY_SIZE(colorRGB565) + 1];

    xSteps[0] = 0;
    xSteps[1] = info->var.xres / 4 * 1;
    xSteps[2] = info->var.xres / 4 * 2;
    xSteps[3] = info->var.xres / 4 * 3;
    xSteps[4] = info->var.xres;

    for(i = 0; i < ARY_SIZE(colorRGB565); ++ i)
    {
        fill_color(buffer,
                   colorRGB565[i],
                   bpp,
                   info->var.xres,
                   xSteps[i],
                   0,
                   xSteps[i+1] - xSteps[i],
                   info->var.yres);
    }
#else
    // clean whole frame buffer as black
    memset(buffer, 0, info->screen_size);

#endif
    return 0;
}


/* Free driver resources. Can be called to rollback an aborted initialization
 * sequence.
 */
static void mtkfb_free_resources(struct mtkfb_device *fbdev, int state)
{
    int r = 0;

    switch (state) {
    case MTKFB_ACTIVE:
        r = unregister_framebuffer(fbdev->fb_info);
        ASSERT(0 == r);
      //lint -fallthrough
    case 5:
        mtkfb_unregister_sysfs(fbdev);
      //lint -fallthrough
    case 4:
        mtkfb_fbinfo_cleanup(fbdev);
      //lint -fallthrough
    case 3:
        DISP_CHECK_RET(DISP_Deinit());
      //lint -fallthrough
    case 2:
        dma_free_coherent(0, fbdev->fb_size_in_byte,
                          fbdev->fb_va_base, fbdev->fb_pa_base);
      //lint -fallthrough
    case 1:
        dev_set_drvdata(fbdev->dev, NULL);
        framebuffer_release(fbdev->fb_info);
      //lint -fallthrough
    case 0:
      /* nothing to free */
        break;
    default:
        BUG();
    }
}

extern char* saved_command_line;
char mtkfb_lcm_name[256] = {0};
BOOL mtkfb_find_lcm_driver(void)
{
	BOOL ret = FALSE;
	char *p, *q;

	p = strstr(saved_command_line, "lcm=");
	if(p == NULL)
	{
		// we can't find lcm string in the command line, the uboot should be old version
		return DISP_SelectDevice(NULL);
	}

	p += 4;
	if((p - saved_command_line) > strlen(saved_command_line+1))
	{
		ret = FALSE;
		goto done;
	}

	printk("%s, %s\n", __func__, p);
	q = p;
	while(*q != ' ' && *q != '\0')
		q++;

	memset((void*)mtkfb_lcm_name, 0, sizeof(mtkfb_lcm_name));
	strncpy((char*)mtkfb_lcm_name, (const char*)p, (int)(q-p));

	printk("%s, %s\n", __func__, mtkfb_lcm_name);
	if(DISP_SelectDevice(mtkfb_lcm_name))
		ret = TRUE;

done:
	return ret;
}

void disp_get_fb_address(UINT32 *fbVirAddr, UINT32 *fbPhysAddr)
{
    struct mtkfb_device  *fbdev = (struct mtkfb_device *)mtkfb_fbi->par;

    *fbVirAddr = (UINT32)fbdev->fb_va_base + mtkfb_fbi->var.yoffset * mtkfb_fbi->fix.line_length;
    *fbPhysAddr =(UINT32)fbdev->fb_pa_base + mtkfb_fbi->var.yoffset * mtkfb_fbi->fix.line_length;
}

static int mtkfb_fbinfo_modify(struct fb_info *info)
{
    struct mtkfb_device *fbdev = (struct mtkfb_device *)info->par;
    struct fb_var_screeninfo var;
	int r = 0;

    memcpy(&var, &(info->var), sizeof(var));
	var.activate		= FB_ACTIVATE_NOW;
	var.bits_per_pixel  = 32;
	var.transp.offset	= 24;
	var.transp.length	= 8;
	var.red.offset 	    = 16; var.red.length 	= 8;
	var.green.offset    = 8;  var.green.length	= 8;
	var.blue.offset	    = 0;  var.blue.length	= 8;
	var.yoffset         = var.yres;

    r = mtkfb_check_var(&var, info);
    if (r != 0)
        PRNERR("failed to mtkfb_check_var\n");

    info->var = var;

    r = mtkfb_set_par(info);
    if (r != 0)
        PRNERR("failed to mtkfb_set_par\n");

    return r;
}

static void mtkfb_fb_565_to_8888(struct fb_info *fb_info)
{
	unsigned int xres = fb_info->var.xres;
	unsigned int yres = fb_info->var.yres;
	unsigned int x_virtual = ALIGN_TO(xres, 32);
	unsigned int fbsize = x_virtual * yres * 2;

	unsigned short *s = fb_info->screen_base;
	unsigned int   *d = fb_info->screen_base + fbsize * 2;
	unsigned short src_rgb565 = 0;
	int j = 0;
	int k = 0;
	PRNERR("mtkfb_fb_565_to_8888 xres=%d yres=%d fbsize=0x%X x_virtual=%d s=0x%08X d=0x%08X\n",
	       xres, yres, fbsize, x_virtual, s, d);

	//printf("[boot_logo_updater]normal\n");
	for (j = 0; j < yres; ++ j){
		for(k = 0; k < xres; ++ k)
		{
			src_rgb565 = *s++;
			*d++ = RGB565_TO_ARGB8888(src_rgb565);
		}
		for(k = xres; k < x_virtual; ++ k){
			*d++ = 0xFFFFFFFF;
			*s++;
		}
	}
	//printf("[boot_logo_updater] loop copy color over\n");

	mtkfb_fbinfo_modify(fb_info);
	wait_event_interruptible_timeout(reg_update_wq, atomic_read(&OverlaySettingApplied), HZ/10);
	s = fb_info->screen_base;
	d = fb_info->screen_base + fbsize * 2;
	memcpy(s,d,fbsize*2);
}


/* Called by LDM binding to probe and attach a new device.
 * Initialization sequence:
 *   1. allocate system fb_info structure
 *      select panel type according to machine type
 *   2. init LCD panel
 *   3. init LCD controller and LCD DMA
 *   4. init system fb_info structure
 *   5. init gfx DMA
 *   6. enable LCD panel
 *      start LCD frame transfer
 *   7. register system fb_info structure
 */

static int mtkfb_probe(struct device *dev)
{
    struct platform_device *pdev;
    struct mtkfb_device    *fbdev = NULL;
    struct fb_info         *fbi;
    int                    init_state;
    int                    r = 0;
	char *p = NULL;
    MSG_FUNC_ENTER();

	if(get_boot_mode() == META_BOOT || get_boot_mode() == FACTORY_BOOT
	|| get_boot_mode() == ADVMETA_BOOT || get_boot_mode() == RECOVERY_BOOT)
		first_update = false;

	printk("%s, %s\n", __func__, saved_command_line);
	p = strstr(saved_command_line, "fps=");
	if(p == NULL){
		lcd_fps = 6000;
		printk("[FB driver]can not get fps from uboot\n");
	}
	else{
		p += 4;
		lcd_fps = simple_strtol(p, NULL, 10);
		if(0 == lcd_fps) lcd_fps = 6000;
	}
	printk("[FB driver] fps = %d\n", lcd_fps);
	if(DISP_IsContextInited() == FALSE)
	{
		if(mtkfb_find_lcm_driver())
		{
			printk("%s, we have found the lcm - %s\n", __func__, mtkfb_lcm_name);

			is_lcm_inited = TRUE;
		}
		else if(DISP_DetectDevice() != DISP_STATUS_OK)
		{
			printk("[mtkfb] detect device fail, maybe caused by the two reasons below:\n");
			printk("\t\t1.no lcm connected\n");
			printk("\t\t2.we can't support this lcm\n");
		}
	}
	else
	{
#ifndef MT65XX_NEW_DISP
		LCD_CHECK_RET(LCD_Init());
#endif
	}

	MTK_FB_XRES  = DISP_GetScreenWidth();
    MTK_FB_YRES  = DISP_GetScreenHeight();
	fb_xres_update = MTK_FB_XRES;
	fb_yres_update = MTK_FB_YRES;

	printk("[MTKFB] XRES=%d, YRES=%d\n", MTK_FB_XRES, MTK_FB_YRES);

    MTK_FB_BPP   = DISP_GetScreenBpp();
    MTK_FB_PAGES = DISP_GetPages();


    init_waitqueue_head(&screen_update_wq);

    //screen_update_task = kthread_create(
    //    screen_update_kthread, NULL, "screen_update_kthread");

    //if (IS_ERR(screen_update_task)) {
    //    return PTR_ERR(screen_update_task);
    //}
    //wake_up_process(screen_update_task);
#ifndef MT65XX_NEW_DISP
    {
        ///register LCD complete interrupt callback
        DISP_INTERRUPT_CALLBACK_STRUCT cbStruct;

        cbStruct.pFunc = mtkfb_lcd_complete_interrupt;
        cbStruct.pParam = NULL;
        /// regster callback
        if (DISP_STATUS_OK != DISP_SetInterruptCallback(DISP_LCD_TRANSFER_COMPLETE_INT, &cbStruct))
        {
            ASSERT(0);
        }
    }

    {
        ///register DPI VSYNC interrupt callback
        DISP_INTERRUPT_CALLBACK_STRUCT cbStruct;

        cbStruct.pFunc = mtkfb_dpi_vsync_interrupt;
        cbStruct.pParam = NULL;
        /// regster callback
        if (DISP_STATUS_OK != DISP_SetInterruptCallback(DISP_DPI_VSYNC_INT, &cbStruct))
        {
            ASSERT(0);
        }
    }
#endif

    if(DISP_EsdRecoverCapbility())
    {
        esd_recovery_task = kthread_create(
    			   esd_recovery_kthread, NULL, "esd_recovery_kthread");

        if (IS_ERR(esd_recovery_task)) {
            MTKFB_LOG("ESD recovery task create fail\n");
        }
        else {
        	wake_up_process(esd_recovery_task);
        }
    }
    init_state = 0;

    pdev = to_platform_device(dev);
    if (pdev->num_resources != 1) {
        PRNERR("probed for an unknown device\n");
        r = -ENODEV;
        goto cleanup;
    }

    fbi = framebuffer_alloc(sizeof(struct mtkfb_device), dev);
    if (!fbi) {
        PRNERR("unable to allocate memory for device info\n");
        r = -ENOMEM;
        goto cleanup;
    }
    mtkfb_fbi = fbi;

    fbdev = (struct mtkfb_device *)fbi->par;
    fbdev->fb_info = fbi;
    fbdev->dev = dev;
    dev_set_drvdata(dev, fbdev);

    init_state++;   // 1

    /* Allocate and initialize video frame buffer */

    fbdev->fb_size_in_byte = MTK_FB_SIZEV;
    {
        struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
        fbdev->fb_pa_base = res->start;
        fbdev->fb_va_base = ioremap_nocache(res->start, res->end - res->start + 1);
        ASSERT(DISP_GetVRamSize() <= (res->end - res->start + 1));
        if (mtkfb_enable_m4u)
        {
            m4u_alloc_mva(M4U_CLNTMOD_LCDC_UI, (unsigned int)fbdev->fb_pa_base, DISP_GetVRamSize(), 0, 0, &fb_pa);
            ASSERT(fb_pa);
            printk("[MTKFB] FB MVA is 0x%08X PA is 0x%08X\n", fb_pa, (unsigned int)fbdev->fb_pa_base);
        }
        else
        {
            fb_pa = fbdev->fb_pa_base;
        }
        //memset(fbdev->fb_va_base, 0, (res->end - res->start + 1));
    }
#if defined(MTK_M4U_SUPPORT)
	fb_va_m4u = fbdev->fb_pa_base;
	fb_size_m4u = fbdev->fb_size_in_byte;
	overlay_buffer_head = (struct fb_overlay_buffer_list*)vmalloc(sizeof(struct fb_overlay_buffer_list));
	if(!overlay_buffer_head){
		printk("[FB driver] vmalloc failed\n");
		r = -ENOMEM;
		goto cleanup;
	}
	overlay_buffer_head->next = NULL;
#endif

	printk("[FB Driver] fbdev->fb_pa_base = %x, fbdev->fb_va_base = %x\n", fbdev->fb_pa_base, (unsigned int)(fbdev->fb_va_base));

    if (!fbdev->fb_va_base) {
        PRNERR("unable to allocate memory for frame buffer\n");
        r = -ENOMEM;
        goto cleanup;
    }

    init_state++;   // 2

    /* Initialize Display Driver PDD Layer */

    if (DISP_STATUS_OK != DISP_Init((DWORD)fbdev->fb_va_base,
        (DWORD)fb_pa,
                                    is_lcm_inited))
    {
        r = -1;
        goto cleanup;
    }

	init_state++;   // 3

    /* Register to system */

    r = mtkfb_fbinfo_init(fbi);
    if (r)
        goto cleanup;
    init_state++;   // 4

    r = mtkfb_register_sysfs(fbdev);
    if (r)
        goto cleanup;
    init_state++;   // 5

    r = register_framebuffer(fbi);
    if (r != 0) {
        PRNERR("register_framebuffer failed\n");
        goto cleanup;
    }

    fbdev->state = MTKFB_ACTIVE;

    /********************************************/

    mtkfb_fb_565_to_8888(fbi);

    /********************************************/
    MSG(INFO, "MTK framebuffer initialized vram=%lu\n", fbdev->fb_size_in_byte);

	MSG_FUNC_LEAVE();
    return 0;

cleanup:
    mtkfb_free_resources(fbdev, init_state);

    MSG_FUNC_LEAVE();
    return r;
}

/* Called when the device is being detached from the driver */
static int mtkfb_remove(struct device *dev)
{
    struct mtkfb_device *fbdev = dev_get_drvdata(dev);
    enum mtkfb_state saved_state = fbdev->state;

    MSG_FUNC_ENTER();
    /* FIXME: wait till completion of pending events */

    fbdev->state = MTKFB_DISABLED;
    mtkfb_free_resources(fbdev, saved_state);

    MSG_FUNC_LEAVE();
    return 0;
}

/* PM suspend */
static int mtkfb_suspend(struct device *pdev, pm_message_t mesg)
{
    NOT_REFERENCED(pdev);
    MSG_FUNC_ENTER();
    MTKFB_LOG("[FB Driver] mtkfb_suspend(): 0x%x\n", mesg.event);
    MSG_FUNC_LEAVE();
    return 0;
}
bool mtkfb_is_suspend(void)
{
    return is_early_suspended;
}
EXPORT_SYMBOL(mtkfb_is_suspend);

static void mtkfb_shutdown(struct device *pdev)
{
    MTKFB_LOG("[FB Driver] mtkfb_shutdown()\n");
    mt65xx_leds_brightness_set(MT65XX_LED_TYPE_LCD, LED_OFF);
    if (!lcd_fps)
        msleep(30);
    else
        msleep(2*100000/lcd_fps); // Delay 2 frames.

	if(is_early_suspended){
		MTKFB_LOG("mtkfb has been power off\n");
		return;
	}

    if (down_interruptible(&sem_early_suspend)) {
        printk("[FB Driver] can't get semaphore in mtkfb_shutdown()\n");
        return;
    }
	sem_early_suspend_cnt--;

	is_early_suspended = TRUE;
	DISP_CHECK_RET(DISP_PanelEnable(FALSE));
 	DISP_CHECK_RET(DISP_PowerEnable(FALSE));

	DISP_CHECK_RET(DISP_PauseVsync(TRUE));
	sem_early_suspend_cnt++;
    up(&sem_early_suspend);

    MTKFB_LOG("[FB Driver] leave mtkfb_shutdown\n");
}

void mtkfb_clear_lcm(void)
{
	int i;
	int layer_status[LCD_LAYER_NUM]={0};
	for(i=0;i<LCD_LAYER_NUM;i++)
	{
		layer_status[i] = LCD_IsLayerEnable(i);
		LCD_LayerEnable(i, false);
	}

	LCD_SetBackgroundColor(0x00000000);

	DISP_CHECK_RET(DISP_UpdateScreen(0, 0, fb_xres_update, fb_yres_update));
	DISP_CHECK_RET(DISP_UpdateScreen(0, 0, fb_xres_update, fb_yres_update));
	DISP_WaitForLCDNotBusy();
	for(i=0;i<LCD_LAYER_NUM;i++)
	{
		LCD_LayerEnable(i, layer_status[i]);
	}
}


#ifdef CONFIG_HAS_EARLYSUSPEND
static void mtkfb_early_suspend(struct early_suspend *h)
{
//    struct mtkfb_device  *fbdev = (struct mtkfb_device *)mtkfb_fbi->par;
//    UINT32 fbVirAddr = (UINT32)fbdev->fb_va_base + mtkfb_fbi->var.yoffset * mtkfb_fbi->fix.line_length;

    MSG_FUNC_ENTER();

    printk("[FB Driver] enter early_suspend\n");
    mutex_lock(&ScreenCaptureMutex);

    mt65xx_leds_brightness_set(MT65XX_LED_TYPE_LCD, LED_OFF);
    if (down_interruptible(&sem_early_suspend)) {
        printk("[FB Driver] can't get semaphore in mtkfb_early_suspend()\n");
        mutex_unlock(&ScreenCaptureMutex);
        return;
    }

	sem_early_suspend_cnt--;
    //MMProfileLogEx(MTKFB_MMP_Events.EarlySuspend, MMProfileFlagStart, 0, 0);

	if(is_early_suspended){
		is_early_suspended = TRUE;
		sem_early_suspend_cnt++;
		up(&sem_early_suspend);
		MTKFB_LOG("[FB driver] has been suspended\n");
        mutex_unlock(&ScreenCaptureMutex);
		return;
	}


	//DISP_SetBacklight(0);

	//memset((void*)fbVirAddr, 0, MTK_FB_SIZE);
	//DISP_CHECK_RET(DISP_UpdateScreen(0, 0, fb_xres_update, fb_yres_update));
	//DISP_CHECK_RET(DISP_UpdateScreen(0, 0, fb_xres_update, fb_yres_update));
//	mtkfb_clear_lcm();

    MMProfileLog(MTKFB_MMP_Events.EarlySuspend, MMProfileFlagStart);
	is_early_suspended = TRUE;

    if (!lcd_fps)
        msleep(30);
    else
        msleep(2*100000/lcd_fps); // Delay 2 frames.

    DISP_PrepareSuspend();
    // Wait for disp finished.
    if (wait_event_interruptible_timeout(disp_done_wq, !disp_running, HZ/10) == 0)
    {
        printk("[FB Driver] Wait disp finished timeout in early_suspend\n");
    }
	DISP_CHECK_RET(DISP_PanelEnable(FALSE));
 	DISP_CHECK_RET(DISP_PowerEnable(FALSE));

	DISP_CHECK_RET(DISP_PauseVsync(TRUE));
	#ifdef MT65XX_NEW_DISP
	disp_path_clock_off("mtkfb");
	#endif

    //MMProfileLogEx(MTKFB_MMP_Events.EarlySuspend, MMProfileFlagEnd, 0, 0);
	sem_early_suspend_cnt++;
    up(&sem_early_suspend);
    mutex_unlock(&ScreenCaptureMutex);

    printk("[FB Driver] leave early_suspend\n");

    MSG_FUNC_LEAVE();
}
#endif

/* PM resume */
static int mtkfb_resume(struct device *pdev)
{
    NOT_REFERENCED(pdev);
    MSG_FUNC_ENTER();
    MTKFB_LOG("[FB Driver] mtkfb_resume()\n");
    MSG_FUNC_LEAVE();
    return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mtkfb_late_resume(struct early_suspend *h)
{
//    struct mtkfb_device  *fbdev = (struct mtkfb_device *)mtkfb_fbi->par;

//    UINT32 fbVirAddr = (UINT32)fbdev->fb_va_base + mtkfb_fbi->var.yoffset * mtkfb_fbi->fix.line_length;

    MSG_FUNC_ENTER();

    printk("[FB Driver] enter late_resume\n");
#if 0
    if (is_ipoh_bootup)
    {
        MMProfileLogEx(MTKFB_MMP_Events.EarlySuspend, MMProfileFlagEnd, 1, 0);
        DISP_CHECK_RET(DISP_PowerEnable(TRUE));
        //DISP_CHECK_RET(DISP_PanelEnable(TRUE));
        is_early_suspended = FALSE;
        is_lcm_in_suspend_mode = FALSE;
        is_engine_in_suspend_mode = FALSE;
        atomic_set(&OverlaySettingDirtyFlag, 0);
        DISP_StartConfigUpdate();
        printk("[FB Driver] leave late_resume after ipoh\n");
        return;
    }
#endif
    mutex_lock(&ScreenCaptureMutex);
    if (down_interruptible(&sem_early_suspend)) {
        printk("[FB Driver] can't get semaphore in mtkfb_late_resume()\n");
        mutex_unlock(&ScreenCaptureMutex);
        return;
    }
	sem_early_suspend_cnt--;
    //MMProfileLogEx(MTKFB_MMP_Events.EarlySuspend, MMProfileFlagStart, 0, 0);

    MMProfileLog(MTKFB_MMP_Events.EarlySuspend, MMProfileFlagEnd);
#ifdef MT65XX_NEW_DISP
    if (is_ipoh_bootup)
    {
        atomic_set(&OverlaySettingDirtyFlag, 0);
	    disp_path_clock_on("ipoh_mtkfb");
	}
	else
	disp_path_clock_on("mtkfb");
#endif
    printk("[FB LR] 1\n");
	DISP_CHECK_RET(DISP_PauseVsync(FALSE));
	printk("[FB LR] 2\n");
    DISP_CHECK_RET(DISP_PowerEnable(TRUE));
    printk("[FB LR] 3\n");
    DISP_CHECK_RET(DISP_PanelEnable(TRUE));
    printk("[FB LR] 4\n");

	is_early_suspended = FALSE;

//	DISP_SetBacklight(0);
#if 0
//	memset(fbVirAddr, 0, MTK_FB_SIZE);
	DISP_CHECK_RET(DISP_UpdateScreen(0, 0, fb_xres_update, fb_yres_update));
	DISP_CHECK_RET(DISP_UpdateScreen(0, 0, fb_xres_update, fb_yres_update));
	DISP_WaitForLCDNotBusy();
#endif
    if (is_ipoh_bootup)
    {
    	DISP_StartConfigUpdate();
    }
    else
    {
	    mtkfb_clear_lcm();
	  }

    //MMProfileLogEx(MTKFB_MMP_Events.EarlySuspend, MMProfileFlagEnd, 0, 0);
	sem_early_suspend_cnt++;
    up(&sem_early_suspend);
    mutex_unlock(&ScreenCaptureMutex);

	if(BL_set_level_resume){
		mtkfb_set_backlight_level(BL_level);
		BL_set_level_resume = FALSE;
		}

    printk("[FB Driver] leave late_resume\n");

    MSG_FUNC_LEAVE();
}
#endif

/*---------------------------------------------------------------------------*/
#ifdef CONFIG_PM
/*---------------------------------------------------------------------------*/
int mtkfb_pm_suspend(struct device *device)
{
    //pr_debug("calling %s()\n", __func__);

    struct platform_device *pdev = to_platform_device(device);
    BUG_ON(pdev == NULL);

    return mtkfb_suspend(pdev, PMSG_SUSPEND);
}

int mtkfb_pm_resume(struct device *device)
{
    //pr_debug("calling %s()\n", __func__);

    struct platform_device *pdev = to_platform_device(device);
    BUG_ON(pdev == NULL);

    return mtkfb_resume(pdev);
}

extern void mt_irq_set_sens(unsigned int irq, unsigned int sens);
extern void mt_irq_set_polarity(unsigned int irq, unsigned int polarity);
void MMProfileStart(int start);
int mtkfb_pm_restore_noirq(struct device *device)
{

    MMProfileStart(0);
    MMProfileStart(1);
    // DPI0
    mt_irq_set_sens(MT6589_DISP_DPI0_IRQ_ID, MT65xx_LEVEL_SENSITIVE);
    mt_irq_set_polarity(MT6589_DISP_DPI0_IRQ_ID, MT65xx_POLARITY_LOW);

    // DPI1
    mt_irq_set_sens(MT6589_DISP_DPI1_IRQ_ID, MT65xx_LEVEL_SENSITIVE);
    mt_irq_set_polarity(MT6589_DISP_DPI1_IRQ_ID, MT65xx_POLARITY_LOW);

    // DSI
    mt_irq_set_sens(MT6589_DISP_DSI_IRQ_ID, MT65xx_LEVEL_SENSITIVE);
    mt_irq_set_polarity(MT6589_DISP_DSI_IRQ_ID, MT65xx_POLARITY_LOW);

    // LCD
    mt_irq_set_sens(MT6589_DISP_DBI_IRQ_ID, MT65xx_LEVEL_SENSITIVE);
    mt_irq_set_polarity(MT6589_DISP_DBI_IRQ_ID, MT65xx_POLARITY_LOW);

    is_ipoh_bootup = true;
    return 0;

}
/*---------------------------------------------------------------------------*/
#else /*CONFIG_PM*/
/*---------------------------------------------------------------------------*/
#define mtkfb_pm_suspend NULL
#define mtkfb_pm_resume  NULL
#define mtkfb_pm_restore_noirq NULL
/*---------------------------------------------------------------------------*/
#endif /*CONFIG_PM*/
/*---------------------------------------------------------------------------*/
struct dev_pm_ops mtkfb_pm_ops = {
    .suspend = mtkfb_pm_suspend,
    .resume = mtkfb_pm_resume,
    .freeze = mtkfb_pm_suspend,
    .thaw = mtkfb_pm_resume,
    .poweroff = mtkfb_pm_suspend,
    .restore = mtkfb_pm_resume,
    .restore_noirq = mtkfb_pm_restore_noirq,
};

static struct platform_driver mtkfb_driver =
{
    .driver = {
        .name    = MTKFB_DRIVER,
#ifdef CONFIG_PM
        .pm     = &mtkfb_pm_ops,
#endif
        .bus     = &platform_bus_type,
        .probe   = mtkfb_probe,
        .remove  = mtkfb_remove,
        .suspend = mtkfb_suspend,
        .resume  = mtkfb_resume,
		.shutdown = mtkfb_shutdown,
    },
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend mtkfb_early_suspend_handler =
{
	.level = EARLY_SUSPEND_LEVEL_DISABLE_FB,
	.suspend = mtkfb_early_suspend,
	.resume = mtkfb_late_resume,
};
#endif

#ifdef DEFAULT_MMP_ENABLE
void MMProfileEnable(int enable);
void MMProfileStart(int start);
void init_mtkfb_mmp_events(void);
void init_ddp_mmp_events(void);
#endif

/* Register both the driver and the device */
int __init mtkfb_init(void)
{
    int r = 0;

    MSG_FUNC_ENTER();

#ifdef DEFAULT_MMP_ENABLE
    MMProfileEnable(1);
	init_mtkfb_mmp_events();
    init_ddp_mmp_events();
    MMProfileStart(1);
#endif

    /* Register the driver with LDM */

    if (platform_driver_register(&mtkfb_driver)) {
        PRNERR("failed to register mtkfb driver\n");
        r = -ENODEV;
        goto exit;
    }

#ifdef CONFIG_HAS_EARLYSUSPEND
   	register_early_suspend(&mtkfb_early_suspend_handler);
#endif

    DBG_Init();

exit:
    MSG_FUNC_LEAVE();
    return r;
}


static void __exit mtkfb_cleanup(void)
{
    MSG_FUNC_ENTER();

    platform_driver_unregister(&mtkfb_driver);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&mtkfb_early_suspend_handler);
#endif

    kthread_stop(screen_update_task);
    if(esd_recovery_task)
         kthread_stop(esd_recovery_task);

    DBG_Deinit();

    MSG_FUNC_LEAVE();
}


module_init(mtkfb_init);
module_exit(mtkfb_cleanup);

MODULE_DESCRIPTION("MEDIATEK framebuffer driver");
MODULE_AUTHOR("Zaikuo Wang <zaikuo.wang@mediatek.com>");
MODULE_LICENSE("GPL");
