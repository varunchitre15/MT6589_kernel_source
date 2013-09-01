


/*******************************************************************************
 *
 * Filename:
 * ---------
 *  auddrv_ioctl.h
 *
 * Project:
 * --------
 *   Android Audio Driver
 *
 * Description:
 * ------------
 *   ioctl control message
 *
 * Author:
 * -------
 *   Chipeng    (mtk02308)
 *
 *
 *------------------------------------------------------------------------------
 * $Revision$
 * $Modtime:$
 * $Log:$
 *
 * 09 28 2011 weiguo.li
 * [ALPS00076254] [Need Patch] [Volunteer Patch]LGE audio driver using Voicebuffer for incall
 * .
 *
 *
 *
 *******************************************************************************/
#ifndef _AUDDRV_IOCTL_MSG_H
#define _AUDDRV_IOCTL_MSG_H


/*****************************************************************************
*                     C O M P I L E R   F L A G S
******************************************************************************
*/


/*****************************************************************************
*                          C O N S T A N T S
******************************************************************************
*/

//below is control message
#define AUD_DRV_IOC_MAGIC 'C'

#define SET_AUDSYS_REG         _IOWR(AUD_DRV_IOC_MAGIC, 0x00, Register_Control*)
#define GET_AUDSYS_REG         _IOWR(AUD_DRV_IOC_MAGIC, 0x01, Register_Control*)
#define SET_ANAAFE_REG         _IOWR(AUD_DRV_IOC_MAGIC, 0x02, Register_Control*)
#define GET_ANAAFE_REG         _IOWR(AUD_DRV_IOC_MAGIC, 0x03, Register_Control*)


// Allocate mean allocate buffer and set stream into ready state.
#define ALLOCATE_MEMIF_DL1           _IOWR(AUD_DRV_IOC_MAGIC, 0x10,unsigned int)
#define FREE_MEMIF_DL1                    _IOWR(AUD_DRV_IOC_MAGIC, 0x11,unsigned int)
#define ALLOCATE_MEMIF_DL2           _IOWR(AUD_DRV_IOC_MAGIC, 0x12,unsigned int)
#define FREE_MEMIF_DL2                    _IOWR(AUD_DRV_IOC_MAGIC, 0x13,unsigned int)
#define ALLOCATE_MEMIF_AWB          _IOWR(AUD_DRV_IOC_MAGIC, 0x14,unsigned int)
#define FREE_MEMIF_AWB                  _IOWR(AUD_DRV_IOC_MAGIC, 0x15,unsigned int)
#define ALLOCATE_MEMIF_ADC           _IOWR(AUD_DRV_IOC_MAGIC, 0x16,unsigned int)
#define FREE_MEMIF_ADC                   _IOWR(AUD_DRV_IOC_MAGIC, 0x17,unsigned int)
#define ALLOCATE_MEMIF_DAI           _IOWR(AUD_DRV_IOC_MAGIC, 0x18,unsigned int)
#define FREE_MEMIF_DAI                    _IOWR(AUD_DRV_IOC_MAGIC, 0x19,unsigned int)
#define ALLOCATE_MEMIF_MODDAI    _IOWR(AUD_DRV_IOC_MAGIC, 0x1a,unsigned int)
#define FREE_MEMIF_MODDAI             _IOWR(AUD_DRV_IOC_MAGIC, 0x1b,unsigned int)

#define AUD_RESTART                         _IOWR(AUD_DRV_IOC_MAGIC, 0x1F,unsigned int)

#define START_MEMIF_TYPE               _IOWR(AUD_DRV_IOC_MAGIC, 0x20,unsigned int)
#define STANDBY_MEMIF_TYPE              _IOWR(AUD_DRV_IOC_MAGIC, 0x21,unsigned int)

#define GET_EAMP_PARAMETER	   _IOWR(AUD_DRV_IOC_MAGIC, 0x3e, AMP_Control *)
#define SET_EAMP_PARAMETER	   _IOWR(AUD_DRV_IOC_MAGIC, 0x3f, AMP_Control *)

#define SET_2IN1_SPEAKER          _IOW(AUD_DRV_IOC_MAGIC, 0x41, int)
#define SET_AUDIO_STATE           _IOWR(AUD_DRV_IOC_MAGIC, 0x42 ,SPH_Control*)
#define GET_AUDIO_STATE           _IOWR(AUD_DRV_IOC_MAGIC, 0x43, SPH_Control*)
#define GET_PMIC_VERSION        _IOWR(AUD_DRV_IOC_MAGIC, 0x44, int)

#define AUD_SET_LINE_IN_CLOCK     _IOWR(AUD_DRV_IOC_MAGIC, 0x50, int)
#define AUD_SET_CLOCK             _IOWR(AUD_DRV_IOC_MAGIC, 0x51, int)
#define AUD_SET_26MCLOCK          _IOWR(AUD_DRV_IOC_MAGIC, 0x52, int)
#define AUD_SET_ADC_CLOCK       _IOWR(AUD_DRV_IOC_MAGIC, 0x53, int)
#define AUD_SET_I2S_CLOCK       _IOWR(AUD_DRV_IOC_MAGIC, 0x54, int)
#define AUD_SET_ANA_CLOCK       _IOWR(AUD_DRV_IOC_MAGIC, 0x55, int)
#define AUD_GET_ANA_CLOCK_CNT   _IOWR(AUD_DRV_IOC_MAGIC, 0x56, int)


