
#include "vdec_hw_common.h"
#include "vdec_hw_vp8.h"
#include "vdec_hal_if_vp8.h"
#include "vdec_hal_if_common.h"
#include "vdec_hal_errcode.h"
//#include "x_hal_ic.h"
//#include "x_hal_1176.h"
//#include "x_debug.h"

#if CONFIG_DRV_VERIFY_SUPPORT
#include "../verify/vdec_verify_general.h"
#include "../verify/vdec_verify_mpv_prov.h"
//#include <mach/cache_operation.h>
#include <linux/string.h>

#if (!CONFIG_DRV_LINUX)
#include <stdio.h>
#include <string.h>
#endif
#include <linux/dma-mapping.h>

extern void vVDecOutputDebugString(const CHAR * format, ...);
extern BOOL fgWrMsg2PC(void* pvAddr, UINT32 u4Size, UINT32 u4Mode, VDEC_INFO_VERIFY_FILE_INFO_T *pFILE_INFO);


#endif



// **************************************************************************
// Function : INT32 i4VDEC_HAL_VP8_InitVDecHW(UINT32 u4Handle, VDEC_INFO_VP8_VFIFO_PRM_T *prVp8VFifoInitPrm);
// Description :Initialize video decoder hardware only for VP8
// Parameter : u4VDecID : video decoder hardware ID
//                  prVp8VFifoInitPrm : pointer to VFIFO info struct
// Return      : =0: success.
//                  <0: fail.
// **************************************************************************
INT32 i4VDEC_HAL_VP8_InitVDecHW(UINT32 u4VDecID, VDEC_INFO_VP8_VFIFO_PRM_T *prVp8VFifoInitPrm)
{
#if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8555)
    vVDecResetHW(u4VDecID);
#else
    vVDecResetHW(u4VDecID, VDEC_VP8);
#endif
    vVDecWriteVP8VLD(u4VDecID, RW_VP8_CTRL, 0x1<<RW_VP8_VP8FLAG, 0);   
    vVDecSetVLDVFIFO(0, u4VDecID, PHYSICAL((UINT32) prVp8VFifoInitPrm->u4VFifoSa), PHYSICAL((UINT32) prVp8VFifoInitPrm->u4VFifoEa));
    return HAL_HANDLE_OK;
}

void vVDEC_VP8_VldReset(UINT32 u4VldId)
{
    UINT32 u4Cnt = 0;
    if(u4VldId==VP8_VLD1)
    {
        if(u4VDecReadVLD(0, RO_VLD_SRAMCTRL) & PROCESS_FLAG)
        {
           while((u4VDecReadVLD(0, RO_VLD_SRAMCTRL) & AA_FIT_TARGET_SCLK) == 0)
           {
              u4Cnt++;
              if(u4Cnt >= WAIT_THRD)
              {
                printk("VP8 VLD1 HWReset WaitSramStable Fail\n");
                break;
              }
           }
        }
        
        vVDecWriteVLD(0, WO_VLD_SRST, 1);
        vVDecWriteVLD(0, WO_VLD_SRST, 0);
        vVDecWriteVP8VLD(0, RW_VP8_CTRL,VDEC_GET_FLAGVAL(1,RW_VP8_VP8FLAG), 0);
    }
    else
    {
        if(u4VDecReadVLD(0, RO_VLD_SRAMCTRL) & PROCESS_FLAG)
        {
           while((u4VDecReadVLD(0, RO_VLD_SRAMCTRL) & AA_FIT_TARGET_SCLK) == 0)
           {
              u4Cnt++;
              if(u4Cnt >= WAIT_THRD)
              {
                printk("VP8 VLD1 HWReset WaitSramStable Fail\n");
                break;
              }
           }
        }
        
        vVDecWriteVP8VLD2(0, WO_VLD_SRST, 1);
        vVDecWriteVP8VLD2(0, WO_VLD_SRST, 0);
    }
}
BOOL fgVDEC_Vp8DecReset(UINT32 u4InstID, VDEC_INFO_VP8_FRM_HDR_T *pVp8DecInfo,BOOL fgInit)
{
   vVDEC_VP8_VldReset(VP8_VLD1);
   if(!fgInit)
   {
       UINT32 u4HalValue;
       while(!i4VDEC_HAL_VP8_InitBarrelShifter(VP8_VLD1, 0, pVp8DecInfo))
       {
           printk("VP8 Init VLD1 BarrelShifter fail\n");
           vVDEC_VP8_VldReset(VP8_VLD1);
       }
       u4HalValue=VDEC_GET_FLAGVAL(1,RW_VP8_BCRT1_FLAG);
       vVDecWriteVP8VLD(0, RW_VP8_BCRT,u4HalValue, 0);
       vVDecWriteVP8VLD(0, RW_VP8_BCRT,0, 0);
   }

   vVDEC_VP8_VldReset(VP8_VLD2);
   if(!fgInit)
   {
       vVDecWriteVP8VLD2(0,RW_VLD_RDY_SWTICH, READY_TO_RISC_1);
       vVDecWriteVP8VLD2(0,RW_VLD_VSTART, PHYSICAL((UINT32)(pVp8DecInfo->u4FifoStart ))>> 6);
       vVDecWriteVP8VLD2(0,RW_VLD_VEND, PHYSICAL((UINT32)(pVp8DecInfo->u4FifoEnd))>> 6);       
   }
   #ifdef VDEC_VER_COMPARE_CRC
   if(fgInit)
   {
      vVDecWriteCRC(0, VDEC_CRC_REG_EN, 1);
   }
#endif

   return TRUE;
}
BOOL fgVDEC_Vp8IsDecFinish(VOID)
{
   return (BOOL )((u4VDecReadVP8VLD(0,RO_VP8_PFR)&1) ||(u4VDecReadVP8VLD(0,RO_VP8_VOKR)&1));
}


// **************************************************************************
// Function : UINT32 u4VDEC_HAL_VP8_ShiftGetBitStream(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits);
// Description :Read barrel shifter after shifting
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
//                 u4ShiftBits : shift bits number
// Return      : Value of barrel shifter input window after shifting
// **************************************************************************
UINT32 u4VDEC_HAL_VP8_ShiftGetBitStream(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits)
{
    UINT32 dRegVal;
  
    dRegVal = u4VDecVP8VLDGetBits(u4BSID, u4VDecID, u4ShiftBits);
    
    return(dRegVal);
}

// **************************************************************************
// Function : UINT32 u4VDEC_HAL_VP8_GetBitStreamShift(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits);
// Description :Read Barrel Shifter before shifting
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
//                 u4ShiftBits : shift bits number
// Return      : Value of barrel shifter input window before shifting
// **************************************************************************
UINT32 u4VDEC_HAL_VP8_GetBitStreamShift(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits)
{
    UINT32 u4RegVal0;
  
    u4RegVal0 = u4VDecVP8VLDGetBits(u4BSID, u4VDecID, 0);
    u4VDecVP8VLDGetBits(u4BSID, u4VDecID, u4ShiftBits);
    
    return(u4RegVal0);
}

// **************************************************************************
// Function : INT32 i4VDEC_HAL_VP8_InitBarrelShifter(UINT32 u4BSID, UINT32 u4VDecID, VDEC_INFO_VP8_FRM_HDR_T *prVp8BSInitPrm, BOOL fgIsVC1);
// Description :Initialize barrel shifter with byte alignment
// Parameter : u4ReadPointer : set read pointer value
//                 u4WrtePointer : set write pointer value
//                 u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
// Return      : =0: success.
//                  <0: fail.
// **************************************************************************
INT32 i4VDEC_HAL_VP8_InitBarrelShifter(UINT32 u1VP8VLD, UINT32 u4VDecID, VDEC_INFO_VP8_FRM_HDR_T *pVp8DecInfo)
{
    BOOL fgFetchOK = FALSE;
    INT32 i,j;
    //UINT32 u4VLDByte, u4VLDBit;
    UINT32 u4VLDRemainByte;
  
#if (CONFIG_DRV_VERIFY_SUPPORT) && (CONFIG_DRV_LINUX)    
 //   HalDisableDCache();
#endif

    
  if (u1VP8VLD == VP8_VLD1)
  	{
    vVDecWriteVLD(u4VDecID,RW_VLD_RDY_SWTICH, READY_TO_RISC_1);
    vVDecWriteVLD(u4VDecID, RW_VLD_VSTART , PHYSICAL((UINT32)pVp8DecInfo->u4FifoStart) >> 6);
    vVDecWriteVLD(u4VDecID, RW_VLD_VEND, PHYSICAL((UINT32)pVp8DecInfo->u4FifoEnd)>> 6);
  	}
  else
  	{
    vVDecWriteVP8VLD2(u4VDecID,RW_VLD_RDY_SWTICH, READY_TO_RISC_1);
    vVDecWriteVP8VLD2(u4VDecID, RW_VLD_VSTART , PHYSICAL((UINT32)pVp8DecInfo->u4FifoStart) >> 6);
    vVDecWriteVP8VLD2(u4VDecID, RW_VLD_VEND, PHYSICAL((UINT32)pVp8DecInfo->u4FifoEnd)>> 6);
  	}
    
    u4VLDRemainByte =  ((pVp8DecInfo->u4VldStartPos)) & 0xf;
    // prevent initialize barrel fail
    for (i = 0; i < 5; i++)
    {
  if (u1VP8VLD == VP8_VLD1)
  	{
        vVDecWriteVLD(u4VDecID, WO_VLD_WPTR, u4VDecReadVLD(u4VDecID, WO_VLD_WPTR) | VLD_CLEAR_PROCESS_EN);
        vVDecWriteVLD(u4VDecID, RW_VLD_RPTR, PHYSICAL((UINT32) pVp8DecInfo->u4VldStartPos));
        vVDecWriteVLD(u4VDecID, RW_VLD_RPTR, PHYSICAL((UINT32) pVp8DecInfo->u4VldStartPos));
        vVDecWriteVLD(u4VDecID, WO_VLD_WPTR,PHYSICAL((UINT32) pVp8DecInfo->u4WritePos));
       // vVDecWriteVLD(u4VDecID, RW_VLD_ASYNC, u4VDecReadVLD(u4VDecID, RW_VLD_ASYNC) | VLD_WR_ENABLE);
        // start to fetch data
            vVDecWriteVLD(u4VDecID, WO_VLD_SRST, 0x100);
            vVDecWriteVLD(u4VDecID, WO_VLD_SRST, 0x0);
        vVDecWriteVLD(u4VDecID, RW_VLD_PROC, VLD_INIFET);
  	}
  else
  	{
        vVDecWriteVP8VLD2(u4VDecID, WO_VLD_WPTR, u4VDecReadVP8VLD2(u4VDecID, WO_VLD_WPTR) | VLD_CLEAR_PROCESS_EN);
        vVDecWriteVP8VLD2(u4VDecID, RW_VLD_RPTR, PHYSICAL((UINT32)( pVp8DecInfo->u4VldStartPos)));
        vVDecWriteVP8VLD2(u4VDecID, RW_VLD_RPTR, PHYSICAL((UINT32)( pVp8DecInfo->u4VldStartPos)));
        vVDecWriteVP8VLD2(u4VDecID, WO_VLD_WPTR,PHYSICAL((UINT32)( pVp8DecInfo->u4WritePos)));
    //    vVDecWriteVP8VLD2(u4VDecID, RW_VLD_ASYNC, u4VDecReadVP8VLD2(u4VDecID, RW_VLD_ASYNC) | VLD_WR_ENABLE);
        // start to fetch data
            vVDecWriteVP8VLD2(u4VDecID, WO_VLD_SRST, 0x100);
            vVDecWriteVP8VLD2(u4VDecID, WO_VLD_SRST, 0x0);
        vVDecWriteVP8VLD2(u4VDecID, RW_VLD_PROC, VLD_INIFET);
  	}
    
        if (fgVDecWaitVP8VldFetchOk(0, u4VDecID, u1VP8VLD))
        {
            fgFetchOK = TRUE;
            break;
        }
  	}
    if (fgFetchOK)
    {
    if (u1VP8VLD == VP8_VLD1)
    	vVDecWriteVLD(u4VDecID, RW_VLD_PROC , VLD_INIBR);           
    else
    	vVDecWriteVP8VLD2(u4VDecID, RW_VLD_PROC, VLD_INIBR);           
    
    for (j=0;j<u4VLDRemainByte;j++)
    {
    if (u1VP8VLD == VP8_VLD1)
    	{  u4VDecVP8VLDGetBits(VP8_VLD1, u4VDecID, 8);}
    else
    	{  u4VDecVP8VLDGetBits(VP8_VLD2, u4VDecID, 8);}
    }
    }
    else
    	{
    	return(INIT_BARRELSHIFTER_FAIL);    	
    	}
    
    //u4VLDByte = u4VDecReadVldRPtr(u4BSID, u4VDecID, &u4VLDBit, PHYSICAL((UINT32) prVp8BSInitPrm->u4VFifoSa));
    return HAL_HANDLE_OK;
}


// **************************************************************************
// Function : UINT32 u4VDEC_HAL_VP8_ReadRdPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4VFIFOSa, UINT32 *pu4Bits);
// Description :Read current read pointer
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
//                 u4VFIFOSa : video FIFO start address
//                 pu4Bits : read pointer value with remained bits
// Return      : Read pointer value with byte alignment
// **************************************************************************
UINT32 u4VDEC_HAL_VP8_ReadRdPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4VFIFOSa, UINT32 *pu4Bits)
{
    return u4VDecReadVldRPtr(u4BSID, u4VDecID, pu4Bits, PHYSICAL((UINT32) u4VFIFOSa));
}

// **************************************************************************
// Function : void v4VDEC_HAL_VP8_AlignRdPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4VFIFOSa, UINT32 u4AlignType);
// Description :Align read pointer to byte,word or double word
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
//                 u4VFIFOSa : video FIFO start address
//                 u4AlignType : read pointer align type
// Return      : None
// **************************************************************************
void vVDEC_HAL_VP8_AlignRdPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4VFIFOSa, UINT32 u4AlignType)
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


// **************************************************************************
// Function : UINT32 u4VDEC_HAL_VP8_GetBitcount(UINT32 u4BSID, UINT32 u4VDecID);
// Description :Read barrel shifter bitcount after initializing 
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
// Return      : Current bit count
// **************************************************************************
UINT32 u4VDEC_HAL_VP8_GetBitcount(UINT32 u4BSID, UINT32 u4VDecID)
{
    return HAL_HANDLE_OK;
}

