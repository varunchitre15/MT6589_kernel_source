#ifndef _CAMERA_FEATURE_UTILITY_H_
#define _CAMERA_FEATURE_UTILITY_H_


namespace NSFeature
{


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Generic Utility (Loki)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
template <MUINT32 v>
struct
Int2Type
{
    enum { value = v };
};

template<MBOOL> struct CompileTimeError;
template<> struct CompileTimeError<MTRUE> {};

#define STATIC_CHECK(expr, msg) \
    { CompileTimeError<((expr) != 0)> ERROR_##msg; (void)ERROR_##msg; }

template <MBOOL _isT1, typename T1, typename T2>
struct SelectType
{
    typedef T1  Type;
};
template <typename T1, typename T2>
struct SelectType<MFALSE, T1, T2>
{
    typedef T2  Type;
};


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Enumeration for subitems supported by a given Featrue ID.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
template <MUINT32 _fid> struct Fid2Type{};

#undef  FTYPE_ENUM
#undef  FID_TO_TYPE_ENUM
#define FTYPE_ENUM(_enums...)  _enums
#define FID_TO_TYPE_ENUM(_fid, _enums)                                      \
    typedef enum { _enums, OVER_NUM_OF_##_fid } _fid##_T;                   \
    template <>                                                             \
    struct Fid2Type<_fid>                                                   \
    {                                                                       \
        typedef _fid##_T        Type;                                       \
        typedef FidInfo<_fid>   Info;                                       \
        enum                                                                \
        {                                                                   \
            Num = (OVER_NUM_OF_##_fid - 1),                                 \
        };                                                                  \
    };                                                                      \
    typedef _fid##_T 


};  //  namespace NSFeature


#endif  //  _CAMERA_FEATURE_UTILITY_H_

