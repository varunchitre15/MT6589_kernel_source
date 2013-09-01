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

#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

/*
 * fdleak_debug_common.c only called by libc.so build
 */
#ifdef _MTK_ENG_
#include <sys/system_properties.h>
#include <dlfcn.h>
#include "fdleak_debug_common.h"


static void* libc_fdleak_impl_handle = NULL;
extern char*  __progname;


/* Initializes fdleak debug once per process. */
static void fdleak_init_impl(void) 
{
    const char* so_name = NULL;
    FDLeakDebugInit fdleak_debug_initialize = NULL;
    unsigned int fdleak_debug_enable = 0;
    char env[PROP_VALUE_MAX];
    char debug_program[PROP_VALUE_MAX];

    /* Default enable. */
    fdleak_debug_enable = 1;

    if (__system_property_get("persist.debug.fdleak", env)) {
        fdleak_debug_enable = atoi(env);
    }

    if(!fdleak_debug_enable) {
        return;
    }

    /* Process need bypass */
    if (strstr(__progname, "system/bin/aee")) {/*bypass aee and aee_dumpstate*/
        fdleak_debug_enable = 0;
    }
    
    /* Control for only one specific program to enable fdleak debug. */
    if (__system_property_get("persist.debug.fdleak.program", debug_program)) {
        if (strstr(__progname, debug_program)) {
            fdleak_debug_enable = 1;
        }
        else {
            fdleak_debug_enable = 0;
        }
    }
    
    fdleak_debug_log("[%s:%d] %d\n", __progname, getpid(), fdleak_debug_enable);
    if(!fdleak_debug_enable) {
        return;
    }

    so_name = "/system/lib/libc_fdleak_debug_mtk.so";
    libc_fdleak_impl_handle = dlopen(so_name, RTLD_LAZY);
    if (libc_fdleak_impl_handle == NULL) {
        fdleak_error_log("%s: Missing module %s required for fdleak debug %d\n",
                 __progname, so_name, fdleak_debug_enable);
        return;
    }

    /* Initialize fdleak debugging in the loaded module.*/
    fdleak_debug_initialize =
            dlsym(libc_fdleak_impl_handle, "fdleak_debug_initialize_mtk");
    if (fdleak_debug_initialize == NULL) {
        fdleak_error_log("%s: fdleak_debug_initialize is not found in %s\n",
                  __progname, so_name);
        dlclose(libc_fdleak_impl_handle);
        return;
    }
    
    if (fdleak_debug_initialize()) {
        dlclose(libc_fdleak_impl_handle);
        return;
    }

    fdleak_record_backtrace = 
            dlsym(libc_fdleak_impl_handle, "fdleak_record_backtrace_mtk");
    fdleak_remove_backtrace = 
            dlsym(libc_fdleak_impl_handle, "fdleak_remove_backtrace_mtk");
    if ((fdleak_record_backtrace == NULL) || 
        (fdleak_remove_backtrace == NULL)) {
        fdleak_error_log("%s: Can't get bactrace record/remove function:%p, %p",
                          __progname,fdleak_record_backtrace,fdleak_remove_backtrace); 
                          
        dlclose(libc_fdleak_impl_handle);
        libc_fdleak_impl_handle = NULL;
        fdleak_record_backtrace = NULL;
        fdleak_remove_backtrace = NULL;
    }
    
}

static void fdleak_fini_impl(void)
{
    if (libc_fdleak_impl_handle) {
        FDLeakDebugFini fdleak_debug_finalize = NULL;
        fdleak_debug_finalize = dlsym(libc_fdleak_impl_handle, "fdleak_debug_finalize_mtk");
        if (fdleak_debug_finalize)
            fdleak_debug_finalize();
        dlclose(libc_fdleak_impl_handle);
        libc_fdleak_impl_handle = NULL;
        fdleak_record_backtrace = NULL;
        fdleak_remove_backtrace = NULL;
    }
}

static pthread_once_t  fdleak_init_once_ctl = PTHREAD_ONCE_INIT;
static pthread_once_t  fdleak_fini_once_ctl = PTHREAD_ONCE_INIT;
#endif // _MTK_ENG_


/* Initializes FD Leakge debugging.
 * This routine is called from __libc_preinit routines
 */
void fdleak_debug_init(void)
{
#ifdef _MTK_ENG_
    if (pthread_once(&fdleak_init_once_ctl, fdleak_init_impl)) {
        fdleak_error_log("Unable to initialize fdleak_debug component.");
    }
#endif
}

/* DeInitializes FD Leakge debugging.
 * This routine is called from __libc_postfini routines
 */
void fdleak_debug_fini(void)
{
#ifdef _MTK_ENG_
    if (pthread_once(&fdleak_fini_once_ctl, fdleak_fini_impl)) {
        fdleak_error_log("Unable to finalize fdleak_debug component.");
    }
#endif
}

