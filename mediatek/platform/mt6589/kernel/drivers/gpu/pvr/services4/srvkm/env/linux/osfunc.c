/*************************************************************************/ /*!
@Title          Environment related functions
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

#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38))
#ifndef AUTOCONF_INCLUDED
#include <linux/config.h>
#endif
#endif

#include <asm/io.h>
#include <asm/page.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22))
#include <asm/system.h>
#endif
#include <asm/cacheflush.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/hugetlb.h> 
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/pci.h>

#include <linux/string.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <asm/hardirq.h>
#include <linux/timer.h>
#include <linux/capability.h>
#include <asm/uaccess.h>
#include <linux/spinlock.h>
#if defined(PVR_LINUX_MISR_USING_WORKQUEUE) || \
	defined(PVR_LINUX_MISR_USING_PRIVATE_WORKQUEUE) || \
	defined(PVR_LINUX_TIMERS_USING_WORKQUEUES) || \
	defined(PVR_LINUX_TIMERS_USING_SHARED_WORKQUEUE) || \
	defined(PVR_LINUX_USING_WORKQUEUES)
#include <linux/workqueue.h>
#endif

#include <mach/sync_write.h>


#include "img_types.h"
#include "services_headers.h"
#include "mm.h"
#include "pvrmmap.h"
#include "mmap.h"
#include "env_data.h"
#include "proc.h"
#include "mutex.h"
#include "event.h"
#include "linkage.h"
#include "pvr_uaccess.h"
#include "lock.h"

#if defined (SUPPORT_ION)
#include "ion.h"
#endif

#if defined (CONFIG_X86_PAE)
#error Physical Address Extension not supported with the driver
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
#define ON_EACH_CPU(func, info, wait) on_each_cpu(func, info, wait)
#else
#define ON_EACH_CPU(func, info, wait) on_each_cpu(func, info, 0, wait)
#endif

#if defined(PVR_LINUX_USING_WORKQUEUES) && !defined(CONFIG_PREEMPT)
/* 
 * Services spins at certain points waiting for events (e.g. swap
 * chain destrucion).  If those events rely on workqueues running,
 * it needs to be possible to preempt the waiting thread.
 * Removing the need for CONFIG_PREEMPT will require adding preemption
 * points at various points in Services.
 */
#error "A preemptible Linux kernel is required when using workqueues"
#endif

#if defined(EMULATOR)
#define EVENT_OBJECT_TIMEOUT_MS		(2000)
#else
#define EVENT_OBJECT_TIMEOUT_MS		(100)
#endif /* EMULATOR */

#if !defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
PVRSRV_ERROR OSAllocMem_Impl(IMG_UINT32 ui32Flags, IMG_UINT32 ui32Size, IMG_PVOID *ppvCpuVAddr, IMG_HANDLE *phBlockAlloc)
#else
PVRSRV_ERROR OSAllocMem_Impl(IMG_UINT32 ui32Flags, IMG_UINT32 ui32Size, IMG_PVOID *ppvCpuVAddr, IMG_HANDLE *phBlockAlloc, IMG_CHAR *pszFilename, IMG_UINT32 ui32Line)
#endif
{
    PVR_UNREFERENCED_PARAMETER(ui32Flags);
    PVR_UNREFERENCED_PARAMETER(phBlockAlloc);

    if (ui32Size > PAGE_SIZE)
    {
        /* Try to allocate the memory using vmalloc */
#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
        *ppvCpuVAddr = _VMallocWrapper(ui32Size, PVRSRV_HAP_CACHED, pszFilename, ui32Line);
#else
        *ppvCpuVAddr = VMallocWrapper(ui32Size, PVRSRV_HAP_CACHED);
#endif
        if (*ppvCpuVAddr)
        {
            return PVRSRV_OK;
        }
    }

#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
    *ppvCpuVAddr = _KMallocWrapper(ui32Size, GFP_KERNEL | __GFP_NOWARN, pszFilename, ui32Line);
#else
    *ppvCpuVAddr = KMallocWrapper(ui32Size, GFP_KERNEL | __GFP_NOWARN);
#endif
    if (!*ppvCpuVAddr)
    {
        return PVRSRV_ERROR_OUT_OF_MEMORY;
    }

    return PVRSRV_OK;
}

#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,24))

static inline int is_vmalloc_addr(const void *pvCpuVAddr)
{
	unsigned long lAddr = (unsigned long)pvCpuVAddr;
	return lAddr >= VMALLOC_START && lAddr < VMALLOC_END;
}

#endif /* (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,24)) */

#if !defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
PVRSRV_ERROR OSFreeMem_Impl(IMG_UINT32 ui32Flags, IMG_UINT32 ui32Size, IMG_PVOID pvCpuVAddr, IMG_HANDLE hBlockAlloc)
#else
PVRSRV_ERROR OSFreeMem_Impl(IMG_UINT32 ui32Flags, IMG_UINT32 ui32Size, IMG_PVOID pvCpuVAddr, IMG_HANDLE hBlockAlloc, IMG_CHAR *pszFilename, IMG_UINT32 ui32Line)
#endif
{	
    PVR_UNREFERENCED_PARAMETER(ui32Flags);
    PVR_UNREFERENCED_PARAMETER(ui32Size);
    PVR_UNREFERENCED_PARAMETER(hBlockAlloc);

    if (is_vmalloc_addr(pvCpuVAddr))
    {
#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
        _VFreeWrapper(pvCpuVAddr, pszFilename, ui32Line);
#else
        VFreeWrapper(pvCpuVAddr);
#endif
    }
    else
    {
#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
        _KFreeWrapper(pvCpuVAddr, pszFilename, ui32Line);
#else
        KFreeWrapper(pvCpuVAddr);
#endif
    }

    return PVRSRV_OK;
}


PVRSRV_ERROR
OSAllocPages_Impl(IMG_UINT32 ui32AllocFlags,
				  IMG_UINT32 ui32Size,
				  IMG_UINT32 ui32PageSize,
				  IMG_PVOID pvPrivData,
				  IMG_UINT32 ui32PrivDataLength,
				  IMG_HANDLE hBMHandle,
				  IMG_VOID **ppvCpuVAddr,
				  IMG_HANDLE *phOSMemHandle)
{
    LinuxMemArea *psLinuxMemArea;

    PVR_UNREFERENCED_PARAMETER(ui32PageSize);

#if 0
    /* For debug: force all OSAllocPages allocations to have a kernel
     * virtual address */
    if(ui32AllocFlags & PVRSRV_HAP_SINGLE_PROCESS)
    {
        ui32AllocFlags &= ~PVRSRV_HAP_SINGLE_PROCESS;
        ui32AllocFlags |= PVRSRV_HAP_MULTI_PROCESS;
    }
#endif

    if(ui32AllocFlags & PVRSRV_MEM_ION)
    {
        /* We'll only see HAP_SINGLE_PROCESS with MEM_ION */
        BUG_ON((ui32AllocFlags & PVRSRV_HAP_MAPTYPE_MASK) != PVRSRV_HAP_SINGLE_PROCESS);

        psLinuxMemArea = NewIONLinuxMemArea(ui32Size, ui32AllocFlags,
											pvPrivData, ui32PrivDataLength);
        if(!psLinuxMemArea)
        {
            return PVRSRV_ERROR_OUT_OF_MEMORY;
        }

        PVRMMapRegisterArea(psLinuxMemArea);
        goto ExitSkipSwitch;
    }

    switch(ui32AllocFlags & PVRSRV_HAP_MAPTYPE_MASK)
    {
        case PVRSRV_HAP_KERNEL_ONLY:
        {
            psLinuxMemArea = NewVMallocLinuxMemArea(ui32Size, ui32AllocFlags);
            if(!psLinuxMemArea)
            {
                return PVRSRV_ERROR_OUT_OF_MEMORY;
            }
            break;
        }
        case PVRSRV_HAP_SINGLE_PROCESS:
        {
            /* Currently PVRSRV_HAP_SINGLE_PROCESS implies that we dont need a
             * kernel virtual mapping, but will need a user space virtual mapping */

            psLinuxMemArea = NewAllocPagesLinuxMemArea(ui32Size, ui32AllocFlags);
            if(!psLinuxMemArea)
            {
                return PVRSRV_ERROR_OUT_OF_MEMORY;
            }
            PVRMMapRegisterArea(psLinuxMemArea);
            break;
        }

        case PVRSRV_HAP_MULTI_PROCESS:
        {
            /* Currently PVRSRV_HAP_MULTI_PROCESS implies that we need a kernel
             * virtual mapping and potentially multiple user space virtual
             * mappings: Note: these eat into our limited kernel virtual
             * address space. */

#if defined(VIVT_CACHE) || defined(__sh__)
            /* ARM9 caches are tagged with virtual pages, not physical. As we are going to
             * share this memory in different address spaces, we don't want it to be cached.
             * ARM11 has physical tagging, so we can cache this memory without fear of virtual
             * address aliasing in the TLB, as long as the kernel supports cache colouring for
             * VIPT architectures. */
            ui32AllocFlags &= ~PVRSRV_HAP_CACHED;
#endif
            psLinuxMemArea = NewVMallocLinuxMemArea(ui32Size, ui32AllocFlags);
            if(!psLinuxMemArea)
            {
                return PVRSRV_ERROR_OUT_OF_MEMORY;
            }
            PVRMMapRegisterArea(psLinuxMemArea);
            break;
        }
        default:
            PVR_DPF((PVR_DBG_ERROR, "OSAllocPages: invalid flags 0x%x\n", ui32AllocFlags));
            *ppvCpuVAddr = NULL;
            *phOSMemHandle = (IMG_HANDLE)0;
            return PVRSRV_ERROR_INVALID_PARAMS;
    }

	/*
		In case of sparse mapping we need to handle back to the BM as it
		knows the mapping info
	*/
	if (ui32AllocFlags & PVRSRV_MEM_SPARSE)
	{
		psLinuxMemArea->hBMHandle = hBMHandle;
	}

ExitSkipSwitch:
    *ppvCpuVAddr = LinuxMemAreaToCpuVAddr(psLinuxMemArea);
    *phOSMemHandle = psLinuxMemArea;
    
    LinuxMemAreaRegister(psLinuxMemArea);

    return PVRSRV_OK;
}


PVRSRV_ERROR
OSFreePages(IMG_UINT32 ui32AllocFlags, IMG_UINT32 ui32Bytes, IMG_VOID *pvCpuVAddr, IMG_HANDLE hOSMemHandle)
{   
    LinuxMemArea *psLinuxMemArea;
    PVRSRV_ERROR eError;
    
    PVR_UNREFERENCED_PARAMETER(ui32Bytes);
    PVR_UNREFERENCED_PARAMETER(pvCpuVAddr);
    
    psLinuxMemArea = (LinuxMemArea *)hOSMemHandle;

    switch(ui32AllocFlags & PVRSRV_HAP_MAPTYPE_MASK)
    {
        case PVRSRV_HAP_KERNEL_ONLY:
            break;
        case PVRSRV_HAP_SINGLE_PROCESS:
        case PVRSRV_HAP_MULTI_PROCESS:
            eError = PVRMMapRemoveRegisteredArea(psLinuxMemArea);
            if (eError != PVRSRV_OK)
            {
                PVR_DPF((PVR_DBG_ERROR,
                         "OSFreePages(ui32AllocFlags=0x%08X, ui32Bytes=%d, "
                                        "pvCpuVAddr=%p, hOSMemHandle=%p) FAILED!",
                         ui32AllocFlags, ui32Bytes, pvCpuVAddr, hOSMemHandle));
                return eError;
            }
            break;
        default:
            PVR_DPF((PVR_DBG_ERROR,"%s: invalid flags 0x%x\n",
                    __FUNCTION__, ui32AllocFlags));
            return PVRSRV_ERROR_INVALID_PARAMS;
    }

    LinuxMemAreaDeepFree(psLinuxMemArea);

    return PVRSRV_OK;
}


PVRSRV_ERROR
OSGetSubMemHandle(IMG_HANDLE hOSMemHandle,
                  IMG_UINT32 ui32ByteOffset,
                  IMG_UINT32 ui32Bytes,
                  IMG_UINT32 ui32Flags,
                  IMG_HANDLE *phOSMemHandleRet)
{
    LinuxMemArea *psParentLinuxMemArea, *psLinuxMemArea;
    PVRSRV_ERROR eError;

    psParentLinuxMemArea = (LinuxMemArea *)hOSMemHandle;
    
    psLinuxMemArea = NewSubLinuxMemArea(psParentLinuxMemArea, ui32ByteOffset, ui32Bytes);
    if(!psLinuxMemArea)
    {
        *phOSMemHandleRet = NULL;
        return PVRSRV_ERROR_OUT_OF_MEMORY;
    }
    *phOSMemHandleRet = psLinuxMemArea;

    /* KERNEL_ONLY areas are never mmapable. */
    if(ui32Flags & PVRSRV_HAP_KERNEL_ONLY)
    {
        return PVRSRV_OK;
    }

    eError = PVRMMapRegisterArea(psLinuxMemArea);
    if(eError != PVRSRV_OK)
    {
        goto failed_register_area;
    }

    return PVRSRV_OK;

failed_register_area:
    *phOSMemHandleRet = NULL;
    LinuxMemAreaDeepFree(psLinuxMemArea);
    return eError;
}

PVRSRV_ERROR
OSReleaseSubMemHandle(IMG_VOID *hOSMemHandle, IMG_UINT32 ui32Flags)
{
    LinuxMemArea *psLinuxMemArea;
    PVRSRV_ERROR eError;
    
    psLinuxMemArea = (LinuxMemArea *)hOSMemHandle;
    PVR_ASSERT(psLinuxMemArea->eAreaType == LINUX_MEM_AREA_SUB_ALLOC);
    
    if((ui32Flags & PVRSRV_HAP_KERNEL_ONLY) == 0)
    {
        eError = PVRMMapRemoveRegisteredArea(psLinuxMemArea);
        if(eError != PVRSRV_OK)
        {
            return eError;
        }
    }
    LinuxMemAreaDeepFree(psLinuxMemArea);

    return PVRSRV_OK;
}


IMG_CPU_PHYADDR
OSMemHandleToCpuPAddr(IMG_VOID *hOSMemHandle, IMG_UINT32 ui32ByteOffset)
{
    PVR_ASSERT(hOSMemHandle);

    return LinuxMemAreaToCpuPAddr(hOSMemHandle, ui32ByteOffset);
}


IMG_BOOL OSMemHandleIsPhysContig(IMG_VOID *hOSMemHandle)
{
	LinuxMemArea *psLinuxMemArea = (LinuxMemArea *)hOSMemHandle;

	PVR_ASSERT(psLinuxMemArea);

	if(psLinuxMemArea->eAreaType == LINUX_MEM_AREA_EXTERNAL_KV)
		return psLinuxMemArea->uData.sExternalKV.bPhysContig;

	return IMG_FALSE;
}


/*!
******************************************************************************

 @Function		OSMemCopy

 @Description	Copies memory around

 @Input    pvDst - pointer to dst
 @Output   pvSrc - pointer to src
 @Input    ui32Size - bytes to copy

 @Return  none

******************************************************************************/
IMG_VOID OSMemCopy(IMG_VOID *pvDst, IMG_VOID *pvSrc, IMG_UINT32 ui32Size)
{
#if defined(USE_UNOPTIMISED_MEMCPY)
    IMG_UINT8 *Src,*Dst;
    IMG_INT i;

    Src=(IMG_UINT8 *)pvSrc;
    Dst=(IMG_UINT8 *)pvDst;
    for(i=0;i<ui32Size;i++)
    {
        Dst[i]=Src[i];
    }
#else
    memcpy(pvDst, pvSrc, ui32Size);
#endif
}


/*!
******************************************************************************

 @Function	OSMemSet

 @Description Function that does the same as the C memset() functions

 @Modified *pvDest :	pointer to start of buffer to be set

 @Input    ui8Value:	value to set each byte to

 @Input    ui32Size :	number of bytes to set

 @Return   IMG_VOID

******************************************************************************/
IMG_VOID OSMemSet(IMG_VOID *pvDest, IMG_UINT8 ui8Value, IMG_UINT32 ui32Size)
{
#if defined(USE_UNOPTIMISED_MEMSET)
    IMG_UINT8 *Buff;
    IMG_INT i;

    Buff=(IMG_UINT8 *)pvDest;
    for(i=0;i<ui32Size;i++)
    {
        Buff[i]=ui8Value;
    }
#else
    memset(pvDest, (IMG_INT) ui8Value, (size_t) ui32Size);
#endif
}


/*!
******************************************************************************
 @Function	OSStringCopy
 @Description strcpy
******************************************************************************/
IMG_CHAR *OSStringCopy(IMG_CHAR *pszDest, const IMG_CHAR *pszSrc)
{
    return (strcpy(pszDest, pszSrc));
}

/*!
******************************************************************************
 @Function	OSSNPrintf
 @Description snprintf
******************************************************************************/
IMG_INT32 OSSNPrintf(IMG_CHAR *pStr, IMG_UINT32 ui32Size, const IMG_CHAR *pszFormat, ...)
{
    va_list argList;
    IMG_INT32 iCount;

    va_start(argList, pszFormat);
    iCount = vsnprintf(pStr, (size_t)ui32Size, pszFormat, argList);
    va_end(argList);

    return iCount;
}

/*!
******************************************************************************

 @Function OSBreakResourceLock

 @Description unlocks an OS dependant resource

 @Input phResource - pointer to OS dependent resource structure
 @Input ui32ID - Lock value to look for

 @Return

******************************************************************************/
IMG_VOID OSBreakResourceLock (PVRSRV_RESOURCE *psResource, IMG_UINT32 ui32ID)
{
    volatile IMG_UINT32 *pui32Access = (volatile IMG_UINT32 *)&psResource->ui32Lock;

    if(*pui32Access)
    {
        if(psResource->ui32ID == ui32ID)
        {
            psResource->ui32ID = 0;
            *pui32Access = 0;
        }
        else
        {
            PVR_DPF((PVR_DBG_MESSAGE,"OSBreakResourceLock: Resource is not locked for this process.")); 
        }
    }
    else
    {
        PVR_DPF((PVR_DBG_MESSAGE,"OSBreakResourceLock: Resource is not locked"));
    }
}


/*!
******************************************************************************

 @Function OSCreateResource

 @Description creates a OS dependant resource object

 @Input phResource - pointer to OS dependent resource

 @Return error status

******************************************************************************/
PVRSRV_ERROR OSCreateResource(PVRSRV_RESOURCE *psResource)
{
    psResource->ui32ID = 0;
    psResource->ui32Lock = 0;

    return PVRSRV_OK;
}


/*!
******************************************************************************

 @Function OSDestroyResource

 @Description destroys an OS dependant resource object

 @Input phResource - pointer to OS dependent resource

 @Return error status

******************************************************************************/
PVRSRV_ERROR OSDestroyResource (PVRSRV_RESOURCE *psResource)
{
    OSBreakResourceLock (psResource, psResource->ui32ID);

    return PVRSRV_OK;
}


