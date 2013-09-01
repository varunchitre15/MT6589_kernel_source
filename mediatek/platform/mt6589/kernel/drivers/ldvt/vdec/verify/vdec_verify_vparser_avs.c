#include "vdec_verify_mpv_prov.h"
#include "../hal/vdec_hal_if_common.h"
#include "../hal/vdec_hal_if_avs.h"
#include "vdec_verify_keydef.h"
#include "vdec_verify_vparser_avs.h"
#include "../include/vdec_info_avs.h"
#include "../include/vdec_drv_avs_info.h"
#include "vdec_verify_file_common.h"
#include "vdec_verify_filesetting.h"
#include <linux/string.h>

#if 0
#define CHKPARAM(u4Param, u4HBound)\
    if(u4Param > u4HBound)\
    {\
        printk( "Hdr err\n");\
        VDEC_ASSERT(0);\
        return;\
    }

#define CHKPARAM_R(i4Param, i4LBound, i4HBound)\
    if((i4Param < i4LBound) || (i4Param > i4HBound))\
    {\
        printk("Hdr err\n");\
        VDEC_ASSERT(0);\
        return;\
    }
#endif


extern int rand(void);

INT32 i4VDECVerAvsSeqHdr(UINT32 u4BsId, UINT32 u4InstID, VDEC_INFO_AVS_SEQ_HDR_T *prVDecAvsSeqHdr);
INT32 i4VDECVerAvsPicHdr(UINT32 u4BsId, UINT32 u4InstID, VDEC_INFO_AVS_SEQ_HDR_T *prVDecAvsSeqHdr, VDEC_INFO_AVS_PIC_HDR_T *prVDecAvsPicHdr);

void _VDEC_ClrAvsFBufInfo(VDEC_INFO_AVS_DEC_PRM_T *prVDecAvsDecPrm, UINT32 u4FBufIdx, BOOL fgInit)
{
    VDEC_INFO_AVS_FBUF_INFO_T* prFBufInfo;
        
    prFBufInfo = &prVDecAvsDecPrm->arFBufInfo[u4FBufIdx];        
    prFBufInfo->fgNonExisting = FALSE;    
    prFBufInfo->fgIsBufRef = FALSE;
    prFBufInfo->fgIsErr = FALSE;
    prFBufInfo->fgIsNotDisp = FALSE;    
    prFBufInfo->u1FBufStatus = NO_PIC;
    prFBufInfo->ucFbId = VDEC_FB_ID_UNKNOWN;    
    prFBufInfo->u4FrameNum = (UINT32)0xffffffff;
    prFBufInfo->i4FrameNumWrap = (INT32)0xefffffff;
}


void vVerInitAVS(UINT32 u4InstID)
{
    VDEC_INFO_AVS_VFIFO_PRM_T     rAvsVDecInitPrm;
    VDEC_INFO_AVS_BS_INIT_PRM_T  rAvsBSInitPrm;
    VDEC_INFO_AVS_SEQ_HDR_T *prVDecAvsSeqHdr = &_rVDecAvsSeqHdr[u4InstID];
    VDEC_INFO_AVS_PIC_HDR_T *prVDecAvsPicHdr = &_rVDecAvsPicHdr[u4InstID];
    VDEC_INFO_AVS_DEC_PRM_T* prVDecAvsDecPrm = &(_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecAVSDecPrm);
    UINT32 i;

    _u4TotalDecFrms[u4InstID] = 0;
    #ifdef BARREL2_THREAD_SUPPORT
    VERIFY (x_sema_lock(_ahVDecEndSema[u4InstID], X_SEMA_OPTION_WAIT) == OSR_OK);
    #endif

    rAvsVDecInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
    rAvsVDecInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;

    i4VDEC_HAL_AVS_InitVDecHW(u4InstID, &rAvsVDecInitPrm);

    rAvsBSInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
    rAvsBSInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
    rAvsBSInitPrm.u4VLDRdPtr = (UINT32)_pucVFifo[u4InstID];
    rAvsBSInitPrm.u4VLDWrPtr = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;

    #ifndef  RING_VFIFO_SUPPORT
    rAvsBSInitPrm.u4VLDWrPtr = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
    #else
    rAvsBSInitPrm.u4VLDWrPtr = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));
    #endif

    #if (MEM_ALLOCATE_IOREMAP)
    rAvsBSInitPrm.u4VLDWrPtr -= 1;
    rAvsBSInitPrm.u4VFifoEa -= 1;
    #endif

    i4VDEC_HAL_AVS_InitBarrelShifter(_u4BSID[u4InstID], u4InstID, &rAvsBSInitPrm);
    #ifdef BARREL2_THREAD_SUPPORT
    VERIFY (x_sema_unlock(_ahVDecEndSema[u4InstID]) == OSR_OK);
    #endif

    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecAVSDecPrm.fgEnPP = TRUE;
    _rVDecAvsPicHdr[u4InstID].fgSecField = FALSE;
    memset((void*)prVDecAvsSeqHdr, 0, sizeof(VDEC_INFO_AVS_SEQ_HDR_T));
    memset((void*)prVDecAvsPicHdr, 0, sizeof(VDEC_INFO_AVS_PIC_HDR_T));   

    for (i = 0; i < AVS_MAX_FRM_BUFNUM; i++)
    {
        _VDEC_ClrAvsFBufInfo(prVDecAvsDecPrm, i, TRUE);
    }

    prVDecAvsDecPrm->arFBufInfo[AVS_PREV_FW_IDX].u4YAddr = (UINT32)_pucPic3Y[u4InstID];
    prVDecAvsDecPrm->arFBufInfo[AVS_PREV_FW_IDX].u4CAddrOffset = ((UINT32)_pucPic3C[u4InstID] - (UINT32)_pucPic3Y[u4InstID]);
    prVDecAvsDecPrm->arFBufInfo[AVS_FW_REF_IDX].u4YAddr = (UINT32)_pucPic2Y[u4InstID];
    prVDecAvsDecPrm->arFBufInfo[AVS_FW_REF_IDX].u4CAddrOffset = ((UINT32)_pucPic2C[u4InstID] - (UINT32)_pucPic2Y[u4InstID]);
    prVDecAvsDecPrm->arFBufInfo[AVS_BW_REF_IDX].u4YAddr = (UINT32)_pucPic1Y[u4InstID];
    prVDecAvsDecPrm->arFBufInfo[AVS_BW_REF_IDX].u4CAddrOffset = ((UINT32)_pucPic1C[u4InstID] - (UINT32)_pucPic1Y[u4InstID]);
    prVDecAvsDecPrm->arFBufInfo[AVS_CURR_WORK_IDX].u4YAddr = (UINT32)_pucPic0Y[u4InstID];
    prVDecAvsDecPrm->arFBufInfo[AVS_CURR_WORK_IDX].u4CAddrOffset = ((UINT32)_pucPic0C[u4InstID] - (UINT32)_pucPic0Y[u4InstID]);


    prVDecAvsDecPrm->u4VLDWrapperWrok = (UINT32)_pucVLDWrapperWrok[u4InstID];
    prVDecAvsDecPrm->u4PPWrapperWrok = (UINT32)_pucPPWrapperWork[u4InstID];
}

BOOL fgAVSNextStartCode(UINT32 u4InstID)
{
    UINT32 u4Retry = 0;
    UINT32 u4NextStart;

    //vVDEC_HAL_AVS_AlignRdPtr(_u4BSID[u4InstID], u4InstID, (UINT32)_pucVFifo[u4InstID], BYTE_ALIGN);
    u4NextStart = u4VDEC_HAL_AVS_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0);

    //check until start code 0x000001XX
    while((u4NextStart>>8)!= 1)
    {
        u4NextStart = u4VDEC_HAL_AVS_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 8);
        u4Retry++;  // Qing Li fix here for some special bitstream
        if(++u4Retry > MAX_RETRY_COUNT1)
        {
            return FALSE;
        }
    }
    return(TRUE);
}



// *********************************************************************
// Function : BOOL u4VerVParserVP6(UINT32 u4InstID)
// Description :
// Parameter :
// Return    :
// *********************************************************************
UINT32 u4VerVParserAVS(UINT32 u4InstID, BOOL fgInquiry)
{
    INT32  i4RetVal = 0;
    UINT32 u4BsId = 0;
    VDEC_INFO_AVS_SEQ_HDR_T *prVDecAvsSeqHdr = &_rVDecAvsSeqHdr[u4InstID];
    VDEC_INFO_AVS_PIC_HDR_T *prVDecAvsPicHdr = &_rVDecAvsPicHdr[u4InstID];

    do 
    {
        do
        {
            if (fgAVSNextStartCode(u4InstID)==FALSE)
            {
                return(NO_START_C_ERR1);
            }
            _u4Datain[u4InstID] = u4VDEC_HAL_AVS_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0);
            if((_u4Datain[u4InstID] != AVS_SEQ_HDR_SC) && (_u4Datain[u4InstID] !=AVS_I_PICTURE_SC) && (_u4Datain[u4InstID] != AVS_PB_PICTURE_SC))
            {
                //Keep search next start code
                if (prVDecAvsPicHdr->fgSecField && _u4Datain[u4InstID] >= AVS_SLICE_SC_MIN && _u4Datain[u4InstID] <= AVS_SLICE_SC_MAX)
                {
                    printk("AVS_2nd Field Find Slice Start Code\n");
                    return (i4RetVal);
                }
                else
                {
                    _u4Datain[u4InstID] = u4VDEC_HAL_AVS_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 8);
                }
            }
            else
            {
                if (  prVDecAvsPicHdr->fgSecField && (_u4Datain[u4InstID] ==AVS_I_PICTURE_SC || _u4Datain[u4InstID] ==AVS_PB_PICTURE_SC) )
                {
                    printk("AVS: Field Picture Not Paired!!!!!\n");
                    VDEC_ASSERT(0);
                }
                break;
            }
        }while (1);

        vVDEC_HAL_AVS_BitStuff_Check(u4InstID, 0);
        if (AVS_SEQ_HDR_SC == _u4Datain[u4InstID])
        {
            //SEQ_HEADER
            i4VDECVerAvsSeqHdr(u4BsId, u4InstID, prVDecAvsSeqHdr);      
        }

        _u4Datain[u4InstID] = u4VDEC_HAL_AVS_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0);
        if (AVS_I_PICTURE_SC == _u4Datain[u4InstID])
        {
            //Parse_I_PIC                  
            prVDecAvsPicHdr->fgIsIPic = TRUE;
            i4VDECVerAvsPicHdr(u4BsId, u4InstID, prVDecAvsSeqHdr, prVDecAvsPicHdr);
            break;
        }
        else
        if (AVS_PB_PICTURE_SC == _u4Datain[u4InstID])
        {
            //Parse_PB_PIC
            prVDecAvsPicHdr->fgIsIPic = FALSE;
            i4VDECVerAvsPicHdr(u4BsId, u4InstID, prVDecAvsSeqHdr, prVDecAvsPicHdr);
            break;
        }
        else
        {
            //Do Nothing.
        }
    }while(1);

    return(i4RetVal);

}

