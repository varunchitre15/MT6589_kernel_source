#include "vdec_verify_mpv_prov.h"
#include "../hal/vdec_hal_if_common.h"
#include "../hal/vdec_hal_if_mpeg.h"
#include "vdec_verify_keydef.h"
#include "vdec_verify_vparser_mpeg.h"

BOOL fgMPEGNextStartCode(UINT32 u4InstID);
INT32 i4DealSeqHdr(UINT32 u4InstID);
INT32 i4SequenceHeader(UINT32 u4InstID);
INT32 i4SequenceExt(UINT32 u4InstID);
INT32 i4ExtUsrData(UINT32 u4InstID, BYTE bIndex);
INT32 i4ExtData(UINT32 u4InstID, BYTE bIndex);
INT32 i4UsrData(UINT32 u4InstID, BYTE bIdx);
INT32 i4SeqDisplayExt(UINT32 u4InstID);
INT32 i4QuantMatrixExt(UINT32 u4InstID);
INT32 i4CopyrightExt(UINT32 u4InstID);
INT32 i4PictureDisExt(UINT32 u4InstID);
INT32 i4GroupPicHdr(UINT32 u4InstID);
INT32 i4PicHdr(UINT32 u4InstID);
INT32 i4PicCodExt(UINT32 u4InstID);
INT32 i4PicData(UINT32 u4InstID);
void vVerifyMp2SetPicLayerDecPrm(UINT32 u4InstID);
//extern void vVDecOutputDebugString(const CHAR * format, ...);

#ifdef FW_WRITE_QUANTIZATION
static void vVerifyParseQtable(UINT32 u4InstID);
#endif

// *********************************************************************
// Function : BOOL vVerifyMpvSetSeqLayerDecPrm(UINT32 u4InstID)
// Description :
// Parameter :
// Return    :
// *********************************************************************
void vVerifyMpvSetSeqLayerDecPrm(UINT32 u4InstID)
{
  VDEC_INFO_MPEG_DEC_PRM_T *ptDecPrm = &_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm;

  ptDecPrm->ucMpegVer = _ucMpegVer[u4InstID];
  _tVerMpvDecPrm[u4InstID].u4PicW = _u4HSize[u4InstID];
  _tVerMpvDecPrm[u4InstID].u4PicH = _u4VSize[u4InstID];
  _tVerMpvDecPrm[u4InstID].u4PicBW = _tVerMpvDecPrm[u4InstID].u4PicW;
}

UINT32 u4VParserMPEG12(UINT32 u4InstID, BOOL fgInquiry)
{
    UINT32 u4Retry = 0;
    INT32  i4RetVal;

    _fgVerVopCoded0[u4InstID] = FALSE;
    vVDEC_HAL_MPEG_SetMPEG4Flag(u4InstID, FALSE);
    // finding the first start code will be used

    printk("<vdec> Input window is 0x%x (%s, %d)\n", u4VDecReadVLD(u4InstID, 0x00), __FUNCTION__, __LINE__);
	
    do
    {
        if (fgMPEGNextStartCode(u4InstID)==FALSE)
        {
            return(NO_START_C_ERR1);
        }
        _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0);
        if((_u4Datain[u4InstID] != SEQUENCE_HEADER_CODE) && (_u4Datain[u4InstID] != GROUP_START_CODE) &&
            (_u4Datain[u4InstID] !=PICTURE_START_CODE))
        {
            u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 8);
        }
        if(++u4Retry > MAX_VER_RETRY_COUNT)
        {
            return(NO_START_C_ERR2);
        }
    } while((_u4Datain[u4InstID] != SEQUENCE_HEADER_CODE) && (_u4Datain[u4InstID] != GROUP_START_CODE) &&
        (_u4Datain[u4InstID] != PICTURE_START_CODE));

    if(_u4Datain[u4InstID] == SEQUENCE_HEADER_CODE)
    {
        u4Retry = 0;
        do
        {
            if(u4Retry > 2)
            {
                #ifdef GOPH_LOC_ERR
                return(GOPH_LOC_ERR);
                #endif
            }
            i4RetVal = i4DealSeqHdr(u4InstID);
            if (i4RetVal!=0)
            {
                return (i4RetVal);
            }
            _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0);
            u4Retry++;

            // parsing of MPEG2 sequence extension
            if(_u4Datain[u4InstID] == EXTENSION_START_CODE)
            {
                i4RetVal = i4SequenceExt(u4InstID);
                if(i4RetVal != 0)
                {
                    return(i4RetVal);
                }
                _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0);
            }

            if(!fgInquiry)
            {
                if(_tVerPic[u4InstID].ucMpegVer != _ucMpegVer[u4InstID])
                {
                    #if SEQE_LOC_ERR
                    return(SEQE_LOC_ERR);
                    #endif
                }
            }
            if(fgIsVerifyMpeg1(u4InstID))
            {
                // mpeg1
                _u4PicStruct[u4InstID] = FRM_PIC;
                _fgVerProgressiveSeq[u4InstID] = TRUE;
                _fgVerProgressiveFrm[u4InstID] = TRUE;
                _fgVerTopFldFirst[u4InstID] = TRUE;
                _fgVerRepFirstFld[u4InstID] = FALSE;
            }

            // for MC para
            _u4HSize[u4InstID] = _u4HSizeVal[u4InstID] + (_ucHSizeExt[u4InstID] << 12);
            _u4VSize[u4InstID] = _u4VSizeVal[u4InstID] + (_ucVSizeExt[u4InstID] << 12);
            _u4PicWidthMB[u4InstID] = (_u4HSize[u4InstID]+15)/16;

            _u4RealHSize[u4InstID] = _u4HSize[u4InstID];
            _u4RealVSize[u4InstID] = _u4VSize[u4InstID];

            if(fgIsVerifyMpeg2(u4InstID)) // MPEG2 sequence display/scalable extension
            {
                i4RetVal = i4ExtUsrData(u4InstID, 0);
            }
            else // MPEG1 user data of sequence layer
            {
                i4RetVal = i4ExtUsrData(u4InstID, 1);
            }
            if(i4RetVal != 0)
            {
                return(i4RetVal);
            }

            vVerifyMpvSetSeqLayerDecPrm(u4InstID);

            if(!fgInquiry)
            {
                // check if size match
                if(_u4HSize[u4InstID] != _tVerPic[u4InstID].u4W)
                {
                    #if(HORI_SIZE_ERR)
                    _u4HSizeVal[u4InstID] = _tVerPic[u4InstID].u4W;
                    _u4HSize[u4InstID] = _tVerPic[u4InstID].u4W;
                    _ucHSizeExt[u4InstID] = 0;
                    _u4VSizeVal[u4InstID] = _tVerPic[u4InstID].u4H;
                    _u4VSize[u4InstID] = _tVerPic[u4InstID].u4H;
                    _ucVSizeExt[u4InstID] = 0;
                    return(HORI_SIZE_ERR);
                    #else
                    _u4HSize[u4InstID] = DEFAULT_H_SIZE;
                    #endif
                }
                if(_u4VSize[u4InstID]!= _tVerPic[u4InstID].u4H)
                {
                    #if(VERT_SIZE_ERR)
                    _u4HSizeVal[u4InstID] = _tVerPic[u4InstID].u4W;
                    _u4HSize[u4InstID] = _tVerPic[u4InstID].u4W;
                    _ucHSizeExt[u4InstID] = 0;
                    _u4VSizeVal[u4InstID] = _tVerPic[u4InstID].u4H;
                    _u4VSize[u4InstID] = _tVerPic[u4InstID].u4H;
                    _ucVSizeExt[u4InstID] = 0;
                    return(VERT_SIZE_ERR);
                    #else
                    _u4VSize[u4InstID] = DEFAULT_V_SIZE;
                    #endif //VERT_SIZE_ERR
                }
            }
            else
            {
                if(!fgVerifyMpvPicDimChk(u4InstID, _u4HSize[u4InstID], _u4VSize[u4InstID], TRUE))
                {
                    return(PIC_DIM_ERR);
                }
                // Inquery only deal with sequence parameters
                return(0);
            }

        } while(_u4Datain[u4InstID] == SEQUENCE_HEADER_CODE);
    }

    if(_u4VSize[u4InstID] & 0xF)
    {
        _u4VSize[u4InstID] = (_u4VSize[u4InstID] & 0xFFFFFFF0) + 0x10;
    }
    if(_u4HSize[u4InstID] & 0xF)
    {
        _u4HSize[u4InstID] = (_u4HSize[u4InstID] & 0xFFFFFFF0) + 0x10;
    }

    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0);
    if(_u4Datain[u4InstID] == GROUP_START_CODE)
    {
        u4Retry = 0;
        do
        {
            if(u4Retry > 2)
            {
                #ifdef PICH_LOC_ERR
                return(PICH_LOC_ERR);
                #endif
            }
            i4RetVal = i4GroupPicHdr(u4InstID);
            if(i4RetVal != 0)
            {
                return(i4RetVal);
            }

            i4RetVal = i4ExtUsrData(u4InstID, 1);
            if(i4RetVal != 0)
            {
                return(i4RetVal);
            }
            _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0);
            u4Retry++;
        }while(_u4Datain[u4InstID] == GROUP_START_CODE);
    }

    // Record if Previous Picture is B Picture, before Deal Picture Header
    _fgVerPrevBPic[u4InstID] = (_u4PicCdTp[u4InstID] == B_TYPE) ? TRUE : FALSE;

    _u4PicPSXOff[u4InstID] = 0xFFFFFFFF;

    i4RetVal = i4PicHdr(u4InstID);
    if(i4RetVal != 0)
    {
        return(i4RetVal);
    }

    if(fgIsVerifyMpeg2(u4InstID)) // Two types of MPEG2 Picture-layer extension
    {
        i4RetVal = i4PicCodExt(u4InstID);
        if(i4RetVal != 0)
        {
            return(i4RetVal);
        }
        //datain = get_bitstream(0, 0, 0);
        i4RetVal = i4ExtUsrData(u4InstID, 2);
        if(i4RetVal != 0)
        {
            return(i4RetVal);
        }
    }
    else if(fgIsVerifyMpeg1(u4InstID)) // MPEG1 user data of Picture layer
    {
        i4RetVal = i4ExtUsrData(u4InstID, 1);
        if(i4RetVal != 0)
        {
            return(i4RetVal);
        }
        _fgVerIntraVlcFmt[u4InstID] = 0;
    }

    i4RetVal=i4PicData(u4InstID);
    vVerifyMp2SetPicLayerDecPrm(u4InstID);

    return(i4RetVal);

}