/*!
******************************************************************************

  @Function		OSInitEnvData

  @Description	Allocates space for env specific data

  @Input		ppvEnvSpecificData - pointer to pointer in which to return
                allocated data.
  @Input		ui32MMUMode - MMU mode.

  @Return		nothing

******************************************************************************/
PVRSRV_ERROR OSInitEnvData(IMG_PVOID *ppvEnvSpecificData)
{
    ENV_DATA		*psEnvData;
    PVRSRV_ERROR	eError;
    
    /* allocate env specific data */
    eError = OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(ENV_DATA), (IMG_VOID **)&psEnvData, IMG_NULL,
        "Environment Data");
    if (eError != PVRSRV_OK)
    {
        return eError;
    }

    eError = OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP, PVRSRV_MAX_BRIDGE_IN_SIZE + PVRSRV_MAX_BRIDGE_OUT_SIZE, 
                    &psEnvData->pvBridgeData, IMG_NULL,
                    "Bridge Data");
    if (eError != PVRSRV_OK)
    {
        OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(ENV_DATA), psEnvData, IMG_NULL);
		/*not nulling pointer, out of scope*/
        return eError;
    }


    /* ISR installation flags */
    psEnvData->bMISRInstalled = IMG_FALSE;
    psEnvData->bLISRInstalled = IMG_FALSE;

    /* copy structure back */
    *ppvEnvSpecificData = psEnvData;

    return PVRSRV_OK;
}


/*!
******************************************************************************

 @Function		OSDeInitEnvData

 @Description	frees env specific data memory

 @Input    pvEnvSpecificData - pointer to private structure

 @Return   PVRSRV_OK on success else PVRSRV_ERROR_OUT_OF_MEMORY

******************************************************************************/
PVRSRV_ERROR OSDeInitEnvData(IMG_PVOID pvEnvSpecificData)
{
    ENV_DATA		*psEnvData = (ENV_DATA*)pvEnvSpecificData;

    PVR_ASSERT(!psEnvData->bMISRInstalled);
    PVR_ASSERT(!psEnvData->bLISRInstalled);

    OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, PVRSRV_MAX_BRIDGE_IN_SIZE + PVRSRV_MAX_BRIDGE_OUT_SIZE, psEnvData->pvBridgeData, IMG_NULL);
    psEnvData->pvBridgeData = IMG_NULL;

    OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(ENV_DATA), pvEnvSpecificData, IMG_NULL);
	/*not nulling pointer, copy on stack*/

    return PVRSRV_OK;
}


/*!
******************************************************************************

 @Function OSReleaseThreadQuanta
 
 @Description
    Releases thread quanta
     
 @Return nothing

******************************************************************************/ 
IMG_VOID OSReleaseThreadQuanta(IMG_VOID)
{
    schedule();
}


/*!
******************************************************************************

 @Function OSClockus
 
 @Description 
    This function returns the clock in microseconds
 
 @Input void

 @Return - clock (us)

******************************************************************************/ 
IMG_UINT32 OSClockus(IMG_VOID)
{
    IMG_UINT32 time, j = jiffies;

    time = j * (1000000 / HZ);

    return time;
}


IMG_VOID OSWaitus(IMG_UINT32 ui32Timeus)
{
    udelay(ui32Timeus);
}


IMG_VOID OSSleepms(IMG_UINT32 ui32Timems)
{
    msleep(ui32Timems);
}


/*!
******************************************************************************

 @Function OSFuncHighResTimerCreate
 
 @Description 
    This function creates a high res timer who's handle is returned
 
 @Input nothing

 @Return handle

******************************************************************************/ 
IMG_HANDLE OSFuncHighResTimerCreate(IMG_VOID)
{
	/* We don't need a handle, but we must return non-NULL */
	return (IMG_HANDLE) 1;
}

/*!
******************************************************************************

 @Function OSFuncHighResTimerGetus
 
 @Description 
    This function returns the current timestamp in us
 
 @Input nothing

 @Return handle

******************************************************************************/ 
IMG_UINT32 OSFuncHighResTimerGetus(IMG_HANDLE hTimer)
{
	return (IMG_UINT32) jiffies_to_usecs(jiffies);
}

/*!
******************************************************************************

 @Function OSFuncHighResTimerDestroy
 
 @Description 
    This function will destroy the high res timer
 
 @Input nothing

 @Return handle

******************************************************************************/ 
IMG_VOID OSFuncHighResTimerDestroy(IMG_HANDLE hTimer)
{
	PVR_UNREFERENCED_PARAMETER(hTimer);
}

/*!
******************************************************************************

  @Function            OSGetCurrentProcessIDKM

  @Description Returns handle for current process

  @Return    ID of current process

*****************************************************************************/
IMG_UINT32 OSGetCurrentProcessIDKM(IMG_VOID)
{
    if (in_interrupt())
    {
        return KERNEL_ID;
    }

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
    return (IMG_UINT32)current->pgrp;
#else
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24))
    return (IMG_UINT32)task_tgid_nr(current);
#else
    return (IMG_UINT32)current->tgid;
#endif
#endif
}


/*!
******************************************************************************

 @Function		OSGetPageSize
 
 @Description	gets page size
    
 @Return   		page size

******************************************************************************/
IMG_UINT32 OSGetPageSize(IMG_VOID)
{
#if defined(__sh__)
    IMG_UINT32 ui32ReturnValue = PAGE_SIZE;

    return (ui32ReturnValue);
#else
    return PAGE_SIZE;
#endif
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0))
/*!
******************************************************************************

 @Function		DeviceISRWrapper
 
 @Description	wrapper for Device ISR function to conform to ISR OS interface
    
 @Return

******************************************************************************/
static irqreturn_t DeviceISRWrapper(int irq, void *dev_id
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19))
        , struct pt_regs *regs
#endif
        )
{
    PVRSRV_DEVICE_NODE *psDeviceNode;
    IMG_BOOL bStatus = IMG_FALSE;

    PVR_UNREFERENCED_PARAMETER(irq);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19))
    PVR_UNREFERENCED_PARAMETER(regs);
#endif	
    psDeviceNode = (PVRSRV_DEVICE_NODE*)dev_id;
    if(!psDeviceNode)
    {
        PVR_DPF((PVR_DBG_ERROR, "DeviceISRWrapper: invalid params\n"));
        goto out;
    }

    bStatus = PVRSRVDeviceLISR(psDeviceNode);

    if (bStatus)
    {
		OSScheduleMISR((IMG_VOID *)psDeviceNode->psSysData);
    }

out:
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
    return bStatus ? IRQ_HANDLED : IRQ_NONE;
#endif
}



/*!
******************************************************************************

 @Function		SystemISRWrapper
 
 @Description	wrapper for System ISR function to conform to ISR OS interface

 @Input    Interrupt - NT interrupt object.
 @Input    Context - Context parameter 
    
 @Return

******************************************************************************/
static irqreturn_t SystemISRWrapper(int irq, void *dev_id
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19))
        , struct pt_regs *regs
#endif
        )
{
    SYS_DATA *psSysData;
    IMG_BOOL bStatus = IMG_FALSE;

    PVR_UNREFERENCED_PARAMETER(irq);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19))
    PVR_UNREFERENCED_PARAMETER(regs);
#endif
    psSysData = (SYS_DATA *)dev_id;
    if(!psSysData)
    {
        PVR_DPF((PVR_DBG_ERROR, "SystemISRWrapper: invalid params\n"));
        goto out;
    }

    bStatus = PVRSRVSystemLISR(psSysData);

    if (bStatus)
    {
        OSScheduleMISR((IMG_VOID *)psSysData);
    }

out:
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
    return bStatus ? IRQ_HANDLED : IRQ_NONE;
#endif
}
/*!
******************************************************************************

 @Function		OSInstallDeviceLISR
 
 @Description	Installs a Device ISR
    
 @Input    pvSysData
 @Input    ui32Irq - IRQ number
 @Input    pszISRName - ISR name
 @Input    pvDeviceNode - device node contains ISR function and data argument

 @Return   error status 

******************************************************************************/
PVRSRV_ERROR OSInstallDeviceLISR(IMG_VOID *pvSysData,
                                    IMG_UINT32 ui32Irq,
                                    IMG_CHAR *pszISRName,
                                    IMG_VOID *pvDeviceNode)
{
    SYS_DATA *psSysData = (SYS_DATA*)pvSysData;
    ENV_DATA *psEnvData = (ENV_DATA *)psSysData->pvEnvSpecificData;

    if (psEnvData->bLISRInstalled)
    {
        PVR_DPF((PVR_DBG_ERROR, "OSInstallDeviceLISR: An ISR has already been installed: IRQ %d cookie %p", psEnvData->ui32IRQ, psEnvData->pvISRCookie));
        return PVRSRV_ERROR_ISR_ALREADY_INSTALLED;
    }

    PVR_TRACE(("Installing device LISR %s on IRQ %d with cookie %p", pszISRName, ui32Irq, pvDeviceNode));

    if(request_irq(ui32Irq, DeviceISRWrapper,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22))
        SA_SHIRQ
#else
        IRQF_TRIGGER_LOW
#endif
        , pszISRName, pvDeviceNode))
    {
        PVR_DPF((PVR_DBG_ERROR,"OSInstallDeviceLISR: Couldn't install device LISR on IRQ %d", ui32Irq));

        return PVRSRV_ERROR_UNABLE_TO_INSTALL_ISR;
    }

    psEnvData->ui32IRQ = ui32Irq;
    psEnvData->pvISRCookie = pvDeviceNode;
    psEnvData->bLISRInstalled = IMG_TRUE;

    return PVRSRV_OK;	
}

/*!
******************************************************************************

 @Function		OSUninstallDeviceLISR
 
 @Description	Uninstalls a Device ISR
    
 @Input    pvSysData - sysdata

 @Return   error status 

******************************************************************************/
PVRSRV_ERROR OSUninstallDeviceLISR(IMG_VOID *pvSysData)
{
    SYS_DATA *psSysData = (SYS_DATA*)pvSysData;
    ENV_DATA *psEnvData = (ENV_DATA *)psSysData->pvEnvSpecificData;

    if (!psEnvData->bLISRInstalled)
    {
        PVR_DPF((PVR_DBG_ERROR, "OSUninstallDeviceLISR: No LISR has been installed"));
        return PVRSRV_ERROR_ISR_NOT_INSTALLED;
    }
        
    PVR_TRACE(("Uninstalling device LISR on IRQ %d with cookie %p", psEnvData->ui32IRQ,  psEnvData->pvISRCookie));

    free_irq(psEnvData->ui32IRQ, psEnvData->pvISRCookie);

    psEnvData->bLISRInstalled = IMG_FALSE;

    return PVRSRV_OK;
}


/*!
******************************************************************************

 @Function		OSInstallSystemLISR
 
 @Description	Installs a System ISR
    
 @Input    psSysData
 @Input    ui32Irq - IRQ number

 @Return   error status 

******************************************************************************/
PVRSRV_ERROR OSInstallSystemLISR(IMG_VOID *pvSysData, IMG_UINT32 ui32Irq)
{
    SYS_DATA *psSysData = (SYS_DATA*)pvSysData;
    ENV_DATA *psEnvData = (ENV_DATA *)psSysData->pvEnvSpecificData;

    if (psEnvData->bLISRInstalled)
    {
        PVR_DPF((PVR_DBG_ERROR, "OSInstallSystemLISR: An LISR has already been installed: IRQ %d cookie %p", psEnvData->ui32IRQ, psEnvData->pvISRCookie));
        return PVRSRV_ERROR_ISR_ALREADY_INSTALLED;
    }

    PVR_TRACE(("Installing system LISR on IRQ %d with cookie %p", ui32Irq, pvSysData));

    if(request_irq(ui32Irq, SystemISRWrapper,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22))
        SA_SHIRQ
#else
        IRQF_SHARED
#endif
        , PVRSRV_MODNAME, pvSysData))
    {
        PVR_DPF((PVR_DBG_ERROR,"OSInstallSystemLISR: Couldn't install system LISR on IRQ %d", ui32Irq));

        return PVRSRV_ERROR_UNABLE_TO_INSTALL_ISR;
    }

    psEnvData->ui32IRQ = ui32Irq;
    psEnvData->pvISRCookie = pvSysData;
    psEnvData->bLISRInstalled = IMG_TRUE;

    return PVRSRV_OK;	
}


/*!
******************************************************************************

 @Function		OSUninstallSystemLISR
 
 @Description	Uninstalls a System ISR
    
 @Input    psSysData

 @Return   error status 

******************************************************************************/
PVRSRV_ERROR OSUninstallSystemLISR(IMG_VOID *pvSysData)
{
    SYS_DATA *psSysData = (SYS_DATA*)pvSysData;
    ENV_DATA *psEnvData = (ENV_DATA *)psSysData->pvEnvSpecificData;

    if (!psEnvData->bLISRInstalled)
    {
        PVR_DPF((PVR_DBG_ERROR, "OSUninstallSystemLISR: No LISR has been installed"));
        return PVRSRV_ERROR_ISR_NOT_INSTALLED;
    }

    PVR_TRACE(("Uninstalling system LISR on IRQ %d with cookie %p", psEnvData->ui32IRQ, psEnvData->pvISRCookie));

    free_irq(psEnvData->ui32IRQ, psEnvData->pvISRCookie);

    psEnvData->bLISRInstalled = IMG_FALSE;

    return PVRSRV_OK;
}

#if defined(PVR_LINUX_MISR_USING_PRIVATE_WORKQUEUE)
/*!
******************************************************************************

 @Function		MISRWrapper
 
 @Description	OS dependent MISR wrapper
	
 @Input    psSysData

 @Return   error status 

******************************************************************************/
static void MISRWrapper(
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
			void *data
#else
			struct work_struct *data
#endif
)
{
	ENV_DATA *psEnvData = container_of(data, ENV_DATA, sMISRWork);
	SYS_DATA *psSysData  = (SYS_DATA *)psEnvData->pvMISRData;

	PVRSRVMISR(psSysData);
}


/*!
******************************************************************************

 @Function		OSInstallMISR
 
 @Description	Installs an OS dependent MISR

 @Input    psSysData
	
 @Return   error status 

******************************************************************************/
PVRSRV_ERROR OSInstallMISR(IMG_VOID *pvSysData)
{
	SYS_DATA *psSysData = (SYS_DATA*)pvSysData;
	ENV_DATA *psEnvData = (ENV_DATA *)psSysData->pvEnvSpecificData;

	if (psEnvData->bMISRInstalled)
	{
		PVR_DPF((PVR_DBG_ERROR, "OSInstallMISR: An MISR has already been installed"));
		return PVRSRV_ERROR_ISR_ALREADY_INSTALLED;
	}

	PVR_TRACE(("Installing MISR with cookie %p", pvSysData));

	psEnvData->psWorkQueue = create_singlethread_workqueue("pvr_workqueue");

	if (psEnvData->psWorkQueue == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "OSInstallMISR: create_singlethreaded_workqueue failed"));
		return PVRSRV_ERROR_UNABLE_TO_CREATE_THREAD;
	}

	INIT_WORK(&psEnvData->sMISRWork, MISRWrapper
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
		, (void *)&psEnvData->sMISRWork
#endif
				);

	psEnvData->pvMISRData = pvSysData;
	psEnvData->bMISRInstalled = IMG_TRUE;

	return PVRSRV_OK;
}


/*!
******************************************************************************

 @Function		OSUninstallMISR
 
 @Description	Uninstalls an OS dependent MISR

 @Input    psSysData
	
 @Return   error status 

******************************************************************************/
PVRSRV_ERROR OSUninstallMISR(IMG_VOID *pvSysData)
{
	SYS_DATA *psSysData = (SYS_DATA*)pvSysData;
	ENV_DATA *psEnvData = (ENV_DATA *)psSysData->pvEnvSpecificData;

	if (!psEnvData->bMISRInstalled)
	{
		PVR_DPF((PVR_DBG_ERROR, "OSUninstallMISR: No MISR has been installed"));
		return PVRSRV_ERROR_ISR_NOT_INSTALLED;
	}

	PVR_TRACE(("Uninstalling MISR"));

	destroy_workqueue(psEnvData->psWorkQueue);

	psEnvData->bMISRInstalled = IMG_FALSE;

	return PVRSRV_OK;
}


/*!
******************************************************************************

 @Function		OSScheduleMISR
 
 @Description	Schedules an OS dependent MISR

 @Input    pvSysData
	
 @Return   error status 

******************************************************************************/
PVRSRV_ERROR OSScheduleMISR(IMG_VOID *pvSysData)
{
	SYS_DATA *psSysData = (SYS_DATA*)pvSysData;
	ENV_DATA *psEnvData = (ENV_DATA*)psSysData->pvEnvSpecificData;

	if (psEnvData->bMISRInstalled)
	{
		queue_work(psEnvData->psWorkQueue, &psEnvData->sMISRWork);
	}

	return PVRSRV_OK;	
}
#else	/* defined(PVR_LINUX_MISR_USING_PRIVATE_WORKQUEUE) */
#if defined(PVR_LINUX_MISR_USING_WORKQUEUE)
/*!
******************************************************************************

 @Function		MISRWrapper
 
 @Description	OS dependent MISR wrapper
	
 @Input    psSysData

 @Return   error status 

******************************************************************************/
static void MISRWrapper(
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
			void *data
#else
			struct work_struct *data
#endif
)
{
	ENV_DATA *psEnvData = container_of(data, ENV_DATA, sMISRWork);
	SYS_DATA *psSysData  = (SYS_DATA *)psEnvData->pvMISRData;

	PVRSRVMISR(psSysData);
}


/*!
******************************************************************************

 @Function		OSInstallMISR
 
 @Description	Installs an OS dependent MISR

 @Input    psSysData
	
 @Return   error status 

******************************************************************************/
PVRSRV_ERROR OSInstallMISR(IMG_VOID *pvSysData)
{
	SYS_DATA *psSysData = (SYS_DATA*)pvSysData;
	ENV_DATA *psEnvData = (ENV_DATA *)psSysData->pvEnvSpecificData;

	if (psEnvData->bMISRInstalled)
	{
		PVR_DPF((PVR_DBG_ERROR, "OSInstallMISR: An MISR has already been installed"));
		return PVRSRV_ERROR_ISR_ALREADY_INSTALLED;
	}

	PVR_TRACE(("Installing MISR with cookie %p", pvSysData));

	INIT_WORK(&psEnvData->sMISRWork, MISRWrapper
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
		, (void *)&psEnvData->sMISRWork
#endif
				);

	psEnvData->pvMISRData = pvSysData;
	psEnvData->bMISRInstalled = IMG_TRUE;

	return PVRSRV_OK;
}


/*!
******************************************************************************

 @Function		OSUninstallMISR
 
 @Description	Uninstalls an OS dependent MISR

 @Input    psSysData
	
 @Return   error status 

******************************************************************************/
PVRSRV_ERROR OSUninstallMISR(IMG_VOID *pvSysData)
{
	SYS_DATA *psSysData = (SYS_DATA*)pvSysData;
	ENV_DATA *psEnvData = (ENV_DATA *)psSysData->pvEnvSpecificData;

	if (!psEnvData->bMISRInstalled)
	{
		PVR_DPF((PVR_DBG_ERROR, "OSUninstallMISR: No MISR has been installed"));
		return PVRSRV_ERROR_ISR_NOT_INSTALLED;
	}

	PVR_TRACE(("Uninstalling MISR"));

	flush_scheduled_work();

	psEnvData->bMISRInstalled = IMG_FALSE;

	return PVRSRV_OK;
}


