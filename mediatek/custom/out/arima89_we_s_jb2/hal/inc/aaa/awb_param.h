

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

#ifndef _AWB_PARAM_H
#define _AWB_PARAM_H

#include <awb_feature.h>

//____General____
#define AWB_LOG_TABLE1_SIZE (256)
#define AWB_LOG_TABLE2_SIZE (2048)
#define AWB_LOG_TABLE2_INDEX_MAX (AWB_LOG_TABLE2_SIZE - 1)
#define AWB_LOG_TABLE2_INDEX_MIN (52) // i.e. around 1/10

#define AWB_LV_INDEX_UNIT (10) // 1.0 LV
#define AWB_LV_INDEX_MIN  (0)  // LV 0
#define AWB_LV_INDEX_MAX  (18) // LV 18
#define AWB_LV_INDEX_NUM ((AWB_LV_INDEX_MAX - AWB_LV_INDEX_MIN) + 1)

//____Parent Block Config____
#define AWB_WINDOW_NUM_X (120)
#define AWB_WINDOW_NUM_Y (90)
#define AWB_WINDOW_NUM (AWB_WINDOW_NUM_X * AWB_WINDOW_NUM_Y)
#define AWB_PARENT_BLK_SIZE_X (5)
#define AWB_PARENT_BLK_SIZE_Y (5)
#define AWB_PARENT_BLK_SIZE (AWB_PARENT_BLK_SIZE_X * AWB_PARENT_BLK_SIZE_Y)
#define AWB_PARENT_BLK_NUM_X (AWB_WINDOW_NUM_X/AWB_PARENT_BLK_SIZE_X)
#define AWB_PARENT_BLK_NUM_Y (AWB_WINDOW_NUM_Y/AWB_PARENT_BLK_SIZE_Y)
#define AWB_PARENT_BLK_NUM (AWB_PARENT_BLK_NUM_X * AWB_PARENT_BLK_NUM_Y)


#define AWB_DAYLIGHT_LOCUS_OFFSET_INDEX_UNIT (500)
#define AWB_DAYLIGHT_LOCUS_OFFSET_INDEX_NUM (21)
#define AWB_DAYLIGHT_LOCUS_OFFSET_INDEX_MAX (AWB_DAYLIGHT_LOCUS_OFFSET_INDEX_NUM - 1)

#define AWB_DAYLIGHT_LOCUS_NEW_OFFSET_INDEX_UNIT (500)
#define AWB_DAYLIGHT_LOCUS_NEW_OFFSET_INDEX_NUM (21)
#define AWB_DAYLIGHT_LOCUS_NEW_OFFSET_INDEX_MAX (AWB_DAYLIGHT_LOCUS_NEW_OFFSET_INDEX_NUM - 1)

#define AWB_WARM_FLUORESCENT_GREEN_OFFSET_INDEX_UNIT (200)
#define AWB_WARM_FLUORESCENT_GREEN_OFFSET_INDEX_NUM (11)
#define AWB_WARM_FLUORESCENT_GREEN_OFFSET_INDEX_MAX (AWB_WARM_FLUORESCENT_GREEN_OFFSET_INDEX_NUM - 1)

//#define AWB_F2_PARENT_BLK_NUM_THR_PERCENT ((UINT32)(30)) // 30 % of total parent block number
//#define AWB_NEUTRAL_PARENT_BLK_NUM_THR ((MUINT32)(10)) // 10 % of total parent block number
#define AWB_NIGHT_SCENE_LV_THR (50) // LV 5

#define AWB_TUNGSTEN_MAGENTA_OFFSET_INDEX_UNIT (100)
#define AWB_TUNGSTEN_MAGENTA_OFFSET_INDEX_NUM (11)
#define AWB_TUNGSTEN_MAGENTA_OFFSET_INDEX_MAX (AWB_TUNGSTEN_MAGENTA_OFFSET_INDEX_NUM - 1)

#define AWB_SHADE_GREEN_OFFSET_INDEX_UNIT (100)
#define AWB_SHADE_GREEN_OFFSET_INDEX_NUM (11)
#define AWB_SHADE_GREEN_OFFSET_INDEX_MAX (AWB_SHADE_GREEN_OFFSET_INDEX_NUM - 1)

#define AWB_SHADE_MAGENTA_OFFSET_INDEX_UNIT (100)
#define AWB_SHADE_MAGENTA_OFFSET_INDEX_NUM (11)
#define AWB_SHADE_MAGENTA_OFFSET_INDEX_MAX (AWB_TUNGSTEN_MAGENTA_OFFSET_INDEX_NUM - 1)

#define AWB_SPEED_MIN (0)
#define AWB_SPEED_MAX (100)

#define AWB_LIGHT_WEIGHT_UNIT (256) // 1.0 = 256

