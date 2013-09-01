

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

#ifndef _LIB3A_AE_FEATURE_H
#define _LIB3A_AE_FEATURE_H

// Cam mode definition
typedef enum                           
{
    LIB3A_AECAM_MODE_UNSUPPORTED      = -1,
    LIB3A_AECAM_MODE_PHOTO                       = 0,	
    LIB3A_AECAM_MODE_VIDEO                   = 1,    
    LIB3A_AECAM_MODE_ZSD                       = 2,    
    LIB3A_AECAM_MODE_S3D                       = 3,        
    LIB3A_AECAM_MODE_MAX
} LIB3A_AECAM_MODE_T;

// AE EV compensation
typedef enum                            // enum  for evcompensate
{
    LIB3A_AE_EV_COMP_UNSUPPORTED = -1,
    LIB3A_AE_EV_COMP_00          =  0,           // Disable EV compenate
    LIB3A_AE_EV_COMP_03          =  1,           // EV compensate 0.3
    LIB3A_AE_EV_COMP_05          =  2,           // EV compensate 0.5
    LIB3A_AE_EV_COMP_07          =  3,           // EV compensate 0.7
    LIB3A_AE_EV_COMP_10          =  4,           // EV compensate 1.0
    LIB3A_AE_EV_COMP_13          =  5,           // EV compensate 1.3
    LIB3A_AE_EV_COMP_15          =  6,           // EV compensate 1.5
    LIB3A_AE_EV_COMP_17          =  7,           // EV compensate 1.7
    LIB3A_AE_EV_COMP_20          =  8,           // EV compensate 2.0
    LIB3A_AE_EV_COMP_25          =  9,           // EV compensate 2.5
    LIB3A_AE_EV_COMP_30          =  10,           // EV compensate 3.0
    LIB3A_AE_EV_COMP_35          =  11,           // EV compensate 3.5
    LIB3A_AE_EV_COMP_40          =  12,           // EV compensate 4.0
    LIB3A_AE_EV_COMP_n03         = 13,           // EV compensate -0.3
    LIB3A_AE_EV_COMP_n05         = 14,           // EV compensate -0.5
    LIB3A_AE_EV_COMP_n07         = 15,           // EV compensate -0.7
    LIB3A_AE_EV_COMP_n10         = 16,           // EV compensate -1.0
    LIB3A_AE_EV_COMP_n13         = 17,           // EV compensate -1.3
    LIB3A_AE_EV_COMP_n15         = 18,           // EV compensate -1.5
    LIB3A_AE_EV_COMP_n17         = 19,           // EV compensate -1.7
    LIB3A_AE_EV_COMP_n20         = 20,           // EV compensate -2.0
    LIB3A_AE_EV_COMP_n25         = 21,           // EV compensate -2.5
    LIB3A_AE_EV_COMP_n30         = 22,           // EV compensate -3.0
    LIB3A_AE_EV_COMP_n35         = 23,           // EV compensate -3.5
    LIB3A_AE_EV_COMP_n40         = 24,           // EV compensate -4.0
    LIB3A_AE_EV_COMP_MAX
}LIB3A_AE_EVCOMP_T;

// AE metering mode
typedef enum                                  // enum for metering
{
    LIB3A_AE_METERING_MODE_UNSUPPORTED    = -1,
    LIB3A_AE_METERING_MODE_CENTER_WEIGHT,           // CENTER WEIGHTED MODE
    LIB3A_AE_METERING_MODE_SOPT,                    // SPOT MODE
    LIB3A_AE_METERING_MODE_AVERAGE,                 // AVERAGE MODE
    LIB3A_AE_METERING_MODE_MULTI,                   // MULTI MODE
    LIB3A_AE_METERING_MODE_MAX
}LIB3A_AE_METERING_MODE_T;

// AE set flicker mode
typedef enum
{
    LIB3A_AE_FLICKER_MODE_UNSUPPORTED = -1,
    LIB3A_AE_FLICKER_MODE_60HZ,
    LIB3A_AE_FLICKER_MODE_50HZ,
    LIB3A_AE_FLICKER_MODE_AUTO,    // No support in MT6516
    LIB3A_AE_FLICKER_MODE_OFF,     // No support in MT6516
    LIB3A_AE_FLICKER_MODE_MAX
}LIB3A_AE_FLICKER_MODE_T;

// AE set frame rate mode   //10base
typedef enum
{
    LIB3A_AE_FRAMERATE_MODE_UNSUPPORTED = -1,
    LIB3A_AE_FRAMERATE_MODE_DYNAMIC = 0,
    LIB3A_AE_FRAMERATE_MODE_05FPS = 50,
    LIB3A_AE_FRAMERATE_MODE_15FPS = 150,
    LIB3A_AE_FRAMERATE_MODE_30FPS = 300,
    LIB3A_AE_FRAMERATE_MODE_60FPS = 600,
    LIB3A_AE_FRAMERATE_MODE_90FPS = 900,
    LIB3A_AE_FRAMERATE_MODE_120FPS = 1200,
    LIB3A_AE_FRAMERATE_MODE_MAX = LIB3A_AE_FRAMERATE_MODE_120FPS
}LIB3A_AE_FRAMERATE_MODE_T;

// for flicker detection algorithm used only.
typedef enum
{
    LIB3A_AE_FLICKER_AUTO_MODE_UNSUPPORTED = -1,
    LIB3A_AE_FLICKER_AUTO_MODE_50HZ,
    LIB3A_AE_FLICKER_AUTO_MODE_60HZ,
    LIB3A_AE_FLICKER_AUTO_MODE_MAX
}LIB3A_AE_FLICKER_AUTO_MODE_T;

// remove later
typedef enum
{
    LIB3A_AE_STROBE_MODE_UNSUPPORTED = -1,
    LIB3A_AE_STROBE_MODE_AUTO        =  0,
    LIB3A_AE_STROBE_MODE_SLOWSYNC    =  0, //NOW DO NOT SUPPORT SLOW SYNC, TEMPERALLY THE SAME WITH AUTO
    LIB3A_AE_STROBE_MODE_FORCE_ON    =  1,
    LIB3A_AE_STROBE_MODE_FORCE_OFF   =  2,
    LIB3A_AE_STROBE_MODE_FORCE_TORCH =  3,
    LIB3A_AE_STROBE_MODE_REDEYE      =  4,
    LIB3A_AE_STROBE_MODE_TOTAL_NUM,
    LIB3A_AE_STROBE_MODE_MIN = LIB3A_AE_STROBE_MODE_AUTO,
    LIB3A_AE_STROBE_MODE_MAX = LIB3A_AE_STROBE_MODE_FORCE_OFF
}LIB3A_AE_STROBE_MODE_T;

#endif
