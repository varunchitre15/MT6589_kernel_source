
#ifndef _CAMERA_CUSTOM_NVRAM_H_
#define _CAMERA_CUSTOM_NVRAM_H_

#include <stddef.h>
#include "MediaTypes.h"
#include "ispif.h"
#include "CFG_Camera_File_Max_Size.h"
#include "camera_custom_AEPlinetable.h"


using namespace NSIspTuning;

#define NVRAM_CAMERA_SHADING_FILE_VERSION       1
#define NVRAM_CAMERA_PARA_FILE_VERSION          1
#define NVRAM_CAMERA_3A_FILE_VERSION            1
#define NVRAM_CAMERA_LENS_FILE_VERSION          1
#define NVRAM_CAMERA_STROBE_FILE_VERSION          1

/*******************************************************************************
* shading
********************************************************************************/
#define SHADING_SUPPORT_CT_NUM          (4)
#define SHADING_SUPPORT_OP_NUM          (6)
#define SHADING_SUPPORT_CH_NUM          (4)
#define MAX_FRM_GRID_NUM                (16)
#define MAX_TIL_GRID_NUM                (32)
#define COEFF_BITS_PER_CH               (128)
#define COEFF_PER_CH_U32                (COEFF_BITS_PER_CH>>5)
#define COEFF_PER_CH_U8                 (COEFF_BITS_PER_CH>>3)
#define MAX_SHADING_SIZE                (MAX_TIL_GRID_NUM*MAX_TIL_GRID_NUM*SHADING_SUPPORT_CH_NUM*COEFF_PER_CH_U32)//(1024) //INT32
#define MAX_SHADING_CapFrm_SIZE         (MAX_FRM_GRID_NUM*MAX_FRM_GRID_NUM*SHADING_SUPPORT_CH_NUM*COEFF_PER_CH_U32)//(4096) //INT32
#define MAX_SHADING_CapTil_SIZE         (MAX_TIL_GRID_NUM*MAX_TIL_GRID_NUM*SHADING_SUPPORT_CH_NUM*COEFF_PER_CH_U32)//(4096) //INT32
#define MAX_SHADING_PvwFrm_SIZE         (MAX_FRM_GRID_NUM*MAX_FRM_GRID_NUM*SHADING_SUPPORT_CH_NUM*COEFF_PER_CH_U32)//(1600) //INT32
#define MAX_SHADING_PvwTil_SIZE         (MAX_TIL_GRID_NUM*MAX_TIL_GRID_NUM*SHADING_SUPPORT_CH_NUM*COEFF_PER_CH_U32)//(1600) //INT32
#define MAX_SHADING_VdoFrm_SIZE         (MAX_FRM_GRID_NUM*MAX_FRM_GRID_NUM*SHADING_SUPPORT_CH_NUM*COEFF_PER_CH_U32)//(1600) //INT32
#define MAX_SVD_SHADING_SIZE            (MAX_TIL_GRID_NUM*MAX_TIL_GRID_NUM*SHADING_SUPPORT_CH_NUM*sizeof(UINT32))//(1024) //Byte
#define MAX_SENSOR_CAL_SIZE             (2048)//(1024) //Byte

#define member_size(type, member) sizeof(((type *)0)->member)
#define struct_size(type, start, end) \
    ((offsetof(type, end) - offsetof(type, start) + member_size(type, end)))

#define SIZEOF  sizeof

typedef struct {
    MUINT8     PixId; //0,1,2,3: B,Gb,Gr,R
    MUINT32    SlimLscType; //00A0  FF 00 02 01 (4 bytes)       4
    MUINT16    Width; //00A8    Capture Width (2 bytes) Capture Height (2 bytes)    2
    MUINT16    Height; //00A8    Capture Width (2 bytes) Capture Height (2 bytes)    2
    MUINT16    OffsetX; //00AA    Capture Offset X (2 bytes)  Capture Offfset Y (2 bytes) 2
    MUINT16    OffsetY; //00AA    Capture Offset X (2 bytes)  Capture Offfset Y (2 bytes) 2
    MUINT32    TblSize; //00B0   Capture Shading Table Size (4 bytes)        4
    MUINT32    IspLSCReg[5]; //00C8 Capture Shading Register Setting (5x4 bytes)        20
    MUINT8     GainTable[2048]; //00DC   Capture Shading Table (16 X 16 X 2 X 4 bytes)       2048
} SHADING_GOLDEN_REF;

#define SHADING_DATA                                                                \
    struct { \
        UINT32 Version;                                                             \
        UINT32 SensorId;                                                            \
        UINT16 LSCSize[SHADING_SUPPORT_OP_NUM];                                     \
        UINT16 PreviewSVDSize;                                                      \
        UINT16 VideoSVDSize;                                                        \
        UINT16 CaptureSVDSize;                                                      \
        UINT32 PreviewFrmTable[SHADING_SUPPORT_CT_NUM][MAX_SHADING_PvwFrm_SIZE];    \
        UINT32 CaptureFrmTable[SHADING_SUPPORT_CT_NUM][MAX_SHADING_CapFrm_SIZE];    \
        UINT32 CaptureTilTable[SHADING_SUPPORT_CT_NUM][MAX_SHADING_CapTil_SIZE];    \
        UINT32 VideoFrmTable[SHADING_SUPPORT_CT_NUM][MAX_SHADING_VdoFrm_SIZE];      \
        UINT32 N3DPvwTable[SHADING_SUPPORT_CT_NUM][MAX_SHADING_PvwTil_SIZE];        \
        UINT32 N3DCapTable[SHADING_SUPPORT_CT_NUM][MAX_SHADING_CapFrm_SIZE];        \
        SHADING_GOLDEN_REF SensorGoldenCalTable;                                    \
        UINT8 PreviewSVDTable[SHADING_SUPPORT_CT_NUM][MAX_SVD_SHADING_SIZE];        \
        UINT8 VideoSVDTable[SHADING_SUPPORT_CT_NUM][MAX_SVD_SHADING_SIZE];          \
        UINT8 CaptureSVDTable[SHADING_SUPPORT_CT_NUM][MAX_SVD_SHADING_SIZE];        \
        UINT8 SensorCalTable[MAX_SENSOR_CAL_SIZE]; \
        }


