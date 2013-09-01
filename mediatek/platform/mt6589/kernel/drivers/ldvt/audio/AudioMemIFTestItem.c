#include "AudioAfe.h"
#include "AudioRom.h"
#include "audio.h"
#include "AudioCommon.h"

#define BUFFER_OFFSET 8 // to test buffer 64 bit alignment

//#define STREAMING_DEBUG
#define AFE_EXTERNAL_DRAM_HALF_SIZE (AFE_EXTERNAL_DRAM_SIZE>>1)
#define UPLINK_COPY_DATA_BASE (u4DramVirBase+AFE_EXTERNAL_DRAM_HALF_SIZE)
#define UPLINK_COPY_DATA_END  (u4DramVirEnd)

UINT32 u4UplinkCopyDataCurAddr;
UINT32 u4UplinkCopyDataStopAddr;

UINT32 u4InterCon_DL_ch1[2]= {I05,I07}; //DL1 ; DL2
UINT32 u4InterCon_DL_ch2[2]= {I06,I08}; //DL1 ; DL2
UINT32 u4InterCon_UL_ch1[2]= {O09,O05};  //VUL ; AWB
UINT32 u4InterCon_UL_ch2[2]= {O010,O06}; //VUL ; AWB
UINT32 u4InterCon_DAI_UL[2]= {O011,O012}; //DAI ; MOD_DAI
UINT32 u4DLSetting[2]= {AFE_MEM_DL1,AFE_MEM_DL2};
UINT32 u4ULSetting[2]= {AFE_MEM_VUL,AFE_MEM_AWB};
UINT32 u4ULDAISetting[2]= {AFE_MEM_DAI,AFE_MEM_MOD_PCM};

UINT32 u4VirBuffer[2];
UINT32 u4PhyBuffer[2];
UINT32 u4BufferSize[2]= {AFE_INTERNAL_SRAM_SIZE, AFE_EXTERNAL_DRAM_HALF_SIZE};

typedef struct
{
    UINT32 u4VirtualBufSA;
    UINT32 u4VirtualBufEA;

    UINT16 *pu2Ch1TableSA;
    UINT16 *pu2Ch1TableEA;
    UINT32 u4Ch1TableSz;
    UINT16 *pu2Ch1TableCur;

    UINT16 *pu2Ch2TableSA;
    UINT16 *pu2Ch2TableEA;
    UINT32 u4Ch2TableSz;
    UINT16 *pu2Ch2TableCur;

    UINT32 *pu4Ch1TableSA;  // for 24 bit
    UINT32 *pu4Ch1TableCur; // for 24 bit

    UINT32 u4NormaData;  // for uplink, the first 2 or 3 samples are zero
    UINT32 u4BufLoopCnt;
} STREAM_INFO_T;


/******************************************************************************
* Function      : fgAudioMemTest
* Description   : write pattern and read to check memory
* Parameter     :
* Return        : None
******************************************************************************/
BOOL fgAudioMemTest()
{
    BOOL fgResult = TRUE;

    printk("[fgAudioMemTest] Sram base = 0x%x, end=0x%x\n",u4SramVirBase, u4SramVirEnd);

    UINT32 u4Pattern[2] = {0x55555555, 0xAAAAAAAA};
    UINT32 u4Addr;

    int i;
    for(i=0; i<2; i++)  // Test odd/even bits
    {
        // Write
        for(u4Addr=u4SramVirBase; u4Addr<u4SramVirEnd; u4Addr+=4)
        {
            WriteREG(u4Addr, u4Pattern[i]);
        }

        // Read & check
        for(u4Addr=u4SramVirBase; u4Addr<u4SramVirEnd; u4Addr+=4)
        {
            if(ReadREG(u4Addr) != u4Pattern[i])
            {
                fgResult = FALSE;
                printk("[fgAudioMemTest] ERROR !! [0x%x] = 0x%x\n",u4Addr, ReadREG(u4Addr));
                return fgResult;
            }
        }
    }

    return fgResult;
}


/******************************************************************************
* Function      : vGetDownLinkBuf
* Description   : Get DL r/w pointer
* Parameter     :
* Return        : None
******************************************************************************/
void vGetDownLinkBuf(MEMIF_BUF_T *rBufInfo,STREAM_INFO_T *rStreamInfo,UINT32* u4WriteAddr1, UINT32* u4Size1, UINT32* u4WriteAddr2, UINT32* u4Size2)
{
    UINT32 u4Sa,u4Ea;
    UINT32 u4Rp,u4Wp;
    u4Rp = rBufInfo->u4AFE_MEMIF_BUF_RP;
    u4Wp = rBufInfo->u4AFE_MEMIF_BUF_WP;
    u4Sa = rBufInfo->u4AFE_MEMIF_BUF_BASE;
    u4Ea = rBufInfo->u4AFE_MEMIF_BUF_END;
    if(u4Rp == u4Wp)  //Buffer full
    {
        //printk("u4Rp == u4Wp = 0x%x\n", u4Rp);
        *u4WriteAddr1 = NULL;
        *u4Size1 = 0;
        *u4WriteAddr2 = NULL;
        *u4Size2 = 0;
    }
    else if(u4Rp > u4Wp)
    {
        *u4WriteAddr1 = u4Wp-u4Sa+rStreamInfo->u4VirtualBufSA;
        *u4Size1 = u4Rp - u4Wp;
        *u4WriteAddr2 = NULL;
        *u4Size2 = 0;
    }
    else
    {
        *u4WriteAddr1 = u4Wp-u4Sa+rStreamInfo->u4VirtualBufSA;
        *u4Size1 = u4Ea - u4Wp+1;
        *u4WriteAddr2 = rStreamInfo->u4VirtualBufSA;
        *u4Size2 = u4Rp-u4Sa;
    }
}