//____Auto White Balance Config____
#define AWB_SCALE_UNIT (512) // 1.0 = 512

//____Preset White Balance Config____
#define PWB_LIGHT_AREA_NUM (2)

#define PWB_NEUTRAL_AREA_INDEX (0)
#define PWB_REFERENCE_AREA_INDEX (1)

//____Strobe AWB____
#define STROBE_AWB_FOREGROUND_FLASH_ONLY_Y_THR_PERCENT (50) // 50 % of flash target Y
#define STROBE_AWB_WEIGHTED_FOREGROUND_PARENT_BLK_NUM_THR_PERCENT (10) // 10 % of total parent block number
#define STROBE_AWB_FOREGROUND_PARENT_BLK_NUM_THR_PERCENT (10) // 10 % of total parent block number

// AWB mode
typedef enum
{
	AWB_SENSOR_MODE_PREVIEW = 0,
	AWB_SENSOR_MODE_VIDEO,
    AWB_SENSOR_MODE_CAPTURE,
	AWB_SENSOR_MODE_NUM
} AWB_SENSOR_MODE_T;

// Strobe mode
typedef enum
{
	AWB_STROBE_MODE_ON = 0,
    AWB_STROBE_MODE_OFF,
	AWB_STROBE_MODE_NUM
} AWB_STROBE_MODE_T;

// Strobe mode
typedef enum
{
	AWB_STATE_PREVIEW = 0,
    AWB_STATE_AF,    
	AWB_STATE_PRECAPTURE,
	AWB_STATE_CAPTURE,
	AWB_STATE_NUM
} AWB_STATE_T;

// Light source definition
typedef enum
{
	AWB_LIGHT_STROBE = 0,                  // Strobe
	AWB_LIGHT_TUNGSTEN,                    // Tungsten
    AWB_LIGHT_WARM_FLUORESCENT,            // Warm fluorescent
    AWB_LIGHT_FLUORESCENT,                 // Fluorescent (TL84)
	AWB_LIGHT_CWF,                         // CWF
	AWB_LIGHT_DAYLIGHT,                    // Daylight
    AWB_LIGHT_SHADE,                       // Shade
	AWB_LIGHT_DAYLIGHT_FLUORESCENT,        // Daylight fluorescent
	AWB_LIGHT_NUM,                         // Light source number
    AWB_LIGHT_NONE = AWB_LIGHT_NUM,        // None: not neutral block
	//AWB_LIGHT_DONT_CARE = 0xFF             // Don't care: don't care the light source of block
} AWB_LIGHT_T;

// Preset white balance light source definition
typedef enum
{
	PWB_DAYLIGHT = 0,         // Preset white balance: Daylight
	PWB_CLOUDY_DAYLIGHT,      // Preset white balance: Cloudy daylight
	PWB_SHADE,                // Preset white balance: Shade
    PWB_TWILIGHT,             // Preset white balance: Twilight
	PWB_FLUORESCENT,          // Preset white balance: Fluorescent
    PWB_WARM_FLUORESCENT,     // Preset white balance: Warm fluorescent
	PWB_INCANDESCENT,         // Preset white balance: Incandescent
	PWB_LIGHT_NUM             // Preset white balance light source number
} PWB_LIGHT_T;

// Light source probability
typedef struct
{
	MINT32 i4P0[AWB_LIGHT_NUM]; // Probability 0
	MINT32 i4P1[AWB_LIGHT_NUM]; // Probability 1
	MINT32 i4P2[AWB_LIGHT_NUM]; // Probability 2
	MINT32 i4P[AWB_LIGHT_NUM];  // Probability
} AWB_LIGHT_PROBABILITY_T;

// AWB speed mode definition
typedef enum
{
	AWB_SPEED_MODE_ONESHOT = 0, // full AWB compensation for half-push/capture
    AWB_SPEED_MODE_SMOOTH_TRANSITION = 1  // partial AWB compensation for preview/movie
} AWB_SPEED_MODE_T;

// AWB statistics info
typedef struct
{
	MINT32 i4SensorWidth[AWB_SENSOR_MODE_NUM];   // Sensor width
    MINT32 i4SensorHeight[AWB_SENSOR_MODE_NUM];  // Sensor height
	MINT32 i4NumX[AWB_SENSOR_MODE_NUM];          // AWB window number (Horizontal)
	MINT32 i4NumY[AWB_SENSOR_MODE_NUM];          // AWB window number (Vertical)
	MINT32 i4SizeX[AWB_SENSOR_MODE_NUM];         // AWB window size (Horizontal)
	MINT32 i4SizeY[AWB_SENSOR_MODE_NUM];         // AWB window size (Vertical)
	MINT32 i4PitchX[AWB_SENSOR_MODE_NUM];        // AWB window pitch (Horizontal)
	MINT32 i4PitchY[AWB_SENSOR_MODE_NUM];        // AWB window pitch (Vertical)
	MINT32 i4OriginX[AWB_SENSOR_MODE_NUM];       // AWB window origin (Horizontal)
	MINT32 i4OriginY[AWB_SENSOR_MODE_NUM];       // AWB window origin (Vertical)
} AWB_WINDOW_CONFIG_T;

