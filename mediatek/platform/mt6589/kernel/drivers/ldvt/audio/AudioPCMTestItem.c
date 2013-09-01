#include "AudioAfe.h"
#include "AudioRom.h"
#include "audio.h"

#define MOD_PCM_BASE (u4SramPhyBase)
#define MOD_PCM_SIZE (AFE_INTERNAL_SRAM_SIZE)
#define MOD_PCM_END  (MOD_PCM_BASE+MOD_PCM_SIZE-1)

//#define MOD_PCM_OFF_ON_TEST

// functions that could be commenly used
void vSetASRC(PCMMODE_T ePcm8k16kmode)
{
    printk("[vSetASRC]: PCMMODE_T = %x \n", ePcm8k16kmode);

    if ( ePcm8k16kmode == PCM_8K)
    {
        WriteREG(AFE_ASRC_CON1, 0x00001964);
        WriteREG(AFE_ASRC_CON2, 0x00400000);
        WriteREG(AFE_ASRC_CON3, 0x00400000);
        WriteREG(AFE_ASRC_CON4, 0x00001964);
        WriteREG(AFE_ASRC_CON7, 0x00000CB2);

    }
    else if ( ePcm8k16kmode == PCM_16K )
    {
        WriteREG(AFE_ASRC_CON1, 0x00000cb2);
        WriteREG(AFE_ASRC_CON2, 0x00400000);
        WriteREG(AFE_ASRC_CON3, 0x00400000);
        WriteREG(AFE_ASRC_CON4, 0x00000cb2);
        WriteREG(AFE_ASRC_CON7, 0x00000659);
    }
}

void vASRCEnable(bool bEnable)
{
    printk("[vASRCEnable]: bEnable = %x \n", bEnable);

    if ( bEnable == TRUE)
    {
        WriteREG(AFE_ASRC_CON6, 0x0001183F); //0x0001188F
        WriteREG(AFE_ASRC_CON0, 0x06003031);
    }
    else
    {
        WriteREG(AFE_ASRC_CON6, 0x00000000);
        WriteREG(AFE_ASRC_CON0, 0x06003030);
    }
}

void vSetModPCM(PCM_MODULE ePcmModule, PCM_INFO_T *pPcmInfo)
{


    /// Mod PCM config
    if (ePcmModule == PCM_1)
    {
        /// clean
        WriteREG(PCM_INTF_CON, 0);

        vRegWriteBits(PCM_INTF_CON,  pPcmInfo->ePcmFmt,        PCM_FMT_POS, PCM_FMT_LEN);
        vRegWriteBits(PCM_INTF_CON,  pPcmInfo->ePcm8k16kmode,  PCM_MODE_POS, PCM_MODE_LEN);
        vRegWriteBits(PCM_INTF_CON,  pPcmInfo->ePcmWlen,       PCM_WLEN_POS, PCM_WLEN_LEN);
        vRegWriteBits(PCM_INTF_CON,  pPcmInfo->ePcmClkSrc,     PCM_SLAVE_POS, PCM_SLAVE_LEN);
        vRegWriteBits(PCM_INTF_CON,  pPcmInfo->ePcmBypassASRC, PCM_BYP_ASRC_POS, PCM_BYP_ASRC_LEN);
        vRegWriteBits(PCM_INTF_CON,  pPcmInfo->ePcmModemSel,   PCM_EXT_MODEM_POS, PCM_EXT_MODEM_LEN);
        vRegWriteBits(PCM_INTF_CON,  pPcmInfo->ePcmVbt16kSel,  PCM_VBT16K_MODE_POS, 1);
    }
    else
    {
        /// clean
        WriteREG(PCM_INTF_CON2, 0);

        vRegWriteBits(PCM_INTF_CON2, pPcmInfo->ePcmFmt,        PCM_FMT_POS, PCM_FMT_LEN);
        vRegWriteBits(PCM_INTF_CON2, pPcmInfo->ePcm8k16kmode,  PCM_MODE_POS, PCM_MODE_LEN);
        vRegWriteBits(PCM_INTF_CON2, pPcmInfo->ePcmWlen,       PCM_WLEN_POS, PCM_WLEN_LEN);
        vRegWriteBits(PCM_INTF_CON2, pPcmInfo->ePcmVbt16kSel,  12, 1);
    }
}

