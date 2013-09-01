#include "vdec_verify_mpv_prov.h"
#include "../hal/vdec_hal_if_common.h"
#include "../hal/vdec_hal_if_vp6.h"
#include "vdec_verify_keydef.h"
#include "vdec_verify_vparser_vp6.h"
#include "../include/vdec_info_vp6.h"
#include "../include/vdec_drv_vp6_info.h"
#include "vdec_verify_file_common.h"
#include "vdec_verify_filesetting.h"
#include <linux/string.h>

extern UINT32 u4VDecReadVP6VLD(UINT32 u4VDecID, UINT32 u4Addr);
extern int rand(void);
extern void vVerifySetVSyncPrmBufPtr(UINT32 u4InstID, UINT32 u4BufIdx);

INT32 i4VP6_Parse_Frame_Header(UINT32 u4BsId, UINT32 u4InstID, VDEC_INFO_VP6_FRM_HDR_T *prVDecVp6FrmHdr);
INT32 i4VP6_Parse_Intra_Header(UINT32 u4BsId, UINT32 u4InstID, VDEC_INFO_VP6_FRM_HDR_T *prVDecVp6FrmHdr);
INT32 i4VP6_Parse_Inter_Header(UINT32 u4BsId, UINT32 u4InstID, VDEC_INFO_VP6_FRM_HDR_T *prVDecVp6FrmHdr);
BOOL fgIsVP6VDecComplete(UINT32 u4InstID);


extern UINT32 u4VDecReadVP6DCAC(UINT32 u4VDecID, UINT32 u4Addr);
extern UINT32 u4VDecReadVP6VLD(UINT32 u4VDecID, UINT32 u4Addr);
extern UINT32 u4VDecReadVP6PP(UINT32 u4VDecID, UINT32 u4Addr);
extern UINT32 u4VDecVP6ReadVLD2(UINT32 u4Addr);
extern UINT32 u4VDecReadVLD(UINT32 u4VDecID,UINT32 u4Addr);


void vp6_dump_register(UINT32 u4InstID)
{
    UINT32 i;
    printk("\n\n<vdec> VP6_VLD_REG_42 ~ VP6_VLD_REG_92\n");
    for(i = 42; i <= 92; i++)
    {
        u4VDecReadVP6VLD(u4InstID, i * 4);
    }

    printk("\n<vdec> MC_REG_0 ~ MC_REG_640\n");
    for (i = 0; i <= 640; i++)
    {
        //u4VDecReadVP6MC(u4InstID, i*4);
        u4VDecReadMC(u4InstID, i*4);
    }

    printk("\n<vdec> DCAC_REG_0 ~ DCAC_REG_21\n");
    for (i = 0; i <= 21; i++)
    {
        u4VDecReadVP6DCAC(u4InstID, i*4);
    }

    printk("\n<vdec> VP6_PP_REG_0 ~ VP6_PP_REG_639\n");
    for (i = 0; i <= 639; i++)
    {
        u4VDecReadVP6PP(u4InstID, i*4);
    }

    printk("\n<vdec> VP6_VLD2_REG_44 ~ VP6_VLD2_REG_84\n");
    for(i = 44; i <= 84; i++)
    {
        u4VDecVP6ReadVLD2(i * 4);
    }

    printk("\n<vdec> VLD_REG_44 ~ VLD_REG_84\n");
    for(i = 44; i <= 84; i++)
    {
        u4VDecReadVLD(u4InstID, i * 4);
    }
}

void vVerInitVP6(UINT32 u4InstID)
{             
    //Open size file

    //Init VP6 Frame Header
    VDEC_INFO_VP6_VFIFO_PRM_T     rVp6VDecInitPrm;
    VDEC_INFO_VP6_BS_INIT_PRM_T  rVp6BSInitPrm;

    _u4TotalDecFrms[u4InstID] = 0;
#ifdef BARREL2_THREAD_SUPPORT
    VERIFY (x_sema_lock(_ahVDecEndSema[u4InstID], X_SEMA_OPTION_WAIT) == OSR_OK);
#endif
    rVp6VDecInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
    rVp6VDecInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
    i4VDEC_HAL_VP6_InitVDecHW(u4InstID, &rVp6VDecInitPrm);
    rVp6BSInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
    rVp6BSInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
    rVp6BSInitPrm.u4ReadPointer = (UINT32)_pucVFifo[u4InstID];
    rVp6BSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
#ifndef  RING_VFIFO_SUPPORT
    rVp6BSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
#else
    //rVp6BSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ*(0.5 + 0.5 *(_u4LoadBitstreamCnt[u4InstID]%2))); // panda
        rVp6BSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));
#endif
    i4VDEC_HAL_VP6_InitBarrelShifter(_u4BSID[u4InstID], u4InstID, &rVp6BSInitPrm);
    u4VDEC_HAL_VP6_VDec_SetWorkspace(u4InstID, (UINT32)_pucVP6VLDWrapperWorkspace[u4InstID], (UINT32)_pucVP6PPWrapperWorkspace[u4InstID]);

#ifdef BARREL2_THREAD_SUPPORT
    VERIFY (x_sema_unlock(_ahVDecEndSema[u4InstID]) == OSR_OK);
#endif

    _u4DecBufIdx[u4InstID] = 0;    //Previous frame buf Index
    _u4PrevfBufIdx[u4InstID] = 1;    //Previous frame buf Index
    _u4GoldfBufIdx[u4InstID] = 2;    //Golden frame buf Index
    _u4VP6ByteCount[u4InstID] = 0;

    //PANDA
    _rVp6BSPrm.u4VFifoSa = rVp6BSInitPrm.u4VFifoSa;
    _rVp6BSPrm.u4VFifoEa = rVp6BSInitPrm.u4VFifoEa;
    _rVp6BSPrm.u4ReadPointer = rVp6BSInitPrm.u4ReadPointer;
    _rVp6BSPrm.u4WritePointer = rVp6BSInitPrm.u4WritePointer;

    // Alpha channel
    if (_u1AlphaBitstream[u4InstID])
    {
        VDEC_INFO_VP6_DEC_PRM_T *prVDecVP6DecPrm =  &_tVerMpvDecPrm[0].SpecDecPrm.rVDecVP6DecPrm;

        u4VDec_HAL_VP6_VDec_BackupSram(u4InstID, prVDecVP6DecPrm);

        prVDecVP6DecPrm =  &_tVerMpvDecPrm[1].SpecDecPrm.rVDecVP6DecPrm;
        u4VDec_HAL_VP6_VDec_BackupSram(u4InstID, prVDecVP6DecPrm);

        _u4DecBufIdx[1] = 0;    //Previous frame buf Index
        _u4PrevfBufIdx[1] = 1;    //Previous frame buf Index
        _u4GoldfBufIdx[1] = 2;    //Golden frame buf Index
    }
}


// *********************************************************************
// Function : BOOL u4VerVParserVP6(UINT32 u4InstID)
// Description :
// Parameter :
// Return    :
// *********************************************************************
UINT32 u4VerVParserVP6(UINT32 u4InstID, BOOL fgInquiry)
{
    UINT32 u4Size0 = 0, u4Size1 = 0, u4Size2 = 0, u4Size3 = 0;
    INT32  i4RetVal;
    UINT32 u4BsId = 0;
    //    VDEC_INFO_VP6_VFIFO_PRM_T rVp6VFifoInitPrm;
    VDEC_INFO_VP6_BS_INIT_PRM_T rVp6BSInitPrm;
    VDEC_INFO_VP6_FRM_HDR_T *prVDecVp6FrmHdr;// = &_rVDecVp6FrmHdr[u4InstID];

#ifndef VP6_ALPHA_SUPPORT
    if ((_u1AlphaFlag[u4InstID] & (VP6_ALPHA_ENABLE | VP6_ALPHA_FRAME)) == (VP6_ALPHA_ENABLE | VP6_ALPHA_FRAME))
    {
        printk("<vdec> VP6 does not support ALPHA\n");
        return (UINT32)(-1);
    }
#endif

    //_u1AlphaFlag[u4InstID] = VP6_ALPHA_ENABLE | VP6_ALPHA_FRAME;  // test alpha frame

#ifdef VP6_ALPHA_SUPPORT
    _u1AlphaDecPrmIdx[u4InstID] = (_u1AlphaFlag[u4InstID] & VP6_ALPHA_ENABLE) ? ((_u1AlphaFlag[u4InstID] & VP6_ALPHA_FRAME) ? 1 : 0) : 0;
    printk("<vdec> u1AlphaFalg[%d]=%d, _u1AlphaDecPrmIdx[%d]=%d\n", u4InstID, _u1AlphaFlag[u4InstID], u4InstID, _u1AlphaDecPrmIdx[u4InstID]);

    prVDecVp6FrmHdr = &_rVDecVp6FrmHdr[_u1AlphaDecPrmIdx[u4InstID]];

    if (_u1AlphaFlag[u4InstID] & VP6_ALPHA_ENABLE)
    {
        UINT8 *pData;
        UINT32 u4YCbCrOfst, u4AlphaOfst;
        VDEC_INFO_VP6_DEC_PRM_T *prVDecVP6DecPrm =  &_tVerMpvDecPrm[_u1AlphaDecPrmIdx[u4InstID]].SpecDecPrm.rVDecVP6DecPrm;

        prVDecVP6DecPrm->u1AlphaFlag = _u1AlphaFlag[u4InstID];

        u4VDec_HAL_VP6_VDec_RestoreSram(u4InstID, prVDecVP6DecPrm);

        //rVp6VFifoInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
        //rVp6VFifoInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
        //i4VDEC_HAL_VP6_InitVDecHW(u4InstID, &rVp6VFifoInitPrm);
        rVp6BSInitPrm.u4VFifoSa =  (UINT32)_pucVFifo[u4InstID];
        rVp6BSInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;

        pData = _pucVFifo[u4InstID] + _u4VP6ByteCount[u4InstID];
        u4YCbCrOfst = (UINT32)(pData + 3);
        u4AlphaOfst = ((((UINT32)pData[0])<<16) | (((UINT32)pData[1])<<8) | (((UINT32)pData[2])<<0)) + u4YCbCrOfst;
        if (_u1AlphaFlag[u4InstID] & VP6_ALPHA_FRAME)
        {
            //u4AlphaOfst = ((((UINT32)pData[0])<<16) | (((UINT32)pData[1])<<8) | (((UINT32)pData[2])<<0)) + u4YCbCrOfst;
            rVp6BSInitPrm.u4ReadPointer= u4AlphaOfst;
        }
        else
        {
            rVp6BSInitPrm.u4ReadPointer= u4YCbCrOfst;
        }

        printk("<vdec> [%u]: _u4VP6ByteCount[%d]=%u (0x%x), u4YCbCrOfst=%u (0x%x), u4AlphaOfst=%u (0x%x)\n",
            _u4FileCnt[u4InstID], u4InstID, _u4VP6ByteCount[u4InstID], _u4VP6ByteCount[u4InstID],
            u4YCbCrOfst - (UINT32)_pucVFifo[u4InstID], u4YCbCrOfst - (UINT32)_pucVFifo[u4InstID], 
            u4AlphaOfst - (UINT32)_pucVFifo[u4InstID], u4AlphaOfst - (UINT32)_pucVFifo[u4InstID]);

#ifndef  RING_VFIFO_SUPPORT
        rVp6BSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
#else
        //rVp6BSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ*(0.5 + 0.5 *(_u4LoadBitstreamCnt[u4InstID]%2))); // panda
           rVp6BSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));
#endif
        i4VDEC_HAL_VP6_InitBarrelShifter(_u4BSID[u4InstID], u4InstID, &rVp6BSInitPrm);

        //MULTI-STREAM PANDA
        _rVp6BSPrm.u4VFifoSa = rVp6BSInitPrm.u4VFifoSa;
        _rVp6BSPrm.u4VFifoEa = rVp6BSInitPrm.u4VFifoEa;
        _rVp6BSPrm.u4ReadPointer = rVp6BSInitPrm.u4ReadPointer;
        _rVp6BSPrm.u4WritePointer = rVp6BSInitPrm.u4WritePointer;
    }
