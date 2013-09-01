
#include "vdec_hw_common.h"
//#include "x_hal_ic.h"
//#include "x_debug.h"
//#include "x_ckgen.h"
//#include <mach/cache_operation.h>

#include "../include/drv_common.h"
#include <linux/delay.h>
extern UINT32 _u4DumpRegPicNum[2];
extern UINT32 _u4FileCnt[2];

#define _LOG_FANTASIA_1_ 0

extern char gpfH264LogFileBuffer[4096];
extern int gfpH264log;
extern unsigned int gH264logbufferOffset;
int vdecwriteFile(int fp,char *buf,int writelen);

#define OUTPUT_H264LOG_URAT 0

#define DBG_H264_PRINTF printk
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



#if CONFIG_DRV_VERIFY_SUPPORT
#include "../verify/vdec_verify_general.h"
#include "../verify/vdec_info_verify.h"

#if (!CONFIG_DRV_LINUX)
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#endif
#include <linux/delay.h>

extern int rand(void);

extern BOOL fgWrMsg2PCEx(void* pvAddr, UINT32 u4Size, UINT32 u4Mode, VDEC_INFO_VERIFY_FILE_INFO_T *pFILE_INFO, const char *szFunction, INT32 i4Line);
#define  fgWrMsg2PC(pvAddr, u4Size, u4Mode, pFILE_INFO)  fgWrMsg2PCEx(pvAddr, u4Size, u4Mode, pFILE_INFO, __FUNCTION__, __LINE__)
extern void vVDecOutputDebugString(const CHAR * format, ...);
#endif

//#define REG_LOG_NEW //Cheng-Jung 20120305 may not see the definition in vdec_info_common.h, if there is a problem enabling, try uncomment this line
#ifdef REG_LOG_NEW
extern UCHAR *_pucRegisterLog[2];
extern UINT32 _u4RegisterLogLen[2];
extern BOOL _fgRegLogConsole[2];
#endif

void vVDecWriteMISC(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
    vWriteReg(u4Addr, u4Val);
}

UINT32 u4VDecReadMISC(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val;

    u4Val = u4ReadReg(u4Addr);
    return u4Val;
}

void vVDecWriteVLD(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif

    if (u4VDecID == 0)
    {
        vWriteReg(VLD_REG_OFFSET0 + u4Addr, u4Val);
        vVDecSimDumpW(u4VDecID, VLD_REG_OFFSET0, u4Addr, u4Val);
    }
    else
    {
        vWriteReg(VLD_REG_OFFSET1 + u4Addr, u4Val);
        vVDecSimDumpW(u4VDecID, VLD_REG_OFFSET1, u4Addr, u4Val);
    }
}


UINT32 u4VDecReadVLD(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val;
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif

    if (u4VDecID == 0)
    {
        u4Val = u4ReadReg(VLD_REG_OFFSET0 + u4Addr);
        //if (u4Addr != 0xA8)
        {
            vVDecSimDumpR(u4VDecID, VLD_REG_OFFSET0, u4Addr, u4Val);
        }
        return u4Val;
    }
    else
    {
#ifdef VDEC_PIP_WITH_ONE_HW
        printk("PIP_ONE_HW: Wrong HW ID!!!\n");
        VDEC_ASSERT(0);
#endif    
        return (u4ReadReg(VLD_REG_OFFSET1 + u4Addr));
    }
}

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
void vVDecWriteVLDTOP(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560)
    u4VDecID = 0;
#endif

    if (u4VDecID == 0)
    {
        vWriteReg(VLD_TOP_REG_OFFSET0 + u4Addr, u4Val);
        vVDecSimDumpW(u4VDecID, VLD_TOP_REG_OFFSET0, u4Addr, u4Val);
    }
    else
    {
#ifdef VDEC_PIP_WITH_ONE_HW
        printk("PIP_ONE_HW: Wrong HW ID!!!\n");
        VDEC_ASSERT(0);
#endif
        vWriteReg(VLD_TOP_REG_OFFSET1 + u4Addr, u4Val);
        vVDecSimDumpW(u4VDecID, VLD_TOP_REG_OFFSET1, u4Addr, u4Val);
    }
}


UINT32 u4VDecReadVLDTOP(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val;
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560)
    u4VDecID = 0;
#endif

    if (u4VDecID == 0)
    {
        u4Val = u4ReadReg(VLD_TOP_REG_OFFSET0 + u4Addr);
        vVDecSimDumpR(u4VDecID, VLD_TOP_REG_OFFSET0, u4Addr, u4Val);
        return u4Val;
    }
    else
    {
#ifdef VDEC_PIP_WITH_ONE_HW
        printk("PIP_ONE_HW: Wrong HW ID!!!\n");
        VDEC_ASSERT(0);
#endif    
        return (u4ReadReg(VLD_TOP_REG_OFFSET1 + u4Addr));
    }
}
#endif

void vVDecWriteMC(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif

    if (u4VDecID == 0)
    {
        vWriteReg(MC_REG_OFFSET0 + u4Addr, u4Val);
        vVDecSimDumpW(u4VDecID, MC_REG_OFFSET0, u4Addr, u4Val);
    }
    else
    {
#ifdef VDEC_PIP_WITH_ONE_HW
        printk("PIP_ONE_HW: Wrong HW ID!!!\n");
        VDEC_ASSERT(0);
#endif
        vVDecSimDumpW(u4VDecID, MC_REG_OFFSET1, u4Addr, u4Val);
        vWriteReg(MC_REG_OFFSET1 + u4Addr, u4Val);
    }
}


UINT32 u4VDecReadMC(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val;
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif

    if (u4VDecID == 0)
    {
        u4Val = (u4ReadReg(MC_REG_OFFSET0 + u4Addr)); 
        //if (u4Addr != 0x28 && u4Addr != 0x2c)
        {
            vVDecSimDumpR(u4VDecID, MC_REG_OFFSET0, u4Addr, u4Val);
        }
        return u4Val;
    }
    else
    {
        return (u4ReadReg(MC_REG_OFFSET1 + u4Addr));
    }
}


void vVDecWriteCRC(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif

    if (u4VDecID == 0)
    {
        vWriteReg(VDEC_CRC_REG_OFFSET0 + u4Addr, u4Val);
        vVDecSimDumpW(u4VDecID, VDEC_CRC_REG_OFFSET0, u4Addr, u4Val);
    }
    else
    {
#ifdef VDEC_PIP_WITH_ONE_HW
        printk("PIP_ONE_HW: Wrong HW ID!!!\n");
        VDEC_ASSERT(0);
#endif
        vWriteReg(VDEC_CRC_REG_OFFSET0 + u4Addr, u4Val);
    }
}

void vVDecEnableCRC(UINT32 u4VDecID, BOOL fgEnable, BOOL fgCRCFromMC)
{
    UINT32 u4CRCSrc;
    UINT32 u4Val = 0;
    u4VDecID = 0;

    if (fgEnable)
    {
        u4Val = VDEC_CRC_EN;
    }

    if (fgCRCFromMC)
    {
        u4CRCSrc = u4Val | VDEC_CRC_SRC_MC;  // CRC input from MC
    }
    else
    {
        u4CRCSrc = u4Val | VDEC_CRC_SRC_PP;
    }

    vVDecWriteCRC(u4VDecID, VDEC_CRC_REG_EN, u4CRCSrc);
}

UINT32 u4VDecReadCRC(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val;
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif

    if (u4VDecID == 0)
    {
        u4Val = (u4ReadReg(VDEC_CRC_REG_OFFSET0 + u4Addr)); 
        vVDecSimDumpR(u4VDecID, VDEC_CRC_REG_OFFSET0, u4Addr, u4Val);
        return u4Val;
    }
    else
    {
        return (u4ReadReg(VDEC_CRC_REG_OFFSET0 + u4Addr));
    }
}

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8555)
void vVDecWriteDV(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif
    if (u4VDecID == 0)
    {
        vWriteReg(DV_REG_OFFSET0 + u4Addr, u4Val);
        vVDecSimDumpW(u4VDecID, DV_REG_OFFSET0, u4Addr, u4Val);
    }
    else
    {
#ifdef VDEC_PIP_WITH_ONE_HW
        printk("PIP_ONE_HW: Wrong HW ID!!!\n");
        VDEC_ASSERT(0);
#endif
        vWriteReg(DV_REG_OFFSET1 + u4Addr, u4Val);
    }
}

UINT32 u4VDecReadDV(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val;
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif

    if (u4VDecID == 0)
    {
        u4Val = (u4ReadReg(DV_REG_OFFSET0 + u4Addr)); 
        vVDecSimDumpR(u4VDecID, DV_REG_OFFSET0, u4Addr, u4Val);
        return u4Val;
    }
    else
    {
        return (u4ReadReg(DV_REG_OFFSET1 + u4Addr));
    }
}
#endif

void vVDec_HAL_CRC_Enable(UINT32 u4VDecID, BOOL fgCRCType)
{
    if (fgCRCType)
    {
        vVDecWriteCRC(u4VDecID, VDEC_CRC_REG_EN, 0x01);
    }
    else
    {
        vVDecWriteCRC(u4VDecID, VDEC_CRC_REG_EN, 0x03);
    }
}

// *********************************************************************
// Function : void vVDecResetVLDHW(UINT32 u4VDecID)
// Description : Reset Video decode HW
// Parameter : u4VDecID : VLD ID
// Return    : None
// *********************************************************************
#if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8555)
void vVDecResetHW(UINT32 u4VDecID)
#else
void vVDecResetHW(UINT32 u4VDecID, UINT32 u4VDecType)
#endif
{
    UINT32 u4Cnt = 0;
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8555)
    UINT32 u4VDecPDNCtrl;
#endif
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    UINT32 u4VDecSysClk;
#endif
    UINT32 u4VDecPDNCtrlSpec;
    UINT32 u4VDecPDNCtrlModule;


#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
    // HW issue, wait for read pointer stable
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    u4Cnt = 50000;
#ifdef VDEC_SIM_DUMP
    printk("if(`VDEC_PROCESS_FLAG == 1) wait(`VDEC_BITS_PROC_NOP == 1);\n");
    DBG_H264_PRINTF("if(`VDEC_PROCESS_FLAG == 1) wait(`VDEC_BITS_PROC_NOP == 1);\n");
