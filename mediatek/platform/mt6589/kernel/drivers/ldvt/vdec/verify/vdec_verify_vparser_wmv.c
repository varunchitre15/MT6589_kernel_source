#include "vdec_verify_mpv_prov.h"
#include "../hal/vdec_hal_if_common.h"
#include "../hal/vdec_hal_if_wmv.h"
#include "vdec_verify_keydef.h"
#include "../include/vdec_drv_wmv_info.h"
#include "vdec_verify_file_common.h"
#include "vdec_verify_vparser_wmv.h"

#include <linux/string.h>
#include <linux/delay.h>

#if (!CONFIG_DRV_LINUX)
#include <string.h>
#include <stdio.h>
#include <math.h>
#endif

/********************/
/* constant variables */
/********************/
const INT32 _iStepRemap[32] = { NO_USE,  1,  2,  3,  4,  5,  6,  7,   // (PQUANT)
                                     8,  6,  7,  8,  9, 10, 11, 12,
                                    13, 14, 15, 16, 17, 18, 19, 20,
                                    21, 22, 23, 24, 25, 27, 29, 31  };
const INT32 _iNumShortVLC[7] = {1, 1, 2, 1, 3, 1, 2};
const INT32 _iDenShortVLC[7] = {2, 3, 3, 4, 4, 5, 5};
const INT32 _iNumLongVLC[14] = {3, 4, 1, 5, 1, 2, 3, 4, 5, 6, 1, 3, 5, 7};
const INT32 _iDenLongVLC[14] = {5, 5, 6, 6, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8};
const INT32 _iBInverse[8] = { 256, 128, 85, 64, 51, 43, 37, 32 };

const INT32 s_pXformLUT_verify[4] = {XFORMMODE_8x8, XFORMMODE_8x4, XFORMMODE_4x8, XFORMMODE_4x4};
const UCHAR s_vopFirstFieldType_verify[8] = {IVOP, IVOP, PVOP, PVOP, BVOP, BVOP, BIVOP, BIVOP};
const UCHAR s_vopSecondFieldType_verify[8] = {IVOP, PVOP, IVOP, PVOP, BVOP, BIVOP, BVOP, BIVOP};

// P field scaling
const VDEC_INFO_WMV_CMVSCALE_T s_sMVScaleValuesFirstField_verify[4] =
{{32, 8, 37, 10, 512, 219, 128},
 {48, 12, 20, 5, 341, 236, 192},
 {53, 13, 14, 4, 307, 242, 213},
 {56, 14, 11, 3, 293, 245, 224}};
const VDEC_INFO_WMV_CMVSCALE_T s_sMVScaleValuesSecondField_verify[4] =
{{32, 8, 37, 10, 512, 219, 128},
 {16, 4, 52, 13, 1024, 204, 64},
 {11, 3, 56, 14, 1536, 200, 43},
 { 8, 2, 58, 15, 2048, 198, 32}};

// B field scaling
const VDEC_INFO_WMV_CMVSCALE_T s_sMVScaleValuesFirstFieldB_verify[4] =
{{43, 11, 26, 7, 384, 230, 171},
 {51, 13, 17, 4, 320, 239, 205},
 {55, 14, 12, 3, 299, 244, 219},
 {57, 14, 10, 3, 288, 246, 228}};
const VDEC_INFO_WMV_CMVSCALE_T s_sMVScaleValuesSecondFieldB_verify[4] =
{{32, 8, 37, 10, 512, 219, 128},
 {48, 12, 20, 5, 341, 236, 192},
 {53, 13, 14, 4, 307, 242, 213},
 {56, 14, 11, 3, 293, 245, 224}};

 
UINT32 u4DecodeVOLHead_WMV3(UINT32 u4InstID);
void SetupMultiResParams(UINT32 u4InstID);
void vRCVFileHeader(UINT32 u4InstID);
INT32 iRCVRead4Bytes(UINT32 u4InstID);
UINT32 u4DecodeVOLHead_WMV12(UINT32 u4InstID);
BOOL fgVParserProcWMV(UINT32 u4InstID);
void vWMVSearchSliceStartCode(UINT32 u4InstID);
void vVPrsIPProc(UINT32 u4InstID);
void vVPrsBProc(UINT32 u4InstID);
UINT32 WMV78DecodePicture(UINT32 u4InstID);
UINT32 WMVideoDecDecodeClipInfo(UINT32 u4InstID);
void vSetDefaultDQuantSetting(UINT32 u4InstID);
void UpdateDCStepSize(UINT32 u4InstID, UCHAR i4StepSize);
void decodeVOPHead_WMV2(UINT32 u4InstID);
UINT32 WMVideoDecDecodeFrameHead(UINT32 u4InstID);
void AdjustReconRange(UINT32 u4InstID);
void vDecodeVOPDQuant(UINT32 u4InstID);
INT32 iGetPMvMode(UINT32 u4InstID, INT32 iPQuant, BOOL fgRepeat);
UINT32 u4VParserWMVA(UINT32 u4InstID);
UINT32 decodeSequenceHead_Advanced(UINT32 u4InstID);
void vResetConditionalVariablesForSequenceSwitch(UINT32 u4InstID);
UINT32 DecodeEntryPointHeader(UINT32 u4InstID);
UINT32 decodeVOPHeadProgressiveWMVA(UINT32 u4InstID);
UINT32 decodeVOPHeadFieldPicture(UINT32 u4InstID);
UINT32 decodeFieldHeadFieldPicture(UINT32 u4InstID);
INT32 iGetIFMvMode(UINT32 u4InstID, INT32 iPQuant, BOOL fgRepeat);
void SetupFieldPictureMVScaling(UINT32 u4InstID, INT32 i4RefFrameDistance);
void SetupBackwardBFieldPictureMVScaling(UINT32 u4InstID, INT32 i4RefFrameDistance);
void decodePFieldMode(UINT32 u4InstID);
void decodeBFieldMode(UINT32 u4InstID);
UINT32 decodeVOPHeadInterlaceV2(UINT32 u4InstID);
void cal_icomp(INT32 *i4Scale, INT32 *i4Shift, INT32  m_iLuminanceScale, INT32 m_iLuminanceShift);
void UpdateVopheaderParam(UINT32 u4InstID);
void vVDecWmvRecBpType(UINT32 u4InstID, UINT32 u4BpNum, VDEC_INFO_WMV_DEC_BP_PRM_T *prWmvDecBpPrm);

//Robert Adds.
void reset_pic_hdr_bits(UINT32 u4InstID) { _u4PicHdrBits[u4InstID] = 0; _fgCounting[u4InstID] = TRUE; }

UINT32 pic_hdr_bitcount(UINT32 u4InstID) { return _u4PicHdrBits[u4InstID]; }
void set_pic_hdr_bits(UINT32 u4InstID, UINT32 n) { _u4PicHdrBits[u4InstID] = n; }
// ~ginny for WMV
//extern void vVDecOutputDebugString(const CHAR * format, ...);


void before_bp(UINT32 u4InstID) 
{ 
    UINT32 u4Bytes,u4Bits;
    VDEC_INFO_WMV_BS_INIT_PRM_T rWmvBSInitPrm;

    _u4PicHdrBits[u4InstID] += 5; 
    _fgCounting[u4InstID] = FALSE;

    if (_u4BSID[u4InstID] == 1) {
        u4Bytes = u4VDEC_HAL_WMV_ReadRdPtr(1, u4InstID, (UINT32)_pucVFifo[u4InstID], &u4Bits);
        rWmvBSInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
        rWmvBSInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
        rWmvBSInitPrm.u4ReadPointer= (UINT32)_pucVFifo[u4InstID] + u4Bytes;
#ifndef  RING_VFIFO_SUPPORT
        rWmvBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
#else
        //    rWmvBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ*(0.5 + 0.5 *(_u4LoadBitstreamCnt[u4InstID]%2)));
        rWmvBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));
#endif
        if (_i4CodecVersion[u4InstID] == VDEC_VC1) {
            i4VDEC_HAL_WMV_InitBarrelShifter(0, u4InstID, &rWmvBSInitPrm, TRUE);
        }
        else {
            i4VDEC_HAL_WMV_InitBarrelShifter(0, u4InstID, &rWmvBSInitPrm, FALSE);
        }
        u4VDEC_HAL_WMV_ShiftGetBitStream(0, u4InstID, u4Bits);
    }
}

void after_bp(UINT32 u4InstID)
{ 
    UINT32 u4Bytes,u4Bits;
    VDEC_INFO_WMV_BS_INIT_PRM_T rWmvBSInitPrm;

    _fgCounting[u4InstID] = TRUE; 
    if (_u4BSID[u4InstID] == 1) {
        u4Bytes = u4VDEC_HAL_WMV_ReadRdPtr(0, u4InstID, (UINT32)_pucVFifo[u4InstID], &u4Bits);
        rWmvBSInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
        rWmvBSInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
        rWmvBSInitPrm.u4ReadPointer= (UINT32)_pucVFifo[u4InstID] + u4Bytes;
#ifndef  RING_VFIFO_SUPPORT
        rWmvBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
#else
        //    rWmvBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ*(0.5 + 0.5 *(_u4LoadBitstreamCnt[u4InstID]%2)));
        rWmvBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));
#endif
        if (_i4CodecVersion[u4InstID] == VDEC_VC1) {
            i4VDEC_HAL_WMV_InitBarrelShifter(1, u4InstID, &rWmvBSInitPrm, TRUE);
        }
        else {
            i4VDEC_HAL_WMV_InitBarrelShifter(1, u4InstID, &rWmvBSInitPrm, FALSE);
        }
        u4VDEC_HAL_WMV_ShiftGetBitStream(1, u4InstID, u4Bits);
    }
}

void AddPicHdrBitCount(UINT32 u4InstID, UINT32 n) 
{
  if(_fgCounting[u4InstID])
    _u4PicHdrBits[u4InstID] += n;
}

/*******************************/
/* RCV File related functions. */
/*******************************/
void vRCVFileHeader(UINT32 u4InstID)
{
  INT32 i4RCVNumFrames,i4CodecVersion,i4PicVertSize,i4PicHorizSize;
  INT32 i4hdrext;
  VDEC_INFO_WMV_SEQ_PRM_T *prWMVSPS = &_rWMVSPS[u4InstID];

  i4RCVNumFrames = iRCVRead4Bytes(u4InstID);
  _i4RcvVersion[u4InstID] = (i4RCVNumFrames >> 30) & 0x1;
  i4CodecVersion = i4RCVNumFrames >> 24;
  i4RCVNumFrames &= 0xffffff;
  
  i4hdrext = (i4CodecVersion >> 7) & 0x1;

  if (_i4RcvVersion[u4InstID] == 0)
  {
    i4CodecVersion &= 0x7f;
  }
  else
  {
    i4CodecVersion &= 0x3f;
  }
  
  if(i4hdrext != 0) 
  {
    _iSeqHdrDataLen[u4InstID] = iRCVRead4Bytes(u4InstID);
    _iSeqHdrData1[u4InstID] = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 32);
    AddPicHdrBitCount(u4InstID, 32);
    if(_iSeqHdrDataLen[u4InstID] == 5)
    {
      _iSeqHdrData2[u4InstID] = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 8);
      AddPicHdrBitCount(u4InstID, 8);
    }
    else
    {
      _iSeqHdrData2[u4InstID] = 0;
    }
  }
  else
  {
    _iSeqHdrData1[u4InstID] = 0;
    _iSeqHdrData2[u4InstID] = 0;
    _iSeqHdrDataLen[u4InstID] = 0;
  }
  printk("position 6\n");
  i4PicVertSize = iRCVRead4Bytes(u4InstID);
  printk("position 7\n");
  i4PicHorizSize = iRCVRead4Bytes(u4InstID);
  printk("position 8\n");
  prWMVSPS->u4PicHeightSrc = i4PicVertSize;
  prWMVSPS->u4PicWidthSrc = i4PicHorizSize;
  _tVerPic[u4InstID].u4W = prWMVSPS->u4PicWidthSrc;
  _tVerPic[u4InstID].u4H = prWMVSPS->u4PicHeightSrc;
  _tVerMpvDecPrm[u4InstID].u4PicH = _tVerPic[u4InstID].u4H;
  _tVerMpvDecPrm[u4InstID].u4PicW = _tVerPic[u4InstID].u4W;


  prWMVSPS->u4PicWidthDec = (prWMVSPS->u4PicWidthSrc + 15) & ~15;
  prWMVSPS->u4PicHeightDec = (prWMVSPS->u4PicHeightSrc + 15) & ~15;
  prWMVSPS->u4NumMBX = prWMVSPS->u4PicWidthDec >> 4;
  prWMVSPS->u4NumMBY = prWMVSPS->u4PicHeightDec >> 4;
  prWMVSPS->u4PicWidthCmp = prWMVSPS->u4PicWidthSrc;
  prWMVSPS->u4PicHeightCmp = prWMVSPS->u4PicHeightSrc;
  _tVerMpvDecPrm[u4InstID].u4PicBW = prWMVSPS->u4PicWidthDec;

  //Calculate four index
  if(_i4CodecVersion[u4InstID] == VDEC_WMV3)
  {
    SetupMultiResParams(u4InstID);
  }
  
  if (_i4RcvVersion[u4InstID] == 1)
  {
    //INT32 CBR;
    //INT32 rcv_additional_header_size, Levels;
    //INT32 bitrate, framerate;
    //UINT32 hdr_buffer;
    printk("position 1\n");
    /*rcv_additional_header_size = */iRCVRead4Bytes(u4InstID);
    printk("position 2\n");
    /*hdr_buffer = */iRCVRead4Bytes(u4InstID);
    printk("position 3\n");
    //Levels = (hdr_buffer>>30)&0x3;
    //CBR = (hdr_buffer>>28)&0x1;
    //hdr_buffer &= 0x07ffffff;
    
    /*bitrate = */iRCVRead4Bytes(u4InstID);
    printk("position 4\n");
    /*framerate = */iRCVRead4Bytes(u4InstID);
    printk("position 5\n");
  }
}

INT32 iRCVRead4Bytes(UINT32 u4InstID)
{
  INT32 iRet;
  UINT32 bB0, bB1, bB2, bB3;


  bB0 = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 8);
  bB1 = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 8);
  bB2 = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 8);
  bB3 = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 8);
  AddPicHdrBitCount(u4InstID, 32);
  
  iRet = (INT32)((bB3 << 24) + (bB2 << 16) + (bB1 << 8) + bB0);

  return iRet;
}

void SetupMultiResParams(UINT32 u4InstID)
{
  VDEC_INFO_WMV_SEQ_PRM_T *prWMVSPS = &_rWMVSPS[u4InstID];

  // Calculate half res params
  INT32 iHalfWidthSrc = prWMVSPS->u4PicWidthSrc >> 1;
  INT32 iHalfHeightSrc = prWMVSPS->u4PicHeightSrc >> 1;
  INT32 iHalfWidthDec = ((prWMVSPS->u4PicWidthSrc >> 1) + 15) & ~15; //Half & MB aligned
  INT32 iHalfHeightDec = ((prWMVSPS->u4PicHeightSrc >> 1) + 15) & ~15; //Half & MB aligned
  VDEC_INFO_WMV_MULTIRES_PRM_T* pMultiResParams;

  if(_i4CodecVersion[u4InstID] == VDEC_VC1)
  {
    //For PROGRESSIVE
    pMultiResParams = &prWMVSPS->rMultiResParams[PROGRESSIVE];

    pMultiResParams->i4FrmWidthSrc = prWMVSPS->u4PicWidthSrc;
    pMultiResParams->i4FrmHeightSrc = prWMVSPS->u4PicHeightSrc;
    pMultiResParams->i4WidthDec = (prWMVSPS->u4PicWidthSrc + 15) & ~15; //MB-aligned
    pMultiResParams->iHeightDec = (prWMVSPS->u4PicHeightSrc + 15) & ~15; //MB-aligned
    pMultiResParams->u4NumMBX = pMultiResParams->i4WidthDec >> 4;
    pMultiResParams->u4NumMBY = pMultiResParams->iHeightDec >> 4;

    //For INTERLACEFIELD
    pMultiResParams = &prWMVSPS->rMultiResParams[INTERLACEFIELD];

    pMultiResParams->i4FrmWidthSrc = prWMVSPS->u4PicWidthSrc;
    pMultiResParams->i4FrmHeightSrc = prWMVSPS->u4PicHeightSrc;
    pMultiResParams->i4WidthDec = (prWMVSPS->u4PicWidthSrc + 15) & ~15; //MB-aligned
    pMultiResParams->iHeightDec = ((prWMVSPS->u4PicHeightSrc >>1) + 15) & ~15; //MB-aligned
    pMultiResParams->u4NumMBX = pMultiResParams->i4WidthDec >> 4;
    pMultiResParams->u4NumMBY = pMultiResParams->iHeightDec >> 4;
    
    //For INTERLACEFRAME
    pMultiResParams = &prWMVSPS->rMultiResParams[INTERLACEFRAME];

    pMultiResParams->i4FrmWidthSrc = prWMVSPS->u4PicWidthSrc;
    pMultiResParams->i4FrmHeightSrc = prWMVSPS->u4PicHeightSrc;
    pMultiResParams->i4WidthDec = (prWMVSPS->u4PicWidthSrc + 15) & ~15; //MB-aligned
    pMultiResParams->iHeightDec = (prWMVSPS->u4PicHeightSrc + 15) & ~15; //MB-aligned
    pMultiResParams->u4NumMBX = pMultiResParams->i4WidthDec >> 4;
    pMultiResParams->u4NumMBY = pMultiResParams->iHeightDec >> 4; 
  }
  else //For WMV3
  {
    // Save parameters for full res (index = 0)
    pMultiResParams = &prWMVSPS->rMultiResParams[0];

    pMultiResParams->i4FrmWidthSrc = prWMVSPS->u4PicWidthSrc;
    pMultiResParams->i4FrmHeightSrc = prWMVSPS->u4PicHeightSrc;
    pMultiResParams->i4WidthDec = (prWMVSPS->u4PicWidthSrc + 15) & ~15; //MB-aligned
    pMultiResParams->iHeightDec = (prWMVSPS->u4PicHeightSrc + 15) & ~15; //MB-aligned
    pMultiResParams->u4NumMBX = pMultiResParams->i4WidthDec >> 4;
    pMultiResParams->u4NumMBY = pMultiResParams->iHeightDec >> 4;

    // Save parameters for half-horizontal, full-vertical res (index = 1)
    pMultiResParams = &prWMVSPS->rMultiResParams[1];

    pMultiResParams->i4FrmWidthSrc = iHalfWidthSrc;
    pMultiResParams->i4FrmHeightSrc = prWMVSPS->u4PicHeightSrc;
    pMultiResParams->i4WidthDec = iHalfWidthDec;
    pMultiResParams->iHeightDec = (prWMVSPS->u4PicHeightSrc + 15) & ~15; //MB-aligned
    pMultiResParams->u4NumMBX = pMultiResParams->i4WidthDec >> 4;
    pMultiResParams->u4NumMBY = pMultiResParams->iHeightDec >> 4;

    // Save parameters for full-horizontal, half-vertical res (index = 2)
    pMultiResParams = &prWMVSPS->rMultiResParams[2];

    pMultiResParams->i4FrmWidthSrc = prWMVSPS->u4PicWidthSrc;
    pMultiResParams->i4FrmHeightSrc =  iHalfHeightSrc;
    pMultiResParams->i4WidthDec = (prWMVSPS->u4PicWidthSrc + 15) & ~15; //MB-aligned
    pMultiResParams->iHeightDec = iHalfHeightDec;
    pMultiResParams->u4NumMBX = pMultiResParams->i4WidthDec >> 4;
    pMultiResParams->u4NumMBY = pMultiResParams->iHeightDec >> 4;

    // Save parameters for half-horizontal, half-vertical res (index = 3)
    pMultiResParams = &prWMVSPS->rMultiResParams[3];

    pMultiResParams->i4FrmWidthSrc = iHalfWidthSrc;
    pMultiResParams->i4FrmHeightSrc =  iHalfHeightSrc;
    pMultiResParams->i4WidthDec = iHalfWidthDec;
    pMultiResParams->iHeightDec = iHalfHeightDec;
    pMultiResParams->u4NumMBX = pMultiResParams->i4WidthDec >> 4;
    pMultiResParams->u4NumMBY = pMultiResParams->iHeightDec >> 4;
  }
}