#endif

    //MULTI-STREAM PANDA
    u4Size0 = *(_pucSizeFileBuf[u4InstID] + ( (_u4FileCnt[u4InstID] *4)+4 ) );
    u4Size1 = *(_pucSizeFileBuf[u4InstID] + ( (_u4FileCnt[u4InstID] *4) +5));
    u4Size2 = *(_pucSizeFileBuf[u4InstID] + ( (_u4FileCnt[u4InstID] *4) +6));
    u4Size3 = *(_pucSizeFileBuf[u4InstID] + ( (_u4FileCnt[u4InstID] *4) +7));
    prVDecVp6FrmHdr->u4FrameSize  = ( ((u4Size3 & 0xFF) << 24) | ((u4Size2 & 0xFF) << 16) | ((u4Size1 & 0xFF) << 8) | (u4Size0 & 0xFF) );

    printk("<vdec> u4FrameSize=%u (0x%x)\n", prVDecVp6FrmHdr->u4FrameSize, prVDecVp6FrmHdr->u4FrameSize);

    i4RetVal = i4VP6_Parse_Frame_Header(u4BsId, u4InstID, prVDecVp6FrmHdr);

    if(i4RetVal != 0)
    {
        return(i4RetVal);
    }   

    return(i4RetVal);
}

INT32 i4VP6_Parse_Frame_Header(UINT32 u4BsId, UINT32 u4InstID, VDEC_INFO_VP6_FRM_HDR_T *prVDecVp6FrmHdr)
{
    const UCHAR vp56_filter_threshold[] = {
        14, 14, 13, 13, 12, 12, 10, 10,
        10, 10,  8,  8,  8,  8,  8,  8,
        8,  8,  8,  8,  8,  8,  8,  8,
        8,  8,  8,  8,  8,  8,  8,  8,
        8,  8,  8,  8,  7,  7,  7,  7,
        7,  7,  6,  6,  6,  6,  6,  6,
        5,  5,  5,  5,  4,  4,  4,  4,
        4,  4,  4,  3,  3,  3,  3,  2,
    };

    const UCHAR vp56_ac_dequant[64] = {
        94, 92, 90, 88, 86, 82, 78, 74,
        70, 66, 62, 58, 54, 53, 52, 51,
        50, 49, 48, 47, 46, 45, 44, 43,
        42, 40, 39, 37, 36, 35, 34, 33,
        32, 31, 30, 29, 28, 27, 26, 25,
        24, 23, 22, 21, 20, 19, 18, 17,
        16, 15, 14, 13, 12, 11, 10,  9,
        8,  7,  6,  5,  4,  3,  2,  1,
    };

    const UCHAR vp56_dc_dequant[64] = {
        47, 47, 47, 47, 45, 43, 43, 43,
        43, 43, 42, 41, 41, 40, 40, 40,
        40, 35, 35, 35, 35, 33, 33, 33,
        33, 32, 32, 32, 27, 27, 26, 26,
        25, 25, 24, 24, 23, 23, 19, 19,
        19, 19, 18, 18, 17, 16, 16, 16,
        16, 16, 15, 11, 11, 11, 10, 10,
        9,  8,  7,  5,  3,  3,  2,  2,
    };

    INT32 i4RetVal = VDEC_NONE_ERROR;
    UINT32 u4Datain, u4VldBit;
    UINT32 u4FrameHeader;

    printk("<vdec> Input window is 0x%x (%s, %d)\n", u4VDecReadVP6VLD(u4InstID, 0xF0), __FUNCTION__, __LINE__);
    //u4VDecReadVldRPtr(u4BsId, u4InstID, &u4VldBit, PHYSICAL((UINT32)_pucVFifo[u4InstID]));

    prVDecVp6FrmHdr->u2Buff2Offset = 0;
    prVDecVp6FrmHdr->u2LoopFilter = 1;
    prVDecVp6FrmHdr->fgParse_Filter_Info = FALSE;

    u4Datain = u4VDEC_HAL_VP6_GetBitStreamShift(u4BsId, u4InstID, 8);
    u4FrameHeader = u4Datain >> 24;
    prVDecVp6FrmHdr->ucFrameType = (u4FrameHeader  & 0x80)? VP6_P_FRM: VP6_I_FRM;

    prVDecVp6FrmHdr->ucDctQMask = ((u4FrameHeader & 0x7E) >> 1);       
    prVDecVp6FrmHdr->u2Vp56_Filter_Threshold = vp56_filter_threshold[prVDecVp6FrmHdr->ucDctQMask];
    prVDecVp6FrmHdr->u4DQuant_Dc = vp56_dc_dequant[prVDecVp6FrmHdr->ucDctQMask] << 2;
    prVDecVp6FrmHdr->u4DQuant_Ac = vp56_ac_dequant[prVDecVp6FrmHdr->ucDctQMask] << 2;

    //MULTI-STREAM PANDA
#ifdef VP6_MULTI_STREAM
    prVDecVp6FrmHdr->fgMultiStream= (u4FrameHeader & 0x1);
    //printk("fgMultiStream = %d\n", prVDecVp6FrmHdr->fgMultiStream);
#else
    prVDecVp6FrmHdr->fgMultiStream= 0;
#endif

    if (prVDecVp6FrmHdr->ucFrameType == VP6_I_FRM)  //I-VOP
    {
        printk("<vdec> vp6  I_FRM\n");
        i4VP6_Parse_Intra_Header(u4BsId, u4InstID, prVDecVp6FrmHdr);
    } 
    else //P-VOP
    {
        printk("<vdec> vp6 P_FRM\n");
        i4VP6_Parse_Inter_Header(u4BsId, u4InstID, prVDecVp6FrmHdr);
    }

    if (prVDecVp6FrmHdr->fgParse_Filter_Info)
    {
        //Parse Advanced Profile Header.       
        prVDecVp6FrmHdr->fgAutoSelectPMFlag =   (u4VDEC_HAL_VP6_GetBoolCoderShift(u4BsId, u4InstID, 1) )? TRUE: FALSE;        

        if (prVDecVp6FrmHdr->fgAutoSelectPMFlag)
        {
            prVDecVp6FrmHdr->ucFilter_Mode = 2;
            prVDecVp6FrmHdr->u4PredictionFilterVarThresh = (u4VDEC_HAL_VP6_GetBoolCoderShift(u4BsId, u4InstID, 5) );
            prVDecVp6FrmHdr->u4PredictionFilterMvSizeThresh = (u4VDEC_HAL_VP6_GetBoolCoderShift(u4BsId, u4InstID, 3) );

            prVDecVp6FrmHdr->u4Sample_Variance_Threshold = (prVDecVp6FrmHdr->u4PredictionFilterVarThresh << prVDecVp6FrmHdr->ucVrt_Shift);
            prVDecVp6FrmHdr->u4Max_Vector_Length = 2 << prVDecVp6FrmHdr->u4PredictionFilterMvSizeThresh;

            //HW related variable
            if (prVDecVp6FrmHdr->u4Max_Vector_Length)
            {
                prVDecVp6FrmHdr->u4Mv_Thr_En = 1; //enable              
            }
            else
            {
                prVDecVp6FrmHdr->u4Mv_Thr_En = 0;
            }

            if (prVDecVp6FrmHdr->u4Sample_Variance_Threshold)
            {
                prVDecVp6FrmHdr->u4Var_Thr_En = 1;             
            }
            else
            { 
                prVDecVp6FrmHdr->u4Var_Thr_En = 0;
            } 

        }
        else
        {
            prVDecVp6FrmHdr->fgBiCubicOrBiLinearFlag = (u4VDEC_HAL_VP6_GetBoolCoderShift(u4BsId, u4InstID, 1) )? TRUE: FALSE;
            if (prVDecVp6FrmHdr->fgBiCubicOrBiLinearFlag) 
            {
                prVDecVp6FrmHdr->ucFilter_Mode = 1;
                prVDecVp6FrmHdr->u4BilinearFilter = 0;
            }
            else
            {
                prVDecVp6FrmHdr->ucFilter_Mode = 0;
                prVDecVp6FrmHdr->u4BilinearFilter = 1;
            }
            prVDecVp6FrmHdr->u4Var_Thr_En = 0;
        }

        if (prVDecVp6FrmHdr->ucVp3VerNo > 7)
        {
            prVDecVp6FrmHdr->u4Filter_Selection = (u4VDEC_HAL_VP6_GetBoolCoderShift(u4BsId, u4InstID, 4) );
        }
        else
        {
            prVDecVp6FrmHdr->u4Filter_Selection = 16;
        }
    }

    prVDecVp6FrmHdr->fgUseHuffman = (u4VDEC_HAL_VP6_GetBoolCoderShift(u4BsId, u4InstID, 1) )? TRUE: FALSE;      // //// eason : marker2

    //MULTI-STREAM PANDA
#ifdef VP6_MULTI_STREAM
    if (prVDecVp6FrmHdr->u2Buff2Offset) 
    {
        VDEC_INFO_VP6_BS_INIT_PRM_T rVp6BSInitPrm;
        rVp6BSInitPrm.u4ReadPointer = _rVp6BSPrm.u4ReadPointer + prVDecVp6FrmHdr->u2Buff2Offset;
        rVp6BSInitPrm.u4WritePointer = _rVp6BSPrm.u4WritePointer - 1024; //_rVp6BSPrm.u4VFifoEa + 0x500000;  //_rVp6BSPrm.u4WritePointer;
        rVp6BSInitPrm.u4VFifoSa = _rVp6BSPrm.u4VFifoSa;
        rVp6BSInitPrm.u4VFifoEa = _rVp6BSPrm.u4VFifoEa;

        printk("<vdec> Setup BarrelShifter 2 for multi-stream, readpointer %x, offset %x\n", _rVp6BSPrm.u4ReadPointer, prVDecVp6FrmHdr->u2Buff2Offset);

        i4VDEC_HAL_VP6_InitBarrelShifter2(1, u4InstID, &rVp6BSInitPrm, prVDecVp6FrmHdr);
        u4VDEC_HAL_VP6_InitBoolCoder(1, u4InstID, 0);  //vp56_init_range_decoder(&s->cc, buf+coeff_offset, buf_size-coeff_offset);
        u4VDEC_HAL_VP6_VDec_SetByteCount(1, u4InstID, prVDecVp6FrmHdr->u4FrameSize - prVDecVp6FrmHdr->u2Buff2Offset);
        printk("<vdec> u4VDEC_HAL_VP6_VDec_SetByteCount  @(%s, %d)\n", __FUNCTION__, __LINE__);
        //u4VDEC_HAL_VP6_InitBoolCoder(u4BsId, u4InstID, 0);  //vp56_init_range_decoder(&s->cc, buf+coeff_offset, buf_size-coeff_offset);        
        //s->ccp = &s->cc;
    }
    else 
    {
        //Do Nothing.
        //s->ccp = &s->c;
    }
#else
    prVDecVp6FrmHdr->fgUseHuffman = FALSE;
#endif

    return i4RetVal;
}

