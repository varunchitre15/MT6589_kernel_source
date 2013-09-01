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
 *   AudioAfe.h
 *
 * Project:
 * --------
 *   MT6589 FPGA LDVT Audio Driver
 *
 * Description:
 * ------------
 *   Audio register
 *
 * Author:
 * -------
 *   Ir Lian (mtk00976)
 *   Harvey Huang (mtk03996)
 *
 *------------------------------------------------------------------------------
 * $Revision: #1 $
 * $Modtime:$
 * $Log:$
 *
 *
 *******************************************************************************/

#ifndef _AUDIO_AFE_H_
#define _AUDIO_AFE_H_

/*****************************************************************************
 *                     C O M P I L E R   F L A G S
 *****************************************************************************/


/*****************************************************************************
 *                E X T E R N A L   R E F E R E N C E S
 *****************************************************************************/

#include "AudioCommon.h"

/*****************************************************************************
 *                         D A T A   T Y P E S
 *****************************************************************************/

typedef enum
{
    AFE_MEM_NONE    = 0,
    AFE_MEM_DL1     = 1,
    AFE_MEM_DL2     = 2,
    AFE_MEM_VUL     = 3,
    AFE_MEM_DAI     = 4,
    AFE_MEM_I2S     = 5,
    AFE_MEM_AWB     = 6,
    AFE_MEM_MOD_PCM = 7,
} MEM_INTERFACE_T;

typedef enum
{
    AFE_8000HZ  = 0,
    AFE_11025HZ = 1,
    AFE_12000HZ = 2,
    AFE_16000HZ = 3,
    AFE_22050HZ = 4,
    AFE_24000HZ = 5,
    AFE_32000HZ = 6,
    AFE_44100HZ = 7,
    AFE_48000HZ = 8
}  SAMPLINGRATE_T;

typedef enum
{
    AFE_DAIMOD_8000HZ  = 0x0,
    AFE_DAIMOD_16000HZ = 0x1,
} DAIMOD_SAMPLINGRATE_T;

typedef enum
{
    AFE_STEREO = 0x0,
    AFE_MONO   = 0x1
} MEMIF_CH_CFG_T;

typedef enum
{
    AFE_MONO_USE_L = 0x0,
    AFE_MONO_USE_R = 0x1
} MEMIF_MONO_SEL_T;

typedef enum
{
    AFE_DUP_WR_DISABLE = 0x0,
    AFE_DUP_WR_ENABLE  = 0x1
} MEMIF_DUP_WRITE_T;

typedef struct
{
    UINT32 u4AFE_MEMIF_BUF_BASE;
    UINT32 u4AFE_MEMIF_BUF_END;
    UINT32 u4AFE_MEMIF_BUF_WP;
    UINT32 u4AFE_MEMIF_BUF_RP;
} MEMIF_BUF_T;

typedef struct
{
    MEM_INTERFACE_T        eMemInterface;
    SAMPLINGRATE_T         eSamplingRate;
    DAIMOD_SAMPLINGRATE_T  eDaiModSamplingRate;
    MEMIF_CH_CFG_T         eChannelConfig;
    MEMIF_MONO_SEL_T       eMonoSelect;            //Used when AWB and VUL and data is mono
    MEMIF_DUP_WRITE_T      eDupWrite;              //Used when MODPCM and DAI
    MEMIF_BUF_T            rBufferSetting;
} MEMIF_CONFIG_T;

// I2S related
typedef enum
{
    I2S_EIAJ = 0x0,
    I2S_I2S  = 0x1
} I2SFMT_T;

typedef enum
{
    I2S_16BIT = 0x0,
    I2S_32BIT = 0x1
} I2SWLEN_T;

typedef enum
{
    I2S_NOSWAP = 0x0,
    I2S_LRSWAP = 0x1
} I2SSWAP_T;

typedef enum
{
    I2S_DISABLE = 0x0,
    I2S_ENABLE  = 0x1
} I2SEN_T;

typedef enum
{
    I2S_MASTER = 0x0,
    I2S_SLAVE  = 0x1
} I2SSRC_T;

typedef enum
{
    I2S_OUT = 0x0,
    I2S_IN  = 0x1
} I2SDIR_T;


// PCM related
typedef enum
{
    PCM_1 = 0x0,    // (O7, O8, I9)
    PCM_2 = 0x1     // (O17, O18, I14)
} PCM_MODULE;

typedef enum
{
    PCM_DISABLE = 0x0,
    PCM_ENABLE  = 0x1
} PCMEN_T;

typedef enum
{
    PCM_I2S   = 0x0,
    PCM_EIAJ  = 0x1,
    PCM_MODEA = 0x2,
    PCM_MODEB = 0x3
} PCMFMT_T;

typedef enum
{
    PCM_8K  = 0x0,
    PCM_16K = 0x1
} PCMMODE_T;

typedef enum
{
    PCM_16BIT = 0x0,
    PCM_32BIT = 0x1
} PCMWLEN_T;


typedef enum
{
    PCM_MASTER = 0x0,
    PCM_SLAVE = 0x1
} PCMCLKSRC_T;


typedef enum
{
    PCM_GO_ASRC  = 0x0,         // (ASRC)       Set to 0 when source & destination uses different crystal
    PCM_GO_ASYNC_FIFO = 0x1     // (Async FIFO) Set to 1 when source & destination uses same crystal
} PCMBYPASRC_T;


typedef enum
{
    PCM_DMTX = 0x0, // dual mic on TX
    PCM_SMTX = 0x1  // single mic on TX (In BT mode, only L channel data is sent on PCM TX.)
} PCMBTMODE_T;