/******************************************************************************
* Function      : vGetUpLinkBuf
* Description   : Get UL r/w pointer
* Parameter     :
* Return        : None
******************************************************************************/
void vGetUpLinkBuf(MEMIF_BUF_T *rBufInfo,STREAM_INFO_T *rStreamInfo,UINT32* u4ReadAddr1, UINT32* u4Size1, UINT32* u4ReadAddr2, UINT32* u4Size2)
{
    UINT32 u4Sa,u4Ea;
    UINT32 u4Rp,u4Wp;
    u4Rp = rBufInfo->u4AFE_MEMIF_BUF_RP;
    u4Wp = rBufInfo->u4AFE_MEMIF_BUF_WP;
    u4Sa = rBufInfo->u4AFE_MEMIF_BUF_BASE;
    u4Ea = rBufInfo->u4AFE_MEMIF_BUF_END;
    if(u4Rp == u4Wp)  //Buffer empty
    {
        //printk("u4Rp == u4Wp = 0x%x\n", u4Rp);
        *u4ReadAddr1 = NULL;
        *u4Size1 = 0;
        *u4ReadAddr2 = NULL;
        *u4Size2 = 0;
    }
    else if(u4Rp > u4Wp)
    {
        *u4ReadAddr1 = u4Rp-u4Sa+rStreamInfo->u4VirtualBufSA;
        *u4Size1 = u4Ea - u4Rp+1;
        *u4ReadAddr2 = rStreamInfo->u4VirtualBufSA;
        *u4Size2 = u4Wp-u4Sa;
    }
    else
    {
        *u4ReadAddr1 = u4Rp-u4Sa+rStreamInfo->u4VirtualBufSA;
        *u4Size1 = u4Wp-u4Rp;
        *u4ReadAddr2 = NULL;
        *u4Size2 = 0;
    }
}


/******************************************************************************
* Function      : vDLStreaming
* Description   : Put PCM data to DL buffer
* Parameter     :
* Return        : None
******************************************************************************/
void vDLStreaming(MEMIF_CONFIG_T *rMemCfg, STREAM_INFO_T *rStreamInfo)
{
    UINT32 i,u4Ch;
    UINT32 u4Sa,u4Ea;
    UINT32 u4Rp,u4Wp;
    UINT32 u4WriteAddr1,u4WriteAddr2;
    UINT32 u4Size1,u4Size2;
    UINT16 *pu2Ptr1;
    MEMIF_BUF_T *rBufInfo = &(rMemCfg->rBufferSetting);

    u4Ch = (rMemCfg->eChannelConfig==AFE_MONO) ? 1 : 2;

    //printk("DL: rBufInfo->u4AFE_MEMIF_BUF_RP= 0x%x \n",rBufInfo->u4AFE_MEMIF_BUF_RP);
    vAfeGetCurPointer(rMemCfg);
    vGetDownLinkBuf(rBufInfo,rStreamInfo,&u4WriteAddr1,&u4Size1,&u4WriteAddr2,&u4Size2);

#ifdef STREAMING_DEBUG
    printk("DL: rBufInfo->u4AFE_MEMIF_BUF_RP= 0x%x \n",rBufInfo->u4AFE_MEMIF_BUF_RP);
    printk("DL: rBufInfo->u4AFE_MEMIF_BUF_WP= 0x%x \n",rBufInfo->u4AFE_MEMIF_BUF_WP);
    printk("DL: u4WriteAddr1= 0x%x \n",u4WriteAddr1);
    printk("DL: u4Size1= 0x%x \n",u4Size1);
    printk("DL: u4WriteAddr2= 0x%x \n",u4WriteAddr2);
    printk("DL: u4Size2= 0x%x \n",u4Size2);
#endif

    pu2Ptr1 = (UINT16 *)u4WriteAddr1;

    for(i=0; i<u4Size1/2/u4Ch; i++)
    {
        *pu2Ptr1++ = *rStreamInfo->pu2Ch1TableCur++;
        if(rStreamInfo->pu2Ch1TableCur == rStreamInfo->pu2Ch1TableSA+rStreamInfo->u4Ch1TableSz)
            rStreamInfo->pu2Ch1TableCur = rStreamInfo->pu2Ch1TableSA;
        rBufInfo->u4AFE_MEMIF_BUF_WP+=2;	  //update write pointer
        if(rBufInfo->u4AFE_MEMIF_BUF_WP==(rBufInfo->u4AFE_MEMIF_BUF_END+1))
        {
            rStreamInfo->u4BufLoopCnt++;
            rBufInfo->u4AFE_MEMIF_BUF_WP=rBufInfo->u4AFE_MEMIF_BUF_BASE;
        }


        // check if having ch1
        if(u4Ch==2)
        {
            *pu2Ptr1++ = *rStreamInfo->pu2Ch2TableCur++;
            if(rStreamInfo->pu2Ch2TableCur == rStreamInfo->pu2Ch2TableSA+rStreamInfo->u4Ch2TableSz)
                rStreamInfo->pu2Ch2TableCur = rStreamInfo->pu2Ch2TableSA;
            rBufInfo->u4AFE_MEMIF_BUF_WP+=2;	  //update write pointer
            if(rBufInfo->u4AFE_MEMIF_BUF_WP==(rBufInfo->u4AFE_MEMIF_BUF_END+1))
            {
                rStreamInfo->u4BufLoopCnt++;
                rBufInfo->u4AFE_MEMIF_BUF_WP=rBufInfo->u4AFE_MEMIF_BUF_BASE;
            }

        }
    }
    pu2Ptr1 = (UINT16 *)u4WriteAddr2;
    for(i=0; i<u4Size2/2/u4Ch; i++)
    {

        *pu2Ptr1++ = *rStreamInfo->pu2Ch1TableCur++;
        if(rStreamInfo->pu2Ch1TableCur == rStreamInfo->pu2Ch1TableSA+rStreamInfo->u4Ch1TableSz)
            rStreamInfo->pu2Ch1TableCur = rStreamInfo->pu2Ch1TableSA;
        rBufInfo->u4AFE_MEMIF_BUF_WP+=2;	  //update write pointer
        if(rBufInfo->u4AFE_MEMIF_BUF_WP==(rBufInfo->u4AFE_MEMIF_BUF_END+1))
        {
            rStreamInfo->u4BufLoopCnt++;
            rBufInfo->u4AFE_MEMIF_BUF_WP=rBufInfo->u4AFE_MEMIF_BUF_BASE;
        }


        // check if having ch1
        if(u4Ch==2)
        {
            *pu2Ptr1++ = *rStreamInfo->pu2Ch2TableCur++;
            if(rStreamInfo->pu2Ch2TableCur == rStreamInfo->pu2Ch2TableSA+rStreamInfo->u4Ch2TableSz)
                rStreamInfo->pu2Ch2TableCur = rStreamInfo->pu2Ch2TableSA;
            rBufInfo->u4AFE_MEMIF_BUF_WP+=2;	  //update write pointer
            if(rBufInfo->u4AFE_MEMIF_BUF_WP==(rBufInfo->u4AFE_MEMIF_BUF_END+1))
            {
                rStreamInfo->u4BufLoopCnt++;
                rBufInfo->u4AFE_MEMIF_BUF_WP=rBufInfo->u4AFE_MEMIF_BUF_BASE;
            }

        }
    }
}


