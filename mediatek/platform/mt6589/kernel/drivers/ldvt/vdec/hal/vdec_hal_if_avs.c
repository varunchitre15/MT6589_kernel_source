
#include "vdec_hw_common.h"
#include "vdec_hal_if_avs.h"
#include "vdec_hw_avs.h"
#include "vdec_hal_errcode.h"
//#include "x_hal_ic.h"
//#include "x_hal_1176.h"
//#include "x_debug.h"


#if CONFIG_DRV_VERIFY_SUPPORT
#include "../verify/vdec_verify_general.h"
#include "../verify/vdec_verify_mpv_prov.h"
#include "../include/drv_common.h"
#include <linux/string.h>

#if (!CONFIG_DRV_LINUX)
#include <string.h>
#include <stdio.h>
#include "x_printf.h"
#endif

extern BOOL fgWrMsg2PC(void* pvAddr, UINT32 u4Size, UINT32 u4Mode, VDEC_INFO_VERIFY_FILE_INFO_T *pFILE_INFO);
#endif

// **************************************************************************
// Function : INT32 i4VDEC_HAL_AVS_InitVDecHW(UINT32 u4Handle, VDEC_INFO_AVS_INIT_PRM_T *prAVSVDecInitPrm);
// Description :Initialize video decoder hardware only for AVS
// Parameter : u4VDecID : video decoder hardware ID
//                  prAVSVDecInitPrm : pointer to VFIFO info struct
// Return      : =0: success.
//                  <0: fail.
// **************************************************************************
INT32 i4VDEC_HAL_AVS_InitVDecHW(UINT32 u4VDecID, VDEC_INFO_AVS_VFIFO_PRM_T *prAVSVDecInitPrm)
{
    vVDecResetHW(u4VDecID, VDEC_AVS);
    return  HAL_HANDLE_OK;
}


// **************************************************************************
// Function : UINT32 u4VDEC_HAL_AVS_ShiftGetBitStream(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits);
// Description :Read barrel shifter after shifting
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
//                 u4ShiftBits : shift bits number
// Return      : Value of barrel shifter input window after shifting
// **************************************************************************
UINT32 u4VDEC_HAL_AVS_ShiftGetBitStream(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits)
{
    UINT32 u4RegVal;
  
    u4RegVal = u4VDecAVSVLDGetBitS(u4BSID, u4VDecID, u4ShiftBits);
  
    return (u4RegVal);
}


// **************************************************************************
// Function : UINT32 u4VDEC_HAL_AVS_GetBitStreamShift(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits);
// Description :Read Barrel Shifter before shifting
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
//                 u4ShiftBits : shift bits number
// Return      : Value of barrel shifter input window before shifting
// **************************************************************************
UINT32 u4VDEC_HAL_AVS_GetBitStreamShift(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits)
{
    UINT32 u4RegVal0;
  
    u4RegVal0 = u4VDecAVSVLDGetBitS(u4BSID, u4VDecID, 0);
    u4VDecAVSVLDGetBitS(u4BSID, u4VDecID, u4ShiftBits);
    
    return (u4RegVal0);
}


// **************************************************************************
// Function : UINT32 u4VDEC_HAL_AVS_GetRealBitStream(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits);
// Description :Read Barrel Shifter before shifting
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
//                 u4ShiftBits : shift bits number
// Return      : Most significant (32 - u4ShiftBits) bits of barrel shifter input window before shifting
// **************************************************************************
UINT32 u4VDEC_HAL_AVS_GetRealBitStream(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits)
{
    UINT32 u4RegVal0;
  
    u4RegVal0 = u4VDecAVSVLDGetBitS(u4BSID, u4VDecID, 0);
    u4VDecAVSVLDGetBitS(u4BSID, u4VDecID, u4ShiftBits);
    
    return (u4RegVal0 >> (32-u4ShiftBits));
}


// **************************************************************************
// Function : INT32 i4VDEC_HAL_AVS_InitBarrelShifter(UINT32 u4BSID, UINT32 u4VDecID, VDEC_INFO_AVS_BS_INIT_PRM_T *prAVSBSInitPrm);
// Description :Initialize barrel shifter with byte alignment
// Parameter :u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
//                 prAVSBSInitPrm : pointer to AVS initialize barrel shifter information struct
// Return      : =0: success.
//                  <0: fail.
// **************************************************************************
INT32 i4VDEC_HAL_AVS_InitBarrelShifter(UINT32 u4BSID, UINT32 u4VDecID, VDEC_INFO_AVS_BS_INIT_PRM_T *prAVSBSInitPrm)
{
    BOOL fgInitBSResult;
    
    if (u4BSID == 0)
    {
        vVDecResetHW(u4VDecID, VDEC_AVS);
        fgInitBSResult = fgInitAVSBarrelShift1(u4VDecID, prAVSBSInitPrm);
    }
    else
    {
        fgInitBSResult = fgInitAVSBarrelShift2(u4VDecID, prAVSBSInitPrm);
    }

    if (fgInitBSResult)
    {
        return HAL_HANDLE_OK;
    }
    else
    {
        return INIT_BARRELSHIFTER_FAIL;
    }
}


void vVDEC_HAL_AVS_BitStuff_Check(UINT32 u4VDecID, UINT32 u4Val)
{
   vVDecWriteAVSVLD(u4VDecID, RW_AVS_VLD_BITSTUFFING_SWITCH, u4Val);
}

// **************************************************************************
// Function : UINT32 u4VDEC_HAL_AVS_ReadRdPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 *pu4Bits);
// Description :Read current read pointer
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
//                 pu4Bits : read pointer value with remained bits
// Return      : Read pointer value with byte alignment
// **************************************************************************
UINT32 u4VDEC_HAL_AVS_ReadRdPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4VFIFOSa, UINT32 *pu4Bits)
{
    return u4VDecReadAVSVldRPtr(u4BSID, u4VDecID, pu4Bits, PHYSICAL(u4VFIFOSa));
}



UINT32 u4VDEC_AvsUeCodeNum(UINT32 u4BSID, UINT32 u4VDecID)
{
    if (u4BSID == 0)
    {
        return u4VDecReadAVSVLD(u4VDecID, RO_AVS_VLD_UE);
    }
    else
    {
        return u4VDecReadAVSVLD(u4VDecID, RO_AVLD_2ND_UE);
    }
}

UINT32 u4VDEC_AvsSeCodeNum(UINT32 u4BSID, UINT32 u4VDecID)
{
    UINT32 u4Temp;    

    u4Temp = u4VDecReadAVSVLD(u4VDecID, RO_AVS_VLD_SE);

    return(u4Temp);
}