typedef enum
{
    PCM_SYNC_LEN_1_BCK = 0x0,
    PCM_SYNC_LEN_N_BCK = 0x1
} PCMSYNCTYPE_T;

typedef UINT8 PCMSYNCLEN_T;


typedef enum
{
    PCM_INT_MD = 0x0,
    PCM_EXT_MD = 0x1
} PCMEXTMODEM_T;


typedef enum
{
    PCM_VBT_16K_MODE_DISABLE = 0x0,
    PCM_VBT_16K_MODE_ENABLE = 0x1
} PCMVBT16KMODE_T;


typedef enum
{
    PCM_NOINV = 0x0,
    PCM_INV   = 0x1
} PCMCLKINV_T;

typedef enum
{
    PCM_LB_DISABLE = 0x0,
    PCM_LB_ENABLE  = 0x1
} PCMLOOPENA_T;

typedef enum
{
    PCM_TXFIX_OFF = 0x0,
    PCM_TXFIX_ON  = 0x1
} PCMTXFIXEN_T;

typedef UINT16 PCMFIXVAL_T;



typedef struct
{
    PCMFMT_T        ePcmFmt;
    PCMMODE_T       ePcm8k16kmode;
    PCMWLEN_T       ePcmWlen;
    PCMCLKSRC_T     ePcmClkSrc;
    PCMBYPASRC_T    ePcmBypassASRC;
    PCMEXTMODEM_T   ePcmModemSel;
    PCMVBT16KMODE_T ePcmVbt16kSel;
} PCM_INFO_T;


// BT PCM
typedef enum
{
    BTPCM_DISABLE = 0x0,
    BTPCM_ENABLE  = 0x1
} BTPCMEN_T;

typedef enum
{
    BTPCM_8K  = 0x0,
    BTPCM_16K = 0x1
} BTPCMMODE_T;



// Interconnection related
typedef enum
{
    I00 = 0,
    I01 = 1,
    I02 = 2,
    I03 = 3,
    I04 = 4,
    I05 = 5,
    I06 = 6,
    I07 = 7,
    I08 = 8,
    I09 = 9,
    IN_MAX
} ITRCON_IN_T;

typedef enum
{
    O00  = 0,
    O01  = 1,
    O02  = 2,
    O03  = 3,
    O04  = 4,
    O05  = 5,
    O06  = 6,
    O07  = 7,
    O08  = 8,
    O09  = 9,
    O010 = 10,
    O011 = 11,
    O012 = 12,
    OUT_MAX
} ITRCON_OUT_T;

// IRQ related
typedef enum
{
    IRQ1 = 1,
    IRQ2 = 2,
    IRQ5 = 3, //HDMI
    IRQ6 = 4, //SPDIF
    IRQMAX
} IRQTYPE_T;


// Side tone filter related
typedef enum
{
    I3I4    = 0,
    HW_SINE = 1,
    I5I6    = 2,
} STF_SRC_T;

// Sine wave generator related
typedef enum
{
    SINE_TONE_CH1    = 0,
    SINE_TONE_CH2    = 1,
    SINE_TONE_STEREO = 2
} SINE_TONE_CH_T;

typedef enum
{
    SINE_TONE_128   = 0,
    SINE_TONE_64    = 1,
    SINE_TONE_32    = 2,
    SINE_TONE_16    = 3,
    SINE_TONE_8     = 4,
    SINE_TONE_4     = 5,
    SINE_TONE_2     = 6,
    SINE_TONE_1     = 7
} SINE_TONE_AMP_T;

typedef enum
{
    SINE_TONE_8K        = 0,
    SINE_TONE_11K       = 1,
    SINE_TONE_12K       = 2,
    SINE_TONE_16K       = 3,
    SINE_TONE_22K       = 4,
    SINE_TONE_24K       = 5,
    SINE_TONE_32K       = 6,
    SINE_TONE_44K       = 7,
    SINE_TONE_48K       = 8,
    SINE_TONE_LOOPBACK  = 9
} SINE_TONE_SINEMODE_T;

typedef enum
{
    SINE_TONE_LOOPBACK_I0_I1   = 0,
    SINE_TONE_LOOPBACK_I2      = 1,
    SINE_TONE_LOOPBACK_I3_I4   = 2,
    SINE_TONE_LOOPBACK_I5_I6   = 3,
    SINE_TONE_LOOPBACK_I7_I8   = 4,
    SINE_TONE_LOOPBACK_I9_I10  = 5,
    SINE_TONE_LOOPBACK_I11_I12 = 6,
    SINE_TONE_LOOPBACK_O0_O1   = 7,
    SINE_TONE_LOOPBACK_O2      = 8,
    SINE_TONE_LOOPBACK_O3_O4   = 9,
    SINE_TONE_LOOPBACK_O5_O6   = 10,
    SINE_TONE_LOOPBACK_O7_O8   = 11,
    SINE_TONE_LOOPBACK_O9_O10  = 12,
    SINE_TONE_LOOPBACK_O11     = 13,
    SINE_TONE_LOOPBACK_O12     = 14
} SINE_TONE_LOOPBACK_T;

typedef struct
{
    UINT32               u4ch1_freq_div; // 64/n sample/period
    SINE_TONE_AMP_T      rch1_amp_div;
    SINE_TONE_SINEMODE_T rch1_sine_mode;
    UINT32               u4ch2_freq_div; // 64/n sample/period
    SINE_TONE_AMP_T      rch2_amp_div;
    SINE_TONE_SINEMODE_T rch2_sine_mode;
    SINE_TONE_LOOPBACK_T rloopback_mode;
} AFE_SINEGEN_INFO_T;