/******************************************************************************
* Function      : vULStreaming
* Description   : Get UL data and copy to dram
* Parameter     :
* Return        : None
******************************************************************************/
void vULStreaming(MEMIF_CONFIG_T *rMemCfg, STREAM_INFO_T *rStreamInfo)
{
    UINT32 u4ReadAddr1,u4ReadAddr2;
    UINT32 u4Size1,u4Size2;
    MEMIF_BUF_T *rBufInfo = &(rMemCfg->rBufferSetting);

    // Updata uplink current write pointer
    vAfeGetCurPointer(rMemCfg);

    // get copy size
    vGetUpLinkBuf(rBufInfo,rStreamInfo,&u4ReadAddr1,&u4Size1,&u4ReadAddr2,&u4Size2);

    // copy ul data to dram
    if(u4Size1 > 0 && u4UplinkCopyDataCurAddr+u4Size1 < UPLINK_COPY_DATA_END)
    {
        memcpy((void*)u4UplinkCopyDataCurAddr, (void*)u4ReadAddr1, u4Size1);
        u4UplinkCopyDataCurAddr += u4Size1;

        if(u4Size2 > 0 && u4UplinkCopyDataCurAddr+u4Size2 < UPLINK_COPY_DATA_END)
        {
            memcpy((void*)u4UplinkCopyDataCurAddr, (void*)u4ReadAddr2, u4Size2);
            u4UplinkCopyDataCurAddr += u4Size2;
        }
    }

    // move read pointer to old write pointer
    rBufInfo->u4AFE_MEMIF_BUF_RP = rBufInfo->u4AFE_MEMIF_BUF_WP;
}

/******************************************************************************
* Function      : fgCheckULStreaming
* Description   : Compare PCM data with source table
* Parameter     :
* Return        : None
******************************************************************************/
BOOL fgCheckULStreaming(MEMIF_CONFIG_T *rMemCfg, STREAM_INFO_T *rStreamInfo, UINT32 u4Size)
{
    UINT32 i,j,u4Ch,u4DataPerSample;
    UINT16 *pDramAddr;
    MEMIF_BUF_T *rBufInfo = &(rMemCfg->rBufferSetting);

    u4Ch = (rMemCfg->eChannelConfig==AFE_MONO) ? 1 : 2;
    u4DataPerSample = (rMemCfg->eDupWrite==AFE_DUP_WR_ENABLE) ? 2 : 1;

    pDramAddr = (UINT16 *) UPLINK_COPY_DATA_BASE;

    printk("[fgCheckULStreaming] pDramAddr = 0x%x, u4Size = 0x%x\n", UPLINK_COPY_DATA_BASE, u4Size);

    UINT32 u4NumSamples = u4Size/2/u4Ch/u4DataPerSample;

    // First 2 or 3 samples are zero
    for(i=0; i<u4NumSamples; i++)
    {
        if ((rStreamInfo->u4NormaData == 0) && (*pDramAddr !=0))
        {
            rStreamInfo->u4NormaData = 1;
            break;
        }

        if (rStreamInfo->u4NormaData == 0)
        {
            if(*pDramAddr !=0)
            {
                printk("Uplink data ERROR!! addr = 0x%x ; data = 0x%x \n",pDramAddr,*pDramAddr);
                return FALSE;
            }
            for(j=0; j<u4DataPerSample; j++)
            {
                pDramAddr++;
            }

            if(u4Ch==2)
            {
                if(*pDramAddr !=0)
                {
                    printk("Uplink data ERROR!! addr = 0x%x ; data = 0x%x \n",pDramAddr,*pDramAddr);
                    return FALSE;
                }
                pDramAddr++;
            }
        }
    }

    // check samples
    for(; i<u4NumSamples; i++)
    {
        for(j=0; j<u4DataPerSample; j++)
        {
            if(*pDramAddr != *rStreamInfo->pu2Ch1TableCur)
            {
                printk("Uplink data ERROR!! addr = 0x%x ; data = 0x%x ;table  = 0x%x \n",pDramAddr,*pDramAddr,*rStreamInfo->pu2Ch1TableCur);
                return FALSE;
            }
            pDramAddr++;
        }

        rStreamInfo->pu2Ch1TableCur++;
        if (rStreamInfo->pu2Ch1TableCur == rStreamInfo->pu2Ch1TableSA+rStreamInfo->u4Ch1TableSz)
        {
            rStreamInfo->pu2Ch1TableCur = rStreamInfo->pu2Ch1TableSA;
        }

        //Check if having ch2
        if(u4Ch==2)
        {
            if(*pDramAddr !=  *rStreamInfo->pu2Ch2TableCur)
            {
                printk("Uplink data ERROR!! addr = 0x%x ; data = 0x%x ;table  = 0x%x \n",pDramAddr,*pDramAddr, *rStreamInfo->pu2Ch2TableCur);
                return FALSE;
            }

            pDramAddr++;
            rStreamInfo->pu2Ch2TableCur++;
            if (rStreamInfo->pu2Ch2TableCur == rStreamInfo->pu2Ch2TableSA+rStreamInfo->u4Ch2TableSz)
            {
                rStreamInfo->pu2Ch2TableCur = rStreamInfo->pu2Ch2TableSA;
            }
        }
    }

    return TRUE;
}