// *********************************************************************
// Function : INT32 i4SequenceExt(UINT32 u4InstID)
// Description : parse sequence extension
// Parameter : NONE
// Return    : 0, if OK
//             others, Error
// *********************************************************************
INT32 i4SequenceExt(UINT32 u4InstID)
{
  BYTE bExtStartCodeId;               //identifier;
  BYTE bPrfInd;                       //profile_id;
  BYTE bLvlInd;                       //level
  //UINT16 wBitRatExt;                    //bit_rate_extension;
  //BYTE bVbvBufSizExt;                 //UINT32 vbv_buffer_size_extension;

  //BOOL fgLowDly;                      //low_delay;
  //BYTE bFrmRatExtN;                   //frame_rate_extension_n;
  //BYTE bFrmRatExtD;                   //frame_rate_extension_d;
//  UINT32 _u4Datain;
  BOOL fgMarker;                      //marker_bit;

//  MPEG1_flag = 0; before call this subroutine, it is FALSE
  _fgVerSeqHdr[u4InstID] = FALSE;
  _ucMpegVer[u4InstID] = VDEC_MPEG2;

  // first turn to get a packet of bitstream
  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 32);// get extension start code

  //extension start code identifier 0, 6, >10 reserved
  bExtStartCodeId = _u4Datain[u4InstID] >> 28;
  if((bExtStartCodeId == 0) || (bExtStartCodeId == 6) || (bExtStartCodeId > 10) )
  {
#if(EXT_ST_COD_ERR)
    return (EXT_ST_COD_ERR);
#endif //EXT_ST_COD_ERR
  }

  bPrfInd = (_u4Datain[u4InstID] & 0x07000000) >> 24;
  if((bPrfInd == 0) || (bPrfInd >= 6))
  {
#if(PROF_ID_RES)
    return(PROF_ID_RES);
#endif //PROF_ID_RES
  }

  bLvlInd = (_u4Datain[u4InstID] & 0x00F00000) >> 20;
  if((bLvlInd != LEVEL_ID_LOW) && (bLvlInd != LEVEL_ID_MAIN) &&
     (bLvlInd != LEVEL_ID_HIGH_1440) && (bLvlInd != LEVEL_ID_HIGH))
  {
#if(LEV_ID_RES)
    return(LEV_ID_RES);
#endif //LEV_ID_RES
  }

  _fgVerProgressiveSeq[u4InstID] = (_u4Datain[u4InstID] & 0x00080000) >> 19;
  _ucCrmaFmt[u4InstID] = (_u4Datain[u4InstID] & 0x00060000) >> 17;
  if(_ucCrmaFmt[u4InstID] == 0)
  {
#if(CH_FORMAT_RES)
    return(CH_FORMAT_RES);
#endif //HORI_SIZE_ERR
  }

  _ucHSizeExt[u4InstID] = (_u4Datain[u4InstID] & 0x00018000) >> 15;
  _ucVSizeExt[u4InstID] = (_u4Datain[u4InstID] & 0x00006000) >> 13;
  //wBitRatExt = (_u4Datain[u4InstID] & 0x00001FFE) >> 1;

  fgMarker = _u4Datain[u4InstID] & 0x00000001;
  if(fgMarker == 0)
  {
#if(SEQE_MKB_ERR)
    return(SEQE_MKB_ERR);
#endif //SEQE_MKB_ERR
  }

  // second turn to get a packet of bitstream
  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 32);// get 32 bits off

  //bVbvBufSizExt = (_u4Datain[u4InstID] & 0xFF000000) >> 24;

  //fgLowDly = (_u4Datain[u4InstID] & 0x00800000) >> 23;

  //bFrmRatExtN = (_u4Datain[u4InstID] & 0x00600000) >> 21;
  //bFrmRatExtD = (_u4Datain[u4InstID] & 0x001F0000) >> 16;

  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 16);// get 16 bits off

  if (fgMPEGNextStartCode(u4InstID)==FALSE)
  {
    return(NO_START_C_ERR4);
  }

  return 0;
}