struct _ISP_SHADING_STRUCT
{
    SHADING_DATA;
};

typedef struct
{
    SHADING_DATA;
    UINT8 CameraData[MAXIMUM_NVRAM_CAMERA_SHADING_FILE_SIZE-
                     sizeof(struct _ISP_SHADING_STRUCT)];
} ISP_SHADING_STRUCT, *PISP_SHADING_STRUCT;

typedef struct
{
   	ISP_SHADING_STRUCT  Shading;
} NVRAM_CAMERA_SHADING_STRUCT, *PNVRAM_CAMERA_SHADING_STRUCT;

/*******************************************************************************
* 3A
********************************************************************************/

//____AE NVRAM____

typedef struct
{
    UINT32 u4MinGain;
    UINT32 u4MaxGain;
    UINT32 u4MiniISOGain;
    UINT32 u4GainStepUnit;
    UINT32 u4PreExpUnit;
    UINT32 u4PreMaxFrameRate;
    UINT32 u4VideoExpUnit;
    UINT32 u4VideoMaxFrameRate;
    UINT32 u4Video2PreRatio;    // 1x = 1024
    UINT32 u4CapExpUnit;
    UINT32 u4CapMaxFrameRate;
    UINT32 u4Cap2PreRatio;	    // 1x = 1024
    UINT32 u4LensFno;           // 10 Base
    UINT32 u4FocusLength_100x;           // 100 Base
} AE_DEVICES_INFO_T;

//histogram control information
#define AE_CCT_STRENGTH_NUM (5)

typedef struct
{
   //histogram info
    UINT32 u4HistHighThres;                         // central histogram high threshold
    UINT32 u4HistLowThres;                          // central histogram low threshold
    UINT32 u4MostBrightRatio;                       // full histogram high threshold
    UINT32 u4MostDarkRatio;                         // full histogram low threshold
    UINT32 u4CentralHighBound;                      // central block high boundary
    UINT32 u4CentralLowBound;                       // central block low bounary
    UINT32 u4OverExpThres[AE_CCT_STRENGTH_NUM];     // over exposure threshold
    UINT32 u4HistStretchThres[AE_CCT_STRENGTH_NUM]; // histogram stretch trheshold
    UINT32 u4BlackLightThres[AE_CCT_STRENGTH_NUM];  // backlight threshold
} AE_HIST_CFG_T;

//strAETable AE table Setting
typedef struct
{
    BOOL   bEnableBlackLight;           // enable back light detector
    BOOL   bEnableHistStretch;          // enable histogram stretch
    BOOL   bEnableAntiOverExposure;     // enable anti over exposure
    BOOL   bEnableTimeLPF;              // enable time domain LPF for smooth converge
    BOOL   bEnableCaptureThres;         // enable capture threshold or fix flare offset
    BOOL   bEnableVideoThres;             // enable video threshold or fix flare offset
    BOOL   bEnableStrobeThres;           // enable strobe threshold or fix flare offset

    UINT32 u4AETarget;                  // central weighting target
    UINT32 u4StrobeAETarget;            // central weighting target
    UINT32 u4InitIndex;                 // AE initiail index

    UINT32 u4BackLightWeight;           // Back light weighting value
    UINT32 u4HistStretchWeight;         // Histogram weighting value
    UINT32 u4AntiOverExpWeight;         // Anti over exposure weighting value

    UINT32 u4BlackLightStrengthIndex;   // Black light threshold strength index
    UINT32 u4HistStretchStrengthIndex;  // Histogram stretch threshold strength index
    UINT32 u4AntiOverExpStrengthIndex;  // Anti over exposure threshold strength index
    UINT32 u4TimeLPFStrengthIndex;      // Smooth converge threshold strength index
    UINT32 u4LPFConvergeLevel[AE_CCT_STRENGTH_NUM];  //LPF converge support level

    UINT32 u4InDoorEV;                  // check the environment indoor/outdoor
    INT32   i4BVOffset;                  // Calibrate BV offset
    UINT32 u4PreviewFlareOffset;        // Fix preview flare offset
    UINT32 u4CaptureFlareOffset;        // Fix capture flare offset
    UINT32 u4CaptureFlareThres;         // Capture flare threshold
    UINT32 u4VideoFlareOffset;        // Fix video flare offset
    UINT32 u4VideoFlareThres;         // video flare threshold
    UINT32 u4StrobeFlareOffset;        // Fix strobe flare offset
    UINT32 u4StrobeFlareThres;         // strobe flare threshold
    UINT32 u4PrvMaxFlareThres;        // for max preview flare thres used
    UINT32 u4PrvMinFlareThres;         // for min preview flare thres used
    UINT32 u4VideoMaxFlareThres;        // for video max flare thres used
    UINT32 u4VideoMinFlareThres;         // for video min flare thres used
    UINT32 u4FlatnessThres;              // 10 base for flatness condition.
    UINT32 u4FlatnessStrength;
} AE_CCT_CFG_T;                            // histogram control information

