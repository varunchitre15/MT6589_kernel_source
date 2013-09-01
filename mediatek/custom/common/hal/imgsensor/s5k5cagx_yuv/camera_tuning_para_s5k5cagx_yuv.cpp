#include <utils/Log.h>
#include <fcntl.h>
#include <math.h>

//#include "msdk_nvram_camera_exp.h"
#include "camera_custom_nvram.h"
//#include "msdk_sensor_exp.h"
#include "camera_custom_sensor.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
//#include "image_sensor.h"
//TODO:remove once build system ready
//#include "camera_custom_cfg.h"

/*
#if (defined(DRV_ISP_6516_SERIES))

#if !defined(ISP_SUPPORT)
	// DO NOT delete this section!!!
	// When ISP_SUPPORT is not defined, NVRAM still need the dummy structure
	// and default value to initialize NVRAM_EF_CAMERA_PARA_LID.
	//#include "camera_para.h"
	const nvram_camera_para_struct S5K5CAGX_YUV_CAMERA_PARA_DEFAULT_VALUE={0};
	const nvram_camera_3a_struct S5K5CAGX_YUV_CAMERA_3A_NVRAM_DEFAULT_VALUE={0};
#else
//#include "camera_para.h"
//#include "camera_sensor_para_S5K5CAGX_YUV.h"
//#include "camera_af_para.h"
*/
#define SENSOR_ID   S5K5CAGX_SENSOR_ID
#if defined(MT6516)

const NVRAM_CAMERA_DEFECT_STRUCT S5K5CAGX_YUV_CAMERA_DEFECT_DEFAULT_VALUE =
      {{ NVRAM_CAMERA_DEFECT_FILE_VERSION,S5K5CAGX_SENSOR_ID,0,0,{0},{0},{0} }};

const NVRAM_CAMERA_SHADING_STRUCT S5K5CAGX_YUV_CAMERA_SHADING_DEFAULT_VALUE =
      {{ NVRAM_CAMERA_SHADING_FILE_VERSION,S5K5CAGX_SENSOR_ID,0,0,0,0,{{0},{0},{0}},{{0},{0},{0}},{{0},{0},{0}},{{0},{0},{0}},{0},{0} }};


