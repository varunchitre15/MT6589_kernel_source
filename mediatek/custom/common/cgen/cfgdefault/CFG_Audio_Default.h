


/*******************************************************************************
 *
 * Filename:
 * ---------
 *   CFG_AUDIO_FILE.h
 *
 * Project:
 * --------
 *   DUMA
 *
 * Description:
 * ------------
 *    header file of AUDIO CFG Default file
 *
 * Author:
 * -------
 *
 *

 *
 *******************************************************************************/


#ifndef _CFG_AUDIO_H
#define _CFG_AUDIO_H

#include "CFG_Audio_Default_Cust.h"
#include "../cfgfileinc/CFG_AUDIO_File.h"
#include "../inc/sph_coeff_record_mode_default.h"
#include "../inc/sph_coeff_dmnr_default.h"
#include "../inc/audio_hd_record_custom.h"
#include "../inc/audio_acf_default.h"
#include "../inc/audio_hcf_default.h"
#include "../inc/audio_effect_default.h"
#include "../inc/audio_gain_table_default.h"
#include "../inc/audio_ver1_volume_custom_default.h"
#include "../inc/audio_hd_record_48k_custom.h"
#include "../inc/voice_recognition_custom.h"
#include "../inc/audio_audenh_control_option.h"


AUDIO_CUSTOM_PARAM_STRUCT speech_custom_default =
{
    /* INT8 volume[MAX_VOL_CATE][MAX_VOL_TYPE] */
    /* Normal volume: TON, SPK, MIC, FMR, SPH, SID, MED */
    GAIN_NOR_TON_VOL, GAIN_NOR_KEY_VOL, GAIN_NOR_MIC_VOL, GAIN_NOR_FMR_VOL, GAIN_NOR_SPH_VOL, GAIN_NOR_SID_VOL, GAIN_NOR_MED_VOL
    ,
    /* Headset volume: TON, SPK, MIC, FMR, SPH, SID, MED */
    GAIN_HED_TON_VOL, GAIN_HED_KEY_VOL, GAIN_HED_MIC_VOL, GAIN_HED_FMR_VOL, GAIN_HED_SPH_VOL, GAIN_HED_SID_VOL, GAIN_HED_MED_VOL
    ,
    /* Handfree volume: TON, SPK, MIC, FMR, SPH, SID, MED */
    GAIN_HND_TON_VOL, GAIN_HND_KEY_VOL, GAIN_HND_MIC_VOL, GAIN_HND_FMR_VOL, GAIN_HND_SPH_VOL, GAIN_HND_SID_VOL, GAIN_HND_MED_VOL
    ,
    /* UINT16 speech_common_para[12] */
    DEFAULT_SPEECH_COMMON_PARA
    ,
    /* UINT16 speech_mode_para[8][16] */
    DEFAULT_SPEECH_NORMAL_MODE_PARA,
    DEFAULT_SPEECH_EARPHONE_MODE_PARA,
    DEFAULT_SPEECH_LOUDSPK_MODE_PARA,
    DEFAULT_SPEECH_BT_EARPHONE_MODE_PARA,
    DEFAULT_SPEECH_BT_CORDLESS_MODE_PARA,
    DEFAULT_SPEECH_CARKIT_MODE_PARA,
    DEFAULT_SPEECH_AUX1_MODE_PARA,
    DEFAULT_SPEECH_AUX2_MODE_PARA
    ,
    /* UINT16 speech_volume_para[4] */
    DEFAULT_SPEECH_VOL_PARA
    ,
    /* UINT16 debug_info[16] */
    DEFAULT_AUDIO_DEBUG_INFO
    ,
    /* INT16 sph_in_fir[6][45], sph_out_fir */
    SPEECH_INPUT_FIR_COEFF,
    SPEECH_OUTPUT_FIR_COEFF
    ,
    /* UINT16 DG_DL_Speech */
    DG_DL_Speech
    ,
    /* UINT16 DG_Microphone */
    DG_Microphone
    ,
    /* UINT16 FM record volume*/
    FM_Record_Vol
    ,
    /* UINT16 BT sync type and length*/
    DEFAULT_BLUETOOTH_SYNC_TYPE,
    DEFAULT_BLUETOOTH_SYNC_LENGTH
    ,
    /* UINT16 BT PCM in/out digital gain*/
    DEFAULT_BT_PCM_IN_VOL,
    DEFAULT_BT_PCM_OUT_VOL
    ,
    /* user mode : normal mode, earphone mode, loud speaker mode */
    /* UCHAR  user_mode             */
    VOL_NORMAL
    ,
    /* auto VM record setting */
    DEFAULT_VM_SUPPORT,
    DEFAULT_AUTO_VM,
    /* Micbais voltage 1900 --> 2200 */
    MICBAIS,
};