INT32 i4VDECVerAvsSeqHdr(UINT32 u4BsId, UINT32 u4InstID, VDEC_INFO_AVS_SEQ_HDR_T *prVDecAvsSeqHdr)
{
    INT32  i4RetVal = VDEC_HDR_SUCCESS;
    UINT32 u4Temp;
    
    memset((void*)prVDecAvsSeqHdr, 0, sizeof(VDEC_INFO_AVS_SEQ_HDR_T));
    
    // Shift 0x000001 for read pointer
    u4Temp = u4VDEC_HAL_AVS_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 32);
    prVDecAvsSeqHdr->u4ProfileID = u4Temp >> 24;
    prVDecAvsSeqHdr->u4LevelID = (u4Temp >> 16) & 0xFF;
    
    u4Temp = u4VDEC_HAL_AVS_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 16);    
    prVDecAvsSeqHdr->u4IsProgSeq = (u4Temp >> 31);
    prVDecAvsSeqHdr->u4HSize = ((u4Temp >> 17) & 0x3FFF);
    prVDecAvsSeqHdr->u4VSize = ((u4Temp >> 3) & 0x3FFF);

    u4Temp = u4VDEC_HAL_AVS_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 29);
    
    prVDecAvsSeqHdr->u4LastHSize = (((prVDecAvsSeqHdr->u4HSize + 15) >> 4) << 4);
    prVDecAvsSeqHdr->u4LastVSize = (((prVDecAvsSeqHdr->u4VSize + 15) >> 4) << 4);

    if ((prVDecAvsSeqHdr->u4HSize == 0) || (prVDecAvsSeqHdr->u4VSize == 0))
    {
        i4RetVal = (INT32)VDEC_HDR_ERR;
        printk("AVS: err at SeqHdr HSize/VSize\n");
        return i4RetVal;
    }
    else
    {
        prVDecAvsSeqHdr->u2WidthDec = (UINT16) (16 * ((prVDecAvsSeqHdr->u4HSize+15) / 16));
        if (prVDecAvsSeqHdr->u4IsProgSeq == 1)
        {
           prVDecAvsSeqHdr->u2HeightDec = (UINT16) (16 * ((prVDecAvsSeqHdr->u4VSize+15) / 16));
        }
        else
        {
           prVDecAvsSeqHdr->u2HeightDec = (UINT16) (32 * ((prVDecAvsSeqHdr->u4VSize+31) / 32));
        }

    }

    prVDecAvsSeqHdr->u4ChromaFmt = (u4Temp >> 30);
    prVDecAvsSeqHdr->u4SamplePrec = (u4Temp >> 27) & 0x7;
    u4Temp = u4VDEC_HAL_AVS_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 5);  
    
    if ( prVDecAvsSeqHdr->u4SamplePrec != 0x1)
    {
        i4RetVal = (INT32)VDEC_HDR_ERR;
        printk("AVS: err at SamplePrecision = %d\n", prVDecAvsSeqHdr->u4SamplePrec);
        return i4RetVal;
    }

    prVDecAvsSeqHdr->u4AspRatio = (u4Temp  >> 28);
    prVDecAvsSeqHdr->u4FrmRate = (u4Temp >> 24) & 0xF;
    u4Temp = u4VDEC_HAL_AVS_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 8);
    if ( prVDecAvsSeqHdr->u4AspRatio == 0x0 || prVDecAvsSeqHdr->u4AspRatio > 0x4)
    {
        i4RetVal = (INT32)VDEC_HDR_ERR;
        printk("AVS: err at u4AspRatio = %d\n", prVDecAvsSeqHdr->u4AspRatio);
        return i4RetVal;
    }  
    
    if ( prVDecAvsSeqHdr->u4FrmRate == 0x0 || prVDecAvsSeqHdr->u4FrmRate > 0x8)
    {
        i4RetVal = (INT32)VDEC_HDR_ERR;
        printk("AVS: err at u4FrmRate = %d\n", prVDecAvsSeqHdr->u4FrmRate);
        return i4RetVal;
    }  
    printk( "prVDecAvsSeqHdr->u4AspRatio = %d, prVDecAvsSeqHdr->u4FrmRate = %d\n", prVDecAvsSeqHdr->u4AspRatio, prVDecAvsSeqHdr->u4FrmRate);
    printk( "H %d x V %d\n", prVDecAvsSeqHdr->u4HSize, prVDecAvsSeqHdr->u4VSize);
    prVDecAvsSeqHdr->u4BitRateL = (u4Temp >> 14);
    prVDecAvsSeqHdr->u4MarketBitBR = (u4Temp >> 13) & 0x1;
    prVDecAvsSeqHdr->u4BitRateU = (u4Temp & 0xFFF);
    u4Temp = u4VDEC_HAL_AVS_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 31);
    prVDecAvsSeqHdr->u4LowDelay  = (u4Temp >> 31);
    prVDecAvsSeqHdr->u4MarkerBit = (u4Temp >> 30) & 0x1;
    prVDecAvsSeqHdr->u4BBVSize = (u4Temp >> 12) & 0x3FFFF;
    prVDecAvsSeqHdr->u4RsvBits = (u4Temp >> 9) & 0x7;
    u4Temp = u4VDEC_HAL_AVS_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 23);
    prVDecAvsSeqHdr->u4IsValid = 1;

    return i4RetVal;
}

INT32 i4VDECVerAvsPicHdr(UINT32 u4BsId, UINT32 u4InstID, VDEC_INFO_AVS_SEQ_HDR_T *prVDecAvsSeqHdr, VDEC_INFO_AVS_PIC_HDR_T *prVDecAvsPicHdr)
{
    INT32  i4RetVal = VDEC_HDR_SUCCESS;
    UINT32 u4Temp = 0xFFFFFFFF;
    BOOL   fgSliceStartCdoe = TRUE;    

    // Turn On Bit Stuffing Check
    vVDEC_HAL_AVS_BitStuff_Check(u4InstID, 1);

    if(!prVDecAvsSeqHdr->u4IsValid)
    {
        i4RetVal = (INT32)VDEC_HDR_ERR;
        printk( "SeqHdr !valid in PicHdr\n");
        return i4RetVal;
    }    

    // Shift 0x000001 for read pointer
    u4Temp = u4VDEC_HAL_AVS_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 32);
    prVDecAvsPicHdr->u4BBVDelay = (u4Temp >> 16);    
    u4Temp = u4VDEC_HAL_AVS_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 16);
    
    if(prVDecAvsPicHdr->fgIsIPic)
    {
        prVDecAvsPicHdr->u4TimeCodeFg = (u4Temp >> 31);
        u4Temp = u4VDEC_HAL_AVS_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);
        if(prVDecAvsPicHdr->u4TimeCodeFg)
        {
            prVDecAvsPicHdr->u4TimeCode = (u4Temp >> 8);
            u4Temp = u4VDEC_HAL_AVS_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 24);
        }
        prVDecAvsPicHdr->u4MarkerBit = (u4Temp >> 31); 
        u4Temp = u4VDEC_HAL_AVS_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);
        prVDecAvsPicHdr->u4PicCodingType = (UINT32) I_PIC;
    }
    else
    {
        prVDecAvsPicHdr->u4PicCodingType = (u4Temp >> 30);
        u4Temp = u4VDEC_HAL_AVS_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 2);
        //CHKPARAM_R(prVDecAvsPicHdr->u4PicCodingType, 1, 2);
    }

    prVDecAvsPicHdr->u4PicDistance = u4Temp >> 24;
    u4Temp = u4VDEC_HAL_AVS_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 8);
    if(prVDecAvsSeqHdr->u4LowDelay)
    {
        //bbv_check_times
        prVDecAvsPicHdr->u4BBVCheckTimes = u4VDEC_AvsUeCodeNum(_u4BSID[u4InstID], u4InstID);
    }

    u4Temp =u4VDEC_HAL_AVS_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0);
    prVDecAvsPicHdr->u4ProgFrm = (u4Temp >> 31);
    u4Temp = u4VDEC_HAL_AVS_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);
    if(!prVDecAvsPicHdr->u4ProgFrm)
    {
        prVDecAvsPicHdr->u4PicStruct = u4Temp >> 31;
        u4Temp = u4VDEC_HAL_AVS_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);
        if(!prVDecAvsPicHdr->u4PicStruct && !prVDecAvsPicHdr->fgIsIPic)
        {
            prVDecAvsPicHdr->u4AdvPredModeDisable = u4Temp >> 31;
            u4Temp = u4VDEC_HAL_AVS_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);
        }
        else
        {
            prVDecAvsPicHdr->u4AdvPredModeDisable = 1;
        }
    }
    else
    {
        prVDecAvsPicHdr->u4PicStruct = FRM_PIC;
        prVDecAvsPicHdr->u4AdvPredModeDisable = 1;
    }
    prVDecAvsPicHdr->u4TFT = u4Temp >> 31;
    prVDecAvsPicHdr->u4RFF = (u4Temp >> 30) & 0x1;
    prVDecAvsPicHdr->u4FixedPicQP = (u4Temp >> 29) & 0x1;
    u4Temp = u4VDEC_HAL_AVS_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 3);
    prVDecAvsPicHdr->u4PicQP = u4Temp >> 26;
    u4Temp = u4VDEC_HAL_AVS_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 6);

    if(!prVDecAvsPicHdr->fgIsIPic)
    {
        if(!((prVDecAvsPicHdr->u4PicCodingType == B_PIC) && (prVDecAvsPicHdr->u4PicStruct == FRM_PIC) && (prVDecAvsPicHdr->u4AdvPredModeDisable)))
        {
            prVDecAvsPicHdr->u4PicRefFg = (u4Temp >> 31);
            u4Temp = u4VDEC_HAL_AVS_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);
        }
        prVDecAvsPicHdr->u4NoForwardRefFg = (u4Temp >> 31);
        u4Temp = u4VDEC_HAL_AVS_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);
        prVDecAvsPicHdr->u4RsvBits = (u4Temp >> 29);
        u4Temp = u4VDEC_HAL_AVS_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 3);
    }

    if((!prVDecAvsPicHdr->u4ProgFrm && !prVDecAvsPicHdr->u4PicStruct) || (!prVDecAvsPicHdr->fgIsIPic))
    {
        prVDecAvsPicHdr->u4SkipModeFg = (u4Temp >> 31);
        u4Temp = u4VDEC_HAL_AVS_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);
    }
    
    if(prVDecAvsPicHdr->fgIsIPic)
    {
        prVDecAvsPicHdr->u4RsvBits = (u4Temp >> 28);
        u4Temp = u4VDEC_HAL_AVS_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 4);
    }
    
    // loop filter
    prVDecAvsPicHdr->u4LoopFilterDisable = (u4Temp >> 31);
    u4Temp = u4VDEC_HAL_AVS_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);
    if(!prVDecAvsPicHdr->u4LoopFilterDisable)
    {
        prVDecAvsPicHdr->u4LoopFilterParamFg = (u4Temp >> 31);
        u4Temp = u4VDEC_HAL_AVS_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);
        if(prVDecAvsPicHdr->u4LoopFilterParamFg)
        {
            u4Temp = u4VDEC_AvsSeCodeNum(_u4BSID[u4InstID], u4InstID);
            if (u4Temp & 0x10000)
            {
                prVDecAvsPicHdr->i4AlphaCOffset = (INT32)(u4Temp & 0xFFFF);
                prVDecAvsPicHdr->i4AlphaCOffset = (INT32)((((UINT32)(prVDecAvsPicHdr->i4AlphaCOffset) ^ (0xFFFFFFFF)) + 1) & 0xFFFF) * (-1); 
            }
            else
            {
                prVDecAvsPicHdr->i4AlphaCOffset = (INT32)(u4Temp & 0xFFFF);
            }

            //CHKPARAM_R(prVDecAvsPicHdr->i4AlphaCOffset, -8, 8);
            u4Temp = u4VDEC_AvsSeCodeNum(_u4BSID[u4InstID], u4InstID);
            if (u4Temp & 0x10000)
            {
                prVDecAvsPicHdr->i4BetaOffset = (INT32)(u4Temp & 0xFFFF);
                prVDecAvsPicHdr->i4BetaOffset = (INT32)((((UINT32)(prVDecAvsPicHdr->i4BetaOffset) ^ (0xFFFFFFFF)) + 1) & 0xFFFF) * (-1); 
            }
            else
            {
                prVDecAvsPicHdr->i4BetaOffset = (INT32)(u4Temp & 0xFFFF);
            }
           //CHKPARAM_R(prVDecAvsPicHdr->i4BetaOffset, -8, 8);
        }
    }


    fgSliceStartCdoe = (BOOL) ( u4VDEC_AVS_Search_SliceSC(_u4BSID[u4InstID], u4InstID) );
    
    if (fgSliceStartCdoe)
        prVDecAvsPicHdr->u4IsValid = 1;
    else
    {
        printk("AVS Cannot Find Slice SC!!!\n");
        VDEC_ASSERT(0);
    }
    return i4RetVal;
    
}