/*!
******************************************************************************

 @Function		OSScheduleMISR
 
 @Description	Schedules an OS dependent MISR

 @Input    pvSysData
	
 @Return   error status 

******************************************************************************/
PVRSRV_ERROR OSScheduleMISR(IMG_VOID *pvSysData)
{
	SYS_DATA *psSysData = (SYS_DATA*)pvSysData;
	ENV_DATA *psEnvData = (ENV_DATA*)psSysData->pvEnvSpecificData;

	if (psEnvData->bMISRInstalled)
	{
		schedule_work(&psEnvData->sMISRWork);
	}

	return PVRSRV_OK;	
}

#else	/* #if defined(PVR_LINUX_MISR_USING_WORKQUEUE) */


/*!
******************************************************************************

 @Function		MISRWrapper
 
 @Description	OS dependent MISR wrapper
    
 @Input    psSysData

 @Return   error status 

******************************************************************************/
static void MISRWrapper(unsigned long data)
{
    SYS_DATA *psSysData;

    psSysData = (SYS_DATA *)data;
    
    PVRSRVMISR(psSysData);
}


/*!
******************************************************************************

 @Function		OSInstallMISR
 
 @Description	Installs an OS dependent MISR

 @Input    psSysData
    
 @Return   error status 

******************************************************************************/
PVRSRV_ERROR OSInstallMISR(IMG_VOID *pvSysData)
{
    SYS_DATA *psSysData = (SYS_DATA*)pvSysData;
    ENV_DATA *psEnvData = (ENV_DATA *)psSysData->pvEnvSpecificData;

    if (psEnvData->bMISRInstalled)
    {
        PVR_DPF((PVR_DBG_ERROR, "OSInstallMISR: An MISR has already been installed"));
        return PVRSRV_ERROR_ISR_ALREADY_INSTALLED;
    }

    PVR_TRACE(("Installing MISR with cookie %p", pvSysData));

    tasklet_init(&psEnvData->sMISRTasklet, MISRWrapper, (unsigned long)pvSysData);

    psEnvData->bMISRInstalled = IMG_TRUE;

    return PVRSRV_OK;
}


/*!
******************************************************************************

 @Function		OSUninstallMISR
 
 @Description	Uninstalls an OS dependent MISR

 @Input    psSysData
    
 @Return   error status 

******************************************************************************/
PVRSRV_ERROR OSUninstallMISR(IMG_VOID *pvSysData)
{
    SYS_DATA *psSysData = (SYS_DATA*)pvSysData;
    ENV_DATA *psEnvData = (ENV_DATA *)psSysData->pvEnvSpecificData;

    if (!psEnvData->bMISRInstalled)
    {
        PVR_DPF((PVR_DBG_ERROR, "OSUninstallMISR: No MISR has been installed"));
        return PVRSRV_ERROR_ISR_NOT_INSTALLED;
    }

    PVR_TRACE(("Uninstalling MISR"));

    tasklet_kill(&psEnvData->sMISRTasklet);

    psEnvData->bMISRInstalled = IMG_FALSE;

    return PVRSRV_OK;
}

/*!
******************************************************************************

 @Function		OSScheduleMISR
 
 @Description	Schedules an OS dependent MISR

 @Input    pvSysData
    
 @Return   error status 

******************************************************************************/
PVRSRV_ERROR OSScheduleMISR(IMG_VOID *pvSysData)
{
    SYS_DATA *psSysData = (SYS_DATA*)pvSysData;
    ENV_DATA *psEnvData = (ENV_DATA*)psSysData->pvEnvSpecificData;

    if (psEnvData->bMISRInstalled)
    {
        tasklet_schedule(&psEnvData->sMISRTasklet);
    }

    return PVRSRV_OK;	
}

#endif /* #if defined(PVR_LINUX_MISR_USING_WORKQUEUE) */
#endif /* #if defined(PVR_LINUX_MISR_USING_PRIVATE_WORKQUEUE) */

#endif /* #if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)) */

IMG_VOID OSPanic(IMG_VOID)
{
	BUG();
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22))
#define	OS_TAS(p)	xchg((p), 1)
#else
#define	OS_TAS(p)	tas(p)
#endif
/*!
******************************************************************************

 @Function OSLockResource

 @Description locks an OS dependant Resource

 @Input phResource - pointer to OS dependent Resource
 @Input bBlock - do we want to block?

 @Return error status

******************************************************************************/
PVRSRV_ERROR OSLockResource ( PVRSRV_RESOURCE 	*psResource,
                                IMG_UINT32 			ui32ID)

{
    PVRSRV_ERROR eError = PVRSRV_OK;

    if(!OS_TAS(&psResource->ui32Lock))
        psResource->ui32ID = ui32ID;
    else
        eError = PVRSRV_ERROR_UNABLE_TO_LOCK_RESOURCE;

    return eError;
}


/*!
******************************************************************************

 @Function OSUnlockResource

 @Description unlocks an OS dependant resource

 @Input phResource - pointer to OS dependent resource structure

 @Return

******************************************************************************/
PVRSRV_ERROR OSUnlockResource (PVRSRV_RESOURCE *psResource, IMG_UINT32 ui32ID)
{
    volatile IMG_UINT32 *pui32Access = (volatile IMG_UINT32 *)&psResource->ui32Lock;
    PVRSRV_ERROR eError = PVRSRV_OK;

    if(*pui32Access)
    {
        if(psResource->ui32ID == ui32ID)
        {
            psResource->ui32ID = 0;
            smp_mb();
            *pui32Access = 0;
        }
        else
        {
            PVR_DPF((PVR_DBG_ERROR,"OSUnlockResource: Resource %p is not locked with expected value.", psResource)); 
            PVR_DPF((PVR_DBG_MESSAGE,"Should be %x is actually %x", ui32ID, psResource->ui32ID));
            eError = PVRSRV_ERROR_INVALID_LOCK_ID;
        }
    }
    else
    {
        PVR_DPF((PVR_DBG_ERROR,"OSUnlockResource: Resource %p is not locked", psResource));
        eError = PVRSRV_ERROR_RESOURCE_NOT_LOCKED;
    }
    
    return eError;
}


/*!
******************************************************************************

 @Function OSIsResourceLocked

 @Description tests if resource is locked

 @Input phResource - pointer to OS dependent resource structure

 @Return error status

******************************************************************************/
IMG_BOOL OSIsResourceLocked (PVRSRV_RESOURCE *psResource, IMG_UINT32 ui32ID)
{
    volatile IMG_UINT32 *pui32Access = (volatile IMG_UINT32 *)&psResource->ui32Lock;

    return 	(*(volatile IMG_UINT32 *)pui32Access == 1) && (psResource->ui32ID == ui32ID)
            ?	IMG_TRUE
            :	IMG_FALSE;
}


#if !defined(SYS_CUSTOM_POWERLOCK_WRAP)
PVRSRV_ERROR OSPowerLockWrap(IMG_BOOL bTryLock)
{
	PVR_UNREFERENCED_PARAMETER(bTryLock);	

	return PVRSRV_OK;
}

IMG_VOID OSPowerLockUnwrap (IMG_VOID)
{
}
#endif /* SYS_CUSTOM_POWERLOCK_WRAP */


IMG_CPU_PHYADDR OSMapLinToCPUPhys(IMG_HANDLE hOSMemHandle,
								  IMG_VOID *pvLinAddr)
{
    IMG_CPU_PHYADDR CpuPAddr;
    LinuxMemArea *psLinuxMemArea;
	IMG_UINTPTR_T uiByteOffset;
	IMG_UINT32 ui32ByteOffset;

	PVR_ASSERT(hOSMemHandle != IMG_NULL);

	psLinuxMemArea = (LinuxMemArea *)hOSMemHandle;

	uiByteOffset = (IMG_UINTPTR_T)pvLinAddr - (IMG_UINTPTR_T)LinuxMemAreaToCpuVAddr(psLinuxMemArea);
	ui32ByteOffset = (IMG_UINT32)uiByteOffset;

	CpuPAddr = LinuxMemAreaToCpuPAddr(hOSMemHandle, ui32ByteOffset);

	return CpuPAddr;
}


/*!
******************************************************************************

 @Function		OSMapPhysToLin

 @Description	Maps the physical memory into linear addr range

 @Input    BasePAddr : physical cpu address

 @Input    ui32Bytes - bytes to map

 @Input    ui32CacheType - cache type

 @Return    : Linear addr of mapping on success, else NULL

 ******************************************************************************/
IMG_VOID *
OSMapPhysToLin(IMG_CPU_PHYADDR BasePAddr,
               IMG_UINT32 ui32Bytes,
               IMG_UINT32 ui32MappingFlags,
               IMG_HANDLE *phOSMemHandle)
{
    if(ui32MappingFlags & PVRSRV_HAP_KERNEL_ONLY)
    {
	/* 
	 * Provide some backwards compatibility, until all callers
	 * have been updated to pass a non-null OSMemHandle pointer.
	 * Such callers must not call OSMapLinToCPUPhys.
	 */
	if(phOSMemHandle == IMG_NULL)
	{
		IMG_VOID *pvIORemapCookie;
		pvIORemapCookie = IORemapWrapper(BasePAddr, ui32Bytes, ui32MappingFlags);
		if(pvIORemapCookie == IMG_NULL)
		{
		    return IMG_NULL;
		}
		return pvIORemapCookie;
	}
	else
	{
		LinuxMemArea *psLinuxMemArea = NewIORemapLinuxMemArea(BasePAddr, ui32Bytes, ui32MappingFlags);

		if(psLinuxMemArea == IMG_NULL)
		{
		    return IMG_NULL;
		}

		*phOSMemHandle = (IMG_HANDLE)psLinuxMemArea;
		return LinuxMemAreaToCpuVAddr(psLinuxMemArea);
	}
    }

    PVR_DPF((PVR_DBG_ERROR,
             "OSMapPhysToLin should only be used with PVRSRV_HAP_KERNEL_ONLY "
             " (Use OSReservePhys otherwise)"));

    return IMG_NULL;
}

/*!
******************************************************************************
 @Function	OSUnMapPhysToLin
 @Description Unmaps memory that was mapped with OSMapPhysToLin
 @Return TRUE on success, else FALSE
******************************************************************************/
IMG_BOOL
OSUnMapPhysToLin(IMG_VOID *pvLinAddr, IMG_UINT32 ui32Bytes, IMG_UINT32 ui32MappingFlags, IMG_HANDLE hOSMemHandle)
{
    PVR_UNREFERENCED_PARAMETER(ui32Bytes);	

    if(ui32MappingFlags & PVRSRV_HAP_KERNEL_ONLY)
    {
        if (hOSMemHandle == IMG_NULL)
	{
		IOUnmapWrapper(pvLinAddr);
	}
	else
	{
		LinuxMemArea *psLinuxMemArea = (LinuxMemArea *)hOSMemHandle;

		PVR_ASSERT(LinuxMemAreaToCpuVAddr(psLinuxMemArea) == pvLinAddr);
		
		FreeIORemapLinuxMemArea(psLinuxMemArea);
	}

        return IMG_TRUE;
    }

    PVR_DPF((PVR_DBG_ERROR,
                 "OSUnMapPhysToLin should only be used with PVRSRV_HAP_KERNEL_ONLY "
                 " (Use OSUnReservePhys otherwise)"));
    return IMG_FALSE;
}

/*!
******************************************************************************
 @Function	RegisterExternalMem
 @Description Registers external memory for user mode mapping
 @Return TRUE on success, else FALSE, MemHandle out
******************************************************************************/
static PVRSRV_ERROR
RegisterExternalMem(IMG_SYS_PHYADDR *pBasePAddr,
          IMG_VOID *pvCPUVAddr,
              IMG_UINT32 ui32Bytes,
          IMG_BOOL bPhysContig,
              IMG_UINT32 ui32MappingFlags,
              IMG_HANDLE *phOSMemHandle)
{
    LinuxMemArea *psLinuxMemArea;

    switch(ui32MappingFlags & PVRSRV_HAP_MAPTYPE_MASK)
    {
        case PVRSRV_HAP_KERNEL_ONLY:
        {
        psLinuxMemArea = NewExternalKVLinuxMemArea(pBasePAddr, pvCPUVAddr, ui32Bytes, bPhysContig, ui32MappingFlags);
        
            if(!psLinuxMemArea)
            {
                return PVRSRV_ERROR_BAD_MAPPING;
            }
            break;
        }
        case PVRSRV_HAP_SINGLE_PROCESS:
        {
        psLinuxMemArea = NewExternalKVLinuxMemArea(pBasePAddr, pvCPUVAddr, ui32Bytes, bPhysContig, ui32MappingFlags);

            if(!psLinuxMemArea)
            {
                return PVRSRV_ERROR_BAD_MAPPING;
            }
            PVRMMapRegisterArea(psLinuxMemArea);
            break;
        }
        case PVRSRV_HAP_MULTI_PROCESS:
        {
            /* Currently PVRSRV_HAP_MULTI_PROCESS implies that we need a kernel
             * virtual mapping and potentially multiple user space virtual mappings.
             * Beware that the kernel virtual address space is a limited resource.
             */
#if defined(VIVT_CACHE) || defined(__sh__)
            /* 
             * ARM9 caches are tagged with virtual pages, not physical. As we are going to
             * share this memory in different address spaces, we don't want it to be cached.
             * ARM11 has physical tagging, so we can cache this memory without fear of virtual
             * address aliasing in the TLB, as long as the kernel supports cache colouring for
             * VIPT architectures.
             */
            ui32MappingFlags &= ~PVRSRV_HAP_CACHED;
#endif
        psLinuxMemArea = NewExternalKVLinuxMemArea(pBasePAddr, pvCPUVAddr, ui32Bytes, bPhysContig, ui32MappingFlags);

            if(!psLinuxMemArea)
            {
                return PVRSRV_ERROR_BAD_MAPPING;
            }
            PVRMMapRegisterArea(psLinuxMemArea);
            break;
        }
        default:
            PVR_DPF((PVR_DBG_ERROR,"OSRegisterMem : invalid flags 0x%x\n", ui32MappingFlags));
            *phOSMemHandle = (IMG_HANDLE)0;
            return PVRSRV_ERROR_INVALID_FLAGS;
    }
    
    *phOSMemHandle = (IMG_HANDLE)psLinuxMemArea;

    LinuxMemAreaRegister(psLinuxMemArea);

    return PVRSRV_OK;
}


/*!
******************************************************************************
 @Function	OSRegisterMem
 @Description Registers external memory for user mode mapping
 @Output phOSMemHandle - handle to registered memory
 @Return TRUE on success, else FALSE
******************************************************************************/
PVRSRV_ERROR
OSRegisterMem(IMG_CPU_PHYADDR BasePAddr,
              IMG_VOID *pvCPUVAddr,
              IMG_UINT32 ui32Bytes,
              IMG_UINT32 ui32MappingFlags,
              IMG_HANDLE *phOSMemHandle)
{
    IMG_SYS_PHYADDR SysPAddr = SysCpuPAddrToSysPAddr(BasePAddr);

    return RegisterExternalMem(&SysPAddr, pvCPUVAddr, ui32Bytes, IMG_TRUE, ui32MappingFlags, phOSMemHandle);
}


PVRSRV_ERROR OSRegisterDiscontigMem(IMG_SYS_PHYADDR *pBasePAddr, IMG_VOID *pvCPUVAddr, IMG_UINT32 ui32Bytes, IMG_UINT32 ui32MappingFlags, IMG_HANDLE *phOSMemHandle)
{
    return RegisterExternalMem(pBasePAddr, pvCPUVAddr, ui32Bytes, IMG_FALSE, ui32MappingFlags, phOSMemHandle);
}


/*!
******************************************************************************
 @Function	OSUnRegisterMem
 @Description UnRegisters external memory for user mode mapping
 @Return TRUE on success, else FALSE
******************************************************************************/
PVRSRV_ERROR
OSUnRegisterMem (IMG_VOID *pvCpuVAddr,
                IMG_UINT32 ui32Bytes,
                IMG_UINT32 ui32MappingFlags,
                IMG_HANDLE hOSMemHandle)
{
    LinuxMemArea *psLinuxMemArea = (LinuxMemArea *)hOSMemHandle;
    PVRSRV_ERROR eError;

    PVR_UNREFERENCED_PARAMETER(pvCpuVAddr);
    PVR_UNREFERENCED_PARAMETER(ui32Bytes);

    switch(ui32MappingFlags & PVRSRV_HAP_MAPTYPE_MASK)
    {
        case PVRSRV_HAP_KERNEL_ONLY:
            break;
        case PVRSRV_HAP_SINGLE_PROCESS:
        case PVRSRV_HAP_MULTI_PROCESS:
        {
            eError = PVRMMapRemoveRegisteredArea(psLinuxMemArea);
            if (eError != PVRSRV_OK)
            {
                 PVR_DPF((PVR_DBG_ERROR, "%s(%p, %d, 0x%08X, %p) FAILED!",
                          __FUNCTION__, pvCpuVAddr, ui32Bytes,
                          ui32MappingFlags, hOSMemHandle));
                return eError;
            }
            break;
        }
        default:
        {
            PVR_DPF((PVR_DBG_ERROR, "OSUnRegisterMem : invalid flags 0x%x", ui32MappingFlags));
            return PVRSRV_ERROR_INVALID_PARAMS;
        }
    }

    LinuxMemAreaDeepFree(psLinuxMemArea);

    return PVRSRV_OK;
}

PVRSRV_ERROR OSUnRegisterDiscontigMem(IMG_VOID *pvCpuVAddr, IMG_UINT32 ui32Bytes, IMG_UINT32 ui32Flags, IMG_HANDLE hOSMemHandle)
{
    return OSUnRegisterMem(pvCpuVAddr, ui32Bytes, ui32Flags, hOSMemHandle);
}

