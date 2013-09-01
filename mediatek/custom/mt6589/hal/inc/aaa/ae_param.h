

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

#ifndef _AE_PARAM_H
#define _AE_PARAM_H

#include <ae_feature.h>

#define AE_BLOCK_NO  5
#define FLARE_SCALE_UNIT (512) // 1.0 = 512
#define FLARE_OFFSET_DOMAIN (4095) // 12bit domain
#define AE_STABLE_THRES 3   //0.3 ev
#define AE_WIN_OFFSET          1000   // for android window define
#define MAX_ISP_GAIN   (10*1024)
#define AE_HISTOGRAM_BIN (128)

typedef enum
{
    AE_WEIGHTING_CENTRALWEIGHT=0,
    AE_WEIGHTING_SPOT,
    AE_WEIGHTING_AVERAGE
}eWeightingID;

typedef struct
{
   eWeightingID eID;  //weighting table ID
   MUINT32 W[5][5];    //AE weighting table
}strWeightTable;

typedef struct
{
    MBOOL   bEnableSaturationCheck;        //if toward high saturation scene , then reduce AE target
    MBOOL   bEnablePreIndex;                    // decide the re-initial index after come back to camera 
    MBOOL   bEnableRotateWeighting;        // AE rotate the weighting automatically or not
    MBOOL   bEV0TriggerStrobe;
    MBOOL   bLockCamPreMeteringWin;
    MBOOL   bLockVideoPreMeteringWin;
    MBOOL   bLockVideoRecMeteringWin;    
    MBOOL   bSkipAEinBirghtRange;            // To skip the AE in some brightness range for meter AE
    MBOOL   bPreAFLockAE;                        // Decide the do AE in the pre-AF or post-AF
    MBOOL   bStrobeFlarebyCapture;          // to Decide the strobe flare by capture image or precapture image
    MUINT32 u4BackLightStrength;              // strength of backlight condtion
    MUINT32 u4OverExpStrength;               // strength of anti over exposure
    MUINT32 u4HistStretchStrength;           //strength of  histogram stretch
    MUINT32 u4SmoothLevel;                      // time LPF smooth level , internal use
    MUINT32 u4TimeLPFLevel;                     //time LOW pass filter level
    MUINT8 uBlockNumX;                         //AE X block number
    MUINT8 uBlockNumY;                         //AE Yblock number
    MUINT8 uHist0StartBlockXRatio;       //Histogram 0 window config start block X ratio (0~100)
    MUINT8 uHist0EndBlockXRatio;         //Histogram 0 window config end block X ratio (0~100) 
    MUINT8 uHist0StartBlockYRatio;       //Histogram 0 window config start block Y ratio (0~100)
    MUINT8 uHist0EndBlockYRatio;         //Histogram 0 window config end block Y ratio (0~100) 
    MUINT8 uHist0OutputMode;               //Histogram 0 output source mode
    MUINT8 uHist0BinMode;                    //Histogram 0 bin mode range
    MUINT8 uHist1StartBlockXRatio;       //Histogram 1 window config start block X ratio (0~100)
    MUINT8 uHist1EndBlockXRatio;         //Histogram 1 window config end block X ratio (0~100) 
    MUINT8 uHist1StartBlockYRatio;       //Histogram 1 window config start block Y ratio (0~100)
    MUINT8 uHist1EndBlockYRatio;         //Histogram 1 window config end block Y ratio (0~100) 
    MUINT8 uHist1OutputMode;               //Histogram 1 output source mode
    MUINT8 uHist1BinMode;                    //Histogram 1 bin mode range
    MUINT8 uHist2StartBlockXRatio;       //Histogram 2 window config start block X ratio (0~100)
    MUINT8 uHist2EndBlockXRatio;         //Histogram 2 window config end block X ratio (0~100) 
    MUINT8 uHist2StartBlockYRatio;       //Histogram 2 window config start block Y ratio (0~100)
    MUINT8 uHist2EndBlockYRatio;         //Histogram 2 window config end block Y ratio (0~100) 
    MUINT8 uHist2OutputMode;               //Histogram 2 output source mode
    MUINT8 uHist2BinMode;                    //Histogram 2 bin mode range
    MUINT8 uHist3StartBlockXRatio;       //Histogram 3 window config start block X ratio (0~100)
    MUINT8 uHist3EndBlockXRatio;         //Histogram 3 window config end block X ratio (0~100) 
    MUINT8 uHist3StartBlockYRatio;       //Histogram 3 window config start block Y ratio (0~100)
    MUINT8 uHist3EndBlockYRatio;         //Histogram 3 window config end block Y ratio (0~100) 
    MUINT8 uHist3OutputMode;               //Histogram 3 output source mode
    MUINT8 uHist3BinMode;                      //Histogram 3 bin mode range
    MUINT8 uSatBlockCheckLow;             //saturation block check , low thres
    MUINT8 uSatBlockCheckHigh;            //sturation  block check , hight thres
    MUINT8 uSatBlockAdjustFactor;        // adjust factore , to adjust central weighting target value
    MUINT8 uMeteringYLowBound;           // metering area min Y value
    MUINT8 uMeteringYHighBound;          // metering area max Y value
    MUINT8 uMeteringYLowSkipRatio;     // metering area min Y value to skip AE
    MUINT8 uMeteringYHighSkipRatio;    // metering area max Y value to skip AE
    MUINT32 u4MeteringStableMax;        // for metering stable using. 100 means the stable point.
    MUINT32 u4MeteringStableMin;          // for metering stable using. 100 means the stable point.    
    MINT8   uAEShutterDelayCycle;         // for AE smooth used.
    MINT8   uAESensorGainDelayCycleWShutter;
    MINT8   uAESensorGainDelayCycleWOShutter;
    MINT8   uAEIspGainDelayCycle;
    MINT8   uPrvFlareWeightArr[16];            // for dynamic flare used
    MINT8   uVideoFlareWeightArr[16];            // for dynamic flare used
    MUINT32 u4FlareStdThrHigh;             // flare std high  256base
    MUINT32 u4FlareStdThrLow;             // flare std low    256 base
    MUINT32 u4PrvCapFlareDiff;             // step diff
}strAEParamCFG;

