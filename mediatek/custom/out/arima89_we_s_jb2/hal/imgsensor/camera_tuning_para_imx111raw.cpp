
#include <utils/Log.h>
#include <fcntl.h>
#include <math.h>

#include "camera_custom_nvram.h"
#include "camera_custom_sensor.h"
#include "image_sensor.h"
#include "kd_imgsensor_define.h"
#include "camera_AE_PLineTable_imx111raw.h"
#include "camera_info_imx111raw.h"
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
//<2013/06/18-26042-alberthsiao, Update camera parameters for imx111 and bf3905
//<2013/05/02-24529-alberthsiao, Update camera parameters for imx111
    ISPCcmPoly22:{
        72625,    // i4R_AVG
        16700,    // i4R_STD
        101750,    // i4B_AVG
        25675,    // i4B_STD
        {  // i4P00[9]
            5055000, -2445000, -50000, -990000, 4070000, -520000, -110000, -2500000, 5172500
        },
        {  // i4P10[9]
            -180821, -367563, 548384, 588382, -1216443, 630610, 169243, -1332689, 1170651
        },
        {  // i4P01[9]
            -523219, -34581, 557800, 515412, -1486923, 965894, 117296, -2438192, 2332327
        },
        {  // i4P20[9]
            0, 0, 0, 0, 0, 0, 0, 0, 0
        },
        {  // i4P11[9]
            0, 0, 0, 0, 0, 0, 0, 0, 0
        },
        {  // i4P02[9]
            0, 0, 0, 0, 0, 0, 0, 0, 0
        }
    }
