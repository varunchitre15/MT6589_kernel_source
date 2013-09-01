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
#include <linux/kernel.h>
#include <linux/console.h>
#include <linux/fb.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/notifier.h>

#include "img_defs.h"
#include "servicesext.h"
#include "kerneldisplay.h"
#include "mtklfb.h"

#define MTKLFB_COMMAND_COUNT		1

#define	MTKLFB_VSYNC_SETTLE_COUNT	5

#define	MTKLFB_MAX_NUM_DEVICES		FB_MAX
#if (MTKLFB_MAX_NUM_DEVICES > FB_MAX)
#error "MTKLFB_MAX_NUM_DEVICES must not be greater than FB_MAX"
#endif

static MTKLFB_DEVINFO *gapsDevInfo[MTKLFB_MAX_NUM_DEVICES];

static PFN_DC_GET_PVRJTABLE gpfnGetPVRJTable = NULL;

static inline unsigned long RoundUpToMultiple(unsigned long x, unsigned long y)
{
	unsigned long div = x / y;
	unsigned long rem = x % y;

	return (div + ((rem == 0) ? 0 : 1)) * y;
}

static unsigned long GCD(unsigned long x, unsigned long y)
{
	while (y != 0)
	{
		unsigned long r = x % y;
		x = y;
		y = r;
	}

	return x;
}

static unsigned long LCM(unsigned long x, unsigned long y)
{
	unsigned long gcd = GCD(x, y);

	return (gcd == 0) ? 0 : ((x / gcd) * y);
}

unsigned MTKLFBMaxFBDevIDPlusOne(void)
{
	return MTKLFB_MAX_NUM_DEVICES;
}

MTKLFB_DEVINFO *MTKLFBGetDevInfoPtr(unsigned uiFBDevID)
{
	WARN_ON(uiFBDevID >= MTKLFBMaxFBDevIDPlusOne());

	if (uiFBDevID >= MTKLFB_MAX_NUM_DEVICES)
	{
		return NULL;
	}

	return gapsDevInfo[uiFBDevID];
}

static inline void MTKLFBSetDevInfoPtr(unsigned uiFBDevID, MTKLFB_DEVINFO *psDevInfo)
{
	WARN_ON(uiFBDevID >= MTKLFB_MAX_NUM_DEVICES);

	if (uiFBDevID < MTKLFB_MAX_NUM_DEVICES)
	{
		gapsDevInfo[uiFBDevID] = psDevInfo;
	}
}

static inline MTKLFB_BOOL SwapChainHasChanged(MTKLFB_DEVINFO *psDevInfo, MTKLFB_SWAPCHAIN *psSwapChain)
{
	return (psDevInfo->psSwapChain != psSwapChain) ||
		(psDevInfo->uiSwapChainID != psSwapChain->uiSwapChainID);
}

static inline MTKLFB_BOOL DontWaitForVSync(MTKLFB_DEVINFO *psDevInfo)
{
	MTKLFB_BOOL bDontWait;

	bDontWait = MTKLFBAtomicBoolRead(&psDevInfo->sBlanked) ||
			MTKLFBAtomicBoolRead(&psDevInfo->sFlushCommands);

#if defined(CONFIG_HAS_EARLYSUSPEND)
	bDontWait = bDontWait || MTKLFBAtomicBoolRead(&psDevInfo->sEarlySuspendFlag);
#endif
#if defined(SUPPORT_DRI_DRM)
	bDontWait = bDontWait || MTKLFBAtomicBoolRead(&psDevInfo->sLeaveVT);
#endif
	return bDontWait;
}

static IMG_VOID SetDCState(IMG_HANDLE hDevice, IMG_UINT32 ui32State)
{
	MTKLFB_DEVINFO *psDevInfo = (MTKLFB_DEVINFO *)hDevice;

	switch (ui32State)
	{
		case DC_STATE_FLUSH_COMMANDS:
			MTKLFBAtomicBoolSet(&psDevInfo->sFlushCommands, MTKLFB_TRUE);
			break;
		case DC_STATE_NO_FLUSH_COMMANDS:
			MTKLFBAtomicBoolSet(&psDevInfo->sFlushCommands, MTKLFB_FALSE);
			break;
		default:
			break;
	}
}

static PVRSRV_ERROR OpenDCDevice(IMG_UINT32 uiPVRDevID,
                                 IMG_HANDLE *phDevice,
                                 PVRSRV_SYNC_DATA* psSystemBufferSyncData)
{
	MTKLFB_DEVINFO *psDevInfo;
	MTKLFB_ERROR eError;
	unsigned uiMaxFBDevIDPlusOne = MTKLFBMaxFBDevIDPlusOne();
	unsigned i;

	for (i = 0; i < uiMaxFBDevIDPlusOne; i++)
	{
		psDevInfo = MTKLFBGetDevInfoPtr(i);
		if (psDevInfo != NULL && psDevInfo->uiPVRDevID == uiPVRDevID)
		{
			break;
		}
	}
	if (i == uiMaxFBDevIDPlusOne)
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX
			": %s: PVR Device %u not found\n", __FUNCTION__, uiPVRDevID));
		return PVRSRV_ERROR_INVALID_DEVICE;
	}

	
	psDevInfo->sSystemBuffer.psSyncData = psSystemBufferSyncData;
	
	eError = MTKLFBUnblankDisplay(psDevInfo);
	if (eError != MTKLFB_OK)
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX
			": %s: Device %u: MTKLFBUnblankDisplay failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, eError));
		return PVRSRV_ERROR_UNBLANK_DISPLAY_FAILED;
	}

	
	*phDevice = (IMG_HANDLE)psDevInfo;
	
	return PVRSRV_OK;
}

