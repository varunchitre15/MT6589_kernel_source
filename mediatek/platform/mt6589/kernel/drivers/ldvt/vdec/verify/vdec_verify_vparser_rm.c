//#include "x_debug.h"
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
//#include "x_hal_1176.h"

#include "vdec_verify_general.h"
//#include <string.h>
//#include <stdio.h>
//#include <stdlib.h>

#include "vdec_verify_vparser_rm.h"
#include "vdec_verify_imgresz_rm.h"

#include "../hal/vdec_hw_rm.h"
#include "../hal/vdec_hal_if_rm.h"
#include <linux/string.h>


extern int rand(void);

void vRM_dscl (UINT32 u4InstID, int srcWidth, int srcHeight, int targetWidth, int targetHeight, 
        UCHAR *srcData, UCHAR *targetData, int component, int pic_stru,
        int tgbufLen, int h_ofst, int v_ofst,
        int src_agent, int spec_type, int mbaff, int fg_src,
	 int src_h_ofst, int src_v_ofst, int scl_h_ofst, int scl_v_ofst,
	 BOOL lk_en, UINT32 luma_key);
void vRM_GenerateDownScalerGolden(UINT32 u4InstID, UINT32 DecYAddr,UINT32 DecCAddr, UINT32 u4Size, 
	                                                                  BOOL lk_en, UINT32 luma_key);

//Add for DownScale Emulation
extern void vSetDownScaleParam(UINT32 u4InstID, BOOL fgEnable, VDEC_INFO_VDSCL_PRM_T *prDownScalerPrm);
extern void vGenerateDownScalerGolden(UINT32 u4InstID, UINT32 DecYAddr,UINT32 DecCAddr, UINT32 u4Size);
extern UINT32 u4CalculatePixelAddress_Y(
  UINT32 u4YBase,                           ///< [IN] frame buffer Y component address
  UINT32 u4XPos,                             ///< [IN] x position of pixel in frame buffer
  UINT32 u4YPos,                             ///< [IN] y position of pixel in frame buffer
  UINT32 u4FrameWidth,                 ///< [IN] Frame buffer width
  BOOL fgBlock,                               ///< [IN] MTK block / raster scan
  UCHAR bBlockW
);

extern UINT32 u4CalculatePixelAddress_C(
  UINT32 u4CBase,                           ///< [IN] frame buffer CbCr component address
  UINT32 u4XPos,                             ///< [IN] x position of pixel in frame buffer
  UINT32 u4YPos,                             ///< [IN] y position of pixel in frame buffer
  UINT32 u4FrameWidth,                 ///< [IN] Frame buffer width
  BOOL fgBlock,                               ///< [IN] MTK block / raster scan
  BOOL fg420,                                   ///< [IN] 420 / 422
  UCHAR bBlockW
);
extern void vFilledFBuf(UINT32 u4InstID, UCHAR *ptAddr, UINT32 u4Size);

extern void vVDSCLReset(UINT32 u4VDecID);
extern void vVDSCLSetEnable(UINT32 u4VDecID, BOOL fgEnable);
extern void vVDSCLSetAddrSwap(UINT32 u4VDecID, UINT32 u4Mode);
extern void vWriteVDSCL(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern void vVDSCLSetBoundPad(UINT32 u4VDecID, EPadMode ePadMode, UCHAR ucYColor, UCHAR ucCbColor, UCHAR ucCrColor);
extern void vVDSCLSetSrcPicSize(UINT32 u4VDecID, UINT32 u4SrcWidth, UINT32 u4SrcHeight);
extern void vVDSCLSetTrgBufWidth(UINT32 u4VDecID, UINT32 u4TrgBufWidth);
extern void vVDSCLSetTrgPicSize(UINT32 u4VDecID, UINT32 u4TrgWidth, UINT32 u4TrgHeight);
extern void vVDSCLSetTrgPicOffset(UINT32 u4VDecID, UINT32 u4TrgX, UINT32 u4TrgY);
extern void vVDSCLSetTrgYBufBaseAddr(UINT32 u4VDecID, UINT32 u4YBufBaseAddr);
extern void vVDSCLSetTrgCBufBaseAddr(UINT32 u4VDecID, UINT32 u4CBufBaseAddr);
extern void vVDSCLSetTmpBufBaseAddr(UINT32 u4VDecID, UINT32 u4TmpBufBaseAddr);
extern void vVDSCLSetScalerHParm(UINT32 u4VDecID, UINT32 u4SrcWidth, UINT32 u4TrgWidth, VDEC_INFO_VDSCL_PRM_T *prDownScalerPrm);
extern void vVDSCLSetSrcOffset(UINT32 u4VDecID, UINT32 u4SrcYOffH, UINT32 u4SrcYOffV, UINT32 u4SrcCOffH, UINT32 u4SrcCOffV);
extern void vVDSCLSetScalerVParm(UINT32 u4VDecID, UINT32 u4SrcHeight, UINT32 u4TrgHeight, BOOL fgIs422, BOOL fgIsMbaff, UINT32 u4OffY, UINT32 u4OffC, VDEC_INFO_VDSCL_PRM_T *prDownScalerPrm);
extern void vVDSCLSetConvert(UINT32 u4VDecID, BOOL fgConvert);
extern void vVDSCLSetPicStruct(UINT32 u4VDecID, EPicStruct ePicStruct);

//#define RM_SKIPFRAME_Test
#ifdef RM_SKIPFRAME_Test
//UINT32 u4SkipFrameNum  = 6506;    //Target Frame + 1
UINT32 u4SkipFrameNum  = 9890; //Target Frame + 1

UINT32 u4SkipCheckFrameNum = 0;
BOOL fgGetNextIPPic = FALSE;
//#define RM_DISABLE_NBM    //Test Only
#endif //RM_SKIPFRAME_Test


//#ifdef RM_ADDRSWAP_ENABLE
UINT8 auAddrSwapMapTable_RM[8] =
{
  4, 5, 6, 7, 0, 1, 2, 3
};

UINT8 auImgRszAddrSwapMapTable[8] =
{
  4, 5, 6, 7, 0, 1, 2, 3
};
//#endif //RM_ADDRSWAP_ENABLE


//#define RM_LOOPDECODE_FLOW      //Only for Debug
#ifdef RM_LOOPDECODE_FLOW
UINT32 u4LoopDecodeFrmCnt = 0;
//UINT32 u4LoopDecodeFrmCnt = 3910;
UINT32 u4LoopDecodeFirstFlag = 1;
UINT32 u4LoopDecodeCnt = 0;
UINT32 u4LoopDecodeTargetCnt = 4;
//UINT32 u4LoopDecodeTargetCnt = 4036;
#endif //RM_LOOPDECODE_FLOW


char strSrcFileNameExt_FileList[] = ".bin";
char strRMSrcFileName[] = "f";
char strGoldenFileNameExt_FileList[] = ".yuv";
char strRMGoldenFileName[] = "g";

UINT32 u4PredefineAlignSize = 0x38000;


// ==========================================================================================
//RM Proc 
void vRM_GetPicSize(UINT32 u4BSID, UINT32 u4VDecID, DWRD *dwPicWidth, DWRD *dwPicHeight)
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
  *dwPicWidth = dwWidth;
  *dwPicHeight = dwHieght;
}


BOOL fgRM_IsRMVDecComplete(UINT32 u4InstID)
{
  UINT32 u4MbX;
  UINT32 u4MbY;  
  UINT32 u4RealMbX;
  UINT32 u4RealMbY;
  VDEC_INFO_RM_PICINFO_T *prParsingPic;
  
  prParsingPic = (VDEC_INFO_RM_PICINFO_T*) &_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo;

  if(_fgVDecComplete[u4InstID])
  {
    vVDEC_HAL_RM_GetMbxMby(u4InstID, &u4MbX, &u4MbY);

    //check MbX, MbY
    u4RealMbX = (prParsingPic->u4Width /16);
    u4RealMbY = (prParsingPic->u4Height /16);

    if ((u4MbX == (u4RealMbX-1)) && 
    	  (u4MbY == (u4RealMbY-1)))
    {
      return TRUE;
    }
  }
  return FALSE;
}


//RM Proc 
// ==========================================================================================







// ===========================================================================================
//Get Picture Information for Emulation Only
inline UINT32 u4GetField(UINT32 u4Val, int b0, int b1)
{
    int i;
    UINT32 u4Mask = 0;
    
    for (i = b0; i <= b1; i++)
      u4Mask |= (1 << i);
    
    u4Val &= u4Mask;
    u4Val >>= b0;
    
    return u4Val;
}


BOOL fgGetRvPic(UINT32 u4InstID, const char *szFunction, INT32 i4Line)
{
    UINT32 u4Parm;
    int i;
    VDEC_INFO_RM_PICINFO_T *prPic;

	//printk("<vdec> RM[%d] FrmInfoRPtr=0x%08X (%u) @(%s, %d)\n",   _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt, 
	//	_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFrmInfoRPtr, _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFrmInfoRPtr,
	//	szFunction, i4Line);

    prPic = (VDEC_INFO_RM_PICINFO_T*) &_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMPicInfo;
    
    u4Parm = *(UINT32 *)(_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFrmInfoBuf +
    	                               _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFrmInfoRPtr);
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFrmInfoRPtr += 4;
    
    if (u4Parm == 0xFFFFFFFF)  // hit file end
      return FALSE;
    
    prPic->ePtype = (EnumRMPicCodType)u4GetField(u4Parm, 0, 1);
    prPic->u4OrgWidth = u4GetField(u4Parm, 2, 12);
    prPic->u4OrgHeight = u4GetField(u4Parm, 13, 23);
    prPic->u4Width = (prPic->u4OrgWidth + 15) / 16 * 16;
    prPic->u4Height = (prPic->u4OrgHeight + 15) / 16 * 16;
    prPic->u4PctszSize = u4GetField(u4Parm, 24, 25);
    prPic->u4Pctsz = u4GetField(u4Parm, 26, 28);
    prPic->u4Oquant = u4GetField(u4Parm, 29, 30);
    prPic->u4DFP = u4GetField(u4Parm, 31, 31);

    u4Parm = *(UINT32 *)(_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFrmInfoBuf +
    	                               _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFrmInfoRPtr);
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFrmInfoRPtr += 4;

    prPic->u4Tr = u4GetField(u4Parm, 0, 12);
    prPic->u4Mba = u4GetField(u4Parm, 13, 26);
    prPic->u4Rtype = u4GetField(u4Parm, 27, 27);
    prPic->fgRV9 = u4GetField(u4Parm, 28, 28);

    u4Parm = *(UINT32 *)(_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFrmInfoBuf +
    	                               _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFrmInfoRPtr);
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFrmInfoRPtr += 4;

    prPic->u4NumSlice = u4GetField(u4Parm, 0, 7);
    prPic->u4Pquant = u4GetField(u4Parm, 8, 12);
    prPic->u4MbaSize = u4GetField(u4Parm, 13, 16);
    prPic->u4HdrSkip = u4GetField(u4Parm, 17, 24);

    u4Parm = *(UINT32 *)(_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFrmInfoBuf +
    	                               _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFrmInfoRPtr);
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFrmInfoRPtr += 4;

    prPic->u4Iratio = u4Parm;

    prPic->u4BstLength = 0;
    
    for (i = 0; i < prPic->u4NumSlice; i++)
    {
      u4Parm = *(UINT32 *)(_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFrmInfoBuf +
    	                               _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFrmInfoRPtr);
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFrmInfoRPtr += 4;
    
      prPic->au4SliceSize[i] = u4Parm;
      prPic->u4BstLength += u4Parm;
    }
    
    return TRUE;
}


//Get Picture Information for Emulation Only
// ===========================================================================================
void vRM_DumpDownScalerParam(UINT32 u4InstID)
{
  VDEC_INFO_VDSCL_PRM_T *prDownScalerPrm = (VDEC_INFO_VDSCL_PRM_T *) &_tVerMpvDecPrm[u4InstID].rDownScalerPrm;

    printk("===== > DownScaler Parameter \n");
    printk("fgDSCLEn %x \n", prDownScalerPrm->fgDSCLEn);
    printk("fgEnColorCvt %x \n", prDownScalerPrm->fgEnColorCvt);
    printk("fgYOnly %x \n", prDownScalerPrm->fgYOnly);
    printk("fgMbaff %x \n", prDownScalerPrm->fgMbaff);
    printk("ucPicStruct %x \n", prDownScalerPrm->ucPicStruct);
    printk("ucScrAgent %x \n", prDownScalerPrm->ucScrAgent);
    printk("ucSpectType %x \n", prDownScalerPrm->ucSpectType);
    printk("ucScanType %x \n", prDownScalerPrm->ucScanType);
    printk("ucVdoFmt %x \n", prDownScalerPrm->ucVdoFmt);
    printk("ucAddrSwapMode %x \n", prDownScalerPrm->ucAddrSwapMode);
    printk("u4DispW %x \n", prDownScalerPrm->u4DispW);
    printk("u4SrcWidth %x \n", prDownScalerPrm->u4SrcWidth);
    printk("u4SrcHeight %x \n", prDownScalerPrm->u4SrcHeight);
    printk("u4SrcYOffH %x \n", prDownScalerPrm->u4SrcYOffH);
    printk("u4SrcYOffV %x \n", prDownScalerPrm->u4SrcYOffV);
    printk("u4SrcCOffH %x \n", prDownScalerPrm->u4SrcCOffH);
    printk("u4SrcCOffV %x \n", prDownScalerPrm->u4SrcCOffV);
    printk("u4TrgBufLen %x \n", prDownScalerPrm->u4TrgBufLen);
    printk("u4TrgWidth %x \n", prDownScalerPrm->u4TrgWidth);
    printk("u4TrgHeight %x \n", prDownScalerPrm->u4TrgHeight);
    printk("u4TrgOffH %x \n", prDownScalerPrm->u4TrgOffH);
    printk("u4TrgOffV %x \n", prDownScalerPrm->u4TrgOffV);
    printk("u4TrgYAddr %x \n", prDownScalerPrm->u4TrgYAddr);
    printk("u4TrgCAddr %x \n", prDownScalerPrm->u4TrgCAddr);
    printk("u4WorkAddr %x \n", prDownScalerPrm->u4WorkAddr);
    printk("u4QY %x \n", prDownScalerPrm->u4QY);
    printk("u4QC %x \n", prDownScalerPrm->u4QC);
    printk("u4R_norm %x \n", prDownScalerPrm->u4R_norm);
    printk("u4OffY %x \n", prDownScalerPrm->u4OffY);
    printk("u4OffC %x \n", prDownScalerPrm->u4OffC);
    printk("u4SclYOffH %x \n", prDownScalerPrm->u4SclYOffH);
    printk("u4SclCOffH %x \n", prDownScalerPrm->u4SclCOffH);
    printk("u4SclYOffV %x \n", prDownScalerPrm->u4SclYOffV);
    printk("u4SclCOffV %x \n", prDownScalerPrm->u4SclCOffV);
}



void vRM_SetDownScaleParam(UINT32 u4InstID, BOOL fgEnable, VDEC_INFO_VDSCL_PRM_T *prDownScalerPrm)
{ 
#ifdef VERIFICATION_DOWN_SCALE
  UINT32 dwPicWidthDec,dwPicHeightDec,u4DramPicSize;
  VDEC_INFO_RM_PICINFO_T *prParsingPic;   
  prParsingPic = (VDEC_INFO_RM_PICINFO_T*) &_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo;  

  dwPicWidthDec = prParsingPic->u4Width;
  dwPicHeightDec = prParsingPic->u4Height;
  //dwPicWidthDec = prParsingPic->u4OrgWidth;
  //dwPicHeightDec = prParsingPic->u4OrgHeight;
  //dwPicWidthDec = _tVerMpvDecPrm[u4InstID].u4PicW;
  //dwPicHeightDec = _tVerMpvDecPrm[u4InstID].u4PicH;

  if(fgEnable)
  {
    prDownScalerPrm->fgMbaff  = FALSE;
    prDownScalerPrm->fgDSCLEn = TRUE;    
#if (!CONFIG_DRV_MT8520)
    #ifdef RM_ADDRSWAP_ENABLE
    #ifdef RM_ADDRSWAP_DSCL_ENABLE
    prDownScalerPrm->ucAddrSwapMode = auAddrSwapMapTable_RM[_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMAddrSwapMode];
    #else //RM_ADDRSWAP_DSCL_ENABLE
    prDownScalerPrm->ucAddrSwapMode = _tVerMpvDecPrm[u4InstID].ucAddrSwapMode ^ 0x4;
    #endif //RM_ADDRSWAP_DSCL_ENABLE
    #else //RM_ADDRSWAP_ENABLE
    prDownScalerPrm->ucAddrSwapMode = _tVerMpvDecPrm[u4InstID].ucAddrSwapMode ^ 0x4;
    #endif //RM_ADDRSWAP_ENABLE
#else
    prDownScalerPrm->ucAddrSwapMode = ADDRSWAP_OFF;
#endif

    //Only for RM 
    if(_u4CodecVer[u4InstID] == VDEC_RM)
    {
      #ifdef RM_INTERLACEFRAME_DSCL_TEST
      prDownScalerPrm->ucPicStruct = TOP_BOTTOM_FIELD;
      #else //RM_INTERLACEFRAME_DSCL_TEST
      prDownScalerPrm->ucPicStruct = FRAME;
      #endif //RM_INTERLACEFRAME_DSCL_TEST

      if (prParsingPic->fgDeblockPassThru == FALSE)
      {
        #ifdef RM_MCOutPut_ENABLE
        if (prParsingPic->fgRV9 == FALSE)
        {
          //RV8 => Sync with HW Behavior
          prDownScalerPrm->ucScrAgent = RW_VDSCL_SRC_MC >> 2;
          prDownScalerPrm->fgEnColorCvt = (BOOL) (((UINT32) rand())&0x1);
        }
        else
        #endif //RM_MCOutPut_ENABLE
        {
          //RV9, RV10
          prDownScalerPrm->ucScrAgent = RW_VDSCL_SRC_PP >> 2;
          prDownScalerPrm->fgEnColorCvt = FALSE;
        }
      }
      else
      {
        prDownScalerPrm->ucScrAgent = RW_VDSCL_SRC_MC >> 2;

        #ifdef NO_SCALE_TEST_ENABLE
        prDownScalerPrm->fgEnColorCvt = 0;
        #else //NO_SCALE_TEST_ENABLE
        prDownScalerPrm->fgEnColorCvt = (BOOL) (((UINT32) rand())&0x1);
        #endif //NO_SCALE_TEST_ENABLE
      }
    }
    else
    {
      ASSERT(0);
    }
    
    if((prDownScalerPrm->ucPicStruct == TOP_FIELD) || (prDownScalerPrm->ucPicStruct == BOTTOM_FIELD))
    {
      dwPicHeightDec = (dwPicHeightDec >> 1);
    }

    //if(((_u4CodecVer[u4InstID] == VDEC_WMV)&&(_rWMVPPS[u4InstID].i4CurrentTemporalField==0))||
    //  ((_u4CodecVer[u4InstID] == VDEC_H264)&&(fgIsDecFlagSet(u4InstID, DEC_FLAG_CHG_FBUF)))||
    //  ((!_fgDec2ndFldPic[u4InstID])&&(_u4CodecVer[u4InstID] != VDEC_WMV)&&(_u4CodecVer[u4InstID] != VDEC_H264)))
    {
#if 0    
      if(_u4CodecVer[u4InstID] == VDEC_RM)
      {
        prDownScalerPrm->ucSpectType = RW_VDSCL_SPEC_MPEG >> 5;
        prDownScalerPrm->fgYOnly = 0;
        
        if (prParsingPic->fgDeblockPassThru == FALSE)
        {
          prDownScalerPrm->ucScrAgent = RW_VDSCL_SRC_PP >> 2;
          prDownScalerPrm->fgEnColorCvt = FALSE;
        }
        else
        {
          prDownScalerPrm->ucScrAgent = RW_VDSCL_SRC_MC >> 2;

          #ifdef NO_SCALE_TEST_ENABLE
          prDownScalerPrm->fgEnColorCvt = 0;
          #else //NO_SCALE_TEST_ENABLE
          prDownScalerPrm->fgEnColorCvt = (BOOL) (rand()&0x1);
          #endif //NO_SCALE_TEST_ENABLE
        }
      }
#endif //0      
        
      prDownScalerPrm->u4SrcHeight = dwPicHeightDec;    
      prDownScalerPrm->u4SrcWidth = dwPicWidthDec;    

      prDownScalerPrm->u4SrcYOffH = 0;    
      prDownScalerPrm->u4SrcYOffV = 0;    
      prDownScalerPrm->u4SrcCOffH = 0;    
      prDownScalerPrm->u4SrcCOffV = 0; 
      prDownScalerPrm->u4SclYOffH = 0;    
      prDownScalerPrm->u4SclYOffV = 0;    
      prDownScalerPrm->u4SclCOffH = 0;    
      prDownScalerPrm->u4SclCOffV = 0; 
      
  
      if(_fgVDSCLEnableRandomTest[u4InstID])
      {
        #ifdef NO_SCALE_TEST_ENABLE
        prDownScalerPrm->ucScanType = 0;
        #else //NO_SCALE_TEST_ENABLE
        prDownScalerPrm->ucScanType = (UCHAR) (((UINT32) rand())&0x1);
        #endif //NO_SCALE_TEST_ENABLE
        
        prDownScalerPrm->ucVdoFmt = 0;
        //prDownScalerPrm->ucVdoFmt = (UCHAR) (rand()&0x1);

        while(TRUE)
        {
          #ifdef NO_SCALE_TEST_ENABLE
          prDownScalerPrm->u4TrgWidth = dwPicWidthDec;
          prDownScalerPrm->u4TrgHeight = dwPicHeightDec;
          #else //NO_SCALE_TEST_ENABLE
          prDownScalerPrm->u4TrgWidth = (UINT32) (((UINT32) rand())%RM_MAX_DSCL_WIDTH);
          //prDownScalerPrm->u4TrgWidth = (UINT32) (rand()%dwPicWidthDec)+0x10;
          prDownScalerPrm->u4TrgHeight = (UINT32) (((UINT32) rand())%dwPicHeightDec)+0x10;
          #endif //NO_SCALE_TEST_ENABLE
          
          if((prDownScalerPrm->u4TrgHeight>=8)&&(prDownScalerPrm->u4TrgWidth>=8)&&((prDownScalerPrm->u4TrgWidth%2)==0)&&((prDownScalerPrm->u4TrgHeight%4)==0)
          &&(prDownScalerPrm->u4TrgWidth <= dwPicWidthDec)&&(prDownScalerPrm->u4TrgHeight <= (dwPicHeightDec>>(prDownScalerPrm->ucVdoFmt))))
          {
            if(prDownScalerPrm->u4TrgWidth == dwPicWidthDec)
            {
              prDownScalerPrm->u4TrgOffH = 0;
            }
            else
            {
              prDownScalerPrm->u4TrgOffH = (((((UINT32) rand())%(dwPicWidthDec-prDownScalerPrm->u4TrgWidth))>>1)<<1);
            }
            if(prDownScalerPrm->u4TrgHeight == (dwPicHeightDec>>(prDownScalerPrm->ucVdoFmt)))
            {
              prDownScalerPrm->u4TrgOffV = 0;
            }
            else
            {
              prDownScalerPrm->u4TrgOffV = (((((UINT32) rand())%((dwPicHeightDec-prDownScalerPrm->u4TrgHeight)))>>2)<<2);
            }
            break;
          }
        }

        #ifdef NO_SCALE_TEST_ENABLE
        prDownScalerPrm->u4SrcYOffV = 0;
        prDownScalerPrm->u4SrcCOffV = 0;
        #else //NO_SCALE_TEST_ENABLE
        if(prDownScalerPrm->ucScrAgent == (RW_VDSCL_SRC_PP >> 2))
        {
          if(prDownScalerPrm->ucSpectType == (RW_VDSCL_SPEC_WMV >> 5))
          {
            if(prDownScalerPrm->ucPicStruct == TOP_BOTTOM_FIELD)
            {
              prDownScalerPrm->u4SrcYOffV = (UINT32)((((UINT32) rand())%3)*2);
              prDownScalerPrm->u4SrcCOffV = (UINT32)((((UINT32) rand())%3)*2);
            }
            else
            {
              prDownScalerPrm->u4SrcYOffV = (UINT32)(((UINT32) rand())%5);
              prDownScalerPrm->u4SrcCOffV = (UINT32)(((UINT32) rand())%5);
            }
          }
          else//h264
          {
            if(prDownScalerPrm->ucPicStruct == TOP_BOTTOM_FIELD)
            {
              prDownScalerPrm->u4SrcYOffV = (UINT32)((((UINT32) rand())%4)*2);
              prDownScalerPrm->u4SrcCOffV = (UINT32)((((UINT32) rand())%2)*2);
            }
            else
            {
              prDownScalerPrm->u4SrcYOffV = (UINT32)(((UINT32) rand())%7);
              prDownScalerPrm->u4SrcCOffV = (UINT32)(((UINT32) rand())%4);
            }
          }
        }
        else
        {
          if(prDownScalerPrm->ucPicStruct == TOP_BOTTOM_FIELD)
          {
            prDownScalerPrm->u4SrcYOffV = (UINT32)((((UINT32) rand())%5)*2);
            prDownScalerPrm->u4SrcCOffV = (UINT32)((((UINT32) rand())%3)*2);
          }
          else
          {
            prDownScalerPrm->u4SrcYOffV = (UINT32)(((UINT32) rand())%9);
            prDownScalerPrm->u4SrcCOffV = (UINT32)(((UINT32) rand())%5);
          }
        }
        #endif //NO_SCALE_TEST_ENABLE
        
        if(prDownScalerPrm->fgEnColorCvt)
        {
          prDownScalerPrm->u4SrcYOffH = prDownScalerPrm->u4SrcYOffH - (prDownScalerPrm->u4SrcYOffH%2);
          prDownScalerPrm->u4SrcYOffV = prDownScalerPrm->u4SrcYOffV - (prDownScalerPrm->u4SrcYOffV%2);
          prDownScalerPrm->u4SrcCOffH = prDownScalerPrm->u4SrcCOffH - (prDownScalerPrm->u4SrcCOffH%2);
          prDownScalerPrm->u4SrcCOffV = prDownScalerPrm->u4SrcCOffV - (prDownScalerPrm->u4SrcCOffV%2);
        }
        if(prDownScalerPrm->fgMbaff)
        {
          prDownScalerPrm->u4SrcYOffH = 0;
          prDownScalerPrm->u4SrcYOffV = 0;
          prDownScalerPrm->u4SrcCOffH = 0;
          prDownScalerPrm->u4SrcCOffV = 0;
        }

        #ifdef NO_SCALE_TEST_ENABLE
        prDownScalerPrm->u4DispW = dwPicWidthDec;
        #else //NO_SCALE_TEST_ENABLE
        if((prDownScalerPrm->u4TrgWidth + prDownScalerPrm->u4TrgOffH) < 1920 )
        {
          prDownScalerPrm->u4DispW = ((((prDownScalerPrm->u4TrgWidth + prDownScalerPrm->u4TrgOffH  + (((UINT32) rand())%(1920 - prDownScalerPrm->u4TrgWidth - prDownScalerPrm->u4TrgOffH  )))+15)>>4)<<4);       
        }
        else
        {
          prDownScalerPrm->u4DispW = 1920;
        }      
        #endif //NO_SCALE_TEST_ENABLE
        prDownScalerPrm->u4TrgBufLen = prDownScalerPrm->u4DispW;      
      }
            
      prDownScalerPrm->u4TrgYAddr = (UINT32)(_pucVDSCLBuf[u4InstID]);
      u4DramPicSize = 0x1FE000;
      prDownScalerPrm->u4TrgCAddr = prDownScalerPrm->u4TrgYAddr + u4DramPicSize;
      prDownScalerPrm->u4WorkAddr = (UINT32)(_pucVDSCLWorkBuf[u4InstID]);


      #ifdef FIX_DSCL_PARAM_SETTING
      prDownScalerPrm->fgDSCLEn = TRUE;
      prDownScalerPrm->fgEnColorCvt = FALSE;  
      prDownScalerPrm->fgYOnly = FALSE;
      prDownScalerPrm->fgMbaff = FALSE;
      prDownScalerPrm->ucPicStruct = TOP_BOTTOM_FIELD;
      //prDownScalerPrm->ucScrAgent = ;
      prDownScalerPrm->ucSpectType = RW_VDSCL_SPEC_MPEG >> 5;
      prDownScalerPrm->ucScanType = 1; 
      prDownScalerPrm->ucVdoFmt = 0;
      prDownScalerPrm->ucAddrSwapMode = 0;
      prDownScalerPrm->u4DispW = 0x1B0;
      prDownScalerPrm->u4SrcWidth = 0xB0;
      prDownScalerPrm->u4SrcHeight = 0x90;
      prDownScalerPrm->u4SrcYOffH = 0;
      prDownScalerPrm->u4SrcYOffV = 2;
      prDownScalerPrm->u4SrcCOffH = 0;
      prDownScalerPrm->u4SrcCOffV = 2;
      prDownScalerPrm->u4TrgBufLen = 0x1B0;
      prDownScalerPrm->u4TrgWidth = 0x10;
      prDownScalerPrm->u4TrgHeight = 0x18;
      prDownScalerPrm->u4TrgOffH = 0xC;
      prDownScalerPrm->u4TrgOffV = 0x2C;
      prDownScalerPrm->u4SclYOffH = 0;
      prDownScalerPrm->u4SclCOffH = 0;
      prDownScalerPrm->u4SclYOffV = 0xAAA;
      prDownScalerPrm->u4SclCOffV = 0xAAA;
      #endif //FIX_DSCL_PARAM_SETTING


      #ifdef RM_INTERLACEFRAME_DSCL_TEST
      {
        UINT32 u4Offset = 0;

        u4Offset = (prDownScalerPrm->u4SrcHeight - prDownScalerPrm->u4TrgHeight) / (prDownScalerPrm->u4TrgHeight * 2);

        if (u4Offset > 15)
        {
          prDownScalerPrm->u4SrcYOffV = 15;
        }
        else
        {
          prDownScalerPrm->u4SrcYOffV  = u4Offset;
        }
        
        prDownScalerPrm->u4SrcYOffH = 0;
        prDownScalerPrm->u4SrcCOffH = 0;
        prDownScalerPrm->u4SrcCOffV = prDownScalerPrm->u4SrcYOffV;
      }
      #endif //RM_INTERLACEFRAME_DSCL_TEST

      vFilledFBuf(u4InstID, _pucVDSCLBuf[u4InstID], u4DramPicSize);
      //vFilledFBuf(u4InstID, _pucVDSCLBuf[u4InstID], _ptCurrFBufInfo[u4InstID]->u4DramPicSize);
    }
  }
  else
  {
    prDownScalerPrm->fgDSCLEn = FALSE;        
  } 
#endif
}