typedef struct 
{
    MINT32 Diff_EV;     //  delta EVx10 ,different between Yavg and Ytarget     Diff_EV=    log(  Yarg/Ytarget,2)
    MINT32  Ration;        //  Yarg/Ytarget  *100
    MINT32  move_index;   // move index
}strAEMOVE;

typedef struct
{
   MBOOL bAFPlineEnable;
   MINT16 i2FrameRate[5][2];
}strAFPlineInfo;

typedef struct
{
    MINT8 iLEVEL1_GAIN;
    MINT8 iLEVEL2_GAIN;
    MINT8 iLEVEL3_GAIN;   
    MINT8 iLEVEL4_GAIN;   
    MINT8 iLEVEL5_GAIN;   
    MINT8 iLEVEL6_GAIN;    
    MINT8 iLEVEL1_TARGET_DIFFERENCE;         
    MINT8 iLEVEL2_TARGET_DIFFERENCE;
    MINT8 iLEVEL3_TARGET_DIFFERENCE;
    MINT8 iLEVEL4_TARGET_DIFFERENCE;
    MINT8 iLEVEL5_TARGET_DIFFERENCE;
    MINT8 iLEVEL6_TARGET_DIFFERENCE;
    MINT8 iLEVEL1_GAINH;
    MINT8 iLEVEL1_GAINL;    
    MINT8 iLEVEL2_GAINH;
    MINT8 iLEVEL2_GAINL;    
    MINT8 iLEVEL3_GAINH;
    MINT8 iLEVEL3_GAINL;    
    MINT8 iLEVEL4_GAINH;
    MINT8 iLEVEL4_GAINL;    
    MINT8 iLEVEL5_GAINH;
    MINT8 iLEVEL5_GAINL;    
    MINT8 iLEVEL6_GAINH;
    MINT8 iLEVEL6_GAINL;    
    MINT8 iGAIN_DIFFERENCE_LIMITER;
}strAELimiterTable;                 

// AE statistics
#define AE_WINDOW_NUM_X (((AWB_WINDOW_NUM_X + 3) / 4) * 4) // size of AE window line

typedef struct
{
    AWB_WINDOW_T AWB_WIN[AWB_WINDOW_NUM_X];
    MUINT8 AE_WIN[AE_WINDOW_NUM_X];
} AWBAE_WINDOW_LINE_T;

typedef struct
{
    AWBAE_WINDOW_LINE_T LINE[AWB_WINDOW_NUM_Y];
    MUINT16 AE_HIST[4*AE_HISTOGRAM_BIN]; // 4 histogram x 128 bin (256 bytes)
} AWBAE_STAT_T;

/*******************************************************************************
* Dynamic Frame Rate for Video
******************************************************************************/
typedef struct VdoDynamicFrameRate_S
{
    MBOOL   isEnableDFps;
    MUINT32 EVThresNormal;
    MUINT32 EVThresNight;
} VdoDynamicFrameRate_T;

