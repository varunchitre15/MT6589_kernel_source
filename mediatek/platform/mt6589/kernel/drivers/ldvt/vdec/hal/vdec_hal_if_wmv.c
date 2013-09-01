#include "vdec_hw_common.h"
#include "vdec_hal_if_wmv.h"
#include "vdec_hal_errcode.h"
//#include "x_hal_ic.h"
//#include "x_hal_1176.h"
//#include "x_debug.h"
#include <asm/system.h>

#define MC_LOWCOST_PERORMANCE 0

#if CONFIG_DRV_VERIFY_SUPPORT
#include "../verify/vdec_verify_general.h"
#include "../verify/vdec_verify_mpv_prov.h"
#include <linux/string.h>
#include <linux/delay.h>

#if (!CONFIG_DRV_LINUX)
#include <stdio.h>
#include <string.h>
#endif

#if VMMU_SUPPORT
#include "vdec_hal_if_common.h"
#endif

extern void vVDecOutputDebugString(const CHAR * format, ...);
extern BOOL fgWrMsg2PCEx(void* pvAddr, UINT32 u4Size, UINT32 u4Mode, VDEC_INFO_VERIFY_FILE_INFO_T *pFILE_INFO, const char *szFunction, INT32 i4Line);
#define  fgWrMsg2PC(pvAddr, u4Size, u4Mode, pFILE_INFO)  fgWrMsg2PCEx(pvAddr, u4Size, u4Mode, pFILE_INFO, __FUNCTION__, __LINE__)

#endif
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8555)
extern void vVDecWriteAVCMV(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadAVCMV(UINT32 u4VDecID, UINT32 u4Addr);
#endif
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8550)
void vVDecWriteWMVDCAC(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
    if (u4VDecID == 0)
    {
        vWriteReg(WMV_DCAC_REG_OFFSET0 + u4Addr, u4Val);
        vVDecSimDumpW(u4VDecID, WMV_DCAC_REG_OFFSET0, u4Addr, u4Val);
    }
    else//temp add for mt8555 verify   @<Youlin.Pei
    {
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8555)
        vWriteReg(WMV_DCAC_REG_OFFSET1 + u4Addr, u4Val);
    vVDecSimDumpW(u4VDecID, WMV_DCAC_REG_OFFSET1, u4Addr, u4Val);
#else
        printk("[VDEC]WMV: Wrong VDecDecID\n");
        //return 0;
#endif
    }
}
#endif

#if VDEC_REMOVE_UNUSED_FUNC
UINT32 u4VDecReadWMVDCAC(UINT32 u4VDecID, UINT32 u4Addr)
{
    if (u4VDecID == 0)
    {
        return (u4ReadReg(WMV_DCAC_REG_OFFSET0 + u4Addr));
    }
    else
    {
       return (u4ReadReg(WMV_DCAC_REG_OFFSET1 + u4Addr));
       //printk("[VDEC]WMV: Wrong VDecDecID\n");
       //return 0;
    }
}
#endif

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8550)
void vVDecWriteWMVMV(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
    if (u4VDecID == 0)
    {
        vWriteReg(WMV_MV_REG_OFFSET0 + u4Addr, u4Val);
        vVDecSimDumpW(u4VDecID, WMV_MV_REG_OFFSET0, u4Addr, u4Val);
    }        
    else//temp add for mt8555 verify   @<Youlin.Pei
    {
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8555)
        vWriteReg(WMV_MV_REG_OFFSET1 + u4Addr, u4Val);
        vVDecSimDumpW(u4VDecID, WMV_MV_REG_OFFSET1, u4Addr, u4Val);
#else
        //printk("[VDEC]WMV: Wrong VDecDecID\n");
        //return 0;
#endif
    }
}
#endif

#if VDEC_REMOVE_UNUSED_FUNC
UINT32 u4VDecReadWMVMV(UINT32 u4VDecID, UINT32 u4Addr)
{
    if (u4VDecID == 0)
    {
        return (u4ReadReg(WMV_MV_REG_OFFSET0 + u4Addr));
    }
    else
    {
        return (u4ReadReg(WMV_MV_REG_OFFSET1 + u4Addr));
        //printk("[VDEC]WMV: Wrong VDecDecID\n");
        //return 0;
    }
}
#endif

// **************************************************************************
// Function : INT32 i4VDEC_HAL_WMV_InitVDecHW(UINT32 u4Handle, VDEC_INFO_WMV_VFIFO_PRM_T *prWmvVFifoInitPrm);
// Description :Initialize video decoder hardware only for WMV
// Parameter : u4VDecID : video decoder hardware ID
//                  prWmvVFifoInitPrm : pointer to VFIFO info struct
// Return      : =0: success.
//                  <0: fail.
// **************************************************************************
INT32 i4VDEC_HAL_WMV_InitVDecHW(UINT32 u4VDecID, VDEC_INFO_WMV_VFIFO_PRM_T *prWmvVFifoInitPrm)
{
#if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8555)
    vVDecResetHW(u4VDecID);
#elif (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560)
    vVDecResetHW(u4VDecID, prWmvVFifoInitPrm->u4CodeType);
#else
    vVDecResetHW(u4VDecID, VDEC_UNKNOWN);
#endif
    #if (WMV_8320_SUPPORT)
    vVDecWriteVLD(u4VDecID, RW_VLD_WMV_MODE, 0x00000001);
    #endif
    if (prWmvVFifoInitPrm->u4CodeType == VDEC_VC1)
    {
        //Ginny: Enable 0x03 replancement before initializing barrel shifter.
        vVDecWriteVLD(u4VDecID, RW_VLD_WMV_ABS, 0x1);
    }
    else
    {
       vVDecWriteVLD(u4VDecID, RW_VLD_WMV_ABS, 0x0);
    }
   
    vVDecSetVLDVFIFO(0, u4VDecID, PHYSICAL((UINT32) prWmvVFifoInitPrm->u4VFifoSa), PHYSICAL((UINT32) prWmvVFifoInitPrm->u4VFifoEa));

    return HAL_HANDLE_OK;
}

// **************************************************************************
// Function : UINT32 u4VDEC_HAL_WMV_ShiftGetBitStream(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits);
// Description :Read barrel shifter after shifting
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
//                 u4ShiftBits : shift bits number
// Return      : Value of barrel shifter input window after shifting
// **************************************************************************
UINT32 u4VDEC_HAL_WMV_ShiftGetBitStream(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits)
{
    UINT32 dRegVal;
  
    dRegVal = u4VDecVLDGetBitS(u4BSID, u4VDecID, u4ShiftBits);
    
    return(dRegVal);
}

// **************************************************************************
// Function : UINT32 u4VDEC_HAL_WMV_GetBitStreamShift(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits);
// Description :Read Barrel Shifter before shifting
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
//                 u4ShiftBits : shift bits number
// Return      : Value of barrel shifter input window before shifting
// **************************************************************************
#if VDEC_REMOVE_UNUSED_FUNC
UINT32 u4VDEC_HAL_WMV_GetBitStreamShift(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits)
{
    UINT32 u4RegVal0;
  
    u4RegVal0 = u4VDecVLDGetBitS(u4BSID, u4VDecID, 0);
    u4VDecVLDGetBitS(u4BSID, u4VDecID, u4ShiftBits);
    
    return(u4RegVal0);
}
#endif

// **************************************************************************
// Function : UINT32 u4VDEC_HAL_WMV_GetRealBitStream(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits);
// Description :Read Barrel Shifter before shifting
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
//                 u4ShiftBits : shift bits number
// Return      : Most significant (32 - u4ShiftBits) bits of barrel shifter input window before shifting
// **************************************************************************
#if 0 //(WMV_8320_SUPPORT)
#include <linux/dma-mapping.h>
UINT32 _dRegValRight = 0x4600000a;
UINT32 _dRegValWrong = 0x4600005a;
#endif
UINT32 u4VDEC_HAL_WMV_GetRealBitStream(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits)
{
    UINT32 dRegVal0;
    UINT32 u4RetVal = 0;
  
    dRegVal0 = u4VDecVLDGetBitS(u4BSID, u4VDecID, 0);
    
    u4VDecVLDGetBitS(u4BSID, u4VDecID, u4ShiftBits);

    u4RetVal = (dRegVal0 >> (32-u4ShiftBits));
    
    #if WMV_LOG_TMP
    printk("GetBS: sh:%d, shfit0 val:0x%x, return:0x%x\n", u4ShiftBits, dRegVal0, u4RetVal);
    #endif
    
    #if 0 //(WMV_8320_SUPPORT)
    if (_dRegValRight == dRegVal0)
    {
      printk("WMV_GetRealBitStream: 1st right!!\n");
    }
    else if (_dRegValWrong == dRegVal0)
    {
      // get address, vdec 0
      UINT32 u4Addr = IO_BASE + VLD_REG_OFFSET0 + RO_VLD_BARL + (u4ShiftBits << 2); 
      //struct device rDev;

      printk("WMV_GetRealBitStream: addr:0x%x, 1st wrong!!\n", u4Addr);
    }     
    #endif
      
    return u4RetVal;
}

// **************************************************************************
// Function : INT32 i4VDEC_HAL_WMV_InitBarrelShifter(UINT32 u4BSID, UINT32 u4VDecID, VDEC_INFO_WMV_BS_INIT_PRM_T *prWmvBSInitPrm, BOOL fgIsVC1);
// Description :Initialize barrel shifter with byte alignment
// Parameter : u4ReadPointer : set read pointer value
//                 u4WrtePointer : set write pointer value
//                 u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
// Return      : =0: success.
//                  <0: fail.
// **************************************************************************
INT32 i4VDEC_HAL_WMV_InitBarrelShifter(UINT32 u4BSID, UINT32 u4VDecID, VDEC_INFO_WMV_BS_INIT_PRM_T *prWmvBSInitPrm, BOOL fgIsVC1)
{
    BOOL fgFetchOK = FALSE;
    #if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8580) 
    INT32 i;
    #endif
    INT32 j;
    UINT32 u4VLDByte, u4VLDBit;
    UINT32 u4VLDRemainByte;
  
#if (CONFIG_DRV_VERIFY_SUPPORT) && (CONFIG_DRV_LINUX)    
//    HalFlushInvalidateDCache();
#endif

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580) 

    #ifdef VDEC_SIM_DUMP
    printk("if(`VDEC_PROCESS_FLAG == 1) wait(`VDEC_BITS_PROC_NOP == 1);\n");
    #endif

    //polling
   if((u4VDecReadVLD(u4VDecID,RO_VLD_SRAMCTRL) & PROCESS_FLAG))
      while (!(u4VDecReadVLD(u4VDecID,RO_VLD_SRAMCTRL) & 1));

    vVDecSetVLDVFIFO(u4BSID, u4VDecID, PHYSICAL((UINT32) prWmvBSInitPrm->u4VFifoSa), PHYSICAL((UINT32) prWmvBSInitPrm->u4VFifoEa));
    
    u4VLDRemainByte =  ((prWmvBSInitPrm->u4ReadPointer)) & 0xf;
    

     vVDecWriteVLD(u4VDecID, RW_VLD_RPTR + (u4BSID << 10), PHYSICAL((UINT32) prWmvBSInitPrm->u4ReadPointer));
     vVDecWriteVLD(u4VDecID, WO_VLD_WPTR + (u4BSID << 10),PHYSICAL((UINT32) prWmvBSInitPrm->u4WritePointer));

      //Reset async fifo for mt8580 new design
      vVDecWriteVLD(u4VDecID, WO_VLD_SRST , (0x1 << 8));
      vVDecWriteVLD(u4VDecID, WO_VLD_SRST , 0);

       vVDecWriteVLD(u4VDecID, 4*51, 1000); //sun for temp test
       
    #if (WMV_8320_SUPPORT)
    {
      UINT32 u4VldVal = u4VDecReadVLD(u4VDecID, RW_VLD_LDSH);
      
      #if WMV_LOG_TMP
        printk("InitBarrelShifter, Reg 0x1D0:0x%x\n", u4VldVal);
      #endif

      u4VldVal |= (1 << 8);
        
      vVDecWriteVLD(u4VDecID, RW_VLD_LDSH, u4VldVal); // for 8320
    }
    #endif
      // start to fetch data
      vVDecWriteVLD(u4VDecID, RW_VLD_PROC + (u4BSID << 10), VLD_INIFET);
    
    if (fgVDecWaitVldFetchOk(u4BSID, u4VDecID)) {
        fgFetchOK = TRUE;
    }
      
#else
    vVDecWriteVLD(u4VDecID,RW_VLD_RDY_SWTICH + (u4BSID << 10), READY_TO_RISC_1);
    
    vVDecSetVLDVFIFO(u4BSID, u4VDecID, PHYSICAL((UINT32) prWmvBSInitPrm->u4VFifoSa), PHYSICAL((UINT32) prWmvBSInitPrm->u4VFifoEa));
    
    u4VLDRemainByte =  ((prWmvBSInitPrm->u4ReadPointer)) & 0xf;
  
    // prevent initialize barrel fail
    for (i = 0; i < 5; i++)  //mark
    {
        vVDecWriteVLD(u4VDecID, WO_VLD_WPTR, u4VDecReadVLD(u4VDecID, WO_VLD_WPTR) | VLD_CLEAR_PROCESS_EN);
        vVDecWriteVLD(u4VDecID, RW_VLD_RPTR + (u4BSID << 10), PHYSICAL((UINT32) prWmvBSInitPrm->u4ReadPointer));
        vVDecWriteVLD(u4VDecID, RW_VLD_RPTR + (u4BSID << 10), PHYSICAL((UINT32) prWmvBSInitPrm->u4ReadPointer));
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)  
    vVDecWriteVLD(u4VDecID, WO_VLD_WPTR + (u4BSID << 10),PHYSICAL((UINT32) prWmvBSInitPrm->u4WritePointer));
    vVDecWriteVLD(u4VDecID, RW_VLD_ASYNC + (u4BSID << 10), u4VDecReadVLD(u4VDecID, RW_VLD_ASYNC) | VLD_WR_ENABLE);
#else
        vVDecWriteVLD(u4VDecID, WO_VLD_WPTR + (u4BSID << 10),((PHYSICAL((UINT32) prWmvBSInitPrm->u4WritePointer) << 4) | 0x2));
#endif
        #if (WMV_8320_SUPPORT)
        {
          UINT32 u4VldVal = u4VDecReadVLD(u4VDecID, RW_VLD_LDSH);
          
          #if WMV_LOG_TMP
            printk("InitBarrelShifter, Reg 0x1D0:0x%x\n", u4VldVal);
          #endif

          u4VldVal |= (1 << 8)
            
          vVDecWriteVLD(u4VDecID, RW_VLD_LDSH, u4VldVal); // for 8320
        }
        #endif
        // start to fetch data
        vVDecWriteVLD(u4VDecID, RW_VLD_PROC + (u4BSID << 10), VLD_INIFET);
    
        if (fgVDecWaitVldFetchOk(u4BSID, u4VDecID)) {
            fgFetchOK = TRUE;
            break;
        }
    }
 #endif
  
    if (!fgFetchOK) {
        return(INIT_BARRELSHIFTER_FAIL);
    }
  
    vVDecWriteVLD(u4VDecID, RW_VLD_PROC + (u4BSID << 10), VLD_INIBR);
  
    if (fgIsVC1) {
        u4VLDRemainByte -= u4VDec03Number((prWmvBSInitPrm->u4ReadPointer)& (0xFFFFFFF0), u4VLDRemainByte);
    }
    
    //ming add for 03 issue:begin
    for ( j=0; j<u4VLDRemainByte; j++) {
        u4VDecVLDGetBitS(u4BSID, u4VDecID, 8);
        u4VLDByte = u4VDecReadVldRPtr(u4BSID, u4VDecID, &u4VLDBit, PHYSICAL((UINT32) prWmvBSInitPrm->u4VFifoSa));
        if (u4VLDByte >= (prWmvBSInitPrm->u4ReadPointer - prWmvBSInitPrm->u4VFifoSa)) {
            break;
        }
    }
    //ming add for 03 issue:end
  
    u4VLDByte = u4VDecReadVldRPtr(u4BSID, u4VDecID, &u4VLDBit, PHYSICAL((UINT32) prWmvBSInitPrm->u4VFifoSa));

    ///u4VLDByte + input window
    #if WMV_LOG_TMP
    printk("i4VDEC_HAL_WMV_InitBarrelShifter, rd:0x%x, input window:0x%x, 0x%x\n", 
      u4VLDByte, u4VDecVLDGetBitS(0, 0, 0), u4VDecReadVLD(u4VDecID, 0xf0));
    #endif
  
    return HAL_HANDLE_OK;
}

// **************************************************************************
// Function : UINT32 u4VDEC_HAL_WMV_ReadRdPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4VFIFOSa, UINT32 *pu4Bits);
// Description :Read current read pointer
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
//                 u4VFIFOSa : video FIFO start address
//                 pu4Bits : read pointer value with remained bits
// Return      : Read pointer value with byte alignment
// **************************************************************************
UINT32 u4VDEC_HAL_WMV_ReadRdPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4VFIFOSa, UINT32 *pu4Bits)
{
    return u4VDecReadVldRPtr(u4BSID, u4VDecID, pu4Bits, PHYSICAL((UINT32) u4VFIFOSa));
}

// **************************************************************************
// Function : void v4VDEC_HAL_WMV_AlignRdPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4VFIFOSa, UINT32 u4AlignType);
// Description :Align read pointer to byte,word or double word
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
//                 u4VFIFOSa : video FIFO start address
//                 u4AlignType : read pointer align type
// Return      : None
// **************************************************************************
void vVDEC_HAL_WMV_AlignRdPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4VFIFOSa, UINT32 u4AlignType)
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
// Function : INT32 i4VDEC_HAL_WMV_HWDecBP(UINT32 u4VDecID, UINT32 u4BpNum, VDEC_INFO_WMV_DEC_BP_PRM_T *prWmvDecBpPrm);
// Description :Set HW to decode bit plane
// Parameter : u4VDecID : VLD ID
//                  u4BpNum : Bit plane type
//                  prWmvDecBpPrm : Pointer to wmv bit plane decoding information struct
// Return      : =0: success.
//                  <0: fail.
// **************************************************************************
INT32 i4VDEC_HAL_WMV_HWDecBP(UINT32 u4VDecID, UINT32 u4BpNum, VDEC_INFO_WMV_DEC_BP_PRM_T *prWmvDecBpPrm)
{
    printk("i4VDEC_HAL_WMV_HWDecBP start\n");
    INT32 i, a = 0;
    INT32 vop_type;
#if 0
    UINT32 u4InputWindow = 0; //sun for temp
    UINT32 u4Bits = 0;
    UINT32  u4VLDByte =  u4VDEC_HAL_WMV_ReadRdPtr(0, u4VDecID, (UINT32)_pucVFifo[u4VDecID], &u4Bits);
#endif
  
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    INT32 a26 = (((prWmvDecBpPrm->u4NumMBY) - 1) << 16) + ((prWmvDecBpPrm->u4NumMBX) - 1);
    INT32 a28 = (((prWmvDecBpPrm->u4NumMBY) *16) << 16) + ((prWmvDecBpPrm->u4NumMBX) * 16);
#else
    INT32 height = (prWmvDecBpPrm->ucFrameCodingMode == INTERLACEFIELD) ? prWmvDecBpPrm->u4PicHeightSrc / 2 : prWmvDecBpPrm->u4PicHeightDec/*Main Profile only*/;
    INT32 a36 = (0<<31)+ //MPEG1_flag
                (((height) + 15 ) << 16)+ //vertical_size
                (63 << 8)+ //max_mbl_mod/** b_buffer_size_s1 **/ << 8)+
                (prWmvDecBpPrm->u4NumMBX);
#endif
    INT32 a37 = (0<<24)+ //part_dec_sta_y /* mc_start_mb_row*/ << 24) +
                (63<<16)+ //part_dec_end_y /* mc_end_mb_row*/  << 16) +
                (0<<8); //b_buf_start  /*b_buffer_start_row*/ << 8);
    INT32 a35 = (1 << 27) + //dec_b_pic_all
                (0 << 8) + //mc_start_mb_addr
                (prWmvDecBpPrm->u4NumMBX); //mc_end_mb_addr
  
    INT32 a137,a201,a198;
    INT32 pred_use_wdle = 0;

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8550)
    INT32 a163 = 0;
    INT32 a164 = 0;
    INT32 i4OwBp, a171;
#endif
  
    // initialize prediction
    vVDecWriteVLD(u4VDecID, RW_VLD_DCACSA, u4AbsDramANc((UINT32) prWmvDecBpPrm->rWmvWorkBufSa.u4DcacSa) >> 2); //192 DCAC
    vVDecWriteVLD(u4VDecID, RW_VLD_MVSA, ((u4AbsDramANc((UINT32) prWmvDecBpPrm->rWmvWorkBufSa.u4Mv1Sa) >> 2) & 0x3FFFFF)); //194 Mv_1#1(frame/top field)
    vVDecWriteVLD(u4VDecID, RW_VLD_ADDREXTEND, 
        (u4VDecReadVLD(u4VDecID, RW_VLD_ADDREXTEND) & 0xFFFFFFC0) | (((u4AbsDramANc((UINT32) prWmvDecBpPrm->rWmvWorkBufSa.u4Mv1Sa) >> 2) >> 22) & 0x3F)); //194 Mv_1#1(frame/top field)
    //MPEG-4 use    WriteREG(RW_VLD_BMB1, 0x2d124f80); //196 set once
    //MPEG-4 use    WriteREG(RW_VLD_BMB2, 0x2d13d620); //197 set once
    a198 = ((u4AbsDramANc((UINT32) prWmvDecBpPrm->rWmvWorkBufSa.u4Bp1Sa) >> 2) & 0xFFFFFF) +
           (((u4AbsDramANc((UINT32) prWmvDecBpPrm->rWmvWorkBufSa.u4Bp1Sa) >> 2) & 0xF000000) << 4) +
           (pred_use_wdle << 25) +  //wdle
           ( 1 << 26);
    vVDecWriteVLD(u4VDecID, RW_VLD_BCODE_SA, a198); 
    //MPEG-4 use    WriteREG(RW_VLD_DIRE_MD_IL, 0x00000000); //199 set once
    if ((prWmvDecBpPrm->i4CodecVersion == VDEC_WMV1) || (prWmvDecBpPrm->i4CodecVersion == VDEC_WMV2)) {
        a201 = 0x30900002 +
              (1 << 16) +  //ming add for test
              (((prWmvDecBpPrm->u4NumMBX == 1)? 1:0) << 22);
    }
    else {
        a201 = 0x00900002 +
               (1 << 16) +  //ming add for test
               (((prWmvDecBpPrm->u4NumMBX == 1)? 1:0) << 22);
    }
    
    vVDecWriteVLD(u4VDecID, RW_VLD_MBDRAM_SEL, a201); //201 set once
    vVDecWriteVLD(u4VDecID, RW_VLD_MV3_ADDR, u4AbsDramANc((UINT32) prWmvDecBpPrm->rWmvWorkBufSa.u4Mv3Sa) >> 2); //214 Mv_3 for new RTL
    vVDecWriteVLD(u4VDecID, RW_VLD_BP2_ADDR, u4AbsDramANc((UINT32) prWmvDecBpPrm->rWmvWorkBufSa.u4Bp2Sa) >> 2); //203 Bp_2
    vVDecWriteVLD(u4VDecID, RW_VLD_BP3_ADDR, u4AbsDramANc((UINT32) prWmvDecBpPrm->rWmvWorkBufSa.u4Bp3Sa) >> 2); //204 Bp_3
    vVDecWriteVLD(u4VDecID, RW_VLD_BP4_ADDR, u4AbsDramANc((UINT32) prWmvDecBpPrm->rWmvWorkBufSa.u4Bp4Sa) >> 2); //205 Bp_4
    vVDecWriteVLD(u4VDecID, RW_VLD_MV2_ADDR, u4AbsDramANc((UINT32) prWmvDecBpPrm->rWmvWorkBufSa.u4Mv2Sa) >> 2); //211 Mv_2
    vVDecWriteVLD(u4VDecID, RW_VLD_DCAC2_ADDR, u4AbsDramANc((UINT32) prWmvDecBpPrm->rWmvWorkBufSa.u4Dcac2Sa) >> 2); //213 Dcac2    
    //MC post process
    vVDecWriteMC(u4VDecID, RW_MC_PP_DBLK_Y_ADDR, u4AbsDramANc((UINT32) prWmvDecBpPrm->rWmvWorkBufSa.u4Pp1Sa) >> 4);
    vVDecWriteMC(u4VDecID, RW_MC_PP_DBLK_C_ADDR, u4AbsDramANc((UINT32) prWmvDecBpPrm->rWmvWorkBufSa.u4Pp2Sa) >> 4);
    // ~initialize prediction
  
    if (prWmvDecBpPrm->i4CodecVersion == VDEC_WMV2) {
        a137 = (u4BpNum)+
               (1<<16)+ //wmv8
               (prWmvDecBpPrm->i4Wmv8BpMode<<17);
    }
    else {
        a137 = u4BpNum;
    }
  
    /*Enable bit-plane decoder to support picture width greater than 2048*/
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    if (((prWmvDecBpPrm->u4NumMBX) * 16) >= 2048)
        a137 = a137 + (0x1 << 21);
#endif
    
    vop_type = (prWmvDecBpPrm->ucPicType == PVOP) ? 1: (prWmvDecBpPrm->ucPicType == BVOP)? 2:0;
  
    vVDecWriteVLD(u4VDecID, RW_VLD_PARA, 0x00000000);
    vVDecWriteVLD(u4VDecID, RW_VLD_PROC, a35);
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)// Step 1 : Set Picture Size
    vVDecWriteVLDTOP(u4VDecID, RW_TOPVLD_WMV_PICSIZE_MB, a26); 
    vVDecWriteVLDTOP(u4VDecID, RW_TOPVLD_WMV_PICSIZE, a28); 
