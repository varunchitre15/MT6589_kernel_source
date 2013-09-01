#include "vdec_verify_mpv_prov.h"
#include "../hal/vdec_hal_if_common.h"
#include "../hal/vdec_hal_if_wmv.h"
#include "vdec_verify_keydef.h"
//#include "vdec_drv_wmv_info.h"
//#include <string.h>
//#include <stdio.h>
//#include <math.h>
#include "vdec_verify_file_common.h"
//#include "vdec_verify_vparser_wmv.h"

#include "../hal/vdec_hw_common.h"

#include "vdec_verify_general.h"
//#include <string.h>
//#include <stdio.h>
//#include <stdlib.h>

#include "vdec_verify_vparser_rm.h"
#include "vdec_verify_imgresz_rm.h"


BOOL fgCKGenEnable = FALSE;


void vRM_VDecWrite_ImgResz(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
  if (u4VDecID == 0)
  {
    vWriteReg(IMG_RESZ_REG_OFFSET0 + u4Addr, u4Val);
  }
}

UINT32 u4RM_VDecRead_ImgResz(UINT32 u4VDecID, UINT32 u4Addr)
{
    if (u4VDecID == 0)
    {
        return (u4ReadReg(IMG_RESZ_REG_OFFSET0 + u4Addr));
    }
    else
    {
      return 0;
    }
}


void vRprImgResz(
  UINT32 u4InstID,
  UINT32 u4PrevPicDecWidth, UINT32 u4PrevPicDecHeight,
  UINT32 u4PrevPicDispWidth, UINT32 u4PrevPicDispHeight,
  UINT32 u4CurrPicDecWidth, UINT32 u4CurrPicDecHeight,
  UINT32 u4CurrPicDispWidth, UINT32 u4CurrPicDispHeight)
{
  UINT32 qw;
  UINT32 hfactor;
  UINT32 hoffset;
  UINT32 vfactor;
  UINT32 voffset;

  int  uInWidth, uInHeight;
  int  uOutWidth, uOutHeight;
  int  iUxR, iUyLB;
  int  Hprime, D;
  int  m, n;
  int  ax_initial, ax_increment;
  int  iUyL_inc;
  int  uTemp;
  
  VDEC_INFO_RM_PICINFO_T *prParsingPic;   

  if (fgCKGenEnable != TRUE)
  {
    UINT32 u4Value;
  ///TODO:
  /*
    u4Value = CKGEN_AgtGetClk(e_CLK_PNG);

  #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580) 
    if(u4Value != CLK_CFG1_PNG_SEL_SYSPLL2_1_2)
        CKGEN_AgtSelClk(e_CLK_PNG,CLK_CFG1_PNG_SEL_SYSPLL2_1_2);
  #else
    if(u4Value != CLK_CLK_PNG_SEL_SYSPLL2_1_2)
        CKGEN_AgtSelClk(e_CLK_PNG,CLK_CLK_PNG_SEL_SYSPLL2_1_2);
  #endif
  */
    fgCKGenEnable = TRUE;
  }
  

  prParsingPic = (VDEC_INFO_RM_PICINFO_T*) &_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo;
  
  if (_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.fgRPRRacingModeEnable)
  {
    vRM_VDecWrite_ImgResz(0, RZ_RPR * 4, 0x00000301);
    vRM_VDecWrite_ImgResz(0, IMG_RESZ_JPG_MODE * 4, 0x8001E000);
    vRM_VDecWrite_ImgResz(0, RZ_JPG_V_Y * 4, 0x00000020);
    vRM_VDecWrite_ImgResz(0, RZ_JPG_V_CB * 4, 0x00000010);
  }
  else	
  {
    vRM_VDecWrite_ImgResz(0, RZ_RPR * 4, 1); // RPR_test, urcrpr = 0
  }

  qw = 0;
  vRM_VDecWrite_ImgResz(0, IMG_RESZ_TYPE * 4, qw);

  #ifdef RM_DDR3MODE_ENABLE
  {
    UINT32 u4AlignSize = 0;
    
    //Image Resizer Must be 4 MB Align (Source and Destination)
    u4AlignSize = u4PrevPicDecWidth / 16;

    if (u4AlignSize % 4)
    {
      u4AlignSize = u4AlignSize + (4 - (u4AlignSize%4));
    }
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560)
    vRM_VDecWrite_ImgResz(0, RZ_SRC_BUF_LEN * 4, (u4AlignSize + (u4AlignSize << 12)));// For 8560 imgresz new design
#else
    vRM_VDecWrite_ImgResz(0, RZ_SRC_BUF_LEN * 4, u4AlignSize);
#endif
    u4AlignSize = (u4CurrPicDecWidth / 16);

    if (u4AlignSize % 4)
    {
      u4AlignSize = u4AlignSize + (4 - (u4AlignSize%4));
    }
    qw = 0x10000000 | u4AlignSize;
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560)
   vRM_VDecWrite_ImgResz(0, RZ_TG_BUF_LEN * 4, (qw + (u4AlignSize << 12))); // For 8560 imgresz new design
