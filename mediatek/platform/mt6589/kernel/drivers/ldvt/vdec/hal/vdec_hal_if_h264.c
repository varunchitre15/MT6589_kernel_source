
#include "vdec_hw_common.h"
#include "vdec_hal_if_h264.h"
#include "vdec_hw_h264.h"
#include "vdec_hal_errcode.h"
#include "../include/vdec_info_common.h"

//#include "x_hal_ic.h"
//#include "x_hal_1176.h"
//#include "x_debug.h"

//extern VDEC_INFO_VERIFY_FILE_INFO_T _tRecFileInfo;
//extern VDEC_INFO_H264_FBUF_INFO_T _ptFBufInfo[17];
//extern char _bFileStr1[9][300];
#include "../include/drv_common.h"

#if CONFIG_DRV_VERIFY_SUPPORT
#include "../verify/vdec_verify_general.h"
#include "../verify/vdec_verify_mpv_prov.h"
#include "../verify/vdec_info_verify.h"

#include <linux/string.h>
#if (!CONFIG_DRV_LINUX)
#include <string.h>
#include <stdio.h>
#include "x_printf.h"
#endif

extern BOOL fgWrMsg2PC(void* pvAddr, UINT32 u4Size, UINT32 u4Mode, VDEC_INFO_VERIFY_FILE_INFO_T *pFILE_INFO);
#endif

extern char gpfH264LogFileBuffer[4096];
extern int gfpH264log;
extern unsigned int gH264logbufferOffset;
int vdecwriteFile(int fp,char *buf,int writelen);

#define DBG_H264_PRINTF
/*
#define DBG_H264_PRINTF(format,...)  \
    do { \
        if (-1 != gfpH264log) {\
            { gH264logbufferOffset += sprintf((char *)(gpfH264LogFileBuffer+gH264logbufferOffset),format, ##__VA_ARGS__);} \
            if (gH264logbufferOffset >= 3840 ) { \
                vdecwriteFile(gfpH264log, gpfH264LogFileBuffer, gH264logbufferOffset); \
                gH264logbufferOffset = 0; \
            } \
        } \
    } while (0)
*/

const CHAR ZZ_SCAN[16]  =
{  0,  1,  4,  8,  5,  2,  3,  6,  9, 12, 13, 10,  7, 11, 14, 15
};

const CHAR ZZ_SCAN8[64] =
{  0,  1,  8, 16,  9,  2,  3, 10, 17, 24, 32, 25, 18, 11,  4,  5,
   12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13,  6,  7, 14, 21, 28,
   35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
   58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
};

#ifdef MPV_DUMP_H264_DEC_REG
void VDec_DumpH264Reg(UCHAR ucMpvId);
#endif

#if VDEC_MVC_SUPPORT
#define fgIsMVCDecode(arg)  (u4VDecReadAVCVLD(arg, RW_AVLD_RM03R) & MVC_SWITCH)
#endif

// **************************************************************************
// Function : INT32 i4VDEC_HAL_H264_InitVDecHW(UINT32 u4Handle, VDEC_INFO_H264_INIT_PRM_T *prH264VDecInitPrm);
// Description :Initialize video decoder hardware only for H264
// Parameter : u4VDecID : video decoder hardware ID
//                  prH264VDecInitPrm : pointer to VFIFO info struct
// Return      : =0: success.
//                  <0: fail.
// **************************************************************************
INT32 i4VDEC_HAL_H264_InitVDecHW(UINT32 u4VDecID, VDEC_INFO_H264_INIT_PRM_T *prH264VDecInitPrm)
{
#if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8555)
    vVDecResetHW(u4VDecID);
#elif (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560)
    vVDecResetHW(u4VDecID, VDEC_H264);
#else
    vVDecResetHW(u4VDecID, VDEC_UNKNOWN);
#endif

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
 //   vVDecWriteVLDTOP(u4VDecID, RO_VLD_TOP_TIMEOUT_THD, VLD_TOP_TIMEOUT_THD);
#else
    vVDecWriteAVCVLD(u4VDecID, RO_AVLD_TIMEOUT_THD, AVLD_TIMEOUT_THD);
#endif
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
    vVDecWriteAVCVLD(u4VDecID, RW_AVLD_ERR_MASK, (~(AVLD_MB_END_CHK 
    | AVLD_4BLOCKS_SKIP_CHK
    )));
#endif    

  #ifdef FGT_SUPPORT
    vInitFgtHWSetting(u4VDecID, prH264VDecInitPrm);
  #endif
  
    return  HAL_HANDLE_OK;
}


// **************************************************************************
// Function : UINT32 u4VDEC_HAL_H264_ShiftGetBitStream(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits);
// Description :Read barrel shifter after shifting
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
//                 u4ShiftBits : shift bits number
// Return      : Value of barrel shifter input window after shifting
// **************************************************************************
UINT32 u4VDEC_HAL_H264_ShiftGetBitStream(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits)
{
    UINT32 u4RegVal;
  
    u4RegVal = u4VDecAVCVLDGetBitS(u4BSID, u4VDecID, u4ShiftBits);
  
    return (u4RegVal);
}


// **************************************************************************
// Function : UINT32 u4VDEC_HAL_H264_GetBitStreamShift(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits);
// Description :Read Barrel Shifter before shifting
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
//                 u4ShiftBits : shift bits number
// Return      : Value of barrel shifter input window before shifting
// **************************************************************************
UINT32 u4VDEC_HAL_H264_GetBitStreamShift(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits)
{
    UINT32 u4RegVal0;
  
    u4RegVal0 = u4VDecAVCVLDGetBitS(u4BSID, u4VDecID, 0);
    u4VDecAVCVLDGetBitS(u4BSID, u4VDecID, u4ShiftBits);
    
    return (u4RegVal0);
}


// **************************************************************************
// Function : UINT32 u4VDEC_HAL_H264_GetRealBitStream(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits);
// Description :Read Barrel Shifter before shifting
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
//                 u4ShiftBits : shift bits number
// Return      : Most significant (32 - u4ShiftBits) bits of barrel shifter input window before shifting
// **************************************************************************
UINT32 u4VDEC_HAL_H264_GetRealBitStream(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits)
{
    UINT32 u4RegVal0;
  
    u4RegVal0 = u4VDecAVCVLDGetBitS(u4BSID, u4VDecID, 0);
    u4VDecAVCVLDGetBitS(u4BSID, u4VDecID, u4ShiftBits);
    
    return (u4RegVal0 >> (32-u4ShiftBits));
}


// **************************************************************************
// Function : UINT32 bVDEC_HAL_H264_GetBitStreamFlg(UINT32 u4BSID, UINT32 u4VDecID);
// Description :Read Barrel Shifter before shifting
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
// Return      : MSB of barrel shifter input window before shifting
// **************************************************************************
BOOL bVDEC_HAL_H264_GetBitStreamFlg(UINT32 u4BSID, UINT32 u4VDecID)
{
    UINT32 u4RegVal;
  
    u4RegVal = u4VDEC_HAL_H264_GetBitStreamShift(u4BSID, u4VDecID, 1);
    return ((u4RegVal >> 31));
}


// **************************************************************************
// Function : UINT32 u4VDEC_HAL_H264_UeCodeNum(UINT32 u4BSID, UINT32 u4VDecID);
// Description :Do UE variable length decoding
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
// Return      : Input window after UE variable length decoding
// **************************************************************************
UINT32 u4VDEC_HAL_H264_UeCodeNum(UINT32 u4BSID, UINT32 u4VDecID)
{
    if (u4BSID == 0)
    {
        return (u4VDecReadAVCVLD(u4VDecID,RO_AVLD_UE));
    }
    else
    {
        u4VDecReadAVCVLD(u4VDecID, RO_AVLD_2ND_BARL);
        return (u4VDecReadAVCVLD(u4VDecID,RO_AVLD_2ND_UE));
    } 
}


// **************************************************************************
// Function : INT32 i4VDEC_HAL_H264_SeCodeNum(UINT32 u4BSID, UINT32 u4VDecID);
// Description :Do SE variable length decoding
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
// Return      : Input window after SE variable length decoding
// **************************************************************************
INT32 i4VDEC_HAL_H264_SeCodeNum(UINT32 u4BSID, UINT32 u4VDecID)
{
    if (u4BSID == 0)
    {
        return ((INT32)u4VDecReadAVCVLD(u4VDecID, RO_AVLD_SE));
    }
    else
    {
        u4VDecReadAVCVLD(u4VDecID, RO_AVLD_2ND_BARL);
        return ((INT32)u4VDecReadAVCVLD(u4VDecID, RO_AVLD_2ND_SE));
    }   
}


// *********************************************************************
// Function    : UINT32 u4VDEC_HAL_H264_GetStartCode(UINT32 u4BSID, UINT32 u4VDecID)
// Description : Get next start code
// Parameter   : u4BSID : Barrel shifter ID
//                   u4VDecID : VLD ID
// Return      : None
// *********************************************************************
UINT32 u4VDEC_HAL_H264_GetStartCode(UINT32 u4BSID, UINT32 u4VDecID)
{
    UINT32 u4Temp;
    BOOL fgVLDRem03;
  
    u4Temp = u4VDEC_HAL_H264_ShiftGetBitStream(u4BSID, u4VDecID, 0);
    if (u4BSID == 0)
    {
        fgVLDRem03 = u4VDecReadAVCVLD(u4VDecID, RW_AVLD_CTRL) >> 31;
    }
    else
    {
        fgVLDRem03 = (u4VDecReadAVCVLD(u4VDecID, RW_AVLD_2ND_CTRL) & 0x40);
    }
  
    while (((u4Temp >> 8) != START_CODE) || fgVLDRem03)
    {
        u4Temp = u4VDEC_HAL_H264_ShiftGetBitStream(u4BSID, u4VDecID, 8);
        if (u4BSID == 0)
        {
            fgVLDRem03 = u4VDecReadAVCVLD(u4VDecID, RW_AVLD_CTRL) >> 31;
        }
        else
        {
            fgVLDRem03 = (u4VDecReadAVCVLD(u4VDecID, RW_AVLD_2ND_CTRL) & 0x40);
        }
    }
    u4Temp = u4VDEC_HAL_H264_GetBitStreamShift(u4BSID, u4VDecID, 32);
    return u4Temp;
}


// *********************************************************************
// Function    : UINT32 u4VDEC_HAL_H264_GetStartCode(UINT32 u4BSID, UINT32 u4VDecID)
// Description : Get next start code
// Parameter   : u4BSID : Barrel shifter ID
//                   u4VDecID : VLD ID
// Return      : None
// *********************************************************************
UINT32 u4VDEC_HAL_H264_GetStartCode_8530 (UINT32 u4BSID, UINT32 u4VDecID)
{	
    UINT32 u4Temp = 0;
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
    BOOL fgVLDRem03;
    UINT32 u4RetryNum = 0x100000;
    UINT32 i;

    if (u4BSID == 0)
    {
       u4Temp = u4VDEC_HAL_H264_ShiftGetBitStream(u4BSID, u4VDecID, 0);
       if (u4BSID == 0)
       {
           fgVLDRem03 = u4VDecReadAVCVLD(u4VDecID, RW_AVLD_CTRL) >> 31;
       }
       else
       {
           fgVLDRem03 = (u4VDecReadAVCVLD(u4VDecID, RW_AVLD_2ND_CTRL) & 0x40);
        }
  
       if (((u4Temp >> 8) != START_CODE) || fgVLDRem03)
       {           
       vVDecWriteAVCVLD(u4VDecID,  RW_AVLD_FSSR, FW_SEARCH_START_CODE);
   
       for(i=0; i < u4RetryNum; i++)
       {
          if ((u4VDecReadAVCVLD(u4VDecID,  RW_AVLD_FSSR) & 0x1) == 0)
          {              
               break;
          }
       }

        fgVLDRem03 = (u4VDecReadAVCVLD(u4VDecID,  RW_AVLD_RM03R) >> 11) & 0x1;
        if (i == u4RetryNum && !fgVLDRem03)
       {
           printk("Can not find AVC start code\n");
           VDEC_ASSERT(0);
        }
     }
     }
     else
     {
         VDEC_ASSERT(0);
     }
         
     u4Temp = u4VDEC_HAL_H264_GetBitStreamShift(u4BSID, u4VDecID, 32);    
    
    /*
    u4Temp = u4VDEC_HAL_H264_ShiftGetBitStream(u4BSID, u4VDecID, 0);
    if (u4BSID == 0)
    {
        fgVLDRem03 = u4VDecReadAVCVLD(u4VDecID, RW_AVLD_CTRL) >> 31;
    }
    else
    {
        fgVLDRem03 = (u4VDecReadAVCVLD(u4VDecID, RW_AVLD_2ND_CTRL) & 0x40);
    }
  
    while (((u4Temp >> 8) != START_CODE) || fgVLDRem03)
    {
        u4Temp = u4VDEC_HAL_H264_ShiftGetBitStream(u4BSID, u4VDecID, 8);
        if (u4BSID == 0)
        {
            fgVLDRem03 = u4VDecReadAVCVLD(u4VDecID, RW_AVLD_CTRL) >> 31;
        }
        else
        {
            fgVLDRem03 = (u4VDecReadAVCVLD(u4VDecID, RW_AVLD_2ND_CTRL) & 0x40);
        }
    }
    u4Temp = u4VDEC_HAL_H264_GetBitStreamShift(u4BSID, u4VDecID, 32);*/
#endif    
    return u4Temp;
}


// **************************************************************************
// Function : INT32 i4VDEC_HAL_H264_InitBarrelShifter(UINT32 u4BSID, UINT32 u4VDecID, VDEC_INFO_H264_BS_INIT_PRM_T *prH264BSInitPrm);
// Description :Initialize barrel shifter with byte alignment
// Parameter :u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
//                 prH264BSInitPrm : pointer to h264 initialize barrel shifter information struct
// Return      : =0: success.
//                  <0: fail.
// **************************************************************************
INT32 i4VDEC_HAL_H264_InitBarrelShifter(UINT32 u4BSID, UINT32 u4VDecID, VDEC_INFO_H264_BS_INIT_PRM_T *prH264BSInitPrm)
{
    BOOL fgInitBSResult;

#if 0//(CONFIG_DRV_VERIFY_SUPPORT) && (CONFIG_DRV_LINUX)    
    HalFlushInvalidateDCache();
#endif
    
    if (u4BSID == 0)
    {
#if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8555)
        vVDecResetHW(u4VDecID);
#elif (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560)
    vVDecResetHW(u4VDecID, VDEC_H264);
#else
    vVDecResetHW(u4VDecID, VDEC_UNKNOWN);
#endif
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
//    vVDecWriteVLDTOP(u4VDecID, RO_VLD_TOP_TIMEOUT_THD, VLD_TOP_TIMEOUT_THD);
#else
        vVDecWriteAVCVLD(u4VDecID, RO_AVLD_TIMEOUT_THD, AVLD_TIMEOUT_THD);
#endif
        fgInitBSResult = fgInitH264BarrelShift1(u4VDecID, prH264BSInitPrm);
    }
    else
    {
        fgInitBSResult = fgInitH264BarrelShift2(u4VDecID, prH264BSInitPrm);
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


// **************************************************************************
// Function : UINT32 u4VDEC_HAL_H264_ReadRdPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 *pu4Bits);
// Description :Read current read pointer
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
//                 pu4Bits : read pointer value with remained bits
// Return      : Read pointer value with byte alignment
// **************************************************************************
UINT32 u4VDEC_HAL_H264_ReadRdPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4VFIFOSa, UINT32 *pu4Bits)
{
    return u4VDecReadH264VldRPtr(u4BSID, u4VDecID, pu4Bits, PHYSICAL(u4VFIFOSa));
}


// **************************************************************************
// Function : void v4VDEC_HAL_H264_AlignRdPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4AlignType);
// Description :Align read pointer to byte,word or double word
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
//                 u4AlignType : read pointer align type
// Return      : None
// **************************************************************************
void vVDEC_HAL_H264_AlignRdPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4AlignType)
{
    return;
}


// **************************************************************************
// Function : UINT32 u4VDEC_HAL_H264_GetBitcount(UINT32 u4BSID, UINT32 u4VDecID);
// Description :Read barrel shifter bitcount after initializing 
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
// Return      : Current bit count
// **************************************************************************
UINT32 u4VDEC_HAL_H264_GetBitcount(UINT32 u4BSID, UINT32 u4VDecID)
{
    return HAL_HANDLE_OK;
}


// **************************************************************************
// Function : void vVDEC_HAL_H264_ScalingList(UINT32 u4BSID, UINT32 u4VDecID, CHAR *pcScalingList, UINT32 u4SizeOfScalingList, BOOL *pfgUseDefaultScalingMatrixFlag);
// Description :Decode scaling list
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
//                 u4SizeOfScalingList : size of scaling list
//                 pcScalingList : pointer to value of scaling list
//                 pfgUseDefaultScalingMatrixFlag : pointer to  flag to use default scaling list or not
// Return      : None
// **************************************************************************
void vVDEC_HAL_H264_ScalingList(UINT32 u4BSID, UINT32 u4VDecID, CHAR *pcScalingList, UINT32 u4SizeOfScalingList, BOOL *pfgUseDefaultScalingMatrixFlag)
{
    UINT32 i;
    UINT32 u4Scanj;
    INT32 i4LastScale;
    INT32 i4NextScale;
    INT32 i4DeltaScale;
   
    i4LastScale = 8;
    i4NextScale = 8;
  
    for (i=0; i<u4SizeOfScalingList; i++)
    {
        u4Scanj = (u4SizeOfScalingList==16) ? ZZ_SCAN[i]:ZZ_SCAN8[i];
        //printk("--[ %d ]------------------------------------------\n",  i);
        if (i4NextScale != 0)
        {
            i4DeltaScale = i4VDEC_HAL_H264_SeCodeNum(u4BSID, u4VDecID);
            //printk("\t i4DeltaScale %d\n",  i4DeltaScale);
            i4NextScale = (i4LastScale + i4DeltaScale + 256) % 256;
            *pfgUseDefaultScalingMatrixFlag = ((u4Scanj == 0) && (i4NextScale == 0)) ? TRUE : FALSE;
        }
        pcScalingList[u4Scanj] = (i4NextScale == 0)? i4LastScale : i4NextScale;
        i4LastScale = pcScalingList[u4Scanj];
    }
    /*
    for (i=0; i<u4SizeOfScalingList; i++) {
        if (i % 8 == 0) {
            printk("\n");
        }
        printk("%2d ", pcScalingList[i]);
    }
    printk("\n");
    */
}