#endif
    if (u4VDecReadVLD(u4VDecID, RO_VLD_SRAMCTRL) & PROCESS_FLAG)
    {
        while((!(u4VDecReadVLD(u4VDecID, RO_VLD_SRAMCTRL)&AA_FIT_TARGET_SCLK)) && (u4Cnt--));
    }
#else
    while (!(u4VDecReadVLD(u4VDecID, RO_VLD_SRAMCTRL)&1))
    {
        u4Cnt++;
        if (u4Cnt >= 0x1000)
        {
            break;
        }
    }
#endif

#else  
    // wait hw stable before reset hw
    while (!(u4VDecReadVLD(u4VDecID, RO_VLD_SRAMCTRL)&0x10000))
    {
        u4Cnt++;
        if (u4Cnt >= 0x1000)
        {
            break;
        }
    }
#endif

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    vVDecWriteVLD(u4VDecID, WO_VLD_SRST, (0x1 |(0x1<<8)));
#else
    vVDecWriteVLD(u4VDecID, WO_VLD_SRST, 1);
#endif
    //printk("[RMVB] after set. \n");

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8555)
    switch(u4VDecType)
    {        
    case VDEC_MPEG:
    case VDEC_MPEG1:
    case VDEC_MPEG2:
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8580)
        u4VDecPDNCtrl = 0x00801ED8;
#elif (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560)
        u4VDecPDNCtrl = 0x00000E98;
#else
        u4VDecPDNCtrl = 0x00000EB8;
#endif
        u4VDecPDNCtrlSpec = 0xFE;
        u4VDecPDNCtrlModule = 0x3F6A151D;
        break;
    case VDEC_DIVX3:
    case VDEC_MPEG4:
    case VDEC_H263:         
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8580)
        u4VDecPDNCtrl = 0x00801EB8;
#else
        u4VDecPDNCtrl = 0x00000EB8;
#endif
        u4VDecPDNCtrlSpec = 0xFD;
        u4VDecPDNCtrlModule = 0x3E6A1108;
        break;
    case VDEC_WMV:
    case VDEC_WMV1:
    case VDEC_WMV2:
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8580)
        u4VDecPDNCtrl = 0x00801EE8;
#else
        u4VDecPDNCtrl = 0x00000EE8;
#endif
        u4VDecPDNCtrlSpec = 0xFB;
        u4VDecPDNCtrlModule = 0x3E6A1108;
        break;
    case VDEC_WMV3:
    case VDEC_VC1:
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8580)
        u4VDecPDNCtrl = 0x00801DE8;
#else
        u4VDecPDNCtrl = 0x00000DE8;
#endif
        u4VDecPDNCtrlSpec = 0xFB;
        u4VDecPDNCtrlModule = 0x3E6A1108;
        break;
    case VDEC_H264:
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8580)
        u4VDecPDNCtrl = 0x00801FF0;
#else
        u4VDecPDNCtrl = 0x00000FF0;
#endif
        u4VDecPDNCtrlSpec = 0xF7;
        u4VDecPDNCtrlModule = 0x13A20100;
        break;
    case VDEC_RM:
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8580)
        u4VDecPDNCtrl = 0x00801F78;
#else
        u4VDecPDNCtrl = 0x00000F78;
#endif
        break;
    case VDEC_VP6:
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8580)
        u4VDecPDNCtrl = 0x00801BF8;
#else
        u4VDecPDNCtrl = 0x00000BF8;
#endif
        break;
    case VDEC_AVS:
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8580)
        u4VDecPDNCtrl = 0x008017F8;
#else
        u4VDecPDNCtrl = 0x000007F8;
#endif
        break;
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    case VDEC_VP8:
        u4VDecPDNCtrl = 0x00000F70;
        u4VDecPDNCtrlSpec = 0x7F;
        u4VDecPDNCtrlModule = 0x31A01100;        
        break;
#endif
    case VDEC_UNKNOWN:
    default:
        //Do nothing
        u4VDecPDNCtrl = 0x00000000;
        break;
    }
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    switch(u4VDecType)
    {        
    case VDEC_MPEG:
    case VDEC_MPEG1:
    case VDEC_MPEG2:
        u4VDecSysClk = 0x00000001;
        break;
    case VDEC_DIVX3:
    case VDEC_MPEG4:
    case VDEC_H263:         
        u4VDecSysClk = 0x00000001;
        break;
    case VDEC_WMV:
    case VDEC_WMV1:
    case VDEC_WMV2:
        u4VDecSysClk = 0x00000001;
        break;
    case VDEC_WMV3:
    case VDEC_VC1:
        u4VDecSysClk = 0x00000001;
        break;
    case VDEC_H264:
        u4VDecSysClk = 0x00000002;
        break;
    case VDEC_RM:
        u4VDecSysClk = 0x00000000;
        break;
    case VDEC_VP6:
        u4VDecSysClk = 0x00000000;
        break;
    case VDEC_AVS:
        u4VDecSysClk = 0x00000000;
        break;
    case VDEC_VP8:
        u4VDecSysClk = 0x00000001;
        break;
    case VDEC_UNKNOWN:
    default:
        //Do nothing
        u4VDecSysClk = 0x00000000;
        break;
    }
    vVDecWriteDV( u4VDecID, RW_SYS_CLK_SEL, u4VDecSysClk);
#endif

#ifdef REALCHIP_DVT
    // Per spec configuration
    vVDecWriteDV( u4VDecID, 0xC8, u4VDecPDNCtrlSpec);
    vVDecWriteDV( u4VDecID, 0xCC, u4VDecPDNCtrlModule);
    // Turn off auto-power-off after IRQ
    vWriteGconReg(6*4, 1);
    vVDecWriteDV( u4VDecID, 59*4, 1);
#endif
    vVDecWriteDV( u4VDecID, RW_PDN_CTRL, ((u4VDecReadDV(u4VDecID, RW_PDN_CTRL) & 0xFF400000) | u4VDecPDNCtrl) ); // for VP8 bit 23 can not be 1
#endif
    //    printk("sysclk = 0x%x,pwd = 0x%x,HWID = %d,codec = 0x%x\n",u4VDecReadDV(u4VDecID, RW_SYS_CLK_SEL),u4VDecReadDV(u4VDecID, RW_PDN_CTRL)&0x3fffff,u4VDecID,u4VDecType);
    vVDecWriteVLD(u4VDecID, WO_VLD_SRST, 0);
}


// *********************************************************************
// Function : void vVDecSetVLDVFIFO(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4VFifoSa, UINT32 u4VFifoEa)
// Description : Set VFIFO start address and end address
// Parameter : u4VDecID : VLD ID
// Return    : None
// *********************************************************************
void vVDecSetVLDVFIFO(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4VFifoSa, UINT32 u4VFifoEa)
{
    vVDecWriteVLD(u4VDecID, RW_VLD_VSTART + (u4BSID << 10), u4VFifoSa >> 6);
    vVDecWriteVLD(u4VDecID, RW_VLD_VEND + (u4BSID << 10), u4VFifoEa >> 6);
}

UINT32 u4VDecReadVldRPtrEx(UINT32 u4BSID, UINT32 u4VDecID, UINT32 *pu4Bits, UINT32 u4VFIFOSa, const char *szFunction, INT32 i4Line)
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
    DBG_H264_PRINTF("if(`VDEC_PROCESS_FLAG == 1) wait(`VDEC_BITS_PROC_NOP == 1);\n");

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
  
    *pu4Bits = u4VDecReadVLD(u4VDecID, RW_VLD_BITCOUNT + (u4BSID << 10)) & 0x3f;

    if (u4VDecReadVLD(u4VDecID, 4*59) & (0x1 << 28))
    {              
        if (u4VDecReadVLD(u4VDecID, RW_VLD_WMV_ABS + (u4BSID << 10)) & 0x1) // vc1 not ready @2011/08/11
        {
            u4BsBufLen = 6 * 4;
        }
        else
        {
            u4BsBufLen = 5 * 4;
        }
    }
    else // old case
    {
        if (u4VDecReadVLD(u4VDecID, RW_VLD_WMV_ABS + (u4BSID << 10)) & 0x1) // vc1 case
        {
            u4BsBufLen = 5 * 4;
        }
        else
        {
            u4BsBufLen = 4 * 4;
        }
    }    


    #ifdef VDEC_SIM_DUMP
      printk("//<vdec> ReadVldRPtr, dRptr:0x%08X, sra:0x%08X, swa:0x%08X, scnt:0x%08X, sum:0x%08X, u4BsBufLen=%u\n", 
        u4DramRptr, vb_sram_ra, vb_sram_wa, seg_rcnt, *pu4Bits, u4BsBufLen);
      DBG_H264_PRINTF("<vdec> ReadVldRPtr, dRptr:0x%08X, sra:0x%08X, swa:0x%08X, scnt:0x%08X, sum:0x%08X, u4BsBufLen=%u\n", u4DramRptr, vb_sram_ra, vb_sram_wa, seg_rcnt, *pu4Bits, u4BsBufLen);
    #endif

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
      printk("//<vdec> RdPtr=0x%08X (%u) @(%s, %d)\n", u4ByteAddr, u4ByteAddr, szFunction, i4Line);
      //DBG_H264_PRINTF("<vdec> RdPtr=0x%08X (%u) @(%s, %d)\n", u4ByteAddr, u4ByteAddr, szFunction, i4Line);
    #endif

    return (u4ByteAddr);
}


// *********************************************************************
// Function : BOOL fgVDecIsVLDFetchOk(UINT32 u4BSID, UINT32 u4VDecID)
// Description : Check if VLD fetch is done
// Parameter : None
// Return    : TRUE: VLD fetch OK, FALSE: not OK
// *********************************************************************
BOOL fgVDecIsVLDFetchOk(UINT32 u4BSID, UINT32 u4VDecID)
{
    if ((u4VDecReadVLD(u4VDecID, RO_VLD_FETCHOK + (u4BSID << 10)) & VLD_FETCH_OK) == 0)
    {
        return (FALSE);
    }
    return (TRUE);
}


