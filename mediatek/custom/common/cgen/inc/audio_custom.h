#ifndef AUDIO_CUSTOM_H
#define AUDIO_CUSTOM_H

/* define Gain For Normal */
/* Normal volume: TON, SPK, MIC, FMR, SPH, SID, MED */
/*
#define GAIN_NOR_TON_VOL        8     // reserved
#define GAIN_NOR_KEY_VOL       43    // TTY_CTM_Mic
#define GAIN_NOR_MIC_VOL       26    // IN_CALL BuiltIn Mic gain
// GAIN_NOR_FMR_VOL is used as idle mode record volume
#define GAIN_NOR_FMR_VOL        0     // Normal BuiltIn Mic gain
#define GAIN_NOR_SPH_VOL       20     // IN_CALL EARPIECE Volume
#define GAIN_NOR_SID_VOL      100  // IN_CALL EARPICE sidetone
#define GAIN_NOR_MED_VOL       25   // reserved
*/

#define GAIN_NOR_TON_VOL        8     // reserved
#define GAIN_NOR_KEY_VOL       43    // TTY_CTM_Mic
#define GAIN_NOR_MIC_VOL       26    // IN_CALL BuiltIn Mic gain
// GAIN_NOR_FMR_VOL is used as idle mode record volume
#define GAIN_NOR_FMR_VOL        0     // Normal BuiltIn Mic gain
#define GAIN_NOR_SPH_VOL       20     // IN_CALL EARPIECE Volume
#define GAIN_NOR_SID_VOL      100  // IN_CALL EARPICE sidetone
#define GAIN_NOR_MED_VOL       25   // reserved

/* define Gain For Headset */
/* Headset volume: TON, SPK, MIC, FMR, SPH, SID, MED */
/*
#define GAIN_HED_TON_VOL        8     // reserved
#define GAIN_HED_KEY_VOL       24    // reserved
#define GAIN_HED_MIC_VOL       20    // IN_CALL BuiltIn headset gain
#define GAIN_HED_FMR_VOL       24     // reserved
#define GAIN_HED_SPH_VOL       12     // IN_CALL Headset volume
#define GAIN_HED_SID_VOL      100  // IN_CALL Headset sidetone
#define GAIN_HED_MED_VOL       12   // Idle, headset Audio Buf Gain setting
*/

#define GAIN_HED_TON_VOL        8     // reserved
#define GAIN_HED_KEY_VOL       24    // reserved
#define GAIN_HED_MIC_VOL       20    // IN_CALL BuiltIn headset gain
#define GAIN_HED_FMR_VOL       24     // reserved
#define GAIN_HED_SPH_VOL       12     // IN_CALL Headset volume
#define GAIN_HED_SID_VOL      100  // IN_CALL Headset sidetone
#define GAIN_HED_MED_VOL       12   // Idle, headset Audio Buf Gain setting

/* define Gain For Handfree */
/* Handfree volume: TON, SPK, MIC, FMR, SPH, SID, MED */
/* GAIN_HND_TON_VOL is used as class-D Amp gain*/
/*
#define GAIN_HND_TON_VOL        8     // use for ringtone volume
#define GAIN_HND_KEY_VOL       24    // reserved
#define GAIN_HND_MIC_VOL       20    // IN_CALL LoudSpeak Mic Gain = BuiltIn Gain
#define GAIN_HND_FMR_VOL       24     // reserved
#define GAIN_HND_SPH_VOL       12     // IN_CALL LoudSpeak
#define GAIN_HND_SID_VOL      100  // IN_CALL LoudSpeak sidetone
#define GAIN_HND_MED_VOL       12   // Idle, loudSPK Audio Buf Gain setting
*/