UINT32 u4VDEC_AVS_Search_SC(UINT32 u4BSID, UINT32 u4VDecID)
{
    UINT32 u4RetryNum = 0;
    UINT32 u4Temp = 1;
    
    vVDecWriteAVSVLD(u4VDecID, RW_AVS_FIND_STARTCODE, 1);
    while (1)
    {        
        while(u4VDecReadAVSVLD(u4VDecID, RW_AVS_FIND_STARTCODE))
        {
           if (u4RetryNum == 0x100000)
           {
              u4Temp = 0;
              break;
           }
           u4RetryNum++;
        }
        // Shift 0x000001 for read pointer
        u4Temp = u4VDEC_HAL_AVS_ShiftGetBitStream(u4BSID, u4VDecID, 0);
        if (((u4Temp >> 8) == 0x000001) &&
            ((u4Temp & 0x000000FF) <= 0xB7))
        {
            //Find Start Code.
            break;
        }
        
        //Search Next Start Code.
        vVDecWriteAVSVLD(u4VDecID, RW_AVS_FIND_STARTCODE, 1);
    }    

    return(u4Temp);
}

UINT32 u4VDEC_AVS_Search_SliceSC(UINT32 u4BSID, UINT32 u4VDecID)
{
    UINT32 u4RetryNum = 0;
    UINT32 u4Temp = 1;
    UINT32 u4Ret = TRUE;
    
    vVDecWriteAVSVLD(u4VDecID, RW_AVS_FIND_STARTCODE, 1);
    while (1)
    {        
        while(u4VDecReadAVSVLD(u4VDecID, RW_AVS_FIND_STARTCODE))
        {
           if (u4RetryNum == 0x100000)
           {
              u4Ret = FALSE;
              break;
           }
           u4RetryNum++;
        }
        // Shift 0x000001 for read pointer
        u4Temp = u4VDEC_HAL_AVS_ShiftGetBitStream(u4BSID, u4VDecID, 0);
        if (((u4Temp >> 8) == 0x000001) &&
            ((u4Temp & 0x000000FF) <= 0xAF))
        {
            //Find Slice Start Code.
            //printk("avs find slice start code: 0x%x\n", u4Temp);
            break;
        }
        else
        {
           printk("avs cannot find slice start code= 0x%x\n", u4Temp);
        }
        u4Temp = u4VDEC_HAL_AVS_ShiftGetBitStream(u4BSID, u4VDecID, 8);
        //Search Next Start Code.
        vVDecWriteAVSVLD(u4VDecID, RW_AVS_FIND_STARTCODE, 1);
    }    

    return(u4Ret);
}

void vVDEC_HAL_AVS_HW_Reset(UINT32 u4BSID, UINT32 u4VDecID)
{
    UINT32 u4RegVal;
    vVDecWriteVLD(u4VDecID, WO_VLD_SRST, 1);
    vVDecWriteVLD(u4VDecID, WO_VLD_SRST, 0);
    u4RegVal = (u4VDecReadVLD(u4VDecID, RW_VLD_VDOUFM) & 0xFFFFF7FF) | (0x1 << 11);
    vVDecWriteVLD(u4VDecID, RW_VLD_VDOUFM, u4RegVal);  
}

INT32 i4VDEC_HAL_AVS_SetBSInfo(UINT32 u4BSID, UINT32 u4InstID, VDEC_INFO_DEC_PRM_T *prDecParam, VDEC_INFO_AVS_BS_INIT_PRM_T rAvsBSInitPrm)
{
    #if 0//def WAIT_SRAM_STABLE
    UINT32 u4Cnt = 0;
    #endif
    UINT32 u4Ret;

    #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    vVDecWriteAVSVLD(u4InstID, RW_AVS_VLD_CTRL, 0x7); 
    //vVDecWriteVLDTOP(u4InstID, RW_VLD_TOP_ERR_CONCEAL, (0x10 << 28));  
    #else
    vVDecWriteAVSVLD(u4InstID, RW_AVS_VLD_CTRL, 0x8007); 
    #endif
    vVDecWriteAVSVLD(u4InstID, RW_AVS_VLD_MODE_SWITCH, 7);
    
    // Set for VLD boundary check 0x5000000
    if(u4BSID == 1)
    {
    #if 0
        _VDEC_AvsVLDHWReset2(FALSE);
        while(!_VDEC_AvsResetVLD2(PHYSICAL(u4RDPtrAddr), PHYSICAL(u4WPtrAddr), 
            PHYSICAL(u4BsFifoStart), PHYSICAL(u4BsFifoEnd)))
        {
            _VDEC_AvsVLDHWReset2(TRUE);
        }
    #endif        
    }
    else
    {
        #if 0//def WAIT_SRAM_STABLE
#ifdef CC_AVS_EMULATION
        while(!_VDEC_AvsResetVLD1(prDecParam, PHYSICAL(u4RDPtrAddr), PHYSICAL(u4WPtrAddr),
            PHYSICAL(u4BsFifoStart), PHYSICAL(u4BsFifoEnd), PHYSICAL(0), u1ECCMode, u4ECType))
#else            
        while(!_VDEC_AvsResetVLD1(prDecParam, PHYSICAL(u4RDPtrAddr), PHYSICAL(u4WPtrAddr),
            PHYSICAL(u4BsFifoStart), PHYSICAL(u4BsFifoEnd), PHYSICAL(0), u1ECCMode, u4ECType))
#endif            
        
        UNUSED(u4Cnt);
        #else               
        u4Ret = i4VDEC_HAL_AVS_InitBarrelShifter(u4BSID, u4InstID, &rAvsBSInitPrm);
        if( u4Ret != HAL_HANDLE_OK)
        {
            printk( "Barrel Shifter1 Init failed");
        }
        #endif
    }

    return 0;
}