void vVerifyAvsPrepareFBufInfo(UINT32 u4InstID, VDEC_INFO_DEC_PRM_T *prDecPrm)
{  
   //Prepare decode picture, reference picture
   UINT32 u4FbIdx = 0;
   VDEC_INFO_AVS_DEC_PRM_T* prVDecAvsDecPrm = &(_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecAVSDecPrm);
   
   prVDecAvsDecPrm->rAvsWorkBufSa.u4PredSa  = (UINT32)_pucAvsPred[u4InstID];
   prVDecAvsDecPrm->rAvsWorkBufSa.u4Mv1       = (UINT32)_pucAvsMv1[u4InstID];
   prVDecAvsDecPrm->rAvsWorkBufSa.u4Mv2       = (UINT32)_pucAvsMv2[u4InstID];

   if (prVDecAvsDecPrm->prPicHdr->u4PicCodingType == I_PIC)
   {
       //Decode Buffer
       prVDecAvsDecPrm->rAvsFrameBufSa.u4Pic0YSa = prVDecAvsDecPrm->arFBufInfo[AVS_CURR_WORK_IDX].u4YAddr;
       prVDecAvsDecPrm->rAvsFrameBufSa.u4Pic0CSa = prVDecAvsDecPrm->arFBufInfo[AVS_CURR_WORK_IDX].u4YAddr + prVDecAvsDecPrm->arFBufInfo[AVS_CURR_WORK_IDX].u4CAddrOffset;
       //Backward Buffer
       prVDecAvsDecPrm->rAvsFrameBufSa.u4Pic1YSa = prVDecAvsDecPrm->arFBufInfo[AVS_BW_REF_IDX].u4YAddr;
       prVDecAvsDecPrm->rAvsFrameBufSa.u4Pic1CSa = prVDecAvsDecPrm->arFBufInfo[AVS_BW_REF_IDX].u4YAddr + prVDecAvsDecPrm->arFBufInfo[AVS_BW_REF_IDX].u4CAddrOffset;
       //Forward Buffer
       prVDecAvsDecPrm->rAvsFrameBufSa.u4Pic2YSa = prVDecAvsDecPrm->arFBufInfo[AVS_FW_REF_IDX].u4YAddr;
       prVDecAvsDecPrm->rAvsFrameBufSa.u4Pic2CSa = prVDecAvsDecPrm->arFBufInfo[AVS_FW_REF_IDX].u4YAddr + prVDecAvsDecPrm->arFBufInfo[AVS_FW_REF_IDX].u4CAddrOffset;
   }
   else
   if (prVDecAvsDecPrm->prPicHdr->u4PicCodingType == P_PIC)
   {
       //Decode Buffer
       prVDecAvsDecPrm->rAvsFrameBufSa.u4Pic0YSa = prVDecAvsDecPrm->arFBufInfo[AVS_CURR_WORK_IDX].u4YAddr;
       prVDecAvsDecPrm->rAvsFrameBufSa.u4Pic0CSa = prVDecAvsDecPrm->arFBufInfo[AVS_CURR_WORK_IDX].u4YAddr + prVDecAvsDecPrm->arFBufInfo[AVS_CURR_WORK_IDX].u4CAddrOffset;
       //Backward Buffer
       prVDecAvsDecPrm->rAvsFrameBufSa.u4Pic1YSa = prVDecAvsDecPrm->arFBufInfo[AVS_LAST_P_IDX].u4YAddr;
       prVDecAvsDecPrm->rAvsFrameBufSa.u4Pic1CSa = prVDecAvsDecPrm->arFBufInfo[AVS_LAST_P_IDX].u4YAddr + prVDecAvsDecPrm->arFBufInfo[AVS_LAST_P_IDX].u4CAddrOffset;
       //Forward Buffer
       prVDecAvsDecPrm->rAvsFrameBufSa.u4Pic2YSa = prVDecAvsDecPrm->arFBufInfo[AVS_PREV_P_IDX].u4YAddr;
       prVDecAvsDecPrm->rAvsFrameBufSa.u4Pic2CSa = prVDecAvsDecPrm->arFBufInfo[AVS_PREV_P_IDX].u4YAddr + prVDecAvsDecPrm->arFBufInfo[AVS_PREV_P_IDX].u4CAddrOffset;
   }
   else
   if (prVDecAvsDecPrm->prPicHdr->u4PicCodingType == B_PIC)
   {
       //Decode Buffer
       prVDecAvsDecPrm->rAvsFrameBufSa.u4Pic0YSa = prVDecAvsDecPrm->arFBufInfo[AVS_CURR_WORK_IDX].u4YAddr;
       prVDecAvsDecPrm->rAvsFrameBufSa.u4Pic0CSa = prVDecAvsDecPrm->arFBufInfo[AVS_CURR_WORK_IDX].u4YAddr + prVDecAvsDecPrm->arFBufInfo[AVS_CURR_WORK_IDX].u4CAddrOffset;
       //Backward Buffer
       prVDecAvsDecPrm->rAvsFrameBufSa.u4Pic1YSa = prVDecAvsDecPrm->arFBufInfo[AVS_BW_REF_IDX].u4YAddr;
       prVDecAvsDecPrm->rAvsFrameBufSa.u4Pic1CSa = prVDecAvsDecPrm->arFBufInfo[AVS_BW_REF_IDX].u4YAddr + prVDecAvsDecPrm->arFBufInfo[AVS_BW_REF_IDX].u4CAddrOffset;
       //Forward Buffer
       prVDecAvsDecPrm->rAvsFrameBufSa.u4Pic2YSa = prVDecAvsDecPrm->arFBufInfo[AVS_FW_REF_IDX].u4YAddr;
       prVDecAvsDecPrm->rAvsFrameBufSa.u4Pic2CSa = prVDecAvsDecPrm->arFBufInfo[AVS_FW_REF_IDX].u4YAddr + prVDecAvsDecPrm->arFBufInfo[AVS_FW_REF_IDX].u4CAddrOffset;
   }

   prVDecAvsDecPrm->prCurrFBufInfo = &(prVDecAvsDecPrm->arFBufInfo[AVS_CURR_WORK_IDX]);
   _VDEC_ClrAvsFBufInfo(prVDecAvsDecPrm, u4FbIdx, TRUE);
}

void vVerifyAVSSWRst(UINT32 u4BSID, UINT32 u4VDecID)
{    
    //_VDEC_VLDAvsWaitForSramStable();
    vVDEC_HAL_AVS_HW_Reset(u4BSID, u4VDecID);
}

void vVerifySetAvsBsInfo(UINT32 u4BSID, UINT32 u4InstID, VDEC_INFO_DEC_PRM_T *prDecParam, VDEC_INFO_AVS_BS_INIT_PRM_T rAvsBSInitPrm)
{    
    i4VDEC_HAL_AVS_SetBSInfo(u4BSID, u4InstID, prDecParam, rAvsBSInitPrm);
}