//WMV3 : Simple & Main Profile, decode sequence header
UINT32 u4DecodeVOLHead_WMV3(UINT32 u4InstID)
{
  //BOOL fgRndCtrlOn;
  BOOL fg16bitXform;
  //BOOL fgStartCode;
  //BOOL fgRTMContent;
  //BOOL fgBetaContent;
  //INT32 iBetaRTMMismatchIndex;
  VDEC_INFO_WMV_SEQ_PRM_T *prWMVSPS = &_rWMVSPS[u4InstID];
  VDEC_INFO_WMV_ETRY_PRM_T *prWMVEPS = &_rWMVEPS[u4InstID];
  VDEC_INFO_WMV_PIC_PRM_T *prWMVPPS = &_rWMVPPS[u4InstID];
  
  prWMVSPS->i4Profile = (_iSeqHdrData1[u4InstID] & 0xc0000000) >> 30;
  if(prWMVSPS->i4Profile == 0)
    prWMVSPS->i4WMV3Profile = WMV3_SIMPLE_PROFILE;
  else if(prWMVSPS->i4Profile == 1)
    prWMVSPS->i4WMV3Profile = WMV3_MAIN_PROFILE;
  else if(prWMVSPS->i4Profile == 2)
    prWMVSPS->i4WMV3Profile = WMV3_PC_PROFILE;

  prWMVSPS->fgYUV411 = (_iSeqHdrData1[u4InstID] & 0x20000000) >> 29;
  prWMVSPS->fgSpriteMode = (_iSeqHdrData1[u4InstID] & 0x10000000) >> 28;
  prWMVSPS->i4FrameRate = (_iSeqHdrData1[u4InstID] & 0x0e000000) >> 25;
  prWMVSPS->i4FrameRate = 4 * prWMVSPS->i4FrameRate + 2;
  prWMVSPS->i4BitRate = (_iSeqHdrData1[u4InstID] & 0x01f00000) >> 20;
  prWMVSPS->i4BitRate = 64 * prWMVSPS->i4BitRate + 32;

  //fgRndCtrlOn = TRUE;
  prWMVEPS->fgLoopFilter = (_iSeqHdrData1[u4InstID] & 0x00080000) >> 19;
  prWMVSPS->fgXintra8Switch = (_iSeqHdrData1[u4InstID] & 0x00040000) >> 18;
  prWMVSPS->fgMultiresEnabled = (_iSeqHdrData1[u4InstID] & 0x00020000) >> 17;
  fg16bitXform = (_iSeqHdrData1[u4InstID] & 0x00010000) >> 16;
  prWMVEPS->fgUVHpelBilinear = (_iSeqHdrData1[u4InstID] & 0x00008000) >> 15;
  prWMVEPS->fgExtendedMvMode = (_iSeqHdrData1[u4InstID] & 0x00004000) >> 14;
  prWMVEPS->i4DQuantCodingOn = (_iSeqHdrData1[u4InstID] & 0x00003000) >> 12;
  prWMVEPS->fgXformSwitch = (_iSeqHdrData1[u4InstID] & 0x00000800) >> 11;
  prWMVSPS->fgDCTTableMBEnabled = (_iSeqHdrData1[u4InstID] & 0x00000400) >> 10;
  prWMVEPS->fgSequenceOverlap = (_iSeqHdrData1[u4InstID] & 0x00000200) >> 9;
  //fgStartCode = (_iSeqHdrData1[u4InstID] & 0x00000100) >> 8;
  prWMVSPS->fgPreProcRange = (_iSeqHdrData1[u4InstID] & 0x00000080) >> 7;
  prWMVSPS->i4NumBFrames = (_iSeqHdrData1[u4InstID] & 0x00000070) >> 4;
  prWMVEPS->fgExplicitSeqQuantizer = (_iSeqHdrData1[u4InstID] & 0x00000008) >> 3;
  if(prWMVEPS->fgExplicitSeqQuantizer)
  {
    prWMVPPS->fgUse3QPDZQuantizer = (_iSeqHdrData1[u4InstID] & 0x00000004) >> 2;
  }
  else
  {
    prWMVEPS->fgExplicitFrameQuantizer = (_iSeqHdrData1[u4InstID] & 0x00000004) >> 2;
  }
  prWMVEPS->fgExplicitQuantizer = _rWMVEPS[u4InstID].fgExplicitSeqQuantizer || _rWMVEPS[u4InstID].fgExplicitFrameQuantizer;

  prWMVSPS->fgSeqFrameInterpolation = (_iSeqHdrData1[u4InstID] & 0x00000002) >> 1;
  prWMVPPS->i4BFrameReciprocal = _iBInverse[prWMVSPS->i4NumBFrames];

  if((prWMVSPS->fgYUV411) || (prWMVSPS->fgSpriteMode))
  {
    // 411 is depreciated and untested.  Use WMV9 SP and MP only porting kit for 411 reference if you need it.
    return WMV_UnSupportedCompressedFormat;
  }

  if((_iSeqHdrData1[u4InstID] & 0x00000001) == 1)
  {
    // RTM content
    //fgRTMContent = TRUE;
    //fgBetaContent = FALSE;
    //iBetaRTMMismatchIndex = 0;
  }

  if(fg16bitXform)
  {
    prWMVSPS->fgRotatedIdct = TRUE;
  }

  // If _i4HeaderLen > 4, it goes into this condition.
  if(_iSeqHdrDataLen[u4InstID] > 4)
  {
    //BOOL bVCMInfoPresent;

    prWMVSPS->fgPostProcInfoPresent = (_iSeqHdrData2[u4InstID] & 0x80) >> 7;
    // VCM bit (34th bit) is reserved for VCM contents if VOL size > 4 bytes.
    //bVCMInfoPresent = (_iSeqHdrData2[u4InstID] & 0x40) >> 6;
  }

  return WMV_Succeeded;
}

//WMV8 only: decode sequence header
UINT32 u4DecodeVOLHead_WMV12(UINT32 u4InstID)
{
  //BOOL fgRndCtrlOn;
  VDEC_INFO_WMV_SEQ_PRM_T *prWMVSPS = &_rWMVSPS[u4InstID];
  VDEC_INFO_WMV_ETRY_PRM_T *prWMVEPS = &_rWMVEPS[u4InstID];
  

  prWMVSPS->i4FrameRate = (_iSeqHdrData1[u4InstID] & 0xf8000000) >> 27;
  prWMVSPS->i4BitRate = (_iSeqHdrData1[u4InstID] & 0x07ff0000) >> 16;
  //fgRndCtrlOn = TRUE;
  prWMVSPS->fgMixedPel = (_iSeqHdrData1[u4InstID] & 0x00008000) >> 15;
  prWMVEPS->fgLoopFilter = (_iSeqHdrData1[u4InstID] & 0x00004000) >> 14;
  prWMVEPS->fgXformSwitch = (_iSeqHdrData1[u4InstID] & 0x00002000) >> 13;
  prWMVSPS->fgXintra8Switch = (_iSeqHdrData1[u4InstID] & 0x00001000) >> 12;

  prWMVSPS->fgFrmHybridMVOn = (_iSeqHdrData1[u4InstID] & 0x00000800) >> 11;

  // DCTTABLE S/W at MB level for WMV2.
  prWMVSPS->fgDCTTableMBEnabled = (_iSeqHdrData1[u4InstID] & 0x00000400) >> 10;
  prWMVSPS->i4SliceCode = (_iSeqHdrData1[u4InstID] & 0x00000380) >> 7;

  return WMV_Succeeded;
}


void vVPrsIPProc(UINT32 u4InstID)
{
  vSetVerFRefBuf(u4InstID, _u4BRefBufIdx[u4InstID]);
  vSetVerBRefBuf(u4InstID, 1 - _u4FRefBufIdx[u4InstID]);
  vSetVerDecBuf(u4InstID, _u4BRefBufIdx[u4InstID]);
}

void vVPrsBProc(UINT32 u4InstID)
{
  vSetVerDecBuf(u4InstID, 2);
  //vSaveFrmBufPrm(_u4DecBufIdx);
}

UINT32 WMVideoDecDecodeClipInfo(UINT32 u4InstID)
{
  //BOOL fgRndCtrlOn;
  VDEC_INFO_WMV_SEQ_PRM_T *prWMVSPS = &_rWMVSPS[u4InstID];
  

  if(_i4CodecVersion[u4InstID] >= VDEC_WMV2)
  {
    if(prWMVSPS->fgXintra8Switch)
    {
      prWMVSPS->fgXintra8 = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
      AddPicHdrBitCount(u4InstID, 1);
    }
  }
  else // WMV1
  {
    INT32 iFrameRate = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 5);
    AddPicHdrBitCount(u4InstID, 5);
    if(prWMVSPS->i4FrameRate == 0) // if the info is available from system (app), use it.
    {
      prWMVSPS->i4FrameRate = iFrameRate;
    }
    prWMVSPS->i4BitRate = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 11);
    /*fgRndCtrlOn = */u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
    AddPicHdrBitCount(u4InstID, 12);
  }

  return WMV_Succeeded;
}

void vSetDefaultDQuantSetting(UINT32 u4InstID)
{
  VDEC_INFO_WMV_PIC_PRM_T *prWMVPPS = &_rWMVPPS[u4InstID];
  
  prWMVPPS->fgDQuantOn = FALSE;
  prWMVPPS->fgDQuantBiLevel = FALSE;
  prWMVPPS->i4Panning = 0;
  prWMVPPS->ucDQuantBiLevelStepSize = prWMVPPS->i4StepSize;
  return; 
}

void UpdateDCStepSize(UINT32 u4InstID, UCHAR i4StepSize)
{
  VDEC_INFO_WMV_ETRY_PRM_T *prWMVEPS = &_rWMVEPS[u4InstID];
  VDEC_INFO_WMV_PIC_PRM_T *prWMVPPS = &_rWMVPPS[u4InstID];

    prWMVPPS->i4StepSize = i4StepSize;

    if(_i4CodecVersion[u4InstID] >= VDEC_WMV3)
    {
      VDEC_INFO_WMV_DQUANT_PRM_T *pDQ;
      i4StepSize = (2 * i4StepSize - 1) + prWMVPPS->fgHalfStep;
      pDQ = &prWMVPPS->prDQuantParam[i4StepSize];
      prWMVPPS->i4DCStepSize = pDQ->i4DCStepSize;
    }
    else
    {
      prWMVPPS->i4DCStepSize = 8;

      if((_i4CodecVersion[u4InstID] >= VDEC_WMV1))
      {
        if(prWMVPPS->i4StepSize <= 4)
        {
          prWMVPPS->i4DCStepSize = 8;
          if(prWMVEPS->fgNewDCQuant && (prWMVPPS->i4StepSize <=2))
            prWMVPPS->i4DCStepSize = prWMVPPS->i4StepSize * 2;
        }
        else if(_i4CodecVersion[u4InstID] >= VDEC_WMV1)
        {
          prWMVPPS->i4DCStepSize = prWMVPPS->i4StepSize / 2 + 6;
        }
        else if(prWMVPPS->i4StepSize <= 8)
        {
          prWMVPPS->i4DCStepSize = 2 * prWMVPPS->i4StepSize;
        }
        else if(prWMVPPS->i4StepSize <= 24)
        {
          prWMVPPS->i4DCStepSize = prWMVPPS->i4StepSize + 8;
        }
        else
        {
          prWMVPPS->i4DCStepSize = 2 * prWMVPPS->i4StepSize - 16;
        }
      }
    }
}

void decodeVOPHead_WMV2(UINT32 u4InstID)
{
  VDEC_INFO_WMV_DEC_BP_PRM_T rWmvDecBpPrm;
  VDEC_INFO_WMV_SEQ_PRM_T *prWMVSPS = &_rWMVSPS[u4InstID];
  VDEC_INFO_WMV_ETRY_PRM_T *prWMVEPS = &_rWMVEPS[u4InstID];
  VDEC_INFO_WMV_PIC_PRM_T *prWMVPPS = &_rWMVPPS[u4InstID];
  
    if (!prWMVSPS->fgSkipBitCoding)
    {
      prWMVSPS->fgCODFlagOn = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
      AddPicHdrBitCount(u4InstID, 1);
      prWMVSPS->i4SkipBitModeV87 = prWMVSPS->fgCODFlagOn; //0:no_skip_bit, 1:raw_mode
    }
    else
    {
      INT32 iSkipBitCode = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 2);
      AddPicHdrBitCount(u4InstID, 2);
      prWMVSPS->fgCODFlagOn = TRUE;
      if(iSkipBitCode == 0)
      {
        prWMVSPS->i4SkipBitModeV87 = 0;
        prWMVSPS->fgCODFlagOn = FALSE;
      }
      else
      {
        prWMVSPS->i4SkipBitModeV87 = 2; 
        if(iSkipBitCode == 1)
        {
          prWMVSPS->i4SkipBitCodingMode = Normal;
          prWMVSPS->i4Wmv8BpMode = 1;
        }
        else if(iSkipBitCode == 2)
        {
          prWMVSPS->i4SkipBitCodingMode = RowPredict;
          prWMVSPS->i4Wmv8BpMode = 2;
        }
        else
        {
          prWMVSPS->i4SkipBitCodingMode = ColPredict;
          prWMVSPS->i4Wmv8BpMode = 3;
        }
        // change SW decode
        //SKIPMB, wmv78 bitplane decode
        vVDecWmvRecBpType(u4InstID, 0, &rWmvDecBpPrm);
        before_bp(u4InstID);
        i4VDEC_HAL_WMV_HWDecBP(u4InstID, 0, &rWmvDecBpPrm);  
        after_bp(u4InstID);
      }
    }

    // NEW_PCBPCY_TABLE
    if(prWMVSPS->fgNewPcbPcyTable)
    {
      if(prWMVPPS->i4StepSize <= 10)
      {
        if(!u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1))
        { //0 High
          prWMVSPS->i4HufNewPCBPCYDec = HighRate;
          AddPicHdrBitCount(u4InstID, 1);
        }
        else if(!u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1))
        { //10 Low
          prWMVSPS->i4HufNewPCBPCYDec = LowRate;
          AddPicHdrBitCount(u4InstID, 2);
        }
        else
        { //11 Mid
          prWMVSPS->i4HufNewPCBPCYDec = MidRate;
          AddPicHdrBitCount(u4InstID, 2);
        }
      }
      else if(prWMVPPS->i4StepSize <= 20)
      {
        if(!u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1))
        { //0 Mid
          prWMVSPS->i4HufNewPCBPCYDec = MidRate;
          AddPicHdrBitCount(u4InstID, 1);
        }
        else if(!u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1))
        { //10 High
          prWMVSPS->i4HufNewPCBPCYDec = HighRate;
          AddPicHdrBitCount(u4InstID, 2);
        }
        else
        { //11 Low
          prWMVSPS->i4HufNewPCBPCYDec = LowRate;
          AddPicHdrBitCount(u4InstID, 2);
        }
      }
      else
      {
        if(!u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1))
        { //0 Low
          prWMVSPS->i4HufNewPCBPCYDec = LowRate;
          AddPicHdrBitCount(u4InstID, 1);
        }
        else if(!u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1))
        { //10 Mid
          prWMVSPS->i4HufNewPCBPCYDec = MidRate;
          AddPicHdrBitCount(u4InstID, 2);
        }
        else
        { //11 High
          prWMVSPS->i4HufNewPCBPCYDec = HighRate;
          AddPicHdrBitCount(u4InstID, 2);
        }
      }
      prWMVPPS->i4CBPTable = prWMVSPS->i4HufNewPCBPCYDec;
    }
    //Robert TODO: prWMVPPS->i4CBPTable = ???. It should be assigned here !!!

    //_MIXEDPEL_
    if(prWMVSPS->fgMixedPel)
    {
      prWMVPPS->fgMvResolution = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
      AddPicHdrBitCount(u4InstID, 1);
    }
    
    if(prWMVEPS->fgXformSwitch)
    {
      if(u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1) == 1)
      {
         prWMVPPS->fgMBXformSwitching = FALSE;
         AddPicHdrBitCount(u4InstID, 1);
         if(u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1) == 0)
         {
           prWMVPPS->i4FrameXformMode = XFORMMODE_8x8;
           AddPicHdrBitCount(u4InstID, 1);
         }
         else if(u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1) == 0)
         {
           prWMVPPS->i4FrameXformMode = XFORMMODE_8x4;
           AddPicHdrBitCount(u4InstID, 2);
         }
         else
         {
           prWMVPPS->i4FrameXformMode = XFORMMODE_4x8;
           AddPicHdrBitCount(u4InstID, 2);
         }
      }
      else
      {
        prWMVPPS->fgMBXformSwitching = TRUE;
        AddPicHdrBitCount(u4InstID, 1);
      }
    }
}

// WMV7 & WMV8 End
//INT32 ReconRangeState_new = 0;
void AdjustReconRange(UINT32 u4InstID)
{
  VDEC_INFO_WMV_ETRY_PRM_T *prWMVEPS = &_rWMVEPS[u4InstID];
  
  prWMVEPS->i4ReconRangeState = prWMVEPS->i4ReconRangeStateNew;
  if(prWMVEPS->i4ReconRangeStateNew == 0)
  {
    if(prWMVEPS->i4RangeState == 1)
    {
      prWMVEPS->i4ReconRangeStateNew = prWMVEPS->i4RangeState;
    }
  }else if(prWMVEPS->i4ReconRangeStateNew == 1)
  {
    if(prWMVEPS->i4RangeState == 0)
    {
      prWMVEPS->i4ReconRangeStateNew = prWMVEPS->i4RangeState;
    }
  }
}

void vDecodeVOPDQuant(UINT32 u4InstID)
{
  UCHAR bPanningCode;
  UCHAR bPicQtDiff;
  UCHAR bAltPicQt;
  VDEC_INFO_WMV_ETRY_PRM_T *prWMVEPS = &_rWMVEPS[u4InstID];
  VDEC_INFO_WMV_PIC_PRM_T *prWMVPPS = &_rWMVPPS[u4InstID];


  if(prWMVEPS->i4DQuantCodingOn == 2)
  {
    prWMVPPS->fgDQuantOn = TRUE;
    prWMVPPS->i4Panning = 0xf; // four edges
    bPicQtDiff = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 3);
    AddPicHdrBitCount(u4InstID, 3);
    if(bPicQtDiff == 7)
    {
      bAltPicQt = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 5);
      AddPicHdrBitCount(u4InstID, 5);
    }
    else
    {
      bAltPicQt = prWMVPPS->i4StepSize + bPicQtDiff + (UCHAR)1;
    }
    prWMVPPS->ucDQuantBiLevelStepSize = bAltPicQt;
  }
  else
  {
    prWMVPPS->fgDQuantBiLevel = FALSE;
    prWMVPPS->i4Panning = 0;
    prWMVPPS->fgDQuantOn = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
    AddPicHdrBitCount(u4InstID, 1);
    prWMVPPS->ucDiffQtProfile = 0;
    if(prWMVPPS->fgDQuantOn)
    {
      prWMVPPS->ucDiffQtProfile = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 2);
      AddPicHdrBitCount(u4InstID, 2);
      if(prWMVPPS->ucDiffQtProfile == ALL_4EDGES)
      {
        prWMVPPS->i4Panning = 0xf;
      }
      else if(prWMVPPS->ucDiffQtProfile == SINGLE_EDGES)
      {
        bPanningCode = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 2);
        AddPicHdrBitCount(u4InstID, 2);
        prWMVPPS->i4Panning = 0x1 << bPanningCode;
      }
      else if(prWMVPPS->ucDiffQtProfile == DOUBLE_EDGES)
      {
        bPanningCode = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 2);
        AddPicHdrBitCount(u4InstID, 2);
        if(bPanningCode != 3)
        {
          prWMVPPS->i4Panning = 0x3 << bPanningCode;
        }
        else
        {
          prWMVPPS->i4Panning = 0x9;
        }
      }
      else if(prWMVPPS->ucDiffQtProfile == ALL_MBS)
      {
        prWMVPPS->fgDQuantBiLevel = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
        AddPicHdrBitCount(u4InstID, 1);
      }

      if(prWMVPPS->i4Panning || prWMVPPS->fgDQuantBiLevel) 
      {
        bPicQtDiff = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 3);
        AddPicHdrBitCount(u4InstID, 3);
        if(bPicQtDiff == 7)
        {
          bAltPicQt = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 5);
          AddPicHdrBitCount(u4InstID, 5);
        }
        else
        {
          bAltPicQt = prWMVPPS->i4StepSize + bPicQtDiff + 1;
        }
        prWMVPPS->ucDQuantBiLevelStepSize = bAltPicQt;
      }
    } 
  }
}

INT32 iGetPMvMode(UINT32 u4InstID, INT32 iPQuant, BOOL fgRepeat)
{
  UCHAR bMvMode;


  if(iPQuant > 12)
  { // P Picture Low Rate
    if(u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1))
    {
      AddPicHdrBitCount(u4InstID, 1);
      bMvMode = ALL_1MV_HALFPEL_BILINEAR; // 1b
    }
    else if(u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1))
    {
      AddPicHdrBitCount(u4InstID, 2);
      bMvMode = ALL_1MV; // 01b
    }
    else if(u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1))
    {
      AddPicHdrBitCount(u4InstID, 3);
      bMvMode = ALL_1MV_HALFPEL; // 001b
    }
    else
    {
      if(fgRepeat || (!u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1)))
      {
        bMvMode = MIXED_MV; // 0000b
        if(fgRepeat)
        {
          AddPicHdrBitCount(u4InstID, 3);
        }
        else
        {
          AddPicHdrBitCount(u4InstID, 4);
        }
      }
      else
      {
        bMvMode = INTENSITY_COMPENSATION; // 0001b
        AddPicHdrBitCount(u4InstID, 4);
      }
    }
  }
  else
  { // P Picture High rate
    if(u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1))
    {
      bMvMode = ALL_1MV; // 1b
      AddPicHdrBitCount(u4InstID, 1);
    }
    else if(u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1))
    {
      bMvMode = MIXED_MV; // 01b
      AddPicHdrBitCount(u4InstID, 2);
    }
    else if(u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1))
    {
      bMvMode = ALL_1MV_HALFPEL; // 001b
      AddPicHdrBitCount(u4InstID, 3);
    }
    else
    {
      if(fgRepeat || (!u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1)))
      {
        bMvMode = ALL_1MV_HALFPEL_BILINEAR; // 0000b
        if(fgRepeat)
        {
          AddPicHdrBitCount(u4InstID, 3);
        }
        else
        {
          AddPicHdrBitCount(u4InstID, 4);
        }
      }
      else
      {
        bMvMode = INTENSITY_COMPENSATION; // 0001b
        AddPicHdrBitCount(u4InstID, 4);
      }
    }
  }
  return bMvMode;
}

