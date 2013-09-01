

/*******************************************************************************
 *
 * Filename:
 * ---------
 * cfg_audio_file.h
 *
 * Project:
 * --------
 *   DUMA
 *
 * Description:
 * ------------
 * This file is the header of audio customization related function or definition.
 *
 * Author:
 * -------
 * Ning.F
 *
 *============================================================================
 *             HISTORY
 * Below this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * $Revision:$
 * $Modtime:$
 * $Log:$
 *
 * 12 29 2012 donglei.ji
 * [ALPS00425279] [Need Patch] [Volunteer Patch] voice ui and password unlock feature check in
 * voice ui - NVRAM .
 *
 * 08 26 2012 weiguo.li
 * [ALPS00347285] [Need Patch] [Volunteer Patch]LGE AudioGainTable modification
 * .
 *
 * 07 29 2012 weiguo.li
 * [ALPS00319405] ALPS.JB.BSP.PRA check in CR for Jades
 * .
 *
 * Jun 22 2009 mtk01352
 * [DUMA00007771] Moving modem side customization to AP
 *
 *
 * Apr 29 2009 mtk80306
 * [DUMA00116080] revise the customization of nvram
 * revise nvram customization
 *
 * Mar 21 2009 mtk80306
 * [DUMA00112158] fix the code convention.
 * modify code convention
 *
 * Mar 9 2009 mtk80306
 * [DUMA00111088] nvram customization
 * nvram customization
 *
 *
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
#ifndef _CFG_AUDIO_FILE_H
#define _CFG_AUDIO_FILE_H

#define CUSTOM_VOLUME_STEP (7)
#define AUDIO_MAX_VOLUME_STEP (15)

#define NB_FIR_NUM (45)
#define NB_FIR_INDEX_NUM   (6)
#define SPEECH_COMMON_NUM (12)
#define SPEECH_PARA_MODE_NUM     (8)
#define SPEECH_PARA_NUM                (16)
#define AUDIO_EQ_PARAM_NUM         (8)

#define WB_FIR_NUM (90)
#define WB_FIR_INDEX_NUM   (6)

#define HD_REC_MODE_INDEX_NUM   (30)
#define HD_REC_FIR_INDEX_NUM    (16)	//extend 8-->16

// for voice ui feature
#define VOICE_FOLDER_NAME_LEN_MAX 32
#define VOICE_AP_NUM_MAX 16
#define VOICE_LANGUAGE_NUM_MAX 8
#define VOICE_CMDS_NUM_MAX 8
#define VOICE_CMDS_PER_LAN_NUM_MAX 32
#define VOICE_RECOG_FEATURE_NUM_MAX 8
#define VOICE_RECOG_PARAM_NUM_MAX 32
/* audio nvram structure definition*/
typedef enum
{
    VOL_NORMAL   = 0 ,
    VOL_HEADSET      ,
    VOL_HANDFREE     ,
    MAX_VOL_CATE
} volume_category_enum;

typedef enum
{
    VOL_TYPE_TON  = 0 ,
    VOL_TYPE_KEY      ,
    VOL_TYPE_MIC      ,
    VOL_TYPE_FMR      ,
    VOL_TYPE_SPH      ,
    VOL_TYPE_SID	    ,
    VOL_TYPE_MEDIA    ,
    MAX_VOL_TYPE
} volume_type_enum;

// new volume customization data structure
enum VolumeType {
    VER1_VOL_TYPE_RING = 0,
    VER1_VOL_TYPE_SIP,
    VER1_VOL_TYPE_MIC,
    VER1_VOL_TYPE_FM,
    VER1_VOL_TYPE_SPH,
    VER1_VOL_TYPE_SPH2,
    VER1_VOL_TYPE_SID,
    VER1_VOL_TYPE_MEDIA,
    VER1_VOL_TYPE_MATV,
    VER1_NUM_OF_VOL_TYPE
};

enum VolumeMode {
    VOLUME_NORMAL_MODE = 0,
    VOLUME_HEADSET_MODE,
    VOLUME_SPEAKER_MODE,
    VOLUME_HEADSET_SPEAKER_MODE,
    NUM_OF_VOL_MODE
};

