#ifdef BUILD_UBOOT
#include <asm/arch/disp_drv_platform.h>

extern s32 mt_set_gpio_out(u32 pin, u32 output);
extern s32 mt_set_gpio_mode(u32 pin, u32 mode);
extern s32 mt_set_gpio_dir(u32 pin, u32 dir);
extern s32 mt_set_gpio_pull_enable(u32 pin, u32 enable);

#else
#include <linux/delay.h>
#include <linux/fb.h>
#include "mtkfb.h"
#include <asm/uaccess.h>

#include "disp_drv.h"
#include <disp_drv_platform.h>
#include "disp_drv_log.h"
#include "lcd_drv.h"
#include "lcm_drv.h"
#include "dpi_drv.h"
#include "dsi_drv.h"
#include "dsi_reg.h"
#include "disp_drv_platform.h"
#include "debug.h"

#include <linux/disp_assert_layer.h>

#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/module.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include "mach/mt_clkmgr.h"
#include <linux/vmalloc.h>
#include "mtkfb_info.h"
extern unsigned int lcd_fps;
extern BOOL is_early_suspended;
extern struct semaphore sem_early_suspend;
LCM_DRIVER* lcm_driver_list[] = 
{ 
#if defined(HX8369)
	&hx8369_lcm_drv,
#endif

#if defined(HX8369_6575)
	&hx8369_6575_lcm_drv,
#endif

#if defined(BM8578)
	&bm8578_lcm_drv,
#endif

#if defined(NT35582_MCU)
	&nt35582_mcu_lcm_drv,
#endif

#if defined(NT35582_MCU_6575)
	&nt35582_mcu_6575_lcm_drv,
#endif

#if defined(NT35590_HD720_DSI_VDO_TRULY)
	&nt35590_hd720_dsi_vdo_truly_lcm_drv, 
#endif

#if defined(SSD2075_HD720_DSI_VDO_TRULY)
	&ssd2075_hd720_dsi_vdo_truly_lcm_drv, 
#endif


#if defined(NT35590_HD720_DSI_CMD_AUO)
	&nt35590_hd720_dsi_cmd_auo_lcm_drv,
#endif

#if defined(NT35590_HD720_DSI_CMD_AUO_QHD)
	&nt35590_hd720_dsi_cmd_auo_qhd_lcm_drv,
#endif

#if defined(NT35590_HD720_DSI_CMD_AUO_FWVGA)
	&nt35590_hd720_dsi_cmd_auo_fwvga_lcm_drv,
#endif

#if defined(NT35590_HD720_DSI_CMD_CMI)
	&nt35590_hd720_dsi_cmd_cmi_lcm_drv,
#endif

#if defined(NT35582_RGB_6575)
	&nt35582_rgb_6575_lcm_drv,
#endif

#if defined(HX8369_RGB_6585_FPGA)
	&hx8369_rgb_6585_fpga_lcm_drv,
#endif

#if defined(HX8357B)
	&hx8357b_lcm_drv,
#endif

#if defined(R61408)
	&r61408_lcm_drv,
#endif

#if defined(HX8369_DSI_VDO)
	&hx8369_dsi_vdo_lcm_drv,
#endif

#if defined(HX8369_DSI)
	&hx8369_dsi_lcm_drv,
#endif

#if defined(HX8369_6575_DSI)
	&hx8369_dsi_6575_lcm_drv,
#endif

#if defined(HX8369_6575_DSI_NFC_ZTE)
	&hx8369_dsi_6575_lcm_drv,
#endif

#if defined(HX8369_6575_DSI_HVGA)
	&hx8369_dsi_6575_hvga_lcm_drv,
#endif

#if defined(HX8369_6575_DSI_QVGA)
	&hx8369_dsi_6575_qvga_lcm_drv,
#endif

#if defined(HX8369_HVGA)
	&hx8369_hvga_lcm_drv,
#endif

#if defined(NT35510)
	&nt35510_lcm_drv,
#endif

#if defined(NT35510_RGB_6575) 
	&nt35510_dpi_lcm_drv,
#endif	
	

#if defined(NT35510_HVGA)
	&nt35510_hvga_lcm_drv,
#endif

#if defined(NT35510_QVGA)
	&nt35510_qvga_lcm_drv,
#endif

#if defined(NT35510_6517)
	&nt35510_6517_lcm_drv,
#endif

#if defined(ILI9481)
	&ili9481_lcm_drv,
#endif

#if defined(NT35582)
	&nt35582_lcm_drv,
#endif

#if defined(S6D0170)
	&s6d0170_lcm_drv,
#endif

#if defined(SPFD5461A)
	&spfd5461a_lcm_drv,
#endif

#if defined(TA7601)
	&ta7601_lcm_drv,
#endif

#if defined(TFT1P3037)
	&tft1p3037_lcm_drv,
#endif

#if defined(HA5266)
	&ha5266_lcm_drv,
#endif

#if defined(HSD070IDW1)
	&hsd070idw1_lcm_drv,
#endif

#if defined(HX8363_6575_DSI)
	&hx8363_6575_dsi_lcm_drv,
#endif

#if defined(HX8363_6575_DSI_HVGA)
	&hx8363_6575_dsi_hvga_lcm_drv,
#endif

#if defined(LG4571)
	&lg4571_lcm_drv,
#endif

#if defined(LVDS_WSVGA)
	&lvds_wsvga_lcm_drv,
#endif

#if defined(LVDS_WSVGA_TI)
	&lvds_wsvga_ti_lcm_drv,
#endif

#if defined(LVDS_WSVGA_TI_N)
	&lvds_wsvga_ti_n_lcm_drv,
#endif

#if defined(NT35565_3D)
	&nt35565_3d_lcm_drv,
#endif

#if defined(TM070DDH03)
	&tm070ddh03_lcm_drv,
#endif
#if defined(R63303_IDISPLAY)
	&r63303_idisplay_lcm_drv,
#endif

#if defined(HX8369B_DSI_VDO)
	&hx8369b_dsi_vdo_lcm_drv,
#endif

#if defined(GN_SSD2825_SMD_S6E8AA)
	&gn_ssd2825_smd_s6e8aa,
#endif
#if defined(HX8369_TM_DSI)
	&hx8369_dsi_tm_lcm_drv,
#endif

#if defined(HX8369_BLD_DSI)
	&hx8369_dsi_bld_lcm_drv,
#endif

#if defined(HJ080IA)
	&hj080ia_lcm_drv,
#endif

#if defined(HJ101NA02A)
	&hj101na02a_lcm_drv,
#endif

#if defined(HSD070PFW3)
	&hsd070pfw3_lcm_drv,
#endif

#if defined(SCF0700M48GGU02)
	&scf0700m48ggu02_lcm_drv,
#endif

#if defined(OTM8018B_DSI_VDO)	
	&otm8018b_dsi_vdo_lcm_drv, 
#endif

#if defined(NT35512_DSI_VDO)
	&nt35512_dsi_vdo_lcm_drv, 
#endif

#if defined(HX8392A_DSI_CMD)
  &hx8392a_dsi_cmd_lcm_drv,
#endif 

#if defined(NT35516_QHD_DSI_CMD_IPSBOE)
  &nt35516_qhd_dsi_cmd_ipsboe_lcm_drv,
#endif

#if defined(NT35516_QHD_DSI_CMD_IPSBOE_WVGA)
  &nt35516_qhd_dsi_cmd_ipsboe_wvga_lcm_drv,
#endif

#if defined(NT35516_QHD_DSI_VEDIO)
  &nt35516_qhd_rav4_lcm_drv,
#endif

#if defined(BP070WS1)
  &bp070ws1_lcm_drv,
#endif

#if defined(BP070WS1_N)
  &bp070ws1_n_lcm_drv,
#endif

#if defined(BP101WX1)
  &bp101wx1_lcm_drv,
#endif

#if defined(BP101WX1_N)
  &bp101wx1_n_lcm_drv,
#endif

#if defined(NT35510_FWVGA)
  &nt35510_fwvga_lcm_drv,
#endif

#if defined(R63311_FHD_DSI_VDO_SHARP)
	&r63311_fhd_dsi_vdo_sharp_lcm_drv,
#endif

#if defined(NT35596_FHD_DSI_VDO_TRULY)
	&nt35596_fhd_dsi_vdo_truly_lcm_drv,
#endif

#if defined(LGLD070WX3_DSI_VDO)
    &lgld070wx3_dsi_vdo_lcm_drv,
#endif

#if defined(HE080IA)
	&he080ia_lcm_drv,
#endif

//<2013/02/26-22208-stevenchen, Add Himax HX8389-B LCM driver.
//#if defined(HX8389B_QHD_DSI_VDO_BYD)
//	&hx8389b_qhd_dsi_vdo_byd_lcm_drv,
// #endif
//>2013/02/26-22208-stevenchen

};

unsigned int lcm_count = sizeof(lcm_driver_list)/sizeof(LCM_DRIVER*);
#endif

extern unsigned int EnableVSyncLog;

#define LCM_ESD_CHECK_MAX_COUNT 5

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------
//<2013/04/12-23801-stevenchen, [Pelican][drv] Implement esd recovery function.
static PDSI_REGS const DSI_REG = (PDSI_REGS)(DSI_BASE); //add by mtk
//>2013/04/12-23801-stevenchen

static const DISP_DRIVER *disp_drv = NULL;
const LCM_DRIVER  *lcm_drv  = NULL;
static LCM_PARAMS s_lcm_params= {0};
LCM_PARAMS *lcm_params= &s_lcm_params;
static LCD_IF_ID ctrl_if = LCD_IF_PARALLEL_0;

static volatile int direct_link_layer = -1;
static UINT32 disp_fb_bpp = 32;     ///ARGB8888
#ifdef MTK_TRIPLE_FRAMEBUFFER_SUPPORT
static UINT32 disp_fb_pages = 3;     ///TRIPLE buffer
static BOOL is_page_set = TRUE;
#else
static UINT32 disp_fb_pages = 2;     ///double buffer
static BOOL is_page_set = FALSE;
#endif

BOOL is_engine_in_suspend_mode = FALSE;
BOOL is_lcm_in_suspend_mode    = FALSE;
static UINT32 dal_layerPA;
static UINT32 dal_layerVA;

static unsigned long u4IndexOfLCMList = 0;

#ifdef MT65XX_NEW_DISP	
static wait_queue_head_t config_update_wq;
static struct task_struct *config_update_task = NULL;
static int config_update_task_wakeup = 0;
extern atomic_t OverlaySettingDirtyFlag;
extern atomic_t OverlaySettingApplied;
extern unsigned int PanDispSettingPending;
extern unsigned int PanDispSettingDirty;
extern unsigned int PanDispSettingApplied;

extern unsigned int fb_pa;

extern bool is_ipoh_bootup;

extern struct mutex OverlaySettingMutex;
extern wait_queue_head_t reg_update_wq;
static wait_queue_head_t vsync_wq;
static bool vsync_wq_flag = false;

static struct hrtimer cmd_mode_update_timer;
static ktime_t cmd_mode_update_timer_period;

static bool needStartEngine = false;

extern unsigned int need_esd_check;
extern wait_queue_head_t esd_check_wq;
extern BOOL esd_kthread_pause;

DEFINE_MUTEX(MemOutSettingMutex);
static struct disp_path_config_mem_out_struct MemOutConfig;

DEFINE_MUTEX(LcmCmdMutex);

unsigned int disp_running = 0;
DECLARE_WAIT_QUEUE_HEAD(disp_done_wq);

OVL_CONFIG_STRUCT cached_layer_config[DDP_OVL_LAYER_MUN] = 
{
    {.layer = 0, .isDirty = 1},
    {.layer = 1, .isDirty = 1},
    {.layer = 2, .isDirty = 1},
    {.layer = 3, .isDirty = 1}
};

static OVL_CONFIG_STRUCT _layer_config[2][DDP_OVL_LAYER_MUN];
static unsigned int layer_config_index = 0;
OVL_CONFIG_STRUCT* captured_layer_config = _layer_config[0];
OVL_CONFIG_STRUCT* realtime_layer_config = _layer_config[0];

struct DBG_OVL_CONFIGS
{
    OVL_CONFIG_STRUCT Layer0;
    OVL_CONFIG_STRUCT Layer1;
    OVL_CONFIG_STRUCT Layer2;
    OVL_CONFIG_STRUCT Layer3;
};

unsigned int gCaptureLayerEnable = 0;
unsigned int gCaptureLayerDownX = 10;
unsigned int gCaptureLayerDownY = 10;

struct task_struct *captureovl_task = NULL;
static int _DISP_CaptureOvlKThread(void *data);
static unsigned int gWakeupCaptureOvlThread = 0;
unsigned int gCaptureOvlThreadEnable = 0;
unsigned int gCaptureOvlDownX = 10;
unsigned int gCaptureOvlDownY = 10;

struct task_struct *capturefb_task = NULL;
static int _DISP_CaptureFBKThread(void *data);
#ifdef USER_BUILD_KERNEL
unsigned int gCaptureFBEnable = 0;
#else
unsigned int gCaptureFBEnable = 0;
#endif
unsigned int gCaptureFBDownX = 10;
unsigned int gCaptureFBDownY = 10;
unsigned int gCaptureFBPeriod = 100;
DECLARE_WAIT_QUEUE_HEAD(gCaptureFBWQ);

extern struct fb_info *mtkfb_fbi;

unsigned int is_video_mode_running = 0;
#endif

DEFINE_SEMAPHORE(sem_update_screen);//linux 3.0 porting
static BOOL isLCMFound 					= FALSE;
/// Some utilities
#define ALIGN_TO_POW_OF_2(x, n)  \
	(((x) + ((n) - 1)) & ~((n) - 1))

#define ALIGN_TO(x, n)  \
	(((x) + ((n) - 1)) & ~((n) - 1))



