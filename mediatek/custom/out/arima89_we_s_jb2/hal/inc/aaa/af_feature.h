
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

#ifndef _LIB3A_AF_FEATURE_H
#define _LIB3A_AF_FEATURE_H

// AF command ID: 0x2000 ~
typedef enum
{
    LIB3A_AF_CMD_ID_SET_AF_MODE  = 0x2000,	 // Set AF Mode
    LIB3A_AF_CMD_ID_SET_AF_METER = 0x2001,	 // Set AF Meter

} LIB3A_AF_CMD_ID_T;

// AF mode definition
typedef enum
{
    LIB3A_AF_MODE_OFF = 0,           // Disable AF
    LIB3A_AF_MODE_AFS,               // AF-Single Shot Mode
    LIB3A_AF_MODE_AFC,               // AF-Continuous Mode
    LIB3A_AF_MODE_AFC_VIDEO,         // AF-Continuous Mode (Video)
	LIB3A_AF_MODE_MACRO,			   // AF Macro Mode
    LIB3A_AF_MODE_INFINITY,          // Infinity Focus Mode
	LIB3A_AF_MODE_MF, 			   // Manual Focus Mode
    LIB3A_AF_MODE_CALIBRATION,       // AF Calibration Mode
	LIB3A_AF_MODE_FULLSCAN,	       // AF Full Scan Mode

    LIB3A_AF_MODE_NUM,               // AF mode number
    LIB3A_AF_MODE_MIN = LIB3A_AF_MODE_OFF,
    LIB3A_AF_MODE_MAX = LIB3A_AF_MODE_FULLSCAN

} LIB3A_AF_MODE_T;

// AF meter definition
typedef enum
{
    LIB3A_AF_METER_SPOT = 0,      // Spot Window
    LIB3A_AF_METER_MATRIX,       		 // Matrix Window
    LIB3A_AF_METER_FD,			     // FD Window
    LIB3A_AF_METER_CONTI,         // for AFC

    LIB3A_AF_METER_NUM,
    LIB3A_AF_METER_MIN = LIB3A_AF_METER_SPOT,
    LIB3A_AF_METER_MAX = LIB3A_AF_METER_CONTI

} LIB3A_AF_METER_T;

#endif