// **************************************************************************
// Function : void v4VDEC_HAL_VP8_GetMbxMby(UINT32 u4VDecID, UINT32 *pu4Mbx, UINT32 *pu4Mby);
// Description :Read current decoded mbx and mby
// Parameter : u4VDecID : video decoder hardware ID
//                 pu4Mbx : macroblock x value
//                 pu4Mby : macroblock y value
// Return      : None
// **************************************************************************
void vVDEC_HAL_VP8_GetMbxMby(UINT32 u4VDecID, UINT32 *pu4Mbx, UINT32 *pu4Mby)
{
    *pu4Mbx = u4VDecReadMC(u4VDecID, RO_MC_MBX);
    *pu4Mby = u4VDecReadMC(u4VDecID, RO_MC_MBY);
}

// **************************************************************************
// Function : void v4VDEC_HAL_VP8_GetErrInfo(UINT32 u4VDecID, VDEC_INFO_VP8_ERR_INFO_T *prVp8ErrInfo);
// Description :Read error count after decoding end
// Parameter : u4VDecID : video decoder hardware ID
//                 prVp8ErrInfo : pointer to Vp8 error info struct
// Return      : None
// **************************************************************************
void vVDEC_HAL_VP8_GetErrInfo(UINT32 u4VDecID, VDEC_INFO_VP8_ERR_INFO_T *prVp8ErrInfo)
{
    UINT32 u4RegVal = 0;
    ///Check!!! 
    u4RegVal = u4VDecReadVLD(u4VDecID, RO_VP8_ERTR);
    prVp8ErrInfo->u4YoutRangeErr = (u4RegVal & 0x1);
    prVp8ErrInfo->u4XoutRangeErr = (u4RegVal & 0x2) >> 1;
    prVp8ErrInfo->u4BlkCoefErr = (u4RegVal & 0x4) >> 2; 
    prVp8ErrInfo->u4BitCnt1stPartErr = (u4RegVal & 0x10) >>4;
    prVp8ErrInfo->u4BitCnt2stPartErr = (u4RegVal & 0x20) >>5;
    prVp8ErrInfo->u4MCBusyOverflowErr = (u4RegVal & 0x1000)>>12;
    prVp8ErrInfo->u4MDecTimoutInt = (u4RegVal & 0x80000000)>>31;
    prVp8ErrInfo->u4Vp8ErrCnt = 0;

    return;
}

// **************************************************************************
// Function : UINT32 u4VDEC_HAL_VP8_GetErrType(UINT32 u4VDecID);
// Description :Read Vp8 error type after decoding end
// Parameter : u4VDecID : video decoder hardware ID
// Return      : Vp8 decode error type value
// **************************************************************************
UINT32 u4VDEC_HAL_VP8_GetErrType(UINT32 u4VDecID)
{
    UINT32 u4RegVal = 0;
    return u4RegVal;
}

//extern BOOL _VDecNeedDumpRegister(UINT32 u4VDecID);

// **************************************************************************
// Function : INT32 i4VDEC_HAL_VP8_DecStart(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm);
// Description :Set video decoder hardware registers to decode for Vp8
// Parameter : prDecVp8Prm : pointer to Vp8 decode info struct
// Return      : =0: success.
//                  <0: fail.
// **************************************************************************
//INT32 i4VDEC_HAL_VP8_DecStart(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm)
UINT32 i4VDEC_HAL_VP8_DecStart(UINT32 u4VDecID, VDEC_INFO_VP8_FRM_HDR_T *pVp8DecInfo)
{
   UINT32 u4HalValue=0;
   VDEC_PARAM_VP8DEC_T *prVp8DecParam=&pVp8DecInfo->rVp8DecParam;
   UINT32 u4FlagParam=prVp8DecParam->u4FlagParam;

   #if VMMU_SUPPORT
   UINT32 u4Page_addr;
   #endif
   //MC setting
/*
   u4HalValue=(pVp8DecInfo->u2Width+15)>>4;
   vVDecWriteVP8MC(u4VDecID, RW_MC_UMV_PIC_WIDTH,u4HalValue);
   u4HalValue=(pVp8DecInfo->u2Height+15)>>4;
   vVDecWriteVP8MC(u4VDecID, RW_MC_UMV_PIC_HEIGHT,u4HalValue);
   */

   vVDecWriteVP8MC(u4VDecID, RW_MC_UMV_PIC_WIDTH,(pVp8DecInfo->u4Width+15)&0xfff0);
   vVDecWriteVP8MC(u4VDecID, RW_MC_UMV_PIC_HEIGHT,(pVp8DecInfo->u4Height+15)&0xfff0);

#if VDEC_DDR3_SUPPORT   
   //vVDecWriteVP8MC(u4VDecID, RW_MC_PIC_W_MB, ((pVp8DecInfo->u2Width + 63)>> 4) & 0x1FF);
   vVDecWriteVP8MC(u4VDecID, RW_MC_PIC_W_MB, ((pVp8DecInfo->u4Width+63)>>6)<<2);
   vVDecWriteVP8MC(u4VDecID, RW_MC_ADDRSWAP,ADDRSWAP_DDR3);
   vVDecWriteVP8MC(u4VDecID, RW_MC_NBM_CTRL, ((u4VDecReadVP8MC(u4VDecID,RW_MC_NBM_CTRL)& 0xFFFFFFF8)|ADDRSWAP_DDR3));
#else
   vVDecWriteVP8MC(u4VDecID, RW_MC_PIC_W_MB, ((pVp8DecInfo->u4Width + 15)>> 4) & 0x3FF);
   vVDecWriteVP8MC(u4VDecID, RW_MC_ADDRSWAP,ADDRSWAP_OFF);
   vVDecWriteVP8MC(u4VDecID, RW_MC_NBM_CTRL, ((u4VDecReadVP8MC(u4VDecID,RW_MC_NBM_CTRL)& 0xFFFFFFF8)|ADDRSWAP_OFF));
#endif 

   //RW_MC_VP8SETTING
   u4HalValue=u4VDecReadVP8MC(u4VDecID, RW_MC_VP8SETTING);
   u4HalValue=u4HalValue&0x1;
   VDEC_REG_SET_FLAG(u4HalValue,VDEC_FLGSET(u4FlagParam,VP8PARAM_FULL_PIXEL),RW_MC_VP8FULLPEL_FLAG);
   if(!VDEC_FLGSET(u4FlagParam,VP8PARAM_BILINER_MCFILTER))
   {
      VDEC_REG_SET_FLAG(u4HalValue,1,RW_MC_BILINEAR_OR_SIXTAP_FLAG);
   }
   vVDecWriteVP8MC(u4VDecID, RW_MC_VP8SETTING,u4HalValue);

   ASSERT(((pVp8DecInfo->u4CurCAddr-pVp8DecInfo->u4CurYAddr) & 0x7F) == 0);
   ASSERT((pVp8DecInfo->u4GldYAddr & 0x1FF) == 0);
   ASSERT((pVp8DecInfo->u4AlfYAddr & 0x1FF) == 0);
   ASSERT((pVp8DecInfo->u4LstYAddr & 0x1FF) == 0);
   vVDecWriteVP8MC(u4VDecID, RW_MC_LUMA_SIZE,(pVp8DecInfo->u4CurCAddr-pVp8DecInfo->u4CurYAddr));
   #if VMMU_SUPPORT
   u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((pVp8DecInfo->u4LstYAddr)/(4*1024))*4);
  // printk("Page Addr = 0x%x\n", u4Page_addr);
   vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32)pVp8DecInfo->u4LstYAddr), PHYSICAL((UINT32)pVp8DecInfo->u4LstYAddr) + PIC_Y_SZ+PIC_C_SZ);
   vVDecWriteVP8MC(u4VDecID, RW_MC_PIC1Y_ADD, (pVp8DecInfo->u4LstYAddr));
   
   u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((pVp8DecInfo->u4GldYAddr)/(4*1024))*4);
  // printk("Page Addr = 0x%x\n", u4Page_addr);
   vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32)pVp8DecInfo->u4GldYAddr), PHYSICAL((UINT32)pVp8DecInfo->u4GldYAddr) + PIC_Y_SZ+PIC_C_SZ);
   vVDecWriteVP8MC(u4VDecID, RW_MC_PIC2Y_ADD, (pVp8DecInfo->u4GldYAddr));
   
   u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((pVp8DecInfo->u4AlfYAddr)/(4*1024))*4);
  // printk("Page Addr = 0x%x\n", u4Page_addr);
   vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32)pVp8DecInfo->u4AlfYAddr), PHYSICAL((UINT32)pVp8DecInfo->u4AlfYAddr) + PIC_Y_SZ+PIC_C_SZ);
   vVDecWriteVP8MC(u4VDecID, RW_MC_PIC3Y_ADD, (pVp8DecInfo->u4AlfYAddr));
   
   #else
   vVDecWriteVP8MC(u4VDecID, RW_MC_PIC1Y_ADD, PHYSICAL((UINT32)pVp8DecInfo->u4LstYAddr));
   vVDecWriteVP8MC(u4VDecID, RW_MC_PIC2Y_ADD, PHYSICAL((UINT32)pVp8DecInfo->u4GldYAddr));
   vVDecWriteVP8MC(u4VDecID, RW_MC_PIC3Y_ADD, PHYSICAL((UINT32)pVp8DecInfo->u4AlfYAddr));
   #endif
   vVDecWriteVP8MC(u4VDecID, RW_VLD_MBDRAM_SEL, u4VDecReadVP8MC(u4VDecID, RW_VLD_MBDRAM_SEL) | (1 << 16));
//printk("LoopFilter = %d\n", prVp8DecParam->iLoopFilterLvl);
//#if (!VDEC_DDR3_SUPPORT && !VDEC_VP8_WRAPPER_OFF)
//   if(prVp8DecParam->iLoopFilterLvl>0)
//#endif
   {
      vVDecWriteVP8MC(u4VDecID, RW_MC_PP_ENABLE, 0x00000001);
      vVDecWriteVP8MC(u4VDecID, RW_MC_PP_WB_BY_POST, 0x00000001);
   #if VMMU_SUPPORT
      u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((pVp8DecInfo->u4CurYAddr)/(4*1024))*4);
  // printk("Page Addr = 0x%x\n", u4Page_addr);
      vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32)pVp8DecInfo->u4CurYAddr), PHYSICAL((UINT32)pVp8DecInfo->u4CurYAddr) + PIC_Y_SZ);
      vVDecWriteVP8MC(u4VDecID, RW_MC_PP_Y_ADDR, (pVp8DecInfo->u4CurYAddr) >> 9);
      
      u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((pVp8DecInfo->u4CurCAddr)/(4*1024))*4);
 //  printk("Page Addr = 0x%x\n", u4Page_addr);
      vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32)pVp8DecInfo->u4CurCAddr), PHYSICAL((UINT32)pVp8DecInfo->u4CurCAddr) + PIC_C_SZ);
      vVDecWriteVP8MC(u4VDecID, RW_MC_PP_C_ADDR, (pVp8DecInfo->u4CurCAddr) >> 8);
      
      vVDecVMMUEnable(PHYSICAL((UINT32)_pucVMMUTable[u4VDecID]));
   #else
      vVDecWriteVP8MC(u4VDecID, RW_MC_PP_Y_ADDR, PHYSICAL((UINT32)pVp8DecInfo->u4CurYAddr) >> 9);
      vVDecWriteVP8MC(u4VDecID, RW_MC_PP_C_ADDR, PHYSICAL((UINT32)pVp8DecInfo->u4CurCAddr) >> 8);
   #endif
#if VDEC_DDR3_SUPPORT
      vVDecWriteVP8MC(u4VDecID, RW_MC_PP_MB_WIDTH, ((pVp8DecInfo->u4Width+63)>>6)<<2);
#else
      vVDecWriteVP8MC(u4VDecID, RW_MC_PP_MB_WIDTH, (pVp8DecInfo->u4Width+15)>>4);
#endif
      vVDecWriteVP8MC(u4VDecID, RW_MC_PP_X_RANGE, ((pVp8DecInfo->u4Width + 15) >> 4) - 1);
      vVDecWriteVP8MC(u4VDecID, RW_MC_PP_Y_RANGE, ((pVp8DecInfo->u4Height+ 15) >> 4) - 1);
      //how set 
      //RW_MC_PP_DBLK_Y_ADDR 0x2C8
      //RW_MC_PP_DBLK_C_ADDR 0x2CC
/*      if(prVp8DecParam->eLoopFilterType==VP8_LF_TYPE_NORMAL)
      {
        vVDecWriteVP8MC(u4VDecID, RW_MC_PP_DBLK_MODE,3);
      }
      else
      {
        vVDecWriteVP8MC(u4VDecID, RW_MC_PP_DBLK_MODE,2);
      }
      */
   }
//#if (!VDEC_DDR3_SUPPORT && !VDEC_VP8_WRAPPER_OFF)
/*   else
   {
      vVDecWriteVP8MC(u4VDecID, RW_MC_OPBUF, 4);
      vVDecWriteVP8MC(u4VDecID, RW_MC_BY1, PHYSICAL((UINT32)pVp8DecInfo->u4CurYAddr)>>9);
      vVDecWriteVP8MC(u4VDecID, RW_MC_BC1, PHYSICAL((UINT32)pVp8DecInfo->u4CurCAddr)>>8);
      vVDecWriteVP8MC(u4VDecID, RW_MC_PP_ENABLE,0);
      vVDecWriteVP8MC(u4VDecID, RW_MC_PP_WB_BY_POST,0);
      vVDecWriteVP8MC(u4VDecID, RW_MC_PP_MB_WIDTH,((pVp8DecInfo->u4Width + 15) >> 4) - 1); 
   }
   #endif
   */
//   if (pVp8DecInfo->u4Width> 1920)
//   {
//      vVDecWriteVP8MC(u4VDecID, RW_MC_WRAPPER_SWITCH, 1);
//   }
//   else
   {
      vVDecWriteVP8MC(u4VDecID, RW_MC_WRAPPER_SWITCH, 0);
   }
#if VDEC_VP8_WRAPPER_OFF
    vVDecWriteVLDTOP(u4VDecID, RW_VLD_TOP_PRED_ADDR, PHYSICAL(pVp8DecInfo->u4VLDWrapper));
    vVDecWriteVLDTOP(u4VDecID, RW_VLD_TOP_SEGID_DRAM_ADDR, PHYSICAL(pVp8DecInfo->u4SegIdWrapper));
    vVDecWriteVLDTOP(u4VDecID, RW_VLD_TOP_PRED_SRAM_CFG, (RW_PRED_SRAM_CFG_SRAM_BYPASS_FLAG|RW_PRED_SRAM_CFG_OUTRANGE_SUPPORT_FLAG)); // bit[30]: vld wrapper bypass on

    u4HalValue=0x80000000;
    VDEC_REG_SET_FLAG(u4HalValue,1,RW_PRED_SRAM_CFG_SRAM_BYPASS_FLAG);
    vVDecWriteVLDTOP(u4VDecID, RW_VLD_TOP_PRED_SRAM_CFG, u4HalValue); // bit[30]: vld wrapper bypass on

    u4HalValue=0;
    VDEC_REG_SET_VALUE(u4HalValue,3,RW_NUM_ROW_SEGID_DATA_M1_S,RW_NUM_ROW_SEGID_DATA_M1_E);
    VDEC_REG_SET_VALUE(u4HalValue,1,RW_DRAM_BURST_MODE_S,RW_DRAM_BURST_MODE_E);
    ASSERT(u4HalValue==0x13);
    vVDecWriteVLDTOP(u4VDecID, RW_VLD_TOP_PRED_MODE, u4HalValue); // pred read dram burst

    vVDecWriteVP8MC(u4VDecID, RW_MC_DDR3_EN, 1);
    vVDecWriteVP8MC(u4VDecID, RW_MC_WRAPPER_SWITCH, 1);
