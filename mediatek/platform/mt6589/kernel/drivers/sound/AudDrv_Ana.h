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
 *   AudDrv_Ana.h
 *
 * Project:
 * --------
 *   MT6583  Audio Driver Ana
 *
 * Description:
 * ------------
 *   Audio register
 *
 * Author:
 * -------
 *   Chipeng Chang (mtk02308)
 *
 *------------------------------------------------------------------------------
 * $Revision: #1 $
 * $Modtime:$
 * $Log:$
 *
 *
 *******************************************************************************/

#ifndef _AUDDRV_ANA_H_
#define _AUDDRV_ANA_H_

/*****************************************************************************
 *                     C O M P I L E R   F L A G S
 *****************************************************************************/


/*****************************************************************************
 *                E X T E R N A L   R E F E R E N C E S
 *****************************************************************************/

#include "AudDrv_Common.h"
#include "AudDrv_Def.h"


/*****************************************************************************
 *                         D A T A   T Y P E S
 *****************************************************************************/


/*****************************************************************************
 *                         M A C R O
 *****************************************************************************/

/*****************************************************************************
 *                  R E G I S T E R       D E F I N I T I O N
 *****************************************************************************/

//---------------digital pmic  register define -------------------------------------------
#define AFE_PMICDIG_AUDIO_BASE        (0x4000)
#define AFE_UL_DL_CON0               (AFE_PMICDIG_AUDIO_BASE+0x0000)
#define AFE_DL_SRC2_CON0_H     (AFE_PMICDIG_AUDIO_BASE+0x0002)
#define AFE_DL_SRC2_CON0_L     (AFE_PMICDIG_AUDIO_BASE+0x0004)
#define AFE_DL_SRC2_CON1_H     (AFE_PMICDIG_AUDIO_BASE+0x0006)
#define AFE_DL_SRC2_CON1_L     (AFE_PMICDIG_AUDIO_BASE+0x0008)
#define AFE_DL_SDM_CON0           (AFE_PMICDIG_AUDIO_BASE+0x000A)
#define AFE_DL_SDM_CON1           (AFE_PMICDIG_AUDIO_BASE+0x000C)
#define AFE_UL_SRC_CON0_H       (AFE_PMICDIG_AUDIO_BASE+0x000E)
#define AFE_UL_SRC_CON0_L       (AFE_PMICDIG_AUDIO_BASE+0x0010)
#define AFE_UL_SRC_CON1_H      (AFE_PMICDIG_AUDIO_BASE+0x0012)
#define AFE_UL_SRC_CON1_L       (AFE_PMICDIG_AUDIO_BASE+0x0014)
#define AFE_PREDIS_CON0_H       (AFE_PMICDIG_AUDIO_BASE+0x0016)
#define AFE_PREDIS_CON0_L       (AFE_PMICDIG_AUDIO_BASE+0x0018)
#define AFE_PREDIS_CON1_H       (AFE_PMICDIG_AUDIO_BASE+0x001a)
#define AFE_PREDIS_CON1_L       (AFE_PMICDIG_AUDIO_BASE+0x001c)
#define ANA_AFE_I2S_CON1                  (AFE_PMICDIG_AUDIO_BASE+0x001e)
#define AFE_I2S_FIFO_UL_CFG0  (AFE_PMICDIG_AUDIO_BASE+0x0020)
#define AFE_I2S_FIFO_DL_CFG0  (AFE_PMICDIG_AUDIO_BASE+0x0022)
#define ANA_AFE_TOP_CON0                  (AFE_PMICDIG_AUDIO_BASE+0x0024)
#define ANA_AUDIO_TOP_CON0             (AFE_PMICDIG_AUDIO_BASE+0x0026)
#define AFE_UL_SRC_DEBUG       (AFE_PMICDIG_AUDIO_BASE+0x0028)
#define AFE_DL_SRC_DEBUG       (AFE_PMICDIG_AUDIO_BASE+0x002a)
#define AFE_UL_SRC_MON0        (AFE_PMICDIG_AUDIO_BASE+0x002c)
#define AFE_DL_SRC_MON0        (AFE_PMICDIG_AUDIO_BASE+0x002e)
#define AFE_DL_SDM_TEST0       (AFE_PMICDIG_AUDIO_BASE+0x0030)
#define AFE_MON_DEBUG0         (AFE_PMICDIG_AUDIO_BASE+0x0032)
#define AFUNC_AUD_CON0         (AFE_PMICDIG_AUDIO_BASE+0x0034)
#define AFUNC_AUD_CON1         (AFE_PMICDIG_AUDIO_BASE+0x0036)
#define AFUNC_AUD_CON2         (AFE_PMICDIG_AUDIO_BASE+0x0038)
#define AFUNC_AUD_CON3         (AFE_PMICDIG_AUDIO_BASE+0x003A)
#define AFUNC_AUD_CON4         (AFE_PMICDIG_AUDIO_BASE+0x003C)
#define AFUNC_AUD_MON0         (AFE_PMICDIG_AUDIO_BASE+0x003E)
#define AFUNC_AUD_MON1         (AFE_PMICDIG_AUDIO_BASE+0x0040)
#define AUDRC_TUNE_MON0        (AFE_PMICDIG_AUDIO_BASE+0x0042)
#define AFE_I2S_FIFO_MON0      (AFE_PMICDIG_AUDIO_BASE+0x0044)
#define AFE_DL_DC_COMP_CFG0    (AFE_PMICDIG_AUDIO_BASE+0x0046)
#define AFE_DL_DC_COMP_CFG1    (AFE_PMICDIG_AUDIO_BASE+0x0048)
#define AFE_DL_DC_COMP_CFG2    (AFE_PMICDIG_AUDIO_BASE+0x004a)
#define AFE_MBIST_CFG0                 (AFE_PMICDIG_AUDIO_BASE+0x004c)
#define AFE_MBIST_CFG1                 (AFE_PMICDIG_AUDIO_BASE+0x004e)
#define AFE_MBIST_CFG2                 (AFE_PMICDIG_AUDIO_BASE+0x0050)
#define AFE_I2S_FIFO_CFG0           (AFE_PMICDIG_AUDIO_BASE+0x0052)
//---------------digital pmic  register define end ---------------------------------------

