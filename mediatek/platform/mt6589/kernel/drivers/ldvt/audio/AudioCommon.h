/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*******************************************************************************
 *
 * Filename:
 * ---------
 *   AudioCommon.h
 *
 * Project:
 * --------
 *   Android Audio Driver
 *
 * Description:
 * ------------
 *   Audio Global definition
 *
 * Author:
 * -------
 *   Ir Lian (mtk00976)
 *
 *------------------------------------------------------------------------------
 * $Revision: #5 $
 * $Modtime:$
 * $Log:$
 *
 *
 *******************************************************************************/

#ifndef ANDROID_AUDIO_GLOBAL_H
#define ANDROID_AUDIO_GLOBAL_H

#define LDVT

#ifdef LDVT
#include <linux/module.h>
#include <linux/time.h>
#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>

// Type re-definition
typedef signed char INT8;
typedef unsigned char UINT8;
typedef short INT16;
typedef unsigned short UINT16;
typedef int INT32;
typedef unsigned int UINT32;
typedef long long INT64;
typedef unsigned long long UNIT64;

typedef enum
{
    FALSE = 0x0,
    TRUE = 0x1
} BOOL;

#else // for DVT

#include "common.h"
#define printk dbg_print

#endif


#endif /*ANDROID_AUDIO_GLOBAL_H*/

