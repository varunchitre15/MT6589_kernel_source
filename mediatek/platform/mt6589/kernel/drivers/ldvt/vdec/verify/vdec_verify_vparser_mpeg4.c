#include "vdec_verify_mpv_prov.h"
#include "../hal/vdec_hal_if_common.h"
#include "../hal/vdec_hal_if_mpeg.h"
#include "vdec_verify_keydef.h"
#include "vdec_verify_vparser_mpeg.h"
#include "vdec_verify_vparser_mpeg4.h"

#define fgIsSGmcVop(u4InstID) ((_ucVopCdTp[u4InstID] == VCT_S) && (_ucSpriteEnable[u4InstID] == GMC))

#define u4Div2Slash(v1, v2) (((v1)+(v2)/2)/(v2))

void vVerifyMp4SetDecPrm(UINT32 u4InstID);
void vVerifyGetUpParm(UINT32 u4InstID);
UINT32 u4ShVParser(UINT32 u4InstID, BOOL fgInquiry);
void vVerifySetShDefaultParm(UINT32 u4InstID);
void vVerifyM4vSetVolPrm(UINT32 u4InstID);
void vVerifyM4vSetGmcPrm(UINT32 u4InstID);
void vVerifyM4vSetVopPrm(UINT32 u4InstID);
void vVerifyCalGmcMv(UINT32 u4InstID);
UINT32 u4VopData(void);
UINT32 u4SrcFmtTbl(UINT32 u4InstID, BYTE bShSrcFmt);
UINT32 u4Dx3VParser(UINT32 u4InstID, BOOL fgInquiry);
void vVerifyDx3SetPicLayerPrm(UINT32 u4InstID);
UINT32 u4M4VParser(UINT32 u4InstID, BOOL fgInquiry);
UINT32 u4VisualObject(UINT32 u4InstID);
UINT32 u4VerifyGetDivXNo(BYTE *pbNo);
UINT32 u4VerifyFindChar(BYTE *pbBuf, UINT32 dwLen, BYTE bChar);
void vVerifyUserData(UINT32 u4InstID);
UINT32 u4GoVop(UINT32 u4InstID);
UINT32 u4VOP(UINT32 u4InstID);
UINT32 u4VerifyLSBMask(UINT32 dwBits);
UINT32 u4SpriteTrajectory(UINT32 u4InstID);
INT32 i4WarpingMvCode(UINT32 u4InstID);
UINT32 u4DmvCodeLength(UINT32 u4InstID);
UINT32 u4NormalVOL(UINT32 u4InstID);
void vVerifyGetPxlAspRat(UINT32 u4InstID, BYTE bAspRatInf);
void vVerifySetMpeg2Var(UINT32 u4InstID);
UINT32 u4Vop2Pic(BYTE ucVopCdTp);
BOOL fgMPEG4ValidStartCode(UINT32 u4SC);
BOOL fgVPrsValidStartCode(UINT32 u4InstID, BOOL fgMp4, UINT32 *pdwSC);
extern BOOL fgMPEGNextStartCode(UINT32 u4InstID);
#ifdef FW_WRITE_QUANTIZATION
static void vVerifyParseQtable(UINT32 u4InstID);
#endif


UINT32 u4VParserMPEG4(UINT32 u4InstID, BOOL fgInquiry)
{
  UINT32 dwRetVal = 0;

  _fgVerVopCoded0[u4InstID] = FALSE;

  vVDEC_HAL_MPEG_SetMPEG4Flag(u4InstID, TRUE);

  vVerifyGetUpParm(u4InstID); // get parameter from upper layer (AVI or QT layer)
  // dwVParser4() shall be called only when _ptPicIdx is available and prepared
  switch(_u4CodecVer[u4InstID])
  {
    case VDEC_MPEG4:
    case VDEC_H263:
      _ucMpegVer[u4InstID] = VDEC_MPEG4;
      _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0);
      //vVDecOutputDebugString("\n input window before parsing = 0x%08x", _u4Datain[_u4VDecID]);
      while(!fgVPrsValidStartCode(u4InstID, TRUE, &_u4Datain[u4InstID]))
      {
        _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 8);
      }
      
      if (GROUP_OF_VOP_START_CODE == _u4Datain[u4InstID])
      {
        dwRetVal = u4M4VParser(u4InstID, fgInquiry);
      }
      else if (VISUAL_OBJECT_START_CODE == _u4Datain[u4InstID])
      {
        dwRetVal = u4M4VParser(u4InstID, fgInquiry);
      }
      else if (VOP_START_CODE == _u4Datain[u4InstID])
      {
        dwRetVal = u4M4VParser(u4InstID, fgInquiry);
      }
      else if ((_u4Datain[u4InstID] >= VIDEO_OBJECT_LAYER_START_CODE_MIN) &&
                  (_u4Datain[u4InstID] <= VIDEO_OBJECT_LAYER_START_CODE_MAX))
      {
        dwRetVal = u4M4VParser(u4InstID, fgInquiry);  
      }
      else //short header
      {
        if(!fgInquiry)
        dwRetVal = u4ShVParser(u4InstID, fgInquiry);
      }
      break;
    case VDEC_DIVX3:
      _ucMpegVer[u4InstID] = VDEC_DIVX3;
      _u4Divx3SetPos[u4InstID] = _u4Divx3SetPos[u4InstID] + 4 + u4VDEC_HAL_MPEG_GetBitStreamShift(_u4BSID[u4InstID], u4InstID, 32);
      dwRetVal = u4Dx3VParser(u4InstID, fgInquiry);
      break;
  }
#if 0
  switch(u4VFifo_GetPicVType(u4VFifo_GetPicReadIdx()))
  {
    case SH_I_VOP:
    case SH_P_VOP:
      _ucMpegVer[u4InstID] = MPEG4;
      dwRetVal = u4ShVParser(fgInquiry);
      break;
    case DX3_I_FRM:
    case DX3_P_FRM:
      _ucMpegVer[u4InstID] = DIVX3;
      dwRetVal = u4Dx3VParser(fgInquiry);
      break;
    default:
      _ucMpegVer[u4InstID] = MPEG4;
      dwRetVal = u4M4VParser(fgInquiry);
      break;
  }
#endif

  if(!fgInquiry)
  {
    vVerifySetMpeg2Var(u4InstID);
    if(_ucVopCdTp[u4InstID] == VCT_B)
    {
      _fgVerPrevBPic[u4InstID] = TRUE;
    }
    else
    {
      _fgVerPrevBPic[u4InstID] = FALSE;
      _ucPrevVopCdTp[u4InstID] = _ucVopCdTp[u4InstID];
    }
  }
  else
  {
    if(!fgVerifyMpvPicDimChk(u4InstID, _u4HSize[u4InstID], _u4VSize[u4InstID], TRUE))
    {
      return(PIC_DIM_ERR);
    }
  }

  if(dwRetVal != 0)
  {
    return(dwRetVal);
  }

  vVerifyMpvSetSeqLayerDecPrm(u4InstID);
  vVerifyMp4SetDecPrm(u4InstID);
  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.u4BBufStart = 0;

  return(0);
}

void vVerifySetMpeg2Var(UINT32 u4InstID)
{
  _u4PicCdTp[u4InstID] = u4Vop2Pic(_ucVopCdTp[u4InstID]);
  _u4PicStruct[u4InstID] = FRM_PIC;
  if((!_fgVerShortVideoHeader[u4InstID] && _fgVerInterlaced[u4InstID]) ||
     (_fgVerShortVideoHeader[u4InstID] && !_fgVerDocuCamera[u4InstID]))
  {
    // in mpeg2 term:
    // frame picture, progressive_frame = 0,
    _fgVerProgressiveSeq[u4InstID] = FALSE;
    _fgVerProgressiveFrm[u4InstID] = FALSE;
    // _fgVerTopFldFirst is in the syntax
    _fgVerRepFirstFld[u4InstID] = FALSE;
  }
  else
  {
    _fgVerProgressiveSeq[u4InstID] = TRUE;
    _fgVerProgressiveFrm[u4InstID] = TRUE;
    _fgVerTopFldFirst[u4InstID] = FALSE;
    _fgVerRepFirstFld[u4InstID] = FALSE;
  }
}

UINT32 u4Vop2Pic(BYTE ucVopCdTp)
{
  switch(ucVopCdTp)
  {
    case VCT_I:
      return(I_TYPE);
    case VCT_P:
    case VCT_S:
      return(P_TYPE);
    case VCT_B:
      return(B_TYPE);
  }
  return(0);
}

/* Table 6-25 */
UINT32 u4SrcFmtTbl(UINT32 u4InstID, BYTE bShSrcFmt)
{
  UINT32 dwWTbl[5] = {128, 176, 352, 704, 1408};
  UINT32 dwHTbl[5] = {96, 144, 288, 576, 1152};

  _u4FrmRatCod[u4InstID] = 4;//FRC_29_97;
  if((bShSrcFmt >= 1) && (bShSrcFmt <= 5))
  {
    _u4HSize[u4InstID] = dwWTbl[bShSrcFmt - 1];
    _u4VSize[u4InstID] = dwHTbl[bShSrcFmt - 1];
    _u4RealHSize[u4InstID] = _u4HSize[u4InstID];
    _u4RealVSize[u4InstID] = _u4VSize[u4InstID];
  }
  else
  {
    return(SRC_FMT_ERR);
  }
  return(0);
}