BOOL fgVDecWaitVldFetchOk(UINT32 u4BSID, UINT32 u4VDecID)
{
    UINT32 u4Cnt;

    #ifdef VDEC_SIM_DUMP
    printk("wait(`VDEC_INI_FETCH_RDY == 1);\n");
    DBG_H264_PRINTF("wait(`VDEC_INI_FETCH_RDY == 1);\n");
    #endif
  
    if (!fgVDecIsVLDFetchOk(u4BSID, u4VDecID))
    {
        u4Cnt = 0;
        while (!fgVDecIsVLDFetchOk(u4BSID, u4VDecID))
        {
            u4Cnt++;
            if (u4Cnt >= 0x1000)
            {
                return (FALSE);
            }
        }
    }
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
#else
    u4Cnt = 0;
    // HW modification
    // read point may not stable after read fetck ok flag
    while (!(u4VDecReadVLD(u4VDecID, RO_VLD_SRAMCTRL + (u4BSID << 10))&0x10000))
    {
        u4Cnt++;
        if (u4Cnt >= 0x1000)
        {
            return (FALSE);
        }
    }
#endif   
    return (TRUE);
}


UINT32 u4VDec03Number(UINT32 u4Addr, UINT32 u4Range)
{
    INT32 i;
    UINT32 u403Cnt = 0;
    UINT32 u4ZeroCnt = 0;
    UCHAR *pucAddr;

    pucAddr = (UCHAR *)u4Addr;

    //    BSP_dma_unmap_single(PHYSICAL(u4Addr),u4Range,FROM_DEVICE);
    for ( i=0; i < u4Range; i++, pucAddr++)
    {
        if ((*pucAddr) == 0)
        {
            u4ZeroCnt++;
        }
        else
        {
            if(((*pucAddr) == 3) && (u4ZeroCnt >= 2))
            {
                u403Cnt++;
            }
            u4ZeroCnt = 0;
        }
    }
    //   BSP_dma_map_single(u4Addr,u4Range,FROM_DEVICE);
    return (u403Cnt);
}


// *********************************************************************
// Function : void vMCSetOutputBuf(UINT32 u4VDecID, UINT32 u4OutBufIdx, UINT32 u4FRefBufIdx)
// Description : MC Set Output buffer index
// Parameter : dOutBufIdx: Output Buffer index, 0, 1, MC_DIG_BUF, MC_B_BUF
//             dFRefBufIdx: Forward reference buffer index
// Return    : None
// *********************************************************************
void vMCSetOutputBuf(UINT32 u4VDecID, UINT32 u4OutBufIdx, UINT32 u4FRefBufIdx)
{
    vVDecWriteMC(u4VDecID, RW_MC_OPBUF, u4OutBufIdx);
    vVDecWriteMC(u4VDecID, RW_MC_FWDP, u4FRefBufIdx);
}


// *********************************************************************
// Function : UINT32 dVLDGetBitS(UINT32 u4BSID, UINT32 u4VDecID, UINT32 dShiftBit)
// Description : Get Bitstream from VLD barrel shifter
// Parameter : dShiftBit: Bits to shift (0-32)
// Return    : barrel shifter
// *********************************************************************
UINT32 u4VDecVLDGetBitS(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBit)
{
    UINT32 u4RegVal;
#if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8580)
    UCHAR ucTmp=0;
#endif
    //u4RegVal = u4VDecReadVLD(u4VDecID, RO_VLD_BARL + (u4BSID << 10));


    u4RegVal = u4VDecReadVLD(u4VDecID, RO_VLD_BARL + (u4BSID << 10) + (u4ShiftBit << 2));

#if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8580)

    /* added by Alan Cheng for JPEG debug error according to GS Lin */

    ucTmp = (u4VDecReadVLD(u4VDecID, WO_VLD_RDY)&0x10000000)>>28;
    while (!ucTmp)
    {
        ucTmp = (u4VDecReadVLD(u4VDecID, WO_VLD_RDY)&0x10000000)>>28;
    }

#endif   

    //u4VDecReadVLD(u4VDecID, RO_VLD_BARL + (u4BSID << 10));
    /* end */

    return (u4RegVal);
}


#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8555)
void vVDecPowerDownHW(UINT32 u4VDecID)
{
    vVDecWriteDV( u4VDecID, RW_PDN_CTRL, (u4VDecReadDV(u4VDecID, RW_PDN_CTRL) | 0x003FFFFF) );
}
#endif

// *********************************************************************
// Function : void vVLDLoadQuanMat(BOOL fgIntra)
// Description : load Q matrix
// Parameter : fgIntra: TRUE: Load Intra Matrix, FALSE: Load Non-Intra
// Return    : None
// *********************************************************************
UINT32 _u4VldDummy=0;
#if (CONFIG_DRV_ONLY || CONFIG_DRV_VERIFY_SUPPORT)
void vVLDLoadQuanMat(UINT32 u4VDecID, BOOL fgIntra)
{
    // hardware will get 64 byte auto
    // use _dwVldDummy to prevent code being optimize by compiler
    if(fgIntra)
    {
        _u4VldDummy = u4VDecReadVLD(u4VDecID, RW_VLD_TABLIM);
    }
    else
    {
        _u4VldDummy = u4VDecReadVLD(u4VDecID, RW_VLD_TABLNIM);
    }
}
#endif

void vWriteVDSCL(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8555)
    VDEC_ASSERT(0);
#endif

#if (CONFIG_DRV_VERIFY_SUPPORT && (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif


    if (u4VDecID == 0)
    {
        vWriteReg(VDSCL_REG_OFFSET0 + u4Addr, u4Val);
        vVDecSimDumpW(u4VDecID, VDSCL_REG_OFFSET0, u4Addr, u4Val);
    }
    else
    {
        vWriteReg(VDSCL_REG_OFFSET1 + u4Addr, u4Val);
    }
}


UINT32 u4ReadVDSCL(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val;
#if (CONFIG_DRV_VERIFY_SUPPORT && (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif

    if (u4VDecID == 0)
    {
        u4Val = (u4ReadReg(VDSCL_REG_OFFSET0 + u4Addr));
        vVDecSimDumpR(u4VDecID, VDSCL_REG_OFFSET0, u4Addr, u4Val);
        return u4Val;
    }
    else
    {
        return (u4ReadReg(VDSCL_REG_OFFSET1 + u4Addr));
    }
}


void vVDSCLReset(UINT32 u4VDecID)
{
    vWriteVDSCL(u4VDecID, RW_VDSCL_ACTIVATE, RW_VDSCL_SW_RESET);
}


void vVDSCLSetEnable(UINT32 u4VDecID, BOOL fgEnable)
{
    if (fgEnable)
    {
        vWriteVDSCL(u4VDecID, RW_VDSCL_ACTIVATE, u4ReadVDSCL(u4VDecID, RW_VDSCL_ACTIVATE) | RW_VDSCL_EN);
    }
    else
    {
        vWriteVDSCL(u4VDecID, RW_VDSCL_ACTIVATE, u4ReadVDSCL(u4VDecID, RW_VDSCL_ACTIVATE) & (~RW_VDSCL_EN));
    }
}


void vVDSCLSetConvert(UINT32 u4VDecID, BOOL fgConvert)
{
    if (fgConvert)
    {
        vWriteVDSCL(u4VDecID, RW_VDSCL_ACTIVATE, u4ReadVDSCL(u4VDecID, RW_VDSCL_ACTIVATE) | RW_VDSCL_CONVERT);
    }
    else
    {
        vWriteVDSCL(u4VDecID, RW_VDSCL_ACTIVATE, u4ReadVDSCL(u4VDecID, RW_VDSCL_ACTIVATE) & (~RW_VDSCL_CONVERT));
    }
}


void vVDSCLSetPicStruct(UINT32 u4VDecID, EPicStruct ePicStruct)
{
    if (ePicStruct > 3) 
    {
        return;
    }
    vWriteVDSCL(u4VDecID, RW_VDSCL_TYPE, (u4ReadVDSCL(u4VDecID, RW_VDSCL_TYPE) & (~RW_VDSCL_PIC_STRU)) | ((UINT32)ePicStruct));
}

#if VDEC_REMOVE_UNUSED_FUNC
void vVDSCLSetRasterScanOut(UINT32 u4VDecID, BOOL fgRasterScan)
{
    if (fgRasterScan)
    {
        vWriteVDSCL(u4VDecID, RW_VDSCL_TYPE, u4ReadVDSCL(u4VDecID, RW_VDSCL_TYPE) | RW_VDSCL_RASTER_SCAN);
    }
    else
    {
        vWriteVDSCL(u4VDecID, RW_VDSCL_TYPE, u4ReadVDSCL(u4VDecID, RW_VDSCL_TYPE) & (~RW_VDSCL_RASTER_SCAN));
    }
}
#endif

void vVDSCLSetAddrSwap(UINT32 u4VDecID, UINT32 u4Mode)
{
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
    if (u4Mode > 7)
#else
    if (u4Mode > 3)
#endif
    {
        return;
    }

#if 0//(CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8520)
    if(u4Mode)
    {
        if(u4Mode == ASM_1)
        {
            u4Mode = RW_VDSCL_ADDR_SWAP_MODE1;
        }
        else
        {
            u4Mode = RW_VDSCL_ADDR_SWAP_MODE2;
        }
    }
#endif
    vWriteVDSCL(u4VDecID, RW_VDSCL_MEM_IF, (u4ReadVDSCL(u4VDecID, RW_VDSCL_MEM_IF) & (~RW_VDSCL_ADDR_SWAP_MODE2)) | u4Mode);
}