UINT32 WMVideoDecDecodeFrameHead(UINT32 u4InstID)
{
    //BOOL fgInterpolateCurrentFrame;
    //INT32 iFrmCntMod4 = 0;
    BOOL fgTransTypeMB;
    INT32 iTransTypeFrame;
    VDEC_INFO_WMV_SEQ_PRM_T *prWMVSPS = &_rWMVSPS[u4InstID];
    VDEC_INFO_WMV_ETRY_PRM_T *prWMVEPS = &_rWMVEPS[u4InstID];
    VDEC_INFO_WMV_PIC_PRM_T *prWMVPPS = &_rWMVPPS[u4InstID];
    VDEC_INFO_WMV_DEC_BP_PRM_T rWmvDecBpPrm;


    if (prWMVSPS->fgSeqFrameInterpolation) {
        /*fgInterpolateCurrentFrame = */u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
        AddPicHdrBitCount(u4InstID, 1);
    }
    /*iFrmCntMod4 =*/ u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 2);
    AddPicHdrBitCount(u4InstID, 2);

    if (prWMVSPS->fgPreProcRange) {
        prWMVEPS->i4RangeState = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
        AddPicHdrBitCount(u4InstID, 1);
    }

    prWMVPPS->ucPrevPicType = prWMVPPS->ucPicType;
    // Picture coding type
    if (1 == u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1)) {
        prWMVPPS->ucPicType = PVOP;
        AddPicHdrBitCount(u4InstID, 1);
    }
    else {
        AddPicHdrBitCount(u4InstID, 1);
        if (prWMVSPS->i4NumBFrames == 0) {
            prWMVPPS->ucPicType = IVOP;
        }
        else {
            if (u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1) == 1) {
                prWMVPPS->ucPicType = IVOP;
            }
            else {
                prWMVPPS->ucPicType = BVOP;
            }
            AddPicHdrBitCount(u4InstID, 1);
        }
    }

    // Decode B frac
    if ((prWMVPPS->ucPicType == BVOP) || (prWMVPPS->ucPicType == BIVOP)) {
        INT32 iShort = 0, iLong = 0;
        iShort = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 3);
        AddPicHdrBitCount(u4InstID, 3);
        if (iShort == 0x7) {
            iLong = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 4);
            AddPicHdrBitCount(u4InstID, 4);
            if (iLong == 0xe) { // "Invalid" in VLC
                return INVALID_32;
            }

            if (iLong == 0xf) {
                prWMVPPS->ucPicType = BIVOP;
            }
            else {
                prWMVPPS->i4BNumerator = _iNumLongVLC[iLong];
                prWMVPPS->i4BDenominator = _iDenLongVLC[iLong];
                prWMVPPS->i4BFrameReciprocal = _iBInverse[prWMVPPS->i4BDenominator - 1];
                if (prWMVSPS->i4NumBFrames == 0) {
                    prWMVSPS->i4NumBFrames = 1; 
                }
            }
        }
        else {
            prWMVPPS->i4BNumerator = _iNumShortVLC[iShort];
            prWMVPPS->i4BDenominator = _iDenShortVLC[iShort];
            prWMVPPS->i4BFrameReciprocal = _iBInverse[prWMVPPS->i4BDenominator - 1];
            if (prWMVSPS->i4NumBFrames == 0) {
                prWMVSPS->i4NumBFrames = 1; 
            }
        }
    }

    if ((prWMVPPS->ucPicType == IVOP) || (prWMVPPS->ucPicType == BIVOP)) {
        //INT32 iBufferFullPercent = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID, _u4VDecID, 7);
        u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 7);
        AddPicHdrBitCount(u4InstID, 7);
    }

    prWMVPPS->i4PicQtIdx = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 5);
    AddPicHdrBitCount(u4InstID, 5);
    if (prWMVPPS->i4PicQtIdx <= MAXHALFQP) {
        prWMVPPS->fgHalfStep = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
        AddPicHdrBitCount(u4InstID, 1);
    }
    else {
        prWMVPPS->fgHalfStep = FALSE;
    }

    if (prWMVEPS->fgExplicitFrameQuantizer) {
        prWMVPPS->fgUse3QPDZQuantizer = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
        AddPicHdrBitCount(u4InstID, 1);
    }

    if (!prWMVEPS->fgExplicitQuantizer) {
        if (prWMVPPS->i4PicQtIdx <= MAX3QP) {
            prWMVPPS->fgUse3QPDZQuantizer = TRUE;
            prWMVPPS->i4StepSize = prWMVPPS->i4PicQtIdx;
        }
        else {
            prWMVPPS->fgUse3QPDZQuantizer = FALSE;
            prWMVPPS->i4StepSize = _iStepRemap[prWMVPPS->i4PicQtIdx];
        }
    }
    else
        prWMVPPS->i4StepSize = prWMVPPS->i4PicQtIdx;

    prWMVPPS->prDQuantParam = prWMVPPS->fgUse3QPDZQuantizer ? prWMVPPS->rDQuantParam3QPDeadzone : prWMVPPS->rDQuantParam5QPDeadzone;

    prWMVPPS->i4Overlap = 0;
    if (prWMVEPS->fgSequenceOverlap && (prWMVPPS->ucPicType != BVOP)) {
        if (prWMVPPS->i4StepSize >= 9)
            prWMVPPS->i4Overlap = 1;
        else if ((prWMVSPS->i4WMV3Profile == WMV3_ADVANCED_PROFILE) && ((prWMVPPS->ucPicType == IVOP) || (prWMVPPS->ucPicType == BIVOP)))
            prWMVPPS->i4Overlap = 7;
        //prWMVPPS->i4Overlap last 3 bits: [MB switch=1/frame switch=0][sent=1/implied=0][on=1/off=0]
    }

    if (prWMVEPS->fgExtendedMvMode) {
        prWMVEPS->i4MVRangeIndex = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
        AddPicHdrBitCount(u4InstID, 1);
        if (prWMVEPS->i4MVRangeIndex) {
            prWMVEPS->i4MVRangeIndex += u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
            AddPicHdrBitCount(u4InstID, 1);
        }
        if (prWMVEPS->i4MVRangeIndex == 2) {
            prWMVEPS->i4MVRangeIndex += u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
            AddPicHdrBitCount(u4InstID, 1);
        }
    }

    if ((prWMVPPS->ucPicType == IVOP) || (prWMVPPS->ucPicType == PVOP)) {
        if (prWMVSPS->fgMultiresEnabled) {
            prWMVSPS->i4ResIndex = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 2);
            AddPicHdrBitCount(u4InstID, 2);
        }
    }

    //  if(prWMVSPS->i4ResIndex != 0)
    {
        prWMVSPS->u4PicWidthDec = prWMVSPS->rMultiResParams[prWMVSPS->i4ResIndex].i4WidthDec;
        prWMVSPS->u4PicHeightDec = prWMVSPS->rMultiResParams[prWMVSPS->i4ResIndex].iHeightDec;
        prWMVSPS->u4NumMBX = prWMVSPS->rMultiResParams[prWMVSPS->i4ResIndex].u4NumMBX;
        prWMVSPS->u4NumMBY = prWMVSPS->rMultiResParams[prWMVSPS->i4ResIndex].u4NumMBY;
        prWMVSPS->u4PicWidthCmp = prWMVSPS->rMultiResParams[prWMVSPS->i4ResIndex].i4FrmWidthSrc;
        prWMVSPS->u4PicHeightCmp = prWMVSPS->rMultiResParams[prWMVSPS->i4ResIndex].i4FrmHeightSrc;

        _tVerMpvDecPrm[u4InstID].u4PicBW = prWMVSPS->u4PicWidthDec;
    }

    if ((prWMVPPS->ucPicType == IVOP) || (prWMVPPS->ucPicType == BIVOP)) {
        WMVideoDecDecodeClipInfo(u4InstID);

        prWMVPPS->u4DCTACInterTableIndx = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
        AddPicHdrBitCount(u4InstID, 1);
        if (prWMVPPS->u4DCTACInterTableIndx) {
            prWMVPPS->u4DCTACInterTableIndx += u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
            AddPicHdrBitCount(u4InstID, 1);
        }
        prWMVPPS->u4DCTACIntraTableIndx = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
        AddPicHdrBitCount(u4InstID, 1);
        if (prWMVPPS->u4DCTACIntraTableIndx) {
            prWMVPPS->u4DCTACIntraTableIndx += u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
            AddPicHdrBitCount(u4InstID, 1);
        }
        prWMVPPS->fgIntraDCTDCTable = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
        AddPicHdrBitCount(u4InstID, 1);

        vSetDefaultDQuantSetting(u4InstID);

        prWMVPPS->i4RndCtrl = 1;
    } //End of IVOP, BIVOP
    else { // PVOP, BVOP
        //Start==== decodeVOPHead_WMV3() ===================================
        prWMVPPS->fgLuminanceWarp = FALSE;
        prWMVPPS->fgLuminanceWarpTop = prWMVPPS->fgLuminanceWarpBottom = FALSE;

        if ((prWMVPPS->ucPicType == BVOP) && (!prWMVPPS->fgFieldMode)) {
            if (u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1)) {
                prWMVPPS->i4X9MVMode = ALL_1MV;
            }
            else {
                prWMVPPS->i4X9MVMode = ALL_1MV_HALFPEL_BILINEAR;
            }
            AddPicHdrBitCount(u4InstID, 1);
            // "DirectMB", bp_num = 3
            vVDecWmvRecBpType(u4InstID, 3, &rWmvDecBpPrm);
            before_bp(u4InstID);
            i4VDEC_HAL_WMV_HWDecBP(u4InstID, 3, &rWmvDecBpPrm);
            after_bp(u4InstID);
        }
        else {
            prWMVPPS->i4X9MVMode = iGetPMvMode(u4InstID, prWMVPPS->i4StepSize, FALSE);
            if (prWMVPPS->i4X9MVMode == INTENSITY_COMPENSATION) {
                prWMVPPS->fgLuminanceWarp = TRUE;
                prWMVPPS->i4X9MVMode = iGetPMvMode(u4InstID, prWMVPPS->i4StepSize, TRUE);
                prWMVPPS->i4LumScale = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 6);
                prWMVPPS->i4LumShift = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 6);
                AddPicHdrBitCount(u4InstID, 12);
            }

            if (prWMVPPS->i4X9MVMode == MIXED_MV) {
                //"4mv", bp_num = 1
                vVDecWmvRecBpType(u4InstID, 1, &rWmvDecBpPrm);
                before_bp(u4InstID);
                i4VDEC_HAL_WMV_HWDecBP(u4InstID, 1, &rWmvDecBpPrm);
                after_bp(u4InstID);
            }
        }

        // "SkipMB", bp_num = 0
        vVDecWmvRecBpType(u4InstID, 0, &rWmvDecBpPrm);
        before_bp(u4InstID);
        i4VDEC_HAL_WMV_HWDecBP(u4InstID, 0, &rWmvDecBpPrm);
        after_bp(u4InstID);

        prWMVSPS->fgCODFlagOn = TRUE;

        prWMVPPS->i4MvTable = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 2);
        prWMVPPS->i4CBPTable = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 2); // Only in P & B pictures.
        AddPicHdrBitCount(u4InstID, 4);
        if (prWMVEPS->i4DQuantCodingOn) {
            vDecodeVOPDQuant(u4InstID);
        }

        if (prWMVEPS->fgXformSwitch) {
            fgTransTypeMB = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
            AddPicHdrBitCount(u4InstID, 1);
            if (fgTransTypeMB) {
                prWMVPPS->fgMBXformSwitching = FALSE;
                iTransTypeFrame = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 2);
                AddPicHdrBitCount(u4InstID, 2);
                prWMVPPS->i4FrameXformMode = s_pXformLUT_verify[iTransTypeFrame];
            }
            else
                prWMVPPS->fgMBXformSwitching = TRUE;
        }
        else {
            prWMVPPS->fgMBXformSwitching = FALSE;
        }
        //End==== decodeVOPHead_WMV3() ===================================

        prWMVPPS->u4DCTACInterTableIndx = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1); // Coding set index = 0, 1, or 2.
        AddPicHdrBitCount(u4InstID, 1);
        if (prWMVPPS->u4DCTACInterTableIndx == 1) {
            prWMVPPS->u4DCTACInterTableIndx += u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
            AddPicHdrBitCount(u4InstID, 1);
        }
        prWMVPPS->u4DCTACIntraTableIndx = prWMVPPS->u4DCTACInterTableIndx;

        prWMVPPS->fgIntraDCTDCTable = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
        AddPicHdrBitCount(u4InstID, 1);
        //=================================================================
        if (prWMVPPS->ucPicType == PVOP)
            prWMVPPS->i4RndCtrl ^= 1;
    }

    if ((prWMVPPS->ucPicType == IVOP) || (prWMVPPS->ucPicType == PVOP) || (prWMVPPS->ucPicType == BIVOP)) {
        UpdateDCStepSize(u4InstID, prWMVPPS->i4StepSize);
    }

    if (prWMVPPS->ucPicType == PVOP) {
        AdjustReconRange(u4InstID);
    }
    else if (prWMVPPS->ucPicType == IVOP) {
        prWMVEPS->i4ReconRangeStateNew = _rWMVEPS[u4InstID].i4RangeState;
    }

    return WMV_Succeeded;
}


//WMV7 & WMV8: decode picture header
UINT32 WMV78DecodePicture(UINT32 u4InstID)
{
  INT32 iNumBitsFrameType;
  VDEC_INFO_WMV_SEQ_PRM_T *prWMVSPS = &_rWMVSPS[u4InstID];
  VDEC_INFO_WMV_PIC_PRM_T *prWMVPPS = &_rWMVPPS[u4InstID];
  UINT32 u4Datain;
  
  iNumBitsFrameType = (_i4CodecVersion[u4InstID] == VDEC_WMV2) ? 1 :  2;
  prWMVPPS->ucPrevPicType = prWMVPPS->ucPicType;
  u4Datain = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, iNumBitsFrameType);
  prWMVPPS->ucPicType = (u4Datain == 0)? IVOP: PVOP;
  AddPicHdrBitCount(u4InstID, iNumBitsFrameType);

  if((_i4CodecVersion[u4InstID] >= VDEC_WMV2) && (prWMVPPS->ucPicType == IVOP))
  {
    //INT32 iBufferFullPercent = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID, _u4VDecID, 7);
    u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 7);
    AddPicHdrBitCount(u4InstID, 7);
  }

  prWMVPPS->i4PicQtIdx = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 5);
  AddPicHdrBitCount(u4InstID, 5);
  prWMVPPS->prDQuantParam = prWMVPPS->rDQuantParam5QPDeadzone;
  prWMVPPS->i4StepSize = prWMVPPS->i4PicQtIdx;

  if(prWMVPPS->ucPicType == IVOP)
  {
    if(_i4CodecVersion[u4InstID] < VDEC_WMV2) // WMV1 only
    {
      prWMVSPS->i4SliceCode = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, NUMBITS_SLICE_SIZE);
      AddPicHdrBitCount(u4InstID, NUMBITS_SLICE_SIZE);
    }

    WMVideoDecDecodeClipInfo(u4InstID);
    if(prWMVSPS->fgXintra8)
    {
      return WMV_Succeeded;
    }
  
    if(_i4CodecVersion[u4InstID] >= VDEC_WMV2)
    {
      prWMVPPS->fgDCPredIMBInPFrame = FALSE;
    }
    else // WMV1
    {
      prWMVSPS->fgDCTTableMBEnabled = (prWMVSPS->i4BitRate > MIN_BITRATE_MB_TABLE);
      prWMVPPS->fgDCPredIMBInPFrame = ((prWMVSPS->i4BitRate <= MAX_BITRATE_DCPred_IMBInPFrame) && \
                                (prWMVSPS->u4PicWidthSrc * prWMVSPS->u4PicHeightSrc < 320 * 240));
    }
  
    if((!prWMVSPS->fgXintra8) && (_i4CodecVersion[u4InstID] >= VDEC_WMV1))
    {
      if(prWMVSPS->fgDCTTableMBEnabled)
      {
        prWMVPPS->fgDCTTableMB = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
        AddPicHdrBitCount(u4InstID, 1);
      }
      if(!prWMVPPS->fgDCTTableMB)
      {
        // DCT Table swtiching, I and P index are coded separately.
        // Can be jointly coded using the following table. 
        // IP Index : Code
        // 00       : 00, 
        // 11       : 01, 
        // 01       : 100,
        // 10       : 101,
        // 02       : 1100,
        // 12       : 1101,
        // 20       : 1110, 
        // 21       : 11110
        // 22       : 11111
        prWMVPPS->u4DCTACInterTableIndx = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
        AddPicHdrBitCount(u4InstID, 1);
        if(prWMVPPS->u4DCTACInterTableIndx)
        {
          prWMVPPS->u4DCTACInterTableIndx += u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
          AddPicHdrBitCount(u4InstID, 1);
        }
        prWMVPPS->u4DCTACIntraTableIndx = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
        AddPicHdrBitCount(u4InstID, 1);
        if(prWMVPPS->u4DCTACIntraTableIndx)
        {
          prWMVPPS->u4DCTACIntraTableIndx += u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
          AddPicHdrBitCount(u4InstID, 1);
        }
      }
      prWMVPPS->fgIntraDCTDCTable = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
      AddPicHdrBitCount(u4InstID, 1);
    }
    vSetDefaultDQuantSetting(u4InstID);
    prWMVPPS->i4RndCtrl = 1;
  }
  else //PVOP
  {
    decodeVOPHead_WMV2(u4InstID);

    if(_i4CodecVersion[u4InstID] >= VDEC_WMV1)
    {
      // MMIDRATE43 || WMV1 (2) 
      if(prWMVSPS->fgDCTTableMBEnabled)
      {
        prWMVPPS->fgDCTTableMB = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
        AddPicHdrBitCount(u4InstID, 1);
      }

      if(!prWMVPPS->fgDCTTableMB)
      {
        prWMVPPS->u4DCTACInterTableIndx = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
        AddPicHdrBitCount(u4InstID, 1);
        if(prWMVPPS->u4DCTACInterTableIndx)
        {
          prWMVPPS->u4DCTACInterTableIndx += u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
          AddPicHdrBitCount(u4InstID, 1);
        }
      }
      prWMVPPS->u4DCTACIntraTableIndx = prWMVPPS->u4DCTACInterTableIndx;

      prWMVPPS->fgIntraDCTDCTable = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
      AddPicHdrBitCount(u4InstID, 1);

      if(_i4CodecVersion[u4InstID] != VDEC_WMV3)
      {
        prWMVPPS->i4MvTable = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
        AddPicHdrBitCount(u4InstID, 1);
      }
    }
    if(prWMVPPS->ucPicType == PVOP)
      prWMVPPS->i4RndCtrl ^= 1;
  }

  UpdateDCStepSize(u4InstID, prWMVPPS->i4StepSize);

  return WMV_Succeeded;
}

void vWMVSearchSliceStartCode(UINT32 u4InstID)
{
   UINT32 u4PicStartAddr = ((UINT32) _pucVFifo[u4InstID]) + _u4CurrPicStartAddr[u4InstID];
   UCHAR *pVideoData = (UCHAR*) (u4PicStartAddr);
   UINT32  u4Val;
   UINT32  u4Byte0, u4Byte1, u4Byte2, u4Byte3;
   UINT32  u4Addr0, u4Addr1;
   UINT32  u4Addr;
   UINT32  u4FrameFieldHdrCnt = 0;
   UCHAR  ucIdx;
   
   u4SliceHdrCnt = 0;
   for (ucIdx = 0; ucIdx < 68; ucIdx++)
   {
        u4SliceAddr[ucIdx] = 0;
   }
   
   //Search slice start code and save into array until next start code
   while (1)
   {
        u4Byte0 = (UINT32) (*pVideoData);
        u4Byte1 = (UINT32) (* (pVideoData+1) );
        u4Byte2 = (UINT32) (* (pVideoData+2) );
        u4Byte3 = (UINT32) (* (pVideoData+3) );
        
        u4Val = (u4Byte0 << 24) + (u4Byte1 << 16) + (u4Byte2 << 8) + (u4Byte3);
        
        if (u4Val  == (WMV_SC_SLICE))
        {        
            //Analyze mby value
            u4Addr0 = (UINT32) ( *(pVideoData+4) );
            u4Addr1 = (UINT32) ( *(pVideoData+5) ); 
            u4Addr = (u4Addr0 << 1) + ((u4Addr1 & 0x80) >> 7);
            u4SliceAddr[u4Addr] = 1;
            u4SliceHdrCnt++;
        }
        else
        if (
            //u4Val == WMV_SC_SEQ || u4Val == WMV_SC_ENDOFSEQ || u4Val == WMV_SC_ENTRY || 
            u4Val == WMV_SC_FRAME || u4Val == WMV_SC_FIELD)
        {
            if (u4FrameFieldHdrCnt)
               break;
            else
               u4FrameFieldHdrCnt++;
            
        }
        else
        if (((UINT32) pVideoData) >/*=*/ ((_tInFileInfo[u4InstID].u4FileLength + 64/*<< 3*/) + ((UINT32) _pucVFifo[u4InstID])) ) // tmp, enlarge 64 bytes for judgment
        {
            printk("[WMV] SearchSliceStartCode to stream end, len:0x%x, Pic count to [%d]\n", _tInFileInfo[u4InstID].u4FileLength, _u4FileCnt[u4InstID]);  
      
            //vVDEC_HAL_WMV_VDec_DumpReg(u4InstID, FALSE);
            
            break;
        }
        
        pVideoData++;
   }
   
}

BOOL fgVParserProcWMV(UINT32 u4InstID)
{
    //INT32 keyframe, u4TimeStamp;
    VDEC_INFO_WMV_SEQ_PRM_T *prWMVSPS = &_rWMVSPS[u4InstID];
    VDEC_INFO_WMV_PIC_PRM_T *prWMVPPS = &_rWMVPPS[u4InstID];
    BOOL fgSkipFrame = FALSE;

    prWMVPPS->u4BPRawFlag = 0;
    if (_i4CodecVersion[u4InstID] != VDEC_VC1) {
        _i4HeaderLen[u4InstID] = iRCVRead4Bytes(u4InstID); //Picture's Length
        _i4HeaderLen[u4InstID] += 4;
        if (_i4RcvVersion[u4InstID] == 0) {
            //keyframe = (_i4HeaderLen[u4InstID] & 0x80000000)>>31;
            _i4HeaderLen[u4InstID] &= 0x0fffffff;
            if (_i4HeaderLen[u4InstID] == 5) { // it indicates Picture Length == 1, means a skipped frame
                fgSkipFrame = TRUE;
            }
        }
        else if (_i4RcvVersion[u4InstID] == 1) {
            //keyframe = _i4HeaderLen[u4InstID]>>31;
            _i4HeaderLen[u4InstID] &= 0x0fffffff;
            if (_i4HeaderLen[u4InstID] == 5) { // it indicates Picture Length == 1, means a skipped frame
                fgSkipFrame = TRUE;
            }
            /*u4TimeStamp =*/ iRCVRead4Bytes(u4InstID); //u4TimeStamp
            _i4HeaderLen[u4InstID] += 4;
        }
        else {
            vVDecOutputDebugString("RCV Version Error\n");
        }
        _iSetPos[u4InstID] += _i4HeaderLen[u4InstID]; //Calculate the position of the next picture.

        if (_i4CodecVersion[u4InstID] == VDEC_WMV3) { //VC-1 Simple & Main
            if(fgSkipFrame) {
                prWMVPPS->ucPicType = SKIPFRAME;
            }
            else {
                _u4VprErr[u4InstID] = WMVideoDecDecodeFrameHead(u4InstID);
            }
        }
        else if ((_i4CodecVersion[u4InstID] == VDEC_WMV1) || (_i4CodecVersion[u4InstID] == VDEC_WMV2)) { //WMV7, 8
            _u4VprErr[u4InstID] = WMV78DecodePicture(u4InstID);
            if (prWMVSPS->fgXintra8) {
                return TRUE;
            }
        }

        switch(prWMVPPS->ucPicType)
        {
            case IVOP:
            case PVOP:
                vVPrsIPProc(u4InstID);
                break;
            case BIVOP:
            case BVOP:
                vVPrsBProc(u4InstID);
                break;
            case SKIPFRAME:
                vVPrsIPProc(u4InstID);
                break;
            default:
                break;
        } //switch(prWMVPPS->ucPicType)
    }
    else { //Advanced Profile
        if ((_u4VprErr[u4InstID] = u4VParserWMVA(u4InstID)) != 0) {
            if (_u4VprErr[u4InstID] == END_OF_FILE) {
                vVDecOutputDebugString("End of file 1");
            }
            else {
                //Error Case
                vVDecOutputDebugString("_u4VprErr = %.x", _u4VprErr[u4InstID]);
            }
            return(FALSE);
        }

        //Switch Frame Buffer
        if ((prWMVPPS->ucFrameCodingMode != INTERLACEFIELD) || (prWMVPPS->i4CurrentTemporalField == 0)) {
            switch(prWMVPPS->ucPicType)
            {
                case IVOP:
                case PVOP:
                    vVPrsIPProc(u4InstID);
                    break;
                case BVOP:
                case BIVOP:        
                    vVPrsBProc(u4InstID);
                    break;
                case SKIPFRAME:
                    break;
                default:
                    break;
            } //switch(prWMVPPS->ucPicType)
        }
    }

    return(TRUE);
}

