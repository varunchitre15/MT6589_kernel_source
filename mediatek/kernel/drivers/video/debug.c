#include <linux/string.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/fb.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/debugfs.h>
#include <linux/wait.h>

#include <disp_drv_platform.h>
#include "disp_drv_log.h"

#include "lcd_drv.h"
#include "lcd_reg.h"
#include "dpi_drv.h"
#include "dpi1_drv.h"
#include "dpi_reg.h"
#include "dsi_drv.h"
#include "dsi_reg.h"
#include "mtkfb.h"

#if defined(MTK_TVOUT_SUPPORT)
#include "tv_out.h"
#include "tvc_drv.h"
#include "tve_drv.h"
#include "tvrot_drv.h"
#endif

#include "debug.h"
#include "lcm_drv.h"

struct MTKFB_MMP_Events_t MTKFB_MMP_Events;

extern LCM_DRIVER *lcm_drv;
extern LCM_PARAMS *lcm_params;
extern unsigned int EnableVSyncLog;

#define MTKFB_DEBUG_FS_CAPTURE_LAYER_CONTENT_SUPPORT

///for debug purpose
static PLCD_REGS const LCD_REG = (PLCD_REGS)(LCD_BASE);


// ---------------------------------------------------------------------------
//  External variable declarations
// ---------------------------------------------------------------------------

extern long tpd_last_down_time;
extern int  tpd_start_profiling;
extern void mtkfb_log_enable(int enable);
extern void disp_log_enable(int enable);
extern void dbi_log_enable(int enable);
extern void DSI_Enable_Log(bool enable);
extern void Glitch_Enable_Log(bool enable);
extern void Glitch_times(unsigned int times);
extern void mtkfb_vsync_log_enable(int enable);
extern void mtkfb_m4u_switch(bool enable);
extern void mtkfb_m4u_dump(void);
extern void mtkfb_capture_fb_only(bool enable);
extern void esd_recovery_pause(BOOL en);
extern int mtkfb_set_backlight_mode(unsigned int mode);
extern void mtkfb_pan_disp_test(void);
extern void mtkfb_show_sem_cnt(void);
extern void mtkfb_hang_test(bool en);
extern void mtkfb_switch_normal_to_factory(void);
extern void mtkfb_switch_factory_to_normal(void);

extern unsigned int gCaptureLayerEnable;
extern unsigned int gCaptureLayerDownX;
extern unsigned int gCaptureLayerDownY;

extern unsigned int gCaptureOvlThreadEnable;
extern unsigned int gCaptureOvlDownX;
extern unsigned int gCaptureOvlDownY;
extern struct task_struct *captureovl_task;

extern unsigned int gCaptureFBEnable;
extern unsigned int gCaptureFBDownX;
extern unsigned int gCaptureFBDownY;
extern unsigned int gCaptureFBPeriod;
extern struct task_struct *capturefb_task;
extern struct wait_queue_head_t gCaptureFBWQ;

#if defined (MTK_TVOUT_SUPPORT)
bool capture_tv_buffer = false;
#endif


#ifdef MTKFB_DEBUG_FS_CAPTURE_LAYER_CONTENT_SUPPORT
struct dentry *mtkfb_layer_dbgfs[LCD_LAYER_NUM];

typedef struct {
    UINT32 layer_index;
    UINT32 working_buf;
    UINT32 working_size;
} MTKFB_LAYER_DBG_OPTIONS;

MTKFB_LAYER_DBG_OPTIONS mtkfb_layer_dbg_opt[LCD_LAYER_NUM];

#endif    
extern LCM_DRIVER *lcm_drv;
// ---------------------------------------------------------------------------
//  Debug Options
// ---------------------------------------------------------------------------

static const long int DEFAULT_LOG_FPS_WND_SIZE = 30;

typedef struct {
    unsigned int en_fps_log;
    unsigned int en_touch_latency_log;
    unsigned int log_fps_wnd_size;
    unsigned int force_dis_layers;
} DBG_OPTIONS;

static DBG_OPTIONS dbg_opt = {0};

static char STR_HELP[] =
    "\n"
    "USAGE\n"
    "        echo [ACTION]... > mtkfb\n"
    "\n"
    "ACTION\n"
    "        fps:[on|off]\n"
    "             enable fps and lcd update time log\n"
    "\n"
    "        tl:[on|off]\n"
    "             enable touch latency log\n"
    "\n"
    "        layer\n"
    "             dump lcd layer information\n"
    "\n"
    "        suspend\n"
    "             enter suspend mode\n"
    "\n"
    "        resume\n"
    "             leave suspend mode\n"
    "\n"
    "        lcm:[on|off|init]\n"
    "             power on/off lcm\n"
    "\n"
    "        cabc:[ui|mov|still]\n"
    "             cabc mode, UI/Moving picture/Still picture\n"
    "\n"
    "        lcd:[on|off]\n"
    "             power on/off display engine\n"
    "\n"
    "        te:[on|off]\n"
    "             turn on/off tearing-free control\n"
    "\n"
    "        tv:[on|off]\n"
    "             turn on/off tv-out\n"
    "\n"
    "        tvsys:[ntsc|pal]\n"
    "             switch tv system\n"
    "\n"
    "        reg:[lcd|dpi|dsi|tvc|tve]\n"
    "             dump hw register values\n"
    "\n"
    "        regw:addr=val\n"
    "             write hw register\n"
    "\n"
    "        regr:addr\n"
    "             read hw register\n"
    "\n"     
    "       cpfbonly:[on|off]\n"
    "             capture UI layer only on/off\n"
    "\n"     
    "       esd:[on|off]\n"
    "             esd kthread on/off\n"
    "       HQA:[NormalToFactory|FactoryToNormal]\n"
    "             for HQA requirement\n"
    "\n"     
    "       mmp\n"
    "             Register MMProfile events\n"
    ;