void vVDSCLSetBoundPad(UINT32 u4VDecID, EPadMode ePadMode, UCHAR ucYColor, UCHAR ucCbColor, UCHAR ucCrColor)
{
    switch (ePadMode)
    {
    case PAD_MODE_NO_PAD:
        vWriteVDSCL(u4VDecID, RW_VDSCL_BOUND, u4ReadVDSCL(u4VDecID, RW_VDSCL_BOUND) | RW_VDSCL_BOUND_NO_PAD);
        break;
    case PAD_MODE_EXTEND:
        vWriteVDSCL(u4VDecID, RW_VDSCL_BOUND, u4ReadVDSCL(u4VDecID, RW_VDSCL_BOUND) & (~RW_VDSCL_BOUND_NO_PAD));
        vWriteVDSCL(u4VDecID, RW_VDSCL_BOUND, u4ReadVDSCL(u4VDecID, RW_VDSCL_BOUND) & (~RW_VDSCL_BOUND_PAD_EN));
        break;
    case PAD_MODE_COLOR:
        vWriteVDSCL(u4VDecID, RW_VDSCL_BOUND, u4ReadVDSCL(u4VDecID, RW_VDSCL_BOUND) & (~RW_VDSCL_BOUND_NO_PAD));
        vWriteVDSCL(u4VDecID, RW_VDSCL_BOUND, u4ReadVDSCL(u4VDecID, RW_VDSCL_BOUND) | RW_VDSCL_BOUND_PAD_EN);
        {
            UINT32 u4Y,u4Cb,u4Cr;

            u4Y = ucYColor;
            u4Cb = ucCbColor;
            u4Cr = ucCrColor;

            vWriteVDSCL(u4VDecID, RW_VDSCL_BOUND, (u4ReadVDSCL(u4VDecID, RW_VDSCL_BOUND) & (~RW_VDSCL_BOUND_PAD_Y) & (~RW_VDSCL_BOUND_PAD_CB) & (~RW_VDSCL_BOUND_PAD_CR)) | (u4Y<<16) | (u4Cb<<8) | u4Cr);
        }
        break;
    }
}


void vVDSCLSetSrcPicSize(UINT32 u4VDecID, UINT32 u4SrcWidth, UINT32 u4SrcHeight)
{
    if ((u4SrcWidth>0xFFF) || (u4SrcHeight>0xFFF))
    {
        return;
    }
    vWriteVDSCL(u4VDecID, RW_VDSCL_SRC_SIZE, (u4ReadVDSCL(u4VDecID, RW_VDSCL_SRC_SIZE) & (~RW_VDSCL_SRC_WIDTH) & (~RW_VDSCL_SRC_HEIGHT)) | (u4SrcWidth<<16) | u4SrcHeight);
}


void vVDSCLSetTrgPicSize(UINT32 u4VDecID, UINT32 u4TrgWidth, UINT32 u4TrgHeight)
{
    if ((u4TrgWidth>0xFFF) || (u4TrgHeight>0xFFF)) 
    {
        return;
    }
    vWriteVDSCL(u4VDecID, RW_VDSCL_TG_SIZE, (u4ReadVDSCL(u4VDecID, RW_VDSCL_TG_SIZE) & (~RW_VDSCL_TG_WIDTH) & (~RW_VDSCL_TG_HEIGHT)) | (u4TrgWidth<<16) | u4TrgHeight);
}


void vVDSCLSetTrgPicOffset(UINT32 u4VDecID, UINT32 u4TrgX, UINT32 u4TrgY)
{
    if ((u4TrgX>0xFFF) || (u4TrgY>0xFFF))
    {
        return;
    }
    vWriteVDSCL(u4VDecID, RW_VDSCL_TG_OFFSET,(u4TrgX<<16) | u4TrgY);
}


void vVDSCLSetTrgBufWidth(UINT32 u4VDecID, UINT32 u4TrgBufWidth)
{
    if (u4TrgBufWidth>0xFFF) 
    {
        return;
    }
    vWriteVDSCL(u4VDecID, RW_VDSCL_TG_BUF_LEN, (u4ReadVDSCL(u4VDecID, RW_VDSCL_TG_BUF_LEN) & (~RW_VDSCL_TRG_BUF_LEN)) | (u4TrgBufWidth>>4));
}


void vVDSCLSetTrgYBufBaseAddr(UINT32 u4VDecID, UINT32 u4YBufBaseAddr)
{
    //if(u4YBufBaseAddr>0xFFFFFF) return;

    vWriteVDSCL(u4VDecID, RW_VDSCL_TG_Y_OW_BASE, (u4ReadVDSCL(u4VDecID, RW_VDSCL_TG_Y_OW_BASE) & (~RW_VDSCL_TG_Y_OW_ADDR_BASE)) | (u4YBufBaseAddr>>4));
}


void vVDSCLSetTrgCBufBaseAddr(UINT32 u4VDecID, UINT32 u4CBufBaseAddr)
{
    //if(u4CBufBaseAddr>0xFFFFFF) return;

    vWriteVDSCL(u4VDecID, RW_VDSCL_TG_C_OW_BASE, (u4ReadVDSCL(u4VDecID, RW_VDSCL_TG_C_OW_BASE) & (~RW_VDSCL_TG_C_OW_ADDR_BASE)) | (u4CBufBaseAddr>>4));
}


void vVDSCLSetHScaleFactor(UINT32 u4VDecID, UINT32 u4HScaleFactorY, UINT32 u4HScaleFactorC)
{
    vWriteVDSCL(u4VDecID, RW_VDSCL_H_SCL_Y, (u4ReadVDSCL(u4VDecID, RW_VDSCL_H_SCL_Y) & (~RW_VDSCL_H_SCL_Y_FACTOR)) | u4HScaleFactorY);
    vWriteVDSCL(u4VDecID, RW_VDSCL_H_SCL_C, (u4ReadVDSCL(u4VDecID, RW_VDSCL_H_SCL_C) & (~RW_VDSCL_H_SCL_C_FACTOR)) | u4HScaleFactorC);
}


void vVDSCLSetHScaleOffset(UINT32 u4VDecID, UINT32 u4HScaleOffsetY, UINT32 u4HScaleOffsetC)
{
    vWriteVDSCL(u4VDecID, RW_VDSCL_H_SCL_Y, (u4ReadVDSCL(u4VDecID, RW_VDSCL_H_SCL_Y) & (~RW_VDSCL_H_SCL_Y_OFFSET)) | (u4HScaleOffsetY<<16));
    vWriteVDSCL(u4VDecID, RW_VDSCL_H_SCL_C, (u4ReadVDSCL(u4VDecID, RW_VDSCL_H_SCL_C) & (~RW_VDSCL_H_SCL_C_OFFSET)) | (u4HScaleOffsetC<<16));
}


void vVDSCLSetVScaleFactor(UINT32 u4VDecID, UINT32 u4VScaleFactorY, UINT32 u4VScaleFactorC)
{
    vWriteVDSCL(u4VDecID, RW_VDSCL_V_SCL_Y, (u4ReadVDSCL(u4VDecID, RW_VDSCL_V_SCL_Y) & (~RW_VDSCL_V_SCL_Y_FACTOR)) | u4VScaleFactorY);
    vWriteVDSCL(u4VDecID, RW_VDSCL_V_SCL_C, (u4ReadVDSCL(u4VDecID, RW_VDSCL_V_SCL_C) & (~RW_VDSCL_V_SCL_C_FACTOR)) | u4VScaleFactorC);
}


void vVDSCLSetVScaleOffset(UINT32 u4VDecID, UINT32 u4VScaleOffsetY, UINT32 u4VScaleOffsetC)
{
    vWriteVDSCL(u4VDecID, RW_VDSCL_V_SCL_Y, (u4ReadVDSCL(u4VDecID, RW_VDSCL_V_SCL_Y) & (~RW_VDSCL_V_SCL_Y_OFFSET)) | (u4VScaleOffsetY<<16));
    vWriteVDSCL(u4VDecID, RW_VDSCL_V_SCL_C, (u4ReadVDSCL(u4VDecID, RW_VDSCL_V_SCL_C) & (~RW_VDSCL_V_SCL_C_OFFSET)) | (u4VScaleOffsetC<<16));
}

#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8520) 
void vVDSCLSetScalerHParm(UINT32 u4VDecID, UINT32 u4SrcWidth, UINT32 u4TrgWidth)
#else
void vVDSCLSetScalerHParm(UINT32 u4VDecID, UINT32 u4SrcWidth, UINT32 u4TrgWidth, VDEC_INFO_VDSCL_PRM_T *prDownScalerPrm)
#endif
{
    UINT32 u4HScalerFactor,u4HScalerOffset;

    if (u4SrcWidth == 0)
    {
        return;
    }
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8520)   
    u4HScalerFactor = ((2048 * u4TrgWidth) + (u4SrcWidth>>1)) / u4SrcWidth;
    u4HScalerOffset = 2048 - u4HScalerFactor;
    //u4HScalerOffset = 0;
#else
    u4HScalerFactor = ((0x8000 * u4TrgWidth) + (u4SrcWidth>>1)) / u4SrcWidth;
#ifdef DOWN_SCALE_SUPPORT
    prDownScalerPrm->u4SclYOffH = (UINT32)(((UINT32) rand())%u4HScalerFactor);
    prDownScalerPrm->u4SclCOffH = prDownScalerPrm->u4SclYOffH ;
#endif
    u4HScalerOffset = 0x8000 - u4HScalerFactor + prDownScalerPrm->u4SclYOffH;
#endif

    vVDSCLSetHScaleFactor(u4VDecID, u4HScalerFactor, u4HScalerFactor);
    vVDSCLSetHScaleOffset(u4VDecID, u4HScalerOffset, u4HScalerOffset);
}