void vRM_VDECSetDownScalerPrm(UINT32 u4VDecID,VDEC_INFO_VDSCL_PRM_T *prDownScalerPrm)
{
#if (CONFIG_DRV_MT8520)
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
        vVDSCLSetTrgYBufBaseAddr(u4VDecID, PHYSICAL(prDownScalerPrm->u4TrgYAddr));
        vVDSCLSetTrgCBufBaseAddr(u4VDecID, PHYSICAL(prDownScalerPrm->u4TrgCAddr));
        vVDSCLSetTmpBufBaseAddr(u4VDecID, PHYSICAL(prDownScalerPrm->u4WorkAddr));
        //vVDSCLSetTrgYBufBaseAddr(u4VDecID, u4AbsDramANc(prDownScalerPrm->u4TrgYAddr));
        //vVDSCLSetTrgCBufBaseAddr(u4VDecID, u4AbsDramANc(prDownScalerPrm->u4TrgCAddr));
        //vVDSCLSetTmpBufBaseAddr(u4VDecID, u4AbsDramANc(prDownScalerPrm->u4WorkAddr));
#if(CONFIG_DRV_MT8520)         
        vVDSCLSetScalerHParm(u4VDecID, prDownScalerPrm->u4SrcWidth, prDownScalerPrm->u4TrgWidth);
#else
        vVDSCLSetScalerHParm(u4VDecID, prDownScalerPrm->u4SrcWidth, prDownScalerPrm->u4TrgWidth, prDownScalerPrm);
#endif        

#if (CONFIG_DRV_MT8520)
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

        #ifdef RM_INTERLACEFRAME_DSCL_TEST
        {
          UINT32 u4Factor = 0;
          UINT32 u4Scl_Offset = 0;
          UINT32 u4RegValue = 0;
          UINT32 u4Offset = 0;

          u4Offset = (prDownScalerPrm->u4SrcHeight - prDownScalerPrm->u4TrgHeight) / (prDownScalerPrm->u4TrgHeight * 2);

          u4Factor = (0x8000 * prDownScalerPrm->u4TrgHeight + prDownScalerPrm->u4SrcHeight/2) / prDownScalerPrm->u4SrcHeight;
          u4Scl_Offset = (((prDownScalerPrm->u4SrcHeight - prDownScalerPrm->u4TrgHeight) - (u4Offset * prDownScalerPrm->u4TrgHeight *2)) * 0x8000) / (2 * prDownScalerPrm->u4SrcHeight);
          //u4Scl_Offset = (((prDownScalerPrm->u4SrcHeight - prDownScalerPrm->u4TrgHeight) - (prDownScalerPrm->u4SrcYOffV * prDownScalerPrm->u4TrgHeight *2)) * 0x8000) / (2 * prDownScalerPrm->u4SrcHeight);

          u4RegValue = (u4Scl_Offset << 16) | u4Factor;

          prDownScalerPrm->u4SclYOffV = u4Scl_Offset;
          prDownScalerPrm->u4SclCOffV = u4Scl_Offset;

          vWriteVDSCL(u4VDecID, RW_VDSCL_V_SCL_Y, u4RegValue);
          vWriteVDSCL(u4VDecID, RW_VDSCL_V_SCL_C, u4RegValue);
        }
        #else //RM_INTERLACEFRAME_DSCL_TEST
        vVDSCLSetScalerVParm(u4VDecID, prDownScalerPrm->u4SrcHeight, prDownScalerPrm->u4TrgHeight, \
        	                                 prDownScalerPrm->ucVdoFmt, prDownScalerPrm->fgMbaff, \
        	                                 prDownScalerPrm->u4OffY, prDownScalerPrm->u4OffC, prDownScalerPrm);
        #endif //RM_INTERLACEFRAME_DSCL_TEST
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
    }  


    #ifdef DSCL_LUMAKEY_ENABLE
    {
      UINT32 u4LumaKeyReg = 0;

      u4LumaKeyReg = (0x110001) | (u4DSCLLumaKey << 8);
      vWriteVDSCL(u4VDecID, 29*4, u4LumaKeyReg);
    }
    #endif //DSCL_LUMAKEY_ENABLE
    
  
    return;
}


void vRM_VerInitDec(UINT32 u4InstID)
{
  //Check RM Define
  #ifdef MT8550_DDR3_VDEC_ENABLE
  printk("[Vdec RM] Enable DDR3 \n");
  #endif //MT8550_DDR3_VDEC_ENABLE

  #ifdef MT8550_ADDRSWAP_ENABLE
  printk("[Vdec RM] Enable Addr Swap \n");
  #endif //MT8550_ADDRSWAP_ENABLE
  
  #ifdef RM_NEW_PPOUT_MODE
  printk("<vdec> [Vdec RM] Enable PP Out \n");
  #endif //RM_NEW_PPOUT_MODE

  #ifdef DOWN_SCALE_SUPPORT
  printk("[Vdec RM] Enable DSCL \n");
  #endif //DOWN_SCALE_SUPPORT

  // 1920*1088*16/16/16 = 130560
  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4MvHwWorkBuf = (UINT32) _pucRMMvHwWorkBuf[u4InstID];

  // 3*128*128 = 49152
  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4VldPredWorkBuf = (UINT32) _pucRmVldPredWorkBuf[u4InstID];

  //Init Read Ptr
  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFrmInfoBuf = (UINT32) _pucRMFrmInfoBuf[u4InstID];
  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFrmInfoRPtr = 0;

  //Golden Data Info
  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMGoldenDataBuf = (UINT32) _pucRMGoldenDataBuf[u4InstID];
  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMGoldenDataBufRPtr = 0;
  //Init Decoded Pic Cnt
  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt = 0;

  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMRingFlowTempFIFO = (UINT32) _pucRMRingWorkBuf[u4InstID];
  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMRingFlowTempFIFOSize = (UINT32) RM_RINGFLOW_TEMPFIFO_SZ;


  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4GlobalChecksumBuf = (UINT32) _pucRMChecksumBuf[u4InstID];
  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4GlobalChecksumBufSize = RM_CHECKSUM_BUFFER_SZ;
  

  #ifdef RM_ATSPEED_TEST_ENABLE
    
  #ifndef RM_RINGVIFO_FLOW
  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMVFIFOWPtr = (UINT32) _pucRMAULikeBuf[u4InstID];
  #endif //RM_RINGVIFO_FLOW
  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFIFORPtr = (UINT32) _pucRMAULikeBuf[u4InstID];
  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMVFIFOSa = (UINT32) _pucRMAULikeBuf[u4InstID];
  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMVFIFOSz = (UINT32) RM_AULIKEBUF_SZ;
  
  #else //RM_ATSPEED_TEST_ENABLE
  
  #ifndef RM_RINGVIFO_FLOW
  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMVFIFOWPtr = (UINT32) _pucVFifo[u4InstID];
  #endif //RM_RINGVIFO_FLOW
  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFIFORPtr = (UINT32) _pucVFifo[u4InstID];
  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMVFIFOSa = (UINT32) _pucVFifo[u4InstID];
  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMVFIFOSz = (UINT32) V_FIFO_SZ;
  
  #endif //RM_ATSPEED_TEST_ENABLE

  #ifdef RM_ADDRSWAP_ENABLE
  {
  UINT32 u4RandomMode = 0;

  #ifdef RM_RANDOM_ADDRSWAP_ENABLE
  u4RandomMode = rand();
  u4RandomMode = u4RandomMode % 6;
  while ((u4RandomMode == 3) || (u4RandomMode == 4))
  {
    u4RandomMode = rand();
    u4RandomMode = u4RandomMode % 6;
  }

  #ifdef RM_DDR3MODE_ENABLE
  ASSERT(0);    //DDR3 Settings only for ADDR Swap2
  #endif //RM_DDR3MODE_ENABLE
  
  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMAddrSwapMode = u4RandomMode;
  #else //RM_RANDOM_ADDRSWAP_ENABLE
  
  #ifdef RM_DDR3MODE_ENABLE
  u4RandomMode = 2;
  #else //RM_DDR3MODE_ENABLE
  //u4RandomMode = 0;      //5351 Mode0
  //u4RandomMode = 1;      //5351 Mode1
  u4RandomMode = 2;    //5351 Mode2
  #endif //RM_DDR3MODE_ENABLE
  
  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMAddrSwapMode = u4RandomMode;
  #endif //RM_RANDOM_ADDRSWAP_ENABLE
  }
  #endif //RM_ADDRSWAP_ENABLE

  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4CRCResultBufAddr = (UINT32) _pucRMCRCResultBuf[u4InstID];
  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4CRCCheckFrameNumber = 0;
  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.fgCRCLoadGoldenFlag = TRUE;
}

UINT32 u4RM_FindIPic(UINT32 u4InstID, UINT32 u4EndFrm)
{
	UINT32 i;
	UINT32 u4IFrmIdx = 0;
    VDEC_INFO_RM_PICINFO_T *prPic;

    vRM_VerInitDec(u4InstID);

    printk("<vdec> pre-parsing I pic from 0 to %u...\n", u4EndFrm);
    prPic = (VDEC_INFO_RM_PICINFO_T*) &_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMPicInfo;

	for (i = 0; i < u4EndFrm; i++)
	{
	   fgGetRvPic(u4InstID, __FUNCTION__, __LINE__);
	   if (prPic->ePtype == RM_INTRAPIC)
   	   {
   	       u4IFrmIdx = i;
	   }
	   _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt++;
	}

    printk("<vdec> %s(%u, %u) return %u.\n", __FUNCTION__, u4InstID, u4EndFrm, u4IFrmIdx);

    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt = u4IFrmIdx;

    return u4IFrmIdx;
}

UINT32 u4RM_PreParseIPic(UINT32 u4InstID, UINT32 u4EndFrm)
{
	UINT32 i;

	for (i = 0; i < u4EndFrm; i++)
	{
	   fgGetRvPic(u4InstID, __FUNCTION__, __LINE__);
	   _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt++;
    }

    return u4EndFrm;
}

void vRM_TriggerDecode(UINT32 u4InstID, VDEC_INFO_RM_PICINFO_T *prParsingPic)
{
#ifdef VERIFICATION_DOWN_SCALE
  #ifdef DOWN_SCALE_SUPPORT
   vRM_SetDownScaleParam(u4InstID, TRUE, &(_tVerMpvDecPrm[u4InstID].rDownScalerPrm));
  #else //DOWN_SCALE_SUPPORT
    vRM_SetDownScaleParam(u4InstID, FALSE, &(_tVerMpvDecPrm[u4InstID].rDownScalerPrm));
  #endif //DOWN_SCALE_SUPPORT
  vRM_VDECSetDownScalerPrm(u4InstID, &(_tVerMpvDecPrm[u4InstID].rDownScalerPrm));
#endif //VERIFICATION_DOWN_SCALE
  
  //Log Input Window before Trigger VDec
  {
    UINT32 u4InputWindow = 0;

    u4InputWindow = u4VDecReadRMVLD(u4InstID, 0x0);

    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4HWInputWindow = u4InputWindow;
    
    //printk("# %d , Input Window %x \n", _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt, u4InputWindow);
  }
  
  #ifdef RM_MCOutPut_ENABLE
  if ((_pucRMMCOutputBufY[u4InstID] == NULL) ||
  	(_pucRMMCOutputBufC[u4InstID] == NULL))
  {
    printk("MC Output Mode - Buffer Addr Error \n");
    ASSERT(0);
  }
  #endif //RM_MCOutPut_ENABLE
  
 prParsingPic->u4AddrSwapMode = 
 	  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMAddrSwapMode;

 prParsingPic->fgEnableCRC = FALSE;
 #ifndef RM_DISABLE_HWCRCCHECK
 #ifdef RM_CRCCHECKFLOW_SUPPORT
 #ifdef RM_CRCCHECK_ENABLE
 prParsingPic->fgEnableCRC = TRUE;
 #endif //RM_CRCCHECK_ENABLE
 #endif //RM_CRCCHECKFLOW_SUPPORT
 #endif //RM_DISABLE_HWCRCCHECK
 
 prParsingPic->fgEnableMCOutput = FALSE;
 #ifdef RM_MCOutPut_ENABLE
 prParsingPic->fgEnableMCOutput = TRUE;
 #endif //RM_MCOutPut_ENABLE
 
 prParsingPic->fgEnableDDR3 = FALSE;
 #ifdef RM_DDR3MODE_ENABLE
 prParsingPic->fgEnableDDR3 = TRUE;
 #endif //RM_DDR3MODE_ENABLE

 
 prParsingPic->fgEnableAddrSwap = FALSE;
 #ifdef RM_ADDRSWAP_ENABLE
 prParsingPic->fgEnableAddrSwap = TRUE;
 #endif //RM_ADDRSWAP_ENABLE

 prParsingPic->fgEnablePPOut  = FALSE;
 #ifdef RM_NEW_PPOUT_MODE
 prParsingPic->fgEnablePPOut = TRUE;
 #endif //RM_NEW_PPOUT_MODE
 

 #if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8560)
 vVDEC_HAL_RM_TriggerDecode(u4InstID, prParsingPic);
 #else
 vVDEC_HAL_RM_TriggerDecode(u4InstID, prParsingPic, NULL); 
 #endif
}

extern UINT32 u4VDEC_HAL_VP6_VDec_SetWorkspace(UINT32 u4VDecID, UINT32 u4VLDWrapperWorkspace, UINT32 u4PPWrapperWorkspace);

