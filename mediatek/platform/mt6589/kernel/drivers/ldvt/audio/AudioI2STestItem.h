/*
 * Copyright (C) 2007 The Android Open Source Project
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
 *   AudioI2STestItem.h
 *
 * Project:
 * --------
 *   Android Audio Driver
 *
 * Description:
 * ------------
 *   Audio I2S related functions
 *
 * Author:
 * -------
 *   Harvey Huang (mtk03996)
 *
 *------------------------------------------------------------------------------
 * $Revision: #5 $
 * $Modtime:$
 * $Log:$
 *
 *
 *******************************************************************************/

#ifndef ANDROID_AUDIO_I2S_H
#define ANDROID_AUDIO_I2S_H

#include "AudioAfe.h"

extern void vI2sInToDacOn(MEMIF_CONFIG_T *memCfg,SAMPLINGRATE_T eSampleRate, BOOL useFOC, I2SSRC_T inSrc);

#endif /*ANDROID_AUDIO_I2S_H*/