// Parent block statistics
typedef struct
{
	MINT32 i4SumR[AWB_PARENT_BLK_NUM_Y][AWB_PARENT_BLK_NUM_X]; // R summation of specified light source of specified parent block
	MINT32 i4SumG[AWB_PARENT_BLK_NUM_Y][AWB_PARENT_BLK_NUM_X]; // G summation of specified light source of specified parent block
	MINT32 i4SumB[AWB_PARENT_BLK_NUM_Y][AWB_PARENT_BLK_NUM_X]; // B summation of specified light source of specified parent block
	MINT32 i4ChildBlkNum[AWB_PARENT_BLK_NUM_Y][AWB_PARENT_BLK_NUM_X]; // Child block number of specified light source of specified parent block
	MINT32 i4Light[AWB_PARENT_BLK_NUM_Y][AWB_PARENT_BLK_NUM_X]; // Light source of specified parent block
} AWB_PARENT_BLK_STAT_T;

// Statistics of child blocks within specified parent block
typedef struct
{
	MINT32 i4SumR[AWB_LIGHT_NUM+1];   // Child block R summation of specified light (+1 for non-neutral color)
	MINT32 i4SumG[AWB_LIGHT_NUM+1];   // Child block G summation of specified light (+1 for non-neutral color)
	MINT32 i4SumB[AWB_LIGHT_NUM+1];   // Child block B summation of specified light (+1 for non-neutral color)
	MINT32 i4BlkNum[AWB_LIGHT_NUM+1]; // Child block number of specified light (+1 for non-neutral color)
} AWB_CHILD_BLK_STAT_T;

// AWB window statistics: RGBX
typedef struct
{
    MUINT8 ucR; // R average of specified AWB window
	MUINT8 ucG; // G average of specified AWB window
	MUINT8 ucB; // B average of specified AWB window
	MUINT8 ucL; // L
} AWB_MAIN_STAT_T;

typedef union
{
    AWB_MAIN_STAT_T rMainStat;
    MUINT32 u4MainStat;
} AWB_WINDOW_T;

typedef struct
{
	MINT32 i4Xr;
	MINT32 i4Yr;
} AWB_ROTATED_XY_COORDINATE_T;

// Light source statistics
typedef struct
{
//#ifdef PC_SIMU //-----------------------------------------------------------------------------------------------
//	MINT32 i4SumR[AWB_LIGHT_NUM];                 // R Summation of specified light source
//	MINT32 i4SumG[AWB_LIGHT_NUM];                 // G Summation of specified light source
//	MINT32 i4SumB[AWB_LIGHT_NUM];                 // B Summation of specified light source
//	MINT32 i4WindowNum[AWB_LIGHT_NUM];            // Window number of specified light source
//#endif //----------------------------------------------------------------------------------------------------
	MINT32 i4WeightedSumR[AWB_LIGHT_NUM];         // Weighted R summation of specified light source
	MINT32 i4WeightedSumG[AWB_LIGHT_NUM];         // Weighted G summation of specified light source
	MINT32 i4WeightedSumB[AWB_LIGHT_NUM];         // Weighted B summation of specified light source
	MINT32 i4WeightedWindowNum[AWB_LIGHT_NUM];    // Weighted window number of specified light source
	MINT32 i4ParentBlkNum[AWB_LIGHT_NUM];         // Parent block number of specified light source
	MINT32 i4WeightedParentBlkNum[AWB_LIGHT_NUM]; // Weighted parent block number of specified light source
	AWB_ROTATED_XY_COORDINATE_T rRotatedXY[AWB_LIGHT_NUM]; // Rotated XY coordinate (for debug purpose)
	MUINT32 u4CalibratedWeightedSumR[AWB_LIGHT_NUM]; // Calibrated weighted R summation of specified light source
	MUINT32 u4CalibratedWeightedSumG[AWB_LIGHT_NUM]; // Calibrated weighted G summation of specified light source
	MUINT32 u4CalibratedWeightedSumB[AWB_LIGHT_NUM]; // Calibrated weighted B summation of specified light source
} AWB_LIGHT_STAT_T;

#define AWB_LIGHT_AREA_NUM (10)