//RM Verify Function
void vRM_VParser(UINT32 u4InstID) 
{
  VDEC_INFO_RM_BS_INIT_PRM_T rRMVdecBSInfo; 
  VDEC_INFO_RM_PICINFO_T *prPic;   
  VDEC_INFO_RM_PICINFO_T *prParsingPic;   
  UINT32 u4RefQpMb0 = 0;
  UINT32 u4DecodedPicCnt = 0;
#if 0
  UINT32 u4RegValue = 0;
  UINT32 u4PpRvPara = 0;
  UINT32 u4Rv8TblNum  =1;
  UINT32 i;

  UINT32 u4CycleCnt = 0x00800000;
  UINT32 u4ErrConceal = 0;
  UINT32 u4FastParsing = 0x40000000;
#endif //0

 // HalFlushInvalidateDCache();
  
//VDec HW Init Flow  //////////////////////////////////////////////////////////
#if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8555)
  vVDecResetHW(u4InstID);
#else
    vVDecResetHW(u4InstID, VDEC_UNKNOWN);
#endif
  
  //Init RM VDec HW
  vRM_Hal_VldSoftReset(u4InstID);

  vRM_Hal_MvInit(u4InstID, _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4MvHwWorkBuf);

  vRM_Hal_McInit(u4InstID);

  vRM_Hal_PpInit(u4InstID);

  //Init RM BS HW
  vRM_Hal_VldInit(u4InstID, _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4VldPredWorkBuf);
  
  //rRMVdecBSInfo.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
  //rRMVdecBSInfo.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
  rRMVdecBSInfo.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
  rRMVdecBSInfo.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;

  #ifdef RM_RINGVIFO_FLOW
  rRMVdecBSInfo.u4ReadPointer= _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFIFORPtr;
  #else //RM_RINGVIFO_FLOW
  rRMVdecBSInfo.u4ReadPointer= (UINT32)_pucVFifo[u4InstID];
  #endif //RM_RINGVIFO_FLOW

  #ifdef RM_RINGVIFO_FLOW
  rRMVdecBSInfo.u4WritePointer = (UINT32) (_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMVFIFOWPtr + 1024);
  #else
  rRMVdecBSInfo.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;;
  #endif //RM_RINGVIFO_FLOW
/*  
#ifndef  RING_VFIFO_SUPPORT
  rRMVdecBSInfo.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
#else
  rRMVdecBSInfo.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ*(0.5 + 0.5 *(_u4LoadBitstreamCnt[u4InstID]%2)));
#endif
*/
  i4RM_HAL_InitBarrelShifter(_u4BSID[u4InstID], u4InstID, &rRMVdecBSInfo);
  u4VDEC_HAL_VP6_VDec_SetWorkspace(u4InstID, (UINT32)_pucVP6VLDWrapperWorkspace[u4InstID], (UINT32)_pucVP6PPWrapperWorkspace[u4InstID]);



  #ifdef RM_RPR_RACINGMODE_ENABLE
  //Pre-Parsing Picture Information
  {
    UINT32 u4BackupFrmInfoRPtr = 0;
    VDEC_INFO_RM_PICINFO_T *prPic_Preparsing;
    VDEC_INFO_RM_PICINFO_T *prPic_Normal;
    UINT32 u4SearchLoop;
    
    prPic_Preparsing = (VDEC_INFO_RM_PICINFO_T*) &_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMPicInfo_Preparsing;
    prPic_Normal = (VDEC_INFO_RM_PICINFO_T*) &_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMPicInfo;
    u4BackupFrmInfoRPtr = _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFrmInfoRPtr;

    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.fgRPRRacingModeEnable = FALSE;

    //Parsing First Pic Info
    fgGetRvPic(u4InstID, __FUNCTION__, __LINE__);

    memcpy((void*) (&_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMPicInfo_Preparsing), 
    	                (void*) (&_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMPicInfo),
    	                sizeof(VDEC_INFO_RM_PICINFO_T));
    if ((prPic_Preparsing->ePtype == RM_INTRAPIC) ||
    	  (prPic_Preparsing->ePtype == RM_FORCED_INTRAPIC) ||
    	  (prPic_Preparsing->ePtype == RM_INTERPIC))
    {
      //Search Next I or  P
      for (u4SearchLoop=0; u4SearchLoop< RPR_RAC_PREPARSING_CNT; u4SearchLoop++)
      {
        if (fgGetRvPic(u4InstID, __FUNCTION__, __LINE__) == FALSE)
        {
          break;
        }

        if ((prPic_Normal->ePtype == RM_INTRAPIC) ||
             (prPic_Normal->ePtype == RM_FORCED_INTRAPIC))
        {
          //Stop Preparsing
          break;
        }

        //Check Pic W/H
        if (prPic_Normal->ePtype == RM_INTERPIC)
        {
          if ((prPic_Normal->u4OrgWidth != prPic_Preparsing->u4OrgWidth) ||
               (prPic_Normal->u4OrgHeight != prPic_Preparsing->u4OrgHeight))
          {
            printk("<vdec> RPR Racing Mode Enalbe !!!!!!!!!!!!!!!\n");
            //Enable RPR Racing Mode
            _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.fgRPRRacingModeEnable = TRUE;
          
            memcpy((void*) (&_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMPicInfo_Preparsing), 
    	                         (void*) (&_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMPicInfo),
    	                         sizeof(VDEC_INFO_RM_PICINFO_T));
          }
          
          break;
        }
      }
    }

    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFrmInfoRPtr = u4BackupFrmInfoRPtr;
  }
  #endif //RM_RPR_RACINGMODE_ENABLE

  

// Get Golden Syntax Pic Info
  #ifdef RM_LOOPDECODE_FLOW
  {
    UINT32 u4Loop;

    if (u4LoopDecodeFirstFlag)
    {
      if (u4LoopDecodeFrmCnt > 0)
      {
        _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt = u4LoopDecodeFrmCnt-1;
        for (u4Loop=0; u4Loop < u4LoopDecodeFrmCnt; u4Loop++)
        {
          fgGetRvPic(u4InstID);
        }
      }
      else
      {
        _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt = 0;
        fgGetRvPic(u4InstID);
      }
      u4LoopDecodeFirstFlag = 0;
    }
    else
    {
      if (u4LoopDecodeTargetCnt >= u4LoopDecodeFrmCnt)
      {
        fgGetRvPic(u4InstID);
        //u4LoopDecodeFrmCnt++;
      }
      else
      {
        //Re-Decode Start
        u4LoopDecodeCnt++;
      }
    }
  }
  #else //RM_LOOPDECODE_FLOW
  //Parsing Current Picture Information
  #ifdef RM_SKIPFRAME_Test
  {
    UINT32 u4SkipIdx = 0;

    if (u4SkipFrameNum > 0)
    {
      for (u4SkipIdx=0; u4SkipIdx< u4SkipFrameNum; u4SkipIdx++)
      {
        fgGetRvPic(u4InstID);
      }

      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt = u4SkipFrameNum -1;

      u4SkipCheckFrameNum = u4SkipFrameNum - 1;
      u4SkipFrameNum = 0;
    }
    else
    {
      fgGetRvPic(u4InstID);
    }
  }
  #else //RM_SKIPFRAME_Test
  fgGetRvPic(u4InstID, __FUNCTION__, __LINE__);
  #endif //RM_SKIPFRAME_Test
  #endif //RM_LOOPDECODE_FLOW

// Synctax Parsing  //////////////////////////////////////////////////////////
  prPic = (VDEC_INFO_RM_PICINFO_T*) &_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMPicInfo;
  prParsingPic = (VDEC_INFO_RM_PICINFO_T*) &_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo;


  printk("<vdec> pType = %d\n", prPic->ePtype);

  //Sync Pic Parameter
  u4DecodedPicCnt = prParsingPic->u4DecodedPicCnt;
  u4RefQpMb0 = prParsingPic->u4RefQpMb0;
  memcpy((void*) prParsingPic, (void*) prPic, sizeof(VDEC_INFO_RM_PICINFO_T));
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
        vRM_GetPicSize(0, u4InstID, &prParsingPic->u4OrgWidth, &prParsingPic->u4OrgHeight);
      }
    }
    else
    {
      vRM_GetPicSize(0, u4InstID, &prParsingPic->u4OrgWidth, &prParsingPic->u4OrgHeight);
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

//Set BwdIsI
  if((prParsingPic->ePtype == RM_INTRAPIC) || (prParsingPic->ePtype == RM_FORCED_INTRAPIC))
  {
    prParsingPic->fgBwdIsI = TRUE;
  }
  else if(prParsingPic->ePtype == RM_INTERPIC)
  {
    prParsingPic->fgBwdIsI = FALSE;
  }

  if (prParsingPic->fgDeblockPassThru == TRUE)
  {
    printk("=====> Disable Deblocking \n");
  }

// Buf Control
#ifdef RM_LOOPDECODE_FLOW
  if (u4LoopDecodeTargetCnt >= u4LoopDecodeFrmCnt)
#endif //RM_LOOPDECODE_FLOW  	
  //if (fgFirstTime)
  {
    if (prParsingPic->ePtype == RM_INTRAPIC || prParsingPic->ePtype == RM_FORCED_INTRAPIC)
    {
      if (prParsingPic->u4FwdBufY == 0)  // the first I pic
      {
        prParsingPic->u4OutBufY = (UINT32) _pucPic1Y[u4InstID]; // PPY_OUT_BUF1;
        prParsingPic->u4OutBufC = (UINT32) _pucPic1C[u4InstID]; //PPC_OUT_BUF1;
        prParsingPic->u4FwdBufY = (UINT32) _pucPic1Y[u4InstID]; //PPY_OUT_BUF1;
        prParsingPic->u4FwdBufC = (UINT32) _pucPic1C[u4InstID]; //PPC_OUT_BUF1;
        prParsingPic->u4BwdBufY = 0;
        prParsingPic->u4BwdBufC = 0;
      }
      else if (prParsingPic->u4BwdBufY == (UINT32) _pucPic1Y[u4InstID])
      {
        prParsingPic->u4OutBufY = (UINT32) _pucPic2Y[u4InstID]; //PPY_OUT_BUF2;
        prParsingPic->u4OutBufC = (UINT32) _pucPic2C[u4InstID]; //PPC_OUT_BUF2;
        prParsingPic->u4FwdBufY = (UINT32) _pucPic1Y[u4InstID]; //PPY_OUT_BUF1;
        prParsingPic->u4FwdBufC = (UINT32) _pucPic1C[u4InstID]; //PPC_OUT_BUF1;
        prParsingPic->u4BwdBufY = (UINT32) _pucPic2Y[u4InstID]; //PPY_OUT_BUF2;
        prParsingPic->u4BwdBufC = (UINT32) _pucPic2C[u4InstID]; //PPC_OUT_BUF2;
      }
      else if (prParsingPic->u4BwdBufY == (UINT32) _pucPic2Y[u4InstID])
      {
        prParsingPic->u4OutBufY = (UINT32) _pucPic1Y[u4InstID]; //PPY_OUT_BUF1;
        prParsingPic->u4OutBufC = (UINT32) _pucPic1C[u4InstID]; //PPC_OUT_BUF1;
        prParsingPic->u4FwdBufY = (UINT32) _pucPic2Y[u4InstID]; //PPY_OUT_BUF2;
        prParsingPic->u4FwdBufC = (UINT32) _pucPic2C[u4InstID]; //PPC_OUT_BUF2;
        prParsingPic->u4BwdBufY = (UINT32) _pucPic1Y[u4InstID]; //PPY_OUT_BUF1;
        prParsingPic->u4BwdBufC = (UINT32) _pucPic1C[u4InstID]; //PPC_OUT_BUF1;
       }
      else if (prParsingPic->u4BwdBufY == 0)  // should happen only when 2 consecutive I pictures at the beginning
      {
        prParsingPic->u4OutBufY = (UINT32) _pucPic2Y[u4InstID]; //PPY_OUT_BUF2;
        prParsingPic->u4OutBufC = (UINT32) _pucPic2C[u4InstID]; //PPC_OUT_BUF2;
        prParsingPic->u4FwdBufY = (UINT32) _pucPic1Y[u4InstID]; //PPY_OUT_BUF1;
        prParsingPic->u4FwdBufC = (UINT32) _pucPic1C[u4InstID]; //PPC_OUT_BUF1;
        prParsingPic->u4BwdBufY = (UINT32) _pucPic2Y[u4InstID]; //PPY_OUT_BUF2;
        prParsingPic->u4BwdBufC =(UINT32)  _pucPic2C[u4InstID]; //PPC_OUT_BUF2;
      }
      else
      {
        ASSERT(0);
      }

      #ifdef RM_RPR_RACINGMODE_ENABLE
      if (_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.fgRPRRacingModeEnable)
      {
        VDEC_INFO_RM_PICINFO_T *rPreParsingPicInfo;
        
        rPreParsingPicInfo = (VDEC_INFO_RM_PICINFO_T*) &_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMPicInfo_Preparsing;
        
        printk("=====> I Racing Mode, About to trigger image resizer in racing mode...\n");
          vRprImgResz(u4InstID,
            prPic->u4Width, prPic->u4Height,
            prPic->u4OrgWidth, prPic->u4OrgHeight,
            rPreParsingPicInfo->u4Width, rPreParsingPicInfo->u4Height,
            rPreParsingPicInfo->u4OrgWidth, rPreParsingPicInfo->u4OrgHeight
          );
      printk("=====> I Racing Mode, Image resizer is now waiting for vdec...\n");
      }
      #endif //RM_RPR_RACINGMODE_ENABLE
/*
#if defined(RPR_RACING) && 1  // megaa 20080116, for realvideo10_56 #395 case: P B I Psc
      if (_eRprRacStt == RPR_RAC_SCP2_DTCED)
      {
        printf("About to trigger image resizer in racing mode...\n");
        vRprImgResz(
          prPic->u4Width, prPic->u4Height,
          prPic->u4OrgWidth, prPic->u4OrgHeight,
          rTmpPic.u4Width, rTmpPic.u4Height,
          rTmpPic.u4OrgWidth, rTmpPic.u4OrgHeight
        );
        printf("Image resizer is now waiting for vdec...\n");
        _eRprRacStt = RPR_RAC_SCP1_PENDING;
      }
#endif
*/

      // initialize the previous picture sizes
      prParsingPic->u4PrevDispWidth = prParsingPic->u4OrgWidth;
      prParsingPic->u4PrevDispHeight = prParsingPic->u4OrgHeight;
      prParsingPic->u4PrevDecWidth = prParsingPic->u4Width;
      prParsingPic->u4PrevDecHeight = prParsingPic->u4Height;
    }
    else if (prPic->ePtype == RM_INTERPIC)
    {
      if (prParsingPic->u4BwdBufY == 0)  // the first P pic
      {
        prParsingPic->u4OutBufY = (UINT32) _pucPic2Y[u4InstID]; //PPY_OUT_BUF2;
        prParsingPic->u4OutBufC = (UINT32) _pucPic2C[u4InstID]; //PPC_OUT_BUF2;
        prParsingPic->u4FwdBufY = (UINT32) _pucPic1Y[u4InstID]; //PPY_OUT_BUF1;
        prParsingPic->u4FwdBufC = (UINT32) _pucPic1C[u4InstID]; //PPC_OUT_BUF1;
        prParsingPic->u4BwdBufY = (UINT32) _pucPic2Y[u4InstID]; //PPY_OUT_BUF2;
        prParsingPic->u4BwdBufC = (UINT32) _pucPic2C[u4InstID]; //PPC_OUT_BUF2;
      }
      else if (prParsingPic->u4BwdBufY == (UINT32) _pucPic1Y[u4InstID])
      {
        prParsingPic->u4OutBufY = (UINT32) _pucPic2Y[u4InstID]; //PPY_OUT_BUF2;
        prParsingPic->u4OutBufC = (UINT32) _pucPic2C[u4InstID]; //PPC_OUT_BUF2;
        prParsingPic->u4FwdBufY = (UINT32) _pucPic1Y[u4InstID]; //PPY_OUT_BUF1;
        prParsingPic->u4FwdBufC = (UINT32) _pucPic1C[u4InstID]; //PPC_OUT_BUF1;
        prParsingPic->u4BwdBufY = (UINT32) _pucPic2Y[u4InstID]; //PPY_OUT_BUF2;
        prParsingPic->u4BwdBufC = (UINT32) _pucPic2C[u4InstID]; //PPC_OUT_BUF2;
      }
      else if (prParsingPic->u4BwdBufY == (UINT32) _pucPic2Y[u4InstID])
      {
        prParsingPic->u4OutBufY = (UINT32) _pucPic1Y[u4InstID]; //PPY_OUT_BUF1;
        prParsingPic->u4OutBufC = (UINT32) _pucPic1C[u4InstID]; //PPC_OUT_BUF1;
        prParsingPic->u4FwdBufY = (UINT32) _pucPic2Y[u4InstID]; //PPY_OUT_BUF2;
        prParsingPic->u4FwdBufC = (UINT32) _pucPic2C[u4InstID]; //PPC_OUT_BUF2;
        prParsingPic->u4BwdBufY = (UINT32) _pucPic1Y[u4InstID]; //PPY_OUT_BUF1;
        prParsingPic->u4BwdBufC = (UINT32) _pucPic1C[u4InstID]; //PPC_OUT_BUF1;
      }
      else
      {
        ASSERT(0);
      }

      #ifdef RM_RPR_RACINGMODE_ENABLE
      if (_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.fgRPRRacingModeEnable)
      {
        VDEC_INFO_RM_PICINFO_T *rPreParsingPicInfo;
        
        rPreParsingPicInfo = (VDEC_INFO_RM_PICINFO_T*) &_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMPicInfo_Preparsing;
        
        printk("=====> P Racing Mode, About to trigger image resizer in racing mode...\n");
        vRprImgResz(u4InstID,
            prPic->u4Width, prPic->u4Height,
            prPic->u4OrgWidth, prPic->u4OrgHeight,
            rPreParsingPicInfo->u4Width, rPreParsingPicInfo->u4Height,
            rPreParsingPicInfo->u4OrgWidth, rPreParsingPicInfo->u4OrgHeight
          );
          printk("=====> P Racing Mode, Image resizer is now waiting for vdec...\n");
      }
      #endif //RM_RPR_RACINGMODE_ENABLE
/*      
#ifdef RPR_RACING
      if (_eRprRacStt == RPR_RAC_SCP2_DTCED)
      {
        printf("About to trigger image resizer in racing mode...\n");
        vRprImgResz(
          prPic->u4Width, prPic->u4Height,
          prPic->u4OrgWidth, prPic->u4OrgHeight,
          rTmpPic.u4Width, rTmpPic.u4Height,
          rTmpPic.u4OrgWidth, rTmpPic.u4OrgHeight
        );
        printf("Image resizer is now waiting for vdec...\n");
        _eRprRacStt = RPR_RAC_SCP1_PENDING;
      }
      else  // to avoid the next if block
#endif
*/

      if (prParsingPic->u4OrgWidth != prParsingPic->u4PrevDispWidth || prParsingPic->u4OrgHeight != prParsingPic->u4PrevDispHeight)  // RPR
      {
        #ifndef RM_RPR_RACINGMODE_ENABLE
        printk("Normal RPR, About to trigger image resizer...\n");
        vRprImgResz(
           u4InstID,
            prParsingPic->u4PrevDecWidth, prParsingPic->u4PrevDecHeight,
            prParsingPic->u4PrevDispWidth, prParsingPic->u4PrevDispHeight,
            prParsingPic->u4Width, prParsingPic->u4Height,
            prParsingPic->u4OrgWidth, prParsingPic->u4OrgHeight
          );
        printk("Normal RPR, Image has been resized!\n");
        #endif //RM_RPR_RACINGMODE_ENABLE
      
        //Will Support this
        //ASSERT(0);
#if 0
#ifdef RPR_RACING
        if (_eRprRacStt == RPR_RAC_NOTHING)
#endif
        {
          printf("About to trigger image resizer...\n");
          vRprImgResz(
            _u4PrevDecWidth, _u4PrevDecHeight,
            _u4PrevDispWidth, _u4PrevDispHeight,
            prPic->u4Width, prPic->u4Height,
            prPic->u4OrgWidth, prPic->u4OrgHeight
          );
          printf("Image has been resized!\n");
        }
#ifdef RPR_RACING
        //ASSERT(_eRprRacStt == RPR_RAC_SCP1_DONE);
        _eRprRacStt = RPR_RAC_NOTHING;
#endif
#endif //0
        prParsingPic->u4FwdBufY = (UINT32) _pucPic3Y[u4InstID]; //PPY_OUT_BUFR;
        prParsingPic->u4FwdBufC = (UINT32) _pucPic3C[u4InstID]; //PPC_OUT_BUFR;
      }
      
      prParsingPic->u4PrevDispWidth = prParsingPic->u4OrgWidth;
      prParsingPic->u4PrevDispHeight = prParsingPic->u4OrgHeight;
      prParsingPic->u4PrevDecWidth = prParsingPic->u4Width;
      prParsingPic->u4PrevDecHeight = prParsingPic->u4Height;
    }
    else if (prParsingPic->ePtype == RM_TRUEBPIC)
    {
      prParsingPic->u4OutBufY = (UINT32) _pucPic0Y[u4InstID]; //PPY_OUT_BUF3;
      prParsingPic->u4OutBufC = (UINT32) _pucPic0C[u4InstID]; //PPC_OUT_BUF3;
      //prParsingPic->u4OutBufY = (UINT32) _pucPic3Y[u4InstID]; //PPY_OUT_BUF3;
      //prParsingPic->u4OutBufC = (UINT32) _pucPic3C[u4InstID]; //PPC_OUT_BUF3;
    }
    else
    {
      ASSERT(0);
    }
  }

  if ((prParsingPic->u4OutBufY == 0) || (prParsingPic->u4OutBufC == 0))
  {
    ASSERT(0);
  }

//Sync Pic Info
  memcpy((void*) prPic, (void*) prParsingPic, sizeof(VDEC_INFO_RM_PICINFO_T));


  //Clear Y, C Memory
  //memset((void*) prParsingPic->u4OutBufY, 0xFF, PIC_Y_SZ);
  //memset((void*) prParsingPic->u4OutBufC, 0xFF, PIC_C_SZ);

  prParsingPic->u4MCOutBufY = (UINT32) _pucRMMCOutputBufY[u4InstID];
  prParsingPic->u4MCOutBufC = (UINT32) _pucRMMCOutputBufC[u4InstID];

//  HalFlushInvalidateDCache();

  #ifdef RM_LOOPDECODE_FLOW
  u4LoopDecodeFrmCnt++;
  #endif //RM_LOOPDECODE_FLOW
  
}


void vRM_VParserEx(UINT32 u4InstID) 
{
  VDEC_INFO_RM_BS_INIT_PRM_T rRMVdecBSInfo; 
  VDEC_INFO_RM_PICINFO_T *prPic;   
  VDEC_INFO_RM_PICINFO_T *prParsingPic;   
  UINT32 u4RefQpMb0 = 0;
  UINT32 u4DecodedPicCnt = 0;
  #ifdef RM_ATSPEED_TEST_ENABLE
  UINT32 *pu4FIFOPtr;
  UINT32 u4FrmInfoLength = 0;
  UINT32 u4PayloadLength = 0;
  #endif //RM_ATSPEED_TEST_ENABLE

  //HalFlushInvalidateDCache();
  
//VDec HW Init Flow  //////////////////////////////////////////////////////////
#if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8555)
  vVDecResetHW(u4InstID);
#else
    vVDecResetHW(u4InstID, VDEC_UNKNOWN);
#endif
  
  //Init RM VDec HW
  vRM_Hal_VldSoftReset(u4InstID);

  vRM_Hal_MvInit(u4InstID, _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4MvHwWorkBuf);

  vRM_Hal_McInit(u4InstID);

  vRM_Hal_PpInit(u4InstID);

  //Init RM BS HW
  vRM_Hal_VldInit(u4InstID, _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4VldPredWorkBuf);


  #ifndef RM_ATSPEED_TEST_ENABLE
  
  //rRMVdecBSInfo.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
  //rRMVdecBSInfo.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
  rRMVdecBSInfo.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
  rRMVdecBSInfo.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;

  #ifdef RM_RINGVIFO_FLOW
  rRMVdecBSInfo.u4ReadPointer= _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFIFORPtr;
  #else //RM_RINGVIFO_FLOW
  rRMVdecBSInfo.u4ReadPointer= (UINT32)_pucVFifo[u4InstID];
  #endif //RM_RINGVIFO_FLOW

  #ifdef RM_RINGVIFO_FLOW
  rRMVdecBSInfo.u4WritePointer = (UINT32) (_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMVFIFOWPtr + 1024);
  #else
  rRMVdecBSInfo.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
  #endif //RM_RINGVIFO_FLOW
  
  #else //RM_ATSPEED_TEST_ENABLE

  //Parsing AU Info
  if (_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFIFORPtr == 
      (UINT32) _pucRMAULikeBuf[u4InstID])
  {
    //Get Total Frame Number
    pu4FIFOPtr = (UINT32*) _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFIFORPtr;

    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4ClipTotalFrameCnt = pu4FIFOPtr[0];

    pu4FIFOPtr = pu4FIFOPtr+1;
  }
  else
  {
    pu4FIFOPtr = (UINT32*) _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFIFORPtr;
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

  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFIFORPtr = ((UINT32) pu4FIFOPtr) + u4FrmInfoLength;
  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4FrameInfoSize = u4FrmInfoLength;
  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4FramePayloadSize = u4PayloadLength;

  rRMVdecBSInfo.u4VFifoSa = (UINT32) _pucRMAULikeBuf[u4InstID];
  rRMVdecBSInfo.u4VFifoEa = (UINT32) _pucRMAULikeBuf[u4InstID] + RM_AULIKEBUF_SZ;
  rRMVdecBSInfo.u4VFifoSa = (UINT32) _pucRMAULikeBuf[u4InstID];
  rRMVdecBSInfo.u4VFifoEa = (UINT32) _pucRMAULikeBuf[u4InstID] + RM_AULIKEBUF_SZ;

  rRMVdecBSInfo.u4ReadPointer= _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFIFORPtr;

  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMVFIFOWPtr = 
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFIFORPtr + u4PayloadLength;
  rRMVdecBSInfo.u4WritePointer = (UINT32) (_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMVFIFOWPtr + 1024);
  
  #endif //RM_ATSPEED_TEST_ENABLE

  i4RM_HAL_InitBarrelShifter(_u4BSID[u4InstID], u4InstID, &rRMVdecBSInfo);
  u4VDEC_HAL_VP6_VDec_SetWorkspace(u4InstID, (UINT32)_pucVP6VLDWrapperWorkspace[u4InstID], (UINT32)_pucVP6PPWrapperWorkspace[u4InstID]);

  #ifdef RM_RPR_RACINGMODE_ENABLE
  //Pre-Parsing Picture Information
  {
    UINT32 u4BackupFrmInfoRPtr = 0;
    VDEC_INFO_RM_PICINFO_T *prPic_Preparsing;
    VDEC_INFO_RM_PICINFO_T *prPic_Normal;
    UINT32 u4SearchLoop;
    
    prPic_Preparsing = (VDEC_INFO_RM_PICINFO_T*) &_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMPicInfo_Preparsing;
    prPic_Normal = (VDEC_INFO_RM_PICINFO_T*) &_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMPicInfo;
    u4BackupFrmInfoRPtr = _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFrmInfoRPtr;

    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.fgRPRRacingModeEnable = FALSE;

    //Parsing First Pic Info
    fgGetRvPic(u4InstID, __FUNCTION__, __LINE__);

    memcpy((void*) (&_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMPicInfo_Preparsing), 
    	                (void*) (&_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMPicInfo),
    	                sizeof(VDEC_INFO_RM_PICINFO_T));

    if ((prPic_Preparsing->ePtype == RM_INTRAPIC) ||
    	  (prPic_Preparsing->ePtype == RM_FORCED_INTRAPIC) ||
    	  (prPic_Preparsing->ePtype == RM_INTERPIC))
    {
      //Search Next I or  P
      for (u4SearchLoop=0; u4SearchLoop< RPR_RAC_PREPARSING_CNT; u4SearchLoop++)
      {
        if (fgGetRvPic(u4InstID, __FUNCTION__, __LINE__) == FALSE)
        {
          break;
        }

        if ((prPic_Normal->ePtype == RM_INTRAPIC) ||
             (prPic_Normal->ePtype == RM_FORCED_INTRAPIC))
        {
          //Stop Preparsing
          break;
        }

        //Check Pic W/H
        if (prPic_Normal->ePtype == RM_INTERPIC)
        {
          if ((prPic_Normal->u4OrgWidth != prPic_Preparsing->u4OrgWidth) ||
               (prPic_Normal->u4OrgHeight != prPic_Preparsing->u4OrgHeight))
          {
            //Enable RPR Racing Mode
            _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.fgRPRRacingModeEnable = TRUE;
          
            memcpy((void*) (&_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMPicInfo_Preparsing), 
    	                         (void*) (&_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMPicInfo),
    	                         sizeof(VDEC_INFO_RM_PICINFO_T));
          }
          
          break;
        }
      }
    }

    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFrmInfoRPtr = u4BackupFrmInfoRPtr;
  }
  #endif //RM_RPR_RACINGMODE_ENABLE

  

// Get Golden Syntax Pic Info
  #ifdef RM_LOOPDECODE_FLOW
  {
    UINT32 u4Loop;

    if (u4LoopDecodeFirstFlag)
    {
      if (u4LoopDecodeFrmCnt > 0)
      {
        _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt = u4LoopDecodeFrmCnt-1;
        for (u4Loop=0; u4Loop < u4LoopDecodeFrmCnt; u4Loop++)
        {
          fgGetRvPic(u4InstID, __FUNCTION__, __LINE__);
        }
      }
      else
      {
        _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt = 0;
        fgGetRvPic(u4InstID, __FUNCTION__, __LINE__);
      }
      u4LoopDecodeFirstFlag = 0;
    }
    else
    {
      if (u4LoopDecodeTargetCnt >= u4LoopDecodeFrmCnt)
      {
        fgGetRvPic(u4InstID, __FUNCTION__, __LINE__);
        //u4LoopDecodeFrmCnt++;
      }
      else
      {
        //Re-Decode Start
        u4LoopDecodeCnt++;
      }
    }
  }
  #else //RM_LOOPDECODE_FLOW
  //Parsing Current Picture Information
  #ifdef RM_SKIPFRAME_Test
  {
    UINT32 u4SkipIdx = 0;

    if (u4SkipFrameNum > 0)
    {
      for (u4SkipIdx=0; u4SkipIdx< u4SkipFrameNum; u4SkipIdx++)
      {
        fgGetRvPic(u4InstID, __FUNCTION__, __LINE__);
      }

      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt = u4SkipFrameNum -1;

      u4SkipCheckFrameNum = u4SkipFrameNum - 1;
      u4SkipFrameNum = 0;
    }
    else
    {
      fgGetRvPic(u4InstID, __FUNCTION__, __LINE__);
    }
  }
  #else //RM_SKIPFRAME_Test
  fgGetRvPic(u4InstID, __FUNCTION__, __LINE__);
  #endif //RM_SKIPFRAME_Test
  #endif //RM_LOOPDECODE_FLOW

// Synctax Parsing  //////////////////////////////////////////////////////////
  prPic = (VDEC_INFO_RM_PICINFO_T*) &_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMPicInfo;
  prParsingPic = (VDEC_INFO_RM_PICINFO_T*) &_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo;

  //Sync Pic Parameter
  u4DecodedPicCnt = prParsingPic->u4DecodedPicCnt;
  u4RefQpMb0 = prParsingPic->u4RefQpMb0;
  memcpy((void*) prParsingPic, (void*) prPic, sizeof(VDEC_INFO_RM_PICINFO_T));
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
        vRM_GetPicSize(0, u4InstID, &prParsingPic->u4OrgWidth, &prParsingPic->u4OrgHeight);
      }
    }
    else
    {
      vRM_GetPicSize(0, u4InstID, &prParsingPic->u4OrgWidth, &prParsingPic->u4OrgHeight);
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

//Set BwdIsI
  if((prParsingPic->ePtype == RM_INTRAPIC) || (prParsingPic->ePtype == RM_FORCED_INTRAPIC))
  {
    prParsingPic->fgBwdIsI = TRUE;
  }
  else if(prParsingPic->ePtype == RM_INTERPIC)
  {
    prParsingPic->fgBwdIsI = FALSE;
  }

  if (prParsingPic->fgDeblockPassThru == TRUE)
  {
    printk("=====> Disable Deblocking \n");
  }

// Buf Control
#ifdef RM_LOOPDECODE_FLOW
  if (u4LoopDecodeTargetCnt >= u4LoopDecodeFrmCnt)
#endif //RM_LOOPDECODE_FLOW  	
  //if (fgFirstTime)
  {
    if (prParsingPic->ePtype == RM_INTRAPIC || prParsingPic->ePtype == RM_FORCED_INTRAPIC)
    {
      if (prParsingPic->u4FwdBufY == 0)  // the first I pic
      {
        prParsingPic->u4OutBufY = (UINT32) _pucPic1Y[u4InstID]; // PPY_OUT_BUF1;
        prParsingPic->u4OutBufC = (UINT32) _pucPic1C[u4InstID]; //PPC_OUT_BUF1;
        prParsingPic->u4FwdBufY = (UINT32) _pucPic1Y[u4InstID]; //PPY_OUT_BUF1;
        prParsingPic->u4FwdBufC = (UINT32) _pucPic1C[u4InstID]; //PPC_OUT_BUF1;
        prParsingPic->u4BwdBufY = 0;
        prParsingPic->u4BwdBufC = 0;
      }
      else if (prParsingPic->u4BwdBufY == (UINT32) _pucPic1Y[u4InstID])
      {
        prParsingPic->u4OutBufY = (UINT32) _pucPic2Y[u4InstID]; //PPY_OUT_BUF2;
        prParsingPic->u4OutBufC = (UINT32) _pucPic2C[u4InstID]; //PPC_OUT_BUF2;
        prParsingPic->u4FwdBufY = (UINT32) _pucPic1Y[u4InstID]; //PPY_OUT_BUF1;
        prParsingPic->u4FwdBufC = (UINT32) _pucPic1C[u4InstID]; //PPC_OUT_BUF1;
        prParsingPic->u4BwdBufY = (UINT32) _pucPic2Y[u4InstID]; //PPY_OUT_BUF2;
        prParsingPic->u4BwdBufC = (UINT32) _pucPic2C[u4InstID]; //PPC_OUT_BUF2;
      }
      else if (prParsingPic->u4BwdBufY == (UINT32) _pucPic2Y[u4InstID])
      {
        prParsingPic->u4OutBufY = (UINT32) _pucPic1Y[u4InstID]; //PPY_OUT_BUF1;
        prParsingPic->u4OutBufC = (UINT32) _pucPic1C[u4InstID]; //PPC_OUT_BUF1;
        prParsingPic->u4FwdBufY = (UINT32) _pucPic2Y[u4InstID]; //PPY_OUT_BUF2;
        prParsingPic->u4FwdBufC = (UINT32) _pucPic2C[u4InstID]; //PPC_OUT_BUF2;
        prParsingPic->u4BwdBufY = (UINT32) _pucPic1Y[u4InstID]; //PPY_OUT_BUF1;
        prParsingPic->u4BwdBufC = (UINT32) _pucPic1C[u4InstID]; //PPC_OUT_BUF1;
       }
      else if (prParsingPic->u4BwdBufY == 0)  // should happen only when 2 consecutive I pictures at the beginning
      {
        prParsingPic->u4OutBufY = (UINT32) _pucPic2Y[u4InstID]; //PPY_OUT_BUF2;
        prParsingPic->u4OutBufC = (UINT32) _pucPic2C[u4InstID]; //PPC_OUT_BUF2;
        prParsingPic->u4FwdBufY = (UINT32) _pucPic1Y[u4InstID]; //PPY_OUT_BUF1;
        prParsingPic->u4FwdBufC = (UINT32) _pucPic1C[u4InstID]; //PPC_OUT_BUF1;
        prParsingPic->u4BwdBufY = (UINT32) _pucPic2Y[u4InstID]; //PPY_OUT_BUF2;
        prParsingPic->u4BwdBufC =(UINT32)  _pucPic2C[u4InstID]; //PPC_OUT_BUF2;
      }
      else
      {
        ASSERT(0);
      }

      #ifdef RM_RPR_RACINGMODE_ENABLE
      if (_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.fgRPRRacingModeEnable)
      {
        VDEC_INFO_RM_PICINFO_T *rPreParsingPicInfo;
        
        rPreParsingPicInfo = (VDEC_INFO_RM_PICINFO_T*) &_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMPicInfo_Preparsing;
        
        printk("=====> I Racing Mode, About to trigger image resizer in racing mode...\n");
          vRprImgResz(u4InstID,
            prPic->u4Width, prPic->u4Height,
            prPic->u4OrgWidth, prPic->u4OrgHeight,
            rPreParsingPicInfo->u4Width, rPreParsingPicInfo->u4Height,
            rPreParsingPicInfo->u4OrgWidth, rPreParsingPicInfo->u4OrgHeight
          );
      printk("=====> I Racing Mode, Image resizer is now waiting for vdec...\n");
      }
      #endif //RM_RPR_RACINGMODE_ENABLE

      // initialize the previous picture sizes
      prParsingPic->u4PrevDispWidth = prParsingPic->u4OrgWidth;
      prParsingPic->u4PrevDispHeight = prParsingPic->u4OrgHeight;
      prParsingPic->u4PrevDecWidth = prParsingPic->u4Width;
      prParsingPic->u4PrevDecHeight = prParsingPic->u4Height;
    }
    else if (prPic->ePtype == RM_INTERPIC)
    {
      if (prParsingPic->u4BwdBufY == 0)  // the first P pic
      {
        prParsingPic->u4OutBufY = (UINT32) _pucPic2Y[u4InstID]; //PPY_OUT_BUF2;
        prParsingPic->u4OutBufC = (UINT32) _pucPic2C[u4InstID]; //PPC_OUT_BUF2;
        prParsingPic->u4FwdBufY = (UINT32) _pucPic1Y[u4InstID]; //PPY_OUT_BUF1;
        prParsingPic->u4FwdBufC = (UINT32) _pucPic1C[u4InstID]; //PPC_OUT_BUF1;
        prParsingPic->u4BwdBufY = (UINT32) _pucPic2Y[u4InstID]; //PPY_OUT_BUF2;
        prParsingPic->u4BwdBufC = (UINT32) _pucPic2C[u4InstID]; //PPC_OUT_BUF2;
      }
      else if (prParsingPic->u4BwdBufY == (UINT32) _pucPic1Y[u4InstID])
      {
        prParsingPic->u4OutBufY = (UINT32) _pucPic2Y[u4InstID]; //PPY_OUT_BUF2;
        prParsingPic->u4OutBufC = (UINT32) _pucPic2C[u4InstID]; //PPC_OUT_BUF2;
        prParsingPic->u4FwdBufY = (UINT32) _pucPic1Y[u4InstID]; //PPY_OUT_BUF1;
        prParsingPic->u4FwdBufC = (UINT32) _pucPic1C[u4InstID]; //PPC_OUT_BUF1;
        prParsingPic->u4BwdBufY = (UINT32) _pucPic2Y[u4InstID]; //PPY_OUT_BUF2;
        prParsingPic->u4BwdBufC = (UINT32) _pucPic2C[u4InstID]; //PPC_OUT_BUF2;
      }
      else if (prParsingPic->u4BwdBufY == (UINT32) _pucPic2Y[u4InstID])
      {
        prParsingPic->u4OutBufY = (UINT32) _pucPic1Y[u4InstID]; //PPY_OUT_BUF1;
        prParsingPic->u4OutBufC = (UINT32) _pucPic1C[u4InstID]; //PPC_OUT_BUF1;
        prParsingPic->u4FwdBufY = (UINT32) _pucPic2Y[u4InstID]; //PPY_OUT_BUF2;
        prParsingPic->u4FwdBufC = (UINT32) _pucPic2C[u4InstID]; //PPC_OUT_BUF2;
        prParsingPic->u4BwdBufY = (UINT32) _pucPic1Y[u4InstID]; //PPY_OUT_BUF1;
        prParsingPic->u4BwdBufC = (UINT32) _pucPic1C[u4InstID]; //PPC_OUT_BUF1;
      }
      else
      {
        ASSERT(0);
      }

      #ifdef RM_RPR_RACINGMODE_ENABLE
      if (_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.fgRPRRacingModeEnable)
      {
        VDEC_INFO_RM_PICINFO_T *rPreParsingPicInfo;
        
        rPreParsingPicInfo = (VDEC_INFO_RM_PICINFO_T*) &_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMPicInfo_Preparsing;
        
        printk("=====> P Racing Mode, About to trigger image resizer in racing mode...\n");
        vRprImgResz(u4InstID,
            prPic->u4Width, prPic->u4Height,
            prPic->u4OrgWidth, prPic->u4OrgHeight,
            rPreParsingPicInfo->u4Width, rPreParsingPicInfo->u4Height,
            rPreParsingPicInfo->u4OrgWidth, rPreParsingPicInfo->u4OrgHeight
          );
          printk("=====> P Racing Mode, Image resizer is now waiting for vdec...\n");
      }
      #endif //RM_RPR_RACINGMODE_ENABLE

      if (prParsingPic->u4OrgWidth != prParsingPic->u4PrevDispWidth || prParsingPic->u4OrgHeight != prParsingPic->u4PrevDispHeight)  // RPR
      {
        #ifndef RM_RPR_RACINGMODE_ENABLE
        printk("Normal RPR, About to trigger image resizer...\n");
        vRprImgResz(
           u4InstID,
            prParsingPic->u4PrevDecWidth, prParsingPic->u4PrevDecHeight,
            prParsingPic->u4PrevDispWidth, prParsingPic->u4PrevDispHeight,
            prParsingPic->u4Width, prParsingPic->u4Height,
            prParsingPic->u4OrgWidth, prParsingPic->u4OrgHeight
          );
        printk("Normal RPR, Image has been resized!\n");
        #endif //RM_RPR_RACINGMODE_ENABLE
      
        prParsingPic->u4FwdBufY = (UINT32) _pucPic3Y[u4InstID]; //PPY_OUT_BUFR;
        prParsingPic->u4FwdBufC = (UINT32) _pucPic3C[u4InstID]; //PPC_OUT_BUFR;
      }
      
      prParsingPic->u4PrevDispWidth = prParsingPic->u4OrgWidth;
      prParsingPic->u4PrevDispHeight = prParsingPic->u4OrgHeight;
      prParsingPic->u4PrevDecWidth = prParsingPic->u4Width;
      prParsingPic->u4PrevDecHeight = prParsingPic->u4Height;
    }
    else if (prParsingPic->ePtype == RM_TRUEBPIC)
    {
      prParsingPic->u4OutBufY = (UINT32) _pucPic0Y[u4InstID]; //PPY_OUT_BUF3;
      prParsingPic->u4OutBufC = (UINT32) _pucPic0C[u4InstID]; //PPC_OUT_BUF3;
    }
    else
    {
      ASSERT(0);
    }
  }

 if ((prParsingPic->u4OutBufY == 0) || (prParsingPic->u4OutBufC == 0))
  {
    ASSERT(0);
  }

//Sync Pic Info
  memcpy((void*) prPic, (void*) prParsingPic, sizeof(VDEC_INFO_RM_PICINFO_T));

  prParsingPic->u4MCOutBufY = (UINT32) _pucRMMCOutputBufY[u4InstID];
  prParsingPic->u4MCOutBufC = (UINT32) _pucRMMCOutputBufC[u4InstID];

//  HalFlushInvalidateDCache();

  #ifdef RM_LOOPDECODE_FLOW
  u4LoopDecodeFrmCnt++;
  #endif //RM_LOOPDECODE_FLOW  
}



