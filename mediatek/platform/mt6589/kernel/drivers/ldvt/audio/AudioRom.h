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
 *   AudioRom.h
 *
 * Project:
 * --------
 *   Android Audio Driver
 *
 * Description:
 * ------------
 *   Audio Rom table definition
 *
 * Author:
 * -------
 *   Ir Lian (mtk00976)
 *
 *------------------------------------------------------------------------------
 * $Revision: #5 $
 * $Modtime:$
 * $Log:$
 *
 *
 *******************************************************************************/

#ifndef ANDROID_AUDIO_ROM_H
#define ANDROID_AUDIO_ROM_H

#include "AudioAfe.h"

#define TBL_SZ_48KHz_1K 48
#define TBL_SZ_48KHz_1p5K 32
#define TBL_SZ_48KHz_500 96
#define TBL_SZ_48KHz_1010 95
#define TBL_SZ_192KHz_1k 192
#define TBL_SZ_RESET 248
#define TBL_SZ_STF_32K_COEFF 29
#define TBL_SZ_STF_32K_INPUT 384
#define TBL_SZ_STF_32K_GOLDEN 441
#define TBL_SZ_STF_16K_COEFF 13
#define TBL_SZ_STF_16K_INPUT 384
#define TBL_SZ_STF_16K_GOLDEN 217
#define TBL_SZ_AC3_RAW 1024
// Tables
extern const UINT8 IntrConCap[IN_MAX][OUT_MAX];
extern const UINT8 IntrConReg[IN_MAX][OUT_MAX];
extern const INT8 IntrConSBit[IN_MAX][OUT_MAX];
extern const INT8 IntrConRBit[IN_MAX][OUT_MAX];

extern const UINT16 tone1k_48kHz[TBL_SZ_48KHz_1K];
extern const UINT16 tone1k_48kHz_ST[TBL_SZ_48KHz_1K * 2];
extern const UINT16 tone1p5k_48kHz[TBL_SZ_48KHz_1p5K];
extern const UINT16 tone500_48kHz[TBL_SZ_48KHz_500];
extern const UINT16 tone1010_48kHz[TBL_SZ_48KHz_1010] ;
extern const UINT16 tone1k_192kHz[TBL_SZ_192KHz_1k] ;
extern const UINT32 tone1k_48kHz_24bit[TBL_SZ_48KHz_1K];
extern const INT32 resetTable[TBL_SZ_RESET];
extern const UINT32 ac3_raw_table[TBL_SZ_AC3_RAW];
extern const UINT32 STF_32K_Coeff_Table[TBL_SZ_STF_32K_COEFF];
extern const UINT16 STF_32K_Input_Table[TBL_SZ_STF_32K_INPUT];
extern const UINT16 STF_32K_Golden_Table[TBL_SZ_STF_32K_GOLDEN];
extern const UINT32 STF_16K_Coeff_Table[TBL_SZ_STF_16K_COEFF];
extern const UINT16 STF_16K_Input_Table[TBL_SZ_STF_16K_INPUT];
extern const UINT16 STF_16K_Golden_Table[TBL_SZ_STF_16K_GOLDEN];

extern const UINT32 table_sgen_golden_values[64];
extern const UINT32 table_sgen_golden_values_ch1_duplicate[64];
extern BOOL check_bit_true(UINT32 mem_base, UINT32 mem_end, const UINT32 *golden_table, UINT32 table_size);
extern BOOL check_bit_true_stress(UINT32 mem_base, UINT32 mem_end, const UINT32 *golden_table, UINT32 table_size);

#endif /*ANDROID_AUDIO_ROM_H*/