// AWB statistics config
typedef struct
{
    // Main stat window size
	// [1] must be even
	// [2] max number of pixels in a single window must <= 256K
	// [3] max number of pixels of any single color per window must <= 128K
	MINT32 i4WindowSizeX; // Main stat window horizontal size: min = 4
	MINT32 i4WindowSizeY; // Main stat window vertical size: min = 2

    // Main stat window pitch
	MINT32 i4WindowPitchX; // Main stat window horizontal pitch
	MINT32 i4WindowPitchY; // Main stat window vertical pitch

    // Start coordinate of the first window (upper left)
	MINT32 i4WindowOriginX; // Horizontal origin 1st main stat window
	MINT32 i4WindowOriginY; // Vertical origin of 1st main stat window

    // Number of AWB windows: 1x1 ~ 128x128
	MINT32 i4WindowNumX; // Number of horizontal AWB windows
	MINT32 i4WindowNumY; // Number of vertical AWB windows

    // Thresholds
	MINT32 i4LowThresholdR;  // Error pixel low threshold of R (8-bit)
	MINT32 i4LowThresholdG;  // Error pixel low threshold of G (8-bit)
	MINT32 i4LowThresholdB;  // Error pixel lowthreshold of B (8-bit)

	MINT32 i4HighThresholdR; // Error pixel high threshold of R (8-bit)
	MINT32 i4HighThresholdG; // Error pixel high threshold of G (8-bit)
	MINT32 i4HighThresholdB; // Error pixel high threshold of B (8-bit)

    // Pixel count
   	MINT32 i4PixelCountR; // ROUND((1<<24)/number of red pixels in a window)
   	MINT32 i4PixelCountG; // ROUND((1<<24)/number of green pixels in a window)
   	MINT32 i4PixelCountB; // ROUND((1<<24)/number of blue pixels in a window)

    // Pre-gain maximum limit clipping
   	MINT32 i4PreGainLimitR; // Maximum limit clipping for R color (12-bit: 0xFFF)
   	MINT32 i4PreGainLimitG; // Maximum limit clipping for G color (12-bit: 0xFFF)
   	MINT32 i4PreGainLimitB; // Maximum limit clipping for B color (12-bit: 0xFFF)

    // Pre-gain values
   	MINT32 i4PreGainR; // Pre-gain Value for R color (format: 4.9; 1x = 512)
   	MINT32 i4PreGainG; // Pre-gain Value for G color (format: 4.9; 1x = 512)
   	MINT32 i4PreGainB; // Pre-gain Value for B color (format: 4.9; 1x = 512)

    // AWB error threshold
   	MINT32 i4ErrorThreshold; // Programmable threshold for the allowed total
                             // over-exposed and under-exposed pixels in one main stat window
    // AWB rotation matrix
	MINT32 i4Cos; // COS x 256
	MINT32 i4Sin; // SIN x 256

	// AWB light area
    MINT32 i4AWBXY_WINR[AWB_LIGHT_AREA_NUM]; // Right bound
    MINT32 i4AWBXY_WINL[AWB_LIGHT_AREA_NUM]; // Left bound
    MINT32 i4AWBXY_WIND[AWB_LIGHT_AREA_NUM]; // Lower bound
    MINT32 i4AWBXY_WINU[AWB_LIGHT_AREA_NUM]; // Upper bound

} AWB_STAT_CONFIG_T;

// AWB statistics parameter
typedef struct
{
    // Thresholds
	MINT32 i4LowThresholdR;  // Low threshold of R
	MINT32 i4LowThresholdG;  // Low threshold of G
	MINT32 i4LowThresholdB;  // Low threshold of B

	MINT32 i4HighThresholdR; // High threshold of R
	MINT32 i4HighThresholdG; // High threshold of G
	MINT32 i4HighThresholdB; // High threshold of B

    // Pre-gain maximum limit clipping
   	MINT32 i4PreGainLimitR; // Maximum limit clipping for R color
   	MINT32 i4PreGainLimitG; // Maximum limit clipping for G color
   	MINT32 i4PreGainLimitB; // Maximum limit clipping for B color

    // AWB error threshold
   	MINT32 i4ErrorThreshold; // Programmable threshold for the allowed total
                             // over-exposured and under-exposered pixels in one main stat window
} AWB_STAT_PARAM_T;

// AWB statistics
#define DUMMY_SIZE (((AWB_WINDOW_NUM_X + 3) / 4) * 4) // size of AE window line

typedef struct
{
    AWB_WINDOW_T WIN[AWB_WINDOW_NUM_X];
#ifndef PC_SIMU
	MUINT8 DUMMY[DUMMY_SIZE];
#endif
} AWB_WINDOW_LINE_T;

typedef struct
{
    AWB_WINDOW_LINE_T LINE[AWB_WINDOW_NUM_Y];
} AWB_STAT_T;