static void vVDEC_HAL_AVS_SetMvReg(UINT32 u4VDecID, const VDEC_INFO_AVS_DEC_PRM_T* prVDecAvsDecPrm)
{
    INT32 ai4BlkDistFw[4] = {0};
    INT32 ai4BlkDistBw[2] = {0};
    INT32 ai4DirectRatio[4] = {0};
    INT32 ai4MedRatioFw[4] = {0};
    INT32 ai4MedRatioBw[4] = {0};

    INT32 i4FwBlkDist1_0 = 0;
    INT32 i4FwBlkDist3_2 = 0;
    INT32 i4BwBlkDist1_0 = 0;
    INT32 i4FwMedRatio1_0 = 0;
    INT32 i4FwMedRatio3_2 = 0;
    INT32 i4BwMedRatio1_0 = 0;
    INT32 i4DirectRatio1_0 = 0;
    INT32 i4DirectRatio3_2 = 0;    
    INT32 i = 0;
    UINT32 u4TopMvBufAddr, u4BottomMvBufAddr;
    VDEC_INFO_AVS_PIC_HDR_T *prPicHdr = prVDecAvsDecPrm->prPicHdr;

    u4TopMvBufAddr = PHYSICAL(prVDecAvsDecPrm->rAvsWorkBufSa.u4Mv1);
    u4BottomMvBufAddr = PHYSICAL(prVDecAvsDecPrm->rAvsWorkBufSa.u4Mv2);
    if ((!u4TopMvBufAddr) || (!u4BottomMvBufAddr))
    {
        printk( "_VDEC_AvsSetMvReg NULL motion vector address\n");
        return;
    }

    if (prPicHdr->u4PicStruct != FRM_PIC)
    {
        if (prPicHdr->u4TFT)
        {
            if (!prPicHdr->fgSecField)
            {
                vVDecWriteAVSMV(u4VDecID, RW_AVS_MV_ADDR, u4TopMvBufAddr >> 4);
            }
            else
            {
                vVDecWriteAVSMV(u4VDecID, RW_AVS_MV_ADDR, u4BottomMvBufAddr >> 4);
            }
        }
        else
        {
            if (!prPicHdr->fgSecField)
            {
                vVDecWriteAVSMV(u4VDecID, RW_AVS_MV_ADDR, u4BottomMvBufAddr >> 4);
            }
            else
            {
                vVDecWriteAVSMV(u4VDecID, RW_AVS_MV_ADDR, u4TopMvBufAddr >> 4);
            }
        }
    }
    else
    {
        vVDecWriteAVSMV(u4VDecID, RW_AVS_MV_ADDR, u4TopMvBufAddr >> 4);
    }

    //x_memset(ai4BlkDistFw, 0, sizeof(INT32) * 4);
    //x_memset(ai4BlkDistBw, 0, sizeof(INT32) * 2);
    //x_memset(ai4DirectRatio, 0, sizeof(INT32) * 4);
    //x_memset(ai4MedRatioFw, 0, sizeof(INT32) * 4);
    //x_memset(ai4MedRatioBw, 0, sizeof(INT32) * 4);
    
    // If the decoding source is the bottom field or the second field, 
    // the index distance should be picture distance * 2 + 1.
    // Otherwise, the index distance should be picture distance * 2
    if (prPicHdr->u4PicCodingType == I_PIC)
    {
        if ((prPicHdr->u4PicStruct == FRM_PIC) || (!prPicHdr->fgSecField))
        {
            printk("I frame/I first field\n");
        }
        else
        {          
            ai4BlkDistFw[0] = 1 ;
            ai4BlkDistFw[1] = (INT32)((INT32)(prPicHdr->u4PicDistance) * 2 - (INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_LAST_P_IDX].u4PicDistance) * 2);
            ai4BlkDistFw[2] = (INT32)((INT32)(prPicHdr->u4PicDistance) * 2 - (INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_LAST_P_IDX].u4PicDistance) * 2) + 1;
            //if (AVS_LAST_P_IDX >= REF_LIST_1)
            {
                ai4BlkDistFw[3] = (INT32)((INT32)(prPicHdr->u4PicDistance) * 2 - (INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_PREV_P_IDX].u4PicDistance) * 2);
            }
            printk("I second field\n");
        }
    }
    else 
    if(prPicHdr->u4PicCodingType == P_PIC)
    {
        if (prPicHdr->u4PicStruct == FRM_PIC)
        {            
            ai4BlkDistFw[0] = (INT32)((INT32)(prPicHdr->u4PicDistance) * 2 - (INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_LAST_P_IDX].u4PicDistance) * 2);
            //if (AVS_LAST_P_IDX >= REF_LIST_1)
            {
                ai4BlkDistFw[1] = (INT32)((INT32)(prPicHdr->u4PicDistance) * 2 - (INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_PREV_P_IDX].u4PicDistance) * 2);
            }
            printk( "P Frame\n");
        }
        else
        {
            if (prPicHdr->u4TFT)
            {
                if (!prPicHdr->fgSecField)
                {                    
                    ai4BlkDistFw[0] = (INT32)((INT32)(prPicHdr->u4PicDistance) * 2 - (INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_LAST_P_IDX].u4PicDistance) * 2) - 1;
                    ai4BlkDistFw[1] = (INT32)((INT32)(prPicHdr->u4PicDistance) * 2 - (INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_LAST_P_IDX].u4PicDistance) * 2);
                    //if (AVS_LAST_P_IDX >= REF_LIST_1)
                    {
                        ai4BlkDistFw[2] = (INT32)((INT32)(prPicHdr->u4PicDistance) * 2 - (INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_PREV_P_IDX].u4PicDistance) * 2) - 1;
                        ai4BlkDistFw[3] = (INT32)((INT32)(prPicHdr->u4PicDistance) * 2 - (INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_PREV_P_IDX].u4PicDistance) * 2);
                    }
                    printk( "P first top field\n");
                }
                else
                {                    
                    ai4BlkDistFw[0] = 1 ;
                    ai4BlkDistFw[1] = (INT32)((INT32)(prPicHdr->u4PicDistance) * 2 - (INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_LAST_P_IDX].u4PicDistance) * 2);
                    ai4BlkDistFw[2] = (INT32)((INT32)(prPicHdr->u4PicDistance) * 2 - (INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_LAST_P_IDX].u4PicDistance) * 2) + 1;
                    //if (AVS_LAST_P_IDX >= REF_LIST_1)
                    {
                        ai4BlkDistFw[3] = (INT32)((INT32)(prPicHdr->u4PicDistance) * 2 - (INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_PREV_P_IDX].u4PicDistance) * 2);
                    }
                    printk( "P second bottom field\n");
                }
            }
        }
    }
    else
    {               
        if (prPicHdr->u4PicStruct == FRM_PIC)
        {
            ai4BlkDistFw[0] = (INT32)((INT32)(prPicHdr->u4PicDistance) * 2 - (INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_FW_REF_IDX].u4PicDistance) * 2);
            ai4BlkDistBw[0] = (INT32)((INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_BW_REF_IDX].u4PicDistance) * 2- (INT32)(prPicHdr->u4PicDistance) * 2);

            ai4DirectRatio[0] =	(INT32)((INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_BW_REF_IDX].u4PicDistance) * 2- 
                (INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_FW_REF_IDX].u4PicDistance) * 2);

            //f (AVS_BW_REF_IDX > REF_LIST_1)
            {
                ai4DirectRatio[1] = (INT32)((INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_BW_REF_IDX].u4PicDistance) * 2- 
                    (INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_PREV_FW_IDX].u4PicDistance) * 2);
            }

            if (prVDecAvsDecPrm->arFBufInfo[AVS_BW_REF_IDX].u1PicCodingType == I_PIC)
            {
                vVDecWriteAVSMV(u4VDecID, RW_AVS_MV_BW_I_REF, 1);
            }
            else
            {
                vVDecWriteAVSMV(u4VDecID, RW_AVS_MV_BW_I_REF, 0);
            }
            printk( "B frame\n");
        }
        else
        {
            if (prPicHdr->u4TFT)
            {
                // Top field
                if (!prPicHdr->fgSecField)
                { 
                    ai4BlkDistFw[0] = (INT32)((INT32)(prPicHdr->u4PicDistance) * 2 - (INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_FW_REF_IDX].u4PicDistance) * 2) - 1;
                    ai4BlkDistFw[1] = (INT32)((INT32)(prPicHdr->u4PicDistance) * 2 - (INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_FW_REF_IDX].u4PicDistance) * 2);

                    ai4BlkDistBw[0] = (INT32)((INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_BW_REF_IDX].u4PicDistance) * 2 - (INT32)(prPicHdr->u4PicDistance) * 2);
                    ai4BlkDistBw[1] = (INT32)((INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_BW_REF_IDX].u4PicDistance) * 2 - (INT32)(prPicHdr->u4PicDistance) * 2) + 1;

                    ai4DirectRatio[0] =	(INT32)((INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_BW_REF_IDX].u4PicDistance) * 2 - (INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_FW_REF_IDX].u4PicDistance) * 2) - 1;
                    ai4DirectRatio[1] =	(INT32)((INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_BW_REF_IDX].u4PicDistance) * 2 - (INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_FW_REF_IDX].u4PicDistance) * 2);

                    //if (AVS_BW_REF_IDX > REF_LIST_1)
                    {                        
                        ai4DirectRatio[2] =	(INT32)((INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_BW_REF_IDX].u4PicDistance * 2) - (INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_PREV_FW_IDX].u4PicDistance) * 2) - 1;
                        ai4DirectRatio[3] =	(INT32)((INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_BW_REF_IDX].u4PicDistance * 2) - (INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_PREV_FW_IDX].u4PicDistance) * 2);
                    }


                    if (prVDecAvsDecPrm->arFBufInfo[AVS_BW_REF_IDX].u1PicCodingType == I_PIC)
                    {
                        vVDecWriteAVSMV(u4VDecID, RW_AVS_MV_BW_I_REF, 1);
                    }
                    else
                    {
                        vVDecWriteAVSMV(u4VDecID, RW_AVS_MV_BW_I_REF, 0);
                    }

                    printk( "B first top field\n");
                }
                else
                {
                    ai4BlkDistFw[0] = (INT32)((INT32)(prPicHdr->u4PicDistance) * 2 - (INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_FW_REF_IDX].u4PicDistance) * 2);
                    ai4BlkDistFw[1] = (INT32)((INT32)(prPicHdr->u4PicDistance) * 2 - (INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_FW_REF_IDX].u4PicDistance) * 2) + 1;

                    ai4BlkDistBw[0] = (INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_BW_REF_IDX].u4PicDistance) * 2 - (INT32)(prPicHdr->u4PicDistance) * 2 -1;
                    ai4BlkDistBw[1] = (INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_BW_REF_IDX].u4PicDistance) * 2 - (INT32)(prPicHdr->u4PicDistance) * 2;

                    ai4DirectRatio[0] =	1;
                    ai4DirectRatio[1] =	(INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_BW_REF_IDX].u4PicDistance * 2 - (INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_FW_REF_IDX].u4PicDistance) * 2);
                    ai4DirectRatio[2] =	(INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_BW_REF_IDX].u4PicDistance * 2 - (INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_FW_REF_IDX].u4PicDistance) * 2) + 1;

                    //if (prVDecAvsDecPrm->u1LastRefIdx > REF_LIST_1)
                    {
                        ai4DirectRatio[3] =	(INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_BW_REF_IDX].u4PicDistance) * 2 - (INT32)(prVDecAvsDecPrm->arFBufInfo[AVS_PREV_FW_IDX].u4PicDistance) * 2;
                    }

                    vVDecWriteAVSMV(u4VDecID, RW_AVS_MV_BW_I_REF, 0);
                    printk( "B second bottom field\n");
                }					
            }
            else
            {
            }
        }
    }        

    for(i = 0; i < 4; i++)
    {
        ai4BlkDistFw[i] = (ai4BlkDistFw[i] + 512)%512;
        ai4DirectRatio[i] = (ai4DirectRatio[i] + 512)%512;
        
        if(ai4BlkDistFw[i] > 0)
        {
            ai4MedRatioFw[i] = 512/ai4BlkDistFw[i];
        }

        if(ai4DirectRatio[i] > 0)
        {
            ai4DirectRatio[i] = 16384/ai4DirectRatio[i];
        }
    }
    
    for(i = 0; i < 2; i++)
    {
        ai4BlkDistBw[i] = (ai4BlkDistBw[i] + 512)%512;
        
        if(ai4BlkDistBw[i] > 0)
        {
            ai4MedRatioBw[i] = 512 /ai4BlkDistBw[i];
        }
    }

    i4FwBlkDist1_0 = ai4BlkDistFw[0] | (ai4BlkDistFw[1] << 16);
    i4FwBlkDist3_2 = ai4BlkDistFw[2] | (ai4BlkDistFw[3] << 16);
    i4BwBlkDist1_0 = ai4BlkDistBw[0] | (ai4BlkDistBw[1] << 16);
    i4FwMedRatio1_0 = ai4MedRatioFw[0] | (ai4MedRatioFw[1] << 16);
    i4FwMedRatio3_2 = ai4MedRatioFw[2] | (ai4MedRatioFw[3] << 16);
    i4BwMedRatio1_0 = ai4MedRatioBw[0] | (ai4MedRatioBw[1] << 16);
    i4DirectRatio1_0 = ai4DirectRatio[0] | (ai4DirectRatio[1] << 16);
    i4DirectRatio3_2 = ai4DirectRatio[2] | (ai4DirectRatio[3] << 16);
   
    vVDecWriteAVSMV(u4VDecID, RW_AVS_BLK_FW_DISTANCE_1_0, i4FwBlkDist1_0);
    vVDecWriteAVSMV(u4VDecID, RW_AVS_BLK_FW_DISTANCE_3_2, i4FwBlkDist3_2);
    vVDecWriteAVSMV(u4VDecID, RW_AVS_BLK_BW_DISTANCE_1_0, i4BwBlkDist1_0);
    vVDecWriteAVSMV(u4VDecID, RW_AVS_BLK_FW_MED_RATIO_1_0, i4FwMedRatio1_0);
    vVDecWriteAVSMV(u4VDecID, RW_AVS_MV_FW_MED_RATIO_3_2, i4FwMedRatio3_2);
    vVDecWriteAVSMV(u4VDecID, RW_AVS_MV_BW_MED_RATIO_1_0, i4BwMedRatio1_0);
    vVDecWriteAVSMV(u4VDecID, RW_AVS_MV_DIRECT_RATIO_1_0, i4DirectRatio1_0);
    vVDecWriteAVSMV(u4VDecID, RW_AVS_MV_DIRECT_RATIO_3_2, i4DirectRatio3_2);
}