void vVerifySetShDefaultParm(UINT32 u4InstID)
{
  _fgVerObmcDisable[u4InstID] = TRUE;
  _fgVerQuantType[u4InstID] = FALSE;
  _fgVerResyncMarkerDisable[u4InstID] = TRUE;
  _fgVerDataPartitioned[u4InstID] = FALSE;
  _fgVerReversibleVlc[u4InstID] = FALSE;
  _ucVopRoundingType[u4InstID] = 0;
  _ucFordFCode[u4InstID] = 1;
  _fgVerInterlaced[u4InstID] = FALSE;

  _fgVerDocuCamera[u4InstID] = FALSE;  // short header
}

UINT32 u4ShVParser(UINT32 u4InstID, BOOL fgInquiry)
{
  //BYTE bTemporalReference;
  UINT32 dwRetVal;

  //PANDA H263
  if(_u4CodecVer[u4InstID] == VDEC_H263)
  {
      BYTE bPictureSize;
      _fgVerShortVideoHeader[u4InstID] = TRUE;
      _fgSorenson[u4InstID] = TRUE;

      _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);
      //bTemporalReference = (_u4Datain[u4InstID] >> 3) & 0xff;
      bPictureSize = _u4Datain[u4InstID] & 0x07;
      switch(bPictureSize)
      {
          case 0:
            _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 16);
            _u4HSize[u4InstID] = (_u4Datain[u4InstID] & 0x0000FF00) >> 8;
            _u4VSize[u4InstID] = (_u4Datain[u4InstID] & 0x000000FF);
            break;
          case 1:
            _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 32);
            _u4HSize[u4InstID] = (_u4Datain[u4InstID] & 0xFFFF0000) >> 16;
            _u4VSize[u4InstID] = (_u4Datain[u4InstID] & 0x0000FFFF);
            break;
          case 2:
            _u4HSize[u4InstID] = 352;
            _u4VSize[u4InstID] = 288;
            break;
          case 3:
            _u4HSize[u4InstID] = 176;
            _u4VSize[u4InstID] = 144;
            break;
          case 4:
            _u4HSize[u4InstID] = 128;
            _u4VSize[u4InstID] = 96;
            break;
          case 5:
            _u4HSize[u4InstID] = 320;
            _u4VSize[u4InstID] = 240;
            break;
          case 6:
            _u4HSize[u4InstID] = 160;
            _u4VSize[u4InstID] = 120;
            break;
          default:
            return(SRC_FMT_ERR);
      }
      _u4RealHSize[u4InstID] = _u4HSize[u4InstID];
      _u4RealVSize[u4InstID] = _u4VSize[u4InstID];

      _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 9);
      
      _fgVerDocuCamera[u4InstID] = 0;
      _ucSourceFormat[u4InstID] = 0;
      _fgVerTopFldFirst[u4InstID] = TRUE;

      
      _ucVopCdTp[u4InstID] = (_u4Datain[u4InstID] >> 7) & 0x3;
	  if(_ucVopCdTp[u4InstID] == 2 || _ucVopCdTp[u4InstID] == 3)
	  {
	  	_ucVopCdTp[u4InstID] = 1;
	  }
      _ucVopQuant[u4InstID] = (_u4Datain[u4InstID] >> 1) & 0x1f;
      while(_u4Datain[u4InstID] & 0x1)  // pei
      {
        _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 9);
      }

      if(fgInquiry)
      {
        return(0);
      }

      vVerifySetShDefaultParm(u4InstID);
      vVerifyM4vSetVolPrm(u4InstID);
      u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 32); // move VLD to picture data beginning
      if((dwRetVal = u4VopData()) != 0)
      {
        return(dwRetVal);
      }
      vVerifyM4vSetVopPrm(u4InstID);
  }
  else
  {
  _fgVerShortVideoHeader[u4InstID] = TRUE;

  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0);
  //bTemporalReference = (_u4Datain[u4InstID] >> 2) & 0xff;
  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 18);
  _fgVerDocuCamera[u4InstID] = (_u4Datain[u4InstID] >> 16) & 0x1;
  if(_fgVerDocuCamera[u4InstID])  // video
  {
    _fgVerTopFldFirst[u4InstID] = TRUE;  // always set to TRUE.
  }
  _ucSourceFormat[u4InstID] = (_u4Datain[u4InstID] >> 12) & 0x7;
  if((dwRetVal = u4SrcFmtTbl(u4InstID, _ucSourceFormat[u4InstID])) != 0)
  {
    return(dwRetVal);
  }
  _ucVopCdTp[u4InstID] = (_u4Datain[u4InstID] >> 11) & 0x1;
  _ucVopQuant[u4InstID] = (_u4Datain[u4InstID] >> 2) & 0x1f;
  while(_u4Datain[u4InstID] & 0x1)  // pei
  {
    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 9);
  }

  if(fgInquiry)
  {
    return(0);
  }

  vVerifySetShDefaultParm(u4InstID);
  vVerifyM4vSetVolPrm(u4InstID);
  u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 32); // move VLD to picture data beginning
  if((dwRetVal = u4VopData()) != 0)
  {
    return(dwRetVal);
  }
  vVerifyM4vSetVopPrm(u4InstID);
  }
  return(0);
}

void vVerifyCalGmcMv(UINT32 u4InstID)
{
  VDEC_INFO_MPEG_GMC_PRM_T *ptGmcPrm = &_rMPEG4GmcPrm[u4InstID];
  int dx, dy;
  int s;
  BOOL fgBadDivX = ((_u4UserDataCodecVersion[u4InstID] == 500) ||
                    (_u4UserDataCodecVersion[u4InstID] == 501)) &&
                   (_u4UserDataBuildNumber[u4InstID] >= 370) &&
                   (_u4UserDataBuildNumber[u4InstID] <= 413);
  long i0_pla, j0_pla;

  if(fgPureIsoM4v())
  {
    fgBadDivX = FALSE;
  }

  s = 2 << _ucSpriteWarpingAccuracy[u4InstID];
  if(fgBadDivX)
  {
    dx = _ppiWarpingMv[u4InstID][0][0];  // shouldn't we add (*s/16) ?
    dy = _ppiWarpingMv[u4InstID][0][1];  // shouldn't we add (*s/16) ?
  }
  else
  {
    dx = _ppiWarpingMv[u4InstID][0][0] * s / 2;
    dy = _ppiWarpingMv[u4InstID][0][1] * s / 2;
  }

  i0_pla = dx * 4 / s;  /* multiply by two to get q-pel precision */
  j0_pla = dy * 4 / s;  /* multiply by two to get q-pel precision */
  //j = (i0_pla << 16) + j0_pla;
  ptGmcPrm->i4GmcYMvX = i0_pla;
  ptGmcPrm->i4GmcYMvY = j0_pla;

  i0_pla = (dx >> 1) | (dx & 0x1);
  i0_pla = i0_pla * 4 / s;   /* multiply by two to get q-pel precision */
  j0_pla = (dy >> 1) | (dy & 0x1);
  j0_pla = j0_pla * 4 / s;   /* multiply by two to get q-pel precision */
  //j = (i0_pla << 16) + j0_pla;
  ptGmcPrm->i4GmcCMvX = i0_pla;
  ptGmcPrm->i4GmcCMvY = j0_pla;

}

void vVerifyM4vSetGmcPrm(UINT32 u4InstID)
{
  VDEC_INFO_MPEG_GMC_PRM_T *ptGmcPrm = &_rMPEG4GmcPrm[u4InstID];

  // _ucEffectiveWarpingPoints was set to 0 if it's not S-VOP
  ptGmcPrm->ucEffectiveWarpingPoints = _ucEffectiveWarpingPoints[u4InstID];
  if(_ucEffectiveWarpingPoints[u4InstID] == 0)
  {
    ptGmcPrm->i4GmcYMvX = 0;
    ptGmcPrm->i4GmcYMvY = 0;
    ptGmcPrm->i4GmcCMvX = 0;
    ptGmcPrm->i4GmcCMvY = 0;
  }
  else
  {
    vVerifyCalGmcMv(u4InstID);
  }
}

void vVerifyM4vSetVopPrm(UINT32 u4InstID)
{
  VDEC_INFO_MPEG4_VOP_PRM_T *ptVopPrm = &_rMPEG4VopPrm[u4InstID];
  VDEC_INFO_MPEG_DIR_MODE_T *ptDirMdPrm = ptVopPrm->prDirMd;

  ptVopPrm->ucIntraDcVlcThr = _ucIntraDcVlcThr[u4InstID];
  ptVopPrm->fgAlternateVerticalScanFlag = BYTE0(_fgVerAlternateVerticalScanFlag[u4InstID]);
  ptVopPrm->fgTopFldFirst = BYTE0(_fgVerTopFldFirst[u4InstID]);
  ptVopPrm->ucFordFCode = _ucFordFCode[u4InstID];
  ptVopPrm->ucBackFCode = _ucBackFCode[u4InstID];
  ptVopPrm->ucBRefCdTp = _ucPrevVopCdTp[u4InstID];

  ptDirMdPrm->u4Trd = _rDirMode[u4InstID].u4Trd;
  ptDirMdPrm->u4Trb = _rDirMode[u4InstID].u4Trb;
  ptDirMdPrm->u4Trdi = _rDirMode[u4InstID].u4Trdi;
  ptDirMdPrm->u4Trbi = _rDirMode[u4InstID].u4Trbi;

  vVerifyM4vSetGmcPrm(u4InstID);
}

