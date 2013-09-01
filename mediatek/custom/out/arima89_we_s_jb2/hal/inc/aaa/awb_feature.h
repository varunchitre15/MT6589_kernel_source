

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

#ifndef _LIB3A_AWB_FEATURE_H
#define _LIB3A_AWB_FEATURE_H

// AWB mode definition
typedef enum
{
    LIB3A_AWB_MODE_AUTO ,                // Auto white balance
    LIB3A_AWB_MODE_DAYLIGHT,             // Daylight
    LIB3A_AWB_MODE_CLOUDY_DAYLIGHT,      // Cloudy daylight
    LIB3A_AWB_MODE_SHADE,                // Shade
    LIB3A_AWB_MODE_TWILIGHT,             // Twilight
    LIB3A_AWB_MODE_FLUORESCENT,          // Fluorescent
    LIB3A_AWB_MODE_WARM_FLUORESCENT,     // Warm fluorescent
    LIB3A_AWB_MODE_INCANDESCENT,         // Incandescent
    LIB3A_AWB_MODE_GRAYWORLD,            // Gray world mode for CCT use
    LIB3A_AWB_MODE_NUM,                  // AWB mode number
    LIB3A_AWB_MODE_MIN = LIB3A_AWB_MODE_AUTO,
    LIB3A_AWB_MODE_MAX = LIB3A_AWB_MODE_GRAYWORLD
} LIB3A_AWB_MODE_T;

#endif