#else
    vVDecWriteVLD(u4VDecID, RW_VLD_PICSZ, a36); 
#endif
    vVDecWriteVLD(u4VDecID, RW_VLD_MBROWPRM, a37);
    vVDecWriteVLD(u4VDecID, RW_VLD_DIGMBSA, 0x00000000);
    vVDecWriteVLD(u4VDecID, RW_VLD_SCALE, 0x00000000);
    vVDecWriteVLD(u4VDecID, RW_VLD_DIGMBYOFF, 0x00000000);
  
    //***********************************************************************
    //-----------------------------------------------------
    // VLD_reg_212 : interlace 420
    //-----------------------------------------------------
    {
        INT32 a212;
        a212 = (vop_type == IVOP) + //VopCodingTypeI
               ((vop_type == PVOP) << 1)+ //VopCodingTypeP
               ((vop_type == BVOP) << 2)+ //VopCodingTypeB
               (0 << 3)+ //VopCodingTypeS (mp4)
               (0 << 4)+ //MPEG1or2
               (0 << 5)+ // Divx311
               (0 << 6)+ //MP4
               (0 << 7)+ //ShortVideo
               (1 << 8)+ //WMV789A
               ((prWmvDecBpPrm->ucFrameCodingMode != INTERLACEFIELD) << 9); //WMVFramePic
    
        vVDecWriteVLD(u4VDecID, RW_VLD_VOP_TYPE, a212);
    }
  
    //bp mode
    vVDecWriteVLD(u4VDecID, RW_VLD_WMV_BP, a137/*u4BpNum*/); // Step 3 : bit-plane num
  
    //load sum_risc
    vVDecWriteVLD(u4VDecID, RW_VLD_WMV_LOAD_SUM, 0x00000101); // Step 4 : load mpeg2_sum to wmv_sum
    mb();
    vVDecWriteVLD(u4VDecID, RW_VLD_WMV_LOAD_SUM, 0x00000000); // Step 4
  
    //change mpeg2_mode to wmv_mode
    if (prWmvDecBpPrm->i4CodecVersion == VDEC_WMV1) {
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8555)
        if (BSP_GetIcVersion() == IC_8555) {
            if (u4VDecID == 1) {
                vWriteReg(0x32000,(u4ReadReg(0x32000) | 0x1));
            }
            else {
                vWriteReg(0x2B000,(u4ReadReg(0x2B000) | 0x1));
            }
        }
#endif

        vVDecWriteVLD(u4VDecID, RW_VLD_WMV_MODE, 0x00000004); // Step 5 : change to WMV mode

#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8555)
        if (BSP_GetIcVersion() == IC_8555) {
            if (u4VDecID == 1) {
                vWriteReg(0x32000,(u4ReadReg(0x32000) & (~0x1)));
            }
            else {
                vWriteReg(0x2B000,(u4ReadReg(0x2B000) & (~0x1)));
            }
        }
#endif
    }
    else if (prWmvDecBpPrm->i4CodecVersion == VDEC_WMV2) {
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8555)
        if (BSP_GetIcVersion() == IC_8555) {
            if (u4VDecID == 1) {
                vWriteReg(0x32000,(u4ReadReg(0x32000) | 0x1));
            }
            else {
            vWriteReg(0x2B000,(u4ReadReg(0x2B000) | 0x1));
            }
        }
#endif
        vVDecWriteVLD(u4VDecID, RW_VLD_WMV_MODE, 0x00000002); // Step 5 : change to WMV mode
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8555)
        if (BSP_GetIcVersion() == IC_8555) {
            if (u4VDecID == 1) {
                vWriteReg(0x32000,(u4ReadReg(0x32000) & (~0x1)));
            }
            else {
                vWriteReg(0x2B000,(u4ReadReg(0x2B000) & (~0x1)));
            }
        }
#endif
    }
    else {
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8555)
        if (BSP_GetIcVersion() == IC_8555) {
            if (u4VDecID == 1) {
                vWriteReg(0x32000,(u4ReadReg(0x32000) | 0x1));
            }
            else {
                vWriteReg(0x2B000,(u4ReadReg(0x2B000) | 0x1));
            }
        }
#endif

        vVDecWriteVLD(u4VDecID, RW_VLD_WMV_MODE, 0x00000001); // Step 5 : change to WMV mode       

#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8555)
        if (BSP_GetIcVersion() == IC_8555) {
            if (u4VDecID == 1) {
                vWriteReg(0x32000,(u4ReadReg(0x32000) & (~0x1)));
            }
            else {
                vWriteReg(0x2B000,(u4ReadReg(0x2B000) & (~0x1)));
            }
        }
#endif
    }
    
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8550)
    if (prWmvDecBpPrm->fgWmvMode) {
    //WMV: DCAC part   
/*
        vVDecWriteWMVDCAC(u4VDecID, RW_WMV_DCAC_RULEB, use_rule_b);
        INT32 i4Predictor = 0;
        if (prWmvDecPrm->prPPS->i4DCStepSize > 0) {
            i4Predictor = (1024 + (prWmvDecPrm->prPPS->i4DCStepSize >> 1) ) / (prWmvDecPrm->prPPS->i4DCStepSize);
        }        
        vVDecWriteWMVDCAC(u4VDecID, RW_WMV_DCAC_PRED, i4Predictor); 
*/

        //VLD part
        a163 = (WMV_NEW_FLG_EN | WMV_PRED_MOD_EN | WMV_PRED_RD_SYNC_DISABLE | WMV_DRAM_BURST_MODE_EN | WMV_BP_USE_PRED_RD_EN);
        vVDecWriteVLD(u4VDecID, RW_VLD_WMV_NEW_CTRL, a163);

        //a164
        a164 = u4AbsDramANc(prWmvDecBpPrm->rWmvWorkBufSa.u4DcacNewSa);
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
        vVDecWriteVLDTOP(u4VDecID, RW_TOPVLD_WMV_DCAC_NEW_ADDR,  a164);
#else
        a164 |= (1<<30);
        vVDecWriteVLD(u4VDecID, RW_VLD_WMV_DCAC_NEW_ADDR,  a164);
#endif
    
        //a167, a168, a169
        vVDecWriteVLD(u4VDecID, RW_VLD_WMV_BP0_NEW_ADDR, u4AbsDramANc(prWmvDecBpPrm->rWmvWorkBufSa.u4Bp0NewSa) );
        vVDecWriteVLD(u4VDecID, RW_VLD_WMV_BP1_NEW_ADDR, u4AbsDramANc(prWmvDecBpPrm->rWmvWorkBufSa.u4Bp1NewSa) );
        vVDecWriteVLD(u4VDecID, RW_VLD_WMV_BP2_NEW_ADDR, u4AbsDramANc(prWmvDecBpPrm->rWmvWorkBufSa.u4Bp2NewSa) );
 
        //a170
        //INT32 a170;
        //a170 = (prWmvDecBpPrm->u4PicWidthDec / 16)  * 7  * (16/16);
        //vVDecWriteVLD(u4VDecID, RW_VLD_WMV_OW_PRED, a170);

        //a171   
        i4OwBp = (prWmvDecBpPrm->u4PicHeightDec / 16)  * (16/16);
        a171 = ( (i4OwBp << WMV_NUM_OW_BP_0) | (i4OwBp << WMV_NUM_OW_BP_1) | (i4OwBp << WMV_NUM_OW_BP_2));
        vVDecWriteVLD(u4VDecID, RW_VLD_WMV_OW_BP, a171);
    }
#endif    
  
#if (WMV_8320_SUPPORT)
    {
        UINT32 u4VldVal = u4VDecReadVLD(u4VDecID, RW_VLD_LDSH);
      
#if WMV_LOG_TMP
        printk("HWDecBP, Reg 0x1D0:0x%x\n", u4VldVal);
#endif

        u4VldVal &= ~(1 << 8);
        
        vVDecWriteVLD(u4VDecID, RW_VLD_LDSH, u4VldVal); // for 8320

#if WMV_LOG_TMP // print input window
        printk("Input window :0x%x, 0x%x\n", u4VDecVLDGetBitS(0, 0, 0), u4VDecReadVLD(u4VDecID, 0xf0));
#endif
    }
#endif
    
    //start bp_decode
    mb();
    vVDecWriteVLD(u4VDecID, RW_VLD_WMVDEC, 0x00000100); // Step 6 : start bit-plane decoding
    mb();
    vVDecWriteVLD(u4VDecID, RW_VLD_WMVDEC, 0x00000000); // Step 6
  
    while (1) {
        for (i = 0; i < 1000; i++) {
            a += 100;
        }
        if ((u4VDecReadVLD(u4VDecID, RO_VLD_BP_DEC_END) & 0x100) == 0x100) { // Step 7 : Wait for bit-plane decode finish
            break;
        }
        else {
            u4VDecReadVLD(u4VDecID, 147*4);
            u4VDecReadVLD(u4VDecID, 132*4);
        }
    }
  
  
    // VLD_reg_116 : load sum_out_wmv(or mp4)
    vVDecWriteVLD(u4VDecID, RW_VLD_LDSH, 0x00000001); // Step 8 : load wmv_sum to mpeg2_sum
    mb();
    vVDecWriteVLD(u4VDecID, RW_VLD_LDSH, 0x00000000); // Step 8
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8555)  
    //change wmv_mode to mpeg2_mode
    if (BSP_GetIcVersion() == IC_8555) {
        if (u4VDecID == 1) { //add for lite random error in mt8555 verification @>Youlin Pei
            vWriteReg(0x32000,(u4ReadReg(0x32000) | 0x1));
        }
        else {
            vWriteReg(0x2B000,(u4ReadReg(0x2B000) | 0x1));
        }
    }
#endif
#if (!WMV_8320_SUPPORT)
    vVDecWriteVLD(u4VDecID, RW_VLD_WMV_MODE, 0x00000000); // Step 9 : change to mpeg2 mode
#else
    {
        UINT32 u4VldVal = u4VDecReadVLD(u4VDecID, RW_VLD_LDSH);
      
#if WMV_LOG_TMP
        printk("HWDecBP, Reg 0x1D0:0x%x\n", 
        u4VldVal);
#endif

        u4VldVal |= (1 << 8);
        
        vVDecWriteVLD(u4VDecID, RW_VLD_LDSH, u4VldVal); // for 8320

#if WMV_LOG_TMP // print input window
        printk("Input window :0x%x, 0x%x\n", u4VDecVLDGetBitS(0, 0, 0), u4VDecReadVLD(u4VDecID, 0xf0));
#endif
    }
#endif
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8555)
    if (BSP_GetIcVersion() == IC_8555) {
        if (u4VDecID == 1) {
            vWriteReg(0x32000,(u4ReadReg(0x32000) & (~0x1)));
        }
        else {
            vWriteReg(0x2B000,(u4ReadReg(0x2B000) & (~0x1)));
        }
    }
#endif
    //vVDecWriteMC(u4VDecID, RW_VLD_WMV_ABS, 0x00000000);
    printk("i4VDEC_HAL_WMV_HWDecBP end\n");
    return HAL_HANDLE_OK;
}

// **************************************************************************
// Function : UINT32 u4VDEC_HAL_WMV_GetBitcount(UINT32 u4BSID, UINT32 u4VDecID);
// Description :Read barrel shifter bitcount after initializing 
// Parameter : u4BSID  : barrelshifter ID
//                 u4VDecID : video decoder hardware ID
// Return      : Current bit count
// **************************************************************************
#if VDEC_REMOVE_UNUSED_FUNC
UINT32 u4VDEC_HAL_WMV_GetBitcount(UINT32 u4BSID, UINT32 u4VDecID)
{
    return HAL_HANDLE_OK;
}
#endif

// **************************************************************************
// Function : void v4VDEC_HAL_WMV_GetMbxMby(UINT32 u4VDecID, UINT32 *pu4Mbx, UINT32 *pu4Mby);
// Description :Read current decoded mbx and mby
// Parameter : u4VDecID : video decoder hardware ID
//                 pu4Mbx : macroblock x value
//                 pu4Mby : macroblock y value
// Return      : None
// **************************************************************************
void vVDEC_HAL_WMV_GetMbxMby(UINT32 u4VDecID, UINT32 *pu4Mbx, UINT32 *pu4Mby)
{
    *pu4Mbx = u4VDecReadMC(u4VDecID, RO_MC_MBX);
    *pu4Mby = u4VDecReadMC(u4VDecID, RO_MC_MBY);
}

// **************************************************************************
// Function : void v4VDEC_HAL_WMV_GetErrInfo(UINT32 u4VDecID, VDEC_INFO_WMV_ERR_INFO_T *prWmvErrInfo);
// Description :Read error count after decoding end
// Parameter : u4VDecID : video decoder hardware ID
//                 prWmvErrInfo : pointer to wmv error info struct
// Return      : None
// **************************************************************************
void vVDEC_HAL_WMV_GetErrInfo(UINT32 u4VDecID, VDEC_INFO_WMV_ERR_INFO_T *prWmvErrInfo)
{
    UINT32 u4RegVal;

#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8520)
    u4RegVal = u4VDecReadVLD(u4VDecID, RO_VLD_WMV_ERRFG);       
    prWmvErrInfo->u4WmvErrType = (u4RegVal & 0xFF);
    prWmvErrInfo->u4WmvErrCnt = ((u4RegVal >> 8) & 0xFFFF);
    prWmvErrInfo->u4WmvErrRow = ((u4RegVal >> 24) & 0xFF);              
#else
       u4RegVal = u4VDecReadVLD(u4VDecID, RO_VLD_WMV_ERRCNT);  
       prWmvErrInfo->u4WmvErrCnt = (u4RegVal & 0xFFF);
       
       u4RegVal = u4VDecReadVLD(u4VDecID, RO_VLD_WMV_ERRTYP);  
       prWmvErrInfo->u4WmvErrType = (u4RegVal );
#endif
}

// **************************************************************************
// Function : UINT32 u4VDEC_HAL_WMV_GetErrType(UINT32 u4VDecID);
// Description :Read wmv error type after decoding end
// Parameter : u4VDecID : video decoder hardware ID
// Return      : wmv decode error type value
// **************************************************************************
UINT32 u4VDEC_HAL_WMV_GetErrType(UINT32 u4VDecID)
{
    return u4VDecReadVLD(u4VDecID, RO_VLD_WMV_ERRFG);
}

//extern BOOL _VDecNeedDumpRegister(UINT32 u4VDecID);

// **************************************************************************
// Function : INT32 i4VDEC_HAL_WMV_DecStart(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm);
// Description :Set video decoder hardware registers to decode for wmv
// Parameter : prDecWmvPrm : pointer to wmv decode info struct
// Return      : =0: success.
//                  <0: fail.
// **************************************************************************
INT32 i4VDEC_HAL_WMV_DecStart(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm)
{
    printk("i4VDEC_HAL_WMV_DecStart start\n");
    INT32 i4Scale, i4Shift;
    long a34, a37, a38, a39, a42, a35,
         a131, a132, a133, a136, a137, a138, a140, a142, a195, b_fraction_chk,
         a193, a198, a199, a200, a201, a206;
    #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    long a26, a28;
    #else
    long a36;
    #endif
    INT32 tmp, TRB, TRD;
    unsigned long high_rate_switch;
    //INT32 dump_size;
    INT32 use_rule_b, ttfrm, use_quarter_pel_mv, use_interpredictor;
    INT32 height; //for interlace_field
    INT32 vop_type,i;
    INT32 wmv78_slice_num;
    INT32 pred_use_wdle = 0;
    UINT32 u4Mbstart_Dcac_Switch = 0;
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8560)
    UINT32 u4RegValue;
#endif

#if (CONFIG_DRV_VERIFY_SUPPORT) && (!VDEC_DRV_PARSER)
    VDEC_INFO_WMV_DEC_PRM_T *prWmvDecPrm = (VDEC_INFO_WMV_DEC_PRM_T *) &(prDecPrm->SpecDecPrm.rVDecWMVDecPrm);
#else
     VDEC_INFO_WMV_DEC_PRM_T *prWmvDecPrm = (VDEC_INFO_WMV_DEC_PRM_T *)prDecPrm->prVDecCodecHalPrm;
#endif
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
       long a134;
#endif

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8550)            
     UINT32 aAvcMv131, aAvcMv134, aAvcMv135;    
     UINT32 aAvcMAddr = 0;
     UINT32 i4Ratio0, i4Ratio1;
     UINT32 i4RegR0, i4RegR1;
     INT32 i4Vc1_Mv_Cfg = 0;
     UINT32 aAvcMV138 = 0;// = 0x17c01fc;
     UINT32 amv_max_v1, amv_max_h1;
     UINT32 aAvcMV139 = 0;// = 0xbc00fc;
     UINT32 amv_max_v2, amv_max_h2;
     INT32 a163,a164;
     INT32 a170;
     INT32 i4OwBp, a171;
     INT32 i4Predictor = 0;
     UINT32 a154;
#endif

#if VDEC_DDR3_SUPPORT
     UINT32 u4DDR3_PicWdith;
     UINT32 aMc406;
#endif
  
 #ifdef WMV_CRCCHECK_ENABLE
    UINT32 u4CRC_Agent = 0;
 #endif
  
    vop_type = (prWmvDecPrm->prPPS->ucPicType == PVOP) ? PVOP: (prWmvDecPrm->prPPS->ucPicType == BVOP)? BVOP : IVOP; //VLD view~ ~ ~
  
  
    if ((prWmvDecPrm->prSPS->fgXintra8) && (prWmvDecPrm->prPPS->ucPicType == IVOP))
    {
        return HAL_HANDLE_OK;
    }
  
    // set video down scaler parameter
    vVDECSetDownScalerPrm(u4VDecID, &prDecPrm->rDownScalerPrm);

#ifdef LETTERBOX_SUPPORT
    vVDecWriteDV(u4VDecID, RW_VEC_LBOX_THD_OFFSET, 0x00802010);
#endif  

#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560) && CONFIG_DRV_FTS_SUPPORT)
    // set letterbox detection parameter
    vVDECSetLetetrBoxDetPrm(u4VDecID, &prDecPrm->rLBDPrm);
#endif    
    
    //-------------------------------------------------
    // global setting 只要一開始設一次即可
    //-------------------------------------------------

    #if VMMU_SUPPORT
    {
    UINT32 u4Page_addr = 0;
    
    u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((prWmvDecPrm->rWmvWorkBufSa.u4Pic0YSa)/(4*1024))*4);
    printk("[WMV] i4VDEC_HAL_WMV_DecStart, 1, Page Addr = 0x%x\n", u4Page_addr);    
    vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic0YSa), PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic0YSa) + PIC_Y_SZ);
    vVDecWriteMC(u4VDecID, RW_MC_R1Y, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic0YSa) >> prWmvDecPrm->i4MemBase >> 9); 

    u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((prWmvDecPrm->rWmvWorkBufSa.u4Pic0CSa)/(4*1024))*4);
    printk("[WMV] i4VDEC_HAL_WMV_DecStart, 2, Page Addr = 0x%x\n", u4Page_addr);    
    vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic0CSa), PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic0CSa) + PIC_C_SZ);
    vVDecWriteMC(u4VDecID, RW_MC_R1C, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic0CSa) >> prWmvDecPrm->i4MemBase >> 8); 

    u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((prWmvDecPrm->rWmvWorkBufSa.u4Pic1YSa)/(4*1024))*4);
    printk("[WMV] i4VDEC_HAL_WMV_DecStart, 3, Page Addr = 0x%x\n", u4Page_addr);    
    vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic1YSa), PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic1YSa) + PIC_Y_SZ);
    vVDecWriteMC(u4VDecID, RW_MC_R2Y, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic1YSa) >> prWmvDecPrm->i4MemBase >> 9); 

    u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((prWmvDecPrm->rWmvWorkBufSa.u4Pic1CSa)/(4*1024))*4);
    printk("[WMV] i4VDEC_HAL_WMV_DecStart, 4, Page Addr = 0x%x\n", u4Page_addr);    
    vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic1CSa), PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic1CSa) + PIC_C_SZ);
    vVDecWriteMC(u4VDecID, RW_MC_R2C, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic1CSa) >> prWmvDecPrm->i4MemBase >> 8); 

    u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((prWmvDecPrm->rWmvWorkBufSa.u4Pic2YSa)/(4*1024))*4);
    printk("[WMV] i4VDEC_HAL_WMV_DecStart, 5, Page Addr = 0x%x\n", u4Page_addr);    
    vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic2YSa), PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic2YSa) + PIC_Y_SZ);
    vVDecWriteMC(u4VDecID, RW_MC_BY, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic2YSa) >> prWmvDecPrm->i4MemBase >> 8); 
    vVDecWriteMC(u4VDecID, RW_MC_BY1, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic2YSa) >> prWmvDecPrm->i4MemBase >> 9); 

    u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((prWmvDecPrm->rWmvWorkBufSa.u4Pic2CSa)/(4*1024))*4);
    printk("[WMV] i4VDEC_HAL_WMV_DecStart, 6, Page Addr = 0x%x\n", u4Page_addr);    
    vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic2CSa), PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic2CSa) + PIC_C_SZ);
    vVDecWriteMC(u4VDecID, RW_MC_BC, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic2CSa) >> prWmvDecPrm->i4MemBase >> 7); 
    vVDecWriteMC(u4VDecID, RW_MC_BC1, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic2CSa) >> prWmvDecPrm->i4MemBase >> 8); 
    }
    #else
    vVDecWriteMC(u4VDecID, RW_MC_R1Y, (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic0YSa) >> prWmvDecPrm->i4MemBase) >> 9); // div 512
    vVDecWriteMC(u4VDecID, RW_MC_R1C, (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic0CSa) >> prWmvDecPrm->i4MemBase) >> 8); // div 256
    vVDecWriteMC(u4VDecID, RW_MC_R2Y, (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic1YSa) >> prWmvDecPrm->i4MemBase) >> 9); // div 512
    vVDecWriteMC(u4VDecID, RW_MC_R2C, (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic1CSa) >> prWmvDecPrm->i4MemBase) >> 8); // div 256
    vVDecWriteMC(u4VDecID, RW_MC_BY,  (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic2YSa) >> prWmvDecPrm->i4MemBase) >> 8); // div 256
    vVDecWriteMC(u4VDecID, RW_MC_BC,  (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic2CSa) >> prWmvDecPrm->i4MemBase) >> 7); // div 128
    vVDecWriteMC(u4VDecID, RW_MC_BY1,  (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic2YSa) >> prWmvDecPrm->i4MemBase) >> 9); // div 256
    vVDecWriteMC(u4VDecID, RW_MC_BC1,  (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic2CSa) >> prWmvDecPrm->i4MemBase) >> 8); // div 128
    #endif
    
    #if VDEC_FIELD_COMPACT
    #else
    // addr swap mode
    vVDecWriteMC(u4VDecID, RW_MC_ADDRSWAP, prDecPrm->ucAddrSwapMode);
    #endif

#if (CONFIG_DRV_FPGA_BOARD)
  vVDecWriteMC(u4VDecID, RW_MC_MODE_CTL, MC_QIU_BANK4|MC_QIU_BANK8|MC_DRAM_REQ_DELAY_1T|MC_DRAM_REQ_MERGE_OFF|MC_MV_MERGE_OFF);
#endif
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
  //vVDecWriteMC(u4VDecID, RW_MC_ADDRSWAP, ADDRSWAP_OFF);  
  #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
  vVDecWriteMC(u4VDecID, RW_MC_PIC_W_MB, ((prDecPrm->u4PicBW + 15)>> 4)); 
  #else
  vVDecWriteVLD(u4VDecID, RW_VLD_PIC_W_MB, ((prDecPrm->u4PicBW + 15)>> 4));  