static void vVDEC_HAL_AVS_FillQTable2Sram(UINT32 u4VDecID)
{
    // Because I don't want to waste memory, I use hard codes for Q table 
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 0);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0xdd38ff);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 4);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0x5678d08);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 8);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0xa49638ff);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 12);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0x6167451d);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 16);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0xff963851);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 20);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0x5b9f3c1);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 24);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0x299638d6);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 28);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0x2e904598);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 32);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0xff9638ff);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 36);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0xf05345ac);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 40);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0xffa038ff);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 44);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0x5673bb7);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 48);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0xff963d04);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 52);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0x56745c1);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 56);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0x3963501);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 60);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0x56745c3);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 64);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0x808c98a5);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 68);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0xb5c5d8ec);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 72);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0x808b98a5);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 76);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0xb4c5d7ea);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 80);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0xff8b98a6);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 84);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0xb5c5d6ea);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 88);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0x808b98a5);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 92);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0xb5c5d7ea);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 96);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0xff8b98a5);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 100);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0xb4c5d7ea);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 104);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0xff8b98a5);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 108);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0xb5c5d7ea);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 112);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0xff8b98a6);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 116);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0xb5c5d7ea);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 120);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0x808b98a6);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 124);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0xb5c5d7ea);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 128);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0xe0e0e0e);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 132);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0xe0e0e0e);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 136);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0xd0d0d0d);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 140);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0xd0d0d0d);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 144);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0xd0c0c0c);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 148);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0xc0c0c0c);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 152);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0xb0b0b0b);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 156);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0xb0b0b0b);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 160);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0xb0a0a0a);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 164);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0xa0a0a0a);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 168);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0xa090909);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 172);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0x9090909);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 176);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0x9080808);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 180);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0x8080808);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 184);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0x7070707);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE_POS, 188);
    vVDecWriteVLD(u4VDecID, RW_AVS_VLD_QUANT_TABLE, 0x7070707);
}