// *********************************************************************
// Function    : void vVerAVSUpdateBufStatus(UINT32 u4InstID)
// Description : Update AVS Frame Buffer Status
// Parameter   : None
// Return      : None
// *********************************************************************
void vVerAVSUpdateBufStatus(UINT32 u4InstID)
{
    VDEC_INFO_AVS_DEC_PRM_T* prVDecAvsDecPrm = &(_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecAVSDecPrm);
    VDEC_INFO_AVS_PIC_HDR_T *prVDecAvsPicHdr = &_rVDecAvsPicHdr[u4InstID];
    VDEC_INFO_AVS_FBUF_INFO_T rTempFBufInfo;

    if ( prVDecAvsPicHdr->u4PicStruct == FRM_PIC || prVDecAvsDecPrm->prPicHdr->fgSecField )
    {
        if (prVDecAvsPicHdr->u4PicCodingType == I_PIC || prVDecAvsPicHdr->u4PicCodingType == P_PIC)
        {
            //Maintain DPB
            memcpy(&rTempFBufInfo, &prVDecAvsDecPrm->arFBufInfo[AVS_PREV_FW_IDX], sizeof(VDEC_INFO_AVS_FBUF_INFO_T));
            memcpy(&prVDecAvsDecPrm->arFBufInfo[AVS_PREV_FW_IDX], &prVDecAvsDecPrm->arFBufInfo[AVS_PREV_P_IDX], sizeof(VDEC_INFO_AVS_FBUF_INFO_T));
            memcpy(&prVDecAvsDecPrm->arFBufInfo[AVS_PREV_P_IDX], &prVDecAvsDecPrm->arFBufInfo[AVS_LAST_P_IDX], sizeof(VDEC_INFO_AVS_FBUF_INFO_T));
            memcpy(&prVDecAvsDecPrm->arFBufInfo[AVS_LAST_P_IDX], &prVDecAvsDecPrm->arFBufInfo[AVS_CURR_WORK_IDX], sizeof(VDEC_INFO_AVS_FBUF_INFO_T));
            //memcpy(&prVDecAvsDecPrm->arFBufInfo[AVS_CURR_WORK_IDX], &rTempFBufInfo, sizeof(VDEC_INFO_AVS_FBUF_INFO_T));        
            _VDEC_ClrAvsFBufInfo(prVDecAvsDecPrm, AVS_CURR_WORK_IDX, TRUE);
            prVDecAvsDecPrm->arFBufInfo[AVS_CURR_WORK_IDX].u4YAddr = rTempFBufInfo.u4YAddr;
        }
        else
        {
            _VDEC_ClrAvsFBufInfo(prVDecAvsDecPrm, AVS_CURR_WORK_IDX, TRUE);
        }
    }
    else
    {
        //First field Do-nothing.          
    }  
}

void vVerifyVDecSetAVSInfo(UINT32 u4InstID)
{    
    //UINT32 u4Temp;
    VDEC_INFO_DEC_PRM_T *prDecPrm = &_tVerMpvDecPrm[u4InstID];
    VDEC_INFO_AVS_DEC_PRM_T* prVDecAvsDecPrm = &(_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecAVSDecPrm);
    //VDEC_INFO_AVS_BS_INIT_PRM_T  rAvsBSInitPrm;

    prVDecAvsDecPrm->prSeqHdr = &_rVDecAvsSeqHdr[u4InstID];
    prVDecAvsDecPrm->prPicHdr = &_rVDecAvsPicHdr[u4InstID];


    if ((prVDecAvsDecPrm->prSeqHdr->u4IsProgSeq) ||((!prVDecAvsDecPrm->prSeqHdr->u4IsProgSeq) && (!prVDecAvsDecPrm->prPicHdr->fgSecField)))
    {
        vVerifyAvsPrepareFBufInfo(u4InstID, prDecPrm);
    }

    prVDecAvsDecPrm->i4MemBase = 0;   
    prVDecAvsDecPrm->u4FRefBufIdx = _u4FRefBufIdx[u4InstID];
    _u4RealHSize[u4InstID] = prVDecAvsDecPrm->prSeqHdr->u2WidthDec;
    _u4RealVSize[u4InstID] = prVDecAvsDecPrm->prSeqHdr->u2HeightDec;

    prDecPrm->ucPicStruct = (UCHAR) prVDecAvsDecPrm->prPicHdr->u4PicStruct;
    prDecPrm->ucPicType = (UCHAR) prVDecAvsDecPrm->prPicHdr->u4PicCodingType;
    prDecPrm->ucAddrSwapMode = _tVerMpvDecPrm[u4InstID].ucAddrSwapMode;
    prDecPrm->u4PicBW = prVDecAvsDecPrm->prSeqHdr->u2WidthDec;
    prDecPrm->u4PicW = prVDecAvsDecPrm->prSeqHdr->u2WidthDec;
    prDecPrm->u4PicH = prVDecAvsDecPrm->prSeqHdr->u2HeightDec;

    prVDecAvsDecPrm->pucDecWorkBufY = (UCHAR*) prVDecAvsDecPrm->rAvsFrameBufSa.u4Pic0YSa;
    prVDecAvsDecPrm->pucDecWorkBufC = (UCHAR*) prVDecAvsDecPrm->rAvsFrameBufSa.u4Pic0CSa;
    _pucDecWorkBuf[u4InstID] = prVDecAvsDecPrm->pucDecWorkBufY ;
    _pucDecCWorkBuf[u4InstID] = prVDecAvsDecPrm->pucDecWorkBufC ;

    prVDecAvsDecPrm->prCurrFBufInfo->u1PicCodingType =  prVDecAvsDecPrm->prPicHdr->u4PicCodingType;
    prVDecAvsDecPrm->prCurrFBufInfo->u4PicDistance = prVDecAvsDecPrm->prPicHdr->u4PicDistance;

    i4VDEC_HAL_AVS_DecStart(_u4BSID[u4InstID], u4InstID, prDecPrm);
}


UINT32 u4AVSInverseAddrSwap(UINT32 u4AddrSwapMode, UINT32 u4SwappedAddr, BOOL fgIsYComponent)
{
  unsigned int u4NonSwappedAddr, u4TempAddr;
  switch(u4AddrSwapMode)
  {
  case 0x1: //MT8520_SWAP_MODE_1
    u4NonSwappedAddr = ((u4SwappedAddr&0xFFFFFFC0) | ((u4SwappedAddr&0x20)>>5) | ((u4SwappedAddr&0x10)>>2) | ((u4SwappedAddr&0x8)>>2) | ((u4SwappedAddr&0x7)<<3));
    break;
  case 0x2: //MT8520_SWAP_MODE_2
    u4NonSwappedAddr = ((u4SwappedAddr&0xFFFFFFE0) | ((u4SwappedAddr&0x10)>>4) | ((u4SwappedAddr&0xF)<<1));
    break;
  case 0x4: // MT5351_SWAP_MODE_0
    if(fgIsYComponent)
    {
      u4TempAddr = ((u4SwappedAddr&0xFFFFFF80) | ((u4SwappedAddr&0x40)>>4) | ((u4SwappedAddr&0x3C)<<1) | (u4SwappedAddr&0x3));
      u4NonSwappedAddr = ((u4TempAddr&0xFFFFFF80) | ((u4TempAddr&0x7C)>>2) | ((u4TempAddr&0x3)<<5));
    }
    else
    {
      u4TempAddr = ((u4SwappedAddr&0xFFFFFFC0) | ((u4SwappedAddr&0x20)>>3) | ((u4SwappedAddr&0x1C)<<1) | (u4SwappedAddr&0x3));
      u4NonSwappedAddr = ((u4TempAddr&0xFFFFFFC0) | ((u4TempAddr&0x3C)>>2) | ((u4TempAddr&0x3)<<4));
    }
    break;
  case 0x5: // MT5351_SWAP_MODE_1
    if(fgIsYComponent)
    {
      u4TempAddr = ((u4SwappedAddr&0xFFFFFF00) | ((~u4SwappedAddr)&0x80) | (u4SwappedAddr&0x7F));
      u4NonSwappedAddr = ((u4TempAddr&0xFFFFFF80) | ((u4TempAddr&0x7C)>>2) | ((u4TempAddr&0x3)<<5));
    }
    else
    {
      u4TempAddr = ((u4SwappedAddr&0xFFFFFF80) | ((~u4SwappedAddr)&0x40) | (u4SwappedAddr&0x3F));
      u4NonSwappedAddr = ((u4TempAddr&0xFFFFFFC0) | ((u4TempAddr&0x3C)>>2) | ((u4TempAddr&0x3)<<4));
    }
    break;
  case 0x6: // MT5351_SWAP_MODE_2
    if(fgIsYComponent)
    {
      u4NonSwappedAddr = ((u4SwappedAddr&0xFFFFFF80) | ((u4SwappedAddr&0x7C)>>2) | ((u4SwappedAddr&0x3)<<5));
    }
    else
    {
      u4NonSwappedAddr = ((u4SwappedAddr&0xFFFFFFC0) | ((u4SwappedAddr&0x3C)>>2) | ((u4SwappedAddr&0x3)<<4));
    }
    break;

  case 0xF: // MT8320_FIELD_COMPACT
    if(fgIsYComponent)
    {
      //u4NonSwappedAddr = ((u4SwappedAddr&0xFFFFFFE0) | ((u4SwappedAddr&0x1E)>>1) | ((u4SwappedAddr&0x1)<<4));
      u4NonSwappedAddr = ((u4SwappedAddr&0xFFFFFFE0) | ((u4SwappedAddr&0x0F)<<1) | ((u4SwappedAddr&0x10)>>4));
    }
    else
    {
      //u4NonSwappedAddr = ((u4SwappedAddr&0xFFFFFFF0) | ((u4SwappedAddr&0x0E)>>1) | ((u4SwappedAddr&0x1)<<3));
      u4NonSwappedAddr = ((u4SwappedAddr&0xFFFFFFF0) | ((u4SwappedAddr&0x07)<<1) | ((u4SwappedAddr&0x8)>>3));
    }
    break;
  default:
    u4NonSwappedAddr = u4SwappedAddr;
    break;
  }
  return u4NonSwappedAddr;
}

