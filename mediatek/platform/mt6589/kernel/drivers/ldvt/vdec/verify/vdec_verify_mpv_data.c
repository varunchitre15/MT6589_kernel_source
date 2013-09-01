//#include <mach/mt6575_typedefs.h>
#include "vdec_verify_mpv_prov.h"

//#define MT6589_VDEC_VP8 //Jackal Chen 20120202
#ifdef MPEG4_6589_ERROR_CONCEAL
UINT32 _u4TotalBitstreamLen[2];
#endif

UCHAR *_pucSecVFifo[2]; 
UCHAR *_pucVFifo[2]; 
UCHAR *_pucDPB[2];
UCHAR *_pucPredSa[2];
UCHAR *_pucMVBuf[2];
UCHAR *_pucDumpYBuf[2];
UCHAR *_pucDumpCBuf[2];
UCHAR *_pucDecWorkBuf[2];
UCHAR *_pucDecCWorkBuf[2];
UCHAR *_pucVDSCLBuf[2];
UCHAR *_pucVDSCLWorkBuf[2];
UCHAR *_pucFGTBuf[2];
UCHAR *_pucFileListSa[2];
UCHAR *_pucRegister[2];
UCHAR *_pucDumpSRAMBuf[2];
UCHAR *_pucGoldenFileInfoSa[2];
HANDLE_T _ahVDecEndSema[2];
UCHAR *_pucRMFrmInfoBuf[2];
UCHAR *_pucRMMvHwWorkBuf[2];
UCHAR *_pucRmVldPredWorkBuf[2];
UCHAR*_pucRMGoldenDataBuf[2];
UCHAR *_pucDumpYBuf_1[2];
UCHAR *_pucDumpCBuf_1[2];
UCHAR *_pucVDSCLBuf_1[2];
UCHAR *_pucRMMCOutputBufY[2];
UCHAR *_pucRMMCOutputBufC[2];
UCHAR *_pucRMReszWorkBuf[2];
UCHAR *_pucRMRingWorkBuf[2];
UCHAR *_pucRMAULikeBuf[2];
UCHAR *_pucRMChecksumBuf[2];
UCHAR* _pucRMCRCResultBuf[2];
UCHAR *_pucAddressSwapBuf[2];
UCHAR *_pucAVCMVBuff_Main[17];
UCHAR *_pucAVCMVBuff_Sub[17];

UCHAR  ucCRCBuf[32];
UCHAR *_pucCRCBuf[2];
#if AVC_NEW_CRC_COMPARE
UCHAR *_pucH264CRCYBuf[2];
#endif

#ifdef MPEG4_CRC_CMP
UCHAR * _pucCRCYBuf[2];
UCHAR *_pucCRCCBCRBuf[2];
#endif
INT iVFifoID[2];
INT iFileListSaID[2];
INT iDumpYBufID[2];
INT iDumpCBufID[2];
INT iCRCBufID[2];

#ifdef CAPTURE_ESA_LOG
UCHAR *_pucESALog[2]; 
UINT32 _u4ESAValue[2][8];
UCHAR *_pucESATotalBuf[2];
UINT32 _u4ESATotalLen[2];
#endif

#ifdef REG_LOG_NEW
UCHAR *_pucRegisterLog[2];
UINT32 _u4RegisterLogLen[2];
BOOL _fgRegLogConsole[2];
#endif

#if VMMU_SUPPORT
UCHAR *_pucVMMUTable[2]; 
#endif

