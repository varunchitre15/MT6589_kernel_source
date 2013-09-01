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
#define MTK_HDMI_TEST 0

#if MTK_HDMI_TEST
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

#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/mach-types.h>
#include <asm/cacheflush.h>
#include <asm/io.h>

#include <mach/dma.h>
#include <mach/irqs.h>

#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/mm.h>
#include <linux/cdev.h>
#include <asm/tlbflush.h>
#include <asm/page.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/slab.h>

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
#include <linux/list.h>

#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/mach-types.h>
#include <asm/cacheflush.h>
#include <asm/io.h>

#include <mach/dma.h>
#include <mach/irqs.h>
#include <linux/vmalloc.h>

#include <asm/uaccess.h>

#include "hdmi_drv.h"

#include "disp_drv_platform.h"
#include "ddp_reg.h"

#include "dpi_reg.h"
#include "mach/eint.h"
#include "mach/irqs.h"
#include "hdmitx.h"

#include <linux/switch.h>

#include <mach/mt_typedefs.h>

#include <mach/mt_clkmgr.h>
#include <mach/mt_reg_base.h>

#undef OUTREG32
#define OUTREG32(x, y) {/*printk("[hdmi]write 0x%08x to 0x%08x\n", (y), (x)); */__OUTREG32((x),(y))}
#define __OUTREG32(x,y) {*(unsigned int*)(x)=(y);}

#define RETIF(cond, rslt)       if ((cond)){HDMI_LOG("return in %d\n",__LINE__);return (rslt);}
#define RET_VOID_IF(cond)       if ((cond)){HDMI_LOG("return in %d\n",__LINE__);return;}
#define RETIF_NOLOG(cond, rslt)       if ((cond)){return (rslt);}
#define RET_VOID_IF_NOLOG(cond)       if ((cond)){return;}
#define RETIFNOT(cond, rslt)    if (!(cond)){HDMI_LOG("return in %d\n",__LINE__);return (rslt);}

#define ALIGN_TO(x, n)  \
    (((x) + ((n) - 1)) & ~((n) - 1))

#ifdef I2C_DBG
#include "tmbslHdmiTx_types.h"
#include "tmbslTDA9989_local.h"
#endif

#include <mach/m4u.h>
#include <mach/mt_typedefs.h>

#if defined(CONFIG_ARCH_MT6577) || defined(CONFIG_ARCH_MT6575)
#include <mach/mt_clock_manager.h>
#include "dpi_drv.h"
#elif defined(CONFIG_ARCH_MT6589)
#include <mach/mt_clkmgr.h>
#include "dpi1_drv.h"
#include <ddp_dpfd.h>
#define NEW_HDMI_ARCH
#endif

#include <mach/mt_reg_base.h>
void hdmi_update_impl(void);

extern int m4u_config_port(M4U_PORT_STRUCT* pm4uport);
extern void HDMI_DBG_Init(void);

extern bool mtkfb_is_suspend(void);


extern UINT32 DISP_GetScreenHeight(void);
extern UINT32 DISP_GetScreenWidth(void);
extern int disp_lock_mutex(void);
extern int disp_unlock_mutex(int id);