#define GAIN_HND_TON_VOL        8     // use for ringtone volume
#define GAIN_HND_KEY_VOL       24    // reserved
#define GAIN_HND_MIC_VOL       20    // IN_CALL LoudSpeak Mic Gain = BuiltIn Gain
#define GAIN_HND_FMR_VOL       24     // reserved
#define GAIN_HND_SPH_VOL       12     // IN_CALL LoudSpeak
#define GAIN_HND_SID_VOL      100  // IN_CALL LoudSpeak sidetone
#define GAIN_HND_MED_VOL       12   // Idle, loudSPK Audio Buf Gain setting
    /* 0: Input FIR coefficients for 2G/3G Normal mode */
    /* 1: Input FIR coefficients for 2G/3G/VoIP Headset mode */
    /* 2: Input FIR coefficients for 2G/3G Handfree mode */
    /* 3: Input FIR coefficients for 2G/3G/VoIP BT mode */
    /* 4: Input FIR coefficients for VoIP Normal mode */
    /* 5: Input FIR coefficients for VoIP Handfree mode */
//<2013/6/18-26039-jessicatseng, [5860] Modify acoustic parameter
//<2013/5/30-25525-jessicatseng, [5860] Modify acoustic parameter
//<2013/5/20-25117-jessicatseng, [5860] Modify acoustic parameter
//<2013/4/30-24478-alberthsiao, Update acoustic parameters
//<2013/4/25-24290-jessicatseng, [5860] Modify acoustic parameter
//<2013/4/24-24216-jessicatseng, [5860] Modify acoustic parameter
//<2013/4/19-24023-jessicatseng, [5860] Modify acoustic parameter
//<2013/4/17-23959-jessicatseng, [Pelican] Modify Acoustic parameter
//<2013/4/11-23734-jessicatseng, [Pelican] Modify acoustic parameter
#define SPEECH_INPUT_FIR_COEFF \
      -55,  1052,  -336,   855,  -448,\
     -201,   118, -1345,  1188,  -842,\
      131, -2546,  2890, -3089,  -188,\
     2222, -2821, -1326, -2805,  6309,\
   -17384, 32767, 32767,-17384,  6309,\
    -2805, -1326, -2821,  2222,  -188,\
    -3089,  2890, -2546,   131,  -842,\
     1188, -1345,   118,  -201,  -448,\
      855,  -336,  1052,   -55,     0,\
                                      \
    -1027,   194, -1031,   200,  -979,\
    -1588,  -430, -2253,   635, -5369,\
     1844, -3760,  1777,  -809, -2978,\
     4658, -7522, 12876,-13336, 20617,\
   -24979, 32767, 32767,-24979, 20617,\
   -13336, 12876, -7522,  4658, -2978,\
     -809,  1777, -3760,  1844, -5369,\
      635, -2253,  -430, -1588,  -979,\
      200, -1031,   194, -1027,     0,\
                                      \
     1055, -2177,  1031, -2220,   786,\
       55,   116,  1047, -1831,  1654,\
    -3352,  3991, -4933,  7923, -3397,\
     9003, -9023,  5647, -8321, 10061,\
   -12326, 32767, 32767,-12326, 10061,\
    -8321,  5647, -9023,  9003, -3397,\
     7923, -4933,  3991, -3352,  1654,\
    -1831,  1047,   116,    55,   786,\
    -2220,  1031, -2177,  1055,     0,\
                                      \
    32767,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
                                      \
    32767,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
                                      \
    32767,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0
    /* 0: Output FIR coefficients for 2G/3G Normal mode */
    /* 1: Output FIR coefficients for 2G/3G/VoIP Headset mode */
    /* 2: Output FIR coefficients for 2G/3G Handfree mode */
    /* 3: Output FIR coefficients for 2G/3G/VoIP BT mode */
    /* 4: Output FIR coefficients for VoIP Normal mode */
    /* 5: Output FIR coefficients for VoIP Handfree mode */