static PVRSRV_ERROR CloseDCDevice(IMG_HANDLE hDevice)
{
#if defined(SUPPORT_DRI_DRM)
	MTKLFB_DEVINFO *psDevInfo = (MTKLFB_DEVINFO *)hDevice;

	MTKLFBAtomicBoolSet(&psDevInfo->sLeaveVT, MTKLFB_FALSE);
	(void) MTKLFBUnblankDisplay(psDevInfo);
#else
	UNREFERENCED_PARAMETER(hDevice);
#endif
	return PVRSRV_OK;
}

static PVRSRV_ERROR EnumDCFormats(IMG_HANDLE hDevice,
                                  IMG_UINT32 *pui32NumFormats,
                                  DISPLAY_FORMAT *psFormat)
{
	MTKLFB_DEVINFO	*psDevInfo;
	
	if(!hDevice || !pui32NumFormats)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (MTKLFB_DEVINFO*)hDevice;
	
	*pui32NumFormats = 1;
	
	if(psFormat)
	{
		psFormat[0] = psDevInfo->sDisplayFormat;
	}

	return PVRSRV_OK;
}

static PVRSRV_ERROR EnumDCDims(IMG_HANDLE hDevice, 
                               DISPLAY_FORMAT *psFormat,
                               IMG_UINT32 *pui32NumDims,
                               DISPLAY_DIMS *psDim)
{
	MTKLFB_DEVINFO	*psDevInfo;

	if(!hDevice || !psFormat || !pui32NumDims)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (MTKLFB_DEVINFO*)hDevice;

	*pui32NumDims = 1;

	
	if(psDim)
	{
		psDim[0] = psDevInfo->sDisplayDim;
	}
	
	return PVRSRV_OK;
}


static PVRSRV_ERROR GetDCSystemBuffer(IMG_HANDLE hDevice, IMG_HANDLE *phBuffer)
{
	MTKLFB_DEVINFO	*psDevInfo;
	
	if(!hDevice || !phBuffer)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (MTKLFB_DEVINFO*)hDevice;

	*phBuffer = (IMG_HANDLE)&psDevInfo->sSystemBuffer;

	return PVRSRV_OK;
}


static PVRSRV_ERROR GetDCInfo(IMG_HANDLE hDevice, DISPLAY_INFO *psDCInfo)
{
	MTKLFB_DEVINFO	*psDevInfo;
	
	if(!hDevice || !psDCInfo)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (MTKLFB_DEVINFO*)hDevice;

	*psDCInfo = psDevInfo->sDisplayInfo;

	return PVRSRV_OK;
}

static PVRSRV_ERROR GetDCBufferAddr(IMG_HANDLE        hDevice,
                                    IMG_HANDLE        hBuffer, 
                                    IMG_SYS_PHYADDR   **ppsSysAddr,
                                    IMG_UINT32        *pui32ByteSize,
                                    IMG_VOID          **ppvCpuVAddr,
                                    IMG_HANDLE        *phOSMapInfo,
                                    IMG_BOOL          *pbIsContiguous,
	                                IMG_UINT32		  *pui32TilingStride)
{
	MTKLFB_DEVINFO	*psDevInfo;
	MTKLFB_BUFFER *psSystemBuffer;

	UNREFERENCED_PARAMETER(pui32TilingStride);

	if(!hDevice)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if(!hBuffer)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if (!ppsSysAddr)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if (!pui32ByteSize)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (MTKLFB_DEVINFO*)hDevice;

	psSystemBuffer = (MTKLFB_BUFFER *)hBuffer;

	*ppsSysAddr = &psSystemBuffer->sSysAddr;

	*pui32ByteSize = (IMG_UINT32)psDevInfo->sFBInfo.ulBufferSize;

	if (ppvCpuVAddr)
	{
		*ppvCpuVAddr = psSystemBuffer->sCPUVAddr;
	}

	if (phOSMapInfo)
	{
		*phOSMapInfo = (IMG_HANDLE)0;
	}

	if (pbIsContiguous)
	{
		*pbIsContiguous = IMG_TRUE;
	}

	return PVRSRV_OK;
}