#endif
#endif

    // initialize prediction
    vVDecWriteVLD(u4VDecID, RW_VLD_DCACSA, u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4DcacSa) >> 2); //192 DCAC
    vVDecWriteVLD(u4VDecID, RW_VLD_MVSA, ((u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Mv1Sa) >> 2) & 0x3FFFFF)); //194 Mv_1#1(frame/top field)
    vVDecWriteVLD(u4VDecID, RW_VLD_ADDREXTEND, (u4VDecReadVLD(u4VDecID, RW_VLD_ADDREXTEND) & 0xFFFFFFC0) |
    (((u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Mv1Sa) >> 2) >> 22) & 0x3F)); //194 Mv_1#1(frame/top field)
    //MPEG-4 use    WriteREG(RW_VLD_BMB1, 0x2d124f80); //196 set once
    //MPEG-4 use    WriteREG(RW_VLD_BMB2, 0x2d13d620); //197 set once
    a198 = ((u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Bp1Sa) >> 2) & 0xFFFFFF) +
               (((u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Bp1Sa) >> 2) & 0xF000000) << 4) +
               (pred_use_wdle << 25) +  //wdle
               ( 1 << 26);
    vVDecWriteVLD(u4VDecID, RW_VLD_BCODE_SA, a198); //(UINT32)prWmvDecPrm->u4Bp1Sa >> 3); //198 Bp_1
    //MPEG-4 use    WriteREG(RW_VLD_DIRE_MD_IL, 0x00000000); //199 set once
    
    if ((prWmvDecPrm->i4CodecVersion == VDEC_WMV1) || (prWmvDecPrm->i4CodecVersion == VDEC_WMV2))
    {
       a201 =0x30900002 +
          (1 << 16) +  //ming add for test
          (((prWmvDecPrm->prSPS->u4NumMBX == 1)? 1:0) << 22);/* 0x00100000 +
           (_pred_use_wdle << 20)+
           (_pred_use_wdle << 1 );*/
    }
    else
    {
    a201 =0x00900002 +
          (1 << 16) +  //ming add for test
          (((prWmvDecPrm->prSPS->u4NumMBX == 1)? 1:0) << 22);/* 0x00100000 +
           (_pred_use_wdle << 20)+
           (_pred_use_wdle << 1 );*/
    }
    
    vVDecWriteVLD(u4VDecID, RW_VLD_MBDRAM_SEL, a201); //201 set once
    vVDecWriteVLD(u4VDecID, RW_VLD_MV3_ADDR, u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Mv3Sa) >> 2); //214 Mv_3 for new RTL
    vVDecWriteVLD(u4VDecID, RW_VLD_BP2_ADDR, u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Bp2Sa) >> 2); //203 Bp_2
    vVDecWriteVLD(u4VDecID, RW_VLD_BP3_ADDR, u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Bp3Sa) >> 2); //204 Bp_3
    vVDecWriteVLD(u4VDecID, RW_VLD_BP4_ADDR, u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Bp4Sa) >> 2); //205 Bp_4
    vVDecWriteVLD(u4VDecID, RW_VLD_MV2_ADDR, u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Mv2Sa) >> 2); //211 Mv_2
    vVDecWriteVLD(u4VDecID, RW_VLD_DCAC2_ADDR, u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Dcac2Sa) >> 2); //213 Dcac2
    //WriteREG(RW_VLD_MV2_ADDR, 0x00011170); //211 Mv_2
    //WriteREG(RW_VLD_MVSA, 0x00013880); //194 Mv_1#2(bottom field)
    //MC post process
    vVDecWriteMC(u4VDecID, RW_MC_PP_DBLK_Y_ADDR, u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pp1Sa) >> 4);
    vVDecWriteMC(u4VDecID, RW_MC_PP_DBLK_C_ADDR, u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pp2Sa) >> 4);
    
  
  //  vVDecWriteMC(u4VDecID, RW_MC_2FLDMD, 0x00000000);
    vVDecWriteMC(u4VDecID, RW_MC_QUARTER_SAMPLE, 0x00000001); //always set to 1 for WMV case
    vVDecWriteMC(u4VDecID, RW_MC_PP_DBLK_THD, 0x000f0606); //always set to this value for WMV case
  //  vVDecWriteMC(u4VDecID, RW_MC_PP_DBLK_Y_ADDR, DEC_PP_1);
  //  vVDecWriteMC(u4VDecID, RW_MC_PP_DBLK_C_ADDR, DEC_PP_2);
  
    //vVDecWriteVLD(u4VDecID, RW_VLD_SRAM_TEST_WRITE_ADDR, 200); //ming add for debug 01/03
  //  vVDecWriteMC(u4VDecID, 0x50c, 0xe00); //err_conceal off
  //vVDecWriteMC(u4VDecID, 0xD7c, 0x1); //turn off the ext_sram
  //vVDecWriteMC(u4VDecID, 0x7f8, 0x1); //turn on check sum shift mode
  //vVDecWriteMC(u4VDecID, VLD_REG_OFST + 0x258, 1000);
  
    vVDecWriteVLD(u4VDecID, RW_VLD_MBDRAM_SEL, u4VDecReadVLD(u4VDecID, RW_VLD_MBDRAM_SEL) | (1 << 16));
    //------------------------------
    //   MC RISC WRITE PATTERN
    //------------------------------
    //----------------------------------------------------------
    // MC_reg_9 : MC output buffer setting
    //            (0:ref_pic1_buf  1:ref_pic2_buf  4:prg_B_pic1)
    //----------------------------------------------------------
    if ((prWmvDecPrm->prPPS->ucPicType == BVOP) || (prWmvDecPrm->prPPS->ucPicType == BIVOP))
    {
        vVDecWriteMC(u4VDecID, RW_MC_OPBUF, 0x00000004);
        vVDecWriteMC(u4VDecID, RW_MC_FWDP, prWmvDecPrm->u4FRefBufIdx);
    }
    else //I, P
    {
        vVDecWriteMC(u4VDecID, RW_MC_OPBUF, prDecPrm->ucDecFBufIdx);
        vVDecWriteMC(u4VDecID, RW_MC_FWDP, prWmvDecPrm->u4FRefBufIdx);
    }
  
    //----------------------------------------------------------
    // MC_reg_8 : indicate if this is the 2nd field picture
    //        (0:1st field picture or frame picture
    //         1:2nd field picture)
    //----------------------------------------------------------
    vVDecWriteMC(u4VDecID, RW_MC_2FLDMD, prWmvDecPrm->prPPS->i4CurrentTemporalField);
  
    //----------------------------------------------------------
    // MC_reg_180 : mixed mv resolution (1: turn on)
    //----------------------------------------------------------
    vVDecWriteMC(u4VDecID, RW_MC_WMV8_MIX_PEL, prWmvDecPrm->prPPS->fgMvResolution);
  
    //----------------------------------------------------------
    // MC_reg_127 : quarter pel type (set 0:TYPE_14496)
    //----------------------------------------------------------
    vVDecWriteMC(u4VDecID, RW_MC_QPEL_TYPE, 0x00000000); //always set to 0 for WMV
  
    //----------------------------------------------------------
    //MC_reg_129 : rounding_control (1:turn on)
    //----------------------------------------------------------
    vVDecWriteMC(u4VDecID, RW_MC_ROUNDING_CTRL, prWmvDecPrm->prPPS->i4RndCtrl);
  
    //--------------------------------------------------------------------
    // MC_reg_130, 131 : UMV Padding boundary (取16的倍數, 或實際pic大小)
    //--------------------------------------------------------------------
    if (prWmvDecPrm->i4CodecVersion <= VDEC_WMV3)
    {
        vVDecWriteMC(u4VDecID, RW_MC_UMV_PIC_WIDTH, prWmvDecPrm->prSPS->u4PicWidthDec);
        vVDecWriteMC(u4VDecID, RW_MC_UMV_PIC_HEIGHT, prWmvDecPrm->prSPS->u4PicHeightDec);
    }
    else /* UMV Padding directly from picture boundary */
    {
        vVDecWriteMC(u4VDecID, RW_MC_UMV_PIC_WIDTH, prWmvDecPrm->prSPS->u4PicWidthSrc);
        vVDecWriteMC(u4VDecID, RW_MC_UMV_PIC_HEIGHT, prWmvDecPrm->prSPS->u4PicHeightSrc);
    }
  
    if (prWmvDecPrm->prSPS->u4PicWidthSrc > 1920)
    {
        #if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8580)
        vVDecWriteMC(u4VDecID, RW_MC_WRAPPER_SWITCH, 1);
        #else
        vVDecWriteMC(u4VDecID, RW_MC_WRAPPER_SWITCH, 0);
        #endif
    }
    else
    {
        vVDecWriteMC(u4VDecID, RW_MC_WRAPPER_SWITCH, 0);
    }
  
    //--------------------------------------------------------------------
    // MC_reg_156 : scale type (0:NO_SCALE, 1:SCALE_DOWN, 2:SCALE_UP)
    //--------------------------------------------------------------------
    if (prWmvDecPrm->prPPS->ucPicType == IVOP)
    {
        vVDecWriteMC(u4VDecID, RW_MC_WMV9_PRE_PROC, PP_NO_SCALE);
    }
    else if ( (prWmvDecPrm->prPPS->ucPicType == PVOP) || (prWmvDecPrm->prPPS->ucPicType == SKIPFRAME) ) /* only for PVOP */
    {
        if (prWmvDecPrm->prEPS->i4ReconRangeState == 0)//pWMVDec->m_iReconRangeState == 0)
        {
            if (prWmvDecPrm->prEPS->i4RangeState == 1)
            {
                // JUP comment
                // Previous not scale range
                // Current scale down range
                // Hence, scan DOWN previous frame to using it for current frame's motion compensation
                //
                // reduce by 2
                vVDecWriteMC(u4VDecID, RW_MC_WMV9_PRE_PROC, PP_SCALE_DOWN);
            }
            else
            {
                vVDecWriteMC(u4VDecID, RW_MC_WMV9_PRE_PROC, PP_NO_SCALE);
            }
        } else if (prWmvDecPrm->prEPS->i4ReconRangeState == 1)//pWMVDec->m_iReconRangeState == 1)
        {
            if (prWmvDecPrm->prEPS->i4RangeState == 0)
            {
                // JUP comment
                // Previous scale down range
                // Current not scale range
                // Hence, scan UP previous frame to using it for current frame's motion compensation
                //
                // increase by 2
                vVDecWriteMC(u4VDecID, RW_MC_WMV9_PRE_PROC, PP_SCALE_UP);
            }
            else
            {
                vVDecWriteMC(u4VDecID, RW_MC_WMV9_PRE_PROC, PP_NO_SCALE);
            }
        }
    }
    else// if (prWmvDecPrm->ucPicType == BVOP) , Ju said both for BIVOP and BVOP
    {
        /* just follow the setting of last decoded P picture */
        vVDecWriteMC(u4VDecID, RW_MC_WMV9_PRE_PROC, prWmvDecPrm->prICOMPS->ucPreProcessFrameStatus);
    }
  
     //-------------------------------------------------------
    // intensity compensation 
    // 
    //------------------------------------------------------
  
    if ((prWmvDecPrm->prPPS->ucPicType == IVOP) && (prWmvDecPrm->i4CodecVersion == VDEC_VC1))
    {
        vVDecWriteMC(u4VDecID, RW_MC_WMV9_ICOMP_EN, ICOMP_C_OFF + ICOMP_Y_OFF);
        vVDecWriteMC(u4VDecID, RW_MC_ICOMP2_EN, 0);
    }
    else if (( (prWmvDecPrm->prPPS->ucPicType == PVOP) || (prWmvDecPrm->prPPS->ucPicType == SKIPFRAME) ) &&  (prWmvDecPrm->i4CodecVersion == VDEC_VC1))
    {
        /* Step 3: output MC parameters */
        // icomp_set1 is forward top field new icomp parameters
        if (prWmvDecPrm->prICOMPS->NewTopField.New.i4Enable == 1)
        {
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X1, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 1));
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X3, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 3));
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X5, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 5));
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X7, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 7));
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X9, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 9));
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X11, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 11));
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X13, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 13));
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X15, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 15));
            vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF1, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Shift + 32));
            vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF1, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale)));
        }
    
        // icomp_set2 is forward bottom field new icomp parameters
        if (prWmvDecPrm->prICOMPS->NewBotField.New.i4Enable == 1)
        {
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X1, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 1));
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X3, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 3));
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X5, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 5));
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X7, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 7));
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X9, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 9));
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X11, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 11));
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X13, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 13));
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X15, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 15));
            vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF2, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Shift + 32));
            vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF2, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale)));
        }
    
        if (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Enable == 1)
        {
            // icomp_set3 is forward top field old icomp parameters
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X1, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale * 1));
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X3, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale * 3));
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X5, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale * 5));
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X7, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale * 7));
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X9, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale * 9));
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X11, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale * 11));
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X13, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale * 13));
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X15, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale * 15));
            vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF3, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Shift + 32));
            vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF3, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale)));
        }
    
        if (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Enable == 1)
        {
            // icomp_set4 is forward bottom field old icomp parameters
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X1, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 1));
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X3, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 3));
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X5, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 5));
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X7, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 7));
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X9, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 9));
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X11, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 11));
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X13, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 13));
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X15, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 15));
            vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF4, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Shift + 32));
            vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF4, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale)));
        }
    
        if (prWmvDecPrm->prICOMPS->i4BoundaryUMVIcomp == 1)
        {
            /* top field picture */
            if (prWmvDecPrm->prICOMPS->i4SecondFieldParity == 0)
            {
                if (prWmvDecPrm->prICOMPS->OldBotField.New.i4Enable == 1)
                {
                    // icomp_set6 is forward bottom field old icomp parameters
                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X1, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 1));
                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X3, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 3));
                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X5, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 5));
                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X7, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 7));
                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X9, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 9));
                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X11, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 11));
                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X13, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 13));
                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X15, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 15));
                    vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF6, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Shift + 32));
                    vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF6, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale)));
                }
            }
            /* bottom field picture */
            else
            {
                if (prWmvDecPrm->prICOMPS->OldTopField.New.i4Enable == 1)
                {
                    // icomp_set6 is forward bottom field old icomp parameters
                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X1, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 1));
                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X3, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 3));
                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X5, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 5));
                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X7, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 7));
                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X9, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 9));
                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X11, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 11));
                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X13, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 13));
                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X15, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 15));
                    vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF6, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Shift + 32));
                    vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF6,((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale)));
                }
            }
        }
        
        if (
               (prWmvDecPrm->prICOMPS->NewTopField.New.i4Enable == 1) ||
               (prWmvDecPrm->prICOMPS->NewBotField.New.i4Enable == 1) ||
               (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Enable == 1) ||
               (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Enable == 1) ||
               (prWmvDecPrm->prICOMPS->i4BoundaryUMVIcompEnable == 1) 
             )
        {
            // Icomp enable bits
            {
                INT32 b208 = (prWmvDecPrm->prICOMPS->i4BoundaryUMVIcompEnable)+
                       (0 << 4)+
                       (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Enable << 8)+
                       (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Enable << 12)+
                       (prWmvDecPrm->prICOMPS->NewBotField.New.i4Enable << 16)+
                       (prWmvDecPrm->prICOMPS->NewTopField.New.i4Enable << 20);
        
                vVDecWriteMC(u4VDecID, RW_MC_WMV9_ICOMP_EN, ICOMP_C_EN + ICOMP_Y_EN);
                vVDecWriteMC(u4VDecID, RW_MC_ICOMP2_EN, b208);
                vVDecWriteMC(u4VDecID, RW_MC_SAME_ICOMP, NO_USE_SAME_ICOPM1_FOR_FRAME);
            }
        }
        else
        {
            vVDecWriteMC(u4VDecID, RW_MC_WMV9_ICOMP_EN, ICOMP_C_OFF + ICOMP_Y_OFF);
            vVDecWriteMC(u4VDecID, RW_MC_ICOMP2_EN, 0);
            vVDecWriteMC(u4VDecID, RW_MC_SAME_ICOMP, NO_USE_SAME_ICOPM1_FOR_FRAME);
        }   
    }//end : PVOP
    else if ((prWmvDecPrm->prPPS->ucPicType == BVOP) && (prWmvDecPrm->i4CodecVersion == VDEC_VC1))
    {
        if (prWmvDecPrm->prPPS->fgFieldMode == TRUE) //field picture
        {
            if (prWmvDecPrm->prPPS->i4CurrentTemporalField== 0)// the first B field picture
            {
                //
                // Step1: No update in the Icomp parameters
                //
             
                //
                // Step2: output MC parameters   
                //
        
                //
                // Last second P field picture is Top Field
                //
                // (1) Forward TF == NEW_TF
                // (2) Forward BF == OLD_BF
                // (3) Backward TF == NULL
                // (4) Backward BF == NEW_BF
        
                if (prWmvDecPrm->prICOMPS->i4SecondFieldParity == 0)
                {      
                    // icomp_set1 is forward top field new icomp parameters 
                    if (prWmvDecPrm->prICOMPS->NewTopField.New.i4Enable == 1)
                    {        
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X1, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 1));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X3, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 3));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X5, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 5));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X7, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 7));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X9, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 9));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X11, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 11));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X13, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 13));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X15, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 15));
                        vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF1, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Shift + 32));
                        vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF1, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale)));
                    }
          
                    // icomp_set2 is forward bottom field new icomp parameters
                    if (prWmvDecPrm->prICOMPS->OldBotField.New.i4Enable == 1)
                    {
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X1, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 1));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X3, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 3));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X5, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 5));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X7, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 7));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X9, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 9));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X11, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 11));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X13, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 13));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X15, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 15));
                        vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF2, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Shift + 32));
                        vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF2, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale)));
                    }
          
                    // icomp_set3 is forward top field old icomp parameters 
                    if (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Enable == 1)
                    {         
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X1, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale * 1));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X3, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale * 3));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X5, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale * 5));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X7, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale * 7));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X9, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale * 9));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X11, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale * 11));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X13, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale * 13));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X15, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale * 15));
                        vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF3, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Shift + 32));
                        vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF3, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale)));
                    }
          
                    // icomp_set4 is forward bottom field old icomp parameters
                    if (prWmvDecPrm->prICOMPS->OldBotField.Old.i4Enable == 1)
                    {          
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X1, (prWmvDecPrm->prICOMPS->OldBotField.Old.i4Scale * 1));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X3, (prWmvDecPrm->prICOMPS->OldBotField.Old.i4Scale * 3));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X5, (prWmvDecPrm->prICOMPS->OldBotField.Old.i4Scale * 5));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X7, (prWmvDecPrm->prICOMPS->OldBotField.Old.i4Scale * 7));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X9, (prWmvDecPrm->prICOMPS->OldBotField.Old.i4Scale * 9));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X11, (prWmvDecPrm->prICOMPS->OldBotField.Old.i4Scale * 11));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X13, (prWmvDecPrm->prICOMPS->OldBotField.Old.i4Scale * 13));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X15, (prWmvDecPrm->prICOMPS->OldBotField.Old.i4Scale * 15));
                        vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF4, (prWmvDecPrm->prICOMPS->OldBotField.Old.i4Shift + 32));
                        vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF4, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->OldBotField.Old.i4Scale)));
                    }
          
                    // icomp_set5 is backward top or bottom field icomp parameters
                    if (prWmvDecPrm->prICOMPS->NewBotField.New.i4Enable == 1)
                    {
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X1, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 1));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X3, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 3));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X5, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 5));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X7, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 7));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X9, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 9));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X11, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 11));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X13, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 13));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X15, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 15));
                        vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF5, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Shift + 32));
                        vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF5, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale)));
                        vVDecWriteMC(u4VDecID, RW_MC_BWD_ICOMP_FLD, BWD_BTM_FLD);
                    }
          
                    if (
                      (prWmvDecPrm->prICOMPS->NewTopField.New.i4Enable == 1) ||
                      (prWmvDecPrm->prICOMPS->OldBotField.New.i4Enable == 1) ||
                      (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Enable == 1) ||
                      (prWmvDecPrm->prICOMPS->OldBotField.Old.i4Enable == 1) ||
                      (prWmvDecPrm->prICOMPS->NewBotField.New.i4Enable == 1) 
                    )
                    {
                        // Icomp enable bits
                        {
                            INT32 b208 = (0)+
                                   (prWmvDecPrm->prICOMPS->NewBotField.New.i4Enable << 4)+
                                   (prWmvDecPrm->prICOMPS->OldBotField.Old.i4Enable << 8)+
                                   (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Enable << 12)+
                                   (prWmvDecPrm->prICOMPS->OldBotField.New.i4Enable << 16)+
                                   (prWmvDecPrm->prICOMPS->NewTopField.New.i4Enable << 20);
              
                            vVDecWriteMC(u4VDecID, RW_MC_WMV9_ICOMP_EN, ICOMP_C_EN + ICOMP_Y_EN);
                            vVDecWriteMC(u4VDecID, RW_MC_ICOMP2_EN, b208);
                            vVDecWriteMC(u4VDecID, RW_MC_SAME_ICOMP, NO_USE_SAME_ICOPM1_FOR_FRAME);
                        }
                    }
                    else
                    {
                        vVDecWriteMC(u4VDecID, RW_MC_WMV9_ICOMP_EN, ICOMP_C_OFF + ICOMP_Y_OFF);
                        vVDecWriteMC(u4VDecID, RW_MC_ICOMP2_EN, 0);
                        vVDecWriteMC(u4VDecID, RW_MC_SAME_ICOMP, NO_USE_SAME_ICOPM1_FOR_FRAME);
                    }  
                }
                else
                {
          
                    //
                    // Last second P field picture is Bottom Field 
                    //
                    // (1) Forward TF == OLD_TF
                    // (2) Forward BF == NEW_BF
                    // (3) Backward TF == NEW_TF
                    // (4) Backward BF == NULL
            
                    // icomp_set1 is forward top field new icomp parameters 
                    if (prWmvDecPrm->prICOMPS->OldTopField.New.i4Enable == 1)
                    {       
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X1, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 1));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X3, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 3));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X5, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 5));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X7, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 7));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X9, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 9));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X11, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 11));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X13, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 13));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X15, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 15));
                        vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF1, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Shift + 32));
                        vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF1, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale)));
                    }
                    // icomp_set2 is forward bottom field new icomp parameters
                    if (prWmvDecPrm->prICOMPS->NewBotField.New.i4Enable == 1)
                    {        
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X1, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 1));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X3, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 3));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X5, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 5));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X7, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 7));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X9, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 9));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X11, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 11));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X13, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 13));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X15, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 15));
                        vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF2, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Shift + 32));
                        vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF2, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale)));
                    }
                 
                    // icomp_set3 is forward top field old icomp parameters 
                    if (prWmvDecPrm->prICOMPS->OldTopField.Old.i4Enable == 1)
                    {         
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X1, (prWmvDecPrm->prICOMPS->OldTopField.Old.i4Scale * 1));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X3, (prWmvDecPrm->prICOMPS->OldTopField.Old.i4Scale * 3));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X5, (prWmvDecPrm->prICOMPS->OldTopField.Old.i4Scale * 5));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X7, (prWmvDecPrm->prICOMPS->OldTopField.Old.i4Scale * 7));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X9, (prWmvDecPrm->prICOMPS->OldTopField.Old.i4Scale * 9));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X11, (prWmvDecPrm->prICOMPS->OldTopField.Old.i4Scale * 11));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X13, (prWmvDecPrm->prICOMPS->OldTopField.Old.i4Scale * 13));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X15, (prWmvDecPrm->prICOMPS->OldTopField.Old.i4Scale * 15));
                        vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF3, (prWmvDecPrm->prICOMPS->OldTopField.Old.i4Shift + 32));
                        vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF3, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->OldTopField.Old.i4Scale)));
                    }
                 
                    // icomp_set4 is forward bottom field old icomp parameters
                    if (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Enable == 1)
                    {          
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X1, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 1));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X3, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 3));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X5, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 5));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X7, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 7));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X9, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 9));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X11, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 11));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X13, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 13));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X15, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 15));
                        vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF4, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Shift + 32));
                        vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF4, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale)));
                    }
          
                    // icomp_set5 is backward top or bottom field icomp parameters
                    if (prWmvDecPrm->prICOMPS->NewTopField.New.i4Enable == 1)
                    {
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X1, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 1));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X3, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 3));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X5, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 5));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X7, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 7));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X9, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 9));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X11, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 11));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X13, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 13));
                        vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X15, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 15));
                        vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF5, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Shift + 32));
                        vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF5, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale)));
                        vVDecWriteMC(u4VDecID, RW_MC_BWD_ICOMP_FLD, BWD_TOP_FLD);
                    }
          
                    if (
                      (prWmvDecPrm->prICOMPS->OldTopField.New.i4Enable == 1) ||
                      (prWmvDecPrm->prICOMPS->NewBotField.New.i4Enable == 1) ||
                      (prWmvDecPrm->prICOMPS->OldTopField.Old.i4Enable == 1) ||
                      (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Enable == 1) ||
                      (prWmvDecPrm->prICOMPS->NewTopField.New.i4Enable == 1)
                    )
                    {
                        // Icomp enable bits
                        {
                            INT32 b208 = (0)+
                                   (prWmvDecPrm->prICOMPS->NewTopField.New.i4Enable << 4)+
                                   (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Enable << 8)+
                                   (prWmvDecPrm->prICOMPS->OldTopField.Old.i4Enable << 12)+
                                   (prWmvDecPrm->prICOMPS->NewBotField.New.i4Enable << 16)+
                                   (prWmvDecPrm->prICOMPS->OldTopField.New.i4Enable << 20);
              
                            vVDecWriteMC(u4VDecID, RW_MC_WMV9_ICOMP_EN, ICOMP_C_EN + ICOMP_Y_EN);
                            vVDecWriteMC(u4VDecID, RW_MC_ICOMP2_EN, b208);
                            vVDecWriteMC(u4VDecID, RW_MC_SAME_ICOMP, NO_USE_SAME_ICOPM1_FOR_FRAME);
                        }
                    }
                    else
                    {
                        vVDecWriteMC(u4VDecID, RW_MC_WMV9_ICOMP_EN, ICOMP_C_OFF + ICOMP_Y_OFF);
                        vVDecWriteMC(u4VDecID, RW_MC_ICOMP2_EN, 0);
                        vVDecWriteMC(u4VDecID, RW_MC_SAME_ICOMP, NO_USE_SAME_ICOPM1_FOR_FRAME);
                    }
                }
            } //end : first B field picture
            else // Second B field picture
            {
               
                if (prWmvDecPrm->prPPS->i4CurrentField == 0) // current field is top field 
                {
                    //
                    // Step1: No update in the Icomp parameters
                    //
               
                    //
                    // Step2: output MC parameters   
                    //
          
                    //
                    // Last second P field picture is Top Field 
                    //
                    // (1) Forward TF == NEW_TF
                    //
                    // (2) Forward BF == NULL
                    //
                    // (3) Backward TF == NULL
                    // (4) Backward BF == NEW_BF
              
                    if (prWmvDecPrm->prICOMPS->i4SecondFieldParity == 0)
                    {      
                        // icomp_set1 is forward top field new icomp parameters 
                        if (prWmvDecPrm->prICOMPS->NewTopField.New.i4Enable == 1)
                        {
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X1, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 1));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X3, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 3));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X5, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 5));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X7, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 7));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X9, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 9));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X11, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 11));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X13, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 13));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X15, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 15));
                            vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF1, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Shift + 32));
                            vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF1, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale)));
                        }
                      
                        // icomp_set3 is forward top field old icomp parameters 
                        if (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Enable == 1)
                        {         
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X1, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale * 1));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X3, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale * 3));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X5, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale * 5));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X7, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale * 7));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X9, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale * 9));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X11, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale * 11));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X13, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale * 13));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X15, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale * 15));
                            vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF3, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Shift + 32));
                            vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF3, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale)));
                        }
                   
                        // icomp_set5 is backward top or bottom field icomp parameters    
                        if (prWmvDecPrm->prICOMPS->NewBotField.New.i4Enable == 1)
                        {
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X1, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 1));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X3, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 3));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X5, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 5));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X7, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 7));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X9, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 9));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X11, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 11));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X13, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 13));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X15, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 15));
                            vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF5, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Shift + 32));
                            vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF5, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale)));
                            vVDecWriteMC(u4VDecID, RW_MC_BWD_ICOMP_FLD, BWD_BTM_FLD);
                        }
            
                     
                        if (prWmvDecPrm->prICOMPS->i4BoundaryUMVIcomp == 1)        
                        {
                            /* top field picture */
                            if (prWmvDecPrm->prICOMPS->i4SecondFieldParity == 0)  
                            {
                                if (prWmvDecPrm->prICOMPS->OldBotField.New.i4Enable == 1)
                                {         
                                    // icomp_set4 is forward bottom field old icomp parameters
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X1, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 1));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X3, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 3));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X5, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 5));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X7, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 7));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X9, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 9));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X11, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 11));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X13, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 13));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X15, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 15));
                                    vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF6, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Shift + 32));
                                    vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF6, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale)));
                                }
                            }
                            /* bottom field picture */         
                            else
                            {
                                if (prWmvDecPrm->prICOMPS->OldTopField.New.i4Enable == 1)
                                {      
                                    // icomp_set4 is forward bottom field old icomp parameters
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X1, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 1));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X3, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 3));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X5, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 5));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X7, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 7));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X9, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 9));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X11, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 11));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X13, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 13));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X15, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 15));
                                    vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF6, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Shift + 32));
                                    vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF6, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale)));
                                }
                            }
                        }
            
                        if (
                          (prWmvDecPrm->prICOMPS->NewTopField.New.i4Enable == 1) ||
                          (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Enable == 1) ||
                          (prWmvDecPrm->prICOMPS->NewBotField.New.i4Enable == 1) ||
                          (prWmvDecPrm->prICOMPS->i4BoundaryUMVIcompEnable == 1)
                        )
                        {
                            // Icomp enable bits
                            {
                                INT32 b208 = (prWmvDecPrm->prICOMPS->i4BoundaryUMVIcompEnable)+
                                       (prWmvDecPrm->prICOMPS->NewBotField.New.i4Enable << 4)+
                                       (0 << 8)+
                                       (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Enable << 12)+
                                       (0 << 16)+
                                       (prWmvDecPrm->prICOMPS->NewTopField.New.i4Enable << 20);
                                
                                vVDecWriteMC(u4VDecID, RW_MC_WMV9_ICOMP_EN, ICOMP_C_EN + ICOMP_Y_EN);
                                vVDecWriteMC(u4VDecID, RW_MC_ICOMP2_EN, b208);
                                vVDecWriteMC(u4VDecID, RW_MC_SAME_ICOMP, NO_USE_SAME_ICOPM1_FOR_FRAME);
                            }
                        }
                        else
                        {
                            vVDecWriteMC(u4VDecID, RW_MC_WMV9_ICOMP_EN, ICOMP_C_OFF + ICOMP_Y_OFF);
                            vVDecWriteMC(u4VDecID, RW_MC_ICOMP2_EN, 0);
                            vVDecWriteMC(u4VDecID, RW_MC_SAME_ICOMP, NO_USE_SAME_ICOPM1_FOR_FRAME);
                        }
                    }
                    else
                    {
                        //
                        // Last second P field picture is Bottom Field 
                        //
                        //
                        // (1) Forward TF == NULL
                        //
                        // (2) Forward BF == NEW_BF
                        // (3) Backward TF == NEW_TF
                        // (4) Backward BF == NULL
                    
                        // icomp_set2 is forward bottom field new icomp parameters
                        if (prWmvDecPrm->prICOMPS->NewBotField.New.i4Enable == 1)
                        {         
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X1, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 1));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X3, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 3));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X5, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 5));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X7, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 7));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X9, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 9));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X11, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 11));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X13, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 13));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X15, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 15));
                            vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF2, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Shift + 32));
                            vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF2, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale)));
                        }       
                    
                        // icomp_set4 is forward bottom field old icomp parameters
                        if (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Enable == 1)
                        {          
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X1, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 1));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X3, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 3));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X5, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 5));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X7, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 7));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X9, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 9));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X11, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 11));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X13, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 13));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X15, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 15));
                            vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF4, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Shift + 32));
                            vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF4, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale)));
                        }
            
                        // icomp_set5 is backward top or bottom field icomp parameters
                
                        if (prWmvDecPrm->prICOMPS->NewTopField.New.i4Enable == 1)
                        {
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X1, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 1));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X3, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 3));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X5, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 5));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X7, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 7));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X9, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 9));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X11, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 11));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X13, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 13));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X15, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 15));
                            vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF5, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Shift + 32));
                            vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF5, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale)));
                            vVDecWriteMC(u4VDecID, RW_MC_BWD_ICOMP_FLD, BWD_TOP_FLD);
                        } 
                  
                        if (prWmvDecPrm->prICOMPS->i4BoundaryUMVIcomp == 1)        
                        {
                            /* top field picture */
                            if (prWmvDecPrm->prICOMPS->i4SecondFieldParity == 0)  
                            {
                                if (prWmvDecPrm->prICOMPS->OldBotField.New.i4Enable == 1)
                                {         
                                    // icomp_set4 is forward bottom field old icomp parameters
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X1, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 1));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X3, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 3));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X5, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 5));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X7, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 7));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X9, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 9));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X11, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 11));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X13, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 13));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X15, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 15));
                                    vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF6, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Shift + 32));
                                    vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF6, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale)));
                                }
                            }
                            /* bottom field picture */         
                            else
                            {
                                if (prWmvDecPrm->prICOMPS->OldTopField.New.i4Enable == 1)
                                {            
                                    // icomp_set4 is forward bottom field old icomp parameters
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X1, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 1));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X3, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 3));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X5, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 5));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X7, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 7));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X9, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 9));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X11, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 11));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X13, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 13));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X15, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 15));
                                    vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF6, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Shift + 32));
                                    vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF6, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale)));
                                }
                            }
                        }
            
                        if (
                          (prWmvDecPrm->prICOMPS->NewBotField.New.i4Enable == 1) ||
                          (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Enable == 1) ||
                          (prWmvDecPrm->prICOMPS->NewTopField.New.i4Enable == 1) ||
                          (prWmvDecPrm->prICOMPS->i4BoundaryUMVIcompEnable == 1)
                          )
                        {
                            // Icomp enable bits
                            {
                                INT32 b208 = (prWmvDecPrm->prICOMPS->i4BoundaryUMVIcompEnable)+
                                       (prWmvDecPrm->prICOMPS->NewTopField.New.i4Enable << 4)+
                                       (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Enable << 8)+
                                       (0 << 12)+
                                       (prWmvDecPrm->prICOMPS->NewBotField.New.i4Enable << 16)+
                                       (0 << 20);
                
                                vVDecWriteMC(u4VDecID, RW_MC_WMV9_ICOMP_EN, ICOMP_C_EN + ICOMP_Y_EN);
                                vVDecWriteMC(u4VDecID, RW_MC_ICOMP2_EN, b208);
                                vVDecWriteMC(u4VDecID, RW_MC_SAME_ICOMP, NO_USE_SAME_ICOPM1_FOR_FRAME);
                            }
                        }
                        else
                        {
                            vVDecWriteMC(u4VDecID, RW_MC_WMV9_ICOMP_EN, ICOMP_C_OFF + ICOMP_Y_OFF);
                            vVDecWriteMC(u4VDecID, RW_MC_ICOMP2_EN, 0);
                            vVDecWriteMC(u4VDecID, RW_MC_SAME_ICOMP, NO_USE_SAME_ICOPM1_FOR_FRAME);
                        }
                    }
                }
                else// current field is bottom field 
                {
                 
                    //
                    // Step1: No update in the Icomp parameters
                    //
                   
                    //
                    // Step2: output MC parameters   
                    //
          
                    //
                    // Last second P field picture is Top Field 
                    //
                    //
                    // (1) Forward TF == NULL
                    //
                    // (2) Forward BF == OLD_BF
                    // (3) Backward TF == NULL
                    // (4) Backward BF == NEW_BF
          
                    if (prWmvDecPrm->prICOMPS->i4SecondFieldParity == 0)
                    {
                        // icomp_set2 is forward bottom field new icomp parameters
                        if (prWmvDecPrm->prICOMPS->OldBotField.New.i4Enable == 1)
                        {         
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X1, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 1));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X3, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 3));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X5, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 5));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X7, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 7));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X9, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 9));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X11, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 11));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X13, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 13));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X15, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 15));
                            vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF2, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Shift + 32));
                            vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF2, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale)));
                        }
                         
                        // icomp_set4 is forward bottom field old icomp parameters
                        if (prWmvDecPrm->prICOMPS->OldBotField.Old.i4Enable == 1)
                        {     
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X1, (prWmvDecPrm->prICOMPS->OldBotField.Old.i4Scale * 1));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X3, (prWmvDecPrm->prICOMPS->OldBotField.Old.i4Scale * 3));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X5, (prWmvDecPrm->prICOMPS->OldBotField.Old.i4Scale * 5));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X7, (prWmvDecPrm->prICOMPS->OldBotField.Old.i4Scale * 7));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X9, (prWmvDecPrm->prICOMPS->OldBotField.Old.i4Scale * 9));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X11, (prWmvDecPrm->prICOMPS->OldBotField.Old.i4Scale * 11));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X13, (prWmvDecPrm->prICOMPS->OldBotField.Old.i4Scale * 13));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X15, (prWmvDecPrm->prICOMPS->OldBotField.Old.i4Scale * 15));
                            vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF4, (prWmvDecPrm->prICOMPS->OldBotField.Old.i4Shift + 32));
                            vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF4, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->OldBotField.Old.i4Scale)));
                        }
            
                        // icomp_set5 is backward top or bottom field icomp parameters
                        if (prWmvDecPrm->prICOMPS->NewBotField.New.i4Enable == 1)
                        {
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X1, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 1));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X3, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 3));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X5, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 5));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X7, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 7));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X9, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 9));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X11, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 11));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X13, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 13));
                            vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X15, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 15));
                            vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF5, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Shift + 32));
                            vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF5, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale)));
                            vVDecWriteMC(u4VDecID, RW_MC_BWD_ICOMP_FLD, BWD_BTM_FLD);
                        }
                                 
                        if (prWmvDecPrm->prICOMPS->i4BoundaryUMVIcomp == 1)        
                        {
                            /* top field picture */
                            if (prWmvDecPrm->prICOMPS->i4SecondFieldParity == 0)
                            {
                                if (prWmvDecPrm->prICOMPS->OldBotField.New.i4Enable == 1)
                                {         
                                    // icomp_set4 is forward bottom field old icomp parameters
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X1, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 1));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X3, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 3));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X5, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 5));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X7, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 7));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X9, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 9));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X11, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 11));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X13, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 13));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X15, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 15));
                                    vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF6, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Shift + 32));
                                    vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF6, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale)));
                                }
                            }
                            /* bottom field picture */         
                            else
                            {
                                if (prWmvDecPrm->prICOMPS->OldTopField.New.i4Enable == 1)
                                {             
                                    // icomp_set4 is forward bottom field old icomp parameters
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X1, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 1));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X3, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 3));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X5, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 5));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X7, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 7));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X9, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 9));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X11, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 11));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X13, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 13));
                                    vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X15, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 15));
                                    vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF6, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Shift + 32));
                                    vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF6, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale)));
                                }
                            }
                        }
              
                        if (
                          (prWmvDecPrm->prICOMPS->OldBotField.New.i4Enable == 1) ||
                          (prWmvDecPrm->prICOMPS->OldBotField.Old.i4Enable == 1) ||
                          (prWmvDecPrm->prICOMPS->NewBotField.New.i4Enable == 1) ||
                          (prWmvDecPrm->prICOMPS->i4BoundaryUMVIcompEnable == 1)   
                        )
                        {
                            // Icomp enable bits
                            {
                                INT32 b208 = (prWmvDecPrm->prICOMPS->i4BoundaryUMVIcompEnable)+
                                       (prWmvDecPrm->prICOMPS->NewBotField.New.i4Enable << 4)+
                                       (prWmvDecPrm->prICOMPS->OldBotField.Old.i4Enable << 8)+
                                       (0 << 12)+
                                       (prWmvDecPrm->prICOMPS->OldBotField.New.i4Enable << 16)+
                                       (0 << 20);
                
                                vVDecWriteMC(u4VDecID, RW_MC_WMV9_ICOMP_EN, ICOMP_C_EN + ICOMP_Y_EN);
                                vVDecWriteMC(u4VDecID, RW_MC_ICOMP2_EN, b208);
                                vVDecWriteMC(u4VDecID, RW_MC_SAME_ICOMP, NO_USE_SAME_ICOPM1_FOR_FRAME);
                            }
                        }
                        else
                        {
                            vVDecWriteMC(u4VDecID, RW_MC_WMV9_ICOMP_EN, ICOMP_C_OFF + ICOMP_Y_OFF);
                            vVDecWriteMC(u4VDecID, RW_MC_ICOMP2_EN, 0);
                            vVDecWriteMC(u4VDecID, RW_MC_SAME_ICOMP, NO_USE_SAME_ICOPM1_FOR_FRAME);
                        }       
                    }
                    else
                    {
                          //
                          // Last second P field picture is Bottom Field 
                          //
                          //
                          // (1) Forward TF == NULL
                          //
                          // (2) Forward BF == NEW_BF
                          // (3) Backward TF == NEW_TF
                          // (4) Backward BF == NULL
                          
                          // icomp_set2 is forward bottom field new icomp parameters
                          if (prWmvDecPrm->prICOMPS->NewBotField.New.i4Enable == 1)
                          {         
                              vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X1, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 1));
                              vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X3, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 3));
                              vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X5, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 5));
                              vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X7, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 7));
                              vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X9, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 9));
                              vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X11, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 11));
                              vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X13, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 13));
                              vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X15, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 15));
                              vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF2, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Shift + 32));
                              vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF2, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale)));
                          }
                            
                          // icomp_set4 is forward bottom field old icomp parameters
                          if (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Enable == 1)
                          {          
                              vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X1, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 1));
                              vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X3, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 3));
                              vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X5, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 5));
                              vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X7, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 7));
                              vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X9, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 9));
                              vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X11, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 11));
                              vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X13, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 13));
                              vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X15, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 15));
                              vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF4, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Shift + 32));
                              vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF4, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale)));
                          }
              
                          // icomp_set5 is backward top or bottom field icomp parameters
                
                          if (prWmvDecPrm->prICOMPS->NewTopField.New.i4Enable == 1)
                          {
                              vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X1, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 1));
                              vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X3, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 3));
                              vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X5, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 5));
                              vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X7, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 7));
                              vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X9, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 9));
                              vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X11, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 11));
                              vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X13, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 13));
                              vVDecWriteMC(u4VDecID, RW_MC_ISCALE5_X15, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 15));
                              vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF5, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Shift + 32));
                              vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF5, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale)));
                              vVDecWriteMC(u4VDecID, RW_MC_BWD_ICOMP_FLD, BWD_TOP_FLD);
                          }
              
                     
                          if (prWmvDecPrm->prICOMPS->i4BoundaryUMVIcomp == 1)        
                          {
                              /* top field picture */
                              if (prWmvDecPrm->prICOMPS->i4SecondFieldParity == 0)  
                              {
                                  if (prWmvDecPrm->prICOMPS->OldBotField.New.i4Enable == 1)
                                  {         
                                      // icomp_set4 is forward bottom field old icomp parameters
                                      vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X1, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 1));
                                      vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X3, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 3));
                                      vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X5, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 5));
                                      vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X7, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 7));
                                      vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X9, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 9));
                                      vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X11, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 11));
                                      vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X13, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 13));
                                      vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X15, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale * 15));
                                      vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF6, (prWmvDecPrm->prICOMPS->OldBotField.New.i4Shift + 32));
                                      vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF6, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->OldBotField.New.i4Scale)));
                                  }
                              }
                              else /* bottom field picture */         
                              {
                                  if (prWmvDecPrm->prICOMPS->OldTopField.New.i4Enable == 1)
                                  {             
                                      // icomp_set4 is forward bottom field old icomp parameters
                                      vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X1, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 1));
                                      vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X3, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 3));
                                      vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X5, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 5));
                                      vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X7, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 7));
                                      vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X9, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 9));
                                      vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X11, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 11));
                                      vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X13, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 13));
                                      vVDecWriteMC(u4VDecID, RW_MC_ISCALE6_X15, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale * 15));
                                      vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF6, (prWmvDecPrm->prICOMPS->OldTopField.New.i4Shift + 32));
                                      vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF6, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->OldTopField.New.i4Scale)));
                                  }
                              }
                          }
              
                          if (
                            (prWmvDecPrm->prICOMPS->NewBotField.New.i4Enable == 1) ||
                            (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Enable == 1) ||
                            (prWmvDecPrm->prICOMPS->NewTopField.New.i4Enable == 1) ||
                            (prWmvDecPrm->prICOMPS->i4BoundaryUMVIcompEnable == 1)
                          )
                          {
                              // Icomp enable bits
                              {
                                  INT32 b208 = (prWmvDecPrm->prICOMPS->i4BoundaryUMVIcompEnable)+
                                         (prWmvDecPrm->prICOMPS->NewTopField.New.i4Enable << 4)+
                                         (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Enable << 8) +
                                         (0 << 12)+
                                         (prWmvDecPrm->prICOMPS->NewBotField.New.i4Enable << 16) +
                                         (0 << 20);
                  
                                  vVDecWriteMC(u4VDecID, RW_MC_WMV9_ICOMP_EN, ICOMP_C_EN + ICOMP_Y_EN);
                                  vVDecWriteMC(u4VDecID, RW_MC_ICOMP2_EN, b208);
                                  vVDecWriteMC(u4VDecID, RW_MC_SAME_ICOMP, NO_USE_SAME_ICOPM1_FOR_FRAME);
                              }
                          }
                          else
                          {
                              vVDecWriteMC(u4VDecID, RW_MC_WMV9_ICOMP_EN, ICOMP_C_OFF + ICOMP_Y_OFF);
                              vVDecWriteMC(u4VDecID, RW_MC_ICOMP2_EN, 0);
                              vVDecWriteMC(u4VDecID, RW_MC_SAME_ICOMP, NO_USE_SAME_ICOPM1_FOR_FRAME);
                          }
                    }     
                }
            }
        }
        else// B frame pciture
        { 
            // (1) Forward TF == NEW_TF
            // (2) Forward BF == NEW_BF
            // (3) Backward TF == NULL
            // (4) Backward BF == NULL
             
            /* Step 3: output MC parameters */
            // icomp_set1 is forward top field new icomp parameters 
            if (prWmvDecPrm->prICOMPS->NewTopField.New.i4Enable == 1)
            { 
                vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X1, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 1));
                vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X3, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 3));
                vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X5, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 5));
                vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X7, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 7));
                vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X9, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 9));
                vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X11, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 11));
                vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X13, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 13));
                vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X15, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale * 15));
                vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF1, (prWmvDecPrm->prICOMPS->NewTopField.New.i4Shift + 32));
                vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF1, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->NewTopField.New.i4Scale)));
            }
      
            //  icomp_set2 is forward bottom field new icomp parameters
            if (prWmvDecPrm->prICOMPS->NewBotField.New.i4Enable == 1)
            {      
                vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X1, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 1));
                vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X3, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 3));
                vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X5, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 5));
                vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X7, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 7));
                vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X9, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 9));
                vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X11, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 11));
                vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X13, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 13));
                vVDecWriteMC(u4VDecID, RW_MC_ISCALE2_X15, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale * 15));
                vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF2, (prWmvDecPrm->prICOMPS->NewBotField.New.i4Shift + 32));
                vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF2, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->NewBotField.New.i4Scale)));
            }
      
            if (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Enable == 1)
            {
                // icomp_set3 is forward top field old icomp parameters
                vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X1, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale * 1));
                vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X3, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale * 3));
                vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X5, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale * 5));
                vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X7, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale * 7));
                vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X9, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale * 9));
                vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X11, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale * 11));
                vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X13, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale * 13));
                vVDecWriteMC(u4VDecID, RW_MC_ISCALE3_X15, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale * 15));
                vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF3, (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Shift + 32));
                vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF3, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->NewTopField.Old.i4Scale)));
            }
      
            if (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Enable == 1)
            {
                // icomp_set4 is forward bottom field old icomp parameters
                vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X1, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 1));
                vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X3, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 3));
                vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X5, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 5));
                vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X7, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 7));
                vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X9, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 9));
                vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X11, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 11));
                vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X13, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 13));
                vVDecWriteMC(u4VDecID, RW_MC_ISCALE4_X15, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale * 15));
                vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF4, (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Shift + 32));
                vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF4, ((128 * 64) + 32 - (128*prWmvDecPrm->prICOMPS->NewBotField.Old.i4Scale)));
            }
      
            if (
              (prWmvDecPrm->prICOMPS->NewTopField.New.i4Enable == 1) ||
              (prWmvDecPrm->prICOMPS->NewBotField.New.i4Enable == 1) ||
              (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Enable == 1) ||
              (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Enable == 1)
            )
            {
                // Icomp enable bits
                {
                    INT32 b208 = (prWmvDecPrm->prICOMPS->NewBotField.Old.i4Enable << 8)+
                           (prWmvDecPrm->prICOMPS->NewTopField.Old.i4Enable <<12)+
                           (prWmvDecPrm->prICOMPS->NewBotField.New.i4Enable <<16)+
                           (prWmvDecPrm->prICOMPS->NewTopField.New.i4Enable <<20);
        
                    vVDecWriteMC(u4VDecID, RW_MC_WMV9_ICOMP_EN, ICOMP_C_EN + ICOMP_Y_EN);
                    vVDecWriteMC(u4VDecID, RW_MC_ICOMP2_EN, b208);
                    vVDecWriteMC(u4VDecID, RW_MC_SAME_ICOMP, NO_USE_SAME_ICOPM1_FOR_FRAME);
                }
            }
            else
            {
                vVDecWriteMC(u4VDecID, RW_MC_WMV9_ICOMP_EN, ICOMP_C_OFF + ICOMP_Y_OFF);
                vVDecWriteMC(u4VDecID, RW_MC_ICOMP2_EN, 0);
                vVDecWriteMC(u4VDecID, RW_MC_SAME_ICOMP, NO_USE_SAME_ICOPM1_FOR_FRAME);
            }
        }
    
    }  //}
    else /* pure frame picture case in WMV9 */
    {
    
        if (prWmvDecPrm->i4CodecVersion >= VDEC_WMV3)
        {
            vVDecWriteMC(u4VDecID, RW_MC_SAME_ICOMP, USE_SAME_ICOMP1_FOR_FRAME);
        }
    
        if (prWmvDecPrm->prPPS->ucPicType == IVOP)
        {
            vVDecWriteMC(u4VDecID, RW_MC_WMV9_ICOMP_EN, ICOMP_C_OFF + ICOMP_Y_OFF);
            if (prWmvDecPrm->i4CodecVersion >= VDEC_WMV3)
            {
                vVDecWriteMC(u4VDecID, RW_MC_ICOMP_TYPE, NO_ICOMP);
            }
        }
        else if  ( (prWmvDecPrm->prPPS->ucPicType == PVOP)  || (prWmvDecPrm->prPPS->ucPicType == SKIPFRAME) )/* only for PVOP */
        {
            if (prWmvDecPrm->prPPS->fgLuminanceWarp)
            {
                vVDecWriteMC(u4VDecID, RW_MC_WMV9_ICOMP_EN, ICOMP_C_EN + ICOMP_Y_EN);
                if (prWmvDecPrm->i4CodecVersion >= VDEC_WMV3)
                {
                    vVDecWriteMC(u4VDecID, RW_MC_ICOMP_TYPE, FRAME_ICOMP);
                }
            }
            else
            {
                vVDecWriteMC(u4VDecID, RW_MC_WMV9_ICOMP_EN, ICOMP_C_OFF + ICOMP_Y_OFF);
                if (prWmvDecPrm->i4CodecVersion >= VDEC_WMV3)
                {
                    vVDecWriteMC(u4VDecID, RW_MC_ICOMP_TYPE, NO_ICOMP);
                }
            }
        }
        else if (prWmvDecPrm->prPPS->ucPicType == BVOP)
        {
            /* just follow the setting of last decoded P picture */
            if (prWmvDecPrm->prICOMPS->i4FirstFieldIntensityComp == 1)
            {
                vVDecWriteMC(u4VDecID, RW_MC_WMV9_ICOMP_EN, ICOMP_C_EN + ICOMP_Y_EN);
                if (prWmvDecPrm->i4CodecVersion >= VDEC_WMV3)
                {
                    vVDecWriteMC(u4VDecID, RW_MC_ICOMP_TYPE, FRAME_ICOMP);
                }
            }
            else
            {
                vVDecWriteMC(u4VDecID, RW_MC_WMV9_ICOMP_EN, ICOMP_C_OFF + ICOMP_Y_OFF);
                if (prWmvDecPrm->i4CodecVersion >= VDEC_WMV3)
                {
                    vVDecWriteMC(u4VDecID, RW_MC_ICOMP_TYPE, NO_ICOMP);
                }
            }
        }
    
    
        // remap luminance scale and shift
        if (prWmvDecPrm->prPPS->i4LumScale == 0)
        {
            i4Scale = - 64;
            i4Shift = 255 * 64 - prWmvDecPrm->prPPS->i4LumShift * 2 * 64;
        }
        else
        {
            i4Scale = prWmvDecPrm->prPPS->i4LumScale  + 32;
            i4Shift = prWmvDecPrm->prPPS->i4LumShift * 64;
        }
    
        ///// MC c208 register
        {
            INT32 c208;
      
            if (prWmvDecPrm->prICOMPS->i4FirstFieldIntensityComp == 1)
            {
                c208 = (0) + (0 << 4) + (0 << 8) + (0 << 12) + (0 << 16) + (1 << 20);
            }
            else
            {
                c208 = 0;
            }
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X1, (i4Scale * 1));
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X3, (i4Scale * 3));
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X5, (i4Scale * 5));
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X7, (i4Scale * 7));
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X9, (i4Scale * 9));
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X11, (i4Scale * 11));
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X13, (i4Scale * 13));
            vVDecWriteMC(u4VDecID, RW_MC_ISCALE1_X15, (i4Scale * 15));
            vVDecWriteMC(u4VDecID, RW_MC_YSHIFT_OFF1, (i4Shift + 32));
            vVDecWriteMC(u4VDecID, RW_MC_CSHIFT_OFF1, ((128 * 64) + 32 - (128*i4Scale)));
            if (prWmvDecPrm->i4CodecVersion >= VDEC_WMV3)
            {
                vVDecWriteMC(u4VDecID, RW_MC_ICOMP2_EN, c208);
            }
        }
    }
  
    //-------------------------------------------------------
    // MC_reg_168 : MV type (0:BILINEAR, 1:BICUBIC)
    //-------------------------------------------------------
    if (prWmvDecPrm->i4CodecVersion >= VDEC_WMV3)
    {
        if (prWmvDecPrm->prPPS->i4X9MVMode == ALL_1MV_HALFPEL_BILINEAR){ /* (pWMVDec->m_iFilterType == FILTER_BICUBIC) */
            vVDecWriteMC(u4VDecID, RW_MC_FILTER_TYPE, (Y_BILINEAR + C_BILINEAR));
        }
        else {/* FILTER_BILINEAR */
            vVDecWriteMC(u4VDecID, RW_MC_FILTER_TYPE, (Y_BICUBIC + C_BILINEAR));
        }
    }
    else /* prWmvDecPrm->i4CodecVersion == VDEC_WMV2  */
    {
        if ( (prWmvDecPrm->i4CodecVersion == VDEC_WMV2) && (prWmvDecPrm->prPPS->fgMvResolution == 1) )
        {
            vVDecWriteMC(u4VDecID, RW_MC_FILTER_TYPE, (Y_BICUBIC + C_BILINEAR));
        }
        else
        {
            vVDecWriteMC(u4VDecID, RW_MC_FILTER_TYPE, (Y_BILINEAR + C_BILINEAR));
        }
    }
  
    //-------------------------------------------------------
    // MC_reg_169 : MC clip signal (1:turn on)
    //-------------------------------------------------------
    if (prWmvDecPrm->prPPS->i4Overlap & 1)
    {  /* clip operation will be handled by overlap_smooth module */
        vVDecWriteMC(u4VDecID, RW_MC_WRITE_BUS_TYPE, UNCLIP_TYPE);
    }
    else
    {
        vVDecWriteMC(u4VDecID, RW_MC_WRITE_BUS_TYPE, CLIP_0_255_TYPE);
    }
  
  
    //----------------------------------------------------------------------
    // MC_reg_170 : indicate if the intra_blocks need to add 128 (1:turn on)
    //----------------------------------------------------------------------
    if (prWmvDecPrm->i4CodecVersion >= VDEC_WMV3)
    {
        //simple/main I
        if ((prWmvDecPrm->prPPS->ucPicType == IVOP || prWmvDecPrm->prPPS->ucPicType == BIVOP) && (prWmvDecPrm->i4CodecVersion == VDEC_WMV3))
        {
            vVDecWriteMC(u4VDecID, RW_MC_INTRA_BLK_ADD128, ADD128_OFF);
        }
        else
        {
            if (prWmvDecPrm->prPPS->ucFrameCodingMode == INTERLACEFRAME)
            {
                vVDecWriteMC(u4VDecID, RW_MC_INTRA_BLK_ADD128, ADD128_ON);
            }
            else if (prWmvDecPrm->prPPS->i4Overlap & 1) {/* add 128 will be handled by overlap_smooth module */
                vVDecWriteMC(u4VDecID, RW_MC_INTRA_BLK_ADD128, ADD128_OFF);
            }
            else {/* enable add 128 to intra mb */
                vVDecWriteMC(u4VDecID, RW_MC_INTRA_BLK_ADD128, ADD128_ON);
            }
        }
    }
    else /* prWmvDecPrm->i4CodecVersion == VDEC_WMV2 */
    {
        if (prWmvDecPrm->prPPS->ucPicType == PVOP)
        {
            vVDecWriteMC(u4VDecID, RW_MC_INTRA_BLK_ADD128, ADD128_OFF);
        }
        else /* IVOP */
        {
            vVDecWriteMC(u4VDecID, RW_MC_INTRA_BLK_ADD128, ADD128_OFF);
        }
    }
  
    //------------------------------------------
    // MC_reg_173 : fast_uvmc (1:turn on)
    //------------------------------------------
    vVDecWriteMC(u4VDecID, RW_MC_FAST_UVMC, prWmvDecPrm->prEPS->fgUVHpelBilinear);
  
    //
    // In-loop deblocking parameters
    //
  
    //------------------------------------------
    // MC_reg_136,148 : post process (1:turn on)
    //------------------------------------------
    if ((prWmvDecPrm->prEPS->fgLoopFilter == 1) || (prWmvDecPrm->prPPS->i4Overlap & 1))
    {
        vVDecWriteMC(u4VDecID, RW_MC_PP_ENABLE, 0x00000001);
        vVDecWriteMC(u4VDecID, RW_MC_PP_WB_BY_POST, 0x00000001);
    }
    else
    {
        vVDecWriteMC(u4VDecID, RW_MC_PP_ENABLE, 0x00000000);
        vVDecWriteMC(u4VDecID, RW_MC_PP_WB_BY_POST, 0x00000000);
    }
  
    //------------------------------------------
    // MC_reg_142 : loopfielter (3:turn on)
    //------------------------------------------
    if (prWmvDecPrm->prEPS->fgLoopFilter == 1) 
    {
        vVDecWriteMC(u4VDecID, RW_MC_PP_DBLK_MODE, 0x00000003);
    }
    else
    {
        vVDecWriteMC(u4VDecID, RW_MC_PP_DBLK_MODE, 0x00000000);
    }
  
    //------------------------------------------
    // MC_reg_175 : overlap smoothing (1:turn on)
    //------------------------------------------
    if (prWmvDecPrm->prPPS->i4Overlap & 1) 
    {
        vVDecWriteMC(u4VDecID, RW_MC_OVL_SMTH_FILTER, OVL_EN);
    }
    else
    {
        vVDecWriteMC(u4VDecID, RW_MC_OVL_SMTH_FILTER, OVL_OFF);
    }
  
    //------------------------------------------
    // MC_reg_152,153 : start_MB # & end_MB #
    //------------------------------------------
    vVDecWriteMC(u4VDecID, RW_MC_PP_X_RANGE, prWmvDecPrm->prSPS->u4NumMBX-1);
    vVDecWriteMC(u4VDecID, RW_MC_PP_Y_RANGE, prWmvDecPrm->prSPS->u4NumMBY-1);
  
    //------------------------------------------
    // MC_reg_139 : picture width in MB
    //------------------------------------------
    vVDecWriteMC(u4VDecID, RW_MC_PP_MB_WIDTH, prWmvDecPrm->prSPS->u4NumMBX);
  
    //-----------------------------------------------------
    // MC_reg_137,138 : dram base address for post process
    //-----------------------------------------------------
    if ((prWmvDecPrm->prPPS->ucPicType == BVOP) || (prWmvDecPrm->prPPS->ucPicType == BIVOP)) //prog_B
    {
        #if VMMU_SUPPORT
        {
        UINT32 u4Page_addr = 0;
        
        u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((prWmvDecPrm->rWmvWorkBufSa.u4Pic2YSa)/(4*1024))*4);
        printk("[WMV] i4VDEC_HAL_WMV_DecStart, 7, Page Addr = 0x%x\n", u4Page_addr);    
        vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic2YSa), PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic2YSa) + PIC_Y_SZ);
        vVDecWriteMC(u4VDecID, RW_MC_PP_Y_ADDR, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic2YSa) >> prWmvDecPrm->i4MemBase >> 9); 

        u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((prWmvDecPrm->rWmvWorkBufSa.u4Pic2CSa)/(4*1024))*4);
        printk("[WMV] i4VDEC_HAL_WMV_DecStart, 8, Page Addr = 0x%x\n", u4Page_addr);    
        vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic2CSa), PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic2CSa) + PIC_C_SZ);
        vVDecWriteMC(u4VDecID, RW_MC_PP_C_ADDR, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic2CSa) >> prWmvDecPrm->i4MemBase >> 8);
        }
        #else
        vVDecWriteMC(u4VDecID, RW_MC_PP_Y_ADDR, (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic2YSa) >> prWmvDecPrm->i4MemBase) >> 9);
        vVDecWriteMC(u4VDecID, RW_MC_PP_C_ADDR, (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic2CSa) >> prWmvDecPrm->i4MemBase) >> 8);
        #endif
    }
    else if (prDecPrm->ucDecFBufIdx== 0) //pic1
    {
        #if VMMU_SUPPORT
        {
        UINT32 u4Page_addr = 0;
        
        u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((prWmvDecPrm->rWmvWorkBufSa.u4Pic0YSa)/(4*1024))*4);
        printk("[WMV] i4VDEC_HAL_WMV_DecStart, 9, Page Addr = 0x%x\n", u4Page_addr);    
        vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic0YSa), PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic0YSa) + PIC_Y_SZ);
        vVDecWriteMC(u4VDecID, RW_MC_PP_Y_ADDR, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic0YSa) >> prWmvDecPrm->i4MemBase >> 9); 

        u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((prWmvDecPrm->rWmvWorkBufSa.u4Pic0CSa)/(4*1024))*4);
        printk("[WMV] i4VDEC_HAL_WMV_DecStart, 10, Page Addr = 0x%x\n", u4Page_addr);    
        vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic0CSa), PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic0CSa) + PIC_C_SZ);
        vVDecWriteMC(u4VDecID, RW_MC_PP_C_ADDR, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic0CSa) >> prWmvDecPrm->i4MemBase >> 8);
        }
        #else
        vVDecWriteMC(u4VDecID, RW_MC_PP_Y_ADDR, (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic0YSa) >> prWmvDecPrm->i4MemBase) >> 9);
        vVDecWriteMC(u4VDecID, RW_MC_PP_C_ADDR, (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic0CSa) >> prWmvDecPrm->i4MemBase) >> 8);
        #endif
    }
    else //pic2
    {
        #if VMMU_SUPPORT
        {
        UINT32 u4Page_addr = 0;
        
        u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((prWmvDecPrm->rWmvWorkBufSa.u4Pic1YSa)/(4*1024))*4);
        printk("[WMV] i4VDEC_HAL_WMV_DecStart, 11, Page Addr = 0x%x\n", u4Page_addr);    
        vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic1YSa), PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic1YSa) + PIC_Y_SZ);
        vVDecWriteMC(u4VDecID, RW_MC_PP_Y_ADDR, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic1YSa) >> prWmvDecPrm->i4MemBase >> 9); 

        u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((prWmvDecPrm->rWmvWorkBufSa.u4Pic1CSa)/(4*1024))*4);
        printk("[WMV] i4VDEC_HAL_WMV_DecStart, 12, Page Addr = 0x%x\n", u4Page_addr);    
        vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic1CSa), PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic1CSa) + PIC_C_SZ);
        vVDecWriteMC(u4VDecID, RW_MC_PP_C_ADDR, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic1CSa) >> prWmvDecPrm->i4MemBase >> 8);
        }
        #else
        vVDecWriteMC(u4VDecID, RW_MC_PP_Y_ADDR, (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic1YSa) >> prWmvDecPrm->i4MemBase) >> 9);
        vVDecWriteMC(u4VDecID, RW_MC_PP_C_ADDR, (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic1CSa) >> prWmvDecPrm->i4MemBase) >> 8);
        #endif
    }
  
    if (prWmvDecPrm->i4CodecVersion == VDEC_VC1) 
    {
        //-----------------------------------------------------
        // MC_reg_176 : conditional overlap flag (1:turn on)
        //-----------------------------------------------------
        vVDecWriteMC(u4VDecID, RW_MC_COND_OVL_FILTER, (prWmvDecPrm->prPPS->i4Overlap & 4)? COND_OVL_EN : COND_OVL_OFF);
    
        //-----------------------------------------------------
        // MC_reg_177 : for fix MS ref_code bug (bit[0] [8] [16])
        //-----------------------------------------------------
        vVDecWriteMC(u4VDecID, RW_MC_PP_DBLK_OPT, VC1_ALL_SET_ONE);
    
        //-----------------------------------------------------
        // MC_reg_184 : Range Map flag (bit1:y, bit0:c)
        //-----------------------------------------------------
    
        //-----------------------------------------------------
        // MC_reg_185 : Range Map parameter
        //-----------------------------------------------------
        //Give it to Display module.
        vVDecWriteMC(u4VDecID, RW_MC_RNG_PARA, ((prWmvDecPrm->prEPS->i4RangeRedY>0)? prWmvDecPrm->prEPS->i4RangeRedY-1:0)*256 + ((prWmvDecPrm->prEPS->i4RangeMapUV>0)? prWmvDecPrm->prEPS->i4RangeMapUV-1:0));
    }
    else
    {
        //-----------------------------------------------------
        // MC_reg_177 : for fix MS ref_code bug (bit[0] [8] [16])
        //-----------------------------------------------------
        vVDecWriteMC(u4VDecID, RW_MC_PP_DBLK_OPT, NOT_VC1_ALL_SET_ZERO);
    }
  
  
    //---------------------------------------------------------
    // MC_reg_231 : referece picture
    //---------------------------------------------------------
    if (prWmvDecPrm->i4CodecVersion >= VDEC_WMV3)
    {
        INT32 b231 = ((prWmvDecPrm->prPPS->u4BackwardRefPicType == 0)? BWD_PROG_REF_PIC: BWD_INTLCE_REF_PIC) +
                   (((prWmvDecPrm->prPPS->u4ForwardRefPicType == 0)? FWD_PROG_REF_PIC : FWD_INTLCE_REF_PIC));//0: 1) << 16);
        vVDecWriteMC(u4VDecID, RW_MC_REF_PIC_TYPE, b231);
    }
  
  
    //------------------------------
    //   PRED RISC WRITE PATTERN
    //------------------------------
  
    //-----------------------------------------------------
    // VLD_reg_143 : prediction parameter
    //-----------------------------------------------------
  
    //-----------------------------------------------------
    // VLD_reg_193 : dcac prediction rule selection
    //-----------------------------------------------------
    //for dc_pred rule selection : Rule_A or Rule_B
    if ((prWmvDecPrm->i4CodecVersion == VDEC_WMV1) || (prWmvDecPrm->i4CodecVersion == VDEC_WMV2))
    {
        use_interpredictor = (prWmvDecPrm->prSPS->i4BitRate <= MAX_BITRATE_DCPred_IMBInPFrame && (prWmvDecPrm->prSPS->u4PicWidthSrc * prWmvDecPrm->prSPS->u4PicHeightSrc < 320 * 240)) &&
    	 					             (prWmvDecPrm->prPPS->ucPicType == PVOP) &&
    	 					             (prWmvDecPrm->i4CodecVersion == VDEC_WMV1);
        use_rule_b =  (((prWmvDecPrm->prPPS->i4Overlap & 0x1) == 0) && (prWmvDecPrm->prPPS->ucPicType == IVOP) &&
                       ((prWmvDecPrm->prSPS->i4WMV3Profile == WMV3_SIMPLE_PROFILE) || (prWmvDecPrm->prSPS->i4WMV3Profile == WMV3_MAIN_PROFILE))) ||
    	                 use_interpredictor | (prWmvDecPrm->i4CodecVersion <= VDEC_WMV2);
        use_quarter_pel_mv = 0;
    
        a193 = 
            //0x0021c0b4 +
           (((prWmvDecPrm->prSPS->u4PicWidthSrc +15)/16)*4)+
           ((((prWmvDecPrm->prSPS->u4PicWidthSrc+15)/16)*4*3)<<12)+
           (use_rule_b << 27) +
           (use_interpredictor << 28) +
           (use_quarter_pel_mv/*(pWMVDec->m_pAltTables->m_iHalfPelMV == 0)*/<< 29) + /* quarter-pel */
           ( 0 << 30); //dcac overlap
  
    }
    else // >= VDEC_WMV3
    {
        use_interpredictor = 0;
        use_rule_b =  ((prWmvDecPrm->prPPS->i4Overlap & 0x1) == 0) &&
                       ((prWmvDecPrm->prPPS->ucPicType == IVOP) || (prWmvDecPrm->prPPS->ucPicType == BIVOP))&&
                       ((prWmvDecPrm->prSPS->i4WMV3Profile == WMV3_SIMPLE_PROFILE) || (prWmvDecPrm->prSPS->i4WMV3Profile == WMV3_MAIN_PROFILE));
        use_quarter_pel_mv = (prWmvDecPrm->prPPS->ucFrameCodingMode == INTERLACEFRAME) ? 1:
                           ((prWmvDecPrm->prPPS->i4X9MVMode == ALL_1MV_HALFPEL || prWmvDecPrm->prPPS->i4X9MVMode == ALL_1MV_HALFPEL_BILINEAR))? 0: 1;
        //printk("use_rule_b = %d\n",use_rule_b);
        //printk("i4Overlap = %d,ucPicType = 0x%x,i4WMV3Profile = %d\n",
        //      prWmvDecPrm->prPPS->i4Overlap,prWmvDecPrm->prPPS->ucPicType,prWmvDecPrm->prSPS->i4WMV3Profile);
        a193 = //0x0021c0b4 +
             (((prWmvDecPrm->prSPS->u4PicWidthSrc+15)/16)*4)+
             ((((prWmvDecPrm->prSPS->u4PicWidthSrc+15)/16)*4*3)<<12)+
           (use_rule_b << 27) +
           (use_interpredictor << 28) +
           (use_quarter_pel_mv/*(pWMVDec->m_pAltTables->m_iHalfPelMV == 0)*/<< 29) + /* quarter-pel */
           ( 0 << 30); //dcac overlap
    }
  
    vVDecWriteVLD(u4VDecID, RW_VLD_DCACPITCH, a193);
  
    //-----------------------------------------------------
    // VLD_reg_198 : bit-plane mode & address
    //-----------------------------------------------------
    a198 = ((u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Bp1Sa) >> 2) & 0xFFFFFF) +
           (((u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Bp1Sa) >> 2) & 0xF000000) << 4) +
           (pred_use_wdle << 25) +
           (1 << 26);  //wdle
    vVDecWriteVLD(u4VDecID, RW_VLD_BCODE_SA, a198); //(UINT32)prWmvDecPrm->u4Bp1Sa >> 2); //198 Bp_1
  
    //-----------------------------------------------------
    // VLD_reg_200 : MV resolution (half or full pel)
    //-----------------------------------------------------
    {
        INT32 mv_sram_use_ok, mv_sram_use, sliceoffset;
        INT32 my_forward;
        if ((prWmvDecPrm->i4CodecVersion == VDEC_WMV1) || (prWmvDecPrm->i4CodecVersion == VDEC_WMV2))
        {
            mv_sram_use_ok = 0;
            mv_sram_use = 1;
            sliceoffset = (prWmvDecPrm->i4CodecVersion == VDEC_WMV1) ? 22 : 0;
      
            a200 = 0x0020003a +
             (pred_use_wdle << 6)+
             (pred_use_wdle << 7)+
             (pred_use_wdle << 20) +
             (1<<11)+
             (((prWmvDecPrm->prPPS->i4Panning & 0xf) != 0) << 9) +
             ((prWmvDecPrm->i4CodecVersion == VDEC_WMV1 || prWmvDecPrm->i4CodecVersion == VDEC_WMV2)<<15)+
             (prWmvDecPrm->prICOMPS->i4ResetMvDram << 18) +
             ((!(prWmvDecPrm->prSPS->u4NumMBY == (prWmvDecPrm->prSPS->u4NumMBY / (prWmvDecPrm->prSPS->i4SliceCode - sliceoffset)))) << 22) +
             (mv_sram_use << 31);// +
             //((prWmvDecPrm->prPPS->i4X9MVMode == ALL_1MV_HALFPEL_BILINEAR) << 25) +
             //((prWmvDecPrm->fgUVHpelBilinear == 1) << 26)+
             //(prWmvDecPrm->i4CurrentTemporalField << 27) +
             //(prWmvDecPrm->fgVC1 << 28) +
             //(pWMVDec->m_bUseOppFieldForRef << 29);
    
        }
        else // >= VDEC_WMV3
        {
            mv_sram_use_ok = ( (prWmvDecPrm->prPPS->ucPicType == PVOP) || ((prWmvDecPrm->prPPS->ucPicType == BVOP)&&(prWmvDecPrm->prPPS->ucFrameCodingMode != INTERLACEFIELD)) ) &&
                               //(prWmvDecPrm->prSPS->u4NumMBX < 46);
                               (prWmvDecPrm->prSPS->u4NumMBX < 128);
            mv_sram_use = mv_sram_use_ok;// // simulation use only ? (rand()% 2) : 0;
      
            my_forward = u4VDecReadMC(u4VDecID, RW_MC_FWDP);
            //my_output = u4VDecReadMC(u4VDecID, RW_MC_OPBUF);
      
            //vSetMcBufPtr();

            #if VMMU_SUPPORT
            {
            UINT32 u4Page_addr = 0;
            
            u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((prWmvDecPrm->rWmvWorkBufSa.u4Pic0YSa)/(4*1024))*4);
            printk("[WMV] i4VDEC_HAL_WMV_DecStart, 13, Page Addr = 0x%x\n", u4Page_addr);    
            vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic0YSa), PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic0YSa) + PIC_Y_SZ);
            vVDecWriteMC(u4VDecID, RW_MC_R1Y, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic0YSa) >> prWmvDecPrm->i4MemBase >> 9); 

            u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((prWmvDecPrm->rWmvWorkBufSa.u4Pic0CSa)/(4*1024))*4);
            printk("[WMV] i4VDEC_HAL_WMV_DecStart, 14, Page Addr = 0x%x\n", u4Page_addr);    
            vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic0CSa), PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic0CSa) + PIC_C_SZ);
            vVDecWriteMC(u4VDecID, RW_MC_R1C, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic0CSa) >> prWmvDecPrm->i4MemBase >> 8); 

            u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((prWmvDecPrm->rWmvWorkBufSa.u4Pic1YSa)/(4*1024))*4);
            printk("[WMV] i4VDEC_HAL_WMV_DecStart, 15, Page Addr = 0x%x\n", u4Page_addr);    
            vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic1YSa), PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic1YSa) + PIC_Y_SZ);
            vVDecWriteMC(u4VDecID, RW_MC_R2Y, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic1YSa) >> prWmvDecPrm->i4MemBase >> 9); 

            u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((prWmvDecPrm->rWmvWorkBufSa.u4Pic1CSa)/(4*1024))*4);
            printk("[WMV] i4VDEC_HAL_WMV_DecStart, 16, Page Addr = 0x%x\n", u4Page_addr);    
            vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic1CSa), PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic1CSa) + PIC_C_SZ);
            vVDecWriteMC(u4VDecID, RW_MC_R2C, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic1CSa) >> prWmvDecPrm->i4MemBase >> 8); 

            u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((prWmvDecPrm->rWmvWorkBufSa.u4Pic2YSa)/(4*1024))*4);
            printk("[WMV] i4VDEC_HAL_WMV_DecStart, 17, Page Addr = 0x%x\n", u4Page_addr);    
            vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic2YSa), PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic2YSa) + PIC_Y_SZ);
            vVDecWriteMC(u4VDecID, RW_MC_BY, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic2YSa) >> prWmvDecPrm->i4MemBase >> 8); 
            vVDecWriteMC(u4VDecID, RW_MC_BY1, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic2YSa) >> prWmvDecPrm->i4MemBase >> 9); 

            u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((prWmvDecPrm->rWmvWorkBufSa.u4Pic2CSa)/(4*1024))*4);
            printk("[WMV] i4VDEC_HAL_WMV_DecStart, 18, Page Addr = 0x%x\n", u4Page_addr);    
            vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic2CSa), PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic2CSa) + PIC_C_SZ);
            vVDecWriteMC(u4VDecID, RW_MC_BC, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic2CSa) >> prWmvDecPrm->i4MemBase >> 7); 
            vVDecWriteMC(u4VDecID, RW_MC_BC1, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic2CSa) >> prWmvDecPrm->i4MemBase >> 8); 
            }
            #else 
            vVDecWriteMC(u4VDecID, RW_MC_R1Y, (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic0YSa) >> prWmvDecPrm->i4MemBase) >> 9); // div 512
            vVDecWriteMC(u4VDecID, RW_MC_R1C, (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic0CSa) >> prWmvDecPrm->i4MemBase) >> 8); // div 256
            vVDecWriteMC(u4VDecID, RW_MC_R2Y, (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic1YSa) >> prWmvDecPrm->i4MemBase) >> 9); // div 512
            vVDecWriteMC(u4VDecID, RW_MC_R2C, (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic1CSa) >> prWmvDecPrm->i4MemBase) >> 8); // div 256
            vVDecWriteMC(u4VDecID, RW_MC_BY,  (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic2YSa) >> prWmvDecPrm->i4MemBase) >> 8); // div 256
            vVDecWriteMC(u4VDecID, RW_MC_BC,  (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic2CSa) >> prWmvDecPrm->i4MemBase) >> 7); // div 128
            vVDecWriteMC(u4VDecID, RW_MC_BY1,  (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic2YSa) >> prWmvDecPrm->i4MemBase) >> 9); // div 256
            vVDecWriteMC(u4VDecID, RW_MC_BC1,  (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic2CSa) >> prWmvDecPrm->i4MemBase) >> 8); // div 128
            #endif
      
            if ((prWmvDecPrm->prICOMPS->ucFrameTypeLast == SKIPFRAME) && (prWmvDecPrm->prPPS->ucPicType == BVOP))
            {
                if (my_forward == 1)
                {
                    #if VMMU_SUPPORT
                    {
                    UINT32 u4Page_addr = 0;
                    
                    u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((prWmvDecPrm->rWmvWorkBufSa.u4Pic0YSa)/(4*1024))*4);
                    printk("[WMV] i4VDEC_HAL_WMV_DecStart, 19, Page Addr = 0x%x\n", u4Page_addr);    
                    vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic0YSa), PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic0YSa) + PIC_Y_SZ);
                    vVDecWriteMC(u4VDecID, RW_MC_R1Y, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic0YSa) >> prWmvDecPrm->i4MemBase >> 9); 
                    vVDecWriteMC(u4VDecID, RW_MC_R2Y, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic0YSa) >> prWmvDecPrm->i4MemBase >> 9); 

                    u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((prWmvDecPrm->rWmvWorkBufSa.u4Pic0CSa)/(4*1024))*4);
                    printk("[WMV] i4VDEC_HAL_WMV_DecStart, 20, Page Addr = 0x%x\n", u4Page_addr);    
                    vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic0CSa), PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic0CSa) + PIC_C_SZ);
                    vVDecWriteMC(u4VDecID, RW_MC_R1C, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic0CSa) >> prWmvDecPrm->i4MemBase >> 8); 
                    vVDecWriteMC(u4VDecID, RW_MC_R2C, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic0CSa) >> prWmvDecPrm->i4MemBase >> 8); 
                    }
                    #else 
                    vVDecWriteMC(u4VDecID, RW_MC_R1Y, (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic0YSa) >> prWmvDecPrm->i4MemBase) >> 9); // div 512
                    vVDecWriteMC(u4VDecID, RW_MC_R1C, (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic0CSa) >> prWmvDecPrm->i4MemBase) >> 8); // div 256
                    vVDecWriteMC(u4VDecID, RW_MC_R2Y, (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic0YSa) >> prWmvDecPrm->i4MemBase) >> 9); // div 512
                    vVDecWriteMC(u4VDecID, RW_MC_R2C, (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic0CSa) >> prWmvDecPrm->i4MemBase) >> 8); // div 256
                    #endif
                }
                else
                {
                    #if VMMU_SUPPORT
                    {
                    UINT32 u4Page_addr = 0;
                    
                    u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((prWmvDecPrm->rWmvWorkBufSa.u4Pic1YSa)/(4*1024))*4);
                    printk("[WMV] i4VDEC_HAL_WMV_DecStart, 21, Page Addr = 0x%x\n", u4Page_addr);    
                    vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic1YSa), PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic1YSa) + PIC_Y_SZ);
                    vVDecWriteMC(u4VDecID, RW_MC_R1Y, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic1YSa) >> prWmvDecPrm->i4MemBase >> 9); 
                    vVDecWriteMC(u4VDecID, RW_MC_R2Y, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic1YSa) >> prWmvDecPrm->i4MemBase >> 9); 

                    u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((prWmvDecPrm->rWmvWorkBufSa.u4Pic1CSa)/(4*1024))*4);
                    printk("[WMV] i4VDEC_HAL_WMV_DecStart, 22, Page Addr = 0x%x\n", u4Page_addr);    
                    vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic1CSa), PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic1CSa) + PIC_C_SZ);
                    vVDecWriteMC(u4VDecID, RW_MC_R1C, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic1CSa) >> prWmvDecPrm->i4MemBase >> 8); 
                    vVDecWriteMC(u4VDecID, RW_MC_R2C, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic1CSa) >> prWmvDecPrm->i4MemBase >> 8); 
                    }
                    #else 
                    vVDecWriteMC(u4VDecID, RW_MC_R1Y, (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic1YSa) >> prWmvDecPrm->i4MemBase) >> 9); // div 512
                    vVDecWriteMC(u4VDecID, RW_MC_R1C, (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic1CSa) >> prWmvDecPrm->i4MemBase) >> 8); // div 256
                    vVDecWriteMC(u4VDecID, RW_MC_R2Y, (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic1YSa) >> prWmvDecPrm->i4MemBase) >> 9); // div 512
                    vVDecWriteMC(u4VDecID, RW_MC_R2C, (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic1CSa) >> prWmvDecPrm->i4MemBase) >> 8); // div 256
                    #endif
                }
            }
            
      
            a200 = 0x00200032 +
               (pred_use_wdle << 6)+
               (pred_use_wdle << 7)+
               (pred_use_wdle << 20) +
               (0<<11)+
               (((prWmvDecPrm->prPPS->i4Panning & 0xf) != 0) << 9) +
               ((prWmvDecPrm->i4CodecVersion == VDEC_WMV1 || prWmvDecPrm->i4CodecVersion == VDEC_WMV2)<<15)+
               (prWmvDecPrm->prICOMPS->i4ResetMvDram << 18) +
               (mv_sram_use << 31);// +
               //((prWmvDecPrm->prPPS->i4X9MVMode == ALL_1MV_HALFPEL_BILINEAR) << 25) +
               //((prWmvDecPrm->fgUVHpelBilinear == 1) << 26)+
               //(prWmvDecPrm->i4CurrentTemporalField << 27) +
               //(prWmvDecPrm->fgVC1 << 28) +
               //(pWMVDec->m_bUseOppFieldForRef << 29);
        }   

        #if VMMU_SUPPORT
        printk("[WMV] i4VDEC_HAL_WMV_DecStart, VMMUTable:[0x%x, 0x%x]\n", ((UINT32)_pucVMMUTable[u4VDecID]), PHYSICAL((UINT32)_pucVMMUTable[u4VDecID])); 
        
        vVDecVMMUEnable(PHYSICAL((UINT32)_pucVMMUTable[u4VDecID]));
        #endif
        
        vVDecWriteVLD(u4VDecID, RW_VLD_DCMVSEL, a200);
    }

    u4Mbstart_Dcac_Switch = u4VDecReadVLD(u4VDecID, RW_MBSTART_DCAC_SWITCH);
    u4Mbstart_Dcac_Switch |= (1<<30);
    vVDecWriteVLD(u4VDecID, RW_MBSTART_DCAC_SWITCH, u4Mbstart_Dcac_Switch);
      
    //-----------------------------------------------------
    // VLD_reg_206 : MV Range
    //----------------------------------------------------- 
    if ((prWmvDecPrm->i4CodecVersion == VDEC_WMV1) || (prWmvDecPrm->i4CodecVersion == VDEC_WMV2))
    {
        a206 = (prWmvDecPrm->i4CodecVersion <= VDEC_WMV2) + //wmv7/8
               ((prWmvDecPrm->i4CodecVersion <= VDEC_WMV2) << 1)+ //wmv7/8
               ((prWmvDecPrm->i4CodecVersion == VDEC_WMV3) << 2)+ //wmv9
               (0 << 3)+ //VDEC_VC1
               ((prWmvDecPrm->prEPS->i4MVRangeIndex&0xf)<<4)+//mv_range
               (0 << 8)+//interlace_420
               (0 << 9) + //interlace_411
               (((prWmvDecPrm->prPPS->i4X9MVMode == ALL_1MV_HALFPEL_BILINEAR) || (prWmvDecPrm->prPPS->i4X9MVMode == ALL_1MV_HALFPEL)) << 10) +//VWMVHalfFilterSel
               ((prWmvDecPrm->prEPS->fgUVHpelBilinear == 1) << 11)+ //VWMVrounding
               (0 << 12) + //VWMVHalfPelBackRef
               (0 << 13) + //VWMVPostCrescent
               (0 << 14) + //VWMVUseOppieldRef
               (0 << 15) + //VWMVinter420Field2ref
               (0 << 16)+ //VWMVinter420FieldCurr
               (0 << 17)+ //VWMVinter420FieldCurrTmp
               (0 << 18)+ //vwmv_420field_postrc1
               (prWmvDecPrm->prSPS->fgFrmHybridMVOn << 19) + //vwmv_8hybrid_mv_on
               (prWmvDecPrm->prPPS->fgMvResolution << 20) +  //vwmv_8mvresolution
               (use_interpredictor << 21)+
               (prWmvDecPrm->prPPS->fgInterlaceV2 << 23);
    }
    else // >= VDEC_WMV3
    {
        a206 = 0 + //wmv7
              (0 << 1)+ //wmv8
              ((prWmvDecPrm->i4CodecVersion == VDEC_WMV3)<<2)+ //wmv9
              ((prWmvDecPrm->i4CodecVersion == VDEC_VC1)<<3)+ //VC1
              ((prWmvDecPrm->prEPS->i4MVRangeIndex&0xf)<<4)+//mv_range
              (prWmvDecPrm->prPPS->fgInterlaceV2 << 8)+//interlace_420
              (0 << 9) + //interlace_411
              (((prWmvDecPrm->prPPS->i4X9MVMode == ALL_1MV_HALFPEL) || (prWmvDecPrm->prPPS->i4X9MVMode == ALL_1MV_HALFPEL_BILINEAR)) << 10) +//VWMVHalfFilterSel
              ((prWmvDecPrm->prEPS->fgUVHpelBilinear == 1) << 11)+ //VWMVrounding
    //      (prWmvDecPrm->i4CurrentTemporalField << 12) + //VWMVHalfPelBackRef
              (prWmvDecPrm->prPPS->fgBackRefUsedHalfPel << 12) + //VWMVHalfPelBackRef
              (prWmvDecPrm->prSPS->fgVC1 << 13) + //VWMVPostCrescent
              (prWmvDecPrm->prPPS->fgUseOppFieldForRef << 14) + //VWMVUseOppieldRef
              ((prWmvDecPrm->prPPS->fgTwoRefPictures && (prWmvDecPrm->prPPS->ucFrameCodingMode == INTERLACEFIELD)) << 15) + //VWMVinter420Field2ref
              (((prWmvDecPrm->prPPS->ucFrameCodingMode == INTERLACEFIELD)&&(prWmvDecPrm->prPPS->i4CurrentField == 1)) << 16)+ //VWMVinter420FieldCurr
              ((prWmvDecPrm->prPPS->i4CurrentTemporalField) << 17)+ //VWMVinter420FieldCurrTmp
              ((prWmvDecPrm->prPPS->fgPostRC1) <<18)+
              ((prWmvDecPrm->prPPS->fgInterlaceV2) << 23)+
              ((prWmvDecPrm->i4CodecVersion == VDEC_VC1 && prWmvDecPrm->prPPS->ucPicType == BVOP ) << 24) +
              (((prWmvDecPrm->prPPS->ucFrameCodingMode == INTERLACEFIELD) || (prWmvDecPrm->prPPS->ucFrameCodingMode == INTERLACEFRAME)) <<25) ;
    //          ((prWmvDecPrm->ucFrameCodingMode == PROGRESSIVE && (prWmvDecPrm->ucPicType == BVOP)) <<26);
    }
    vVDecWriteVLD(u4VDecID, RW_VLD_MVVOP_SEL, a206);
  
  
    //-----------------------------------------------------
    // VLD_reg_194 : set top_mv1 or bot_mv1
    //-----------------------------------------------------
    {
        INT32 a194,a194_1;
        if ((prWmvDecPrm->prPPS->ucFrameCodingMode == INTERLACEFIELD) && (prWmvDecPrm->prPPS->i4CurrentField == 1) && (prWmvDecPrm->i4CodecVersion >= VDEC_WMV3)) //bot
        {
            a194 = ((u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Mv12Sa) >> 2) & 0x3FFFFF) + //dram_addr_base
                   ((prWmvDecPrm->prSPS->u4NumMBX * 4) << 22); //Mv_1_2
            a194_1 = (((u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Mv12Sa) >> 2) >> 22) & 0x3F);
        }
        else
        {
            a194 = ((u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Mv1Sa) >> 2) & 0x3FFFFF) + //dram_addr_base
                   ((prWmvDecPrm->prSPS->u4NumMBX * 4) << 22); //Mv_1
            a194_1 = (((u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Mv1Sa) >> 2) >> 22 ) & 0x3F);
        }
        vVDecWriteVLD(u4VDecID, RW_VLD_MVSA, a194); //Robert: Temporary Marked! Interlace Field Needs it!!!!
        vVDecWriteVLD(u4VDecID, RW_VLD_ADDREXTEND, (u4VDecReadVLD(u4VDecID, RW_VLD_ADDREXTEND) & 0xFFFFFFC0) |
        a194_1); //Robert: Temporary Marked! Interlace Field Needs it!!!!
        
    }
  
  
    //-----------------------------------------------------
    // VLD_reg_195 : TRB & TRD
    //-----------------------------------------------------
    TRB=prWmvDecPrm->prPPS->i4BNumerator * prWmvDecPrm->prPPS->i4BFrameReciprocal;
    TRD=prWmvDecPrm->prPPS->i4BNumerator * prWmvDecPrm->prPPS->i4BFrameReciprocal - 256;
    if (prWmvDecPrm->i4CodecVersion >= VDEC_WMV3)
    {
        a195 = (TRB << 0) + (TRD << 16);
        vVDecWriteVLD(u4VDecID, RW_VLD_DIRE_MD, a195);
    }
    else
    {
        if(prWmvDecPrm->i4CodecVersion == VDEC_WMV2)
            wmv78_slice_num = prWmvDecPrm->prSPS->u4NumMBY / prWmvDecPrm->prSPS->i4SliceCode;
        else
            wmv78_slice_num = (prWmvDecPrm->prSPS->u4NumMBY / (prWmvDecPrm->prSPS->i4SliceCode - 22));
            
        if (prWmvDecPrm->prSPS->u4NumMBY < 33)
        {
  		a195 = 0;
  		for (i=1;i<prWmvDecPrm->prSPS->u4NumMBY;i++)
  			a195 += ((i%wmv78_slice_num) == 0) << (i-1);
  		a199 = 0;
        }
        else
  	  {
  		a195 = 0; a199 = 0;
  		for (i=1;i<33;i++)
  		    a195 += ((i%wmv78_slice_num) == 0) << (i-1);
  
  		for (i=33;i<prWmvDecPrm->prSPS->u4NumMBY;i++)
  		    a199 += ((i%wmv78_slice_num) == 0) << (i-33);
        }
  
          vVDecWriteVLD(u4VDecID, RW_VLD_DIRE_MD, a195);
          vVDecWriteVLD(u4VDecID, RW_VLD_DIRE_MD_IL, a199);
    }
  
  
  
    //-----------------------------------------------------
    // VLD_reg_207 : interlace 420
    //-----------------------------------------------------
  if (prWmvDecPrm->i4CodecVersion >= VDEC_WMV3)
  {
      INT32 use_half = use_quarter_pel_mv ? 1 : 2;
      INT32 a207;
      a207 = (use_half*prWmvDecPrm->prPPS->i4MaxZone1ScaledFarMVX & 0x7f) +
           (((use_half*prWmvDecPrm->prPPS->i4MaxZone1ScaledFarMVY) & 0x1f) << 8) +
           (((use_half*prWmvDecPrm->prPPS->i4Zone1OffsetScaledFarMVX) & 0x7f) << 16) +
           (((use_half*prWmvDecPrm->prPPS->i4Zone1OffsetScaledFarMVY) & 0x1f) << 24);
      vVDecWriteVLD(u4VDecID, RW_VLD_MVF1, a207);
  }
  
  
    //-----------------------------------------------------
    // VLD_reg_208 : interlace 420
    //-----------------------------------------------------
  if (prWmvDecPrm->i4CodecVersion >= VDEC_WMV3)
  {
      INT32 use_half = 1;//use_quarter_pel_mv ? 1 : 2;
      INT32 a208;
      a208 = ((use_half*prWmvDecPrm->prPPS->i4FarFieldScale1) & 0x3fff) +
           (((use_half*prWmvDecPrm->prPPS->i4FarFieldScale2) & 0x1ff) << 14) +
           (((use_half*prWmvDecPrm->prPPS->i4NearFieldScale) & 0x1ff) << 23);
      vVDecWriteVLD(u4VDecID, RW_VLD_MVF2, a208);
  }
  
    //-----------------------------------------------------
    // VLD_reg_209 : interlace 420
    //-----------------------------------------------------
  if (prWmvDecPrm->i4CodecVersion >= VDEC_WMV3)
  {
      INT32 use_half = use_quarter_pel_mv ? 1 : 2;
      INT32 a209;
      a209 = ((use_half*prWmvDecPrm->prPPS->i4MaxZone1ScaledFarBackMVX) & 0x7f) +
           (((use_half*prWmvDecPrm->prPPS->i4MaxZone1ScaledFarBackMVY) & 0x1f) << 8) +
           (((use_half*prWmvDecPrm->prPPS->i4Zone1OffsetScaledFarBackMVX) & 0x7f) << 16) +
           (((use_half*prWmvDecPrm->prPPS->i4Zone1OffsetScaledFarBackMVY) & 0x1f) << 24);
      vVDecWriteVLD(u4VDecID, RW_VLD_MVF3, a209);
  }
  
    //-----------------------------------------------------
    // VLD_reg_210 : interlace 420
    //-----------------------------------------------------
  if (prWmvDecPrm->i4CodecVersion >= VDEC_WMV3)
  {
      INT32 use_half = 1;//use_quarter_pel_mv ? 1 : 2;
      INT32 a210;
      a210 = ((use_half*prWmvDecPrm->prPPS->i4FarFieldScaleBack1) & 0x3fff) +
           (((use_half*prWmvDecPrm->prPPS->i4FarFieldScaleBack2) & 0x1ff) << 14) +
           (((use_half*prWmvDecPrm->prPPS->i4NearFieldScaleBack) & 0x1ff) << 23);
      vVDecWriteVLD(u4VDecID, RW_VLD_MVF4, a210);
  }
  
    //-----------------------------------------------------
    // VLD_reg_212 : interlace 420
    //-----------------------------------------------------
  {
      INT32 a212;
      a212 = (vop_type == IVOP) + //VopCodingTypeI
            ((vop_type == PVOP) << 1)+ //VopCodingTypeP
            ((vop_type == BVOP) << 2)+ //VopCodingTypeB
            (0 << 3)+ //VopCodingTypeS (mp4)
            (0 << 4)+ //MPEG1or2
            (0 << 5)+ // Divx311
            (0 << 6)+ //MP4
            (0 << 7)+ //ShortVideo
            (1 << 8)+ //WMV789A
            ((prWmvDecPrm->prPPS->ucFrameCodingMode != INTERLACEFIELD) << 9); //WMVFramePic
      vVDecWriteVLD(u4VDecID, RW_VLD_VOP_TYPE, a212);
  }
    //----------------------------------------------
    // VLD_reg_116 : load sum_out_wmv(or mp4) fpga
    //----------------------------------------------
  // No need for emuSoft
  //        vVDecWriteVLD(u4VDecID, RW_VLD_116, 0x00000001);
  //        vVDecWriteVLD(u4VDecID, RW_VLD_116, 0x00000000);
  
    //------------------------------
    //   VLD RISC WRITE PATTERN
    //------------------------------
    //initialize before load r_ptr and shift_bit
  // HMLin suggests to mark.
  //        vVDecWriteVLD(u4VDecID, RW_VLD_WMV_MODE, 0x00000000);
  
    //-------------------------------------------
    // VLD_reg_44 : r_ptr (only for simulation)
    //-------------------------------------------
  
    //-------------------------------------------
    // VLD_reg_45 : v_start address in dram
    //-------------------------------------------
  
    //-------------------------------------------
    // VLD_reg_46 : v_end address in dram
    //-------------------------------------------
  
    //-------------------------------------------
    // VLD_reg_35 : load bitstream from dram
    //-------------------------------------------
  
    //-------------------------------------------
    // read VLD_reg to shift the barrel shifter
    //-------------------------------------------
  
    //-------------------------------------------
    // VLD_reg_34~39,42 : VLD parameter setting
    //-------------------------------------------
    if (prWmvDecPrm->prPPS->ucFrameCodingMode == INTERLACEFIELD)
    {
      if (prWmvDecPrm->prPPS->i4CurrentField == 0) tmp = 1; //top
      else tmp = 2; //bot
    } else tmp = 3; //frame
  
  
    a34 = (tmp <<30) + //picture_structure << 30) +
            (0<<29) + //frame_pred_frame_dct << 29) +
          (0<<28) + //concealment_motion_vectors << 28) +
          (0<<27) + //q_scale_type << 27) +
            // (top_field_first << 26) +
            // (full_pel_forward_vector << 25) +
            // (full_pel_backward_vector << 24) +
            // ((mp4_state->hdr.prediction_type) << 21) + //picture_coding_type << 21) +
            ((vop_type&0xF) << 21) + //picture_coding_type << 21) +
            (0<<20)+ //intra_vlc_format << 20) +
        // (intra_dc_precision << 18) +
        // (alternate_scan << 17) +
        // (reset_of_mat << 16) +
        // ((mp4_state->hdr.fcode_for) << 12)+ //f_code[0][0] << 12) +
        // ((pWMVDec->uiFCode) << 12)+ //f_code[0][0] << 12) +
        // it seems that the f_code not exists in WMV
        // ((mp4_state->hdr.fcode_back) << 8); //(f_code[0][1] << 8) +
        // ((mp4_state->hdr.fcode_back)<<4); //f_code[1][0] << 4) +
        // it seems that the f_code not exists in WMV
        (0); //f_code[1][1]);
    vVDecWriteVLD(u4VDecID, RW_VLD_PARA, a34);
  
  //  height = (prWmvDecPrm->ucFrameCodingMode == INTERLACEFIELD)? prWmvDecPrm->u4PicHeightSrc/2 : prWmvDecPrm->u4PicHeightSrc;
    height = (prWmvDecPrm->prPPS->ucFrameCodingMode == INTERLACEFIELD)? prWmvDecPrm->prSPS->u4PicHeightSrc/2 : prWmvDecPrm->prSPS->u4PicHeightDec/*Main Profile only*/;
    #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    a26 = (((prWmvDecPrm->prSPS->u4NumMBY) - 1) << 16) + ((prWmvDecPrm->prSPS->u4NumMBX) - 1);
    a28 = (((prWmvDecPrm->prSPS->u4NumMBY) *16) << 16) + ((prWmvDecPrm->prSPS->u4NumMBX) * 16);
    vVDecWriteVLDTOP(u4VDecID, RW_TOPVLD_WMV_PICSIZE_MB, a26); 
    vVDecWriteVLDTOP(u4VDecID, RW_TOPVLD_WMV_PICSIZE, a28); 
    #if MC_LOWCOST_PERORMANCE
    printk("pic_width = %d  pic_height = %d\n", ((prWmvDecPrm->prSPS->u4NumMBX) * 16),((prWmvDecPrm->prSPS->u4NumMBY) *16)); //sun for temp
    #endif
    #else
    a36 = (0<<31)+ //MPEG1_flag
            ((height + 15 ) << 16)+ //vertical_size
            (63 << 8)+ //max_mbl_mod/** b_buffer_size_s1 **/ << 8)+
  //          ((prWmvDecPrm->u4PicWidthSrc + 15)>>4); //picture_width_in_MB);
            (prWmvDecPrm->prSPS->u4NumMBX);
    vVDecWriteVLD(u4VDecID, RW_VLD_PICSZ, a36);
    #endif
  
    if ((prWmvDecPrm->i4CodecVersion == VDEC_WMV1) || (prWmvDecPrm->i4CodecVersion == VDEC_WMV2))
    {
        a37 = (0<<24)+ //part_dec_sta_y /* mc_start_mb_row*/ << 24) +
              (63<<16)+ //part_dec_end_y /* mc_end_mb_row*/  << 16) +
              (0<<8); //b_buf_start  /*b_buffer_start_row*/ << 8);
    }
    else
    {
        a37 = (0<<24)+ //part_dec_sta_y /* mc_start_mb_row*/ << 24) +
              (127<<16)+ //part_dec_end_y /* mc_end_mb_row*/  << 16) +
              (0<<8); //b_buf_start  /*b_buffer_start_row*/ << 8);
    }
    vVDecWriteVLD(u4VDecID, RW_VLD_MBROWPRM, a37);
  
    a38 = (0 /* digest_mb_x */ << 16) +
            (0 /* digest_mb_y */ );
    vVDecWriteVLD(u4VDecID, RW_VLD_DIGMBSA, a38);
  
    a39 = (0<<24) + //H_scale /*h_scale*/ << 24) +
            (0<<16) + //V_scale /*v_scale*/  << 16) +
          (0<<8);   //write_to_digest_buf /*w_to_dig_buf*/ << 8);
    vVDecWriteVLD(u4VDecID, RW_VLD_SCALE, a39);
  
    a42 = (0 /* digest_MB_y_offset1 */ << 16) +
            (0 /* digest_MB_y_offset1 */ << 8) +
            (0 /* digest_MB_y_offset1 */ << 0);
    vVDecWriteVLD(u4VDecID, RW_VLD_DIGMBYOFF, a42);
  
    a35 = (1 << 27) + //dec_b_pic_all
        (0 << 8) + //mc_start_mb_addr
        (prWmvDecPrm->prSPS->u4NumMBX); //mc_end_mb_addr
    vVDecWriteVLD(u4VDecID, RW_VLD_PROC, a35);
  
    //-------------------------------------------
    // VLD_reg_135 : load_sum_risc (for fpga)
    //-------------------------------------------
    vVDecWriteVLD(u4VDecID, RW_VLD_WMV_LOAD_SUM, 0x00000001);
    mb();
    vVDecWriteVLD(u4VDecID, RW_VLD_WMV_LOAD_SUM, 0x00000000);
  
    //-------------------------------------------
    // VLD_reg_131 : wmv version setting
    //-------------------------------------------
    if ((prWmvDecPrm->i4CodecVersion == VDEC_WMV1) || (prWmvDecPrm->i4CodecVersion == VDEC_WMV2))
    {
        a131 = (prWmvDecPrm->i4CodecVersion == VDEC_WMV3) + //wmv9
               ((prWmvDecPrm->i4CodecVersion == VDEC_WMV2)<<1)+ //wmv8
               ((prWmvDecPrm->i4CodecVersion == VDEC_WMV1)<<2)+ //wmv7
               (1 << 8) + // profile : [main]
               (0 << 9) + // profile : [advance]
               (prWmvDecPrm->prSPS->fgYUV411 << 16) + //interlace_411 flag
               (0 << 17)+ //interlace_420 flag
               (1 << 24) + // frame_picture
               (0 << 25); //bottom_field
    }
    else
    {
        a131 = (((prWmvDecPrm->prSPS->i4WMV3Profile & 0x2) == 0) << 0) +  // wmv9_flag (this code only can decode the WMV9 bitstream)
               ((prWmvDecPrm->prSPS->i4WMV3Profile & 0x2) << 8) + // profile : [advanced //main]
               (prWmvDecPrm->prSPS->fgYUV411 << 16) + //interlace_411 flag
               (prWmvDecPrm->prPPS->fgInterlaceV2 << 17)+ //interlace_420 flag
               ((prWmvDecPrm->prPPS->ucFrameCodingMode != INTERLACEFIELD) << 24) + // frame_picture
               ((tmp == 2) << 25) + //top field:0  bottom_field:1
               (((prWmvDecPrm->prPPS->i4CurrentTemporalField == 1)&&(prWmvDecPrm->prPPS->ucFrameCodingMode == INTERLACEFIELD)) << 26); //1st field:0, 2nd field:1
    }
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8555)
	 if (BSP_GetIcVersion() == IC_8555)
        {
    if(u4VDecID == 1)//add for lite random error in mt8555 verification @>Youlin Pei
    {
        vWriteReg(0x32000,(u4ReadReg(0x32000) | 0x1));
    }
    else
    {
        vWriteReg(0x2B000,(u4ReadReg(0x2B000) | 0x1));
    }
        }
