#include "AudioAfe.h"
#include "AudioRom.h"
#include "audio.h"
#include "AudioCommon.h"

#ifdef LDVT
#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>
#endif

int TestCount=8;

UINT32 uTargetGain1[8]= {0x80000,0x40000,0x20000,0x10000,0x0,0x10000,0x40000,0x80000};
//UINT32 uTargetGain1[8]= {0x20000,0x00000,0x20000,0x00000,0x80000,0x00000,0x80000,0x00000};

UINT32 uSamplePerStep1[8]= {200,100,50,20,10,100,200,255};
//UINT32 uSamplePerStep1[8]= {200,50,10,255,10,100,200,255};

UINT32 uGainDownStep1[3]= {0x7c5e5,0x7e2f3,0x78bca};	//the first step is 0.25dB for default; 0.125dB; 0.5dB
UINT32 uGainUpStep1[3]= {0x83bcd,0x81de6,0x8779a};	//the first step is 0.25dB for default; 0.125dB; 0.5dB


UINT32 uTargetGain2[8]= {0x80000,0x40000,0x10000,0x500,0x0,0x10000,0x30000,0x50000};
UINT32 uSamplePerStep2[8]= {200,100,50,20,10,100,200,255};

UINT32 u4SramVirBuffer;
UINT32 u4SramPhyBuffer;
UINT32 u4DramVirBuffer;
UINT32 u4DramPhyBuffer;

//static const kal_uint32 table_sgen_golden_values[64] =
const UINT32 table_sgen_golden_values_1[64] =
{
    0x0FE50FE5, 0x285E1C44, 0x3F4A285E, 0x53C73414,
    0x650C3F4A, 0x726F49E3, 0x7B6C53C7, 0x7FAB5CDC,
    0x7F02650C, 0x79776C43, 0x6F42726F, 0x60C67781,
    0x4E917B6C, 0x39587E27, 0x21EB7FAB, 0x09307FF4,
    0xF01A7F02, 0xD7A17CD6, 0xC0B67977, 0xAC3874ED,
    0x9AF36F42, 0x8D906884, 0x849360C6, 0x80545818,
    0x80FD4E91, 0x86884449, 0x90BD3958, 0x9F3A2DDA,
    0xB16E21EB, 0xC6A715A8, 0xDE140930, 0xF6CFFCA1,
    0x0FE5F01A, 0x285EE3BB, 0x3F4AD7A1, 0x53C7CBEB,
    0x650CC0B6, 0x726FB61C, 0x7B6CAC38, 0x7FABA323,
    0x7F029AF3, 0x797793BC, 0x6F428D90, 0x60C6887E,
    0x4E918493, 0x395881D8, 0x21EB8054, 0x0930800B,
    0xF01A80FD, 0xD7A18329, 0xC0B68688, 0xAC388B12,
    0x9AF390BD, 0x8D90977B, 0x84939F3A, 0x8054A7E7,
    0x80FDB16E, 0x8688BBB6, 0x90BDC6A7, 0x9F3AD225,
    0xB16EDE14, 0xC6A7EA57, 0xDE14F6CF, 0xF6CF035E
};


/*
static const kal_uint32 table_sgen_golden_values[64] =
{
    0x7FFF8000, 0x7FFF8000, 0x7FFF8000, 0x7FFF8000,
    0x7FFF8000, 0x7FFF8000, 0x7FFF8000, 0x7FFF8000,
    0x7FFF8000, 0x7FFF8000, 0x7FFF8000, 0x7FFF8000,
    0x7FFF8000, 0x7FFF8000, 0x7FFF8000, 0x7FFF8000,
    0x7FFF8000, 0x7FFF8000, 0x7FFF8000, 0x7FFF8000,
    0x7FFF8000, 0x7FFF8000, 0x7FFF8000, 0x7FFF8000,
    0x7FFF8000, 0x7FFF8000, 0x7FFF8000, 0x7FFF8000,
    0x7FFF8000, 0x7FFF8000, 0x7FFF8000, 0x7FFF8000,
    0x7FFF8000, 0x7FFF8000, 0x7FFF8000, 0x7FFF8000,
    0x7FFF8000, 0x7FFF8000, 0x7FFF8000, 0x7FFF8000,
    0x7FFF8000, 0x7FFF8000, 0x7FFF8000, 0x7FFF8000,
    0x7FFF8000, 0x7FFF8000, 0x7FFF8000, 0x7FFF8000,
    0x7FFF8000, 0x7FFF8000, 0x7FFF8000, 0x7FFF8000,
    0x7FFF8000, 0x7FFF8000, 0x7FFF8000, 0x7FFF8000,
    0x7FFF8000, 0x7FFF8000, 0x7FFF8000, 0x7FFF8000,
    0x7FFF8000, 0x7FFF8000, 0x7FFF8000, 0x7FFF8000
};
*/