typedef struct
{
    AE_DEVICES_INFO_T rDevicesInfo;
    AE_HIST_CFG_T rHistConfig;
    AE_CCT_CFG_T rCCTConfig;
} AE_NVRAM_T;

//____AF NVRAM____

#define AF_TABLE_NUM (30)
#define ISO_MAX_NUM 8
#define GMEAN_MAX_NUM 6

#define JUMP_NUM 5

typedef struct
{
    MINT32 i4InfPos;
    MINT32 i4MacroPos;

} FOCUS_RANGE_T;

typedef struct
{
    MINT32  i4Offset;
    MINT32  i4NormalNum;
    MINT32  i4MacroNum;
    MINT32  i4InfIdxOffset;
    MINT32  i4MacroIdxOffset;
    MINT32  i4Pos[AF_TABLE_NUM];

} NVRAM_AF_TABLE_T;

typedef struct
{
    MINT32 i4ISONum;
    MINT32 i4ISO[ISO_MAX_NUM];

    MINT32 i4GMeanNum;
    MINT32 i4GMean[GMEAN_MAX_NUM];

    MINT32 i4GMR[3][ISO_MAX_NUM];

    MINT32 i4FV_DC[GMEAN_MAX_NUM][ISO_MAX_NUM];
    MINT32 i4MIN_TH[GMEAN_MAX_NUM][ISO_MAX_NUM];
    MINT32 i4HW_TH[GMEAN_MAX_NUM][ISO_MAX_NUM];

    MINT32 i4FV_DC2[GMEAN_MAX_NUM][ISO_MAX_NUM];
    MINT32 i4MIN_TH2[GMEAN_MAX_NUM][ISO_MAX_NUM];
    MINT32 i4HW_TH2[GMEAN_MAX_NUM][ISO_MAX_NUM];

} NVRAM_AF_THRES_T;

typedef struct
{
    NVRAM_AF_TABLE_T sTABLE;
    MINT32 i4THRES_MAIN;
    MINT32 i4THRES_SUB;

    MINT32 i4INIT_WAIT;
    MINT32 i4FRAME_WAIT[JUMP_NUM];
    MINT32 i4DONE_WAIT;

    MINT32 i4FAIL_POS;

    // FV Monitor
    MINT32 i4FRAME_TIME;

    MINT32 i4FIRST_FV_WAIT;

    MINT32 i4FV_CHANGE_THRES;
    MINT32 i4FV_CHANGE_OFFSET;
    MINT32 i4FV_CHANGE_CNT;
    MINT32 i4GS_CHANGE_THRES;
    MINT32 i4GS_CHANGE_OFFSET;
    MINT32 i4GS_CHANGE_CNT;
    MINT32 i4FV_STABLE_THRES;         // percentage -> 0 more stable
    MINT32 i4FV_STABLE_OFFSET;        // value -> 0 more stable
    MINT32 i4FV_STABLE_NUM;           // max = 9 (more stable), reset = 0
    MINT32 i4FV_STABLE_CNT;           // max = 9
    MINT32 i4FV_1ST_STABLE_THRES;
    MINT32 i4FV_1ST_STABLE_OFFSET;
    MINT32 i4FV_1ST_STABLE_NUM;
    MINT32 i4FV_1ST_STABLE_CNT;

} NVRAM_AF_COEF;

typedef struct
{
    NVRAM_AF_COEF sAF_Coef;
    NVRAM_AF_COEF sZSD_AF_Coef;
    NVRAM_AF_COEF sVAFC_Coef;

    NVRAM_AF_THRES_T sAF_TH;
    NVRAM_AF_THRES_T sZSD_AF_TH;

    // VAFC
    MINT32 i4VAFC_FAIL_CNT;
    // common use
    MINT32 i4CHANGE_CNT_DELTA;

    MINT32 i4LV_THRES;

    // --- AFC window location ---
    MINT32 i4SPOT_PERCENT_W;
    MINT32 i4SPOT_PERCENT_H;

    MINT32 i4InfPos;
    MINT32 i4AFC_STEP_SIZE;

    MINT32 i4Hyst[2][JUMP_NUM];
    MINT32 i4BackJump[JUMP_NUM];
    MINT32 i4BackJumpPos;

    // --- FD ---
    MINT32 i4FDWinPercent;
    MINT32 i4FDSizeDiff;
    
    // --- Stat ---
    MINT32 i4StatGain;
    
    // --- reserved ---
    MINT32 i4Coef[20];

} AF_NVRAM_T;

//____AWB NVRAM____

// AWB gain
typedef struct
{
    INT32 i4R; // R gain
    INT32 i4G; // G gain
    INT32 i4B; // B gain
} AWB_GAIN_T;

// XY coordinate
typedef struct
{
    INT32 i4X; // X
    INT32 i4Y; // Y
} XY_COORDINATE_T;

