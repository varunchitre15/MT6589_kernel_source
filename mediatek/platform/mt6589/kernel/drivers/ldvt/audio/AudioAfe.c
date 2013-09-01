/*****************************************************************************
*                E X T E R N A L   R E F E R E N C E S
******************************************************************************
*/
#include "AudioAfe.h"
#include "AudioRom.h"
#include "audio.h"

/*****************************************************************************
*                         D A T A   T Y P E S
******************************************************************************
*/
typedef struct
{
    /*temp register */
    INT32 reg_AUDIO_TOP_CON0;
    INT32 reg_AFE_DAC_CON0;
    INT32 reg_AFE_DAC_CON1;
} AFE_INFO_T;

AFE_INFO_T gAfeInfo;

typedef struct
{
    /*temp register */
    UINT32 reg_AFE_IRQ_CON;
    UINT32 reg_AFE_IRQ_STATUS;
    UINT32 reg_AFE_IRQ_MISS_STATUS;
    UINT32 reg_AFE_IRQ_CLR;
    UINT32 reg_AFE_IRQ_CNT;
} AFE_IRQ_INFO_T;

AFE_IRQ_INFO_T grIRQInfo;

/*****************************************************************************
*                         M A C R O
******************************************************************************
*/

#define VarWriteBits(variable,value,bits,len)\
    {\
        INT32 u4TargetBitField = ((0x1 << len) - 1) << bits;\
        INT32 u4TargetValue = (value << bits) & u4TargetBitField;\
        variable = ((variable & (~u4TargetBitField)) | u4TargetValue);\
    }
#define vSetVarBit(variable,bit) variable =  (variable  | (1<<bit))
#define vResetVarBit(variable,bit) variable = (variable & (~( 1<<bit)))


/*****************************************************************************
*                         G L O B A L    V A R I A B L E S
******************************************************************************
*/

UINT32 u4DramPhyBase = 0;
UINT32 u4DramVirBase = 0;
UINT32 u4DramPhyEnd = 0;
UINT32 u4DramVirEnd = 0;

UINT32 u4SramPhyBase = 0;
UINT32 u4SramVirBase = 0;
UINT32 u4SramPhyEnd = 0;
UINT32 u4SramVirEnd = 0;

UINT32 u4SamplingRate[9]= {AFE_8000HZ,AFE_11025HZ,AFE_12000HZ,AFE_16000HZ,AFE_22050HZ,AFE_24000HZ,AFE_32000HZ,AFE_44100HZ,AFE_48000HZ};
UINT32 u4SamplingRateConvert[9] = {0, 1, 2, 4, 5, 6, 8, 9, 10};
UINT32 u4SamplingRateHz[9] = {8000,11025,12000,16000,22050,24000,32000,44100,48000};

UINT32 u4BufPhyAddr2VirAddr(UINT32 u4PhyBufAddr)
{
    UINT32 u4VirBurAddr;

    if (u4SramPhyBase <= u4PhyBufAddr && u4PhyBufAddr <= u4SramPhyEnd) // Sram
    {
        u4VirBurAddr = u4PhyBufAddr - u4SramPhyBase + u4SramVirBase;
    }
    else // Dram
    {
        u4VirBurAddr = u4PhyBufAddr - u4DramPhyBase + u4DramVirBase;
    }

    return u4VirBurAddr;
}

/******************************************************************************
* Function      : vAudioTopControl
* Description   : vAudioTopControl
* Parameter     :
* Return        : None
******************************************************************************/
void vAudioTopControl(void)
{
    WriteREG(AUDIO_TOP_CON0,0x00004000);
    printk("[vAudioTopControl]: AUDIO_TOP_CON0 after update = 0x%x \n", ReadREG(AUDIO_TOP_CON0));
}

/*######################################################################*/
/*                                                                                                                                            */
/*                                         AFE memory interface                                                                      */
/*                                                                                                                                            */
/*######################################################################*/

