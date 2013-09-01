
#include "x_hal_1176.h"
#include "chip_ver.h" 
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8550) 
#include "x_bim_8550.h"  
#elif (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8555) 
#include "x_bim_8555.h"  
#endif
 
#include "vdec_hw_common.h"
#include "vdec_hal_if_rm.h"
#include "vdec_hw_rm.h"
#include "vdec_hal_errcode.h"
#include "drv_fs.h"

#include "slt_if.h"



#if CONFIG_DRV_VERIFY_SUPPORT
#include "x_printf.h"
#include "x_debug.h"

#include "vdec_verify_general.h"
#include "vdec_verify_mpv_prov.h"
#include "vdec_verify_vparser_rm.h"
#endif

#if (SLT_TEST_RM_EN)
#define RM_HAL_SUPPORT_SLT
#endif //SLT_TEST_RM_EN

#ifdef RM_HAL_SUPPORT_SLT
//#define RM_SLT_USE_SEMIHOSTING

#define RM_SLT_MVHWBUF_SZ    0x1FE00
#define RM_SLT_VLDPRED_SZ      0xC000
#define RM_SLT_GOLD_Y_SZ  0x400000
#define RM_SLT_GOLD_C_SZ  0x1FE000

char strSLTBaseFolder[] = "F:\\RM_SLT\\TCH_ff8000\\";

BOOL fgDebugLogEnable = FALSE;

VDEC_INFO_RM_PICINFO_T *prPic;   
VDEC_INFO_RM_PICINFO_T *prParsingPic;   

BYTE _arbSrcFilePath[4][128] = {
    "/mnt/usb/aulinfo.bin",
    "/mnt/usb/g0000000.yuv",
    "/mnt/usb/g0000001.yuv",
    "/mnt/usb/g0000002.yuv"
};

void vRM_SLT_DumpVldCkSum(void)
{
  printk("RV VLD 33: 0x%08X\n", u4ReadReg(RM_VLD_REG_OFFSET0 + 33 * 4));
  printk("RV VLD 34: 0x%08X\n", u4ReadReg(RM_VLD_REG_OFFSET0 + 34 * 4));
  printk("RV VLD 35: 0x%08X\n", u4ReadReg(RM_VLD_REG_OFFSET0 + 35 * 4));
  printk("RV VLD 36: 0x%08X\n", u4ReadReg(RM_VLD_REG_OFFSET0 + 36 * 4));
  printk("RV VLD 37: 0x%08X\n", u4ReadReg(RM_VLD_REG_OFFSET0 + 37 * 4));
  printk("RV VLD 43: 0x%08X\n", u4ReadReg(RM_VLD_REG_OFFSET0 + 43 * 4));
  printk("RV VLD 44: 0x%08X\n", u4ReadReg(RM_VLD_REG_OFFSET0 + 44 * 4));
  printk("RV VLD 48: 0x%08X\n", u4ReadReg(RM_VLD_REG_OFFSET0 + 48 * 4));
  printk("RV VLD 59: 0x%08X\n", u4ReadReg(RM_VLD_REG_OFFSET0 + 59 * 4));
  printk("RV VLD 60: 0x%08X\n", u4ReadReg(RM_VLD_REG_OFFSET0 + 60 * 4));
  printk("RV VLD 67: 0x%08X\n", u4ReadReg(RM_VLD_REG_OFFSET0 + 67 * 4));
  printk("RV VLD 63: 0x%08X\n", u4ReadReg(RM_VLD_REG_OFFSET0 + 63 * 4));
  printk("RV VLD 64: 0x%08X\n", u4ReadReg(RM_VLD_REG_OFFSET0 + 64 * 4));
  printk("RV VLD 65: 0x%08X\n", u4ReadReg(RM_VLD_REG_OFFSET0 + 65 * 4));
  printk("RV VLD 196: 0x%08X\n", u4ReadReg(RM_VLD_REG_OFFSET0 + 196 * 4));

  printk("RV VLD 53: 0x%08X\n", u4ReadReg(RM_VLD_REG_OFFSET0 + 53 * 4));
  printk("RV VLD 62: 0x%08X\n", u4ReadReg(RM_VLD_REG_OFFSET0 + 62 * 4));
  printk("RV VLD 196: 0x%08X\n", u4ReadReg(RM_VLD_REG_OFFSET0 + 196 * 4));
}

void vRM_SLT_DumpAvcMvCkSum(void)
{
  printk("AVC MV 148: 0x%08X\n", u4ReadReg(AVC_MV_REG_OFFSET0 + 148 * 4));
  printk("AVC MV 149: 0x%08X\n", u4ReadReg(AVC_MV_REG_OFFSET0 + 149 * 4));
  printk("AVC MV 150: 0x%08X\n", u4ReadReg(AVC_MV_REG_OFFSET0 + 150 * 4));
  printk("AVC MV 151: 0x%08X\n", u4ReadReg(AVC_MV_REG_OFFSET0 + 151 * 4));
}


