
#include <utils/Log.h>
#include <fcntl.h>
#include <math.h>

#include "camera_custom_nvram.h"
#include "camera_custom_sensor.h"
#include "image_sensor.h"
#include "kd_imgsensor_define.h"
#include "camera_AE_PLineTable_ov5648mipiraw.h"
#include "camera_info_ov5648mipiraw.h"
#include "camera_custom_AEPlinetable.h"

const NVRAM_CAMERA_ISP_PARAM_STRUCT CAMERA_ISP_DEFAULT_VALUE =
{{
    //Version
    Version: NVRAM_CAMERA_PARA_FILE_VERSION,

    //SensorId
    SensorId: SENSOR_ID,
    ISPComm:{
        {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    	}
    },
    ISPPca: {
        #include INCLUDE_FILENAME_ISP_PCA_PARAM
    },
    ISPRegs:{
        #include INCLUDE_FILENAME_ISP_REGS_PARAM
    },
    ISPMfbMixer:{{
        {//00: MFB mixer for ISO 100
            0x00000000, 0x00000000
        },
        {//01: MFB mixer for ISO 200
            0x00000000, 0x00000000
        },
        {//02: MFB mixer for ISO 400
            0x00000000, 0x00000000
        },
        {//03: MFB mixer for ISO 800
            0x00000000, 0x00000000
        },
        {//04: MFB mixer for ISO 1600
            0x00000000, 0x00000000
        },
        {//05: MFB mixer for ISO 2400
            0x00000000, 0x00000000
        },
        {//06: MFB mixer for ISO 3200
            0x00000000, 0x00000000
        }
    }},
    ISPCcmPoly22:{
        75170, // i4R_AVG
        13190, // i4R_STD
        79140, // i4B_AVG
        26270, // i4B_STD
        { // i4P00[9]
            4448648, -1494813, -393843, -604477, 3414513, -250036,  85095, -1385454, 3860283 
        },
        { // i4P10[9]
            933698,  -628943, -304758, -247520,  -22220,  269740, -73861,   196166, -122555
        },
        { // i4P01[9]
            814367,  -494023, -320352, -358410, -180556,  538966, -57406,  -190454,  247689
        },
        { // i4P20[9]
            394007,  -491950,   98031,  -21525,   59812,  -38287, 140879,  -521951,  381045
        },
        { // i4P11[9]
            -35750,  -344806,  380738,  121574,   59500, -181074, 143388,  -309535,  166309
        },
        { // i4P02[9]
            -315751,    65233,  250618,  151463,   34149, -185612,  21808,    -8637,  -12997
        }        
    }
}};

