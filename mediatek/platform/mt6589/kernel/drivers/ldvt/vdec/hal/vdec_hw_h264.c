
#include "vdec_hw_common.h"
#include "../include/vdec_info_h264.h"
#include "vdec_hw_h264.h"
//#include "x_hal_ic.h"

#if CONFIG_DRV_VERIFY_SUPPORT
#include "../verify/vdec_verify_general.h"
#include "../verify/vdec_info_verify.h"

#if (!CONFIG_DRV_LINUX)
#include <string.h>
#include <stdio.h>
#endif

extern char gpfH264LogFileBuffer[4096];
extern int gfpH264log;
extern unsigned int gH264logbufferOffset;
int vdecwriteFile(int fp,char *buf,int writelen);

#define OUTPUT_H264LOG_URAT 0

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


extern void vVDecOutputDebugString(const CHAR * format, ...);
extern BOOL fgWrMsg2PC(void* pvAddr, UINT32 u4Size, UINT32 u4Mode, VDEC_INFO_VERIFY_FILE_INFO_T *pFILE_INFO);
extern void vVDecOutputDebugString(const CHAR * format, ...);
#endif

void vVDecWriteAVCVLD(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
     u4VDecID = 0;
#endif

    if (u4VDecID == 0)
    {
        vWriteReg(AVC_VLD_REG_OFFSET0 + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
        vVDecSimDumpW(u4VDecID, AVC_VLD_REG_OFFSET0, u4Addr, u4Val);
        //vVDecSimDump(u4VDecID, AVC_VLD_REG_OFFSET0, u4Addr, u4Val);
#endif
    }
    else
    {
        vWriteReg(AVC_VLD_REG_OFFSET1 + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
        vVDecSimDumpW(u4VDecID, AVC_VLD_REG_OFFSET1, u4Addr, u4Val);
        //vVDecSimDump(u4VDecID, AVC_VLD_REG_OFFSET1, u4Addr, u4Val);
#endif
    }
}

UINT32 u4VDecReadAVCVLD(UINT32 u4VDecID, UINT32 u4Addr)
{
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
     u4VDecID = 0;
#endif
    UINT32 u4Val;

    if (u4VDecID == 0)
    {
        #ifdef VDEC_SIM_DUMP
        u4Val = (u4ReadReg(AVC_VLD_REG_OFFSET0 + u4Addr));
        vVDecSimDumpR(u4VDecID, AVC_VLD_REG_OFFSET0, u4Addr, u4Val);
        return(u4Val); 
        #endif
        return(u4ReadReg(AVC_VLD_REG_OFFSET0 + u4Addr)); 
    }
    else
    {
        #ifdef VDEC_SIM_DUMP
        u4Val = (u4ReadReg(AVC_VLD_REG_OFFSET1 + u4Addr));
        vVDecSimDumpR(u4VDecID, AVC_VLD_REG_OFFSET1, u4Addr, u4Val);
        return(u4Val); 
        #endif
        return (u4ReadReg(AVC_VLD_REG_OFFSET1 + u4Addr));
    }
}

void vVDecWriteAVCMV(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
     u4VDecID = 0;
#endif

    if (u4VDecID == 0)
    {
        vWriteReg(AVC_MV_REG_OFFSET0 + u4Addr, u4Val);
        vVDecSimDumpW(u4VDecID, AVC_MV_REG_OFFSET0, u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
        //vVDecSimDump(u4VDecID, AVC_MV_REG_OFFSET0, u4Addr, u4Val);
#endif
    }
    else
    {
        vWriteReg(AVC_MV_REG_OFFSET1 + u4Addr, u4Val);
        vVDecSimDumpW(u4VDecID, AVC_MV_REG_OFFSET1, u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
        //vVDecSimDump(u4VDecID, AVC_MV_REG_OFFSET1, u4Addr, u4Val);
#endif
    }
}

UINT32 u4VDecReadAVCMV(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val;
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
     u4VDecID = 0;
#endif

    if (u4VDecID == 0)
    {
        u4Val = (u4ReadReg(AVC_MV_REG_OFFSET0 + u4Addr));
        vVDecSimDumpR(u4VDecID, AVC_MV_REG_OFFSET0, u4Addr, u4Val);
        return (u4ReadReg(AVC_MV_REG_OFFSET0 + u4Addr)); 
    }
    else
    {
        u4Val = (u4ReadReg(AVC_MV_REG_OFFSET1 + u4Addr));
        vVDecSimDumpR(u4VDecID, AVC_MV_REG_OFFSET1, u4Addr, u4Val);
        return u4Val;
    }
}

void vVDecWriteAVCFG(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
     u4VDecID = 0;
#endif

    if (u4VDecID == 0)
    {
        vWriteReg(AVC_FG_REG_OFFSET0 + u4Addr, u4Val);
    }
    else
    {
        vWriteReg(AVC_FG_REG_OFFSET1 + u4Addr, u4Val);
    }
}

UINT32 u4VDecReadAVCFG(UINT32 u4VDecID, UINT32 u4Addr)
{
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
     u4VDecID = 0;
#endif

    if (u4VDecID == 0)
    {
        return (u4ReadReg(AVC_FG_REG_OFFSET0 + u4Addr)); 
    }
    else
    {
        return (u4ReadReg(AVC_FG_REG_OFFSET1 + u4Addr));
    }
}

// *********************************************************************
// Function : UINT32 dVLDGetBitS(UINT32 u4BSID,UINT32 u4VDecID,UINT32 dShiftBit)
// Description : Get Bitstream from VLD barrel shifter
// Parameter : dShiftBit: Bits to shift (0-32)
// Return    : barrel shifter
// *********************************************************************
UINT32 u4VDecAVCVLDGetBitS(UINT32 u4BSID,UINT32 u4VDecID,UINT32 dShiftBit)
{

    UINT32 u4RegVal;
    #if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
         u4VDecID = 0;
    #endif
    if (u4BSID == 0)
    {
        u4RegVal = u4VDecReadAVCVLD(u4VDecID, RO_AVLD_BARL  + (dShiftBit << 2));
    }
    else
    {
        u4RegVal = u4VDecReadAVCVLD(u4VDecID, RO_AVLD_2ND_BARL  + (dShiftBit << 2));
    }
    return (u4RegVal);
}

// *********************************************************************
// Function : UINT32 u4VDecAVCVLDShiftBits(UINT32 u4BSID, UINT32 u4VDecID)
// Description : Get AVCVLD shift bits %64
// Parameter : None
// Return    : VLD Sum
// *********************************************************************
UINT32 u4VDecAVCVLDShiftBits(UINT32 u4BSID, UINT32 u4VDecID)
{
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
         u4VDecID = 0;
#endif

    if (u4BSID==0)
    {
        return ((u4VDecReadAVCVLD(u4VDecID, RW_AVLD_CTRL) >> 16) & 0x3F);
    }
    else
    {
        return (u4VDecReadAVCVLD(u4VDecID, RW_AVLD_2ND_CTRL) & 0x3F);
    }
}

// *********************************************************************
// Function : void vInitFgtHWSetting(UINT32 u4VDecID, VDEC_INFO_H264_INIT_PRM_T *prH264VDecInitPrm)
// Description : Get VLD Current Dram pointer
// Parameter : None
// Return    : VLD Sum
// *********************************************************************
void vInitFgtHWSetting(UINT32 u4VDecID, VDEC_INFO_H264_INIT_PRM_T *prH264VDecInitPrm)
{
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
     u4VDecID = 0;
#endif

#if CONFIG_DRV_VIRTUAL_ADDR
    vVDecWriteAVCFG(u4VDecID, RW_FGT_SEED_ADDR, (PHYSICAL((UINT32) prH264VDecInitPrm->u4FGSeedbase)) >> 4);
    vVDecWriteAVCFG(u4VDecID, RW_FGT_SEI_ADDR_A, (PHYSICAL((UINT32) prH264VDecInitPrm->u4CompModelValue)) >> 4);  
    vVDecWriteAVCFG(u4VDecID, RW_FGT_DATABASE_ADDR, (PHYSICAL((UINT32) prH264VDecInitPrm->u4FGDatabase)) >> 4);    
#else
    vVDecWriteAVCFG(u4VDecID, RW_FGT_SEED_ADDR, (u4AbsDramANc((UINT32) prH264VDecInitPrm->u4FGSeedbase)) >> 4);
    vVDecWriteAVCFG(u4VDecID, RW_FGT_SEI_ADDR_A, (u4AbsDramANc((UINT32) prH264VDecInitPrm->u4CompModelValue)) >> 4);  
    vVDecWriteAVCFG(u4VDecID, RW_FGT_DATABASE_ADDR, (u4AbsDramANc((UINT32) prH264VDecInitPrm->u4FGDatabase)) >> 4);    
#endif
}

// *********************************************************************
// Function : BOOL fgH264VLDInitBarrelShifter1(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4VLDRdPtr, UINT32 u4VLDWrPtr)
// Description : Init HW Barrel Shifter
// Parameter : u4Ptr: Physical DRAM Start Address to fill Barrel Shifter
// Return    : TRUE: Initial Success, Fail: Initial Fail
// *********************************************************************
BOOL fgH264VLDInitBarrelShifter(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4FIFOSa, UINT32 u4VLDRdPtr, UINT32 u4VLDWrPtr)
{
    UINT32 u4ByteAddr;
    UINT32 u4TgtByteAddr;
    UINT32 u4BSREMOVE03;
    //UINT32 u4Bits;
    INT32 i;
    BOOL fgFetchOK = FALSE;
    UINT32 u4Reg;
    #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    UINT32 u4Cnt;
    #endif
    // prevent initialize barrel fail
    for (i = 0; i < 5; i++)
    {
      #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
      u4Cnt = 50000;
      if (u4VDecReadVLD(u4VDecID, RO_VLD_SRAMCTRL) & (1<<15))
      	{
      	while((!(u4VDecReadVLD(u4VDecID, RO_VLD_SRAMCTRL)&0x1)) && (u4Cnt--));
      	}
      #endif
        vVDecWriteVLD(u4VDecID, WO_VLD_WPTR, u4VDecReadVLD(u4VDecID, WO_VLD_WPTR) | VLD_CLEAR_PROCESS_EN);
        vVDecWriteVLD(u4VDecID, RW_VLD_RPTR + (u4BSID << 10), u4VLDRdPtr);
        vVDecWriteVLD(u4VDecID, RW_VLD_RPTR + (u4BSID << 10), u4VLDRdPtr);
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530) 
  vVDecWriteVLD(u4VDecID, WO_VLD_WPTR + (u4BSID << 10), u4VLDWrPtr);
#if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8580) 
  vVDecWriteVLD(u4VDecID, RW_VLD_ASYNC + (u4BSID << 10), u4VDecReadVLD(u4VDecID, RW_VLD_ASYNC) | VLD_WR_ENABLE);
#endif
#else
        vVDecWriteVLD(u4VDecID, WO_VLD_WPTR + (u4BSID << 10),((u4VLDWrPtr << 4) | 0x2));
#endif
      
        if (u4BSID == 0)
        {
            vVDecWriteAVCVLD(u4VDecID, RW_AVLD_RESET_SUM, AVLD_RESET_SUM_ON);
        }
        
        #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
        vVDecWriteVLD(u4VDecID, WO_VLD_SRST , 1 << 8);
        vVDecWriteVLD(u4VDecID, WO_VLD_SRST , 0);
        #endif      

        #if (CONFIG_DRV_FPGA_BOARD)
        vVDecWriteVLD(u4VDecID,RW_VLD_BS_SPEEDUP,0);
        #endif
        
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
    
    if (u4BSID == 0)
    {
      // HW workaround
      // can not reset sum off until barrel shifter finish initialization
 #if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8580)
 #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8550)
 
       #ifdef MEM_PAUSE_SUPPORT  
        while(0x01000001 != (u4VDecReadVLD(u4VDecID, RO_VLD_VBAR + (u4BSID << 10)) & 0x03000003));
      #else
        while(0x00000001 != (u4VDecReadVLD(u4VDecID, RO_VLD_VBAR + (u4BSID << 10)) & 0x03000003));
      #endif
      
  #else 
  
    #if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8530)
    #else
    
      #ifdef MEM_PAUSE_SUPPORT  
        while(5 != (u4VDecReadVLD(u4VDecID, RO_VLD_VBAR + (u4BSID << 10)) & 0x3f));
      #else
        while(4 != (u4VDecReadVLD(u4VDecID, RO_VLD_VBAR + (u4BSID << 10)) & 0x3f));
      #endif
      
    #endif
    
  #endif
    #endif
        vVDecWriteAVCVLD(u4VDecID, RW_AVLD_RESET_SUM, AVLD_RESET_SUM_OFF);  
    }
  
    //while (u4VDecReadH264VldRPtr(u4BSID, u4VDecID, &u4Bits, u4FIFOSa) < (u4VLDRdPtr - u4FIFOSa))
    //{
    //    u4VDecAVCVLDGetBitS(u4BSID, u4VDecID, 8);
    //}

    // move range 0~15 bytes
    u4TgtByteAddr = u4VLDRdPtr & 0xf;
    u4ByteAddr = u4VLDRdPtr & 0xfffffff0;
    #if 1
    i = 0;
    while (u4TgtByteAddr)
    {
        u4TgtByteAddr --;
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8520)
        if(((((UINT32 *)(VIRTUAL((UINT32) u4ByteAddr)))[0] & 0xFFFFFF) == 0x030000) &&
            ((((UINT32 *)(VIRTUAL((UINT32) u4ByteAddr)))[0]>>24) <= 0x03)
            && u4TgtByteAddr)
#else
        u4BSREMOVE03 = u4VDecReadAVCVLD(u4VDecID, RW_AVLD_RM03R);
        //printk("@@@@ >>> u4TgtByteAddr %x \n",  u4BSREMOVE03);
        //if( ((u4VDecReadAVCVLD(u4VDecID, RW_AVLD_RM03R)&RO_ALVD_FIND_03)) && u4TgtByteAddr)
        if((u4BSREMOVE03&RO_ALVD_FIND_03)&& u4TgtByteAddr)
#endif
        {
        	if((i > 2) || (u4TgtByteAddr > 1) )
                {
                    u4TgtByteAddr --;
                    i++;
                    //printk("@@@@ 1 >>> u4TgtByteAddr %d, i %d\n", u4TgtByteAddr, i);
        	}
        }
        u4ByteAddr ++;
	i++;
        u4VDecAVCVLDGetBitS(u4BSID, u4VDecID, 8);
        //printk("@@@@ 2 >>> u4TgtByteAddr %d, i %d\n", u4TgtByteAddr, i);
    }
    #else  //for 6589
    #if 1   
    while (u4TgtByteAddr)
    {
        u4TgtByteAddr --;
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8520)
        if(((((UINT32 *)(VIRTUAL((UINT32) u4ByteAddr)))[0] & 0xFFFFFF) == 0x030000) &&
            ((((UINT32 *)(VIRTUAL((UINT32) u4ByteAddr)))[0]>>24) <= 0x03)
            && u4TgtByteAddr)
#else
        u4BSREMOVE03 = u4VDecReadAVCVLD(u4VDecID, RW_AVLD_RM03R);
        //printk("@@@@ >>> u4TgtByteAddr %x \n",  u4BSREMOVE03);
        //if( ((u4VDecReadAVCVLD(u4VDecID, RW_AVLD_RM03R)&RO_ALVD_FIND_03)) && u4TgtByteAddr)
        if((u4BSREMOVE03&RO_ALVD_FIND_03)&& u4TgtByteAddr)
#endif
        {
                u4TgtByteAddr --;
                //printk("@@@@ 1 >>> u4TgtByteAddr %d\n", u4TgtByteAddr);
            }
        }
        u4ByteAddr ++;
        u4VDecAVCVLDGetBitS(u4BSID, u4VDecID, 8);
        
        //printk("@@@@ 2 >>> u4TgtByteAddr %d\n", u4TgtByteAddr);
    }
    #endif
    #endif

    u4Reg = u4VDecReadAVCVLD(u4VDecID, RO_AVLD_BARL);
    DBG_H264_PRINTF("{H264 DEC >> } u4InstID = 0x%x, Input Window: 0x%08x\n", u4VDecID, u4Reg);
    return (TRUE);
}


// *********************************************************************
// Function : BOOL fgInitH264BarrelShift1(UINT32 u4VDecID, VDEC_INFO_H264_BS_INIT_PRM_T *prH264BSInitPrm)
// Description : Reset VLD2
// Parameter : None
// Return    : None
// *********************************************************************
BOOL fgInitH264BarrelShift1(UINT32 u4VDecID, VDEC_INFO_H264_BS_INIT_PRM_T *prH264BSInitPrm)
{
    vVDecWriteVLD(u4VDecID,RW_VLD_RDY_SWTICH, READY_TO_RISC_1);
    
    //vVDecWriteAVCVLD(u4VDecID, RW_AVLD_CTRL, AVC_EN | AVC_RDY_WITH_CNT | AVC_RDY_CNT_THD | AVLD_MEM_PAUSE_MOD_EN | AVC_DEC_CYCLE_EN | AVC_SUM6_APPEND_INV |AVC_NOT_CHK_DATA_VALID);
    //vVDecWriteAVCVLD(u4VDecID, RW_AVLD_CTRL, AVC_EN | AVC_RDY_WITH_CNT | AVC_RDY_CNT_THD | AVLD_MEM_PAUSE_MOD_EN | AVC_SUM6_APPEND_INV |AVC_NOT_CHK_DATA_VALID);
#if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8560)
    vVDecWriteAVCVLD(u4VDecID, RW_AVLD_CTRL, AVC_EN | AVC_RDY_WITH_CNT | AVC_RDY_CNT_THD | AVLD_MEM_PAUSE_MOD_EN  
    | AVC_SUM6_APPEND_INV |AVC_NOT_CHK_DATA_VALID | AVC_RBSP_CHK_INV | AVC_READ_FLAG_CHK_INV |AVC_ERR_BYPASS /*| AVC_ERR_CONCEALMENT*/
#if (CONFIG_DRV_VERIFY_SUPPORT)    

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
#else
    | AVC_NON_SPEC_SWITCH
#endif    
#ifdef NO_COMPARE
    | AVC_DEC_CYCLE_EN
#endif    
    | AVC_ERR_CONCEALMENT
    
#endif    
);
#else
    vVDecWriteAVCVLD(u4VDecID, RW_AVLD_CTRL, AVC_EN | AVC_RDY_WITH_CNT | AVC_RDY_CNT_THD | AVLD_MEM_PAUSE_MOD_EN  
    | AVC_SUM6_APPEND_INV |AVC_NOT_CHK_DATA_VALID | AVC_RBSP_CHK_INV | AVC_ERR_BYPASS /*| AVC_ERR_CONCEALMENT*/
#if (CONFIG_DRV_VERIFY_SUPPORT)    

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
#else
    | AVC_NON_SPEC_SWITCH
#endif    
#ifdef NO_COMPARE
#if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8580)
    | AVC_DEC_CYCLE_EN
#endif    
#endif    
    | AVC_ERR_CONCEALMENT
    
#endif    
);
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
vVDecWriteVLDTOP(u4VDecID, RW_VLD_TOP_TIMEOUT_SW, u4VDecReadVLDTOP(u4VDecID, RW_VLD_TOP_TIMEOUT_SW)|VLD_TOP_DEC_CYCLE_EN);
#endif
#endif
    vVDecWriteMC(u4VDecID, RW_MC_OPBUF, 6);  
    
    vVDecSetVLDVFIFO(0, u4VDecID, PHYSICAL(prH264BSInitPrm->u4VFifoSa), PHYSICAL((UINT32) prH264BSInitPrm->u4VFifoEa));
#if CONFIG_DRV_VIRTUAL_ADDR
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
  vVDecWriteVLDTOP(u4VDecID, RW_VLD_TOP_PRED_ADDR, (prH264BSInitPrm->u4PredSa ? PHYSICAL((UINT32) prH264BSInitPrm->u4PredSa) : 0));    
#else
vVDecWriteAVCVLD(u4VDecID, RW_AVLD_PRED_ADDR, (prH264BSInitPrm->u4PredSa ? PHYSICAL((UINT32) prH264BSInitPrm->u4PredSa) : 0));    
#endif
#else    
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
  // Cheng-Jung 2012/08/30 Note: for MT6582, VLD_TOP_10 is for VLD Wrapper, not Predition buffer
#if MT6582_L2_EMULATION == 1
  vVDecWriteVLDTOP(u4VDecID, RW_VLD_TOP_PRED_ADDR, u4AbsDramANc((UINT32) prH264BSInitPrm->u4PredSa));
#elif MT6582_L2_EMULATION == 2
  vVDecWriteVLDTOP(u4VDecID, RW_VLD_TOP_PRED_ADDR, 0x00200000);
#else
  vVDecWriteVLDTOP(u4VDecID, RW_VLD_TOP_PRED_ADDR, u4AbsDramANc((UINT32) prH264BSInitPrm->u4PredSa));
#endif
#else    
    vVDecWriteAVCVLD(u4VDecID, RW_AVLD_PRED_ADDR, u4AbsDramANc((UINT32) prH264BSInitPrm->u4PredSa));    
#endif
#endif
  
    // Reset AVC VLD Sum
    vVDecWriteAVCVLD(u4VDecID, RW_AVLD_RESET_SUM, AVLD_RESET_SUM_ON);
    vVDecWriteAVCVLD(u4VDecID, RW_AVLD_RESET_SUM, AVLD_RESET_SUM_OFF);  
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
  // CB test
  //vVDecWriteVLD(u4VDecID, RW_VLD_ASYNC, u4VDecReadVLD(u4VDecID, RW_VLD_ASYNC) | (VLD_NEW_DRAM_DISABLE));  
#endif
    
    if (!fgH264VLDInitBarrelShifter(0, u4VDecID, PHYSICAL((UINT32) prH264BSInitPrm->u4VFifoSa), PHYSICAL((UINT32) prH264BSInitPrm->u4VLDRdPtr), PHYSICAL(prH264BSInitPrm->u4VLDWrPtr)))
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
BOOL fgInitH264BarrelShift2(UINT32 u4VDecID, VDEC_INFO_H264_BS_INIT_PRM_T *prH264BSInitPrm)
{
    // reset barrel shifter 2
    while(!(u4VDecReadVLD(u4VDecID, RO_VLD_SRAMCTRL | (1<<10))&0x10000));
    vVDecWriteVLD(u4VDecID, WO_VLD_SRST , 1 << 8);
    vVDecWriteVLD(u4VDecID, WO_VLD_SRST , 0);
    
    vVDecWriteVLD(u4VDecID,RW_VLD_2ND_RDY_SWTICH, READY_TO_RISC_1);
  
    // temporarily workaround, will be fixed by ECO
    vVDecWriteAVCVLD(u4VDecID, RW_AVLD_CTRL, 1 << 30);
    vVDecWriteAVCVLD(u4VDecID,RW_AVLD_2ND_BARL_CTRL, AVLD_2ND_BARL_EN);    
    vVDecSetVLDVFIFO(1, u4VDecID, PHYSICAL(prH264BSInitPrm->u4VFifoSa), PHYSICAL((UINT32) prH264BSInitPrm->u4VFifoEa));
    
    if (!fgH264VLDInitBarrelShifter(1, u4VDecID, PHYSICAL((UINT32) prH264BSInitPrm->u4VFifoSa), PHYSICAL((UINT32) prH264BSInitPrm->u4VLDRdPtr), PHYSICAL(prH264BSInitPrm->u4VLDWrPtr)))
    {
        return FALSE;
    }
    return TRUE;
}

#if 1
UINT32 u4VDecReadH264VldRPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 *pu4Bits, UINT32 u4VFIFOSa)
{
    UINT32 u4DramRptr;
    UINT32 u4SramRptr, u4SramWptr;
    UINT32 u4SramDataSz;
    UINT32 u4ByteAddr;
    UINT32 u4RegVal, u4SramCtr;
	  UINT32 vb_sram_ra, vb_sram_wa, seg_rcnt;
    UINT32 u4Cnt = 0;
    UINT32 u4BsBufLen = 0;

    // HW issue, wait for read pointer stable
    u4Cnt = 50000;
    //if (u4VDecReadVLD(u4VDecID, RO_VLD_SRAMCTRL) & (1<<15))
    
    #ifdef VDEC_SIM_DUMP
    printk("if(`VDEC_PROCESS_FLAG == 1) wait(`VDEC_BITS_PROC_NOP == 1);\n");
    #endif
    
    if (u4VDecReadVLD(u4VDecID, RO_VLD_SRAMCTRL) & (PROCESS_FLAG))
    {
       while((!(u4VDecReadVLD(u4VDecID, RO_VLD_SRAMCTRL)&0x1)) && (u4Cnt--));
    }

    u4RegVal = u4VDecReadVLD(u4VDecID, RO_VLD_VBAR + (u4BSID << 10));
    vb_sram_ra = u4RegVal & 0x1F;
    vb_sram_wa = (u4RegVal >> 8) & 0x1F;
	  seg_rcnt = (u4RegVal >> 24) & 0x3;

	  u4SramRptr = vb_sram_ra;
	  u4SramWptr = vb_sram_wa;
	  u4SramCtr = seg_rcnt;
	  u4DramRptr = u4VDecReadVLD(u4VDecID, RO_VLD_VRPTR + (u4BSID << 10));

    if (u4SramWptr > u4SramRptr)
    {
        u4SramDataSz = u4SramWptr - u4SramRptr;
    }
    else
    {
	    u4SramDataSz = 32 - (u4SramRptr - u4SramWptr);
    }
  
    //*pu4Bits = u4VDecReadVLD(u4VDecID, RW_VLD_BITCOUNT + (u4BSID << 10)) & 0x3f;
    *pu4Bits = u4VDecAVCVLDShiftBits(u4BSID,u4VDecID);

    #ifdef VDEC_SIM_DUMP
	  printk("<vdec> ReadH264VldRPtr, dRptr:0x%08X, sra:0x%08X, swa:0x%08X, scnt:0x%08X, sum:0x%08X\n", 
	    u4DramRptr, vb_sram_ra, vb_sram_wa, seg_rcnt, *pu4Bits);
    #endif

    if (u4VDecReadVLD(u4VDecID, 4*59) & (0x1 << 28))
    {    
        u4BsBufLen = 7 * 4;
    }
    else // old case
    {
        u4BsBufLen = 6 * 4;
    }    

    u4ByteAddr = u4DramRptr - u4SramDataSz * 16 + u4SramCtr * 4 - u4BsBufLen + *pu4Bits/8;  
    
    *pu4Bits &= 0x7;

    if (u4ByteAddr < u4VFIFOSa)
    {
        u4ByteAddr = u4ByteAddr  
                     + ((u4VDecReadVLD(u4VDecID, RW_VLD_VEND + (u4BSID << 10)) << 6) - ((UINT32)u4VFIFOSa))
                     - u4VFIFOSa;
    }
    else
    {
        u4ByteAddr -= ((UINT32)u4VFIFOSa);
    }
    
    #ifdef VDEC_SIM_DUMP
	  //printk("<vdec> ReadH264VldRPtr, RdPtr=0x%08X (%u) @(%s, %d)\n", u4ByteAddr, u4ByteAddr);
    #endif

    return (u4ByteAddr);
}
#else
UINT32 u4VDecReadH264VldRPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 *pu4Bits, UINT32 u4VFIFOSa)
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
#elif (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
    while (!(u4VDecReadVLD(u4VDecID, RO_VLD_SRAMCTRL + (u4BSID << 10))&0x1));