void vVerifyM4vSetVolPrm(UINT32 u4InstID)
{
  VDEC_INFO_MPEG4_VOL_PRM_T *ptVolPrm = &_rMPEG4VolPrm[u4InstID];

  ptVolPrm->ucShortVideoHeader = BYTE0(_fgVerShortVideoHeader[u4InstID]);
  ptVolPrm->ucSorenson = BYTE0(_fgSorenson[u4InstID]);
  ptVolPrm->u2VopTimeIncrementResolution = _u2VopTimeIncrementResolution[u4InstID];
  ptVolPrm->ucInterlaced = BYTE0(_fgVerInterlaced[u4InstID]);
  ptVolPrm->ucQuantType = BYTE0(_fgVerQuantType[u4InstID]);
  ptVolPrm->ucQuarterSample = BYTE0(_fgVerQuarterSample[u4InstID]);
  ptVolPrm->ucResyncMarkerDisable = BYTE0(_fgVerResyncMarkerDisable[u4InstID]);
  ptVolPrm->ucDataPartitioned = BYTE0(_fgVerDataPartitioned[u4InstID]);
  ptVolPrm->ucReversibleVlc = BYTE0(_fgVerReversibleVlc[u4InstID]);
  ptVolPrm->ucSourceFormat = _ucSourceFormat[u4InstID];
}

void vVerifyGetUpParm(UINT32 u4InstID)
{
  if(_u4UPicW[u4InstID] != 0)
  {
    _u4HSize[u4InstID] = _u4UPicW[u4InstID];
  }
  if(_u4UPicH[u4InstID] != 0)
  {
    _u4VSize[u4InstID] = _u4UPicH[u4InstID];
  }
  if(_u4UFrmRatCod[u4InstID] != 0)
  {
    _u4FrmRatCod[u4InstID] = _u4UFrmRatCod[u4InstID];
  }
  if(_fgVerUDx4M4v[u4InstID])
  {
    _u4UserDataCodecVersion[u4InstID] = 412;
  }
}

// MPEG4/DivX3 common parameters
void vVerifyMp4SetDecPrm(UINT32 u4InstID)
{
  VDEC_INFO_MPEG4_DEC_PRM_T *ptMp4Prm = &_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm;
  bool bDec14496Flag = TRUE;

  ptMp4Prm->ucVopCdTp = _ucVopCdTp[u4InstID];
  ptMp4Prm->ucVopQuant = _ucVopQuant[u4InstID];
  ptMp4Prm->ucVopRoundingType = _ucVopRoundingType[u4InstID];
  ptMp4Prm->u4CMvType = 0x00;
  if((_u4UserDataCodecVersion[u4InstID] < 500) &&
     (_u4UserDataCodecVersion[u4InstID] != 0) &&
     (_u4UserDataCodecVersion[u4InstID] != 311) &&
     _fgVerUDivXM4v[u4InstID] &&
     !fgPureIsoM4v())
  {
    ptMp4Prm->u4UmvPicW = _u4HSize[u4InstID];
    ptMp4Prm->u4UmvPicH = _u4VSize[u4InstID];
  }
  else
  {
    ptMp4Prm->u4UmvPicW = ((_u4HSize[u4InstID] + 15) / 16) * 16;
    ptMp4Prm->u4UmvPicH = ((_u4VSize[u4InstID] + 15) / 16) * 16;
  }

  if((_u4UserDataCodecVersion[u4InstID] > 0) &&
     (_u4UserDataCodecVersion[u4InstID] < 503) &&
//     _fgVerUDivXM4v[u4InstID] &&
     !fgPureIsoM4v())
  {
    ptMp4Prm->u4QPelType = TYPE_14496;
  }
  else
  {
    ptMp4Prm->u4QPelType = TYPE_MOMUSYS;
  }

  if(fgPureIsoM4v())
  {
    ptMp4Prm->u4CMvType = QPEL_4MV_CMV_14496;
    ptMp4Prm->u4CMvType |= QPEL_FRAME_CMV_14496;
    ptMp4Prm->u4CMvType |= QPEL_FIELD_CMV_14496;
  }
  else
  {
    #if 0
    ptMp4Prm->u4CMvType = QPEL_4MV_CMV_14496;  // Follow DivX source code
    if(_ucVopCdTp[u4InstID] != VCT_B)
    {
      if(_u4CodecVer[u4InstID] == VDEC_DIVX3)
      {
      	ptMp4Prm->u4CMvType |= QPEL_FRAME_CMV_DIVXOLD;
		ptMp4Prm->u4CMvType = (ptMp4Prm->u4CMvType & 0xfffffffe) | QPEL_4MV_CMV_14496;
      }
	  else
      if((_u4UserDataCodecVersion[u4InstID] == 0) || (_u4UserDataCodecVersion[u4InstID] >= 503))
      {
        //ptMp4Prm->u4CMvType |= QPEL_FRAME_CMV_DIVX503;
        ptMp4Prm->u4CMvType |= QPEL_FRAME_CMV_14496;
      }
      else
      {
        ptMp4Prm->u4CMvType |= QPEL_FRAME_CMV_DIVXOLD;
      }
	  if(_u4CodecVer[u4InstID] == VDEC_DIVX3)
	  {
		ptMp4Prm->u4CMvType |= QPEL_FIELD_CMV_14496;
	  }
	  else
	  {
        ptMp4Prm->u4CMvType |= QPEL_FIELD_CMV_DIVX503;
        // Cheng-Jung 2012/07/10 Added per designer's notes - only for MPEG4 mode!!!
        if (ptMp4Prm->rDep.rM4vDecPrm.prVol->ucQuarterSample)
        {
            ptMp4Prm->u4CMvType |= QPEL_FIELD_CMV_14496;
        }
      }
    }
    else
    {
      if((_u4UserDataCodecVersion[u4InstID] == 0) || (_u4UserDataCodecVersion[u4InstID] >= 500))
      {
      	ptMp4Prm->u4CMvType |= QPEL_FRAME_CMV_14496;
      	//ptMp4Prm->u4CMvType |= QPEL_FRAME_CMV_DIVXOLD;
      }
      else
      {
        ptMp4Prm->u4CMvType |= QPEL_FRAME_CMV_DIVXOLD;
	  }
      //ptMp4Prm->u4CMvType |= QPEL_FRAME_CMV_DIVXOLD;
      ptMp4Prm->u4CMvType |= QPEL_FIELD_CMV_14496;
	  if(_u4CodecVer[u4InstID] == VDEC_DIVX3)
	  {
	  	ptMp4Prm->u4CMvType |= QPEL_FRAME_CMV_DIVXOLD;
		ptMp4Prm->u4CMvType |= QPEL_4MV_CMV_14496;
        ptMp4Prm->u4CMvType |= QPEL_FIELD_CMV_14496;
      }
    }
    #else
    ptMp4Prm->u4CMvType = QPEL_4MV_CMV_14496;
    if (_ucVopCdTp[u4InstID] != VCT_B)
    {
      if(_u4CodecVer[u4InstID] == VDEC_DIVX3)
      {
        ptMp4Prm->u4CMvType |= QPEL_FRAME_CMV_DIVXOLD;
        ptMp4Prm->u4CMvType = (ptMp4Prm->u4CMvType & 0xfffffffe) | QPEL_4MV_CMV_14496;
      }
      else if (400 <= _u4UserDataCodecVersion[u4InstID] && 
               503 > _u4UserDataCodecVersion[u4InstID]) 
      {
        ptMp4Prm->u4CMvType |= QPEL_FRAME_CMV_DIVXOLD;
      }
      else 
      {
        ptMp4Prm->u4CMvType |= QPEL_FRAME_CMV_14496;
      }

      if(_u4CodecVer[u4InstID] == VDEC_DIVX3)
	  {
		ptMp4Prm->u4CMvType |= QPEL_FIELD_CMV_14496;
	  } 
      else if ((ptMp4Prm->rDep.rM4vDecPrm.prVol->ucQuarterSample == 1) && bDec14496Flag) 
	  {
        ptMp4Prm->u4CMvType |= QPEL_FIELD_CMV_14496;
      } 
      else 
      {
        ptMp4Prm->u4CMvType |= QPEL_FIELD_CMV_DIVX503;
      }
    }
    else
    {
      if (ptMp4Prm->rDep.rM4vDecPrm.prVol->ucQuarterSample == 1) {
        ptMp4Prm->u4CMvType |= QPEL_FRAME_CMV_14496;
      } else {
        ptMp4Prm->u4CMvType |= QPEL_FRAME_CMV_DIVXOLD;
      }    
      ptMp4Prm->u4CMvType |= QPEL_FIELD_CMV_14496;
	  if(_u4CodecVer[u4InstID] == VDEC_DIVX3)
	  {
	  	ptMp4Prm->u4CMvType |= QPEL_FRAME_CMV_DIVXOLD;
		ptMp4Prm->u4CMvType |= QPEL_4MV_CMV_14496;
        ptMp4Prm->u4CMvType |= QPEL_FIELD_CMV_14496;
      }
    }
    //6582 MPEG4 mode sync C-model fix
    if (bDec14496Flag) ptMp4Prm->u4CMvType = QPEL_4MV_CMV_14496;
    #endif
  }
  //printk("<vdec>ptMp4Prm->u4CMvType = 0x%x,ptMp4Prm->u4QPelType = 0x%x,codetype = %d\n",ptMp4Prm->u4CMvType,ptMp4Prm->u4QPelType,_ucVopCdTp[u4InstID]);
}

