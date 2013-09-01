
//#include "x_hal_1176.h"
//#include "chip_ver.h" 
 
#include "vdec_hw_common.h"
#include "vdec_hal_if_rm.h"
#include "vdec_hw_rm.h"
#include "vdec_hal_errcode.h"
//#include "drv_fs.h"
#include <linux/string.h>

#if CONFIG_DRV_VERIFY_SUPPORT
//#include "x_printf.h"
//#include "x_debug.h"

#include "../verify/vdec_verify_general.h"
#include "../verify/vdec_verify_mpv_prov.h"
#include "../verify/vdec_verify_vparser_rm.h"
#endif

#if VMMU_SUPPORT
#include "vdec_hal_if_common.h"
#endif


//Hal Define Config
//#define VDEC_RM_HAL_ENABLELOG


void vRM_Hal_InitVDecHW(UINT32 u4VDecID, VDEC_INFO_RM_VFIFO_PRM_T *prRMVFifoPrm)
{
#if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8555)
  vVDecResetHW(u4VDecID);
#elif (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560)
    vVDecResetHW(u4VDecID, VDEC_RM);
#else
    vVDecResetHW(u4VDecID, VDEC_UNKNOWN);
#endif
  vVDecSetVLDVFIFO(0, u4VDecID, PHYSICAL(prRMVFifoPrm->u4VFifoSa), PHYSICAL(prRMVFifoPrm->u4VFifoEa));
}


void vRM_Hal_VldSoftReset(UINT32 u4VDecID)
{
  vRM_VldSoftReset(u4VDecID);
}

void vRM_Hal_MvInit(UINT32 u4VDecID, UINT32 u4MVHwWorkBuf)
{
  vRM_MvInit(u4VDecID, u4MVHwWorkBuf);
}

void vRM_Hal_McInit(UINT32 u4VDecID)
{
  vRM_McInit(u4VDecID);
}

void vRM_Hal_PpInit(UINT32 u4VDecID)
{
  vRM_PpInit(u4VDecID);
}

  //Init RM BS HW
void vRM_Hal_VldInit(UINT32 u4VDecID, UINT32 u4VldPredWorkBuf)
{
  vRM_VldInit(u4VDecID, u4VldPredWorkBuf);
}


void vRM_HAL_InitVDecHW(UINT32 u4VDecID, VDEC_INFO_RM_VFIFO_PRM_T *prRMVFifoPrm)
{
#if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8555)
    vVDecResetHW(u4VDecID);
#elif (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560)
    vVDecResetHW(u4VDecID, VDEC_RM);
#else
    vVDecResetHW(u4VDecID, VDEC_UNKNOWN);
#endif
    vVDecSetVLDVFIFO(0, u4VDecID, 
    	                              PHYSICAL(prRMVFifoPrm->u4VFifoSa), 
    	                              PHYSICAL(prRMVFifoPrm->u4VFifoEa));    
}

  

INT32 i4RM_HAL_InitBarrelShifter(UINT32 u4BSID, 
                                                               UINT32 u4VDecID, 
                                                               VDEC_INFO_RM_BS_INIT_PRM_T *prMpegBSInitPrm)
{
  INT32 iRet = 0;

  iRet = i4RM_InitBarrelShifter(u4BSID, u4VDecID, prMpegBSInitPrm);

  return iRet;
}

UINT32 u4VDEC_HAL_RM_ShiftGetBitStream(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits)
{
  DWRD dwRetVal;
 
  dwRetVal = (u4RM_VDecVLDGetBitS(u4BSID, u4VDecID, 0) >> (32 - u4ShiftBits));
  u4RM_VDecVLDGetBitS(u4BSID, u4VDecID, u4ShiftBits);

  return dwRetVal;
}


//#if (CONFIG_DRV_ONLY || CONFIG_DRV_VERIFY_SUPPORT)
#if CONFIG_DRV_VERIFY_SUPPORT
void vVDEC_HAL_RM_VDec_DumpReg(UINT32 u4VDecID, BOOL fgBefore)
{
  INT32 i; 
  INT32 i4VldStart = 42;
  INT32 i4VldEnd = 256;
  INT32 i4RMVldStart = 0;
  INT32 i4RMVldEnd = 400;
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
  INT32 i4TopVldStart = 0;
  INT32 i4TopVldEnd = 100;
#endif  
  UINT32 u4Data;
  
  printk("RM  Dump Register: ");
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
  for (i=i4RMVldStart; i<i4RMVldEnd; i++)
  {
     u4Data = u4VDecReadMC(u4VDecID, (i<<2));
     printk("%04d (0x%04X) = 0x%08X\n", i, (i<<2), u4Data);
  }
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
   printk("TOPVLD Registers\n");
   for (i=i4TopVldStart; i<i4TopVldEnd; i++)
   {
      u4Data = u4VDecReadVLDTOP(u4VDecID, (i<<2));
      printk("%04d (0x%04X) = 0x%08X\n", i, (i<<2), u4Data);
   }
#endif
    
}
#endif

#if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8560)
void vVDEC_HAL_RM_TriggerDecode(UINT32 u4VDecID, VDEC_INFO_RM_PICINFO_T *prParsingPic)
#else
void vVDEC_HAL_RM_TriggerDecode(UINT32 u4VDecID, VDEC_INFO_RM_PICINFO_T *prParsingPic, VDEC_INFO_LBD_PRM_T *prLBDInfo)
#endif
{
  UINT32 u4RegValue;
  UINT32 u4Rv8TblNum  =1;
  UINT32 i;
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
  UINT32 u4CycleCnt = 0x1;
#else
  UINT32 u4CycleCnt = 0x00800000;
#endif
  UINT32 u4ErrConceal = 0;
  UINT32 u4FastParsing = 0x40000000;  

  prParsingPic->u4InitInputWindow = u4VDecReadRMVLD(u4VDecID, 0x0);
 
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560) && CONFIG_DRV_FTS_SUPPORT)
    // set letterbox detection parameter
    if (NULL != prLBDInfo)
      vVDECSetLetetrBoxDetPrm(u4VDecID, prLBDInfo);