//////////////////////////////////////////
//
//  AE Parameter structure
//  Define AE algorithm initialize parameter here
//
////////////////////////////////////////

#define MAX_WEIGHT_TABLE 4
#define MAX_MAPPING_PLINE_TABLE 30

struct AE_PARAMETER
{
    strAEParamCFG strAEParasetting;
//    strAEPLineMapping   *pAEModePLineMapping;   // Get PLine ID for different AE mode
//    strAEPLineTable AEPlineTable;
    strWeightTable *pWeighting[MAX_WEIGHT_TABLE];   //AE WEIGHTING TABLE
    strAFPlineInfo strAFPLine;
    strAFPlineInfo strAFZSDPLine;
    strAFPlineInfo strStrobePLine;
    strAFPlineInfo strStrobeZSDPLine;
    MUINT32 *pEVValueArray;
    strAEMOVE *pAEMovingTable;
    strAEMOVE *pAEVideoMovingTable;    
    strAEMOVE *pAEFaceMovingTable;    
    strAELimiterTable strAELimiterData;
    VdoDynamicFrameRate_T strVdoDFps;
};

typedef struct AE_PARAMETER AE_PARAM_T;

/***********************
    Exposure time value , use in AE TV mode
***********************/
typedef enum
{
    TV_1_2      =0x00000002,    //!<: TV= 1/2 sec
    TV_1_3      =0x00000003,    //!<: TV= 1/3 sec
    TV_1_4      =0x00000004,    //!<: TV= 1/4 sec
    TV_1_5      =0x00000005,    //!<: TV= 1/5 sec
    TV_1_6      =0x00000006,    //!<: TV= 1/6 sec
    TV_1_7      =0x00000007,    //!<: TV= 1/7 sec
    TV_1_8      =0x00000008,    //!<: TV= 1/8 sec
    TV_1_10     =0x0000000A,    //!<: TV= 1/10 sec
    TV_1_13     =0x0000000D,    //!<: TV= 1/13 sec
    TV_1_15     =0x0000000F,    //!<: TV= 1/15 sec
    TV_1_20     =0x00000014,    //!<: TV= 1/20 sec
    TV_1_25     =0x00000019,    //!<: TV= 1/25 sec
    TV_1_30     =0x0000001E,    //!<: TV= 1/30 sec
    TV_1_40     =0x00000028,    //!<: TV= 1/40 sec
    TV_1_50     =0x00000032,    //!<: TV= 1/50 sec
    TV_1_60     =0x0000003C,    //!<: TV= 1/60 sec
    TV_1_80     =0x00000050,    //!<: TV= 1/80 sec
    TV_1_100    =0x00000064,    //!<: TV= 1/100 sec
    TV_1_125    =0x0000007D,    //!<: TV= 1/125 sec
    TV_1_160    =0x000000A0,    //!<: TV= 1/160  sec
    TV_1_200    =0x000000C8,    //!<: TV= 1/200 sec
    TV_1_250    =0x000000FA,    //!<: TV= 1/250 sec
    TV_1_320    =0x00000140,    //!<: TV= 1/320 sec
    TV_1_400    =0x00000190,    //!<: TV= 1/400 sec
    TV_1_500    =0x000001F4,    //!<: TV= 1/500 sec
    TV_1_640    =0x00000280,    //!<: TV= 1/640 sec
    TV_1_800    =0x00000320,    //!<: TV= 1/800 sec
    TV_1_1000   =0x000003E8,    //!<: TV= 1/1000 sec
    TV_1_1250   =0x000004E2,    //!<: TV= 1/1250 sec
    TV_1_1600   =0x00000640,    //!<: TV= 1/1600 sec

    TV_1_1      =0xFFFF0001,    //!<: TV= 1sec
    TV_2_1      =0xFFFF0002,    //!<: TV= 2sec
    TV_3_1      =0xFFFF0003,    //!<: TV= 3sec
    TV_4_1      =0xFFFF0004,    //!<: TV= 4sec
    TV_8_1      =0xFFFF0008,    //!<: TV= 8sec
    TV_16_1     =0xFFFF0016    //!<: TV= 16 sec
}eTimeValue;

/***********************
    Apertur time value , use in AE AV mode
    It's impossible list all Fno in enum
    So choose most close Fno.in enum
    and set real value in  structure "strAV.AvValue"
***********************/
typedef enum
{
    Fno_2,       //!<: Fno 2.0
    Fno_2_3,     //!<: Fno  2.3
    Fno_2_8,     //!<: Fno 2.8
    Fno_3_2,     //!<: Fno 3.2
    Fno_3_5,     //!<: Fno 3.5
    Fno_4_0,     //!<: Fno 4.0
    Fno_5_0,     //!<: Fno 5.0
    Fno_5_6,     //!<: Fno 5.6
    Fno_6_2,     //!<: Fno 6.2
    Fno_8_0,     //!<: Fno 8.0

    Fno_MAx
}eApetureValue ;


