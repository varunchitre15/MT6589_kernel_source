/**********************************************************************
 *
 * Copyright(C) 2008 Imagination Technologies Ltd. All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope it will be useful but, except 
 * as otherwise stated in writing, without any warranty; without even the 
 * implied warranty of merchantability or fitness for a particular purpose. 
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 * 
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 * Contact Information:
 * Imagination Technologies Ltd. <gpl-support@imgtec.com>
 * Home Park Estate, Kings Langley, Herts, WD4 8LZ, UK 
 *
 ******************************************************************************/

#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38))
#ifndef AUTOCONF_INCLUDED
#include <linux/config.h>
#endif
#endif

#include <linux/version.h>

#include <asm/atomic.h>

#if defined(SUPPORT_DRI_DRM)
#include <drm/drmP.h>
#else
#include <linux/module.h>
#endif

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/hardirq.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/fb.h>
#include <linux/console.h>
#include <linux/mutex.h>

//#include <mach/vrfb.h>

#if defined(DEBUG)
#define	PVR_DEBUG DEBUG
#undef DEBUG
#endif
#if defined(DEBUG)
#undef DEBUG
#endif
#if defined(PVR_DEBUG)
#define	DEBUG PVR_DEBUG
#undef PVR_DEBUG
#endif

#include "img_defs.h"
#include "servicesext.h"
#include "kerneldisplay.h"
#include "mtklfb.h"
#include "pvrmodule.h"
#if defined(SUPPORT_DRI_DRM)
#include "pvr_drm.h"
#include "3rdparty_dc_drm_shared.h"
#endif

#if !defined(PVR_LINUX_USING_WORKQUEUES)
#error "PVR_LINUX_USING_WORKQUEUES must be defined"
#endif

#ifdef MTK_DEBUG_TIMER_MONITOR
#include <linux/timer.h>
#endif

MODULE_SUPPORTED_DEVICE(DEVNAME);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34))
//#define MTK_DSS_DRIVER(drv, dev) struct omap_dss_driver *drv = (dev) != NULL ? (dev)->driver : NULL
//#define MTK_DSS_MANAGER(man, dev) struct omap_overlay_manager *man = (dev) != NULL ? (dev)->manager : NULL
#define	WAIT_FOR_VSYNC(man)	((man)->wait_for_vsync)
#else
//#define MTK_DSS_DRIVER(drv, dev) struct omap_dss_device *drv = (dev)
//#define MTK_DSS_MANAGER(man, dev) struct omap_dss_device *man = (dev)
#define	WAIT_FOR_VSYNC(man)	((man)->wait_vsync)
#endif

#ifdef MTK_DEBUG_TIMER_MONITOR
static struct timer_list g_sTimer;
static bool g_bEnableMonitor;
#endif

#ifdef MTK_DEBUG_TIMER_MONITOR
static void MTKLFBMonitorHandle(unsigned long ui32Data)
{
	if (g_bEnableMonitor)
	{
		xlog_printk(ANDROID_LOG_ERROR, DRIVER_PREFIX, "MTKLFB: pan display timeout:\n");
	}
}

static void MTKLFBStartMonitor(void)
{
	g_bEnableMonitor = true;
	mod_timer(&g_sTimer, jiffies + msecs_to_jiffies(5000));
}

static void MTKLFBEndMonitor(void)
{
	g_bEnableMonitor = false;
}

static void MTKLFBTimerInit(void)
{
    setup_timer(&g_sTimer, MTKLFBMonitorHandle, 0);
}

static void MTKLFBTimerEnd(void)
{
	del_timer(&g_sTimer);
}
#endif

void *MTKLFBAllocKernelMem(unsigned long ulSize)
{
	return kmalloc(ulSize, GFP_KERNEL);
}

void MTKLFBFreeKernelMem(void *pvMem)
{
	kfree(pvMem);
}

void MTKLFBCreateSwapChainLockInit(MTKLFB_DEVINFO *psDevInfo)
{
	mutex_init(&psDevInfo->sCreateSwapChainMutex);
}

void MTKLFBCreateSwapChainLockDeInit(MTKLFB_DEVINFO *psDevInfo)
{
	mutex_destroy(&psDevInfo->sCreateSwapChainMutex);
}

