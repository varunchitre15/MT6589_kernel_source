

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

#ifndef __FLASH_FEATURE_H__
#define __FLASH_FEATURE_H__

// flash mode definition
typedef enum
{
    LIB3A_FLASH_MODE_UNSUPPORTED = -1,
    LIB3A_FLASH_MODE_AUTO        =  0,
    LIB3A_FLASH_MODE_SLOWSYNC    =  0, //NOW DO NOT SUPPORT SLOW SYNC, TEMPERALLY THE SAME WITH AUTO
    LIB3A_FLASH_MODE_FORCE_ON    =  1,
    LIB3A_FLASH_MODE_FORCE_OFF   =  2,
    LIB3A_FLASH_MODE_REDEYE      =  3,
    LIB3A_FLASH_MODE_FORCE_TORCH =  4,    
    LIB3A_FLASH_MODE_TOTAL_NUM,
    LIB3A_FLASH_MODE_MIN = LIB3A_FLASH_MODE_AUTO,
    LIB3A_FLASH_MODE_MAX = LIB3A_FLASH_MODE_FORCE_TORCH, //cotta-- modified to TORCH for FLASH in video
}LIB3A_FLASH_MODE_T;

#endif //#ifndef __FLASH_FEATURE_H__