/******************************************************************************
* Function      : fgAudioMemULMonoTest
* Description   :
* Parameter     :
* Return        : None
******************************************************************************/
BOOL fgAudioMemULMonoTest()
{
    UINT32 u4DLCnt,u4ULCnt,u4MonoUseR;
    UINT32 u4BufferCnt = 1;
    UINT32 i,u4MonoLoopCnt;
    UINT32 u4SourceBuf;
    UINT32 u4ReceiveBuf;
    UINT32 u4MemIfBufferSize;
    UINT16 *pPtr1;

    MEMIF_CONFIG_T rDLMemCfg,rULMemCfg;
    STREAM_INFO_T rDLStreamInfo,rULStreamInfo;

    printk("=======================================================================================================================================\n");
    printk("[[[[[  fgAudioMemULMonoTest  ]]]]]\n");

    // reset
    vStrongResetAFE();

    // power on
    vAudioTopControl();

    u4VirBuffer[0] = u4SramVirBase;
    u4PhyBuffer[0] = u4SramPhyBase;
    u4VirBuffer[1] = u4DramVirBase;
    u4PhyBuffer[1] = u4DramPhyBase;

    //use external memory
    SAMPLINGRATE_T u4SamplingRateCnt = AFE_48000HZ;  //sampling rate = 48000
    for(u4DLCnt=0; u4DLCnt<2; u4DLCnt++) // DL1 & DL2
    {
        for(u4ULCnt=0; u4ULCnt<2; u4ULCnt++) // VUL & AWB
        {
            for(u4MonoUseR=0; u4MonoUseR<2; u4MonoUseR++)
            {
                printk("=======================================================================================================================================\n");
                printk("[fgAudioMemULMonoTest]: DOWNLINK LOOP = %d \n",u4DLCnt);
                printk("[fgAudioMemULMonoTest]: UPLINK LOOP = %d \n",u4ULCnt);
                printk("[fgAudioMemULMonoTest]: Mono Use R = %d \n\n",u4MonoUseR);

                vSoftResetAFE();

                // Set buffer size for each memory interface
                u4MemIfBufferSize = u4BufferSize[u4BufferCnt]/4;  //to fully use memory(DL1+DL2+VUL+AWB)

                //Set whole buffer as zero
                printk("[fgAudioMemULMonoTest]: clean buffer base: 0x%x, size: 0x%x\n", u4VirBuffer[u4BufferCnt], u4BufferSize[u4BufferCnt]);
                memset((void*)u4VirBuffer[u4BufferCnt], 0, u4BufferSize[u4BufferCnt]);

                //Buffer allocation
                u4SourceBuf = u4PhyBuffer[u4BufferCnt] + u4MemIfBufferSize*u4DLCnt + BUFFER_OFFSET; //+BUFFER_OFFSET to test buffer alignment at start address
                u4ReceiveBuf = u4PhyBuffer[u4BufferCnt] + u4MemIfBufferSize*(2+u4ULCnt) + BUFFER_OFFSET;
                printk("[fgAudioMemULMonoTest]: Source buf [DL%d] address = 0x%x \n",u4DLCnt,u4SourceBuf);
                printk("[fgAudioMemULMonoTest]: Received buf [UL%d]address=0x%x\n",u4ULCnt,u4ReceiveBuf);

                // Initial buffer control variables
                memset(&rDLMemCfg, 0, sizeof(MEMIF_CONFIG_T));
                rDLMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE= u4SourceBuf;
                rDLMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_END = u4SourceBuf + u4MemIfBufferSize-BUFFER_OFFSET*2-1; //-BUFFER_OFFSET to test buffer alignment at end address
                rDLMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_RP = rDLMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE;
                rDLMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_WP = rDLMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE;

                memset(&rULMemCfg, 0, sizeof(MEMIF_CONFIG_T));
                rULMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE = u4ReceiveBuf;
                rULMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_END = u4ReceiveBuf + u4MemIfBufferSize-BUFFER_OFFSET*2-1-0x40; //-0x40 for different size with DL buffer
                rULMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_RP = rULMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE;
                rULMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_WP = rULMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE;

                // Update streaming info for virtual address
                rDLStreamInfo.u4VirtualBufSA = rDLMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE-u4PhyBuffer[u4BufferCnt]+u4VirBuffer[u4BufferCnt];
                rDLStreamInfo.u4VirtualBufEA = rDLMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_END-u4PhyBuffer[u4BufferCnt]+u4VirBuffer[u4BufferCnt];
                rULStreamInfo.u4VirtualBufSA = rULMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE-u4PhyBuffer[u4BufferCnt]+u4VirBuffer[u4BufferCnt];
                rULStreamInfo.u4VirtualBufEA = rULMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_END-u4PhyBuffer[u4BufferCnt]+u4VirBuffer[u4BufferCnt];

                // Initial downlink table info
                rDLStreamInfo.pu2Ch1TableSA = (UINT16 *)tone1k_48kHz;
                rDLStreamInfo.u4Ch1TableSz = TBL_SZ_48KHz_1K;
                rDLStreamInfo.pu2Ch1TableCur = (UINT16 *)tone1k_48kHz;
                rDLStreamInfo.pu2Ch2TableSA = (UINT16 *)tone1p5k_48kHz;
                rDLStreamInfo.u4Ch2TableSz = TBL_SZ_48KHz_1p5K;
                rDLStreamInfo.pu2Ch2TableCur = (UINT16 *)tone1p5k_48kHz;

                // Reset info for UL checking
                rULStreamInfo.u4NormaData = 0;
                rULStreamInfo.u4BufLoopCnt= 0;
                pPtr1 = (UINT16 *)rDLStreamInfo.u4VirtualBufSA;

                // Download PCM data to DL buf
                for(i = 0 ; i < ((rDLStreamInfo.u4VirtualBufEA-rDLStreamInfo.u4VirtualBufSA+1)/4) ; i++) // 16bit * 2ch = 32bits = 4 bytes
                {
                    *pPtr1++ = *rDLStreamInfo.pu2Ch1TableCur++;
                    *pPtr1++ = *rDLStreamInfo.pu2Ch2TableCur++;
                    if ((UINT32)pPtr1 > rDLStreamInfo.u4VirtualBufEA)
                        pPtr1 = (UINT16 *)rDLStreamInfo.u4VirtualBufSA;

                    if (rDLStreamInfo.pu2Ch1TableCur == rDLStreamInfo.pu2Ch1TableSA+rDLStreamInfo.u4Ch1TableSz)
                        rDLStreamInfo.pu2Ch1TableCur =  rDLStreamInfo.pu2Ch1TableSA;
                    if (rDLStreamInfo.pu2Ch2TableCur == rDLStreamInfo.pu2Ch2TableSA+rDLStreamInfo.u4Ch2TableSz)
                        rDLStreamInfo.pu2Ch2TableCur =  rDLStreamInfo.pu2Ch2TableSA;
                }
                // Write pointer = Read pointer (buffer full)

                // Set interconnection
                bConnect(u4InterCon_DL_ch1[u4DLCnt], u4InterCon_UL_ch1[u4ULCnt], false);
                bConnect(u4InterCon_DL_ch2[u4DLCnt], u4InterCon_UL_ch2[u4ULCnt], false);

                //Set Downlink memory interface
                rDLMemCfg.eMemInterface = u4DLSetting[u4DLCnt];
                rDLMemCfg.eSamplingRate = u4SamplingRateCnt;
                rDLMemCfg.eChannelConfig = AFE_STEREO;
                vAfeTurnOnMemif(&rDLMemCfg);

                //Set Uplink memory interface
                rULMemCfg.eMemInterface = u4ULSetting[u4ULCnt];
                rULMemCfg.eSamplingRate = u4SamplingRateCnt;
                rULMemCfg.eChannelConfig = AFE_MONO;
                rULMemCfg.eDupWrite = 0;
                rULMemCfg.eMonoSelect= u4MonoUseR;

                if(rULMemCfg.eMonoSelect == AFE_MONO_USE_L)
                {
                    // Initial uplink table info as L channel
                    rULStreamInfo.pu2Ch1TableSA = (UINT16 *)tone1k_48kHz;
                    rULStreamInfo.u4Ch1TableSz = TBL_SZ_48KHz_1K;
                    rULStreamInfo.pu2Ch1TableCur = (UINT16 *)tone1k_48kHz;
                }
                else
                {
                    // Initial uplink table info as R channel
                    rULStreamInfo.pu2Ch1TableSA = (UINT16 *)tone1p5k_48kHz;
                    rULStreamInfo.u4Ch1TableSz = TBL_SZ_48KHz_1p5K;
                    rULStreamInfo.pu2Ch1TableCur = (UINT16 *)tone1p5k_48kHz;
                }

                vAfeTurnOnMemif(&rULMemCfg);

                //copy uplink data for validate bit true
                u4UplinkCopyDataCurAddr = UPLINK_COPY_DATA_BASE;
                u4UplinkCopyDataStopAddr = UPLINK_COPY_DATA_BASE + (u4MemIfBufferSize<<2) - 0x100;

                //turn on
                vAfeTurnOn();

                while(u4UplinkCopyDataCurAddr < u4UplinkCopyDataStopAddr)
                {
                    vDLStreaming(&rDLMemCfg,&rDLStreamInfo);
                    vULStreaming(&rULMemCfg,&rULStreamInfo);
                }

                //turn off
                vAfeTurnOff();

                //check result
                if (!fgCheckULStreaming(&rULMemCfg,&rULStreamInfo,u4UplinkCopyDataCurAddr-UPLINK_COPY_DATA_BASE) )
                {
                    printk("!!!!!!!!!! DL=%d UL=%d MonoUseR=%d\n", u4DLCnt, u4ULCnt, u4MonoUseR);
                    return false;
                }

                printk("\n");

            }
        }
    }

    return TRUE;
}