#endif

    vVDecWriteVLD(u4VDecID, RW_VLD_WMV_MODE, a131);

#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8555)
	if (BSP_GetIcVersion() == IC_8555)
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
    //-------------------------------------------------
    // VLD_reg_132 : Quantizer type , half_step, slice
    //-------------------------------------------------
    a132 =  ((prWmvDecPrm->prPPS->fgUse3QPDZQuantizer) << 0) +
        ((prWmvDecPrm->prPPS->fgHalfStep) << 8) //+
        //((_fgSliceVC1) << 16)
        //((prWmvDecPrm->prPPS->i4FrameXformMode) << 16) //+
        //// { 0: XFORMMODE_8x8,
        ////   1: XFORMMODE_8x4,
        ////   2: XFORMMODE_4x8,
        ////   3: XFORMMODE_MBSWITCH};
        //((pmbmd->m_chMBMode) << 20)
        + (prWmvDecPrm->prPPS->u4BPRawFlag << 16)
        ;
    vVDecWriteVLD(u4VDecID, RW_VLD_WMV_PARA1, a132);
  
    //-------------------------------------------------
    // VLD_reg_133 : TTMB, QP_value, ...
    //-------------------------------------------------
    if ((prWmvDecPrm->i4CodecVersion == VDEC_WMV1) || (prWmvDecPrm->i4CodecVersion == VDEC_WMV2))
    {
        high_rate_switch = (prWmvDecPrm->prPPS->i4PicQtIdx <= MAXHIGHRATEQP) && (prWmvDecPrm->i4CodecVersion == VDEC_WMV3);
        ttfrm = (prWmvDecPrm->prPPS->fgMBXformSwitching == 1)? 0 :
                  (prWmvDecPrm->prPPS->i4FrameXformMode == 4) ? 3 : (prWmvDecPrm->prPPS->i4FrameXformMode & 0x3);
    
        a133 = (prWmvDecPrm->prPPS->i4CBPTable << 0) +
               ((prWmvDecPrm->prPPS->fgDCPredIMBInPFrame) << 3) +
               ((prWmvDecPrm->prPPS->fgMBXformSwitching == 1) << 4) + //TTMBF
               ((ttfrm) << 5) +
               ((prWmvDecPrm->prPPS->fgMvResolution) << 7 ) +
               ((prWmvDecPrm->prPPS->fgIntraDCTDCTable) << 8) +
               ((prWmvDecPrm->prPPS->fgDCTTableMB) << 9) +
               ((high_rate_switch) << 10) +
               ((prWmvDecPrm->prSPS->fgRotatedIdct == 0) << 11) +
               ((prWmvDecPrm->prPPS->u4DCTACIntraTableIndx) << 12) +
               ((prWmvDecPrm->prPPS->u4DCTACInterTableIndx) << 14) + //ming added
               ((prWmvDecPrm->prPPS->i4DCStepSize)  << 16) +
               ((prWmvDecPrm->prPPS->i4StepSize)  << 24);
    }
    else
    {
        high_rate_switch = prWmvDecPrm->prPPS->i4PicQtIdx <= MAXHIGHRATEQP;// && prWmvDecPrm->i4CodecVersion == VDEC_WMV3;
        ttfrm = (prWmvDecPrm->prPPS->fgMBXformSwitching == 1)? 0 :
                     (prWmvDecPrm->prPPS->i4FrameXformMode == 4)? 3 : (prWmvDecPrm->prPPS->i4FrameXformMode & 0x3);
    
        a133 = (prWmvDecPrm->prPPS->i4CBPTable << 0) +
               ((prWmvDecPrm->prPPS->fgMBXformSwitching == 0) << 4) + //TTMBF
               ((ttfrm) << 5) +
               ((prWmvDecPrm->prPPS->fgIntraDCTDCTable) << 8) +
               ((prWmvDecPrm->prPPS->fgDCTTableMB) << 9) +
               ((high_rate_switch) << 10) +
               ((prWmvDecPrm->prSPS->fgRotatedIdct == 0) << 11) +
               ((prWmvDecPrm->prPPS->u4DCTACIntraTableIndx) << 12) +
               ((prWmvDecPrm->prPPS->u4DCTACInterTableIndx) << 14) + //ming added
               ((prWmvDecPrm->prPPS->i4DCStepSize)  << 16) +
               ((prWmvDecPrm->prPPS->i4StepSize)  << 24);
    }
    vVDecWriteVLD(u4VDecID, RW_VLD_WMV_PARA2, a133);
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
  a134 = u4VDecReadVLD(u4VDecID, RW_VLD_WMV_OVC);

  #if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8550)
  //bit18 is fixed in 8550
    a134 |= (1<<18);
  #endif

  a134 |= (1<<13);
  vVDecWriteVLD(u4VDecID, RW_VLD_WMV_OVC, a134 );

 #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8550)
     //This is new error concealment mode for 8550     
     a154 = u4VDecReadVLD(u4VDecID, (154<<2));
     a154 |= (1<<24);
     vVDecWriteVLD(u4VDecID, (154<<2), a154);
 #endif
    