INT32 i4VP6_Parse_Intra_Header(UINT32 u4BsId, UINT32 u4InstID, VDEC_INFO_VP6_FRM_HDR_T *prVDecVp6FrmHdr)
{
    UINT32 u4Datain, u4VldBit;
    //    UINT32 u4Width, u4Height;

    u4Datain = u4VDEC_HAL_VP6_GetBitStreamShift(u4BsId, u4InstID, 8);
    prVDecVp6FrmHdr->ucVp3VerNo = (u4Datain >> 27);
    prVDecVp6FrmHdr->ucVpProfile = (VP6_PROFILE_T) ((u4Datain >> 25) & 0x3);
    prVDecVp6FrmHdr->ucReserved = ((u4Datain >> 24) & 0x1);    

    // assert here CCCI_TX check sub_version with CodecID
    if (prVDecVp6FrmHdr->ucVp3VerNo > 8)
        return 0;

    if (prVDecVp6FrmHdr->ucReserved) 
    {
        return 0;
    }

    if (prVDecVp6FrmHdr->fgMultiStream || prVDecVp6FrmHdr->ucVpProfile == VP6_PROFILE_SIMPLE) 
    {
        u4Datain = u4VDEC_HAL_VP6_GetBitStreamShift(u4BsId, u4InstID, 16);
        prVDecVp6FrmHdr->u2Buff2Offset = (UINT16) ((u4Datain & 0xFFFF0000) >> 16);
        printk("<vdec> [VP6] Get Buff2Offset = %x\n from Intra header", prVDecVp6FrmHdr->u2Buff2Offset);
    }
    else
    {
        prVDecVp6FrmHdr->u2Buff2Offset = 0;
    }

    //Bool Coder: init range decoder
    // eason : dim_x, dim_y
    u4Datain = u4VDEC_HAL_VP6_GetBitStreamShift(u4BsId, u4InstID, 32);
    prVDecVp6FrmHdr->u2VFragments = (UINT16) (u4Datain >> 24 & 0xFF);
    prVDecVp6FrmHdr->u2HFragments = (UINT16) (u4Datain >> 16 & 0xFF);
    prVDecVp6FrmHdr->u2OutVFragments = (UINT16) (u4Datain >> 8 & 0xFF);
    prVDecVp6FrmHdr->u2OutHFragments = (UINT16) (u4Datain  & 0xFF);

    prVDecVp6FrmHdr->u2WidthDec = 16*prVDecVp6FrmHdr->u2HFragments;
    prVDecVp6FrmHdr->u2HeightDec = 16*prVDecVp6FrmHdr->u2VFragments;
    printk("<vdec> u2WidthDec=%u, u2HeightDec=%u\n", prVDecVp6FrmHdr->u2WidthDec, prVDecVp6FrmHdr->u2HeightDec);

    u4VDEC_HAL_VP6_InitBoolCoder(u4BsId, u4InstID, 0);//??? vp56_init_range_decoder(c, buf+6, buf_size-6);

#ifdef VP6_MULTI_STREAM
    if (prVDecVp6FrmHdr->u2Buff2Offset)
    {
        u4VDEC_HAL_VP6_VDec_SetByteCount(u4BsId, u4InstID, prVDecVp6FrmHdr->u2Buff2Offset - 6);
        printk("<vdec> u4VDEC_HAL_VP6_VDec_SetByteCount  @(%s, %d)\n", __FUNCTION__, __LINE__);
    }
    else
    {
        u4VDEC_HAL_VP6_VDec_SetByteCount(u4BsId, u4InstID, prVDecVp6FrmHdr->u4FrameSize - 6);
        printk("<vdec> u4VDEC_HAL_VP6_VDec_SetByteCount  @(%s, %d)\n", __FUNCTION__, __LINE__);
    }
#endif

    prVDecVp6FrmHdr->ucScalingMode = (UCHAR) ((u4VDEC_HAL_VP6_GetBoolCoderShift(u4BsId, u4InstID, 2) ) & 0x3);

    if (prVDecVp6FrmHdr->ucVp3VerNo < 8)
    {
        prVDecVp6FrmHdr->ucVrt_Shift = 5;
    }
    else
    {
        prVDecVp6FrmHdr->ucVrt_Shift = 0;
    }

    printk("<vdec> ucVp3VerNo=%d, ucVrt_Shift=%d\n", prVDecVp6FrmHdr->ucVp3VerNo, prVDecVp6FrmHdr->ucVrt_Shift);

    prVDecVp6FrmHdr->fgParse_Filter_Info = ((prVDecVp6FrmHdr->ucVpProfile == VP6_PROFILE_ADVANCED)? TRUE: FALSE);

    return 1;
}

INT32 i4VP6_Parse_Inter_Header(UINT32 u4BsId, UINT32 u4InstID, VDEC_INFO_VP6_FRM_HDR_T *prVDecVp6FrmHdr)
{
    UINT32 u4Datain, u4VldBit;    

    if (!prVDecVp6FrmHdr->ucVp3VerNo)
        return 0;

    if (prVDecVp6FrmHdr->fgMultiStream || prVDecVp6FrmHdr->ucVpProfile == VP6_PROFILE_SIMPLE) 
    {
        u4Datain = u4VDEC_HAL_VP6_GetBitStreamShift(u4BsId, u4InstID, 16);
        prVDecVp6FrmHdr->u2Buff2Offset = (UINT16) ((u4Datain & 0xFFFF0000) >> 16);
        printk("<vdec> [VP6] Get Buff2Offset = %x from Inter header\n", prVDecVp6FrmHdr->u2Buff2Offset);
    }
    else
    {
        prVDecVp6FrmHdr->u2Buff2Offset = 0;
    }

    u4VDEC_HAL_VP6_InitBoolCoder(u4BsId, u4InstID, 0); //vp56_init_range_decoder(c, buf+1, buf_size-1);

#ifdef VP6_MULTI_STREAM
    if (prVDecVp6FrmHdr->u2Buff2Offset)
    {
        u4VDEC_HAL_VP6_VDec_SetByteCount(u4BsId, u4InstID, prVDecVp6FrmHdr->u2Buff2Offset - 1);
        printk("<vdec> u4VDEC_HAL_VP6_VDec_SetByteCount  @(%s, %d)\n", __FUNCTION__, __LINE__);
    }
    else
    {
        u4VDEC_HAL_VP6_VDec_SetByteCount(u4BsId, u4InstID, prVDecVp6FrmHdr->u4FrameSize - 1);
        printk("<vdec> u4VDEC_HAL_VP6_VDec_SetByteCount  @(%s, %d)\n", __FUNCTION__, __LINE__);
    }
#endif

    //Table 2
    prVDecVp6FrmHdr->fgRefreshGoldenFrame = (u4VDEC_HAL_VP6_GetBoolCoderShift(u4BsId, u4InstID, 1) )? TRUE: FALSE;
    if (prVDecVp6FrmHdr->ucVpProfile == VP6_PROFILE_ADVANCED)
    {
        prVDecVp6FrmHdr->u2LoopFilter = (u4VDEC_HAL_VP6_GetBoolCoderShift(u4BsId, u4InstID, 1) )? 1: 0 ;
        if (prVDecVp6FrmHdr->u2LoopFilter)
            prVDecVp6FrmHdr->fgLoopFilterSelector = (u4VDEC_HAL_VP6_GetBoolCoderShift(u4BsId, u4InstID, 1) )? TRUE: FALSE;

        if (prVDecVp6FrmHdr->ucVp3VerNo > 7)
            prVDecVp6FrmHdr->fgParse_Filter_Info = (u4VDEC_HAL_VP6_GetBoolCoderShift(u4BsId, u4InstID, 1) )? TRUE: FALSE;
    }
    return 1;
}



// *********************************************************************
// Function     : void vVerVP6UpdateBufStatus(UINT32 u4InstID)
// Description : Set VP6 Frame Buffer Status
// Parameter  : None
// Return        : None
// *********************************************************************
void vVerVP6SetBufStatus(UINT32 u4InstID)
{
    //   VDEC_INFO_VP6_FRM_HDR_T *prVDecVp6FrmHdr = &_rVDecVp6FrmHdr[u4InstID];   
    VDEC_INFO_VP6_DEC_PRM_T *prVDecVP6DecPrm =  &(_tVerMpvDecPrm[_u1AlphaDecPrmIdx[u4InstID]].SpecDecPrm.rVDecVP6DecPrm);

    u4InstID = _u1AlphaDecPrmIdx[u4InstID];

    prVDecVP6DecPrm->rVp6PpInfo.fgPpEnable = VDEC_PP_ENABLE;
    prVDecVP6DecPrm->rVp6PpInfo.u4PpYBufSa = (UINT32)_pucPpYSa[u4InstID];
    prVDecVP6DecPrm->rVp6PpInfo.u4PpCBufSa = (UINT32)_pucPpCSa[u4InstID];

    vVDec_InvDCacheRange((UINT32) _pucPpYSa[u4InstID],DEC_PP_Y_SZ);
    vVDec_InvDCacheRange((UINT32) _pucPpCSa[u4InstID],DEC_PP_C_SZ);

    prVDecVP6DecPrm->rVp6PpInfo.u1PpLevel = 1;
    switch (prVDecVP6DecPrm->rVp6PpInfo.u1PpLevel)
    {
    case 1:
        prVDecVP6DecPrm->rVp6PpInfo.au1MBqp[0] = 12;
        prVDecVP6DecPrm->rVp6PpInfo.au1MBqp[1] = 15;
        prVDecVP6DecPrm->rVp6PpInfo.au1MBqp[2] = 26;
        prVDecVP6DecPrm->rVp6PpInfo.au1MBqp[3] = 29;
        break;
    case 2:
        prVDecVP6DecPrm->rVp6PpInfo.au1MBqp[0] = 10;
        prVDecVP6DecPrm->rVp6PpInfo.au1MBqp[1] = 13;
        prVDecVP6DecPrm->rVp6PpInfo.au1MBqp[2] = 24;
        prVDecVP6DecPrm->rVp6PpInfo.au1MBqp[3] = 26;
        break;
    case 3:
        prVDecVP6DecPrm->rVp6PpInfo.au1MBqp[0] = 8;
        prVDecVP6DecPrm->rVp6PpInfo.au1MBqp[1] = 11;
        prVDecVP6DecPrm->rVp6PpInfo.au1MBqp[2] = 22;
        prVDecVP6DecPrm->rVp6PpInfo.au1MBqp[3] = 25;
        break;
    default:
        break;
    }



    switch(_u4PrevfBufIdx[u4InstID])
    {
    case 0:
        if (_u4GoldfBufIdx[u4InstID] == 0)
        {
            _u4DecBufIdx[u4InstID] = 1;
            prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic0CSa = (UINT32)_pucPic1C[u4InstID];
            prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic0YSa = (UINT32)_pucPic1Y[u4InstID];
            prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic1CSa = (UINT32)_pucPic0C[u4InstID];
            prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic1YSa = (UINT32)_pucPic0Y[u4InstID];
            prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic2CSa = (UINT32)_pucPic0C[u4InstID];
            prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic2YSa = (UINT32)_pucPic0Y[u4InstID];
        }
        else
            if (_u4GoldfBufIdx[u4InstID] == 1)
            {
                _u4DecBufIdx[u4InstID] = 2;
                prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic0CSa = (UINT32)_pucPic2C[u4InstID];
                prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic0YSa = (UINT32)_pucPic2Y[u4InstID];
                prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic1CSa = (UINT32)_pucPic0C[u4InstID];
                prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic1YSa = (UINT32)_pucPic0Y[u4InstID];
                prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic2CSa = (UINT32)_pucPic1C[u4InstID];
                prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic2YSa = (UINT32)_pucPic1Y[u4InstID];
            }
            else
                if (_u4GoldfBufIdx[u4InstID] == 2)
                {
                    _u4DecBufIdx[u4InstID] = 1;
                    prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic0CSa = (UINT32)_pucPic1C[u4InstID];
                    prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic0YSa = (UINT32)_pucPic1Y[u4InstID];
                    prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic1CSa = (UINT32)_pucPic0C[u4InstID];
                    prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic1YSa = (UINT32)_pucPic0Y[u4InstID];
                    prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic2CSa = (UINT32)_pucPic2C[u4InstID];
                    prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic2YSa = (UINT32)_pucPic2Y[u4InstID];            
                }
                break;
    case 1:
        if (_u4GoldfBufIdx[u4InstID] == 0)
        {
            _u4DecBufIdx[u4InstID] = 2;
            prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic0CSa = (UINT32)_pucPic2C[u4InstID];
            prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic0YSa = (UINT32)_pucPic2Y[u4InstID];
            prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic1CSa = (UINT32)_pucPic1C[u4InstID];
            prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic1YSa = (UINT32)_pucPic1Y[u4InstID];
            prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic2CSa = (UINT32)_pucPic0C[u4InstID];
            prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic2YSa = (UINT32)_pucPic0Y[u4InstID];            
        }
        else
            if (_u4GoldfBufIdx[u4InstID] == 1)
            {
                _u4DecBufIdx[u4InstID] = 0;
                prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic0CSa = (UINT32)_pucPic0C[u4InstID];
                prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic0YSa = (UINT32)_pucPic0Y[u4InstID];
                prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic1CSa = (UINT32)_pucPic1C[u4InstID];
                prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic1YSa = (UINT32)_pucPic1Y[u4InstID];
                prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic2CSa = (UINT32)_pucPic1C[u4InstID];
                prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic2YSa = (UINT32)_pucPic1Y[u4InstID];            
            }
            else
                if (_u4GoldfBufIdx[u4InstID] == 2)
                {
                    _u4DecBufIdx[u4InstID] = 0;
                    prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic0CSa = (UINT32)_pucPic0C[u4InstID];
                    prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic0YSa = (UINT32)_pucPic0Y[u4InstID];
                    prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic1CSa = (UINT32)_pucPic1C[u4InstID];
                    prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic1YSa = (UINT32)_pucPic1Y[u4InstID];
                    prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic2CSa = (UINT32)_pucPic2C[u4InstID];
                    prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic2YSa = (UINT32)_pucPic2Y[u4InstID];            
                }
                break;
    case 2:
        if (_u4GoldfBufIdx[u4InstID] == 0)
        {
            _u4DecBufIdx[u4InstID] = 1;
            prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic0CSa = (UINT32)_pucPic1C[u4InstID];
            prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic0YSa = (UINT32)_pucPic1Y[u4InstID];
            prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic1CSa = (UINT32)_pucPic2C[u4InstID];
            prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic1YSa = (UINT32)_pucPic2Y[u4InstID];
            prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic2CSa = (UINT32)_pucPic0C[u4InstID];
            prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic2YSa = (UINT32)_pucPic0Y[u4InstID];            
        }
        else
            if (_u4GoldfBufIdx[u4InstID] == 1)
            {
                _u4DecBufIdx[u4InstID] = 0;
                prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic0CSa = (UINT32)_pucPic0C[u4InstID];
                prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic0YSa = (UINT32)_pucPic0Y[u4InstID];
                prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic1CSa = (UINT32)_pucPic2C[u4InstID];
                prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic1YSa = (UINT32)_pucPic2Y[u4InstID];
                prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic2CSa = (UINT32)_pucPic1C[u4InstID];
                prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic2YSa = (UINT32)_pucPic1Y[u4InstID];            
            }
            else
                if (_u4GoldfBufIdx[u4InstID] == 2)
                {
                    _u4DecBufIdx[u4InstID] = 1;
                    prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic0CSa = (UINT32)_pucPic1C[u4InstID];
                    prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic0YSa = (UINT32)_pucPic1Y[u4InstID];
                    prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic1CSa = (UINT32)_pucPic2C[u4InstID];
                    prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic1YSa = (UINT32)_pucPic2Y[u4InstID];
                    prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic2CSa = (UINT32)_pucPic2C[u4InstID];
                    prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic2YSa = (UINT32)_pucPic2Y[u4InstID];            
                }
                break;          
    default:
        prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic0CSa = (UINT32)_pucPic0C[u4InstID];
        prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic0YSa = (UINT32)_pucPic0Y[u4InstID];
        prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic1CSa = (UINT32)_pucPic1C[u4InstID];
        prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic1YSa = (UINT32)_pucPic1Y[u4InstID];
        prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic2CSa = (UINT32)_pucPic2C[u4InstID];
        prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic2YSa = (UINT32)_pucPic2Y[u4InstID];
        break;
    }   

    vVDec_InvDCacheRange((UINT32) _pucPic0Y[u4InstID],PIC_Y_SZ);
    vVDec_InvDCacheRange((UINT32) _pucPic0C[u4InstID],PIC_C_SZ);
    vVDec_InvDCacheRange((UINT32) _pucPic1Y[u4InstID],PIC_Y_SZ);
    vVDec_InvDCacheRange((UINT32) _pucPic1C[u4InstID],PIC_C_SZ);
    vVDec_InvDCacheRange((UINT32) _pucPic2Y[u4InstID],PIC_Y_SZ);
    vVDec_InvDCacheRange((UINT32) _pucPic2C[u4InstID],PIC_C_SZ);
}