static size_t hdmi_log_on = 1;
static struct switch_dev hdmi_switch_data;
#define HDMI_LOG(fmt, arg...) \
	do { \
		if (hdmi_log_on) {printk("[hdmi]%s,#%d ", __func__, __LINE__); printk(fmt, ##arg);} \
	}while (0)

#define HDMI_FUNC()	\
    do { \
        if(hdmi_log_on) printk("[hdmi] %s\n", __func__); \
    }while (0)

#define HDMI_LINE()	\
    do { \
        if (hdmi_log_on) {printk("[hdmi]%s,%d ", __func__, __LINE__); printk(fmt, ##arg);} \
    }while (0)

//extern int pll_fsel(enum mt65xx_pll_id id, unsigned int pll_value);
#define HDMI_DEVNAME "hdmitx"
HDMI_PARAMS _s_hdmi_params = {0};
HDMI_PARAMS *hdmi_params = &_s_hdmi_params;
HDMI_DRIVER *hdmi_drv = NULL;

void hdmi_log_enable(int enable)
{
	printk("hdmi log %s\n", enable?"enabled":"disabled");
	hdmi_log_on = enable;
	hdmi_drv->log_enable(enable);
}

static DEFINE_SEMAPHORE(hdmi_update_mutex);
typedef struct{
    bool is_ipo_poweroff;   // IPO power off or not, like hdmi_suspend(), hdmi_resume()
    bool is_force_fullscreen;    // whether memory is forced to be rotated in Camera, no need to set orientation again
    bool is_force_portrait; // whether forced portrait mode, priority is higher than is_force_fullscreen
    bool is_reconfig_needed;    // whether need to reset HDMI memory
    bool is_enabled;    // whether HDMI is enabled or disabled by user
    //bool is_active;     // whether HDMI is actived with cable pluged in.
    bool is_force_disable; 		//used for camera scenario.
    bool is_clock_on;   // DPI is running or not
    bool is_factory_mode;
    atomic_t state; // HDMI_POWER_STATE state
    bool is_audio_avaliable;
    int 	lcm_width;  // LCD write buffer width
    int		lcm_height; // LCD write buffer height
    int		hdmi_width; // DPI read buffer width
    int		hdmi_height; // DPI read buffer height
    HDMI_VIDEO_RESOLUTION		output_video_resolution;
    HDMI_AUDIO_FORMAT           output_audio_format;
    int		orientation;    // MDP's orientation, 0 means 0 degree, 1 means 90 degree, 2 means 180 degree, 3 means 270 degree
    int     orientation_store;    // Store orientation setting when HDMI driver is_force_fullscreen
    int     orientation_store_portrait;	    // Store orientation setting when HDMI driver is_force_portrait
    HDMI_OUTPUT_MODE    output_mode;
}_t_hdmi_context;

struct hdmi_video_buffer_list {
    struct hdmi_video_buffer_info buffer_info;
    pid_t  pid;
    void*  file_addr;
    unsigned int buffer_mva;
    struct list_head list;
};

static struct list_head hdmi_video_mode_buffer_list;
//static struct list_head *hdmi_video_buffer_list_head = &hdmi_video_mode_buffer_list;
DEFINE_SEMAPHORE(hdmi_video_mode_mutex);
static atomic_t hdmi_video_mode_flag = ATOMIC_INIT(0);
//static int hdmi_add_video_buffer(struct hdmi_video_buffer_info *buffer_info, struct file *file);
//static struct hdmi_video_buffer_list* hdmi_search_video_buffer(struct hdmi_video_buffer_info *buffer_info, struct file *file);
//static void hdmi_destory_video_buffer(void);
#define IS_HDMI_IN_VIDEO_MODE()        atomic_read(&hdmi_video_mode_flag)
#define SET_HDMI_TO_VIDEO_MODE()       atomic_set(&hdmi_video_mode_flag, 1)
#define SET_HDMI_LEAVE_VIDEO_MODE()    atomic_set(&hdmi_video_mode_flag, 0)
static wait_queue_head_t hdmi_video_mode_wq;
static atomic_t hdmi_video_mode_event = ATOMIC_INIT(0);
static atomic_t hdmi_video_mode_dpi_change_address = ATOMIC_INIT(0);
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

extern int m4u_alloc_mva_stub(M4U_MODULE_ID_ENUM eModuleID, const unsigned int BufAddr, const unsigned int BufSize, unsigned int *pRetMVABuf);
extern int m4u_dealloc_mva_stub(M4U_MODULE_ID_ENUM eModuleID, const unsigned int BufAddr, const unsigned int BufSize, const unsigned int MVA);
extern int m4u_config_port_stub(M4U_PORT_STRUCT* pM4uPort);

extern int m4u_insert_tlb_range_stub(M4U_MODULE_ID_ENUM eModuleID, unsigned int MVAStart, const unsigned int MVAEnd, M4U_RANGE_PRIORITY_ENUM ePriority, unsigned int entryCount);
extern int m4u_invalid_tlb_range_stub(M4U_MODULE_ID_ENUM eModuleID, unsigned int MVAStart, unsigned int MVAEnd);

#ifndef NEW_HDMI_ARCH
static unsigned int temp_mva, temp_va, hdmi_va, hdmi_mva /*, fb_pa, fb_va, fb_size*/;
#else
int hdmi_allocate_hdmi_buffer(void);
int hdmi_free_hdmi_buffer(void);
int hdmi_display_path_overlay_config(bool enable);
int hdmi_dst_display_path_config(bool enable);
int hdmi_src_display_path_config(bool enable);
static int dp_mutex_src = -1, dp_mutex_dst = -1;
static unsigned int temp_mva_r, temp_mva_w, temp_va, hdmi_va, hdmi_mva_r, hdmi_mva_w /*, fb_pa, fb_va, fb_size*/;
#endif

static dev_t hdmi_devno;
static struct cdev *hdmi_cdev;
static struct class *hdmi_class = NULL;

static UINT32 const DPI_PAD_CON = 0xf2080900;
static UINT32 const NLI_ARB_CS = 0xf100d014;

static int hdmi_bpp = 4;

//static int hdmi_default_width = 1280;
//static int hdmi_default_height = 720;

static int hdmi_buffer_write_id = 0;
static int hdmi_buffer_read_id = 0;
//static int hdmi_buffer_lcdw_id = 0;

static DPI_POLARITY clk_pol, de_pol, hsync_pol, vsync_pol;
static unsigned int dpi_clk_div, dpi_clk_duty, hsync_pulse_width, hsync_back_porch, hsync_front_porch, vsync_pulse_width, vsync_back_porch, vsync_front_porch, intermediat_buffer_num;//mipi_pll_clk_ref, mipi_pll_clk_div1, mipi_pll_clk_div2, 
static HDMI_COLOR_ORDER rgb_order;


static struct task_struct *hdmi_update_task = NULL;

static wait_queue_head_t hdmi_update_wq;
static atomic_t hdmi_update_event = ATOMIC_INIT(0);

static unsigned int hdmi_resolution_param_table[][3] =
{
//        #ifdef MTK_MT8193_HDMI_SUPPORT
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
//        #else 
        {720,   480,    60},
        {1280,  720,    60},
        {1920,  1080,   30},
//        #endif
};

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

extern const HDMI_DRIVER* HDMI_GetDriver(void);

extern unsigned int DPI_GetCurrentFB(void);

/* Will be called in LCD Interrupt handler to check whether HDMI is actived */
bool is_hdmi_active(void)
{
	return IS_HDMI_ON();
}

/* Used for HDMI Driver update */
static int hdmi_update_kthread(void *data)
{
	struct sched_param param = { .sched_priority = RTPM_PRIO_SCRN_UPDATE };
	sched_setscheduler(current, SCHED_RR, &param);

    	for( ;; ) 
	{
        	wait_event_interruptible(hdmi_update_wq, atomic_read(&hdmi_update_event));
		//HDMI_LOG("wq wakeup\n");
        	//hdmi_update_impl();

		atomic_set(&hdmi_update_event,0);
        	hdmi_update_impl();
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
    RET_VOID_IF(IS_HDMI_NOT_ON());

    atomic_set(&hdmi_update_event, 1);
    wake_up_interruptible(&hdmi_update_wq); //wake up hdmi_update_kthread() to do update
}

/* Switch LCD write buffer, will be called in LCD Interrupt handler */
void hdmi_test_switch_buffer(void)
{
    	printk("lcd write buffer\n");

	hdmi_update();
}

static long int get_current_time_us(void)
{
	struct timeval t;
	do_gettimeofday(&t);
	return (t.tv_sec & 0xFFF) * 1000000 + t.tv_usec;
}

void hdmi_update_impl(void)
{
    HDMI_LOG("hdmi_update_impl\n");
	int t = 0;
    //int ret = 0;
    //MdpkBitbltConfig pmdp;
    //int lcm_physical_rotation = 0;
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

    if(temp_va != 0 && hdmi_va != 0)
    {
        DdpkBitbltConfig pddp;
        int dstOffset;
        memset((void*)&pddp, 0, sizeof(DdpkBitbltConfig));

        pddp.srcX = pddp.srcY = 0;
        pddp.srcW = p->lcm_width;
        pddp.srcH   = p->lcm_height;
        pddp.srcWStride = p->lcm_width;
        pddp.srcHStride = p->lcm_height;
        pddp.srcAddr[0] = temp_va;
        pddp.srcFormat = eRGB888_K;
        pddp.srcBufferSize[0] = p->lcm_width*p->lcm_height*3;
        pddp.srcPlaneNum = 1;

        pddp.dstX = 0;
        pddp.dstY = 0;
        pddp.dstFormat = eARGB8888_K;
        pddp.pitch = p->hdmi_width;
        pddp.dstWStride = p->hdmi_width;
        pddp.dstHStride = p->hdmi_height;
        pddp.dstPlaneNum = 1;

        pddp.orientation = 0;

        switch(pddp.orientation)
        {
            case 90:
            case 270:
#if 1
            {
                pddp.dstW = ALIGN_TO(p->lcm_height * p->hdmi_height / p->lcm_width * 95 / 100, 4);
                pddp.dstH = ALIGN_TO(p->hdmi_height * 95 /100, 4);
                break;
            }
#endif // fall through now
            case 0:
            case 180:
            {
                pddp.dstW = ALIGN_TO(p->hdmi_width * 95 / 100, 4);
                pddp.dstH = ALIGN_TO(p->hdmi_height * 95 /100, 4);
                break;
            }
            default:
                HDMI_LOG("Unknown orientation %d\n", pddp.orientation);
                return;
        }
        dstOffset = (p->hdmi_height - pddp.dstH ) / 2 * p->hdmi_width * hdmi_bpp +
                    (p->hdmi_width - pddp.dstW) / 2 * hdmi_bpp;

        pddp.dstAddr[0] = hdmi_va;// + hdmi_buffer_write_id * p->hdmi_width * p->hdmi_height * hdmi_bpp + dstOffset;
        pddp.dstBufferSize[0] = p->hdmi_width*p->hdmi_height*hdmi_bpp;
		t = get_current_time_us();
        DDPK_Bitblt_Config(DDPK_CH_HDMI_0, &pddp);
        DDPK_Bitblt(DDPK_CH_HDMI_0);
    }

    //HDMI_LOG("dstw=%d, dsth=%d, ori=%d\n", p.dstW, p.dstH, p.orientation);

    DBG_OnHDMIDone();
    HDMI_LOG("cost %d us\n", get_current_time_us() - t);

    //hdmi_buffer_read_id = hdmi_buffer_write_id;

    //hdmi_buffer_write_id = (hdmi_buffer_write_id+1) % hdmi_params->intermediat_buffer_num;
    
    done:
    up(&hdmi_update_mutex);

    return;
}

/* Allocate memory, set M4U, LCD, MDP, DPI */
/* LCD overlay to memory -> MDP resize and rotate to memory -> DPI read to HDMI */
/* Will only be used in ioctl(MTK_HDMI_AUDIO_VIDEO_ENABLE) */
static HDMI_STATUS hdmi_drv_init(void)
{
    int lcm_width, lcm_height;
    int tmpBufferSize;
		M4U_PORT_STRUCT portStruct;

    HDMI_FUNC();

    RETIF(p->output_mode == HDMI_OUTPUT_MODE_DPI_BYPASS, 0);

    p->hdmi_width = 1920;
    p->hdmi_height = 1080;

    lcm_width = DISP_GetScreenWidth();
    lcm_height = DISP_GetScreenHeight();

    //printk("[hdmi]%s, hdmi_width=%d, hdmi_height=%d\n", __func__, p->hdmi_width, p->hdmi_height);
    HDMI_LOG("lcm_width=%d, lcm_height=%d\n", lcm_width, lcm_height);

    tmpBufferSize = lcm_width * lcm_height *4 * 4;

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
						temp_va, 
						tmpBufferSize,
						DMA_BIDIRECTIONAL);

	portStruct.ePortID = M4U_PORT_WDMA1;			 //hardware port ID, defined in M4U_PORT_ID_ENUM
	portStruct.Virtuality = 1;							 
	portStruct.Security = 0;
	portStruct.domain = 0;						//domain : 0 1 2 3
	portStruct.Distance = 1;
	portStruct.Direction = 0; 	
	m4u_config_port(&portStruct);

    HDMI_LOG("temp_va=0x%08x, temp_mva_w=0x%08x\n", temp_va, temp_mva_w);

    p->lcm_width = lcm_width;
    p->lcm_height = lcm_height;
    p->output_video_resolution = hdmi_params->init_config.vformat;
    p->output_audio_format = hdmi_params->init_config.aformat;

//#ifdef NEW_HDMI_ARCH
//    hdmi_display_path_overlay_config(true);
//#endif
	DISP_Config_Overlay_to_Memory(temp_mva_w, 1);

    //hdmi_dpi_config_clock(); // configure dpi clock

    //hdmi_dpi_power_switch(false);   // but dpi power is still off
    //hdmi_drv->suspend();

#if 0
    LCD_WaitForNotBusy();
    LCD_SetOutputMode(3); // LCD write to memory and LCM
#endif

    return HDMI_STATUS_OK;
}


/* Will only be used in hdmi_drv_init(), this means that will only be use in ioctl(MTK_HDMI_AUDIO_VIDEO_ENABLE) */
/*static*/ void hdmi_dpi_config_clock(void)
{
    int ret = 0;

    RET_VOID_IF(p->output_mode == HDMI_OUTPUT_MODE_DPI_BYPASS);

    ret = enable_pll(TVDPLL, "HDMI");
    if(ret)
    {
        HDMI_LOG("enable_pll fail!!\n");
    }

 
            printk("[hdmi]1080p 30Hz\n");
            //ret = pll_fsel(TVDPLL, 0x800B6C4E);

			OUTREG32(TVDPLL_CON1, 0x800b6c4e); //148.5MHz
			OUTREG32(TVDPLL_CON0, 0x80000081);
			//OUTREG32(TVDPLL_CON0, 0x80000081);
						
			//OUTREG32(DISPSYS_BASE+0x038, 0x1);	// rdma0_out_sel, 2 for DPI0
			//OUTREG32(DISPSYS_BASE+0x05c, 0x1);	// DPI0_SEL, 0 is from rdma0
			
			clk_pol	 = HDMI_POLARITY_FALLING;
			de_pol 	 = HDMI_POLARITY_RISING;
			hsync_pol	 = HDMI_POLARITY_RISING;
			vsync_pol	 = HDMI_POLARITY_RISING;;

			dpi_clk_div = 2;
			dpi_clk_duty = 1;

			hsync_pulse_width	 = 44;
			hsync_back_porch	 = 148;
			hsync_front_porch	 = 88;

			vsync_pulse_width	 = 5;
			vsync_back_porch	 = 36;
			vsync_front_porch	 = 4;
            ASSERT(!ret);


    rgb_order           = hdmi_params->rgb_order;
    intermediat_buffer_num = 4;

    // dpi clock configuration using MIPITX
    //if(hdmi_params->dpi_port == HDMI_DPI_OUTPUT_PORT_0)
    {
		DPI_CHECK_RET(DPI_Init(FALSE));

		DPI_CHECK_RET(DPI_ConfigPixelClk(clk_pol, dpi_clk_div, dpi_clk_duty));

		DPI_CHECK_RET(DPI_ConfigDataEnable(de_pol));

		DPI_CHECK_RET(DPI_ConfigHsync(hsync_pol, hsync_pulse_width, hsync_back_porch, hsync_front_porch));

		DPI_CHECK_RET(DPI_ConfigVsync(vsync_pol, vsync_pulse_width, vsync_back_porch, vsync_front_porch));

		DPI_CHECK_RET(DPI_FBSetSize(1920, 1080));

		
		//if (LCM_COLOR_ORDER_BGR == rgb_order)
		if (HDMI_COLOR_ORDER_BGR == rgb_order)
		{
		    DPI_CHECK_RET(DPI_SetRGBOrder(DPI_RGB_ORDER_RGB, DPI_RGB_ORDER_BGR));
		}
		else
		{
		    DPI_CHECK_RET(DPI_SetRGBOrder(DPI_RGB_ORDER_RGB, DPI_RGB_ORDER_RGB));
		}
		//DPI_Internal_Pattern(1, 5);

		DPI_CHECK_RET(DPI_EnableClk());

		p->is_clock_on = true;
    }
}

#ifdef NEW_HDMI_ARCH
int hdmi_allocate_hdmi_buffer(void)
{
    M4U_PORT_STRUCT m4uport;
    int hdmiPixelSize = p->hdmi_width * p->hdmi_height;
    int hdmiDataSize = hdmiPixelSize * hdmi_bpp;
    int hdmiBufferSize = hdmiDataSize * 4;

    HDMI_FUNC();

    hdmi_va = (unsigned int) vmalloc(hdmiBufferSize);
    if (((void*) hdmi_va) == NULL)
    {
        HDMI_LOG("vmalloc %dbytes fail!!!\n", hdmiBufferSize);
        return -1;
    }

    memset((void*) hdmi_va, 0x80, hdmiBufferSize);

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

    HDMI_LOG("hdmi_va=0x%08x, hdmi_mva_r=0x%08x, hdmi_mva_w=0x%08x\n", hdmi_va, hdmi_mva_r, hdmi_mva_w);

    return 0;
}



int hdmi_dst_display_path_config(bool enable)
{
    HDMI_FUNC();
    if(enable)
    {
        //FIXME: now nothing can be seen on TV if output UYVY from WDMA0
          
        unsigned int hdmiSourceAddr = hdmi_mva_r;// + p->hdmi_width * p->hdmi_height * hdmi_bpp * hdmi_buffer_read_id;

        struct disp_path_config_struct config = {0};

        // Config RDMA->DPI1
        config.srcWidth = 1920;
        config.srcHeight = 1080;

        config.srcModule = DISP_MODULE_RDMA1;
        config.inFormat = RDMA_INPUT_FORMAT_ARGB;
        config.outFormat = RDMA_OUTPUT_FORMAT_ARGB;
        config.addr = hdmiSourceAddr;
        config.pitch = config.srcWidth * 4;
				
		config.dstModule = DISP_MODULE_DPI0;

        //if(-1 == dp_mutex_dst)
         //   dp_mutex_dst = disp_lock_mutex();
        dp_mutex_dst = 2;
		disp_dump_reg(DISP_MODULE_RDMA0);
		disp_dump_reg(DISP_MODULE_RDMA1);
		disp_dump_reg(DISP_MODULE_CONFIG);

        HDMI_LOG("Get mutex ID %d for RDMA1>DPI1\n", dp_mutex_dst);
        disp_path_get_mutex_(dp_mutex_dst);
        disp_path_config_(&config, dp_mutex_dst);
        disp_path_release_mutex_(dp_mutex_dst);
		disp_dump_reg(DISP_MODULE_CONFIG);				
		disp_dump_reg(DISP_MODULE_RDMA0);
		disp_dump_reg(DISP_MODULE_RDMA1);
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

#endif //NEW_HDMI_ARCH


static int hdmi_release(struct inode *inode, struct file *file)
{
	return 0;
}

static int hdmi_open(struct inode *inode, struct file *file)
{
	return 0;
}

void hdmi_force_init(void)
{
	p->hdmi_width = 1920;
	p->hdmi_height = 1080;
	p->output_video_resolution = HDMI_VIDEO_1920x1080p_30Hz;
	
	p->is_enabled = true;
	p->is_factory_mode = false;

	HDMI_CHECK_RET(hdmi_drv_init());

	hdmi_allocate_hdmi_buffer();
	//hdmi_src_display_path_config(true);

	hdmi_dst_display_path_config(true);

	hdmi_dpi_config_clock();
	SET_HDMI_ON();
	
	//hdmi_power_on();
	//hdmi_resume();
}

struct file_operations hdmi_fops = {
	.owner   = THIS_MODULE,
	.open    = hdmi_open,
	.release = hdmi_release,
};

static int hdmi_remove(struct platform_device *pdev)
{
	return 0;
}


static void __exit hdmi_exit(void)
{
	device_destroy(hdmi_class, hdmi_devno);
	class_destroy(hdmi_class);
	cdev_del(hdmi_cdev);
	unregister_chrdev_region(hdmi_devno, 1);

}



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
	class_dev = (struct class_device *)device_create(hdmi_class, NULL, hdmi_devno,	NULL, HDMI_DEVNAME);

	printk("[hdmi][%s] current=0x%08x\n", __func__, (unsigned int)current);
    	init_waitqueue_head(&hdmi_update_wq);


    	hdmi_update_task = kthread_create(hdmi_update_kthread, NULL, "hdmi_update_kthread");
    	wake_up_process(hdmi_update_task);
    	return 0;
}

static struct platform_driver hdmi_driver = {
    .probe  = hdmi_probe,
    .remove = hdmi_remove,
    .driver = { .name = HDMI_DEVNAME }
};
static struct platform_device hdmi_device = {
	.name = HDMI_DEVNAME,
	.id   = 0,
};

static int __init hdmi_init(void)
{
	int ret = 0;
	printk("[hdmi]%s\n", __func__);

		if (platform_device_register(&hdmi_device))
		{
			printk("[hdmi]failed to register hdmi device");
		}

    	if (platform_driver_register(&hdmi_driver))
    	{
        	printk("[hdmi]failed to register mtkfb driver\n");
        	return -1;
   	}

	return 0;
}

module_init(hdmi_init);
module_exit(hdmi_exit);
MODULE_AUTHOR("Xuecheng, Zhang <xuecheng.zhang@mediatek.com>");
MODULE_DESCRIPTION("HDMI Driver");
MODULE_LICENSE("GPL");
#else
void hdmi_force_init(void)
{

}
void hdmi_test_switch_buffer(void)
{

}
#endif