// *********************************************************************
// Function : BOOL fgWMVNextStartCode(UINT32 u4InstID)
// Description : Get form bitstream the Next start code, 0x000001xx
// Parameter : None
// Return    : FALSE, if error found next start code
// *********************************************************************
BOOL fgWMVNextStartCode(UINT32 u4InstID)
{
    UINT32 u4Retry = 0;
    UINT32 u4NextStart;

    vVDEC_HAL_WMV_AlignRdPtr(_u4BSID[u4InstID], u4InstID, (UINT32)_pucVFifo[u4InstID], BYTE_ALIGN);

    u4NextStart = u4VDEC_HAL_WMV_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0);
    //check until start code 0x000001XX
    while ((u4NextStart >> 8) != 1) {
        u4NextStart = u4VDEC_HAL_WMV_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 8);
        if (++u4Retry > MAX_RETRY_COUNT1) {
            return FALSE;
        }
    }

    return(TRUE);
}

UINT32 u4VParserWMVA(UINT32 u4InstID)
{
    msleep(5);
    VDEC_INFO_WMV_SEQ_PRM_T *prWMVSPS = &_rWMVSPS[u4InstID];
    VDEC_INFO_WMV_ETRY_PRM_T *prWMVEPS = &_rWMVEPS[u4InstID];
    VDEC_INFO_WMV_PIC_PRM_T *prWMVPPS = &_rWMVPPS[u4InstID];
    UINT32 u4Datain;

    do {
        if (fgWMVNextStartCode(u4InstID) == FALSE) {
            printk("end of file in u4VParserWMVA\n");
            return(END_OF_FILE);
        }
        u4Datain = u4VDEC_HAL_WMV_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0); //get one Start Code

        switch(u4Datain)
        {
            case WMV_SC_FRAME:
            case WMV_SC_FIELD:
                break;
            case WMV_SC_SEQ:
                decodeSequenceHead_Advanced(u4InstID);
                break;
            case WMV_SC_ENTRY:
                vResetConditionalVariablesForSequenceSwitch(u4InstID);
                DecodeEntryPointHeader(u4InstID);
                break;
            case WMV_SC_ENDOFSEQ:
                u4VDEC_HAL_WMV_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 32); // flush the Seq End Code.
                return END_OF_FILE;
            default:
                u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 8); //flush this Start Code
                AddPicHdrBitCount(u4InstID, 8);
                break;
        }
    } while((u4Datain != WMV_SC_FRAME) && (u4Datain != WMV_SC_FIELD) && (u4Datain != WMV_SC_ENDOFSEQ));


    if (_i4CodecVersion[u4InstID] != VDEC_VC1) { // simple or main profile
        // WMVideoDecDecodeFrameHead();
    }
    else { //advanced profile
        u4VDEC_HAL_WMV_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 32); //flush Start Code.
        reset_pic_hdr_bits(u4InstID);
        printk("@@@@@ advanced profile start bit cnt %d\n", _u4PicHdrBits[u4InstID]);

        if (prWMVPPS->i4CurrentTemporalField == 1) {
            set_pic_hdr_bits(u4InstID, prWMVPPS->u4SlicePicHeaderNumField);
            printk("@@@@@ go to INTERLACE_FIELD bit cnt %d\n", _u4PicHdrBits[u4InstID]);
            goto INTERLACE_FIELD;
        }

        if (prWMVSPS->fgInterlacedSource) {
            if (0 == u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1)) {
                AddPicHdrBitCount(u4InstID, 1);
                prWMVPPS->ucFrameCodingMode = PROGRESSIVE;
            }
            else {
                if (0 == u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1)) {
                    prWMVPPS->ucFrameCodingMode = INTERLACEFRAME;
                }
                else {
                    prWMVPPS->ucFrameCodingMode = INTERLACEFIELD;
                }
                AddPicHdrBitCount(u4InstID, 2);
            }
        }
        else {
            prWMVPPS->ucFrameCodingMode = PROGRESSIVE;
        }

        if (prWMVPPS->ucFrameCodingMode == PROGRESSIVE) {
            printk("@@@@@ PROGRESSIVE start bit cnt %d\n", _u4PicHdrBits[u4InstID]);
            prWMVSPS->u4PicWidthDec = prWMVSPS->rMultiResParams[PROGRESSIVE].i4WidthDec;
            prWMVSPS->u4PicHeightDec = prWMVSPS->rMultiResParams[PROGRESSIVE].iHeightDec;
            prWMVSPS->u4NumMBX = prWMVSPS->u4PicWidthDec >> 4;
            prWMVSPS->u4NumMBY = prWMVSPS->u4PicHeightDec >> 4;
            prWMVSPS->u4PicWidthCmp = prWMVSPS->rMultiResParams[PROGRESSIVE].i4FrmWidthSrc;
            prWMVSPS->u4PicHeightCmp = prWMVSPS->rMultiResParams[PROGRESSIVE].i4FrmHeightSrc;

            prWMVPPS->fgInterlaceV2 = FALSE;
            prWMVPPS->fgFieldMode = FALSE;
            prWMVPPS->i4CurrentField = 0; // 0:TOP, 1:BOTTOM field
            _tVerMpvDecPrm[u4InstID].u4PicBW = prWMVSPS->u4PicWidthDec;

            decodeVOPHeadProgressiveWMVA(u4InstID); // advanced progressive
            printk("@@@@@ PROGRESSIVE end bit cnt %d\n", _u4PicHdrBits[u4InstID]);
        }
        else if(prWMVPPS->ucFrameCodingMode == INTERLACEFRAME) {
            printk("@@@@@ INTERLACEFRAME start bit cnt %d\n", _u4PicHdrBits[u4InstID]);
            prWMVSPS->u4PicWidthDec = prWMVSPS->rMultiResParams[INTERLACEFRAME].i4WidthDec;
            prWMVSPS->u4PicHeightDec = prWMVSPS->rMultiResParams[INTERLACEFRAME].iHeightDec;
            prWMVSPS->u4NumMBX = prWMVSPS->u4PicWidthDec >> 4;
            prWMVSPS->u4NumMBY = prWMVSPS->u4PicHeightDec >> 4;
            prWMVSPS->u4PicWidthCmp = prWMVSPS->rMultiResParams[INTERLACEFRAME].i4FrmWidthSrc;
            prWMVSPS->u4PicHeightCmp = prWMVSPS->rMultiResParams[INTERLACEFRAME].i4FrmHeightSrc;

            prWMVPPS->fgInterlaceV2 = TRUE;
            prWMVPPS->fgFieldMode = FALSE;
            prWMVPPS->i4CurrentField = 0; // 0:TOP, 1:BOTTOM field
            _tVerMpvDecPrm[u4InstID].u4PicBW = prWMVSPS->u4PicWidthDec;

            decodeVOPHeadInterlaceV2(u4InstID); // advanced interlace-frame
            printk("@@@@@ INTERLACEFRAME end bit cnt %d\n", _u4PicHdrBits[u4InstID]);
        }
        else { // INTERLACE_FIELD
INTERLACE_FIELD:
            printk("@@@@@ INTERLACE_FIELD start bit cnt %d\n", _u4PicHdrBits[u4InstID]);
            prWMVSPS->u4PicWidthDec = prWMVSPS->rMultiResParams[INTERLACEFIELD].i4WidthDec;
            prWMVSPS->u4PicHeightDec = prWMVSPS->rMultiResParams[INTERLACEFIELD].iHeightDec;
            prWMVSPS->u4NumMBX = prWMVSPS->u4PicWidthDec >> 4;
            prWMVSPS->u4NumMBY = prWMVSPS->u4PicHeightDec >> 4;
            prWMVSPS->u4PicWidthCmp = prWMVSPS->rMultiResParams[INTERLACEFIELD].i4FrmWidthSrc;
            prWMVSPS->u4PicHeightCmp = prWMVSPS->rMultiResParams[INTERLACEFIELD].i4FrmHeightSrc;

            prWMVPPS->fgInterlaceV2 = TRUE;
            prWMVPPS->fgFieldMode = TRUE;
            _tVerMpvDecPrm[u4InstID].u4PicBW = prWMVSPS->u4PicWidthDec;

            if (prWMVPPS->i4CurrentTemporalField == 0) { //1st field
                decodeVOPHeadFieldPicture(u4InstID); // advanced interlace-field
                prWMVPPS->u4SlicePicHeaderNumField = pic_hdr_bitcount(u4InstID);
                printk("@@@@@ prWMVPPS->u4SlicePicHeaderNumField bit cnt %d",prWMVPPS->u4SlicePicHeaderNumField);

                prWMVPPS->i4CurrentField = (prWMVPPS->fgTopFieldFirst) ? 0 : 1; // 0:TOP, 1:BOTTOM field

                prWMVPPS->ucPicType = prWMVPPS->ucFirstFieldType;
                decodeFieldHeadFieldPicture(u4InstID);
                if (prWMVPPS->ucPicType == PVOP) {
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8550)          
                    prWMVPPS->iForwardRefDistance = prWMVEPS->i4RefFrameDistance;
#endif        
                    SetupFieldPictureMVScaling(u4InstID, prWMVEPS->i4RefFrameDistance);
                    decodePFieldMode(u4InstID);
                }
                else if (prWMVPPS->ucPicType == BVOP) {
                    INT32 iForwardRefDistance = (prWMVPPS->i4BNumerator * prWMVEPS->i4RefFrameDistance * prWMVPPS->i4BFrameReciprocal) >> 8;
                    INT32 iBackwardRefDistance = prWMVEPS->i4RefFrameDistance - iForwardRefDistance - 1;
                    // SetupForwardBFieldPictureMVScaling (pWMVDec, iForwardRefDistance);
                    if (iBackwardRefDistance < 0)
                        iBackwardRefDistance = 0;
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8550)          
                    prWMVPPS->iForwardRefDistance = iForwardRefDistance;
                    prWMVPPS->iBackwardRefDistance = iBackwardRefDistance;
#endif
                    SetupFieldPictureMVScaling(u4InstID, iForwardRefDistance);
                    SetupBackwardBFieldPictureMVScaling(u4InstID, iBackwardRefDistance);
                    decodeBFieldMode(u4InstID);
                }
                printk("@@@@@ 1st INTERLACE_FIELD end bit cnt %d\n", _u4PicHdrBits[u4InstID]);
            }
            else { //2nd field
                prWMVPPS->ucPicType = prWMVPPS->ucSecondFieldType;
                decodeFieldHeadFieldPicture(u4InstID);
                if (prWMVPPS->ucPicType == PVOP) {
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8550)          
                    prWMVPPS->iForwardRefDistance = prWMVEPS->i4RefFrameDistance;
#endif
                    SetupFieldPictureMVScaling(u4InstID, prWMVEPS->i4RefFrameDistance);
                    decodePFieldMode(u4InstID);
                }
                else if (prWMVPPS->ucPicType == BVOP) {
                    INT32 iForwardRefDistance = (prWMVPPS->i4BNumerator * prWMVEPS->i4RefFrameDistance * prWMVPPS->i4BFrameReciprocal) >> 8;
                    INT32 iBackwardRefDistance = prWMVEPS->i4RefFrameDistance - iForwardRefDistance - 1;
                    //SetupForwardBFieldPictureMVScaling (pWMVDec, iForwardRefDistance);
                    if (iBackwardRefDistance < 0)
                        iBackwardRefDistance = 0;
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8550)          
                    prWMVPPS->iForwardRefDistance = iForwardRefDistance;
                    prWMVPPS->iBackwardRefDistance = iBackwardRefDistance;
#endif          

                    SetupFieldPictureMVScaling(u4InstID, iForwardRefDistance);
                    SetupBackwardBFieldPictureMVScaling(u4InstID, iBackwardRefDistance);
                    decodeBFieldMode(u4InstID);
                }
                printk("@@@@@ INTERLACE_FIELD 2nd end bit cnt %d\n", _u4PicHdrBits[u4InstID]);
            }
        }
        prWMVPPS->i4SlicePicHeaderNum = pic_hdr_bitcount(u4InstID);
    }
    return 0;
}

UINT32 decodeSequenceHead_Advanced(UINT32 u4InstID)
{
  //UCHAR bReserved;
  BOOL fgDisplay_Ext;
  BOOL fgAspect_Ratio_Flag;
  BOOL fgFrameRate_Flag;
  BOOL fgFrameRateInd;
  BOOL fgColor_Format_Flag;
  //INT32 iLevel = 0;
  //INT32 iChromaFormat = 1;
  //UINT32 u4Disp_Horiz_Size;
  //UINT32 u4Disp_Vert_Size;
  //INT32 iColor_Prim;
  //INT32 iTransfer_Char;
  //INT32 iMatrix_Coef;
  //INT32 iBit_Rate_Exponent;
  //INT32 iBuffer_Size_Exponent;
  //UINT16 wHrd_Rate[32];
  //UINT16 wHrd_Buffer[32];
  UINT32 u4GetVal;
  VDEC_INFO_WMV_SEQ_PRM_T *prWMVSPS = &_rWMVSPS[u4InstID];
  VDEC_INFO_WMV_ETRY_PRM_T *prWMVEPS = &_rWMVEPS[u4InstID];
  VDEC_INFO_WMV_PIC_PRM_T *prWMVPPS = &_rWMVPPS[u4InstID];
  

  INT32 i;

  u4VDEC_HAL_WMV_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 32); // Flush the Seq Head Start Code

  prWMVEPS->fgRangeRedYFlag = FALSE; // Resetting range red flags at the beginning of new advance sequence header
  prWMVEPS->fgRangeRedUVFlag = FALSE;

  // old/unused stuff
  prWMVSPS->fgMixedPel = prWMVSPS->fgFrmHybridMVOn = FALSE;
  prWMVSPS->fgYUV411 = FALSE;
  prWMVSPS->fgSpriteMode = FALSE;
  prWMVSPS->fgXintra8Switch = FALSE;
  prWMVSPS->fgXintra8 = FALSE;
  prWMVSPS->fgMultiresEnabled = FALSE;

  
  prWMVSPS->fgRotatedIdct = TRUE;
  


  u4GetVal = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 28);
  AddPicHdrBitCount(u4InstID, 28);
  prWMVSPS->i4Profile = (u4GetVal & 0xc000000) >> 26;
  prWMVSPS->i4WMV3Profile = WMV3_ADVANCED_PROFILE;
  //iLevel = (u4GetVal & 0x3800000) >> 23;
  //iChromaFormat = (u4GetVal & 0x600000) >> 21;
  prWMVSPS->i4FrameRate = (u4GetVal & 0x1c0000) >> 18;
  prWMVSPS->i4BitRate = (u4GetVal & 0x3e000) >> 13;
  prWMVSPS->fgPostProcInfoPresent = (u4GetVal & 0x1000) >> 12;
  prWMVSPS->u4MaxCodedWidth = u4GetVal & 0xfff;

  u4GetVal = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 19);
  AddPicHdrBitCount(u4InstID, 19);
  prWMVSPS->u4MaxCodedHeight = (u4GetVal & 0x7ff80) >> 7;

  // Robert: translate to what I use
  prWMVSPS->u4MaxPicWidthSrc = 2 *  prWMVSPS->u4MaxCodedWidth + 2;
  prWMVSPS->u4MaxPicHeightSrc = 2 * prWMVSPS->u4MaxCodedHeight + 2;
  prWMVSPS->u4PicWidthSrc = 2 *  prWMVSPS->u4MaxCodedWidth + 2;
  prWMVSPS->u4PicHeightSrc = 2 * prWMVSPS->u4MaxCodedHeight + 2;

   _tVerPic[u4InstID].u4W = prWMVSPS->u4PicWidthSrc;
   _tVerPic[u4InstID].u4H = prWMVSPS->u4PicHeightSrc;
   _tVerMpvDecPrm[u4InstID].u4PicH = _tVerPic[u4InstID].u4H;
  _tVerMpvDecPrm[u4InstID].u4PicW = _tVerPic[u4InstID].u4W;
   _tVerMpvDecPrm[u4InstID].u4PicBW = _tVerPic[u4InstID].u4W;


   SetupMultiResParams(u4InstID);

  prWMVSPS->fgBroadcastFlags = (u4GetVal & 0x40) >> 6;
  prWMVSPS->fgInterlacedSource = (u4GetVal & 0x20) >> 5;
  prWMVSPS->fgTemporalFrmCntr = (u4GetVal & 0x10) >> 4;
  prWMVSPS->fgSeqFrameInterpolation = (u4GetVal & 0x8) >> 3;
  //bReserved = (UCHAR)((u4GetVal & 0x6) >> 1);
  fgDisplay_Ext = u4GetVal & 0x1;

  if(fgDisplay_Ext)
  {
    u4GetVal = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 29);
    AddPicHdrBitCount(u4InstID, 29);
    //u4Disp_Horiz_Size = (u4GetVal & 0x1fff8000) >> 15;
    //u4Disp_Vert_Size = (u4GetVal & 0x7ffe) >> 1;
    fgAspect_Ratio_Flag = u4GetVal & 0x1;
    if(fgAspect_Ratio_Flag)
    {
      prWMVSPS->i4AspectRatio = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 4);
      AddPicHdrBitCount(u4InstID, 4);
      if(prWMVSPS->i4AspectRatio == 15)
      {
        u4GetVal = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 16);
        AddPicHdrBitCount(u4InstID, 16);
        prWMVSPS->i4AspectHorizSize = (u4GetVal & 0xff00) >> 8;
        prWMVSPS->i4AspectVertSize = u4GetVal & 0xff;
      }
    }
    fgFrameRate_Flag = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
    AddPicHdrBitCount(u4InstID, 1);
    if(fgFrameRate_Flag)
    {
      fgFrameRateInd = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
      AddPicHdrBitCount(u4InstID, 1);
      if(fgFrameRateInd == FALSE)
      {
        u4GetVal = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 12);
        AddPicHdrBitCount(u4InstID, 12);
        prWMVSPS->i4FrameRateNr = (u4GetVal & 0xff0) >> 4;
        prWMVSPS->i4FrameRateDr = u4GetVal & 0xf;
      }
      else
      {
        prWMVSPS->i4FrameRateExp = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 16);
        AddPicHdrBitCount(u4InstID, 16);
      }
    }
    fgColor_Format_Flag = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
    AddPicHdrBitCount(u4InstID, 1);
    if(fgColor_Format_Flag)
    {
      u4GetVal = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 24);
      AddPicHdrBitCount(u4InstID, 24);
      //iColor_Prim = (u4GetVal & 0xff0000) >> 16;
      //iTransfer_Char = (u4GetVal & 0xff00) >> 8;
      //iMatrix_Coef = u4GetVal & 0xff;
    }
  }

  prWMVSPS->fgHRDPrmFlag = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
  AddPicHdrBitCount(u4InstID, 1);
  if(prWMVSPS->fgHRDPrmFlag)
  {
    //HDR_PARAM()
    u4GetVal = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 13);
    AddPicHdrBitCount(u4InstID, 13);
    prWMVSPS->i4HRDNumLeakyBuckets = (u4GetVal & 0x1f00) >> 8;
    //iBit_Rate_Exponent = (u4GetVal & 0xf0) >> 4;
    //iBuffer_Size_Exponent = u4GetVal & 0xf;

    for(i = 1; i <= prWMVSPS->i4HRDNumLeakyBuckets; i++)
    {
      u4GetVal = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 32);
      AddPicHdrBitCount(u4InstID, 32);
      //wHrd_Rate[i] = (UINT16)((u4GetVal & 0xffff0000) >> 16);
      //wHrd_Buffer[i] = (UINT16)(u4GetVal & 0xffff);
    }
  }

  prWMVPPS->i4BFrameReciprocal = _iBInverse[prWMVSPS->i4NumBFrames];

  return(TRUE);
}

void vResetConditionalVariablesForSequenceSwitch(UINT32 u4InstID)
{
  VDEC_INFO_WMV_ETRY_PRM_T *prWMVEPS = &_rWMVEPS[u4InstID];
  VDEC_INFO_WMV_PIC_PRM_T *prWMVPPS = &_rWMVPPS[u4InstID];
  
  prWMVEPS->fgExtendedDeltaMvMode = FALSE;
  _rWMVEPS[u4InstID].fgExplicitSeqQuantizer = prWMVEPS->fgExplicitFrameQuantizer = prWMVPPS->fgUse3QPDZQuantizer = FALSE;
  prWMVPPS->fgRepeatFirstField = FALSE;
  prWMVPPS->ucRepeatFrameCount = 0;
  prWMVEPS->i4MVRangeIndex = 0;
  prWMVEPS->i4DeltaMVRangeIndex = 0;
  prWMVEPS->i4ExtendedDMVX = prWMVEPS->i4ExtendedDMVY = 0;
  prWMVPPS->fgLuminanceWarp = FALSE;
  prWMVEPS->i4RefFrameDistance = 0;
  prWMVPPS->i4FrameXformMode = XFORMMODE_8x8;
  prWMVPPS->fgMBXformSwitching = FALSE;
}