static PVRSRV_ERROR CreateDCSwapChain(IMG_HANDLE hDevice,
                                      IMG_UINT32 ui32Flags,
                                      DISPLAY_SURF_ATTRIBUTES *psDstSurfAttrib,
                                      DISPLAY_SURF_ATTRIBUTES *psSrcSurfAttrib,
                                      IMG_UINT32 ui32BufferCount,
                                      PVRSRV_SYNC_DATA **ppsSyncData,
                                      IMG_UINT32 ui32OEMFlags,
                                      IMG_HANDLE *phSwapChain,
                                      IMG_UINT32 *pui32SwapChainID)
{
	MTKLFB_DEVINFO	*psDevInfo;
	MTKLFB_SWAPCHAIN *psSwapChain;
	MTKLFB_BUFFER *psBuffer;
	IMG_UINT32 i;
	PVRSRV_ERROR eError;
	IMG_UINT32 ui32BuffersToSkip;

	UNREFERENCED_PARAMETER(ui32OEMFlags);
	
	
	if(!hDevice
	|| !psDstSurfAttrib
	|| !psSrcSurfAttrib
	|| !ppsSyncData
	|| !phSwapChain)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (MTKLFB_DEVINFO*)hDevice;
	
	
	if (psDevInfo->sDisplayInfo.ui32MaxSwapChains == 0)
	{
		return PVRSRV_ERROR_NOT_SUPPORTED;
	}

	MTKLFBCreateSwapChainLock(psDevInfo);

	
	if(psDevInfo->psSwapChain != NULL)
	{
		eError = PVRSRV_ERROR_FLIP_CHAIN_EXISTS;
		goto ExitUnLock;
	}
	
	
	if(ui32BufferCount > psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers)
	{
		eError = PVRSRV_ERROR_TOOMANYBUFFERS;
		goto ExitUnLock;
	}
	
	if ((psDevInfo->sFBInfo.ulRoundedBufferSize * (unsigned long)ui32BufferCount) > psDevInfo->sFBInfo.ulFBSize)
	{
		eError = PVRSRV_ERROR_TOOMANYBUFFERS;
		goto ExitUnLock;
	}

	
	ui32BuffersToSkip = psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers - ui32BufferCount;

	
	if(psDstSurfAttrib->pixelformat != psDevInfo->sDisplayFormat.pixelformat
	|| psDstSurfAttrib->sDims.ui32ByteStride != psDevInfo->sDisplayDim.ui32ByteStride
	|| psDstSurfAttrib->sDims.ui32Width != psDevInfo->sDisplayDim.ui32Width
	|| psDstSurfAttrib->sDims.ui32Height != psDevInfo->sDisplayDim.ui32Height)
	{
		
		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto ExitUnLock;
	}		

	if(psDstSurfAttrib->pixelformat != psSrcSurfAttrib->pixelformat
	|| psDstSurfAttrib->sDims.ui32ByteStride != psSrcSurfAttrib->sDims.ui32ByteStride
	|| psDstSurfAttrib->sDims.ui32Width != psSrcSurfAttrib->sDims.ui32Width
	|| psDstSurfAttrib->sDims.ui32Height != psSrcSurfAttrib->sDims.ui32Height)
	{
		
		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto ExitUnLock;
	}		

	
	UNREFERENCED_PARAMETER(ui32Flags);
	
#if defined(PVR_MTKFB3_UPDATE_MODE)
	if (!MTKLFBSetUpdateMode(psDevInfo, PVR_MTKFB3_UPDATE_MODE))
	{
		xlog_printk(ANDROID_LOG_WARN, DRIVER_PREFIX, DRIVER_PREFIX ": %s: Device %u: Couldn't set frame buffer update mode %d\n", __FUNCTION__, psDevInfo->uiFBDevID, PVR_MTKFB3_UPDATE_MODE);
	}
#endif
	
	psSwapChain = (MTKLFB_SWAPCHAIN*)MTKLFBAllocKernelMem(sizeof(MTKLFB_SWAPCHAIN));
	if(!psSwapChain)
	{
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ExitUnLock;
	}

	psBuffer = (MTKLFB_BUFFER*)MTKLFBAllocKernelMem(sizeof(MTKLFB_BUFFER) * ui32BufferCount);
	if(!psBuffer)
	{
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ErrorFreeSwapChain;
	}

	psSwapChain->ulBufferCount = (unsigned long)ui32BufferCount;
	psSwapChain->psBuffer = psBuffer;
	psSwapChain->bNotVSynced = MTKLFB_TRUE;
	psSwapChain->uiFBDevID = psDevInfo->uiFBDevID;

	
	for(i=0; i<ui32BufferCount-1; i++)
	{
		psBuffer[i].psNext = &psBuffer[i+1];
	}
	
	psBuffer[i].psNext = &psBuffer[0];

	for (i = 0; i < ui32BufferCount; i++)
	{
		IMG_UINT32 ui32SwapBuffer = i + ui32BuffersToSkip;
		IMG_UINT32 ui32BufferOffset = ui32SwapBuffer * (IMG_UINT32)psDevInfo->sFBInfo.ulRoundedBufferSize;

		psBuffer[i].psSyncData = ppsSyncData[i];

		psBuffer[i].sSysAddr.uiAddr = psDevInfo->sFBInfo.sSysAddr.uiAddr + ui32BufferOffset;
		psBuffer[i].sCPUVAddr = psDevInfo->sFBInfo.sCPUVAddr + ui32BufferOffset;
		psBuffer[i].ulYOffset = ui32BufferOffset / psDevInfo->sFBInfo.ulByteStride;
		psBuffer[i].psDevInfo = psDevInfo;

		MTKLFBInitBufferForSwap(&psBuffer[i]);
	}

	if (MTKLFBCreateSwapQueue(psSwapChain) != MTKLFB_OK)
	{ 
		xlog_printk(ANDROID_LOG_WARN, DRIVER_PREFIX, DRIVER_PREFIX ": %s: Device %u: Failed to create workqueue\n", __FUNCTION__, psDevInfo->uiFBDevID);
		eError = PVRSRV_ERROR_UNABLE_TO_INSTALL_ISR;
		goto ErrorFreeBuffers;
	}

	if (MTKLFBEnableLFBEventNotification(psDevInfo)!= MTKLFB_OK)
	{
		eError = PVRSRV_ERROR_UNABLE_TO_ENABLE_EVENT;
		xlog_printk(ANDROID_LOG_WARN, DRIVER_PREFIX, DRIVER_PREFIX ": %s: Device %u: Couldn't enable framebuffer event notification\n", __FUNCTION__, psDevInfo->uiFBDevID);
		goto ErrorDestroySwapQueue;
	}

	psDevInfo->uiSwapChainID++;
	if (psDevInfo->uiSwapChainID == 0)
	{
		psDevInfo->uiSwapChainID++;
	}

	psSwapChain->uiSwapChainID = psDevInfo->uiSwapChainID;

	psDevInfo->psSwapChain = psSwapChain;

	*pui32SwapChainID = psDevInfo->uiSwapChainID;

	*phSwapChain = (IMG_HANDLE)psSwapChain;

	eError = PVRSRV_OK;
	goto ExitUnLock;

ErrorDestroySwapQueue:
	MTKLFBDestroySwapQueue(psSwapChain);
ErrorFreeBuffers:
	MTKLFBFreeKernelMem(psBuffer);
ErrorFreeSwapChain:
	MTKLFBFreeKernelMem(psSwapChain);
ExitUnLock:
	MTKLFBCreateSwapChainUnLock(psDevInfo);
	return eError;
}