void MTKLFBCreateSwapChainLock(MTKLFB_DEVINFO *psDevInfo)
{
	mutex_lock(&psDevInfo->sCreateSwapChainMutex);
}

void MTKLFBCreateSwapChainUnLock(MTKLFB_DEVINFO *psDevInfo)
{
	mutex_unlock(&psDevInfo->sCreateSwapChainMutex);
}

void MTKLFBAtomicBoolInit(MTKLFB_ATOMIC_BOOL *psAtomic, MTKLFB_BOOL bVal)
{
	atomic_set(psAtomic, (int)bVal);
}

void MTKLFBAtomicBoolDeInit(MTKLFB_ATOMIC_BOOL *psAtomic)
{
}

void MTKLFBAtomicBoolSet(MTKLFB_ATOMIC_BOOL *psAtomic, MTKLFB_BOOL bVal)
{
	atomic_set(psAtomic, (int)bVal);
}

MTKLFB_BOOL MTKLFBAtomicBoolRead(MTKLFB_ATOMIC_BOOL *psAtomic)
{
	return (MTKLFB_BOOL)atomic_read(psAtomic);
}

void MTKLFBAtomicIntInit(MTKLFB_ATOMIC_INT *psAtomic, int iVal)
{
	atomic_set(psAtomic, iVal);
}

void MTKLFBAtomicIntDeInit(MTKLFB_ATOMIC_INT *psAtomic)
{
}

void MTKLFBAtomicIntSet(MTKLFB_ATOMIC_INT *psAtomic, int iVal)
{
	atomic_set(psAtomic, iVal);
}

int MTKLFBAtomicIntRead(MTKLFB_ATOMIC_INT *psAtomic)
{
	return atomic_read(psAtomic);
}

void MTKLFBAtomicIntInc(MTKLFB_ATOMIC_INT *psAtomic)
{
	atomic_inc(psAtomic);
}

MTKLFB_ERROR MTKLFBGetLibFuncAddr (char *szFunctionName, PFN_DC_GET_PVRJTABLE *ppfnFuncTable)
{
	if(strcmp("PVRGetDisplayClassJTable", szFunctionName) != 0)
	{
		return (MTKLFB_ERROR_INVALID_PARAMS);
	}

	
	*ppfnFuncTable = PVRGetDisplayClassJTable;

	return (MTKLFB_OK);
}

void MTKLFBQueueBufferForSwap(MTKLFB_SWAPCHAIN *psSwapChain, MTKLFB_BUFFER *psBuffer)
{
	int res = queue_work(psSwapChain->psWorkQueue, &psBuffer->sWork);

	if (res == 0)
	{
		xlog_printk(ANDROID_LOG_WARN, DRIVER_PREFIX, DRIVER_PREFIX ": %s: Device %u: Buffer already on work queue\n", __FUNCTION__, psSwapChain->uiFBDevID);
	}
}

static void WorkQueueHandler(struct work_struct *psWork)
{
	MTKLFB_BUFFER *psBuffer = container_of(psWork, MTKLFB_BUFFER, sWork);

	MTKLFBSwapHandler(psBuffer);
}

MTKLFB_ERROR MTKLFBCreateSwapQueue(MTKLFB_SWAPCHAIN *psSwapChain)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
	psSwapChain->psWorkQueue = alloc_ordered_workqueue(DEVNAME, WQ_FREEZABLE | WQ_MEM_RECLAIM);
#else
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
	psSwapChain->psWorkQueue = create_freezable_workqueue(DEVNAME);
#else
	psSwapChain->psWorkQueue = __create_workqueue(DEVNAME, 1, 1, 1);
#endif
#endif
	if (psSwapChain->psWorkQueue == NULL)
	{
		xlog_printk(ANDROID_LOG_ERROR, DRIVER_PREFIX, DRIVER_PREFIX ": %s: Device %u: Couldn't create workqueue\n", __FUNCTION__, psSwapChain->uiFBDevID);

		return (MTKLFB_ERROR_INIT_FAILURE);
	}

	return (MTKLFB_OK);
}

