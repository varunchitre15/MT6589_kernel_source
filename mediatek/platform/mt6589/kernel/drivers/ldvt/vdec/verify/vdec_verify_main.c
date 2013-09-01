/**********************************************************************/
/***************                                       ****************/
/***************                                       ****************/
/***************   Description : MT8118 MTKPrintf      ****************/
/***************                 Procedure             ****************/
/***************                                       ****************/
/***************       Company : MediaTek Inc.         ****************/
/***************    Programmer : Ted Hu                ****************/
/**********************************************************************/
#define _VDEC_VERIFY_MAIN_C_
//#include "drv_config.h"
//#include "x_ckgen.h"
//#include "x_debug.h"


#include "vdec_verify_file_common.h"
#include "vdec_verify_filesetting.h"
//#include "vdec_ide.h"
#include "vdec_verify_mm_map.h"
#include "vdec_verify_mpv_prov.h"
#include "vdec_verify_vdec.h"
#include "vdec_verify_vdecode.h"
#include "../hal/vdec_hal_if_common.h"
#include "vdec_verify_vparser_rm.h"
#include "vdec_verify_vparser_vp6.h"

#if (!CONFIG_DRV_LINUX)
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#endif
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/delay.h>



extern int rand(void);

#ifdef SATA_HDD_FS_SUPPORT
#include "sata_fs_io.h"
//#include "usb_io.h"
#endif       
//#include "x_hal_ic.h"
//#include "x_hal_1176.h"

void vVDecGconEnable(void);
void vVDecClockSelect(void);
void vDrmaBusySet (UINT32  u4InstID);
BOOL _fgSemaCreated[2]={FALSE,FALSE};
BOOL _fgMemAllocate=FALSE;

// *********************************************************************
// Function    : void vVDecReDecSetting(UINT32 u4InstID)
// Description : System entry point
// Parameter   : None
// Return      : None
// *********************************************************************
void vVDecReDecSetting(UINT32  u4InstID, UINT32 u4ReDecPicNum, UINT32 u4ReDecTimes)
{
#ifdef REDEC 
    _u4ReDecPicNum[u4InstID] = u4ReDecPicNum;
    _u4ReDecNum[u4InstID] = u4ReDecTimes;
#endif    
}

extern char strSrcFileNameExt_FileList[];
extern char strRMSrcFileName[];
extern char strGoldenFileNameExt_FileList[];
extern char strRMGoldenFileName[];


