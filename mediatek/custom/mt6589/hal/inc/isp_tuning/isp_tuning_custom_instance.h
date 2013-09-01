
#ifndef _ISP_TUNING_CUSTOM_INSTANCE_H_
#define _ISP_TUNING_CUSTOM_INSTANCE_H_


namespace NSIspTuning
{

/*******************************************************************************
*
*******************************************************************************/
template <ESensorDev_T const eSensorDev>
class CTIspTuningCustom : public IspTuningCustom
{
public:
    static
    IspTuningCustom*
    getInstance(MUINT32 const u4SensorID)
    {
        static CTIspTuningCustom<eSensorDev> singleton(u4SensorID);
        return &singleton;
    }
    virtual void destroyInstance() {}

    CTIspTuningCustom(MUINT32 const u4SensorID)
        : IspTuningCustom()
        , m_rIdxSetMgr(IdxSetMgrBase::getInstance())
        , m_u4SensorID(u4SensorID)
    {}

public:     ////    Attributes
    virtual ESensorDev_T   getSensorDev() const { return eSensorDev; }
    virtual MUINT32   getSensorID() const { return m_u4SensorID; }
    virtual
    INDEX_T const*
    getDefaultIndex(
        EIspProfile_T const eIspProfile,
        EIndex_Scene_T const eIdx_Scene,
        EIndex_ISO_T const eIdx_ISO
    ) const
    {
        return m_rIdxSetMgr.get(eIspProfile, eIdx_Scene, eIdx_ISO);
    }

protected:  ////    Data Members.
    IdxSetMgrBase&  m_rIdxSetMgr;
    MUINT32 const m_u4SensorID;

};


/*******************************************************************************
* Customers can specialize CTIspTuningCustom<xxx>
* and then override default behaviors if needed.
*******************************************************************************/
#if 0
/*
    ps: where ESensorDev_xxx = ESensorDev_Main/ESensorDev_MainSecond/ESensorDev_Sub
*/
template <>
class CTIspTuningCustom< ESensorDev_Main > : public IspTuningCustom
{
public:
    static
    IspTuningCustom*
    getInstance()
    {
        static CTIspTuningCustom< ESensorDev_Main > singleton;
        return &singleton;
    }
    virtual void destroyInstance() {}

public:     ////    Overrided Interfaces.
    //......
};

#endif


/*******************************************************************************
*
*******************************************************************************/

#define INSTANTIATE(_dev_id) \
    case _dev_id: return  CTIspTuningCustom<_dev_id>::getInstance(u4SensorID)

IspTuningCustom*
IspTuningCustom::
createInstance(ESensorDev_T const eSensorDev, MUINT32 const u4SensorID)
{
    switch  (eSensorDev)
    {
    INSTANTIATE(ESensorDev_Main);       //  Main Sensor
    INSTANTIATE(ESensorDev_MainSecond); //  Main Second Sensor
    INSTANTIATE(ESensorDev_Sub);        //  Sub Sensor
    default:
        break;
    }

    return  NULL;
}


};  //  NSIspTuning
#endif  //  _ISP_TUNING_CUSTOM_INSTANCE_H_