UINT32 u4VopData(void)
{
  return(0);
}

UINT32 u4Dx3VParser(UINT32 u4InstID, BOOL fgInquiry)
{
  UINT32 dwRetVal;

  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0);
  dwRetVal = _u4Datain[u4InstID] >> 30;
  _ucVopCdTp[u4InstID] = dwRetVal & 0x1;
  if(dwRetVal & 0x2)
  {
    _u4SkipFrameCnt[u4InstID]++;
    _fgVerVopCoded0[u4InstID] = TRUE;
    return(VOP_CODED_0);
  }

  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 2);
  _ucVopQuant[u4InstID] = (_u4Datain[u4InstID] >> 27) & 0x1f;
  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 5);
  if(_ucVopCdTp[u4InstID] == VCT_I)
  {
    _ucVopRoundingType[u4InstID] = 1;
    _ucFrameMode[u4InstID] = (_u4Datain[u4InstID] >> 27) & 0x1f;
    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 5);
    _fgVerAltIAcChromDct[u4InstID] = (_u4Datain[u4InstID] >> 31) & 0x1;
    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);
    if(_fgVerAltIAcChromDct[u4InstID])
    {
      _fgVerAltIAcChromDctIdx[u4InstID] = (_u4Datain[u4InstID] >> 31) & 0x1;
      _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);
    }
    else
    {
      _fgVerAltIAcChromDctIdx[u4InstID] = 0;
    }
    _fgVerAltIAcLumDct[u4InstID] = (_u4Datain[u4InstID] >> 31) & 0x1;
    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);
    if(_fgVerAltIAcLumDct[u4InstID])
    {
      _fgVerAltIAcLumDctIdx[u4InstID] = (_u4Datain[u4InstID] >> 31) & 0x1;
      _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);
    }
    else
    {
      _fgVerAltIAcLumDctIdx[u4InstID] = 0;
    }
    _fgVerAltIDcDct[u4InstID] = (_u4Datain[u4InstID] >> 31) & 0x1;
    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);
    // 050131: has_skip and alternative_P_AC_DCT_index should be 0 in I-VOP,
    //         or the I will be wrongly decoded.
    _fgVerHasSkip[u4InstID] = 0;
    _fgVerAltPAcDct[u4InstID] = 0;
    _fgVerAltPAcDctIdx[u4InstID] = 0;
    _fgVerAltPDcDct[u4InstID] = 0;
    _fgVerAltMv[u4InstID] = 0;
  }
  else
  {
    if(_fgVerSwitchRounding[u4InstID])
    {
      _ucVopRoundingType[u4InstID] = 1 - _ucVopRoundingType[u4InstID];
    }
    else
    {
      _ucVopRoundingType[u4InstID] = FALSE;
    }
    _fgVerHasSkip[u4InstID] = (_u4Datain[u4InstID] >> 31) & 0x1;
    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);
    _fgVerAltPAcDct[u4InstID] = (_u4Datain[u4InstID] >> 31) & 0x1;
    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);
    if(_fgVerAltPAcDct[u4InstID])
    {
      _fgVerAltPAcDctIdx[u4InstID] = (_u4Datain[u4InstID] >> 31) & 0x1;
      _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);
    }
    else
    {
      _fgVerAltPAcDctIdx[u4InstID] = 0;
    }
    _fgVerAltPDcDct[u4InstID] = (_u4Datain[u4InstID] >> 31) & 0x1;
    _fgVerAltMv[u4InstID] = (_u4Datain[u4InstID] >> 30) & 0x1;
    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 2);
  }

  if(fgInquiry)
  {
    return(0);
  }

  if((dwRetVal = u4VopData()) != 0)
  {
    return(dwRetVal);
  }
  vVerifyDx3SetPicLayerPrm(u4InstID);
  return(0);
}

void vVerifyDx3SetPicLayerPrm(UINT32 u4InstID)
{
  VDEC_INFO_DIVX3_PIC_PRM_T *ptDx3Prm = &_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rDep.rDx3DecPrm;

  ptDx3Prm->ucAltIAcChromDct = BYTE0(_fgVerAltIAcChromDct[u4InstID]);
  ptDx3Prm->ucAltIAcChromDctIdx = BYTE0(_fgVerAltIAcChromDctIdx[u4InstID]);
  ptDx3Prm->ucAltIAcLumDct = BYTE0(_fgVerAltIAcLumDct[u4InstID]);
  ptDx3Prm->ucAltIAcLumDctIdx = BYTE0(_fgVerAltIAcLumDctIdx[u4InstID]);
  ptDx3Prm->ucAltIDcDct = BYTE0(_fgVerAltIDcDct[u4InstID]);
  ptDx3Prm->ucHasSkip = BYTE0(_fgVerHasSkip[u4InstID]);
  ptDx3Prm->ucAltPAcDct = BYTE0(_fgVerAltPAcDct[u4InstID]);
  ptDx3Prm->ucAltPAcDctIdx = BYTE0(_fgVerAltPAcDctIdx[u4InstID]);
  ptDx3Prm->ucAltPDcDct = BYTE0(_fgVerAltPDcDct[u4InstID]);
  ptDx3Prm->ucAltMv = BYTE0(_fgVerAltMv[u4InstID]);
  ptDx3Prm->ucFrameMode = _ucFrameMode[u4InstID];
}

UINT32 u4M4VParser(UINT32 u4InstID, BOOL fgInquiry)
{
  UINT32 dwRetVal;
  UINT32 dwRetry = 0;
#ifdef VDEC_MPEG4_SUPPORT_RESYNC_MARKER        
    //VDEC_INFO_MPEG_VFIFO_PRM_T rMpegVFifoInitPrm;
    VDEC_INFO_MPEG_BS_INIT_PRM_T rMpegBSInitPrm;
    UINT32 u4BytePos, u4BitsPos;
    UINT32 u4MbW, u4MbH;
    UINT32 u4ResyncMarkerZeroLength = 16;
    UINT32 u4ResyncMarkerCnt = 0;
    UINT32 u4ResyncPattern = 0;
    UINT32 u4ResyncMask = 0;
    //UINT32 u4MbNumLength = 13;
    UINT32 u4MbNum = 0;
    UINT32 u4MbLength = 13;
    
    _u4ResyncMarkMbx[u4InstID][0] = 0;    
    _u4ResyncMarkMby[u4InstID][0] = 0;
    _u4ResyncMarkMbx[u4InstID][1] = 0;
    _u4ResyncMarkMby[u4InstID][1] = 0;
    _u4ResyncMarkerCnt[u4InstID] = 0;
#endif

  do
  {
    if(fgMPEGNextStartCode(u4InstID) == FALSE)
    {
      return(NO_START_C_ERR1);
    }
    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0);
    if(!fgMPEG4ValidStartCode(_u4Datain[u4InstID]))
    {
      u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 8);
    }
    if(++dwRetry > MAX_VER_RETRY_COUNT)
    {
      return(NO_START_C_ERR2);
    }
  } while(!fgMPEG4ValidStartCode(_u4Datain[u4InstID]));

  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0);

  if(_u4Datain[u4InstID] == VISUAL_OBJECT_START_CODE)
  {
    if((dwRetVal = u4VisualObject(u4InstID)) != 0)
    {
      return(dwRetVal);
    }
  }

  while((_u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0)) == USER_DATA_START_CODE)
  {
    vVerifyUserData(u4InstID);
  }

/*
  _u4Datain = dwGetBitStream(0);
  if(_u4Datain == USER_DATA_START_CODE)
  {
    dwUserData();
  }
*/
  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0);
  if((_u4Datain[u4InstID] >= VIDEO_OBJECT_START_CODE_MIN) &&
     (_u4Datain[u4InstID] <= VIDEO_OBJECT_START_CODE_MAX))
  {
    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 32); // skip video_object_start_code
  }

  if((_u4Datain[u4InstID] >= VIDEO_OBJECT_LAYER_START_CODE_MIN) &&
     (_u4Datain[u4InstID] <= VIDEO_OBJECT_LAYER_START_CODE_MAX))
  {
    _fgVerShortVideoHeader[u4InstID] = FALSE;
    if((dwRetVal = u4NormalVOL(u4InstID)) != 0)
    {
      return(dwRetVal);
    }
  }
#if H263_USE_SORENSON  
  else if((_u4Datain[u4InstID] & H263_VIDEO_START_MASK) == H263_VIDEO_START_MARKER) // Sorenson
#else  
  else if((_u4Datain[u4InstID] & SHORT_VIDEO_START_MASK) == SHORT_VIDEO_START_MARKER) // H.263
