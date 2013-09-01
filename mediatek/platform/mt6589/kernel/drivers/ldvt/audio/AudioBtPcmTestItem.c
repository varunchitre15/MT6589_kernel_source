#include "AudioAfe.h"
#include "AudioRom.h"
#include "AudioI2STestItem.h"
#include "audio.h"

#define PCM_BUF_BASE (u4DramPhyBase)
#define PCM_BUF_SIZE (0x1000)
#define PCM_BUF_END  (PCM_BUF_BASE+PCM_BUF_SIZE-1)

#define MRG_I2S_BUF_BASE (u4SramPhyBase)
#define MRG_I2S_BUF_SIZE (AFE_INTERNAL_SRAM_SIZE)
#define MRG_I2S_BUF_END  (MRG_I2S_BUF_BASE+MRG_I2S_BUF_SIZE-1)

#define I2S_IN_BUF_BASE (PCM_BUF_END+1)
#define I2S_IN_BUF_SIZE (MRG_I2S_BUF_SIZE)
#define I2S_IN_BUF_END  (I2S_IN_BUF_BASE+I2S_IN_BUF_SIZE-1)

#define SYNC_DELAY 0

/// Merge Interface
vMergeInterfaceFrameSyncSet(SAMPLINGRATE_T eMergeI2sInFs)
{
    printk("[vMergeInterfaceFrameSyncSet] fs = %d\n", u4SamplingRateHz[eMergeI2sInFs]);

    // set sample rate
    vRegWriteBits(AFE_I2S_CON3, u4SamplingRateConvert[eMergeI2sInFs], 20, 4);

    // set sync delay
    vRegWriteBits(AFE_I2S_CON3, SYNC_DELAY, 8, 4);

    printk("[vMergeInterfaceFrameSyncSet]: AFE_I2S_CON3 after update = 0x%x \n", ReadREG(AFE_I2S_CON3));
}

vMergeInterfaceEnable(bool bMergeIfOn)
{
    vRegWriteBits(AFE_I2S_CON3, bMergeIfOn, 0, 1);

    if (!bMergeIfOn)
        vRegWriteBits(AFE_I2S_CON3, 0, 20, 4);

    printk("[vMergeInterfaceEnable] AFE_I2S_CON3 after update = 0x%x \n", ReadREG(AFE_I2S_CON3));
}


/// DAI/BT
void vBtPcmLoopSet(MEMIF_CONFIG_T *memDaiCfg, BTPCMMODE_T eBtFs)
{
    printk("[vBtPcmLoopSet] btpcm fs = %d (0=8k, 1=16k)\n");

    // do pin select to DAI
    vFpgaModemDaiSwitch(FALSE);

    // interconnection setting
    bConnect(I02, O011, false);

    memset(memDaiCfg, 0, sizeof(MEMIF_CONFIG_T));
    memDaiCfg->eMemInterface = AFE_MEM_DAI;
    memDaiCfg->eDaiModSamplingRate = eBtFs;
    memDaiCfg->eChannelConfig = AFE_MONO;
    memDaiCfg->eDupWrite = AFE_DUP_WR_ENABLE;
    memDaiCfg->rBufferSetting.u4AFE_MEMIF_BUF_BASE = PCM_BUF_BASE;
    memDaiCfg->rBufferSetting.u4AFE_MEMIF_BUF_END = PCM_BUF_END;
    vAfeTurnOnMemif(memDaiCfg);

    // set sampling rate
    vRegWriteBits(AFE_DAIBT_CON0, 1, 12, 1);    // for Merge IF
    vBtPcmSet(eBtFs);
}

