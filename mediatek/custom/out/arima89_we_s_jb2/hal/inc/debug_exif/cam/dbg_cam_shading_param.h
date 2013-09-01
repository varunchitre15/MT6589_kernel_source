

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

#ifndef _DBG_CAM_SHADING_PARAM_H
#define _DBG_CAM_SHADING_PARAM_H

//Shading Parameter Structure
typedef enum
{
    SHAD_TAG_VERSION = 0,
    /* add tags here */
    SHAD_TAG_SCENE_IDX,
    SHAD_TAG_CT_IDX,
    SHAD_TAG_CAM_CTL_DMA_EN,
    SHAD_TAG_CAM_LSCI_BASE_ADDR,
    SHAD_TAG_CAM_LSCI_XSIZE,
    SHAD_TAG_CAM_CTL_EN1,
    SHAD_TAG_CAM_LSC_CTL1,
    SHAD_TAG_CAM_LSC_CTL2,
    SHAD_TAG_CAM_LSC_CTL3,
    SHAD_TAG_CAM_LSC_LBLOCK,
    SHAD_TAG_CAM_LSC_RATIO,
    SHAD_TAG_CAM_LSC_GAIN_TH,
    SHAD_TAG_END
}DEBUG_SHAD_TAG_T;

// Shading debug info
#define SHAD_ARRAY_VALUE_SIZE   400 // FIXME
#define SHAD_DEBUG_TAG_SIZE     (SHAD_TAG_END+10)
#define SHAD_DEBUG_TAG_VERSION  (1)
//
#define DEBUF_SHAD_ARRAY_TOT_MODULE_NUM    1
#define DEBUF_SHAD_ARRAY_TAG_MODULE_NUM    1
//
#define DEBUG_SHAD_ARRAY_KEYID    0xF2F4F6F8
//

typedef struct DEBUG_SHAD_ARRAY_S
{
    MUINT32 ArrayWidth;
    MUINT32 ArrayHeight;
    MUINT32 Array[SHAD_ARRAY_VALUE_SIZE];
} DEBUG_SHAD_ARRAY_T;


typedef struct DEBUG_SHAD_INFO_S
{
    DEBUG_CAM_TAG_T Tag[SHAD_DEBUG_TAG_SIZE];
} DEBUG_SHAD_INFO_T;

//
typedef struct DEBUG_SHAD_ARRAY_INFO_S
{
    struct Header
    {
        MUINT32  u4KeyID;
        MUINT32  u4ModuleCount;
        MUINT32  u4DbgSHADArrayOffset;
    } hdr;
    DEBUG_SHAD_ARRAY_T  rDbgSHADArray;

} DEBUG_SHAD_ARRAY_INFO_T;
//


#endif //_DBG_CAM_SHADING_PARAM_H