// =======================================================================================
// Result Check Function
inline UINT32 mb_to_line_y(UINT32 buf, int width, int x, int y)
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

static inline void mb_to_line_c(UINT32 *pu4Val1, UINT32 *pu4Val2, UINT32 buf, int width, int x, int y)
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

UINT32 u4InverseAddrSwap_RM(UINT32 u4AddrSwapMode, UINT32 u4SwappedAddr, BOOL fgIsYComponent)
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

void vRM_InvAddressSwap(UINT32 u4InstID, UINT32 u4SrcType)   //SrcType = 0  =>  Original Size Data, 1 => DSCL Data, 2 => MC Out (Only for RV8 DSCL)
{
  UINT32 i;
  VDEC_INFO_RM_PICINFO_T *prParsingPic;
  UINT32 u4DataLength;
  UINT32 u4AlignW_Luma;
  UINT32 u4AlignH_Luma;
  //UINT32 u4AlignW_Chroma;
  UINT32 u4AlignH_Chroma;
  UINT32 u4AlignSize = 0;
  //UINT32 u4AlignSize = 0x10000;
  //UINT32 u4AlignSize = 0x28000;
  UINT32 u4NonSwappedAddr;
  UINT32 u4AddrSwapMode;
  UINT32 u4SwappedAddr;
  BYTE* pbSrcBufY;
  BYTE* pbSrcBufC;
  BYTE* pbOutBufY;
  BYTE* pbOutBufC;
  BYTE * pbTempBufAddr;
  UINT32 u4AddrressSwapSize = 16;

  u4AlignSize = u4PredefineAlignSize;

  prParsingPic = (VDEC_INFO_RM_PICINFO_T*) &_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo;

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
    pbSrcBufY = (BYTE*) _pucVDSCLBuf[u4InstID];
    pbSrcBufC = (BYTE*) (_pucVDSCLBuf[u4InstID] + 0x1FE000);
  }

  pbOutBufY = (BYTE*) _pucDumpYBuf_1[u4InstID];
  pbOutBufC = (BYTE*) _pucDumpCBuf_1[u4InstID];
  //pbOutBufY = (BYTE*) _pucDumpYBuf[u4InstID];
  //pbOutBufC = (BYTE*) _pucDumpCBuf[u4InstID];

  u4AddrSwapMode = auAddrSwapMapTable_RM[_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMAddrSwapMode];
  
  //Luma 
  u4DataLength = u4AlignW_Luma * u4AlignH_Luma;
  u4DataLength = (u4DataLength + u4AlignSize)/u4AlignSize;
  u4DataLength = (u4DataLength+1) * u4AlignSize + u4AlignSize;
  //u4DataLength = u4DataLength * u4AlignSize + u4AlignSize;
  //u4DataLength = u4DataLength * u4AlignSize;
  u4SwappedAddr = 0;
  
  for (i=0; i<u4DataLength; i+=u4AddrressSwapSize)
  {
    u4NonSwappedAddr = u4InverseAddrSwap_RM(u4AddrSwapMode, u4SwappedAddr, TRUE);
    pbTempBufAddr = (BYTE*) (pbSrcBufY+i);
    memcpy(&pbOutBufY[u4NonSwappedAddr<<4], &pbTempBufAddr[0],u4AddrressSwapSize);
    u4SwappedAddr++;
  }
  
  //Chroma
  u4DataLength = u4AlignW_Luma * u4AlignH_Chroma;
  u4DataLength = (u4DataLength + u4AlignSize)/u4AlignSize;
  u4DataLength = u4DataLength * u4AlignSize + u4AlignSize;
  //u4DataLength = u4DataLength * u4AlignSize;
  u4SwappedAddr = 0;

  for (i=0; i<u4DataLength; i+=u4AddrressSwapSize)
  {
    u4NonSwappedAddr = u4InverseAddrSwap_RM(u4AddrSwapMode, u4SwappedAddr, FALSE);
    pbTempBufAddr = (BYTE*) (pbSrcBufC+i);
    memcpy(&pbOutBufC[u4NonSwappedAddr<<4], &pbTempBufAddr[0],u4AddrressSwapSize);
    u4SwappedAddr++;
  }
}

static void vDumpVldCkSum(void);
static void vDumpAvcMvCkSum(void);
static void vDumpMcCkSum(void);
static void vDumpPpCkSum(void);
static void vDumpReg(void);

 void vRMVfyBufDeBlock(UINT32 u4SrcBufSA,UINT32 u4DestBufSA,UINT32 u4BufWidth,UINT32 u4BufHeight,
                           UINT32 u4BlockWidth,UINT32 u4BlockHeight)
{
	UINT32 u4Addr;
	UINT32 u4Len;
	UINT32 u4X,u4Y;

    u4X = 0;
    u4Y = 0;
    u4Len = u4BufWidth*u4BufHeight;
    u4Addr = 0;

    while(u4Addr<u4Len)
    {
        *(UCHAR *)(u4DestBufSA + u4Y*u4BufWidth + u4X) = *(UCHAR *)(u4SrcBufSA + u4Addr);
        u4Addr++;
        u4X++;
        if((u4X % u4BlockWidth) == 0)
        {
            u4X -= u4BlockWidth;
            u4Y++;
            if((u4Y % u4BlockHeight) == 0)
            {
                u4Y -= u4BlockHeight;
                u4X += u4BlockWidth;

                if(u4X>=u4BufWidth)
                {
                    u4Y += u4BlockHeight;
                    u4X = 0;
                }
            }
        }
    }
}
BOOL fgRM_CheckYCGolden(UINT32 u4InstID)
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

  VDEC_INFO_RM_PICINFO_T *prParsingPic;

  UINT32 u4OutBufY;
  UINT32 u4OutBufC;
  UINT32 u4GoldenBuf;

  UINT32 u4UVal;
  UINT32 u4VVal;
  UINT32 u4ErrVal2;
  UINT32 u4GoldYSz;
  UINT32 u4GoldYUSz;
  UINT32 u4GolVal2;

 // HalFlushInvalidateDCache();

  prParsingPic = (VDEC_INFO_RM_PICINFO_T*) &_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo;

  #ifdef RM_ADDRSWAP_ENABLE

  //HalFlushInvalidateDCache();

  //memset(_pucDumpYBuf[u4InstID], 0x0, GOLD_Y_SZ);
  //memset(_pucDumpCBuf[u4InstID], 0x0, GOLD_C_SZ);
  memset(_pucDumpYBuf_1[u4InstID], 0x0, GOLD_Y_SZ);
  memset(_pucDumpCBuf_1[u4InstID], 0x0, GOLD_C_SZ);

 // HalFlushInvalidateDCache();

  vRM_InvAddressSwap(u4InstID, 0);

//  HalFlushInvalidateDCache();

  u4OutBufY = (UINT32) _pucDumpYBuf_1[u4InstID];
  u4OutBufC = (UINT32) _pucDumpCBuf_1[u4InstID];
  //u4OutBufY = (UINT32) _pucDumpYBuf[u4InstID];
  //u4OutBufC = (UINT32) _pucDumpCBuf[u4InstID];
  #else //RM_ADDRSWAP_ENABLE
  u4OutBufY = prParsingPic->u4OutBufY;
  u4OutBufC = prParsingPic->u4OutBufC;
  #endif //RM_ADDRSWAP_ENABLE

#if VMMU_SUPPORT
  u4OutBufY += 0x1000;
  u4OutBufC += 0x1000;

  printk("[RM] vRM_InvAddressSwap, 1, Y Base:0x%x, C Base:0x%x\n", u4OutBufY, u4OutBufC); 
