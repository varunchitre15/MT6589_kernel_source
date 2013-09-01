/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*******************************************************************************
 *
 * Filename:
 * ---------
 * audio_customization.h
 *
 * Project:
 * --------
 *   Android + MT6516
 *
 * Description:
 * ------------
 *   This file implements Customization base function
 *
 * Author:
 * -------
 *   Chipeng chang (mtk02308)
 *
 *------------------------------------------------------------------------------
 * $Revision: #1 $
 * $Modtime:$
 * $Log:$
 *
 *******************************************************************************/



#include <stdint.h>
#include <sys/types.h>
#include <utils/Timers.h>
#include <utils/Errors.h>
#include <utils/KeyedVector.h>
#include <AudioCustomizationBase.h>

namespace android {

//AudioCustomizeInterface  Interface
class AudioCustomization : public AudioCustomizationBase
{

public:
    AudioCustomization();
    ~AudioCustomization();

    // indicate a change in device connection status
    void EXT_DAC_Init(void);

    void EXT_DAC_SetPlaybackFreq(unsigned int frequency);

    void EXT_DAC_TurnOnSpeaker(unsigned int source ,unsigned int speaker);

    void EXT_DAC_TurnOffSpeaker(unsigned int source,unsigned int speaker);

    void EXT_DAC_SetVolume(unsigned int speaker,unsigned int vol);

};

};
