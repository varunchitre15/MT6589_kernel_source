#ifndef _CAMERA_FEATURE_TYPES_H
#define _CAMERA_FEATURE_TYPES_H

/*******************************************************************************
*
*******************************************************************************/
#include "camera_custom_types.h"

namespace NSFeature
{


typedef struct 
{
    MUINT32 u4FeatureID;
    MUINT32 u4FIDSupportFlag;
    MUINT32 u4SubItemTotalNum;
    MUINT32 u4DefaultSelection;
    MUINT32*pu4SubItemAllSupport;
} CamFeatureStruct, *PCamFeatureStruct;


};


#endif // _CAMERA_FEATURE_TYPES_H