//    vVDecWriteVP8PP(u4VDecID, RW_PP_SPECIAL_MODE, u4VDecReadVP8PP(u4VDecID, RW_PP_SPECIAL_MODE) | 0x101000); /// for webp get-lucky.webp.ivf use.
    vVDecWriteVP8PP(u4VDecID, RW_PP_SPECIAL_MODE, u4VDecReadVP8PP(u4VDecID, RW_PP_SPECIAL_MODE) | 0x1000);
    vVDecWriteVP8MC(u4VDecID, RW_MC_PP_DBLK_Y_ADDR,PHYSICAL(pVp8DecInfo->u4PPWrapperY)>>4);
    vVDecWriteVP8MC(u4VDecID, RW_MC_PP_DBLK_C_ADDR,PHYSICAL(pVp8DecInfo->u4PPWrapperC)>>4);
   
#if (CONFIG_DRV_FPGA_BOARD)
   vVDecWriteVP8MC(u4VDecID, RW_MC_MODE_CTL, (u4VDecReadVP8MC(u4VDecID, RW_MC_MODE_CTL) |(0x1 <<12)) );
 //  vVDecWriteVLD(u4VDecID, 0xCC, 0); // no idle, accelerate decode speed.
#endif
//   vVDecWriteVP8MC(u4VDecID, 0x660, (u4VDecReadVP8MC(u4VDecID, 0x660) |(0x80000000)));
#else   
#if (CONFIG_DRV_FPGA_BOARD)
   vVDecWriteVP8MC(u4VDecID, RW_MC_MODE_CTL, (u4VDecReadVP8MC(u4VDecID, RW_MC_MODE_CTL) |(0x1 <<12)) );
//   vVDecWriteVLD(u4VDecID, 0xCC, 0); // no idle, accelerate decode speed.
//   vVDecWriteVP8MC(u4VDecID, 0x91C, (u4VDecReadVP8MC(u4VDecID, 0x91C) &(0xFFFFFF3F)));
//   vVDecWriteVP8MC(u4VDecID, 0x81C, (u4VDecReadVP8MC(u4VDecID, 0x81C) &(0xF000FFFF)));
#endif
//   vVDecWriteVP8MC(u4VDecID, 0x660, (u4VDecReadVP8MC(u4VDecID, 0x660) |(0x80000000)));
#endif
    vVDecWriteVP8MC(u4VDecID, RW_MC_VLD_WRAPPER,PHYSICAL(pVp8DecInfo->u4VLDWrapperWrok));
    vVDecWriteVP8MC(u4VDecID, RW_MC_PP_WRAPPER,PHYSICAL(pVp8DecInfo->u4PPWrapperWrok));
    if (pVp8DecInfo->u4Width < 176)
    	{
    	vVDecWriteDV(u4VDecID, VDEC_DV_LAT_BUF_BYPASS, u4VDecReadDV(u4VDecID, VDEC_DV_LAT_BUF_BYPASS) |0x1);
    	}
    else
    	{
    	vVDecWriteDV(u4VDecID, VDEC_DV_LAT_BUF_BYPASS, u4VDecReadDV(u4VDecID, VDEC_DV_LAT_BUF_BYPASS) & 0xFFFFFFFE);
    	}


    //PP Setting
    if((prVp8DecParam->iLoopFilterLvl)>0)
    { 
      UINT32 u4Index=0;
      INT32 i4Level=0;
      if(prVp8DecParam->eLoopFilterType==VP8_LF_TYPE_NORMAL)
      {
        vVDecWriteVP8MC(u4VDecID, RW_MC_PP_DBLK_MODE,3);
      }
      else
      {
        vVDecWriteVP8MC(u4VDecID, RW_MC_PP_DBLK_MODE,2);
      }

      u4HalValue=1; //VP8 ENABLE
      VDEC_REG_SET_FLAG(u4HalValue,prVp8DecParam->eLoopFilterType,RW_PP_FLTTYPE_FLAG);
      VDEC_REG_SET_FLAG(u4HalValue,VDEC_FLGSET(u4FlagParam,VP8PARAM_MODEREF_LFDELTA_ENABLE),RW_PP_MODEREF_DELTA_ENABLE_FLAG);
      VDEC_REG_SET_FLAG(u4HalValue, pVp8DecInfo->uFrameType,RW_PP_FRAMETYPE_FLAG);
      VDEC_REG_SET_VALUE(u4HalValue, prVp8DecParam->iSharpLvl, RW_PP_SHARPLVL_S, RW_PP_SHARPLVL_E);
      vVDecWriteVP8PP(u4VDecID, RW_PP_VP8PARA,u4HalValue);
      
      u4HalValue=0;
      for(u4Index=0;u4Index<MAX_MB_SEGMENTS;u4Index++)
      {
         if(VDEC_FLGSET(u4FlagParam,VP8PARAM_SEGMENT_ENABLE))
         {
            if(VDEC_FLGSET(u4FlagParam,VP8PARAM_SEGMENT_ABSDATA))
            {
               i4Level=prVp8DecParam->SegmentFeatureData[VP8_MBLVL_ALT_LF][u4Index];
            }
            else
            {
               i4Level=prVp8DecParam->iLoopFilterLvl
                        +prVp8DecParam->SegmentFeatureData[VP8_MBLVL_ALT_LF][u4Index];
               i4Level=i4Level>0 ? (i4Level<=MAX_FILTER_LVL ? i4Level : MAX_FILTER_LVL) : 0;
            }
         }
         else
         {
            i4Level=prVp8DecParam->iLoopFilterLvl;
         }
         u4HalValue|=i4Level<<(u4Index*8);
      }

      vVDecWriteVP8PP(u4VDecID, RW_PP_VP8FILTER_LEVEL,u4HalValue);
      u4HalValue=0;
      for(u4Index=0;u4Index<MAX_REF_LF_DELTAS;u4Index++)
      {
        u4HalValue|=(((UINT8)prVp8DecParam->RefLFDeltas[u4Index])<<(u4Index*8));
      }
      vVDecWriteVP8PP(u4VDecID, RW_PP_VP8REFDELTA,u4HalValue);
      u4HalValue=0;
      for(u4Index=0;u4Index<MAX_MODE_LF_DELTAS;u4Index++)
      {
        u4HalValue|=(((UINT8)prVp8DecParam->ModeLFDeltas[u4Index])<<(u4Index*8));
      }
      vVDecWriteVP8PP(u4VDecID, RW_PP_VP8MODEDELTA,u4HalValue);
   }

#if VDEC_DDR3_SUPPORT
   //Turn on DDR3 mode
   vVDecWriteVP8MC(u4VDecID, RW_MC_DDR_CTRL0,((u4VDecReadVP8MC(u4VDecID, RW_MC_DDR_CTRL0)  & 0xFFFFFFFE) |0x1));    
   vVDecWriteVP8MC(u4VDecID, RW_MC_DDR_CTRL1,((u4VDecReadVP8MC(u4VDecID, RW_MC_DDR_CTRL1)  & 0xFFFFFFFE) |0x1));       
   vVDecWriteVP8MC(u4VDecID, (406<<2), u4VDecReadVP8MC(u4VDecID, (406<<2)) & 0xFFFFFFEF);
         
   //Turn-on DDR3, Set 0x834[0] = 0
   vVDecWriteVP8MC(u4VDecID, RW_MC_DDR3_EN, (u4VDecReadVP8MC(u4VDecID, RW_MC_DDR3_EN)  & 0xFFFFFFFE));
   //DDR3 Output Selector:
   //a. MC Only:  RW_MC_PP_ENABLE = 0 && RW_MC_PP_WB_BY_POST = 0
   //b. MC+PP:    RW_MC_PP_ENABLE = 1 && RW_MC_PP_WB_BY_POST = 0
   //c. PP Only:   RW_MC_PP_ENABLE = 1 && RW_MC_PP_WB_BY_POST = 1
   //Post-process enable
   vVDecWriteVP8MC(u4VDecID, RW_MC_PP_ENABLE, (u4VDecReadVP8MC(u4VDecID, RW_MC_PP_ENABLE)  | 0x1));
   //Writeback by PP
   //if RW_MC_PP_WB_BY_POST =1, then Only output to PP Buffer.
   //if RW_MC_PP_WB_BY_POST = 0,then ouput to both PP & MC
   //vVDecWriteMC(u4VDecID, RW_MC_PP_WB_BY_POST, 0x00000001);
#endif

#if VDEC_VP8_WEBP_SUPPORT
#if VP8_MB_ROW_MODE_SUPPORT
       {
       //UINT32 u4Index=0;
      
       // need width greate than 16, is ic problem.
       if(prVp8DecParam->iLoopFilterLvl>0 && prVp8DecParam->eLoopFilterType==VP8_LF_TYPE_SIMPLE)
       {
           vVDecWriteVP8PP(u4VDecID, RW_PP_REG_17, 1);
           vVDecWriteVP8MC(u4VDecID, RW_MC_PP_DBLK_MODE, 3);
       }
       
       if(pVp8DecInfo->u4Width>16/* && pVp8DecInfo->u2Height>=16*/)
       {
          u4HalValue=u4VDecReadVLDTOP(u4VDecID, RW_VLD_TOP_WEBP_CTRL);
          VDEC_REG_SET_FLAG(u4HalValue,1,RW_BUFCTRL_ON_FLAG);
   //       if(VDEC_FLGSET(u4FlagParam,VP8PARAM_WEBP_PICMODE))
   //       {
  //           VDEC_REG_SET_FLAG(u4HalValue,1,RW_RESIZE_INTER_ON_FLAG);
  //        }
          
          vVDecWriteVLDTOP(u4VDecID, RW_VLD_TOP_WEBP_CTRL,u4HalValue);
          vVDecWriteVP8PP(u4VDecID, RW_PP_BRC_OFFSET,((pVp8DecInfo->u4Width + 15) >> 4)*32);
       }

/*       if(VDEC_FLGSET(u4FlagParam,VP8PARAM_RINGBUF_CTRL))
       {
          VP8_VLD_WRITE32(RW_VP8_PART_CHG_PAUSE,VP8_VLD_READ32(RW_VP8_PART_CHG_PAUSE)|0x1);
          for(u4Index=0;u4Index<pVp8DecInfo->rVp8DecParam.uDataPartitionNum;u4Index++)
          {
			  VP8_VLD_WRITE32(RW_VP8_COEFR0+u4Index*4,0);
          }
       }
       else if(VDEC_FLGSET(u4FlagParam,VP8PARAM_WEBP_RESET_BUF))
       {
           VP8_VLD_WRITE32(RW_VP8_COEFR0,pVp8DecInfo->u4VldStartPos);
          for(u4Index=1;u4Index<pVp8DecInfo->rVp8DecParam.uDataPartitionNum;u4Index++)
          {
			  VP8_VLD_WRITE32(RW_VP8_COEFR0+u4Index*4,VP8_VLD_READ32(RW_VP8_COEFR0+((u4Index-1)*4))+pVp8DecInfo->u4rPartitionSize[u4Index]);
          }
       }
       */
}
#endif
    vVDecWriteVP8MC(u4VDecID, RW_MC_PP_ENABLE, 0x1);                // PP always on
    vVDecWriteVP8MC(u4VDecID, RW_MC_PP_WB_BY_POST, 0x1);            // write recon. frame by dbk always on
    u4HalValue = u4VDecReadVLDTOP(u4VDecID, RW_VLD_TOP_WEBP_CTRL);
    printk("RW_VLD_TOP_WEBP_CTRL read = 0x%x\n", u4HalValue);
#if VP8_MB_ROW_MODE_SUPPORT_ME2_INTEGRATION
    u4HalValue = (u4HalValue | 0x01);
#else   //VDEC_VP8_WEBP_SUPPORT_ME2_INTEGRATION
    u4HalValue = (u4HalValue | 0x11);
#endif
    printk("RW_VLD_TOP_WEBP_CTRL write = 0x%x\n", u4HalValue);
    vVDecWriteVLDTOP(u4VDecID, RW_VLD_TOP_WEBP_CTRL, u4HalValue);   // bit[0]: buf_ctrl on / off, bit[4]: resz interface on/off
#if VP8_MB_ROW_MODE_SUPPORT_ME2_INTEGRATION
    u4HalValue = ((pVp8DecInfo->u4Width+15)/16) * 128;               // frame buffer width in MB * 128
#else   //VDEC_VP8_WEBP_SUPPORT_ME2_INTEGRATION
    u4HalValue = ((pVp8DecInfo->u4Width+15)/16) * 64;               // frame buffer width in MB * 64
#endif
    vVDecWriteVP8PP(u4VDecID, 0x70, u4HalValue);
#if VP8_MB_ROW_MODE_SUPPORT_ME2_INTEGRATION
    vVDecWriteVP8PP(u4VDecID, 0x6C, 0x2);                           // MB row size = 8
#else   //VDEC_VP8_WEBP_SUPPORT_ME2_INTEGRATION
    vVDecWriteVP8PP(u4VDecID, 0x6C, 0x1);                           // MB row size = 4
#endif
#endif

#if VDEC_VER_COMPARE_CRC
    u4HalValue=u4VDecReadCRC(u4VDecID, VDEC_CRC_REG_EN);
    ASSERT(u4HalValue&0x1);
    u4HalValue=((u4VDecReadVP8MC(u4VDecID, RW_MC_PP_ENABLE)&0x1)<<1)+1;
    vVDecWriteCRC(u4VDecID, VDEC_CRC_REG_EN,u4HalValue);
#endif    
#if 0//VDEC_VER_COMPARE_CRC
    UINT32 u4CRCSrc = 0x1; 
    u4CRCSrc = (VDEC_CRC_EN | VDEC_CRC_SRC_MC);   //CRC input from MC
    //u4CRCSrc = (VDEC_CRC_EN | VDEC_CRC_SRC_PP);  //CRC input from PP
    vVDecWriteCRC(u4VDecID, 0x4, u4CRCSrc);
#endif