#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8520)
void vVDSCLSetScalerVParm(UINT32 u4VDecID, UINT32 u4SrcHeight, UINT32 u4TrgHeight, BOOL fgIs422, BOOL fgIsMbaff, UINT32 u4OffY, UINT32 u4OffC)
{
    UINT32 u4VScalerFactorY,u4VScalerOffsetY;
    UINT32 u4VScalerFactorC,u4VScalerOffsetC;

    if (u4SrcHeight == 0)
    {
        return;
    }

    //u4VScalerFactorY = ((2048 * u4TrgHeight) + (u4SrcHeight>>1)) / u4SrcHeight;
    if (0 == fgIsMbaff)
    {
        u4VScalerFactorY = ((2048 * u4TrgHeight) + (u4SrcHeight>>1)) / u4SrcHeight;
    }
    else
    {
        // If has remainder, plus 1
        u4VScalerFactorY = (2048 * u4TrgHeight + (u4SrcHeight - 1)) / u4SrcHeight; // by Subrina, 11212007
    }
    u4VScalerOffsetY = 0;
    if (fgIs422)
    {
        //u4VScalerFactorC = ((4096 * u4TrgHeight) + (u4SrcHeight>>1)) / u4SrcHeight;
        if (0 == fgIsMbaff)
        {
            u4VScalerFactorC = ((4096 * u4TrgHeight) + (u4SrcHeight>>1)) / u4SrcHeight;
        }
        else
        {
            // If has remainder, plus 1
            u4VScalerFactorC = (4096 * u4TrgHeight + (u4SrcHeight - 1)) / u4SrcHeight; // by Subrina, 11212007
        }
        u4VScalerOffsetC = 0;
    }
    else // 420
    {
#if 0 //ndef NEW_VDSCL_PARAM    
        u4VScalerFactorC = u4VScalerFactorY;
        u4VScalerOffsetC = u4VScalerOffsetY;
#else  
        u4VScalerFactorC = u4VScalerFactorY;
        u4VScalerOffsetC = u4OffC; 
        u4VScalerOffsetY = u4OffY;
#endif
    }

    vVDSCLSetVScaleFactor(u4VDecID, u4VScalerFactorY, u4VScalerFactorC);
    vVDSCLSetVScaleOffset(u4VDecID, u4VScalerOffsetY, u4VScalerOffsetC);

}
#else
void vVDSCLSetScalerVParm(UINT32 u4VDecID, UINT32 u4SrcHeight, UINT32 u4TrgHeight, BOOL fgIs422, BOOL fgIsMbaff, UINT32 u4OffY, UINT32 u4OffC, VDEC_INFO_VDSCL_PRM_T *prDownScalerPrm)
{
    UINT32 u4VScalerFactorY,u4VScalerOffsetY;
    UINT32 u4VScalerFactorC,u4VScalerOffsetC;


    if (u4SrcHeight == 0)
    {
        return;
    }

    u4VScalerFactorY = ((0x8000 * u4TrgHeight) + (u4SrcHeight>>1)) / u4SrcHeight;
#if 0
    if (0 == fgIsMbaff)
    {
        u4VScalerFactorY = ((0x8000 * u4TrgHeight) + (u4SrcHeight>>1)) / u4SrcHeight;
    }
    else
    {
        u4Remainer = (0x8000 * u4TrgHeight) % u4SrcHeight;
        if (0 == u4Remainer)
        {
            u4VScalerFactorY = (0x8000 * u4TrgHeight) / u4SrcHeight; // by Subrina, 11212007
        }
        else
        {
            u4VScalerFactorY = (0x8000 * u4TrgHeight) / u4SrcHeight + 1; // by Subrina, 11212007
        }
    }
#endif

    if (fgIs422)
    {
        u4VScalerFactorC = ((0x10000 * u4TrgHeight) + (u4SrcHeight>>1)) / u4SrcHeight;
#if 0
        if (0 == fgIsMbaff)
        {
            u4VScalerFactorC = ((0x10000 * u4TrgHeight) + (u4SrcHeight>>1)) / u4SrcHeight;
        }
        else
        {
            u4Remainer = (0x10000 * u4TrgHeight) % u4SrcHeight;
            if (0 == u4Remainer)
            {
                u4VScalerFactorC = (0x10000 * u4TrgHeight) / u4SrcHeight; // by Subrina, 11212007
            }
            else
            {
                u4VScalerFactorC = (0x10000 * u4TrgHeight) / u4SrcHeight + 1; // by Subrina, 11212007
            }
        }
#endif
#ifdef DOWN_SCALE_SUPPORT
        u4VScalerOffsetC = (UINT32)(((UINT32) rand())%u4VScalerFactorC); 
        u4VScalerOffsetY = (UINT32)(((UINT32) rand())%u4VScalerFactorY);
#else
        u4VScalerOffsetC = (u4VScalerFactorC -1); 
        u4VScalerOffsetY = (u4VScalerFactorY-1);
#endif
    }
    else // 420
    {
        u4VScalerFactorC = u4VScalerFactorY;
#ifdef DOWN_SCALE_SUPPORT
        u4VScalerOffsetC = (UINT32)(((UINT32) rand())%u4VScalerFactorC); 
        u4VScalerOffsetY = (UINT32)(((UINT32) rand())%u4VScalerFactorY);
#else
        u4VScalerOffsetC = (u4VScalerFactorC-1); 
        u4VScalerOffsetY = (u4VScalerFactorY-1);
#endif
    }


    prDownScalerPrm->u4SclYOffV = u4VScalerOffsetY;
    prDownScalerPrm->u4SclCOffV = u4VScalerOffsetC;

    vVDSCLSetVScaleFactor(u4VDecID, u4VScalerFactorY, u4VScalerFactorC);
    vVDSCLSetVScaleOffset(u4VDecID, u4VScalerOffsetY, u4VScalerOffsetC);
}
#endif

void vVDSCLSetSrcOffset(UINT32 u4VDecID, UINT32 u4SrcYOffH, UINT32 u4SrcYOffV, UINT32 u4SrcCOffH, UINT32 u4SrcCOffV)
{
    vWriteVDSCL(u4VDecID, RW_VDSCL_SRC_OFFSET,  (((u4SrcCOffH&0xF)<<12)|((u4SrcCOffV&0xF)<<8)|((u4SrcYOffH&0xF)<<4)|(u4SrcYOffV&0xF)));
}

void vVDSCLSetTmpBufBaseAddr(UINT32 u4VDecID, UINT32 u4TmpBufBaseAddr)
{
    vWriteVDSCL(u4VDecID, RW_VDSCL_TMP_OW_BASE, (u4ReadVDSCL(u4VDecID, RW_VDSCL_TMP_OW_BASE) & (~RW_VDSCL_TMP_OW_ADDR_BASE)) | (u4TmpBufBaseAddr >> 4));
}

#if VDEC_REMOVE_UNUSED_FUNC
BOOL fgVDSCLIsDone(UINT32 u4VDecID)
{
    return (u4ReadVDSCL(u4VDecID, RO_VDSCL_DONE) & RO_VDSCL_DONE_FRAME);
}
#endif

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8550)
void vVDSCLSetLumaKey(UINT32 u4VDecID, BOOL fgLumaKeyEn, UINT16 u2Key)
{
    if (fgLumaKeyEn)
    {
        vWriteVDSCL(u4VDecID, RW_VDSCL_LUMA_KEY, (0x110001 | (u2Key << 0x8)) );
    }
    else
    {
        vWriteVDSCL(u4VDecID, RW_VDSCL_LUMA_KEY, 0);
    }
}
#endif