// **************************************************************************
// Function : void vVDEC_HAL_H264_WriteScalingList(UINT32 u4VDecID, UINT32 u4Idx, CHAR *pcSlicePtr);
// Description :Write scaling list to HW
// Parameter : u4VDecID : video decoder hardware ID
//                 u4Idx : scaling list index
//                 pcSlicePtr : pointer to scaling list matrix
// Return      : None
// **************************************************************************
void vVDEC_HAL_H264_WriteScalingList(UINT32 u4VDecID, UINT32 u4Idx, CHAR *pcSlicePtr)
{
    INT32 i;
    UINT32 u4Temp;
    //printk("\n vVDEC_HAL_H264_WriteScalingList() %d\n", u4Idx);

    if (u4Idx < 6)
    {
        u4Idx = (u4Idx << 4);
        for(i=0; i<4; i++)
        {
            // add 16 for every list
            u4Temp = (((UCHAR)pcSlicePtr[i<<2]) << 24) + (((UCHAR)pcSlicePtr[(i<<2) + 1]) << 16) +(((UCHAR)pcSlicePtr[(i<<2) + 2]) << 8) + (((UCHAR)pcSlicePtr[(i<<2) + 3]));
            //printk("vVDEC_HAL_H264_WriteScalingList %d %d %d %d\n", pcSlicePtr[i<<2], pcSlicePtr[(i<<2) + 1], pcSlicePtr[(i<<2) + 2], pcSlicePtr[(i<<2) + 3]);
            vVDecWriteVLD(u4VDecID, RW_VLD_SCL_ADDR, 0x200 + u4Idx + (i << 2));
            vVDecWriteVLD(u4VDecID, RW_VLD_SCL_DATA, u4Temp);
            vVDecWriteVLD(u4VDecID, RW_VLD_SCL_ADDR, 0x100 + u4Idx + (i << 2));
        }
    }
    else
    {
        u4Idx = (u4Idx == 6) ? (u4Idx << 4) : ((u4Idx + 3) << 4); // 6=>16*6   7=>16*6+64(equal to 16*7+48)
        for (i=0; i<16; i++)
        {
            // add 64 for every list
            u4Temp = (((UCHAR)pcSlicePtr[i<<2]) << 24) + (((UCHAR)pcSlicePtr[(i<<2) + 1]) << 16) +(((UCHAR)pcSlicePtr[(i<<2) + 2]) << 8) + (((UCHAR)pcSlicePtr[(i<<2) + 3]));
            //printk("vVDEC_HAL_H264_WriteScalingList %d %d %d %d\n", pcSlicePtr[i<<2], pcSlicePtr[(i<<2) + 1], pcSlicePtr[(i<<2) + 2], pcSlicePtr[(i<<2) + 3]);
            vVDecWriteVLD(u4VDecID, RW_VLD_SCL_ADDR, 0x200 + u4Idx + (i << 2));
            vVDecWriteVLD(u4VDecID, RW_VLD_SCL_DATA, u4Temp);
            vVDecWriteVLD(u4VDecID, RW_VLD_SCL_ADDR, 0x100 + u4Idx + (i << 2));
        }
    }
}


// **************************************************************************
// Function : void vVDEC_HAL_H264_Reording(UINT32 u4VDecID);
// Description :Reference list reordering
// Parameter : u4VDecID : video decoder hardware ID
// Return      : None
// **************************************************************************
void vVDEC_HAL_H264_Reording(UINT32 u4VDecID)
{
    UINT32 u4Cnt;
    
    vVDecWriteAVCVLD(u4VDecID, RW_AVLD_RPL_REORD, 1);
    u4Cnt = 0;
    while(1)
    {
        if (u4Cnt == 100)
        {
            if (u4VDecReadAVCVLD(u4VDecID, RW_AVLD_RPL_REORD))
            {
                break;
            }
            else
            {
                u4Cnt = 0;        
            }
        }
        else
        {
            u4Cnt ++;
        }
    }
}

// **************************************************************************
// Function : void vVDEC_HAL_H264_PredWeightTable(UINT32 u4VDecID);
// Description :Decode prediction weighting table
// Parameter : u4VDecID : video decoder hardware ID
// Return      : None
// **************************************************************************
void vVDEC_HAL_H264_PredWeightTable(UINT32 u4VDecID)
{
  
    UINT32 u4Cnt;

    // if MT6589
    UINT32 u4Data;

    u4Data = u4VDecReadMISC(u4VDecID, 60 * 4);
    u4Data |= 0x01;
    vVDecWriteMISC(u4VDecID, 60 * 4, u4Data);
    // end if MT6589
    vVDecWriteAVCVLD(u4VDecID, RW_AVLD_WEIGHT_PRED_TBL, 1);
    u4Cnt = 0;
    while(1)
    {
        if (u4Cnt == 100)
        {
            if (u4VDecReadAVCVLD(u4VDecID, RW_AVLD_WEIGHT_PRED_TBL))
            {
                break;
            }
            else
            {
                u4Cnt = 0;        
            }
        }
        else
        {
            u4Cnt ++;
        }
    }
    // if MT6589
    u4Data &= 0xFFFFFFFE;
    vVDecWriteMISC(u4VDecID, 60 * 4, u4Data);
    // end if MT6589
  
}


// **************************************************************************
// Function : void vVDEC_HAL_H264_TrailingBits(UINT32 u4BSID, UINT32 u4VDecID);
// Description :Remove traling bits to byte align
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
// Return      : None
// **************************************************************************
void vVDEC_HAL_H264_TrailingBits(UINT32 u4BSID, UINT32 u4VDecID)
{
    UINT32 u4Temp;
  
    u4Temp = 8 - (u4VDecAVCVLDShiftBits(u4BSID, u4VDecID) % 8);
    // at list trailing bit
    if (u4Temp < 8)
    {
        u4Temp = u4VDecAVCVLDGetBitS(u4BSID, u4VDecID, u4Temp);
    }
}


// **************************************************************************
// Function : BOOL bVDEC_HAL_H264_IsMoreRbspData(UINT32 u4BSID, UINT32 u4VDecID);
// Description :Check whether there is more rbsp data
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
// Return      : Is morw Rbsp data or not
// **************************************************************************
BOOL bVDEC_HAL_H264_IsMoreRbspData(UINT32 u4BSID, UINT32 u4VDecID)
{
    UINT32 u4RemainedBits;
    UINT32 u4Temp;
    INT32 i;
    
    u4RemainedBits = (u4VDecAVCVLDShiftBits(u4BSID, u4VDecID) % 8); //0~7
    //u4RemainedBits = (8 - (((u4VDecReadAVCVLD(RW_AVLD_CTRL) >> 16) & 0x3F) % 8));  
    u4Temp = 0xffffffff;
    for (i=0; i<=u4RemainedBits; i++)
    {
        u4Temp &= (~(1<<i));
    }
    
    if ((u4VDecAVCVLDGetBitS(u4BSID, u4VDecID, 0) & u4Temp) == (0x80000000))
    {
        // no more
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}


// **************************************************************************
// Function : void vVDEC_HAL_H264_InitPRefList(UINT32 u4VDecID, BOOL fgPicFrm, UINT32 u4MaxFrameNum, UINT32 u4CurrPicNum);
// Description :Set HW registers to initialize P reference list
// Parameter : u4VDecID : video decoder hardware ID
//                 fgPicFrm : flag of frame picture or not
//                 u4MaxFrameNum : maximium frame number
//                 u4CurrPicNum : current pic number
// Return      : None
// **************************************************************************
void vVDEC_HAL_H264_InitPRefList(UINT32 u4VDecID, BOOL fgPicFrm, UINT32 u4MaxFrameNum, UINT32 u4CurrPicNum)
{
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8550)
    int i;
#endif

    vVDecWriteMC(u4VDecID, RW_AMC_P_LIST0_FLD, 0);
    vVDecWriteAVCVLD(u4VDecID, RW_AVLD_RESET_PIC_NUM, RESET_PIC_NUM);

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8550)    
    for(i= 0; i<10000; i++)
    {
         int a = 0;
         if ( (u4VDecReadAVCVLD(u4VDecID, RW_AVLD_RESET_PIC_NUM) & 0x1) == 0)
         {
            break;
         }            

         for (i = 0; i < 1000; i++)
         {
            a += 100;
         }
    }

    if (i == 10000)
    {
        printk("[VDEC][8550_AVC]: RW_AVLD_RESET_PIC_NUM: Pooling FAIL!!!\n");
    }
#endif
    vVDecWriteAVCVLD(u4VDecID, RW_AVLD_MAX_PIC_NUM, fgPicFrm ? u4MaxFrameNum : (u4MaxFrameNum << 1));
    vVDecWriteAVCVLD(u4VDecID, RW_AVLD_CUR_PIC_NUM, u4CurrPicNum);
}


// **************************************************************************
// Function : void vVDEC_HAL_H264_SetPRefPicListReg(UINT32 u4VDecID, VDEC_INFO_H264_P_REF_PRM_T *prPRefPicListInfo);
// Description :Set HW registers related with P reference list
// Parameter : u4VDecID : video decoder hardware ID
//                 prPRefPicListInfo : pointer to information of p reference list
// Return      : None
// **************************************************************************
void vVDEC_HAL_H264_SetPRefPicListReg(UINT32 u4VDecID, VDEC_INFO_H264_P_REF_PRM_T *prPRefPicListInfo)
{
    UCHAR ucFBufIdx;
    UCHAR ucFld;  
    //UCHAR bRefPicIdx;  
    UCHAR ucRegIdx;
    UINT32 u4Temp;
    BOOL fgLRefPic;
    UINT32 u4Param = 0;
    
    ucFld = (prPRefPicListInfo->u4FBufInfo & 0xff);  
    //bRefPicIdx = ((prPRefPicListInfo->u4FBufInfo >> 8) & 0xff);  
    ucRegIdx = ((prPRefPicListInfo->u4FBufInfo >> 16) & 0xff);  
  
    ucFBufIdx = prPRefPicListInfo->ucFBufIdx;
    fgLRefPic = (prPRefPicListInfo->u4ListIdx > 3)? TRUE: FALSE;
  
    if (ucFld == FRAME)
    {    
        //bRegIdx = bRefPicIdx;
#if CONFIG_DRV_VIRTUAL_ADDR
        vVDecWriteMC(u4VDecID, RW_MC_P_LIST0 + (ucRegIdx <<2), PHYSICAL((UINT32)  prPRefPicListInfo->u4FBufYStartAddr));
#else
        vVDecWriteMC(u4VDecID, RW_MC_P_LIST0 + (ucRegIdx <<2), u4AbsDramANc((UINT32)  prPRefPicListInfo->u4FBufYStartAddr));    
#endif
#if VDEC_MVC_SUPPORT
        if(fgIsMVCDecode(u4VDecID))
        {
            //vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_P0_RPL + (ucRegIdx<<2), (1 << 19) + (fgLRefPic << 20) + (fgLRefPic? (prPRefPicListInfo->i4LongTermPicNum & 0x7ffff) : (prPRefPicListInfo->i4PicNum & 0x7ffff)));
            u4Param = (1 << 19) + (fgLRefPic << 20) + (fgLRefPic? (prPRefPicListInfo->i4LongTermPicNum & 0x7ffff) : (prPRefPicListInfo->i4PicNum & 0x7ffff));
            vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_P0_RPL + (ucRegIdx<<2), u4Param);
            //printk("SetPRefPicList1: [0x%X] = 0x%X\n", RW_AVLD_REORD_P0_RPL + (ucRegIdx<<2),  u4Param);
        }
        else
#endif            
        {
            //vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_P0_RPL + (ucRegIdx<<2), (fgLRefPic << 19) + (fgLRefPic? (prPRefPicListInfo->i4LongTermPicNum & 0x7ffff) : (prPRefPicListInfo->i4PicNum & 0x7ffff)));
            u4Param = (fgLRefPic << 19) + (fgLRefPic? (prPRefPicListInfo->i4LongTermPicNum & 0x7ffff) : (prPRefPicListInfo->i4PicNum & 0x7ffff));
            vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_P0_RPL + (ucRegIdx<<2), u4Param);
            //printk("SetPRefPicList2: [0x%X] = 0x%X\n", RW_AVLD_REORD_P0_RPL + (ucRegIdx<<2),  u4Param);
        }
    
        u4Temp = (((prPRefPicListInfo->i4TFldPOC <= prPRefPicListInfo->i4BFldPOC) && (prPRefPicListInfo->i4TFldPOC != 0x7fffffff))? 1: 0) << 21;    
        u4Temp |= (fgLRefPic << 20);    
        u4Temp |= ((ucFBufIdx)<<1<<22) + prPRefPicListInfo->u4TFldPara;
        vVDecWriteAVCMV(u4VDecID, RW_AMV_P_REF_PARA + (ucRegIdx<<3), u4Temp);
    
        u4Temp = (((prPRefPicListInfo->i4TFldPOC <= prPRefPicListInfo->i4BFldPOC) && (prPRefPicListInfo->i4TFldPOC != 0x7fffffff))? 1: 0) << 21;    
        u4Temp |= (fgLRefPic << 20);    
        u4Temp |= ((((ucFBufIdx)<<1) + 1)<<22) + prPRefPicListInfo->u4BFldPara;
        vVDecWriteAVCMV(u4VDecID, RW_AMV_P_REF_PARA + (ucRegIdx<<3) + 4, u4Temp);
    }
    else
    { 
        if (ucFld == TOP_FIELD)
        {
#if CONFIG_DRV_VIRTUAL_ADDR
            vVDecWriteMC(u4VDecID, RW_MC_P_LIST0 + (ucRegIdx<<2), PHYSICAL((UINT32) prPRefPicListInfo->u4FBufYStartAddr));    
#else
            vVDecWriteMC(u4VDecID, RW_MC_P_LIST0 + (ucRegIdx<<2), u4AbsDramANc((UINT32) prPRefPicListInfo->u4FBufYStartAddr));    
#endif
#if VDEC_MVC_SUPPORT
            if(fgIsMVCDecode(u4VDecID))
            {
                //vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_P0_RPL + (ucRegIdx<<2), (1 << 19) + (fgLRefPic << 20) + (fgLRefPic? (prPRefPicListInfo->i4TFldLongTermPicNum & 0x7ffff) : (prPRefPicListInfo->i4TFldPicNum & 0x7ffff)));
                u4Param = (1 << 19) + (fgLRefPic << 20) + (fgLRefPic? (prPRefPicListInfo->i4TFldLongTermPicNum & 0x7ffff) : (prPRefPicListInfo->i4TFldPicNum & 0x7ffff));
                vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_P0_RPL + (ucRegIdx<<2), u4Param);
                //printk("SetPRefPicList3: [0x%X] = 0x%X\n", RW_AVLD_REORD_P0_RPL + (ucRegIdx<<2),  u4Param);
            }
            else
#endif            
            {
                //vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_P0_RPL + (ucRegIdx<<2), (fgLRefPic << 19) + (fgLRefPic? (prPRefPicListInfo->i4TFldLongTermPicNum & 0x7ffff) : (prPRefPicListInfo->i4TFldPicNum & 0x7ffff)));
                u4Param = (fgLRefPic << 19) + (fgLRefPic? (prPRefPicListInfo->i4TFldLongTermPicNum & 0x7ffff) : (prPRefPicListInfo->i4TFldPicNum & 0x7ffff));
                vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_P0_RPL + (ucRegIdx<<2), u4Param);
                 //printk("SetPRefPicList4: [0x%X] = 0x%X\n", RW_AVLD_REORD_P0_RPL + (ucRegIdx<<2),  u4Param);
            }
            
            //u4Temp = (((_ptFBufInfo[ucFBufIdx].i4TFldPOC <= _ptFBufInfo[ucFBufIdx].i4BFldPOC) && (_ptFBufInfo[ucFBufIdx].i4TFldPOC != 0x7fffffff))? 1: 0) << 21;    
            u4Temp = (fgLRefPic << 20);      
            u4Temp |= ((ucFBufIdx)<<1<<22) + prPRefPicListInfo->u4TFldPara;
            vVDecWriteAVCMV(u4VDecID, RW_AMV_P_REF_PARA + (ucRegIdx<<2), u4Temp);
        }
        else
        {        
#if CONFIG_DRV_VIRTUAL_ADDR
            vVDecWriteMC(u4VDecID, RW_MC_P_LIST0 + (ucRegIdx<<2), PHYSICAL((UINT32) prPRefPicListInfo->u4FBufYStartAddr));    
#else
            vVDecWriteMC(u4VDecID, RW_MC_P_LIST0 + (ucRegIdx<<2), u4AbsDramANc((UINT32) prPRefPicListInfo->u4FBufYStartAddr));    
#endif
#if VDEC_MVC_SUPPORT
            if(fgIsMVCDecode(u4VDecID))
            {
                //vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_P0_RPL + (ucRegIdx<<2), (1 << 19) + (fgLRefPic << 20) + (fgLRefPic? (prPRefPicListInfo->i4BFldLongTermPicNum & 0x7ffff) : (prPRefPicListInfo->i4BFldPicNum & 0x7ffff)));
                u4Param = (1 << 19) + (fgLRefPic << 20) + (fgLRefPic? (prPRefPicListInfo->i4BFldLongTermPicNum & 0x7ffff) : (prPRefPicListInfo->i4BFldPicNum & 0x7ffff));
                vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_P0_RPL + (ucRegIdx<<2), u4Param);
                 //printk("SetPRefPicList4: [0x%X] = 0x%X\n", RW_AVLD_REORD_P0_RPL + (ucRegIdx<<2),  u4Param);
            }
            else