#define AUDDRV_SET_BT_FM_GPIO       _IOWR(AUD_DRV_IOC_MAGIC, 0x5f, int)
#define AUDDRV_RESET_BT_FM_GPIO      _IOWR(AUD_DRV_IOC_MAGIC, 0x60, int)
#define AUDDRV_SET_BT_PCM_GPIO       _IOWR(AUD_DRV_IOC_MAGIC, 0x61, int)
#define AUDDRV_SET_FM_I2S_GPIO       _IOWR(AUD_DRV_IOC_MAGIC, 0x62, int)
#define AUDDRV_CHIP_VER              _IOWR(AUD_DRV_IOC_MAGIC, 0x63, int)
#define AUDDRV_SET_RECEIVER_GPIO     _IOWR(AUD_DRV_IOC_MAGIC, 0x64, int)

#define AUDDRV_ENABLE_ATV_I2S_GPIO   _IOWR(AUD_DRV_IOC_MAGIC, 0x65, int)
#define AUDDRV_DISABLE_ATV_I2S_GPIO  _IOWR(AUD_DRV_IOC_MAGIC, 0x66, int)
#define AUDDRV_RESET_FMCHIP_MERGEIF  _IOWR(AUD_DRV_IOC_MAGIC, 0x67, int)

#define AUD_SET_HDMI_CLOCK           _IOWR(AUD_DRV_IOC_MAGIC, 0x68, int)
#define AUD_SET_HDMI_GPIO            _IOWR(AUD_DRV_IOC_MAGIC, 0x69, int)
#define AUD_SET_HDMI_SR              _IOWR(AUD_DRV_IOC_MAGIC, 0x70, int)
#define AUD_SET_HDMI_MUTE            _IOWR(AUD_DRV_IOC_MAGIC, 0x72, int)


#define YUSU_INFO_FROM_USER _IOWR(AUD_DRV_IOC_MAGIC, 0x71, struct _Info_Data*)

#define AUDDRV_START_DAI_OUTPUT    _IOWR(AUD_DRV_IOC_MAGIC, 0x81, int)
#define AUDDRV_STOP_DAI_OUTPUT    _IOWR(AUD_DRV_IOC_MAGIC, 0x82, int)

// used for AUDIO_HQA
#define AUDDRV_HQA_AMP_MODESEL    _IOWR(AUD_DRV_IOC_MAGIC, 0x90, int)
#define AUDDRV_HQA_AMP_AMPEN      _IOWR(AUD_DRV_IOC_MAGIC, 0x91, int)
#define AUDDRV_HQA_AMP_AMPVOL     _IOWR(AUD_DRV_IOC_MAGIC, 0x92, int)
#define AUDDRV_HQA_AMP_RECEIVER   _IOWR(AUD_DRV_IOC_MAGIC, 0x93, int)
#define AUDDRV_HQA_AMP_RECGAIN    _IOWR(AUD_DRV_IOC_MAGIC, 0x94, int)

// used for FTM OC Test
#define AUDDRV_AMP_OC_CFG         _IOWR(AUD_DRV_IOC_MAGIC, 0x95, int)
#define AUDDRV_AMP_OC_READ        _IOWR(AUD_DRV_IOC_MAGIC, 0x96, int)

// used for MD RESET Recovery
#define AUDDRV_MD_RST_RECOVERY    _IOWR(AUD_DRV_IOC_MAGIC, 0x97, int)

// device selection ioctl
#define SET_SPEAKER_VOL          _IOW(AUD_DRV_IOC_MAGIC, 0xa0, int)
#define SET_SPEAKER_ON            _IOW(AUD_DRV_IOC_MAGIC, 0xa1, int)
#define SET_SPEAKER_OFF          _IOW(AUD_DRV_IOC_MAGIC, 0xa2, int)
#define SET_HEADSET_STATE      _IOW(AUD_DRV_IOC_MAGIC, 0xa3, int)
#define SET_HEADPHONE_ON      _IOW(AUD_DRV_IOC_MAGIC, 0xa4, int)
#define SET_HEADPHONE_OFF     _IOW(AUD_DRV_IOC_MAGIC, 0xa5, int)
#define SET_EARPIECE_ON	      _IOW(AUD_DRV_IOC_MAGIC, 0xa6, int)
#define SET_EARPIECE_OFF	      _IOW(AUD_DRV_IOC_MAGIC, 0xa7, int)


// used for debug
#define AUDDRV_AEE_IOCTL              _IOW(AUD_DRV_IOC_MAGIC, 0xFA, int)
#define AUDDRV_GPIO_IOCTL              _IOW(AUD_DRV_IOC_MAGIC, 0xFB, int)
#define AUDDRV_LOG_PRINT              _IOWR(AUD_DRV_IOC_MAGIC, 0xFD, int)
#define AUDDRV_ASSERT_IOCTL        _IOW(AUD_DRV_IOC_MAGIC, 0xFE, int)
#define AUDDRV_BEE_IOCTL              _IOW(AUD_DRV_IOC_MAGIC, 0xFF, int)

/*****************************************************************************
*                         D A T A   T Y P E S
******************************************************************************
*/

typedef struct
{
    uint32 offset;
    uint32 value;
    uint32 mask;
}Register_Control;

typedef struct
{
   int bSpeechFlag;
   int bBgsFlag;
   int bRecordFlag;
   int bTtyFlag;
   int bVT;
   int bAudioPlay;
}SPH_Control;

/*****************************************************************************
*                        F U N C T I O N   D E F I N I T I O N
******************************************************************************
*/


#endif

