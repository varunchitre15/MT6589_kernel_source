
#ifndef _ISP_TUNING_CAM_INFO_H_
#define _ISP_TUNING_CAM_INFO_H_

using namespace NSFeature;

namespace NSIspTuning
{


/*******************************************************************************
*
*******************************************************************************/

//  Scene index
typedef Fid2Type<FID_SCENE_MODE>::Type      EIndex_Scene_T;
enum { eNUM_OF_SCENE_IDX = Fid2Type<FID_SCENE_MODE>::Num };


//  Color Effect Index
typedef Fid2Type<FID_COLOR_EFFECT>::Type    EIndex_Effect_T;


//  ISP End-User-Define Tuning Index:
//  Edge, Hue, Saturation, Brightness, Contrast
typedef Fid2Type<FID_ISP_EDGE    >::Type    EIndex_Isp_Edge_T;
typedef Fid2Type<FID_ISP_HUE     >::Type    EIndex_Isp_Hue_T;
typedef Fid2Type<FID_ISP_SAT     >::Type    EIndex_Isp_Saturation_T;
typedef Fid2Type<FID_ISP_BRIGHT  >::Type    EIndex_Isp_Brightness_T;
typedef Fid2Type<FID_ISP_CONTRAST>::Type    EIndex_Isp_Contrast_T;
typedef struct IspUsrSelectLevel
{
    EIndex_Isp_Edge_T           eIdx_Edge;
    EIndex_Isp_Hue_T            eIdx_Hue;
    EIndex_Isp_Saturation_T     eIdx_Sat;
    EIndex_Isp_Brightness_T     eIdx_Bright;
    EIndex_Isp_Contrast_T       eIdx_Contrast;

    IspUsrSelectLevel()
        : eIdx_Edge     (ISP_EDGE_MIDDLE)
        , eIdx_Hue      (ISP_HUE_MIDDLE)
        , eIdx_Sat      (ISP_SAT_MIDDLE)
        , eIdx_Bright   (ISP_BRIGHT_MIDDLE)
        , eIdx_Contrast (ISP_CONTRAST_MIDDLE)
    {}
} IspUsrSelectLevel_T;


//  ISO index.
typedef enum EIndex_ISO
{
    eIDX_ISO_100 = 0,
    eIDX_ISO_200,
    eIDX_ISO_400,
    eIDX_ISO_800,
    eIDX_ISO_1600,
    eIDX_ISO_2400,
    eIDX_ISO_3200,
    eNUM_OF_ISO_IDX
} EIndex_ISO_T;


// PCA LUT index
typedef enum EIndex_PCA_LUT
{
    eIDX_PCA_LOW  = 0,
    eIDX_PCA_MIDDLE,
    eIDX_PCA_HIGH
} EIndex_PCA_LUT_T;


//  Correlated color temperature index for shading.
typedef enum EIndex_Shading_CCT
{
    eIDX_Shading_CCT_BEGIN  = 0,
    eIDX_Shading_CCT_ALight   = eIDX_Shading_CCT_BEGIN,
    eIDX_Shading_CCT_CWF,
    eIDX_Shading_CCT_D65,
    eIDX_Shading_CCT_RSVD
} EIndex_Shading_CCT_T;


/*******************************************************************************
*
*******************************************************************************/
struct IspCamInfo
{
public:
    EIspProfile_T       eIspProfile; // ISP profile.
    EIndex_Scene_T      eIdx_Scene;  // scene mode.

public:
    IspCamInfo()
    : eIspProfile(EIspProfile_NormalPreview)
    , eIdx_Scene(SCENE_MODE_OFF)
    {}

public:
    void dump() const
    {
        LOGD("[IspCamInfo][dump](eIspProfile, eIdx_Scene)=(%d, %d)", eIspProfile, eIdx_Scene);
    }
};


/*******************************************************************************
*
*******************************************************************************/
struct RAWIspCamInfo : public IspCamInfo
{
public:
    MUINT32             u4ISOValue;         //  iso value
    EIndex_ISO_T        eIdx_ISO;           //  iso index

    //MINT32              i4CCT;              //  color temperature.
    EIndex_PCA_LUT_T    eIdx_PCA_LUT;       // Index for PCA
    //EIndex_CCM_CCT_T    eIdx_CCM_CCT;       //  CT index for CCM.
    EIndex_Shading_CCT_T    eIdx_Shading_CCT;       //  CT index for Shading.
    AWB_INFO_T rAWBInfo; // AWB info for ISP tuning
    AE_INFO_T rAEInfo; // AE info for ISP tuning
    AF_INFO_T rAFInfo; // AF info for ISP tuning
    FLASH_INFO_T rFlashInfo; // Flash info for ISP tuning

    /*
        x100 Zoom Ratio
    */
    MINT32             i4ZoomRatio_x100;

    /*
        x10 "light value" or ¡§light level¡¨

        In photography, light value has been used to refer to a ¡§light level¡¨
        for either incident or reflected light, often on a base-2 logarithmic scale.
        The term does not derive from a published standard, and has had several
        different meanings:
    */
    MINT32              i4LightValue_x10;

public:
    RAWIspCamInfo()
        : IspCamInfo()
        , u4ISOValue(0)
        , eIdx_ISO(eIDX_ISO_100)
        , eIdx_PCA_LUT(eIDX_PCA_LOW)
        //, eIdx_CCM_CCT(eIDX_CCM_CCT_BEGIN)
        , eIdx_Shading_CCT(eIDX_Shading_CCT_CWF)
        , rAWBInfo()
        , i4ZoomRatio_x100(0)
        , i4LightValue_x10(0)
    {}

public:
    void dump() const
    {
        IspCamInfo::dump();
        LOGD(
            "[RAWIspCamInfo][dump]"
            "(eIdx_ISO, u4ISOValue, i4ZoomRatio_x100, i4LightValue_x10)"
            "=(%d, %d, %d, %d, %d)"
            , eIdx_ISO, u4ISOValue, i4ZoomRatio_x100, i4LightValue_x10
        );
    }
};


/*******************************************************************************
*
*******************************************************************************/
struct YUVIspCamInfo : public IspCamInfo
{
public:
    YUVIspCamInfo()
        : IspCamInfo()
    {}
};


/*******************************************************************************
*
*******************************************************************************/
};  //  NSIspTuning
#endif //  _ISP_TUNING_CAM_INFO_H_