void vVerifyVDecSetVP6Info(UINT32 u4InstID)
{    

    UINT32 u4BsId = 0;  
    VDEC_INFO_DEC_PRM_T *prDecPrm = &_tVerMpvDecPrm[_u1AlphaDecPrmIdx[u4InstID]];
    VDEC_INFO_VP6_DEC_PRM_T *prVDecVP6DecPrm =  &(_tVerMpvDecPrm[_u1AlphaDecPrmIdx[u4InstID]].SpecDecPrm.rVDecVP6DecPrm);
    VDEC_INFO_VP6_FRM_HDR_T *prVDecVp6FrmHdr = &_rVDecVp6FrmHdr[_u1AlphaDecPrmIdx[u4InstID]];    

    if (prVDecVp6FrmHdr->ucFrameType == VP6_I_FRM)
    {
        u4VDEC_HAL_VP6_Default_Models_Init(u4BsId, u4InstID);
        u4VDEC_HAL_VP6_Load_QMatrix(u4BsId, u4InstID);
    }
    else
    {
        u4VDEC_HAL_VP6_Parse_Mb_Type_Models(u4BsId, u4InstID);
    }

    u4VDEC_HAL_VP6_Load_Filter_Coef(u4BsId, u4InstID, prVDecVp6FrmHdr->u4Filter_Selection);

    prVDecVP6DecPrm->i4MemBase = 0;

    vVerVP6SetBufStatus(u4InstID);

    prVDecVP6DecPrm->u4FRefBufIdx = _u4FRefBufIdx[_u1AlphaDecPrmIdx[u4InstID]];
    prVDecVP6DecPrm->fgAdobeMode = _u4AdobeMode[u4InstID];
    prVDecVP6DecPrm->prFrmHdr = &_rVDecVp6FrmHdr[_u1AlphaDecPrmIdx[u4InstID]];
    _u4RealHSize[u4InstID] = prVDecVP6DecPrm->prFrmHdr->u2WidthDec;
    _u4RealVSize[u4InstID] = prVDecVP6DecPrm->prFrmHdr->u2HeightDec;

    prDecPrm->ucPicStruct = FRM_PIC;
    prDecPrm->ucPicType = prVDecVp6FrmHdr->ucFrameType;
    prDecPrm->ucAddrSwapMode = _tVerMpvDecPrm[u4InstID].ucAddrSwapMode;
    prDecPrm->u4PicBW = prVDecVP6DecPrm->prFrmHdr->u2WidthDec;
    prDecPrm->u4PicW = prVDecVP6DecPrm->prFrmHdr->u2WidthDec;
    prDecPrm->u4PicH = prVDecVP6DecPrm->prFrmHdr->u2HeightDec;
    i4VDEC_HAL_VP6_DecStart(u4InstID, prDecPrm);
}


UINT32 u4VP6InverseAddrSwap(UINT32 u4AddrSwapMode, UINT32 u4SwappedAddr, BOOL fgIsYComponent)
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
    default:
        u4NonSwappedAddr = u4SwappedAddr;
        break;
    }
    return u4NonSwappedAddr;
}

void vVP6_InvAddressSwap(UINT32 u4InstID, 
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

    UINT8 auAddrSwapMapTable[8] =
    {
        4, 5, 6, 7, 0, 1, 2, 3
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
        u4NonSwappedAddr = u4VP6InverseAddrSwap(u4AddrSwapMode, u4SwappedAddr, TRUE);
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
        u4NonSwappedAddr = u4VP6InverseAddrSwap(u4AddrSwapMode, u4SwappedAddr, FALSE);
        pbTempBufAddr = (BYTE*) (pbSrcBufC+i);
        memcpy(&pbOutBufC[u4NonSwappedAddr<<4], &pbTempBufAddr[0],u4AddrressSwapSize);
        u4SwappedAddr++;
    }
}

BOOL fgVP6SmallFolder(UINT32 u4InstID)
{
    CHAR _bFileNameCRC[20] = {"_Y.out\0"};
    CHAR _bGoldFileName[256] = {"/mc_out_\0"};
    CHAR _bPatternPath [256];
    CHAR _bPathAddStr[256] = {"emu_data/pattern/\0"};
    CHAR _bFileName [256];
    UINT32 u4Temp;
    //Searh path name
    UINT32 path_byte_addr = 0;
    UINT32 filename_byte_addr = 0;
    BOOL fgOpen;
    INT32 i, j;
    UCHAR strMessage [ 256];

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
                if( (_bFileStr1[u4InstID][1][i+1] == 'v' || _bFileStr1[u4InstID][1][i+1] == 'V')
                    && (_bFileStr1[u4InstID][1][i+2] == 'd' || _bFileStr1[u4InstID][1][i+2] == 'D')
                    && (_bFileStr1[u4InstID][1][i+3] == 'o' || _bFileStr1[u4InstID][1][i+3] == 'O'))
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
    u4Temp += sprintf(_bPatternPath+path_byte_addr+u4Temp, "/%u", 1000);
    u4Temp += sprintf(_bPatternPath+path_byte_addr+u4Temp, "%s", _bGoldFileName);    
    _bPatternPath[path_byte_addr+u4Temp] = '\0';

    _tFBufFileInfo[u4InstID].fgGetFileInfo = FALSE; 
    _tFBufFileInfo[u4InstID].pucTargetAddr = _pucDumpYBuf[u4InstID];
    _tFBufFileInfo[u4InstID].u4TargetSz = GOLD_Y_SZ;  
    _tFBufFileInfo[u4InstID].u4FileLength = 0;  
    vConcateStr((char*)_bFileStr1[u4InstID][3], (char*)_bPatternPath, (char*)_bFileNameCRC, 1000);
    fgOpen = fgOpenFile(u4InstID, (char*)_bFileStr1[u4InstID][3],(char*)"r+b", &_tFBufFileInfo[u4InstID]);

    return fgOpen;
}

BOOL fgVP6CRCPatternExist(UINT32 u4InstID)
{
    CHAR _bFileNameCRC[20] = {"_CRC.out\0"};
    CHAR _bGoldFileName[256] = {"/mc_out_\0"};
    CHAR _bPatternPath [256];
    CHAR _bPathAddStr[256] = {"emu_data/pattern/\0"};
    CHAR _bFileName [256];
    UINT32 u4Temp;
    //Searh path name
    UINT32 path_byte_addr = 0;
    UINT32 filename_byte_addr = 0;
    BOOL fgOpen;
    INT32 i, j;
    UCHAR strMessage [ 256];

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
                if( (_bFileStr1[u4InstID][1][i+1] == 'v' || _bFileStr1[u4InstID][1][i+1] == 'V')
                    && (_bFileStr1[u4InstID][1][i+2] == 'd' || _bFileStr1[u4InstID][1][i+2] == 'D')
                    && (_bFileStr1[u4InstID][1][i+3] == 'o' || _bFileStr1[u4InstID][1][i+3] == 'O'))
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
    u4Temp += sprintf(_bPatternPath+path_byte_addr+u4Temp, "%s", _bGoldFileName);    
    _bPatternPath[path_byte_addr+u4Temp] = '\0';

    _tFBufFileInfo[u4InstID].fgGetFileInfo = FALSE; 
    _tFBufFileInfo[u4InstID].pucTargetAddr = _pucCRCBuf[u4InstID];
    _tFBufFileInfo[u4InstID].u4TargetSz = 32;  
    _tFBufFileInfo[u4InstID].u4FileLength = 0;  
    vConcateStr((char*)_bFileStr1[u4InstID][3], (char*)_bPatternPath, (char*)_bFileNameCRC, 0);
    fgOpen = fgOpenFile(u4InstID, (char*)_bFileStr1[u4InstID][3],(char*)"r+b", &_tFBufFileInfo[u4InstID]);

    return fgOpen;
}