static PVRSRV_ERROR DestroyDCSwapChain(IMG_HANDLE hDevice,
	IMG_HANDLE hSwapChain)
{
	MTKLFB_DEVINFO	*psDevInfo;
	MTKLFB_SWAPCHAIN *psSwapChain;
	MTKLFB_ERROR eError;

	
	if(!hDevice || !hSwapChain)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}
	
	psDevInfo = (MTKLFB_DEVINFO*)hDevice;
	psSwapChain = (MTKLFB_SWAPCHAIN*)hSwapChain;

	MTKLFBCreateSwapChainLock(psDevInfo);

	if (SwapChainHasChanged(psDevInfo, psSwapChain))
	{
		xlog_printk(ANDROID_LOG_WARN, DRIVER_PREFIX, DRIVER_PREFIX
			": %s: Device %u: Swap chain mismatch\n", __FUNCTION__, psDevInfo->uiFBDevID);

		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto ExitUnLock;
	}

	MTKLFBDestroySwapQueue(psSwapChain);

	eError = MTKLFBDisableLFBEventNotification(psDevInfo);
	if (eError != MTKLFB_OK)
	{
		xlog_printk(ANDROID_LOG_WARN, DRIVER_PREFIX, DRIVER_PREFIX ": %s: Device %u: Couldn't disable framebuffer event notification\n", __FUNCTION__, psDevInfo->uiFBDevID);
	}

	MTKLFBFreeKernelMem(psSwapChain->psBuffer);
	MTKLFBFreeKernelMem(psSwapChain);

	psDevInfo->psSwapChain = NULL;

	MTKLFBFlip(psDevInfo, &psDevInfo->sSystemBuffer);
	(void) MTKLFBCheckModeAndSync(psDevInfo);

	eError = PVRSRV_OK;

ExitUnLock:
	MTKLFBCreateSwapChainUnLock(psDevInfo);

	return eError;
}

static PVRSRV_ERROR SetDCDstRect(IMG_HANDLE hDevice,
	IMG_HANDLE hSwapChain,
	IMG_RECT *psRect)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(psRect);

	
	
	return PVRSRV_ERROR_NOT_SUPPORTED;
}

static PVRSRV_ERROR SetDCSrcRect(IMG_HANDLE hDevice,
                                 IMG_HANDLE hSwapChain,
                                 IMG_RECT *psRect)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(psRect);

	

	return PVRSRV_ERROR_NOT_SUPPORTED;
}

static PVRSRV_ERROR SetDCDstColourKey(IMG_HANDLE hDevice,
                                      IMG_HANDLE hSwapChain,
                                      IMG_UINT32 ui32CKColour)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(ui32CKColour);

	

	return PVRSRV_ERROR_NOT_SUPPORTED;
}

static PVRSRV_ERROR SetDCSrcColourKey(IMG_HANDLE hDevice,
                                      IMG_HANDLE hSwapChain,
                                      IMG_UINT32 ui32CKColour)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(ui32CKColour);

	

	return PVRSRV_ERROR_NOT_SUPPORTED;
}

static PVRSRV_ERROR GetDCBuffers(IMG_HANDLE hDevice,
                                 IMG_HANDLE hSwapChain,
                                 IMG_UINT32 *pui32BufferCount,
                                 IMG_HANDLE *phBuffer)
{
	MTKLFB_DEVINFO   *psDevInfo;
	MTKLFB_SWAPCHAIN *psSwapChain;
	PVRSRV_ERROR eError;
	unsigned i;
	
	
	if(!hDevice 
	|| !hSwapChain
	|| !pui32BufferCount
	|| !phBuffer)
	{
		xlog_printk(ANDROID_LOG_ERROR, DRIVER_PREFIX, 
			"%s: invalid params\n", __FUNCTION__);

		return PVRSRV_ERROR_INVALID_PARAMS;
	}
	
	psDevInfo = (MTKLFB_DEVINFO*)hDevice;
	psSwapChain = (MTKLFB_SWAPCHAIN*)hSwapChain;

	MTKLFBCreateSwapChainLock(psDevInfo);

	if (SwapChainHasChanged(psDevInfo, psSwapChain))
	{
		xlog_printk(ANDROID_LOG_ERROR, DRIVER_PREFIX,
			"%s: Device %u: Swap chain mismatch\n", __FUNCTION__, psDevInfo->uiFBDevID);

		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto Exit;
	}
	
	
	*pui32BufferCount = (IMG_UINT32)psSwapChain->ulBufferCount;
	
	
	for(i=0; i<psSwapChain->ulBufferCount; i++)
	{
		phBuffer[i] = (IMG_HANDLE)&psSwapChain->psBuffer[i];
	}
	
	eError = PVRSRV_OK;

Exit:
	MTKLFBCreateSwapChainUnLock(psDevInfo);

	return eError;
}

static PVRSRV_ERROR SwapToDCBuffer(IMG_HANDLE hDevice,
                                   IMG_HANDLE hBuffer,
                                   IMG_UINT32 ui32SwapInterval,
                                   IMG_HANDLE hPrivateTag,
                                   IMG_UINT32 ui32ClipRectCount,
                                   IMG_RECT *psClipRect)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hBuffer);
	UNREFERENCED_PARAMETER(ui32SwapInterval);
	UNREFERENCED_PARAMETER(hPrivateTag);
	UNREFERENCED_PARAMETER(ui32ClipRectCount);
	UNREFERENCED_PARAMETER(psClipRect);
	
	

	return PVRSRV_OK;
}

static MTKLFB_BOOL WaitForVSyncSettle(MTKLFB_DEVINFO *psDevInfo)
{
		unsigned i;
		for(i = 0; i < MTKLFB_VSYNC_SETTLE_COUNT; i++)
		{
			if (DontWaitForVSync(psDevInfo) || !MTKLFBWaitForVSync(psDevInfo))
			{
				return MTKLFB_FALSE;
			}
		}

		return MTKLFB_TRUE;
}