// ginny for WMV
UCHAR *_pucPic0Y[2];            // Video Output Pic 0, Ref0
UCHAR *_pucPic0C[2];            // Video Output Pic 0, Ref0
UCHAR *_pucPic1Y[2];            // Video Output Pic 1, Ref1
UCHAR *_pucPic1C[2];            // Video Output Pic 1, Ref1
UCHAR *_pucPic2Y[2];            // Video Output Pic 2, B
UCHAR *_pucPic2C[2];            // Video Output Pic 2, B
UCHAR *_pucPic3Y[2];            // Video Output Pic 3, B
UCHAR *_pucPic3C[2];            // Video Output Pic 3, B
UCHAR *_pucDcac[2];
UCHAR *_pucMv_1[2];
UCHAR *_pucMv_2[2];
UCHAR *_pucBp_1[2];
UCHAR *_pucBp_2[2];
UCHAR *_pucBp_3[2];
UCHAR *_pucBp_4[2];
UCHAR *_pucMv_3[2];
UCHAR *_pucMv_1_2[2];
UCHAR *_pucDcac_2[2];
UCHAR *_pucPp_1[2];
UCHAR *_pucPp_2[2];
UCHAR *_pucPpYSa[2];
UCHAR *_pucPpCSa[2];
UCHAR *_pucMp4Dcac[2];
UCHAR *_pucMp4Mvec[2];
UCHAR *_pucMp4Bmb1[2];
UCHAR *_pucMp4Bmb2[2];
UCHAR *_pucMp4Bcode[2];
#if (MPEG4_6589_SUPPORT)
//6589NEW 2.4, 2.5, 4.1
UCHAR *_pucMp4DataPartition[2];
UCHAR *_pucMp4NotCoded[2];
UCHAR *_pucMp4MvDirect[2];
#endif
UINT32 _u4DispBufIdx[2];
UINT32 _u4DecBufIdx[2];
UINT32 _u4BRefBufIdx[2];
UINT32 _u4FRefBufIdx[2];
UINT32 _u4BBufIdx[2];

UCHAR *_pucDcacNew[2];
UCHAR *_pucMvNew[2];
UCHAR *_pucBp0New[2];
UCHAR *_pucBp1New[2];
UCHAR *_pucBp2New[2];

//For RCV file format
INT32 _i4CodecVersion[2];
INT32 _i4RcvVersion[2];
INT32 _i4HeaderLen[2];
INT32 _iSeqHdrData1[2], _iSeqHdrData2[2], _iSeqHdrDataLen[2];
INT32 _iSetPos[2];
UINT32 _u4BitCount[2];
UINT32 _u4WMVDecPicNo[2];
UINT32 _u4WMVBitCount[2];
UINT32 _u4WMVByteCount[2];
UINT32 _new_entry_point[2];
UINT32 _u4VprErr[2];
BOOL _fgCounting[2];
UINT32 _u4PicHdrBits[2];

UINT32  u4SliceHdrCnt;
UINT32  u4SliceAddr[70];
// ~ginny for WMV