// **************************************************************************
// Function : void vVDECSetDownScalerPrm(UINT32 u4VDecID,VDEC_INFO_VDSCL_PRM_T *prDownScalerPrm)
// Description :Set down scaler hardware registers
// Parameter : u4VDecID : video decoder hardware ID
//                 prDownScalerPrm : pointer to down scaler prameter struct
// Return      : None
// **************************************************************************
void vVDECSetDownScalerPrm(UINT32 u4VDecID,VDEC_INFO_VDSCL_PRM_T *prDownScalerPrm)
{
#if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8555)

#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8520)
    UINT32 src_h, tg_h;
    UINT32 u4VDSCL_SRC_OFFSET = 0;
#endif

  
    // down scaler
    vVDSCLReset(u4VDecID);
    vVDSCLSetEnable(u4VDecID, prDownScalerPrm->fgDSCLEn);
    if (prDownScalerPrm->fgDSCLEn)
    {
        vVDSCLSetAddrSwap(u4VDecID, prDownScalerPrm->ucAddrSwapMode);
        vWriteVDSCL(u4VDecID, RW_VDSCL_TYPE, (prDownScalerPrm->ucScrAgent << 2) | 
                                                          (prDownScalerPrm->ucSpectType << 5) | 
                                                          (prDownScalerPrm->fgYOnly << 7) |
                                                          (prDownScalerPrm->ucScanType << 16) |
                                                          (prDownScalerPrm->ucVdoFmt << 17));
        
        vVDSCLSetBoundPad(u4VDecID, PAD_MODE_NO_PAD, 0, 0, 0);
        
        vVDSCLSetSrcPicSize(u4VDecID, prDownScalerPrm->u4SrcWidth, prDownScalerPrm->u4SrcHeight);
        vVDSCLSetTrgBufWidth(u4VDecID, prDownScalerPrm->u4DispW);
        vVDSCLSetTrgPicSize(u4VDecID, prDownScalerPrm->u4TrgWidth, prDownScalerPrm->u4TrgHeight);
        vVDSCLSetTrgPicOffset(u4VDecID, prDownScalerPrm->u4TrgOffH, prDownScalerPrm->u4TrgOffV);
        vVDSCLSetTrgYBufBaseAddr(u4VDecID, u4AbsDramANc((UINT32) prDownScalerPrm->u4TrgYAddr));
        vVDSCLSetTrgCBufBaseAddr(u4VDecID, u4AbsDramANc((UINT32) prDownScalerPrm->u4TrgCAddr));
        vVDSCLSetTmpBufBaseAddr(u4VDecID, u4AbsDramANc((UINT32) prDownScalerPrm->u4WorkAddr));
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8520)       
        vVDSCLSetScalerHParm(u4VDecID, prDownScalerPrm->u4SrcWidth, prDownScalerPrm->u4TrgWidth);
#else
        vVDSCLSetScalerHParm(u4VDecID, prDownScalerPrm->u4SrcWidth, prDownScalerPrm->u4TrgWidth, prDownScalerPrm);
#endif        

#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8520)
        prDownScalerPrm->u4QY = 0;
        prDownScalerPrm->u4QC = 0;
        prDownScalerPrm->u4R_norm = 0;
        if (prDownScalerPrm->ucPicStruct == BTM_FLD_PIC)
        {
           src_h = prDownScalerPrm->u4SrcHeight;
           tg_h = prDownScalerPrm->u4TrgHeight;
            prDownScalerPrm->u4QY = (src_h - tg_h) /(tg_h * 2 );
            prDownScalerPrm->u4QC = (src_h - tg_h) /(tg_h * 2 );
            prDownScalerPrm->u4R_norm = ( ((src_h - tg_h) - (prDownScalerPrm->u4QY * (tg_h *2))) * 2048 ) / (2 * src_h );           
        }
        
        prDownScalerPrm->u4OffY = prDownScalerPrm->u4R_norm;
        prDownScalerPrm->u4OffC = prDownScalerPrm->u4R_norm;
        
        if (prDownScalerPrm->ucScrAgent == VDSCL_SRC_PP)
        { 
            //source PP
            if (prDownScalerPrm->ucSpectType == VDSCL_SPEC_WMV)
            {
                //WMV case, max src_v_ofty = 4, max src_v_oftc = 0
                if (prDownScalerPrm->u4QY > 4)
                {
                    prDownScalerPrm->u4QY = 4;
                }

                prDownScalerPrm->u4QC = 0;          
            }
            else
            {
                //h264 case, max src_v_ofty = 6, max src_v_oftc = 3
                if (prDownScalerPrm->u4QY > 6)
                {
                    prDownScalerPrm->u4QY = 6;
                }

                if (prDownScalerPrm->u4QC > 3)
                {
                    prDownScalerPrm->u4QC = 3;
                }
            }

        }
        else
        { 
            //source MC
            //for MC output, max src_v_ofty = 8, max src_v_oftc = 4      
            if (prDownScalerPrm->u4QY > 8)
            {
                prDownScalerPrm->u4QY = 8;
            }
            
            if (prDownScalerPrm->u4QC > 4)
            {
                prDownScalerPrm->u4QC = 4;
            }
        }
         
        //vWriteVDSCL(u4VDecID, RW_VDSCL_SRC_OFFSET, u4ReadVDSCL(u4VDecID, RW_VDSCL_SRC_OFFSET)  | (prDownScalerPrm->u4QC<<8) | (prDownScalerPrm->u4QY));
        u4VDSCL_SRC_OFFSET = (u4ReadVDSCL(u4VDecID, RW_VDSCL_SRC_OFFSET) & 0xFFFF0000);
     u4VDSCL_SRC_OFFSET |=  ( (prDownScalerPrm->u4QC<<8) | (prDownScalerPrm->u4QY));
     vWriteVDSCL(u4VDecID,RW_VDSCL_SRC_OFFSET, u4VDSCL_SRC_OFFSET);

        vVDSCLSetScalerVParm(u4VDecID, prDownScalerPrm->u4SrcHeight, prDownScalerPrm->u4TrgHeight, \
                                             prDownScalerPrm->ucVdoFmt, prDownScalerPrm->fgMbaff, \
                                             prDownScalerPrm->u4OffY, prDownScalerPrm->u4OffC);
#else
        vVDSCLSetSrcOffset(u4VDecID, prDownScalerPrm->u4SrcYOffH, prDownScalerPrm->u4SrcYOffV, prDownScalerPrm->u4SrcCOffH, prDownScalerPrm->u4SrcCOffV);

        vVDSCLSetScalerVParm(u4VDecID, prDownScalerPrm->u4SrcHeight, prDownScalerPrm->u4TrgHeight, \
                                             prDownScalerPrm->ucVdoFmt, prDownScalerPrm->fgMbaff, \
                                             prDownScalerPrm->u4OffY, prDownScalerPrm->u4OffC, prDownScalerPrm);
#endif
        
        if (prDownScalerPrm->fgEnColorCvt)
        {
            vVDSCLSetConvert(u4VDecID, TRUE);
        }
        else
        {
            vVDSCLSetConvert(u4VDecID, FALSE);
        }
        
        switch (prDownScalerPrm->ucPicStruct)
        {
            case TOP_FIELD:
                vVDSCLSetPicStruct(u4VDecID, PIC_STRUCT_TOP);
                break;
            case BOTTOM_FIELD:
                vVDSCLSetPicStruct(u4VDecID, PIC_STRUCT_BOTTOM);
                break;
            case FRAME:
                vVDSCLSetPicStruct(u4VDecID, PIC_STRUCT_FRAME);
                break;
            default:
                vVDSCLSetPicStruct(u4VDecID, PIC_STRUCT_TOP_BOTTOM);
                break;
        }

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8550)
       vVDSCLSetLumaKey(u4VDecID, prDownScalerPrm->fgLumaKeyEn, prDownScalerPrm->u2LumaKeyValue);
#endif  
    }  
  
#endif
    return;
}


#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560)
void vVDECSetLetetrBoxDetPrm(UINT32 u4VDecID, VDEC_INFO_LBD_PRM_T *prLBDPrm)
{
    vVDecWriteDV(u4VDecID, RW_VEC_LBOX_UP_CFG , (prLBDPrm->u4UpBoundEnd << 16) | (prLBDPrm->u4UpBoundStart));
    vVDecWriteDV(u4VDecID, RW_VEC_LBOX_LOW_CFG , (prLBDPrm->u4LowBoundStart << 16) | (prLBDPrm->u4LowBoundEnd));
    vVDecWriteDV(u4VDecID, RW_VEC_LBOX_SIDE_CFG , (prLBDPrm->u4LeftBound << 16) | (prLBDPrm->u4RightBound));
    vVDecWriteDV(u4VDecID, RW_VEC_LBOX_THD_OFFSET, (prLBDPrm->u4YOffset << 24) | (prLBDPrm->u4COffset << 16)
                                                                                                                       | (prLBDPrm->u4YValueThd << 8) | (prLBDPrm->u4CValueThd << 0));
    vVDecWriteDV(u4VDecID, RW_VEC_LBOX_CNT_THD, (prLBDPrm->u4YCntThd << 16) | (prLBDPrm->u4CCntThd));
}


void vVDECReadLetetrBoxDetResult(UINT32 u4VDecID, UINT32 *u4YResult, UINT32 *u4CResult)
{
    *u4YResult = u4VDecReadDV(u4VDecID, RO_VEC_LBOX_Y_LINE);
    *u4CResult = u4VDecReadDV(u4VDecID, RO_VEC_LBOX_C_LINE);
}
#endif


#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560) && (!CONFIG_DRV_FPGA_BOARD))
void vVDecSetVldMcClk(UINT32 u4VDecID,UINT32 u4CodecType)
{
    
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    vVDecWriteVLD(u4VDecID, WO_VLD_SRST, (0x1 |(0x1<<8)));
#else
    vVDecWriteVLD(u4VDecID, WO_VLD_SRST, 1);
#endif
    switch(u4CodecType)
    {
///TODO: set clock
/*
   #if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8580)
        case VDEC_H264:
        case VDEC_MPEG2:
        case VDEC_MPEG4:
        case VDEC_DIVX3:
        case VDEC_WMV:
        case VDEC_VP8:
        case VDEC_VC1:
            CKGEN_AgtSelClk(e_CLK_VDEC, CLK_CFG1_VDEC_SEL_27M);
            CKGEN_WRITE32(REG_RW_CLK_CFG8, (CKGEN_REG32(REG_RW_CLK_CFG8)&0xFFFFFFF8) | 0x07);
            break;
        case VDEC_VP6:
        case VDEC_RM:
        case VDEC_AVS:
            CKGEN_AgtSelClk(e_CLK_VDEC, CLK_CFG1_VDEC_SEL_SYSPLL1_1_2);
            break;
        default:
            CKGEN_AgtSelClk(e_CLK_VDEC, CLK_CFG1_VDEC_SEL_SYSPLL1_1_2);
            break;
   #else
        case VDEC_H264:
        case VDEC_AVS:
        case VDEC_MPEG2:
        case VDEC_RM:
            CKGEN_AgtSelClk(e_CLK_VDEC, CLK_CLK_VDEC_SEL_SYSPLL2_1_2);
            break;
        case VDEC_MPEG4:
        case VDEC_WMV:
        case VDEC_VP8:
        case VDEC_DIVX3:
            CKGEN_AgtSelClk(e_CLK_VDEC, CLK_CLK_VDEC_SEL_USBPLL_240M);
            break;
        default:
            CKGEN_AgtSelClk(e_CLK_VDEC, CLK_CLK_VDEC_SEL_SYSPLL2_1_2);
            break;
   #endif
   */
    }

///TODO: set clock
/*
   #if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8580)
//  CKGEN_AgtSelClk(e_CLK_MC,   CLK_CFG1_MC_SEL_SYSPLL2_1_2); //no need set MC Clk in MT8580
   #elif (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8560)
    CKGEN_AgtSelClk(e_CLK_MC,   CLK_CLK_MC_SEL_SYSPLL2_1_2);
   #else
    CKGEN_AgtSelClk(e_CLK_MC,   CLK_CLK_MC_SEL_SYSPLL2_1_2);
   #endif
   */
//   printk("clk cfg1 = 0x%x,cfg8 = %x\n",u4ReadReg(0x74)&0x07,u4ReadReg(0x9c)&0x07);
    vVDecWriteVLD(u4VDecID, WO_VLD_SRST, 0);
}
#endif

#ifdef VDEC_SIM_DUMP

#define DUMP_FOR_SIM 1

