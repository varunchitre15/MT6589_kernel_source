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

#define LOG_TAG "AudioCustomization"
//#define LOG_NDEBUG 0
#include <utils/Log.h>
#include <audio_customization.h>

namespace android {


// ----------------------------------------------------------------------------
// AudioPolicyInterface implementation
// ----------------------------------------------------------------------------

// AudioCustomizeInterface

AudioCustomization::AudioCustomization()
{
    LOGD("AudioCustomization contructor");
}

AudioCustomization::~AudioCustomization()
{
    LOGD("AudioCustomization destructor");
}


void AudioCustomization::EXT_DAC_Init()
{
    LOGD("AudioCustomization EXT_DAC_Init");
}


void AudioCustomization::EXT_DAC_SetPlaybackFreq(unsigned int frequency)
{
    LOGD("AudioCustomization EXT_DAC_SetPlaybackFreq frequency = %d",frequency);
}

void AudioCustomization::EXT_DAC_TurnOnSpeaker(unsigned int source ,unsigned int speaker)
{
    LOGD("AudioCustomization EXT_DAC_TurnOnSpeaker source = %d speaker = %d",source,speaker);
}

void AudioCustomization::EXT_DAC_TurnOffSpeaker(unsigned int source , unsigned int speaker)
{
    LOGD("AudioCustomization EXT_DAC_TurnOffSpeaker source = %d speaker = %d",source,speaker);
}

void AudioCustomization::EXT_DAC_SetVolume(unsigned int speaker,unsigned int vol)
{
    LOGD("AudioCustomization EXT_DAC_SetVolume speaker = %d vol = %d",speaker,vol);
}

}; // namespace android