/******************************************************************************
* Function      : vAfeTurnOnMemif
* Description   : Memory interface settings and enable path
* Parameter     :
* Return        : None
******************************************************************************/
void vAfeTurnOnMemif(MEMIF_CONFIG_T *memif_config)
{
#if 1
    printk("==============================================\n");
    printk("[vAfeTurnOnMemif]: memInterface path  = 0x%x \n",memif_config->eMemInterface);
    printk("[vAfeTurnOnMemif]: sampling rate = %d \n",u4SamplingRateHz[memif_config->eSamplingRate]);
    printk("[vAfeTurnOnMemif]: Dai sampling rate = 0x%x \n",memif_config->eDaiModSamplingRate);
    printk("[vAfeTurnOnMemif]: Duplicate write = 0x%x \n",memif_config->eDupWrite);
    printk("[vAfeTurnOnMemif]: channel config = 0x%x \n",memif_config->eChannelConfig);
    printk("[vAfeTurnOnMemif]: Base Address = 0x%x \n",memif_config->rBufferSetting.u4AFE_MEMIF_BUF_BASE);
    printk("[vAfeTurnOnMemif]: End Address = 0x%x \n",memif_config->rBufferSetting.u4AFE_MEMIF_BUF_END);
    printk("==============================================\n");
#endif

    UINT32 eSamplingRateConvert = u4SamplingRateConvert[memif_config->eSamplingRate];

    gAfeInfo.reg_AFE_DAC_CON1=ReadREG(AFE_DAC_CON1);
    switch (memif_config->eMemInterface)
    {
        case AFE_MEM_DL1:
            // configurations
            VarWriteBits(gAfeInfo.reg_AFE_DAC_CON1,eSamplingRateConvert,DL1_MODE_POS,DL1_MODE_LEN);
            VarWriteBits(gAfeInfo.reg_AFE_DAC_CON1,memif_config->eChannelConfig,DL1_DATA_POS,DL1_DATA_LEN);

            //Buffer setting
            WriteREG(AFE_DL1_BASE,memif_config->rBufferSetting.u4AFE_MEMIF_BUF_BASE);
            WriteREG(AFE_DL1_END,memif_config->rBufferSetting.u4AFE_MEMIF_BUF_END);

            break;

        case AFE_MEM_DL2:
            // configurations
            VarWriteBits(gAfeInfo.reg_AFE_DAC_CON1,eSamplingRateConvert,DL2_MODE_POS,DL2_MODE_LEN);
            VarWriteBits(gAfeInfo.reg_AFE_DAC_CON1,memif_config->eChannelConfig,DL2_DATA_POS,DL2_DATA_LEN);
            //Buffer setting
            WriteREG(AFE_DL2_BASE,memif_config->rBufferSetting.u4AFE_MEMIF_BUF_BASE);
            WriteREG(AFE_DL2_END,memif_config->rBufferSetting.u4AFE_MEMIF_BUF_END);

            break;

        case AFE_MEM_VUL:
            // configurations
            VarWriteBits(gAfeInfo.reg_AFE_DAC_CON1,eSamplingRateConvert,VUL_MODE_POS,VUL_MODE_LEN);
            VarWriteBits(gAfeInfo.reg_AFE_DAC_CON1,memif_config->eChannelConfig,VUL_DATA_POS,VUL_DATA_LEN);
            if(memif_config->eChannelConfig == AFE_MONO)
            {
                // Set MONO select
                VarWriteBits(gAfeInfo.reg_AFE_DAC_CON1,memif_config->eMonoSelect,VUL_R_MONO_POS,VUL_R_MONO_LEN);
            }
            //Buffer setting
            WriteREG(AFE_VUL_BASE,memif_config->rBufferSetting.u4AFE_MEMIF_BUF_BASE);
            WriteREG(AFE_VUL_END,memif_config->rBufferSetting.u4AFE_MEMIF_BUF_END);
            break;

        case AFE_MEM_DAI:
            VarWriteBits(gAfeInfo.reg_AFE_DAC_CON1,memif_config->eDaiModSamplingRate,DAI_MODE_POS,DAI_MODE_LEN);
            VarWriteBits(gAfeInfo.reg_AFE_DAC_CON1,memif_config->eDupWrite,DAI_DUP_WR_POS,DAI_DUP_WR_LEN);
            //Buffer setting
            WriteREG(AFE_DAI_BASE,memif_config->rBufferSetting.u4AFE_MEMIF_BUF_BASE);
            WriteREG(AFE_DAI_END,memif_config->rBufferSetting.u4AFE_MEMIF_BUF_END);
            break;

        case AFE_MEM_I2S:
            printk("[vAfeTurnOnMemif]: ERR wrong case, I2S should be removed  \n");
            break;

        case AFE_MEM_AWB:
            VarWriteBits(gAfeInfo.reg_AFE_DAC_CON1,eSamplingRateConvert,AWB_MODE_POS,AWB_MODE_LEN);
            VarWriteBits(gAfeInfo.reg_AFE_DAC_CON1,memif_config->eChannelConfig,AWB_DATA_POS,AWB_DATA_LEN);
            if(memif_config->eChannelConfig == AFE_MONO)
            {
                // Set MONO select
                VarWriteBits(gAfeInfo.reg_AFE_DAC_CON1,memif_config->eMonoSelect,AWB_R_MONO_POS,AWB_R_MONO_LEN);
            }
            //Buffer setting
            WriteREG(AFE_AWB_BASE,memif_config->rBufferSetting.u4AFE_MEMIF_BUF_BASE);
            WriteREG(AFE_AWB_END,memif_config->rBufferSetting.u4AFE_MEMIF_BUF_END);

            break;

        case AFE_MEM_MOD_PCM:
            VarWriteBits(gAfeInfo.reg_AFE_DAC_CON1,memif_config->eDaiModSamplingRate,MOD_PCM_MODE_POS,MOD_PCM_MODE_LEN);
            VarWriteBits(gAfeInfo.reg_AFE_DAC_CON1,memif_config->eDupWrite,MOD_PCM_DUP_WR_POS,MOD_PCM_DUP_WR_LEN);
            //Buffer setting
            WriteREG(AFE_MOD_PCM_BASE,memif_config->rBufferSetting.u4AFE_MEMIF_BUF_BASE);
            WriteREG(AFE_MOD_PCM_END,memif_config->rBufferSetting.u4AFE_MEMIF_BUF_END);
            break;

        default:
            printk("[vAfeTurnOnMemif]: ERR wrong case  \n");
            break;
    }

    // Update configurations
    //printk("[vAfeTurnOnMemif]: AFE_DAC_CON1 before update = 0x%x \n",ReadREG(AFE_DAC_CON1));
    WriteREG(AFE_DAC_CON1,gAfeInfo.reg_AFE_DAC_CON1);
    printk("[vAfeTurnOnMemif]: AFE_DAC_CON1 after update = 0x%x \n",ReadREG(AFE_DAC_CON1));


    // Enable target path
    gAfeInfo.reg_AFE_DAC_CON0=ReadREG(AFE_DAC_CON0);
    //printk("[vAfeTurnOnMemif]: AFE_DAC_CON0 before update = 0x%x \n",gAfeInfo.reg_AFE_DAC_CON0);
    vSetVarBit(gAfeInfo.reg_AFE_DAC_CON0,memif_config->eMemInterface);
    WriteREG(AFE_DAC_CON0,gAfeInfo.reg_AFE_DAC_CON0);
    printk("[vAfeTurnOnMemif]: AFE_DAC_CON0 after update = 0x%x \n",ReadREG(AFE_DAC_CON0));
}