#endif
  
    //-------------------------------------------------
    // VLD_reg_136 : MV parameter setting for VLD
    //-------------------------------------------------
    b_fraction_chk =  ((prWmvDecPrm->prPPS->i4BNumerator * prWmvDecPrm->prPPS->i4BFrameReciprocal) >> 7)? 1:0;
  
    if ((prWmvDecPrm->i4CodecVersion == VDEC_WMV1) || (prWmvDecPrm->i4CodecVersion == VDEC_WMV2))
    {
        a136 = ((prWmvDecPrm->prEPS->i4MVRangeIndex) << 0) +
               ((prWmvDecPrm->prPPS->i4X9MVMode)     << 8) +
               ((b_fraction_chk)           << 16) +
               (0 << 17) +
               (0 << 18) +
               (0 << 19) +
               (0 << 24); //((lookup_mv_tbl_number)     << 24);
    }
    else
    {
        a136 = ((prWmvDecPrm->prEPS->i4MVRangeIndex) << 0) +
               ((prWmvDecPrm->prPPS->i4X9MVMode)     << 8) +
               ((b_fraction_chk)           << 16) +
               ((prWmvDecPrm->prPPS->fgTwoRefPictures && (prWmvDecPrm->prPPS->ucFrameCodingMode == INTERLACEFIELD)) << 17) +
               (prWmvDecPrm->prEPS->i4ExtendedDMVX << 18) +
               (prWmvDecPrm->prEPS->i4ExtendedDMVY << 19) +
               ((prWmvDecPrm->prPPS->i4MvTable)     << 24);
    }
    vVDecWriteVLD(u4VDecID, RW_VLD_WMV_PARA3, a136);
  
    if ((prWmvDecPrm->i4CodecVersion == VDEC_WMV1) || (prWmvDecPrm->i4CodecVersion == VDEC_WMV2))
    {
        a137 = (prWmvDecPrm->prSPS->i4SkipBitModeV87 << 8);
        vVDecWriteVLD(u4VDecID, RW_VLD_WMV_BP, a137);
    }
  
    //-------------------------------------------------
    // VLD_reg_148 : VLD mode setting
    //-------------------------------------------------
    a138 = 0 +
          (0 << 25);
  
    vVDecWriteVLD(u4VDecID, RW_VLD_MODE , a138);
    //-------------------------------------------------
    // VLD_reg_140 : VOPQUANT setting (MBQUANT)
    //-------------------------------------------------
    a140 = ((prWmvDecPrm->prPPS->i4Panning & 0xf)                 << 0)+
         (prWmvDecPrm->prPPS->fgDQuantOn           << 8)+
         ((prWmvDecPrm->prPPS->ucDiffQtProfile == 3)                << 9)+
         ((prWmvDecPrm->prPPS->fgDQuantBiLevel & 0x1)           <<10)+
         ((prWmvDecPrm->prPPS->ucDQuantBiLevelStepSize & 0x1f)  <<16);
    vVDecWriteVLD(u4VDecID, RW_VLD_WMV_QM, a140);
  
  
    //-------------------------------------------------
    // VLD_reg_142 : MV table index for VLD
    //-------------------------------------------------
    a142 = ((prWmvDecPrm->prPPS->i4X9MVMode == MIXED_MV)&&((prWmvDecPrm->prPPS->ucPicType == PVOP)||(prWmvDecPrm->prPPS->ucPicType == BVOP))) +
           (((prWmvDecPrm->i4CodecVersion <= VDEC_WMV2) && (prWmvDecPrm->prPPS->i4MvTable)) << 1) +
           (((prWmvDecPrm->prPPS->i4Overlap & 4) != 0) << 8)+
           (prWmvDecPrm->prPPS->i4MBModeTable << 16)+
           (prWmvDecPrm->prPPS->i42MVBPTable << 24)+
           (prWmvDecPrm->prPPS->i44MVBPTable << 28);
    vVDecWriteVLD(u4VDecID, RW_VLD_WMV_TAB, a142);
  
    //-------------------------------------------------
    // VLD_reg_146 : parse slice picture header (unit:bit)
    //-------------------------------------------------
    if (prWmvDecPrm->i4CodecVersion == VDEC_VC1)
    {
        vVDecWriteVLD(u4VDecID, RW_VLD_WMV_PBN, prWmvDecPrm->prPPS->i4SlicePicHeaderNum);
    }
    //vVDecWriteVLD(u4VDecID, RW_VLD_SRAM_TEST_WRITE_DATA, 0x101); //ming add for debug 12/15
  
    // write raw mode flag by firmware 