void vVDecSimWDump(const char * szFunction,UINT32 u4VDecID,const char * szOffset,UINT32 u4Addr,UINT32 u4Val, UINT32 u4BaseReg)
{
    if (szFunction)
    {
        msleep(1);
        if (DUMP_FOR_SIM)
        {
            //printk("RISCWrite(`%s + 4*%d, 32'h%x); /* 0x%08X */\n", szOffset, u4Addr>>2, u4Val, u4BaseReg);
            //printk("RISCWrite(`%s + 4*%d, 32'h%x);\n", szOffset, u4Addr>>2, u4Val);
            #ifdef REG_LOG_NEW
            if (_fgRegLogConsole[u4VDecID])
            #endif
            {
                switch (u4BaseReg)
                {
                case DV_REG_OFFSET0:
                    //printk("RISCWrite_VDEC_TOP( %d, 32'h%x);\n", u4Addr >> 2, u4Val);
                    DBG_H264_PRINTF("RISCWrite_MISC( %d, 32'h%x);\n", u4Addr >> 2, u4Val);
                    break;
                case VLD_REG_OFFSET0:
                    //printk("RISCWrite_VLD( %d, 32'h%x);\n", u4Addr >> 2, u4Val);
                    DBG_H264_PRINTF("RISCWrite_VLD( %d, 32'h%x);\n", u4Addr >> 2, u4Val);
                    break;
                case VLD_TOP_REG_OFFSET0:
                    //printk("RISCWrite_VLD_TOP( %d, 32'h%x);\n", u4Addr >> 2, u4Val);
                    DBG_H264_PRINTF("RISCWrite_VLD_TOP( %d, 32'h%x);\n", u4Addr >> 2, u4Val);
                    break;
                case MC_REG_OFFSET0:
                    //printk("RISCWrite_MC( %d, 32'h%x);\n", u4Addr >> 2, u4Val);
                    DBG_H264_PRINTF("RISCWrite_MC( %d, 32'h%x);\n", u4Addr >> 2, u4Val);
                    break;
                case AVC_MV_REG_OFFSET0:
                    //printk("RISCWrite_MV( %d, 32'h%x);\n", u4Addr >> 2, u4Val);
                    DBG_H264_PRINTF("RISCWrite_AVC_MV( %d, 32'h%x);\n", u4Addr >> 2, u4Val);
                    break;
                case RM_VDEC_PP_BASE:
                    //printk("RISCWrite_PP( %d, 32'h%x);\n", u4Addr >> 2, u4Val);
                    DBG_H264_PRINTF("RISCWrite_PP( %d, 32'h%x);\n", u4Addr >> 2, u4Val);
                    break;
                default:
                    //printk("RISCWrite_Others( %d, 32'h%x); /* base 0x%x */\n", u4Addr >> 2, u4Val, u4BaseReg);
                    DBG_H264_PRINTF("RISCWrite_Others( %d, 32'h%x); /* base 0x%x */\n", u4Addr >> 2, u4Val, u4BaseReg);
                    break;
                }
            }            
            #ifdef REG_LOG_NEW
            else
            {
                switch (u4BaseReg)
                {
                case DV_REG_OFFSET0:
                    _u4RegisterLogLen[u4VDecID] += sprintf(_pucRegisterLog[u4VDecID]+_u4RegisterLogLen[u4VDecID], "RISCWrite_VDEC_TOP( %d, 32'h%x);\n", u4Addr >> 2, u4Val);
                    break;
                case VLD_REG_OFFSET0:
                    _u4RegisterLogLen[u4VDecID] += sprintf(_pucRegisterLog[u4VDecID]+_u4RegisterLogLen[u4VDecID], "RISCWrite_VLD( %d, 32'h%x);\n", u4Addr >> 2, u4Val);
                    break;
                case VLD_TOP_REG_OFFSET0:
                    _u4RegisterLogLen[u4VDecID] += sprintf(_pucRegisterLog[u4VDecID]+_u4RegisterLogLen[u4VDecID], "RISCWrite_VLD_TOP( %d, 32'h%x);\n", u4Addr >> 2, u4Val);
                    break;
                case MC_REG_OFFSET0:
                    _u4RegisterLogLen[u4VDecID] += sprintf(_pucRegisterLog[u4VDecID]+_u4RegisterLogLen[u4VDecID], "RISCWrite_MC( %d, 32'h%x);\n", u4Addr >> 2, u4Val);
                    break;
                case AVC_MV_REG_OFFSET0:
                    _u4RegisterLogLen[u4VDecID] += sprintf(_pucRegisterLog[u4VDecID]+_u4RegisterLogLen[u4VDecID], "RISCWrite_MV( %d, 32'h%x);\n", u4Addr >> 2, u4Val);
                    break;
                case RM_VDEC_PP_BASE:
                    _u4RegisterLogLen[u4VDecID] += sprintf(_pucRegisterLog[u4VDecID]+_u4RegisterLogLen[u4VDecID], "RISCWrite_PP( %d, 32'h%x);\n", u4Addr >> 2, u4Val);
                    break;
                default:
                    _u4RegisterLogLen[u4VDecID] += sprintf(_pucRegisterLog[u4VDecID]+_u4RegisterLogLen[u4VDecID], "RISCWrite_Others( %d, 32'h%x); /* base 0x%x */\n", u4Addr >> 2, u4Val, u4BaseReg);
                    break;
                }
            }
            #endif
        }
        else
        {
            //printk("%s(u4VDecID, 0x%x, 0x%x); /* 0x%08X */\n", szFunction, u4Addr, u4Val, u4BaseReg);
            //printk("%s(u4VDecID, 0x%x, 0x%x);\n", szFunction, u4Addr, u4Val);
            DBG_H264_PRINTF("%s(u4VDecID, 0x%x, 0x%x);\n", szFunction, u4Addr, u4Val);
        }
    }
    else
    {
    }
    msleep(10);
}

void vVDecSimRDump(const char * szFunction,UINT32 u4VDecID,const char * szOffset,UINT32 u4Addr,UINT32 u4Val, UINT32 u4BaseReg)
{
    if (szFunction)
    {
        if (DUMP_FOR_SIM)
        {
            //printk("RISCRead(`%s + 4*%d, 32'h%x); /* 0x%08X */\n", szOffset, u4Addr>>2, u4Val, u4BaseReg);
            //printk("RISCRead(`%s + 4*%d, 32'h%x);\n", szOffset, u4Addr>>2, u4Val);
            #ifdef REG_LOG_NEW
            if (_fgRegLogConsole[u4VDecID])
            #endif
            {
                msleep(1);
                switch (u4BaseReg)
                {
                case DV_REG_OFFSET0:
                    //printk("RISCRead_VDEC_TOP( %d); /* return 0x%x */\n", u4Addr >> 2, u4Val);
                    DBG_H264_PRINTF("RISCRead_MISC( %d); /* return 0x%x */\n", u4Addr >> 2, u4Val);
                    break;
                case VLD_REG_OFFSET0:
                    //printk("RISCRead_VLD( %d); /* return 0x%x */\n", u4Addr >> 2, u4Val);
                    DBG_H264_PRINTF("RISCRead_VLD( %d); /* return 0x%x */\n", u4Addr >> 2, u4Val);
                    break;
                case VLD_TOP_REG_OFFSET0:
                    //printk("RISCRead_VLD_TOP( %d); /* return 0x%x */\n", u4Addr >> 2, u4Val);
                    DBG_H264_PRINTF("RISCRead_VLD_TOP( %d); /* return 0x%x */\n", u4Addr >> 2, u4Val);
                    break;
                case MC_REG_OFFSET0:
                    //printk("RISCRead_MC( %d); /* return 0x%x */\n", u4Addr >> 2, u4Val);
                    DBG_H264_PRINTF("RISCRead_MC( %d); /* return 0x%x */\n", u4Addr >> 2, u4Val);
                    break;
                case AVC_MV_REG_OFFSET0:
                    //printk("RISCRead_MV( %d); /* return 0x%x */\n", u4Addr >> 2, u4Val);
                    DBG_H264_PRINTF("RISCRead_MV( %d); /* return 0x%x */\n", u4Addr >> 2, u4Val);
                    break;
                case RM_VDEC_PP_BASE:
                    //printk("RISCRead_PP( %d); /* return 0x%x */\n", u4Addr >> 2, u4Val);
                    DBG_H264_PRINTF("RISCRead_PP( %d); /* return 0x%x */\n", u4Addr >> 2, u4Val);
                    break;
                case AVC_VLD_REG_OFFSET0:
                case AVC_VLD_REG_OFFSET1:
                    //if(u4Addr >= RW_AVLD_CTRL)
                    {
                        //printk("[VDEC]RISCWrite('VDEC_FULL_ADDR_AVC_VLD + 4*%d, 32'h%x); \n", u4Addr>>2, u4Val);
                        //printk("RISCRead_AVC_VLD( %d);  /* return 0x%x */\n", u4Addr>>2, u4Val);
                        DBG_H264_PRINTF("RISCRead_AVC_VLD( %d);  /* return 0x%x */\n", u4Addr>>2, u4Val);
                    }
                    if(u4Addr >= RW_AVLD_PROC)
                    {
                        //_u4SimDumpCnt ++;
                        //printk("[H264]@@ [VDEC]RISCRead_AVC_VLD( %d, 32'h%x); \n", u4Addr>>2, u4Val);
                        //printk("[H264]@@ Fantasia Something wrong?\n");
                    }
                break;
                default:
                    //printk("RISCRead_Others( %d);  /* return 0x%x */ /* base 0x%x */\n", u4Addr >> 2, u4Val, u4BaseReg);
                    DBG_H264_PRINTF("RISCRead_Others( %d);  /* return 0x%x */ /* base 0x%x */\n", u4Addr >> 2, u4Val, u4BaseReg);
                    break;
                }
            }
            #ifdef REG_LOG_NEW
            else
            {
                switch (u4BaseReg)
                {
                case DV_REG_OFFSET0:
                    _u4RegisterLogLen[u4VDecID] += sprintf(_pucRegisterLog[u4VDecID]+_u4RegisterLogLen[u4VDecID], "RISCRead_VDEC_TOP( %d); /* return 0x%x */\n", u4Addr >> 2, u4Val);
                    break;
                case VLD_REG_OFFSET0:
                    _u4RegisterLogLen[u4VDecID] += sprintf(_pucRegisterLog[u4VDecID]+_u4RegisterLogLen[u4VDecID], "RISCRead_VLD( %d); /* return 0x%x */\n", u4Addr >> 2, u4Val);
                    break;
                case VLD_TOP_REG_OFFSET0:
                    _u4RegisterLogLen[u4VDecID] += sprintf(_pucRegisterLog[u4VDecID]+_u4RegisterLogLen[u4VDecID], "RISCRead_VLD_TOP( %d); /* return 0x%x */\n", u4Addr >> 2, u4Val);
                    break;
                case MC_REG_OFFSET0:
                    _u4RegisterLogLen[u4VDecID] += sprintf(_pucRegisterLog[u4VDecID]+_u4RegisterLogLen[u4VDecID], "RISCRead_MC( %d); /* return 0x%x */\n", u4Addr >> 2, u4Val);
                    break;
                case AVC_MV_REG_OFFSET0:
                    _u4RegisterLogLen[u4VDecID] += sprintf(_pucRegisterLog[u4VDecID]+_u4RegisterLogLen[u4VDecID], "RISCRead_MV( %d); /* return 0x%x */\n", u4Addr >> 2, u4Val);
                    break;
                case RM_VDEC_PP_BASE:
                    _u4RegisterLogLen[u4VDecID] += sprintf(_pucRegisterLog[u4VDecID]+_u4RegisterLogLen[u4VDecID], "RISCRead_PP( %d); /* return 0x%x */\n", u4Addr >> 2, u4Val);
                    break;
                default:
                    _u4RegisterLogLen[u4VDecID] += sprintf(_pucRegisterLog[u4VDecID]+_u4RegisterLogLen[u4VDecID], "RISCRead_Others( %d);  /* return 0x%x */ /* base 0x%x */\n", u4Addr >> 2, u4Val, u4BaseReg);
                    break;
                }
            }
            
            #endif
        }
        else
        {
            //printk("%s(u4VDecID, 0x%x); /* return 0x%x */ /* 0x%08X */\n", szFunction, u4Addr, u4Val, u4BaseReg);
            printk("%s(u4VDecID, 0x%x); /* return 0x%x */\n", szFunction, u4Addr, u4Val);
        }
    }
    else
    {
    }
}



