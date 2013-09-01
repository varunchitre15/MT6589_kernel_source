
#include "vdec_hw_common.h"
#include "vdec_hal_if_mpeg.h"
#include "vdec_hw_mpeg.h"
#include "vdec_hal_errcode.h"

//#include "x_hal_1176.h"
//#include "x_hal_ic.h"

#if CONFIG_DRV_VERIFY_SUPPORT
#include "../verify/vdec_verify_general.h"
#include "../verify/vdec_verify_mpv_prov.h"
#include <linux/string.h>

#if (!CONFIG_DRV_LINUX)
#include <stdio.h>
#include <string.h>
#endif

extern BOOL fgWrMsg2PC(void* pvAddr, UINT32 u4Size, UINT32 u4Mode, VDEC_INFO_VERIFY_FILE_INFO_T *pFILE_INFO);
extern void vVDecOutputDebugString(const CHAR * format, ...);
#endif

extern void vVDecWriteAVCVLD(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);


// **************************************************************************
// Function : INT32 i4VDEC_HAL_MPEG_InitVDecHW(UINT32 u4VDecID, VDEC_INFO_MPEG_VFIFO_PRM_T *prMpegVFifoPrm);
// Description :Initialize video decoder hardware only for MPEG
// Parameter : u4VDecID : video decoder hardware ID
//                  prMpegVFifoPrm : pointer to VFIFO info struct
// Return      : =0: success.
//                  <0: fail.
// **************************************************************************
INT32 i4VDEC_HAL_MPEG_InitVDecHW(UINT32 u4VDecID, VDEC_INFO_MPEG_VFIFO_PRM_T *prMpegVFifoPrm)
{
#if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8555)
    vVDecResetHW(u4VDecID);
#elif (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560)
    vVDecResetHW(u4VDecID, prMpegVFifoPrm->u4CodeType);
#else
    vVDecResetHW(u4VDecID, VDEC_UNKNOWN);
#endif
    vVDecSetVLDVFIFO(0, u4VDecID, PHYSICAL(prMpegVFifoPrm->u4VFifoSa), PHYSICAL(prMpegVFifoPrm->u4VFifoEa));

    // for MT8320
    if((prMpegVFifoPrm->u4CodeType == VDEC_MPEG) || (prMpegVFifoPrm->u4CodeType == VDEC_MPEG1) ||(prMpegVFifoPrm->u4CodeType == VDEC_MPEG2) )
    {
      vVDecWriteVLD(u4VDecID, 39 * 4, 0x30307);
      vVDecWriteVLD(u4VDecID, 59 * 4, (u4VDecReadVLD(u4VDecID, 59 * 4) | (0x1 << 28)));
    }
    return HAL_HANDLE_OK;
}


// **************************************************************************
// Function : UINT32 u4VDEC_HAL_MPEG_ShiftGetBitStream(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits);
// Description :Read barrel shifter after shifting
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
//                 u4ShiftBits : shift bits number
// Return      : Value of barrel shifter input window after shifting
// **************************************************************************
UINT32 u4VDEC_HAL_MPEG_ShiftGetBitStream(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits)
{
    UINT32 u4RegVal;

    u4RegVal = u4VDecVLDGetBitS(u4BSID, u4VDecID, u4ShiftBits);
    
    return( u4RegVal);
}


// **************************************************************************
// Function : UINT32 u4VDEC_HAL_MPEG_GetBitStreamShift(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits);
// Description :Read Barrel Shifter before shifting
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
//                 u4ShiftBits : shift bits number
// Return      : Value of barrel shifter input window before shifting
// **************************************************************************
UINT32 u4VDEC_HAL_MPEG_GetBitStreamShift(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits)
{
    UINT32 u4RegVal0;

    u4RegVal0 = u4VDecVLDGetBitS(u4BSID, u4VDecID, 0);
    u4VDecVLDGetBitS(u4BSID, u4VDecID, u4ShiftBits);
    
    return (u4RegVal0);
}


// **************************************************************************
// Function : INT32 i4VDEC_HAL_MPEG_InitBarrelShifter(UINT32 u4BSID, UINT32 u4VDecID, VDEC_INFO_MPEG_BS_INIT_PRM_T *prMpegBSInitPrm);
// Description :Initialize barrel shifter with byte alignment
// Parameter : u4ReadPointer : set read pointer value
//                 u4WrtePointer : set write pointer value
//                 u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
// Return      : =0: success.
//                  <0: fail.
// **************************************************************************
INT32 i4VDEC_HAL_MPEG_InitBarrelShifter(UINT32 u4BSID, UINT32 u4VDecID, VDEC_INFO_MPEG_BS_INIT_PRM_T *prMpegBSInitPrm)
{
    BOOL fgFetchOK = FALSE;
    #if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8580)
    INT32 i;
    #endif
    INT32 j;
    UINT32 u4VLDRemainByte;

#if (CONFIG_DRV_LINUX_DATA_CONSISTENCY)
    //HalFlushInvalidateDCache();
#endif
  #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580) 
	//polling
   #ifdef VDEC_SIM_DUMP
   printk("if(`VDEC_PROCESS_FLAG == 1) wait(`VDEC_BITS_PROC_NOP == 1);\n");
   #endif
   if((u4VDecReadVLD(u4VDecID,RO_VLD_SRAMCTRL) & (PROCESS_FLAG)))
       while (!(u4VDecReadVLD(u4VDecID,RO_VLD_SRAMCTRL) & 1));

   vVDecSetVLDVFIFO(u4BSID, u4VDecID, PHYSICAL((UINT32) prMpegBSInitPrm->u4VFifoSa), PHYSICAL((UINT32) prMpegBSInitPrm->u4VFifoEa));

   u4VLDRemainByte =  ((prMpegBSInitPrm->u4ReadPointer)) & 0xf;


   vVDecWriteVLD(u4VDecID, RW_VLD_RPTR + (u4BSID << 10), PHYSICAL((UINT32) prMpegBSInitPrm->u4ReadPointer));
   vVDecWriteVLD(u4VDecID, WO_VLD_WPTR + (u4BSID << 10),PHYSICAL((UINT32) prMpegBSInitPrm->u4WritePointer));
   //     vVDecWriteVLD(u4VDecID, RW_VLD_ASYNC + (u4BSID << 10), u4VDecReadVLD(u4VDecID, RW_VLD_ASYNC) | VLD_WR_ENABLE);//mark

   //Reset async fifo for mt8580 new design
   vVDecWriteVLD(u4VDecID, WO_VLD_SRST , (0x1 << 8));
   vVDecWriteVLD(u4VDecID, WO_VLD_SRST , 0);

   //      vVDecWriteVLD(u4VDecID, 0xCC , 0x00); //Just for debug
#if (CONFIG_DRV_FPGA_BOARD || CONFIG_DRV_VERIFY_SUPPORT)
   if (_u4CodecVer[u4VDecID] == VDEC_MPEG4 || _u4CodecVer[u4VDecID] == VDEC_H263 || _u4CodecVer[u4VDecID] == VDEC_DIVX3)
   {
       vVDecWriteVLD(u4VDecID,RW_VLD_LDSH,0x100);
   }
#endif
   // start to fetch data
   vVDecWriteVLD(u4VDecID, RW_VLD_PROC + (u4BSID << 10), VLD_INIFET);

   if (fgVDecWaitVldFetchOk(u4BSID, u4VDecID))
   {
       fgFetchOK = TRUE;
   }
#else
    vVDecWriteVLD(u4VDecID,RW_VLD_RDY_SWTICH + (u4BSID << 10), READY_TO_RISC_1);

    vVDecSetVLDVFIFO(u4BSID, u4VDecID, PHYSICAL(prMpegBSInitPrm->u4VFifoSa), PHYSICAL(prMpegBSInitPrm->u4VFifoEa));
 
    u4VLDRemainByte =  (PHYSICAL(prMpegBSInitPrm->u4ReadPointer)) & 0xf;
    
    // prevent initialize barrel fail
    for (i = 0; i < 5; i++)
    {
        vVDecWriteVLD(u4VDecID, WO_VLD_WPTR, u4VDecReadVLD(u4VDecID, WO_VLD_WPTR) | VLD_CLEAR_PROCESS_EN);
        vVDecWriteVLD(u4VDecID, RW_VLD_RPTR + (u4BSID << 10), PHYSICAL(prMpegBSInitPrm->u4ReadPointer));
        vVDecWriteVLD(u4VDecID, RW_VLD_RPTR + (u4BSID << 10), PHYSICAL(prMpegBSInitPrm->u4ReadPointer));       
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
        vVDecWriteVLD(u4VDecID, WO_VLD_WPTR + (u4BSID << 10), PHYSICAL(prMpegBSInitPrm->u4WritePointer));
        vVDecWriteVLD(u4VDecID, RW_VLD_ASYNC + (u4BSID << 10), u4VDecReadVLD(u4VDecID, RW_VLD_ASYNC) | VLD_WR_ENABLE);
#else
        vVDecWriteVLD(u4VDecID, WO_VLD_WPTR + (u4BSID << 10),((PHYSICAL(prMpegBSInitPrm->u4WritePointer) << 4) | 0x2));
#endif
        // start to fetch data
        vVDecWriteVLD(u4VDecID, RW_VLD_PROC + (u4BSID << 10), VLD_INIFET);
    
        if (fgVDecWaitVldFetchOk(u4BSID, u4VDecID))
        {
            fgFetchOK = TRUE;
            break;
        }
    }
#endif
    if (!fgFetchOK)
    {
        return(INIT_BARRELSHIFTER_FAIL);
    }
  
    vVDecWriteVLD(u4VDecID, RW_VLD_PROC + (u4BSID << 10), VLD_INIBR);
  
    for (j=0;j<u4VLDRemainByte;j++)
    {
        u4VDecVLDGetBitS(u4BSID, u4VDecID, 8);
    }
  
    //u4VLDByte = u4VDecReadVldRPtr(u4BSID, u4VDecID, &u4VLDBit, PHYSICAL(prMpegBSInitPrm->u4VFifoSa));
  
    return HAL_HANDLE_OK;
}

