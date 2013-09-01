
#include "vdec_hw_common.h"

//#include "vdec_info_rm.h"

#include "vdec_hw_rm.h"

#include "vdec_hal_errcode.h"

#if CONFIG_DRV_VERIFY_SUPPORT
#include "../verify/vdec_verify_general.h"
#include "../verify/vdec_info_verify.h"

//#include <string.h>
//#include <stdio.h>
//extern void vVDecOutputDebugString(const CHAR * format, ...);
//extern BOOL fgWrMsg2PC(void* pvAddr, UINT32 u4Size, UINT32 u4Mode, VDEC_INFO_VERIFY_FILE_INFO_T *pFILE_INFO);
//extern void vVDecOutputDebugString(const CHAR * format, ...);
#endif


#ifdef RM_CRCCHECKFLOW_SUPPORT
void vVDecWriteRMCRC(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
  if (u4VDecID == 0)
  {
    vWriteReg(VDEC_CRC_REG_OFFSET0 + u4Addr, u4Val);
    vVDecSimDumpW(u4VDecID, VDEC_CRC_REG_OFFSET0, u4Addr, u4Val);
  }
  else
  {
    ASSERT(0);
    //vWriteReg(RM_VLD_REG_OFFSET1 + u4Addr, u4Val);
  }
}
#endif //RM_CRCCHECKFLOW_SUPPORT


void vVDecWriteRMVLD(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
  if (u4VDecID == 0)
  {
    vWriteReg(RM_VLD_REG_OFFSET0 + u4Addr, u4Val);
    vVDecSimDumpW(u4VDecID, RM_VLD_REG_OFFSET0, u4Addr, u4Val);
  }
  else
  {
    ASSERT(0);
    //vWriteReg(RM_VLD_REG_OFFSET1 + u4Addr, u4Val);
  }
}


UINT32 u4VDecReadRMVLD(UINT32 u4VDecID, UINT32 u4Addr)
{
  UINT32 u4Val;

  if (u4VDecID == 0)
  {
    u4Val = (u4ReadReg(RM_VLD_REG_OFFSET0 + u4Addr));
    vVDecSimDumpR(u4VDecID, RM_VLD_REG_OFFSET0, u4Addr, u4Val);
  }
  else
  {
    ASSERT(0);
    //return (u4ReadReg(RM_VLD_REG_OFFSET1 + u4Addr));
    return (u4ReadReg(RM_VLD_REG_OFFSET0 + u4Addr));
  }

  return u4Val;
}

void vVDecWriteRMPP(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
    if (u4VDecID == 0)
    {
        vWriteReg(RM_VDEC_PP_BASE + u4Addr, u4Val);
	    vVDecSimDumpW(u4VDecID, RM_VDEC_PP_BASE, u4Addr, u4Val);
    }
    else
    {
        ASSERT(0);
        //vWriteReg(RM_VDEC_PP_BASE + u4Addr, u4Val);
    }
}

UINT32 u4VDecReadRMPP(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val;

    if (u4VDecID == 0)
    {
        u4Val = (u4ReadReg(RM_VDEC_PP_BASE + u4Addr));
		vVDecSimDumpR(u4VDecID, RM_VDEC_PP_BASE, u4Addr, u4Val);
    }
    else
    {
        ASSERT(0);
        return (u4ReadReg(RM_VDEC_PP_BASE + u4Addr));
        //return (u4ReadReg(RM_VDEC_PP_BASE + u4Addr));
    }

	return u4Val;
}

void vVDecWriteRMMV(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
    if (u4VDecID == 0)
    {
        vWriteReg(AVC_MV_REG_OFFSET0 + u4Addr, u4Val);
		vVDecSimDumpW(u4VDecID, AVC_MV_REG_OFFSET0, u4Addr, u4Val);
    }
    else
    {
        ASSERT(0);
        //vWriteReg(AVC_MV_REG_OFFSET1 + u4Addr, u4Val);
    }
}

UINT32 u4VDecReadRMMV(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val;

    if (u4VDecID == 0)
    {
        u4Val = (u4ReadReg(AVC_MV_REG_OFFSET0 + u4Addr));
		vVDecSimDumpR(u4VDecID, AVC_MV_REG_OFFSET0, u4Addr, u4Val);
    }
    else
    {
        ASSERT(0);
        return (u4ReadReg(AVC_MV_REG_OFFSET0 + u4Addr));
        //return (u4ReadReg(AVC_MV_REG_OFFSET1 + u4Addr));
    }
}