enum VOLUME_NORMAL_TYPE {
    NORMAL_AUDIO_BUFFER = 0,
    NORMAL_FM_RECORD_A,
    NORMAL_FM_RECORD_D,
    NORMAL_SIP_AUDIO_BUFFER,
    NORMAL_RSERVED_2,
    NORMAL_RSERVED_3,
    NORMAL_VOLUME_TYPE_MAX
};

enum VOLUME_HEADSET_TYPE {
    HEADSET_AUDIO_BUFFER = 0,
    HEADSET_FM_RECORD_A,
    HEADSET_FM_RECORD_D,
    HEADSET_SIP_AUDIO_BUFFER,
    HEADSET_RSERVED_2,
    HEADSET_RSERVED_3,
    HEADSET_VOLUME_TYPE_MAX
};

enum VOLUME_SPEAKER_TYPE {
    SPEAKER_AMP = 0,
    SPEAKER_FM_RECORD_A,
    SPEAKER_FM_RECORD_D,
    SPEAKER_SIP_AUDIO_BUFFER,
    SPEAKER_RSERVED_2,
    SPEAKER_RSERVED_3,
    SPEAKER_VOLUME_TYPE_MAX
};

enum VOLUME_HEADSET_SPEAKER_TYPE {
    HEADSET_SPEAKER_AUDIO_BUFFER = 0,
    HEADSET_SPEAKER_AMP,
    HEADSET_SPEAKER_IV_BUFFER,
    HEADSET_SPEAKER_FM_RECORD_A,
    HEADSET_SPEAKER_FM_RECORD_D,
    HEADSET_SPEAKER_SIP_AUDIO_BUFFER,
    HEADSET_SPEAKER_RSERVED_2,
    HEADSET_SPEAKER_RSERVED_3,
    HEADSET_SPEAKER_VOLUME_TYPE_MAX
};

// use for external amp
enum VOLUME_EXTAMP_TYPE {
    VOLUME_IV_BUFFER_EXTAMP = 0,
    VOLUME_AUDIO_BUFFER_EXTAMP,
    VOLUME_EXTAMP, // control exp amp gain
    VOLUME_EXTAMP_RSERVED_1,
    VOLUME_EXTAMP_RSERVED_2,
    VOLUME_EXTAMP_RSERVED_3,
    EXTAMP_VOLUME_TYPE_MAX
};

#define     NUM_ABF_PARAM 44
#define     NUM_ABFWB_PARAM 76

typedef struct _AUDIO_CUSTOM_EXTRA_PARAM_STRUCT
{
    /* ABF parameters */
    unsigned short ABF_para[NUM_ABF_PARAM + NUM_ABFWB_PARAM];    //with WB
} AUDIO_CUSTOM_EXTRA_PARAM_STRUCT;

#define CFG_FILE_SPEECH_DUAL_MIC_SIZE    sizeof(AUDIO_CUSTOM_EXTRA_PARAM_STRUCT)
#define CFG_FILE_SPEECH_DUAL_MIC_TOTAL   1

typedef struct _AUDIO_CUSTOM_PARAM_STRUCT
{
    /* volume setting */
    unsigned char volume[MAX_VOL_CATE][MAX_VOL_TYPE];
    /* speech enhancement */
    unsigned short speech_common_para[SPEECH_COMMON_NUM];
    unsigned short speech_mode_para[SPEECH_PARA_MODE_NUM][SPEECH_PARA_NUM];
    unsigned short speech_volume_para[4];//in the feature, should extend to [MAX_VOL_CATE][MAX_VOL_TYPE][4]
    /* debug info */
    unsigned short debug_info[16];
    /* speech input FIR */
    short          sph_in_fir[NB_FIR_INDEX_NUM][NB_FIR_NUM];
    /* speech output FIR */
    short          sph_out_fir[NB_FIR_INDEX_NUM][NB_FIR_NUM];
    /* digital gain of DL speech */
    unsigned short Digi_DL_Speech;
    /* digital gain of uplink speech */
    unsigned short Digi_Microphone;
    /* FM record volume*/
    unsigned short FM_Record_Volume;
    /* user mode : normal mode, earphone mode, loud speaker mode */
    unsigned short Bluetooth_Sync_Type;
    unsigned short Bluetooth_Sync_Length;
    unsigned short bt_pcm_in_vol;
    unsigned short bt_pcm_out_vol;
    unsigned short user_mode;
    /* auto VM record setting */
    unsigned short uSupportVM;
    unsigned short uAutoVM;
    // mic bias
    unsigned short uMicbiasVolt;

} AUDIO_CUSTOM_PARAM_STRUCT;