void vVP6_CheckCRCResult(UINT32 u4InstID)//, UINT32 u4DecFrameCnt, UINT32 u4CRCResBuf)
{
    //read
    //REG_2: y_crc_checksum[31:0]
    //REG_3: y_crc_checksum[63:32]
    //REG_4: y_crc_checksum[95:64]
    //REG_5: y_crc_checksum[127:96]

    //REG_6: c_crc_checksum[31:0]
    //REG_7: c_crc_checksum[63:32]
    //REG_8: c_crc_checksum[95:64]
    //REG_9: c_crc_checksum[127:96]


    //UINT32* pu4CRCResultCurrAddr;
    //UINT32 u4CRCResult = 0;
    //UINT32 i=0;
    //UINT32 u4SWResult;
    UINT32 u4HWCRC_Y0, u4HWCRC_Y1, u4HWCRC_Y2, u4HWCRC_Y3;
    UINT32 u4HWCRC_C0, u4HWCRC_C1, u4HWCRC_C2, u4HWCRC_C3;

    CHAR _bFileNameCRC[20] = {"_CRC.out\0"};
    CHAR _bGoldFileName[256] = {"/mc_out_\0"};
    CHAR _bPatternPath [256];
    CHAR _bPathAddStr[256] = {"emu_data/pattern/\0"};
    CHAR _bFileName [256];
    UINT32 u4Temp;
    //Searh path name
    UINT32 path_byte_addr = 0;
    UINT32 filename_byte_addr = 0;
    UINT32 u4CRCValueY0, u4CRCValueY1, u4CRCValueY2, u4CRCValueY3;
    UINT32 u4CRCValueC0, u4CRCValueC1, u4CRCValueC2, u4CRCValueC3;
    UINT32 u4CRCTmp3, u4CRCTmp2, u4CRCTmp1, u4CRCTmp0;
    BOOL fgDecErr = FALSE;
    BOOL fgOpen;
    INT32 i, j;
    UCHAR strMessage [ 256];

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
                if( (_bFileStr1[u4InstID][1][i+1] == 'v' || _bFileStr1[u4InstID][1][i+1] == 'V')
                    && (_bFileStr1[u4InstID][1][i+2] == 'd' || _bFileStr1[u4InstID][1][i+2] == 'D')
                    && (_bFileStr1[u4InstID][1][i+3] == 'o' || _bFileStr1[u4InstID][1][i+3] == 'O'))
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
    if (_fgVP6SmallFlolder[u4InstID])
    {
        UINT32 u4TmpVal = _u4FileCnt[u4InstID] / 1000;
        if (u4TmpVal > 0)
        {
            u4Temp += sprintf(_bPatternPath+path_byte_addr+u4Temp, "/%u", u4TmpVal * 1000);
        }
    }
    u4Temp += sprintf(_bPatternPath+path_byte_addr+u4Temp, "%s", _bGoldFileName);    
    _bPatternPath[path_byte_addr+u4Temp] = '\0';

    _tFBufFileInfo[u4InstID].fgGetFileInfo = TRUE;  
    _tFBufFileInfo[u4InstID].pucTargetAddr = _pucCRCBuf[u4InstID];
    _tFBufFileInfo[u4InstID].u4TargetSz = 32;  
    _tFBufFileInfo[u4InstID].u4FileLength = 0;  
    vConcateStr((char*)_bFileStr1[u4InstID][3], (char*)_bPatternPath, (char*)_bFileNameCRC, (UINT32)_u4FileCnt[u4InstID]);
    fgOpenFile(u4InstID, (char*)_bFileStr1[u4InstID][3],(char*)"r+b", &_tFBufFileInfo[u4InstID]);

    u4CRCTmp3 = (_pucCRCBuf[u4InstID][3] );
    u4CRCTmp2 = (_pucCRCBuf[u4InstID][2] );
    u4CRCTmp1 = (_pucCRCBuf[u4InstID][1] );
    u4CRCTmp0 = (_pucCRCBuf[u4InstID][0] );
    u4CRCValueY0 = (u4CRCTmp3 << 24) | (u4CRCTmp2 << 16) |  (u4CRCTmp1 << 8) |  (u4CRCTmp0 << 0) ;

    u4CRCTmp3 = (_pucCRCBuf[u4InstID][7] );
    u4CRCTmp2 = (_pucCRCBuf[u4InstID][6] );
    u4CRCTmp1 = (_pucCRCBuf[u4InstID][5] );
    u4CRCTmp0 = (_pucCRCBuf[u4InstID][4] );
    u4CRCValueY1 = (u4CRCTmp3 << 24) | (u4CRCTmp2 << 16) |  (u4CRCTmp1 << 8) |  (u4CRCTmp0 << 0) ;

    u4CRCTmp3 = (_pucCRCBuf[u4InstID][11] );
    u4CRCTmp2 = (_pucCRCBuf[u4InstID][10] );
    u4CRCTmp1 = (_pucCRCBuf[u4InstID][9] );
    u4CRCTmp0 = (_pucCRCBuf[u4InstID][8] );
    u4CRCValueY2 = (u4CRCTmp3 << 24) | (u4CRCTmp2 << 16) |  (u4CRCTmp1 << 8) |  (u4CRCTmp0 << 0) ;

    u4CRCTmp3 = (_pucCRCBuf[u4InstID][15] );
    u4CRCTmp2 = (_pucCRCBuf[u4InstID][14] );
    u4CRCTmp1 = (_pucCRCBuf[u4InstID][13] );
    u4CRCTmp0 = (_pucCRCBuf[u4InstID][12] );
    u4CRCValueY3 = (u4CRCTmp3 << 24) | (u4CRCTmp2 << 16) |  (u4CRCTmp1 << 8) |  (u4CRCTmp0 << 0) ;

    u4CRCTmp3 = (_pucCRCBuf[u4InstID][19] );
    u4CRCTmp2 = (_pucCRCBuf[u4InstID][18] );
    u4CRCTmp1 = (_pucCRCBuf[u4InstID][17] );
    u4CRCTmp0 = (_pucCRCBuf[u4InstID][16] );
    u4CRCValueC0 = (u4CRCTmp3 << 24) | (u4CRCTmp2 << 16) |  (u4CRCTmp1 << 8) |  (u4CRCTmp0 << 0) ;

    u4CRCTmp3 = (_pucCRCBuf[u4InstID][23] );
    u4CRCTmp2 = (_pucCRCBuf[u4InstID][22] );
    u4CRCTmp1 = (_pucCRCBuf[u4InstID][21] );
    u4CRCTmp0 = (_pucCRCBuf[u4InstID][20] );
    u4CRCValueC1 = (u4CRCTmp3 << 24) | (u4CRCTmp2 << 16) |  (u4CRCTmp1 << 8) |  (u4CRCTmp0 << 0) ;

    u4CRCTmp3 = (_pucCRCBuf[u4InstID][27] );
    u4CRCTmp2 = (_pucCRCBuf[u4InstID][26] );
    u4CRCTmp1 = (_pucCRCBuf[u4InstID][25] );
    u4CRCTmp0 = (_pucCRCBuf[u4InstID][24] );
    u4CRCValueC2 = (u4CRCTmp3 << 24) | (u4CRCTmp2 << 16) |  (u4CRCTmp1 << 8) |  (u4CRCTmp0 << 0) ;

    u4CRCTmp3 = (_pucCRCBuf[u4InstID][31] );
    u4CRCTmp2 = (_pucCRCBuf[u4InstID][30] );
    u4CRCTmp1 = (_pucCRCBuf[u4InstID][29] );
    u4CRCTmp0 = (_pucCRCBuf[u4InstID][28] );
    u4CRCValueC3 = (u4CRCTmp3 << 24) | (u4CRCTmp2 << 16) |  (u4CRCTmp1 << 8) |  (u4CRCTmp0 << 0) ;

    u4HWCRC_Y0 = u4VDecReadCRC(u4InstID, VDEC_CRC_Y_CHKSUM0);
    u4HWCRC_Y1 = u4VDecReadCRC(u4InstID, VDEC_CRC_Y_CHKSUM1);
    u4HWCRC_Y2 = u4VDecReadCRC(u4InstID, VDEC_CRC_Y_CHKSUM2);
    u4HWCRC_Y3 = u4VDecReadCRC(u4InstID, VDEC_CRC_Y_CHKSUM3);

    u4HWCRC_C0 = u4VDecReadCRC(u4InstID, VDEC_CRC_C_CHKSUM0);
    u4HWCRC_C1 = u4VDecReadCRC(u4InstID, VDEC_CRC_C_CHKSUM1);
    u4HWCRC_C2 = u4VDecReadCRC(u4InstID, VDEC_CRC_C_CHKSUM2);
    u4HWCRC_C3 = u4VDecReadCRC(u4InstID, VDEC_CRC_C_CHKSUM3);


    if( (u4HWCRC_Y0 != u4CRCValueY0) 
        || (u4HWCRC_Y1 != u4CRCValueY1)
        || (u4HWCRC_Y2 != u4CRCValueY2)
        || (u4HWCRC_Y3 != u4CRCValueY3)
        || (u4HWCRC_C0 != u4CRCValueC0)
        || (u4HWCRC_C1 != u4CRCValueC1)
        || (u4HWCRC_C2 != u4CRCValueC2)
        || (u4HWCRC_C3 != u4CRCValueC3)
        )
    {
        fgDecErr = TRUE;
        sprintf(strMessage," 3 Error ==> Pic count to [%d] \n", _u4FileCnt[u4InstID]);     
        fgWrMsg2PC((void*)strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
    }

    _u4FileCnt[u4InstID]++;

    if (_fgVP6SmallFlolder[u4InstID] && (_u4FileCnt[u4InstID] / 1000) > 0)
    {
        u4Temp = sprintf(_bPatternPath+path_byte_addr, "%s", _bPathAddStr);  
        u4Temp += sprintf(_bPatternPath+path_byte_addr+u4Temp, "%s", _bFileName);
        u4Temp += sprintf(_bPatternPath+path_byte_addr+u4Temp, "/%u", (_u4FileCnt[u4InstID] / 1000) * 1000);
        u4Temp += sprintf(_bPatternPath+path_byte_addr+u4Temp, "%s", _bGoldFileName);    
        _bPatternPath[path_byte_addr+u4Temp] = '\0';
    }

    _tFBufFileInfo[u4InstID].fgGetFileInfo = TRUE;  
    _tFBufFileInfo[u4InstID].pucTargetAddr = _pucCRCBuf[u4InstID];
    _tFBufFileInfo[u4InstID].u4TargetSz = 32;  
    _tFBufFileInfo[u4InstID].u4FileLength = 0;  
    vConcateStr(_bFileStr1[u4InstID][3], _bPatternPath, _bFileNameCRC, _u4FileCnt[u4InstID]);

#ifdef IDE_READ_SUPPORT
    fgOpen = fgPureOpenIdeFile( _bFileStr1[u4InstID][3],"r+b", &_tFBufFileInfo[u4InstID]);
#else
    fgOpen = fgOpenFile(u4InstID, _bFileStr1[u4InstID][3],"r+b", &_tFBufFileInfo[u4InstID]);
#endif

    if((fgOpen == FALSE) ||(fgDecErr == TRUE))
    {
        sprintf(strMessage, "%s", "\n");
        fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
        //fprintf(_tFileListRecInfo.fpFile, "\n");  
        // break decode
        if(fgOpen == FALSE)
        {
            sprintf(strMessage," Compare Finish==> Pic count to [%d] \n", _u4FileCnt[u4InstID] - 1);   
            fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
            //fprintf(_tFileListRecInfo.fpFile, " Compare Finish==> Pic count to [%d] \n", _u4FileCnt[_u4VDecID] - 1);   
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
            sprintf(strMessage," 2 Error ==> Pic count to [%d] \n", _u4FileCnt[u4InstID] - 1);     
            fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
            //fprintf(_tFileListRecInfo.fpFile, " Error ==> Pic count to [%d] \n", _u4FileCnt[_u4VDecID] - 1);         
        }
        _u4VerBitCount[u4InstID] = 0xffffffff;
    }

    if(_u4FileCnt[u4InstID] >= _u4EndCompPicNum[u4InstID])
    {
        _u4VerBitCount[u4InstID] = 0xffffffff;
    }    
}

// *********************************************************************
// Function    : void vVP6WrData2PC(UINT32 u4InstID, BYTE *ptAddr, UINT32 u4Size, BOOL *fgNextFrameExist)
// Description : Write the decoded data to PC for compare
// Parameter   : None
// Return      : None
// *********************************************************************
void vVP6WrData2PC(UINT32 u4InstID, UCHAR *ptAddr, UINT32 u4Size, BOOL *fgNextFrameExist)
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
    VDEC_INFO_VP6_FRM_HDR_T *prVDecVp6FrmHdr = &_rVDecVp6FrmHdr[_u1AlphaDecPrmIdx[u4InstID]];   

    BOOL fgDecErr,fgOpen;
    char strMessage[256];

#if (!VDEC_VP6_ERR_TEST)
    BOOL fgCompare = TRUE;
#else
    BOOL fgCompare = FALSE;
#endif

    //#ifndef DOWN_SCALE_SUPPORT               
    UINT32 u4NonSwapYBase = 0;
    UINT32 u4NonSwapCBase = 0;
    //#endif   

    CHAR _bFileAddStrY[20] = {"_Y.out\0"};
    CHAR _bFileAddStrC[20] = {"_CbCr.out\0"};
    CHAR _bGoldFileName[256] = {"/mc_out_\0"};
    CHAR _bPatternPath [256];
    CHAR _bPathAddStr[256] = {"emu_data/pattern/\0"};
    CHAR _bFileName [256];
    UINT32 u4Temp;
    //Searh path name
    UINT32 path_byte_addr = 0;
    UINT32 filename_byte_addr = 0;
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
                if( (_bFileStr1[u4InstID][1][i+1] == 'v' || _bFileStr1[u4InstID][1][i+1] == 'V')
                    && (_bFileStr1[u4InstID][1][i+2] == 'd' || _bFileStr1[u4InstID][1][i+2] == 'D')
                    && (_bFileStr1[u4InstID][1][i+3] == 'o' || _bFileStr1[u4InstID][1][i+3] == 'O'))
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
    if (_u1AlphaBitstream[u4InstID] && (_u1AlphaFlag[u4InstID] & VP6_ALPHA_FRAME))
    {
        u4Temp += sprintf(_bPatternPath+path_byte_addr+u4Temp, "%s", "/alpha");
    }
    if (_fgVP6SmallFlolder[u4InstID])
    {
        UINT32 u4TmpVal = _u4FileCnt[u4InstID] / 1000;
        if (u4TmpVal > 0)
        {
            u4Temp += sprintf(_bPatternPath+path_byte_addr+u4Temp, "/%u", u4TmpVal * 1000);
        }
    }
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
        //if(!fgCompVP6ChkSumGolden(u4InstID))
        //{
        //    fgDecErr = TRUE;
        //    vVDecOutputDebugString("Check sum comparison mismatch\n");
        //}
#else // compare pixel by pixel
        // Y compare
        _tFBufFileInfo[u4InstID].fgGetFileInfo = TRUE;  
        _tFBufFileInfo[u4InstID].pucTargetAddr = _pucDumpYBuf[u4InstID];
        _tFBufFileInfo[u4InstID].u4TargetSz = GOLD_Y_SZ;  
        _tFBufFileInfo[u4InstID].u4FileLength = 0;  
        // Y decoded data Compare   
        vConcateStr(_bFileStr1[u4InstID][3], _bPatternPath, _bFileAddStrY, _u4FileCnt[u4InstID]);

#ifdef EXT_COMPARE 
        _tFBufFileInfo[u4InstID].u4FileLength = (((prVDecVp6FrmHdr->u2WidthDec + 15)>>4)<<4) *(((prVDecVp6FrmHdr->u2HeightDec + 31)>>5)<<5);
#else
#ifdef DIRECT_DEC
        if((_u4FileCnt[u4InstID] >= _u4StartCompPicNum[u4InstID]) && ( _u4FileCnt[u4InstID] <= _u4EndCompPicNum[u4InstID]))
#endif    
        {
#ifdef DOWN_SCALE_SUPPORT
            if(_u4CodecVer[u4InstID] == VDEC_VP6)
            {
                if(!(_tVerMpvDecPrm[_u1AlphaDecPrmIdx[u4InstID]].SpecDecPrm.rVDecVP6DecPrm.fgDec2ndFld))
                {
                    x_memset(_pucDumpYBuf[u4InstID], 0x0, GOLD_Y_SZ);
                    x_memset(_pucDumpCBuf[u4InstID], 0x0, GOLD_C_SZ);
                }
            }

            u4NonSwapYBase = (UINT32)_pucDecWorkBuf[u4InstID];
            u4NonSwapCBase = (UINT32)_pucDecCWorkBuf[u4InstID];
            if (_tVerMpvDecPrm[u4InstID].ucAddrSwapMode != 4)
            {
                UINT32 u4AlignWidth, u4AlignHeight;
                UINT32 u4AlignSize = 0;

                // swap off down scaler data
                u4AlignWidth = _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4DispW;
                u4AlignWidth = (((u4AlignWidth +63) >>6) <<6); //Align to 4MB width

                if((_tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucPicStruct == TOP_FIELD) ||(_tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucPicStruct == BOTTOM_FIELD))
                {
                    u4AlignHeight = _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4TrgOffV + (_tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4TrgHeight*2);
                }
                else
                {
                    u4AlignHeight = _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4TrgOffV + _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4TrgHeight;
                }

                u4AlignHeight =  (((u4AlignHeight +31) >>5) <<5);

                vVP6_InvAddressSwap(u4InstID, 
                    (BYTE*)_pucVDSCLBuf[u4InstID],  (BYTE*)(_pucVDSCLBuf[u4InstID] + 0x1FE000), 
                    (BYTE*)_pucDumpYBuf[u4InstID], (BYTE*) _pucDumpCBuf[u4InstID],
                    u4AlignWidth,  u4AlignHeight, u4AlignSize,
                    _tVerMpvDecPrm[u4InstID].ucAddrSwapMode);

                u4NonSwapYBase = (UINT32)_pucVDSCLBuf[u4InstID];
                u4NonSwapCBase = (UINT32)(_pucVDSCLBuf[u4InstID] + 0x1FE000);
                memcpy((UCHAR*)u4NonSwapYBase, _pucDumpYBuf[u4InstID],(u4AlignWidth*u4AlignHeight) +u4AlignSize);
                memcpy((UCHAR*)u4NonSwapCBase, _pucDumpCBuf[u4InstID],(u4AlignWidth*u4AlignHeight/2) + u4AlignSize);                 

                // swap off mc data
                u4AlignWidth = _tVerMpvDecPrm[u4InstID].u4PicW;
                u4AlignWidth = (((u4AlignWidth +63) >>6) <<6); //Align to 4MB width                    
                u4AlignHeight = _tVerMpvDecPrm[u4InstID].u4PicH;
                u4AlignHeight =  (((u4AlignHeight +31) >>5) <<5);              

                vVP6_InvAddressSwap(u4InstID, 
                    (BYTE*)_pucDecWorkBuf[u4InstID],  (BYTE*)_pucDecCWorkBuf[u4InstID], 
                    (BYTE*)_pucDumpYBuf[u4InstID], (BYTE*) _pucDumpCBuf[u4InstID],
                    u4AlignWidth,  u4AlignHeight, u4AlignSize,
                    _tVerMpvDecPrm[u4InstID].ucAddrSwapMode);

                u4NonSwapYBase = (UINT32)_pucPpYSa[u4InstID];
                u4NonSwapCBase = (UINT32)_pucPpCSa[u4InstID];
                memcpy((UCHAR*)u4NonSwapYBase, _pucDumpYBuf[u4InstID],(u4AlignWidth*u4AlignHeight) +u4AlignSize);
                memcpy((UCHAR*)u4NonSwapCBase, _pucDumpCBuf[u4InstID],(u4AlignWidth*u4AlignHeight/2) + u4AlignSize);                 

                printk("MPV emu VP6 inverse swap mode %d\n", _tVerMpvDecPrm[u4InstID].ucAddrSwapMode);
            }

            if(_tVerMpvDecPrm[_u1AlphaDecPrmIdx[u4InstID]].SpecDecPrm.rVDecVP6DecPrm.rVP6PpInfo.fgPpEnable)
            {
                vGenerateDownScalerGolden(u4InstID, (UINT32)_pucPpYSa[_u1AlphaDecPrmIdx[u4InstID]],(UINT32)(_pucPpCSa[_u1AlphaDecPrmIdx[u4InstID]]),u4Size);
            }
            else
            {
                vGenerateDownScalerGolden(u4InstID, u4NonSwapYBase, u4NonSwapCBase, u4Size);
            }
#else
            #if VMMU_SUPPORT
            u4NonSwapYBase = (UINT32)_pucDecWorkBuf[u4InstID] + 0x1000;
            u4NonSwapCBase = (UINT32)_pucDecCWorkBuf[u4InstID] + 0x1000;

            printk("[VP6] vVP6WrData2PC, 1, Y Base:0x%x, C Base:0x%x\n", u4NonSwapYBase, u4NonSwapCBase); 
            #else
            u4NonSwapYBase = (UINT32)_pucDecWorkBuf[u4InstID];
            u4NonSwapCBase = (UINT32)_pucDecCWorkBuf[u4InstID];
            #endif
            
            {
                //UINT8 *p = (UINT8 *)u4NonSwapYBase;
                //printk("<vdec> data test addr=0x%08x, @(%s, %d)===============\n", p, __FUNCTION__, __LINE__);
                //printk("<vdec> %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", 
                //    p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
            }

            if (_tVerMpvDecPrm[u4InstID].ucAddrSwapMode != 4)
            {
                UINT32 u4AlignWidth, u4AlignHeight;
                UINT32 u4AlignSize = 0;

                u4AlignWidth = _tVerMpvDecPrm[u4InstID].u4PicW;
                u4AlignWidth = (((u4AlignWidth +63) >>6) <<6); //Align to 4MB width                    
                u4AlignHeight = _tVerMpvDecPrm[u4InstID].u4PicH;
                u4AlignHeight =  (((u4AlignHeight +31) >>5) <<5);              

                vVP6_InvAddressSwap(u4InstID, 
                    (BYTE*)_pucDecWorkBuf[u4InstID],  (BYTE*)_pucDecCWorkBuf[u4InstID], 
                    (BYTE*)_pucDumpYBuf[u4InstID], (BYTE*) _pucDumpCBuf[u4InstID],
                    u4AlignWidth,  u4AlignHeight, u4AlignSize,
                    _tVerMpvDecPrm[u4InstID].ucAddrSwapMode);

                u4NonSwapYBase = (UINT32)_pucVDSCLBuf[u4InstID];
                u4NonSwapCBase = (UINT32)_pucVDSCLBuf[u4InstID] + 0x1FE000;
                memcpy((UCHAR*)u4NonSwapYBase, _pucDumpYBuf[u4InstID],(u4AlignWidth*u4AlignHeight) +u4AlignSize);
                memcpy((UCHAR*)u4NonSwapCBase, _pucDumpCBuf[u4InstID],(u4AlignWidth*u4AlignHeight/2) + u4AlignSize);                 

                printk("MPV emu VP6 inverse swap mode %d\n", _tVerMpvDecPrm[u4InstID].ucAddrSwapMode);
            }

            fgOpenFile(u4InstID, _bFileStr1[u4InstID][3],"r+b", &_tFBufFileInfo[u4InstID]);               
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
#if defined(DOWN_SCALE_SUPPORT)
        u4YBase = (UINT32)_pucVDSCLBuf[u4InstID];
        //u4BufferWidth = ((_tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4DispW + 15) >> 4) << 4;
        u4Width = _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4DispW;
        if((_tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucPicStruct == TOP_FIELD) ||(_tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucPicStruct == BOTTOM_FIELD))
        {
            u4Height = _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4TrgOffV + (_tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4TrgHeight*2);
        }
        else
        {
            u4Height = _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4TrgOffV + _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4TrgHeight;
        }

        printk("MPV emu down scaler target width = %d\n", u4Width);
        printk("MPV emu down scaler target height = %d\n", u4Height);

#else
        u4YBase = u4NonSwapYBase;//(UINT32)_pucDecWorkBuf[u4InstID];

        {
            //UINT8 *p = (UINT8 *)u4NonSwapYBase;
            //printk("<vdec> data test addr=0x%08x, @(%s, %d)===============\n", p, __FUNCTION__, __LINE__);
            //printk("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", 
            //    p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
        }
        u4Width = prVDecVp6FrmHdr->u2WidthDec;
        u4Height = prVDecVp6FrmHdr->u2HeightDec;
#endif  

        //fred add for 32byte align in height
        //u4Height = ( (u4Height+31)>>5 ) <<5;
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
                                        vVDecOutputDebugString("Y Data Mismatch at [x=%d, y=%d] = 0x%x, Golden = 0x%x !!! \n", u4XPix, u4YPix, (*pbDecBuf), (*pbGoldenBuf));
                                        sprintf(strMessage,"Y Data Mismatch at [x=%d, y=%d] = 0x%x, Golden = 0x%x !!! \n", u4XPix, u4YPix, (*pbDecBuf), (*pbGoldenBuf));
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

        if (fgDecErr)
        {
            UINT32 u4SizeTmp = (((prVDecVp6FrmHdr->u2WidthDec + 15)>>4)<<4) *(((prVDecVp6FrmHdr->u2HeightDec + 31)>>5)<<5);
            vConcateStr(_bFileStr1[u4InstID][4], _bPatternPath, "_Y.out.err", _u4FileCnt[u4InstID]);
            _tTempFileInfo[u4InstID].fgGetFileInfo = FALSE;

            printk("<vdec> %s\n", _bFileStr1[u4InstID][4]);
            fgWrData2PC((void*)u4NonSwapYBase, u4SizeTmp, 7, _bFileStr1[u4InstID][4]);
        }

        // CbCr compare
        //if(!fgIsMonoPic(_u4VDecID))
        if (0 == _u1AlphaBitstream[u4InstID] || (0 == (_u1AlphaFlag[u4InstID] & VP6_ALPHA_FRAME)))
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
                fgOpenFile(u4InstID, _bFileStr1[u4InstID][4],"r+b", &_tFBufFileInfo[u4InstID]);               
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
                                            vVDecOutputDebugString("Cb Data Mismatch at [x=%d, y=%d] = 0x%x, Golden = 0x%x !!! \n", u4XPix, u4YPix, (*pbDecBuf), (*pbGoldenBuf));
                                            sprintf(strMessage,"Cb Data Mismatch at [x=%d, y=%d] = 0x%x, Golden = 0x%x !!! \n", u4XPix, u4YPix, (*pbDecBuf), (*pbGoldenBuf));
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
                    if((_tVerMpvDecPrm[_u1AlphaDecPrmIdx[u4InstID]].SpecDecPrm.rVDecVP6DecPrm.fgDec2ndFld) || (_tVerMpvDecPrm[_u1AlphaDecPrmIdx[u4InstID]].ucPicStruct == FRM_PIC))
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
        sprintf(strMessage,"[%d], ", _u4FileCnt[u4InstID]);  
        fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
    }
#endif

#ifdef REDEC    
    if(_u4ReDecCnt[u4InstID] == 0)
#endif    
    {
        if (_u1AlphaBitstream[u4InstID])
        {
            if (_u1AlphaFlag[u4InstID] & VP6_ALPHA_FRAME)
            {
                _u4FileCnt[u4InstID] ++;
                _u1AlphaFlag[u4InstID] &= ~VP6_ALPHA_FRAME;
            }
            else
            {
                _u1AlphaFlag[u4InstID] |= VP6_ALPHA_FRAME;
            }
        }
        else
        {
            _u4FileCnt[u4InstID] ++;
        }
    }

#ifndef INTERGRATION_WITH_DEMUX
    // Check if still pic needed compare
    if((_u4FileCnt[u4InstID] >= _u4StartCompPicNum[u4InstID]) && ( _u4FileCnt[u4InstID] <= _u4EndCompPicNum[u4InstID]))
        if (fgCompare)
        {
            if (_fgVP6SmallFlolder[u4InstID] && (_u4FileCnt[u4InstID] / 1000) > 0)
            {
                u4Temp = sprintf(_bPatternPath+path_byte_addr, "%s", _bPathAddStr);  
                u4Temp += sprintf(_bPatternPath+path_byte_addr+u4Temp, "%s", _bFileName);
                u4Temp += sprintf(_bPatternPath+path_byte_addr+u4Temp, "/%u", (_u4FileCnt[u4InstID] / 1000) * 1000);
                u4Temp += sprintf(_bPatternPath+path_byte_addr+u4Temp, "%s", _bGoldFileName);    
                _bPatternPath[path_byte_addr+u4Temp] = '\0';
            }

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
            if (fgOpen && _tFBufFileInfo[u4InstID].u4FileLength == 0)
            {
                fgOpen = FALSE;
                printk("<vdec> file length = 0\n");
            }
#endif
#endif

            if (fgDecErr == TRUE)
            {
                vp6_dump_register(u4InstID);

                fgDecErr = 0;
            }

            //if((fgOpen == FALSE) ||(fgDecErr == TRUE) || (_fgVDecErr[u4InstID] == TRUE)) // houlong temp
            if((fgOpen == FALSE))
            {

                *fgNextFrameExist = FALSE;

                sprintf(strMessage, "%s", "\n");
                fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
                //fprintf(_tFileListRecInfo.fpFile, "\n");  
                // break decode
                if(fgOpen == FALSE)
                {
                    sprintf(strMessage," Compare Finish==> Pic count to [%d] \n", _u4FileCnt[u4InstID] - 1);   
                    fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
                    //fprintf(_tFileListRecInfo.fpFile, " Compare Finish==> Pic count to [%d] \n", _u4FileCnt[_u4VDecID] - 1);   
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
                    sprintf(strMessage," 1 Error ==> Pic count to [%d] \n", _u4FileCnt[u4InstID] - 1);     
                    fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
                    //fprintf(_tFileListRecInfo.fpFile, " Error ==> Pic count to [%d] \n", _u4FileCnt[_u4VDecID] - 1);         
                }
                _u4VerBitCount[u4InstID] = 0xffffffff;
            }
        }//~fgCompare
#endif
        if(_u4FileCnt[u4InstID] >= _u4EndCompPicNum[u4InstID])
        {
            _u4VerBitCount[u4InstID] = 0xffffffff;
        }

}