void vModPCMEnable(PCM_MODULE ePcmModule, PCMEN_T bEnable)
{
    if (ePcmModule == PCM_1)
    {
        vRegWriteBits(PCM_INTF_CON,  bEnable, PCM_EN_POS, PCM_EN_LEN);
    }
    else
    {
        vRegWriteBits(PCM_INTF_CON2, bEnable, PCM_EN_POS, PCM_EN_LEN);
    }
}

void vModPcmConfigAndRun(PCM_MODULE ePcmModule, PCM_INFO_T *pPcmInfo)
{
    MEMIF_CONFIG_T memModDaiCfg;
    SAMPLINGRATE_T eSampleRate;

    /// reset
    vStrongResetAFE();

    /// power on
    vAudioTopControl(); // AUDIO_TOP_CON0[14] = 1

    /// config FPGA
    WriteREG(FPGA_CFG0, 0x00000007);    // hopping 26m, MCLK : 3.072M
    if (ePcmModule == PCM_1 && pPcmInfo->ePcmModemSel == PCM_INT_MD)
    {
        vRegSetBit(FPGA_CFG1, 14);
    }

    /// get sample rate info
    eSampleRate = (pPcmInfo->ePcm8k16kmode == PCM_8K) ? AFE_8000HZ : AFE_16000HZ;

#if 1
    /// Sgen config as Tx data
    AFE_SINEGEN_INFO_T rSineTone;
    rSineTone.u4ch1_freq_div = 1;
    rSineTone.rch1_amp_div = SINE_TONE_1;
    rSineTone.rch1_sine_mode = eSampleRate;
    rSineTone.u4ch2_freq_div = 2;
    rSineTone.rch2_amp_div = SINE_TONE_1;
    rSineTone.rch2_sine_mode = eSampleRate;
    rSineTone.rloopback_mode = SINE_TONE_LOOPBACK_O7_O8; // O17_O18 simultaneously
    vAfeSineWaveGenCfg(&rSineTone);
#else
    bConnect(I05, O07, FALSE);
    bConnect(I06, O08, FALSE);

    MEMIF_CONFIG_T memDL1Cfg;
    memset(&memDL1Cfg, 0, sizeof(MEMIF_CONFIG_T));
    UINT32 addr;
    for(addr=u4DramVirBase; addr<=u4DramVirBase+0x3FFF; addr+=4)
    {
        *(UINT32*) addr = 0x66660000;
    }
    memDL1Cfg.eMemInterface = AFE_MEM_DL1;
    memDL1Cfg.eDaiModSamplingRate = eSampleRate;
    memDL1Cfg.eChannelConfig = AFE_STEREO;
    memDL1Cfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE = u4DramPhyBase;
    memDL1Cfg.rBufferSetting.u4AFE_MEMIF_BUF_END = u4DramPhyBase + 0x3FFF;

    vAfeTurnOnMemif(&memDL1Cfg);

    /// Sgen config as Rx data
    AFE_SINEGEN_INFO_T rSineTone;
    rSineTone.u4ch1_freq_div = 1;
    rSineTone.rch1_amp_div = SINE_TONE_1;
    rSineTone.rch1_sine_mode = eSampleRate;
    rSineTone.u4ch2_freq_div = 0;
    rSineTone.rch2_amp_div = SINE_TONE_1;
    rSineTone.rch2_sine_mode = eSampleRate;
    rSineTone.rloopback_mode = SINE_TONE_LOOPBACK_O0_O1;
    vAfeSineWaveGenCfg(&rSineTone);

    vI2SSet(I2S_16BIT, I2S_I2S, eSampleRate, I2S_MASTER, I2S_OUT);
    vI2SEnable(I2S_ENABLE);

    vRegWriteBits(PCM_INTF_CON, 1,  26, 1);
#endif

    /// ASRC
    if (pPcmInfo->ePcmClkSrc==PCM_SLAVE && pPcmInfo->ePcmBypassASRC==PCM_GO_ASRC)
    {
        vSetASRC(pPcmInfo->ePcm8k16kmode);
        vASRCEnable(TRUE);
    }

    /// Config PCM
    vSetModPCM(ePcmModule, pPcmInfo);
    vModPCMEnable(ePcmModule, PCM_ENABLE);

    /// Memory Interface

    // Dump Rx data to MOD_DAI_Data (O_12)
    if (ePcmModule == PCM_1)
    {
        bConnect(I09, O012, false);
    }
    else
    {
        //bConnect(I14, O012, false);
        vRegSetBit(AFE_CONN4, 12);
    }

    // Memory (O_12)
    memset(&memModDaiCfg, 0, sizeof(MEMIF_CONFIG_T));
    memModDaiCfg.eMemInterface = AFE_MEM_MOD_PCM;
    memModDaiCfg.eDaiModSamplingRate = pPcmInfo->ePcm8k16kmode;
    memModDaiCfg.eDupWrite = AFE_DUP_WR_DISABLE;
    memModDaiCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE = MOD_PCM_BASE;
    memModDaiCfg.rBufferSetting.u4AFE_MEMIF_BUF_END = MOD_PCM_END;
    vAfeTurnOnMemif(&memModDaiCfg);
    vRegResetBit(AFE_DAC_CON0, MOD_PCM_ON);

#ifdef MOD_PCM_OFF_ON_TEST
    UINT32 u4PcmOffAddr = MOD_PCM_BASE + (MOD_PCM_SIZE / 4);
    UINT32 u4PcmOnAddr  = MOD_PCM_END  - (MOD_PCM_SIZE / 4) + 1;
    printk("MOD_PCM_OFF_ON_TEST: u4PcmOffAddr=0x%x, u4PcmOnAddr=0x%x\n", u4PcmOffAddr, u4PcmOnAddr);
#endif

    /// turn on
    vAfeTurnOn();
    vAfeSineWaveGenEnable(true);

    /// wait for 10 sec - observe frequency and amplitude
    vFpgaAudioClockDelaySec(10);

    /// wait
    vRegSetBit(AFE_DAC_CON0, MOD_PCM_ON);

#ifdef MOD_PCM_OFF_ON_TEST
    while( ReadREG(AFE_MOD_PCM_CUR) <= u4PcmOffAddr);
    vModPCMEnable(ePcmModule, PCM_DISABLE);

    while( ReadREG(AFE_MOD_PCM_CUR) <= u4PcmOnAddr);
    vModPCMEnable(ePcmModule, PCM_ENABLE);
#endif

    while( ReadREG(AFE_MOD_PCM_CUR) <= (ReadREG(AFE_MOD_PCM_END)-0x100));
    vRegResetBit(AFE_DAC_CON0, MOD_PCM_ON);


    /// turn off
    vAfeSineWaveGenEnable(false);

    vModPCMEnable(ePcmModule, PCM_DISABLE);
    vASRCEnable(FALSE);

    vRegResetBit(AFE_DAC_CON0, AFE_ON);

    /// wait for 10 sec - dump memory
    printk("Please config the next item\n");
    vFpgaAudioClockDelaySec(10);
}