static void vVDEC_HAL_AVS_SetMcReg(UINT32 u4VDecID, const VDEC_INFO_AVS_DEC_PRM_T* prVDecAvsDecPrm)
{
    UINT32 u4WidthMb;
    //UINT32 u4FbWidth, u4FbHeight;

    VDEC_INFO_AVS_SEQ_HDR_T *prSeqHdr = prVDecAvsDecPrm->prSeqHdr;
    VDEC_INFO_AVS_PIC_HDR_T *prPicHdr = prVDecAvsDecPrm->prPicHdr;

#if 0
    if (prDecParam->ucAddrSwapMode == ADDR_MODE_5351_NORMAL)
    {
        u4WidthMb = (UINT32)((prVDecAvsDecPrm->u4LineSize + 15) >> 4);
        u4WidthMb = ((u4WidthMb + 3) / 4) * 4;
        vVDecWriteVLD(u4VDecID, RW_VLD_MCPICSZ, u4WidthMb);
        LOG(6, "vVDecWriteVLD(u4VDecID, %d, %d)\n", RW_VLD_MCPICSZ >> 2, u4WidthMb);
    }
    else
#endif    	
    {
        u4WidthMb = (UINT32)((prSeqHdr->u4LastHSize + 15) >> 4);
        #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)    //PANDA
        vVDecWriteMC(u4VDecID, RW_MC_PIC_W_MB, u4WidthMb);  
        #else
        vVDecWriteVLD(u4VDecID, RW_VLD_MCPICSZ, u4WidthMb);
        #endif
    }

    // Set frame width and height
    vVDecWriteMC(u4VDecID, RW_MC_UMV_PIC_WIDTH, prSeqHdr->u4LastHSize);
    vVDecWriteMC(u4VDecID, RW_MC_UMV_PIC_HEIGHT, prSeqHdr->u4LastVSize);

    // For debug usage
    vVDecWriteMC(u4VDecID, 0x788, 3);
    vVDecWriteMC(u4VDecID, 0x790, 3);

    // Enable PP
    vVDecWriteMC(u4VDecID, RW_AVS_MC_PP_ENABLE, prVDecAvsDecPrm->fgEnPP);

    // Set the size of luma
    //u4FbWidth = prSeqHdr->u2WidthDec;
    //u4FbHeight = prSeqHdr->u2WidthDec;
    //vVDecWriteMC(u4VDecID, RW_AVS_MC_LUMA_SIZE, (u4FbWidth * (u4FbHeight)));
    vVDecWriteMC(u4VDecID, RW_AVS_MC_LUMA_SIZE, prVDecAvsDecPrm->prCurrFBufInfo->u4CAddrOffset);

    // Disable MC Write
    vVDecWriteMC(u4VDecID, RW_AVS_MC_DISABLE_WRITE, 1);

    // Set Reference Buffer
    if (prPicHdr->u4PicStruct == FRM_PIC)
    {
        if (prPicHdr->u4PicCodingType == I_PIC)
        {
            vVDecWriteMC(u4VDecID, RW_AVS_MC_P_LIST0_PIC0_Y_ADDR, 
                PHYSICAL(prVDecAvsDecPrm->arFBufInfo[AVS_LAST_P_IDX].u4YAddr) );
        }
        else if (prPicHdr->u4PicCodingType == P_PIC)
        {                      
            vVDecWriteMC(u4VDecID, RW_AVS_MC_P_LIST0_PIC0_Y_ADDR, 
                PHYSICAL(prVDecAvsDecPrm->arFBufInfo[AVS_LAST_P_IDX].u4YAddr) );

            vVDecWriteMC(u4VDecID, RW_AVS_MC_P_LIST0_PIC1_Y_ADDR, 
                PHYSICAL(prVDecAvsDecPrm->arFBufInfo[AVS_PREV_P_IDX].u4YAddr));
        }
        else if (prPicHdr->u4PicCodingType == B_PIC)
        {
            vVDecWriteMC(u4VDecID, RW_AVS_MC_B_LIST0_PIC0_Y_ADDR, 
                PHYSICAL(prVDecAvsDecPrm->arFBufInfo[AVS_FW_REF_IDX].u4YAddr));
                        
            vVDecWriteMC(u4VDecID, RW_AVS_MC_B_LIST1_PIC0_Y_ADDR, 
                PHYSICAL(prVDecAvsDecPrm->arFBufInfo[AVS_BW_REF_IDX].u4YAddr));
        }
    }
    else
    {
        if ((prPicHdr->u4PicCodingType == I_PIC) && (prPicHdr->fgSecField))
        {
            vVDecWriteMC(u4VDecID, RW_AVS_MC_P_LIST0_PIC0_Y_ADDR, PHYSICAL((UINT32)prVDecAvsDecPrm->pucDecWorkBufY));
            vVDecWriteMC(u4VDecID, RW_AVS_MC_P_LIST0_PIC1_Y_ADDR, 
                PHYSICAL(prVDecAvsDecPrm->arFBufInfo[AVS_LAST_P_IDX].u4YAddr));
            vVDecWriteMC(u4VDecID, RW_AVS_MC_P_LIST0_PIC2_Y_ADDR, 
                PHYSICAL(prVDecAvsDecPrm->arFBufInfo[AVS_LAST_P_IDX].u4YAddr));

            //if (prVDecAvsDecPrm->u1LastRefIdx >= REF_LIST_1)
            {
                vVDecWriteMC(u4VDecID, RW_AVS_MC_P_LIST0_PIC3_Y_ADDR, 
                    PHYSICAL(prVDecAvsDecPrm->arFBufInfo[AVS_PREV_P_IDX].u4YAddr));
            }
            // bottom Field
            vVDecWriteMC(u4VDecID, RW_AVS_MC_P_FIELD_DESCRIPTION, 0xa);
        }
        else if (prPicHdr->u4PicCodingType == P_PIC)
        {
            if (prPicHdr->u4TFT)
            {
                if (!prPicHdr->fgSecField)
                {
                    vVDecWriteMC(u4VDecID, RW_AVS_MC_P_LIST0_PIC0_Y_ADDR, 
                        PHYSICAL(prVDecAvsDecPrm->arFBufInfo[AVS_LAST_P_IDX].u4YAddr));
                    vVDecWriteMC(u4VDecID, RW_AVS_MC_P_LIST0_PIC1_Y_ADDR, 
                        PHYSICAL(prVDecAvsDecPrm->arFBufInfo[AVS_LAST_P_IDX].u4YAddr));

                    //if (prVDecAvsDecPrm->u1LastRefIdx >= REF_LIST_1)
                    {
                        vVDecWriteMC(u4VDecID, RW_AVS_MC_P_LIST0_PIC2_Y_ADDR, 
                            PHYSICAL(prVDecAvsDecPrm->arFBufInfo[AVS_PREV_P_IDX].u4YAddr));
                        vVDecWriteMC(u4VDecID, RW_AVS_MC_P_LIST0_PIC3_Y_ADDR, 
                            PHYSICAL(prVDecAvsDecPrm->arFBufInfo[AVS_PREV_P_IDX].u4YAddr));
                    }
                    // Top Field
                    vVDecWriteMC(u4VDecID, RW_AVS_MC_P_FIELD_DESCRIPTION, 0x5);
                }
                else
                {
                    vVDecWriteMC(u4VDecID, RW_AVS_MC_P_LIST0_PIC0_Y_ADDR, PHYSICAL((UINT32)prVDecAvsDecPrm->pucDecWorkBufY));
                    vVDecWriteMC(u4VDecID, RW_AVS_MC_P_LIST0_PIC1_Y_ADDR, 
                        PHYSICAL(prVDecAvsDecPrm->arFBufInfo[AVS_LAST_P_IDX].u4YAddr));
                    vVDecWriteMC(u4VDecID, RW_AVS_MC_P_LIST0_PIC2_Y_ADDR, 
                        PHYSICAL(prVDecAvsDecPrm->arFBufInfo[AVS_LAST_P_IDX].u4YAddr));

                    //if (prVDecAvsDecPrm->u1LastRefIdx >= REF_LIST_1)
                    {
                        vVDecWriteMC(u4VDecID, RW_AVS_MC_P_LIST0_PIC3_Y_ADDR, 
                            PHYSICAL(prVDecAvsDecPrm->arFBufInfo[AVS_PREV_P_IDX].u4YAddr));
                    }
                    // bottom Field
                    vVDecWriteMC(u4VDecID, RW_AVS_MC_P_FIELD_DESCRIPTION, 0xa);
                }
            }
            else
            {
            }
        }
        else if (prPicHdr->u4PicCodingType == B_PIC)
        {
            vVDecWriteMC(u4VDecID, RW_AVS_MC_B_LIST0_PIC0_Y_ADDR, 
                PHYSICAL(prVDecAvsDecPrm->arFBufInfo[AVS_FW_REF_IDX].u4YAddr));
            vVDecWriteMC(u4VDecID, RW_AVS_MC_B_LIST0_PIC1_Y_ADDR, 
                PHYSICAL(prVDecAvsDecPrm->arFBufInfo[AVS_FW_REF_IDX].u4YAddr));

            vVDecWriteMC(u4VDecID, RW_AVS_MC_B_LIST1_PIC0_Y_ADDR, 
                PHYSICAL(prVDecAvsDecPrm->arFBufInfo[AVS_BW_REF_IDX].u4YAddr));
            vVDecWriteMC(u4VDecID, RW_AVS_MC_B_LIST1_PIC1_Y_ADDR, 
                PHYSICAL(prVDecAvsDecPrm->arFBufInfo[AVS_BW_REF_IDX].u4YAddr));

            vVDecWriteMC(u4VDecID, 0x568, 1);
            vVDecWriteMC(u4VDecID, 0x56C, 2);
        }
    }

    if (prPicHdr->u4PicCodingType == B_PIC)
    {
        vVDecWriteMC(u4VDecID, RW_AVS_MC_B_REF, 1);
    }
}