// *********************************************************************
// Function : void vVLDVdec2Barl(UINT32 u4VDecID)
// Description : Load sum from VDec HW to barrel shifter
// Parameter : None
// Return    : None
// *********************************************************************
void vVDEC_HAL_MPEG_VLDVdec2Barl(UINT32 u4VDecID)
{
    vVDecWriteVLD(u4VDecID, RW_VLD_LDSH, 1);
    vVDecWriteVLD(u4VDecID, RW_VLD_LDSH, 0);
}


// **************************************************************************
// Function : UINT32 u4VDEC_HAL_MPEG_ReadRdPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4VFIFOSa, UINT32 *pu4Bits);
// Description :Read current read pointer
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
//                 u4VFIFOSa : video FIFO start address
//                 pu4Bits : read pointer value with remained bits
// Return      : Current read pointer with byte alignment
// **************************************************************************
UINT32 u4VDEC_HAL_MPEG_ReadRdPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4VFIFOSa, UINT32 *pu4Bits)
{
#if (CONFIG_DRV_LINUX_DATA_CONSISTENCY)
    //return (UINT32)(VIRTUAL((u4VDecReadVldRPtr(u4BSID, u4VDecID, pu4Bits, PHYSICAL(u4VFIFOSa)))));
    return (UINT32)(((u4VDecReadVldRPtr(u4BSID, u4VDecID, pu4Bits, PHYSICAL(u4VFIFOSa)))));
#else
    return u4VDecReadVldRPtr(u4BSID, u4VDecID, pu4Bits, PHYSICAL(u4VFIFOSa));
#endif
}


// **************************************************************************
// Function : void v4VDEC_HAL_MPEG_AlignRdPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4VFIFOSa, UINT32 u4AlignType);
// Description :Align read pointer to byte,word or double word
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
//                 u4VFIFOSa : video FIFO start address
//                 u4AlignType : read pointer align type
// Return      : None
// **************************************************************************
#if (CONFIG_DRV_ONLY || CONFIG_DRV_VERIFY_SUPPORT)
void vVDEC_HAL_MPEG_AlignRdPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4VFIFOSa, UINT32 u4AlignType)
{
    INT32 i;
    UINT32 u4VLDByte,u4VLDbits;   
    
    u4VLDByte= u4VDecReadVldRPtr(u4BSID, u4VDecID, &u4VLDbits, PHYSICAL(u4VFIFOSa));

    if (u4VLDbits != 0)
    {
        u4VDecVLDGetBitS(u4BSID, u4VDecID, 8 - u4VLDbits);
        u4VLDByte++;
    }
    if (u4AlignType == WORD_ALIGN)
    {
        for (i=0;i<(u4VLDByte & 1);i++)
        {
            u4VDecVLDGetBitS(u4BSID, u4VDecID, 8);
        }
    }
    else if (u4AlignType == DWRD_ALIGN)
    {
        if ((u4VLDByte & 3) != 0)
        {
            for (i=0;i<(4- (u4VLDByte & 3));i++)
            {
                u4VDecVLDGetBitS(u4BSID, u4VDecID, 8);
            }
        }
    }
    return;

}
#endif

#if VDEC_REMOVE_UNUSED_FUNC
// **************************************************************************
// Function : UINT32 u4VDEC_HAL_MPEG_GetBitcount(UINT32 u4BSID, UINT32 u4VDecID);
// Description :Read barrel shifter bitcount after initializing 
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
// Return      : Bitcount counted by HAL
// **************************************************************************
UINT32 u4VDEC_HAL_MPEG_GetBitcount(UINT32 u4BSID, UINT32 u4VDecID)
{
    return HAL_HANDLE_OK;
}
#endif

//#ifndef FW_WRITE_QUANTIZATION_MATRIX
// **************************************************************************
// Function : void vVDEC_HAL_MPEG12_LoadQuantMatrix(UINT32 u4VDecID, BOOL fgIntra, VDEC_INFO_MPEG_QANTMATRIX_T *prQuantMatrix);
// Description :Set video decoder hardware registers to load quantization, only for MPEG12
// Parameter : u4VDecID : video decoder hardware ID
//                 fgIntra : flag of intra block or not
//                 prQuantMatrix : pointer to MPEG12 quantization matrix
// Return      : None
// **************************************************************************
#if (CONFIG_DRV_ONLY || CONFIG_DRV_VERIFY_SUPPORT)
void vVDEC_HAL_MPEG12_LoadQuantMatrix(UINT32 u4VDecID, BOOL fgIntra, VDEC_INFO_MPEG_QANTMATRIX_T *prQuantMatrix)
{
    vVLDLoadQuanMat(u4VDecID, fgIntra);
}
#endif

