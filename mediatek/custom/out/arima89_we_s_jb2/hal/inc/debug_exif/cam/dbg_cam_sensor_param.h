
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

#ifndef _DBG_CAM_SENSOR_PARAM_H
#define _DBG_CAM_SENSOR_PARAM_H

// Sensor debug info
#define SENSOR_DEBUG_TAG_SIZE     20
#define SENSOR_DEBUG_TAG_VERSION  0


typedef struct DEBUG_SENSOR_INFO_S
{
    DEBUG_CAM_TAG_T Tag[SENSOR_DEBUG_TAG_SIZE];
} DEBUG_SENSOR_INFO_T;


//Common Parameter Structure
typedef enum
{
    SENSOR_TAG_VERSION = 0,
    SENSOR1_TAG_COLORORDER, //0:B , 1:Gb, 2:Gr, 3:R, 4:UYVY, 5:VYUY, 6:YUYV, 7:YVYU
    SENSOR1_TAG_DATATYPE, //0:RAW, 1:YUV, 2:YCBCR, 3:RGB565, 4:RGB888, 5:JPEG
    SENSOR1_TAG_HARDWARE_INTERFACE,//0: parallel, 1:MIPI
    SENSOR1_TAG_GRAB_START_X,
    SENSOR1_TAG_GRAB_START_Y,
    SENSOR1_TAG_GRAB_WIDTH,
    SENSOR1_TAG_GRAB_HEIGHT,
    SENSOR2_TAG_COLORORDER, //0:B , 1:Gb, 2:Gr, 3:R, 4:UYVY, 5:VYUY, 6:YUYV, 7:YVYU
    SENSOR2_TAG_DATATYPE, //0:RAW, 1:YUV, 2:YCBCR, 3:RGB565, 4:RGB888, 5:JPEG
    SENSOR2_TAG_HARDWARE_INTERFACE,//0: parallel, 1:MIPI
    SENSOR2_TAG_GRAB_START_X,
    SENSOR2_TAG_GRAB_START_Y,
    SENSOR2_TAG_GRAB_WIDTH,
    SENSOR2_TAG_GRAB_HEIGHT,
    
    /* TBD */

}DEBUG_SENSOR_TAG_T;

#endif //_DBG_CAM_SENSOR_PARAM_H
