/**********************************************************************
 *
 * Copyright (C) Imagination Technologies Ltd. All rights reserved.
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

#if !defined(__SYSLOCAL_H__)
#define __SYSLOCAL_H__

#if defined(__linux__)

#include <linux/version.h>
#include <linux/clk.h>
#if defined(PVR_LINUX_USING_WORKQUEUES)
#include <linux/mutex.h>
#else
#include <linux/spinlock.h>
#endif
#include <asm/atomic.h>

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,26))
#include <linux/semaphore.h>
#include <linux/resource.h>
#else 
#include <asm/semaphore.h>
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,22))
#include <asm/arch/resource.h>
#endif 
#endif 


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35))
#if !defined(LDM_PLATFORM)
#error "LDM_PLATFORM must be set"
#endif
//#define	PVR_LINUX_DYNAMIC_SGX_RESOURCE_INFO
//#include <linux/platform_device.h>
#endif

//#if ((defined(DEBUG) || defined(TIMING)) &&
//    (LINUX_VERSION_CODE == KERNEL_VERSION(2,6,34))) &&
//    !defined(PVR_NO_OMAP_TIMER)
//#define	PVR_OMAP4_TIMING_PRCM
//#endif

//#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35))
//#include <plat/gpu.h>
//#if !defined(PVR_NO_OMAP_TIMER)
//#define	PVR_OMAP_USE_DM_TIMER_API
//#include <plat/dmtimer.h>
//#endif
//#endif

//#if !defined(PVR_NO_OMAP_TIMER)
//#define PVR_OMAP_TIMER_BASE_IN_SYS_SPEC_DATA
//#endif
#endif 

//#if !defined(NO_HARDWARE) &&
//     defined(SYS_USING_INTERRUPTS)
//#define SGX_OCP_REGS_ENABLED
//#endif

//#if defined(__linux__)
//#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35)) && defined(SGX_OCP_REGS_ENABLED)
//#if !defined(SGX544)
//#define SGX_OCP_NO_INT_BYPASS
//#endif
//#endif
//#endif

#if defined (__cplusplus)
extern "C" {
#endif

 
 
IMG_VOID DisableSystemClocks(SYS_DATA *psSysData);
PVRSRV_ERROR EnableSystemClocks(SYS_DATA *psSysData);

IMG_VOID DisableSGXClocks(SYS_DATA *psSysData);
PVRSRV_ERROR EnableSGXClocks(SYS_DATA *psSysData);

#define SYS_SPECIFIC_DATA_ENABLE_SYSCLOCKS	0x00000001
#define SYS_SPECIFIC_DATA_ENABLE_LISR		0x00000002
#define SYS_SPECIFIC_DATA_ENABLE_MISR		0x00000004
#define SYS_SPECIFIC_DATA_ENABLE_ENVDATA	0x00000008
#define SYS_SPECIFIC_DATA_ENABLE_LOCDEV		0x00000010
#define SYS_SPECIFIC_DATA_ENABLE_REGDEV		0x00000020
#define SYS_SPECIFIC_DATA_ENABLE_PDUMPINIT	0x00000040
#define SYS_SPECIFIC_DATA_ENABLE_INITDEV	0x00000080
#define SYS_SPECIFIC_DATA_ENABLE_LOCATEDEV	0x00000100

#define	SYS_SPECIFIC_DATA_PM_UNINSTALL_LISR	0x00000200
#define	SYS_SPECIFIC_DATA_PM_DISABLE_SYSCLOCKS	0x00000400
#define SYS_SPECIFIC_DATA_ENABLE_OCPREGS	0x00000800
#define SYS_SPECIFIC_DATA_ENABLE_PM_RUNTIME	0x00001000
#define SYS_SPECIFIC_DATA_IRQ_ENABLED		0x00002000

#define	SYS_SPECIFIC_DATA_SET(psSysSpecData, flag) ((IMG_VOID)((psSysSpecData)->ui32SysSpecificData |= (flag)))

#define	SYS_SPECIFIC_DATA_CLEAR(psSysSpecData, flag) ((IMG_VOID)((psSysSpecData)->ui32SysSpecificData &= ~(flag)))

#define	SYS_SPECIFIC_DATA_TEST(psSysSpecData, flag) (((psSysSpecData)->ui32SysSpecificData & (flag)) != 0)
 
typedef struct _SYS_SPECIFIC_DATA_TAG_
{
	IMG_UINT32	ui32SysSpecificData;
	PVRSRV_DEVICE_NODE *psSGXDevNode;
	IMG_BOOL	bSGXInitComplete;
//#if defined(PVR_OMAP_TIMER_BASE_IN_SYS_SPEC_DATA)
//	IMG_CPU_PHYADDR	sTimerRegPhysBase;
//#endif
#if !defined(__linux__)
	IMG_BOOL	bSGXClocksEnabled;
#endif
	IMG_UINT32	ui32SrcClockDiv;
#if defined(__linux__)
	IMG_BOOL	bSysClocksOneTimeInit;
	atomic_t	sSGXClocksEnabled;
#if defined(PVR_LINUX_USING_WORKQUEUES)
	struct mutex	sPowerLock;
#else
	IMG_BOOL	bConstraintNotificationsEnabled;
	spinlock_t	sPowerLock;
	atomic_t	sPowerLockCPU;
	spinlock_t	sNotifyLock;
	atomic_t	sNotifyLockCPU;
	IMG_BOOL	bCallVDD2PostFunc;
#endif
#if defined(DEBUG) || defined(TIMING)
	struct clk	*psGPT11_FCK;
	struct clk	*psGPT11_ICK;
#endif
//#if defined(PVR_OMAP_USE_DM_TIMER_API)
//	struct omap_dm_timer *psGPTimer;
//#endif
#endif	
} SYS_SPECIFIC_DATA;

extern SYS_SPECIFIC_DATA *gpsSysSpecificData;

#if defined(SGX_OCP_REGS_ENABLED) && defined(SGX_OCP_NO_INT_BYPASS)
IMG_VOID SysEnableSGXInterrupts(SYS_DATA* psSysData);
IMG_VOID SysDisableSGXInterrupts(SYS_DATA* psSysData);
#else
#define	SysEnableSGXInterrupts(psSysData)
#define SysDisableSGXInterrupts(psSysData)
#endif

#if defined(SYS_CUSTOM_POWERLOCK_WRAP)
IMG_BOOL WrapSystemPowerChange(SYS_SPECIFIC_DATA *psSysSpecData);
IMG_VOID UnwrapSystemPowerChange(SYS_SPECIFIC_DATA *psSysSpecData);
#endif

#if defined(__linux__)

PVRSRV_ERROR SysPMRuntimeRegister(void);
PVRSRV_ERROR SysPMRuntimeUnregister(void);

#else 

#ifdef INLINE_IS_PRAGMA
#pragma inline(SysPMRuntimeRegister)
#endif
static INLINE PVRSRV_ERROR SysPMRuntimeRegister(void)
{
	return PVRSRV_OK;
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(SysPMRuntimeUnregister)
#endif
static INLINE PVRSRV_ERROR SysPMRuntimeUnregister(void)
{
	return PVRSRV_OK;
}

#endif 

/*
#if defined(MTK_USE_GDC)
IMG_UINT32 SysCacheBypass(IMG_UINT32 ui32RegVal);
IMG_VOID OnSGXResetDone(void);
#endif
*/

#if defined(__cplusplus)
}
#endif

#endif	