// AWB info for ISP tuning
typedef struct
{
    AWB_LIGHT_PROBABILITY_T rProb; // Light source probability
    AWB_LIGHT_STAT_T rLightStat; // Light source statistics
    AWB_LIGHT_SOURCE_AWB_GAIN_T rLightAWBGain; // Golden sample's AWB gain for multi-CCM
    AWB_GAIN_T rCurrentAWBGain; // Current preview AWB gain
    MINT32 i4NeutralParentBlkNum; // Neutral parent block number
	MINT32 i4CCT; // CCT
	MINT32 i4FluorescentIndex; // Fluorescent index
	MINT32 i4DaylightFluorescentIndex; // Daylight fluorescent index
	MINT32 i4SceneLV; // Scene LV
} AWB_INFO_T;

// AWB gain output (for debug purpose)
typedef struct
{
	AWB_GAIN_T rAWBGain;
	AWB_GAIN_T rRAWPreGain2;
	AWB_GAIN_T rStrobeAWBGain;
	AWB_GAIN_T rStrobeRAWPreGain2;
} AWB_GAIN_OUTPUT_T;

// Tungsten specific algorithm data
typedef struct
{
    MINT32 i4RG;                             // Tungsten R/G
	MINT32 i4BG;                             // Tungsten B/G
	MINT32 i4LogRG;                          // Tungsten LOG(R/G)
	MINT32 i4LogBG;                          // Tungsten LOG(B/G)
	MINT32 i4DaylightLocusRG;                // Daylight locus R/G, by projection in log domain
	MINT32 i4DaylightLocusBG;                // Daylight locus B/G, by projection in log domain
	MINT32 i4DaylightLocusLogRG;             // Daylight locus LOG(R/G), by projection in log domain
	MINT32 i4DaylightLocusLogBG;             // Daylight locus LOG(B/G), by projection in log domain
	MINT32 i4DaylightLocusTargetRG;          // Daylight locus target R/G, target for AWB compensation
	MINT32 i4DaylightLocusTargetBG;          // Daylight locus target B/G, target for AWB compensation
	MINT32 i4DaylightLocusTargetLogRG;       // Daylight locus target LOG(R/G), target for AWB compensation
	MINT32 i4DaylightLocusTargetLogBG;       // Daylight locus target LOG(B/G), target for AWB compensation
    MINT32 i4IsAboveDaylightLocus;           // 1: above or on daylight locus, 0: below daylight locus
	MINT32 i4GMOffset;                       // Tungsten green/magenta offset
	MINT32 i4DaylightLocusOffset;            // Daylight locus offset
	MINT32 i4DaylightLocusNewOffset;         // Daylight locus new offset
	MINT32 i4DaylightLocusTargetOffset;      // Daylight locus target offset
	MINT32 i4DaylightLocusTargetOffsetRatio; // Daylight locus target offset ratio
	MINT32 i4Weight;                         // Tungsten light weight
} AWB_TUNGSTEN_ALGORITHM_DATA_T;

// Warm fluorescent specific algorithm data
typedef struct
{
    MINT32 i4RG;                            // Warm fluorescent R/G
	MINT32 i4BG;                            // Warm fluorescent B/G
	MINT32 i4LogRG;                         // Warm fluorescent LOG(R/G)
	MINT32 i4LogBG;                         // Warm fluorescent LOG(B/G)
	MINT32 i4DaylightLocusLogRG;            // Daylight locus LOG(R/G), by projection in log domain
	MINT32 i4DaylightLocusLogBG;            // Daylight locus LOG(B/G), by projection in log domain
	MINT32 i4DaylightLocusTargetRG;         // Daylight locus target R/G, target for AWB compensation
	MINT32 i4DaylightLocusTargetBG;         // Daylight locus target B/G, target for AWB compensation
	MINT32 i4DaylightLocusTargetLogRG;      // Daylight locus target LOG(R/G), target for AWB compensation
	MINT32 i4DaylightLocusTargetLogBG;      // Daylight locus target LOG(B/G), target for AWB compensation
	MINT32 i4GOffsetRG;                     // Green offset R/G
	MINT32 i4GOffsetBG;                     // Green offset B/G
	MINT32 i4GOffsetThr;                     // Green offset threshold
	MINT32 i4GOffsetLogRG;                   // Green offset LOG(R/G)
	MINT32 i4GOffsetLogBG;                   // Green offset LOG(B/G)
	MINT32 i4GOffset;                        // Green offset
	MINT32 i4DaylightLocusOffset;            // Daylight locus offset
	MINT32 i4DaylightLocusNewOffset;         // Daylight locus new offset
	MINT32 i4DaylightLocusTargetOffset;      // Daylight locus target offset
	MINT32 i4DaylightLocusTargetOffsetRatio; // Daylight locus target offset ratio
	MINT32 i4Weight;                         // Warm fluorescent weight
} AWB_WARM_FLUORESCENT_ALGORITHM_DATA_T;