// ---------------------------------------------------------------------------
//  Information Dump Routines
// ---------------------------------------------------------------------------

void init_mtkfb_mmp_events(void)
{
    if (MTKFB_MMP_Events.MTKFB == 0)
    {
        MTKFB_MMP_Events.MTKFB = MMProfileRegisterEvent(MMP_RootEvent, "MTKFB");
        MTKFB_MMP_Events.PanDisplay = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "PanDisplay");
        MTKFB_MMP_Events.SetOverlayLayer = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "SetOverlayLayer");
        MTKFB_MMP_Events.SetOverlayLayers = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "SetOverlayLayers");
        MTKFB_MMP_Events.TrigOverlayOut = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "TrigOverlayOut");
        MTKFB_MMP_Events.UpdateScreenImpl = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "UpdateScreenImpl");
        MTKFB_MMP_Events.VSync = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "VSync");
        MTKFB_MMP_Events.UpdateConfig = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "UpdateConfig");
        MTKFB_MMP_Events.EsdCheck = MMProfileRegisterEvent(MTKFB_MMP_Events.UpdateConfig, "EsdCheck");
        MTKFB_MMP_Events.ConfigOVL = MMProfileRegisterEvent(MTKFB_MMP_Events.UpdateConfig, "ConfigOVL");
        MTKFB_MMP_Events.ConfigAAL = MMProfileRegisterEvent(MTKFB_MMP_Events.UpdateConfig, "ConfigAAL");
        MTKFB_MMP_Events.ConfigMemOut = MMProfileRegisterEvent(MTKFB_MMP_Events.UpdateConfig, "ConfigMemOut");
        MTKFB_MMP_Events.ScreenUpdate = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "ScreenUpdate");
        MTKFB_MMP_Events.CaptureFramebuffer = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "CaptureFB");
        MTKFB_MMP_Events.RegUpdate = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "RegUpdate");
        MTKFB_MMP_Events.EarlySuspend = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "EarlySuspend");
        MTKFB_MMP_Events.DispDone = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "DispDone");
        MTKFB_MMP_Events.DSICmd = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "DSICmd");
        MTKFB_MMP_Events.DSIIRQ = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "DSIIrq");
        MTKFB_MMP_Events.WaitVSync = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "WaitVSync");
        MTKFB_MMP_Events.LayerDump = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "LayerDump");
        MTKFB_MMP_Events.Layer[0] = MMProfileRegisterEvent(MTKFB_MMP_Events.LayerDump, "Layer0");
        MTKFB_MMP_Events.Layer[1] = MMProfileRegisterEvent(MTKFB_MMP_Events.LayerDump, "Layer1");
        MTKFB_MMP_Events.Layer[2] = MMProfileRegisterEvent(MTKFB_MMP_Events.LayerDump, "Layer2");
        MTKFB_MMP_Events.Layer[3] = MMProfileRegisterEvent(MTKFB_MMP_Events.LayerDump, "Layer3");
        MTKFB_MMP_Events.OvlDump = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "OvlDump");
        MTKFB_MMP_Events.FBDump = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "FBDump");
        MTKFB_MMP_Events.DSIRead = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "DSIRead");
        MTKFB_MMP_Events.GetLayerInfo = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "GetLayerInfo");
        MTKFB_MMP_Events.LayerInfo[0] = MMProfileRegisterEvent(MTKFB_MMP_Events.GetLayerInfo, "LayerInfo0");
        MTKFB_MMP_Events.LayerInfo[1] = MMProfileRegisterEvent(MTKFB_MMP_Events.GetLayerInfo, "LayerInfo1");
        MTKFB_MMP_Events.LayerInfo[2] = MMProfileRegisterEvent(MTKFB_MMP_Events.GetLayerInfo, "LayerInfo2");
        MTKFB_MMP_Events.LayerInfo[3] = MMProfileRegisterEvent(MTKFB_MMP_Events.GetLayerInfo, "LayerInfo3");
        MTKFB_MMP_Events.Debug = MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "Debug");
        MMProfileEnableEventRecursive(MTKFB_MMP_Events.MTKFB, 1);
    }
}

static __inline int is_layer_enable(unsigned int roi_ctl, unsigned int layer)
{
    return (roi_ctl >> (31 - layer)) & 0x1;
}


static __inline const char *narrate_lcd_layer_format(unsigned int format)
{
    ///TODO: if we have a new format support, should wrap with HW feature option, such as, MTK_HW_LCD_SUPPORT_RGBXXX_LAYER_FORMAT
    switch(format)
    {

        case LCD_LAYER_FORMAT_RGB565    : return "RGB565";
        case LCD_LAYER_FORMAT_YUYV422   : return "YUYV422";
        case LCD_LAYER_FORMAT_RGB888    : return "RGB888";
        case LCD_LAYER_FORMAT_ARGB8888  : return "ARGB8888";
        case LCD_LAYER_FORMAT_PARGB8888 : return "PARGB8888";
        case LCD_LAYER_FORMAT_xRGB8888  : return "xRGB8888";
        default : ASSERT(0);
    }
}


static void dump_lcd_layer_info(void)
{
#ifndef MT65XX_NEW_DISP
    unsigned int roi_ctl = AS_UINT32(&LCD_REG->WROI_CONTROL);

    unsigned int i = 0;

    DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", 
           "------------------------------------------\n"
           "[mtkfb] dump lcd layer information\n"
           "------------------------------------------\n");

    for (i = 0; i < 6; ++ i)
    {
        unsigned int layer_en = is_layer_enable(roi_ctl, i);
        
        DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "Layer[%d] is %s\n", i, layer_en ? "enabled" : "disabled");
        if (!layer_en) continue;

        DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "   size   : %d x %d\n", LCD_REG->LAYER[i].SIZE.WIDTH,
                                        LCD_REG->LAYER[i].SIZE.HEIGHT);
        DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "   offset : (%d, %d)\n", LCD_REG->LAYER[i].OFFSET.X,
                                         LCD_REG->LAYER[i].OFFSET.Y);
        DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "   format : %s\n", 
               narrate_lcd_layer_format(LCD_REG->LAYER[i].CONTROL.CLRDPT));
        DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "   dlink  : %d\n", 
               (AS_UINT32(&LCD_REG->WROI_DC) >> (31 - i)) & 0x1);
    }

    DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "\n");