void MTKLFBSwapHandler(MTKLFB_BUFFER *psBuffer)
{
	MTKLFB_DEVINFO *psDevInfo = psBuffer->psDevInfo;
	MTKLFB_SWAPCHAIN *psSwapChain = psDevInfo->psSwapChain;
	MTKLFB_BOOL bPreviouslyNotVSynced;

#if defined(SUPPORT_DRI_DRM)
	if (!MTKLFBAtomicBoolRead(&psDevInfo->sLeaveVT))
#endif
	{
		MTKLFBFlip(psDevInfo, psBuffer);
	}

	bPreviouslyNotVSynced = psSwapChain->bNotVSynced;
	psSwapChain->bNotVSynced = MTKLFB_TRUE;


	if (!DontWaitForVSync(psDevInfo))
	{
		MTKLFB_UPDATE_MODE eMode = MTKLFBGetUpdateMode(psDevInfo);
		int iBlankEvents = MTKLFBAtomicIntRead(&psDevInfo->sBlankEvents);

		switch(eMode)
		{
			case MTKLFB_UPDATE_MODE_AUTO:
				psSwapChain->bNotVSynced = MTKLFB_FALSE;

				if (bPreviouslyNotVSynced || psSwapChain->iBlankEvents != iBlankEvents)
				{
					psSwapChain->iBlankEvents = iBlankEvents;
					psSwapChain->bNotVSynced = !WaitForVSyncSettle(psDevInfo);
				} else if (psBuffer->ulSwapInterval != 0)
				{
					psSwapChain->bNotVSynced = !MTKLFBWaitForVSync(psDevInfo);
				}
				break;
#if defined(PVR_MTKFB3_MANUAL_UPDATE_SYNC_IN_SWAP)
			case MTKLFB_UPDATE_MODE_MANUAL:
				if (psBuffer->ulSwapInterval != 0)
				{
					(void) MTKLFBManualSync(psDevInfo);
				}
				break;
#endif
			default:
				break;
		}
	}

	psDevInfo->sPVRJTable.pfnPVRSRVCmdComplete((IMG_HANDLE)psBuffer->hCmdComplete, IMG_TRUE);
}

static IMG_BOOL ProcessFlip(IMG_HANDLE  hCmdCookie,
                            IMG_UINT32  ui32DataSize,
                            IMG_VOID   *pvData)
{
	DISPLAYCLASS_FLIP_COMMAND *psFlipCmd;
	MTKLFB_DEVINFO *psDevInfo;
	MTKLFB_BUFFER *psBuffer;
	MTKLFB_SWAPCHAIN *psSwapChain;

	
	if(!hCmdCookie || !pvData)
	{
		xlog_printk(ANDROID_LOG_ERROR, DRIVER_PREFIX,
			"hCmdCookie(%p) or pvData(%p) is NULL\n", hCmdCookie, pvData);

		return IMG_FALSE;
	}

	
	psFlipCmd = (DISPLAYCLASS_FLIP_COMMAND*)pvData;

	if (psFlipCmd == IMG_NULL)
	{
		xlog_printk(ANDROID_LOG_ERROR, DRIVER_PREFIX,
			"psFlipCmd is NULL\n");

		return IMG_FALSE;
	}

	
	psDevInfo = (MTKLFB_DEVINFO*)psFlipCmd->hExtDevice;
	psBuffer = (MTKLFB_BUFFER*)psFlipCmd->hExtBuffer;
	psSwapChain = (MTKLFB_SWAPCHAIN*) psFlipCmd->hExtSwapChain;

	MTKLFBCreateSwapChainLock(psDevInfo);
	if (SwapChainHasChanged(psDevInfo, psSwapChain))
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX
			": %s: Device %u (PVR Device ID %u): The swap chain has been destroyed\n",
			__FUNCTION__, psDevInfo->uiFBDevID, psDevInfo->uiPVRDevID));
	}
	else
	{
		psBuffer->hCmdComplete = (MTKLFB_HANDLE)hCmdCookie;
		psBuffer->ulSwapInterval = (unsigned long)psFlipCmd->ui32SwapInterval;
		MTKLFBQueueBufferForSwap(psSwapChain, psBuffer);
	}
	MTKLFBCreateSwapChainUnLock(psDevInfo);

	return IMG_TRUE;
}