// Shade specific algorithm data
typedef struct
{
    MINT32 i4RG;                        // Shade R/G
	MINT32 i4BG;                        // Shade B/G
	MINT32 i4LogRG;                     // Shade LOG(R/G)
	MINT32 i4LogBG;                     // Shade LOG(B/G)
    MINT32 i4DaylightLocusRG;           // Daylight locus R/G, by projection in log domain
	MINT32 i4DaylightLocusBG;           // Daylight locus B/G, by projection in log domain
 	MINT32 i4DaylightLocusLogRG;        // Daylight locus LOG(R/G), by projection in log domain
	MINT32 i4DaylightLocusLogBG;        // Daylight locus LOG(B/G), by projection in log domain
	MINT32 i4DaylightLocusTargetRG;     // Daylight locus target R/G, target for AWB compensation
	MINT32 i4DaylightLocusTargetBG;     // Daylight locus target B/G, target for AWB compensation
	MINT32 i4DaylightLocusTargetLogRG;  // Daylight locus target LOG(R/G), target for AWB compensation
	MINT32 i4DaylightLocusTargetLogBG;  // Daylight locus target LOG(R/G), target for AWB compensation
	MINT32 i4IsAboveDaylightLocus;      // 1: above or on daylight locus, 0: below daylight locus
	MINT32 i4GMOffset;                  // Shade green/magenta offset
	MINT32 i4DaylightLocusOffset;       // Daylight locus offset
	MINT32 i4DaylightLocusNewOffset;    // Daylight locus new offset
	MINT32 i4DaylightLocusTargetOffset; // Daylight locus target offset
	MINT32 i4Weight;                    // Shade light weight
} AWB_SHADE_ALGORITHM_DATA_T;

// AWB algorithm related data structure
typedef struct
{
	MINT32 i4WBGainR[AWB_LIGHT_NUM]; // Gain R
    MINT32 i4WBGainG[AWB_LIGHT_NUM]; // Gain G
	MINT32 i4WBGainB[AWB_LIGHT_NUM]; // Gain B
	//MINT32 i4Weight[AWB_LIGHT_NUM]; // Light source weight

	// Tungsten specific algorithm data
    AWB_TUNGSTEN_ALGORITHM_DATA_T rTungsten;

	// Warm fluorescent specific algorithm data
    AWB_WARM_FLUORESCENT_ALGORITHM_DATA_T rWF;

	// Shade specific algorithm data
	AWB_SHADE_ALGORITHM_DATA_T rShade;

} AWB_ALGORITHM_DATA_T;

// Strobe AWB algorithm related data structure
typedef struct
{
    AWB_GAIN_T rPresetGain; // Preset strobe AWB gain after AWB calibration
    AWB_GAIN_T rNonStrobeGain; // Non-strobe AWB gain
    AWB_GAIN_T rAWBGain; // Strobe AWB gain
} STROBE_AWB_ALGORITHM_DATA_T;

// PWB algorithm statistics
typedef struct
{
	MINT32 i4NeutralAreaParentBlkNum; // Parent block number of neutral area
	MINT32 i4ReferenceAreaParentBlkNum; // Parent block number of reference area
	MINT32 i4ParentBlkNum;  // Parent block number of neutral area + reference area
	AWB_GAIN_T rDefaultGain; // Default gain
	AWB_GAIN_T rWBGain_NeutralArea; // WB gain of neutral area
	AWB_GAIN_T rWBGain_ReferenceArea; // WB gain of reference area
	PWB_LIGHT_T ePWBLight; // PWB light source
	MINT32 i4Xo; // reference area original X coordinate
	MINT32 i4Yo; // reference area original Y coordinate
	MINT32 i4Xor; // reference area original X coordinate (rotated)
	MINT32 i4Yor; // reference area original Y coordinate (rotated)
	MINT32 i4Xpr; // reference area projected X coordinate (rotated)
	MINT32 i4Ypr; // reference area projected Y coordinate (rotated)
	MINT32 i4Xp; // reference area projected X coordinate
	MINT32 i4Yp; // reference area projected Y coordinate
	MINT32 i4CosInv; // Inverse rotation matrix
	MINT32 i4SinInv; // Inverse rotation matrix
} PWB_ALGORITHM_DATA_T;

// CCT estimation
typedef struct
{
    MINT32 i4LogRG;
	MINT32 i4LogBG;
	MINT32 i4X;
    MINT32 i4Y;
    MINT32 i4XR;
	MINT32 i4YR;
	//MINT32 i4CalLogRG;
	//MINT32 i4CalLogBG;
	//MINT32 i4CalX;
    //MINT32 i4CalY;
    //MINT32 i4CalXR;
	//MINT32 i4CalYR;
	//MINT32 i4CCTXR;
	//MINT32 i4CCTYR;
    MINT32 i4MIRED_H;
    MINT32 i4MIRED_L;
    MINT32 i4MIRED;
    MINT32 i4YR_TL84;
    MINT32 i4YR_CWF;
	MINT32 i4YR_Mean_F;
    MINT32 i4YR_D65;
    MINT32 i4YR_DF;
	MINT32 i4YR_Mean_DF;
} CCT_INFO_T;

