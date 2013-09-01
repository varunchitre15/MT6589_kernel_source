#ifndef _CAMERA_FEATURE_INFO_H_
#define _CAMERA_FEATURE_INFO_H_

#include <cutils/xlog.h>
#define LOGD(fmt, arg...) XLOGD(fmt, ##arg)

namespace NSFeature
{


/*******************************************************************************
* 
*******************************************************************************/
//  Feature Info Interface
class FInfoIF
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Common.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:     ////
    //  type of feature id.
    typedef MUINT32 FID_T;
    //  type of scene id.
    typedef MUINT32 SID_T;
    //  Generic Feature Type.
    typedef MUINT32 Feature_T;

private:    ////    Disallowed.
    //  Copy constructor is disallowed.
    FInfoIF(FInfoIF const&);
    //  Copy-assignment operator is disallowed.
    FInfoIF& operator=(FInfoIF const&);

public:
    FInfoIF() {}
    virtual ~FInfoIF() {}

public:     ////    Attributes.

    virtual FID_T           GetFID()        const = 0;
    virtual MUINT32         GetCount()      const = 0;
    virtual Feature_T const*GetTable()      const = 0;
    virtual Feature_T       GetDefault()    const = 0;
    virtual MBOOL           SetDefault(Feature_T const fValue) = 0;

public:     ////    Operations.
    virtual MBOOL ClearAll() = 0;
    virtual MBOOL Copy(FInfoIF*const pFInfo) = 0;
    virtual MBOOL SetToOne(Feature_T const fValue) = 0;
    virtual MBOOL IsWithin(Feature_T const fValue) = 0;
    virtual MBOOL Remove(Feature_T const fValue) = 0;
    virtual void Dump() = 0;
};


/*******************************************************************************
* 
*******************************************************************************/
template <
    MBOOL       _isAryType, 
    MUINT32     _fid, 
    MUINT32     _capacity, 
    typename    _ftype
>
class FInfoTemplate :   public FInfoIF
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Common.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
public:
    typedef _ftype ftype;
    enum { ECapacity = _capacity };
    enum { EFid = _fid };

    //  type of feature table in form of array
    typedef ftype       Table_As_Array_T [ECapacity];
    //  type of feature table in form of pointer to array
    typedef ftype*const Table_As_Pointer_T;

    typedef typename SelectType<
        _isAryType, Table_As_Array_T, Table_As_Pointer_T
    >::Type             TABLE_T;

protected:  ////    Data Members.

    TABLE_T     m_fTable;
    //  default feature.
    ftype       m_fDefault;
    //  count of legal elements in table.
    MUINT32 m_u4Count;

private:    ////    Disallowed.
    //  Copy constructor is disallowed.
    FInfoTemplate(FInfoTemplate const&);
    //  Copy-assignment operator is disallowed.
    FInfoTemplate& operator=(FInfoTemplate const&);

public:
    FInfoTemplate()
        : m_u4Count(0)
    {
        STATIC_CHECK(0 < ECapacity, ZERO_CAPACITY);
        STATIC_CHECK(
            sizeof(ftype)==sizeof(Feature_T), 
            Different_Sizeof_Feature_Type
        );
    }

    FInfoTemplate(ftype fDefault, ftype afTable[])
        : m_fTable(afTable)
        , m_fDefault(fDefault)
        , m_u4Count(ECapacity)
    {
        STATIC_CHECK(0 < ECapacity, ZERO_CAPACITY);
        STATIC_CHECK(
            sizeof(ftype)==sizeof(Feature_T), 
            Different_Sizeof_Feature_Type
        );
    }

    virtual ~FInfoTemplate() {}

public:     ////    Attributes.

    virtual FID_T           GetFID()    const   { return EFid;       }
    virtual MUINT32         GetCount()  const   { return m_u4Count;  }
    virtual Feature_T       GetDefault()const   { return m_fDefault; }
    virtual Feature_T const*GetTable() const
    {
        return reinterpret_cast<Feature_T const*>(m_fTable);
    }

public:     ////    Operations.

    virtual
    MBOOL
    IsWithin(Feature_T const fValue)
    {
        MUINT32 const u4Count = GetCount();
        Feature_T const* pTable = 
        reinterpret_cast<Feature_T const*>(
            GetTable()
        );
        for (MUINT32 i = 0; i < u4Count; i++)
        {
            if  ( fValue == pTable[i] )
                return  MTRUE;
        }
        return  MFALSE;
    }

    virtual
    void
    Dump()
    {
        MUINT32 const u4Count = GetCount();
        ftype const* pTable = reinterpret_cast<ftype const*>(GetTable());
        ftype const fDefault= static_cast<ftype const>(GetDefault());
//        LOGD("[FInfoTemplate](fid, u4Count, pTable, fDefault)=(%d, %d, %p, %d)"
//            , GetFID(), u4Count, pTable, fDefault);
        MUINT32 u4fMask = 0;
        for (MUINT32 i = 0; i < u4Count; i++)
        {
            u4fMask |= (1<<pTable[i]);
//            LOGD("%d-th: %d", i, pTable[i]);
        }
        LOGD("[FInfoTemplate](fid, u4Count, pTable, fDefault, u4fMask)="
             "(%d, %d, %p, %d, 0x%08X)"
            , GetFID(), u4Count, pTable, fDefault, u4fMask);
    }
};


