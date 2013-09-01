#ifndef AUDDRV_KERNEL_H
#define AUDDRV_KERNEL_H

#include "AudDrv_Common.h"
#include "AudDrv_Def.h"
#include "AudDrv_ioctl.h"

/*****************************************************************************
 *                     C O M P I L E R   F L A G S
 *****************************************************************************/


/*****************************************************************************
 *                E X T E R N A L   R E F E R E N C E S
 *****************************************************************************/


/*****************************************************************************
 *                         D A T A   T Y P E S
 *****************************************************************************/

typedef struct
{
   volatile uint32 Suspend_AUDIO_TOP_CON0;
   volatile uint32 Suspend_AUDIO_TOP_CON3;
   volatile uint32 Suspend_AFE_DAC_CON0;
   volatile uint32 Suspend_AFE_DAC_CON1;
   volatile uint32 Suspend_AFE_I2S_CON;
   volatile uint32 Suspend_AFE_DAIBT_CON0;

   volatile uint32 Suspend_AFE_CONN0;
   volatile uint32 Suspend_AFE_CONN1;
   volatile uint32 Suspend_AFE_CONN2;
   volatile uint32 Suspend_AFE_CONN3;
   volatile uint32 Suspend_AFE_CONN4;

   volatile uint32 Suspend_AFE_I2S_CON1;
   volatile uint32 Suspend_AFE_I2S_CON2;
   volatile uint32 Suspend_AFE_MRGIF_CON;

   volatile uint32 Suspend_AFE_DL1_BASE;
   volatile uint32 Suspend_AFE_DL1_CUR;
   volatile uint32 Suspend_AFE_DL1_END;
   volatile uint32 Suspend_AFE_DL2_BASE;
   volatile uint32 Suspend_AFE_DL2_CUR;
   volatile uint32 Suspend_AFE_DL2_END;
   volatile uint32 Suspend_AFE_AWB_BASE;
   volatile uint32 Suspend_AFE_AWB_CUR;
   volatile uint32 Suspend_AFE_AWB_END;
   volatile uint32 Suspend_AFE_VUL_BASE;
   volatile uint32 Suspend_AFE_VUL_CUR;
   volatile uint32 Suspend_AFE_VUL_END;
   volatile uint32 Suspend_AFE_DAI_BASE;
   volatile uint32 Suspend_AFE_DAI_CUR;
   volatile uint32 Suspend_AFE_DAI_END;

   volatile uint32 Suspend_AFE_IRQ_CON;
   volatile uint32 Suspend_AFE_MEMIF_MON0;
   volatile uint32 Suspend_AFE_MEMIF_MON1;
   volatile uint32 Suspend_AFE_MEMIF_MON2;
   volatile uint32 Suspend_AFE_MEMIF_MON3;
   volatile uint32 Suspend_AFE_MEMIF_MON4;

   volatile uint32 Suspend_AFE_FOC_CON;
   volatile uint32 Suspend_AFE_FOC_CON1;
   volatile uint32 Suspend_AFE_FOC_CON2;
   volatile uint32 Suspend_AFE_FOC_CON3;
   volatile uint32 Suspend_AFE_FOC_CON4;
   volatile uint32 Suspend_AFE_FOC_CON5;
   volatile uint32 Suspend_AFE_MON_STEP;

   volatile uint32 Suspend_AFE_SIDETONE_DEBUG;
   volatile uint32 Suspend_AFE_SIDETONE_MON;
   volatile uint32 Suspend_AFE_SIDETONE_CON0;
   volatile uint32 Suspend_AFE_SIDETONE_COEFF;
   volatile uint32 Suspend_AFE_SIDETONE_CON1;
   volatile uint32 Suspend_AFE_SIDETONE_GAIN;
   volatile uint32 Suspend_AFE_SGEN_CON0;

   volatile uint32 Suspend_AFE_TOP_CON0  ;

   volatile uint32 Suspend_AFE_PREDIS_CON0;
   volatile uint32 Suspend_AFE_PREDIS_CON1;
   volatile uint32 Suspend_AFE_MRG_MON0;
   volatile uint32 Suspend_AFE_MRG_MON1;
   volatile uint32 Suspend_AFE_MRG_MON2;

   volatile uint32 Suspend_AFE_MOD_PCM_BASE;
   volatile uint32 Suspend_AFE_MOD_PCM_END;
   volatile uint32 Suspend_AFE_MOD_PCM_CUR;
   volatile uint32 Suspend_AFE_IRQ_MCU_CON;
   volatile uint32 Suspend_AFE_IRQ_MCU_STATUS;
   volatile uint32 Suspend_AFE_IRQ_CLR;
   volatile uint32 Suspend_AFE_IRQ_MCU_CNT1;
   volatile uint32 Suspend_AFE_IRQ_MCU_CNT2;
   volatile uint32 Suspend_AFE_IRQ_MCU_MON2;
   volatile uint32 Suspend_AFE_IRQ_MCU_CNT5;
   volatile uint32 Suspend_AFE_IRQ1_MCN_CNT_MON;
   volatile uint32 Suspend_AFE_IRQ2_MCN_CNT_MON;
   volatile uint32 Suspend_AFE_IRQ1_MCU_EN_CNT_MON;
   volatile uint32 Suspend_AFE_IRQ5_MCU_EN_CNT_MON;
   volatile uint32 Suspend_AFE_MEMIF_MINLEN;
   volatile uint32 Suspend_AFE_MEMIF_MAXLEN;
   volatile uint32 Suspend_AFE_MEMIF_PBUF_SIZE;

   volatile uint32 Suspend_AFE_GAIN1_CON0;
   volatile uint32 Suspend_AFE_GAIN1_CON1;
   volatile uint32 Suspend_AFE_GAIN1_CON2;
   volatile uint32 Suspend_AFE_GAIN1_CON3;
   volatile uint32 Suspend_AFE_GAIN1_CONN;
   volatile uint32 Suspend_AFE_GAIN1_CUR;
   volatile uint32 Suspend_AFE_GAIN2_CON0;
   volatile uint32 Suspend_AFE_GAIN2_CON1;
   volatile uint32 Suspend_AFE_GAIN2_CON2;
   volatile uint32 Suspend_AFE_GAIN2_CON3;
   volatile uint32 Suspend_AFE_GAIN2_CONN;

   volatile uint32 Suspend_DBG_MON0;
   volatile uint32 Suspend_DBG_MON1;
   volatile uint32 Suspend_DBG_MON2;
   volatile uint32 Suspend_DBG_MON3;
   volatile uint32 Suspend_DBG_MON4;
   volatile uint32 Suspend_DBG_MON5;
   volatile uint32 Suspend_DBG_MON6;
   volatile uint32 Suspend_AFE_ASRC_CON0;
   volatile uint32 Suspend_AFE_ASRC_CON1;
   volatile uint32 Suspend_AFE_ASRC_CON2;
   volatile uint32 Suspend_AFE_ASRC_CON3;
   volatile uint32 Suspend_AFE_ASRC_CON4;
   volatile uint32 Suspend_AFE_ASRC_CON6;
   volatile uint32 Suspend_AFE_ASRC_CON7;
   volatile uint32 Suspend_AFE_ASRC_CON8;
   volatile uint32 Suspend_AFE_ASRC_CON9;
   volatile uint32 Suspend_AFE_ASRC_CON10;
   volatile uint32 Suspend_AFE_ASRC_CON11;
   volatile uint32 Suspend_PCM_INTF_CON1;
   volatile uint32 Suspend_PCM_INTF_CON2;
   volatile uint32 Suspend_PCM2_INTF_CON;
   volatile uint32 Suspend_FOC_ROM_SIG;
}AudAfe_Suspend_Reg;

