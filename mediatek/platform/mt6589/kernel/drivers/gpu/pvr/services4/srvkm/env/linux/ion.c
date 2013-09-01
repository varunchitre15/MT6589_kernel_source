/*************************************************************************/ /*!
@Title          Ion driver inter-operability code.
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/

#include "ion.h"

#include "services.h"
#include "servicesint.h"
#include "mutex.h"
#include "lock.h"
#include "mm.h"
#include "handle.h"
#include "perproc.h"
#include "env_perproc.h"
#include "private_data.h"
#include "pvr_debug.h"

#include <linux/module.h>
#include <linux/file.h>
#include <linux/fs.h>

#if defined (CONFIG_ION_OMAP)
extern struct ion_client *gpsIONClient;

void PVRSRVExportFDToIONHandles(int fd, struct ion_client **client,
								struct ion_handle *handles[2])
{
	PVRSRV_FILE_PRIVATE_DATA *psPrivateData;
	PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo;
	LinuxMemArea *psLinuxMemArea;
	PVRSRV_ERROR eError;
	struct file *psFile;

	/* Take the bridge mutex so the handle won't be freed underneath us */
	LinuxLockMutex(&gPVRSRVLock);

	psFile = fget(fd);
	if(!psFile)
		goto err_unlock;

	psPrivateData = psFile->private_data;
	if(!psPrivateData)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: struct file* has no private_data; "
								"invalid export handle", __func__));
		goto err_fput;
	}

	eError = PVRSRVLookupHandle(KERNEL_HANDLE_BASE,
								(IMG_PVOID *)&psKernelMemInfo,
								psPrivateData->hKernelMemInfo,
								PVRSRV_HANDLE_TYPE_MEM_INFO);
	if(eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to look up MEM_INFO handle",
								__func__));
		goto err_fput;
	}

	psLinuxMemArea = (LinuxMemArea *)psKernelMemInfo->sMemBlk.hOSMemHandle;
	BUG_ON(psLinuxMemArea == IMG_NULL);

	if(psLinuxMemArea->eAreaType != LINUX_MEM_AREA_ION)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Valid handle, but not an ION buffer",
								__func__));
		goto err_fput;
	}

	handles[0] = psLinuxMemArea->uData.sIONTilerAlloc.psIONHandle[0];
	handles[1] = psLinuxMemArea->uData.sIONTilerAlloc.psIONHandle[1];
	if(client)
		*client = gpsIONClient;

err_fput:
	fput(psFile);
err_unlock:
	/* Allow PVRSRV clients to communicate with srvkm again */
	LinuxUnLockMutex(&gPVRSRVLock);
}

struct ion_handle *
PVRSRVExportFDToIONHandle(int fd, struct ion_client **client)
{
	struct ion_handle *psHandles[2] = { IMG_NULL, IMG_NULL };
	PVRSRVExportFDToIONHandles(fd, client, psHandles);
	return psHandles[0];
}

EXPORT_SYMBOL(PVRSRVExportFDToIONHandles);
EXPORT_SYMBOL(PVRSRVExportFDToIONHandle);
#endif

#if defined (CONFIG_ION_MTK)
#include <linux/ion_drv.h>

extern struct ion_client *gpsIONClient;

void PVRSRVExportFDToIONHandles(int fd, struct ion_client **client,
								struct ion_handle *handles[2])
{
#if 0
	PVRSRV_FILE_PRIVATE_DATA *psPrivateData;
	PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo;
	LinuxMemArea *psLinuxMemArea;
	PVRSRV_ERROR eError;
	struct file *psFile;

	/* Take the bridge mutex so the handle won't be freed underneath us */
	LinuxLockMutex(&gPVRSRVLock);

	psFile = fget(fd);
	if(!psFile)
		goto err_unlock;

	psPrivateData = psFile->private_data;
	if(!psPrivateData)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: struct file* has no private_data; "
								"invalid export handle", __func__));
		goto err_fput;
	}

	eError = PVRSRVLookupHandle(KERNEL_HANDLE_BASE,
								(IMG_PVOID *)&psKernelMemInfo,
								psPrivateData->hKernelMemInfo,
								PVRSRV_HANDLE_TYPE_MEM_INFO);
	if(eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to look up MEM_INFO handle",
								__func__));
		goto err_fput;
	}

	psLinuxMemArea = (LinuxMemArea *)psKernelMemInfo->sMemBlk.hOSMemHandle;
	BUG_ON(psLinuxMemArea == IMG_NULL);

	if(psLinuxMemArea->eAreaType != LINUX_MEM_AREA_ION)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Valid handle, but not an ION buffer",
								__func__));
		goto err_fput;
	}

	handles[0] = psLinuxMemArea->uData.sIONTilerAlloc.psIONHandle[0];
	handles[1] = psLinuxMemArea->uData.sIONTilerAlloc.psIONHandle[1];
	if(client)
		*client = gpsIONClient;

