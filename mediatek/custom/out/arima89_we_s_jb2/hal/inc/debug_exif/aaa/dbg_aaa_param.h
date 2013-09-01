
#ifndef _DBG_AAA_PARAM_H_
#define _DBG_AAA_PARAM_H_

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 3A debug info
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#define AAATAG(module_id, tag, line_keep)   \
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

typedef struct
{
    MUINT32 u4FieldID;
    MUINT32 u4FieldValue;
}  AAA_DEBUG_TAG_T;

#include "dbg_ae_param.h"
#include "dbg_af_param.h"
#include "dbg_awb_param.h"
#include "dbg_flash_param.h"
#include "dbg_flicker_param.h"

#define AAA_DEBUG_AE_MODULE_ID          0x0001
#define AAA_DEBUG_AF_MODULE_ID          0x0002
#define AAA_DEBUG_AWB_MODULE_ID         0x0003
#define AAA_DEBUG_FLASH_MODULE_ID       0x0004
#define AAA_DEBUG_FLICKER_MODULE_ID     0x0005
#define AAA_DEBUG_AWB_DATA_MODULE_ID    0x0006

#define AAA_DEBUG_KEYID 0xF0F1F202 // DP version2

typedef struct
{
    struct Header
    {
        MUINT32  u4KeyID;
        MUINT32  u4ModuleCount;
        MUINT32  u4AEDebugInfoOffset;
        MUINT32  u4AFDebugInfoOffset;
        MUINT32  u4AWBDebugInfoOffset;
        MUINT32  u4FlashDebugInfoOffset;
        MUINT32  u4FlickerDebugInfoOffset;
        MUINT32  u4AWBDebugDataOffset;

    } hdr;

    AE_DEBUG_INFO_T  rAEDebugInfo;
    AF_DEBUG_INFO_T  rAFDebugInfo;
    AWB_DEBUG_INFO_T rAWBDebugInfo;
    FLASH_DEBUG_INFO_T rFlashDebugInfo;
    FLICKER_DEBUG_INFO_T rFlickerDebugInfo;
    AWB_DEBUG_DATA_T rAWBDebugData;
} AAA_DEBUG_INFO_T;

#endif // _DBG_AAA_PARAM_H_