#define SPEECH_OUTPUT_FIR_COEFF \
      134,   568,    20,   422,    -2,\
      -27,    43,  -754,   443,  -548,\
      535, -2238,  -881,   -56, -1488,\
     1197, -2206, -3956, -3840, 12172,\
   -19934, 32767, 32767,-19934, 12172,\
    -3840, -3956, -2206,  1197, -1488,\
      -56,  -881, -2238,   535,  -548,\
      443,  -754,    43,   -27,    -2,\
      422,    20,   568,   134,     0,\
                                      \
     -449,   -82,   170,   772,   -95,\
    -1212,  1319,   228,  -369, -1874,\
     1925,  1361, -3691, -1204,  5960,\
     5324,-16076,  5213, 12469, -4004,\
   -32767, 30798, 30798,-32767, -4004,\
    12469,  5213,-16076,  5324,  5960,\
    -1204, -3691,  1361,  1925, -1874,\
     -369,   228,  1319, -1212,   -95,\
      772,   170,   -82,  -449,     0,\
                                      \
    32767,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
                                      \
    32767,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
                                      \
    32767,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
                                      \
    32767,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0
#define   DG_DL_Speech    0xe3d
#define   DG_Microphone    0x1400
#define   FM_Record_Vol    6     /* 0 is smallest. each step increase 1dB.
                            Be careful of distortion when increase too much.
                            Generally, it's not suggested to tune this parameter */
/*
* The Bluetooth DAI Hardware COnfiguration Parameter
*/
#define   DEFAULT_BLUETOOTH_SYNC_TYPE    0
#define   DEFAULT_BLUETOOTH_SYNC_LENGTH    1
    /* 0: Input FIR coefficients for 2G/3G Normal mode */
    /* 1: Input FIR coefficients for 2G/3G/VoIP Headset mode */
    /* 2: Input FIR coefficients for 2G/3G Handfree mode */
    /* 3: Input FIR coefficients for 2G/3G/VoIP BT mode */
    /* 4: Input FIR coefficients for VoIP Normal mode */
    /* 5: Input FIR coefficients for VoIP Handfree mode */
#define WB_Speech_Input_FIR_Coeff \
     -486,   -97,  -221,  -289,   -97,  -399,  -316,  -236,  -139,  -242,\
     -125,  -369,  -194,  -296,  -202,  -407,    60,  -120,   152,    91,\
     -524,   174,  -802,   154,  -372,   445,   463,   106,  -649,  -517,\
     -365,   850,  1644,  -588,   163,  -197,  2699, -2906,   846, -4166,\
     5995, -4174,  5469,-15633, 32767, 32767,-15633,  5469, -4174,  5995,\
    -4166,   846, -2906,  2699,  -197,   163,  -588,  1644,   850,  -365,\
     -517,  -649,   106,   463,   445,  -372,   154,  -802,   174,  -524,\
       91,   152,  -120,    60,  -407,  -202,  -296,  -194,  -369,  -125,\
     -242,  -139,  -236,  -316,  -399,   -97,  -289,  -221,   -97,  -486,\
                                       \
     -625,   202,  -136,  -411,    37,  -126,  -329,  -296,  -210,  -646,\
       56,   -67,   284,    76,   425,   -40,   863,  -221,  -519,   310,\
     -626,  1085,  -435,  1049,  1288,  1672,  -360,  1016,   846,  -213,\
      -93,   762,  2221,  -703,   333,  -613,  8592, -4453,  1509,   393,\
     2492, -3109,  8480,-19375, 32767, 32767,-19375,  8480, -3109,  2492,\
      393,  1509, -4453,  8592,  -613,   333,  -703,  2221,   762,   -93,\
     -213,   846,  1016,  -360,  1672,  1288,  1049,  -435,  1085,  -626,\
      310,  -519,  -221,   863,   -40,   425,    76,   284,   -67,    56,\
     -646,  -210,  -296,  -329,  -126,    37,  -411,  -136,   202,  -625,\
                                       \
     -407,  -272,  -627,  -581,  -826, -1153,  -963, -1176,  -781,  -890,\
     -395,  -249,  -171,    83,   132,    92,   477,  -152,   143,  -635,\
     -986,  -251, -1135,   506, -1291,   -74,  -341,   471,  1778,  1084,\
     2338,  1632,  1385,  1289,   813,   -97,  -142, -3242,  5969, -5413,\
     8089, -6683,  8271, -3029, 32767, 32767, -3029,  8271, -6683,  8089,\
    -5413,  5969, -3242,  -142,   -97,   813,  1289,  1385,  1632,  2338,\
     1084,  1778,   471,  -341,   -74, -1291,   506, -1135,  -251,  -986,\
     -635,   143,  -152,   477,    92,   132,    83,  -171,  -249,  -395,\
     -890,  -781, -1176,  -963, -1153,  -826,  -581,  -627,  -272,  -407,\
                                       \
    32767,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
                                       \
    32767,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
                                       \
    32767,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0
    /* 0: Output FIR coefficients for 2G/3G Normal mode */
    /* 1: Output FIR coefficients for 2G/3G/VoIP Headset mode */
    /* 2: Output FIR coefficients for 2G/3G Handfree mode */
    /* 3: Output FIR coefficients for 2G/3G/VoIP BT mode */
    /* 4: Output FIR coefficients for VoIP Normal mode */
    /* 5: Output FIR coefficients for VoIP Handfree mode */
