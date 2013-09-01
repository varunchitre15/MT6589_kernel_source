
#include "vdec_hw_common.h"
#include "../include/vdec_info_vp6.h"
#include "vdec_hw_vp6.h"
//#include "x_hal_ic.h"

#if CONFIG_DRV_VERIFY_SUPPORT
#include "../verify/vdec_verify_general.h"
#include "../verify/vdec_info_verify.h"
#include <linux/string.h>

#if (!CONFIG_DRV_LINUX)
#include <string.h>
#include <stdio.h>
#endif

extern void vVDecOutputDebugString(const CHAR * format, ...);
extern BOOL fgWrMsg2PC(void* pvAddr, UINT32 u4Size, UINT32 u4Mode, VDEC_INFO_VERIFY_FILE_INFO_T *pFILE_INFO);
extern void vVDecOutputDebugString(const CHAR * format, ...);
#endif


//MULTI-STREAM PANDA
void vVDecVP6WriteVLD2(UINT32 u4Addr, UINT32 u4Val)
{
    vWriteReg(VP6_VLD2_REG_OFFSET + u4Addr, u4Val);
    vVDecSimDumpW(0, VP6_VLD2_REG_OFFSET, u4Addr, u4Val);
}

UINT32 u4VDecVP6ReadVLD2(UINT32 u4Addr)
{
    UINT32 u4Val;
    u4Val = u4ReadReg(VP6_VLD2_REG_OFFSET + u4Addr);
    vVDecSimDumpR(0, VP6_VLD2_REG_OFFSET, u4Addr, u4Val);

    return u4Val;
}

void vVDecVP6WriteVLD2Shift(UINT32 u4Addr, UINT32 u4Val)
{
    vWriteReg(VP6_VLD2_SHIFT_OFFSET + u4Addr, u4Val);
    vVDecSimDumpW(0, VP6_VLD2_SHIFT_OFFSET, u4Addr, u4Val);
}

UINT32 u4VDecVP6ReadVLD2Shift(UINT32 u4Addr)
{
    UINT32 u4Val;
    u4Val = u4ReadReg(VP6_VLD2_SHIFT_OFFSET + u4Addr);
    vVDecSimDumpR(0, VP6_VLD2_SHIFT_OFFSET, u4Addr, u4Val);

    return u4Val;
}

void vVDecVP6SetVLD2VFIFO(UINT32 u4VFifoSa, UINT32 u4VFifoEa)
{
    vVDecVP6WriteVLD2(RW_VLD_VSTART, u4VFifoSa >> 6);
    vVDecVP6WriteVLD2(RW_VLD_VEND, u4VFifoEa >> 6);
}

BOOL fgVDecVP6WaitVld2FetchOk(void)
{
    UINT32 u4Cnt = 0;

    while((u4VDecVP6ReadVLD2(RO_VLD_FETCHOK) & VLD_FETCH_OK) == 0)
    {
        u4Cnt++;
        if (u4Cnt >= 0x1000)
        {
            return (FALSE);
        }
    }
    return (TRUE);
}

void vVDecWriteVP6VLD(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val, UINT32 u4BSID)
{
     if (u4VDecID == 0)
    {
        vWriteReg(VP6_VLD_REG_OFFSET0 + u4Addr, u4Val);
        vVDecSimDumpW(u4VDecID, VP6_VLD_REG_OFFSET0, u4Addr, u4Val);
    }
    else
    {
        vWriteReg(VP6_VLD_REG_OFFSET1 + u4Addr, u4Val);
        vVDecSimDumpW(u4VDecID, VP6_VLD_REG_OFFSET1, u4Addr, u4Val);
    }
}

UINT32 u4VDecReadVP6VLD(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val;
    if (u4VDecID == 0)
    {
        u4Val = u4ReadReg(VP6_VLD_REG_OFFSET0 + u4Addr);
        vVDecSimDumpR(u4VDecID, VP6_VLD_REG_OFFSET0, u4Addr, u4Val);
    }
    else
    {
        u4Val = u4ReadReg(VP6_VLD_REG_OFFSET1 + u4Addr);
        vVDecSimDumpR(u4VDecID, VP6_VLD_REG_OFFSET1, u4Addr, u4Val);
    }

    return u4Val;
}

void vVDecWriteVP6MC(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
    if (u4VDecID == 0)
    {
        vWriteReg(VP6_MV_REG_OFFSET0 + u4Addr, u4Val);
        vVDecSimDumpW(u4VDecID, VP6_MV_REG_OFFSET0, u4Addr, u4Val);
    }
    else
    {
        vWriteReg(VP6_MV_REG_OFFSET1 + u4Addr, u4Val);
        vVDecSimDumpW(u4VDecID, VP6_MV_REG_OFFSET1, u4Addr, u4Val);
    }
}

