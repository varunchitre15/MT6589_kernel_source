

#define LOG_TAG "isp_tuning_user"

#ifndef ENABLE_MY_LOG
    #define ENABLE_MY_LOG       (0)
#endif

#include <aaa_types.h>
#include <aaa_log.h>

#include <camera_custom_nvram.h>
#include <isp_tuning.h>
#include <camera_feature.h>
#include <awb_param.h>
#include <ae_param.h>
#include <af_param.h>
#include <flash_param.h>
#include <isp_tuning_cam_info.h>
#include <isp_tuning_idx.h>
#include <isp_tuning_custom.h>
#include <math.h>

namespace NSIspTuning
{

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  EE
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
MVOID
IspTuningCustom::userSetting_EE(
    EIndex_Isp_Edge_T eIdx_Edge,
    ISP_NVRAM_EE_T& rEE
)
{
    MINT32 i4Slider, dX, dY;

    MY_LOG("eIdx_Edge : %d\n",eIdx_Edge);

    switch ( eIdx_Edge )
    {
    case ISP_EDGE_LOW:
        i4Slider = -4; // Soft EE
        break;
    case ISP_EDGE_HIGH:
        i4Slider = 4; //Texture EE
        break;
    case ISP_EDGE_MIDDLE: // Normal EE
    default:
        i4Slider = 0;
        break;
    }

    if (i4Slider == 0) // Normal EE: do nothing
        return;

    // Get normal EE parameter
    MINT32 LOCK_EE_Y1 = rEE.ed_ctrl1.bits.USM_ED_Y1;   // 0~1023
	MINT32 LOCK_EE_Y2 = rEE.ed_ctrl2.bits.USM_ED_Y2;   // 0~1023
	MINT32 LOCK_EE_Y3 = rEE.ed_ctrl3.bits.USM_ED_Y3;   // 0~1023
	MINT32 LOCK_EE_Y4 = rEE.ed_ctrl4.bits.USM_ED_Y4;   // 0~1023
	MINT32 LOCK_EE_S5 = rEE.ed_ctrl5.bits.USM_ED_S5;   // 0~255
	MINT32 LOCK_EE_CLIP = rEE.clip_ctrl.bits.USM_CLIP; // 0~255
	MINT32 LOCK_EE_TH_OV = rEE.ed_ctrl6.bits.USM_ED_TH_OVER; // 0~255
	MINT32 LOCK_EE_TH_UN = rEE.ed_ctrl6.bits.USM_ED_TH_UNDER; // 0~255
	MINT32 USM_ED_Y1, USM_ED_Y2, USM_ED_Y3, USM_ED_Y4, USM_CLIP, USM_ED_TH_OVER, USM_ED_TH_UNDER;

    // Determine user setting EE parameter
    if (i4Slider < 0) {  // Soft EE
        USM_ED_Y1 = LIMIT(static_cast<MINT32>(static_cast<MDOUBLE>(LOCK_EE_Y1) * (1 + 0.5 * i4Slider) + 0.5), 0, 1023);
        USM_ED_Y2 = LIMIT(static_cast<MINT32>(static_cast<MDOUBLE>(LOCK_EE_Y2) * (1 + 0.3 * i4Slider) + 0.5), 0, 1023);
    }
    else { // Texture EE
        USM_ED_Y1 = LIMIT(static_cast<MINT32>(static_cast<MDOUBLE>(LOCK_EE_Y1) * (1 + 1.0 * i4Slider) + 0.5), 0, 1023);
        USM_ED_Y2 = LIMIT(static_cast<MINT32>(static_cast<MDOUBLE>(LOCK_EE_Y2) * (1 + 1.5 * i4Slider) + 0.5), 0, 1023);
    }

	USM_ED_Y3 = LIMIT(static_cast<MINT32>(static_cast<MDOUBLE>(LOCK_EE_Y3) * (1 + 0.2 * i4Slider) + 0.5), 0, 1023);
	USM_ED_Y4 = LIMIT(static_cast<MINT32>(static_cast<MDOUBLE>(LOCK_EE_Y4) * (1 + 0.2 * i4Slider) + 0.5), 0, 1023);
	USM_CLIP = LIMIT(static_cast<MINT32>(static_cast<MDOUBLE>(LOCK_EE_CLIP) * (1 + 0.2 * i4Slider) + 0.5), 0, 255);
	USM_ED_TH_OVER = LIMIT(static_cast<MINT32>(static_cast<MDOUBLE>(LOCK_EE_TH_OV) * (1 + 0.2 * i4Slider) + 0.5), 0, 255);
	USM_ED_TH_UNDER = LIMIT(static_cast<MINT32>(static_cast<MDOUBLE>(LOCK_EE_TH_UN) * (1 + 0.2 * i4Slider) + 0.5), 0, 255);

	MINT32 EE_LCE_THO_1 = LIMIT(static_cast<MINT32>(USM_ED_TH_OVER - 10), 0, 255);
	MINT32 EE_LCE_THO_2 = LIMIT(static_cast<MINT32>(USM_ED_TH_OVER - 20), 0, 255);
	MINT32 EE_LCE_THO_3 = LIMIT(static_cast<MINT32>(USM_ED_TH_OVER - 30), 0, 255);
	MINT32 EE_LCE_THU_1 = LIMIT(static_cast<MINT32>(USM_ED_TH_UNDER - 10), 0, 255);
	MINT32 EE_LCE_THU_2 = LIMIT(static_cast<MINT32>(USM_ED_TH_UNDER - 20), 0, 255);
	MINT32 EE_LCE_THU_3 = LIMIT(static_cast<MINT32>(USM_ED_TH_UNDER - 30), 0, 255);

    // USM_ED_S1
    MINT32 USM_ED_X1 = rEE.ed_ctrl1.bits.USM_ED_X1;
    MINT32 USM_ED_S1;

    if (USM_ED_X1 == 0) USM_ED_S1 = 0;
    else USM_ED_S1 = static_cast<MINT32>(static_cast<MDOUBLE>(USM_ED_Y1) / USM_ED_X1 + 0.5);

    if (USM_ED_S1 > 127) USM_ED_S1 = 127;

    // EE_LCE_S1_1
    MINT32 EE_LCE_X1_1 = rEE.ee_link1.bits.EE_LCE_X1_1;
    MINT32 EE_LCE_S1_1;

    if (EE_LCE_X1_1 == 0) EE_LCE_S1_1 = 0;
    else EE_LCE_S1_1 = static_cast<MINT32>(static_cast<MDOUBLE>(USM_ED_Y1) / EE_LCE_X1_1 + 0.5);

    if (EE_LCE_S1_1 > 127) EE_LCE_S1_1 = 127;

    // EE_LCE_S1_2
    MINT32 EE_LCE_X1_2 = rEE.ee_link2.bits.EE_LCE_X1_2;
    MINT32 EE_LCE_S1_2;

    if (EE_LCE_X1_2 == 0) EE_LCE_S1_2 = 0;
    else EE_LCE_S1_2 = static_cast<MINT32>(static_cast<MDOUBLE>(USM_ED_Y1) / EE_LCE_X1_2 + 0.5);

    if(EE_LCE_S1_2 > 127) EE_LCE_S1_2 = 127;

    // EE_LCE_S1_3
    MINT32 EE_LCE_X1_3 = rEE.ee_link3.bits.EE_LCE_X1_3;
    MINT32 EE_LCE_S1_3;

    if (EE_LCE_X1_3 == 0) EE_LCE_S1_3 = 0;
    else EE_LCE_S1_3 = static_cast<MINT32>(static_cast<MDOUBLE>(USM_ED_Y1) / EE_LCE_X1_3 + 0.5);

    if (EE_LCE_S1_3 > 127) EE_LCE_S1_3 = 127;

    // USM_ED_S2
    MINT32 USM_ED_X2 = rEE.ed_ctrl2.bits.USM_ED_X2;
    MINT32 USM_ED_S2;
    // EE_LCE_S2_1
    MINT32 EE_LCE_S2_1;
    // EE_LCE_S2_2
    MINT32 EE_LCE_S2_2;
    // EE_LCE_S2_3
    MINT32 EE_LCE_S2_3;

    dY = USM_ED_Y2 - USM_ED_Y1;
    if (dY > 0){
        // USM_ED_S2
        dX = USM_ED_X2 - USM_ED_X1;
        if (dX == 0) USM_ED_S2 = 0;
        else USM_ED_S2 = static_cast<MINT32>(static_cast<MDOUBLE>(dY) / dX + 0.5);

        // EE_LCE_S2_1
        dX = USM_ED_X2 - EE_LCE_X1_1;
        if (dX == 0) EE_LCE_S2_1 = 0;
        else EE_LCE_S2_1 = static_cast<MINT32>(static_cast<MDOUBLE>(dY) / dX + 0.5);

        // EE_LCE_S2_2
        dX = USM_ED_X2 - EE_LCE_X1_2;
        if (dX == 0) EE_LCE_S2_2 = 0;
        else EE_LCE_S2_2 = static_cast<MINT32>(static_cast<MDOUBLE>(dY) / dX + 0.5);

        // EE_LCE_S2_3
        dX = USM_ED_X2 - EE_LCE_X1_3;
        if (dX == 0) EE_LCE_S2_3 = 0;
        else EE_LCE_S2_3 = static_cast<MINT32>(static_cast<MDOUBLE>(dY) / dX + 0.5);
    }
    else {
        // USM_ED_S2
        dX = USM_ED_X2 - USM_ED_X1;
        if (dX == 0) USM_ED_S2 = 0;
        else USM_ED_S2 = static_cast<MINT32>(static_cast<MDOUBLE>(dY) / dX - 0.5);

        // EE_LCE_S2_1
        dX = USM_ED_X2 - EE_LCE_X1_1;
        if (dX == 0) EE_LCE_S2_1 = 0;
        else EE_LCE_S2_1 = static_cast<MINT32>(static_cast<MDOUBLE>(dY) / dX  - 0.5);

        // EE_LCE_S2_2
        dX = USM_ED_X2 - EE_LCE_X1_2;
        if (dX == 0) EE_LCE_S2_2 = 0;
        else EE_LCE_S2_2 = static_cast<MINT32>(static_cast<MDOUBLE>(dY) / dX - 0.5);

        // EE_LCE_S2_3
        dX = USM_ED_X2 - EE_LCE_X1_3;
        if (dX == 0) EE_LCE_S2_3 = 0;
        else EE_LCE_S2_3 = static_cast<MINT32>(static_cast<MDOUBLE>(dY) / dX - 0.5);
    }

    USM_ED_S2 = LIMIT(USM_ED_S2, -127, 127);
    EE_LCE_S2_1 = LIMIT(EE_LCE_S2_1, -127, 127);
    EE_LCE_S2_2 = LIMIT(EE_LCE_S2_2, -127, 127);
    EE_LCE_S2_3 = LIMIT(EE_LCE_S2_3, -127, 127);

    // USM_ED_S3
    MINT32 USM_ED_X3 = rEE.ed_ctrl3.bits.USM_ED_X3;
    MINT32 USM_ED_S3;
    dX = USM_ED_X3 - USM_ED_X2;
    dY = USM_ED_Y3 - USM_ED_Y2;
    if (dY > 0) {
        if (dX == 0) USM_ED_S3 = 0;
        else USM_ED_S3 = static_cast<MINT32>(static_cast<MDOUBLE>(dY) / dX + 0.5);
    }
    else {
        if (dX == 0) USM_ED_S3 = 0;
        else USM_ED_S3 = static_cast<MINT32>(static_cast<MDOUBLE>(dY) / dX - 0.5);
    }

    USM_ED_S3 = LIMIT(USM_ED_S3, -127, 127);

    // USM_ED_S4
    MINT32 USM_ED_X4 = rEE.ed_ctrl4.bits.USM_ED_X4;
    MINT32 USM_ED_S4;
    dX = USM_ED_X4 - USM_ED_X3;
    dY = USM_ED_Y4 - USM_ED_Y3;
    if (dY > 0){
        if (dX == 0) USM_ED_S4 = 0;
        else USM_ED_S4 = static_cast<MINT32>(static_cast<MDOUBLE>(dY) / dX + 0.5);
    }
    else{
        if (dX == 0) USM_ED_S4 = 0;
        else USM_ED_S4 = static_cast<MINT32>(static_cast<MDOUBLE>(dY) / dX - 0.5);
    }

    USM_ED_S4 = LIMIT(USM_ED_S4, -127, 127);

    // USM_ED_S5
	MINT32 USM_ED_S5;
	dX = 255 - USM_ED_X4;
	if (LOCK_EE_S5 < 128){
		if (dX == 0) USM_ED_S5 = 0;
		else USM_ED_S5 = static_cast<MINT32>(static_cast<MDOUBLE>(LOCK_EE_S5) * (1 + 0.2 * i4Slider) + 0.5);
    }
    else {
		if (dX == 0) USM_ED_S5 = 0;
		else USM_ED_S5 = static_cast<MINT32>(static_cast<MDOUBLE>(LOCK_EE_S5 - 256) * (1 + 0.2 * i4Slider) - 0.5);
    }

    USM_ED_S5 = LIMIT(USM_ED_S5, -127, 127);


    MY_LOG("[userSetting_EE] old\nX1, Y1, S1: %3d, %3d, %3d, 0x%08x\nX2, Y2, S2: %3d, %3d, %3d, 0x%08x\nX3, Y3, S3: %3d, %3d, %3d, 0x%08x\nX4, Y4, S4: %3d, %3d, %3d, 0x%08x\nS5: %3d, 0x%08x\nCLIP: %3d, 0x%08x\nTHO, THU: %3d, %3d, 0x%08x\n",
                               rEE.ed_ctrl1.bits.USM_ED_X1,
                               rEE.ed_ctrl1.bits.USM_ED_Y1,
                               rEE.ed_ctrl1.bits.USM_ED_S1,
							   rEE.ed_ctrl1.val,
                               rEE.ed_ctrl2.bits.USM_ED_X2,
                               rEE.ed_ctrl2.bits.USM_ED_Y2,
                               rEE.ed_ctrl2.bits.USM_ED_S2,
							   rEE.ed_ctrl2.val,
                               rEE.ed_ctrl3.bits.USM_ED_X3,
                               rEE.ed_ctrl3.bits.USM_ED_Y3,
                               rEE.ed_ctrl3.bits.USM_ED_S3,
							   rEE.ed_ctrl3.val,
                               rEE.ed_ctrl4.bits.USM_ED_X4,
                               rEE.ed_ctrl4.bits.USM_ED_Y4,
                               rEE.ed_ctrl4.bits.USM_ED_S4,
							   rEE.ed_ctrl4.val,
                               rEE.ed_ctrl5.bits.USM_ED_S5,
							   rEE.ed_ctrl5.val,
							   rEE.clip_ctrl.bits.USM_CLIP,
							   rEE.clip_ctrl.val,
							   rEE.ed_ctrl6.bits.USM_ED_TH_OVER,
							   rEE.ed_ctrl6.bits.USM_ED_TH_UNDER,
							   rEE.ed_ctrl6.val
                               );

    MY_LOG("[userSetting_EE] old\nLCE_X1_1, LCE_S1_1, LCE_S2_1: %3d, %3d, %3d, 0x%08x\nLCE_X1_2, LCE_S1_2, LCE_S2_2: %3d, %3d, %3d, 0x%08x\nLCE_X1_3, LCE_S1_3, LCE_S2_3: %3d, %3d, %3d, 0x%08x\nLCE_THO_1, LCE_THU_1, LCE_THO_2, LCE_THU_2: %3d, %3d, %3d, %3d, 0x%08x\nLCE_THO_3, LCE_THU_3: %3d, %3d, 0x%08x\n",
                               rEE.ee_link1.bits.EE_LCE_X1_1,
                               rEE.ee_link1.bits.EE_LCE_S1_1,
                               rEE.ee_link1.bits.EE_LCE_S2_1,
							   rEE.ee_link1.val,
                               rEE.ee_link2.bits.EE_LCE_X1_2,
                               rEE.ee_link2.bits.EE_LCE_S1_2,
                               rEE.ee_link2.bits.EE_LCE_S2_2,
							   rEE.ee_link2.val,
                               rEE.ee_link3.bits.EE_LCE_X1_3,
                               rEE.ee_link3.bits.EE_LCE_S1_3,
                               rEE.ee_link3.bits.EE_LCE_S2_3,
							   rEE.ee_link3.val,
							   rEE.ee_link4.bits.EE_LCE_THO_1,
							   rEE.ee_link4.bits.EE_LCE_THU_1,
							   rEE.ee_link4.bits.EE_LCE_THO_2,
							   rEE.ee_link4.bits.EE_LCE_THU_2,
							   rEE.ee_link4.val,
							   rEE.ee_link5.bits.EE_LCE_THO_3,
							   rEE.ee_link5.bits.EE_LCE_THU_3,
							   rEE.ee_link5.val
                               );

    // Write back
    rEE.ed_ctrl1.bits.USM_ED_Y1 = static_cast<MUINT32>(USM_ED_Y1);
    rEE.ed_ctrl2.bits.USM_ED_Y2 = static_cast<MUINT32>(USM_ED_Y2);
    rEE.ed_ctrl3.bits.USM_ED_Y3 = static_cast<MUINT32>(USM_ED_Y3);
    rEE.ed_ctrl4.bits.USM_ED_Y4 = static_cast<MUINT32>(USM_ED_Y4);
    rEE.clip_ctrl.bits.USM_CLIP = static_cast<MUINT32>(USM_CLIP);
    rEE.ed_ctrl6.bits.USM_ED_TH_OVER = static_cast<MUINT32>(USM_ED_TH_OVER);
    rEE.ed_ctrl6.bits.USM_ED_TH_UNDER = static_cast<MUINT32>(USM_ED_TH_UNDER);

    rEE.ed_ctrl1.bits.USM_ED_S1 = (USM_ED_S1 >= 0) ? static_cast<MUINT32>(USM_ED_S1) : static_cast<MUINT32>(256 + USM_ED_S1);
    rEE.ed_ctrl2.bits.USM_ED_S2 = (USM_ED_S2 >= 0) ? static_cast<MUINT32>(USM_ED_S2) : static_cast<MUINT32>(256 + USM_ED_S2);
    rEE.ed_ctrl3.bits.USM_ED_S3 = (USM_ED_S3 >= 0) ? static_cast<MUINT32>(USM_ED_S3) : static_cast<MUINT32>(256 + USM_ED_S3);
    rEE.ed_ctrl4.bits.USM_ED_S4 = (USM_ED_S4 >= 0) ? static_cast<MUINT32>(USM_ED_S4) : static_cast<MUINT32>(256 + USM_ED_S4);
    rEE.ed_ctrl5.bits.USM_ED_S5 = (USM_ED_S5 >= 0) ? static_cast<MUINT32>(USM_ED_S5) : static_cast<MUINT32>(256 + USM_ED_S5);

    rEE.ee_link1.bits.EE_LCE_S1_1 = (EE_LCE_S1_1 >= 0) ? static_cast<MUINT32>(EE_LCE_S1_1) : static_cast<MUINT32>(256 + EE_LCE_S1_1);
    rEE.ee_link1.bits.EE_LCE_S2_1 = (EE_LCE_S2_1 >= 0) ? static_cast<MUINT32>(EE_LCE_S2_1) : static_cast<MUINT32>(256 + EE_LCE_S2_1);
    rEE.ee_link2.bits.EE_LCE_S1_2 = (EE_LCE_S1_2 >= 0) ? static_cast<MUINT32>(EE_LCE_S1_2) : static_cast<MUINT32>(256 + EE_LCE_S1_2);
    rEE.ee_link2.bits.EE_LCE_S2_2 = (EE_LCE_S2_2 >= 0) ? static_cast<MUINT32>(EE_LCE_S2_2) : static_cast<MUINT32>(256 + EE_LCE_S2_2);
    rEE.ee_link3.bits.EE_LCE_S1_3 = (EE_LCE_S1_3 >= 0) ? static_cast<MUINT32>(EE_LCE_S1_3) : static_cast<MUINT32>(256 + EE_LCE_S1_3);
    rEE.ee_link3.bits.EE_LCE_S2_3 = (EE_LCE_S2_3 >= 0) ? static_cast<MUINT32>(EE_LCE_S2_3) : static_cast<MUINT32>(256 + EE_LCE_S2_3);

	rEE.ee_link4.bits.EE_LCE_THO_1 = EE_LCE_THO_1;
	rEE.ee_link4.bits.EE_LCE_THO_2 = EE_LCE_THO_2;
	rEE.ee_link5.bits.EE_LCE_THO_3 = EE_LCE_THO_3;
	rEE.ee_link4.bits.EE_LCE_THU_1 = EE_LCE_THU_1;
	rEE.ee_link4.bits.EE_LCE_THU_2 = EE_LCE_THU_2;
	rEE.ee_link5.bits.EE_LCE_THU_3 = EE_LCE_THU_3;

	MY_LOG("[userSetting_EE] new\nX1, Y1, S1: %3d, %3d, %3d, 0x%08x\nX2, Y2, S2: %3d, %3d, %3d, 0x%08x\nX3, Y3, S3: %3d, %3d, %3d, 0x%08x\nX4, Y4, S4: %3d, %3d, %3d, 0x%08x\nS5: %3d, 0x%08x\nCLIP: %3d, 0x%08x\nTHO, THU: %3d, %3d, 0x%08x\n",
                               rEE.ed_ctrl1.bits.USM_ED_X1,
                               rEE.ed_ctrl1.bits.USM_ED_Y1,
                               rEE.ed_ctrl1.bits.USM_ED_S1,
							   rEE.ed_ctrl1.val,
                               rEE.ed_ctrl2.bits.USM_ED_X2,
                               rEE.ed_ctrl2.bits.USM_ED_Y2,
                               rEE.ed_ctrl2.bits.USM_ED_S2,
							   rEE.ed_ctrl2.val,
                               rEE.ed_ctrl3.bits.USM_ED_X3,
                               rEE.ed_ctrl3.bits.USM_ED_Y3,
                               rEE.ed_ctrl3.bits.USM_ED_S3,
							   rEE.ed_ctrl3.val,
                               rEE.ed_ctrl4.bits.USM_ED_X4,
                               rEE.ed_ctrl4.bits.USM_ED_Y4,
                               rEE.ed_ctrl4.bits.USM_ED_S4,
							   rEE.ed_ctrl4.val,
                               rEE.ed_ctrl5.bits.USM_ED_S5,
							   rEE.ed_ctrl5.val,
							   rEE.clip_ctrl.bits.USM_CLIP,
							   rEE.clip_ctrl.val,
							   rEE.ed_ctrl6.bits.USM_ED_TH_OVER,
							   rEE.ed_ctrl6.bits.USM_ED_TH_UNDER,
							   rEE.ed_ctrl6.val
                               );

	MY_LOG("[userSetting_EE] new\nLCE_X1_1, LCE_S1_1, LCE_S2_1: %3d, %3d, %3d, 0x%08x\nLCE_X1_2, LCE_S1_2, LCE_S2_2: %3d, %3d, %3d, 0x%08x\nLCE_X1_3, LCE_S1_3, LCE_S2_3: %3d, %3d, %3d, 0x%08x\nLCE_THO_1, LCE_THU_1, LCE_THO_2, LCE_THU_2: %3d, %3d, %3d, %3d, 0x%08x\nLCE_THO_3, LCE_THU_3: %3d, %3d, 0x%08x\n",
                               rEE.ee_link1.bits.EE_LCE_X1_1,
                               rEE.ee_link1.bits.EE_LCE_S1_1,
                               rEE.ee_link1.bits.EE_LCE_S2_1,
							   rEE.ee_link1.val,
                               rEE.ee_link2.bits.EE_LCE_X1_2,
                               rEE.ee_link2.bits.EE_LCE_S1_2,
                               rEE.ee_link2.bits.EE_LCE_S2_2,
							   rEE.ee_link2.val,
                               rEE.ee_link3.bits.EE_LCE_X1_3,
                               rEE.ee_link3.bits.EE_LCE_S1_3,
                               rEE.ee_link3.bits.EE_LCE_S2_3,
							   rEE.ee_link3.val,
							   rEE.ee_link4.bits.EE_LCE_THO_1,
							   rEE.ee_link4.bits.EE_LCE_THU_1,
							   rEE.ee_link4.bits.EE_LCE_THO_2,
							   rEE.ee_link4.bits.EE_LCE_THU_2,
							   rEE.ee_link4.val,
							   rEE.ee_link5.bits.EE_LCE_THO_3,
							   rEE.ee_link5.bits.EE_LCE_THU_3,
							   rEE.ee_link5.val
                               );

}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// HSBC + Effect
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#define PI (3.1415926)
#define max(x1,x2) ((x1) > (x2) ? (x1):(x2))
#define Round(dInput) ((dInput) > 0.0f ? (int)(dInput + 0.5f):(int)(dInput - 0.5f))

MVOID
IspTuningCustom::userSetting_EFFECT(
        RAWIspCamInfo const& rCamInfo,
        EIndex_Effect_T const& eIdx_Effect,
        IspUsrSelectLevel_T const& rIspUsrSelectLevel,
        ISP_NVRAM_G2C_T& rG2C,
        ISP_NVRAM_SE_T& rSE
)
{
	MDOUBLE H_param[] = {-40, 0, 40};				// rotate degree H of 360
	MDOUBLE S_param[] = {44.0/64, 1, 84.0/64};		// gain = S
	MDOUBLE B_param[] = {-16*4, 0, 16*4};			// add B of 256
	MDOUBLE C_param[] = { 54.0/64, 1, 74.0/64};		// gain = C
	MINT32 M00, M01, M02, M10, M11, M12, M20, M21, M22, Yoffset, Uoffset, Voffset;
	MDOUBLE H, S, B, C;
    MINT32 YR,YG, YB, UR, UG, UB, VR, VG, VB, Y_OFFSET11, U_OFFSET10, V_OFFSET10;		// ISP registers for RGB2YUV
    MUINT32 SE_EDGE, SE_Y, SE_Y_CONST, SE_OILEN, SE_KNEESEL, SE_EGAIN_HB, SE_EGAIN_HA,	     \
	        SE_SPECIPONLY, SE_SPECIGAIN, SE_EGAIN_VB, SE_EGAIN_VA, SE_SPECIABS,	SE_SPECIINV, \
	        SE_COREH, SE_E_TH1_V, SE_EMBOSS1, SE_EMBOSS2, SE_YOUT_QBIT, SE_COUT_QBIT;	// ISP registers for SEEE

	M00 = 153;
	M01 = 301;
	M02 = 58;
	M10 = -86;
	M11 = -170;
	M12 = 256;
	M20 = 256;
	M21 = -214;
	M22 = -42;
	Yoffset = 0;
	Uoffset = 0;
	Voffset = 0;

    SE_Y = SE_Y_CONST = SE_OILEN = SE_KNEESEL = SE_EGAIN_HB = SE_EGAIN_HA = \
	SE_SPECIPONLY = SE_SPECIGAIN = SE_EGAIN_VB = SE_EGAIN_VA = SE_SPECIABS = SE_SPECIINV = \
	SE_COREH = SE_E_TH1_V = SE_EMBOSS1 = SE_EMBOSS2 = SE_YOUT_QBIT = SE_COUT_QBIT = 0;

    SE_EDGE = 1;

	MY_LOG("special effect selection : %d\n",eIdx_Effect);
	MY_LOG("input user level : H:%d S:%d B:%d C:%d\n",rIspUsrSelectLevel.eIdx_Hue,
                                                      rIspUsrSelectLevel.eIdx_Sat,
                                                      rIspUsrSelectLevel.eIdx_Bright,
                                                      rIspUsrSelectLevel.eIdx_Contrast);

	switch (eIdx_Effect) {
		case MEFFECT_MONO: // Mono
			M10 = 0;
			M11 = 0;
			M12 = 0;
			M20 = 0;
			M21 = 0;
			M22 = 0;

			//SE_EDGE = 0;

            break;

		case MEFFECT_SEPIA:	// Sepia
			M10 = 0;
			M11 = 0;
			M12 = 0;
			M20 = 0;
			M21 = 0;
			M22 = 0;
			Uoffset = -120; // -72 (Recommend)
			Voffset =  120; //  88 (Recommend)

			//SE_EDGE = 0;

			break;

		case MEFFECT_AQUA: // Aqua
			M10 = 0;
			M11 = 0;
			M12 = 0;
			M20 = 0;
			M21 = 0;
			M22 = 0;
			Uoffset = 352;	//  154 (Recommend)
			Voffset = -120;	// -154 (Recommend)

			//SE_EDGE = 0;

			break;

		case MEFFECT_NEGATIVE: // Negative
			M00 = -M00;
			M01 = -M01;
			M02 = -M02;
			M10 = -M10;
			M11 = -M11;
			M12 = -M12;
			M20 = -M20;
			M21 = -M21;
			M22 = -M22;
			Yoffset = 1023;

			//SE_EDGE = 0;

			break;

		case MEFFECT_POSTERIZE: // Posterize

			M00 = M00*1.6;
			M01 = M01*1.6;
			M02 = M02*1.6;
			M10 = M10*1.6;
			M11 = M11*1.6;
			M12 = M12*1.6;
			M20 = M20*1.6;
			M21 = M21*1.6;
			M22 = M22*1.6;
			Yoffset = Round(-512.0 * 0.6);

			SE_EDGE = 2;
			SE_YOUT_QBIT = 6;
			SE_COUT_QBIT = 5;

			break;

		case MEFFECT_BLACKBOARD: // Blackboard
			M10 = 0;
			M11 = 0;
			M12 = 0;
			M20 = 0;
			M21 = 0;
			M22 = 0;

			SE_Y = 1;
			SE_Y_CONST = 0;
			SE_EDGE = 2;
			SE_OILEN = 1;
			SE_KNEESEL = 3;
			SE_EGAIN_HB = 31;
			SE_EGAIN_HA = 15;
			SE_SPECIPONLY = 1;
			SE_SPECIGAIN = 2;
			SE_EGAIN_VB = 31;
			SE_EGAIN_VA = 15;
			SE_SPECIABS = 0;
			SE_SPECIINV = 0;
			SE_COREH = 0;
			SE_E_TH1_V = 4;
			SE_EMBOSS1 = 1;
			SE_EMBOSS2 = 1;

			break;

		case MEFFECT_WHITEBOARD:		// Whiteboard
			M10 = 0;
			M11 = 0;
			M12 = 0;
			M20 = 0;
			M21 = 0;
			M22 = 0;

			SE_Y_CONST = 255;
			SE_Y = 1;
			SE_EDGE = 2;
			SE_OILEN = 1;
			SE_KNEESEL = 3;
			SE_EGAIN_HB = 31;
			SE_EGAIN_HA = 15;
			SE_SPECIPONLY = 1;
			SE_SPECIGAIN = 2;
			SE_EGAIN_VB = 31;
			SE_EGAIN_VA = 15;
			SE_SPECIABS = 0;
			SE_SPECIINV = 1;
			SE_COREH = 0;
			SE_E_TH1_V = 4;
			SE_EMBOSS1 = 1;
			SE_EMBOSS2 = 1;

			break;

    case MEFFECT_OFF:           //  Do nothing.
    case MEFFECT_SOLARIZE:      //  Unsupport.
    case MEFFECT_SEPIAGREEN:    //  Unsupport.
    case MEFFECT_SEPIABLUE:     //  Unsupport.
    default:
        break;

	}

	H = H_param[rIspUsrSelectLevel.eIdx_Hue] * PI / 180;
	S = S_param[rIspUsrSelectLevel.eIdx_Sat];
	B = B_param[rIspUsrSelectLevel.eIdx_Bright];
	C = C_param[rIspUsrSelectLevel.eIdx_Contrast];

    MY_LOG("H=%f, S=%f, B=%f, C=%f \n",H, S, B, C);

	YR = LIMIT(Round(C * M00), -1023, 1023);
	YG = LIMIT(Round(C * M01), -1023, 1023);
	YB = LIMIT(Round(C * M02), -1023, 1023);
	UR = LIMIT(Round(S * cos(H) * M10 - S * sin(H) * M20), -1023, 1023);
	UG = LIMIT(Round(S * cos(H) * M11 - S * sin(H) * M21), -1023, 1023);
	UB = LIMIT(Round(S * cos(H) * M12 - S * sin(H) * M22), -1023, 1023);
	VR = LIMIT(Round(S * sin(H) * M10 + S * cos(H) * M20), -1023, 1023);
	VG = LIMIT(Round(S * sin(H) * M11 + S * cos(H) * M21), -1023, 1023);
	VB = LIMIT(Round(S * sin(H) * M12 + S * cos(H) * M22), -1023, 1023);

	Y_OFFSET11 = LIMIT(Round(Yoffset + B - 512 * (C-1)), -1023, 1023);
	U_OFFSET10 = LIMIT(Round(Uoffset * S), -511, 511);
	V_OFFSET10 = LIMIT(Round(Voffset * S), -511, 511);

	MY_LOG("YR=%d, YG=%d, YB=%d \n",YR, YG, YB);
	MY_LOG("UR=%d, UG=%d, UB=%d \n",UR, UG, UB);
	MY_LOG("VR=%d, VG=%d, VB=%d \n",VR, VG, VB);
	MY_LOG("Y_OFFSET11=%d, U_OFFSET10=%d, V_OFFSET10=%d \n",Y_OFFSET11, U_OFFSET10, V_OFFSET10);

    // Write back: G2C
    rG2C.conv_0a.bits.G2C_CNV00 = (YR >= 0) ? static_cast<MUINT32>(YR) : static_cast<MUINT32>(2048 + YR);
    rG2C.conv_0a.bits.G2C_CNV01 = (YG >= 0) ? static_cast<MUINT32>(YG) : static_cast<MUINT32>(2048 + YG);
    rG2C.conv_0b.bits.G2C_CNV02 = (YB >= 0) ? static_cast<MUINT32>(YB) : static_cast<MUINT32>(2048 + YB);
    rG2C.conv_0b.bits.G2C_YOFFSET11 = (Y_OFFSET11 >= 0) ? static_cast<MUINT32>(Y_OFFSET11) : static_cast<MUINT32>(2048 + Y_OFFSET11);
    rG2C.conv_1a.bits.G2C_CNV10 = (UR >= 0) ? static_cast<MUINT32>(UR) : static_cast<MUINT32>(2048 + UR);
    rG2C.conv_1a.bits.G2C_CNV11 = (UG >= 0) ? static_cast<MUINT32>(UG) : static_cast<MUINT32>(2048 + UG);
    rG2C.conv_1b.bits.G2C_CNV12 = (UB >= 0) ? static_cast<MUINT32>(UB) : static_cast<MUINT32>(2048 + UB);
    rG2C.conv_1b.bits.G2C_UOFFSET10 = (U_OFFSET10 >= 0) ? static_cast<MUINT32>(U_OFFSET10) : static_cast<MUINT32>(1024 + U_OFFSET10);
    rG2C.conv_2a.bits.G2C_CNV20 = (VR >= 0) ? static_cast<MUINT32>(VR) : static_cast<MUINT32>(2048 + VR);
    rG2C.conv_2a.bits.G2C_CNV21 = (VG >= 0) ? static_cast<MUINT32>(VG) : static_cast<MUINT32>(2048 + VG);
    rG2C.conv_2b.bits.G2C_CNV22 = (VB >= 0) ? static_cast<MUINT32>(VB) : static_cast<MUINT32>(2048 + VB);
    rG2C.conv_2b.bits.G2C_VOFFSET10 = (V_OFFSET10 >= 0) ? static_cast<MUINT32>(V_OFFSET10) : static_cast<MUINT32>(1024 + V_OFFSET10);

    MY_LOG("rG2C.conv_0a.bits.G2C_CNV00=0x%8x \n", rG2C.conv_0a.bits.G2C_CNV00);
    MY_LOG("rG2C.conv_0a.bits.G2C_CNV01=0x%8x \n", rG2C.conv_0a.bits.G2C_CNV01);
    MY_LOG("rG2C.conv_0b.bits.G2C_CNV02=0x%8x \n", rG2C.conv_0b.bits.G2C_CNV02);
    MY_LOG("rG2C.conv_0b.bits.G2C_YOFFSET11=0x%8x \n", rG2C.conv_0b.bits.G2C_YOFFSET11);
    MY_LOG("rG2C.conv_1a.bits.G2C_CNV10=0x%8x \n", rG2C.conv_1a.bits.G2C_CNV10);
    MY_LOG("rG2C.conv_1a.bits.G2C_CNV11=0x%8x \n", rG2C.conv_1a.bits.G2C_CNV11);
    MY_LOG("rG2C.conv_1b.bits.G2C_CNV12=0x%8x \n", rG2C.conv_1b.bits.G2C_CNV12);
    MY_LOG("rG2C.conv_1b.bits.G2C_UOFFSET10=0x%8x \n", rG2C.conv_1b.bits.G2C_UOFFSET10);
    MY_LOG("rG2C.conv_2a.bits.G2C_CNV20=0x%8x \n", rG2C.conv_2a.bits.G2C_CNV20);
    MY_LOG("rG2C.conv_2a.bits.G2C_CNV21=0x%8x \n", rG2C.conv_2a.bits.G2C_CNV21);
    MY_LOG("rG2C.conv_2b.bits.G2C_CNV22=0x%8x \n", rG2C.conv_2b.bits.G2C_CNV22);
    MY_LOG("rG2C.conv_2b.bits.G2C_VOFFSET10=0x%8x \n", rG2C.conv_2b.bits.G2C_VOFFSET10);

    // Write back: SE
    rSE.edge_ctrl.bits.SE_EDGE = SE_EDGE;
    rSE.y_ctrl.bits.SE_YOUT_QBIT = SE_YOUT_QBIT;
    rSE.y_ctrl.bits.SE_COUT_QBIT = SE_COUT_QBIT;
    rSE.y_ctrl.bits.SE_Y = SE_Y;
    rSE.y_ctrl.bits.SE_Y_CONST = SE_Y_CONST;
    rSE.edge_ctrl3.bits.SE_OILEN = SE_OILEN;
    rSE.special_ctrl.bits.SE_KNEESEL = SE_KNEESEL;
    rSE.edge_ctrl1.bits.SE_EGAIN_HB = SE_EGAIN_HB;
    rSE.edge_ctrl1.bits.SE_EGAIN_HA = SE_EGAIN_HA;
    rSE.special_ctrl.bits.SE_SPECIPONLY = SE_SPECIPONLY;
    rSE.special_ctrl.bits.SE_SPECIGAIN = SE_SPECIGAIN;
    rSE.edge_ctrl1.bits.SE_EGAIN_VB = SE_EGAIN_VB;
    rSE.edge_ctrl1.bits.SE_EGAIN_VA = SE_EGAIN_VA;
    rSE.special_ctrl.bits.SE_SPECIABS = SE_SPECIABS;
    rSE.special_ctrl.bits.SE_SPECIINV = SE_SPECIINV;
    rSE.core_ctrl1.bits.SE_COREH = SE_COREH;
    rSE.core_ctrl2.bits.SE_E_TH1_V = SE_E_TH1_V;
    rSE.edge_ctrl2.bits.SE_EMBOSS1 = SE_EMBOSS1;
    rSE.edge_ctrl2.bits.SE_EMBOSS2 = SE_EMBOSS2;

    MY_LOG("rSE.edge_ctrl.bits.SE_EDGE=%d \n",rSE.edge_ctrl.bits.SE_EDGE);
	MY_LOG("rSE.y_ctrl.bits.SE_YOUT_QBIT=%d \n",rSE.y_ctrl.bits.SE_YOUT_QBIT);
	MY_LOG("rSE.y_ctrl.bits.SE_COUT_QBIT=%d \n",rSE.y_ctrl.bits.SE_COUT_QBIT);        
	MY_LOG("rSE.y_ctrl.bits.SE_Y=%d \n",rSE.y_ctrl.bits.SE_Y);
	MY_LOG("rSE.y_ctrl.bits.SE_Y_CONST=%d \n",rSE.y_ctrl.bits.SE_Y_CONST);
	MY_LOG("rSE.edge_ctrl3.bits.SE_OILEN=%d \n",rSE.edge_ctrl3.bits.SE_OILEN);
	MY_LOG("rSE.special_ctrl.bits.SE_KNEESEL=%d \n",rSE.special_ctrl.bits.SE_KNEESEL);
	MY_LOG("rSE.edge_ctrl1.bits.SE_EGAIN_HB=%d \n",rSE.edge_ctrl1.bits.SE_EGAIN_HB);
	MY_LOG("rSE.edge_ctrl1.bits.SE_EGAIN_HA=%d \n",rSE.edge_ctrl1.bits.SE_EGAIN_HA);
	MY_LOG("rSE.special_ctrl.bits.SE_SPECIPONLY=%d \n",rSE.special_ctrl.bits.SE_SPECIPONLY);
	MY_LOG("rSE.special_ctrl.bits.SE_SPECIGAIN=%d \n",rSE.special_ctrl.bits.SE_SPECIGAIN);
	MY_LOG("rSE.edge_ctrl1.bits.SE_EGAIN_VB=%d \n",rSE.edge_ctrl1.bits.SE_EGAIN_VB);
	MY_LOG("rSE.edge_ctrl1.bits.SE_EGAIN_VA=%d \n",rSE.edge_ctrl1.bits.SE_EGAIN_VA);
	MY_LOG("rSE.special_ctrl.bits.SE_SPECIABS=%d \n",rSE.special_ctrl.bits.SE_SPECIABS);
	MY_LOG("rSE.special_ctrl.bits.SE_SPECIINV=%d \n",rSE.special_ctrl.bits.SE_SPECIINV);
	MY_LOG("rSE.core_ctrl1.bits.SE_COREH=%d \n",rSE.core_ctrl1.bits.SE_COREH);
	MY_LOG("rSE.core_ctrl2.bits.SE_E_TH1_V=%d \n",rSE.core_ctrl2.bits.SE_E_TH1_V);
	MY_LOG("rSE.edge_ctrl2.bits.SE_EMBOSS1=%d \n",rSE.edge_ctrl2.bits.SE_EMBOSS1);
	MY_LOG("rSE.edge_ctrl2.bits.SE_EMBOSS2=%d \n",rSE.edge_ctrl2.bits.SE_EMBOSS2);

    MY_LOG("rSE.edge_ctrl.val=0x%8x \n",rSE.edge_ctrl.val);
    MY_LOG("rSE.y_ctrl.val=0x%8x \n",rSE.y_ctrl.val);
    MY_LOG("rSE.edge_ctrl1.val=0x%8x \n",rSE.edge_ctrl1.val);
    MY_LOG("rSE.edge_ctrl2.val=0x%8x \n",rSE.edge_ctrl2.val);
    MY_LOG("rSE.edge_ctrl3.val=0x%8x \n",rSE.edge_ctrl3.val);
    MY_LOG("rSE.special_ctrl.val=0x%8x \n",rSE.special_ctrl.val);
    MY_LOG("rSE.core_ctrl1.val=0x%8x \n",rSE.core_ctrl1.val);
    MY_LOG("rSE.core_ctrl2.val=0x%8x \n",rSE.core_ctrl2.val);

}

};  //NSIspTuning