const NVRAM_CAMERA_PARA_STRUCT S5K5CAGX_YUV_CAMERA_PARA_DEFAULT_VALUE=
{
    //Version
    NVRAM_CAMERA_PARA_FILE_VERSION,
    //SensorId
    S5K5CAGX_SENSOR_ID,

    {//ISP_COMMON_PARA_STRUCT  ISPComm;
        {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    	}
    },
    {//ISP_TUNING_PARA_STRUCT  ISPTuning;
        {//ISP_TUNING_INDEX_STRUCT Idx
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        },
        {//ShadingReg
            {//00
        	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
            },
            {//01
        	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
            },
            {//02
        	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
            }
        },
        {//NR1
            {//00 Capture ISO100/ISO200
                0x000000F6, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x000000A0, 0x00069050, 0x0000069E,
                0x01010202, 0x02030303, 0x01020202, 0x03030304, 0x01020202, 0x03030304, 0x01020202, 0x03030304
            },
            {//01 Capture ISO400
                0x000000F6, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  0x000000A0, 0x00079050, 0x000007BF,
                0x03030506, 0x070B0B0B, 0x01020202, 0x03030304, 0x01020202, 0x03030304, 0x01020202, 0x03030304
            },
            {//02 Capture ISO800
                0x000000F6, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x000000A0, 0x00079050, 0x000007BF,
                0x0505080A, 0x0B0D0F10, 0x01020202, 0x03030304, 0x01020202, 0x03030304, 0x01020202, 0x03030304
            },
            {//03 Preview ISO200 ISO400
                0x000000F6, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x000000C8, 0x00089078, 0x000009BD,
                0x0B090908, 0x090B0C0E, 0x01020202, 0x03030304, 0x01020202, 0x03030304, 0x01020202, 0x03030304
            },
            {//04 Preview ISO800
                0x000000F6, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x000000C8, 0x00089078, 0x000008BD,
                0x0C0B0A0A, 0x0B0D0E10, 0x01020202, 0x03030304, 0x01020202, 0x03030304, 0x01020202, 0x03030304
            },
            {//05 Capture ISO1600
                0x000000F6, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x000000A0, 0x00089050, 0x00000CDF,
                0x090A0B0D, 0x0F111315, 0x01020202, 0x03030304, 0x01020202, 0x03030304, 0x01020202, 0x03030304
            },
            {//06 Preview ISO1600
                0x000000F6, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x000000C8, 0x00089078, 0x00000BDF,
                0x0C0E0E0E, 0x0E121218, 0x01020202, 0x03030304, 0x01020202, 0x03030304, 0x01020202, 0x03030304
            }
        },
        {//NR2
            {//00
                0x00000003, 0x00914488, 0x02040506, 0x02040506
            },
            {//01
                0x00000003, 0x00914488, 0x02040506, 0x02040506
            },
            {//02
                0x00000003, 0x00914488, 0x02040506, 0x02040506
            },
            {//03
                0x00000003, 0x00914488, 0x02040506, 0x02040506
            },
            {//04
                0x00000003, 0x00914488, 0x02040506, 0x02040506
            },
            {//05
                0x00000003, 0x00914488, 0x02040506, 0x02040506
            },
            {//06
                0x00000003, 0x00914488, 0x02040506, 0x02040506
            }
        },
        {//EE
            {//00 no one use this parameter, this is Min EE
                0x081f0814, 0x01030332, 0x010f000f, 0x32800707, 0x181f0232, 0x00000008, 0x70080115, 0x00302010, 0x0a080402
            },
            {//01 all preview EE parameter
                0x081f0814, 0x01030332, 0x010f000f, 0x32800707, 0x181f0232, 0x00000008, 0x70080115, 0x00302010, 0x0e0a0804
            },
            {//02 Capture ISO1600
                0x081f0814, 0x01030332, 0x010f000f, 0x32800707, 0x181f0232, 0x00000008, 0x70080115, 0x00302010, 0x18141008
            },
            {//03 Capture ISO800 same as ISO400
                0x081f0814, 0x01030332, 0x010f000f, 0x32800707, 0x181f0232, 0x00000008, 0x805f1f1d, 0x00302010, 0x12151006
            },
            {//04 Capture ISO200
                0x081f0814, 0x01030332, 0x010f000f, 0x32800707, 0x181f0232, 0x00000008, 0x8045351d, 0x00302010, 0x161f1408
            },
            {//05 Capture ISO100
                0x081f0814, 0x01030332, 0x010f000f, 0x32800707, 0x181f0232, 0x00000008, 0x70080115, 0x00302010, 0x18141008
            },
            {//06 no one use this parameter, this is Max EE
                0x081f0814, 0x01030332, 0x010f000f, 0x32800707, 0x181f0232, 0x00000008, 0x8055451d, 0x00302010, 0x1f221814
            }
        },
        {//Auto Defect
            {//00
                0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
            },
            {//01
                0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
            },
            {//02
                0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
            }
        },
        {//YCCGO saturation
         //Org value
            {//00
                //0x00000009, 0x80807F01, 0x1020E0F0, 0x081E1E1E, 0x080A0000, 0xFF000040, 0xFF00FF00
                0x00000001, 0x00000000, 0x1020e0f0, 0x081e1e1e, 0x08000000, 0xff000000, 0xff00ff00
                //0x00000009, 0x80807f01, 0x1020e0f0, 0x20464846, 0x20000000, 0xff000040, 0xff00ff00
            },
            {//01
                //0x00000009, 0x80807F01, 0x1020E0F0, 0x0A282828, 0x0A0A0000, 0xFF000040, 0xFF00FF00
             	  0x00000001, 0x00000000, 0x1020e0f0, 0x0a282828, 0x0a000000, 0xff000000, 0xff00ff00
                //0x00000009, 0x80807f01, 0x1020e0f0, 0x20464846, 0x20000000, 0xff000040, 0xff00ff00
            },
            {//02
                //0x00000009, 0x80807F01, 0x1020E0F0, 0x12323432, 0x120A0000, 0xFF000040, 0xFF00FF00
                0x00000001, 0x00000000, 0x1020e0f0, 0x12323432, 0x12000000, 0xff000000, 0xff00ff00
                //0x00000009, 0x80807f01, 0x1020e0f0, 0x20464846, 0x20000000, 0xff000040, 0xff00ff00
            },
            {//03
                //0x00000009, 0x80807F01, 0x1020E0F0, 0x20464846, 0x200A0000, 0xFF000040, 0xFF00FF00
                //0x00000009, 0x80807f01, 0x1020e0f0, 0x20464846, 0x20000000, 0xff000040, 0xff00ff00
                //                                       0x1020E0F0, 0x20464846, 0x200A0000
                //                                                                              0x40000000, 0xFF000040
               0x00000001, 0x00000000, 0x1020e0f0, 0x20464846, 0x20000000, 0xff000000, 0xff00ff00
                //0x00000009, 0x80807f01, 0x1020e0f0, 0x20464846, 0x20000000, 0xff000040, 0xff00ff00
            },
            {//04
                //0x00000009, 0x80807F01, 0x1020E0F0, 0x245A5C5A, 0x240A0000, 0xFF000040, 0xFF00FF00
                0x00000001, 0x00000000, 0x1020e0f0, 0x245a5c5a, 0x24000000, 0xff000000, 0xff00ff00,
                //0x00000009, 0x80807f01, 0x1020e0f0, 0x20464846, 0x20000000, 0xff000040, 0xff00ff00
            },
            {//05
                //0x00000009, 0x80807F01, 0x1020E0F0, 0x284C4E4C, 0x240A0000, 0xFF000040, 0xFF00FF00
                0x00000001, 0x00000000, 0x1020e0f0, 0x284c4e4c, 0x24000000, 0xff000000, 0xff00ff00
                //0x00000009, 0x80807f01, 0x1020e0f0, 0x20464846, 0x20000000, 0xff000040, 0xff00ff00
            },
            {//06
                //0x00000009, 0x80807F01, 0x1020E0F0, 0x284C4E4C, 0x240A0000, 0xFF000040, 0xFF00FF00
                0x00000001, 0x00000000, 0x1020e0f0, 0x284c4e4c, 0x24000000, 0xff000000, 0xff00ff00
                //0x00000009, 0x80807f01, 0x1020e0f0, 0x20464846, 0x20000000, 0xff000040, 0xff00ff00
            }
        },
        {//YCCGO Contrast
            {//00
                //0x00000009, 0x80807F01, 0x1020E0F0, 0x081E1E1E, 0x080A0000, 0xFF000040, 0xFF00FF00
                0x00000008, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
                //0x00000009, 0x80807f01, 0x1020e0f0, 0x20464846, 0x20000000, 0xff000040, 0xff00ff00
            },
            {//01
                //0x00000009, 0x80807F01, 0x1020E0F0, 0x0A282828, 0x0A0A0000, 0xFF000040, 0xFF00FF00
                0x00000008, 0x00000000, 0x00000000, 0x00000000, 0x0000000, 0x00000000, 0x00000000
                //0x00000009, 0x80807f01, 0x1020e0f0, 0x20464846, 0x20000000, 0xff000040, 0xff00ff00
            },
            {//02
                //0x00000009, 0x80807F01, 0x1020E0F0, 0x12323432, 0x120A0000, 0xFF000040, 0xFF00FF00
                0x00000028, 0x00000000, 0x00000000, 0x00000000, 0x000a0000, 0x00000045, 0x00000000
            }
        },
        {//CCM
            {//00
                0x00339805, 0x0087368F, 0x00838C2F
            },
            {//01
                0x00339805, 0x0087368F, 0x00838C2F
            },
            {//02
                0x00339805, 0x0087368F, 0x00838C2F
            }
        },
        {//Gamma
            {//00
                //0x34241404, 0x72645544, 0xab9e9182, 0xd7cdc2b7, 0xfaf3ebe3
                0x482d1811, 0x897a715b, 0xc2b5a096, 0xe2dcd6cd, 0xf9f7f3ed
            },
            {//01
                //0x34241404, 0x72645544, 0xab9e9182, 0xd7cdc2b7, 0xfaf3ebe3
                0x482d1811, 0x897a715b, 0xc2b5a096, 0xe2dcd6cd, 0xf9f7f3ed
            },
            {//02
                0x482d1811, 0x897a715b, 0xc2b5a096, 0xe2dcd6cd, 0xf9f7f3ed
            },
            {//03
                //0x553f280c, 0xa08f7d69, 0xd7cabeaf, 0xf6f0eae1, 0xfffefdfa
                0x482d1811, 0x897a715b, 0xc2b5a096, 0xe2dcd6cd, 0xf9f7f3ed
            },
            {//04
                //0x553f280c, 0xa08f7d69, 0xd7cabeaf, 0xf6f0eae1, 0xfffefdfa
                0x482d1811, 0x897a715b, 0xc2b5a096, 0xe2dcd6cd, 0xf9f7f3ed
            },                  
        }
    },
    {//SHUTTER_DELAY_STRUCT MShutter;
        0, 0
    },
    {
        0
    }
};



