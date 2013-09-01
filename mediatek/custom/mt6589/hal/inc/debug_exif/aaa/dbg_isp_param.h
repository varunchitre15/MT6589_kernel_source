
#ifndef _DBG_ISP_PARAM_H_
#define _DBG_ISP_PARAM_H_
/*******************************************************************************
*
*******************************************************************************/
#define ISP_DEBUG_KEYID 0xF4F5F6F7

namespace NSIspExifDebug
{

enum { IspDebugTagVersion = 0 };

enum IspDebugTagID
{
    IspTagVersion,
    //  RAWIspCamInfo
    IspProfile,
    SceneIdx,
    ISOValue,
    ISOIdx,
    ShadingIdx,
    LightValue_x10,
    // Effect
    EffectMode,
    // UserSelectLevel
    EdgeIdx,
    HueIdx,
    SatIdx,
    BrightIdx,
    ContrastIdx,
    //  Index
    IDX_OBC,
    IDX_BPC,
    IDX_NR1,
    IDX_LSC,
    IDX_CFA,
    IDX_LCE,
    IDX_GGM,
    IDX_ANR,
    IDX_CCR,
    IDX_PCA,
    IDX_EE,
    IDX_NR3D,
    IDX_MFB,
    // PCA slider
    PCA_SLIDER,    
    //
    // ISP enable (TOP)
    CAM_CTL_EN1,
    CAM_CTL_EN2,
    //
    //  OBC
    CAM_OBC_OFFST0,
    CAM_OBC_OFFST1,
    CAM_OBC_OFFST2,
    CAM_OBC_OFFST3,
    CAM_OBC_GAIN0,
    CAM_OBC_GAIN1,
    CAM_OBC_GAIN2,
    CAM_OBC_GAIN3,
    //
    //  BPC
    CAM_BPC_CON,
    CAM_BPC_CD1_1,
    CAM_BPC_CD1_2,
    CAM_BPC_CD1_3,
    CAM_BPC_CD1_4,
    CAM_BPC_CD1_5,
    CAM_BPC_CD1_6,
    CAM_BPC_CD2_1,
    CAM_BPC_CD2_2,
    CAM_BPC_CD2_3,
    CAM_BPC_CD0,
    CAM_BPC_DET,
    CAM_BPC_COR,
    CAM_BPC_TBLI1,
    CAM_BPC_TBLI2,
    //
    //  NR1
    CAM_NR1_CON,
    CAM_NR1_CT_CON,
    //
    //  LSC
    CAM_LSC_CTL1,
    CAM_LSC_CTL2,
    CAM_LSC_CTL3,
    CAM_LSC_LBLOCK,
    CAM_LSC_RATIO,
    CAM_LSC_GAIN_TH,
    //
    //  PGN
    CAM_PGN_SATU01,
    CAM_PGN_SATU23,
    CAM_PGN_GAIN01,
    CAM_PGN_GAIN23,
    CAM_PGN_OFFS01,
    CAM_PGN_OFFS23,
    //
    //  CFA
    CAM_CFA_BYPASS,
    CAM_CFA_ED_F,
    CAM_CFA_ED_NYQ,
    CAM_CFA_ED_STEP,
    CAM_CFA_RGB_HF,
    CAM_CFA_BW,
    CAM_CFA_F1_ACT,
    CAM_CFA_F2_ACT,
    CAM_CFA_F3_ACT,
    CAM_CFA_F4_ACT,
    CAM_CFA_F1_L,
    CAM_CFA_F2_L,
    CAM_CFA_F3_L,
    CAM_CFA_F4_L,
    CAM_CFA_HF_RB,
    CAM_CFA_HF_GAIN,
    CAM_CFA_HF_COMP,
    CAM_CFA_HF_CORING_TH,
    CAM_CFA_ACT_LUT,
    CAM_CFA_SPARE,
    CAM_CFA_BB,
    //
    //  G2G (CCM)
    CAM_G2G_CONV0A,
    CAM_G2G_CONV0B,
    CAM_G2G_CONV1A,
    CAM_G2G_CONV1B,
    CAM_G2G_CONV2A,
    CAM_G2G_CONV2B,
    //
    //  G2C
    CAM_G2C_CONV_0A,
    CAM_G2C_CONV_0B,
    CAM_G2C_CONV_1A,
    CAM_G2C_CONV_1B,
    CAM_G2C_CONV_2A,
    CAM_G2C_CONV_2B,
    //
    //  ANR
    CAM_ANR_CON1,
    CAM_ANR_CON2,
    CAM_ANR_CON3,
    CAM_ANR_YAD1,
    CAM_ANR_YAD2,
    CAM_ANR_4LUT1,
    CAM_ANR_4LUT2,
    CAM_ANR_4LUT3,
    CAM_ANR_PTY,
    CAM_ANR_CAD,
    CAM_ANR_PTC,
    CAM_ANR_LCE1,
    CAM_ANR_LCE2,
    CAM_ANR_HP1,
    CAM_ANR_HP2,
    CAM_ANR_HP3,
    CAM_ANR_ACTY,
    CAM_ANR_ACTC,
    //
    //  CCR
    CAM_CCR_CON,
    CAM_CCR_YLUT,
    CAM_CCR_UVLUT,
    CAM_CCR_YLUT2,
    //
    //  PCA
    CAM_PCA_CON1,
    CAM_PCA_CON2,
    //
    //  EE
    CAM_SEEE_SRK_CTRL,
    CAM_SEEE_CLIP_CTRL,
    CAM_SEEE_HP_CTRL1,
    CAM_SEEE_HP_CTRL2,
    CAM_SEEE_ED_CTRL1,
    CAM_SEEE_ED_CTRL2,
    CAM_SEEE_ED_CTRL3,
    CAM_SEEE_ED_CTRL4,
    CAM_SEEE_ED_CTRL5,
    CAM_SEEE_ED_CTRL6,
    CAM_SEEE_ED_CTRL7,
    CAM_SEEE_EE_LINK1,
    CAM_SEEE_EE_LINK2,
    CAM_SEEE_EE_LINK3,
    CAM_SEEE_EE_LINK4,
    CAM_SEEE_EE_LINK5,
    //
    // SE
    CAM_SEEE_EDGE_CTRL,
    CAM_SEEE_Y_CTRL,
    CAM_SEEE_EDGE_CTRL1,
    CAM_SEEE_EDGE_CTRL2,
    CAM_SEEE_EDGE_CTRL3,
    CAM_SEEE_SPECIAL_CTRL,
    CAM_SEEE_CORE_CTRL1,
    CAM_SEEE_CORE_CTRL2,
    //
    // NR3D
    CAM_NR3D_BLEND,
    CAM_NR3D_SKIP_KEY,
    CAM_NR3D_FBCNT_OFF,
    CAM_NR3D_FBCNT_SIZ,
    CAM_NR3D_FB_COUNT,
    CAM_NR3D_LIMIT_CPX,
    CAM_NR3D_LIMIT_Y_CON1,
    CAM_NR3D_LIMIT_Y_CON2,
    CAM_NR3D_LIMIT_Y_CON3,
    CAM_NR3D_LIMIT_U_CON1,
    CAM_NR3D_LIMIT_U_CON2,
    CAM_NR3D_LIMIT_U_CON3,
    CAM_NR3D_LIMIT_V_CON1,
    CAM_NR3D_LIMIT_V_CON2,
    CAM_NR3D_LIMIT_V_CON3,
    //
    // MFB
    CAM_MFB_CON,
    CAM_MFB_LL,
    //
    //  Common
    COMM_00,
    COMM_01,
    COMM_02,
    COMM_03,
    COMM_04,
    COMM_05,
    COMM_06,
    COMM_07,
    COMM_08,
    COMM_09,
    COMM_10,
    COMM_11,
    COMM_12,
    COMM_13,
    COMM_14,
    COMM_15,
    COMM_16,
    COMM_17,
    COMM_18,
    COMM_19,
    COMM_20,
    COMM_21,
    COMM_22,
    COMM_23,
    COMM_24,
    COMM_25,
    COMM_26,
    COMM_27,
    COMM_28,
    COMM_29,
    COMM_30,
    COMM_31,
    COMM_32,
    COMM_33,
    COMM_34,
    COMM_35,
    COMM_36,
    COMM_37,
    COMM_38,
    COMM_39,
    COMM_40,
    COMM_41,
    COMM_42,
    COMM_43,
    COMM_44,
    COMM_45,
    COMM_46,
    COMM_47,
    COMM_48,
    COMM_49,
    COMM_50,
    COMM_51,
    COMM_52,
    COMM_53,
    COMM_54,
    COMM_55,
    COMM_56,
    COMM_57,
    COMM_58,
    COMM_59,
    COMM_60,
    COMM_61,
    COMM_62,
    COMM_63,
    //
};

enum
{
    // CAM_CTL_EN
    CAM_CTL_EN_Begin        =   CAM_CTL_EN1,
    //  OBC
    CAM_OBC_Begin           =   CAM_OBC_OFFST0,
    //  BPC
    CAM_BPC_Begin           =   CAM_BPC_CON,
    //  NR1
    CAM_NR1_Begin           =   CAM_NR1_CON,
    //  LSC
    CAM_LSC_Begin           =   CAM_LSC_CTL1,
    //  PGN
    CAM_PGN_Begin           =   CAM_PGN_SATU01,
    //  CFA
    CAM_CFA_Begin           =   CAM_CFA_BYPASS,
    //  G2G
    CAM_G2G_Begin           =   CAM_G2G_CONV0A,
    //  G2C
    CAM_G2C_Begin           =   CAM_G2C_CONV_0A,
    //  ANR
    CAM_ANR_Begin           =   CAM_ANR_CON1,
    //  CCR
    CAM_CCR_Begin           =   CAM_CCR_CON,
    //  PCA
    CAM_PCA_Begin           =   CAM_PCA_CON1,
    //  EE
    CAM_EE_Begin            =   CAM_SEEE_SRK_CTRL,
    //  SE
    CAM_SE_Begin            =   CAM_SEEE_EDGE_CTRL,
    //  NR3D
    CAM_NR3D_Begin          =   CAM_NR3D_BLEND,
    //  MFB
    CAM_MFB_Begin           =   CAM_MFB_CON,
    //  Common
    COMM_Begin              =   COMM_00,
    //
    //
    TagID_Total_Num         =   COMM_63 + 1
};

struct IspDebugTag
{
    MUINT32     u4ID;
    MUINT32     u4Val;
};

typedef struct IspExifDebugInfo
{
    struct  Header
    {
        MUINT32     u4KeyID;
        MUINT32     u4ModuleCount;
        MUINT32     u4DebugInfoOffset;
        MUINT32     u4TableInfoOffset;
    }   hdr;

    struct IspDebugInfo
    {
        IspDebugTag     tags[TagID_Total_Num];
    } debugInfo;

    struct IspTableInfo
    {
        MUINT32         GGM[288];
        MUINT32         PCA[180];
    } tableInfo;

} IspExifDebugInfo_T;


};  //  namespace NSIspExifDebug
/*******************************************************************************
*
*******************************************************************************/
namespace NSIspTuning
{


/*******************************************************************************
*
*******************************************************************************/
template <MUINT32 total_module, MUINT32 tag_module>
struct ModuleNum
{
/*
    |   8  |       8      |   8  |     8      |
    | 0x00 | total_module | 0x00 | tag_module |
*/
    enum
    {
        val = ((total_module & 0xFF) << 16) | ((tag_module & 0xFF))
    };
};


template <MUINT32 module_id, MUINT32 tag_id, MUINT32 line_keep = 0>
struct ModuleTag
{
/*
    |     8     |      1    |   7  |    16    |
    | module_id | line_keep | 0x00 |  tag_id  |
*/
    enum
    {
        val = ((module_id & 0xFF) << 24)
            | ((line_keep & 0x01) << 23)
            | ((tag_id  & 0xFFFF) << 0)
    };
};


inline MUINT32 getModuleTag(MUINT32 module_id, MUINT32 tag_id, MUINT32 line_keep = 0)
{
/*
    |     8     |      1    |   7  |    16    |
    | module_id | line_keep | 0x00 |  tag_id  |
*/
    return  ((module_id & 0xFF) << 24)
          | ((line_keep & 0x01) << 23)
          | ((tag_id  & 0xFFFF) << 0)
            ;
}


enum { EModuleID_IspDebug = 0x0004 };
template <MUINT32 tag_id, MUINT32 line_keep = 0>
struct IspTag
{
    enum { val = ModuleTag<EModuleID_IspDebug, tag_id, line_keep>::val };
};


inline MUINT32 getIspTag(MUINT32 tag_id, MUINT32 line_keep = 0)
{
    return  getModuleTag(EModuleID_IspDebug, tag_id, line_keep);
}


//  Default of IspExifDebugInfo::Header
static NSIspExifDebug::IspExifDebugInfo::Header const g_rIspExifDebugInfoHdr =
{
    u4KeyID:            ISP_DEBUG_KEYID,
    u4ModuleCount:      ModuleNum<2, 1>::val,
    u4DebugInfoOffset:  sizeof(NSIspExifDebug::IspExifDebugInfo::Header),
    u4TableInfoOffset:  sizeof(NSIspExifDebug::IspExifDebugInfo::Header) + sizeof(NSIspExifDebug::IspExifDebugInfo::IspDebugInfo)
};

};  //  namespace NSIspExifDebug
#endif // _DBG_ISP_PARAM_H_