// *********************************************************************
// Function : INT32 i4ExtUsrData(UINT32 u4InstID, BYTE bIndex)
// Description : parse extension and user data
// Parameter :
// Return    : 0, if OK
//             others, Error
// *********************************************************************
INT32 i4ExtUsrData(UINT32 u4InstID, BYTE bIndex)        //extension_and_user_data(BYTE i)
{
  INT32 iRetVal;

  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0);

  while((_u4Datain[u4InstID] == EXTENSION_START_CODE) || (_u4Datain[u4InstID] == USER_DATA_START_CODE))
  {
    if(_u4Datain[u4InstID] == EXTENSION_START_CODE)
    {
      if(bIndex != 1)
      {
        iRetVal = i4ExtData(u4InstID, bIndex);
        if(iRetVal != 0)
          return(iRetVal);
        _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0);
      }
#ifdef EXT_DATA_IDX_ERR
      else
      {
        return(EXT_DATA_IDX_ERR);
      }
#endif
    }

    if(_u4Datain[u4InstID] == USER_DATA_START_CODE)
    {
      iRetVal = i4UsrData(u4InstID, bIndex);
      if(iRetVal != 0)
        return(iRetVal);
      _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0);
    }
  }
  return 0;
}

// *********************************************************************
// Function : INT32 i4ExtData(UINT32 u4InstID, BYTE bIndex)
// Description : parse extension data
// Parameter :
// Return    : 0, if OK
//             others, Error
// *********************************************************************
INT32 i4ExtData(UINT32 u4InstID, BYTE bIndex)       //extension_data(BYTE i)
{
  INT32 iRetVal;
  BYTE bExtStartCode;

  while((_u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0)) == EXTENSION_START_CODE)
  {
    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 32);
    bExtStartCode = _u4Datain[u4InstID] >>28;

    //seq_extension
    if(bIndex==0)
    {
      if(bExtStartCode == SEQUENCE_DISPLAY_EXTENSION_ID)
      {
        iRetVal = i4SeqDisplayExt(u4InstID);
        if(iRetVal != 0)
          return(iRetVal);
      }
      else
      {
        // find next start code
        u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 8);
        if (fgMPEGNextStartCode(u4InstID)==FALSE)
        {
          return(NO_START_C_ERR5);
        }
      }
    } // end if(bIndex==0)

    //pix_coding_extension
    if(bIndex==2)
    {
      iRetVal = 0;
      if(bExtStartCode == QUANT_MATRIX_EXTENSION_ID)
      {
        iRetVal = i4QuantMatrixExt(u4InstID);
      }
      else if(bExtStartCode == COPYRIGHT_EXTENSION_ID)
      {
        iRetVal = i4CopyrightExt(u4InstID);
      }
      else if(bExtStartCode == PICTURE_DISPLAY_EXTENSION_ID)
      {
        iRetVal = i4PictureDisExt(u4InstID);
      }
      else
      {
        u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 8);
        if(fgMPEGNextStartCode(u4InstID)==FALSE)
        {
          return(NO_START_C_ERR6);
        }
      }
      if(iRetVal != 0)
        return(iRetVal);
    } // end if(bIndex==2)
  } // end while((_u4Datain = dwGetBitStream(0)) == EXTENSION_START_CODE)
  return 0;
}

// *********************************************************************
// Function : INT32 i4UsrData(UINT32 u4InstID, BYTE bIdx)
// Description : parse user data
// Parameter : bIdx: indicate which layer this user_data() belongs to
// Return    : 0, if OK
//             others, Error
// *********************************************************************
INT32 i4UsrData(UINT32 u4InstID, BYTE bIdx)
{
  BYTE bIndex = 0;

  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 32);

#if 0
  if(((_u4Datain[u4InstID] >> 16) == LINE21_INDICATOR) &&
     (bIdx == 1) && fgCCOn())
  {
    iRet = iLine21DataProc();
    if(iRet != 0)
    {
      // Error
    }
  }
#endif


  {
    while((_u4Datain[u4InstID] >> 8) != 0x000001)
    {
      _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 8);
      if(bIndex==MAX_USER_DATA_SIZE)
      {
#if(USR_DAT_BG_ERR)
        return(USR_DAT_BG_ERR);
#else
        break;
#endif
      }
    }
  }
  if (fgMPEGNextStartCode(u4InstID)==FALSE)
  {
    return(NO_START_C_ERR7);
  }

  return 0;
}

// *********************************************************************
// Function : INT32 i4SeqDisplayExt(UINT32 u4InstID)
// Description : parse sequence display extension
// Parameter : NONE
// Return    : 0, if OK
//             others, Error
// *********************************************************************
INT32 i4SeqDisplayExt(UINT32 u4InstID)        //sequence_display_extension()
{
  BOOL fgColorDes;              //UINT32 color_description;
  BYTE bColorPrim;              //UINT32 color_primaries;
  BYTE bTnsfrCh;                //UINT32 transfer_characteristics;
  BYTE bMatrixCof;              //UINT32 matrix_coefficients;
  //UINT16 wDisplayHSize;           //UINT32 display_horizontal_size;
  //UINT16 wDisplayVSize;
  BOOL fgMarker;
  //BYTE bVdoFmt;

  // first turn to get a packet of bitstream
  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 4); // extension start code id

  //bVdoFmt = (_u4Datain[u4InstID] & 0xE0000000) >> 29;
  fgColorDes = (_u4Datain[u4InstID] & 0x10000000) >> 28;

  if(fgColorDes==TRUE)
  {
    bColorPrim = (_u4Datain[u4InstID] & 0x0FF00000) >> 20;
    if((bColorPrim == 0) || (bColorPrim >= 8))
    {
#if(COLOR_PRI_ERR)
      //return(COLOR_PRI_ERR);
#endif //COLOR_PRI_ERR
    }

    bTnsfrCh = (_u4Datain[u4InstID] & 0x000FF000) >> 12;
    if((bTnsfrCh == 0) || (bTnsfrCh >= 9))
    {
#if(TRA_CHA_ERR)
      //return(TRA_CHA_ERR);
#endif //TRA_CHA_ERR
    }

    bMatrixCof = (_u4Datain[u4InstID] & 0x00000FF0) >> 4;
    if((bMatrixCof == 0) || (bMatrixCof >= 8))
    {
#if(MAT_COE_ERR)
      //return(MAT_COE_ERR);
#endif //MAT_COE_ERR
    }
    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 28);    // 3 + 1 + 8 + 8 + 8
  }
  else
  {
    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 4);     // 3 + 1
  }

  //wDisplayHSize = (_u4Datain[u4InstID] & 0xFFFC0000) >> 18;
  fgMarker = (_u4Datain[u4InstID] & 0x00020000) >> 17;
  if(fgMarker == FALSE)
  {
#if(SEQDE_MKB_ERR)
    return(SEQDE_MKB_ERR);
#endif //SEQDE_MKB_ERR
  }

  //wDisplayVSize = (_u4Datain[u4InstID] & 0x0001FFF8) >> 3;

  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 29);      // 14 + 1 + 14
  if (fgMPEGNextStartCode(u4InstID)==FALSE)
  {
    return(NO_START_C_ERR8);
  }
  return 0;
}