const NVRAM_CAMERA_3A_STRUCT CAMERA_3A_NVRAM_DEFAULT_VALUE =
{
    NVRAM_CAMERA_3A_FILE_VERSION, // u4Version
    SENSOR_ID, // SensorId

    // AE NVRAM
    {
        // rDevicesInfo
        {
            1195,   // u4MinGain, 1024 base =  1x
            16384,  // u4MaxGain, 16x
            55,     // u4MiniISOGain, ISOxx
            128,    // u4GainStepUnit, 1x/8
            33,     // u4PreExpUnit
            30,     // u4PreMaxFrameRate
            33,     // u4VideoExpUnit
            30,     // u4VideoMaxFrameRate
            1024,   // u4Video2PreRatio, 1024 base = 1x
            58,     // u4CapExpUnit
            15,     // u4CapMaxFrameRate
            1024,   // u4Cap2PreRatio, 1024 base = 1x
            28,      // u4LensFno, Fno = 2.8
            350     // u4FocusLength_100x
         },
         // rHistConfig
        {
            2,   // u4HistHighThres
            40,  // u4HistLowThres
            2,   // u4MostBrightRatio
            1,   // u4MostDarkRatio
            160, // u4CentralHighBound
            20,  // u4CentralLowBound
            {240, 230, 220, 210, 200}, // u4OverExpThres[AE_CCT_STRENGTH_NUM]
            {86, 108, 128, 148, 170},  // u4HistStretchThres[AE_CCT_STRENGTH_NUM]
            {18, 22, 26, 30, 34}       // u4BlackLightThres[AE_CCT_STRENGTH_NUM]
        },
        // rCCTConfig
        {
            TRUE,            // bEnableBlackLight
            TRUE,            // bEnableHistStretch
            FALSE,           // bEnableAntiOverExposure
            TRUE,            // bEnableTimeLPF
            TRUE,            // bEnableCaptureThres
            TRUE,            // bEnableVideoThres
            TRUE,            // bEnableStrobeThres
            47,                // u4AETarget
            47,                // u4StrobeAETarget

            50,                // u4InitIndex
            4,                 // u4BackLightWeight
            32,                // u4HistStretchWeight
            4,                 // u4AntiOverExpWeight
            2,                 // u4BlackLightStrengthIndex
            2,                 // u4HistStretchStrengthIndex
            2,                 // u4AntiOverExpStrengthIndex
            2,                 // u4TimeLPFStrengthIndex
            {1, 3, 5, 7, 8}, // u4LPFConvergeTable[AE_CCT_STRENGTH_NUM]
            90,                // u4InDoorEV = 9.0, 10 base
            -10,               // i4BVOffset delta BV = -2.3
            4,                 // u4PreviewFlareOffset
            4,                 // u4CaptureFlareOffset
            5,                 // u4CaptureFlareThres
            4,                 // u4VideoFlareOffset
            5,                 // u4VideoFlareThres
            2,                 // u4StrobeFlareOffset
            2,                 // u4StrobeFlareThres
            8,                 // u4PrvMaxFlareThres
            0,                 // u4PrvMinFlareThres
            8,                 // u4VideoMaxFlareThres
            0,                 // u4VideoMinFlareThres            
            18,                // u4FlatnessThres              // 10 base for flatness condition.
            75                 // u4FlatnessStrength
         }
    },

    // AWB NVRAM
    {
    	// AWB calibration data
    	{
    		// rUnitGain (unit gain: 1.0 = 512)
    		{
    			0,	// i4R
    			0,	// i4G
    			0	// i4B
    		},
    		// rGoldenGain (golden sample gain: 1.0 = 512)
    		{
	            0,	// i4R
	            0,	// i4G
	            0	// i4B
            },
    		// rTuningUnitGain (Tuning sample unit gain: 1.0 = 512)
    		{
	            0,	// i4R
	            0,	// i4G
	            0	// i4B
            },
            // rD65Gain (D65 WB gain: 1.0 = 512)
            {
                816,    // i4R
                512,    // i4G
                564    // i4B
    		}
    	},
    	// Original XY coordinate of AWB light source
    	{
           // Strobe
            {
                137,    // i4X
                -208    // i4Y
            },
            // Horizon
            {
                -389,    // i4X
                -342    // i4Y
            },
            // A
            {
                -282,    // i4X
                -343    // i4Y
            },
            // TL84
            {
                -142,    // i4X
                -315    // i4Y
            },
            // CWF
            {
                -102,    // i4X
                -426    // i4Y
            },
            // DNP
            {
                5,    // i4X
                -257    // i4Y
            },
            // D65
            {
                137,    // i4X
                -208    // i4Y
            },
            // DF
            {
                0,    // i4X
                0    // i4Y
            }
    	},
    	// Rotated XY coordinate of AWB light source
    	{
            // Strobe
            {
                74,    // i4X
                -238    // i4Y
            },
            // Horizon
            {
                -469,    // i4X
                -221    // i4Y
            },
            // A
            {
                -366,    // i4X
                -251    // i4Y
            },
            // TL84
            {
                -224,    // i4X
                -263    // i4Y
            },
            // CWF
            {
                -216,    // i4X
                -381    // i4Y
            },
            // DNP
            {
                -66,    // i4X
                -248    // i4Y
            },
            // D65
            {
                74,    // i4X
                -238    // i4Y
            },
		// DF
		{
			0,	// i4X
			0	// i4Y
    		}
    	},
	// AWB gain of AWB light source
	{
		// Strobe
            {
                816,    // i4R
                512,    // i4G
                564    // i4B
            },
            // Horizon 
            {
                512,    // i4R
                546,    // i4G
                1468    // i4B
            },
            // A 
            {
                556,    // i4R
                512,    // i4G
                1194    // i4B
            },
            // TL84 
            {
                647,    // i4R
                512,    // i4G
                951    // i4B
            },
            // CWF 
            {
                794,    // i4R
                512,    // i4G
                1046    // i4B
            },
            // DNP 
            {
                730,    // i4R
                512,    // i4G
                720    // i4B
            },
            // D65 
            {
                816,    // i4R
                512,    // i4G
                564    // i4B
            },
            // DF 
            {
                512,    // i4R
                512,    // i4G
                512    // i4B
		}
	},
    	// Rotation matrix parameter
    	{
            16,    // i4RotationAngle
            246,    // i4Cos
            71    // i4Sin
        },
        // Daylight locus parameter
        {
            -234,    // i4SlopeNumerator
            128    // i4SlopeDenominator
        },
        // AWB light area
        {
            // Strobe:FIXME
            {
            0,    // i4RightBound
            0,    // i4LeftBound
            0,    // i4UpperBound
            0    // i4LowerBound
            },
            // Tungsten
            {
            -274,    // i4RightBound
            -924,    // i4LeftBound
            -186,    // i4UpperBound
            -286    // i4LowerBound
            },
            // Warm fluorescent
            {
            -274,    // i4RightBound
            -924,    // i4LeftBound
            -286,    // i4UpperBound
            -406    // i4LowerBound
            },
            // Fluorescent
            {
            -116,    // i4RightBound
            -274,    // i4LeftBound
            -172,    // i4UpperBound
            -322    // i4LowerBound
            },
            // CWF
            {
            -116,    // i4RightBound
            -274,    // i4LeftBound
            -322,    // i4UpperBound
            -431    // i4LowerBound
            },
            // Daylight
            {
            99,    // i4RightBound
            -116,    // i4LeftBound
            -158,    // i4UpperBound
            -318    // i4LowerBound
            },
            // Shade
            {
            459,    // i4RightBound
            99,    // i4LeftBound
            -158,    // i4UpperBound
            -318    // i4LowerBound
            },
            // Daylight Fluorescent
            {
            0,    // i4RightBound
            0,    // i4LeftBound
            0,    // i4UpperBound
            0    // i4LowerBound
            }
        },
        // PWB light area
        {
            // Reference area
            {
            459,    // i4RightBound
            -924,    // i4LeftBound
            -133,    // i4UpperBound
            -431    // i4LowerBound
            },
            // Daylight
            {
            124,    // i4RightBound
            -116,    // i4LeftBound
            -158,    // i4UpperBound
            -318    // i4LowerBound
            },
            // Cloudy daylight
            {
            224,    // i4RightBound
            49,    // i4LeftBound
            -158,    // i4UpperBound
            -318    // i4LowerBound
            },
            // Shade
            {
            324,    // i4RightBound
            49,    // i4LeftBound
            -158,    // i4UpperBound
            -318    // i4LowerBound
            },
            // Twilight
            {
            -116,    // i4RightBound
            -276,    // i4LeftBound
            -158,    // i4UpperBound
            -318    // i4LowerBound
            },
            // Fluorescent
            {
            124,    // i4RightBound
            -324,    // i4LeftBound
            -188,    // i4UpperBound
            -431    // i4LowerBound
            },
            // Warm fluorescent
            {
            -266,    // i4RightBound
            -466,    // i4LeftBound
            -188,    // i4UpperBound
            -431    // i4LowerBound
            },
            // Incandescent
            {
            -266,    // i4RightBound
            -466,    // i4LeftBound
            -158,    // i4UpperBound
            -318    // i4LowerBound
            },
            // Gray World
            {
            5000,    // i4RightBound
            -5000,    // i4LeftBound
            5000,    // i4UpperBound
            -5000    // i4LowerBound
            }
        },
        // PWB default gain	
        {
            // Daylight
            {
            766,    // i4R
            512,    // i4G
            634    // i4B
            },
            // Cloudy daylight
            {
            866,    // i4R
            512,    // i4G
            508    // i4B
            },
            // Shade
            {
            907,    // i4R
            512,    // i4G
            467    // i4B
            },
            // Twilight
            {
            636,    // i4R
            512,    // i4G
            886    // i4B
            },
            // Fluorescent
            {
            784,    // i4R
            512,    // i4G
            806    // i4B
            },
            // Warm fluorescent
            {
            613,    // i4R
            512,    // i4G
            1259    // i4B
            },
            // Incandescent
            {
            544,    // i4R
            512,    // i4G
            1178    // i4B
            },
            // Gray World
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            }
        },
        // AWB preference color	
        {
            // Tungsten
            {
            0,    // i4SliderValue
            7123    // i4OffsetThr
            },
            // Warm fluorescent	
            {
            50,    // i4SliderValue
            4944    // i4OffsetThr
            },
            // Shade
            {
            50,    // i4SliderValue
            355    // i4OffsetThr
            },
            // Daylight WB gain
            {
            718,    // i4R
            512,    // i4G
            713    // i4B
            },
            // Preference gain: strobe
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: tungsten
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: warm fluorescent
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: fluorescent
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: CWF
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: daylight
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: shade
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: daylight fluorescent
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            }
        },
        {// CCT estimation
            {// CCT
                2300,    // i4CCT[0]
                2850,    // i4CCT[1]
                4100,    // i4CCT[2]
                5100,    // i4CCT[3]
                6500    // i4CCT[4]
            },
            {// Rotated X coordinate
                -543,    // i4RotatedXCoordinate[0]
                -440,    // i4RotatedXCoordinate[1]
                -298,    // i4RotatedXCoordinate[2]
                -140,    // i4RotatedXCoordinate[3]
                0    // i4RotatedXCoordinate[4]
    		}
    	}
    },
	{0}
};
 