static MTKLFB_ERROR MTKLFBInitFBDev(MTKLFB_DEVINFO *psDevInfo)
{
	struct fb_info *psLINFBInfo;
	struct module *psLINFBOwner;
	MTKLFB_FBINFO *psPVRFBInfo = &psDevInfo->sFBInfo;
	MTKLFB_ERROR eError = MTKLFB_ERROR_GENERIC;
	unsigned long FBSize;
	unsigned long ulLCM;
	unsigned uiFBDevID = psDevInfo->uiFBDevID;

	MTKLFB_CONSOLE_LOCK();

	psLINFBInfo = registered_fb[uiFBDevID];
	if (psLINFBInfo == NULL)
	{
		eError = MTKLFB_ERROR_INVALID_DEVICE;
		goto ErrorRelSem;
	}

#ifdef USE_ARGB8888_FB
    {
        int res;
        struct fb_var_screeninfo info;
        info = psLINFBInfo->var;

        info.activate       = FB_ACTIVATE_NOW;

        info.bits_per_pixel = 32;
        info.transp.offset  = 24;
        info.transp.length  = 8;
        info.red.offset     = 16;
        info.red.length     = 8;
        info.green.offset   = 8;
        info.green.length   = 8;
        info.blue.offset    = 0;
        info.blue.length    = 8;

        res = fb_set_var(psLINFBInfo, &info);

        if (res != 0)
        {
            xlog_printk(ANDROID_LOG_INFO, DRIVER_PREFIX, DRIVER_PREFIX ": %s: Device %u: fb_set_var failed (Error: %d)\n", __FUNCTION__, uiFBDevID, res);
            eError = MTKLFB_ERROR_INIT_FAILURE;
            goto ErrorRelSem;
        }
    }
#endif

	FBSize = (psLINFBInfo->screen_size) != 0 ?
					psLINFBInfo->screen_size :
					psLINFBInfo->fix.smem_len;

	
	if (FBSize == 0 || psLINFBInfo->fix.line_length == 0)
	{
		eError = MTKLFB_ERROR_INVALID_DEVICE;
		goto ErrorRelSem;
	}

	psLINFBOwner = psLINFBInfo->fbops->owner;
	if (!try_module_get(psLINFBOwner))
	{
		xlog_printk(ANDROID_LOG_INFO, DRIVER_PREFIX, DRIVER_PREFIX
			": %s: Device %u: Couldn't get framebuffer module\n", __FUNCTION__, uiFBDevID);

		goto ErrorRelSem;
	}

	if (psLINFBInfo->fbops->fb_open != NULL)
	{
		int res;

		res = psLINFBInfo->fbops->fb_open(NULL, psLINFBInfo, 0);
		if (res != 0)
		{
			xlog_printk(ANDROID_LOG_INFO, DRIVER_PREFIX, DRIVER_PREFIX
				" %s: Device %u: Couldn't open framebuffer(%d)\n", __FUNCTION__, uiFBDevID, res);

			goto ErrorModPut;
		}
	}

	psDevInfo->psLINFBInfo = psLINFBInfo;

	ulLCM = LCM(psLINFBInfo->fix.line_length, MTKLFB_PAGE_SIZE);

	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer physical address: 0x%lx\n",
			psDevInfo->uiFBDevID, psLINFBInfo->fix.smem_start));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer virtual address: 0x%lx\n",
			psDevInfo->uiFBDevID, (unsigned long)psLINFBInfo->screen_base));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer size: %lu\n",
			psDevInfo->uiFBDevID, FBSize));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer virtual width: %u\n",
			psDevInfo->uiFBDevID, psLINFBInfo->var.xres_virtual));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer virtual height: %u\n",
			psDevInfo->uiFBDevID, psLINFBInfo->var.yres_virtual));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer width: %u\n",
			psDevInfo->uiFBDevID, psLINFBInfo->var.xres));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer height: %u\n",
			psDevInfo->uiFBDevID, psLINFBInfo->var.yres));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer stride: %u\n",
			psDevInfo->uiFBDevID, psLINFBInfo->fix.line_length));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: LCM of stride and page size: %lu\n",
			psDevInfo->uiFBDevID, ulLCM));

	
	psPVRFBInfo->sSysAddr.uiAddr = psLINFBInfo->fix.smem_start;
	psPVRFBInfo->sCPUVAddr = psLINFBInfo->screen_base;

	psPVRFBInfo->ulWidth = psLINFBInfo->var.xres;
	psPVRFBInfo->ulHeight = psLINFBInfo->var.yres;
	psPVRFBInfo->ulByteStride =  psLINFBInfo->fix.line_length;
	psPVRFBInfo->ulFBSize = FBSize;
	psPVRFBInfo->ulBufferSize = psPVRFBInfo->ulHeight * psPVRFBInfo->ulByteStride;
	
	psPVRFBInfo->ulRoundedBufferSize = RoundUpToMultiple(psPVRFBInfo->ulBufferSize, ulLCM);

	if(psLINFBInfo->var.bits_per_pixel == 16)
	{
		if((psLINFBInfo->var.red.length == 5) &&
			(psLINFBInfo->var.green.length == 6) && 
			(psLINFBInfo->var.blue.length == 5) && 
			(psLINFBInfo->var.red.offset == 11) &&
			(psLINFBInfo->var.green.offset == 5) && 
			(psLINFBInfo->var.blue.offset == 0) && 
			(psLINFBInfo->var.red.msb_right == 0))
		{
			psPVRFBInfo->ePixelFormat = PVRSRV_PIXEL_FORMAT_RGB565;
		}
		else
		{
			xlog_printk(ANDROID_LOG_INFO, DRIVER_PREFIX, DRIVER_PREFIX ": %s: Device %u: Unknown FB format\n", __FUNCTION__, uiFBDevID);
		}
	}
	else if(psLINFBInfo->var.bits_per_pixel == 32)
	{
		if((psLINFBInfo->var.red.length == 8) &&
			(psLINFBInfo->var.green.length == 8) && 
			(psLINFBInfo->var.blue.length == 8) && 
			(psLINFBInfo->var.red.offset == 16) &&
			(psLINFBInfo->var.green.offset == 8) && 
			(psLINFBInfo->var.blue.offset == 0) && 
			(psLINFBInfo->var.red.msb_right == 0))
		{
			psPVRFBInfo->ePixelFormat = PVRSRV_PIXEL_FORMAT_ARGB8888;
		}
		else
		{
			xlog_printk(ANDROID_LOG_INFO, DRIVER_PREFIX, DRIVER_PREFIX ": %s: Device %u: Unknown FB format\n", __FUNCTION__, uiFBDevID);
		}
	}	
	else
	{
		xlog_printk(ANDROID_LOG_INFO, DRIVER_PREFIX, DRIVER_PREFIX ": %s: Device %u: Unknown FB format\n", __FUNCTION__, uiFBDevID);
	}

	psDevInfo->sFBInfo.ulPhysicalWidthmm =
		((int)psLINFBInfo->var.width  > 0) ? psLINFBInfo->var.width  : 0;

	psDevInfo->sFBInfo.ulPhysicalHeightmm =
		((int)psLINFBInfo->var.height > 0) ? psLINFBInfo->var.height : 0;

	
	psDevInfo->sFBInfo.sSysAddr.uiAddr = psPVRFBInfo->sSysAddr.uiAddr;
	psDevInfo->sFBInfo.sCPUVAddr = psPVRFBInfo->sCPUVAddr;

	eError = MTKLFB_OK;
	goto ErrorRelSem;

ErrorModPut:
	module_put(psLINFBOwner);
ErrorRelSem:
	MTKLFB_CONSOLE_UNLOCK();

	return eError;
}