/*****************************************************************************
 *                          C O N S T A N T S
 *****************************************************************************/
#define AUDIO_HW_PHYSICAL_BASE  (0x12000000)

#ifdef LDVT
#define AUDIO_HW_VIRTUAL_BASE   (0xF2000000)
#else
#define AUDIO_HW_VIRTUAL_BASE   (0x12000000)
#endif

#define AFE_BASE                (AUDIO_HW_VIRTUAL_BASE)

//Internal sram: 0x12004000~0x12007FFF (16K)
#define AFE_INTERNAL_SRAM_PHY_BASE  (AUDIO_HW_PHYSICAL_BASE+0x4000)
#define AFE_INTERNAL_SRAM_VIR_BASE  (AUDIO_HW_VIRTUAL_BASE +0x4000)
#define AFE_INTERNAL_SRAM_SIZE  (0x4000)

//Dram
//#define AFE_EXTERNAL_DRAM_SIZE  (0x00200000) //2M
#define AFE_EXTERNAL_DRAM_SIZE  (0x8000)
/*****************************************************************************
 *                         M A C R O
 *****************************************************************************/
#define WriteREG(_addr, _value) (*(volatile UINT32 *)(_addr) = (_value))
#define ReadREG(_addr)          (*(volatile UINT32 *)(_addr))

/*****************************************************************************
 *                  R E G I S T E R       D E F I N I T I O N
 *****************************************************************************/
#define AUDIO_TOP_CON0  (AFE_BASE + 0x0000)
#define AUDIO_TOP_CON1  (AFE_BASE + 0x0004)
#define AUDIO_TOP_CON3  (AFE_BASE + 0x000C)
#define AFE_DAC_CON0    (AFE_BASE + 0x0010)
#define AFE_DAC_CON1    (AFE_BASE + 0x0014)
#define AFE_I2S_CON     (AFE_BASE + 0x0018)
#define AFE_DAIBT_CON0  (AFE_BASE + 0x001c)

#define AFE_CONN0       (AFE_BASE + 0x0020)
#define AFE_CONN1       (AFE_BASE + 0x0024)
#define AFE_CONN2       (AFE_BASE + 0x0028)
#define AFE_CONN3       (AFE_BASE + 0x002C)
#define AFE_CONN4       (AFE_BASE + 0x0030)

#define AFE_I2S_CON1    (AFE_BASE + 0x0034)
#define AFE_I2S_CON2    (AFE_BASE + 0x0038)
#define AFE_I2S_CON3    (AFE_BASE + 0x003C)

// Memory interface
#define AFE_DL1_BASE    (AFE_BASE + 0x0040)
#define AFE_DL1_CUR     (AFE_BASE + 0x0044)
#define AFE_DL1_END     (AFE_BASE + 0x0048)
#define AFE_DL2_BASE    (AFE_BASE + 0x0050)
#define AFE_DL2_CUR     (AFE_BASE + 0x0054)
#define AFE_DL2_END     (AFE_BASE + 0x0058)
#define AFE_AWB_BASE    (AFE_BASE + 0x0070)
#define AFE_AWB_END     (AFE_BASE + 0x0078)
#define AFE_AWB_CUR     (AFE_BASE + 0x007C)
#define AFE_VUL_BASE    (AFE_BASE + 0x0080)
#define AFE_VUL_END     (AFE_BASE + 0x0088)
#define AFE_VUL_CUR     (AFE_BASE + 0x008C)
#define AFE_DAI_BASE    (AFE_BASE + 0x0090)
#define AFE_DAI_END     (AFE_BASE + 0x0098)
#define AFE_DAI_CUR     (AFE_BASE + 0x009C)

// IRQ control, for Fcore (Modem), not use any more
/*
#define AFE_IRQ_CON         (AFE_BASE + 0x00A0)
#define AFE_IRQ_STATUS      (AFE_BASE + 0x00A4)
#define AFE_IRQ_CLR         (AFE_BASE + 0x00A8)
#define AFE_IRQ_CNT1        (AFE_BASE + 0x00AC)
#define AFE_IRQ_CNT2        (AFE_BASE + 0x00B0)
#define AFE_IRQ_MON2        (AFE_BASE + 0x00B8)
#define AFE_IRQ1_CNT_MON    (AFE_BASE + 0x00C0)
#define AFE_IRQ2_CNT_MON    (AFE_BASE + 0x00C4)
#define AFE_IRQ_CNT_MON     (AFE_BASE + 0x00C8)
*/

// Memory interface monitor
#define AFE_MEMIF_MON0 (AFE_BASE + 0x00D0)
#define AFE_MEMIF_MON1 (AFE_BASE + 0x00D4)
#define AFE_MEMIF_MON2 (AFE_BASE + 0x00D8)
#define AFE_MEMIF_MON3 (AFE_BASE + 0x00DC)
#define AFE_MEMIF_MON4 (AFE_BASE + 0x00E0)

#define AFE_DL_SRC1_1   (AFE_BASE + 0x0100)
#define AFE_DL_SRC1_2   (AFE_BASE + 0x0104)
#define AFE_DL_SRC2_1   (AFE_BASE + 0x0108)
#define AFE_DL_SRC2_2   (AFE_BASE + 0x010C)
#define AFE_DL_SDM_CON0 (AFE_BASE + 0x0110)
#define AFE_UL_SRC_0    (AFE_BASE + 0x0114)
#define AFE_UL_SRC_1    (AFE_BASE + 0x0118)