#else
	LCD_DumpLayer();
#endif
}


// ---------------------------------------------------------------------------
//  FPS Log
// ---------------------------------------------------------------------------

typedef struct {
    long int current_lcd_time_us;
    long int current_te_delay_time_us;
    long int total_lcd_time_us;
    long int total_te_delay_time_us;
    long int start_time_us;
    long int trigger_lcd_time_us;
    unsigned int trigger_lcd_count;

    long int current_hdmi_time_us;
    long int total_hdmi_time_us;
    long int hdmi_start_time_us;
    long int trigger_hdmi_time_us;
    unsigned int trigger_hdmi_count;
} FPS_LOGGER;

static FPS_LOGGER fps = {0};
static FPS_LOGGER hdmi_fps = {0};

static long int get_current_time_us(void)
{
    struct timeval t;
    do_gettimeofday(&t);
    return (t.tv_sec & 0xFFF) * 1000000 + t.tv_usec;
}


static void __inline reset_fps_logger(void)
{
    memset(&fps, 0, sizeof(fps));
}

static void __inline reset_hdmi_fps_logger(void)
{
    memset(&hdmi_fps, 0, sizeof(hdmi_fps));
}

void DBG_OnTriggerLcd(void)
{
    if (!dbg_opt.en_fps_log && !dbg_opt.en_touch_latency_log) return;
    
    fps.trigger_lcd_time_us = get_current_time_us();
    if (fps.trigger_lcd_count == 0) {
        fps.start_time_us = fps.trigger_lcd_time_us;
    }
}

void DBG_OnTriggerHDMI(void)
{
    if (!dbg_opt.en_fps_log && !dbg_opt.en_touch_latency_log) return;
    
    hdmi_fps.trigger_hdmi_time_us = get_current_time_us();
    if (hdmi_fps.trigger_hdmi_count == 0) {
        hdmi_fps.hdmi_start_time_us = hdmi_fps.trigger_hdmi_time_us;
    }
}

void DBG_OnTeDelayDone(void)
{
    long int time;
    
    if (!dbg_opt.en_fps_log && !dbg_opt.en_touch_latency_log) return;

    time = get_current_time_us();
    fps.current_te_delay_time_us = (time - fps.trigger_lcd_time_us);
    fps.total_te_delay_time_us += fps.current_te_delay_time_us;
}


void DBG_OnLcdDone(void)
{
    long int time;

    if (!dbg_opt.en_fps_log && !dbg_opt.en_touch_latency_log) return;

    // deal with touch latency log

    time = get_current_time_us();
    fps.current_lcd_time_us = (time - fps.trigger_lcd_time_us);

#if 0   // FIXME
    if (dbg_opt.en_touch_latency_log && tpd_start_profiling) {

        DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "Touch Latency: %ld ms\n", 
               (time - tpd_last_down_time) / 1000);

        DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "LCD update time %ld ms (TE delay %ld ms + LCD %ld ms)\n",
               fps.current_lcd_time_us / 1000,
               fps.current_te_delay_time_us / 1000,
               (fps.current_lcd_time_us - fps.current_te_delay_time_us) / 1000);
        
        tpd_start_profiling = 0;
    }
#endif

    if (!dbg_opt.en_fps_log) return;

    // deal with fps log
        
    fps.total_lcd_time_us += fps.current_lcd_time_us;
    ++ fps.trigger_lcd_count;

    if (fps.trigger_lcd_count >= dbg_opt.log_fps_wnd_size) {

        long int f = fps.trigger_lcd_count * 100 * 1000 * 1000 
                     / (time - fps.start_time_us);

        long int update = fps.total_lcd_time_us * 100 
                          / (1000 * fps.trigger_lcd_count);

        long int te = fps.total_te_delay_time_us * 100 
                      / (1000 * fps.trigger_lcd_count);

        long int lcd = (fps.total_lcd_time_us - fps.total_te_delay_time_us) * 100
                       / (1000 * fps.trigger_lcd_count);

        DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "MTKFB FPS: %ld.%02ld, Avg. update time: %ld.%02ld ms "
               "(TE delay %ld.%02ld ms, LCD %ld.%02ld ms)\n",
               f / 100, f % 100,
               update / 100, update % 100,
               te / 100, te % 100,
               lcd / 100, lcd % 100);
		reset_fps_logger();
	}
}

void DBG_OnHDMIDone(void)
{
    long int time;

    if (!dbg_opt.en_fps_log && !dbg_opt.en_touch_latency_log) return;

    // deal with touch latency log

    time = get_current_time_us();
    hdmi_fps.current_hdmi_time_us = (time - hdmi_fps.trigger_hdmi_time_us);


    if (!dbg_opt.en_fps_log) return;

    // deal with fps log
        
    hdmi_fps.total_hdmi_time_us += hdmi_fps.current_hdmi_time_us;
    ++ hdmi_fps.trigger_hdmi_count;

    if (hdmi_fps.trigger_hdmi_count >= dbg_opt.log_fps_wnd_size) {

        long int f = hdmi_fps.trigger_hdmi_count * 100 * 1000 * 1000 
                     / (time - hdmi_fps.hdmi_start_time_us);

        long int update = hdmi_fps.total_hdmi_time_us * 100 
                          / (1000 * hdmi_fps.trigger_hdmi_count);

        DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "[HDMI] FPS: %ld.%02ld, Avg. update time: %ld.%02ld ms\n",
               f / 100, f % 100,
               update / 100, update % 100);
        
        reset_hdmi_fps_logger();
    }
}