static size_t disp_log_on = false;
#define DISP_LOG(fmt, arg...) \
    do { \
        if (disp_log_on) DISP_LOG_PRINT(ANDROID_LOG_WARN, "COMMON", fmt, ##arg); \
    }while (0)

#define DISP_FUNC()	\
	do { \
		if(disp_log_on) DISP_LOG_PRINT(ANDROID_LOG_INFO, "COMMON", "[Func]%s\n", __func__); \
	}while (0)

void disp_log_enable(int enable)
{
    disp_log_on = enable;
	DISP_LOG("disp common log %s\n", enable?"enabled":"disabled");
}
// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

static void lcm_set_reset_pin(UINT32 value)
{
	int ret = 0;
	if(LCM_TYPE_DSI == lcm_params->type){
		ret = enable_clock(MT_CG_DISP1_DBI_ENGINE, "DSI_LRST");
	}
	LCD_SetResetSignal(value);
	if(LCM_TYPE_DSI == lcm_params->type){
		ret	+= disable_clock(MT_CG_DISP1_DBI_ENGINE, "DSI_LRST");
	}
	if(ret > 0)
	{
		printk("[DSI_LRST]power manager API return FALSE\n");
	}   
}

static void lcm_udelay(UINT32 us)
{
	udelay(us);
}

static void lcm_mdelay(UINT32 ms)
{
	msleep(ms);
}

static void lcm_send_cmd(UINT32 cmd)
{
	if(lcm_params== NULL)
		return;

    ASSERT(LCM_CTRL_SERIAL_DBI   == lcm_params->ctrl ||
           LCM_CTRL_PARALLEL_DBI == lcm_params->ctrl);

    LCD_CHECK_RET(LCD_WriteIF(ctrl_if, LCD_IF_A0_LOW,
                              cmd, lcm_params->dbi.cpu_write_bits));
}

static void lcm_send_data(UINT32 data)
{
	if(lcm_params== NULL)
		return;

    ASSERT(LCM_CTRL_SERIAL_DBI   == lcm_params->ctrl ||
           LCM_CTRL_PARALLEL_DBI == lcm_params->ctrl);

    LCD_CHECK_RET(LCD_WriteIF(ctrl_if, LCD_IF_A0_HIGH,
                              data, lcm_params->dbi.cpu_write_bits));
}

static UINT32 lcm_read_data(void)
{
	UINT32 data = 0;

	if(lcm_params== NULL)
		return 0;

	ASSERT(LCM_CTRL_SERIAL_DBI   == lcm_params->ctrl ||
			LCM_CTRL_PARALLEL_DBI == lcm_params->ctrl);

	LCD_CHECK_RET(LCD_ReadIF(ctrl_if, LCD_IF_A0_HIGH,
				&data, lcm_params->dbi.cpu_write_bits));

	return data;
}

static __inline LCD_IF_WIDTH to_lcd_if_width(LCM_DBI_DATA_WIDTH data_width)
{
	switch(data_width)
	{
		case LCM_DBI_DATA_WIDTH_8BITS  : return LCD_IF_WIDTH_8_BITS;
		case LCM_DBI_DATA_WIDTH_9BITS  : return LCD_IF_WIDTH_9_BITS;
		case LCM_DBI_DATA_WIDTH_16BITS : return LCD_IF_WIDTH_16_BITS;
		case LCM_DBI_DATA_WIDTH_18BITS : return LCD_IF_WIDTH_18_BITS;
		case LCM_DBI_DATA_WIDTH_24BITS : return LCD_IF_WIDTH_24_BITS;
		default : ASSERT(0);
	}
	return LCD_IF_WIDTH_18_BITS;
}

static void disp_drv_set_driving_current(LCM_PARAMS *lcm)
{
	LCD_Set_DrivingCurrent(lcm);
}

static void disp_drv_init_io_pad(LCM_PARAMS *lcm)
{
	LCD_Init_IO_pad(lcm);
}

static void disp_drv_init_ctrl_if(void)
{
	const LCM_DBI_PARAMS *dbi = NULL;

	if(lcm_params== NULL)
		return;

	dbi = &(lcm_params->dbi);
	switch(lcm_params->ctrl)
	{
		case LCM_CTRL_NONE :
		case LCM_CTRL_GPIO : return;

		case LCM_CTRL_SERIAL_DBI :
							 ASSERT(dbi->port <= 1);
#ifdef MT65XX_NEW_DISP
							LCD_CHECK_RET(LCD_Init());
#endif
							 ctrl_if = LCD_IF_SERIAL_0 + dbi->port;
#if (MTK_LCD_HW_SIF_VERSION == 1)
							 LCD_ConfigSerialIF(ctrl_if,
									 (LCD_IF_SERIAL_BITS)dbi->data_width,
									 dbi->serial.clk_polarity,
									 dbi->serial.clk_phase,
									 dbi->serial.cs_polarity,
									 dbi->serial.clock_base,
									 dbi->serial.clock_div,
									 dbi->serial.is_non_dbi_mode);
#else    ///(MTK_LCD_HW_SIF_VERSION == 2)
							 LCD_ConfigSerialIF(ctrl_if,
									 (LCD_IF_SERIAL_BITS)dbi->data_width,
									 dbi->serial.sif_3wire,
									 dbi->serial.sif_sdi,
									 dbi->serial.sif_1st_pol,
									 dbi->serial.sif_sck_def,
									 dbi->serial.sif_div2,
									 dbi->serial.sif_hw_cs,
									 dbi->serial.css,
									 dbi->serial.csh,
									 dbi->serial.rd_1st,
									 dbi->serial.rd_2nd,
									 dbi->serial.wr_1st,
									 dbi->serial.wr_2nd);
#endif
                             break;

		case LCM_CTRL_PARALLEL_DBI :
							 ASSERT(dbi->port <= 2);
#ifdef MT65XX_NEW_DISP
							LCD_CHECK_RET(LCD_Init());
#endif
							 ctrl_if = LCD_IF_PARALLEL_0 + dbi->port;
							 LCD_ConfigParallelIF(ctrl_if,
									 (LCD_IF_PARALLEL_BITS)dbi->data_width,
									 (LCD_IF_PARALLEL_CLK_DIV)dbi->clock_freq,
									 dbi->parallel.write_setup,
									 dbi->parallel.write_hold,
									 dbi->parallel.write_wait,
									 dbi->parallel.read_setup,
									 dbi->parallel.read_hold,
									 dbi->parallel.read_latency,
									 dbi->parallel.wait_period,
									 dbi->parallel.cs_high_width);
							 break;

		default : ASSERT(0);
	}

	LCD_CHECK_RET(LCD_SelectWriteIF(ctrl_if));

	LCD_CHECK_RET(LCD_ConfigIfFormat(dbi->data_format.color_order,
				dbi->data_format.trans_seq,
				dbi->data_format.padding,
				dbi->data_format.format,
				to_lcd_if_width(dbi->data_format.width)));
}

static const LCM_UTIL_FUNCS lcm_utils =
{
	.set_reset_pin      = lcm_set_reset_pin,
	.udelay             = lcm_udelay,
	.mdelay             = lcm_mdelay,
	.send_cmd           = lcm_send_cmd,
	.send_data          = lcm_send_data,
	.read_data          = lcm_read_data,
    .dsi_set_cmdq		= (void (*)(unsigned int *, unsigned int, unsigned char))DSI_set_cmdq,
	.dsi_set_cmdq_V2	= DSI_set_cmdq_V2,
	.dsi_set_cmdq_V3	= (void (*)(LCM_setting_table_V3 *, unsigned int, unsigned char))DSI_set_cmdq_V3,	
	.dsi_write_cmd		= DSI_write_lcm_cmd,
	.dsi_write_regs 	= DSI_write_lcm_regs,
	.dsi_read_reg		= DSI_read_lcm_reg,
	.dsi_dcs_read_lcm_reg       = DSI_dcs_read_lcm_reg,
	.dsi_dcs_read_lcm_reg_v2    = DSI_dcs_read_lcm_reg_v2,
	/** FIXME: GPIO mode should not be configured in lcm driver
	  REMOVE ME after GPIO customization is done    
	 */
	.set_gpio_out       = mt_set_gpio_out,
	.set_gpio_mode        = mt_set_gpio_mode,
	.set_gpio_dir         = mt_set_gpio_dir,
	.set_gpio_pull_enable = (int (*)(unsigned int, unsigned char))mt_set_gpio_pull_enable
};



extern void init_dsi(BOOL isDsiPoweredOn);
const LCM_DRIVER *disp_drv_get_lcm_driver(const char *lcm_name)
{
	LCM_DRIVER *lcm = NULL;
	printk("[LCM Auto Detect], we have %d lcm drivers built in\n", lcm_count);
	printk("[LCM Auto Detect], try to find driver for [%s]\n", 
			(lcm_name==NULL)?"unknown":lcm_name);

	if(lcm_count ==1)
	{
		// we need to verify whether the lcm is connected
		// even there is only one lcm type defined
		lcm = lcm_driver_list[0];
		lcm->set_util_funcs(&lcm_utils);
		lcm->get_params(&s_lcm_params);
		u4IndexOfLCMList = 0;

		lcm_params = &s_lcm_params;
		lcm_drv = lcm;
/*
		disp_drv_init_ctrl_if();
		disp_drv_set_driving_current(lcm_params);
		disp_drv_init_io_pad(lcm_params);

		if(lcm_drv->compare_id)
		{
            BOOL id_equal;
			if(LCM_TYPE_DSI == lcm_params->type){
				init_dsi(FALSE);
			}
            mutex_lock(&LcmCmdMutex);
            id_equal = lcm_drv->compare_id();
            mutex_unlock(&LcmCmdMutex);

			if(id_equal == TRUE)
			{
				printk("[LCM Specified] compare id success\n");
				isLCMFound = TRUE;
			}
			else
			{
				printk("[LCM Specified] compare id fail\n");
				printk("%s, lcm is not connected\n", __func__);

				if(LCM_TYPE_DSI == lcm_params->type)
					DSI_Deinit();
			}
		}
		else
*/
		{
			isLCMFound = TRUE;
		}

        printk("[LCM Specified]\t[%s]\n", (lcm->name==NULL)?"unknown":lcm->name);

		goto done;
	}
	else
	{
		int i;

		for(i = 0;i < lcm_count;i++)
		{
			lcm_params = &s_lcm_params;
			lcm = lcm_driver_list[i];

			printk("[LCM Auto Detect] [%d] - [%s]\t", i, (lcm->name==NULL)?"unknown":lcm->name);

			lcm->set_util_funcs(&lcm_utils);
			memset((void*)lcm_params, 0, sizeof(LCM_PARAMS));
			lcm->get_params(lcm_params);

			disp_drv_init_ctrl_if();
			disp_drv_set_driving_current(lcm_params);
			disp_drv_init_io_pad(lcm_params);

			if(lcm_name != NULL)
			{
				if(!strcmp(lcm_name,lcm->name))
				{
					printk("\t\t[success]\n");
					isLCMFound = TRUE;
                                   u4IndexOfLCMList = i;
					lcm_drv = lcm;

					goto done;
				}
				else
				{
					printk("\t\t[fail]\n");
				}
			}
			else 
			{
				if(LCM_TYPE_DSI == lcm_params->type){
					init_dsi(FALSE);
				}

				if(lcm->compare_id != NULL && lcm->compare_id())
				{
					printk("\t\t[success]\n");
					isLCMFound = TRUE;
					lcm_drv = lcm;
                                   u4IndexOfLCMList = i;
					goto done;
				}
				else
				{
				
					if(LCM_TYPE_DSI == lcm_params->type)
						DSI_Deinit();
					printk("\t\t[fail]\n");
				}
			}
		}
	}
done:
	return lcm_drv;
}


static void disp_dump_lcm_parameters(LCM_PARAMS *lcm_params)
{
	unsigned char *LCM_TYPE_NAME[] = {"DBI", "DPI", "DSI"};
	unsigned char *LCM_CTRL_NAME[] = {"NONE", "SERIAL", "PARALLEL", "GPIO"};

	if(lcm_params == NULL)
		return;

	printk("[mtkfb] LCM TYPE: %s\n", LCM_TYPE_NAME[lcm_params->type]);
	printk("[mtkfb] LCM INTERFACE: %s\n", LCM_CTRL_NAME[lcm_params->ctrl]);
	printk("[mtkfb] LCM resolution: %d x %d\n", lcm_params->width, lcm_params->height);

	return;
}

char disp_lcm_name[256] = {0};
BOOL disp_get_lcm_name_boot(char *cmdline)
{
	BOOL ret = FALSE;
	char *p, *q;

	p = strstr(cmdline, "lcm=");
	if(p == NULL)
	{
		// we can't find lcm string in the command line, 
		// the uboot should be old version, or the kernel is loaded by ICE debugger
		return DISP_SelectDeviceBoot(NULL);
	}

	p += 4;
	if((p - cmdline) > strlen(cmdline+1))
	{
		ret = FALSE;
		goto done;
	}

	isLCMFound = strcmp(p, "0");
	printk("[mtkfb] LCM is %sconnected\n", ((isLCMFound)?"":"not "));
	p += 2;
	q = p;
	while(*q != ' ' && *q != '\0')
		q++;

	memset((void*)disp_lcm_name, 0, sizeof(disp_lcm_name));
	strncpy((char*)disp_lcm_name, (const char*)p, (int)(q-p));

	if(DISP_SelectDeviceBoot(disp_lcm_name))
		ret = TRUE;

done:
	return ret;
}

static BOOL disp_drv_init_context(void)
{
	if (disp_drv != NULL && lcm_drv != NULL){
		return TRUE;
	}

    if(!isLCMFound)
	    DISP_DetectDevice();
    
	disp_drv_init_ctrl_if();

	switch(lcm_params->type)
	{
		case LCM_TYPE_DBI : disp_drv = DISP_GetDriverDBI(); break;
		case LCM_TYPE_DPI : disp_drv = DISP_GetDriverDPI(); break;
		case LCM_TYPE_DSI : disp_drv = DISP_GetDriverDSI(); break;
		default : ASSERT(0);
	}

	if (!disp_drv) return FALSE;

	return TRUE;
}

BOOL DISP_IsLCDBusy(void)
{
	return LCD_IsBusy();
}

BOOL DISP_IsLcmFound(void)
{
	return isLCMFound;
}

BOOL DISP_IsContextInited(void)
{
	if(lcm_params && disp_drv && lcm_drv)
		return TRUE;
	else
		return FALSE;
}

BOOL DISP_SelectDeviceBoot(const char* lcm_name)
{
	LCM_DRIVER *lcm = NULL;
	int i;

	printk("%s\n", __func__);
	if(lcm_name == NULL)
	{
		// we can't do anything in boot stage if lcm_name is NULL
		return false;
	}
	for(i = 0;i < lcm_count;i++)
	{
		lcm_params = &s_lcm_params;
		lcm = lcm_driver_list[i];

		printk("[LCM Auto Detect] [%d] - [%s]\t", 
			i, 
			(lcm->name==NULL)?"unknown":lcm->name);

		lcm->set_util_funcs(&lcm_utils);
		memset((void*)lcm_params, 0, sizeof(LCM_PARAMS));
		lcm->get_params(lcm_params);

		// if lcm type is speficied, we don't need to compare the lcm name
		// in case the uboot is old version, which don't have lcm name in command line
		if(lcm_count == 1)
		{
			lcm_drv = lcm;
			isLCMFound = true;
			u4IndexOfLCMList = 0;
			break;
		}

		if(!strcmp(lcm_name,lcm->name))
		{
			printk("\t\t[success]\n");
			u4IndexOfLCMList = i;
			lcm_drv = lcm;
			isLCMFound = TRUE;

			break;
		}
		else
		{
			printk("\t\t[fail]\n");
		}
	}

	if (NULL == lcm_drv)
	{
		printk("%s, disp_drv_get_lcm_driver() returns NULL\n", __func__);
		return FALSE;
	}

	switch(lcm_params->type)
	{
		case LCM_TYPE_DBI : disp_drv = DISP_GetDriverDBI(); break;
		case LCM_TYPE_DPI : disp_drv = DISP_GetDriverDPI(); break;
		case LCM_TYPE_DSI : disp_drv = DISP_GetDriverDSI(); break;
		default : ASSERT(0);
	}

	disp_dump_lcm_parameters(lcm_params);
	return TRUE;
}

BOOL DISP_SelectDevice(const char* lcm_name)
{

#ifndef MT65XX_NEW_DISP
	LCD_STATUS ret;
	ret = LCD_Init();
	printk("ret of LCD_Init() = %d\n", ret);
#endif

	lcm_drv = disp_drv_get_lcm_driver(lcm_name);
	if (NULL == lcm_drv)
	{
		printk("%s, disp_drv_get_lcm_driver() returns NULL\n", __func__);
		return FALSE;
	}

	disp_dump_lcm_parameters(lcm_params);
	return disp_drv_init_context();
}

BOOL DISP_DetectDevice(void)
{
#ifndef MT65XX_NEW_DISP
	LCD_STATUS ret;
	ret = LCD_Init();
	printk("ret of LCD_Init() = %d\n", ret);
#endif

	lcm_drv = disp_drv_get_lcm_driver(NULL);
	if (NULL == lcm_drv)
	{
		printk("%s, disp_drv_get_lcm_driver() returns NULL\n", __func__);
		return FALSE;
	}

	disp_dump_lcm_parameters(lcm_params);

	return TRUE;
}

// ---------------------------------------------------------------------------
//  DISP Driver Implementations
// ---------------------------------------------------------------------------
extern mtk_dispif_info_t dispif_info[MTKFB_MAX_DISPLAY_COUNT];

DISP_STATUS DISP_Init(UINT32 fbVA, UINT32 fbPA, BOOL isLcmInited)
{
    DISP_STATUS r = DISP_STATUS_OK;

#ifdef MT65XX_NEW_DISP
#if 0
	//OUTREG32(DISPSYS_BASE + 0x104, 0xffffffff);//set CG
	OUTREG32(DISPSYS_BASE + 0x108, 0xffffffff);
	//OUTREG32(DISPSYS_BASE + 0x114, 0xffffffff);
	OUTREG32(DISPSYS_BASE + 0x118, 0xffffffff);
	OUTREG32(DISPSYS_BASE + 0xC08, 0x1);
#endif
#endif
    captureovl_task = kthread_create(_DISP_CaptureOvlKThread, NULL, "disp_captureovl_kthread");
    if (IS_ERR(captureovl_task))
    {
        DISP_LOG("DISP_InitVSYNC(): Cannot create capture ovl kthread\n");
    }
    if (gWakeupCaptureOvlThread)
	    wake_up_process(captureovl_task);

    capturefb_task = kthread_create(_DISP_CaptureFBKThread, mtkfb_fbi, "disp_capturefb_kthread");
    if (IS_ERR(capturefb_task))
    {
        DISP_LOG("DISP_InitVSYNC(): Cannot create capture fb kthread\n");
    }
	wake_up_process(capturefb_task);

	if (!disp_drv_init_context()) {
        return DISP_STATUS_NOT_IMPLEMENTED;
    }

//	/* power on LCD before config its registers*/
//	LCD_CHECK_RET(LCD_Init());

    disp_drv_init_ctrl_if();
	disp_path_clock_on("mtkfb");
    if(clk_is_force_on(MT_CG_DISP0_LARB2_SMI)==0)
    {
        printk("[DDP] error, MT_CG_DISP0_LARB2_SMI is not force on\n");
    }
    else
    {
        //printk("[DDP] MT_CG_DISP0_LARB2_SMI is force on\n");
    }
    clk_clr_force_on(MT_CG_DISP0_LARB2_SMI);

	// For DSI PHY current leakage SW workaround.
	///TODO: HOW!!!
#if !defined (MTK_HDMI_SUPPORT)
#ifndef MT65XX_NEW_DISP
	if((lcm_params->type!=LCM_TYPE_DSI) && (lcm_params->type!=LCM_TYPE_DPI)){
		DSI_PHY_clk_switch(TRUE);
		DSI_PHY_clk_switch(FALSE);
	}
#endif
#endif

#ifndef MT65XX_NEW_DISP	
    fbVA += DISP_GetFBRamSize();
    fbPA += DISP_GetFBRamSize();
#endif
    r = (disp_drv->init) ?
        (disp_drv->init(fbVA, fbPA, isLcmInited)) :
        DISP_STATUS_NOT_IMPLEMENTED;

#ifndef BUILD_UBOOT
	DISP_InitVSYNC((100000000/lcd_fps) + 1);//us
#endif

    {
        DAL_STATUS ret;
        
        /// DAL init here
#ifndef MT65XX_NEW_DISP
        fbVA += disp_drv->get_working_buffer_size();
        fbPA += disp_drv->get_working_buffer_size();
#else
		fbVA += DISP_GetFBRamSize();
		fbPA += DISP_GetFBRamSize();
#endif
        ret = DAL_Init(fbVA, fbPA);
        ASSERT(DAL_STATUS_OK == ret);
        dal_layerPA = fbPA;
        dal_layerVA = fbVA;
    }
    		// check lcm status
		if(lcm_drv->check_status)
			lcm_drv->check_status();

		memset((void*)(&dispif_info[MTKFB_DISPIF_PRIMARY_LCD]), 0, sizeof(mtk_dispif_info_t));
		
		switch(lcm_params->type)
		{
			case LCM_TYPE_DBI:
			{
				dispif_info[MTKFB_DISPIF_PRIMARY_LCD].displayType = DISPIF_TYPE_DBI;
				dispif_info[MTKFB_DISPIF_PRIMARY_LCD].displayMode = DISPIF_MODE_COMMAND;
				dispif_info[MTKFB_DISPIF_PRIMARY_LCD].isHwVsyncAvailable = 1;
				printk("DISP Info: DBI, CMD MOde, HY Vsync enable\n");
				break;
			}
			case LCM_TYPE_DPI:
			{
				dispif_info[MTKFB_DISPIF_PRIMARY_LCD].displayType = DISPIF_TYPE_DPI;
				dispif_info[MTKFB_DISPIF_PRIMARY_LCD].displayMode = DISPIF_MODE_VIDEO;
				dispif_info[MTKFB_DISPIF_PRIMARY_LCD].isHwVsyncAvailable = 1;				
				printk("DISP Info: DPI, VDO MOde, HY Vsync enable\n");
				break;
			}
			case LCM_TYPE_DSI:
			{
				dispif_info[MTKFB_DISPIF_PRIMARY_LCD].displayType = DISPIF_TYPE_DSI;
				if(lcm_params->dsi.mode == CMD_MODE)
				{
					dispif_info[MTKFB_DISPIF_PRIMARY_LCD].displayMode = DISPIF_MODE_COMMAND;
					dispif_info[MTKFB_DISPIF_PRIMARY_LCD].isHwVsyncAvailable = 1;
					printk("DISP Info: DSI, CMD MOde, HY Vsync enable\n");
				}
				else
				{
					dispif_info[MTKFB_DISPIF_PRIMARY_LCD].displayMode = DISPIF_MODE_VIDEO;
					dispif_info[MTKFB_DISPIF_PRIMARY_LCD].isHwVsyncAvailable = 1;
					printk("DISP Info: DSI, VDO MOde, HY Vsync enable\n");
				}
				
				break;
			}
			default:
				break;
		}
		

	if(disp_drv->get_panel_color_format())
	{
		switch(disp_drv->get_panel_color_format())
		{
			case PANEL_COLOR_FORMAT_RGB565:
				dispif_info[MTKFB_DISPIF_PRIMARY_LCD].displayFormat = DISPIF_FORMAT_RGB565;
			case PANEL_COLOR_FORMAT_RGB666:
				dispif_info[MTKFB_DISPIF_PRIMARY_LCD].displayFormat = DISPIF_FORMAT_RGB666;
			case PANEL_COLOR_FORMAT_RGB888:
				dispif_info[MTKFB_DISPIF_PRIMARY_LCD].displayFormat = DISPIF_FORMAT_RGB888;
			default:
				break;
		}
	}
	
	dispif_info[MTKFB_DISPIF_PRIMARY_LCD].displayWidth = DISP_GetScreenWidth();
	dispif_info[MTKFB_DISPIF_PRIMARY_LCD].displayHeight = DISP_GetScreenHeight();
	dispif_info[MTKFB_DISPIF_PRIMARY_LCD].vsyncFPS = lcd_fps;

	if(dispif_info[MTKFB_DISPIF_PRIMARY_LCD].displayWidth * dispif_info[MTKFB_DISPIF_PRIMARY_LCD].displayHeight <= 240*432)
	{
		dispif_info[MTKFB_DISPIF_PRIMARY_LCD].xDPI = dispif_info[MTKFB_DISPIF_PRIMARY_LCD].yDPI = 120;
	}
	else if(dispif_info[MTKFB_DISPIF_PRIMARY_LCD].displayWidth * dispif_info[MTKFB_DISPIF_PRIMARY_LCD].displayHeight <= 320*480)
	{
		dispif_info[MTKFB_DISPIF_PRIMARY_LCD].xDPI = dispif_info[MTKFB_DISPIF_PRIMARY_LCD].yDPI = 160;
	}
	else if(dispif_info[MTKFB_DISPIF_PRIMARY_LCD].displayWidth * dispif_info[MTKFB_DISPIF_PRIMARY_LCD].displayHeight <= 480*854)
	{
		dispif_info[MTKFB_DISPIF_PRIMARY_LCD].xDPI = dispif_info[MTKFB_DISPIF_PRIMARY_LCD].yDPI = 240;
	}
	else
	{
		dispif_info[MTKFB_DISPIF_PRIMARY_LCD].xDPI = dispif_info[MTKFB_DISPIF_PRIMARY_LCD].yDPI = 320;
	}
	
	dispif_info[MTKFB_DISPIF_PRIMARY_LCD].isConnected = 1;

	

    return r;
}


DISP_STATUS DISP_Deinit(void)
{
	DISP_CHECK_RET(DISP_PanelEnable(FALSE));
	DISP_CHECK_RET(DISP_PowerEnable(FALSE));

	return DISP_STATUS_OK;
}

// -----

DISP_STATUS DISP_PowerEnable(BOOL enable)
{
	DISP_STATUS ret = DISP_STATUS_OK;

#ifdef BUILD_UBOOT
    static BOOL s_enabled = FALSE;
#else
	static BOOL s_enabled = TRUE;
#endif

	if (enable != s_enabled)
		s_enabled = enable;
	else
		return ret;

	if (down_interruptible(&sem_update_screen)) {
		printk("ERROR: Can't get sem_update_screen in DISP_PowerEnable()\n");
		return DISP_STATUS_ERROR;
	}

	disp_drv_init_context();

	is_engine_in_suspend_mode = enable ? FALSE : TRUE;

    if (!is_ipoh_bootup)
        needStartEngine = true;

    if (enable && lcm_drv && lcm_drv->resume_power)
    {
		lcm_drv->resume_power();
    }

	ret = (disp_drv->enable_power) ?
		(disp_drv->enable_power(enable)) :
		DISP_STATUS_NOT_IMPLEMENTED;

    if (enable) {
        DAL_OnDispPowerOn();
    }
    else if (lcm_drv && lcm_drv->suspend_power)
    {
        lcm_drv->suspend_power();
    }

	up(&sem_update_screen);


	return ret;
}


DISP_STATUS DISP_PanelEnable(BOOL enable)
{
#ifdef BUILD_UBOOT
    static BOOL s_enabled = FALSE;
#else
    static BOOL s_enabled = TRUE;
#endif
	DISP_STATUS ret = DISP_STATUS_OK;

	DISP_LOG("panel is %s\n", enable?"enabled":"disabled");

	if (down_interruptible(&sem_update_screen)) {
		printk("ERROR: Can't get sem_update_screen in DISP_PanelEnable()\n");
		return DISP_STATUS_ERROR;
	}

	disp_drv_init_context();

	is_lcm_in_suspend_mode = enable ? FALSE : TRUE;

	if (is_ipoh_bootup)
	    s_enabled = TRUE;

	if (!lcm_drv->suspend || !lcm_drv->resume) {
		ret = DISP_STATUS_NOT_IMPLEMENTED;
		goto End;
	}

	if (enable && !s_enabled) {
		s_enabled = TRUE;

		if(lcm_params->type==LCM_TYPE_DSI && lcm_params->dsi.mode != CMD_MODE)
		{		
			DSI_SetMode(CMD_MODE);
		}
        mutex_lock(&LcmCmdMutex);
		lcm_drv->resume();
		
		if(lcm_drv->check_status)
			lcm_drv->check_status();
		
		DSI_LP_Reset();
        mutex_unlock(&LcmCmdMutex);

		if(lcm_params->type==LCM_TYPE_DSI && lcm_params->dsi.mode != CMD_MODE)
		{
			//DSI_clk_HS_mode(1);
			DSI_WaitForNotBusy();
			DSI_SetMode(lcm_params->dsi.mode);
			
			//DPI_CHECK_RET(DPI_EnableClk());
			//DSI_CHECK_RET(DSI_EnableClk());

			//msleep(200);
		}
	}
	else if (!enable && s_enabled)
	{
		LCD_CHECK_RET(LCD_WaitForNotBusy());
		if(lcm_params->type==LCM_TYPE_DSI && lcm_params->dsi.mode == CMD_MODE)
			DSI_CHECK_RET(DSI_WaitForNotBusy());
		s_enabled = FALSE;

		if (lcm_params->type==LCM_TYPE_DSI && lcm_params->dsi.mode != CMD_MODE)
		{		
			DPI_CHECK_RET(DPI_DisableClk());
			//msleep(200);
			DSI_Reset();
			DSI_clk_HS_mode(0);
			DSI_SetMode(CMD_MODE);
		}

        mutex_lock(&LcmCmdMutex);
		lcm_drv->suspend();
        mutex_unlock(&LcmCmdMutex);
	}

End:
	up(&sem_update_screen);

	return ret;
}

//<2013/02/10-21805-stevenchen, Add ADB commands to turn on/off LCM.
DISP_STATUS DISP_PanelOnOff(BOOL on_off)
{
	static BOOL s_enabled = TRUE;
	
	DISP_STATUS ret = DISP_STATUS_OK;

	DISP_LOG("panel is %s\n", on_off?"ON":"OFF");

	if (down_interruptible(&sem_update_screen)) {
		printk("ERROR: Can't get sem_update_screen in DISP_PanelOnOff()\n");
		return DISP_STATUS_ERROR;
	}

	disp_drv_init_context();

	if (!lcm_drv->init || !lcm_drv->poweroff) {
		ret = DISP_STATUS_NOT_IMPLEMENTED;
		goto End;
	}

	if (on_off && !s_enabled) {
		s_enabled = TRUE;

		if(lcm_params->type==LCM_TYPE_DSI && lcm_params->dsi.mode != CMD_MODE)
		{		
			DSI_SetMode(CMD_MODE);
		}
        mutex_lock(&LcmCmdMutex);
//<2013/04/12-23797-stevenchen, [Pelican][drv] Fix the adb command of turning on/off LCM.
		lcm_drv->poweron();
//>2013/04/12-23797-stevenchen
		
		if(lcm_drv->check_status)
			lcm_drv->check_status();
		
		DSI_LP_Reset();
        mutex_unlock(&LcmCmdMutex);

		if(lcm_params->type==LCM_TYPE_DSI && lcm_params->dsi.mode != CMD_MODE)
		{
			//DSI_clk_HS_mode(1);
			DSI_WaitForNotBusy();
			DSI_SetMode(lcm_params->dsi.mode);
			
			//DPI_CHECK_RET(DPI_EnableClk());
			//DSI_CHECK_RET(DSI_EnableClk());

			//msleep(200);
		}
	}
	else if (!on_off && s_enabled)
	{
		LCD_CHECK_RET(LCD_WaitForNotBusy());
		if(lcm_params->type==LCM_TYPE_DSI && lcm_params->dsi.mode == CMD_MODE)
			DSI_CHECK_RET(DSI_WaitForNotBusy());
		s_enabled = FALSE;

		if (lcm_params->type==LCM_TYPE_DSI && lcm_params->dsi.mode != CMD_MODE)
		{		
			DPI_CHECK_RET(DPI_DisableClk());
			//msleep(200);
			DSI_Reset();
			DSI_clk_HS_mode(0);
			DSI_SetMode(CMD_MODE);
		}

        mutex_lock(&LcmCmdMutex);
		lcm_drv->poweroff();
        mutex_unlock(&LcmCmdMutex);
	}

End:
	up(&sem_update_screen);

	return ret;
}
//>2013/02/10-21805-stevenchen

DISP_STATUS DISP_LCDPowerEnable(BOOL enable)
{
	if (enable)
    {
        LCD_CHECK_RET(LCD_PowerOn());
        #if defined(MTK_M4U_SUPPORT)
        LCD_CHECK_RET(LCD_M4UPowerOn());
        #endif
    }
    else
    {
        LCD_CHECK_RET(LCD_PowerOff());
        #if defined(MTK_M4U_SUPPORT)
        LCD_CHECK_RET(LCD_M4UPowerOff());
        #endif
    }

	return DISP_STATUS_OK;
}

DISP_STATUS DISP_SetBacklight(UINT32 level)
{
	DISP_STATUS ret = DISP_STATUS_OK;

	if (down_interruptible(&sem_update_screen)) {
		printk("ERROR: Can't get sem_update_screen in DISP_SetBacklight()\n");
		return DISP_STATUS_ERROR;
	}

	disp_drv_init_context();

//	LCD_WaitForNotBusy();
	if(lcm_params->type==LCM_TYPE_DSI && lcm_params->dsi.mode == CMD_MODE)
		DSI_CHECK_RET(DSI_WaitForNotBusy());

	if (!lcm_drv->set_backlight) {
		ret = DISP_STATUS_NOT_IMPLEMENTED;
		goto End;
	}

    mutex_lock(&LcmCmdMutex);
	lcm_drv->set_backlight(level);

    mutex_unlock(&LcmCmdMutex);

End:

	up(&sem_update_screen);

	return ret;
}

DISP_STATUS DISP_SetBacklight_mode(UINT32 mode)
{
	DISP_STATUS ret = DISP_STATUS_OK;

	if (down_interruptible(&sem_update_screen)) {
		printk("ERROR: Can't get sem_update_screen in DISP_SetBacklight_mode()\n");
		return DISP_STATUS_ERROR;
	}

	disp_drv_init_context();

	LCD_WaitForNotBusy();
	if(lcm_params->type==LCM_TYPE_DSI && lcm_params->dsi.mode == CMD_MODE)
		DSI_CHECK_RET(DSI_WaitForNotBusy());

	if (!lcm_drv->set_backlight) {
		ret = DISP_STATUS_NOT_IMPLEMENTED;
		goto End;
	}

	if(lcm_params->type==LCM_TYPE_DSI && lcm_params->dsi.mode != CMD_MODE)
		DSI_SetMode(CMD_MODE);

    mutex_lock(&LcmCmdMutex);
	lcm_drv->set_backlight_mode(mode);
	DSI_LP_Reset();
    mutex_unlock(&LcmCmdMutex);

	if(lcm_params->type==LCM_TYPE_DSI && lcm_params->dsi.mode != CMD_MODE)
		DSI_SetMode(lcm_params->dsi.mode);

End:

	up(&sem_update_screen);

	return ret;

}

DISP_STATUS DISP_SetPWM(UINT32 divider)
{
	DISP_STATUS ret = DISP_STATUS_OK;

	if (down_interruptible(&sem_update_screen)) {
		printk("ERROR: Can't get sem_update_screen in DISP_SetPWM()\n");
		return DISP_STATUS_ERROR;
	}

	disp_drv_init_context();

	LCD_WaitForNotBusy();
	if(lcm_params->type==LCM_TYPE_DSI && lcm_params->dsi.mode == CMD_MODE)
		DSI_CHECK_RET(DSI_WaitForNotBusy());

	if (!lcm_drv->set_pwm) {
		ret = DISP_STATUS_NOT_IMPLEMENTED;
		goto End;
	}

    mutex_lock(&LcmCmdMutex);
	lcm_drv->set_pwm(divider);
	DSI_LP_Reset();
    mutex_unlock(&LcmCmdMutex);
End:

	up(&sem_update_screen);

	return ret;
}

DISP_STATUS DISP_GetPWM(UINT32 divider, unsigned int *freq)
{
	DISP_STATUS ret = DISP_STATUS_OK;
	
	disp_drv_init_context();

	if (!lcm_drv->get_pwm) {
		ret = DISP_STATUS_NOT_IMPLEMENTED;
		goto End;
	}

    mutex_lock(&LcmCmdMutex);
	*freq = lcm_drv->get_pwm(divider);
    mutex_unlock(&LcmCmdMutex);

End:
	return ret;
}

#ifndef BUILD_UBOOT
#if defined(MTK_LCD_HW_3D_SUPPORT)
static BOOL is3denabled = FALSE;
static BOOL ispwmenabled = FALSE;
static BOOL ismodechanged = FALSE;

static BOOL gCurrentMode = FALSE;
static BOOL gUsingMode = FALSE;


DISP_STATUS DISP_Set3DPWM(BOOL enable, BOOL landscape)
{
#if 1
	unsigned int temp_reg;

	if (enable && (!ispwmenabled || ismodechanged))
	{
		struct  pwm_easy_config pwm_setting;
		// Set GPIO66, GPIO67, GPIO68 to PWM2, PWM1, PWM3
		mt_set_gpio_mode(GPIO187, GPIO_MODE_GPIO);	
		mt_set_gpio_out(GPIO187, GPIO_OUT_ONE);
		
		mt_set_gpio_mode(GPIO66, GPIO_MODE_07);	
		mt_set_gpio_mode(GPIO67, GPIO_MODE_07);	
		mt_set_gpio_mode(GPIO68, GPIO_MODE_07);	

		pwm_setting.clk_src = PWM_CLK_OLD_MODE_32K;
		pwm_setting.duty = 50;
		pwm_setting.clk_div = CLK_DIV1;
		pwm_setting.duration = 533;

		pwm_setting.pwm_no = PWM1;
		pwm_set_easy_config(&pwm_setting);
		pwm_setting.pwm_no = PWM2;
		pwm_set_easy_config(&pwm_setting);
		pwm_setting.pwm_no = PWM3;
		pwm_set_easy_config(&pwm_setting);

		temp_reg=INREG32(INFRA_SYS_CFG_BASE+0x700); 

		temp_reg&=~0xF000000;

		if(landscape)
			temp_reg|=0x9000000;
		else
			temp_reg|=0x3000000;

		OUTREG32((INFRA_SYS_CFG_BASE+0x700), temp_reg);

		ispwmenabled = TRUE;
		
		DISP_LOG("3D PWM is enabled. landscape:%d ! \n", landscape);
	}
	else if (!enable && ispwmenabled)
	{	
		mt_set_gpio_mode(GPIO187, GPIO_MODE_GPIO);	
		mt_set_gpio_out(GPIO187, GPIO_OUT_ZERO);

		mt_set_gpio_mode(GPIO66, GPIO_MODE_GPIO);	
		mt_set_gpio_mode(GPIO67, GPIO_MODE_GPIO);	
		mt_set_gpio_mode(GPIO68, GPIO_MODE_GPIO);	

		ispwmenabled = FALSE;

		DISP_LOG("3D PWM is disabled ! \n");
	}
#endif
	return DISP_STATUS_OK;
}


BOOL DISP_Is3DEnabled(void)
{
	is3denabled = LCD_Is3DEnabled();

	return is3denabled;
}

BOOL DISP_is3DLandscapeMode(void)
{
	gCurrentMode = LCD_Is3DLandscapeMode();

	if (gCurrentMode != gUsingMode)
		ismodechanged = TRUE;
	else
		ismodechanged = FALSE;
	
	gUsingMode = gCurrentMode;

	return LCD_Is3DLandscapeMode();
}
#endif
#endif

// -----

DISP_STATUS DISP_SetFrameBufferAddr(UINT32 fbPhysAddr)
{
	LCD_CHECK_RET(LCD_LayerSetAddress(FB_LAYER, fbPhysAddr));

    return DISP_STATUS_OK;
}

// -----

static BOOL is_overlaying = FALSE;

DISP_STATUS DISP_EnterOverlayMode(void)
{
	DISP_FUNC();
	if (is_overlaying) {
		return DISP_STATUS_ALREADY_SET;
	} else {
		is_overlaying = TRUE;
	}

	return DISP_STATUS_OK;
}


DISP_STATUS DISP_LeaveOverlayMode(void)
{
	DISP_FUNC();
	if (!is_overlaying) {
		return DISP_STATUS_ALREADY_SET;
	} else {
		is_overlaying = FALSE;
	}

	return DISP_STATUS_OK;
}


// -----

DISP_STATUS DISP_EnableDirectLinkMode(UINT32 layer)
{
    ///since MT6573 we do not support DC mode
	return DISP_STATUS_OK;
}


DISP_STATUS DISP_DisableDirectLinkMode(UINT32 layer)
{
    ///since MT6573 we do not support DC mode
	return DISP_STATUS_OK;
}

// -----

extern int MT6516IDP_EnableDirectLink(void);

DISP_STATUS DISP_UpdateScreen(UINT32 x, UINT32 y, UINT32 width, UINT32 height)
{
    unsigned int is_video_mode;
	DISP_LOG("update screen, (%d,%d),(%d,%d)\n", x, y, width, height);
	if ((lcm_params->type==LCM_TYPE_DPI) ||
	    ((lcm_params->type==LCM_TYPE_DSI) && (lcm_params->dsi.mode != CMD_MODE)))
	    is_video_mode = 1;
	else
	    is_video_mode = 0;
	if (down_interruptible(&sem_update_screen)) {
		printk("ERROR: Can't get sem_update_screen in DISP_UpdateScreen()\n");
		return DISP_STATUS_ERROR;
	}
#ifndef BUILD_UBOOT
#if defined(MTK_LCD_HW_3D_SUPPORT)
	LCD_CHECK_RET(DISP_Set3DPWM( DISP_Is3DEnabled(), DISP_is3DLandscapeMode() ));
#endif
#endif
	// if LCM is powered down, LCD would never recieve the TE signal
	//
	if (is_lcm_in_suspend_mode || is_engine_in_suspend_mode) goto End;
#ifndef MT65XX_NEW_DISP
	LCD_CHECK_RET(LCD_WaitForNotBusy());
	if(lcm_params->type==LCM_TYPE_DSI && lcm_params->dsi.mode == CMD_MODE)
		DSI_CHECK_RET(DSI_WaitForNotBusy());
#endif
    if (is_video_mode && is_video_mode_running)
        needStartEngine = false;
    if (needStartEngine)
    {
	    if (lcm_drv->update) {
            mutex_lock(&LcmCmdMutex);
		    lcm_drv->update(x, y, width, height);
			DSI_LP_Reset();
            mutex_unlock(&LcmCmdMutex);
        }
    }
#ifndef MT65XX_NEW_DISP
    LCD_CHECK_RET(LCD_SetRoiWindow(x, y, width, height));

	LCD_CHECK_RET(LCD_FBSetStartCoord(x, y));
#endif
	if (-1 != direct_link_layer) {
		//MT6516IDP_EnableDirectLink();     // FIXME
	} 
    else 
    {
        if (needStartEngine)
		    disp_drv->update_screen(FALSE);
	}
    needStartEngine = false;
End:
	up(&sem_update_screen);

	return DISP_STATUS_OK;
}

static DISP_INTERRUPT_CALLBACK_STRUCT DISP_CallbackArray[DISP_LCD_INTERRUPT_EVENTS_NUMBER + DISP_DSI_INTERRUPT_EVENTS_NUMBER + DISP_DPI_INTERRUPT_EVENTS_NUMBER];

static void _DISP_InterruptCallbackProxy(DISP_INTERRUPT_EVENTS eventID)
{
    UINT32 offset;

    if(eventID >= DISP_LCD_INTERRUPT_EVENTS_START && eventID <= DISP_LCD_INTERRUPT_EVENTS_END )
    {
        offset = eventID - DISP_LCD_INTERRUPT_EVENTS_START;
        if(DISP_CallbackArray[offset].pFunc)
        {
            DISP_CallbackArray[offset].pFunc(DISP_CallbackArray[offset].pParam);
        }
    }
    else if(eventID >= DISP_DSI_INTERRUPT_EVENTS_START && eventID <= DISP_DSI_INTERRUPT_EVENTS_END )
    {
        offset = eventID - DISP_DSI_INTERRUPT_EVENTS_START + DISP_LCD_INTERRUPT_EVENTS_NUMBER;
        if(DISP_CallbackArray[offset].pFunc)
        {
            DISP_CallbackArray[offset].pFunc(DISP_CallbackArray[offset].pParam);
        }
    }
    else if(eventID >= DISP_DPI_INTERRUPT_EVENTS_START && eventID <= DISP_DPI_INTERRUPT_EVENTS_END )
    {
        offset = eventID - DISP_DPI_INTERRUPT_EVENTS_START + DISP_LCD_INTERRUPT_EVENTS_NUMBER + DISP_DSI_INTERRUPT_EVENTS_NUMBER;
        if(DISP_CallbackArray[offset].pFunc)
        {
            DISP_CallbackArray[offset].pFunc(DISP_CallbackArray[offset].pParam);
        }
    }
    else
    {
        DISP_LOG("Invalid event id: %d\n", eventID);
        ASSERT(0);
    }

    return;
}

DISP_STATUS DISP_SetInterruptCallback(DISP_INTERRUPT_EVENTS eventID, DISP_INTERRUPT_CALLBACK_STRUCT *pCBStruct)
{
    UINT32 offset;
    ASSERT(pCBStruct != NULL);
    
	disp_drv_init_context();

    if(eventID >= DISP_LCD_INTERRUPT_EVENTS_START && eventID <= DISP_LCD_INTERRUPT_EVENTS_END )
    {
        ///register callback
        offset = eventID - DISP_LCD_INTERRUPT_EVENTS_START;
        DISP_CallbackArray[offset].pFunc = pCBStruct->pFunc;
        DISP_CallbackArray[offset].pParam = pCBStruct->pParam;
        
        LCD_CHECK_RET(LCD_SetInterruptCallback(_DISP_InterruptCallbackProxy));
        LCD_CHECK_RET(LCD_EnableInterrupt(eventID));
    }
    else if(eventID >= DISP_DSI_INTERRUPT_EVENTS_START && eventID <= DISP_DSI_INTERRUPT_EVENTS_END )
    {
        ///register callback
        offset = eventID - DISP_DSI_INTERRUPT_EVENTS_START + DISP_LCD_INTERRUPT_EVENTS_NUMBER;
        DISP_CallbackArray[offset].pFunc = pCBStruct->pFunc;
        DISP_CallbackArray[offset].pParam = pCBStruct->pParam;

        DSI_CHECK_RET(DSI_SetInterruptCallback(_DISP_InterruptCallbackProxy));
        DSI_CHECK_RET(DSI_EnableInterrupt(eventID));
    }
    else if(eventID >= DISP_DPI_INTERRUPT_EVENTS_START && eventID <= DISP_DPI_INTERRUPT_EVENTS_END )
    {
        offset = eventID - DISP_DPI_INTERRUPT_EVENTS_START + DISP_LCD_INTERRUPT_EVENTS_NUMBER + DISP_DSI_INTERRUPT_EVENTS_NUMBER;
        DISP_CallbackArray[offset].pFunc = pCBStruct->pFunc;
        DISP_CallbackArray[offset].pParam = pCBStruct->pParam;
    
        DPI_CHECK_RET(DPI_SetInterruptCallback(_DISP_InterruptCallbackProxy));
        DPI_CHECK_RET(DPI_EnableInterrupt(eventID));
    }
    else
    {
        DISP_LOG("Invalid event id: %d\n", eventID);
        ASSERT(0);
        return DISP_STATUS_ERROR;        ///TODO: error log
    }
    return DISP_STATUS_OK;
}


DISP_STATUS DISP_WaitForLCDNotBusy(void)
{
    LCD_WaitForNotBusy();
    return DISP_STATUS_OK;
}

#ifdef MT65XX_NEW_DISP

DISP_STATUS _DISP_ConfigUpdateScreen(UINT32 x, UINT32 y, UINT32 width, UINT32 height)
{
#if defined(MTK_LCD_HW_3D_SUPPORT)
    LCD_CHECK_RET(DISP_Set3DPWM( DISP_Is3DEnabled(), DISP_is3DLandscapeMode() ));
#endif
    // if LCM is powered down, LCD would never recieve the TE signal
    //
    if (is_lcm_in_suspend_mode || is_engine_in_suspend_mode) return DISP_STATUS_ERROR;
    if (lcm_drv->update) {
        mutex_lock(&LcmCmdMutex);
        lcm_drv->update(x, y, width, height);
		DSI_LP_Reset();
        mutex_unlock(&LcmCmdMutex);
    }
    disp_drv->update_screen(TRUE);

    return DISP_STATUS_OK;
}

#define DISP_CB_MAXCNT 2
typedef struct {
    DISP_EXTRA_CHECKUPDATE_PTR checkupdate_cb[DISP_CB_MAXCNT];
    DISP_EXTRA_CONFIG_PTR config_cb[DISP_CB_MAXCNT];
} CONFIG_CB_ARRAY;
CONFIG_CB_ARRAY g_CB_Array = { {NULL , NULL},{NULL , NULL}};//if DISP_CB_MAXCNT == 2
DEFINE_MUTEX(UpdateRegMutex);
void GetUpdateMutex(void)
{
    mutex_lock(&UpdateRegMutex);
}
int TryGetUpdateMutex(void)
{
    return mutex_trylock(&UpdateRegMutex);
}
void ReleaseUpdateMutex(void)
{
    mutex_unlock(&UpdateRegMutex);
}

int DISP_RegisterExTriggerSource(DISP_EXTRA_CHECKUPDATE_PTR pCheckUpdateFunc , DISP_EXTRA_CONFIG_PTR pConfFunc)
{
    int index = 0;
    int hit = 0;
    if((NULL == pCheckUpdateFunc) || (NULL == pConfFunc))
    {
        printk("Warnning! [Func]%s register NULL function : %p,%p\n", __func__ , pCheckUpdateFunc , pConfFunc);
        return -1;
    }

    GetUpdateMutex();

    for(index = 0 ; index < DISP_CB_MAXCNT ; index += 1)
    {
        if(NULL == g_CB_Array.checkupdate_cb[index])
        {
            g_CB_Array.checkupdate_cb[index] = pCheckUpdateFunc;
            g_CB_Array.config_cb[index] = pConfFunc;
            hit = 1;
            break;
        }
    }

    ReleaseUpdateMutex();

    return (hit ? index : (-1));
}

void DISP_UnRegisterExTriggerSource(int u4ID)
{
    if(DISP_CB_MAXCNT < (u4ID+1))
    {
        printk("Warnning! [Func]%s unregister a never registered function : %d\n", __func__ , u4ID);
        return;
    }

    GetUpdateMutex();

    g_CB_Array.checkupdate_cb[u4ID] = NULL;
    g_CB_Array.config_cb[u4ID] = NULL;

    ReleaseUpdateMutex();
}
#if defined (MTK_HDMI_SUPPORT)
extern 	bool is_hdmi_active(void);
#endif
#if defined (MTK_WFD_SUPPORT)
extern 	bool is_wfd_active(void);
extern unsigned int wfd_get_current_ovl_dst_addr(void);
static int debug_ovl_dst_addr = 0;
#endif


static int _DISP_CaptureFBKThread(void *data)
{
	struct fb_info* pInfo = (struct fb_info*) data;
	MMP_MetaDataBitmap_t Bitmap;
    while(1)
    {
		wait_event_interruptible(gCaptureFBWQ, gCaptureFBEnable);
		Bitmap.data1 = pInfo->var.yoffset;
		Bitmap.width = DISP_GetScreenWidth();
		Bitmap.height = DISP_GetScreenHeight() * 2;
		Bitmap.bpp = pInfo->var.bits_per_pixel;
		switch (pInfo->var.bits_per_pixel)
		{
		case 16:
		    Bitmap.format = MMProfileBitmapRGB565;
		    break;
		case 32:
		    Bitmap.format = MMProfileBitmapBGRA8888;
		    break;
		default:
		    Bitmap.format = MMProfileBitmapRGB565;
		    Bitmap.bpp = 16;
		    break;
		}
		Bitmap.start_pos = 0;
		Bitmap.pitch = pInfo->fix.line_length;
		if (Bitmap.pitch == 0)
		{
		    Bitmap.pitch = ALIGN_TO(Bitmap.width, 32) * Bitmap.bpp / 8;
		}
		Bitmap.data_size = Bitmap.pitch * Bitmap.height;
		Bitmap.down_sample_x = gCaptureFBDownX;
		Bitmap.down_sample_y = gCaptureFBDownY;
		Bitmap.pData = ((struct mtkfb_device*)pInfo->par)->fb_va_base;
		MMProfileLogMetaBitmap(MTKFB_MMP_Events.FBDump, MMProfileFlagPulse, &Bitmap);
        msleep(gCaptureFBPeriod);
    }
    return 0;
}

static int _DISP_CaptureOvlKThread(void *data)
{
	unsigned int index = 0;
	unsigned int mva[2];
	void* va[2];
	unsigned int buf_size;
	M4U_PORT_STRUCT portStruct;
	unsigned int init = 0;
	unsigned int enabled = 0;
       MMP_MetaDataBitmap_t Bitmap;
    buf_size = DISP_GetScreenWidth() * DISP_GetScreenHeight() * 4;

    while(1)
    {
        wait_event_interruptible(reg_update_wq, gWakeupCaptureOvlThread);
        gWakeupCaptureOvlThread = 0;
        if (init == 0)
        {
			va[0] = vmalloc(buf_size);
			va[1] = vmalloc(buf_size);
			memset(va[0], 0, buf_size);
			memset(va[1], 0, buf_size);
			m4u_alloc_mva(M4U_CLNTMOD_WDMA, (unsigned int)va[0], buf_size, 0, 0, &(mva[0]));
			m4u_alloc_mva(M4U_CLNTMOD_WDMA, (unsigned int)va[1], buf_size, 0, 0, &(mva[1]));
            disp_module_clock_on(DISP_MODULE_WDMA1, "Dump OVL");
			portStruct.ePortID = M4U_PORT_WDMA1;		   //hardware port ID, defined in M4U_PORT_ID_ENUM
			portStruct.Virtuality = 1;
			portStruct.Security = 0;
			portStruct.domain = 0;			  //domain : 0 1 2 3
			portStruct.Distance = 1;
			portStruct.Direction = 0;
			m4u_config_port(&portStruct);
			init = 1;
        }
        if (!gCaptureOvlThreadEnable)
        {
            if (enabled)
            {
                DISP_Config_Overlay_to_Memory(mva[index], 0);
                enabled = 0;
            }
            continue;
        }
		DISP_Config_Overlay_to_Memory(mva[index], 1);
        enabled = 1;

		m4u_dma_cache_maint(M4U_CLNTMOD_WDMA,
			va[index],
			DISP_GetScreenHeight()*DISP_GetScreenWidth()*24/8,
			DMA_BIDIRECTIONAL);

		Bitmap.data1 = index;
		Bitmap.data2 = mva[index];
		Bitmap.width = DISP_GetScreenWidth();
		Bitmap.height = DISP_GetScreenHeight();
		Bitmap.format = MMProfileBitmapBGR888;
		Bitmap.start_pos = 0;
		Bitmap.bpp = 24;
		Bitmap.pitch = DISP_GetScreenWidth()*3;
		Bitmap.data_size = Bitmap.pitch * Bitmap.height;
		Bitmap.down_sample_x = gCaptureOvlDownX;
		Bitmap.down_sample_y = gCaptureOvlDownY;
		Bitmap.pData = va[index];
		MMProfileLogMetaBitmap(MTKFB_MMP_Events.OvlDump, MMProfileFlagPulse, &Bitmap);
		index = 1 - index;
    }
    return 0;
}

static void _DISP_DumpLayer(OVL_CONFIG_STRUCT* pLayer)
{
    if (gCaptureLayerEnable && pLayer->layer_en)
    {
        MMP_MetaDataBitmap_t Bitmap;
        Bitmap.data1 = pLayer->vaddr;
        Bitmap.width = pLayer->dst_w;
        Bitmap.height = pLayer->dst_h;
        switch (pLayer->fmt)
        {
        case OVL_INPUT_FORMAT_RGB565: Bitmap.format = MMProfileBitmapRGB565; Bitmap.bpp = 16; break;
        case OVL_INPUT_FORMAT_RGB888: Bitmap.format = MMProfileBitmapBGR888; Bitmap.bpp = 24; break;
        case OVL_INPUT_FORMAT_ARGB8888:
        case OVL_INPUT_FORMAT_PARGB8888:
        case OVL_INPUT_FORMAT_xRGB8888: Bitmap.format = MMProfileBitmapBGRA8888; Bitmap.bpp = 32; break;
        case OVL_INPUT_FORMAT_BGR888: Bitmap.format = MMProfileBitmapRGB888; Bitmap.bpp = 24; break;
        case OVL_INPUT_FORMAT_ABGR8888:
        case OVL_INPUT_FORMAT_PABGR8888:
        case OVL_INPUT_FORMAT_xBGR8888: Bitmap.format = MMProfileBitmapRGBA8888; Bitmap.bpp = 32; break;
        default: return;
        }
        Bitmap.start_pos = 0;
        Bitmap.pitch = pLayer->src_pitch;
        Bitmap.data_size = Bitmap.pitch * Bitmap.height;
        Bitmap.down_sample_x = gCaptureLayerDownX;
        Bitmap.down_sample_y = gCaptureLayerDownY;
        if (pLayer->addr >= fb_pa &&
            pLayer->addr < (fb_pa+DISP_GetVRamSize()))
        {
            Bitmap.pData = pLayer->vaddr;
            MMProfileLogMetaBitmap(MTKFB_MMP_Events.Layer[pLayer->layer], MMProfileFlagPulse, &Bitmap);
        }
        else
        {
            m4u_mva_map_kernel(pLayer->addr, Bitmap.data_size, 0, (unsigned int*)&Bitmap.pData, &Bitmap.data_size);
            MMProfileLogMetaBitmap(MTKFB_MMP_Events.Layer[pLayer->layer], MMProfileFlagPulse, &Bitmap);
            m4u_mva_unmap_kernel(pLayer->addr, Bitmap.data_size, (unsigned int)Bitmap.pData);
        }
    }
}

static void _DISP_VSyncCallback(void* pParam);

void DISP_StartConfigUpdate(void)
{
	if(((LCM_TYPE_DSI == lcm_params->type) && (CMD_MODE == lcm_params->dsi.mode)) || (LCM_TYPE_DBI == lcm_params->type))
	{
		config_update_task_wakeup = 1;
		wake_up_interruptible(&config_update_wq);
	}
}

static int _DISP_ConfigUpdateKThread(void *data)
{
    int i;
    int dirty = 0;
    int overlay_dirty = 0;
    int aal_dirty = 0;
    int index = 0;
    unsigned int esd_check_count;
    struct disp_path_config_mem_out_struct mem_out_config;
    struct sched_param param = { .sched_priority = RTPM_PRIO_SCRN_UPDATE };
    sched_setscheduler(current, SCHED_RR, &param);
    while(1)
    {
        wait_event_interruptible(config_update_wq, config_update_task_wakeup);
        config_update_task_wakeup = 0;
        //MMProfileLog(MTKFB_MMP_Events.UpdateConfig, MMProfileFlagStart);
        dirty = 0;

        if (down_interruptible(&sem_early_suspend)) {
            printk("[FB Driver] can't get semaphore in mtkfb_early_suspend()\n");
            continue;
        }
        //MMProfileLogEx(MTKFB_MMP_Events.EarlySuspend, MMProfileFlagStart, 1, 0);
        if (need_esd_check && (!is_early_suspended))
        {
            esd_check_count = 0;
            disp_running = 1;
            MMProfileLog(MTKFB_MMP_Events.EsdCheck, MMProfileFlagStart);
            while (esd_check_count < LCM_ESD_CHECK_MAX_COUNT)
            {
				if(DISP_EsdCheck())
				{
					MMProfileLogEx(MTKFB_MMP_Events.EsdCheck, MMProfileFlagPulse, 0, 0);
					DISP_EsdRecover();
				}
				else
				{
				    break;
				}
				esd_check_count++;
            }
            if (esd_check_count >= LCM_ESD_CHECK_MAX_COUNT)
            {
				MMProfileLogEx(MTKFB_MMP_Events.EsdCheck, MMProfileFlagPulse, 2, 0);
//<2013/04/12-23801-stevenchen, [Pelican][drv] Implement esd recovery function.
				esd_kthread_pause = FALSE;	//TRUE
//>2013/04/12-23801-stevenchen
            }
            if (esd_check_count)
            {
				if (lcm_drv->update)
				{
					mutex_lock(&LcmCmdMutex);
					lcm_drv->update(0, 0, DISP_GetScreenWidth(), DISP_GetScreenHeight());
					mutex_unlock(&LcmCmdMutex);
				}
				dirty = 1;
            }
            need_esd_check = 0;
            wake_up_interruptible(&esd_check_wq);
            MMProfileLog(MTKFB_MMP_Events.EsdCheck, MMProfileFlagEnd);
            // Do not set disp_running to 0 for DSI video mode.
            if ((lcm_params->type == LCM_TYPE_DSI) && (lcm_params->dsi.mode != CMD_MODE))
                disp_running = 1;
            else
            disp_running = 0;
        }

        if (!is_early_suspended)
        {
            if (mutex_trylock(&OverlaySettingMutex))
            {
                overlay_dirty = atomic_read(&OverlaySettingDirtyFlag);
                if (overlay_dirty)
                {
                    layer_config_index = 1 - layer_config_index;
                    captured_layer_config = _layer_config[layer_config_index];
                    memcpy(captured_layer_config, cached_layer_config, sizeof(OVL_CONFIG_STRUCT)*DDP_OVL_LAYER_MUN);
                    for (i=0; i<DDP_OVL_LAYER_MUN; i++)
                        cached_layer_config[i].isDirty = false;
                    MMProfileLogStructure(MTKFB_MMP_Events.ConfigOVL, MMProfileFlagPulse, captured_layer_config, struct DBG_OVL_CONFIGS);
                    atomic_set(&OverlaySettingDirtyFlag, 0);
                    PanDispSettingDirty = 0;
                    dirty = 1;
                }
                mutex_unlock(&OverlaySettingMutex);
            }
            aal_dirty = overlay_dirty;//overlay refresh means AAL also needs refresh
            //GetUpdateMutex();
            if(1 == TryGetUpdateMutex())
            {
                for(index = 0 ; index < DISP_CB_MAXCNT ; index += 1)
                {
                    if((NULL != g_CB_Array.checkupdate_cb[index]) && g_CB_Array.checkupdate_cb[index](overlay_dirty))
                    {
                        dirty = 1;
                        aal_dirty = 1;
                        break;
                    }
                }
                ReleaseUpdateMutex();
            }

            if (mutex_trylock(&MemOutSettingMutex))
            {
                if (MemOutConfig.dirty)
                {
                    memcpy(&mem_out_config, &MemOutConfig, sizeof(MemOutConfig));
                    MemOutConfig.dirty = 0;
                    dirty = 1;
                }
                else
                    mem_out_config.dirty = 0;
                mutex_unlock(&MemOutSettingMutex);
            }
        }
		if(((LCM_TYPE_DSI == lcm_params->type) && (CMD_MODE == lcm_params->dsi.mode)) || (LCM_TYPE_DBI == lcm_params->type))
		{
			_DISP_VSyncCallback(NULL);
		}

        if (dirty)
        {
            // Apply configuration here.
            disp_running = 1;
            disp_path_get_mutex();
            if (overlay_dirty)
            {
                MMProfileLog(MTKFB_MMP_Events.ConfigOVL, MMProfileFlagStart);
                for (i=0; i<DDP_OVL_LAYER_MUN; i++)
                {
                    if (captured_layer_config[i].isDirty)
                    {
                        _DISP_DumpLayer(&captured_layer_config[i]);
                        disp_path_config_layer(&captured_layer_config[i]);
                    }
                }
                if ((lcm_params->type == LCM_TYPE_DBI) ||
                    ((lcm_params->type == LCM_TYPE_DSI) && (lcm_params->dsi.mode == CMD_MODE)))
                {
                    if ((PanDispSettingPending==1) && (PanDispSettingDirty==0))
                    {
                        PanDispSettingApplied = 1;
                        PanDispSettingPending = 0;
                    }
                    atomic_set(&OverlaySettingApplied, 1);
                    wake_up_interruptible(&reg_update_wq);
                }
                MMProfileLog(MTKFB_MMP_Events.ConfigOVL, MMProfileFlagEnd);


            }

            // Apply AAL config here.
            if (aal_dirty)
            {
                MMProfileLog(MTKFB_MMP_Events.ConfigAAL, MMProfileFlagStart);

                //GetUpdateMutex();
                if(1 == TryGetUpdateMutex())
                {
                    for(index = 0 ; index < DISP_CB_MAXCNT ; index += 1)
                    {
                        if((NULL != g_CB_Array.checkupdate_cb[index]) && g_CB_Array.checkupdate_cb[index](overlay_dirty))
                        {
                            g_CB_Array.config_cb[index](overlay_dirty);
                        }
                    }
                    ReleaseUpdateMutex();
                }
                MMProfileLog(MTKFB_MMP_Events.ConfigAAL, MMProfileFlagEnd);
            }

            // Apply memory out config here.
            if (mem_out_config.dirty)
            {
                MMProfileLogEx(MTKFB_MMP_Events.ConfigMemOut, MMProfileFlagStart, mem_out_config.enable, mem_out_config.dstAddr);
                disp_path_config_mem_out(&mem_out_config);
                MMProfileLogEx(MTKFB_MMP_Events.ConfigMemOut, MMProfileFlagEnd, 0, 0);
            }
#if defined(MTK_HDMI_SUPPORT)
                if(is_hdmi_active() && !DISP_IsVideoMode())
                {
                MMProfileLogEx(MTKFB_MMP_Events.ConfigMemOut, MMProfileFlagStart, mem_out_config.enable, mem_out_config.dstAddr);
                    memcpy(&mem_out_config, &MemOutConfig, sizeof(MemOutConfig));
                    disp_path_config_mem_out(&mem_out_config);
                MMProfileLogEx(MTKFB_MMP_Events.ConfigMemOut, MMProfileFlagEnd, 1, 0);
                }
#endif
#if defined(MTK_WFD_SUPPORT)
            if(is_wfd_active() && !DISP_IsVideoMode())
            {
                MMProfileLogEx(MTKFB_MMP_Events.ConfigMemOut, MMProfileFlagStart, mem_out_config.enable, mem_out_config.dstAddr);
                memcpy(&mem_out_config, &MemOutConfig, sizeof(MemOutConfig));

				mem_out_config.dstAddr = wfd_get_current_ovl_dst_addr();
				if(debug_ovl_dst_addr == mem_out_config.dstAddr)
					MMProfileLogEx(MTKFB_MMP_Events.ConfigMemOut, MMProfileFlagPulse, 0xFFFFFFFF,0xFFFFFFFF);
			
				debug_ovl_dst_addr = wfd_get_current_ovl_dst_addr();					
                disp_path_config_mem_out(&mem_out_config);
                MMProfileLogEx(MTKFB_MMP_Events.ConfigMemOut, MMProfileFlagEnd, 2, 0);
            }
#endif
            // Trigger interface engine for cmd mode.
            if ((lcm_params->type == LCM_TYPE_DBI) ||
                ((lcm_params->type == LCM_TYPE_DSI) && (lcm_params->dsi.mode == CMD_MODE)))
            {
                DISP_STATUS ret = _DISP_ConfigUpdateScreen(0, 0, DISP_GetScreenWidth(), DISP_GetScreenHeight());
                if ((ret != DISP_STATUS_OK) && (is_early_suspended == 0))
                    hrtimer_start(&cmd_mode_update_timer, cmd_mode_update_timer_period, HRTIMER_MODE_REL);
            }
            disp_path_release_mutex();
        }
        else
        {
            if ((lcm_params->type == LCM_TYPE_DBI) ||
                ((lcm_params->type == LCM_TYPE_DSI) && (lcm_params->dsi.mode == CMD_MODE)))
            {
                // Start update timer.
                if (!is_early_suspended)
                    hrtimer_start(&cmd_mode_update_timer, cmd_mode_update_timer_period, HRTIMER_MODE_REL);
            }
        }

        //MMProfileLog(MTKFB_MMP_Events.UpdateConfig, MMProfileFlagEnd);
        //MMProfileLogEx(MTKFB_MMP_Events.EarlySuspend, MMProfileFlagEnd, 1, 0);
        up(&sem_early_suspend);
        if (kthread_should_stop())
            break;
    }

    return 0;
}

static void _DISP_HWDoneCallback(void* pParam)
{
    MMProfileLogEx(MTKFB_MMP_Events.DispDone, MMProfileFlagPulse, is_early_suspended, 0);
    // For DPI, this callback is called each time DPI VSync arrives.
    // So trigger disp done event only in early suspend.
    if ((lcm_params->type != LCM_TYPE_DPI) || (is_early_suspended))
    {
        // Disable DPI immediately to avoid restarting of DDP.
        if (lcm_params->type == LCM_TYPE_DPI)
            DPI_DisableClk();
        disp_running = 0;
        wake_up_interruptible(&disp_done_wq);
    }
}

static void _DISP_VSyncCallback(void* pParam)
{
    MMProfileLog(MTKFB_MMP_Events.VSync, MMProfileFlagPulse);
    vsync_wq_flag = 1;
    wake_up_interruptible(&vsync_wq);
}

static void _DISP_RegUpdateCallback(void* pParam)
{
    MMProfileLog(MTKFB_MMP_Events.RegUpdate, MMProfileFlagPulse);
    realtime_layer_config = captured_layer_config;
    if ((lcm_params->type == LCM_TYPE_DPI) ||
        ((lcm_params->type == LCM_TYPE_DSI) && (lcm_params->dsi.mode != CMD_MODE)))
    {
    	if ((PanDispSettingPending==1) && (PanDispSettingDirty==0))
    	{
			PanDispSettingApplied = 1;
			PanDispSettingPending = 0;
	    }
        atomic_set(&OverlaySettingApplied, 1);
    }
    gWakeupCaptureOvlThread = 1;
    wake_up_interruptible(&reg_update_wq);
}

static void _DISP_TargetLineCallback(void* pParam)
{
    //tasklet_hi_schedule(&ConfigUpdateTask);
    MMProfileLogEx(MTKFB_MMP_Events.UpdateConfig, MMProfileFlagPulse, 0, 0);
    config_update_task_wakeup = 1;
    wake_up_interruptible(&config_update_wq);
}

static void _DISP_CmdDoneCallback(void* pParam)
{
    //tasklet_hi_schedule(&ConfigUpdateTask);
    MMProfileLogEx(MTKFB_MMP_Events.UpdateConfig, MMProfileFlagPulse, 1, 0);
    config_update_task_wakeup = 1;
    wake_up_interruptible(&config_update_wq);
    _DISP_HWDoneCallback(NULL);
}

static enum hrtimer_restart _DISP_CmdModeTimer_handler(struct hrtimer *timer)
{
    MMProfileLogEx(MTKFB_MMP_Events.UpdateConfig, MMProfileFlagPulse, 2, 0);
    config_update_task_wakeup = 1;
    wake_up_interruptible(&config_update_wq);
    return HRTIMER_NORESTART;
}
#endif

void DISP_InitVSYNC(unsigned int vsync_interval)
{
#ifndef MT65XX_NEW_DISP
	if((LCM_TYPE_DBI == lcm_params->type) || 
	(LCM_TYPE_DSI == lcm_params->type && lcm_params->dsi.mode == CMD_MODE)){
    	LCD_InitVSYNC(vsync_interval);
	}
	else if((LCM_TYPE_DPI == lcm_params->type) || 
	(LCM_TYPE_DSI == lcm_params->type && lcm_params->dsi.mode != CMD_MODE)){
		DPI_InitVSYNC(vsync_interval);
	}
	else
	{
	    DISP_LOG("DISP_FMDesense_Query():unknown interface\n");
	}
#else
    DISP_INTERRUPT_CALLBACK_STRUCT cb;
    init_waitqueue_head(&config_update_wq);
    init_waitqueue_head(&vsync_wq);
    config_update_task = kthread_create(
        _DISP_ConfigUpdateKThread, NULL, "disp_config_update_kthread");

    if (IS_ERR(config_update_task)) 
    {
        DISP_LOG("DISP_InitVSYNC(): Cannot create config update kthread\n");
        return;
    }
    wake_up_process(config_update_task);
    if (LCM_TYPE_DBI == lcm_params->type)
    {
        LCD_InitVSYNC(vsync_interval);
        cmd_mode_update_timer_period = ktime_set(0 , vsync_interval*1000);//ktime_set(0, 14285714); // 70Hz
        hrtimer_init(&cmd_mode_update_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
        cmd_mode_update_timer.function = _DISP_CmdModeTimer_handler;
//        cb.pFunc = _DISP_VSyncCallback;
//        DISP_SetInterruptCallback(DISP_LCD_SYNC_INT, &cb);
        cb.pFunc = _DISP_CmdDoneCallback;
        DISP_SetInterruptCallback(DISP_LCD_TRANSFER_COMPLETE_INT, &cb);
        cb.pFunc = _DISP_RegUpdateCallback;
        DISP_SetInterruptCallback(DISP_LCD_REG_COMPLETE_INT, &cb);
        config_update_task_wakeup = 1;
        wake_up_interruptible(&config_update_wq);
    }
    else if (LCM_TYPE_DPI == lcm_params->type)
    {
        RDMASetTargetLine(0, DISP_GetScreenHeight()*4/5);
        cb.pFunc = _DISP_VSyncCallback;
        DISP_SetInterruptCallback(DISP_DPI_VSYNC_INT, &cb);
        cb.pFunc = _DISP_TargetLineCallback;
        DISP_SetInterruptCallback(DISP_DPI_TARGET_LINE_INT, &cb);
        cb.pFunc = _DISP_RegUpdateCallback;
        DISP_SetInterruptCallback(DISP_DPI_REG_UPDATE_INT, &cb);
    }
    else if (LCM_TYPE_DSI == lcm_params->type)
    {
        if (CMD_MODE == lcm_params->dsi.mode)
        {
            DSI_InitVSYNC(vsync_interval);
            cmd_mode_update_timer_period = ktime_set(0 , vsync_interval*1000);//ktime_set(0, 14285714); // 70Hz
            hrtimer_init(&cmd_mode_update_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
            cmd_mode_update_timer.function = _DISP_CmdModeTimer_handler;
//            cb.pFunc = _DISP_VSyncCallback;
//            DISP_SetInterruptCallback(DISP_DSI_VSYNC_INT, &cb);
            cb.pFunc = _DISP_CmdDoneCallback;
            DISP_SetInterruptCallback(DISP_DSI_CMD_DONE_INT, &cb);
            cb.pFunc = _DISP_RegUpdateCallback;
            DISP_SetInterruptCallback(DISP_DSI_REG_UPDATE_INT, &cb);
            config_update_task_wakeup = 1;
            wake_up_interruptible(&config_update_wq);
        }
        else
        {
            RDMASetTargetLine(0, DISP_GetScreenHeight()*4/5);
            cb.pFunc = _DISP_VSyncCallback;
            DISP_SetInterruptCallback(DISP_DSI_VSYNC_INT, &cb);
            cb.pFunc = _DISP_TargetLineCallback;
            DISP_SetInterruptCallback(DISP_DSI_TARGET_LINE_INT, &cb);
            cb.pFunc = _DISP_RegUpdateCallback;
            DISP_SetInterruptCallback(DISP_DSI_REG_UPDATE_INT, &cb);
            cb.pFunc = _DISP_HWDoneCallback;
            DISP_SetInterruptCallback(DISP_DSI_VMDONE_INT, &cb);
        }
    }
    else
        DISP_LOG("DISP_InitVSYNC():unknown interface\n");
#endif
}

void DISP_WaitVSYNC(void)
{
    MMProfileLog(MTKFB_MMP_Events.WaitVSync, MMProfileFlagStart);
#ifndef MT65XX_NEW_DISP
	if((LCM_TYPE_DBI == lcm_params->type) || 
	(LCM_TYPE_DSI == lcm_params->type && lcm_params->dsi.mode == CMD_MODE)){
    	LCD_WaitTE();
	}
	else if((LCM_TYPE_DPI == lcm_params->type) || 
	(LCM_TYPE_DSI == lcm_params->type && lcm_params->dsi.mode != CMD_MODE)){
		DPI_WaitVSYNC();
	}
	else
	{
	    DISP_LOG("DISP_WaitVSYNC():unknown interface\n");
	}
#else
    if((LCM_TYPE_DBI == lcm_params->type))
    {
        LCD_WaitTE();
    }
    else if(LCM_TYPE_DSI == lcm_params->type && lcm_params->dsi.mode == CMD_MODE)
    {
//        DSI_WaitTE();
        vsync_wq_flag = 0;
        if (wait_event_interruptible_timeout(vsync_wq, vsync_wq_flag, HZ/10) == 0)
        {
            printk("[DISP] Wait VSync timeout. early_suspend=%d\n", is_early_suspended);
        }
        
    }
    else if((LCM_TYPE_DPI == lcm_params->type) || 
        (LCM_TYPE_DSI == lcm_params->type && lcm_params->dsi.mode != CMD_MODE))
    {
        //printk("[DISP] +VSync\n");
        vsync_wq_flag = 0;
        if (wait_event_interruptible_timeout(vsync_wq, vsync_wq_flag, HZ/10) == 0)
        {
            printk("[DISP] Wait VSync timeout. early_suspend=%d\n", is_early_suspended);
        }
        //printk("[DISP] -VSync\n");
    }
    else
    {
        DISP_LOG("DISP_WaitVSYNC():unknown interface\n");
    }
#endif
    MMProfileLog(MTKFB_MMP_Events.WaitVSync, MMProfileFlagEnd);
}

DISP_STATUS DISP_PauseVsync(BOOL enable)
{
	if((LCM_TYPE_DBI == lcm_params->type) || 
	(LCM_TYPE_DSI == lcm_params->type && lcm_params->dsi.mode == CMD_MODE)){
        if (enable)
            hrtimer_cancel(&cmd_mode_update_timer);
    	LCD_PauseVSYNC(enable);
	}
	else if((LCM_TYPE_DPI == lcm_params->type) || 
	(LCM_TYPE_DSI == lcm_params->type && lcm_params->dsi.mode != CMD_MODE)){
		DPI_PauseVSYNC(enable);
	}
	else
	{
	    DISP_LOG("DISP_PauseVSYNC():unknown interface\n");
	}
	return DISP_STATUS_OK;
}

DISP_STATUS DISP_ConfigDither(int lrs, int lgs, int lbs, int dbr, int dbg, int dbb)
{
    DISP_LOG("DISP_ConfigDither lrs:0x%x, lgs:0x%x, lbs:0x%x, dbr:0x%x, dbg:0x%x, dbb:0x%x \n", lrs, lgs, lbs, dbr, dbg, dbb);

    if(LCD_STATUS_OK == LCD_ConfigDither(lrs, lgs, lbs, dbr, dbg, dbb))
    {
        return DISP_STATUS_OK;
    }
    else
    {
		DISP_LOG("DISP_ConfigDither error \n");
        return DISP_STATUS_ERROR;
    }
}


// ---------------------------------------------------------------------------
//  Retrieve Information
// ---------------------------------------------------------------------------

BOOL DISP_IsVideoMode(void)
{
    disp_drv_init_context();
    if(lcm_params)
        return lcm_params->type==LCM_TYPE_DPI || (lcm_params->type==LCM_TYPE_DSI && lcm_params->dsi.mode != CMD_MODE);
    else
    {
        printk("WARNING!! DISP_IsVideoMode is called before display driver inited!\n");
        return 0;
    }
}

UINT32 DISP_GetScreenWidth(void)
{
    disp_drv_init_context();
	if(lcm_params)
	    return lcm_params->width;
	else
	{
		printk("WARNING!! get screen width before display driver inited!\n");
		return 0;
	}
}
EXPORT_SYMBOL(DISP_GetScreenWidth);

UINT32 DISP_GetScreenHeight(void)
{
    disp_drv_init_context();
	if(lcm_params)
    	return lcm_params->height;
	else
	{
		printk("WARNING!! get screen height before display driver inited!\n");
		return 0;
	}
}
UINT32 DISP_GetActiveHeight(void)
{
    disp_drv_init_context();
	if(lcm_params)
	{
	    printk("[wwy]lcm_parms->active_height = %d\n",lcm_params->active_height);
    	return lcm_params->active_height;
	}
	else
	{
		printk("WARNING!! get active_height before display driver inited!\n");
		return 0;
	}
}

UINT32 DISP_GetActiveWidth(void)
{
    disp_drv_init_context();
	if(lcm_params)
	{
	    printk("[wwy]lcm_parms->active_width = %d\n",lcm_params->active_width);
    	return lcm_params->active_width;
	}
	else
	{
		printk("WARNING!! get active_width before display driver inited!\n");
		return 0;
	}
}

DISP_STATUS DISP_SetScreenBpp(UINT32 bpp)
{
    ASSERT(bpp != 0);

    if( bpp != 16   &&          \
        bpp != 24   &&          \
        bpp != 32   &&          \
        1 )
    {
		DISP_LOG("DISP_SetScreenBpp error, not support %d bpp\n", bpp);
        return DISP_STATUS_ERROR;
    }

    disp_fb_bpp = bpp;
    DISP_LOG("DISP_SetScreenBpp %d bpp\n", bpp);

	return DISP_STATUS_OK; 
}

UINT32 DISP_GetScreenBpp(void)
{
	return disp_fb_bpp; 
}

DISP_STATUS DISP_SetPages(UINT32 pages)
{
    ASSERT(pages != 0);

    disp_fb_pages = pages;
    DISP_LOG("DISP_SetPages %d pages\n", pages);

	return DISP_STATUS_OK; 
}
extern u32 get_devinfo_with_index(u32 index);
UINT32 DISP_GetPages(void)
{
    u32 isHwSupportTripleBuffer = 0;

    if(is_page_set==FALSE)
    {
        isHwSupportTripleBuffer = get_devinfo_with_index(3) & 0x00000007;

        if(isHwSupportTripleBuffer == 0x1)
        {
        	printk("[K]DISP_GetPages: hw support!\n");
            disp_fb_pages = 3;
        }
        else
        {
        	printk("[K]DISP_GetPages: hw not support!\n");
            disp_fb_pages = 2;
        }

        is_page_set = TRUE;
    }

    printk("[K]DISP_GetPages: disp_fb_pages=%d!\n", disp_fb_pages);
    
    return disp_fb_pages;   // Double Buffers
}


BOOL DISP_IsDirectLinkMode(void)
{
	return (-1 != direct_link_layer) ? TRUE : FALSE;
}


BOOL DISP_IsInOverlayMode(void)
{
	return is_overlaying;
}

UINT32 DISP_GetFBRamSize(void)
{
    return ALIGN_TO(DISP_GetScreenWidth(), 32) * 
           ALIGN_TO(DISP_GetScreenHeight(), 32) * 
           ((DISP_GetScreenBpp() + 7) >> 3) * 
           DISP_GetPages();
}


UINT32 DISP_GetVRamSize(void)
{
	// Use a local static variable to cache the calculated vram size
	//    
	static UINT32 vramSize = 0;

	if (0 == vramSize)
	{
		disp_drv_init_context();

        ///get framebuffer size
		vramSize = DISP_GetFBRamSize();

        ///get DXI working buffer size
		vramSize += disp_drv->get_working_buffer_size();

        // get assertion layer buffer size
        vramSize += DAL_GetLayerSize();
        
		// Align vramSize to 1MB
		//
		vramSize = ALIGN_TO_POW_OF_2(vramSize, 0x100000);

		DISP_LOG("DISP_GetVRamSize: %u bytes\n", vramSize);
	}

	return vramSize;
}

UINT32 DISP_GetVRamSizeBoot(char *cmdline)
{
	static UINT32 vramSize = 0;

	if(vramSize)
	{
		return vramSize;
	}

	disp_get_lcm_name_boot(cmdline);

	// if can't get the lcm type from uboot, we will return 0x800000 for a safe value
	if(disp_drv)
	    vramSize = DISP_GetVRamSize();
	else
	{
		printk("%s, can't get lcm type, reserved memory size will be set as 0x800000\n", __func__);
		return 0xC00000;
    }   
    // Align vramSize to 1MB
    //
    vramSize = ALIGN_TO_POW_OF_2(vramSize, 0x100000);

    printk("DISP_GetVRamSizeBoot: %u bytes[%dMB]\n", vramSize, (vramSize>>20));

    return vramSize;
}



PANEL_COLOR_FORMAT DISP_GetPanelColorFormat(void)
{
	disp_drv_init_context();

	return (disp_drv->get_panel_color_format) ?
		(disp_drv->get_panel_color_format()) :
		DISP_STATUS_NOT_IMPLEMENTED;
}

UINT32 DISP_GetPanelBPP(void)
{
	PANEL_COLOR_FORMAT fmt;
	disp_drv_init_context();

	if(disp_drv->get_panel_color_format == NULL) 
	{
		return DISP_STATUS_NOT_IMPLEMENTED;
	}

	fmt = disp_drv->get_panel_color_format();
	switch(fmt)
	{
		case PANEL_COLOR_FORMAT_RGB332:
			return 8;
		case PANEL_COLOR_FORMAT_RGB444:
			return 12;
		case PANEL_COLOR_FORMAT_RGB565:
			return 16;
		case PANEL_COLOR_FORMAT_RGB666:
			return 18;
		case PANEL_COLOR_FORMAT_RGB888:
			return 24;
		default:
			return 0;
	}
}

UINT32 DISP_GetOutputBPPforDithering(void)
{
	disp_drv_init_context();

	return (disp_drv->get_dithering_bpp) ?
		(disp_drv->get_dithering_bpp()) :
		DISP_STATUS_NOT_IMPLEMENTED;
}

DISP_STATUS DISP_Config_Overlay_to_Memory(unsigned int mva, int enable)
{
//	int ret = 0;

//	struct disp_path_config_mem_out_struct mem_out = {0};

	if(enable)
	{
		MemOutConfig.outFormat = WDMA_OUTPUT_FORMAT_RGB888;

		MemOutConfig.enable = 1;
		MemOutConfig.dstAddr = mva;
		MemOutConfig.srcROI.x = 0;
		MemOutConfig.srcROI.y = 0;
		MemOutConfig.srcROI.height= DISP_GetScreenHeight();
		MemOutConfig.srcROI.width= DISP_GetScreenWidth();

#if !defined(MTK_WFD_SUPPORT) && !defined(MTK_HDMI_SUPPORT)
		mutex_lock(&MemOutSettingMutex);
		MemOutConfig.dirty = 1;
		mutex_unlock(&MemOutSettingMutex);
#endif
	}
	else
	{
		MemOutConfig.outFormat = WDMA_OUTPUT_FORMAT_RGB888;
		MemOutConfig.enable = 0;
		MemOutConfig.dstAddr = mva;
		MemOutConfig.srcROI.x = 0;
		MemOutConfig.srcROI.y = 0;
		MemOutConfig.srcROI.height= DISP_GetScreenHeight();
		MemOutConfig.srcROI.width= DISP_GetScreenWidth();

//#if !defined(MTK_WFD_SUPPORT) && !defined(MTK_HDMI_SUPPORT)
		mutex_lock(&MemOutSettingMutex);
		MemOutConfig.dirty = 1;		
		mutex_unlock(&MemOutSettingMutex);
//#endif

		// Wait for reg update.
		wait_event_interruptible(reg_update_wq, !MemOutConfig.dirty);
	}

	return DSI_STATUS_OK;
}


DISP_STATUS DISP_Config_Wfd_Overlay_to_Memory(unsigned int mva, int enable)
{
//	int ret = 0;

	if(enable)
	{
		MemOutConfig.outFormat = WDMA_OUTPUT_FORMAT_UYVY;///WDMA_OUTPUT_FORMAT_YUY2;

		MemOutConfig.enable = 1;
		MemOutConfig.dstAddr = mva;
		MemOutConfig.srcROI.x = 0;
		MemOutConfig.srcROI.y = 0;
		MemOutConfig.srcROI.height= DISP_GetScreenHeight();
		MemOutConfig.srcROI.width= DISP_GetScreenWidth();

#if !defined(MTK_WFD_SUPPORT) && !defined(MTK_HDMI_SUPPORT)
		mutex_lock(&MemOutSettingMutex);
		MemOutConfig.dirty = 1;
		mutex_unlock(&MemOutSettingMutex);
#endif
	}
	else
	{
		MemOutConfig.outFormat = WDMA_OUTPUT_FORMAT_UYVY;///WDMA_OUTPUT_FORMAT_YUY2;
		MemOutConfig.enable = 0;
		MemOutConfig.dstAddr = mva;
		MemOutConfig.srcROI.x = 0;
		MemOutConfig.srcROI.y = 0;
		MemOutConfig.srcROI.height= DISP_GetScreenHeight();
		MemOutConfig.srcROI.width= DISP_GetScreenWidth();

//#if !defined(MTK_WFD_SUPPORT) && !defined(MTK_HDMI_SUPPORT)
		mutex_lock(&MemOutSettingMutex);
		MemOutConfig.dirty = 1;		
		mutex_unlock(&MemOutSettingMutex);
//#endif

		// Wait for reg update.
		wait_event_interruptible(reg_update_wq, !MemOutConfig.dirty);
	}

	return DSI_STATUS_OK;
}	

DISP_STATUS HDMI_Config_Overlay_to_Memory(unsigned int mva, int enable, unsigned int moutFormat)
{
    //  int ret = 0;

    //  struct disp_path_config_mem_out_struct mem_out = {0};
	MemOutConfig.outFormat = moutFormat;
    if (enable)
    {

        MemOutConfig.enable = 1;
        MemOutConfig.dstAddr = mva;
        MemOutConfig.srcROI.x = 0;
        MemOutConfig.srcROI.y = 0;
        MemOutConfig.srcROI.height = DISP_GetScreenHeight();
        MemOutConfig.srcROI.width = DISP_GetScreenWidth();

#if !defined(MTK_WFD_SUPPORT) && !defined(MTK_HDMI_SUPPORT)
        mutex_lock(&MemOutSettingMutex);
        MemOutConfig.dirty = 1;
        mutex_unlock(&MemOutSettingMutex);
#endif
    }
    else
    {
        MemOutConfig.enable = 0;
        MemOutConfig.dstAddr = mva;
        MemOutConfig.srcROI.x = 0;
        MemOutConfig.srcROI.y = 0;
        MemOutConfig.srcROI.height = DISP_GetScreenHeight();
        MemOutConfig.srcROI.width = DISP_GetScreenWidth();

        //#if !defined(MTK_WFD_SUPPORT) && !defined(MTK_HDMI_SUPPORT)
        mutex_lock(&MemOutSettingMutex);
        MemOutConfig.dirty = 1;
        mutex_unlock(&MemOutSettingMutex);
        //#endif

        // Wait for reg update.
        wait_event_interruptible(reg_update_wq, !MemOutConfig.dirty);
    }

	printk("HDMI_Config_Overlay_to_Memory done: %d\n",enable);
    return DSI_STATUS_OK;
}

DISP_STATUS DISP_Capture_Framebuffer( unsigned int pvbuf, unsigned int bpp, unsigned int is_early_suspended )
{
    unsigned int mva;
    unsigned int ret = 0;
    M4U_PORT_STRUCT portStruct;
	DISP_FUNC();
	int i;
    for (i=0; i<OVL_LAYER_NUM; i++)
    {
        if (cached_layer_config[i].layer_en && cached_layer_config[i].security)
            break;
    }
    if (i < OVL_LAYER_NUM)
    {
        // There is security layer.
        memset(pvbuf, 0, DISP_GetScreenHeight()*DISP_GetScreenWidth()*bpp/8);
        return DISP_STATUS_OK;
    }
	disp_drv_init_context();
	disp_module_clock_on(DISP_MODULE_WDMA1, "Screen Capture");

    if (is_early_suspended)
    {
        if(lcm_params->type==LCM_TYPE_DSI)
        {
            DSI_SetMode(lcm_params->dsi.mode);
        }
    }

    MMProfileLogEx(MTKFB_MMP_Events.CaptureFramebuffer, MMProfileFlagPulse, 0, pvbuf);
    MMProfileLogEx(MTKFB_MMP_Events.CaptureFramebuffer, MMProfileFlagPulse, 1, bpp);
    ret = m4u_alloc_mva(M4U_CLNTMOD_WDMA, 
        pvbuf, 
        DISP_GetScreenHeight()*DISP_GetScreenWidth()*bpp/8, 
        0,
        0,
        &mva);
    if(ret!=0)
    {
		disp_module_clock_off(DISP_MODULE_WDMA1, "Screen Capture");    
        printk("m4u_alloc_mva() fail! \n");
        return DSI_STATUS_OK;  
    }

    m4u_dma_cache_maint(M4U_CLNTMOD_WDMA, 
        (const void *)pvbuf, 
        DISP_GetScreenHeight()*DISP_GetScreenWidth()*bpp/8,
        DMA_BIDIRECTIONAL);

    portStruct.ePortID = M4U_PORT_WDMA1;		   //hardware port ID, defined in M4U_PORT_ID_ENUM
    portStruct.Virtuality = 1;						   
    portStruct.Security = 0;
    portStruct.domain = 0;            //domain : 0 1 2 3
    portStruct.Distance = 1;
    portStruct.Direction = 0; 	
    m4u_config_port(&portStruct);

    mutex_lock(&MemOutSettingMutex);
    if(bpp == 32)
        MemOutConfig.outFormat = WDMA_OUTPUT_FORMAT_ARGB;
    else if(bpp == 16)
        MemOutConfig.outFormat = WDMA_OUTPUT_FORMAT_RGB565;
    else if(bpp == 24)
        MemOutConfig.outFormat = WDMA_OUTPUT_FORMAT_RGB888;
    else
    {
        printk("DSI_Capture_FB, fb color format not support\n");
        MemOutConfig.outFormat = WDMA_OUTPUT_FORMAT_RGB888;
    }

    MemOutConfig.enable = 1;
    MemOutConfig.dstAddr = mva;
    MemOutConfig.srcROI.x = 0;
    MemOutConfig.srcROI.y = 0;
    MemOutConfig.srcROI.height= DISP_GetScreenHeight();
    MemOutConfig.srcROI.width= DISP_GetScreenWidth();
    if (is_early_suspended == 0)
        MemOutConfig.dirty = 1;
    mutex_unlock(&MemOutSettingMutex);
    MMProfileLogEx(MTKFB_MMP_Events.CaptureFramebuffer, MMProfileFlagPulse, 2, mva);
    if (is_early_suspended)
    {
        disp_path_get_mutex();
        disp_path_config_mem_out_without_lcd(&MemOutConfig);
        disp_path_release_mutex();
        // Wait for mem out done.
        disp_path_wait_mem_out_done();
        MMProfileLogEx(MTKFB_MMP_Events.CaptureFramebuffer, MMProfileFlagPulse, 3, 0);
        MemOutConfig.enable = 0;
        if (lcm_params->type == LCM_TYPE_DPI)
            disp_path_get_mutex();
        disp_path_config_mem_out_without_lcd(&MemOutConfig);
        if (lcm_params->type == LCM_TYPE_DPI)
            disp_path_release_mutex();
    }
    else
    {
        // Wait for mem out done.
        disp_path_wait_mem_out_done();
        MMProfileLogEx(MTKFB_MMP_Events.CaptureFramebuffer, MMProfileFlagPulse, 3, 0);

        mutex_lock(&MemOutSettingMutex);
        MemOutConfig.enable = 0;
        MemOutConfig.dirty = 1;
        mutex_unlock(&MemOutSettingMutex);
        // Wait for reg update.
        wait_event_interruptible(reg_update_wq, !MemOutConfig.dirty);
        MMProfileLogEx(MTKFB_MMP_Events.CaptureFramebuffer, MMProfileFlagPulse, 4, 0);
    }

    BOOL deconfigWdma = TRUE;

#if defined(MTK_HDMI_SUPPORT)
    deconfigWdma &= !is_hdmi_active();
#endif

#if defined(MTK_WFD_SUPPORT)
    deconfigWdma &= !is_wfd_active();
#endif

    if(deconfigWdma)
    {
        portStruct.ePortID = M4U_PORT_WDMA1;		   //hardware port ID, defined in M4U_PORT_ID_ENUM
        portStruct.Virtuality = 0;
        portStruct.Security = 0;
        portStruct.domain = 0;			  //domain : 0 1 2 3
        portStruct.Distance = 1;
        portStruct.Direction = 0;
        m4u_config_port(&portStruct);
    }

    m4u_dealloc_mva(M4U_CLNTMOD_WDMA, 
        pvbuf, 
        DISP_GetScreenHeight()*DISP_GetScreenWidth()*bpp/8, 
        mva);

	disp_module_clock_off(DISP_MODULE_WDMA1, "Screen Capture");    

	return DISP_STATUS_OK;
}

DISP_STATUS DISP_Capture_Videobuffer(unsigned int pvbuf, unsigned int bpp, unsigned int video_rotation)
{
	DISP_FUNC();
	if (down_interruptible(&sem_update_screen)) {
		printk("ERROR: Can't get sem_update_screen in DISP_Capture_Videobuffer()\n");
		return DISP_STATUS_ERROR;
	}
	disp_drv_init_context();
	LCD_Capture_Videobuffer(pvbuf, bpp, video_rotation);
	up(&sem_update_screen);
	return DISP_STATUS_OK;
}
// xuecheng, 2010-09-19
// this api is for mATV signal interfere workaround.
// immediate update == (TE disabled + delay update in overlay mode disabled)
static BOOL is_immediateupdate = false;
DISP_STATUS DISP_ConfigImmediateUpdate(BOOL enable)
{
	disp_drv_init_context();

	if(enable == TRUE)
	{
		LCD_TE_Enable(FALSE);
	}
	else
	{
		if(disp_drv->init_te_control)
			disp_drv->init_te_control();
		else
			return DISP_STATUS_NOT_IMPLEMENTED;
	}

	is_immediateupdate = enable;

	return DISP_STATUS_OK;
}

BOOL DISP_IsImmediateUpdate(void)
{
	return is_immediateupdate;
}

DISP_STATUS DISP_FMDesense_Query()
{
	if(LCM_TYPE_DBI == lcm_params->type){//DBI
    	return (DISP_STATUS)LCD_FMDesense_Query();
	}
	else if(LCM_TYPE_DPI == lcm_params->type){//DPI
		return (DISP_STATUS)DPI_FMDesense_Query();
	}
	else if(LCM_TYPE_DSI == lcm_params->type){// DSI
		return (DISP_STATUS)DSI_FMDesense_Query();
	}
	else
	{
	    DISP_LOG("DISP_FMDesense_Query():unknown interface\n");
	}
	
	return DISP_STATUS_ERROR;
}

DISP_STATUS DISP_FM_Desense(unsigned long freq)
{
    DISP_STATUS ret = DISP_STATUS_OK;
    
	if (down_interruptible(&sem_update_screen)) {
		DISP_LOG("ERROR: Can't get sem_update_screen in DISP_FM_Desense()\n");
		return DISP_STATUS_ERROR;
	}
    
	if(LCM_TYPE_DBI == lcm_params->type){//DBI
		DISP_LOG("DISP_FM_Desense():DBI interface\n");        
		LCD_CHECK_RET(LCD_FM_Desense(ctrl_if, freq));
	}
	else if(LCM_TYPE_DPI == lcm_params->type){//DPI
	    DISP_LOG("DISP_FM_Desense():DPI interface\n");
		DPI_CHECK_RET(DPI_FM_Desense(freq));
	}
	else if(LCM_TYPE_DSI == lcm_params->type){// DSI
	    DISP_LOG("DISP_FM_Desense():DSI interface\n");
		DSI_CHECK_RET(DSI_FM_Desense(freq));
	}
	else
	{
	    DISP_LOG("DISP_FM_Desense():unknown interface\n");
	    ret = DISP_STATUS_ERROR;
	}

	up(&sem_update_screen);
	return ret;
}

DISP_STATUS DISP_Reset_Update()
{
    DISP_STATUS ret = DISP_STATUS_OK;

	if (down_interruptible(&sem_update_screen)) {
		DISP_LOG("ERROR: Can't get sem_update_screen in DISP_Reset_Update()\n");
		return DISP_STATUS_ERROR;
	}
    
	if(LCM_TYPE_DBI == lcm_params->type){//DBI
		DISP_LOG("DISP_Reset_Update():DBI interface\n");        
		LCD_CHECK_RET(LCD_Reset_WriteCycle(ctrl_if));
	}
	else if(LCM_TYPE_DPI == lcm_params->type){//DPI
	    DISP_LOG("DISP_Reset_Update():DPI interface\n");
		DPI_CHECK_RET(DPI_Reset_CLK());
	}
	else if(LCM_TYPE_DSI == lcm_params->type){// DSI
	    DISP_LOG("DISP_Reset_Update():DSI interface\n");
	    DSI_CHECK_RET(DSI_Reset_CLK());
	}
	else
	{
	    DISP_LOG("DISP_Reset_Update():unknown interface\n");
      ret = DISP_STATUS_ERROR;
	}

	up(&sem_update_screen);
	
	return ret;
}

DISP_STATUS DISP_Get_Default_UpdateSpeed(unsigned int *speed)
{
    DISP_STATUS ret = DISP_STATUS_OK;
    if(LCM_TYPE_DBI == lcm_params->type){//DBI
		DISP_LOG("DISP_Get_Default_UpdateSpeed():DBI interface\n");        
		LCD_CHECK_RET(LCD_Get_Default_WriteCycle(ctrl_if, speed));
	}
	else if(LCM_TYPE_DPI == lcm_params->type){//DPI
	    DISP_LOG("DISP_Get_Default_UpdateSpeed():DPI interface\n");
		DPI_CHECK_RET(DPI_Get_Default_CLK(speed));
	}
	else  if(LCM_TYPE_DSI == lcm_params->type){// DSI
	    DISP_LOG("DISP_Get_Default_UpdateSpeed():DSI interface\n");
	    DSI_CHECK_RET(DSI_Get_Default_CLK(speed));
	}
	else
	{
	    DISP_LOG("DISP_Reset_Update():unknown interface\n");
      ret = DISP_STATUS_ERROR;
	}

    return ret;
}

DISP_STATUS DISP_Get_Current_UpdateSpeed(unsigned int *speed)
{
    DISP_STATUS ret = DISP_STATUS_OK;
    if(LCM_TYPE_DBI == lcm_params->type){//DBI
		DISP_LOG("DISP_Get_Current_UpdateSpeed():DBI interface\n");        
		LCD_CHECK_RET(LCD_Get_Current_WriteCycle(ctrl_if, speed));
	}
	else if(LCM_TYPE_DPI == lcm_params->type){//DPI
	    DISP_LOG("DISP_Get_Current_UpdateSpeed():DPI interface\n");
		DPI_CHECK_RET(DPI_Get_Current_CLK(speed));
	}
	else if(LCM_TYPE_DSI == lcm_params->type){// DSI
	    DISP_LOG("DISP_Get_Current_UpdateSpeed():DSI interface\n");
	    DSI_CHECK_RET(DSI_Get_Current_CLK(speed));
	}
	else
	{
	    DISP_LOG("DISP_Reset_Update():unknown interface\n");
      ret = DISP_STATUS_ERROR;
	}

    return ret;
}

DISP_STATUS DISP_Change_Update(unsigned int speed)
{
    DISP_STATUS ret = DISP_STATUS_OK;

	if (down_interruptible(&sem_update_screen)) {
		DISP_LOG("ERROR: Can't get sem_update_screen in DISP_Change_Update()\n");
		return DISP_STATUS_ERROR;
	}
    
	if(LCM_TYPE_DBI == lcm_params->type){//DBI
		DISP_LOG("DISP_Change_Update():DBI interface\n");        
		LCD_CHECK_RET(LCD_Change_WriteCycle(ctrl_if, speed));
	}
	else if(LCM_TYPE_DPI == lcm_params->type){//DPI
	    DISP_LOG("DISP_Change_Update():DPI interface\n");
		DPI_CHECK_RET(DPI_Change_CLK(speed));
	}
	else if(LCM_TYPE_DSI == lcm_params->type){// DSI
	    DISP_LOG("DISP_Change_Update():DSI interface\n");
		DSI_CHECK_RET(DSI_Change_CLK(speed));
	}
	else
	{
	    DISP_LOG("DISP_Reset_Update():unknown interface\n");
      ret = DISP_STATUS_ERROR;
	}

	up(&sem_update_screen);

	return ret;
}

#if defined(MTK_M4U_SUPPORT)
DISP_STATUS DISP_InitM4U()
{
    LCD_InitM4U();
	if(LCM_TYPE_DPI == lcm_params->type){
//	    DPI_InitM4U();     //DPI not use m4u currently
	}
	return DISP_STATUS_OK;
}

DISP_STATUS DISP_ConfigAssertLayerMva()
{
    unsigned int mva;
	ASSERT(dal_layerPA);
    LCD_CHECK_RET(LCD_AllocUIMva(dal_layerPA, &mva, DAL_GetLayerSize())); 
	DISP_LOG("DAL Layer PA = DAL_layerPA = 0x%x, MVA = 0x%x\n", dal_layerPA, mva);
	LCD_CHECK_RET(LCD_LayerSetAddress(ASSERT_LAYER, mva));
	return DISP_STATUS_OK;
}

DISP_STATUS DISP_AllocUILayerMva(unsigned int pa, unsigned int *mva, unsigned int size)
{
    LCD_CHECK_RET(LCD_AllocUIMva(pa, mva, size)); 
	DISP_LOG("UI Layer PA = 0x%x, MVA = 0x%x\n", pa, *mva);
	if(LCM_TYPE_DPI == lcm_params->type){ //dpi buffer
		unsigned int i,dpi_mva;
		UINT32 dpi_size = DISP_GetScreenWidth()*DISP_GetScreenHeight()*disp_drv->get_working_buffer_bpp();
		for(i=0;i<lcm_params->dpi.intermediat_buffer_num;i++){
			LCD_AllocUIMva(pa + size + i * dpi_size, &dpi_mva, dpi_size);
			DISP_LOG("dpi pa = 0x%x,size = %d, mva = 0x%x\n", pa + size + i * dpi_size, dpi_size, dpi_mva);
			LCD_CHECK_RET(LCD_FBSetAddress(LCD_FB_0 + i, dpi_mva));
		}
	}

	if((LCM_TYPE_DSI == lcm_params->type) && (lcm_params->dsi.mode != CMD_MODE)){ //dsi buffer
		unsigned int i,dsi_mva;
		UINT32 dsi_size = DISP_GetScreenWidth()*DISP_GetScreenHeight()*disp_drv->get_working_buffer_bpp();
		for(i=0;i<lcm_params->dsi.intermediat_buffer_num;i++){
			LCD_AllocUIMva(pa + size + i * dsi_size, &dsi_mva, dsi_size);
			DISP_LOG("dsi pa = 0x%x,size = %d, mva = 0x%x\n", pa + size + i * dsi_size, dsi_size, dsi_mva);
			LCD_CHECK_RET(LCD_FBSetAddress(LCD_FB_0 + i, dsi_mva));
		}
	}
	return DISP_STATUS_OK;
}

DISP_STATUS DISP_AllocOverlayMva(unsigned int va, unsigned int *mva, unsigned int size)
{
    LCD_CHECK_RET(LCD_AllocOverlayMva(va, mva, size));
	return DISP_STATUS_OK;
}

DISP_STATUS DISP_DeallocMva(unsigned int va, unsigned int mva, unsigned int size)
{
    LCD_DeallocMva(va, mva, size);
    return DISP_STATUS_OK;
}

DISP_STATUS DISP_M4U_On(BOOL enable)
{
	LCD_M4U_On(enable);

	return DISP_STATUS_OK;
}

DISP_STATUS DISP_DumpM4U(void)
{
    LCD_DumpM4U();
    return DISP_STATUS_OK;
}

#endif

DISP_STATUS DISP_ChangeLCDWriteCycle()// this function is called when bootanimation start and stop in Surfaceflinger on 6573, because 6573 LCDC DBI write speed not stable when bootup
{
#if 0 //defined(only for MT6573)
	if(LCM_TYPE_DBI == lcm_params->type){//DBI
		DISP_LOG("DISP_ChangeLCDWriteCycle():DBI interface\n");        
		LCD_CHECK_RET(LCD_Change_WriteCycle_ex(ctrl_if));
	}
	else{//DPI and DSI has not this issue, so not need change
	    DISP_LOG("DPI or DSI interface not need change write cycles\n");
	}
#endif
    return DISP_STATUS_OK;
}

const char* DISP_GetLCMId(void)
{
    if(lcm_drv)
        return lcm_drv->name;
    else
        return NULL;
}


BOOL DISP_EsdCheck(void)
{
    BOOL result = FALSE;

//<2013/04/12-23801-stevenchen, [Pelican][drv] Implement esd recovery function.
    DSI_TXRX_CTRL_REG tmp_reg;               //add by mtk
    tmp_reg=DSI_REG->DSI_TXRX_CTRL;          //add by mtk
    tmp_reg.HSTX_CKLP_EN = 0;                //add by mtk
    DSI_clk_HS_mode(1);                      //add by mtk
//>2013/04/12-23801-stevenchen

    disp_drv_init_context();
    MMProfileLogEx(MTKFB_MMP_Events.EsdCheck, MMProfileFlagPulse, 0x10, 0);

    if(lcm_drv->esd_check == NULL && disp_drv->esd_check == NULL)
    {
        return FALSE;
    }

    if (down_interruptible(&sem_update_screen)) {
        printk("ERROR: Can't get sem_update_screen in DISP_EsdCheck()\n");
        return FALSE;
    }
    MMProfileLogEx(MTKFB_MMP_Events.EsdCheck, MMProfileFlagPulse, 0x11, 0);

    if(is_lcm_in_suspend_mode)
    {
        up(&sem_update_screen);
        return FALSE;
    }

	if(disp_drv->esd_check)
		result |= disp_drv->esd_check();
    MMProfileLogEx(MTKFB_MMP_Events.EsdCheck, MMProfileFlagPulse, 0x12, 0);

#ifndef MT65XX_NEW_DISP
    LCD_CHECK_RET(LCD_WaitForNotBusy());
    if(lcm_params->type==LCM_TYPE_DSI)
        DSI_CHECK_RET(DSI_WaitForNotBusy());

	if(lcm_drv->esd_check)
    {
        mutex_lock(&LcmCmdMutex);
		result |= lcm_drv->esd_check();
        mutex_unlock(&LcmCmdMutex);
    }
#endif

    up(&sem_update_screen);

//<2013/04/12-23801-stevenchen, [Pelican][drv] Implement esd recovery function.
    tmp_reg.HSTX_CKLP_EN = 1;            //add by mtk
//>2013/04/12-23801-stevenchen

    return result;
}


BOOL DISP_EsdRecoverCapbility(void)
{
    if(!disp_drv_init_context())
        return FALSE;

    if((lcm_drv->esd_check && lcm_drv->esd_recover) || (lcm_params->dsi.lcm_ext_te_monitor) || (lcm_params->dsi.lcm_int_te_monitor))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL DISP_EsdRecover(void)
{
    BOOL result = FALSE;
    DISP_LOG("DISP_EsdRecover enter");

    if(lcm_drv->esd_recover == NULL)
    {
        return FALSE;
    }

    if (down_interruptible(&sem_update_screen)) {
        printk("ERROR: Can't get sem_update_screen in DISP_EsdRecover()\n");
        return FALSE;
    }

    if(is_lcm_in_suspend_mode)
    {
        up(&sem_update_screen);
        return FALSE;
    }

    LCD_CHECK_RET(LCD_WaitForNotBusy());
    if(lcm_params->type==LCM_TYPE_DSI)
    {
        DSI_CHECK_RET(DSI_WaitForNotBusy());        
    }

     DISP_LOG("DISP_EsdRecover do LCM recover");

    // do necessary configuration reset for LCM re-init
    if(disp_drv->esd_reset)
        disp_drv->esd_reset();

    /// LCM recover
    mutex_lock(&LcmCmdMutex);
    result = lcm_drv->esd_recover();
    mutex_unlock(&LcmCmdMutex);

    up(&sem_update_screen);

	DISP_CHECK_RET(DISP_UpdateScreen(0, 0, DISP_GetScreenWidth(), DISP_GetScreenHeight()));
    return result;
}

unsigned long DISP_GetLCMIndex(void)
{
    return u4IndexOfLCMList;
}

DISP_STATUS DISP_PrepareSuspend(void)
{
    if(lcm_params->type==LCM_TYPE_DSI && lcm_params->dsi.mode != CMD_MODE)
    {		
        DSI_SetMode(CMD_MODE);
    }
    return DISP_STATUS_OK;
}

DISP_STATUS DISP_GetLayerInfo(DISP_LAYER_INFO *pLayer)
{
    int id = pLayer->id;
    mutex_lock(&OverlaySettingMutex);
    pLayer->curr_en = captured_layer_config[id].layer_en;
    pLayer->next_en = cached_layer_config[id].layer_en;
    pLayer->hw_en = realtime_layer_config[id].layer_en;
    pLayer->curr_idx = captured_layer_config[id].buff_idx;
    pLayer->next_idx = cached_layer_config[id].buff_idx;
    pLayer->hw_idx = realtime_layer_config[id].buff_idx;
    pLayer->curr_identity = captured_layer_config[id].identity;
    pLayer->next_identity = cached_layer_config[id].identity;
    pLayer->hw_identity = realtime_layer_config[id].identity;
    pLayer->curr_conn_type = captured_layer_config[id].connected_type;
    pLayer->next_conn_type = cached_layer_config[id].connected_type;
    pLayer->hw_conn_type = realtime_layer_config[id].connected_type;
    mutex_unlock(&OverlaySettingMutex);
}