#define CFG_FILE_SPEECH_REC_SIZE        sizeof(AUDIO_CUSTOM_PARAM_STRUCT)
#define CFG_FILE_SPEECH_REC_TOTAL   1

typedef struct _AUDIO_CUSTOM_WB_A2M_PARAM_STRUCT_
{
    /* WB speech enhancement */
    unsigned short speech_mode_wb_para[SPEECH_PARA_MODE_NUM][SPEECH_PARA_NUM];
    /* WB speech input/output FIR */
    short          sph_wb_fir[WB_FIR_INDEX_NUM][WB_FIR_NUM];
    /* in_out flag */
    short          input_out_fir_flag; // 0: input, 1: output
} AUDIO_CUSTOM_WB_A2M_PARAM_STRUCT;

typedef struct _AUDIO_CUSTOM_WB_PARAM_STRUCT
{
    /* WB speech enhancement */
    unsigned short speech_mode_wb_para[SPEECH_PARA_MODE_NUM][SPEECH_PARA_NUM];
    /* WB speech input FIR */
    short          sph_wb_in_fir[WB_FIR_INDEX_NUM][WB_FIR_NUM];
    /* WB speech output FIR */
    short          sph_wb_out_fir[WB_FIR_INDEX_NUM][WB_FIR_NUM];
} AUDIO_CUSTOM_WB_PARAM_STRUCT;

#define CFG_FILE_WB_SPEECH_REC_SIZE        sizeof(AUDIO_CUSTOM_WB_PARAM_STRUCT)
#define CFG_FILE_WB_SPEECH_REC_TOTAL   1

#if defined(MTK_AUDIO_BLOUD_CUSTOMPARAMETER_V4)
typedef struct _AUDIO_ACF_CUSTOM_PARAM_STRUCT
{
    unsigned int bes_loudness_hsf_coeff[2][9][5];     // Compensation Filter HSF coeffs	[9][4]->[2][9][5]
    unsigned int bes_loudness_bpf_coeff[8][6][3];  // Compensation Filter BPF coeffs	[4][6][3]->[6][6][3]->[8][6][3]
    //unsigned int bes_loudness_DRC_Forget_Table[9][2];
    unsigned int bes_loudness_lpf_coeff[6][3];
    unsigned int bes_loudness_WS_Gain_Max;
    unsigned int bes_loudness_WS_Gain_Min;
    unsigned int bes_loudness_Filter_First;
	unsigned int bes_loudness_Att_Time;		// unit: 0.1 ms / 6dB
	unsigned int bes_loudness_Rel_Time;		// unit: 0.1 ms / 6dB
    char bes_loudness_Gain_Map_In[5];
    char bes_loudness_Gain_Map_Out[5];
} AUDIO_ACF_CUSTOM_PARAM_STRUCT;
#else
typedef struct _AUDIO_ACF_CUSTOM_PARAM_STRUCT
{
    /* Compensation Filter HSF coeffs       */
    /* BesLoudness also uses this coeffs    */
    unsigned int bes_loudness_hsf_coeff[9][4];

    /* Compensation Filter BPF coeffs       */
    unsigned int bes_loudness_bpf_coeff[4][6][3];
    unsigned int bes_loudness_DRC_Forget_Table[9][2];
    unsigned int bes_loudness_WS_Gain_Max;
    unsigned int bes_loudness_WS_Gain_Min;
    unsigned int bes_loudness_Filter_First;
    char bes_loudness_Gain_Map_In[5];
    char bes_loudness_Gain_Map_Out[5];

} AUDIO_ACF_CUSTOM_PARAM_STRUCT;
#endif
/*
*/
#define CFG_FILE_AUDIO_COMPFLT_REC_SIZE        sizeof(AUDIO_ACF_CUSTOM_PARAM_STRUCT)
#define CFG_FILE_AUDIO_COMPFLT_REC_TOTAL   1
#define CFG_FILE_HEADPHONE_COMPFLT_REC_TOTAL   1

