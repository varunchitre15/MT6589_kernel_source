
#include <utils/Log.h>
#include <fcntl.h>
#include <math.h>

#include "camera_custom_nvram.h"
#include "camera_custom_sensor.h"
#include "image_sensor.h"
#include "kd_imgsensor_define.h"
#include "camera_AE_PLineTable_ov12830raw.h"
#include "camera_info_ov12830raw.h"
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
            100,     // u4MiniISOGain, ISOxx
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
            64,                 // u4PreviewFlareOffset
            64,                 // u4CaptureFlareOffset
            3,                 // u4CaptureFlareThres
            64,                 // u4VideoFlareOffset
            3,                 // u4VideoFlareThres
            32,                 // u4StrobeFlareOffset
            2,                 // u4StrobeFlareThres
            160,                 // u4PrvMaxFlareThres
            0,                 // u4PrvMinFlareThres
            160,                 // u4VideoMaxFlareThres
            0,                 // u4VideoMinFlareThres            
            18,                // u4FlatnessThres              // 10 base for flatness condition.
            75                 // u4FlatnessStrength
         }
    },

    // AWB NVRAM
{								
	// AWB calibration data							
	{							
		// rCalGain (calibration gain: 1.0 = 512)						
		{						
			0,	// u4R				
			0,	// u4G				
			0	// u4B				
		},						
		// rDefGain (Default calibration gain: 1.0 = 512)						
		{						
			0,	// u4R				
			0,	// u4G				
			0	// u4B				
		},						
		// rDefGain (Default calibration gain: 1.0 = 512)						
		{						
			0,	// u4R				
			0,	// u4G				
			0	// u4B				
		},						
		// rD65Gain (D65 WB gain: 1.0 = 512)						
		{						
			857,	// u4R				
			512,	// u4G				
			614	// u4B				
		}						
	},							
	// Original XY coordinate of AWB light source							
	{							
		// Strobe						
		{						
			-5,	// i4X				
			-364	// i4Y				
		},						
		// Horizon						
		{						
			-413,	// i4X				
			-271	// i4Y				
		},						
		// A						
		{						
			-291,	// i4X				
			-282	// i4Y				
		},						
		// TL84						
		{						
			-161,	// i4X				
			-250	// i4Y				
		},						
		// CWF						
		{						
			-133,	// i4X				
			-331	// i4Y				
		},						
		// DNP						
		{						
			-14,	// i4X				
			-255	// i4Y				
		},						
		// D65						
		{						
			123,	// i4X				
			-257	// i4Y				
		},						
		// DF						
		{						
			0, 	// i4X				
			0	// i4Y				
		}						
	},							
	// Rotated XY coordinate of AWB light source							
	{							
		// Strobe						
		{						
			-18,	// i4X				
			-364	// i4Y				
		},						
		// Horizon						
		{						
			-423,	// i4X				
			-256	// i4Y				
		},						
		// A						
		{						
			-301,	// i4X				
			-272	// i4Y				
		},						
		// TL84						
		{						
			-170,	// i4X				
			-244	// i4Y				
		},						
		// CWF						
		{						
			-145,	// i4X				
			-326	// i4Y				
		},						
		// DNP						
		{						
			-23,	// i4X				
			-255	// i4Y				
		},						
		// D65						
		{						
			114,	// i4X				
			-261	// i4Y				
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
			833,	// u4R				
			512,	// u4G				
			844	// u4B				
		},						
		// Horizon						
		{						
			512,	// u4R				
			621,	// u4G				
			1565	// u4B				
		},						
		// A						
		{						
			512,	// u4R				
			519,	// u4G				
			1127	// u4B				
		},						
		// TL84						
		{						
			578,	// u4R				
			512,	// u4G				
			894	// u4B				
		},						
		// CWF						
		{						
			670,	// u4R				
			512,	// u4G				
			960	// u4B				
		},						
		// DNP						
		{						
			709,	// u4R				
			512,	// u4G				
			737	// u4B				
		},						
		// D65						
		{						
			857,	// u4R				
			512,	// u4G				
			614	// u4B				
		},						
		// DF						
		{						
			512,	// u4R				
			512,	// u4G				
			512	// u4B				
		}						
	},							
	// Rotation matrix parameter							
	{							
		2,	// i4RotationAngle					
		256,	// i4Cos					
		9,	// i4Sin					
	},							
	// Daylight locus parameter							
	{							
		-135,	// i4SlopeNumerator					
		128	// i4SlopeDenominator					
	},							
	// AWB light area							
	{							
		// Strobe						
		{						
			152,	// i4RightBound				
			-68,	// i4LeftBound				
			-341,	// i4UpperBound				
			-424	// i4LowerBound				
		},						
		// Tungsten						
		{						
			-220,	// i4RightBound				
			-870,	// i4LeftBound				
			-214,	// i4UpperBound				
			-314	// i4LowerBound				
		},						
		// Warm fluorescent						
		{						
			-220,	// i4RightBound				
			-870,	// i4LeftBound				
			-314,	// i4UpperBound				
			-434	// i4LowerBound				
		},						
		// Fluorescent						
		{						
			-73,	// i4RightBound				
			-220,	// i4LeftBound				
			-179,	// i4UpperBound				
			-285	// i4LowerBound				
		},						
		// CWF						
		{						
			-73,	// i4RightBound				
			-220,	// i4LeftBound				
			-285,	// i4UpperBound				
			-426	// i4LowerBound				
		},						
		// Daylight						
		{						
			139,	// i4RightBound				
			-73,	// i4LeftBound				
			-181,	// i4UpperBound				
			-341	// i4LowerBound				
		},						
		// Shade						
		{						
			499,	// i4RightBound				
			139,	// i4LeftBound				
			-181,	// i4UpperBound				
			-341	// i4LowerBound				
		},						
		// Daylight Fluorescent						
		{						
			0,	// i4RightBound				
			0,	// i4LeftBound				
			0,	// i4UpperBound				
			0	// i4LowerBound				
		}						
	},							
	// PWB light area							
	{							
		// Reference area						
		{						
			499,	// i4RightBound				
			-870,	// i4LeftBound				
			-154,	// i4UpperBound				
			-434	// i4LowerBound				
		},						
		// Daylight						
		{						
			164,	// i4RightBound				
			-73,	// i4LeftBound				
			-181,	// i4UpperBound				
			-341	// i4LowerBound				
		},						
		// Cloudy daylight						
		{						
			264,	// i4RightBound				
			89,	// i4LeftBound				
			-181,	// i4UpperBound				
			-341	// i4LowerBound				
		},						
		// Shade						
		{						
			364,	// i4RightBound				
			89,	// i4LeftBound				
			-181,	// i4UpperBound				
			-341	// i4LowerBound				
		},						
		// Twilight						
		{						
			-73,	// i4RightBound				
			-233,	// i4LeftBound				
			-181,	// i4UpperBound				
			-341	// i4LowerBound				
		},						
		// Fluorescent						
		{						
			164,	// i4RightBound				
			-270,	// i4LeftBound				
			-194,	// i4UpperBound				
			-376	// i4LowerBound				
		},						
		// Warm fluorescent						
		{						
			-201,	// i4RightBound				
			-401,	// i4LeftBound				
			-194,	// i4UpperBound				
			-376	// i4LowerBound				
		},						
		// Incandescent						
		{						
			-201,	// i4RightBound				
			-401,	// i4LeftBound				
			-181,	// i4UpperBound				
			-341	// i4LowerBound				
		},						
		// Gray World						
		{						
			5000,	// i4RightBound				
			-5000,	// i4LeftBound				
			5000,	// i4UpperBound				
			-5000	// i4LowerBound				
		}						
	},							
	// PWB default gain							
	{							
		// Daylight						
		{						
			783,	// u4R				
			512,	// u4G				
			675	// u4B				
		},						
		// Cloudy daylight						
		{						
			929,	// u4R				
			512,	// u4G				
			562	// u4B				
		},						
		// Shade						
		{						
			991,	// u4R				
			512,	// u4G				
			524	// u4B				
		},						
		// Twilight						
		{						
			604,	// u4R				
			512,	// u4G				
			892	// u4B				
		},						
		// Fluorescent						
		{						
			712,	// u4R				
			512,	// u4G				
			800	// u4B				
		},						
		// Warm fluorescent						
		{						
			515,	// u4R				
			512,	// u4G				
			1132	// u4B				
		},						
		// Incandescent						
		{						
			498,	// u4R				
			512,	// u4G				
			1097	// u4B				
		},						
		// Gray World						
		{						
			512,	// u4R				
			512,	// u4G				
			512	// u4B				
		}						
	},							
	// AWB preference color							
	{							
		// Tungsten						
		{						
			50,	// i4SliderValue				
			4552	// i4OffsetThr				
		},						
		// Warm fluorescent						
		{						
			50,	// i4SliderValue				
			4552	// i4OffsetThr				
		},						
		// Shade						
		{						
			50,	// i4SliderValue				
			841	// i4OffsetThr				
		},						
		// Daylight WB gain						
		{						
			716,	// u4R				
			512,	// u4G				
			743	// u4B				
		},						
		// Preference gain: strobe						
		{						
			512,	// u4R				
			512,	// u4G				
			512	// u4B				
		},						
		// Preference gain: tungsten						
		{						
			512,	// u4R				
			512,	// u4G				
			512	// u4B				
		},						
		// Preference gain: warm fluorescent						
		{						
			512,	// u4R				
			512,	// u4G				
			512	// u4B				
		},						
		// Preference gain: fluorescent						
		{						
			512,	// u4R				
			512,	// u4G				
			512	// u4B				
		},						
		// Preference gain: CWF						
		{						
			512,	// u4R				
			512,	// u4G				
			512	// u4B				
		},						
		// Preference gain: daylight						
		{						
			512,	// u4R				
			512,	// u4G				
			512	// u4B				
		},						
		// Preference gain: shade						
		{						
			512,	// u4R				
			512,	// u4G				
			512	// u4B				
		},						
		// Preference gain: daylight fluorescent						
		{						
			512,	// u4R				
			512,	// u4G				
			512	// u4B				
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
			6500 	// i4CCT[4]				
		},						
		// Rotated X coordinate						
		{						
			-537,	// i4RotatedXCoordinate[0]				
			-415,	// i4RotatedXCoordinate[1]				
			-284,	// i4RotatedXCoordinate[2]				
			-137,	// i4RotatedXCoordinate[3]				
			0 	// i4RotatedXCoordinate[4]				
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