#if CONFIG_DRV_VERIFY_SUPPORT
    if (_fgVP8DumpReg)
    {
    	printk("******* INPUT Window *******\n");
    	printk("VP8_VLD1 VBSR1: 0x%08x\n", u4VDecReadVP8VLD(0, RO_VP8_VBSR1));
    	printk("VP8_VLD2 VBSR2: 0x%08x\n", u4VDecReadVP8VLD(0, RO_VP8_VBSR2));
    }
#ifdef VP8_REG_DUMP
    if (_fgVP8DumpReg)
    {
    	printk("******* INPUT Window *******\n");
    	printk("VP8_VLD1 VBSR1: 0x%08x\n", u4VDecReadVP8VLD(0, RO_VP8_VBSR1));
    	printk("VP8_VLD2 VBSR2: 0x%08x\n", u4VDecReadVP8VLD(0, RO_VP8_VBSR2));
    }
#endif
#endif    
       prVp8DecParam->u4LastParam=u4FlagParam;
//    	printk("RW_PP_SPECIAL_MODE: 0x%08x\n", u4VDecReadVP8PP(u4VDecID, RW_PP_SPECIAL_MODE));

   vVDecWriteVLDTOP(u4VDecID, RW_VLD_TOP_VP8_ENHANCE, 0);
   vVDecWriteVP8MC(u4VDecID, RW_MC_VP8RANDOMERRSET, (u4VDecReadVP8MC(u4VDecID, RW_MC_VP8RANDOMERRSET) |RW_MC_VP8RANDOM_ERR));
 //      memset(_pucWorkYBuf[u4VDecID] , 0x0A, PIC_Y_SZ+PIC_C_SZ);

#if(CONFIG_DRV_VERIFY_SUPPORT && !MEM_ALLOCATE_IOREMAP)    
        _pu1VP8VDecYAddrPhy = (UINT8 *)dma_map_single(NULL, pVp8DecInfo->u4CurYAddr, GOLD_Y_SZ+GOLD_C_SZ, DMA_FROM_DEVICE);
        _pu1VP8VDecYAltPhy = (UINT8 *)dma_map_single(NULL, pVp8DecInfo->u4LstYAddr, GOLD_Y_SZ+GOLD_C_SZ, DMA_TO_DEVICE);
        _pu1VP8VDecYCurPhy = (UINT8 *)dma_map_single(NULL, pVp8DecInfo->u4AlfYAddr, GOLD_Y_SZ+GOLD_C_SZ, DMA_TO_DEVICE);
        _pu1VP8VDecYGldPhy = (UINT8 *)dma_map_single(NULL, pVp8DecInfo->u4GldYAddr, GOLD_Y_SZ+GOLD_C_SZ, DMA_TO_DEVICE);
  #endif    

   //Trigger decoder
   vVDecWriteVP8VLD(u4VDecID, RW_VP8_HDR,1,0);
   vVDecWriteVP8VLD(u4VDecID, RW_VP8_HDR,0,0);
   return 0;
}



// **************************************************************************
// Function : INT32 i4VDEC_HAL_VP8_VPStart(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm);
// Description :Set video decoder hardware registers to vp for Vp8
// Parameter : prDecVp8Prm : pointer to Vp8 decode info struct
// Return      : =0: success.
//                  <0: fail.
// **************************************************************************
INT32 i4VDEC_HAL_VP8_VPStart(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm)
{
    
              
    return HAL_HANDLE_OK;
}


// **************************************************************************
// Function : INT32 i4VDEC_HAL_VP8_VPStart(UINT32 u4VDecID);
// Description :Set video decoder hardware registers to vp for Vp8
// Parameter : prDecVp8Prm : pointer to Vp8 decode info struct
// Return      : =0: success.
//                  <0: fail.
// **************************************************************************
/// Read DDR3 Decode Finish Flag
/// \return 
INT32 i4VDEC_HAL_VP8_DDR3_DecFinish(UINT32 u4VDecID)
{
    ///Check!!! 
    //     UINT32 u4RegVal = 0;
     
    ///Check!!! 
 //    u4RegVal = u4VDecReadVLD(u4VDecID, RO_VP8_VLD_DDR3_Finish);
     
//     if (u4RegVal & VP8_DDR3_FINISH)
     	return TRUE;
//     else
//     	return FALSE;
}


// The following functions are only for verify
void vVDEC_HAL_VP8_DecEndProcess(UINT32 u4VDecID)
{

}


void vVDEC_HAL_VP8_VDec_DumpReg(UINT32 u4VDecID, BOOL fgBefore)
{
  INT32 i; 
  INT32 i4VldStart = 42;
  INT32 i4VldEnd = 256;
  INT32 i4McStart = 0;
  INT32 i4McEnd = 400;
  UINT32 u4Data;
  
  printk("VP8 Dump Register: ");
  if (fgBefore == TRUE)
  {
     printk("Before Decode\n");
  }
  else
  {
     printk("After Decode\n");
  }

  printk("VLD Registers\n");
  for (i= i4VldStart; i<i4VldEnd; i++)
  {
      u4Data = u4VDecReadVLD(u4VDecID, (i<<2));
      printk("%04d (0x%04X) = 0x%08X\n", i, (i<<2), u4Data);
  }

  printk("MC Registers\n");
  for (i=i4McStart; i<i4McEnd; i++)
  {
     u4Data = u4VDecReadMC(u4VDecID, (i<<2));
     printk("%04d (0x%04X) = 0x%08X\n", i, (i<<2), u4Data);
  }

    i = 147;
    u4Data = u4VDecReadVLD(u4VDecID, (i<<2));
    printk("%04d (0x%04X) = 0x%08X\n", i, (i<<2), u4Data);

     i = 148;
     u4Data = u4VDecReadVLD(u4VDecID, (i<<2));
     printk("%04d (0x%04X) = 0x%08X\n", i, (i<<2), u4Data);
     
    i = 147;
    u4Data = u4VDecReadVLD(u4VDecID, (i<<2));
    printk("%04d (0x%04X) = 0x%08X\n", i, (i<<2), u4Data);

     i = 148;
     u4Data = u4VDecReadVLD(u4VDecID, (i<<2));
     printk("%04d (0x%04X) = 0x%08X\n", i, (i<<2), u4Data);

         i = 147;
    u4Data = u4VDecReadVLD(u4VDecID, (i<<2));
    printk("%04d (0x%04X) = 0x%08X\n", i, (i<<2), u4Data);

     i = 148;
     u4Data = u4VDecReadVLD(u4VDecID, (i<<2));
     printk("%04d (0x%04X) = 0x%08X\n", i, (i<<2), u4Data);

         i = 147;
    u4Data = u4VDecReadVLD(u4VDecID, (i<<2));
    printk("%04d (0x%04X) = 0x%08X\n", i, (i<<2), u4Data);

     i = 148;
     u4Data = u4VDecReadVLD(u4VDecID, (i<<2));
     printk("%04d (0x%04X) = 0x%08X\n", i, (i<<2), u4Data);
    
}


// **************************************************************************
// Function : UINT32 u4VDEC_HAL_VP8_GetBoolCoderShift(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits);
// Description :Read Barrel Shifter before shifting
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
//                 u4ShiftBits : shift bits number
// Return      : Value of barrel shifter input window before shifting
// **************************************************************************
UINT32 u4VDEC_HAL_VP8_GetBoolCoderShift(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits)
{
    UINT32 u4RegVal0;
  
    u4RegVal0 = u4VDecVP8BOOLGetBits(u4BSID, u4VDecID, u4ShiftBits);
    
    return(u4RegVal0);
}


// **************************************************************************
// Function : UINT32 u4VDEC_HAL_VP8_InitBoolCoder(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits);
// Description :Read Barrel Shifter before shifting
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
//                 u4ShiftBits : shift bits number
// Return      : Value of barrel shifter input window before shifting
// **************************************************************************
UINT32 u4VDEC_HAL_VP8_InitBoolCoder(UINT32 u4BSID, UINT32 u4VDecID)
{  
   if(u4BSID==VP8_VLD1)
   {
      return u4VDecReadVP8VLD(u4VDecID, RO_VP8_BINIT1);
   }
   else
   {
      return u4VDecReadVP8VLD(u4VDecID, RO_VP8_BINIT2);
   }
}


// **************************************************************************
// Function : UINT32 u4VDEC_HAL_Default_Models_Init(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4VFIFOSa, UINT32 *pu4Bits);
// Description :Read current read pointer
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
//                 u4VFIFOSa : video FIFO start address
//                 pu4Bits : read pointer value with remained bits
// Return      : Read pointer value with byte alignment
// **************************************************************************
UINT32 u4VDEC_HAL_VP8_Default_Models_Init(UINT32 u4BSID, UINT32 u4VDecID)
{
	UINT32 pred_range, mbt_range, mvd_range, cfm_range;
	UINT32 i, j, u4Val;
	UINT32 u4Tmp0, u4Tmp1, u4Tmp2, u4Tmp3;
	UCHAR vector_model_dct[2];
	UCHAR vector_model_sig[2];
       UCHAR vp56_def_mb_types_stats[3][10][2] = 
       {
             { {  69, 42 }, {   1,  2 }, {  1,   7 }, {  44, 42 }, {  6, 22 },
                {   1,  3 }, {   0,  2 }, {  1,   5 }, {   0,  1 }, {  0,  0 }, },
             { { 229,  8 }, {   1,  1 }, {  0,   8 }, {   0,  0 }, {  0,  0 },
                {   1,  2 }, {   0,  1 }, {  0,   0 }, {   1,  1 }, {  0,  0 }, },
             { { 122, 35 }, {   1,  1 }, {  1,   6 }, {  46, 34 }, {  0,  0 },
                {   1,  2 }, {   0,  1 }, {  0,   1 }, {   1,  1 }, {  0,  0 }, },
       };

       UCHAR vp8_def_pdv_vector_model[2][7] = 
       {    
       	{ 225, 146, 172, 147, 214,  39, 156 },   
       	{ 204, 170, 119, 235, 140, 230, 228 },
       };

       UCHAR vp8_def_fdv_vector_model[2][8] = 
       {    
              { 247, 210, 135, 68, 138, 220, 239, 246 },
              { 244, 184, 201, 44, 173, 221, 239, 253 },
       };

       UCHAR vp8_def_runv_coeff_model[2][14] = 
       {    { 198, 197, 196, 146, 198, 204, 169, 142, 130, 136, 149, 149, 191, 249 },
             { 135, 201, 181, 154,  98, 117, 132, 126, 146, 169, 184, 240, 246, 254 },
       };

       UINT32 Log_def_mb_types_stats_50[3][2];
       UINT32 Log_def_mb_types_stats_51[3][2];
       UINT32 Log_def_mb_types_stats_52[3][2];
       UINT32 Log_def_mb_types_stats_53[3][2];
       UINT32 Log_def_mb_types_stats_54[3][2];

       UINT32 Log_vector_model_dct[5];
       UINT32 Log_def_pdv_vector_model[5];              
       UINT32 Log_def_fdv_vector_model[5];       
       UINT32 Log_def_runv0_coeff_model[5];
       UINT32 Log_def_runv1_coeff_model[5];
       
       vector_model_dct[0] = 0xA2;
       vector_model_dct[1] = 0xA4;
       vector_model_sig[0] = 0x80;
       vector_model_sig[1] = 0x80;
	
//    MEMCPY_ALIGNED(s->mb_types_stats, vp56_def_mb_types_stats, sizeof(s->mb_types_stats));
//    MEMCPY_ALIGNED(s->vector_model_fdv, vp8_def_fdv_vector_model, sizeof(s->vector_model_fdv));
//    MEMCPY_ALIGNED(s->vector_model_pdv, vp8_def_pdv_vector_model, sizeof(s->vector_model_pdv));
//    MEMCPY_ALIGNED(s->coeff_model_runv, vp8_def_runv_coeff_model, sizeof(s->coeff_model_runv));
//    MEMCPY_ALIGNED(s->coeff_reorder, vp8_def_coeff_reorder, sizeof(s->coeff_reorder));

//   vp8_coeff_order_table_init(s);

       pred_range = 840;
	mbt_range = 102;
	mvd_range = 3;
	cfm_range = 40;
	
       //fprintf(risc_out, "//---------- mbt%d model initial value ----------\n", i*2+j);
	for (i=0; i<3; i++)
	{
		for (j=0; j<2; j++)
		{
		       //fprintf(risc_out,"RISCWrite(`VP8_ADDR + 4*50, 32'd%d);\n", (pred_range+96+i*2+j));
			u4Val = pred_range+96+i*2+j;
    ///Check!!! 
//			vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_WARR, u4Val, u4BSID);
			Log_def_mb_types_stats_50[i][j] = u4Val;
			
			//fprintf(risc_out,"RISCWrite(`VP8_ADDR + 4*51, 32'h%.2x%.2x%.2x%.2x);\n",
				//vp56_def_mb_types_stats[i][0][j],
				//vp56_def_mb_types_stats[i][1][j],
				//vp56_def_mb_types_stats[i][2][j],
				//vp56_def_mb_types_stats[i][3][j]);
			u4Tmp0 = vp56_def_mb_types_stats[i][0][j];
			u4Tmp1 = vp56_def_mb_types_stats[i][1][j];
			u4Tmp2 = vp56_def_mb_types_stats[i][2][j];
			u4Tmp3 = vp56_def_mb_types_stats[i][3][j];
			u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
    ///Check!!! 
	//		vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_FCVR1, u4Val, u4BSID);
			Log_def_mb_types_stats_51[i][j] = u4Val;
			
			//fprintf(risc_out,"RISCWrite(`VP8_ADDR + 4*52, 32'h%.2x%.2x%.2x%.2x);\n",
				//vp56_def_mb_types_stats[i][4][j],
				//vp56_def_mb_types_stats[i][5][j],
				//vp56_def_mb_types_stats[i][6][j],
				//vp56_def_mb_types_stats[i][7][j]);
			u4Tmp0 = vp56_def_mb_types_stats[i][4][j];
			u4Tmp1 = vp56_def_mb_types_stats[i][5][j];
			u4Tmp2 = vp56_def_mb_types_stats[i][6][j];
			u4Tmp3 = vp56_def_mb_types_stats[i][7][j];
			u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));						
    ///Check!!! 
	//		vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_FCVR2, u4Val, u4BSID);
			Log_def_mb_types_stats_52[i][j] = u4Val;
			
			//fprintf(risc_out,"RISCWrite(`VP8_ADDR + 4*53, 32'h%.2x%.2x0000);\n",
				//vp56_def_mb_types_stats[i][8][j],
				//vp56_def_mb_types_stats[i][9][j]);	
			u4Tmp0 = vp56_def_mb_types_stats[i][8][j];
			u4Tmp1 = vp56_def_mb_types_stats[i][9][j];
			u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16));						
    ///Check!!! 
	//		vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_FCVR3, u4Val, u4BSID);
			Log_def_mb_types_stats_53[i][j] = u4Val;
			
			//fprintf(risc_out,"RISCWrite(`VP8_ADDR + 4*54, 32'h0);\n");
			u4Val = 0;
    ///Check!!! 