#define AFE_VAGC_CON1  (AFE_BASE + 0x0120)
#define AFE_VAGC_CON2  (AFE_BASE + 0x0124)
#define AFE_VAGC_CON3  (AFE_BASE + 0x0128)
#define AFE_VAGC_CON4  (AFE_BASE + 0x012C)
#define AFE_VAGC_CON5  (AFE_BASE + 0x0130)
#define AFE_VAGC_CON6  (AFE_BASE + 0x0134)
#define AFE_VAGC_CON7  (AFE_BASE + 0x0138)
#define AFE_VAGC_CON8  (AFE_BASE + 0x013C)
#define AFE_VAGC_CON9  (AFE_BASE + 0x0140)
#define AFE_VAGC_CON10 (AFE_BASE + 0x0144)
#define AFE_VAGC_CON11 (AFE_BASE + 0x0148)
#define AFE_VAGC_CON12 (AFE_BASE + 0x014C)
#define AFE_VAGC_CON13 (AFE_BASE + 0x0150)
#define AFE_VAGC_CON14 (AFE_BASE + 0x0154)
#define AFE_VAGC_CON15 (AFE_BASE + 0x0158)
#define AFE_VAGC_CON16 (AFE_BASE + 0x015C)
#define AFE_VAGC_CON17 (AFE_BASE + 0x0160)
#define AFE_VAGC_CON18 (AFE_BASE + 0x0164)
#define AFE_VAGC_CON19 (AFE_BASE + 0x0168)

#define AFE_FOC_CON  (AFE_BASE + 0x0170)
#define AFE_FOC_CON1 (AFE_BASE + 0x0174)
#define AFE_FOC_CON2 (AFE_BASE + 0x0178)
#define AFE_FOC_CON3 (AFE_BASE + 0x017C)
#define AFE_FOC_CON4 (AFE_BASE + 0x0180)
#define AFE_FOC_CON5 (AFE_BASE + 0x0184)

#define AFE_MON_STEP        (AFE_BASE + 0x0188)
#define AFE_SIDETONE_DEBUG  (AFE_BASE + 0x01D0)
#define AFE_SIDETONE_MON    (AFE_BASE + 0x01D4)
#define AFE_SIDETONE_CON0   (AFE_BASE + 0x01E0)
#define AFE_SIDETONE_COEFF  (AFE_BASE + 0x01E4)
#define AFE_SIDETONE_CON1   (AFE_BASE + 0x01E8)
#define AFE_SIDETONE_GAIN   (AFE_BASE + 0x01EC)

#define AFE_SGEN_CON0   (AFE_BASE + 0x01F0)
#define AFE_TOP_CON0    (AFE_BASE + 0x0200)
#define AFE_VAGC_CON0   (AFE_BASE + 0x020C)
#define AFE_PREDIS_CON0 (AFE_BASE + 0x0260)
#define AFE_PREDIS_CON1 (AFE_BASE + 0x0264)

#define AFE_AGC_MON0 (AFE_BASE + 0x0290)
#define AFE_AGC_MON1 (AFE_BASE + 0x0294)
#define AFE_AGC_MON2 (AFE_BASE + 0x0298)
#define AFE_AGC_MON3 (AFE_BASE + 0x029C)
#define AFE_AGC_MON4 (AFE_BASE + 0x02A0)
#define AFE_AGC_MON5 (AFE_BASE + 0x0318)

#define AFE_VAD_MON0  (AFE_BASE + 0x02A4)
#define AFE_VAD_MON1  (AFE_BASE + 0x02A8)
#define AFE_VAD_MON2  (AFE_BASE + 0x02AC)
#define AFE_VAD_MON3  (AFE_BASE + 0x02B0)
#define AFE_VAD_MON4  (AFE_BASE + 0x02B4)
#define AFE_VAD_MON5  (AFE_BASE + 0x02B8)
#define AFE_VAD_MON6  (AFE_BASE + 0x02BC)
#define AFE_VAD_MON7  (AFE_BASE + 0x02C0)
#define AFE_VAD_MON8  (AFE_BASE + 0x02C4)
#define AFE_VAD_MON9  (AFE_BASE + 0x02C8)
#define AFE_VAD_MON10 (AFE_BASE + 0x02CC)
#define AFE_VAD_MON11 (AFE_BASE + 0x02D0)

#define AFE_MOD_PCM_BASE (AFE_BASE + 0x0330)
#define AFE_MOD_PCM_END  (AFE_BASE + 0x0338)
#define AFE_MOD_PCM_CUR  (AFE_BASE + 0x033C)

//HDMI Memory interface
#define AFE_HDMI_OUT_CON0 (AFE_BASE + 0x0370)
#define AFE_HDMI_OUT_BASE (AFE_BASE + 0x0374)
#define AFE_HDMI_OUT_CUR  (AFE_BASE + 0x0378)
#define AFE_HDMI_OUT_END  (AFE_BASE + 0x037C)

#define AFE_SPDIF_OUT_CON0 (AFE_BASE + 0x0380)
#define AFE_SPDIF_BASE     (AFE_BASE + 0x0384)
#define AFE_SPDIF_CUR      (AFE_BASE + 0x0388)
#define AFE_SPDIF_END      (AFE_BASE + 0x038C)

#define AFE_HDMI_CONN0      (AFE_BASE + 0x0390)
#define AFE_8CH_I2S_OUT_CON (AFE_BASE + 0x0394)