static void vVDEC_HAL_AVS_SetPpReg(UINT32 u4VDecID, const VDEC_INFO_AVS_DEC_PRM_T* prVDecAvsDecPrm)
{
    UINT32 u4Height;
    UINT32 u4WidthMB, u4HeightMB;
    VDEC_INFO_AVS_SEQ_HDR_T *prSeqHdr;
    VDEC_INFO_AVS_PIC_HDR_T* prPicHdr;    
    prSeqHdr = prVDecAvsDecPrm->prSeqHdr;
    prPicHdr = prVDecAvsDecPrm->prPicHdr;

    // Set AVS mode for PP
    vVDecWriteAVSPP(u4VDecID, RW_AVS_PP_AVS_MODE, 0x100);

    // Set PP buffer
    vVDecWriteMC(u4VDecID, RW_AVS_PP_LUMA_ADDR, PHYSICAL((UINT32)prVDecAvsDecPrm->pucDecWorkBufY) >> 9);
    vVDecWriteMC(u4VDecID, RW_AVS_PP_CHROM_ADDR, PHYSICAL((UINT32)prVDecAvsDecPrm->pucDecWorkBufC) >> 8);    

    vVDecWriteMC(u4VDecID, 0x250, 1);
    // PP Write Enable.
    vVDecWriteMC(u4VDecID, 0x220, 1);

    if (!prPicHdr->u4LoopFilterDisable)
    {
        vVDecWriteMC(u4VDecID, 0x238, 3);
    }
    else
    {
        vVDecWriteMC(u4VDecID, 0x238, 0);
    }

    if (!prPicHdr->u4ProgFrm)
    {
        u4Height = prSeqHdr->u4LastVSize/2;
    }
    else
    {
        u4Height = prSeqHdr->u4LastVSize;
    }

    u4WidthMB = ((prSeqHdr->u4LastHSize + 15)/16) - 1;
    u4HeightMB = ((u4Height + 15)/16) - 1;
        
    vVDecWriteMC(u4VDecID, 0x260, u4WidthMB);
    vVDecWriteMC(u4VDecID, 0x264, u4HeightMB);
    u4WidthMB += 1;

#if 0
    if (prVDecAvsDecPrm->u4AddrMode == ADDR_MODE_5351_NORMAL)
    {
        u4WidthMB = ((prVDecAvsDecPrm->u4LineSize + 15)/16) - 1;
        u4WidthMB = ((u4WidthMB + 3) / 4) * 4;
        vVDecWriteMC(u4VDecID, 0x22C, u4WidthMB);
    }
    else
#endif    	
    {
        vVDecWriteMC(u4VDecID, 0x22C, u4WidthMB);
    }
}