/// Merge I2S In
void vMergeI2sInLoopSet(MEMIF_CONFIG_T *memI2sCfg, SAMPLINGRATE_T eI2sSr)
{
    printk("[vMergeI2sInLoopSet] merge I2s In fs = %d\n", u4SamplingRateHz[eI2sSr]);

    // interconnection setting
    //WriteREG(AFE_CONN4, 0x140); for AWB

    //bConnect(I015, O09, false);
    //bConnect(I016, O010, false);
    vRegSetBit(AFE_CONN4, 10);
    vRegSetBit(AFE_CONN4, 11);

    memset(memI2sCfg, 0, sizeof(MEMIF_CONFIG_T));
    memI2sCfg->eMemInterface = AFE_MEM_VUL;//AFE_MEM_AWB;
    memI2sCfg->eSamplingRate = eI2sSr;
    memI2sCfg->eChannelConfig = AFE_STEREO;
    memI2sCfg->rBufferSetting.u4AFE_MEMIF_BUF_BASE = MRG_I2S_BUF_BASE;
    memI2sCfg->rBufferSetting.u4AFE_MEMIF_BUF_END = MRG_I2S_BUF_END;
    vAfeTurnOnMemif(memI2sCfg);
}

void vMergeI2sInEnable(bool bMergeI2SInOn)
{
    // set Merge I2S In On/Off
    vRegWriteBits(AFE_I2S_CON3, bMergeI2SInOn, 16, 1);

    printk("[vMergeI2sInLoopEnable]: AFE_I2S_CON3 after update = 0x%x \n", ReadREG(AFE_I2S_CON3));
}

// Test Case implementation
/******************************************************************************
* Function      : bTc42_PcmTxRx
* Description   : Test Case 4-2 implementation
* Parameter     :
* Return        : None
******************************************************************************/

const UINT32 u4DefaultMergeI2sFs = AFE_44100HZ;

BOOL fgMergeIfPcmOnly()
{
    // reset
    vStrongResetAFE();

    // power on
    vAudioTopControl();

    // Merge IF setting
    vMergeInterfaceFrameSyncSet(u4DefaultMergeI2sFs);
    vMergeInterfaceEnable(true);

    // DAI/BT setting (fix 8k)
    MEMIF_CONFIG_T memDaiCfg;
    vBtPcmLoopSet(&memDaiCfg, BTPCM_8K);
    vBtPcmEnable(BTPCM_ENABLE);

#if 0
    /// AWB
    UINT32 u4DL1Base = u4BufPhyAddr2VirAddr(I2S_IN_BUF_BASE);
    UINT32 u4DL1End  = u4BufPhyAddr2VirAddr(I2S_IN_BUF_END);

    WriteREG(AFE_DL1_BASE, I2S_IN_BUF_BASE);
    WriteREG(AFE_DL1_END, I2S_IN_BUF_END);
    UINT32 addr;
    for(addr=u4DL1Base; addr<u4DL1End; addr+=2)
    {
        *(UINT16 *) addr = 0x00FF;
    }

    bConnect(I05, O02, false);

    WriteREG(AFE_DAC_CON0, 0x00000052);
    WriteREG(AFE_DAC_CON1, 0x2000A00A);
#else
    // Sgen config (fix 8k)
    AFE_SINEGEN_INFO_T rSineTone;
    rSineTone.u4ch1_freq_div = 1;
    rSineTone.rch1_amp_div = SINE_TONE_1;
    rSineTone.rch1_sine_mode = AFE_8000HZ;
    rSineTone.u4ch2_freq_div = 1;
    rSineTone.rch2_amp_div = SINE_TONE_1;
    rSineTone.rch2_sine_mode = AFE_8000HZ;
    rSineTone.rloopback_mode = SINE_TONE_LOOPBACK_O2;
    vAfeSineWaveGenCfg(&rSineTone);
#endif

    // turn on
    vAfeTurnOn();
    vAfeSineWaveGenEnable(true);

    // wait
    while( ReadREG(AFE_DAI_CUR) <= (ReadREG(AFE_DAI_END)-0x100) );
    vRegResetBit(AFE_DAC_CON0, DAI_ON);

    // turn off
    vAfeSineWaveGenEnable(false);

    vBtPcmEnable(BTPCM_DISABLE);
    vMergeInterfaceEnable(false);

    vRegResetBit(AFE_DAC_CON0, AFE_ON);

    return check_bit_true(ReadREG(AFE_DAI_BASE),ReadREG(AFE_DAI_END),table_sgen_golden_values_ch1_duplicate,64);
}