#endif            
            {
                //vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_P0_RPL + (ucRegIdx<<2), (fgLRefPic << 19) + (fgLRefPic? (prPRefPicListInfo->i4BFldLongTermPicNum & 0x7ffff) : (prPRefPicListInfo->i4BFldPicNum & 0x7ffff)));
                u4Param = (fgLRefPic << 19) + (fgLRefPic? (prPRefPicListInfo->i4BFldLongTermPicNum & 0x7ffff) : (prPRefPicListInfo->i4BFldPicNum & 0x7ffff));
                vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_P0_RPL + (ucRegIdx<<2), u4Param);
                 //printk("SetPRefPicList4: [0x%X] = 0x%X\n", RW_AVLD_REORD_P0_RPL + (ucRegIdx<<2),  u4Param);
            }
            
            //u4Temp = (((_ptFBufInfo[ucFBufIdx].i4TFldPOC <= _ptFBufInfo[ucFBufIdx].i4BFldPOC) && (_ptFBufInfo[ucFBufIdx].i4TFldPOC != 0x7fffffff))? 1: 0) << 21;    
            u4Temp = (fgLRefPic << 20);      
            u4Temp |= (((ucFBufIdx<<1) + 1)<<22) + prPRefPicListInfo->u4BFldPara;
            vVDecWriteAVCMV(u4VDecID, RW_AMV_P_REF_PARA + (ucRegIdx<<2), u4Temp);
            
            vVDecWriteMC(u4VDecID, RW_AMC_P_LIST0_FLD, u4VDecReadMC(u4VDecID, RW_AMC_P_LIST0_FLD) | (0x1 << ucRegIdx));        
        }     
    }
}


// **************************************************************************
// Function : void vVDEC_HAL_H264_SetPOC(UINT32 u4VDecID, VDEC_INFO_H264_POC_PRM_T *prPOCInfo);
// Description :Set POC number to HW registers
// Parameter : u4VDecID : video decoder hardware ID
//                 prPOCInfo : pointer to information of current POC
// Return      : None
// **************************************************************************
void vVDEC_HAL_H264_SetPOC(UINT32 u4VDecID, VDEC_INFO_H264_POC_PRM_T *prPOCInfo)
{
    INT32 i4CurrPOC;
    
    if (prPOCInfo->fgIsFrmPic)
    {
        i4CurrPOC = prPOCInfo->i4POC;
        vVDecWriteAVCMV(u4VDecID, RW_AMV_CURR_TFLD_POC, prPOCInfo->i4TFldPOC & 0x3ffff);
        vVDecWriteAVCMV(u4VDecID, RW_AMV_CURR_BFLD_POC, prPOCInfo->i4BFldPOC & 0x3ffff);  
        vVDecWriteAVCMV(u4VDecID, RW_AMV_CURR_POC, prPOCInfo->i4POC & 0x3ffff);
    }
    else
    {
        i4CurrPOC = (prPOCInfo->ucPicStruct == TOP_FIELD)? prPOCInfo->i4TFldPOC : prPOCInfo->i4BFldPOC;
        vVDecWriteAVCMV(u4VDecID, RW_AMV_CURR_POC, i4CurrPOC & 0x3ffff);
    }
}


// **************************************************************************
// Function : void vVDEC_HAL_H264_InitBRefList(UINT32 u4VDecID);
// Description :Set HW registers to initialize B reference list
// Parameter : u4VDecID : video decoder hardware ID
// Return      : None
// **************************************************************************
void vVDEC_HAL_H264_InitBRefList(UINT32 u4VDecID)
{
    vVDecWriteMC(u4VDecID, RW_AMC_B_LIST0_FLD, 0);
    vVDecWriteMC(u4VDecID, RW_AMC_B_LIST1_FLD, 0); 
}


// **************************************************************************
// Function : BOOL bVDEC_HAL_H264_SetBRefPicListReg(UINT32 u4VDecID, VDEC_INFO_H264_B_REF_PRM_T *prBRefPicListInfo);
// Description :Set HW registers related with B reference list
// Parameter : u4VDecID : video decoder hardware ID
//                 prBRefPicListInfo : pointer to information of b reference list
// Return      : None
// **************************************************************************
BOOL bVDEC_HAL_H264_SetBRefPicListReg(UINT32 u4VDecID, VDEC_INFO_H264_B_REF_PRM_T *prBRefPicListInfo)
{
    UCHAR ucFBufIdx;
    UCHAR ucFld;  
    //UCHAR bRefPicIdx;  
    UCHAR ucRegIdx;
    UINT32 u4Temp;
    BOOL fgLRefPic;  
    UINT32 u4FieldDistance = 4;
#if (CONFIG_DRV_VERIFY_SUPPORT) && (!VDEC_DRV_PARSER)
    VDEC_INFO_H264_DEC_PRM_T *prH264DecPrm = (VDEC_INFO_H264_DEC_PRM_T *) &(_tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecH264DecPrm);
    //prH264DecPrm->prCurrFBufInfo->u4YStartAddr = prH264DecPrm->u4DecWorkBuf;
    prH264DecPrm->prCurrFBufInfo->u4CAddrOffset = prH264DecPrm->prCurrFBufInfo->u4DramPicSize;
    #if (!VDEC_H264_REDUCE_MV_BUFF)
    prH264DecPrm->prCurrFBufInfo->u4MvStartAddr = (prH264DecPrm->prCurrFBufInfo->u4YStartAddr + ((prH264DecPrm->prCurrFBufInfo->u4DramPicSize * 3) >>1));
    #endif
#else
    //VDEC_INFO_H264_DEC_PRM_T *prH264DecPrm = (VDEC_INFO_H264_DEC_PRM_T *)prDecPrm->prVDecCodecHalPrm;
#endif
    
    ucFld = (prBRefPicListInfo->u4FBufInfo & 0xff);  
    //bRefPicIdx = ((prBRefPicListInfo->u4FBufInfo >> 8) & 0xff);  
    ucRegIdx = ((prBRefPicListInfo->u4FBufInfo >> 16) & 0xff);  
  
    ucFBufIdx = prBRefPicListInfo->ucFBufIdx;
    fgLRefPic = (prBRefPicListInfo->u4ListIdx > 3)? TRUE: FALSE;
  

#if (CONFIG_DRV_VERIFY_SUPPORT)
  #if VDEC_H264_REDUCE_MV_BUFF
     if (prH264DecPrm->fgIsReduceMVBuffer)
     {
        vVDecWriteAVCMV(u4VDecID, RW_AMV_REDUCE_BMV,  (u4VDecReadAVCMV(u4VDecID, RW_AMV_REDUCE_BMV) | EN_AMV_REDUCE_BMV ));
    #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560)
    u4FieldDistance = 2;
    #else
        u4FieldDistance= 1;
    #endif
	
//        printk("AVC use reduced mv buffer\n");
     }
  #endif
#endif

    if (ucFld == FRAME)
    {
        // B_0
#if CONFIG_DRV_VIRTUAL_ADDR
        vVDecWriteMC(u4VDecID, RW_MC_B_LIST0 + (ucRegIdx<<2), PHYSICAL((UINT32) prBRefPicListInfo->u4FBufYStartAddr));    
#else
        vVDecWriteMC(u4VDecID, RW_MC_B_LIST0 + (ucRegIdx<<2), u4AbsDramANc((UINT32) prBRefPicListInfo->u4FBufYStartAddr));    
#endif
#if VDEC_MVC_SUPPORT
        if(fgIsMVCDecode(u4VDecID))
        {
            vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_B0_RPL + (ucRegIdx<<2), (1 << 19) + (fgLRefPic << 20) + (fgLRefPic? (prBRefPicListInfo->i4LongTermPicNum & 0x7ffff) : (prBRefPicListInfo->i4PicNum & 0x7ffff)));        
        }
        else
#endif            
        {
            vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_B0_RPL + (ucRegIdx<<2), (fgLRefPic << 19) + (fgLRefPic? (prBRefPicListInfo->i4LongTermPicNum & 0x7ffff) : (prBRefPicListInfo->i4PicNum & 0x7ffff)));
        }
        
        u4Temp = (((prBRefPicListInfo->i4TFldPOC <= prBRefPicListInfo->i4BFldPOC) && (prBRefPicListInfo->i4TFldPOC != 0x7fffffff))? 1: 0) << 21;
        u4Temp |= (fgLRefPic << 20);
        u4Temp += ((ucFBufIdx)<<1<<22) + prBRefPicListInfo->u4TFldPara + (prBRefPicListInfo->i4TFldPOC & 0x3ffff);
        vVDecWriteAVCMV(u4VDecID, RW_AMV_B0_REF_PARA + (ucRegIdx<<3), u4Temp);
  
        u4Temp = (((prBRefPicListInfo->i4TFldPOC <= prBRefPicListInfo->i4BFldPOC) && (prBRefPicListInfo->i4TFldPOC != 0x7fffffff))? 1: 0) << 21;
        u4Temp |= (fgLRefPic << 20);
        u4Temp += ((((ucFBufIdx)<<1) + 1)<<22) + prBRefPicListInfo->u4BFldPara + (prBRefPicListInfo->i4BFldPOC & 0x3ffff);
        vVDecWriteAVCMV(u4VDecID, RW_AMV_B0_REF_PARA + (ucRegIdx<<3) + 4, u4Temp);
  
  
        // B_1
        ucFBufIdx = prBRefPicListInfo->ucFBufIdx1;
#if CONFIG_DRV_VIRTUAL_ADDR
        vVDecWriteMC(u4VDecID, RW_MC_B_LIST1 + (ucRegIdx<<2), PHYSICAL((UINT32) prBRefPicListInfo->u4FBufYStartAddr1));    
#else
        vVDecWriteMC(u4VDecID, RW_MC_B_LIST1 + (ucRegIdx<<2), u4AbsDramANc((UINT32) prBRefPicListInfo->u4FBufYStartAddr1));    
#endif
#if VDEC_MVC_SUPPORT
        if(fgIsMVCDecode(u4VDecID))
        {
            vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_B1_RPL + (ucRegIdx<<2), (1 << 19) + (fgLRefPic << 20) + (fgLRefPic? (prBRefPicListInfo->i4LongTermPicNum1 & 0x7ffff) : (prBRefPicListInfo->i4PicNum1 & 0x7ffff)));        
        }
        else
#endif            
        {
            vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_B1_RPL + (ucRegIdx<<2), (fgLRefPic << 19) + (fgLRefPic? (prBRefPicListInfo->i4LongTermPicNum1 & 0x7ffff) : (prBRefPicListInfo->i4PicNum1 & 0x7ffff)));
        }
        
        u4Temp = (((prBRefPicListInfo->i4TFldPOC1 <= prBRefPicListInfo->i4BFldPOC1) && (prBRefPicListInfo->i4TFldPOC1 != 0x7fffffff))? 1: 0) << 21;
        u4Temp |= (fgLRefPic << 20);
        u4Temp += ((ucFBufIdx)<<1<<22) + prBRefPicListInfo->u4TFldPara1 + (prBRefPicListInfo->i4TFldPOC1 & 0x3ffff);
        vVDecWriteAVCMV(u4VDecID, RW_AMV_B1_REF_PARA + (ucRegIdx<<3), u4Temp);
  
        u4Temp = (((prBRefPicListInfo->i4TFldPOC1 <= prBRefPicListInfo->i4BFldPOC1) && (prBRefPicListInfo->i4TFldPOC1 != 0x7fffffff))? 1: 0) << 21;
        u4Temp |= (fgLRefPic << 20);
        u4Temp += ((((ucFBufIdx)<<1) + 1)<<22) + prBRefPicListInfo->u4BFldPara1 + (prBRefPicListInfo->i4BFldPOC1 & 0x3ffff);
        vVDecWriteAVCMV(u4VDecID, RW_AMV_B1_REF_PARA + (ucRegIdx<<3) + 4, u4Temp);

#if CONFIG_DRV_VIRTUAL_ADDR
        u4Temp = PHYSICAL((UINT32) prBRefPicListInfo->u4FBufMvStartAddr1);
#else
        u4Temp = u4AbsDramANc((UINT32) prBRefPicListInfo->u4FBufMvStartAddr1);
#endif
        vVDecWriteAVCMV(u4VDecID, RW_AMV_B1_REF_ADDR + (ucRegIdx<<3), u4Temp >> 4);
        vVDecWriteAVCMV(u4VDecID, RW_AMV_B1_REF_ADDR + (ucRegIdx<<3) + 4, (u4Temp >> 4) + u4FieldDistance);
    }
    else
    {
      // B_0
      if ((prBRefPicListInfo->u4ListIdx == 0) || (prBRefPicListInfo->u4ListIdx == 1) || (prBRefPicListInfo->u4ListIdx == 4) || (prBRefPicListInfo->u4ListIdx == 5))
      {      
        //u4Temp = (((_ptFBufInfo[ucFBufIdx].i4TFldPOC <= _ptFBufInfo[ucFBufIdx].i4BFldPOC) && (_ptFBufInfo[ucFBufIdx].i4TFldPOC != 0x7fffffff))? 1: 0) << 21;
        if (ucFld == TOP_FIELD)
        {
#if CONFIG_DRV_VIRTUAL_ADDR
          vVDecWriteMC(u4VDecID, RW_MC_B_LIST0 + (ucRegIdx<<2), PHYSICAL((UINT32) prBRefPicListInfo->u4FBufYStartAddr));    
#else
          vVDecWriteMC(u4VDecID, RW_MC_B_LIST0 + (ucRegIdx<<2), u4AbsDramANc((UINT32) prBRefPicListInfo->u4FBufYStartAddr));    
#endif
#if VDEC_MVC_SUPPORT
            if(fgIsMVCDecode(u4VDecID))
            {
                vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_B0_RPL + (ucRegIdx<<2), (1 << 19) + (fgLRefPic << 20) + (fgLRefPic? (prBRefPicListInfo->i4TFldLongTermPicNum & 0x7ffff) : (prBRefPicListInfo->i4TFldPicNum & 0x7ffff)));
            }
            else
#endif            
            {
                vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_B0_RPL + (ucRegIdx<<2), (fgLRefPic << 19) + (fgLRefPic? (prBRefPicListInfo->i4TFldLongTermPicNum & 0x7ffff) : (prBRefPicListInfo->i4TFldPicNum & 0x7ffff)));
            }
          
          u4Temp = ((ucFBufIdx)<<1<<22) + prBRefPicListInfo->u4TFldPara + (prBRefPicListInfo->i4TFldPOC & 0x3ffff);
          u4Temp |= (fgLRefPic << 20);
          vVDecWriteAVCMV(u4VDecID, RW_AMV_B0_REF_PARA + (ucRegIdx<<2), u4Temp);
        }
        else
        {        
#if CONFIG_DRV_VIRTUAL_ADDR
          vVDecWriteMC(u4VDecID, RW_MC_B_LIST0 + (ucRegIdx<<2), PHYSICAL((UINT32) prBRefPicListInfo->u4FBufYStartAddr));    
#else
          vVDecWriteMC(u4VDecID, RW_MC_B_LIST0 + (ucRegIdx<<2), u4AbsDramANc((UINT32) prBRefPicListInfo->u4FBufYStartAddr));    
#endif
#if VDEC_MVC_SUPPORT
            if(fgIsMVCDecode(u4VDecID))
            {
                vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_B0_RPL + (ucRegIdx<<2), (1 << 19) + (fgLRefPic << 20) + (fgLRefPic? (prBRefPicListInfo->i4BFldLongTermPicNum & 0x7ffff) : (prBRefPicListInfo->i4BFldPicNum & 0x7ffff)));
            }
            else
#endif            
            {
                vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_B0_RPL + (ucRegIdx<<2), (fgLRefPic << 19) + (fgLRefPic? (prBRefPicListInfo->i4BFldLongTermPicNum & 0x7ffff) : (prBRefPicListInfo->i4BFldPicNum & 0x7ffff)));
            }
          
          u4Temp = (((ucFBufIdx<<1) + 1)<<22) + prBRefPicListInfo->u4BFldPara + (prBRefPicListInfo->i4BFldPOC & 0x3ffff);
          u4Temp |= (fgLRefPic << 20);
          vVDecWriteAVCMV(u4VDecID, RW_AMV_B0_REF_PARA + (ucRegIdx<<2), u4Temp);
          
          vVDecWriteMC(u4VDecID, RW_AMC_B_LIST0_FLD, u4VDecReadMC(u4VDecID, RW_AMC_B_LIST0_FLD) | (0x1 << ucRegIdx));        
        }
      }
      // B_1    
      if ((prBRefPicListInfo->u4ListIdx == 2) || (prBRefPicListInfo->u4ListIdx == 3) || (prBRefPicListInfo->u4ListIdx == 4) || (prBRefPicListInfo->u4ListIdx == 5))
      {      
        //u4Temp = (((_ptFBufInfo[ucFBufIdx].i4TFldPOC <= _ptFBufInfo[ucFBufIdx].i4BFldPOC) && (_ptFBufInfo[ucFBufIdx].i4TFldPOC != 0x7fffffff))? 1: 0) << 21;
        if (ucFld == TOP_FIELD)
        {
#if CONFIG_DRV_VIRTUAL_ADDR
          vVDecWriteMC(u4VDecID, RW_MC_B_LIST1 + (ucRegIdx<<2), PHYSICAL((UINT32) prBRefPicListInfo->u4FBufYStartAddr));    
#else
          vVDecWriteMC(u4VDecID, RW_MC_B_LIST1 + (ucRegIdx<<2), u4AbsDramANc((UINT32) prBRefPicListInfo->u4FBufYStartAddr));    
#endif
#if VDEC_MVC_SUPPORT
            if(fgIsMVCDecode(u4VDecID))
            {
                vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_B1_RPL + (ucRegIdx<<2), (1 << 19) +(fgLRefPic << 20) + (fgLRefPic? (prBRefPicListInfo->i4TFldLongTermPicNum & 0x7ffff) : (prBRefPicListInfo->i4TFldPicNum & 0x7ffff)));
            }
            else
#endif            
            {
                vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_B1_RPL + (ucRegIdx<<2), (fgLRefPic << 19) + (fgLRefPic? (prBRefPicListInfo->i4TFldLongTermPicNum & 0x7ffff) : (prBRefPicListInfo->i4TFldPicNum & 0x7ffff)));
            }
