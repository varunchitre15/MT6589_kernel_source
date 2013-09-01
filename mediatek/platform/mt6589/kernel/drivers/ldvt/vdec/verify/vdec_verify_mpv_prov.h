#ifndef _DEC_VERIFY_MPV_PROV_H_
#define _DEC_VERIFY_MPV_PROV_H_
#include "vdec_verify_general.h"
#include "vdec_verify_macro.h"
//#include "x_bim.h"
//#include "x_os.h"
//#include "x_assert.h"
#include "../include/vdec_errcode.h"
#include "../include/vdec_info_common.h"
#include "vdec_info_verify.h"
#include "../include/vdec_info_mpeg.h"
#include "../include/vdec_info_h264.h"
#include "../include/vdec_info_wmv.h"
#include "../include/vdec_info_rm.h"
#include "../include/vdec_info_avs.h"
#include "../include/vdec_info_vp8.h"

#ifdef SATA_HDD_READ_SUPPORT
//#include  "drv_ide.h"
#endif

//extern void vDumpReg(void);

//extern void vFilledFBuf(UINT32 u4InstID, UCHAR *ptAddr, UINT32 u4Size);
//extern void vCompData4RefPicList(UCHAR *ptCompAddr, UCHAR *ptGoldAddr, UINT32 u4Size, UINT32 u4FileNum);
//extern void vConcateStr(char *strTarFileName, char *strSrcFileName, char *strAddFileName, UINT32 u4Idx);
//extern void vVerifyVDecIsrStop(UINT32 u4InstID);
#ifdef MPEG4_6589_ERROR_CONCEAL
extern UINT32 _u4TotalBitstreamLen[2];
#endif

extern UCHAR *_pucSecVFifo[2];
extern UCHAR *_pucVFifo[2];
extern UCHAR *_pucMVBuf[2];
extern UCHAR *_pucDPB[2];
extern UCHAR *_pucPredSa[2];
extern UCHAR *_pucDumpYBuf[2];
extern UCHAR *_pucDumpCBuf[2];
extern UCHAR *_pucDecWorkBuf[2];
extern UCHAR *_pucDecCWorkBuf[2];
extern UCHAR *_pucFGDatabase[2];
extern UCHAR *_pucFGSeedbase[2];
extern UCHAR *_pucFGSEISa[2];
extern UCHAR *_pucVDSCLBuf[2];
extern UCHAR *_pucVDSCLWorkBuf[2];
extern UCHAR *_pucFGTBuf[2];
extern UCHAR *_pucFileListSa[2];
extern UCHAR *_pucDumpSRAMBuf[2];
extern UCHAR *_pucGoldenFileInfoSa[2];
extern UCHAR *_pucVDSCLWork1Sa[2];
extern UCHAR *_pucVDSCLWork2Sa[2];
extern UCHAR *_pucVDSCLWork3Sa[2];
extern UCHAR *_pucVDSCLWork4Sa[2];
extern HANDLE_T _ahVDecEndSema[2];
extern UCHAR *_pucRMFrmInfoBuf[2];
extern UCHAR *_pucRMMvHwWorkBuf[2];
extern UCHAR *_pucRmVldPredWorkBuf[2];
extern UCHAR *_pucRMGoldenDataBuf[2];
extern UCHAR *_pucDumpYBuf_1[2];
extern UCHAR *_pucDumpCBuf_1[2];
extern UCHAR *_pucVDSCLBuf_1[2];
extern UCHAR *_pucRMMCOutputBufY[2];
extern UCHAR *_pucRMMCOutputBufC[2];
extern UCHAR *_pucRMReszWorkBuf[2];
extern UCHAR *_pucRMRingWorkBuf[2];
extern UCHAR *_pucRMAULikeBuf[2];
extern UCHAR *_pucRMChecksumBuf[2];
extern UCHAR* _pucRMCRCResultBuf[2];
extern UCHAR *_pucAddressSwapBuf[2];
extern BOOL _fgMVCReady[2];
extern UCHAR _ucMVCType[2];
extern BOOL _fgMVCError[2];
extern BOOL _fgMVCBaseGo;
extern BOOL _fgMVCType;
extern BOOL _fgMVCResReady[2];
extern UCHAR *_pucAVCMVBuff_Main[17];
extern UCHAR *_pucAVCMVBuff_Sub[17];

extern UCHAR *_pucRegister[2];