// *********************************************************************
// Function : INT32 i4QuantMatrixExt(UINT32 u4InstID)
// Description : parse quant matrix extension
// Parameter : NONE
// Return    : 0, if OK
//             others, Error
// *********************************************************************
INT32 i4QuantMatrixExt(UINT32 u4InstID)
{
  //BYTE pbChromaIntraQMatrix[64];
  //BYTE pbChromaNonIntraQMatrix[64];

  BOOL fgLoadChromaIntraQMatrix;
  BOOL fgLoadChromaNonIntraQMatrix;
  BYTE bIndex;

  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 4);                   // extension start code id

  _fgVerLoadIntraMatrix[u4InstID] = _u4Datain[u4InstID] >> 31;

  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);

  if(_fgVerLoadIntraMatrix[u4InstID]==TRUE)
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
//      rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ*(0.5 + 0.5 *(_u4LoadBitstreamCnt[u4InstID]%2)));
	rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));//mtk40343
    #endif
      i4VDEC_HAL_MPEG_InitBarrelShifter(0, u4InstID, &rMpegBSInitPrm);
      u4VDEC_HAL_MPEG_ShiftGetBitStream(0, u4InstID, u4Bits);
    }
    #ifdef FW_WRITE_QUANTIZATION
    vVerifyParseQtable(u4InstID);
    vVDEC_HAL_MPEG12_FW_LoadQuantMatrix(u4InstID,TRUE,(UINT32 *)(rQuantMatrix[u4InstID].ucQuantMatrix));
    #else
    vVDEC_HAL_MPEG12_LoadQuantMatrix(u4InstID, TRUE, &rQuantMatrix[u4InstID]);
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
	//		  rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ*(0.5 + 0.5 *(_u4LoadBitstreamCnt[u4InstID]%2)));
		rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));//mtk40343
    #endif
      i4VDEC_HAL_MPEG_InitBarrelShifter(1, u4InstID, &rMpegBSInitPrm);
      u4VDEC_HAL_MPEG_ShiftGetBitStream(1, u4InstID, u4Bits);
    }
    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0);
  }

  _fgVerLoadNonIntraMatrix[u4InstID] = _u4Datain[u4InstID] >> 31;

  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);

  if(_fgVerLoadNonIntraMatrix[u4InstID]==TRUE)
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
	//		  rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ*(0.5 + 0.5 *(_u4LoadBitstreamCnt[u4InstID]%2)));
		rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));//mtk40343
    #endif
      i4VDEC_HAL_MPEG_InitBarrelShifter(0, u4InstID, &rMpegBSInitPrm);
      u4VDEC_HAL_MPEG_ShiftGetBitStream(0, u4InstID, u4Bits);
    }
    #ifdef FW_WRITE_QUANTIZATION
    vVerifyParseQtable(u4InstID);
    printk("FW Load Qtable ! \n");
    vVDEC_HAL_MPEG12_FW_LoadQuantMatrix(u4InstID,FALSE,(UINT32 *)(rQuantMatrix[u4InstID].ucQuantMatrix));
    #else
    vVDEC_HAL_MPEG12_LoadQuantMatrix(u4InstID, FALSE, &rQuantMatrix[u4InstID]);
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
	//		  rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ*(0.5 + 0.5 *(_u4LoadBitstreamCnt[u4InstID]%2)));
		rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));//mtk40343
    #endif
      i4VDEC_HAL_MPEG_InitBarrelShifter(1, u4InstID, &rMpegBSInitPrm);
      u4VDEC_HAL_MPEG_ShiftGetBitStream(1, u4InstID, u4Bits);
    }
    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0);
  }

  fgLoadChromaIntraQMatrix = _u4Datain[u4InstID] >> 31;

  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);

  if(fgLoadChromaIntraQMatrix==TRUE)
  {
    for(bIndex=0 ; bIndex < 64 ; bIndex++)
    {
      //pbChromaIntraQMatrix[bIndex] = _u4Datain >> 24;
      _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 8);
    }
  }

  fgLoadChromaNonIntraQMatrix = _u4Datain[u4InstID]>> 31;
  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);

  if(fgLoadChromaNonIntraQMatrix==TRUE)
  {
    for(bIndex=0 ; bIndex < 64 ; bIndex++)
    {
      //pbChromaNonIntraQMatrix[bIndex] = _u4Datain >> 24;
      _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 8);
    }
  }

  if (fgMPEGNextStartCode(u4InstID)==FALSE)
  {
    return(NO_START_C_ERR9);
  }
  return(0);
}