#if CONFIG_DRV_VIRTUAL_ADDR
          u4Temp = PHYSICAL((UINT32) prBRefPicListInfo->u4FBufMvStartAddr);
#else
          u4Temp = u4AbsDramANc((UINT32) prBRefPicListInfo->u4FBufMvStartAddr);
#endif
          vVDecWriteAVCMV(u4VDecID, RW_AMV_B1_REF_ADDR + (ucRegIdx<<2), (u4Temp >> 4));      
          
          u4Temp = ((ucFBufIdx)<<1<<22) + prBRefPicListInfo->u4TFldPara + (prBRefPicListInfo->i4TFldPOC & 0x3ffff);
          u4Temp |= (fgLRefPic << 20);
          vVDecWriteAVCMV(u4VDecID, RW_AMV_B1_REF_PARA + (ucRegIdx<<2), u4Temp);
        }
        else
        {
#if CONFIG_DRV_VIRTUAL_ADDR
          vVDecWriteMC(u4VDecID, RW_MC_B_LIST1 + (ucRegIdx<<2), PHYSICAL((UINT32) prBRefPicListInfo->u4FBufYStartAddr));    
#else
          vVDecWriteMC(u4VDecID, RW_MC_B_LIST1 + (ucRegIdx<<2), u4AbsDramANc((UINT32) prBRefPicListInfo->u4FBufYStartAddr));    
#endif
#if VDEC_MVC_SUPPORT
            if(fgIsMVCDecode(u4VDecID))
            {
                vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_B1_RPL + (ucRegIdx<<2), (1 << 19) +(fgLRefPic << 20) + (fgLRefPic? (prBRefPicListInfo->i4BFldLongTermPicNum & 0x7ffff) : (prBRefPicListInfo->i4BFldPicNum & 0x7ffff)));
            }
            else
#endif            
            {
                vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_B1_RPL + (ucRegIdx<<2), (fgLRefPic << 19) + (fgLRefPic? (prBRefPicListInfo->i4BFldLongTermPicNum & 0x7ffff) : (prBRefPicListInfo->i4BFldPicNum & 0x7ffff)));
            }

#if CONFIG_DRV_VIRTUAL_ADDR
          u4Temp = PHYSICAL((UINT32) prBRefPicListInfo->u4FBufMvStartAddr);
#else  
          u4Temp = u4AbsDramANc((UINT32) prBRefPicListInfo->u4FBufMvStartAddr);
#endif
          vVDecWriteAVCMV(u4VDecID, RW_AMV_B1_REF_ADDR + (ucRegIdx<<2), (u4Temp >> 4) + u4FieldDistance);
       
          u4Temp = (((ucFBufIdx<<1) + 1)<<22) + prBRefPicListInfo->u4BFldPara + (prBRefPicListInfo->i4BFldPOC & 0x3ffff);
          u4Temp |= (fgLRefPic << 20);
          vVDecWriteAVCMV(u4VDecID, RW_AMV_B1_REF_PARA + (ucRegIdx<<2), u4Temp);
  
          vVDecWriteMC(u4VDecID, RW_AMC_B_LIST1_FLD, u4VDecReadMC(u4VDecID, RW_AMC_B_LIST1_FLD) | (0x1 << ucRegIdx));        
        }
      }
    }
    if (u4VDecReadAVCMV(u4VDecID, RW_AMV_B0_REF_PARA + (ucRegIdx<<2)) != u4VDecReadAVCMV(u4VDecID, RW_AMV_B1_REF_PARA + (ucRegIdx<<2)))
    {
      return TRUE;
    }
    else
    {
      return FALSE;
    }
}


// **************************************************************************
// Function : void vVDEC_HAL_H264_B1ListSwap(UINT32 u4VDecID, BOOL fgIsFrmPic);
// Description :Swap B1 reference list1 
// Parameter : u4VDecID : video decoder hardware ID
//                 fgIsFrmPic : flag to frame picture or not
// Return      : None
// **************************************************************************
void vVDEC_HAL_H264_B1ListSwap(UINT32 u4VDecID, BOOL fgIsFrmPic)
{
    UINT32 u4Temp;
    
    if (fgIsFrmPic)
    {
        u4Temp = u4VDecReadMC(u4VDecID, RW_MC_B_LIST1);
        vVDecWriteMC(u4VDecID, RW_MC_B_LIST1, u4VDecReadMC(u4VDecID, RW_MC_B_LIST1 + 4));
        vVDecWriteMC(u4VDecID, RW_MC_B_LIST1 + 4, u4Temp);        
        
        u4Temp = u4VDecReadAVCVLD(u4VDecID, RW_AVLD_REORD_B1_RPL);    
        vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_B1_RPL, u4VDecReadAVCVLD(u4VDecID, RW_AVLD_REORD_B1_RPL + 4));
        vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_B1_RPL + 4, u4Temp);        
          
        u4Temp = u4VDecReadAVCMV(u4VDecID, RW_AMV_B1_REF_PARA);
        vVDecWriteAVCMV(u4VDecID, RW_AMV_B1_REF_PARA, u4VDecReadAVCMV(u4VDecID, RW_AMV_B1_REF_PARA + 8));
        vVDecWriteAVCMV(u4VDecID, RW_AMV_B1_REF_PARA + 8, u4Temp);        
          
        u4Temp = u4VDecReadAVCMV(u4VDecID, RW_AMV_B1_REF_PARA + 4);
        vVDecWriteAVCMV(u4VDecID, RW_AMV_B1_REF_PARA + 4, u4VDecReadAVCMV(u4VDecID, RW_AMV_B1_REF_PARA + 12));
        vVDecWriteAVCMV(u4VDecID, RW_AMV_B1_REF_PARA + 12, u4Temp); 
          
        u4Temp = u4VDecReadAVCMV(u4VDecID, RW_AMV_B1_REF_ADDR);
        vVDecWriteAVCMV(u4VDecID, RW_AMV_B1_REF_ADDR, u4VDecReadAVCMV(u4VDecID, RW_AMV_B1_REF_ADDR + 8));
        vVDecWriteAVCMV(u4VDecID, RW_AMV_B1_REF_ADDR + 8, u4Temp);        
        
        u4Temp = u4VDecReadAVCMV(u4VDecID, RW_AMV_B1_REF_ADDR + 4);
        vVDecWriteAVCMV(u4VDecID, RW_AMV_B1_REF_ADDR + 4, u4VDecReadAVCMV(u4VDecID, RW_AMV_B1_REF_ADDR + 12));
        vVDecWriteAVCMV(u4VDecID, RW_AMV_B1_REF_ADDR + 12, u4Temp); 
    }
    else
    {
        u4Temp = u4VDecReadMC(u4VDecID, RW_MC_B_LIST1);
        vVDecWriteMC(u4VDecID, RW_MC_B_LIST1, u4VDecReadMC(u4VDecID, RW_MC_B_LIST1 + 4));
        vVDecWriteMC(u4VDecID, RW_MC_B_LIST1 + 4, u4Temp);        
          
        u4Temp = u4VDecReadAVCVLD(u4VDecID, RW_AVLD_REORD_B1_RPL);    
        vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_B1_RPL, u4VDecReadAVCVLD(u4VDecID, RW_AVLD_REORD_B1_RPL + 4));
        vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_B1_RPL + 4, u4Temp);        
          
        u4Temp = u4VDecReadAVCMV(u4VDecID, RW_AMV_B1_REF_PARA);
        vVDecWriteAVCMV(u4VDecID, RW_AMV_B1_REF_PARA, u4VDecReadAVCMV(u4VDecID, RW_AMV_B1_REF_PARA + 4));
        vVDecWriteAVCMV(u4VDecID, RW_AMV_B1_REF_PARA + 4, u4Temp);        
    
        u4Temp = u4VDecReadAVCMV(u4VDecID, RW_AMV_B1_REF_ADDR);
        vVDecWriteAVCMV(u4VDecID, RW_AMV_B1_REF_ADDR, u4VDecReadAVCMV(u4VDecID, RW_AMV_B1_REF_ADDR + 4));
        vVDecWriteAVCMV(u4VDecID, RW_AMV_B1_REF_ADDR + 4, u4Temp);        
     
        u4Temp = u4VDecReadMC(u4VDecID, RW_AMC_B_LIST1_FLD) & 1; // bit 0
        // Write bit 1 to bit 0
        vVDecWriteMC(u4VDecID, RW_AMC_B_LIST1_FLD,  (u4VDecReadMC(u4VDecID, RW_AMC_B_LIST1_FLD) & (~1)) | ((u4VDecReadMC(u4VDecID, RW_AMC_B_LIST1_FLD)  >> 1) & 1)); 
        vVDecWriteMC(u4VDecID, RW_AMC_B_LIST1_FLD,  (u4VDecReadMC(u4VDecID, RW_AMC_B_LIST1_FLD) & (~2)) | (u4Temp << 1)); 
    }
}


// **************************************************************************
// Function : void vVDEC_HAL_H264_SetSPSAVLD(UINT32 u4VDecID, VDEC_INFO_H264_SPS_T *prSPS);
// Description :Set SPS data to HW
// Parameter : u4VDecID : video decoder hardware ID
//                 prSPS : pointer to sequence parameter set struct
// Return      : None
// **************************************************************************
void vVDEC_HAL_H264_SetSPSAVLD(UINT32 u4VDecID, VDEC_INFO_H264_SPS_T *prSPS)
{
    UINT32 u4SPSInfo;
    
    u4SPSInfo = (prSPS->u4ChromaFormatIdc & 0x3); // 1~0
    u4SPSInfo |= ((prSPS->u4Log2MaxFrameNumMinus4 & 0xf)<< 2); //5~2
    u4SPSInfo |= ((prSPS->u4PicOrderCntType & 0x3) << 6); //7~6
    u4SPSInfo |= ((prSPS->u4Log2MaxPicOrderCntLsbMinus4 & 0xf) << 8);
    u4SPSInfo |= (prSPS->fgDeltaPicOrderAlwaysZeroFlag << 12);
    u4SPSInfo |= ((prSPS->u4NumRefFrames & 0x1f) << 13);
    u4SPSInfo |= (prSPS->fgFrameMbsOnlyFlag << 18);
    u4SPSInfo |= (prSPS->fgMbAdaptiveFrameFieldFlag << 19);  
    u4SPSInfo |= (prSPS->fgDirect8x8InferenceFlag << 20);    
    u4SPSInfo |= (1 << 21);    
    vVDecWriteAVCVLD(u4VDecID, RW_AVLD_SPS, u4SPSInfo);
    #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    vVDecWriteVLDTOP(u4VDecID, RW_VLD_PIC_MB_SIZE_M1, ((prSPS->u4PicHeightInMapUnitsMinus1) << VLD_TOP_PIC_HEIGHT_IN_MBS_POS) | prSPS->u4PicWidthInMbsMinus1);  
    #else
    vVDecWriteAVCVLD(u4VDecID, RW_AVLD_PIC_SIZE, ((prSPS->u4PicHeightInMapUnitsMinus1) << AVLD_PIC_HEIGHT_IN_MBS_POS) | prSPS->u4PicWidthInMbsMinus1);  
    #endif
}


// **************************************************************************
// Function : void vVDEC_HAL_H264_SetPPSAVLD(UINT32 u4VDecID, BOOL fgUserScalingMatrixPresentFlag,
//    BOOL *fgUserScalingListPresentFlag, VDEC_INFO_H264_PPS_T *prVDecH264PPS);
// Description :Set PPS data to HW
// Parameter : u4VDecID : video decoder hardware ID
//                 prPPS : pointer to picture parameter set struct
// Return      : None
// **************************************************************************
void vVDEC_HAL_H264_SetPPSAVLD(UINT32 u4VDecID, BOOL fgUserScalingMatrixPresentFlag,
    BOOL *pfgUserScalingListPresentFlag, VDEC_INFO_H264_PPS_T *prVDecH264PPS)
{
    UINT32 u4PPSInfo;
    INT32 i;
    
    u4PPSInfo = prVDecH264PPS->fgEntropyCodingModeFlag;
    u4PPSInfo |= (prVDecH264PPS->fgPicOrderPresentFlag << 1);
    u4PPSInfo |= (prVDecH264PPS->fgWeightedPredFlag << 2);
    u4PPSInfo |= ((prVDecH264PPS->u4WeightedBipredIdc & 0x03) << 3);  
    u4PPSInfo |= ((prVDecH264PPS->i4PicInitQpMinus26 & 0x3f) << 5);  
    u4PPSInfo |= ((prVDecH264PPS->i4ChromaQpIndexOffset & 0x1f)<< 11);    
    u4PPSInfo |= (prVDecH264PPS->fgDeblockingFilterControlPresentFlag << 16);
    u4PPSInfo |= (prVDecH264PPS->fgConstrainedIntraPredFlag << 17);
    u4PPSInfo |= (prVDecH264PPS->fgTransform8x8ModeFlag << 18);  
    u4PPSInfo |= ((prVDecH264PPS->i4SecondChromaQpIndexOffset & 0x1f) << 19);  
    
    vVDecWriteAVCVLD(u4VDecID, RW_AVLD_PPS_1 , u4PPSInfo);
  
    u4PPSInfo = prVDecH264PPS->u4NumRefIdxL0ActiveMinus1;
    u4PPSInfo |= (prVDecH264PPS->u4NumRefIdxL1ActiveMinus1 << 5);
    for (i=0; i<8; i++)
    {
      u4PPSInfo |= (pfgUserScalingListPresentFlag[i] << (10 + i));  
    }
    u4PPSInfo |= (fgUserScalingMatrixPresentFlag << 18);  
    
    vVDecWriteAVCVLD(u4VDecID, RW_AVLD_PPS_2 , u4PPSInfo);
}


// **************************************************************************
// Function : void vVDEC_HAL_H264_SetSHDRAVLD1(UINT32 u4VDecID, VDEC_INFO_H264_SLICE_HDR_T *prSliceHdr);
// Description :Set part of slice header data to HW
// Parameter : u4VDecID : video decoder hardware ID
//                 prSliceHdr : pointer to slice parameter set struct
// Return      : None
// **************************************************************************
void vVDEC_HAL_H264_SetSHDRAVLD1(UINT32 u4VDecID, VDEC_INFO_H264_SLICE_HDR_T *prSliceHdr)
{
    UINT32 u4SHDRInfo;
    
    u4SHDRInfo = prSliceHdr->u4FirstMbInSlice & 0x1fff;
    u4SHDRInfo |= ((prSliceHdr->u4SliceType & 0xf) << 13);
    u4SHDRInfo |= (prSliceHdr->fgFieldPicFlag << 17);
    u4SHDRInfo |= (prSliceHdr->fgBottomFieldFlag << 18);
    u4SHDRInfo |= (prSliceHdr->fgDirectSpatialMvPredFlag << 19);
    u4SHDRInfo |= ((prSliceHdr->u4NumRefIdxL0ActiveMinus1 & 0x1f) << 20);
    u4SHDRInfo |= ((prSliceHdr->u4NumRefIdxL1ActiveMinus1 & 0x1f) << 25);  
  
    #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    vVDecWriteAVCVLD(u4VDecID, RW_AVLD_PIC_SIZE, (u4VDecReadAVCVLD(u4VDecID, RW_AVLD_PIC_SIZE) &(~0x1FFFF)) | (u4SHDRInfo&0x1FFF));  
    vVDecWriteAVCVLD(u4VDecID, RW_AVLD_SHDR_1, (u4VDecReadAVCVLD(u4VDecID, RW_AVLD_SHDR_1) &(~0xFFFFE000)) | (u4SHDRInfo&0xFFFFE000));  
    #else
    vVDecWriteAVCVLD(u4VDecID, RW_AVLD_SHDR_1, u4SHDRInfo);  
    #endif
}


// **************************************************************************
// Function : INT32 i4VDEC_HAL_H264_FGTSetting(UINT32 u4VDecID, VDEC_INFO_H264_FGT_PRM_T *prFGTPrm);
// Description :Set film gram hardware registers
// Parameter : prFGTPrm : pointer to H264 film gram info struct
// Return      : =0: success.
//                  <0: fail.
// **************************************************************************
INT32 i4VDEC_HAL_H264_FGTSetting(UINT32 u4VDecID, VDEC_INFO_H264_FGT_PRM_T *prFGTPrm)
{
  // Trun off then on to reset FGT module
  //vVDecWriteAVCFG(RW_FGT_MODE, FGT_SCR_PP);

    vVDecWriteAVCFG(u4VDecID, RW_FGT_MODE, 0);
    vVDecWriteAVCFG(u4VDecID, RW_FGT_MODE, prFGTPrm->ucDataScr);

    if (prFGTPrm->ucDataScr)
    {
        vVDecWriteAVCFG(u4VDecID, RW_FGT_DRAM_CTRL, 0x0100); 
#if CONFIG_DRV_VIRTUAL_ADDR
        vVDecWriteAVCFG(u4VDecID, RW_FGT_OUT_Y_ADDR, PHYSICAL((UINT32) prFGTPrm->pucFGTTrgYAddr)>>4);
        vVDecWriteAVCFG(u4VDecID, RW_FGT_OUT_C_ADDR, PHYSICAL((UINT32) prFGTPrm->pucFGTTrgCAddr)>>4);
        vVDecWriteAVCFG(u4VDecID, RW_FGT_IN_Y_ADDR, PHYSICAL((UINT32) prFGTPrm->pucFGTScrYAddr)>>4);
        vVDecWriteAVCFG(u4VDecID, RW_FGT_IN_C_ADDR, PHYSICAL((UINT32) prFGTPrm->pucFGTScrCAddr)>>4);  
#else
        vVDecWriteAVCFG(u4VDecID, RW_FGT_OUT_Y_ADDR, u4AbsDramANc((UINT32) prFGTPrm->pucFGTTrgYAddr)>>4);
        vVDecWriteAVCFG(u4VDecID, RW_FGT_OUT_C_ADDR, u4AbsDramANc((UINT32) prFGTPrm->pucFGTTrgCAddr)>>4);
        vVDecWriteAVCFG(u4VDecID, RW_FGT_IN_Y_ADDR, u4AbsDramANc((UINT32) prFGTPrm->pucFGTScrYAddr)>>4);
        vVDecWriteAVCFG(u4VDecID, RW_FGT_IN_C_ADDR, u4AbsDramANc((UINT32) prFGTPrm->pucFGTScrCAddr)>>4);  
#endif
        vVDecWriteAVCFG(u4VDecID, RW_FGT_MB_SIZE, (prFGTPrm->ucMBYSize << 8) +prFGTPrm->ucMBXSize);    
        vVDecWriteAVCFG(u4VDecID, RW_FGT_SEI_CTRL_A, prFGTPrm->u4Ctrl);
    }
    return HAL_HANDLE_OK;
}