void vMergeIfI2sOnly(SAMPLINGRATE_T eSampleRate)
{
    // reset
    vStrongResetAFE();

    // power on
    vAudioTopControl();

    // Merge IF setting
    vMergeInterfaceFrameSyncSet(eSampleRate);
    vMergeInterfaceEnable(true);

    // Merge I2S In setting
    MEMIF_CONFIG_T memI2sCfg;
    vMergeI2sInLoopSet(&memI2sCfg, eSampleRate);
    vMergeI2sInEnable(true);

    // turn on
    vAfeTurnOn();

    // wait
    while( ReadREG(AFE_VUL_CUR) <= (ReadREG(AFE_VUL_END) - 0x100) );
    vRegResetBit(AFE_DAC_CON0, VUL_ON);

    // turn off
    vMergeI2sInEnable(false);
    vMergeInterfaceEnable(false);

    vRegResetBit(AFE_DAC_CON0, AFE_ON);
}



void vMergeIfPcmWithI2s(SAMPLINGRATE_T eSampleRate)
{
    // reset
    vStrongResetAFE();

    // power on
    vAudioTopControl();


    // Merge IF setting
    vMergeInterfaceFrameSyncSet(eSampleRate);
    vMergeInterfaceEnable(true);

    // Merge I2S In setting
    MEMIF_CONFIG_T memI2sCfg;
    vMergeI2sInLoopSet(&memI2sCfg, eSampleRate);
    vMergeI2sInEnable(true);


    // DAI/BT setting (fix 8k)
    MEMIF_CONFIG_T memDaiCfg;
    vBtPcmLoopSet(&memDaiCfg, BTPCM_8K);
    vBtPcmEnable(BTPCM_ENABLE);

    // Sgen config (fix 8k)
    AFE_SINEGEN_INFO_T rSineTone;
    rSineTone.u4ch1_freq_div = 1;
    rSineTone.rch1_amp_div = SINE_TONE_1;
    rSineTone.rch1_sine_mode = AFE_8000HZ;
    rSineTone.u4ch2_freq_div = 1;
    rSineTone.rch2_amp_div = SINE_TONE_1;
    rSineTone.rch2_sine_mode = AFE_8000HZ;
    rSineTone.rloopback_mode = SINE_TONE_LOOPBACK_O2;
    vAfeSineWaveGenCfg(&rSineTone);

    // turn on
    vAfeTurnOn();
    vAfeSineWaveGenEnable(true);

    // wait
#if 1
    while( ReadREG(AFE_VUL_CUR) <= (ReadREG(AFE_VUL_END)-0x100)
            && ReadREG(AFE_DAI_CUR) <= (ReadREG(AFE_DAI_END)-0x100) );
#else //pcm stress test
    UINT32 u4WriteAddr;
    UINT32 u4ReadAddr = ReadREG(AFE_DAI_BASE);

    BOOL bIsPass = TRUE;
    BOOL bStarted = FALSE;        ;

    printk("Waiting for 1st data ...\n\n");
    while(!bStarted)   // get rid of 0
    {
        u4WriteAddr = ReadREG(AFE_DAI_CUR);
        for(; u4ReadAddr<=u4WriteAddr; u4ReadAddr+=4)
        {
            if (ReadREG(u4ReadAddr) != 0)
            {
                bStarted = TRUE;
                break;
            }
        }
    }
    printk("Start from 0x%x\n", u4ReadAddr);

    printk("Stress Testing!! Stop only when fail ...\n\n");
    do
    {
        u4WriteAddr = ReadREG(AFE_DAI_CUR);

        if (u4ReadAddr == u4WriteAddr)
        {
            continue;
        }
        else if(u4ReadAddr < u4WriteAddr)
        {
            bIsPass &= check_bit_true_stress(u4ReadAddr, u4WriteAddr, table_sgen_golden_values_ch1_duplicate, 64);
            u4ReadAddr = u4WriteAddr;
        }
        else
        {
            bIsPass &= check_bit_true_stress(u4ReadAddr, ReadREG(AFE_DAI_END), table_sgen_golden_values_ch1_duplicate, 64);
            bIsPass &= check_bit_true_stress(ReadREG(AFE_DAI_BASE), u4WriteAddr, table_sgen_golden_values_ch1_duplicate, 64);
            u4ReadAddr = u4WriteAddr;
        }
    }
    while(bIsPass);
    printk("FAIL!!!!!\n\n");
#endif

    vRegResetBit(AFE_DAC_CON0, VUL_ON);
    vRegResetBit(AFE_DAC_CON0, DAI_ON);

    // wait for 10 sec - dump memory, observe frame sync and bck
    //vFpgaAudioClockDelaySec(10);

    // turn off
    vAfeSineWaveGenEnable(false);

    vMergeI2sInEnable(false);
    vBtPcmEnable(BTPCM_DISABLE);
    vMergeInterfaceEnable(false);

    vRegResetBit(AFE_DAC_CON0, AFE_ON);

    // check PCM
    if (check_bit_true(ReadREG(AFE_DAI_BASE),ReadREG(AFE_DAI_END),table_sgen_golden_values_ch1_duplicate,64))
    {
        printk("PCM Pass\n");
    }
    else
    {
        printk("PCM Fail\n");
    }
}