// ---------------------------------------------------------------------------
//  Command Processor
// ---------------------------------------------------------------------------
extern void mtkfb_clear_lcm(void);
extern void hdmi_force_init(void);

static void process_dbg_opt(const char *opt)
{
    if (0 == strncmp(opt, "hdmion", 6))
    {
	hdmi_force_init();
    }
    else if (0 == strncmp(opt, "fps:", 4))
    {
        if (0 == strncmp(opt + 4, "on", 2)) {
            dbg_opt.en_fps_log = 1;
        } else if (0 == strncmp(opt + 4, "off", 3)) {
            dbg_opt.en_fps_log = 0;
        } else {
            goto Error;
        }
        reset_fps_logger();
    }
    else if (0 == strncmp(opt, "tl:", 3))
    {
        if (0 == strncmp(opt + 3, "on", 2)) {
            dbg_opt.en_touch_latency_log = 1;
        } else if (0 == strncmp(opt + 3, "off", 3)) {
            dbg_opt.en_touch_latency_log = 0;
        } else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "black", 5))
    {
	mtkfb_clear_lcm();
    }
    else if (0 == strncmp(opt, "suspend", 4))
    {
        DISP_PanelEnable(FALSE);
        DISP_PowerEnable(FALSE);
    }
    else if (0 == strncmp(opt, "resume", 4))
    {
        DISP_PowerEnable(TRUE);
        DISP_PanelEnable(TRUE);
    }
//<2013/02/10-21805-stevenchen, Add ADB commands to turn on/off LCM.
    else if (0 == strncmp(opt, "poweron", 7))
    {
        DISP_PowerEnable(TRUE);
        DISP_PanelEnable(TRUE);
	DISP_PanelOnOff(TRUE);
    }
    else if (0 == strncmp(opt, "poweroff", 8))
    {
        DISP_PowerEnable(FALSE);
        DISP_PanelEnable(FALSE);
	DISP_PanelOnOff(FALSE);
    }
//>2013/02/10-21805-stevenchen
    else if (0 == strncmp(opt, "lcm:", 4))
    {
        if (0 == strncmp(opt + 4, "on", 2)) {
            DISP_PanelEnable(TRUE);
        } else if (0 == strncmp(opt + 4, "off", 3)) {
            DISP_PanelEnable(FALSE);
        }
		else if (0 == strncmp(opt + 4, "init", 4)) {
			if (NULL != lcm_drv && NULL != lcm_drv->init) {
        		lcm_drv->init();
    		}
        }else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "cabc:", 5))
    {
        if (0 == strncmp(opt + 5, "ui", 2)) {
			mtkfb_set_backlight_mode(1);
        }else if (0 == strncmp(opt + 5, "mov", 3)) {
			mtkfb_set_backlight_mode(3);
        }else if (0 == strncmp(opt + 5, "still", 5)) {
			mtkfb_set_backlight_mode(2);
        }else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "lcd:", 4))
    {
        if (0 == strncmp(opt + 4, "on", 2)) {
            DISP_PowerEnable(TRUE);
        } else if (0 == strncmp(opt + 4, "off", 3)) {
            DISP_PowerEnable(FALSE);
        } else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "dbi:", 4))
    {
        if (0 == strncmp(opt + 4, "on", 2)) {
            LCD_PowerOn();
        } else if (0 == strncmp(opt + 4, "off", 3)) {
            LCD_PowerOff();
        } else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "dpi:", 4))
    {
        if (0 == strncmp(opt + 4, "on", 2)) {
            DPI_PowerOn();
        } else if (0 == strncmp(opt + 4, "off", 3)) {
            DPI_PowerOff();
        } else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "dsi:", 4))
    {
        if (0 == strncmp(opt + 4, "on", 2)) {
            DSI_PowerOn();
        } else if (0 == strncmp(opt + 4, "off", 3)) {
            DSI_PowerOff();
        } else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "te:", 3))
    {
        if (0 == strncmp(opt + 3, "on", 2)) {
            LCD_TE_Enable(TRUE);
			DSI_TE_Enable(TRUE);
        } else if (0 == strncmp(opt + 3, "off", 3)) {
            LCD_TE_Enable(FALSE);
			DSI_TE_Enable(FALSE);
        } else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "vsynclog:", 9))
    {
        if (0 == strncmp(opt + 9, "on", 2))
        {
            EnableVSyncLog = 1;
        } else if (0 == strncmp(opt + 9, "off", 3)) 
        {
            EnableVSyncLog = 0;
        } else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "layer", 5))
    {
        dump_lcd_layer_info();
    }
    else if (0 == strncmp(opt, "reg:", 4))
    {
        if (0 == strncmp(opt + 4, "lcd", 3)) {
            LCD_DumpRegisters();
        }else if (0 == strncmp(opt + 4, "dpi1", 4)) {
            DPI1_DumpRegisters();
        }else if (0 == strncmp(opt + 4, "dpi", 3)) {
            DPI_DumpRegisters();
        } else if (0 == strncmp(opt + 4, "dsi", 3)) {
            DSI_DumpRegisters();
#if defined(MTK_TVOUT_SUPPORT)
        } else if (0 == strncmp(opt + 4, "tvc", 3)) {
            TVC_DumpRegisters();
        } else if (0 == strncmp(opt + 4, "tve", 3)) {
            TVE_DumpRegisters();
        } else if (0 == strncmp(opt + 4, "tvr", 3)) {
            TVR_DumpRegisters();
#endif		
        } else {
            goto Error;
        }
    }