// **************************************************************************
// Function : INT32 i4VDEC_HAL_H264_DecStart(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm);
// Description :Set video decoder hardware registers to decode for H264
// Parameter : ptHalDecH264Info : pointer to H264 decode info struct
// Return      : =0: success.
//                  <0: fail.
// **************************************************************************
INT32 i4VDEC_HAL_H264_DecStart(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm)
{
    UINT32 u4SHDRInfo;
    UINT32 u4ResultReg;
    BOOL fgMbAdapFrameFieldFlag= 0;
    BOOL fgFwFieldPicFlag= 0;
    BOOL fgMBAFF= 0;
    BOOL fgEntropyMode= 0;
    UINT32 u4PicWidthInMB;
    UINT32 u4PredRdDramSize;
    UINT32 u4PredRdDramStart;
    UINT32 u4PredRdDramEnd;
    UINT32 u4Reg;

#if VDEC_DDR3_SUPPORT
    UINT32 u4DDR3_PicWdith;
    UINT32 aMc406;
#endif

#if (!CONFIG_DRV_VERIFY_SUPPORT)
    UINT32  aAVCVLD84 = 0;
#endif    
    
#if ((CONFIG_DRV_VERIFY_SUPPORT) && (VDEC_VER_COMPARE_CRC))
    UINT32 u4CRCSrc = 0x1; 
#endif
    
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    UINT32 u4total_mbs_in_pic = 0;
#endif

    UINT32 u4FieldDistance = 4;
#if (CONFIG_DRV_VERIFY_SUPPORT) && (!VDEC_DRV_PARSER)
    VDEC_INFO_H264_DEC_PRM_T *prH264DecPrm = (VDEC_INFO_H264_DEC_PRM_T *) &(prDecPrm->SpecDecPrm.rVDecH264DecPrm);
    //prH264DecPrm->prCurrFBufInfo->u4YStartAddr = prH264DecPrm->u4DecWorkBuf;
    prH264DecPrm->prCurrFBufInfo->u4CAddrOffset = prH264DecPrm->prCurrFBufInfo->u4DramPicSize;
    #if (!VDEC_H264_REDUCE_MV_BUFF)
    prH264DecPrm->prCurrFBufInfo->u4MvStartAddr = (prH264DecPrm->prCurrFBufInfo->u4YStartAddr + ((prH264DecPrm->prCurrFBufInfo->u4DramPicSize * 3) >>1));
    #endif
#else
    VDEC_INFO_H264_DEC_PRM_T *prH264DecPrm = (VDEC_INFO_H264_DEC_PRM_T *)prDecPrm->prVDecCodecHalPrm;
#endif  
    
#if (CONFIG_DRV_VERIFY_SUPPORT)
#if VDEC_H264_REDUCE_MV_BUFF
     if (prH264DecPrm->fgIsReduceMVBuffer)
     {
        vVDecWriteAVCMV(u4VDecID, RW_AMV_REDUCE_BMV,  (u4VDecReadAVCMV(u4VDecID, RW_AMV_REDUCE_BMV) | EN_AMV_REDUCE_BMV ));
	#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560)
	u4FieldDistance= 2;  
	#else
        u4FieldDistance= 1;  
	#endif
     }
#endif
#endif

#if (CONFIG_DRV_VERIFY_SUPPORT)
#if 0//VDEC_MVC_ALLEGRO
    if (prH264DecPrm->fgIsAllegMvcCfg)
    {
        //vVDecWriteAVCMV(u4VDecID, RW_AMV_REDUCE_BMV,  (u4VDecReadAVCMV(u4VDecID, RW_AMV_REDUCE_BMV) | EN_AMV_ALLEG_MVC_CFG ));
    }
#endif
#endif

#if ((CONFIG_DRV_VERIFY_SUPPORT) && (VDEC_VER_COMPARE_CRC))
    //u4CRCSrc = (VDEC_CRC_EN | VDEC_CRC_SRC_MC);   //CRC input from MC
    u4CRCSrc = (VDEC_CRC_EN | VDEC_CRC_SRC_PP);  //CRC input from PP
    vVDecWriteCRC(u4VDecID, 0x4, u4CRCSrc);
#endif


    // set video down scaler parameter
    vVDECSetDownScalerPrm(u4VDecID, &prDecPrm->rDownScalerPrm);

#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560) && CONFIG_DRV_FTS_SUPPORT)
    // set letterbox detection parameter
    vVDECSetLetetrBoxDetPrm(u4VDecID, &prDecPrm->rLBDPrm);
#endif

#if VDEC_FIELD_COMPACT
#else
    // addr swap mode
    vVDecWriteMC(u4VDecID, RW_MC_ADDRSWAP, prDecPrm->ucAddrSwapMode);
#endif
    
#if (CONFIG_DRV_VERIFY_SUPPORT)
  #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    vVDecWriteVLDTOP(u4VDecID, RW_VLD_TOP_BUSY_THRESHOLD, 0xFFFFFFFF);
  #else
    vVDecWriteAVCVLD(u4VDecID, RW_AVLD_MC_BUSY_THRESHOLD, 0x1000);
  #endif
  #else
    // set MC wait timeout threshold
  #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    vVDecWriteVLDTOP(u4VDecID, RW_VLD_TOP_BUSY_THRESHOLD, 0xFFFFFFFF);
  #else
    vVDecWriteAVCVLD(u4VDecID, RW_AVLD_MC_BUSY_THRESHOLD, 0x8000);
  #endif
  #endif
    // Fld & Frame height same?
    vVDecWriteMC(u4VDecID, RW_MC_UMV_PIC_WIDTH, prDecPrm->u4PicW);
    vVDecWriteMC(u4VDecID, RW_MC_UMV_PIC_HEIGHT, prDecPrm->u4PicH);  
  
#if (!CONFIG_DRV_VERIFY_SUPPORT)
    if(prH264DecPrm->ucECLevel == 0)
    {
         // Default No EC, Mask for selected types
        vVDecWriteAVCVLD(u4VDecID, RW_AVLD_ERR_MASK, ~(CABAC_ZERO_WORD_ERR));
    }
    else
    {
        //vVDecWriteAVCVLD(u4VDecID, RW_AVLD_CTRL, u4VDecReadAVCVLD(u4VDecID, RW_AVLD_CTRL) | AVC_ERR_CONCEALMENT);        
        aAVCVLD84 = (u4VDecReadAVCVLD(u4VDecID, RW_AVLD_CTRL) | AVC_ERR_CONCEALMENT);
        aAVCVLD84 |= AVC_SUM6_APPEND_INV;

 #ifdef MEM_PAUSE_SUPPORT
        aAVCVLD84 |= AVC_NOT_CHK_DATA_VALID;
 #else
        aAVCVLD84 &= (0x7FFFFFFF);
 #endif

       vVDecWriteAVCVLD(u4VDecID, RW_AVLD_CTRL, aAVCVLD84);
        
        vVDecWriteAVCVLD(u4VDecID, RW_AVLD_ERR_MASK,  (~(CABAC_ALIGN_BIT_ERR | CABAC_ZERO_WORD_ERR)));
    }
#else
    vVDecWriteAVCVLD(u4VDecID, RW_AVLD_ERR_MASK, (~(AVLD_MB_END_CHK |AVLD_4BLOCKS_SKIP_CHK)) );    
#endif    
    
    // turn of DRAM wrapper when  current frame is Mono
#if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8560)
    if (prH264DecPrm->prSPS->u4ChromaFormatIdc == 0)
    {
        vVDecWriteMC(u4VDecID, RW_MC_WRAPPER_SWITCH, 1);
    }
    else
    {
        vVDecWriteMC(u4VDecID, RW_MC_WRAPPER_SWITCH, 0);
    }
#endif
    // Only one case needs to turn off deblocking
    if(0)//prH264DecPrm->prSliceHdr->u4DisableDeblockingFilterIdc == 1)
    {
        //test only
        vVDecWriteMC(u4VDecID, RW_MC_PP_ENABLE, 0);  
        vVDecWriteMC(u4VDecID, RW_MC_PP_WB_BY_POST, 0);  
        vVDecWriteMC(u4VDecID, RW_MC_PP_DBLK_MODE, 0);            
    }
    else
    {
        vVDecWriteMC(u4VDecID, RW_MC_PP_ENABLE, 1);  
#if CONFIG_DRV_VIRTUAL_ADDR
        vVDecWriteMC(u4VDecID, RW_MC_PP_Y_ADDR, PHYSICAL((UINT32) prH264DecPrm->prCurrFBufInfo->u4YStartAddr) >> 9);  
        vVDecWriteMC(u4VDecID, RW_MC_PP_C_ADDR, PHYSICAL((UINT32) (prH264DecPrm->prCurrFBufInfo->u4YStartAddr + prH264DecPrm->prCurrFBufInfo->u4CAddrOffset)) >> 8);  
#else
        vVDecWriteMC(u4VDecID, RW_MC_PP_Y_ADDR, u4AbsDramANc((UINT32) prH264DecPrm->prCurrFBufInfo->u4YStartAddr) >> 9);  
        vVDecWriteMC(u4VDecID, RW_MC_PP_C_ADDR, u4AbsDramANc((UINT32) (prH264DecPrm->prCurrFBufInfo->u4YStartAddr + prH264DecPrm->prCurrFBufInfo->u4CAddrOffset)) >> 8);  
#endif
        vVDecWriteMC(u4VDecID, RW_MC_PP_WB_BY_POST, 1); 
        vVDecWriteMC(u4VDecID, RW_MC_PP_DBLK_MODE, DBLK_Y | DBLK_C);            
        vVDecWriteMC(u4VDecID, RW_MC_PP_X_RANGE, ((prDecPrm->u4PicW + 15)>> 4) - 1);    
        vVDecWriteMC(u4VDecID, RW_MC_PP_Y_RANGE, (((prDecPrm->u4PicH >> (1-(prH264DecPrm->fgIsFrmPic))) + 15)>> 4) - 1);    
        vVDecWriteMC(u4VDecID, RW_MC_PP_MB_WIDTH, ((prDecPrm->u4PicW + 15)>> 4));            
    }

#if (CONFIG_DRV_FPGA_BOARD)
  vVDecWriteMC(u4VDecID, RW_MC_MODE_CTL, MC_QIU_BANK4|MC_QIU_BANK8|MC_DRAM_REQ_DELAY_1T|MC_DRAM_REQ_MERGE_OFF|MC_MV_MERGE_OFF);
#endif

#if CONFIG_DRV_VIRTUAL_ADDR
    vVDecWriteMC(u4VDecID, RW_AMC_Y_OUT_ADDR, PHYSICAL((UINT32) prH264DecPrm->prCurrFBufInfo->u4YStartAddr));
#else
    vVDecWriteMC(u4VDecID, RW_AMC_Y_OUT_ADDR, u4AbsDramANc((UINT32) prH264DecPrm->prCurrFBufInfo->u4YStartAddr));
#endif
    vVDecWriteMC(u4VDecID, RW_AMC_CBCR_OFFSET, prH264DecPrm->prCurrFBufInfo->u4CAddrOffset);  
  
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)  
  #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
  vVDecWriteMC(u4VDecID, RW_MC_PIC_W_MB, ((prDecPrm->u4PicBW + 15)>> 4));  
  #else
  vVDecWriteVLD(u4VDecID, RW_VLD_PIC_W_MB, ((prDecPrm->u4PicBW + 15)>> 4));  
  #endif

 #ifdef MEM_PAUSE_SUPPORT
  // Turn off find start code function
  vVDecWriteAVCVLD(u4VDecID, RW_AVLD_RM03R, ((u4VDecReadAVCVLD(u4VDecID,  RW_AVLD_RM03R)) & 0xFFFFFFDF) );    
  //vVDecWriteAVCVLD(u4VDecID, RW_AVLD_RM03R, ((u4VDecReadAVCVLD(u4VDecID,  RW_AVLD_RM03R)) & 0xFFFFFCDF) );
 #else
   vVDecWriteAVCVLD(u4VDecID, RW_AVLD_RM03R, ((u4VDecReadAVCVLD(u4VDecID,  RW_AVLD_RM03R)) & 0xFFFFFCDD) );
 #endif
 
#endif

#if CONFIG_DRV_VIRTUAL_ADDR
    vVDecWriteAVCMV(u4VDecID, RW_AMV_WR_ADDR, 
                          ((PHYSICAL((UINT32) prH264DecPrm->prCurrFBufInfo->u4MvStartAddr) >> 4)) + ((prDecPrm->ucPicStruct == BOTTOM_FIELD)? u4FieldDistance : 0));
#else
    vVDecWriteAVCMV(u4VDecID, RW_AMV_WR_ADDR, 
                          ((u4AbsDramANc((UINT32) prH264DecPrm->prCurrFBufInfo->u4MvStartAddr) >> 4)) + ((prDecPrm->ucPicStruct == BOTTOM_FIELD)? u4FieldDistance : 0));
#endif
  
#if VDEC_MVC_SUPPORT
    vVDecWriteAVCMV(u4VDecID, RW_AMV_MVC_CTRL, (u4VDecReadAVCMV(u4VDecID,  RW_AMV_MVC_CTRL)) | FW_NO_MV_MVC);
#endif

    //SetSHDRAVLD2  
    u4SHDRInfo = (prH264DecPrm->prSliceHdr->i4SliceQpDelta) & 0x7f;
    u4SHDRInfo |= ((prH264DecPrm->prSliceHdr->u4DisableDeblockingFilterIdc & 0x3) << 7);
    u4SHDRInfo |= ((prH264DecPrm->prSliceHdr->i4SliceAlphaC0OffsetDiv2 & 0xf) << 9);  
    u4SHDRInfo |= ((prH264DecPrm->prSliceHdr->i4SliceBetaOffsetDiv2 & 0xf) << 13);  
    u4SHDRInfo |= ((prH264DecPrm->ucNalRefIdc & 0x3) << 17);  
    u4SHDRInfo |= (prH264DecPrm->fgIsIDRPic << 19);  
    u4SHDRInfo |= ((prH264DecPrm->prSliceHdr->u4CabacInitIdc & 0x3) << 20);
  
    vVDecWriteAVCVLD(u4VDecID, RW_AVLD_SHDR_2, u4SHDRInfo);
    if (prH264DecPrm->prPPS->fgEntropyCodingModeFlag) // CABAC only
    {
        vVDecWriteAVCVLD(u4VDecID, RW_AVLD_INIT_CTX_SRAM, 0x1);  
    }
  
  #ifdef MEM_PAUSE_SUPPORT
    while(u4VDecReadAVCVLD(u4VDecID, RO_AVLD_STATUS) & RO_AVLD_STALL);
  #endif
  
    #ifdef MPV_DUMP_H264_DEC_REG
    VDec_DumpH264Reg(u4VDecID);
    #endif
    #ifdef MPV_DUMP_H264_CHKSUM
    vVDEC_HAL_H264_VDec_ReadCheckSum1(u4VDecID);
    #endif

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
    vVDecWriteMC(0, 0x5E4, (u4VDecReadMC(0, 0x5E4) |(0x1 <<12)) );

    #if (!VDEC_8320_SUPPORT)
    vVDecWriteMC(0, 0x660, (u4VDecReadMC(0, 0x660) |(0x80000000)) );
    #endif
    
    #ifndef VDEC_PIP_WITH_ONE_HW
    vVDecWriteMC(1, 0x5E4, (u4VDecReadMC(1, 0x5E4) |(0x1 <<12)) );
    #if (!VDEC_8320_SUPPORT)
    vVDecWriteMC(1, 0x660, (u4VDecReadMC(1, 0x660) |(0x80000000)) );    
    #endif
    #endif
#endif

#if VDEC_FIELD_COMPACT
    printk( "[H264] Enable Field Compact Mode\n");
    DBG_H264_PRINTF("[H264] Enable Field Compact Mode\n");
    //vVDecWriteAVSPP(u4VDecID, 0x3C, (u4VDecReadAVSPP(u4VDecID, 0x3C)& 0xEFFFFFFF));
    vWriteReg(AVS_PP_REG_OFFSET0 + 0x3C, (u4ReadReg(AVS_PP_REG_OFFSET0 + 0x3C) & 0xEFFFFFFF));
    vVDecWriteMC(u4VDecID, 0x920, (u4VDecReadMC(u4VDecID, 0x920)  & 0xFEFFFFFF)); 
#else
    #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8550)
     vVDecWriteMC(u4VDecID, RW_MC_NBM_CTRL,  
             ((u4VDecReadMC(u4VDecID, RW_MC_NBM_CTRL)  & 0xFFFFFFF8) |prDecPrm->ucAddrSwapMode));
    #endif 
#endif

// turn off test mode 0x834[4]
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8560)
     vVDecWriteMC(u4VDecID, RW_MC_DDR3_EN, (u4VDecReadMC(u4VDecID, RW_MC_DDR3_EN)  & 0xFFFFFFEF));
