

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

#include "camera_custom_types.h"
#include "tsf_tuning_custom.h"
#include "TSF_data.h"
#include "TSF_para.h"

MVOID *
getTSFTrainingData(void)
{
    return tsf_data;
}

MVOID *
getTSFTuningData(void)
{

    return TSF_para;
}

MBOOL
isEnableTSF(void)
{
    return MFALSE;
}

MUINT32
getTSFD65Idx(void)
{
#define D65_IDX     (2)
    return D65_IDX;
}