#define AFE_IRQ_CON             (AFE_BASE + 0x03A0)
#define AFE_IRQ_STATUS          (AFE_BASE + 0x03A4)
#define AFE_IRQ_CLR             (AFE_BASE + 0x03A8)
#define AFE_IRQ_CNT1            (AFE_BASE + 0x03AC)
#define AFE_IRQ_CNT2            (AFE_BASE + 0x03B0)
#define AFE_IRQ_MON2            (AFE_BASE + 0x03B8)
#define AFE_IRQ_CNT5            (AFE_BASE + 0x03BC)
#define AFE_IRQ1_CNT_MON        (AFE_BASE + 0x03C0)
#define AFE_IRQ2_CNT_MON        (AFE_BASE + 0x03C4)
#define AFE_IRQ1_EN_CNT_MON     (AFE_BASE + 0x03C8)
#define AFE_MEMIF_MINLEN        (AFE_BASE + 0x03D0)
#define AFE_MEMIF_MAXLEN        (AFE_BASE + 0x03D4)
#define AFE_IEC_PREFETCH_SIZE   (AFE_BASE + 0x03D8)

//AFE GAIN CONTROL REGISTER
#define AFE_GAIN1_CON0         (AFE_BASE + 0x0410)
#define AFE_GAIN1_CON1         (AFE_BASE + 0x0414)
#define AFE_GAIN1_CON2         (AFE_BASE + 0x0418)
#define AFE_GAIN1_CON3         (AFE_BASE + 0x041C)
#define AFE_GAIN1_CONN         (AFE_BASE + 0x0420)
#define AFE_GAIN1_CUR          (AFE_BASE + 0x0424)
#define AFE_GAIN2_CON0         (AFE_BASE + 0x0428)
#define AFE_GAIN2_CON1         (AFE_BASE + 0x042C)
#define AFE_GAIN2_CON2         (AFE_BASE + 0x0430)
#define AFE_GAIN2_CON3         (AFE_BASE + 0x0434)
#define AFE_GAIN2_CONN         (AFE_BASE + 0x0438)
#define AFE_GAIN2_CONN2        (AFE_BASE + 0x0438)
#define AFE_GAIN2_CUR          (AFE_BASE + 0x043C)


#define AFE_IEC_CFG         (AFE_BASE + 0x0480)
#define AFE_IEC_NSNUM       (AFE_BASE + 0x0484)
#define AFE_IEC_BURST_INFO  (AFE_BASE + 0x0488)
#define AFE_IEC_BURST_LEN   (AFE_BASE + 0x048C)
#define AFE_IEC_NSADR       (AFE_BASE + 0x0490)
#define AFE_IEC_CHL_STAT0   (AFE_BASE + 0x04A0)
#define AFE_IEC_CHL_STAT1   (AFE_BASE + 0x04A4)
#define AFE_IEC_CHR_STAT0   (AFE_BASE + 0x04A8)
#define AFE_IEC_CHR_STAT1   (AFE_BASE + 0x04AC)

#define FPGA_CFG0           (AFE_BASE + 0x4C0)
#define FPGA_CFG1           (AFE_BASE + 0x4C4)
#define FPGA_VERSION        (AFE_BASE + 0x4C8)
#define FPGA_AUDIO_CLOCK    (AFE_BASE + 0x4CC)

#define AFE_ASRC_CON0   (AFE_BASE + 0x500)
#define AFE_ASRC_CON1   (AFE_BASE + 0x504)
#define AFE_ASRC_CON2   (AFE_BASE + 0x508)
#define AFE_ASRC_CON3   (AFE_BASE + 0x50C)
#define AFE_ASRC_CON4   (AFE_BASE + 0x510)
#define AFE_ASRC_CON5   (AFE_BASE + 0x514)
#define AFE_ASRC_CON6   (AFE_BASE + 0x518)
#define AFE_ASRC_CON7   (AFE_BASE + 0x51C)
#define AFE_ASRC_CON8   (AFE_BASE + 0x520)
#define AFE_ASRC_CON9   (AFE_BASE + 0x524)
#define AFE_ASRC_CON10  (AFE_BASE + 0x528)
#define AFE_ASRC_CON11  (AFE_BASE + 0x52C)

#define PCM_INTF_CON    (AFE_BASE + 0x530)
#define PCM_INTF_CON2   (AFE_BASE + 0x53C)


/**********************************
 *  Detailed Definitions
 **********************************/

// AUDIO_TOP_CON3
#define HDMI_BCK_DIV_LEN      6
#define HDMI_BCK_DIV_POS      8

//AFE_TOP_CON0
#define PDN_AFE  2
#define PDN_ADC  5
#define PDN_I2S  6
#define APB_W2T 12
#define APB_R2T 13
#define APB_SRC 14
//AFE_DAC_CON0
#define AFE_ON      0
#define DL1_ON      1
#define DL2_ON      2
#define VUL_ON      3
#define DAI_ON      4
#define I2S_ON      5
#define AWB_ON      6
#define MOD_PCM_ON  7
#define AFE_ON_RETM  12
#define AFE_DL1_RETM 13
#define AFE_DL2_RETM 14
#define AFE_AWB_RETM 16

//AFE_DAC_CON1
#define DL1_MODE_LEN    4
#define DL1_MODE_POS    0

#define DL2_MODE_LEN    4
#define DL2_MODE_POS    4

#define I2S_MODE_LEN    4
#define I2S_MODE_POS    8

#define AWB_MODE_LEN    4
#define AWB_MODE_POS    12

#define VUL_MODE_LEN    4
#define VUL_MODE_POS    16

#define DAI_MODE_LEN    1
#define DAI_MODE_POS    20

#define DL1_DATA_LEN    1
#define DL1_DATA_POS    21

#define DL2_DATA_LEN    1
#define DL2_DATA_POS    22

#define I2S_DATA_LEN    1
#define I2S_DATA_POS    23

#define AWB_DATA_LEN    1
#define AWB_DATA_POS    24

