#ifndef _CAMERA_CUSTOM_N3D_IF_
#define _CAMERA_CUSTOM_N3D_IF_
//
#include "camera_custom_types.h"
//

/*******************************************************************************
* N3D sensor position
******************************************************************************/
typedef struct customSensorPos_N3D_s
{
    MUINT32  uSensorPos;
} customSensorPos_N3D_t;

customSensorPos_N3D_t const&  getSensorPosN3D();

/*******************************************************************************
* Return enable/disable flag of N3D to ISP
*******************************************************************************/
MBOOL get_N3DFeatureFlag(void);  //cotta : added for N3D

#endif  //  _CAMERA_CUSTOM_N3D_IF_
