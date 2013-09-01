

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


#include "camera_custom_nvram.h"
#include "camera_custom_types.h"

#include "camera_custom_AEPlinetable.h"
#ifdef WIN32
#else
#include "camera_custom_nvram.h"
#endif

#include <cutils/xlog.h>
#include "flash_feature.h"
#include "flash_param.h"
#include "flash_tuning_custom.h"
#include <kd_camera_feature.h>
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

int cust_isNeedAFLamp(int flashMode, int afLampMode, int isBvHigherTriger)
{
	if(flashMode==FLASHLIGHT_FORCE_OFF)
		return 0;
	if(flashMode==FLASHLIGHT_FORCE_ON)
		return 1;
	//if(afLampMode==AF_LAMP_OFF)
	//	return 0;
	//if(afLampMode==AF_LAMP_ON)
	//	return 1;
	if(isBvHigherTriger==1)
		return 1;
	else
		return 0;
	// #flash mode
	// FLASHLIGHT_AUTO
    // FLASHLIGHT_FORCE_ON,
    // FLASHLIGHT_FORCE_OFF,
    // FLASHLIGHT_REDEYE,
    // FLASHLIGHT_TORCH,

	// #AF lamp mode
	// AF_LAMP_OFF
    // AF_LAMP_ON,
    // AF_LAMP_AUTO,
    // AF_LAMP_FLASH,
}
int cust_isNeedDoPrecapAF(int isFocused, int flashMode, int afLampMode, int isBvLowerTriger)
{	
	return 0;
}

void copyTuningPara(FLASH_TUNING_PARA* p, NVRAM_FLASH_TUNING_PARA* nv_p)
{
	XLOGD("copyTuningPara ytar =%d %d", p->yTar ,nv_p->yTar);
	p->yTar = nv_p->yTar;
	p->antiIsoLevel = nv_p->antiIsoLevel;
	p->antiExpLevel = nv_p->antiExpLevel;
	p->antiStrobeLevel = nv_p->antiStrobeLevel;
	p->antiUnderLevel = nv_p->antiUnderLevel;
	p->antiOverLevel = nv_p->antiOverLevel;
	p->foregroundLevel = nv_p->foregroundLevel;
	p->isRefAfDistance = nv_p->isRefAfDistance;
    p->accuracyLevel = nv_p->accuracyLevel;
}