AUDIO_VER1_CUSTOM_VOLUME_STRUCT audio_ver1_custom_default = {
    VER1_AUD_VOLUME_RING,
    VER1_AUD_VOLUME_SIP,
    VER1_AUD_VOLUME_MIC,
    VER1_AUD_VOLUME_FM,
    VER1_AUD_VOLUME_SPH,
    VER1_AUD_VOLUME_SPH, // sph2 now use the same
    VER1_AUD_VOLUME_SID,
    VER1_AUD_VOLUME_MEDIA,
    VER1_AUD_VOLUME_MATV,
    VER1_AUD_NORMAL_VOLUME_DEFAULT,
    VER1_AUD_HEADSER_VOLUME_DEFAULT,
    VER1_AUD_SPEAKER_VOLUME_DEFAULT,
    VER1_AUD_HEADSETSPEAKER_VOLUME_DEFAULT,
    VER1_AUD_EXTAMP_VOLUME_DEFAULT,
    VER1_AUD_VOLUME_LEVEL_DEFAULT
};


AUDIO_CUSTOM_WB_PARAM_STRUCT wb_speech_custom_default =
{
    /* unsigned short speech_mode_wb_para[8][16] */
    DEFAULT_WB_SPEECH_NORMAL_MODE_PARA,
    DEFAULT_WB_SPEECH_EARPHONE_MODE_PARA,
    DEFAULT_WB_SPEECH_LOUDSPK_MODE_PARA,
    DEFAULT_WB_SPEECH_BT_EARPHONE_MODE_PARA,
    DEFAULT_WB_SPEECH_BT_CORDLESS_MODE_PARA,
    DEFAULT_WB_SPEECH_CARKIT_MODE_PARA,
    DEFAULT_WB_SPEECH_AUX1_MODE_PARA,
    DEFAULT_WB_SPEECH_AUX2_MODE_PARA,
    /* short sph_wb_in_fir[6][90] */
    WB_Speech_Input_FIR_Coeff,
    /* short sph_wb_out_fir[6][90] */
    WB_Speech_Output_FIR_Coeff,
};
#if defined(MTK_AUDIO_BLOUD_CUSTOMPARAMETER_V4)
AUDIO_ACF_CUSTOM_PARAM_STRUCT audio_custom_default =
{
    BES_LOUDNESS_HSF_COEFF,
	BES_LOUDNESS_BPF_COEFF,
	BES_LOUDNESS_LPF_COEFF,
	BES_LOUDNESS_WS_GAIN_MAX,
	BES_LOUDNESS_WS_GAIN_MIN,
	BES_LOUDNESS_FILTER_FIRST,
	BES_LOUDNESS_ATT_TIME,
	BES_LOUDNESS_REL_TIME,
	BES_LOUDNESS_GAIN_MAP_IN,
	BES_LOUDNESS_GAIN_MAP_OUT,
};