#ifdef WMV_SW_DEC_BP
    vVDecWriteVLD(u4VDecID, RW_VLD_MODE, 0x04080000); //bit 26. Latch BP RAW Flag.
    vVDecWriteVLD(u4VDecID, RW_VLD_MODE, 0x00080000); //bit 26. Latch BP RAW Flag.
#endif
    // HW issue : turn on DCAC time out modify
    vVDecWriteVLD(u4VDecID, RW_VLD_DCACWK, 0x10000);
  
    // turn on error detect
    vVDecWriteVLD(u4VDecID, RW_VLD_VDOUFM, VLD_VDOUFM + VLD_ENSTCNT + VLD_AATO+ VLD_ERR_MONITOR);

#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8520)
    if ((prDecPrm->rDownScalerPrm.fgDSCLEn) || (u4VDecReadMC(u4VDecID, RW_MC_WRAPPER_SWITCH) == 0))
    {
        // turn off error concealment because VDSCL and PP Wrapper  can not work with it
        vVDecWriteVLD(u4VDecID, RW_VLD_PROC, u4VDecReadVLD(u4VDecID, RW_VLD_PROC) | VLD_PERRCON);
    }
#endif
	
#if CONFIG_DRV_VERIFY_SUPPORT
  if (_u4FileCnt[u4VDecID] == _u4DumpRegPicNum[u4VDecID])
  {
     vVDEC_HAL_WMV_VDec_DumpReg(u4VDecID, TRUE);
  }