// **************************************************************************
// Function : BOOL fgVDEC_HAL_MPEG4_LoadQuantMatrix(UINT32 u4VDecID, BOOL fgIntra, UINT32 *pu4QuanMatrixLen, 
//        VDEC_INFO_MPEG_BS_INIT_PRM_T *prMpegBSInitPrm, VDEC_INFO_MPEG_QANTMATRIX_T *prQuantMatrix);
// Description :Set video decoder hardware registers to load quantization, only for MPEG4
// Parameter : u4VDecID : video decoder hardware ID
//                 fgIntra : flag of intra block or not
//                 pu4QuanMatrixLen : pointer of MPEG4 quantization matrix length
//                 prQuantMatrix : pointer to MPEG4 quantization matrix
// Return      : None
// **************************************************************************
#if (CONFIG_DRV_ONLY || CONFIG_DRV_VERIFY_SUPPORT)
BOOL fgVDEC_HAL_MPEG4_LoadQuantMatrix(UINT32 u4VDecID, BOOL fgIntra, UINT32 *pu4QuanMatrixLen, 
        VDEC_INFO_MPEG_BS_INIT_PRM_T *prMpegBSInitPrm, VDEC_INFO_MPEG_QANTMATRIX_T *prQuantMatrix)
{
    UINT32 u4OldByte, u4Bit;
    UINT32 dQ;
    UINT32 i;

    u4OldByte = u4VDecReadVldRPtr(0, u4VDecID, &u4Bit, PHYSICAL(prMpegBSInitPrm->u4VFifoSa));
    vVLDLoadQuanMat(u4VDecID, fgIntra);
  
    /* 040329: Workaround for HW bug:
               If quantization matrix is smaller than 64 bytes,
               HW may load more than necessary bytes,
               causing successive vparser reading to be wrong.
               To cover it, after loading a matrix,
               set vld rd ptr back to the matrix,
               and shift the correct byte number by FW.
               One issue is not covered yet:
               The remaining values should be equal to the value before the 0,
               but if HW loads the values after the 0, it will use those values.
               To really solve this issue we have to prepare a 64-byte buffer with
               correct trailing values for HW to load. */
    prMpegBSInitPrm->u4ReadPointer = u4OldByte + prMpegBSInitPrm->u4VFifoSa;
    prMpegBSInitPrm->u4WritePointer =  u4VDecReadVLD(u4VDecID, RO_VLD_VWPTR);
    if (i4VDEC_HAL_MPEG_InitBarrelShifter(0, u4VDecID, prMpegBSInitPrm) < 0)
    {
        return (FALSE);
    }
    u4VDecVLDGetBitS(0, u4VDecID, u4Bit);
  
    dQ = u4VDecVLDGetBitS(0, u4VDecID, 0);
    for (i = 0; i < 64; i++)
    {
        if (BYTE3(dQ) == 0)
        {
            u4VDecVLDGetBitS(0, u4VDecID, 8);
            break;
        }
        dQ = u4VDecVLDGetBitS(0, u4VDecID, 8);
    }
  
    return (TRUE);
}
#endif

//#else

// **************************************************************************
// Function : void vVDEC_HAL_MPEG12_FW_LoadQuantMatrix(UINT32 u4VDecID, BOOL fgIntra, UINT32 *pu4QuantMatrix);
// Description :load quantization, only for MPEG12
// Parameter : u4VDecID : video decoder hardware ID
//                 fgIntra : flag of intra block or not
//                 prQuantMatrix : pointer to MPEG12 quantization matrix
// Return      : None
// **************************************************************************
void vVDEC_HAL_MPEG12_FW_LoadQuantMatrix(UINT32 u4VDecID, BOOL fgIntra, UINT32 *pu4QuantMatrix)
{
    INT32 i;
    UINT32 u4Temp, u4Idx;

    u4Idx = fgIntra ? 0 : 64;
    
    for (i=0; i < 16; i++)
    {
        // add 16 for every list
        u4Temp = pu4QuantMatrix[i];
        vVDecWriteVLD(u4VDecID, RW_VLD_SCL_ADDR, 0x200 + u4Idx + (i << 2));
        vVDecWriteVLD(u4VDecID, RW_VLD_SCL_DATA, INVERSE_ENDIAN(u4Temp));
        vVDecWriteVLD(u4VDecID, RW_VLD_SCL_ADDR, 0x100 + u4Idx + (i << 2));
    }
    
    if (fgIntra)
    {
        vVDecWriteVLD(u4VDecID, WO_VLD_FDEC, VLD_RELOAD_INTRA);
    }
    else
    {
        vVDecWriteVLD(u4VDecID, WO_VLD_FDEC, VLD_RELOAD_NONINTRA);
    }
}


// **************************************************************************
// Function : void vVDEC_HAL_MPEG4_FW_LoadQuantMatrix(UINT32 u4VDecID, BOOL fgIntra, UINT32 *pu4QuanMatrixLen, 
//        UINT32 *pu4QuantMatrix);
// Description :load quantization, only for MPEG12
// Parameter : u4VDecID : video decoder hardware ID
//                 fgIntra : flag of intra block or not
//                 prQuantMatrix : pointer to MPEG12 quantization matrix
// Return      : None
// **************************************************************************
void vVDEC_HAL_MPEG4_FW_LoadQuantMatrix(UINT32 u4VDecID, BOOL fgIntra, UINT32 *pu4QuanMatrixLen, 
        UINT32 *pu4QuantMatrix)
{
    INT32 i;
    UINT32 u4Temp, u4Idx;
    //printk("[MPEG4] vVDEC_HAL_MPEG4_FW_LoadQuantMatrix\n");
    u4Idx = fgIntra ? 0 : 64;
    
    for (i=0; i < (((*pu4QuanMatrixLen) + 3) >> 2); i++)
    {
        // add 16 for every list
        u4Temp = pu4QuantMatrix[i];
        vVDecWriteVLD(u4VDecID, RW_VLD_SCL_ADDR, 0x200 + u4Idx + (i << 2));
        vVDecWriteVLD(u4VDecID, RW_VLD_SCL_DATA, INVERSE_ENDIAN(u4Temp));
        vVDecWriteVLD(u4VDecID, RW_VLD_SCL_ADDR, 0x100 + u4Idx + (i << 2));
    }
    if (fgIntra)
    {
        vVDecWriteVLD(u4VDecID, WO_VLD_FDEC, VLD_RELOAD_INTRA);
    }
    else
    {
        vVDecWriteVLD(u4VDecID, WO_VLD_FDEC, VLD_RELOAD_NONINTRA);
    }
}
//#endif


// **************************************************************************
// Function : void v4DEC_HAL_MPEG_ReLoadQuantMatrix(UINT32 u4VDecID, BOOL fgIntra);
// Description :Set video decoder hardware registers to load quantization
// Parameter : u4VDecID : video decoder hardware ID
//                 fgIntra : pointer to quantization matrix
// Return      : None
// **************************************************************************
void vVDEC_HAL_MPEG_ReLoadQuantMatrix(UINT32 u4VDecID, BOOL fgIntra)
{
    if (fgIntra)
    {
        vVDecWriteVLD(u4VDecID, WO_VLD_FDEC, VLD_RELOAD_INTRA);
    }
    else
    {
        vVDecWriteVLD(u4VDecID, WO_VLD_FDEC, VLD_RELOAD_NONINTRA);
    }
}