#else
    vRM_VDecWrite_ImgResz(0, RZ_TG_BUF_LEN * 4, qw);
#endif
  }
  #else //RM_DDR3MODE_ENABLE
  qw = u4PrevPicDecWidth / 16;
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560)
   vRM_VDecWrite_ImgResz(0, RZ_SRC_BUF_LEN * 4, (qw + (qw << 12)));// For 8560 imgresz new design
#else
  vRM_VDecWrite_ImgResz(0, RZ_SRC_BUF_LEN * 4, qw);
#endif

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560)
   qw = 0x10000000 | (u4CurrPicDecWidth / 16) | ((u4CurrPicDecWidth / 16) << 12);// For 8560 imgresz new design
#else
  qw = 0x10000000 | (u4CurrPicDecWidth / 16);
#endif
  //qw = 0x04000000 | (u4CurrPicDecWidth / 16);
  vRM_VDecWrite_ImgResz(0, RZ_TG_BUF_LEN * 4, qw);
  #endif //RM_DDR3MODE_ENABLE

  if (_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.fgRPRRacingModeEnable)
  {
    vRM_VDecWrite_ImgResz(0, RZ_SRC_Y_OW_ADDR_BASE1 * 4, PHYSICAL(prParsingPic->u4OutBufY)/16);
    vRM_VDecWrite_ImgResz(0, RZ_SRC_CB_OW_ADDR_BASE1 * 4, PHYSICAL(prParsingPic->u4OutBufC)/16);
  }
  else
  {
    vRM_VDecWrite_ImgResz(0, RZ_SRC_Y_OW_ADDR_BASE1 * 4, (UINT32) (PHYSICAL(prParsingPic->u4FwdBufY)/16));
    vRM_VDecWrite_ImgResz(0, RZ_SRC_CB_OW_ADDR_BASE1 * 4, (UINT32) (PHYSICAL(prParsingPic->u4FwdBufC)/16));
  }

  vRM_VDecWrite_ImgResz(0, RZ_TG_Y_OW_ADDR_BASE * 4, (UINT32) ((UINT32) PHYSICAL((UINT32)_pucPic3Y[u4InstID])/16));
  vRM_VDecWrite_ImgResz(0, RZ_TG_C_OW_ADDR_BASE * 4, (UINT32) ((UINT32) PHYSICAL((UINT32)_pucPic3C[u4InstID])/16));

  vRM_VDecWrite_ImgResz(0, RZ_TMP_Y_OW_ADDR_BASE * 4, ((UINT32) PHYSICAL((UINT32)_pucRMReszWorkBuf[u4InstID]))/16);
  //vRM_VDecWrite_ImgResz(0, RZ_TMP_Y_OW_ADDR_BASE * 4, RM_RSZTMPBUF_SA/16);
  
  qw = (u4PrevPicDispWidth << 16) | u4PrevPicDispHeight;
  vRM_VDecWrite_ImgResz(0, RZ_SRC_SIZE_Y * 4, qw); //src_width_y, src_height_y    

  qw = ((u4PrevPicDispWidth >> 1) << 16) | (u4PrevPicDispHeight >> 1);
  vRM_VDecWrite_ImgResz(0, RZ_SRC_SIZE_CB * 4, qw); //src_width_c, src_height_c      