/*******************************************************************************
* 
*******************************************************************************/
template <
    MUINT32     _fid, 
    MUINT32     _capacity, 
    typename    _ftype
>
class FInfo_Target : public FInfoTemplate<MTRUE, _fid, _capacity, _ftype>
{
public:     ////
    typedef FInfoTemplate<MTRUE, _fid, _capacity, _ftype> Parent_T;

    //  Generic Feature Type.
    typedef FInfoIF::Feature_T          Feature_T;
    //  Real Feature Type.
    typedef typename Parent_T::ftype    ftype;

private:    ////    Disallowed.
    //  Copy constructor is disallowed.
    FInfo_Target(FInfo_Target const&);
    //  Copy-assignment operator is disallowed.
    FInfo_Target& operator=(FInfo_Target const&);

public:
    FInfo_Target()
        : Parent_T()
    {}
    virtual ~FInfo_Target() {}

public:     ////    Attributes.
    virtual MBOOL SetDefault(Feature_T const fValue)
    {
        if  ( Parent_T::IsWithin(fValue) )
        {
            Parent_T::m_fDefault = static_cast<ftype const>(fValue);
            return  MTRUE;
        }
        return  MFALSE;
    }

public:     ////    Operations.
    virtual
    MBOOL
    ClearAll()
    {
        Parent_T::m_u4Count = 0;
        memset(Parent_T::m_fTable, 0, sizeof(Parent_T::m_fTable));
        return  MTRUE;
    }

    virtual
    MBOOL
    Copy(FInfoIF*const pFInfo)
    {
        ftype const* pTable = reinterpret_cast<ftype const*>(
            pFInfo->GetTable()
        );
        ftype const fDefault= static_cast<ftype const>(
            pFInfo->GetDefault()
        );
        MUINT32 const u4Count = pFInfo->GetCount();
        if  ( Parent_T::ECapacity < u4Count || ! pTable )
            return  MFALSE;

        Parent_T::m_fDefault = fDefault;
        Parent_T::m_u4Count  = u4Count;
        if  ( u4Count > 0 && pTable )
        {
            memcpy(Parent_T::m_fTable, pTable, u4Count*sizeof(ftype));
        }
        return  MTRUE;
    }

    virtual
    MBOOL
    SetToOne(Feature_T const fValue)
    {
        ftype const fOneValue = static_cast<ftype const>(fValue);
        Parent_T::m_fDefault = fOneValue;
        Parent_T::m_u4Count  = 1;
        Parent_T::m_fTable[0]= fOneValue;
        return  MTRUE;
    }

    virtual
    MBOOL
    Remove(Feature_T const fValue)
    {
        MUINT32 const u4Count = Parent_T::m_u4Count;
        ftype *const pTable = Parent_T::m_fTable;
        for (MUINT32 i = 0; i < u4Count; i++)
        {
            if  ( fValue == pTable[i] )
            {
                if  ( u4Count - 1 != i )
                {   //  not the last element.
                    ::memcpy(&pTable[i], &pTable[i+1], sizeof(ftype)*(u4Count-1-i));
                }
                Parent_T::m_u4Count--;

                if  ( fValue == Parent_T::m_fDefault )
                {
                    Parent_T::m_fDefault = pTable[0];
                }
                return  MTRUE;
            }
        }
        return MFALSE;
    }
};


/*******************************************************************************
* 
*******************************************************************************/
template <
    MUINT32     _fid, 
    MUINT32     _capacity, 
    typename    _ftype
>
class FInfo_Source : public FInfoTemplate<MFALSE, _fid, _capacity, _ftype>
{
public:     ////
    typedef FInfoTemplate<MFALSE, _fid, _capacity, _ftype> Parent_T;

    //  Generic Feature Type.
    typedef FInfoIF::Feature_T          Feature_T;
    //  Real Feature Type.
    typedef typename Parent_T::ftype    ftype;

private:    ////    Disallowed.
    //  Copy constructor is disallowed.
    FInfo_Source(FInfo_Source const&);
    //  Copy-assignment operator is disallowed.
    FInfo_Source& operator=(FInfo_Source const&);

public:
    FInfo_Source(ftype fDefault, ftype afTable[])
        : Parent_T(fDefault, afTable)
    {}
    virtual ~FInfo_Source() {}

public:     ////    Attributes.
    virtual MBOOL SetDefault(Feature_T const fValue)
    {
        return  MFALSE;
    }

public:     ////    Operations.
    virtual MBOOL ClearAll()
        { return MFALSE; }
    virtual MBOOL Copy(FInfoIF*const pFInfo)
        { return MFALSE; }
    virtual MBOOL SetToOne(Feature_T const fValue)
        { return MFALSE; }
    virtual MBOOL Remove(Feature_T const fValue)
        { return MFALSE; }
};


};  //  namespace NSFeature


#endif // _CAMERA_FEATURE_INFO_H_