#else
    while (!(u4VDecReadVLD(u4VDecID, RO_VLD_SRAMCTRL + (u4BSID << 10))&0x10000));
#endif

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
  u4DramRptr = u4VDecReadVLD(u4VDecID, RO_VLD_VRPTR + (u4BSID << 10));
//  u4SramRptr = (u4VDecReadVLD(u4VDecID, RO_VLD_VBAR + (u4BSID << 10)) & 0xf) * 4; //count in 128bits
  u4SramRptr = ((u4VDecReadVLD(u4VDecID, RO_VLD_VBAR + (u4BSID << 10))) & 0xf) * 4 +
                     (((u4VDecReadVLD(u4VDecID, RO_VLD_VBAR + (u4BSID << 10))>>24)) & 0x3); //count in 128bits
  //u4SramRptrIn32Bits = 4 - ((u4VDecReadVLD(u4VDecID, RO_VLD_VBAR + (u4BSID << 10))>>24) & 0x3); //count in 32bits
  u4SramWptr = (((u4VDecReadVLD(u4VDecID, RO_VLD_VBAR + (u4BSID << 10))>>8)) & 0xf) *4;
  if(u4SramWptr > u4SramRptr)
  {
    u4SramDataSz = u4SramWptr - u4SramRptr;  // 128bits
  }
  else
  {
    u4SramDataSz = 64 - (u4SramRptr - u4SramWptr);
  }

  //*pu4Bits = u4VDecReadVLD(u4VDecID, RO_VLD_SUM)& 0x3f; 
  *pu4Bits = u4VDecAVCVLDShiftBits(u4BSID, u4VDecID);
  