typedef struct _AUDIO_EFFECT_CUSTOM_PARAM_STRUCT
{
    //surround parameters
    int bsrd_level;
    unsigned int Distance1;
    unsigned int Distance2;
    int bsrd_band_select;

    //bass parameters
    unsigned int bass_CutoffFreq;
    int bass_IsVB;

    //EQ parameters
    short Normal_Gain_dB_level[AUDIO_EQ_PARAM_NUM];
    short Dance_Gain_dB_level[AUDIO_EQ_PARAM_NUM];
    short Bass_Gain_dB_level[AUDIO_EQ_PARAM_NUM];
    short Classical_Gain_dB_level[AUDIO_EQ_PARAM_NUM];
    short Treble_Gain_dB_level[AUDIO_EQ_PARAM_NUM];
    short Party_Gain_dB_level[AUDIO_EQ_PARAM_NUM];
    short Pop_Gain_dB_level[AUDIO_EQ_PARAM_NUM];
    short Rock_Gain_dB_level[AUDIO_EQ_PARAM_NUM];

    //loudness mode
    int LoudEnhancemode;

    // time stretch
    int Time_TD_FD;
    int Time_TS_Ratio;

} AUDIO_EFFECT_CUSTOM_PARAM_STRUCT;

#define CFG_FILE_AUDIO_EFFECT_REC_SIZE        sizeof(AUDIO_EFFECT_CUSTOM_PARAM_STRUCT)
#define CFG_FILE_AUDIO_EFFECT_REC_TOTAL   1

typedef struct _AUDIO_PARAM_MED_STRUCT
{
    short speech_input_FIR_coeffs[SPEECH_PARA_MODE_NUM][NB_FIR_NUM];
    short speech_output_FIR_coeffs[SPEECH_PARA_MODE_NUM][NB_FIR_INDEX_NUM][NB_FIR_NUM];
    short select_FIR_output_index[SPEECH_PARA_MODE_NUM];
    short select_FIR_intput_index[SPEECH_PARA_MODE_NUM];
    short speech_mode_para[SPEECH_PARA_MODE_NUM][SPEECH_PARA_NUM];
} AUDIO_PARAM_MED_STRUCT;

#define CFG_FILE_AUDIO_PARAM_MED_REC_SIZE        sizeof(AUDIO_PARAM_MED_STRUCT)
#define CFG_FILE_AUDIO_PARAM_MED_REC_TOTAL   1


typedef struct _AUDIO_VOLUME_CUSTOM_STRUCT
{
    unsigned char audiovolume_ring[MAX_VOL_CATE][CUSTOM_VOLUME_STEP];
    unsigned char audiovolume_key[MAX_VOL_CATE][CUSTOM_VOLUME_STEP];
    unsigned char audiovolume_mic[MAX_VOL_CATE][CUSTOM_VOLUME_STEP];
    unsigned char audiovolume_fmr[MAX_VOL_CATE][CUSTOM_VOLUME_STEP];
    unsigned char audiovolume_sph[MAX_VOL_CATE][CUSTOM_VOLUME_STEP];
    unsigned char audiovolume_sid[MAX_VOL_CATE][CUSTOM_VOLUME_STEP];
    unsigned char audiovolume_media[MAX_VOL_CATE][CUSTOM_VOLUME_STEP];
    unsigned char audiovolume_matv[MAX_VOL_CATE][CUSTOM_VOLUME_STEP];
} AUDIO_VOLUME_CUSTOM_STRUCT;