void vMergeIfI2sOnlyChangeFs()
{
    // reset
    vStrongResetAFE();

    // power on
    vAudioTopControl();

    // afe turn on
    vAfeTurnOn();

    SAMPLINGRATE_T eSampleRate;
    for(eSampleRate=AFE_32000HZ; eSampleRate<=AFE_48000HZ; eSampleRate++)
    {
        // wait for 10 sec - reconfig mt6628
        printk("[vMergeIfI2sOnlyChangeFs] please dump current sample & re-config mt6628 %dHz.\n", u4SamplingRateHz[eSampleRate]);
        vFpgaAudioClockDelaySec(10);

        // Merge IF setting
        vMergeInterfaceFrameSyncSet(eSampleRate);
        vMergeInterfaceEnable(true);

        // Merge I2S In setting
        MEMIF_CONFIG_T memI2sCfg;
        vMergeI2sInLoopSet(&memI2sCfg, eSampleRate);
        vMergeI2sInEnable(true);

        // wait
        while( ReadREG(AFE_VUL_CUR) <= ReadREG(AFE_VUL_END) - 0x100);
        vRegResetBit(AFE_DAC_CON0, VUL_ON);

        // turn off
        vMergeI2sInEnable(false);
        vMergeInterfaceEnable(false);
    }

    // afe turn off
    vRegResetBit(AFE_DAC_CON0, AFE_ON);
}


