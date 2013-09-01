
#include "vdec_hw_common.h"
#include "../include/vdec_info_avs.h"
#include "vdec_hw_avs.h"
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

void vVDecWriteAVSVLD(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#if (CONFIG_DRV_FPGA_BOARD)
     u4VDecID = 0;
#endif

    if (u4VDecID == 0)
    {
        vWriteReg(AVS_VLD_REG_OFFSET0 + u4Addr, u4Val);
        #ifdef VDEC_REG_DUMP
            printk("[VDEC]RISCWrite('VDEC_FULL_ADDR_AVS_VLD + 4*%d, 32'h%x); \n", u4Addr>>2, u4Val);
        #endif
#ifdef VDEC_SIM_DUMP
        vVDecSimDump(u4VDecID, AVS_VLD_REG_OFFSET0, u4Addr, u4Val);
#endif
        //printk("[VDEC]0x      %x (AVS vld Register %d) =  0x%x\n", u4Addr, u4Addr/4, u4Val );
    }
    else
    {
        vWriteReg(AVS_VLD_REG_OFFSET1 + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
        vVDecSimDump(u4VDecID, AVS_VLD_REG_OFFSET1, u4Addr, u4Val);
#endif
    }
}

UINT32 u4VDecReadAVSVLD(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4RegVal = 0;
#if (CONFIG_DRV_FPGA_BOARD)
     u4VDecID = 0;
#endif

    if (u4VDecID == 0)
    {
        u4RegVal = (u4ReadReg(AVS_VLD_REG_OFFSET0 + u4Addr));
        #ifdef VDEC_REG_DUMP
        printk("[VDEC]RISCRead('VDEC_FULL_ADDR_AVS_VLD + 4*%d, 32'h%x); \n", u4Addr>>2, u4RegVal);
        #endif
        return u4RegVal; 
    }
    else
    {
        return (u4ReadReg(AVS_VLD_REG_OFFSET1 + u4Addr));
    }
}

void vVDecWriteAVSMV(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#if (CONFIG_DRV_FPGA_BOARD)
     u4VDecID = 0;
#endif

    if (u4VDecID == 0)
    {
        vWriteReg(AVC_MV_REG_OFFSET0 + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
        vVDecSimDump(u4VDecID, AVC_MV_REG_OFFSET0, u4Addr, u4Val);
#endif

        //printk("[VDEC]0x      %x (AVS MV Register %d) =  0x%x\n", u4Addr, u4Addr/4, u4Val );
    }
    else
    {
        vWriteReg(AVC_MV_REG_OFFSET1 + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
        vVDecSimDump(u4VDecID, AVC_MV_REG_OFFSET1, u4Addr, u4Val);
#endif
    }
}

UINT32 u4VDecReadAVSMV(UINT32 u4VDecID, UINT32 u4Addr)
{
#if (CONFIG_DRV_FPGA_BOARD)
     u4VDecID = 0;
#endif

    if (u4VDecID == 0)
    {
        return (u4ReadReg(AVC_MV_REG_OFFSET0 + u4Addr));
    }
    else
    {
        return (u4ReadReg(AVC_MV_REG_OFFSET1 + u4Addr));
    }
}

void vVDecWriteAVSPP(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#if (CONFIG_DRV_FPGA_BOARD)
     u4VDecID = 0;
#endif

    if (u4VDecID == 0)
    {
        vWriteReg(AVS_PP_REG_OFFSET0 + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
        vVDecSimDump(u4VDecID, AVS_PP_REG_OFFSET0, u4Addr, u4Val);
#endif
        //printk("[VDEC]0x      %x (AVS PP Register %d) =  0x%x\n", u4Addr, u4Addr/4, u4Val );
    }
    else
    {
        vWriteReg(AVS_PP_REG_OFFSET1 + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
        vVDecSimDump(u4VDecID, AVS_PP_REG_OFFSET1, u4Addr, u4Val);
#endif
    }
}

UINT32 u4VDecReadAVSPP(UINT32 u4VDecID, UINT32 u4Addr)
{
#if (CONFIG_DRV_FPGA_BOARD)
     u4VDecID = 0;
#endif

    if (u4VDecID == 0)
    {
        return (u4ReadReg(AVC_MV_REG_OFFSET0 + u4Addr));
    }
    else
    {
        return (u4ReadReg(AVC_MV_REG_OFFSET1 + u4Addr));
    }
}

// *********************************************************************
// Function : UINT32 dVLDGetBitS(UINT32 u4BSID,UINT32 u4VDecID,UINT32 dShiftBit)
// Description : Get Bitstream from VLD barrel shifter
// Parameter : dShiftBit: Bits to shift (0-32)
// Return    : barrel shifter
// *********************************************************************
UINT32 u4VDecAVSVLDGetBitS(UINT32 u4BSID,UINT32 u4VDecID,UINT32 dShiftBit)
{
    UINT32 u4RegVal;
  
    if (u4BSID == 0)
    {
        u4RegVal = u4VDecReadAVSVLD(u4VDecID, RO_AVLD_BARL  + (dShiftBit << 2));
    }
    else
    {
        u4RegVal = u4VDecReadAVSVLD(u4VDecID, RO_AVLD_2ND_BARL  + (dShiftBit << 2));
    }
    return (u4RegVal);
}

// *********************************************************************
// Function : UINT32 u4VDecAVSVLDShiftBits(UINT32 u4BSID, UINT32 u4VDecID)
// Description : Get AVSVLD shift bits %64
// Parameter : None
// Return    : VLD Sum
// *********************************************************************
UINT32 u4VDecAVSVLDShiftBits(UINT32 u4BSID, UINT32 u4VDecID)
{
    if (u4BSID==0)
    {
        return ((u4VDecReadAVSVLD(u4VDecID, RW_AVLD_CTRL) >> 16) & 0x3F);
    }
    else
    {
        return (u4VDecReadAVSVLD(u4VDecID, RW_AVLD_2ND_CTRL) & 0x3F);
    }
}


// *********************************************************************
// Function : BOOL fgH264VLDInitBarrelShifter1(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4VLDRdPtr, UINT32 u4VLDWrPtr)
// Description : Init HW Barrel Shifter
// Parameter : u4Ptr: Physical DRAM Start Address to fill Barrel Shifter
// Return    : TRUE: Initial Success, Fail: Initial Fail
// *********************************************************************
BOOL fgAVSVLDInitBarrelShifter(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4FIFOSa, UINT32 u4VLDRdPtr, UINT32 u4VLDWrPtr)
{
    UINT32 u4ByteAddr;
    UINT32 u4TgtByteAddr;
    INT32 i;
    BOOL fgFetchOK = FALSE;
    UINT32 u4Cnt = 0;
    
    // prevent initialize barrel fail
    for (i = 0; i < 5; i++)
    {
        u4Cnt = 50000;
        if (u4VDecReadVLD(u4VDecID, RO_VLD_SRAMCTRL) & (1<<15))
        {
            while((!(u4VDecReadVLD(u4VDecID, RO_VLD_SRAMCTRL)&0x1)) && (u4Cnt--));
        }
    
        vVDecWriteVLD(u4VDecID, RW_VLD_RPTR + (u4BSID << 10), u4VLDRdPtr);
        vVDecWriteVLD(u4VDecID, RW_VLD_RPTR + (u4BSID << 10), u4VLDRdPtr);
        vVDecWriteVLD(u4VDecID, WO_VLD_WPTR + (u4BSID << 10), u4VLDWrPtr);
        vVDecWriteVLD(u4VDecID, WO_VLD_SRST , 1 << 8);
        vVDecWriteVLD(u4VDecID, WO_VLD_SRST , 0);

        // start to fetch data
        vVDecWriteVLD(u4VDecID, RW_VLD_PROC + (u4BSID << 10), VLD_INIFET);
        if (fgVDecWaitVldFetchOk(u4BSID, u4VDecID))
        {
            fgFetchOK = TRUE;
            break;
        }
    }

    if (!fgFetchOK)
    {
        return (FALSE);
    }
  
    vVDecWriteVLD(u4VDecID, RW_VLD_PROC + (u4BSID << 10), VLD_INIBR);

    // move range 0~15 bytes
    u4TgtByteAddr = u4VLDRdPtr & 0xf;
    u4ByteAddr = u4VLDRdPtr & 0xfffffff0;
    while (u4TgtByteAddr)
    {
        u4TgtByteAddr --;
        u4ByteAddr ++;
        u4VDecAVSVLDGetBitS(u4BSID, u4VDecID, 8);
    }  

    // Reset byte counts
    vVDecWriteAVSVLD(u4VDecID, RW_AVS_VLD_CLR_BYTE_COUNT + (u4BSID << 10), 1);
    vVDecWriteAVSVLD(u4VDecID, RW_AVS_VLD_CLR_BYTE_COUNT + (u4BSID << 10), 0);
    
    return (TRUE);
}


// *********************************************************************
// Function : BOOL fgInitH264BarrelShift1(UINT32 u4VDecID, VDEC_INFO_H264_BS_INIT_PRM_T *prH264BSInitPrm)
// Description : Reset VLD2
// Parameter : None
// Return    : None
// *********************************************************************
BOOL fgInitAVSBarrelShift1(UINT32 u4VDecID, VDEC_INFO_AVS_BS_INIT_PRM_T *prAvsBSInitPrm)
{        
    vVDecWriteAVSVLD(u4VDecID, RW_AVS_VLD_CTRL, 0x7); 
    vVDecWriteAVSVLD(u4VDecID, RW_AVS_VLD_MODE_SWITCH, 7);  
    
    vVDecSetVLDVFIFO(0, u4VDecID, PHYSICAL(prAvsBSInitPrm->u4VFifoSa), PHYSICAL((UINT32) prAvsBSInitPrm->u4VFifoEa));
     
    if (!fgAVSVLDInitBarrelShifter(0, u4VDecID, PHYSICAL((UINT32) prAvsBSInitPrm->u4VFifoSa), PHYSICAL((UINT32) prAvsBSInitPrm->u4VLDRdPtr), PHYSICAL(prAvsBSInitPrm->u4VLDWrPtr)))
    {
        return FALSE;
    }
    return TRUE;
}

// *********************************************************************
// Function : BOOL fgInitH264BarrelShift2(UINT32 u4RDPtrAddr)
// Description : Reset VLD2
// Parameter : None
// Return    : None
// *********************************************************************
BOOL fgInitAVSBarrelShift2(UINT32 u4VDecID, VDEC_INFO_AVS_BS_INIT_PRM_T *prAvsBSInitPrm)
{    
    //printk("VDEC: INIT AVS Barrel Shift2\n");
    VDEC_ASSERT(0);
    return TRUE;
}

UINT32 u4VDecReadAVSVldRPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 *pu4Bits, UINT32 u4VFIFOSa)
{
    UINT32 u4DramRptr;
    UINT32 u4SramRptr, u4SramWptr;
    UINT32 u4SramDataSz;
    UINT32 u4ByteAddr;
    UINT32 u4Cnt, u4RegVal, u4SramCtr;

    // HW issue, wait for read pointer stable
    u4Cnt = 50000;
    if (u4VDecReadVLD(u4VDecID, RO_VLD_SRAMCTRL) & (1<<15))
    {
        while((!(u4VDecReadVLD(u4VDecID, RO_VLD_SRAMCTRL)&0x1)) && (u4Cnt--));
    }

    u4RegVal = u4VDecReadVLD(u4VDecID, RO_VLD_VBAR + (u4BSID << 10));

    u4SramRptr = u4RegVal & 0x1F;
    u4SramWptr = (u4RegVal >> 8) & 0x1F;
    u4SramCtr = (u4RegVal >> 24) & 0x3;
    u4DramRptr = u4VDecReadVLD(u4VDecID, RO_VLD_VRPTR + (u4BSID << 10));


    if (u4SramWptr > u4SramRptr)
    {
        u4SramDataSz = u4SramWptr - u4SramRptr;
    }
    else
    {
        u4SramDataSz = 32 - (u4SramRptr - u4SramWptr);
    }

    *pu4Bits = (u4VDecReadAVSVLD(u4VDecID, RW_AVS_VLD_CTRL + (u4BSID << 10)) >> 16) & 0x3f;

    u4ByteAddr = u4DramRptr - (u4SramDataSz * 16) + (u4SramCtr * 4) - (5 * 4) + (*pu4Bits / 8);

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


/*
 UINT32 u4VDecReadAVSVldRPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 *pu4Bits, UINT32 u4VFIFOSa)
{
    UINT32 u4DramRptr;
    UINT32 u4SramRptr, u4SramWptr;
    UINT32 u4SramDataSz;
    UINT32 u4ByteAddr;
      #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    UINT32 u4Cnt;
    #endif
  
    // HW issue, wait for read pointer stable
       #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
      u4Cnt = 50000;
      if (u4VDecReadVLD(u4VDecID, RO_VLD_SRAMCTRL) & (1<<15))
      	{
      	while((!(u4VDecReadVLD(u4VDecID, RO_VLD_SRAMCTRL)&0x1)) && (u4Cnt--));
      	}
      #else
    while (!(u4VDecReadVLD(u4VDecID, RO_VLD_SRAMCTRL)&1));
      #endif

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
        //u4SramDataSz = 64 - (u4SramRptr - u4SramWptr);
        u4SramDataSz = 32 - (u4SramRptr - u4SramWptr);
    }

    *pu4Bits = (u4VDecReadAVSVLD(u4VDecID, RW_AVS_VLD_CTRL + (u4BSID << 10)) >> 16) & 0x3f;
      
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
*/

