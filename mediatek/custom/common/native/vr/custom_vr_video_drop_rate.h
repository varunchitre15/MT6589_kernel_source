
#ifndef CUSTOM_VR_VIDEO_DROP_RATE_H
#define CUSTOM_VR_VIDEO_DROP_RATE_H
#include <media/MediaProfiles.h> 
namespace android {

struct VideoQualityAdjustParam{
	camera_id mCameraId; 
	camcorder_quality mQuality;
	int32_t mBitRateDropPercentage; //1~100,such as 25,will mean 25/100
	int32_t mFrameRateDropPercentage; //1~100,such as 50,will mean 50/100
};
//for 77
//hd:14Mbps->9Mbps 15fps->15fps final 15fps, 9Mbps    fine:12.5Mbps->8Mbps  30fps->20fps final 20fps,5Mbps
//night mode hd: 14Mbps->9Mbps 15fps->15fps             fine: 6Mbps->4Mbps  15fps->15fps

//customer can change these drop rate for each profile
//or add new profile
static  VideoQualityAdjustParam sVideoQualityAdjustParamTable[] = {
	{BACK_CAMERA,CAMCORDER_QUALITY_MTK_MPEG4_1080P,64,100},
	{BACK_CAMERA,CAMCORDER_QUALITY_MTK_FINE,64,66},
	{BACK_CAMERA,CAMCORDER_QUALITY_MTK_NIGHT_FINE,66,100},
	{FRONT_CAMERA,CAMCORDER_QUALITY_MTK_MPEG4_1080P,64,100},	
	{FRONT_CAMERA,CAMCORDER_QUALITY_MTK_FINE,64,66},
	{FRONT_CAMERA,CAMCORDER_QUALITY_MTK_NIGHT_FINE,66,100},
};

};
#endif  //CUSTOM_VR_VIDEO_DROP_RATE_H