UINT32 u4VDecReadVP6MC(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val;
    if (u4VDecID == 0)
    {
        u4Val = u4ReadReg(VP6_MV_REG_OFFSET0 + u4Addr);
        vVDecSimDumpR(u4VDecID, VP6_MV_REG_OFFSET0, u4Addr, u4Val);
    }
    else
    {
        u4Val = u4ReadReg(VP6_MV_REG_OFFSET1 + u4Addr);
        vVDecSimDumpR(u4VDecID, VP6_MV_REG_OFFSET1, u4Addr, u4Val);
    }
    return u4Val;
}

#define RPTR_ALIGN   16
#define WPTR_OFFSET  512
    
void vVDecVP6SetVLD2Wptr(UINT32 u4WPtr)
{
    UINT32 u4WPtrAlign, u4VStart, u4VEnd;

    u4WPtrAlign = ((u4WPtr + RPTR_ALIGN) & (~((UINT32)RPTR_ALIGN - 1))) + WPTR_OFFSET;

    u4VStart    = u4VDecVP6ReadVLD2(RW_VLD_VSTART)<<6;
    u4VEnd      = u4VDecVP6ReadVLD2(RW_VLD_VEND)<<6;    

    if(u4WPtrAlign < u4VStart)
    {
        u4WPtrAlign = u4VEnd - (u4VStart - u4WPtrAlign);
    }
    
    if(u4WPtrAlign > u4VEnd)
    {
        u4WPtrAlign = u4VStart + (u4WPtrAlign - u4VEnd);
    }
    
    vVDecVP6WriteVLD2(WO_VLD_WPTR, PHYSICAL(u4WPtrAlign));
}

UINT32 u4VDecVP6VLD2GetBits(UINT32 dShiftBit)
{
    return u4VDecVP6ReadVLD2Shift(RO_VP6_VLD_BARL + (dShiftBit << 2));
}

BOOL fgVDecIsVLD2FetchOk(UINT32 u4VDecID)
{
    if ((u4VDecVP6ReadVLD2(RO_VLD_FETCHOK) & VLD_FETCH_OK) == 0)
    {
        return (FALSE);
    }
    return (TRUE);
}