BOOL fgModPcmVbtConfigAndRun(PCM_MODULE ePcmModule, PCM_INFO_T *pPcmInfo)
{
    BOOL retval;
    
    MEMIF_CONFIG_T memModDaiCfg;
    SAMPLINGRATE_T eSampleRate;

    /// reset
    vStrongResetAFE();

    /// power on
    vAudioTopControl(); // AUDIO_TOP_CON0[14] = 1

    /// config FPGA
    vFpgaModemDaiSwitch(FALSE);      // do pin (4~10) select to DAI
    if (ePcmModule == PCM_1 && pPcmInfo->ePcmModemSel == PCM_INT_MD)
    {
        vRegSetBit(FPGA_CFG1, 14);
    }

    /// get sample rate info
    eSampleRate = (pPcmInfo->ePcm8k16kmode == PCM_8K) ? AFE_8000HZ : AFE_16000HZ;

    /// Sgen config as Tx data
    AFE_SINEGEN_INFO_T rSineTone;
    rSineTone.u4ch1_freq_div = 1;
    rSineTone.rch1_amp_div = SINE_TONE_1;
    rSineTone.rch1_sine_mode = eSampleRate;
    rSineTone.u4ch2_freq_div = 2;
    rSineTone.rch2_amp_div = SINE_TONE_1;
    rSineTone.rch2_sine_mode = eSampleRate;
    rSineTone.rloopback_mode = SINE_TONE_LOOPBACK_O7_O8; // O17_O18 simultaneously
    vAfeSineWaveGenCfg(&rSineTone);

    /// ASRC
    if (pPcmInfo->ePcmClkSrc==PCM_SLAVE && pPcmInfo->ePcmBypassASRC==PCM_GO_ASRC)
    {
        vSetASRC(pPcmInfo->ePcm8k16kmode);
        vASRCEnable(TRUE);
    }

    /// Config PCM
    vSetModPCM(ePcmModule, pPcmInfo);
    vModPCMEnable(ePcmModule, PCM_ENABLE);

    /// Config DAI
    bConnect(I02, O02, false);  // Loopback
#if 0
    bConnect(I02, O011, false);
    MEMIF_CONFIG_T memDaiCfg;
    memset(&memDaiCfg, 0, sizeof(MEMIF_CONFIG_T));

    memDaiCfg.eMemInterface = AFE_MEM_DAI;
    memDaiCfg.eChannelConfig = AFE_MONO;
    memDaiCfg.eDupWrite = AFE_DUP_WR_DISABLE;
    memDaiCfg.eDaiModSamplingRate = AFE_DAIMOD_16000HZ;
    memDaiCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE = 0x80100000; //TODO
    memDaiCfg.rBufferSetting.u4AFE_MEMIF_BUF_END = 0x80103FFF;  //TODO
    vAfeTurnOnMemif(&memDaiCfg);
#endif

    vBtPcmSet(BTPCM_16K);
    vBtPcmEnable(BTPCM_ENABLE);

    /// Dump Rx data to MOD_DAI_Data (O_12)
    if (ePcmModule == PCM_1)
    {
        bConnect(I09, O012, false);
    }
    else
    {
        //bConnect(I14, O012, false);
        vRegSetBit(AFE_CONN4, 12);
    }

    /// Memory Interface
    memset(&memModDaiCfg, 0, sizeof(MEMIF_CONFIG_T));
    memModDaiCfg.eMemInterface = AFE_MEM_MOD_PCM;
    memModDaiCfg.eDaiModSamplingRate = pPcmInfo->ePcm8k16kmode;
    memModDaiCfg.eDupWrite = AFE_DUP_WR_ENABLE;
    memModDaiCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE = u4DramPhyBase;
    memModDaiCfg.rBufferSetting.u4AFE_MEMIF_BUF_END = u4DramPhyEnd;
    vAfeTurnOnMemif(&memModDaiCfg);

    /// turn on
    vAfeTurnOn();
    vAfeSineWaveGenEnable(true);

    /// wait
    while( ReadREG(AFE_MOD_PCM_CUR) <= (ReadREG(AFE_MOD_PCM_END)-0x100));
    vRegResetBit(AFE_DAC_CON0, MOD_PCM_ON);

    /// turn off
    vAfeSineWaveGenEnable(false);

    vModPCMEnable(ePcmModule, PCM_DISABLE);
    vASRCEnable(FALSE);

    vBtPcmEnable(BTPCM_ENABLE);

    vRegResetBit(AFE_DAC_CON0, AFE_ON);

    if (check_bit_true(ReadREG(AFE_MOD_PCM_BASE),ReadREG(AFE_MOD_PCM_END),table_sgen_golden_values_ch1_duplicate,64))
    {
        printk("VBT 16K Mode Pass\n\n");
        retval= TRUE;
    }
    else
    {
        printk("VBT 16K Mode Fail\n\n");
        retval= FALSE;
    }

    return retval;
}