INT32 i4VDEC_HAL_AVS_DecStart(UINT32 u4BsID, UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm)
{
    UINT32 u4Temp;
    UINT32 u4PicStruct;
    UINT32 u4Php = 0;
    UINT32 u4MbWidthMinusOne;
    UINT32 u4MbHeightMinusOne;
    //UINT32 u4WorkingSize, u4WorkingAddr;
    UINT32 u4PicCodingType; 

#if (CONFIG_DRV_VERIFY_SUPPORT) && (!VDEC_DRV_PARSER)
    VDEC_INFO_AVS_DEC_PRM_T* prVDecAvsDecPrm = (VDEC_INFO_AVS_DEC_PRM_T *) &(prDecPrm->SpecDecPrm.rVDecAVSDecPrm);
#else
    VDEC_INFO_AVS_DEC_PRM_T* prVDecAvsDecPrm = (VDEC_INFO_AVS_DEC_PRM_T *)prDecPrm->prVDecCodecHalPrm;
#endif
    VDEC_INFO_AVS_SEQ_HDR_T *prSeqHdr = prVDecAvsDecPrm->prSeqHdr;
    VDEC_INFO_AVS_PIC_HDR_T *prPicHdr = prVDecAvsDecPrm->prPicHdr;
    //UINT32 u4Rptr, u4Wptr;

   //VDEC_DISP_PIC_INFO_T rDispInfo = {0};   

    u4PicCodingType = prPicHdr->u4PicCodingType; 
    
    u4MbWidthMinusOne = ((prSeqHdr->u4LastHSize + 15) >> 4) - 1;

    if (prPicHdr->u4PicStruct != FRM_PIC)
    {
        u4MbHeightMinusOne = ((prSeqHdr->u4LastVSize + 15) >> 5) - 1;
    }
    else
    {
        u4MbHeightMinusOne = ((prSeqHdr->u4LastVSize + 15) >> 4) - 1;
    }

#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560) && CONFIG_DRV_FTS_SUPPORT)
    // set letterbox detection parameter
    vVDECSetLetetrBoxDetPrm(u4VDecID, &prDecPrm->rLBDPrm);
#endif    

    // Write Addressing Mode

    #if VDEC_FIELD_COMPACT
    printk( "[AVS] Enable Field Compact Mode\n");
    vVDecWriteAVSPP(u4VDecID, 0x3C, (u4VDecReadAVSPP(u4VDecID, 0x3C)& 0xEFFFFFFF));
    vVDecWriteMC(u4VDecID, 0x920, (u4VDecReadMC(u4VDecID, 0x920)  & 0xFEFFFFFF));
    #else
    vVDecWriteMC(u4VDecID, RW_MC_ADDRSWAP, prDecPrm->ucAddrSwapMode);
    vVDecWriteMC(u4VDecID, RW_MC_NBM_CTRL,
             ((u4VDecReadMC(u4VDecID, RW_MC_NBM_CTRL)  & 0xFFFFFFF8) |prDecPrm->ucAddrSwapMode));
    #endif


    u4Temp = u4VDecReadMC(u4VDecID, 0x5E4);
    u4Temp |= (0x1000);
    vVDecWriteMC(u4VDecID, 0x5E4, u4Temp);

    //Setting Wrapper buffer for 8320
    vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER,PHYSICAL(prVDecAvsDecPrm->u4VLDWrapperWrok));
    vVDecWriteMC(u4VDecID, RW_MC_PP_WRAPPER,PHYSICAL(prVDecAvsDecPrm->u4PPWrapperWrok));

    // Set some VLD registers
    #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    vVDecWriteAVSVLD(u4VDecID, RW_AVS_VLD_CTRL, 0x7); 
    //vVDecWriteVLDTOP(u4VDecID, RW_VLD_TOP_ERR_CONCEAL, (0x10 << 28));  
    #else
    vVDecWriteAVSVLD(u4VDecID, RW_AVS_VLD_CTRL, 0x8007); 
    #endif

    #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    vVDecWriteVLDTOP(u4VDecID, RW_VLD_TOP_TOTAL_MBS_IN_PIC, (u4MbWidthMinusOne + 1) * (u4MbHeightMinusOne + 1));  
    vVDecWriteVLDTOP(u4VDecID, RW_VLD_TOP_PIC_MB_SIZE_M1, ((u4MbHeightMinusOne << 16) | u4MbWidthMinusOne)); 
    #else
    vVDecWriteAVSVLD(u4VDecID, RW_AVS_VLD_PIC_SIZE, ((u4MbHeightMinusOne << 8) | u4MbWidthMinusOne)); 
    #endif

    // pic type/pic struct/pic reference/skip mode flag/fix pic qp
    if (prPicHdr->u4PicStruct == FRM_PIC)
    {
        u4PicStruct = FRM_PIC;
    }
    else 
    {
        if (prPicHdr->u4TFT)
        {
            if (!prPicHdr->fgSecField)
            {
                u4PicStruct = TOP_FLD_PIC; // top field
            }
            else
            {
                u4PicStruct = BTM_FLD_PIC; //bottom field
            }
        }
        else
        {
            if (!prPicHdr->fgSecField)
            {
                u4PicStruct = BTM_FLD_PIC; // bottom field
            }
            else
            {
                u4PicStruct = TOP_FLD_PIC; //top field

                // bottom field reconstructs from top field
                if (prPicHdr->u4PicCodingType == I_PIC)
                {
                    u4PicCodingType = P_PIC;
                }
            }
        }
    }

    u4Php = ((u4PicCodingType & 3) |
                 ((u4PicStruct & 3) << 2) |
                 ((prPicHdr->u4PicRefFg & 1) << 4) |
                 ((prPicHdr->u4SkipModeFg & 1) << 5) |
                 ((prPicHdr->u4FixedPicQP & 1) << 6) |
                 ((prPicHdr->u4PicQP & 0x3f) << 8) |
                 ((prPicHdr->i4AlphaCOffset & 0x1f) << 16) |
                 ((prPicHdr->i4BetaOffset & 0x1f) << 24));
    vVDecWriteAVSVLD(u4VDecID, RW_AVS_VLD_PHP, u4Php); 

    #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    vVDecWriteVLDTOP(u4VDecID, RW_VLD_TOP_ERR_CONCEAL, AVS_ERR_CONCEALMENT_ENABLE);  
    #else
    vVDecWriteAVSVLD(u4VDecID, RW_AVS_VLD_ERR_CONCEALMENT, AVS_ERR_CONCEALMENT_ENABLE);
    #endif


    #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    vVDecWriteVLDTOP(u4VDecID, RW_VLD_TOP_PRED_SRAM_CFG, 0);    //PANDA
    #if (CONFIG_DRV_FPGA_BOARD)
    vVDecWriteVLDTOP(u4VDecID, RW_VLD_TOP_PRED_ADDR, PHYSICAL(prVDecAvsDecPrm->rAvsWorkBufSa.u4PredSa));
    #else
    vVDecWriteVLDTOP(u4VDecID, RW_VLD_TOP_PRED_ADDR, 0); 
    #endif
    #else
    #if (CONFIG_DRV_FPGA_BOARD)
    // Get/Set working buffer for VLD prediction module
    //vVDecWriteAVSVLD(u4VDecID, RW_AVS_VLD_PRED_BASE_ADDR, ( (PHYSICAL(prVDecAvsDecPrm->rAvsWorkBufSa.u4PredSa)) | 0x40000000));
    vVDecWriteAVSVLD(u4VDecID, RW_AVS_VLD_PRED_BASE_ADDR, PHYSICAL(prVDecAvsDecPrm->rAvsWorkBufSa.u4PredSa));
    #else    
    vVDecWriteAVSVLD(u4VDecID, RW_AVS_VLD_PRED_BASE_ADDR, 0);    
    #endif
    #endif

    // Set MV Related Settings
    vVDEC_HAL_AVS_SetMvReg(u4VDecID, prVDecAvsDecPrm);
    
    // Fill Quantization Table
    vVDEC_HAL_AVS_FillQTable2Sram(u4VDecID);

    // Set MC Related Settings
    vVDEC_HAL_AVS_SetMcReg(u4VDecID, prVDecAvsDecPrm);

    // Set PP Related Settings
    vVDEC_HAL_AVS_SetPpReg(u4VDecID, prVDecAvsDecPrm);

    // Trigger Decoder
    vVDecWriteAVSVLD(u4VDecID, RW_AVS_VLD_DECSTART, 1);   
     return 0;
}