void vMergeIfI2sFirstPcmLater()
{
    UINT32 u4PauseAddress;

    // reset
    vStrongResetAFE();

    // power on
    vAudioTopControl();

    // Merge IF setting
    vMergeInterfaceFrameSyncSet(u4DefaultMergeI2sFs);
    vMergeInterfaceEnable(true);

    // Merge I2S In setting
    MEMIF_CONFIG_T memI2sCfg;
    vMergeI2sInLoopSet(&memI2sCfg, u4DefaultMergeI2sFs);
    vMergeI2sInEnable(true);
    u4PauseAddress = MRG_I2S_BUF_BASE + (MRG_I2S_BUF_SIZE >> 2);
    printk("u4PauseAddress = 0x%x\n", u4PauseAddress);

    // Sgen config (fix 8k)
    AFE_SINEGEN_INFO_T rSineTone;
    rSineTone.u4ch1_freq_div = 1;
    rSineTone.rch1_amp_div = SINE_TONE_1;
    rSineTone.rch1_sine_mode = AFE_8000HZ;
    rSineTone.u4ch2_freq_div = 1;
    rSineTone.rch2_amp_div = SINE_TONE_1;
    rSineTone.rch2_sine_mode = AFE_8000HZ;
    rSineTone.rloopback_mode = SINE_TONE_LOOPBACK_O2;
    vAfeSineWaveGenCfg(&rSineTone);

    // turn on (Current I2S Only)
    vAfeTurnOn();

    // Get I2S Data until 1/4 buffer filled
    while( ReadREG(AFE_VUL_CUR) <= u4PauseAddress);

    // Add PCM (DAI/BT setting (fix 8k))
    MEMIF_CONFIG_T memDaiCfg;
    vBtPcmLoopSet(&memDaiCfg, BTPCM_8K);
    vBtPcmEnable(BTPCM_ENABLE);

    vAfeSineWaveGenEnable(true);

    // continue on
    while( ReadREG(AFE_VUL_CUR) <= (ReadREG(AFE_VUL_END)-0x100)
            && ReadREG(AFE_DAI_CUR) <= (ReadREG(AFE_DAI_END)-0x100) );
    vRegResetBit(AFE_DAC_CON0, VUL_ON);
    vRegResetBit(AFE_DAC_CON0, DAI_ON);

    // turn off
    vAfeSineWaveGenEnable(false);

    vMergeI2sInEnable(false);
    vBtPcmEnable(BTPCM_DISABLE);
    vMergeInterfaceEnable(false);

    vRegResetBit(AFE_DAC_CON0, AFE_ON);

    if (check_bit_true(ReadREG(AFE_DAI_BASE),ReadREG(AFE_DAI_END),table_sgen_golden_values_ch1_duplicate,64))
    {
        printk("PCM Pass\n");
    }
    else
    {
        printk("PCM Fail\n");
    }
}

void vMergeIfPcmFirstI2sLater()
{
    UINT32 u4PauseAddress;

    // reset
    vStrongResetAFE();

    // power on
    vAudioTopControl();

    // Merge IF setting
    vMergeInterfaceFrameSyncSet(u4DefaultMergeI2sFs);
    vMergeInterfaceEnable(true);

    // DAI/BT setting (fix 8k)
    MEMIF_CONFIG_T memDaiCfg;
    vBtPcmLoopSet(&memDaiCfg, BTPCM_8K);
    vBtPcmEnable(BTPCM_ENABLE);

    u4PauseAddress = PCM_BUF_BASE + (PCM_BUF_SIZE >> 2);
    printk("u4PauseAddress = 0x%x\n", u4PauseAddress);

    // Sgen config (fix 8k)
    AFE_SINEGEN_INFO_T rSineTone;
    rSineTone.u4ch1_freq_div = 1;
    rSineTone.rch1_amp_div = SINE_TONE_1;
    rSineTone.rch1_sine_mode = AFE_8000HZ;
    rSineTone.u4ch2_freq_div = 1;
    rSineTone.rch2_amp_div = SINE_TONE_1;
    rSineTone.rch2_sine_mode = AFE_8000HZ;
    rSineTone.rloopback_mode = SINE_TONE_LOOPBACK_O2;
    vAfeSineWaveGenCfg(&rSineTone);

    // turn on (Current PCM Only)
    vAfeTurnOn();
    vAfeSineWaveGenEnable(true);

    // Get I2S Data until 1/4 buffer filled
    while( ReadREG(AFE_DAI_CUR) <= u4PauseAddress);

    // Add Merge I2S In setting
    MEMIF_CONFIG_T memI2sCfg;
    vMergeI2sInLoopSet(&memI2sCfg, u4DefaultMergeI2sFs);
    vMergeI2sInEnable(true);

    // continue on
    while( ReadREG(AFE_VUL_CUR) <= (ReadREG(AFE_VUL_END)-0x100)
            && ReadREG(AFE_DAI_CUR) <= (ReadREG(AFE_DAI_END)-0x100) );
    vRegResetBit(AFE_DAC_CON0, VUL_ON);
    vRegResetBit(AFE_DAC_CON0, DAI_ON);

    // turn off
    vAfeSineWaveGenEnable(false);

    vMergeI2sInEnable(false);
    vBtPcmEnable(BTPCM_DISABLE);
    vMergeInterfaceEnable(false);

    vRegResetBit(AFE_DAC_CON0, AFE_ON);

    if (check_bit_true(ReadREG(AFE_DAI_BASE),ReadREG(AFE_DAI_END),table_sgen_golden_values_ch1_duplicate,64))
    {
        printk("PCM Pass\n");
    }
    else
    {
        printk("PCM Fail\n");
    }
}