AUDIO_ACF_CUSTOM_PARAM_STRUCT audio_hcf_custom_default =
{
    BES_LOUDNESS_HCF_HSF_COEFF,
	BES_LOUDNESS_HCF_BPF_COEFF,
	BES_LOUDNESS_HCF_LPF_COEFF,
	BES_LOUDNESS_HCF_WS_GAIN_MAX,
	BES_LOUDNESS_HCF_WS_GAIN_MIN,
	BES_LOUDNESS_HCF_FILTER_FIRST,
	BES_LOUDNESS_HCF_ATT_TIME,
	BES_LOUDNESS_HCF_REL_TIME,
	BES_LOUDNESS_HCF_GAIN_MAP_IN,
	BES_LOUDNESS_HCF_GAIN_MAP_OUT,
};
#else
AUDIO_ACF_CUSTOM_PARAM_STRUCT audio_custom_default =
{
    /* Compensation Filter HSF coeffs: default all pass filter       */
    /* BesLoudness also uses this coeffs    */
    BES_LOUDNESS_HSF_COEFF,
    /* Compensation Filter BPF coeffs: default all pass filter      */
    BES_LOUDNESS_BPF_COEFF,
    BES_LOUDNESS_DRC_FORGET_TABLE,
    BES_LOUDNESS_WS_GAIN_MAX,
    BES_LOUDNESS_WS_GAIN_MIN,
    BES_LOUDNESS_FILTER_FIRST,
    BES_LOUDNESS_GAIN_MAP_IN,
    BES_LOUDNESS_GAIN_MAP_OUT,
};

AUDIO_ACF_CUSTOM_PARAM_STRUCT audio_hcf_custom_default =
{
    /* Compensation Filter HSF coeffs: default all pass filter       */
    /* BesLoudness also uses this coeffs    */
    BES_LOUDNESS_HCF_HSF_COEFF,
    /* Compensation Filter BPF coeffs: default all pass filter      */
    BES_LOUDNESS_HCF_BPF_COEFF,
    BES_LOUDNESS_HCF_DRC_FORGET_TABLE,
    BES_LOUDNESS_HCF_WS_GAIN_MAX,
    BES_LOUDNESS_HCF_WS_GAIN_MIN,
    BES_LOUDNESS_HCF_FILTER_FIRST,
    BES_LOUDNESS_HCF_GAIN_MAP_IN,
    BES_LOUDNESS_HCF_GAIN_MAP_OUT,
};
#endif

AUDIO_EFFECT_CUSTOM_PARAM_STRUCT audio_effect_custom_default =
{
    // DSRD parameters
    BSRD_LEVEL,
    BSRD_DISTANCE1,
    BSRD_DISTANCE2,
    BSRD_BAND_SELECT,

    // BASS
    BASS_CUTOFF_FREQ,
    BASS_ISVB,

    //EQ effect
    NORMAL_GAIN_LEVEL,
    DANCE_GAIN_LEVEL,
    BASS_GAIN_LEVEL,
    CLASSICAL_GAIN_LEVEL,
    TREBLE_GAIN_LEVEL,
    PARTY_GAIN_LEVEL,
    POP_GAIN_LEVEL,
    ROCK_GAIN_LEVEL,

    //loudness mode
    LOUDENHANCEMODE,

    //TS
    TIME_TD_TF,
    TIME_TS_RATIO
};

AUDIO_PARAM_MED_STRUCT audio_param_med_default =
{
    SPEECH_INPUT_MED_FIR_COEFF,
    SPEECH_OUTPUT_MED_FIR_COEFF,
    FIR_output_index,
    FIR_input_index,
    MED_SPEECH_NORMAL_MODE_PARA,
    MED_SPEECH_EARPHONE_MODE_PARA,
    MED_SPEECH_BT_EARPHONE_MODE_PARA,
    MED_SPEECH_LOUDSPK_MODE_PARA,
    MED_SPEECH_CARKIT_MODE_PARA,
    MED_SPEECH_BT_CORDLESS_MODE_PARA,
    MED_SPEECH_AUX1_MODE_PARA,
    MED_SPEECH_AUX2_MODE_PARA
};


AUDIO_VOLUME_CUSTOM_STRUCT audio_volume_custom_default =
{
    AUD_VOLUME_RING,
    AUD_VOLUME_KEY,
    AUD_VOLUME_MIC,
    AUD_VOLUME_FMR,
    AUD_VOLUME_SPH,
    AUD_VOLUME_SID,
    AUD_VOLUME_MEDIA,
    AUD_VOLUME_MATV
};

