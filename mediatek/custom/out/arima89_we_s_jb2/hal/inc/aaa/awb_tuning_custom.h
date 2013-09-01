

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

#ifndef _AWB_TUNING_CUSTOM_H
#define _AWB_TUNING_CUSTOM_H


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//         P U B L I C    F U N C T I O N    D E C L A R A T I O N              //
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
MBOOL isAWBEnabled();
MBOOL isAWBCalibrationBypassed();
AWB_PARAM_T const& getAWBParam();
AWB_STAT_PARAM_T const& getAWBStatParam();
const MINT32* getAWBActiveCycle(MINT32 i4SceneLV);
MINT32 getAWBCycleNum();

#endif

