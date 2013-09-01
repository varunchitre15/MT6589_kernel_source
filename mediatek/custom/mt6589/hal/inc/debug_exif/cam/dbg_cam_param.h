
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

#ifndef _DBG_CAM_PARAM_H
#define _DBG_CAM_PARAM_H

typedef struct
{
    MUINT32 u4FieldID;
    MUINT32 u4FieldValue;
} DEBUG_CAM_TAG_T;

#include "dbg_cam_common_param.h"
#include "dbg_cam_mf_param.h"
#include "dbg_cam_n3d_param.h"
#include "dbg_cam_sensor_param.h"
#include "dbg_cam_shading_param.h"


// dbgCAM debug info
#define CAMTAG(module_id, tag, line_keep)   \
( (MINT32)                                  \
  ((MUINT32)(0x00000000) |                  \
   (MUINT32)((module_id & 0xff) << 24) |    \
   (MUINT32)((line_keep & 0x01) << 23) |    \
   (MUINT32)(tag & 0xffff))                 \
)

#define MODULE_NUM(total_module, tag_module)      \
((MINT32)                                         \
 ((MUINT32)(0x00000000) |                         \
  (MUINT32)((total_module & 0xff) << 16) |        \
  (MUINT32)(tag_module & 0xff))                   \
)
//
#define DEBUG_CAM_CMN_MID           0x0005
#define DEBUG_CAM_MF_MID            0x0006
#define DEBUG_CAM_N3D_MID           0x0007
#define DEBUG_CAM_SENSOR_MID        0x0008
#define DEBUG_CAM_SHAD_MID          0x0009
#define DEBUG_CAM_SHAD_ARRAY_MID    0x0010
//
#define DEBUF_CAM_TOT_MODULE_NUM    5 //should be modified
#define DEBUF_CAM_TAG_MODULE_NUM    5 //should be modified

//
#define DEBUG_CAM_KEYID     0xF8F9FAFB
//

typedef struct DEBUG_CAM_INFO_S
{
    struct Header
    {
        MUINT32  u4KeyID;
        MUINT32  u4ModuleCount;
        MUINT32  u4DbgCMNInfoOffset;
        MUINT32  u4DbgMFInfoOffset;
        MUINT32  u4DbgN3DInfoOffset;
        MUINT32  u4DbgSENSORInfoOffset;
        MUINT32  u4DbgSHADInfoOffset;
    } hdr;

    DEBUG_CMN_INFO_T    rDbgCMNInfo;
    DEBUG_MF_INFO_T     rDbgMFInfo;
    DEBUG_N3D_INFO_T    rDbgN3DInfo;
    DEBUG_SENSOR_INFO_T rDbgSENSORInfo;
    DEBUG_SHAD_INFO_T   rDbgSHADInfo;

} DEBUG_CAM_INFO_T;


#endif  //_DBG_CAM_PARAM_H