/******************************************************************************
* Function      : vAfeGetCurPointer
* Description   : Get current pointer of memory interface
* Parameter     :
* Return        : None
******************************************************************************/
void vAfeGetCurPointer(MEMIF_CONFIG_T *memif_config)
{
    switch (memif_config->eMemInterface)
    {
        case AFE_MEM_DL1:
            memif_config->rBufferSetting.u4AFE_MEMIF_BUF_RP = ReadREG(AFE_DL1_CUR);
            break;
        case AFE_MEM_DL2:
            memif_config->rBufferSetting.u4AFE_MEMIF_BUF_RP = ReadREG(AFE_DL2_CUR);
            break;
        case AFE_MEM_VUL:
            memif_config->rBufferSetting.u4AFE_MEMIF_BUF_WP = ReadREG(AFE_VUL_CUR);
            break;
        case AFE_MEM_DAI:
            memif_config->rBufferSetting.u4AFE_MEMIF_BUF_WP = ReadREG(AFE_DAI_CUR);
            break;
        case AFE_MEM_AWB:
            memif_config->rBufferSetting.u4AFE_MEMIF_BUF_WP = ReadREG(AFE_AWB_CUR);
            break;
        case AFE_MEM_MOD_PCM:
            memif_config->rBufferSetting.u4AFE_MEMIF_BUF_WP = ReadREG(AFE_MOD_PCM_CUR);
            break;
    }
}


/******************************************************************************
* Function      : vTurnOffAfeMemif
* Description   : Disable AFE path
* Parameter     :
* Return        : None
******************************************************************************/
void vAfeTurnOffMemif(MEMIF_CONFIG_T *memif_config)
{
    // Disable target path
    gAfeInfo.reg_AFE_DAC_CON0=ReadREG(AFE_DAC_CON0);
    vResetVarBit(gAfeInfo.reg_AFE_DAC_CON0,memif_config->eMemInterface);
    WriteREG(AFE_DAC_CON0,gAfeInfo.reg_AFE_DAC_CON0);
    printk("[vAfeTurnOffMemif]: AFE_DAC_CON0 after update = 0x%x \n",gAfeInfo.reg_AFE_DAC_CON0);
    //double confirm the register
    printk("[vAfeTurnOffMemif]: AFE_DAC_CON0 register = 0x%x \n",ReadREG(AFE_DAC_CON0));
}