//____AWB Algorithm Parameters____

// Chip dependent parameter
typedef struct
{
    MINT32 i4AWBGainOutputScaleUnit; // AWB gain output scale unit
	MINT32 i4AWBGainOutputUpperLimit; // AWB gain output upper limit
	MINT32 i4RotationMatrixUnit; // Rotation matrix unit
} AWB_CHIP_PARAM_T;

// AWB light source probability look-up table
typedef struct
{
	MINT32 i4SizeX; // AWB light source probability look-up table horizontal dimension
	MINT32 i4SizeY; // AWB light source probability look-up table vertical dimension
	MINT32 i4LUT[AWB_LIGHT_NUM][AWB_LV_INDEX_NUM];  // AWB light source probability look-up table
} AWB_LIGHT_PROBABILITY_LUT_T;

// AWB convergence parameter
typedef struct
{
	MINT32 i4Speed; // Convergence speed
	MINT32 i4StableThr; // Stable threshold
} AWB_CONVERGENCE_PARAM_T;

// AWB daylight locus target offset ratio LUT for tungsten and fluorescent0
typedef struct
{
	MINT32 i4LUTSize; // LUT dimension
	MINT32 i4LUT[AWB_DAYLIGHT_LOCUS_NEW_OFFSET_INDEX_NUM]; // Look-up table
} AWB_DAYLIGHT_LOCUS_TARGET_OFFSET_RATIO_T;

// AWB green offset threshold LUT for fluorescent0
typedef struct
{
	MINT32 i4LUTSize; // LUT dimension
	MINT32 i4LUT[AWB_DAYLIGHT_LOCUS_OFFSET_INDEX_NUM]; // Look-up table
} AWB_GREEN_OFFSET_THRESHOLD_LUT_T;

// AWB tungsten light weight LUT
typedef struct
{
	MINT32 i4LUTSize; // LUT dimension
	MINT32 i4LUT[AWB_TUNGSTEN_MAGENTA_OFFSET_INDEX_NUM]; // Look-up table
} AWB_LIGHT_WEIGHT_LUT_TUNGSTEN_T;

// AWB warm fluorescent weight LUT
typedef struct
{
	MINT32 i4LUTSize; // LUT dimension
	MINT32 i4LUT[AWB_WARM_FLUORESCENT_GREEN_OFFSET_INDEX_NUM]; // Look-up table
} AWB_LIGHT_WEIGHT_LUT_WARM_FLUORESCENT_T;

// AWB shade light weight LUT
typedef struct
{
	MINT32 i4MagentaLUTSize; // LUT dimension
	MINT32 i4MagentaLUT[AWB_SHADE_MAGENTA_OFFSET_INDEX_NUM]; // Look-up table
	MINT32 i4GreenLUTSize; // LUT dimension
	MINT32 i4GreenLUT[AWB_SHADE_GREEN_OFFSET_INDEX_NUM]; // Look-up table
} AWB_LIGHT_WEIGHT_LUT_SHADE_T;

// AWB one-shot parameter
typedef struct
{
	MBOOL bSmoothEnable; // Enable smooth one-shot AWB for dark environment: take weighted average of one-shot AWB gain and preview AWB gain based on scene LV
	MINT32 i4LVThrL; // Low LV threshold: take preview AWB gain only when scene LV <= i4LVThrL
	MINT32 i4LVThrH; // High LV threshold: take one-shot AWB gain only when scene LV >= i4LVThrH
	                 // Perform interpolation when i4LVThrH > LV > i4LVThrL
} AWB_ONE_SHOT_T;

// Parent block weight parameter used in light source statistics
typedef struct
{
	MBOOL bEnable; // Enable parent block weight
	MINT32 i4ScalingFactor; // 6: 1~12, 7: 1~6, 8: 1~3, 9: 1~2, >=10: 1
} AWB_PARENT_BLOCK_WEIGHT_T;

// LV threshold for AWB gain prediction
typedef struct
{
	MINT32 i4IntermediateSceneLvThr_L1;
    MINT32 i4IntermediateSceneLvThr_H1;
	MINT32 i4IntermediateSceneLvThr_L2;
    MINT32 i4IntermediateSceneLvThr_H2;
	MINT32 i4DaylightLocusLvThr_L;
    MINT32 i4DaylightLocusLvThr_H;
} AWB_LV_THR_T;

typedef struct
{
	AWB_LV_THR_T rStrobe;
	AWB_LV_THR_T rTungsten;
	AWB_LV_THR_T rWarmFluorescent;
	AWB_LV_THR_T rFluorescent;
	AWB_LV_THR_T rCWF;
	AWB_LV_THR_T rDaylight;
	AWB_LV_THR_T rDaylightFluorescent;
	AWB_LV_THR_T rShade;
} AWB_GAIN_PREDICTION_T;