// *********************************************************************
// Function : INT32 i4CopyrightExt(UINT32 u4InstID)
// Description : parse c_o_p_y_r_i_g_h_t e_x_t_e_n_s_i_o_n
// Parameter : NONE
// Return    : 0, if OK
//             others, Error
// *********************************************************************
INT32 i4CopyrightExt(UINT32 u4InstID)   //copyright_extension()
{
  //BOOL fgCopyright;             //UINT32 copyright_flag;
  //BYTE bCopyIndr;               //UINT32 copyright_identifier;
  //BOOL fgOrgOrCopy;             //UINT32 original_or_copy;
  //UINT32 dwCopyNum1;              //UINT32 copyright_number_1;
  //UINT32 dwCopyNum2;              //UINT32 copyright_number_2;
  //UINT32 dwCopyNum3;              //UINT32 copyright_number_3;
//  UINT32 _u4Datain;
  BOOL fgMarker;
  BYTE bResVal;                 //reserved_value;

  // first turn to get a packet of bitstream
  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 4);

  //fgCopyright = _u4Datain[u4InstID] >> 31;
  //bCopyIndr = (_u4Datain[u4InstID] & 0x7F800000) >> 23;
  //fgOrgOrCopy = (_u4Datain[u4InstID] & 0x00400000) >> 22;

  bResVal = (_u4Datain[u4InstID] & 0x003F8000) >> 15;
  if(bResVal != 0)
  {
#if(PICCE_RES_ERR)
    return(PICCE_RES_ERR);
#endif //PICCE_RES_ERR
  }

  fgMarker = (_u4Datain[u4InstID] & 0x00004000) >> 14;
  if(fgMarker == 0)
  {
#if(PICCE_MKB1_ERR)
    return(PICCE_MKB1_ERR);
#endif //PICCE_MKB1_ERR
  }

  // second turn to get a packet of bitstream
  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 18);
  //dwCopyNum1 = _u4Datain[u4InstID] >> 12;

  fgMarker = (_u4Datain[u4InstID] & 0x00000800) >> 11;
  if(fgMarker == 0)
  {
#if(PICCE_MKB2_ERR)
    return(PICCE_MKB2_ERR);
#endif //PICCE_MKB2_ERR
  }

  // third turn to get a packet of bitstream
  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 21);

  //dwCopyNum2 = _u4Datain[u4InstID] >> 10;
  fgMarker = (_u4Datain[u4InstID] & 0x00000200) >> 9;
  if(fgMarker == 0)
  {
#if(PICCE_MKB3_ERR)
    return(PICCE_MKB3_ERR);
#endif //PICCE_MKB3_ERR
  }

  // forth turn to get a packet of bitstream
  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 23);
  //dwCopyNum3 = _u4Datain[u4InstID] >> 10;
  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 22);

  if (fgMPEGNextStartCode(u4InstID)==FALSE)
  {
    return(NO_START_C_ERR10);
  }

  return 0;
}

// *********************************************************************
// Function : INT32 i4PictureDisExt(UINT32 u4InstID)
// Description : parse picture display extension
// Parameter : NONE
// Return    : 0, if OK
//             others, Error
// *********************************************************************
INT32 i4PictureDisExt(UINT32 u4InstID)        //picture_display_extension()
{
  //UINT16 wFrmCenterHOfst[3];                    //frame_center_horizontal_offset[3];
  //UINT16 wFrmCenterVOfst[3];                    //frame_center_vertical_offset[3];
  BYTE bNumFrmCenterOfst;                     //number_of_frame_centre_offsets;
  UINT32 i;
  BOOL fgMarker;

  if(_fgVerProgressiveSeq[u4InstID] == 1)
  {
    if(_fgVerRepFirstFld[u4InstID] == 1)
    {
      if(_fgVerTopFldFirst[u4InstID] == 1)
      {
        bNumFrmCenterOfst = 3;
      }
      else
      {
        bNumFrmCenterOfst = 2;
      }
    }
    else
    {
      bNumFrmCenterOfst = 1;
    }
  }
  else
  {
    if(_u4PicStruct[u4InstID] < 3) //// field
    {
      bNumFrmCenterOfst = 1;
    }
    else
    {
      if(_fgVerRepFirstFld[u4InstID]==1)
      {
        bNumFrmCenterOfst = 3;
      }
      else
      {
        bNumFrmCenterOfst = 2;
      }
    }
  }

  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 4);

  for(i=0 ; i<bNumFrmCenterOfst; i++)
  {
    //wFrmCenterHOfst[i] = _u4Datain[u4InstID] >> 16;
    fgMarker = (_u4Datain[u4InstID] & 0x00008000) >> 15;
    if(fgMarker == 0)
    {
#if(PICDE_MKB1_ERR)
      return(PICDE_MKB1_ERR);
#endif //PICDE_MKB1_ERR
    }

    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 17);

    //wFrmCenterVOfst[i] = _u4Datain[u4InstID] >> 16;
    fgMarker = (_u4Datain[u4InstID] & 0x00008000) >> 15;
    if(fgMarker == 0)
    {
#if(PICDE_MKB2_ERR)
      return(PICDE_MKB2_ERR);
#endif //PICDE_MKB2_ERR
    }

    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 17);
  }

  if (fgMPEGNextStartCode(u4InstID)==FALSE)
  {
    return(NO_START_C_ERR11);
  }

  return 0;
}

// *********************************************************************
// Function : INT32 i4DealSeqHdr(UINT32 u4InstID)
// Description :
// Parameter : None
// Return    : None
// *********************************************************************
INT32 i4DealSeqHdr(UINT32 u4InstID)
{
  INT32 iRetVal;
  //reset of quant_matrix
  vVDEC_HAL_MPEG_ResetQuantMatrix(u4InstID);
  //resetting of b_pic_ini_flag
  _u4BPicIniFlag[u4InstID] = 0;
  _u4BPicIniFlag0[u4InstID] = 0;
  iRetVal = i4SequenceHeader(u4InstID);
  return(iRetVal);
}

// *********************************************************************
// Function : INT32 i4SequenceHeader(UINT32 u4InstID)
// Description : parse sequence header
// Parameter : NONE
// Return    : 0, if OK
//             others, Error
// *********************************************************************
INT32 i4SequenceHeader(UINT32 u4InstID)
{
  //UINT16 wVbvBufSizeVal;           //  vbv_buffer_size_value;
  //BOOL fgCnstainPara;            //  constrained_parameters_flag;
  BOOL fgMarkBit;

//setting of the seq_header_flag to check MPEG1 bits
  _fgVerSeqHdr[u4InstID] = TRUE;
  _ucMpegVer[u4InstID] = VDEC_MPEG1;

// first turn to get a packet of bitstream
  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 32);                // get sequence header code

  _u4HSizeVal[u4InstID] = (_u4Datain[u4InstID] & 0xFFF00000) >> 20;
  _u4VSizeVal[u4InstID] = (_u4Datain[u4InstID] & 0x000FFF00) >> 8;


  _ucAspRatInf[u4InstID] = (_u4Datain[u4InstID] & 0x000000F0) >> 4;
  if((_ucAspRatInf[u4InstID] == 0) || (_ucAspRatInf[u4InstID] == 15))
  {
#if(ASP_RAT_RES)
    return(ASP_RAT_RES);
#else
    _ucAspRatInf[u4InstID] = 2;/// 3:4
#endif //ASP_RAT_RES
  }

  _u4FrmRatCod[u4InstID] = _u4Datain[u4InstID] & 0x0000000F;
  if((_u4FrmRatCod[u4InstID] == 0) || (_u4FrmRatCod[u4InstID] > 8))
  {
    _u4FrmRatCod[u4InstID] = 4;///
  }