// Light area
typedef struct
{
    INT32 i4RightBound; // Right bound
    INT32 i4LeftBound;  // Left bound
    INT32 i4UpperBound; // Upper bound
    INT32 i4LowerBound; // Lower bound
} LIGHT_AREA_T;

// Preference color
typedef struct
{
    INT32 i4SliderValue; // Slider value
    INT32 i4OffsetThr;   // Offset threshold
} PREFERENCE_COLOR_T;

// AWB calibration data
typedef struct
{
    AWB_GAIN_T rUnitGain;      // Unit gain: WB gain of DNP (individual camera)
    AWB_GAIN_T rGoldenGain;    // Golden sample gain: WB gain of DNP (golden sample)
    AWB_GAIN_T rTuningUnitGain; // Unit gain of tuning sample (for debug purpose)
    AWB_GAIN_T rD65Gain;    // WB gain of D65 (golden sample)
} AWB_CALIBRATION_DATA_T;

// AWB light source XY coordinate
typedef struct
{
	XY_COORDINATE_T rStrobe;   // Strobe
    XY_COORDINATE_T rHorizon;  // Horizon
    XY_COORDINATE_T rA;        // A
    XY_COORDINATE_T rTL84;     // TL84
    XY_COORDINATE_T rCWF;      // CWF
    XY_COORDINATE_T rDNP;      // DNP
    XY_COORDINATE_T rD65;      // D65
    XY_COORDINATE_T rDF;       // Daylight fluorescent
} AWB_LIGHT_SOURCE_XY_COORDINATE_T;

// AWB light source AWB gain
typedef struct
{
	AWB_GAIN_T rStrobe;   // Strobe
    AWB_GAIN_T rHorizon;  // Horizon
    AWB_GAIN_T rA;        // A
    AWB_GAIN_T rTL84;     // TL84
    AWB_GAIN_T rCWF;      // CWF
    AWB_GAIN_T rDNP;      // DNP
    AWB_GAIN_T rD65;      // D65
    AWB_GAIN_T rDF;       // Daylight fluorescent
} AWB_LIGHT_SOURCE_AWB_GAIN_T;

// Rotation matrix parameter
typedef struct
{
    INT32 i4RotationAngle; // Rotation angle
    INT32 i4Cos;           // cos
    INT32 i4Sin;           // sin
} AWB_ROTATION_MATRIX_T;

// Daylight locus parameter
typedef struct
{
    INT32 i4SlopeNumerator;   // Slope numerator
    INT32 i4SlopeDenominator; // Slope denominator
} AWB_DAYLIGHT_LOCUS_T;

// AWB light area
typedef struct
{
	LIGHT_AREA_T rStrobe; // Strobe
    LIGHT_AREA_T rTungsten;        // Tungsten
    LIGHT_AREA_T rWarmFluorescent; // Warm fluorescent
    LIGHT_AREA_T rFluorescent;     // Fluorescent
    LIGHT_AREA_T rCWF;             // CWF
    LIGHT_AREA_T rDaylight;        // Daylight
    LIGHT_AREA_T rShade;           // Shade
    LIGHT_AREA_T rDaylightFluorescent; // Daylight fluorescent
} AWB_LIGHT_AREA_T;

// PWB light area
typedef struct
{
    LIGHT_AREA_T rReferenceArea;   // Reference area
    LIGHT_AREA_T rDaylight;        // Daylight
    LIGHT_AREA_T rCloudyDaylight;  // Cloudy daylight
    LIGHT_AREA_T rShade;           // Shade
    LIGHT_AREA_T rTwilight;        // Twilight
    LIGHT_AREA_T rFluorescent;     // Fluorescent
    LIGHT_AREA_T rWarmFluorescent; // Warm fluorescent
    LIGHT_AREA_T rIncandescent;    // Incandescent
    LIGHT_AREA_T rGrayWorld; // for CCT use
} PWB_LIGHT_AREA_T;

// PWB default gain
typedef struct
{
    AWB_GAIN_T rDaylight;        // Daylight
    AWB_GAIN_T rCloudyDaylight;  // Cloudy daylight
    AWB_GAIN_T rShade;           // Shade
    AWB_GAIN_T rTwilight;        // Twilight
    AWB_GAIN_T rFluorescent;     // Fluorescent
    AWB_GAIN_T rWarmFluorescent; // Warm fluorescent
    AWB_GAIN_T rIncandescent;    // Incandescent
    AWB_GAIN_T rGrayWorld; // for CCT use
} PWB_DEFAULT_GAIN_T;

// AWB preference color
typedef struct
{
    PREFERENCE_COLOR_T rTungsten;        // Tungsten
    PREFERENCE_COLOR_T rWarmFluorescent; // Warm fluorescent
    PREFERENCE_COLOR_T rShade;           // Shade
    AWB_GAIN_T rDaylightWBGain;           // Daylight WB gain
	AWB_GAIN_T rPreferenceGain_Strobe;              // Preference gain: strobe
    AWB_GAIN_T rPreferenceGain_Tungsten;            // Preference gain: tungsten
    AWB_GAIN_T rPreferenceGain_WarmFluorescent;     // Preference gain: warm fluorescent
    AWB_GAIN_T rPreferenceGain_Fluorescent;         // Preference gain: fluorescent
    AWB_GAIN_T rPreferenceGain_CWF;                 // Preference gain: CWF
    AWB_GAIN_T rPreferenceGain_Daylight;            // Preference gain: daylight
    AWB_GAIN_T rPreferenceGain_Shade;               // Preference gain: shade
    AWB_GAIN_T rPreferenceGain_DaylightFluorescent; // Preference gain: daylight fluorescent
} AWB_PREFERENCE_COLOR_T;