/******************************************************************************
* Function      : vAudioHWGain1Test
* Description   : Test Case on HW gain1 function
* Parameter     :
* Return        : None
******************************************************************************/
BOOL vAudioHWGain1Test()
{
	BOOL ret = TRUE;

    MEMIF_CONFIG_T rAWBMemCfg,rDL1MemCfg;
	//need to change different sample rate to test
	SAMPLINGRATE_T srCnt = AFE_8000HZ;

	printk("[vAudioHWGain1Test] start\n");

	for(srCnt=AFE_8000HZ; srCnt<=AFE_48000HZ; srCnt++)	//could hard code to set specific sample rate
	{    
		memset(&rAWBMemCfg, 0, sizeof(MEMIF_CONFIG_T));
		memset(&rDL1MemCfg, 0, sizeof(MEMIF_CONFIG_T));
		// reset
		vStrongResetAFE();
		// power on
		vAudioTopControl();

		//set interconnection
		vRegSetBit(AFE_GAIN1_CONN, 16);	//I05 <-->O13	DL1_data in to HW GAIN1
		vRegSetBit(AFE_GAIN1_CONN, 21);	//I06 <-->O14
		
		vRegSetBit(AFE_GAIN1_CONN, 10);	//I11 <-->O04	HW GAIN1 output to I2S Out
		vRegSetBit(AFE_GAIN1_CONN, 8);	//I10 <-->O03
		
		vRegSetBit(AFE_GAIN1_CONN, 13);	//I11 <-->O06	HW GAIN1 to awb_data
		vRegSetBit(AFE_GAIN1_CONN, 12);	//I10 <-->O05

		//set HW GAIN and sample rate
		vRegWriteBits(AFE_GAIN1_CON0, u4SamplingRateConvert[srCnt], 4, 4);	//sample rate setting
		//WriteREG(AFE_GAIN1_CON1, 0x80000);	//define the target gain, default is 0x80000H which 0db
		vRegSetBit(AFE_GAIN1_CON0, 0);	//GAIN1 on


		// set I2S DAC output
   		vDACI2SSet(I2S_16BIT, I2S_I2S, srCnt, 0, I2S_NOSWAP);
	    vDACI2SEnable(I2S_ENABLE);

		//clear the SRAM and DRAM, modify the external DRAM size to 2M for test
		u4SramVirBuffer = u4SramVirBase;
	    u4SramPhyBuffer = u4SramPhyBase;
    	u4DramVirBuffer = u4DramVirBase;
	    u4DramPhyBuffer = u4DramPhyBase;
		printk("u4SramVirBuffer=0x%x, u4SramPhyBuffer=0x%x, u4DramVirBuffer=0x%x, u4DramPhyBuffer=0x%x\n",u4SramVirBuffer,u4SramPhyBuffer,u4DramVirBuffer,u4DramPhyBuffer);
	
		memset((void*)u4SramVirBuffer,0,AFE_INTERNAL_SRAM_SIZE);
		memset((void*)u4DramVirBuffer,0,AFE_EXTERNAL_DRAM_SIZE);	//need to set the DRAM size as (0x00200000) //2M to prepare enough buffer to dump PCM data
	
		unsigned int addr;
		unsigned int cnt = 0;
		unsigned int val = 0;

		//fill in source data
		for(addr=u4SramVirBuffer; addr<u4SramVirBuffer+AFE_INTERNAL_SRAM_SIZE-1; addr+=4)
		{
			*( unsigned int *)addr = table_sgen_golden_values_1[cnt++];
			if (cnt == 64)
				cnt = 0;
		}

	
		//AWB memory interface setting, use the DRAM, for record the data after HW gain1 control
		rAWBMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE = u4DramPhyBuffer;
		rAWBMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_END= u4DramPhyBuffer+AFE_EXTERNAL_DRAM_SIZE-1;
		rAWBMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_RP= rAWBMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE;
		rAWBMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_WP= rAWBMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE;
		rAWBMemCfg.eChannelConfig = AFE_STEREO;
		rAWBMemCfg.eDupWrite = 0;
		rAWBMemCfg.eSamplingRate = srCnt;
		rAWBMemCfg.eMemInterface = AFE_MEM_AWB;
	

		// DL1 memory interface setting, use the internal SRAM, for output source data
		rDL1MemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE = u4SramPhyBuffer;
		rDL1MemCfg.rBufferSetting.u4AFE_MEMIF_BUF_END= u4SramPhyBuffer+AFE_INTERNAL_SRAM_SIZE-1;
		rDL1MemCfg.rBufferSetting.u4AFE_MEMIF_BUF_RP= rDL1MemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE;
		rDL1MemCfg.rBufferSetting.u4AFE_MEMIF_BUF_WP= rDL1MemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE;
		rDL1MemCfg.eChannelConfig = AFE_STEREO;
		rDL1MemCfg.eDupWrite = 0;
		rDL1MemCfg.eSamplingRate = srCnt;
		rDL1MemCfg.eMemInterface = AFE_MEM_DL1;

		vAfeTurnOnMemif(&rAWBMemCfg);
		vAfeTurnOnMemif(&rDL1MemCfg);

		vAfeTurnOn();

		printk("[vAudioHWGain1Test] start to test\n");

		BOOL bBufferEnd = FALSE;
		BOOL bDirectUpdateTargetGain = FALSE;
		int i,j;
		int GDStepCnt,GUStepCnt;
	

		for(GDStepCnt=0; GDStepCnt<3; GDStepCnt++)	//set for Gain down step
		{
			WriteREG(AFE_GAIN1_CON2,uGainDownStep1[GDStepCnt]);	//GDStepCnt=0 is default value, set the gain down step value
		
			for(GUStepCnt=0; GUStepCnt<3; GUStepCnt++)	//set for Gain up step
			{
				WriteREG(AFE_GAIN1_CON3,uGainUpStep1[GUStepCnt]);	//set the gain up step value

				printk("uGainDownStep1=0x%x, uGainUpStep1=0x%x!!!\n",ReadREG(AFE_GAIN1_CON2),ReadREG(AFE_GAIN1_CON3));
			
				for(i=0; i<TestCount; i++)
				{
					if(bBufferEnd)
					{
						printk("[vAudioHWGain1Test] Buffer is runout of, stop1\n");
						break;
					}		
		
					vRegWriteBits(AFE_GAIN1_CON0, uSamplePerStep1[i], 8, 8);	//sample per step change

					printk("target gain before set, AFE_GAIN1_CON1=0x%x, AFE_GAIN1_CON0=0x%x!!!\n",ReadREG(AFE_GAIN1_CON1),ReadREG(AFE_GAIN1_CON0));

					for(j=0; j<TestCount; j++)	//set the target gain
					{
						WriteREG(AFE_GAIN1_CON1,uTargetGain1[j]);
						int Cnt = 0;
						bDirectUpdateTargetGain = FALSE;
						if(bBufferEnd) 
						{
							printk("[TcHWGain1Test] Buffer is runout of, stop2\n");
							break;
						}
						if(ReadREG(AFE_AWB_CUR) <= ReadREG(AFE_AWB_END) - 0x200)
						{
							printk("target gain set, AFE_GAIN1_CUR=0x%x, AFE_GAIN1_CON1=0x%x!!!\n",ReadREG(AFE_GAIN1_CUR),ReadREG(AFE_GAIN1_CON1));

							while(ReadREG(AFE_GAIN1_CUR)!= uTargetGain1[j])
							{
								vFpgaAudioClockDelaymSec(100);
								//could check the target gain changed before the current gain reach the target gain
	/*							if(Cnt++>=50)
								{
									printk("can't not reach the TargetGain in Time, update new target gain directly AFE_GAIN1_CUR=0x%x!!!\n",ReadREG(AFE_GAIN1_CUR));
									bDirectUpdateTargetGain = TRUE;
									break;
								}
	*/							//printk("AFE_GAIN1_CUR=0x%x!!!\n",ReadREG(AFE_GAIN1_CUR));
							}
							printk("current gain, AFE_GAIN1_CUR=0x%x!!!\n",ReadREG(AFE_GAIN1_CUR));
							if(!bDirectUpdateTargetGain)
								vFpgaAudioClockDelaySec(1);
						}
						else
						{
							bBufferEnd = TRUE;
							printk("[vAudioHWGain1Test] Buffer is runout of, stop3\n");
							break;
						}
					}//target gain change

					//set the breakpoint to check
					//ReadREG(FPGA_VERSION);	
					//ReadREG(FPGA_AUDIO_CLOCK);
				}			//sample per step change
			}
		}

		printk("AWB current address =0x%x!!!\n",ReadREG(AFE_AWB_CUR));
		printk("u4SramVirBuffer=0x%x, u4SramPhyBuffer=0x%x, u4DramVirBuffer=0x%x, u4DramPhyBuffer=0x%x\n",u4SramVirBuffer,u4SramPhyBuffer,u4DramVirBuffer,u4DramPhyBuffer);

    	vRegResetBit(AFE_DAC_CON0, AFE_ON);	//Stop
		//set the breakpoint to check
		ReadREG(FPGA_VERSION);	
		ReadREG(FPGA_AUDIO_CLOCK);
 
	} 	
    printk("[TcHWGain1Test] end\n");

	return ret;
}



