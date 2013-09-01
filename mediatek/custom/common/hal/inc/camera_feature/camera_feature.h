#ifndef _CAMERA_FEATURE_H_
#define _CAMERA_FEATURE_H_

#include "camera_feature_types.h"
#include "camera_feature_id.h"
#include "camera_feature_utility.h"
#include "camera_feature_enum.h"
#include "camera_feature_info.h"
#ifdef  USE_CAMERA_FEATURE_MACRO
    #include "camera_feature_macro.h"
    #include "camera_feature_debug.h"
#endif


namespace NSFeature
{


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Typedef
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
typedef MUINT32 FID_T;  //  type of feature id.
typedef MUINT32 SID_T;  //  type of scene id.


typedef FInfoIF*(*PF_GETFINFO_SCENE_INDEP_T)(FID_T const);
typedef FInfoIF*(*PF_GETFINFO_SCENE_DEP_T  )(FID_T const, SID_T const);
struct FeatureInfoProvider
{
    PF_GETFINFO_SCENE_INDEP_T   pfGetFInfo_SceneIndep;
    PF_GETFINFO_SCENE_DEP_T     pfGetFInfo_SceneDep;
};


typedef enum
{
    ECamRole_Main   =   0,  //   Main Camera
    ECamRole_Sub,           //   Sub Camera
    ECamRole_N3D_Main,      //   N3D Main Camera
    ECamRole_B3D_Main,      //   B3D Main Camera
}   ECamRole_T;

typedef enum
{
    ESensorType_RAW =   0,  //  RAW Sensor
    ESensorType_YUV,        //  YUV Sensor
}   ESensorType;


enum
{
    ENumOfScene =   Fid2Type<FID_SCENE_MODE>::Num
};


};  //  namespace NSFeature


#endif  //  _CAMERA_FEATURE_H_