/*!
******************************************************************************
 @Function	OSReservePhys
 @Description Registers physical memory for user mode mapping
 @Output    ppvCpuVAddr
 @Output    phOsMemHandle handle to registered memory
 @Return TRUE on success, else FALSE
******************************************************************************/
PVRSRV_ERROR
OSReservePhys(IMG_CPU_PHYADDR BasePAddr,
              IMG_UINT32 ui32Bytes,
              IMG_UINT32 ui32MappingFlags,
              IMG_HANDLE hBMHandle,
              IMG_VOID **ppvCpuVAddr,
              IMG_HANDLE *phOSMemHandle)
{
    LinuxMemArea *psLinuxMemArea;

#if 0
    /* For debug: force all OSReservePhys reservations to have a kernel
     * virtual address */
    if(ui32MappingFlags & PVRSRV_HAP_SINGLE_PROCESS)
    {
        ui32MappingFlags &= ~PVRSRV_HAP_SINGLE_PROCESS;
        ui32MappingFlags |= PVRSRV_HAP_MULTI_PROCESS;
    }
#endif

    switch(ui32MappingFlags & PVRSRV_HAP_MAPTYPE_MASK)
    {
        case PVRSRV_HAP_KERNEL_ONLY:
        {
            /* Currently PVRSRV_HAP_KERNEL_ONLY implies that a kernel virtual
             * mapping is required for the allocation and no user virtual
             * mappings are allowed: Note these eat into our limited kernel
             * virtual address space */
            psLinuxMemArea = NewIORemapLinuxMemArea(BasePAddr, ui32Bytes, ui32MappingFlags);
            if(!psLinuxMemArea)
            {
                return PVRSRV_ERROR_BAD_MAPPING;
            }
            break;
        }
        case PVRSRV_HAP_SINGLE_PROCESS:
        {
            /* Currently this implies that we dont need a kernel virtual
             * mapping, but will need a user space virtual mapping */
            psLinuxMemArea = NewIOLinuxMemArea(BasePAddr, ui32Bytes, ui32MappingFlags);
            if(!psLinuxMemArea)
            {
                return PVRSRV_ERROR_BAD_MAPPING;
            }
            PVRMMapRegisterArea(psLinuxMemArea);
            break;
        }
        case PVRSRV_HAP_MULTI_PROCESS:
        {
            /* Currently PVRSRV_HAP_MULTI_PROCESS implies that we need a kernel
             * virtual mapping and potentially multiple user space virtual mappings.
             * Beware that the kernel virtual address space is a limited resource.
             */
#if defined(VIVT_CACHE) || defined(__sh__)
            /* 
             * ARM9 caches are tagged with virtual pages, not physical. As we are going to
             * share this memory in different address spaces, we don't want it to be cached.
             * ARM11 has physical tagging, so we can cache this memory without fear of virtual
             * address aliasing in the TLB, as long as the kernel supports cache colouring for
             * VIPT architectures.
             */
            ui32MappingFlags &= ~PVRSRV_HAP_CACHED;
#endif
            psLinuxMemArea = NewIORemapLinuxMemArea(BasePAddr, ui32Bytes, ui32MappingFlags);
            if(!psLinuxMemArea)
            {
                return PVRSRV_ERROR_BAD_MAPPING;
            }
            PVRMMapRegisterArea(psLinuxMemArea);
            break;
        }
        default:
            PVR_DPF((PVR_DBG_ERROR,"OSMapPhysToLin : invalid flags 0x%x\n", ui32MappingFlags));
            *ppvCpuVAddr = NULL;
            *phOSMemHandle = (IMG_HANDLE)0;
            return PVRSRV_ERROR_INVALID_FLAGS;
    }

	/*
		In case of sparse mapping we need to handle back to the BM as it
		knows the mapping info
	*/
	if (ui32MappingFlags & PVRSRV_MEM_SPARSE)
	{
		PVR_ASSERT(hBMHandle != IMG_NULL);
		psLinuxMemArea->hBMHandle = hBMHandle;
	}

    *phOSMemHandle = (IMG_HANDLE)psLinuxMemArea;
    *ppvCpuVAddr = LinuxMemAreaToCpuVAddr(psLinuxMemArea);

    LinuxMemAreaRegister(psLinuxMemArea);

    return PVRSRV_OK;
}

/*!
******************************************************************************
 @Function	OSUnReservePhys
 @Description UnRegisters physical memory for user mode mapping
 @Return TRUE on success, else FALSE
******************************************************************************/
PVRSRV_ERROR
OSUnReservePhys(IMG_VOID *pvCpuVAddr,
                IMG_UINT32 ui32Bytes,
                IMG_UINT32 ui32MappingFlags,
                IMG_HANDLE hOSMemHandle)
{
    LinuxMemArea *psLinuxMemArea;
    PVRSRV_ERROR eError;

    PVR_UNREFERENCED_PARAMETER(pvCpuVAddr);
    PVR_UNREFERENCED_PARAMETER(ui32Bytes);

    psLinuxMemArea = (LinuxMemArea *)hOSMemHandle;
    
    switch(ui32MappingFlags & PVRSRV_HAP_MAPTYPE_MASK)
    {
        case PVRSRV_HAP_KERNEL_ONLY:
            break;
        case PVRSRV_HAP_SINGLE_PROCESS:
        case PVRSRV_HAP_MULTI_PROCESS:
        {
            eError = PVRMMapRemoveRegisteredArea(psLinuxMemArea);
            if (eError != PVRSRV_OK)
            {
                 PVR_DPF((PVR_DBG_ERROR, "%s(%p, %d, 0x%08X, %p) FAILED!",
                          __FUNCTION__, pvCpuVAddr, ui32Bytes,
                          ui32MappingFlags, hOSMemHandle));
                return eError;
            }
            break;
        }
        default:
        {
            PVR_DPF((PVR_DBG_ERROR, "OSUnMapPhysToLin : invalid flags 0x%x", ui32MappingFlags));
            return PVRSRV_ERROR_INVALID_PARAMS;
        }
    }
    
    LinuxMemAreaDeepFree(psLinuxMemArea);

    return PVRSRV_OK;
}


/*!
******************************************************************************
 @Function	OSBaseAllocContigMemory
 @Description Allocate a block of contiguous virtual non-paged memory.
 @Input     ui32Size - number of bytes to allocate
 @Output    ppvLinAddr - pointer to variable that will receive the linear address of buffer
 @Return PVRSRV_OK if allocation successed else returns PVRSRV_ERROR_OUT_OF_MEMORY
 **************************************************************************/
PVRSRV_ERROR OSBaseAllocContigMemory(IMG_UINT32 ui32Size, IMG_CPU_VIRTADDR *pvLinAddr, IMG_CPU_PHYADDR *psPhysAddr)
{
#if !defined(NO_HARDWARE)
    PVR_UNREFERENCED_PARAMETER(ui32Size);
    PVR_UNREFERENCED_PARAMETER(pvLinAddr);
    PVR_UNREFERENCED_PARAMETER(psPhysAddr);
    PVR_DPF((PVR_DBG_ERROR, "%s: Not available", __FUNCTION__));

    return PVRSRV_ERROR_OUT_OF_MEMORY;
#else
/*
 * On Linux, the returned virtual address should be used for CPU access,
 * and not be remapped into the CPU virtual address using ioremap.  The fact
 * that the RAM is being managed by the kernel, and already has a virtual
 * address, seems to lead to problems when the attributes of the memory are
 * changed in the ioremap call (such as from cached to non-cached).
 */
    IMG_VOID *pvKernLinAddr;

#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
    pvKernLinAddr = _KMallocWrapper(ui32Size, GFP_KERNEL, __FILE__, __LINE__);
#else
    pvKernLinAddr = KMallocWrapper(ui32Size, GFP_KERNEL);
#endif
    if (!pvKernLinAddr)
    {
    return PVRSRV_ERROR_OUT_OF_MEMORY;
    }

    *pvLinAddr = pvKernLinAddr;

    psPhysAddr->uiAddr = virt_to_phys(pvKernLinAddr);

    return PVRSRV_OK;
#endif	/* !defined(NO_HARDWARE) */
}


/*!
******************************************************************************
 @Function	OSBaseFreeContigMemory
 @Description Frees memory allocated with OSBaseAllocContigMemory
 @Input     LinAddr - pointer to buffer allocated with OSBaseAllocContigMemory
 **************************************************************************/
PVRSRV_ERROR OSBaseFreeContigMemory(IMG_UINT32 ui32Size, IMG_CPU_VIRTADDR pvLinAddr, IMG_CPU_PHYADDR psPhysAddr)
{
#if !defined(NO_HARDWARE)
    PVR_UNREFERENCED_PARAMETER(ui32Size);
    PVR_UNREFERENCED_PARAMETER(pvLinAddr);
    PVR_UNREFERENCED_PARAMETER(psPhysAddr.uiAddr);

    PVR_DPF((PVR_DBG_WARNING, "%s: Not available", __FUNCTION__));
#else
    PVR_UNREFERENCED_PARAMETER(ui32Size);
    PVR_UNREFERENCED_PARAMETER(psPhysAddr.uiAddr);

    KFreeWrapper(pvLinAddr);
#endif
    return PVRSRV_OK;
}

/*!
******************************************************************************

 @Function	OSWriteHWReg
 
 @Description 
 
 register access function

 @input pvLinRegBaseAddr :	lin addr of register block base

 @input ui32Offset :	 

 @input ui32Value :	
 
 @Return   none

******************************************************************************/

IMG_UINT32 OSReadHWReg(IMG_PVOID pvLinRegBaseAddr, IMG_UINT32 ui32Offset)
{
#if !defined(NO_HARDWARE)
    return (IMG_UINT32) readl((IMG_PBYTE)pvLinRegBaseAddr+ui32Offset);
#else
    return *(IMG_UINT32 *)((IMG_PBYTE)pvLinRegBaseAddr+ui32Offset);
#endif
}

IMG_VOID OSWriteHWReg(IMG_PVOID pvLinRegBaseAddr, IMG_UINT32 ui32Offset, IMG_UINT32 ui32Value)
{
#if !defined(NO_HARDWARE)
//    writel(ui32Value, (IMG_PBYTE)pvLinRegBaseAddr+ui32Offset);
    mt65xx_reg_sync_writel(ui32Value, (IMG_PBYTE)pvLinRegBaseAddr+ui32Offset);
#else
    *(IMG_UINT32 *)((IMG_PBYTE)pvLinRegBaseAddr+ui32Offset) = ui32Value;
#endif
}

#if defined(CONFIG_PCI) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,14))

/*!
******************************************************************************

 @Function	OSPCISetDev
 
 @Description 
 
 Set a PCI device for subsequent use.

 @input pvPCICookie :	Pointer to OS specific PCI structure/cookie

 @input eFlags :	Flags

 @Return   Pointer to PCI device handle

******************************************************************************/
PVRSRV_PCI_DEV_HANDLE OSPCISetDev(IMG_VOID *pvPCICookie, HOST_PCI_INIT_FLAGS eFlags)
{
    int err;
    IMG_UINT32 i;
    PVR_PCI_DEV *psPVRPCI;

    PVR_TRACE(("OSPCISetDev"));

    if(OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(*psPVRPCI), (IMG_VOID **)&psPVRPCI, IMG_NULL,
        "PCI Device") != PVRSRV_OK)
    {
        PVR_DPF((PVR_DBG_ERROR, "OSPCISetDev: Couldn't allocate PVR PCI structure"));
        return IMG_NULL;
    }

    psPVRPCI->psPCIDev = (struct pci_dev *)pvPCICookie;
    psPVRPCI->ePCIFlags = eFlags;

    err = pci_enable_device(psPVRPCI->psPCIDev);
    if (err != 0)
    {
        PVR_DPF((PVR_DBG_ERROR, "OSPCISetDev: Couldn't enable device (%d)", err));
        return IMG_NULL;
    }

    if (psPVRPCI->ePCIFlags & HOST_PCI_INIT_FLAG_BUS_MASTER)	/* PRQA S 3358 */ /* misuse of enums */
    {
        pci_set_master(psPVRPCI->psPCIDev);
    }

    if (psPVRPCI->ePCIFlags & HOST_PCI_INIT_FLAG_MSI)		/* PRQA S 3358 */ /* misuse of enums */
    {
#if defined(CONFIG_PCI_MSI)
        err = pci_enable_msi(psPVRPCI->psPCIDev);
        if (err != 0)
        {
            PVR_DPF((PVR_DBG_WARNING, "OSPCISetDev: Couldn't enable MSI (%d)", err));
            psPVRPCI->ePCIFlags &= ~HOST_PCI_INIT_FLAG_MSI;	/* PRQA S 1474,3358,4130 */ /* misuse of enums */
        }
#else
        PVR_DPF((PVR_DBG_WARNING, "OSPCISetDev: MSI support not enabled in the kernel"));
#endif
    }

    /* Initialise the PCI resource tracking array */
    for (i = 0; i < DEVICE_COUNT_RESOURCE; i++)
    {
        psPVRPCI->abPCIResourceInUse[i] = IMG_FALSE;
    }

    return (PVRSRV_PCI_DEV_HANDLE)psPVRPCI;
}

/*!
******************************************************************************

 @Function	OSPCIAcquireDev
 
 @Description 
 
 Acquire a PCI device for subsequent use.

 @input ui16VendorID :	Vendor PCI ID

 @input ui16VendorID :	Device PCI ID

 @input eFlags :	Flags

 @Return   PVESRV_ERROR

******************************************************************************/
PVRSRV_PCI_DEV_HANDLE OSPCIAcquireDev(IMG_UINT16 ui16VendorID, IMG_UINT16 ui16DeviceID, HOST_PCI_INIT_FLAGS eFlags)
{
    struct pci_dev *psPCIDev;

    psPCIDev = pci_get_device(ui16VendorID, ui16DeviceID, NULL);
    if (psPCIDev == NULL)
    {
        PVR_DPF((PVR_DBG_ERROR, "OSPCIAcquireDev: Couldn't acquire device"));
        return IMG_NULL;
    }

    return OSPCISetDev((IMG_VOID *)psPCIDev, eFlags);
}

/*!
******************************************************************************

 @Function	OSPCIIRQ
 
 @Description 
 
 Get the interrupt number for the device.

 @input hPVRPCI :	PCI device handle

 @input pui32IRQ :	Pointer to where the interrupt number should be returned

 @Return   PVESRV_ERROR

******************************************************************************/
PVRSRV_ERROR OSPCIIRQ(PVRSRV_PCI_DEV_HANDLE hPVRPCI, IMG_UINT32 *pui32IRQ)
{
    PVR_PCI_DEV *psPVRPCI = (PVR_PCI_DEV *)hPVRPCI;

    *pui32IRQ = psPVRPCI->psPCIDev->irq;

    return PVRSRV_OK;
}

/* Functions supported by OSPCIAddrRangeFunc */
enum HOST_PCI_ADDR_RANGE_FUNC
{
    HOST_PCI_ADDR_RANGE_FUNC_LEN,
    HOST_PCI_ADDR_RANGE_FUNC_START,
    HOST_PCI_ADDR_RANGE_FUNC_END,
    HOST_PCI_ADDR_RANGE_FUNC_REQUEST,
    HOST_PCI_ADDR_RANGE_FUNC_RELEASE
};

/*!
******************************************************************************

 @Function	OSPCIAddrRangeFunc
 
 @Description 
 
 Internal support function for various address range related functions

 @input eFunc :	Function to perform

 @input hPVRPCI :	PCI device handle

 @input ui32Index :	Address range index

 @Return   function dependent

******************************************************************************/
static IMG_UINT32 OSPCIAddrRangeFunc(enum HOST_PCI_ADDR_RANGE_FUNC eFunc,
                                     PVRSRV_PCI_DEV_HANDLE hPVRPCI,
                                     IMG_UINT32 ui32Index)
{
    PVR_PCI_DEV *psPVRPCI = (PVR_PCI_DEV *)hPVRPCI;

    if (ui32Index >= DEVICE_COUNT_RESOURCE)
    {
        PVR_DPF((PVR_DBG_ERROR, "OSPCIAddrRangeFunc: Index out of range"));
        return 0;

    }

    switch (eFunc)
    {
        case HOST_PCI_ADDR_RANGE_FUNC_LEN:
            return pci_resource_len(psPVRPCI->psPCIDev, ui32Index);
        case HOST_PCI_ADDR_RANGE_FUNC_START:
            return pci_resource_start(psPVRPCI->psPCIDev, ui32Index);
        case HOST_PCI_ADDR_RANGE_FUNC_END:
            return pci_resource_end(psPVRPCI->psPCIDev, ui32Index);
        case HOST_PCI_ADDR_RANGE_FUNC_REQUEST:
        {
            int err;

            err = pci_request_region(psPVRPCI->psPCIDev, (IMG_INT)ui32Index, PVRSRV_MODNAME);
            if (err != 0)
            {
                PVR_DPF((PVR_DBG_ERROR, "OSPCIAddrRangeFunc: pci_request_region_failed (%d)", err));
                return 0;
            }
            psPVRPCI->abPCIResourceInUse[ui32Index] = IMG_TRUE;
            return 1;
        }
        case HOST_PCI_ADDR_RANGE_FUNC_RELEASE:
            if (psPVRPCI->abPCIResourceInUse[ui32Index])
            {
                pci_release_region(psPVRPCI->psPCIDev, (IMG_INT)ui32Index);
                psPVRPCI->abPCIResourceInUse[ui32Index] = IMG_FALSE;
            }
            return 1;
        default:
            PVR_DPF((PVR_DBG_ERROR, "OSPCIAddrRangeFunc: Unknown function"));
            break;
    }

    return 0;
}

/*!
******************************************************************************

 @Function	OSPCIAddrRangeLen
 
 @Description 
 
 Returns length of a given address range length

 @input hPVRPCI :	PCI device handle

 @input ui32Index :	Address range index

 @Return   Length of address range, or 0 if no such range

******************************************************************************/
IMG_UINT32 OSPCIAddrRangeLen(PVRSRV_PCI_DEV_HANDLE hPVRPCI, IMG_UINT32 ui32Index)
{
    return OSPCIAddrRangeFunc(HOST_PCI_ADDR_RANGE_FUNC_LEN, hPVRPCI, ui32Index); 
}

/*!
******************************************************************************

 @Function	OSPCIAddrRangeStart
 
 @Description 
 
 Returns the start of a given address range

 @input hPVRPCI :	PCI device handle

 @input ui32Index :	Address range index

 @Return   Start of address range, or 0 if no such range

******************************************************************************/
IMG_UINT32 OSPCIAddrRangeStart(PVRSRV_PCI_DEV_HANDLE hPVRPCI, IMG_UINT32 ui32Index)
{
    return OSPCIAddrRangeFunc(HOST_PCI_ADDR_RANGE_FUNC_START, hPVRPCI, ui32Index); 
}

/*!
******************************************************************************

 @Function	OSPCIAddrRangeEnd
 
 @Description 
 
 Returns the end of a given address range

 @input hPVRPCI :	PCI device handle"ayy

 @input ui32Index :	Address range index

 @Return   End of address range, or 0 if no such range

******************************************************************************/
IMG_UINT32 OSPCIAddrRangeEnd(PVRSRV_PCI_DEV_HANDLE hPVRPCI, IMG_UINT32 ui32Index)
{
    return OSPCIAddrRangeFunc(HOST_PCI_ADDR_RANGE_FUNC_END, hPVRPCI, ui32Index); 
}

/*!
******************************************************************************

 @Function	OSPCIRequestAddrRange
 
 @Description 
 
 Request a given address range index for subsequent use

 @input hPVRPCI :	PCI device handle

 @input ui32Index :	Address range index

 @Return   PVESRV_ERROR

******************************************************************************/
PVRSRV_ERROR OSPCIRequestAddrRange(PVRSRV_PCI_DEV_HANDLE hPVRPCI,
                                   IMG_UINT32 ui32Index)
{
    return OSPCIAddrRangeFunc(HOST_PCI_ADDR_RANGE_FUNC_REQUEST, hPVRPCI, ui32Index) == 0 ? PVRSRV_ERROR_PCI_CALL_FAILED : PVRSRV_OK;
}