void vMergeIfWithI2sIn(I2SSRC_T bIsSlave)
{
    const SAMPLINGRATE_T eSampleRateI2sIn = AFE_48000HZ;
    const SAMPLINGRATE_T eSampleRateMrgIf = AFE_44100HZ;

    // reset
    vStrongResetAFE();

    // power on
    vAudioTopControl();

    // Merge IF setting
    vMergeInterfaceFrameSyncSet(eSampleRateMrgIf);
    vMergeInterfaceEnable(true);

    // Merge I2S In setting
    MEMIF_CONFIG_T memMrgI2sCfg;
    vMergeI2sInLoopSet(&memMrgI2sCfg, eSampleRateMrgIf);
    vMergeI2sInEnable(true);

    // DAI/BT setting (fix 8k)
    MEMIF_CONFIG_T memDaiCfg;
    vBtPcmLoopSet(&memDaiCfg, BTPCM_8K);
    vBtPcmEnable(BTPCM_ENABLE);

    /// I2S In
    MEMIF_CONFIG_T memI2sInCfg;
    memI2sInCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE = I2S_IN_BUF_BASE;
    memI2sInCfg.rBufferSetting.u4AFE_MEMIF_BUF_END = I2S_IN_BUF_END;
    if(bIsSlave)  //slave
    {
        vI2sInToDacOn(&memI2sInCfg, eSampleRateI2sIn, TRUE, I2S_SLAVE);
    }
    else
    {
        vI2sInToDacOn(&memI2sInCfg, eSampleRateI2sIn, FALSE, I2S_MASTER);
    }

    // Sgen config (fix 8k)
    AFE_SINEGEN_INFO_T rSineTone;
    rSineTone.u4ch1_freq_div = 1;
    rSineTone.rch1_amp_div = SINE_TONE_1;
    rSineTone.rch1_sine_mode = AFE_8000HZ;
    rSineTone.u4ch2_freq_div = 1;
    rSineTone.rch2_amp_div = SINE_TONE_1;
    rSineTone.rch2_sine_mode = AFE_8000HZ;
    rSineTone.rloopback_mode = SINE_TONE_LOOPBACK_O2;
    vAfeSineWaveGenCfg(&rSineTone);

    // turn on
    vAfeTurnOn();
    vAfeSineWaveGenEnable(true);

    // wait
    while( ReadREG(AFE_VUL_CUR) <= (ReadREG(AFE_VUL_END)-0x100)
            && ReadREG(AFE_DAI_CUR) <= (ReadREG(AFE_DAI_END)-0x100)
            && ReadREG(AFE_AWB_CUR) <= (ReadREG(AFE_AWB_END)-0x100) );
    vRegResetBit(AFE_DAC_CON0, VUL_ON);
    vRegResetBit(AFE_DAC_CON0, DAI_ON);
    vRegResetBit(AFE_DAC_CON0, AWB_ON);

    // turn off
    vAfeSineWaveGenEnable(false);

    vMergeI2sInEnable(false);
    vBtPcmEnable(BTPCM_DISABLE);
    vMergeInterfaceEnable(false);

    vI2SEnable(I2S_DISABLE);
    vDACI2SEnable(I2S_DISABLE);

    vRegResetBit(AFE_DAC_CON0, AFE_ON);

    if (check_bit_true(ReadREG(AFE_DAI_BASE),ReadREG(AFE_DAI_END),table_sgen_golden_values_ch1_duplicate,64))
    {
        printk("PCM Pass\n");
    }
    else
    {
        printk("PCM Fail\n");
    }
}