// Neutral parent block number threshold
typedef struct
{
	MINT32 m_i4AWBGainPredict[AWB_LV_INDEX_NUM]; // unit: %
    MINT32 m_i4CWF[AWB_LV_INDEX_NUM]; // unit: %
	MINT32 m_i4DF[AWB_LV_INDEX_NUM]; // unit: %
} AWB_NEUTRAL_PARENT_BLK_NUM_THR_T;

// AWB algorithm parameter
typedef struct
{
	AWB_CHIP_PARAM_T rChipParam;
    AWB_LIGHT_PROBABILITY_LUT_T rLightProb;
    AWB_CONVERGENCE_PARAM_T rConvergence;
    AWB_DAYLIGHT_LOCUS_TARGET_OFFSET_RATIO_T rDaylightLocusTargetOffsetRatio_Tungsten;
    AWB_DAYLIGHT_LOCUS_TARGET_OFFSET_RATIO_T rDaylightLocusTargetOffsetRatio_WF;
    AWB_GREEN_OFFSET_THRESHOLD_LUT_T rGreenOffsetThr;
    AWB_LIGHT_WEIGHT_LUT_TUNGSTEN_T rLightWeight_Tungsten;
    AWB_LIGHT_WEIGHT_LUT_WARM_FLUORESCENT_T rLightWeight_WF;
    AWB_LIGHT_WEIGHT_LUT_SHADE_T rLightWeight_Shade;
    AWB_ONE_SHOT_T rOneShotParam;
	AWB_PARENT_BLOCK_WEIGHT_T rParentBlkWeightParam;
	AWB_GAIN_PREDICTION_T rAWBGainPredictParam;
	AWB_NEUTRAL_PARENT_BLK_NUM_THR_T rNeutralParentBlkNumThr;
} AWB_PARAM_T;

// AWB ASD info data structure
typedef struct
{
    MINT32 i4AWBRgain_X128;     // AWB Rgain
    MINT32 i4AWBBgain_X128;     // AWB Bgain
    MINT32 i4AWBRgain_D65_X128; // AWB Rgain (D65; golden sample)
    MINT32 i4AWBBgain_D65_X128; // AWB Bgain (D65; golden sample)
    MINT32 i4AWBRgain_CWF_X128; // AWB Rgain (CWF; golden sample)
    MINT32 i4AWBBgain_CWF_X128; // AWB Bgain (CWF; golden sample)
    MBOOL bAWBStable;           // AWB stable
} AWB_ASD_INFO_T;

// AWB init input parameters
typedef struct
{
    AWB_NVRAM_T rAWBNVRAM;       // AWB NVRAM param
    AWB_PARAM_T rAWBParam;       // AWB param
    AWB_STAT_PARAM_T rAWBStatParam; // AWB statistics param
    LIB3A_AWB_MODE_T eAWBMode; // AWB mode
	//MBOOL bIsZsdEnable;          // ZSD enable
	//MBOOL bIsStrobeFired;        // ZSD enable
	MINT32 i4SensorMode;         // Sensor mode
    MINT32 i4PvSensorWidth;      // Preview sensor width
    MINT32 i4PvSensorHeight;     // Preview sensor height
    MINT32 i4VideoSensorWidth;   // Video sensor width
    MINT32 i4VideoSensorHeight;  // Video sensor height
    MINT32 i4CapSensorWidth;     // Capture sensor width
    MINT32 i4CapSensorHeight;    // Capture sensor height
} AWB_INIT_INPUT_T;

// AWB frame-based input parameters
typedef struct
{
	MVOID* pAWBStatBuf; // Statistic buffer address
	AWB_SPEED_MODE_T eAWBSpeedMode; // AWB speed mode
	MINT32 i4SceneLV; // Scene LV
	MINT32 i4SensorMode; // Sensor mode
	MINT32 i4AWBState; // AWB state
	MBOOL bIsStrobeFired; // AWB for AF lamp or pre-flash
} AWB_INPUT_T;

// AWB algorithm output (used for lib3a to mhal interface)
typedef struct
{
	AWB_GAIN_T rPreviewAWBGain;       // AWB gain for DIP
	AWB_GAIN_T rPreviewStrobeAWBGain; // AWB gain for DIP
	AWB_GAIN_T rCaptureAWBGain;       // AWB gain for DIP
	AWB_GAIN_T rPreviewRAWPreGain2;          // AWB gain for AE statistics
	AWB_GAIN_T rPreviewStrobeRAWPreGain2;    // AWB gain for AE statistics
	AWB_INFO_T rAWBInfo;                     // AWB info
} AWB_OUTPUT_T;

#endif