// ginny for MPEG
UINT32 _u4DIVX3Width[2];
UINT32 _u4DIVX3Height[2];
UINT32 _u4Divx3SetPos[2];
UINT32 _u4MpegDecPicNo[2];
UINT32 _u4BitCount[2];
UCHAR  _ucParW[2];
UCHAR  _ucParH[2];
UCHAR  _ucFullPelFordVec[2];
UCHAR  _ucFordFCode[2];
UCHAR  _ucFullPelBackVec[2];
UCHAR  _ucBackFCode[2]; 
UCHAR  _ucIntraDcPre[2];
BOOL    _fgVerAltScan[2];
BOOL    _fgVerQScaleType[2];
BOOL    _fgVerFrmPredFrmDct[2]; 
BOOL    _fgVerIntraVlcFmt[2];
BOOL    _fgVerConcealMotionVec[2];  
BOOL    _fgVerBrokenLink[2];
BOOL    _fgVerClosedGop[2];
UINT32 _u4UserDataCodecVersion[2];
UINT32 _u4UserDataBuildNumber[2];
UINT32 _u4TimeBase[2];
UINT32 _u4PicPSXOff[2];
BOOL    _fgVerShortVideoHeader[2];
BOOL    _fgSorenson[2];
UCHAR  _ucSourceFormat[2];
UCHAR  _ucVisualObjectVerid[2];
BOOL    _fgVerQuarterSample[2];
BOOL    _fgVerReversibleVlc[2];
BOOL    _fgVerReducedResolutionVopEnable[2];
UINT32 _u4BPicIniFlag[2];
UINT32 _u4BPicIniFlag0[2];
BOOL    _fgVerSeqHdr[2];
UCHAR  _ucMpegVer[2];
UINT32 _u4HSizeVal[2];
UINT32 _u4VSizeVal[2];
UCHAR  _ucAspRatInf[2];
UINT32 _u4FrmRatCod[2];
UINT32 _u4Datain[2];
UINT32 _u4BitRatVal[2];
BOOL    _fgVerLoadIntraMatrix[2];
BOOL    _fgVerLoadNonIntraMatrix[2];
BOOL    _fgVerProgressiveSeq[2];
UCHAR  _ucCrmaFmt[2];
UCHAR  _ucHSizeExt[2];
UCHAR  _ucVSizeExt[2];
BOOL    _fgVerRepFirstFld[2];
BOOL    _fgVerTopFldFirst[2];
UINT32 _u4PicStruct[2];
UINT32 _u4HSize[2];
UINT32 _u4VSize[2];
BOOL    _fgVerProgressiveFrm[2];
UINT32 _u4PicWidthMB[2];
UINT32 _u4TemporalRef[2];
BOOL    _fgVerPrevBPic[2];
UINT32 _u4PicCdTp[2];
UCHAR  _pucfcode[2][2][2];
UINT32 _u4BBufStart[2];
BOOL    _fgDec2ndFldPic[2];
UINT32 _u4RealHSize[2];
UINT32 _u4RealVSize[2];
UINT32 _u4BrokenLink[2]={0};
// ~ginny for MPEG
// ginny for MPEG4
BOOL    _fgShortHeader[2];
BOOL    _fgVerDocuCamera[2];
UCHAR  _ucVopCdTp[2];
UCHAR  _ucVopQuant[2];
UCHAR  _ucSpriteWarpingAccuracy[2];
UCHAR  _ucEffectiveWarpingPoints[2];
BOOL    _fgVerInterlaced[2];
BOOL    _fgVerQuantType[2];
BOOL    _fgVerResyncMarkerDisable[2];
BOOL    _fgVerDataPartitioned[2];
UINT32 _u4UPicW[2];
UINT32 _u4UPicH[2];
UINT32 _u4UFrmRatCod[2];
BOOL    _fgVerUDx4M4v[2];
UCHAR  _ucVopRoundingType[2];
BOOL    _fgVerUDivXM4v[2] = {1, 1};
UCHAR  _ucIntraDcVlcThr[2];
INT32   _ppiWarpingMv[2][4][2];
BOOL    _fgVerObmcDisable[2];
UINT16 _u2VopTimeIncrementResolution[2];
BOOL    _fgVerVopCoded0[2];
UCHAR  _ucFrameMode[2];
BOOL    _fgVerAltIAcChromDct[2];
BOOL    _fgVerAltIAcChromDctIdx[2];
BOOL    _fgVerAltIAcLumDct[2];
BOOL    _fgVerAltIAcLumDctIdx[2];
BOOL    _fgVerAltIDcDct[2];
BOOL    _fgVerHasSkip[2];
BOOL    _fgVerAltPAcDct[2];
BOOL    _fgVerAltPAcDctIdx[2];
BOOL    _fgVerAltPDcDct[2];
BOOL    _fgVerAltMv[2];
BOOL    _fgVerSwitchRounding[2];
BOOL    _fgVerPrefixed[2];
BOOL    _fgVerHistoryPrefixed[2];
UINT32 _u4RefPicTimeBase[2];
UINT32 _u4CurrDispTime[2];
UINT16 _u2VopTimeIncrement[2];
UCHAR  _ucVopTimeIncrementResolutionBits[2];
UINT32 _u4NextDispTime[2];
UINT32 _u4PrevDispTime[2];
BOOL    _fgVerVopReducedResolution[2];
BOOL    _fgVerAlternateVerticalScanFlag[2];
UCHAR  _ucNoOfSpriteWarpingPoints[2];
UCHAR  _ucSpriteEnable[2];
UCHAR  _ucVideoObjectLayerVerid[2];
UINT16 _u2FixedVopTimeIncrement[2];
UCHAR  _ucPrevVopCdTp[2];
UINT32 _u4QuanMatrixLen[2];
VDEC_INFO_MPEG_QANTMATRIX_T rQuantMatrix[2];
// ~ginny for MPEG4