extern UCHAR  ucCRCBuf[32];
extern UCHAR *_pucCRCBuf[2];
#if AVC_NEW_CRC_COMPARE
extern UCHAR *_pucH264CRCYBuf[2];
#endif

#ifdef MPEG4_CRC_CMP
extern UCHAR * _pucCRCYBuf[2];
extern UCHAR *_pucCRCCBCRBuf[2];
#endif

extern INT iVFifoID[2];
extern INT iFileListSaID[2];
extern INT iDumpYBufID[2];
extern INT iDumpCBufID[2];
extern INT iCRCBufID[2];
#ifdef CAPTURE_ESA_LOG
extern UCHAR *_pucESALog[2]; 
extern UINT32 _u4ESAValue[2][8];
extern UCHAR *_pucESATotalBuf[2];
extern UINT32 _u4ESATotalLen[2];
#endif

#ifdef REG_LOG_NEW
extern UCHAR *_pucRegisterLog[2];
extern UINT32 _u4RegisterLogLen[2];
extern BOOL _fgRegLogConsole[2];
#endif

// ginny for WMV
extern UCHAR *_pucPic0Y[2];            // Video Output Pic 0, Ref0
extern UCHAR *_pucPic0C[2];            // Video Output Pic 0, Ref0
extern UCHAR *_pucPic1Y[2];            // Video Output Pic 1, Ref1
extern UCHAR *_pucPic1C[2];            // Video Output Pic 1, Ref1
extern UCHAR *_pucPic2Y[2];            // Video Output Pic 2, B
extern UCHAR *_pucPic2C[2];            // Video Output Pic 2, B
extern UCHAR *_pucPic3Y[2];            // Video Output Pic 3, B
extern UCHAR *_pucPic3C[2];            // Video Output Pic 3, B
extern UCHAR *_pucDcac[2];
extern UCHAR *_pucMv_1[2];
extern UCHAR *_pucMv_2[2];
extern UCHAR *_pucBp_1[2];
extern UCHAR *_pucBp_2[2];
extern UCHAR *_pucBp_3[2];
extern UCHAR *_pucBp_4[2];
extern UCHAR *_pucMv_3[2];
extern UCHAR *_pucMv_1_2[2];
extern UCHAR *_pucDcac_2[2];
extern UCHAR *_pucPp_1[2];
extern UCHAR *_pucPp_2[2];
extern UCHAR *_pucPpYSa[2];
extern UCHAR *_pucPpCSa[2];
extern UCHAR *_pucMp4Dcac[2];
extern UCHAR *_pucMp4Mvec[2];
extern UCHAR *_pucMp4Bmb1[2];
extern UCHAR *_pucMp4Bmb2[2];
extern UCHAR *_pucMp4Bcode[2];
#if (MPEG4_6589_SUPPORT)
//6589NEW 2.4, 2.5, 4.1
extern UCHAR *_pucMp4DataPartition[2];
extern UCHAR *_pucMp4NotCoded[2];
extern UCHAR *_pucMp4MvDirect[2];
#endif
extern UINT32 _u4DispBufIdx[2];
extern UINT32 _u4DecBufIdx[2];
extern UINT32 _u4BRefBufIdx[2];
extern UINT32 _u4FRefBufIdx[2];

extern UCHAR *_pucDcacNew[2];
extern UCHAR *_pucMvNew[2];
extern UCHAR *_pucBp0New[2];
extern UCHAR *_pucBp1New[2];
extern UCHAR *_pucBp2New[2];

//For RCV file format
extern INT32 _i4CodecVersion[2];
extern INT32 _i4RcvVersion[2];
extern INT32 _i4HeaderLen[2];
extern INT32 _iSeqHdrData1[2], _iSeqHdrData2[2], _iSeqHdrDataLen[2];
extern INT32 _iSetPos[2];
extern UINT32 _u4BitCount[2];
extern UINT32 _u4WMVDecPicNo[2];
extern UINT32 _u4WMVBitCount[2];
extern UINT32 _u4WMVByteCount[2];
extern UINT32 _new_entry_point[2];
extern UINT32 _u4VprErr[2];
extern BOOL _fgCounting[2];
extern UINT32 _u4PicHdrBits[2];

extern UINT32  u4SliceHdrCnt;
extern UINT32  u4SliceAddr[70];
// ~ginny for WMV