#define AWB_CCT_ESTIMATION_LIGHT_SOURCE_NUM (5)

// CCT estimation
typedef struct
{
    INT32 i4CCT[AWB_CCT_ESTIMATION_LIGHT_SOURCE_NUM];                // CCT
    INT32 i4RotatedXCoordinate[AWB_CCT_ESTIMATION_LIGHT_SOURCE_NUM]; // Rotated X coordinate
} AWB_CCT_ESTIMATION_T;


// AWB NVRAM structure
typedef struct
{
    AWB_CALIBRATION_DATA_T rCalData; // AWB calibration data
    AWB_LIGHT_SOURCE_XY_COORDINATE_T rOriginalXY; // Original XY coordinate of AWB light source
    AWB_LIGHT_SOURCE_XY_COORDINATE_T rRotatedXY; // Rotated XY coordinate of AWB light source
	AWB_LIGHT_SOURCE_AWB_GAIN_T rLightAWBGain; // AWB gain of AWB light source
    AWB_ROTATION_MATRIX_T rRotationMatrix; // Rotation matrix parameter
    AWB_DAYLIGHT_LOCUS_T rDaylightLocus; // Daylight locus parameter
    AWB_LIGHT_AREA_T rAWBLightArea; // AWB light area
    PWB_LIGHT_AREA_T rPWBLightArea; // PWB light area
    PWB_DEFAULT_GAIN_T rPWBDefaultGain; // PWB default gain
    AWB_PREFERENCE_COLOR_T rPreferenceColor; // AWB preference color
    AWB_CCT_ESTIMATION_T rCCTEstimation; // CCT estimation
} AWB_NVRAM_T;

//____3A NVRAM____

//typedef unsigned char  UINT8;

typedef struct
{
    //data structure version, update once structure been modified.
    UINT32 u4Version;

    // ID of sensor module
    UINT32 SensorId;

    //data content
    AE_NVRAM_T rAENVRAM;
    AWB_NVRAM_T rAWBNVRAM;

    //SSS(reserved unused spaces(bytes)) = total-used;,
    //ex. SSS = 4096-sizeof(UINT32)--sizeof(NVRAM_AAA_T)-sizeof(NVRAM_bbb_T);
    //    UINT8 reserved[MAXIMUM_NVRAM_CAMERA_3A_FILE_SIZE-sizeof(UINT32)-sizeof(AE_NVRAM_T)-sizeof(AF_NVRAM_T)-sizeof(AWB_NVRAM_T)];
    UINT8 reserved[MAXIMUM_NVRAM_CAMERA_3A_FILE_SIZE-sizeof(UINT32)-sizeof(AE_NVRAM_T)-sizeof(AWB_NVRAM_T)];
} NVRAM_CAMERA_3A_STRUCT, *PNVRAM_CAMERA_3A_STRUCT;

///////////////////////////////////////////////////////////////////




typedef struct
{
	int yTar;
	int antiIsoLevel;
	int antiExpLevel;
	int antiStrobeLevel;
	int antiUnderLevel;
	int antiOverLevel;
	int foregroundLevel;
	int isRefAfDistance;
    int accuracyLevel;

} NVRAM_FLASH_TUNING_PARA;

typedef struct
{
	int exp;
	int afe_gain;
	int isp_gain;
	int distance;
	short yTab[256];  //x128
	short rgTab[256]; //1024 base
	short bgTab[256]; //1024 base
	short rsv[400];

}NVRAM_FLASH_CCT_ENG_TABLE;


typedef struct
{
	//torch, video
	int torchEngMode;
	int torchPeakI;
    int torchAveI;
	int torchDuty;
	int torchStep;

	//AF
	int afEngMode;
	int afPeakI;
    int afAveI;
	int afDuty;
	int afStep;

	//pf, mf
	int pmfEngMode;
	//normal bat setting
	int pfAveI;
	int mfAveIMax;
	int mfAveIMin;
	int pmfPeakI;
	int pfDuty;
	int mfDutyMax;
	int mfDutyMin;
	int pmfStep;
	//low bat setting
	int IChangeByVBatEn;
	int vBatL;	//mv
	int pfAveIL;
	int mfAveIMaxL;
	int mfAveIMinL;
	int pmfPeakIL;
	int pfDutyL;
	int mfDutyMaxL;
	int mfDutyMinL;
	int pmfStepL;
	//burst setting
	int IChangeByBurstEn;
	int pfAveIB;
	int mfAveIMaxB;
	int mfAveIMinB;
	int pmfPeakIB;
	int pfDutyB;
	int mfDutyMaxB;
	int mfDutyMinB;
	int pmfStepB;
}
NVRAM_FLASH_ENG_LEVEL;


typedef struct
{
	int usedCount;
	int arg[100];
}
NVRAM_FLASH_EXTENSION;