//			vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_FCVR4, u4Val, u4BSID);
			Log_def_mb_types_stats_54[i][j] = u4Val;
		}
	}


	//fprintf(risc_out, "//---------- dct-sig model initial value ----------\n");
	//fprintf(risc_out,"RISCWrite(`VP8_ADDR + 4*50, 32'd%d);\n", (pred_range+mbt_range));	
	u4Val = pred_range+mbt_range;
    ///Check!!! 
//	vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_WARR, u4Val, u4BSID);
	Log_vector_model_dct[0] = u4Val;
	
	//fprintf(risc_out,"RISCWrite(`VP8_ADDR + 4*51, 32'h%.2x%.2x%.2x%.2x);\n",
		//s->vector_model_dct[0],
		//s->vector_model_sig[0],
		//s->vector_model_dct[1],
		//s->vector_model_sig[1]);
	u4Tmp0 = vector_model_dct[0];
	u4Tmp1 = vector_model_sig[0];
	u4Tmp2 = vector_model_dct[1];
	u4Tmp3 = vector_model_sig[1];
	u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
    ///Check!!! 
//	vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_FCVR1, u4Val, u4BSID);
	Log_vector_model_dct[1] = u4Val;
	
	//fprintf(risc_out,"RISCWrite(`VP8_ADDR + 4*52, 32'h0);\n");
	u4Val = 0;
    ///Check!!! 
//	vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_FCVR2, u4Val, u4BSID);
	Log_vector_model_dct[2] = u4Val;
	
	//fprintf(risc_out,"RISCWrite(`VP8_ADDR + 4*53, 32'h0);\n");
	u4Val = 0;
    ///Check!!! 
//	vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_FCVR3, u4Val, u4BSID);
	Log_vector_model_dct[3] = u4Val;
	
	//fprintf(risc_out,"RISCWrite(`VP8_ADDR + 4*54, 32'h0);\n");
	u4Val = 0;
    ///Check!!! 
//	vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_FCVR4, u4Val, u4BSID);
	Log_vector_model_dct[4] = u4Val;

	//fprintf(risc_out, "//---------- pdv model initial value ----------\n");
	//fprintf(risc_out,"RISCWrite(`VP8_ADDR + 4*50, 32'd%d);\n", (pred_range+mbt_range+1));	
	u4Val = pred_range+mbt_range+1;
    ///Check!!! 
//	vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_WARR, u4Val, u4BSID);
       Log_def_pdv_vector_model[0] = u4Val;
	
	//fprintf(risc_out,"RISCWrite(`VP8_ADDR + 4*51, 32'h%.2x%.2x%.2x%.2x);\n",
		//vp8_def_pdv_vector_model[0][0],
		//vp8_def_pdv_vector_model[0][1],
		//vp8_def_pdv_vector_model[0][2],
		//vp8_def_pdv_vector_model[0][3]);
	u4Tmp0 = vp8_def_pdv_vector_model[0][0];
	u4Tmp1 = vp8_def_pdv_vector_model[0][1];
	u4Tmp2 = vp8_def_pdv_vector_model[0][2];
	u4Tmp3 = vp8_def_pdv_vector_model[0][3];
	u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
    ///Check!!! 
//	vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_FCVR1, u4Val, u4BSID);
	Log_def_pdv_vector_model[1] = u4Val;
	
	//fprintf(risc_out,"RISCWrite(`VP8_ADDR + 4*52, 32'h%.2x%.2x%.2x%.2x);\n",
		//vp8_def_pdv_vector_model[0][4],
		//vp8_def_pdv_vector_model[0][5],
		//vp8_def_pdv_vector_model[0][6],
		//vp8_def_pdv_vector_model[1][0]);
	u4Tmp0 = vp8_def_pdv_vector_model[0][4];
	u4Tmp1 = vp8_def_pdv_vector_model[0][5];
	u4Tmp2 = vp8_def_pdv_vector_model[0][6];
	u4Tmp3 = vp8_def_pdv_vector_model[1][0];
	u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
    ///Check!!! 
//	vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_FCVR2, u4Val, u4BSID);
	Log_def_pdv_vector_model[2] = u4Val;
	
	//fprintf(risc_out,"RISCWrite(`VP8_ADDR + 4*53, 32'h%.2x%.2x%.2x%.2x);\n",
		//vp8_def_pdv_vector_model[1][1],
		//vp8_def_pdv_vector_model[1][2],
		//vp8_def_pdv_vector_model[1][3],
		//vp8_def_pdv_vector_model[1][4]);
	u4Tmp0 = vp8_def_pdv_vector_model[1][1];
	u4Tmp1 = vp8_def_pdv_vector_model[1][2];
	u4Tmp2 = vp8_def_pdv_vector_model[1][3];
	u4Tmp3 = vp8_def_pdv_vector_model[1][4];
	u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
    ///Check!!! 
//	vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_FCVR3, u4Val, u4BSID);
	Log_def_pdv_vector_model[3] = u4Val;
	
	//fprintf(risc_out,"RISCWrite(`VP8_ADDR + 4*54, 32'h%.2x%.2x0000);\n",
		//vp8_def_pdv_vector_model[1][5],
		//vp8_def_pdv_vector_model[1][6]);
	u4Tmp0 = vp8_def_pdv_vector_model[1][5];
	u4Tmp1 = vp8_def_pdv_vector_model[1][6];
	u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16));
    ///Check!!! 
//	vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_FCVR4, u4Val, u4BSID);
	Log_def_pdv_vector_model[4] = u4Val;

	//fprintf(risc_out, "//---------- fdv model initial value ----------\n");
	//fprintf(risc_out,"RISCWrite(`VP8_ADDR + 4*50, 32'd%d);\n", (pred_range+mbt_range+2));	
	u4Val = pred_range+mbt_range+2;
    ///Check!!! 
//	vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_WARR, u4Val, u4BSID);
	Log_def_fdv_vector_model[0] = u4Val;
	
	//fprintf(risc_out,"RISCWrite(`VP8_ADDR + 4*51, 32'h%.2x%.2x%.2x%.2x);\n",
		//vp8_def_fdv_vector_model[0][0],
		//vp8_def_fdv_vector_model[0][1],
		//vp8_def_fdv_vector_model[0][2],
		//vp8_def_fdv_vector_model[0][3]);
	u4Tmp0 = vp8_def_fdv_vector_model[0][0];
	u4Tmp1 = vp8_def_fdv_vector_model[0][1];
	u4Tmp2 = vp8_def_fdv_vector_model[0][2];
	u4Tmp3 = vp8_def_fdv_vector_model[0][3];
	u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
    ///Check!!! 
//	vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_FCVR1, u4Val, u4BSID);
	Log_def_fdv_vector_model[1] = u4Val;
	
	//fprintf(risc_out,"RISCWrite(`VP8_ADDR + 4*52, 32'h%.2x%.2x%.2x%.2x);\n",
		//vp8_def_fdv_vector_model[0][4],
		//vp8_def_fdv_vector_model[0][5],
		//vp8_def_fdv_vector_model[0][6],
		//vp8_def_fdv_vector_model[0][7]);
	u4Tmp0 = vp8_def_fdv_vector_model[0][4];
	u4Tmp1 = vp8_def_fdv_vector_model[0][5];
	u4Tmp2 = vp8_def_fdv_vector_model[0][6];
	u4Tmp3 = vp8_def_fdv_vector_model[0][7];
	u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
    ///Check!!! 
//	vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_FCVR2, u4Val, u4BSID);
	Log_def_fdv_vector_model[2] = u4Val;
	
	//fprintf(risc_out,"RISCWrite(`VP8_ADDR + 4*53, 32'h%.2x%.2x%.2x%.2x);\n",
		//vp8_def_fdv_vector_model[1][0],
		//vp8_def_fdv_vector_model[1][1],
		//vp8_def_fdv_vector_model[1][2],
		//vp8_def_fdv_vector_model[1][3]);
	u4Tmp0 = vp8_def_fdv_vector_model[1][0];
	u4Tmp1 = vp8_def_fdv_vector_model[1][1];
	u4Tmp2 = vp8_def_fdv_vector_model[1][2];
	u4Tmp3 = vp8_def_fdv_vector_model[1][3];
	u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
    ///Check!!! 
//	vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_FCVR3, u4Val, u4BSID);
	Log_def_fdv_vector_model[3] = u4Val;
	
	//fprintf(risc_out,"RISCWrite(`VP8_ADDR + 4*54, 32'h%.2x%.2x%.2x%.2x);\n",
		//vp8_def_fdv_vector_model[1][4],
		//vp8_def_fdv_vector_model[1][5],
		//vp8_def_fdv_vector_model[1][6],
		//vp8_def_fdv_vector_model[1][7]);
	u4Tmp0 = vp8_def_fdv_vector_model[1][4];
	u4Tmp1 = vp8_def_fdv_vector_model[1][5];
	u4Tmp2 = vp8_def_fdv_vector_model[1][6];
	u4Tmp3 = vp8_def_fdv_vector_model[1][7];
	u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
    ///Check!!! 
//	vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_FCVR4, u4Val, u4BSID);
	Log_def_fdv_vector_model[4] = u4Val;

	//fprintf(risc_out, "//---------- runv0 model initial value ----------\n");
	//fprintf(risc_out,"RISCWrite(`VP8_ADDR + 4*50, 32'd%d);\n", (pred_range+mbt_range+mvd_range+2));	
	u4Val = pred_range+mbt_range+mvd_range+2;
    ///Check!!! 
//	vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_WARR, u4Val, u4BSID);
	Log_def_runv0_coeff_model[0] = u4Val;
	
	//fprintf(risc_out,"RISCWrite(`VP8_ADDR + 4*51, 32'h%.2x%.2x%.2x%.2x);\n",
		//vp8_def_runv_coeff_model[0][0],
		//vp8_def_runv_coeff_model[0][1],
		//vp8_def_runv_coeff_model[0][2],
		//vp8_def_runv_coeff_model[0][3]);
	u4Tmp0 = vp8_def_runv_coeff_model[0][0];
	u4Tmp1 = vp8_def_runv_coeff_model[0][1];
	u4Tmp2 = vp8_def_runv_coeff_model[0][2];
	u4Tmp3 = vp8_def_runv_coeff_model[0][3];
	u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
    ///Check!!! 
//	vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_FCVR1, u4Val, u4BSID);
	Log_def_runv0_coeff_model[1] = u4Val;
	
	//fprintf(risc_out,"RISCWrite(`VP8_ADDR + 4*52, 32'h%.2x%.2x%.2x%.2x);\n",
		//vp8_def_runv_coeff_model[0][4],
		//vp8_def_runv_coeff_model[0][5],
		//vp8_def_runv_coeff_model[0][6],
		//vp8_def_runv_coeff_model[0][7]);
	u4Tmp0 = vp8_def_runv_coeff_model[0][4];
	u4Tmp1 = vp8_def_runv_coeff_model[0][5];
	u4Tmp2 = vp8_def_runv_coeff_model[0][6];
	u4Tmp3 = vp8_def_runv_coeff_model[0][7];
	u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
    ///Check!!! 
//	vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_FCVR2, u4Val, u4BSID);
	Log_def_runv0_coeff_model[2] = u4Val;
	
	//fprintf(risc_out,"RISCWrite(`VP8_ADDR + 4*53, 32'h%.2x%.2x%.2x%.2x);\n",
		//vp8_def_runv_coeff_model[0][8],
		//vp8_def_runv_coeff_model[0][9],
		//vp8_def_runv_coeff_model[0][10],
		//vp8_def_runv_coeff_model[0][11]);
	u4Tmp0 = vp8_def_runv_coeff_model[0][8];
	u4Tmp1 = vp8_def_runv_coeff_model[0][9];
	u4Tmp2 = vp8_def_runv_coeff_model[0][10];
	u4Tmp3 = vp8_def_runv_coeff_model[0][11];
	u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
    ///Check!!! 
//	vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_FCVR3, u4Val, u4BSID);
	Log_def_runv0_coeff_model[3] = u4Val;
	
	//fprintf(risc_out,"RISCWrite(`VP8_ADDR + 4*54, 32'h%.2x%.2x0000);\n",
		//vp8_def_runv_coeff_model[0][12],
		//vp8_def_runv_coeff_model[0][13]);
	u4Tmp0 = vp8_def_runv_coeff_model[0][12];
	u4Tmp1 = vp8_def_runv_coeff_model[0][13];
	u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16));
    ///Check!!! 
//	vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_FCVR4, u4Val, u4BSID);
	Log_def_runv0_coeff_model[4] = u4Val;

	//fprintf(risc_out, "//---------- runv1 model initial value ----------\n");
	//fprintf(risc_out,"RISCWrite(`VP8_ADDR + 4*50, 32'd%d);\n",(pred_range+mbt_range+mvd_range+3));
	u4Val = pred_range+mbt_range+mvd_range+3;
    ///Check!!! 
//	vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_WARR, u4Val, u4BSID);
	Log_def_runv1_coeff_model[0] = u4Val;	
	
	//fprintf(risc_out,"RISCWrite(`VP8_ADDR + 4*51, 32'h%.2x%.2x%.2x%.2x);\n",
		//vp8_def_runv_coeff_model[1][0],
		//vp8_def_runv_coeff_model[1][1],
		//vp8_def_runv_coeff_model[1][2],
		//vp8_def_runv_coeff_model[1][3]);
	u4Tmp0 = vp8_def_runv_coeff_model[1][0];
	u4Tmp1 = vp8_def_runv_coeff_model[1][1];
	u4Tmp2 = vp8_def_runv_coeff_model[1][2];
	u4Tmp3 = vp8_def_runv_coeff_model[1][3];
	u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
    ///Check!!! 
//	vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_FCVR1, u4Val, u4BSID);
	Log_def_runv1_coeff_model[1] = u4Val;	
	
	//fprintf(risc_out,"RISCWrite(`VP8_ADDR + 4*52, 32'h%.2x%.2x%.2x%.2x);\n",
		//vp8_def_runv_coeff_model[1][4],
		//vp8_def_runv_coeff_model[1][5],
		//vp8_def_runv_coeff_model[1][6],
		//vp8_def_runv_coeff_model[1][7]);
	u4Tmp0 = vp8_def_runv_coeff_model[1][4];
	u4Tmp1 = vp8_def_runv_coeff_model[1][5];
	u4Tmp2 = vp8_def_runv_coeff_model[1][6];
	u4Tmp3 = vp8_def_runv_coeff_model[1][7];
	u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
    ///Check!!! 