typedef enum
{
    MEM_DL1,
    MEM_DL2,
    MEM_VUL,
    MEM_DAI,
    MEM_I2S, // Cuurently not used. Add for sync with user space
    MEM_AWB,
    MEM_MOD_DAI,
    NUM_OF_MEM_INTERFACE
}MEMIF_BUFFER_TYPE;


typedef enum
{
    INTERRUPT_IRQ1_MCU = 1,
    INTERRUPT_IRQ2_MCU = 2,
    INTERRUPT_IRQ_MCU_DAI_SET = 4,
    INTERRUPT_IRQ_MCU_DAI_RST = 8
}IRQ_MCU_TYPE;

enum
{
    CLOCK_AUD_AFE =0,
    CLOCK_AUD_I2S,
    CLOCK_AUD_ADC,
    CLOCK_AUD_DAC,
    CLOCK_AUD_LINEIN,
    CLOCK_AUD_HDMI,
    CLOCK_AUD_26M,  // core clock
    CLOCK_TYPE_MAX
};

/*****************************************************************************
 *                       VARIBALE
 *****************************************************************************/

struct fasync_struct  *AudDrv_async = NULL;

//information about
AFE_MEM_CONTROL_T 	 AFE_dL1_Control_context;
AFE_MEM_CONTROL_T 	 AFE_dL2_Control_context;
AFE_MEM_CONTROL_T	 AWB_Control_context;
AFE_MEM_CONTROL_T	 VUL_Control_context;
AFE_MEM_CONTROL_T     DAI_Control_context;
AFE_MEM_CONTROL_T     MODDAI_Control_context;

AudAfe_Suspend_Reg	 Suspend_reg;
SPH_Control			 SPH_Ctrl_State;

// Auddrv_driver_status_counter
bool     Auddrv_First_bootup  = true;
int        Auddrv_Flush_counter  = 0;

// here is temp address for ioremap audio hardware regisster
void* AFE_BASE_ADDRESS = 0;
void* AFE_SRAM_ADDRESS = 0;

#endif