/******************************************************************************
* Function      : vAudioHWGain2Test
* Description   : Test Case on HW gain2 function
* Parameter     :
* Return        : None
******************************************************************************/
BOOL vAudioHWGain2Test()
{
	BOOL ret = TRUE;

    MEMIF_CONFIG_T rAWBMemCfg;
	SAMPLINGRATE_T srCnt = AFE_8000HZ;

//	for(srCnt=AFE_8000HZ; srCnt<=AFE_48000HZ; srCnt++)	//could set the specific sample rate, need to meet the AudioPrecision
	{
    	printk("vAudioHWGain2Test start, srCnt=%d\n",srCnt);
		memset(&rAWBMemCfg, 0, sizeof(MEMIF_CONFIG_T));

		// reset
		vStrongResetAFE();
		// power on
		vAudioTopControl();

		//set interconnection
		vRegSetBit(AFE_GAIN2_CONN, 16);	//I00 <-->O15	I2S in to HW GAIN2
		vRegSetBit(AFE_GAIN2_CONN, 23);	//I01 <-->O16
		
		vRegSetBit(AFE_GAIN2_CONN, 10);	//I13 <-->O04	HW GAIN2 output to I2S Out
		vRegSetBit(AFE_GAIN2_CONN, 8);	//I12 <-->O03
	
		vRegSetBit(AFE_GAIN2_CONN, 13);	//I13 <-->O06	HW GAIN2 to awb
		vRegSetBit(AFE_GAIN2_CONN, 12);	//I12 <-->O05

		//set HW GAIN
		vRegWriteBits(AFE_GAIN2_CON0, u4SamplingRateConvert[srCnt], 4, 4);	//sample rate setting
		//WriteREG(AFE_GAIN2_CON2, 0x80000);	//define the target gain, default is 0x80000H which 0db
		vRegSetBit(AFE_GAIN2_CON0, 0);	//GAIN2 on

		// setup I2S in as slave mode
		vI2SSet(I2S_16BIT, I2S_I2S, srCnt, I2S_SLAVE, I2S_IN);
		vI2SEnable(I2S_ENABLE);
		vFOCEnable(TRUE);
	
		// set I2S DAC output
		vDACI2SSet(I2S_16BIT, I2S_I2S, srCnt, 0, I2S_NOSWAP);
		vDACI2SEnable(I2S_ENABLE);
		
		//clear the SRAM and DRAM
		//need to set the DRAM size as (0x00200000) //2M to prepare enough buffer to dump PCM data
		u4SramVirBuffer = u4SramVirBase;
	    u4SramPhyBuffer = u4SramPhyBase;
    	u4DramVirBuffer = u4DramVirBase;
	    u4DramPhyBuffer = u4DramPhyBase;

		printk("u4SramVirBuffer=0x%x, u4SramPhyBuffer=0x%x, u4DramVirBuffer=0x%x, u4DramPhyBuffer=0x%x\n",u4SramVirBuffer,u4SramPhyBuffer,u4DramVirBuffer,u4DramPhyBuffer);
		memset((void*)u4DramVirBuffer,0,AFE_EXTERNAL_DRAM_SIZE);	
		
		//AWB memory interface setting, use the DRAM
		rAWBMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE = u4DramPhyBuffer;
		rAWBMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_END= u4DramPhyBuffer + AFE_EXTERNAL_DRAM_SIZE-1;
		rAWBMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_RP= rAWBMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE;
		rAWBMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_WP= rAWBMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE;
		rAWBMemCfg.eChannelConfig = AFE_STEREO;
		rAWBMemCfg.eDupWrite = 0;
		rAWBMemCfg.eSamplingRate = srCnt;
		rAWBMemCfg.eMemInterface = AFE_MEM_AWB;	


		vAfeTurnOnMemif(&rAWBMemCfg);

		vAfeTurnOn();
	

		BOOL bBufferEnd = FALSE;
		BOOL bDirectUpdateTargetGain = FALSE;
		int i,j;
		int GDStepCnt,GUStepCnt;
	
		//while(1)
		{
			for(GDStepCnt=0; GDStepCnt<3; GDStepCnt++)	//modify gain down step
			{
				WriteREG(AFE_GAIN2_CON2,uGainDownStep1[GDStepCnt]);	//GDStepCnt=0 is default value, set the gain down step value
		
				for(GUStepCnt=0; GUStepCnt<3; GUStepCnt++)	//modify gain up step
				{
					WriteREG(AFE_GAIN2_CON3,uGainUpStep1[GUStepCnt]);	//set the gain up step value

					printk("uGainDownStep2=0x%x, uGainUpStep2=0x%x!!!\n",ReadREG(AFE_GAIN2_CON2),ReadREG(AFE_GAIN2_CON3));
			
					for(i=0; i<TestCount; i++)
					{
						if(bBufferEnd)
						{
							printk("[vAudioHWGain2Test] Buffer is runout of, stop1\n");
							break;
						}				
						vRegWriteBits(AFE_GAIN2_CON0, uSamplePerStep1[i], 8, 8);	//sample per step change

						printk("target gain before set, AFE_GAIN2_CON1=0x%x, AFE_GAIN2_CON0=0x%x!!!\n",ReadREG(AFE_GAIN2_CON1),ReadREG(AFE_GAIN2_CON0));

						for(j=0; j<TestCount; j++)
						{
							WriteREG(AFE_GAIN2_CON1,uTargetGain1[j]);
							int Cnt = 0;
							bDirectUpdateTargetGain = FALSE;
							if(bBufferEnd) 
							{
								printk("[vAudioHWGain2Test] Buffer is runout of, stop2\n");
								break;
							}
							
							if(ReadREG(AFE_AWB_CUR) <= ReadREG(AFE_AWB_END) - 0x200)
							{
								printk("target gain set, AFE_GAIN2_CUR=0x%x, AFE_GAIN2_CON1=0x%x!!!\n",ReadREG(AFE_GAIN2_CUR),ReadREG(AFE_GAIN2_CON1));

								while(ReadREG(AFE_GAIN2_CUR)!= uTargetGain1[j])
								{
									vFpgaAudioClockDelaymSec(10);
		/*							if(Cnt++>=50)
									{
										printk("can't not reach the TargetGain in Time, update new target gain directly AFE_GAIN1_CUR=0x%x!!!\n",ReadREG(AFE_GAIN2_CUR));
										bDirectUpdateTargetGain = TRUE;
										break;
									}
	*/								//printk("AFE_GAIN2_CUR=0x%x!!!\n",ReadREG(AFE_GAIN2_CUR));
								}
							
								printk("current gain, AFE_GAIN2_CUR=0x%x!!!\n",ReadREG(AFE_GAIN2_CUR));
							
								if(!bDirectUpdateTargetGain)
									vFpgaAudioClockDelaySec(1);
							}				
							else
							{
								bBufferEnd = TRUE;
								printk("[vAudioHWGain2Test] Buffer is runout of, stop3\n");
								break;
							}
						}

						//while(1);	//do loop to check AudioPrecision input
						//set the breakpoint to check
						ReadREG(FPGA_VERSION);	
						ReadREG(FPGA_AUDIO_CLOCK);
					}			
				}
			}
		}
		printk("AWB current address =0x%x!!!\n",ReadREG(AFE_AWB_CUR));

	    vRegResetBit(AFE_DAC_CON0, AFE_ON);	//Stop
    	ReadREG(FPGA_VERSION);
		ReadREG(FPGA_AUDIO_CLOCK);
	}
 
	printk("vAudioHWGain2Test end\n");

	return ret;

}