void vModPcm1ExtMasterMode()
{
    PCM_INFO_T rPcmInfo;
    memset(&rPcmInfo, 0, sizeof(PCM_INFO_T));

    rPcmInfo.ePcmModemSel = PCM_EXT_MD;
    rPcmInfo.ePcmClkSrc = PCM_MASTER;

    for(rPcmInfo.ePcm8k16kmode=PCM_8K; rPcmInfo.ePcm8k16kmode<=PCM_16K; rPcmInfo.ePcm8k16kmode++)   // 8/16k PCM
    {
        for(rPcmInfo.ePcmWlen=PCM_16BIT; rPcmInfo.ePcmWlen<=PCM_32BIT; rPcmInfo.ePcmWlen++)   // 32/64 BCK cycles
        {
            for(rPcmInfo.ePcmFmt=PCM_I2S; rPcmInfo.ePcmFmt<=PCM_MODEB; rPcmInfo.ePcmFmt++)   // I2S, EIAJ, Mode A, Mode B
            {
                printk("==================================================================\n");
                printk("==================================================================\n");
                printk("[vModPcm1ExtMasterMode] PCM_MODE=%d, PCM_WLEN=%d, PCM_FMT=%d\n",rPcmInfo.ePcm8k16kmode, rPcmInfo.ePcmWlen, rPcmInfo.ePcmFmt);
                vModPcmConfigAndRun(PCM_1, &rPcmInfo);
            }
        }
    }
}