#endif
	 
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
   #ifdef WMV_EC_IMPROVE_SUPPORT
    //Set slice addr registers
    if (prWmvDecPrm->i4CodecVersion == VDEC_VC1)
    {
        UINT32 a157 = 0;
        UINT32 a158 = 0;
        UINT32 a159 = 0;
        
      #if (CONFIG_DRV_VERIFY_SUPPORT)
        UCHAR ucCnt = 0;
        UCHAR ucbit = 0;
        
        for (ucCnt = 0; ucCnt <= 31; ucCnt++)
        {
            //Set register #157
            a157 |= (u4SliceAddr[ ucCnt] << ucbit);
            ucbit++;
        }

        ucbit=0;
        for (ucCnt = 32; ucCnt <= 63; ucCnt++)
        {
            //Set register #158
            a158 |= (u4SliceAddr[ ucCnt] << ucbit);
            ucbit++;
        }

        ucbit=0;
        for (ucCnt = 64; ucCnt <= 67; ucCnt++)
        {
            //Set register #159
            a159 |= (u4SliceAddr[ ucCnt] << ucbit);
            ucbit++;
        }

        //Only use WMV slice header error dectection in first three frames.
        //To avoid last frame's header dectection error in EMU/VER stage.
        if (_u4FileCnt[u4VDecID] < 3)
        {
            a134 = u4VDecReadVLD(u4VDecID, RW_VLD_WMV_OVC);
            a134 &= (0xFFFF7FFF);
            vVDecWriteVLD(u4VDecID, RW_VLD_WMV_OVC, a134);
        }
        else
        {
            a134 = u4VDecReadVLD(u4VDecID, RW_VLD_WMV_OVC);
            a134 |= (1<<15);
            vVDecWriteVLD(u4VDecID, RW_VLD_WMV_OVC, a134);
        }           
      #else
        a157 = prWmvDecPrm->u4WMVSliceAddr[0];
        a158 = prWmvDecPrm->u4WMVSliceAddr[1];
        a159 = prWmvDecPrm->u4WMVSliceAddr[2];
      #endif        

        vVDecWriteVLD(u4VDecID, RW_VLD_WMV_SLICE_IDX0, a157);
        vVDecWriteVLD(u4VDecID, RW_VLD_WMV_SLICE_IDX1, a158);
        vVDecWriteVLD(u4VDecID, RW_VLD_WMV_SLICE_IDX2, a159);
    }
    #endif
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

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8550)
    if (prWmvDecPrm->fgWmvMode)
    {
        //WMV: MV part
        if (prWmvDecPrm->prPPS->ucFrameCodingMode != INTERLACEFIELD)
        {
           aAvcMAddr = prWmvDecPrm->rWmvWorkBufSa.u4MvNewSa;
        }
        else
        {
           if (prWmvDecPrm->prPPS->i4CurrentField == 0)
              aAvcMAddr = prWmvDecPrm->rWmvWorkBufSa.u4MvNewSa;
           else
           {
              aAvcMAddr = prWmvDecPrm->rWmvWorkBufSa.u4MvNewSa 
                + (prWmvDecPrm->prSPS->u4NumMBX * prWmvDecPrm->prSPS->u4NumMBY * 16);
           }
        }
		
    		#if (CONFIG_DRV_FPGA_BOARD)
    		aAvcMv131 = PHYSICAL(aAvcMAddr) >> 4;
    		#else
        aAvcMv131 = aAvcMAddr >> 4;   
    		#endif

        vVDecWriteWMVMV(u4VDecID, RW_WMV_MV_NEW_ADDR, aAvcMv131);

        if (prWmvDecPrm->prPPS->fgFieldMode == TRUE && (prWmvDecPrm->i4CodecVersion == VDEC_VC1))
       {
          if(prWmvDecPrm->prPPS->i4CurrentField == 0) //top
          {
            aAvcMv134 = ((prWmvDecPrm->prICOMPS->ucFrameTypeLastTop != PVOP)? 1: 0);
          }
          else
          {
            aAvcMv134 = ((prWmvDecPrm->prICOMPS->ucFrameTypeLastBot != PVOP)? 1: 0);
          }
        }
        else
        {
           aAvcMv134 = ((prWmvDecPrm->prICOMPS->ucFrameTypeLast != PVOP)? 1: 0);
        }
        vVDecWriteWMVMV(u4VDecID, RW_WMV_BWDREF_PIC_TYPE, aAvcMv134);
            
        
        i4Ratio0 = prWmvDecPrm->prPPS->i4BNumerator * prWmvDecPrm->prPPS->i4BFrameReciprocal;
        i4Ratio1 = i4Ratio0 - 256;
        printk("i4Ratio0 = %d, i4Ratio1 = %d\n", i4Ratio0, i4Ratio1);
        i4RegR0 = (i4Ratio0 & 0x0000FFFF);
        i4RegR1 = ((i4Ratio1 << WMV_IRAT_1) & 0xFFFF0000);
        aAvcMv135 = (i4RegR0 | i4RegR1 );
        vVDecWriteWMVMV(u4VDecID, RW_WMV_MV_DIRECT_SCL_FAC, aAvcMv135);
                
        i4Vc1_Mv_Cfg = 
              ( ((prWmvDecPrm->prPPS->i4CurrentTemporalField==0)? 1: 0)   << WMV_FIRST_FIELD)
           + (prWmvDecPrm->prPPS->iBackwardRefDistance                         << WMV_BWDREF_DIST)
           + (prWmvDecPrm->prPPS->iForwardRefDistance                           << WMV_FWDREF_DIST)
           + (prWmvDecPrm->prPPS->fgUseOppFieldForRef                           << WMV_USE_OPP_REF_FIELD)
           + (prWmvDecPrm->prPPS->fgTwoRefPictures                                << WMV_TWO_REF_PIC)
           + ( ((prWmvDecPrm->i4CodecVersion == VDEC_VC1)? 1: 0)         << WMV_POST_RESCENT)
           + (prWmvDecPrm->prEPS->fgUVHpelBilinear                                << WMV_FAST_UV)
           + (prWmvDecPrm->prEPS->i4MVRangeIndex                               << WMV_MV_RNG_IDX)
           + (use_quarter_pel_mv                                                               << WMV_QPIXEL);
           //+ (prWmvDecPrm->prPPS->fgHalfStep                                    << WMV_QPIXEL);
    
        vVDecWriteWMVMV(u4VDecID, RW_WMV_MV_CFG, i4Vc1_Mv_Cfg);           
        

        aAvcMV138 = 0;// = 0x17c01fc;
        amv_max_v1 = ((prWmvDecPrm->prSPS->u4PicWidthDec + 15)>> 4) *2 *8 *4 -4;
        amv_max_h1 = ((prWmvDecPrm->prSPS->u4PicHeightDec + 15)>> 4) *2 *8 *4 -4;
        aAvcMV138 =  ((amv_max_h1 << 16) | amv_max_v1);
    
        aAvcMV139 = 0;// = 0xbc00fc;
        amv_max_v2 = ((prWmvDecPrm->prSPS->u4PicWidthDec + 15)>> 4) *8 *4 -4;
        amv_max_h2 = ((prWmvDecPrm->prSPS->u4PicHeightDec + 15)>> 4)  *8 *4 -4;
        aAvcMV139 =  ((amv_max_h2 << 16) | amv_max_v2);
        vVDecWriteWMVMV(u4VDecID, (138<<2), aAvcMV138);           
        vVDecWriteWMVMV(u4VDecID, (139<<2), aAvcMV139);           

        //UINT32 aAvcMv137 =  i4Vc1_Mv_Cfg;
        //printk("AVCMV_131 = %d\n", aAvcMv131);
        //printk("AVCMV_134 = %d\n", aAvcMv134);
        //printk("AVCMV_135 = %d\n", aAvcMv135);
        //printk("AVCMV_137 = %d\n", aAvcMv137);

        //VLD part        
        a163 = (WMV_NEW_FLG_EN | WMV_PRED_MOD_EN | WMV_PRED_RD_SYNC_DISABLE | WMV_DRAM_BURST_MODE_EN | WMV_BP_USE_PRED_RD_EN);
        vVDecWriteVLD(u4VDecID, RW_VLD_WMV_NEW_CTRL, a163);

        //a164
        a164 = u4AbsDramANc(prWmvDecPrm->rWmvWorkBufSa.u4DcacNewSa);       
        #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
        vVDecWriteVLDTOP(u4VDecID, RW_TOPVLD_WMV_DCAC_NEW_ADDR,  a164);
        #else
        a164 |= (1<<30);
        vVDecWriteVLD(u4VDecID, RW_VLD_WMV_DCAC_NEW_ADDR, a164 );            
        #endif
        
        //a167, a168, a169
        vVDecWriteVLD(u4VDecID, RW_VLD_WMV_BP0_NEW_ADDR, u4AbsDramANc(prWmvDecPrm->rWmvWorkBufSa.u4Bp0NewSa) );
        vVDecWriteVLD(u4VDecID, RW_VLD_WMV_BP1_NEW_ADDR, u4AbsDramANc(prWmvDecPrm->rWmvWorkBufSa.u4Bp1NewSa) );
        vVDecWriteVLD(u4VDecID, RW_VLD_WMV_BP2_NEW_ADDR, u4AbsDramANc(prWmvDecPrm->rWmvWorkBufSa.u4Bp2NewSa) );
        //UINT32 a167 = u4AbsDramANc(prWmvDecPrm->rWmvWorkBufSa.u4Bp0NewSa);
        //UINT32 a168 = u4AbsDramANc(prWmvDecPrm->rWmvWorkBufSa.u4Bp1NewSa);
        //UINT32 a169 = u4AbsDramANc(prWmvDecPrm->rWmvWorkBufSa.u4Bp2NewSa);
    
        //a170        
        a170 = (prWmvDecPrm->prSPS->u4PicWidthDec / 16)  * 7  * (16/16);
        vVDecWriteVLD(u4VDecID, RW_VLD_WMV_OW_PRED, a170);

        //a171           
        i4OwBp = (prWmvDecPrm->prSPS->u4PicHeightDec / 16)  * (16/16);
        a171 = ( (i4OwBp << WMV_NUM_OW_BP_0) | (i4OwBp << WMV_NUM_OW_BP_1) | (i4OwBp << WMV_NUM_OW_BP_2));
        vVDecWriteVLD(u4VDecID, RW_VLD_WMV_OW_BP, a171);

        //printk("VLD_163 = %d\n", a163);
        //printk("VLD_164 = %d\n", a164);
        //printk("VLD_167 = %d\n", a167);
        //printk("VLD_168 = %d\n", a168);
        //printk("VLD_169 = %d\n", a169);
        //printk("VLD_170 = %d\n", a170);
        //printk("VLD_171 = %d\n", a171);

        //WMV: DCAC part
        vVDecWriteWMVDCAC(u4VDecID, RW_WMV_DCAC_RULEB, use_rule_b);
        //UINT32 aWmvDcac0 = use_rule_b;
            
        if (prWmvDecPrm->prPPS->i4DCStepSize > 0)
        {
            i4Predictor = (1024 + (prWmvDecPrm->prPPS->i4DCStepSize >> 1) ) / (prWmvDecPrm->prPPS->i4DCStepSize);
        }   
        
        #if WMV_LOG_TMP
        printk("i4DCStepSize = %d\n",prWmvDecPrm->prPPS->i4DCStepSize);
        #endif
        
        vVDecWriteWMVDCAC(u4VDecID, RW_WMV_DCAC_PRED, i4Predictor); 
        //UINT32 aWmvDcac1 = i4Predictor;
    
        //printk("DCAC_0 = %d\n", aWmvDcac0);
        //printk("DCAC_1 = %d\n", aWmvDcac1);
    }