BOOL vAudioHWGain1_2CombineTest(SAMPLINGRATE_T samplerate)
{
	BOOL ret = TRUE;

    MEMIF_CONFIG_T rAWBMemCfg,rDL1MemCfg;
	SAMPLINGRATE_T srCnt = AFE_8000HZ;

	//for(srCnt=AFE_8000HZ; srCnt<=AFE_48000HZ; srCnt++)	//set the specific samplerate
	{
	    printk("vAudioHWGain1_2CombineTest start, srCnt=%d\n",srCnt);
	
		memset(&rAWBMemCfg, 0, sizeof(MEMIF_CONFIG_T));
		memset(&rDL1MemCfg, 0, sizeof(MEMIF_CONFIG_T));

		// reset
		vStrongResetAFE();
		// power on
		vAudioTopControl();

		//set interconnection Gain1
		vRegSetBit(AFE_GAIN1_CONN, 16);	//I05 <-->O13	DL1_data in to HW GAIN1
		vRegSetBit(AFE_GAIN1_CONN, 21);	//I06 <-->O14
		
		//vRegSetBit(AFE_GAIN1_CONN, 10);	//I11 <-->O04	HW GAIN1 output to I2S Out
		//vRegSetBit(AFE_GAIN1_CONN, 8);	//I10 <-->O03
		
		//vRegSetBit(AFE_GAIN1_CONN, 13);	//I11 <-->O06	HW GAIN1 to awb data
		vRegSetBit(AFE_GAIN1_CONN, 12);	//I10 <-->O05	Gain1 to awb one channel

		//set HW GAIN and sample rate
		vRegWriteBits(AFE_GAIN1_CON0, u4SamplingRateConvert[srCnt], 4, 4);	//sample rate setting
		//WriteREG(AFE_GAIN1_CON1, 0x80000);	//define the target gain, default is 0x80000H which 0db
		vRegSetBit(AFE_GAIN1_CON0, 0);	//GAIN1 on

	
		//set interconnection Gain2
		vRegSetBit(AFE_GAIN2_CONN, 16);	//I00 <-->O15	I2S in to HW GAIN2
		vRegSetBit(AFE_GAIN2_CONN, 23);	//I01 <-->O16
		
		vRegSetBit(AFE_GAIN2_CONN, 10);	//I13 <-->O04	HW GAIN2 output to I2S Out
		vRegSetBit(AFE_GAIN2_CONN, 8);	//I12 <-->O03
	
		vRegSetBit(AFE_GAIN2_CONN, 13);	//I13 <-->O06	HW GAIN2 to awb, gain2 to awb one channel
		//vRegSetBit(AFE_GAIN2_CONN, 12);	//I12 <-->O05

		//set HW GAIN
		vRegWriteBits(AFE_GAIN2_CON0, u4SamplingRateConvert[srCnt], 4, 4);	//sample rate setting
		//WriteREG(AFE_GAIN2_CON2, 0x80000);	//define the target gain, default is 0x80000H which 0db
		vRegSetBit(AFE_GAIN2_CON0, 0);	//GAIN2 on

		// setup I2S in
		vI2SSet(I2S_16BIT, I2S_I2S, srCnt, I2S_SLAVE, I2S_IN);
		vI2SEnable(I2S_ENABLE);
		vFOCEnable(TRUE);
	
		// set I2S DAC output
		vDACI2SSet(I2S_16BIT, I2S_I2S, srCnt, 0, I2S_NOSWAP);
		vDACI2SEnable(I2S_ENABLE);

		//clear the SRAM and DRAM
		//need to set the DRAM size as (0x00200000) //2M to prepare enough buffer to dump PCM data
		u4SramVirBuffer = u4SramVirBase;
	    u4SramPhyBuffer = u4SramPhyBase;
    	u4DramVirBuffer = u4DramVirBase;
	    u4DramPhyBuffer = u4DramPhyBase;

		printk("u4SramVirBuffer=0x%x, u4SramPhyBuffer=0x%x, u4DramVirBuffer=0x%x, u4DramPhyBuffer=0x%x\n",u4SramVirBuffer,u4SramPhyBuffer,u4DramVirBuffer,u4DramPhyBuffer);
		
		memset((void*)u4SramVirBuffer,0,AFE_INTERNAL_SRAM_SIZE);
	
		unsigned int addr;
		unsigned int cnt = 0;
		unsigned int val = 0;
	
		for(addr=u4SramVirBuffer; addr<u4SramVirBuffer+AFE_INTERNAL_SRAM_SIZE-1; addr+=4)
		{
			*( unsigned int *)addr = table_sgen_golden_values_1[cnt++];
			if (cnt == 64)
				cnt = 0;
		}		

		memset((void*)u4DramVirBuffer,0,AFE_EXTERNAL_DRAM_SIZE);	
		
		//AWB memory interface setting, use the DRAM
		rAWBMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE = u4DramPhyBuffer;
		rAWBMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_END= u4DramPhyBuffer + AFE_EXTERNAL_DRAM_SIZE-1;
		rAWBMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_RP= rAWBMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE;
		rAWBMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_WP= rAWBMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE;
		rAWBMemCfg.eChannelConfig = AFE_STEREO;
		rAWBMemCfg.eDupWrite = 0;
		rAWBMemCfg.eSamplingRate = srCnt;
		rAWBMemCfg.eMemInterface = AFE_MEM_AWB;	

		// DL1 memory interface setting, use the internal SRAM
		rDL1MemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE = u4SramPhyBuffer;
		rDL1MemCfg.rBufferSetting.u4AFE_MEMIF_BUF_END= u4SramPhyBuffer+AFE_INTERNAL_SRAM_SIZE-1;
		rDL1MemCfg.rBufferSetting.u4AFE_MEMIF_BUF_RP= rDL1MemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE;
		rDL1MemCfg.rBufferSetting.u4AFE_MEMIF_BUF_WP= rDL1MemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE;
		rDL1MemCfg.eChannelConfig = AFE_STEREO;
		rDL1MemCfg.eDupWrite = 0;
		rDL1MemCfg.eSamplingRate = srCnt;
		rDL1MemCfg.eMemInterface = AFE_MEM_DL1;

		vAfeTurnOnMemif(&rDL1MemCfg);

		vAfeTurnOnMemif(&rAWBMemCfg);

		vAfeTurnOn();
	

		BOOL bBufferEnd = FALSE;
		BOOL bDirectUpdateTargetGain = FALSE;
		int i,j;
		int GDStepCnt,GUStepCnt;
	
	//while(1)
	{
		for(GDStepCnt=0; GDStepCnt<3; GDStepCnt++)
		{
			WriteREG(AFE_GAIN2_CON2,uGainDownStep1[GDStepCnt]);	//GDStepCnt=0 is default value, set the gain down step value
			WriteREG(AFE_GAIN1_CON2,uGainDownStep1[2-GDStepCnt]);

			for(GUStepCnt=0; GUStepCnt<3; GUStepCnt++)
			{
				WriteREG(AFE_GAIN2_CON3,uGainUpStep1[2-GUStepCnt]);	//set the gain up step value
				WriteREG(AFE_GAIN1_CON3,uGainUpStep1[GUStepCnt]);
				printk("uGainDownStep2=0x%x, uGainUpStep2=0x%x!!!\n",ReadREG(AFE_GAIN2_CON2),ReadREG(AFE_GAIN2_CON3));
			
				for(i=0; i<TestCount; i++)
				{
					if(bBufferEnd)
					{
						printk("[vAudioHWGain1_2CombineTest] Buffer is runout of, stop1\n");
						break;
					}		
		
					vRegWriteBits(AFE_GAIN2_CON0, uSamplePerStep1[i], 8, 8);	//sample per step change

					printk("target gain before set, AFE_GAIN2_CON1=0x%x, AFE_GAIN2_CON0=0x%x!!!\n",ReadREG(AFE_GAIN2_CON1),ReadREG(AFE_GAIN2_CON0));

					for(j=0; j<TestCount; j++)
					{
						WriteREG(AFE_GAIN2_CON1,uTargetGain2[j]);
						WriteREG(AFE_GAIN1_CON1,uTargetGain1[j]);
						int Cnt = 0;
						
						bDirectUpdateTargetGain = FALSE;
						if(bBufferEnd) 
						{
							printk("[vAudioHWGain1_2CombineTest] Buffer is runout of, stop2\n");
							break;
						}
						if(ReadREG(AFE_AWB_CUR) <= ReadREG(AFE_AWB_END) - 0x200)
						{
							printk("target gain set, AFE_GAIN2_CUR=0x%x, AFE_GAIN2_CON1=0x%x!!!\n",ReadREG(AFE_GAIN2_CUR),ReadREG(AFE_GAIN2_CON1));

							while(ReadREG(AFE_GAIN2_CUR)!= uTargetGain2[j])
							{
								vFpgaAudioClockDelaymSec(10);
	/*							if(Cnt++>=50)
								{
									printk("can't not reach the TargetGain in Time, update new target gain directly AFE_GAIN1_CUR=0x%x!!!\n",ReadREG(AFE_GAIN2_CUR));
									bDirectUpdateTargetGain = TRUE;
									break;
								}
	*/							printk("AFE_GAIN1_CUR=0x%x, AFE_GAIN2_CUR=0x%x!!!\n",ReadREG(AFE_GAIN1_CUR),ReadREG(AFE_GAIN2_CUR));
							}

							printk("current gain AFE_GAIN1_CUR=0x%x, AFE_GAIN2_CUR=0x%x!!!\n",ReadREG(AFE_GAIN1_CUR),ReadREG(AFE_GAIN2_CUR));

							if(!bDirectUpdateTargetGain)
								vFpgaAudioClockDelaySec(1);
						}
						else
						{
							bBufferEnd = TRUE;
							printk("[vAudioHWGain1_2CombineTest] Buffer is runout of, stop3\n");
							break;
						}
					}
					//while(1); check the AP
					//set breakpoint to check
					ReadREG(FPGA_VERSION);
					ReadREG(FPGA_AUDIO_CLOCK);
				}			
			}
		}
	}
		vRegResetBit(AFE_DAC_CON0, AFE_ON);	//Stop
    	ReadREG(FPGA_VERSION);
		ReadREG(FPGA_AUDIO_CLOCK);
	}
 
	printk("vAudioHWGain1_2CombineTest end\n");

	return ret;	
}