/******************************************************************************
* Function      : vTurnOnAfe
* Description   : Turn ON Afe
* Parameter     :
* Return        : None
******************************************************************************/
void vAfeTurnOn(void)
{
    vRegSetBit(AFE_DAC_CON0, AFE_ON);
    printk("[vAfeTurnOn]: AFE_DAC_CON0 after update = 0x%x \n", ReadREG(AFE_DAC_CON0));
}


/******************************************************************************
* Function      : vTurnOffAfe
* Description   : Turn OFF Afe
* Parameter     :
* Return        : None
******************************************************************************/
void vAfeTurnOff(void)
{
    vRegResetBit(AFE_DAC_CON0, AFE_ON);
    printk("[vAfeTurnOff]: AFE_DAC_CON0 after update = 0x%x \n", ReadREG(AFE_DAC_CON0));
}

/*######################################################################*/
/*                                                                                                                                            */
/*                             Interconnection Related Interfaces                                                                 */
/*                                                                                                                                            */
/*######################################################################*/

/******************************************************************************
* Function      : bConnect
* Description   :
* Parameter    :
* Return         :
******************************************************************************/
BOOL bConnect(ITRCON_IN_T rIn, ITRCON_OUT_T rOut, BOOL bRShift)
{
    INT32 address;
    INT32 sbit;
    INT32 rbit;
    printk("bConnect in[%d], out[%d] \n", rIn, rOut);
    // Check connection capability
    //printk("Check connection capability IntrConCap[%d][%d] :%d \n", rIn, rOut, IntrConCap[rIn][rOut]);
    if(IntrConCap[rIn][rOut] == 0)
    {
        printk("capability check fail \n");
        return FALSE;
    }

    // Check register
    if(IntrConReg[rIn][rOut] == 0xFF)
    {
        printk("Register check fail \n");
        return FALSE;
    }

    //Set S bit
    address = AFE_CONN0 + (IntrConReg[rIn][rOut] >> 4) * 4;
    if(IntrConSBit[rIn][rOut] == -1)
    {
        printk("Sbit check fail \n");
        return FALSE;
    }
    sbit = IntrConSBit[rIn][rOut];
    vRegSetBit(address, sbit);
    printk("Set S add: 0x%x bit: %d \n",address, sbit);

    //Set R bit
    if(bRShift)
    {
        address = AFE_CONN0 + (IntrConReg[rIn][rOut] & 0xF) * 4;
        if(IntrConRBit[rIn][rOut] == -1)
        {
            printk("Rbit check fail");
            return FALSE;
        }
        rbit = IntrConRBit[rIn][rOut];
        vRegSetBit(address, rbit);
        //printk("Set R add: 0x%x bit: %d \n",address, rbit);
    }
    return TRUE;
}


/******************************************************************************
* Function      : bDisConnect
* Description   :
* Parameter    :
* Return         :
******************************************************************************/
BOOL bDisConnect(ITRCON_IN_T in, ITRCON_OUT_T out)
{
    INT32 address;
    INT32 sbit;
    INT32 rbit;
    printk("bDisConnect in[%d], out[%d] \n", in, out);

    // Check connection capability
    printk("Check connection capability IntrConCap[%d][%d] :%d \n", in, out, IntrConCap[in][out]);
    if(IntrConCap[in][out] == 0)
    {
        printk("capability check fail \n");
        return FALSE;
    }

    // Check register
    if(IntrConReg[in][out] == 0x99)
    {
        printk("Register check fail \n");
        return FALSE;
    }
    address = AFE_CONN0+IntrConReg[in][out];

    //Reset Set S bit
    if(IntrConSBit[in][out] == -1)
    {
        printk("Sbit check fail \n");
        return FALSE;
    }
    sbit = IntrConSBit[in][out];
    vRegResetBit(address, sbit);
    printk("Reset S add: 0x%x bit: %d \n",address, sbit);

    //Reset R bit if available
    if(IntrConRBit[in][out] != -1)
    {
        rbit = IntrConRBit[in][out];
        vRegResetBit(address, rbit);
        printk("Reset R add: 0x%x bit: %d \n",address, rbit);
    }


    return TRUE;
}