err_fput:
	fput(psFile);
err_unlock:
	/* Allow PVRSRV clients to communicate with srvkm again */
	LinuxUnLockMutex(&gPVRSRVLock);
#endif 
}

struct ion_handle *
PVRSRVExportFDToIONHandle(int fd, struct ion_client **client)
{
#if 0
	struct ion_handle *psHandles[2] = { IMG_NULL, IMG_NULL };
	PVRSRVExportFDToIONHandles(fd, client, psHandles);
	return psHandles[0];
#else
    return NULL;
#endif    
}

int PVRSRVGetIONFDKM(PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo)
{

    LinuxMemArea *psLinuxMemArea;
    struct ion_handle *handles[2];
    int share_fd = 0;
    
    //LinuxLockMutex(&gPVRSRVLock);

    psLinuxMemArea = (LinuxMemArea *)psKernelMemInfo->sMemBlk.hOSMemHandle;
	BUG_ON(psLinuxMemArea == IMG_NULL);
    
    if(psLinuxMemArea->eAreaType != LINUX_MEM_AREA_ION)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Valid handle, but not an ION buffer",
								__func__));
		goto err_unlock;
	}

    handles[0] = psLinuxMemArea->uData.sIONTilerAlloc.psIONHandle[0];
    //[shally review, only set handles[0]]
	//handles[1] = psLinuxMemArea->uData.sIONTilerAlloc.psIONHandle[1];

    share_fd = ion_share_dma_buf(gpsIONClient, handles[0]);
    if (share_fd == 0)
    {
        PVR_DPF((PVR_DBG_ERROR, "%s: share_fd fail:0x%x",__func__,share_fd));
		goto err_unlock;
    }
    
    
err_unlock:
	/* Allow PVRSRV clients to communicate with srvkm again */
//	LinuxUnLockMutex(&gPVRSRVLock);

    return share_fd;    
}

EXPORT_SYMBOL(PVRSRVExportFDToIONHandles);
EXPORT_SYMBOL(PVRSRVExportFDToIONHandle);
EXPORT_SYMBOL(PVRSRVGetIONFDKM);

#endif


#if defined (SUPPORT_ION)
#include "syscommon.h"
#include "env_data.h"
#include "../drivers/gpu/ion/ion_priv.h"
#include "linux/kernel.h"

struct ion_heap **apsIonHeaps;
struct ion_device *psIonDev;

#if 0 //#ifndef MTK_GPU_SUPPORT_ION  