void vAVS_InvAddressSwap(UINT32 u4InstID, 
                                                         BYTE* pbSrcBufY, BYTE* pbSrcBufC, 
                                                         BYTE* pbOutBufY, BYTE* pbOutBufC,
                                                         UINT32 u4AlignWidth, UINT32 u4AlignHeight, UINT32 u4AlignSize,
                                                         UINT32 u4HwSwapMode)
{
  UINT32 i;
  UINT32 u4DataLength;
  UINT32 u4AlignW_Luma;
  UINT32 u4AlignH_Luma;
  UINT32 u4AlignW_Chroma;
  UINT32 u4AlignH_Chroma;
  //UINT32 u4AlignSize = 0x32000;
  UINT32 u4NonSwappedAddr;  
  UINT32 u4SwappedAddr;
  BYTE * pbTempBufAddr;
  UINT32 u4AddrressSwapSize = 16;
  UINT32 u4AddrSwapMode;

   UINT8 auAddrSwapMapTable[9] =
 {
    4, 5, 6, 7, 0, 1, 2, 3, 15
  };

  #ifdef RM_DDR3MODE_ENABLE
  u4AddrressSwapSize = 16;
  #else //RM_DDR3MODE_ENABLE
  u4AddrressSwapSize = 16;
  #endif //RM_DDR3MODE_ENABLE

  //prParsingPic = (VDEC_INFO_RM_PICINFO_T*) &_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo;

  u4AlignW_Luma = u4AlignWidth;//prParsingPic->u4Width;
  u4AlignH_Luma = u4AlignHeight;//prParsingPic->u4Height;
  
  //pbSrcBufY = (BYTE*) prParsingPic->u4OutBufY;
  //pbSrcBufC = (BYTE*) prParsingPic->u4OutBufC;
  //pbOutBufY = (BYTE*) _pucDumpYBuf[u4InstID];
  //pbOutBufC = (BYTE*) _pucDumpCBuf[u4InstID];

  u4AddrSwapMode = auAddrSwapMapTable[u4HwSwapMode];
  
  //Luma 
  u4DataLength = u4AlignW_Luma * u4AlignH_Luma;
  //u4DataLength = (u4DataLength + u4AlignSize-1)/u4AlignSize;
  //u4DataLength = u4DataLength * u4AlignSize;
  u4SwappedAddr = 0;
  
  for (i=0; i<u4DataLength; i+=u4AddrressSwapSize)
  {
    u4NonSwappedAddr = u4AVSInverseAddrSwap(u4AddrSwapMode, u4SwappedAddr, TRUE);
    pbTempBufAddr = (BYTE*) (pbSrcBufY+i);
    memcpy(&pbOutBufY[u4NonSwappedAddr<<4], &pbTempBufAddr[0],u4AddrressSwapSize);
    u4SwappedAddr++;
  }
  

  //Chroma
  u4AlignW_Chroma = u4AlignW_Luma;
  u4AlignH_Chroma = u4AlignH_Luma / 2;
  
  u4DataLength = u4AlignW_Chroma * u4AlignH_Chroma;
  //u4DataLength = (u4DataLength + u4AlignSize-1)/u4AlignSize;
  //u4DataLength = u4DataLength * u4AlignSize;
  u4SwappedAddr = 0;

  for (i=0; i<u4DataLength; i+=u4AddrressSwapSize)
  {
    u4NonSwappedAddr = u4AVSInverseAddrSwap(u4AddrSwapMode, u4SwappedAddr, FALSE);
    pbTempBufAddr = (BYTE*) (pbSrcBufC+i);
    memcpy(&pbOutBufC[u4NonSwappedAddr<<4], &pbTempBufAddr[0],u4AddrressSwapSize);
    u4SwappedAddr++;
  }
}


void vAVS_CheckCRCResult(UINT32 u4InstID)//, UINT32 u4DecFrameCnt, UINT32 u4CRCResBuf)
{
}