/******************************************************************************
* Function      : vDisConnectAll
* Description   : clear all interconnection
* Parameter    : none
* Return         : none
******************************************************************************/
void vDisConnectAll(void)
{
    printk("[vDisConnectAll]\n");
    WriteREG(AFE_CONN0, 0x0);
    WriteREG(AFE_CONN1, 0x0);
    WriteREG(AFE_CONN2, 0x0);
    WriteREG(AFE_CONN3, 0x0);
}


/**************************************************************************/
/**                      I2S Related Interfaces                          **/
/**************************************************************************/

// ADC Related
/******************************************************************************
* Function      : vADCI2SEnable
* Description   : Enable ADC I2S
* Parameter     :
* Return        : none
******************************************************************************/
void vADCI2SEnable(I2SEN_T value)
{
    printk("vADCI2SEnable:%d \n", value);
    if ( value == I2S_ENABLE )
    {
        vRegSetBit(AFE_I2S_CON2, AI2S_EN_POS);
    }
    else
    {
        vRegResetBit(AFE_I2S_CON2, AI2S_EN_POS);
    }
    //vRegWriteBits(AFE_I2S_CON2, AI2S_EN_POS, AI2S_EN_LEN, value);
}

/******************************************************************************
* Function      : vADCI2SSet
* Description   : ADC I2S setting
* Parameter     : word len, format, sampling rate, update word, swap LR
*                         note that update word is suggest to be '0'
* Return        : none
******************************************************************************/
void vADCI2SSet( I2SWLEN_T wlen, I2SFMT_T fmt, SAMPLINGRATE_T sr, INT32 update, I2SSWAP_T swap)
{
    INT32 reg_AFE_I2S_CON2=ReadREG(AFE_I2S_CON2);
    update = 8;  // fix this value
    VarWriteBits(reg_AFE_I2S_CON2, wlen, AI2S_WLEN_POS, AI2S_WLEN_LEN);
    VarWriteBits(reg_AFE_I2S_CON2, fmt,  AI2S_FMT_POS, AI2S_FMT_LEN);
    VarWriteBits(reg_AFE_I2S_CON2, u4SamplingRateConvert[sr], AI2S_OUT_MODE_POS, AI2S_OUT_MODE_LEN);
    VarWriteBits(reg_AFE_I2S_CON2, update, AI2S_UPDATE_WORD_POS, AI2S_UPDATE_WORD_LEN);
    VarWriteBits(reg_AFE_I2S_CON2, swap, AI2S_LR_SWAP_POS, AI2S_LR_SWAP_LEN);
    printk("vADCI2SSet, set add: 0x%x, value:0x%x \n", AFE_I2S_CON2, reg_AFE_I2S_CON2);
    WriteREG(AFE_I2S_CON2,reg_AFE_I2S_CON2);
}

// DAC Related
/******************************************************************************
* Function      : vDACI2SEnable
* Description   :
* Parameter     :
* Return        :
******************************************************************************/
void vDACI2SEnable(I2SEN_T value)
{
    vRegWriteBits(AFE_I2S_CON1, value, AI2S_EN_POS, AI2S_EN_LEN);
    printk("[vDACI2SEnable]: AFE_I2S_CON1 after update = 0x%x \n", ReadREG(AFE_I2S_CON1));
}

/******************************************************************************
* Function      : vDACI2SSet
* Description   :
* Parameter     :
* Return        :
******************************************************************************/
void vDACI2SSet( I2SWLEN_T wlen, I2SFMT_T fmt, SAMPLINGRATE_T sr, INT32 update, I2SSWAP_T swap)
{
    INT32 reg_AFE_I2S_CON1=ReadREG(AFE_I2S_CON1);
    VarWriteBits(reg_AFE_I2S_CON1, wlen, AI2S_WLEN_POS, AI2S_WLEN_LEN);
    VarWriteBits(reg_AFE_I2S_CON1, fmt,  AI2S_FMT_POS, AI2S_FMT_LEN);
    VarWriteBits(reg_AFE_I2S_CON1, u4SamplingRateConvert[sr], AI2S_OUT_MODE_POS, AI2S_OUT_MODE_LEN);
    VarWriteBits(reg_AFE_I2S_CON1, update, AI2S_UPDATE_WORD_POS, AI2S_UPDATE_WORD_LEN);
    VarWriteBits(reg_AFE_I2S_CON1, swap, AI2S_LR_SWAP_POS, AI2S_LR_SWAP_LEN);
    WriteREG(AFE_I2S_CON1,reg_AFE_I2S_CON1);
    printk("[vDACI2SSet]: AFE_I2S_CON1 after update = 0x%x \n", ReadREG(AFE_I2S_CON1));
}

