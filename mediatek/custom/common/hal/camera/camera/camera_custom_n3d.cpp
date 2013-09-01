#include "camera_custom_n3d.h"

/*******************************************************************************
* N3D sensor position
*******************************************************************************/
customSensorPos_N3D_t const&
getSensorPosN3D()
{
    static customSensorPos_N3D_t inst = {
        uSensorPos   : 1,   //0:LR 1:RL (L:tg1, R:tg2)
    };
    return inst;
}

/*******************************************************************************
* Author : cotta
* brief : Return enable/disable flag of N3D
*******************************************************************************/
MBOOL get_N3DFeatureFlag(void)
{
#ifdef MTK_NATIVE_3D_SUPPORT
    return MTRUE;
#else
    return MFALSE;
#endif
}