#endif  


  //Only for MT8555 + CRC HW 
 #ifdef CONFIG_DRV_VERIFY_SUPPORT
 #ifdef RM_CRCCHECKFLOW_SUPPORT
  if (prParsingPic->fgEnableCRC)
  {
    //Enable CRC HW
    vVDecWriteRMCRC(u4VDecID, 0x4, 0x3);
  }
 #endif
 #endif
  
  u4RegValue = (prParsingPic->u4Iratio & 0x0000FFFF);
  vVDecWriteRMMV(u4VDecID, RW_RMMV_IRATIO0, u4RegValue);
    
  if (prParsingPic->ePtype == RM_TRUEBPIC)
  {
    u4RegValue = prParsingPic->fgBwdIsI;
    vVDecWriteRMMV(u4VDecID, RW_RMMV_CPLOCPICTYPE, u4RegValue);
  }

  if (prParsingPic->fgEnableMCOutput)
  {
    u4RegValue= u4VDecReadMC(u4VDecID, RW_MC_OPBUF);
    u4RegValue = u4RegValue & 0xFFFFFFF8;
    u4RegValue = u4RegValue | (4<<0);   	
  
    vVDecWriteMC(u4VDecID, RW_MC_OPBUF, u4RegValue);
  }
  
  if (prParsingPic->fgEnableMCOutput)
  {
    //MC Luma Addr
    #if (CONFIG_DRV_VERIFY_SUPPORT == 1)
    #if VMMU_SUPPORT
    {
    UINT32 u4Page_addr = 0;

    u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + ((((UINT32) prParsingPic->u4MCOutBufY)/(4*1024))*4);
    printk("[RM] vVDEC_HAL_RM_TriggerDecode, 1, Page Addr = 0x%x\n", u4Page_addr);    
    vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32) prParsingPic->u4MCOutBufY), PHYSICAL((UINT32) prParsingPic->u4MCOutBufY) + PIC_Y_SZ);
    u4RegValue = (UINT32) (prParsingPic->u4MCOutBufY) >> 9;
    }
    #else
    u4RegValue = (UINT32) (PHYSICAL(prParsingPic->u4MCOutBufY)>>9);
    #endif
    #else //CONFIG_DRV_VERIFY_SUPPORT
    u4RegValue = (UINT32) (prParsingPic->u4MCOutBufY)>>9;
    #endif //CONFIG_DRV_VERIFY_SUPPORT
    vVDecWriteMC(u4VDecID, RW_MC_BY1, u4RegValue);
      
    //MC Chroma Addr
    #if (CONFIG_DRV_VERIFY_SUPPORT == 1)
    #if VMMU_SUPPORT
    {
    UINT32 u4Page_addr = 0;

    u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + ((((UINT32) prParsingPic->u4MCOutBufC)/(4*1024))*4);
    printk("[RM] vVDEC_HAL_RM_TriggerDecode, 2, Page Addr = 0x%x\n", u4Page_addr);    
    vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32) prParsingPic->u4MCOutBufC), PHYSICAL((UINT32) prParsingPic->u4MCOutBufC) + PIC_C_SZ);
    u4RegValue = (UINT32) (prParsingPic->u4MCOutBufC) >> 8;
    }
    #else
    u4RegValue = (UINT32) (PHYSICAL(prParsingPic->u4MCOutBufC)>>8);
    #endif
    #else //CONFIG_DRV_VERIFY_SUPPORT
    u4RegValue = (UINT32) (prParsingPic->u4MCOutBufC)>>8;
    #endif //CONFIG_DRV_VERIFY_SUPPORT
    vVDecWriteMC(u4VDecID, RW_MC_BC1, u4RegValue);
  }
  else 
  {
    //MC Luma Addr
    #if (CONFIG_DRV_VERIFY_SUPPORT == 1)
    #if VMMU_SUPPORT
    {
    UINT32 u4Page_addr = 0;

    u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + ((((UINT32) prParsingPic->u4OutBufY)/(4*1024))*4);
    printk("[RM] vVDEC_HAL_RM_TriggerDecode, 3, Page Addr = 0x%x\n", u4Page_addr);    
    vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32) prParsingPic->u4OutBufY), PHYSICAL((UINT32) prParsingPic->u4OutBufY) + PIC_Y_SZ);
    u4RegValue = (UINT32) (prParsingPic->u4OutBufY) >> 9;
    }
    #else
    u4RegValue = (UINT32) PHYSICAL((UINT32) prParsingPic->u4OutBufY)>>9;
    #endif
    #else //CONFIG_DRV_VERIFY_SUPPORT
    u4RegValue = (UINT32) (prParsingPic->u4OutBufY)>>9;
    #endif //CONFIG_DRV_VERIFY_SUPPORT
    vVDecWriteMC(u4VDecID, RW_MC_BY1, u4RegValue);
    
    //MC Chroma Addr
    #if (CONFIG_DRV_VERIFY_SUPPORT == 1)
    #if VMMU_SUPPORT
    {
    UINT32 u4Page_addr = 0;

    u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + ((((UINT32) prParsingPic->u4OutBufC)/(4*1024))*4);
    printk("[RM] vVDEC_HAL_RM_TriggerDecode, 4, Page Addr = 0x%x\n", u4Page_addr);    
    vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32) prParsingPic->u4OutBufC), PHYSICAL((UINT32) prParsingPic->u4OutBufC) + PIC_C_SZ);
    u4RegValue = (UINT32) (prParsingPic->u4OutBufC) >> 8;
    }
    #else
    u4RegValue = (UINT32) PHYSICAL((UINT32) prParsingPic->u4OutBufC)>>8;
    #endif
    #else //CONFIG_DRV_VERIFY_SUPPORT
    u4RegValue = (UINT32) (prParsingPic->u4OutBufC)>>8;
    #endif //CONFIG_DRV_VERIFY_SUPPORT
    vVDecWriteMC(u4VDecID, RW_MC_BC1, u4RegValue);
  }

  #if (CONFIG_DRV_VERIFY_SUPPORT == 1)
  #if VMMU_SUPPORT
  {
  UINT32 u4Page_addr = 0;

  u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + ((((UINT32) prParsingPic->u4OutBufY)/(4*1024))*4);
  printk("[RM] vVDEC_HAL_RM_TriggerDecode, 5, Page Addr = 0x%x\n", u4Page_addr);    
  vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32) prParsingPic->u4OutBufY), PHYSICAL((UINT32) prParsingPic->u4OutBufY) + PIC_Y_SZ);
  u4RegValue = (UINT32) (prParsingPic->u4OutBufY) >> 9;
  }
  #else
  u4RegValue = (UINT32) PHYSICAL((UINT32) prParsingPic->u4OutBufY)>>9;
  #endif
  #else //CONFIG_DRV_VERIFY_SUPPORT
  u4RegValue = (UINT32) (prParsingPic->u4OutBufY)>>9;
  #endif //CONFIG_DRV_VERIFY_SUPPORT
  vVDecWriteMC(u4VDecID, RW_MC_PP_Y_ADDR, u4RegValue);
  
  #if (CONFIG_DRV_VERIFY_SUPPORT == 1)
  #if VMMU_SUPPORT
  {
  UINT32 u4Page_addr = 0;

  u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + ((((UINT32) prParsingPic->u4OutBufC)/(4*1024))*4);
  printk("[RM] vVDEC_HAL_RM_TriggerDecode, 6, Page Addr = 0x%x\n", u4Page_addr);    
  vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32) prParsingPic->u4OutBufC), PHYSICAL((UINT32) prParsingPic->u4OutBufC) + PIC_C_SZ);
  u4RegValue = (UINT32) (prParsingPic->u4OutBufC) >> 8;
  }
  #else
  u4RegValue = (UINT32) PHYSICAL((UINT32) prParsingPic->u4OutBufC)>>8;
  #endif
  #else //CONFIG_DRV_VERIFY_SUPPORT
  u4RegValue = (UINT32) (prParsingPic->u4OutBufC)>>8;
  #endif //CONFIG_DRV_VERIFY_SUPPORT
  vVDecWriteMC(u4VDecID, RW_MC_PP_C_ADDR, u4RegValue);

  u4RegValue = 0x1;
  vVDecWriteMC(u4VDecID, RW_MC_PP_ENABLE, u4RegValue);    //Enable MC PP
  
  if (prParsingPic->fgEnableDDR3)
  {
    #ifdef RM_DDR3MODE_DEBUG_ENABLE
    u4RegValue = (prParsingPic->u4Width/16);
    vVDecWriteMC(u4VDecID, RW_MC_PP_MB_WIDTH, u4RegValue);
    #else //RM_DDR3MODE_DEBUG_ENABLE
    u4RegValue = prParsingPic->u4Width/ 16;
    if (u4RegValue % 4)
    {
      u4RegValue = u4RegValue + (4 - (u4RegValue % 4));
    }
    
    vVDecWriteMC(u4VDecID, RW_MC_PP_MB_WIDTH, u4RegValue);
    #endif //RM_DDR3MODE_DEBUG_ENABLE
  }
  else
  {
    u4RegValue = (prParsingPic->u4Width/16);
    vVDecWriteMC(u4VDecID, RW_MC_PP_MB_WIDTH, u4RegValue);
  }

  u4RegValue = (prParsingPic->fgDeblockPassThru == 0 ? 
  //u4RegValue = (prParsingPic->u4DFP == 0 ? 
  	                  ((!prParsingPic->fgRV9 && 
  	                    (prParsingPic->ePtype != RM_INTRAPIC && prParsingPic->ePtype != RM_FORCED_INTRAPIC)) ? 2 : 3) : 0);
  vVDecWriteMC(u4VDecID, RW_MC_PP_DBLK_MODE, u4RegValue);

  if (prParsingPic->fgEnableMCOutput)
  {
    u4RegValue= u4VDecReadMC(u4VDecID, RW_MC_PP_WB_BY_POST);
    u4RegValue = u4RegValue & 0xFFFFFFFE;
    
    vVDecWriteMC(u4VDecID, RW_MC_PP_WB_BY_POST, u4RegValue);
  }
  else
  {
    u4RegValue = 0x1;
    vVDecWriteMC(u4VDecID, RW_MC_PP_WB_BY_POST, u4RegValue);    //?????
  }

  u4RegValue = (prParsingPic->u4Width/16)-1;
  vVDecWriteMC(u4VDecID, RW_MC_PP_X_RANGE, u4RegValue);

  u4RegValue = (prParsingPic->u4Height/16)-1;
  vVDecWriteMC(u4VDecID, RW_MC_PP_Y_RANGE, u4RegValue);
  
  if (prParsingPic->ePtype == RM_TRUEBPIC)
  {
    u4RegValue = prParsingPic->u4RefQpMb0 << 16;
  }
  else
  {
    u4RegValue = prParsingPic->u4PSQuant << 16;
    //u4RegValue = prParsingPic->u4Pquant << 16;
  }
  
  u4RegValue |= (prParsingPic->ePtype == RM_TRUEBPIC ? 1 : 0) << 11;
  u4RegValue |= (prParsingPic->ePtype == RM_TRUEBPIC ? 1 : 0) << 10;
  u4RegValue |= prParsingPic->fgDeblockPassThru << 9;
  //u4RegValue |= prParsingPic->u4DFP << 9;
  u4RegValue |= (prParsingPic->u4Width * prParsingPic->u4Height <= 176 * 144 ? 1 : 0) << 8;
  u4RegValue |= u4Rv8TblNum << 4;
  u4RegValue |= (prParsingPic->fgRV9 ? 1 : 0) << 1;
  u4RegValue |= (prParsingPic->fgRV9 ? 0 : 1);
  vVDecWriteRMPP(u4VDecID, RW_RMPP_PARAM, u4RegValue);

  // MC setting
  if (prParsingPic->ePtype == RM_INTERPIC || prParsingPic->ePtype == RM_TRUEBPIC)
  {
    #if (CONFIG_DRV_VERIFY_SUPPORT == 1)
    #if VMMU_SUPPORT
    {
    UINT32 u4Page_addr = 0;

    u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + ((((UINT32) prParsingPic->u4FwdBufY)/(4*1024))*4);
    printk("[RM] vVDEC_HAL_RM_TriggerDecode, 7, Page Addr = 0x%x\n", u4Page_addr);    
    vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32) prParsingPic->u4FwdBufY), PHYSICAL((UINT32) prParsingPic->u4FwdBufY) + PIC_Y_SZ);
    u4RegValue = (UINT32) (prParsingPic->u4FwdBufY) >> 9;
    }
    #else
    u4RegValue = (UINT32) PHYSICAL((UINT32) prParsingPic->u4FwdBufY) >> 9;
    #endif
    #else //CONFIG_DRV_VERIFY_SUPPORT
    u4RegValue = (UINT32) (prParsingPic->u4FwdBufY) >> 9;
    #endif //CONFIG_DRV_VERIFY_SUPPORT
    vVDecWriteMC(u4VDecID, RW_MC_R1Y, u4RegValue);

    #if (CONFIG_DRV_VERIFY_SUPPORT == 1)
    #if VMMU_SUPPORT
    {
    UINT32 u4Page_addr = 0;

    u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + ((((UINT32) prParsingPic->u4FwdBufC)/(4*1024))*4);
    printk("[RM] vVDEC_HAL_RM_TriggerDecode, 8, Page Addr = 0x%x\n", u4Page_addr);    
    vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32) prParsingPic->u4FwdBufC), PHYSICAL((UINT32) prParsingPic->u4FwdBufC) + PIC_C_SZ);
    u4RegValue = (UINT32) (prParsingPic->u4FwdBufC) >> 8;
    }
    #else
    u4RegValue = (UINT32) PHYSICAL((UINT32) prParsingPic->u4FwdBufC) >> 8;
    #endif
    #else //CONFIG_DRV_VERIFY_SUPPORT
    u4RegValue = (UINT32) (prParsingPic->u4FwdBufC) >> 8;
    #endif //CONFIG_DRV_VERIFY_SUPPORT
    vVDecWriteMC(u4VDecID, RW_MC_R1C, u4RegValue);
  }

  if (prParsingPic->ePtype == RM_TRUEBPIC)
  {
    #if (CONFIG_DRV_VERIFY_SUPPORT == 1)
    #if VMMU_SUPPORT
    {
    UINT32 u4Page_addr = 0;

    u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + ((((UINT32) prParsingPic->u4BwdBufY)/(4*1024))*4);
    printk("[RM] vVDEC_HAL_RM_TriggerDecode, 9, Page Addr = 0x%x\n", u4Page_addr);    
    vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32) prParsingPic->u4BwdBufY), PHYSICAL((UINT32) prParsingPic->u4BwdBufY) + PIC_Y_SZ);
    u4RegValue = (UINT32) (prParsingPic->u4BwdBufY) >> 9;
    }
    #else
    u4RegValue = (UINT32) PHYSICAL((UINT32) prParsingPic->u4BwdBufY) >> 9;
    #endif
    #else //CONFIG_DRV_VERIFY_SUPPORT
    u4RegValue = (UINT32) (prParsingPic->u4BwdBufY) >> 9;
    #endif //CONFIG_DRV_VERIFY_SUPPORT
    vVDecWriteMC(u4VDecID, RW_MC_R2Y, u4RegValue);

    #if (CONFIG_DRV_VERIFY_SUPPORT == 1)    
    #if VMMU_SUPPORT
    {
    UINT32 u4Page_addr = 0;

    u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + ((((UINT32) prParsingPic->u4BwdBufC)/(4*1024))*4);
    printk("[RM] vVDEC_HAL_RM_TriggerDecode, 10, Page Addr = 0x%x\n", u4Page_addr);    
    vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32) prParsingPic->u4BwdBufC), PHYSICAL((UINT32) prParsingPic->u4BwdBufC) + PIC_C_SZ);
    u4RegValue = (UINT32) (prParsingPic->u4BwdBufC) >> 8;
    }
    #else
    u4RegValue = (UINT32) PHYSICAL((UINT32) prParsingPic->u4BwdBufC) >> 8;
    #endif
    #else //CONFIG_DRV_VERIFY_SUPPORT
    u4RegValue = (UINT32) (prParsingPic->u4BwdBufC) >> 8;
    #endif //CONFIG_DRV_VERIFY_SUPPORT
    vVDecWriteMC(u4VDecID, RW_MC_R2C, u4RegValue);
  }
  
  #if VMMU_SUPPORT
  printk("[RM] vVDEC_HAL_RM_TriggerDecode, VMMUTable:[0x%x, 0x%x]\n", ((UINT32)_pucVMMUTable[u4VDecID]), PHYSICAL((UINT32)_pucVMMUTable[u4VDecID]));         
  vVDecVMMUEnable(PHYSICAL((UINT32)_pucVMMUTable[u4VDecID]));
  #endif

  u4RegValue = prParsingPic->u4Iratio;
  vVDecWriteMC(u4VDecID, RW_MC_IRATIO, u4RegValue);       // B-picture I ratio

  u4RegValue = prParsingPic->u4Width;
  vVDecWriteMC(u4VDecID, RW_MC_UMV_PIC_WIDTH, u4RegValue);

  u4RegValue = prParsingPic->u4Height;
  vVDecWriteMC(u4VDecID, RW_MC_UMV_PIC_HEIGHT, u4RegValue);

  //Turn-on mc_new_bandwidth mode
  #ifdef RM_DISABLE_NBM
  u4RegValue= u4VDecReadMC(u4VDecID, 474*4);
  u4RegValue = u4RegValue | (1 << 8);
  
  vVDecWriteMC(u4VDecID, 474*4,  u4RegValue);
  #endif //RM_DISABLE_NBM

  u4RegValue = 0x0000ffff;
  vVDecWriteMC(u4VDecID, 460*4, u4RegValue);    //????????

  u4RegValue = (prParsingPic->u4Height << 16) | 
  	                (prParsingPic->u4Width / 16) | 
  	                0xFF00;
  vVDecWriteVLD(u4VDecID, RW_VLD_PICSZ, u4RegValue);

  if (prParsingPic->fgEnableDDR3)
  {
    #ifdef RM_DDR3MODE_DEBUG_ENABLE
    u4RegValue = prParsingPic->u4Width / 16;
    #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
       vVDecWriteMC(u4VDecID, RW_MC_PIC_W_MB, u4RegValue);
    #else
    vVDecWriteVLD(u4VDecID, RW_VLD_PIC_W_MB, u4RegValue);
    #endif
    #else //RM_DDR3MODE_DEBUG_ENABLE
    u4RegValue = prParsingPic->u4Width/ 16;
    if (u4RegValue % 4)
    {
      u4RegValue = u4RegValue + (4 - (u4RegValue % 4));
    }
    
    #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
       vVDecWriteMC(u4VDecID, RW_MC_PIC_W_MB, u4RegValue);
    #else
    vVDecWriteVLD(u4VDecID, RW_VLD_PIC_W_MB, u4RegValue);
    #endif
    #endif //RM_DDR3MODE_DEBUG_ENABLE
  }
  else
  {
    u4RegValue = prParsingPic->u4Width / 16;
    #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
       vVDecWriteMC(u4VDecID, RW_MC_PIC_W_MB, u4RegValue);
    #else
    vVDecWriteVLD(u4VDecID, RW_VLD_PIC_W_MB, u4RegValue);
    #endif
  }

  u4RegValue = 0xffffffff;
  vVDecWriteVLD(u4VDecID, 243*4, u4RegValue);    //????????

  u4RegValue = 0xff000000;
  vVDecWriteVLD(u4VDecID, 244*4, u4RegValue);    //????????

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
 vVDecWriteVLDTOP(u4VDecID, RW_RM_TOPVLD_TIMEOUT_INT_SWITCH, u4CycleCnt);
 u4RegValue =  u4ErrConceal | u4FastParsing | 0x3 | (prParsingPic->fgRV9 ? 4 : 0);
 vVDecWriteRMVLD(u4VDecID, RW_RMVLD_CTRL, u4RegValue);  // RV8 or RV9