//>2013/05/02-24529-alberthsiao
}};
//<2013/06/21-26216-alberthsiao,update camera parameters for imx111
const NVRAM_CAMERA_3A_STRUCT CAMERA_3A_NVRAM_DEFAULT_VALUE =
{
    NVRAM_CAMERA_3A_FILE_VERSION, // u4Version
    SENSOR_ID, // SensorId

    // AE NVRAM
    {
        // rDevicesInfo
        {
            1260,   // u4MinGain, 1024 base =  1x
            8192,  // u4MaxGain, 16x
            120,    // u4MiniISOGain, ISOxx  
            128,    // u4GainStepUnit, 1x/8
            27,    // u4PreExpUnit 
            30,    // u4PreMaxFrameRate
            18,    // u4VideoExpUnit  
            30,    // u4VideoMaxFrameRate 
            1024,    // u4Video2PreRatio, 1024 base = 1x 
            27,    // u4CapExpUnit 
            15,    // u4CapMaxFrameRate
            1024,    // u4Cap2PreRatio, 1024 base = 1x
            26,    // u4LensFno, Fno = 2.8
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
            FALSE,            // bEnableCaptureThres
            FALSE,            // bEnableVideoThres
            FALSE,            // bEnableStrobeThres
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
            -13,               // i4BVOffset delta BV = -2.3
            128,//64,                 // u4PreviewFlareOffset
            128,//64,                 // u4CaptureFlareOffset
            5,                 // u4CaptureFlareThres
            64,                 // u4VideoFlareOffset
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
//>2013/06/21-26216-alberthsiao
    // AWB NVRAM
    {
        // AWB calibration data
        {
            // rUnitGain (unit gain: 1.0 = 512)
            {
                976,    // i4R
                512,    // i4G
                846    // i4B
            },
            // rGoldenGain (golden sample gain: 1.0 = 512)
            {
                976,    // i4R
                512,    // i4G
                857    // i4B
            },
            // rTuningUnitGain (Tuning sample unit gain: 1.0 = 512)
            {
                976,    // i4R
                512,    // i4G
                846    // i4B
            },
            // rD65Gain (D65 WB gain: 1.0 = 512)
            {
                964,    // i4R
                512,    // i4G
                701    // i4B
            }
        },
        // Original XY coordinate of AWB light source
        {
           // Strobe
            {
                89,    // i4X
                -369    // i4Y
            },
            // Horizon
            {
                -425,    // i4X
                -372    // i4Y
            },
            // A
            {
                -307,    // i4X
                -375    // i4Y
            },
            // TL84
            {
                -166,    // i4X
                -363    // i4Y
            },
            // CWF
            {
                -121,    // i4X
                -437    // i4Y
            },
            // DNP
            {
                -3,    // i4X
                -405    // i4Y
            },
            // D65
            {
                117,    // i4X
                -350    // i4Y
            },
            // DF
            {
                75,    // i4X
                -412    // i4Y
            }
        },
        // Rotated XY coordinate of AWB light source
        {
            // Strobe
            {
                70,    // i4X
                -374    // i4Y
            },
            // Horizon
            {
                -444,    // i4X
                -350    // i4Y
            },
            // A
            {
                -326,    // i4X
                -359    // i4Y
            },
            // TL84
            {
                -184,    // i4X
                -355    // i4Y
            },
            // CWF
            {
                -143,    // i4X
                -431    // i4Y
            },
            // DNP
            {
                -24,    // i4X
                -405    // i4Y
            },
            // D65
            {
                99,    // i4X
                -276,//-356    // i4Y
            },
            // DF
            {
                54,    // i4X
                -524,//-416    // i4Y
            }
        },
        // AWB gain of AWB light source
        {
            // Strobe 
            {
                952,    // i4R
                512,    // i4G
                748    // i4B
            },
            // Horizon 
            {
                512,    // i4R
                550,    // i4G
                1620    // i4B
            },
            // A 
            {
                561,    // i4R
                512,    // i4G
                1289    // i4B
            },
            // TL84 
            {
                669,    // i4R
                512,    // i4G
                1048    // i4B
            },
            // CWF 
            {
                785,    // i4R
                512,    // i4G
                1090    // i4B
            },
            // DNP 
            {
                882,    // i4R
                512,    // i4G
                890    // i4B
            },
            // D65 
            {
                964,    // i4R
                512,    // i4G
                701    // i4B
            },
            // DF 
            {
                989,    // i4R
                512,    // i4G
                807    // i4B
            }
        },
        // Rotation matrix parameter
        {
            3,    // i4RotationAngle
            256,    // i4Cos
            13    // i4Sin
        },
        // Daylight locus parameter
        {
            -141,    // i4SlopeNumerator
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
            -234,    // i4RightBound
            -884,    // i4LeftBound
            -304,    // i4UpperBound
            -404    // i4LowerBound
            },
            // Warm fluorescent
            {
            -234,    // i4RightBound
            -884,    // i4LeftBound
            -404,    // i4UpperBound
            -524    // i4LowerBound
            },
            // Fluorescent
            {
            -74,    // i4RightBound
            -234,    // i4LeftBound
            -290,    // i4UpperBound
            -393    // i4LowerBound
            },
            // CWF
            {
            -74,    // i4RightBound
            -234,    // i4LeftBound
            -393,    // i4UpperBound
            -481    // i4LowerBound
            },
            // Daylight
            {
            26,    // i4RightBound
            -74,    // i4LeftBound
            -276,    // i4UpperBound
            -436    // i4LowerBound
            },
            // Shade
            {
            484,    // i4RightBound
            26,    // i4LeftBound
            -276,    // i4UpperBound
            -436    // i4LowerBound
            },
            // Daylight Fluorescent
            {
            124,    // i4RightBound
            -74,    // i4LeftBound
            -436,    // i4UpperBound
            -524    // i4LowerBound
            }
        },
        // PWB light area
        {
            // Reference area
            {
            484,    // i4RightBound
            -884,    // i4LeftBound
            0,    // i4UpperBound
            -524    // i4LowerBound
            },
            // Daylight
            {
            149,    // i4RightBound
            -74,    // i4LeftBound
            -276,    // i4UpperBound
            -436    // i4LowerBound
            },
            // Cloudy daylight
            {
            249,    // i4RightBound
            74,    // i4LeftBound
            -276,    // i4UpperBound
            -436    // i4LowerBound
            },
            // Shade
            {
            349,    // i4RightBound
            74,    // i4LeftBound
            -276,    // i4UpperBound
            -436    // i4LowerBound
            },
            // Twilight
            {
            -74,    // i4RightBound
            -234,    // i4LeftBound
            -276,    // i4UpperBound
            -436    // i4LowerBound
            },
            // Fluorescent
            {
//<2013/05/9-159510-albertwu,modify wb Fluorescent parmeter             
            //149,    // i4RightBound
	    	-73,    // i4RightBound	
//>2013/05/9-159510-albertwu	
            -284,    // i4LeftBound
            -305,    // i4UpperBound
            -481    // i4LowerBound
            },
            // Warm fluorescent
            {
            -226,    // i4RightBound
            -426,    // i4LeftBound
            -305,    // i4UpperBound
            -481    // i4LowerBound
            },
            // Incandescent
            {
            -226,    // i4RightBound
            -426,    // i4LeftBound
            -276,    // i4UpperBound
            -436    // i4LowerBound
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
            890,    // i4R
            512,    // i4G
            766    // i4B
            },
            // Cloudy daylight
            {
            1044,    // i4R
            512,    // i4G
            643    // i4B
            },
            // Shade
            {
            1113,    // i4R
            512,    // i4G
            599    // i4B
            },
            // Twilight
            {
            696,    // i4R
            512,    // i4G
            1005    // i4B
            },
            // Fluorescent
            {
            820,    // i4R
            512,    // i4G
            932    // i4B
            },
            // Warm fluorescent
            {
            589,    // i4R
            512,    // i4G
            1346    // i4B
            },
            // Incandescent
            {
            559,    // i4R
            512,    // i4G
            1283    // i4B
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
//<2013/06/25-26300-alberthsiao, update camera AWB parameters for imx111
            // Tungsten
            {
            50,    // i4SliderValue
            7200 //5800 //4542    // i4OffsetThr
            },
            // Warm fluorescent	
            {
            50,    // i4SliderValue
            7200 //5800 //4542    // i4OffsetThr
            },
//>2013/06/25-26300-alberthsiao
            // Shade
            {
            50,    // i4SliderValue
            341    // i4OffsetThr
            },
            // Daylight WB gain
            {
            823,    // i4R
            512,    // i4G
            836    // i4B
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
                -425,    // i4RotatedXCoordinate[1]
                -283,    // i4RotatedXCoordinate[2]
                -123,    // i4RotatedXCoordinate[3]
                0    // i4RotatedXCoordinate[4]
            }
        }
    },
    {0}
};
//>2013/06/18-26042-alberthsiao

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