#define AWB_R_MONO_LEN  1
#define AWB_R_MONO_POS  25

#define VUL_DATA_LEN    1
#define VUL_DATA_POS    27

#define VUL_R_MONO_LEN  1
#define VUL_R_MONO_POS  28

#define DAI_DUP_WR_LEN  1
#define DAI_DUP_WR_POS  29

#define MOD_PCM_MODE_LEN    1
#define MOD_PCM_MODE_POS    30

#define MOD_PCM_DUP_WR_LEN  1
#define MOD_PCM_DUP_WR_POS  31

//AFE_I2S_CON1 and AFE_I2S_CON2
#define AI2S_EN_POS             0
#define AI2S_EN_LEN             1
#define AI2S_WLEN_POS           1
#define AI2S_WLEN_LEN           1
#define AI2S_FMT_POS            3
#define AI2S_FMT_LEN            1
#define AI2S_OUT_MODE_POS       8
#define AI2S_OUT_MODE_LEN       4
#define AI2S_UPDATE_WORD_POS    24
#define AI2S_UPDATE_WORD_LEN    5
#define AI2S_LR_SWAP_POS        31
#define AI2S_LR_SWAP_LEN        1


#define I2S_EN_POS          0
#define I2S_EN_LEN          1
#define I2S_WLEN_POS        1
#define I2S_WLEN_LEN        1
#define I2S_SRC_POS         2
#define I2S_SRC_LEN         1
#define I2S_FMT_POS         3
#define I2S_FMT_LEN         1
#define I2S_DIR_POS         4
#define I2S_DIR_LEN         1
#define I2S_OUT_MODE_POS    8
#define I2S_OUT_MODE_LEN    4

#define FOC_EN_POS  0
#define FOC_EN_LEN  1


// Modem PCM 1
#define PCM_EN_POS          0
#define PCM_EN_LEN          1

#define PCM_FMT_POS         1
#define PCM_FMT_LEN         2

#define PCM_MODE_POS        3
#define PCM_MODE_LEN        1

#define PCM_WLEN_POS        4
#define PCM_WLEN_LEN        1

#define PCM_SLAVE_POS       5
#define PCM_SLAVE_LEN       1

#define PCM_BYP_ASRC_POS    6
#define PCM_BYP_ASRC_LEN    1

#define PCM_BTMODE_POS      7
#define PCM_BTMODE_LEN      1

#define PCM_SYNC_TYPE_POS   8
#define PCM_SYNC_TYPE_LEN   1

#define PCM_SYNC_LEN_POS    9
#define PCM_SYNC_LEN_LEN    5

#define PCM_EXT_MODEM_POS   17
#define PCM_EXT_MODEM_LEN   1

#define PCM_VBT16K_MODE_POS 18
#define PCM_VBT16K_MODE_LEN 1


//#define PCM_BCKINV_POS      6
//#define PCM_BCKINV_LEN      1
//#define PCM_SYNCINV_POS     7
//#define PCM_SYNCINV_LEN     1

#define PCM_SERLOOPBK_POS   28
#define PCM_SERLOOPBK_LEN   1

#define PCM_PARLOOPBK_POS   29
#define PCM_PARLOOPBK_LEN   1

#define PCM_BUFLOOPBK_POS   30
#define PCM_BUFLOOPBK_LEN   1

#define PCM_FIX_VAL_SEL_POS 31
#define PCM_FIX_VAL_SEL_LEN 1



// BT PCM
#define DAIBT_EN_POS   0
#define DAIBT_EN_LEN   1
#define BTPCM_EN_POS   1
#define BTPCM_EN_LEN   1
#define BTPCM_SYNC_POS   2
#define BTPCM_SYNC_LEN   1
#define DAIBT_DATARDY_POS   3
#define DAIBT_DATARDY_LEN   1
#define BTPCM_LENGTH_POS   4
#define BTPCM_LENGTH_LEN   3
#define DAIBT_MODE_POS   9
#define DAIBT_MODE_LEN   1



// AFE_IRQ_CON
#define IRQ1_ON         0
#define IRQ2_ON         1
#define IRQ3_ON         2
#define IRQ4_ON         3
#define IRQ1_FS         4
#define IRQ2_FS         8
#define IRQ5_ON         12
#define IRQ6_ON         13
#define IRQ_SETTING_BIT 0x3007

// AFE_IRQ_STATUS
#define IRQ1_ON_BIT     1<<0
#define IRQ2_ON_BIT     1<<1
#define IRQ3_ON_BIT     1<<2
#define IRQ4_ON_BIT     1<<3
#define IRQ5_ON_BIT     1<<4
#define IRQ6_ON_BIT     1<<5
#define IRQ_STATUS_BIT  0x3F

// AFE_IRQ_CLR
#define IRQ1_CLR 1<<0
#define IRQ2_CLR 1<<1
#define IRQ3_CLR 1<<2
#define IRQ4_CLR 1<<3
#define IRQ_CLR  1<<4

#define IRQ1_MISS_CLR 1<<8
#define IRQ2_MISS_CLR 1<<9
#define IRQ3_MISS_CLR 1<<10
#define IRQ4_MISS_CLR 1<<11
#define IRQ5_MISS_CLR 1<<12
#define IRQ6_MISS_CLR 1<<13

// AFE_IRQ_MON2
#define IRQ1_MISS_BIT       1<<8
#define IRQ2_MISS_BIT       1<<9
#define IRQ3_MISS_BIT       1<<10
#define IRQ4_MISS_BIT       1<<11
#define IRQ5_MISS_BIT       1<<12
#define IRQ6_MISS_BIT       1<<13
#define IRQ_MISS_STATUS_BIT 0x3F00

