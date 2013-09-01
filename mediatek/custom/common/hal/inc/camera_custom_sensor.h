#ifndef _CAMERA_CUSTOM_SENSOR_H_
#define _CAMERA_CUSTOM_SENSOR_H_

#include "camera_custom_types.h"
#include "camera_custom_nvram.h"


namespace NSFeature
{


struct FeatureInfoProvider;
class SensorInfoBase
{
public:     ////            Feature Type.
                            typedef enum
                            {
                                EType_RAW =   0,  //  RAW Sensor
                                EType_YUV,        //  YUV Sensor
                            }   EType;
                        
                            typedef NSFeature::FeatureInfoProvider FeatureInfoProvider_T;

public:
    virtual                 ~SensorInfoBase(){}

public:     ////            Interface.
    virtual MBOOL           GetFeatureProvider(FeatureInfoProvider_T& rFInfoProvider) { return false; }
    virtual EType           GetType() const                                 = 0;
    virtual MUINT32         GetID()   const                                 = 0;
    virtual char const*     getDrvName() const                              = 0;
    virtual char const*     getDrvMacroName() const                         = 0;
};


template <SensorInfoBase::EType _sensor_type, MUINT32 _sensor_id>
class SensorInfo : public SensorInfoBase
{
public:     ////
    typedef SensorInfo<_sensor_type, _sensor_id>    SensorInfo_T;
public:     ////            Interface.
    virtual EType           GetType() const { return _sensor_type; }
    virtual MUINT32         GetID()   const { return _sensor_id;   }
                            //
    virtual char const*     getDrvName() const      { return mpszDrvName; }
    virtual char const*     getDrvMacroName() const { return mpszDrvMacroName; }
public:     ////            Implementation.
                            SensorInfo()
                                : mpszDrvName(0), mpszDrvMacroName(0)
                            {
                            }
protected:  ////            Data Members.
    char const*             mpszDrvName;
    char const*             mpszDrvMacroName;
};


template <MUINT32 _sensor_id>
class YUVSensorInfo : public SensorInfo<SensorInfoBase::EType_YUV, _sensor_id>
{
    typedef YUVSensorInfo<_sensor_id>   SensorInfo_T;
public:     ////            Interface.
    static  SensorInfo_T*   createInstance(char const* pszDrvName = "", char const* pszDrvMacroName = "")
                            {
                                getInstance()->mpszDrvName      = pszDrvName;;
                                getInstance()->mpszDrvMacroName = pszDrvMacroName;
                                return  getInstance();
                            }
    static  SensorInfo_T*   getInstance() { static SensorInfo_T inst; return &inst; }
    static  MUINT32         getDefaultData(CAMERA_DATA_TYPE_ENUM const CameraDataType, MVOID*const pDataBuf, MUINT32 const size)
                            {
                                return  -1;
                            }
                            //
    typedef SensorInfoBase::FeatureInfoProvider_T FeatureInfoProvider_T;
    virtual MBOOL           GetFeatureProvider(FeatureInfoProvider_T& rFInfoProvider) { return false; }
protected:  ////            Implementation.
    virtual MUINT32         impGetDefaultData(CAMERA_DATA_TYPE_ENUM const CameraDataType, MVOID*const pDataBuf, MUINT32 const size) const { return  -1; }
    static  SensorInfoBase* GetInstance();
};


template <MUINT32 _sensor_id>
class RAWSensorInfo : public SensorInfo<SensorInfoBase::EType_RAW, _sensor_id>
{
    typedef RAWSensorInfo<_sensor_id>   SensorInfo_T;
public:     ////            Interface.
    static  SensorInfo_T*   createInstance(char const* pszDrvName = "", char const* pszDrvMacroName = "")
                            {
                                getInstance()->mpszDrvName      = pszDrvName;;
                                getInstance()->mpszDrvMacroName = pszDrvMacroName;
                                return  getInstance();
                            }
    static  SensorInfo_T*   getInstance() { static SensorInfo_T inst; return &inst; }
    static  MUINT32         getDefaultData(CAMERA_DATA_TYPE_ENUM const CameraDataType, MVOID*const pDataBuf, MUINT32 const size)
                            {
                                return  getInstance()->impGetDefaultData(CameraDataType, pDataBuf, size);
                            }
protected:  ////            Implementation.
    virtual MUINT32         impGetDefaultData(CAMERA_DATA_TYPE_ENUM const CameraDataType, MVOID*const pDataBuf, MUINT32 const size) const;

};


};  //  NSFeature


typedef struct
{
    MUINT32 SensorId;
    MUINT8  drvname[32];
    NSFeature::SensorInfoBase* pSensorInfo;
    NSFeature::SensorInfoBase* (*pfGetSensorInfoInstance)();
    MUINT32 (*getCameraDefault)(CAMERA_DATA_TYPE_ENUM CameraDataType, MVOID *pDataBuf, MUINT32 size);
    MUINT32 (*getCameraCalData)(MUINT32* pGetCalData);
} MSDK_SENSOR_INIT_FUNCTION_STRUCT, *PMSDK_SENSOR_INIT_FUNCTION_STRUCT;

MUINT32 GetSensorInitFuncList(MSDK_SENSOR_INIT_FUNCTION_STRUCT **ppSensorList);


#endif  //  _CAMERA_CUSTOM_SENSOR_H_