// ginny for MPEG
extern UINT32 _u4DIVX3Width[2];
extern UINT32 _u4DIVX3Height[2];
extern UINT32 _u4Divx3SetPos[2];
extern UINT32 _u4MpegDecPicNo[2];
extern UCHAR  _ucParW[2];
extern UCHAR  _ucParH[2];
extern UCHAR  _ucFullPelFordVec[2];
extern UCHAR  _ucFordFCode[2];
extern UCHAR  _ucFullPelBackVec[2];
extern UCHAR  _ucBackFCode[2]; 
extern UCHAR  _ucIntraDcPre[2];
extern BOOL    _fgVerAltScan[2];
extern BOOL    _fgVerQScaleType[2];
extern BOOL    _fgVerFrmPredFrmDct[2]; 
extern BOOL    _fgVerIntraVlcFmt[2];
extern BOOL    _fgVerConcealMotionVec[2];  
extern BOOL    _fgVerBrokenLink[2];
extern BOOL    _fgVerClosedGop[2];
extern UINT32 _u4UserDataCodecVersion[2];
extern UINT32 _u4UserDataBuildNumber[2];
extern UINT32 _u4TimeBase[2];
extern UINT32 _u4PicPSXOff[2];
extern BOOL    _fgVerShortVideoHeader[2];
extern BOOL    _fgSorenson[2];
extern UCHAR  _ucSourceFormat[2];
extern UCHAR  _ucVisualObjectVerid[2];
extern BOOL    _fgVerQuarterSample[2];
extern BOOL    _fgVerReversibleVlc[2];
extern BOOL    _fgVerReducedResolutionVopEnable[2];
extern UINT32 _u4BPicIniFlag[2];
extern UINT32 _u4BPicIniFlag0[2];
extern BOOL    _fgVerSeqHdr[2];
extern UCHAR  _ucMpegVer[2];
extern UINT32 _u4HSizeVal[2];
extern UINT32 _u4VSizeVal[2];
extern UCHAR  _ucAspRatInf[2];
extern UINT32 _u4FrmRatCod[2];
extern UINT32 _u4Datain[2];
extern UINT32 _u4BitRatVal[2];
extern BOOL    _fgVerLoadIntraMatrix[2];
extern BOOL    _fgVerLoadNonIntraMatrix[2];
extern BOOL    _fgVerProgressiveSeq[2];
extern UCHAR  _ucCrmaFmt[2];
extern UCHAR  _ucHSizeExt[2];
extern UCHAR  _ucVSizeExt[2];
extern BOOL    _fgVerRepFirstFld[2];
extern BOOL    _fgVerTopFldFirst[2];
extern UINT32 _u4PicStruct[2];
extern UINT32 _u4HSize[2];
extern UINT32 _u4VSize[2];
extern BOOL    _fgVerProgressiveFrm[2];
extern UINT32 _u4PicWidthMB[2];
extern UINT32 _u4TemporalRef[2];
extern BOOL    _fgVerPrevBPic[2];
extern UINT32 _u4PicCdTp[2];
extern UCHAR  _pucfcode[2][2][2];
extern UINT32 _u4BBufStart[2];
extern BOOL    _fgDec2ndFldPic[2];
extern UINT32 _u4RealHSize[2];
extern UINT32 _u4RealVSize[2];
extern UINT32 _u4BrokenLink[2];
// ~ginny for MPEG
// ginny for MPEG4
extern BOOL    _fgShortHeader[2];
extern BOOL    _fgVerDocuCamera[2];
extern UCHAR  _ucVopCdTp[2];
extern UCHAR  _ucVopQuant[2];
extern UCHAR  _ucSpriteWarpingAccuracy[2];
extern UCHAR  _ucEffectiveWarpingPoints[2];
extern BOOL    _fgVerInterlaced[2];
extern BOOL    _fgVerQuantType[2];
extern BOOL    _fgVerResyncMarkerDisable[2];
extern BOOL    _fgVerDataPartitioned[2];
extern UINT32 _u4UPicW[2];
extern UINT32 _u4UPicH[2];
extern UINT32 _u4UFrmRatCod[2];
extern BOOL    _fgVerUDx4M4v[2];
extern UCHAR  _ucVopRoundingType[2];
extern BOOL    _fgVerUDivXM4v[2];
extern UCHAR  _ucIntraDcVlcThr[2];
extern INT32   _ppiWarpingMv[2][4][2];
extern BOOL    _fgVerObmcDisable[2];
extern UINT16 _u2VopTimeIncrementResolution[2];
extern BOOL    _fgVerVopCoded0[2];
extern UCHAR  _ucFrameMode[2];
extern BOOL    _fgVerAltIAcChromDct[2];
extern BOOL    _fgVerAltIAcChromDctIdx[2];
extern BOOL    _fgVerAltIAcLumDct[2];
extern BOOL    _fgVerAltIAcLumDctIdx[2];
extern BOOL    _fgVerAltIDcDct[2];
extern BOOL    _fgVerHasSkip[2];
extern BOOL    _fgVerAltPAcDct[2];
extern BOOL    _fgVerAltPAcDctIdx[2];
extern BOOL    _fgVerAltPDcDct[2];
extern BOOL    _fgVerAltMv[2];
extern BOOL    _fgVerSwitchRounding[2];
extern BOOL    _fgVerPrefixed[2];
extern BOOL    _fgVerHistoryPrefixed[2];
extern UINT32 _u4RefPicTimeBase[2];
extern UINT32 _u4CurrDispTime[2];
extern UINT16 _u2VopTimeIncrement[2];
extern UCHAR  _ucVopTimeIncrementResolutionBits[2];
extern UINT32 _u4NextDispTime[2];
extern UINT32 _u4PrevDispTime[2];
extern BOOL    _fgVerVopReducedResolution[2];
extern BOOL    _fgVerAlternateVerticalScanFlag[2];
extern UCHAR  _ucNoOfSpriteWarpingPoints[2];
extern UCHAR  _ucSpriteEnable[2];
extern UCHAR  _ucVideoObjectLayerVerid[2];
extern UINT16 _u2FixedVopTimeIncrement[2];
extern UCHAR  _ucPrevVopCdTp[2];
extern UINT32 _u4QuanMatrixLen[2];
extern VDEC_INFO_MPEG_QANTMATRIX_T rQuantMatrix[2];
// ~ginny for MPEG4

