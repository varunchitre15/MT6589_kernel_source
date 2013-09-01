#ifndef _CAMERA_FEATURE_ID_H_
#define _CAMERA_FEATURE_ID_H_


namespace NSFeature
{


#include "kd_camera_feature_id.h"
typedef FEATURE_ID  FEATURE_ID_T;


enum
{
    FID_BEGIN               = FID_PRE_BEGIN + 1, 
    FID_NUM                 = FID_OVER_LAST - FID_BEGIN, 

  //////////////////////////////////////////////////////////////////////////////
  // Scene-Independent (SI) feature id.
    FID_BEGIN_SI            = FID_PRE_BEGIN_SI + 1, 
    FID_END_SI              = FID_OVER_LAST_SI, 

    //Misc. feature id.
    FID_BEGIN_MISC_SI       = FID_PRE_BEGIN_MISC_SI + 1, 
    FID_END_MISC_SI         = FID_OVER_LAST_MISC_SI, 
    FID_NUM_MISC_SI         = FID_END_MISC_SI - FID_BEGIN_MISC_SI, 

    //RAW-only feature id.
    FID_BEGIN_RAW_ONLY_SI   = FID_PRE_BEGIN_RAW_ONLY_SI + 1, 
    FID_END_RAW_ONLY_SI     = FID_OVER_LAST_RAW_ONLY_SI, 
    FID_NUM_RAW_ONLY_SI     = FID_END_RAW_ONLY_SI - FID_BEGIN_RAW_ONLY_SI, 
    //RAW-YUV-shared feature id.
    FID_BEGIN_RAW_YUV_SI    = FID_PRE_BEGIN_RAW_YUV_SI + 1, 
    FID_END_RAW_YUV_SI      = FID_OVER_LAST_RAW_YUV_SI, 
    FID_NUM_RAW_YUV_SI      = FID_END_RAW_YUV_SI - FID_BEGIN_RAW_YUV_SI, 
    //YUV-only feature id.
    FID_BEGIN_YUV_ONLY_SI   = FID_PRE_BEGIN_YUV_ONLY_SI + 1, 
    FID_END_YUV_ONLY_SI     = FID_OVER_LAST_YUV_ONLY_SI, 
    FID_NUM_YUV_ONLY_SI     = FID_END_YUV_ONLY_SI - FID_BEGIN_YUV_ONLY_SI, 

    //RAW feature id.
    FID_BEGIN_RAW_SI        = FID_BEGIN_RAW_ONLY_SI, 
    FID_END_RAW_SI          = FID_OVER_LAST_RAW_YUV_SI, 
    FID_NUM_RAW_SI          = FID_END_RAW_SI - FID_BEGIN_RAW_SI, 
    //YUV feature id.
    FID_BEGIN_YUV_SI        = FID_BEGIN_RAW_YUV_SI, 
    FID_END_YUV_SI          = FID_END_YUV_ONLY_SI, 
    FID_NUM_YUV_SI          = FID_END_YUV_SI - FID_BEGIN_YUV_SI, 

  //////////////////////////////////////////////////////////////////////////////
  // Scene-Dependent (SD) feature id.
    FID_BEGIN_SD            = FID_PRE_BEGIN_SD + 1, 
    FID_END_SD              = FID_OVER_LAST_SD, 

    //Misc. feature id.
    FID_BEGIN_MISC_SD       = FID_PRE_BEGIN_MISC_SD + 1, 
    FID_END_MISC_SD         = FID_OVER_LAST_MISC_SD, 
    FID_NUM_MISC_SD         = FID_END_MISC_SD - FID_BEGIN_MISC_SD, 

    //RAW-only feature id.
    FID_BEGIN_RAW_ONLY_SD   = FID_PRE_BEGIN_RAW_ONLY_SD + 1, 
    FID_END_RAW_ONLY_SD     = FID_OVER_LAST_RAW_ONLY_SD, 
    FID_NUM_RAW_ONLY_SD     = FID_END_RAW_ONLY_SD - FID_BEGIN_RAW_ONLY_SD, 
    //RAW-YUV-shared feature id.
    FID_BEGIN_RAW_YUV_SD    = FID_PRE_BEGIN_RAW_YUV_SD + 1, 
    FID_END_RAW_YUV_SD      = FID_OVER_LAST_RAW_YUV_SD, 
    FID_NUM_RAW_YUV_SD      = FID_END_RAW_YUV_SD - FID_BEGIN_RAW_YUV_SD, 
    //YUV-only feature id.
    FID_BEGIN_YUV_ONLY_SD   = FID_PRE_BEGIN_YUV_ONLY_SD + 1, 
    FID_END_YUV_ONLY_SD     = FID_OVER_LAST_YUV_ONLY_SD, 
    FID_NUM_YUV_ONLY_SD     = FID_END_YUV_ONLY_SD - FID_BEGIN_YUV_ONLY_SD, 

    //RAW feature id.
    FID_BEGIN_RAW_SD        = FID_BEGIN_RAW_ONLY_SD, 
    FID_END_RAW_SD          = FID_OVER_LAST_RAW_YUV_SD, 
    FID_NUM_RAW_SD          = FID_END_RAW_SD - FID_BEGIN_RAW_SD, 
    //YUV feature id.
    FID_BEGIN_YUV_SD        = FID_BEGIN_RAW_YUV_SD, 
    FID_END_YUV_SD          = FID_END_YUV_ONLY_SD, 
    FID_NUM_YUV_SD          = FID_END_YUV_SD - FID_BEGIN_YUV_SD, 

};


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Generic Feature ID Info
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
template <MUINT32 _fid>
struct FidInfo
{
    template <MUINT32 _val, MUINT32 _beg, MUINT32 _end>
    struct IsWithin
    {
        enum { Result = (_beg <= _val && _val < _end) };
    };

    enum
    {
      isSceneIndep      =
          IsWithin<_fid,FID_BEGIN_SI,FID_END_SI>::Result, 
      isMiscSceneIndep  =
          IsWithin<_fid,FID_BEGIN_MISC_SI,FID_END_MISC_SI>::Result, 
      isRAWSceneIndep   =
          IsWithin<_fid,FID_BEGIN_RAW_SI,FID_END_RAW_SI>::Result, 
      isYUVSceneIndep   =
          IsWithin<_fid,FID_BEGIN_YUV_SI,FID_END_YUV_SI>::Result, 

      isSceneDep        =
          IsWithin<_fid,FID_BEGIN_SD,FID_END_SD>::Result, 
      isMiscSceneDep    =
          IsWithin<_fid,FID_BEGIN_MISC_SD,FID_END_MISC_SD>::Result, 
      isRAWSceneDep     =
          IsWithin<_fid,FID_BEGIN_RAW_SD,FID_END_RAW_SD>::Result, 
      isYUVSceneDep     =
          IsWithin<_fid,FID_BEGIN_YUV_SD,FID_END_YUV_SD>::Result, 

      isRAW             = (isRAWSceneIndep || isRAWSceneDep), 
      isYUV             = (isYUVSceneIndep || isYUVSceneDep), 
    };
};


};  //  namespace NSFeature


#endif  //  _CAMERA_FEATURE_ID_H_