#endif
  {
    if((dwRetVal = u4ShVParser(u4InstID, fgInquiry)) != 0)
    {
      return(dwRetVal);
    }
  }

  if(fgInquiry)
  {
    return(0);
  }

  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0);
  if(_u4Datain[u4InstID] == GROUP_OF_VOP_START_CODE)
  {
    if((dwRetVal = u4GoVop(u4InstID)) != 0)
    {
      return(dwRetVal);
    }
  }
  // 040531: There's a user data between GoVop and VOP in
  //         B_B_K's 叢林奇兵.avi.
  while((_u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0)) == USER_DATA_START_CODE)
  {
    vVerifyUserData(u4InstID);
  }
  if((dwRetVal = u4VOP(u4InstID)) != 0)
  {
    return(dwRetVal);
  }

  // reset HW
#ifdef VDEC_MPEG4_SUPPORT_RESYNC_MARKER        

  u4MbW = ((_u4HSize[u4InstID] + 15) / 16);
  u4MbH  = ((_u4VSize[u4InstID] + 15) / 16);
  
  if (_fgVerResyncMarkerDisable[u4InstID] == 0 && (u4MbW * u4MbH) > 4096)
  {    
    //Keep barrel shifter address.
    if(u4MbW * u4MbH > 8193)
    {
        u4MbLength = 14;
    }
    u4BytePos = u4VDEC_HAL_MPEG_ReadRdPtr(_u4BSID[u4InstID], u4InstID, (UINT32)_pucVFifo[u4InstID], &u4BitsPos);

    //Search resync marker bits.
    //1. Step go to byte align.
    u4VDEC_HAL_MPEG_ShiftGetBitStream(0, u4InstID, (8-u4BitsPos));
    
    //2. Decide resync marker bit length.
    if(_ucVopCdTp[u4InstID] == VCT_I)
    {
       u4ResyncMarkerZeroLength = 16;
    }
    else
    if(_ucVopCdTp[u4InstID] == VCT_P)
    {
       u4ResyncMarkerZeroLength = 15 + _ucFordFCode[u4InstID];
    }
    else
    if(_ucVopCdTp[u4InstID] == VCT_B)
    {
       u4ResyncMarkerZeroLength = 17;
       
       if (15 + _ucFordFCode[u4InstID] > 17)
       {
           u4ResyncMarkerZeroLength = 15 + _ucFordFCode[u4InstID];
           
           if (_ucBackFCode[u4InstID] > _ucFordFCode[u4InstID])
           {
               u4ResyncMarkerZeroLength = 15 + _ucBackFCode[u4InstID];
           }
       }
       else
       if (15 + _ucBackFCode[u4InstID] > 17)
       {
          u4ResyncMarkerZeroLength = 15 + _ucBackFCode[u4InstID];
       }       
    }
    
    //3. Resync marker pattern
    u4ResyncPattern = (1 << (31 - u4ResyncMarkerZeroLength));
    INT32 i4Idx;
    for (i4Idx = 31; i4Idx >= (31 - u4ResyncMarkerZeroLength); i4Idx--)
    {
        u4ResyncMask |= (1<<i4Idx);
    }
    
    //u4ResyncMask = ((0xFFFFFFFF) >> (31 - u4ResyncMarkerZeroLength)) << ((31 - u4ResyncMarkerZeroLength));
    //vVDecOutputDebugString("ResyncPattern = 0x%x, ResyncMask = 0x%x !!! \n", u4ResyncPattern, u4ResyncMask);
    
    //4. Search resync marker bits
    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0);
    while (!fgMPEG4ValidStartCode(_u4Datain[u4InstID]))
    {      
      if ((_u4Datain[u4InstID] &  u4ResyncMask) == u4ResyncPattern)
      {
         if (u4ResyncMarkerCnt >= 2)
         {
             return(NO_START_C_ERR1);
         }
         
          _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, (u4ResyncMarkerZeroLength+1));
          if(u4MbLength == 14)
          {
              u4MbNum = ((_u4Datain[u4InstID] & 0xFFFC0000) >> 18);
          }
          else
          {
          u4MbNum = ((_u4Datain[u4InstID] & 0xFFF80000) >> 19);          
          }
          _u4ResyncMarkMby[u4InstID][u4ResyncMarkerCnt] = u4MbNum / u4MbW;
          _u4ResyncMarkMbx[u4InstID][u4ResyncMarkerCnt] = (u4MbNum - (_u4ResyncMarkMby[u4InstID][u4ResyncMarkerCnt] * u4MbW));
          //vVDecOutputDebugString("ResyncMbx = 0x%x, ResyncMby = 0x%x !!! \n", _u4ResyncMarkMbx[u4InstID][u4ResyncMarkerCnt], _u4ResyncMarkMby[u4InstID][u4ResyncMarkerCnt]);
          u4ResyncMarkerCnt++;

          //consume 13 bits
          _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, (u4MbLength));

          //goto byte align
          _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, (8 - ((u4ResyncMarkerZeroLength+1+13) & 0x7)));
      }
      else
      {
          _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 8);
      }
    }
    
    //5. Set marker count number
    _u4ResyncMarkerCnt[u4InstID] = u4ResyncMarkerCnt;
    
    //6.
    //Reset barrel shifter.    
    //rMpegVFifoInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
    //rMpegVFifoInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
    //i4VDEC_HAL_MPEG_InitVDecHW(u4InstID,&rMpegVFifoInitPrm);
    rMpegBSInitPrm.u4VFifoSa =  (UINT32)_pucVFifo[u4InstID];
    rMpegBSInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
    rMpegBSInitPrm.u4ReadPointer =  (UINT32)_pucVFifo[u4InstID] + u4BytePos;
  #ifndef  RING_VFIFO_SUPPORT
    rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
  #else
  //		rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ*(0.5 + 0.5 *(_u4LoadBitstreamCnt[u4InstID]%2)));
	  rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));//mtk40343
  #endif
    i4VDEC_HAL_MPEG_InitBarrelShifter(0, u4InstID, &rMpegBSInitPrm);
    u4VDEC_HAL_MPEG_ShiftGetBitStream(0, u4InstID, u4BitsPos);
  }
#endif

  return(dwRetVal);
}



UINT32 u4NormalVOL(UINT32 u4InstID)
{
  int i;
  BOOL fgVolControlPar;

  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 10);
  if(_u4Datain[u4InstID] & 0x1) // is_object_layer_identifier
  {
    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 7);
    _ucVideoObjectLayerVerid[u4InstID] = (_u4Datain[u4InstID] >> 3) & 0xf;
  }
  else
  {
    _ucVideoObjectLayerVerid[u4InstID] = _ucVisualObjectVerid[u4InstID];
  }

  _ucAspRatInf[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 4) & 0xf;
  vVerifyGetPxlAspRat(u4InstID, _ucAspRatInf[u4InstID]);