#endif

    if(prDecPrm->u4PicBW < 176)
    {
    	vVDecWriteDV(u4VDecID, VDEC_DV_LAT_BUF_BYPASS, u4VDecReadDV(u4VDecID, VDEC_DV_LAT_BUF_BYPASS) |0x1);
        #if 1  //for 6582
        
        #endif
    }
    else
    {
    	vVDecWriteDV(u4VDecID, VDEC_DV_LAT_BUF_BYPASS, u4VDecReadDV(u4VDecID, VDEC_DV_LAT_BUF_BYPASS) & 0xFFFFFFFE);
    }


#if VDEC_DDR3_SUPPORT
      u4DDR3_PicWdith = (((prDecPrm->u4PicBW + 63) >> 6) << 2);
      vVDecWriteMC(u4VDecID, RW_MC_PP_MB_WIDTH, u4DDR3_PicWdith);  
      #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
      vVDecWriteMC(u4VDecID, RW_MC_PIC_W_MB, u4DDR3_PicWdith);  
      #else
      vVDecWriteVLD(u4VDecID, RW_VLD_PIC_W_MB, u4DDR3_PicWdith);                  
      #endif

     //Turn on DDR3 mode
     vVDecWriteMC(u4VDecID, RW_MC_DDR_CTRL0, 
               ((u4VDecReadMC(u4VDecID, RW_MC_DDR_CTRL0)  & 0xFFFFFFFE) |0x1));

     vVDecWriteMC(u4VDecID, RW_MC_DDR_CTRL1, 
               ((u4VDecReadMC(u4VDecID, RW_MC_DDR_CTRL1)  & 0xFFFFFFFE) |0x1));

     aMc406 = u4VDecReadMC(u4VDecID, (406<<2));
     aMc406 &= 0xFFFFFFEF;
     vVDecWriteMC(u4VDecID, (406<<2), aMc406);
     
     //Turn-on DDR3, Set 0x834[0] = 0
     vVDecWriteMC(u4VDecID, RW_MC_DDR3_EN, (u4VDecReadMC(u4VDecID, RW_MC_DDR3_EN)  & 0xFFFFFFFE));
     
    //Post-process enable
    vVDecWriteMC(u4VDecID, RW_MC_PP_ENABLE, (u4VDecReadMC(u4VDecID, RW_MC_PP_ENABLE)  | 0x1));

    //Writeback by PP
    vVDecWriteMC(u4VDecID, RW_MC_PP_WB_BY_POST, 0x00000001);
#endif

#if CONFIG_DRV_VERIFY_SUPPORT
    //vVDEC_HAL_H264_VDec_DumpReg(u4VDecID, TRUE);
#endif

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
if (prH264DecPrm->prSPS->fgFrameMbsOnlyFlag == 0 && prH264DecPrm->prSliceHdr->fgFieldPicFlag == 0)
     {
        u4total_mbs_in_pic = ((prH264DecPrm->prSPS->u4PicHeightInMapUnitsMinus1<<1) + 2)*(prH264DecPrm->prSPS->u4PicWidthInMbsMinus1 + 1); 
     }
     else
     {
        u4total_mbs_in_pic = (prH264DecPrm->prSPS->u4PicHeightInMapUnitsMinus1 + 1)*(prH264DecPrm->prSPS->u4PicWidthInMbsMinus1 + 1); 
     }
     
  vVDecWriteVLDTOP(u4VDecID, RW_VLD_TOP_TOTAL_MBS_IN_PIC, u4total_mbs_in_pic);  
  vVDecWriteVLDTOP(u4VDecID, RO_VLD_TOP_TIMEOUT_THD, VLD_TOP_TIMEOUT_THD);
#endif

#if 0//(CONFIG_DRV_VERIFY_SUPPORT) && (CONFIG_DRV_LINUX)    
    HalFlushInvalidateDCache();
#endif

#if 1 //(AVC_8320_SUPPORT)
    #if AVC_LOG_TMP
    printk("H264_DecStart, u4VLDWrapperWrok = 0x%x\n", _tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecH264DecPrm.u4VLDWrapperWrok);
    printk("H264_DecStart, u4PPWrapperWrok = 0x%x\n", _tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecH264DecPrm.u4PPWrapperWrok);
    #endif
    
    vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER, PHYSICAL(_tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecH264DecPrm.u4VLDWrapperWrok));
    //vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER, 0x00200000);
    //DBG_H264_PRINTF("MC_648 = 0x%08x\n", 0x00200000);
#if MT6582_L2_EMULATION == 1
    vVDecWriteMC(u4VDecID, RW_MC_PP_WRAPPER, PHYSICAL(_pucPredSa[u4VDecID]) + 0x5A00);
#elif MT6582_L2_EMULATION == 2
    vVDecWriteMC(u4VDecID, RW_MC_PP_WRAPPER, 0x00205A00);
#else
    vVDecWriteMC(u4VDecID, RW_MC_PP_WRAPPER, PHYSICAL(_tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecH264DecPrm.u4PPWrapperWrok));
#endif

#endif

    //for 6582 PP & VLD Wrapper setting
    #if 0 //for 6582
    //FPGA PP Wrapper setting
    //vVDecWriteVLDTOP(u4VDecID, 0x28, 0x10000000);
    //vVDecWriteMC(u4VDecID, 0xA20, 0x10000000);
    //vVDecWriteMC(u4VDecID, 0xA24, (0x10000000 + (1920*12)));

    //no by pass mode
    //FPGA VLD Wrapper setting
    //pp_reg_513[4] = 1'b0
    u4ResultReg = u4VDecReadPP(u4VDecID, 0x804);
    u4ResultReg &= 0xFFFFFFEF;
    vVDecWritePP(u4VDecID, 0x804 , u4ResultReg);
    //setting for vld_top_reg_37

    u4ResultReg = u4VDecReadAVCVLD(u4VDecID, 0x88);
    fgMbAdapFrameFieldFlag = (u4ResultReg & 0x00080000) >> 19;

    u4ResultReg = u4VDecReadAVCVLD(u4VDecID, 0x98);
    fgFwFieldPicFlag = (u4ResultReg & 0x00020000) >> 17;

    fgMBAFF = fgMbAdapFrameFieldFlag & (~fgFwFieldPicFlag);

    u4ResultReg = u4VDecReadAVCVLD(u4VDecID, 0x90);
    fgEntropyMode= (u4ResultReg & 0x00000001);

    DBG_H264_PRINTF("{Info} MBAFF Mode = 0x%x, Entropy Mode: 0x%08x\n", fgMBAFF, fgEntropyMode);

    u4ResultReg = u4VDecReadVLDTOP(u4VDecID, 0x68);

    u4PicWidthInMB = (u4ResultReg & 0x00000FFF) + 1;

    if(fgMBAFF){
        if(fgEntropyMode){
            u4PredRdDramSize = u4PicWidthInMB * 12;
        }
        else{
            u4PredRdDramSize = u4PicWidthInMB * 10;            
        }
    }
    else{
        if(fgEntropyMode){
            u4PredRdDramSize = u4PicWidthInMB * 6;
        }
        else{
            u4PredRdDramSize = u4PicWidthInMB * 5;
        }
    }

    u4ResultReg = u4VDecReadVLDTOP(u4VDecID, 0x28);
    u4PredRdDramStart = u4ResultReg;
    //u4PredRdDramEnd = (u4ResultReg >> 4) + u4PredRdDramSize - 1;
    u4PredRdDramEnd = (u4ResultReg) + ((u4PredRdDramSize - 1) << 4);

    DBG_H264_PRINTF("{Info} PredDram Start = 0x%x, PredDram End = 0x%08x, size %d\n", u4PredRdDramStart, u4PredRdDramEnd, u4PredRdDramSize);

    vVDecWriteVLDTOP(u4VDecID, 0x94, u4PredRdDramEnd);
    //vVDecWriteVLDTOP(u4VDecID, 51*4, 0); // Turn on debug

    #if 1
    if(prDecPrm->u4PicBW <= 176)
    {
        //Vld_top_11[30] = 1'b1 //wld wrapper bypass enable
        u4ResultReg = u4VDecReadVLDTOP(u4VDecID, 0x2C);
        u4ResultReg |= 0x40000000;
        vVDecWriteVLDTOP(u4VDecID, 0x2C , u4ResultReg);

        #if 1
        //Vld_top_16[5:4] = 2'b2 // WCP2 dram burst mode
        u4ResultReg = u4VDecReadVLDTOP(u4VDecID, 0x40);
        u4ResultReg &= 0xFFFFFFEF;
        u4ResultReg |= 0x00000020;
        vVDecWriteVLDTOP(u4VDecID, 0x40 , u4ResultReg);
        #else
        //Vld_top_16[5:4] = 2'b0 // dram not burst mode
        u4ResultReg = u4VDecReadVLDTOP(u4VDecID, 0x40);
        u4ResultReg &= 0xFFFFFFCF;
        vVDecWriteVLDTOP(u4VDecID, 0x40 , u4ResultReg);
        #endif
        
    }
    else{
        //vVDecWriteVLDTOP(u4VDecID, 0x2C , u4VDecReadVLDTOP(u4VDecID, 0x2C) | (0x1 << 29) | (0x1 << 26));
        //not by pass mode
        //Vld_top_11[29] = 1'b1 //wld wrapper bypass enable
        //u4ResultReg = u4VDecReadVLDTOP(u4VDecID, 0x2C);
        //u4ResultReg |= 0x20000000;
        //vVDecWriteVLDTOP(u4VDecID, 0x2C , u4ResultReg);
    }
    #endif

    #endif

    u4Reg = u4VDecReadAVCVLD(u4VDecID, RO_AVLD_BARL);
    printk("{H264 DEC} u4InstID = 0x%x, Input Window: 0x%08x\n", u4VDecID, u4Reg);
    DBG_H264_PRINTF("{H264 DEC} u4InstID = 0x%x, Input Window: 0x%08x\n", u4VDecID, u4Reg);

    vVDecWriteAVCVLD(u4VDecID, RW_AVLD_PROC, 0x1); 
  
    return HAL_HANDLE_OK;
}


// **************************************************************************
// Function : void v4VDEC_HAL_H264_GetMbxMby(UINT32 u4VDecID, UINT32 *pu4Mbx, UINT32 *pu4Mby);
// Description :Read current decoded mbx and mby
// Parameter : u4VDecID : video decoder hardware ID
//                 u4Mbx : macroblock x value
//                 u4Mby : macroblock y value
// Return      : None
// **************************************************************************
void vVDEC_HAL_H264_GetMbxMby(UINT32 u4VDecID, UINT32 *pu4Mbx, UINT32 *pu4Mby)
{
    *pu4Mbx = u4VDecReadMC(u4VDecID, RO_MC_MBX);
    *pu4Mby = u4VDecReadMC(u4VDecID, RO_MC_MBY);
}


// **************************************************************************
// Function : BOOL fgVDEC_HAL_H264_DecPicComplete(UINT32 u4VDecID);
// Description :Check if all video decoder modules are complete
// Parameter : u4VDecID : video decoder hardware ID
// Return      : TRUE: Decode complete, FALSE: Not yet
// **************************************************************************
BOOL fgVDEC_HAL_H264_DecPicComplete(UINT32 u4VDecID)
{
    if(u4VDecReadAVCVLD(u4VDecID, RO_AVLD_STATE_INFO) & AVLD_PIC_FINISH)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

// **************************************************************************
// Function : void u4VDEC_HAL_H264_GetErrMsg(UINT32 u4VDecID);
// Description :Read h264 error message after decoding end
// Parameter : u4VDecID : video decoder hardware ID
// Return      : H264 decode error message
// **************************************************************************
UINT32 u4VDEC_HAL_H264_GetErrMsg(UINT32 u4VDecID)
{
    //return u4VDecReadAVCVLD(u4VDecID, RO_AVLD_ERR_MESSAGE);
    u4VDecReadAVCVLD(u4VDecID, RO_AVLD_ERR_MESSAGE);
    u4VDecReadAVCVLD(u4VDecID, RW_AVLD_ERR_MASK);
    return u4VDecReadAVCVLD(u4VDecID, RO_AVLD_ERR_ACCUMULATOR);
}


// **************************************************************************
// Function : void u4VDEC_HAL_H264_GetErrMsg(UINT32 u4VDecID);
// Description :Read h264 error message after decoding end
// Parameter : u4VDecID : video decoder hardware ID
// Return      : H264 decode error message
// **************************************************************************
BOOL fgVDEC_HAL_H264_ChkErrInfo(UINT32 ucBsId, UINT32 u4VDecID, UINT32 u4DecErrInfo, UINT32 u4ECLevel)
{
    UINT32 u4Data; 
    BOOL fgIsVDecErr;
    
    fgIsVDecErr = TRUE;

    switch(u4ECLevel)
    {
        case 2:
            // Ignore the real non-NextStartCode condition
            if( (u4DecErrInfo == (CABAC_ZERO_WORD_ERR|NO_NEXT_START_CODE))
            // Add For CQ: 31166, 31113 Customer_B_B_K: AVCHD Disc            
            || (u4DecErrInfo == (CABAC_ZERO_WORD_ERR))
            )            	
            {
                fgIsVDecErr = FALSE;
            }
        case 0:
        case 1:
        default:
            if (u4DecErrInfo == CABAC_ZERO_WORD_ERR)
            {
                vVDEC_HAL_H264_TrailingBits(ucBsId, u4VDecID);
                u4Data = u4VDEC_HAL_H264_ShiftGetBitStream(ucBsId, u4VDecID, 0);
                if(((u4Data >> 8) == START_CODE) || (u4Data == 0x00000000) || (u4Data == START_CODE))
                {
                    fgIsVDecErr = FALSE;
                }
            }
            else if (u4DecErrInfo == NO_NEXT_START_CODE) // don't care "No next start code"
            {
                fgIsVDecErr = FALSE;
            }
            else if ((u4DecErrInfo == CABAC_ALIGN_BIT_ERR) && (!(u4VDecReadAVCVLD(u4VDecID, RW_AVLD_ERR_MASK) & CABAC_ALIGN_BIT_ERR))) // don't care "No next start code"
            {
                fgIsVDecErr = FALSE;
            } 
            break;
    }
    
    return fgIsVDecErr;
}

#if VDEC_MVC_SUPPORT
// **************************************************************************
// Function : void vVDEC_HAL_H264_SetInterViewPRefPicListReg(UINT32 u4VDecID, VDEC_INFO_H264_P_REF_PRM_T *prPRefPicListInfo);
// Description :Set HW registers related with InterView Ref Pic
// Parameter : u4VDecID : video decoder hardware ID
//                 prPRefPicListInfo : pointer to information of p reference list
// Return      : None
// **************************************************************************
void vVDEC_HAL_H264_MVC_Switch(UINT32 u4VDecID, BOOL fgIsMVCDec)
{
    if(fgIsMVCDec)
    {
        vVDecWriteAVCVLD(u4VDecID, RW_AVLD_RM03R, u4VDecReadAVCVLD(u4VDecID,  RW_AVLD_RM03R) | (MVC_SWITCH | HEADER_EXT_SWITCH | REORDER_MVC_SWITCH));    
    }
    else
    {
        vVDecWriteAVCVLD(u4VDecID, RW_AVLD_RM03R, u4VDecReadAVCVLD(u4VDecID,  RW_AVLD_RM03R) & (~(MVC_SWITCH | HEADER_EXT_SWITCH | REORDER_MVC_SWITCH)));    
    }
    vVDecWriteAVCVLD(u4VDecID, RW_AVLD_RM03R, u4VDecReadAVCVLD(u4VDecID,  RW_AVLD_RM03R) & (~VIEW_REORDER_SWITCH));    

}

// **************************************************************************
// Function : void vVDEC_HAL_H264_SetInterViewPRefPicListReg(UINT32 u4VDecID, VDEC_INFO_H264_P_REF_PRM_T *prPRefPicListInfo);
// Description :Set HW registers related with InterView Ref Pic
// Parameter : u4VDecID : video decoder hardware ID
//                 prPRefPicListInfo : pointer to information of p reference list
// Return      : None
// **************************************************************************
void vVDEC_HAL_H264_SetInterViewPRefPicListReg(UINT32 u4VDecID, VDEC_INFO_H264_P_REF_PRM_T *prPRefPicListInfo)
{
    UCHAR ucFBufIdx;
    UCHAR ucFld;  
    UCHAR ucRegIdx;
    UINT32 u4Temp;
    UINT32 u4Param;
    
    ucFld = (prPRefPicListInfo->u4FBufInfo & 0xff);  
    ucRegIdx = ((prPRefPicListInfo->u4FBufInfo >> 16) & 0xff);  
    ucFBufIdx = prPRefPicListInfo->ucFBufIdx;
  
    if (ucFld == FRAME)
    {    
        //bRegIdx = bRefPicIdx;
#if CONFIG_DRV_VIRTUAL_ADDR
        vVDecWriteMC(u4VDecID, RW_MC_P_LIST0 + (ucRegIdx <<2), PHYSICAL((UINT32)  prPRefPicListInfo->u4FBufYStartAddr));    
#else
        vVDecWriteMC(u4VDecID, RW_MC_P_LIST0 + (ucRegIdx <<2), u4AbsDramANc((UINT32)  prPRefPicListInfo->u4FBufYStartAddr));    
#endif
         //vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_P0_RPL + (ucRegIdx<<2), (1 << 19) + (2 << 20) + (prPRefPicListInfo->u4ViewId & 0x7ffff));
         u4Param = (1 << 19) + (2 << 20) + (prPRefPicListInfo->u4ViewId & 0x7ffff);
         vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_P0_RPL + (ucRegIdx<<2), u4Param);
          //printk("SetInterPRefPicList1: [0x%X] = 0x%X\n", RW_AVLD_REORD_P0_RPL + (ucRegIdx<<2),  u4Param);

        u4Temp = (((prPRefPicListInfo->i4TFldPOC <= prPRefPicListInfo->i4BFldPOC) && (prPRefPicListInfo->i4TFldPOC != 0x7fffffff))? 1: 0) << 21;    
        u4Temp |= ((ucFBufIdx)<<1<<22) + prPRefPicListInfo->u4TFldPara;
        vVDecWriteAVCMV(u4VDecID, RW_AMV_P_REF_PARA + (ucRegIdx<<3), u4Temp);
    
        u4Temp = (((prPRefPicListInfo->i4TFldPOC <= prPRefPicListInfo->i4BFldPOC) && (prPRefPicListInfo->i4TFldPOC != 0x7fffffff))? 1: 0) << 21;    
        u4Temp |= ((((ucFBufIdx)<<1) + 1)<<22) + prPRefPicListInfo->u4BFldPara;
        vVDecWriteAVCMV(u4VDecID, RW_AMV_P_REF_PARA + (ucRegIdx<<3) + 4, u4Temp);
    }
    else
    { 
        if (ucFld == TOP_FIELD)
        {
#if CONFIG_DRV_VIRTUAL_ADDR
            vVDecWriteMC(u4VDecID, RW_MC_P_LIST0 + (ucRegIdx<<2), PHYSICAL((UINT32) prPRefPicListInfo->u4FBufYStartAddr));    
#else
            vVDecWriteMC(u4VDecID, RW_MC_P_LIST0 + (ucRegIdx<<2), u4AbsDramANc((UINT32) prPRefPicListInfo->u4FBufYStartAddr));    
#endif
            //vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_P0_RPL + (ucRegIdx<<2), (1 << 19) + (2 << 20) + (prPRefPicListInfo->u4ViewId & 0x7ffff));
            u4Param = (1 << 19) + (2 << 20) + (prPRefPicListInfo->u4ViewId & 0x7ffff);
            vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_P0_RPL + (ucRegIdx<<2), u4Param);
            //printk("SetInterPRefPicList2: [0x%X] = 0x%X\n", RW_AVLD_REORD_P0_RPL + (ucRegIdx<<2),  u4Param);


            u4Temp = (((prPRefPicListInfo->i4TFldPOC <= prPRefPicListInfo->i4BFldPOC) && (prPRefPicListInfo->i4TFldPOC != 0x7fffffff))? 1: 0) << 21;    
            u4Temp |= ((ucFBufIdx)<<1<<22) + prPRefPicListInfo->u4TFldPara;
            vVDecWriteAVCMV(u4VDecID, RW_AMV_P_REF_PARA + (ucRegIdx<<2), u4Temp);
        }
        else
        {        
#if CONFIG_DRV_VIRTUAL_ADDR
            vVDecWriteMC(u4VDecID, RW_MC_P_LIST0 + (ucRegIdx<<2), PHYSICAL((UINT32) prPRefPicListInfo->u4FBufYStartAddr));    
#else
            vVDecWriteMC(u4VDecID, RW_MC_P_LIST0 + (ucRegIdx<<2), u4AbsDramANc((UINT32) prPRefPicListInfo->u4FBufYStartAddr));    
#endif
            //vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_P0_RPL + (ucRegIdx<<2), (1 << 19) + (2 << 20) + (prPRefPicListInfo->u4ViewId & 0x7ffff));

            u4Param = (1 << 19) + (2 << 20) + (prPRefPicListInfo->u4ViewId & 0x7ffff);
            vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_P0_RPL + (ucRegIdx<<2), u4Param);
            //printk("SetInterPRefPicList3: [0x%X] = 0x%X\n", RW_AVLD_REORD_P0_RPL + (ucRegIdx<<2),  u4Param);

            u4Temp = (((prPRefPicListInfo->i4TFldPOC <= prPRefPicListInfo->i4BFldPOC) && (prPRefPicListInfo->i4TFldPOC != 0x7fffffff))? 1: 0) << 21;    
            u4Temp |= (((ucFBufIdx<<1) + 1)<<22) + prPRefPicListInfo->u4BFldPara;
            vVDecWriteAVCMV(u4VDecID, RW_AMV_P_REF_PARA + (ucRegIdx<<2), u4Temp);
            
            vVDecWriteMC(u4VDecID, RW_AMC_P_LIST0_FLD, u4VDecReadMC(u4VDecID, RW_AMC_P_LIST0_FLD) | (0x1 << ucRegIdx));        
        }     
    }
}