// **************************************************************************
// Function : void vVDEC_HAL_MPEG_ResetQuantMatrix(UINT32 u4VDecID);
// Description :Set video decoder hardware registers to load quantization
// Parameter : u4VDecID : video decoder hardware ID
// Return      : None
// **************************************************************************
void vVDEC_HAL_MPEG_ResetQuantMatrix(UINT32 u4VDecID)
{
    vVDecWriteVLD(u4VDecID, WO_VLD_FDEC, (u4VDecReadVLD(u4VDecID, WO_VLD_FDEC) & VLD_PIC_COMPLETE) | VLD_RST_QMATRIX);
}

#if VDEC_REMOVE_UNUSED_FUNC
// **************************************************************************
// Function : void vVDEC_HAL_MPEG_ReStoreQuantMatrix(UINT32 u4VDecID, BOOL fgIntra, VDEC_INFO_MPEG_QANTMATRIX_T *prQuantMatrix);
// Description :Write video decoder hardware quantization matrix and load it
// Parameter : u4VDecID : video decoder hardware ID
//                 fgIntra : flag of intra block or not
//                 prQuantMatrix : pointer to quantization matrix
// Return      : None
// **************************************************************************
void vVDEC_HAL_MPEG_ReStoreQuantMatrix(UINT32 u4VDecID, BOOL fgIntra, VDEC_INFO_MPEG_QANTMATRIX_T *prQuantMatrix)
{
    return;
}
#endif

// **************************************************************************
// Function : void vVDEC_HAL_MPEG_SetMPEG4Flag(UINT32 u4VDecID, BOOL fgMp4);
// Description :Set Mpeg4 register flag
// Parameter : u4VDecID : video decoder hardware ID
//                 fgMp4 : flag of mpeg4 or not
// Return      : None
// **************************************************************************
#if (CONFIG_DRV_ONLY || CONFIG_DRV_VERIFY_SUPPORT)
void vVDEC_HAL_MPEG_SetMPEG4Flag(UINT32 u4VDecID, BOOL fgMp4)
{
    UINT32 u4RegVal;
    if (fgMp4)
    {
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8555)
	if(BSP_GetIcVersion() == IC_8555)
      {
        if(u4VDecID == 1)
        {
            vWriteReg(0x32000,(u4ReadReg(0x32000) | 0x1));
        }
        else
        {
            vWriteReg(0x2B000,(u4ReadReg(0x2B000) | 0x1));
        }
      }
#endif
        vVDecWriteVLD(u4VDecID, RW_VLD_MP4_FLG, 1);
        //6589NEW 2.1
#if (MPEG4_6589_SUPPORT)
        u4RegVal = u4VDecReadVLD(u4VDecID, RW_VLD_MP4_FLG) & ~(0x1 << 9);
        u4RegVal |= (0x1 << 16) | 
                    ((_u4CodecVer[u4VDecID] == VDEC_DIVX3) ? (0x1 << 9) : (0x0 << 9));
        vVDecWriteVLD(u4VDecID, RW_VLD_MP4_FLG, u4RegVal);
#endif

#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8555)
	if(BSP_GetIcVersion() == IC_8555)
	{
        if(u4VDecID == 1)
        {
            vWriteReg(0x32000,(u4ReadReg(0x32000) & (~0x1)));
        }
        else
        {
            vWriteReg(0x2B000,(u4ReadReg(0x2B000) & (~0x1)));
        }
       }
#endif
    }
    else
    {
        vVDecWriteVLD(u4VDecID, RW_VLD_MP4_FLG, 0);
    }
}
#endif

// **************************************************************************
// Function : INT32 i4VDEC_HAL_MPEG12_DecStart(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm);
// Description :Set video decoder hardware registers to decode for mpeg12
// Parameter : u4VDecID : video decoder hardware ID
//                 prDecMpeg12Prm : pointer to mpeg12 decode info struct
// Return      : =0: success.
//                  <0: fail.
// **************************************************************************
INT32 i4VDEC_HAL_MPEG12_DecStart(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm)
{
#if (CONFIG_DRV_LINUX_DATA_CONSISTENCY)
    //HalFlushInvalidateDCache();
#endif
    vVDECSetDownScalerPrm(u4VDecID, &prDecPrm->rDownScalerPrm);
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560) && CONFIG_DRV_FTS_SUPPORT)
    // set letterbox detection parameter
    vVDECSetLetetrBoxDetPrm(u4VDecID, &prDecPrm->rLBDPrm);
#endif    
    vVDecMpegCommSetting(u4VDecID, prDecPrm);
    vVLDDec(u4VDecID, prDecPrm);
    return HAL_HANDLE_OK;
}


// **************************************************************************
// Function : INT32 i4VDEC_HAL_DIVX3_DecStart(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm);
// Description :Set video decoder hardware registers to decode for divx3
// Parameter : u4VDecID : video decoder hardware ID 
//                 prDecDivx3Prm : pointer to divx3 decode info struct
// Return      : =0: success.
//                  <0: fail.
// **************************************************************************
INT32 i4VDEC_HAL_DIVX3_DecStart(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm)
{
#if (CONFIG_DRV_LINUX_DATA_CONSISTENCY)
    //HalFlushInvalidateDCache();
#endif
    vVDECSetDownScalerPrm(u4VDecID, &prDecPrm->rDownScalerPrm);
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560) && CONFIG_DRV_FTS_SUPPORT)
    // set letterbox detection parameter
    vVDECSetLetetrBoxDetPrm(u4VDecID, &prDecPrm->rLBDPrm);
#endif    
    vVDecMpegCommSetting(u4VDecID, prDecPrm);
    vVLDDx3Dec(u4VDecID, prDecPrm);
    return HAL_HANDLE_OK;
}


// **************************************************************************
// Function : INT32 i4VDEC_HAL_MPEG4_DecStart(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm);
// Description :Set video decoder hardware registers to decode for mpeg4
// Parameter : u4VDecID : video decoder hardware ID
//                 prDecMpeg4Prm : pointer to mpeg4 decode info struct
// Return      : =0: success.
//                  <0: fail.
// **************************************************************************
INT32 i4VDEC_HAL_MPEG4_DecStart(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm)
{
#ifdef LETTERBOX_SUPPORT
    vVDecWriteDV(u4VDecID, RW_VEC_LBOX_THD_OFFSET, 0x00802010);
#endif 
#if (CONFIG_DRV_LINUX_DATA_CONSISTENCY)
    //HalFlushInvalidateDCache();
#endif
    #if  (!CONFIG_DRV_VERIFY_SUPPORT)
    vVDECSetDownScalerPrm(u4VDecID, &prDecPrm->rDownScalerPrm);
    #endif
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560) && CONFIG_DRV_FTS_SUPPORT)
    // set letterbox detection parameter
    vVDECSetLetetrBoxDetPrm(u4VDecID, &prDecPrm->rLBDPrm);
#endif    
    vVDecMpegCommSetting(u4VDecID, prDecPrm);
    vVLDM4vDec(u4VDecID, prDecPrm, (prDecPrm->ucPicType == B_TYPE));
    return HAL_HANDLE_OK;
}