// Original I2S
/******************************************************************************
* Function      : vI2SEnable
* Description   : Setting to original I2S
* Parameter     :
* Return        :
******************************************************************************/
void vI2SEnable(I2SEN_T value)
{
    printk("vI2SEnable: %d \n", value);
    if ( value == I2S_ENABLE )
    {
        vRegSetBit(AFE_I2S_CON, I2S_EN_POS);
    }
    else
    {
        vRegResetBit(AFE_I2S_CON, I2S_EN_POS);
    }
    //vRegWriteBits(AFE_I2S_CON, I2S_EN_POS, I2S_EN_LEN, value);
}


/******************************************************************************
* Function      : vI2SSet
* Description   : Setting to original I2S
* Parameter     :
* Return        :
******************************************************************************/
void vI2SSet( I2SWLEN_T wlen, I2SFMT_T fmt, SAMPLINGRATE_T sr, I2SSRC_T src, I2SDIR_T dir)
{
    INT32 reg_AFE_I2S_CON=ReadREG(AFE_I2S_CON);
    INT32 reg_AFE_DAC_CON1=ReadREG(AFE_DAC_CON1);
    printk("vI2SSet, before set add: 0x%x, value:0x%x\n", AFE_I2S_CON, reg_AFE_I2S_CON);
    printk("wlen:%d, fmt:%d, src:%d, sr:%d, dir:%d\n", wlen, fmt, src, u4SamplingRateHz[sr], dir);
    VarWriteBits(reg_AFE_I2S_CON, wlen, I2S_WLEN_POS, I2S_WLEN_LEN);
    VarWriteBits(reg_AFE_I2S_CON, fmt, I2S_FMT_POS, I2S_FMT_LEN);
    VarWriteBits(reg_AFE_I2S_CON, src, I2S_SRC_POS, I2S_SRC_LEN);
    VarWriteBits(reg_AFE_DAC_CON1, u4SamplingRateConvert[sr], I2S_OUT_MODE_POS, I2S_OUT_MODE_LEN);
    VarWriteBits(reg_AFE_I2S_CON, dir, I2S_DIR_POS, I2S_DIR_LEN);
    WriteREG(AFE_I2S_CON,reg_AFE_I2S_CON);
    WriteREG(AFE_DAC_CON1,reg_AFE_DAC_CON1);
    printk("vI2SSet, after set add: 0x%x, value:0x%x\n", AFE_I2S_CON, reg_AFE_I2S_CON);
    printk("add: 0x%x, value:0x%x \n", AFE_DAC_CON1, reg_AFE_DAC_CON1);
}

//FOC (basically we use reset value, we do not implement basic IF
/******************************************************************************
* Function      : vFOCEnable
* Description   : enable FOC
* Parameter     :
* Return        : none
******************************************************************************/
void vFOCEnable(BOOL bEnable)
{
    printk("vFOCEnable: %d \n", bEnable);
    if (!bEnable)   // Disable
    {
        vRegSetBit(AFE_CONN4, 30);  // skip FOC and bypass to I_0, I_1
    }
    else   // Enable
    {
        WriteREG(AFE_FOC_CON3, 0x0034);
        WriteREG(AFE_FOC_CON5, 0x0067); // 0x0067 / 0x019A
        WriteREG(AFE_FOC_CON, 0x5a04);
        vRegSetBit(AFE_FOC_CON, FOC_EN_POS);
    }
}

/**************************************************************************/
/**                      BT PCM Related Interfaces                          **/
/**************************************************************************/
/******************************************************************************
* Function      : vBtPcmEnable
* Description   : Enable BT PCM module
* Parameter     :
* Return        :
******************************************************************************/
void vBtPcmEnable(BTPCMEN_T eValue)
{
    //printk("vBtPcmEnable: %d \n", eValue);
    if ( eValue == BTPCM_ENABLE)
    {
        vRegSetBit(AFE_DAIBT_CON0, BTPCM_EN_POS);
        vRegSetBit(AFE_DAIBT_CON0, DAIBT_EN_POS);
    }
    else
    {
        vRegResetBit(AFE_DAIBT_CON0, DAIBT_EN_POS);
        vRegResetBit(AFE_DAIBT_CON0, BTPCM_EN_POS);
    }

    printk("[vBtPcmEnable]: AFE_DAIBT_CON0 after update = 0x%x \n", ReadREG(AFE_DAIBT_CON0));
}