UINT32 DecodeEntryPointHeader(UINT32 u4InstID)
{
  INT32 i;
  UINT32 u4GetVal;
  BOOL fgCoded_Size_Flag;
  //UCHAR bHrd_Fullness[32];
  VDEC_INFO_WMV_SEQ_PRM_T *prWMVSPS = &_rWMVSPS[u4InstID];
  VDEC_INFO_WMV_ETRY_PRM_T *prWMVEPS = &_rWMVEPS[u4InstID];
  VDEC_INFO_WMV_PIC_PRM_T *prWMVPPS = &_rWMVPPS[u4InstID];
  
  _new_entry_point[u4InstID] = 1;

  u4VDEC_HAL_WMV_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 32); // Flush the Entry Point Start Code

  u4GetVal = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 13);
  AddPicHdrBitCount(u4InstID, 13);
  prWMVPPS->fgWMVBrokenLink =  (u4GetVal & 0x1000) >> 12;
  prWMVEPS->fgClosedEntryPoint = (u4GetVal & 0x800) >> 11;
  prWMVEPS->fgPanScanPresent = (u4GetVal & 0x400) >> 10;
  prWMVEPS->fgRefDistPresent = (u4GetVal & 0x200) >> 9;
  prWMVEPS->fgLoopFilter = (u4GetVal & 0x100) >> 8;
  prWMVEPS->fgUVHpelBilinear = (u4GetVal & 0x80) >> 7;
  prWMVEPS->fgExtendedMvMode = (u4GetVal & 0x40) >> 6;
  prWMVEPS->i4DQuantCodingOn = (u4GetVal & 0x30) >> 4;
  prWMVEPS->fgXformSwitch = (u4GetVal & 0x8) >> 3;
  prWMVEPS->fgSequenceOverlap = (u4GetVal & 0x4) >> 2;
  prWMVEPS->fgExplicitSeqQuantizer = (u4GetVal & 0x2) >> 1;
  if(prWMVEPS->fgExplicitSeqQuantizer)
  {
    prWMVPPS->fgUse3QPDZQuantizer = u4GetVal & 0x1;
  }
  else
  {
    prWMVEPS->fgExplicitFrameQuantizer = u4GetVal & 0x1;
  }

  prWMVEPS->fgExplicitQuantizer = prWMVEPS->fgExplicitSeqQuantizer || prWMVEPS->fgExplicitFrameQuantizer;
  prWMVPPS->i4BFrameReciprocal = _iBInverse[prWMVSPS->i4NumBFrames];
  prWMVPPS->ucPicType = IVOP;

  if(prWMVSPS->fgHRDPrmFlag)
  {
    //hrd_fullness()
    for(i = 1; i <= prWMVSPS->i4HRDNumLeakyBuckets; i++)
    {
     /*bHrd_Fullness[i] = (UCHAR)*/ u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 8);
     AddPicHdrBitCount(u4InstID, 8);
    }
  }

  fgCoded_Size_Flag = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
  AddPicHdrBitCount(u4InstID, 1);
  if(fgCoded_Size_Flag)
  {
    u4GetVal = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 24);
    AddPicHdrBitCount(u4InstID, 24);
    prWMVEPS->u4CodedWidth = ((u4GetVal & 0xfff000) >> 12);
    prWMVEPS->u4CodedHeight = (u4GetVal & 0xfff);

    // Robert: translate to what I use
    prWMVSPS->u4PicWidthSrc = 2 *  prWMVEPS->u4CodedWidth + 2;
    prWMVSPS->u4PicHeightSrc = 2 * prWMVEPS->u4CodedHeight + 2;

    _tVerPic[u4InstID].u4W = prWMVSPS->u4PicWidthSrc;
    _tVerPic[u4InstID].u4H = prWMVSPS->u4PicHeightSrc;
    _tVerMpvDecPrm[u4InstID].u4PicH = _tVerPic[u4InstID].u4H;
    _tVerMpvDecPrm[u4InstID].u4PicW = _tVerPic[u4InstID].u4W;
    _tVerMpvDecPrm[u4InstID].u4PicBW = _tVerPic[u4InstID].u4W;

    SetupMultiResParams(u4InstID);
  }
  if(prWMVEPS->fgExtendedMvMode)
  {
    prWMVEPS->fgExtendedDeltaMvMode = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
    AddPicHdrBitCount(u4InstID, 1);
  }
  prWMVEPS->fgRangeRedYFlag = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
  AddPicHdrBitCount(u4InstID, 1);
  if(prWMVEPS->fgRangeRedYFlag)
  {
    prWMVEPS->i4RangeRedY = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 3) + 1;
    AddPicHdrBitCount(u4InstID, 3);
  }
  else
  {
    prWMVEPS->i4RangeRedY = 0;
  }

  prWMVEPS->fgRangeRedUVFlag = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
  AddPicHdrBitCount(u4InstID, 1);
  if(prWMVEPS->fgRangeRedUVFlag)
  {
    prWMVEPS->i4RangeMapUV = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 3) + 1;
    AddPicHdrBitCount(u4InstID, 3);
  }
  else
  {
    prWMVEPS->i4RangeMapUV = 0;
  }

  return(TRUE);
}

UINT32 decodeVOPHeadProgressiveWMVA(UINT32 u4InstID)
{
    BOOL fgPanScanPresent;
    //BOOL fgInterpolateCurrentFrame;
    UINT32 u4NumberOfPanScanWindows;
    //BOOL fgUVProgressiveSubsampling;
    //INT32 iPpMethod;

    BOOL fgTransTypeMB;
    INT32 iTransTypeFrame;

    UINT32 i, u4Idx,u4GetVal;
    VDEC_INFO_WMV_SEQ_PRM_T *prWMVSPS = &_rWMVSPS[u4InstID];
    VDEC_INFO_WMV_ETRY_PRM_T *prWMVEPS = &_rWMVEPS[u4InstID];
    VDEC_INFO_WMV_PIC_PRM_T *prWMVPPS = &_rWMVPPS[u4InstID];
    VDEC_INFO_WMV_DEC_BP_PRM_T rWmvDecBpPrm;


    prWMVPPS->ucPrevPicType = prWMVPPS->ucPicType;

    if (0 == u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1)) {
        prWMVPPS->ucPicType = PVOP;
        AddPicHdrBitCount(u4InstID, 1);
    }
    else if (0 == u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1)) {
        prWMVPPS->ucPicType = BVOP;
        AddPicHdrBitCount(u4InstID, 2);
    }
    else if (0 == u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1)) {
        prWMVPPS->ucPicType = IVOP;
        AddPicHdrBitCount(u4InstID, 3);
    }
    else if (0 == u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1)) {
        prWMVPPS->ucPicType = BIVOP;
        AddPicHdrBitCount(u4InstID, 4);
    }
    else {
        prWMVPPS->ucPicType = SKIPFRAME;
        AddPicHdrBitCount(u4InstID, 4);
    }

    if (prWMVPPS->ucPicType != SKIPFRAME) { // SKIPFRAME does not contain temporal reference
        if (prWMVSPS->fgTemporalFrmCntr) {
            prWMVPPS->i4TemporalRef = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 8);
            AddPicHdrBitCount(u4InstID, 8);
        }
    }

    if (prWMVSPS->fgBroadcastFlags) {
        if (prWMVSPS->fgInterlacedSource) {
            u4GetVal = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 2);
            AddPicHdrBitCount(u4InstID, 2);
            prWMVPPS->fgTopFieldFirst = (u4GetVal & 0x2) >> 31;
            prWMVPPS->fgRepeatFirstField = u4GetVal & 0x1;
        }
        else {
            prWMVPPS->ucRepeatFrameCount = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 2);
            AddPicHdrBitCount(u4InstID, 2);
        }
    }

    if (prWMVEPS->fgPanScanPresent) {
        fgPanScanPresent = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
        AddPicHdrBitCount(u4InstID, 1);
        if (fgPanScanPresent) {
            if (prWMVSPS->fgInterlacedSource) {
                if (prWMVSPS->fgBroadcastFlags)
                    u4NumberOfPanScanWindows = 2 + prWMVPPS->fgRepeatFirstField;
                else
                    u4NumberOfPanScanWindows = 2;
            }
            else {
                if (prWMVSPS->fgBroadcastFlags)
                    u4NumberOfPanScanWindows = 1 + prWMVPPS->ucRepeatFrameCount;
                else
                    u4NumberOfPanScanWindows = 1;
            }
            for (i = 0; i < u4NumberOfPanScanWindows; i++) {
                prWMVPPS->rPanScanWindowInfo[i].u4PanScanHorizOffset = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 18);
                prWMVPPS->rPanScanWindowInfo[i].u4PanScanVertOffset = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 18);
                prWMVPPS->rPanScanWindowInfo[i].u4PanScanWidth = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 14);
                prWMVPPS->rPanScanWindowInfo[i].u4PanScanHeight = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 14);
                AddPicHdrBitCount(u4InstID, 64);
            }
        } // fgPanScanPresent
    }

    if (prWMVPPS->ucPicType == SKIPFRAME) {
        return(TRUE);
    }

    prWMVPPS->i4RndCtrl = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
    AddPicHdrBitCount(u4InstID, 1);

    if (prWMVSPS->fgInterlacedSource) {
        /*fgUVProgressiveSubsampling =*/ u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
        AddPicHdrBitCount(u4InstID, 1);
    }
    if (prWMVSPS->fgSeqFrameInterpolation) {
        /*fgInterpolateCurrentFrame =*/ u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
        AddPicHdrBitCount(u4InstID, 1);
    }

    // Decode B FRACION
    if (prWMVPPS->ucPicType == BVOP) {
        u4Idx = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 3);
        AddPicHdrBitCount(u4InstID, 3);
        if (u4Idx == 0x7) {
            u4Idx = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 4);
            AddPicHdrBitCount(u4InstID, 4);
            if (u4Idx == 0xe) // "Invalid" in VLC
                return INVALID_32;
            prWMVPPS->i4BNumerator = _iNumLongVLC[u4Idx];
            prWMVPPS->i4BDenominator = _iDenLongVLC[u4Idx];
        }
        else {
            prWMVPPS->i4BNumerator = _iNumShortVLC[u4Idx];
            prWMVPPS->i4BDenominator = _iDenShortVLC[u4Idx];
        }
        prWMVPPS->i4BFrameReciprocal = _iBInverse[prWMVPPS->i4BDenominator - 1];
        if (prWMVSPS->i4NumBFrames == 0) {
            prWMVSPS->i4NumBFrames = 1;
        }
    }

    prWMVPPS->i4PicQtIdx = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 5);
    AddPicHdrBitCount(u4InstID, 5);

    if (prWMVPPS->i4PicQtIdx <= MAXHALFQP) {
        prWMVPPS->fgHalfStep = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
        AddPicHdrBitCount(u4InstID, 1);
    }
    else {
        prWMVPPS->fgHalfStep = FALSE;
    }

    if (prWMVEPS->fgExplicitFrameQuantizer) {
        prWMVPPS->fgUse3QPDZQuantizer = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
        AddPicHdrBitCount(u4InstID, 1);
    }

    if (!prWMVEPS->fgExplicitQuantizer) {
        if (prWMVPPS->i4PicQtIdx <= MAX3QP) {
            prWMVPPS->fgUse3QPDZQuantizer = TRUE;
            prWMVPPS->i4StepSize = prWMVPPS->i4PicQtIdx;
        }
        else {
            prWMVPPS->fgUse3QPDZQuantizer = FALSE;
            prWMVPPS->i4StepSize = _iStepRemap[prWMVPPS->i4PicQtIdx];
        }
    }
    else
        prWMVPPS->i4StepSize = prWMVPPS->i4PicQtIdx;

    prWMVPPS->prDQuantParam = prWMVPPS->fgUse3QPDZQuantizer ? prWMVPPS->rDQuantParam3QPDeadzone : prWMVPPS->rDQuantParam5QPDeadzone;

    prWMVPPS->i4Overlap = 0;
    if (prWMVEPS->fgSequenceOverlap && (prWMVPPS->ucPicType != BVOP)) {
        if (prWMVPPS->i4StepSize >= 9)
            prWMVPPS->i4Overlap = 1;
        else if (prWMVPPS->ucPicType == IVOP || prWMVPPS->ucPicType == BIVOP)
            prWMVPPS->i4Overlap = 7; // last 3 bits: [MB switch=1/frame switch=0][sent=1/implied=0][on=1/off=0]
    }

    if (prWMVSPS->fgPostProcInfoPresent) {
        /*iPpMethod =*/ u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 2);
        AddPicHdrBitCount(u4InstID, 2);
    }

    if ((prWMVPPS->ucPicType == IVOP) || (prWMVPPS->ucPicType == BIVOP)) {
        //ACPRED, Bitplane
        vVDecWmvRecBpType(u4InstID, 5, &rWmvDecBpPrm);
        before_bp(u4InstID);
        i4VDEC_HAL_WMV_HWDecBP(u4InstID, 5, &rWmvDecBpPrm);
        after_bp(u4InstID);

        if (prWMVPPS->i4Overlap & 2) {
            if (0 == u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1)) {
                prWMVPPS->i4Overlap = 0;
                AddPicHdrBitCount(u4InstID, 1);
            }
            else if (0 == u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1)) {
                prWMVPPS->i4Overlap = 1;
                AddPicHdrBitCount(u4InstID, 2);
            }
            else {
                AddPicHdrBitCount(u4InstID, 2);
                // "OVERFLAGS", bp_num = 6
                vVDecWmvRecBpType(u4InstID, 6, &rWmvDecBpPrm);
                before_bp(u4InstID);
                i4VDEC_HAL_WMV_HWDecBP(u4InstID, 6, &rWmvDecBpPrm);
                after_bp(u4InstID);
            }
        }

        prWMVPPS->u4DCTACInterTableIndx = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
        AddPicHdrBitCount(u4InstID, 1);
        if (prWMVPPS->u4DCTACInterTableIndx) {
            prWMVPPS->u4DCTACInterTableIndx += u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
            AddPicHdrBitCount(u4InstID, 1);
        }
        prWMVPPS->u4DCTACIntraTableIndx = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
        AddPicHdrBitCount(u4InstID, 1);
        if (prWMVPPS->u4DCTACIntraTableIndx) {
            prWMVPPS->u4DCTACIntraTableIndx += u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
            AddPicHdrBitCount(u4InstID, 1);
        }
        prWMVPPS->fgIntraDCTDCTable = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
        AddPicHdrBitCount(u4InstID, 1);

        if (prWMVEPS->i4DQuantCodingOn != 0)
            vDecodeVOPDQuant(u4InstID);
        else
            vSetDefaultDQuantSetting(u4InstID);

        if (prWMVPPS->ucPicType == IVOP) {
            prWMVPPS->fgLuminanceWarp = FALSE;
            prWMVPPS->fgLuminanceWarpTop = prWMVPPS->fgLuminanceWarpBottom = FALSE;
        }

    }// End of IVOP, BIVOP
    else if ((prWMVPPS->ucPicType == PVOP) || (prWMVPPS->ucPicType == BVOP)) {
        if (prWMVEPS->fgExtendedMvMode) {
            // MVRANGE0 = 0, MVRANGE1 = 1,  MVRANGE2 = 2, MVRANGE3 = 3
            prWMVEPS->i4MVRangeIndex = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1); // 0: MVRANGE0;
            AddPicHdrBitCount(u4InstID, 1);
            if (prWMVEPS->i4MVRangeIndex) {
                prWMVEPS->i4MVRangeIndex += u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1); // 1: MVRANGE1
                AddPicHdrBitCount(u4InstID, 1);
                if (prWMVEPS->i4MVRangeIndex == 2) {
                    prWMVEPS->i4MVRangeIndex += u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1); // 2: MVRANGE2,  3: MVRANGE3
                    AddPicHdrBitCount(u4InstID, 1);
                }
            }
        }

        //Start==== decodeVOPHead_WMV3() ===================================

        prWMVPPS->fgLuminanceWarp = FALSE;
        prWMVPPS->fgLuminanceWarpTop = prWMVPPS->fgLuminanceWarpBottom = FALSE;

        if ((prWMVPPS->ucPicType == BVOP) && (!prWMVPPS->fgFieldMode)) {
            if (u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1)) {
                prWMVPPS->i4X9MVMode = ALL_1MV;
            }
            else {
                prWMVPPS->i4X9MVMode = ALL_1MV_HALFPEL_BILINEAR;
            }
            AddPicHdrBitCount(u4InstID, 1);
            // "DirectMB", bp_num = 3
            vVDecWmvRecBpType(u4InstID, 3, &rWmvDecBpPrm);
            before_bp(u4InstID);
            i4VDEC_HAL_WMV_HWDecBP(u4InstID, 3, &rWmvDecBpPrm); 
            after_bp(u4InstID);
        }
        else {
            prWMVPPS->i4X9MVMode = iGetPMvMode(u4InstID, prWMVPPS->i4StepSize, FALSE);
            if (prWMVPPS->i4X9MVMode == INTENSITY_COMPENSATION) {
                prWMVPPS->fgLuminanceWarp = TRUE;
                prWMVPPS->i4X9MVMode = iGetPMvMode(u4InstID, prWMVPPS->i4StepSize, TRUE);
                prWMVPPS->i4LumScale = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 6);
                prWMVPPS->i4LumShift = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 6);
                AddPicHdrBitCount(u4InstID, 12);
            }

            if (prWMVPPS->i4X9MVMode == MIXED_MV) {
                //"4mv", bp_num = 1
                vVDecWmvRecBpType(u4InstID, 1, &rWmvDecBpPrm);
                before_bp(u4InstID);
                i4VDEC_HAL_WMV_HWDecBP(u4InstID, 1, &rWmvDecBpPrm);
                after_bp(u4InstID);
            }
        }

        // "SkipMB", bp_num = 0
        vVDecWmvRecBpType(u4InstID, 0, &rWmvDecBpPrm);
        before_bp(u4InstID);
        i4VDEC_HAL_WMV_HWDecBP(u4InstID, 0, &rWmvDecBpPrm);
        after_bp(u4InstID);

        prWMVSPS->fgCODFlagOn = TRUE;

        prWMVPPS->i4MvTable = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 2);
        prWMVPPS->i4CBPTable = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 2);
        AddPicHdrBitCount(u4InstID, 4);
        if (prWMVEPS->i4DQuantCodingOn) {
            vDecodeVOPDQuant(u4InstID);
        }

        if (prWMVEPS->fgXformSwitch) {
            fgTransTypeMB = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
            AddPicHdrBitCount(u4InstID, 1);
            if (fgTransTypeMB) {
                prWMVPPS->fgMBXformSwitching = FALSE;
                iTransTypeFrame = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 2);
                AddPicHdrBitCount(u4InstID, 2);
                prWMVPPS->i4FrameXformMode = s_pXformLUT_verify[iTransTypeFrame];
            }
            else
                prWMVPPS->fgMBXformSwitching = TRUE;
        }
        else {
            prWMVPPS->fgMBXformSwitching = FALSE;
        }

        //End==== decodeVOPHead_WMV3() ===================================

        prWMVPPS->u4DCTACInterTableIndx = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1); // Coding set index = 0, 1, or 2.
        AddPicHdrBitCount(u4InstID, 1);
        if (prWMVPPS->u4DCTACInterTableIndx == 1) {
            prWMVPPS->u4DCTACInterTableIndx += u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
            AddPicHdrBitCount(u4InstID, 1);
        }
        prWMVPPS->u4DCTACIntraTableIndx = prWMVPPS->u4DCTACInterTableIndx;

        prWMVPPS->fgIntraDCTDCTable = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
        AddPicHdrBitCount(u4InstID, 1);
    //=================================================================
    } // PVOP, BVOP

    if ((prWMVPPS->ucPicType == IVOP) || (prWMVPPS->ucPicType == PVOP) || (prWMVPPS->ucPicType == BIVOP)) {
        UpdateDCStepSize(u4InstID, prWMVPPS->i4StepSize);
    }

    return(TRUE);
}

UINT32 decodeVOPHeadFieldPicture(UINT32 u4InstID)
{
  INT32 iCode;
  BOOL fgPSPresent;
  UINT32 i, u4NumberOfPanScanWindows,u4GetVal;
  //BOOL fgUVProgressiveSubsampling;
  VDEC_INFO_WMV_SEQ_PRM_T *prWMVSPS = &_rWMVSPS[u4InstID];
  VDEC_INFO_WMV_ETRY_PRM_T *prWMVEPS = &_rWMVEPS[u4InstID];
  VDEC_INFO_WMV_PIC_PRM_T *prWMVPPS = &_rWMVPPS[u4InstID];
  
  iCode =  u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 3);
  AddPicHdrBitCount(u4InstID, 3);

  prWMVPPS->ucFirstFieldType = s_vopFirstFieldType_verify[iCode];
  prWMVPPS->ucSecondFieldType = s_vopSecondFieldType_verify[iCode];

  if(prWMVPPS->ucPicType != SKIPFRAME) // SKIPFRAME does not contain temporal reference
  {
    if(prWMVSPS->fgTemporalFrmCntr)
    {
      prWMVPPS->i4TemporalRef = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 8);
      AddPicHdrBitCount(u4InstID, 8);
    }
  }

  if(prWMVSPS->fgBroadcastFlags)
  {
    if(prWMVSPS->fgInterlacedSource)
    {
      u4GetVal = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 2);
      AddPicHdrBitCount(u4InstID, 2);
      prWMVPPS->fgTopFieldFirst = (u4GetVal & 0x2) >> 1;
      prWMVPPS->fgRepeatFirstField = u4GetVal & 0x1;
    }
    else
    {
      prWMVPPS->ucRepeatFrameCount = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 2);
      AddPicHdrBitCount(u4InstID, 2);
    }
  }

  if(prWMVEPS->fgPanScanPresent)
  {
    fgPSPresent = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
    AddPicHdrBitCount(u4InstID, 1);
    if(fgPSPresent)
    {
      if(prWMVSPS->fgInterlacedSource) 
      {
        if(prWMVSPS->fgBroadcastFlags)
          u4NumberOfPanScanWindows = 2 + prWMVPPS->fgRepeatFirstField;
        else
          u4NumberOfPanScanWindows = 2;
      }
      else
      {
        if(prWMVSPS->fgBroadcastFlags)
          u4NumberOfPanScanWindows = 1 + prWMVPPS->ucRepeatFrameCount;
        else
          u4NumberOfPanScanWindows = 1;
      }
      for(i = 0; i < u4NumberOfPanScanWindows; i++)
      {
        prWMVPPS->rPanScanWindowInfo[i].u4PanScanHorizOffset = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 18);
        prWMVPPS->rPanScanWindowInfo[i].u4PanScanVertOffset = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 18);
        prWMVPPS->rPanScanWindowInfo[i].u4PanScanWidth = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 14);
        prWMVPPS->rPanScanWindowInfo[i].u4PanScanHeight = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 14);
        AddPicHdrBitCount(u4InstID, 64);
      }
    } // fgPSPresent
  }

  prWMVPPS->i4RndCtrl = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
  AddPicHdrBitCount(u4InstID, 1);
  /*fgUVProgressiveSubsampling =*/ u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
  AddPicHdrBitCount(u4InstID, 1);

  if(prWMVEPS->fgRefDistPresent && ((prWMVPPS->ucFirstFieldType == IVOP) || (prWMVPPS->ucFirstFieldType == PVOP)))
  {
    prWMVEPS->i4RefFrameDistance = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 2);
    AddPicHdrBitCount(u4InstID, 2);
    if(prWMVEPS->i4RefFrameDistance == 3)
    {
      while(u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1))
      {
        AddPicHdrBitCount(u4InstID, 1);
        prWMVEPS->i4RefFrameDistance++;
      }
      AddPicHdrBitCount(u4InstID, 1);
    }
  }

  // Decode B frac
  if((prWMVPPS->ucFirstFieldType == BVOP) || (prWMVPPS->ucFirstFieldType == BIVOP))
  {
    INT32 iShort = 0, iLong = 0;
    iShort = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 3);
    AddPicHdrBitCount(u4InstID, 3);
    if(iShort == 0x7)
    {
      iLong = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 4);
      AddPicHdrBitCount(u4InstID, 4);
      if(iLong == 0xe) // "Invalid" in VLC
        return INVALID_32;
  
      if(iLong == 0xf)
        prWMVPPS->ucPicType = BIVOP;
      else
      {
        prWMVPPS->i4BNumerator = _iNumLongVLC[iLong];
        prWMVPPS->i4BDenominator = _iDenLongVLC[iLong];
        prWMVPPS->i4BFrameReciprocal = _iBInverse[prWMVPPS->i4BDenominator - 1];
        if(prWMVSPS->i4NumBFrames == 0)
        {
          prWMVSPS->i4NumBFrames = 1; 
        }
      }
    }
    else
    {
      prWMVPPS->i4BNumerator = _iNumShortVLC[iShort];
      prWMVPPS->i4BDenominator = _iDenShortVLC[iShort];
      prWMVPPS->i4BFrameReciprocal = _iBInverse[prWMVPPS->i4BDenominator - 1];
      if(prWMVSPS->i4NumBFrames == 0)
      {
        prWMVSPS->i4NumBFrames = 1; 
      }
    }
  }

  return(TRUE);
}