const NVRAM_CAMERA_3A_STRUCT S5K5CAGX_YUV_CAMERA_3A_NVRAM_DEFAULT_VALUE=
{
	NVRAM_CAMERA_3A_FILE_VERSION, // u4Version
	S5K5CAGX_SENSOR_ID, // SensorId

    // AE NVRAM
	{
        // rDevicesInfo
		{
            1195,   // u4MinGain, 1024 base =  1x
            16384,  // u4MaxGain, 16x
            70,     // u4MiniISOGain, ISOxx
            128,    // u4GainStepUnit, 1x/8
            31,     // u4PreExpUnit
            30,     // u4PreMaxFrameRate
            31,     // u4VideoExpUnit
            30,     // u4VideoMaxFrameRate
            1024,   // u4Video2PreRatio, 1024 base = 1x
            58,     // u4CapExpUnit
            30,     // u4CapMaxFrameRate
            1024,   // u4Cap2PreRatio, 1024 base = 1x
            28      // u4LensFno, Fno = 2.8
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
            47,              // u4AETarget
            20,              // u4InitIndex
            4,               // u4BackLightWeight
            32,              // u4HistStretchWeight
            4,               // u4AntiOverExpWeight
            2,               // u4BlackLightStrengthIndex
            2,               // u4HistStretchStrengthIndex
            2,               // u4AntiOverExpStrengthIndex
            2,               // u4TimeLPFStrengthIndex
			{3, 4, 5, 6, 7}, // u4LPFConvergeTable[AE_CCT_STRENGTH_NUM]
            90,              // u4InDoorEV = 9.0, 10 base
            -23,               // u4BVOffset delta BV = -2.3
            4,               // u4PreviewFlareOffset
            0,               // u4CaptureFlareOffset
            5,                // u4CaptureFlareThres
            8                 // u4MaxCaptureFlareThres
		}
	},

    // AWB NVRAM
	{
        // AWB calibration data
		{
            // rCalGain (calibration gain: 1.0 = 512)
			{
		        0, // u4R
		        0, // u4G
		        0  // u4B
			},
            // rDefGain (Default calibration gain: 1.0 = 512)
			{
				0,	// u4R
				0,	// u4G
				0	// u4B
			},
			// rD65Gain (D65 WB gain: 1.0 = 512)
			{
				826, // u4R
				512, // u4G
				588	 // u4B
			}
		},
        // Original XY coordinate of AWB light source
		{
		    // Horizon
			{
			    -466, // i4X
				-276  // i4Y
			},
			// A
			{
				-298, // i4X
				-284  // i4Y
			},
			// TL84
			{
				-78, // i4X
				-285 // i4Y
			},
			// CWF
			{
				-65, // i4X
				-369 // i4Y
			},
			// DNP
			{
				107, // i4X
				-316 // i4Y
			},
			// D65
			{
				257, // i4X
				-281 // i4Y
			},
			// D75
			{
				392, // i4X
				-277 // i4Y
			},
		},
		// Rotated XY coordinate of AWB light source
		{
			// Horizon
			{
				-466, // i4X
				-276  // i4Y
			},
			// A
			{
				-298, // i4X
				-284  // i4Y
			},
			// TL84
			{
				-78, // i4X
				-285 // i4Y
			},
			// CWF
			{
				-65, // i4X
				-369 // i4Y
			},
			// DNP
			{
				107, // i4X
				-316 // i4Y
			},
			// D65
			{
				257, // i4X
				-281 // i4Y
			},
			// D75
			{
				392, // i4X
				-277 // i4Y
			},
		},
		// Rotation matrix parameter
		{
			0,   // i4RotationAngle
			128, // i4H11
			0,   // i4H12
			0,   // i4H21
			128  // i4H22
		},
		// Daylight locus parameter
		{
			-128, // i4SlopeNumerator
			128   // i4SlopeDenominator
		},
		// AWB light area
		{
			// Tungsten
			{
				-128, // i4RightBound
				-778, // i4LeftBound
				-230, // i4UpperBound
				-330  // i4LowerBound
			},
			// Warm fluorescent
			{
				-128, // i4RightBound
				-778, // i4LeftBound
				-330, // i4UpperBound
				-450  // i4LowerBound
			},
			// Fluorescent
			{
				57,   // i4RightBound
				-128, // i4LeftBound
				-220, // i4UpperBound
				-327  // i4LowerBound
			},
			// CWF
			{
				57,   // i4RightBound
				-128, // i4LeftBound
				-327, // i4UpperBound
				-419  // i4LowerBound
			},
			// Daylight
			{
				325,  // i4RightBound
				57,   // i4LeftBound
				-201, // i4UpperBound
				-361  // i4LowerBound
			},
			// Shade
			{
				685,  // i4RightBound
				325,  // i4LeftBound
				-197, // i4UpperBound
				-357  // i4LowerBound
			}
		},
		// PWB light area
		{
			// Reference area
			{
				685,  // i4RightBound
				-778, // i4LeftBound
				-197, // i4UpperBound
				-450  // i4LowerBound
			},
			// Daylight
			{
				325,  // i4RightBound
				57,   // i4LeftBound
				-201, // i4UpperBound
				-361  // i4LowerBound
			},
			// Cloudy daylight
			{
				472,  // i4RightBound
				325,  // i4LeftBound
				-197, // i4UpperBound
				-357  // i4LowerBound
			},
			// Shade
			{
				632,  // i4RightBound
				472,  // i4LeftBound
				-197, // i4UpperBound
				-357  // i4LowerBound
			},
			// Twilight
			{
				57,   // i4RightBound
				-103, // i4LeftBound
				-201, // i4UpperBound
				-361  // i4LowerBound
			},
			// Fluorescent
			{
				307,  // i4RightBound
				-128, // i4LeftBound
				-235, // i4UpperBound
				-419  // i4LowerBound
			},
			// Warm fluorescent
			{
				-128, // i4RightBound
				-348, // i4LeftBound
				-235, // i4UpperBound
				-419  // i4LowerBound
			},
			// Incandescent
			{
				-128, // i4RightBound
				-348, // i4LeftBound
				-201, // i4UpperBound
				-361  // i4LowerBound
			}
		},
		// PWB default gain
		{
			// Daylight
			{
				882, // u4R
				512, // u4G
				568, // u4B
			},
			// Cloudy daylight
			{
				907, // u4R
				512, // u4G
				362, // u4B
			},
			// Shade
			{
				1330, // u4R
				512, // u4G
				373, // u4B
			},
			// Twilight
			{
				689, // u4R
				512, // u4G
				727, // u4B
			},
			// Fluorescent
			{
				827, // u4R
				512, // u4G
				673, // u4B
			},
			// Warm fluorescent
			{
				567, // u4R
				512, // u4G
				981, // u4B
			},
			// Incandescent
			{
				538, // u4R
				512, // u4G
				931, // u4B
			}
		},
		// AWB preference color
		{
			// Tungsten
			{
				50,  // i4SliderValue
				4460 // i4OffsetThr
			},
			// Warm fluorescent
			{
				50,  // i4SliderValue
				4460 // i4OffsetThr
			},
			// Shade
			{
				50,  // i4SliderValue
				788  // i4OffsetThr
			},
			// Daylight WB gain
			{
				800, // u4R
				512, // u4G
				626  // u4B
			}
		},
		// CCT estimation
		{
			// CCT
			{
				2400, // i4CCT[0]
				2850, // i4CCT[1]
				4100, // i4CCT[2]
				5100, // i4CCT[3]
				6500, // i4CCT[4]
				7500  // i4CCT[5]
			},
			// Rotated X coordinate
			{
				-723, // i4RotatedXCoordinate[0]
				-555, // i4RotatedXCoordinate[1]
				-335, // i4RotatedXCoordinate[2]
				-150, // i4RotatedXCoordinate[3]
				   0, // i4RotatedXCoordinate[4]
				135  // i4RotatedXCoordinate[5]
			}
		}
	},
	{0}
};