#ifdef MEM_PAUSE_SUPPORT  
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
  u4ByteAddr = u4DramRptr - ((u4SramDataSz + 6) * 4) + ((*pu4Bits) / 8);
#else
  u4ByteAddr = u4DramRptr - ((u4SramDataSz + 5) * 4) + ((*pu4Bits) / 8);
#endif
#else
  u4ByteAddr = u4DramRptr - (u4SramDataSz + 4) * 4 + ((*pu4Bits) / 8);
#endif
  
  if (u4ByteAddr < u4VFIFOSa)  
  {      
    u4ByteAddr = u4ByteAddr +  
                ((u4VDecReadVLD(u4VDecID, RW_VLD_VEND + (u4BSID << 10)) << 6)- ((UINT32)u4VFIFOSa))                        
                - u4VFIFOSa;  
  }  
  else
  {
    u4ByteAddr -= ((UINT32)u4VFIFOSa);
  }
  *pu4Bits &= 0x7;
#else    
    u4DramRptr = u4VDecReadVLD(u4VDecID, RO_VLD_VRPTR + (u4BSID << 10));
    u4SramRptr = (u4VDecReadVLD(u4VDecID, RO_VLD_VBAR + (u4BSID << 10)) & 0x3f);
    u4SramWptr = ((u4VDecReadVLD(u4VDecID, RO_VLD_VBAR + (u4BSID << 10)) >> 16) & 0x3f);
    if (u4SramWptr > u4SramRptr)
    {
        u4SramDataSz = u4SramWptr - u4SramRptr;
    }
    else
    {
        u4SramDataSz = 64 - (u4SramRptr - u4SramWptr);
    }
  
    //*pu4Bits = u4VDecReadVLD(u4VDecID, RO_VLD_SUM)& 0x3f; 
    *pu4Bits = u4VDecAVCVLDShiftBits(u4BSID, u4VDecID);
    
  #ifdef MEM_PAUSE_SUPPORT  
    u4ByteAddr = u4DramRptr - (u4SramDataSz + 5) * 4 + ((*pu4Bits) / 8);
  #else
    u4ByteAddr = u4DramRptr - (u4SramDataSz + 4) * 4 + ((*pu4Bits) / 8);
  #endif
  
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
    *pu4Bits &= 0x7;
