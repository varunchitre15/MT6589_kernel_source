#ifndef _CFG_FTBL_OV3640_YUV_H_
#define _CFG_FTBL_OV3640_YUV_H_


namespace NSYUV
{


namespace NSSceneIndep
{
GETFINFO_SCENE_INDEP()
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //..........................................................................
    //  Scene Mode
    CONFIG_FEATURE_SI(FID_SCENE_MODE, 
        BY_DEFAULT(SCENE_MODE_OFF), 
        SCENE_MODE_OFF, SCENE_MODE_NIGHTSCENE
    )
    //..........................................................................
    //  Effect
    CONFIG_FEATURE_SI(FID_COLOR_EFFECT, 
        BY_DEFAULT(MEFFECT_OFF), 
        MEFFECT_OFF, MEFFECT_MONO, MEFFECT_SEPIA, MEFFECT_NEGATIVE,
        MEFFECT_SEPIAGREEN, MEFFECT_SEPIABLUE
    )
    //..........................................................................
    //  Capture Mode
    CONFIG_FEATURE_SI(FID_CAPTURE_MODE, 
        BY_DEFAULT(CAPTURE_MODE_NORMAL), 
        CAPTURE_MODE_NORMAL,     CAPTURE_MODE_BURST_SHOT, 
        CAPTURE_MODE_SMILE_SHOT, CAPTURE_MODE_PANORAMA
    )
    //..........................................................................
    //  Capture Size
    CONFIG_FEATURE_SI(FID_CAP_SIZE, 
        BY_DEFAULT(CAPTURE_SIZE_2048_1536), 
        CAPTURE_SIZE_2048_1536,CAPTURE_SIZE_1600_1200,
        CAPTURE_SIZE_1280_960,CAPTURE_SIZE_640_480,        
    )
    //..........................................................................
    //  Preview Size
    CONFIG_FEATURE_SI(FID_PREVIEW_SIZE, 
        BY_DEFAULT(PREVIEW_SIZE_320_240), 
        PREVIEW_SIZE_176_144, PREVIEW_SIZE_320_240, 
        PREVIEW_SIZE_352_288, PREVIEW_SIZE_480_320,
        PREVIEW_SIZE_480_368, PREVIEW_SIZE_640_480
    )
    //..........................................................................
    //  Frame Rate
    CONFIG_FEATURE_SI(FID_FRAME_RATE, 
        BY_DEFAULT(FRAME_RATE_300FPS), 
        FRAME_RATE_150FPS, FRAME_RATE_300FPS
    )
    //..........................................................................
    //  Frame Rate Range
    CONFIG_FEATURE_SI(FID_FRAME_RATE_RANGE, 
        BY_DEFAULT(FRAME_RATE_RANGE_5_30_FPS), 
        FRAME_RATE_RANGE_5_30_FPS
    )
    //..........................................................................
    //  Focus Distance Normal
    CONFIG_FEATURE_SI(FID_FOCUS_DIST_NORMAL, 
        BY_DEFAULT(FOCUS_DIST_N_10CM), 
        FOCUS_DIST_N_10CM
    )
    //..........................................................................
    //  Focus Distance Macro
    CONFIG_FEATURE_SI(FID_FOCUS_DIST_MACRO, 
        BY_DEFAULT(FOCUS_DIST_M_5CM), 
        FOCUS_DIST_M_5CM
    )
    //..........................................................................
    //  AE Flicker
    CONFIG_FEATURE_SI(FID_AE_FLICKER, 
        BY_DEFAULT(AE_FLICKER_MODE_50HZ), 
        AE_FLICKER_MODE_60HZ, AE_FLICKER_MODE_50HZ
    )
//------------------------------------------------------------------------------
END_GETFINFO_SCENE_INDEP()
};  //  namespace NSSceneIndep


namespace NSSceneDep  
{
GETFINFO_SCENE_DEP()
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //  Scene Auto
    CONFIG_SCENE(SCENE_MODE_OFF)
        //......................................................................
#if 1
        //  AE Mode
        CONFIG_FEATURE_SD(FID_AE_SCENE_MODE, 
            BY_DEFAULT(AE_MODE_AUTO), 
            AE_MODE_AUTO
        )
#endif
        //......................................................................
#if 0
        //  AE Meter
        CONFIG_FEATURE_SD(FID_AE_METERING, 
            BY_DEFAULT(AE_METERING_MODE_CENTER_WEIGHT), 
            AE_METERING_MODE_CENTER_WEIGHT, AE_METERING_MODE_SOPT, 
            AE_METERING_MODE_AVERAGE
        )
#endif
        //......................................................................
        //  AE ISO
        CONFIG_FEATURE_SD(FID_AE_ISO, 
            BY_DEFAULT(AE_ISO_AUTO), 
            AE_ISO_AUTO
        )
        //......................................................................
        //  AE EV
        CONFIG_FEATURE_SD(FID_AE_EV, 
            BY_DEFAULT(AE_EV_COMP_00), 
            AE_EV_COMP_00, 
            AE_EV_COMP_03,  AE_EV_COMP_07,  AE_EV_COMP_10,  AE_EV_COMP_13, 
            AE_EV_COMP_n03, AE_EV_COMP_n07, AE_EV_COMP_n10, AE_EV_COMP_n13
        )
        //......................................................................
#if 1
        //  AF Mode
        CONFIG_FEATURE_SD(FID_AF_MODE, 
            BY_DEFAULT(AF_MODE_AFS), 
            AF_MODE_AFS,AF_MODE_INFINITY
        )
#endif
        //......................................................................
#if 1
        //  AF Meter
        CONFIG_FEATURE_SD(FID_AF_METERING, 
            BY_DEFAULT(AF_METER_MOVESPOT), 
            AF_METER_SPOT, AF_METER_MATRIX,AF_METER_MOVESPOT
        )
#endif
        //......................................................................
        //  AWB Mode
        CONFIG_FEATURE_SD(FID_AWB_MODE, 
            BY_DEFAULT(AWB_MODE_AUTO), 
            AWB_MODE_AUTO, AWB_MODE_DAYLIGHT, 
            AWB_MODE_CLOUDY_DAYLIGHT, AWB_MODE_FLUORESCENT, 
            AWB_MODE_INCANDESCENT, AWB_MODE_TUNGSTEN
        )
        //......................................................................
#if 0
        //  ISP Edge
        CONFIG_FEATURE_SD(FID_ISP_EDGE, 
            BY_DEFAULT(ISP_EDGE_MIDDLE), 
            ISP_EDGE_LOW, ISP_EDGE_MIDDLE, ISP_EDGE_HIGH
        )
#endif
        //......................................................................
#if 0
        //  ISP Hue
        CONFIG_FEATURE_SD(FID_ISP_HUE, 
            BY_DEFAULT(ISP_HUE_MIDDLE), 
            ISP_HUE_LOW, ISP_HUE_MIDDLE, ISP_HUE_HIGH
        )
#endif
        //......................................................................
#if 0
        //  ISP Saturation
        CONFIG_FEATURE_SD(FID_ISP_SAT, 
            BY_DEFAULT(ISP_SAT_MIDDLE), 
            ISP_SAT_LOW, ISP_SAT_MIDDLE, 
            ISP_SAT_HIGH
        )
#endif
        //......................................................................
#if 0
        //  ISP Brightness
        CONFIG_FEATURE_SD(FID_ISP_BRIGHT, 
            BY_DEFAULT(ISP_BRIGHT_MIDDLE), 
            ISP_BRIGHT_LOW, ISP_BRIGHT_MIDDLE, 
            ISP_BRIGHT_HIGH
        )
#endif
        //......................................................................
#if 0
        //  ISP Contrast
        CONFIG_FEATURE_SD(FID_ISP_CONTRAST, 
            BY_DEFAULT(ISP_CONTRAST_MIDDLE), 
            ISP_CONTRAST_LOW, ISP_CONTRAST_MIDDLE, ISP_CONTRAST_HIGH
        )
#endif
        //......................................................................
    END_CONFIG_SCENE()
    //==========================================================================
    //  Scene Night
    CONFIG_SCENE(SCENE_MODE_NIGHTSCENE)
        //......................................................................
#if 1
        //  AE Mode
        CONFIG_FEATURE_SD(FID_AE_SCENE_MODE, 
            BY_DEFAULT(AE_MODE_AUTO), 
            AE_MODE_AUTO
        )
#endif
        //......................................................................
#if 0
        //  AE Meter
        CONFIG_FEATURE_SD(FID_AE_METERING, 
            BY_DEFAULT(AE_METERING_MODE_CENTER_WEIGHT), 
            AE_METERING_MODE_CENTER_WEIGHT, AE_METERING_MODE_SOPT, 
            AE_METERING_MODE_AVERAGE
        )
#endif
        //......................................................................
        //  AE ISO
        CONFIG_FEATURE_SD(FID_AE_ISO, 
            BY_DEFAULT(AE_ISO_AUTO), 
            AE_ISO_AUTO
        )
        //......................................................................
        //  AE EV
        CONFIG_FEATURE_SD(FID_AE_EV, 
            BY_DEFAULT(AE_EV_COMP_00), 
            AE_EV_COMP_00, 
            AE_EV_COMP_03,  AE_EV_COMP_07,  AE_EV_COMP_10,  AE_EV_COMP_13, 
            AE_EV_COMP_n03, AE_EV_COMP_n07, AE_EV_COMP_n10, AE_EV_COMP_n13
        )
        //......................................................................
#if 1
        //  AF Mode
        CONFIG_FEATURE_SD(FID_AF_MODE, 
            BY_DEFAULT(AF_MODE_AFS), 
            AF_MODE_AFS,AF_MODE_INFINITY
        )
#endif
        //......................................................................
#if 1
        //  AF Meter
        CONFIG_FEATURE_SD(FID_AF_METERING, 
            BY_DEFAULT(AF_METER_MOVESPOT), 
            AF_METER_SPOT, AF_METER_MATRIX,AF_METER_MOVESPOT
        )
#endif
        //......................................................................
        //  AWB Mode
        CONFIG_FEATURE_SD(FID_AWB_MODE, 
            BY_DEFAULT(AWB_MODE_AUTO), 
            AWB_MODE_AUTO, AWB_MODE_DAYLIGHT, 
            AWB_MODE_CLOUDY_DAYLIGHT, AWB_MODE_FLUORESCENT, 
            AWB_MODE_INCANDESCENT, AWB_MODE_TUNGSTEN
        )
        //......................................................................
#if 0
        //  ISP Edge
        CONFIG_FEATURE_SD(FID_ISP_EDGE, 
            BY_DEFAULT(ISP_EDGE_MIDDLE), 
            ISP_EDGE_LOW, ISP_EDGE_MIDDLE, ISP_EDGE_HIGH
        )
#endif
        //......................................................................
#if 0
        //  ISP Hue
        CONFIG_FEATURE_SD(FID_ISP_HUE, 
            BY_DEFAULT(ISP_HUE_MIDDLE), 
            ISP_HUE_LOW, ISP_HUE_MIDDLE, ISP_HUE_HIGH
        )
#endif
        //......................................................................
#if 0
        //  ISP Saturation
        CONFIG_FEATURE_SD(FID_ISP_SAT, 
            BY_DEFAULT(ISP_SAT_MIDDLE), 
            ISP_SAT_LOW, ISP_SAT_MIDDLE, 
            ISP_SAT_HIGH
        )
#endif
        //......................................................................
#if 0
        //  ISP Brightness
        CONFIG_FEATURE_SD(FID_ISP_BRIGHT, 
            BY_DEFAULT(ISP_BRIGHT_MIDDLE), 
            ISP_BRIGHT_LOW, ISP_BRIGHT_MIDDLE, 
            ISP_BRIGHT_HIGH
        )
#endif
        //......................................................................
#if 0
        //  ISP Contrast
        CONFIG_FEATURE_SD(FID_ISP_CONTRAST, 
            BY_DEFAULT(ISP_CONTRAST_MIDDLE), 
            ISP_CONTRAST_LOW, ISP_CONTRAST_MIDDLE, ISP_CONTRAST_HIGH
        )
#endif
        //......................................................................
    END_CONFIG_SCENE()
//------------------------------------------------------------------------------
END_GETFINFO_SCENE_DEP()
};  //  namespace NSSceneDep


};  //  namespace NSYUV


#endif // _CFG_FTBL_OV3640_YUV_H_