// AE Input/Output Structure
typedef enum
{
    AE_STATE_CREATE,         // CREATE , JUST CREATE
    AE_STATE_INIT,           //  INIT
    AE_STATE_NORMAL_PREVIEW, // normal AE
    AE_STATE_AFASSIST,       // aF assist mode, limit exposuret time
    AE_STATE_AELOCK,         // LOCK ae
    AE_STATE_CAPTURE,        //capture
    AE_STATE_ONE_SHOT, // one shot AE
    AE_STATE_BACKUP_PREVIEW,
    AE_STATE_RESTORE_PREVIEW,    
    AE_STATE_POST_CAPTURE,
    AE_STATE_MAX
}eAESTATE;


typedef struct
{
    eAESTATE  eAeState;   //ae state
    void*     pAESatisticBuffer;
} strAEInput;

typedef struct
{
    MBOOL          bAEStable;      // Only used in Preview/Movie
    strEvSetting  EvSetting;
    MINT32         Bv;
    MUINT32        u4AECondition;
    MINT32         i4DeltaBV;
    MUINT32        u4ISO;          //correspoing ISO , only use in capture
    MUINT16         u2FrameRate;     // Calculate the frame
    MINT16        i2FlareOffset;
    MINT16        i2FlareGain;   // in 512 domain
    MINT16        i2FaceDiffIndex;
    MINT32        i4AEidxCurrent;  // current AE idx
    MINT32        i4AEidxNext;   // next AE idx
} strAEOutput;

typedef struct
{
   MUINT32 u4HighY;
   MUINT32 u4LowY;
   MUINT32 u4Maxbin;
   MUINT32 u4Brightest;
   MUINT32 u4Darkest;
   MUINT32 u4BrightHalf;
   MUINT32 u4DarkHalf;
   MUINT32 u4DownSideBrightest;
   MUINT32 u4FullBrightest;
}strHistInfo;//histogram information

typedef struct
{
    MUINT32  u4CWValue;
    MUINT32  u4Dir;
    MUINT32  u4GreenCount;
    MUINT32  u4FaceMean;
    MUINT32*  pu4Hist1;
    MUINT32*  pu4Hist2;
    MUINT32*  pu4Hist3;
    strHistInfo sHistInfo;
}strAEInterInfo;

//Low Pass filer filter
#define G_FILTER_TAPIZE  8
#define  LPF_BUFFER_SIZE  G_FILTER_TAPIZE

typedef struct
{
    MUINT32 u4Idx ;          //index of ring buffer
    MUINT32 u4valid;         //valid data in ring buffer
    MUINT32 pu4LPFBuffer[LPF_BUFFER_SIZE]; //ring low pass buffer
    MUINT32 u4LPFLevel;                   //low pass filter level ;

}strTimeLPF;


typedef struct
{
    MUINT32 u4LpfMin;          // min lpf that be used
    MUINT32 u4LpfMax;         /// max lpf that be used

//
//
//  max LPF  level~~~~~~|       ------------
//                      |      //:          :\\,
//                      |     // :          : \\,
//                      |    //  :          :  \\,
//  min LPF level~~~~~~~|---//------------------\\---
//                          p1  p2         p3 p4    delta_idx

    MUINT32 u4p1;              // p1 please reference chart , 8X delta index
    MUINT32 u4p2;
    MUINT32 u4p3;
    MUINT32 u4p4;
}strLpfConfig;

//////////////////////////////
//  enum of AE condition
//
//////////////////////////////
enum
{
    AE_CONDITION_NORMAL=0x00,
    AE_CONDITION_BACKLIGHT=0x01,
    AE_CONDITION_OVEREXPOSURE=0x02,
    AE_CONDITION_HIST_STRETCH=0x04,
    AE_CONDITION_SATURATION_CHECK=0x08,
    AE_CONDITION_FACEAE=0x10
};

// AE algorithm parameter
//typedef struct
//{
//    struct_AE_Para   strAEPara;      //AE algorithm parameter
//    struct_AE           strAEStatConfig;   //AE statistic configuration
//} AE_PARAM_T;

