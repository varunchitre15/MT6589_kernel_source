

/*******************************************************************************
 *
 * Filename:
 * ---------
 * audio_ver1_volume_custom_default.h
 *
 * Project:
 * --------
 *   ALPS
 *
 * Description:
 * ------------
 * This file is the header of audio customization related parameters or definition.
 *
 * Author:
 * -------
 * Chipeng chang
 *
 *============================================================================
 *             HISTORY
 * Below this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * $Revision:$
 * $Modtime:$
 * $Log:$
 *
 *
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
#ifndef AUDIO_VER1_VOLUME_CUSTOM_DEFAULT_H
#define AUDIO_VER1_VOLUME_CUSTOM_DEFAULT_H

//<2013/6/24-26171-jessicatseng, [5860] Modify acoustic parameter
//<2013/6/03-25612-jessicatseng, [5860] Update acoustic parameter
//<2013/5/30-25525-jessicatseng, [5860] Modify acoustic parameter
//<2013/5/21-25155-jessicatseng, [5860][ATS00159991] Modify acoustic parameter
//<2013/5/10-24780-jessicatseng, [5860] Modify acoustic parameter
//<2013/4/24-24216-jessicatseng, [5860] Modify acoustic parameter
//<2013/4/19-24023-jessicatseng, [5860] Modify acoustic parameter
//<2013/4/17-23959-jessicatseng, [Pelican] Modify Acoustic parameter
//<2013/4/11-23734-jessicatseng, [Pelican] Modify acoustic parameter
#define VER1_AUD_VOLUME_RING \
    112,136,160,184,208,232,255,0,0,0,0,0,0,0,0,\
    112,136,160,184,208,232,255,0,0,0,0,0,0,0,0,\
    80,110,130,150,170,210,255,0,0,0,0,0,0,0,0,\
    120,140,160,180,200,225,240,0,0,0,0,0,0,0,0

#define VER1_AUD_VOLUME_SIP \
    112,136,160,184,208,232,255,0,0,0,0,0,0,0,0,\
    112,136,160,184,208,232,255,0,0,0,0,0,0,0,0,\
    112,136,160,184,208,232,255,0,0,0,0,0,0,0,0,\
    0,43,85,128,171,213,255,0,0,0,0,0,0,0,0

//<2013/3/26-23223-jessicatseng, Modify acoustic parameter
#define VER1_AUD_VOLUME_MIC \
    64,112,192,152,192,192,184,184,184,184,184,0,0,0,0,\
    255,192,184,192,184,208,172,184,184,184,184,0,0,0,0,\
    255,208,208,192,255,208,196,0,0,0,0,0,0,0,0,\
    255,208,208,164,255,208,172,0,0,0,0,0,0,0,0
//>2013/3/26-23223-jessicatseng

#define VER1_AUD_VOLUME_FM \
    16,80,112,144,176,208,240,0,0,0,0,0,0,0,0,\
    100,120,135,150,164,200,220,0,0,0,0,0,0,0,0,\
    80,110,130,150,170,210,255,0,0,0,0,0,0,0,0,\
    112,136,160,184,208,232,255,0,0,0,0,0,0,0,0

//<2013/4/30-24478-alberthsiao, Update acoustic parameters
//<2013/3/26-23223-jessicatseng, Modify acoustic parameter
#define VER1_AUD_VOLUME_SPH \
    28,44,64,88,100,116,132,0,0,0,0,0,0,0,0,\
    28,48,68,84,100,112,132,0,0,0,0,0,0,0,0,\
    108,116,132,144,148,152,160,0,0,0,0,0,0,0,0,\
    40,52,64,76,88,100,112,0,0,0,0,0,0,0,0
//>2013/3/26-23223-jessicatseng
//>2013/4/30-24478-alberthsiao

#define VER1_AUD_VOLUME_SID \
    0,0,16,0,0,0,0,0,0,0,0,0,0,0,0,\
    0,0,32,0,0,0,0,0,0,0,0,0,0,0,0,\
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,\
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0

#define VER1_AUD_VOLUME_MEDIA \
    112,136,160,184,208,232,255,0,0,0,0,0,0,0,0,\
    100,120,135,150,164,200,220,0,0,0,0,0,0,0,0,\
    80,110,130,150,170,210,255,0,0,0,0,0,0,0,0,\
    104,128,152,176,200,224,255,0,0,0,0,0,0,0,0

#define VER1_AUD_VOLUME_MATV \
    0,32,64,92,128,160,192,0,0,0,0,0,0,0,0,\
    0,32,64,92,128,160,192,0,0,0,0,0,0,0,0,\
    112,136,160,184,208,232,255,0,0,0,0,0,0,0,0,\
    0,43,85,128,171,213,255,0,0,0,0,0,0,0,0

#define VER1_AUD_NORMAL_VOLUME_DEFAULT \
    128,128,128,128,128,128

#define VER1_AUD_HEADSER_VOLUME_DEFAULT \
    140,148,148,148,148,148

#define VER1_AUD_SPEAKER_VOLUME_DEFAULT \
    154,144,144,144,144,144

#define VER1_AUD_HEADSETSPEAKER_VOLUME_DEFAULT \
    112,154,132,132,132,132,132,132

#define VER1_AUD_EXTAMP_VOLUME_DEFAULT \
    132,132,132,132,132,132

#define VER1_AUD_VOLUME_LEVEL_DEFAULT \
    7,7,7,7,7,7,7,7,7

#endif
//>2013/4/11-23734-jessicatseng
//>2013/4/17-23959-jessicatseng
//>2013/4/19-24023-jessicatseng
//>2013/4/24-24216-jessicatseng
//>2013/5/10-24780-jessicatseng
//>2013/5/21-25155-jessicatseng
//>2013/5/30-25525-jessicatseng
//>2013/6/03-25612-jessicatseng
//>2013/6/24-26271-jessicatseng