//extern UINT32 _u4ResyncMarkMbx[2][2];
//extern UINT32 _u4ResyncMarkMby[2][2];
//extern UINT32 _u4ResyncMarkerCnt[2];

//AVS
extern UCHAR *_pucAvsPred[2];
extern UCHAR *_pucAvsMv1[2];
extern UCHAR *_pucAvsMv2[2];
//!AVS

extern UCHAR *_pucDramBusy[2];

extern VDEC_INFO_VERIFY_FILE_INFO_T _tFileListInfo[2];
extern VDEC_INFO_VERIFY_FILE_INFO_T _tFileListRecInfo[2];
extern VDEC_INFO_VERIFY_FILE_INFO_T _tInFileInfo[2];
extern VDEC_INFO_VERIFY_FILE_INFO_T _tRecFileInfo[2];
extern VDEC_INFO_VERIFY_FILE_INFO_T _tFBufFileInfo[2];
extern VDEC_INFO_VERIFY_FILE_INFO_T _tTempFileInfo[2];

extern VDEC_INFO_VERIFY_DEC_T _tVerDec[2];
extern VDEC_INFO_VERIFY_PIC_T _tVerPic[2];
extern VDEC_INFO_H264_FBUF_INFO_T _ptFBufInfo[2][17];
extern VDEC_INFO_H264_FBUF_INFO_T *_ptCurrFBufInfo[2];
extern VDEC_INFO_H264_REF_PIC_LIST_T _ptRefPicList[2][6]; // 0:P0, 1:B0, 2:B1, 3:Temp for Fields

extern UINT32 _u4PowerTestInit[2];
extern UINT32 _u4VerBitCount[2];
extern UCHAR _u4NalRefIdc[2];
extern UCHAR _ucNalUnitType[2];