//  qw = (u4CurrPicDecWidth) << 12 | u4CurrPicDecHeight;
  qw = (u4CurrPicDecWidth) << 16 | u4CurrPicDecHeight; // david
  vRM_VDecWrite_ImgResz(0, RZ_TG_SIZE_Y * 4, qw); //tg_width_y, tg_height_y

  // horizontal bilinear
  vRM_VDecWrite_ImgResz(0, RZ_H_COEF0 * 4, 0x0);
  vRM_VDecWrite_ImgResz(0, RZ_H_COEF1 * 4, 0xf0000000);
  vRM_VDecWrite_ImgResz(0, RZ_H_COEF2 * 4, 0xe0000000);
  vRM_VDecWrite_ImgResz(0, RZ_H_COEF3 * 4, 0xd0000000);
  vRM_VDecWrite_ImgResz(0, RZ_H_COEF4 * 4, 0xc0000000);
  vRM_VDecWrite_ImgResz(0, RZ_H_COEF5 * 4, 0xb0000000);
  vRM_VDecWrite_ImgResz(0, RZ_H_COEF6 * 4, 0xa0000000);
  vRM_VDecWrite_ImgResz(0, RZ_H_COEF7 * 4, 0x90000000);
  vRM_VDecWrite_ImgResz(0, RZ_H_COEF8 * 4, 0x80000000);
  vRM_VDecWrite_ImgResz(0, RZ_H_COEF9 * 4, 0x70000000);
  vRM_VDecWrite_ImgResz(0, RZ_H_COEF10 * 4, 0x60000000);
  vRM_VDecWrite_ImgResz(0, RZ_H_COEF11 * 4, 0x50000000);
  vRM_VDecWrite_ImgResz(0, RZ_H_COEF12 * 4, 0x40000000);
  vRM_VDecWrite_ImgResz(0, RZ_H_COEF13 * 4, 0x30000000);
  vRM_VDecWrite_ImgResz(0, RZ_H_COEF14 * 4, 0x20000000);
  vRM_VDecWrite_ImgResz(0, RZ_H_COEF15 * 4, 0x10000000);
  vRM_VDecWrite_ImgResz(0, RZ_H_COEF16 * 4, 0x00000008);
  vRM_VDecWrite_ImgResz(0, RZ_H_COEF17 * 4, 0x00000000);

  // vertical bilinear 
  vRM_VDecWrite_ImgResz(0, RZ_V_COEF0 * 4, 0xf0000000);
  vRM_VDecWrite_ImgResz(0, RZ_V_COEF1 * 4, 0xd000e000);
  vRM_VDecWrite_ImgResz(0, RZ_V_COEF2 * 4, 0xb000c000);
  vRM_VDecWrite_ImgResz(0, RZ_V_COEF3 * 4, 0x9000a000);
  vRM_VDecWrite_ImgResz(0, RZ_V_COEF4 * 4, 0x70008000);
  vRM_VDecWrite_ImgResz(0, RZ_V_COEF5 * 4, 0x50006000);
  vRM_VDecWrite_ImgResz(0, RZ_V_COEF6 * 4, 0x30004000);
  vRM_VDecWrite_ImgResz(0, RZ_V_COEF7 * 4, 0x10002000);
  vRM_VDecWrite_ImgResz(0, RZ_V_COEF8 * 4, 0x1);
  
  // -- scaling factor calculation --
  uInWidth   = u4PrevPicDispWidth;
  uInHeight  = u4PrevPicDispHeight;
  uOutWidth  = u4CurrPicDispWidth;
  uOutHeight = u4CurrPicDispHeight;
  
  m = 0;
  uTemp = uInWidth;
  while (uTemp > 0)
  {
    m=m+1;
    uTemp = uTemp>>1;
  }
  /* check for case when uInWidth is power of two */
  if (uInWidth == (1<<(m-1))) m=m-1;
  Hprime = 1<<m;
  D = (64*Hprime)/16;

  n = 0;
  uTemp = uInHeight;
  while (uTemp > 0)
  {
    n=n+1;
    uTemp = uTemp>>1;
  }
  /* check for case when uInHeight is power of two */
  if (uInHeight == (1<<(n-1))) n=n-1;

  /* iUxL and iUxR are independent of row, so compute once only */
  iUxR = ((((uInWidth - uOutWidth)<<1))<<(4+m));    /* numerator part */
  /* complete iUxR init by dividing by H with rounding to nearest integer, */
  /* half-integers away from 0 */
  if (iUxR >= 0)
    iUxR = (iUxR + (uOutWidth>>1))/uOutWidth;
  else
    iUxR = (iUxR - (uOutWidth>>1))/uOutWidth;

  /* initial x displacement and the x increment are independent of row */
  /* so compute once only */
  ax_initial = iUxR + (D>>1);
  ax_increment = (Hprime<<6) + (iUxR<<1);

  iUyLB = ((uInHeight - uOutHeight)<<(n+5)); /* numerator */
  /* complete iUyLB by dividing by V with rounding to nearest integer, */
  /* half-integers away from 0 */
  if (iUyLB >= 0)
    iUyLB = (iUyLB + (uOutHeight>>1))/uOutHeight;
  else
    iUyLB = (iUyLB - (uOutHeight>>1))/uOutHeight;
  iUyL_inc = (iUyLB<<1);

  hfactor = ax_increment<<(18-(m+6));
  hoffset = ax_initial<<(18-(m+6));
  vfactor = ((1<<(6+n)) + iUyL_inc) << (18-(n+6));
  voffset = (iUyLB + (1<<(1+n))) << (18-(n+6));

  vRM_VDecWrite_ImgResz(0, RZ_H8TAPS_SCL_Y * 4,  hfactor); 
  vRM_VDecWrite_ImgResz(0, RZ_H8TAPS_OFT_Y * 4,  hoffset); 
  vRM_VDecWrite_ImgResz(0, RZ_V4TAPS_SCL_Y * 4,  vfactor);
  vRM_VDecWrite_ImgResz(0, RZ_V4TAPS_OFT_Y * 4,  voffset);
  vRM_VDecWrite_ImgResz(0, RZ_H8TAPS_SCL_CB * 4, hfactor); 
  vRM_VDecWrite_ImgResz(0, RZ_H8TAPS_OFT_CB * 4, hoffset); 
  vRM_VDecWrite_ImgResz(0, RZ_V4TAPS_SCL_CB * 4, vfactor);
  vRM_VDecWrite_ImgResz(0, RZ_V4TAPS_OFT_CB * 4, voffset);

  #ifdef RM_ADDRSWAP_ENABLE
  {
    UINT32 u4AddressSwapMode = 0;
    UINT32 u4MIFMODRegValue = 0;

    u4AddressSwapMode = auImgRszAddrSwapMapTable[_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMAddrSwapMode];      

    u4MIFMODRegValue = u4RM_VDecRead_ImgResz(0, RZ_MEM_IF_MODE * 4);

    u4MIFMODRegValue = u4MIFMODRegValue & 0xFFFFFFC0;

    u4MIFMODRegValue = u4MIFMODRegValue | (u4AddressSwapMode<<4) | (u4AddressSwapMode << 0);
    
    vRM_VDecWrite_ImgResz(0, RZ_MEM_IF_MODE * 4, u4MIFMODRegValue);
  }
  #endif //RM_ADDRSWAP_ENABLE

  //RISCWrite(IMG_RESZ_BASE + IMG_RESZ_START * 4, 0x8E); // soft reset
  //RISCWrite(IMG_RESZ_BASE + IMG_RESZ_START * 4, 0x82);    
  vRM_VDecWrite_ImgResz(0, IMG_RESZ_START * 4, 0x83); // activate                
  vRM_VDecWrite_ImgResz(0, IMG_RESZ_START * 4, 0x82);    

  if (_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.fgRPRRacingModeEnable)  // since it will never finish without triggering vdec
  {
    return;
  }
  else
  {
    while (u4RM_VDecRead_ImgResz(0, IMG_RESZ_DONE * 4) == 0);
    printk("IMGresz Done!\n");
  }
}