UINT32 _u4SimDumpCnt;
UINT32 _u4SimDumpMax;
void vVDecSimDump(UINT32 u4VDecID, UINT32 u4OffsetAddr, UINT32 u4Addr, UINT32 u4Val)
{
    /*
    if(_u4SimDumpCnt > _u4SimDumpMax)
    {
        return;
    }
    */
    switch(u4OffsetAddr)
    {
     case VLD_REG_OFFSET0:
     case VLD_REG_OFFSET1:
         //printk("[VDEC]RISCWrite('VDEC_FULL_ADDR_VLD + 4*%d, 32'h%x); \n", u4Addr>>2, u4Val);
         DBG_H264_PRINTF("[VDEC]RISCWrite('VDEC_FULL_ADDR_VLD + 4*%d, 32'h%x); \n", u4Addr>>2, u4Val);         
            break;

     #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
     case VLD_TOP_REG_OFFSET0:
     case VLD_TOP_REG_OFFSET1:
            //printk("[VDEC]RISCWrite('VDEC_FULL_ADDR_VLD_TOP + 4*%d, 32'h%x); \n", u4Addr>>2, u4Val);
            DBG_H264_PRINTF("[VDEC]RISCWrite('VDEC_FULL_ADDR_VLD_TOP + 4*%d, 32'h%x); \n", u4Addr>>2, u4Val);
            break;
     #endif
     
        case MC_REG_OFFSET0:
        case MC_REG_OFFSET1:
            //printk("[VDEC]RISCWrite('VDEC_FULL_ADDR_MC + 4*%d, 32'h%x); \n", u4Addr>>2, u4Val);
            DBG_H264_PRINTF("[VDEC]RISCWrite('VDEC_FULL_ADDR_MC + 4*%d, 32'h%x); \n", u4Addr>>2, u4Val);
            break;

        #if 0
        case VDEC_CRC_REG_OFFSET0:
            printk("[VDEC]RISCWrite('VDEC_FULL_ADDR_CRC + 4*%d, 32'h%x); \n", u4Addr>>2, u4Val);
            break;
        #endif
            
        case DV_REG_OFFSET0:
        case DV_REG_OFFSET1:
            DBG_H264_PRINTF("[VDEC]RISCWrite('VDEC_FULL_ADDR_DV + 4*%d, 32'h%x); \n", u4Addr>>2, u4Val);
            break;
            
        case AVC_VLD_REG_OFFSET0:
        case AVC_VLD_REG_OFFSET1:
            if(u4Addr >= RW_AVLD_CTRL)
            {
                //printk("[VDEC]RISCWrite('VDEC_FULL_ADDR_AVC_VLD + 4*%d, 32'h%x); \n", u4Addr>>2, u4Val);
                //printk("RISCWrite_AVC_VLD( %d, 32'h%x); \n", u4Addr>>2, u4Val);
                DBG_H264_PRINTF("RISCWrite_AVC_VLD( %d, 32'h%x); \n", u4Addr>>2, u4Val);                
            }
            if(u4Addr >= RW_AVLD_PROC)
            {
                _u4SimDumpCnt ++;
            }
            break;
        case AVC_MV_REG_OFFSET0:
 //       case AVC_MV_REG_OFFSET1:
            //printk("[VDEC]RISCWrite('VDEC_FULL_ADDR_AVC_MV + 4*%d, 32'h%x); \n", u4Addr>>2, u4Val);
            DBG_H264_PRINTF("[VDEC]RISCWrite('VDEC_FULL_ADDR_AVC_MV + 4*%d, 32'h%x); \n", u4Addr>>2, u4Val);
            break;

 //       case AVC_FG_REG_OFFSET0:
 //       case AVC_FG_REG_OFFSET1:
            //printk("[VDEC]RISCWrite('VDEC_FULL_ADDR_FG + 4*%d, 32'h%x); \n", u4Addr>>2, u4Val);
            DBG_H264_PRINTF("[VDEC]RISCWrite('VDEC_FULL_ADDR_FG + 4*%d, 32'h%x); \n", u4Addr>>2, u4Val);
            break;
    }
    msleep(4);  //In wait state
}

void vVDecSimDumpAvcVldR(UINT32 u4VDecID, UINT32 u4OffsetAddr, UINT32 u4Addr, UINT32 u4Val)
{
    /* fantasia 2012-03-06
    if(_u4SimDumpCnt > _u4SimDumpMax)
    {
        return;
    }
    */    
    //fantasia 2012-03-07 to limit the log output
    if((_u4FileCnt[u4VDecID] != _u4DumpRegPicNum[u4VDecID]) && (_u4FileCnt[u4VDecID] != (_u4DumpRegPicNum[u4VDecID]-1))&& (_u4FileCnt[u4VDecID] != (_u4DumpRegPicNum[u4VDecID]+1)))
    {
        #if _LOG_FANTASIA_1_
        printk("[H264] fantasia >> Current frame %d dump frame %d \n", _u4FileCnt[u4VDecID], _u4DumpRegPicNum[u4VDecID]);
        #endif
        return;
    }    
    //msleep(5);
    switch(u4OffsetAddr)
    {
     case VLD_REG_OFFSET0:
     case VLD_REG_OFFSET1:
         //printk("RISCRead('VDEC_FULL_ADDR_VLD + 4*%d, 32'h%x); \n", u4Addr>>2, u4Val);
         DBG_H264_PRINTF("RISCRead('VDEC_FULL_ADDR_VLD + 4*%d, 32'h%x); \n", u4Addr>>2, u4Val);
            break;

     #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
     case VLD_TOP_REG_OFFSET0:
     case VLD_TOP_REG_OFFSET1:
            //printk("RISCRead('VDEC_FULL_ADDR_VLD_TOP + 4*%d, 32'h%x); \n", u4Addr>>2, u4Val);
            DBG_H264_PRINTF("RISCRead('VDEC_FULL_ADDR_VLD_TOP + 4*%d, 32'h%x); \n", u4Addr>>2, u4Val);
            break;
     #endif
     
        case MC_REG_OFFSET0:
        case MC_REG_OFFSET1:
            //printk("RISCRead('VDEC_FULL_ADDR_MC + 4*%d, 32'h%x); \n", u4Addr>>2, u4Val);
            DBG_H264_PRINTF("RISCRead('VDEC_FULL_ADDR_MC + 4*%d, 32'h%x); \n", u4Addr>>2, u4Val);
            break;

        #if 0
        case VDEC_CRC_REG_OFFSET0:
            printk("[VDEC]RISCRead('VDEC_FULL_ADDR_CRC + 4*%d, 32'h%x); \n", u4Addr>>2, u4Val);
            break;
        #endif
            
        case DV_REG_OFFSET0:
        case DV_REG_OFFSET1:
            //printk("RISCRead('VDEC_FULL_ADDR_DV + 4*%d, 32'h%x); \n", u4Addr>>2, u4Val);
            DBG_H264_PRINTF("RISCRead('VDEC_FULL_ADDR_DV + 4*%d, 32'h%x); \n", u4Addr>>2, u4Val);
            break;
            
        case AVC_VLD_REG_OFFSET0:
        case AVC_VLD_REG_OFFSET1:
            if(u4Addr >= RW_AVLD_CTRL)
            {
                //printk("[VDEC]RISCRead('VDEC_FULL_ADDR_AVC_VLD + 4*%d, 32'h%x); \n", u4Addr>>2, u4Val);
                //printk("RISCRead_AVC_VLD(%d, 32'h%x); \n", u4Addr>>2, u4Val);
                DBG_H264_PRINTF("RISCRead_AVC_VLD(%d, 32'h%x); \n", u4Addr>>2, u4Val);
            }
            if(u4Addr >= RW_AVLD_PROC)
            {
                _u4SimDumpCnt ++;
            }
            break;
        case AVC_MV_REG_OFFSET0:
 //       case AVC_MV_REG_OFFSET1:
             //printk("RISCRead('VDEC_FULL_ADDR_AVC_MV + 4*%d, 32'h%x); \n", u4Addr>>2, u4Val);
             DBG_H264_PRINTF("RISCRead('VDEC_FULL_ADDR_AVC_MV + 4*%d, 32'h%x); \n", u4Addr>>2, u4Val);
            break;

 //       case AVC_FG_REG_OFFSET0:
 //       case AVC_FG_REG_OFFSET1:
            //printk("RISCRead('VDEC_FULL_ADDR_FG + 4*%d, 32'h%x); \n", u4Addr>>2, u4Val);
            DBG_H264_PRINTF("RISCRead('VDEC_FULL_ADDR_FG + 4*%d, 32'h%x); \n", u4Addr>>2, u4Val);
            break;
    }
    msleep(4);  //In wait state
}

#endif