typedef struct
{
    UINT32 u4Version;

	NVRAM_FLASH_CCT_ENG_TABLE engTab;
	NVRAM_FLASH_TUNING_PARA tuningPara[6];

	int isTorchEngUpdate;
	int isAfEngUpdate;
	int isNormaEnglUpdate;
	int isLowBatEngUpdate;
	int isBurstEngUpdate;
	NVRAM_FLASH_ENG_LEVEL engLevel;
	NVRAM_FLASH_EXTENSION ext;

    UINT8 reserved[MAXIMUM_NVRAM_CAMERA_DEFECT_FILE_SIZE-sizeof(UINT32)-sizeof(NVRAM_FLASH_CCT_ENG_TABLE)-sizeof(NVRAM_FLASH_TUNING_PARA)*6-sizeof(int)*5-sizeof(NVRAM_FLASH_ENG_LEVEL)-sizeof(NVRAM_FLASH_EXTENSION)];

} NVRAM_CAMERA_STROBE_STRUCT, *PNVRAM_CAMERA_STROBE_STRUCT;



/*******************************************************************************
* ISP NVRAM parameter
********************************************************************************/
#define NVRAM_OBC_TBL_NUM               (12)
#define NVRAM_BPC_TBL_NUM               (9)
#define NVRAM_NR1_TBL_NUM               (9)
#define NVRAM_LSC_TBL_NUM               (6)
#define NVRAM_CFA_TBL_NUM               (43) // +1: default for disable
#define NVRAM_CCM_TBL_NUM               (4)
#define NVRAM_GGM_TBL_NUM               (5)
#define NVRAM_ANR_TBL_NUM               (49)
#define NVRAM_CCR_TBL_NUM               (7)
#define NVRAM_EE_TBL_NUM                (49)
#define NVRAM_NR3D_TBL_NUM              (28)
#define NVRAM_MFB_TBL_NUM               (7)

#define NVRAM_CFA_DISABLE_IDX           (NVRAM_CFA_TBL_NUM-1)

// camera common parameters and sensor parameters
typedef struct
{
    UINT32 CommReg[64];
} ISP_NVRAM_COMMON_STRUCT, *PISP_NVRAM_COMMON_STRUCT;

typedef struct ISP_NVRAM_REG_INDEX_STRUCT
{
    UINT8 OBC;
    UINT8 BPC;
    UINT8 NR1;
    UINT8 LSC;
    UINT8 CFA;
    UINT8 CCM;
    UINT8 GGM;
    UINT8 ANR;
    UINT8 CCR;
    UINT8 EE;
    UINT8 NR3D;
    UINT8 MFB;
}	ISP_NVRAM_REG_INDEX_T, *PISP_NVRAM_REG_INDEX_T;

typedef struct
{
    ISP_NVRAM_REG_INDEX_T       Idx;
    ISP_NVRAM_OBC_T             OBC[NVRAM_OBC_TBL_NUM];
    ISP_NVRAM_BPC_T             BPC[NVRAM_BPC_TBL_NUM];
    ISP_NVRAM_NR1_T             NR1[NVRAM_NR1_TBL_NUM];
    ISP_NVRAM_LSC_T             LSC[NVRAM_LSC_TBL_NUM];
    ISP_NVRAM_CFA_T             CFA[NVRAM_CFA_TBL_NUM];
    ISP_NVRAM_CCM_T             CCM[NVRAM_CCM_TBL_NUM];
    ISP_NVRAM_GGM_T             GGM[NVRAM_GGM_TBL_NUM];
    ISP_NVRAM_ANR_T             ANR[NVRAM_ANR_TBL_NUM];
    ISP_NVRAM_CCR_T             CCR[NVRAM_CCR_TBL_NUM];
    ISP_NVRAM_EE_T              EE[NVRAM_EE_TBL_NUM];
    ISP_NVRAM_NR3D_T            NR3D[NVRAM_NR3D_TBL_NUM];
    ISP_NVRAM_MFB_T             MFB[NVRAM_MFB_TBL_NUM];
} ISP_NVRAM_REGISTER_STRUCT, *PISP_NVRAM_REGISTER_STRUCT;

typedef struct
{
    MINT32 value[3];
} ISP_NVRAM_PCA_SLIDER_STRUCT, *PISP_NVRAM_PCA_SLIDER_STRUCT;

typedef struct
{
    ISP_NVRAM_PCA_SLIDER_STRUCT Slider;
    ISP_NVRAM_PCA_T        Config;
    ISP_NVRAM_PCA_LUTS_T   PCA_LUTS;
} ISP_NVRAM_PCA_STRUCT, *PISP_NVRAM_PCA_STRUCT;

// MFB SW mixer parameter
typedef struct
{
    MINT32 i4M0;
    MINT32 i4M1;
} ISP_NVRAM_MFB_MIXER_PARAM_T;

typedef struct
{
    ISP_NVRAM_MFB_MIXER_PARAM_T param[7];
} ISP_NVRAM_MFB_MIXER_STRUCT, *PISP_NVRAM_MFB_MIXER_STRUCT;

typedef struct
{
    MINT32 i4R_AVG;
    MINT32 i4R_STD;
    MINT32 i4B_AVG;
    MINT32 i4B_STD;
    MINT32 i4P00[9];
    MINT32 i4P10[9];
    MINT32 i4P01[9]; 
    MINT32 i4P20[9]; 
    MINT32 i4P11[9];
    MINT32 i4P02[9];
} ISP_NVRAM_CCM_POLY22_STRUCT, *PISP_NVRAM_CCM_POLY22_STRUCT;