#if defined(MTK_TVOUT_SUPPORT)
    else if (0 == strncmp(opt, "tv:", 3))
    {
        if (0 == strncmp(opt + 3, "on", 2)) {
            TVOUT_TvCablePlugIn();
        } else if (0 == strncmp(opt +3, "off", 3)) {
            TVOUT_TvCablePlugOut();
        } else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "tvforce:", 8))
    {
        if (0 == strncmp(opt + 8, "on", 2)) {
            TVOUT_TvCablePlugIn_Directly();
        } else if (0 == strncmp(opt +8, "off", 3)) {
            TVOUT_TvCablePlugOut_Directly();
        } else {
            goto Error;
        }
    }
	else if (0 == strncmp(opt, "tvsys:", 6))
    {
        if (0 == strncmp(opt + 6, "ntsc", 4)) {
            TVOUT_SetTvSystem(TVOUT_SYSTEM_NTSC);
        } else if (0 == strncmp(opt + 6, "pal", 3)) {
            TVOUT_SetTvSystem(TVOUT_SYSTEM_PAL);
        } else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "tvrot:", 6))
    {
        if (0 == strncmp(opt + 6, "on", 2)) {
            TVOUT_SetOrientation(TVOUT_ROT_270);
        } else if (0 == strncmp(opt + 6, "off", 3)) {
            TVOUT_SetOrientation(TVOUT_ROT_0);
        } else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "tvcb:", 5))
    {
        if (0 == strncmp(opt + 5, "on", 2)) {
            TVOUT_EnableColorBar(TRUE);
        } else if (0 == strncmp(opt + 5, "off", 3)) {
            TVOUT_EnableColorBar(FALSE);
        } else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "tvvm:", 5))
    {
        if (0 == strncmp(opt + 5, "on", 2)) {
            TVOUT_DisableVideoMode(false);
        } else if (0 == strncmp(opt + 5, "off", 3)) {
            TVOUT_DisableVideoMode(true);
        } else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "tvlog:", 6))
    {
        if (0 == strncmp(opt + 6, "on", 2)) {
            TVOUT_EnableLog(true);
        } else if (0 == strncmp(opt + 6, "off", 3)) {
            TVOUT_EnableLog(false);
        } else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "tvtime:", 7))
    {
        if (0 == strncmp(opt + 7, "on", 2)) {
            TVOUT_EnableTimeProfiling(true);
        } else if (0 == strncmp(opt + 7, "off", 3)) {
            TVOUT_EnableTimeProfiling(false);
        } else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "tvcap:", 6))
    {
        if (0 == strncmp(opt + 6, "on", 2)) {
            capture_tv_buffer = true;
        } else if (0 == strncmp(opt + 6, "off", 3)) {
            capture_tv_buffer = false;
        } else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "tvcam:", 6))
    {
        if (0 == strncmp(opt + 6, "on", 2)) {
            TVOUT_ForceClose();
        } else if (0 == strncmp(opt + 6, "off", 3)) {
            TVOUT_RestoreOpen();
        } else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "tvuser:", 7))
    {
        if (0 == strncmp(opt + 7, "on", 2)) {
            TVOUT_TurnOn(true);
        } else if (0 == strncmp(opt + 7, "off", 3)) {
            TVOUT_TurnOn(false);
        } else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "tvcoffset:", 10))
    {
        char *p = (char *)opt + 10;
        unsigned long offset = simple_strtoul(p, &p, 10);
        TVC_SetCheckLineOffset(offset);
    }
#endif
    else if (0 == strncmp(opt, "regw:", 5))
    {
        char *p = (char *)opt + 5;
        unsigned long addr = simple_strtoul(p, &p, 16);
        unsigned long val  = simple_strtoul(p + 1, &p, 16);

        if (addr) {
            OUTREG32(addr, val);
        } else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "regr:", 5))
    {
        char *p = (char *)opt + 5;
        unsigned int addr = (unsigned int) simple_strtoul(p, &p, 16);

        if (addr) {
            DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "Read register 0x%08x: 0x%08x\n", addr, INREG32(addr));
        } else {
            goto Error;
        }
    }
	else if(0 == strncmp(opt, "bkl:", 4))
	{
		char *p = (char *)opt + 4;
		unsigned int level = (unsigned int) simple_strtoul(p, &p, 10);
		DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "process_dbg_opt(), set backlight level = %d\n", level);
		DISP_SetBacklight(level);
	}
	else if(0 == strncmp(opt, "dither:", 7))
	{
		unsigned lrs, lgs, lbs, dbr, dbg, dbb;
		char *p = (char *)opt + 7;
		lrs = (unsigned int) simple_strtoul(p, &p, 16);
		p++;
		lgs = (unsigned int) simple_strtoul(p, &p, 16);
		p++;
		lbs = (unsigned int) simple_strtoul(p, &p, 16);
		p++;
		dbr = (unsigned int) simple_strtoul(p, &p, 16);
		p++;
		dbg = (unsigned int) simple_strtoul(p, &p, 16);
		p++;
		dbb = (unsigned int) simple_strtoul(p, &p, 16);

		DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "process_dbg_opt(), %d %d %d %d %d %d\n", lrs, lgs, lbs, dbr, dbg, dbb);
#if 0 //defined(CONFIG_ARCH_MT6573)
		LCD_WaitForNotBusy();
		LCD_ConfigDither(lrs, lgs, lbs, dbr, dbg, dbb);
		LCD_StartTransfer(true);