void vModPcm1ExtSlaveModeASRC()
{
    PCM_INFO_T rPcmInfo;
    memset(&rPcmInfo, 0, sizeof(PCM_INFO_T));

    rPcmInfo.ePcmModemSel = PCM_EXT_MD;

    rPcmInfo.ePcmClkSrc = PCM_SLAVE;
    rPcmInfo.ePcmBypassASRC = PCM_GO_ASRC;
    rPcmInfo.ePcmVbt16kSel = PCM_VBT_16K_MODE_DISABLE;

    for(rPcmInfo.ePcm8k16kmode=PCM_8K; rPcmInfo.ePcm8k16kmode<=PCM_16K; rPcmInfo.ePcm8k16kmode++)   // 8/16k PCM
    {
        for(rPcmInfo.ePcmWlen=PCM_16BIT; rPcmInfo.ePcmWlen<=PCM_32BIT; rPcmInfo.ePcmWlen++)   // 32/64 BCK cycles
        {
            for(rPcmInfo.ePcmFmt=PCM_I2S; rPcmInfo.ePcmFmt<=PCM_MODEB; rPcmInfo.ePcmFmt++)   // I2S, EIAJ, Mode A, Mode B
            {
                printk("==================================================================\n");
                printk("==================================================================\n");
                printk("[vModPcm1ExtSlaveModeASRC] PCM_MODE=%d, PCM_WLEN=%d, PCM_FMT=%d\n",rPcmInfo.ePcm8k16kmode, rPcmInfo.ePcmWlen, rPcmInfo.ePcmFmt);
                vModPcmConfigAndRun(PCM_1, &rPcmInfo);
            }
        }
    }
}



void vModPcm1ExtSlaveModeAsyncFIFO()
{
    PCM_INFO_T rPcmInfo;
    memset(&rPcmInfo, 0, sizeof(PCM_INFO_T));

    rPcmInfo.ePcmModemSel = PCM_EXT_MD;

    rPcmInfo.ePcmClkSrc = PCM_SLAVE;
    rPcmInfo.ePcmBypassASRC = PCM_GO_ASYNC_FIFO;
    rPcmInfo.ePcmVbt16kSel = PCM_VBT_16K_MODE_DISABLE;

    for(rPcmInfo.ePcm8k16kmode=PCM_8K; rPcmInfo.ePcm8k16kmode<=PCM_16K; rPcmInfo.ePcm8k16kmode++)   // 8/16k PCM
    {
        for(rPcmInfo.ePcmWlen=PCM_16BIT; rPcmInfo.ePcmWlen<=PCM_32BIT; rPcmInfo.ePcmWlen++)   // 32/64 BCK cycles
        {
            for(rPcmInfo.ePcmFmt=PCM_I2S; rPcmInfo.ePcmFmt<=PCM_MODEB; rPcmInfo.ePcmFmt++)   // I2S, EIAJ, Mode A, Mode B
            {
                printk("==================================================================\n");
                printk("==================================================================\n");
                printk("[vModPcm1ExtSlaveModeAsyncFIFO] PCM_MODE=%d, PCM_WLEN=%d, PCM_FMT=%d\n",rPcmInfo.ePcm8k16kmode, rPcmInfo.ePcmWlen, rPcmInfo.ePcmFmt);
                vModPcmConfigAndRun(PCM_1, &rPcmInfo);
            }
        }
    }
}