// **************************************************************************
// Function : INT32 i4VDEC_HAL_WMV_VPStart(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm);
// Description :Set video decoder hardware registers to vp for wmv
// Parameter : prDecWmvPrm : pointer to wmv decode info struct
// Return      : =0: success.
//                  <0: fail.
// **************************************************************************
INT32 i4VDEC_HAL_MPEG_VPStart(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm)
{
    VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *)prDecPrm->prVDecCodecHalPrm;
#if VDEC_DDR3_SUPPORT
    UINT32 u4DDR3_PicWdith = 0;
#endif

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
	UINT32 u4PicW,u4PicH,u4WithMB;
#endif


#if (CONFIG_DRV_LINUX_DATA_CONSISTENCY)
    //HalFlushInvalidateDCache();
#endif
    
    if (prDecPrm->ucDecFBufIdx == 2)
    {
       if(prMpegDecPrm->u4FRefBufIdx == 0)
       {
           prMpegDecPrm->rMpegFrameBufSa.u4Pic1YSa = prMpegDecPrm->rMpegFrameBufSa.u4Pic2YSa;
           prMpegDecPrm->rMpegFrameBufSa.u4Pic1CSa = prMpegDecPrm->rMpegFrameBufSa.u4Pic2CSa;
           prDecPrm->ucDecFBufIdx = 1;
       }
       else
       {
          prMpegDecPrm->rMpegFrameBufSa.u4Pic0YSa = prMpegDecPrm->rMpegFrameBufSa.u4Pic2YSa;
          prMpegDecPrm->rMpegFrameBufSa.u4Pic0CSa = prMpegDecPrm->rMpegFrameBufSa.u4Pic2CSa;
          prDecPrm->ucDecFBufIdx = 0;
       }
    }

    #if (CONFIG_CHIP_VER_CURR <= CONFIG_CHIP_VER_MT8560)
    vVDECSetDownScalerPrm(u4VDecID, &prDecPrm->rDownScalerPrm);
    #endif
    
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560) && CONFIG_DRV_FTS_SUPPORT)
    // set letterbox detection parameter
    vVDECSetLetetrBoxDetPrm(u4VDecID, &prDecPrm->rLBDPrm);
#endif    

    vVDecWriteMC(u4VDecID, RW_MC_R1Y, (u4AbsDramANc(prMpegDecPrm->rMpegFrameBufSa.u4Pic0YSa)) >> 9); // div 512
    vVDecWriteMC(u4VDecID, RW_MC_R1C, (u4AbsDramANc(prMpegDecPrm->rMpegFrameBufSa.u4Pic0CSa)) >> 8); // div 256
    vVDecWriteMC(u4VDecID, RW_MC_R2Y, (u4AbsDramANc(prMpegDecPrm->rMpegFrameBufSa.u4Pic1YSa)) >> 9); // div 512
    vVDecWriteMC(u4VDecID, RW_MC_R2C, (u4AbsDramANc(prMpegDecPrm->rMpegFrameBufSa.u4Pic1CSa)) >> 8); // div 256
    vVDecWriteMC(u4VDecID, RW_MC_BY,  (u4AbsDramANc(prMpegDecPrm->rMpegFrameBufSa.u4Pic2YSa)) >> 8); // div 256
    vVDecWriteMC(u4VDecID, RW_MC_BC,  (u4AbsDramANc(prMpegDecPrm->rMpegFrameBufSa.u4Pic2CSa)) >> 7); // div 128
    vVDecWriteMC(u4VDecID, RW_MC_BY1,  (u4AbsDramANc(prMpegDecPrm->rMpegFrameBufSa.u4Pic2YSa)) >> 9); // div 256
    vVDecWriteMC(u4VDecID, RW_MC_BC1,  (u4AbsDramANc(prMpegDecPrm->rMpegFrameBufSa.u4Pic2CSa)) >> 8); // div 128
    vVDecWriteMC(u4VDecID, RW_MC_DIGY, (u4AbsDramANc(prMpegDecPrm->rMpegFrameBufSa.u4Pic0YSa)) >> 9); // div 512
    vVDecWriteMC(u4VDecID, RW_MC_DIGC, (u4AbsDramANc(prMpegDecPrm->rMpegFrameBufSa.u4Pic0CSa)) >> 8); // div 256  
    vMCSetOutputBuf(u4VDecID, (UINT32)prDecPrm->ucDecFBufIdx, prMpegDecPrm->u4FRefBufIdx);

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
      #if VDEC_DDR3_SUPPORT
          u4DDR3_PicWdith = (((prDecPrm->u4PicBW + 63) >> 6) << 2);
          #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
          vVDecWriteMC(u4VDecID, RW_MC_PIC_W_MB, u4DDR3_PicWdith);  
          #else
          vVDecWriteVLD(u4VDecID, RW_VLD_PIC_W_MB, u4DDR3_PicWdith);  
          #endif
      #else
          #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
          vVDecWriteMC(u4VDecID, RW_MC_PIC_W_MB, ((prDecPrm->u4PicBW + 15)>> 4));  
          vVDecWriteMC(u4VDecID, RW_MC_DDR3_EN, (u4VDecReadMC(u4VDecID, RW_MC_DDR3_EN)  & 0xFFFFFFFE));
          #else
          vVDecWriteVLD(u4VDecID, RW_VLD_PIC_W_MB, ((prDecPrm->u4PicBW + 15)>> 4));  
          #endif
      #endif
#endif

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
    vVDecWriteMC(0, 0x5E4, (u4VDecReadMC(0, 0x5E4) |(0x1 <<12)) );
    //vVDecWriteMC(0, 0x660, (u4VDecReadMC(0, 0x660) |(0x80000000)) );    
    #ifndef VDEC_PIP_WITH_ONE_HW
    vVDecWriteMC(1, 0x5E4, (u4VDecReadMC(1, 0x5E4) |(0x1 <<12)) );
    //vVDecWriteMC(1, 0x660, (u4VDecReadMC(1, 0x660) |(0x80000000)) );    
    #endif
#endif

    if (prMpegDecPrm->rMpegPpInfo.fgPpEnable)
    {
        UINT32 u4MBqp = 0;
        
        vVDecWriteMC(u4VDecID, RW_MC_PP_ENABLE, 1);
        vVDecWriteMC(u4VDecID, RW_MC_PP_Y_ADDR, u4AbsDramANc(prMpegDecPrm->rMpegPpInfo.u4PpYBufSa) >> 9);
        vVDecWriteMC(u4VDecID, RW_MC_PP_C_ADDR, u4AbsDramANc(prMpegDecPrm->rMpegPpInfo.u4PpCBufSa) >> 8);
        vVDecWriteMC(u4VDecID, RW_MC_PP_MB_WIDTH, (prDecPrm->u4PicW + 15) >> 4);
        
        u4MBqp = (prMpegDecPrm->rMpegPpInfo.au1MBqp[0] & 0x1F) | ((UINT32)(prMpegDecPrm->rMpegPpInfo.au1MBqp[1] & 0x1F) << 8) \
                       | ((UINT32)(prMpegDecPrm->rMpegPpInfo.au1MBqp[2] & 0x1F) << 16) | ((UINT32)(prMpegDecPrm->rMpegPpInfo.au1MBqp[3] & 0x1F) << 24);        
        vVDecWriteMC(u4VDecID, RW_MC_PP_QP_TYPE, u4MBqp);  
        
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8550)
        vVDecWriteMC(u4VDecID, RW_MC_PP_DBLK_MODE, DBLK_Y+DBLK_C);
        vVDecWriteMC(u4VDecID, RW_MC_PP_WB_BY_POST, 0); // wirte MC out and PP out
        if (prMpegDecPrm->rMpegPpInfo.fgPpDemoEn)
        {
            vVDecWriteMC(u4VDecID, 0x658, ((u4VDecReadMC(u4VDecID, 0x658)&0xFFFFFFFE)|0x1)); // partial deblocking
            vVDecWriteMC(u4VDecID, 0x65C, ((((prDecPrm->u4PicH + 15) >> 4) - 1) << 24) | ((((prDecPrm->u4PicW + 15) >> 5) - 1) << 8)); // XY end MB
        }
        else
        {
            vVDecWriteMC(u4VDecID, 0x658, (u4VDecReadMC(u4VDecID, 0x658)&0xFFFFFFFE));
        }