/*!
******************************************************************************

 @Function	OSPCIReleaseAddrRange
 
 @Description 
 
 Release a given address range that is no longer being used

 @input hPVRPCI :	PCI device handle

 @input ui32Index :	Address range index

 @Return   PVESRV_ERROR

******************************************************************************/
PVRSRV_ERROR OSPCIReleaseAddrRange(PVRSRV_PCI_DEV_HANDLE hPVRPCI, IMG_UINT32 ui32Index)
{
    return OSPCIAddrRangeFunc(HOST_PCI_ADDR_RANGE_FUNC_RELEASE, hPVRPCI, ui32Index) == 0 ? PVRSRV_ERROR_PCI_CALL_FAILED : PVRSRV_OK;
}

/*!
******************************************************************************

 @Function	OSPCIReleaseDev
 
 @Description 
 
 Release a PCI device that is no longer being used

 @input hPVRPCI :	PCI device handle

 @Return   PVESRV_ERROR

******************************************************************************/
PVRSRV_ERROR OSPCIReleaseDev(PVRSRV_PCI_DEV_HANDLE hPVRPCI)
{
    PVR_PCI_DEV *psPVRPCI = (PVR_PCI_DEV *)hPVRPCI;
    int i;

    PVR_TRACE(("OSPCIReleaseDev"));

    /* Release all PCI regions that are currently in use */
    for (i = 0; i < DEVICE_COUNT_RESOURCE; i++)
    {
        if (psPVRPCI->abPCIResourceInUse[i])
        {
            PVR_TRACE(("OSPCIReleaseDev: Releasing Address range %d", i));
            pci_release_region(psPVRPCI->psPCIDev, i);
            psPVRPCI->abPCIResourceInUse[i] = IMG_FALSE;
        }
    }

#if defined(CONFIG_PCI_MSI)
    if (psPVRPCI->ePCIFlags & HOST_PCI_INIT_FLAG_MSI)		/* PRQA S 3358 */ /* misuse of enums */
    {
        pci_disable_msi(psPVRPCI->psPCIDev);
    }
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29))
    if (psPVRPCI->ePCIFlags & HOST_PCI_INIT_FLAG_BUS_MASTER)	/* PRQA S 3358 */ /* misuse of enums */
    {
        pci_clear_master(psPVRPCI->psPCIDev);
    }
#endif
    pci_disable_device(psPVRPCI->psPCIDev);

    OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(*psPVRPCI), (IMG_VOID *)psPVRPCI, IMG_NULL);
	/*not nulling pointer, copy on stack*/

    return PVRSRV_OK;
}

/*!
******************************************************************************

 @Function	OSPCISuspendDev
 
 @Description 
 
 Prepare PCI device to be turned off by power management

 @input hPVRPCI :	PCI device handle

 @Return   PVESRV_ERROR

******************************************************************************/
PVRSRV_ERROR OSPCISuspendDev(PVRSRV_PCI_DEV_HANDLE hPVRPCI)
{
    PVR_PCI_DEV *psPVRPCI = (PVR_PCI_DEV *)hPVRPCI;
    int i;
    int err;

    PVR_TRACE(("OSPCISuspendDev"));

    /* Release all PCI regions that are currently in use */
    for (i = 0; i < DEVICE_COUNT_RESOURCE; i++)
    {
        if (psPVRPCI->abPCIResourceInUse[i])
        {
            pci_release_region(psPVRPCI->psPCIDev, i);
        }
    }

    err = pci_save_state(psPVRPCI->psPCIDev);
    if (err != 0)
    {
        PVR_DPF((PVR_DBG_ERROR, "OSPCISuspendDev: pci_save_state_failed (%d)", err));
        return PVRSRV_ERROR_PCI_CALL_FAILED;
    }

    pci_disable_device(psPVRPCI->psPCIDev);

    err = pci_set_power_state(psPVRPCI->psPCIDev, pci_choose_state(psPVRPCI->psPCIDev, PMSG_SUSPEND));
    switch(err)
    {
        case 0:
            break;
        case -EIO:
            PVR_DPF((PVR_DBG_WARNING, "OSPCISuspendDev: device doesn't support PCI PM"));
            break;
        case -EINVAL:
            PVR_DPF((PVR_DBG_ERROR, "OSPCISuspendDev: can't enter requested power state"));
            break;
        default:
            PVR_DPF((PVR_DBG_ERROR, "OSPCISuspendDev: pci_set_power_state failed (%d)", err));
            break;
    }

    return PVRSRV_OK;
}

/*!
******************************************************************************

 @Function	OSPCIResumeDev
 
 @Description 
 
 Prepare a PCI device to be resumed by power management

 @input hPVRPCI :	PCI device handle

 @input pvPCICookie :	Pointer to OS specific PCI structure/cookie

 @input eFlags :	Flags

 @Return   PVESRV_ERROR

******************************************************************************/
PVRSRV_ERROR OSPCIResumeDev(PVRSRV_PCI_DEV_HANDLE hPVRPCI)
{
    PVR_PCI_DEV *psPVRPCI = (PVR_PCI_DEV *)hPVRPCI;
    int err;
    int i;

    PVR_TRACE(("OSPCIResumeDev"));

    err = pci_set_power_state(psPVRPCI->psPCIDev, pci_choose_state(psPVRPCI->psPCIDev, PMSG_ON));
    switch(err)
    {
        case 0:
            break;
        case -EIO:
            PVR_DPF((PVR_DBG_WARNING, "OSPCIResumeDev: device doesn't support PCI PM"));
            break;
        case -EINVAL:
            PVR_DPF((PVR_DBG_ERROR, "OSPCIResumeDev: can't enter requested power state"));
            return PVRSRV_ERROR_UNKNOWN_POWER_STATE;
        default:
            PVR_DPF((PVR_DBG_ERROR, "OSPCIResumeDev: pci_set_power_state failed (%d)", err));
            return PVRSRV_ERROR_UNKNOWN_POWER_STATE;
    }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38))
    pci_restore_state(psPVRPCI->psPCIDev);
#else
    err = pci_restore_state(psPVRPCI->psPCIDev);
    if (err != 0)
    {
        PVR_DPF((PVR_DBG_ERROR, "OSPCIResumeDev: pci_restore_state failed (%d)", err));
        return PVRSRV_ERROR_PCI_CALL_FAILED;
    }
#endif

    err = pci_enable_device(psPVRPCI->psPCIDev);
    if (err != 0)
    {
        PVR_DPF((PVR_DBG_ERROR, "OSPCIResumeDev: Couldn't enable device (%d)", err));
        return PVRSRV_ERROR_PCI_CALL_FAILED;
    }

    if (psPVRPCI->ePCIFlags & HOST_PCI_INIT_FLAG_BUS_MASTER)	/* PRQA S 3358 */ /* misuse of enums */
        pci_set_master(psPVRPCI->psPCIDev);

    /* Restore the PCI resource tracking array */
    for (i = 0; i < DEVICE_COUNT_RESOURCE; i++)
    {
        if (psPVRPCI->abPCIResourceInUse[i])
        {
            err = pci_request_region(psPVRPCI->psPCIDev, i, PVRSRV_MODNAME);
            if (err != 0)
            {
                PVR_DPF((PVR_DBG_ERROR, "OSPCIResumeDev: pci_request_region_failed (region %d, error %d)", i, err));
            }
        }

    }

    return PVRSRV_OK;
}

#endif /* #if defined(CONFIG_PCI) && (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0)) */

#define	OS_MAX_TIMERS	8

/* Timer callback strucure used by OSAddTimer */
typedef struct TIMER_CALLBACK_DATA_TAG
{
    IMG_BOOL			bInUse;
    PFN_TIMER_FUNC		pfnTimerFunc;
    IMG_VOID 			*pvData;	
    struct timer_list		sTimer;
    IMG_UINT32			ui32Delay;
    IMG_BOOL			bActive;
#if defined(PVR_LINUX_TIMERS_USING_WORKQUEUES) || defined(PVR_LINUX_TIMERS_USING_SHARED_WORKQUEUE)
    struct work_struct		sWork;
#endif
}TIMER_CALLBACK_DATA;

#if defined(PVR_LINUX_TIMERS_USING_WORKQUEUES)
static struct workqueue_struct	*psTimerWorkQueue;
#endif

static TIMER_CALLBACK_DATA sTimers[OS_MAX_TIMERS];

#if defined(PVR_LINUX_TIMERS_USING_WORKQUEUES) || defined(PVR_LINUX_TIMERS_USING_SHARED_WORKQUEUE)
DEFINE_MUTEX(sTimerStructLock);
#else
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,39))
/* The lock is used to control access to sTimers */
/* PRQA S 0671,0685 1 */ /* C99 macro not understood by QAC */
static spinlock_t sTimerStructLock = SPIN_LOCK_UNLOCKED;
#else
static DEFINE_SPINLOCK(sTimerStructLock);
#endif
#endif

static void OSTimerCallbackBody(TIMER_CALLBACK_DATA *psTimerCBData)
{
    if (!psTimerCBData->bActive)
        return;

    /* call timer callback */
    psTimerCBData->pfnTimerFunc(psTimerCBData->pvData);
    
    /* reset timer */
    mod_timer(&psTimerCBData->sTimer, psTimerCBData->ui32Delay + jiffies);
}


/*!
******************************************************************************

 @Function	OSTimerCallbackWrapper
 
 @Description 
 
 OS specific timer callback wrapper function
 
 @Input    ui32Data : timer callback data

 @Return   NONE

******************************************************************************/
static IMG_VOID OSTimerCallbackWrapper(IMG_UINT32 ui32Data)
{
    TIMER_CALLBACK_DATA	*psTimerCBData = (TIMER_CALLBACK_DATA*)ui32Data;
    
#if defined(PVR_LINUX_TIMERS_USING_WORKQUEUES) || defined(PVR_LINUX_TIMERS_USING_SHARED_WORKQUEUE)
    int res;

#if defined(PVR_LINUX_TIMERS_USING_WORKQUEUES)
    res = queue_work(psTimerWorkQueue, &psTimerCBData->sWork);
#else
    res = schedule_work(&psTimerCBData->sWork);
#endif
    if (res == 0)
    {
        PVR_DPF((PVR_DBG_WARNING, "OSTimerCallbackWrapper: work already queued"));		
    }
#else
    OSTimerCallbackBody(psTimerCBData);
#endif
}


#if defined(PVR_LINUX_TIMERS_USING_WORKQUEUES) || defined(PVR_LINUX_TIMERS_USING_SHARED_WORKQUEUE)
static void OSTimerWorkQueueCallBack(struct work_struct *psWork)
{
    TIMER_CALLBACK_DATA *psTimerCBData = container_of(psWork, TIMER_CALLBACK_DATA, sWork);

    OSTimerCallbackBody(psTimerCBData);
}
#endif

/*!
******************************************************************************

 @Function	OSAddTimer
 
 @Description 
 
 OS specific function to install a timer callback
 
 @Input    pfnTimerFunc : timer callback

 @Input    *pvData :callback data
 
 @Input		ui32MsTimeout: callback period 

 @Return   IMG_HANDLE  : valid handle success, NULL failure

******************************************************************************/
IMG_HANDLE OSAddTimer(PFN_TIMER_FUNC pfnTimerFunc, IMG_VOID *pvData, IMG_UINT32 ui32MsTimeout)
{
    TIMER_CALLBACK_DATA	*psTimerCBData;
    IMG_UINT32		ui32i;
#if !(defined(PVR_LINUX_TIMERS_USING_WORKQUEUES) || defined(PVR_LINUX_TIMERS_USING_SHARED_WORKQUEUE))
    unsigned long		ulLockFlags;
#endif

    /* check callback */
    if(!pfnTimerFunc)
    {
        PVR_DPF((PVR_DBG_ERROR, "OSAddTimer: passed invalid callback"));		
        return IMG_NULL;		
    }
    
    /* Allocate timer callback data structure */
#if defined(PVR_LINUX_TIMERS_USING_WORKQUEUES) || defined(PVR_LINUX_TIMERS_USING_SHARED_WORKQUEUE)
    mutex_lock(&sTimerStructLock);
#else
    spin_lock_irqsave(&sTimerStructLock, ulLockFlags);
#endif
    for (ui32i = 0; ui32i < OS_MAX_TIMERS; ui32i++)
    {
        psTimerCBData = &sTimers[ui32i];
        if (!psTimerCBData->bInUse)
        {
            psTimerCBData->bInUse = IMG_TRUE;
            break;
        }
    }
#if defined(PVR_LINUX_TIMERS_USING_WORKQUEUES) || defined(PVR_LINUX_TIMERS_USING_SHARED_WORKQUEUE)
    mutex_unlock(&sTimerStructLock);
#else
    spin_unlock_irqrestore(&sTimerStructLock, ulLockFlags);
#endif
    if (ui32i >= OS_MAX_TIMERS)
    {
        PVR_DPF((PVR_DBG_ERROR, "OSAddTimer: all timers are in use"));		
        return IMG_NULL;	
    }

    psTimerCBData->pfnTimerFunc = pfnTimerFunc;
    psTimerCBData->pvData = pvData;
    psTimerCBData->bActive = IMG_FALSE;
    
    /*
        HZ = ticks per second
        ui32MsTimeout = required ms delay
        ticks = (Hz * ui32MsTimeout) / 1000	
    */
    psTimerCBData->ui32Delay = ((HZ * ui32MsTimeout) < 1000)
                                ?	1
                                :	((HZ * ui32MsTimeout) / 1000);
    /* initialise object */
    init_timer(&psTimerCBData->sTimer);
    
    /* setup timer object */
    /* PRQA S 0307,0563 1 */ /* ignore warning about inconpartible ptr casting */
    psTimerCBData->sTimer.function = (IMG_VOID *)OSTimerCallbackWrapper;
    psTimerCBData->sTimer.data = (IMG_UINT32)psTimerCBData;
    
    return (IMG_HANDLE)(ui32i + 1);
}


static inline TIMER_CALLBACK_DATA *GetTimerStructure(IMG_HANDLE hTimer)
{
    IMG_UINT32 ui32i = ((IMG_UINT32)hTimer) - 1;

    PVR_ASSERT(ui32i < OS_MAX_TIMERS);

    return &sTimers[ui32i];
}

/*!
******************************************************************************

 @Function	OSRemoveTimer
 
 @Description 
 
 OS specific function to remove a timer callback
 
 @Input    hTimer : timer handle

 @Return   PVRSRV_ERROR  : 

******************************************************************************/
PVRSRV_ERROR OSRemoveTimer (IMG_HANDLE hTimer)
{
    TIMER_CALLBACK_DATA *psTimerCBData = GetTimerStructure(hTimer);

    PVR_ASSERT(psTimerCBData->bInUse);
    PVR_ASSERT(!psTimerCBData->bActive);

    /* free timer callback data struct */
    psTimerCBData->bInUse = IMG_FALSE;
    
    return PVRSRV_OK;
}


/*!
******************************************************************************

 @Function	OSEnableTimer
 
 @Description 
 
 OS specific function to enable a timer callback
 
 @Input    hTimer : timer handle

 @Return   PVRSRV_ERROR  : 

******************************************************************************/
PVRSRV_ERROR OSEnableTimer (IMG_HANDLE hTimer)
{
    TIMER_CALLBACK_DATA *psTimerCBData = GetTimerStructure(hTimer);

    PVR_ASSERT(psTimerCBData->bInUse);
    PVR_ASSERT(!psTimerCBData->bActive);

    /* Start timer arming */
    psTimerCBData->bActive = IMG_TRUE;

    /* set the expire time */
    psTimerCBData->sTimer.expires = psTimerCBData->ui32Delay + jiffies;

    /* Add the timer to the list */
    add_timer(&psTimerCBData->sTimer);
    
    return PVRSRV_OK;
}


/*!
******************************************************************************

 @Function	OSDisableTimer
 
 @Description 
 
 OS specific function to disable a timer callback
 
 @Input    hTimer : timer handle

 @Return   PVRSRV_ERROR  : 

******************************************************************************/
PVRSRV_ERROR OSDisableTimer (IMG_HANDLE hTimer)
{
    TIMER_CALLBACK_DATA *psTimerCBData = GetTimerStructure(hTimer);

    PVR_ASSERT(psTimerCBData->bInUse);
    PVR_ASSERT(psTimerCBData->bActive);

    /* Stop timer from arming */
    psTimerCBData->bActive = IMG_FALSE;
    smp_mb();

#if defined(PVR_LINUX_TIMERS_USING_WORKQUEUES)
    flush_workqueue(psTimerWorkQueue);
#endif
#if defined(PVR_LINUX_TIMERS_USING_SHARED_WORKQUEUE)
    flush_scheduled_work();
#endif

    /* remove timer */
    del_timer_sync(&psTimerCBData->sTimer);	
    
#if defined(PVR_LINUX_TIMERS_USING_WORKQUEUES)
    /*
     * This second flush is to catch the case where the timer ran
     * before we managed to delete it, in which case, it will have
     * queued more work for the workqueue.  Since the bActive flag
     * has been cleared, this second flush won't result in the
     * timer being rearmed.
     */
    flush_workqueue(psTimerWorkQueue);
#endif
#if defined(PVR_LINUX_TIMERS_USING_SHARED_WORKQUEUE)
    flush_scheduled_work();
#endif

    return PVRSRV_OK;
}


/*!
******************************************************************************

 @Function	OSEventObjectCreateKM
 
 @Description 
 
 OS specific function to create an event object
 
 @Input    pszName : Globally unique event object name (if null name must be autogenerated)
 
 @Output   psEventObject : OS event object info structure

 @Return   PVRSRV_ERROR  : 

******************************************************************************/
#if defined (SUPPORT_SID_INTERFACE)
PVRSRV_ERROR OSEventObjectCreateKM(const IMG_CHAR *pszName, PVRSRV_EVENTOBJECT_KM *psEventObject)
#else
PVRSRV_ERROR OSEventObjectCreateKM(const IMG_CHAR *pszName, PVRSRV_EVENTOBJECT *psEventObject)
#endif
{

    PVRSRV_ERROR eError = PVRSRV_OK;
    
    if(psEventObject)
    {
        if(pszName)
        {
            /* copy over the event object name */
            strncpy(psEventObject->szName, pszName, EVENTOBJNAME_MAXLENGTH);
        }
        else
        {
            /* autogenerate a name */	
            static IMG_UINT16 ui16NameIndex = 0;			
#if defined (SUPPORT_SID_INTERFACE)
            snprintf(psEventObject->szName, EVENTOBJNAME_MAXLENGTH, "PVRSRV_EVENTOBJECT_KM_%d", ui16NameIndex++);
#else
            snprintf(psEventObject->szName, EVENTOBJNAME_MAXLENGTH, "PVRSRV_EVENTOBJECT_%d", ui16NameIndex++);
#endif
        }
        
        if(LinuxEventObjectListCreate(&psEventObject->hOSEventKM) != PVRSRV_OK)
        {
             eError = PVRSRV_ERROR_OUT_OF_MEMORY;	
        }

    }
    else
    {
        PVR_DPF((PVR_DBG_ERROR, "OSEventObjectCreateKM: psEventObject is not a valid pointer"));
        eError = PVRSRV_ERROR_UNABLE_TO_CREATE_EVENT;	
    }
    
    return eError;

}