#endif

  
  u4GoldenBuf = _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMGoldenDataBuf;

  u4DecWidth = prParsingPic->u4Width;
  u4DecWidthAlign = prParsingPic->u4Width;
  u4DecHeight = prParsingPic->u4Height;
  u4Width = prParsingPic->u4OrgWidth;
  u4Height = prParsingPic->u4OrgHeight;


  #ifdef RM_DDR3MODE_ENABLE
  #ifndef RM_DDR3MODE_DEBUG_ENABLE
  {
    UINT32 u4WMBSize = prParsingPic->u4Width/ 16;
    if (u4WMBSize % 4)
    {
      u4WMBSize = u4WMBSize + (4 - (u4WMBSize % 4));
    }

    u4DecWidthAlign = u4WMBSize * 16;
  }
  #endif //RM_DDR3MODE_DEBUG_ENABLE
  #endif //RM_DDR3MODE_ENABLE
  
  
  u4GoldYSz = u4DecWidth * u4DecHeight;
  u4GoldYUSz = u4GoldYSz + u4GoldYSz / 4;
 
  //Compare Y Golden
  fgYOK = TRUE;
  for (y = 0; y < u4Height && fgYOK; y++)
  {
    for (x = 0; x < u4Width && fgYOK; x += 4)
    {
      u4ErrVal = mb_to_line_y(u4OutBufY, u4DecWidthAlign, x, y);
      u4GolVal = *(UINT32 *)(u4GoldenBuf + y * u4DecWidth + x);
      
      if (u4ErrVal != u4GolVal)
      {
        u4ErrCntY++;
        fgYOK = FALSE;

        printk("<vdec> Check Golden Y Error\n");
        printk("<vdec> Golden address = 0x%08x. size = %x\n",u4GoldenBuf,(u4Height*u4Width));
        printk("<vdec> HW address = 0x%08x. size = %x\n",u4OutBufY,(u4Height*u4Width));

        printk("<vdec> Y mismatch. x=%d, y=%d. HW Val = 0x%08X, Golden Val = 0x%08X\n", x, y, u4ErrVal, u4GolVal);
        #if 0 // houlong temp
        {
           //vRMVfyBufDeBlock(u4OutBufY,u4GoldenBuf,u4DecWidthAlign,u4DecHeight,16,32);
           vDumpVldCkSum();
           vDumpAvcMvCkSum();
           vDumpMcCkSum();
           vDumpPpCkSum();
           vDumpReg();
         }

        ASSERT(0);
        //while (1);
	#endif

        fgYOK = FALSE;
      }
    }
  }

  //Compare C Data
  fgUOK = TRUE;
  fgVOK = TRUE;
  for (y = 0; y < u4Height/2 && fgUOK && fgVOK; y++)
  {
    for (x = 0; x < u4Width/2 && fgUOK && fgVOK; x += 4)
    {
      u4UVal = *(UINT32 *)(u4GoldenBuf + u4GoldYSz  + y * u4DecWidth / 2 + x);
      u4VVal = *(UINT32 *)(u4GoldenBuf + u4GoldYUSz + y * u4DecWidth / 2 + x);

      mb_to_line_c(&u4ErrVal, &u4ErrVal2, u4OutBufC, u4DecWidthAlign, x, y);
      u4GolVal = ((u4VVal << 16) & 0xFF000000) | ((u4UVal << 8) & 0x00FF0000) |
                 ((u4VVal <<  8) & 0x0000FF00) | ((u4UVal) & 0x000000FF);

      if (u4ErrVal != u4GolVal)
      {
        u4ErrCntC++;
        fgUOK = fgVOK = FALSE;

        printk("Check Golden UV Error /n");
        printk("<vdec> Check Golden UV Error\n");
        printk("<vdec> C mismatch. x=%d, y=%d. HW Val = 08X, Golden Val = 0x%08X\n", x, y, u4ErrVal, u4GolVal);

        #if 0 // houlong temp
        {
           //vRMVfyBufDeBlock(u4OutBufY,u4GoldenBuf,u4DecWidthAlign,u4DecHeight,16,32);
           vDumpVldCkSum();
           vDumpAvcMvCkSum();
           vDumpMcCkSum();
           vDumpPpCkSum();
           vDumpReg();
         }
        ASSERT(0);
        //while (1);
	#endif
      }

      u4GolVal2 = ((u4VVal) & 0xFF000000)      | ((u4UVal >>  8) & 0x00FF0000) |
                  ((u4VVal >> 8) & 0x0000FF00) | ((u4UVal >> 16) & 0x000000FF);
                 
      if (u4ErrVal2 != u4GolVal2)
      {
        u4ErrCntC++;
        fgUOK = fgVOK = FALSE;

        printk("Check Golden UV Error\n");
        //ASSERT(0);
        //while (1);
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

BOOL fgRM_CheckYCDownScalerGolden(UINT32 u4InstID)
{
  UINT32 u4Width,u4Height;
  UINT32 u4YBase,u4CBase;
  UCHAR *pbDecBuf,*pbGoldenBuf;
  UINT32 u4Pix;
  UINT32 u4ErrorCnt = 0;
  #ifdef DOWN_SCALE_SUPPORT
  VDEC_INFO_RM_PICINFO_T *prParsingPic; 
  #endif //RM_ADDRSWAP_DSCL_ENABLE
  
  #ifdef DOWN_SCALE_SUPPORT
  prParsingPic = (VDEC_INFO_RM_PICINFO_T*) &_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo;
  #endif //RM_ADDRSWAP_DSCL_ENABLE

  u4YBase = 0;
  u4Height = 0;
  u4Width = 0;
  
  #ifdef EXT_COMPARE 
    //NULL
  #else //EXT_COMPARE
  
  #if defined(DOWN_SCALE_SUPPORT)
  #ifdef RM_ADDRSWAP_DSCL_ENABLE
  //HalFlushInvalidateDCache();

  memset(_pucDumpYBuf_1[u4InstID], 0x0, GOLD_Y_SZ);
  memset(_pucDumpCBuf_1[u4InstID], 0x0, GOLD_C_SZ);
  //memset(_pucVDSCLBuf_1[u4InstID], 0x0, VDSCL_BUF_SZ_1);
  
//  HalFlushInvalidateDCache();

  vRM_InvAddressSwap(u4InstID, 1);

//  HalFlushInvalidateDCache();

  u4YBase = (UINT32) _pucDumpYBuf_1[u4InstID];
  #else //RM_ADDRSWAP_DSCL_ENABLE
  u4YBase = (UINT32)_pucVDSCLBuf[u4InstID];
  #endif //RM_ADDRSWAP_DSCL_ENABLE
  
  u4Width = _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4DispW;

  if (prParsingPic->fgInterlace)
  {
    u4Height = _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4TrgOffV + _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4TrgHeight;
    ASSERT(0);
  }
  else
  {
    u4Height = _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4TrgOffV + _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4TrgHeight;
  }
  #else //defined(DOWN_SCALE_SUPPORT)
    // NULL
  #endif //defined(DOWN_SCALE_SUPPORT)

  //fred add for 32byte align in height
  u4Height = ((u4Height>>5)<<5);
  #ifdef DIRECT_DEC
    // NULL
  #endif //DIRECT_DEC
  {
    for (u4Pix = 0; u4Pix < u4Width*u4Height; u4Pix++)
    {
      //Always check each frame
      //if((_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.fgDec2ndFld) || (_tVerMpvDecPrm[u4InstID].ucPicStruct == FRM_PIC))
      {
        pbDecBuf = (UCHAR*)(u4YBase+u4Pix);
        pbGoldenBuf = (UCHAR*)((UINT32)_pucDumpYBuf[u4InstID]+u4Pix);
        
        if ((*(pbDecBuf)) != (*(pbGoldenBuf)))
        {
          vRM_DumpDownScalerParam(u4InstID);
        
          ASSERT(0);
        }            
      }
    }          
  }
  #endif //EXT_COMPARE

  // CbCr compare
  //if(!fgIsMonoPic(_u4VDecID))
  {
    // CbCr decoded data Compare
    #ifdef DIRECT_DEC
      // NULL
    #endif //DIRECT_DEC      
    {
      #ifdef RM_ADDRSWAP_DSCL_ENABLE
      u4CBase = (UINT32) _pucDumpCBuf_1[u4InstID];
      //u4CBase = (UINT32)_pucVDSCLBuf_1[u4InstID] + 0x1FE000;
      #else //RM_ADDRSWAP_DSCL_ENABLE
      u4CBase = (UINT32)_pucVDSCLBuf[u4InstID] + 0x1FE000;
      #endif //RM_ADDRSWAP_DSCL_ENABLE
            
      for (u4Pix = 0; u4Pix < u4Width*(u4Height>>1); u4Pix++)
      {
        //Always check each frame
        //if((_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.fgDec2ndFld) || (_tVerMpvDecPrm[u4InstID].ucPicStruct == FRM_PIC))
        {
          pbDecBuf = (UCHAR*)(u4CBase+u4Pix);
          pbGoldenBuf = (UCHAR*)((UINT32)_pucDumpCBuf[u4InstID]+u4Pix);

          if ((((*(pbDecBuf)) > (*(pbGoldenBuf))) && ((*(pbDecBuf))<= (*(pbGoldenBuf) + DSCL_TOLERATED_RANGE)))  || 
               (((*(pbDecBuf)) < (*(pbGoldenBuf))) && ((*(pbDecBuf))>= (*(pbGoldenBuf) - DSCL_TOLERATED_RANGE))) )
          //if ((*(pbDecBuf)) > (*(pbGoldenBuf)) && (*(pbDecBuf))<= (*(pbGoldenBuf) + DSCL_TOLERATED_RANGE)  || 
          //     (*(pbDecBuf)) < (*(pbGoldenBuf)) && (*(pbDecBuf))>= (*(pbGoldenBuf) - DSCL_TOLERATED_RANGE) )
          {
            //Pass
            //How the C Code round off floating number method is not the same as HW in full agreement
            //Therefor, difference between +-1 is tolerated
            //printk("Chroma round off floating number: %x %x \n", *(pbDecBuf), *(pbGoldenBuf));
            u4ErrorCnt++;
          }
          else if ((*(pbDecBuf)) != (*(pbGoldenBuf)))
          //if ((*(pbDecBuf)) != (*(pbGoldenBuf)))
          {
            vRM_DumpDownScalerParam(u4InstID);
            
            ASSERT(0);
          }
        }
      }   
    }
    //#endif    
  }
  //#endif

  

  return TRUE;
}


void vRM_dscl (UINT32 u4InstID, int srcWidth, int srcHeight, int targetWidth, int targetHeight, 
        UCHAR *srcData, UCHAR *targetData, int component, int pic_stru,
        int tgbufLen, int h_ofst, int v_ofst,
        int src_agent, int spec_type, int mbaff, int fg_src,
	 int src_h_ofst, int src_v_ofst, int scl_h_ofst, int scl_v_ofst,
	 BOOL lk_en, UINT32 luma_key)
// component: 0 => Y, 1 => C
{
  int srcbufLen;
    UINT8 *tempbuffer;
    int factor, factor_eval;
    int M, c, next_c, OF, next_OF;
    int weight; //, weight_n;
    int tg_pxl_cnt, tg_line_cnt;
    int i, j, round;
    int temp, temp_round, temp2, temp_round2;
    int hbuff, hbuff2;
    int h_result, h_result2;
    int v_result;
    int vbuff[4096];
    int v_ofst_real;
    int tmp_src_v_ofst;
    int v_region_0, v_region_1, rgn_change;


    #if 0
    printk("=====> vdscl parameter Log - Start \n");
    printk("srcWidth, srcHeight, targetWidth, targetHeight %x %x %x %x \n", srcWidth, srcHeight, targetWidth, targetHeight);
    printk("srcData, targetData, component, pic_stru %x %x %x %x \n", srcData, targetData, component, pic_stru);
    printk("tgbufLen, h_ofst, v_ofst %x %x %x \n", tgbufLen, h_ofst, v_ofst);
    printk("src_agent, spec_type, mbaff, fg_src %x %x %x %x \n", src_agent, spec_type, mbaff, fg_src);
    printk("src_h_ofst, src_v_ofst, scl_h_ofst, scl_v_ofst %x %x %x %x \n", src_h_ofst, src_v_ofst, scl_h_ofst, scl_v_ofst);
    printk("=====> vdscl parameter Log - End \n");
    #endif //0
    
        
    srcbufLen = ((srcWidth+15)/16)*16;
//  tgbufLen = ((targetWidth+15)/16)*16;
    tempbuffer = (UCHAR*)_pucVDSCLWork1Sa[u4InstID];
    //tempbuffer = (UINT8 *) malloc(tgbufLen*srcHeight*sizeof(UINT8));

    if ((pic_stru == 1) || (pic_stru == 2))
        v_ofst_real = v_ofst/2;
    else
        v_ofst_real = v_ofst;
        
    // Horizontal down scaling ---------------------------------------------------
    factor = (0x8000 * targetWidth + srcWidth/2) / srcWidth;
    M = 0x8000 - factor ;
    c = 0;
    next_c = 0;
    factor_eval = (factor >> 4);
    for (j=0; j<srcHeight; j++)
    {
        tg_pxl_cnt = 0;
        hbuff = 0;
        //c = 0;
        // @06102008
        c = scl_h_ofst;
        next_c = 0;
        rgn_change = 0;
        if (component == 0) // Y
        {
            //for (i=1; i<srcWidth; i++)
            // @06102008
            for (i=(1+src_h_ofst); i<srcWidth; i++)
            {
                c= (c+M)%0x8000;
                next_c = (c+M)%0x8000;
                OF = (c<M) ? 1 : 0;
                next_OF = (next_c<M) ? 1 : 0;
                weight = (c >> 4);
                //weight_n = (next_c >> 4);
    
                if (OF == 0 && next_OF == 0)
                    temp = weight * srcData[j*srcbufLen+i] + 
                        (2048-weight) * srcData[j*srcbufLen+i-1];
                else if (OF == 0 && next_OF == 1)
                    temp = factor_eval * srcData[j*srcbufLen+i] +
                        (2048-weight) * srcData[j*srcbufLen+i-1];
                else if (OF == 1 && next_OF == 0)
                    temp = weight * srcData[j*srcbufLen+i] + 128 * hbuff;
                else
                    temp = factor_eval * srcData[j*srcbufLen+i] + 128 * hbuff;
    
    						v_region_0 = srcData[j*srcbufLen+i-1] > luma_key;
    						v_region_1 = srcData[j*srcbufLen+i]   > luma_key;
    						
    						if (((i%16) == 0) && (OF == 1))
    								rgn_change = 1;
    					  else if (((i%16) == 0) && (v_region_0 == v_region_1))
    								rgn_change = 0;
    						else if (v_region_0 != v_region_1)
    								rgn_change = 1;
    								
                // 12-bit horizontal result
                temp_round = (temp >= 0x7f800) ? 0xff0 : (temp / 0x80);
                // 8-bit horizontal result (after rounding)
                h_result = (temp_round + 8) / 0x10;
    
                if (tg_pxl_cnt < targetWidth)
                {
                    if (next_OF == 0)
                    {
                        if (lk_en && (OF == 1) && (rgn_change == 1))
                        {
                        		tempbuffer[j*tgbufLen + tg_pxl_cnt + h_ofst] = srcData[j*srcbufLen+i];
                        		rgn_change = 0;
                        }
                        else if (lk_en && (rgn_change == 1))
                        {
                        		tempbuffer[j*tgbufLen + tg_pxl_cnt + h_ofst] = srcData[j*srcbufLen+i-1];
                        		rgn_change = 0;
                        }
                        else
                        tempbuffer[j*tgbufLen + tg_pxl_cnt + h_ofst] = (UINT8) h_result; // 8 bit
                        tg_pxl_cnt = tg_pxl_cnt + 1;
                    }
                    else
                        hbuff = temp_round; // 12 bit
                }
                else
                    break;

                // debug
                /*
                if ((tg_pxl_cnt == targetWidth) && (next_OF == 0))
                {
                    printf ("h_result(%d, %d) = %3x\n", i, j, h_result);
                }*/ 
                /*
                if ((j == 15) && (tg_pxl_cnt >= (targetWidth - 4)))
                {
                    printf ("h_result(%d, %d) = %3x, srcData = %3x, hbuff = %3x\n", 
                            i, j, h_result, srcData[j*srcbufLen+srcWidth-1], hbuff);
                }
                */
                /*
                if ((i>333) && (i<338) && (j>61) && (j<65))
                {
                    printf ("hbuff(%d, %d) = %3x, hcoeff = %3x, \n", i, j, hbuff, c);
                }
                */
                // debug
            }
            while (tg_pxl_cnt < targetWidth)
            {
                c= (c+M)%0x8000;
                next_c = (c+M)%0x8000;
                OF = (c<M) ? 1 : 0;
                next_OF = (next_c<M) ? 1 : 0;
                weight = (c >> 4);
                //weight_n = (next_c >> 4);
                
                if (OF == 0 && next_OF == 0)
                    temp = 2048 * srcData[j*srcbufLen+srcWidth-1];
                else if (OF == 0 && next_OF == 1)
                    temp = (2048-weight+factor_eval) * srcData[j*srcbufLen+srcWidth-1];
                else if (OF == 1 && next_OF == 0)
                    temp = weight * srcData[j*srcbufLen+srcWidth-1] + 128 * hbuff;
                else
                    temp = factor_eval * srcData[j*srcbufLen+srcWidth-1] + 128 * hbuff;
    
                // 12-bit horizontal result
                temp_round = (temp >= 0x7f800) ? 0xff0 : (temp / 0x80);
                // 8-bit horizontal result (after rounding)
                h_result = (temp_round + 8) / 0x10;
    
                if (next_OF == 0)
                {
                    if (lk_en && (OF == 1) && (rgn_change == 1))
                    {
                    		tempbuffer[j*tgbufLen + tg_pxl_cnt + h_ofst] = srcData[j*srcbufLen+srcWidth-1];
                    		rgn_change = 0;
                    }
                    else if (lk_en && (rgn_change == 1))
                    {
                    		tempbuffer[j*tgbufLen + tg_pxl_cnt + h_ofst] = srcData[j*srcbufLen+srcWidth-1];
                    		rgn_change = 0;
                    }
                    else
                    tempbuffer[j*tgbufLen + tg_pxl_cnt + h_ofst] = (UINT8) h_result; // 8 bit
                    tg_pxl_cnt = tg_pxl_cnt + 1;
                }
                else
                    hbuff = temp_round; // 12 bit
                // debug
                /*
                if ((j == 15) && (tg_pxl_cnt == targetWidth) && (next_OF == 0))
                {
                    printf ("h_result(%d, %d) = %3x, srcData = %3x, hbuff = %3x\n", 
                            i, j, h_result, srcData[j*srcbufLen+srcWidth-1], hbuff);
                }
                */
                // debug
            }
        }
        else // C
        {
            for (i=2*(1+src_h_ofst); i<srcWidth; i=i+2)
            {
                c= (c+M)%0x8000;
                next_c = (c+M)%0x8000;
                OF = (c<M) ? 1 : 0;
                next_OF = (next_c<M) ? 1 : 0;
                weight = (c >> 4);
                //weight_n = (next_c >> 4);
    
                if (OF == 0 && next_OF == 0)
                {
                    temp = weight * srcData[j*srcbufLen+i] + 
                        (2048-weight) * srcData[j*srcbufLen+i-2];
                    temp2 = weight * srcData[j*srcbufLen+i+1] + 
                        (2048-weight) * srcData[j*srcbufLen+i-1];
                }
                else if (OF == 0 && next_OF == 1)
                {
                    temp = factor_eval * srcData[j*srcbufLen+i] +
                        (2048-weight) * srcData[j*srcbufLen+i-2];
                    temp2 = factor_eval * srcData[j*srcbufLen+i+1] +
                        (2048-weight) * srcData[j*srcbufLen+i-1];
                }
                else if (OF == 1 && next_OF == 0)
                {
                    temp = weight * srcData[j*srcbufLen+i] + 128 * hbuff;
                    temp2 = weight * srcData[j*srcbufLen+i+1] + 128 * hbuff2;
                }
                else
                {
                    temp = factor_eval * srcData[j*srcbufLen+i] + 128 * hbuff;
                    temp2 = factor_eval * srcData[j*srcbufLen+i+1] + 128 * hbuff2;
                }
    
                // 12-bit horizontal result
                temp_round = (temp >= 0x7f800) ? 0xff0 : (temp / 0x80);
                temp_round2 = (temp2 >= 0x7f800) ? 0xff0 : (temp2 / 0x80);
                // 8-bit horizontal result (after rounding)
                h_result = (temp_round + 8) / 0x10;
                h_result2 = (temp_round2 + 8) / 0x10;
    
                if (tg_pxl_cnt < targetWidth)
                {
                    if (next_OF == 0)
                    {
                        tempbuffer[j*tgbufLen + tg_pxl_cnt + h_ofst] = (UINT8)h_result; // 8 bit
                        tempbuffer[j*tgbufLen + tg_pxl_cnt + h_ofst + 1] = (UINT8)h_result2; // 8 bit
                        tg_pxl_cnt = tg_pxl_cnt + 2;
                    }
                    else
                    {
                        hbuff = temp_round; // 12 bit
                        hbuff2 = temp_round2; // 12 bit
                    }
                }
                else
                    break;
            }

            while (tg_pxl_cnt < targetWidth)
            {
                c= (c+M)%0x8000;
                next_c = (c+M)%0x8000;
                OF = (c<M) ? 1 : 0;
                next_OF = (next_c<M) ? 1 : 0;
                weight = (c >> 4);
                //weight_n = (next_c >> 4);

                if (OF == 0 && next_OF == 0)
                {
                    temp = 2048 * srcData[j*srcbufLen+srcWidth-2];
                    temp2 = 2048 * srcData[j*srcbufLen+srcWidth-1];
                }
                else if (OF == 0 && next_OF == 1)
                {
                    temp = (2048-weight+factor_eval) * srcData[j*srcbufLen+srcWidth-2];
                    temp2 = (2048-weight+factor_eval) * srcData[j*srcbufLen+srcWidth-1];
                }
                else if (OF == 1 && next_OF == 0)
                {
                    temp = weight * srcData[j*srcbufLen+srcWidth-2] + 128 * hbuff;
                    temp2 = weight * srcData[j*srcbufLen+srcWidth-1] + 128 * hbuff2;
                }
                else
                {
                    temp = factor_eval * srcData[j*srcbufLen+srcWidth-2] + 128 * hbuff;
                    temp2 = factor_eval * srcData[j*srcbufLen+srcWidth-1] + 128 * hbuff2;
                }
    
                // 12-bit horizontal result
                temp_round = (temp >= 0x7f800) ? 0xff0 : (temp / 0x80);
                temp_round2 = (temp2 >= 0x7f800) ? 0xff0 : (temp2 / 0x80);
                // 8-bit horizontal result (after rounding)
                h_result = (temp_round + 8) / 0x10;
                h_result2 = (temp_round2 + 8) / 0x10;

                if (next_OF == 0)
                {
                    tempbuffer[j*tgbufLen + tg_pxl_cnt + h_ofst] = (UINT8)h_result; // 8 bit
                    tempbuffer[j*tgbufLen + tg_pxl_cnt + h_ofst + 1] = (UINT8)h_result2; // 8 bit
                    tg_pxl_cnt = tg_pxl_cnt + 2;
                }
                else
                {
                    hbuff = temp_round; // 12 bit
                    hbuff2 = temp_round2; // 12 bit
                }
            }
        }
    }
    
    // Vertical down scaling ---------------------------------------------------
    factor = (0x8000 * targetHeight + srcHeight/2) / srcHeight;
    M = 0x8000 - factor ;
    //c = 0;
    // @06102008
    c = scl_v_ofst;
    next_c = 0;
    tg_line_cnt = 0;
    factor_eval = (factor >> 4);

    if ((pic_stru != 3) && (pic_stru != 4)) // single picture
    {
        //for (i=1; i<srcHeight; i++)
        for (i=(1+src_v_ofst); i<srcHeight; i++)
        {
            c= (c+M)%0x8000;
            next_c = (c+M)%0x8000;
            OF = (c<M) ? 1 : 0;
            next_OF = (next_c<M) ? 1 : 0;
            weight = (c >> 4);
            //weight_n = (next_c >> 4);
                    
            for (j=0; j<targetWidth; j++)
            {
                if (OF == 0 && next_OF == 0)
                    temp = weight * tempbuffer[i*tgbufLen+h_ofst+j] + 
                        (2048-weight) * tempbuffer[(i-1)*tgbufLen+h_ofst+j];
                else if (OF == 0 && next_OF == 1)
                    temp = factor_eval * tempbuffer[i*tgbufLen+h_ofst+j] +
                        (2048-weight) * tempbuffer[(i-1)*tgbufLen+h_ofst+j];
                else if (OF == 1 && next_OF == 0)
                    temp = weight * tempbuffer[i*tgbufLen+h_ofst+j] + 128 * vbuff[j];
                else
                    temp = factor_eval * tempbuffer[i*tgbufLen+h_ofst+j] + 128 * vbuff[j];
    
                // 12-bit vertical result
                temp_round = (temp >= 0x7f800) ? 0xff0 : (temp / 0x80);
                // 8-bit vertical result
                v_result = (temp_round + 8) / 0x10;
        
                if (next_OF == 0)
                    targetData[(tg_line_cnt + v_ofst_real)*tgbufLen + h_ofst + j] = (UINT8)v_result; // 8 bit
                else
                {   
                    if (src_agent == 4) // fgt
                    {
                        if (fg_src == 1) // pp
                        {
                            if (((i%16) == 7) && (i < (srcHeight-9)) && (component == 0)) // Y
                                vbuff[j] = v_result * 0x10; // 12 bit
                            else if (((i%8) == 7) && (i < (srcHeight-9)) && (component == 1)) // C
                                vbuff[j] = v_result * 0x10; // 12 bit
                            else
                                vbuff[j] = temp_round; // 12 bit
                        }
                        else // mc
                        {
                            if (((i%16) == 15) && (i < (srcHeight-1)) && (component == 0)) // Y
                                vbuff[j] = v_result * 0x10; // 12 bit
                            else if (((i%8) == 7) && (i < (srcHeight-1)) && (component == 1)) // C
                                vbuff[j] = v_result * 0x10; // 12 bit
                            else
                                vbuff[j] = temp_round; // 12 bit
                        }
                    }
                    else if (src_agent == 2) // pp
                    {
                        if (spec_type == 1) // WMV
                        {
                            if (((i%16) == 7) && (i < (srcHeight-9)) && (component == 0)) // Y
                                vbuff[j] = v_result * 0x10; // 12 bit
                            else if (((i%8) == 7) && (i < (srcHeight-9)) && (component == 1)) // C
                                vbuff[j] = v_result * 0x10; // 12 bit
                            else
                                vbuff[j] = temp_round; // 12 bit
                        }
                        else if ((spec_type == 2) && (mbaff == 1)) // 264mbaff
                        {
                            if (((i%32) == 23) && (i < (srcHeight-9)) && (component == 0)) // Y
                                vbuff[j] = v_result * 0x10; // 12 bit
                            else if (((i%16) == 11) && (i < (srcHeight-5)) && (component == 1)) // C
                                vbuff[j] = v_result * 0x10; // 12 bit
                            else
                                vbuff[j] = temp_round; // 12 bit
                        }
                        else if (spec_type == 2)// 264normal
                        {
                            if (((i%16) == 11) && (i < (srcHeight-5)) && (component == 0)) // Y
                                vbuff[j] = v_result * 0x10; // 12 bit
                            else if (((i%8) == 5) && (i < (srcHeight-3)) && (component == 1)) // C
                                vbuff[j] = v_result * 0x10; // 12 bit
                            else
                                vbuff[j] = temp_round; // 12 bit
                        }
                        else // only RV support pp out, mp2, mp4 only mc out
                        {
                            if (((i%16) == 7) && (i < (srcHeight-9)) && (component == 0)) // Y
                                vbuff[j] = v_result * 0x10; // 12 bit
                            else if (((i%8) == 3) && (i < (srcHeight-5)) && (component == 1)) // C
                                vbuff[j] = v_result * 0x10; // 12 bit
                            else
                                vbuff[j] = temp_round; // 12 bit                            
                        }
                    }
                    else // mc
                    {
                        if ((spec_type == 2) && (mbaff == 1)) // 264mbaff
                        {
                            if (((i%32) == 31) && (i < (srcHeight-1)) && (component == 0)) // Y
                                vbuff[j] = v_result * 0x10; // 12 bit
                            else if (((i%16) == 15) && (i < (srcHeight-1)) && (component == 1)) // C
                                vbuff[j] = v_result * 0x10; // 12 bit
                            else
                                vbuff[j] = temp_round; // 12 bit
                        }
                        else
                        {
                            if (((i%16) == 15) && (i < (srcHeight-1)) && (component == 0)) // Y
                                vbuff[j] = v_result * 0x10; // 12 bit
                            else if (((i%8) == 7) && (i < (srcHeight-1)) && (component == 1)) // C
                                vbuff[j] = v_result * 0x10; // 12 bit
                            else
                                vbuff[j] = temp_round; // 12 bit
                        }
                    }
                }
    
                // debug
                /*
                if ((j>187) && (j<193) && (i>61) && (i<65))
                    printf ("vbuff(%d, %d) = %3x\n", j, i, vbuff[j]);
                */
                // debug
            }
            tg_line_cnt = (next_OF == 0) ? (tg_line_cnt+1) : tg_line_cnt;
            if (tg_line_cnt >= targetHeight)
                break;
        }
        
        while (tg_line_cnt < targetHeight)
        {
            c= (c+M)%0x8000;
            next_c = (c+M)%0x8000;
            OF = (c<M) ? 1 : 0;
            next_OF = (next_c<M) ? 1 : 0;
            weight = (c >> 4);
            //weight_n = (next_c >> 4);
                    
            for (j=0; j<targetWidth; j++)
            {
                if (OF == 0 && next_OF == 0)
                    temp = weight * tempbuffer[(srcHeight-1)*tgbufLen+h_ofst+j] + 
                        (2048-weight) * tempbuffer[(srcHeight-1)*tgbufLen+h_ofst+j];
                else if (OF == 0 && next_OF == 1)
                    temp = factor_eval * tempbuffer[(srcHeight-1)*tgbufLen+h_ofst+j] +
                        (2048-weight) * tempbuffer[(srcHeight-1)*tgbufLen+h_ofst+j];
                else if (OF == 1 && next_OF == 0)
                    temp = weight * tempbuffer[(srcHeight-1)*tgbufLen+h_ofst+j] + 128 * vbuff[j];
                else
                    temp = factor_eval * tempbuffer[(srcHeight-1)*tgbufLen+h_ofst+j] + 128 * vbuff[j];
    
                // 12-bit vertical result
                temp_round = (temp >= 0x7f800) ? 0xff0 : (temp / 0x80);
                // 8-bit vertical result
                v_result = (temp_round + 8) / 0x10;
        
                if (next_OF == 0)
                    targetData[(tg_line_cnt+v_ofst_real)*tgbufLen + h_ofst + j] = (UINT8)v_result; // 8 bit
                else
                    vbuff[j] = temp_round; // 12 bit
            }
            tg_line_cnt = (next_OF == 0) ? (tg_line_cnt+1) : tg_line_cnt;
        }
    }
    else // interlaced frame
    {
                //if (pic_stru == 4) // both fields
                src_v_ofst = src_v_ofst * 2;

        for (round=2; round<=3; round++)
        {
            // c = 0;
                        // @07022008
                        // @02262009
                        if ((round == 2) && (pic_stru == 3)) // top field of two field mode
                        {
                            tmp_src_v_ofst = src_v_ofst;
                            src_v_ofst = 0;
                        }
                        else if ((round == 3) && (pic_stru == 3)) // bot field of two field mode
                        {
                            src_v_ofst = tmp_src_v_ofst;
                        }

                        if ((round == 2) && (pic_stru == 3)) // top field of two field mode
                            c = 0;
                        else
                            c = scl_v_ofst;
            next_c = 0;
            tg_line_cnt = round-2;
            
            for (i=(round+src_v_ofst); i<srcHeight; i=i+2)
            {
                c= (c+M)%0x8000;
                next_c = (c+M)%0x8000;
                OF = (c<M) ? 1 : 0;
                next_OF = (next_c<M) ? 1 : 0;
                weight = (c >> 4);
                //weight_n = (next_c >> 4);
                
                for (j=0; j<targetWidth; j++)
                {
                    if (OF == 0 && next_OF == 0)
                        temp = weight * tempbuffer[i*tgbufLen+h_ofst+j] + 
                            (2048-weight) * tempbuffer[(i-2)*tgbufLen+h_ofst+j];
                    else if (OF == 0 && next_OF == 1)
                        temp = factor_eval * tempbuffer[i*tgbufLen+h_ofst+j] +
                            (2048-weight) * tempbuffer[(i-2)*tgbufLen+h_ofst+j];
                    else if (OF == 1 && next_OF == 0)
                        temp = weight * tempbuffer[i*tgbufLen+h_ofst+j] + 128 * vbuff[j];
                    else
                        temp = factor_eval * tempbuffer[i*tgbufLen+h_ofst+j] + 128 * vbuff[j];
        
                    // 12-bit vertical result
                    temp_round = (temp >= 0x7f800) ? 0xff0 : (temp / 0x80);
                    // 8-bit vertical result
                    v_result = (temp_round + 8) / 0x10;
            
                    if (next_OF == 0)
                        targetData[(tg_line_cnt+v_ofst_real)*tgbufLen + h_ofst + j] = (UINT8)v_result; // 8 bit
                    else if (pic_stru == 3)
                    {
                        if (src_agent == 4) // fgt
                        {
                            if (fg_src == 1) // pp
                            {
                                if ((((i%16) == 7) || ((i%16) == 6)) &&
                                        (i < (srcHeight-10)) && (component == 0)) // Y
                                    vbuff[j] = v_result * 0x10; // 12 bit
                                else if ((((i%8) == 7) || ((i%8) == 6)) &&
                                        (i < (srcHeight-10)) && (component == 1)) // C
                                    vbuff[j] = v_result * 0x10; // 12 bit
                                else
                                    vbuff[j] = temp_round; // 12 bit
                            }
                            else // mc
                            {
                                if ((((i%16) == 15) || ((i%16) == 14)) && 
                                        (i < (srcHeight-2)) && (component == 0)) // Y
                                    vbuff[j] = v_result * 0x10; // 12 bit
                                else if ((((i%8) == 7) || ((i%8) == 6)) && 
                                        (i < (srcHeight-2)) && (component == 1))
                                    vbuff[j] = v_result * 0x10; // 12 bit
                                else
                                    vbuff[j] = temp_round; // 12 bit
                            }
                            
                        }
                        else if (src_agent == 2) // pp
                        {
                            if (spec_type == 1) // WMV
                            {
                                if ((((i%16) == 7) || ((i%16) == 6)) &&
                                        (i < (srcHeight-10)) && (component == 0)) // Y
                                    vbuff[j] = v_result * 0x10; // 12 bit
                                else if ((((i%8) == 7) || ((i%8) == 6)) &&
                                        (i < (srcHeight-10)) && (component == 1)) // C
                                    vbuff[j] = v_result * 0x10; // 12 bit
                                else
                                    vbuff[j] = temp_round; // 12 bit
                            }
                            else if ((spec_type == 2) && (mbaff == 1)) // 264mbaff
                            {
                                if ((((i%32) == 23) || ((i%32) == 22)) &&
                                        (i < (srcHeight-10)) && (component == 0)) // Y
                                    vbuff[j] = v_result * 0x10; // 12 bit
                                else if ((((i%16) == 11) || ((i%16) == 10)) &&
                                        (i < (srcHeight-6)) && (component == 1)) // C
                                    vbuff[j] = v_result * 0x10; // 12 bit
                                else
                                    vbuff[j] = temp_round; // 12 bit
                            }
                            else if (spec_type == 2)// 264normal
                            {
                                if ((((i%16) == 11) || ((i%16) == 10)) &&
                                        (i < (srcHeight-6)) && (component == 0)) // Y
                                    vbuff[j] = v_result * 0x10; // 12 bit
                                else if ((((i%8) == 5) || ((i%8) == 4)) &&
                                        (i < (srcHeight-4)) && (component == 1)) // C
                                    vbuff[j] = v_result * 0x10; // 12 bit
                                else
                                    vbuff[j] = temp_round; // 12 bit
                            }
                            else // only RV support pp out, mp2, mp4 only mc out
                            {
                                if ((((i%16) == 7) || ((i%16) == 6)) &&
                                        (i < (srcHeight-10)) && (component == 0)) // Y
                                    vbuff[j] = v_result * 0x10; // 12 bit
                                else if ((((i%8) == 3) || ((i%8) == 2)) &&
                                        (i < (srcHeight-6)) && (component == 1)) // C
                                    vbuff[j] = v_result * 0x10; // 12 bit
                                else
                                    vbuff[j] = temp_round; // 12 bit                                
                            }
                        }
                        else // mc
                        {
                            if ((spec_type == 2) && (mbaff == 1)) // 264mbaff
                            {
                                if ((((i%32) == 31) || ((i%32) == 30)) && 
                                        (i < (srcHeight-2)) && (component == 0)) // Y
                                    vbuff[j] = v_result * 0x10; // 12 bit
                                else if ((((i%16) == 15) || ((i%16) == 14)) && 
                                        (i < (srcHeight-2)) && (component == 1))
                                    vbuff[j] = v_result * 0x10; // 12 bit
                                else
                                    vbuff[j] = temp_round; // 12 bit
                            }
                            else
                            {
                                if ((((i%16) == 15) || ((i%16) == 14)) && 
                                        (i < (srcHeight-2)) && (component == 0)) // Y
                                    vbuff[j] = v_result * 0x10; // 12 bit
                                else if ((((i%8) == 7) || ((i%8) == 6)) && 
                                        (i < (srcHeight-2)) && (component == 1))
                                    vbuff[j] = v_result * 0x10; // 12 bit
                                else
                                    vbuff[j] = temp_round; // 12 bit
                            }
                        }
                    }
                    else // both top & bottom fields, modified by cyc118 at 01202006
                    {
                        if (src_agent == 4) // fgt
                        {
                            if (fg_src == 1) // pp
                            {
                                if ((((i%32) == 15) || ((i%32) == 14)) &&
                                        (i < (srcHeight-18)) && (component == 0)) // Y
                                    vbuff[j] = v_result * 0x10; // 12 bit
                                else if ((((i%16) == 15) || ((i%16) == 14)) &&
                                        (i < (srcHeight-18)) && (component == 1)) // C
                                    vbuff[j] = v_result * 0x10; // 12 bit
                                else
                                    vbuff[j] = temp_round; // 12 bit
                            }
                            else // mc
                            {
                                if ((((i%32) == 31) || ((i%32) == 30)) && 
                                        (i < (srcHeight-2)) && (component == 0)) // Y
                                    vbuff[j] = v_result * 0x10; // 12 bit
                                else if ((((i%16) == 15) || ((i%16) == 14)) && 
                                        (i < (srcHeight-2)) && (component == 1))
                                     vbuff[j] = v_result * 0x10; // 12 bit
                                else
                                     vbuff[j] = temp_round; // 12 bit
                            }
                            
                        }
                        else if (src_agent == 2) // pp
                        {
                            if (spec_type == 1) // WMV
                            {
                                if ((((i%32) == 15) || ((i%32) == 14)) &&
                                        (i < (srcHeight-18)) && (component == 0)) // Y
                                    vbuff[j] = v_result * 0x10; // 12 bit
                                else if ((((i%16) == 15) || ((i%16) == 14)) &&
                                        (i < (srcHeight-18)) && (component == 1)) // C
                                    vbuff[j] = v_result * 0x10; // 12 bit
                                else
                                    vbuff[j] = temp_round; // 12 bit
                            }
                            else if ((spec_type == 2) && (mbaff == 1)) // 264mbaff
                            {
                                if ((((i%64) == 47) || ((i%64) == 46)) &&
                                        (i < (srcHeight-18)) && (component == 0)) // Y
                                    vbuff[j] = v_result * 0x10; // 12 bit
                                else if ((((i%32) == 23) || ((i%32) == 22)) &&
                                        (i < (srcHeight-10)) && (component == 1)) // C
                                    vbuff[j] = v_result * 0x10; // 12 bit
                                else
                                    vbuff[j] = temp_round; // 12 bit
                            }
                            else if (spec_type == 2)// 264normal
                            {
                                if ((((i%32) == 23) || ((i%32) == 22)) &&
                                        (i < (srcHeight-10)) && (component == 0)) // Y
                                    vbuff[j] = v_result * 0x10; // 12 bit
                                else if ((((i%16) == 11) || ((i%16) == 10)) &&
                                        (i < (srcHeight-6)) && (component == 1)) // C
                                    vbuff[j] = v_result * 0x10; // 12 bit
                                else
                                    vbuff[j] = temp_round; // 12 bit
                            }
                            else // only RV support pp out, mp2, mp4 only mc out
                            {
                                if ((((i%32) == 15) || ((i%32) == 14)) &&
                                        (i < (srcHeight-18)) && (component == 0)) // Y
                                    vbuff[j] = v_result * 0x10; // 12 bit
                                else if ((((i%16) == 7) || ((i%16) == 6)) &&
                                        (i < (srcHeight-10)) && (component == 1)) // C
                                    vbuff[j] = v_result * 0x10; // 12 bit
                                else
                                    vbuff[j] = temp_round; // 12 bit                                
                            }
                        }
                        else // mc
                        {
                            if ((spec_type == 2) && (mbaff == 1)) // 264mbaff
                            {
                                if ((((i%64) == 63) || ((i%64) == 62)) && 
                                        (i < (srcHeight-2)) && (component == 0)) // Y
                                    vbuff[j] = v_result * 0x10; // 12 bit
                                else if ((((i%32) == 31) || ((i%32) == 30)) && 
                                        (i < (srcHeight-2)) && (component == 1))
                                     vbuff[j] = v_result * 0x10; // 12 bit
                                else
                                     vbuff[j] = temp_round; // 12 bit
                            }
                            else
                            {
                                if ((((i%32) == 31) || ((i%32) == 30)) && 
                                        (i < (srcHeight-2)) && (component == 0)) // Y
                                    vbuff[j] = v_result * 0x10; // 12 bit
                                else if ((((i%16) == 15) || ((i%16) == 14)) && 
                                        (i < (srcHeight-2)) && (component == 1))
                                     vbuff[j] = v_result * 0x10; // 12 bit
                                else
                                     vbuff[j] = temp_round; // 12 bit
                            }
                        }
                    }
        
                    // debug
                    /*
                    if ((j>187) && (j<193) && (i>61) && (i<65))
                        printf ("vbuff(%d, %d) = %3x\n", j, i, vbuff[j]);
                    */
                    // debug
                }
                tg_line_cnt = (next_OF == 0) ? (tg_line_cnt+2) : tg_line_cnt;
                if (tg_line_cnt >= targetHeight)
                    break;
            }
            
            while (tg_line_cnt < targetHeight)
            {
                c= (c+M)%0x8000;
                next_c = (c+M)%0x8000;
                OF = (c<M) ? 1 : 0;
                next_OF = (next_c<M) ? 1 : 0;
                weight = (c >> 4);
                //weight_n = (next_c >> 4);
                
                for (j=0; j<targetWidth; j++)
                {
                    if (OF == 0 && next_OF == 0)
                        temp = weight * tempbuffer[(srcHeight+round-4)*tgbufLen+h_ofst+j] + 
                            (2048-weight) * tempbuffer[(srcHeight+round-4)*tgbufLen+h_ofst+j];
                    else if (OF == 0 && next_OF == 1)
                        temp = factor_eval * tempbuffer[(srcHeight+round-4)*tgbufLen+h_ofst+j] +
                            (2048-weight) * tempbuffer[(srcHeight+round-4)*tgbufLen+h_ofst+j];
                    else if (OF == 1 && next_OF == 0)
                        temp = weight * tempbuffer[(srcHeight+round-4)*tgbufLen+h_ofst+j] + 128 * vbuff[j];
                    else
                        temp = factor_eval * tempbuffer[(srcHeight+round-4)*tgbufLen+h_ofst+j] + 128 * vbuff[j];
        
                    // 12-bit vertical result
                    temp_round = (temp >= 0x7f800) ? 0xff0 : (temp / 0x80);
                    // 8-bit vertical result
                    v_result = (temp_round + 8) / 0x10;
            
                    if (next_OF == 0)
                        targetData[(tg_line_cnt+v_ofst_real)*tgbufLen + h_ofst + j] = (UINT8)v_result; // 8 bit
                    else
                        vbuff[j] = temp_round; // 12 bit
                }
                tg_line_cnt = (next_OF == 0) ? (tg_line_cnt+2) : tg_line_cnt;
            }
        }
    }
}

void vRM_GenerateDownScalerGolden(UINT32 u4InstID, UINT32 DecYAddr,UINT32 DecCAddr, UINT32 u4Size, 
	                                                                  BOOL lk_en, UINT32 luma_key)
{
    UCHAR *srcDataY, *srcDataC, *targetData, *VDSCLData;
    UCHAR *DecDataY, *DecDataC;
    int srcWidth, srcHeight, targetWidth, targetHeight;
    int pic_stru, tgbufLen, h_ofst, v_ofst;
    int src_agent, spec_type, mbaff;
    int v_fmt, convert_en, fg_src;
    UCHAR tmpbuf;
    int srcbufLen, tgbufHgh;
    int srcblkCnt, tgblkCnt, tgblkCnt16;
    int i, j, k, l,h;
#if (!CONFIG_DRV_MT8520)    
    int y_src_h_ofst, c_src_h_ofst, y_src_v_ofst, c_src_v_ofst;
    int y_scl_h_ofst, c_scl_h_ofst, y_scl_v_ofst, c_scl_v_ofst;
#endif

    // @04072009. support luma key
    //int lk_en, luma_key;
    int y_coord_0, y_coord_1, y_coord_2, y_coord_3;
    int lk_region0, lk_region1, lk_region2, lk_region3;

    // for color space conversion
    int Y601, Cb601, Cr601;
    int Y709, Cb709, Cr709;
    
    DecDataY = (UCHAR*)DecYAddr;
    DecDataC = (UCHAR*)DecCAddr;
    srcWidth     = _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4SrcWidth;
    srcHeight    = _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4SrcHeight;
    targetWidth  = _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4TrgWidth;
    targetHeight = _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4TrgHeight;

#if (!CONFIG_DRV_MT8520)  
    y_src_h_ofst = _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4SrcYOffH;
    c_src_h_ofst = _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4SrcCOffH;
    y_src_v_ofst = _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4SrcYOffV ;
    c_src_v_ofst = _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4SrcCOffV ;
    y_scl_h_ofst = _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4SclYOffH ;
    c_scl_h_ofst = _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4SclCOffH ;
    y_scl_v_ofst = _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4SclYOffV ;
    c_scl_v_ofst = _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4SclCOffV ;
#endif

  #if 0
    if((_tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucPicStruct == TOP_FIELD) ||(_tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucPicStruct == BOTTOM_FIELD))
    {
      pic_stru     = 4;
      srcHeight = srcHeight*2;
      targetHeight = targetHeight*2;
    }
    else
    {
      pic_stru     = _tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucPicStruct;
    }
  #endif
    switch (_tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucPicStruct)
    {
      case TOP_FIELD:
      case BOTTOM_FIELD:
        pic_stru     = 4;
        srcHeight = srcHeight*2;
        targetHeight = targetHeight*2;
        break;
      case FRAME:
        pic_stru = PIC_STRUCT_FRAME;
        break;
      default:
        pic_stru = PIC_STRUCT_TOP_BOTTOM;
        break;
    }

    
    tgbufLen     = _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4TrgBufLen;
    h_ofst       = _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4TrgOffH;
    v_ofst       = _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4TrgOffV;
    src_agent    = _tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucScrAgent;
    spec_type    = _tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucSpectType;
    v_fmt        = _tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucVdoFmt;
    convert_en   = _tVerMpvDecPrm[u4InstID].rDownScalerPrm.fgEnColorCvt;
    if(_u4CodecVer[u4InstID] == VDEC_H264)
    {
      if((_tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucPicStruct != TOP_FIELD) && (_tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucPicStruct != BOTTOM_FIELD))
      {
        mbaff      = _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecH264DecPrm.prSPS->fgMbAdaptiveFrameFieldFlag;
      }
      else
      {
        mbaff      = 0;
      }
      fg_src       = 1;
    }
    else
    {
      mbaff        = 0;
      fg_src       = 0;
    }

    // y
    srcbufLen  = ((srcWidth+15)/16)*16;
    srcblkCnt  = ((srcHeight+31)/32);
    //tgbufLen = ((targetWidth+15)/16)*16;
    tgbufHgh   = ((targetHeight + v_ofst + 31)/32)*32; // multiple of 32
    tgblkCnt   = tgbufHgh/32;
    tgblkCnt16 = tgbufHgh/16;
    
    srcDataY = (UCHAR *)_pucVDSCLWork2Sa[u4InstID];
    for(i=0;i<srcbufLen;i++)
      for(j=0;j<srcHeight;j++)
         *(srcDataY++) = 0;
    srcDataC = (UCHAR *) _pucVDSCLWork3Sa[u4InstID];
    for(i=0;i<srcbufLen;i++)
      for(j=0;j<(srcHeight>>1);j++)
         *(srcDataC++) = 0;
    targetData = (UCHAR *) _pucVDSCLWork4Sa[u4InstID];
    for(i=0;i<tgbufLen;i++)
      for(j=0;j<tgbufHgh;j++)
         *(targetData++) = 0;

    srcDataY = (UCHAR *)_pucVDSCLWork2Sa[u4InstID];
    srcDataC = (UCHAR *) _pucVDSCLWork3Sa[u4InstID];
    targetData = (UCHAR *) _pucVDSCLWork4Sa[u4InstID];
    // Y component in---------------------------------------------------------------

    for (i = 0; i < srcblkCnt; i++) // block count
        for (k=0; k < (srcbufLen/16); k++)
            for (j=0; j < 32; j++) // line count
                for (l=0; l<16; l++)
                {
                    if ((i*32+j) >= srcHeight)
                    {
                        tmpbuf = *(DecDataY++);  // unnecessary data
                    }
                    else
                    {
                        tmpbuf = *(DecDataY++);
                        srcDataY[(i*32+j)*srcbufLen+k*16+l] = (UCHAR)tmpbuf;
                    }
                }


    // C component in---------------------------------------------------------------
    for (i = 0; i < srcblkCnt; i++) // block count
        for (k=0; k < (srcbufLen/16); k++)
            for (j=0; j < 16; j++) // line count
                for (l=0; l<16; l++)
                {
                    if ((i*16+j) >= (srcHeight/2))
                    {
                        tmpbuf = *(DecDataC++);  // unnecessary data
                    }
                    else
                    {
                        tmpbuf = *(DecDataC++);
                        srcDataC[(i*16+j)*srcbufLen+k*16+l] = (UCHAR)tmpbuf;
                    }
                }

    // @04072009. support luma key
    if (lk_en == 1)
    {
        for (j=0; j < srcHeight/2; j++)
            for (i=0; i < srcWidth/2; i++)
            {
                y_coord_0 = srcbufLen*2*j+i*2;
                y_coord_1 = srcbufLen*(2*j+1)+i*2;
                y_coord_2 = srcbufLen*2*j+i*2+1;
                y_coord_3 = srcbufLen*(2*j+1)+i*2+1;
                lk_region0 = (srcDataY[y_coord_0] <= luma_key) ? 1 : 0;
                lk_region1 = (srcDataY[y_coord_1] <= luma_key) ? 1 : 0;
                lk_region2 = (srcDataY[y_coord_2] <= luma_key) ? 1 : 0;
                lk_region3 = (srcDataY[y_coord_3] <= luma_key) ? 1 : 0;
                if ((lk_region0>0) && (lk_region1>0) && (lk_region2>0) && (lk_region3>0))
                {
                    srcDataC[j*srcbufLen+i*2] = 0x80; // Cb
                    srcDataC[j*srcbufLen+i*2+1] = 0x80; // Cr
                }
            }
    }

    // color space conversion
    if (convert_en == 1)
    {
        // Y component conversion
        if ((pic_stru != 3) && (pic_stru != 4))
        {
            for (j=0; j < srcHeight; j++)
                for (i=0; i < srcWidth; i++)
                {
                    Y709  = (int) srcDataY[srcbufLen*j+i];
                    if ((i%2) == 0)
                    {
                        Cb709 = ((int) srcDataC[srcbufLen*(j/2)+i]) - 128;
                        Cr709 = ((int) srcDataC[srcbufLen*(j/2)+i+1]) - 128;
                    }
                    else
                    {
                        Cb709 = ((int) srcDataC[srcbufLen*(j/2)+i-1]) - 128;
                        Cr709 = ((int) srcDataC[srcbufLen*(j/2)+i]) - 128;
                    }
                    //Original
                    //Y601 = (int)(Y709 + Cb709 * 0.099609375 + Cr709 * 0.19140625 + 0.5);
                    // modified @07012008
				Y601 = (Y709*1024 + Cb709*102 + Cr709*196 + 512) >> 10;
                    if (Y601 > 255)
                        srcDataY[srcbufLen*j+i] = 255;
                    else if (Y601 <0)
                        srcDataY[srcbufLen*j+i] = 0;
                    else
                        srcDataY[srcbufLen*j+i] = (UCHAR) Y601;
                }
        }
        else // interlaced frame
        {
            for (j=0; j < srcHeight; j=j+2)
                for (i=0; i < srcWidth; i++)
                {
                    Y709  = (int) srcDataY[srcbufLen*j+i];
                    if ((i%2) == 0)
                    {
                        Cb709 = ((int) srcDataC[srcbufLen*2*(j/4)+i]) - 128;
                        Cr709 = ((int) srcDataC[srcbufLen*2*(j/4)+i+1]) - 128;
                    }
                    else
                    {
                        Cb709 = ((int) srcDataC[srcbufLen*2*(j/4)+i-1]) - 128;
                        Cr709 = ((int) srcDataC[srcbufLen*2*(j/4)+i]) - 128;
                    }
                    //Original
                    //Y601 = (int)(Y709 + Cb709 * 0.099609375 + Cr709 * 0.19140625 + 0.5);
                    // modified @07012008
				Y601 = (Y709*1024 + Cb709*102 + Cr709*196 + 512) >> 10;
                    if (Y601 > 255)
                        srcDataY[srcbufLen*j+i] = 255;
                    else if (Y601 <0)
                        srcDataY[srcbufLen*j+i] = 0;
                    else
                        srcDataY[srcbufLen*j+i] = (UCHAR) Y601;
                }

            for (j=1; j < srcHeight; j=j+2)
                for (i=0; i < srcWidth; i++)
                {
                    Y709  = (int) srcDataY[srcbufLen*j+i];
                    if ((i%2) == 0)
                    {
                        Cb709 = ((int) srcDataC[srcbufLen*(2*(j/4)+1)+i]) - 128;
                        Cr709 = ((int) srcDataC[srcbufLen*(2*(j/4)+1)+i+1]) - 128;
                    }
                    else
                    {
                        Cb709 = ((int) srcDataC[srcbufLen*(2*(j/4)+1)+i-1]) - 128;
                        Cr709 = ((int) srcDataC[srcbufLen*(2*(j/4)+1)+i]) - 128;
                    }
                    //Original
                    //Y601 = (int)(Y709 + Cb709 * 0.099609375 + Cr709 * 0.19140625 + 0.5);
                    // modified @07012008
				Y601 = (Y709*1024 + Cb709*102 + Cr709*196 + 512) >> 10;
                    if (Y601 > 255)
                        srcDataY[srcbufLen*j+i] = 255;
                    else if (Y601 <0)
                        srcDataY[srcbufLen*j+i] = 0;
                    else
                        srcDataY[srcbufLen*j+i] = (UCHAR) Y601;
                }
        }

        // C component conversion
        for (j=0; j < srcHeight/2; j++)
            for (i=0; i < srcWidth/2; i++)
            {
                Cb709 = ((int) srcDataC[srcbufLen*j+2*i]) - 128;
                Cr709 = ((int) srcDataC[srcbufLen*j+2*i+1]) - 128;
                
                 // original
			  // Cb601 = (int)(0.9902343750 * Cb709 - 0.1103515625 * Cr709 + 128.5);				    
			  // Cr601 = (int)(0.9833984375 * Cr709 - 0.0722656250 * Cb709 + 128.5);			      
			  // modified @07012008
			  Cb601 = (1014 * Cb709 - 113 * Cr709 + 131584) >> 10;
                Cr601 = (1007 * Cr709 - 74 * Cb709 + 131584) >> 10;
                if (Cb601 > 255)
                    srcDataC[srcbufLen*j+2*i] = 255;
                else if (Cb601 < 0)
                    srcDataC[srcbufLen*j+2*i] = 0;
                else
                    srcDataC[srcbufLen*j+2*i] = (UCHAR) Cb601;

                if (Cr601 > 255)
                    srcDataC[srcbufLen*j+2*i+1] = 255;
                else if (Cr601 < 0)
                    srcDataC[srcbufLen*j+2*i+1] = 0;
                else
                    srcDataC[srcbufLen*j+2*i+1] = (UCHAR) Cr601;
            }
    }

    // Evaluate Y component ---------------------------------------------------------------
    /**** do down scaling ****/
#if (CONFIG_DRV_MT8520) 
    vdscl (u4InstID, srcWidth, srcHeight, targetWidth, targetHeight, srcDataY, targetData, 0, pic_stru, 
            tgbufLen, h_ofst, v_ofst, src_agent, spec_type, mbaff, fg_src);
#else
    vRM_dscl (u4InstID, srcWidth, srcHeight, targetWidth, targetHeight, srcDataY, targetData, 0, pic_stru,
            tgbufLen, h_ofst, v_ofst, src_agent, spec_type, mbaff, fg_src,
            y_src_h_ofst, y_src_v_ofst, y_scl_h_ofst, y_scl_v_ofst, lk_en, luma_key);
#endif

    //PANDA
    //memset(_pucDumpYBuf[u4InstID], 0x0, GOLD_Y_SZ);
    //memset(_pucDumpCBuf[u4InstID], 0x0, GOLD_C_SZ);
    VDSCLData = (UCHAR *)_pucDumpYBuf[u4InstID];
    /**** output result to file (raster scan) ****/
    if(_tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucScanType==1)
    {
        for (j=0; j < tgbufHgh; j++)
        {
            //if (pic_stru == 2) // bot field
            if (_tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucPicStruct == BOTTOM_FIELD)
            {
                for (i=0; i < tgbufLen/16; i++)
                {
                    for(h=0;h<16;h++)
                    {
                        VDSCLData++;
                        //*(VDSCLData++) = 0;
                        //fwrite(&Zero[0],sizeof(char),16,file_outY_rs);
                    }
                }
                j++;
            }

            for (i=0; i < tgbufLen/16; i++)
            {
                for(h=0;h<16;h++)
                {
                    *(VDSCLData++) = targetData[j*tgbufLen+16*i+h];
                    //fwrite(&targetData[j*tgbufLen+16*i],sizeof(char),16,file_outY_rs);
                }
            }

            //if (pic_stru == 1) // top field
            if (_tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucPicStruct == TOP_FIELD)
            {
                for (i=0; i < tgbufLen/16; i++)
                {
                    for(h=0;h<16;h++)
                    {
                        VDSCLData++;
                        //*(VDSCLData++) = 0;
                        //fwrite(&Zero[0],sizeof(char),16,file_outY_rs);
                    }
                }
                j++;
            }
        }
    }
    else
    {
        /**** output result to file (block based) ****/
        if ((pic_stru == 0) || (pic_stru == 3) || (pic_stru == 4))
        // progressive or interlaced frame
        {
            for (i = 0; i < tgblkCnt; i++) // block count
            {
                for (k=0; k < (tgbufLen/16); k++)
                {
                    for (j=0; j < 32; j++) // line count
                    {
                        if ((i*32+j) >= tgbufHgh)
                        {
                            for(h=0;h<16;h++)
                            {
                                *(VDSCLData++) = 0;
                                //fwrite(&Zero[0],sizeof(char),16,file_outY_blk);
                            }
                        }
                        //PANDA
                        else if(((_tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucPicStruct == TOP_FIELD) && (j % 2 == 1)) || ((_tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucPicStruct == BOTTOM_FIELD) && (j % 2 == 0)))
                        {
                            for(h=0;h<16;h++)
                            {
                                VDSCLData++;
                            }
                        }
                        else
                        {
                            for(h=0;h<16;h++)
                            {
                                *(VDSCLData++) = targetData[(i*32+j)*tgbufLen+k*16+h];
                                //fwrite(&targetData[(i*32+j)*tgbufLen+k*16],sizeof(char),16,file_outY_blk);
                            }
                        }
                    }
                }
            }
        }
        else // field picture: top or bottom 
        {
            for (i = 0; i < tgblkCnt16; i++) // block count
            {
                for (k=0; k < (tgbufLen/16); k++)
                {
                    for (j=0; j < 16; j++) // line count
                    {
                        //if (pic_stru == 2) // bot field
                        if (_tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucPicStruct == BOTTOM_FIELD)
                        {
                            for(h=0;h<16;h++)
                            {
                                VDSCLData++;
                                //*(VDSCLData++) = 0;
                                //fwrite(&Zero[0],sizeof(char),16,file_outY_blk);
                            }
                        }

                        if ((i*16+j) >= tgbufHgh)
                        {
                            for(h=0;h<16;h++)
                            {
                                VDSCLData++;
                                //*(VDSCLData++) = 0;
                                //fwrite(&Zero[0],sizeof(char),16,file_outY_blk);
                            }
                        }
                        else
                        { 
                            for(h=0;h<16;h++)
                            {
                                *(VDSCLData++) = targetData[(i*16+j)*tgbufLen+k*16+h];
                                //fwrite(&targetData[(i*16+j)*tgbufLen+k*16],sizeof(char),16,file_outY_blk);
                            }
                        }
                        //if (pic_stru == 1) // top field
                        if (_tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucPicStruct == TOP_FIELD)
                        {
                            for(h=0;h<16;h++)
                            {
                                VDSCLData++;
                                //*(VDSCLData++) = 0;
                                //fwrite(&Zero[0],sizeof(char),16,file_outY_blk);
                            }
                        }
                    }
                }
            }
        }
    }


    //srcData = (UCHAR *) malloc(srcbufLen*srcHeight/2*sizeof(UCHAR));
    //memset(srcData, 0, srcbufLen*srcHeight/2);
    if (v_fmt == 1) // 422
    {
        targetData = (UCHAR *) _pucVDSCLWork4Sa[u4InstID];
        for(i=0;i<tgbufLen;i++)
          for(j=0;j<tgbufHgh;j++)
             *(targetData++) = 0;
    }
    else
    {
        targetData = (UCHAR *) _pucVDSCLWork4Sa[u4InstID];
        for(i=0;i<tgbufLen;i++)
          for(j=0;j<(tgbufHgh>>1);j++)
             *(targetData++) = 0;
    }
    targetData = (UCHAR *) _pucVDSCLWork4Sa[u4InstID];
    VDSCLData = (UCHAR *)_pucDumpCBuf[u4InstID];
    // Evaluate C component ---------------------------------------------------------------
    /**** do down scaling ****/
    if (v_fmt == 0) // 420
    {
        targetHeight = targetHeight/2;
        v_ofst = v_ofst/2;
        tgbufHgh = tgbufHgh/2;
        
    }
    else
    {
        tgblkCnt = tgblkCnt * 2;
        tgblkCnt16 = tgblkCnt16 *2;
    }

#if (CONFIG_DRV_MT8520) 
    vdscl (u4InstID, srcWidth, srcHeight/2, targetWidth, targetHeight, srcDataC, targetData, 1, pic_stru,
            tgbufLen, h_ofst, v_ofst, src_agent, spec_type, mbaff, fg_src);
#else
    vRM_dscl (u4InstID, srcWidth, srcHeight/2, targetWidth, targetHeight, srcDataC, targetData, 1, pic_stru,
            tgbufLen, h_ofst, v_ofst, src_agent, spec_type, mbaff, fg_src,
            c_src_h_ofst, c_src_v_ofst, c_scl_h_ofst, c_scl_v_ofst, lk_en, luma_key);
#endif

    if(_tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucScanType==1)
    {
        /**** output result to file (raster scan) ****/
        for (j=0; j < tgbufHgh; j++)
        {
            //if (pic_stru == 2) // bot field
            if (_tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucPicStruct == BOTTOM_FIELD)
            {
                for (i=0; i < tgbufLen/16; i++)
                {
                    for(h=0;h<16;h++)
                    {
                        VDSCLData++;
                        //*(VDSCLData++) = 0;
                        //fwrite(&Zero[0],sizeof(char),16,file_outC_rs);
                    }
                }
                j++;
            }

            for (i=0; i < tgbufLen/16; i++)
            {
                for(h=0;h<16;h++)
                {
                    *(VDSCLData++) = targetData[j*tgbufLen+16*i+h];
                    //fwrite(&targetData[j*tgbufLen+16*i],sizeof(char),16,file_outC_rs);
                }
            }

            //if (pic_stru == 1) // top field
            if (_tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucPicStruct == TOP_FIELD)
            {
                for (i=0; i < tgbufLen/16; i++)
                {
                    for(h=0;h<16;h++)
                    {
                        VDSCLData++;
                        //*(VDSCLData++) = 0;
                        //fwrite(&Zero[0],sizeof(char),16,file_outC_rs);
                    }
                }
                j++;
            }
        }
    }
    else
    {
        /**** output result to file (block based) ****/
        if ((pic_stru == 0) || (pic_stru == 3) || (pic_stru == 4))
        // progressive or interlaced frame
        {
            for (i = 0; i < tgblkCnt; i++) // block count
            {
                for (k=0; k < (tgbufLen/16); k++)
                {
                    for (j=0; j < 16; j++) // line count
                    {
                        if ((i*16+j) >= (tgbufHgh))
                        {
                            for(h=0;h<16;h++)
                            {
                                //*(VDSCLData++);
                                *(VDSCLData++) = 0;
                                //fwrite(&Zero[0],sizeof(char),16,file_outC_blk);
                            }
                        }
                        //PANDA
                        else if(((_tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucPicStruct == TOP_FIELD) && (j % 2 == 1)) || ((_tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucPicStruct == BOTTOM_FIELD) && (j % 2 == 0)))
                        {                           
                            for(h=0;h<16;h++)        
                            {                      
                                VDSCLData++;             
                            }                   
                        }
                        else
                        {
                            for(h=0;h<16;h++)
                            {
                                *(VDSCLData++) = targetData[(i*16+j)*tgbufLen+k*16+h];
                                //fwrite(&targetData[(i*16+j)*tgbufLen+k*16],sizeof(char),16,file_outC_blk);
                            }
                        }
                    }
                }
            }
        }
        else // field picture: top or bottom 
        {
            for (i = 0; i < tgblkCnt16; i++) // block count
            {
                for (k=0; k < (tgbufLen/16); k++)
                {
                    for (j=0; j < 8; j++) // line count
                    {
                        //if (pic_stru == 2) // bot field
                        if (_tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucPicStruct == BOTTOM_FIELD)
                        {
                            for(h=0;h<16;h++)
                            {
                                VDSCLData++;
                                //*(VDSCLData++) = 0;
                                //fwrite(&Zero[0],sizeof(char),16,file_outC_blk);
                            }
                        }

                        if ((i*8+j) >= (tgbufHgh))
                        {
                            for(h=0;h<16;h++)
                            {
                                VDSCLData++;
                                //*(VDSCLData++) = 0;
                                //fwrite(&Zero[0],sizeof(char),16,file_outC_blk);
                            }
                        }
                        else
                        {
                            for(h=0;h<16;h++)
                            {
                                *(VDSCLData++) = targetData[(i*8+j)*tgbufLen+k*16+h];
                                //fwrite(&targetData[(i*8+j)*tgbufLen+k*16],sizeof(char),16,file_outC_blk);
                            }
                        }

                        //if (pic_stru == 1) // top field
                        if (_tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucPicStruct == TOP_FIELD)
                        {
                            for(h=0;h<16;h++)
                            {
                                VDSCLData++;
                                //*(VDSCLData++) = 0;
                                //fwrite(&Zero[0],sizeof(char),16,file_outC_blk);
                            }
                        }
                    }
                }
            }
        }
    }

    return;
}

static void vDumpVldCkSum(void)
{
  printk("\n<vdec> %s\n", __FUNCTION__);
  u4VDecReadRMVLD(0, 33 * 4);
  u4VDecReadRMVLD(0, 34 * 4);
  u4VDecReadRMVLD(0, 35 * 4);
  u4VDecReadRMVLD(0, 36 * 4);
  u4VDecReadRMVLD(0, 37 * 4);
  u4VDecReadRMVLD(0, 43 * 4);
  u4VDecReadRMVLD(0, 44 * 4);
  u4VDecReadRMVLD(0, 48 * 4);
  u4VDecReadRMVLD(0, 59 * 4);
  u4VDecReadRMVLD(0, 60 * 4);
  u4VDecReadRMVLD(0, 67 * 4);
  u4VDecReadRMVLD(0, 63 * 4);
  u4VDecReadRMVLD(0, 64 * 4);
  u4VDecReadRMVLD(0, 65 * 4);
  u4VDecReadRMVLD(0, 196 * 4);
  u4VDecReadRMVLD(0, 53 * 4);
  u4VDecReadRMVLD(0, 62 * 4);
  /*
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
  */
}

static void vDumpAvcMvCkSum(void)
{
  printk("\n<vdec> %s\n", __FUNCTION__);
  u4VDecReadRMMV(0, 148 * 4);
  u4VDecReadRMMV(0, 149 * 4);
  u4VDecReadRMMV(0, 150 * 4);
  u4VDecReadRMMV(0, 151 * 4);
  /*
  printk("AVC MV 148: 0x%08X\n", u4ReadReg(AVC_MV_REG_OFFSET0 + 148 * 4));
  printk("AVC MV 149: 0x%08X\n", u4ReadReg(AVC_MV_REG_OFFSET0 + 149 * 4));
  printk("AVC MV 150: 0x%08X\n", u4ReadReg(AVC_MV_REG_OFFSET0 + 150 * 4));
  printk("AVC MV 151: 0x%08X\n", u4ReadReg(AVC_MV_REG_OFFSET0 + 151 * 4));
  */
}


void vDumpMCPerformance(void)
{
	printk("\n<vdec> %s\n", __FUNCTION__);
	u4VDecReadMC(0, 474 * 4);
	u4VDecReadMC(0, 476 * 4);
	u4VDecReadMC(0, 477 * 4);
	u4VDecReadMC(0, 478 * 4);
	u4VDecReadMC(0, 522 * 4);
  /*
  printk("MC 474: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 474 * 4));
  printk("MC 476: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 476 * 4));
  printk("MC 477: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 477 * 4));
  printk("MC 478: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 478 * 4));
  printk("MC 522: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 522 * 4));
  */
}


static void vDumpMcCkSum(void)
{
	printk("\n<vdec> %s\n", __FUNCTION__);
	u4VDecReadMC(0, 378 * 4);
	u4VDecReadMC(0, 379 * 4);
	u4VDecReadMC(0, 380 * 4);
	u4VDecReadMC(0, 381 * 4);
	u4VDecReadMC(0, 382 * 4);
	u4VDecReadMC(0, 388 * 4);
	u4VDecReadMC(0, 390 * 4);
	u4VDecReadMC(0, 391 * 4);
	u4VDecReadMC(0, 392 * 4);
	u4VDecReadMC(0, 393 * 4);
	u4VDecReadMC(0, 446 * 4);
	u4VDecReadMC(0, 447 * 4);
	u4VDecReadMC(0, 525 * 4);
  /*
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
  printk("MC 525: 0x%08X\n", u4ReadReg(MC_REG_OFFSET0 + 525 * 4));
  */
}

static void vDumpPpCkSum(void)
{
  int i;

  	printk("\n<vdec> %s\n", __FUNCTION__);
	u4VDecReadRMPP(0, 5 * 4);
    u4VDecReadRMPP(0, 6 * 4);
	u4VDecReadRMPP(0, 67 * 4);

	for (i = 69; i <= 76; i++)
		u4VDecReadRMPP(0, i * 4);

	for (i = 161; i <= 178; i++)
		u4VDecReadRMPP(0, i * 4);

	u4VDecReadRMPP(0, 67 * 4);
	u4VDecReadRMPP(0, 199 * 4);
	u4VDecReadRMPP(0, 200 * 4);
	u4VDecReadRMPP(0, 201 * 4);
	u4VDecReadRMPP(0, 227 * 4);

  /*
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
  */
}

static void vDumpReg(void)
{
  INT32 i;

  printk("\n<vdec> %s\n", __FUNCTION__);
  u4VDecReadRMMV(0, 131 * 4);
  u4VDecReadRMMV(0, 134 * 4);
  u4VDecReadRMMV(0, 135 * 4);
  
  u4VDecReadMC(0, 136 * 4);
  u4VDecReadMC(0, 137 * 4);
  u4VDecReadMC(0, 138 * 4);
  u4VDecReadMC(0, 139 * 4);
  u4VDecReadMC(0, 142 * 4);
  u4VDecReadMC(0, 148 * 4);
  u4VDecReadMC(0, 152 * 4);
  u4VDecReadMC(0, 153 * 4);
  u4VDecReadRMPP(0, 2 * 4);

  u4VDecReadMC(0, 420 * 4);
  u4VDecReadMC(0, 130 * 4);
  u4VDecReadMC(0, 131 * 4);
  u4VDecReadVLD(0, 36 * 4);
  u4VDecReadVLD(0, 50 * 4);
  u4VDecReadMC(0, 0 * 4);
  u4VDecReadMC(0, 1 * 4);
  u4VDecReadMC(0, 2 * 4);
  u4VDecReadMC(0, 3 * 4);
  u4VDecReadMC(0, 39 * 4);
  u4VDecReadMC(0, 40 * 4);
  u4VDecReadMC(0, 9 * 4);
  u4VDecReadMC(0, 28 * 4);
  u4VDecReadMC(0, 36 * 4);
  u4VDecReadMC(0, 6 * 4);
  u4VDecReadMC(0, 38 * 4);
  u4VDecReadMC(0, 114 * 4);

  u4VDecReadRMVLD(0, 39 * 4);
  u4VDecReadRMVLD(0, 66 * 4);
  u4VDecReadRMVLD(0, 33 * 4);
  u4VDecReadRMVLD(0, 34 * 4);
  u4VDecReadRMVLD(0, 35 * 4);
  u4VDecReadRMVLD(0, 36 * 4);
  u4VDecReadRMVLD(0, 37 * 4);

  u4VDecReadRMVLD(0, 42 * 4);
  printk("<vdec> RV VLD window: 0x%08X\n", _tVerMpvDecPrm[0].SpecDecPrm.rVDecRMDecPrm.u4HWInputWindow);
  #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
   printk("<vdec> TOPVLD Registers\n");
   for (i=0; i<100; i++)
   {
      u4VDecReadVLDTOP(0, (i<<2));
      //printk("TOP VLD%04d (0x%04X) = 0x%08X\n", i, (i<<2), u4VDecReadVLDTOP(0, (i<<2)));
   }
   #endif

   printk("<vdec> MC Registers\n");
   for (i=0; i<700; i++)
   {
      u4VDecReadMC(0, (i<<2));
      //printk("MC%04d (0x%04X) = 0x%08X\n", i, (i<<2), u4VDecReadMC(0, (i<<2)));
   }

   printk("<vdec> PP Registers\n");
   for (i=0; i<30; i++)
   {
      u4VDecReadRMPP(0, i << 2);
      //printk("PP%04d (0x%04X) = 0x%08X\n", i, (i<<2), u4ReadReg(RM_VDEC_PP_BASE + (i<<2)));
   }
}


void vCheckGlobalChecksum(UINT32 u4InstID)
{
  UINT32 au4ChecksumData[10];
  UINT32 u4ChecksumBuf = _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4GlobalChecksumBuf;
  #ifdef RM_DUMP_CHECKSUM
  UINT32 u4ChecksumBufSize = _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4GlobalChecksumBufSize;
  #endif //RM_DUMP_CHECKSUM
  UINT32 u4DecPicCnt = _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt;
  BYTE* pbChecksumBuf = (BYTE*) u4ChecksumBuf;
    
  
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

#ifdef RM_DUMP_CHECKSUM
//Keep in Momery
  //Keep Pic number 
  pbChecksumBuf = pbChecksumBuf + 44*u4DecPicCnt;
  memcpy(pbChecksumBuf, &u4DecPicCnt, 4);

  //printk("Keep Checksum Data to Temp Buffer %x \n", (UINT32) pbChecksumBuf);

  //Keep Checksum Data
  pbChecksumBuf = pbChecksumBuf + 4;
  memcpy(pbChecksumBuf, au4ChecksumData, 40);

  if (40*u4DecPicCnt > u4ChecksumBufSize)
  {
    ASSERT(0);
  }
#else //RM_DUMP_CHECKSUM
  //printk("Check HW Checksum with Golden Checksum \n");

  pbChecksumBuf = pbChecksumBuf + 40*u4DecPicCnt;
  pbChecksumBuf = pbChecksumBuf + 4*u4DecPicCnt + 4;

  if (memcmp(pbChecksumBuf, au4ChecksumData, 40) == 0)
  {
    // TODO:
  }
  else
  {
    ASSERT(0);
  }
#endif //RM_DUMP_CHECKSUM
}



//Result Check Function
// ===========================================================================
BOOL fgRMCheckCRCResult = TRUE;

//Input Y, C should be Block mode
#define CRC_POLYNOMIAL 0x04c11db7
#define CRC_INITIAL    0xffffffff

UINT32 pic_width_in_mb;
UINT32 pic_height_in_mb;
UINT32 result_y[4];
UINT32 result_c[4];

int crc32_core(int crc, int data)
{
	int result, i, tmp_d;

	result = crc;
	for(i=31;i>=0;i--)
	{
		tmp_d = ((data>>i)&0x1) ^ ((result>>31)&0x1);
		result = (tmp_d == 1)? ((result<<1) ^ CRC_POLYNOMIAL):(result<<1);
	}
	return result;
}

void crc32(unsigned char *src, int component)
{
	int data;
	int i;

	for(i=0;i<4;i++)
	{
		data = (src[4*i+0]<<0)+(src[4*i+1]<<8)+(src[4*i+2]<<16)+(src[4*i+3]<<24);
		if(component == 0) //Y
			result_y[i] = crc32_core(result_y[i], data);
		else //C
			result_c[i] = crc32_core(result_c[i], data);
	}
}

int get_addr_offset(int mbx, int mby, int component)
{
  UINT32 pic_width_in_mb_align;
    
  pic_width_in_mb_align = pic_width_in_mb;

  if (pic_width_in_mb % 4)
  {
    pic_width_in_mb_align = pic_width_in_mb + (4 - (pic_width_in_mb % 4));
  }

  if(component == 0) //Y
    return (((mby/2)*pic_width_in_mb_align*32)+(mbx*32)+(mby%2)*16);
  else //C
    return (((mby/2)*pic_width_in_mb_align*16)+(mbx*16)+(mby%2)*8);
}

void vRM_CheckCRCResultFromHWOutput(UINT32 u4InstID, UINT32 blk_y, UINT32 blk_c)
{
  VDEC_INFO_RM_PICINFO_T *prParsingPic;
  UINT32 u4OrgWidth;
  UINT32 u4OrgHeight;
  
  BYTE* src_y;
  BYTE* src_c;
  UINT32 y_num[3];
  UINT32 c_num[3];
  
  UINT32 mby;
  UINT32 mbx;
  BYTE* src_curr;
  UINT32 i;
  BYTE* src_neighbor;
  UINT32 end_num;
  
  UINT32 dbk_y_off;
  UINT32 dbk_c_off;
  
  prParsingPic = (VDEC_INFO_RM_PICINFO_T*) 
  	                 &_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo;

  u4OrgWidth = prParsingPic->u4OrgWidth;
  u4OrgHeight = prParsingPic->u4OrgHeight;
  
  pic_width_in_mb = (u4OrgWidth + 15)/16;
  pic_height_in_mb = (u4OrgHeight + 15)/16;
  src_y = (BYTE*) blk_y;
  src_c = (BYTE*) blk_c;

  if (prParsingPic->fgRV9 == FALSE)
  {
    if (prParsingPic->fgDeblockPassThru == TRUE)
    {
      dbk_y_off = 1;
      dbk_c_off = 1;
    }
    else
    {
      if ((prParsingPic->ePtype == RM_INTRAPIC) || 
      	    (prParsingPic->ePtype == RM_FORCED_INTRAPIC))
      {
        dbk_y_off = 0;
	 dbk_c_off = 0;
      }
      else
      {
        dbk_y_off = 0;
        dbk_c_off = 1;
      }
    }
  }
  else
  {
    if (prParsingPic->fgDeblockPassThru == TRUE)
    {
      dbk_y_off = 1;
      dbk_c_off = 1;
    }
    else
    {
      dbk_y_off = 0;
      dbk_c_off = 0;
    }
  }

  if(dbk_y_off == 1)
  {
    y_num[0]=16; y_num[1]=16; y_num[2]=16;
  }
  else
  {
    y_num[0]=8; y_num[1]=16; y_num[2]=24;
  }

  if(dbk_c_off == 1)
  {
    c_num[0]=8; c_num[1]=8; c_num[2]=8;
  }
  else
  {
    c_num[0]=4; c_num[1]=8;  c_num[2]=12;
  }
  
  result_y[0] = result_y[1] = result_y[2] = result_y[3] = CRC_INITIAL;
  result_c[0] = result_c[1] = result_c[2] = result_c[3] = CRC_INITIAL;

  //Only for RV Frame Picture
  for(mby=0;mby<pic_height_in_mb;mby++)
  {
    for(mbx=0;mbx<pic_width_in_mb;mbx++)
    {
      //Y
      src_curr = src_y + get_addr_offset(mbx, mby, 0)*16;
      if(mby == 0) //curr MB only
      {
        for(i=0;i<y_num[0];i++)
	   crc32(src_curr+i*16, 0);
      }
      else
      {
        src_neighbor = src_y + get_addr_offset(mbx, mby-1, 0)*16;
        end_num = (mby == (pic_height_in_mb - 1))? y_num[2]:y_num[1];
	 //top MB
	 for(i=y_num[0];i<16;i++)
	  crc32(src_neighbor+i*16, 0);
	 //curr MB
	 for(i=0;i<(end_num-(16-y_num[0]));i++)
	   crc32(src_curr+i*16, 0);
	}
	
	//C
	src_curr = src_c + get_addr_offset(mbx, mby, 1)*16;
	if(mby == 0) //curr MB only
	{
	  for(i=0;i<c_num[0];i++)
	    crc32(src_curr+i*16, 1);
	}
	else 
	{
	  src_neighbor = src_c + get_addr_offset(mbx, mby-1, 1)*16;
	  end_num = (mby == (pic_height_in_mb - 1))? c_num[2]:c_num[1];
	  //top MB
	  for(i=c_num[0];i<8;i++)
	    crc32(src_neighbor+i*16, 1);
	  //curr MB
	  for(i=0;i<(end_num-(8-c_num[0]));i++)
	    crc32(src_curr+i*16, 1);
	}
    } //end of for(mbx=0;mbx<pic_width_in_mb;mbx++)
  } //end of for(mby=0;mby<pic_height_in_mb;mby++) 
}


void vRM_CheckYCGoldenbyCRC(UINT32 u4InstID)
{
  VDEC_INFO_RM_PICINFO_T *prParsingPic;

  UINT32 u4OutBufY;
  UINT32 u4OutBufC;
    
//  HalFlushInvalidateDCache();

  prParsingPic = (VDEC_INFO_RM_PICINFO_T*) &_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo;

  #ifdef RM_ADDRSWAP_ENABLE

  //HalFlushInvalidateDCache();

  //memset(_pucDumpYBuf_1[u4InstID], 0x0, GOLD_Y_SZ);
  //memset(_pucDumpCBuf_1[u4InstID], 0x0, GOLD_C_SZ);

  //HalFlushInvalidateDCache();

  vRM_InvAddressSwap(u4InstID, 0);

  //HalFlushInvalidateDCache();

  u4OutBufY = (UINT32) _pucDumpYBuf_1[u4InstID];
  u4OutBufC = (UINT32) _pucDumpCBuf_1[u4InstID];
  #else //RM_ADDRSWAP_ENABLE
  u4OutBufY = prParsingPic->u4OutBufY;
  u4OutBufC = prParsingPic->u4OutBufC;
  #endif //RM_ADDRSWAP_ENABLE 
  
  vRM_CheckCRCResultFromHWOutput(u4InstID, u4OutBufY, u4OutBufC);
}



void vRM_CheckCRCResult(UINT32 u4InstID, UINT32 u4DecFrameCnt, UINT32 u4CRCResBuf)
{
  UINT32* pu4CRCResultCurrAddr;
#ifndef RM_DISABLE_HWCRCCHECK
  UINT32 u4CRCResult = 0;
  UINT32 i=0;
#endif
  UINT32 u4SWResult;


  pu4CRCResultCurrAddr = (UINT32*) (u4CRCResBuf + u4DecFrameCnt*36);

  u4SWResult = *pu4CRCResultCurrAddr;
  if (u4SWResult != u4DecFrameCnt)
  {
    ASSERT(0);
  }
  pu4CRCResultCurrAddr++;

#ifndef RM_DISABLE_HWCRCCHECK
  //Y
  for (i=0; i<4; i++)
  {
    u4CRCResult = u4ReadReg(VDEC_CRC_REG_OFFSET0 + ((2+i)*4));
    u4SWResult = *(pu4CRCResultCurrAddr+i);
    if (u4SWResult != u4CRCResult)
    {
      printk("Y CRC Compare Golden Error !!!!!!!!!!!!!!!!!!!!!\n");
      //ASSERT(0);
      break;
    }
  }

  //C
  for (i=0; i<4; i++)
  {
    u4CRCResult = u4ReadReg(VDEC_CRC_REG_OFFSET0 + ((6+i)*4));
    u4SWResult = *(pu4CRCResultCurrAddr+4+i);
    if (u4SWResult != u4CRCResult)
    {
      printk("UV CRC Compare Golden Error !!!!!!!!!!!!!!!!!!!!!\n");
      //ASSERT(0);
      break;
    }
  }
#endif //RM_DISABLE_HWCRCCHECK  

  #ifdef RM_CRCCHECK_HWOUT_GENERATE
  //Check HW Output
  //Y
  for (i=0; i<4; i++)
  {
    u4CRCResult = result_y[i];
    u4SWResult = *(pu4CRCResultCurrAddr+i);
    if (u4SWResult != u4CRCResult)
    {
      printk("CRC Compare Golden Error\n");
      ASSERT(0);
    }
  }

  //C
  for (i=0; i<4; i++)
  {
    u4CRCResult = result_c[i];
    u4SWResult = *(pu4CRCResultCurrAddr+4+i);
    if (u4SWResult != u4CRCResult)
    {
      printk("CRC Compare Golden Error\n");
      ASSERT(0);
    }
  }
  #endif //RM_CRCCHECK_HWOUT_GENERATE
}


BOOL fgAtSpeedWithCheckGolden = FALSE;
extern void vWrite2PC(UINT32 u4InstID, UCHAR bType, UCHAR *pbAddr);

void vRM_VDecDecEndProc(UINT32 u4InstID)
{
  UINT32 u4Cnt;
  UINT32 u4CntTimeChk;
  UINT32 u4MbX;
  UINT32 u4MbY;  
  //char strMessage[256];
  UINT32 u4MbX_last;
  UINT32 u4MbY_last;
  //UINT32 u4MpegErrType = 0;
  //VDEC_INFO_MPEG_ERR_INFO_T prMpegErrInfo;
  VDEC_INFO_RM_PICINFO_T *prParsingPic;   
  #ifdef DOWN_SCALE_SUPPORT
  UINT32 u4Size = 0;
  #endif //DOWN_SCALE_SUPPORT
  #ifdef DOWN_SCALE_SUPPORT
  #ifdef DSCL_LUMAKEY_ENABLE
  BOOL fgLumaKeyEnable = FALSE;
  UINT32 u4LumaKey = 0;
  #endif //DSCL_LUMAKEY_ENABLE
  #endif //DOWN_SCALE_SUPPORT
  #ifndef RM_ATSPEED_TEST_ENABLE
  char strEmuFileName[1024];
  char strMessage[512];
  BOOL fgOpen;
  #endif //RM_ATSPEED_TEST_ENABLE
  
  u4Cnt=0;
  u4CntTimeChk = 0;
  _fgVDecErr[u4InstID] = FALSE;

  
  //if (_u4StartCompPicNum[u4InstID] == 0 || (g_fgRMIPicCnt == 1 && _u4StartCompPicNum[u4InstID] > 0))
  {   
      while(u4CntTimeChk < DEC_RETRY_NUM)
      {    
        u4Cnt ++;    
        
        if((u4Cnt & 0xff)== 0xff)
        {
      #ifndef IRQ_DISABLE    
      #else
          if(u4VDEC_HAL_RM_VDec_ReadFinishFlag(u4InstID))
          {
            _fgVDecComplete[u4InstID] = TRUE;
            printk("<vdec> got finish flag\n");
            #ifdef CAPTURE_ESA_LOG
            vWrite2PC(u4InstID, 17, (UCHAR*)_pucESALog[u4InstID]);
            #endif
    /*        if(u4InstID == 0)
            {
              BIM_ClearIrq(VECTOR_VDFUL);
            }*/
          }
      #endif      
          if(fgRM_IsRMVDecComplete(u4InstID))
          {
            u4CntTimeChk = 0;
            break;
          }
          else
          {
            u4MbX_last = u4MbX;
            u4MbY_last = u4MbY;
            vVDEC_HAL_RM_GetMbxMby(u4InstID, &u4MbX, &u4MbY);
            if((u4MbX == u4MbX_last) && (u4MbY == u4MbY_last))
            {
              u4CntTimeChk ++;
            }
            else
            {
              u4CntTimeChk =0;
            }
          }
          u4Cnt = 0;
        }
      }


#ifdef RM_RISCCHECKFINISH_ENABLE  
      {
        UINT32 u4TimeChk = 0;
        
        while (u4ReadReg(RM_VLD_REG_OFFSET0 + 0xD0) == 0)
        {
          if (u4TimeChk > DEC_RETRY_NUM)
          {
            //ASSERT(0);
            break;
          }
          u4TimeChk++;
        }
      }
#endif //RM_RISCCHECKFINISH_ENABLE

#ifdef RM_DUMP_MCPERFORMANCE_INFO
      vDumpMCPerformance();
#endif //RM_DUMP_MCPERFORMANCE_INFO
      

    //Check Error Condition
      if (u4CntTimeChk == DEC_RETRY_NUM)
      {
      #if 0 // houlong temp
        vDumpVldCkSum();
        vDumpAvcMvCkSum();
        vDumpMcCkSum();
        vDumpPpCkSum();
        vDumpReg();
              
        ASSERT(0);
      #endif
	  printk("<vdec> rm decode timeout!!!!!!!!!!\n");
      }
    }
#if VDEC_DRAM_BUSY_TEST
     vDrmaBusyOff (u4InstID);
#endif

//Need Keep Decode End Info
  prParsingPic = (VDEC_INFO_RM_PICINFO_T*) &_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo;
  if (prParsingPic->ePtype != RM_TRUEBPIC)// && !_fgDecTimeout)
  {
    prParsingPic->u4RefQpMb0 = u4VDecReadRMPP(0, RW_RMPP_PQ);
  }

  #ifdef RM_CRCCHECK_TIMER
  printk("==> Dec End Start %x \n", _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt);
  #endif //RM_CRCCHECK_TIMER


  #ifdef RM_CRCCHECKFLOW_SUPPORT
  #ifdef RM_CRCCHECK_ENABLE
  //Check CRC Reslut
  if (fgRMCheckCRCResult)
  {
    #ifdef RM_CRCCHECK_HWOUT_GENERATE
    #ifdef RM_CRCCHECK_TIMER
    printk("==> Start CRC Gen \n");
    #endif //RM_CRCCHECK_TIMER
    
    vRM_CheckYCGoldenbyCRC(u4InstID);
    #endif //RM_CRCCHECK_HWOUT_GENERATE

    #ifdef RM_CRCCHECK_TIMER
    printk("==> Start CRC Check \n");
    #endif //RM_CRCCHECK_TIMER
//    HalFlushInvalidateDCache();
    vRM_CheckCRCResult(u4InstID, _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt, 
                                        _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4CRCResultBufAddr);  
   // printk("==>  CRC Check OK");
  }
  #endif //RM_CRCCHECK_ENABLE
  #endif //RM_CRCCHECKFLOW_SUPPORT

  #ifdef RM_ATSPEED_TEST_ENABLE
  vCheckGlobalChecksum(u4InstID);

  if (fgAtSpeedWithCheckGolden == TRUE)
  {
    if (fgRM_CheckYCGolden(u4InstID) == FALSE)
    {
      //ASSERT(0);
    }
  }
  #endif //RM_ATSPEED_TEST_ENABLE

#ifndef RM_ATSPEED_TEST_ENABLE

#ifdef RM_LOOPDECODE_FLOW
  if ((u4LoopDecodeTargetCnt+1) >= u4LoopDecodeFrmCnt)
#endif //RM_LOOPDECODE_FLOW
#ifdef RM_SKIPFRAME_Test
  if (fgGetNextIPPic == TRUE)
#endif //RM_SKIPFRAME_Test
  {
    #ifdef RM_CRCCHECK_ENABLE
    #ifdef RM_CRCCHECK_RANDOMCHECK
    if (_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4CRCCheckFrameNumber == 
    	 _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt)
    {
    //Check Golden
    if (fgRM_CheckYCGolden(u4InstID) == FALSE)
    {
      //ASSERT(0);
    }
  }
    #endif //RM_CRCCHECK_RANDOMCHECK
    #else //RM_CRCCHECK_ENABLE
    //Check Golden
    if((_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt >= _u4StartCompPicNum[u4InstID])&&
      (_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt <= _u4EndCompPicNum[u4InstID]))
    {
    if (fgRM_CheckYCGolden(u4InstID) == FALSE)
    {
      printk("<vdec> RM Dec Cnt = %u is error \n", _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt);
      //ASSERT(0);
    }
        
    }
    #endif //RM_CRCCHECK_ENABLE
  }
#endif //RM_ATSPEED_TEST_ENABLE


  #ifdef RM_SKIPFRAME_Test
  if (((prParsingPic->ePtype == RM_INTRAPIC) || 
  	(prParsingPic->ePtype == RM_FORCED_INTRAPIC) ||
  	(prParsingPic->ePtype == RM_INTERPIC)) && 
  	(fgGetNextIPPic == FALSE))
  {
    if (u4SkipCheckFrameNum < _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt)
    {
      fgGetNextIPPic = TRUE;
    }
  }
  #endif //RM_SKIPFRAME_Test


#ifndef RM_ATSPEED_TEST_ENABLE

#ifdef DOWN_SCALE_SUPPORT
  #ifdef RM_ADDRSWAP_ENABLE
  //Inverse Address Swap
//  HalFlushInvalidateDCache();
  
  memset(_pucDumpYBuf_1[u4InstID], 0x0, GOLD_Y_SZ);
  memset(_pucDumpCBuf_1[u4InstID], 0x0, GOLD_C_SZ);

//  HalFlushInvalidateDCache();

  #ifdef RM_MCOutPut_ENABLE
  if (prParsingPic->fgRV9 == FALSE)
  {
    vRM_InvAddressSwap(u4InstID, 2);
  }
  else
  {
    vRM_InvAddressSwap(u4InstID, 0);
  }
  #else //RM_MCOutPut_ENABLE
  vRM_InvAddressSwap(u4InstID, 0);
  #endif //RM_MCOutPut_ENABLE

//  HalFlushInvalidateDCache();


  //DownScaler Golden Generator
  u4Size = ((((prParsingPic->u4OrgWidth + 15) >> 4) * ((prParsingPic->u4OrgHeight + 31) >> 5)) << 9);
  _pucDecWorkBuf[u4InstID] = (UCHAR*) _pucDumpYBuf_1[u4InstID];
  _pucDecCWorkBuf[u4InstID] = (UCHAR*) _pucDumpCBuf_1[u4InstID];

//  HalFlushInvalidateDCache();
  
  //Clear Golden Buffer 
  memset(_pucDumpYBuf[u4InstID], 0x0, GOLD_Y_SZ);
  memset(_pucDumpCBuf[u4InstID], 0x0, GOLD_C_SZ);

  #ifdef DSCL_LUMAKEY_ENABLE
  fgLumaKeyEnable = TRUE;
  u4LumaKey = u4DSCLLumaKey;
  #else //DSCL_LUMAKEY_ENABLE
  //fgLumaKeyEnable = FALSE;
  //u4LumaKey = 0xFF;
  #endif //DSCL_LUMAKEY_ENABLE

  #ifdef DSCL_LUMAKEY_ENABLE
  vRM_GenerateDownScalerGolden(u4InstID, 
                                                         (UINT32)_pucDecWorkBuf[u4InstID],
                                                         (UINT32)(_pucDecCWorkBuf[u4InstID]),
                                                         u4Size,
                                                         fgLumaKeyEnable,
                                                         u4LumaKey);
  #else //DSCL_LUMAKEY_ENABLE
  vGenerateDownScalerGolden(u4InstID, (UINT32)_pucDecWorkBuf[u4InstID],(UINT32)(_pucDecCWorkBuf[u4InstID]),u4Size);
  #endif //DSCL_LUMAKEY_ENABLE

//  HalFlushInvalidateDCache();
  #else //RM_ADDRSWAP_ENABLE
  u4Size = ((((prParsingPic->u4OrgWidth + 15) >> 4) * ((prParsingPic->u4OrgHeight + 31) >> 5)) << 9);

  #ifdef RM_MCOutPut_ENABLE
  if (prParsingPic->fgRV9 == FALSE)
  {
    //RV8
    _pucDecWorkBuf[u4InstID] = (UCHAR*) prParsingPic->u4MCOutBufY;
    _pucDecCWorkBuf[u4InstID] = (UCHAR*) prParsingPic->u4MCOutBufC;
  }
  else
  {
    _pucDecWorkBuf[u4InstID] = (UCHAR*) prParsingPic->u4OutBufY;
    _pucDecCWorkBuf[u4InstID] = (UCHAR*) prParsingPic->u4OutBufC;
  }
  #else //RM_MCOutPut_ENABLE
  _pucDecWorkBuf[u4InstID] = (UCHAR*) prParsingPic->u4OutBufY;
  _pucDecCWorkBuf[u4InstID] = (UCHAR*) prParsingPic->u4OutBufC;
  #endif //RM_MCOutPut_ENABLE

//  HalFlushInvalidateDCache();
  
  //Clear Golden Buffer 
  memset(_pucDumpYBuf[u4InstID], 0x0, GOLD_Y_SZ);
  memset(_pucDumpCBuf[u4InstID], 0x0, GOLD_C_SZ);

  #ifdef DSCL_LUMAKEY_ENABLE
  fgLumaKeyEnable = TRUE;
  u4LumaKey = u4DSCLLumaKey;
  #else //DSCL_LUMAKEY_ENABLE
  //fgLumaKeyEnable = FALSE;
  //u4LumaKey = 0xFF;
  #endif //DSCL_LUMAKEY_ENABLE

  #ifdef DSCL_LUMAKEY_ENABLE
  vRM_GenerateDownScalerGolden(u4InstID, 
                                                         (UINT32)_pucDecWorkBuf[u4InstID],
                                                         (UINT32)(_pucDecCWorkBuf[u4InstID]),
                                                         u4Size,
                                                         fgLumaKeyEnable,
                                                         u4LumaKey);
  #else //DSCL_LUMAKEY_ENABLE
  vGenerateDownScalerGolden(u4InstID, (UINT32)_pucDecWorkBuf[u4InstID],(UINT32)(_pucDecCWorkBuf[u4InstID]),u4Size);
  #endif //DSCL_LUMAKEY_ENABLE

//  HalFlushInvalidateDCache();
  #endif //RM_ADDRSWAP_ENABLE

  if (fgRM_CheckYCDownScalerGolden(u4InstID) == FALSE)
  {
    ASSERT(0);
  }

//  HalFlushInvalidateDCache();

#endif //DOWN_SCALE_SUPPORT
#endif //RM_ATSPEED_TEST_ENABLE
// ===================================================================


//#ifdef RM_CRCCHECK_ENABLE
#if 0
  if (_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4CRCCheckFrameNumber <= 
  	_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt)
  {
    UINT32 u4RandomNum;

    u4RandomNum = rand();
    u4RandomNum = u4RandomNum & 0xFF;
    u4RandomNum += 100;

    if (u4RandomNum > 300)
    {
      u4RandomNum = u4RandomNum % 300;
    }

    //Update New CRC Check Frame
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4CRCCheckFrameNumber = 
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt +
      u4RandomNum;

    #ifdef RM_CRCCHECK_RANDOMCHECK
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.fgCRCLoadGoldenFlag = FALSE;
    #else //RM_CRCCHECK_RANDOMCHECK
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.fgCRCLoadGoldenFlag = TRUE;
    #endif //RM_CRCCHECK_RANDOMCHECK
  }	
  #endif //RM_CRCCHECK_ENABLE


//Add Decoded Pic Cnt
  #ifdef RM_ADDRSWAP_ENABLE
  #ifdef RM_ADDRSWAP_DSCL_ENABLE
  printk("<vdec> RM Dec Cnt = %d, AdrSwap = %d, DSCL AdrSwap On \n", _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt, 
                                               _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMAddrSwapMode);
  #else //RM_ADDRSWAP_DSCL_ENABLE
  printk("<vdec> RM Dec Cnt = %d, AdrSwap = %d \n", _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt, 
                                               _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMAddrSwapMode);
  #endif //RM_ADDRSWAP_DSCL_ENABLE
  #else //RM_ADDRSWAP_ENABLE
  printk("<vdec> RM Dec Cnt = %d, Disable AdrSwap \n", _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt);
  #endif //RM_ADDRSWAP_ENABLE
  
#ifdef RM_LOOPDECODE_FLOW
  if (u4LoopDecodeTargetCnt >= u4LoopDecodeFrmCnt)
#endif //RM_LOOPDECODE_FLOW
  {
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt++;
  }

#ifdef RM_LOOPDECODE_FLOW
  if (u4LoopDecodeTargetCnt >= u4LoopDecodeFrmCnt)
#endif //RM_LOOPDECODE_FLOW
//Update Src Bitstream and Golden Data
  {
#ifdef RM_CRCCHECK_ENABLE
  if (_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.fgCRCLoadGoldenFlag == FALSE)
  {
#endif //RM_CRCCHECK_ENABLE
#ifndef RM_ATSPEED_TEST_ENABLE    ///////////////////RM_ATSPEED_TEST_ENABLE
  //Read RM Goldem from PC
  _tInFileInfo[u4InstID].fgGetFileInfo = TRUE;
  _tInFileInfo[u4InstID].pucTargetAddr = _pucRMGoldenDataBuf[u4InstID];
  _tInFileInfo[u4InstID].u4TargetSz = RM_GOLDENDATA_SZ;    
  _tInFileInfo[u4InstID].u4FileLength = 0;    
  _tInFileInfo[u4InstID].u4FileOffset = 0;

  //Update File Name
  #ifdef SATA_HDD_FS_SUPPORT
  #ifdef RM_CRCCHECK_ENABLE
  sprintf(strEmuFileName, "%s/%s%07d%s", _bFileStr1[u4InstID][RM_GOLDENPATH_INDEX], 
                                                             strRMGoldenFileName,
                                                            _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4CRCCheckFrameNumber,
                                                            strGoldenFileNameExt_FileList);
  #else //RM_CRCCHECK_ENABLE
  {
      UINT32 u4PicCnt = _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt;
      if (u4PicCnt/5000 > 0)
      {
          sprintf(strEmuFileName, "%s%05d/%s%07d%s", _bFileStr1[u4InstID][RM_GOLDENPATH_INDEX], 
                                                            u4PicCnt/5000 * 5000,
                                                            strRMGoldenFileName,
                                                            _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt ,
                                                            strGoldenFileNameExt_FileList);
      }
      else
      {
          //sprintf(strEmuFileName, "%s/%s%07d%s", _bFileStr1[u4InstID][RM_GOLDENPATH_INDEX], 
          sprintf(strEmuFileName, "%s%s%07d%s", _bFileStr1[u4InstID][RM_GOLDENPATH_INDEX], 
                                                             strRMGoldenFileName,
                                                            _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt ,
                                                            strGoldenFileNameExt_FileList);
      }
  }
  #endif //RM_CRCCHECK_ENABLE
  #else //SATA_HDD_FS_SUPPORT
  #ifdef RM_CRCCHECK_ENABLE
  sprintf(strEmuFileName, "%s\\%s%07d%s", _bFileStr1[u4InstID][RM_GOLDENPATH_INDEX], 
                                                             strRMGoldenFileName,
                                                            _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4CRCCheckFrameNumber,
                                                            strGoldenFileNameExt_FileList);
  #else //RM_CRCCHECK_ENABLE
  sprintf(strEmuFileName, "%s\\%s%07d%s", _bFileStr1[u4InstID][RM_GOLDENPATH_INDEX], 
                                                             strRMGoldenFileName,
                                                            _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt ,
                                                            strGoldenFileNameExt_FileList);
  #endif //RM_CRCCHECK_ENABLE
  #endif //SATA_HDD_FS_SUPPORT

  sprintf(strMessage, "%s: \n", strEmuFileName);
  //sprintf(strMessage, "%s: ", _bFileStr1[u4InstID][5]);
  vVDecOutputDebugString(strMessage);
  if((_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt >= _u4StartCompPicNum[u4InstID])&&
     (_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt <= _u4EndCompPicNum[u4InstID]))
  {
  fgOpen = fgOpenFile(u4InstID, strEmuFileName,"r+b", &_tInFileInfo[u4InstID]);
  //fgOpen = fgOpenFile(u4InstID, _bFileStr1[u4InstID][5],"r+b", &_tInFileInfo[u4InstID]);
  if(fgOpen == FALSE)
  {
     printk("%s Emulation Finish at %d, change to next sequence \n",strEmuFileName,_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt);
    _tVerDec[u4InstID].ucState = 4;
    _u4VerBitCount[u4InstID] = 0xffffffff;

    #ifdef RM_CRCCHECKFLOW_SUPPORT
    #ifdef RM_CRCCHECK_ENABLE_ONEPATH
    fgRMCheckCRCResult = FALSE;
    #endif //RM_CRCCHECK_ENABLE_ONEPATH
    #endif //RM_CRCCHECKFLOW_SUPPORT
    return;
    
    #if 0
    vVDecOutputDebugString("Open bit-stream fail\n");
    strcpy(_tFileListRecInfo[u4InstID].bFileName,_FileList_Rec[u4InstID]);
    sprintf(strMessage, "%s", "Open bit-stream fail\n");
    fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);

    while(1);
    #endif //0
  }
  }
#endif //RM_ATSPEED_TEST_ENABLE                       ////////////////RM_ATSPEED_TEST_ENABLE
#ifdef RM_CRCCHECK_ENABLE
  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.fgCRCLoadGoldenFlag = TRUE;
  }
#endif //RM_CRCCHECK_ENABLE
  

#ifndef RM_ATSPEED_TEST_ENABLE                    ////////////////RM_ATSPEED_TEST_ENABLE
  _tInFileInfo[u4InstID].fgGetFileInfo = TRUE;
  #ifdef RM_RINGVIFO_FLOW
  _tInFileInfo[u4InstID].pucTargetAddr = (UCHAR*) _pucRMRingWorkBuf[u4InstID];
  _tInFileInfo[u4InstID].u4TargetSz = (UINT32) RM_RINGFLOW_TEMPFIFO_SZ;    
  #else //RM_RINGVIFO_FLOW
  _tInFileInfo[u4InstID].pucTargetAddr = _pucVFifo[u4InstID];
  _tInFileInfo[u4InstID].u4TargetSz = V_FIFO_SZ;    
  #endif //RM_RINGVIFO_FLOW
  
  _tInFileInfo[u4InstID].u4FileLength = 0;    
  _tInFileInfo[u4InstID].u4FileOffset = 0;

  //Update File Name
  #ifdef SATA_HDD_FS_SUPPORT
  //sprintf(strEmuFileName, "%s/%s%07d%s", _bFileStr1[u4InstID][RM_SOURCEPATH_INDEX], 
  {
      UINT32 u4PicCnt = _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt;
	  if (u4PicCnt/5000 > 0)
	  {
          sprintf(strEmuFileName, "%s%05d/%s%07d%s", _bFileStr1[u4InstID][RM_SOURCEPATH_INDEX], 
		  	                                         u4PicCnt/5000 * 5000,
                                                     strRMSrcFileName,
                                                     u4PicCnt,
                                                     strSrcFileNameExt_FileList);	      
	  }
	  else
	  {	  
	      sprintf(strEmuFileName, "%s%s%07d%s", _bFileStr1[u4InstID][RM_SOURCEPATH_INDEX], 
												strRMSrcFileName,
												u4PicCnt,
												strSrcFileNameExt_FileList);
      }
  }
  #else //SATA_HDD_FS_SUPPORT
  sprintf(strEmuFileName, "%s\\%s%07d%s", _bFileStr1[u4InstID][RM_SOURCEPATH_INDEX], 
                                                            strRMSrcFileName,
                                                            _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt ,
                                                            strSrcFileNameExt_FileList);
  #endif //SATA_HDD_FS_SUPPORT

  sprintf(strMessage, "%s: \n", strEmuFileName);
  //sprintf(strMessage, "%s: ", _bFileStr1[u4InstID][1]);
  vVDecOutputDebugString(strMessage);
  fgOpen = fgOpenFile(u4InstID, strEmuFileName,"r+b", &_tInFileInfo[u4InstID]);
  //fgOpen = fgOpenFile(u4InstID, _bFileStr1[u4InstID][1],"r+b", &_tInFileInfo[u4InstID]);
  if(fgOpen == FALSE)
  {
   //printk("Emulation Finish, change to next sequence \n");
    printk("%s Emulation Finish at %d, change to next sequence \n",strEmuFileName,_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt);
    _tVerDec[u4InstID].ucState = 4;
    _u4VerBitCount[u4InstID] = 0xffffffff;

    #ifdef RM_CRCCHECKFLOW_SUPPORT
    #ifdef RM_CRCCHECK_ENABLE_ONEPATH
    fgRMCheckCRCResult = FALSE;
    #endif //RM_CRCCHECK_ENABLE_ONEPATH
    #endif //RM_CRCCHECKFLOW_SUPPORT
    return;
  }

  #ifdef RM_CRCCHECK_TIMER
  printk("==> Start Src Loading \n");
  #endif //RM_CRCCHECK_TIMER
  #ifdef RM_RINGVIFO_FLOW
  {
    UINT32 u4VFifoWPtr = _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMVFIFOWPtr;
    UINT32 u4VFIFOSa = _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMVFIFOSa;
    UINT32 u4VFIFOSz = _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMVFIFOSz;
    UINT32 u4TempVFIFOSa = (UINT32) _pucRMRingWorkBuf[u4InstID];
    UINT32 u4CopySize = _tInFileInfo[u4InstID].u4FileLength;
    UINT32 u4RemSz = 0;

    //Update Read Pointer
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFIFORPtr = _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMVFIFOWPtr;

    if (_tInFileInfo[u4InstID].u4FileLength > u4VFIFOSz)
    {
      printk("VFIFO Size Error (< One Picture) \n");
      ASSERT(0);
    }

    if ((u4VFifoWPtr+_tInFileInfo[u4InstID].u4FileLength) < (UINT32) (u4VFIFOSa+u4VFIFOSz))
    {
      memcpy((void*)(u4VFifoWPtr), (void*)u4TempVFIFOSa, u4CopySize);
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMVFIFOWPtr += u4CopySize;
    }
    else
    {
      u4CopySize = u4VFIFOSa+u4VFIFOSz - u4VFifoWPtr;
      u4RemSz = _tInFileInfo[u4InstID].u4FileLength - u4CopySize;
      memcpy((void*)(u4VFifoWPtr), (void*)u4TempVFIFOSa, u4CopySize);

      memcpy((void*)(u4VFIFOSa), (void*)(u4TempVFIFOSa+u4CopySize), u4RemSz);
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMVFIFOWPtr = u4VFIFOSa + u4RemSz;
    }
  }
  #endif //RM_RINGVIFO_FLOW
#else //RM_ATSPEED_TEST_ENABLE                      ///////////////////////RM_ATSPEED_TEST_ENABLE

  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFIFORPtr = 
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFIFORPtr + 
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4FramePayloadSize;

#endif //RM_ATSPEED_TEST_ENABLE                      ///////////////////////RM_ATSPEED_TEST_ENABLE
  
  }

  #ifdef RM_ATSPEED_TEST_ENABLE
  if (_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo.u4DecodedPicCnt >= 
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4ClipTotalFrameCnt)
  {
    printk("Emulation Finish, change to next sequence \n");
    _tVerDec[u4InstID].ucState = 4;
    _u4VerBitCount[u4InstID] = 0xffffffff;

    #ifdef RM_CRCCHECKFLOW_SUPPORT
    #ifdef RM_CRCCHECK_ENABLE_ONEPATH
    fgRMCheckCRCResult = FALSE;
    #endif //RM_CRCCHECK_ENABLE_ONEPATH
    #endif //RM_CRCCHECKFLOW_SUPPORT
    return;
  }
  #endif //RM_ATSPEED_TEST_ENABLE
  
  #ifdef RM_CRCCHECK_TIMER
  printk("==> End Dec End \n");
  #endif //RM_CRCCHECK_TIMER
  
//Change VDec Status  
  _tVerDec[u4InstID].ucState = DEC_NORM_VPARSER;
}