// **************************************************************************
// Function : void u4VDEC_HAL_AVS_GetErrMsg(UINT32 u4VDecID);
// Description :Read AVS error message after decoding end
// Parameter : u4VDecID : video decoder hardware ID
// Return      : AVS decode error message
// **************************************************************************
BOOL fgVDEC_HAL_AVS_ChkErrInfo(UINT32 ucBsId, UINT32 u4VDecID, UINT32 u4DecErrInfo, UINT32 u4ECLevel)
{
    BOOL fgIsVDecErr = FALSE;        
    return fgIsVDecErr;
}


void vVDEC_HAL_AVS_GetMbxMby(UINT32 u4VDecID, UINT32 *pu4Mbx, UINT32 *pu4Mby)
{
    //UINT32 u4Tmp;
    VDEC_ASSERT(pu4Mby!=NULL);
    VDEC_ASSERT(pu4Mby!=NULL);

    //u4Tmp = u4VDecReadAVSVLD(u4VDecID, RO_AVLD_SMKR);
    //*pu4Mbx = u4Tmp>>24;
    //*pu4Mby = (u4Tmp<<8)>>24;
    *pu4Mbx = u4VDecReadMC(u4VDecID, RO_MC_MBX);
    *pu4Mby = u4VDecReadMC(u4VDecID, RO_MC_MBY);
}

void vVDEC_HAL_AVS_GetErrInfo(UINT32 u4VDecID, VDEC_INFO_AVS_ERR_INFO_T *prAvsErrInfo)
{
    UINT32 u4RegVal = 0;
    
    u4RegVal = u4VDecReadVLD(u4VDecID, RO_AVS_ERROR_ACC);
    prAvsErrInfo->u4AvsErrCnt = u4RegVal;
    prAvsErrInfo->u4AvsErrRow = 0;
    prAvsErrInfo->u2AvsMBErrCnt =0 ;
    return;
}

// **************************************************************************
// Function : UINT32 u4VDEC_HAL_VP6_GetErrType(UINT32 u4VDecID);
// Description :Read Vp6 error type after decoding end
// Parameter : u4VDecID : video decoder hardware ID
// Return      : Vp6 decode error type value
// **************************************************************************
UINT32 u4VDEC_HAL_AVS_GetErrType(UINT32 u4VDecID, VDEC_INFO_AVS_ERR_INFO_T *prAvsErrInfo)
{
    UINT32 u4RegVal = 0;

    u4RegVal= u4VDecReadAVSVLD(u4VDecID, RO_AVS_ERROR_TYPE);
    prAvsErrInfo->u4AvsErrType = u4RegVal;
    return u4RegVal;
}

#if CONFIG_DRV_VERIFY_SUPPORT
UINT32 u4VDEC_HAL_AVS_VDec_ReadFinishFlag(UINT32 u4VDecID)
{
  //return u4VDecReadAVSVLD(u4VDecID, RO_AVLD_COMPLETE);
  return (u4VDecReadMISC(u4VDecID, 0xA4) & (0x1 << 16));
}

void u4VDEC_HAL_AVS_VDec_ClearInt(UINT32 u4VDecID)
{
    UINT32 u4Reg;
    u4Reg = u4VDecReadMISC(u4VDecID, 0xA4);
    vVDecWriteMISC(u4VDecID, 0xA4, (u4Reg |0x11));
    vVDecWriteMISC(u4VDecID, 0xA4, u4Reg);
}

BOOL fgVDEC_HAL_AVS_VDec_CompCheckSum(UINT32 *pu4DecCheckSum, UINT32 *pu4GoldenCheckSum)
{
  if((*pu4GoldenCheckSum) != (*pu4DecCheckSum))
  {
    return (FALSE);
  }
  pu4GoldenCheckSum ++;
  pu4DecCheckSum ++;
  if((*pu4GoldenCheckSum) != (*pu4DecCheckSum))
  {
    return (FALSE);
  }
  pu4GoldenCheckSum ++;
  pu4DecCheckSum ++;
  if((*pu4GoldenCheckSum) != (*pu4DecCheckSum))
  {
    return (FALSE);
  }
  pu4GoldenCheckSum ++;
  pu4DecCheckSum ++;
  if((*pu4GoldenCheckSum) != (*pu4DecCheckSum))
  {
    return (FALSE);
  }
  pu4GoldenCheckSum ++;
  pu4DecCheckSum ++;
  return (TRUE);
}

#endif