// *********************************************************************
// Function    : void vVP6WrData2PC(UINT32 u4InstID, BYTE *ptAddr, UINT32 u4Size, BOOL *fgNextFrameExist)
// Description : Write the decoded data to PC for compare
// Parameter   : None
// Return      : None
// *********************************************************************
void vAVSWrData2PC(UINT32 u4InstID, UCHAR *ptAddr, UINT32 u4Size, BOOL *fgNextFrameExist)
{
    #if ((!defined(COMP_HW_CHKSUM)) || defined(DOWN_SCALE_SUPPORT))
    UINT32 u4Cnt;
    #ifdef GOLDEN_128BIT_COMP  
    UINT32 u4XPix,u4YPix;
    #endif

    UINT32 u4Width,u4Height;
    UINT32 u4YBase,u4CBase;
    //UINT32 u4BufferWidth;
    UCHAR *pbDecBuf,*pbGoldenBuf;

    #ifndef GOLDEN_128BIT_COMP  
    UINT32 u4Pix;
    #else
    UINT32 u4Ty0, u4Tx0, u4Ty1, u4Tx1;
    UINT32 u4X, u4Y;
    UINT32 mbw, mbh, i, j;
    UINT32 u4Start;  
    #endif  

    #endif  
    
    VDEC_INFO_AVS_SEQ_HDR_T *prVDecAvsSeqHdr = &_rVDecAvsSeqHdr[u4InstID];
    BOOL fgDecErr,fgOpen;
    char strMessage[256];   
    BOOL fgCompare = TRUE;

    UINT32 u4NonSwapYBase = 0;
    UINT32 u4NonSwapCBase = 0;
    UINT32 u4SwapYBase = 0;
    UINT32 u4SwapCBase = 0;
    UINT32 u4NonSwapTargY = 0;
    UINT32 u4NonSwapTargC = 0;

    CHAR _bFileAddStrY[20] = {"_Y.out\0"};
    CHAR _bFileAddStrC[20] = {"_CbCr.out\0"};
    CHAR _bGoldFileName[256] = {"/post_\0"};
    CHAR _bPatternPath [256];
    CHAR _bPathAddStr[256] = {"pattern/\0"};
    CHAR _bPP_PathStr[256] = {"/pp_pat/pp_pat\0"};
    CHAR _bFileName [256];
    UINT32 u4Temp;
//Searh path name
    UINT32 path_byte_addr = 0;
    UINT32 filename_byte_addr = 0;

     UINT32 u4AlignWidth, u4AlignHeight;
     UINT32 u4AlignSize = 0;
     
     for (i=0; ; i++)
     {
          if (_bFileStr1[u4InstID][1][i] == '\0')
          	break;

          if (_bFileStr1[u4InstID][1][i] == 'b' || _bFileStr1[u4InstID][1][i] == 'B')
          {
              if( (_bFileStr1[u4InstID][1][i+1] == 'i' || _bFileStr1[u4InstID][1][i+1] == 'I')
              && (_bFileStr1[u4InstID][1][i+2] == 't' || _bFileStr1[u4InstID][1][i+2] == 'T')
              && (_bFileStr1[u4InstID][1][i+3] == 's' || _bFileStr1[u4InstID][1][i+3] == 'S')
              && (_bFileStr1[u4InstID][1][i+4] == 't' || _bFileStr1[u4InstID][1][i+4] == 'T')
              && (_bFileStr1[u4InstID][1][i+5] == 'r' || _bFileStr1[u4InstID][1][i+5] == 'R')
              && (_bFileStr1[u4InstID][1][i+6] == 'e' || _bFileStr1[u4InstID][1][i+6] == 'E')
              && (_bFileStr1[u4InstID][1][i+7] == 'a' || _bFileStr1[u4InstID][1][i+7] == 'A')
              && (_bFileStr1[u4InstID][1][i+8] == 'm' || _bFileStr1[u4InstID][1][i+8] == 'M') )
              {
                  path_byte_addr = i;
              }
          }
          else
          if (_bFileStr1[u4InstID][1][i] == '.')
          {
              if( (_bFileStr1[u4InstID][1][i+1] == 'a' || _bFileStr1[u4InstID][1][i+1] == 'A')
              && (_bFileStr1[u4InstID][1][i+2] == 'v' || _bFileStr1[u4InstID][1][i+2] == 'V')
              && (_bFileStr1[u4InstID][1][i+3] == 's' || _bFileStr1[u4InstID][1][i+3] == 'S'))
              {
                  filename_byte_addr = i;
              }
          }
    }

    j = 0;
    for (i=path_byte_addr+10; i < filename_byte_addr; i++)
    {
        _bFileName[j] = _bFileStr1[u4InstID][1][i];
        j++;
    }
    _bFileName[j] = '\0';

    for (j=0; j < path_byte_addr; j++)
    {
        _bPatternPath[j] = _bFileStr1[u4InstID][1][j];
    }

    u4Temp = sprintf(_bPatternPath+path_byte_addr, "%s", _bPathAddStr);  
    u4Temp += sprintf(_bPatternPath+path_byte_addr+u4Temp, "%s", _bFileName);
    u4Temp += sprintf(_bPatternPath+path_byte_addr+u4Temp, "%s", _bPP_PathStr);    
    u4Temp += sprintf(_bPatternPath+path_byte_addr+u4Temp, "%s", _bGoldFileName);    
    _bPatternPath[path_byte_addr+u4Temp] = '\0';
    vConcateStr(_bFileStr1[u4InstID][3], _bPatternPath, _bFileAddStrY, _u4FileCnt[u4InstID]);


    strcpy(_tFileListRecInfo[u4InstID].bFileName, _FileList_Rec[u4InstID]);

    #ifdef REDEC  
    if(_u4FileCnt[u4InstID] == _u4ReDecPicNum[u4InstID] )
    {
        if(_u4ReDecNum[u4InstID] != 0)
        {
            _u4ReDecPicNum[u4InstID] = 0xFFFFFFFF;
            _u4ReDecCnt[u4InstID] = _u4ReDecNum[u4InstID];
            vVDecOutputDebugString("/n!!!!!!!!!!!!!! Re-Decode and Wait for debug !!!!!!!!!!!!!!!!\n");
        }
    }
    if(_u4ReDecCnt[u4InstID] > 0)
    {
        _u4ReDecCnt[u4InstID]--;
    }
    #endif

    fgDecErr = FALSE;

    #ifdef GEN_HW_CHKSUM
    #ifndef INTERGRATION_WITH_DEMUX
    vWrite2PC(u4InstID, 9, NULL);
    #endif
    #endif

    #ifndef INTERGRATION_WITH_DEMUX
    if(fgCompare)
    {
        #if (defined(COMP_HW_CHKSUM) && (!defined(DOWN_SCALE_SUPPORT)))       
        #else // compare pixel by pixel
        // Y compare
        _tFBufFileInfo[u4InstID].fgGetFileInfo = TRUE;  
        _tFBufFileInfo[u4InstID].pucTargetAddr = _pucDumpYBuf[u4InstID];
        _tFBufFileInfo[u4InstID].u4TargetSz = GOLD_Y_SZ;  
        _tFBufFileInfo[u4InstID].u4FileLength = 0;  
        // Y decoded data Compare   
         vConcateStr(_bFileStr1[u4InstID][3], _bPatternPath, _bFileAddStrY, _u4FileCnt[u4InstID]);

        #ifdef EXT_COMPARE 
        _tFBufFileInfo[u4InstID].u4FileLength = (((prVDecAvsSeqHdr->u2WidthDec + 15)>>4)<<4) *(((prVDecAvsSeqHdr->u2HeightDec + 31)>>5)<<5);
        #else
        #ifdef DIRECT_DEC
        if((_u4FileCnt[u4InstID] >= _u4StartCompPicNum[u4InstID]) && ( _u4FileCnt[u4InstID] <= _u4EndCompPicNum[u4InstID]))
        #endif    
        {
            #ifdef DOWN_SCALE_SUPPORT            
            #else
            
            u4NonSwapYBase = (UINT32)_pucDecWorkBuf[u4InstID];
            u4NonSwapCBase = (UINT32)_pucDecCWorkBuf[u4InstID];
            if (_tVerMpvDecPrm[u4InstID].ucAddrSwapMode != ADDRSWAP_OFF)
            {
               
                u4SwapYBase = (UINT32)_pucDecWorkBuf[u4InstID];
                u4SwapCBase = (UINT32)_pucDecCWorkBuf[u4InstID];
                
                u4AlignWidth = _tVerMpvDecPrm[u4InstID].u4PicW;
                u4AlignWidth = (((u4AlignWidth +63) >>6) <<6); //Align to 4MB width                    
                u4AlignHeight = _tVerMpvDecPrm[u4InstID].u4PicH;
                u4AlignHeight =  (((u4AlignHeight +31) >>5) <<5);

                u4NonSwapTargY = (UINT32)_pucDumpYBuf[u4InstID];
                u4NonSwapTargC = (UINT32)_pucDumpCBuf[u4InstID];

                vAVS_InvAddressSwap(u4InstID, 
                                   (BYTE*) u4SwapYBase,  (BYTE*) u4SwapCBase, 
                                   (BYTE*) u4NonSwapTargY, (BYTE*) u4NonSwapTargC,
                                  u4AlignWidth,  u4AlignHeight, u4AlignSize,
                                  _tVerMpvDecPrm[u4InstID].ucAddrSwapMode);

                u4NonSwapYBase = (UINT32)_pucAddressSwapBuf[u4InstID];
                u4NonSwapCBase = (UINT32)_pucAddressSwapBuf[u4InstID] + 0x1FE000;
                memcpy((UCHAR*)u4NonSwapYBase, _pucDumpYBuf[u4InstID],(u4AlignWidth*u4AlignHeight) +u4AlignSize);
                memcpy((UCHAR*)u4NonSwapCBase, _pucDumpCBuf[u4InstID],(u4AlignWidth*u4AlignHeight/2) + u4AlignSize);                 

                printk("AVS inverse swap mode %d\n", _tVerMpvDecPrm[u4InstID].ucAddrSwapMode);
            }

            if(fgOpenFile(u4InstID, _bFileStr1[u4InstID][3],"r+b", &_tFBufFileInfo[u4InstID]) == FALSE)
            {
                printk("%s open error\n", _bFileStr1[u4InstID][3]);
            }
            _u4GoldenYSize[u4InstID] = _tFBufFileInfo[u4InstID].u4RealGetBytes;
            #endif
        }
        #endif


        u4Cnt = 0; 
        #ifdef EXT_COMPARE 
        if(_ptCurrFBufInfo[u4InstID]->ucFBufStatus == FRAME)
        {
            vWrite2PC(u4InstID, 5, _pucDecWorkBuf[u4InstID]);
        }
        #else
        
        u4YBase = u4NonSwapYBase;//(UINT32)_pucDecWorkBuf[u4InstID];        
        u4Width = prVDecAvsSeqHdr->u2WidthDec;
        u4Height = prVDecAvsSeqHdr->u2HeightDec;       

        u4Height = ( (u4Height)>>5 ) <<5;
        #ifdef DIRECT_DEC
        if((_u4FileCnt[u4InstID] >= _u4StartCompPicNum[u4InstID]) && ( _u4FileCnt[u4InstID] <= _u4EndCompPicNum[u4InstID]))
        #endif    
        {
            #ifdef GOLDEN_128BIT_COMP
            u4Tx0 = (u4Width >> 4);   // w/ 16
            u4Ty0 = (u4Height >> 5);  // h /32
            u4X = (u4Width & 0xF);    // w % 16
            u4Y = (u4Height & 0x1F);  // h%32
            u4Tx1 = (u4X==0)? u4Tx0 : (u4Tx0+1);
            u4Ty1 = (u4Y==0)? u4Ty0 : (u4Ty0+1);

            for (mbh=0; mbh < u4Ty1; mbh++)
            {
                for (mbw=0; mbw < u4Tx1; mbw++)
                {
                    u4Start = (mbh*u4Tx1 + mbw) * (16*32);
                    pbGoldenBuf = (UCHAR*) (((UINT32) (_pucDumpYBuf[u4InstID])) + u4Start);
                    pbDecBuf = (UCHAR*) (u4YBase + u4Start);

                    for(j=0; j < 32; j++)
                    {             
                        for(i=0; i < 16; i++)
                        {                   
                            if(1)
                            {
                                if(  (mbw == u4Tx0 && i >= u4X) || (mbh == u4Ty0 && j >= u4Y))
                                {
                                    //Do not compare
                                }
                                else
                                {
                                    if ((*(pbDecBuf)) != (*(pbGoldenBuf)))
                                    {
                                        u4Cnt ++;
                                        u4XPix = mbw * 16 + i;
                                        u4YPix = mbh * 32 + j;
								        vVDecOutputDebugString("Pic count to [%d]\n", _u4FileCnt[u4InstID]);  
                                        vVDecOutputDebugString("Y Data Mismatch at [x= 0x%d, y=0x%d] = 0x%x, Golden = 0x%x !!! \n", u4XPix, u4YPix, (*pbDecBuf), (*pbGoldenBuf));
                                        sprintf(strMessage,"Y Data Mismatch at [x= 0x%d, y=0x%d] = 0x%x, Golden = 0x%x !!! \n", u4XPix, u4YPix, (*pbDecBuf), (*pbGoldenBuf));
                                        fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
                                        //fprintf(_tFileListRecInfo.fpFile, "Y Data Mismatch at [0x%.8x] = 0x%.2x, Golden = 0x%.2x !!! \n", i, pbDecBuf[i], _pucDumpBuf[i]);
                                        fgDecErr = TRUE;
                                        //vDumpReg();  // mark by ginny
                                        //vVDEC_HAL_VP6_VDec_DumpReg(u4InstID, FALSE);  // mark by ginny                                          
                                        break;    
                                    }
                                }
                            }//end of if


                            pbGoldenBuf++;
                            pbDecBuf++;
                        }//End of i

                        if(fgDecErr == TRUE)
                        {
                        break;
                        }
                    }//End of j

                    if(fgDecErr == TRUE)
                    {
                        break;
                    }
                }

                if(fgDecErr == TRUE)
                {
                    break;
                }
            }
            #else              
            for (u4Pix = 0; u4Pix < u4Width*u4Height; u4Pix++)
            {
                if(1)
                {
                    //pbDecBuf = (UCHAR*)u4CalculatePixelAddress_Y(u4YBase, u4XPix, u4YPix, u4BufferWidth, 1, 4);
                    pbDecBuf = (UCHAR*)(u4YBase+u4Pix);
                    //pbGoldenBuf = (UCHAR*)u4CalculatePixelAddress_Y((UINT32)_pucDumpYBuf[u4InstID], u4XPix, u4YPix, u4BufferWidth, 1, 4);
                    pbGoldenBuf = (UCHAR*)((UINT32)_pucDumpYBuf[u4InstID]+u4Pix);
                    if ((*(pbDecBuf)) != (*(pbGoldenBuf)))
                    {
                        u4Cnt ++;
                        //vVDecOutputDebugString("Y Data Mismatch at [x= 0x%.8x, y=0x%.8x] = 0x%.2x, Golden = 0x%.2x !!! \n", u4XPix, u4YPix, (*pbDecBuf), (*pbGoldenBuf));
                        vVDecOutputDebugString("Y Data Mismatch at [%d] = 0x%.2x, Golden = 0x%.2x !!! \n", u4Pix, (*pbDecBuf), (*pbGoldenBuf));
                        sprintf(strMessage,"Y Data Mismatch at [%d] = 0x%.2x, Golden = 0x%.2x !!! \n", u4Pix, (*pbDecBuf), (*pbGoldenBuf));
                        fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
                        //fprintf(_tFileListRecInfo.fpFile, "Y Data Mismatch at [0x%.8x] = 0x%.2x, Golden = 0x%.2x !!! \n", i, pbDecBuf[i], _pucDumpYBuf[_u4VDecID][i]);
                        fgDecErr = TRUE;
                        //vVDEC_HAL_VP6_VDec_DumpReg(u4InstID, FALSE);  // mark by ginny
                        break;    
                    }            
                }
                if(fgDecErr == TRUE)
                {
                    break;
                }
            }
            #endif		
            //vVDecOutputDebugString("\nY Data Compare Over!!! Total bytes [0x%.8x] & error [%d]\n", _u4GoldenYSize[_u4VDecID], u4Cnt);
        }
        #endif

        // CbCr compare
        //if(!fgIsMonoPic(_u4VDecID))
        {
            // CbCr decoded data Compare
            _tFBufFileInfo[u4InstID].fgGetFileInfo = TRUE;  
            _tFBufFileInfo[u4InstID].pucTargetAddr = _pucDumpCBuf[u4InstID];
            _tFBufFileInfo[u4InstID].u4TargetSz = GOLD_C_SZ;  
            _tFBufFileInfo[u4InstID].u4FileLength = 0;
            vConcateStr(_bFileStr1[u4InstID][4], _bPatternPath, _bFileAddStrC, _u4FileCnt[u4InstID]);
            #ifdef EXT_COMPARE 
            _tFBufFileInfo[u4InstID].u4FileLength = (((prVDecVp6FrmHdr->u2WidthDec + 15)>>4)<<4) *(((prVDecVp6FrmHdr->u2HeightDec + 31)>>5)<<5) >>1;
            #else
            #ifdef DIRECT_DEC
            if((_u4FileCnt[u4InstID] >= _u4StartCompPicNum[u4InstID]) && ( _u4FileCnt[u4InstID] <= _u4EndCompPicNum[u4InstID]))
            #endif    
            {  
                #ifndef DOWN_SCALE_SUPPORT
                   if(fgOpenFile(u4InstID, _bFileStr1[u4InstID][4],"r+b", &_tFBufFileInfo[u4InstID]) == FALSE)
                    {
                        printk("%s open error\n", _bFileStr1[u4InstID][4]);
                    }
                _u4GoldenCSize[u4InstID] = _tFBufFileInfo[u4InstID].u4RealGetBytes;
                #endif
            }
            #endif       
            u4Cnt = 0; 
            #ifdef EXT_COMPARE  
            if(_ptCurrFBufInfo[u4InstID]->ucFBufStatus == FRAME)
            {
                vWrite2PC(u4InstID, 6, _pucDecCWorkBuf[u4InstID]);
            }
            #else    
            #if defined(DOWN_SCALE_SUPPORT)
            UINT32 u4DramPicSize = 0x1FE000;
            u4CBase = (UINT32)_pucVDSCLBuf[u4InstID] + u4DramPicSize;
            #else
            u4CBase = u4NonSwapCBase;//(UINT32)_pucDecCWorkBuf[u4InstID];
            #endif  

            #ifdef DIRECT_DEC
            if((_u4FileCnt[u4InstID] >= _u4StartCompPicNum[u4InstID]) && ( _u4FileCnt[u4InstID] <= _u4EndCompPicNum[u4InstID]))
            #endif      
            {
                #ifdef GOLDEN_128BIT_COMP
                UINT32 u4WidthC = u4Width / 2;
                UINT32 u4HeightC = u4Height / 2;
                u4Tx0 = ( (u4WidthC+7) >> 3);   // w/ 8
                u4Ty0 = ( (u4HeightC+15) >> 4);  // h /16
                u4X = (u4WidthC & 0x7);    // w % 8
                u4Y = (u4HeightC & 0xF);  // h % 16
                u4Tx1 = (u4X==0)? u4Tx0 : (u4Tx0+1);
                u4Ty1 = (u4Y==0)? u4Ty0 : (u4Ty0+1);

                for (mbh=0; mbh < u4Ty1; mbh++)
                {
                    for (mbw=0; mbw < u4Tx1; mbw++)
                    {
                        u4Start = (mbh*u4Tx1 + mbw) * (16*16);
                        pbGoldenBuf = (UCHAR*) (((UINT32) (_pucDumpCBuf[u4InstID])) + u4Start);
                        pbDecBuf = (UCHAR*) (u4CBase + u4Start);

                        for(j=0; j < 16; j++)
                        {
                            for(i=0; i < 8; i++)
                            {                       
                                if(1)
                                {
                                    if(  (mbw == u4Tx0 && i >= u4X) || (mbh == u4Ty0 && j >= u4Y))
                                    {
                                        //Do not compare
                                        pbGoldenBuf+=2;
                                        pbDecBuf+=2;
                                    }
                                    else
                                    {
                                        //Compare Cb
                                        if ((*(pbDecBuf)) != (*(pbGoldenBuf)))
                                        {
                                            u4XPix = mbw * 8 + i;
                                            u4YPix = mbh * 16 + j;
                                            u4Cnt ++;
                                            vVDecOutputDebugString("Cb Data Mismatch at [x= 0x%d, y=0x%d] = 0x%x, Golden = 0x%x !!! \n", u4XPix, u4YPix, (*pbDecBuf), (*pbGoldenBuf));
                                            sprintf(strMessage,"Cb Data Mismatch at [x= 0x%d, y=0x%d] = 0x%x, Golden = 0x%x !!! \n", u4XPix, u4YPix, (*pbDecBuf), (*pbGoldenBuf));
                                            fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
                                            //fprintf(_tFileListRecInfo.fpFile, "Y Data Mismatch at [0x%.8x] = 0x%.2x, Golden = 0x%.2x !!! \n", i, pbDecBuf[i], _pucDumpBuf[i]);
                                            fgDecErr = TRUE;
                                            //vVDEC_HAL_VP6_VDec_DumpReg(u4InstID, FALSE);  // mark by ginny
                                            break;    
                                        }

                                        pbGoldenBuf++;
                                        pbDecBuf++;
                                        //Compare Cr
                                        if ((*(pbDecBuf)) != (*(pbGoldenBuf)))
                                        {
                                            u4XPix = mbw * 8 + i;
                                            u4YPix = mbh * 16 + j;
                                            u4Cnt ++;
                                            vVDecOutputDebugString("Cr Data Mismatch at [x= 0x%d, y=0x%d] = 0x%x, Golden = 0x%x !!! \n", u4XPix, u4YPix, (*pbDecBuf), (*pbGoldenBuf));
                                            sprintf(strMessage,"Cr Data Mismatch at [x= 0x%d, y=0x%d] = 0x%x, Golden = 0x%x !!! \n", u4XPix, u4YPix, (*pbDecBuf), (*pbGoldenBuf));
                                            fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
                                            //fprintf(_tFileListRecInfo.fpFile, "Y Data Mismatch at [0x%.8x] = 0x%.2x, Golden = 0x%.2x !!! \n", i, pbDecBuf[i], _pucDumpBuf[i]);
                                            fgDecErr = TRUE;
                                            //vVDEC_HAL_VP6_VDec_DumpReg(u4InstID, FALSE);  // mark by ginny                                            
                                            break;    
                                        }
                                        pbGoldenBuf++;
                                        pbDecBuf++;
                                    }
                                }
                                else
                                {
                                    pbGoldenBuf+=2;
                                    pbDecBuf+=2;
                                }

                            }
                            if(fgDecErr == TRUE)
                            {
                                break;
                            }
                        }

                        if(fgDecErr == TRUE)
                        {
                            break;
                        }
                    }

                    if(fgDecErr == TRUE)
                    {
                        break;
                    }
                }
                #else                          
                for (u4Pix = 0; u4Pix < u4Width*(u4Height>>1); u4Pix++)
                {
                    if((_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecVP6DecPrm.fgDec2ndFld) || (_tVerMpvDecPrm[u4InstID].ucPicStruct == FRM_PIC))
                    {
                        pbDecBuf = (UCHAR*)(u4CBase+u4Pix);
                        pbGoldenBuf = (UCHAR*)((UINT32)_pucDumpCBuf[u4InstID]+u4Pix);
                        if ((*(pbDecBuf)) != (*(pbGoldenBuf)))
                        {
                        #if defined(DOWN_SCALE_SUPPORT)
                            if(VDEC_PP_ENABLE)
                            {
                                if ((*(pbDecBuf)) > (*(pbGoldenBuf)) && (*(pbDecBuf))<= (*(pbGoldenBuf) + 1)  || 
                                    (*(pbDecBuf)) < (*(pbGoldenBuf)) && (*(pbDecBuf))>= (*(pbGoldenBuf) - 1) )
                                    {
                                        //Pass
                                        //How the C Code round off floating number method is not the same as HW in full agreement
                                        //Therefor, difference between +-1 is tolerated
                                    }
                            }
                            else
                        #endif
                            {
                            u4Cnt ++;
                            vVDecOutputDebugString("C Data Mismatch at [%d] = 0x%.2x, Golden = 0x%.2x !!! \n", u4Pix, (*pbDecBuf), (*pbGoldenBuf));
                            sprintf(strMessage,"C Data Mismatch at [%d] = 0x%.2x, Golden = 0x%.2x !!! \n", u4Pix, (*pbDecBuf), (*pbGoldenBuf));
                            fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
                            //fprintf(_tFileListRecInfo.fpFile, "Y Data Mismatch at [0x%.8x] = 0x%.2x, Golden = 0x%.2x !!! \n", i, pbDecBuf[i], _pucDumpBuf[i]);
                            fgDecErr = TRUE;
                            //vVDEC_HAL_VP6_VDec_DumpReg(u4InstID, FALSE);  // mark by ginny                            
                            break;    
                        }
                    }
                    }
                    if(fgDecErr == TRUE)
                    {
                        break;
                    }
                }
                #endif          
                //vVDecOutputDebugString("CbCr Data Compare Over!!! Total bytes [0x%.8x] & error [%d]\n", _u4GoldenCSize[_u4VDecID], u4Cnt);
            }
            #endif    
        }
        #endif
    }

    #ifndef IDE_WRITE_SUPPORT
    if((_u4FileCnt[u4InstID]%10) == 0)
    #endif
    {
        #ifndef IDE_WRITE_SUPPORT
        vVDecOutputDebugString("Pic count to [%d]\n", _u4FileCnt[u4InstID]);  
        #endif
        printk("Frame [%d]: OK\n ", _u4FileCnt[u4InstID]);
    }
    #endif

    #ifdef REDEC    
       if(_u4ReDecCnt[u4InstID] == 0)
    #endif    
          _u4FileCnt[u4InstID] ++;

    #ifndef INTERGRATION_WITH_DEMUX
    // Check if still pic needed compare
 if((_u4FileCnt[u4InstID] >= _u4StartCompPicNum[u4InstID]) && ( _u4FileCnt[u4InstID] <= _u4EndCompPicNum[u4InstID]))
    if (fgCompare)
    {
    #if (defined(COMP_HW_CHKSUM) && (!defined(DOWN_SCALE_SUPPORT)))
    _tTempFileInfo[u4InstID].fgGetFileInfo = TRUE;  
    _tTempFileInfo[u4InstID].pucTargetAddr = _pucDumpYBuf[u4InstID];
    _tTempFileInfo[u4InstID].u4TargetSz = GOLD_Y_SZ;  
    _tTempFileInfo[u4InstID].u4FileLength = 0; 
    vConcateStr(_bFileStr1[u4InstID][3], _bFileStr1[u4InstID][0], "_chksum.bin\0", _u4FileCnt[u4InstID]);
    #ifdef IDE_READ_SUPPORT
    fgOpen = fgPureOpenIdeFile( _bTempStr1[u4InstID],"r+b", &_tTempFileInfo[u4InstID]);
    #else
    fgOpen = fgOpenFile(u4InstID, _bTempStr1[u4InstID],"r+b", &_tTempFileInfo[u4InstID]);
    #endif
    if(fgOpen == FALSE)
    {
        printk("%s open error\n", _bTempStr1[u4InstID]);
    }
    #else
    _tFBufFileInfo[u4InstID].fgGetFileInfo = FALSE;  
    _tFBufFileInfo[u4InstID].pucTargetAddr = _pucDumpYBuf[u4InstID];
    _tFBufFileInfo[u4InstID].u4TargetSz = GOLD_Y_SZ;  
    _tFBufFileInfo[u4InstID].u4FileLength = 4;      
    vConcateStr(_bFileStr1[u4InstID][3], _bPatternPath, _bFileAddStrY, _u4FileCnt[u4InstID]);
    
    #ifdef IDE_READ_SUPPORT
    fgOpen = fgPureOpenIdeFile( _bFileStr1[u4InstID][3],"r+b", &_tFBufFileInfo[u4InstID]);
    #else
    fgOpen = fgOpenFile(u4InstID, _bFileStr1[u4InstID][3],"r+b", &_tFBufFileInfo[u4InstID]);
    #endif
    if(fgOpen == FALSE)
    {
        printk("%s open error\n", _bFileStr1[u4InstID][3]);
    }
    #endif

    if((fgOpen == FALSE) ||(fgDecErr == TRUE) || (_fgVDecErr[u4InstID] == TRUE))
    {

        *fgNextFrameExist = FALSE;
        
        printk("\n\n");
        if(fgOpen == FALSE)
        {
            printk(" Compare Finish==> Pic count to [%d] \n", _u4FileCnt[u4InstID] - 1);
            if(_u4FileCnt[u4InstID] == 1)
            {
                if(fgOpen == FALSE)
                {
                    vVDecOutputDebugString("real NULL\n");
                }
            }
        }
        else
        {
            printk(" Error ==> Pic count to [%d] \n", _u4FileCnt[u4InstID] - 1);     
        }
        _u4VerBitCount[u4InstID] = 0xffffffff;
    }
  }//~fgCompare

    if(_u4FileCnt[u4InstID] >= _u4EndCompPicNum[u4InstID])
    {
        _u4VerBitCount[u4InstID] = 0xffffffff;
    }

}


