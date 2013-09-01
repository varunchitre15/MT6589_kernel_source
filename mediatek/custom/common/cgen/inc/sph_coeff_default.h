
/*******************************************************************************
 *
 * Filename:
 * ---------
 *
 *
 * Project:
 * --------
 *   ALPS
 *
 * Description:
 * ------------
 *
 * Author:
 * -------
 * ChiPeng
 *
 *------------------------------------------------------------------------------
 * $Revision:$ 1.0.0
 * $Modtime:$
 * $Log:$
 *
 * 06 26 2010 chipeng.chang
 * [ALPS00002705][Need Patch] [Volunteer Patch] ALPS.10X.W10.11 Volunteer patch for speech parameter 
 * modify speech parameters.
 *
 * Mar 15 2010 mtk02308
 * [ALPS] Init Custom parameter
 *
 *

 *
 *
 *******************************************************************************/
#ifndef SPEECH_COEFF_DEFAULT_H
#define SPEECH_COEFF_DEFAULT_H

#ifndef FALSE
#define FALSE 0
#endif

//speech parameter depen on BT_CHIP cersion
#if defined(MTK_MT6611)

#define BT_COMP_FILTER (1 << 15)
#define BT_SYNC_DELAY  86

#elif defined(MTK_MT6612)

#define BT_COMP_FILTER (1 << 15)
#define BT_SYNC_DELAY  86

#elif defined(MTK_MT6616) || defined(MTK_MT6620) || defined(MTK_MT6622) || defined(MTK_MT6626) || defined(MTK_MT6628)

#define BT_COMP_FILTER (1 << 15)
#define BT_SYNC_DELAY  86

#else // MTK_MT6620

#define BT_COMP_FILTER (0 << 15)
#define BT_SYNC_DELAY  86

#endif

#ifdef MTK_DUAL_MIC_SUPPORT
#define SPEECH_MODE_PARA13 (371)
#define SPEECH_MODE_PARA14 (23)
#else
#define SPEECH_MODE_PARA13 (0)
#define SPEECH_MODE_PARA14 (0)
#endif

//<2013/6/24-26271-jessicatseng, [5860] Modify acoustic parameter
//<2013/5/30-25525-jessicatseng, [5860] Modify acoustic parameter
//<2013/5/21-25155-jessicatseng, [5860][ATS00159991] Modify acoustic parameter
//<2013/4/30-24478-alberthsiao, Update acoustic parameters
//<2013/4/25-24290-jessicatseng, [5860] Modify acoustic parameter
//<2013/4/24-24216-jessicatseng, [5860] Modify acoustic parameter
//<2013/4/19-24023-jessicatseng, [5860] Modify acoustic parameter
//<2013/4/17-23959-jessicatseng, [Pelican] Modify Acoustic parameter
//<2013/4/11-23734-jessicatseng, [Pelican] Modify acoustic parameter
#define DEFAULT_SPEECH_NORMAL_MODE_PARA \
    96,   253, 16388,    31, 57351,    31,   400,    49,\
    80,  4325,   611,     0, 20488,     0|SPEECH_MODE_PARA13,     0|SPEECH_MODE_PARA14,  8192

//<2013/3/26-23223-jessicatseng, Modify acoustic parameter
#define DEFAULT_SPEECH_EARPHONE_MODE_PARA \
    96,   221, 10756,    31, 57351,  8223,   400,    82,\
    80,  4325,   611,     0, 20488,     0,     0,     0
//>2013/3/26-23223-jessicatseng

#define DEFAULT_SPEECH_BT_EARPHONE_MODE_PARA \
     0,   253,  6212,    31, 53255,    31,   400,   246,\
    80,  4325,   611,     0, 53256|BT_COMP_FILTER,     0,     0,BT_SYNC_DELAY

#define DEFAULT_SPEECH_LOUDSPK_MODE_PARA \
   128,   224,  2218,    31, 57351, 24607,   400,   242,\
    84,  4325,   611,     0, 20488,     0,     0,     0

#define DEFAULT_SPEECH_CARKIT_MODE_PARA \
    96,   224,  5256,    31, 57351, 24607,   400,   132,\
    84,  4325,   611,     0, 20488,     0,     0,     0