typedef union
{
    struct  {
        MUINT32                     Version;
        MUINT32                     SensorId;    // ID of sensor module
        ISP_NVRAM_COMMON_STRUCT     ISPComm;
        ISP_NVRAM_PCA_STRUCT        ISPPca;
        ISP_NVRAM_REGISTER_STRUCT   ISPRegs;
        ISP_NVRAM_MFB_MIXER_STRUCT  ISPMfbMixer;
        ISP_NVRAM_CCM_POLY22_STRUCT ISPCcmPoly22;
    };
    UINT8   Data[MAXIMUM_NVRAM_CAMERA_ISP_FILE_SIZE];
} NVRAM_CAMERA_ISP_PARAM_STRUCT, *PNVRAM_CAMERA_ISP_PARAM_STRUCT;


class IspNvramRegMgr
{
public:
    IspNvramRegMgr(ISP_NVRAM_REGISTER_STRUCT*const pIspNvramRegs)
        : m_rRegs(*pIspNvramRegs)
        , m_rIdx(pIspNvramRegs->Idx)
    {}
    virtual ~IspNvramRegMgr() {}

public:
    enum EIndexNum
    {
        NUM_OBC         =   NVRAM_OBC_TBL_NUM,
        NUM_BPC         =   NVRAM_BPC_TBL_NUM,
        NUM_NR1         =   NVRAM_NR1_TBL_NUM,
        NUM_LSC         =   NVRAM_LSC_TBL_NUM,
        NUM_CFA         =   NVRAM_CFA_TBL_NUM,
        NUM_CCM         =   NVRAM_CCM_TBL_NUM,
        NUM_GGM         =   NVRAM_GGM_TBL_NUM,
        NUM_ANR         =   NVRAM_ANR_TBL_NUM,
        NUM_CCR         =   NVRAM_CCR_TBL_NUM,
        NUM_EE          =   NVRAM_EE_TBL_NUM,
        NUM_NR3D        =   NVRAM_NR3D_TBL_NUM,
        NUM_MFB         =   NVRAM_MFB_TBL_NUM
    };

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Index.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
private:    ////    Set Index
    inline bool setIdx(UINT8 &rIdxTgt, UINT8 const IdxSrc, EIndexNum const Num)
    {
        if  (IdxSrc < Num)
        {
            rIdxTgt = IdxSrc;
            return  true;
        }
        return  false;
    }
public:     ////    Set Index
    inline bool setIdx_OBC(UINT8 const idx)        { return setIdx(m_rIdx.OBC, idx, NUM_OBC); }
    inline bool setIdx_BPC(UINT8 const idx)        { return setIdx(m_rIdx.BPC, idx, NUM_BPC); }
    inline bool setIdx_NR1(UINT8 const idx)        { return setIdx(m_rIdx.NR1, idx, NUM_NR1); }
    inline bool setIdx_LSC(UINT8 const idx)        { return setIdx(m_rIdx.LSC, idx, NUM_LSC); }
    inline bool setIdx_CFA(UINT8 const idx)        { return setIdx(m_rIdx.CFA, idx, NUM_CFA); }
    inline bool setIdx_CCM(UINT8 const idx)        { return setIdx(m_rIdx.CCM, idx, NUM_CCM); }
    inline bool setIdx_GGM(UINT8 const idx)        { return setIdx(m_rIdx.GGM, idx, NUM_GGM); }
    inline bool setIdx_ANR(UINT8 const idx)        { return setIdx(m_rIdx.ANR, idx, NUM_ANR); }
    inline bool setIdx_CCR(UINT8 const idx)        { return setIdx(m_rIdx.CCR, idx, NUM_CCR); }
    inline bool setIdx_EE(UINT8 const idx)         { return setIdx(m_rIdx.EE, idx, NUM_EE); }
    inline bool setIdx_NR3D(UINT8 const idx)       { return setIdx(m_rIdx.NR3D, idx, NUM_NR3D); }
    inline bool setIdx_MFB(UINT8 const idx)        { return setIdx(m_rIdx.MFB, idx, NUM_MFB); }

public:     ////    Get Index
    inline UINT8 getIdx_OBC()       const { return m_rIdx.OBC; }
    inline UINT8 getIdx_BPC()       const { return m_rIdx.BPC; }
    inline UINT8 getIdx_NR1()       const { return m_rIdx.NR1; }
    inline UINT8 getIdx_LSC()       const { return m_rIdx.LSC; }
    inline UINT8 getIdx_CFA()       const { return m_rIdx.CFA; }
    inline UINT8 getIdx_CCM()       const { return m_rIdx.CCM; }
    inline UINT8 getIdx_GGM()       const { return m_rIdx.GGM; }
    inline UINT8 getIdx_ANR()       const { return m_rIdx.ANR; }
    inline UINT8 getIdx_CCR()       const { return m_rIdx.CCR; }
    inline UINT8 getIdx_EE()        const { return m_rIdx.EE; }
    inline UINT8 getIdx_NR3D()      const { return m_rIdx.NR3D; }
    inline UINT8 getIdx_MFB()       const { return m_rIdx.MFB; }


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Data.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    inline ISP_NVRAM_OBC_T&         getOBC() { return m_rRegs.OBC[getIdx_OBC()]; }
    inline ISP_NVRAM_BPC_T&         getBPC() { return m_rRegs.BPC[getIdx_BPC()]; }
    inline ISP_NVRAM_NR1_T&         getNR1() { return m_rRegs.NR1[getIdx_NR1()]; }
    inline ISP_NVRAM_LSC_T&         getLSC() { return m_rRegs.LSC[getIdx_LSC()]; }
    inline ISP_NVRAM_CFA_T&         getCFA() { return m_rRegs.CFA[getIdx_CFA()]; }
    inline ISP_NVRAM_CCM_T&         getCCM() { return m_rRegs.CCM[getIdx_CCM()]; }
    inline ISP_NVRAM_GGM_T&         getGGM() { return m_rRegs.GGM[getIdx_GGM()]; }
    inline ISP_NVRAM_ANR_T&         getANR() { return m_rRegs.ANR[getIdx_ANR()]; }
    inline ISP_NVRAM_CCR_T&         getCCR() { return m_rRegs.CCR[getIdx_CCR()]; }
    inline ISP_NVRAM_EE_T&          getEE()  { return m_rRegs.EE[getIdx_EE()]; }
    inline ISP_NVRAM_NR3D_T&        getNR3D(){ return m_rRegs.NR3D[getIdx_NR3D()]; }
    inline ISP_NVRAM_MFB_T&         getMFB() { return m_rRegs.MFB[getIdx_MFB()]; }