typedef struct
{
    MBOOL bZoomChange;
    MUINT32 u4XOffset;
    MUINT32 u4YOffset;
    MUINT32 u4XWidth;
    MUINT32 u4YHeight;
} EZOOM_WINDOW_T;

typedef struct
{
    MUINT32 u4XLow;
    MUINT32 u4XHi;
    MUINT32 u4YLow;
    MUINT32 u4YHi;
    MUINT32 u4Weight;
} AE_BLOCK_WINDOW_T;

//AE Sensor Config information
typedef struct
{
    AE_NVRAM_T rAENVRAM;         // AE NVRAM param
    AE_PARAM_T rAEPARAM;
    AE_PLINETABLE_T rAEPlineTable;
    EZOOM_WINDOW_T rEZoomWin;
    MINT32 i4AEMaxBlockWidth;  // AE max block width
    MINT32 i4AEMaxBlockHeight; // AE max block height    
    LIB3A_AE_METERING_MODE_T eAEMeteringMode;
    LIB3A_AE_MODE_T eAEMode;
    LIB3A_AECAM_MODE_T eAECamMode;
    LIB3A_AE_FLICKER_MODE_T eAEFlickerMode;
    LIB3A_AE_FLICKER_AUTO_MODE_T eAEAutoFlickerMode;
    LIB3A_AE_EVCOMP_T eAEEVcomp;
    LIB3A_AE_ISO_SPEED_T eAEISOSpeed;
    MINT32    i4AEMaxFps;    
    MINT32    i4AEMinFps;    
} AE_INITIAL_INPUT_T;

#if 0
//Handle AE input/output
typedef struct
{
    MUINT32 u4AEWindowInfo[25];
    MUINT32 u4AEHistogram[64];
    MUINT32 u4FlareHistogram[10];
    MUINT32 u4AEBlockCnt;
    FD_AE_STAT_T rFDAEStat;
} AE_STAT_T;
#endif

//AAA_OUTPUT_PARAM_T use strAEOutput
typedef struct
{
	MUINT32 u4ExposureMode;	  // 0: exposure time, 1: exposure line
    MUINT32 u4Eposuretime;   //!<: Exposure time in ms
    MUINT32 u4AfeGain;       //!<: sensor gain
    MUINT32 u4IspGain;       //!<: raw gain
    MUINT16 u2FrameRate;
    MUINT32 u4RealISO;      //!<: ISO speed
    MINT16   i2FlareOffset;
    MINT16   i2FlareGain;   // 512 is 1x 
}AE_MODE_CFG_T;

typedef struct
{
    AE_MODE_CFG_T rPreviewMode;
    AE_MODE_CFG_T rAFMode;
    AE_MODE_CFG_T rCaptureMode[3];
}AE_OUTPUT_T;

typedef struct
{
    MBOOL bAEHistEn;
    MUINT8 uAEHistOpt;    // output source
    MUINT8 uAEHistBin;    // bin mode
    MUINT8 uAEHistYHi;    
    MUINT8 uAEHistYLow;    
    MUINT8 uAEHistXHi;    
    MUINT8 uAEHistXLow;    
} AE_HIST_WIN_T;

//AE Statistic window config
typedef struct
{
    AE_HIST_WIN_T rAEHistWinCFG[4];
} AE_STAT_PARAM_T;

typedef struct
{
    MUINT32 u4SensorExpTime;
    MUINT32 u4SensorGain;
    MUINT32 u4IspGain;
    MUINT32 u4ISOSpeed;
}AE_EXP_GAIN_MODIFY_T;

typedef struct AEMeterArea {
    MINT32 i4Left;
    MINT32 i4Top;
    MINT32 i4Right;
    MINT32 i4Bottom;
    MINT32 i4Weight;
} AEMeterArea_T;


#define MAX_AE_METER_AREAS  9

typedef struct AEMeteringArea {
	AEMeterArea_T rAreas[MAX_AE_METER_AREAS];
	MUINT32 u4Count;
} AEMeteringArea_T;

// AE info for ISP tuning
typedef struct
{
    MUINT32 u4AETarget;
    MUINT32 u4AECurrentTarget;
    MUINT32 u4Eposuretime;   //!<: Exposure time in ms
    MUINT32 u4AfeGain;           //!<: raw gain
    MUINT32 u4IspGain;           //!<: sensor gain
    MUINT32 u4RealISOValue;
    MINT32   i4LightValue_x10;                    
    MUINT32 u4AECondition;
    LIB3A_AE_METERING_MODE_T eAEMeterMode;
    MINT16   i2FlareOffset;
    MUINT16 u2Histogrm[AE_HISTOGRAM_BIN];
} AE_INFO_T;

#endif