/*
  if(_ucAspRatInf == 0xf)
  {
    dwGetBitStream(16); // par_width, par_height
  }
*/
  fgVolControlPar = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1) & 0x1;
  if(fgVolControlPar)
  {
    if(u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 4) & 0x1) // vbv_parameters
    {
      u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 32); // first_half_bit_rate, marker_bit, latter_half_bit_rate, marker_bit
      u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 19); // first_half_vbv_buffer_size, marker_bit, latter_half_vbv_buffer_size
      u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 28); // first_half_vbv_occupancy, marker_bit, latter_half_vbv_occupancy, marker_bit
    }
  }
  if((u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 2) & 0x3) != 0)
  {
    return(UNKNOWN_SHAPE_ERR);
  }
  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 19);
  _u2VopTimeIncrementResolution[u4InstID] = (_u4Datain[u4InstID] >> 2) & 0xffff;
  for(i = 15; i > 0; i--)
  {
    if((_u2VopTimeIncrementResolution[u4InstID] - 1) & (0x1 << i))
    {
      break;
    }
  }
  _ucVopTimeIncrementResolutionBits[u4InstID] = i + 1;
  if(_u4Datain[u4InstID] & 0x1) // fixed_vop_rate
  {
    _u2FixedVopTimeIncrement[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, _ucVopTimeIncrementResolutionBits[u4InstID]);
    _u2FixedVopTimeIncrement[u4InstID] &= u4VerifyLSBMask(_ucVopTimeIncrementResolutionBits[u4InstID]);
  }

  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 29);
  _u4HSize[u4InstID] = (_u4Datain[u4InstID] >> 15) & 0x1fff;
  _u4VSize[u4InstID] = (_u4Datain[u4InstID] >> 1) & 0x1fff;

  _u4RealHSize[u4InstID] = _u4HSize[u4InstID];
  _u4RealVSize[u4InstID] = _u4VSize[u4InstID];

  
  // 021227: Override upper layer width/height.  Trust the ones in VOL.
  _u4UPicW[u4InstID] = _u4HSize[u4InstID];
  _u4UPicH[u4InstID] = _u4VSize[u4InstID];

  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 2);
  _fgVerInterlaced[u4InstID] = (_u4Datain[u4InstID] >> 1) & 0x1;
  _fgVerObmcDisable[u4InstID] = _u4Datain[u4InstID] & 0x1;
  if(_ucVideoObjectLayerVerid[u4InstID] == 1)
  {
    _ucSpriteEnable[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1) & 0x1;
  }
  else
  {
    _ucSpriteEnable[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 2) & 0x3;
  }
  if(_ucSpriteEnable[u4InstID] != SPRITE_NOT_USED && _ucSpriteEnable[u4InstID] != GMC)
  {
    return(UNKNOWN_SPRITE_ERR);
  }
  if(_ucSpriteEnable[u4InstID] == GMC)
  {
    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 9);
    _ucNoOfSpriteWarpingPoints[u4InstID] = (_u4Datain[u4InstID] >> 3) & 0x3f;
    _ucSpriteWarpingAccuracy[u4InstID] = (_u4Datain[u4InstID] >> 1) & 0x3;
    if(_u4Datain[u4InstID] & 0x1)
    {
      return(GMC_BR_CHG_ERR);
    }
  }

  if(u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1) & 0x1)   // not_8_bit
  {
    return(NOT_8_BIT_ERR);
  }

  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 32);
  u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);
  _fgVerQuantType[u4InstID] = _u4Datain[u4InstID] >> 31;
  if(_fgVerQuantType[u4InstID])
  {
    VDEC_INFO_MPEG_BS_INIT_PRM_T rMpegBSInitPrm;
    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0);
    u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);
    _fgVerLoadIntraMatrix[u4InstID] = (_u4Datain[u4InstID] >> 31);
    rMpegBSInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
    rMpegBSInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
    if(_fgVerLoadIntraMatrix[u4InstID])
    {
      if(_u4BSID[u4InstID] == 1)
      {
        UINT32 u4Bytes,u4Bits;
        VDEC_INFO_MPEG_BS_INIT_PRM_T rMpegBSInitPrm;
        
        u4Bytes = u4VDEC_HAL_MPEG_ReadRdPtr(1, u4InstID, (UINT32)_pucVFifo[u4InstID], &u4Bits);
        rMpegBSInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
        rMpegBSInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
        rMpegBSInitPrm.u4ReadPointer= (UINT32)_pucVFifo[u4InstID] + u4Bytes;
      #ifndef  RING_VFIFO_SUPPORT
        rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
      #else
	  //		rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ*(0.5 + 0.5 *(_u4LoadBitstreamCnt[u4InstID]%2)));
		  rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));//mtk40343
      #endif
        i4VDEC_HAL_MPEG_InitBarrelShifter(0, u4InstID, &rMpegBSInitPrm);
        u4VDEC_HAL_MPEG_ShiftGetBitStream(0, u4InstID, u4Bits);
      }
	  #ifdef FW_WRITE_QUANTIZATION
	  vVerifyParseQtable(u4InstID);
	  vVDEC_HAL_MPEG4_FW_LoadQuantMatrix(u4InstID,TRUE,&_u4QuanMatrixLen[u4InstID],(UINT32 *)(rQuantMatrix[u4InstID].ucQuantMatrix));
	  #else
      if(!fgVDEC_HAL_MPEG4_LoadQuantMatrix(u4InstID, TRUE, &_u4QuanMatrixLen[u4InstID], &rMpegBSInitPrm, &rQuantMatrix[u4InstID]))
      {
        return(INTRA_Q_BARSH_ERR);
      }
	  #endif
      if(_u4BSID[u4InstID] == 1)
      {
        UINT32 u4Bytes,u4Bits;
        VDEC_INFO_MPEG_BS_INIT_PRM_T rMpegBSInitPrm;
        
        u4Bytes = u4VDEC_HAL_MPEG_ReadRdPtr(0, u4InstID, (UINT32)_pucVFifo[u4InstID], &u4Bits);
        rMpegBSInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
        rMpegBSInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
        rMpegBSInitPrm.u4ReadPointer= (UINT32)_pucVFifo[u4InstID] + u4Bytes;
      #ifndef  RING_VFIFO_SUPPORT
        rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
      #else
	  //		rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ*(0.5 + 0.5 *(_u4LoadBitstreamCnt[u4InstID]%2)));
		  rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));//mtk40343
      #endif
        i4VDEC_HAL_MPEG_InitBarrelShifter(1, u4InstID, &rMpegBSInitPrm);
        u4VDEC_HAL_MPEG_ShiftGetBitStream(1, u4InstID, u4Bits);
      }
    }
    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0);
    u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);
    _fgVerLoadNonIntraMatrix[u4InstID] = _u4Datain[u4InstID] >> 31;
    if(_fgVerLoadNonIntraMatrix[u4InstID])
    {
      if(_u4BSID[u4InstID] == 1)
      {
        UINT32 u4Bytes,u4Bits;
        VDEC_INFO_MPEG_BS_INIT_PRM_T rMpegBSInitPrm;
        
        u4Bytes = u4VDEC_HAL_MPEG_ReadRdPtr(1, u4InstID, (UINT32)_pucVFifo[u4InstID], &u4Bits);
        rMpegBSInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
        rMpegBSInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
        rMpegBSInitPrm.u4ReadPointer= (UINT32)_pucVFifo[u4InstID] + u4Bytes;
      #ifndef  RING_VFIFO_SUPPORT
        rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
      #else
	  //		rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ*(0.5 + 0.5 *(_u4LoadBitstreamCnt[u4InstID]%2)));
		  rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));//mtk40343
      #endif
        i4VDEC_HAL_MPEG_InitBarrelShifter(0, u4InstID, &rMpegBSInitPrm);
        u4VDEC_HAL_MPEG_ShiftGetBitStream(0, u4InstID, u4Bits);
      }
      
	  #ifdef FW_WRITE_QUANTIZATION
	  vVerifyParseQtable(u4InstID);
	  vVDEC_HAL_MPEG4_FW_LoadQuantMatrix(u4InstID,FALSE,&_u4QuanMatrixLen[u4InstID],(UINT32 *)(rQuantMatrix[u4InstID].ucQuantMatrix));
	  #else
      if(!fgVDEC_HAL_MPEG4_LoadQuantMatrix(u4InstID, FALSE, &_u4QuanMatrixLen[u4InstID], &rMpegBSInitPrm, &rQuantMatrix[u4InstID]))
      {
        return(NINTRA_Q_BARSH_ERR);
      }
	  #endif
      if(_u4BSID[u4InstID] == 1)
      {
        UINT32 u4Bytes,u4Bits;
        VDEC_INFO_MPEG_BS_INIT_PRM_T rMpegBSInitPrm;
        
        u4Bytes = u4VDEC_HAL_MPEG_ReadRdPtr(0, u4InstID, (UINT32)_pucVFifo[u4InstID], &u4Bits);
        rMpegBSInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
        rMpegBSInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
        rMpegBSInitPrm.u4ReadPointer= (UINT32)_pucVFifo[u4InstID] + u4Bytes;
      #ifndef  RING_VFIFO_SUPPORT
        rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
      #else
	  //		rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ*(0.5 + 0.5 *(_u4LoadBitstreamCnt[u4InstID]%2)));
		  rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));//mtk40343
      #endif
        i4VDEC_HAL_MPEG_InitBarrelShifter(1, u4InstID, &rMpegBSInitPrm);
        u4VDEC_HAL_MPEG_ShiftGetBitStream(1, u4InstID, u4Bits);
      }
    }
  }
  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0);
  if(_ucVideoObjectLayerVerid[u4InstID] != 0x1)
  {
    _fgVerQuarterSample[u4InstID] = _u4Datain[u4InstID] >> 31;
    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);
  }
  if(!(_u4Datain[u4InstID] >> 31))
  {
    return(COMP_EST_ERR);
  }
  _fgVerResyncMarkerDisable[u4InstID] = (_u4Datain[u4InstID] >> 30) & 0x1;
  _fgVerDataPartitioned[u4InstID] = (_u4Datain[u4InstID] >> 29) & 0x1;
  if(_fgVerDataPartitioned[u4InstID])
  {
    _fgVerReversibleVlc[u4InstID] = (_u4Datain[u4InstID] >> 28) & 0x1;
    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);
  }
  if(_ucVideoObjectLayerVerid[u4InstID] != 0x1)
  {
    if((_u4Datain[u4InstID] >> 28) & 0x1) // newpred_enable
    {
      return(NEWPRED_ERR);
    }
    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);
//    _fgVerReducedResolutionVopEnable = (_u4Datain >> 27) & 0x1;
    _fgVerReducedResolutionVopEnable[u4InstID] = (_u4Datain[u4InstID] >> 28) & 0x1;
    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);
  }
  // 031021: 阿飛正傳(Days of Being Wild) from Kenloon,
  //         scalability bit is 1, but the following fields aren't present,
  //         so just ignore this bit.
  /*
  if((_u4Datain >> 28) & 0x1) // scalability
  {
    return(SCALABILITY_ERR);
  }*/
  if(fgMPEGNextStartCode(u4InstID) == FALSE)
  {
    return(NO_START_C_ERR3);
  }

  while((_u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0)) == USER_DATA_START_CODE)
  {
    vVerifyUserData(u4InstID);
  }

/*
  _u4Datain = dwGetBitStream(0);
  if(_u4Datain == USER_DATA_START_CODE)
  {
    if((dwRetVal = dwUserData()) != 0)
    {
      return(dwRetVal);
    }
  }
*/
  vVerifyM4vSetVolPrm(u4InstID);
  return(0);
}