//---------------analog pmic  register define start --------------------------------------
//---------------digital pmic  register define -------------------------------------------
#define AFE_PMICANA_AUDIO_BASE        (0x0)

#define TOP_CKPDN                 (AFE_PMICANA_AUDIO_BASE + 0x102)
#define TOP_CKPDN_SET        (AFE_PMICANA_AUDIO_BASE + 0x104)
#define TOP_CKPDN_CLR        (AFE_PMICANA_AUDIO_BASE + 0x106)
#define TOP_CKPDN2               (AFE_PMICANA_AUDIO_BASE + 0x108)
#define TOP_CKPDN2_SET      (AFE_PMICANA_AUDIO_BASE + 0x10a)
#define TOP_CKPDN2_CLR      (AFE_PMICANA_AUDIO_BASE + 0x10c)
#define TOP_CKCON1              (AFE_PMICANA_AUDIO_BASE + 0x128)

#define SPK_CON0                    (AFE_PMICANA_AUDIO_BASE+0x0600)
#define SPK_CON1                    (AFE_PMICANA_AUDIO_BASE+0x0602)
#define SPK_CON2                    (AFE_PMICANA_AUDIO_BASE+0x0604)
#define SPK_CON3                    (AFE_PMICANA_AUDIO_BASE+0x0606)
#define SPK_CON4                    (AFE_PMICANA_AUDIO_BASE+0x0608)
#define SPK_CON5                    (AFE_PMICANA_AUDIO_BASE+0x060A)
#define SPK_CON6                    (AFE_PMICANA_AUDIO_BASE+0x060C)
#define SPK_CON7                    (AFE_PMICANA_AUDIO_BASE+0x060E)
#define SPK_CON8                    (AFE_PMICANA_AUDIO_BASE+0x0610)
#define SPK_CON9                    (AFE_PMICANA_AUDIO_BASE+0x0612)
#define SPK_CON10                  (AFE_PMICANA_AUDIO_BASE+0x0614)
#define SPK_CON11                  (AFE_PMICANA_AUDIO_BASE+0x0616)