// **************************************************************************
// Function : void vVDEC_HAL_H264_SetInterViewB0RefPicListReg(UINT32 u4VDecID, VDEC_INFO_H264_P_REF_PRM_T *prPRefPicListInfo);
// Description :Set HW registers related with InterView Ref Pic
// Parameter : u4VDecID : video decoder hardware ID
//                 prPRefPicListInfo : pointer to information of p reference list
// Return      : None
// **************************************************************************
void vVDEC_HAL_H264_SetInterViewB0RefPicListReg(UINT32 u4VDecID, VDEC_INFO_H264_P_REF_PRM_T *prPRefPicListInfo)
{
    UCHAR ucFBufIdx;
    UCHAR ucFld;  
    UCHAR ucRegIdx;
    UINT32 u4Temp;

    ucFld = (prPRefPicListInfo->u4FBufInfo & 0xff);  
    ucRegIdx = ((prPRefPicListInfo->u4FBufInfo >> 16) & 0xff);  
    ucFBufIdx = prPRefPicListInfo->ucFBufIdx;

    if (ucFld == FRAME)
    {
        // B_0
#if CONFIG_DRV_VIRTUAL_ADDR
        vVDecWriteMC(u4VDecID, RW_MC_B_LIST0 + (ucRegIdx<<2), PHYSICAL((UINT32) prPRefPicListInfo->u4FBufYStartAddr));    
#else
        vVDecWriteMC(u4VDecID, RW_MC_B_LIST0 + (ucRegIdx<<2), u4AbsDramANc((UINT32) prPRefPicListInfo->u4FBufYStartAddr));    
#endif
        vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_B0_RPL + (ucRegIdx<<2), (1 << 19) + (2 << 20) + (prPRefPicListInfo->u4ViewId & 0x7ffff));

        u4Temp = (((prPRefPicListInfo->i4TFldPOC <= prPRefPicListInfo->i4BFldPOC) && (prPRefPicListInfo->i4TFldPOC != 0x7fffffff))? 1: 0) << 21;    
        u4Temp += ((ucFBufIdx)<<1<<22) + prPRefPicListInfo->u4TFldPara + (prPRefPicListInfo->i4TFldPOC & 0x3ffff);
        vVDecWriteAVCMV(u4VDecID, RW_AMV_B0_REF_PARA + (ucRegIdx<<3), u4Temp);

        // Always set Tfld first & !LRef in Interview
        u4Temp = (1 << 21);
        u4Temp += ((((ucFBufIdx)<<1) + 1)<<22) + prPRefPicListInfo->u4BFldPara + (prPRefPicListInfo->i4BFldPOC & 0x3ffff);
        vVDecWriteAVCMV(u4VDecID, RW_AMV_B0_REF_PARA + (ucRegIdx<<3) + 4, u4Temp);
    }
    else
    {
        if (ucFld == TOP_FIELD)
        {
#if CONFIG_DRV_VIRTUAL_ADDR
            vVDecWriteMC(u4VDecID, RW_MC_B_LIST0 + (ucRegIdx<<2), PHYSICAL((UINT32) prPRefPicListInfo->u4FBufYStartAddr));    
#else
            vVDecWriteMC(u4VDecID, RW_MC_B_LIST0 + (ucRegIdx<<2), u4AbsDramANc((UINT32) prPRefPicListInfo->u4FBufYStartAddr));    
#endif
            vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_B0_RPL + (ucRegIdx<<2), (1 << 19) + (2 << 20) + (prPRefPicListInfo->u4ViewId & 0x7ffff));

            u4Temp = ((ucFBufIdx)<<1<<22) + prPRefPicListInfo->u4TFldPara + (prPRefPicListInfo->i4TFldPOC & 0x3ffff);
            vVDecWriteAVCMV(u4VDecID, RW_AMV_B0_REF_PARA + (ucRegIdx<<2), u4Temp);
        }
        else
        {        
#if CONFIG_DRV_VIRTUAL_ADDR
            vVDecWriteMC(u4VDecID, RW_MC_B_LIST0 + (ucRegIdx<<2), PHYSICAL((UINT32) prPRefPicListInfo->u4FBufYStartAddr));    
#else
            vVDecWriteMC(u4VDecID, RW_MC_B_LIST0 + (ucRegIdx<<2), u4AbsDramANc((UINT32) prPRefPicListInfo->u4FBufYStartAddr));    
#endif
            vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_B0_RPL + (ucRegIdx<<2), (1 << 19) + (2 << 20) + (prPRefPicListInfo->u4ViewId & 0x7ffff));

            u4Temp = (((ucFBufIdx<<1) + 1)<<22) + prPRefPicListInfo->u4BFldPara + (prPRefPicListInfo->i4BFldPOC & 0x3ffff);
            vVDecWriteAVCMV(u4VDecID, RW_AMV_B0_REF_PARA + (ucRegIdx<<2), u4Temp);

            vVDecWriteMC(u4VDecID, RW_AMC_B_LIST0_FLD, u4VDecReadMC(u4VDecID, RW_AMC_B_LIST0_FLD) | (0x1 << ucRegIdx));        
        }
    }
}

// **************************************************************************
// Function : void vVDEC_HAL_H264_SetInterViewB1RefPicListReg(UINT32 u4VDecID, VDEC_INFO_H264_P_REF_PRM_T *prPRefPicListInfo);
// Description :Set HW registers related with InterView Ref Pic
// Parameter : u4VDecID : video decoder hardware ID
//                 prPRefPicListInfo : pointer to information of p reference list
// Return      : None
// **************************************************************************
void vVDEC_HAL_H264_SetInterViewB1RefPicListReg(UINT32 u4VDecID, VDEC_INFO_H264_P_REF_PRM_T *prPRefPicListInfo)
{
    UCHAR ucFBufIdx;
    UCHAR ucFld;  
    UCHAR ucRegIdx;
    UINT32 u4Temp;

    ucFld = (prPRefPicListInfo->u4FBufInfo & 0xff);  
    ucRegIdx = ((prPRefPicListInfo->u4FBufInfo >> 16) & 0xff);  
    ucFBufIdx = prPRefPicListInfo->ucFBufIdx;

    if (ucFld == FRAME)
    {
        // B_1
        ucFBufIdx = prPRefPicListInfo->ucFBufIdx;
#if CONFIG_DRV_VIRTUAL_ADDR
        vVDecWriteMC(u4VDecID, RW_MC_B_LIST1 + (ucRegIdx<<2), PHYSICAL((UINT32) prPRefPicListInfo->u4FBufYStartAddr));    
#else
        vVDecWriteMC(u4VDecID, RW_MC_B_LIST1 + (ucRegIdx<<2), u4AbsDramANc((UINT32) prPRefPicListInfo->u4FBufYStartAddr));    
#endif
        vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_B1_RPL + (ucRegIdx<<2), (1 << 19) + (2 << 20) + (prPRefPicListInfo->u4ViewId & 0x7ffff));

        u4Temp = (((prPRefPicListInfo->i4TFldPOC <= prPRefPicListInfo->i4BFldPOC) && (prPRefPicListInfo->i4TFldPOC != 0x7fffffff))? 1: 0) << 21;    
        u4Temp += ((ucFBufIdx)<<1<<22) + prPRefPicListInfo->u4TFldPara + (prPRefPicListInfo->i4TFldPOC & 0x3ffff);
        vVDecWriteAVCMV(u4VDecID, RW_AMV_B1_REF_PARA + (ucRegIdx<<3), u4Temp);

        u4Temp = (((prPRefPicListInfo->i4TFldPOC <= prPRefPicListInfo->i4BFldPOC) && (prPRefPicListInfo->i4TFldPOC != 0x7fffffff))? 1: 0) << 21;    
        u4Temp += ((((ucFBufIdx)<<1) + 1)<<22) + prPRefPicListInfo->u4BFldPara + (prPRefPicListInfo->i4BFldPOC & 0x3ffff);
        vVDecWriteAVCMV(u4VDecID, RW_AMV_B1_REF_PARA + (ucRegIdx<<3) + 4, u4Temp);

#if CONFIG_DRV_VIRTUAL_ADDR
        u4Temp = PHYSICAL((UINT32) prPRefPicListInfo->u4FBufMvStartAddr);
#else
        u4Temp = u4AbsDramANc((UINT32) prPRefPicListInfo->u4FBufMvStartAddr);
#endif
        vVDecWriteAVCMV(u4VDecID, RW_AMV_B1_REF_ADDR + (ucRegIdx<<3), u4Temp >> 4);
        vVDecWriteAVCMV(u4VDecID, RW_AMV_B1_REF_ADDR + (ucRegIdx<<3) + 4, (u4Temp >> 4) + 4); 
    }
    else
    {
        if (ucFld == TOP_FIELD)
        {
#if CONFIG_DRV_VIRTUAL_ADDR
            vVDecWriteMC(u4VDecID, RW_MC_B_LIST1 + (ucRegIdx<<2), PHYSICAL((UINT32) prPRefPicListInfo->u4FBufYStartAddr));    
#else
            vVDecWriteMC(u4VDecID, RW_MC_B_LIST1 + (ucRegIdx<<2), u4AbsDramANc((UINT32) prPRefPicListInfo->u4FBufYStartAddr));    
#endif
            vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_B1_RPL + (ucRegIdx<<2), (1 << 19) + (2 << 20) + (prPRefPicListInfo->u4ViewId & 0x7ffff));

            u4Temp = (1<<19) + ((ucFBufIdx)<<1<<22)+ (prPRefPicListInfo->i4TFldPOC & 0x3ffff);
            vVDecWriteAVCMV(u4VDecID, RW_AMV_B1_REF_PARA + (ucRegIdx<<2), u4Temp);

#if CONFIG_DRV_VIRTUAL_ADDR
            u4Temp = PHYSICAL((UINT32) prPRefPicListInfo->u4FBufMvStartAddr);
#else            
            u4Temp = u4AbsDramANc((UINT32) prPRefPicListInfo->u4FBufMvStartAddr);
#endif
            vVDecWriteAVCMV(u4VDecID, RW_AMV_B1_REF_ADDR + (ucRegIdx<<2), (u4Temp >> 4));      
        }
        else
        {
#if CONFIG_DRV_VIRTUAL_ADDR
            vVDecWriteMC(u4VDecID, RW_MC_B_LIST1 + (ucRegIdx<<2), PHYSICAL((UINT32) prPRefPicListInfo->u4FBufYStartAddr));    
#else
            vVDecWriteMC(u4VDecID, RW_MC_B_LIST1 + (ucRegIdx<<2), u4AbsDramANc((UINT32) prPRefPicListInfo->u4FBufYStartAddr));    
#endif
            vVDecWriteAVCVLD(u4VDecID, RW_AVLD_REORD_B1_RPL + (ucRegIdx<<2), (1 << 19) + (2 << 20) + (prPRefPicListInfo->u4ViewId & 0x7ffff));

#if CONFIG_DRV_VIRTUAL_ADDR
            u4Temp = PHYSICAL((UINT32) prPRefPicListInfo->u4FBufMvStartAddr);
#else
            u4Temp = u4AbsDramANc((UINT32) prPRefPicListInfo->u4FBufMvStartAddr);
#endif
            vVDecWriteAVCMV(u4VDecID, RW_AMV_B1_REF_ADDR + (ucRegIdx<<2), (u4Temp >> 4) + 4);

            u4Temp = (1<<19) + (((ucFBufIdx<<1) + 1)<<22) + (prPRefPicListInfo->i4BFldPOC & 0x3ffff);
            vVDecWriteAVCMV(u4VDecID, RW_AMV_B1_REF_PARA + (ucRegIdx<<2), u4Temp);

            vVDecWriteMC(u4VDecID, RW_AMC_B_LIST1_FLD, u4VDecReadMC(u4VDecID, RW_AMC_B_LIST1_FLD) | (0x1 << ucRegIdx));        
        }
    }

}

#endif


void vVDEC_HAL_H264_VDec_PowerDown(UCHAR u4VDecID)
{
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8555)
     vVDecPowerDownHW(u4VDecID);
#endif
}

