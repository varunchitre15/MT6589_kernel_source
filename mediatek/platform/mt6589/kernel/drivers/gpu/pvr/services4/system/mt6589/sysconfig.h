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

#if !defined(__SOCCONFIG_H__)
#define __SOCCONFIG_H__

/*#if defined(CONFIG_ARCH_MT6577)
#define VS_PRODUCT_NAME	"MT6577"
#else
#define VS_PRODUCT_NAME	"MT6575"
#endif*/

#define VS_PRODUCT_NAME "MT6589"

#define SYS_SGX_CLOCK_SPEED     286000000

#define SYS_SGX_HWRECOVERY_TIMEOUT_FREQ		(100)	
#define SYS_SGX_PDS_TIMER_FREQ				(1000)	

#if !defined(SYS_SGX_ACTIVE_POWER_LATENCY_MS)
#define SYS_SGX_ACTIVE_POWER_LATENCY_MS		(2)
#endif

#define SYS_MTK_SGX_REGS_SYS_PHYS_BASE  0x13000000 // MFG_AXI_BASE

#define SYS_MTK_SGX_REGS_SIZE           0xFFFF

#define SYS_MTK_SGX_IRQ				 220 //(188+32) // MT6589_MFG_IRQ_ID

#define DEVICE_SGX_INTERRUPT		(1<<0)

#if defined(__linux__)
#if defined(PVR_LDM_PLATFORM_PRE_REGISTERED_DEV)
#define	SYS_SGX_DEV_NAME	PVR_LDM_PLATFORM_PRE_REGISTERED_DEV
#else
#define	SYS_SGX_DEV_NAME	"mt6589_gpu"
#endif	
#endif	

 
#endif	