/*!
******************************************************************************

 @Function	OSEventObjectDestroyKM
 
 @Description 
 
 OS specific function to destroy an event object
 
 @Input    psEventObject : OS event object info structure

 @Return   PVRSRV_ERROR  : 

******************************************************************************/
#if defined (SUPPORT_SID_INTERFACE)
PVRSRV_ERROR OSEventObjectDestroyKM(PVRSRV_EVENTOBJECT_KM *psEventObject)
#else
PVRSRV_ERROR OSEventObjectDestroyKM(PVRSRV_EVENTOBJECT *psEventObject)
#endif
{
    PVRSRV_ERROR eError = PVRSRV_OK;

    if(psEventObject)
    {
        if(psEventObject->hOSEventKM)
        {
            LinuxEventObjectListDestroy(psEventObject->hOSEventKM);
        }
        else
        {
            PVR_DPF((PVR_DBG_ERROR, "OSEventObjectDestroyKM: hOSEventKM is not a valid pointer"));
            eError = PVRSRV_ERROR_INVALID_PARAMS;
        }
    }
    else
    {
        PVR_DPF((PVR_DBG_ERROR, "OSEventObjectDestroyKM: psEventObject is not a valid pointer"));
        eError = PVRSRV_ERROR_INVALID_PARAMS;
    }
    
    return eError;
}

/*!
******************************************************************************

 @Function	OSEventObjectWaitKM
 
 @Description 
 
 OS specific function to wait for an event object.  Called from client
 
 @Input    hOSEventKM : OS and kernel specific handle to event object

 @Return   PVRSRV_ERROR  : 

******************************************************************************/
PVRSRV_ERROR OSEventObjectWaitKM(IMG_HANDLE hOSEventKM)
{
    PVRSRV_ERROR eError;
    
    if(hOSEventKM)
    {
        eError = LinuxEventObjectWait(hOSEventKM, EVENT_OBJECT_TIMEOUT_MS);
    }
    else
    {
        PVR_DPF((PVR_DBG_ERROR, "OSEventObjectWaitKM: hOSEventKM is not a valid handle"));
        eError = PVRSRV_ERROR_INVALID_PARAMS;
    }
    
    return eError;
}

/*!
******************************************************************************

 @Function	OSEventObjectOpenKM
 
 @Description 
 
 OS specific function to open an event object.  Called from client
 
 @Input    psEventObject : Pointer to an event object
 @Output   phOSEvent : OS and kernel specific handle to event object

 @Return   PVRSRV_ERROR  : 

******************************************************************************/
#if defined (SUPPORT_SID_INTERFACE)
PVRSRV_ERROR OSEventObjectOpenKM(PVRSRV_EVENTOBJECT_KM *psEventObject,
#else
PVRSRV_ERROR OSEventObjectOpenKM(PVRSRV_EVENTOBJECT *psEventObject,
#endif
                                            IMG_HANDLE *phOSEvent)
{
    PVRSRV_ERROR eError = PVRSRV_OK;
    
    if(psEventObject)
    {
        if(LinuxEventObjectAdd(psEventObject->hOSEventKM, phOSEvent) != PVRSRV_OK)
        {
            PVR_DPF((PVR_DBG_ERROR, "LinuxEventObjectAdd: failed"));
            eError = PVRSRV_ERROR_INVALID_PARAMS;
        }

    }
    else
    {
        PVR_DPF((PVR_DBG_ERROR, "OSEventObjectCreateKM: psEventObject is not a valid pointer"));
        eError = PVRSRV_ERROR_INVALID_PARAMS;
    }
    
    return eError;
}

/*!
******************************************************************************

 @Function	OSEventObjectCloseKM
 
 @Description 
 
 OS specific function to close an event object.  Called from client
 
 @Input    psEventObject : Pointer to an event object
 @OInput   hOSEventKM : OS and kernel specific handle to event object


 @Return   PVRSRV_ERROR  : 

******************************************************************************/
#if defined (SUPPORT_SID_INTERFACE)
PVRSRV_ERROR OSEventObjectCloseKM(PVRSRV_EVENTOBJECT_KM *psEventObject,
#else
PVRSRV_ERROR OSEventObjectCloseKM(PVRSRV_EVENTOBJECT *psEventObject,
#endif
                                            IMG_HANDLE hOSEventKM)
{
    PVRSRV_ERROR eError = PVRSRV_OK;

    if(psEventObject)
    {
        if(LinuxEventObjectDelete(psEventObject->hOSEventKM, hOSEventKM) != PVRSRV_OK)
        {
            PVR_DPF((PVR_DBG_ERROR, "LinuxEventObjectDelete: failed"));
            eError = PVRSRV_ERROR_INVALID_PARAMS;
        }

    }
    else
    {
        PVR_DPF((PVR_DBG_ERROR, "OSEventObjectDestroyKM: psEventObject is not a valid pointer"));
        eError = PVRSRV_ERROR_INVALID_PARAMS;
    }
    
    return eError;
    
}

/*!
******************************************************************************

 @Function	OSEventObjectSignalKM
 
 @Description 
 
 OS specific function to 'signal' an event object.  Called from L/MISR
 
 @Input    hOSEventKM : OS and kernel specific handle to event object

 @Return   PVRSRV_ERROR  : 

******************************************************************************/
PVRSRV_ERROR OSEventObjectSignalKM(IMG_HANDLE hOSEventKM)
{
    PVRSRV_ERROR eError;
    
    if(hOSEventKM)
    {
        eError = LinuxEventObjectSignal(hOSEventKM);
    }
    else
    {
        PVR_DPF((PVR_DBG_ERROR, "OSEventObjectSignalKM: hOSEventKM is not a valid handle"));
        eError = PVRSRV_ERROR_INVALID_PARAMS;
    }
    
    return eError;
}

/*!
******************************************************************************

 @Function	OSProcHasPrivSrvInit
 
 @Description 
 
 Does the process have sufficient privileges to initialise services?
 
 @Input    none

 @Return   IMG_BOOL :

******************************************************************************/
IMG_BOOL OSProcHasPrivSrvInit(IMG_VOID)
{
    return (capable(CAP_SYS_MODULE) != 0) ? IMG_TRUE : IMG_FALSE;
}

/*!
******************************************************************************

 @Function	OSCopyToUser
 
 @Description 
 
 Copy a block of data into user space
 
 @Input    pvSrc
 
 @Output    pvDest

 @Input 	ui32Bytes 

 @Return   PVRSRV_ERROR  : 

******************************************************************************/
PVRSRV_ERROR OSCopyToUser(IMG_PVOID pvProcess, 
                          IMG_VOID *pvDest, 
                          IMG_VOID *pvSrc, 
                          IMG_UINT32 ui32Bytes)
{
    PVR_UNREFERENCED_PARAMETER(pvProcess);

    if(pvr_copy_to_user(pvDest, pvSrc, ui32Bytes)==0)
        return PVRSRV_OK;
    else
        return PVRSRV_ERROR_FAILED_TO_COPY_VIRT_MEMORY;
}

/*!
******************************************************************************

 @Function	OSCopyFromUser
 
 @Description 
 
 Copy a block of data from the user space
 
 @Output    pvDest
 
 @Input    pvSrc

 @Input 	ui32Bytes 

 @Return   PVRSRV_ERROR  : 

******************************************************************************/
PVRSRV_ERROR OSCopyFromUser( IMG_PVOID pvProcess, 
                             IMG_VOID *pvDest, 
                             IMG_VOID *pvSrc, 
                             IMG_UINT32 ui32Bytes)
{
    PVR_UNREFERENCED_PARAMETER(pvProcess);

    if(pvr_copy_from_user(pvDest, pvSrc, ui32Bytes)==0)
        return PVRSRV_OK;
    else
        return PVRSRV_ERROR_FAILED_TO_COPY_VIRT_MEMORY;
}

/*!
******************************************************************************

 @Function	OSAccessOK
 
 @Description 
 
 Checks if a user space pointer is valide
 
 @Input    eVerification

 @Input    pvUserPtr

 @Input 	ui32Bytes 

 @Return   IMG_BOOL :

******************************************************************************/
IMG_BOOL OSAccessOK(IMG_VERIFY_TEST eVerification, IMG_VOID *pvUserPtr, IMG_UINT32 ui32Bytes)
{
    IMG_INT linuxType;

    if (eVerification == PVR_VERIFY_READ)
    {
        linuxType = VERIFY_READ;
    }
    else
    {
        PVR_ASSERT(eVerification == PVR_VERIFY_WRITE);
        linuxType = VERIFY_WRITE;
    }

    return access_ok(linuxType, pvUserPtr, ui32Bytes);
}

typedef enum _eWrapMemType_
{
    WRAP_TYPE_NULL = 0,
    WRAP_TYPE_GET_USER_PAGES,
    WRAP_TYPE_FIND_VMA
} eWrapMemType;

typedef struct _sWrapMemInfo_
{
    eWrapMemType eType;
    IMG_INT iNumPages;
    IMG_INT iNumPagesMapped;
    struct page **ppsPages;
    IMG_SYS_PHYADDR *psPhysAddr;
    IMG_INT iPageOffset;
#if defined(DEBUG)
    IMG_UINT32 ulStartAddr;
    IMG_UINT32 ulBeyondEndAddr;
    struct vm_area_struct *psVMArea;
#endif
} sWrapMemInfo;


/*!
******************************************************************************

 @Function	*CPUVAddrToPFN
 
 @Description 
 
 Find the PFN associated with a given CPU virtual address, and return
 the associated page structure, if it exists.
 The page in question must be present (i.e. no fault handling required),
 and must be writable.  A get_page is done on the returned page structure.
 
 @Input    psVMArea - pointer to VM area structure
       ulCPUVAddr - CPU virtual address
       pulPFN - Pointer to returned PFN.
       ppsPAge - Pointer to returned page structure pointer.

 @Output   *pulPFN - Set to PFN
 	   *ppsPage - Pointer to the page structure if present, else NULL.
 @Return   IMG_TRUE if PFN lookup was succesful.

******************************************************************************/
static IMG_BOOL CPUVAddrToPFN(struct vm_area_struct *psVMArea, IMG_UINT32 ulCPUVAddr, IMG_UINT32 *pulPFN, struct page **ppsPage)
{
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,10))
    pgd_t *psPGD;
    pud_t *psPUD;
    pmd_t *psPMD;
    pte_t *psPTE;
    struct mm_struct *psMM = psVMArea->vm_mm;
    spinlock_t *psPTLock;
    IMG_BOOL bRet = IMG_FALSE;

    *pulPFN = 0;
    *ppsPage = NULL;

    psPGD = pgd_offset(psMM, ulCPUVAddr);
    if (pgd_none(*psPGD) || pgd_bad(*psPGD))
        return bRet;

    psPUD = pud_offset(psPGD, ulCPUVAddr);
    if (pud_none(*psPUD) || pud_bad(*psPUD))
        return bRet;

    psPMD = pmd_offset(psPUD, ulCPUVAddr);
    if (pmd_none(*psPMD) || pmd_bad(*psPMD))
        return bRet;

    psPTE = (pte_t *)pte_offset_map_lock(psMM, psPMD, ulCPUVAddr, &psPTLock);

    if ((pte_none(*psPTE) == 0) && (pte_present(*psPTE) != 0) && (pte_write(*psPTE) != 0))
    {
        *pulPFN = pte_pfn(*psPTE);
	bRet = IMG_TRUE;

        if (pfn_valid(*pulPFN))
        {
            *ppsPage = pfn_to_page(*pulPFN);

            get_page(*ppsPage);
        }
    }

    pte_unmap_unlock(psPTE, psPTLock);

    return bRet;
#else
    return IMG_FALSE;
#endif
}

/*!
******************************************************************************

 @Function	OSReleasePhysPageAddr
 
 @Description 
 
 Release wrapped memory.

 @Input    hOSWrapMem : Driver cookie

 @Return   PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR OSReleasePhysPageAddr(IMG_HANDLE hOSWrapMem)
{
    sWrapMemInfo *psInfo = (sWrapMemInfo *)hOSWrapMem;
    IMG_INT i;

    if (psInfo == IMG_NULL)
    {
        PVR_DPF((PVR_DBG_WARNING,
            "OSReleasePhysPageAddr: called with null wrap handle"));
	return PVRSRV_OK;
    }

    switch (psInfo->eType)
    {
	case WRAP_TYPE_NULL:
	{
            PVR_DPF((PVR_DBG_WARNING,
                "OSReleasePhysPageAddr: called with wrap type WRAP_TYPE_NULL"));
	    break;
	}
        case WRAP_TYPE_GET_USER_PAGES:
        {
            for (i = 0; i < psInfo->iNumPagesMapped; i++)
            {
                struct page *psPage = psInfo->ppsPages[i];

		PVR_ASSERT(psPage != NULL);

                /*
		 * If the number of pages mapped is not the same as
		 * the number of pages in the address range, then
		 * get_user_pages must have failed, so we are cleaning
		 * up after failure, and the pages can't be dirty.
	 	 */
		if (psInfo->iNumPagesMapped == psInfo->iNumPages)
		{
                    if (!PageReserved(psPage))
                    {
                        SetPageDirty(psPage);
                    }
	        }
                page_cache_release(psPage);
	    }
            break;
        }
        case WRAP_TYPE_FIND_VMA:
        {
            for (i = 0; i < psInfo->iNumPages; i++)
            {
		if (psInfo->ppsPages[i] != IMG_NULL)
		{
                    put_page(psInfo->ppsPages[i]);
		}
            }
            break;
        }
        default:
        {
            PVR_DPF((PVR_DBG_ERROR,
                "OSReleasePhysPageAddr: Unknown wrap type (%d)", psInfo->eType));
            return PVRSRV_ERROR_INVALID_WRAP_TYPE;
        }
    }

    if (psInfo->ppsPages != IMG_NULL)
    {
        kfree(psInfo->ppsPages);
    }

    if (psInfo->psPhysAddr != IMG_NULL)
    {
        kfree(psInfo->psPhysAddr);
    }

    kfree(psInfo);

    return PVRSRV_OK;
}

#if defined(CONFIG_TI_TILER)

static IMG_UINT32 CPUAddrToTilerPhy(IMG_UINT32 uiAddr)
{
	IMG_UINT32 ui32PhysAddr = 0;
	pte_t *ptep, pte;
	pgd_t *pgd;
	pmd_t *pmd;

	pgd = pgd_offset(current->mm, uiAddr);
	if (pgd_none(*pgd) || pgd_bad(*pgd))
		goto err_out;

	pmd = pmd_offset(pgd, uiAddr);
	if (pmd_none(*pmd) || pmd_bad(*pmd))
		goto err_out;

	ptep = pte_offset_map(pmd, uiAddr);
	if (!ptep)
		goto err_out;

	pte = *ptep;
	if (!pte_present(pte))
		goto err_out;

	ui32PhysAddr = (pte & PAGE_MASK) | (~PAGE_MASK & uiAddr);

	/* If the physAddr is not in the TILER physical range
	 * then we don't proceed.
	 */
	if (ui32PhysAddr < 0x60000000 && ui32PhysAddr > 0x7fffffff)
	{
		PVR_DPF((PVR_DBG_ERROR, "CPUAddrToTilerPhy: Not in tiler range"));
		ui32PhysAddr = 0;
		goto err_out;
	}

err_out:
	return ui32PhysAddr;
}

#endif /* defined(CONFIG_TI_TILER) */

/*!
******************************************************************************

 @Function	OSAcquirePhysPageAddr
 
 @Description 
 
 @Return   PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR OSAcquirePhysPageAddr(IMG_VOID *pvCPUVAddr, 
                                    IMG_UINT32 ui32Bytes, 
                                    IMG_SYS_PHYADDR *psSysPAddr,
                                    IMG_HANDLE *phOSWrapMem)
{
    IMG_UINT32 ulStartAddrOrig = (IMG_UINT32) pvCPUVAddr;
    IMG_UINT32 ulAddrRangeOrig = (IMG_UINT32) ui32Bytes;
    IMG_UINT32 ulBeyondEndAddrOrig = ulStartAddrOrig + ulAddrRangeOrig;
    IMG_UINT32 ulStartAddr;
    IMG_UINT32 ulAddrRange;
    IMG_UINT32 ulBeyondEndAddr;
    IMG_UINT32 ulAddr;
    IMG_INT i;
    struct vm_area_struct *psVMArea;
    sWrapMemInfo *psInfo = NULL;
    IMG_BOOL bHavePageStructs = IMG_FALSE;
    IMG_BOOL bHaveNoPageStructs = IMG_FALSE;
    IMG_BOOL bMMapSemHeld = IMG_FALSE;
    PVRSRV_ERROR eError = PVRSRV_ERROR_OUT_OF_MEMORY;

    /* Align start and end addresses to page boundaries */
    ulStartAddr = ulStartAddrOrig & PAGE_MASK;
    ulBeyondEndAddr = PAGE_ALIGN(ulBeyondEndAddrOrig);
    ulAddrRange = ulBeyondEndAddr - ulStartAddr;

    /*
     * Check for address range calculation overflow, and attempts to wrap
     * zero bytes.
     */
    if (ulBeyondEndAddr <= ulStartAddr)
    {
        PVR_DPF((PVR_DBG_ERROR,
            "OSAcquirePhysPageAddr: Invalid address range (start %x, length %x)",
		ulStartAddrOrig, ulAddrRangeOrig));
        goto error;
    }

    /* Allocate information structure */
    psInfo = kmalloc(sizeof(*psInfo), GFP_KERNEL);
    if (psInfo == NULL)
    {
        PVR_DPF((PVR_DBG_ERROR,
            "OSAcquirePhysPageAddr: Couldn't allocate information structure"));
        goto error;
    }
    memset(psInfo, 0, sizeof(*psInfo));

#if defined(DEBUG)
    psInfo->ulStartAddr = ulStartAddrOrig;
    psInfo->ulBeyondEndAddr = ulBeyondEndAddrOrig;