// second turn to get a packet of bitstream
  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 32);  // HSize + VSize + AspRat + FrmRat

  _u4BitRatVal[u4InstID] = (_u4Datain[u4InstID] & 0xFFFFC000) >> 14;
  if(_u4BitRatVal[u4InstID] == 0 && fgIsPicMpeg1(u4InstID))
  {
#if(BIT_RATE_RES)
    return(BIT_RATE_RES);
#endif //BIT_RATE_RES
  }

  fgMarkBit = (_u4Datain[u4InstID] & 0x00002000) >> 13;
  if(fgMarkBit == 0)
  {
#if (SEQH_MKB_ERR)
    return(SEQH_MKB_ERR);
#endif //HORI_SIZE_ERR
  }

  //wVbvBufSizeVal = (_u4Datain[u4InstID] & 0x00001FF8) >> 3;
  //not EC check
  //fgCnstainPara = (_u4Datain[u4InstID] & 0x00000004) >> 2;
  //not EC check

  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 30);   // BitRat + Marker + VbvBufSize + ConPar

  _fgVerLoadIntraMatrix[u4InstID] = (_u4Datain[u4InstID] >> 31);

  u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);

  if(_fgVerLoadIntraMatrix[u4InstID]==TRUE)
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
	//		  rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ*(0.5 + 0.5 *(_u4LoadBitstreamCnt[u4InstID]%2)));
		rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));//mtk40343
    #endif
      i4VDEC_HAL_MPEG_InitBarrelShifter(0, u4InstID, &rMpegBSInitPrm);
      u4VDEC_HAL_MPEG_ShiftGetBitStream(0, u4InstID, u4Bits);
    }
    #ifdef FW_WRITE_QUANTIZATION
    vVerifyParseQtable(u4InstID);
    vVDEC_HAL_MPEG12_FW_LoadQuantMatrix(u4InstID,TRUE,(UINT32 *)(rQuantMatrix[u4InstID].ucQuantMatrix));
    #else
    vVDEC_HAL_MPEG12_LoadQuantMatrix(u4InstID, TRUE, &rQuantMatrix[u4InstID]);
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
	//		  rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ*(0.5 + 0.5 *(_u4LoadBitstreamCnt[u4InstID]%2)));
		rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));//mtk40343
    #endif
      i4VDEC_HAL_MPEG_InitBarrelShifter(1, u4InstID, &rMpegBSInitPrm);
      u4VDEC_HAL_MPEG_ShiftGetBitStream(1, u4InstID, u4Bits);
    }
  }
  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0);

  _fgVerLoadNonIntraMatrix[u4InstID] = (_u4Datain[u4InstID] >> 31);

  u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);

  if(_fgVerLoadNonIntraMatrix[u4InstID]==TRUE)
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
	//		  rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ*(0.5 + 0.5 *(_u4LoadBitstreamCnt[u4InstID]%2)));
		rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));//mtk40343
    #endif
      i4VDEC_HAL_MPEG_InitBarrelShifter(0, u4InstID, &rMpegBSInitPrm);
      u4VDEC_HAL_MPEG_ShiftGetBitStream(0, u4InstID, u4Bits);
    }
    #ifdef FW_WRITE_QUANTIZATION
    vVerifyParseQtable(u4InstID);
    printk("FW Load Qtable ! \n");
    vVDEC_HAL_MPEG12_FW_LoadQuantMatrix(u4InstID,FALSE,(UINT32 *)(rQuantMatrix[u4InstID].ucQuantMatrix));
    #else
    vVDEC_HAL_MPEG12_LoadQuantMatrix(u4InstID, FALSE, &rQuantMatrix[u4InstID]);
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
	//		  rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ*(0.5 + 0.5 *(_u4LoadBitstreamCnt[u4InstID]%2)));
		rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));//mtk40343
    #endif
      i4VDEC_HAL_MPEG_InitBarrelShifter(1, u4InstID, &rMpegBSInitPrm);
      u4VDEC_HAL_MPEG_ShiftGetBitStream(1, u4InstID, u4Bits);
    }
  }
  if (fgMPEGNextStartCode(u4InstID)==FALSE)
  {
    return(NO_START_C_ERR3);
  }

  return 0;
}

// *********************************************************************
// Function : INT32 i4GroupPicHdr(UINT32 u4InstID)
// Description : parse group of ppicture header
// Parameter : NONE
// Return    : 0, if OK
//             others, Error
// *********************************************************************
INT32 i4GroupPicHdr(UINT32 u4InstID)            //group_of_picture_header()
{
  //BOOL fgDrop;                 //BYTE drop_flag;
  BOOL fgMarker;
  //UINT32 dTmp;

  // first turn to get a packet of bitstream
  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 32);

  //fgDrop = _u4Datain[u4InstID] >> 30;

  //dTmp = (_u4Datain[u4InstID] & 0x7C000000) >> 26;
  //_tGopTime.bHour = BYTE0(dTmp);
  //dTmp = (_u4Datain[u4InstID] & 0x03F00000) >> 20;
  //_tGopTime.bMin = BYTE0(dTmp);
  //dTmp = (_u4Datain[u4InstID] & 0x0007E000) >> 13;
  //_tGopTime.bSec = BYTE0(dTmp);
  //dTmp = (_u4Datain[u4InstID] & 0x00001F80) >> 7;
  //_tGopTime.bFrm = BYTE0(dTmp);

  fgMarker = (_u4Datain[u4InstID] & 0x00080000) >> 19;
  if(fgMarker == 0)
  {
#if(GOP_MKB_ERR)
    return(GOP_MKB_ERR);
#endif //GOP_MKB_ERR
  }

  _fgVerClosedGop[u4InstID] = (_u4Datain[u4InstID] & 0x00000040)>> 6;
  _fgVerBrokenLink[u4InstID] = (_u4Datain[u4InstID] & 0x00000020) >> 5;

  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 27);
  if (fgMPEGNextStartCode(u4InstID)==FALSE)
  {
    return(NO_START_C_ERR12);
  }


  return 0;
}