//UINT32 _u4ResyncMarkMbx[2][2];
//UINT32 _u4ResyncMarkMby[2][2];
//UINT32 _u4ResyncMarkerCnt[2];

//AVS
UCHAR *_pucAvsPred[2];
UCHAR *_pucAvsMv1[2];
UCHAR *_pucAvsMv2[2];
//~AVS

UCHAR *_pucDramBusy[2];

UCHAR *_pucFGDatabase[2];
UCHAR *_pucFGSeedbase[2];
UCHAR *_pucFGSEISa[2];
UCHAR *_pucVDSCLWork1Sa[2];
UCHAR *_pucVDSCLWork2Sa[2];
UCHAR *_pucVDSCLWork3Sa[2];
UCHAR *_pucVDSCLWork4Sa[2];

UINT32 _u4CRCCnt[2];
UINT32 _u4FileCnt[2];
UINT32 _u4PicCnt[2];
UINT32 _u4GoldenYSize[2];
UINT32 _u4GoldenCSize[2];
UINT32 _u4RedecBistreamCnt[2];
//UINT32 _u4VDecID;
UINT32 _u4BSID[2];
// ginny start
UINT32 _u4CodecVer[2]; 
UCHAR _ucMVCType[2];
BOOL _fgMVCReady[2];
BOOL _fgMVCError[2];
BOOL _fgMVCBaseGo;
BOOL _fgMVCType;
BOOL _fgMVCResReady[2];
// Common Struct
VDEC_INFO_DEC_PRM_T _tVerMpvDecPrm[2]; 
VDEC_INFO_VERIFY_DEC_T _tVerDec[2]; 
VDEC_INFO_VERIFY_PIC_T _tVerPic[2];
//VDEC_INFO_VDSCL_PRM_T _tDownScalerPrm[2];
// ~Common Struct

#ifdef LETTERBOX_SUPPORT
UCHAR *_pucSettingFileSa[2];
UCHAR *_pucGoldenFileSa[2];
char *_pcLBDSettingFile[2];
char *_pcLBDGoldenFile[2];
VDEC_INFO_LBD_PRM_T _rLBDPrm[2];
#endif
// H264 Struct
VDEC_INFO_H264_FBUF_INFO_T _ptFBufInfo[2][17];
VDEC_INFO_H264_FBUF_INFO_T *_ptCurrFBufInfo[2];
VDEC_INFO_H264_REF_PIC_LIST_T _ptRefPicList[2][6];
VDEC_INFO_H264_SPS_T _rH264SPS[2][32];
VDEC_INFO_H264_PPS_T _rH264PPS[2][256];
VDEC_INFO_H264_SLICE_HDR_T _rH264SliceHdr[2];
VDEC_INFO_H264_SEI_T _rSEI[2];
VDEC_INFO_H264_FGT_PRM_T _rFGTPrm[2];
#if VDEC_MVC_SUPPORT
VDEC_INFO_H264_FBUF_INFO_T _rH264PrevFbInfo[2];
#endif
VDEC_INFO_H264_P_REF_PRM_T _arPRefPicListInfo[2];
VDEC_INFO_H264_B_REF_PRM_T _arBRefPicListInfo[2];
// ~H264 Struct

// WMV Struct
VDEC_INFO_WMV_SEQ_PRM_T _rWMVSPS[2];
VDEC_INFO_WMV_ETRY_PRM_T _rWMVEPS[2];
VDEC_INFO_WMV_PIC_PRM_T _rWMVPPS[2];
VDEC_INFO_WMV_ICOMP_PRM_T _rWMVICOMPPS[2];
// ~WMV Struct