/******************************************************************************
* Function      : vBtPcmSet
* Description   : BT PCM module setting
* Parameter     :
* Return        :
******************************************************************************/
void vBtPcmSet(BTPCMMODE_T eSr)
{
    vRegWriteBits(AFE_DAIBT_CON0, eSr, DAIBT_MODE_POS, DAIBT_MODE_LEN);
    vRegWriteBits(AFE_DAIBT_CON0, 5, BTPCM_LENGTH_POS, BTPCM_LENGTH_LEN);
    vRegSetBit(AFE_DAIBT_CON0, DAIBT_DATARDY_POS);
    vRegSetBit(AFE_DAIBT_CON0, BTPCM_SYNC_POS);
}

/*######################################################################*/
/*                                                                                                                                            */
/*                                         Reset Functions                                                                      */
/*                                                                                                                                            */
/*######################################################################*/
/******************************************************************************
* Function      : vStrongResetAFE
* Description   : reset all registers
* Parameter     : None
* Return        : None
******************************************************************************/
void vStrongResetAFE(void)
{
    INT32 i;
    INT32 offset = AUDIO_TOP_CON0;

    printk("[vStrongResetAFE] reset all registers\n");

    WriteREG(FPGA_CFG0, 0x00001007);    // hopping 32m, MCLK : 3.072M
    WriteREG(FPGA_CFG1, 0x00000000);
    for(i = 0; i < TBL_SZ_RESET; i++)
    {
        WriteREG(offset, resetTable[i]);
        offset += 4;
    }

    // clean sram / dram
    memset((void*)u4SramVirBase, 0, AFE_INTERNAL_SRAM_SIZE);
    memset((void*)u4DramVirBase, 0, AFE_EXTERNAL_DRAM_SIZE);
}

/******************************************************************************
* Function      : vSoftResetAFE
* Description   : reset MEMIF, Interconnection registers
* Parameter     : None
* Return        : None
******************************************************************************/
void vSoftResetAFE(void)
{
    printk("[vSoftResetAFE] reset MEMIF, Interconnection Registers only \n");
    //WriteREG(AFE_DAC_CON0, 0x0);
    vAfeTurnOff();
    vDisConnectAll();
}

/******************************************************************************
* Function      : vAfeSineWaveGenCfg
* Description   : Generate HW sine tone
* Parameter     : rSineTone
* Return        : None
******************************************************************************/
void vAfeSineWaveGenCfg(AFE_SINEGEN_INFO_T *pSineTone)
{
    UINT32 u4Reg = 0;
    VarWriteBits(u4Reg, pSineTone->u4ch1_freq_div, SINE_TONE_FREQ_DIV_CH1, 5);
    VarWriteBits(u4Reg, pSineTone->rch1_amp_div, SINE_TONE_AMP_DIV_CH1, 3);
    VarWriteBits(u4Reg, pSineTone->rch1_sine_mode, SINE_TONE_MODE_CH1, 4);
    VarWriteBits(u4Reg, pSineTone->u4ch2_freq_div, SINE_TONE_FREQ_DIV_CH2, 5);
    VarWriteBits(u4Reg, pSineTone->rch2_amp_div, SINE_TONE_AMP_DIV_CH2, 3);
    VarWriteBits(u4Reg, pSineTone->rch2_sine_mode, SINE_TONE_MODE_CH2, 4);
    vResetVarBit(u4Reg, SINE_TONE_MUTE_CH1);
    vResetVarBit(u4Reg, SINE_TONE_MUTE_CH2);
    //vSetVarBit(u4Reg, SINE_TONE_ENABLE);
    VarWriteBits(u4Reg, pSineTone->rloopback_mode, SINE_TONE_LOOPBACK_MOD, 4);
    WriteREG(AFE_SGEN_CON0, u4Reg);
    printk("[vAfeSineWaveGenCfg]: AFE_SGEN_CON0 after update = 0x%x \n", u4Reg);
}

/******************************************************************************
* Function      : vAfeSineWaveGenEnable
* Description   : Disable HW sine tone
* Parameter     : None
* Return        : None
******************************************************************************/
void vAfeSineWaveGenEnable(BOOL bEnable)
{
    vRegWriteBits(AFE_SGEN_CON0, bEnable, SINE_TONE_ENABLE, 1);
    printk("[vAfeSineWaveGenEnable]: AFE_SGEN_CON0 after update = 0x%x \n", ReadREG(AFE_SGEN_CON0));
}