//	vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_FCVR2, u4Val, u4BSID);
	Log_def_runv1_coeff_model[2] = u4Val;	
	
	//fprintf(risc_out,"RISCWrite(`VP8_ADDR + 4*53, 32'h%.2x%.2x%.2x%.2x);\n",
		//vp8_def_runv_coeff_model[1][8],
		//vp8_def_runv_coeff_model[1][9],
		//vp8_def_runv_coeff_model[1][10],
		//vp8_def_runv_coeff_model[1][11]);
	u4Tmp0 = vp8_def_runv_coeff_model[1][8];
	u4Tmp1 = vp8_def_runv_coeff_model[1][9];
	u4Tmp2 = vp8_def_runv_coeff_model[1][10];
	u4Tmp3 = vp8_def_runv_coeff_model[1][11];
	u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
    ///Check!!! 
//	vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_FCVR3, u4Val, u4BSID);
	Log_def_runv1_coeff_model[3] = u4Val;	
	
	//fprintf(risc_out,"RISCWrite(`VP8_ADDR + 4*54, 32'h%.2x%.2x0000);\n",
		//vp8_def_runv_coeff_model[1][12],
		//vp8_def_runv_coeff_model[1][13]);
	u4Tmp0 = vp8_def_runv_coeff_model[1][12];
	u4Tmp1 = vp8_def_runv_coeff_model[1][13];
	u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16));
    ///Check!!! 
//	vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_FCVR4, u4Val, u4BSID);
	Log_def_runv1_coeff_model[4] = u4Val;	

	//fprintf(risc_out, "//---------- model ini finial ----------\n");
	////fprintf(risc_ini_out,"RISCWrite(`VP8_ADDR + 4*55, 32'h1);\n");
       u4Val = 1;
    ///Check!!! 
//	vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_WWFR, u4Val, u4BSID);

	return HAL_HANDLE_OK;
}


