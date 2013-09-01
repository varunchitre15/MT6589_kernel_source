#include "AudioAfe.h"
#include "AudioRom.h"
#include "audio.h"

#if 0
BOOL bDL1toI2sDacOutOn(SAMPLINGRATE_T eSampleRate)
{
    INT32 smapleCount = TBL_SZ_48KHz_1K;
    MEMIF_CONFIG_T memCfg;
    printk("[bDL1toI2sDacOutOn] SampleRate:%d\n", u4SamplingRateHz[eSampleRate]);

    // put sine table into memory
    memcpy((void*)u4DramVirBase, tone1k_48kHz_ST, smapleCount*2*2);

    // set interconnection
    bConnect(I05, O03, false);
    bConnect(I06, O04, false);

    // set MEMIF
    memset(&memCfg, 0, sizeof(MEMIF_CONFIG_T));
    memCfg.eMemInterface = AFE_MEM_DL1;
    memCfg.eSamplingRate = eSampleRate;
    memCfg.eChannelConfig = AFE_STEREO;
    memCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE = u4SramPhyBase;
    memCfg.rBufferSetting.u4AFE_MEMIF_BUF_END = u4SramPhyBase + (smapleCount * 2 * 2) - 1;
    vAfeTurnOnMemif(&memCfg);

    // set I2S
    vDACI2SSet(I2S_16BIT, I2S_I2S, eSampleRate, 0, I2S_NOSWAP);
    vDACI2SEnable(I2S_ENABLE);

    return TRUE;
}
#endif

/******************************************************************************
* Function      : vI2sInToDacOn
* Description   : Playback through I2S through External DAC
* Parameter   :
* Return        : None
******************************************************************************/
BOOL vI2sInToDacOn(MEMIF_CONFIG_T *memCfg,SAMPLINGRATE_T eSampleRate, BOOL useFOC, I2SSRC_T inSrc)
{
    printk("[vI2sInToDacOn] fs:%d, useFOC:%d, inSrc:%d\n" , u4SamplingRateHz[eSampleRate], useFOC, inSrc);

    // set interconnection
    bConnect(I00, O03, false);
    bConnect(I01, O04, false);

    bConnect(I00, O05, false);
    bConnect(I01, O06, false);

    // set I2S In
    vI2SSet(I2S_16BIT, I2S_I2S, eSampleRate, inSrc, I2S_IN);
    vI2SEnable(I2S_ENABLE);
    if (inSrc == I2S_SLAVE) //Slave
    {
        vFOCEnable(useFOC);
    }
    else //Master (Enable I/O pin output for MCKL, BCK)
    {
        WriteREG(FPGA_CFG1, 0x00000100);    //I2S In
    }

    // set I2S DAC output
    vDACI2SSet(I2S_16BIT, I2S_I2S, eSampleRate, 0, I2S_NOSWAP);
    vDACI2SEnable(I2S_ENABLE);

    // set MEMIF
    memCfg->eMemInterface = AFE_MEM_AWB;
    memCfg->eSamplingRate = eSampleRate;
    memCfg->eChannelConfig = AFE_STEREO;
    vAfeTurnOnMemif(memCfg);

    return TRUE;
}

// Test Case implementation
/******************************************************************************
* Function      : vDacI2sOut
* Description   : Test Case 3-1 implementation
* Parameter     :
* Return        : None
******************************************************************************/
void vDacI2sOut(I2SWLEN_T i2s_wlen)
{
    SAMPLINGRATE_T eSampleRate;

    // reset
    vStrongResetAFE();

    // power on
    vAudioTopControl();

    for (eSampleRate=AFE_8000HZ; eSampleRate<=AFE_48000HZ; eSampleRate++)
    {
        printk("===============================================================\n");
        printk("I2S Out playing, fs = %d, i2s_wlen = %d (0:16bit,1:20bit)\n", u4SamplingRateHz[eSampleRate], i2s_wlen);

        // Set Sgen
        AFE_SINEGEN_INFO_T rSineTone;
        rSineTone.u4ch1_freq_div = 1;
        rSineTone.rch1_amp_div = SINE_TONE_1;
        rSineTone.rch1_sine_mode = eSampleRate;
        rSineTone.u4ch2_freq_div = 2;
        rSineTone.rch2_amp_div = SINE_TONE_1;
        rSineTone.rch2_sine_mode = eSampleRate;
        rSineTone.rloopback_mode = SINE_TONE_LOOPBACK_O3_O4;
        vAfeSineWaveGenCfg(&rSineTone);

        // Set I2S Out
        vDACI2SSet(i2s_wlen, I2S_I2S, eSampleRate, 0, I2S_NOSWAP);
        vDACI2SEnable(I2S_ENABLE);

        // turn on
        vAfeTurnOn();
        vAfeSineWaveGenEnable(true);

        // wait for 10 sec - for recording DAC output wave
        vFpgaAudioClockDelaySec(10);

        // turn off
        vAfeSineWaveGenEnable(false);
        vDACI2SEnable(I2S_DISABLE);
        vAfeTurnOff();
    }
}

/******************************************************************************
* Function      : vI2sIn
* Description   : Test Case 3-3 implementation
* Parameter     :
* Return        : None
******************************************************************************/
void vI2sIn(I2SSRC_T bIsSlave, BOOL useFOC)
{
    SAMPLINGRATE_T eSampleRate;
    MEMIF_CONFIG_T memCfg;
    memset(&memCfg, 0, sizeof(MEMIF_CONFIG_T));
    int dummy;

    // reset
    vStrongResetAFE();

    // power on
    vAudioTopControl();

    for(eSampleRate=AFE_8000HZ; eSampleRate<=AFE_48000HZ; eSampleRate++)
    {
        printk("===============================================================\n");
        printk("I2S In (Slave), fs = %d, useFOC = %d\n", u4SamplingRateHz[eSampleRate], useFOC);

        // buffer address
        memCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE = u4SramPhyBase;
        memCfg.rBufferSetting.u4AFE_MEMIF_BUF_END = u4SramPhyEnd;

        // setup I2S
        vI2sInToDacOn(&memCfg, eSampleRate, useFOC, bIsSlave);
        vRegResetBit(AFE_DAC_CON0, AWB_ON);

        // turn on
        vAfeTurnOn();

        // wait for 10 sec - observe THD+N
        vFpgaAudioClockDelaySec(10);

        // wait loop
        vRegSetBit(AFE_DAC_CON0, AWB_ON);
        while(ReadREG(AFE_AWB_CUR) <= (ReadREG(AFE_AWB_END) - 0x100));
        vRegResetBit(AFE_DAC_CON0, AWB_ON);

        // turn off
        vI2SEnable(I2S_DISABLE);
        vDACI2SEnable(I2S_DISABLE);
        vAfeTurnOff();

        // wait for 10 sec - dump memory & reconfig external ADC (WM8904 or APWIN)
        vFpgaAudioClockDelaySec(10); 
    }
}