BOOL fgVDecVerify_PrepareInfo_RM(UINT32 u4InstID)
{
  BOOL fgOpen;
  char strMessage[512];
  char strEmuFileName[512];

  //Read RM Frame Info from PC  
  _tInFileInfo[u4InstID].fgGetFileInfo = TRUE;
  _tInFileInfo[u4InstID].pucTargetAddr = _pucRMFrmInfoBuf[u4InstID];
  _tInFileInfo[u4InstID].u4TargetSz = RM_FRMINFO_SZ;    
  _tInFileInfo[u4InstID].u4FileLength = 0;    
  _tInFileInfo[u4InstID].u4FileOffset = 0;

  sprintf(strMessage, "%s: ", _bFileStr1[u4InstID][RM_FRMINFO_INDEX]);
  vVDecOutputDebugString(strMessage);

  fgOpen = fgOpenFile(u4InstID, _bFileStr1[u4InstID][RM_FRMINFO_INDEX],"r+b", &_tInFileInfo[u4InstID]);
  if(fgOpen == FALSE)
  {
    vVDecOutputDebugString("Open bit-stream fail\n");
    strcpy(_tFileListRecInfo[u4InstID].bFileName,_FileList_Rec[u4InstID]);
    sprintf(strMessage, "%s", "Open bit-stream fail\n");
    fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
    return FALSE;
  }

  if (_u4StartCompPicNum[u4InstID] > 0)
  {
    _u4FileCnt[u4InstID] = u4RM_FindIPic(u4InstID, _u4StartCompPicNum[u4InstID]);
	_u4StartCompPicNum[u4InstID] = _u4FileCnt[u4InstID];
  }

  #ifdef RM_CRCCHECK_ENABLE
  sprintf(strEmuFileName, "%s/%s%07d%s", _bFileStr1[u4InstID][RM_GOLDENPATH_INDEX], 
                                                             strRMGoldenFileName,
                                                            _u4FileCnt[u4InstID],
                                                            strGoldenFileNameExt_FileList);
  #else //RM_CRCCHECK_ENABLE
    {   
      UINT32 u4PicCnt = _u4FileCnt[u4InstID];
      if (u4PicCnt/5000 > 0)
      {
          sprintf(strEmuFileName, "%s%05d/%s%07d%s", _bFileStr1[u4InstID][RM_GOLDENPATH_INDEX], 
                                                     u4PicCnt/5000 * 5000,
                                                     strRMGoldenFileName,
                                                     _u4FileCnt[u4InstID],
                                                     strGoldenFileNameExt_FileList);          
      }
      else
      {   
          //sprintf(strEmuFileName, "%s/%s%07d%s", _bFileStr1[u4InstID][RM_GOLDENPATH_INDEX], 
          sprintf(strEmuFileName, "%s%s%07d%s", _bFileStr1[u4InstID][RM_GOLDENPATH_INDEX], 
                                                                     strRMGoldenFileName,
                                                                    _u4FileCnt[u4InstID],
                                                                    strGoldenFileNameExt_FileList);
      }
    }
  #endif //RM_CRCCHECK_ENABLE

    //Update File Name
  #ifdef SATA_HDD_FS_SUPPORT
    { 
        UINT32 u4PicCnt = _u4FileCnt[u4InstID];
        if (u4PicCnt/5000 > 0)
        {
              sprintf(_bFileStr1[u4InstID][1], "%s%05d/%s%07d%s", _bFileStr1[u4InstID][RM_SOURCEPATH_INDEX], 
                                                            u4PicCnt/5000 * 5000,
                                                            strRMSrcFileName,
                                                            _u4FileCnt[u4InstID],
                                                            strSrcFileNameExt_FileList);
        }
        else
        {
          //sprintf(strEmuFileName, "%s/%s%07d%s", _bFileStr1[u4InstID][RM_SOURCEPATH_INDEX], 
          sprintf(_bFileStr1[u4InstID][1], "%s%s%07d%s", _bFileStr1[u4InstID][RM_SOURCEPATH_INDEX], 
                                                            strRMSrcFileName,
                                                            _u4FileCnt[u4InstID],
                                                            strSrcFileNameExt_FileList);
        }
    }
  #else //SATA_HDD_FS_SUPPORT
  sprintf(_bFileStr1[u4InstID][1], "%s\\%s%07d%s", _bFileStr1[u4InstID][RM_SOURCEPATH_INDEX], 
                                                            strRMSrcFileName,
                                                            _u4FileCnt[u4InstID],
                                                            strSrcFileNameExt_FileList);
  #endif //SATA_HDD_FS_SUPPORT
  
  #ifdef RM_ATSPEED_TEST_ENABLE
  //AU Info 
  _tInFileInfo[u4InstID].fgGetFileInfo = TRUE;
  _tInFileInfo[u4InstID].pucTargetAddr = _pucRMAULikeBuf[u4InstID];
  _tInFileInfo[u4InstID].u4TargetSz = RM_AULIKEBUF_SZ;    
  _tInFileInfo[u4InstID].u4FileLength = 0;    
  _tInFileInfo[u4InstID].u4FileOffset = 0;

  sprintf(strMessage, "%s: ", _bFileStr1[u4InstID][RM_AUFIFO_INDEX]);
  vVDecOutputDebugString(strMessage);

  fgOpen = fgOpenFile(u4InstID, _bFileStr1[u4InstID][RM_AUFIFO_INDEX],"r+b", &_tInFileInfo[u4InstID]);
  if(fgOpen == FALSE)
  {
    vVDecOutputDebugString("Open bit-stream fail\n");
    strcpy(_tFileListRecInfo[u4InstID].bFileName,_FileList_Rec[u4InstID]);
    sprintf(strMessage, "%s", "Open bit-stream fail\n");
    fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
    return FALSE;
  }

  //Checksum Info
  #if 1
  _tInFileInfo[u4InstID].fgGetFileInfo = TRUE;
  _tInFileInfo[u4InstID].pucTargetAddr = _pucRMChecksumBuf[u4InstID];
  _tInFileInfo[u4InstID].u4TargetSz = RM_CHECKSUM_BUFFER_SZ;    
  _tInFileInfo[u4InstID].u4FileLength = 0;    
  _tInFileInfo[u4InstID].u4FileOffset = 0;

  sprintf(strMessage, "%s: ", _bFileStr1[u4InstID][RM_SUMINFO_INDEX]);
  vVDecOutputDebugString(strMessage);

  fgOpen = fgOpenFile(u4InstID, _bFileStr1[u4InstID][RM_SUMINFO_INDEX],"r+b", &_tInFileInfo[u4InstID]);
  if(fgOpen == FALSE)
  {
    vVDecOutputDebugString("Open bit-stream fail\n");
    strcpy(_tFileListRecInfo[u4InstID].bFileName,_FileList_Rec[u4InstID]);
    sprintf(strMessage, "%s", "Open bit-stream fail\n");
    fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
    return FALSE;
  }
  #endif //0
  #endif //RM_ATSPEED_TEST_ENABLE

#ifndef RM_CRCCHECKFLOW_SUPPORT
#ifndef RM_ATSPEED_TEST_ENABLE
  //Read RM Goldem from PC
  _tInFileInfo[u4InstID].fgGetFileInfo = TRUE;
  _tInFileInfo[u4InstID].pucTargetAddr = _pucRMGoldenDataBuf[u4InstID];
  _tInFileInfo[u4InstID].u4TargetSz = RM_GOLDENDATA_SZ;    
  _tInFileInfo[u4InstID].u4FileLength = 0;    
  _tInFileInfo[u4InstID].u4FileOffset = 0;

  sprintf(strMessage, "%s: ", _bFileStr1[u4InstID][5]);
  vVDecOutputDebugString(strMessage);

  //fgOpen = fgOpenFile(u4InstID, _bFileStr1[u4InstID][5],"r+b", &_tInFileInfo[u4InstID]);
  fgOpen = fgOpenFile(u4InstID, strEmuFileName,"r+b", &_tInFileInfo[u4InstID]);
  if(fgOpen == FALSE)
  {
    vVDecOutputDebugString("Open bit-stream fail\n");
    strcpy(_tFileListRecInfo[u4InstID].bFileName,_FileList_Rec[u4InstID]);
    sprintf(strMessage, "%s", "Open bit-stream fail\n");
    fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
    return FALSE;
  }
#endif //#ifndef RM_ATSPEED_TEST_ENABLE        
#endif //RM_CRCCHECKFLOW_SUPPORT

  #ifdef RM_CRCCHECKFLOW_SUPPORT
  #ifdef RM_CRCCHECK_ENABLE
  //Only for CRC Check
  if (fgRMCheckCRCResult)
  {
    _tInFileInfo[u4InstID].fgGetFileInfo = TRUE;
    _tInFileInfo[u4InstID].pucTargetAddr = _pucRMCRCResultBuf[u4InstID];
    _tInFileInfo[u4InstID].u4TargetSz = RM_CRCRESULT_SZ;    
    _tInFileInfo[u4InstID].u4FileLength = 0;    
    _tInFileInfo[u4InstID].u4FileOffset = 0;

    sprintf(strMessage, "%s: ", _bFileStr1[u4InstID][RM_CRCINFO_INDEX]);
    vVDecOutputDebugString(strMessage);

    fgOpen = fgOpenFile(u4InstID, _bFileStr1[u4InstID][RM_CRCINFO_INDEX],"r+b", &_tInFileInfo[u4InstID]);
    if(fgOpen == FALSE)
    {
      vVDecOutputDebugString("Open bit-stream fail\n");
      strcpy(_tFileListRecInfo[u4InstID].bFileName,_FileList_Rec[u4InstID]);
      sprintf(strMessage, "%s", "Open bit-stream fail\n");
      fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
      return FALSE;
    }
  }
  #endif //RM_CRCCHECK_ENABLE
  #endif //RM_CRCCHECKFLOW_SUPPORT

  return TRUE;
}



