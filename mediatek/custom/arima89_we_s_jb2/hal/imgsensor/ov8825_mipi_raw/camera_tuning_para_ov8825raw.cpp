
#include <utils/Log.h>
#include <fcntl.h>
#include <math.h>

#include "camera_custom_nvram.h"
#include "camera_custom_sensor.h"
#include "image_sensor.h"
#include "kd_imgsensor_define.h"
#include "camera_AE_PLineTable_ov8825raw.h"
#include "camera_info_ov8825raw.h"
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
        74125, // i4R_AVG
        15573, // i4R_STD
        92925, // i4B_AVG
        25511, // i4B_STD
        { // i4P00[9]
			4767500,  -1717500,   -490000,   -675000,   3250000,    -15000,     42500,  -2067500,		4585000
        },
        { // i4P10[9]
			-182396,    203165,    -20768,     38604,     74648,   -113252,    -82655,    723950,		-641295
        },
        { // i4P01[9]
			-530158,    587665,    -57506,    -87925,      6140,     81785,   -152115,    154175,		  -2060
        },
        { // i4P20[9]
			    0,         0,         0,         0,         0,         0,         0,         0,           0
        },
        { // i4P11[9]
			    0,         0,         0,         0,         0,         0,         0,         0,			  0
        },
        { // i4P02[9]
			    0,         0,         0,         0,         0,         0,         0,         0,			  0
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
            1144,   // u4MinGain, 1024 base =  1x
            10240,  // u4MaxGain, 16x
            100,     // u4MiniISOGain, ISOxx
            128,    // u4GainStepUnit, 1x/8
            26355,     // u4PreExpUnit
            30,     // u4PreMaxFrameRate
            17763,     // u4VideoExpUnit
            30,     // u4VideoMaxFrameRate
            512,   // u4Video2PreRatio, 1024 base = 1x
            17763,     // u4CapExpUnit
            23,     // u4CapMaxFrameRate
            512,   // u4Cap2PreRatio, 1024 base = 1x
            24,      // u4LensFno, Fno = 2.8
            350     // u4FocusLength_100x
         },
         // rHistConfig
        {
            4, // 2,   // u4HistHighThres
            40,  // u4HistLowThres
            2,   // u4MostBrightRatio
            1,   // u4MostDarkRatio
            160, // u4CentralHighBound
            20,  // u4CentralLowBound
            {240, 230, 220, 210, 200}, // u4OverExpThres[AE_CCT_STRENGTH_NUM]
            {82, 108, 128, 148, 170},  // u4HistStretchThres[AE_CCT_STRENGTH_NUM]
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
            0, // 2,                 // u4HistStretchStrengthIndex
            2,                 // u4AntiOverExpStrengthIndex
            2,                 // u4TimeLPFStrengthIndex
            {1, 3, 5, 7, 8}, // u4LPFConvergeTable[AE_CCT_STRENGTH_NUM]
            90,                // u4InDoorEV = 9.0, 10 base
            -10,               // i4BVOffset delta BV = -2.3
            64,                 // u4PreviewFlareOffset
            64,                 // u4CaptureFlareOffset
            3,                 // u4CaptureFlareThres
            64,                 // u4VideoFlareOffset
            3,                 // u4VideoFlareThres
            64,                 // u4StrobeFlareOffset //12 bits
            3,                 // u4StrobeFlareThres // 0.5%
            160,                 // u4PrvMaxFlareThres //12 bit
            0,                 // u4PrvMinFlareThres
            160,                 // u4VideoMaxFlareThres // 12 bit
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
                905,    // i4R
	           512,	// i4G
                574    // i4B
    		}
    	},
    	// Original XY coordinate of AWB light source
    	{
		    // Strobe
		    {
                131,    // i4X
                -380    // i4Y
		    },
    		// Horizon
    		{
                -402,    // i4X
                -341    // i4Y
    		},
    		// A
    		{
                -278,    // i4X
                -333    // i4Y
    		},
    		// TL84
    		{
                -118,    // i4X
                -332    // i4Y
    		},
    		// CWF
    		{
                -82,    // i4X
                -434    // i4Y
    		},
    		// DNP
    		{
                35,    // i4X
                -319    // i4Y
    		},
    		// D65
    		{
                168,    // i4X
                -253    // i4Y
    		},
		// DF
		{
                131,    // i4X
                -380    // i4Y
    		}
    	},
    	// Rotated XY coordinate of AWB light source
    	{
		    // Strobe
		    {
                64,    // i4X
                -397    // i4Y
		    },
    		// Horizon
    		{
                -454,    // i4X
                -267    // i4Y
    		},
    		// A
    		{
                -331,    // i4X
                -280    // i4Y
    		},
    		// TL84
    		{
                -173,    // i4X
                -307    // i4Y
    		},
    		// CWF
    		{
                -155,    // i4X
                -413    // i4Y
    		},
    		// DNP
    		{
                -20,    // i4X
                -320    // i4Y
    		},
    		// D65
    		{
                122,    // i4X
                -278    // i4Y
    		},
		// DF
		{
                64,    // i4X
                -397    // i4Y
    		}
    	},
	// AWB gain of AWB light source
	{
		// Strobe
		{
                1024,    // i4R
			512,	// i4G				
                717    // i4B
		},
		// Horizon
		{
			512,	// i4R				
                556,    // i4G
                1522    // i4B
		},
		// A
		{
                552,    // i4R
                512,    // i4G
                1172    // i4B
		},
		// TL84
		{
                684,    // i4R
			512,	// i4G				
                942    // i4B
		},
		// CWF
		{
                824,    // i4R
			512,	// i4G				
                1029    // i4B
		},
		// DNP
		{
                826,    // i4R
			512,	// i4G				
                752    // i4B
		},
		// D65
		{
                905,    // i4R
			512,	// i4G				
                574    // i4B
		},
		// DF
		{
                1024,    // i4R
			512,	// i4G
                717    // i4B
		}
	},
    	// Rotation matrix parameter
    	{
            10,    // i4RotationAngle
            252,    // i4Cos
            44    // i4Sin
    	},
    	// Daylight locus parameter
    	{
            -184,    // i4SlopeNumerator
    		128	// i4SlopeDenominator
    	},
    	// AWB light area
    	{
		    // Strobe:FIXME
		    {
            147,    // i4RightBound
            -70,    // i4LeftBound
            -358,    // i4UpperBound
            -463    // i4LowerBound
		    },
    		// Tungsten
    		{
            -223,    // i4RightBound
            -873,    // i4LeftBound
            -223,    // i4UpperBound
            -323    // i4LowerBound
    		},
    		// Warm fluorescent
    		{
            -223,    // i4RightBound
            -873,    // i4LeftBound
            -323,    // i4UpperBound
            -443    // i4LowerBound
    		},
    		// Fluorescent
    		{
            -70,    // i4RightBound
            -223,    // i4LeftBound
            -210,    // i4UpperBound
            -360    // i4LowerBound
    		},
    		// CWF
    		{
            -70,    // i4RightBound
            -223,    // i4LeftBound
            -360,    // i4UpperBound
            -463    // i4LowerBound
    		},
    		// Daylight
    		{
            147,    // i4RightBound
            -70,    // i4LeftBound
            -198,    // i4UpperBound
            -358    // i4LowerBound
    		},
    		// Shade
    		{
            507,    // i4RightBound
            147,    // i4LeftBound
            -198,    // i4UpperBound
            -358    // i4LowerBound
		},
		// Daylight Fluorescent
		{
            147,    // i4RightBound
            -70,    // i4LeftBound
            -358,    // i4UpperBound
            -463    // i4LowerBound
    		}
    	},
    	// PWB light area
    	{
    		// Reference area
    		{
            507,    // i4RightBound
            -873,    // i4LeftBound
            -198,    // i4UpperBound
            -463    // i4LowerBound
    		},
    		// Daylight
    		{
            172,    // i4RightBound
            -70,    // i4LeftBound
            -198,    // i4UpperBound
            -358    // i4LowerBound
    		},
    		// Cloudy daylight
    		{
            272,    // i4RightBound
            97,    // i4LeftBound
            -198,    // i4UpperBound
            -358    // i4LowerBound
    		},
    		// Shade
    		{
            372,    // i4RightBound
            97,    // i4LeftBound
            -198,    // i4UpperBound
            -358    // i4LowerBound
    		},
    		// Twilight
    		{
            -70,    // i4RightBound
            -230,    // i4LeftBound
            -198,    // i4UpperBound
            -358    // i4LowerBound
    		},
    		// Fluorescent
    		{
            172,    // i4RightBound
            -273,    // i4LeftBound
            -228,    // i4UpperBound
            -463    // i4LowerBound
    		},
    		// Warm fluorescent
    		{
            -231,    // i4RightBound
            -431,    // i4LeftBound
            -228,    // i4UpperBound
            -463    // i4LowerBound
    		},
    		// Incandescent
    		{
            -231,    // i4RightBound
            -431,    // i4LeftBound
            -198,    // i4UpperBound
            -358    // i4LowerBound
    		},
            {// Gray World
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
            837,    // i4R
			512,	// i4G				
            642    // i4B
    		},
    		// Cloudy daylight
    		{
            970,    // i4R
			512,	// i4G				
            521    // i4B
    		},
    		// Shade
    		{
            1025,    // i4R
			512,	// i4G				
            482    // i4B
    		},
    		// Twilight
    		{
            671,    // i4R
			512,	// i4G				
            880    // i4B
    		},
    		// Fluorescent
    		{
            832,    // i4R
			512,	// i4G				
            811    // i4B
    		},
    		// Warm fluorescent
    		{
            611,    // i4R
			512,	// i4G				
            1259    // i4B
    		},
    		// Incandescent
    		{
            550,    // i4R
			512,	// i4G				
            1168    // i4B
		},
		// Gray World
		{
			512,	// i4R
			512,	// i4G
			512	// i4B
    		}
    	},
    	// AWB preference color
    	{
    		// Tungsten
    		{
            50,    // i4SliderValue
            8000//6800    // i4OffsetThr
    		},
    		// Warm fluorescent
    		{
            50,    // i4SliderValue
            8000//6200    // i4OffsetThr
    		},
    		// Shade
    		{
    			50,	// i4SliderValue
            346    // i4OffsetThr
    		},
    		// Daylight WB gain
    		{
            774,    // i4R
			512,	// i4G				
            718    // i4B
		},
		// Preference gain: strobe
		{
			512,	// i4R
			512,	// i4G
			512	// i4B
		},
		// Preference gain: tungsten
		{
			512,	// i4R
			512,	// i4G
			512	    // i4B
		},
		// Preference gain: warm fluorescent
		{
			512,	// i4R
			512,	// i4G
			512	    // i4B
		},
		// Preference gain: fluorescent
		{
			512,	// i4R
			512,	// i4G
			512	    // i4B
		},
		// Preference gain: CWF
		{
			512,	// i4R
			512,	// i4G
			512	    // i4B
		},
		// Preference gain: daylight
		{
			512,	// i4R
			512,	// i4G
			512	    // i4B
		},
		// Preference gain: shade
		{
			512,	// i4R
			512,	// i4G
			512	    // i4B
		},
		// Preference gain: daylight fluorescent
		{
			512,	// i4R
			512,	// i4G
			512	    // i4B
    		}
    	},
    	// CCT estimation
    	{
    		// CCT
    		{
			    2300,	// i4CCT[0]
    			2850,	// i4CCT[1]
    			4100,	// i4CCT[2]
    			5100,	// i4CCT[3]
    			6500	// i4CCT[4]
    		},
            {// Rotated X coordinate
                -576,    // i4RotatedXCoordinate[0]
                -453,    // i4RotatedXCoordinate[1]
                -295,    // i4RotatedXCoordinate[2]
			-142,	// i4RotatedXCoordinate[3]				
    			0	// i4RotatedXCoordinate[4]
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