void vRM_SLT_DumpMCPerformance(void)
{
  printk("MC 474: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 474 * 4));
  printk("MC 476: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 476 * 4));
  printk("MC 477: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 477 * 4));
  printk("MC 478: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 478 * 4));
  printk("MC 522: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 522 * 4));
}


void vRM_SLT_DumpMcCkSum(void)
{
  printk("MC 378: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 378 * 4));
  printk("MC 379: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 379 * 4));
  printk("MC 380: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 380 * 4));
  printk("MC 381: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 381 * 4));
  printk("MC 382: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 382 * 4));
  printk("MC 388: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 388 * 4));
  printk("MC 390: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 390 * 4));
  printk("MC 391: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 391 * 4));
  printk("MC 392: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 392 * 4));
  printk("MC 393: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 393 * 4));
  printk("MC 446: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 446 * 4));
  printk("MC 447: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 447 * 4));
}

void vRM_SLT_DumpPpCkSum(void)
{
  int i;
  
  printk("PP   5: 0x%08X\n", u4ReadReg(RM_VDEC_PP_BASE +   5 * 4));
  printk("PP   6: 0x%08X\n", u4ReadReg(RM_VDEC_PP_BASE +   6 * 4));
  printk("PP  67: 0x%08X\n", u4ReadReg(RM_VDEC_PP_BASE +  67 * 4));

  for (i = 69; i <= 76; i++)
    printk("PP %3d: 0x%08X\n", i, u4ReadReg(RM_VDEC_PP_BASE + i * 4));
    
  for (i = 161; i <= 178; i++)
    printk("PP %3d: 0x%08X\n", i, u4ReadReg(RM_VDEC_PP_BASE + i * 4));

  printk("PP  67: 0x%08X\n", u4ReadReg(RM_VDEC_PP_BASE +  67 * 4));
  printk("PP 199: 0x%08X\n", u4ReadReg(RM_VDEC_PP_BASE + 199 * 4));
  printk("PP 200: 0x%08X\n", u4ReadReg(RM_VDEC_PP_BASE + 200 * 4));
  printk("PP 201: 0x%08X\n", u4ReadReg(RM_VDEC_PP_BASE + 201 * 4));
  printk("PP 227: 0x%08X\n", u4ReadReg(RM_VDEC_PP_BASE + 227 * 4));
}

void vRM_SLT_DumpReg(VDEC_INFO_RM_PICINFO_T *prParsingPic)
{
  printk("AVC_MV 131: 0x%08X\n", u4ReadReg(AVC_MV_REG_OFFSET0 + 131 * 4));
  printk("AVC_MV 134: 0x%08X\n", u4ReadReg(AVC_MV_REG_OFFSET0 + 134 * 4));
  printk("AVC_MV 135: 0x%08X\n", u4ReadReg(AVC_MV_REG_OFFSET0 + 135 * 4));
  
  printk("MC 136: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 136 * 4));
  printk("MC 137: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 137 * 4));
  printk("MC 138: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 138 * 4));
  printk("MC 139: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 139 * 4));
  printk("MC 142: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 142 * 4));
  printk("MC 148: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 148 * 4));
  printk("MC 152: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 152 * 4));
  printk("MC 153: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 153 * 4));
  printk("PP 2: 0x%08X\n", u4ReadReg(RM_VDEC_PP_BASE + 2 * 4));

  printk("MC 420: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 420 * 4));
  printk("MC 130: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 130 * 4));
  printk("MC 131: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 131 * 4));
  printk("VLD 36: 0x%08X\n", u4ReadReg(VLD_REG_OFFSET0 + 36 * 4));
  printk("VLD 50: 0x%08X\n", u4ReadReg(VLD_REG_OFFSET0 + 50 * 4));
  printk("MC 0: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 0 * 4));
  printk("MC 1: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 1 * 4));
  printk("MC 2: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 2 * 4));
  printk("MC 3: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 3 * 4));
  printk("MC 39: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 39 * 4));
  printk("MC 40: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 40 * 4));
  printk("MC 9: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 9 * 4));
  printk("MC 28: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 28 * 4));
  printk("MC 36: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 36 * 4));
  printk("MC 6: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 6 * 4));
  printk("MC 38: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 38 * 4));
  printk("MC 114: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 114 * 4));

  printk("RV VLD 39: 0x%08X\n", u4ReadReg(RM_VLD_REG_OFFSET0 + 39 * 4));
  printk("RV VLD 66: 0x%08X\n", u4ReadReg(RM_VLD_REG_OFFSET0 + 66 * 4));
  printk("RV VLD 33: 0x%08X\n", u4ReadReg(RM_VLD_REG_OFFSET0 + 33 * 4));
  printk("RV VLD 34: 0x%08X\n", u4ReadReg(RM_VLD_REG_OFFSET0 + 34 * 4));
  printk("RV VLD 35: 0x%08X\n", u4ReadReg(RM_VLD_REG_OFFSET0 + 35 * 4));
  printk("RV VLD 36: 0x%08X\n", u4ReadReg(RM_VLD_REG_OFFSET0 + 36 * 4));
  printk("RV VLD 37: 0x%08X\n", u4ReadReg(RM_VLD_REG_OFFSET0 + 37 * 4));

  printk("RV VLD 42: 0x%08X\n", u4ReadReg(RM_VLD_REG_OFFSET0 + 42 * 4));
  printk("RV VLD window: 0x%08X\n", prParsingPic->u4InitInputWindow);
}


void vRM_SLT_CheckGlobalChecksum(UINT32 u4InstID)
{
  UINT32 i;
  UINT32 au4ChecksumData[10];   
  
//  RM_VLD
  au4ChecksumData[0] = u4VDecReadRMVLD(u4InstID, 59*4);     //	59	bitstream check_sum
  au4ChecksumData[1] = u4VDecReadRMVLD(u4InstID, 60*4);     //	60	residual check_sum

//MC
  au4ChecksumData[2] = u4VDecReadMC(u4InstID, 378*4);    //	378	idct_in_y0_y1_checksum_reg
  au4ChecksumData[3] = u4VDecReadMC(u4InstID, 379*4);    //	379	idct_in_y2_y3_checksum_reg
  au4ChecksumData[4] = u4VDecReadMC(u4InstID, 380*4);    //	380	idct_in_cb_cr_checksum_reg
  au4ChecksumData[5] = u4VDecReadMC(u4InstID, 381*4);    //	381	mc_out_y_checksum_reg
  au4ChecksumData[6] = u4VDecReadMC(u4InstID, 382*4);    //	382	mc_out_c_checksum_reg
  au4ChecksumData[7] = u4VDecReadMC(u4InstID, 390*4);    //	390	dram_rbus_checksum_reg

//PP
  au4ChecksumData[8] = u4VDecReadRMPP(u4InstID, 5*4);    //	5	pp_y_chk_sum_reg
  au4ChecksumData[9] = u4VDecReadRMPP(u4InstID, 6*4);    //	6	pp_c_chk_sum_reg

  for (i=0; i<10; i++)
  {
    printk("%x, Checksum %x \n", i, au4ChecksumData[i]);
  }
  
}


BOOL fgRM_SLT_IsRMVDecComplete(UINT32 u4InstID)
{
  UINT32 u4MbX;
  UINT32 u4MbY;  
  UINT32 u4RealMbX;
  UINT32 u4RealMbY;
    
  vVDEC_HAL_RM_GetMbxMby(u4InstID, &u4MbX, &u4MbY);

  //check MbX, MbY
  u4RealMbX = (prParsingPic->u4Width /16);
  u4RealMbY = (prParsingPic->u4Height /16);

  if ((u4MbX == (u4RealMbX-1)) && 
       (u4MbY == (u4RealMbY-1)))
  {
    return TRUE;
  }
  
  return FALSE;
}


UINT32 u4RM_SLT_GetField(UINT32 u4Val, int b0, int b1)
{
    int i;
    UINT32 u4Mask = 0;
    
    for (i = b0; i <= b1; i++)
      u4Mask |= (1 << i);
    
    u4Val &= u4Mask;
    u4Val >>= b0;
    
    return u4Val;
}


UINT32 u4RM_SLT_mb_to_line_y(UINT32 buf, int width, int x, int y)
{
  UINT32 val = 0;
  int mb_x;
  int mb_y;
  int offset_x;
  int offset_y;
  int offset;

  mb_x = x >> 4;
  mb_y = y >> 4;

  offset_x = x % 16;
  offset_y = y % 32;

  offset = offset_x + 
           (offset_y << 4) +
           (mb_x << 9) +
           (((mb_y >> 1) * width) << 5);

  val = *(UINT32 *)(buf + offset);
    
  return val;
}

void vRM_SLT_mb_to_line_c(UINT32 *pu4Val1, UINT32 *pu4Val2, UINT32 buf, int width, int x, int y)
{
  int mb_x;
  int mb_y;
  int offset_x;
  int offset_y;
  int offset;

  mb_x = x >> 3;
  mb_y = y >> 3;

  offset_x = x % 8;
  offset_y = y % 16;

  offset = (offset_x << 1) + 
           (offset_y << 4) +
           (mb_x << 8) +
           (((mb_y >> 1) * width) << 4);

  *pu4Val1 = *(UINT32 *)(buf + offset);
  *pu4Val2 = *(UINT32 *)(buf + offset + 4);
}


UINT32 u4RM_SLT_InverseAddrSwap(UINT32 u4AddrSwapMode, UINT32 u4SwappedAddr, BOOL fgIsYComponent)
{
  unsigned int u4NonSwappedAddr, u4TempAddr;
  switch(u4AddrSwapMode)
  {
  case 0x1: //MT8520_SWAP_MODE_1
    u4NonSwappedAddr = ((u4SwappedAddr&0xFFFFFFC0) | ((u4SwappedAddr&0x20)>>5) | ((u4SwappedAddr&0x10)>>2) | ((u4SwappedAddr&0x8)>>2) | ((u4SwappedAddr&0x7)<<3));
    break;
  case 0x2: //MT8520_SWAP_MODE_2
    u4NonSwappedAddr = ((u4SwappedAddr&0xFFFFFFE0) | ((u4SwappedAddr&0x10)>>4) | ((u4SwappedAddr&0xF)<<1));
    break;
  case 0x4: // MT5351_SWAP_MODE_0
    if(fgIsYComponent)
    {
      u4TempAddr = ((u4SwappedAddr&0xFFFFFF80) | ((u4SwappedAddr&0x40)>>4) | ((u4SwappedAddr&0x3C)<<1) | (u4SwappedAddr&0x3));
      u4NonSwappedAddr = ((u4TempAddr&0xFFFFFF80) | ((u4TempAddr&0x7C)>>2) | ((u4TempAddr&0x3)<<5));
    }
    else
    {
      u4TempAddr = ((u4SwappedAddr&0xFFFFFFC0) | ((u4SwappedAddr&0x20)>>3) | ((u4SwappedAddr&0x1C)<<1) | (u4SwappedAddr&0x3));
      u4NonSwappedAddr = ((u4TempAddr&0xFFFFFFC0) | ((u4TempAddr&0x3C)>>2) | ((u4TempAddr&0x3)<<4));
    }
    break;
  case 0x5: // MT5351_SWAP_MODE_1
    if(fgIsYComponent)
    {
      u4TempAddr = ((u4SwappedAddr&0xFFFFFF00) | ((~u4SwappedAddr)&0x80) | (u4SwappedAddr&0x7F));
      u4NonSwappedAddr = ((u4TempAddr&0xFFFFFF80) | ((u4TempAddr&0x7C)>>2) | ((u4TempAddr&0x3)<<5));
    }
    else
    {
      u4TempAddr = ((u4SwappedAddr&0xFFFFFF80) | ((~u4SwappedAddr)&0x40) | (u4SwappedAddr&0x3F));
      u4NonSwappedAddr = ((u4TempAddr&0xFFFFFFC0) | ((u4TempAddr&0x3C)>>2) | ((u4TempAddr&0x3)<<4));
    }
    break;
  case 0x6: // MT5351_SWAP_MODE_2
    if(fgIsYComponent)
    {
      u4NonSwappedAddr = ((u4SwappedAddr&0xFFFFFF80) | ((u4SwappedAddr&0x7C)>>2) | ((u4SwappedAddr&0x3)<<5));
    }
    else
    {
      u4NonSwappedAddr = ((u4SwappedAddr&0xFFFFFFC0) | ((u4SwappedAddr&0x3C)>>2) | ((u4SwappedAddr&0x3)<<4));
    }
    break;
  default:
    u4NonSwappedAddr = u4SwappedAddr;
    break;
  }
  return u4NonSwappedAddr;
}


void vRM_SLT_InvAddressSwap(UINT32 u4InstID, 
	                                                       UINT32 u4SrcType, 
	                                                       VDEC_INFO_RM_PICINFO_T *prParsingPic,
	                                                       UINT32 u4InvsAddrBufY,
	                                                       UINT32 u4InvsAddrBufC)
{
  UINT32 i;
  UINT32 u4DataLength;
  UINT32 u4AlignW_Luma;
  UINT32 u4AlignH_Luma;
  UINT32 u4AlignH_Chroma;
  UINT32 u4AlignSize = 0;
  UINT32 u4NonSwappedAddr;
  UINT32 u4AddrSwapMode;
  UINT32 u4SwappedAddr;
  BYTE* pbSrcBufY;
  BYTE* pbSrcBufC;
  BYTE* pbOutBufY;
  BYTE* pbOutBufC;
  BYTE * pbTempBufAddr;
  UINT32 u4AddrressSwapSize = 16;

  u4AlignSize = 0x38000;

  u4AlignW_Luma = prParsingPic->u4Width;
  u4AlignH_Luma = prParsingPic->u4Height;

  //u4AlignW_Chroma = u4AlignW_Luma / 2;
  u4AlignH_Chroma = u4AlignH_Luma / 2;

  if (u4SrcType == 0)
  {
    pbSrcBufY = (BYTE*) prParsingPic->u4OutBufY;
    pbSrcBufC = (BYTE*) prParsingPic->u4OutBufC;
  }
  else if (u4SrcType == 2)
  {
    pbSrcBufY = (BYTE*) prParsingPic->u4MCOutBufY;
    pbSrcBufC = (BYTE*) prParsingPic->u4MCOutBufC;
  }
  else
  {
    pbSrcBufY = 0;
    pbSrcBufC = 0;

    ASSERT(0);
    return;
    //N/A
  }
  pbOutBufY = (BYTE*) u4InvsAddrBufY;
  pbOutBufC = (BYTE*) u4InvsAddrBufC;
  
  u4AddrSwapMode = 6;
  
  //Luma 
  u4DataLength = u4AlignW_Luma * u4AlignH_Luma;
  u4DataLength = (u4DataLength + u4AlignSize)/u4AlignSize;
  u4DataLength = (u4DataLength+1) * u4AlignSize + u4AlignSize;
  u4SwappedAddr = 0;
  
  for (i=0; i<u4DataLength; i+=u4AddrressSwapSize)
  {
    u4NonSwappedAddr = u4RM_SLT_InverseAddrSwap(u4AddrSwapMode, u4SwappedAddr, TRUE);
    pbTempBufAddr = (BYTE*) (pbSrcBufY+i);
    x_memcpy(&pbOutBufY[u4NonSwappedAddr<<4], &pbTempBufAddr[0],u4AddrressSwapSize);
    u4SwappedAddr++;
  }
  
  //Chroma
  u4DataLength = u4AlignW_Luma * u4AlignH_Chroma;
  u4DataLength = (u4DataLength + u4AlignSize)/u4AlignSize;
  u4DataLength = u4DataLength * u4AlignSize + u4AlignSize;
  u4SwappedAddr = 0;

  for (i=0; i<u4DataLength; i+=u4AddrressSwapSize)
  {
    u4NonSwappedAddr = u4RM_SLT_InverseAddrSwap(u4AddrSwapMode, u4SwappedAddr, FALSE);
    pbTempBufAddr = (BYTE*) (pbSrcBufC+i);
    x_memcpy(&pbOutBufC[u4NonSwappedAddr<<4], &pbTempBufAddr[0],u4AddrressSwapSize);
    u4SwappedAddr++;
  }
}



BOOL fgRM_SLT_CheckYCGolden(UINT32 u4InstID, 
	                                                         VDEC_INFO_RM_PICINFO_T *prParsingPic,
	                                                         UINT32 u4InvsAddrBufY,
	                                                         UINT32 u4InvsAddrBufC,
	                                                         UINT32 u4YCGoldenBuf)
{
  UINT32 u4DecWidth;    //Align Width
  UINT32 u4DecWidthAlign;    //Align Width
  UINT32 u4DecHeight;    //Align Height
  UINT32 u4Width;    //Org Width
  UINT32 u4Height;    //Org Height

  UINT32 y;
  UINT32 x;

  UINT32 u4ErrVal;
  UINT32 u4GolVal;
  UINT32 u4ErrCntY;
  UINT32 u4ErrCntC;

  BOOL fgYOK;
  BOOL fgUOK;
  BOOL fgVOK;

  UINT32 u4OutBufY;
  UINT32 u4OutBufC;
  UINT32 u4GoldenBuf;

  UINT32 u4UVal;
  UINT32 u4VVal;
  UINT32 u4ErrVal2;
  UINT32 u4GoldYSz;
  UINT32 u4GoldYUSz;
  UINT32 u4GolVal2;

  UINT32 u4WMBSize;

  HalFlushInvalidateDCache();

  HalFlushInvalidateDCache();

  HalFlushInvalidateDCache();

  vRM_SLT_InvAddressSwap(u4InstID, 0, prParsingPic, u4InvsAddrBufY, u4InvsAddrBufC);

  HalFlushInvalidateDCache();

  u4OutBufY = (UINT32) u4InvsAddrBufY;
  u4OutBufC = (UINT32) u4InvsAddrBufC;
  
  
  u4GoldenBuf = u4YCGoldenBuf;

  u4DecWidth = prParsingPic->u4Width;
  u4DecWidthAlign = prParsingPic->u4Width;
  u4DecHeight = prParsingPic->u4Height;
  u4Width = prParsingPic->u4OrgWidth;
  u4Height = prParsingPic->u4OrgHeight;

  u4WMBSize = prParsingPic->u4Width/ 16;
  if (u4WMBSize % 4)
  {
    u4WMBSize = u4WMBSize + (4 - (u4WMBSize % 4));
  }

  u4DecWidthAlign = u4WMBSize * 16;   
  
  u4GoldYSz = u4DecWidth * u4DecHeight;
  u4GoldYUSz = u4GoldYSz + u4GoldYSz / 4;
 
  //Compare Y Golden
  fgYOK = TRUE;
  for (y = 0; y < u4Height; y++)
  {
    for (x = 0; x < u4Width; x += 4)
    {
      u4ErrVal = u4RM_SLT_mb_to_line_y(u4OutBufY, u4DecWidthAlign, x, y);
      u4GolVal = *(UINT32 *)(u4GoldenBuf + y * u4DecWidth + x);
      
      if (u4ErrVal != u4GolVal)
      {
        u4ErrCntY++;
        fgYOK = FALSE;

        //while (1);
        //{
          u4ErrCntY+=2;
          printk("Check Golden Y Fail /n");
        //  x_thread_delay(5);
        //}
      }
    }
  }

  //Compare C Data
  fgUOK = TRUE;
  fgVOK = TRUE;
  for (y = 0; y < u4Height/2; y++)
  {
    for (x = 0; x < u4Width/2; x += 4)
    {
      u4UVal = *(UINT32 *)(u4GoldenBuf + u4GoldYSz  + y * u4DecWidth / 2 + x);
      u4VVal = *(UINT32 *)(u4GoldenBuf + u4GoldYUSz + y * u4DecWidth / 2 + x);

      vRM_SLT_mb_to_line_c(&u4ErrVal, &u4ErrVal2, u4OutBufC, u4DecWidthAlign, x, y);
      u4GolVal = ((u4VVal << 16) & 0xFF000000) | ((u4UVal << 8) & 0x00FF0000) |
                 ((u4VVal <<  8) & 0x0000FF00) | ((u4UVal) & 0x000000FF);

      if (u4ErrVal != u4GolVal)
      {
        u4ErrCntC++;
        fgUOK = fgVOK = FALSE;

        //while (1);
        //{
          printk("Check Golden U/V Fail /n");
        //  x_thread_delay(1);
          u4ErrCntY+=3;
        //}
      }

      u4GolVal2 = ((u4VVal) & 0xFF000000)      | ((u4UVal >>  8) & 0x00FF0000) |
                  ((u4VVal >> 8) & 0x0000FF00) | ((u4UVal >> 16) & 0x000000FF);
                 
      if (u4ErrVal2 != u4GolVal2)
      {
        u4ErrCntC++;
        fgUOK = fgVOK = FALSE;

        //while (1);
        //{
          printk("Check Golden U/V Fail /n");
          //x_thread_delay(3);
          u4ErrCntY+=1;
        //}
      }
    }
  }

  if (fgYOK && fgUOK && fgVOK)
  {
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}


UINT32 u4RM_SLT_GetRvPic(UINT32 u4InstID, UINT32 * pu4InfobufAddr, VDEC_INFO_RM_PICINFO_T *prPic)
{
    UINT32 u4Parm;
    int i;
    
    u4Parm = *(UINT32 *)(pu4InfobufAddr);
    pu4InfobufAddr += 1;
    
    if (u4Parm == 0xFFFFFFFF)  // hit file end
      return FALSE;
    
    prPic->ePtype = (EnumRMPicCodType)u4RM_SLT_GetField(u4Parm, 0, 1);
    prPic->u4OrgWidth = u4RM_SLT_GetField(u4Parm, 2, 12);
    prPic->u4OrgHeight = u4RM_SLT_GetField(u4Parm, 13, 23);
    prPic->u4Width = (prPic->u4OrgWidth + 15) / 16 * 16;
    prPic->u4Height = (prPic->u4OrgHeight + 15) / 16 * 16;
    prPic->u4PctszSize = u4RM_SLT_GetField(u4Parm, 24, 25);
    prPic->u4Pctsz = u4RM_SLT_GetField(u4Parm, 26, 28);
    prPic->u4Oquant = u4RM_SLT_GetField(u4Parm, 29, 30);
    prPic->u4DFP = u4RM_SLT_GetField(u4Parm, 31, 31);

    u4Parm = *(UINT32 *)(pu4InfobufAddr);
    pu4InfobufAddr += 1;

    prPic->u4Tr = u4RM_SLT_GetField(u4Parm, 0, 12);
    prPic->u4Mba = u4RM_SLT_GetField(u4Parm, 13, 26);
    prPic->u4Rtype = u4RM_SLT_GetField(u4Parm, 27, 27);
    prPic->fgRV9 = u4RM_SLT_GetField(u4Parm, 28, 28);

    u4Parm = *(UINT32 *)(pu4InfobufAddr);
    pu4InfobufAddr += 1;

    prPic->u4NumSlice = u4RM_SLT_GetField(u4Parm, 0, 7);
    prPic->u4Pquant = u4RM_SLT_GetField(u4Parm, 8, 12);
    prPic->u4MbaSize = u4RM_SLT_GetField(u4Parm, 13, 16);
    prPic->u4HdrSkip = u4RM_SLT_GetField(u4Parm, 17, 24);

    u4Parm = *(UINT32 *)(pu4InfobufAddr);
    pu4InfobufAddr += 1;

    prPic->u4Iratio = u4Parm;

    prPic->u4BstLength = 0;
    
    for (i = 0; i < prPic->u4NumSlice; i++)
    {
      u4Parm = *(UINT32 *)(pu4InfobufAddr);
      pu4InfobufAddr += 1;
    
      prPic->au4SliceSize[i] = u4Parm;
      prPic->u4BstLength += u4Parm;
    }
    
    return (UINT32) pu4InfobufAddr;
}


void vRM_SLT_GetPicSize(UINT32 u4BSID, UINT32 u4VDecID, UINT32 * pu4PicWidth, UINT32 *pu4PicHeight)
{
  DWRD dwCode;	
  DWRD dwWidth, dwHieght;
  
  const DWRD code_width[8] = {160, 176, 240, 320, 352, 640, 704, 0};
  const DWRD code_height1[8] = {120, 132, 144, 240, 288, 480, 0, 0};
  const DWRD code_height2[4] = {180, 360, 576, 0};

  /* width */	
  dwCode = u4VDEC_HAL_RM_ShiftGetBitStream(u4BSID, u4VDecID, 3);
  dwWidth = code_width[dwCode];
  if (dwWidth == 0)
  {		
    do		
    {			
      dwCode = u4VDEC_HAL_RM_ShiftGetBitStream(u4BSID, u4VDecID, 8);
      dwWidth += (dwCode << 2);
    }		
    while (dwCode == 255);
  }
  /* height */
  dwCode = u4VDEC_HAL_RM_ShiftGetBitStream(u4BSID, u4VDecID, 3);
  dwHieght = code_height1[dwCode];
  if (dwHieght == 0)
  {
    dwCode <<= 1;
    dwCode |= u4VDEC_HAL_RM_ShiftGetBitStream(u4BSID, u4VDecID, 1);
    dwCode &= 3;
    dwHieght = code_height2[dwCode];
    if (dwHieght == 0)
    {
      do
      {
        dwCode = u4VDEC_HAL_RM_ShiftGetBitStream(u4BSID, u4VDecID, 8);
        dwHieght += (dwCode << 2);
      }
      while (dwCode == 255);
    }
  }
  *pu4PicWidth = dwWidth;
  *pu4PicHeight = dwHieght;
}


//Add Code for RM SLT
void vDEC_SLT_RM(const char *pFile1Name,const char *pFile2Name,const char *pFile3Name,const char *pFile4Name)
{
  UINT32 i;
  UINT32 u4InstID = 0;

  BOOL fgVdecRMSLTResult = TRUE;

  UINT32 u4VFIFOSize = 0x1400000;
  
  BYTE* pbRMVFIFOBuf = 0;
  BYTE* pbRMMvWorkBuf = 0;
  BYTE* pbRMVldWorkBuf = 0;
  BYTE* pbRMYBuf[4];
  BYTE* pbRMCBuf[4];
  BYTE* pbRMYCGoldenBuf[3];
  BYTE* pbRMInvAddrSwapBufY;
  BYTE* pbRMInvAddrSwapBufC;
  
  #ifdef RM_SLT_USE_SEMIHOSTING
  char strEmuFileName[256];
  FILE* fFileHandle;
  #else //RM_SLT_USE_SEMIHOSTING
  INT32 i4Fd = 0;
  INT32 i4_ret = 0;
  //UINT32 dwDriveNo   = 0;
  //UINT32 u4DrvFSTag  = 0;
  #endif //RM_SLT_USE_SEMIHOSTING
  UINT32 u4SrcFileSize;

  VDEC_INFO_RM_BS_INIT_PRM_T rRMVdecBSInfo; 

  UINT32 u4RMVFIFORPtr;
  UINT32 *pu4FIFOPtr;
  UINT32 u4ClipTotalFrameCnt = 3;
  UINT32 u4FrmInfoLength;
  UINT32 u4PayloadLength;
  UINT32 u4ProcessFrameCnt = 0; 

  UINT32 u4DecodedPicCnt;
  UINT32 u4RefQpMb0;

  UINT32 u4CntTimeChk = 0;
  UINT32 u4CheckCnt = 40000;

  x_strcpy( _arbSrcFilePath[0], pFile1Name);
  x_strcpy( _arbSrcFilePath[1], pFile2Name);
  x_strcpy( _arbSrcFilePath[2], pFile3Name);
  x_strcpy( _arbSrcFilePath[3], pFile4Name);


  VDec_IsrStop();
  
//Memory Alloc for VFIFO, HW Work Buffer, Y/C Buffer
  pbRMVFIFOBuf = x_alloc_aligned_dma_mem(u4VFIFOSize, 4096);    //VFIFO = 1 MB
  pbRMMvWorkBuf = x_alloc_aligned_dma_mem(RM_SLT_MVHWBUF_SZ, 4096);
  pbRMVldWorkBuf = x_alloc_aligned_dma_mem(RM_SLT_VLDPRED_SZ, 4096);
  pbRMYCGoldenBuf[0] = x_alloc_aligned_dma_mem(RM_SLT_GOLD_Y_SZ+RM_SLT_GOLD_C_SZ, 2048);
  pbRMYCGoldenBuf[1] = x_alloc_aligned_dma_mem(RM_SLT_GOLD_Y_SZ+RM_SLT_GOLD_C_SZ, 2048);
  pbRMYCGoldenBuf[2] = x_alloc_aligned_dma_mem(RM_SLT_GOLD_Y_SZ+RM_SLT_GOLD_C_SZ, 2048);
  pbRMInvAddrSwapBufY = x_alloc_aligned_dma_mem(RM_SLT_GOLD_Y_SZ, 2048);
  pbRMInvAddrSwapBufC = x_alloc_aligned_dma_mem(RM_SLT_GOLD_Y_SZ, 2048);

  prPic = x_alloc_aligned_dma_mem(sizeof(VDEC_INFO_RM_PICINFO_T), 8);
  prParsingPic = x_alloc_aligned_dma_mem(sizeof(VDEC_INFO_RM_PICINFO_T), 8);

  //i4VDEC_HAL_Common_Init(0);

  x_memset(prPic, 0x0, sizeof(VDEC_INFO_RM_PICINFO_T));
  x_memset(prParsingPic, 0x0, sizeof(VDEC_INFO_RM_PICINFO_T));
  
  u4RMVFIFORPtr = (UINT32) pbRMVFIFOBuf;

  if ((pbRMVFIFOBuf == 0) || 
  	(pbRMMvWorkBuf == 0) || 
  	(pbRMVldWorkBuf == 0) ||
  	(pbRMYCGoldenBuf[0] == 0) ||
  	(pbRMYCGoldenBuf[1] == 0) ||
  	(pbRMYCGoldenBuf[2] == 0) ||
  	(prPic == 0) ||
  	(prParsingPic == 0) ||
  	(pbRMInvAddrSwapBufY == 0) ||
  	(pbRMInvAddrSwapBufC == 0))
  {
    ASSERT(0);
  }

  for (i=0; i<4; i++)
  {
    pbRMYBuf[i] = x_alloc_aligned_dma_mem(RM_SLT_GOLD_Y_SZ, 2048);
    pbRMCBuf[i] = x_alloc_aligned_dma_mem(RM_SLT_GOLD_C_SZ, 2048);



    if ((pbRMYBuf[i] == 0) ||
    	 (pbRMCBuf[i] == 0))
    {
      ASSERT(0);
    }
  }


//Load SLT Verification Source
  #ifdef RM_SLT_USE_SEMIHOSTING
  //Semihosting Path
  x_sprintf(strEmuFileName, "%s%s%s", strSLTBaseFolder, "aulinfo", ".bin");
  fFileHandle = fopen(strEmuFileName, "rb");

  fseek(fFileHandle,0,SEEK_END);
  u4SrcFileSize = ftell(fFileHandle);
  fseek(fFileHandle,0,SEEK_SET);
  fread(pbRMVFIFOBuf, u4SrcFileSize, 1, fFileHandle);
  fclose(fFileHandle);

  for (i=0; i<3; i++)
  {
    x_sprintf(strEmuFileName, "%s%s%07d%s", strSLTBaseFolder, 
                                                             "g",
                                                            i,
                                                            ".yuv");
    fFileHandle = fopen(strEmuFileName, "rb");

    fseek(fFileHandle,0,SEEK_END);
    u4SrcFileSize = ftell(fFileHandle);
    fseek(fFileHandle,0,SEEK_SET);
    fread(pbRMYCGoldenBuf[i], u4SrcFileSize, 1, fFileHandle);
    fclose(fFileHandle);
  }
  #else //RM_SLT_USE_SEMIHOSTING
  //USB Path
  //i4_ret = DrvFSMount(dwDriveNo, &u4DrvFSTag);
  //if (i4_ret < 0)
  //{
  //    printk("Fs mount fail %d\n", i4_ret);
  //    ASSERT(0);
  //    return;
  //}

  //aulinfo.bin
  i4_ret = DrvFSOpenFile((char*)(_arbSrcFilePath[0]), DRV_FS_RDONLY, &i4Fd);
  if (i4_ret < 0)
  {
    printk("Fs open file fail %d\n", i4_ret);
    ASSERT(0);
    return;
  }
  i4_ret = DrvFSGetFileSize(i4Fd,(UINT32*) (&u4SrcFileSize));
  if (i4_ret < 0)
  {
    printk("Fs get file size fail %d\n", i4_ret);
    ASSERT(0);
    return;
  }
  i4_ret = DrvFSReadFile(i4Fd, (void*) pbRMVFIFOBuf, u4SrcFileSize);
  if(i4_ret < 0)
  {
    printk("read file Fail!\n");
    return;
  }
  DrvFSCloseFile(i4Fd);

  for (i=0; i<3; i++)
  {
    i4_ret = DrvFSOpenFile((char*)(_arbSrcFilePath[i+1]), DRV_FS_RDONLY, &i4Fd);
    if (i4_ret < 0)
    {
      printk("Fs open file fail %d\n", i4_ret);
      ASSERT(0);
      return;
    }
    i4_ret = DrvFSGetFileSize(i4Fd,(UINT32*) (&u4SrcFileSize));
    if (i4_ret < 0)
    {
      printk("Fs get file size fail %d\n", i4_ret);
      ASSERT(0);
      return;
    }
    i4_ret = DrvFSReadFile(i4Fd, (void*) pbRMYCGoldenBuf[i], u4SrcFileSize);
    if(i4_ret < 0)
    {
      printk("read file Fail!\n");
      break;
    }
    DrvFSCloseFile(i4Fd);
  }
  //DrvFSUnMount();      
  #endif //RM_SLT_USE_SEMIHOSTING
  


//Parsing Data
  while (u4ProcessFrameCnt < u4ClipTotalFrameCnt)
  {
    if (u4RMVFIFORPtr == (UINT32) pbRMVFIFOBuf)
    {
      //Get Total Frame Number
      pu4FIFOPtr = (UINT32*) u4RMVFIFORPtr;
      u4ClipTotalFrameCnt = pu4FIFOPtr[0];
  
      pu4FIFOPtr = pu4FIFOPtr+1;
    }
    else
    {
      pu4FIFOPtr = (UINT32*) u4RMVFIFORPtr;
    }
  
    //Check RM Sync Code
    if (pu4FIFOPtr[0] != 0x0FF00101)
    {
      printk("RM Error Data in AU FIFO \n");
      while (1)
      {
        x_thread_delay(1);
      }
    }
  
    pu4FIFOPtr+=1;
    u4FrmInfoLength = pu4FIFOPtr[0];
  
    pu4FIFOPtr +=1;
    u4PayloadLength = pu4FIFOPtr[0];
    
    HalFlushInvalidateDCache();
    
  //VDec HW Init Flow  
#if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8555)
    vVDecResetHW(u4InstID);
#else
    vVDecResetHW(u4InstID, VDEC_UNKNOWN);
#endif
    
    //Init RM VDec HW
    vRM_Hal_VldSoftReset(u4InstID);
  
    vRM_Hal_MvInit(u4InstID, (UINT32) pbRMMvWorkBuf);
  
    vRM_Hal_McInit(u4InstID);
  
    vRM_Hal_PpInit(u4InstID);
  
    //Init RM BS HW
    vRM_Hal_VldInit(u4InstID, (UINT32) pbRMVldWorkBuf);
  
    u4RMVFIFORPtr = ((UINT32) pu4FIFOPtr) + u4FrmInfoLength;
    rRMVdecBSInfo.u4VFifoSa = (UINT32) pbRMVFIFOBuf;
    rRMVdecBSInfo.u4VFifoEa = (UINT32) pbRMVFIFOBuf + u4VFIFOSize;
    rRMVdecBSInfo.u4ReadPointer = u4RMVFIFORPtr;
    rRMVdecBSInfo.u4WritePointer = u4RMVFIFORPtr + u4PayloadLength  +1024;
    i4RM_HAL_InitBarrelShifter(0, u4InstID, &rRMVdecBSInfo);
  
    pu4FIFOPtr += 2;    //Sync to frminfo Data Ptr
    u4RMVFIFORPtr = u4RM_SLT_GetRvPic(u4InstID, pu4FIFOPtr, prPic);  
    
    //Update Rptr to Next Header
    u4RMVFIFORPtr = u4RMVFIFORPtr + u4PayloadLength;
  
    u4DecodedPicCnt = prParsingPic->u4DecodedPicCnt;
    u4RefQpMb0 = prParsingPic->u4RefQpMb0;
    x_memcpy((void*) prParsingPic, (void*) prPic, sizeof(VDEC_INFO_RM_PICINFO_T));
    prParsingPic->u4RefQpMb0 = u4RefQpMb0;
    prParsingPic->u4DecodedPicCnt = u4DecodedPicCnt;
    
    if (prPic->fgRV9)
    {
      //RV9, RV10 Syntax
      prParsingPic->fgECC = u4VDEC_HAL_RM_ShiftGetBitStream(0, u4InstID, 1);
      prParsingPic->ePtype = (EnumRMPicCodType) u4VDEC_HAL_RM_ShiftGetBitStream(0, u4InstID, 2);
      prParsingPic->u4PSQuant = u4VDEC_HAL_RM_ShiftGetBitStream(0, u4InstID, 5);
      prParsingPic->fgBit_Ver = u4VDEC_HAL_RM_ShiftGetBitStream(0, u4InstID, 1);
      prParsingPic->fgInterlace = u4VDEC_HAL_RM_ShiftGetBitStream(0, u4InstID, 1);
      prParsingPic->u4OsvQuant = u4VDEC_HAL_RM_ShiftGetBitStream(0, u4InstID, 2);
      prParsingPic->fgDeblockPassThru = u4VDEC_HAL_RM_ShiftGetBitStream(0, u4InstID, 1);
      prParsingPic->u4RvTr = u4VDEC_HAL_RM_ShiftGetBitStream(0, u4InstID, 13);
  
      prParsingPic->u4Iratio = prPic->u4Iratio;
  
      if ((prParsingPic->ePtype == RM_INTERPIC) || (prParsingPic->ePtype == RM_TRUEBPIC))
      {
        prParsingPic->fgUserPrevWidth = u4VDEC_HAL_RM_ShiftGetBitStream(0, u4InstID, 1);
  
        if (!(prParsingPic->fgUserPrevWidth))
        {
          vRM_SLT_GetPicSize(0, u4InstID, &prParsingPic->u4OrgWidth, &prParsingPic->u4OrgHeight);
        }
      }
      else
      {
        vRM_SLT_GetPicSize(0, u4InstID, &prParsingPic->u4OrgWidth, &prParsingPic->u4OrgHeight);
      }
  
      prParsingPic->u4MbaSize = prPic->u4MbaSize;
      prParsingPic->u4Mba = u4VDEC_HAL_RM_ShiftGetBitStream(0, u4InstID, prParsingPic->u4MbaSize);
    }
    else
    {
      //RV8 Syntax
      prParsingPic->u4RvBistreamVersion = u4VDEC_HAL_RM_ShiftGetBitStream(0, u4InstID, 3);
      prParsingPic->ePtype = (EnumRMPicCodType) u4VDEC_HAL_RM_ShiftGetBitStream(0, u4InstID, 2);
      prParsingPic->fgECC = u4VDEC_HAL_RM_ShiftGetBitStream(0, u4InstID, 1);
      prParsingPic->u4PSQuant = u4VDEC_HAL_RM_ShiftGetBitStream(0, u4InstID, 5);
      prParsingPic->fgDeblockPassThru = u4VDEC_HAL_RM_ShiftGetBitStream(0, u4InstID, 1);
      prParsingPic->u4RvTr = u4VDEC_HAL_RM_ShiftGetBitStream(0, u4InstID, 13);
  
      prParsingPic->u4PctszSize = prPic->u4PctszSize;
      if (prPic->u4PctszSize > 0)
      {
        prParsingPic->u4PicSz = u4VDEC_HAL_RM_ShiftGetBitStream(0, u4InstID, prParsingPic->u4PctszSize);
      }
  
      prParsingPic->u4MbaSize = prPic->u4MbaSize;
      prParsingPic->u4Mba = u4VDEC_HAL_RM_ShiftGetBitStream(0, u4InstID, prParsingPic->u4MbaSize);
      prParsingPic->fgRType = u4VDEC_HAL_RM_ShiftGetBitStream(0, u4InstID, 1);
    }
  
    if((prParsingPic->ePtype == RM_INTRAPIC) || (prParsingPic->ePtype == RM_FORCED_INTRAPIC))
    {
      prParsingPic->fgBwdIsI = TRUE;
    }
    else if(prParsingPic->ePtype == RM_INTERPIC)
    {
      prParsingPic->fgBwdIsI = FALSE;
    }
  
  
  //Update Y/C Decode Output Buffer
    if (prParsingPic->ePtype == RM_INTRAPIC || prParsingPic->ePtype == RM_FORCED_INTRAPIC)
    {
      if (prParsingPic->u4FwdBufY == 0)  // the first I pic
      {
        prParsingPic->u4OutBufY = (UINT32) pbRMYBuf[1];
        prParsingPic->u4OutBufC = (UINT32) pbRMCBuf[1];
        prParsingPic->u4FwdBufY = (UINT32) pbRMYBuf[1];
        prParsingPic->u4FwdBufC = (UINT32) pbRMCBuf[1];
        prParsingPic->u4BwdBufY = 0;
        prParsingPic->u4BwdBufC = 0;
      }
      else if (prParsingPic->u4BwdBufY == (UINT32) pbRMYBuf[1])
      {
        prParsingPic->u4OutBufY = (UINT32) pbRMYBuf[2];
        prParsingPic->u4OutBufC = (UINT32) pbRMCBuf[2];
        prParsingPic->u4FwdBufY = (UINT32) pbRMYBuf[1];
        prParsingPic->u4FwdBufC = (UINT32) pbRMCBuf[1];
        prParsingPic->u4BwdBufY = (UINT32) pbRMYBuf[2];
        prParsingPic->u4BwdBufC = (UINT32) pbRMCBuf[2];
      }
      else if (prParsingPic->u4BwdBufY == (UINT32) pbRMYBuf[2])
      {
        prParsingPic->u4OutBufY = (UINT32) pbRMYBuf[1];
        prParsingPic->u4OutBufC = (UINT32) pbRMCBuf[1];
        prParsingPic->u4FwdBufY = (UINT32) pbRMYBuf[2];
        prParsingPic->u4FwdBufC = (UINT32) pbRMCBuf[2];
        prParsingPic->u4BwdBufY = (UINT32) pbRMYBuf[1];
        prParsingPic->u4BwdBufC = (UINT32) pbRMCBuf[1];
      }
      else if (prParsingPic->u4BwdBufY == 0)  // should happen only when 2 consecutive I pictures at the beginning
      {
        prParsingPic->u4OutBufY = (UINT32) pbRMYBuf[2];
        prParsingPic->u4OutBufC = (UINT32) pbRMCBuf[2];
        prParsingPic->u4FwdBufY = (UINT32) pbRMYBuf[1];
        prParsingPic->u4FwdBufC = (UINT32) pbRMCBuf[1];
        prParsingPic->u4BwdBufY = (UINT32) pbRMYBuf[2];
        prParsingPic->u4BwdBufC =(UINT32)  pbRMCBuf[2];
      }
      else
      {
        ASSERT(0);
      }
  
      // initialize the previous picture sizes
      prParsingPic->u4PrevDispWidth = prParsingPic->u4OrgWidth;
      prParsingPic->u4PrevDispHeight = prParsingPic->u4OrgHeight;
      prParsingPic->u4PrevDecWidth = prParsingPic->u4Width;
      prParsingPic->u4PrevDecHeight = prParsingPic->u4Height;
    }
    else if (prParsingPic->ePtype == RM_INTERPIC)
    {
      if (prParsingPic->u4BwdBufY == 0)  // the first P pic
      {
        prParsingPic->u4OutBufY = (UINT32) pbRMYBuf[2];
        prParsingPic->u4OutBufC = (UINT32) pbRMCBuf[2];
        prParsingPic->u4FwdBufY = (UINT32) pbRMYBuf[1];
        prParsingPic->u4FwdBufC = (UINT32) pbRMCBuf[1];
        prParsingPic->u4BwdBufY = (UINT32) pbRMYBuf[2];
        prParsingPic->u4BwdBufC = (UINT32) pbRMCBuf[2];
      }
      else if (prParsingPic->u4BwdBufY == (UINT32) pbRMYBuf[1])
      {
        prParsingPic->u4OutBufY = (UINT32) pbRMYBuf[2]; //PPY_OUT_BUF2;
        prParsingPic->u4OutBufC = (UINT32) pbRMCBuf[2]; //PPC_OUT_BUF2;
        prParsingPic->u4FwdBufY = (UINT32) pbRMYBuf[1]; //PPY_OUT_BUF1;
        prParsingPic->u4FwdBufC = (UINT32) pbRMCBuf[1]; //PPC_OUT_BUF1;
        prParsingPic->u4BwdBufY = (UINT32) pbRMYBuf[2]; //PPY_OUT_BUF2;
        prParsingPic->u4BwdBufC = (UINT32) pbRMCBuf[2]; //PPC_OUT_BUF2;
      }
      else if (prParsingPic->u4BwdBufY == (UINT32) pbRMYBuf[2])
      {
        prParsingPic->u4OutBufY = (UINT32) pbRMYBuf[1]; //PPY_OUT_BUF1;
        prParsingPic->u4OutBufC = (UINT32) pbRMCBuf[1]; //PPC_OUT_BUF1;
        prParsingPic->u4FwdBufY = (UINT32) pbRMYBuf[2]; //PPY_OUT_BUF2;
        prParsingPic->u4FwdBufC = (UINT32) pbRMCBuf[2]; //PPC_OUT_BUF2;
        prParsingPic->u4BwdBufY = (UINT32) pbRMYBuf[1]; //PPY_OUT_BUF1;
        prParsingPic->u4BwdBufC = (UINT32) pbRMCBuf[1]; //PPC_OUT_BUF1;
      }
      else
      {
        ASSERT(0);
      }
       
      if (prParsingPic->u4OrgWidth != prParsingPic->u4PrevDispWidth || prParsingPic->u4OrgHeight != prParsingPic->u4PrevDispHeight)  // RPR
      {
        printk("Normal RPR, About to trigger image resizer...\n");
        #if 0
        vRprImgResz(
           u4InstID,
            prParsingPic->u4PrevDecWidth, prParsingPic->u4PrevDecHeight,
            prParsingPic->u4PrevDispWidth, prParsingPic->u4PrevDispHeight,
            prParsingPic->u4Width, prParsingPic->u4Height,
            prParsingPic->u4OrgWidth, prParsingPic->u4OrgHeight
          );
        #endif //0
        printk("Normal RPR, Image has been resized!\n");
                
        prParsingPic->u4FwdBufY = (UINT32) pbRMYBuf[3]; //PPY_OUT_BUFR;
        prParsingPic->u4FwdBufC = (UINT32) pbRMCBuf[3]; //PPC_OUT_BUFR;
      }
        
      prParsingPic->u4PrevDispWidth = prParsingPic->u4OrgWidth;
      prParsingPic->u4PrevDispHeight = prParsingPic->u4OrgHeight;
      prParsingPic->u4PrevDecWidth = prParsingPic->u4Width;
      prParsingPic->u4PrevDecHeight = prParsingPic->u4Height;
    }
    else if (prParsingPic->ePtype == RM_TRUEBPIC)
    {
      prParsingPic->u4OutBufY = (UINT32) pbRMYBuf[0]; //PPY_OUT_BUF3;
      prParsingPic->u4OutBufC = (UINT32) pbRMCBuf[0]; //PPC_OUT_BUF3;
    }
    else
    {
      ASSERT(0);
    }
  
    if ((prParsingPic->u4OutBufY == 0) || (prParsingPic->u4OutBufC == 0))
    {
      ASSERT(0);
    }
  
    x_memcpy((void*) prPic, (void*) prParsingPic, sizeof(VDEC_INFO_RM_PICINFO_T));  
  
  
  
  //Trigger Decode
    prParsingPic->u4AddrSwapMode = 2;
    prParsingPic->fgEnableCRC = FALSE;
    prParsingPic->fgEnableMCOutput = FALSE;
    prParsingPic->fgEnableDDR3 = TRUE;
    prParsingPic->fgEnableAddrSwap = TRUE;
    prParsingPic->fgEnablePPOut = TRUE;
 
 #if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8560)
    vVDEC_HAL_RM_TriggerDecode(u4InstID, prParsingPic); 
 #else
    vVDEC_HAL_RM_TriggerDecode(u4InstID, prParsingPic,NULL); 
 #endif
  
  
  //Verify Result
    //Check Decode End
    u4CntTimeChk = 0;
    while (u4VDEC_HAL_RM_VDec_ReadFinishFlag(u4InstID) == 0)
    {
    	//Printf("[VDEC-RM] Check1 %d %d \n", u4CntTimeChk, u4ProcessFrameCnt);
      u4CntTimeChk++;
      u4CheckCnt = 0;
      x_thread_delay(1);
    }
    
    u4CntTimeChk = 0;
    while (fgRM_SLT_IsRMVDecComplete(u4InstID) == FALSE)
    {
    	//Printf("[VDEC-RM] Check2 %d %d \n", u4CntTimeChk, u4ProcessFrameCnt);
      u4CntTimeChk++;
      u4CheckCnt = 0;
      x_thread_delay(1);
    }
#if 0    
    u4CntTimeChk = 0;
    while (u4CntTimeChk < u4CheckCnt)
    {
      if (u4VDEC_HAL_RM_VDec_ReadFinishFlag(u4InstID))
      {
        //u4CntTimeChk = 0;
        break;
      }
      u4CntTimeChk++;
    }

    u4CntTimeChk = 0;
    while (u4CntTimeChk < u4CheckCnt)
    {
      if(fgRM_SLT_IsRMVDecComplete(u4InstID))
      {
        //u4CntTimeChk = 0;
        break;
      }
  
      u4CntTimeChk++;
    }
    //if (u4CntTimeChk >= u4CheckCnt)
    //{
    //  while (1)
    //  {
    //    Printf("Decoding End Error %x \n", u4ProcessFrameCnt);
    //    x_thread_delay(5);
    //  }
    //}
#endif //0    

    BIM_ClearIrq(VECTOR_VDFUL);
    
    if (fgDebugLogEnable == TRUE)
    {
      vRM_SLT_DumpVldCkSum();
      vRM_SLT_DumpAvcMvCkSum();
      vRM_SLT_DumpMcCkSum();
      vRM_SLT_DumpPpCkSum();
      vRM_SLT_DumpReg(prParsingPic);
      vRM_SLT_CheckGlobalChecksum(u4InstID);
    }

//Check Golden Data
    fgVdecRMSLTResult = fgRM_SLT_CheckYCGolden(u4InstID, prParsingPic, 
                                                (UINT32) pbRMInvAddrSwapBufY, 
                                                (UINT32) pbRMInvAddrSwapBufC, 
                                                (UINT32) pbRMYCGoldenBuf[u4ProcessFrameCnt]);
    
    BIM_ClearIrq(VECTOR_VDFUL);

    if (fgVdecRMSLTResult == FALSE)
    {
      printk("[RM SLT] Golden Check Fail \n");
      break;
    }

    if (prParsingPic->ePtype != RM_TRUEBPIC)// && !_fgDecTimeout)
    {
      prParsingPic->u4RefQpMb0 = u4VDecReadRMPP(0, RW_RMPP_PQ);

      if (fgDebugLogEnable == TRUE)
      {
        printk("RefQpMb0 %x \n", prParsingPic->u4RefQpMb0);
      }
    }

    u4ProcessFrameCnt++;
  }

  if(fgVdecRMSLTResult)
  {
    printk("RM SLT Pass \n");
    SLTSendResult(SLT_ITEMS_RM, SLT_STATUS_PASS); 
  }
  else
  {
    printk("RM SLT Fail \n");
    SLTSendResult(SLT_ITEMS_RM, SLT_STATUS_FAIL);
  }


//Free Memory
  if (pbRMVFIFOBuf != 0)
  {
    x_free_aligned_dma_mem(pbRMVFIFOBuf);
  }
  if (pbRMMvWorkBuf != 0)
  {
    x_free_aligned_dma_mem(pbRMMvWorkBuf);
  }
  if (pbRMVldWorkBuf != 0)
  {
    x_free_aligned_dma_mem(pbRMVldWorkBuf);
  }
  if (prPic != 0)
  {
    x_free_aligned_dma_mem(prPic);
  }
  if (prParsingPic != 0)
  { 
    x_free_aligned_dma_mem(prParsingPic);
  }
  for (i=0; i<3; i++)
  {
    if (pbRMYCGoldenBuf[i] != 0)
    {
      x_free_aligned_dma_mem(pbRMYCGoldenBuf[i]);
    }
  }
  if (pbRMInvAddrSwapBufY != 0)
  {
    x_free_aligned_dma_mem(pbRMInvAddrSwapBufY);
  }
  if (pbRMInvAddrSwapBufC != 0)
  {
    x_free_aligned_dma_mem(pbRMInvAddrSwapBufC);
  }

  for (i=0; i<4; i++)
  {
    if (pbRMYBuf[i] != 0)
    {
      x_free_aligned_dma_mem(pbRMYBuf[i]);
    }
    if (pbRMCBuf[i] != 0)
    {
      x_free_aligned_dma_mem(pbRMCBuf[i]);
    }
  }

  BIM_ClearIrq(VECTOR_VDFUL);
  BIM_ClearIrq(VECTOR_VDFUL);
  VDec_IsrInit();
}

#endif //RM_HAL_SUPPORT_SLT