static void MTKLFBDeInitFBDev(MTKLFB_DEVINFO *psDevInfo)
{
	struct fb_info *psLINFBInfo = psDevInfo->psLINFBInfo;
	struct module *psLINFBOwner;

	MTKLFB_CONSOLE_LOCK();

	psLINFBOwner = psLINFBInfo->fbops->owner;

	if (psLINFBInfo->fbops->fb_release != NULL) 
	{
		(void) psLINFBInfo->fbops->fb_release(NULL, psLINFBInfo, 0);
	}

	module_put(psLINFBOwner);

	MTKLFB_CONSOLE_UNLOCK();
}

static MTKLFB_DEVINFO *MTKLFBInitDev(unsigned uiFBDevID)
{
	PFN_CMD_PROC	 	pfnCmdProcList[MTKLFB_COMMAND_COUNT];
	IMG_UINT32		aui32SyncCountList[MTKLFB_COMMAND_COUNT][2];
	MTKLFB_DEVINFO		*psDevInfo = NULL;

	
	psDevInfo = (MTKLFB_DEVINFO *)MTKLFBAllocKernelMem(sizeof(MTKLFB_DEVINFO));

	if(psDevInfo == NULL)
	{
		xlog_printk(ANDROID_LOG_ERROR, DRIVER_PREFIX, DRIVER_PREFIX
			": %s: Device %u: Couldn't allocate device information structure\n", __FUNCTION__, uiFBDevID);

		goto ErrorExit;
	}

	
	memset(psDevInfo, 0, sizeof(MTKLFB_DEVINFO));

	psDevInfo->uiFBDevID = uiFBDevID;

	
	if(!(*gpfnGetPVRJTable)(&psDevInfo->sPVRJTable))
	{
		goto ErrorFreeDevInfo;
	}

	
	if(MTKLFBInitFBDev(psDevInfo) != MTKLFB_OK)
	{
		
		goto ErrorFreeDevInfo;
	}

	psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers = (IMG_UINT32)(psDevInfo->sFBInfo.ulFBSize / psDevInfo->sFBInfo.ulRoundedBufferSize);
	if (psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers != 0)
	{
		psDevInfo->sDisplayInfo.ui32MaxSwapChains = 1;
		psDevInfo->sDisplayInfo.ui32MaxSwapInterval = 1;
	}

	psDevInfo->sDisplayInfo.ui32PhysicalWidthmm = psDevInfo->sFBInfo.ulPhysicalWidthmm;
	psDevInfo->sDisplayInfo.ui32PhysicalHeightmm = psDevInfo->sFBInfo.ulPhysicalHeightmm;

	strncpy(psDevInfo->sDisplayInfo.szDisplayName, DISPLAY_DEVICE_NAME, MAX_DISPLAY_NAME_SIZE);

	psDevInfo->sDisplayFormat.pixelformat = psDevInfo->sFBInfo.ePixelFormat;
	psDevInfo->sDisplayDim.ui32Width      = (IMG_UINT32)psDevInfo->sFBInfo.ulWidth;
	psDevInfo->sDisplayDim.ui32Height     = (IMG_UINT32)psDevInfo->sFBInfo.ulHeight;
	psDevInfo->sDisplayDim.ui32ByteStride = (IMG_UINT32)psDevInfo->sFBInfo.ulByteStride;

	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
		": Device %u: Maximum number of swap chain buffers: %u\n",
		psDevInfo->uiFBDevID, psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers));

	
	psDevInfo->sSystemBuffer.sSysAddr = psDevInfo->sFBInfo.sSysAddr;
	psDevInfo->sSystemBuffer.sCPUVAddr = psDevInfo->sFBInfo.sCPUVAddr;
	psDevInfo->sSystemBuffer.psDevInfo = psDevInfo;

	MTKLFBInitBufferForSwap(&psDevInfo->sSystemBuffer);

	

	psDevInfo->sDCJTable.ui32TableSize = sizeof(PVRSRV_DC_SRV2DISP_KMJTABLE);
	psDevInfo->sDCJTable.pfnOpenDCDevice = OpenDCDevice;
	psDevInfo->sDCJTable.pfnCloseDCDevice = CloseDCDevice;
	psDevInfo->sDCJTable.pfnEnumDCFormats = EnumDCFormats;
	psDevInfo->sDCJTable.pfnEnumDCDims = EnumDCDims;
	psDevInfo->sDCJTable.pfnGetDCSystemBuffer = GetDCSystemBuffer;
	psDevInfo->sDCJTable.pfnGetDCInfo = GetDCInfo;
	psDevInfo->sDCJTable.pfnGetBufferAddr = GetDCBufferAddr;
	psDevInfo->sDCJTable.pfnCreateDCSwapChain = CreateDCSwapChain;
	psDevInfo->sDCJTable.pfnDestroyDCSwapChain = DestroyDCSwapChain;
	psDevInfo->sDCJTable.pfnSetDCDstRect = SetDCDstRect;
	psDevInfo->sDCJTable.pfnSetDCSrcRect = SetDCSrcRect;
	psDevInfo->sDCJTable.pfnSetDCDstColourKey = SetDCDstColourKey;
	psDevInfo->sDCJTable.pfnSetDCSrcColourKey = SetDCSrcColourKey;
	psDevInfo->sDCJTable.pfnGetDCBuffers = GetDCBuffers;
	psDevInfo->sDCJTable.pfnSwapToDCBuffer = SwapToDCBuffer;
	psDevInfo->sDCJTable.pfnSetDCState = SetDCState;

	
	if(psDevInfo->sPVRJTable.pfnPVRSRVRegisterDCDevice(
		&psDevInfo->sDCJTable,
		&psDevInfo->uiPVRDevID) != PVRSRV_OK)
	{
		xlog_printk(ANDROID_LOG_ERROR, DRIVER_PREFIX, DRIVER_PREFIX
			": %s: Device %u: PVR Services device registration failed\n", __FUNCTION__, uiFBDevID);

		goto ErrorDeInitFBDev;
	}
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
		": Device %u: PVR Device ID: %u\n",
		psDevInfo->uiFBDevID, psDevInfo->uiPVRDevID));
	
	
	pfnCmdProcList[DC_FLIP_COMMAND] = ProcessFlip;

	
	aui32SyncCountList[DC_FLIP_COMMAND][0] = 0; 
	aui32SyncCountList[DC_FLIP_COMMAND][1] = 10; 

	



	if (psDevInfo->sPVRJTable.pfnPVRSRVRegisterCmdProcList(psDevInfo->uiPVRDevID,
															&pfnCmdProcList[0],
															aui32SyncCountList,
															MTKLFB_COMMAND_COUNT) != PVRSRV_OK)
	{
		xlog_printk(ANDROID_LOG_ERROR, DRIVER_PREFIX, DRIVER_PREFIX
			": %s: Device %u: Couldn't register command processing functions with PVR Services\n", __FUNCTION__, uiFBDevID);
		goto ErrorUnregisterDevice;
	}

	MTKLFBCreateSwapChainLockInit(psDevInfo);

	MTKLFBAtomicBoolInit(&psDevInfo->sBlanked, MTKLFB_FALSE);
	MTKLFBAtomicIntInit(&psDevInfo->sBlankEvents, 0);
	MTKLFBAtomicBoolInit(&psDevInfo->sFlushCommands, MTKLFB_FALSE);