AUDIO_CUSTOM_EXTRA_PARAM_STRUCT dual_mic_custom_default =
{
    DEFAULT_SPEECH_DUAL_MIC_ABF_PARA,
    DEFAULT_SPEECH_DUAL_MIC_ABFWB_PARA
};

AUDIO_GAIN_TABLE_STRUCT Gain_control_table_default ={
    DEFAULT_VOICE_GAIN_TABLE_PARA,
    DEFAULT_SYSTEM_GAIN_TABLE_PARA,
    DEFAULT_RINGTONE_GAIN_TABLE_PARA,
    DEFAULT_MUSIC_GAIN_TABLE_PARA,
    DEFAULT_ALARM_GAIN_TABLE_PARA,
    DEFAULT_NOTIFICATION_GAIN_TABLE_PARA,
    DEFAULT_BLUETOOTH_SCO_GAIN_TABLE_PARA,
    DEFAULT_ENFORCEAUDIBLE_GAIN_TABLE_PARA,
    DEFAULT_DTMF_GAIN_TABLE_PARA,
    DEFAULT_TTS_GAIN_TABLE_PARA,
    DEFAULT_FM_GAIN_TABLE_PARA,
    DEFAULT_MICROPHONE_GAIN_TABLE_PARA,
    DEFAULT_SIDETONE_GAIN_TABLE_PARA,
    DEFAULT_SPEECH_GAIN_TABLE_PARA
};

AUDIO_HD_RECORD_PARAM_STRUCT Hd_Recrod_Par_default = {
    //hd_rec mode num & fir num
    HD_REC_MODE_INDEX_NUM,
    HD_REC_FIR_INDEX_NUM,
    //hd_rec_speech_mode_para
    DEFAULT_HD_RECORD_SPH_MODE_PAR,
    //hd_rec_fir
    DEFAULT_HD_RECORD_FIR_Coeff,
    //hd_rec fir mapping - ch1
    DEFAULT_HD_RECORD_MODE_FIR_MAPPING_CH1,
    //hd_rec fir mapping - ch2
    DEFAULT_HD_RECORD_MODE_FIR_MAPPING_CH2,
    //hd_rec device mode mapping
    DEFAULT_HD_RECORD_MODE_DEV_MAPPING,
    //hd_rec_map_to_input_src
    DEFAULT_HD_RECORD_MODE_INPUT_SRC_MAPPING,
    //hd_rec_map_to_stereo_flag
    DEFAULT_HD_RECORD_MODE_STEREO_FLAGS
};

AUDIO_HD_RECORD_SCENE_TABLE_STRUCT Hd_Recrod_Scene_Table_default = {
	DEFAULT_HD_RECORD_NUM_VOICE_RECOGNITION_SCENES,
    DEFAULT_HD_RECORD_NUM_VOICE_SCENES,
    DEFAULT_HD_RECORD_NUM_VIDEO_SCENES,
    DEFAULT_HD_RECORD_NUM_VOICE_UNLOCK_SCENES,
    DEFAULT_HD_RECORD_NUM_CUSTOMIZATION_SCENES,
    DEFAULT_HD_RECORD_SCENE_TABLE,
    DEFAULT_HD_RECORD_SCENE_NAME
};

AUDIO_HD_RECORD_48K_PARAM_STRUCT Hd_Recrod_48k_Par_default = {    
    //hd_rec_48k_fir
    DEFAULT_HD_RECORD_48K_COMPEN_FIR_Coeff
};

VOICE_RECOGNITION_PARAM_STRUCT Voice_Recognize_Par_default = {
	//for framework, voice ui related
	DEFAULT_AP_NUM,
	DEFAULT_LANGUAGE_NUM,
	DEFAULT_LANGUAGE_FOLDER_NAME,
	DEFAULT_COMMAND_NUM_PER_LAN,
	DEFAULT_AP_SUPPORT_INFO,
	DEFAULT_ALGORITHM_PARAM // for CTO
};

AUDIO_AUDENH_CONTROL_OPTION_STRUCT AUDENH_Control_Option_Par_default = {    
    DEFAULT_AUDIO_AUDENH_CONTROL_OPTION_Coeff
};

#endif


