
#include "vdec_hw_common.h"
#include "../include/vdec_info_vp8.h"
#include "../include/vdec_info_common.h"
#include "vdec_hw_vp8.h"
//#include "x_hal_ic.h"
//#include "x_debug.h"

#if CONFIG_DRV_VERIFY_SUPPORT
#include "../verify/vdec_verify_general.h"
#include "../verify/vdec_info_verify.h"
#include "../verify/vdec_verify_mpv_prov.h"

#if (!CONFIG_DRV_LINUX)
#include <string.h>
#include <stdio.h>
#endif
extern void vVDecOutputDebugString(const CHAR * format, ...);
extern BOOL fgWrMsg2PC(void* pvAddr, UINT32 u4Size, UINT32 u4Mode, VDEC_INFO_VERIFY_FILE_INFO_T *pFILE_INFO);
extern void vVDecOutputDebugString(const CHAR * format, ...);
#endif
//int UTIL_Printf( const char * format, ... );
//int sprintf ( char * str, const char * format, ... );

void vVDecWriteVP8VLD(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val, UINT32 u4BSID)
{
     u4VDecID = 0;
     if (u4VDecID == 0)
    {
        vWriteReg(VP8_VLD_REG_OFFSET0 + u4Addr, u4Val);
//        printk("VP8Write(VP8VLD(0x%X) + 0x%X[%d])=0x%X\n",VP8_VLD_REG_OFFSET0,u4Addr,u4Addr>>2,u4Val);            
#ifdef VP8_REG_DUMP
	   if (_fgVP8DumpReg)
	   	{
	   	printk("RISCWrite(`VP8_VLD_BASE + %d*4, 32'h%x); \n", u4Addr>>2, u4Val);
	   	}
#endif
#ifdef VDEC_SIM_DUMP
        vVDecSimDump(u4VDecID, VP8_VLD_REG_OFFSET0, u4Addr, u4Val);
#endif
    }
    else
    {
        vWriteReg(VP8_VLD_REG_OFFSET1 + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
        vVDecSimDump(u4VDecID, VP8_VLD_REG_OFFSET1, u4Addr, u4Val);
#endif
    }
}

UINT32 u4VDecReadVP8VLD(UINT32 u4VDecID, UINT32 u4Addr)
{
    if (u4VDecID == 0)
    {
        return (u4ReadReg(VP8_VLD_REG_OFFSET0 + u4Addr));
    }
    else
    {
        return (u4ReadReg(VP8_VLD_REG_OFFSET1 + u4Addr));
    }
}

void vVDecWriteVP8MC(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
     u4VDecID = 0;
    if (u4VDecID == 0)
    {
        vWriteReg(MC_REG_OFFSET0 + u4Addr, u4Val);
#ifdef VP8_REG_DUMP
	   if (_fgVP8DumpReg)
	   	{
	   	printk("RISCWrite('MC_REG_OFFSET0 + %d*4, 32'h%x); \n", u4Addr>>2, u4Val);
	   	}
#endif
#ifdef VDEC_SIM_DUMP
        vVDecSimDump(u4VDecID, MC_REG_OFFSET0, u4Addr, u4Val);
#endif
    }
    else
    {
        vWriteReg(MC_REG_OFFSET1 + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
        vVDecSimDump(u4VDecID, MC_REG_OFFSET1, u4Addr, u4Val);
#endif
    }
}

UINT32 u4VDecReadVP8MC(UINT32 u4VDecID, UINT32 u4Addr)
{
    if (u4VDecID == 0)
    {
        return (u4ReadReg(MC_REG_OFFSET0 + u4Addr));
    }
    else
    {
        return (u4ReadReg(MC_REG_OFFSET1 + u4Addr));
    }
}
void vVDecWriteVP8MV(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
     u4VDecID = 0;
    if (u4VDecID == 0)
    {
        vWriteReg(VP8_MV_REG_OFFSET0 + u4Addr, u4Val);
#ifdef VP8_REG_DUMP
	   if (_fgVP8DumpReg)
	   	{
	   	printk("RISCWrite(VP8MV(0x%X) + 0x%X[%d])=0x%X\n",VP8_MV_REG_OFFSET0,u4Addr,u4Addr>>2,u4Val);    
	   	}
#endif
#ifdef VDEC_SIM_DUMP
        vVDecSimDump(u4VDecID, VP8_MV_REG_OFFSET0, u4Addr, u4Val);
#endif
    }
    else
    {
        vWriteReg(VP8_MV_REG_OFFSET1 + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
        vVDecSimDump(u4VDecID, VP8_MV_REG_OFFSET1, u4Addr, u4Val);
#endif
    }
}

UINT32 u4VDecReadVP8MV(UINT32 u4VDecID, UINT32 u4Addr)
{
    if (u4VDecID == 0)
    {
        return (u4ReadReg(VP8_MV_REG_OFFSET0 + u4Addr));
    }
    else
    {
        return (u4ReadReg(VP8_MV_REG_OFFSET1 + u4Addr));
    }
}void vVDecWriteVP8VLD2(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
     u4VDecID = 0;

    if (u4VDecID == 0)
    {
        vWriteReg(VP8_VLD2_REG_OFFSET0 + u4Addr, u4Val);
#ifdef VP8_REG_DUMP
	   if (_fgVP8DumpReg)
	   	{
	   	printk("VP8Write(VLD2(0x%X) + 0x%X[%d])=0x%X\n",VP8_VLD2_REG_OFFSET0,u4Addr,u4Addr>>2,u4Val);
	   	}
#endif
        #ifdef VDEC_REG_DUMP
            printk("[VDEC]RISCWrite('VP8_VLD2_REG_OFFSET0 + 4*%d, 32'h%x); \n", u4Addr>>2, u4Val);
        #endif
        
#ifdef VDEC_SIM_DUMP
        vVDecSimDump(u4VDecID, VP8_VLD2_REG_OFFSET0, u4Addr, u4Val);
#endif
    }
    else
    {
        vWriteReg(VP8_VLD2_REG_OFFSET1 + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
        vVDecSimDump(u4VDecID, VP8_VLD2_REG_OFFSET1, u4Addr, u4Val);
#endif
    }
}


UINT32 u4VDecReadVP8VLD2(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4RegVal = 0;

     u4VDecID = 0;

    if (u4VDecID == 0)
    {
        u4RegVal = u4ReadReg(VP8_VLD2_REG_OFFSET0 + u4Addr);
        #ifdef VDEC_REG_DUMP
            printk("[VDEC]RISCRead('VP8_VLD2_REG_OFFSET0 + 4*%d, 32'h%x); \n", u4Addr>>2, u4RegVal);
        #endif
        return u4RegVal;
    }
    else
    {
#ifdef VDEC_PIP_WITH_ONE_HW
        printk("PIP_ONE_HW: Wrong HW ID!!!\n");
        VDEC_ASSERT(0);
#endif    
        return (u4ReadReg(VP8_VLD2_REG_OFFSET1 + u4Addr));
    }
}

void vVDecWriteVP8PP(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
     u4VDecID = 0;

    if (u4VDecID == 0)
    {
        vWriteReg(VP8_PP_REG_OFFSET0 + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
        vVDecSimDump(u4VDecID, VP8_PP_REG_OFFSET0, u4Addr, u4Val);
#endif
        //printk("[VDEC]0x      %x (AVS PP Register %d) =  0x%x\n", u4Addr, u4Addr/4, u4Val );
    }
    else
    {
        vWriteReg(VP8_PP_REG_OFFSET1 + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
        vVDecSimDump(u4VDecID, VP8_PP_REG_OFFSET1, u4Addr, u4Val);
#endif
    }
}

UINT32 u4VDecReadVP8PP(UINT32 u4VDecID, UINT32 u4Addr)
{
     u4VDecID = 0;

    if (u4VDecID == 0)
    {
        return (u4ReadReg(VP8_PP_REG_OFFSET0 + u4Addr));
    }
    else
    {
        return (u4ReadReg(VP8_PP_REG_OFFSET1 + u4Addr));
    }
}



BOOL fgVDecWaitVP8VldFetchOk(UINT32 u4BSID, UINT32 u4VDecID, UINT8 u1VP8VLD)
{
    UINT32 u4Cnt;
  if (u1VP8VLD == VP8_VLD1)
  {
    if ((u4VDecReadVLD(u4VDecID, RO_VLD_FETCHOK) & VLD_FETCH_OK) == 0)
    {
        u4Cnt = 0;
        while ((u4VDecReadVLD(u4VDecID, RO_VLD_FETCHOK) & VLD_FETCH_OK) == 0)
        {
            u4Cnt++;
            if (u4Cnt >= 0x1000)
            {
            printk("VP8VLD 1 WaitSramStable fail\n");
                return (FALSE);
            }
        }
    }
    // HW modification
    // read point may not stable after read fetck ok flag
      u4Cnt = 50000;
      if (u4VDecReadVLD(u4VDecID, RO_VLD_SRAMCTRL) & PROCESS_FLAG)
      	{
      	while((!(u4VDecReadVLD(u4VDecID, RO_VLD_SRAMCTRL)&AA_FIT_TARGET_SCLK)) && (u4Cnt--));
      	}
      if (u4Cnt == 0)
      	{
      	printk("Initial Fetch fail\n");
      	return (FALSE);
      	}
}
else
	
{
    if ((u4VDecReadVP8VLD2(u4VDecID, RO_VLD_FETCHOK) & VLD_FETCH_OK) == 0)
    {
        u4Cnt = 0;
        while ((u4VDecReadVP8VLD2(u4VDecID, RO_VLD_FETCHOK) & VLD_FETCH_OK) == 0)
        {
            u4Cnt++;
            if (u4Cnt >= 0x1000)
            {
            printk("VP8VLD 2 WaitSramStable fail\n");
                return (FALSE);
            }
        }
    }
    u4Cnt = 0;
    // HW modification
    // read point may not stable after read fetck ok flag
      u4Cnt = 50000;
      if (u4VDecReadVLD(u4VDecID, RO_VLD_SRAMCTRL) & PROCESS_FLAG)
      	{
      	while((!(u4VDecReadVLD(u4VDecID, RO_VLD_SRAMCTRL)&AA_FIT_TARGET_SCLK)) && (u4Cnt--));
      	}
      if (u4Cnt == 0)
      	{
      	printk("Initial Fetch 2 fail\n");
      	return (FALSE);
      	}
}

  
    return (TRUE);
}


// *********************************************************************
// Function : UINT32 dVLDGetBitS(UINT32 u4BSID,UINT32 u4VDecID,UINT32 dShiftBit)
// Description : Get Bitstream from VLD barrel shifter
// Parameter : dShiftBit: Bits to shift (0-32)
// Return    : barrel shifter
// *********************************************************************
UINT32 u4VDecVP8VLDGetBits(UINT32 u4BSID,UINT32 u4VDecID,UINT32 dShiftBit)
{
   UINT32 u4VP8_ShiftReg=RO_VP8_VBSR1;
   UINT32 u4RepeatNum=0;
   UINT32 u4RetValue=0;

    if (u4BSID == VP8_VLD2)
    {
      u4VP8_ShiftReg=RO_VP8_VBSR2;    
    }
   u4RepeatNum=dShiftBit>>5;
   while(u4RepeatNum>0)
   {
      u4VDecReadVP8VLD(u4VDecID, u4VP8_ShiftReg+(32<<2));
      u4RepeatNum--;
   }
      u4RepeatNum=dShiftBit&0x1f;
   u4RetValue=(u4VDecReadVP8VLD(u4VDecID, u4VP8_ShiftReg+0)>>(32-dShiftBit));
   u4VDecReadVP8VLD(u4VDecID, u4VP8_ShiftReg+u4RepeatNum*4);

   return u4RetValue;
    
}

UINT32 u4VDEC_VP8_VLDReadLiteral(UINT32 u4VDecID, UINT32 u4Bits)
{
   UINT32 u4RetValue=0;
   u4RetValue = u4VDecReadVP8VLD(u4VDecID, RO_VP8_VRLR+(u4Bits-1)*4);
//            printk("VP8_VLD_READ_LIT(%d); = %d\n",u4Bits ,u4RetValue);
      return u4RetValue;
}


UINT32 u4VDecReadVP8VldRPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 *pu4Bits, UINT32 u4VFIFOSa)
{
     UINT32 u4DramRptr;
    UINT32 u4SramRptr, u4SramWptr;
    UINT32 u4SramDataSz;
    UINT32 u4ByteAddr;
    UINT32 u4Cnt;

    // HW issue, wait for read pointer stable
      u4Cnt = 50000;
      if (u4VDecReadVLD(u4VDecID, RO_VLD_SRAMCTRL) & (1<<15))
      	{
      	while((!(u4VDecReadVLD(u4VDecID, RO_VLD_SRAMCTRL)&0x1)) && (u4Cnt--));
      	}

    u4DramRptr = u4VDecReadVLD(u4VDecID, RO_VLD_VRPTR + (u4BSID << 10));
  u4SramRptr = ((u4VDecReadVLD(u4VDecID, RO_VLD_VBAR + (u4BSID << 10))) & 0xf) * 4 +
                     (((u4VDecReadVLD(u4VDecID, RO_VLD_VBAR + (u4BSID << 10))>>24)) & 0x3); //count in 128bits
  u4SramWptr = (((u4VDecReadVLD(u4VDecID, RO_VLD_VBAR + (u4BSID << 10))>>8)) & 0xf) *4;

    if (u4SramWptr > u4SramRptr)
    {
        u4SramDataSz = u4SramWptr - u4SramRptr;
    }
    else
    {
        u4SramDataSz = 32 - (u4SramRptr - u4SramWptr);
    }
  
    *pu4Bits = u4VDecReadVLD(u4VDecID, RW_VLD_BITCOUNT + (u4BSID << 10)) & 0x3f;
  
        u4ByteAddr = u4DramRptr - (u4SramDataSz + 4) * 4 + (*pu4Bits / 8);

    *pu4Bits &= 0x7;

    if (u4ByteAddr < u4VFIFOSa)
    {
        u4ByteAddr = u4ByteAddr + 
                            ((u4VDecReadVLD(u4VDecID, RW_VLD_VEND + (u4BSID << 10)) << 6) - ((UINT32)u4VFIFOSa))
                            - u4VFIFOSa;
    }
    else
    {
        u4ByteAddr -= ((UINT32)u4VFIFOSa);
    }
  
    return (u4ByteAddr);
  
}


UINT32 u4VDecWaitVP8GetBitsReady(UINT32 u4VDecID, UINT32 u4BSID)
{
    UINT32 u4Val = 0;
    //UINT32 u4TryCnt = 0;

  //  u4Val = u4VDecReadVP8VLD(u4VDecID, RW_vp8_GBFR + (u4BSID << 10));   
    //Remove polling
    /*
    while ( !(u4Val & 1))
    {
         u4Val = u4VDecReadvp8VLD(u4VDecID, RW_vp8_GBFR + (u4BSID << 10));
         u4TryCnt++;

         if ( u4TryCnt > 1000000)
         {
         	break;
         }
    }*/

    return u4Val;
}


// *********************************************************************
// Function : UINT32 u4VDecvp8BOOLGetBits(UINT32 u4BSID,UINT32 u4VDecID,UINT32 dShiftBit)
// Description : Get Bitstream from VLD barrel shifter
// Parameter : dShiftBit: Bits to shift (0-32)
// Return    : barrel shifter
// *********************************************************************
UINT32 u4VDecVP8BOOLGetBits(UINT32 u4BSID, UINT32 u4VDecID,UINT32 dShiftBit)
{
    UINT32 u4RegVal;
    UINT32 u4Tmp=0;
    //UINT32 u4TryCnt = 0;
    
    vVDecWriteVP8VLD(u4VDecID, RO_VP8_VRLR + (u4BSID << 10), dShiftBit, u4BSID);
    u4Tmp =  u4VDecWaitVP8GetBitsReady(u4VDecID, u4BSID);
    u4RegVal = ((u4Tmp & 0xFF00) >> 8);
 
    return (u4RegVal);
}

UINT32 u4VDec_VP8_VLDReadBit(UINT32 u4VDecID, UINT32 u4Prob)
{
   UINT32 u4RetValue=0;
   vVDecWriteVP8VLD(u4VDecID, RW_VP8_FWPROB,u4Prob,0);
   u4RetValue=u4VDecReadVP8VLD(u4VDecID, RO_VP8_VRLR);
   vVDecWriteVP8VLD(u4VDecID, RW_VP8_FWPROB,128,0);
   return u4RetValue;
}

void vVDec_VP8_HwAccCoefProbUpdate(UINT32 u4VDecID)
{
    UINT32 u4WaitCount=0,u4DivValue=0;
   static UINT32 u4MaxCoefProbWaitCount=0;
   vVDecWriteVP8VLD(u4VDecID, RW_VP8_CPUT,1,0);
   while(u4VDecReadVP8VLD(u4VDecID, RW_VP8_CPUT)!=0)
   {
     u4WaitCount++;
     u4DivValue=u4WaitCount/50+u4DivValue;
   }
   if(u4MaxCoefProbWaitCount<u4WaitCount)
   {
     printk("Get NewCoefProb counter=%d\n",u4WaitCount);
     u4MaxCoefProbWaitCount=u4WaitCount;
   }
}

void vVDEC_VP8_HwAccMVProbUpdate(UINT32 u4VDecID)
{
    UINT32 u4WaitCount=0,u4DivValue=0;
   static UINT32 u4MaxMvProbWaitCount=0;
   vVDecWriteVP8VLD(u4VDecID, RW_VP8_MVPUT,1,0);
   while(u4VDecReadVP8VLD(u4VDecID, RW_VP8_MVPUT)!=0)
   {
     u4WaitCount++;
     u4DivValue=u4WaitCount/50+u4DivValue;
   }
   if(u4MaxMvProbWaitCount<u4WaitCount)
   {
     printk("Get NewMVProb counter=%d\n",u4WaitCount);
     u4MaxMvProbWaitCount=u4WaitCount;
   }
}


void vVDecWritevp8PP(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val, UINT32 u4BSID)
{
    //UINT32 u4Tmp = 0;
    //UINT32 u4TryCnt = 0;
     if (u4VDecID == 0)
    {
        vWriteReg(RM_VDEC_PP_BASE + u4Addr, u4Val);
        printk("VP8Write(PP(0x%X) + 0x%X[%d])=0x%X\n",RM_VDEC_PP_BASE,u4Addr,u4Addr>>2,u4Val);
#ifdef VDEC_SIM_DUMP
        vVDecSimDump(u4VDecID, RM_VDEC_PP_BASE, u4Addr, u4Val);
#endif
    }
    else
    {
        vWriteReg(RM_VDEC_PP_BASE + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
        vVDecSimDump(u4VDecID, RM_VDEC_PP_BASE, u4Addr, u4Val);
#endif
    }
}


