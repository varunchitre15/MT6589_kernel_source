#include "camera_custom_cam_cal.h"//for 658x new compilier option#include "camera_custom_eeprom.h"

CAM_CAL_TYPE_ENUM CAM_CALInit(void)
{
	return CAM_CAL_NONE;
}

unsigned int CAM_CALDeviceName(char* DevName)
{
	return 1;
}