#else
  u4RegValue = u4CycleCnt | u4ErrConceal | u4FastParsing | 0x3 | (prParsingPic->fgRV9 ? 4 : 0);
  vVDecWriteRMVLD(u4VDecID, RW_RMVLD_CTRL, u4RegValue);  // RV8 or RV9
#endif

  #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
  u4RegValue =  prParsingPic->u4OrgHeight << 16 | prParsingPic->u4OrgWidth;
  vVDecWriteVLDTOP(u4VDecID, RW_RM_TOPVLD_PICSIZE1, u4RegValue);
  #else
  u4RegValue = (prParsingPic->u4Width / 16 - 1) << 24 | prParsingPic->u4OrgHeight << 12 | prParsingPic->u4OrgWidth;
  vVDecWriteRMVLD(u4VDecID, RW_RMVLD_PICSIZE1, u4RegValue);
  #endif

  #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
  u4RegValue =  (prParsingPic->u4Height / 16 - 1)<< 16 | 
  	                 (prParsingPic->u4Width / 16 - 1) ;
  vVDecWriteVLDTOP(u4VDecID, RW_RM_TOPVLD_PICSIZE2, u4RegValue);  
  u4RegValue = (prParsingPic->u4Width / 16) * (prParsingPic->u4Height / 16);
  vVDecWriteVLDTOP(u4VDecID, RW_RM_TOPVLD_PICSIZE3, u4RegValue); 
  u4RegValue = prParsingPic->u4MbaSize << 24 |  prParsingPic->u4PctszSize << 22; 
  vVDecWriteRMVLD(u4VDecID, RW_RMVLD_PICSIZE2, u4RegValue); 
  #else
  u4RegValue = prParsingPic->u4MbaSize << 24 | 
  	                prParsingPic->u4PctszSize << 22 | 
  	                ((prParsingPic->u4Width / 16) * (prParsingPic->u4Height / 16)) << 8 | 
  	                (prParsingPic->u4Height / 16 - 1);
  vVDecWriteRMVLD(u4VDecID, RW_RMVLD_PICSIZE2, u4RegValue);  //  0xbc39816
  #endif

  u4RegValue = (UINT32) (((UINT8)(prParsingPic->fgRType) << 26) | 
  //u4RegValue = prParsingPic->u4Rtype << 26 | 
  	                prParsingPic->u4PicSz << 23 |
  	                //prParsingPic->u4Pctsz << 23 | 
  	                prParsingPic->u4RvTr << 10 | 
  	                prParsingPic->fgDeblockPassThru << 9 | 
  	                //prParsingPic->u4Tr << 10 | 
  	                //prParsingPic->u4DFP << 9 | 
  	                prParsingPic->u4OsvQuant << 7 | 
  	                prParsingPic->u4PSQuant << 2 |
  	                //prParsingPic->u4Oquant << 7 | | 
  	                //prParsingPic->u4Pquant << 2 | 
  	                (UINT32)prParsingPic->ePtype);
  vVDecWriteRMVLD(u4VDecID, RW_RMVLD_PICHDRPARA1, u4RegValue);  // 0x4000044

  u4RegValue = prParsingPic->u4Mba;
  vVDecWriteRMVLD(u4VDecID, RW_RMVLD_PICHDRPARA2, u4RegValue);

  u4RegValue = prParsingPic->u4NumSlice;
  vVDecWriteRMVLD(u4VDecID, RW_RMVLD_SLICENUM, u4RegValue);
  
  for (i = 0; i < prParsingPic->u4NumSlice; i++)
  {
    u4RegValue = prParsingPic->au4SliceSize[i];
    vVDecWriteRMVLD(u4VDecID, (RW_RMVLD_SLICEBCNT + (i*4)), u4RegValue);
  }
  
  #ifdef RM_FPGA_LIMITATION
  if (prParsingPic->u4OrgWidth >= 720)
  {
    u4RegValue = u4VDecReadMC(u4VDecID, RW_MC_WRAPPER_SWITCH);
    u4RegValue |= (1<<0);  
    vVDecWriteMC(u4VDecID, RW_MC_WRAPPER_SWITCH, u4RegValue);
    
    #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    u4RegValue = u4VDecReadVLDTOP(u4VDecID, RW_RM_TOPVLD_PREDADDR_MODE);
    u4RegValue |= 0x1;   
    vVDecWriteVLDTOP(u4VDecID, RW_RM_TOPVLD_PREDADDR_MODE, u4RegValue);    
    #else
    u4RegValue = u4VDecReadRMVLD(u4VDecID, RW_RMVLD_PREDADDR);
    u4RegValue |= (1<<30);   
    vVDecWriteRMVLD(u4VDecID, RW_RMVLD_PREDADDR, u4RegValue);
    #endif
  }
  
  #ifdef RM_NEW_PPOUT_MODE
  if (prParsingPic->u4OrgWidth < 720)
  {
    u4RegValue= u4VDecReadMC(u4VDecID, 525*4);

    u4RegValue = u4RegValue & 0xFFFFFFFE;
    	
    vVDecWriteMC(u4VDecID, 525*4, u4RegValue);
  }
  #endif //RM_NEW_PPOUT_MODE
  #else //RM_FPGA_LIMITATION
  if (prParsingPic->fgEnablePPOut)
  {
    u4RegValue= u4VDecReadMC(u4VDecID, 525*4);

    u4RegValue = u4RegValue & 0xFFFFFFFE;
    	
    vVDecWriteMC(u4VDecID, 525*4, u4RegValue);
  }
  #endif //RM_FPGA_LIMITATION


  if (prParsingPic->fgEnableAddrSwap)
  {
    vVDecWriteMC(u4VDecID, RW_MC_ADDRSWAP, prParsingPic->u4AddrSwapMode);
  
    u4RegValue= u4VDecReadMC(u4VDecID, 474*4);
    u4RegValue = u4RegValue & 0xFFFFFFF8;
    u4RegValue = u4RegValue | prParsingPic->u4AddrSwapMode;
    vVDecWriteMC(u4VDecID, 474*4,  u4RegValue);
  }

  if (prParsingPic->fgEnableDDR3)
  {
    u4RegValue= u4VDecReadMC(u4VDecID, 511*4);
    u4RegValue = (u4RegValue | 0x00000001);
    vVDecWriteMC(u4VDecID, 511*4, u4RegValue);
  
    u4RegValue= u4VDecReadMC(u4VDecID, 512*4);
    u4RegValue = (u4RegValue | 0x00000001);
    vVDecWriteMC(u4VDecID, 512*4, u4RegValue);
  } 

  u4RegValue = u4VDecReadRMVLD(u4VDecID, RW_RMVLD_CTRL2);
  u4RegValue |= 0x3;
  vVDecWriteRMVLD(u4VDecID, RW_RMVLD_CTRL2, u4RegValue);
  
  #if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8560)
   u4RegValue= u4VDecReadMC(u4VDecID, RW_MC_DDR3_EN);
   u4RegValue = (u4RegValue | 0x00000010);
   vVDecWriteMC(u4VDecID, RW_MC_DDR3_EN, u4RegValue);
  #endif
  
  // trigger 
  u4RegValue = 0x1;
  vVDecWriteRMVLD(u4VDecID, RW_RMVLD_STARTDEC, u4RegValue);
}
#if 0
void vVDEC_HAL_RM_TriggerDecode(UINT32 u4VDecID, VDEC_INFO_RM_PICINFO_T *prParsingPic)
{
  UINT32 u4RegValue;
  UINT32 u4Rv8TblNum  =1;
  UINT32 i;

  UINT32 u4CycleCnt = 0x00800000;
  UINT32 u4ErrConceal = 0;
  UINT32 u4FastParsing = 0x40000000;  

  prParsingPic->u4InitInputWindow = u4VDecReadRMVLD(u4VDecID, 0x0);
 
  #ifndef RM_DISABLE_HWCRCCHECK
  #ifdef RM_CRCCHECKFLOW_SUPPORT
  #ifdef RM_CRCCHECK_ENABLE
  //Enable CRC HW
  vVDecWriteRMCRC(u4VDecID, 0x4, 0x3);
  #endif //RM_CRCCHECK_ENABLE
  #endif //RM_CRCCHECKFLOW_SUPPORT
  #endif //RM_DISABLE_HWCRCCHECK

  u4RegValue = (prParsingPic->u4Iratio & 0x0000FFFF);
  #ifdef VDEC_RM_HAL_ENABLELOG
  printk("[RM] RW_RMMV_IRATIO0 = %x \n", u4RegValue);
  #endif //VDEC_RM_HAL_ENABLELOG
  vVDecWriteRMMV(u4VDecID, RW_RMMV_IRATIO0, u4RegValue);
    
  if (prParsingPic->ePtype == RM_TRUEBPIC)
  {
    u4RegValue = prParsingPic->fgBwdIsI;
    #ifdef VDEC_RM_HAL_ENABLELOG
    printk("[RM] RW_RMMV_CPLOCPICTYPE = %x \n", u4RegValue);
    #endif //VDEC_RM_HAL_ENABLELOG
    vVDecWriteRMMV(u4VDecID, RW_RMMV_CPLOCPICTYPE, u4RegValue);
  }

  #ifdef RM_MCOutPut_ENABLE
  u4RegValue= u4VDecReadMC(u4VDecID, RW_MC_OPBUF);
  u4RegValue = u4RegValue & 0xFFFFFFF8;
  u4RegValue = u4RegValue | (4<<0);   	
  
  vVDecWriteMC(u4VDecID, RW_MC_OPBUF, u4RegValue);
  #endif //RM_MCOutPut_ENABLE

  #ifdef RM_MCOutPut_ENABLE
  //MC Luma Addr
  u4RegValue = (UINT32) (PHYSICAL(prParsingPic->u4MCOutBufY)>>9);
  vVDecWriteMC(u4VDecID, RW_MC_BY1, u4RegValue);
    
  //MC Chroma Addr
  u4RegValue = (UINT32) (PHYSICAL(prParsingPic->u4MCOutBufC)>>8);
  vVDecWriteMC(u4VDecID, RW_MC_BC1, u4RegValue);
  #else //RM_MCOutPut_ENABLE
  //MC Luma Addr
  u4RegValue = (UINT32) PHYSICAL((UINT32) prParsingPic->u4OutBufY)>>9;
  #ifdef VDEC_RM_HAL_ENABLELOG
  printk("[RM] RW_MC_BY1 = %x \n", u4RegValue);
  #endif //VDEC_RM_HAL_ENABLELOG
  vVDecWriteMC(u4VDecID, RW_MC_BY1, u4RegValue);
  
  //MC Chroma Addr
  u4RegValue = (UINT32) PHYSICAL((UINT32) prParsingPic->u4OutBufC)>>8;
  #ifdef VDEC_RM_HAL_ENABLELOG
  printk("[RM] RW_MC_BC1 = %x \n", u4RegValue);
  #endif //VDEC_RM_HAL_ENABLELOG
  vVDecWriteMC(u4VDecID, RW_MC_BC1, u4RegValue);
  #endif //RM_MCOutPut_ENABLE

  u4RegValue = (UINT32) PHYSICAL((UINT32) prParsingPic->u4OutBufY)>>9;
  vVDecWriteMC(u4VDecID, RW_MC_PP_Y_ADDR, u4RegValue);
  
  u4RegValue = (UINT32) PHYSICAL((UINT32) prParsingPic->u4OutBufC)>>8;
  vVDecWriteMC(u4VDecID, RW_MC_PP_C_ADDR, u4RegValue);

  u4RegValue = 0x1;
  vVDecWriteMC(u4VDecID, RW_MC_PP_ENABLE, u4RegValue);    //Enable MC PP
  
  #ifdef RM_DDR3MODE_ENABLE
  #ifdef RM_DDR3MODE_DEBUG_ENABLE
  u4RegValue = (prParsingPic->u4Width/16);
  vVDecWriteMC(u4VDecID, RW_MC_PP_MB_WIDTH, u4RegValue);
  #else //RM_DDR3MODE_DEBUG_ENABLE
  u4RegValue = prParsingPic->u4Width/ 16;
  if (u4RegValue % 4)
  {
    u4RegValue = u4RegValue + (4 - (u4RegValue % 4));
  }
  
  vVDecWriteMC(u4VDecID, RW_MC_PP_MB_WIDTH, u4RegValue);
  #endif //RM_DDR3MODE_DEBUG_ENABLE
  #else //RM_DDR3MODE_ENABLE
  u4RegValue = (prParsingPic->u4Width/16);
  vVDecWriteMC(u4VDecID, RW_MC_PP_MB_WIDTH, u4RegValue);
  #endif //RM_DDR3MODE_ENABLE

  u4RegValue = (prParsingPic->fgDeblockPassThru == 0 ? 
  //u4RegValue = (prParsingPic->u4DFP == 0 ? 
  	                  ((!prParsingPic->fgRV9 && 
  	                    (prParsingPic->ePtype != RM_INTRAPIC && prParsingPic->ePtype != RM_FORCED_INTRAPIC)) ? 2 : 3) : 0);
  vVDecWriteMC(u4VDecID, RW_MC_PP_DBLK_MODE, u4RegValue);

  #ifdef RM_MCOutPut_ENABLE
  u4RegValue= u4VDecReadMC(u4VDecID, RW_MC_PP_WB_BY_POST);
  u4RegValue = u4RegValue & 0xFFFFFFFE;
  
  vVDecWriteMC(u4VDecID, RW_MC_PP_WB_BY_POST, u4RegValue);
  #else //RM_MCOutPut_ENABLE
  u4RegValue = 0x1;
  vVDecWriteMC(u4VDecID, RW_MC_PP_WB_BY_POST, u4RegValue);    //?????
  #endif //RM_MCOutPut_ENABLE

  u4RegValue = (prParsingPic->u4Width/16)-1;
  vVDecWriteMC(u4VDecID, RW_MC_PP_X_RANGE, u4RegValue);

  u4RegValue = (prParsingPic->u4Height/16)-1;
  vVDecWriteMC(u4VDecID, RW_MC_PP_Y_RANGE, u4RegValue);
  
  if (prParsingPic->ePtype == RM_TRUEBPIC)
  {
    u4RegValue = prParsingPic->u4RefQpMb0 << 16;
  }
  else
  {
    u4RegValue = prParsingPic->u4PSQuant << 16;
    //u4RegValue = prParsingPic->u4Pquant << 16;
  }
  
  u4RegValue |= (prParsingPic->ePtype == RM_TRUEBPIC ? 1 : 0) << 11;
  u4RegValue |= (prParsingPic->ePtype == RM_TRUEBPIC ? 1 : 0) << 10;
  u4RegValue |= prParsingPic->fgDeblockPassThru << 9;
  //u4RegValue |= prParsingPic->u4DFP << 9;
  u4RegValue |= (prParsingPic->u4Width * prParsingPic->u4Height <= 176 * 144 ? 1 : 0) << 8;
  u4RegValue |= u4Rv8TblNum << 4;
  u4RegValue |= (prParsingPic->fgRV9 ? 1 : 0) << 1;
  u4RegValue |= (prParsingPic->fgRV9 ? 0 : 1);
  vVDecWriteRMPP(u4VDecID, RW_RMPP_PARAM, u4RegValue);

  // MC setting
  if (prParsingPic->ePtype == RM_INTERPIC || prParsingPic->ePtype == RM_TRUEBPIC)
  {
    u4RegValue = (UINT32) PHYSICAL((UINT32) prParsingPic->u4FwdBufY) >> 9;
    vVDecWriteMC(u4VDecID, RW_MC_R1Y, u4RegValue);

    u4RegValue = (UINT32) PHYSICAL((UINT32) prParsingPic->u4FwdBufC) >> 8;
    vVDecWriteMC(u4VDecID, RW_MC_R1C, u4RegValue);
  }

  if (prParsingPic->ePtype == RM_TRUEBPIC)
  {
    u4RegValue = (UINT32) PHYSICAL((UINT32) prParsingPic->u4BwdBufY) >> 9;
    vVDecWriteMC(u4VDecID, RW_MC_R2Y, u4RegValue);

    u4RegValue = (UINT32) PHYSICAL((UINT32) prParsingPic->u4BwdBufC) >> 8;
    vVDecWriteMC(u4VDecID, RW_MC_R2C, u4RegValue);
  }

  u4RegValue = prParsingPic->u4Iratio;
  vVDecWriteMC(u4VDecID, RW_MC_IRATIO, u4RegValue);       // B-picture I ratio

  u4RegValue = prParsingPic->u4Width;
  vVDecWriteMC(u4VDecID, RW_MC_UMV_PIC_WIDTH, u4RegValue);

  u4RegValue = prParsingPic->u4Height;
  vVDecWriteMC(u4VDecID, RW_MC_UMV_PIC_HEIGHT, u4RegValue);

  //Turn-on mc_new_bandwidth mode
  #ifdef RM_DISABLE_NBM
  u4RegValue= u4VDecReadMC(u4VDecID, 474*4);
  u4RegValue = u4RegValue | (1 << 8);
  
  vVDecWriteMC(u4VDecID, 474*4,  u4RegValue);
  #endif //RM_DISABLE_NBM

  u4RegValue = 0x0000ffff;
  vVDecWriteMC(u4VDecID, 460*4, u4RegValue);    //????????

  u4RegValue = (prParsingPic->u4Height << 16) | 
  	                (prParsingPic->u4Width / 16) | 
  	                0xFF00;
  vVDecWriteVLD(u4VDecID, RW_VLD_PICSZ, u4RegValue);

  #ifdef RM_DDR3MODE_ENABLE
  #ifdef RM_DDR3MODE_DEBUG_ENABLE
  u4RegValue = prParsingPic->u4Width / 16;
  vVDecWriteVLD(u4VDecID, RW_VLD_PIC_W_MB, u4RegValue);
  #else //RM_DDR3MODE_DEBUG_ENABLE
  u4RegValue = prParsingPic->u4Width/ 16;
  if (u4RegValue % 4)
  {
    u4RegValue = u4RegValue + (4 - (u4RegValue % 4));
  }
  
  vVDecWriteVLD(u4VDecID, RW_VLD_PIC_W_MB, u4RegValue);
  #endif //RM_DDR3MODE_DEBUG_ENABLE
  #else //RM_DDR3MODE_ENABLE
  u4RegValue = prParsingPic->u4Width / 16;
  vVDecWriteVLD(u4VDecID, RW_VLD_PIC_W_MB, u4RegValue);
  #endif //RM_DDR3MODE_ENABLE

  u4RegValue = 0xffffffff;
  vVDecWriteVLD(u4VDecID, 243*4, u4RegValue);    //????????

  u4RegValue = 0xff000000;
  vVDecWriteVLD(u4VDecID, 244*4, u4RegValue);    //????????

  u4RegValue = u4CycleCnt | u4ErrConceal | u4FastParsing | 0x3 | (prParsingPic->fgRV9 ? 4 : 0);
  vVDecWriteRMVLD(u4VDecID, RW_RMVLD_CTRL, u4RegValue);  // RV8 or RV9

  u4RegValue = (prParsingPic->u4Width / 16 - 1) << 24 | prParsingPic->u4OrgHeight << 12 | prParsingPic->u4OrgWidth;
  vVDecWriteRMVLD(u4VDecID, RW_RMVLD_PICSIZE1, u4RegValue);

  u4RegValue = prParsingPic->u4MbaSize << 24 | 
  	                prParsingPic->u4PctszSize << 22 | 
  	                ((prParsingPic->u4Width / 16) * (prParsingPic->u4Height / 16)) << 8 | 
  	                (prParsingPic->u4Height / 16 - 1);
  vVDecWriteRMVLD(u4VDecID, RW_RMVLD_PICSIZE2, u4RegValue);  //  0xbc39816

  u4RegValue = prParsingPic->u4Rtype << 26 | 
  	                prParsingPic->u4PicSz << 23 |
  	                //prParsingPic->u4Pctsz << 23 | 
  	                prParsingPic->u4RvTr << 10 | 
  	                prParsingPic->fgDeblockPassThru << 9 | 
  	                //prParsingPic->u4Tr << 10 | 
  	                //prParsingPic->u4DFP << 9 | 
  	                prParsingPic->u4OsvQuant << 7 | 
  	                prParsingPic->u4PSQuant << 2 |
  	                //prParsingPic->u4Oquant << 7 | | 
  	                //prParsingPic->u4Pquant << 2 | 
  	                (UINT32)prParsingPic->ePtype;
  vVDecWriteRMVLD(u4VDecID, RW_RMVLD_PICHDRPARA1, u4RegValue);  // 0x4000044

  u4RegValue = prParsingPic->u4Mba;
  vVDecWriteRMVLD(u4VDecID, RW_RMVLD_PICHDRPARA2, u4RegValue);

  u4RegValue = prParsingPic->u4NumSlice;
  vVDecWriteRMVLD(u4VDecID, RW_RMVLD_SLICENUM, u4RegValue);
  
  for (i = 0; i < prParsingPic->u4NumSlice; i++)
  {
    u4RegValue = prParsingPic->au4SliceSize[i];
    vVDecWriteRMVLD(u4VDecID, (RW_RMVLD_SLICEBCNT + (i*4)), u4RegValue);
  }
  
  #ifdef RM_FPGA_LIMITATION
  if (prParsingPic->u4OrgWidth >= 720)
  {
    u4RegValue = u4VDecReadMC(u4VDecID, RW_MC_WRAPPER_SWITCH);
    u4RegValue |= (1<<0);  
    vVDecWriteMC(u4VDecID, RW_MC_WRAPPER_SWITCH, u4RegValue);
    
    u4RegValue = u4VDecReadRMVLD(u4VDecID, RW_RMVLD_PREDADDR);
    u4RegValue |= (1<<30);   
    vVDecWriteRMVLD(u4VDecID, RW_RMVLD_PREDADDR, u4RegValue);
  }
  
  #ifdef RM_NEW_PPOUT_MODE
  if (prParsingPic->u4OrgWidth < 720)
  {
    u4RegValue= u4VDecReadMC(u4VDecID, 525*4);

    u4RegValue = u4RegValue & 0xFFFFFFFE;
    	
    vVDecWriteMC(u4VDecID, 525*4, u4RegValue);
  }
  #endif //RM_NEW_PPOUT_MODE
  #else //RM_FPGA_LIMITATION
  #ifdef RM_NEW_PPOUT_MODE
  {
    u4RegValue= u4VDecReadMC(u4VDecID, 525*4);

    u4RegValue = u4RegValue & 0xFFFFFFFE;
    	
    vVDecWriteMC(u4VDecID, 525*4, u4RegValue);
  }
  #endif //RM_NEW_PPOUT_MODE
  #endif //RM_FPGA_LIMITATION


  #ifdef RM_ADDRSWAP_ENABLE
  vVDecWriteMC(u4VDecID, RW_MC_ADDRSWAP, prParsingPic->u4AddrSwapMode);

  u4RegValue= u4VDecReadMC(u4VDecID, 474*4);
  u4RegValue = u4RegValue & 0xFFFFFFF8;
  u4RegValue = u4RegValue | prParsingPic->u4AddrSwapMode;
  vVDecWriteMC(u4VDecID, 474*4,  u4RegValue);
  #endif //RM_ADDRSWAP_ENABLE

  #ifdef RM_DDR3MODE_ENABLE
  u4RegValue= u4VDecReadMC(u4VDecID, 511*4);
  u4RegValue = (u4RegValue | 0x00000001);
  vVDecWriteMC(u4VDecID, 511*4, u4RegValue);

  u4RegValue= u4VDecReadMC(u4VDecID, 512*4);
  u4RegValue = (u4RegValue | 0x00000001);
  vVDecWriteMC(u4VDecID, 512*4, u4RegValue);
  #endif //RM_DDR3MODE_ENABLE 

  u4RegValue = u4VDecReadRMVLD(u4VDecID, RW_RMVLD_CTRL2);
  u4RegValue |= 0x3;
  vVDecWriteRMVLD(u4VDecID, RW_RMVLD_CTRL2, u4RegValue);
  
  // trigger 
  u4RegValue = 0x1;
  vVDecWriteRMVLD(u4VDecID, RW_RMVLD_STARTDEC, u4RegValue);
}
#endif //0