#endif

    psInfo->iNumPages = (IMG_INT)(ulAddrRange >> PAGE_SHIFT);
    psInfo->iPageOffset = (IMG_INT)(ulStartAddrOrig & ~PAGE_MASK);

    /* Allocate physical address array */
    psInfo->psPhysAddr = kmalloc((size_t)psInfo->iNumPages * sizeof(*psInfo->psPhysAddr), GFP_KERNEL);
    if (psInfo->psPhysAddr == NULL)
    {
        PVR_DPF((PVR_DBG_ERROR,
            "OSAcquirePhysPageAddr: Couldn't allocate page array"));		
        goto error;
    }
    memset(psInfo->psPhysAddr, 0, (size_t)psInfo->iNumPages * sizeof(*psInfo->psPhysAddr));

    /* Allocate page array */
    psInfo->ppsPages = kmalloc((size_t)psInfo->iNumPages * sizeof(*psInfo->ppsPages),  GFP_KERNEL);
    if (psInfo->ppsPages == NULL)
    {
        PVR_DPF((PVR_DBG_ERROR,
            "OSAcquirePhysPageAddr: Couldn't allocate page array"));		
        goto error;
    }
    memset(psInfo->ppsPages, 0, (size_t)psInfo->iNumPages * sizeof(*psInfo->ppsPages));

    /* Default error code from now on */
    eError = PVRSRV_ERROR_BAD_MAPPING;

    /* Set the mapping type to aid clean up */
    psInfo->eType = WRAP_TYPE_GET_USER_PAGES;

    /* Lock down user memory */
    down_read(&current->mm->mmap_sem);
    bMMapSemHeld = IMG_TRUE;

    /* Get page list */
    psInfo->iNumPagesMapped = get_user_pages(current, current->mm, ulStartAddr, psInfo->iNumPages, 1, 0, psInfo->ppsPages, NULL);

    if (psInfo->iNumPagesMapped >= 0)
    {
        /* See if we got all the pages we wanted */
        if (psInfo->iNumPagesMapped != psInfo->iNumPages)
        {
            PVR_TRACE(("OSAcquirePhysPageAddr: Couldn't map all the pages needed (wanted: %d, got %d)", psInfo->iNumPages, psInfo->iNumPagesMapped));

            goto error;
        }

        /* Build list of physical page addresses */
        for (i = 0; i < psInfo->iNumPages; i++)
        {
            IMG_CPU_PHYADDR CPUPhysAddr;
	    IMG_UINT32 ulPFN;

            ulPFN = page_to_pfn(psInfo->ppsPages[i]);
            CPUPhysAddr.uiAddr = ulPFN << PAGE_SHIFT;
	    if ((CPUPhysAddr.uiAddr >> PAGE_SHIFT) != ulPFN)
	    {
                PVR_DPF((PVR_DBG_ERROR,
		    "OSAcquirePhysPageAddr: Page frame number out of range (%x)", ulPFN));

		    goto error;
	    }
            psInfo->psPhysAddr[i] = SysCpuPAddrToSysPAddr(CPUPhysAddr);
            psSysPAddr[i] = psInfo->psPhysAddr[i];
            
        }

        goto exit;
    }

    PVR_DPF((PVR_DBG_MESSAGE, "OSAcquirePhysPageAddr: get_user_pages failed (%d), using CPU page table", psInfo->iNumPagesMapped));
    
    /* Reset some fields */
    psInfo->eType = WRAP_TYPE_NULL;
    psInfo->iNumPagesMapped = 0;
    memset(psInfo->ppsPages, 0, (size_t)psInfo->iNumPages * sizeof(*psInfo->ppsPages));

    /*
     * get_user_pages didn't work.  If this is due to the address range
     * representing memory mapped I/O, then we'll look for the pages
     * in the appropriate memory region of the process.
     */

    /* Set the mapping type to aid clean up */
    psInfo->eType = WRAP_TYPE_FIND_VMA;

    psVMArea = find_vma(current->mm, ulStartAddrOrig);
    if (psVMArea == NULL)
    {
        PVR_DPF((PVR_DBG_ERROR,
            "OSAcquirePhysPageAddr: Couldn't find memory region containing start address %x", ulStartAddrOrig));
        
        goto error;
    }
#if defined(DEBUG)
    psInfo->psVMArea = psVMArea;
#endif

    /*
     * find_vma locates a region with an end point past a given
     * virtual address.  So check the address is actually in the region.
     */
    if (ulStartAddrOrig < psVMArea->vm_start)
    {
        PVR_DPF((PVR_DBG_ERROR,
            "OSAcquirePhysPageAddr: Start address %x is outside of the region returned by find_vma", ulStartAddrOrig));
        goto error;
    }

    /* Now check the end address is in range */
    if (ulBeyondEndAddrOrig > psVMArea->vm_end)
    {
        PVR_DPF((PVR_DBG_ERROR,
            "OSAcquirePhysPageAddr: End address %x is outside of the region returned by find_vma", ulBeyondEndAddrOrig));
        goto error;
    }

    /* Does the region represent memory mapped I/O? */
    if ((psVMArea->vm_flags & (VM_IO | VM_RESERVED)) != (VM_IO | VM_RESERVED))
    {
        PVR_DPF((PVR_DBG_ERROR,
            "OSAcquirePhysPageAddr: Memory region does not represent memory mapped I/O (VMA flags: 0x%lx)", psVMArea->vm_flags));
        goto error;
    }

    /* We require read and write access */
    if ((psVMArea->vm_flags & (VM_READ | VM_WRITE)) != (VM_READ | VM_WRITE))
    {
        PVR_DPF((PVR_DBG_ERROR,
            "OSAcquirePhysPageAddr: No read/write access to memory region (VMA flags: 0x%lx)", psVMArea->vm_flags));
        goto error;
    }

    for (ulAddr = ulStartAddrOrig, i = 0; ulAddr < ulBeyondEndAddrOrig; ulAddr += PAGE_SIZE, i++)
    {
	IMG_CPU_PHYADDR CPUPhysAddr;
	IMG_UINT32 ulPFN = 0;

	PVR_ASSERT(i < psInfo->iNumPages);

	if (!CPUVAddrToPFN(psVMArea, ulAddr, &ulPFN, &psInfo->ppsPages[i]))
	{
            PVR_DPF((PVR_DBG_ERROR,
	       "OSAcquirePhysPageAddr: Invalid CPU virtual address"));

	    goto error;
	}
	if (psInfo->ppsPages[i] == NULL)
	{
#if defined(CONFIG_TI_TILER)
		/* This could be tiler memory.*/
		IMG_UINT32 ui32TilerAddr = CPUAddrToTilerPhy(ulAddr);
		if (ui32TilerAddr)
		{
			bHavePageStructs = IMG_TRUE;
			psInfo->iNumPagesMapped++;
			psInfo->psPhysAddr[i].uiAddr = ui32TilerAddr;
			psSysPAddr[i].uiAddr = ui32TilerAddr;
			continue;
		}
#endif /* defined(CONFIG_TI_TILER) */

	    bHaveNoPageStructs = IMG_TRUE;
	}
	else
	{
	    bHavePageStructs = IMG_TRUE;

	    psInfo->iNumPagesMapped++;

	    PVR_ASSERT(ulPFN == page_to_pfn(psInfo->ppsPages[i]));
	}

        CPUPhysAddr.uiAddr = ulPFN << PAGE_SHIFT;
	if ((CPUPhysAddr.uiAddr >> PAGE_SHIFT) != ulPFN)
	{
                PVR_DPF((PVR_DBG_ERROR,
		    "OSAcquirePhysPageAddr: Page frame number out of range (%x)", ulPFN));

		    goto error;
	}

	psInfo->psPhysAddr[i] = SysCpuPAddrToSysPAddr(CPUPhysAddr);
	psSysPAddr[i] = psInfo->psPhysAddr[i];
    }
    PVR_ASSERT(i ==  psInfo->iNumPages);

#if defined(VM_MIXEDMAP)
    if ((psVMArea->vm_flags & VM_MIXEDMAP) != 0)
    {
        goto exit;
    }
#endif

    if (bHavePageStructs && bHaveNoPageStructs)
    {
        PVR_DPF((PVR_DBG_ERROR,
            "OSAcquirePhysPageAddr: Region is VM_MIXEDMAP, but isn't marked as such"));
	goto error;
    }

    if (!bHaveNoPageStructs)
    {
	/* The ideal case; every page has a page structure */
	goto exit;
    }

#if defined(VM_PFNMAP)
    if ((psVMArea->vm_flags & VM_PFNMAP) == 0)
#endif
    {
        PVR_DPF((PVR_DBG_ERROR,
            "OSAcquirePhysPageAddr: Region is VM_PFNMAP, but isn't marked as such"));
	goto error;
    }

exit:
    PVR_ASSERT(bMMapSemHeld);
    up_read(&current->mm->mmap_sem);

    /* Return the cookie */
    *phOSWrapMem = (IMG_HANDLE)psInfo;

    if (bHaveNoPageStructs)
    {
        PVR_DPF((PVR_DBG_MESSAGE,
            "OSAcquirePhysPageAddr: Region contains pages which can't be locked down (no page structures)"));
    }

    PVR_ASSERT(psInfo->eType != 0);

    return PVRSRV_OK;

error:
    if (bMMapSemHeld)
    {
        up_read(&current->mm->mmap_sem);
    }
    OSReleasePhysPageAddr((IMG_HANDLE)psInfo);

    PVR_ASSERT(eError != PVRSRV_OK);

    return eError;
}

typedef void (*InnerCacheOp_t)(const void *pvStart, const void *pvEnd);

#if defined(__arm__) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,39))
typedef void (*OuterCacheOp_t)(phys_addr_t uStart, phys_addr_t uEnd);
#else
typedef void (*OuterCacheOp_t)(unsigned long ulStart, unsigned long ulEnd);
#endif

#if defined(CONFIG_OUTER_CACHE)

typedef IMG_BOOL (*MemAreaToPhys_t)(LinuxMemArea *psLinuxMemArea,
										 IMG_VOID *pvRangeAddrStart,
										 IMG_UINT32 ui32PageNumOffset,
										 IMG_UINT32 ui32PageNum,
										 unsigned long *pulStart);

static IMG_BOOL VMallocAreaToPhys(LinuxMemArea *psLinuxMemArea,
								  IMG_VOID *pvRangeAddrStart,
								  IMG_UINT32 ui32PageNumOffset,
								  IMG_UINT32 ui32PageNum,
								  unsigned long *pulStart)
{
	*pulStart = vmalloc_to_pfn(pvRangeAddrStart + ui32PageNum * PAGE_SIZE) << PAGE_SHIFT;
	return IMG_TRUE;
}

static IMG_BOOL ExternalKVAreaToPhys(LinuxMemArea *psLinuxMemArea,
									 IMG_VOID *pvRangeAddrStart,
									 IMG_UINT32 ui32PageNumOffset,
									 IMG_UINT32 ui32PageNum,
									 unsigned long *pulStart)
{
	IMG_SYS_PHYADDR SysPAddr;
	IMG_CPU_PHYADDR CpuPAddr;
	SysPAddr = psLinuxMemArea->uData.sExternalKV.uPhysAddr.pSysPhysAddr[ui32PageNumOffset + ui32PageNum];
	CpuPAddr = SysSysPAddrToCpuPAddr(SysPAddr);
	*pulStart = CpuPAddr.uiAddr;
	return IMG_TRUE;
}

static IMG_BOOL AllocPagesAreaToPhys(LinuxMemArea *psLinuxMemArea,
									 IMG_VOID *pvRangeAddrStart,
									 IMG_UINT32 ui32PageNumOffset,
									 IMG_UINT32 ui32PageNum,
									 unsigned long *pulStart)
{
	struct page *pPage;

	pPage = psLinuxMemArea->uData.sPageList.ppsPageList[ui32PageNumOffset + ui32PageNum];
	*pulStart = page_to_pfn(pPage) << PAGE_SHIFT;
	return IMG_TRUE;
}

static IMG_BOOL AllocPagesSparseAreaToPhys(LinuxMemArea *psLinuxMemArea,
										   IMG_VOID *pvRangeAddrStart,
										   IMG_UINT32 ui32PageNumOffset,
										   IMG_UINT32 ui32PageNum,
										   unsigned long *pulStart)
{
	IMG_UINT32 ui32VirtOffset = (ui32PageNumOffset + ui32PageNum) << PAGE_SHIFT;
	IMG_UINT32 ui32PhysOffset;
	struct page *pPage;

	if (BM_VirtOffsetToPhysical(psLinuxMemArea->hBMHandle, ui32VirtOffset, &ui32PhysOffset))
	{
		PVR_ASSERT(ui32PhysOffset <= ui32VirtOffset);
		pPage = psLinuxMemArea->uData.sPageList.ppsPageList[ui32PhysOffset >> PAGE_SHIFT];
		*pulStart = page_to_pfn(pPage) << PAGE_SHIFT;
		return IMG_TRUE;
	}

	return IMG_FALSE;
}


static IMG_BOOL IONAreaToPhys(LinuxMemArea *psLinuxMemArea,
							  IMG_VOID *pvRangeAddrStart,
							  IMG_UINT32 ui32PageNumOffset,
							  IMG_UINT32 ui32PageNum,
							  unsigned long *pulStart)
{
	IMG_CPU_PHYADDR CpuPAddr;
	CpuPAddr = psLinuxMemArea->uData.sIONTilerAlloc.pCPUPhysAddrs[ui32PageNumOffset + ui32PageNum];
	*pulStart = CpuPAddr.uiAddr;
	return IMG_TRUE;
}

#endif /* defined(CONFIG_OUTER_CACHE) */

/* g_sMMapMutex must be held while this function is called */

static
IMG_VOID *FindMMapBaseVAddr(struct list_head *psMMapOffsetStructList,
							IMG_VOID *pvRangeAddrStart, IMG_UINT32 ui32Length)
{
	PKV_OFFSET_STRUCT psOffsetStruct;
	IMG_VOID *pvMinVAddr;

	/* There's no kernel-virtual for this type of allocation, so if
	 * we're flushing it, it must be user-virtual, and therefore
	 * have a mapping.
	 */
	list_for_each_entry(psOffsetStruct, psMMapOffsetStructList, sAreaItem)
	{
		if(OSGetCurrentProcessIDKM() != psOffsetStruct->ui32PID)
			continue;

		pvMinVAddr = (IMG_VOID *)psOffsetStruct->ui32UserVAddr;

		/* Within permissible range */
		if(pvRangeAddrStart >= pvMinVAddr &&
		   ui32Length <= psOffsetStruct->ui32RealByteSize)
			return pvMinVAddr;
	}

	return IMG_NULL;
}

extern PVRSRV_LINUX_MUTEX g_sMMapMutex;

static inline void DoInnerCacheOp(IMG_HANDLE hOSMemHandle,
								  IMG_UINT32 ui32ByteOffset,
								  IMG_VOID *pvRangeAddrStart,
								  IMG_UINT32 ui32Length,
								  InnerCacheOp_t pfnInnerCacheOp)
{
	LinuxMemArea *psLinuxMemArea = hOSMemHandle;

	if (!psLinuxMemArea->hBMHandle)
	{
		pfnInnerCacheOp(pvRangeAddrStart, pvRangeAddrStart + ui32Length);
	}
	else
	{
		IMG_UINT32 ui32ByteRemain = ui32Length;
		IMG_UINT32 ui32BytesToDo = PAGE_SIZE - (((IMG_UINT32) pvRangeAddrStart) & (~PAGE_MASK));
		IMG_UINT8 *pbDo = (IMG_UINT8 *) pvRangeAddrStart;
	
		while(ui32ByteRemain)
		{
			if (BM_MapPageAtOffset(psLinuxMemArea->hBMHandle, ui32ByteOffset + (ui32Length - ui32ByteRemain)))
			{
				pfnInnerCacheOp(pbDo, pbDo + ui32BytesToDo);
			}
			pbDo += ui32BytesToDo;
			ui32ByteRemain -= ui32BytesToDo;
			ui32BytesToDo = MIN(ui32ByteRemain, PAGE_SIZE);
		}
	}
}

static
IMG_BOOL CheckExecuteCacheOp(IMG_HANDLE hOSMemHandle,
							 IMG_UINT32 ui32ByteOffset,
							 IMG_VOID *pvRangeAddrStart,
							 IMG_UINT32 ui32Length,
							 InnerCacheOp_t pfnInnerCacheOp,
							 OuterCacheOp_t pfnOuterCacheOp)
{
	LinuxMemArea *psLinuxMemArea = (LinuxMemArea *)hOSMemHandle;
	IMG_UINT32 ui32AreaLength, ui32AreaOffset = 0;
	struct list_head *psMMapOffsetStructList;
	IMG_VOID *pvMinVAddr;

#if defined(CONFIG_OUTER_CACHE)
	MemAreaToPhys_t pfnMemAreaToPhys = IMG_NULL;
	IMG_UINT32 ui32PageNumOffset = 0;
#endif

	PVR_ASSERT(psLinuxMemArea != IMG_NULL);

	LinuxLockMutex(&g_sMMapMutex);

	psMMapOffsetStructList = &psLinuxMemArea->sMMapOffsetStructList;
	ui32AreaLength = psLinuxMemArea->ui32ByteSize;

	/*
		Don't check the length in the case of sparse mappings as
		we only know the physical length not the virtual
	*/
	if (!psLinuxMemArea->hBMHandle)
	{
		PVR_ASSERT(ui32Length <= ui32AreaLength);
	}

	if(psLinuxMemArea->eAreaType == LINUX_MEM_AREA_SUB_ALLOC)
	{
		ui32AreaOffset = psLinuxMemArea->uData.sSubAlloc.ui32ByteOffset;
		psLinuxMemArea = psLinuxMemArea->uData.sSubAlloc.psParentLinuxMemArea;
	}

	/* Recursion surely isn't possible? */
	PVR_ASSERT(psLinuxMemArea->eAreaType != LINUX_MEM_AREA_SUB_ALLOC);

	switch(psLinuxMemArea->eAreaType)
	{
		case LINUX_MEM_AREA_VMALLOC:
		{
			if(is_vmalloc_addr(pvRangeAddrStart))
			{
				pvMinVAddr = psLinuxMemArea->uData.sVmalloc.pvVmallocAddress + ui32AreaOffset;

				/* Outside permissible range */
				if(pvRangeAddrStart < pvMinVAddr)
					goto err_blocked;

				DoInnerCacheOp(hOSMemHandle,
							   ui32ByteOffset,
							   pvRangeAddrStart,
							   ui32Length,
							   pfnInnerCacheOp);
			}
			else
			{
				/* If this isn't a vmalloc address, assume we're flushing by
				 * user-virtual. Compute the mmap base vaddr and use this to
				 * compute the offset in vmalloc space.
				 */

				pvMinVAddr = FindMMapBaseVAddr(psMMapOffsetStructList,
											   pvRangeAddrStart, ui32Length);
				if(!pvMinVAddr)
					goto err_blocked;

				DoInnerCacheOp(hOSMemHandle,
							   ui32ByteOffset,
							   pvRangeAddrStart,
							   ui32Length,
							   pfnInnerCacheOp);

#if defined(CONFIG_OUTER_CACHE)
				/*
				 * We don't need to worry about cache aliasing here because
				 * we have already flushed the virtually-indexed caches (L1
				 * etc.) by the supplied user-virtual addresses.
				 *
				 * The vmalloc address will only be used to determine
				 * affected physical pages for outer cache flushing.
				 */
				pvRangeAddrStart = psLinuxMemArea->uData.sVmalloc.pvVmallocAddress +
								   (ui32AreaOffset & PAGE_MASK) + (pvRangeAddrStart - pvMinVAddr);
			}

			pfnMemAreaToPhys = VMallocAreaToPhys;
#else /* defined(CONFIG_OUTER_CACHE) */
			}
#endif /* defined(CONFIG_OUTER_CACHE) */
			break;
		}

		case LINUX_MEM_AREA_EXTERNAL_KV:
		{
			/* We'll only see bPhysContig for frame buffers, and we shouldn't
			 * be flushing those (they're write combined or uncached).
			 */
			if (psLinuxMemArea->uData.sExternalKV.bPhysContig == IMG_TRUE)
			{
				PVR_DPF((PVR_DBG_WARNING, "%s: Attempt to flush contiguous external memory", __func__));
				goto err_blocked;
			}

			/* If it has a kernel virtual address, something odd has happened.
			 * We expect EXTERNAL_KV _only_ from the wrapping of ALLOC_PAGES.
			 */
			if (psLinuxMemArea->uData.sExternalKV.pvExternalKV != IMG_NULL)
			{
				PVR_DPF((PVR_DBG_WARNING, "%s: Attempt to flush external memory with a kernel virtual address", __func__));
				goto err_blocked;
			}

			pvMinVAddr = FindMMapBaseVAddr(psMMapOffsetStructList,
										   pvRangeAddrStart, ui32Length);
			if(!pvMinVAddr)
				goto err_blocked;

			DoInnerCacheOp(hOSMemHandle,
						   ui32ByteOffset,
						   pvRangeAddrStart,
						   ui32Length,
						   pfnInnerCacheOp);

#if defined(CONFIG_OUTER_CACHE)
			ui32PageNumOffset = ((ui32AreaOffset & PAGE_MASK) + (pvRangeAddrStart - pvMinVAddr)) >> PAGE_SHIFT;
			pfnMemAreaToPhys = ExternalKVAreaToPhys;
#endif
			break;
		}

		case LINUX_MEM_AREA_ION:
		{
			pvMinVAddr = FindMMapBaseVAddr(psMMapOffsetStructList,
										   pvRangeAddrStart, ui32Length);
			if(!pvMinVAddr)
				goto err_blocked;

			DoInnerCacheOp(hOSMemHandle,
						   ui32ByteOffset,
						   pvRangeAddrStart,
						   ui32Length,
						   pfnInnerCacheOp);

#if defined(CONFIG_OUTER_CACHE)
			ui32PageNumOffset = ((ui32AreaOffset & PAGE_MASK) + (pvRangeAddrStart - pvMinVAddr)) >> PAGE_SHIFT;
			pfnMemAreaToPhys = IONAreaToPhys;
#endif
			break;
		}

		case LINUX_MEM_AREA_ALLOC_PAGES:
		{
			pvMinVAddr = FindMMapBaseVAddr(psMMapOffsetStructList,
										   pvRangeAddrStart, ui32Length);
			if(!pvMinVAddr)
				goto err_blocked;

			DoInnerCacheOp(hOSMemHandle,
						   ui32ByteOffset,
						   pvRangeAddrStart,
						   ui32Length,
						   pfnInnerCacheOp);

#if defined(CONFIG_OUTER_CACHE)
			ui32PageNumOffset = ((ui32AreaOffset & PAGE_MASK) + (pvRangeAddrStart - pvMinVAddr)) >> PAGE_SHIFT;
			if (psLinuxMemArea->hBMHandle)
			{
				pfnMemAreaToPhys = AllocPagesSparseAreaToPhys;
			}
			else
			{
				pfnMemAreaToPhys = AllocPagesAreaToPhys;
			}
#endif
			break;
		}

		default:
			PVR_DBG_BREAK;
	}

	LinuxUnLockMutex(&g_sMMapMutex);