UINT32 decodeFieldHeadFieldPicture(UINT32 u4InstID)
{
  //INT32 iPpMethod;

  BOOL fgTransTypeMB;
  INT32 iTransTypeFrame;
  VDEC_INFO_WMV_SEQ_PRM_T *prWMVSPS = &_rWMVSPS[u4InstID];
  VDEC_INFO_WMV_ETRY_PRM_T *prWMVEPS = &_rWMVEPS[u4InstID];
  VDEC_INFO_WMV_PIC_PRM_T *prWMVPPS = &_rWMVPPS[u4InstID];
  VDEC_INFO_WMV_DEC_BP_PRM_T rWmvDecBpPrm;

  prWMVPPS->i4PicQtIdx = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 5);
  AddPicHdrBitCount(u4InstID, 5);

  if(prWMVPPS->i4PicQtIdx <= MAXHALFQP)
  {
    prWMVPPS->fgHalfStep = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
    AddPicHdrBitCount(u4InstID, 1);
  }
  else
  {
    prWMVPPS->fgHalfStep = FALSE;
  }

  if(prWMVEPS->fgExplicitFrameQuantizer)
  {
    prWMVPPS->fgUse3QPDZQuantizer = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
    AddPicHdrBitCount(u4InstID, 1);
  }

  if(!prWMVEPS->fgExplicitQuantizer)
  {
    if(prWMVPPS->i4PicQtIdx <= MAX3QP)
    {
      prWMVPPS->fgUse3QPDZQuantizer = TRUE;
      prWMVPPS->i4StepSize = prWMVPPS->i4PicQtIdx;
    }
    else
    {
      prWMVPPS->fgUse3QPDZQuantizer = FALSE;
      prWMVPPS->i4StepSize = _iStepRemap[prWMVPPS->i4PicQtIdx];
    }
  }
  else // Explicit quantizer
    prWMVPPS->i4StepSize = prWMVPPS->i4PicQtIdx;

  prWMVPPS->prDQuantParam = prWMVPPS->fgUse3QPDZQuantizer ? prWMVPPS->rDQuantParam3QPDeadzone : prWMVPPS->rDQuantParam5QPDeadzone;

  prWMVPPS->i4Overlap = 0;
  if(prWMVEPS->fgSequenceOverlap && (prWMVPPS->ucPicType != BVOP))
  {
    if(prWMVPPS->i4StepSize >= 9)
      prWMVPPS->i4Overlap = 1;
    else if(prWMVPPS->ucPicType == IVOP || prWMVPPS->ucPicType == BIVOP)
      prWMVPPS->i4Overlap = 7; // last 3 bits: [MB switch=1/frame switch=0][sent=1/implied=0][on=1/off=0]
  }

  if(prWMVSPS->fgPostProcInfoPresent)
  {
    /*iPpMethod =*/ u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 2);
    AddPicHdrBitCount(u4InstID, 2);
  }

  if((prWMVPPS->ucPicType == IVOP) || (prWMVPPS->ucPicType == BIVOP))
  {
    //ACPRED, Bitplane
    vVDecWmvRecBpType(u4InstID, 5, &rWmvDecBpPrm);
    before_bp(u4InstID);
    i4VDEC_HAL_WMV_HWDecBP(u4InstID, 5, &rWmvDecBpPrm);
    after_bp(u4InstID);

    if(prWMVPPS->i4Overlap & 2)
    {
      if(0 == u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1))
      {
        prWMVPPS->i4Overlap = 0;
        AddPicHdrBitCount(u4InstID, 1);
      }
      else if(0 == u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1))
      {
        prWMVPPS->i4Overlap = 1;
        AddPicHdrBitCount(u4InstID, 2);
      }
      else
      {
        AddPicHdrBitCount(u4InstID, 2);
        // "OVERFLAGS", bp_num = 6
        vVDecWmvRecBpType(u4InstID, 6, &rWmvDecBpPrm);
        before_bp(u4InstID);
        i4VDEC_HAL_WMV_HWDecBP(u4InstID, 6, &rWmvDecBpPrm);
        after_bp(u4InstID);
      }
    }

    prWMVPPS->u4DCTACInterTableIndx = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
    AddPicHdrBitCount(u4InstID, 1);
    if(prWMVPPS->u4DCTACInterTableIndx)
    {
      prWMVPPS->u4DCTACInterTableIndx += u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
      AddPicHdrBitCount(u4InstID, 1);
    }
    prWMVPPS->u4DCTACIntraTableIndx = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
    AddPicHdrBitCount(u4InstID, 1);
    if(prWMVPPS->u4DCTACIntraTableIndx)
    {
      prWMVPPS->u4DCTACIntraTableIndx += u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
      AddPicHdrBitCount(u4InstID, 1);
    }

    prWMVPPS->fgIntraDCTDCTable = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
    AddPicHdrBitCount(u4InstID, 1);

    if(prWMVEPS->i4DQuantCodingOn != 0)
      vDecodeVOPDQuant(u4InstID);
    else
      vSetDefaultDQuantSetting(u4InstID);

    if(prWMVPPS->ucPicType == IVOP)
    {
      prWMVPPS->fgLuminanceWarp = FALSE;
      prWMVPPS->fgLuminanceWarpTop = prWMVPPS->fgLuminanceWarpBottom = FALSE;
    }

  }// End of IVOP, BIVOP
  else if((prWMVPPS->ucPicType == PVOP) || (prWMVPPS->ucPicType == BVOP))
  {
    BOOL fgUseMostRecentFieldForRef;

   //NUMREF
    if(prWMVPPS->ucPicType != BVOP)
    {
      prWMVPPS->fgTwoRefPictures = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
      AddPicHdrBitCount(u4InstID, 1);
    }
    else
    {
      prWMVPPS->fgTwoRefPictures = TRUE; // For BVOP, _fgTwoRefPictures is always TRUE;
    }

    if(prWMVPPS->fgTwoRefPictures)
    {
      prWMVPPS->fgUseSameFieldForRef = TRUE;
      prWMVPPS->fgUseOppFieldForRef = TRUE;
    }
    else
    {
      fgUseMostRecentFieldForRef = !u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
      AddPicHdrBitCount(u4InstID, 1);
      if(fgUseMostRecentFieldForRef)
      {
        prWMVPPS->fgUseSameFieldForRef = FALSE;
        prWMVPPS->fgUseOppFieldForRef = TRUE;
      }
      else
      {
        prWMVPPS->fgUseSameFieldForRef = TRUE;
        prWMVPPS->fgUseOppFieldForRef = FALSE;
      }
    }

    if(prWMVEPS->fgExtendedMvMode)
    {
      // MVRANGE0 = 0, MVRANGE1 = 1,  MVRANGE2 = 2, MVRANGE3 = 3
      prWMVEPS->i4MVRangeIndex = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1); // 0: MVRANGE0;
      AddPicHdrBitCount(u4InstID, 1);
      if(prWMVEPS->i4MVRangeIndex)
      {
        prWMVEPS->i4MVRangeIndex += u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1); // 1: MVRANGE1
        AddPicHdrBitCount(u4InstID, 1);
        if(prWMVEPS->i4MVRangeIndex == 2)
        {
          prWMVEPS->i4MVRangeIndex += u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1); // 2: MVRANGE2,  3: MVRANGE3
          AddPicHdrBitCount(u4InstID, 1);
        }
      }
    }

    if(prWMVEPS->fgExtendedDeltaMvMode)
    {
      prWMVEPS->i4DeltaMVRangeIndex = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
      AddPicHdrBitCount(u4InstID, 1);
      if(prWMVEPS->i4DeltaMVRangeIndex)
	{
        prWMVEPS->i4DeltaMVRangeIndex += u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
        AddPicHdrBitCount(u4InstID, 1);
        if(prWMVEPS->i4DeltaMVRangeIndex == 2)
	  {
          prWMVEPS->i4DeltaMVRangeIndex += u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
          AddPicHdrBitCount(u4InstID, 1);
	  }
	}
      prWMVEPS->i4ExtendedDMVX = _rWMVEPS[u4InstID].i4DeltaMVRangeIndex & 1;
      prWMVEPS->i4ExtendedDMVY = (_rWMVEPS[u4InstID].i4DeltaMVRangeIndex & 2) >> 1;
    }

    prWMVPPS->fgLuminanceWarp = FALSE;
    prWMVPPS->fgLuminanceWarpTop = prWMVPPS->fgLuminanceWarpBottom = FALSE;

    //MVMODE
    prWMVPPS->i4X9MVMode = iGetIFMvMode(u4InstID, prWMVPPS->i4StepSize, FALSE);
    if(prWMVPPS->i4X9MVMode == INTENSITY_COMPENSATION) //PVOP only.
    {
      prWMVPPS->fgLuminanceWarp = TRUE;
      prWMVPPS->i4X9MVMode = iGetIFMvMode(u4InstID, prWMVPPS->i4StepSize, TRUE);
      if(u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1))
      {
        AddPicHdrBitCount(u4InstID, 1);
        // Both reference field remapped
        prWMVPPS->fgLuminanceWarpTop = prWMVPPS->fgLuminanceWarpBottom = TRUE;
        prWMVPPS->i4LumScaleTop = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 6);
        prWMVPPS->i4LumShiftTop = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 6);
        prWMVPPS->i4LumScaleBottom = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 6);
        prWMVPPS->i4LumShiftBottom = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 6);
        AddPicHdrBitCount(u4InstID, 24);
      }
      else if(u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1))
      {
        AddPicHdrBitCount(u4InstID, 2);
        // Bottom reference field remapped
        prWMVPPS->fgLuminanceWarpBottom = TRUE;
        prWMVPPS->i4LumScaleBottom = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 6);
        prWMVPPS->i4LumShiftBottom = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 6);
        AddPicHdrBitCount(u4InstID, 12);
      }
      else
      {
        AddPicHdrBitCount(u4InstID, 2);
        // Top reference field remapped 
        prWMVPPS->fgLuminanceWarpTop = TRUE;
        prWMVPPS->i4LumScaleTop = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 6);
        prWMVPPS->i4LumShiftTop = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 6);
        AddPicHdrBitCount(u4InstID, 12);
      }      
    }

    if(prWMVPPS->ucPicType == BVOP)
    {
      //"FORWARDMB", bp_num = 7
      vVDecWmvRecBpType(u4InstID, 7, &rWmvDecBpPrm);
      before_bp(u4InstID);
      i4VDEC_HAL_WMV_HWDecBP(u4InstID, 7, &rWmvDecBpPrm);
      after_bp(u4InstID);
    }

    prWMVSPS->fgCODFlagOn = TRUE;
    // read MV and CBP codetable indices
    prWMVPPS->i4MBModeTable = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 3);
    AddPicHdrBitCount(u4InstID, 3);
    prWMVPPS->i4MvTable = (prWMVPPS->fgTwoRefPictures) ? u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 3) : u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 2);
    if(prWMVPPS->fgTwoRefPictures)
    {
      AddPicHdrBitCount(u4InstID, 3);
    }
    else
    {
      AddPicHdrBitCount(u4InstID, 2);
    }
    prWMVPPS->i4CBPTable = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 3);
    AddPicHdrBitCount(u4InstID, 3);
    if(prWMVPPS->i4X9MVMode == MIXED_MV)
    {
      prWMVPPS->i44MVBPTable = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 2);
      AddPicHdrBitCount(u4InstID, 2);
    }

    if(prWMVEPS->i4DQuantCodingOn)
    {
      vDecodeVOPDQuant(u4InstID);
    }

    if(prWMVEPS->fgXformSwitch)
    {
      fgTransTypeMB = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
      AddPicHdrBitCount(u4InstID, 1);
      if(fgTransTypeMB)
      {
        prWMVPPS->fgMBXformSwitching = FALSE;
        iTransTypeFrame = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 2);
        AddPicHdrBitCount(u4InstID, 2);
        prWMVPPS->i4FrameXformMode = s_pXformLUT_verify[iTransTypeFrame];
      }
      else
        prWMVPPS->fgMBXformSwitching = TRUE;
    }
    else
    {
      prWMVPPS->fgMBXformSwitching = FALSE;
    }

    prWMVPPS->u4DCTACInterTableIndx = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1); // Coding set index = 0, 1, or 2.
    AddPicHdrBitCount(u4InstID, 1);
    if(prWMVPPS->u4DCTACInterTableIndx == 1)
    {
      prWMVPPS->u4DCTACInterTableIndx += u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
      AddPicHdrBitCount(u4InstID, 1);
    }
    prWMVPPS->u4DCTACIntraTableIndx = prWMVPPS->u4DCTACInterTableIndx;

    prWMVPPS->fgIntraDCTDCTable = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
    AddPicHdrBitCount(u4InstID, 1);
  } // PVOP, BVOP

//  if((prWMVPPS->ucPicType == PVOP) || (prWMVPPS->ucPicType == BVOP)) 
  {
    UpdateDCStepSize(u4InstID, prWMVPPS->i4StepSize);
  }

  return(TRUE);
}

INT32 iGetIFMvMode(UINT32 u4InstID, INT32 iPQuant, BOOL fgRepeat) //for Interlace Field
{
  VDEC_INFO_WMV_PIC_PRM_T *prWMVPPS = &_rWMVPPS[u4InstID];
  INT32 iMvMode;

  if(iPQuant > 12)
  { // P/B Field Picture Low Rate
    if(u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1))
    {
      iMvMode = ALL_1MV_HALFPEL_BILINEAR; // 1b
      AddPicHdrBitCount(u4InstID, 1);
    }
    else if(u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1))
    {
      iMvMode = ALL_1MV; // 01b
      AddPicHdrBitCount(u4InstID, 2);
    }
    else if(u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1))
    {
      iMvMode = ALL_1MV_HALFPEL; // 001b
      AddPicHdrBitCount(u4InstID, 3);
    }
    else
    {
      if(prWMVPPS->ucPicType == BVOP)
      {
        iMvMode = MIXED_MV;
        AddPicHdrBitCount(u4InstID, 3);
      }
      else
      {
        if(fgRepeat || (!u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1)))
        {
          iMvMode = MIXED_MV; // 0000b
          if(fgRepeat)
          {
            AddPicHdrBitCount(u4InstID, 3);
          }
          else
          {
            AddPicHdrBitCount(u4InstID, 4);
          }
        }
        else
        {
          iMvMode = INTENSITY_COMPENSATION; // 0001b
          AddPicHdrBitCount(u4InstID, 4);
        }      
      }
    }
  }
  else
  { // P/B Field Picture High rate
    if(u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1))
    {
      iMvMode = ALL_1MV; // 1b
      AddPicHdrBitCount(u4InstID, 1);
    }
    else if(u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1))
    {
      iMvMode = MIXED_MV; // 01b
      AddPicHdrBitCount(u4InstID, 2);
    }
    else if(u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1))
    {
      iMvMode = ALL_1MV_HALFPEL; // 001b
      AddPicHdrBitCount(u4InstID, 3);
    }
    else
    {
      if(prWMVPPS->ucPicType == BVOP)
      {
        iMvMode = ALL_1MV_HALFPEL_BILINEAR;
        AddPicHdrBitCount(u4InstID, 3);
      }
      else
      {
        if(fgRepeat || (!u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1)))
        {
          iMvMode = ALL_1MV_HALFPEL_BILINEAR; // 0000b
          if(fgRepeat)
          {
            AddPicHdrBitCount(u4InstID, 3);
          }
          else
          {
            AddPicHdrBitCount(u4InstID, 4);
          }
        }
        else
        {
          iMvMode = INTENSITY_COMPENSATION; // 0001b
          AddPicHdrBitCount(u4InstID, 4);
        }
      }
    }
  }
  return iMvMode;
}

void SetupFieldPictureMVScaling(UINT32 u4InstID, INT32 i4RefFrameDistance)
{
  VDEC_INFO_WMV_PIC_PRM_T *prWMVPPS = &_rWMVPPS[u4InstID];
  
    if (i4RefFrameDistance > 3)
        i4RefFrameDistance = 3;
    
    if (prWMVPPS->i4CurrentTemporalField == 0) {
        prWMVPPS->i4MaxZone1ScaledFarMVX = s_sMVScaleValuesFirstField_verify[i4RefFrameDistance].i4MaxZone1ScaledFarMVX;
        prWMVPPS->i4MaxZone1ScaledFarMVY = s_sMVScaleValuesFirstField_verify[i4RefFrameDistance].i4MaxZone1ScaledFarMVY;
        prWMVPPS->i4Zone1OffsetScaledFarMVX = s_sMVScaleValuesFirstField_verify[i4RefFrameDistance].i4Zone1OffsetScaledFarMVX;
        prWMVPPS->i4Zone1OffsetScaledFarMVY = s_sMVScaleValuesFirstField_verify[i4RefFrameDistance].i4Zone1OffsetScaledFarMVY;
        prWMVPPS->i4FarFieldScale1 = s_sMVScaleValuesFirstField_verify[i4RefFrameDistance].i4FarFieldScale1;
        prWMVPPS->i4FarFieldScale2 = s_sMVScaleValuesFirstField_verify[i4RefFrameDistance].i4FarFieldScale2;
        prWMVPPS->i4NearFieldScale = s_sMVScaleValuesFirstField_verify[i4RefFrameDistance].i4NearFieldScale;
    }
    else {
        prWMVPPS->i4MaxZone1ScaledFarMVX = s_sMVScaleValuesSecondField_verify[i4RefFrameDistance].i4MaxZone1ScaledFarMVX;
        prWMVPPS->i4MaxZone1ScaledFarMVY = s_sMVScaleValuesSecondField_verify[i4RefFrameDistance].i4MaxZone1ScaledFarMVY;
        prWMVPPS->i4Zone1OffsetScaledFarMVX = s_sMVScaleValuesSecondField_verify[i4RefFrameDistance].i4Zone1OffsetScaledFarMVX;
        prWMVPPS->i4Zone1OffsetScaledFarMVY = s_sMVScaleValuesSecondField_verify[i4RefFrameDistance].i4Zone1OffsetScaledFarMVY;
        prWMVPPS->i4FarFieldScale1 = s_sMVScaleValuesSecondField_verify[i4RefFrameDistance].i4FarFieldScale1;
        prWMVPPS->i4FarFieldScale2 = s_sMVScaleValuesSecondField_verify[i4RefFrameDistance].i4FarFieldScale2;
        prWMVPPS->i4NearFieldScale = s_sMVScaleValuesSecondField_verify[i4RefFrameDistance].i4NearFieldScale;
    }
}

void SetupBackwardBFieldPictureMVScaling(UINT32 u4InstID, INT32 i4RefFrameDistance)
{
  VDEC_INFO_WMV_PIC_PRM_T *prWMVPPS = &_rWMVPPS[u4InstID];
    if (i4RefFrameDistance > 3)
    {
        i4RefFrameDistance = 3;
    }
    if (prWMVPPS->i4CurrentTemporalField == 0) {
        prWMVPPS->i4MaxZone1ScaledFarBackMVX = s_sMVScaleValuesFirstFieldB_verify[i4RefFrameDistance].i4MaxZone1ScaledFarMVX;
        prWMVPPS->i4MaxZone1ScaledFarBackMVY = s_sMVScaleValuesFirstFieldB_verify[i4RefFrameDistance].i4MaxZone1ScaledFarMVY;
        prWMVPPS->i4Zone1OffsetScaledFarBackMVX = s_sMVScaleValuesFirstFieldB_verify[i4RefFrameDistance].i4Zone1OffsetScaledFarMVX;
        prWMVPPS->i4Zone1OffsetScaledFarBackMVY = s_sMVScaleValuesFirstFieldB_verify[i4RefFrameDistance].i4Zone1OffsetScaledFarMVY;
        prWMVPPS->i4FarFieldScaleBack1 = s_sMVScaleValuesFirstFieldB_verify[i4RefFrameDistance].i4FarFieldScale1;
        prWMVPPS->i4FarFieldScaleBack2 = s_sMVScaleValuesFirstFieldB_verify[i4RefFrameDistance].i4FarFieldScale2;
        prWMVPPS->i4NearFieldScaleBack = s_sMVScaleValuesFirstFieldB_verify[i4RefFrameDistance].i4NearFieldScale;
    }
    else {
        prWMVPPS->i4MaxZone1ScaledFarBackMVX = s_sMVScaleValuesSecondFieldB_verify[i4RefFrameDistance].i4MaxZone1ScaledFarMVX;
        prWMVPPS->i4MaxZone1ScaledFarBackMVY = s_sMVScaleValuesSecondFieldB_verify[i4RefFrameDistance].i4MaxZone1ScaledFarMVY;
        prWMVPPS->i4Zone1OffsetScaledFarBackMVX = s_sMVScaleValuesSecondFieldB_verify[i4RefFrameDistance].i4Zone1OffsetScaledFarMVX;
        prWMVPPS->i4Zone1OffsetScaledFarBackMVY = s_sMVScaleValuesSecondFieldB_verify[i4RefFrameDistance].i4Zone1OffsetScaledFarMVY;
        prWMVPPS->i4FarFieldScaleBack1 = s_sMVScaleValuesSecondFieldB_verify[i4RefFrameDistance].i4FarFieldScale1;
        prWMVPPS->i4FarFieldScaleBack2 = s_sMVScaleValuesSecondFieldB_verify[i4RefFrameDistance].i4FarFieldScale2;
        prWMVPPS->i4NearFieldScaleBack = s_sMVScaleValuesSecondFieldB_verify[i4RefFrameDistance].i4NearFieldScale;
    }
}