#ifdef LETTERBOX_SUPPORT
extern UCHAR *_pucSettingFileSa[2];
extern UCHAR *_pucGoldenFileSa[2];
extern char *_pcLBDSettingFile[2];
extern char *_pcLBDGoldenFile[2];
extern VDEC_INFO_LBD_PRM_T _rLBDPrm[2];
#endif
// H264 Struct
extern VDEC_INFO_H264_SPS_T _rH264SPS[2][32];
extern VDEC_INFO_H264_PPS_T _rH264PPS[2][256];
extern VDEC_INFO_H264_SLICE_HDR_T _rH264SliceHdr[2];
extern VDEC_INFO_H264_SEI_T _rSEI[2];
extern VDEC_INFO_H264_FGT_PRM_T _rFGTPrm[2];
#if VDEC_MVC_SUPPORT
extern VDEC_INFO_H264_FBUF_INFO_T _rH264PrevFbInfo[2];
#endif
extern VDEC_INFO_H264_P_REF_PRM_T _arPRefPicListInfo[2];
extern VDEC_INFO_H264_B_REF_PRM_T _arBRefPicListInfo[2];
// H264 Struct

// WMV Struct
extern VDEC_INFO_MPEG_DIR_MODE_T _rDirMode[2];
extern VDEC_INFO_WMV_SEQ_PRM_T _rWMVSPS[2];
extern VDEC_INFO_WMV_ETRY_PRM_T _rWMVEPS[2];
extern VDEC_INFO_WMV_PIC_PRM_T _rWMVPPS[2];
extern VDEC_INFO_WMV_ICOMP_PRM_T _rWMVICOMPPS[2];
// ~WMV Struct

// MPEG Strcut
extern VDEC_INFO_MPEG_DIR_MODE_T _rDirMode[2];
extern VDEC_INFO_MPEG4_VOL_PRM_T _rMPEG4VolPrm[2];
extern VDEC_INFO_MPEG4_VOP_PRM_T _rMPEG4VopPrm[2];
extern VDEC_INFO_MPEG_GMC_PRM_T _rMPEG4GmcPrm[2];
// ~MPEG Struct

// Common Struct
extern VDEC_INFO_DEC_PRM_T _tVerMpvDecPrm[2]; 
extern VDEC_INFO_VERIFY_DEC_T _tVerDec[2]; 
extern VDEC_INFO_VERIFY_PIC_T _tVerPic[2];
//extern VDEC_INFO_VDSCL_PRM_T _tDownScalerPrm[2];
// ~Common Struct

//VP6 Struct
extern VDEC_INFO_VP6_FRM_HDR_T _rVDecVp6FrmHdr [2];
extern UINT32 _u4PrevfBufIdx[2];    //Previous frame buf Index
extern UINT32 _u4GoldfBufIdx[2];    //Golden frame buf Index
extern UINT32 _u4VP6ByteCount[2];
extern VDEC_INFO_VERIFY_FILE_INFO_T _rSizeFileInfo;
extern UCHAR* _pucSizeFileBuf [2];
extern VDEC_INFO_VP6_BS_INIT_PRM_T _rVp6BSPrm;
extern UCHAR *_pucVP6VLDWrapperWorkspace[2];
extern UCHAR *_pucVP6PPWrapperWorkspace[2];;
extern BOOL  _fgVP6CRCExist[2];
extern BOOL  _fgVP6SmallFlolder[2];
extern UCHAR _u1AlphaFlag[2];
extern UCHAR _u1AlphaBitstream[2];
extern UCHAR _u1AlphaDecPrmIdx[2];
//~VP6

//VP8 Struct
extern VDEC_INFO_VP8_FRM_HDR_T _rVDecVp8FrmHdr [2];
extern VDEC_INFO_VP8_FRM_HDR_T _rVDecVp8FrmHdrAddr [2];
extern UINT32 _u4VP8ByteCount[2];
extern UINT32 _u4VP8FrmSZ[2];
extern UCHAR *_pucDumpArfYBuf[2];
extern UCHAR *_pucDumpGldYBuf[2];
extern UCHAR *_pucDumpLstYBuf[2];
extern UCHAR *_pucDumpArfCBuf[2];
extern UCHAR *_pucDumpGldCBuf[2];
extern UCHAR *_pucDumpLstCBuf[2];
extern UCHAR *_pucWorkYBuf[2];
extern UCHAR *_pucWorkCBuf[2];
extern UINT32 _u4VP8LastBufIdx[2];    //Last frame buf Index
extern UINT32 _u4VP8GoldfBufIdx[2];    //Golden frame buf Index
extern UINT32 _u4VP8CurrBufIdx[2];    //Current frame buf Index
extern UINT32 _u4VP8AltBufIdx[2];    //Alternative frame buf Index
extern UCHAR *_pucVLDWrapper[2];
extern UCHAR *_pucSegIdWrapper[2];
extern UCHAR *_pucPPWrapperY[2];
extern UCHAR *_pucPPWrapperC[2];
extern UCHAR* _pucWorkYBufTmp;    //Last frame buf Index
extern UCHAR* _pucDumpArfYBufTmp;    //Last frame buf Index
extern UCHAR* _pucDumpGldYBufTmp;    //Last frame buf Index
extern UCHAR* _pucDumpLstYBufTmp;    //Last frame buf Index
extern BOOL _fgVP8DumpReg;
extern UINT8  *_pu1VP8VDecYAddrPhy;
extern UINT8  *_pu1VP8VDecYAltPhy;
extern UINT8  *_pu1VP8VDecYCurPhy;
extern UINT8  *_pu1VP8VDecYGldPhy;
extern UINT8  *_pu1VP8VDecYGoldPhy;
extern UINT8  *_pu1VP8VDecCGoldPhy;
extern UINT32  _u4MbH;
extern BOOL    _fgOpenfile;
extern UINT32 _testCnt;
extern BOOL   _fgLastMBRow;
extern UINT32 _u4ErrCnt;