void vVerVP6VDecEnd(UINT32 u4InstID)
{
    VDEC_INFO_VP6_VFIFO_PRM_T rVp6VFifoInitPrm;
    VDEC_INFO_VP6_BS_INIT_PRM_T rVp6BSInitPrm;
    UINT32 u4VldByte/*,u4VldBit*/;
    UINT32 u4Size0, u4Size1, u4Size2, u4Size3;
    UINT32 u4TargFrm = 10000;
    //#if (!VDEC_VER_COMPARE_CRC)
#if 1
    VDEC_INFO_VP6_FRM_HDR_T *prVDecVp6FrmHdr = &_rVDecVp6FrmHdr[_u1AlphaDecPrmIdx[u4InstID]];
#endif
#if (VDEC_VP6_ERR_TEST)
    CHAR *pBsData;
    UINT32 u4BsSize;
    UINT32 u4RndCnt;
    UINT32 u4RndPos;
    INT32   i;
#endif
    BOOL   fgNextFrameExist = TRUE;

#if (CONFIG_DRV_VERIFY_SUPPORT)
    //For Debug Only

    //u4VDEC_HAL_VP6_Read_QMatrix(u4BSID, u4VDecID);
    if (_u4FileCnt[u4InstID] == u4TargFrm)
    {
        //u4VDEC_HAL_VP6_Write_SRAMData1(u4BSID, u4VDecID);
        u4VDEC_HAL_VP6_Read_SRAMData1(0, u4InstID);
    }
#endif

    //u4VldByte = u4VDEC_HAL_VP6_ReadRdPtr(0, u4InstID, (UINT32)_pucVFifo[u4InstID], &u4VldBit) - 4;

    //#if VDEC_VER_COMPARE_CRC
#ifdef DIRECT_DEC
    if((_u4FileCnt[u4InstID] >= _u4StartCompPicNum[u4InstID]) && ( _u4FileCnt[u4InstID] <= _u4EndCompPicNum[u4InstID]))
#endif    
    {
        if (_fgVP6CRCExist[u4InstID])
        {
            vVP6_CheckCRCResult(u4InstID);
        }
        //#else
        else
        {
            vVP6WrData2PC(u4InstID, _pucDumpYBuf[u4InstID], ((((prVDecVp6FrmHdr->u2WidthDec+ 15) >> 4) * ((prVDecVp6FrmHdr->u2HeightDec + 31) >> 5)) << 9),  &fgNextFrameExist);
        }
    }
    else
    {
        fgNextFrameExist = TRUE;
        _u4FileCnt[u4InstID] ++;
    }
    //#endif

    //if (fgDecOk)
    {
        VDEC_INFO_VP6_DEC_PRM_T *prVDecVP6DecPrm = &_tVerMpvDecPrm[_u1AlphaDecPrmIdx[u4InstID]].SpecDecPrm.rVDecVP6DecPrm;
        if (prVDecVP6DecPrm->u1AlphaFlag & VP6_ALPHA_ENABLE)
        {
            u4VDec_HAL_VP6_VDec_BackupSram(u4InstID, prVDecVP6DecPrm);
        }
    }
    // reset HW
#ifdef REDEC   
    if(_u4ReDecCnt[u4InstID] > 0)
    {
        rVp6VFifoInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
        rVp6VFifoInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
        i4VDEC_HAL_VP6_InitVDecHW(u4InstID,&rVp6VFifoInitPrm);
        rVp6BSInitPrm.u4VFifoSa =  (UINT32)_pucVFifo[u4InstID];
        rVp6BSInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
        rVp6BSInitPrm.u4ReadPointer =  (UINT32)_pucVFifo[u4InstID] + _u4VP6ByteCount[u4InstID];
#ifndef  RING_VFIFO_SUPPORT
        rVp6BSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
#else
        rVp6BSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ*(0.5 + 0.5 *(_u4LoadBitstreamCnt[u4InstID]%2))); // panda
        //   rVp6BSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));
#endif
        i4VDEC_HAL_VP6_InitBarrelShifter(0, u4InstID, &rVp6BSInitPrm);  
        u4VDEC_HAL_VP6_VDec_SetWorkspace(u4InstID, (UINT32)_pucVP6VLDWrapperWorkspace[u4InstID], (UINT32)_pucVP6PPWrapperWorkspace[u4InstID]);
        _tVerDec[u4InstID].ucState = DEC_NORM_WAIT_TO_DEC;
        return;
    }