#define AUDDAC_CON0           (AFE_PMICANA_AUDIO_BASE + 0x700)
#define AUDBUF_CFG0            (AFE_PMICANA_AUDIO_BASE + 0x702)
#define AUDBUF_CFG1            (AFE_PMICANA_AUDIO_BASE + 0x704)
#define AUDBUF_CFG2            (AFE_PMICANA_AUDIO_BASE + 0x706)
#define AUDBUF_CFG3            (AFE_PMICANA_AUDIO_BASE + 0x708)
#define AUDBUF_CFG4            (AFE_PMICANA_AUDIO_BASE + 0x70a)
#define IBIASDIST_CFG0        (AFE_PMICANA_AUDIO_BASE + 0x70c)
#define AUDACCDEPOP_CFG0        (AFE_PMICANA_AUDIO_BASE + 0x70e)
#define AUD_IV_CFG0             (AFE_PMICANA_AUDIO_BASE + 0x710)
#define AUDCLKGEN_CFG0        (AFE_PMICANA_AUDIO_BASE + 0x712)
#define AUDLDO_CFG0            (AFE_PMICANA_AUDIO_BASE + 0x714)
#define AUDLDO_CFG1            (AFE_PMICANA_AUDIO_BASE + 0x716)
#define AUDNVREGGLB_CFG0        (AFE_PMICANA_AUDIO_BASE + 0x718)
#define AUD_NCP0                        (AFE_PMICANA_AUDIO_BASE + 0x71a)
#define AUDPREAMP_CON0        (AFE_PMICANA_AUDIO_BASE + 0x71c)
#define AUDADC_CON0           (AFE_PMICANA_AUDIO_BASE + 0x71e)
#define AUDADC_CON1           (AFE_PMICANA_AUDIO_BASE + 0x720)
#define AUDADC_CON2           (AFE_PMICANA_AUDIO_BASE + 0x722)
#define AUDADC_CON3           (AFE_PMICANA_AUDIO_BASE + 0x724)
#define AUDADC_CON4           (AFE_PMICANA_AUDIO_BASE + 0x726)
#define AUDADC_CON5           (AFE_PMICANA_AUDIO_BASE + 0x728)
#define AUDADC_CON6           (AFE_PMICANA_AUDIO_BASE + 0x72a)
#define AUDDIGMI_CON0        (AFE_PMICANA_AUDIO_BASE + 0x72c)
#define AUDLSBUF_CON0        (AFE_PMICANA_AUDIO_BASE + 0x72e)
#define AUDLSBUF_CON1        (AFE_PMICANA_AUDIO_BASE + 0x730)
#define AUDENCSPARE_CON0        (AFE_PMICANA_AUDIO_BASE + 0x732)
#define AUDENCCLKSQ_CON0        (AFE_PMICANA_AUDIO_BASE + 0x734)
#define AUDPREAMPGAIN_CON0        (AFE_PMICANA_AUDIO_BASE + 0x736)
#define ZCD_CON0        (AFE_PMICANA_AUDIO_BASE + 0x738)
#define ZCD_CON1        (AFE_PMICANA_AUDIO_BASE + 0x73a)
#define ZCD_CON2        (AFE_PMICANA_AUDIO_BASE + 0x73c)
#define ZCD_CON3        (AFE_PMICANA_AUDIO_BASE + 0x73e)
#define ZCD_CON4        (AFE_PMICANA_AUDIO_BASE + 0x740)
#define ZCD_CON5        (AFE_PMICANA_AUDIO_BASE + 0x742)
#define NCP_CLKDIV_CON0        (AFE_PMICANA_AUDIO_BASE + 0x744)
#define NCP_CLKDIV_CON1        (AFE_PMICANA_AUDIO_BASE + 0x746)


void Ana_Set_Reg(uint32 offset,uint32 value,uint32 mask);
uint32  Ana_Get_Reg(uint32 offset);

// for debug usage
void Ana_Log_Print(void);

#endif

