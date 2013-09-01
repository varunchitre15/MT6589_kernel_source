/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 * 
 * MediaTek Inc. (C) 2010. All rights reserved.
 * 
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

/*
 * Contains declarations of types and constants used by malloc leak
 * detection code in both, libc and libc_malloc_debug libraries.
 */
#ifndef FDLEAK_DEBUG_COMMON_H
#define FDLEAK_DEBUG_COMMON_H

#include "private/logd.h"

#ifdef __cplusplus
extern "C" {
#endif

/* FD leakage debugging initialization and finalization routines.
 *
 * These routines must be implemented in libc_fdleak_debug_mtk.so that implement FD leak
 * debugging. They are called once per process from fdleak_init_impl and
 * fdleak_fini_impl respectively.
 *
 * FDLeakDebugInit returns:
 *    0 on success, -1 on failure.
 */
typedef int (*FDLeakDebugInit)(void);
typedef void (*FDLeakDebugFini)(void);


/* FD leakage debugging backtrace record and remove routines.
 *
 * These routines must be implemented in libc_fdleak_debug_mtk.so that implement FD leak
 * debugging. They are called once per process from fdleak_init_impl and
 * fdleak_fini_impl respectively.
 *
 */
typedef void (*FDLeak_Record_Backtrace)(int);
typedef void (*FDLeak_Remove_Backtrace)(int);


/* 
 *log functions
 */
#define fdleak_debug_log(format, ...)  \
    __libc_android_log_print(ANDROID_LOG_DEBUG, "fdleak_debug", (format), ##__VA_ARGS__ )
#define fdleak_error_log(format, ...)  \
    __libc_android_log_print(ANDROID_LOG_ERROR, "fdleak_debug", (format), ##__VA_ARGS__ )
#define fdleak_info_log(format, ...)  \
    __libc_android_log_print(ANDROID_LOG_INFO, "fdleak_debug", (format), ##__VA_ARGS__ )

/* 
 *global function pointer for FD allocate/close backtrace record/remove
 * these rountines will be provided in libc_fdleak_debug_mtk.so 
 */
#ifdef _MTK_ENG_
FDLeak_Record_Backtrace fdleak_record_backtrace;
FDLeak_Remove_Backtrace fdleak_remove_backtrace;
#endif

#ifdef __cplusplus
};
#endif
#endif
