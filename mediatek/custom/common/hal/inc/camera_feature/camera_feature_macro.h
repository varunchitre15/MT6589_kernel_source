#ifndef _CAMERA_FEATURE_MACRO_H_
#define _CAMERA_FEATURE_MACRO_H_


namespace NSFeature
{


/*******************************************************************************
* MACRO Define: Scene Independent
*******************************************************************************/
#define _GETFINFO_SCENE_INDEP(_sensor_type_sz)                              \
    static                                                                  \
    FInfoIF*                                                                \
    GetFInfo(FID_T const fid)                                               \
    {                                                                       \
        MY_LOG_OBJ_ONCE(_sensor_type_sz "[SI][GetFInfo]\n");                \
        switch  (fid)                                                       \
        {                                                                   \

#define _END_GETFINFO_SCENE_INDEP(_sensor_type_sz)                          \
        default:                                                            \
            MY_LOG(                                                         \
                _sensor_type_sz "[SI][GetFInfo] unsupported FID:"           \
                "(fid)=(%d)", fid                                           \
            );                                                              \
            break;                                                          \
        }                                                                   \
        return  NULL;                                                       \
    }                                                                       \


/*******************************************************************************
* MACRO Define: Scene Dependent
*******************************************************************************/
#define _GETFINFO_SCENE_DEP(_sensor_type_sz)                                \
    template <MUINT32 _SceneNum>                                            \
    static                                                                  \
    FInfoIF*                                                                \
    GetFInfo(FID_T const fid, SID_T const sid)                              \
    {                                                                       \
        enum { ESceneNum  = _SceneNum };                                    \
        MY_LOG_OBJ_ONCE(                                                    \
            _sensor_type_sz "[SD][GetFInfo]"                                \
            "(SceneNum)=(%d)", ESceneNum                                    \
        );                                                                  \
        if  ( ESceneNum <= sid )                                            \
        {                                                                   \
            MY_ERR(                                                         \
                _sensor_type_sz "[SD][GetFInfo] unsupported SID:"           \
                "(sid)=(%d)\n", sid                                         \
            );                                                              \
            return  NULL;                                                   \
        }                                                                   \
        switch  (sid)                                                       \
        {

#define _END_GETFINFO_SCENE_DEP(_sensor_type_sz)                            \
        default:                                                            \
            MY_ERR(                                                         \
                _sensor_type_sz "[SD][GetFInfo] unsupported FID:"           \
                "(sid,fid)=(%d,%d)", sid, fid                               \
            );                                                              \
            break;                                                          \
        }                                                                   \
        return  NULL;                                                       \
    }


/*******************************************************************************
* MACRO Define: Config Scene
*******************************************************************************/
#define _CONFIG_SCENE(_sid, _sensor_type_sz)                                \
    case _sid:                                                              \
    {                                                                       \
        STATIC_CHECK(_sid < TOTAL_TABLE_SCENE_NUM, _sid##__NOT_scene_dep);  \
        switch (fid)                                                        \
        {

#define _END_CONFIG_SCENE(_sensor_type_sz)                                  \
        default:                                                            \
            MY_LOG(                                                         \
                _sensor_type_sz "[SD][GetFInfo] unsupported FID:"           \
                "(sid,fid)=(%d,%d)", sid, fid                               \
            );                                                              \
            break;                                                          \
        }                                                                   \
    }                                                                       \
    break;


/*******************************************************************************
* MACRO Define: Config Feature
*******************************************************************************/
#define CHECK_FID_RAW(_fid)     \
    STATIC_CHECK(Fid2Type<_fid>::Info::isRAW, _fid##__NOT_RAW_fid)
#define CHECK_FID_RAW_SI(_fid)  \
    STATIC_CHECK(Fid2Type<_fid>::Info::isRAWSceneIndep, _fid##__NOT_scene_indep)
#define CHECK_FID_RAW_SD(_fid)  \
    STATIC_CHECK(Fid2Type<_fid>::Info::isRAWSceneDep, _fid##__NOT_scene_dep)
#define CHECK_FID_YUV   (_fid)  \
    STATIC_CHECK(Fid2Type<_fid>::Info::isYUV, _fid##__NOT_YUV_fid)
#define CHECK_FID_YUV_SI(_fid)  \
    STATIC_CHECK(Fid2Type<_fid>::Info::isYUVSceneIndep, _fid##__NOT_scene_indep)
#define CHECK_FID_YUV_SD(_fid)  \
    STATIC_CHECK(Fid2Type<_fid>::Info::isYUVSceneDep, _fid##__NOT_scene_dep)
#define CHECK_FID_MISC_SI(_fid)  \
    STATIC_CHECK(Fid2Type<_fid>::Info::isMiscSceneIndep, _fid##__NOT_scene_indep)
#define CHECK_FID_MISC_SD(_fid)  \
    STATIC_CHECK(Fid2Type<_fid>::Info::isMiscSceneDep, _fid##__NOT_scene_dep)


#define __CONFIG_FEATURE(_fid, _ftype)                                      \
        enum { EFeatureID = _fid };                                         \
        typedef _ftype const type;

#define __END_CONFIG_FEATURE()                                              \
    STATIC_CHECK(sizeof(aDefault)==sizeof(type), _NOT_assign_DEFAULT);      \
    STATIC_CHECK(sizeof(aTable) > 0, _table_CANNOT_be_emtpy);               \
    enum { ETblNum = sizeof(aTable)/sizeof(type) };                         \
    static FInfo_Source<EFeatureID, ETblNum, type>                          \
    FInfo(aDefault[0], aTable);

#define _CONFIG_FEATURE(_fid, _ftype)                                       \
    case _fid:                                                              \
    {   __CONFIG_FEATURE(_fid, _ftype)

#define _END_CONFIG_FEATURE()                                               \
        __END_CONFIG_FEATURE()                                              \
        return  &FInfo;                                                     \
    }

#define BY_DEFAULT(_default)                                \
        type const aDefault[] = { _default };

#define CONFIG_FEATURE_SI(_fid, _default_macro, _table...)  \
    _CONFIG_FEATURE(_fid, Fid2Type<_fid>::Type)             \
        CHECK_FID_SI(_fid);                                 \
        _default_macro                                      \
        static type aTable[] = { _table };                  \
    _END_CONFIG_FEATURE()

#define CONFIG_FEATURE_SD(_fid, _default_macro, _table...)  \
    _CONFIG_FEATURE(_fid, Fid2Type<_fid>::Type)             \
        CHECK_FID_SD(_fid);                                 \
        _default_macro                                      \
        static type aTable[] = { _table };                  \
    _END_CONFIG_FEATURE()


};  //  namespace NSFeature


#endif  //  _CAMERA_FEATURE_MACRO_H_