#endif
  
    return (u4ByteAddr);
  
}
#endif

#if 0
UINT32 u4VDecReadPP(UINT32 u4VDecID, UINT32 u4Addr)
{
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
     u4VDecID = 0;
#endif

    if (u4VDecID == 0)
    {
        return (u4ReadReg(RM_VDEC_PP_BASE + u4Addr));
    }
    else
    {
        return (u4ReadReg(RM_VDEC_PP_BASE + u4Addr));
    }
}
#else
void vVDecWritePP(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#if (CONFIG_DRV_FPGA_BOARD)
     u4VDecID = 0;
#endif

    if (u4VDecID == 0)
    {
        vWriteReg(RM_VDEC_PP_BASE + u4Addr, u4Val);
        vVDecSimDumpW(u4VDecID, RM_VDEC_PP_BASE, u4Addr, u4Val);
    }
    else
    {
        vWriteReg(RM_VDEC_PP_BASE + u4Addr, u4Val);
        vVDecSimDumpW(u4VDecID, RM_VDEC_PP_BASE, u4Addr, u4Val);
    }
}


UINT32 u4VDecReadPP(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val;
#if (CONFIG_DRV_FPGA_BOARD)
     u4VDecID = 0;
#endif    
    if (u4VDecID == 0)
    {
        u4Val = u4ReadReg(RM_VDEC_PP_BASE + u4Addr);
        vVDecSimDumpR(u4VDecID, RM_VDEC_PP_BASE, u4Addr, u4Val);
        return u4Val;
    }
    else
    {
        u4Val = u4ReadReg(RM_VDEC_PP_BASE + u4Addr);
        vVDecSimDumpR(u4VDecID, RM_VDEC_PP_BASE, u4Addr, u4Val);
        return u4Val;
    }
}
#endif