// **************************************************************************
// Function : UINT32 u4VDEC_HAL_VP8_Parse_Mb_Type_Models(UINT32 u4BSID, UINT32 u4VDecID)
// Description :Read current read pointer
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
//                 u4VFIFOSa : video FIFO start address
//                 pu4Bits : read pointer value with remained bits
// Return      : Read pointer value with byte alignment
// **************************************************************************
UINT32 u4VDEC_HAL_VP8_Parse_Mb_Type_Models(UINT32 u4BSID, UINT32 u4VDecID)
{    
    UCHAR    vp56_pre_def_mb_type_stats[16][3][10][2] = 
    {
       { 
      	   { {   9, 15 }, {  32, 25 }, {  7,  19 }, {   9, 21 }, {  1, 12 },
             {  14, 12 }, {   3, 18 }, { 14,  23 }, {   3, 10 }, {  0,  4 }, },
          { {  41, 22 }, {   1,  0 }, {  1,  31 }, {   0,  0 }, {  0,  0 },
             {   0,  1 }, {   1,  7 }, {  0,   1 }, {  98, 25 }, {  4, 10 }, },
          { {   2,  3 }, {   2,  3 }, {  0,   2 }, {   0,  2 }, {  0,  0 },
             {  11,  4 }, {   1,  4 }, {  0,   2 }, {   3,  2 }, {  0,  4 }, }, 
       },
       { 
           { {  48, 39 }, {   1,  2 }, { 11,  27 }, {  29, 44 }, {  7, 27 },
              {   1,  4 }, {   0,  3 }, {  1,   6 }, {   1,  2 }, {  0,  0 }, },
           { { 123, 37 }, {   6,  4 }, {  1,  27 }, {   0,  0 }, {  0,  0 },
              {   5,  8 }, {   1,  7 }, {  0,   1 }, {  12, 10 }, {  0,  2 }, },
           { {  49, 46 }, {   3,  4 }, {  7,  31 }, {  42, 41 }, {  0,  0 },
              {   2,  6 }, {   1,  7 }, {  1,   4 }, {   2,  4 }, {  0,  1 }, }, 
       },
       {
           { {  21, 32 }, {   1,  2 }, {  4,  10 }, {  32, 43 }, {  6, 23 },
              {   2,  3 }, {   1, 19 }, {  1,   6 }, {  12, 21 }, {  0,  7 }, },
           { {  26, 14 }, {  14, 12 }, {  0,  24 }, {   0,  0 }, {  0,  0 },
              {  55, 17 }, {   1,  9 }, {  0,  36 }, {   5,  7 }, {  1,  3 }, },
           { {  26, 25 }, {   1,  1 }, {  2,  10 }, {  67, 39 }, {  0,  0 },
              {   1,  1 }, {   0, 14 }, {  0,   2 }, {  31, 26 }, {  1,  6 }, }, 
       },
       {
           { {  69, 83 }, {   0,  0 }, {  0,   2 }, {  10, 29 }, {  3, 12 },
              {   0,  1 }, {   0,  3 }, {  0,   3 }, {   2,  2 }, {  0,  0 }, },
           { { 209,  5 }, {   0,  0 }, {  0,  27 }, {   0,  0 }, {  0,  0 },
              {   0,  1 }, {   0,  1 }, {  0,   1 }, {   0,  0 }, {  0,  0 }, },
           { { 103, 46 }, {   1,  2 }, {  2,  10 }, {  33, 42 }, {  0,  0 },
             {   1,  4 }, {   0,  3 }, {  0,   1 }, {   1,  3 }, {  0,  0 }, }, 
       },
       {
           { {  11, 20 }, {   1,  4 }, { 18,  36 }, {  43, 48 }, { 13, 35 },
              {   0,  2 }, {   0,  5 }, {  3,  12 }, {   1,  2 }, {  0,  0 }, },
           { {   2,  5 }, {   4,  5 }, {  0, 121 }, {   0,  0 }, {  0,  0 },
              {   0,  3 }, {   2,  4 }, {  1,   4 }, {   2,  2 }, {  0,  1 }, },
           { {  14, 31 }, {   9, 13 }, { 14,  54 }, {  22, 29 }, {  0,  0 },
              {   2,  6 }, {   4, 18 }, {  6,  13 }, {   1,  5 }, {  0,  1 }, }, 
       },
       { 
           { {  70, 44 }, {   0,  1 }, {  2,  10 }, {  37, 46 }, {  8, 26 },
              {   0,  2 }, {   0,  2 }, {  0,   2 }, {   0,  1 }, {  0,  0 }, },
           { { 175,  5 }, {   0,  1 }, {  0,  48 }, {   0,  0 }, {  0,  0 },
              {   0,  2 }, {   0,  1 }, {  0,   2 }, {   0,  1 }, {  0,  0 }, },
          { {  85, 39 }, {   0,  0 }, {  1,   9 }, {  69, 40 }, {  0,  0 },
             {   0,  1 }, {   0,  3 }, {  0,   1 }, {   2,  3 }, {  0,  0 }, }, 
       },
       { 
           { {   8, 15 }, {   0,  1 }, {  8,  21 }, {  74, 53 }, { 22, 42 },
              {   0,  1 }, {   0,  2 }, {  0,   3 }, {   1,  2 }, {  0,  0 }, },
           { {  83,  5 }, {   2,  3 }, {  0, 102 }, {   0,  0 }, {  0,  0 },
              {   1,  3 }, {   0,  2 }, {  0,   1 }, {   0,  0 }, {  0,  0 }, },
           { {  31, 28 }, {   0,  0 }, {  3,  14 }, { 130, 34 }, {  0,  0 },
              {   0,  1 }, {   0,  3 }, {  0,   1 }, {   3,  3 }, {  0,  1 }, }, 
       },
       { 
           { { 141, 42 }, {   0,  0 }, {  1,   4 }, {  11, 24 }, {  1, 11 },
              {   0,  1 }, {   0,  1 }, {  0,   2 }, {   0,  0 }, {  0,  0 }, },
           { { 233,  6 }, {   0,  0 }, {  0,   8 }, {   0,  0 }, {  0,  0 },
              {   0,  1 }, {   0,  1 }, {  0,   0 }, {   0,  1 }, {  0,  0 }, },
           { { 171, 25 }, {   0,  0 }, {  1,   5 }, {  25, 21 }, {  0,  0 },
              {   0,  1 }, {   0,  1 }, {  0,   0 }, {   0,  0 }, {  0,  0 }, },
        },
      {
           { {   8, 19 }, {   4, 10 }, { 24,  45 }, {  21, 37 }, {  9, 29 },
              {   0,  3 }, {   1,  7 }, { 11,  25 }, {   0,  2 }, {  0,  1 }, },
           { {  34, 16 }, { 112, 21 }, {  1,  28 }, {   0,  0 }, {  0,  0 },
              {   6,  8 }, {   1,  7 }, {  0,   3 }, {   2,  5 }, {  0,  2 }, },
           { {  17, 21 }, {  68, 29 }, {  6,  15 }, {  13, 22 }, {  0,  0 },
              {   6, 12 }, {   3, 14 }, {  4,  10 }, {   1,  7 }, {  0,  3 }, }, 
       },
       {
           { {  46, 42 }, {   0,  1 }, {  2,  10 }, {  54, 51 }, { 10, 30 },
              {   0,  2 }, {   0,  2 }, {  0,   1 }, {   0,  1 }, {  0,  0 }, },
           { { 159, 35 }, {   2,  2 }, {  0,  25 }, {   0,  0 }, {  0,  0 },
              {   3,  6 }, {   0,  5 }, {  0,   1 }, {   4,  4 }, {  0,  1 }, },
           { {  51, 39 }, {   0,  1 }, {  2,  12 }, {  91, 44 }, {  0,  0 },
              {   0,  2 }, {   0,  3 }, {  0,   1 }, {   2,  3 }, {  0,  1 }, }, 
       },
       {   
            { {  28, 32 }, {   0,  0 }, {  3,  10 }, {  75, 51 }, { 14, 33 },
               {   0,  1 }, {   0,  2 }, {  0,   1 }, {   1,  2 }, {  0,  0 }, },
            { {  75, 39 }, {   5,  7 }, {  2,  48 }, {   0,  0 }, {  0,  0 },
               {   3, 11 }, {   2, 16 }, {  1,   4 }, {   7, 10 }, {  0,  2 }, },
            { {  81, 25 }, {   0,  0 }, {  2,   9 }, { 106, 26 }, {  0,  0 },
               {   0,  1 }, {   0,  1 }, {  0,   1 }, {   1,  1 }, {  0,  0 }, }, 
       },
       {   
           { { 100, 46 }, {   0,  1 }, {  3,   9 }, {  21, 37 }, {  5, 20 },
              {   0,  1 }, {   0,  2 }, {  1,   2 }, {   0,  1 }, {  0,  0 }, },
           { { 212, 21 }, {   0,  1 }, {  0,   9 }, {   0,  0 }, {  0,  0 },
              {   1,  2 }, {   0,  2 }, {  0,   0 }, {   2,  2 }, {  0,  0 }, },
           { { 140, 37 }, {   0,  1 }, {  1,   8 }, {  24, 33 }, {  0,  0 },
              {   1,  2 }, {   0,  2 }, {  0,   1 }, {   1,  2 }, {  0,  0 }, },
       },
       { 
           { {  27, 29 }, {   0,  1 }, {  9,  25 }, {  53, 51 }, { 12, 34 },
              {   0,  1 }, {   0,  3 }, {  1,   5 }, {   0,  2 }, {  0,  0 }, },
           { {   4,  2 }, {   0,  0 }, {  0, 172 }, {   0,  0 }, {  0,  0 },
              {   0,  1 }, {   0,  2 }, {  0,   0 }, {   2,  0 }, {  0,  0 }, },
           { {  14, 23 }, {   1,  3 }, { 11,  53 }, {  90, 31 }, {  0,  0 },
              {   0,  3 }, {   1,  5 }, {  2,   6 }, {   1,  2 }, {  0,  0 }, }, 
       },
       { 
           { {  80, 38 }, {   0,  0 }, {  1,   4 }, {  69, 33 }, {  5, 16 },
              {   0,  1 }, {   0,  1 }, {  0,   0 }, {   0,  1 }, {  0,  0 }, },
           { { 187, 22 }, {   1,  1 }, {  0,  17 }, {   0,  0 }, {  0,  0 },
              {   3,  6 }, {   0,  4 }, {  0,   1 }, {   4,  4 }, {  0,  1 }, },
           { { 123, 29 }, {   0,  0 }, {  1,   7 }, {  57, 30 }, {  0,  0 },
              {   0,  1 }, {   0,  1 }, {  0,   1 }, {   0,  1 }, {  0,  0 }, }, 
       },
       { 
           { {  16, 20 }, {   0,  0 }, {  2,   8 }, { 104, 49 }, { 15, 33 },
              {   0,  1 }, {   0,  1 }, {  0,   1 }, {   1,  1 }, {  0,  0 }, },
           { { 133,  6 }, {   1,  2 }, {  1,  70 }, {   0,  0 }, {  0,  0 },
              {   0,  2 }, {   0,  4 }, {  0,   3 }, {   1,  1 }, {  0,  0 }, },
           { {  13, 14 }, {   0,  0 }, {  4,  20 }, { 175, 20 }, {  0,  0 },
              {   0,  1 }, {   0,  1 }, {  0,   1 }, {   1,  1 }, {  0,  0 }, }, 
       },
       {
           { { 194, 16 }, {   0,  0 }, {  1,   1 }, {   1,  9 }, {  1,  3 },
              {   0,  0 }, {   0,  1 }, {  0,   1 }, {   0,  0 }, {  0,  0 }, },
           { { 251,  1 }, {   0,  0 }, {  0,   2 }, {   0,  0 }, {  0,  0 },
              {   0,  0 }, {   0,  0 }, {  0,   0 }, {   0,  0 }, {  0,  0 }, },
           { { 202, 23 }, {   0,  0 }, {  1,   3 }, {   2,  9 }, {  0,  0 },
              {   0,  1 }, {   0,  1 }, {  0,   1 }, {   0,  0 }, {  0,  0 }, }, 
       },
    };
    UINT32    pred_range = 840;
    //UINT32    mbt_range = 102;
    //UINT32    mvd_range = 3;
    //UINT32    cfm_range = 40;
    UINT32    idx, ctx, i;
    UINT32    u4Val, u4Tmp0, u4Tmp1, u4Tmp2, u4Tmp3;
    UINT32    u4Log50 [16][3][2] = 
    {
    	{{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
    	{{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
    	{{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
    	{{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
    };
    UINT32    u4Log51 [16][3][2] =
    {
    	{{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
    	{{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
    	{{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
    	{{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
    };
    UINT32    u4Log52 [16][3][2] =
    {
    	{{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
    	{{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
    	{{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
    	{{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
    };
    UINT32    u4Log53 [16][3][2] =
    {
    	{{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
    	{{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
    	{{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
    	{{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
    };
    UINT32    u4Log54 [16][3][2] =
    {
    	{{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
    	{{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
    	{{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
    	{{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
    };
    
    for (idx=0; idx<16; idx++) 
        for (ctx=0; ctx<3; ctx++) 
            for(i=0; i<2; i++)
            {
		   //fprintf(risc_out, "//---------- idx = %d ----------\n", idx);
		   //fprintf(risc_out,"RISCWrite(`VP8_ADDR + 4*50, 32'h%x);\n", (idx*3*2 +ctx*2 +i+pred_range));
		   u4Val = (idx*3*2 +ctx*2 +i+pred_range);
    ///Check!!! 
//		   vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_WARR, u4Val, u4BSID);
		   u4Log50[idx][ctx][i] = u4Val;
			
  		    //fprintf(risc_out,"RISCWrite(`VP8_ADDR + 4*51, 32'h%.2x%.2x%.2x%.2x);\n",
                               //vp56_pre_def_mb_type_stats[idx][ctx][0][i],
                               //vp56_pre_def_mb_type_stats[idx][ctx][1][i],
                               //vp56_pre_def_mb_type_stats[idx][ctx][2][i],
                               //vp56_pre_def_mb_type_stats[idx][ctx][3][i]);
		    u4Tmp0 = vp56_pre_def_mb_type_stats[idx][ctx][0][i],
		    u4Tmp1 = vp56_pre_def_mb_type_stats[idx][ctx][1][i],
		    u4Tmp2 = vp56_pre_def_mb_type_stats[idx][ctx][2][i],
		    u4Tmp3 = vp56_pre_def_mb_type_stats[idx][ctx][3][i],
		    u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
    ///Check!!! 
//		    vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_FCVR1, u4Val, u4BSID);
 	           u4Log51[idx][ctx][i] = u4Val;
		    
                  //fprintf(risc_out,"RISCWrite(`VP8_ADDR + 4*52, 32'h%.2x%.2x%.2x%.2x);\n",
                               //vp56_pre_def_mb_type_stats[idx][ctx][4][i],
                               //vp56_pre_def_mb_type_stats[idx][ctx][5][i],
                               //vp56_pre_def_mb_type_stats[idx][ctx][6][i],
                               //vp56_pre_def_mb_type_stats[idx][ctx][7][i]);
                  u4Tmp0 = vp56_pre_def_mb_type_stats[idx][ctx][4][i],
		    u4Tmp1 = vp56_pre_def_mb_type_stats[idx][ctx][5][i],
		    u4Tmp2 = vp56_pre_def_mb_type_stats[idx][ctx][6][i],
		    u4Tmp3 = vp56_pre_def_mb_type_stats[idx][ctx][7][i],
		    u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
    ///Check!!! 
//		    vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_FCVR2, u4Val, u4BSID);
		    u4Log52[idx][ctx][i] = u4Val;
		    
                  //fprintf(risc_out,"RISCWrite(`VP8_ADDR + 4*53, 32'h%.2x%.2x0000);\n",
                               //vp56_pre_def_mb_type_stats[idx][ctx][8][i],
                               //vp56_pre_def_mb_type_stats[idx][ctx][9][i]);
                  u4Tmp0 = vp56_pre_def_mb_type_stats[idx][ctx][8][i],
		    u4Tmp1 = vp56_pre_def_mb_type_stats[idx][ctx][9][i],
		    u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16));
    ///Check!!! 
//		    vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_FCVR3, u4Val, u4BSID);
		    u4Log53[idx][ctx][i] = u4Val;
		    
                  //fprintf(risc_out,"RISCWrite(`VP8_ADDR + 4*54, 32'h0);\n");
                  u4Val = 0;
    ///Check!!! 
//		    vVDecWriteVP8VLD(u4VDecID, RW_VP8_VLD_FCVR4, u4Val, u4BSID);
		    u4Log54[idx][ctx][i] = u4Val;
          }
            
   return 0;
}

// **************************************************************************
// Function : UINT32 u4VDEC_HAL_VP8_Load_QMatrix(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4VFIFOSa, UINT32 *pu4Bits);
// Description :Read current read pointer
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
//                 u4VFIFOSa : video FIFO start address
//                 pu4Bits : read pointer value with remained bits
// Return      : Read pointer value with byte alignment
// **************************************************************************
UINT32 u4VDEC_HAL_VP8_Load_QMatrix(UINT32 u4BSID, UINT32 u4VDecID)
{
   UINT32 i;
   UINT32 u4Tmp0, u4Tmp1, u4Tmp2, u4Tmp3;
   UINT32 u4Val;
   UCHAR vp8_def_coeff_reorder[] = 
   {
   	0,  0,  1,  1,  1,  2,  2,  2,
   	2,  2,  2,  3,  3,  4,  4,  4,
   	5,  5,  5,  5,  6,  6,  7,  7,
   	7,  7,  7,  8,  8,  9,  9,  9,
   	9,  9,  9, 10, 10, 11, 11, 11,
   	11, 11, 11, 12, 12, 12, 12, 12,
   	12, 13, 13, 13, 13, 13, 14, 14,
   	14, 14, 15, 15, 15, 15, 15, 15,
   };
   
   for (i=0; i<64; i+=4) 
   {
      //fprintf(risc_out,"RISCWrite(`VLD_ADDR_1 +4*152, (0<<8) + %d);\n", i);
      vVDecWriteVLD(u4VDecID, RW_VLD_SCL_ADDR, ((0<<8)+i) );
      
      //fprintf(risc_out,"RISCWrite(`VLD_ADDR_1 +4*153, (%d<<24) + (%d<<16) + (%d<<8) + (%d<<0));\n", 
          //s->coeff_reorder[i], 
          //s->coeff_reorder[i+1], 
          //s->coeff_reorder[i+2], 
          //s->coeff_reorder[i+3]);
          
      u4Tmp0 = vp8_def_coeff_reorder[i];
      u4Tmp1 = vp8_def_coeff_reorder[i+1];
      u4Tmp2 = vp8_def_coeff_reorder[i+2];
      u4Tmp3 = vp8_def_coeff_reorder[i+3];      
      u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
      vVDecWriteVLD(u4VDecID, RW_VLD_SCL_DATA, u4Val);
   }
   
   return HAL_HANDLE_OK;
}


UINT32 u4VDEC_HAL_VP8_Read_QMatrix(UINT32 u4BSID, UINT32 u4VDecID)
{
   UINT32 i;
   UINT32 u4Val;
   UCHAR rd_vp8_def_coeff_reorder[64] = 
   {
   	0,  0,  0,  0,  0,  0,  0,  0,
   	0,  0,  0,  0,  0,  0,  0,  0,
   	0,  0,  0,  0,  0,  0,  0,  0,
   	0,  0,  0,  0,  0,  0,  0,  0,
   	0,  0,  0,  0,  0,  0,  0,  0,
   	0,  0,  0,  0,  0,  0,  0,  0,
   	0,  0,  0,  0,  0,  0,  0,  0,
   	0,  0,  0,  0,  0,  0,  0,  0,
   };
   
   for (i=0; i<64; i+=4) 
   {
      vVDecWriteVLD(u4VDecID, RW_VLD_SCL_ADDR, ((1<<8)+i) );
      u4Val = u4VDecReadVLD(u4VDecID, RW_VLD_SCL_DATA);
          
      rd_vp8_def_coeff_reorder[i]     = ((u4Val & 0xFF000000)>> 24);
      rd_vp8_def_coeff_reorder[i+1] = ((u4Val & 0x00FF0000) >> 16);
      rd_vp8_def_coeff_reorder[i+2] = ((u4Val & 0x0000FF00) >> 8);
      rd_vp8_def_coeff_reorder[i+3] = ((u4Val & 0x000000FF) >> 0);
   }
   
   return HAL_HANDLE_OK;
}

// **************************************************************************
// Function : UINT32 u4VDEC_HAL_VP8_Load_Filter_Coef(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4Select);
// Description :Read current read pointer
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
//                 u4VFIFOSa : video FIFO start address
//                 pu4Bits : read pointer value with remained bits
// Return      : Read pointer value with byte alignment
// **************************************************************************
UINT32 u4VDEC_HAL_VP8_Load_Filter_Coef(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4Select)
{
   INT16 vp8_block_copy_filter[16][8][4] = 
   {
    { {   0, 128,   0,   0  },  /* 0 */
      {  -3, 122,   9,   0  },
      {  -4, 109,  24,  -1  },
      {  -5,  91,  45,  -3  },
      {  -4,  68,  68,  -4  },
      {  -3,  45,  91,  -5  },
      {  -1,  24, 109,  -4  },
      {   0,   9, 122,  -3  } },
      
   { {   0, 128,   0,   0  },  /* 1 */
      {  -4, 124,   9,  -1  },
      {  -5, 110,  25,  -2  },
      {  -6,  91,  46,  -3  },
      {  -5,  69,  69,  -5  },
      {  -3,  46,  91,  -6  },
      {  -2,  25, 110,  -5  },
      {  -1,   9, 124,  -4  } },
      
   { {   0, 128,   0,   0  },  /* 2 */
      {  -4, 123,  10,  -1  },
      {  -6, 110,  26,  -2  },
      {  -7,  92,  47,  -4  },
      {  -6,  70,  70,  -6  },
      {  -4,  47,  92,  -7  },
      {  -2,  26, 110,  -6  },
      {  -1,  10, 123,  -4  } },
      
   { {   0, 128,   0,   0  },  /* 3 */
      {  -5, 124,  10,  -1  },
      {  -7, 110,  27,  -2  },
      {  -7,  91,  48,  -4  },
      {  -6,  70,  70,  -6  },
      {  -4,  48,  92,  -8  },
      {  -2,  27, 110,  -7  },
      {  -1,  10, 124,  -5  } },
      
   { {   0, 128,   0,   0  },  /* 4 */
      {  -6, 124,  11,  -1  },
      {  -8, 111,  28,  -3  },
      {  -8,  92,  49,  -5  },
      {  -7,  71,  71,  -7  },
      {  -5,  49,  92,  -8  },
      {  -3,  28, 111,  -8  },
      {  -1,  11, 124,  -6  } },
      
   { {  0,  128,   0,   0  },  /* 5 */
      {  -6, 123,  12,  -1  },
      {  -9, 111,  29,  -3  },
      {  -9,  93,  50,  -6  },
      {  -8,  72,  72,  -8  },
      {  -6,  50,  93,  -9  },
      {  -3,  29, 111,  -9  },
      {  -1,  12, 123,  -6  } },
      
   { {   0, 128,   0,   0  },  /* 6 */
      {  -7, 124,  12,  -1  },
      { -10, 111,  30,  -3  },
      { -10,  93,  51,  -6  },
      {  -9,  73,  73,  -9  },
      {  -6,  51,  93, -10  },
      {  -3,  30, 111, -10  },
      {  -1,  12, 124,  -7  } },
      
   { {   0, 128,   0,   0  },  /* 7 */
      {  -7, 123,  13,  -1  },
      { -11, 112,  31,  -4  },
      { -11,  94,  52,  -7  },
      { -10,  74,  74, -10  },
      {  -7,  52,  94, -11  },
      {  -4,  31, 112, -11  },
      {  -1,  13, 123,  -7  } },
      
   { {   0, 128,   0,  0  },  /* 8 */
      {  -8, 124,  13,  -1  },
      { -12, 112,  32,  -4  },
      { -12,  94,  53,  -7  },
      { -10,  74,  74, -10  },
      {  -7,  53,  94, -12  },
      {  -4,  32, 112, -12  },
      {  -1,  13, 124,  -8  } },
      
   { {   0, 128,   0,   0  },  /* 9 */
      {  -9, 124,  14,  -1  },
      { -13, 112,  33,  -4  },
      { -13,  95,  54,  -8  },
      { -11,  75,  75, -11  },
      {  -8,  54,  95, -13  },
      {  -4,  33, 112, -13  },
      {  -1,  14, 124,  -9  } },
   { {   0, 128,   0,   0  },  /* 10 */
      {  -9, 123,  15,  -1  },
      { -14, 113,  34,  -5  },
      { -14,  95,  55,  -8  },
      { -12,  76,  76, -12  },
      {  -8,  55,  95, -14  },
      {  -5,  34, 112, -13  },
      {  -1,  15, 123,  -9  } },
      
   { {   0, 128,   0,   0  },  /* 11 */
      { -10, 124,  15,  -1  },
      { -14, 113,  34,  -5  },
      { -15,  96,  56,  -9  },
      { -13,  77,  77, -13  },
      {  -9,  56,  96, -15  },
      {  -5,  34, 113, -14  },
      {  -1,  15, 124, -10  } },
      
   { {   0, 128,   0,   0  },  /* 12 */
      { -10, 123,  16,  -1  },
      { -15, 113,  35,  -5  },
      { -16,  98,  56, -10  },
      { -14,  78,  78, -14  },
      { -10,  56,  98, -16  },
      {  -5,  35, 113, -15  },
      {  -1,  16, 123, -10  } },
      
   { {   0, 128,   0,   0  },  /* 13 */
      { -11, 124,  17,  -2  },
      { -16, 113,  36,  -5  },
      { -17,  98,  57, -10  },
      { -14,  78,  78, -14  },
      { -10,  57,  98, -17  },
      {  -5,  36, 113, -16  },
      {  -2,  17, 124, -11  } },
      
   { {   0, 128,   0,   0  },  /* 14 */
      { -12, 125,  17,  -2  },
      { -17, 114,  37,  -6  },
      { -18,  99,  58, -11  },
      { -15,  79,  79, -15  },
      { -11,  58,  99, -18  },
      {  -6,  37, 114, -17  },
      {  -2,  17, 125, -12  } },
      
   { {   0, 128,   0,   0  },  /* 15 */   
     { -12, 124,  18,  -2  },
      { -18, 114,  38,  -6  },
      { -19,  99,  59, -11  },
      { -16,  80,  80, -16  },
      { -11,  59,  99, -19  },
      {  -6,  38, 114, -18  },
      {  -2,  18, 124, -12  } },
  };

   INT32 u4Tmp0, u4Tmp1, u4Tmp2, u4Tmp3;
   INT32 u4Val;//, i ,j;
//     Tap0 : -4, -5, -6, -5, -3, -2, -1
//     ==> 0x86C [28:24] = 4
//     ==> 0x86C [20:16] = 5
//     ==> 0x86C [12:8] = 6
//     ==> 0x86C [4:0] = 5
//     ==> 0x870 [28:24] = 3
//     ==> 0x870 [20:16] = 2
//     ==> 0x870 [12:8] = 1

      u4Tmp0 = (vp8_block_copy_filter[u4Select][1][0] * -1);
      u4Tmp1 = (vp8_block_copy_filter[u4Select][2][0] * -1);
      u4Tmp2 = (vp8_block_copy_filter[u4Select][3][0] * -1);
      u4Tmp3 = (vp8_block_copy_filter[u4Select][4][0] * -1);
      u4Val = ((u4Tmp0 << 24) | (u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
    ///Check!!! 
    //      vVDecWriteMC(u4VDecID, RW_VP6_COEF0_P1, u4Val);

      u4Tmp0 = (vp8_block_copy_filter[u4Select][5][0] * -1);
      u4Tmp1 = (vp8_block_copy_filter[u4Select][6][0] * -1);
      u4Tmp2 = (vp8_block_copy_filter[u4Select][7][0] * -1);
      u4Val = ((u4Tmp0 << 24) | (u4Tmp1 << 16) | (u4Tmp2 << 8));
    ///Check!!! 
    //      vVDecWriteMC(u4VDecID, RW_VP8_COEF0_P2, u4Val);


      u4Tmp0 = (vp8_block_copy_filter[u4Select][1][1]);
      u4Tmp1 = (vp8_block_copy_filter[u4Select][2][1]);
      u4Tmp2 = (vp8_block_copy_filter[u4Select][3][1]);
      u4Tmp3 = (vp8_block_copy_filter[u4Select][4][1]);
      u4Val = ((u4Tmp0 << 24) | (u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
    ///Check!!! 
    //      vVDecWriteMC(u4VDecID, RW_VP8_COEF1_P1, u4Val);
      
      u4Tmp0 = (vp8_block_copy_filter[u4Select][5][1]);
      u4Tmp1 = (vp8_block_copy_filter[u4Select][6][1]);
      u4Tmp2 = (vp8_block_copy_filter[u4Select][7][1]);
      u4Val = ((u4Tmp0 << 24) | (u4Tmp1 << 16) | (u4Tmp2 << 8));
    ///Check!!! 
    //      vVDecWriteMC(u4VDecID, RW_VP8_COEF1_P2, u4Val);

       u4Tmp0 = (vp8_block_copy_filter[u4Select][1][2]);
       u4Tmp1 = (vp8_block_copy_filter[u4Select][2][2]);
       u4Tmp2 = (vp8_block_copy_filter[u4Select][3][2]);
       u4Tmp3 = (vp8_block_copy_filter[u4Select][4][2]);
      u4Val = ((u4Tmp0 << 24) | (u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
    ///Check!!! 
    //      vVDecWriteMC(u4VDecID, RW_VP8_COEF2_P1, u4Val);
      
      u4Tmp0 = (vp8_block_copy_filter[u4Select][5][2]);
      u4Tmp1 = (vp8_block_copy_filter[u4Select][6][2]);
      u4Tmp2 = (vp8_block_copy_filter[u4Select][7][2]);
      u4Val = ((u4Tmp0 << 24) | (u4Tmp1 << 16) | (u4Tmp2 << 8));
    ///Check!!! 
    //      vVDecWriteMC(u4VDecID, RW_VP8_COEF2_P2, u4Val);

      u4Tmp0 = (vp8_block_copy_filter[u4Select][1][3] * -1);
      u4Tmp1 = (vp8_block_copy_filter[u4Select][2][3] * -1);
      u4Tmp2 = (vp8_block_copy_filter[u4Select][3][3] * -1);
      u4Tmp3 = (vp8_block_copy_filter[u4Select][4][3] * -1);
      u4Val = ((u4Tmp0 << 24) | (u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
    ///Check!!! 
    //      vVDecWriteMC(u4VDecID, RW_VP8_COEF3_P1, u4Val);

      u4Tmp0 = (vp8_block_copy_filter[u4Select][5][3] * -1);
      u4Tmp1 = (vp8_block_copy_filter[u4Select][6][3] * -1);
      u4Tmp2 = (vp8_block_copy_filter[u4Select][7][3] * -1);
      u4Val = ((u4Tmp0 << 24) | (u4Tmp1 << 16) | (u4Tmp2 << 8));
    ///Check!!! 
    //      vVDecWriteMC(u4VDecID, RW_VP8_COEF3_P2, u4Val);      
   
   return HAL_HANDLE_OK;
}


UINT32 u4VDEC_HAL_VP8_Write_SRAMData1(UINT32 u4BSID, UINT32 u4VDecID)
{

   //---------- ctx = 0 ----------
vVDecWriteVP8VLD(u4VDecID, 4*50, 0x3a8, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0x4501012c, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x06010001, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0x00000000, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ctx = 0 ----------
vVDecWriteVP8VLD(u4VDecID, 4*50, 0x3a9, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0x2a02072a, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x16030205, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0x01000000, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ctx = 1 ----------
vVDecWriteVP8VLD(u4VDecID, 4*50, 0x3aa, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0xe5010000, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x00010000, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0x01000000, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ctx = 1 ----------
vVDecWriteVP8VLD(u4VDecID, 4*50, 0x3ab, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0x08010800, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x00020100, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0x01000000, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ctx = 2 ----------
vVDecWriteVP8VLD(u4VDecID, 4*50, 0x3ac, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0x7a01012e, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x00010000, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0x01000000, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ctx = 2 ----------
vVDecWriteVP8VLD(u4VDecID, 4*50, 0x3ad, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0x23010622, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x00020101, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0x01000000, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- dct-sig model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  942, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0xa280a480, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x0, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0x0, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- pdv model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  943, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0xe192ac93, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0xd6279ccc, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xaa77eb8c, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0xe6e40000, u4BSID);
//---------- fdv model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  944, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0xf7d28744, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x8adceff6, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xf4b8c92c, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0xadddeffd, u4BSID);
//---------- dccv0 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  945, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0xfe011c4a, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x5e806280, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xa680be00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- runv0 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  947, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0xe8ded892, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0xfaf6a98e, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0x82889595, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0xbff90000, u4BSID);
//---------- dccv1 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  946, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0xfe0150aa, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x80809e80, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xa680be00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- runv1 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  948, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0xc8c9b59a, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x9892847e, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0x92a9b8f0, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0xf6fe0000, u4BSID);
//---------- ract0 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  949, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0xfefe50aa, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x80809e80, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xa680be00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ract1 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  950, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0x01b650aa, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x80809e80, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xa680be00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ract2 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  951, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0x01b6508c, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x80809e80, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xea80be00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ract3 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  952, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0x01b638ae, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x80b09e80, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xea80be00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ract4 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  953, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0x01b638ae, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x80b0d080, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ract5 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  954, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0x01b65ac6, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x50e0fec0, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ract6 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  955, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0xfefe5ac6, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x50e0fec0, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ract7 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  956, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0x01ee5a72, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x50e09636, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ract8 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  957, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0x01ee5a72, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x50e096ca, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ract9 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  958, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0x01ee2472, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x50e0f060, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ract10 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  959, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0x01ee24da, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x50e0f060, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ract11 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  960, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0x01ee24da, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x50e0f060, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ract12 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  961, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0x98bc66da, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x50e05c60, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ract13 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  962, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0xacd266da, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x50e05c60, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ract14 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  963, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0xac866674, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x50e05c60, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ract15 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  964, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0x7a3e66b0, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x50945c60, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ract16 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  965, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0x7a3e66b0, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0xa294e860, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ract17 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  966, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0xae6aaef8, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0xa294e860, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ract18 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  967, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0xf0e6aef8, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0xa294e860, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ract19 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  968, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0xa8e6ae48, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0xa294e860, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ract20 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  969, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0xa87aae48, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0xa294e860, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ract21 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  970, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0xa87aae48, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0xa294e860, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ract22 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  971, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0xa87aae48, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0xa294e860, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ract23 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  972, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0xa87aae48, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0xa294e860, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ract24 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  973, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0x4e7a326e, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x6a946e60, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xea96be00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ract25 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  974, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0x4e7a324c, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x6a946e60, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xb696be00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ract26 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  975, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0x3c1a1c4c, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x6a946e60, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xde96fe00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ract27 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  976, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0x3c0a1c66, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x6a94967c, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xfa96fe00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ract28 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  977, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0x521e1c82, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x6ac2c07c, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xfae4fe00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ract29 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  978, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0x8e361ca4, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x40eaee7c, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xfae4fe00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ract30 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  979, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0xd6ae5aa4, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x40eaee7c, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xfae4fe00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ract31 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  980, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0x704c1c54, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x40ea9e7c, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xfae4fe00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ract32 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  981, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0x704c1c54, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x40ea9e7c, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xfae4fe00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ract33 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  982, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0x70081c94, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x40ea9e7c, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xfae4fe00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ract34 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  983, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0x9c281c94, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x40ea9e7c, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xfae4fe00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
//---------- ract35 model initial value ----------
vVDecWriteVP8VLD(u4VDecID, 4*50,  984, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*51, 0x9c281c94, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*52, 0x40ea9e7c, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*53, 0xfae4fe00, u4BSID);
vVDecWriteVP8VLD(u4VDecID, 4*54, 0x0, u4BSID);
   return 0;
}

UINT32 u4VDEC_HAL_VP8_Read_SRAMData1(UINT32 u4BSID, UINT32 u4VDecID)
{
   //read start from 3A8~3D8
   UINT32 u4Start = 936;
   UINT32 i = u4Start;
   UINT32 j = 0;
   UINT32 u4Reg76 [50] = {0};
   UINT32 u4Reg77 [50] = {0};
   UINT32 u4Reg78 [50] = {0};
   UINT32 u4Reg79 [50] = {0};
   
   for ( i = 936; i <= 984; i++)
   {
       //RISCWrite(`VP8_ADDR + 4*75, 32'h3a8);
       //RISCRead(`VP8_ADDR + 4*76);
       //RISCRead(`VP8_ADDR + 4*77);
       //RISCRead(`VP8_ADDR + 4*78);
       //RISCRead(`VP8_ADDR + 4*79);
       vVDecWriteVP8VLD(u4VDecID, 0x12C, i, u4BSID);
       
     	u4Reg76[j] = u4VDecReadVP8VLD(u4VDecID, 0x130);
       u4Reg77[j] = u4VDecReadVP8VLD(u4VDecID, 0x134);
       u4Reg78[j] = u4VDecReadVP8VLD(u4VDecID, 0x138);
       u4Reg79[j] = u4VDecReadVP8VLD(u4VDecID, 0x13C);       	
       j++;
   }
   return 0;
}

#if CONFIG_DRV_VERIFY_SUPPORT
void vVDEC_HAL_VP8_VDec_ReadCheckSum(UINT32 u4VDecID, UINT32 *pu4DecCheckSum)
{
  UINT32  u4Temp;
  
  u4Temp = 0;
  // DCAC
  *pu4DecCheckSum = u4VDecReadVLD(u4VDecID, 0x3AC);
  pu4DecCheckSum ++;
  u4Temp ++;
  *pu4DecCheckSum = u4VDecReadVLD(u4VDecID, 0x3B0);    
  pu4DecCheckSum ++;
  u4Temp ++;
  *pu4DecCheckSum = u4VDecReadVLD(u4VDecID, 0x3B4);    
  pu4DecCheckSum ++;
  u4Temp ++;
  *pu4DecCheckSum = u4VDecReadVLD(u4VDecID, 0x3B8);        
  pu4DecCheckSum ++;
  u4Temp ++;
  // VLD
  *pu4DecCheckSum = u4VDecReadVLD(u4VDecID, 0x2F4);        
  pu4DecCheckSum ++;
  u4Temp ++;
  // MC
  *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x5E8);
  pu4DecCheckSum ++;
  u4Temp ++;
  *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x5EC);    
  pu4DecCheckSum ++;
  u4Temp ++;
  *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x5F0);    
  pu4DecCheckSum ++;
  u4Temp ++;
  *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x5F4);        
  pu4DecCheckSum ++;
  u4Temp ++;
  *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x5F8);
  pu4DecCheckSum ++;
  u4Temp ++;
  *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x5FC);    
  pu4DecCheckSum ++;
  u4Temp ++;
  *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x600);    
  pu4DecCheckSum ++;
  u4Temp ++;
  *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x604);        
  pu4DecCheckSum ++;
  u4Temp ++;
  // PP
  *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x608);    
  pu4DecCheckSum ++;
  u4Temp ++;
  *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x60C);        
  pu4DecCheckSum ++;
  u4Temp ++;       
  *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x610);        
  pu4DecCheckSum ++;
  u4Temp ++; 
  *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x614);        
  pu4DecCheckSum ++;
  u4Temp ++; 
  *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x618);        
  pu4DecCheckSum ++;
  u4Temp ++; 
  *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x61C);        
  pu4DecCheckSum ++;
  u4Temp ++; 
  *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x620);        
  pu4DecCheckSum ++;
  u4Temp ++; 
  *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x624);        
  pu4DecCheckSum ++;
  u4Temp ++; 
  *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x628);        
  pu4DecCheckSum ++;
  u4Temp ++; 
  *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x62C);        
  pu4DecCheckSum ++;
  u4Temp ++; 
  *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x630);        
  pu4DecCheckSum ++;
  u4Temp ++; 

  while(u4Temp < MAX_CHKSUM_NUM)
  {
    *pu4DecCheckSum = 0;            
    pu4DecCheckSum ++;   
    u4Temp ++;
  }  
}

BOOL fgVDEC_HAL_VP8_VDec_CompCheckSum(UINT32 *pu4DecCheckSum, UINT32 *pu4GoldenCheckSum)
{
  UINT32 i;
  // DCAC
  for (i = 0; i < 4; i ++)
  {
    if((*pu4GoldenCheckSum) != (*pu4DecCheckSum))
    {
      vVDecOutputDebugString("\n!!!!!!!!! DCAC Check Sum Compare Error  !!!!!!\n");
      return (FALSE);
    }
    pu4GoldenCheckSum ++;
    pu4DecCheckSum ++;
  }
  // VLD
  if((*pu4GoldenCheckSum) != (*pu4DecCheckSum))
  {
    vVDecOutputDebugString("\n!!!!!!!!! VLD Check Sum Compare Error  !!!!!!\n");
    return (FALSE);
  }
  pu4GoldenCheckSum ++;
  pu4DecCheckSum ++;
  // MC
  for (i = 0; i < 8; i ++)
  {
    if((*pu4GoldenCheckSum) != (*pu4DecCheckSum))
    {
      vVDecOutputDebugString("\n!!!!!!!!! MC Check Sum Compare Error  !!!!!!\n");
      return (FALSE);
    }
    pu4GoldenCheckSum ++;
    pu4DecCheckSum ++;
  }
  // PP
  for (i = 0; i < 2; i ++)
  {
    if((*pu4GoldenCheckSum) != (*pu4DecCheckSum))
    {
      vVDecOutputDebugString("\n!!!!!!!!! MC Check Sum Compare Error  !!!!!!\n");
      return (FALSE);
    }
    pu4GoldenCheckSum ++;
    pu4DecCheckSum ++;
  }

  return (TRUE);
}

UINT32 u4VDEC_HAL_VP8_VDec_ReadFinishFlag(UINT32 u4VDecID)
{
   return (BOOL )((u4VDecReadVP8VLD(0,RO_VP8_PFR)&1) ||(u4VDecReadVP8VLD(0,RO_VP8_VOKR)&1));
}

#endif