#define CFG_FILE_AUDIO_VOLUME_CUSTOM_REC_SIZE        sizeof(AUDIO_VOLUME_CUSTOM_STRUCT)
#define CFG_FILE_AUDIO_VOLUME_CUSTOM_REC_TOTAL   1

typedef struct _AUDIO_VER1_CUSTOM_VOLUME_STRUCT {
    unsigned char audiovolume_ring[NUM_OF_VOL_MODE][AUDIO_MAX_VOLUME_STEP];
    unsigned char audiovolume_sip[NUM_OF_VOL_MODE][AUDIO_MAX_VOLUME_STEP];
    unsigned char audiovolume_mic[NUM_OF_VOL_MODE][AUDIO_MAX_VOLUME_STEP];
    unsigned char audiovolume_fm[NUM_OF_VOL_MODE][AUDIO_MAX_VOLUME_STEP];
    unsigned char audiovolume_sph[NUM_OF_VOL_MODE][AUDIO_MAX_VOLUME_STEP];
    unsigned char audiovolume_sph2[NUM_OF_VOL_MODE][AUDIO_MAX_VOLUME_STEP];
    unsigned char audiovolume_sid[NUM_OF_VOL_MODE][AUDIO_MAX_VOLUME_STEP];
    unsigned char audiovolume_media[NUM_OF_VOL_MODE][AUDIO_MAX_VOLUME_STEP];
    unsigned char audiovolume_matv[NUM_OF_VOL_MODE][AUDIO_MAX_VOLUME_STEP];

    unsigned char normalaudiovolume[NORMAL_VOLUME_TYPE_MAX];
    unsigned char headsetaudiovolume[HEADSET_VOLUME_TYPE_MAX];
    unsigned char speakeraudiovolume[SPEAKER_VOLUME_TYPE_MAX];
    unsigned char headsetspeakeraudiovolume[HEADSET_SPEAKER_VOLUME_TYPE_MAX];
    unsigned char extampaudiovolume[EXTAMP_VOLUME_TYPE_MAX];

    unsigned char audiovolume_level[VER1_NUM_OF_VOL_TYPE];
} AUDIO_VER1_CUSTOM_VOLUME_STRUCT;

#define CFG_FILE_AUDIO_VER1_VOLUME_CUSTOM_REC_SIZE        sizeof(AUDIO_VER1_CUSTOM_VOLUME_STRUCT)
#define CFG_FILE_AUDIO_VER1_VOLUME_CUSTOM_REC_TOTAL   1



/********************************************************************
*   Audio Gain Table
*********************************************************************/
#define GAIN_TABLE_LEVEL (20)

#define VOICE_GAIN_TABLE_LEVEL (6+1)
#define SYSTEM_GAIN_TABLE_LEVEL (7+1)
#define RING_GAIN_TABLE_LEVEL (7+1)
#define MUSIC_GAIN_TABLE_LEVEL (13+1)
#define ALARM_GAIN_TABLE_LEVEL (7+1)
#define NOTIFICATION_GAIN_TABLE_LEVEL (7+1)
#define BLUETOOTHSCO_GAIN_TABLE_LEVEL (15+1)
#define ENFORCEAUDIBLE_GAIN_TABLE_LEVEL (7+1)
#define DTMF_GAIN_TABLE_LEVEL (15+1)
#define TTS_GAIN_TABLE_LEVEL (15+1)
#define FM_GAIN_TABLE_LEVEL (13+1)
#define MATV_GAIN_TABLE_LEVEL (13+1)

//speech should equal to voice call level
#define SPEECH_GAIN_TABLE_LEVLE (VOICE_GAIN_TABLE_LEVEL)