void MTKLFBInitBufferForSwap(MTKLFB_BUFFER *psBuffer)
{
	INIT_WORK(&psBuffer->sWork, WorkQueueHandler);
}

void MTKLFBDestroySwapQueue(MTKLFB_SWAPCHAIN *psSwapChain)
{
	destroy_workqueue(psSwapChain->psWorkQueue);
}

void MTKLFBFlip(MTKLFB_DEVINFO *psDevInfo, MTKLFB_BUFFER *psBuffer)
{
	struct fb_var_screeninfo sFBVar;
	int res;
	unsigned long ulYResVirtual;

	MTKLFB_CONSOLE_LOCK();

#ifdef MTK_DEBUG_TIMER_MONITOR
	MTKLFBStartMonitor();
#endif

	sFBVar = psDevInfo->psLINFBInfo->var;

	sFBVar.xoffset = 0;
	sFBVar.yoffset = psBuffer->ulYOffset;

	ulYResVirtual = psBuffer->ulYOffset + sFBVar.yres;

	
	if (sFBVar.xres_virtual != sFBVar.xres || sFBVar.yres_virtual < ulYResVirtual)
	{
		sFBVar.xres_virtual = sFBVar.xres;
		sFBVar.yres_virtual = ulYResVirtual;

		sFBVar.activate = FB_ACTIVATE_NOW | FB_ACTIVATE_FORCE;

		res = fb_set_var(psDevInfo->psLINFBInfo, &sFBVar);
		if (res != 0)
		{
			xlog_printk(ANDROID_LOG_ERROR, DRIVER_PREFIX, DRIVER_PREFIX ": %s: Device %u: fb_set_var failed (Y Offset: %lu, Error: %d)\n", __FUNCTION__, psDevInfo->uiFBDevID, psBuffer->ulYOffset, res);
		}
	}
	else
	{
		res = fb_pan_display(psDevInfo->psLINFBInfo, &sFBVar);
		if (res != 0)
		{
			xlog_printk(ANDROID_LOG_ERROR, DRIVER_PREFIX, DRIVER_PREFIX ": %s: Device %u: fb_pan_display failed (Y Offset: %lu, Error: %d)\n", __FUNCTION__, psDevInfo->uiFBDevID, psBuffer->ulYOffset, res);
		}
	}

#ifdef MTK_DEBUG_TIMER_MONITOR
	MTKLFBEndMonitor();
#endif

	MTKLFB_CONSOLE_UNLOCK();
}

MTKLFB_UPDATE_MODE MTKLFBGetUpdateMode(MTKLFB_DEVINFO *psDevInfo)
{
	return MTKLFB_UPDATE_MODE_UNDEFINED;
}

MTKLFB_BOOL MTKLFBSetUpdateMode(MTKLFB_DEVINFO *psDevInfo, MTKLFB_UPDATE_MODE eMode)
{
	return MTKLFB_TRUE;
}

MTKLFB_BOOL MTKLFBWaitForVSync(MTKLFB_DEVINFO *psDevInfo)
{
	return MTKLFB_TRUE;
}

MTKLFB_BOOL MTKLFBManualSync(MTKLFB_DEVINFO *psDevInfo)
{
	return MTKLFB_TRUE;
}

MTKLFB_BOOL MTKLFBCheckModeAndSync(MTKLFB_DEVINFO *psDevInfo)
{
	MTKLFB_UPDATE_MODE eMode = MTKLFBGetUpdateMode(psDevInfo);

	switch(eMode)
	{
		case MTKLFB_UPDATE_MODE_AUTO:
		case MTKLFB_UPDATE_MODE_MANUAL:
			return MTKLFBManualSync(psDevInfo);
		default:
			break;
	}

	return MTKLFB_TRUE;
}