UINT32 u4VDEC_HAL_RM_VDec_ReadFinishFlag(UINT32 u4VDecID)
{
  UINT32 u4RegValue = 0;
  UINT32 u4RegValue1 = 0;

  u4RegValue = u4VDecReadRMVLD(u4VDecID, RW_RMVLD_VLDOK);    //VLD Finish
  u4RegValue1 = u4VDecReadRMVLD(u4VDecID, RW_RMVLD_PICFINISH);    //Picture Finish

  if (u4RegValue && u4RegValue1)
  {
    return 1;
  }
  else
  {
    return 0;
  }
} 


void vVDEC_HAL_RM_GetMbxMby(UINT32 u4VDecID, UINT32 *pu4Mbx, UINT32 *pu4Mby)
{
    *pu4Mbx = u4VDecReadMC(u4VDecID, RO_MC_MBX);
    *pu4Mby = u4VDecReadMC(u4VDecID, RO_MC_MBY);
}


void vVDEC_HAL_RM_GetRefQpMb0(UINT32 u4VDecID, UINT32 *pu4RefQpMb0)
{
  *pu4RefQpMb0 = u4VDecReadRMPP(u4VDecID, RW_RMPP_PQ);
}


void vVDEC_HAL_RM_GetErrInfo(UINT32 u4VDecID, VDEC_INFO_RM_ERR_INFO_T *prRMErrInfo)
{
    UINT32 u4RegVal;

    //Check 0xC0 and 0xC4
    //0xC0   =>   Error Type, For one slice only, if slice0 error, slice1 ok => 0xC0 error type will reset
    //0xC4   =>   Error Count for one picture
    u4RegVal = u4VDecReadRMVLD(u4VDecID, RW_RMVLD_ERRTYPE);
    prRMErrInfo->u4RMErrType = u4RegVal;
    
    u4RegVal = u4VDecReadRMVLD(u4VDecID, RW_RMVLD_ERRTYPEACCUM);
    prRMErrInfo->u4RMErrCnt = u4RegVal;
}

UINT32 u4VDEC_HAL_RM_ReadRdPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4VFIFOSa, UINT32 *pu4Bits)
{
    return u4VDecReadVldRPtr(u4BSID, u4VDecID, pu4Bits, PHYSICAL((UINT32) u4VFIFOSa));
}



