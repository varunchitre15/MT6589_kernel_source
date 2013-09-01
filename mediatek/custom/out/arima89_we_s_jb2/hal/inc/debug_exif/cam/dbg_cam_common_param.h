

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

#ifndef _DBG_CAM_COMMON_PARAM_H
#define _DBG_CAM_COMMON_PARAM_H

// Common debug info
#define CMN_DEBUG_TAG_SIZE     10
#define CMN_DEBUG_TAG_VERSION  0


typedef struct DEBUG_CMN_INFO_S
{
    DEBUG_CAM_TAG_T Tag[CMN_DEBUG_TAG_SIZE];
} DEBUG_CMN_INFO_T;


//Common Parameter Structure
typedef enum
{
    CMN_TAG_VERSION = 0,
    CMN_TAG_SHOT_MODE,
    CMN_TAG_CAM_MODE
    
}DEBUG_CMN_TAG_T;

#endif //_DBG_CAM_COMMON_PPARAM_H