FLASH_PROJECT_PARA& cust_getFlashProjectPara(int aeMode, NVRAM_CAMERA_STROBE_STRUCT* nvrame)
{
	static FLASH_PROJECT_PARA para;

	para.dutyNum = 3;
	para.stepNum = 1;
	//tuning



	if(nvrame!=0)
	{
		XLOGD("cust_getFlashProjectPara nvrame->tuningPara[0].yTar=%d", nvrame->tuningPara[0].yTar);
		switch(aeMode)
		{
			case LIB3A_AE_MODE_OFF:
			case LIB3A_AE_MODE_AUTO:
				copyTuningPara(&para.tuningPara, &nvrame->tuningPara[0]);
			break;
			case LIB3A_AE_MODE_NIGHT:
			case LIB3A_AE_MODE_CANDLELIGHT:
			case LIB3A_AE_MODE_FIREWORKS:
			case LIB3A_AE_MODE_NIGHT_PORTRAIT:
				copyTuningPara(&para.tuningPara, &nvrame->tuningPara[1]);
			break;
			case LIB3A_AE_MODE_ACTION:
			case LIB3A_AE_MODE_SPORTS:
			case LIB3A_AE_MODE_ISO_ANTI_SHAKE:
				copyTuningPara(&para.tuningPara, &nvrame->tuningPara[2]);
			break;
			case LIB3A_AE_MODE_PORTRAIT:
				copyTuningPara(&para.tuningPara, &nvrame->tuningPara[3]);
			break;
			default:
				copyTuningPara(&para.tuningPara, &nvrame->tuningPara[0]);
			break;
		}
	}
	//--------------------
	//eng level
	//index mode
	//torch
	para.engLevel.torchEngMode = ENUM_FLASH_ENG_INDEX_MODE;
	para.engLevel.torchDuty = 0;
	para.engLevel.torchStep = 0;

	//af
	para.engLevel.afEngMode = ENUM_FLASH_ENG_INDEX_MODE;
	para.engLevel.afDuty = 0;
	para.engLevel.afStep = 0;

	//pf, mf, normal
	para.engLevel.pmfEngMode = ENUM_FLASH_ENG_INDEX_MODE;
	para.engLevel.pfDuty = 1;
	para.engLevel.mfDutyMax = 2;
	para.engLevel.mfDutyMin = 0;
	para.engLevel.pmfStep = 0;

	//low bat
	para.engLevel.IChangeByVBatEn=0;
	para.engLevel.vBatL = 3400;	//mv
	para.engLevel.pfDutyL = 1;
	para.engLevel.mfDutyMaxL = 2;
	para.engLevel.mfDutyMinL = 0;
	para.engLevel.pmfStepL = 0;

	//burst setting
	para.engLevel.IChangeByBurstEn=1;
	para.engLevel.pfDutyB = 1;
	para.engLevel.mfDutyMaxB = 1;
	para.engLevel.mfDutyMinB = 0;
	para.engLevel.pmfStepB = 0;

	//--------------------
	//cooling delay para
	para.coolTimeOutPara.tabMode = ENUM_FLASH_ENG_INDEX_MODE;
	para.coolTimeOutPara.tabNum = 3;
	para.coolTimeOutPara.tabId[0]=0;
	para.coolTimeOutPara.tabId[1]=1;
	para.coolTimeOutPara.tabId[2]=2;
	para.coolTimeOutPara.coolingTM[0]=0;
	para.coolTimeOutPara.coolingTM[1]=0;
	para.coolTimeOutPara.coolingTM[2]=5;

	para.coolTimeOutPara.timOutMs[0]=ENUM_FLASH_TIME_NO_TIME_OUT;
	para.coolTimeOutPara.timOutMs[1]=ENUM_FLASH_TIME_NO_TIME_OUT;
	para.coolTimeOutPara.timOutMs[2]=500;

	/*
	//---------------
	//current mode, for mtk internal pmic
	//torch
	para.engLevel.torchEngMode = ENUM_FLASH_ENG_CURRENT_MODE;
	para.engLevel.torchPeakI = 100;
	para.engLevel.torchAveI = 100;

	//af
	para.engLevel.afEngMode = ENUM_FLASH_ENG_CURRENT_MODE;
	para.engLevel.afPeakI = 200;
	para.engLevel.afAveI = 200;

	//pf, mf normal
	para.engLevel.pmfEngMode = ENUM_FLASH_ENG_CURRENT_MODE;
	para.engLevel.pfAveI = 200;
	para.engLevel.mfAveIMax = 600;
	para.engLevel.mfAveIMin = 50;
	para.engLevel.pmfPeakI = 800;

	//low bat setting
	para.engLevel.IChangeByVBatEn = 0;
	para.engLevel.vBatL = 3400;
	para.engLevel.pfAveIL = 200;
	para.engLevel.mfAveIMaxL = 800;
	para.engLevel.mfAveIMinL = 100;
	para.engLevel.pmfPeakIL =1000;

	//burst setting
	para.engLevel.IChangeByBurstEn=1;
	para.engLevel.pfAveIB = 200;
	para.engLevel.mfAveIMaxB = 400;
	para.engLevel.mfAveIMinB = 100;
	para.engLevel.pmfPeakIB = 500;

	//stable current
	para.engLevel.extrapI = 200;
	para.engLevel.extrapRefI = 200;
	//calibration
	para.engLevel.minPassI = 200;
	para.engLevel.maxTestI  = 800;
	para.engLevel.minTestBatV = 3500;
	para.engLevel.toleranceI = 200;
	para.engLevel.toleranceV = 200;
	*/
	if(nvrame!=0)
	{
		if(nvrame->isTorchEngUpdate)
		{
			para.engLevel.torchEngMode = nvrame->engLevel.torchEngMode;
			para.engLevel.torchPeakI = nvrame->engLevel.torchPeakI;
		    para.engLevel.torchAveI = nvrame->engLevel.torchAveI;
			para.engLevel.torchDuty = nvrame->engLevel.torchDuty;
			para.engLevel.torchStep = nvrame->engLevel.torchStep;
		}
		if(nvrame->isAfEngUpdate)
		{
			para.engLevel.afEngMode = nvrame->engLevel.afEngMode;
			para.engLevel.afPeakI = nvrame->engLevel.afPeakI;
		    para.engLevel.afAveI = nvrame->engLevel.afAveI;
			para.engLevel.afDuty = nvrame->engLevel.afDuty;
			para.engLevel.afStep = nvrame->engLevel.afStep;
		}
		if(nvrame->isNormaEnglUpdate)
		{
			para.engLevel.pfAveI = nvrame->engLevel.pfAveI;
			para.engLevel.mfAveIMax = nvrame->engLevel.mfAveIMax;
			para.engLevel.mfAveIMin = nvrame->engLevel.mfAveIMin;
			para.engLevel.pmfPeakI = nvrame->engLevel.pmfPeakI;
			para.engLevel.pfDuty = nvrame->engLevel.pfDuty;
			para.engLevel.mfDutyMax = nvrame->engLevel.mfDutyMax;
			para.engLevel.mfDutyMin = nvrame->engLevel.mfDutyMin;
			para.engLevel.pmfStep = nvrame->engLevel.pmfStep;
		}
		if(nvrame->isLowBatEngUpdate)
		{
			para.engLevel.IChangeByVBatEn = nvrame->engLevel.IChangeByVBatEn;
			para.engLevel.vBatL = nvrame->engLevel.vBatL;
			para.engLevel.pfAveIL = nvrame->engLevel.pfAveIL;
			para.engLevel.mfAveIMaxL = nvrame->engLevel.mfAveIMaxL;
			para.engLevel.mfAveIMinL = nvrame->engLevel.mfAveIMinL;
			para.engLevel.pmfPeakIL = nvrame->engLevel.pmfPeakIL;
			para.engLevel.pfDutyL = nvrame->engLevel.pfDutyL;
			para.engLevel.mfDutyMaxL = nvrame->engLevel.mfDutyMaxL;
			para.engLevel.mfDutyMinL = nvrame->engLevel.mfDutyMinL;
			para.engLevel.pmfStepL = nvrame->engLevel.pmfStepL;
		}
		if(nvrame->isBurstEngUpdate)
		{
			para.engLevel.IChangeByBurstEn = nvrame->engLevel.IChangeByBurstEn;
			para.engLevel.pfAveIB = nvrame->engLevel.pfAveIB;
			para.engLevel.mfAveIMaxB = nvrame->engLevel.mfAveIMaxB;
			para.engLevel.mfAveIMaxB = nvrame->engLevel.mfAveIMaxB;
			para.engLevel.mfAveIMinB = nvrame->engLevel.mfAveIMinB;
			para.engLevel.pmfPeakIB = nvrame->engLevel.pmfPeakIB;
			para.engLevel.pfDutyB = nvrame->engLevel.pfDutyB;
			para.engLevel.mfDutyMaxB = nvrame->engLevel.mfDutyMaxB;
			para.engLevel.mfDutyMinB = nvrame->engLevel.mfDutyMinB;
			para.engLevel.pmfStepB = nvrame->engLevel.pmfStepB;
		}
	}
	return para;
}