#ifdef MPV_DUMP_H264_DEC_REG
static UINT32 _aru4DumpH264Data[0x500];
// *********************************************************************
// Function :void VDec_DumpH264Reg(UCHAR ucMpvId)
// Description : Set RW_MC_BREF
// Parameter : fgIVop: indicate the backward reference picture is an I-VOP
// Return    : None
// *********************************************************************
void VDec_DumpH264Reg(UCHAR ucMpvId)
{
    UINT32 dwTemp;
    INT32 i,j;

    UINT32  u4Idx;
    UINT32 u4DumpIdx;

    UINT32 *pu4DumpArea;

    //pu4DumpArea = (UINT32*)x_mem_alloc(0x200);
    pu4DumpArea = (&_aru4DumpH264Data[0]);
    u4DumpIdx = 0;

    for (u4Idx = 33; u4Idx <= 39; u4Idx++) // 7 DWRDs
    {
        pu4DumpArea[u4DumpIdx] = u4VDecReadAVCVLD(ucMpvId, 4*u4Idx);
        u4DumpIdx ++;
    }
    u4Idx = 42;  //0xa8
    pu4DumpArea[u4DumpIdx] = u4VDecReadAVCVLD(ucMpvId, 4*u4Idx);
    u4DumpIdx ++;
    u4Idx = 74;  //0x128
    pu4DumpArea[u4DumpIdx] = u4VDecReadAVCVLD(ucMpvId, 4*u4Idx);
    u4DumpIdx ++;
    u4Idx = 106; //0x1a8
    pu4DumpArea[u4DumpIdx] = u4VDecReadAVCVLD(ucMpvId, 4*u4Idx);
    u4DumpIdx ++;
    u4Idx = 0; //0x0    
    pu4DumpArea[u4DumpIdx] = u4VDecReadAVCVLD(ucMpvId, 4*u4Idx);
    u4DumpIdx ++;

    // "== Deblocking "
    u4DumpIdx = 12;    //0x30
    u4Idx = 136; // 0x220
    pu4DumpArea[u4DumpIdx] = u4VDecReadMC(ucMpvId, 4*u4Idx);
    u4DumpIdx ++;
    u4Idx = 137;
    pu4DumpArea[u4DumpIdx] = u4VDecReadMC(ucMpvId, 4*u4Idx);
    u4DumpIdx ++;
    u4Idx = 138;
    pu4DumpArea[u4DumpIdx] = u4VDecReadMC(ucMpvId, 4*u4Idx);
    u4DumpIdx ++;
    u4Idx = 139; // 22c
    pu4DumpArea[u4DumpIdx] = u4VDecReadMC(ucMpvId, 4*u4Idx);
    u4DumpIdx ++;
    u4Idx = 142; // 238
    pu4DumpArea[u4DumpIdx] = u4VDecReadMC(ucMpvId, 4*u4Idx);
    u4DumpIdx ++;
    u4Idx = 148; // 250
    pu4DumpArea[u4DumpIdx] = u4VDecReadMC(ucMpvId, 4*u4Idx);
    u4DumpIdx ++;
    u4Idx = 152; // 260
    pu4DumpArea[u4DumpIdx] = u4VDecReadMC(ucMpvId, 4*u4Idx);
    u4DumpIdx ++;
    u4Idx = 153; // 264
    pu4DumpArea[u4DumpIdx] = u4VDecReadMC(ucMpvId, 4*u4Idx);
    u4DumpIdx ++;

    // Motion Compensation
    u4DumpIdx = 20;    //0x50
    u4Idx = 130;  // 208
    pu4DumpArea[u4DumpIdx] = u4VDecReadMC(ucMpvId, 4*u4Idx);
    u4DumpIdx ++;
    u4Idx = 131;  // 20c
    pu4DumpArea[u4DumpIdx] = u4VDecReadMC(ucMpvId, 4*u4Idx);
    u4DumpIdx ++;
    u4Idx = 343; // 0x55c P_H264_CBCR_ADDR_OFFSET
    pu4DumpArea[u4DumpIdx] = u4VDecReadMC(ucMpvId, 4*u4Idx);
    u4DumpIdx ++;
    u4Idx = 344; // 560 P_H264_OUT_PIC_Y_ADDR
    pu4DumpArea[u4DumpIdx] = u4VDecReadMC(ucMpvId, 4*u4Idx);
    u4DumpIdx ++; 
    u4Idx = 345; 
    pu4DumpArea[u4DumpIdx] = u4VDecReadMC(ucMpvId, 4*u4Idx);
    u4DumpIdx ++;
    u4Idx = 346;
    pu4DumpArea[u4DumpIdx] = u4VDecReadMC(ucMpvId, 4*u4Idx);
    u4DumpIdx ++;
    u4Idx = 347; // 56c
    pu4DumpArea[u4DumpIdx] = u4VDecReadMC(ucMpvId, 4*u4Idx);
    u4DumpIdx ++;

    u4Idx = 131; // 0x20c, MV_START_ADDR
    pu4DumpArea[u4DumpIdx] = u4VDecReadAVCMV(ucMpvId, 4*u4Idx);
    u4DumpIdx ++;

    //MC p_list0 setting
    u4DumpIdx = 28;    //0x70    
    for(i=0;i<32;i++)
    {
        pu4DumpArea[u4DumpIdx] = u4VDecReadMC(ucMpvId, RW_MC_P_LIST0  + 4*i);
        u4DumpIdx ++;
    }

    //MC b_list0 setting
    u4DumpIdx = 60;    //0xf0
    for(i=0;i<32;i++)
    {
        pu4DumpArea[u4DumpIdx] = u4VDecReadMC(ucMpvId, RW_MC_B_LIST0  + 4*i);
        u4DumpIdx ++;
    }

    //MC b_list1 setting
    u4DumpIdx = 92;    //0x170
    for(i=0;i<32;i++)
    {
        pu4DumpArea[u4DumpIdx] = u4VDecReadMC(ucMpvId, RW_MC_B_LIST1  + 4*i);
        u4DumpIdx ++;
    }

    //MV p_list0 setting
    u4DumpIdx = 124;    //0x1F0
    for(i=0;i<32;i++)
    {
        pu4DumpArea[u4DumpIdx] = u4VDecReadAVCMV(ucMpvId, RW_AMV_P_REF_PARA  + 4*i);
        u4DumpIdx ++;
    }

    //MV b_list0 setting
    u4DumpIdx = 156;    //0x270
    for(i=0;i<32;i++)
    {
        pu4DumpArea[u4DumpIdx] = u4VDecReadAVCMV(ucMpvId, RW_AMV_B0_REF_PARA  + 4*i);
        u4DumpIdx ++;
    }

    //MV b_list1 setting
    u4DumpIdx = 188;    //0x2F0
    for(i=0;i<32;i++)
    {
        pu4DumpArea[u4DumpIdx] = u4VDecReadAVCMV(ucMpvId, RW_AMV_B1_REF_PARA  + 4*i);
        u4DumpIdx ++;
    }

    //MV drt_adr setting
    u4DumpIdx = 220;    //0x370    
    for(i=0;i<32;i++)
    {
        pu4DumpArea[u4DumpIdx] = u4VDecReadAVCMV(ucMpvId, RW_AMV_B1_REF_ADDR  + 4*i);
        u4DumpIdx ++;
    }

    // MV POC setting
    u4DumpIdx = 252;    //0x3F0    
    u4Idx = 128; //RW_AMV_CURR_POC
    pu4DumpArea[u4DumpIdx] = u4VDecReadAVCMV(ucMpvId, 4*u4Idx);
    u4DumpIdx ++;
    u4Idx = 129; //RW_AMV_CURR_TFLD_POC
    pu4DumpArea[u4DumpIdx] = u4VDecReadAVCMV(ucMpvId, 4*u4Idx);
    u4DumpIdx ++;
    u4Idx = 130; //RW_AMV_CURR_BFLD_POC
    pu4DumpArea[u4DumpIdx] = u4VDecReadAVCMV(ucMpvId, 4*u4Idx);
    u4DumpIdx ++;


    // total 6*4 + 2*16 DWRDs = 56 DWRDs
    u4DumpIdx = 256;    //0x400
    for(j=0; j<8; j++)
    {
        if(j < 6)
        {
            dwTemp = (j << 4);
            for(i=0; i<4; i++)
            {
                // add 16 for every list
                vVDecWriteVLD(ucMpvId, RW_VLD_SCL_ADDR, 0x100 + dwTemp + (i << 2));
                pu4DumpArea[u4DumpIdx] = u4VDecReadVLD(ucMpvId, RW_VLD_SCL_DATA);
                u4DumpIdx ++;
            }
        }
        else
        {
            dwTemp = (j == 6)? (j << 4) : ((j + 3) << 4); // 6=>16*6   7=>16*6+64(equal to 16*7+48)
            for(i=0; i<16; i++)
            {
                // add 64 for every list
                vVDecWriteVLD(ucMpvId, RW_VLD_SCL_ADDR, 0x100 + dwTemp + (i << 2));
                pu4DumpArea[u4DumpIdx] = u4VDecReadVLD(ucMpvId, RW_VLD_SCL_DATA);
                u4DumpIdx ++;
            }
        }
    }    
}
#endif

#if CONFIG_DRV_VERIFY_SUPPORT
UINT32 u4VDEC_HAL_H264_VDec_ReadFinishFlag(UINT32 u4VDecID)
{
  return u4VDecReadAVCVLD(u4VDecID, RO_AVLD_COMPLETE);
}

void vVDEC_HAL_H264_VDec_DumpReg(UINT32 u4VDecID)
{

}

void vVDEC_HAL_H264_VDec_ReadCheckSum(UINT32 u4VDecID, UINT32 *pu4CheckSum)
{
  UINT32  u4Temp,u4Cnt;
  
  u4Temp = 0;
  *pu4CheckSum = u4VDecReadMC(u4VDecID, 0x5f4);
  pu4CheckSum ++;
  u4Temp ++;
  *pu4CheckSum = u4VDecReadMC(u4VDecID, 0x5f8);    
  pu4CheckSum ++;
  u4Temp ++;
  *pu4CheckSum = u4VDecReadMC(u4VDecID, 0x608);    
  pu4CheckSum ++;
  u4Temp ++;
  *pu4CheckSum = u4VDecReadMC(u4VDecID, 0x60c);        
  pu4CheckSum ++;
  u4Temp ++;

  //MC  378~397  
  for(u4Cnt=378; u4Cnt<=397; u4Cnt++)
  {
    *pu4CheckSum = u4VDecReadMC(u4VDecID, (u4Cnt<<2));
    pu4CheckSum ++;   
    u4Temp ++;
  }

  //AVC VLD  165~179
  for(u4Cnt=165; u4Cnt<=179; u4Cnt++)
  {
    *pu4CheckSum = u4VDecReadAVCVLD(u4VDecID, (u4Cnt<<2));            
      pu4CheckSum ++;  
      u4Temp ++;
  }

  //MV  147~151
  for(u4Cnt=147; u4Cnt<=151; u4Cnt++)
  {
    *pu4CheckSum = u4VDecReadAVCMV(u4VDecID, (u4Cnt<<2));            
    pu4CheckSum ++;       
    u4Temp ++;
  }

  //IP  212    
  *pu4CheckSum = u4VDecReadAVCMV(u4VDecID, (212 << 2));
  pu4CheckSum ++;  
  u4Temp ++;
   
  //IQ  235~239
  for(u4Cnt=241; u4Cnt<=245; u4Cnt++)
  {
    *pu4CheckSum = u4VDecReadAVCMV(u4VDecID, (u4Cnt<<2));            
    pu4CheckSum ++;     
    u4Temp ++;
  }    

  //IS  241~245
  for(u4Cnt=241; u4Cnt<=245; u4Cnt++)
  {
    *pu4CheckSum = u4VDecReadAVCMV(u4VDecID, (u4Cnt<<2));            
    pu4CheckSum ++;     
    u4Temp ++;
  }

  while(u4Temp < MAX_CHKSUM_NUM)
  {
    *pu4CheckSum = 0;            
    pu4CheckSum ++;   
    u4Temp ++;
  }  
}

BOOL fgVDEC_HAL_H264_VDec_CompCheckSum(UINT32 *pu4DecCheckSum, UINT32 *pu4GoldenCheckSum)
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

#ifdef MPV_DUMP_H264_CHKSUM
#define MAX_CHKSUM_NUM 80
UINT32 _u4DumpChksum[2][MAX_CHKSUM_NUM];
void vVDEC_HAL_H264_VDec_ReadCheckSum1(UINT32 u4VDecID)
{
    UINT32  u4Temp,u4Cnt;

    UINT32 *pu4CheckSum = _u4DumpChksum[0];
        
    u4Temp = 0;
    *pu4CheckSum = u4VDecReadMC(u4VDecID, 0x5f4);
    pu4CheckSum ++;
    u4Temp ++;
    *pu4CheckSum = u4VDecReadMC(u4VDecID, 0x5f8);    
    pu4CheckSum ++;
    u4Temp ++;
    *pu4CheckSum = u4VDecReadMC(u4VDecID, 0x608);    
    pu4CheckSum ++;
    u4Temp ++;
    *pu4CheckSum = u4VDecReadMC(u4VDecID, 0x60c);        
    pu4CheckSum ++;
    u4Temp ++;

    //MC  378~397  
    for(u4Cnt=378; u4Cnt<=397; u4Cnt++)
    {
        *pu4CheckSum = u4VDecReadMC(u4VDecID, (u4Cnt<<2));
        pu4CheckSum ++;   
        u4Temp ++;
    }

    *pu4CheckSum = u4VDecReadVLD(u4VDecID, (44<<2));        
    pu4CheckSum ++;
    u4Temp ++;
    
    *pu4CheckSum = u4VDecReadVLD(u4VDecID, (45<<2));        
    pu4CheckSum ++;
    u4Temp ++;
    
    *pu4CheckSum = u4VDecReadVLD(u4VDecID, (46<<2));        
    pu4CheckSum ++;
    u4Temp ++;
    
    //VLD  58~63
    for(u4Cnt=58; u4Cnt<=63; u4Cnt++)
    {
        *pu4CheckSum = u4VDecReadVLD(u4VDecID, (u4Cnt<<2));            
        pu4CheckSum ++;  
        u4Temp ++;
    }

    *pu4CheckSum = u4VDecReadAVCVLD(u4VDecID, 0x84);            
    pu4CheckSum ++;  
    u4Temp ++;
    
    //AVC VLD  148~152
    for(u4Cnt=148; u4Cnt<=155; u4Cnt++)
    {
        *pu4CheckSum = u4VDecReadAVCVLD(u4VDecID, (u4Cnt<<2));            
        pu4CheckSum ++;  
        u4Temp ++;
    }

    //AVC VLD  165~179
    for(u4Cnt=165; u4Cnt<=179; u4Cnt++)
    {
        *pu4CheckSum = u4VDecReadAVCVLD(u4VDecID, (u4Cnt<<2));            
        pu4CheckSum ++;  
        u4Temp ++;
    }

    //MV  147~151
    for(u4Cnt=147; u4Cnt<=151; u4Cnt++)
    {
        *pu4CheckSum = u4VDecReadAVCMV(u4VDecID, (u4Cnt<<2));            
        pu4CheckSum ++;       
        u4Temp ++;
    }

    //IP  212    
    *pu4CheckSum = u4VDecReadAVCMV(u4VDecID, (212 << 2));
    pu4CheckSum ++;  
    u4Temp ++;

    //IQ  235~239
    for(u4Cnt=241; u4Cnt<=245; u4Cnt++)
    {
        *pu4CheckSum = u4VDecReadAVCMV(u4VDecID, (u4Cnt<<2));            
        pu4CheckSum ++;     
        u4Temp ++;
    }    

    //IS  241~245
    for(u4Cnt=241; u4Cnt<=245; u4Cnt++)
    {
        *pu4CheckSum = u4VDecReadAVCMV(u4VDecID, (u4Cnt<<2));            
        pu4CheckSum ++;     
        u4Temp ++;
    }

    while(u4Temp < MAX_CHKSUM_NUM)
    {
        *pu4CheckSum = 0;            
        pu4CheckSum ++;   
        u4Temp ++;
    }  
}

void vVDEC_HAL_H264_VDec_ReadCheckSum2(UINT32 u4VDecID)
{
    UINT32  u4Temp,u4Cnt;

    UINT32 *pu4CheckSum = _u4DumpChksum[1];
        
    u4Temp = 0;
    *pu4CheckSum = u4VDecReadMC(u4VDecID, 0x5f4);
    pu4CheckSum ++;
    u4Temp ++;
    *pu4CheckSum = u4VDecReadMC(u4VDecID, 0x5f8);    
    pu4CheckSum ++;
    u4Temp ++;
    *pu4CheckSum = u4VDecReadMC(u4VDecID, 0x608);    
    pu4CheckSum ++;
    u4Temp ++;
    *pu4CheckSum = u4VDecReadMC(u4VDecID, 0x60c);        
    pu4CheckSum ++;
    u4Temp ++;

    //MC  378~397  
    for(u4Cnt=378; u4Cnt<=397; u4Cnt++)
    {
        *pu4CheckSum = u4VDecReadMC(u4VDecID, (u4Cnt<<2));
        pu4CheckSum ++;   
        u4Temp ++;
    }

    *pu4CheckSum = u4VDecReadVLD(u4VDecID, (44<<2));        
    pu4CheckSum ++;
    u4Temp ++;
    
    *pu4CheckSum = u4VDecReadVLD(u4VDecID, (45<<2));        
    pu4CheckSum ++;
    u4Temp ++;
    
    *pu4CheckSum = u4VDecReadVLD(u4VDecID, (46<<2));        
    pu4CheckSum ++;
    u4Temp ++;
    
    //VLD  58~63
    for(u4Cnt=58; u4Cnt<=63; u4Cnt++)
    {
        *pu4CheckSum = u4VDecReadVLD(u4VDecID, (u4Cnt<<2));            
        pu4CheckSum ++;  
        u4Temp ++;
    }

    *pu4CheckSum = u4VDecReadAVCVLD(u4VDecID, 0x84);            
    pu4CheckSum ++;  
    u4Temp ++;
    
    //AVC VLD  148~152
    for(u4Cnt=148; u4Cnt<=155; u4Cnt++)
    {
        *pu4CheckSum = u4VDecReadAVCVLD(u4VDecID, (u4Cnt<<2));            
        pu4CheckSum ++;  
        u4Temp ++;
    }

    //AVC VLD  165~179
    for(u4Cnt=165; u4Cnt<=179; u4Cnt++)
    {
        *pu4CheckSum = u4VDecReadAVCVLD(u4VDecID, (u4Cnt<<2));            
        pu4CheckSum ++;  
        u4Temp ++;
    }
    
    //MV  147~151
    for(u4Cnt=147; u4Cnt<=151; u4Cnt++)
    {
        *pu4CheckSum = u4VDecReadAVCMV(u4VDecID, (u4Cnt<<2));            
        pu4CheckSum ++;       
        u4Temp ++;
    }

    //IP  212    
    *pu4CheckSum = u4VDecReadAVCMV(u4VDecID, (212 << 2));
    pu4CheckSum ++;  
    u4Temp ++;

    //IQ  235~239
    for(u4Cnt=241; u4Cnt<=245; u4Cnt++)
    {
        *pu4CheckSum = u4VDecReadAVCMV(u4VDecID, (u4Cnt<<2));            
        pu4CheckSum ++;     
        u4Temp ++;
    }    

    //IS  241~245
    for(u4Cnt=241; u4Cnt<=245; u4Cnt++)
    {
        *pu4CheckSum = u4VDecReadAVCMV(u4VDecID, (u4Cnt<<2));            
        pu4CheckSum ++;     
        u4Temp ++;
    }

    while(u4Temp < MAX_CHKSUM_NUM)
    {
        *pu4CheckSum = 0;            
        pu4CheckSum ++;   
        u4Temp ++;
    }  
}
#endif