enum AUDIO_GAIN_TYPE {
	AUDIO_GAIN_DEFAULT 		    =-1,
	AUDIO_GAIN_VOICE_CALL		= 0,
	AUDIO_GAIN_SYSTEM			= 1,
	AUDIO_GAIN_RING			    = 2,
	AUDIO_GAIN_MUSIC			= 3,
	AUDIO_GAIN_ALARM			= 4,
	AUDIO_GAIN_NOTIFICATION	    = 5,
	AUDIO_GAIN_BLUETOOTH_SCO	= 6,
	AUDIO_GAIN_ENFORCED_AUDIBLE = 7, // Sounds that cannot be muted by user and must be routed to speaker
	AUDIO_GAIN_DTMF			    = 8,
	AUDIO_GAIN_TTS 			    = 9,
	AUDIO_GAIN_FM				= 10,
	AUDIO_GAIN_MAX_STREAM       = 10, //max index of stream
	AUDIO_GAIN_MIC              = 11,
	AUDIO_GAIN_SIDETONE         = 12,
	AUDIO_GAIN_SPEECH           = 13,
    NUM_AUDIO_GAIN_TYPES
};

enum GAIN_OUTPUT_DEVICE
{
	GAIN_OUTPUT_RECEIVER = 0,
	GAIN_OUTPUT_HEADSET  = 1,
	GAIN_OUTPUT_SPEAKER  = 2,
	NUM_GAIN_OUTPUT_DEVICES
};

enum MICROPHONE_DEVICEGAIN{
    GAIN_IDLE_RECORD_MIC =0,
    GAIN_IDLE_RECORD_HEADSET ,
    GAIN_INCALL_RECEIVER,
    GAIN_INCALL_HEADSET,
    GAIN_INCALL_SPEAKER,
    GAIN_VOIP_RECEIVER,
    GAIN_VOIP_HEADSET,
    GAIN_VOIP_SPEAKER,
    GAIN_FM_RECORDING,
    GAIN_TTY_DEVICE,
    GAIN_VOICE_RECOGNITION,
    NUM_OF_MICGAINS
};

enum SIDETONE_DEVICEGAIN{
    GAIN_SIDETONE_RECEIVER =0,
    GAIN_SIDETONE_HEADSET,
    GAIN_SIDETONE_SPEAKER,
    NUM_OF_SIDETONEGAINS
};

enum GAIN_SPEEECH{
	SPEECH_RECEIVER = 0,
    SPEECH_HEADSET  = 1,
    SPEECH_SPEAKER  = 2,
    NUM_OF_SPEECHSGAINS
};

typedef struct _AUDIO_GAIN_CONTROL_STRUCT
{
    unsigned char u8Gain_digital;
    unsigned int u32Gain_PGA_Amp;
} AUDIO_GAIN_CONTROL_STRUCT;

typedef struct _STREAM_GAIN_CONTROL_STRUCT
{
    AUDIO_GAIN_CONTROL_STRUCT streamGain[NUM_GAIN_OUTPUT_DEVICES][GAIN_TABLE_LEVEL];
}STREAM_GAIN_CONTROL_STRUCT;

typedef struct _STREAMS_GAIN_CONTROL_STRUCT
{
    STREAM_GAIN_CONTROL_STRUCT voiceCall;
    STREAM_GAIN_CONTROL_STRUCT system;
    STREAM_GAIN_CONTROL_STRUCT ring;
    STREAM_GAIN_CONTROL_STRUCT music;
    STREAM_GAIN_CONTROL_STRUCT alarm;
    STREAM_GAIN_CONTROL_STRUCT notification;
    STREAM_GAIN_CONTROL_STRUCT blueToothSco;
    STREAM_GAIN_CONTROL_STRUCT enforceAudible;
    STREAM_GAIN_CONTROL_STRUCT dtmf;
    STREAM_GAIN_CONTROL_STRUCT tts;
    STREAM_GAIN_CONTROL_STRUCT fm;
}STREAMS_GAIN_CONTROL_STRUCT;

typedef struct _STREAM_MICROPHONE_GAIN_CONTROL_STRUCT
{
    unsigned char micGain[GAIN_TABLE_LEVEL];
} STREAM_MICROPHONE_GAIN_CONTROL_STRUCT;

typedef struct _STREAM_SIDETONE_GAIN_CONTROL_STRUCT
{
    unsigned char sidetoneGain[GAIN_TABLE_LEVEL];
} STREAM_SIDETONE_GAIN_CONTROL_STRUCT;

