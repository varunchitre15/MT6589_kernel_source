#ifndef _VDEC_VERIFY_MACRO_H_
#define _VDEC_VERIFY_MACRO_H_
//#include "mpv_prov.h"


#define AVI_DIVX_FULL_SUPPORT

#ifdef AVI_DIVX_FULL_SUPPORT
#define fgPureIsoM4v()     FALSE
#define fgDisableDivXSp()  FALSE
#define fgDisableDRM()     FALSE
#else
#define fgPureIsoM4v()     ((_dwBStatus & (0x1 << 20)) != 0)
#define fgDisableDivXSp()  ((_dwBStatus & (0x1 << 20)) != 0)
#define fgDisableDRM()     ((_dwBStatus & (0x1 << 20)) != 0)
#endif

// BYTE 0 first       [BYTE0, BYTE1, BYTE2, BYTE3]
#define BYTE3(arg)          (*((BYTE *)&(arg) + 3))
#define BYTE2(arg)          (*((BYTE *)&(arg) + 2))
#define BYTE1(arg)          (*((BYTE *)&(arg) + 1))
#define BYTE0(arg)          (* (BYTE *)&(arg))

#define fgIsVerifyMpeg1(u4InstID)    (_ucMpegVer[u4InstID] == VDEC_MPEG1)
#define fgIsVerifyMpeg2(u4InstID)    (_ucMpegVer[u4InstID] == VDEC_MPEG2)
#define fgIsVerifyMpeg4(u4InstID)    (_ucMpegVer[u4InstID] == VDEC_MPEG4)
#define fgIsDivX3(u4InstID)    (_ucMpegVer[u4InstID] == VDEC_DIVX3)
#define fgIsPicMpeg1(u4InstID) (_tVerPic[u4InstID].ucMpegVer == VDEC_MPEG1)
#define fgIsPicMpeg2(u4InstID) (_tVerPic[u4InstID].ucMpegVer == VDEC_MPEG2)
#define fgIsPicMPeg4(u4InstID) (_tVerPic[u4InstID].ucMpegVer == VDEC_MPEG4)


#define fgIsRefPic(u4InstID) ((_u4NalRefIdc[u4InstID] > 0))
#define fgIsIDRPic(u4InstID) ((_ucNalUnitType[u4InstID] == IDR_SLICE))
#define fgIsFrmPic(u4InstID) ((_tVerMpvDecPrm[u4InstID].ucPicStruct == FRAME))
#define fgMPEGIsFrmPic(u4InstID) ((_tVerMpvDecPrm[u4InstID].ucPicStruct == FRM_PIC))
#define fgWMVIsFrmPic(u4InstID) ((_rWMVPPS[u4InstID].ucFrameCodingMode != INTERLACEFIELD))
#define fgIsISlice(bType) ((bType == I_Slice) ||(bType == SI_Slice) || (bType == I_Slice_ALL))
#define fgIsPSlice(bType) ((bType == P_Slice) ||(bType == SP_Slice) || (bType == P_Slice_ALL))
#define fgIsBSlice(bType) ((bType == B_Slice) || (bType == B_Slice_ALL))

#define fgIsMonoPic(u4InstID) ((_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecH264DecPrm.prSPS->u4ChromaFormatIdc == 0))

#define DEC_FLAG_CHG_FBUF    (0x1U << 0)

#define vSetVerDecBuf(u4InstID, arg)       _u4DecBufIdx[u4InstID] = (arg)
#define vSetVerBRefBuf(u4InstID,arg)      _u4BRefBufIdx[u4InstID] = (arg)
#define vSetVerFRefBuf(u4InstID,arg)      _u4FRefBufIdx[u4InstID] = (arg)
#define vSetVerBBuf(u4InstID, arg)         _u4BBufIdx[u4InstID] = (arg)
#define vSetDecFlag(u4InstID, arg)         (_tVerDec[u4InstID].u4DecFlag |= arg)
#define fgIsDecFlagSet(u4InstID, arg)     (_tVerDec[u4InstID].u4DecFlag & (arg))
#define vClrDecFlag(u4InstID, arg)           (_tVerDec[u4InstID].u4DecFlag &= (~arg))

#define fgIsNonRefFBuf(u4InstID, arg) ((_ptFBufInfo[u4InstID][arg].ucFBufRefType == NREF_PIC) && (_ptFBufInfo[u4InstID][arg].ucTFldRefType == NREF_PIC) && (_ptFBufInfo[u4InstID][arg].ucBFldRefType == NREF_PIC))
#endif
