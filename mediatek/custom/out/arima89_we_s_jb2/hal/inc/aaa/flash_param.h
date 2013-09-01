

/*
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef __FLASH_PARAM_H__
#define __FLASH_PARAM_H__

#include <flash_feature.h>



enum
{
	ENUM_FLASH_TIME_NO_TIME_OUT = 1000000,


};

enum
{
	ENUM_FLASH_ENG_INDEX_MODE=0, // duty & step or dig (external driver)
	ENUM_FLASH_ENG_CURRENT_MODE,
	ENUM_FLASH_ENG_PERCENTAGE_MODE,
};

typedef struct
{
    MINT32 flashMode; //LIB3A_FLASH_MODE_AUTO, LIB3A_FLASH_MODE_ON, LIB3A_FLASH_MODE_OFF,
    MBOOL  isFlash; //0: no flash, 1: image with flash

} FLASH_INFO_T;



typedef struct
{
	int yTar;
	int antiIsoLevel;
	int antiExpLevel;
	int antiStrobeLevel;
	int antiUnderLevel;
	int antiOverLevel;
	int foregroundLevel;
	int isRefAfDistance;
    int accuracyLevel;

} FLASH_TUNING_PARA;

typedef struct
{
	//torch, video
	int torchEngMode;
	int torchPeakI;
    int torchAveI;
	int torchDuty;
	int torchStep;

	//AF
	int afEngMode;
	int afPeakI;
    int afAveI;
	int afDuty;
	int afStep;

	//pf, mf
	int pmfEngMode;
	//normal bat setting
	int pfAveI;
	int mfAveIMax;
	int mfAveIMin;
	int pmfPeakI;
	int pfDuty;
	int mfDutyMax;
	int mfDutyMin;
	int pmfStep;
	//low bat setting
	int IChangeByVBatEn;
	int vBatL;	//mv
	int pfAveIL;
	int mfAveIMaxL;
	int mfAveIMinL;
	int pmfPeakIL;
	int pfDutyL;
	int mfDutyMaxL;
	int mfDutyMinL;
	int pmfStepL;
	//burst setting
	int IChangeByBurstEn;
	int pfAveIB;
	int mfAveIMaxB;
	int mfAveIMinB;
	int pmfPeakIB;
	int pfDutyB;
	int mfDutyMaxB;
	int mfDutyMinB;
	int pmfStepB;
} FLASH_ENG_LEVEL;


typedef struct
{
	int tabNum;
	int tabMode;
	int tabId[10]; //index or current
	float coolingTM[10]; //time multiply factor
	int timOutMs[10];

}FLASH_COOL_TIMEOUT_PARA;


typedef struct
{
	//stable current
	int extrapI;
	int extrapRefI;
	//calibration
	int minPassI;
	int maxTestI;
	int minTestBatV;
	int toleranceI;
	int toleranceV;

}FLASH_CALI_PARA;


typedef struct
{
	int dutyNum;
	int stepNum;
	FLASH_TUNING_PARA tuningPara;
	FLASH_ENG_LEVEL engLevel;
	FLASH_CALI_PARA caliPara;
	FLASH_COOL_TIMEOUT_PARA coolTimeOutPara;
	int maxCapExpTimeUs; //us
	int pfExpFollowPline; //0: increase frame rate during pf
	int maxPfAfe; //max afe gain during pf


} FLASH_PROJECT_PARA;





#endif  //#ifndef __FLASH_PARAM_H__