#define DEFAULT_SPEECH_BT_CORDLESS_MODE_PARA \
     0,     0,     0,     0,     0,     0,     0,     0,\
     0,     0,     0,     0,     0,     0,     0,     0

#define DEFAULT_SPEECH_AUX1_MODE_PARA \
     0,     0,     0,     0,     0,     0,     0,     0,\
     0,     0,     0,     0,     0,     0,     0,     0

#define DEFAULT_SPEECH_AUX2_MODE_PARA \
     0,     0,     0,     0,     0,     0,     0,     0,\
     0,     0,     0,     0,     0,     0,     0,     0

#define DEFAULT_SPEECH_COMMON_PARA \
     6, 55997, 31000, 32767, 32769,     0,     0,     0, \
     0,     0,     0,     0

#define DEFAULT_SPEECH_VOL_PARA \
     0,     0,     0,     0

#define DEFAULT_AUDIO_DEBUG_INFO \
     0,     0,     0,     0,     0,     0,     0,     0, \
     0,     0,     0,     0,     0,     0,     0,     0

#define DEFAULT_VM_SUPPORT FALSE

#define DEFAULT_AUTO_VM FALSE

#define DEFAULT_WB_SPEECH_NORMAL_MODE_PARA \
    96,   253, 10756,    31, 57607,    31,   400,   114,\
    80,  4325,   611,     0, 16392,     0|SPEECH_MODE_PARA13,     0|SPEECH_MODE_PARA14,  8192

//<2013/3/26-23223-jessicatseng, Modify acoustic parameter
#define DEFAULT_WB_SPEECH_EARPHONE_MODE_PARA \
    96,   221, 16388,    31, 57607,    31,   400,    53,\
    80,  4325,   611,     0, 16392,     0,     0,     0
//>2013/3/26-23223-jessicatseng

#define DEFAULT_WB_SPEECH_BT_EARPHONE_MODE_PARA \
     0,   253,  6212,    31, 53511,    31,   400,   246,\
    80,  4325,   611,     0, 49160|BT_COMP_FILTER,     0,     0,BT_SYNC_DELAY

#define DEFAULT_WB_SPEECH_LOUDSPK_MODE_PARA \
   128,   224,  2218,    31, 57607, 24607,   400,   128,\
    84,  4325,   611,     0, 16392,     0,     0,     0

#define DEFAULT_WB_SPEECH_CARKIT_MODE_PARA \
    60, 65416,   152,    91, 65012,   174, 64734,   154,\
 65164,   445,   463,   106, 64887, 65019, 65171,   850

#define DEFAULT_WB_SPEECH_BT_CORDLESS_MODE_PARA \
 65050, 65439, 65315, 65247, 65439, 65137, 65220, 65300,\
 65397, 65294, 65411, 65167, 65342, 65240, 65334, 65129

#define DEFAULT_WB_SPEECH_AUX1_MODE_PARA \
  1644, 64948,   163, 65339,  2699, 62630,   846, 61370,\
  5995, 61362,  5469, 49903, 32767, 32767, 49903,  5469

#define DEFAULT_WB_SPEECH_AUX2_MODE_PARA \
 61362,  5995, 61370,   846, 62630,  2699, 65339,   163,\
 64948,  1644,   850, 65171, 65019, 64887,   106,   463

#define MICBAIS  1900

/* The Bluetooth PCM digital volume */
/* default_bt_pcm_in_vol : uplink, only for enlarge volume,
                       0x100 : 0dB  gain
                       0x200 : 6dB  gain
                       0x300 : 9dB  gain
                       0x400 : 12dB gain
                       0x800 : 18dB gain
                       0xF00 : 24dB gain             */

#define DEFAULT_BT_PCM_IN_VOL  0x100
/* default_bt_pcm_out_vol : downlink gain,
                       0x1000 : 0dB; maximum 0x7FFF  */
#define DEFAULT_BT_PCM_OUT_VOL  0x1000

#endif
//>2013/4/11-23734-jessicatseng
//>2013/4/17-23959-jessicatseng
//>2013/4/19-24023-jessicatseng
//>2013/4/24-24216-jessicatseng
//>2013/4/25-24290-jessicatseng
//>2013/4/30-24478-alberthsiao
//>2013/5/21-25155-jessicatseng
//>2013/5/30-25525-jessicatseng
//>2013/6/24-26271-jessicatseng