// *********************************************************************
// Function : INT32 i4PicHdr(UINT32 u4InstID)
// Description : parse picture header
// Parameter : NONE
// Return    : 0, if OK
//             others, Error
// *********************************************************************
INT32 i4PicHdr(UINT32 u4InstID)
{
  //UINT16 wVbvDelay;                  //vbv_delay;

  // first turn to get a packet of bitstream
  // !!!
  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0);

  if(_u4Datain[u4InstID]!=PICTURE_START_CODE)
  {
#ifdef PICH_LOC_ERR
    return(PICH_LOC_ERR);
#endif
  }
  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 32);

  _u4TemporalRef[u4InstID] = _u4Datain[u4InstID] >> 22;

  _u4PicCdTp[u4InstID] = (_u4Datain[u4InstID] & 0x00380000L) >> 19;

  switch(_u4PicCdTp[u4InstID])
  {
  case I_TYPE:
  	  printk("<vdec> I_TYPE\n");
  	  break;
  case P_TYPE:
  	  printk("<vdec> P_TYPE\n");
	  break;
  case B_TYPE:
  	  printk("<vdec> B_TYPE\n");
  	break;
  case D_TYPE:
  	  printk("<vdec> D_TYPE\n");
	break;
  default:
  	  printk("<vdec> unknown TYPE\n");
  	break;
  }
  
  if((_u4PicCdTp[u4InstID] == 0) || (_u4PicCdTp[u4InstID] >= 5))
  {
#if(PIC_COD_TYP_ERR)
    return(PIC_COD_TYP_ERR);
#endif //PIC_COD_TYP_ERR
  }

  if((_u4PicCdTp[u4InstID] == B_TYPE) && _u4BPicIniFlag[u4InstID] == 0 && _u4BPicIniFlag0[u4InstID] == 0)
  {
    _u4BPicIniFlag[u4InstID] = 1;
  }
  else //if(_u4PicCdTp != B_TYPE)
  {
    _u4BPicIniFlag[u4InstID] = 0;
  }

  if((_u4PicCdTp[u4InstID] == B_TYPE) && _u4BPicIniFlag[u4InstID] == 0 && _u4BPicIniFlag0[u4InstID] == 0)
  {
    _u4BPicIniFlag0[u4InstID] = 1;
  }
  else if(_u4PicCdTp[u4InstID] != B_TYPE)
  {
    _u4BPicIniFlag0[u4InstID] = 0;
  }

  //wVbvDelay = (_u4Datain[u4InstID] & 0x0007FFF8) >> 3;

  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 29);

  if((_u4PicCdTp[u4InstID] == P_TYPE) || (_u4PicCdTp[u4InstID] == B_TYPE))
  {
    _ucFullPelFordVec[u4InstID] = _u4Datain[u4InstID] >> 31;
    _ucFordFCode[u4InstID] = (_u4Datain[u4InstID] & 0x70000000) >> 28;
    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 4);
  }

  if(_u4PicCdTp[u4InstID] == B_TYPE)
  {
    _ucFullPelBackVec[u4InstID] = _u4Datain[u4InstID] >> 31;
    _ucBackFCode[u4InstID] = (_u4Datain[u4InstID] & 0x70000000) >> 28;
    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 4);
  }

  while((_u4Datain[u4InstID] >> 31) == 1)
  {
    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 9);
  }

  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 1);

  if (fgMPEGNextStartCode(u4InstID)==FALSE)
  {
    return(NO_START_C_ERR13);
  }

  return 0;
}

// *********************************************************************
// Function : INT32 i4PicCodExt(UINT32 u4InstID)
// Description : parse picture coding extension
// Parameter : NONE
// Return    : 0, if OK
//             others, Error
// *********************************************************************
INT32 i4PicCodExt(UINT32 u4InstID)      //picture_coding_extension()
{
  //BOOL fgCroma420Type;                     //int chroma_420_type;
  //BOOL fgProgressiveFrm;                   //int progressive_frame;
  BOOL fgCompositeDis;                     //int composite_display_flag;
  //BOOL fgVAxis;                            //int v_axis;
  //BYTE bFldSeq;                            //int field_sequence;
  //BOOL fgSubCar;                           //int sub_carrier;
  //BYTE bBrstAmp;                           //burst_amplitude;
  //BYTE bSubCarPhase;                       //int sub_carrier_phase;

  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 32);

//  if((datain >> 28) != 0x8)
//    fprintf(outfile, "Picture_Coding_Extension_ID ERROR !!!");

  _pucfcode[u4InstID][0][0] = (_u4Datain[u4InstID] & 0x0F000000) >> 24;
  if((_pucfcode[u4InstID][0][0] == 0) || ((_pucfcode[u4InstID][0][0] >= 10) && (_pucfcode[u4InstID][0][0] < 15)))
  {
#if(F_CODE_00_ERR)
    return(F_CODE_00_ERR);
#endif //F_CODE_00_ERR
  }

  _pucfcode[u4InstID][0][1] = (_u4Datain[u4InstID] & 0x00F00000) >> 20;
  if((_pucfcode[u4InstID][0][1] == 0) || ((_pucfcode[u4InstID][0][1] >= 10) && (_pucfcode[u4InstID][0][1] < 15)))
  {
#if(F_CODE_01_ERR)
    return(F_CODE_01_ERR);
#endif //F_CODE_01_ERR
  }

  _pucfcode[u4InstID][1][0] = (_u4Datain[u4InstID] & 0x000F0000) >> 16;
  if((_pucfcode[u4InstID][1][0] == 0) || ((_pucfcode[u4InstID][1][0] >= 10) && (_pucfcode[u4InstID][1][0] < 15)))
  {
#if(F_CODE_10_ERR)
    return(F_CODE_10_ERR);
#endif //F_CODE_10_ERR
  }

  _pucfcode[u4InstID][1][1] = (_u4Datain[u4InstID] & 0x0000F000) >> 12;
  if((_pucfcode[u4InstID][1][1] == 0) || ((_pucfcode[u4InstID][1][1] >= 10) && (_pucfcode[u4InstID][1][1] < 15)))
  {
#if(F_CODE_11_ERR)
    return(F_CODE_11_ERR);
#endif //F_CODE_11_ERR
  }

  _ucIntraDcPre[u4InstID] = (_u4Datain[u4InstID] & 0x00000C00) >> 10;
  _u4PicStruct[u4InstID] = (_u4Datain[u4InstID] & 0x00000300) >> 8;
  if(_u4PicStruct[u4InstID] == 0)
  {
#if(PIC_STR_ERR)
    return(PIC_STR_ERR);
#endif //PIC_STR_ERR
  }

  _fgVerTopFldFirst[u4InstID] = (_u4Datain[u4InstID] & 0x00000080) >> 7;
  _fgVerFrmPredFrmDct[u4InstID] = (_u4Datain[u4InstID] & 0x00000040) >> 6;

  _fgVerConcealMotionVec[u4InstID] = (_u4Datain[u4InstID] & 0x00000020) >> 5;
  _fgVerQScaleType[u4InstID] = (_u4Datain[u4InstID] & 0x00000010) >> 4; //option
  _fgVerIntraVlcFmt[u4InstID] = (_u4Datain[u4InstID] & 0x00000008) >> 3;
  _fgVerAltScan[u4InstID] = (_u4Datain[u4InstID] & 0x00000004) >> 2; //option
  _fgVerRepFirstFld[u4InstID] = (_u4Datain[u4InstID] & 0x00000002) >> 1;
  //fgCroma420Type = (_u4Datain[u4InstID] & 0x00000001) >> 0;

  _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 32);

  _fgVerProgressiveFrm[u4InstID] = _u4Datain[u4InstID] >> 31;
  fgCompositeDis = (_u4Datain[u4InstID] & 0x40000000) >> 30;

  if(fgCompositeDis)
  {
    //fgVAxis = (_u4Datain[u4InstID] & 0x20000000) >> 29;
    //bFldSeq = (_u4Datain[u4InstID] & 0x1C000000) >> 26;
    //fgSubCar = (_u4Datain[u4InstID] & 0x02000000) >> 25;
    //bBrstAmp = (_u4Datain[u4InstID] & 0x01FC0000) >> 18;
    //bSubCarPhase = (_u4Datain[u4InstID] & 0x0003FC00) >> 10;

    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 22);
    if (fgMPEGNextStartCode(u4InstID)==FALSE)
    {
      return(NO_START_C_ERR14);
    }
  }
  else
  {
    _u4Datain[u4InstID] = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 2);
    if (fgMPEGNextStartCode(u4InstID)==FALSE)
    {
      return(NO_START_C_ERR15);
    }
  }
  return 0;
}