static int MTKLFBFrameBufferEvents(struct notifier_block *psNotif,
                             unsigned long event, void *data)
{
	MTKLFB_DEVINFO *psDevInfo;
	struct fb_event *psFBEvent = (struct fb_event *)data;
	struct fb_info *psFBInfo = psFBEvent->info;
	MTKLFB_BOOL bBlanked;

	
	if (event != FB_EVENT_BLANK)
	{
		return 0;
	}

	bBlanked = (*(IMG_INT *)psFBEvent->data != 0) ? MTKLFB_TRUE: MTKLFB_FALSE;

	psDevInfo = MTKLFBGetDevInfoPtr(psFBInfo->node);

#if 0
	if (psDevInfo != NULL)
	{
		if (bBlanked)
		{
			DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": %s: Device %u: Blank event received\n", __FUNCTION__, psDevInfo->uiFBDevID));
		}
		else
		{
			DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": %s: Device %u: Unblank event received\n", __FUNCTION__, psDevInfo->uiFBDevID));
		}
	}
	else
	{
		DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": %s: Device %u: Blank/Unblank event for unknown framebuffer\n", __FUNCTION__, psFBInfo->node));
	}
#endif

	if (psDevInfo != NULL)
	{
		MTKLFBAtomicBoolSet(&psDevInfo->sBlanked, bBlanked);
		MTKLFBAtomicIntInc(&psDevInfo->sBlankEvents);
	}

	return 0;
}