void vVerifyGetPxlAspRat(UINT32 u4InstID, BYTE bAspRatInf)
{
  // Table 6-12 of 14496-2
  switch(bAspRatInf)
  {
    case 1:
      _ucParW[u4InstID] = 1;
      _ucParH[u4InstID] = 1;
      break;
    case 2:
      _ucParW[u4InstID] = 12;
      _ucParH[u4InstID] = 11;
      break;
    case 3:
      _ucParW[u4InstID] = 10;
      _ucParH[u4InstID] = 11;
      break;
    case 4:
      _ucParW[u4InstID] = 16;
      _ucParH[u4InstID] = 11;
      break;
    case 5:
      _ucParW[u4InstID] = 40;
      _ucParH[u4InstID] = 33;
      break;
    case 0xf:
      _ucParW[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 8) & 0xff;
      _ucParH[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 8) & 0xff;
      break;
  }
}

UINT32 u4VOP(UINT32 u4InstID)
{
  UINT32 dwRetVal;

  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0);
  if(_u4Datain[u4InstID] != VOP_START_CODE)
  {
    return(VOP_SC_ERR);
  }
  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 3);
  _ucVopCdTp[u4InstID] = (_u4Datain[u4InstID] >> 1) & 0x3;

  if(_ucVopCdTp[u4InstID] != VCT_B)
  {
    _u4RefPicTimeBase[u4InstID] = _u4TimeBase[u4InstID];
    _u4CurrDispTime[u4InstID] = _u4TimeBase[u4InstID];
  }
  else
  {
    _u4CurrDispTime[u4InstID] = _u4RefPicTimeBase[u4InstID];
  }

  while(_u4Datain[u4InstID] & 0x1)
  {
    if(_ucVopCdTp[u4InstID] != VCT_B)
    {
      _u4TimeBase[u4InstID]++;
    }
    _u4CurrDispTime[u4InstID]++;
    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);  // modulo_time_base
  }
  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, _ucVopTimeIncrementResolutionBits[u4InstID] + 1);
  _u2VopTimeIncrement[u4InstID] = _u4Datain[u4InstID] & u4VerifyLSBMask(_ucVopTimeIncrementResolutionBits[u4InstID]);
  _u4CurrDispTime[u4InstID] = (_u4CurrDispTime[u4InstID] * _u2VopTimeIncrementResolution[u4InstID]) + _u2VopTimeIncrement[u4InstID];
  if(_ucVopCdTp[u4InstID] != VCT_B)
  {
    _u4PrevDispTime[u4InstID] = _u4NextDispTime[u4InstID];
    _u4NextDispTime[u4InstID] = _u4CurrDispTime[u4InstID];
    _rDirMode[u4InstID].u4Trd = _u4NextDispTime[u4InstID] - _u4PrevDispTime[u4InstID];
  }
  else
  {
    _rDirMode[u4InstID].u4Trb = _u4CurrDispTime[u4InstID] - _u4PrevDispTime[u4InstID];
    if(_fgVerInterlaced[u4InstID])
    {
      if(_rDirMode[u4InstID].u4TFrm == 0xffffffff)
      {
        _rDirMode[u4InstID].u4TFrm = _u4NextDispTime[u4InstID] - _u4CurrDispTime[u4InstID];
      }
      dwRetVal = u4Div2Slash(_u4PrevDispTime[u4InstID], _rDirMode[u4InstID].u4TFrm);
      _rDirMode[u4InstID].u4Trbi = 2 * (u4Div2Slash(_u4CurrDispTime[u4InstID], _rDirMode[u4InstID].u4TFrm) - dwRetVal);
      _rDirMode[u4InstID].u4Trdi = 2 * (u4Div2Slash(_u4NextDispTime[u4InstID], _rDirMode[u4InstID].u4TFrm) - dwRetVal);
    }
  }
  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 2);
  if((_u4Datain[u4InstID] & 0x1) == 0)  // vop_coded == '0'
  {
    _fgVerVopCoded0[u4InstID] = TRUE;
    _u4SkipFrameCnt[u4InstID]++;
    return(VOP_CODED_0);
  }
  if((_ucVopCdTp[u4InstID] == VCT_P) || fgIsSGmcVop(u4InstID))
  {
    _ucVopRoundingType[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1) & 0x1;
  }
  else
  {
    _ucVopRoundingType[u4InstID] = 0;
  }
  if(_fgVerReducedResolutionVopEnable[u4InstID] &&
     ((_ucVopCdTp[u4InstID] == VCT_I) || (_ucVopCdTp[u4InstID] == VCT_P)))
  {
    _fgVerVopReducedResolution[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1) & 0x1;
  }

  _ucIntraDcVlcThr[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 3) & 0x7;
  if(_fgVerInterlaced[u4InstID])
  {
    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 2);
    _fgVerTopFldFirst[u4InstID] = (_u4Datain[u4InstID] >> 1) & 0x1;
    _fgVerAlternateVerticalScanFlag[u4InstID] = _u4Datain[u4InstID] & 0x1;
  }
  else
  {
    _fgVerAlternateVerticalScanFlag[u4InstID] = FALSE;
  }

  if(fgIsSGmcVop(u4InstID))
  {
    if(_ucNoOfSpriteWarpingPoints[u4InstID] > 0)
    {
      if((dwRetVal = u4SpriteTrajectory(u4InstID)) != 0)
      {
        return(dwRetVal);
      }
    }
  }
  else
  {
    _ucEffectiveWarpingPoints[u4InstID] = 0;
  }
  _ucVopQuant[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 5) & 0x1f;
  if(_ucVopCdTp[u4InstID] != VCT_I)
  {
    _ucFordFCode[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 3) & 0x7;
  }
  if(_ucVopCdTp[u4InstID] == VCT_B)
  {
    _ucBackFCode[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 3) & 0x7;
  }
  u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 32);
  if((dwRetVal = u4VopData()) != 0)
  {
    return(dwRetVal);
  }
  vVerifyM4vSetVopPrm(u4InstID);
  return(0);
}

UINT32 u4SpriteTrajectory(UINT32 u4InstID)
{
  int i;
  BOOL fgBadDivX = ((_u4UserDataCodecVersion[u4InstID] == 500) ||
                    (_u4UserDataCodecVersion[u4InstID] == 501)) &&
                   (_u4UserDataBuildNumber[u4InstID] >= 370) &&
                   (_u4UserDataBuildNumber[u4InstID] <= 413);

  if(fgPureIsoM4v())
  {
    fgBadDivX = FALSE;
  }

  for(i = 0; i < _ucNoOfSpriteWarpingPoints[u4InstID]; i++)
  {
     _ppiWarpingMv[u4InstID][i][0] = i4WarpingMvCode(u4InstID);
    if(!fgBadDivX)
    {
      u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);  // marker_bit
    }
    _ppiWarpingMv[u4InstID][i][1] = i4WarpingMvCode(u4InstID);
    u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);  // marker_bit
  }

  _ucEffectiveWarpingPoints[u4InstID] = _ucNoOfSpriteWarpingPoints[u4InstID];
  while ((_ucEffectiveWarpingPoints[u4InstID] > 0) &&
         (_ppiWarpingMv[u4InstID][_ucEffectiveWarpingPoints[u4InstID] - 1][0] == 0) &&
         (_ppiWarpingMv[u4InstID][_ucEffectiveWarpingPoints[u4InstID] - 1][1] == 0))
  {
    _ucEffectiveWarpingPoints[u4InstID]--;
  }

  if(_ucEffectiveWarpingPoints[u4InstID] > 1)
  {
    return(WARPING_PT_ERR);
  }
  return(0);
}

INT32 i4WarpingMvCode(UINT32 u4InstID)
{
  UINT32 dwLen;
  INT32 code;

  dwLen = u4DmvCodeLength(u4InstID);

  if(dwLen == 0)
  {
    return(0);
  }
  if(u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1) & 0x1)
  {
    code = 1 << (dwLen - 1);
  }
  else
  {
    code = -(1 << dwLen) + 1;
  }
  code += (u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, dwLen - 1) & u4VerifyLSBMask(dwLen - 1));
  return(code);
}

UINT32 u4DmvCodeLength(UINT32 u4InstID)
{
  int dmv_length_msb;
  int dmv_code_length;

  dmv_length_msb = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 2) & 0x3;
  switch(dmv_length_msb)
  {
    case 0:
      return(0);
    case 1:
      return(1 + (u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1) & 0x1));
    case 2:
      return(3 + (u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1) & 0x1));
    case 3:
    default:
      dmv_code_length = 3;
      while(u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1) & 0x1)
      {
        dmv_code_length++;
      }
      return(dmv_code_length + 2);
  }
}

UINT32 u4VerifyLSBMask(UINT32 dwBits)
{
  UINT32 i;
  UINT32 dwMask = 0;

  for(i = 0; i < dwBits; i++)
  {
    dwMask |= (0x1 << i);
  }
  return(dwMask);
}

UINT32 u4GoVop(UINT32 u4InstID)
{
  BOOL fgMarker;

  // first turn to get a packet of bitstream
  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 20);

  fgMarker = (_u4Datain[u4InstID] >> 8) & 0x1;
  if(fgMarker == 0)
  {
    return(GOV_MKB_ERR);
  }

  _fgVerClosedGop[u4InstID] = (_u4Datain[u4InstID] >> 1) & 0x1 ;
  _fgVerBrokenLink[u4InstID] = _u4Datain[u4InstID] & 0x1;

  if (fgMPEGNextStartCode(u4InstID) == FALSE)
  {
    return(NO_START_C_ERR5);
  }

  return(0);
}