// MPEG Strcut
VDEC_INFO_MPEG_DIR_MODE_T _rDirMode[2];
VDEC_INFO_MPEG4_VOL_PRM_T _rMPEG4VolPrm[2];
VDEC_INFO_MPEG4_VOP_PRM_T _rMPEG4VopPrm[2];
VDEC_INFO_MPEG_GMC_PRM_T _rMPEG4GmcPrm[2];
// ~MPEG Struct

// Record File Struct
VDEC_INFO_VERIFY_FILE_INFO_T _tFBufFileInfo[2];
VDEC_INFO_VERIFY_FILE_INFO_T _tFileListInfo[2];
VDEC_INFO_VERIFY_FILE_INFO_T _tFileListRecInfo[2];
VDEC_INFO_VERIFY_FILE_INFO_T _tInFileInfo[2];
VDEC_INFO_VERIFY_FILE_INFO_T _tRecFileInfo[2];
VDEC_INFO_VERIFY_FILE_INFO_T _tTempFileInfo[2];
// ~Record File Struct

//VP6 Struct
VDEC_INFO_VP6_FRM_HDR_T _rVDecVp6FrmHdr [2];
UINT32 _u4PrevfBufIdx[2];    //Previous frame buf Index
UINT32 _u4GoldfBufIdx[2];    //Golden frame buf Index
UINT32 _u4VP6ByteCount[2];
VDEC_INFO_VERIFY_FILE_INFO_T _rSizeFileInfo;
UCHAR *_pucSizeFileBuf [2];
VDEC_INFO_VP6_BS_INIT_PRM_T _rVp6BSPrm;
UCHAR *_pucVP6VLDWrapperWorkspace[2] = {NULL, NULL};
UCHAR *_pucVP6PPWrapperWorkspace[2] = {NULL, NULL};
BOOL  _fgVP6CRCExist[2] = {FALSE, FALSE};
BOOL  _fgVP6SmallFlolder[2] = {FALSE, FALSE};
UCHAR _u1AlphaFlag[2] = {0, 0};
UCHAR _u1AlphaBitstream[2] = {0, 0};
UCHAR _u1AlphaDecPrmIdx[2] = {0, 0};
//~VP6

//VP8 Struct
VDEC_INFO_VP8_FRM_HDR_T _rVDecVp8FrmHdr [2];
VDEC_INFO_VP8_FRM_HDR_T _rVDecVp8FrmHdrAddr [2];
UINT32 _u4VP8ByteCount[2];
UINT32 _u4VP8FrmSZ[2];
UCHAR *_pucDumpArfYBuf[2];
UCHAR *_pucDumpGldYBuf[2];
UCHAR *_pucDumpLstYBuf[2];
UCHAR *_pucDumpArfCBuf[2];
UCHAR *_pucDumpGldCBuf[2];
UCHAR *_pucDumpLstCBuf[2];
UCHAR *_pucWorkYBuf[2];
UCHAR *_pucWorkCBuf[2];
UINT32 _u4VP8LastBufIdx[2];    //Last frame buf Index
UINT32 _u4VP8GoldfBufIdx[2];    //Golden frame buf Index
UINT32 _u4VP8CurrBufIdx[2];    //Current frame buf Index
UINT32 _u4VP8AltBufIdx[2];    //Alternative frame buf Index
UCHAR *_pucVLDWrapper[2];
UCHAR *_pucSegIdWrapper[2];
UCHAR *_pucPPWrapperY[2];
UCHAR *_pucPPWrapperC[2];

