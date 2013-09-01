/*******************************************************************************
 *
 * Filename:
 * ---------
 * audio_custom_exp.h
 *
 * Project:
 * --------
 *   ALPS
 *
 * Description:
 * ------------
 * This file is the header of audio customization related function or definition.
 *
 * Author:
 * -------
 * ChiPeng
 *
***********************************************************************************/

#ifndef AUDIO_CUSTOM_EXP_H
#define AUDIO_CUSTOM_EXP_H

#include "Audio_Customization_Common.h"

/*********************************************************************************
*Common definitations  are  defined  in below  file 
*alps\mediatek\custom\common\hal\audioflinger\Audio_Customization_Common.h.
*if  some of  common definitations are not need, the specific customer  can mark the definitation in
* Audio_Customization_Common.h or  can undefine  the definitations in this file,just like:
*#undef ENABLE_AUDIO_COMPENSATION_FILTER
***********************************************************************************/

#undef  ENABLE_AUDIO_COMPENSATION_FILTER
#undef  ENABLE_AUDIO_DRC_SPEAKER
#undef  ENABLE_HEADPHONE_COMPENSATION_FILTER
#undef  ENABLE_AUDIO_SW_STEREO_TO_MONO
#undef  ENABLE_HIGH_SAMPLERATE_RECORD

#endif