#include INCLUDE_FILENAME_ISP_LSC_PARAM
//};  //  namespace


typedef NSFeature::RAWSensorInfo<SENSOR_ID> SensorInfoSingleton_T;


namespace NSFeature {
template <>
UINT32
SensorInfoSingleton_T::
impGetDefaultData(CAMERA_DATA_TYPE_ENUM const CameraDataType, VOID*const pDataBuf, UINT32 const size) const
{
    UINT32 dataSize[CAMERA_DATA_TYPE_NUM] = {sizeof(NVRAM_CAMERA_ISP_PARAM_STRUCT),
                                             sizeof(NVRAM_CAMERA_3A_STRUCT),
                                             sizeof(NVRAM_CAMERA_SHADING_STRUCT),
                                             sizeof(NVRAM_LENS_PARA_STRUCT),
                                             sizeof(AE_PLINETABLE_T)};

    if (CameraDataType > CAMERA_DATA_AE_PLINETABLE || NULL == pDataBuf || (size < dataSize[CameraDataType]))
    {
        return 1;
    }

    switch(CameraDataType)
    {
        case CAMERA_NVRAM_DATA_ISP:
            memcpy(pDataBuf,&CAMERA_ISP_DEFAULT_VALUE,sizeof(NVRAM_CAMERA_ISP_PARAM_STRUCT));
            break;
        case CAMERA_NVRAM_DATA_3A:
            memcpy(pDataBuf,&CAMERA_3A_NVRAM_DEFAULT_VALUE,sizeof(NVRAM_CAMERA_3A_STRUCT));
            break;
        case CAMERA_NVRAM_DATA_SHADING:
            memcpy(pDataBuf,&CAMERA_SHADING_DEFAULT_VALUE,sizeof(NVRAM_CAMERA_SHADING_STRUCT));
            break;
        case CAMERA_DATA_AE_PLINETABLE:
            memcpy(pDataBuf,&g_PlineTableMapping,sizeof(AE_PLINETABLE_T));
            break;
        default:
            break;
    }
    return 0;
}};  //  NSFeature