void vVerAVSVDecEnd(UINT32 u4InstID)
{
    VDEC_INFO_AVS_VFIFO_PRM_T rAvsVFifoInitPrm;
    VDEC_INFO_AVS_BS_INIT_PRM_T rAvsBSInitPrm;
    UINT32 u4VldByte,u4VldBit;
    BOOL   fgNextFrameExist = TRUE;
    VDEC_INFO_AVS_DEC_PRM_T* prVDecAvsDecPrm = &(_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecAVSDecPrm);
    #if !VDEC_AVS_ERR_TEST     
    VDEC_INFO_AVS_SEQ_HDR_T *prVDecAvsSeqHdr = &_rVDecAvsSeqHdr[u4InstID];
    #endif
    VDEC_INFO_AVS_PIC_HDR_T *prPicHdr = prVDecAvsDecPrm->prPicHdr; 



    u4VldByte = u4VDEC_HAL_AVS_ReadRdPtr(0, u4InstID, (UINT32)_pucVFifo[u4InstID], &u4VldBit);

    if ( prPicHdr->u4PicStruct == FRM_PIC || prPicHdr->fgSecField)
    {
        #if !VDEC_AVS_ERR_TEST   
        vAVSWrData2PC(u4InstID, _pucDumpYBuf[u4InstID], ((((prVDecAvsSeqHdr->u2WidthDec+ 15) >> 4) * ((prVDecAvsSeqHdr->u2HeightDec + 31) >> 5)) << 9),  &fgNextFrameExist);
        #else  
        _u4FileCnt[u4InstID]++;

        if (_tInFileInfo[u4InstID].u4FileLength - u4VldByte <1024 || ( _u4FileCnt[u4InstID] >= _u4EndCompPicNum[u4InstID]))
        {
            printk("AVS ERR TEST: OK: PicCnt = %d\n", _u4FileCnt[u4InstID]-1);
            _u4VerBitCount[u4InstID] = 0xffffffff;
        }
        #endif
    }

    // reset HW
    #ifdef REDEC   
    if(_u4ReDecCnt[u4InstID] > 0)
    {
        rAvsVFifoInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
        rAvsVFifoInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
        i4VDEC_HAL_AVS_InitVDecHW(u4InstID,&rAvsVFifoInitPrm);
        rAvsBSInitPrm.u4VFifoSa =  (UINT32)_pucVFifo[u4InstID];
        rAvsBSInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ - 1;
        rAvsBSInitPrm.u4VLDRdPtr =  (UINT32)_pucVFifo[u4InstID] + _u4AVSByteCount[u4InstID];
        rAvsBSInitPrm.u4VLDWrPtr = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ - 1;
        i4VDEC_HAL_AVS_InitBarrelShifter(0, u4InstID, &rAvsBSInitPrm);  
        _tVerDec[u4InstID].ucState = DEC_NORM_WAIT_TO_DEC;
        return;
    }
    #endif

    if (fgNextFrameExist)
    {
        _u4AVSByteCount[u4InstID] = u4VldByte;
        rAvsVFifoInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
        rAvsVFifoInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
        i4VDEC_HAL_AVS_InitVDecHW(u4InstID,&rAvsVFifoInitPrm);
        rAvsBSInitPrm.u4VFifoSa =  (UINT32)_pucVFifo[u4InstID];
        rAvsBSInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ - 1;
        rAvsBSInitPrm.u4VLDRdPtr =  (UINT32)_pucVFifo[u4InstID] + _u4AVSByteCount[u4InstID];
        rAvsBSInitPrm.u4VLDWrPtr = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ - 1;
        i4VDEC_HAL_AVS_InitBarrelShifter(_u4BSID[u4InstID], u4InstID, &rAvsBSInitPrm);

        printk("[AVS] Set Readpointer to 0x%X\n", rAvsBSInitPrm.u4VLDRdPtr);
    }
    #endif
    _tVerDec[u4InstID].ucState = DEC_NORM_VPARSER;

}