#endif

    if (fgNextFrameExist)
    {
      if ((0 == _u1AlphaBitstream[u4InstID]) || (_u1AlphaBitstream[u4InstID] && (0 == (_u1AlphaFlag[u4InstID] & VP6_ALPHA_FRAME))))
        {
            u4Size0 = *(_pucSizeFileBuf[u4InstID] + ( (_u4FileCnt[u4InstID] *4) ) );
            u4Size1 = *(_pucSizeFileBuf[u4InstID] + ( (_u4FileCnt[u4InstID] *4) +1));
            u4Size2 = *(_pucSizeFileBuf[u4InstID] + ( (_u4FileCnt[u4InstID] *4) +2));
            u4Size3 = *(_pucSizeFileBuf[u4InstID] + ( (_u4FileCnt[u4InstID] *4) +3));
            u4VldByte = ( ((u4Size3 & 0xFF) << 24) | ((u4Size2 & 0xFF) << 16) | ((u4Size1 & 0xFF) << 8) | (u4Size0 & 0xFF) );
            _u4VP6ByteCount[u4InstID] += u4VldByte;
        }


        printk("<vdec> filecnt=%d, read_pointer=0x%08X (%u)\n", _u4FileCnt[u4InstID], _u4VP6ByteCount[u4InstID]);

        rVp6VFifoInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
        rVp6VFifoInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
        i4VDEC_HAL_VP6_InitVDecHW(u4InstID,&rVp6VFifoInitPrm);
        rVp6BSInitPrm.u4VFifoSa =  (UINT32)_pucVFifo[u4InstID];
        rVp6BSInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
        rVp6BSInitPrm.u4ReadPointer =  (UINT32)_pucVFifo[u4InstID] + _u4VP6ByteCount[u4InstID];
#ifndef  RING_VFIFO_SUPPORT
        rVp6BSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
#else
        //rVp6BSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ*(0.5 + 0.5 *(_u4LoadBitstreamCnt[u4InstID]%2))); // panda
           rVp6BSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));
#endif
        i4VDEC_HAL_VP6_InitBarrelShifter(_u4BSID[u4InstID], u4InstID, &rVp6BSInitPrm);
        u4VDEC_HAL_VP6_VDec_SetWorkspace(u4InstID, (UINT32)_pucVP6VLDWrapperWorkspace[u4InstID], (UINT32)_pucVP6PPWrapperWorkspace[u4InstID]);

        //MULTI-STREAM PANDA
        _rVp6BSPrm.u4VFifoSa = rVp6BSInitPrm.u4VFifoSa;
        _rVp6BSPrm.u4VFifoEa = rVp6BSInitPrm.u4VFifoEa;
        _rVp6BSPrm.u4ReadPointer = rVp6BSInitPrm.u4ReadPointer;
        _rVp6BSPrm.u4WritePointer = rVp6BSInitPrm.u4WritePointer;

