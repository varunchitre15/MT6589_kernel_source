

//
#define LOG_TAG "IspTuningCustom_effect"
#ifndef ENABLE_MY_LOG
    #define ENABLE_MY_LOG       (0)
#endif
//
#include <utils/Errors.h>
#include <cutils/log.h>
//
#define USE_CUSTOM_ISP_TUNING
#include "isp_tuning.h"
//
//using namespace NS_MT6575ISP_EFFECT;
//
namespace NSIspTuning
{

#if 0
/*******************************************************************************
* Effect: MONO
*******************************************************************************/
template <>
MVOID
IspTuningCustom::
prepare_effect<MEFFECT_MONO>(ISP_EFFECT_T& rEffect)
{
    MY_LOG("[+prepare_effect] MEFFECT_MONO");
    //--------------------------------------------------------------------------
    //  Reference.
    ISP_EFFECT_YCCGO_T& rYCCGO = rEffect.yccgo;
    ISP_EFFECT_EDGE_T&  rEdge  = rEffect.edge;
    ISP_EFFECT_CCM_T&   rCCM   = rEffect.ccm;
    //--------------------------------------------------------------------------
    //  TODO:
    //  Begin your setting.
    //
    //  YCCGO
    rYCCGO.ctrl.val         = 0;
    rYCCGO.ctrl.bits.ENC2   = 1;
    rYCCGO.cfg1.bits.H12    = 0;
    rYCCGO.cfg1.bits.H11    = 0;

}


/*******************************************************************************
* Effect: SEPIA
*******************************************************************************/
template <>
MVOID
IspTuningCustom::
prepare_effect<MEFFECT_SEPIA>(ISP_EFFECT_T& rEffect)
{
    MY_LOG("[+prepare_effect] MEFFECT_SEPIA");
    //--------------------------------------------------------------------------
    //  Reference.
    ISP_EFFECT_YCCGO_T& rYCCGO = rEffect.yccgo;
    ISP_EFFECT_EDGE_T&  rEdge  = rEffect.edge;
    ISP_EFFECT_CCM_T&   rCCM   = rEffect.ccm;
    //--------------------------------------------------------------------------
    //  TODO:
    //  Begin your setting.
    //
    //  YCCGO
    rYCCGO.ctrl.val         = 0;
    rYCCGO.ctrl.bits.ENC1   = 1;
    rYCCGO.cfg1.bits.MU     = 98;
    rYCCGO.cfg1.bits.MV     = 158;

}


/*******************************************************************************
* Effect: AQUA
*******************************************************************************/
template <>
MVOID
IspTuningCustom::
prepare_effect<MEFFECT_AQUA>(ISP_EFFECT_T& rEffect)
{
    MY_LOG("[+prepare_effect] MEFFECT_AQUA");
    //--------------------------------------------------------------------------
    //  Reference.
    ISP_EFFECT_YCCGO_T& rYCCGO = rEffect.yccgo;
    ISP_EFFECT_EDGE_T&  rEdge  = rEffect.edge;
    ISP_EFFECT_CCM_T&   rCCM   = rEffect.ccm;
    //--------------------------------------------------------------------------
    //  TODO:
    //  Begin your setting.
    //
    //  YCCGO
    rYCCGO.ctrl.val         = 0;
    rYCCGO.ctrl.bits.ENC1   = 1;
    rYCCGO.cfg1.bits.MU     = 216;
    rYCCGO.cfg1.bits.MV     = 98;

}


/*******************************************************************************
* Effect: NEGATIVE
*******************************************************************************/
template <>
MVOID
IspTuningCustom::
prepare_effect<MEFFECT_NEGATIVE>(ISP_EFFECT_T& rEffect)
{
    MY_LOG("[+prepare_effect] MEFFECT_NEGATIVE");
    //--------------------------------------------------------------------------
    //  Reference.
    ISP_EFFECT_YCCGO_T& rYCCGO = rEffect.yccgo;
    ISP_EFFECT_EDGE_T&  rEdge  = rEffect.edge;
    ISP_EFFECT_CCM_T&   rCCM   = rEffect.ccm;
    //--------------------------------------------------------------------------
    //  TODO:
    //  Begin your setting.
    //
    //  EDGE
    rEdge.cpscon2.bits.OPRGM_IVT = 1;

}


/*******************************************************************************
* Effect: SOLARIZE
*******************************************************************************/
template <>
MVOID
IspTuningCustom::
prepare_effect<MEFFECT_SOLARIZE>(ISP_EFFECT_T& rEffect)
{
    MY_LOG("[+prepare_effect] MEFFECT_SOLARIZE");
    //--------------------------------------------------------------------------
    //  Reference.
    ISP_EFFECT_YCCGO_T& rYCCGO = rEffect.yccgo;
    ISP_EFFECT_EDGE_T&  rEdge  = rEffect.edge;
    ISP_EFFECT_CCM_T&   rCCM   = rEffect.ccm;
    //--------------------------------------------------------------------------
    //  TODO:
    //  Begin your setting.
    //
    //  YCCGO
    rYCCGO.ctrl.val     = 0x0000002b;
    rYCCGO.cfg1.val     = 0x80807f01;
    rYCCGO.cfg2.val     = 0x1020e0f0;
    rYCCGO.cfg3.val     = 0x20464846;
    rYCCGO.cfg4.val     = 0x00e00000;
    rYCCGO.cfg5.val     = 0x90000058;
    rYCCGO.cfg6.val     = 0xff00ff00;
    //
    //  EDGE
    rEdge.edgcore.val   = 0x001F1814;
    rEdge.edggain1.val  = 0x00000080;
    rEdge.edgvcon.val   = 0x231F0232;
    rEdge.ee_ctrl.val   = 0x005f1f1f;

}


/*******************************************************************************
* Effect: POSTERIZE
*******************************************************************************/
template <>
MVOID
IspTuningCustom::
prepare_effect<MEFFECT_POSTERIZE>(ISP_EFFECT_T& rEffect)
{
    MY_LOG("[+prepare_effect] MEFFECT_POSTERIZE");
    //--------------------------------------------------------------------------
    //  Reference.
    ISP_EFFECT_YCCGO_T& rYCCGO = rEffect.yccgo;
    ISP_EFFECT_EDGE_T&  rEdge  = rEffect.edge;
    ISP_EFFECT_CCM_T&   rCCM   = rEffect.ccm;
    //--------------------------------------------------------------------------
    //  TODO:
    //  Begin your setting.
    //
    //  TODO:
    //  Begin your setting.
    //
    //  YCCGO
    //  Set YCCGO Saturation Gain: (100,100,100,100,100)
    rYCCGO.ctrl.val         = 0;
    rYCCGO.ctrl.bits.ENC3   = 1;
    rYCCGO.cfg3.bits.G1     = 100;
    rYCCGO.cfg3.bits.G2     = 100;
    rYCCGO.cfg3.bits.G3     = 100;
    rYCCGO.cfg3.bits.G4     = 100;
    //
    //  EDGE
    rEdge.edgcore.bits.SPECIAL_EN   = 1;
    rEdge.edgcore.bits.EMBOSS2_EN   = 1;
    rEdge.edgcore.bits.EMBOSS1_EN   = 0;
    rEdge.edgcore.bits.COREH        = 0;

    rEdge.edggain1.bits.EGAINLINE   = 15;
    rEdge.edggain1.bits.KNEESEL     = 3;
    rEdge.edggain1.bits.OILEN       = 1;
    rEdge.edggain1.bits.EGAIN_VB    = 31;
    rEdge.edggain1.bits.EGAIN_H2    = 31;
    rEdge.edggain1.bits.EGAIN_H     = 15;
    rEdge.edggain1.bits.SPECIPONLY  = 0;
    rEdge.edggain1.bits.SPECIGAIN   = 1;

    rEdge.edgvcon.bits.E_TH1_V      = 35;

    rEdge.ee_ctrl.bits.RGBEDGE_EN   = 1;

}


/*******************************************************************************
* Effect: BLACKBOARD
*******************************************************************************/
template <>
MVOID
IspTuningCustom::
prepare_effect<MEFFECT_BLACKBOARD>(ISP_EFFECT_T& rEffect)
{
    MY_LOG("[+prepare_effect] MEFFECT_BLACKBOARD");
    //--------------------------------------------------------------------------
    //  Reference.
    ISP_EFFECT_YCCGO_T& rYCCGO = rEffect.yccgo;
    ISP_EFFECT_EDGE_T&  rEdge  = rEffect.edge;
    ISP_EFFECT_CCM_T&   rCCM   = rEffect.ccm;
    //--------------------------------------------------------------------------
    //  TODO:
    //  Begin your setting.
    //
    //  TODO:
    //  Begin your setting.
    //
    //  CCM
    rCCM.ccm1.val = 0x00000000;
    rCCM.ccm2.val = 0x00000000;
    rCCM.ccm3.val = 0x00000000;
    rCCM.ccm4.val = 0x00000000;
    rCCM.ccm5.val = 0x00000000;

    //
    //  YCCGO
    rYCCGO.ctrl.val         = 0;
    rYCCGO.ctrl.bits.ENC2   = 1;
    rYCCGO.cfg1.bits.H12    = 0;
    rYCCGO.cfg1.bits.H11    = 0;
    //
    //  EDGE
    rEdge.ed_ctrl.val               = 0x00000122;
    rEdge.ed_inter1.val             = 0x08000810;
    rEdge.ed_inter2.val             = 0x00000414;
    rEdge.ed_inter2.bits.THRE_LEDGE = 127;

    rEdge.edgcore.bits.SPECIAL_EN   = 1;
    rEdge.edgcore.bits.EMBOSS2_EN   = 1;
    rEdge.edgcore.bits.EMBOSS1_EN   = 1;    //
    rEdge.edgcore.bits.COREH        = 0;

    rEdge.edggain1.bits.EGAINLINE   = 0;    //
    rEdge.edggain1.bits.KNEESEL     = 3;
    rEdge.edggain1.bits.OILEN       = 0;    //
    rEdge.edggain1.bits.EGAIN_VB    = 31;
    rEdge.edggain1.bits.EGAIN_H2    = 31;
    rEdge.edggain1.bits.EGAIN_H     = 0;    //
    rEdge.edggain1.bits.SPECIPONLY  = 1;    //
    rEdge.edggain1.bits.SPECIGAIN   = 0;    //

    rEdge.edggain2.bits.SPECIINV    = 0;    //
    rEdge.edggain2.bits.SPECIABS    = 0;    //

    rEdge.edgvcon.bits.E_TH1_V      = 4;    //

    rEdge.cpscon2.bits.Y_EGAIN      = 15;   //
    rEdge.cpscon2.bits.OPRGM_IVT    = 0;    //  //

    rEdge.ee_ctrl.bits.YEDGE_EN     = 1;    //
    rEdge.ee_ctrl.bits.RGBEDGE_EN   = 1;

}


/*******************************************************************************
* Effect: WHITEBOARD
*******************************************************************************/
template <>
MVOID
IspTuningCustom::
prepare_effect<MEFFECT_WHITEBOARD>(ISP_EFFECT_T& rEffect)
{
    MY_LOG("[+prepare_effect] MEFFECT_WHITEBOARD");
    //--------------------------------------------------------------------------
    //  Reference.
    ISP_EFFECT_YCCGO_T& rYCCGO = rEffect.yccgo;
    ISP_EFFECT_EDGE_T&  rEdge  = rEffect.edge;
    ISP_EFFECT_CCM_T&   rCCM   = rEffect.ccm;
    //--------------------------------------------------------------------------
    //  TODO:
    //  Begin your setting.
    //
    //  TODO:
    //  Begin your setting.
    //
    //  CCM
    rCCM.ccm1.val = 0x00000000;
    rCCM.ccm2.val = 0x00000000;
    rCCM.ccm3.val = 0x00000000;
    rCCM.ccm4.val = 0x00000000;
    rCCM.ccm5.val = 0x00000000;

    //
    //  YCCGO
    rYCCGO.ctrl.val         = 0;
    rYCCGO.ctrl.bits.ENC2   = 1;
    rYCCGO.cfg1.bits.H12    = 0;
    rYCCGO.cfg1.bits.H11    = 0;
    //
    //  EDGE
    rEdge.ed_ctrl.val               = 0x00000122;
    rEdge.ed_inter1.val             = 0x08000810;
    rEdge.ed_inter2.val             = 0x00000414;
    rEdge.ed_inter2.bits.THRE_LEDGE = 127;

    rEdge.edgcore.bits.SPECIAL_EN   = 1;
    rEdge.edgcore.bits.EMBOSS2_EN   = 1;
    rEdge.edgcore.bits.EMBOSS1_EN   = 1;    //
    rEdge.edgcore.bits.COREH        = 0;

    rEdge.edggain1.bits.EGAINLINE   = 0;    //
    rEdge.edggain1.bits.KNEESEL     = 3;
    rEdge.edggain1.bits.OILEN       = 0;    //
    rEdge.edggain1.bits.EGAIN_VB    = 31;
    rEdge.edggain1.bits.EGAIN_H2    = 31;
    rEdge.edggain1.bits.EGAIN_H     = 0;    //
    rEdge.edggain1.bits.SPECIPONLY  = 1;    //
    rEdge.edggain1.bits.SPECIGAIN   = 0;    //

    rEdge.edggain2.bits.SPECIINV    = 0;    //
    rEdge.edggain2.bits.SPECIABS    = 0;    //

    rEdge.edgvcon.bits.E_TH1_V      = 4;    //

    rEdge.cpscon2.bits.Y_EGAIN      = 15;   //
    rEdge.cpscon2.bits.OPRGM_IVT    = 1;    //  //

    rEdge.ee_ctrl.bits.YEDGE_EN     = 1;    //
    rEdge.ee_ctrl.bits.RGBEDGE_EN   = 1;

}
#endif

/*******************************************************************************
*
*******************************************************************************/
};  //NSIspTuning