#if defined(CONFIG_OUTER_CACHE)
	PVR_ASSERT(pfnMemAreaToPhys != IMG_NULL);

	/* Outer caches need some more work, to get a list of physical addresses */
	{
		unsigned long ulStart, ulEnd, ulLength, ulStartOffset, ulEndOffset;
		IMG_UINT32 i, ui32NumPages;
		IMG_BOOL bValidPage;

		/* Length and offsets of flush region WRT page alignment */
		ulLength = (unsigned long)ui32Length;
		ulStartOffset = ((unsigned long)pvRangeAddrStart) & (PAGE_SIZE - 1);
		ulEndOffset = ((unsigned long)pvRangeAddrStart + ulLength) & (PAGE_SIZE - 1);

		/* The affected pages, rounded up */
		ui32NumPages = (ulStartOffset + ulLength + PAGE_SIZE - 1) >> PAGE_SHIFT;

		for(i = 0; i < ui32NumPages; i++)
		{
			bValidPage = pfnMemAreaToPhys(psLinuxMemArea, pvRangeAddrStart,
									   ui32PageNumOffset, i, &ulStart);
			if (bValidPage)
			{
				ulEnd = ulStart + PAGE_SIZE;
	
				if(i == ui32NumPages - 1 && ulEndOffset != 0)
					ulEnd = ulStart + ulEndOffset;
	
				if(i == 0)
					ulStart += ulStartOffset;
	
				pfnOuterCacheOp(ulStart, ulEnd);
			}
		}
	}
#endif

	return IMG_TRUE;

err_blocked:
	PVR_DPF((PVR_DBG_WARNING, "%s: Blocked cache op on virtual range "
							  "%p-%p (type %d)", __func__,
			 pvRangeAddrStart, pvRangeAddrStart + ui32Length,
			 psLinuxMemArea->eAreaType));
	LinuxUnLockMutex(&g_sMMapMutex);
	return IMG_FALSE;
}

#if defined(__i386__)

#define ROUND_UP(x,a) (((x) + (a) - 1) & ~((a) - 1))

static void per_cpu_cache_flush(void *arg)
{
    PVR_UNREFERENCED_PARAMETER(arg);
    wbinvd();
}

static void x86_flush_cache_range(const void *pvStart, const void *pvEnd)
{
	IMG_BYTE *pbStart = (IMG_BYTE *)pvStart;
	IMG_BYTE *pbEnd = (IMG_BYTE *)pvEnd;
	IMG_BYTE *pbBase;

	pbEnd = (IMG_BYTE *)ROUND_UP((IMG_UINTPTR_T)pbEnd,
								 boot_cpu_data.x86_clflush_size);

	mb();
	for(pbBase = pbStart; pbBase < pbEnd; pbBase += boot_cpu_data.x86_clflush_size)
	{
		clflush(pbBase);
	}
	mb();
}

IMG_VOID OSCleanCPUCacheKM(IMG_VOID)
{
	/* No clean feature on x86 */
	ON_EACH_CPU(per_cpu_cache_flush, NULL, 1);
}

IMG_VOID OSFlushCPUCacheKM(IMG_VOID)
{
	ON_EACH_CPU(per_cpu_cache_flush, NULL, 1);
}

IMG_BOOL OSFlushCPUCacheRangeKM(IMG_HANDLE hOSMemHandle,
								IMG_UINT32 ui32ByteOffset,
								IMG_VOID *pvRangeAddrStart,
								IMG_UINT32 ui32Length)
{
	/* Write-back and invalidate */
	return CheckExecuteCacheOp(hOSMemHandle, ui32ByteOffset, pvRangeAddrStart, ui32Length,
							   x86_flush_cache_range, IMG_NULL);
}

IMG_BOOL OSCleanCPUCacheRangeKM(IMG_HANDLE hOSMemHandle,
								IMG_UINT32 ui32ByteOffset,
								IMG_VOID *pvRangeAddrStart,
								IMG_UINT32 ui32Length)
{
	/* No clean feature on x86 */
	return CheckExecuteCacheOp(hOSMemHandle, ui32ByteOffset, pvRangeAddrStart, ui32Length,
							   x86_flush_cache_range, IMG_NULL);
}

IMG_BOOL OSInvalidateCPUCacheRangeKM(IMG_HANDLE hOSMemHandle,
									 IMG_UINT32 ui32ByteOffset,
									 IMG_VOID *pvRangeAddrStart,
									 IMG_UINT32 ui32Length)
{
	/* No invalidate-only support */
	return CheckExecuteCacheOp(hOSMemHandle, ui32ByteOffset, pvRangeAddrStart, ui32Length,
							   x86_flush_cache_range, IMG_NULL);
}

#else /* defined(__i386__) */

#if defined(__arm__)

static void per_cpu_cache_flush(void *arg)
{
	PVR_UNREFERENCED_PARAMETER(arg);
	flush_cache_all();
}

IMG_VOID OSCleanCPUCacheKM(IMG_VOID)
{
	/* No full (inner) cache clean op */
	ON_EACH_CPU(per_cpu_cache_flush, NULL, 1);
#if defined(CONFIG_OUTER_CACHE)
	outer_clean_range(0, ULONG_MAX);
#endif
}

IMG_VOID OSFlushCPUCacheKM(IMG_VOID)
{
	ON_EACH_CPU(per_cpu_cache_flush, NULL, 1);
#if defined(CONFIG_OUTER_CACHE) && \
	(LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
	/* To use the "deferred flush" (not clean) DDK feature you need a kernel
	 * implementation of outer_flush_all() for ARM CPUs with an outer cache
	 * controller (e.g. PL310, common with Cortex A9 and later).
	 *
	 * Reference DDKs don't require this functionality, as they will only
	 * clean the cache, never flush (clean+invalidate) it.
	 */
	outer_flush_all();
#endif
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34))
static inline size_t pvr_dmac_range_len(const void *pvStart, const void *pvEnd)
{
	return (size_t)((char *)pvEnd - (char *)pvStart);
}
#endif

static void pvr_dmac_inv_range(const void *pvStart, const void *pvEnd)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,34))
	dmac_inv_range(pvStart, pvEnd);
#else
	dmac_map_area(pvStart, pvr_dmac_range_len(pvStart, pvEnd), DMA_FROM_DEVICE);
#endif
}

static void pvr_dmac_clean_range(const void *pvStart, const void *pvEnd)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,34))
	dmac_clean_range(pvStart, pvEnd);
#else
	dmac_map_area(pvStart, pvr_dmac_range_len(pvStart, pvEnd), DMA_TO_DEVICE);
#endif
}

IMG_BOOL OSFlushCPUCacheRangeKM(IMG_HANDLE hOSMemHandle,
								IMG_UINT32 ui32ByteOffset,
								IMG_VOID *pvRangeAddrStart,
								IMG_UINT32 ui32Length)
{
	return CheckExecuteCacheOp(hOSMemHandle, ui32ByteOffset,
							   pvRangeAddrStart, ui32Length,
							   dmac_flush_range, outer_flush_range);
}

IMG_BOOL OSCleanCPUCacheRangeKM(IMG_HANDLE hOSMemHandle,
								IMG_UINT32 ui32ByteOffset,
								IMG_VOID *pvRangeAddrStart,
								IMG_UINT32 ui32Length)
{
	return CheckExecuteCacheOp(hOSMemHandle, ui32ByteOffset,
							   pvRangeAddrStart, ui32Length,
							   pvr_dmac_clean_range, outer_clean_range);
}

IMG_BOOL OSInvalidateCPUCacheRangeKM(IMG_HANDLE hOSMemHandle,
									 IMG_UINT32 ui32ByteOffset,
									 IMG_VOID *pvRangeAddrStart,
									 IMG_UINT32 ui32Length)
{
	return CheckExecuteCacheOp(hOSMemHandle, ui32ByteOffset,
							   pvRangeAddrStart, ui32Length,
							   pvr_dmac_inv_range, outer_inv_range);
}

#else /* defined(__arm__) */

#if defined(__mips__)
/* 
 * dmac cache functions are supposed to be used for dma 
 * memory which comes from dma-able memory. However examining
 * the implementation of dmac cache functions and experimenting,
 * can assert that dmac functions are safe to use for high-mem
 * memory as well for our OS{Clean/Flush/Invalidate}Cache functions
 * 
 */

IMG_VOID OSCleanCPUCacheKM(IMG_VOID)
{
	/* dmac functions flush full cache if size is larger than
	 * p-cache size. This is a workaround for the fact that
	 * __flush_cache_all is not an exported symbol. Please
	 * replace with custom function if available in latest
	 * version of linux being used.
	 * Arbitrary large number (1MB) which should be larger than 
	 * mips p-cache sizes for some time in future.
	 * */
	dma_cache_wback(0, 0x100000);
}

IMG_VOID OSFlushCPUCacheKM(IMG_VOID)
{
	/* dmac functions flush full cache if size is larger than
	 * p-cache size. This is a workaround for the fact that
	 * __flush_cache_all is not an exported symbol. Please
	 * replace with custom function if available in latest
	 * version of linux being used.
	 * Arbitrary large number (1MB) which should be larger than 
	 * mips p-cache sizes for some time in future.
	 * */
	dma_cache_wback_inv(0, 0x100000);
}

static inline IMG_UINT32 pvr_dma_range_len(const void *pvStart, const void *pvEnd)
{
	return (IMG_UINT32)((char *)pvEnd - (char *)pvStart);
}

static void pvr_dma_cache_wback_inv(const void *pvStart, const void *pvEnd)
{
	dma_cache_wback_inv((IMG_UINTPTR_T)pvStart, pvr_dma_range_len(pvStart, pvEnd));	
}

static void pvr_dma_cache_wback(const void *pvStart, const void *pvEnd)
{
	dma_cache_wback((IMG_UINTPTR_T)pvStart, pvr_dma_range_len(pvStart, pvEnd));
}

static void pvr_dma_cache_inv(const void *pvStart, const void *pvEnd)
{
	dma_cache_inv((IMG_UINTPTR_T)pvStart, pvr_dma_range_len(pvStart, pvEnd));
}

IMG_BOOL OSFlushCPUCacheRangeKM(IMG_HANDLE hOSMemHandle,
								IMG_UINT32 ui32ByteOffset,
								IMG_VOID *pvRangeAddrStart,
								IMG_UINT32 ui32Length)
{
	return CheckExecuteCacheOp(hOSMemHandle, ui32ByteOffset,
							   pvRangeAddrStart, ui32Length,
							   pvr_dma_cache_wback_inv, IMG_NULL);
}

IMG_BOOL OSCleanCPUCacheRangeKM(IMG_HANDLE hOSMemHandle,
								IMG_UINT32 ui32ByteOffset,
								IMG_VOID *pvRangeAddrStart,
								IMG_UINT32 ui32Length)
{
	return CheckExecuteCacheOp(hOSMemHandle, ui32ByteOffset,
							   pvRangeAddrStart, ui32Length,
							   pvr_dma_cache_wback, IMG_NULL);
}

IMG_BOOL OSInvalidateCPUCacheRangeKM(IMG_HANDLE hOSMemHandle,
									 IMG_UINT32 ui32ByteOffset,
									 IMG_VOID *pvRangeAddrStart,
									 IMG_UINT32 ui32Length)
{
	return CheckExecuteCacheOp(hOSMemHandle, ui32ByteOffset,
							   pvRangeAddrStart, ui32Length,
							   pvr_dma_cache_inv, IMG_NULL);
}

#else /* defined(__mips__) */

#error "Implement CPU cache flush/clean/invalidate primitives for this CPU!"

#endif /* defined(__mips__) */

#endif /* defined(__arm__) */

#endif /* defined(__i386__) */

typedef struct _AtomicStruct
{
	atomic_t RefCount;
} AtomicStruct;

PVRSRV_ERROR OSAtomicAlloc(IMG_PVOID *ppvRefCount)
{
	AtomicStruct *psRefCount;

	psRefCount = kmalloc(sizeof(AtomicStruct), GFP_KERNEL);
	if (psRefCount == NULL)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	atomic_set(&psRefCount->RefCount, 0);
	
	*ppvRefCount = psRefCount;
	return PVRSRV_OK;
}

IMG_VOID OSAtomicFree(IMG_PVOID pvRefCount)
{
	AtomicStruct *psRefCount = pvRefCount;

	PVR_ASSERT(atomic_read(&psRefCount->RefCount) == 0);
	kfree(psRefCount);
}

IMG_VOID OSAtomicInc(IMG_PVOID pvRefCount)
{
	AtomicStruct *psRefCount = pvRefCount;

	atomic_inc(&psRefCount->RefCount);
}

IMG_BOOL OSAtomicDecAndTest(IMG_PVOID pvRefCount)
{
	AtomicStruct *psRefCount = pvRefCount;

	return atomic_dec_and_test(&psRefCount->RefCount) ? IMG_TRUE:IMG_FALSE;
}

IMG_UINT32 OSAtomicRead(IMG_PVOID pvRefCount)
{
	AtomicStruct *psRefCount = pvRefCount;

	return (IMG_UINT32) atomic_read(&psRefCount->RefCount);
}

IMG_VOID OSReleaseBridgeLock(IMG_VOID)
{
       LinuxUnLockMutex(&gPVRSRVLock);
}

IMG_VOID OSReacquireBridgeLock(IMG_VOID)
{
       LinuxLockMutex(&gPVRSRVLock);
}

typedef struct _OSTime
{
	unsigned long ulTime;
} OSTime;

PVRSRV_ERROR OSTimeCreateWithUSOffset(IMG_PVOID *pvRet, IMG_UINT32 ui32USOffset)
{
	OSTime *psOSTime;

	psOSTime = kmalloc(sizeof(OSTime), GFP_KERNEL);
	if (psOSTime == IMG_NULL)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	psOSTime->ulTime = usecs_to_jiffies(jiffies_to_usecs(jiffies) + ui32USOffset);
	*pvRet = psOSTime;
	return PVRSRV_OK;
}


IMG_BOOL OSTimeHasTimePassed(IMG_PVOID pvData)
{
	OSTime *psOSTime = pvData;

	if (time_is_before_jiffies(psOSTime->ulTime))
	{
		return IMG_TRUE;
	}
	return IMG_FALSE;
}

IMG_VOID OSTimeDestroy(IMG_PVOID pvData)
{
	kfree(pvData);
}

IMG_VOID OSGetCurrentProcessNameKM(IMG_CHAR *pszName, IMG_UINT32 ui32Size)
{
	strncpy(pszName, current->comm, MIN(ui32Size,TASK_COMM_LEN));
}

/* One time osfunc initialisation */
PVRSRV_ERROR PVROSFuncInit(IMG_VOID)
{
#if defined(PVR_LINUX_TIMERS_USING_WORKQUEUES)
    {
        psTimerWorkQueue = create_workqueue("pvr_timer");
        if (psTimerWorkQueue == NULL)
        {
	    PVR_DPF((PVR_DBG_ERROR, "%s: couldn't create timer workqueue", __FUNCTION__));		
	    return PVRSRV_ERROR_UNABLE_TO_CREATE_THREAD;

        }
    }
#endif

#if defined(PVR_LINUX_TIMERS_USING_WORKQUEUES) || defined(PVR_LINUX_TIMERS_USING_SHARED_WORKQUEUE)
    {
	IMG_UINT32 ui32i;

        for (ui32i = 0; ui32i < OS_MAX_TIMERS; ui32i++)
        {
            TIMER_CALLBACK_DATA *psTimerCBData = &sTimers[ui32i];

	    INIT_WORK(&psTimerCBData->sWork, OSTimerWorkQueueCallBack);
        }
    }
#endif

#if defined (SUPPORT_ION)
	{
		PVRSRV_ERROR eError;

		eError = IonInit();
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: IonInit failed", __FUNCTION__));
		}
	}
#endif
    return PVRSRV_OK;
}

/*
 * Osfunc deinitialisation.
 * Note that PVROSFuncInit may not have been called
 */
IMG_VOID PVROSFuncDeInit(IMG_VOID)
{
#if defined (SUPPORT_ION)
	IonDeinit();
#endif
#if defined(PVR_LINUX_TIMERS_USING_WORKQUEUES)
    if (psTimerWorkQueue != NULL)
    {
	destroy_workqueue(psTimerWorkQueue);
    }
#endif
}