// *********************************************************************
// Function    : void vVDecVerifyThread(UINT32 u4InstID)
// Description : System entry point
// Parameter   : None
// Return      : None
// *********************************************************************
#if MEM_ALLOCATE_IOREMAP
#define VDEC_CACHE_PROC (0)
#else
#define VDEC_CACHE_PROC (1)
#endif

#if (VDEC_CACHE_PROC)
#include <linux/dma-mapping.h>
#endif
extern volatile BOOL g_fgAllocate[2];

UINT32 u4BitStreamLengthH264 = 0;
extern UINT32 g_u4AllocSize;

char gpfH264LogFileBuffer[4096];
unsigned int gH264logbufferOffset = 0;
int gfpH264log = -1;
#define H264LOGFILENAME "/mnt/sdcard/H264RISCLog.txt"

#define OUTPUT_USB_FILE 0


void vVDecVerifyThread(void* vpdata)
{
  VDEC_PARAM_T* pparam = vpdata;
  UINT32 u4InstID;
    UINT32 u4WmvMode;
  BOOL fgOpen,fgExistVerifyLoop;
  BOOL fgMVCType;
  BOOL fgInit;
  char strMessage[512];
  CHAR strVP6SizeFile[ 512];
  u4InstID =  pparam->u4InstanceId;
  _fgMVCType = pparam->fgMVCType;
//  u4WmvMode  =  pparam->u4Mode;   
  u4WmvMode  =  1;//(UINT32) ppv_param[1]; /* 1: VP6 adobe mode; 0: Open source or multi-stream. */  
 
  #if VDEC_CACHE_PROC
  UINT32 u4FifoDataSize = 0;
  UINT32 u4MappedVFifoSa = 0;
  //struct device rDev; 
  #endif
  g_fgAllocate[0] = FALSE;
  g_fgAllocate[1] = FALSE;
  g_u4AllocSize = 0;
  printk("u4InstID = %d,u4WmvMode = %d mvc = %d\n",u4InstID,u4WmvMode,_fgMVCType);
  fgExistVerifyLoop = FALSE;
  
    if (u4WmvMode > 0)
    {
        _u4WmvMode[u4InstID] = 1;
        _u4AdobeMode[u4InstID] = 1;
    }
    else
    {
        _u4WmvMode[u4InstID] = 0;
        _u4AdobeMode[u4InstID] = 0;
    }

    if(_fgMVCType)
    {
      _fgMVCBaseGo = FALSE;
      if(u4InstID == 1)
      {
        _fgMVCResReady[0] = FALSE;
        _fgMVCResReady[1] = FALSE;
      }
      while((u4InstID == 0) && (!_fgMVCBaseGo))
      {
        msleep(5);
      }
    }
    
    
    vVDecGconEnable();
#if (CONFIG_DRV_FPGA_BOARD)
#else
  vVDecClockSelect();
#endif

  vMemoryAllocate(u4InstID);

#if VDEC_DRAM_BUSY_TEST
   vDrmaBusySet (u4InstID);
#endif

#ifdef SATA_HDD_READ_SUPPORT
   #ifndef  SATA_HDD_FS_SUPPORT
  fgInitHDDFileAccess(u4InstID);
   #else
      //FS mount
    fgHDDFsMount(0);
   #endif
#endif

#if OUTPUT_USB_FILE
if(-1 == gfpH264log){
  gfpH264log = vdecopenFile((char*)H264LOGFILENAME,O_CREAT|O_WRONLY,0); 
  if (gfpH264log == -1)
  {
      printk("Fs open file fail %d\n", gfpH264log);
  }
  else{
      gH264logbufferOffset = 0;
      printk("Create H264Log file --> %d \n", gfpH264log);
  }
}

#endif


#ifdef IDE_READ_SUPPORT
  _DmxIdeReset();
#endif
#ifdef BARREL2_THREAD_SUPPORT
  if(!_fgSemaCreated[u4InstID])
  {
    VERIFY (x_sema_create(&_ahVDecEndSema[u4InstID], X_SEMA_TYPE_BINARY, X_SEMA_STATE_UNLOCK) == OSR_OK);
    _fgSemaCreated[u4InstID] = TRUE;
  }
#endif
  _u4StartCompPicNum[u4InstID] = 0;
  _u4EndCompPicNum[u4InstID] = 0xffffffff;
  _u4DumpRegPicNum[u4InstID] = 0xffffffff;
  _u4MaxReDecBistreamCnt[u4InstID] = 0;
#ifdef REDEC 
  //_u4ReDecNum[u4InstID] = 0;
  //_u4ReDecPicNum[u4InstID] = 0;
  _u4ReDecCnt[u4InstID] = 0;
#endif
  _u4RedecBistreamCnt[u4InstID] = 0xffffffff;
  _u4FileCnt[u4InstID] = 0;
  _u4FileListIdx[u4InstID] = 0;
  _u4FilePos[u4InstID] = 0;
  #ifdef WMV_CRC_COMPOSITE_CHECK_ENABLE
  _u4CRCCnt[u4InstID] = 0;
  #endif
#ifdef VERIFICATION_DOWN_SCALE
  _fgVDSCLEnableRandomTest[u4InstID] = TRUE;
   _fgVDSCLEnableLumaKeyTest[u4InstID] = FALSE;
#endif
#ifdef BARREL2_SUPPORT
  _u4BSID[u4InstID] = 1;
#else
  _u4BSID[u4InstID] = 0;
#endif


#if VDEC_DDR3_SUPPORT    
    _u2AddressSwapMode[u4InstID] = ADDRSWAP_DDR3;
#else
    _u2AddressSwapMode[u4InstID] = ADDRSWAP_OFF;
#endif

  fgInit = TRUE;
  _u4PowerTestInit[u4InstID] = 0;

  _tFileListInfo[u4InstID].i4FileId = 0xFFFFFFFF;
  while(fgVdecReadFileName(u4InstID, &_tFileListInfo[u4InstID], &_tFileListRecInfo[u4InstID], &_u4StartCompPicNum[u4InstID], &_u4EndCompPicNum[u4InstID], &_u4DumpRegPicNum[u4InstID]))
  {
     _u4SkipFrameCnt[u4InstID] = 0;
     _u4PicCnt[u4InstID] = 0;
     _u4FileCnt[u4InstID] = 0;
     if(fgExistVerifyLoop)
     {
       break;
     }

    if (fgInit == TRUE)
    {
        _u4StartCompPicNum[u4InstID] = 0;
        fgInit = FALSE;
    }

    #if VDEC_TEST_ADDSWAP
        _u2AddressSwapMode[u4InstID] = (UINT16) (((UINT32) rand())%8);
        while(_u2AddressSwapMode[u4InstID] == 3 || _u2AddressSwapMode[u4InstID] == 7)
        {
            _u2AddressSwapMode[u4InstID] = (UINT16) (((UINT32) rand())%8);
        }

        printk("[VDEC] Address Swap Mode = %d\n", _u2AddressSwapMode[u4InstID]);
    #endif

        #if VDEC_FIELD_COMPACT
        _u2AddressSwapMode[u4InstID] = 8;
        printk("[VDEC] Address Swap Mode = %d, Field Compact Enable\n", _u2AddressSwapMode[u4InstID]);
        #endif

     if (!fgVDecAllocWorkBuffer(u4InstID))
     {
         printk("[VDEC]Memory alloc failed\n");
         break;
     }

     #if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560) && (!CONFIG_DRV_FPGA_BOARD))
 	vVDecSetVldMcClk(u4InstID,_u4CodecVer[u4InstID]);
     #endif

     #ifdef VDEC_VIDEOCODEC_RM
     fgOpen = fgVDecVerify_PrepareInfo_RM(u4InstID);

     if(fgOpen == FALSE)
     {
       continue;
     }
     #endif //VDEC_VIDEOCODEC_RM
   
#ifdef LETTERBOX_SUPPORT
     if(_u4CodecVer[u4InstID] == VDEC_H264)
     {
     VDEC_INFO_VERIFY_FILE_INFO_T tInFileInfo;
     
     tInFileInfo.fgGetFileInfo = TRUE;
     tInFileInfo.pucTargetAddr = (UCHAR*) _pucSettingFileSa[u4InstID];
     tInFileInfo.u4TargetSz = (UINT32) FILE_LIST_SZ;    
     tInFileInfo.u4FileLength = 0;    
     tInFileInfo.u4FileOffset = 0;
      sprintf(strMessage, "%s: ", _bFileStr1[u4InstID][12]);
      vVDecOutputDebugString(strMessage);
      printk("=====> Setting File Name = %s < ===== \n", strMessage);
      fgOpen = fgOpenFile(u4InstID, _bFileStr1[u4InstID][12],"r+b", &tInFileInfo);
      if(fgOpen == FALSE)
      {
       vVDecOutputDebugString("Open setting file fail\n");
       strcpy(_tFileListRecInfo[u4InstID].bFileName,_FileList_Rec[u4InstID]);
       sprintf(strMessage, "%s", "Open setting file fail\n");
       fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
       vVDecFreeWorkBuffer(u4InstID);
       continue;
      }
      _pcLBDSettingFile[u4InstID] = (char *)_pucSettingFileSa[u4InstID];

     tInFileInfo.fgGetFileInfo = TRUE;
     tInFileInfo.pucTargetAddr = (UCHAR*) _pucGoldenFileSa[u4InstID];
     tInFileInfo.u4TargetSz = (UINT32) FILE_LIST_SZ;    
     tInFileInfo.u4FileLength = 0;    
     tInFileInfo.u4FileOffset = 0;
      sprintf(strMessage, "%s: ", _bFileStr1[u4InstID][13]);
      vVDecOutputDebugString(strMessage);
      printk("=====> Golden File Name = %s < ===== \n", strMessage);
      fgOpen = fgOpenFile(u4InstID, _bFileStr1[u4InstID][13],"r+b", &tInFileInfo);
      if(fgOpen == FALSE)
      {
       vVDecOutputDebugString("Open golden file fail\n");
       strcpy(_tFileListRecInfo[u4InstID].bFileName,_FileList_Rec[u4InstID]);
       sprintf(strMessage, "%s", "Open golden file fail\n");
       fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
       vVDecFreeWorkBuffer(u4InstID);
       continue;
      }
      _pcLBDGoldenFile[u4InstID] = (char *)_pucGoldenFileSa[u4InstID];
      }
#endif
     
   
   #ifdef  RING_VFIFO_SUPPORT
      _u4LoadBitstreamCnt[u4InstID] = 0;
    #endif
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


      sprintf(strMessage, "%s: ", _bFileStr1[u4InstID][1]);
      vVDecOutputDebugString(strMessage);
      
      printk("=====> Test File Name = %s < ===== \n", strMessage);
      
      fgOpen = fgOpenFile(u4InstID, _bFileStr1[u4InstID][1],"r+b", &_tInFileInfo[u4InstID]);//Load bitstream
      //6589NEW (4) error concealment test only
      #ifdef MPEG4_6589_ERROR_CONCEAL
      _u4TotalBitstreamLen[u4InstID] = _tInFileInfo[u4InstID].u4FileLength;
      #endif
      if(_tInFileInfo[u4InstID].u4FileLength > V_FIFO_SZ)
       {
         printk("=====>The Vfifo size is not enough!. \n");
         printk("=====>The file's size is 0x%.8x bytes\n", _tInFileInfo[u4InstID].u4FileLength);
         //continue;
       }
     
   #ifdef  RING_VFIFO_SUPPORT
      _u4LoadBitstreamCnt[u4InstID]++;
    #endif

     if(fgOpen == FALSE)
     {
       vVDecOutputDebugString("Open bit-stream fail\n");
       strcpy(_tFileListRecInfo[u4InstID].bFileName,_FileList_Rec[u4InstID]);
       sprintf(strMessage, "%s", "Open bit-stream fail\n");
       fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
       //vVDecFreeWorkBuffer(u4InstID);
       continue;
     }
     else
     	{
     	  if((u4InstID == 1) && _ucMVCType[1]) 
     	  {    
     	    _fgMVCReady[0] = TRUE;    
     	    _fgMVCReady[1] = FALSE;	  
     	  }

     	}

        //Copy for Temp Buf to VFIFO
        #ifdef RM_RINGVIFO_FLOW
        {
          UINT32 u4VFifoWPtr = (UINT32) _pucVFifo[u4InstID];
          UINT32 u4VFIFOSa = (UINT32) _pucVFifo[u4InstID];
          UINT32 u4VFIFOSz = (UINT32) V_FIFO_SZ;
          UINT32 u4TempVFIFOSa = (UINT32) _pucRMRingWorkBuf[u4InstID];
          UINT32 u4CopySize = _tInFileInfo[u4InstID].u4FileLength;
          UINT32 u4RemSz = 0;

          #ifdef RM_ATSPEED_TEST_ENABLE
          _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFIFORPtr = (UINT32) _pucRMAULikeBuf[u4InstID];
          #else //RM_ATSPEED_TEST_ENABLE
          _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMFIFORPtr = (UINT32) _pucVFifo[u4InstID];
          #endif //RM_ATSPEED_TEST_ENABLE

          if ((u4VFifoWPtr+_tInFileInfo[u4InstID].u4FileLength) < (UINT32) (u4VFIFOSa+u4VFIFOSz))
          {
            memcpy((void*)(u4VFifoWPtr), (void*)u4TempVFIFOSa, u4CopySize);
            _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.u4RMVFIFOWPtr = u4VFIFOSa + u4CopySize;
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

      
     if((_u4CodecVer[u4InstID] == VDEC_H264) ||(_u4CodecVer[u4InstID] == VDEC_MPEG4))
     {
       vAddStartCode2Dram(_pucVFifo[u4InstID]+_tInFileInfo[u4InstID].u4FileLength);
     }
     
      #if VDEC_CACHE_PROC // update physical memory
      {      
      if(_u4CodecVer[u4InstID] == VDEC_H264)
      {
        u4FifoDataSize = _tInFileInfo[u4InstID].u4FileLength + 4;
      }
      else
      {
        u4FifoDataSize = _tInFileInfo[u4InstID].u4FileLength;
      }    

      printk("vVDecVerifyThread, update cache to physical mem, addr:0x%x, size:0x%x\n", 
        _pucVFifo[u4InstID], u4FifoDataSize);

      // tmp:
      //u4FifoDataSize = V_FIFO_SZ;

      u4MappedVFifoSa = dma_map_single(NULL, (UINT32)(_pucVFifo[u4InstID]), u4FifoDataSize, DMA_TO_DEVICE);  
      }
      #endif

     strcpy(_tFileListRecInfo[u4InstID].bFileName,_FileList_Rec[u4InstID]);
     sprintf(strMessage,"The bitstream file length / real returns is %d/%d (0x%.8x/0x%.8x)\n",
      _tInFileInfo[u4InstID].u4FileLength, _tInFileInfo[u4InstID].u4RealGetBytes,_tInFileInfo[u4InstID].u4FileLength, _tInFileInfo[u4InstID].u4RealGetBytes);
     fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);

     u4BitStreamLengthH264 = _tInFileInfo[u4InstID].u4RealGetBytes;
        
     if (_u4CodecVer[u4InstID] == VDEC_VP6)
     {
         _rSizeFileInfo.fgGetFileInfo = TRUE;  
         _rSizeFileInfo.pucTargetAddr = _pucSizeFileBuf[u4InstID];
         _rSizeFileInfo.u4TargetSz = 1024*400;;  
         _rSizeFileInfo.u4FileLength = 0;
         sprintf(strVP6SizeFile, "%s.size", _bFileStr1[u4InstID][1]);
         fgOpenFile(u4InstID, strVP6SizeFile,"r+b", &_rSizeFileInfo);

         if(fgOpen == FALSE)
         {
             sprintf(strMessage, "%s", "Open VP6 size fail\n");
             fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
             continue;
        }

        if (0 == _u1AlphaBitstream[u4InstID])
        {
            //_fgVP6CRCExist[u4InstID] = fgVP6CRCPatternExist(u4InstID);
            _fgVP6CRCExist[u4InstID] = 0;
        }
        else
        {
            _fgVP6CRCExist[u4InstID] = 0;
            _u4AdobeMode[u4InstID] = 0;
        }
        _fgVP6SmallFlolder[u4InstID] = fgVP6SmallFolder(u4InstID);

        printk("<vdec> _fgVP6CRCExist[%u]=%d, _fgVP6SmallFlolder[%u]=%d\n", u4InstID, _fgVP6CRCExist[u4InstID], u4InstID, _fgVP6SmallFlolder[u4InstID]);
     }        

     // main decoding loop
#if VDEC_VP8_WEBP_SUPPORT_ME2_INTEGRATION
     break;
#else
     vMpvPlay(u4InstID);
#endif
     
    #if VDEC_CACHE_PROC
    printk("vVDecVerifyThread, unmap physical mem, addr:0x%x, size:0x%x\n", 
     _pucVFifo[u4InstID], u4FifoDataSize);

    dma_unmap_single(NULL, u4MappedVFifoSa, u4FifoDataSize, DMA_TO_DEVICE); 
    #endif

    _u4StartCompPicNum[u4InstID] = 0;

     //vVDecFreeWorkBuffer(u4InstID);
  }

#if VDEC_VP8_WEBP_SUPPORT_ME2_INTEGRATION
#else
  #ifdef PCFILE_WRITE  
  if(_tInFileInfo[u4InstID].pFile)
  {
    fclose(_tInFileInfo[u4InstID].pFile);
  }
  #endif
  vVerifyVDecIsrStop(u4InstID);

  vMemoryFree(u4InstID);

if(-1 != gfpH264log){
 if(gH264logbufferOffset > 0 ){ 
    vdecwriteFile(gfpH264log, gpfH264LogFileBuffer, gH264logbufferOffset);
 }
 vdeccloseFile(gfpH264log);
 printk("close H264 log file\n");
}


#ifdef SATA_HDD_READ_SUPPORT
   #ifdef  SATA_HDD_FS_SUPPORT
      if (_tFileListInfo[u4InstID].i4FileId != 0xFFFFFFFF)
      {
         // fgHDDFsCloseFile(u4InstID); // temp avoid system crash
         _tFileListInfo[u4InstID].i4FileId = 0xFFFFFFFF;
      }
      //FS mount
    fgHDDFsUnMount(0);
   #endif
#endif

#endif
}


void vSecBSVerifyThread(void *param_array)
{
#ifdef BARREL2_THREAD_SUPPORT
  void** ppv_param = (void**)param_array ;
  UINT32 u4InstID =  (UINT32) ppv_param[0];
  UINT32 u4SecBSSpec =  (UINT32) ppv_param[1];

  if(u4SecBSSpec == 0)
  {
    VDEC_INFO_WMV_BS_INIT_PRM_T rWmvBSInitPrm;
  
    _pucSecVFifo[u4InstID] = (UCHAR *)x_alloc_aligned_nc_mem(1024*1024,1024);
    x_memset(_pucSecVFifo[u4InstID], 0x73, 1024*1024);
    rWmvBSInitPrm.u4VFifoSa = (UINT32)_pucSecVFifo[u4InstID];
    rWmvBSInitPrm.u4VFifoEa = (UINT32)_pucSecVFifo[u4InstID] + 1024*1024;
    rWmvBSInitPrm.u4ReadPointer= (UINT32)_pucSecVFifo[u4InstID];
    rWmvBSInitPrm.u4WritePointer= 0xFFFFFFFF;//(UINT32)_pucSecVFifo[u4InstID] + 1024*1024;
    if (_i4CodecVersion[u4InstID] == VC1)
    {
        i4VDEC_HAL_WMV_InitBarrelShifter(1 ,u4InstID, &rWmvBSInitPrm, TRUE);
    }
    else
    {
        i4VDEC_HAL_WMV_InitBarrelShifter(1 ,u4InstID, &rWmvBSInitPrm, FALSE);
    }
    while(1)
    {
      u4VDEC_HAL_WMV_ShiftGetBitStream(1, u4InstID, random(32));
      u4VDEC_HAL_WMV_ShiftGetBitStream(1, u4InstID, random(32));
      u4VDEC_HAL_WMV_ShiftGetBitStream(1, u4InstID, random(32));
      u4VDEC_HAL_WMV_ShiftGetBitStream(1, u4InstID, random(32));
      x_thread_delay(1);
    }
  }
  else if(u4SecBSSpec == 1)
  {
    VDEC_INFO_H264_BS_INIT_PRM_T rH264BSInitPrm;

    if(!_fgSemaCreated[u4InstID])
    {
      VERIFY (x_sema_create(&_ahVDecEndSema[u4InstID], X_SEMA_TYPE_BINARY, X_SEMA_STATE_UNLOCK) == OSR_OK);
      _fgSemaCreated[u4InstID] = TRUE;
    }
    _pucSecVFifo[u4InstID] = (UCHAR *)x_alloc_aligned_nc_mem(1024*1024,1024);
    x_memset(_pucSecVFifo[u4InstID], 0x73, 1024*1024);
    rH264BSInitPrm.u4VFifoSa = (UINT32)_pucSecVFifo[u4InstID];
    rH264BSInitPrm.u4VFifoEa = (UINT32)_pucSecVFifo[u4InstID] + 1024*1024;
    rH264BSInitPrm.u4VLDRdPtr = (UINT32)_pucSecVFifo[u4InstID];
    rH264BSInitPrm.u4VLDWrPtr = 0xFFFFFFFF;//(UINT32)_pucSecVFifo[u4InstID] + 1024*1024;
    i4VDEC_HAL_H264_InitBarrelShifter(1, u4InstID, &rH264BSInitPrm);
    while(1)
    {
      VERIFY (x_sema_lock(_ahVDecEndSema[u4InstID], X_SEMA_OPTION_WAIT) == OSR_OK);
      u4VDEC_HAL_H264_ShiftGetBitStream(1, u4InstID, random(32));
      u4VDEC_HAL_H264_ShiftGetBitStream(1, u4InstID, random(32));
      u4VDEC_HAL_H264_ShiftGetBitStream(1, u4InstID, random(32));
      u4VDEC_HAL_H264_ShiftGetBitStream(1, u4InstID, random(32));
      VERIFY (x_sema_unlock(_ahVDecEndSema[u4InstID]) == OSR_OK);
      x_thread_delay(1);
    }
  }
#endif
}

void vVDecGconEnable(void)
{
    i4VDEC_HAL_Common_Gcon_Enable();
}

void vVDecClockSelect(void)
{
   i4VDEC_HAL_Common_Init(0);
}

void vDrmaBusySet (UINT32  u4InstID)
{
    i4VDEC_HAL_Dram_Busy(0, PHYSICAL( (UINT32)(_pucDramBusy[u4InstID]) ), 0x10000);
}

void vDrmaBusyOff (UINT32  u4InstID)
{
    i4VDEC_HAL_Dram_Busy_Off(0, PHYSICAL( (UINT32)(_pucDramBusy[u4InstID]) ), 0x10000);
}