//~VP8
extern UCHAR *_pucVLDWrapperWrok[2];
extern UCHAR *_pucPPWrapperWork[2];

//AVS Struct
extern VDEC_INFO_AVS_SEQ_HDR_T _rVDecAvsSeqHdr[2];
extern VDEC_INFO_AVS_PIC_HDR_T _rVDecAvsPicHdr[2];
extern UINT32 _u4AVSByteCount[2];

//~AVS


extern volatile BOOL _fgVDecComplete[2];
extern volatile BOOL _fgVDecErr[2];

extern UINT32 _u4CRCCnt[2];
extern UINT32 _u4FileCnt[2];
extern UINT32 _u4PicCnt[2];
extern UINT32 _u4GoldenYSize[2];
extern UINT32 _u4GoldenCSize[2];
extern UINT32 _u4RedecBistreamCnt[2];
//extern UINT32 _u4VDecID;
extern UINT32 _u4BSID[2];
extern UINT32 _u4TotalDecFrms[2];

extern UCHAR _ucPrevDecFld[2];

extern UINT32 _u4FileListIdx[2];
extern char*_pcBitstreamFileName[2];

extern char _bFileStr1[2][17][300];
//extern char _bFileStr1[2][9][300];
extern UINT32 _u4StringCnt[2];


extern char _bTempStr1[2][300];

extern volatile UINT32 _u4CurrPicStartAddr[2];
extern volatile UINT32 _u4PrevPtr[2];

extern UINT32 _u4SkipFrameCnt[2];
extern UINT32 _u4StartSTC[2];
extern UINT32 _u4EndSTC[2];
extern UINT32 _u4StartCompPicNum[2];
extern UINT32 _u4EndCompPicNum[2];
extern UINT32 _u4DumpRegPicNum[2];
extern UINT32 _u4MaxReDecBistreamCnt[2];
extern UINT32 _u4LoadBitstreamCnt[2];

extern UINT32 _u4DumpChksum[2][MAX_CHKSUM_NUM];
extern UINT32 _u4WmvMode[2];
extern UINT32 _u4AdobeMode[2];

#ifdef VERIFICATION_DOWN_SCALE
extern BOOL _fgVDSCLEnableRandomTest[2];
extern BOOL _fgVDSCLEnableLumaKeyTest[2];
#endif

#if defined(SW_RESET) || defined(REDEC)
extern volatile UINT32 _u4FileOffset[2];
#endif

#ifdef REDEC
extern UINT32 _u4ReDecCnt[2];
extern UINT32 _u4ReDecNum[2];
extern UINT32 _u4ReDecPicNum[2];
extern UINT32 _u4VLDPosByte[2];
extern UINT32 _u4VLDPosBit[2];
#endif
#ifdef CAPTURE_ESA_LOG
extern char _bESAStr1[2][100];
extern char _ESAFileName[2][100];
#endif

#ifdef REG_LOG_NEW
extern char _RegFileName[2][100];
#endif 

#if VMMU_SUPPORT
extern UCHAR *_pucVMMUTable[2]; 
#endif

extern UINT16 _u2AddressSwapMode[2];
// ginny start 
extern UINT32	 _u4FilePos[2];
extern UINT32 _u4CodecVer[2];
extern char _FileList_Rec[2][100];
extern char _FileList[2][100];
#endif