/******************************************************************************
* Function      : vAfeSineWaveMute
* Description   : Mute HW sine tone for specified channel
* Parameter     : eCh (ch1, ch2 or both)
* Return        : None
******************************************************************************/
void vAfeSineWaveMute(SINE_TONE_CH_T eCh)
{
    UINT32 u4Reg = 0;
    u4Reg = ReadREG(AFE_SGEN_CON0);
    if (eCh == SINE_TONE_CH1)
        vSetVarBit(u4Reg, SINE_TONE_MUTE_CH1);
    else if (eCh == SINE_TONE_CH2)
        vSetVarBit(u4Reg, SINE_TONE_MUTE_CH2);
    else
    {
        vSetVarBit(u4Reg, SINE_TONE_MUTE_CH1);
        vSetVarBit(u4Reg, SINE_TONE_MUTE_CH2);
    }
    WriteREG(AFE_SGEN_CON0, u4Reg);
    printk("[vAfeSineWaveMute]: AFE_SGEN_CON0 after update = 0x%x \n", u4Reg);
}

/*######################################################################*/
/*                                                                                                                                            */
/*                                         F P G A    C O N F I G                                                                      */
/*                                                                                                                                            */
/*######################################################################*/
/******************************************************************************
* Function      : vFpgaModemDaiSwitch
* Description   : fpga swith to modem output or dai output
* Parameter    : modem or not
* Return         : None
******************************************************************************/
void vFpgaModemDaiSwitch(BOOL switchModem)
{
    if ( switchModem )
    {
        vRegResetBit(FPGA_CFG1, CODEC_SEL_POS);
    }
    else
    {
        vRegSetBit(FPGA_CFG1, CODEC_SEL_POS);
    }
}

/******************************************************************************
* Function      : vAfeGetATime
* Description   : get time (unit based on audio clock (26M))
* Parameter     : u4delaytime (ms)
* Return        : None
******************************************************************************/
UINT32 u4AfeGetATime(void)
{
    UINT32 u4time;

    u4time = ReadREG(FPGA_AUDIO_CLOCK);
    return u4time;
}

/******************************************************************************
* Function      : vAfeGetATime
* Description   : get time (unit based on audio clock (26M/32M/48M))
* Parameter     : u4StartTime
* Return        : Difference time
******************************************************************************/
UINT32 u4AfeGetATimeDiff(UINT32 u4StartTime)
{
    UINT32 u4time;

    u4time = ReadREG(FPGA_AUDIO_CLOCK);
    if (u4time >= u4StartTime)
    {
        u4time -= u4StartTime;
    }
    else
    {
        u4time += ((0xffffffff - u4StartTime) + 1);
    }
    
    return u4time;
}

void vFpgaAudioClockDelaySec(UINT32 u4Second)
{
    UINT32 u4StartClock = u4AfeGetATime();

    UINT32 u4TargetDiffClock; 
    switch(u4RegReadBits(FPGA_CFG0, HOP26M_SEL_POS, HOP26M_SEL_LEN))
    {
        case 0:
            u4TargetDiffClock = u4Second * 26000000;
            break;
        case 1:
            u4TargetDiffClock = u4Second * 32000000;
            break;
        case 2:
            u4TargetDiffClock = u4Second * 48000000;
            break;
        default:
            u4TargetDiffClock = u4Second * 26000000;
            break;
    }
        
    UINT32 u4PreviousClock = u4AfeGetATime();
    while(u4AfeGetATimeDiff(u4StartClock) < u4TargetDiffClock) 
    {        
        // if has stopped in CVD -> stop wait
        if (u4AfeGetATimeDiff(u4PreviousClock) > 0x200000) break;

        u4PreviousClock = u4AfeGetATime();
    }
}

void vFpgaAudioClockDelaymSec(UINT32 u4mSecond)
{
    UINT32 u4StartClock = u4AfeGetATime();

    UINT32 u4TargetDiffClock; 
    switch(u4RegReadBits(FPGA_CFG0, HOP26M_SEL_POS, HOP26M_SEL_LEN))
    {
        case 0:
            u4TargetDiffClock = u4mSecond * 26000;
            break;
        case 1:
            u4TargetDiffClock = u4mSecond * 32000;
            break;
        case 2:
            u4TargetDiffClock = u4mSecond * 48000;
            break;
        default:
            u4TargetDiffClock = u4mSecond * 26000;
            break;
    }
        
    UINT32 u4PreviousClock = u4AfeGetATime();
    while(u4AfeGetATimeDiff(u4StartClock) < u4TargetDiffClock) 
    {        
        // if has stopped in CVD -> stop wait
        if (u4AfeGetATimeDiff(u4PreviousClock) > 0x200000) break;

        u4PreviousClock = u4AfeGetATime();
    }
}