#if (VDEC_VP6_ERR_TEST)
        u4Size0 = *(_pucSizeFileBuf[u4InstID] + ( ((_u4FileCnt[u4InstID]+1) *4) ) );
        u4Size1 = *(_pucSizeFileBuf[u4InstID] + ( ((_u4FileCnt[u4InstID]+1) *4) +1));
        u4Size2 = *(_pucSizeFileBuf[u4InstID] + ( ((_u4FileCnt[u4InstID]+1) *4) +2));
        u4Size3 = *(_pucSizeFileBuf[u4InstID] + ( ((_u4FileCnt[u4InstID]+1) *4) +3));
        u4BsSize = ( ((u4Size3 & 0xFF) << 24) | ((u4Size2 & 0xFF) << 16) | ((u4Size1 & 0xFF) << 8) | (u4Size0 & 0xFF) );   
        pBsData = (CHAR*)  (_pucVFifo[u4InstID] + _u4VP6ByteCount[u4InstID]);

        //
        if (u4BsSize > 20)
        {
            u4RndCnt = (UINT32) (rand()% (u4BsSize - 20));

            for (i=0; i <u4RndCnt; i++)
            {
                u4RndPos = 20 + (UINT32) (rand()% (u4BsSize - 20));

                if (u4RndPos < u4BsSize)
                    pBsData[u4RndPos] = (UINT32) (rand()%255);
            }
        }  
#endif


#ifndef INTERGRATION_WITH_DEMUX
#ifdef  RING_VFIFO_SUPPORT
        if ( (_u4LoadBitstreamCnt[u4InstID]&0x1) 
            && (rVp6BSInitPrm.u4ReadPointer  > ((UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ/2))) )
        {
            _tInFileInfo[u4InstID].fgGetFileInfo = TRUE;
            _tInFileInfo[u4InstID].pucTargetAddr = _pucVFifo[u4InstID];
            _tInFileInfo[u4InstID].u4FileOffset = (V_FIFO_SZ * ((_u4LoadBitstreamCnt[u4InstID]+ 1)/2));
            _tInFileInfo[u4InstID].u4TargetSz = (V_FIFO_SZ/2);    
            _tInFileInfo[u4InstID].u4FileLength = 0; 
#ifdef  SATA_HDD_READ_SUPPORT
            if(!fgOpenHDDFile(u4InstID, (char*)_bFileStr1[u4InstID][1],"r+b", &_tInFileInfo[u4InstID]))
            {
                fgOpenPCFile(u4InstID, (char*)_bFileStr1[u4InstID][1],"r+b", &_tInFileInfo[u4InstID]);
            }
#else
            fgOpenPCFile(u4InstID, (char*)_bFileStr1[u4InstID][1],"r+b", &_tInFileInfo[u4InstID]);
#endif  
            _u4LoadBitstreamCnt[u4InstID]++;
        }
        else
            if( (!(_u4LoadBitstreamCnt[u4InstID]&0x1)) 
                && (rVp6BSInitPrm.u4ReadPointer  < ((UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ/2))) )
            {
                _tInFileInfo[u4InstID].fgGetFileInfo = TRUE;
                _tInFileInfo[u4InstID].pucTargetAddr = _pucVFifo[u4InstID] + (V_FIFO_SZ/2);
                _tInFileInfo[u4InstID].u4FileOffset =  ((V_FIFO_SZ * (_u4LoadBitstreamCnt[u4InstID]+ 1)) /2);
                _tInFileInfo[u4InstID].u4TargetSz = (V_FIFO_SZ/2);    
                _tInFileInfo[u4InstID].u4FileLength = 0; 
#ifdef  SATA_HDD_READ_SUPPORT
                if(!fgOpenHDDFile(u4InstID, (char*)_bFileStr1[u4InstID][1],"r+b", &_tInFileInfo[u4InstID]))
                {
                    fgOpenPCFile(u4InstID, (char*)_bFileStr1[u4InstID][1],"r+b", &_tInFileInfo[u4InstID]);
                }
#else
                fgOpenPCFile(u4InstID, (char*)_bFileStr1[u4InstID][1],"r+b", &_tInFileInfo[u4InstID]);
#endif  
                _u4LoadBitstreamCnt[u4InstID]++;
            }
#endif
#endif

    }

    _tVerDec[u4InstID].ucState = DEC_NORM_VPARSER;

}


// *********************************************************************
// Function    : void vVerVP6UpdateBufStatus(UINT32 u4InstID)
// Description : Update VP6 Frame Buffer Status
// Parameter   : None
// Return      : None
// *********************************************************************
void vVerVP6UpdateBufStatus(UINT32 u4InstID)
{
    VDEC_INFO_VP6_FRM_HDR_T *prVDecVp6FrmHdr = &_rVDecVp6FrmHdr[_u1AlphaDecPrmIdx[u4InstID]];   

    u4InstID = _u1AlphaDecPrmIdx[u4InstID];
    _u4PrevfBufIdx[u4InstID] = _u4DecBufIdx[u4InstID];    //Previous frame buf Index

    if (prVDecVp6FrmHdr->ucFrameType == VP6_I_FRM || prVDecVp6FrmHdr->fgRefreshGoldenFrame)
    {
        _u4GoldfBufIdx[u4InstID] = _u4DecBufIdx[u4InstID];    //Golden frame buf Index
    }
}

// *********************************************************************
// Function    : BOOL fgIsVP6VDecComplete(UINT32 u4InstID)
// Description : Check if VDec complete with interrupt
// Parameter   : None
// Return      : None
// *********************************************************************
BOOL fgIsVP6VDecComplete(UINT32 u4InstID)
{
    UINT32 u4MbX;
    UINT32 u4MbY;  
    VDEC_INFO_VP6_FRM_HDR_T *prVDecVp6FrmHdr = &_rVDecVp6FrmHdr[_u1AlphaDecPrmIdx[u4InstID]];

    if(_fgVDecComplete[u4InstID])
    {
        vVDEC_HAL_VP6_GetMbxMby(u4InstID, &u4MbX, &u4MbY);

        if( (u4MbX < (prVDecVp6FrmHdr->u2WidthDec / 16 - 1) ) || (u4MbY < (prVDecVp6FrmHdr->u2HeightDec / 16 -1)) )
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


extern UINT32 u4VDecReadVP6MC(UINT32 u4VDecID, UINT32 u4Addr);
extern UINT32 u4VDecReadMC(UINT32 u4VDecID, UINT32 u4Addr);
extern UINT32 u4VDecReadVP6DCAC(UINT32 u4VDecID, UINT32 u4Addr);
extern UINT32 u4VDecReadVP6PP(UINT32 u4VDecID, UINT32 u4Addr);
extern UINT32 u4VDecVP6ReadVLD2(UINT32 u4Addr);



void vVerVP6DecEndProc(UINT32 u4InstID)
{
    UINT32 u4Cnt;
    UINT32 u4CntTimeChk;
    UINT32 u4MbX;
    UINT32 u4MbY;  
    UCHAR strMessage[256];
    UINT32 u4MbX_last;
    UINT32 u4MbY_last;
    UINT32 u4VP6ErrType = 0;
    VDEC_INFO_VP6_ERR_INFO_T rVp6ErrInfo;
    VDEC_INFO_VP6_FRM_HDR_T *prVDecVp6FrmHdr = &_rVDecVp6FrmHdr[_u1AlphaDecPrmIdx[u4InstID]];
#if 0//ChunChia_LOG
    UINT32 u4Mc770, u4Mc774, u4Mc778, u4Mc8B8;
#endif

    u4Cnt=0;
    u4CntTimeChk = 0;
    _fgVDecErr[u4InstID] = FALSE;

    {

        while(u4CntTimeChk < DEC_RETRY_NUM)
        {    
            u4Cnt ++;    
            if((u4Cnt & 0xFF)== 0xFF)
            {
#ifndef IRQ_DISABLE    
#else
                if(
                    u4VDEC_HAL_VP6_VDec_ReadFinishFlag(u4InstID) & 0x1
#if VDEC_DDR3_SUPPORT
                    && i4VDEC_HAL_VP6_DDR3_DecFinish(u4InstID)
#endif
                    )
                {
                    printk("<vdec> VP6 Got finish flag\n");
                    _fgVDecComplete[u4InstID] = TRUE;
#ifdef CAPTURE_ESA_LOG
                    vWrite2PC(u4InstID, 17, (UCHAR*)_pucESALog[u4InstID]);
#endif
                    /*          if(u4InstID == 0)
                    {
                    BIM_ClearIrq(VECTOR_VDFUL);
                    }
                    else
                    {
                    BIM_ClearIrq(VECTOR_VDLIT);
                    }*/
                }
#endif      
                if(fgIsVP6VDecComplete(u4InstID))
                {
                    u4CntTimeChk = 0;
                    break;
                }
                else
                {
                    u4MbX_last = u4MbX;
                    u4MbY_last = u4MbY;
                    vVDEC_HAL_VP6_GetMbxMby(u4InstID, &u4MbX, &u4MbY);
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

        vVDEC_HAL_VP6_GetErrInfo(u4InstID, &rVp6ErrInfo);
        u4VP6ErrType = u4VDEC_HAL_VP6_GetErrType(u4InstID);
        if((u4CntTimeChk == DEC_RETRY_NUM) || (u4VP6ErrType!= 0) || (rVp6ErrInfo.u4Vp6ErrCnt != 0))
        {
#ifndef INTERGRATION_WITH_DEMUX
            //#ifdef EXT_COMPARE     
            _fgVDecErr[u4InstID] = TRUE;
            //#endif
            if(u4CntTimeChk == DEC_RETRY_NUM)
            {
                vVDecOutputDebugString("\n!!!!!!!!! Decoding Timeout !!!!!!!\n");
                sprintf(strMessage, "%s", "\n!!!!!!!!! Decoding Timeout !!!!!!!");

                vp6_dump_register(u4InstID);

                fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
                //vDumpReg();
            }
            vVDEC_HAL_VP6_GetMbxMby(u4InstID, &u4MbX, &u4MbY);
            vVDecOutputDebugString("\n!!!!!!!!! Decoding Error 0x%.8x!!!!!!!\n", rVp6ErrInfo.u4Vp6ErrType);
            sprintf(strMessage,"\n!!!!!!!!! Decoding Error 0x%.8x 0x%.8x 0x%.8xat MC (x,y)=(%d/%d, %d/%d)  !!!!!!!\n", u4VP6ErrType, 
                rVp6ErrInfo.u4Vp6ErrType,rVp6ErrInfo.u4Vp6ErrRow,u4MbX, prVDecVp6FrmHdr->u2HFragments -1, u4MbY, prVDecVp6FrmHdr->u2VFragments -1);
            fgWrMsg2PC((void*)strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
            sprintf(strMessage,"the length is %d (0x%.8x)\n", _tInFileInfo[u4InstID].u4FileLength, _tInFileInfo[u4InstID].u4FileLength);
            fgWrMsg2PC((void*)strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
            //vReadVP6ChkSumGolden(u4InstID);
            vWrite2PC(u4InstID, 1, (UCHAR*)_pucVFifo[u4InstID]);
            vWrite2PC(u4InstID, 12, (UCHAR *)(&_u4DumpChksum[u4InstID][0]));
            //vDumpReg();
#endif
        }

        //vVDEC_HAL_VP6_AlignRdPtr(0, u4InstID, (UINT32)_pucVFifo[u4InstID], BYTE_ALIGN);
        vVerifySetVSyncPrmBufPtr(u4InstID, _u4DecBufIdx[_u1AlphaDecPrmIdx[u4InstID]]);
        //vReadVP6ChkSumGolden(u4InstID);
    }

    //Print LOG
    sprintf(strMessage,"<vdec> ======\n");  
    printk("%s", strMessage);
#if 0//ChunChia_LOG  
    u4Mc770 = u4VDecReadMC(u4InstID, 0x770);
    u4Mc774 = u4VDecReadMC(u4InstID, 0x774);
    u4Mc778 = u4VDecReadMC(u4InstID, 0x778);
    u4Mc8B8 = u4VDecReadMC(u4InstID, 0x8B8);

    sprintf(strMessage,"======\n");  
    printk("%s", strMessage);

    sprintf(strMessage,"(dram_dle_cnt: 0x%x, mc_dle_cnt: 0x%x, cycle_cnt: 0x%x, dram_dle_by_preq: 0x%x)\n", u4Mc770, u4Mc774, u4Mc778, u4Mc8B8);
    printk("%s", strMessage);

#endif

    vVerVP6VDecEnd(u4InstID);
    vVerVP6UpdateBufStatus(u4InstID);
}