void vVerifyUserData(UINT32 u4InstID)
{
  BYTE pbUdBuf[64];
  UINT32 dwBufPos = 0;
  UINT32 dwIdx;

  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 32);
  while((_u4Datain[u4InstID] >> 8) != 0x000001)
  {
    pbUdBuf[dwBufPos] = _u4Datain[u4InstID] >> 24;
    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 8);
    dwBufPos++;

    if(dwBufPos == 64)
    {
      dwBufPos = 0;
    }
  }

  if((pbUdBuf[0] != 'D') ||
     (pbUdBuf[1] != 'i') ||
     (pbUdBuf[2] != 'v') ||
     (pbUdBuf[3] != 'X'))
  {
    return;
  }

  _u4UserDataCodecVersion[u4InstID] = u4VerifyGetDivXNo(&pbUdBuf[4]);

  if((pbUdBuf[7] == 'B') &&
     (pbUdBuf[8] == 'u') &&
     (pbUdBuf[9] == 'i') &&
     (pbUdBuf[10] == 'l') &&
     (pbUdBuf[11] == 'd'))
  {
    _u4UserDataBuildNumber[u4InstID] = u4VerifyGetDivXNo(&pbUdBuf[12]);
    return;
  }

  if((dwIdx = u4VerifyFindChar(pbUdBuf, dwBufPos, 'b')) > 0)
  {
    _u4UserDataBuildNumber[u4InstID] = u4VerifyGetDivXNo(&pbUdBuf[dwIdx + 1]);
  }

  if(u4VerifyFindChar(pbUdBuf, dwBufPos, 'p'))
  {
    _fgVerPrefixed[u4InstID] = TRUE;
    _fgVerHistoryPrefixed[u4InstID] = TRUE;
  }
}

UINT32 u4VerifyGetDivXNo(BYTE *pbNo)
{
  UINT32 dwRetVal;

  dwRetVal = ((pbNo[0] & 0xf) * 100) +
             ((pbNo[1] & 0xf) * 10) +
             (pbNo[2] & 0xf);
  return(dwRetVal);
}

UINT32 u4VerifyFindChar(BYTE *pbBuf, UINT32 dwLen, BYTE bChar)
{
  UINT32 i;

  for(i=0; i<dwLen; i++)
  {
    if(pbBuf[i] == bChar)
    {
      return(i);
    }
  }
  return(0);
}

UINT32 u4VisualObject(UINT32 u4InstID)
{
  if(u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1) & 0x1) // is_visual_object_identifier
  {
    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 7);
    _ucVisualObjectVerid[u4InstID] = (_u4Datain[u4InstID] >> 3) & 0xf;
  }
  if((u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 4) & 0xf) != 1)
  {
    return(VIS_OBJ_TYPE_ERR);
  }

  if(u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1) & 0x1)
  {
    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 5);
    if(_u4Datain[u4InstID] & 0x1)
    {
      u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 24);
    }
  }

  if(fgMPEGNextStartCode(u4InstID) == FALSE)
  {
    return(M4V_NO_START_C_ERR1);
  }

  return(0);
}

BOOL fgMPEG4ValidStartCode(UINT32 u4SC)
{
  if(u4SC == VISUAL_OBJECT_START_CODE)
  {
    return(TRUE);
  }

  if((u4SC >= VIDEO_OBJECT_LAYER_START_CODE_MIN) &&
     (u4SC <= VIDEO_OBJECT_LAYER_START_CODE_MAX))
  {
    return(TRUE);
  }

  if(u4SC == GROUP_OF_VOP_START_CODE)
  {
    return(TRUE);
  }

  if(u4SC == VOP_START_CODE)
  {
    return(TRUE);
  }

  return(FALSE);
}

BOOL fgVPrsValidStartCode(UINT32 u4InstID, BOOL fgMp4, UINT32 *pdwSC)
{
  if (fgMp4)
  {
    if (fgMPEG4ValidStartCode(*pdwSC))
      return TRUE;
    else
    {// check video plane with short header
      if (((*pdwSC) >= VIDEO_OBJECT_START_CODE_MIN) &&
          ((*pdwSC) <= VIDEO_OBJECT_START_CODE_MAX))
      {
        UINT32 dwDatain = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 32);
        if ((dwDatain >= VIDEO_OBJECT_LAYER_START_CODE_MIN) &&
             (dwDatain <= VIDEO_OBJECT_LAYER_START_CODE_MAX))
        {
          *pdwSC = dwDatain;
          return TRUE;
        }
#if H263_USE_SORENSON
        else if((dwDatain & H263_VIDEO_START_MASK) == H263_VIDEO_START_MARKER) // Sorenson
#else
        else if ((dwDatain & SHORT_VIDEO_START_MASK) == SHORT_VIDEO_START_MARKER)
#endif
        {
          *pdwSC = dwDatain;
          return TRUE;
        }
        return FALSE;
      }
#if H263_USE_SORENSON
      else if(((*pdwSC) & H263_VIDEO_START_MASK) == H263_VIDEO_START_MARKER) // Sorenson
#else
      else if(/*(_fgShortHeader[u4InstID])&&*/(((*pdwSC)& SHORT_VIDEO_START_MASK) == SHORT_VIDEO_START_MARKER))
#endif
      {
        return TRUE;
      }
    }
  }
  else
  {
    if ((SEQUENCE_HEADER_CODE == (*pdwSC)) ||
         (GROUP_START_CODE == (*pdwSC)) ||
         (PICTURE_START_CODE == (*pdwSC)) ||
         (SEQUENCE_END_CODE == (*pdwSC)))
      return TRUE;
  }
  return FALSE;
}


#ifdef FW_WRITE_QUANTIZATION
static void vVerifyParseQtable(UINT32 u4InstID)
{
	UINT32 u4Datain, u4RemainData, u4LastQuant = 0;
	UCHAR ucMatrixIdx, ucRestIdx;
	UINT32* pu4Addr = 0;
	UINT32* pu4LenAddr = 0;
	UINT32  u4Byte0, u4Byte1, u4Byte2, u4Byte3;

    //printk("[MPEG4] vVerifyParseQtable\n");
	pu4Addr = (UINT32 *)(rQuantMatrix[u4InstID].ucQuantMatrix);
	pu4LenAddr = &_u4QuanMatrixLen[u4InstID];
	
	(*pu4LenAddr) = 0;
	u4Datain = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0); //skip first byte '8'	
	for (ucMatrixIdx = 0; ucMatrixIdx < MPV_MATRIX_SIZE; ucMatrixIdx++)
	{	  	   
	   u4Byte0 = ((u4Datain >> 24) & 0xFF);
	   u4Byte1 = ((u4Datain >> 16) & 0xFF);
	   u4Byte2 = ((u4Datain >>  8) & 0xFF);
	   u4Byte3 = ((u4Datain)       & 0xFF);
	   if (u4Byte0== 0x0  || u4Byte1 == 0x0 || u4Byte2 == 0x0 || u4Byte3 == 0x0)
         { 
            if (u4Byte0 == 0x0)
            {
               u4RemainData = (u4LastQuant << 24) + (u4LastQuant << 16)+ (u4LastQuant << 8) + u4LastQuant;
               u4Datain = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 8);
            }
            else
            if (u4Byte1 == 0x0)
            {               
               u4RemainData = (u4Byte0 << 24) + (u4Byte0 << 16)+ (u4Byte0 << 8) + u4Byte0;
               pu4Addr[ucMatrixIdx] = u4RemainData;
               
               u4Datain = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 16);
               (*pu4LenAddr) += 4;
               ucMatrixIdx++;
               u4LastQuant = u4Byte0;
               u4RemainData =  (u4LastQuant << 24) + (u4LastQuant << 16)+ (u4LastQuant << 8) + u4LastQuant;
            }
            else
            if (u4Byte2 == 0x0)
            {
               u4RemainData = (u4Byte1 << 24) + (u4Byte1 << 16)+ (u4Byte1 << 8) + u4Byte0;
               pu4Addr[ucMatrixIdx] = u4RemainData;
               
               u4Datain = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 24);
               (*pu4LenAddr) += 4;
               ucMatrixIdx++;
               u4LastQuant = u4Byte1;
               u4RemainData =  (u4LastQuant << 24) + (u4LastQuant << 16)+ (u4LastQuant << 8) + u4LastQuant;
            }
            else
            {              
               u4RemainData = (u4Byte2 << 24) + (u4Byte2 << 16)+ (u4Byte1 << 8) + u4Byte0;
               pu4Addr[ucMatrixIdx] = u4RemainData;
               
               u4Datain = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 32);
               (*pu4LenAddr) += 4;
               ucMatrixIdx++;
               u4LastQuant = u4Byte2;
               u4RemainData =  (u4LastQuant << 24) + (u4LastQuant << 16)+ (u4LastQuant << 8) + u4LastQuant;
            }
                        
            for (ucRestIdx = ucMatrixIdx; ucRestIdx < MPV_MATRIX_SIZE; ucRestIdx++)
	     {
	         pu4Addr[ucRestIdx] = u4RemainData;
                (*pu4LenAddr) += 4;
	     }
            break;
         }
	   	      
	   pu4Addr[ucMatrixIdx] = INVERSE_ENDIAN(u4Datain);            
	   u4LastQuant = u4Byte3;
	   (*pu4LenAddr) += 4;
	   
	   u4Datain = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 32);
	}
	
}
#endif