#else
        vVDecWriteMC(u4VDecID, RW_MC_PP_DBLK_MODE, DBLK_Y+DBLK_C);
        //vVDecWriteMC(u4VDecID, RW_MC_PP_WB_BY_POST, 0); // wirte MC out and PP out
#endif        
        //vVDecWriteMC(u4VDecID, RW_MC_PP_QP_TYPE, 0x00000114);
        //vVDecWriteMC(u4VDecID, RW_MC_PP_WB_BY_POST, 0); // wirte MC out and PP out
        vVDecWriteMC(u4VDecID, RW_MC_PP_X_RANGE, ((prDecPrm->u4PicW + 15) >> 4) - 1);
        vVDecWriteMC(u4VDecID, RW_MC_PP_Y_RANGE, (((prDecPrm->u4PicH + 15) >> 4) >> (prDecPrm->ucPicStruct != 3)) - 1);
        //vVDecWriteAVCVLD(u4VDecID, RW_AVLD_SHDR_2, 0x6E00);
        //vVDecWriteMC(u4VDecID, RW_MC_PP_MODE, H264_MODE);
    }
    else
    {
        vVDecWriteMC(u4VDecID, RW_MC_PP_ENABLE, 0);
    }

    vVDecWriteVLD(u4VDecID, RW_VLD_PSUPCTR, ((prDecPrm->u4PicW * prDecPrm->u4PicH) >> 8) + 1);
    vVDecWriteVLD(u4VDecID, RW_VLD_PARA, 0xC0500000); //Frame Picture + VP ???

    #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
//    vVDecWriteVLD(u4VDecID, RW_VLD_PICSZ, ((prDecPrm->u4PicH) << 16) + (prDecPrm->u4PicW >> 4));
	
	u4PicW = ((prDecPrm->u4PicW +15)>>4)<<4;
	u4PicH = ((prDecPrm->u4PicH + 15)>>4)<<4;
	u4WithMB = ((prDecPrm->u4PicW +15)>>4);
	vVDecWriteVLDTOP(u4VDecID,RW_TOPVLD_WMV_PICSIZE,u4PicH<<16|u4PicW);
	vVDecWriteVLD(u4VDecID, RW_VLD_PICSZ, ((prDecPrm->u4PicH)<<16) | 0x1ff);
	//vVDecWriteVLD(u4VDecID, RW_VLD_PICSZ, (u4VDecReadVLD(u4VDecID, RW_VLD_PICSZ) | 0x1ff));
	vVDecWriteVLD(u4VDecID, RW_VLD_DIGMBSA, u4WithMB);
//	vVDecWriteTopVLD(u4VDecID,RW_TOPVLD_WMV_PICSIZE_MB,(((prDecPrm->u4PicW+ 15)>>4) -1) | ((((prDecPrm->u4PicH + 15)>>4) - 1)<<16));
	vVDecWriteVLD(u4VDecID, RW_VLD_MBROWPRM,  0x1ff );
    vVDecWriteDV(u4VDecID,VDEC_DV_INT_CFG,u4VDecReadDV(u4VDecID,VDEC_DV_INT_CFG)&(~(1<<12)));
    #else
    vVDecWriteVLD(u4VDecID, RW_VLD_PICSZ, ((prDecPrm->u4PicH + 15) << 16) + (prDecPrm->u4PicW >> 4));
    vVDecWriteVLD(u4VDecID, RW_VLD_MBROWPRM,  ( ((prDecPrm->u4PicH + 15) >> 4 ) - 1) << 16);
    #endif
    // addr swap mode
    vVDecWriteMC(u4VDecID, RW_MC_ADDRSWAP, prDecPrm->ucAddrSwapMode);
  
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8550)
     vVDecWriteMC(u4VDecID, RW_MC_NBM_CTRL,  
                 ((u4VDecReadMC(u4VDecID, RW_MC_NBM_CTRL)  & 0xFFFFFFF8) |prDecPrm->ucAddrSwapMode));
#endif 

    vVDecWriteMC(u4VDecID, RW_MC_UMV_PIC_WIDTH, prDecPrm->u4PicW);
    vVDecWriteMC(u4VDecID, RW_MC_UMV_PIC_HEIGHT, prDecPrm->u4PicH);

#if (CONFIG_CHIP_VER_CURR <= CONFIG_CHIP_VER_MT8560)
    vVDecWriteMC(u4VDecID, RW_MC_HREFP, 0);
    vVDecWriteMC(u4VDecID, RW_MC_DIGWD, ((prDecPrm->u4PicW + 15) >> 4));
    vVDecWriteVLD(u4VDecID, RW_VLD_DIGMBSA, 0);
    vVDecWriteVLD(u4VDecID, RW_VLD_SCALE, 0);//(random(3)<<24) |(random(3)<<16));
    vVDecWriteVLD(u4VDecID, RW_VLD_DIGMBYOFF, 0);
#endif

    vVDecWriteVLD(u4VDecID, RW_VLD_PROC, VLD_RTERR + VLD_PDHW + VLD_PSUP +
              (prDecPrm->u4PicW >> 4));
              
    return HAL_HANDLE_OK;
}


// **************************************************************************
// Function : void v4VDEC_HAL_MPEG_GetMbxMby(UINT32 u4VDecID, UINT32 *pu4Mbx, UINT32 *pu4Mby);
// Description :Read current decoded mbx and mby
// Parameter : u4VDecID : video decoder hardware ID
//                 pu4Mbx : macroblock x value
//                 pu4Mby : macroblock y value
// Return      : None
// **************************************************************************
void vVDEC_HAL_MPEG_GetMbxMby(UINT32 u4VDecID, UINT32 *pu4Mbx, UINT32 *pu4Mby)
{
    *pu4Mbx = u4VDecReadMC(u4VDecID, RO_MC_MBX);
    *pu4Mby = u4VDecReadMC(u4VDecID, RO_MC_MBY);
}


// **************************************************************************
// Function : void v4VDEC_HAL_MPEG_GetErrInfo(UINT32 u4VDecID, VDEC_INFO_MPEG_ERR_INFO_T *prMpegErrInfo);
// Description :Read error count after decoding end
// Parameter : u4VDecID : video decoder hardware ID
//                 prMpegErrInfo : pointer to mpeg error info struct
// Return      : None
// **************************************************************************
void vVDEC_HAL_MPEG_GetErrInfo(UINT32 u4VDecID, VDEC_INFO_MPEG_ERR_INFO_T *prMpegErrInfo)
{
    UINT32 u4RegVal;
    u4RegVal = u4VDecReadVLD(u4VDecID, RO_VLD_ERRFG);
    prMpegErrInfo->u4MpegErrCnt = ((u4RegVal >> 8) & 0xFFF);
    prMpegErrInfo->u4MpegErrRow = ((u4RegVal >> 24) & 0x3F);
    prMpegErrInfo->u4MpegErrType = (u4RegVal & 0x3F);
    
    u4RegVal = u4VDecReadVLD(u4VDecID, RO_VLD_FSBI);
    prMpegErrInfo->u2MpegMBErrCnt = ((u4RegVal >> 16) & 0xFFFF);
}