MTKLFB_ERROR MTKLFBUnblankDisplay(MTKLFB_DEVINFO *psDevInfo)
{
	int res;

	MTKLFB_CONSOLE_LOCK();
	res = fb_blank(psDevInfo->psLINFBInfo, 0);
	MTKLFB_CONSOLE_UNLOCK();
	if (res != 0 && res != -EINVAL)
	{
		xlog_printk(ANDROID_LOG_ERROR, DRIVER_PREFIX, DRIVER_PREFIX
			": %s: Device %u: fb_blank failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, res);
		return (MTKLFB_ERROR_GENERIC);
	}

	return (MTKLFB_OK);
}

#ifdef CONFIG_HAS_EARLYSUSPEND

static void MTKLFBBlankDisplay(MTKLFB_DEVINFO *psDevInfo)
{
	MTKLFB_CONSOLE_LOCK();
	fb_blank(psDevInfo->psLINFBInfo, 1);
	MTKLFB_CONSOLE_UNLOCK();
}

static void MTKLFBEarlySuspendHandler(struct early_suspend *h)
{
	unsigned uiMaxFBDevIDPlusOne = MTKLFBMaxFBDevIDPlusOne();
	unsigned i;

	for (i=0; i < uiMaxFBDevIDPlusOne; i++)
	{
		MTKLFB_DEVINFO *psDevInfo = MTKLFBGetDevInfoPtr(i);

		if (psDevInfo != NULL)
		{
			MTKLFBAtomicBoolSet(&psDevInfo->sEarlySuspendFlag, MTKLFB_TRUE);
			MTKLFBBlankDisplay(psDevInfo);
		}
	}
}

static void MTKLFBEarlyResumeHandler(struct early_suspend *h)
{
	unsigned uiMaxFBDevIDPlusOne = MTKLFBMaxFBDevIDPlusOne();
	unsigned i;

	for (i=0; i < uiMaxFBDevIDPlusOne; i++)
	{
		MTKLFB_DEVINFO *psDevInfo = MTKLFBGetDevInfoPtr(i);

		if (psDevInfo != NULL)
		{
			MTKLFBUnblankDisplay(psDevInfo);
			MTKLFBAtomicBoolSet(&psDevInfo->sEarlySuspendFlag, MTKLFB_FALSE);
		}
	}
}

#endif 

MTKLFB_ERROR MTKLFBEnableLFBEventNotification(MTKLFB_DEVINFO *psDevInfo)
{
	int                res;
	MTKLFB_ERROR         eError;

	
	memset(&psDevInfo->sLINNotifBlock, 0, sizeof(psDevInfo->sLINNotifBlock));

	psDevInfo->sLINNotifBlock.notifier_call = MTKLFBFrameBufferEvents;

	MTKLFBAtomicBoolSet(&psDevInfo->sBlanked, MTKLFB_FALSE);
	MTKLFBAtomicIntSet(&psDevInfo->sBlankEvents, 0);

	res = fb_register_client(&psDevInfo->sLINNotifBlock);
	if (res != 0)
	{
		xlog_printk(ANDROID_LOG_ERROR, DRIVER_PREFIX, DRIVER_PREFIX
			": %s: Device %u: fb_register_client failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, res);

		return (MTKLFB_ERROR_GENERIC);
	}

	eError = MTKLFBUnblankDisplay(psDevInfo);
	if (eError != MTKLFB_OK)
	{
		xlog_printk(ANDROID_LOG_ERROR, DRIVER_PREFIX, DRIVER_PREFIX
			": %s: Device %u: UnblankDisplay failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, eError);
		return eError;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	psDevInfo->sEarlySuspend.suspend = MTKLFBEarlySuspendHandler;
	psDevInfo->sEarlySuspend.resume = MTKLFBEarlyResumeHandler;
	psDevInfo->sEarlySuspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 1;
	register_early_suspend(&psDevInfo->sEarlySuspend);
#endif

	return (MTKLFB_OK);
}

MTKLFB_ERROR MTKLFBDisableLFBEventNotification(MTKLFB_DEVINFO *psDevInfo)
{
	int res;

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&psDevInfo->sEarlySuspend);
#endif

	
	res = fb_unregister_client(&psDevInfo->sLINNotifBlock);
	if (res != 0)
	{
		xlog_printk(ANDROID_LOG_ERROR, DRIVER_PREFIX, DRIVER_PREFIX
			": %s: Device %u: fb_unregister_client failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, res);
		return (MTKLFB_ERROR_GENERIC);
	}

	MTKLFBAtomicBoolSet(&psDevInfo->sBlanked, MTKLFB_FALSE);

	return (MTKLFB_OK);
}

#if defined(SUPPORT_DRI_DRM) && defined(PVR_DISPLAY_CONTROLLER_DRM_IOCTL)
static MTKLFB_DEVINFO *MTKLFBPVRDevIDToDevInfo(unsigned uiPVRDevID)
{
	unsigned uiMaxFBDevIDPlusOne = MTKLFBMaxFBDevIDPlusOne();
	unsigned i;

	for (i=0; i < uiMaxFBDevIDPlusOne; i++)
	{
		MTKLFB_DEVINFO *psDevInfo = MTKLFBGetDevInfoPtr(i);

		if (psDevInfo->uiPVRDevID == uiPVRDevID)
		{
			return psDevInfo;
		}
	}

	xlog_printk(ANDROID_LOG_ERROR, DRIVER_PREFIX, DRIVER_PREFIX
		": %s: PVR Device %u: Couldn't find device\n", __FUNCTION__, uiPVRDevID);

	return NULL;
}

int PVR_DRM_MAKENAME(DISPLAY_CONTROLLER, _Ioctl)(struct drm_device unref__ *dev, void *arg, struct drm_file unref__ *pFile)
{
	uint32_t *puiArgs;
	uint32_t uiCmd;
	unsigned uiPVRDevID;
	int ret = 0;
	MTKLFB_DEVINFO *psDevInfo;

	if (arg == NULL)
	{
		return -EFAULT;
	}

	puiArgs = (uint32_t *)arg;
	uiCmd = puiArgs[PVR_DRM_DISP_ARG_CMD];
	uiPVRDevID = puiArgs[PVR_DRM_DISP_ARG_DEV];

	psDevInfo = MTKLFBPVRDevIDToDevInfo(uiPVRDevID);
	if (psDevInfo == NULL)
	{
		return -EINVAL;
	}


	switch (uiCmd)
	{
		case PVR_DRM_DISP_CMD_LEAVE_VT:
		case PVR_DRM_DISP_CMD_ENTER_VT:
		{
			MTKLFB_BOOL bLeaveVT = (uiCmd == PVR_DRM_DISP_CMD_LEAVE_VT);
			DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: PVR Device %u: %s\n",
				__FUNCTION__, uiPVRDevID,
				bLeaveVT ? "Leave VT" : "Enter VT"));

			MTKLFBCreateSwapChainLock(psDevInfo);
			
			MTKLFBAtomicBoolSet(&psDevInfo->sLeaveVT, bLeaveVT);
			if (psDevInfo->psSwapChain != NULL)
			{
				flush_workqueue(psDevInfo->psSwapChain->psWorkQueue);

				if (bLeaveVT)
				{
					MTKLFBFlip(psDevInfo, &psDevInfo->sSystemBuffer);
					(void) MTKLFBCheckModeAndSync(psDevInfo);
				}
			}

			MTKLFBCreateSwapChainUnLock(psDevInfo);
			(void) MTKLFBUnblankDisplay(psDevInfo);
			break;
		}
		case PVR_DRM_DISP_CMD_ON:
		case PVR_DRM_DISP_CMD_STANDBY:
		case PVR_DRM_DISP_CMD_SUSPEND:
		case PVR_DRM_DISP_CMD_OFF:
		{
			int iFBMode;
#if defined(DEBUG)
			{
				const char *pszMode;
				switch(uiCmd)
				{
					case PVR_DRM_DISP_CMD_ON:
						pszMode = "On";
						break;
					case PVR_DRM_DISP_CMD_STANDBY:
						pszMode = "Standby";
						break;
					case PVR_DRM_DISP_CMD_SUSPEND:
						pszMode = "Suspend";
						break;
					case PVR_DRM_DISP_CMD_OFF:
						pszMode = "Off";
						break;
					default:
						pszMode = "(Unknown Mode)";
						break;
				}
				xlog_printk(ANDROID_LOG_WARN, DRIVER_PREFIX, DRIVER_PREFIX ": %s: PVR Device %u: Display %s\n",
				__FUNCTION__, uiPVRDevID, pszMode);
			}
#endif
			switch(uiCmd)
			{
				case PVR_DRM_DISP_CMD_ON:
					iFBMode = FB_BLANK_UNBLANK;
					break;
				case PVR_DRM_DISP_CMD_STANDBY:
					iFBMode = FB_BLANK_HSYNC_SUSPEND;
					break;
				case PVR_DRM_DISP_CMD_SUSPEND:
					iFBMode = FB_BLANK_VSYNC_SUSPEND;
					break;
				case PVR_DRM_DISP_CMD_OFF:
					iFBMode = FB_BLANK_POWERDOWN;
					break;
				default:
					return -EINVAL;
			}

			MTKLFBCreateSwapChainLock(psDevInfo);

			if (psDevInfo->psSwapChain != NULL)
			{
				flush_workqueue(psDevInfo->psSwapChain->psWorkQueue);
			}

			MTKLFB_CONSOLE_LOCK();
			ret = fb_blank(psDevInfo->psLINFBInfo, iFBMode);
			MTKLFB_CONSOLE_UNLOCK();

			MTKLFBCreateSwapChainUnLock(psDevInfo);

			break;
		}
		default:
		{
			ret = -EINVAL;
			break;
		}
	}

	return ret;
}
#endif

#if defined(SUPPORT_DRI_DRM)
int PVR_DRM_MAKENAME(DISPLAY_CONTROLLER, _Init)(struct drm_device unref__ *dev)
#else
static int __init MTKLFB_Init(void)
#endif
{

	if(MTKLFBInit() != MTKLFB_OK)
	{
		xlog_printk(ANDROID_LOG_ERROR, DRIVER_PREFIX, DRIVER_PREFIX ": %s: MTKLFBInit failed\n", __FUNCTION__);
		return -ENODEV;
	}

#ifdef MTK_DEBUG_TIMER_MONITOR
	MTKLFBTimerInit();
#endif

#ifdef MTK_DEBUG_LFB
	dbg_init();
#endif

	return 0;

}

#if defined(SUPPORT_DRI_DRM)
void PVR_DRM_MAKENAME(DISPLAY_CONTROLLER, _Cleanup)(struct drm_device unref__ *dev)
#else
static void __exit MTKLFB_Cleanup(void)
#endif
{    
	if(MTKLFBDeInit() != MTKLFB_OK)
	{
		xlog_printk(ANDROID_LOG_ERROR, DRIVER_PREFIX, DRIVER_PREFIX ": %s: MTKLFBDeInit failed\n", __FUNCTION__);
	}

#ifdef MTK_DEBUG_LFB
	dbg_exit();
#endif

#ifdef MTK_DEBUG_TIMER_MONITOR
	MTKLFBTimerEnd();
#endif
}

#if !defined(SUPPORT_DRI_DRM)
late_initcall(MTKLFB_Init);
module_exit(MTKLFB_Cleanup);
#endif