UINT32 S5K5CAGX_YUV_getDefaultData(CAMERA_DATA_TYPE_ENUM CameraDataType, VOID *pDataBuf, UINT32 size)
{
UINT32 dataSize[CAMERA_DATA_TYPE_NUM] = {sizeof(NVRAM_CAMERA_PARA_STRUCT),
                                         sizeof(NVRAM_CAMERA_3A_STRUCT),
                                         sizeof(NVRAM_CAMERA_SHADING_STRUCT),
                                         sizeof(NVRAM_CAMERA_DEFECT_STRUCT),
                                         sizeof(NVRAM_SENSOR_DATA_STRUCT),
                                         sizeof(NVRAM_LENS_PARA_STRUCT),
                                         0,
                                         0};

    if (CameraDataType > CAMERA_NVRAM_DATA_SENSOR || NULL == pDataBuf || (size < dataSize[CameraDataType]))
    {
        return 1;
    }

    switch(CameraDataType)
    {
        case CAMERA_NVRAM_DATA_PARA:
            memcpy(pDataBuf,&S5K5CAGX_YUV_CAMERA_PARA_DEFAULT_VALUE,sizeof(NVRAM_CAMERA_PARA_STRUCT));
            break;
        case CAMERA_NVRAM_DATA_3A:
            memcpy(pDataBuf,&S5K5CAGX_YUV_CAMERA_3A_NVRAM_DEFAULT_VALUE,sizeof(NVRAM_CAMERA_3A_STRUCT));
            break;
        case CAMERA_NVRAM_DATA_SHADING:
            memcpy(pDataBuf,&S5K5CAGX_YUV_CAMERA_SHADING_DEFAULT_VALUE,sizeof(NVRAM_CAMERA_SHADING_STRUCT));
            break;
        case CAMERA_NVRAM_DATA_DEFECT:
            memcpy(pDataBuf,&S5K5CAGX_YUV_CAMERA_DEFECT_DEFAULT_VALUE,sizeof(NVRAM_CAMERA_DEFECT_STRUCT));
            break;
        case CAMERA_NVRAM_DATA_SENSOR:
            break;
        case CAMERA_DATA_3A_PARA:
            break;
        case CAMERA_DATA_3A_STAT_CONFIG_PARA:
            break;
        default:
            break;
    }
    return 0;
}//

#endif  //  defined(MT6516)

typedef NSFeature::YUVSensorInfo<SENSOR_ID> SensorInfoSingleton_T;
namespace NSFeature {
template <>
UINT32
SensorInfoSingleton_T::
impGetDefaultData(CAMERA_DATA_TYPE_ENUM const CameraDataType, VOID*const pDataBuf, UINT32 const size) const
{
#if defined(MT6516)
    return  S5K5CAGX_YUV_getDefaultData(CameraDataType, pDataBuf, size);
#else
    return  NULL;
#endif  //  defined(MT6516)
}};  //  NSFeature











//PFUNC_GETCAMERADEFAULT pS5K5CAGX_YUV_getDefaultData = S5K5CAGX_YUV_getDefaultData;

/*
#endif //#if !defined(ISP_SUPPORT)

#endif //#if (defined(DRV_ISP_6516_SERIES))
*/