void decodePFieldMode(UINT32 u4InstID)
{
  VDEC_INFO_WMV_PIC_PRM_T *prWMVPPS = &_rWMVPPS[u4InstID];
  BOOL fgHalfPelMode = ((prWMVPPS->i4X9MVMode == ALL_1MV_HALFPEL) || (prWMVPPS->i4X9MVMode == ALL_1MV_HALFPEL_BILINEAR));

  // Set MV state for B field direct mode
  if(prWMVPPS->i4CurrentField == 0)
    prWMVPPS->fgBackRefTopFieldHalfPelMode = fgHalfPelMode;
  else
    prWMVPPS->fgBackRefBottomFieldHalfPelMode = fgHalfPelMode;
}


void decodeBFieldMode(UINT32 u4InstID)
{
  VDEC_INFO_WMV_PIC_PRM_T *prWMVPPS = &_rWMVPPS[u4InstID];
  if(prWMVPPS->i4CurrentField == 0)
  {
    prWMVPPS->fgBackRefUsedHalfPel = prWMVPPS->fgBackRefTopFieldHalfPelMode;
  }
  else
  {
    prWMVPPS->fgBackRefUsedHalfPel = prWMVPPS->fgBackRefBottomFieldHalfPelMode;
  }
}

UINT32 decodeVOPHeadInterlaceV2(UINT32 u4InstID)
{
    BOOL fgPSPresent;
    UINT32 u4NumberOfPanScanWindows;
    //BOOL fgUVProgressiveSubsampling;
    //INT32 iPpMethod;
    BOOL fgTransTypeMB;
    INT32 iTransTypeFrame;

    UINT32 i, u4Idx,u4GetVal;
    VDEC_INFO_WMV_SEQ_PRM_T *prWMVSPS = &_rWMVSPS[u4InstID];
    VDEC_INFO_WMV_ETRY_PRM_T *prWMVEPS = &_rWMVEPS[u4InstID];
    VDEC_INFO_WMV_PIC_PRM_T *prWMVPPS = &_rWMVPPS[u4InstID];
    VDEC_INFO_WMV_DEC_BP_PRM_T rWmvDecBpPrm;


    prWMVPPS->ucPrevPicType = prWMVPPS->ucPicType;
    // PTYPE
    if (0 == u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1)) {
        prWMVPPS->ucPicType = PVOP;
        AddPicHdrBitCount(u4InstID, 1);
    }
    else if (0 == u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1)) {
        prWMVPPS->ucPicType = BVOP;
        AddPicHdrBitCount(u4InstID, 2);
    }
    else if (0 == u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1)) {
        prWMVPPS->ucPicType = IVOP;
        AddPicHdrBitCount(u4InstID, 3);
    }
    else if (0 == u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1)) {
        prWMVPPS->ucPicType = BIVOP;
        AddPicHdrBitCount(u4InstID, 4);
    }
    else {
        prWMVPPS->ucPicType = SKIPFRAME;
        AddPicHdrBitCount(u4InstID, 4);
    }  


    if (prWMVPPS->ucPicType != SKIPFRAME) { // SKIPFRAME does not contain temporal reference
        if (prWMVSPS->fgTemporalFrmCntr) {
            prWMVPPS->i4TemporalRef = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 8);
            AddPicHdrBitCount(u4InstID, 8);
        }
    }

    if (prWMVSPS->fgBroadcastFlags) {
        if (prWMVSPS->fgInterlacedSource) {
            u4GetVal = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 2);
            AddPicHdrBitCount(u4InstID, 2);
            prWMVPPS->fgTopFieldFirst = (u4GetVal & 0x2) >> 1;
            prWMVPPS->fgRepeatFirstField = u4GetVal & 0x1;
        }
        else {
            prWMVPPS->ucRepeatFrameCount = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 2);
            AddPicHdrBitCount(u4InstID, 2);
        }
    }

    if (prWMVEPS->fgPanScanPresent) {
        fgPSPresent = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
        AddPicHdrBitCount(u4InstID, 1);
        if (fgPSPresent) {
            if (prWMVSPS->fgInterlacedSource)  {
                if (prWMVSPS->fgBroadcastFlags)
                    u4NumberOfPanScanWindows = 2 + prWMVPPS->fgRepeatFirstField;
                else
                    u4NumberOfPanScanWindows = 2;
            }
            else {
                if (prWMVSPS->fgBroadcastFlags)
                    u4NumberOfPanScanWindows = 1 + prWMVPPS->ucRepeatFrameCount;
                else
                    u4NumberOfPanScanWindows = 1;
            }
            for (i = 0; i < u4NumberOfPanScanWindows; i++) {
                prWMVPPS->rPanScanWindowInfo[i].u4PanScanHorizOffset = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 18);
                prWMVPPS->rPanScanWindowInfo[i].u4PanScanVertOffset = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 18);
                prWMVPPS->rPanScanWindowInfo[i].u4PanScanWidth = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 14);
                prWMVPPS->rPanScanWindowInfo[i].u4PanScanHeight = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 14);
                AddPicHdrBitCount(u4InstID, 64);
            }
        } // fgPSPresent
    }

    if (prWMVPPS->ucPicType == SKIPFRAME) {
        return(TRUE);
    }

    prWMVPPS->i4RndCtrl = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
    AddPicHdrBitCount(u4InstID, 1);

    if (prWMVSPS->fgInterlacedSource) {
        /*fgUVProgressiveSubsampling =*/ u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
        AddPicHdrBitCount(u4InstID, 1);
    }

    prWMVPPS->i4PicQtIdx = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 5);
    AddPicHdrBitCount(u4InstID, 5);

    if (prWMVPPS->i4PicQtIdx <= MAXHALFQP) {
        prWMVPPS->fgHalfStep = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
        AddPicHdrBitCount(u4InstID, 1);
    }
    else {
        prWMVPPS->fgHalfStep = FALSE;
    }

    if (prWMVEPS->fgExplicitFrameQuantizer) {
        prWMVPPS->fgUse3QPDZQuantizer = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
        AddPicHdrBitCount(u4InstID, 1);
    }

    if (!prWMVEPS->fgExplicitQuantizer) {
        if (prWMVPPS->i4PicQtIdx <= MAX3QP) {
            prWMVPPS->fgUse3QPDZQuantizer = TRUE;
            prWMVPPS->i4StepSize = prWMVPPS->i4PicQtIdx;
        }
        else {
            prWMVPPS->fgUse3QPDZQuantizer = FALSE;
            prWMVPPS->i4StepSize = _iStepRemap[prWMVPPS->i4PicQtIdx];
        }
    }
    else
        prWMVPPS->i4StepSize = prWMVPPS->i4PicQtIdx;

    prWMVPPS->prDQuantParam = prWMVPPS->fgUse3QPDZQuantizer ? prWMVPPS->rDQuantParam3QPDeadzone : prWMVPPS->rDQuantParam5QPDeadzone;

    prWMVPPS->i4Overlap = 0;
    if (prWMVEPS->fgSequenceOverlap && (prWMVPPS->ucPicType != BVOP)) {
        if (prWMVPPS->i4StepSize >= 9)
            prWMVPPS->i4Overlap = 1;
        else if (prWMVPPS->ucPicType == IVOP || prWMVPPS->ucPicType == BIVOP)
            prWMVPPS->i4Overlap = 7; // last 3 bits: [MB switch=1/frame switch=0][sent=1/implied=0][on=1/off=0]
    }

    if (prWMVSPS->fgPostProcInfoPresent) {
        /*iPpMethod =*/ u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 2);
        AddPicHdrBitCount(u4InstID, 2);
    }

    // Decode B FRACION
    if (prWMVPPS->ucPicType == BVOP) {
        u4Idx = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 3);
        AddPicHdrBitCount(u4InstID, 3);
        if (u4Idx == 0x7) {
            u4Idx = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 4);
            AddPicHdrBitCount(u4InstID, 4);
            if (u4Idx == 0xe) // "Invalid" in VLC
                return INVALID_32;
            prWMVPPS->i4BNumerator = _iNumLongVLC[u4Idx];
            prWMVPPS->i4BDenominator = _iDenLongVLC[u4Idx];
        }
        else {
            prWMVPPS->i4BNumerator = _iNumShortVLC[u4Idx];
            prWMVPPS->i4BDenominator = _iDenShortVLC[u4Idx];
        }
        prWMVPPS->i4BFrameReciprocal = _iBInverse[prWMVPPS->i4BDenominator - 1];
        if (prWMVSPS->i4NumBFrames == 0) {
            prWMVSPS->i4NumBFrames = 1;
        }
    }

    if ((prWMVPPS->ucPicType == IVOP) || (prWMVPPS->ucPicType == BIVOP)) {
        // "FIELDTX", bp_num = 4
        vVDecWmvRecBpType(u4InstID, 4, &rWmvDecBpPrm);
        before_bp(u4InstID);
        i4VDEC_HAL_WMV_HWDecBP(u4InstID, 4, &rWmvDecBpPrm);
        after_bp(u4InstID);

        // "ACPRED", bp_num = 5
        vVDecWmvRecBpType(u4InstID, 5, &rWmvDecBpPrm);
        before_bp(u4InstID);
        i4VDEC_HAL_WMV_HWDecBP(u4InstID, 5, &rWmvDecBpPrm);
        after_bp(u4InstID);

        if (prWMVPPS->i4Overlap & 2) {
            if (0 == u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1)) {
                prWMVPPS->i4Overlap = 0;
                AddPicHdrBitCount(u4InstID, 1);
            }
            else if (0 == u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1)) {
                prWMVPPS->i4Overlap = 1;
                AddPicHdrBitCount(u4InstID, 2);
            }
            else {
                AddPicHdrBitCount(u4InstID, 2);
                // "OVERFLAGS", bp_num = 6
                vVDecWmvRecBpType(u4InstID, 6, &rWmvDecBpPrm);
                before_bp(u4InstID);
                i4VDEC_HAL_WMV_HWDecBP(u4InstID, 6, &rWmvDecBpPrm);
                after_bp(u4InstID);
            }
        }

        prWMVPPS->u4DCTACInterTableIndx = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
        AddPicHdrBitCount(u4InstID, 1);
        if (prWMVPPS->u4DCTACInterTableIndx) {
            prWMVPPS->u4DCTACInterTableIndx += u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
            AddPicHdrBitCount(u4InstID, 1);
        }
        prWMVPPS->u4DCTACIntraTableIndx = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
        AddPicHdrBitCount(u4InstID, 1);
        if (prWMVPPS->u4DCTACIntraTableIndx) {
            prWMVPPS->u4DCTACIntraTableIndx += u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
            AddPicHdrBitCount(u4InstID, 1);
        }

        prWMVPPS->fgIntraDCTDCTable = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
        AddPicHdrBitCount(u4InstID, 1);

        if (prWMVEPS->i4DQuantCodingOn != 0)
            vDecodeVOPDQuant(u4InstID);
        else
            vSetDefaultDQuantSetting(u4InstID);
    }// End of IVOP, BIVOP
    else if ((prWMVPPS->ucPicType == PVOP) || (prWMVPPS->ucPicType == BVOP)) {
        INT32 iMBModeIndex;

        if (prWMVEPS->fgExtendedMvMode) {
            // MVRANGE0 = 0, MVRANGE1 = 1,  MVRANGE2 = 2, MVRANGE3 = 3
            prWMVEPS->i4MVRangeIndex = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1); // 0: MVRANGE0;
            AddPicHdrBitCount(u4InstID, 1);
            if (prWMVEPS->i4MVRangeIndex) {
                prWMVEPS->i4MVRangeIndex += u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1); // 1: MVRANGE1
                AddPicHdrBitCount(u4InstID, 1);
                if (prWMVEPS->i4MVRangeIndex == 2) {
                    prWMVEPS->i4MVRangeIndex += u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1); // 2: MVRANGE2,  3: MVRANGE3
                    AddPicHdrBitCount(u4InstID, 1);
                }
            }
        }

        if (prWMVEPS->fgExtendedDeltaMvMode) {
            prWMVEPS->i4DeltaMVRangeIndex = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
            AddPicHdrBitCount(u4InstID, 1);
            if (prWMVEPS->i4DeltaMVRangeIndex) {
                prWMVEPS->i4DeltaMVRangeIndex += u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
                AddPicHdrBitCount(u4InstID, 1);
                if (prWMVEPS->i4DeltaMVRangeIndex == 2) {
                    prWMVEPS->i4DeltaMVRangeIndex += u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
                    AddPicHdrBitCount(u4InstID, 1);
                }
            }
            prWMVEPS->i4ExtendedDMVX = _rWMVEPS[u4InstID].i4DeltaMVRangeIndex & 1;
            prWMVEPS->i4ExtendedDMVY = (_rWMVEPS[u4InstID].i4DeltaMVRangeIndex & 2) >> 1;
        }

        //    prWMVPPS->fgLuminanceWarpTop = prWMVPPS->fgLuminanceWarpBottom = FALSE;
        //4MVSWITCH
        if (prWMVPPS->ucPicType == BVOP) {
            prWMVPPS->i4X9MVMode = ALL_1MV; //Interlaced Frame B picture has only ALL_1MV mode.
        }
        else {
            if (u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1)) {
                prWMVPPS->i4X9MVMode = MIXED_MV;
            }
            else {
                prWMVPPS->i4X9MVMode = ALL_1MV;
            }
            AddPicHdrBitCount(u4InstID, 1);
        }

        //INTCOMP
        prWMVPPS->fgLuminanceWarp = FALSE;
        if (prWMVPPS->ucPicType == BVOP) {
            u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);               //No matter what value this bit is,
            AddPicHdrBitCount(u4InstID, 1);
            prWMVPPS->fgLuminanceWarp = FALSE; //Shall always be FALSE for Interlaced Frame B picture.
        }
        else { //PVOP
            if (u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1)) {
                prWMVPPS->fgLuminanceWarp = TRUE;
            }
            AddPicHdrBitCount(u4InstID, 1);
        }
        if (prWMVPPS->fgLuminanceWarp) {
            prWMVPPS->i4LumScale = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 6);
            prWMVPPS->i4LumShift = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 6);
            AddPicHdrBitCount(u4InstID, 12);
        }

        if (prWMVPPS->ucPicType == BVOP) {
            //"DIRECTMB", bp_num = 3
            vVDecWmvRecBpType(u4InstID, 3, &rWmvDecBpPrm);
            before_bp(u4InstID);
            i4VDEC_HAL_WMV_HWDecBP(u4InstID, 3, &rWmvDecBpPrm);
            after_bp(u4InstID);
        }

        // "SKIPMB", bp_num = 0
        vVDecWmvRecBpType(u4InstID, 0, &rWmvDecBpPrm);
        before_bp(u4InstID);
        i4VDEC_HAL_WMV_HWDecBP(u4InstID, 0, &rWmvDecBpPrm);
        after_bp(u4InstID);

        prWMVSPS->fgCODFlagOn = TRUE;

        iMBModeIndex = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 2);
        AddPicHdrBitCount(u4InstID, 2);

        if (prWMVPPS->i4X9MVMode == MIXED_MV) {
            prWMVPPS->i4MBModeTable = iMBModeIndex; //ming add
        }
        else {
            prWMVPPS->i4MBModeTable = 4 + (iMBModeIndex & 0x3); //ming add
        }
        prWMVPPS->i4MvTable = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 2);
        prWMVPPS->i4CBPTable = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 3);
        prWMVPPS->i42MVBPTable = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 2);
        AddPicHdrBitCount(u4InstID, 7);

        if ((prWMVPPS->i4X9MVMode == MIXED_MV) || (prWMVPPS->ucPicType == BVOP)) {
            prWMVPPS->i44MVBPTable = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 2);
            AddPicHdrBitCount(u4InstID, 2);
        }

        if (prWMVEPS->i4DQuantCodingOn) {
            vDecodeVOPDQuant(u4InstID);
        }

        if (prWMVEPS->fgXformSwitch) {
            fgTransTypeMB = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
            AddPicHdrBitCount(u4InstID, 1);
            if (fgTransTypeMB) {
                prWMVPPS->fgMBXformSwitching = FALSE;
                iTransTypeFrame = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 2);
                AddPicHdrBitCount(u4InstID, 2);
                prWMVPPS->i4FrameXformMode = s_pXformLUT_verify[iTransTypeFrame];
            }
            else
                prWMVPPS->fgMBXformSwitching = TRUE;
        }
        else {
            prWMVPPS->fgMBXformSwitching = FALSE;
        }

//End==== decodeVOPHead_WMV3() ===================================

        prWMVPPS->u4DCTACInterTableIndx = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1); // Coding set index = 0, 1, or 2.
        AddPicHdrBitCount(u4InstID, 1);
        if (prWMVPPS->u4DCTACInterTableIndx == 1) {
            prWMVPPS->u4DCTACInterTableIndx += u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
            AddPicHdrBitCount(u4InstID, 1);
        }
        prWMVPPS->u4DCTACIntraTableIndx = prWMVPPS->u4DCTACInterTableIndx;

        prWMVPPS->fgIntraDCTDCTable = u4VDEC_HAL_WMV_GetRealBitStream(_u4BSID[u4InstID], u4InstID, 1);
        AddPicHdrBitCount(u4InstID, 1);
//=================================================================
    } // PVOP, BVOP
    return(TRUE);
}

void vVDecWmvRecBpType(UINT32 u4InstID, UINT32 u4BpNum, VDEC_INFO_WMV_DEC_BP_PRM_T *prWmvDecBpPrm)
{
  VDEC_INFO_WMV_SEQ_PRM_T *prWMVSPS = &_rWMVSPS[u4InstID];
  VDEC_INFO_WMV_PIC_PRM_T *prWMVPPS = &_rWMVPPS[u4InstID];
  UINT32 u4Tmp = (u4VDEC_HAL_WMV_ShiftGetBitStream(0, u4InstID, 0) >> 27) & 0xF;

  if(u4Tmp == 0)
  {
    switch(u4BpNum)
    {
      case 0: //SKIP
        prWMVPPS->u4BPRawFlag |= 0x1;
        break;
      case 1: //4MV
        prWMVPPS->u4BPRawFlag |= 0x2;
        break;
      case 3: //DIRECT
        prWMVPPS->u4BPRawFlag |= 0x4;
        break;
      case 4: //FIELD
        prWMVPPS->u4BPRawFlag |= 0x8;
        break;
      case 5: //ACPRED
        prWMVPPS->u4BPRawFlag |= 0x10;
        break;
      case 6: //OVERLAP
        prWMVPPS->u4BPRawFlag |= 0x20;
        break;
      case 7: //FORWARD
        prWMVPPS->u4BPRawFlag |= 0x40;
        break;
      default:
        prWMVPPS->u4BPRawFlag = 0;
        break;
    }
  }
  prWmvDecBpPrm->ucFrameCodingMode = prWMVPPS->ucFrameCodingMode;
  prWmvDecBpPrm->ucPicType = prWMVPPS->ucPicType;
  prWmvDecBpPrm->i4CodecVersion = _i4CodecVersion[u4InstID];
  prWmvDecBpPrm->i4Wmv8BpMode = prWMVSPS->i4Wmv8BpMode;
  prWmvDecBpPrm->u4NumMBX = prWMVSPS->u4NumMBX;
  #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
  prWmvDecBpPrm->u4NumMBY = prWMVSPS->u4NumMBY;
  #endif
  prWmvDecBpPrm->u4PicHeightDec = prWMVSPS->u4PicHeightDec;
  prWmvDecBpPrm->u4PicHeightSrc = prWMVSPS->u4PicHeightSrc;
  prWmvDecBpPrm->u4PicHeightSrc = prWMVSPS->u4PicHeightSrc;     
  prWmvDecBpPrm->fgWmvMode = _u4WmvMode[u4InstID];

if (!prWmvDecBpPrm->fgWmvMode)
{
  prWmvDecBpPrm->rWmvWorkBufSa.u4Bp1Sa = (UINT32)_pucBp_1[u4InstID];
  prWmvDecBpPrm->rWmvWorkBufSa.u4Bp2Sa = (UINT32)_pucBp_2[u4InstID];
  prWmvDecBpPrm->rWmvWorkBufSa.u4Bp3Sa = (UINT32)_pucBp_3[u4InstID];
  prWmvDecBpPrm->rWmvWorkBufSa.u4Bp4Sa = (UINT32)_pucBp_4[u4InstID];
  prWmvDecBpPrm->rWmvWorkBufSa.u4Dcac2Sa = (UINT32)_pucDcac_2[u4InstID];
  prWmvDecBpPrm->rWmvWorkBufSa.u4DcacSa = (UINT32)_pucDcac[u4InstID];
  prWmvDecBpPrm->rWmvWorkBufSa.u4Mv12Sa = (UINT32)_pucMv_1_2[u4InstID];
  prWmvDecBpPrm->rWmvWorkBufSa.u4Mv1Sa = (UINT32)_pucMv_1[u4InstID];
  prWmvDecBpPrm->rWmvWorkBufSa.u4Mv2Sa = (UINT32)_pucMv_2[u4InstID];
  prWmvDecBpPrm->rWmvWorkBufSa.u4Mv3Sa = (UINT32)_pucMv_3[u4InstID];
}
else
{
  prWmvDecBpPrm->rWmvWorkBufSa.u4DcacNewSa = (UINT32)_pucDcacNew[u4InstID];
  prWmvDecBpPrm->rWmvWorkBufSa.u4MvNewSa    = (UINT32)_pucMvNew[u4InstID];
  prWmvDecBpPrm->rWmvWorkBufSa.u4Bp0NewSa   = (UINT32)_pucBp0New[u4InstID];
  prWmvDecBpPrm->rWmvWorkBufSa.u4Bp1NewSa   = (UINT32)_pucBp1New[u4InstID];
  prWmvDecBpPrm->rWmvWorkBufSa.u4Bp2NewSa   = (UINT32)_pucBp2New[u4InstID];
}

  prWmvDecBpPrm->rWmvWorkBufSa.u4Pic0CSa = (UINT32)_pucPic0C[u4InstID];
  prWmvDecBpPrm->rWmvWorkBufSa.u4Pic0YSa = (UINT32)_pucPic0Y[u4InstID];
  prWmvDecBpPrm->rWmvWorkBufSa.u4Pic1CSa = (UINT32)_pucPic1C[u4InstID];
  prWmvDecBpPrm->rWmvWorkBufSa.u4Pic1YSa = (UINT32)_pucPic1Y[u4InstID];
  prWmvDecBpPrm->rWmvWorkBufSa.u4Pic2CSa = (UINT32)_pucPic2C[u4InstID];
  prWmvDecBpPrm->rWmvWorkBufSa.u4Pic2YSa = (UINT32)_pucPic2Y[u4InstID];
  prWmvDecBpPrm->rWmvWorkBufSa.u4Pp1Sa = (UINT32)_pucPp_1[u4InstID];
  prWmvDecBpPrm->rWmvWorkBufSa.u4Pp2Sa = (UINT32)_pucPp_2[u4InstID];   
}


