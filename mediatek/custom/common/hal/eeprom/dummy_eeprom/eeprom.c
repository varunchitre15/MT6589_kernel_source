//#include "eeprom_layout.h"
#include "camera_custom_eeprom.h"

EEPROM_TYPE_ENUM EEPROMInit(void)
{
	return EEPROM_NONE;
}

unsigned int EEPROMDeviceName(char* DevName)
{
	return 1;
}