/******************************************************************************
* Function      : fgAudioMemLoop1Test
* Description   : Loop1: (DL1, DL2) x (ADC, AWB) x (9 rates) x (Stereo) x (EMI, Internal) = 72 items
* Parameter     :
* Return        : None
******************************************************************************/
BOOL fgAudioMemLoop1Test()
{
    UINT32 u4DLCnt,u4ULCnt,u4BufferCnt;
    UINT32 i;
    UINT32 u4SourceBuf;
    UINT32 u4ReceiveBuf;
    UINT32 u4MemIfBufferSize;
    UINT16 *pPtr1;

    MEMIF_CONFIG_T rDLMemCfg,rULMemCfg;
    STREAM_INFO_T rDLStreamInfo,rULStreamInfo;

    SAMPLINGRATE_T u4SamplingRateCnt;
    UINT8 sr_convert;

    printk("=======================================================================================================================================\n");
    printk("[[[[[  fgAudioMemLoop1Test  ]]]]]\n");

    // reset
    vStrongResetAFE();

    // power on
    vAudioTopControl();

    u4VirBuffer[0] = u4SramVirBase;
    u4PhyBuffer[0] = u4SramPhyBase;
    u4VirBuffer[1] = u4DramVirBase;
    u4PhyBuffer[1] = u4DramPhyBase;

    for(u4DLCnt=0; u4DLCnt<2; u4DLCnt++)
    {
        for(u4ULCnt=0; u4ULCnt<2; u4ULCnt++)
        {
            for(u4SamplingRateCnt=AFE_8000HZ; u4SamplingRateCnt<=AFE_48000HZ; u4SamplingRateCnt++)
            {
                for(u4BufferCnt=0; u4BufferCnt<2; u4BufferCnt++)
                {
                    printk("=======================================================================================================================================\n");
                    printk("[fgAudioMemLoop1Test]: DL=%d UL=%d rs=%d BufferCnt=%d\n", u4DLCnt, u4ULCnt, u4SamplingRateHz[u4SamplingRateCnt], u4BufferCnt);

                    vSoftResetAFE();

                    // Set buffer size for each memory interface
                    u4MemIfBufferSize = u4BufferSize[u4BufferCnt]/4;  //to fully use memory

                    //Set whole buffer as zero
                    memset((void*)u4VirBuffer[u4BufferCnt],0,u4BufferSize[u4BufferCnt]);

                    //Buffer allocation
                    u4SourceBuf = u4PhyBuffer[u4BufferCnt] + u4MemIfBufferSize*u4DLCnt + BUFFER_OFFSET; //+BUFFER_OFFSET to test buffer alignment at start address
                    u4ReceiveBuf = u4PhyBuffer[u4BufferCnt] + u4MemIfBufferSize*(2+u4ULCnt) + BUFFER_OFFSET;
                    printk("[fgAudioMemLoop1Test]: Source buf [DL%d] address = 0x%x \n",u4DLCnt,u4SourceBuf);
                    printk("[fgAudioMemLoop1Test]: Received buf [UL%d]address= 0x%x\n",u4ULCnt,u4ReceiveBuf);

                    // Initial buffer control variables
                    memset(&rDLMemCfg, 0, sizeof(MEMIF_CONFIG_T));
                    rDLMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE= u4SourceBuf;
                    rDLMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_END = u4SourceBuf + u4MemIfBufferSize-BUFFER_OFFSET*2-1; //-BUFFER_OFFSET to test buffer alignment at end address
                    rDLMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_RP = rDLMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE;
                    rDLMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_WP = rDLMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE;

                    memset(&rULMemCfg, 0, sizeof(MEMIF_CONFIG_T));
                    rULMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE = u4ReceiveBuf;
                    rULMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_END = u4ReceiveBuf + u4MemIfBufferSize-BUFFER_OFFSET*2-1-0x40; //-0x40 for different size with DL buffer
                    rULMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_RP = rULMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE;
                    rULMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_WP = rULMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE;

                    // Update streaming info for virtual address
                    rDLStreamInfo.u4VirtualBufSA = rDLMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE-u4PhyBuffer[u4BufferCnt]+u4VirBuffer[u4BufferCnt];
                    rDLStreamInfo.u4VirtualBufEA = rDLMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_END-u4PhyBuffer[u4BufferCnt]+u4VirBuffer[u4BufferCnt];
                    rULStreamInfo.u4VirtualBufSA = rULMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE-u4PhyBuffer[u4BufferCnt]+u4VirBuffer[u4BufferCnt];
                    rULStreamInfo.u4VirtualBufEA = rULMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_END-u4PhyBuffer[u4BufferCnt]+u4VirBuffer[u4BufferCnt];

                    // Initial downlink table info
                    rDLStreamInfo.pu2Ch1TableSA = (UINT16 *)tone1k_48kHz;
                    rDLStreamInfo.u4Ch1TableSz = TBL_SZ_48KHz_1K;
                    rDLStreamInfo.pu2Ch1TableCur = (UINT16 *)tone1k_48kHz;
                    rDLStreamInfo.pu2Ch2TableSA = (UINT16 *)tone1p5k_48kHz;
                    rDLStreamInfo.u4Ch2TableSz = TBL_SZ_48KHz_1p5K;
                    rDLStreamInfo.pu2Ch2TableCur = (UINT16 *)tone1p5k_48kHz;
                    // Initial uplink table info
                    rULStreamInfo.pu2Ch1TableSA = (UINT16 *)tone1k_48kHz;
                    rULStreamInfo.u4Ch1TableSz = TBL_SZ_48KHz_1K;
                    rULStreamInfo.pu2Ch1TableCur = (UINT16 *)tone1k_48kHz;
                    rULStreamInfo.pu2Ch2TableSA = (UINT16 *)tone1p5k_48kHz;
                    rULStreamInfo.u4Ch2TableSz = TBL_SZ_48KHz_1p5K;
                    rULStreamInfo.pu2Ch2TableCur = (UINT16 *)tone1p5k_48kHz;

                    // Reset info for UL checking
                    rULStreamInfo.u4NormaData = 0;
                    rULStreamInfo.u4BufLoopCnt= 0;
                    pPtr1 = (UINT16 *)rDLStreamInfo.u4VirtualBufSA;

                    // Download PCM data
                    for(i = 0 ; i < ((rDLStreamInfo.u4VirtualBufEA-rDLStreamInfo.u4VirtualBufSA+1)/2/2) ; i++)
                    {
                        *pPtr1++ = *rDLStreamInfo.pu2Ch1TableCur++;
                        *pPtr1++ = *rDLStreamInfo.pu2Ch2TableCur++;
                        if((UINT32)pPtr1 > rDLStreamInfo.u4VirtualBufEA)
                            pPtr1 = (UINT16 *)rDLStreamInfo.u4VirtualBufSA;

                        if(rDLStreamInfo.pu2Ch1TableCur == rDLStreamInfo.pu2Ch1TableSA+rDLStreamInfo.u4Ch1TableSz)
                            rDLStreamInfo.pu2Ch1TableCur = rDLStreamInfo.pu2Ch1TableSA;
                        if(rDLStreamInfo.pu2Ch2TableCur == rDLStreamInfo.pu2Ch2TableSA+rDLStreamInfo.u4Ch2TableSz)
                            rDLStreamInfo.pu2Ch2TableCur = rDLStreamInfo.pu2Ch2TableSA;
                    }
                    // Write pointer =Read pointer (buffer full)

                    // Set interconnection
                    bConnect(u4InterCon_DL_ch1[u4DLCnt], u4InterCon_UL_ch1[u4ULCnt], false);
                    bConnect(u4InterCon_DL_ch2[u4DLCnt], u4InterCon_UL_ch2[u4ULCnt], false);

                    //Set Downlink memory interface
                    rDLMemCfg.eMemInterface = u4DLSetting[u4DLCnt];
                    rDLMemCfg.eSamplingRate = u4SamplingRateCnt;
                    rDLMemCfg.eChannelConfig = AFE_STEREO;
                    rDLMemCfg.eDupWrite = 0;
                    vAfeTurnOnMemif(&rDLMemCfg);

                    //Set Uplink memory interface
                    rULMemCfg.eMemInterface = u4ULSetting[u4ULCnt];
                    rULMemCfg.eSamplingRate = u4SamplingRateCnt;
                    rULMemCfg.eChannelConfig = AFE_STEREO;
                    rULMemCfg.eDupWrite = 0;
                    vAfeTurnOnMemif(&rULMemCfg);

                    //copy uplink data for validate bit true
                    u4UplinkCopyDataCurAddr = UPLINK_COPY_DATA_BASE;
                    u4UplinkCopyDataStopAddr = UPLINK_COPY_DATA_BASE + (u4MemIfBufferSize<<2) - 0x100;

                    // turn on
                    vAfeTurnOn();

                    while(u4UplinkCopyDataCurAddr < u4UplinkCopyDataStopAddr)
                    {
                        vULStreaming(&rULMemCfg,&rULStreamInfo);
                        vDLStreaming(&rDLMemCfg,&rDLStreamInfo);
                    }

                    //turn off
                    vAfeTurnOff();

                    //check result
                    if (!fgCheckULStreaming(&rULMemCfg,&rULStreamInfo,u4UplinkCopyDataCurAddr-UPLINK_COPY_DATA_BASE) )
                    {
                        printk("!!!!!!!!!! DL=%d UL=%d rs=%d BufferCnt=%d\n", u4DLCnt, u4ULCnt, u4SamplingRateHz[u4SamplingRateCnt], u4BufferCnt);
                        break;//return false;
                    }

                    printk("\n");

                }
            }
        }
    }
    return TRUE;
}