void vRM_VldSoftReset(UINT32 u4VDecID)
{
  // Soft Reset, Use Vdec Common HW Layer
  #ifdef VDEC_RM_USE_NORMAL_HAL_HW
#if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8555)
  vVDecResetHW(u4VDecID);
#elif (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560)
    vVDecResetHW(u4VDecID, VDEC_RM);
#else
    vVDecResetHW(u4VDecID, VDEC_UNKNOWN);
#endif
  #else //VDEC_RM_USE_NORMAL_HAL_HW
  //vWriteReg(VLD_REG_OFFSET0 + WO_VLD_SRST, 0x1);    //Common VLD SRST, 66x4 = 264 => 0x108 => WO_VLD_SRST
  vVDecWriteVLD(u4VDecID, WO_VLD_SRST, 0x1);
  //vWriteReg(VLD_REG_OFFSET0 + WO_VLD_SRST, 0x0);    //Common VLD SRST, 66x4 = 264 => 0x108 => WO_VLD_SRST
  vVDecWriteVLD(u4VDecID, WO_VLD_SRST, 0x0);
  #endif //VDEC_RM_USE_NORMAL_HAL_HW
}

void vRM_MvInit(UINT32 u4VDecID, UINT32 u4MVHwWorkBuf)
{
  // MV global setting //Must be 16 Byte Align
  if (u4MVHwWorkBuf == 0)
  {
    ASSERT(0);
  }

  #ifdef VDEC_RM_USE_NORMAL_HAL_HW
  vVDecWriteRMMV(u4VDecID, RW_AMV_WR_ADDR, PHYSICAL(u4MVHwWorkBuf) >> 4);
  vVDecWriteRMMV(u4VDecID, RW_RMMV_CPLOCPICTYPE, 0x0000);
  	
  //vWriteReg(AVC_MV_REG_OFFSET0 + RW_AMV_WR_ADDR, u4MVHwWorkBuf >> 4);    //AVC MV, 131x4 = 524 => 0x20c
  //vWriteReg(AVC_MV_REG_OFFSET0 + RW_RMMV_CPLOCPICTYPE, 0x0000);                             //AVC MV, 134x4 = 536 => 0x218
  #else //VDEC_RM_USE_NORMAL_HAL_HW
  //vWriteReg(AVC_MV_REG_OFFSET0 + 131 * 4, PHYSICAL(u4MVHwWorkBuf) >> 4);    //AVC MV, 131x4 = 524 => 0x20c
  vVDecWriteRMMV(u4VDecID, 131 * 4, PHYSICAL(u4MVHwWorkBuf) >> 4);
  //vWriteReg(AVC_MV_REG_OFFSET0 + 134 * 4, 0x0000);                             //AVC MV, 134x4 = 536 => 0x218
  vVDecWriteRMMV(u4VDecID, 134 * 4, 0x0000);
  #endif //VDEC_RM_USE_NORMAL_HAL_HW
}

void vRM_McInit(UINT32 u4VDecID)
{
  #ifdef VDEC_RM_USE_NORMAL_HAL_HW
  vVDecWriteMC(u4VDecID, RW_MC_HREFP, 0x0);      //Common MC, 28x4 = 112 => 0x70
  vVDecWriteMC(u4VDecID, RW_MC_ADDRSWAP, 0x4);      //Common MC, 36x4 = 144 => 0x90
  vVDecWriteMC(u4VDecID, RW_MC_FWDP, 0x0);      //Common MC, 6x4 = 24 => 0x18
  vVDecWriteMC(u4VDecID, RW_MC_DISSOLVE, 0x0);     //Common MC, 38x4 = 152 => 0x98
  vVDecWriteMC(u4VDecID, RW_MC_PS_DRAM_MODE, 0x0);   //Common MC, 114x4 = 456 => 0x1C8
  vVDecWriteMC(u4VDecID, RW_MC_OPBUF, 0x4);       //Common MC, 9x4 = 36 => 0x24
  #else //VDEC_RM_USE_NORMAL_HAL_HW
  vWriteReg(MC_REG_OFFSET0 + 28*4, 0x0);      //Common MC, 28x4 = 112 => 0x70
  vWriteReg(MC_REG_OFFSET0 + 36*4, 0x4);      //Common MC, 36x4 = 144 => 0x90
  vWriteReg(MC_REG_OFFSET0 +  6*4, 0x0);      //Common MC, 6x4 = 24 => 0x18
  vWriteReg(MC_REG_OFFSET0 + 38*4, 0x0);     //Common MC, 38x4 = 152 => 0x98
  vWriteReg(MC_REG_OFFSET0 + 114*4, 0x0);   //Common MC, 114x4 = 456 => 0x1C8
  vWriteReg(MC_REG_OFFSET0 + 9*4, 0x4);       //Common MC, 9x4 = 36 => 0x24
  #endif //VDEC_RM_USE_NORMAL_HAL_HW
}