// **************************************************************************
// Function : vVDEC_HAL_MPEG2_DisableMVOverflowDetection(UINT32 u4VDecID)
// Description :Disable Motion Vector Overflow Dectetion
// Parameter : u4VDecID : video decoder hardware ID
// Return      : =0: success.
//                  <0: fail.
// **************************************************************************
INT32 vVDEC_HAL_MPEG2_DisableMVOverflowDetection(UINT32 u4VDecID)
{
    vVDecWriteVLD(u4VDecID, WO_VLD_BBUFRST, u4VDecReadVLD(u4VDecID, WO_VLD_BBUFRST) | VLD_MVOF_DET_DISABLE);
    return HAL_HANDLE_OK;
}

// **************************************************************************
// Function : UINT32 u4VDEC_HAL_MPEG4_GetErrType(UINT32 u4VDecID);
// Description :Read mpeg4 error type after decoding end
// Parameter : u4VDecID : video decoder hardware ID
//                 u4ErrType : pointer to mpeg4 decode error type value
// Return      : Mpeg4 or Divx3 decode error type
// **************************************************************************
UINT32 u4VDEC_HAL_MPEG4_GetErrType(UINT32 u4VDecID)
{
     return(u4VDecReadVLD(u4VDecID, RO_VLD_MP4DECERR) & 0x7f);
}

#if CONFIG_DRV_VERIFY_SUPPORT
void vVDEC_HAL_MPEG_VDec_ReadCheckSum(UINT32 u4VDecID, UINT32 *pu4DecCheckSum)
{
#if 1
	{
	    UINT32 reg, u4Val;
		//printk("CRC register ...............\n");
		for(reg = 2; reg < 10; reg++)
		{
		    u4Val = u4VDecReadCRC(u4VDecID, reg<<2);
			*pu4DecCheckSum++ = u4Val;
			//printk("%d (0x%x) = 0x%4x\n", reg, reg<<2, u4Val);
		}
	}
#else
    UINT32  u4Temp;
    
    u4Temp = 0;

    *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x5E8); // 378, 
    pu4DecCheckSum ++;
    u4Temp ++;
    *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x5EC); // 379, 
    pu4DecCheckSum ++;
    u4Temp ++;
    *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x5f0); // 380, 
    pu4DecCheckSum ++;
    u4Temp ++;
    *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x5f4); // 381, mc_out_y
    pu4DecCheckSum ++;
    u4Temp ++;
    *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x5f8); // 382, mc_out_c
    pu4DecCheckSum ++;
    u4Temp ++;
    *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x5fc); // 383, mv
    pu4DecCheckSum ++;
    u4Temp ++;
    *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x600); // 384, mc_para_1
    pu4DecCheckSum ++;
    u4Temp ++;
    *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x604); // 385, mc_para_2
    pu4DecCheckSum ++;
    u4Temp ++;
    *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x608);
    pu4DecCheckSum ++;
    u4Temp ++;
    *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x60C); 
    pu4DecCheckSum ++;
    u4Temp ++;
    *pu4DecCheckSum= u4VDecReadMC(u4VDecID, 0x610); //388, MC_DRAM_in
    pu4DecCheckSum ++;
    u4Temp ++;
    *pu4DecCheckSum= u4VDecReadMC(u4VDecID, 0x614); //389, MC_DRAM_in
    pu4DecCheckSum ++;
    u4Temp ++;
    *pu4DecCheckSum= u4VDecReadMC(u4VDecID, 0x618); //390, MC_DRAM_in
    pu4DecCheckSum ++;
    u4Temp ++;
    *pu4DecCheckSum= u4VDecReadMC(u4VDecID, 0x61C); //391, MC_DRAM_in
    pu4DecCheckSum ++;
    u4Temp ++;
    *pu4DecCheckSum= u4VDecReadMC(u4VDecID, 0x620); //392, MC_DRAM_in
    pu4DecCheckSum ++;
    u4Temp ++;
    *pu4DecCheckSum= u4VDecReadMC(u4VDecID, 0x624); //393, MC_DRAM_in
    pu4DecCheckSum ++;
    u4Temp ++;    
    *pu4DecCheckSum= u4VDecReadVLD(u4VDecID, 0x2f4); //189, chk_sum_all_vld
    pu4DecCheckSum ++;
    u4Temp ++;
    *pu4DecCheckSum= u4VDecReadVLD(u4VDecID, 0x3AC); // 235, dcac_out   
    pu4DecCheckSum ++;
    u4Temp ++;
    *pu4DecCheckSum= u4VDecReadVLD(u4VDecID, 0x3B4); // 237, dcac_dram_do
    pu4DecCheckSum ++;
    u4Temp ++;
    *pu4DecCheckSum = u4VDecReadVLD(u4VDecID, 0x3B8); // 238, is_data
    pu4DecCheckSum ++;
    u4Temp ++;
    *pu4DecCheckSum= u4VDecReadVLD(u4VDecID, 0x3B0); // 236, dcac_dram_di
    pu4DecCheckSum ++;
    u4Temp ++;
    
    while(u4Temp < MAX_CHKSUM_NUM)
    {
        *pu4DecCheckSum = 0;            
        pu4DecCheckSum ++;   
        u4Temp ++;
    }  
#endif
}


BOOL fgVDEC_HAL_MPEG_VDec_CompCheckSum(UINT32 *pu4DecCheckSum, UINT32 *pu4GoldenCheckSum)
{
    UINT32 i;

#if 1
	for (i = 0; i < 8; i ++)
	{
		if ((*(pu4GoldenCheckSum+i)) != (*(pu4DecCheckSum+i)))
		{
		  vVDecOutputDebugString("\n!!!!!!!!! Check Sum Compare Error  !!!!!!\n");
          break;
		}
	}

	if (i != 8)
	{
		printk("CRC register ...............\n");
		for(i = 0; i < 8; i++)
		{
			printk("[%d] = 0x%4x\n", i, pu4DecCheckSum[i]);
		}
		printk("CRC golden   ...............\n");
		for(i = 0; i < 8; i++)
		{
			printk("[%d] = 0x%4x\n", i, pu4GoldenCheckSum[i]);
		}
	}
	else
	{
	    printk("CRC compare OK\n");
	}
#else
    // MC
    for (i = 0; i < 5; i ++)
    {
        if ((*pu4GoldenCheckSum) != (*pu4DecCheckSum))
        {
          vVDecOutputDebugString("\n!!!!!!!!! MC Check Sum Compare Error  !!!!!!\n");
          return (FALSE);
        }
        pu4GoldenCheckSum ++;
        pu4DecCheckSum ++;
    }
    // VLD
    for (i = 0; i < 5; i ++)
    {
        if ((*pu4GoldenCheckSum) != (*pu4DecCheckSum))
        {
          vVDecOutputDebugString("\n!!!!!!!!! VLD Check Sum Compare Error  !!!!!!\n");
          return (FALSE);
        }
        pu4GoldenCheckSum ++;
        pu4DecCheckSum ++;
    }
#endif
    return (TRUE);
}