// *********************************************************************
// Function : BOOL vVerifyMp2SetPicLayerDecPrm(UINT32 u4InstID)
// Description :
// Parameter :
// Return    :
// *********************************************************************
void vVerifyMp2SetPicLayerDecPrm(UINT32 u4InstID)
{
  VDEC_INFO_MPEG2_PIC_PRM_T *ptPicPrm = &_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp2PicPrm;

  ptPicPrm->ucPicStruct = BYTE0(_u4PicStruct[u4InstID]);
  ptPicPrm->ucPicCdTp = BYTE0(_u4PicCdTp[u4InstID]);
  ptPicPrm->ucFrmPredFrmDct = BYTE0(_fgVerFrmPredFrmDct[u4InstID]);
  ptPicPrm->ucConcMotVec = BYTE0(_fgVerConcealMotionVec[u4InstID]);
  ptPicPrm->ucQScaleType = BYTE0(_fgVerQScaleType[u4InstID]);
  ptPicPrm->ucTopFldFirst = BYTE0(_fgVerTopFldFirst[u4InstID]);
  ptPicPrm->ucFullPelFordVec = _ucFullPelFordVec[u4InstID];
  ptPicPrm->ucFullPelBackVec = _ucFullPelBackVec[u4InstID];
  ptPicPrm->ucIntraVlcFmt = BYTE0(_fgVerIntraVlcFmt[u4InstID]);;
  ptPicPrm->ucIntraDcPre = _ucIntraDcPre[u4InstID];
  ptPicPrm->ucAltScan = _fgVerAltScan[u4InstID];

  ptPicPrm->pucfcode[0][0] =  _pucfcode[u4InstID][0][0];
  ptPicPrm->pucfcode[0][1] =  _pucfcode[u4InstID][0][1];
  ptPicPrm->pucfcode[1][0] =  _pucfcode[u4InstID][1][0];
  ptPicPrm->pucfcode[1][1] =  _pucfcode[u4InstID][1][1];

  ptPicPrm->ucFordFCode = _ucFordFCode[u4InstID];
  ptPicPrm->ucBackFCode = _ucBackFCode[u4InstID];

  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.u4BBufStart = _u4BBufStart[u4InstID];
}

// *********************************************************************
// Function : INT32 i4PicData(UINT32 u4InstID)
// Description : parse picture data
// Parameter : NONE
// Return    : 0, if OK
//             others, Error
// *********************************************************************
INT32 i4PicData(UINT32 u4InstID)        //picture_data()
{
  // Count MaxMbl
  if(_u4PicCdTp[u4InstID] == B_TYPE)
  {
    _u4BBufStart[u4InstID] = 0;//vCountBBufStart();
  }
  else
  {
    _u4BBufStart[u4InstID] = 0;
  }
  
  if (fgMPEGNextStartCode(u4InstID)==FALSE)
  {
    return(NO_START_C_ERR16);
  }

  return 0;
}

// *********************************************************************
// Function : BOOL fgMPEGNextStartCode(UINT32 u4InstID)
// Description : Get form bitstream the Next start code, 0x000001xx
// Parameter : None
// Return    : FALSE, if error found next start code
// *********************************************************************
BOOL fgMPEGNextStartCode(UINT32 u4InstID)
{
    UINT32 u4Retry = 0;
    UINT32 u4NextStart;
    vVDEC_HAL_MPEG_AlignRdPtr(_u4BSID[u4InstID], u4InstID, (UINT32)_pucVFifo[u4InstID], BYTE_ALIGN);
    u4NextStart = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0);
#ifdef HW_FINDSTARTCODE_SUPPORT
    while((u4NextStart>>8)!= 1)
    {
        if(u4InstID == 0)
        {
            vVDecWriteVLD(u4InstID, RW_VLD_PROC, u4VDecReadVLD(u4InstID, RW_VLD_PROC) | VLD_SSCBYTE);
        }
        u4NextStart = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0);
        if(++u4Retry > 2)
        {
            return FALSE;
        }
    }
#else
    //check until start code 0x000001XX
    while((u4NextStart>>8)!= 1)
    {
        u4NextStart = u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 8);
		u4Retry++;  // Qing Li fix here for some special bitstream

        //mtk40227 debug change
		//if(++u4Retry > MAX_RETRY_COUNT1)
        //{
        //    return FALSE;
        //}
    }
#endif
    return(TRUE);
}

// *********************************************************************
// Function : BOOL fgVerifyMpvPicDimChk(UINT32 u4InstID, UINT32 dWidth, UINT32 dHeight, BOOL fgVParser)
// Description : check if picture dimension can be supported,
//               and set DEC_FLG_FRMBUF_SZ_CHG if necessary
// Parameter : dWidth: width
//             dHeight: height
//             fgVParser: indicate if this function is called by vparser
// Return    : TRUE: dimension is supported;  FALSE: not supported
// *********************************************************************
#define HW_LIMIT_WIDTH 1920
#define HW_LIMIT_HEIGHT 1088
BOOL fgVerifyMpvPicDimChk(UINT32 u4InstID, UINT32 dWidth, UINT32 dHeight, BOOL fgVParser)
{
  // HW limitation: width <= 720, 
  //                height <= 1023
  // FW limitation: (width * height) <= (720 * 576)

  // Frame buffer size: width * (((height + 31) / 32) * 32)
  // If this size is greater than the normal size (720x576),
  // larger frame size should be used, and DEC_FLG_LRG_FRMBUF will be set
  UINT32 dHeight32Mul; // smallest 32's multiple greater or equal to dHeight

  dHeight32Mul = (dHeight + 31) / 32 * 32;

  if((dWidth * dHeight) > (HW_LIMIT_WIDTH * HW_LIMIT_HEIGHT))
  {
    return(FALSE);
  }
  else if(dWidth > HW_LIMIT_WIDTH)
  {
    return(FALSE);
  }
  else if(dWidth * dHeight32Mul > (HW_LIMIT_WIDTH * HW_LIMIT_HEIGHT))
  {
    return(FALSE);
  }
  return(TRUE);
}


#ifdef FW_WRITE_QUANTIZATION
static void vVerifyParseQtable(UINT32 u4InstID)
{
	UINT32 u4Datain, u4RemainData, u4LastQuant = 0;
	UCHAR ucMatrixIdx, ucRestIdx;
	UINT32* pu4Addr = 0;
	UINT32* pu4LenAddr = 0;
	UINT32  u4Byte0, u4Byte1, u4Byte2, u4Byte3;

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

