//#include "eeprom_layout.h"
#include "camera_custom_eeprom.h"
 
EEPROM_TYPE_ENUM EEPROMInit(void)
{
	return EEPROM_USED;
}

unsigned int EEPROMDeviceName(char* DevName)
{
       char* str= "EEPROM_S24CS64A";
       strcat (DevName, str );
	return 0;
}