    inline ISP_NVRAM_OBC_T&         getOBC(UINT8 const idx) { return m_rRegs.OBC[idx]; }
    inline ISP_NVRAM_BPC_T&         getBPC(UINT8 const idx) { return m_rRegs.BPC[idx]; }
    inline ISP_NVRAM_NR1_T&         getNR1(UINT8 const idx) { return m_rRegs.NR1[idx]; }
    inline ISP_NVRAM_LSC_T&         getLSC(UINT8 const idx) { return m_rRegs.LSC[idx]; }
    inline ISP_NVRAM_CFA_T&         getCFA(UINT8 const idx) { return m_rRegs.CFA[idx]; }
    inline ISP_NVRAM_CCM_T&         getCCM(UINT8 const idx) { return m_rRegs.CCM[idx]; }
    inline ISP_NVRAM_GGM_T&         getGGM(UINT8 const idx) { return m_rRegs.GGM[idx]; }
    inline ISP_NVRAM_ANR_T&         getANR(UINT8 const idx) { return m_rRegs.ANR[idx]; }
    inline ISP_NVRAM_CCR_T&         getCCR(UINT8 const idx) { return m_rRegs.CCR[idx]; }
    inline ISP_NVRAM_EE_T&          getEE(UINT8 const idx)  { return m_rRegs.EE[idx]; }
    inline ISP_NVRAM_NR3D_T&        getNR3D(UINT8 const idx){ return m_rRegs.NR3D[idx]; }
    inline ISP_NVRAM_MFB_T&         getMFB(UINT8 const idx) { return m_rRegs.MFB[idx]; }


private:    ////    Data Members.
    ISP_NVRAM_REGISTER_STRUCT&      m_rRegs;
    ISP_NVRAM_REG_INDEX_STRUCT&     m_rIdx;
};

/*******************************************************************************
*
********************************************************************************/

typedef struct
{
    UINT32 Version;
    FOCUS_RANGE_T rFocusRange;
    AF_NVRAM_T    rAFNVRAM;
    UINT8 reserved[MAXIMUM_NVRAM_CAMERA_LENS_FILE_SIZE-sizeof(UINT32)-sizeof(FOCUS_RANGE_T)-sizeof(AF_NVRAM_T)];
} NVRAM_LENS_PARA_STRUCT, *PNVRAM_LENS_PARA_STRUCT;


/*******************************************************************************
*
********************************************************************************/
#define CAL_GET_DEFECT_FLAG     0x01
#define CAL_GET_3ANVRAM_FLAG    0x02
#define CAL_GET_SHADING_FLAG    0x04
#define CAL_GET_PARA_FLAG       0x08
#define CAL_DATA_LOAD           0x6C6F6164//"load"
#define CAL_DATA_UNLOAD         0x00000000
#define CAL_SHADING_TYPE_SENSOR 0x216D746B//"!mtk"
#define CAL_SHADING_TYPE_ISP    0x3D6D746B//"=mtk"

typedef struct
{
//    PNVRAM_CAMERA_DEFECT_STRUCT     pCameraDefect;
    PNVRAM_CAMERA_SHADING_STRUCT    pCameraShading;
    PNVRAM_CAMERA_ISP_PARAM_STRUCT  pCameraPara;
    AWB_GAIN_T 						rCalGain;
} GET_SENSOR_CALIBRATION_DATA_STRUCT, *PGET_SENSOR_CALIBRATION_DATA_STRUCT;

/*******************************************************************************
*
********************************************************************************/
typedef enum
{
    CAMERA_NVRAM_DATA_ISP = 0,
    CAMERA_NVRAM_DATA_3A,
    CAMERA_NVRAM_DATA_SHADING,
    CAMERA_NVRAM_DATA_LENS,
    CAMERA_DATA_AE_PLINETABLE,
    CAMERA_NVRAM_DATA_STROBE,
    CAMERA_DATA_TYPE_NUM
} CAMERA_DATA_TYPE_ENUM;

typedef enum
{
    GET_CAMERA_DATA_NVRAM,
    GET_CAMERA_DATA_DEFAULT,
    SET_CAMERA_DATA_NVRAM,
} MSDK_CAMERA_NVRAM_DATA_CTRL_CODE_ENUM;

#endif // _CAMERA_CUSTOM_NVRAM_H_