BOOL fgVDecWaitVld2FetchOk(UINT32 u4VDecID)
{
    UINT32 u4Cnt;
  
    if (!fgVDecIsVLD2FetchOk(u4VDecID))
    {
        u4Cnt = 0;
        while (!fgVDecIsVLD2FetchOk(u4VDecID))
        {
            u4Cnt++;
            if (u4Cnt >= WAIT_THRD)
            {
                return (FALSE);
            }
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
UINT32 u4VDecVP6VLDGetBits(UINT32 u4BSID,UINT32 u4VDecID,UINT32 dShiftBit)
{
    UINT32 u4RegVal = 0;
    //UCHAR ucTmp=0;

    if (dShiftBit == 0)
    {
       u4RegVal = u4VDecReadVP6VLD(u4VDecID, RO_VP6_VLD_BARL + (u4BSID << 10) + (dShiftBit << 2));
    }
    else
    {
        u4RegVal = u4VDecReadVP6VLD(u4VDecID, RO_VP6_VLD_BARL + (u4BSID << 10) + (dShiftBit << 2));
    }
    
    return (u4RegVal);
}

UINT32 u4VDecReadVP6VldRPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 *pu4Bits, UINT32 u4VFIFOSa)
{
     UINT32 u4DramRptr;
    UINT32 u4SramRptr, u4SramWptr;
    UINT32 u4SramDataSz;
    UINT32 u4ByteAddr;
  
    // HW issue, wait for read pointer stable
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
    while (!(u4VDecReadVLD(u4VDecID, RO_VLD_SRAMCTRL)&1));

    u4DramRptr = u4VDecReadVLD(u4VDecID, RO_VLD_VRPTR + (u4BSID << 10));
  u4SramRptr = ((u4VDecReadVLD(u4VDecID, RO_VLD_VBAR + (u4BSID << 10))) & 0xf) * 4 +
                     (((u4VDecReadVLD(u4VDecID, RO_VLD_VBAR + (u4BSID << 10))>>24)) & 0x3); //count in 128bits
  u4SramWptr = (((u4VDecReadVLD(u4VDecID, RO_VLD_VBAR + (u4BSID << 10))>>8)) & 0xf) *4;
#else    
    while (!(u4VDecReadVLD(u4VDecID, RO_VLD_SRAMCTRL + (u4BSID << 10))&0x10000));
  
    u4DramRptr = u4VDecReadVLD(u4VDecID, RO_VLD_VRPTR + (u4BSID << 10));
    u4SramRptr = u4VDecReadVLD(u4VDecID, RO_VLD_VBAR + (u4BSID << 10)) & 0xff;
    u4SramWptr = (u4VDecReadVLD(u4VDecID, RO_VLD_VBAR + (u4BSID << 10)) >> 16) & 0xff;
#endif

    if (u4SramWptr > u4SramRptr)
    {
        u4SramDataSz = u4SramWptr - u4SramRptr;
    }
    else
    {
        u4SramDataSz = 64 - (u4SramRptr - u4SramWptr);
    }
  
    *pu4Bits = u4VDecReadVLD(u4VDecID, RW_VLD_BITCOUNT + (u4BSID << 10)) & 0x3f;
  
    if (u4VDecReadVLD(u4VDecID, RW_VLD_WMV_ABS + (u4BSID << 10)) & 0x1) // 03 replacement enable!
    {
        u4ByteAddr = u4DramRptr - (u4SramDataSz + 4) * 4 + (*pu4Bits / 8);
    }
    else
    {
        u4ByteAddr = u4DramRptr - (u4SramDataSz + 3)* 4 + (*pu4Bits / 8);
    }
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


UINT32 u4VDecWaitVP6GetBitsReady(UINT32 u4VDecID, UINT32 u4BSID)
{
    UINT32 u4Val = 0;
    //UINT32 u4TryCnt = 0;

    u4Val = u4VDecReadVP6VLD(u4VDecID, RW_VP6_GBFR + (u4BSID << 10));   
    //Remove polling
    /*
    while ( !(u4Val & 1))
    {
         u4Val = u4VDecReadVP6VLD(u4VDecID, RW_VP6_GBFR + (u4BSID << 10));
         u4TryCnt++;

         if ( u4TryCnt > 1000000)
         {
            break;
         }
    }*/

    return u4Val;
}


// *********************************************************************
// Function : UINT32 u4VDecVP6BOOLGetBits(UINT32 u4BSID,UINT32 u4VDecID,UINT32 dShiftBit)
// Description : Get Bitstream from VLD barrel shifter
// Parameter : dShiftBit: Bits to shift (0-32)
// Return    : barrel shifter
// *********************************************************************
UINT32 u4VDecVP6BOOLGetBits(UINT32 u4BSID,UINT32 u4VDecID,UINT32 dShiftBit)
{
    UINT32 u4RegVal;
    UINT32 u4Tmp=0;
    //UINT32 u4TryCnt = 0;
    
    vVDecWriteVP6VLD(u4VDecID, RW_VP6_BCPR + (u4BSID << 10), dShiftBit, u4BSID);
    u4Tmp =  u4VDecWaitVP6GetBitsReady(u4VDecID, u4BSID);
    u4RegVal = ((u4Tmp & 0xFF00) >> 8);
 
    return (u4RegVal);
}


void vVDecWriteVP6PP(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val, UINT32 u4BSID)
{
    //UINT32 u4Tmp = 0;
    //UINT32 u4TryCnt = 0;
     if (u4VDecID == 0)
    {
        vWriteReg(VP6_PP_REG_OFFSET0 + u4Addr, u4Val);
        vVDecSimDumpW(u4VDecID, VP6_PP_REG_OFFSET0, u4Addr, u4Val);
    }
    else
    {
        ASSERT(0);
        vWriteReg(VP6_PP_REG_OFFSET0 + u4Addr, u4Val);
        vVDecSimDumpW(u4VDecID, VP6_PP_REG_OFFSET0, u4Addr, u4Val);
    }
}

UINT32 u4VDecReadVP6PP(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val;

    if (u4VDecID == 0)
    {
        u4Val = u4ReadReg(VP6_PP_REG_OFFSET0 + u4Addr);
        vVDecSimDumpR(u4VDecID, VP6_PP_REG_OFFSET0, u4Addr, u4Val);
    }
    else
    {
        ASSERT(0);
        u4Val = u4ReadReg(VP6_PP_REG_OFFSET0 + u4Addr);
        vVDecSimDumpR(u4VDecID, VP6_PP_REG_OFFSET0, u4Addr, u4Val);
}

    return u4Val;
}


UINT32 u4VDecReadVP6DCAC(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val = 0;
    if (u4VDecID == 0)
    {
        u4Val = u4ReadReg(VP6_DCAC_REG_OFFSET0+ u4Addr);
        vVDecSimDumpR(u4VDecID, VP6_DCAC_REG_OFFSET0, u4Addr, u4Val);
    }
    else
    {
        //u4Val = u4ReadReg(VP6_MV_REG_OFFSET1 + u4Addr);
        //vVDecSimDumpR(u4VDecID, VP6_MV_REG_OFFSET1, u4Addr, u4Val);
    }
    return u4Val;
}