void vRM_PpInit(UINT32 u4VDecID)
{
  // PP Global Setting
}


void vRM_VldInit(UINT32 u4VDecID, UINT32 u4VldPredHwWorkBuf)
{
  #ifdef VDEC_RM_USE_NORMAL_HAL_HW
  #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
  vVDecWriteVLDTOP(u4VDecID, RW_RM_TOPVLD_TIMEOUT, 0x80000000);
  #else
  vVDecWriteRMVLD(u4VDecID, RW_RMVLD_TIMEOUT, 0x80000000);
  #endif
  
  #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
  vVDecWriteVLDTOP(u4VDecID, RW_RM_TOPVLD_PREDADDR, PHYSICAL(u4VldPredHwWorkBuf));
  #else
  vVDecWriteRMVLD(u4VDecID, RW_RMVLD_PREDADDR, PHYSICAL(u4VldPredHwWorkBuf));
  #endif
  
  #else //VDEC_RM_USE_NORMAL_HAL_HW
  vWriteReg(RM_VLD_REG_OFFSET0 + 38*4, 0x80000000);    //Time Out    //RM VLD, 38x4 = 152 => 0x98
  vWriteReg(RM_VLD_REG_OFFSET0 + 39*4, PHYSICAL(u4VldPredHwWorkBuf));     //RM VLD, 39x4 = 156 => 0x9C
  #endif //VDEC_RM_USE_NORMAL_HAL_HW
}

UINT32 u4RM_VDecVLDGetBitS(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBit)
{
  UINT32 u4RegVal;
  #if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8580) 
  UCHAR ucTmp=0;
  #endif
  u4RegVal = u4VDecReadRMVLD(u4VDecID, RO_VLD_BARL + (u4BSID << 10) + (u4ShiftBit << 2));

  #if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8580) 
  /* added by Alan Cheng for JPEG debug error according to GS Lin */
  ucTmp = (u4VDecReadVLD(u4VDecID, WO_VLD_RDY)&0x10000000)>>28;
  while (!ucTmp)
  {
    ucTmp = (u4VDecReadVLD(u4VDecID, WO_VLD_RDY)&0x10000000)>>28;
  }
  //u4VDecReadVLD(u4VDecID, RO_VLD_BARL + (u4BSID << 10));
  /* end */   
  #endif
  return (u4RegVal);
}