// AUDIO_TOP_CON3
#define HDMI_OUT_SPEAKER_BIT    4
#define SPEAKER_OUT_HDMI        5
#define HDMI_2CH_SEL_POS        6
#define HDMI_2CH_SEL_LEN        2

//AFE_HDMI_OUT_CON0
#define HDMI_OUT_ON_POS         0
#define HDMI_OUT_BITWIDTH_POS   1
#define HDMI_OUT_BITWIDTH_LEN   1
#define HDMI_OUT_CH_NUM_POS     4
#define HDMI_OUT_CH_NUM_LEN     4

//AFE_HDMI_CONN0
#define HDMI_SPDIF_SRC_SEL  30

//AFE_8CH_I2S_OUT_CON
#define I2S_8CH_WLEN_POS    4
#define I2S_8CH_WLEN_LEN    2

// AFE_SIDETONE_DEBUG
#define STF_SRC_SEL     16
#define STF_I5I6_SEL    19

// AFE_SIDETONE_CON0
#define STF_COEFF_VAL       0
#define STF_COEFF_ADDRESS   16
#define STF_CH_SEL          23
#define STF_COEFF_W_ENABLE  24
#define STF_W_ENABLE        25
#define STF_COEFF_BIT       0x0000FFFF

// AFE_SIDETONE_CON1
#define STF_TAP_NUM 0
#define STF_ON      8
#define STF_BYPASS  31

// AFE_SGEN_CON0
#define SINE_TONE_FREQ_DIV_CH1  0
#define SINE_TONE_AMP_DIV_CH1   5
#define SINE_TONE_MODE_CH1      8
#define SINE_TONE_FREQ_DIV_CH2  12
#define SINE_TONE_AMP_DIV_CH2   17
#define SINE_TONE_MODE_CH2      20
#define SINE_TONE_MUTE_CH1      24
#define SINE_TONE_MUTE_CH2      25
#define SINE_TONE_ENABLE        26
#define SINE_TONE_LOOPBACK_MOD  28


// AFE_SPDIF_OUT_CON0
#define AFE_SPDIF_OUT_ENABLE    0
#define AFE_SPDIF_MEMIF_ENABLE  1

// AFE_IEC_CFG
#define IEC_RAWSEL_POS          0
#define IEC_RAWSEL_LEN          1
#define IEC_PCMSEL_POS          1
#define IEC_PCMSEL_LEN          1
#define IEC_MUTE                3
#define IEC_VALID_POS           4
#define IEC_VALID_LEN           1
#define IEC_IEC_ENABLE_POS      16
#define IEC_IEC_ENABLE_LEN      1
#define IEC_DOWNSAMPLE_POS      18
#define IEC_DOWNSAMPLE_LEN      2
#define IEC_IEC_FORCE_EN_POS    20
#define IEC_IEC_FORCE_EN_LEN    1
#define IEC_REG_LOCK_EN_POS     22
#define IEC_REG_LOCK_EN_LEN     1
#define IEC_IEC_RESET_EN_POS    23
#define IEC_IEC_RESET_EN_LEN    1
#define IEC_IEC_FORCE_CNT_POS   24
#define IEC_IEC_FORCE_CNT_LEN   8

// AFE_IEC_NSNUM
#define IEC_NX_SAM_NUM      0x3FFF
#define IEC_NX_SAM_NUM_POS  0
#define IEC_NX_SAM_NUM_LEN  14
#define IEC_NX_INTR_NUM_POS 16
#define IEC_NX_INTR_NUM_LEN 14

// AFE_IEC_BURST_INFO
#define IEC_NXBURST_RDY     1<<16
#define IEC_NXBURST_RDY_POS 16
#define IEC_REG_LOCK_STA    1<<20

// AFE_IEC_BURST_LEN
#define IEC_BURST_LEN       0x7FFFF
#define IEC_BURST_LEN_POS   0
#define IEC_BURST_LEN_LEN   19

//FPGA_CFG0
#define MCLK_MUX2_POS    26
#define MCLK_MUX2_LEN    1
#define MCLK_MUX1_POS    25
#define MCLK_MUX1_LEN    1
#define MCLK_MUX0_POS    24
#define MCLK_MUX0_LEN    1
#define SOFT_RST_POS   16
#define SOFT_RST_LEN   8
#define HOP26M_SEL_POS   12
#define HOP26M_SEL_LEN   2

//FPGA_CFG1
#define CODEC_SEL_POS   0
#define DAC_SEL_POS     4
#define ADC_SEL_POS     8

/**********************************
 *  Other Definitions             *
 **********************************/
