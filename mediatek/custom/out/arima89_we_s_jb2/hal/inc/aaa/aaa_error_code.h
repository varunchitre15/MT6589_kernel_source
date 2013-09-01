

/*
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef _AAA_ERROR_CODE_H
#define _AAA_ERROR_CODE_H

///////////////////////////////////////////////////////////////////////////////
//!  Error code formmat is:
//!
//!  Bit 31~24 is global, each module must follow it, bit 23~0 is defined by module
//!  | 31(1 bit) |30-24(7 bits) |         23-0   (24 bits)      |
//!  | Indicator | Module ID    |   Module-defined error Code   |
//!
//!  Example 1:
//!  | 31(1 bit) |30-24(7 bits) |   23-16(8 bits)   | 15-0(16 bits) |
//!  | Indicator | Module ID    | group or sub-mod  |    Err Code   |
//!
//!  Example 2:
//!  | 31(1 bit) |30-24(7 bits) | 23-12(12 bits)| 11-8(8 bits) | 7-0(16 bits)  |
//!  | Indicator | Module ID    |   line number |    group     |    Err Code   |
//!
//!  Indicator  : 0 - success, 1 - error
//!  Module ID  : module ID, defined below
//!  Extended   : module dependent, but providee macro to add partial line info
//!  Err code   : defined in each module's public include file,
//!               IF module ID is MODULE_COMMON, the errocode is
//!               defined here
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
//! Error code type definition
///////////////////////////////////////////////////////////////////////////
typedef MINT32 MRESULT;

///////////////////////////////////////////////////////////////////////////
//! Helper macros to define error code
///////////////////////////////////////////////////////////////////////////
#define ERROR_CODE_DEFINE(modid, errid)           \
  ((MINT32)                              \
    ((MUINT32)(0x80000000) |             \
     (MUINT32)((modid & 0x7f) << 24) |   \
     (MUINT32)(errid & 0xffff))          \
  )

#define OK_CODE_DEFINE(modid, okid)             \
  ((MINT32)                              \
    ((MUINT32)(0x00000000) |             \
     (MUINT32)((modid & 0x7f) << 24) |   \
     (MUINT32)(okid & 0xffff))           \
  )

///////////////////////////////////////////////////////////////////////////
//! Helper macros to check error code
///////////////////////////////////////////////////////////////////////////
#define SUCCEEDED(Status)   ((MRESULT)(Status) >= 0)
#define FAILED(Status)      ((MRESULT)(Status) < 0)

#define MODULE_MTK_3A (0) // Temp value

#define MTK3A_OKCODE(errid)         OK_CODE_DEFINE(MODULE_MTK_3A, errid)
#define MTK3A_ERRCODE(errid)        ERROR_CODE_DEFINE(MODULE_MTK_3A, errid)


// 3A error code
#define S_3A_OK                  MTK3A_OKCODE(0x0000)

#define E_3A_NEED_OVER_WRITE     MTK3A_ERRCODE(0x0001)
#define E_3A_NULL_OBJECT         MTK3A_ERRCODE(0x0002)
#define E_3A_INCORRECT_STATE     MTK3A_ERRCODE(0x0003)
#define E_3A_INCORRECT_CMD_ID    MTK3A_ERRCODE(0x0004)
#define E_3A_INCORRECT_CMD_PARAM MTK3A_ERRCODE(0x0005)
#define E_3A_UNSUPPORT_COMMAND   MTK3A_ERRCODE(0x0006)

#define E_3A_ERR                 MTK3A_ERRCODE(0x0100)

// AE error code
#define S_AE_OK                     MTK3A_OKCODE(0x0000)

#define E_AE_WRONG_STATE            MTK3A_ERRCODE(0x0101)
#define E_AE_NULL_AE_TABLE          MTK3A_ERRCODE(0x0102)
#define E_AE_DISALBE                MTK3A_ERRCODE(0x0103)
#define E_AE_HIST_ROI_ERR           MTK3A_ERRCODE(0x0104)
#define E_AE_NOMATCH_TABLE          MTK3A_ERRCODE(0x0105)
#define E_AE_ISO_NOT_SUPPORT        MTK3A_ERRCODE(0x0106)
#define E_AE_CAPTURE_AE_WRONG       MTK3A_ERRCODE(0x0107)
#define E_AE_PARAMETER_ERROR        MTK3A_ERRCODE(0x0108)
#define E_AE_NULL_BUFFER            MTK3A_ERRCODE(0x0109)
#define E_AE_UNSUPPORT_MODE         MTK3A_ERRCODE(0x0110)
#define E_AE_NVRAM_DATA         MTK3A_ERRCODE(0x0111)
#define E_AE_SENSOR_INFO         MTK3A_ERRCODE(0x0112)
#define E_AE_ALGO_INIT_ERR         MTK3A_ERRCODE(0x0113)

// AF error code
#define S_AF_OK                     MTK3A_OKCODE(0x0000)
#define E_AF_BUSY                   MTK3A_ERRCODE(0x0201)
#define E_AF_INPUT                  MTK3A_ERRCODE(0x0202)
#define E_AF_NOSUPPORT              MTK3A_ERRCODE(0x0203)
#define E_AF_NULL_POINTER           MTK3A_ERRCODE(0x0204)
#define E_AF_MCU_FAIL               MTK3A_ERRCODE(0x0205)
#define E_AF_ISP_DRV_FAIL           MTK3A_ERRCODE(0x0206)
#define E_AF_ISP_REG_FAIL           MTK3A_ERRCODE(0x0207)




// AWB error code
#define S_AWB_OK                     MTK3A_OKCODE(0x0000)
#define E_AWB_DISABLE                MTK3A_ERRCODE(0x0301)
#define E_AWB_STATISTIC_ERROR        MTK3A_ERRCODE(0x0302)
#define E_AWB_UNSUPPORT_MODE         MTK3A_ERRCODE(0x0303)
#define E_AWB_STATISTIC_CONFIG_ERROR MTK3A_ERRCODE(0x0304)
#define E_AWB_PARAMETER_ERROR        MTK3A_ERRCODE(0x0305)
#define E_AWB_NULL_POINTER           MTK3A_ERRCODE(0x0306)
#define E_STROBE_AWB_PARAMETER_ERROR MTK3A_ERRCODE(0x0307)
#define E_AWB_SENSOR_ERROR           MTK3A_ERRCODE(0x0308)
#define E_AWB_NVRAM_ERROR            MTK3A_ERRCODE(0x0309)

// NVRAM error code
#define S_NVRAM_OK                   MTK3A_OKCODE(0x0000)
#define E_NVRAM_BAD_SENSOR_DRV       MTK3A_ERRCODE(0x0401)
#define E_NVRAM_BAD_PARAM            MTK3A_ERRCODE(0x0402)
#define E_NVRAM_BAD_NVRAM_DRV        MTK3A_ERRCODE(0x0403)

// ISP manager error code
#define S_ISPMGR_OK                  MTK3A_OKCODE(0x0000)
#define E_ISPMGR_NULL_POINTER        MTK3A_ERRCODE(0x0501)
#define E_ISPMGR_NULL_ADDRESS        MTK3A_ERRCODE(0x0502)
#define E_ISPMGR_INVALID_SETTING     MTK3A_ERRCODE(0x0503)

// Buffer manager error code
#define S_BUFMGR_OK                  MTK3A_OKCODE(0x0000)
#define E_BUFMGR_ILLEGAL_DMA_CHANNEL MTK3A_ERRCODE(0x0601)
#define E_BUFMGR_MEMORY_ERROR        MTK3A_ERRCODE(0x0602)

// AAA sensor manager error code
#define S_AAA_SENSOR_MGR_OK  MTK3A_OKCODE(0x0000)
#define E_AAA_SENSOR_NULL  MTK3A_ERRCODE(0x0701)
#define E_AAA_SENSOR_TYPE  MTK3A_ERRCODE(0x0702)

#endif