/* Intensity compensation */
void cal_icomp(INT32 *i4Scale, INT32 *i4Shift, INT32  m_iLuminanceScale, INT32 m_iLuminanceShift)
{
  /* derived from interpolate_wmv9.c IntensityCompensation() */
  if (m_iLuminanceShift > 31)
    m_iLuminanceShift -= 64;

  // remap luminance scale and shift
  if (m_iLuminanceScale == 0)
  {
    *i4Scale = - 64;
    *i4Shift = 255 * 64 - m_iLuminanceShift * 2 * 64;
  }
  else
  {
    *i4Scale = m_iLuminanceScale + 32;
    *i4Shift = m_iLuminanceShift * 64;
  }
}   

void UpdateVopheaderParam(UINT32 u4InstID)
{
    INT32 iScaleTop, iShiftTop;
    INT32 iScaleBot, iShiftBot;
    VDEC_INFO_WMV_SEQ_PRM_T *prWMVSPS = &_rWMVSPS[u4InstID];
    VDEC_INFO_WMV_ETRY_PRM_T *prWMVEPS = &_rWMVEPS[u4InstID];
    VDEC_INFO_WMV_PIC_PRM_T *prWMVPPS = &_rWMVPPS[u4InstID];
    VDEC_INFO_WMV_ICOMP_PRM_T *prWMVICOMPPS = &_rWMVICOMPPS[u4InstID];



    if ( ((prWMVPPS->ucPicType == IVOP) || (prWMVPPS->ucPicType == PVOP) || (prWMVPPS->ucPicType == SKIPFRAME)) &&
       (/* first field picture */
         ((prWMVPPS->fgFieldMode == TRUE) && (prWMVPPS->i4CurrentTemporalField== 0)) ||
        /* frame picture */
         (prWMVPPS->fgFieldMode == FALSE)) ) {
        prWMVPPS->u4ForwardRefPicType = prWMVPPS->u4BackwardRefPicType;

        if ((_i4CodecVersion[u4InstID] == VDEC_VC1) && (prWMVPPS->fgInterlaceV2)) {
            if (prWMVPPS->fgFieldMode == TRUE) {
                prWMVPPS->u4BackwardRefPicType = INTERLACEFIELD;
            }
            else {
                prWMVPPS->u4BackwardRefPicType = INTERLACEFRAME;
            }
        }
        else {
            prWMVPPS->u4BackwardRefPicType = PROGRESSIVE;
        }
    }

    if ((prWMVSPS->fgXintra8) && (prWMVPPS->ucPicType == IVOP)) {
        return;
    }

    if (prWMVPPS->ucPicType == IVOP) {
        prWMVICOMPPS->ucPreProcessFrameStatus = PP_NO_SCALE;
    }
    else if ( (prWMVPPS->ucPicType == PVOP) || (prWMVPPS->ucPicType == SKIPFRAME) ) /* only for PVOP */ {
        if (prWMVEPS->i4ReconRangeState == 0) { //pWMVDec->m_iReconRangeState == 0)
            if (prWMVEPS->i4RangeState == 1) {
                // JUP comment
                // Previous not scale range
                // Current scale down range
                // Hence, scan DOWN previous frame to using it for current frame's motion compensation
                //
                // reduce by 2
                prWMVICOMPPS->ucPreProcessFrameStatus = PP_SCALE_DOWN;
            }
            else {
                prWMVICOMPPS->ucPreProcessFrameStatus = PP_NO_SCALE;
            }
        } else if (prWMVEPS->i4ReconRangeState == 1) {//pWMVDec->m_iReconRangeState == 1)
            if (prWMVEPS->i4RangeState == 0) {
                // JUP comment
                // Previous scale down range
                // Current not scale range
                // Hence, scan UP previous frame to using it for current frame's motion compensation
                //
                // increase by 2
                prWMVICOMPPS->ucPreProcessFrameStatus = PP_SCALE_UP;
            }
            else {
                prWMVICOMPPS->ucPreProcessFrameStatus = PP_NO_SCALE;
            }
        }
    }


    //
    // Update Icomp parameters for both IVOP and PVOP
    //
    if (_i4CodecVersion[u4InstID] == VDEC_VC1) {
        if ( (prWMVPPS->ucPicType == PVOP) || (prWMVPPS->ucPicType == IVOP) || (prWMVPPS->ucPicType == SKIPFRAME) ) {
            if (prWMVPPS->ucPicType == IVOP) {
                prWMVPPS->fgLuminanceWarpTop = 0;
                prWMVPPS->fgLuminanceWarpBottom = 0;
                prWMVPPS->fgLuminanceWarp = 0;
            }
            if (prWMVPPS->fgFieldMode == TRUE) { //field picture
                if (prWMVPPS->i4CurrentTemporalField== 0) { // the first P field picture
                    prWMVICOMPPS->i4BoundaryUMVIcomp = 0;

                    /* Step 1: Update Icomp parameters, Old <= New */
                    //prWMVICOMPPS->NewTopField.Old = prWMVICOMPPS->NewTopField.New; 
                    //prWMVICOMPPS->NewBotField.Old = prWMVICOMPPS->NewBotField.New; 

                    if (prWMVPPS->i4CurrentField == 0) { //current decode field is top field
                        /* Step 1: Update Icomp parameters     */
                        /* OLD_BF <= NEW_BF                */ 

                        prWMVICOMPPS->OldBotField.Old = prWMVICOMPPS->NewBotField.Old; 
                        prWMVICOMPPS->OldBotField.New = prWMVICOMPPS->NewBotField.New;

                        /* reset NEW_BF */
                        prWMVICOMPPS->NewBotField.Old.i4Enable = 0;
                        prWMVICOMPPS->NewBotField.New.i4Enable = 0;

                        /* update NEW_TF, OLD <= NEW */
                        prWMVICOMPPS->NewTopField.Old = prWMVICOMPPS->NewTopField.New;      

                        if ( /* forward reference picture is frame picture */ /* and decode the first field picture now */
                            (prWMVPPS->u4ForwardRefPicType == PROGRESSIVE) || (prWMVPPS->u4ForwardRefPicType == INTERLACEFRAME)) {
                            prWMVICOMPPS->NewTopField.Old.i4Enable = 0;
                        }

                    }
                    else { // current decode field is bottom field  
                        /* Step 1: Update Icomp parameters     */
                        /* OLD_TF <= NEW_TF                */ 

                        prWMVICOMPPS->OldTopField.Old = prWMVICOMPPS->NewTopField.Old; 
                        prWMVICOMPPS->OldTopField.New = prWMVICOMPPS->NewTopField.New; 

                        /* reset NEW_TF */
                        prWMVICOMPPS->NewTopField.Old.i4Enable = 0;
                        prWMVICOMPPS->NewTopField.New.i4Enable = 0;   

                        /* update NEW_BF, OLD <= NEW */
                        prWMVICOMPPS->NewBotField.Old = prWMVICOMPPS->NewBotField.New;

                        if ( /* forward reference picture is frame picture */ /* and decode the first field picture now */
                            (prWMVPPS->u4ForwardRefPicType == PROGRESSIVE) || (prWMVPPS->u4ForwardRefPicType == INTERLACEFRAME)) {
                            prWMVICOMPPS->NewBotField.Old.i4Enable = 0;
                        }                      
                    }

                    /* Step 2: calculate Icomp parameters, New <= Icomp */
                    if (prWMVPPS->fgLuminanceWarpTop == TRUE) {
                        cal_icomp(&iScaleTop, &iShiftTop, prWMVPPS->i4LumScaleTop, prWMVPPS->i4LumShiftTop);
                        prWMVICOMPPS->NewTopField.New.i4Shift = iShiftTop;           
                        prWMVICOMPPS->NewTopField.New.i4Scale = iScaleTop;   
                        prWMVICOMPPS->NewTopField.New.i4Enable = 1;
                    }
                    else {
                        prWMVICOMPPS->NewTopField.New.i4Enable = 0;
                    }

                    if (prWMVPPS->fgLuminanceWarpBottom == TRUE) {
                        cal_icomp(&iScaleBot, &iShiftBot, prWMVPPS->i4LumScaleBottom, prWMVPPS->i4LumShiftBottom);
                        prWMVICOMPPS->NewBotField.New.i4Shift = iShiftBot;
                        prWMVICOMPPS->NewBotField.New.i4Scale = iScaleBot;   
                        prWMVICOMPPS->NewBotField.New.i4Enable = 1;           
                    }
                    else {
                        prWMVICOMPPS->NewBotField.New.i4Enable = 0;
                    }                        
                }       
                else { // the second P field picture 
                    if ((prWMVPPS->u4ForwardRefPicType == PROGRESSIVE) && (prWMVPPS->u4BackwardRefPicType == INTERLACEFIELD)) {
                    /* && this is a second field */
                        prWMVICOMPPS->i4BoundaryUMVIcomp = 1;
                    }
                    else {
                        prWMVICOMPPS->i4BoundaryUMVIcomp = 0;
                    }

                    prWMVICOMPPS->i4SecondFieldParity = prWMVPPS->i4CurrentField;

                    if (prWMVPPS->i4CurrentField == 0) { //current decode field is top field 
                        /* Step 1: Update Icomp parameters     */
                        /* OLD_BF <= NEW_BF  */          
                        prWMVICOMPPS->OldBotField.Old = prWMVICOMPPS->NewBotField.Old; 
                        prWMVICOMPPS->OldBotField.New = prWMVICOMPPS->NewBotField.New; 

                        /* reset NEW_BF */
                        prWMVICOMPPS->NewBotField.Old.i4Enable = 0;
                        prWMVICOMPPS->NewBotField.New.i4Enable = 0;              

                        /* update NEW_TF: Old <= New   */
                        prWMVICOMPPS->NewTopField.Old = prWMVICOMPPS->NewTopField.New;
                    }
                    else { // current decode field is bottom field
                        /* Step 1: Update Icomp parameters     */
                        /* OLD_TF <= NEW_TF                */       
                        prWMVICOMPPS->OldTopField.Old = prWMVICOMPPS->NewTopField.Old; 
                        prWMVICOMPPS->OldTopField.New = prWMVICOMPPS->NewTopField.New; 

                        /* reset NEW_TF */
                        prWMVICOMPPS->NewTopField.Old.i4Enable = 0;
                        prWMVICOMPPS->NewTopField.New.i4Enable = 0;              

                        /* update NEW_BF: Old <= New            */   
                        prWMVICOMPPS->NewBotField.Old = prWMVICOMPPS->NewBotField.New;
                    }

                    /* Step 2: calculate Icomp parameters, both NEW_BF.New amd NEW_TF.New <= Icomp */
                        if (prWMVPPS->fgLuminanceWarpTop == TRUE) {
                        cal_icomp(&iScaleTop, &iShiftTop, prWMVPPS->i4LumScaleTop, prWMVPPS->i4LumShiftTop);
                        prWMVICOMPPS->NewTopField.New.i4Shift = iShiftTop;           
                        prWMVICOMPPS->NewTopField.New.i4Scale = iScaleTop;   
                        prWMVICOMPPS->NewTopField.New.i4Enable = 1;
                    }
                    else {
                        prWMVICOMPPS->NewTopField.New.i4Enable = 0;
                    }

                    if (prWMVPPS->fgLuminanceWarpBottom == TRUE) {
                        cal_icomp(&iScaleBot, &iShiftBot, prWMVPPS->i4LumScaleBottom, prWMVPPS->i4LumShiftBottom);
                        prWMVICOMPPS->NewBotField.New.i4Shift = iShiftBot;           
                        prWMVICOMPPS->NewBotField.New.i4Scale = iScaleBot;   
                        prWMVICOMPPS->NewBotField.New.i4Enable = 1;           
                    }
                    else {
                        prWMVICOMPPS->NewBotField.New.i4Enable = 0;
                    }
                }
            } //end : if (prWMVPPS->fgFieldMode == TRUE)
            else { // frame picture 
                prWMVICOMPPS->i4BoundaryUMVIcomp = 0;

                /* Step 1: Update Icomp parameters, both NEW_BF and NEW_TF, Old <= New */
                prWMVICOMPPS->NewTopField.Old = prWMVICOMPPS->NewTopField.New; 
                prWMVICOMPPS->NewBotField.Old = prWMVICOMPPS->NewBotField.New; 

                if ( /* forward reference picture is frame picture */
                    (prWMVPPS->u4ForwardRefPicType == PROGRESSIVE) || (prWMVPPS->u4ForwardRefPicType == INTERLACEFRAME)) {
                    prWMVICOMPPS->NewTopField.Old.i4Enable = 0;
                    prWMVICOMPPS->NewBotField.Old.i4Enable = 0;             
                }
                else { /* prWMVPPS->u4ForwardRefPicType == INTERLACE_FIELD */
                    /* previous reference second field is top field */
                    if (prWMVICOMPPS->i4SecondFieldParity == 0) {
                        prWMVICOMPPS->NewTopField.Old.i4Enable = 0;
                    }
                    /* previous reference second field is bottom field */
                    else {
                        prWMVICOMPPS->NewBotField.Old.i4Enable = 0;
                    }
                }

                /* Step 2: calculate Icomp parameters, both NEW_BF and NEW_TF, New <= Icomp */
                if (prWMVPPS->fgLuminanceWarp == TRUE) {
                    cal_icomp(&iScaleTop, &iShiftTop, prWMVPPS->i4LumScale, prWMVPPS->i4LumShift);
                    prWMVICOMPPS->NewTopField.New.i4Shift = iShiftTop;           
                    prWMVICOMPPS->NewTopField.New.i4Scale = iScaleTop;   
                    prWMVICOMPPS->NewTopField.New.i4Enable = 1;
                }
                else {
                    prWMVICOMPPS->NewTopField.New.i4Enable = 0;
                }

                if (prWMVPPS->fgLuminanceWarp == TRUE) {
                    cal_icomp(&iScaleBot, &iShiftBot, prWMVPPS->i4LumScale, prWMVPPS->i4LumShift);
                    prWMVICOMPPS->NewBotField.New.i4Shift = iShiftBot;           
                    prWMVICOMPPS->NewBotField.New.i4Scale = iScaleBot;   
                    prWMVICOMPPS->NewBotField.New.i4Enable = 1;           
                }
                else {
                    prWMVICOMPPS->NewBotField.New.i4Enable = 0;
                }
            } //end : frame picture
        } //end : IVOP PVOP SKIPFRAME
    } //end : if (_i4CodecVersion == WMVA)



    //-------------------------------------------------------
    // intensity compensation 
    // 
    //------------------------------------------------------

    if(( (prWMVPPS->ucPicType == PVOP) || (prWMVPPS->ucPicType == SKIPFRAME) ) &&  (_i4CodecVersion[u4InstID] == VDEC_VC1)) {
        prWMVICOMPPS->i4BoundaryUMVIcompEnable = 0;
        if (prWMVICOMPPS->i4BoundaryUMVIcomp == 1) {
            /* top field picture */
            if (prWMVICOMPPS->i4SecondFieldParity == 0) {
                if (prWMVICOMPPS->OldBotField.New.i4Enable == 1) {
                    prWMVICOMPPS->i4BoundaryUMVIcompEnable = 1;
                }
            }
            /* bottom field picture */
            else {
                if (prWMVICOMPPS->OldTopField.New.i4Enable == 1) {
                    prWMVICOMPPS->i4BoundaryUMVIcompEnable = 1;
                }
            }
        }
    }//end : PVOP
    else if ((prWMVPPS->ucPicType == BVOP) && (_i4CodecVersion[u4InstID] == VDEC_VC1)) {
        if (prWMVPPS->fgFieldMode == TRUE) { //field picture
            if (prWMVPPS->i4CurrentTemporalField== 0) { // the first B field picture
                prWMVICOMPPS->i4BoundaryUMVIcomp = 0;
            } //end : first B field picture
            else { // Second B field picture
                if ((prWMVPPS->u4ForwardRefPicType == PROGRESSIVE) && (prWMVPPS->u4BackwardRefPicType == INTERLACEFIELD)) {
                    /* && this is a second field */
                    prWMVICOMPPS->i4BoundaryUMVIcomp = 1;
                }
                else {
                    prWMVICOMPPS->i4BoundaryUMVIcomp = 0;
                }

                if (prWMVPPS->i4CurrentField == 0) { // current field is top field 
                    if (prWMVICOMPPS->i4SecondFieldParity == 0) {      
                        prWMVICOMPPS->i4BoundaryUMVIcompEnable = 0;
                        if (prWMVICOMPPS->i4BoundaryUMVIcomp == 1) {
                            /* top field picture */
                            if (prWMVICOMPPS->i4SecondFieldParity == 0) {
                                if (prWMVICOMPPS->OldBotField.New.i4Enable == 1) {
                                    prWMVICOMPPS->i4BoundaryUMVIcompEnable = 1;
                                }
                            }
                            else { /* bottom field picture */
                                if (prWMVICOMPPS->OldTopField.New.i4Enable == 1) {
                                    prWMVICOMPPS->i4BoundaryUMVIcompEnable = 1;
                                }
                            }
                        }
                    }
                    else { 
                        prWMVICOMPPS->i4BoundaryUMVIcompEnable = 0;
                        if (prWMVICOMPPS->i4BoundaryUMVIcomp == 1) {
                            /* top field picture */
                            if (prWMVICOMPPS->i4SecondFieldParity == 0) {
                                if (prWMVICOMPPS->OldBotField.New.i4Enable == 1) {         
                                    prWMVICOMPPS->i4BoundaryUMVIcompEnable = 1;
                                }
                            }
                            else { /* bottom field picture */
                                if (prWMVICOMPPS->OldTopField.New.i4Enable == 1) {
                                    prWMVICOMPPS->i4BoundaryUMVIcompEnable = 1;           
                                }
                            }
                        }
                    }
                }
                else { // current field is bottom field 
                    if (prWMVICOMPPS->i4SecondFieldParity == 0) {
                        prWMVICOMPPS->i4BoundaryUMVIcompEnable = 0;  
                        if (prWMVICOMPPS->i4BoundaryUMVIcomp == 1) {
                            /* top field picture */
                            if (prWMVICOMPPS->i4SecondFieldParity == 0) {
                                if (prWMVICOMPPS->OldBotField.New.i4Enable == 1) {         
                                    prWMVICOMPPS->i4BoundaryUMVIcompEnable = 1;
                                }
                            }
                            else { /* bottom field picture */         
                                if (prWMVICOMPPS->OldTopField.New.i4Enable == 1) {
                                    prWMVICOMPPS->i4BoundaryUMVIcompEnable = 1;
                                }
                            }
                        }      
                    }
                    else {
                        prWMVICOMPPS->i4BoundaryUMVIcompEnable = 0;
                        if (prWMVICOMPPS->i4BoundaryUMVIcomp == 1) {
                            /* top field picture */
                            if (prWMVICOMPPS->i4SecondFieldParity == 0) {
                                if (prWMVICOMPPS->OldBotField.New.i4Enable == 1) {
                                    prWMVICOMPPS->i4BoundaryUMVIcompEnable = 1;
                                }
                            }
                            else { /* bottom field picture */         
                                if (prWMVICOMPPS->OldTopField.New.i4Enable == 1) {      
                                    prWMVICOMPPS->i4BoundaryUMVIcompEnable = 1;           
                                }
                            }
                        }
                    }     
                }
            }
        }
        else { // B frame pciture
            prWMVICOMPPS->i4BoundaryUMVIcomp = 0;
        }
    }  
    else { /* pure frame picture case in WMV9 */
        if (prWMVPPS->ucPicType == IVOP) {
            prWMVICOMPPS->i4FirstFieldIntensityComp = 0;
        }
        else if  ( (prWMVPPS->ucPicType == PVOP)  || (prWMVPPS->ucPicType == SKIPFRAME) )/* only for PVOP */ {
            if (prWMVPPS->fgLuminanceWarp) {
                prWMVICOMPPS->i4FirstFieldIntensityComp = 1;
            }
            else {
                prWMVICOMPPS->i4FirstFieldIntensityComp = 0; 
            }
        }
        /* derived from interpolate_wmv9.c IntensityCompensation() */
        if (prWMVPPS->i4LumShift > 31) {
            prWMVPPS->i4LumShift -= 64;
        }
    }



    //-----------------------------------------------------
    // VLD_reg_200 : MV resolution (half or full pel)
    //-----------------------------------------------------
    {
        if ((_i4CodecVersion[u4InstID] == VDEC_WMV1) || (_i4CodecVersion[u4InstID] == VDEC_WMV2)) {
            prWMVICOMPPS->i4ResetMvDram = 0;
        }
        else { // >= WMV3
            if (prWMVICOMPPS->ucFrameTypeLast == SKIPFRAME) {
                if((prWMVICOMPPS->i4ResetMvDram == 1) && ((prWMVPPS->ucPicType == IVOP) || (prWMVPPS->ucPicType == PVOP))) {
                    prWMVICOMPPS->i4ResetMvDram = 0;
                }
                else {
                    prWMVICOMPPS->i4ResetMvDram = 1;
                }
            }
            else {
                if (((prWMVPPS->ucFrameCodingMode == INTERLACEFIELD) && ( ((prWMVPPS->i4CurrentField == 0) && (prWMVICOMPPS->ucFrameTypeLastTop == IVOP)) || ((prWMVPPS->i4CurrentField == 1) && (prWMVICOMPPS->ucFrameTypeLastBot == IVOP)))) ||
                    ((prWMVPPS->ucFrameCodingMode != INTERLACEFIELD) && (prWMVICOMPPS->ucFrameTypeLast == IVOP)) ) {
                    if (prWMVPPS->ucPicType == PVOP) {
                        _new_entry_point[u4InstID] = 0;
                    }
                    prWMVICOMPPS->i4ResetMvDram = 1;
                }
                else if(_new_entry_point[u4InstID] == 1) {
                    if (prWMVPPS->ucPicType == PVOP) {
                        _new_entry_point[u4InstID] = 0;
                    }
                    prWMVICOMPPS->i4ResetMvDram = 1;
                }
                else {
                    prWMVICOMPPS->i4ResetMvDram = 0;
                }
            }

            if ((prWMVPPS->ucPicType == IVOP) || (prWMVPPS->ucPicType == PVOP) || (prWMVPPS->ucPicType == SKIPFRAME)) {
                prWMVICOMPPS->ucFrameTypeLast = prWMVPPS->ucPicType;

                if (prWMVPPS->i4CurrentField == 0) { //top
                    prWMVICOMPPS->ucFrameTypeLastTop = prWMVPPS->ucPicType;
                }
                else { //bot
                    prWMVICOMPPS->ucFrameTypeLastBot = prWMVPPS->ucPicType;
                }
            } //ming modify@07/02/10
        }
    }
}