void vModPcm1IntSlaveModeAsyncFIFO()
{
    PCM_INFO_T rPcmInfo;
    memset(&rPcmInfo, 0, sizeof(PCM_INFO_T));

    rPcmInfo.ePcmModemSel = PCM_INT_MD;

    rPcmInfo.ePcmClkSrc = PCM_SLAVE;
    rPcmInfo.ePcmBypassASRC = PCM_GO_ASYNC_FIFO;
    rPcmInfo.ePcmVbt16kSel = PCM_VBT_16K_MODE_DISABLE;

    rPcmInfo.ePcm8k16kmode=PCM_8K;
    rPcmInfo.ePcmWlen=PCM_16BIT;
    rPcmInfo.ePcmFmt=PCM_I2S;

    printk("==================================================================\n");
    printk("==================================================================\n");
    printk("[vModPcm1IntSlaveModeAsyncFIFO] PCM_MODE=%d, PCM_WLEN=%d, PCM_FMT=%d\n",rPcmInfo.ePcm8k16kmode, rPcmInfo.ePcmWlen, rPcmInfo.ePcmFmt);
    vModPcmConfigAndRun(PCM_1, &rPcmInfo);
}

void vModPcm2AsyncFIFO()
{
    PCM_INFO_T rPcmInfo;
    memset(&rPcmInfo, 0, sizeof(PCM_INFO_T));

    rPcmInfo.ePcmVbt16kSel = PCM_VBT_16K_MODE_DISABLE;

    for(rPcmInfo.ePcm8k16kmode=PCM_8K; rPcmInfo.ePcm8k16kmode<=PCM_16K; rPcmInfo.ePcm8k16kmode++)   // 8/16k PCM
    {
        for(rPcmInfo.ePcmWlen=PCM_16BIT; rPcmInfo.ePcmWlen<=PCM_32BIT; rPcmInfo.ePcmWlen++)   // 32/64 BCK cycles
        {
            for(rPcmInfo.ePcmFmt=PCM_I2S; rPcmInfo.ePcmFmt<=PCM_MODEB; rPcmInfo.ePcmFmt++)   // I2S, EIAJ, Mode A, Mode B
            {
                printk("==================================================================\n");
                printk("==================================================================\n");
                printk("[vModPcm2AsyncFIFO] PCM_MODE=%d, PCM_WLEN=%d, PCM_FMT=%d\n",rPcmInfo.ePcm8k16kmode, rPcmInfo.ePcmWlen, rPcmInfo.ePcmFmt);
                vModPcmConfigAndRun(PCM_2, &rPcmInfo);
            }
        }
    }
}

BOOL fgModPcmVbt16kMode()
{
    BOOL retval = TRUE;
    
    PCM_MODULE ePcmModule;

    PCM_INFO_T rPcmInfo;
    memset(&rPcmInfo, 0, sizeof(PCM_INFO_T));

    rPcmInfo.ePcmModemSel = PCM_INT_MD;

    rPcmInfo.ePcmClkSrc = PCM_SLAVE;
    rPcmInfo.ePcmBypassASRC = PCM_GO_ASYNC_FIFO;
    rPcmInfo.ePcmVbt16kSel = PCM_VBT_16K_MODE_ENABLE;

    rPcmInfo.ePcm8k16kmode=PCM_16K;
    rPcmInfo.ePcmWlen=PCM_16BIT;
    rPcmInfo.ePcmFmt=PCM_MODEB;

    for(ePcmModule=PCM_1; ePcmModule<=PCM_2; ePcmModule++)
    {
        printk("==================================================================\n");
        printk("==================================================================\n");
        printk("[fgModPcmVbt16kMode (PCM%d)] PCM_MODE=%d, PCM_WLEN=%d, PCM_FMT=%d\n",ePcmModule+1, rPcmInfo.ePcm8k16kmode, rPcmInfo.ePcmWlen, rPcmInfo.ePcmFmt);
        retval &= fgModPcmVbtConfigAndRun(ePcmModule, &rPcmInfo);
    }

    return retval;
}