#if defined(CONFIG_HAS_EARLYSUSPEND)
	MTKLFBAtomicBoolInit(&psDevInfo->sEarlySuspendFlag, MTKLFB_FALSE);
#endif
#if defined(SUPPORT_DRI_DRM)
	MTKLFBAtomicBoolInit(&psDevInfo->sLeaveVT, MTKLFB_FALSE);
#endif
	return psDevInfo;

ErrorUnregisterDevice:
	(void)psDevInfo->sPVRJTable.pfnPVRSRVRemoveDCDevice(psDevInfo->uiPVRDevID);
ErrorDeInitFBDev:
	MTKLFBDeInitFBDev(psDevInfo);
ErrorFreeDevInfo:
	MTKLFBFreeKernelMem(psDevInfo);
ErrorExit:
	return NULL;
}

MTKLFB_ERROR MTKLFBInit(void)
{
	unsigned uiMaxFBDevIDPlusOne = MTKLFBMaxFBDevIDPlusOne();
	unsigned i;
	unsigned uiDevicesFound = 0;

	if(MTKLFBGetLibFuncAddr ("PVRGetDisplayClassJTable", &gpfnGetPVRJTable) != MTKLFB_OK)
	{
		return MTKLFB_ERROR_INIT_FAILURE;
	}

	
	for(i = uiMaxFBDevIDPlusOne; i-- != 0;)
	{
		MTKLFB_DEVINFO *psDevInfo = MTKLFBInitDev(i);

		if (psDevInfo != NULL)
		{
			
			MTKLFBSetDevInfoPtr(psDevInfo->uiFBDevID, psDevInfo);
			uiDevicesFound++;
		}
	}

	return (uiDevicesFound != 0) ? MTKLFB_OK : MTKLFB_ERROR_INIT_FAILURE;
}

static MTKLFB_BOOL MTKLFBDeInitDev(MTKLFB_DEVINFO *psDevInfo)
{
	PVRSRV_DC_DISP2SRV_KMJTABLE *psPVRJTable = &psDevInfo->sPVRJTable;

	MTKLFBCreateSwapChainLockDeInit(psDevInfo);

	MTKLFBAtomicBoolDeInit(&psDevInfo->sBlanked);
	MTKLFBAtomicIntDeInit(&psDevInfo->sBlankEvents);
	MTKLFBAtomicBoolDeInit(&psDevInfo->sFlushCommands);
#if defined(CONFIG_HAS_EARLYSUSPEND)
	MTKLFBAtomicBoolDeInit(&psDevInfo->sEarlySuspendFlag);
#endif
#if defined(SUPPORT_DRI_DRM)
	MTKLFBAtomicBoolDeInit(&psDevInfo->sLeaveVT);
#endif
	psPVRJTable = &psDevInfo->sPVRJTable;

	if (psPVRJTable->pfnPVRSRVRemoveCmdProcList (psDevInfo->uiPVRDevID, MTKLFB_COMMAND_COUNT) != PVRSRV_OK)
	{
		xlog_printk(ANDROID_LOG_ERROR, DRIVER_PREFIX, DRIVER_PREFIX
			": %s: Device %u: PVR Device %u: Couldn't unregister command processing functions\n", __FUNCTION__, psDevInfo->uiFBDevID, psDevInfo->uiPVRDevID);
		return MTKLFB_FALSE;
	}

	
	if (psPVRJTable->pfnPVRSRVRemoveDCDevice(psDevInfo->uiPVRDevID) != PVRSRV_OK)
	{
		xlog_printk(ANDROID_LOG_ERROR, DRIVER_PREFIX, DRIVER_PREFIX
			": %s: Device %u: PVR Device %u: Couldn't remove device from PVR Services\n", __FUNCTION__, psDevInfo->uiFBDevID, psDevInfo->uiPVRDevID);
		return MTKLFB_FALSE;
	}
	
	MTKLFBDeInitFBDev(psDevInfo);

	MTKLFBSetDevInfoPtr(psDevInfo->uiFBDevID, NULL);

	
	MTKLFBFreeKernelMem(psDevInfo);

	return MTKLFB_TRUE;
}

MTKLFB_ERROR MTKLFBDeInit(void)
{
	unsigned uiMaxFBDevIDPlusOne = MTKLFBMaxFBDevIDPlusOne();
	unsigned i;
	MTKLFB_BOOL bError = MTKLFB_FALSE;

	for(i = 0; i < uiMaxFBDevIDPlusOne; i++)
	{
		MTKLFB_DEVINFO *psDevInfo = MTKLFBGetDevInfoPtr(i);

		if (psDevInfo != NULL)
		{
			bError |= !MTKLFBDeInitDev(psDevInfo);
		}
	}

	return (bError) ? MTKLFB_ERROR_INIT_FAILURE : MTKLFB_OK;
}