UCHAR* _pucWorkYBufTmp;    //Last frame buf Index
UCHAR* _pucDumpArfYBufTmp;    //Last frame buf Index
UCHAR* _pucDumpGldYBufTmp;    //Last frame buf Index
UCHAR* _pucDumpLstYBufTmp;    //Last frame buf Index
BOOL _fgVP8DumpReg=0;
UINT8  *_pu1VP8VDecYAddrPhy = NULL;
UINT8  *_pu1VP8VDecYAltPhy = NULL;
UINT8  *_pu1VP8VDecYCurPhy = NULL;
UINT8  *_pu1VP8VDecYGldPhy = NULL;
UINT8  *_pu1VP8VDecYGoldPhy = NULL;
UINT8  *_pu1VP8VDecCGoldPhy = NULL;
UINT32  _u4MbH=0;
BOOL    _fgOpenfile = FALSE;
UINT32 _testCnt=0;
BOOL   _fgLastMBRow =FALSE;
UINT32 _u4ErrCnt=0;

//~VP8
UCHAR *_pucVLDWrapperWrok[2];
UCHAR *_pucPPWrapperWork[2];

//AVS Struct
VDEC_INFO_AVS_SEQ_HDR_T _rVDecAvsSeqHdr[2];
VDEC_INFO_AVS_PIC_HDR_T _rVDecAvsPicHdr[2];
UINT32 _u4AVSByteCount[2];
//~AVS

UINT32 _u4PowerTestInit[2];
UINT32 _u4VerBitCount[2];
UINT32 _u4TotalDecFrms[2];

UCHAR _u4NalRefIdc[2];
UCHAR _ucNalUnitType[2];

UCHAR _ucPrevDecFld[2];

volatile BOOL _fgVDecComplete[2];
volatile BOOL _fgVDecErr[2];

volatile UINT32 _u4CurrPicStartAddr[2];
volatile UINT32 _u4PrevPtr[2];

#ifdef VERIFICATION_DOWN_SCALE
BOOL _fgVDSCLEnableRandomTest[2];
BOOL _fgVDSCLEnableLumaKeyTest[2];
#endif
#ifdef REDEC
UINT32 _u4ReDecCnt[2];
UINT32 _u4ReDecNum[2];
UINT32 _u4ReDecPicNum[2];
UINT32 _u4VLDPosByte[2];
UINT32 _u4VLDPosBit[2];
#endif
  
UINT16 _u2AddressSwapMode[2];
  
#if defined(SW_RESET) || defined(REDEC)
volatile UINT32 _u4FileOffset[2];
#endif

UINT32 _u4SkipFrameCnt[2];
UINT32 _u4StartSTC[2];
UINT32 _u4EndSTC[2];
UINT32 _u4StartCompPicNum[2];
UINT32 _u4EndCompPicNum[2];
UINT32 _u4DumpRegPicNum[2];
UINT32 _u4MaxReDecBistreamCnt[2];
UINT32 _u4LoadBitstreamCnt[2];

UINT32 _u4DumpChksum[2][MAX_CHKSUM_NUM];
UINT32 _u4WmvMode[2];
UINT32 _u4AdobeMode[2];

UINT32 _u4FileListIdx[2];
UINT32 _u4FilePos[2];
char *_pcBitstreamFileName[2];
UINT32 _u4StringCnt[2];


char _bFileStr1[2][17][300] = {{"\0","\0","\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0"},
                                          {"\0","\0","\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0"}};
//char _bFileStr1[2][9][300] = {{"\0","\0","\0", "\0", "\0", "\0", "\0", "\0", "\0"},
//                                          {"\0","\0","\0", "\0", "\0", "\0", "\0", "\0", "\0"}};


char _bTempStr1[2][300] = {"\0","\0"};
#ifdef CAPTURE_ESA_LOG
char _bESAStr1[2][100] = {" ,NBM_DLE_NUM,ESA_REQ_DATA_NUM,MC_REQ_DATA_NUM,MC_MBX,MC_MBY,CYC_SYS,INTRA_CNT,LAT_BUF_BYPASS,\0",
	                                  " ,NBM_DLE_NUM,ESA_REQ_DATA_NUM,MC_REQ_DATA_NUM,MC_MBX,MC_MBY,CYC_SYS,INTRA_CNT,LAT_BUF_BYPASS,\0"};