/******************************************************************************
* Function      : fgAudioMemLoop2Test
* Description   : Loop2: (DL1, DL2) x (dai, dai_mod) x (2 rates) x (Mono) x (EMI, Internal) = 16 items
* Parameter     :
* Return        : None
******************************************************************************/
BOOL fgAudioMemLoop2Test()
{
    UINT32 u4DLCnt,u4ULCnt,u4BufferCnt,u4DuplicateWrite;
    UINT32 i;
    UINT32 u4SourceBuf;
    UINT32 u4ReceiveBuf;
    UINT32 u4MemIfBufferSize;
    UINT16 *pPtr1;

    MEMIF_CONFIG_T rDLMemCfg,rULMemCfg;
    STREAM_INFO_T rDLStreamInfo,rULStreamInfo;

    DAIMOD_SAMPLINGRATE_T u4SamplingRateCnt;

    printk("=======================================================================================================================================\n");
    printk("[[[[[  fgAudioMemLoop2Test  ]]]]]\n");

    // reset
    vStrongResetAFE();

    // power on
    vAudioTopControl();

    u4VirBuffer[0] = u4SramVirBase;
    u4PhyBuffer[0] = u4SramPhyBase;
    u4VirBuffer[1] = u4DramVirBase;
    u4PhyBuffer[1] = u4DramPhyBase;

    for(u4DLCnt=0; u4DLCnt<2; u4DLCnt++)
    {
        for(u4ULCnt=0; u4ULCnt<2; u4ULCnt++)
        {
            for(u4SamplingRateCnt=AFE_DAIMOD_8000HZ; u4SamplingRateCnt<=AFE_DAIMOD_16000HZ; u4SamplingRateCnt++)
            {
                for(u4BufferCnt=0; u4BufferCnt<2; u4BufferCnt++)
                {
                    for(u4DuplicateWrite=0; u4DuplicateWrite<2; u4DuplicateWrite++)
                    {
                        printk("=======================================================================================================================================\n");
                        printk("[fgAudioMemLoop2Test]: DOWNLINK LOOP = %d \n",u4DLCnt);
                        printk("[fgAudioMemLoop2Test]: UPLINK LOOP = %d \n",u4ULCnt);
                        printk("[fgAudioMemLoop2Test]: SAMPLING RATE LOOP = %d (0:8K, 1:16K)\n", u4SamplingRateCnt);
                        printk("[fgAudioMemLoop2Test]: 2 BUFFER LOOP = %d \n",u4BufferCnt);
                        printk("[fgAudioMemLoop2Test]: u4DuplicateWrite = %d \n\n",u4DuplicateWrite);

                        vSoftResetAFE();

                        // Set buffer size for each memory interface
                        u4MemIfBufferSize = u4BufferSize[u4BufferCnt]/4;  //to fully use memory

                        //Set whole buffer as zero
                        memset((void*)u4VirBuffer[u4BufferCnt],0,u4BufferSize[u4BufferCnt]);

                        //Buffer allocation
                        u4SourceBuf = u4PhyBuffer[u4BufferCnt] + u4MemIfBufferSize*u4DLCnt + BUFFER_OFFSET; //+BUFFER_OFFSET to test buffer alignment at start address
                        u4ReceiveBuf = u4PhyBuffer[u4BufferCnt] + u4MemIfBufferSize*(2+u4ULCnt) + BUFFER_OFFSET;
                        printk("[fgAudioMemLoop2Test]: Source buf [DL%d] address = 0x%x \n",u4DLCnt,u4SourceBuf);
                        printk("[fgAudioMemLoop2Test]: Received buf [UL%d]address=0x%x\n",u4ULCnt,u4ReceiveBuf);

                        // Initial buffer control variables
                        memset(&rDLMemCfg, 0, sizeof(MEMIF_CONFIG_T));
                        rDLMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE= u4SourceBuf;
                        rDLMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_END = u4SourceBuf + u4MemIfBufferSize-BUFFER_OFFSET*2-1; //-BUFFER_OFFSET to test buffer alignment at end address
                        rDLMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_RP = rDLMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE;
                        rDLMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_WP = rDLMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE;

                        memset(&rULMemCfg, 0, sizeof(MEMIF_CONFIG_T));
                        rULMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE = u4ReceiveBuf;
                        rULMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_END = u4ReceiveBuf + u4MemIfBufferSize-BUFFER_OFFSET*2-1-0x40; //-0x40 for different size with DL buffer
                        rULMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_RP = rULMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE;
                        rULMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_WP = rULMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE;

                        // Update streaming info for virtual address
                        rDLStreamInfo.u4VirtualBufSA = rDLMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE-u4PhyBuffer[u4BufferCnt]+u4VirBuffer[u4BufferCnt];
                        rDLStreamInfo.u4VirtualBufEA = rDLMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_END-u4PhyBuffer[u4BufferCnt]+u4VirBuffer[u4BufferCnt];
                        rULStreamInfo.u4VirtualBufSA = rULMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_BASE-u4PhyBuffer[u4BufferCnt]+u4VirBuffer[u4BufferCnt];
                        rULStreamInfo.u4VirtualBufEA = rULMemCfg.rBufferSetting.u4AFE_MEMIF_BUF_END-u4PhyBuffer[u4BufferCnt]+u4VirBuffer[u4BufferCnt];
                        // Initial downlink table info
                        rDLStreamInfo.pu2Ch1TableSA = (UINT16 *)tone1k_48kHz;
                        rDLStreamInfo.u4Ch1TableSz = TBL_SZ_48KHz_1K;
                        rDLStreamInfo.pu2Ch1TableCur = (UINT16 *)tone1k_48kHz;
                        rDLStreamInfo.pu2Ch2TableSA = 0;
                        rDLStreamInfo.u4Ch2TableSz = 0;
                        rDLStreamInfo.pu2Ch2TableCur = 0;
                        // Initial uplink table info
                        rULStreamInfo.pu2Ch1TableSA = (UINT16 *)tone1k_48kHz;
                        rULStreamInfo.u4Ch1TableSz = TBL_SZ_48KHz_1K;
                        rULStreamInfo.pu2Ch1TableCur = (UINT16 *)tone1k_48kHz;
                        rULStreamInfo.pu2Ch2TableSA = 0;
                        rULStreamInfo.u4Ch2TableSz = 0;
                        rULStreamInfo.pu2Ch2TableCur = 0;

                        // Reset info for UL checking
                        rULStreamInfo.u4NormaData = 0;
                        rULStreamInfo.u4BufLoopCnt= 0;
                        pPtr1 = (UINT16 *)rDLStreamInfo.u4VirtualBufSA;
                        // Download PCM data
                        for(i = 0 ; i < ((rDLStreamInfo.u4VirtualBufEA-rDLStreamInfo.u4VirtualBufSA+1)/2) ; i++)
                        {
                            *pPtr1++ = *rDLStreamInfo.pu2Ch1TableCur++;
                            if((UINT32)pPtr1 > rDLStreamInfo.u4VirtualBufEA)
                                pPtr1 = (UINT16 *)rDLStreamInfo.u4VirtualBufSA;

                            if(rDLStreamInfo.pu2Ch1TableCur == rDLStreamInfo.pu2Ch1TableSA+rDLStreamInfo.u4Ch1TableSz)
                                rDLStreamInfo.pu2Ch1TableCur = rDLStreamInfo.pu2Ch1TableSA;
                        }
                        // Write pointer =Read pointer (buffer full)

                        // Set interconnection
                        if(u4ULCnt == 0)
                            bConnect(u4InterCon_DL_ch1[u4DLCnt], u4InterCon_DAI_UL[u4ULCnt], false);
                        else
                            bConnect(u4InterCon_DL_ch2[u4DLCnt], u4InterCon_DAI_UL[u4ULCnt], false);


                        //Set Downlink memory interface
                        rDLMemCfg.eMemInterface = u4DLSetting[u4DLCnt];
                        rDLMemCfg.eSamplingRate = (u4SamplingRateCnt == AFE_DAIMOD_8000HZ) ? AFE_8000HZ : AFE_16000HZ;
                        rDLMemCfg.eChannelConfig = AFE_MONO;
                        rDLMemCfg.eDupWrite = 0;
                        vAfeTurnOnMemif(&rDLMemCfg);

                        //Set Uplink memory interface
                        rULMemCfg.eMemInterface = u4ULDAISetting[u4ULCnt];
                        rULMemCfg.eDaiModSamplingRate = u4SamplingRateCnt;
                        rULMemCfg.eChannelConfig = AFE_MONO;
                        rULMemCfg.eDupWrite = u4DuplicateWrite;
                        vAfeTurnOnMemif(&rULMemCfg);

                        //copy uplink data for validate bit true
                        u4UplinkCopyDataCurAddr = UPLINK_COPY_DATA_BASE;
                        u4UplinkCopyDataStopAddr = UPLINK_COPY_DATA_BASE + (u4MemIfBufferSize<<2) - 0x100;

                        // turn on
                        vAfeTurnOn();

                        while(u4UplinkCopyDataCurAddr < u4UplinkCopyDataStopAddr)
                        {
                            vDLStreaming(&rDLMemCfg,&rDLStreamInfo);
                            vULStreaming(&rULMemCfg,&rULStreamInfo);
                        }

                        //turn off
                        vAfeTurnOff();

                        //check result
                        if (!fgCheckULStreaming(&rULMemCfg,&rULStreamInfo,u4UplinkCopyDataCurAddr-UPLINK_COPY_DATA_BASE) )
                        {
                            printk("!!!!!!!!!! DL=%d UL=%d rs=%d BufferCnt=%d DupWrite=%d\n", u4DLCnt, u4ULCnt, u4SamplingRateHz[u4SamplingRateCnt], u4BufferCnt, u4DuplicateWrite);
                            return false;
                        }

                        printk("\n");
                    }

                }
            }
        }
    }
    return TRUE;
}