static struct ion_platform_data generic_config = {
	.nr = 2,
	.heaps = {
				{
					.type = ION_HEAP_TYPE_SYSTEM_CONTIG,
					.name = "System contig",
					.id = ION_HEAP_TYPE_SYSTEM_CONTIG,
				},
				{
					.type = ION_HEAP_TYPE_SYSTEM,
					.name = "System",
					.id = ION_HEAP_TYPE_SYSTEM,
				}
			}
};
#endif 
PVRSRV_ERROR IonInit(IMG_VOID)
{

#if 1 //#if defined (MTK_GPU_SUPPORT_ION)

     //[shallytest] no need to init ion_heap, ion_drv will do this 
    psIonDev = g_ion_device; //import from ion_drv driver

	return PVRSRV_OK;    
    
#else
	int uiHeapCount = generic_config.nr;
	int uiError;
	int i;

	apsIonHeaps = kzalloc(sizeof(struct ion_heap *) * uiHeapCount, GFP_KERNEL);
	/* Create the ion devicenode */
	psIonDev = ion_device_create(NULL);
	if (IS_ERR_OR_NULL(psIonDev)) {
		kfree(apsIonHeaps);
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	/* Register all the heaps */
	for (i = 0; i < generic_config.nr; i++)
	{
		struct ion_platform_heap *psPlatHeapData = &generic_config.heaps[i];

		apsIonHeaps[i] = ion_heap_create(psPlatHeapData);
		if (IS_ERR_OR_NULL(apsIonHeaps[i]))
		{
			uiError = PTR_ERR(apsIonHeaps[i]);
			goto failHeapCreate;
		}
		ion_device_add_heap(psIonDev, apsIonHeaps[i]);
	}

	return PVRSRV_OK;
failHeapCreate:
	for (i = 0; i < uiHeapCount; i++) {
		if (apsIonHeaps[i])
		{
				ion_heap_destroy(apsIonHeaps[i]);
		}
	}
	kfree(apsIonHeaps);
	return PVRSRV_ERROR_OUT_OF_MEMORY;
#endif    
}

IMG_VOID IonDeinit(IMG_VOID)
{
#if 0//#ifndef MTK_GPU_SUPPORT_ION
	int uiHeapCount = generic_config.nr;
	int i;

	for (i = 0; i < uiHeapCount; i++) {
		if (apsIonHeaps[i])
		{
				ion_heap_destroy(apsIonHeaps[i]);
		}
	}
	kfree(apsIonHeaps);
	ion_device_destroy(psIonDev);
#else
//#error "get here sgdg"
    //[shallytest]no need to init ion_heap, ion_drv will do this 
#endif 
}

typedef struct _ION_IMPORT_DATA_
{
	struct ion_client *psIonClient;
	struct ion_handle *psIonHandle;
	IMG_PVOID pvKernAddr;
} ION_IMPORT_DATA;

PVRSRV_ERROR IonImportBufferAndAquirePhysAddr(IMG_HANDLE hIonDev,
											  IMG_HANDLE hIonFD,
											  IMG_UINT32 *pui32PageCount,
											  IMG_SYS_PHYADDR **ppasSysPhysAddr,
											  IMG_PVOID *ppvKernAddr,
											  IMG_HANDLE *phPriv)
{
#if 0//#ifndef MTK_GPU_SUPPORT_ION
	struct ion_client *psIonClient = hIonDev;
	struct ion_handle *psIonHandle;
	struct scatterlist *psScatterList;
	struct scatterlist *psTemp;
	IMG_SYS_PHYADDR *pasSysPhysAddr = NULL;
	ION_IMPORT_DATA *psImportData;
	PVRSRV_ERROR eError;
	IMG_UINT32 ui32PageCount = 0;
	IMG_UINT32 i;
	IMG_PVOID pvKernAddr;
	int fd = (int) hIonFD;

	psImportData = kmalloc(sizeof(ION_IMPORT_DATA), GFP_KERNEL);
	if (psImportData == NULL)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	/* Get the buffer handle */
	psIonHandle = ion_import_fd(psIonClient, fd);
	if (psIonHandle == IMG_NULL)
	{
		eError = PVRSRV_ERROR_BAD_MAPPING;
		goto exitFailImport;
	}

	/* Create data for free callback */
	psImportData->psIonClient = psIonClient;
	psImportData->psIonHandle = psIonHandle;	

	psScatterList = ion_map_dma(psIonClient, psIonHandle);
	if (psScatterList == NULL)
	{
		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto exitFailMap;
	}

	/*
		We do a two pass process, 1st workout how many pages there
		are, 2nd fill in the data.
	*/
	for (i=0;i<2;i++)
	{
		psTemp = psScatterList;
		if (i == 1)
		{
			pasSysPhysAddr = kmalloc(sizeof(IMG_SYS_PHYADDR) * ui32PageCount, GFP_KERNEL);
			if (pasSysPhysAddr == NULL)
			{
				eError = PVRSRV_ERROR_OUT_OF_MEMORY;
				goto exitFailAlloc;
			}
			ui32PageCount = 0;	/* Reset the page count a we use if for the index */
		}

		while(psTemp)
		{
			IMG_UINT32 j;

			for (j=0;j<psTemp->length;j+=PAGE_SIZE)
			{
				if (i == 1)
				{
					/* Pass 2: Get the page data */
					pasSysPhysAddr[ui32PageCount].uiAddr = sg_phys(psTemp);
				}
				ui32PageCount++;
			}
			psTemp = sg_next(psTemp);
		}
	}

	pvKernAddr = ion_map_kernel(psIonClient, psIonHandle);
	if (IS_ERR(pvKernAddr))
	{
		pvKernAddr = IMG_NULL;
	}

	psImportData->pvKernAddr = pvKernAddr;

	*ppvKernAddr = pvKernAddr;
	*pui32PageCount = ui32PageCount;
	*ppasSysPhysAddr = pasSysPhysAddr;
	*phPriv = psImportData;
	return PVRSRV_OK;

exitFailAlloc:
	ion_unmap_dma(psIonClient, psIonHandle);
exitFailMap:
	ion_free(psIonClient, psIonHandle);
exitFailImport:
	kfree(psImportData);
	return eError;
#else  
//[shallytest]mark first , and add later.
    return PVRSRV_OK;
#endif 

}


IMG_VOID IonUnimportBufferAndReleasePhysAddr(IMG_HANDLE hPriv)
{
#if 0//#ifndef MTK_GPU_SUPPORT_ION
	ION_IMPORT_DATA *psImportData = hPriv;

	ion_unmap_dma(psImportData->psIonClient, psImportData->psIonHandle);
	if (psImportData->pvKernAddr)
	{
		ion_unmap_kernel(psImportData->psIonClient, psImportData->psIonHandle);
	}
	ion_free(psImportData->psIonClient, psImportData->psIonHandle);
	kfree(psImportData);
#else
//[shallytest]mark first , and add later.

#endif
}
#endif