#ifdef MT6589_VDEC_VP8  //Jackal Chen 20120202
char _ESAFileName[2][100] = {"Z:\\ESA_LOG.csv\0","Z:\\ESA_LOG1.csv\0"};
#else
char _ESAFileName[2][100] = {"D:\\Sandbox\\ESA_LOG.csv\0","/mnt/udiska/ESA_LOG1.csv\0"};
#endif
#endif

#ifdef REG_LOG_NEW
char _RegFileName[2][100] = {"D:\\Sandbox\\RegDump.txt\0","D:\\Sandbox\\RegDump.txt\0"};
#endif

#if defined(FPGA_FOR_MPEG2)
char _FileList_Rec[2][100]= {"\\\\emu264\\f\\ChkFolder\\8530_filelist_rec_mpeg2.txt","\\\\emu264\\f\\ChkFolder\\8530_filelist_rec_mpeg2_1.txt"};
char _FileList[2][100]= {"\\\\emu264\\f\\ChkFolder\\8530_filelist_mpeg2.txt","\\\\emu264\\f\\ChkFolder\\8530_filelist_mpeg2_1.txt"};
#elif defined(FPGA_FOR_H264)
char _FileList_Rec[2][100]= {"\\\\emu264\\f\\ChkFolder\\8530_filelist_rec_h264.txt","\\\\emu264\\f\\ChkFolder\\8530_filelist_rec_h264_1.txt"};
char _FileList[2][100]= {"\\\\emu264\\f\\ChkFolder\\8530_filelist_h264.txt","\\\\emu264\\f\\ChkFolder\\8530_filelist_h264_1.txt"};
#else

#ifdef SATA_HDD_FS_SUPPORT
#ifdef MT6589_VDEC_VP8  //Jackal Chen 20120202
char _FileList_Rec[2][100]= {"Z:\\VDec0_filelist_rec.txt","/mnt/udiska/VDec1_filelist_rec.txt"};
#else
char _FileList_Rec[2][100]= {"D:\\Sandbox\\VDec0_filelist_rec.txt","/mnt/udiska/VDec1_filelist_rec.txt"};
#endif
//char _FileList[2][100]= {"list_normal_test1.txt","list_normal_test2.txt"};
#ifdef MT6589_VDEC_VP8  //Jackal Chen 20120202
char _FileList[2][100]= {"Z:\\6589_ver_list.txt\0", "/mnt/udiska/8320_ver_list.txt\0"};
#else
char _FileList[2][100]= {"/mnt/sdcard/8320_ver_list.txt\0", "/mnt/udiska/8320_ver_list.txt\0"};
#endif
#elif defined (IDE_READ_SUPPORT)
//char _FileList_Rec[2][100]= {"C:\\ChkFolder\\VDec0_filelist_rec.txt","C:\\ChkFolder\\VDec1_filelist_rec.txt"};    //Only for RM Decoder
//char _FileList[2][100]= {"C:\\ChkFolder\\list_normal_test1.txt","C:\\ChkFolder\\list_normal_test2.txt"};             //Only for RM Decoder
char _FileList_Rec[2][100]= {"D:\\ChkFolder\\VDec0_filelist_rec.txt","D:\\ChkFolder\\VDec1_filelist_rec.txt"};
char _FileList[2][100]= {"D:\\ChkFolder\\list_normal_test1.txt","D:\\ChkFolder\\list_normal_test2.txt"};
#else
//char _FileList_Rec[2][100]= {"D:\\ChkFolder\\VDec0_filelist_rec.txt","D:\\ChkFolder\\VDec1_filelist_rec.txt"};    //Only for RM Decoder
//char _FileList[2][100]= {"D:\\ChkFolder\\list_normal_test1.txt","D:\\ChkFolder\\list_normal_test2.txt"};             //Only for RM Decoder
char _FileList_Rec[2][100]= {"d:\\ChkFolder\\VDec0_filelist_rec.txt","F:\\ChkFolder\\VDec1_filelist_rec.txt"}; 
char _FileList[2][100]= {"d:\\ChkFolder\\list_normal_test1.txt","F:\\ChkFolder\\list_normal_test2.txt"}; 
#endif

#endif