#endif
	}
    else if (0 == strncmp(opt, "mtkfblog:", 9))
    {
        if (0 == strncmp(opt + 9, "on", 2)) {
            mtkfb_log_enable(true);
        } else if (0 == strncmp(opt + 9, "off", 3)) {
            mtkfb_log_enable(false);
        } else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "displog:", 8))
    {
        if (0 == strncmp(opt + 8, "on", 2)) {
            disp_log_enable(true);
        } else if (0 == strncmp(opt + 8, "off", 3)) {
            disp_log_enable(false);
        } else {
            goto Error;
        }
    }

    else if (0 == strncmp(opt, "lcdlog:", 7))
    {
        if (0 == strncmp(opt + 7, "on", 2)) {
            dbi_log_enable(true);
        } else if (0 == strncmp(opt + 7, "off", 3)) {
            dbi_log_enable(false);
        } else {
            goto Error;
        }
    }

	else if (0 == strncmp(opt, "dsilog:", 7))
    {
        if (0 == strncmp(opt + 7, "on", 2)) {
            DSI_Enable_Log(true);
        } else if (0 == strncmp(opt + 7, "off", 3)) {
            DSI_Enable_Log(false);
        } else {
            goto Error;
        }
    }

	else if (0 == strncmp(opt, "glitchlog:", 10))
    {
        if (0 == strncmp(opt + 10, "on", 2)) {
            Glitch_Enable_Log(true);
        } else if (0 == strncmp(opt + 10, "off", 3)) {
            Glitch_Enable_Log(false);
        } else {
            goto Error;
        }
    }
	
	else if(0 == strncmp(opt, "glitch_times:", 13))
	{
		char *p = (char *)opt + 13;
		unsigned int level = (unsigned int) simple_strtoul(p, &p, 10);
		Glitch_times(level);
	}
	else if (0 == strncmp(opt, "glitchlog:", 10))
    {
        if (0 == strncmp(opt + 10, "on", 2)) {
            Glitch_Enable_Log(true);
        } else if (0 == strncmp(opt + 10, "off", 3)) {
            Glitch_Enable_Log(false);
        } else {
            goto Error;
        }
    }
	
	else if(0 == strncmp(opt, "glitch_times:", 13))
	{
		char *p = (char *)opt + 13;
		unsigned int level = (unsigned int) simple_strtoul(p, &p, 10);
		Glitch_times(level);
	}
	else if (0 == strncmp(opt, "mtkfb_vsynclog:", 15))
    {
        if (0 == strncmp(opt + 15, "on", 2)) {
            mtkfb_vsync_log_enable(true);
        } else if (0 == strncmp(opt + 15, "off", 3)) {
            mtkfb_vsync_log_enable(false);
        } else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "dpilog:", 7))
    {
        if (0 == strncmp(opt + 7, "on", 2)) {
            //dpi_log_enable(true);
        } else if (0 == strncmp(opt + 7, "off", 3)) {
            //dpi_log_enable(false);
        } else {
            goto Error;
        }
    }

    else if (0 == strncmp(opt, "log:", 4))
    {
        if (0 == strncmp(opt + 4, "on", 2)) {
			mtkfb_log_enable(true);
			disp_log_enable(true);

			dbi_log_enable(true);

            //dpi_log_enable(true);
        } else if (0 == strncmp(opt + 4, "off", 3)) {
            mtkfb_log_enable(false);
			disp_log_enable(false);

			dbi_log_enable(false);

			//dpi_log_enable(false);
        } else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "update", 6))
    {
		DISP_UpdateScreen(0, 0, DISP_GetScreenWidth(), DISP_GetScreenHeight());
    }
    else if (0 == strncmp(opt, "pan_disp", 8))
    {
		mtkfb_pan_disp_test();
    }
    else if (0 == strncmp(opt, "sem_cnt", 7))
    {
		mtkfb_show_sem_cnt();
		LCD_GetVsyncCnt();
    }
    else if (0 == strncmp(opt, "hang:", 5))
    {
        if (0 == strncmp(opt + 5, "on", 2)) {
            mtkfb_hang_test(true);
        } else if (0 == strncmp(opt + 5, "off", 3)) {
            mtkfb_hang_test(false);
        } else{
            goto Error;
        }
	}
 #if defined(MTK_M4U_SUPPORT)
    else if (0 == strncmp(opt, "m4u:", 4))
    {
        if (0 == strncmp(opt + 4, "on", 2)) {
            mtkfb_m4u_switch(true);
        } else if (0 == strncmp(opt + 4, "off", 3)) {
            mtkfb_m4u_switch(false);
        } else if (0 == strncmp(opt + 4, "dump", 4)) {
            mtkfb_m4u_dump();
        } else{
            goto Error;
        }
	}
#endif
    else if (0 == strncmp(opt, "cpfbonly:", 9))
    {
        if (0 == strncmp(opt + 9, "on", 2))
        {
            mtkfb_capture_fb_only(true);
        }
        else if (0 == strncmp(opt + 9, "off", 3))
        {
            mtkfb_capture_fb_only(false);
        }
    }
    else if (0 == strncmp(opt, "esd:", 4))
    {
        if (0 == strncmp(opt + 4, "on", 2))
        {
            esd_recovery_pause(FALSE);
        }
        else if (0 == strncmp(opt + 4, "off", 3))
        {
            esd_recovery_pause(TRUE);
        }
    }
    else if (0 == strncmp(opt, "HQA:", 4))
    {
        if (0 == strncmp(opt + 4, "NormalToFactory", 15))
        {
            mtkfb_switch_normal_to_factory();
        }
        else if (0 == strncmp(opt + 4, "FactoryToNormal", 15))
        {
            mtkfb_switch_factory_to_normal();
        }
    }
    else if (0 == strncmp(opt, "mmp", 3))
    {
        init_mtkfb_mmp_events();
    }
    else if (0 == strncmp(opt, "dump_layer:", 11))
    {
        if (0 == strncmp(opt + 11, "on", 2))
        {
            char *p = (char *)opt + 14;
            gCaptureLayerDownX = simple_strtoul(p, &p, 10);
            gCaptureLayerDownY = simple_strtoul(p+1, &p, 10);
            gCaptureLayerEnable = 1;
        }
        else if (0 == strncmp(opt + 11, "off", 3))
        {
            gCaptureLayerEnable = 0;
        }
    }
    else if (0 == strncmp(opt, "dump_ovl:", 9))
    {
        if (0 == strncmp(opt + 9, "on", 2))
        {
            char *p = (char *)opt + 12;
            gCaptureOvlDownX = simple_strtoul(p, &p, 10);
            gCaptureOvlDownY = simple_strtoul(p+1, &p, 10);
            gCaptureOvlThreadEnable = 1;
			wake_up_process(captureovl_task);
        }
        else if (0 == strncmp(opt + 9, "off", 3))
        {
            gCaptureOvlThreadEnable = 0;
        }
    }
    else if (0 == strncmp(opt, "dump_fb:", 8))
    {
        if (0 == strncmp(opt + 8, "on", 2))
        {
            char *p = (char *)opt + 11;
            gCaptureFBDownX = simple_strtoul(p, &p, 10);
            gCaptureFBDownY = simple_strtoul(p+1, &p, 10);
            gCaptureFBPeriod = simple_strtoul(p+1, &p, 10);
            gCaptureFBEnable = 1;
			wake_up_interruptible(&gCaptureFBWQ);
        }
        else if (0 == strncmp(opt + 8, "off", 3))
        {
            gCaptureFBEnable = 0;
        }   
    }
    else
	{
		goto Error;
	}

    return;