typedef struct _STREAM_SPEECH_GAIN_CONTROL_STRUCT
{
    AUDIO_GAIN_CONTROL_STRUCT speechGain[NUM_GAIN_OUTPUT_DEVICES][GAIN_TABLE_LEVEL];
} STREAM_SPEECH_GAIN_CONTROL_STRUCT;

typedef struct _AUDIO_GAIN_TABLE_STRUCT
{
    STREAM_GAIN_CONTROL_STRUCT voiceCall;
    STREAM_GAIN_CONTROL_STRUCT system;
    STREAM_GAIN_CONTROL_STRUCT ring;
    STREAM_GAIN_CONTROL_STRUCT music;
    STREAM_GAIN_CONTROL_STRUCT alarm;
    STREAM_GAIN_CONTROL_STRUCT notification;
    STREAM_GAIN_CONTROL_STRUCT blueToothSco;
    STREAM_GAIN_CONTROL_STRUCT enforceAudible;
    STREAM_GAIN_CONTROL_STRUCT dtmf;
    STREAM_GAIN_CONTROL_STRUCT tts;
    STREAM_GAIN_CONTROL_STRUCT fm;
    STREAM_MICROPHONE_GAIN_CONTROL_STRUCT microphoneGain;
    STREAM_SIDETONE_GAIN_CONTROL_STRUCT sidetoneGain;
    STREAM_SPEECH_GAIN_CONTROL_STRUCT speechGain;
} AUDIO_GAIN_TABLE_STRUCT;

#define CFG_FILE_AUDIO_GAIN_TABLE_CUSTOM_REC_SIZE        sizeof(AUDIO_GAIN_TABLE_STRUCT)
#define CFG_FILE_AUDIO_GAIN_TABLE_CUSTOM_REC_TOTAL   1


//#if defined(MTK_HD_RECORD_SUPPORT)

#define SPC_MAX_NUM_RECORD_SPH_MODE HD_REC_MODE_INDEX_NUM
#define SPC_MAX_NUM_RECORD_INPUT_FIR HD_REC_FIR_INDEX_NUM	//extend 8 -->16
typedef struct _AUDIO_HD_RECORD_PARAM_STRUCT
{
    /* HD RECORD Mode Num & FIR Num*/
    unsigned short hd_rec_mode_num; // max(hd_rec_fir_num) == 30 ??
    unsigned short hd_rec_fir_num;  // max(hd_rec_fir_num) == 16 ??

    /* HD RECORD Speech Enhancement */
    unsigned short hd_rec_speech_mode_para[SPC_MAX_NUM_RECORD_SPH_MODE][SPEECH_PARA_NUM]; // the contain only have hd_rec_mode_num effective values

    /* HD RECORD FIR */
    short          hd_rec_fir[SPC_MAX_NUM_RECORD_INPUT_FIR][WB_FIR_NUM]; // the contain only have hd_rec_fir_num effective values

    /* HD RECORD FIR Mapping (ex, map[sph_mode] = FIR3) */
    unsigned short hd_rec_map_to_fir_for_ch1[SPC_MAX_NUM_RECORD_SPH_MODE];
    unsigned short hd_rec_map_to_fir_for_ch2[SPC_MAX_NUM_RECORD_SPH_MODE];

    /* HD RECORD Device Mode Mapping (ex, map[sph_mode] = SPH_MODE_NORMAL ) */
    unsigned char hd_rec_map_to_dev_mode[SPC_MAX_NUM_RECORD_SPH_MODE];

    /* HD RECORD Input Source Mapping (ex, map[sph_mode] = BT Earphone mic)*/
    unsigned char hd_rec_map_to_input_src[SPC_MAX_NUM_RECORD_SPH_MODE];

    /* HD RECORD mode is stereo or not (ex, map[sph_mode] = 0(mono), 1(stereo) )*/
    unsigned char hd_rec_map_to_stereo_flag[SPC_MAX_NUM_RECORD_SPH_MODE];

} AUDIO_HD_RECORD_PARAM_STRUCT;