// *********************************************************************
// Function    : BOOL fgIsVP6VDecComplete(UINT32 u4InstID)
// Description : Check if VDec complete with interrupt
// Parameter   : None
// Return      : None
// *********************************************************************
BOOL fgIsAVSVDecComplete(UINT32 u4InstID)
{
    UINT32 u4MbX;
    UINT32 u4MbY;   
    VDEC_INFO_AVS_DEC_PRM_T* prVDecAvsDecPrm = &(_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecAVSDecPrm);
    VDEC_INFO_AVS_SEQ_HDR_T *prSeqHdr = prVDecAvsDecPrm->prSeqHdr;
    UINT32 u4MbH;
    if (prSeqHdr->u4IsProgSeq)
    u4MbH =  (prSeqHdr->u2HeightDec / 16 -1);
    else
    u4MbH =  (prSeqHdr->u2HeightDec / 32 -1);

    if(_fgVDecComplete[u4InstID])
    {
        vVDEC_HAL_AVS_GetMbxMby(u4InstID, &u4MbX, &u4MbY);

        if( (u4MbX < (prSeqHdr->u2WidthDec / 16 - 1) ) || (u4MbY < u4MbH) )
        {
            return FALSE;
        }
        else
        {
            return TRUE;
        }    
    }
    return FALSE;
}

void vVerAVSDecEndProc(UINT32 u4InstID)
{
    UINT32 u4Cnt;
    UINT32 u4CntTimeChk;
    UINT32 u4MbX;
    UINT32 u4MbY;  
    #if !VDEC_AVS_ERR_TEST     
    UCHAR strMessage[256];
    #endif
    UINT32 u4MbX_last;
    UINT32 u4MbY_last;
    UINT32 u4AvsErrType = 0; 
    VDEC_INFO_AVS_DEC_PRM_T* prVDecAvsDecPrm = &(_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecAVSDecPrm);
    VDEC_INFO_AVS_SEQ_HDR_T *prSeqHdr = prVDecAvsDecPrm->prSeqHdr; 
    VDEC_INFO_AVS_PIC_HDR_T *prPicHdr = prVDecAvsDecPrm->prPicHdr; 
    VDEC_INFO_AVS_ERR_INFO_T rAvsErrInfo;

    rAvsErrInfo.u2AvsMBErrCnt = 0;
    rAvsErrInfo.u4AvsErrCnt = 0;
    rAvsErrInfo.u4AvsErrRow = 0;
    rAvsErrInfo.u4AvsErrType = 0;   
    u4Cnt=0;
    u4CntTimeChk = 0;
    _fgVDecErr[u4InstID] = FALSE;

    //while(u4CntTimeChk < DEC_RETRY_NUM)
    while(TRUE)
    {    
        u4Cnt ++;    
        if((u4Cnt & 0x3f)== 0x3f)
        {
            #ifndef IRQ_DISABLE    
            #else
            //if( u4VDEC_HAL_AVS_VDec_ReadFinishFlag(u4InstID) & 0x1)
            if( u4VDEC_HAL_AVS_VDec_ReadFinishFlag(u4InstID))
            {
                printk("AVS detect int\n");
                _fgVDecComplete[u4InstID] = TRUE;
                u4VDEC_HAL_AVS_VDec_ClearInt(u4InstID);
            }
            #endif      
            if(fgIsAVSVDecComplete(u4InstID))
            {
                #ifdef CAPTURE_ESA_LOG
                vWrite2PC(u4InstID, 17, (UCHAR*)_pucESALog[u4InstID]);
                #endif
                u4CntTimeChk = 0;
                break;
            }
            else
            {
                u4MbX_last = u4MbX;
                u4MbY_last = u4MbY;
                vVDEC_HAL_AVS_GetMbxMby(u4InstID, &u4MbX, &u4MbY);
                if((u4MbX == u4MbX_last) && (u4MbY == u4MbY_last))
                {
                    u4CntTimeChk ++;
                }
                else
                {
                    u4CntTimeChk =0;
                }
            }
            u4Cnt = 0;
        }
    }

    #if !VDEC_AVS_ERR_TEST
    if((u4CntTimeChk == DEC_RETRY_NUM) || (u4AvsErrType!= 0) || (rAvsErrInfo.u4AvsErrCnt != 0))
    {
        #ifndef INTERGRATION_WITH_DEMUX
        //#ifdef EXT_COMPARE     
        _fgVDecErr[u4InstID] = TRUE;
        //#endif
        if(u4CntTimeChk == DEC_RETRY_NUM)
        {
            vVDecOutputDebugString("\n!!!!!!!!! Decoding Timeout !!!!!!!\n");
            sprintf(strMessage, "%s", "\n!!!!!!!!! Decoding Timeout !!!!!!!");
            fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
            //vDumpReg();
        }
        vVDEC_HAL_AVS_GetMbxMby(u4InstID, &u4MbX, &u4MbY);
        vVDecOutputDebugString("\n!!!!!!!!! Decoding Error 0x%.8x!!!!!!!\n", rAvsErrInfo.u4AvsErrType);
        sprintf(strMessage,"\n!!!!!!!!! Decoding Error 0x%.8x 0x%.8x 0x%.8xat MC (x,y)=(%d/%d, %d/%d)  !!!!!!!\n", u4AvsErrType, 
        rAvsErrInfo.u4AvsErrType,rAvsErrInfo.u4AvsErrRow,u4MbX, prSeqHdr->u4HSize -1, u4MbY, prSeqHdr->u4VSize -1);
        fgWrMsg2PC((void*)strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
        #endif
    }
    #else
    u4AvsErrType = u4VDEC_HAL_AVS_GetErrType(u4InstID, &rAvsErrInfo);

    if ((u4CntTimeChk == DEC_RETRY_NUM) && (u4AvsErrType>>31))
    {
        printk("AVS ERR Bitstream TimeOut!!!!\n");
    }
    #endif

    //vVerifySetVSyncPrmBufPtr(u4InstID, _u4DecBufIdx[u4InstID]);

    vVerAVSVDecEnd(u4InstID);
    if ( prPicHdr->u4PicStruct == FRM_PIC || prPicHdr->fgSecField)
    {
        vVerAVSUpdateBufStatus(u4InstID);
    }    

    if (!prSeqHdr->u4IsProgSeq)
    {
        prPicHdr->fgSecField = (!prPicHdr->fgSecField);
    }
}