Error:
    DISP_LOG_PRINT(ANDROID_LOG_INFO, "ERROR", "parse command error!\n\n%s", STR_HELP);
}


static void process_dbg_cmd(char *cmd)
{
    char *tok;
    
    DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "[mtkfb_dbg] %s\n", cmd);
    
    while ((tok = strsep(&cmd, " ")) != NULL)
    {
        process_dbg_opt(tok);
    }
}


// ---------------------------------------------------------------------------
//  Debug FileSystem Routines
// ---------------------------------------------------------------------------

struct dentry *mtkfb_dbgfs = NULL;
//<2013/02/26-22200-stevenchen, Add adb shell cat command to turn on/off LCM.
struct dentry *mtkfb_dbglcdon = NULL;
struct dentry *mtkfb_dbglcdoff = NULL;
//>2013/02/26-22200-stevenchen


static ssize_t debug_open(struct inode *inode, struct file *file)
{
    file->private_data = inode->i_private;
    return 0;
}

//<2013/02/26-22200-stevenchen, Add adb shell cat command to turn on/off LCM.
static ssize_t debug_lcdon_open(struct inode *inode, struct file *file)
{
    file->private_data = inode->i_private;
    return 0;
}

static ssize_t debug_lcdoff_open(struct inode *inode, struct file *file)
{
    file->private_data = inode->i_private;
    return 0;
}
//>2013/02/26-22200-stevenchen

static char debug_buffer[2048];

static ssize_t debug_read(struct file *file,
                          char __user *ubuf, size_t count, loff_t *ppos)
{
    const int debug_bufmax = sizeof(debug_buffer) - 1;
    int n = 0;

    n += scnprintf(debug_buffer + n, debug_bufmax - n, STR_HELP);
    debug_buffer[n++] = 0;

    return simple_read_from_buffer(ubuf, count, ppos, debug_buffer, n);
}

//<2013/04/12-23797-stevenchen, [Pelican][drv] Fix the adb command of turning on/off LCM.
//<2013/02/26-22200-stevenchen, Add adb shell cat command to turn on/off LCM.
static void debug_lcdon_read(struct file *file,
                          char __user *ubuf, size_t count, loff_t *ppos)
{
	DISP_PowerEnable(TRUE);
	//DISP_PanelEnable(TRUE);
	DISP_PanelOnOff(TRUE);

	//<2013/04/15-23830-stevenchen, [Pelican][drv] Fix the adb command of turning on/off LCM.
	DISP_UpdateScreen(0, 0, DISP_GetScreenWidth(), DISP_GetScreenHeight());
	//>2013/04/15-23830-stevenchen
}

static void debug_lcdoff_read(struct file *file,
                          char __user *ubuf, size_t count, loff_t *ppos)
{
	//DISP_PanelEnable(FALSE);
	DISP_PanelOnOff(FALSE);
        DISP_PowerEnable(FALSE);	
}
//>2013/02/26-22200-stevenchen
//>2013/04/12-23797-stevenchen

static ssize_t debug_write(struct file *file,
                           const char __user *ubuf, size_t count, loff_t *ppos)
{
    const int debug_bufmax = sizeof(debug_buffer) - 1;
	size_t ret;

	ret = count;

	if (count > debug_bufmax) 
        count = debug_bufmax;

	if (copy_from_user(&debug_buffer, ubuf, count))
		return -EFAULT;

	debug_buffer[count] = 0;

    process_dbg_cmd(debug_buffer);

    return ret;
}


static struct file_operations debug_fops = {
	.read  = debug_read,
    .write = debug_write,
	.open  = debug_open,
};
//<2013/02/26-22200-stevenchen, Add adb shell cat command to turn on/off LCM.
static struct file_operations debug_lcdon_fops = {
	.read  = debug_lcdon_read,
	.open  = debug_lcdon_open,
};

static struct file_operations debug_lcdoff_fops = {
	.read  = debug_lcdoff_read,
	.open  = debug_lcdoff_open,
};
//>2013/02/26-22200-stevenchen

#ifdef MTKFB_DEBUG_FS_CAPTURE_LAYER_CONTENT_SUPPORT

static ssize_t layer_debug_open(struct inode *inode, struct file *file)
{
    MTKFB_LAYER_DBG_OPTIONS *dbgopt;
    ///record the private data
    file->private_data = inode->i_private;
    dbgopt = (MTKFB_LAYER_DBG_OPTIONS *)file->private_data;

    dbgopt->working_size = DISP_GetScreenWidth()*DISP_GetScreenHeight()*2 + 32;
    dbgopt->working_buf = (UINT32)vmalloc(dbgopt->working_size);
    if(dbgopt->working_buf == 0)
        DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "Vmalloc to get temp buffer failed\n");

    return 0;
}