#define CFG_FILE_AUDIO_HD_REC_PAR_SIZE   sizeof(AUDIO_HD_RECORD_PARAM_STRUCT)
#define CFG_FILE_AUDIO_HD_REC_PAR_TOTAL  1



#define MAX_HD_REC_SCENES 10  // max #scene = 10 (10 * 3 = 30 = max modes)

enum HD_REC_DEVICE_SOURCE_T
{
    HD_REC_DEVICE_SOURCE_HANDSET     = 0,
    HD_REC_DEVICE_SOURCE_HEADSET     = 1,
    HD_REC_DEVICE_SOURCE_BT_EARPHONE = 2,
    NUM_HD_REC_DEVICE_SOURCE
};

typedef struct
{
	unsigned char num_voice_recognition_scenes;	//for voice recognition
    unsigned char num_voice_rec_scenes;
    unsigned char num_video_rec_scenes;
	unsigned char num_voice_unlock_scenes;	//for voice unlock feature
	unsigned char num_customization_scenes;	//for customization
    unsigned char scene_table[MAX_HD_REC_SCENES][NUM_HD_REC_DEVICE_SOURCE];
	unsigned char scene_name[MAX_HD_REC_SCENES][10];	//name of each scene
} AUDIO_HD_RECORD_SCENE_TABLE_STRUCT;


#define CFG_FILE_AUDIO_HD_REC_SCENE_TABLE_SIZE   sizeof(AUDIO_HD_RECORD_SCENE_TABLE_STRUCT)
#define CFG_FILE_AUDIO_HD_REC_SCENE_TABLE_TOTAL  1


#define SPC_MAX_NUM_48K_RECORD_INPUT_FIR 8
//for HD 48k sample rate record
typedef struct _AUDIO_HD_RECORD_48K_PARAM_STRUCT
{
	/* HD RECORD 48k FIR */
	short	hd_rec_fir[SPC_MAX_NUM_48K_RECORD_INPUT_FIR][WB_FIR_NUM]; // the contain only have hd_rec_fir_num effective values
} AUDIO_HD_RECORD_48K_PARAM_STRUCT;


#define CFG_FILE_AUDIO_HD_REC_48K_PAR_SIZE   sizeof(AUDIO_HD_RECORD_48K_PARAM_STRUCT)
#define CFG_FILE_AUDIO_HD_REC_48K_PAR_TOTAL  1


//#endif  //MTK_HD_RECORD_SUPPORT

//for voice recognition customization
typedef struct _VOICE_RECOGNITION_PARAM_STRUCT
{
	  /* HD RECORD 48k FIR */
	  unsigned char ap_num;
    unsigned char language_num;
    unsigned char language_folder[VOICE_LANGUAGE_NUM_MAX][VOICE_FOLDER_NAME_LEN_MAX];
    unsigned char cmd_num[VOICE_LANGUAGE_NUM_MAX];
    unsigned char ap_support_info[VOICE_AP_NUM_MAX][VOICE_LANGUAGE_NUM_MAX][VOICE_CMDS_NUM_MAX];
    unsigned char cust_param[VOICE_RECOG_FEATURE_NUM_MAX][VOICE_RECOG_PARAM_NUM_MAX];

} VOICE_RECOGNITION_PARAM_STRUCT;


#define CFG_FILE_VOICE_RECOGNIZE_PAR_SIZE   sizeof(VOICE_RECOGNITION_PARAM_STRUCT)
#define CFG_FILE_VOICE_RECOGNIZE_PAR_TOTAL  1


typedef struct
{
	unsigned int u32EnableFlg;

}AUDIO_AUDENH_CONTROL_OPTION_STRUCT;

#define CFG_FILE_AUDIO_AUDENH_CONTROL_OPTION_PAR_SIZE   sizeof(AUDIO_AUDENH_CONTROL_OPTION_STRUCT)
#define CFG_FILE_AUDIO_AUDENH_CONTROL_OPTION_PAR_TOTAL  1

#endif // _CFG_AUDIO_FILE_H