#define WB_Speech_Output_FIR_Coeff \
       68,     0,   195,   203,  -120,    91,   102,    57,    78,    27,\
       67,   200,   212,  -198,   321,     2,  -120,   181,   -37,    44,\
     -190,   -61,   -18,  -448,  -429,   223,  -552,    41,  -173,  -218,\
      247,   667,  -420,  2678,  1207, -7761,   682, -1448,   621,  6003,\
      393, -2564,  1275,-24381, 32767, 32767,-24381,  1275, -2564,   393,\
     6003,   621, -1448,   682, -7761,  1207,  2678,  -420,   667,   247,\
     -218,  -173,    41,  -552,   223,  -429,  -448,   -18,   -61,  -190,\
       44,   -37,   181,  -120,     2,   321,  -198,   212,   200,    67,\
       27,    78,    57,   102,    91,  -120,   203,   195,     0,    68,\
                                       \
     -265,   273,   436,   305,    88,  -484,  -601,   -85,   479,   675,\
      648,  -260,  -575,  -892,  -258,   204,   942,   788,    32,  -915,\
    -1646,  -233,    65,  1525,  1578,   598, -1707, -1975, -1775,  1989,\
     3904,  3006,  1202, -5029, -5037, -4442,  3167,  6690,  6966, -5573,\
     -419,-15191, -5748,-11616, 32767, 32767,-11616, -5748,-15191,  -419,\
    -5573,  6966,  6690,  3167, -4442, -5037, -5029,  1202,  3006,  3904,\
     1989, -1775, -1975, -1707,   598,  1578,  1525,    65,  -233, -1646,\
     -915,    32,   788,   942,   204,  -258,  -892,  -575,  -260,   648,\
      675,   479,   -85,  -601,  -484,    88,   305,   436,   273,  -265,\
                                       \
      405,   237,    71,   245,  -117,    91,   -43,  -175,   -67,  -170,\
      -40,  -137,    67,  -151,   145,    97,   123,   243,   157,   130,\
      198,  -127,   -92,   103,  -624,   350,  -852,   121,  -467,  -485,\
     -249,  -367,   101,  -466,  1081,  -609,  1828, -1010,  2493, -2043,\
     3743, -4871,  6514,-11788, 32767, 32767,-11788,  6514, -4871,  3743,\
    -2043,  2493, -1010,  1828,  -609,  1081,  -466,   101,  -367,  -249,\
     -485,  -467,   121,  -852,   350,  -624,   103,   -92,  -127,   198,\
      130,   157,   243,   123,    97,   145,  -151,    67,  -137,   -40,\
     -170,   -67,  -175,   -43,    91,  -117,   245,    71,   237,   405,\
                                       \
    32767,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
                                       \
    32767,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
                                       \
    32767,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0,\
        0,     0,     0,     0,     0,     0,     0,     0,     0,     0
#endif
//>2013/4/11-23734-jessicatseng
//>2013/4/17-23959-jessicatseng
//>2013/4/19-24023-jessicatseng
//>2013/4/24-24216-jessicatseng
//>2013/4/25-24290-jessicatseng
//>2013/4/30-24478-alberthsiao
//>2013/5/20-25117-jessicatseng
//>2013/5/30-25525-jessicatseng
//>2013/6/18-26039-jessicatseng