#endif

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8550)
    #if VDEC_FIELD_COMPACT
        printk( "[WMV] Enable Field Compact Mode\n");
        //vVDecWriteAVSPP(u4VDecID, 0x3C, (u4VDecReadAVSPP(u4VDecID, 0x3C)& 0xEFFFFFFF));
        vWriteReg(AVS_PP_REG_OFFSET0 + 0x3C, (u4ReadReg(AVS_PP_REG_OFFSET0 + 0x3C) & 0xEFFFFFFF));
        vVDecWriteMC(u4VDecID, 0x920, (u4VDecReadMC(u4VDecID, 0x920)  & 0xFEFFFFFF));    
    #else
        //Set NBM address swap mode
        vVDecWriteMC(u4VDecID, RW_MC_NBM_CTRL, 
                     ((u4VDecReadMC(u4VDecID, RW_MC_NBM_CTRL)  & 0xFFFFFFF8) |prDecPrm->ucAddrSwapMode));
         
        #if VDEC_MC_NBM_OFF
        //Turn off NBM address swap mode
        vVDecWriteMC(u4VDecID, RW_MC_NBM_CTRL, 
                     (u4VDecReadMC(u4VDecID, RW_MC_NBM_CTRL) | (RW_MC_NBM_OFF)  ));
        #endif
    #endif
#if VDEC_DDR3_SUPPORT
      u4DDR3_PicWdith =  (((prDecPrm->u4PicBW+ 15)>> 4) + 3) / 4 * 4;
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

#endif
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8555))
{
    UINT32 AVCMV_reg_180;
    UINT32 VLD_reg_134;
    UINT32 VLD_reg_157 = 0;
    UINT32 VLD_reg_158 = 0;
    UINT32 VLD_reg_159 = 0;
    UINT32 DCAC_reg_0;
    UINT32 DCAC_reg_1;
    UINT32 u4NumMBY;
    UINT32 u4NumMBYSlice;
    UINT32 u4index;

    if (prWmvDecPrm->fgWmvMode)// For WMV7&8 new mode.
    {
        if (prWmvDecPrm->i4CodecVersion < VDEC_WMV3)
        {
            AVCMV_reg_180 = u4VDecReadAVCMV(u4VDecID, 180 * 4);
            AVCMV_reg_180 = AVCMV_reg_180 & (0xFFFFFFFE);
            //if(prWmvDecPrm->prPPS->i4MvTable)
            {
                AVCMV_reg_180 |= (prWmvDecPrm->prSPS->fgFrmHybridMVOn) & ((!prWmvDecPrm->prPPS->fgMvResolution) ? 1 : 0);
            }
            vVDecWriteAVCMV(u4VDecID, 180 * 4, AVCMV_reg_180);

            //Printf("New MV before Setting\n");
            //Printf("%d (0x%4X) = 0x%8X\n", 180, (180<<2), u4VDecReadWMVMV(u4VDecID, 180 * 4));
    
            VLD_reg_134 = u4VDecReadVLD(u4VDecID, 134 * 4);
            VLD_reg_134 |= 0x8000;
            vVDecWriteVLD(u4VDecID, 134 * 4, VLD_reg_134);

            u4NumMBY = prWmvDecPrm->prSPS->u4NumMBY;

            VLD_reg_157 = 0x0;
            VLD_reg_158 = 0x0;
            VLD_reg_159 = 0x0;

            //Printf("u4NumMBY = %x,\n",u4NumMBY);
            //Printf("prWmvDecPrm->prSPS->i4SliceCode = %x,\n",prWmvDecPrm->prSPS->i4SliceCode);
            //if(u4NumMBY > prWmvDecPrm->prSPS->i4SliceCode)
            {
                if(prWmvDecPrm->i4CodecVersion == VDEC_WMV2)
                    u4NumMBYSlice = prWmvDecPrm->prSPS->u4NumMBY / prWmvDecPrm->prSPS->i4SliceCode;
                else
                    u4NumMBYSlice = (prWmvDecPrm->prSPS->u4NumMBY / (prWmvDecPrm->prSPS->i4SliceCode - 22));
                if(u4NumMBYSlice != 0)
                {
                    for(u4index = 1; u4index < u4NumMBY; u4index++)
                    {
                        if(u4index < 31)
                            VLD_reg_157 += ((u4index % u4NumMBYSlice) == 0) << (u4index + 1);
                        else if(u4index < 63)
                            VLD_reg_158 += ((u4index % u4NumMBYSlice) == 0) << (u4index - 31);
                        else
                            VLD_reg_159 += ((u4index % u4NumMBYSlice) == 0) << (u4index - 63);
                    }
                }
            }
            vVDecWriteVLD(u4VDecID, 157 * 4, VLD_reg_157);
            vVDecWriteVLD(u4VDecID, 158 * 4, VLD_reg_158);
            vVDecWriteVLD(u4VDecID, 159 * 4, VLD_reg_159);
            //Printf("New VLD before Setting\n");
            //Printf("%d (0x%4X) = 0x%8X\n", 157, (157<<2), u4VDecReadVLD(u4VDecID, 157 * 4));
            //Printf("%d (0x%4X) = 0x%8X\n", 158, (158<<2), u4VDecReadVLD(u4VDecID, 158 * 4));
            //Printf("%d (0x%4X) = 0x%8X\n", 159, (159<<2), u4VDecReadVLD(u4VDecID, 159 * 4));
            //Printf("%d (0x%4X) = 0x%8X\n", 134, (134<<2), u4VDecReadVLD(u4VDecID, 134 * 4));

            //Printf("prWmvDecPrm->prPPS->i4DCStepSize value:%x\n",prWmvDecPrm->prPPS->i4DCStepSize);
            DCAC_reg_1 = (1024 + (prWmvDecPrm->prPPS->i4DCStepSize >> 1)) / prWmvDecPrm->prPPS->i4DCStepSize;
            vVDecWriteWMVDCAC(u4VDecID, 1 * 4, DCAC_reg_1);

            DCAC_reg_0 = 0;
            DCAC_reg_0 |= 0x1;
            DCAC_reg_0 |= (prWmvDecPrm->prPPS->i4StepSize<< 8);
            //Printf("prWmvDecPrm->prPPS->ucPicType == 0x%x\n",prWmvDecPrm->prPPS->ucPicType);
            if(prWmvDecPrm->prPPS->ucPicType == PVOP)
                DCAC_reg_0 |= (prWmvDecPrm->prPPS->fgDCPredIMBInPFrame << 2);
            vVDecWriteWMVDCAC(u4VDecID, 0, DCAC_reg_0);

            //Printf("WMVDCAC before Setting\n");
            //Printf("%d (0x%4X) = 0x%8X\n", 0, (0<<2), u4VDecReadWMVDCAC(u4VDecID, 0));
            //Printf("%d (0x%4X) = 0x%8X\n", 1, (1<<2), u4VDecReadWMVDCAC(u4VDecID, 1 * 4));   
        }
    }
}
#endif

#if 0
{
    vVDEC_HAL_WMV_VDec_DumpReg (u4VDecID, TRUE);
 #if 0 
    UINT32 u4Val;
    int reg;
    int vldstart = 42;
    int vldsize = 200;
    int mcstart = 0;
    int mcsize = 400;
    int dsclstart = 0;
    int dsclsize = 20;
    if (_VDecNeedDumpRegister(u4VDecID))
    {
        printk("VLD Before Settings\n");
        for(reg= 33; reg < 39; reg++)
        {          
            u4Val = u4VDecReadVLD(u4VDecID, (reg << 2));
            printk("%d (0x%x) = 0x%4x\n", reg, (reg<<2), u4Val);
        }        
        for(reg= vldstart; reg < (vldstart+vldsize); reg++)
        {          
            u4Val = u4VDecReadVLD(u4VDecID, (reg << 2));
            printk("%d (0x%x) = 0x%4x\n", reg, (reg<<2), u4Val);
        }
        
        printk("MC Before Settings\n");
        for(reg= mcstart; reg < (mcstart+mcsize); reg++)
        {          
            u4Val = u4VDecReadMC(u4VDecID, (reg << 2));
            printk("%d (0x%x) = 0x%4x\n", reg, (reg<<2), u4Val);
        }
        printk("VLDTOP Before Setting\n");
        for(reg = 0; reg < 100; reg++)
        {
            u4Val = u4VDecReadTopVLD(u4VDecID, (reg << 2));
            printk("%d (0x%x) = 0x%4x\n", reg, (reg<<2), u4Val);
        }
        printk("DSCL Before Setting\n");
        for(reg = dsclstart; reg < (dsclstart+dsclsize); reg++)
        {
            u4Val = u4ReadVDSCL(u4VDecID, (reg << 2));
            printk("%d (0x%x) = 0x%4x\n", reg, (reg<<2), u4Val);
        }
    }
    #endif
}
#endif

#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8560)
 u4RegValue= u4VDecReadMC(u4VDecID, RW_MC_DDR3_EN);
 u4RegValue = (u4RegValue | 0x00000010);
 vVDecWriteMC(u4VDecID, RW_MC_DDR3_EN, u4RegValue);
#endif

/*sun for enable wmv crc*/
 #ifdef WMV_CRCCHECK_ENABLE
    u4CRC_Agent = u4VDecReadMC(u4VDecID, RW_VLD_WMV_PARA3);
    vWriteReg((VDEC_CRC_REG_OFFSET0 + 0x4), ((u4CRC_Agent << 1)+ 0x1));
 #endif
 
#if (CONFIG_DRV_VERIFY_SUPPORT) && (CONFIG_DRV_LINUX)    
   // HalFlushInvalidateDCache();
#endif

    #if WMV_LOG_TMP
    printk("WMV_DecStart, Input window :0x%x, 0x%x\n", u4VDecVLDGetBitS(0, 0, 0), u4VDecReadVLD(u4VDecID, 0xf0));
    #endif
    
    #if (WMV_8320_SUPPORT)
     {
      UINT32 u4VldVal = u4VDecReadVLD(u4VDecID, RW_VLD_LDSH);
      
      #if WMV_LOG_TMP
      printk("i4VDEC_HAL_WMV_DecStart, Reg 0x1D0:0x%x\n", 
        u4VldVal);
      #endif

      u4VldVal &= ~(1 << 8);
        
      vVDecWriteVLD(u4VDecID, RW_VLD_LDSH, u4VldVal); // for 8320
    }
    #endif
    mb();
    vVDecWriteVLD(u4VDecID, RW_VLD_WMVDEC, WMV_DEC_START);
    mb();
    vVDecWriteVLD(u4VDecID, RW_VLD_WMVDEC, 0x00000000);
    printk("i4VDEC_HAL_WMV_DecStart end\n");
    return HAL_HANDLE_OK;
}


// **************************************************************************
// Function : INT32 i4VDEC_HAL_WMV_VPStart(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm);
// Description :Set video decoder hardware registers to vp for wmv
// Parameter : prDecWmvPrm : pointer to wmv decode info struct
// Return      : =0: success.
//                  <0: fail.
// **************************************************************************
INT32 i4VDEC_HAL_WMV_VPStart(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm)
{
    VDEC_INFO_WMV_DEC_PRM_T *prWmvDecPrm = (VDEC_INFO_WMV_DEC_PRM_T *)prDecPrm->prVDecCodecHalPrm;
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
	UINT32 u4PicW,u4PicH,u4WithMB;
#endif
    
    vVDECSetDownScalerPrm(u4VDecID, &prDecPrm->rDownScalerPrm);

#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560) && CONFIG_DRV_FTS_SUPPORT)
    // set letterbox detection parameter
    vVDECSetLetetrBoxDetPrm(u4VDecID, &prDecPrm->rLBDPrm);
#endif    


    #if VMMU_SUPPORT
    {
    UINT32 u4Page_addr = 0;
    
    u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((prWmvDecPrm->rWmvWorkBufSa.u4Pic0YSa)/(4*1024))*4);
    printk("[WMV] i4VDEC_HAL_WMV_VPStart, 1, Page Addr = 0x%x\n", u4Page_addr);    
    vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic0YSa), PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic0YSa) + PIC_Y_SZ);
    vVDecWriteMC(u4VDecID, RW_MC_R1Y, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic0YSa) >> 9); 
    vVDecWriteMC(u4VDecID, RW_MC_DIGY, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic0YSa) >> 9); 

    u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((prWmvDecPrm->rWmvWorkBufSa.u4Pic0CSa)/(4*1024))*4);
    printk("[WMV] i4VDEC_HAL_WMV_VPStart, 2, Page Addr = 0x%x\n", u4Page_addr);    
    vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic0CSa), PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic0CSa) + PIC_C_SZ);
    vVDecWriteMC(u4VDecID, RW_MC_R1C, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic0CSa) >> 8); 
    vVDecWriteMC(u4VDecID, RW_MC_DIGC, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic0CSa) >> 8);

    u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((prWmvDecPrm->rWmvWorkBufSa.u4Pic1YSa)/(4*1024))*4);
    printk("[WMV] i4VDEC_HAL_WMV_VPStart, 3, Page Addr = 0x%x\n", u4Page_addr);    
    vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic1YSa), PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic1YSa) + PIC_Y_SZ);
    vVDecWriteMC(u4VDecID, RW_MC_R2Y, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic1YSa) >> 9); 

    u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((prWmvDecPrm->rWmvWorkBufSa.u4Pic1CSa)/(4*1024))*4);
    printk("[WMV] i4VDEC_HAL_WMV_VPStart, 4, Page Addr = 0x%x\n", u4Page_addr);    
    vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic1CSa), PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic1CSa) + PIC_C_SZ);
    vVDecWriteMC(u4VDecID, RW_MC_R2C, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic1CSa) >> 8); 

    u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((prWmvDecPrm->rWmvWorkBufSa.u4Pic2YSa)/(4*1024))*4);
    printk("[WMV] i4VDEC_HAL_WMV_VPStart, 5, Page Addr = 0x%x\n", u4Page_addr);    
    vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic2YSa), PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic2YSa) + PIC_Y_SZ);
    vVDecWriteMC(u4VDecID, RW_MC_BY, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic2YSa) >> 8); 
    vVDecWriteMC(u4VDecID, RW_MC_BY1, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic2YSa) >> 9); 

    u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((prWmvDecPrm->rWmvWorkBufSa.u4Pic2CSa)/(4*1024))*4);
    printk("[WMV] i4VDEC_HAL_WMV_VPStart, 6, Page Addr = 0x%x\n", u4Page_addr);    
    vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic2CSa), PHYSICAL((UINT32)prWmvDecPrm->rWmvWorkBufSa.u4Pic2CSa) + PIC_C_SZ);
    vVDecWriteMC(u4VDecID, RW_MC_BC, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic2CSa) >> 7); 
    vVDecWriteMC(u4VDecID, RW_MC_BC1, ((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic2CSa) >> 8); 
    }
    #else
    vVDecWriteMC(u4VDecID, RW_MC_R1Y, (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic0YSa)) >> 9); // div 512
    vVDecWriteMC(u4VDecID, RW_MC_R1C, (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic0CSa)) >> 8); // div 256
    vVDecWriteMC(u4VDecID, RW_MC_R2Y, (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic1YSa)) >> 9); // div 512
    vVDecWriteMC(u4VDecID, RW_MC_R2C, (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic1CSa)) >> 8); // div 256
    vVDecWriteMC(u4VDecID, RW_MC_BY,  (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic2YSa)) >> 8); // div 256
    vVDecWriteMC(u4VDecID, RW_MC_BC,  (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic2CSa)) >> 7); // div 128
    vVDecWriteMC(u4VDecID, RW_MC_BY1,  (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic2YSa)) >> 9); // div 256
    vVDecWriteMC(u4VDecID, RW_MC_BC1,  (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic2CSa)) >> 8); // div 128
    vVDecWriteMC(u4VDecID, RW_MC_DIGY, (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic0YSa)) >> 9); // div 512
    vVDecWriteMC(u4VDecID, RW_MC_DIGC, (u4AbsDramANc((UINT32) prWmvDecPrm->rWmvWorkBufSa.u4Pic0CSa)) >> 8); // div 256    
    #endif
    vMCSetOutputBuf(u4VDecID, prDecPrm->ucDecFBufIdx, prWmvDecPrm->u4FRefBufIdx);

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
  #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
      vVDecWriteMC(u4VDecID, RW_MC_PIC_W_MB, ((prDecPrm->u4PicBW + 15)>> 4)); 
  #else
   vVDecWriteVLD(u4VDecID, RW_VLD_PIC_W_MB, ((prDecPrm->u4PicBW + 15)>> 4));  
#endif
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
  
    vVDecWriteVLD(u4VDecID, RW_VLD_PSUPCTR, ((prDecPrm->u4PicW * prDecPrm->u4PicH) >> 8) + 1);
    vVDecWriteVLD(u4VDecID, RW_VLD_PARA, 0xC0500000); //Frame Picture + VP ???    
    #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
//    vVDecWriteVLD(u4VDecID, RW_VLD_PICSZ, ((prDecPrm->u4PicH) << 16) + (prDecPrm->u4PicW >> 4));
	
	u4PicW = ((prDecPrm->u4PicW +15)>>4)<<4;
	u4PicH = ((prDecPrm->u4PicH + 15)>>4)<<4;
	u4WithMB = ((prDecPrm->u4PicW +15)>>4);
	vVDecWriteVLDTOP(u4VDecID,RW_TOPVLD_WMV_PICSIZE,u4PicH<<16|u4PicW);
	vVDecWriteVLD(u4VDecID, RW_VLD_PICSZ, (prDecPrm->u4PicH)<<16);
	vVDecWriteVLD(u4VDecID, RW_VLD_DIGMBSA, u4WithMB);
//	vVDecWriteTopVLD(u4VDecID,RW_TOPVLD_WMV_PICSIZE_MB,(((prDecPrm->u4PicW+ 15)>>4) -1) | ((((prDecPrm->u4PicH + 15)>>4) - 1)<<16));
	vVDecWriteVLD(u4VDecID, RW_VLD_MBROWPRM,  0x1ff );
    vVDecWriteDV(u4VDecID,VDEC_DV_INT_CFG,u4VDecReadDV(u4VDecID,VDEC_DV_INT_CFG)&(~(1<<12)));
    #else
    vVDecWriteVLD(u4VDecID, RW_VLD_PICSZ, ((prDecPrm->u4PicH + 15) << 16) + (prDecPrm->u4PicW >> 4));
    vVDecWriteVLD(u4VDecID, RW_VLD_MBROWPRM,  ( ((prDecPrm->u4PicH + 15) >> 4 ) - 1) << 16);
    #endif

    #if VDEC_FIELD_COMPACT
    #else
    // addr swap mode
    vVDecWriteMC(u4VDecID, RW_MC_ADDRSWAP, prDecPrm->ucAddrSwapMode);
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
  #if (CONFIG_DRV_LINUX_DATA_CONSISTENCY)
    //HalFlushInvalidateDCache();
  #endif
    
    vVDecWriteVLD(u4VDecID, RW_VLD_PROC, VLD_RTERR + VLD_PDHW + VLD_PSUP +
              (prDecPrm->u4PicW >> 4));
              
    return HAL_HANDLE_OK;
}


// The following functions are only for verify
void vVDEC_HAL_WMV_DecEndProcess(UINT32 u4VDecID)
{
    // load wmv-sum to mpeg2-sum
    vVDecWriteVLD(u4VDecID, RW_VLD_LDSH, 0x1);
    mb();
    vVDecWriteVLD(u4VDecID, RW_VLD_LDSH, 0x0);
    
    #if (WMV_8320_SUPPORT)
    {
      UINT32 u4VldVal = u4VDecReadVLD(u4VDecID, RW_VLD_LDSH);
      
      #if WMV_LOG_TMP
      printk("WMV_DecEndProcess, Reg 0x1D0:0x%x\n", 
        u4VldVal);
      #endif

      u4VldVal |= (1 << 8);
        
      vVDecWriteVLD(u4VDecID, RW_VLD_LDSH, u4VldVal); // for 8320
    }
    #endif
    
    // switch to mpeg2 mode
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8555)
	 if (BSP_GetIcVersion() == IC_8555)
        {
    if(u4VDecID == 1)//add for lite random error in mt8555 verification @>Youlin Pei
    {
        vWriteReg(0x32000,(u4ReadReg(0x32000) | 0x1));
    }
    else
    {
        vWriteReg(0x2B000,(u4ReadReg(0x2B000) | 0x1));
    }
        }
#endif
    #if (!WMV_8320_SUPPORT)
    vVDecWriteVLD(u4VDecID, RW_VLD_WMV_MODE, 0x0);
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

#if (CONFIG_DRV_ONLY || CONFIG_DRV_VERIFY_SUPPORT)
void vVDEC_HAL_WMV_VDec_DumpReg(UINT32 u4VDecID, BOOL fgBefore)
{
  INT32 i; 
  INT32 i4VldStart = 42;
  INT32 i4VldEnd = 256;
  INT32 i4McStart = 0;
      INT32 i4McEnd = 700;
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
  INT32 i4TopVldStart = 0;
  INT32 i4TopVldEnd = 100;
#endif
  UINT32 u4Data;
  
  printk("WMV Dump Register: ");
  if (fgBefore == TRUE)
  {
     printk("Before Decode\n");
  }
  else
  {
     printk("After Decode\n");
  }

  printk("VLD Registers\n");
     //Enable some register read 

      vVDecWriteVLD(u4VDecID, RW_VLD_WMV_MODE, 1);

  for (i= i4VldStart; i<i4VldEnd; i++)
  {
      u4Data = u4VDecReadVLD(u4VDecID, (i<<2));
      printk("%04d (0x%04X) = 0x%08X\n", i, (i<<2), u4Data);
      msleep(10);
  }

  printk("MC Registers\n");
  for (i=i4McStart; i<i4McEnd; i++)
  {
     u4Data = u4VDecReadMC(u4VDecID, (i<<2));
     printk("%04d (0x%04X) = 0x%08X\n", i, (i<<2), u4Data);
     msleep(20);
  }
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
   printk("TOPVLD Registers\n");
   for (i=i4TopVldStart; i<i4TopVldEnd; i++)
   {
      u4Data = u4VDecReadVLDTOP(u4VDecID, (i<<2));
      printk("%04d (0x%04X) = 0x%08X\n", i, (i<<2), u4Data);
      msleep(10);
   }
#endif

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8555)
        printk("Y CRC Registers\n");
        for (i=0; i<4; i++)
        {
          u4Data = u4ReadReg(VDEC_CRC_REG_OFFSET0 + ((2+i)*4));
          printk("%04d (0x%04X) = 0x%08X\n", (2+i), ((2+i)<<2), u4Data);      
         }
        printk("C CRC Registers\n");
        for (i=0; i<4; i++)
        {
          u4Data =  u4ReadReg(VDEC_CRC_REG_OFFSET0 + ((6+i)*4));
          printk("%04d (0x%04X) = 0x%08X\n", (6+i), ((6+i)<<2), u4Data);      
         }
        
         printk("MV Registers\n");
        for (i=130; i<182; i++)
        {
          u4Data =  u4ReadReg(AVC_MV_REG_OFFSET0 + (i*4));
          printk("%04d (0x%04X) = 0x%08X\n", i, (i<<2), u4Data);      
          msleep(10);
         }
        
        printk("IS Registers\n");
        for (i=128; i<192; i++)
        {
          u4Data =  u4ReadReg(AVS_PP_REG_OFFSET0 + (i*4));
          printk("%04d (0x%04X) = 0x%08X\n", i, (i<<2), u4Data);      
          msleep(10);
         }
        printk("IQ Registers\n");
         for (i=320; i<384; i++)
        {
          u4Data =  u4ReadReg(AVS_PP_REG_OFFSET0 + (i*4));
          printk("%04d (0x%04X) = 0x%08X\n", i, (i<<2), u4Data);      
          msleep(100);
         }
         printk("IT Registers\n");
         for (i=576; i<640; i++)
        {
          u4Data =  u4ReadReg(AVS_PP_REG_OFFSET0 + (i*4));
          printk("%04d (0x%04X) = 0x%08X\n", i, (i<<2), u4Data);      
          msleep(100);
         }
         
         printk("DCAC Registers\n");
         for (i=448; i<512; i++)
        {
          u4Data =  u4ReadReg(AVS_PP_REG_OFFSET0 + (i*4));
          printk("%04d (0x%04X) = 0x%08X\n", i, (i<<2), u4Data);      
          msleep(10);
         }
#endif
    
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
    #if (!WMV_8320_SUPPORT)
         vVDecWriteVLD(u4VDecID, RW_VLD_WMV_MODE, 0);
    #endif
}
#endif

#if CONFIG_DRV_VERIFY_SUPPORT
void vVDEC_HAL_WMV_VDec_ReadCheckSum(UINT32 u4VDecID, UINT32 *pu4DecCheckSum)
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

BOOL fgVDEC_HAL_WMV_VDec_CompCheckSum(UINT32 *pu4DecCheckSum, UINT32 *pu4GoldenCheckSum)
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

UINT32 u4VDEC_HAL_WMV_VDec_ReadFinishFlag(UINT32 u4VDecID)
{
  return u4VDecReadVLD(u4VDecID, RO_VLD_PIC_DEC_END);
}

#endif