INT32 i4RM_InitBarrelShifter(UINT32 u4BSID, UINT32 u4VDecID, VDEC_INFO_RM_BS_INIT_PRM_T *prMpegBSInitPrm)
{
    BOOL fgFetchOK = FALSE;
    #if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8580) 
    INT32 i;
    #endif
    INT32 j;
    UINT32 u4VLDRemainByte;
    UINT32 u4Value = 0;    

    vVDecWriteRMVLD(u4VDecID, RW_RMVLD_CTRL, CFG_RM_CDECODER);
    //vWriteReg(RM_VLD_REG_OFFSET0 + 0x84, 0x1);    // RM VLD, 0x84, RM_CTRL
   #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580) 
      //polling
      if((u4VDecReadVLD(u4VDecID,RO_VLD_SRAMCTRL) & (0x1<<15)))
      while (!(u4VDecReadVLD(u4VDecID,RO_VLD_SRAMCTRL) & 1));

      vVDecSetVLDVFIFO(u4BSID, u4VDecID, PHYSICAL(prMpegBSInitPrm->u4VFifoSa), PHYSICAL(prMpegBSInitPrm->u4VFifoEa));
      
       u4VLDRemainByte =  (PHYSICAL(prMpegBSInitPrm->u4ReadPointer)) & 0xf;

        vVDecWriteVLD(u4VDecID, RW_VLD_RPTR + (u4BSID << 10), PHYSICAL(prMpegBSInitPrm->u4ReadPointer));
        vVDecWriteVLD(u4VDecID, WO_VLD_WPTR + (u4BSID << 10), PHYSICAL(prMpegBSInitPrm->u4WritePointer));
        //vVDecWriteVLD(u4VDecID, RW_VLD_ASYNC + (u4BSID << 10), u4VDecReadVLD(u4VDecID, RW_VLD_ASYNC) | VLD_WR_ENABLE);//mark

        //Reset async fifo for mt8580 new design
         vVDecWriteVLD(u4VDecID, WO_VLD_SRST , (0x1 << 8));
         vVDecWriteVLD(u4VDecID, WO_VLD_SRST , 0);
  
         vVDecWriteVLD(u4VDecID, 4*51, 1000); //sun for temp test
         
        // start to fetch data
        vVDecWriteVLD(u4VDecID, RW_VLD_PROC + (u4BSID << 10), VLD_INIFET);

        u4Value= u4VDecReadVLD(u4VDecID, RO_VLD_FETCHOK);
        while (!(u4Value & 0x1))
        {
          u4Value= u4VDecReadVLD(u4VDecID, RO_VLD_FETCHOK);
        }

        vVDecWriteVLD(u4VDecID, RW_VLD_PROC + (u4BSID << 10), VLD_INIBR);
  
        if (fgVDecWaitVldFetchOk(u4BSID, u4VDecID))
        {
            fgFetchOK = TRUE;
        }
   #else
    vVDecWriteVLD(u4VDecID,RW_VLD_RDY_SWTICH + (u4BSID << 10), READY_TO_RISC_1);

    vVDecSetVLDVFIFO(u4BSID, u4VDecID, PHYSICAL(prMpegBSInitPrm->u4VFifoSa), PHYSICAL(prMpegBSInitPrm->u4VFifoEa));
    //vVDecSetVLDVFIFO(u4BSID, u4VDecID, u4AbsDramANc(prMpegBSInitPrm->u4VFifoSa), u4AbsDramANc(prMpegBSInitPrm->u4VFifoEa));

    u4VLDRemainByte =  (PHYSICAL(prMpegBSInitPrm->u4ReadPointer)) & 0xf;
    //u4VLDRemainByte =  (u4AbsDramANc(prMpegBSInitPrm->u4ReadPointer)) & 0xf;
    
    // prevent initialize barrel fail
    for (i = 0; i < 5; i++)
    {
        //#ifdef RM_RINGVIFO_FLOW
        vVDecWriteVLD(u4VDecID, WO_VLD_WPTR, u4VDecReadVLD(u4VDecID, WO_VLD_WPTR) | VLD_CLEAR_PROCESS_EN);
        //#endif //RM_RINGVIFO_FLOW
        vVDecWriteVLD(u4VDecID, RW_VLD_RPTR + (u4BSID << 10), PHYSICAL(prMpegBSInitPrm->u4ReadPointer));
        vVDecWriteVLD(u4VDecID, RW_VLD_RPTR + (u4BSID << 10), PHYSICAL(prMpegBSInitPrm->u4ReadPointer));
        
        //#ifdef RM_RINGVIFO_FLOW
        vVDecWriteVLD(u4VDecID, WO_VLD_WPTR + (u4BSID << 10), PHYSICAL(prMpegBSInitPrm->u4WritePointer));
        vVDecWriteVLD(u4VDecID, RW_VLD_ASYNC + (u4BSID << 10), u4VDecReadVLD(u4VDecID, RW_VLD_ASYNC) | VLD_WR_ENABLE);
        //#else //RM_RINGVIFO_FLOW
        //vVDecWriteVLD(u4VDecID, WO_VLD_WPTR + (u4BSID << 10), 0xFFFFFFFF);//u4AbsDramANc(prMpegBSInitPrm->u4WritePointer));
        //#endif //RM_RINGVIFO_FLOW

        // start to fetch data
        vVDecWriteVLD(u4VDecID, RW_VLD_PROC + (u4BSID << 10), VLD_INIFET);

        u4Value= u4VDecReadVLD(u4VDecID, RO_VLD_FETCHOK);
        while (!(u4Value & 0x1))
        {
          u4Value= u4VDecReadVLD(u4VDecID, RO_VLD_FETCHOK);
        }

        vVDecWriteVLD(u4VDecID, RW_VLD_PROC + (u4BSID << 10), VLD_INIBR);

        u4Value= u4VDecReadVLD(u4VDecID, RO_VLD_VBAR);
        while((((u4Value & 0xf) * 4) + (((u4Value) >> 24) & 0x3)) !=3);
        {
          u4Value= u4VDecReadVLD(u4VDecID, RO_VLD_FETCHOK);
        }       
    
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
  
    for (j=0;j<u4VLDRemainByte;j++)
    {
        u4RM_VDecVLDGetBitS(u4BSID, u4VDecID, 8);
    }

    //Reset Byte Cnt
    vVDecWriteRMVLD(u4VDecID, RW_RMVLD_BCNTRST, CFG_RM_RST1);
    vVDecWriteRMVLD(u4VDecID, RW_RMVLD_BCNTRST, CFG_RM_RST0);
           
    return HAL_HANDLE_OK;
}


