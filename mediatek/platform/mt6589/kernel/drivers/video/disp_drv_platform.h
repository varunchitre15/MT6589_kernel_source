

#ifndef __DISP_DRV_PLATFORM_H__
#define __DISP_DRV_PLATFORM_H__

#ifdef BUILD_UBOOT
#include <config.h>
#include <common.h>
#include <version.h>
#include <stdarg.h>
#include <linux/types.h>
#include <lcd.h>
#include <video_fb.h>
#include <mmc.h>
#include <part.h>
#include <fat.h>
#include <malloc.h>
#include <asm/errno.h>
#include <asm/io.h>
#include <asm/arch/boot_mode.h>
#include <asm/arch/mt65xx.h>
#include <asm/arch/mt65xx_typedefs.h>
#include <asm/arch/disp_drv.h>
#include <asm/arch/lcd_drv.h>
#include <asm/arch/dpi_drv.h>
#include <asm/arch/dsi_drv.h>
#include <asm/arch/lcd_reg.h>
#include <asm/arch/dpi_reg.h>
#include <asm/arch/dsi_reg.h>
#include <asm/arch/disp_assert_layer.h>
#include <asm/arch/disp_drv_log.h>
#include <asm/arch/mt65xx_disp_drv.h>
#include "lcm_drv.h"


#undef MTK_M4U_SUPPORT
#undef MTK_HDMI_SUPPORT
#define DEFINE_SEMAPHORE(x)  
#define down_interruptible(x) 0
#define up(x)                
#define DBG_OnTriggerLcd()   

#else
#include <linux/dma-mapping.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/m4u.h>
//#include <mach/mt6585_pwm.h>
#include <mach/mt_reg_base.h>
#include <mach/mt_clkmgr.h>
#include <mach/mt_irq.h>
//#include <mach/boot.h>
#include <board-custom.h>
#include <linux/disp_assert_layer.h>
#include "ddp_drv.h"
#include "ddp_path.h"
#include "ddp_rdma.h"
#include "dsi_drv.h"
#endif

///LCD HW feature options for MT6575
#define MTK_LCD_HW_SIF_VERSION      2       ///for MT6575, we naming it is V2 because MT6516/73 is V1...
#define MTKFB_NO_M4U
#define MT65XX_NEW_DISP
//#define MTK_LCD_HW_3D_SUPPORT

#endif //__DISP_DRV_PLATFORM_H__
