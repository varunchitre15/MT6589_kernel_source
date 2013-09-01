

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
//for 89
//fine 26Mbps->18Mbps High 9Mbps->6Mbps  30fps->20fps  
//Final Fine 12Mbps 20fps, High 4Mbps 20fps
//night mode fine 13Mbps->9Mbps  High 4.5Mbps->3Mbps 15fps->15fps

//customer can change these drop rate for each profile
//or add new profile
static  VideoQualityAdjustParam sVideoQualityAdjustParamTable[] = {
	{BACK_CAMERA,CAMCORDER_QUALITY_MTK_MPEG4_1080P,64,100},
	{BACK_CAMERA,CAMCORDER_QUALITY_MTK_FINE,70,66},
	{BACK_CAMERA,CAMCORDER_QUALITY_MTK_HIGH,66,66},
	{BACK_CAMERA,CAMCORDER_QUALITY_MTK_NIGHT_FINE,70,100},
	{BACK_CAMERA,CAMCORDER_QUALITY_MTK_NIGHT_HIGH,66,100},
	
	{FRONT_CAMERA,CAMCORDER_QUALITY_MTK_MPEG4_1080P,64,100},
	{FRONT_CAMERA,CAMCORDER_QUALITY_MTK_FINE,70,66},
	{FRONT_CAMERA,CAMCORDER_QUALITY_MTK_HIGH,66,66},
	{FRONT_CAMERA,CAMCORDER_QUALITY_MTK_NIGHT_FINE,70,100},
	{FRONT_CAMERA,CAMCORDER_QUALITY_MTK_NIGHT_HIGH,66,100},
	
};


};

#endif  //CUSTOM_VR_VIDEO_DROP_RATE_H