UINT32 u4VDEC_HAL_MPEG_VDec_ReadFinishFlag(UINT32 u4VDecID, BOOL fgMpeg4)
{
    if(fgMpeg4)
    {
        //6589NEW ?
        //return (u4VDecReadVLD(u4VDecID, RO_VLD_MP4DECBSY) & 0x1);
        return (u4VDecReadDV(u4VDecID,41*4) & (0x01 << 16));
    }
    else
    {
        return (u4VDecReadVLD(u4VDecID, RO_VLD_BLKFIN) & 0x1);
    }
} 

UINT32 u4VDEC_HAL_MPEG_VDec_ReadErrorFlag(UINT32 u4VDecID)
{
    return (u4VDecReadVLD(u4VDecID, 0x3CC) & (0x1 << 8));
}


void vVDEC_HAL_MPEG_VDec_DumpReg(UINT32 u4VDecID, BOOL fgBefore)
{
  UINT32 u4Temp;
  INT32 i;
  char strMessage[256];
  INT32 i4VldStart = 33;
  INT32 i4VldEnd = 256;
  INT32 i4McStart = 0;
  INT32 i4McEnd = 700;
  
  //#ifndef VDEC_SIM_DUMP
  //return;
  //#endif

  printk("<vdec> %s, %d!\n", __FUNCTION__, __LINE__);
  
  //_tRecFileInfo.fpFile = fopen(_bFileStr1[_u4VDecID][2],"a+t");
#if 1

   strcpy(_tRecFileInfo[u4VDecID].bFileName, _bFileStr1[u4VDecID][2]);    
  
  sprintf(strMessage, "%s", "\n");
  fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tRecFileInfo[u4VDecID]);
  sprintf(strMessage, "%s", "===================================================\n");
  fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tRecFileInfo[u4VDecID]);
  sprintf(strMessage, "%s", "===================================================\n");
  fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tRecFileInfo[u4VDecID]);
  //fprintf(_tRecFileInfo.fpFile, "\n");
  //fprintf(_tRecFileInfo.fpFile, "===================================================\n");
  //fprintf(_tRecFileInfo.fpFile, "===================================================\n");

 if (fgBefore == TRUE)
 {
     sprintf(strMessage, "%s", "Before Decode\n");
     fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tRecFileInfo[u4VDecID]);
 }
 else
 {
     sprintf(strMessage, "%s", "After Decode\n");
     fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tRecFileInfo[u4VDecID]);
 }
#endif  


#ifdef DOWN_SCALE_SUPPORT
  #if 1
  sprintf(strMessage, "%s", "MPEG DOWN SCALE setting\n");     
  fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tRecFileInfo[u4VDecID]);
  //fprintf(_tRecFileInfo.fpFile,"WMV DOWN SCALE setting\n");     
  for(i=0;i<20;i++) //
  {
    u4Temp = i;
    sprintf(strMessage, "%d (0x%.4x) = 0x%.8x\n", u4Temp, 0x2b800+(u4Temp << 2), u4ReadVDSCL(u4VDecID, (u4Temp << 2)));
    fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tRecFileInfo[u4VDecID]);
    //fprintf(_tRecFileInfo.fpFile, "%d (0x%.4x) = 0x%.8x\n", u4Temp, 0xf800+(u4Temp << 2), u4VDecReadVLD(u4VDecID, (u4Temp << 2)));
  }
  #endif
#endif

#if 1
  sprintf(strMessage, "<vdec> %s", "WMV VLD setting\n");     
  fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tRecFileInfo[u4VDecID]);
  //fprintf(_tRecFileInfo.fpFile,"WMV VLD setting\n");     
  for(i=i4VldStart; i<i4VldEnd; i++) //
  {
    u4Temp = i;
    sprintf(strMessage, "%d (0x%.4x) = 0x%.8x\n", u4Temp, (u4Temp << 2), u4VDecReadVLD(u4VDecID, (u4Temp << 2)));
    fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tRecFileInfo[u4VDecID]);
    //fprintf(_tRecFileInfo.fpFile, "%d (0x%.4x) = 0x%.8x\n", u4Temp, 0xf800+(u4Temp << 2), u4VDecReadVLD(u4VDecID, (u4Temp << 2)));
  }

  sprintf(strMessage, "\n\n<vdec> %s", "MPEG MC setting\n");     
  fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tRecFileInfo[u4VDecID]);
  //fprintf(_tRecFileInfo.fpFile,"WMV VLD setting\n");     
  for(i=i4McStart; i<i4McEnd; i++) //
  {
    u4Temp = i;
    sprintf(strMessage, "%d (0x%.4x) = 0x%.8x\n", u4Temp, (u4Temp << 2), u4VDecReadMC(u4VDecID, (u4Temp << 2)));
    fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tRecFileInfo[u4VDecID]);
    //fprintf(_tRecFileInfo.fpFile, "%d (0x%.4x) = 0x%.8x\n", u4Temp, 0xf800+(u4Temp << 2), u4VDecReadVLD(u4VDecID, (u4Temp << 2)));
  }
  #endif
// end : write out test bench for FPGA RTL simulation  
}
#endif


#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
void vVDEC_HAL_MPEG_WriteSram(UINT32 u4InstID,UINT32 u4SramAddr,UINT32 u4SramValue)
{
    UINT32 u4Temp;
	vVDecWriteMC(u4InstID,0x93C,u4VDecReadMC(u4InstID,0x93C)|(1<<0));//enable of sram and cs of sram
	vVDecWriteMC(u4InstID,0x93C,u4VDecReadMC(u4InstID,0x93C)&(~(0x3fff<<12)));//set sram addr
	vVDecWriteMC(u4InstID,0x93C,u4VDecReadMC(u4InstID,0x93C) | (u4SramAddr<<12));//set sram addr
	vVDecWriteMC(u4InstID,0x940,u4SramValue);//set sram data
	u4Temp = (1<<4)|(1<<8);
	vVDecWriteMC(u4InstID,0x93C,u4VDecReadMC(u4InstID,0x93C) | u4Temp);//enable write
	vVDecWriteMC(u4InstID,0x93C,0);//clear all
}
UINT32 u4VDEC_HAL_MPEG_ReadSram(UINT32 u4InstID,UINT32 u4SramAddr) 
{
    UINT32 u4RegVal;
	vVDecWriteMC(u4InstID,0x93C,u4VDecReadMC(u4InstID,0x93C)|(1<<0)|(1<<4));//enable of sram and cs of sram
    vVDecWriteMC(u4InstID,0x93C,u4VDecReadMC(u4InstID,0x93C)&(~(0x3fff<<12)));//set sram addr
	vVDecWriteMC(u4InstID,0x93C,u4VDecReadMC(u4InstID,0x93C) | u4SramAddr<<12);//set sram addr
	u4RegVal = u4VDecReadMC(u4InstID,0x944);
//	vVDecWriteMC(u4InstID,0x93C,u4VDecReadMC(u4InstID,0x93C)&(~(1<<0))&(~(1<<4)));//disable sram read 
	vVDecWriteMC(u4InstID,0x93C,0);
	return u4RegVal;
}
#endif

