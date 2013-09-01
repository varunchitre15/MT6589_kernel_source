

/*
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include "camera_custom_types.h"
#include "camera_custom_nvram.h"
#include "ae_feature.h"
#include <aaa_types.h>
#include "camera_custom_AEPlinetable.h"
#include "awb_param.h"
#include "ae_param.h"
#include "ae_tuning_custom.h"

/*******************************************************************************
*
********************************************************************************/
AE_PARAM_T const&
getAEParam()
{
    static strAEParamCFG g_AEParasetting = 
    {
        FALSE,               // bEnableSaturationCheck
        TRUE,                // bEnablePreIndex
        TRUE,                // bEnableRotateWeighting;
        TRUE,               // bEV0TriggerStrobe
        FALSE,               // bLockCamPreMeteringWin;
        FALSE,               // bLockVideoPreMeteringWin;
        TRUE,                // bLockVideoRecMeteringWin;    
        TRUE,                // bSkipAEinBirghtRange;    
        TRUE,                // bPreAFLockAE
        TRUE,                // bStrobeFlarebyCapture
        256,                  // u4BackLightStrength : strength of backlight condtion
        256,                  // u4OverExpStrength : strength of anti over exposure
        256,                  // u4HistStretchStrength : strength of  histogram stretch
        0,                     // u4SmoothLevel : time LPF smooth level , internal use
        4,                     // u4TimeLPFLevel : time LOW pass filter level

        AE_BLOCK_NO,  // uBockXNum : AE X block number;
        AE_BLOCK_NO,  // uBockYNum : AE Yblock number;
        0,                      // uHist0StartBlockXRatio : Histogram 0 window config start block X ratio (0~100)
        100,                   // uHist0EndBlockXRatio : Histogram 0 window config end block X ratio (0~100) 
        0,                      // uHist0StartBlockYRatio : Histogram 0 window config start block Y ratio (0~100)
        100,                   // uHist0EndBlockYRatio : Histogram 0 window config end block Y ratio (0~100) 
        3,                      // uHist0OutputMode : Histogram 0 output source mode
        2,                      // uHist0BinMode : Histogram 0 bin mode range
        0,                      // uHist1StartBlockXRatio : Histogram 1 window config start block X ratio (0~100)
        100,                   // uHist1EndBlockXRatio : Histogram 1 window config end block X ratio (0~100) 
        0,                      // uHist1StartBlockYRatio : Histogram 1 window config start block Y ratio (0~100)
        100,                   // uHist1EndBlockYRatio : Histogram 1 window config end block Y ratio (0~100) 
        3,                      // uHist1OutputMode : Histogram 1 output source mode
        0,                      // uHist1BinMode : Histogram 1 bin mode range
        0,                      // uHist2StartBlockXRatio : Histogram 2 window config start block X ratio (0~100)
        100,                   // uHist2EndBlockXRatio : Histogram 2 window config end block X ratio (0~100) 
        0,                      // uHist2StartBlockYRatio : Histogram 2 window config start block Y ratio (0~100)
        100,                   // uHist2EndBlockYRatio : Histogram 2 window config end block Y ratio (0~100) 
        4,                      // uHist2OutputMode : Histogram 2 output source mode
        0,                      // uHist2BinMode : Histogram 2 bin mode range
        25,                     // uHist3StartBlockXRatio : Histogram 3 window config start block X ratio (0~100)
        75,                    // uHist3EndBlockXRatio : Histogram 3 window config end block X ratio (0~100) 
        25,                     // uHist3StartBlockYRatio : Histogram 3 window config start block Y ratio (0~100)
        75,                    // uHist3EndBlockYRatio : Histogram 3 window config end block Y ratio (0~100) 
        4,                      // uHist3OutputMode : Histogram 3 output source mode
        0,                      // uHist3BinMode : Histogram 3 bin mode range
        
        20,                    // uSatBlockCheckLow : saturation block check , low thres
        50,                     // uSatBlockCheckHigh : sturation  block check , hight thres
        50,                     // uSatBlockAdjustFactor : adjust factore , to adjust central weighting target value
 
        40,                     // uMeteringYLowBound;
        50,                     // uMeteringYHighBound;
        80,                     // uMeteringYLowSkipRatio : metering area min Y value to skip AE
        120,                   // uMeteringYHighSkipRatio : metering area max Y value to skip AE
        120,                   // u4MeteringStableMax;    // for metering stable using. 100 means the stable point.
        80,                    // u4MeteringStableMin;    // for metering stable using. 100 means the stable point.
        2,                      // uAEShutterDelayCycle;         // for AE smooth used.
        2,                      // uAESensorGainDelayCycleWShutter;
        1,                      // uAESensorGainDelayCycleWOShutter;
        0,                      // uAEIspGainDelayCycle;
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, // uPrvFlareWeightArr[16]            // for dynamic flare used
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, // uVideoFlareWeightArr[16]            // for dynamic flare used
        96,                   // u4FlareStdThrHigh             // flare std high
        48,                     // u4FlareStdThrLow             // flare std low
        0,                     // u4PrvCapFlareDiff
    };

    static strWeightTable  g_Weight_Matrix =
    {
        AE_WEIGHTING_CENTRALWEIGHT,
        {
            {22, 22, 22,  22, 22},
            {1 , 50, 50,  50, 1 },
            {34, 84, 191, 84, 34},
            {1,  50, 50,  50, 1 },
            {22, 22, 22,  22, 22},
        }
    };

    static strWeightTable  g_Weight_Spot =
    {
        AE_WEIGHTING_SPOT,
        {
            {0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0},
            {0, 0, 1, 0, 0},
            {0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0},
        }
    };

    static strWeightTable  g_Weight_Average =
    {
        AE_WEIGHTING_AVERAGE,
        {
            {1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1},
        }
    };

    static strAFPlineInfo g_strAFPLineTable =     // from high lv to low lv
    {
         0,                  // 1 enable the AF Pline, 0 disable the AF Pline (bypass)
         {
             {73, 30},       // {LV value, AF frame rate}
             {65, 25},  
             {55, 19}, 
             {45, 17}, 
             {0, 17},
         }
    }; 

    static strAFPlineInfo g_strAFZSDPLineTable =     // from high lv to low lv
    {
        0,                  // 1 enable the AF ZSD Pline, 0 disable the AF ZSD Pline (bypass)
        {
            {73, 15},       // {LV value, AF frame rate}
            {65, 15},  
            {55, 15}, 
            {45, 15}, 
            {0, 10},
        }
    }; 

    static strAFPlineInfo g_strStrobePLineTable =     // from high lv to low lv
    {
         1,                  // 1 enable the Strobe Pline, 0 disable the Strobe Pline (bypass)
         {
             {73, 30},       // {LV value, Strobe frame rate}
             {65, 25},  
             {55, 19}, 
             {45, 17}, 
             {0, 17},
         }
    }; 

    static strAFPlineInfo g_strStrobeZSDPLineTable =     // from high lv to low lv
    {
        1,                  // 1 enable the Strobe ZSD Pline, 0 disable the Strobe ZSD Pline (bypass)
        {
            {73, 15},       // {LV value, Strobe frame rate}
            {65, 15},  
            {55, 15}, 
            {45, 15}, 
            {0, 10},
        }
    }; 

    static MUINT32 g_strEVValueArray[25] =
    {
        1024,  // EV   0
        1160,  // EV   0.3
        1261,  // EV   0.5
        1370,  // EV   0.7
        1700, //1552,  // EV   1.0
        1758,  // EV   1.3
        1911,  // EV   1.5
        2077,  // EV   1.7
        2750, //2353,  // EV   2.0
        2896,  // EV   2.5
        5200, //3566,  // EV   3.0
        4390,  // EV   3.5
        5405,  // EV   4.0
         904,  // EV -0.3
         832,  // EV -0.5
         765,  // EV -0.7
         650, //748,  // EV -1.0
         596,  // EV -1.3
         549,  // EV -1.5
         505,  // EV -1.7
         420, //459,  // EV -2.0
         362,  // EV -2.5
         218,//218,  // EV -3.0
         239,  // EV -3.5
         194,  // EV -4.0
    };

    // total 24 sets
    static strAEMOVE  g_AEMoveTable[] =
    {
        {-20,   17,    24}, //   mean below -2.5  move increase 25 index
        {-20,   25,    19}, //   -2.5~-2  move increase 20 index
        {-15,   33,    15}, //   -2~-1.6
        {-15,   40,    12}, //   -1.6~-1.3
        {-10,   50,    9}, //   -1.3~-1
        { -8,   57,     7}, //   -1~-0.8
        { -5,   71,     4}, //   -0.8~-0.5
        { -4,   75,     3}, //   -0.5~-0.4
        { -3,   81,     2}, //   -0.4~-0.3
        { -1,   90,     1}, //   -0.3~-0.1
        {   0,  100,     0}, //   -0.1~0
        {   1,  110,     0}, //     0~0.1
        {   2,  114,    -1}, //    0.1~0.2       move decrease 1 index
        {   3,  123,    -1}, //    0.2~0.3
        {   4,  131,    -2}, //    0.3~0.4    
        {   5,  141,    -3}, //    0.4~0.5    
        {   7,  162,    -4}, //    0.5~0.7    
        {   9,  186,    -6}, //    0.7~0.9    
        { 10,  200,   -8}, //    0.9~1.0
        { 13,  246,   -9}, //    1.0~1.3
        { 16,  303,   -12}, //    1.3~1.6
        { 20,  400,   -15}, //    1.6~2       move decrease 10  index
        { 25,  566,   -19}, //    2~2.5       move decrease 20  index
        { 30,  800,   -22}, //    2.5~3      move decrease 30  index
    };

    // total 24 sets
    static strAEMOVE  g_AEVideoMoveTable[] =
    {
        {-20,   17,    20}, //   mean below -2.5  move increase 25 index
        {-20,   25,    15}, //   -2.5~-2  move increase 20 index
        {-15,   33,    10}, //   -2~-1.6
        {-15,   40,    6}, //   -1.6~-1.3
        {-10,   50,    4}, //   -1.3~-1
        { -8,   57,     3}, //   -1~-0.8
        { -5,   71,     2}, //   -0.8~-0.5
        { -4,   75,     1}, //   -0.5~-0.4
        { -3,   81,     1}, //   -0.4~-0.3
        { -1,   90,     1}, //   -0.3~-0.1
        {   0,  100,     0}, //   -0.1~0
        {   1,  110,     0}, //     0~0.1
        {   2,  114,    -1}, //    0.1~0.2       move decrease 1 index
        {   3,  123,    -1}, //    0.2~0.3
        {   4,  131,    -1}, //    0.3~0.4    
        {   5,  141,    -2}, //    0.4~0.5    
        {   7,  162,    -2}, //    0.5~0.7    
        {   9,  186,    -3}, //    0.7~0.9    
        { 10,  200,   -4}, //    0.9~1.0
        { 13,  246,   -4}, //    1.0~1.3
        { 16,  303,   -6}, //    1.3~1.6
        { 20,  400,   -7}, //    1.6~2       move decrease 10  index
        { 25,  566,   -9}, //    2~2.5       move decrease 20  index
        { 30,  800,   -11}, //    2.5~3      move decrease 30  index
    };

    // total 24 sets
    static strAEMOVE  g_AEFaceMoveTable[] =
    {
        {-20,   17,    20}, //   mean below -2.5  move increase 25 index
        {-20,   25,    15}, //   -2.5~-2  move increase 20 index
        {-15,   33,    10}, //   -2~-1.6
        {-15,   40,    6}, //   -1.6~-1.3
        {-10,   50,    4}, //   -1.3~-1
        { -8,   57,     3}, //   -1~-0.8
        { -5,   71,     2}, //   -0.8~-0.5
        { -4,   75,     1}, //   -0.5~-0.4
        { -3,   81,     1}, //   -0.4~-0.3
        { -1,   90,     1}, //   -0.3~-0.1
        {   0,  100,     0}, //   -0.1~0
        {   1,  110,     0}, //     0~0.1
        {   2,  114,    -1}, //    0.1~0.2       move decrease 1 index
        {   3,  123,    -1}, //    0.2~0.3
        {   4,  131,    -1}, //    0.3~0.4    
        {   5,  141,    -2}, //    0.4~0.5    
        {   7,  162,    -2}, //    0.5~0.7    
        {   9,  186,    -3}, //    0.7~0.9    
        { 10,  200,   -4}, //    0.9~1.0
        { 13,  246,   -4}, //    1.0~1.3
        { 16,  303,   -6}, //    1.3~1.6
        { 20,  400,   -7}, //    1.6~2       move decrease 10  index
        { 25,  566,   -9}, //    2~2.5       move decrease 20  index
        { 30,  800,   -11}, //    2.5~3      move decrease 30  index
    };

    // for AE limiter
    static strAELimiterTable g_AELimiterDataTable = 
    {
          2,    //  iLEVEL1_GAIN
        10,  //  iLEVEL2_GAIN
        18,  //  iLEVEL3_GAIN   
        28,  //  iLEVEL4_GAIN   
        40,  //  iLEVEL5_GAIN   
        50,  //  iLEVEL6_GAIN    
          0,    //  iLEVEL1_TARGET_DIFFERENCE
          4,    //  iLEVEL2_TARGET_DIFFERENCE
          7,    //  iLEVEL3_TARGET_DIFFERENCE
          7,    //  iLEVEL4_TARGET_DIFFERENCE
          7,    //  iLEVEL5_TARGET_DIFFERENCE
          7,    //  iLEVEL6_TARGET_DIFFERENCE
          2,    //  iLEVEL1_GAINH
        -2,  //  iLEVEL1_GAINL    
          6,    //  iLEVEL2_GAINH
        -7,  //  iLEVEL2_GAINL    
          8,    // iLEVEL3_GAINH
        -9,  //  iLEVEL3_GAINL    
        10,  //  iLEVEL4_GAINH
      -12, // iLEVEL4_GAINL    
        12,   // iLEVEL5_GAINH
      -16, // iLEVEL5_GAINL    
       15,   // iLEVEL6_GAINH
      -20, // iLEVEL6_GAINL    
          1,     // iGAIN_DIFFERENCE_LIMITER
    };

    // for video dynamic frame rate
    static VdoDynamicFrameRate_T g_VdoDynamicFpsTable = 
    {
       TRUE, // isEnableDFps
       50,  // EVThresNormal
       50,  // EVThresNight
    };
    
    static AE_PARAM_T strAEInitPara =
    {
        g_AEParasetting,
        {
            &g_Weight_Matrix,
            &g_Weight_Spot,
            &g_Weight_Average,
            NULL
        },
        g_strAFPLineTable,
        g_strAFZSDPLineTable,
        g_strStrobePLineTable,
        g_strStrobeZSDPLineTable,
        &g_strEVValueArray[0],
        g_AEMoveTable,
        g_AEVideoMoveTable,
        g_AEFaceMoveTable,
        g_AELimiterDataTable,
        g_VdoDynamicFpsTable,
    };

    return strAEInitPara;
}

/*******************************************************************************
*
********************************************************************************/
MBOOL
isAEinPreAFStatue()
{
    return MTRUE;
}

/*******************************************************************************
*
********************************************************************************/
MBOOL
isAEEnabled()
{
    return MTRUE;
}