static ssize_t layer_debug_read(struct file *file,
                          char __user *ubuf, size_t count, loff_t *ppos)
{

#ifndef MT65XX_NEW_DISP
    int ret;
    MTKFB_LAYER_DBG_OPTIONS *dbgopt = (MTKFB_LAYER_DBG_OPTIONS *)file->private_data;
    UINT32 aligned_buffer;

    if(dbgopt->working_buf == 0)
    {
        DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "No working buffer is available \n");
        return 0;
    }
    
    ///if LCD layer is enabled 
    if(!LCD_IsLayerEnable(dbgopt->layer_index))
    {
        DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "The layer %d is not enabled \n", dbgopt->layer_index);
        return 0;
    }

    aligned_buffer = (dbgopt->working_buf + 32) & 0xFFFFFFC0;
    if(*ppos == 0)
    {
        extern struct semaphore sem_flipping;
        extern struct semaphore sem_early_suspend;
        extern struct semaphore sem_update_screen;
        extern BOOL is_early_suspended;
        
        ret = down_interruptible(&sem_flipping);
        ret = down_interruptible(&sem_early_suspend);
        if (is_early_suspended) {
              up(&sem_early_suspend);
              up(&sem_flipping);
              return 0;
        }
        ret = down_interruptible(&sem_update_screen);

        DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "This is layer %d capturing \n", dbgopt->layer_index);
        DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "This layer width:%d , height:%d\n", LCD_REG->LAYER[dbgopt->layer_index].SIZE.WIDTH, LCD_REG->LAYER[dbgopt->layer_index].SIZE.HEIGHT);

        if(lcm_params->type == LCM_TYPE_DSI && lcm_params->dsi.mode == CMD_MODE)
        {
            DSI_CHECK_RET(DSI_WaitForNotBusy());
            LCD_CHECK_RET(LCD_EnableDCtoDsi(FALSE));
        }

        LCD_Capture_Layerbuffer(dbgopt->layer_index,aligned_buffer,16);
        
        if(lcm_params->type == LCM_TYPE_DSI && lcm_params->dsi.mode == CMD_MODE)
        {
            LCD_CHECK_RET(LCD_EnableDCtoDsi(TRUE));
        }

        up(&sem_update_screen);
        up(&sem_early_suspend);
        up(&sem_flipping);    
    }
    
    return simple_read_from_buffer(ubuf, count, ppos, (void *)aligned_buffer, DISP_GetScreenWidth()*DISP_GetScreenHeight()*2);
#else
	return 0;
#endif
}


static ssize_t layer_debug_write(struct file *file,
                           const char __user *ubuf, size_t count, loff_t *ppos)
{
    MTKFB_LAYER_DBG_OPTIONS *dbgopt = (MTKFB_LAYER_DBG_OPTIONS *)file->private_data;

    DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "mtkfb_layer%d write is not implemented yet \n", dbgopt->layer_index);

    return count;
}

static int layer_debug_release(struct inode *inode, struct file *file)
{
    MTKFB_LAYER_DBG_OPTIONS *dbgopt;
    dbgopt = (MTKFB_LAYER_DBG_OPTIONS *)file->private_data;

    if(dbgopt->working_buf != 0)
        vfree((void *)dbgopt->working_buf);

    dbgopt->working_buf = 0;

    return 0;
}


static struct file_operations layer_debug_fops = {
	.read  = layer_debug_read,
    .write = layer_debug_write,
	.open  = layer_debug_open,
    .release = layer_debug_release,
};

#endif

void DBG_Init(void)
{
    mtkfb_dbgfs = debugfs_create_file("mtkfb",
        S_IFREG|S_IRUGO, NULL, (void *)0, &debug_fops);

//Steven start
    mtkfb_dbglcdon = debugfs_create_file("mtkfb_lcdon",
        S_IRUGO, NULL, (void *)0, &debug_lcdon_fops);

    mtkfb_dbglcdoff = debugfs_create_file("mtkfb_lcdoff",
        S_IRUGO, NULL, (void *)0, &debug_lcdoff_fops);
//Steven end

    memset(&dbg_opt, sizeof(dbg_opt), 0);
	memset(&fps, sizeof(fps), 0);
    dbg_opt.log_fps_wnd_size = DEFAULT_LOG_FPS_WND_SIZE;

#ifdef MTKFB_DEBUG_FS_CAPTURE_LAYER_CONTENT_SUPPORT
    {
        unsigned int i;
        unsigned char a[13];

        a[0] = 'm';
        a[1] = 't';
        a[2] = 'k';
        a[3] = 'f';
        a[4] = 'b';
        a[5] = '_';
        a[6] = 'l';
        a[7] = 'a';
        a[8] = 'y';
        a[9] = 'e';
        a[10] = 'r';
        a[11] = '0';
        a[12] = '\0';
        
        for(i=0;i<LCD_LAYER_NUM;i++)
        {
            a[11] = '0' + i;
            mtkfb_layer_dbg_opt[i].layer_index = i;
            mtkfb_layer_dbgfs[i] = debugfs_create_file(a,
                S_IFREG|S_IRUGO, NULL, (void *)&mtkfb_layer_dbg_opt[i], &layer_debug_fops);
        }
    }
#endif    
}


void DBG_Deinit(void)
{
    debugfs_remove(mtkfb_dbgfs);
#ifdef MTKFB_DEBUG_FS_CAPTURE_LAYER_CONTENT_SUPPORT
    {
        unsigned int i;
        
        for(i=0;i<LCD_LAYER_NUM;i++)
            debugfs_remove(mtkfb_layer_dbgfs[i]);
    }
#endif
}

