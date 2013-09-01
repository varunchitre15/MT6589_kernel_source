

/*****************************************************************************
 *
 * Filename:
 * ---------
 *    audio.h
 *
 * Project:
 * --------
 *
 *
 * Description:
 * ------------
 *   This file is for Audio.
 *
 * Author:
 * -------
 *
 *
 *============================================================================
 *             HISTORY
 * Below this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
#include "AudioAfe.h"

#ifndef MT6589_LDVT_AUDIO_TS_H
#define MT6589_LDVT_AUDIO_TS_H

#define INTERNAL_SRAM_TEST      101

#define MEMORY_UL_MONO_TEST     200
#define MEMORY_LOOP1_TEST       201
#define MEMORY_LOOP2_TEST       202

#define I2S_DAC_OUT_16BIT       301
#define I2S_DAC_OUT_20BIT       302
#define I2S_IN_SLAVE_FOC_ON     303
#define I2S_IN_SLAVE_FOC_OFF    304
#define I2S_IN_MASTER           305

#define HW_GAIN_1   401
#define HW_GAIN_2   402
#define HW_GAIN_COMBINATION 403


#define MOD_PCM_1_EXT_MD_MASTER                         501
#define MOD_PCM_1_EXT_MD_SLAVE_ASRC                     502
#define MOD_PCM_1_EXT_MD_SLAVE_ASYNC_FIFO               503
#define MOD_PCM_1_INT_MD_SLAVE_ASYNC_FIFO               504
#define MOD_PCM_2_INT_MD_SLAVE_ASYNC_FIFO               505
#define MOD_PCM_1_2_INT_MD_SLAVE_ASYNC_FIFO_VBT_16K     506

#define MERGE_IF_PCM_ONLY_08000         601
#define MERGE_IF_I2S_ONLY_44100         602
#define MERGE_IF_PCM_WITH_I2S_32000     603
#define MERGE_IF_PCM_WITH_I2S_44100     604
#define MERGE_IF_PCM_WITH_I2S_48000     605
#define MERGE_IF_I2S_ONLY_CHANGE_FS     606
#define MERGE_IF_I2S_FIRST_PCM_LATER    607
#define MERGE_IF_PCM_FIRST_I2S_LATER    608
#define MERGE_IF_WITH_I2S_IN_MASTER     609
#define MERGE_IF_WITH_I2S_IN_SLAVE      610

extern BOOL fgAudioMemTest();
extern BOOL fgAudioMemLoop1Test();
extern BOOL fgAudioMemLoop2Test();
extern BOOL fgAudioMemULMonoTest();

extern void vDacI2sOut(I2SWLEN_T i2s_wlen);
extern void vI2sIn(I2SSRC_T bIsSlave, BOOL useFOC);

extern void vModPcm1ExtMasterMode();
extern void vModPcm1ExtSlaveModeASRC();
extern void vModPcm1ExtSlaveModeAsyncFIFO();
extern void vModPcm1IntSlaveModeAsyncFIFO();
extern void vModPcm2AsyncFIFO();
extern BOOL fgModPcmVbt16kMode();

extern BOOL fgMergeIfPcmOnly();
extern void vMergeIfI2sOnly(SAMPLINGRATE_T eSampleRate);
extern void vMergeIfPcmWithI2s(SAMPLINGRATE_T eSampleRate);
extern void vMergeIfI2sOnlyChangeFs();
extern void vMergeIfI2sFirstPcmLater();
extern void vMergeIfPcmFirstI2sLater();
extern void vMergeIfWithI2sIn(I2SSRC_T bIsSlave);

extern BOOL vAudioHWGain1Test();
extern BOOL vAudioHWGain2Test();
extern BOOL vAudioHWGain1_2CombineTest(SAMPLINGRATE_T eSampleRate);


#endif //MT6589_LDVT_AUDIO_TS_H