#define BIT_00	0x00000001        /* ---- ---- ---- ---- ---- ---- ---- ---1 */
#define BIT_01	0x00000002        /* ---- ---- ---- ---- ---- ---- ---- --1- */
#define BIT_02	0x00000004        /* ---- ---- ---- ---- ---- ---- ---- -1-- */
#define BIT_03	0x00000008        /* ---- ---- ---- ---- ---- ---- ---- 1--- */
#define BIT_04	0x00000010        /* ---- ---- ---- ---- ---- ---- ---1 ---- */
#define BIT_05	0x00000020        /* ---- ---- ---- ---- ---- ---- --1- ---- */
#define BIT_06	0x00000040        /* ---- ---- ---- ---- ---- ---- -1-- ---- */
#define BIT_07	0x00000080        /* ---- ---- ---- ---- ---- ---- 1--- ---- */
#define BIT_08	0x00000100        /* ---- ---- ---- ---- ---- ---1 ---- ---- */
#define BIT_09	0x00000200        /* ---- ---- ---- ---- ---- --1- ---- ---- */
#define BIT_10	0x00000400        /* ---- ---- ---- ---- ---- -1-- ---- ---- */
#define BIT_11	0x00000800        /* ---- ---- ---- ---- ---- 1--- ---- ---- */
#define BIT_12	0x00001000        /* ---- ---- ---- ---- ---1 ---- ---- ---- */
#define BIT_13	0x00002000        /* ---- ---- ---- ---- --1- ---- ---- ---- */
#define BIT_14	0x00004000        /* ---- ---- ---- ---- -1-- ---- ---- ---- */
#define BIT_15	0x00008000        /* ---- ---- ---- ---- 1--- ---- ---- ---- */
#define BIT_16	0x00010000        /* ---- ---- ---- ---1 ---- ---- ---- ---- */
#define BIT_17	0x00020000        /* ---- ---- ---- --1- ---- ---- ---- ---- */
#define BIT_18	0x00040000        /* ---- ---- ---- -1-- ---- ---- ---- ---- */
#define BIT_19	0x00080000        /* ---- ---- ---- 1--- ---- ---- ---- ---- */
#define BIT_20	0x00100000        /* ---- ---- ---1 ---- ---- ---- ---- ---- */
#define BIT_21	0x00200000        /* ---- ---- --1- ---- ---- ---- ---- ---- */
#define BIT_22	0x00400000        /* ---- ---- -1-- ---- ---- ---- ---- ---- */
#define BIT_23	0x00800000        /* ---- ---- 1--- ---- ---- ---- ---- ---- */
#define BIT_24	0x01000000        /* ---- ---1 ---- ---- ---- ---- ---- ---- */
#define BIT_25	0x02000000        /* ---- --1- ---- ---- ---- ---- ---- ---- */
#define BIT_26	0x04000000        /* ---- -1-- ---- ---- ---- ---- ---- ---- */
#define BIT_27	0x08000000        /* ---- 1--- ---- ---- ---- ---- ---- ---- */
#define BIT_28	0x10000000        /* ---1 ---- ---- ---- ---- ---- ---- ---- */
#define BIT_29	0x20000000        /* --1- ---- ---- ---- ---- ---- ---- ---- */
#define BIT_30	0x40000000        /* -1-- ---- ---- ---- ---- ---- ---- ---- */
#define BIT_31	0x80000000        /* 1--- ---- ---- ---- ---- ---- ---- ---- */

UINT32 u4BufPhyAddr2VirAddr(UINT32 u4PhyBufAddr);

extern void vRegWriteBits(INT32 addr,INT32 value,INT8 bits,INT8 len);
extern UINT32 u4RegReadBits(INT32 addr,INT8 bits,INT8 len);
extern void vRegSetBit(INT32 addr,INT32 bit);
extern void vRegResetBit(INT32 addr,INT32 bit);

extern void vAfeTurnOnMemif(MEMIF_CONFIG_T *memif_config);
extern void vAfeTurnOffMemif(MEMIF_CONFIG_T *memif_config);
extern void vAfeGetCurPointer(MEMIF_CONFIG_T *memif_config);
extern void vAfeTurnOn(void);
extern void vAfeTurnOff(void);
extern void vAudioTopControl(void);
extern BOOL bConnect(ITRCON_IN_T in, ITRCON_OUT_T out, BOOL rShift);
extern BOOL bDisConnect(ITRCON_IN_T in, ITRCON_OUT_T out);
extern void vDisConnectAll(void);
extern void vADCI2SEnable(I2SEN_T value);
extern void vADCI2SSet( I2SWLEN_T wlen, I2SFMT_T fmt, SAMPLINGRATE_T sr, INT32 update, I2SSWAP_T swap);
extern void vDACI2SEnable(I2SEN_T value);
extern void vDACI2SSet( I2SWLEN_T wlen, I2SFMT_T fmt, SAMPLINGRATE_T sr, INT32 update, I2SSWAP_T swap);
extern void vI2SEnable(I2SEN_T value);
extern void vI2SSet( I2SWLEN_T wlen, I2SFMT_T fmt, SAMPLINGRATE_T sr, I2SSRC_T src, I2SDIR_T dir);
extern void vFOCEnable(BOOL bEnable);

extern void vBtPcmEnable(BTPCMEN_T eValue);
extern void vBtPcmSet(BTPCMMODE_T eSr) ;

extern void vStrongResetAFE(void);
extern void vSoftResetAFE(void);

extern void vAfeSineWaveGenCfg(AFE_SINEGEN_INFO_T *rSineTone);
extern void vAfeSineWaveGenEnable(BOOL bEnable);
extern void vAfeSineWaveMute(SINE_TONE_CH_T eCh);
extern void vFpgaModemDaiSwitch(BOOL switchModem);

extern UINT32 u4AfeGetATime(void);
extern UINT32 u4AfeGetATimeDiff(UINT32 u4StartTime);
extern void vFpgaAudioClockDelaySec(UINT32 u4Second);
extern void vFpgaAudioClockDelaymSec(UINT32 u4mSecond);


extern UINT32 u4SamplingRate[9];
extern UINT32 u4SamplingRateConvert[9];
extern UINT32 u4SamplingRateHz[9];

extern UINT32 u4DramPhyBase;
extern UINT32 u4DramVirBase;
extern UINT32 u4DramPhyEnd;
extern UINT32 u4DramVirEnd;

extern UINT32 u4SramPhyBase;
extern UINT32 u4SramVirBase;
extern UINT32 u4SramPhyEnd;
extern UINT32 u4SramVirEnd;

#endif /*_AUDIO_AFE_H_*/
