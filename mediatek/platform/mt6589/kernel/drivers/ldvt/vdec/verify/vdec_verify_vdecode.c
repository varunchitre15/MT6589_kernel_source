#include <linux/string.h>  
#include <linux/interrupt.h>
#include "vdec_verify_vdecode.h"
#include "../hal/vdec_hal_if_common.h"
#include "../hal/vdec_hal_if_wmv.h"
#include "../hal/vdec_hal_if_mpeg.h"
#include "../hal/vdec_hal_if_h264.h"
#include "../include/vdec_info_common.h"
//#include "vdec_verify_dvdec.h"
//#include "vdec_info_dv.h"
#include "../vdec.h"
//#include "vdec_hw_dvdec.h"
#include "../hal/vdec_hw_common.h"
#include "../hal/vdec_hw_h264.h"

#include "../include/drv_common.h"

//#include "x_bim.h"
//#include "x_os.h"
//#include "x_assert.h"
//#include "x_debug.h"

#ifdef VERIFICATION_FGT
#include "vdec_verify_fgt.h"
#endif

#include "vdec_verify_file_common.h"
#include "vdec_verify_filesetting.h"
#include "vdec_verify_irq_fiq_proc.h"
#include "vdec_verify_common.h"

#include "vdec_verify_vparser_mpeg.h"
#include "vdec_verify_vparser_mpeg4.h"
#include "vdec_verify_vparser_wmv.h"
#include "vdec_verify_vparser_h264.h"
#include "vdec_verify_vparser_rm.h"
#include "vdec_verify_vparser_vp6.h"
#include "vdec_verify_vparser_avs.h"
#include "vdec_verify_vparser_vp8.h"


#include <linux/init.h>
#include <linux/module.h> 
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>

#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>

#include <asm/uaccess.h>
#include <linux/sched.h>
#include <linux/file.h> 
#include <linux/slab.h>


#include <linux/delay.h>

extern char gpfH264LogFileBuffer[4096];
extern int gfpH264log;
extern unsigned int gH264logbufferOffset;
int vdecwriteFile(int fp,char *buf,int writelen);

//#define DBG_H264_PRINTF(format,...)  do { if(-1 != gfpH264log){ { gH264logbufferOffset += sprintf((char *)(gpfH264LogFileBuffer+gH264logbufferOffset),format, ##__VA_ARGS__);} if(gH264logbufferOffset >= 3840 ){ vdecwriteFile(gfpH264log, gpfH264LogFileBuffer, gH264logbufferOffset); gH264logbufferOffset = 0; }  }     } while (0)
#define DBG_H264_PRINTF
/*
#define DBG_H264_PRINTF(format,...)  \
    do { \
        if (-1 != gfpH264log) {\
            { gH264logbufferOffset += sprintf((char *)(gpfH264LogFileBuffer+gH264logbufferOffset),format, ##__VA_ARGS__);} \
            if (gH264logbufferOffset >= 3840 ) { \
                vdecwriteFile(gfpH264log, gpfH264LogFileBuffer, gH264logbufferOffset); \
                gH264logbufferOffset = 0; \
            } \
        } \
    } while (0)
*/


extern int rand(void);

extern void reset_pic_hdr_bits(UINT32 u4InstID);
extern UINT32 pic_hdr_bitcount(UINT32 u4InstID) ;

void vNormDecProc(UINT32 u4InstID);
void vVerifyWMVInitProc(UINT32 u4InstID);
void vInitVParserMPEG(UINT32 u4InstID);
void ComputeDQuantDecParam(UINT32 u4InstID);
void vVerTestMTCMOS(UINT32 u4InstID);
void vVerTestDCM(UINT32 u4InstID);
void vVerInitVDec(UINT32 u4InstID);
void vVDecProc(UINT32 u4InstID);
void vSetDownScaleParam(UINT32 u4InstID, BOOL fgEnable, VDEC_INFO_VDSCL_PRM_T *prDownScalerPrm);
void vCodecVersion(UINT32 u4InstID, UINT32 u4CodecFOURCC);
void vChkVDec(UINT32 u4InstID);
void vH264VDecEnd(UINT32 u4InstID);
void vVerifyFlushBufInfo(UINT32 u4InstID);
BOOL fgIsH264VDecComplete(UINT32 u4InstID);
void vVerifyAdapRefPicmarkingProce(UINT32 u4InstID);
void vVerifySetPicRefType(UINT32 u4InstID, UCHAR ucPicStruct, UCHAR ucRefType);
UCHAR bGetPicRefType(UINT32 u4InstID, UCHAR ucPicStruct);
void vChkOutputFBuf(UINT32 u4InstID);
void vAdd2RefPicList(UINT32 u4InstID);
void vVerifyClrPicRefInfo(UINT32 u4InstID, UCHAR ucPicType, UCHAR ucFBufIdx);

void vVerifyFlushAllSetData(UINT32 u4InstID);
void vH264DecEndProc(UINT32 u4InstID);
void vWMVDecEndProc(UINT32 u4InstID);
void vMPEGDecEndProc(UINT32 u4InstID);
void vVerifyDx3SufxChk(UINT32 u4InstID);
void vSetDx3SliceBoundary(UINT32 u4InstID, VDEC_INFO_MPEG_DEC_PRM_T *prVDecMPEGDecPrm);
void vMp4FixBCode(UINT32 u4InstID);
void PostAdjustReconRange(UINT32 u4InstID);
void vWMVVDecEnd(UINT32 u4InstID);
BOOL fgIsWMVVDecComplete(UINT32 u4InstID);
void vVerifySetVSyncPrmBufPtr(UINT32 u4InstID, UINT32 u4BufIdx);
void vReadWMVChkSumGolden(UINT32 u4InstID);
void vReadH264ChkSumGolden(UINT32 u4InstID);
void vReadMPEGChkSumGolden(UINT32 u4InstID);
void vVerifySetUpParm(UINT32 u4InstID, UINT32 dwPicW, UINT32 dwPicH, UINT32 dwFrmRatCod, BOOL fgDivXM4v, BOOL fgDx4M4v);
void vDvCompare(UINT32 u4InstID);
void vReadDvChkSumGolden(UINT32 u4InstID);
void vH264ChkSumDump(UINT32 u4InstID);


void vVParserProc(UINT32 u4InstID);
void vVerifyInitVParserWMV(UINT32 u4InstID);
void vVPrsMPEGIPProc(UINT32 u4InstID);
void vVPrsMPEGBProc(UINT32 u4InstID);

#ifdef VPMODE
INT32 i4VPModeDecStart(UINT32 u4VDecID,VDEC_INFO_DEC_PRM_T *prDecPrm);
#endif

void vAVCDumpChkSum(void);
void vPrintDumpReg(UINT32 u4InstID,UINT32 fgTAB);


#ifdef MPEG4_CRC_CMP
extern void vMPEG4CrcCmp(UINT32 u4InstID,UCHAR *ptAddr,UINT32 u4Size);
#endif

#ifdef	VDEC_SRAM
void vDumpSram(UINT32 u4InstID);
void vWriteSram(UINT32 u4InstID,UINT32 u4SramAddr,UINT32 u4SramValue);
UINT32 u4ReadSram(UINT32 u4InstID,UINT32 u4SramAddr);
#endif

extern void reset_dec_counter(UINT32 u4InstID);


extern UINT32 u4FilePicCont_noVOP;

// *********************************************************************
// Function    : void vNormDecProc(UINT32 u4InstID)
// Description : normal decode procedure
// Parameter   : None
// Return      : None
// *********************************************************************
void vMpvPlay(UINT32 u4InstID)
{
  if(_u4CodecVer[u4InstID] == VDEC_WMV)
  {
    vVerifyWMVInitProc(u4InstID);
  }
  while(_u4VerBitCount[u4InstID] < (_tInFileInfo[u4InstID].u4FileLength << 3))  
  {
    vNormDecProc(u4InstID);
  }
}

// *********************************************************************
// Function    : void vVerifyWMVInitProc(UINT32 u4InstID)
// Description : WMV initialize process
// Parameter   : None
// Return      : None
// *********************************************************************
void vVerifyWMVInitProc(UINT32 u4InstID)
{
  VDEC_INFO_WMV_VFIFO_PRM_T rWmvVFifoInitPrm;
  VDEC_INFO_WMV_BS_INIT_PRM_T rWmvBSInitPrm;
  //UINT32 *pu4VFIFOSa;
  char fiInName[256];
  char FileExt[4];
  INT32 iLen,i4RCVNumFrames,i4CodecVersion,u4CodecFOURCC;
    
  strcpy(fiInName, _bFileStr1[u4InstID][1]);
  iLen = strlen(fiInName);
  if( (fiInName[iLen-4] == '.') && (fiInName[iLen-3] == 'v') &&
      (fiInName[iLen-2] == '9') && (fiInName[iLen-1] == 'e') )
  {
    strcpy(FileExt, "v9e");
    u4CodecFOURCC = FOURCC_WMVA_WMV;
  }
  if( (fiInName[iLen-4] == '.') && (fiInName[iLen-3] == 'v') &&
      (fiInName[iLen-2] == 'c') && (fiInName[iLen-1] == '1') )
  {
    strcpy(FileExt, "vc1");
    u4CodecFOURCC = FOURCC_WVC1_WMV;
  }
  if( (fiInName[iLen-4] == '.') && (fiInName[iLen-3] == 'r') &&
      (fiInName[iLen-2] == 'c') && (fiInName[iLen-1] == 'v') )
  {
    strcpy(FileExt, "rcv");
    i4RCVNumFrames = (*_pucVFifo[u4InstID])+((*(_pucVFifo[u4InstID]+1))<<8)+((*(_pucVFifo[u4InstID]+2))<<16)+((*(_pucVFifo[u4InstID]+3))<<24);//(*pu4VFIFOSa);
    _i4RcvVersion[u4InstID] = (i4RCVNumFrames >> 30) & 0x1;
    i4CodecVersion = i4RCVNumFrames >> 24;
    if (_i4RcvVersion[u4InstID] == 0)
    {
      i4CodecVersion &= 0x7f;
    }
    else
    {
      i4CodecVersion &= 0x3f;
    }
  
    if(i4CodecVersion == 0)      /* WMV7 */
      u4CodecFOURCC = FOURCC_WMV1_WMV;
    else if(i4CodecVersion == 1)              /* MP43, not supported */
      u4CodecFOURCC = FOURCC_MP43_WMV;
    else if(i4CodecVersion == 2) /* WMV8 */
      u4CodecFOURCC = FOURCC_WMV2_WMV;
    else if(i4CodecVersion== 3)               /* MP42, not supported */
      u4CodecFOURCC = FOURCC_MP42_WMV;
    else if(i4CodecVersion == 4)              /* MP4S, not supported */
      u4CodecFOURCC = FOURCC_MP4S_WMV;
    else if(i4CodecVersion == 5) /* Simple & Main Profile */
      u4CodecFOURCC = FOURCC_WMV3_WMV;
    else if(i4CodecVersion == 6) /* Advanced Profile */
      u4CodecFOURCC = FOURCC_WMVA_WMV;
    else if(i4CodecVersion == 8) /* Advanced Profile */
      u4CodecFOURCC = FOURCC_WVC1_WMV;
  }
      
  vCodecVersion(u4InstID, u4CodecFOURCC);
  vSetVerFRefBuf(u4InstID, 0);
  vSetVerBRefBuf(u4InstID, 1);
  rWmvVFifoInitPrm.u4CodeType = _i4CodecVersion[u4InstID];
  rWmvVFifoInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
  rWmvVFifoInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
  i4VDEC_HAL_WMV_InitVDecHW(u4InstID,&rWmvVFifoInitPrm);
  rWmvBSInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
  rWmvBSInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
  rWmvBSInitPrm.u4ReadPointer= (UINT32)_pucVFifo[u4InstID];
  rWmvBSInitPrm.u4WritePointer= (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
  
  if (_i4CodecVersion[u4InstID] == VDEC_VC1)
  {
      i4VDEC_HAL_WMV_InitBarrelShifter(_u4BSID[u4InstID],u4InstID, &rWmvBSInitPrm, TRUE);
  }
  else
  {
     i4VDEC_HAL_WMV_InitBarrelShifter(_u4BSID[u4InstID],u4InstID, &rWmvBSInitPrm, FALSE);
  }
}

// *********************************************************************
// Function    : void vNormDecProc(UINT32 u4InstID)
// Description : normal decode procedure
// Parameter   : None
// Return      : None
// *********************************************************************
#if (WMV_8320_SUPPORT)
#define WMV_8320_TEST_BARREL_SHIFT (0)
#if WMV_8320_TEST_BARREL_SHIFT
BOOL _fgTestBarrelShift = TRUE;
#endif
#endif
void vNormDecProc(UINT32 u4InstID)
{
  VDEC_INFO_WMV_VFIFO_PRM_T rWmvVFifoInitPrm;
  VDEC_INFO_WMV_BS_INIT_PRM_T rWmvBSInitPrm;
  //printk("<vdec> _tVerDec[%d].ucState=%d\n", u4InstID, _tVerDec[u4InstID]);
  switch(_tVerDec[u4InstID].ucState)
  {
    case DEC_NORM_INIT_PRM:
      //printk("vNormDecProc, DEC_NORM_INIT_PRM\n");
      #if (WMV_8320_TEST_BARREL_SHIFT)
      if (TRUE == _fgTestBarrelShift)
      {
        if(_u4CodecVer[u4InstID] == VDEC_WMV)
        {
          rWmvVFifoInitPrm.u4CodeType = _i4CodecVersion[u4InstID];
          rWmvVFifoInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
          rWmvVFifoInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
          i4VDEC_HAL_WMV_InitVDecHW(u4InstID,&rWmvVFifoInitPrm);
          rWmvBSInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
          rWmvBSInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
          rWmvBSInitPrm.u4ReadPointer= (UINT32)_pucVFifo[u4InstID];
          rWmvBSInitPrm.u4WritePointer= (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;

           if (_i4CodecVersion[u4InstID] == VDEC_VC1)
          {
              i4VDEC_HAL_WMV_InitBarrelShifter(_u4BSID[u4InstID],u4InstID, &rWmvBSInitPrm, TRUE);
          }
          else
          {
             i4VDEC_HAL_WMV_InitBarrelShifter(_u4BSID[u4InstID],u4InstID, &rWmvBSInitPrm, FALSE);
          }
        } 
        vVerInitVDec(u4InstID);

        // delay
        //x_thread_delay(2);

        printk("vNormDecProc, 72:0x%x\n", u4VDecReadVLD(u4InstID, 4*72));
      }
      #else
      vVerInitVDec(u4InstID);
      _tVerDec[u4InstID].ucState = DEC_NORM_VPARSER;
      #endif
      break;
    case DEC_NORM_VPARSER:
      //printk("vNormDecProc, DEC_NORM_VPARSER\n");
      vVParserProc(u4InstID);
      break;
    case DEC_NORM_WAIT_TO_DEC:
      //printk("vNormDecProc, DEC_NORM_WAIT_TO_DEC\n");
      vVDecProc(u4InstID);  
      break;
    case DEC_NORM_WAIT_DECODE:
      //printk("vNormDecProc, DEC_NORM_WAIT_DECODE\n");
      vChkVDec(u4InstID);
#if (POWER_TEST_CASE == POWER_TEST_MTCMOS)
      vVerTestMTCMOS(u4InstID);
#elif (POWER_TEST_CASE == POWER_TEST_DCM)
      vVerTestDCM(u4InstID);
#endif
      break;    
  }

}

#if (POWER_TEST_CASE == POWER_TEST_MTCMOS)
// *********************************************************************
// Function    : void vVerTestMTCMOS(UINT32 u4InstID)
// Description : Test MTCMOS on/off (power to VDEC or not)
// Parameter   : None
// Return      : None
// *********************************************************************
void vVerTestMTCMOS(UINT32 u4InstID)
{
    struct file *fp;
    char buf[512];
    char buf2[2048];
    int writelen = 10;
    int readlen = 1024;
    UINT32 retval = 0;
    mm_segment_t vdecoldfs;

    // Only test once for 1st bitstream 1st pic
    if (_u4PowerTestInit[u4InstID] == 1)
    {
        return;
    }

    _u4PowerTestInit[u4InstID] = 1;

    vdecoldfs = get_fs();
    set_fs(KERNEL_DS);        
     
    fp=filp_open("/proc/clkmgr/subsys_test", O_RDWR, 0); 
    
    if (fp)
    {
        printk("/proc/clkmgr/subsys_test open successful\n");
    }
    else
    {
        printk("/proc/clkmgr/subsys_test open failed\n");
    }

    if (fp->f_op && fp->f_op->write) 
    {
        sprintf(buf, "disable 8\0"); // for 6589, 8 is VDEC
        writelen = 10;        
        retval = fp->f_op->write(fp,buf,writelen,&fp->f_pos);
        printk("[PWR_TEST] MTCMOS write off for 10 sec! retval = %d\n", retval);
#if POWER_TEST_MANUAL_CHECK
        msleep(10000);
        retval = fp->f_op->read(fp,buf2,readlen, &fp->f_pos);
        buf2[512] = '\0';
        printk("[PWR_TEST] Check MTCMOS\n");
        printk("%s \n retval = %d\n", buf2, retval);
#endif
        sprintf(buf, "enable 8\0");
        writelen = 9;
        retval = fp->f_op->write(fp,buf,writelen,&fp->f_pos);
        printk("[PWR_TEST] MTCMOS write on, wait for 10 sec! retval = %d\n", retval);
#if POWER_TEST_MANUAL_CHECK
        msleep(10000);
        fp->f_op->read(fp,buf2,readlen, &fp->f_pos);
        buf2[512] = '\0';
        printk("[PWR_TEST] Check MTCMOS setback\n");
        printk("%s\n retval = %d\n", buf2, retval);
#endif

        reset_dec_counter(u4InstID);

        vWriteGconReg(0, 0x1); // MTCMOS off turns off power, need to turn back VDEC clock afterwards

        filp_close(fp, NULL);
    }       
    set_fs(vdecoldfs); 
}
#endif

#if (POWER_TEST_CASE == POWER_TEST_DCM)
// *********************************************************************
// Function    : void vVerTestDCM(UINT32 u4InstID)
// Description : Test DCM on/off (auto clock adjust or not)
// Parameter   : None
// Return      : None
// *********************************************************************
void vVerTestDCM(UINT32 u4InstID)
{
    UINT32 u4RegVal1 = 0;
    UINT32 u4RegVal2 = 0;

    // VDEC on GCON_0[0] = 1, off GCON_1[0] = 1
    // LARB on GCON_2[0] = 1, off GCON_3[0] = 1

    u4RegVal1 = u4ReadGconReg(0x0);
    u4RegVal2 = u4ReadGconReg(0x8);    
    printk("[PWR_TEST] DCM status 0x0 = 0x%x, 0x8 = 0x%x\n", u4RegVal1, u4RegVal2);

    // OFF
    vWriteGconReg(0x4, 1);
    vWriteGconReg(0xC, 1);

    u4RegVal1 = u4ReadGconReg(0x0);
    u4RegVal2 = u4ReadGconReg(0x8);
    printk("[PWR_TEST] DCM write off for 10 sec! 0x0 = 0x%x, 0x8 = 0x%x\n", u4RegVal1, u4RegVal2);
#if POWER_TEST_MANUAL_CHECK
    msleep(10000);
#endif

    // ON
    vWriteGconReg(0x0, 1);
    vWriteGconReg(0x8, 1);

    u4RegVal1 = u4ReadGconReg(0x0);
    u4RegVal2 = u4ReadGconReg(0x8);        
    printk("[PWR_TEST] DCM write on for 10 sec! 0x0 = 0x%x, 0x8 = 0x%x\n", u4RegVal1, u4RegVal2);
#if POWER_TEST_MANUAL_CHECK
    msleep(10000);
#endif
}
#endif

// *********************************************************************
// Function    : void vVerInitVDec(UINT32 u4InstID)
// Description : Dec procedure initilize
// Parameter   : None
// Return      : None
// *********************************************************************
void vVerInitVDec(UINT32 u4InstID)
{
    #ifdef REG_LOG_NEW
    _fgRegLogConsole[u4InstID] = TRUE;
    #endif
    if (_u4CodecVer[u4InstID] == VDEC_RM)
    {
      vRM_VerInitDec(u4InstID);
        // 
        if (_u4FileCnt[u4InstID] > 0)
	{
	    _u4FileCnt[u4InstID] = u4RM_PreParseIPic(u4InstID, _u4FileCnt[u4InstID]);
	}
    }
    else if (_u4CodecVer[u4InstID] == VDEC_H264)
    //if(_u4CodecVer[u4InstID] == VDEC_H264)
  {
    VDEC_INFO_H264_INIT_PRM_T rH264VDecInitPrm;
    VDEC_INFO_H264_BS_INIT_PRM_T rH264BSInitPrm;
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecH264DecPrm.ucMaxFBufNum = 0xff;
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecH264DecPrm.rLastInfo.u4LastPicW = 0;
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecH264DecPrm.rLastInfo.u4LastPicH = 0;  
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecH264DecPrm.prSEI = &_rSEI[u4InstID];
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecH264DecPrm.prFGTPrm= &_rFGTPrm[u4InstID]; 
    _u4TotalDecFrms[u4InstID] = 0;
    
    #if AVC_8320_SUPPORT
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecH264DecPrm.u4VLDWrapperWrok = (UINT32)_pucVLDWrapperWrok[u4InstID];
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecH264DecPrm.u4PPWrapperWrok = (UINT32)_pucPPWrapperWork[u4InstID];
    #endif
    
    vOutputPOCData(0xFFFFFFFF);
    vVerifyFlushBufInfo(u4InstID);
    vVerifyFlushAllSetData(u4InstID);
    vSetDecFlag(u4InstID, DEC_FLAG_CHG_FBUF);
  #ifdef VERIFICATION_FGT
    vAllocFGTTable(u4InstID);
  #endif
  #ifdef BARREL2_THREAD_SUPPORT
    VERIFY (x_sema_lock(_ahVDecEndSema[u4InstID], X_SEMA_OPTION_WAIT) == OSR_OK);
  #endif
    rH264VDecInitPrm.u4FGDatabase = (UINT32)_pucFGDatabase[u4InstID];
    rH264VDecInitPrm.u4CompModelValue = (UINT32)(_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecH264DecPrm.prSEI->pucCompModelValue);
    rH264VDecInitPrm.u4FGSeedbase = (UINT32)_pucFGSeedbase[u4InstID];
    i4VDEC_HAL_H264_InitVDecHW(u4InstID, &rH264VDecInitPrm);
    rH264BSInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
    rH264BSInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
    rH264BSInitPrm.u4VLDRdPtr = (UINT32)_pucVFifo[u4InstID];
    rH264BSInitPrm.u4VLDWrPtr = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
  #ifndef  RING_VFIFO_SUPPORT
    rH264BSInitPrm.u4VLDWrPtr = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
  #else
    rH264BSInitPrm.u4VLDWrPtr = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));
  #endif
    rH264BSInitPrm.u4PredSa = /*PHYSICAL*/((UINT32)_pucPredSa[u4InstID]);
    i4VDEC_HAL_H264_InitBarrelShifter(_u4BSID[u4InstID], u4InstID, &rH264BSInitPrm);
  #ifdef BARREL2_THREAD_SUPPORT
    VERIFY (x_sema_unlock(_ahVDecEndSema[u4InstID]) == OSR_OK);
  #endif
  }
  else if(_u4CodecVer[u4InstID] == VDEC_WMV)
  {
    vVerifyInitVParserWMV(u4InstID);
    vDEC_HAL_COMMON_SetVLDPower(u4InstID,ON);
    if(_i4CodecVersion[u4InstID] != VDEC_VC1)
    {
      vRCVFileHeader(u4InstID);
      if(_i4CodecVersion[u4InstID] == VDEC_WMV3)
      {
        _u4VprErr[u4InstID] = u4DecodeVOLHead_WMV3(u4InstID);
      }
      else if((_i4CodecVersion[u4InstID] == VDEC_WMV1) || (_i4CodecVersion[u4InstID] == VDEC_WMV2))
      {
        _u4VprErr[u4InstID] = u4DecodeVOLHead_WMV12(u4InstID);
      }
      _u4WMVBitCount[u4InstID] = pic_hdr_bitcount(u4InstID);
      _i4HeaderLen[u4InstID] = _u4WMVBitCount[u4InstID] / 8;
      _iSetPos[u4InstID] = _i4HeaderLen[u4InstID];
    }
    _u4PicHdrBits[u4InstID] = 0;
    _fgCounting[u4InstID] = FALSE;
  }
  else if(_u4CodecVer[u4InstID] == VDEC_VP6)
  {  
      vVerInitVP6(u4InstID);
  }
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
  else if(_u4CodecVer[u4InstID] == VDEC_VP8)
  {  
      vVerInitVP8(u4InstID);
  }
#endif  
  else if(_u4CodecVer[u4InstID] == VDEC_AVS)
  {  
      vVerInitAVS(u4InstID);
  }
  else
  {
    VDEC_INFO_MPEG_VFIFO_PRM_T rMPEGVDecInitPrm;
    VDEC_INFO_MPEG_BS_INIT_PRM_T rMPEGBSInitPrm;
    
    vInitVParserMPEG(u4InstID);
    rMPEGVDecInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
    rMPEGVDecInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
    rMPEGBSInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
    rMPEGBSInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
    rMPEGBSInitPrm.u4ReadPointer= (UINT32)_pucVFifo[u4InstID];
	rMPEGVDecInitPrm.u4CodeType = _u4CodecVer[u4InstID];
  #ifndef  RING_VFIFO_SUPPORT
    rMPEGBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
  #else
//    rMPEGBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ*(0.5 + 0.5 *(_u4LoadBitstreamCnt[u4InstID]%2)));
	rMPEGBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));//mtk40343
  #endif
    i4VDEC_HAL_MPEG_InitVDecHW(u4InstID, &rMPEGVDecInitPrm);
    i4VDEC_HAL_MPEG_InitBarrelShifter(_u4BSID[u4InstID], u4InstID, &rMPEGBSInitPrm);
    // Restore Quantization Matrix
    if(_fgVerLoadIntraMatrix[u4InstID])
    {
      vVDEC_HAL_MPEG_ReLoadQuantMatrix(u4InstID, TRUE);
    }
    if(_fgVerLoadNonIntraMatrix[u4InstID])
    {
      vVDEC_HAL_MPEG_ReLoadQuantMatrix(u4InstID, FALSE);
    }
    if(_u4CodecVer[u4InstID] == VDEC_MPEG2)
    {
      vDEC_HAL_COMMON_SetVLDPower(u4InstID,ON);
	  vVDec_HAL_CRC_Enable(u4InstID, 1);
      u4VParserMPEG12(u4InstID, TRUE);
      vDEC_HAL_COMMON_SetVLDPower(u4InstID,OFF);
    }
    else if((_u4CodecVer[u4InstID] == VDEC_MPEG4) || (_u4CodecVer[u4InstID] == VDEC_H263))
    {
      if(!_fgShortHeader[u4InstID])
      {
        vDEC_HAL_COMMON_SetVLDPower(u4InstID,ON);
        u4VParserMPEG4(u4InstID, TRUE);
        vDEC_HAL_COMMON_SetVLDPower(u4InstID,OFF);
      }
    }
    else if(_u4CodecVer[u4InstID] == VDEC_DIVX3)
    {
      UINT32 temp;
      _u4Divx3SetPos[u4InstID] += 8;
      // skip Compression and "SizeImage"
      temp = u4VDEC_HAL_MPEG_GetBitStreamShift(_u4BSID[u4InstID], u4InstID, 32);
      _u4DIVX3Width[u4InstID] = (((temp&0x00ff0000)>>16)<<8) + ((temp & 0xff000000)>>24);
      temp = u4VDEC_HAL_MPEG_GetBitStreamShift(_u4BSID[u4InstID], u4InstID, 32);
      _u4DIVX3Height[u4InstID] = (((temp&0x00ff0000)>>16)<<8) + ((temp & 0xff000000)>>24);
      vVerifySetUpParm(u4InstID, _u4DIVX3Width[u4InstID], _u4DIVX3Height[u4InstID], 4 /*FRC_29_97*/, FALSE, FALSE);
    }
    _tVerPic[u4InstID].u4W = _u4HSize[u4InstID];
    _tVerPic[u4InstID].u4H = _u4VSize[u4InstID];
    _tVerPic[u4InstID].ucMpegVer = _ucMpegVer[u4InstID];
  }
  vVerifyVDecIsrInit(u4InstID);
}

// for AVI or QT, we know frame rate from system layer
// for M4V, maybe set it to 4 (29.976Hz)

void vVerifySetUpParm(UINT32 u4InstID, UINT32 dwPicW, UINT32 dwPicH, UINT32 dwFrmRatCod, BOOL fgDivXM4v, BOOL fgDx4M4v)
{
  _u4RealHSize[u4InstID] = dwPicW;
  _u4RealVSize[u4InstID] = dwPicH;
  _u4UPicW[u4InstID] = dwPicW;
  _u4UPicH[u4InstID] = dwPicH;
  _u4UFrmRatCod[u4InstID] = dwFrmRatCod;
  _fgVerUDivXM4v[u4InstID] = fgDivXM4v;
  _fgVerUDx4M4v[u4InstID] = fgDx4M4v;
}

void vVerifyVDecIsrInit(UINT32 u4InstID)
{
  //BIM_DisableIrq(0xffffffff); //ginny mark it 071015
#ifndef IRQ_DISABLE  
  if (u4InstID == 0)
  {
    // reg ISR
    if (request_irq(MT8320_VDEC_IRQ, vVDec0IrqProc, 0, "VDEC0_VT", NULL))
    {
	ASSERT(0);
    }	
  }
  else if (u4InstID == 1)
  {
  
  }
#endif
}

void vVerifyVDecIsrStop(UINT32 u4InstID)
{
#ifndef IRQ_DISABLE 
 // x_os_isr_fct pfnOldIsr;

  if(u4InstID)
  {
    // dereg ISR
//    if (x_reg_isr(VECTOR_VDFUL, NULL, &pfnOldIsr) != OSR_OK)
    {
//      ASSERT(0);
    }
  }
  else
  {
    // dereg ISR
  }
#endif
}

void vVerifyInitVParserWMV(UINT32 u4InstID)
{
  VDEC_INFO_WMV_SEQ_PRM_T *prWMVSPS = &_rWMVSPS[u4InstID];
  VDEC_INFO_WMV_ETRY_PRM_T *prWMVEPS = &_rWMVEPS[u4InstID];
  VDEC_INFO_WMV_PIC_PRM_T *prWMVPPS = &_rWMVPPS[u4InstID];
  VDEC_INFO_WMV_ICOMP_PRM_T *prWMVICOMPPS = &_rWMVICOMPPS[u4InstID];
  
  _u4WMVBitCount[u4InstID] = 0;
  _u4WMVByteCount[u4InstID] = 0;
  reset_pic_hdr_bits(u4InstID);
  
  _u4DispBufIdx[u4InstID] = 1;
  if(_u4DispBufIdx[u4InstID] == 1)
  {
    vSetVerFRefBuf(u4InstID, 0);
    vSetVerBRefBuf(u4InstID, 1); 
  }
  else
  {
    vSetVerFRefBuf(u4InstID, 1);
    vSetVerBRefBuf(u4InstID, 0);
  }
  _u4WMVDecPicNo[u4InstID] = 0;

  //Sequence Header variables initialization
  //Advanced
  prWMVSPS->fgBroadcastFlags = FALSE;
  prWMVSPS->fgInterlacedSource = FALSE;
  prWMVSPS->fgTemporalFrmCntr = FALSE;

  prWMVSPS->fgSeqFrameInterpolation = FALSE;

  prWMVSPS->fgHRDPrmFlag = FALSE;
  prWMVSPS->i4HRDNumLeakyBuckets = 0;
  prWMVPPS->fgTopFieldFirst = TRUE; //ming
  //Simple & Main
  prWMVSPS->fgXintra8Switch = FALSE;
  prWMVSPS->fgMultiresEnabled = FALSE;
  prWMVSPS->i4ResIndex = 0;
  prWMVSPS->fgDCTTableMBEnabled = FALSE;
  prWMVSPS->fgPreProcRange = FALSE;
  prWMVSPS->i4NumBFrames = 1;
  prWMVSPS->fgRotatedIdct = FALSE;
  prWMVPPS->ucFrameCodingMode = PROGRESSIVE; //ming
   // WMV7 & WMV8
  prWMVSPS->fgMixedPel = FALSE;
  prWMVSPS->fgFrmHybridMVOn = FALSE;
  prWMVSPS->fgXintra8 = FALSE;

  //End of Sequence

  prWMVPPS->fgPostRC1 = TRUE;

  //EntryPoint Header variables initialization
  prWMVEPS->fgBrokenLink = FALSE;
  prWMVEPS->fgClosedEntryPoint = FALSE;
  prWMVEPS->fgPanScanPresent = FALSE;
  prWMVEPS->fgRefDistPresent = FALSE;
  prWMVEPS->fgLoopFilter = FALSE;
  prWMVEPS->fgUVHpelBilinear = FALSE;
  prWMVEPS->i4RangeState = 0;
  prWMVEPS->i4ReconRangeState = 0;
  prWMVEPS->fgExtendedMvMode = FALSE;
  prWMVEPS->i4MVRangeIndex = 0;
  prWMVEPS->i4DQuantCodingOn = 0;
  prWMVEPS->fgXformSwitch = FALSE;
  prWMVEPS->fgSequenceOverlap = FALSE;
  //Quant related
  prWMVEPS->fgExplicitSeqQuantizer = FALSE;
  prWMVEPS->fgExplicitFrameQuantizer = FALSE;
  prWMVEPS->fgExplicitQuantizer = FALSE;
  prWMVPPS->fgUse3QPDZQuantizer = FALSE;
  prWMVPPS->fgHalfStep = FALSE;

  prWMVEPS->fgExtendedDeltaMvMode = FALSE;
  prWMVEPS->i4DeltaMVRangeIndex = 0;
  prWMVEPS->i4ExtendedDMVX = 0;
  prWMVEPS->i4ExtendedDMVY = 0;
  prWMVEPS->i4RefFrameDistance = 0;

  prWMVPPS->i4BNumerator = 0;
  prWMVSPS->i4NumBFrames = 1;
  prWMVPPS->i4DCStepSize = 0;
  prWMVPPS->i4X9MVMode = 0;

  prWMVPPS->fgMBXformSwitching = FALSE;
  prWMVPPS->i4FrameXformMode = 0;
    

  prWMVEPS->fgRangeRedYFlag = FALSE;
  prWMVEPS->fgRangeRedUVFlag = FALSE;
//End of EntryPoint


  prWMVPPS->u4ForwardRefPicType = 0;
  prWMVPPS->u4BackwardRefPicType = 0;

  // Picture Header
  //WMV7 & WMV8
  prWMVPPS->fgDCPredIMBInPFrame = FALSE;
  //for field pictures

  prWMVPPS->fgTopFieldFirst = TRUE;
  prWMVPPS->fgRepeatFirstField = FALSE;
  //FALSE for PROGRESSIVE.
  prWMVPPS->fgInterlaceV2 = FALSE; 
  prWMVPPS->fgFieldMode = FALSE;
  prWMVPPS->i4CurrentField = 0; // 0:TOP, 1:BOTTOM field
  prWMVPPS->i4CurrentTemporalField = 0; // 0:1st field or frame picture, 1: 2nd field

  prWMVPPS->i4MaxZone1ScaledFarMVX = 0;
  prWMVPPS->i4MaxZone1ScaledFarMVY = 0;
  prWMVPPS->i4Zone1OffsetScaledFarMVX = 0;
  prWMVPPS->i4Zone1OffsetScaledFarMVY = 0;
  prWMVPPS->i4FarFieldScale1 = 0;
  prWMVPPS->i4FarFieldScale2 = 0;
  prWMVPPS->i4NearFieldScale = 0;
  prWMVPPS->i4MaxZone1ScaledFarBackMVX = 0; 
  prWMVPPS->i4MaxZone1ScaledFarBackMVY = 0; 
  prWMVPPS->i4Zone1OffsetScaledFarBackMVX = 0;
  prWMVPPS->i4Zone1OffsetScaledFarBackMVY = 0;
  prWMVPPS->i4FarFieldScaleBack1 = 0; 
  prWMVPPS->i4FarFieldScaleBack2 = 0;   
  prWMVPPS->i4NearFieldScaleBack = 0;

  prWMVPPS->fgTwoRefPictures = TRUE;
  prWMVPPS->fgUseOppFieldForRef = TRUE;
  prWMVPPS->fgUseSameFieldForRef = TRUE;
  //Robert TODO: 0511
  prWMVPPS->fgBackRefUsedHalfPel = FALSE;
  prWMVPPS->fgBackRefTopFieldHalfPelMode = FALSE;
  prWMVPPS->fgBackRefBottomFieldHalfPelMode = FALSE;

  prWMVPPS->fgMvResolution = FALSE;

  prWMVPPS->i4Overlap = 0;
  prWMVPPS->i4MvTable = 0;
  prWMVPPS->i4CBPTable = 0;
  prWMVPPS->i4MBModeTable = 0;
  prWMVPPS->i42MVBPTable = 0;
  prWMVPPS->i44MVBPTable = 0;

  //!WMVA profile
  prWMVSPS->fgPreProcRange = FALSE;
  

  prWMVEPS->fgNewDCQuant = FALSE;

  prWMVPPS->fgDCTTableMB = FALSE;

 // WMV7 & WMV8
 //Robert TODO:
  if(_i4CodecVersion[u4InstID] == VDEC_WMV2)
  {
    prWMVSPS->fgSkipBitCoding = TRUE;
    prWMVSPS->fgNewPcbPcyTable = TRUE;
  }
  else
  {
    prWMVSPS->fgSkipBitCoding = FALSE;
    prWMVSPS->fgNewPcbPcyTable = FALSE;
  }

  prWMVSPS->fgCODFlagOn = TRUE;
  

  if(_i4CodecVersion[u4InstID] >= VDEC_WMV3)
  {
    prWMVEPS->fgNewDCQuant = TRUE;
  }

  ComputeDQuantDecParam(u4InstID);

  prWMVPPS->ucDiffQtProfile = 0;

  _iSeqHdrData1[u4InstID] = 0;
  _iSeqHdrData2[u4InstID] = 0;
  prWMVSPS->i4SkipBitModeV87 = 0;
  prWMVSPS->i4Wmv8BpMode = 0;

  //NEEDS to initialize
  _new_entry_point[u4InstID] = 0;
  prWMVSPS->fgPostProcInfoPresent = FALSE;
  prWMVSPS->fgYUV411 = FALSE;
  prWMVSPS->fgSpriteMode = FALSE;
  prWMVEPS->i4RangeRedY = 0;
  prWMVEPS->i4RangeMapUV = 0;
  prWMVPPS->ucRepeatFrameCount = 0;
  prWMVPPS->ucDQuantBiLevelStepSize = 0;
  prWMVPPS->fgDQuantOn = FALSE;
  prWMVPPS->i4Panning = 0;
  prWMVPPS->fgDQuantBiLevel = FALSE;

  prWMVICOMPPS->i4ResetMvDram = 0;
  prWMVICOMPPS->i4SecondFieldParity = 0;
  prWMVICOMPPS->i4BoundaryUMVIcomp = 0;

  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecWMVDecPrm.prSPS = &_rWMVSPS[u4InstID];
  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecWMVDecPrm.prEPS = &_rWMVEPS[u4InstID];
  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecWMVDecPrm.prPPS = &_rWMVPPS[u4InstID];
  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecWMVDecPrm.prICOMPS = &_rWMVICOMPPS[u4InstID];
  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecWMVDecPrm.fgWmvMode = _u4WmvMode[u4InstID];

  #if (WMV_8320_SUPPORT)
  #if WMV_LOG_TMP
  printk("vVerifyInitVParserWMV, u4VLDWrapperWrok = 0x%x\n", _pucVLDWrapperWrok[u4InstID]);
  printk("vVerifyInitVParserWMV, u4PPWrapperWrok = 0x%x\n", _pucPPWrapperWork[u4InstID]);
  #endif
    
  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecWMVDecPrm.rWmvWorkBufSa.u4VLDWrapperWrok = (UINT32)_pucVLDWrapperWrok[u4InstID];
  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecWMVDecPrm.rWmvWorkBufSa.u4PPWrapperWrok = (UINT32)_pucPPWrapperWork[u4InstID];
  #endif
}

void vInitVParserMPEG(UINT32 u4InstID)
{
  _u4Divx3SetPos[u4InstID] = 0;
  _u4BrokenLink[u4InstID] = 2;
  _u4DispBufIdx[u4InstID] = 1;
  if(_u4DispBufIdx[u4InstID] == 1)
  {
    vSetVerFRefBuf(u4InstID, 0);
    vSetVerBRefBuf(u4InstID, 1); 
  }
  else
  {
    vSetVerFRefBuf(u4InstID, 1);
    vSetVerBRefBuf(u4InstID, 0);
  }
  _u4Datain[u4InstID] = 0;          // for dwGetBitStream() return value
  _u4BitCount[u4InstID] = 0;        // for dwGetBitStream() byte aligned

  _fgVerSeqHdr[u4InstID]=0;            // sequence header process
  _ucMpegVer[u4InstID] = VDEC_UNKNOWN;

  _u4HSize[u4InstID]=0;             // horizontal size = horizontal size value +
                          // horizontal size extension << 12
  _u4VSize[u4InstID]=0;             // vertical size = vertical size value +
                          //                 vertical size extension << 12
  _ucParW[u4InstID] = 1;
  _ucParH[u4InstID] = 1;
  _u4HSizeVal[u4InstID]=0;          // horizontal size value
  _u4VSizeVal[u4InstID]=0;          // vertical size value

  _fgVerProgressiveSeq[u4InstID]=0;    //progressive_sequence
  _u4PicWidthMB[u4InstID]=0;
  _u4BPicIniFlag[u4InstID]=0;
  _u4BPicIniFlag0[u4InstID]=0;

  //_ucHSizeExt=0;           // horizontal size extension
  //_ucVSizeExt=0;           // vertical size extension
  _ucCrmaFmt[u4InstID]=0;            //chroma_format;
  _ucFullPelFordVec[u4InstID]=0;     // full_pel_forward_vector;
  _ucFordFCode[u4InstID]=0;          // forward_f_code;
  _ucFullPelBackVec[u4InstID]=0;     // full_pel_backward_vector;
  _ucBackFCode[u4InstID]=0;          // backward_f_code;
  _ucIntraDcPre[u4InstID]=0;         // intra_dc_precision;
  _fgVerAltScan[u4InstID]=0;
  _fgVerQScaleType[u4InstID]=0;        // q_scale_type;
  _fgVerFrmPredFrmDct[u4InstID]=0;     // frame_pred_frame_dct;
  _fgVerIntraVlcFmt[u4InstID]=0;
  _fgVerConcealMotionVec[u4InstID]=0;  // concealment_motion_vectors;
  _pucfcode[u4InstID][0][0]=0;
  _pucfcode[u4InstID][0][1]=0;
  _pucfcode[u4InstID][1][0]=0;
  _pucfcode[u4InstID][1][1]=0;
  _u4PicPSXOff[u4InstID] = 0xFFFFFFFF;
  //_dwOldPicPSXSkip = 0xFFFFFFFF;

  _fgVerLoadIntraMatrix[u4InstID] = 0;
  _fgVerLoadNonIntraMatrix[u4InstID] = 0;

  //_bLastPicBBufMd=0;      // 1 for 8-line mode, 0 for 16-line mode

  _fgVerBrokenLink[u4InstID] = FALSE;
  _fgVerClosedGop[u4InstID] = FALSE;

  _u4UserDataCodecVersion[u4InstID] = 0;
  _u4UserDataBuildNumber[u4InstID] = 0;
  _u4TimeBase[u4InstID] = 0;
  _fgVerShortVideoHeader[u4InstID] = FALSE;
  _fgSorenson[u4InstID] = FALSE;
  _ucSourceFormat[u4InstID] = 0;
  _ucVisualObjectVerid[u4InstID] = 1;
  _fgVerQuarterSample[u4InstID] = FALSE;
  _fgVerReversibleVlc[u4InstID] = FALSE;
  _fgVerReducedResolutionVopEnable[u4InstID] = FALSE;
  _rDirMode[u4InstID].u4TFrm = 0xffffffff;
  _fgVerTopFldFirst[u4InstID] = FALSE;

  _rMPEG4VopPrm[u4InstID].prDirMd = &_rDirMode[u4InstID];
  _rMPEG4VopPrm[u4InstID].prGmcPrm = &_rMPEG4GmcPrm[u4InstID];
  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rDep.rM4vDecPrm.prVol = &_rMPEG4VolPrm[u4InstID];
  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rDep.rM4vDecPrm.prVop = &_rMPEG4VopPrm[u4InstID];
}

void vSetWMVDecParam( UINT32 u4InstID, VDEC_INFO_DEC_PRM_T *tVerMpvDecPrm)
{
  
  tVerMpvDecPrm->SpecDecPrm.rVDecWMVDecPrm.i4CodecVersion = _i4CodecVersion[u4InstID];
  tVerMpvDecPrm->SpecDecPrm.rVDecWMVDecPrm.i4MemBase = 0;
  
if (!tVerMpvDecPrm->SpecDecPrm.rVDecWMVDecPrm.fgWmvMode)
{
  tVerMpvDecPrm->SpecDecPrm.rVDecWMVDecPrm.rWmvWorkBufSa.u4Bp1Sa = (UINT32)_pucBp_1[u4InstID];
  tVerMpvDecPrm->SpecDecPrm.rVDecWMVDecPrm.rWmvWorkBufSa.u4Bp2Sa = (UINT32)_pucBp_2[u4InstID];
  tVerMpvDecPrm->SpecDecPrm.rVDecWMVDecPrm.rWmvWorkBufSa.u4Bp3Sa = (UINT32)_pucBp_3[u4InstID];
  tVerMpvDecPrm->SpecDecPrm.rVDecWMVDecPrm.rWmvWorkBufSa.u4Bp4Sa = (UINT32)_pucBp_4[u4InstID];
  tVerMpvDecPrm->SpecDecPrm.rVDecWMVDecPrm.rWmvWorkBufSa.u4Dcac2Sa = (UINT32)_pucDcac_2[u4InstID];
  tVerMpvDecPrm->SpecDecPrm.rVDecWMVDecPrm.rWmvWorkBufSa.u4DcacSa = (UINT32)_pucDcac[u4InstID];
  tVerMpvDecPrm->SpecDecPrm.rVDecWMVDecPrm.rWmvWorkBufSa.u4Mv12Sa = (UINT32)_pucMv_1_2[u4InstID];
  tVerMpvDecPrm->SpecDecPrm.rVDecWMVDecPrm.rWmvWorkBufSa.u4Mv1Sa = (UINT32)_pucMv_1[u4InstID];
  tVerMpvDecPrm->SpecDecPrm.rVDecWMVDecPrm.rWmvWorkBufSa.u4Mv2Sa = (UINT32)_pucMv_2[u4InstID];
  tVerMpvDecPrm->SpecDecPrm.rVDecWMVDecPrm.rWmvWorkBufSa.u4Mv3Sa = (UINT32)_pucMv_3[u4InstID];
}  
else
{
  tVerMpvDecPrm->SpecDecPrm.rVDecWMVDecPrm.rWmvWorkBufSa.u4DcacNewSa = (UINT32)_pucDcacNew[u4InstID];
  tVerMpvDecPrm->SpecDecPrm.rVDecWMVDecPrm.rWmvWorkBufSa.u4MvNewSa = (UINT32)_pucMvNew[u4InstID];
  tVerMpvDecPrm->SpecDecPrm.rVDecWMVDecPrm.rWmvWorkBufSa.u4Bp0NewSa = (UINT32)_pucBp0New[u4InstID];
  tVerMpvDecPrm->SpecDecPrm.rVDecWMVDecPrm.rWmvWorkBufSa.u4Bp1NewSa = (UINT32)_pucBp1New[u4InstID];
  tVerMpvDecPrm->SpecDecPrm.rVDecWMVDecPrm.rWmvWorkBufSa.u4Bp2NewSa = (UINT32)_pucBp2New[u4InstID];
}
//#endif

  tVerMpvDecPrm->SpecDecPrm.rVDecWMVDecPrm.rWmvWorkBufSa.u4Pic0CSa = (UINT32)_pucPic0C[u4InstID];
  tVerMpvDecPrm->SpecDecPrm.rVDecWMVDecPrm.rWmvWorkBufSa.u4Pic0YSa = (UINT32)_pucPic0Y[u4InstID];
  tVerMpvDecPrm->SpecDecPrm.rVDecWMVDecPrm.rWmvWorkBufSa.u4Pic1CSa = (UINT32)_pucPic1C[u4InstID];
  tVerMpvDecPrm->SpecDecPrm.rVDecWMVDecPrm.rWmvWorkBufSa.u4Pic1YSa = (UINT32)_pucPic1Y[u4InstID];
  tVerMpvDecPrm->SpecDecPrm.rVDecWMVDecPrm.rWmvWorkBufSa.u4Pic2CSa = (UINT32)_pucPic2C[u4InstID];
  tVerMpvDecPrm->SpecDecPrm.rVDecWMVDecPrm.rWmvWorkBufSa.u4Pic2YSa = (UINT32)_pucPic2Y[u4InstID];
  tVerMpvDecPrm->SpecDecPrm.rVDecWMVDecPrm.rWmvWorkBufSa.u4Pp1Sa = (UINT32)_pucPp_1[u4InstID];
  tVerMpvDecPrm->SpecDecPrm.rVDecWMVDecPrm.rWmvWorkBufSa.u4Pp2Sa = (UINT32)_pucPp_2[u4InstID];
  tVerMpvDecPrm->ucDecFBufIdx = BYTE0(_u4DecBufIdx[u4InstID]);
  tVerMpvDecPrm->SpecDecPrm.rVDecWMVDecPrm.u4FRefBufIdx = _u4FRefBufIdx[u4InstID];
}

BOOL fgVDecProcWMV(UINT32 u4InstID)
{
  UINT32 u4Bytes,u4Bits;
  VDEC_INFO_WMV_BS_INIT_PRM_T rWmvBSInitPrm;
  VDEC_INFO_DEC_PRM_T *tVerMpvDecPrm;
  
  tVerMpvDecPrm = &_tVerMpvDecPrm[u4InstID];
  vSetWMVDecParam(u4InstID, tVerMpvDecPrm);


  //Log Input Window before Trigger VDec
{
  UINT32 u4InputWindow = 0;

  u4InputWindow = u4VDecReadVLD(u4InstID, 0xf0); 
  printk("fgVDecProcWMV, Input Window %x \n", u4InputWindow);
}
  
#ifdef VERIFICATION_DOWN_SCALE
  #ifdef DOWN_SCALE_SUPPORT
    //vSetDownScaleParam(u4InstID, TRUE, &_tDownScalerPrm[u4InstID]);
    vSetDownScaleParam(u4InstID, TRUE, &(_tVerMpvDecPrm[u4InstID].rDownScalerPrm));
  #else
    //vSetDownScaleParam(u4InstID, FALSE, &_tDownScalerPrm[u4InstID]);
    vSetDownScaleParam(u4InstID, FALSE, &(_tVerMpvDecPrm[u4InstID].rDownScalerPrm));
  #endif
  //vDEC_HAL_COMMON_SetDownScaler(u4InstID, &_tDownScalerPrm[u4InstID]);
  //vVDECSetDownScalerPrm(u4InstID, &_tDownScalerPrm[u4InstID]);
  vVDECSetDownScalerPrm(u4InstID, &(_tVerMpvDecPrm[u4InstID].rDownScalerPrm));
#endif
  
  if(_u4BSID[u4InstID] == 1)
  {
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
    if (_i4CodecVersion[u4InstID] == VDEC_VC1)
    {
       i4VDEC_HAL_WMV_InitBarrelShifter(0, u4InstID, &rWmvBSInitPrm, TRUE);
    }
    else
    {
       i4VDEC_HAL_WMV_InitBarrelShifter(0, u4InstID, &rWmvBSInitPrm, FALSE);
    }
    u4VDEC_HAL_WMV_ShiftGetBitStream(0, u4InstID, u4Bits);
  }
  if(_i4CodecVersion[u4InstID] == VDEC_VC1)
  {
    i4VDEC_HAL_WMV_DecStart(u4InstID, tVerMpvDecPrm); //umv_from_mb = 0 for WMVA
    if((_rWMVPPS[u4InstID].ucFrameCodingMode != INTERLACEFIELD) || (_rWMVPPS[u4InstID].i4CurrentTemporalField == 1))
    {
      _u4WMVDecPicNo[u4InstID]++;
    }
  }
  else
  {
    i4VDEC_HAL_WMV_DecStart(u4InstID, tVerMpvDecPrm); //umv_from_mb = 1 for !WMVA
    _u4WMVDecPicNo[u4InstID]++;
  }

  return (TRUE);
}

void vCodecVersion(UINT32 u4InstID, UINT32 u4CodecFOURCC)
{
  VDEC_INFO_WMV_SEQ_PRM_T *prWMVSPS = &_rWMVSPS[u4InstID];
  
  prWMVSPS->fgVC1 = FALSE;
  if(u4CodecFOURCC == FOURCC_WVC1_WMV)
  {
    u4CodecFOURCC = FOURCC_WMVA_WMV;
    prWMVSPS->fgVC1 = TRUE;
  }

  if((u4CodecFOURCC == FOURCC_WMV1_WMV) || (u4CodecFOURCC == FOURCC_wmv1_WMV))
    _i4CodecVersion[u4InstID] = VDEC_WMV1;
  else if((u4CodecFOURCC == FOURCC_WMV2_WMV) || (u4CodecFOURCC == FOURCC_wmv2_WMV))
    _i4CodecVersion[u4InstID] = VDEC_WMV2;
  else if((u4CodecFOURCC == FOURCC_WMV3_WMV) || (u4CodecFOURCC == FOURCC_wmv3_WMV))
    _i4CodecVersion[u4InstID] = VDEC_WMV3;
  else if((u4CodecFOURCC == FOURCC_WMVA_WMV) || (u4CodecFOURCC == FOURCC_wmva_WMV))
    _i4CodecVersion[u4InstID] = VDEC_VC1;
  else
  {
    vVDecOutputDebugString("WMV Codec Error\n");
  }
}

void vSetFGTParam(UINT32 u4InstID, VDEC_INFO_H264_FGT_PRM_T *prFGTPrm)
{
  //VDEC_INFO_DEC_PRM_T *ptVerMpvDecPrm;
  //ptVerMpvDecPrm = &_tVerMpvDecPrm[u4InstID]; 

#ifdef FGT_SUPPORT
    VDEC_INFO_DEC_PRM_T *ptVerMpvDecPrm;
    ptVerMpvDecPrm = &_tVerMpvDecPrm[u4InstID]; 
  
    prFGTPrm->ucDataScr = FGT_EN | FGT_SCR_PP; // If Cancel flag is TRUE, it will be bypass mode
#ifdef DOWN_SCALE_SUPPORT
    prFGTPrm->ucDataScr |= FGT_VDSCL_BUSY_EN; 
#endif
    prFGTPrm->pucFGTScrYAddr = _pucDecWorkBuf[u4InstID];
    prFGTPrm->pucFGTScrCAddr = _pucDecWorkBuf[u4InstID] + _ptCurrFBufInfo[u4InstID]->u4DramPicSize;
    prFGTPrm->pucFGTTrgYAddr = _pucFGTBuf[u4InstID];
    prFGTPrm->pucFGTTrgCAddr = _pucFGTBuf[u4InstID] + _ptCurrFBufInfo[u4InstID]->u4DramPicSize;
    prFGTPrm->ucMBXSize =  ((_ptCurrFBufInfo[u4InstID]->u4W + 15)>> 4);
    prFGTPrm->ucMBYSize =  (((_ptCurrFBufInfo[u4InstID]->u4H >> (1-(fgIsFrmPic(u4InstID)))) + 15)>> 4);
    prFGTPrm->u4Ctrl =  ((ptVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSEI->u4NumModelValuesMinus1[0] & 0x3) << 0) |
                                                             ((ptVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSEI->u4NumModelValuesMinus1[1] & 0x3) << 2) |
                                                             ((ptVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSEI->u4NumModelValuesMinus1[2] & 0x3) << 4) |
                                                             ((ptVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSEI->u4NumModelValuesMinus1[2] & 0x3) << 4) |
                                                             (ptVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSEI->fgFilmGrainCharacteristicsCancelFlag << 8) |
                                                             (ptVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSEI->fgCompModelPresentFlag[0] << 9) |              
                                                             (ptVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSEI->fgCompModelPresentFlag[1] << 10) |
                                                             (ptVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSEI->fgCompModelPresentFlag[2] << 11) |
                                                             ((ptVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSEI->u4Log2ScaleFactor&0xfff) << 12) |
                                                             ((_ptCurrFBufInfo[u4InstID]->i4POC & 0xff) << 16) |
                                                             (((ptVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4IdrPicId) & 0x7) << 24);
#else
    prFGTPrm->ucDataScr = 0;
#endif
}

void vSaveDownScaleParam(UINT32 u4InstID, VDEC_INFO_VDSCL_PRM_T *prDownScalerPrm)
{
  _tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucPicStruct = prDownScalerPrm->ucPicStruct;
  _tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucScanType = prDownScalerPrm->ucScanType;
  _tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucScrAgent = prDownScalerPrm->ucScrAgent;
  _tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucSpectType = prDownScalerPrm->ucSpectType;
  _tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucVdoFmt = prDownScalerPrm->ucVdoFmt;
  _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4DispW = prDownScalerPrm->u4DispW;
  _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4SrcHeight = prDownScalerPrm->u4SrcHeight;
  _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4SrcWidth = prDownScalerPrm->u4SrcWidth;
  _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4TrgHeight = prDownScalerPrm->u4TrgHeight;
  _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4TrgWidth = prDownScalerPrm->u4TrgWidth;
  _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4TrgOffH = prDownScalerPrm->u4TrgOffH;
  _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4TrgOffV = prDownScalerPrm->u4TrgOffV;
  _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4TrgYAddr = prDownScalerPrm->u4TrgYAddr; 
  _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4TrgCAddr = prDownScalerPrm->u4TrgCAddr;
  _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4WorkAddr = prDownScalerPrm->u4WorkAddr;
}

void vSetDownScaleParam(UINT32 u4InstID, BOOL fgEnable, VDEC_INFO_VDSCL_PRM_T *prDownScalerPrm)
{
#ifdef VERIFICATION_DOWN_SCALE
  UINT32 dwPicWidthDec,dwPicHeightDec,u4DramPicSize;
  VDEC_INFO_DEC_PRM_T *ptVerMpvDecPrm;
  ptVerMpvDecPrm = &_tVerMpvDecPrm[u4InstID]; 

  dwPicWidthDec = _tVerMpvDecPrm[u4InstID].u4PicW;
  dwPicHeightDec = _tVerMpvDecPrm[u4InstID].u4PicH;
  if(fgEnable)
  {
    prDownScalerPrm->fgMbaff  = FALSE;
    prDownScalerPrm->fgDSCLEn = TRUE;    
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
    prDownScalerPrm->ucAddrSwapMode = _tVerMpvDecPrm[u4InstID].ucAddrSwapMode ^ 0x4;
#else
    prDownScalerPrm->ucAddrSwapMode = ADDRSWAP_OFF;
#endif

    prDownScalerPrm->fgLumaKeyEn = _fgVDSCLEnableLumaKeyTest[u4InstID];       
    prDownScalerPrm->u2LumaKeyValue= (UINT16) (((UINT32) rand())%256);

    if(_u4CodecVer[u4InstID] == VDEC_WMV)
    {
      if(_rWMVPPS[u4InstID].ucFrameCodingMode == INTERLACEFIELD)
      {
        if(_rWMVPPS[u4InstID].i4CurrentField == 1)
        {
          prDownScalerPrm->ucPicStruct = BOTTOM_FIELD;
        }
        else
        {
          prDownScalerPrm->ucPicStruct = TOP_FIELD;
        }
      }
      else if(_rWMVPPS[u4InstID].ucFrameCodingMode == INTERLACEFRAME)
      {
        prDownScalerPrm->ucPicStruct = TOP_BOTTOM_FIELD;
      }
      else
      {
        prDownScalerPrm->ucPicStruct = FRAME;
      }
      if ((_rWMVEPS[u4InstID].fgLoopFilter == 1) || (_rWMVPPS[u4InstID].i4Overlap & 1))
      {

         if (prDownScalerPrm->fgLumaKeyEn)
            prDownScalerPrm->ucScrAgent = RW_VDSCL_SRC_MC >> 2; //WMV+Luma_Key Only support MC out
         else
            prDownScalerPrm->ucScrAgent = RW_VDSCL_SRC_PP >> 2;

        prDownScalerPrm->fgEnColorCvt = FALSE;
      }
      else
      {
        prDownScalerPrm->ucScrAgent = RW_VDSCL_SRC_MC >> 2;
        prDownScalerPrm->fgEnColorCvt = (BOOL) (((UINT32) rand())&0x1);//random(2);
      }
    }
    else if(_u4CodecVer[u4InstID] == VDEC_H264)
    {
      prDownScalerPrm->ucPicStruct = ptVerMpvDecPrm->ucPicStruct;
      prDownScalerPrm->fgMbaff = ptVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSPS->fgMbAdaptiveFrameFieldFlag;
    }
    else 
    {
      if(_fgVerProgressiveFrm[u4InstID] || _fgVerProgressiveSeq[u4InstID])
      {
        prDownScalerPrm->ucPicStruct = 3;
      }
      else
      {
        prDownScalerPrm->ucPicStruct = (ptVerMpvDecPrm->ucPicStruct > 3) ? 
          (ptVerMpvDecPrm->ucPicStruct - 3) : ptVerMpvDecPrm->ucPicStruct;
      }
    }
    
    if((prDownScalerPrm->ucPicStruct == TOP_FIELD) || (prDownScalerPrm->ucPicStruct == BOTTOM_FIELD))
    {
      dwPicHeightDec = (dwPicHeightDec >> 1);
    }
    if(((_u4CodecVer[u4InstID] == VDEC_WMV)&&(_rWMVPPS[u4InstID].i4CurrentTemporalField==0))||
      ((_u4CodecVer[u4InstID] == VDEC_H264)&&(fgIsDecFlagSet(u4InstID, DEC_FLAG_CHG_FBUF)))||
      ((!_fgDec2ndFldPic[u4InstID])&&(_u4CodecVer[u4InstID] != VDEC_WMV)&&(_u4CodecVer[u4InstID] != VDEC_H264)))
    {
      if(_u4CodecVer[u4InstID] == VDEC_WMV)
      {
        prDownScalerPrm->ucSpectType = RW_VDSCL_SPEC_WMV >> 5;
        prDownScalerPrm->fgYOnly = 0;
        if ((_rWMVEPS[u4InstID].fgLoopFilter == 1) || (_rWMVPPS[u4InstID].i4Overlap & 1))
        {
             if (prDownScalerPrm->fgLumaKeyEn)
                prDownScalerPrm->ucScrAgent = RW_VDSCL_SRC_MC >> 2; //WMV+Luma_Key Only support MC out
             else
                prDownScalerPrm->ucScrAgent = RW_VDSCL_SRC_PP >> 2;

          prDownScalerPrm->fgEnColorCvt = FALSE;
        }
        else
        {
          prDownScalerPrm->ucScrAgent = RW_VDSCL_SRC_MC >> 2;
          prDownScalerPrm->fgEnColorCvt = (BOOL) (((UINT32) rand())&0x1);//random(2);
        }
      }
      else if(_u4CodecVer[u4InstID] == VDEC_H264)
      {
        prDownScalerPrm->ucSpectType = RW_VDSCL_SPEC_264 >> 5;
        prDownScalerPrm->fgYOnly = (fgIsMonoPic(u4InstID)? (RW_VDSCL_Y_ONLY>>7): 0);
      #ifdef FGT_SUPPORT
        prDownScalerPrm->ucScrAgent = RW_VDSCL_SRC_FG >> 2;
      #else
        prDownScalerPrm->ucScrAgent = RW_VDSCL_SRC_PP >> 2;
      #endif  
        prDownScalerPrm->fgEnColorCvt = FALSE;
      }
      else
      {
        prDownScalerPrm->ucSpectType = RW_VDSCL_SPEC_MPEG >> 5;
        prDownScalerPrm->fgYOnly = 0;
          if((_u4CodecVer[u4InstID] == VDEC_MPEG2) && _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rMpegPpInfo.fgPpEnable)
          {
              prDownScalerPrm->ucScrAgent = RW_VDSCL_SRC_PP >> 2;
              prDownScalerPrm->fgEnColorCvt = FALSE;
          }
          else
          {
        prDownScalerPrm->ucScrAgent = RW_VDSCL_SRC_MC >> 2;
        prDownScalerPrm->fgEnColorCvt = (BOOL) (((UINT32) rand())&0x1);//random(2);
      }
      }
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
        prDownScalerPrm->ucScanType = (UCHAR) (((UINT32) rand())&0x1);//random(2);
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8550)
        prDownScalerPrm->ucVdoFmt = 0;
#else
        prDownScalerPrm->ucVdoFmt = (UCHAR) (((UINT32) rand())&0x1);//random(2);
#endif
  
        while(TRUE)
        {
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8550)
          if (dwPicWidthDec > 960)
          {
             prDownScalerPrm->u4TrgWidth = (UINT32) (((UINT32) rand())%960)+0x10;
             
             if (prDownScalerPrm->u4TrgWidth > 960)
                prDownScalerPrm->u4TrgWidth = 960;
             
          }
          else
#endif
          {
             prDownScalerPrm->u4TrgWidth = (UINT32) (((UINT32) rand())%dwPicWidthDec)+0x10;
          }

          prDownScalerPrm->u4TrgHeight = (UINT32) (((UINT32) rand())%dwPicHeightDec)+0x10; 
          if(((prDownScalerPrm->u4TrgWidth%2)==0)
            &&((prDownScalerPrm->u4TrgHeight%4)==0)
            &&(prDownScalerPrm->u4TrgWidth <= dwPicWidthDec)
            &&(prDownScalerPrm->u4TrgHeight <= (dwPicHeightDec>>(prDownScalerPrm->ucVdoFmt)))
          )
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
        if(prDownScalerPrm->ucScrAgent == (RW_VDSCL_SRC_PP >> 2))
        {
          if(prDownScalerPrm->ucSpectType == (RW_VDSCL_SPEC_WMV >> 5))
          {
            #if 0
            if((prDownScalerPrm->u4SrcWidth>>1) >= prDownScalerPrm->u4TrgWidth)
            {
              prDownScalerPrm->u4SrcYOffH = (UINT32)(rand()%7); 
              prDownScalerPrm->u4SrcCOffH = (UINT32)(rand()%4); 
            }
            #endif
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
            #if 0
            if((prDownScalerPrm->u4SrcWidth>>1) >= prDownScalerPrm->u4TrgWidth)
            {
              prDownScalerPrm->u4SrcYOffH = (UINT32)(rand()%9);
              prDownScalerPrm->u4SrcCOffH = (UINT32)(rand()%5); 
            }
            #endif
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
          #if 0
          if((prDownScalerPrm->u4SrcWidth>>1) >= prDownScalerPrm->u4TrgWidth)
          {
            prDownScalerPrm->u4SrcYOffH = (UINT32)(rand()%7);
            prDownScalerPrm->u4SrcCOffH = (UINT32)(rand()%4); 
          }
          #endif
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
        if((prDownScalerPrm->u4TrgWidth + prDownScalerPrm->u4TrgOffH) < 1920 )
        {
          prDownScalerPrm->u4DispW = ((((prDownScalerPrm->u4TrgWidth + prDownScalerPrm->u4TrgOffH  + (((UINT32) rand())%(1920 - prDownScalerPrm->u4TrgWidth - prDownScalerPrm->u4TrgOffH  )))+15)>>4)<<4);       
        }
        else
        {
          prDownScalerPrm->u4DispW = 1920;
        }      
        prDownScalerPrm->u4TrgBufLen = prDownScalerPrm->u4DispW;      
      }
      else
      {
        //srand(IO_READ32(PARSER_BASE,0x4C)&0xFFFF);
        prDownScalerPrm->ucScanType = 0;//random(2);    
        prDownScalerPrm->ucVdoFmt = 0;//random(2);
        //prDownScalerPrm->u4DispW = (((dwPicWidthDec + 15) >> 4) << 4);       
        //prDownScalerPrm->u4TrgBufLen = prDownScalerPrm->u4DispW;
        while(TRUE)
        {
          prDownScalerPrm->u4TrgWidth = dwPicWidthDec; //random(dwPicWidthDec) + 0x40;
          prDownScalerPrm->u4TrgHeight = dwPicHeightDec;//random(dwPicHeightDec) + 0x30;
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
        if((prDownScalerPrm->u4TrgWidth + prDownScalerPrm->u4TrgOffH) < 1920 )
        {
          prDownScalerPrm->u4DispW = ((((prDownScalerPrm->u4TrgWidth + prDownScalerPrm->u4TrgOffH  + (((UINT32) rand())%(1920 - prDownScalerPrm->u4TrgWidth - prDownScalerPrm->u4TrgOffH  )))+15)>>4)<<4);       
        }
        else
        {
          prDownScalerPrm->u4DispW = 1920;
        }      

        prDownScalerPrm->u4DispW = (((dwPicWidthDec + 15) >> 4) << 4);
        prDownScalerPrm->u4TrgBufLen = prDownScalerPrm->u4DispW;
      #if 0
        prDownScalerPrm->u4TrgHeight = dwPicHeightDec;    
        prDownScalerPrm->u4TrgWidth = dwPicWidthDec; 
        prDownScalerPrm->u4TrgOffH = 0;    
        prDownScalerPrm->u4TrgOffV = 0;  
      #endif
      }
      //prDownScalerPrm->pucTrgYAddr = _pucVDSCLBuf[u4InstID];
      prDownScalerPrm->u4TrgYAddr = (UINT32)(_pucVDSCLBuf[u4InstID]);
      u4DramPicSize = 0x1FE000;//1920*1088//((((_tVerMpvDecPrm.u4PicW + 15) >> 4) * ((_tVerMpvDecPrm.u4PicH + 31) >> 5)) << 9);
      //prDownScalerPrm->u4TrgCAddr = *(UINT32*)(_pucVDSCLBuf[u4InstID] + u4DramPicSize);
      prDownScalerPrm->u4TrgCAddr = prDownScalerPrm->u4TrgYAddr + u4DramPicSize;
      //prDownScalerPrm->u4WorkAddr = *(UINT32*)_pucVDSCLWorkBuf[u4InstID];
      prDownScalerPrm->u4WorkAddr = (UINT32)(_pucVDSCLWorkBuf[u4InstID]);
      vFilledFBuf(u4InstID, _pucVDSCLBuf[u4InstID], _ptCurrFBufInfo[u4InstID]->u4DramPicSize);
    }
    //vVDecOutputDebugString("Vdo=%d PicHeight= %d TargHeight=%d DispW=%d\n",prDownScalerPrm->ucVdoFmt,
    //dwPicHeightDec, prDownScalerPrm->u4TrgHeight,prDownScalerPrm->u4DispW);
  }
  else
  {
    prDownScalerPrm->fgDSCLEn = FALSE;        
  }
  
#if 0
  if((_u4CodecVer == WMV)&&(_rWMVPPS.i4CurrentTemporalField==0)&&(prDownScalerPrm->fgDSCLEn==TRUE))
  {
    vFilledFBuf(_pucVDSCLBuf, _ptCurrFBufInfo->u4DramPicSize);
  }
  else if((_u4CodecVer != H264)&&(!_fgDec2ndFldPic)&&(prDownScalerPrm->fgDSCLEn==TRUE))
  {
    vFilledFBuf(_pucVDSCLBuf, _ptCurrFBufInfo->u4DramPicSize);
  }
#endif
#endif
}

void ComputeDQuantDecParam(UINT32 u4InstID)
{
    INT32 i4StepSize;
    INT32 i4DCStepSize ;
    VDEC_INFO_WMV_ETRY_PRM_T *prWMVEPS = &_rWMVEPS[u4InstID];
    VDEC_INFO_WMV_PIC_PRM_T *prWMVPPS = &_rWMVPPS[u4InstID];
   
    for (i4StepSize = 1; i4StepSize < 63; i4StepSize++) {
        VDEC_INFO_WMV_DQUANT_PRM_T *pDQ = &prWMVPPS->rDQuantParam3QPDeadzone[i4StepSize];

        INT32 i4DoubleStepSize = (i4StepSize + 1);

        pDQ->i4DoubleStepSize = i4DoubleStepSize;
        pDQ->i4StepMinusStepIsEven = 0;
        pDQ->i4DoublePlusStepSize = i4DoubleStepSize;
        pDQ->i4DoublePlusStepSizeNeg = -1 * pDQ->i4DoublePlusStepSize;

        i4DCStepSize = (i4StepSize + 1) >> 1;
        if (i4DCStepSize <= 4) {
            pDQ->i4DCStepSize = 8;
            if(prWMVEPS->fgNewDCQuant && i4DCStepSize <= 2) {
                pDQ->i4DCStepSize = 2 * i4DCStepSize;
            }
        } else {  
            pDQ->i4DCStepSize = i4DCStepSize / 2 + 6;
        }
    }
    
    for (i4StepSize = 1; i4StepSize < 63; i4StepSize++) {
        VDEC_INFO_WMV_DQUANT_PRM_T *pDQ = &prWMVPPS->rDQuantParam5QPDeadzone [i4StepSize];
        INT32 i4DoubleStepSize;

        i4DoubleStepSize = (i4StepSize + 1);

        pDQ->i4DoubleStepSize = i4DoubleStepSize;

        if (_i4CodecVersion[u4InstID] >= VDEC_WMV3) {
            pDQ->i4StepMinusStepIsEven = (i4StepSize + 1) >> 1;
           
            pDQ->i4DoublePlusStepSize = i4DoubleStepSize + pDQ->i4StepMinusStepIsEven;
        } else {
            INT32 iStepSize2 = (i4StepSize + 1) >> 1;
            pDQ->i4StepMinusStepIsEven = iStepSize2 - ((iStepSize2 & 1) == 0);
            pDQ->i4DoublePlusStepSize = i4DoubleStepSize + pDQ->i4StepMinusStepIsEven;      
        }
      	    
	    pDQ->i4DoublePlusStepSizeNeg = -1 * pDQ->i4DoublePlusStepSize;

        i4DCStepSize = (i4StepSize + 1) >> 1;
        if (i4DCStepSize <= 4) {
            pDQ->i4DCStepSize = 8;
            if(prWMVEPS->fgNewDCQuant && i4DCStepSize <= 2) 
                pDQ->i4DCStepSize = 2 * i4DCStepSize;

        } else {  
            pDQ->i4DCStepSize = i4DCStepSize / 2 + 6;
        }
    }
}

// *********************************************************************
// Function    : void vVParserProc(UINT32 u4InstID)
// Description : Video parser procedure
// Parameter   : None
// Return      : None
// *********************************************************************
void vVParserProc(UINT32 u4InstID)
{
  UINT32 u4VldByte,u4VldBit;
  char strMessage[512];

  if (_u4CodecVer[u4InstID] == VDEC_RM)
  {
    #ifdef RM_ATSPEED_TEST_ENABLE
    vRM_VParserEx(u4InstID);
    #else //RM_ATSPEED_TEST_ENABLE
    vRM_VParser(u4InstID);
    #endif //RM_ATSPEED_TEST_ENABLE
    _tVerDec[u4InstID].ucState = DEC_NORM_WAIT_TO_DEC;
  }
  else if(_u4CodecVer[u4InstID] == VDEC_H264)
  //if(_u4CodecVer[u4InstID] == VDEC_H264)
  {
#if VDEC_MVC_SUPPORT    
    while(_ucMVCType[u4InstID] && (_fgMVCReady[u4InstID] == FALSE))
    { 
        #if VDEC_8320_SUPPORT
        msleep(5);
        _fgMVCBaseGo = TRUE;
        #else
        x_thread_delay(5);  
        #endif
    }
#endif  
    vSearchRealPic(u4InstID);
    _tVerDec[u4InstID].ucState = DEC_NORM_WAIT_TO_DEC;
  }
  else if(_u4CodecVer[u4InstID] == VDEC_WMV)
  {

    VDEC_INFO_WMV_SEQ_PRM_T *prWMVSPS = &_rWMVSPS[u4InstID];
    VDEC_INFO_WMV_PIC_PRM_T *prWMVPPS = &_rWMVPPS[u4InstID];

    #if (WMV_8320_SUPPORT)
    #if WMV_LOG_TMP
    printk("vVParserProc, u4VLDWrapperWrok = 0x%x\n", _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecWMVDecPrm.rWmvWorkBufSa.u4VLDWrapperWrok);
    printk("vVParserProc, u4PPWrapperWrok = 0x%x\n", _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecWMVDecPrm.rWmvWorkBufSa.u4PPWrapperWrok);
    #endif
    
    vVDecWriteMC(u4InstID, RW_MC_VLD_WRAPPER, PHYSICAL(_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecWMVDecPrm.rWmvWorkBufSa.u4VLDWrapperWrok));
    vVDecWriteMC(u4InstID, RW_MC_PP_WRAPPER, PHYSICAL(_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecWMVDecPrm.rWmvWorkBufSa.u4PPWrapperWrok));
    #endif

    _u4CurrPicStartAddr[u4InstID] = u4VDEC_HAL_WMV_ReadRdPtr(_u4BSID[u4InstID], u4InstID, (UINT32)_pucVFifo[u4InstID], &u4VldBit);

    // _u4CurrPicStartAddr[u4InstID]
    #if WMV_LOG_TMP
    printk("vVParserProc, rd:0x%x\n", 
      _u4CurrPicStartAddr[u4InstID]);
    #endif
    
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
   #if WMV_EC_IMPROVE_SUPPORT
    //Search Slice Start Code
    if(_i4CodecVersion[u4InstID] == VDEC_VC1)
    {
        vWMVSearchSliceStartCode(u4InstID);
    }
    #endif
#endif

    if(fgVParserProcWMV(u4InstID))
    {
      if(prWMVSPS->fgXintra8)
      {
        _u4WMVDecPicNo[u4InstID]++; 
        _tVerDec[u4InstID].ucState = DEC_NORM_WAIT_DECODE;
        return;
      }
      UpdateVopheaderParam(u4InstID);
      if(prWMVPPS->ucPicType == SKIPFRAME)
      {
        VDEC_INFO_WMV_VFIFO_PRM_T rWmvVFifoInitPrm;
        VDEC_INFO_WMV_BS_INIT_PRM_T rWmvBSInitPrm;

        _u4SkipFrameCnt[u4InstID]++;
        _u4WMVDecPicNo[u4InstID]++; 
        _tVerDec[u4InstID].ucState = DEC_NORM_WAIT_DECODE;
        if(_i4CodecVersion[u4InstID] != VDEC_VC1)
        {
          rWmvVFifoInitPrm.u4CodeType = _i4CodecVersion[u4InstID];
          rWmvVFifoInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
          rWmvVFifoInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
          i4VDEC_HAL_WMV_InitVDecHW(u4InstID,&rWmvVFifoInitPrm);
          if(_iSetPos[u4InstID] >= V_FIFO_SZ)
          {
            _iSetPos[u4InstID] = _iSetPos[u4InstID] - V_FIFO_SZ;
          }
          rWmvBSInitPrm.u4VFifoSa =  (UINT32)_pucVFifo[u4InstID];
          rWmvBSInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
          rWmvBSInitPrm.u4ReadPointer =  (UINT32)_pucVFifo[u4InstID] + _iSetPos[u4InstID];
        #ifndef  RING_VFIFO_SUPPORT
          rWmvBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
        #else
 //         rWmvBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ*(0.5 + 0.5 *(_u4LoadBitstreamCnt[u4InstID]%2)));
          rWmvBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));
        #endif
          i4VDEC_HAL_WMV_InitBarrelShifter(_u4BSID[u4InstID], u4InstID, &rWmvBSInitPrm, FALSE);
        }
        else
        {
          vVDEC_HAL_WMV_AlignRdPtr(_u4BSID[u4InstID], u4InstID, (UINT32)_pucVFifo[u4InstID], BYTE_ALIGN);  //in order to use fgNextStartCode().
          u4VldByte = u4VDEC_HAL_WMV_ReadRdPtr(_u4BSID[u4InstID], u4InstID, (UINT32)_pucVFifo[u4InstID], &u4VldBit);
          rWmvVFifoInitPrm.u4CodeType = _i4CodecVersion[u4InstID];
          rWmvVFifoInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
          rWmvVFifoInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
          i4VDEC_HAL_WMV_InitVDecHW(u4InstID,&rWmvVFifoInitPrm);
          rWmvBSInitPrm.u4VFifoSa =  (UINT32)_pucVFifo[u4InstID];
          rWmvBSInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
          rWmvBSInitPrm.u4ReadPointer =  (UINT32)_pucVFifo[u4InstID] + u4VldByte;
        #ifndef  RING_VFIFO_SUPPORT
          rWmvBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
        #else
 //         rWmvBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ*(0.5 + 0.5 *(_u4LoadBitstreamCnt[u4InstID]%2)));
          rWmvBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));
        #endif
          i4VDEC_HAL_WMV_InitBarrelShifter(_u4BSID[u4InstID], u4InstID, &rWmvBSInitPrm, TRUE);
        }
      }
      else
      {
        u4VldByte = u4VDEC_HAL_WMV_ReadRdPtr(_u4BSID[u4InstID], u4InstID, (UINT32)_pucVFifo[u4InstID], &u4VldBit);
        //vVDecOutputDebugString("BYTE = %d and Bit = %d after parsing\n",u4VldByte,u4VldBit);
        _tVerDec[u4InstID].ucState = DEC_NORM_WAIT_TO_DEC;
      }
    }
    else
    {
      if(_u4VprErr[u4InstID] == END_OF_FILE)
      {
        _tVerDec[u4InstID].ucState = DEC_NORM_WAIT_DECODE;
      }
      else
      {
        strcpy(_tFileListRecInfo[u4InstID].bFileName,_FileList_Rec[u4InstID]);
        sprintf(strMessage,"Parsing header error : 0x%.8x\n",_u4VprErr[u4InstID]);
        fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
      }
    }

#if VDEC_DDR3_SUPPORT    
  /* if (_rWMVSPS[u4InstID].u4PicWidthSrc > 720|| _rWMVSPS[u4InstID].u4PicHeightSrc> 576)
   {
        printk("DDR3 Not Support Size over HD\n");
        strcpy(_tFileListRecInfo[u4InstID].bFileName,_FileList_Rec[u4InstID]);
        sprintf(strMessage,"DDR3 Size Over HD : Not Support in FPGA\n");
        fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);

        _u4VerBitCount[u4InstID] = 0xFFFFFFFF;
   }*/
#endif

    
  }
  else if(_u4CodecVer[u4InstID] == VDEC_MPEG2)
  {
    vDEC_HAL_COMMON_SetVLDPower(u4InstID,ON);
    _u4CurrPicStartAddr[u4InstID] = u4VDEC_HAL_MPEG_ReadRdPtr(_u4BSID[u4InstID], u4InstID, (UINT32)_pucVFifo[u4InstID], &u4VldBit);
    printk("<vdec> _u4CurrPicStartAddr[%u]=0x%x (%u)\n", u4InstID, _u4CurrPicStartAddr[u4InstID], _u4CurrPicStartAddr[u4InstID]);
    u4VParserMPEG12(u4InstID, FALSE);
    vDEC_HAL_COMMON_SetVLDPower(u4InstID,OFF);
    switch(_u4PicCdTp[u4InstID])
    {
      case I_TYPE:
      case P_TYPE:
        vVPrsMPEGIPProc(u4InstID);
        break;
      case B_TYPE:
        vVPrsMPEGBProc(u4InstID);
        break;
      default:
        break;
    } 
    _tVerDec[u4InstID].ucState = DEC_NORM_WAIT_TO_DEC;
  }
  else if((_u4CodecVer[u4InstID] == VDEC_DIVX3) || (_u4CodecVer[u4InstID] == VDEC_MPEG4) || (_u4CodecVer[u4InstID] == VDEC_H263))
  {
    vDEC_HAL_COMMON_SetVLDPower(u4InstID,ON);
    _u4CurrPicStartAddr[u4InstID] = u4VDEC_HAL_MPEG_ReadRdPtr(_u4BSID[u4InstID], u4InstID, (UINT32)_pucVFifo[u4InstID], &u4VldBit);
    u4VParserMPEG4(u4InstID, FALSE);
    vDEC_HAL_COMMON_SetVLDPower(u4InstID,OFF);
    if(_fgVerVopCoded0[u4InstID])
    {
    #ifndef	VPMODE
      _tVerDec[u4InstID].ucState = DEC_NORM_WAIT_DECODE;
    #else
      _tVerDec[u4InstID].ucState = DEC_NORM_WAIT_TO_DEC;//qiguo 8/6
    #endif
    }
    else
    {    
      switch(_u4PicCdTp[u4InstID])
      {
        case I_TYPE:
        case P_TYPE:
          vVPrsMPEGIPProc(u4InstID);
          break;
        case B_TYPE:
          vVPrsMPEGBProc(u4InstID);
          break;
        default:
          break;
      } 
      _tVerDec[u4InstID].ucState = DEC_NORM_WAIT_TO_DEC;
    }
    _tVerPic[u4InstID].u4W = _u4HSize[u4InstID];
    _tVerPic[u4InstID].u4H = _u4VSize[u4InstID];
    _tVerPic[u4InstID].ucMpegVer = _ucMpegVer[u4InstID];
	u4FilePicCont_noVOP++;
  }
  else if(_u4CodecVer[u4InstID] == VDEC_VP6)
  {
      u4VerVParserVP6(u4InstID, FALSE);
      _tVerDec[u4InstID].ucState = DEC_NORM_WAIT_TO_DEC;
  }  
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
  else if(_u4CodecVer[u4InstID] == VDEC_VP8)
  {
      u4VerVParserVP8(u4InstID, FALSE);
      _tVerDec[u4InstID].ucState = DEC_NORM_WAIT_TO_DEC;
  }  
#endif  
  else if(_u4CodecVer[u4InstID] == VDEC_AVS)
  {
      u4VerVParserAVS(u4InstID, FALSE);
      _tVerDec[u4InstID].ucState = DEC_NORM_WAIT_TO_DEC;
  }  
  #ifdef VERIFY_DV_SUPPORT
  else if(_u4CodecVer[u4InstID] == DV)
  {
    _tVerDec[u4InstID].ucState = DEC_NORM_WAIT_TO_DEC;
  }
  #endif
}


void vVPrsMPEGIPProc(UINT32 u4InstID)
{
  if(_u4PicStruct[u4InstID] == FRM_PIC)
  {
    if(_u4BrokenLink[u4InstID] > 0)
    {
      _u4BrokenLink[u4InstID] --;
    }
    _fgDec2ndFldPic[u4InstID] = 0;
    vSetVerFRefBuf(u4InstID, _u4BRefBufIdx[u4InstID]);
    vSetVerBRefBuf(u4InstID, 1 - _u4FRefBufIdx[u4InstID]);
    vSetVerDecBuf(u4InstID, _u4BRefBufIdx[u4InstID]);

  }
  else  // Field Picture
  {
    if(_fgVerPrevBPic[u4InstID])
    {
      _fgDec2ndFldPic[u4InstID] = 0;
    }
    // 1st Field Picture
    if(!_fgDec2ndFldPic[u4InstID])
    {
      if(_u4BrokenLink[u4InstID] > 0)
      {
        _u4BrokenLink[u4InstID] --;
      }
      // Set Reference Buffer
      vSetVerFRefBuf(u4InstID, _u4BRefBufIdx[u4InstID]);
      vSetVerBRefBuf(u4InstID, 1 - _u4FRefBufIdx[u4InstID]);
      vSetVerDecBuf(u4InstID, _u4BRefBufIdx[u4InstID]);
      _u4PicStruct[u4InstID] = (_u4PicStruct[u4InstID] == TOP_FLD_PIC) ? TWO_FLDPIC_TOPFIRST : TWO_FLDPIC_BTMFIRST;
    }
    // 2nd Field Picture
    else
    {
      vSetVerDecBuf(u4InstID, _u4BRefBufIdx[u4InstID]);
    }
  }
}

void vVPrsMPEGBProc(UINT32 u4InstID)
{
  if(_u4PicStruct[u4InstID] == FRM_PIC || !_fgDec2ndFldPic[u4InstID])
  {
    // Set Decoding Buffer
    vSetVerDecBuf(u4InstID, 2);
  }
  else
  {
    // Field picture and 2ND_FLD_PIC
    vSetVerDecBuf(u4InstID, 2);
  }

  if(_u4PicStruct[u4InstID] == FRM_PIC)
  {
    _fgDec2ndFldPic[u4InstID] = FALSE;
  }
  else
  {
    if(!_fgVerPrevBPic[u4InstID])
    {
      _fgDec2ndFldPic[u4InstID] = FALSE;
    }
  }
}


// *********************************************************************
// Function    : void vVDecProc(UINT32 u4InstID)
// Description : Set VDec related parameters
// Parameter   : None
// Return      : None
// *********************************************************************
BOOL _fgReInitBS=TRUE;
void vVDecProc(UINT32 u4InstID)
{
  _fgVDecComplete[u4InstID] = FALSE;

#if VDEC_DRAM_BUSY_TEST
     vDrmaBusySet (u4InstID);
#endif

#if VDEC_DDR3_SUPPORT    
    _tVerMpvDecPrm[u4InstID].ucAddrSwapMode = ADDRSWAP_DDR3;
#else
    _tVerMpvDecPrm[u4InstID].ucAddrSwapMode = _u2AddressSwapMode[u4InstID];
#endif

  if (_u4CodecVer[u4InstID] == VDEC_RM)
  {
    vRM_TriggerDecode(u4InstID, &_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo);
    _tVerDec[u4InstID].ucState = DEC_NORM_WAIT_DECODE;
  }
  else if(_u4CodecVer[u4InstID] == VDEC_H264)
  //if(_u4CodecVer[u4InstID] == VDEC_H264)
  {
  #ifdef REDEC
    _u4VLDPosByte[u4InstID] = u4VDEC_HAL_H264_ReadRdPtr(0, u4InstID, (UINT32)_pucVFifo[u4InstID], &_u4VLDPosBit[u4InstID]);
    if( _u4ReDecCnt[u4InstID] > 0)
    {
     vVerifyVDecSetPicInfo(u4InstID, &_tVerMpvDecPrm[u4InstID]);
    }
  #endif
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecH264DecPrm.ucNalRefIdc = _u4NalRefIdc[u4InstID];

    if(_ucMVCType[u4InstID] == 2) 
    {
       //Dep View
       _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecH264DecPrm.fgIsAllegMvcCfg = (_u4NalRefIdc[0] > 0)? 1: 0;
    }
    else
    {
       _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecH264DecPrm.fgIsAllegMvcCfg = 0;
    }
    
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecH264DecPrm.fgIsFrmPic = fgIsFrmPic(u4InstID);
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecH264DecPrm.fgIsIDRPic = fgIsIDRPic(u4InstID);
    //_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecH264DecPrm.u4DecWorkBuf = (UINT32)_pucDecWorkBuf[u4InstID];     
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecH264DecPrm.prCurrFBufInfo = _ptCurrFBufInfo[u4InstID];
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecH264DecPrm.prCurrFBufInfo->u4YStartAddr = /*PHYSICAL*/((UINT32)_pucDecWorkBuf[u4InstID]); 
  #ifdef VERIFICATION_DOWN_SCALE
    #ifdef DOWN_SCALE_SUPPORT
        //vSetDownScaleParam(u4InstID, TRUE, &_tDownScalerPrm[u4InstID]);
        vSetDownScaleParam(u4InstID, TRUE, &(_tVerMpvDecPrm[u4InstID].rDownScalerPrm));
    #else
        //vSetDownScaleParam(u4InstID, FALSE, &_tDownScalerPrm[u4InstID]);
        vSetDownScaleParam(u4InstID, FALSE, &(_tVerMpvDecPrm[u4InstID].rDownScalerPrm));
    #endif
    //vDEC_HAL_COMMON_SetDownScaler(u4InstID, &_tDownScalerPrm[u4InstID]);
        //vVDECSetDownScalerPrm(u4InstID, &_tDownScalerPrm[u4InstID]);
        vVDECSetDownScalerPrm(u4InstID, &(_tVerMpvDecPrm[u4InstID].rDownScalerPrm));
  #endif
  #ifdef FGT_SUPPORT
    vSetFGTParam(&_rFGTPrm[u4InstID]);
    i4VDEC_HAL_H264_FGTSetting(u4InstID, &_rFGTPrm[u4InstID]);
  #endif
  
    if(_ucMVCType[u4InstID] == 2) 
  	 _fgVDecComplete[0] = FALSE;
    
  #ifdef LETTERBOX_SUPPORT
    vLBDParaParsing(u4InstID);
    vVDECSetLetetrBoxDetPrm(u4InstID, &_rLBDPrm[u4InstID]);
  #endif
//    BSP_InvDCacheRange((UINT32)_pucDPB[u4InstID],DPB_SZ);
    DBG_H264_PRINTF("[Info] >>> Trigger Decode BEGIN \n"); 
    i4VDEC_HAL_H264_DecStart(u4InstID, &_tVerMpvDecPrm[u4InstID]);
    DBG_H264_PRINTF("[Info] >>> Trigger Decode END \n"); 
    _tVerDec[u4InstID].ucState = DEC_NORM_WAIT_DECODE;
  }
  else if(_u4CodecVer[u4InstID] == VDEC_WMV)
  {
  #ifdef REDEC
    _u4VLDPosByte[u4InstID] = u4VDEC_HAL_WMV_ReadRdPtr(_u4BSID[u4InstID], u4InstID, (UINT32)_pucVFifo[u4InstID], &_u4VLDPosBit[u4InstID]);
  #endif
    if(fgVDecProcWMV(u4InstID))
    {
      _tVerDec[u4InstID].ucState = DEC_NORM_WAIT_DECODE;
    }
  }
  else if(_u4CodecVer[u4InstID] == VDEC_VP6)
  {
    vVDecEnableCRC(u4InstID, 1, 1);
    vVerifyVDecSetVP6Info(u4InstID);
    _tVerDec[u4InstID].ucState = DEC_NORM_WAIT_DECODE;
  }
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
  else if(_u4CodecVer[u4InstID] == VDEC_VP8)
  {
    vVerifyVDecSetVP8Info(u4InstID);
    _tVerDec[u4InstID].ucState = DEC_NORM_WAIT_DECODE;
  }
#endif  
  else if(_u4CodecVer[u4InstID] == VDEC_AVS)
  {
    vVerifyVDecSetAVSInfo(u4InstID);
    _tVerDec[u4InstID].ucState = DEC_NORM_WAIT_DECODE;
  }

  else 
  {
    vVDecEnableCRC(u4InstID, 1, (VDEC_PP_ENABLE == FALSE)); // MPEG2 crc from MC
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.fgDec2ndFld = _fgDec2ndFldPic[u4InstID];
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rMpegFrameBufSa.u4Pic0CSa = (UINT32)_pucPic0C[u4InstID];
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rMpegFrameBufSa.u4Pic0YSa = (UINT32)_pucPic0Y[u4InstID];
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rMpegFrameBufSa.u4Pic1CSa = (UINT32)_pucPic1C[u4InstID];
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rMpegFrameBufSa.u4Pic1YSa = (UINT32)_pucPic1Y[u4InstID];
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rMpegFrameBufSa.u4Pic2CSa = (UINT32)_pucPic2C[u4InstID];
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rMpegFrameBufSa.u4Pic2YSa = (UINT32)_pucPic2Y[u4InstID];
        _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rMpegPpInfo.fgPpEnable = VDEC_PP_ENABLE;
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rMpegPpInfo.u4PpYBufSa = (UINT32)_pucPpYSa[u4InstID];
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rMpegPpInfo.u4PpCBufSa = (UINT32)_pucPpCSa[u4InstID];
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.u4DecH = ((_tVerMpvDecPrm[u4InstID].u4PicH + 15) >> 4 ) << 4;
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.u4DecW = ((_tVerMpvDecPrm[u4InstID].u4PicW + 15) >> 4 ) << 4;
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.u4DecXOff = 0;
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.u4DecYOff = 0;
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.u4FRefBufIdx = _u4FRefBufIdx[u4InstID];
    _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.u4MaxMbl = ((_tVerMpvDecPrm[u4InstID].u4PicH + 15) >> 4 ) - 1;
    _tVerMpvDecPrm[u4InstID].ucDecFBufIdx = BYTE0(_u4DecBufIdx[u4InstID]);
    _tVerMpvDecPrm[u4InstID].ucPicStruct = BYTE0(_u4PicStruct[u4InstID]);
    _tVerMpvDecPrm[u4InstID].ucPicType = BYTE0(_u4PicCdTp[u4InstID]);

    if(_u4CodecVer[u4InstID] == VDEC_MPEG2)
    {
    #ifdef REDEC
      _u4VLDPosByte[u4InstID] = u4VDEC_HAL_MPEG_ReadRdPtr(_u4BSID[u4InstID], u4InstID, (UINT32)_pucVFifo[u4InstID], &_u4VLDPosBit[u4InstID]);
    #endif
    #ifdef VERIFICATION_DOWN_SCALE
      #ifdef DOWN_SCALE_SUPPORT
            //vSetDownScaleParam(u4InstID, TRUE, &_tDownScalerPrm[u4InstID]);
            vSetDownScaleParam(u4InstID, TRUE, &(_tVerMpvDecPrm[u4InstID].rDownScalerPrm));
      #else
            //vSetDownScaleParam(u4InstID, FALSE, &_tDownScalerPrm[u4InstID]);
            vSetDownScaleParam(u4InstID, FALSE, &(_tVerMpvDecPrm[u4InstID].rDownScalerPrm));
      #endif
      //vDEC_HAL_COMMON_SetDownScaler(u4InstID, &_tDownScalerPrm[u4InstID]);
            //vVDECSetDownScalerPrm(u4InstID, &_tDownScalerPrm[u4InstID]);
            //vVDECSetDownScalerPrm(u4InstID, &(_tVerMpvDecPrm[u4InstID].rDownScalerPrm));
    #endif
      if(_u4BSID[u4InstID] == 1)
      {
        UINT32 u4Bytes,u4Bits;
        VDEC_INFO_MPEG_BS_INIT_PRM_T rMpegBSInitPrm;

        vDEC_HAL_COMMON_SetVLDPower(u4InstID,ON);
        u4Bytes = u4VDEC_HAL_MPEG_ReadRdPtr(1, u4InstID, (UINT32)_pucVFifo[u4InstID], &u4Bits);
        rMpegBSInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
        rMpegBSInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
        rMpegBSInitPrm.u4ReadPointer= (UINT32)_pucVFifo[u4InstID] + u4Bytes;
      #ifndef  RING_VFIFO_SUPPORT
        rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
      #else
//        rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ*(0.5 + 0.5 *(_u4LoadBitstreamCnt[u4InstID]%2)));
	rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));//mtk40343
      #endif
        i4VDEC_HAL_MPEG_InitBarrelShifter(0, u4InstID, &rMpegBSInitPrm);
        u4VDEC_HAL_MPEG_ShiftGetBitStream(0, u4InstID, u4Bits);
      }
	  
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
	  vVDEC_HAL_MPEG2_DisableMVOverflowDetection(u4InstID);
	  printk("<vdec> disable mv overflow\n");
#endif
	  	
      _tVerMpvDecPrm[u4InstID].ucMpegSpecType = 1;

      i4VDEC_HAL_MPEG12_DecStart(u4InstID, &_tVerMpvDecPrm[u4InstID]);
      printk("@(posedge `VDEC_INTERRUPT);\n");
      _tVerDec[u4InstID].ucState = DEC_NORM_WAIT_DECODE;
    }
    else if(_u4CodecVer[u4InstID] == VDEC_DIVX3)
    {
    #ifdef REDEC
      _u4VLDPosByte[u4InstID] = u4VDEC_HAL_MPEG_ReadRdPtr(_u4BSID[u4InstID], u4InstID, (UINT32)_pucVFifo[u4InstID], &_u4VLDPosBit[u4InstID]);
    #endif
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rMpeg4WorkBufSa.u4BcodeSa = (UINT32)_pucMp4Bcode[u4InstID];
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rMpeg4WorkBufSa.u4Bmb1Sa = (UINT32)_pucMp4Bmb1[u4InstID];
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rMpeg4WorkBufSa.u4Bmb2Sa = (UINT32)_pucMp4Bmb2[u4InstID];
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rMpeg4WorkBufSa.u4DcacSa = (UINT32)_pucMp4Dcac[u4InstID];
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rMpeg4WorkBufSa.u4MvecSa = (UINT32)_pucMp4Mvec[u4InstID];
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rMpeg4WorkBufSa.u4VldWrapperSa = (UINT32)_pucVLDWrapperWrok[u4InstID];
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rMpeg4WorkBufSa.u4PPWrapperSa = (UINT32)_pucPPWrapperWork[u4InstID];
      //6589NEW 2.4, 2.5, 4.1
     #if (MPEG4_6589_SUPPORT)
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rMpeg4WorkBufSa.u4DataPartitionSa= (UINT32)_pucMp4DataPartition[u4InstID];
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rMpeg4WorkBufSa.u4NotCodedSa = (UINT32)_pucMp4NotCoded[u4InstID];
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rMpeg4WorkBufSa.u4MvDirectSa = (UINT32)_pucMp4MvDirect[u4InstID];
     #endif
     #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rMpeg4WorkBufSize.u4BcodeSize = BCODE_SZ;//count in 16 byte
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rMpeg4WorkBufSize.u4DcacSize = DCAC_SZ;
	  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rMpeg4WorkBufSize.u4MVSize= VER_MVEC_SZ;
	  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rMpeg4WorkBufSize.u4MB1Size = VER_BMB1_SZ;
	  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rMpeg4WorkBufSize.u4MB2Size = VER_BMB2_SZ;
     //6589NEW 2.4, 2.5, 4.1(MV Direct size not required)
     #if (MPEG4_6589_SUPPORT)
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rMpeg4WorkBufSize.u4DataPartitionSize = DATA_PARTITION_SZ;
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rMpeg4WorkBufSize.u4NotCodedSize = NOT_CODED_SZ;            
     #endif
     #endif
     #if (MPEG4_6589_SUPPORT)
      vSetDx3SliceBoundary(u4InstID, &(_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm));
     #endif
     
      _tVerMpvDecPrm[u4InstID].ucMpegSpecType = 3;//divx  mode	  //_tVerMpvDecPrm[u4InstID].ucMpegSpecType = 3;//divx mode
    #ifdef VERIFICATION_DOWN_SCALE
      #ifdef DOWN_SCALE_SUPPORT
            //vSetDownScaleParam(u4InstID, TRUE, &_tDownScalerPrm[u4InstID]);
            vSetDownScaleParam(u4InstID, TRUE, &(_tVerMpvDecPrm[u4InstID].rDownScalerPrm));
      #else
            //vSetDownScaleParam(u4InstID, FALSE, &_tDownScalerPrm[u4InstID]);
            vSetDownScaleParam(u4InstID, FALSE, &(_tVerMpvDecPrm[u4InstID].rDownScalerPrm));
      #endif
      //vDEC_HAL_COMMON_SetDownScaler(u4InstID, &_tDownScalerPrm[u4InstID]);
            //vVDECSetDownScalerPrm(u4InstID, &_tDownScalerPrm[u4InstID]);
            vVDECSetDownScalerPrm(u4InstID, &(_tVerMpvDecPrm[u4InstID].rDownScalerPrm));
    #endif
      if(_u4BSID[u4InstID] == 1)
      {
        UINT32 u4Bytes,u4Bits;
        VDEC_INFO_MPEG_BS_INIT_PRM_T rMpegBSInitPrm;

        vDEC_HAL_COMMON_SetVLDPower(u4InstID,ON);
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
      i4VDEC_HAL_DIVX3_DecStart(u4InstID, &_tVerMpvDecPrm[u4InstID]);
      _tVerDec[u4InstID].ucState = DEC_NORM_WAIT_DECODE;
    }
        else if(_u4CodecVer[u4InstID] == VDEC_MPEG4 || _u4CodecVer[u4InstID] == VDEC_H263)
    {
    #ifdef REDEC
      _u4VLDPosByte[u4InstID] = u4VDEC_HAL_MPEG_ReadRdPtr(_u4BSID[u4InstID], u4InstID, (UINT32)_pucVFifo[u4InstID], &_u4VLDPosBit[u4InstID]);
    #endif
            //PANDA H263 Deblocking test
            _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rMpegPpInfo.fgPpEnable = VDEC_PP_ENABLE;
            _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rMpegPpInfo.u4PpYBufSa = (UINT32)_pucPpYSa[u4InstID];
            _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rMpegPpInfo.u4PpCBufSa = (UINT32)_pucPpCSa[u4InstID];
            //~PANDA
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rDep.rM4vDecPrm.prVol = &_rMPEG4VolPrm[u4InstID];
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rDep.rM4vDecPrm.prVop = &_rMPEG4VopPrm[u4InstID];
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rDep.rM4vDecPrm.prVop->prDirMd = &_rDirMode[u4InstID];
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rDep.rM4vDecPrm.prVop->prGmcPrm = &_rMPEG4GmcPrm[u4InstID];
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rMpeg4WorkBufSa.u4BcodeSa = (UINT32)_pucMp4Bcode[u4InstID];
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rMpeg4WorkBufSa.u4Bmb1Sa = (UINT32)_pucMp4Bmb1[u4InstID];
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rMpeg4WorkBufSa.u4Bmb2Sa = (UINT32)_pucMp4Bmb2[u4InstID];
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rMpeg4WorkBufSa.u4DcacSa = (UINT32)_pucMp4Dcac[u4InstID];
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rMpeg4WorkBufSa.u4MvecSa = (UINT32)_pucMp4Mvec[u4InstID];
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rMpeg4WorkBufSa.u4VldWrapperSa = (UINT32)_pucVLDWrapperWrok[u4InstID];
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rMpeg4WorkBufSa.u4PPWrapperSa = (UINT32)_pucPPWrapperWork[u4InstID];
       //6589NEW 2.4, 2.5, 4.1
      #if (MPEG4_6589_SUPPORT)
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rMpeg4WorkBufSa.u4DataPartitionSa= (UINT32)_pucMp4DataPartition[u4InstID];
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rMpeg4WorkBufSa.u4NotCodedSa = (UINT32)_pucMp4NotCoded[u4InstID];
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rMpeg4WorkBufSa.u4MvDirectSa = (UINT32)_pucMp4MvDirect[u4InstID];      
      #endif

	  //_tVerMpvDecPrm[u4InstID].ucMpegSpecType = 3;//divx mode	  //_tVerMpvDecPrm[u4InstID].ucMpegSpecType = 3;//divx mode
	  _tVerMpvDecPrm[u4InstID].ucMpegSpecType = 2;//mpeg4 mode	  //_tVerMpvDecPrm[u4InstID].ucMpegSpecType = 3;//divx mode
      #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rMpeg4WorkBufSize.u4BcodeSize = BCODE_SZ;//count in 16 byte
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rMpeg4WorkBufSize.u4DcacSize = DCAC_SZ;
	  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rMpeg4WorkBufSize.u4MVSize= VER_MVEC_SZ;
	  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rMpeg4WorkBufSize.u4MB1Size = VER_BMB1_SZ;
	  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rMpeg4WorkBufSize.u4MB2Size = VER_BMB2_SZ;
      //6589NEW 2.4, 2.5, 4.1(MV Direct size not required)
      #if (MPEG4_6589_SUPPORT)
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rMpeg4WorkBufSize.u4DataPartitionSize = DATA_PARTITION_SZ;
      _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rMpeg4WorkBufSize.u4NotCodedSize = NOT_CODED_SZ;            
      #endif      
      #endif
	  
    #ifdef VERIFICATION_DOWN_SCALE
      #ifdef DOWN_SCALE_SUPPORT
            //vSetDownScaleParam(u4InstID, TRUE, &_tDownScalerPrm[u4InstID]);
            vSetDownScaleParam(u4InstID, TRUE, &(_tVerMpvDecPrm[u4InstID].rDownScalerPrm));
      #else
            //vSetDownScaleParam(u4InstID, FALSE, &_tDownScalerPrm[u4InstID]);
            vSetDownScaleParam(u4InstID, FALSE, &(_tVerMpvDecPrm[u4InstID].rDownScalerPrm));
      #endif
      //vDEC_HAL_COMMON_SetDownScaler(u4InstID, &_tDownScalerPrm[u4InstID]);
            //vVDECSetDownScalerPrm(u4InstID, &_tDownScalerPrm[u4InstID]);
            vVDECSetDownScalerPrm(u4InstID, &(_tVerMpvDecPrm[u4InstID].rDownScalerPrm));
    #endif
      if(_u4BSID[u4InstID] == 1)
      {
        UINT32 u4Bytes,u4Bits;
        VDEC_INFO_MPEG_BS_INIT_PRM_T rMpegBSInitPrm;

        vDEC_HAL_COMMON_SetVLDPower(u4InstID,ON);
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
      if(0)//(_fgReInitBS)
      {
        UINT32 u4Bytes,u4Bits;
        VDEC_INFO_MPEG_BS_INIT_PRM_T rMpegBSInitPrm;

        vDEC_HAL_COMMON_SetVLDPower(u4InstID,ON);
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
        i4VDEC_HAL_MPEG_InitBarrelShifter(0, u4InstID, &rMpegBSInitPrm);
        u4VDEC_HAL_MPEG_ShiftGetBitStream(0, u4InstID, u4Bits);
      }
      vVDecEnableCRC(u4InstID,1,1);
#ifdef VPMODE
      if(_fgVerVopCoded0[u4InstID])//qiguo
      {
	vVDEC_HAL_MPEG_SetMPEG4Flag(u4InstID, FALSE);
	_u4FileCnt[u4InstID] = _u4FileCnt[u4InstID]  -1;
      	i4VPModeDecStart(u4InstID, &_tVerMpvDecPrm[u4InstID]);
      }
      else
#endif
      {
      i4VDEC_HAL_MPEG4_DecStart(u4InstID, &_tVerMpvDecPrm[u4InstID]);
      }
	  
      _tVerDec[u4InstID].ucState = DEC_NORM_WAIT_DECODE;
    }
  }
}

void PostAdjustReconRange(UINT32 u4InstID)
{
    VDEC_INFO_WMV_SEQ_PRM_T *prWMVSPS = &_rWMVSPS[u4InstID];
    VDEC_INFO_WMV_ETRY_PRM_T *prWMVEPS = &_rWMVEPS[u4InstID];
    VDEC_INFO_WMV_PIC_PRM_T *prWMVPPS = &_rWMVPPS[u4InstID];
    
    if (prWMVSPS->fgPreProcRange) {
        if (prWMVPPS->ucPicType == IVOP || prWMVPPS->ucPicType == BIVOP) {
            if (prWMVSPS->i4NumBFrames == 0) {
                prWMVEPS->i4ReconRangeState = prWMVEPS->i4RangeState;
            }
            else if(prWMVPPS->ucPicType != BVOP) {
                AdjustReconRange(u4InstID);
            }
        }
    }
}

void vVerifySetVSyncPrmBufPtr(UINT32 u4InstID, UINT32 u4BufIdx)
{
  switch(u4BufIdx)
  {
    case 0:
      _pucDecWorkBuf[u4InstID] = (UCHAR *) _pucPic0Y[_u1AlphaDecPrmIdx[u4InstID]];
      _pucDecCWorkBuf[u4InstID] = (UCHAR *) _pucPic0C[_u1AlphaDecPrmIdx[u4InstID]];
      break;
    case 1:
      _pucDecWorkBuf[u4InstID] = (UCHAR *) _pucPic1Y[_u1AlphaDecPrmIdx[u4InstID]];
      _pucDecCWorkBuf[u4InstID] = (UCHAR *) _pucPic1C[_u1AlphaDecPrmIdx[u4InstID]];
      break;
    case 2:
      _pucDecWorkBuf[u4InstID] = (UCHAR *) _pucPic2Y[_u1AlphaDecPrmIdx[u4InstID]];
      _pucDecCWorkBuf[u4InstID] = (UCHAR *) _pucPic2C[_u1AlphaDecPrmIdx[u4InstID]];
      break;
  }
}

void vWMVVDecEnd(UINT32 u4InstID)
{
  //VDEC_INFO_DEC_PRM_T *tVerMpvDecPrm;
  //VDEC_INFO_H264_FBUF_INFO_T *tFBufInfo;
  VDEC_INFO_WMV_VFIFO_PRM_T rWmvVFifoInitPrm;
  VDEC_INFO_WMV_BS_INIT_PRM_T rWmvBSInitPrm;
  UINT32 u4VldByte,u4VldBit;
  VDEC_INFO_WMV_PIC_PRM_T *prWMVPPS = &_rWMVPPS[u4InstID];

  //tFBufInfo = _ptCurrFBufInfo[u4InstID];
  //tVerMpvDecPrm = &_tVerMpvDecPrm[u4InstID];

  u4VldByte = u4VDEC_HAL_WMV_ReadRdPtr(0, u4InstID, (UINT32)_pucVFifo[u4InstID], &u4VldBit);
  _u4WMVByteCount[u4InstID] = u4VldByte;
   ///u4VldByte
  #if WMV_LOG_TMP
  printk("vWMVVDecEnd, rd:0x%x\n", u4VldByte);
  #endif
    
#ifdef LETTERBOX_DETECTION_ONLY  
  vCheckLBDResult(u4InstID);
#else  
  vWMVWrData2PC(u4InstID, _pucDumpYBuf[u4InstID], ((((_tVerPic[u4InstID].u4W + 15) >> 4) * ((_tVerPic[u4InstID].u4H + 31) >> 5)) << 9));
#endif
  
  // reset HW
#ifdef REDEC   
  if(_u4ReDecCnt[u4InstID] > 0)
  {
    _u4WMVDecPicNo[u4InstID]--;
    rWmvVFifoInitPrm.u4CodeType = _i4CodecVersion[u4InstID];
    rWmvVFifoInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
    rWmvVFifoInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
    i4VDEC_HAL_WMV_InitVDecHW(u4InstID,&rWmvVFifoInitPrm);
    rWmvBSInitPrm.u4VFifoSa =  (UINT32)_pucVFifo[u4InstID];
    rWmvBSInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
    rWmvBSInitPrm.u4ReadPointer =  (UINT32)_pucVFifo[u4InstID] + _u4VLDPosByte[u4InstID];
  #ifndef  RING_VFIFO_SUPPORT
    rWmvBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
  #else
//    rWmvBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ*(0.5 + 0.5 *(_u4LoadBitstreamCnt[u4InstID]%2)));
    rWmvBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));
  #endif
    if (_i4CodecVersion[u4InstID] == VDEC_VC1)
    {
        i4VDEC_HAL_WMV_InitBarrelShifter(0, u4InstID, &rWmvBSInitPrm, TRUE);
    }
    else
    {
       i4VDEC_HAL_WMV_InitBarrelShifter(0, u4InstID, &rWmvBSInitPrm, FALSE);
    }
    u4VDEC_HAL_WMV_ShiftGetBitStream(0, u4InstID, _u4VLDPosBit[u4InstID]);
    _tVerDec[u4InstID].ucState = DEC_NORM_WAIT_TO_DEC;
    return;
  }
#endif

  //ming modify@2006/4/12
  if(prWMVPPS->ucFrameCodingMode == INTERLACEFIELD)
  {
    prWMVPPS->i4CurrentTemporalField ^= 1; //toggle field
     prWMVPPS->i4CurrentField ^= 1;
  }
  
  if(_i4CodecVersion[u4InstID] != VDEC_VC1)
  {
    rWmvVFifoInitPrm.u4CodeType = _i4CodecVersion[u4InstID];
    rWmvVFifoInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
    rWmvVFifoInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
    i4VDEC_HAL_WMV_InitVDecHW(u4InstID,&rWmvVFifoInitPrm);
    if(_iSetPos[u4InstID] >= V_FIFO_SZ)
    {
      _iSetPos[u4InstID] = _iSetPos[u4InstID] - V_FIFO_SZ;
    }
    rWmvBSInitPrm.u4VFifoSa =  (UINT32)_pucVFifo[u4InstID];
    rWmvBSInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
    rWmvBSInitPrm.u4ReadPointer =  (UINT32)_pucVFifo[u4InstID] + _iSetPos[u4InstID];
  #ifndef  RING_VFIFO_SUPPORT
    rWmvBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
  #else
//    rWmvBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ*(0.5 + 0.5 *(_u4LoadBitstreamCnt[u4InstID]%2)));
    rWmvBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));
  #endif
    i4VDEC_HAL_WMV_InitBarrelShifter(_u4BSID[u4InstID], u4InstID, &rWmvBSInitPrm, FALSE);
  }
  else // WMVA
  {
    rWmvVFifoInitPrm.u4CodeType = _i4CodecVersion[u4InstID];
    rWmvVFifoInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
    rWmvVFifoInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
    i4VDEC_HAL_WMV_InitVDecHW(u4InstID,&rWmvVFifoInitPrm);
    rWmvBSInitPrm.u4VFifoSa =  (UINT32)_pucVFifo[u4InstID];
    rWmvBSInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
    rWmvBSInitPrm.u4ReadPointer =  (UINT32)_pucVFifo[u4InstID] + u4VldByte;
  #ifndef  RING_VFIFO_SUPPORT
    rWmvBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
  #else
//    rWmvBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ*(0.5 + 0.5 *(_u4LoadBitstreamCnt[u4InstID]%2)));
    rWmvBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));
  #endif
    i4VDEC_HAL_WMV_InitBarrelShifter(_u4BSID[u4InstID], u4InstID, &rWmvBSInitPrm, TRUE);
  }  
  if(_rWMVPPS[u4InstID].ucPicType != SKIPFRAME)
  {
    //update _iReconRangeState
    PostAdjustReconRange(u4InstID);
  }
#ifndef INTERGRATION_WITH_DEMUX
#ifdef  RING_VFIFO_SUPPORT
  if((_u4LoadBitstreamCnt[u4InstID]&0x1) && (rWmvBSInitPrm.u4ReadPointer > 
  ((UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ/2))))
  {
    _tInFileInfo[u4InstID].fgGetFileInfo = TRUE;
    _tInFileInfo[u4InstID].pucTargetAddr = _pucVFifo[u4InstID];
    _tInFileInfo[u4InstID].u4FileOffset = (V_FIFO_SZ * ((_u4LoadBitstreamCnt[u4InstID]+ 1)/2));
    _tInFileInfo[u4InstID].u4TargetSz = (V_FIFO_SZ/2);    
    _tInFileInfo[u4InstID].u4FileLength = 0; 
  #ifdef  SATA_HDD_READ_SUPPORT
    if(!fgOpenHDDFile(u4InstID, _bFileStr1[u4InstID][1],"r+b", &_tInFileInfo[u4InstID]))
    {
      fgOpenPCFile(u4InstID, _bFileStr1[u4InstID][1],"r+b", &_tInFileInfo[u4InstID]);
    }
  #else
    fgOpenFile(u4InstID, _bFileStr1[u4InstID][1],"r+b", &_tInFileInfo[u4InstID]);
  #endif  
    _u4LoadBitstreamCnt[u4InstID]++;
  }
  else if((!(_u4LoadBitstreamCnt[u4InstID]&0x1)) && (rWmvBSInitPrm.u4ReadPointer < 
  ((UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ/2))))
  {
    _tInFileInfo[u4InstID].fgGetFileInfo = TRUE;
    _tInFileInfo[u4InstID].pucTargetAddr = _pucVFifo[u4InstID] + (V_FIFO_SZ/2);
    _tInFileInfo[u4InstID].u4FileOffset =  ((V_FIFO_SZ * (_u4LoadBitstreamCnt[u4InstID]+ 1)) /2);
    _tInFileInfo[u4InstID].u4TargetSz = (V_FIFO_SZ/2);    
    _tInFileInfo[u4InstID].u4FileLength = 0; 
  #ifdef  SATA_HDD_READ_SUPPORT
    if(!fgOpenHDDFile(u4InstID, _bFileStr1[u4InstID][1],"r+b", &_tInFileInfo[u4InstID]))
    {
      fgOpenPCFile(u4InstID, _bFileStr1[u4InstID][1],"r+b", &_tInFileInfo[u4InstID]);
    }
  #else
    fgOpenFile(u4InstID, _bFileStr1[u4InstID][1],"r+b", &_tInFileInfo[u4InstID]);
  #endif  
    _u4LoadBitstreamCnt[u4InstID]++;
  }
  #endif
  #endif
  _tVerDec[u4InstID].ucState = DEC_NORM_VPARSER;
}

// *********************************************************************
// Function    : BOOL fgIsWMVVDecComplete(UINT32 u4InstID)
// Description : Check if VDec complete with interrupt
// Parameter   : None
// Return      : None
// *********************************************************************
BOOL fgIsWMVVDecComplete(UINT32 u4InstID)
{
    printk("fgIsWMVVDecComplete\n");
  UINT32 u4MbX;
  UINT32 u4MbY;  
  VDEC_INFO_WMV_SEQ_PRM_T *prWMVSPS = &_rWMVSPS[u4InstID];
  VDEC_INFO_WMV_PIC_PRM_T *prWMVPPS = &_rWMVPPS[u4InstID];
    

  if(_fgVDecComplete[u4InstID])
  {
    vVDEC_HAL_WMV_GetMbxMby(u4InstID, &u4MbX, &u4MbY);
    if(prWMVPPS->ucFrameCodingMode != INTERLACEFIELD)
    {      
      if(u4MbX < ((prWMVSPS->u4PicWidthDec >> 4) -1) || (u4MbY < ((prWMVSPS->u4PicHeightDec >> 4) -1)))
      {
        return FALSE;
      }
      else
      {
        return TRUE;
      }
    }
    else
    {
      if(u4MbX < ((prWMVSPS->u4PicWidthDec >> 4) -1) || u4MbY < ((prWMVSPS->u4PicHeightDec >> 5) -1))
      {
        return FALSE;
      }
      else
      {
        return TRUE;
      }
    }
  }
  return FALSE;
}

void vWMVDecEndProc(UINT32 u4InstID)
{
  UINT32 u4Cnt;
  UINT32 u4CntTimeChk;
  UINT32 u4MbX;
  UINT32 u4MbY;  
  char strMessage[256];
  UINT32 u4MbX_last;
  UINT32 u4MbY_last;
  UINT32 u4mvErrType;
  VDEC_INFO_WMV_ERR_INFO_T prWmvErrInfo;
  
  u4Cnt=0;
  u4CntTimeChk = 0;
  _fgVDecErr[u4InstID] = FALSE;
  if(_rWMVPPS[u4InstID].ucPicType != SKIPFRAME)
  {
    while(u4CntTimeChk < DEC_RETRY_NUM)
    {    
      u4Cnt ++;    
      if((u4Cnt & 0x3f)== 0x3f)
      {
  #ifndef IRQ_DISABLE    
  #else
        if(u4VDEC_HAL_WMV_VDec_ReadFinishFlag(u4InstID) & 0x1)
        {
          _fgVDecComplete[u4InstID] = TRUE;
/*          if(u4InstID == 0)
          {
            BIM_ClearIrq(VECTOR_VDFUL);
          }
          else
          {
            BIM_ClearIrq(VECTOR_VDLIT);
          }
          */
        }
  #endif      
        if(fgIsWMVVDecComplete(u4InstID))
        {
          
          #ifdef CAPTURE_ESA_LOG
          vWrite2PC(u4InstID, 17, (UCHAR*)_pucESALog[u4InstID]);
          #endif
          u4CntTimeChk = 0;
          break;
        }
        else
        {
          u4MbX_last = u4MbX;
          u4MbY_last = u4MbY;
          vVDEC_HAL_WMV_GetMbxMby(u4InstID, &u4MbX, &u4MbY);
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
  
    u4mvErrType = u4VDEC_HAL_WMV_GetErrType(u4InstID);
    vVDEC_HAL_WMV_GetErrInfo(u4InstID, &prWmvErrInfo);
    if((u4CntTimeChk == DEC_RETRY_NUM) || 
      (u4mvErrType!= 0) || (prWmvErrInfo.u4WmvErrCnt != 0))
    {
    #ifndef INTERGRATION_WITH_DEMUX
    #ifdef EXT_COMPARE     
      _fgVDecErr[u4InstID] = TRUE;
    #endif
      if(u4CntTimeChk == DEC_RETRY_NUM)
      {
        vVDecOutputDebugString("\n!!!!!!!!! Decoding Timeout !!!!!!!\n");
        sprintf(strMessage, "%s", "\n!!!!!!!!! Decoding Timeout !!!!!!!");
        fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
        //vDumpReg();
      }
      vVDEC_HAL_WMV_GetMbxMby(u4InstID, &u4MbX, &u4MbY);			
      vVDecOutputDebugString("\n!!!!!!!!! Decoding Error 0x%.8x!!!!!!!\n", prWmvErrInfo.u4WmvErrType);
      sprintf(strMessage,"\n!!!!!!!!! Decoding Error 0x%.8x at MC (x,y)=(%d/%d, %d/%d)  !!!!!!!\n", u4mvErrType, 
                u4MbX, ((_tVerPic[u4InstID].u4W + 15)>> 4) - 1, u4MbY, (((_tVerPic[u4InstID].u4H >> (1-(fgWMVIsFrmPic(u4InstID)))) + 15)>> 4) - 1);
      fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
      sprintf(strMessage,"the length is %d (0x%.8x)\n", _tInFileInfo[u4InstID].u4FileLength, _tInFileInfo[u4InstID].u4FileLength);
      fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
      vReadWMVChkSumGolden(u4InstID);
      vWrite2PC(u4InstID, 1, _pucVFifo[u4InstID]);
      //vDumpReg();
    #endif
    }
    //vWMVDumpReg();
    vVDEC_HAL_WMV_GetMbxMby(u4InstID, &u4MbX, &u4MbY);            
    sprintf(strMessage,"\n!!!!!!!!! Decoding result at MC (x,y)=(%d/%d, %d/%d)  !!!!!!!\n",  
              u4MbX, ((_tVerPic[u4InstID].u4W + 15)>> 4) - 1, u4MbY, (((_tVerPic[u4InstID].u4H >> (1-(fgWMVIsFrmPic(u4InstID)))) + 15)>> 4) - 1);
    fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
    vVDEC_HAL_WMV_DecEndProcess(u4InstID);
    vVDEC_HAL_WMV_AlignRdPtr(0, u4InstID, (UINT32)_pucVFifo[u4InstID], BYTE_ALIGN);
    vVerifySetVSyncPrmBufPtr(u4InstID, _u4DecBufIdx[u4InstID]);
    vReadWMVChkSumGolden(u4InstID);
  }

  
  #if VDEC_DRAM_BUSY_TEST
       vDrmaBusyOff (u4InstID);
  #endif


#if 0//(CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8550)
  UINT32 u4MbnReg474, u4MbnReg476, u4MbnReg477;
  u4MbnReg474 = u4VDecReadMC(u4InstID, (474<<2));
  u4MbnReg476 = u4VDecReadMC(u4InstID, (476<<2));
  u4MbnReg477 = u4VDecReadMC(u4InstID, (477<<2));

  sprintf(strMessage, "\nMBN LOG_474 = 0x%.8x!!!!!!!\n", u4MbnReg474);
  fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
  sprintf(strMessage, "\nMBN LOG_476 = 0x%.8x!!!!!!!\n", u4MbnReg476);
  fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
  sprintf(strMessage, "\nMBN LOG_477 = 0x%.8x!!!!!!!\n", u4MbnReg477);
  fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
#endif

  vWMVVDecEnd(u4InstID);
}

#define ChunChia_LOG 0
void vH264DecEndProc(UINT32 u4InstID)
{
  UINT32 u4Cnt;
  UINT32 u4CntTimeChk;
  UINT32 u4Bit,u4CurrentPtr;
  //BOOL fgWaitChk;

  UINT32 u4MbX;
  UINT32 u4MbY;  
  
  UINT32 u4MbX_last;
  UINT32 u4MbY_last;  
  char strMessage[256];

#if ChunChia_LOG
  UINT32 u4Mc770, u4Mc774, u4Mc778, u4Mc8B8;
#endif

  vVDEC_HAL_H264_GetMbxMby(u4InstID,&u4MbX, &u4MbY);

  _fgVDecErr[u4InstID] = FALSE;

  u4Cnt=0;
  u4CntTimeChk = 0;

  while(u4CntTimeChk < DEC_RETRY_NUM)
  {    
    u4Cnt ++;    
    if((u4Cnt & 0x3f)== 0x3f)
    {
#ifndef IRQ_DISABLE    
#else
      if(u4VDEC_HAL_H264_VDec_ReadFinishFlag(u4InstID))
      {
        _fgVDecComplete[u4InstID] = TRUE;
 /*       if(u4InstID == 0)
        {
          BIM_ClearIrq(VECTOR_VDFUL);
        }
        else
        {
          BIM_ClearIrq(VECTOR_VDLIT);
        }*/
      }
#endif      
      if(fgIsH264VDecComplete(u4InstID))
      {
        //printk("[H264] vH264DecEndProc, dec complete, cnt:%d \n", u4CntTimeChk);
        
        #ifdef CAPTURE_ESA_LOG //fantasia H264 enable ESA log
          vWrite2PC(u4InstID, 17, (UCHAR*)_pucESALog[u4InstID]);
        #endif
        u4CntTimeChk = 0;
        break;
      }
      else
      {
        u4MbX_last = u4MbX;
        u4MbY_last = u4MbY;
        vVDEC_HAL_H264_GetMbxMby(u4InstID,&u4MbX, &u4MbY);
        //vVDecOutputDebugString("\nMbX = %d, MbY = %d\n", u4MbX,u4MbY);
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
  
  vVDEC_HAL_H264_VDec_PowerDown(u4InstID);

  if((u4CntTimeChk == DEC_RETRY_NUM) || 
    ((u4VDEC_HAL_H264_GetErrMsg(u4InstID) != 0)
      && (!(((u4VDEC_HAL_H264_GetErrMsg(u4InstID) == 8) || (u4VDEC_HAL_H264_GetErrMsg(u4InstID) == 0x40)) && (fgVDEC_HAL_H264_DecPicComplete(u4InstID))))))
  {
#ifndef INTERGRATION_WITH_DEMUX  
#ifdef EXT_COMPARE     
    _fgVDecErr[u4InstID] = TRUE;
#endif
    if(u4CntTimeChk == DEC_RETRY_NUM)
    {
      vVDecOutputDebugString("\n!!!!!!!!! Decoding Timeout !!!!!!!\n");
      sprintf(strMessage, "%s", "\n!!!!!!!!! Decoding Timeout !!!!!!!");
      fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
      vVDEC_HAL_H264_VDec_DumpReg(u4InstID);
    }
    vVDEC_HAL_H264_GetMbxMby(u4InstID,&u4MbX, &u4MbY);
    vVDecOutputDebugString("\n!!!!!!!!! Decoding Error 0x%.8x in pic %d (frm %d) !!!!!!!\n", u4VDEC_HAL_H264_GetErrMsg(u4InstID), _u4PicCnt[u4InstID], _u4FileCnt[u4InstID]);
    sprintf(strMessage,"\n!!!!!!!!! Decoding Error 0x%.8x at MC (x,y)=(%d/%d, %d/%d) in pic %d (frm %d) !!!!!!!\n", u4VDEC_HAL_H264_GetErrMsg(u4InstID), 
              u4MbX, ((_ptCurrFBufInfo[u4InstID]->u4W + 15)>> 4) - 1, u4MbY, (((_ptCurrFBufInfo[u4InstID]->u4H >> (1-(fgIsFrmPic(u4InstID)))) + 15)>> 4) - 1, _u4PicCnt[u4InstID], _u4FileCnt[u4InstID]);
    fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
    sprintf(strMessage,"the length is %d (0x%.8x)\n", _tInFileInfo[u4InstID].u4FileLength, _tInFileInfo[u4InstID].u4FileLength);
    fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
    //vWrite2PC(u4InstID, 1, _pucVFifo[u4InstID]);
    vVDEC_HAL_H264_VDec_DumpReg(u4InstID);
    //fgWaitChk = TRUE;
    //while(fgWaitChk);
  #endif
  }
  vReadH264ChkSumGolden(u4InstID);
  // @@ fantasia -> 2012-02-24 don't show the registers value 
  //vH264ChkSumDump(u4InstID);
//Print LOG
#if ChunChia_LOG  
  u4Mc770 = u4VDecReadMC(u4InstID, 0x770);
  u4Mc774 = u4VDecReadMC(u4InstID, 0x774);
  u4Mc778 = u4VDecReadMC(u4InstID, 0x778);
  u4Mc8B8 = u4VDecReadMC(u4InstID, 0x8B8);

  sprintf(strMessage,"======\n");  
  printk("%s", strMessage);
  
  sprintf(strMessage,"(dram_dle_cnt: 0x%x, mc_dle_cnt: 0x%x, cycle_cnt: 0x%x, dram_dle_by_preq: 0x%x)\n", u4Mc770, u4Mc774, u4Mc778, u4Mc8B8);
  printk("%s", strMessage);

#endif
  u4CurrentPtr = u4VDEC_HAL_H264_ReadRdPtr(_u4BSID[u4InstID], u4InstID, (UINT32)_pucVFifo[u4InstID], &u4Bit);
  if(u4CurrentPtr < _u4PrevPtr[u4InstID])//HW is ring,so read fifo overflow
  {
  	printk("HW decode overflow ........!u4CurrentPtr = 0x%x,VFIFO = 0x%x\n",u4CurrentPtr,V_FIFO_SZ);
  }
  _u4PrevPtr[u4InstID] = u4CurrentPtr;
  vH264VDecEnd(u4InstID);  
}


void vH264ChkSumDump(UINT32 u4InstID)
{
	UINT32 i,u4regval;
	printk("H264 Check sum!\n");
	for(i = 72; i < 76; i++)
	{
		printk("VLD #%d(0x%x) = 0x%x\n",i,i<<2,u4VDecReadVLD(u4InstID,i<<2));
	}

	for(i = 378; i < 386; i++)
	{
		printk("MC #%d(0x%x) = 0x%x\n",i,i<<2,u4VDecReadMC(u4InstID,i<<2));
	}
	for(i = 388; i < 398; i++)
	{
		printk("MC #%d(0x%x) = 0x%x\n",i,i<<2,u4VDecReadMC(u4InstID,i<<2));
	}
	for(i = 479; i < 482; i++)
	{
		printk("MC #%d(0x%x) = 0x%x\n",i,i<<2,u4VDecReadMC(u4InstID,i<<2));
	}

	printk("MC #%d(0x%x) = 0x%x\n",483,483<<2,u4VDecReadMC(u4InstID,483<<2));
	printk("MC #%d(0x%x) = 0x%x\n",571,571<<2,u4VDecReadMC(u4InstID,571<<2));
	printk("MC #%d(0x%x) = 0x%x\n",498,498<<2,u4VDecReadMC(u4InstID,498<<2));
	printk("MC #%d(0x%x) = 0x%x\n",446,446<<2,u4VDecReadMC(u4InstID,446<<2));

	for(i = 147; i < 152; i++)
	{
		printk("AVCMV #%d(0x%x) = 0x%x\n",i,i<<2,u4VDecReadAVCMV(u4InstID,i<<2));
	}

	for(i = 41; i < 45; i++)
	{
		printk("VLDTOP #%d(0x%x) = 0x%x\n",i,i<<2,u4VDecReadVLDTOP(u4InstID,i<<2));
	}

	for(i = 165; i < 169; i++)
	{
		printk("AVCVLD #%d(0x%x) = 0x%x\n",i,i<<2,u4VDecReadAVCVLD(u4InstID,i<<2));
	}
	for(i = 175; i < 177; i++)
	{
		printk("AVCVLD #%d(0x%x) = 0x%x\n",i,i<<2,u4VDecReadAVCVLD(u4InstID,i<<2));
	}

}


// *********************************************************************
// Function    : BOOL fgIsMPEGVDecComplete(UINT32 u4InstID)
// Description : Check if VDec complete with interrupt
// Parameter   : None
// Return      : None
// *********************************************************************
BOOL fgIsMPEGVDecComplete(UINT32 u4InstID)
{
  UINT32 u4MbX;
  UINT32 u4MbY;  
    

  if(_fgVDecComplete[u4InstID])
  {
    vVDEC_HAL_MPEG_GetMbxMby(u4InstID, &u4MbX, &u4MbY);
    if(_u4PicStruct[u4InstID] == FRM_PIC)
    {      
      if((u4MbX < (((_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.u4DecW + _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.u4DecXOff + 15) >> 4) -1))
        || (u4MbY < (((_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.u4DecH + _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.u4DecYOff + 15) >> 4) -1)))
      {
        return FALSE;
      }
      else
      {
        return TRUE;
      }
    }
    else
    {
      if((u4MbX < (((_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.u4DecW + _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.u4DecXOff + 15) >> 4) -1))
        || u4MbY < (((_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.u4DecH + _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.u4DecYOff + 15) >> 5) -1))
      {
        return FALSE;
      }
      else
      {
        return TRUE;
      }
    }
  }
  return FALSE;
}

//Qing Li add here for dump reg and pic raw data
extern void VDecDumpMP4Register(UINT32 u4VDecID);
extern void VDecDumpMpegRegister(UINT32 u4VDecID,BOOL fgTriggerAB);

BOOL   _fgDumpDeblocky = FALSE;
UINT32 _u4MpvEmuDumpCount = 0;
UINT32 _u4MpvEmuDumpStartPicCount = (UINT32)(-1);
UINT32 _u4MpvEmuDumpEndPicCount = (UINT32)(-1);
FILE*  _pFileHandleY = NULL;
FILE*  _pFileHandleCbcr = NULL;
FILE*  _pFileHandleDeblockyY = NULL;
FILE*  _pFileHandleDeblockyCbcr = NULL;

void vMPEGVDecDumpPic(UINT32 u4InstID)
{
    CHAR*  pFileNameY = "B:\\dumppic\\YGroupPic.raw";
    CHAR*  pFileNameCbcr = "B:\\dumppic\\CGroupPic.raw";
    CHAR*  pFileNameYDeblocky = "B:\\dumppic\\YGroupPic_Deblocky.raw";
    CHAR*  pFileNameCbcrDeblocky = "B:\\dumppic\\CGroupPic_Deblocky.raw";
    FILE*  pFileTemp = NULL;
    UINT32 u4DataSizeY = 0;
    UINT32 u4DataSizeCbcr = 0;
    UINT32 u4SizeV = 0;
    UINT32 u4SizeH = 0;

    if ((_u4FileCnt[u4InstID] < _u4MpvEmuDumpStartPicCount) || 
        (_u4FileCnt[u4InstID] > _u4MpvEmuDumpEndPicCount))
    {
        return;
    }

    printk("MPV Emu start to dump No.%d pic to PC\n", _u4FileCnt[u4InstID]);

    if (0 != (_u4RealHSize[u4InstID] * _u4RealVSize[u4InstID]))
    {
        u4SizeH = _u4RealHSize[u4InstID];
        u4SizeV = _u4RealVSize[u4InstID];
    }
    else
    {
        u4SizeH = _u4HSizeVal[u4InstID];
        u4SizeV = _u4VSizeVal[u4InstID];
    }
    
    u4DataSizeY = (((u4SizeH + 15)>>4)<<4) * (((u4SizeV + 31)>>5)<<5);
    u4DataSizeCbcr = u4DataSizeY / 2;
    printk("MPV Emu H size %d\n", (((u4SizeH + 15)>>4)<<4));
    printk("MPV Emu V size %d\n", (((u4SizeV + 31)>>5)<<5));

    if (!_fgDumpDeblocky)
    {
        /* Dump Y */
        do
        {
            if (NULL == _pFileHandleY)
            {
                pFileTemp = linux_fopen(pFileNameY, "wb");
                if (NULL == pFileTemp)
                {
                    printk("MPV Emu Create %s fail\n", pFileNameY);
                    break;
                }
        
                _pFileHandleY = pFileTemp;
                printk("MPV Emu create %s success\n", pFileNameY);
            }
        
            if (u4DataSizeY != linux_fwrite ((char* )(_pucDecWorkBuf[u4InstID]), 1, u4DataSizeY, _pFileHandleY))
            {
                printk("MPV Emu Write to %s fail\n", pFileNameY);
                printk("MPV Emu need to Write data count %d\n", u4DataSizeY);
                linux_fclose(_pFileHandleY);
                _pFileHandleY = NULL;
                printk("MPV Emu close file %s\n", pFileNameY);
            }
        
            printk("MPV Emu Write to %s success\n", pFileNameY);
        } while(0);
        
        /* Dump Cbcr */
        do
        {
            if (NULL == _pFileHandleCbcr)
            {
                pFileTemp = linux_fopen(pFileNameCbcr, "wb");
                if (NULL == pFileTemp)
                {
                    printk("MPV Emu Create %s fail\n", pFileNameCbcr);
                    break;
                }
        
                _pFileHandleCbcr = pFileTemp;
                printk("MPV Emu create %s success\n", pFileNameCbcr);
            }
        
            if (u4DataSizeCbcr != linux_fwrite ((char* )(_pucDecCWorkBuf[u4InstID]), 1, u4DataSizeCbcr, _pFileHandleCbcr))
            {
                printk("MPV Emu Write to %s fail\n", pFileNameCbcr);
                printk("MPV Emu need to Write data count %d\n", u4DataSizeCbcr);
                linux_fclose(_pFileHandleCbcr);
                _pFileHandleCbcr = NULL;
                printk("MPV Emu close file %s\n", pFileNameCbcr);
            }
        
            printk("MPV Emu Write to %s success\n", pFileNameCbcr);
        } while(0);
    }
    else
    {
        /* Dump Deblocking Y */
        do
        {
            if (NULL == _pFileHandleDeblockyY)
            {
                pFileTemp = linux_fopen(pFileNameYDeblocky, "wb");
                if (NULL == pFileTemp)
                {
                    printk("MPV Emu Create %s fail\n", pFileNameYDeblocky);
                    break;
                }
        
                _pFileHandleDeblockyY = pFileTemp;
                printk("MPV Emu create %s success\n", pFileNameYDeblocky);
            }
        
            if (u4DataSizeY != linux_fwrite ((char* )(_pucPpYSa[u4InstID]), 1, u4DataSizeY, _pFileHandleDeblockyY))
            {
                printk("MPV Emu Write to %s fail\n", pFileNameYDeblocky);
                printk("MPV Emu need to Write data count %d\n", u4DataSizeY);
                linux_fclose(_pFileHandleDeblockyY);
                _pFileHandleDeblockyY = NULL;
                printk("MPV Emu close file %s\n", pFileNameYDeblocky);
            }
        
            printk("MPV Emu Write to %s success\n", pFileNameYDeblocky);
        } while(0);
        
        /* Dump Deblocking Cbcr */
        do
        {
            if (NULL == _pFileHandleDeblockyCbcr)
            {
                pFileTemp = linux_fopen(pFileNameCbcrDeblocky, "wb");
                if (NULL == pFileTemp)
                {
                    printk("MPV Emu Create %s fail\n", pFileNameCbcrDeblocky);
                    break;
                }
        
                _pFileHandleDeblockyCbcr = pFileTemp;
                printk("MPV Emu create %s success\n", pFileNameCbcrDeblocky);
            }
        
            if (u4DataSizeCbcr != linux_fwrite ((char* )(_pucPpCSa[u4InstID]), 1, u4DataSizeCbcr, _pFileHandleDeblockyCbcr))
            {
                printk("MPV Emu Write to %s fail\n", pFileNameCbcrDeblocky);
                printk("MPV Emu need to Write data count %d\n", u4DataSizeCbcr);
                linux_fclose(_pFileHandleDeblockyCbcr);
                _pFileHandleDeblockyCbcr = NULL;
                printk("MPV Emu close file %s\n", pFileNameCbcrDeblocky);
            }
        
            printk("MPV Emu Write to %s success\n", pFileNameCbcrDeblocky);
        } while(0);

    }

    _u4MpvEmuDumpCount ++;
    printk("MPV Emu Has written %d pic to PC\n", _u4MpvEmuDumpCount);

    // close all files after completing writing raw data to these files
    if (_u4FileCnt[u4InstID] == _u4MpvEmuDumpEndPicCount)
    {
        if (_pFileHandleY)
        {
            linux_fclose(_pFileHandleY);
            _pFileHandleY = NULL;
            printk("MPV Emu close file %s\n", pFileNameY);
        }

        if (_pFileHandleCbcr)
        {
            linux_fclose(_pFileHandleCbcr);
            _pFileHandleCbcr = NULL;
            printk("MPV Emu close file %s\n", pFileNameCbcr);
        }

        if (_pFileHandleDeblockyY)
        {
            linux_fclose(_pFileHandleDeblockyY);
            _pFileHandleDeblockyY = NULL;
            printk("MPV Emu close file %s\n", pFileNameYDeblocky);
        }

        if (_pFileHandleDeblockyCbcr)
        {
            linux_fclose(_pFileHandleDeblockyCbcr);
            _pFileHandleDeblockyCbcr = NULL;
            printk("MPV Emu close file %s\n", pFileNameCbcrDeblocky);
        }

        _fgDumpDeblocky = FALSE;
        _u4MpvEmuDumpStartPicCount = (UINT32)(-1);
        _u4MpvEmuDumpEndPicCount = (UINT32)(-1);
        _u4MpvEmuDumpCount = 0;
    }
}

BOOL _fgDumpReg = FALSE;
void vMPEGVDecEnd(UINT32 u4InstID)
{
  //VDEC_INFO_DEC_PRM_T *tVerMpvDecPrm;
  //VDEC_INFO_H264_FBUF_INFO_T *tFBufInfo;
  VDEC_INFO_MPEG_VFIFO_PRM_T rMpegVFifoInitPrm;
  VDEC_INFO_MPEG_BS_INIT_PRM_T rMpegBSInitPrm;
  UINT32 u4VldByte,u4VldBit;

  rMpegVFifoInitPrm.u4CodeType = _u4CodecVer[u4InstID];

  //tFBufInfo = _ptCurrFBufInfo[u4InstID];
  //tVerMpvDecPrm = &_tVerMpvDecPrm[u4InstID];

  if(_fgVerVopCoded0[u4InstID])
  {
    u4VldByte = u4VDEC_HAL_MPEG_ReadRdPtr(0, u4InstID, (UINT32)_pucVFifo[u4InstID], &u4VldBit) + 4;
  }
  else
  {
    u4VldByte = u4VDEC_HAL_MPEG_ReadRdPtr(0, u4InstID, (UINT32)_pucVFifo[u4InstID], &u4VldBit) - 4;
  }
  _u4WMVByteCount[u4InstID] = u4VldByte;

#ifdef LETTERBOX_DETECTION_ONLY  
  vCheckLBDResult(u4InstID);
#else    
  //if (_u4CodecVer[u4InstID] == VDEC_MPEG4)
  if (1)  //Cheng-Jung 20120305 Use CRC comparison for MPEG4, H263 and DIVX3
  {
  #ifdef MPEG4_CRC_CMP  
  #ifndef VPMODE  
  if(!_fgVerVopCoded0[u4InstID])  
  #endif	
  {		
  	vMPEG4CrcCmp(u4InstID,NULL,0);	
  }  
  #else
  vMPEGWrData2PC(u4InstID, _pucDumpYBuf[u4InstID], ((((_u4RealHSize[u4InstID] + 15) >> 4) * ((_u4RealVSize[u4InstID] + 31) >> 5)) << 9));
  #endif
  }
  else
  {
      vMPEGWrData2PC(u4InstID, _pucDumpYBuf[u4InstID], ((((_u4RealHSize[u4InstID] + 15) >> 4) * ((_u4RealVSize[u4InstID] + 31) >> 5)) << 9));
  }
#endif
/*
  if (_fgDumpReg)
  {
      printk("MPV Dump register after decode\n");
      VDecDumpMpegRegister(u4InstID,1);
      printk("\n MPV Dump register end \n");
  }
*/
  // reset HW
#ifdef REDEC   
  if(_u4ReDecCnt[u4InstID] > 0)
  {
    rMpegVFifoInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
    rMpegVFifoInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
    i4VDEC_HAL_MPEG_InitVDecHW(u4InstID,&rMpegVFifoInitPrm);
    rMpegBSInitPrm.u4VFifoSa =  (UINT32)_pucVFifo[u4InstID];
    rMpegBSInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
    rMpegBSInitPrm.u4ReadPointer =  (UINT32)_pucVFifo[u4InstID] + _u4VLDPosByte[u4InstID];
  #ifndef  RING_VFIFO_SUPPORT
    rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
  #else
  //		rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ*(0.5 + 0.5 *(_u4LoadBitstreamCnt[u4InstID]%2)));
	  rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));//mtk40343
  #endif
    i4VDEC_HAL_MPEG_InitBarrelShifter(0, u4InstID, &rMpegBSInitPrm);
    u4VDEC_HAL_MPEG_ShiftGetBitStream(0, u4InstID, _u4VLDPosBit[u4InstID]);
    // Restore Quantization Matrix
    if(_fgVerLoadIntraMatrix[u4InstID])
    {
      vVDEC_HAL_MPEG_ReLoadQuantMatrix(u4InstID, TRUE);
    }
    if(_fgVerLoadNonIntraMatrix[u4InstID])
    {
      vVDEC_HAL_MPEG_ReLoadQuantMatrix(u4InstID, FALSE);
    }
    _tVerDec[u4InstID].ucState = DEC_NORM_WAIT_TO_DEC;
    return;
  }
#endif

  if(!fgMPEGIsFrmPic(u4InstID))
  {
    _fgDec2ndFldPic[u4InstID] = 1 - _fgDec2ndFldPic[u4InstID];//vToggleDecFlag(DEC_FLG_2ND_FLD_PIC);
  }
  _u4MpegDecPicNo[u4InstID]++;
  
  if(_u4CodecVer[u4InstID] == VDEC_DIVX3)
  {
    rMpegVFifoInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
    rMpegVFifoInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
    i4VDEC_HAL_MPEG_InitVDecHW(u4InstID,&rMpegVFifoInitPrm);
    if(_u4Divx3SetPos[u4InstID] >= V_FIFO_SZ)
    {
      _u4Divx3SetPos[u4InstID] = _u4Divx3SetPos[u4InstID] - V_FIFO_SZ;
    }
    rMpegBSInitPrm.u4VFifoSa =  (UINT32)_pucVFifo[u4InstID];
    rMpegBSInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
    rMpegBSInitPrm.u4ReadPointer =  (UINT32)_pucVFifo[u4InstID] + _u4Divx3SetPos[u4InstID];
  #ifndef  RING_VFIFO_SUPPORT
    rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
  #else
  //		rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ*(0.5 + 0.5 *(_u4LoadBitstreamCnt[u4InstID]%2)));
	  rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));//mtk40343
  #endif
    i4VDEC_HAL_MPEG_InitBarrelShifter(_u4BSID[u4InstID], u4InstID, &rMpegBSInitPrm);
    // Restore Quantization Matrix
    if(_fgVerLoadIntraMatrix[u4InstID])
    {
      vVDEC_HAL_MPEG_ReLoadQuantMatrix(u4InstID, TRUE);
    }
    if(_fgVerLoadNonIntraMatrix[u4InstID])
    {
      vVDEC_HAL_MPEG_ReLoadQuantMatrix(u4InstID, FALSE);
    }
  }
  else // MPEG
  {
    rMpegVFifoInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
    rMpegVFifoInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
    i4VDEC_HAL_MPEG_InitVDecHW(u4InstID,&rMpegVFifoInitPrm);
    rMpegBSInitPrm.u4VFifoSa =  (UINT32)_pucVFifo[u4InstID];
    rMpegBSInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
    rMpegBSInitPrm.u4ReadPointer =  (UINT32)_pucVFifo[u4InstID] + u4VldByte;
  #ifndef  RING_VFIFO_SUPPORT
    rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
  #else
  //		rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ*(0.5 + 0.5 *(_u4LoadBitstreamCnt[u4InstID]%2)));
	  rMpegBSInitPrm.u4WritePointer = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));//mtk40343
  #endif
    i4VDEC_HAL_MPEG_InitBarrelShifter(_u4BSID[u4InstID], u4InstID, &rMpegBSInitPrm);
  
    // Restore Quantization Matrix
    if(_fgVerLoadIntraMatrix[u4InstID])
    {
      vVDEC_HAL_MPEG_ReLoadQuantMatrix(u4InstID, TRUE);
    }
    if(_fgVerLoadNonIntraMatrix[u4InstID])
    {
      vVDEC_HAL_MPEG_ReLoadQuantMatrix(u4InstID, FALSE);
    }
  }
  
  //6589NEW (4) Error concealment end of bitstream workaround
  #ifdef MPEG4_6589_ERROR_CONCEAL
  //printk("<vdec> Bitstream pos: %d, total len: %d\n", u4VldByte, _u4TotalBitstreamLen[u4InstID]);
  if (u4VldByte >= _u4TotalBitstreamLen[u4InstID] - 20)
  {
      _u4VerBitCount[u4InstID] = 0xffffffff;
  }
  #endif

#ifndef INTERGRATION_WITH_DEMUX
#ifdef  RING_VFIFO_SUPPORT
  if((_u4LoadBitstreamCnt[u4InstID]&0x1) && (rMpegBSInitPrm.u4ReadPointer  > 
  ((UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ/2))))
  {
    _tInFileInfo[u4InstID].fgGetFileInfo = TRUE;
    _tInFileInfo[u4InstID].pucTargetAddr = _pucVFifo[u4InstID];
    _tInFileInfo[u4InstID].u4FileOffset = (V_FIFO_SZ * ((_u4LoadBitstreamCnt[u4InstID]+ 1)/2));
    _tInFileInfo[u4InstID].u4TargetSz = (V_FIFO_SZ/2);    
    _tInFileInfo[u4InstID].u4FileLength = 0; 
  #ifdef  SATA_HDD_READ_SUPPORT
    if(!fgOpenHDDFile(u4InstID, _bFileStr1[u4InstID][1],"r+b", &_tInFileInfo[u4InstID]))
    {
      fgOpenPCFile(u4InstID, _bFileStr1[u4InstID][1],"r+b", &_tInFileInfo[u4InstID]);
    }
  #else
    fgOpenPCFile(u4InstID, _bFileStr1[u4InstID][1],"r+b", &_tInFileInfo[u4InstID]);
  #endif  
    _u4LoadBitstreamCnt[u4InstID]++;
  }
  else if((!(_u4LoadBitstreamCnt[u4InstID]&0x1)) && (rMpegBSInitPrm.u4ReadPointer  < 
  ((UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ/2))))
  {
    _tInFileInfo[u4InstID].fgGetFileInfo = TRUE;
    _tInFileInfo[u4InstID].pucTargetAddr = _pucVFifo[u4InstID] + (V_FIFO_SZ/2);
    _tInFileInfo[u4InstID].u4FileOffset =  ((V_FIFO_SZ * (_u4LoadBitstreamCnt[u4InstID]+ 1)) /2);
    _tInFileInfo[u4InstID].u4TargetSz = (V_FIFO_SZ/2);    
    _tInFileInfo[u4InstID].u4FileLength = 0; 
  #ifdef  SATA_HDD_READ_SUPPORT
    if(!fgOpenHDDFile(u4InstID, _bFileStr1[u4InstID][1],"r+b", &_tInFileInfo[u4InstID]))
    {
      fgOpenPCFile(u4InstID, _bFileStr1[u4InstID][1],"r+b", &_tInFileInfo[u4InstID]);
    }
  #else
    fgOpenPCFile(u4InstID, _bFileStr1[u4InstID][1],"r+b", &_tInFileInfo[u4InstID]);
  #endif  
    _u4LoadBitstreamCnt[u4InstID]++;
  }
#endif
#endif
  _tVerDec[u4InstID].ucState = DEC_NORM_VPARSER;
}

void vMPEGDecEndProc(UINT32 u4InstID)
{
  #ifdef IRQ_DISABLE 
  BOOL fgMpeg4;
  #endif
  UINT32 u4Cnt;
  UINT32 u4CntTimeChk;
  UINT32 u4MbX;
  UINT32 u4MbY;  
  char strMessage[256];
  UINT32 u4MbX_last;
  UINT32 u4MbY_last;
  UINT32 u4MpegErrType = 0;
  VDEC_INFO_MPEG_ERR_INFO_T prMpegErrInfo;
  UINT32 u4RegVal;
  INT32 i;
  
  u4Cnt=0;
  u4CntTimeChk = 0;
  _fgVDecErr[u4InstID] = FALSE;
  #ifndef VPMODE
  if(!_fgVerVopCoded0[u4InstID])
  #endif
  {
    #ifdef IRQ_DISABLE 
    fgMpeg4 = (_u4CodecVer[u4InstID] != VDEC_MPEG2)? TRUE : FALSE;
    #endif

    while(u4CntTimeChk < DEC_RETRY_NUM)
    {    
      u4Cnt ++;    
      if((u4Cnt & 0x3f)== 0x3f)
      {
  #ifndef IRQ_DISABLE    
  #else
        if(u4VDEC_HAL_MPEG_VDec_ReadFinishFlag(u4InstID, fgMpeg4))
        {
          UINT32 u4VldBit;
          #ifdef REG_LOG_NEW
          #ifndef MPEG4_6589_ERROR_CONCEAL
          #if (DUMP_ERROR == 0)
          printk("[VDEC] End decoding _u4FileCnt = %d, dump = %d\n", _u4FileCnt[u4InstID], _u4DumpRegPicNum[u4InstID]);
          if (_u4FileCnt[u4InstID] == _u4DumpRegPicNum[u4InstID]) 
          #endif
          //if (u4VDEC_HAL_MPEG_VDec_ReadErrorFlag(u4InstID)) // REMOVE
          {
            _fgRegLogConsole[u4InstID] = FALSE;
            VDecDumpMP4Register(u4InstID);          
            fgWrData2PC(_pucRegisterLog[u4InstID],_u4RegisterLogLen[u4InstID],7,_RegFileName[u4InstID]);
            _u4RegisterLogLen[u4InstID] = 0;
            _fgRegLogConsole[u4InstID] = TRUE;
          }
          #else          
          if (u4VDEC_HAL_MPEG_VDec_ReadErrorFlag(u4InstID))
          {
              _fgRegLogConsole[u4InstID] = FALSE;
              VDecDumpMP4Register(u4InstID);          
              fgWrData2PC(_pucRegisterLog[u4InstID],_u4RegisterLogLen[u4InstID],7,_RegFileName[u4InstID]);
              _u4RegisterLogLen[u4InstID] = 0;
              _fgRegLogConsole[u4InstID] = TRUE;
          }
          #endif          
          #endif

          _fgVDecComplete[u4InstID] = TRUE;
		  //printk("<vdec> got finish flag\n");
  		  //u4VDEC_HAL_MPEG_ReadRdPtr(0, u4InstID, (UINT32)_pucVFifo[u4InstID], &u4VldBit);
		  #ifdef CAPTURE_ESA_LOG
          vWrite2PC(u4InstID, 17, (UCHAR*)_pucESALog[u4InstID]);
          #endif
/*          if(u4InstID == 0)
          {
            BIM_ClearIrq(VECTOR_VDFUL);
          }
          else
          {
            BIM_ClearIrq(VECTOR_VDLIT);
          }*/
        }
  #endif      
        if(fgIsMPEGVDecComplete(u4InstID))
        {
          u4CntTimeChk = 0;
          break;
        }
        else
        {
          u4MbX_last = u4MbX;
          u4MbY_last = u4MbY;
          vVDEC_HAL_MPEG_GetMbxMby(u4InstID, &u4MbX, &u4MbY);
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

        if (_fgVDecComplete[u4InstID])
        {
            break;
        }
      }
    }
  
    if(_ucMpegVer[u4InstID] != VDEC_MPEG2)
    {
      u4MpegErrType = u4VDEC_HAL_MPEG4_GetErrType(u4InstID);
    }
    vVDEC_HAL_MPEG_GetErrInfo(u4InstID, &prMpegErrInfo);
    if((u4CntTimeChk == DEC_RETRY_NUM) || 
      (u4MpegErrType!= 0) || (prMpegErrInfo.u4MpegErrCnt != 0))
    {
    #ifndef INTERGRATION_WITH_DEMUX
    //#ifdef EXT_COMPARE     
      _fgVDecErr[u4InstID] = TRUE;
    //#endif
      if(u4CntTimeChk == DEC_RETRY_NUM)
      {
        vVDecOutputDebugString("\n<vdec> !!!!!!!!! Decoding Timeout !!!!!!!\n");
        sprintf(strMessage, "%s", "\n<vdec> !!!!!!!!! Decoding Timeout !!!!!!!\n");
        fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
        //vDumpReg();
#ifdef REG_LOG_NEW
#if (DUMP_ERROR == 0)
        printk("[VDEC] End decoding (timeout) _u4FileCnt = %d, dump = %d\n", _u4FileCnt[u4InstID], _u4DumpRegPicNum[u4InstID]);
        if (_u4FileCnt[u4InstID] == _u4DumpRegPicNum[u4InstID]) 
#endif
        {
          _fgRegLogConsole[u4InstID] = FALSE;
          VDecDumpMP4Register(u4InstID);          
          fgWrData2PC(_pucRegisterLog[u4InstID],_u4RegisterLogLen[u4InstID],7,_RegFileName[u4InstID]);
          _u4RegisterLogLen[u4InstID] = 0;
          _fgRegLogConsole[u4InstID] = TRUE;
        }
#endif
      }

      
      vVDEC_HAL_MPEG_GetMbxMby(u4InstID, &u4MbX, &u4MbY);
      vVDecOutputDebugString("\n<vdec> !!!!!!!!! Decoding Error 0x%.8x!!!!!!!\n", prMpegErrInfo.u4MpegErrType);
      sprintf(strMessage,"\n//<vdec> !!!!!!!!! Decoding Error 0x%.8x 0x%.8x 0x%.8x at MC (x,y)=(%d/%d, %d/%d)  !!!!!!!\n", u4MpegErrType, 
                prMpegErrInfo.u4MpegErrType,prMpegErrInfo.u4MpegErrRow,u4MbX, ((_tVerPic[u4InstID].u4W + 15)>> 4) - 1, u4MbY, 
                (((_tVerPic[u4InstID].u4H >> (1-(fgMPEGIsFrmPic(u4InstID)))) + 15)>> 4) - 1);
      printk("%s\n", strMessage);
//      fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
      sprintf(strMessage,"<vdec> the length is %d (0x%.8x)\n", _tInFileInfo[u4InstID].u4FileLength, _tInFileInfo[u4InstID].u4FileLength);
//      fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
      vReadMPEGChkSumGolden(u4InstID);
//      vWrite2PC(u4InstID, 1, _pucVFifo[u4InstID]);
//      vWrite2PC(u4InstID, 12, (UCHAR *)(&_u4DumpChksum[u4InstID][0]));
      //vDumpReg();
    #endif
    }
    if (u4VDEC_HAL_MPEG_VDec_ReadErrorFlag(u4InstID))
    {
        printk("VLD_243[8] = 1, error occurs\n");
        for (i=241; i<246; i++)
        {
            u4RegVal = u4VDecReadVLD(u4InstID, i*4);
            printk("VLD_%d = 0x%08x\n", i, u4RegVal);
        }        
    }
    else
    {
        printk("DECODING FINISH WITH NO ERROR!!!\n");
    }
    if(_ucMpegVer[u4InstID] != VDEC_MPEG2)
    {
      vVDEC_HAL_MPEG_VLDVdec2Barl(u4InstID);
    }
    if(_ucMpegVer[u4InstID] == VDEC_DIVX3)
    {
      vVerifyDx3SufxChk(u4InstID);
    }
    if(_u4CodecVer[u4InstID] == VDEC_MPEG4)
    {
    //6589NEW (C)
    #if (!MPEG4_6589_SUPPORT)
      // Mpeg4 workaround
      vMp4FixBCode(u4InstID);
    #endif
    }
    vVDEC_HAL_MPEG_AlignRdPtr(0, u4InstID, (UINT32)_pucVFifo[u4InstID], BYTE_ALIGN);
    vVerifySetVSyncPrmBufPtr(u4InstID, _u4DecBufIdx[u4InstID]);
    vReadMPEGChkSumGolden(u4InstID);
  }

#if 0// Qing Li add here for test speed log
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8550)
	UINT32 u4MbnReg474, u4MbnReg476, u4MbnReg477;
	u4MbnReg474 = u4VDecReadMC(u4InstID, (474<<2));
	u4MbnReg476 = u4VDecReadMC(u4InstID, (476<<2));
	u4MbnReg477 = u4VDecReadMC(u4InstID, (477<<2));

	sprintf(strMessage, "\nMBN LOG_474 = 0x%.8x!!!!!!!\n", u4MbnReg474);
	fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
	sprintf(strMessage, "\nMBN LOG_476 = 0x%.8x!!!!!!!\n", u4MbnReg476);
	fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
	sprintf(strMessage, "\nMBN LOG_477 = 0x%.8x!!!!!!!\n", u4MbnReg477);
	fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
#endif
#endif
        vMPEGVDecDumpPic(u4InstID);
#if 0 //mc performance
  {
  UINT32 u4CodeType,u4CycleDram;
  UCHAR u4TypeCode[2];
  u4CodeType = u4VDecReadVLD(u4InstID,RW_VLD_VOP_TYPE)&0x0f;
  switch(u4CodeType)
  {
	  case 0:
		  u4TypeCode[0] = 'I';
		  break;
	  case 2:
		  u4TypeCode[0] = 'P';
		  break;
	  case 4:
		  u4TypeCode[0] = 'B';
		  break;
	  case 8:
		  u4TypeCode[0] = 'S';
		  break;
  }
  u4TypeCode[1] = '\0';
  u4CycleDram = u4VDecReadMC(u4InstID,RO_MC_DRAM_CYCLE);
  printk("VDEC_perf_measure: seq_name=%s pic_idx=%d pic_width=%d pic_height=%d pic_type=%s CYCLE_DRAM=%d\n",_bFileStr1[u4InstID][8],
			   _u4FileCnt[u4InstID],_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.u4DecW,_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecMPEGDecPrm.u4DecH,
			   u4TypeCode,u4CycleDram);
  printk("DRAM BEHAVIOR SETTING 0x%x\n",u4VDecReadMC(u4InstID,RW_MC_PARA_BEHAVIOR));
  }
#endif
    #if 0
    if(_u4FileCnt[u4InstID] == 5)
    {
    	printk("WorkAround before 4139 = 0x%x,4140 = 0x%x\n",u4ReadSram(u4InstID,4139),u4ReadSram(u4InstID,4140));
    	vWriteSram(u4InstID,4139,u4ReadSram(u4InstID,4140));
	printk("WorkAround after 4139 = 0x%x,4140 = 0x%x\n",u4ReadSram(u4InstID,4139),u4ReadSram(u4InstID,4140));	
    }
    #endif
	
    //dump sram ==>dcac and bcode data
    #if 0
	vDumpSram(u4InstID);
    #endif

  #if 0
  {
    UINT i;
    for(i = 2; i < 10; i++)
    {
      printk("CRC %d == 0x%x\n",i,u4VDecReadCRC(u4InstID,i<<2));
    }
  }
  #endif
  #if CONFIG_DRV_VERIFY_SUPPORT 
    #if (DUMP_ERROR == 0)
    if (_u4FileCnt[u4InstID] == _u4DumpRegPicNum[u4InstID])
	#endif
    {
       //vVDEC_HAL_MPEG_VDec_DumpReg(u4VDecID, TRUE);
       VDecDumpMpegRegister(u4InstID,1);
    }
  #endif
  vMPEGVDecEnd(u4InstID);
}

#define VDEC_BCODE_SRAM_ADDR      8192
void vMp4FixBCode(UINT32 u4InstID)
{
  UINT32 dMbx, dMby;
  UINT32 dPrd;
  UINT32 dIdx;
  #ifdef VDEC_SRAM
  UINT32 dTemp;
  #else
  UINT32 *pdPtr;
  #endif

  if(_u4PicCdTp[u4InstID] == P_TYPE)
  {
    dMbx = (_u4RealHSize[u4InstID] + 15) / 16;
    dMby = (_u4RealVSize[u4InstID] + 15) / 16;
    dPrd = dMbx * dMby;
    if((dPrd % 32) == 0)
    {
      dIdx = dPrd / 32;
      #ifdef VDEC_SRAM
      dTemp = VDEC_BCODE_SRAM_ADDR + dIdx;
      vWriteSram(u4InstID,(dTemp-1),u4ReadSram(u4InstID,dTemp));
//    printk("<vdec>u4ReadSram = 0x%x\n",u4ReadSram(u4InstID,(dTemp-1)));
      #else
      pdPtr = (UINT32 *)_pucMp4Bcode[u4InstID];
      pdPtr[dIdx - 1] = pdPtr[dIdx];
      #endif
      
    }
  }
}

BOOL fgIsDvDecComplete(UINT32 u4InstID)
{
/*
  //UINT32 u4MbX;
  //UINT32 u4MbY;  
  UINT32 dwTmp;
  
  dwTmp = dReadDV_8520(RO_DV_ST);
  //if(dwTmp && 0x1)
  if(dwTmp & 0x1)
  {
    dwDVDecodeDone_8520++;
  }
  //else if(dwTmp==0x2)
  else if(dwTmp & 0x2)
  {
    dwDVDecodeTimeOut_8520++;
  }
  else
  {
    dwDVDecodeOther_8520++;
  }

  if(_fgVDecComplete[u4InstID])
  {
    _fgVDecComplete[u4InstID] = FALSE;
    return TRUE;
  }
  return FALSE;
*/
    return TRUE;
}

#if VP8_MB_ROW_MODE_SUPPORT_ME2_INTEGRATION
extern void vVerVP8DecEndProc_MB_ROW_End(UINT32 u4InstID);

void vChkVDec_Webp_Row_Mode(UINT32 u4InstID)
{
    //vVerVP8DecEndProc(u4InstID);
    vVerVP8DecEndProc_MB_ROW_End(u4InstID);
    _tVerDec[u4InstID].ucState = DEC_NORM_VPARSER;//DEC_NORM_DEC_END;
    _u4PicCnt[u4InstID] ++;
}
#endif

// *********************************************************************
// Function    : void vChkVDec(UINT32 u4InstID)
// Description : Check if decoded complete & related settings
// Parameter   : None
// Return      : None
// *********************************************************************
void vChkVDec(UINT32 u4InstID)
{
  char strMessage[512];

  #ifdef VDEC_BREAK_EN
  if (!fgBreakVDec(u4InstID))
  {
    printk("VDEC Break Time Out\n");
  }
  #endif

  if (_u4CodecVer[u4InstID] == VDEC_RM)
  {
    vRM_VDecDecEndProc(u4InstID);
  }
  else if(_u4CodecVer[u4InstID] == VDEC_H264)
  //if(_u4CodecVer[u4InstID] == VDEC_H264)
  {
    vH264DecEndProc(u4InstID);
  }
  else if(_u4CodecVer[u4InstID] == VDEC_WMV)
  {
    if(_u4VprErr[u4InstID] == END_OF_FILE)
    {
      msleep(1000);
      printk("=====>end of file. \n");
      sprintf(strMessage," Compare Finish==> %s Pic count to [%d] \n",_bFileStr1[u4InstID][0], _u4FileCnt[u4InstID] - 1);   
      msleep(1000);
      strcpy(_tFileListRecInfo[u4InstID].bFileName,_FileList_Rec[u4InstID]);
      fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
      _u4VerBitCount[u4InstID] = 0xffffffff;
    }
    else if(_rWMVSPS[u4InstID].fgXintra8)
    {printk("=====>fgxintra8. \n");
      sprintf(strMessage," Compare Finish==> Pic count to [%d] \n", _u4FileCnt[u4InstID] - 1);   
      strcpy(_tFileListRecInfo[u4InstID].bFileName,_FileList_Rec[u4InstID]);
      fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
      _u4VerBitCount[u4InstID] = 0xffffffff;
    }
    else
    {
      vWMVDecEndProc(u4InstID);
    }
  }
 else if(_u4CodecVer[u4InstID] == VDEC_VP6)
 {
     vVerVP6DecEndProc(u4InstID);
    _tVerDec[u4InstID].ucState = DEC_NORM_VPARSER;//DEC_NORM_DEC_END;
 }
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
 else if(_u4CodecVer[u4InstID] == VDEC_VP8)
 {
     vVerVP8DecEndProc(u4InstID);
    _tVerDec[u4InstID].ucState = DEC_NORM_VPARSER;//DEC_NORM_DEC_END;
 }
#endif 
 else if(_u4CodecVer[u4InstID] == VDEC_AVS)
 {
     vVerAVSDecEndProc(u4InstID);
     _tVerDec[u4InstID].ucState = DEC_NORM_VPARSER;//DEC_NORM_DEC_END; 
  }
  else
  {
    vMPEGDecEndProc(u4InstID);
  }
#ifdef REDEC   
    if(_u4ReDecCnt[u4InstID] == 0)
#endif        
    {
        _u4PicCnt[u4InstID] ++;

    }
#ifdef REDEC   
    else
    {
      sprintf(strMessage,"[%d], ", _u4PicCnt[u4InstID]);  
      strcpy(_tFileListRecInfo[u4InstID].bFileName,_FileList_Rec[u4InstID]);
      fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
    }
#endif        
}


void vVerifyDx3SufxChk(UINT32 u4InstID)
{
  UINT32 dwByte, dwBit, dwShift, dwNextPicAddr;

  if(_ucVopCdTp[u4InstID] == VCT_I)
  {
    dwByte = u4VDEC_HAL_MPEG_ReadRdPtr(_u4BSID[u4InstID], u4InstID, (UINT32)_pucVFifo[u4InstID], &dwBit);
    dwBit += (dwByte * 8);
    if(_u4Divx3SetPos[u4InstID] >= V_FIFO_SZ)
    {
      _u4Divx3SetPos[u4InstID] = _u4Divx3SetPos[u4InstID] - V_FIFO_SZ;
    }
    dwNextPicAddr = _u4Divx3SetPos[u4InstID] * 8;

    if(dwNextPicAddr >= dwBit)
    {
      dwShift = dwNextPicAddr - dwBit;
    }
    else
    {
      dwShift = (dwNextPicAddr + (V_FIFO_SZ * 8)) - dwBit;
    }

    _fgVerSwitchRounding[u4InstID] = FALSE;
    if(dwShift >= 17)
    {
      _fgVerSwitchRounding[u4InstID] = (u4VDEC_HAL_MPEG_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, 0) >> 15) & 0x1;
    }
  }
}

void vSetDx3SliceBoundary(UINT32 u4InstID, VDEC_INFO_MPEG_DEC_PRM_T *prVDecMPEGDecPrm)
{
    UINT32 u4MbH = prVDecMPEGDecPrm->u4DecH >> 4;
    UINT32 u4SliceSize = 0;
    UINT32 i = 0;
    UINT32 j = 0;
    
    memset(prVDecMPEGDecPrm->rPicLayer.rMp4DecPrm.rDep.rDx3DecPrm.ucSliceBoundary, 0, 5*sizeof(UINT32));
    if (prVDecMPEGDecPrm->rPicLayer.rMp4DecPrm.rDep.rDx3DecPrm.ucFrameMode != 22)
    {
        u4SliceSize = u4MbH / (prVDecMPEGDecPrm->rPicLayer.rMp4DecPrm.rDep.rDx3DecPrm.ucFrameMode - 22);
        if (u4SliceSize != 0)
        {
            for (i=0; i<u4MbH; i++)
            {
                if (!(i%u4SliceSize))
                {
                    if (j < 5)
                    {
                        prVDecMPEGDecPrm->rPicLayer.rMp4DecPrm.rDep.rDx3DecPrm.ucSliceBoundary[j] = i;
                    }
                    j++;            
                }
            }
        }
    }    
}

// *********************************************************************
// Function    : void vVerifyFlushAllSetData(UINT32 u4InstID)
// Description : flush DPB info
// Parameter   None
// Return      : None
// *********************************************************************
void vVerifyFlushAllSetData(UINT32 u4InstID)
{
  UINT32 i;
  
  for(i=0; i<32; i++)
  {
    _rH264SPS[u4InstID][i].fgSPSValid = FALSE;    
  }
  for(i=0; i<256; i++)
  {
    _rH264PPS[u4InstID][i].fgPPSValid = FALSE;    
  }
}

  
// *********************************************************************
// Function    : void vVerifyFlushBufInfo(UINT32 u4InstID)
// Description : flush DPB info
// Parameter   None
// Return      : None
// *********************************************************************
void vVerifyFlushBufInfo(UINT32 u4InstID)
{
  UINT32 i;
  
  _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecH264DecPrm.u4MaxLongTermFrameIdx = 0xffffffff;
  
  for(i=0; i<17; i++)
  {
    _ptFBufInfo[u4InstID][i].u4DecOrder = 0;
    vVerifyClrFBufInfo(u4InstID, i);    
  }
  for(i=0; i<6; i++)
  {        
    _ptRefPicList[u4InstID][i].u4RefPicCnt = 0;
  }
}
























  
// *********************************************************************
// Function    : void vH264VDecEnd(UINT32 u4InstID)
// Description : VDec complete related setting
// Parameter   : None
// Return      : None
// *********************************************************************
void vH264VDecEnd(UINT32 u4InstID)
{
    msleep(5);
  VDEC_INFO_DEC_PRM_T *tVerMpvDecPrm;
  VDEC_INFO_H264_FBUF_INFO_T *tFBufInfo;
#if defined(SW_RESET) || defined(REDEC)
  VDEC_INFO_H264_INIT_PRM_T rH264VDecInitPrm;
  VDEC_INFO_H264_BS_INIT_PRM_T rH264BSInitPrm;
#endif
#ifdef SW_RESET
  UINT32 u4Bits;
#endif
  UINT32 u4RegVal;
  INT32 i;

#if VDEC_VER_COMPARE_CRC
#ifndef LETTERBOX_DETECTION_ONLY  
  BOOL fgCRCPass = FALSE;
#endif
#endif

  tFBufInfo = _ptCurrFBufInfo[u4InstID];
  tVerMpvDecPrm = &_tVerMpvDecPrm[u4InstID];

  
#ifdef LETTERBOX_DETECTION_ONLY  
  vCheckLBDResult(u4InstID);
#else
#if VDEC_VER_COMPARE_CRC
  fgCRCPass = vH264_CheckCRCResult(u4InstID);

  //printk("[H264] vH264VDecEnd, after CRC check \n");

   if (fgCRCPass == FALSE)
#endif   	
  {
    vH264WrData2PC(u4InstID, _pucDumpYBuf[u4InstID], tFBufInfo->u4DramPicSize);
    printk("[H264] @@ do golden compare InstID %d, DramPicSize %d\n", u4InstID, tFBufInfo->u4DramPicSize);
    DBG_H264_PRINTF("[H264] @@ do golden compare InstID %d, DramPicSize %d\n", u4InstID, tFBufInfo->u4DramPicSize);
  }
/*
    u4RegVal = u4VDecReadVLD(u4InstID, 161*4);
    printk("[H264] golden mismatch, VLD_161 = 0x%08x\n", u4RegVal);
    DBG_H264_PRINTF("[H264] golden mismatch, VLD_161 = 0x%08x\n", u4RegVal);

    u4RegVal = u4VDecReadVLD(u4InstID, 251*4);
    printk("[H264] golden mismatch, VLD_251 = 0x%08x\n", u4RegVal);
    DBG_H264_PRINTF("[H264] golden mismatch, VLD_251 = 0x%08x\n", u4RegVal);

    for (i=378; i<398; i++)
    {
        u4RegVal = u4VDecReadMC(u4InstID, i*4);
        printk("[H264] golden mismatch, MC_%d = 0x%08x\n", i, u4RegVal);
        DBG_H264_PRINTF("[H264] golden mismatch, MC_%d = 0x%08x\n", i, u4RegVal);
    }
    for (i=470; i<473; i++)
    {
        u4RegVal = u4VDecReadMC(u4InstID, i*4);
        printk("[H264] golden mismatch, MC_%d = 0x%08x\n", i, u4RegVal);
        DBG_H264_PRINTF("[H264] golden mismatch, MC_%d = 0x%08x\n", i, u4RegVal);
    }
    for (i=479; i<486; i++)
    {
        u4RegVal = u4VDecReadMC(u4InstID, i*4);
        printk("[H264] golden mismatch, MC_%d = 0x%08x\n", i, u4RegVal);
        DBG_H264_PRINTF("[H264] golden mismatch, MC_%d = 0x%08x\n", i, u4RegVal);
    }
    for (i=527; i<529; i++)
    {
        u4RegVal = u4VDecReadMC(u4InstID, i*4);
        printk("[H264] golden mismatch, MC_%d = 0x%08x\n", i, u4RegVal);
        DBG_H264_PRINTF("[H264] golden mismatch, MC_%d = 0x%08x\n", i, u4RegVal);
    }
    for (i=147; i<153; i++)
    {
        u4RegVal = u4VDecReadAVCMV(u4InstID, i*4);
        printk("[H264] golden mismatch, MV_%d = 0x%08x\n", i, u4RegVal);
        DBG_H264_PRINTF("[H264] golden mismatch, MV_%d = 0x%08x\n", i, u4RegVal);
    }    
    for (i=41; i<76; i++)
    {
        u4RegVal = u4VDecReadVLDTOP(u4InstID, i*4);
        printk("[H264] golden mismatch, VLD_TOP_%d = 0x%08x\n", i, u4RegVal);
        DBG_H264_PRINTF("[H264] golden mismatch, VLD_TOP_%d = 0x%08x\n", i, u4RegVal);
    }
    for (i=64; i<81; i++)
    {
        u4RegVal = u4VDecReadPP(u4InstID, i*4);
        printk("[H264] golden mismatch, PP_%d = 0x%08x\n", i, u4RegVal);
        DBG_H264_PRINTF("[H264] golden mismatch, PP_%d = 0x%08x\n", i, u4RegVal);
    }
*/


#endif  

#ifdef REDEC   
    if(_u4ReDecCnt[u4InstID] > 0)
    {
    #ifdef BARREL2_THREAD_SUPPORT
      VERIFY (x_sema_lock(_ahVDecEndSema[u4InstID], X_SEMA_OPTION_WAIT) == OSR_OK);
    #endif
      _u4FileOffset[u4InstID] =  _u4VLDPosByte[u4InstID];
      rH264VDecInitPrm.u4FGDatabase = (UINT32)_pucFGDatabase[u4InstID];
      rH264VDecInitPrm.u4CompModelValue = (UINT32)(_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecH264DecPrm.prSEI->pucCompModelValue);
      rH264VDecInitPrm.u4FGSeedbase = (UINT32)_pucFGSeedbase[u4InstID];
      i4VDEC_HAL_H264_InitVDecHW(u4InstID,&rH264VDecInitPrm);
      rH264BSInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
      rH264BSInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
      rH264BSInitPrm.u4VLDRdPtr = (UINT32)_pucVFifo[u4InstID] + _u4FileOffset[u4InstID];
    #ifndef  RING_VFIFO_SUPPORT
      rH264BSInitPrm.u4VLDWrPtr = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
    #else
	//	rH264BSInitPrm.u4VLDWrPtr = (UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ*(0.5 + 0.5 *(_u4LoadBitstreamCnt[u4InstID]%2)));
		rH264BSInitPrm.u4VLDWrPtr = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));
    #endif
      rH264BSInitPrm.u4PredSa = /*PHYSICAL*/((UINT32)_pucPredSa[u4InstID]);
      i4VDEC_HAL_H264_InitBarrelShifter(0, u4InstID, &rH264BSInitPrm);
      u4VDEC_HAL_H264_ShiftGetBitStream(0, u4InstID, _u4VLDPosBit[u4InstID]);
    #ifdef BARREL2_THREAD_SUPPORT
      VERIFY (x_sema_unlock(_ahVDecEndSema[u4InstID]) == OSR_OK);
    #endif
      _u4VLDPosByte[u4InstID] = u4VDEC_HAL_H264_ReadRdPtr(0, u4InstID, (UINT32)_pucVFifo[u4InstID], &_u4VLDPosBit[u4InstID]);
      _tVerDec[u4InstID].ucState = DEC_NORM_WAIT_TO_DEC;
      return;
    }
#endif

  tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.rLastInfo.u4LastPicW = tFBufInfo->u4W;
  tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.rLastInfo.u4LastPicH = tFBufInfo->u4H;  
  printk("[H264] @@ Last Pic W %d, H %d\n", tFBufInfo->u4W, tFBufInfo->u4H);
  DBG_H264_PRINTF("[H264] @@ Last Pic W %d, H %d\n", tFBufInfo->u4W, tFBufInfo->u4H);
  //Marking procedure
  if(fgIsRefPic(u4InstID))
  {
    if(fgIsIDRPic(u4InstID)) // IDR pic
    {
      if(tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->fgLongTermReferenceFlag)
      {
        vVerifySetPicRefType(u4InstID, tVerMpvDecPrm->ucPicStruct, LREF_PIC);
        tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.u4MaxLongTermFrameIdx = 0;
        tFBufInfo->u4LongTermFrameIdx = 0;
        tFBufInfo->u4TFldLongTermFrameIdx = 0;
        tFBufInfo->u4BFldLongTermFrameIdx = 0;
      }
      else
      {
        tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.u4MaxLongTermFrameIdx = 0xffffffff;        
        vVerifySetPicRefType(u4InstID, tVerMpvDecPrm->ucPicStruct, SREF_PIC);
      }
    }
    else // !IDR pic
    {      
      if(tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->fgAdaptiveRefPicMarkingModeFlag)
      {
        vVerifyAdapRefPicmarkingProce(u4InstID);
      }
      else
      {              
        vVerifySlidingWindowProce(u4InstID);
      }
      if(bGetPicRefType(u4InstID, tVerMpvDecPrm->ucPicStruct) != LREF_PIC)
      {
        vVerifySetPicRefType(u4InstID, tVerMpvDecPrm->ucPicStruct, SREF_PIC);  
      }      
    }
  }
  
  tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.rLastInfo.fgLastMmco5 = tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->fgMmco5;  
  tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.rLastInfo.ucLastPicStruct = tVerMpvDecPrm->ucPicStruct;  
  if(tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.rLastInfo.fgLastMmco5)
  {
    tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.rLastInfo.u4LastFrameNum = 0;
    tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.rLastInfo.i4LastFrameNumOffset = 0;
  }
  else
  {
    tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.rLastInfo.u4LastFrameNum = tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4FrameNum;
    tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.rLastInfo.i4LastFrameNumOffset = tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.i4FrmNumOffset;
  }
  tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.rLastInfo.i4LastPOC =  tFBufInfo->i4POC;      
  tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.rLastInfo.i4LastTFldPOC = tFBufInfo->i4TFldPOC;
  tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.rLastInfo.i4LastBFldPOC = tFBufInfo->i4BFldPOC;  
      
  if(fgIsRefPic(u4InstID))
  {
    tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.rLastInfo.i4LastRefPOC = tFBufInfo->i4POC;  
//    tVerMpvDecPrm->rLastInfo.iLastRefPOCCntLsb = tVerMpvDecPrm->prSliceHdr->i4PicOrderCntLsb;  
//    tVerMpvDecPrm->rLastInfo.iLastRefPOCCntMsb = tVerMpvDecPrm->prSliceHdr->i4PicOrderCntMsb;  
    tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.rLastInfo.i4LastRefPOCLsb =  tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->i4PicOrderCntLsb;  
    tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.rLastInfo.i4LastRefPOCMsb = tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->i4PicOrderCntMsb;  
  }

#if VDEC_MVC_SUPPORT  
  if(_ucMVCType[u4InstID] != 0) 
  {     
       tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.rLastInfo.ucLastDpbId = tVerMpvDecPrm->ucDecFBufIdx;      
       tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.rLastInfo.u4LastViewId =  tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.rMvcExtInfo.u4ViewId;      
       memcpy(&_rH264PrevFbInfo[u4InstID], _ptCurrFBufInfo[u4InstID], sizeof(VDEC_INFO_H264_FBUF_INFO_T));  
       }
#endif

  if(fgIsDecFlagSet(u4InstID, DEC_FLAG_CHG_FBUF))
  {
    _ptCurrFBufInfo[u4InstID]->eH264DpbStatus = H264_DPB_STATUS_DECODED;
    _ptCurrFBufInfo[u4InstID]->u4DecOrder = _u4TotalDecFrms[u4InstID];
    vChkOutputFBuf(u4InstID);
    _u4TotalDecFrms[u4InstID] ++;
  }
  else
  {
    _ptCurrFBufInfo[u4InstID]->eH264DpbStatus = H264_DPB_STATUS_FLD_DECODED;
  }

  vAdd2RefPicList(u4InstID);
#ifdef SW_RESET
#ifdef BARREL2_THREAD_SUPPORT
  VERIFY (x_sema_lock(_ahVDecEndSema[u4InstID], X_SEMA_OPTION_WAIT) == OSR_OK);
#endif
  _u4FileOffset[u4InstID] = u4VDEC_HAL_H264_ReadRdPtr(0, u4InstID, (UINT32)_pucVFifo[u4InstID], &u4Bits);
  rH264VDecInitPrm.u4FGDatabase = (UINT32)_pucFGDatabase[u4InstID];
  rH264VDecInitPrm.u4CompModelValue = (UINT32)(_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecH264DecPrm.prSEI->pucCompModelValue);
  rH264VDecInitPrm.u4FGSeedbase = (UINT32)_pucFGSeedbase[u4InstID];
  i4VDEC_HAL_H264_InitVDecHW(u4InstID,&rH264VDecInitPrm);
  rH264BSInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
  rH264BSInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
  rH264BSInitPrm.u4VLDRdPtr = (UINT32)_pucVFifo[u4InstID] + _u4FileOffset[u4InstID];
   if (_ucMVCType[u4InstID] > 0)
   {
      if (u4InstID == 0)
          rH264BSInitPrm.u4VLDRdPtr = (UINT32)_pucVFifo[u4InstID] + _u4FileOffset[1];
      else if (u4InstID == 1)
         rH264BSInitPrm.u4VLDRdPtr = (UINT32)_pucVFifo[u4InstID] + _u4FileOffset[0];
   }


#ifndef  RING_VFIFO_SUPPORT
  rH264BSInitPrm.u4VLDWrPtr = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
#else
  //  rH264BSInitPrm.u4VLDWrPtr = (UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ*(0.5 + 0.5 *(_u4LoadBitstreamCnt[u4InstID]%2)));
	  rH264BSInitPrm.u4VLDWrPtr = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));
#endif
  rH264BSInitPrm.u4PredSa = /*PHYSICAL*/((UINT32)_pucPredSa[u4InstID]);
  i4VDEC_HAL_H264_InitBarrelShifter(_u4BSID[u4InstID], u4InstID, &rH264BSInitPrm);
  u4VDEC_HAL_H264_ShiftGetBitStream(_u4BSID[u4InstID], u4InstID, u4Bits);
#ifdef BARREL2_THREAD_SUPPORT
  VERIFY (x_sema_unlock(_ahVDecEndSema[u4InstID]) == OSR_OK);
#endif
#endif
#if 0
  if(dwGetBitStream(0) == 0x0000010B)
    {
        vFlushDPB(tVerMpvDecPrm, TRUE);
    }
#endif
#ifndef INTERGRATION_WITH_DEMUX
#ifdef  RING_VFIFO_SUPPORT
  if((_u4LoadBitstreamCnt[u4InstID]&0x1) && (rH264BSInitPrm.u4VLDRdPtr > 
  ((UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ/2))))
  {
    _tInFileInfo[u4InstID].fgGetFileInfo = TRUE;
    _tInFileInfo[u4InstID].pucTargetAddr = _pucVFifo[u4InstID];
    _tInFileInfo[u4InstID].u4FileOffset = (V_FIFO_SZ * ((_u4LoadBitstreamCnt[u4InstID]+ 1)/2));
    _tInFileInfo[u4InstID].u4TargetSz = (V_FIFO_SZ/2);    
    _tInFileInfo[u4InstID].u4FileLength = 0; 
  #ifdef  SATA_HDD_READ_SUPPORT
    if(!fgOpenHDDFile(u4InstID, _bFileStr1[u4InstID][1],"r+b", &_tInFileInfo[u4InstID]))
    {
      fgOpenPCFile(u4InstID, _bFileStr1[u4InstID][1],"r+b", &_tInFileInfo[u4InstID]);
    }
  #elif defined(IDE_READ_SUPPORT)
       fgOpenIdeFile(_bFileStr1[u4InstID][1],"r+b", &_tInFileInfo[u4InstID]);
  #else
    fgOpenPCFile(u4InstID, _bFileStr1[u4InstID][1],"r+b", &_tInFileInfo[u4InstID]);
  #endif  
    _u4LoadBitstreamCnt[u4InstID]++;
  }
  else if((!(_u4LoadBitstreamCnt[u4InstID]&0x1)) && (rH264BSInitPrm.u4VLDRdPtr < 
  ((UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ/2))))
  {
    _tInFileInfo[u4InstID].fgGetFileInfo = TRUE;
    _tInFileInfo[u4InstID].pucTargetAddr = _pucVFifo[u4InstID] + (V_FIFO_SZ/2);
    _tInFileInfo[u4InstID].u4FileOffset = ((V_FIFO_SZ * (_u4LoadBitstreamCnt[u4InstID]+ 1)) /2);//(V_FIFO_SZ * ((_u4LoadBitstreamCnt[u4InstID]+ 1)/2));
    _tInFileInfo[u4InstID].u4TargetSz = (V_FIFO_SZ/2);    
    _tInFileInfo[u4InstID].u4FileLength = 0; 
  #ifdef  SATA_HDD_READ_SUPPORT
    if(!fgOpenHDDFile(u4InstID, _bFileStr1[u4InstID][1],"r+b", &_tInFileInfo[u4InstID]))
    {
      fgOpenPCFile(u4InstID, _bFileStr1[u4InstID][1],"r+b", &_tInFileInfo[u4InstID]);
    }
  #elif defined(IDE_READ_SUPPORT)
       fgOpenIdeFile(_bFileStr1[u4InstID][1],"r+b", &_tInFileInfo[u4InstID]);
  #else
    fgOpenPCFile(u4InstID, _bFileStr1[u4InstID][1],"r+b", &_tInFileInfo[u4InstID]);
  #endif  
    _u4LoadBitstreamCnt[u4InstID]++;
  }
  if((0 ==_tInFileInfo[u4InstID].u4RealGetBytes) ||
  	(V_FIFO_SZ/2 != _tInFileInfo[u4InstID].u4RealGetBytes) )
	{
		//vAddStartCode2Dram(_pucVFifo+_tInFileInfo.u4FileLength);
		UCHAR *pbDramAddr = _tInFileInfo[u4InstID].pucTargetAddr+_tInFileInfo[u4InstID].u4RealGetBytes;

		pbDramAddr[0] = 0x00; pbDramAddr++;
		if((UINT32)(_pucVFifo[u4InstID] + V_FIFO_SZ) <= (UINT32)pbDramAddr)	
		{
			pbDramAddr = _pucVFifo[u4InstID];
		}
		pbDramAddr[0] = 0x00; pbDramAddr++;
		if((UINT32)(_pucVFifo[u4InstID] + V_FIFO_SZ) <= (UINT32)pbDramAddr)	
		{
			pbDramAddr = _pucVFifo[u4InstID];
		}
		pbDramAddr[0] = 0x01;  
  	}
  #endif
  #endif
  _tVerDec[u4InstID].ucState = DEC_NORM_VPARSER;

  if((u4InstID == 0) && _ucMVCType[0])  
  {    
      _fgMVCReady[0] = FALSE;
      _fgMVCReady[1] = TRUE;
	  #if 0
      if(_u4VerBitCount[u4InstID] == 0xffffffff)	
      {		
          while((_fgMVCReady[0] == FALSE) || (_fgMVCReady[1] == TRUE))
          {	
            udelay(5);
          }

          udelay(10);
      } 
	  #else
	  if(_u4VerBitCount[u4InstID] == 0xffffffff)	
      {		
          while((_fgMVCReady[0] == FALSE) || (_fgMVCReady[1] == TRUE))
          {	
            msleep(5);
          }

          msleep(10);
      } 
	  #endif
  }  
  
  if((u4InstID == 1) && _ucMVCType[1] && (_u4VerBitCount[1] != 0xffffffff)) 
  {    
      _fgMVCReady[0] = TRUE;    
      _fgMVCReady[1] = FALSE;	  
  }
  
}

// *********************************************************************
// Function    : void vVerifySetPicRefType(UINT32 u4InstID, UCHAR ucPicStruct, ucPicStruct ucRefType)
// Description : set pic ref type
// Parameter   : UCHAR ucPicType: pic struct : FRAME, TOP_FIELD, BOTTOM_FIELD
//                     UCHAR ucRefType: pic ref type: NREF_PIC, SREF_PIC, LREF_PIC
// Return      : None
// *********************************************************************
void vVerifySetPicRefType(UINT32 u4InstID, UCHAR ucPicStruct, UCHAR ucRefType)
{
  if(ucPicStruct & TOP_FIELD)
  {
    _ptCurrFBufInfo[u4InstID]->ucTFldRefType = ucRefType;
  }
  if(ucPicStruct & BOTTOM_FIELD)
  {
    _ptCurrFBufInfo[u4InstID]->ucBFldRefType = ucRefType;      
  }
  //if(ucPicStruct == FRAME)
  {
    _ptCurrFBufInfo[u4InstID]->ucFBufRefType = ucRefType;
  }
  //else
  {
    //_ptCurrFBufInfo->ucFBufRefType = NREF_PIC;
  }
}

// *********************************************************************
// Function    : void vVerifyAdapRefPicmarkingProce(UINT32 u4InstID)
// Description : marking the decoded ref pic with adaptive method
// Parameter   : None
// Return      : None
// *********************************************************************
void vVerifyAdapRefPicmarkingProce(UINT32 u4InstID)
{
  VDEC_INFO_DEC_PRM_T *tVerMpvDecPrm;
  UINT32 u4PicNumX;
  UINT32 u4Cnt;
  INT32 i;
  
  tVerMpvDecPrm = &_tVerMpvDecPrm[u4InstID];
  u4Cnt = 0;
  while(tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4MemoryManagementControlOperation[u4Cnt] != 0)
  {
    switch(tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4MemoryManagementControlOperation[u4Cnt]&0xff)
    {
      case 0:      
        break;
      case 1:
        // picNumX
        tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4DifferencOfPicNumsMinus1 = tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4MemoryManagementControlOperation[u4Cnt] >> 8;
        //if(fgIsFrmPic(_u4VDecID))
        {
          u4PicNumX = _ptCurrFBufInfo[u4InstID]->i4PicNum - tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4DifferencOfPicNumsMinus1 - 1;
        }
#if 0        
        else if(tVerMpvDecPrm->ucPicStruct & TOP_FIELD)
        {
          u4PicNumX = _ptCurrFBufInfo[u4InstID]->i4TFldPicNum - tVerMpvDecPrm->prSliceHdr->u4DifferencOfPicNumsMinus1 - 1;
        }
        else if(tVerMpvDecPrm->ucPicStruct & BOTTOM_FIELD)
        {
          u4PicNumX = _ptCurrFBufInfo[u4InstID]->i4BFldPicNum - tVerMpvDecPrm->prSliceHdr->u4DifferencOfPicNumsMinus1 - 1;
        }
#endif        
        for(i=0; i < tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.ucMaxFBufNum; i++)
        {
          if(fgIsFrmPic(u4InstID) && (_ptFBufInfo[u4InstID][i].i4PicNum == u4PicNumX) && (_ptFBufInfo[u4InstID][i].ucFBufRefType == SREF_PIC))
          {
            vVerifyClrPicRefInfo(u4InstID, FRAME, i);            
            i = tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.ucMaxFBufNum; // break
          }
          else if((!fgIsFrmPic(u4InstID)) && 
                     (((_ptFBufInfo[u4InstID][i].i4TFldPicNum == u4PicNumX) && (_ptFBufInfo[u4InstID][i].ucTFldRefType == SREF_PIC))
                     || ((_ptFBufInfo[u4InstID][i].i4BFldPicNum == u4PicNumX) && (_ptFBufInfo[u4InstID][i].ucBFldRefType == SREF_PIC))))
          {
            if((_ptFBufInfo[u4InstID][i].i4TFldPicNum == u4PicNumX) && (_ptFBufInfo[u4InstID][i].ucTFldRefType == SREF_PIC))
            {
              vVerifyClrPicRefInfo(u4InstID, TOP_FIELD, i);            
            }
            if((_ptFBufInfo[u4InstID][i].i4BFldPicNum == u4PicNumX) && (_ptFBufInfo[u4InstID][i].ucBFldRefType == SREF_PIC))
            {
              vVerifyClrPicRefInfo(u4InstID, BOTTOM_FIELD, i);            
            }
            i = tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.ucMaxFBufNum; // break
          }
        }
        break;
      case 2:
        tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4LongTermPicNum = tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4MemoryManagementControlOperation[u4Cnt] >> 8;
        u4PicNumX = tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4LongTermPicNum;
        for(i=0; i < tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.ucMaxFBufNum; i++)
        {
          if(fgIsFrmPic(u4InstID) && (_ptFBufInfo[u4InstID][i].i4LongTermPicNum == u4PicNumX) && (_ptFBufInfo[u4InstID][i].ucFBufRefType == LREF_PIC))
          {
            vVerifyClrPicRefInfo(u4InstID, FRAME, i);            
            i = tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.ucMaxFBufNum; // break
          }
          else if((!fgIsFrmPic(u4InstID)) && 
                     (((_ptFBufInfo[u4InstID][i].i4TFldLongTermPicNum == u4PicNumX) && (_ptFBufInfo[u4InstID][i].ucTFldRefType == LREF_PIC))
                     || ((_ptFBufInfo[u4InstID][i].i4BFldLongTermPicNum == u4PicNumX) && (_ptFBufInfo[u4InstID][i].ucBFldRefType == LREF_PIC))))
          {
            if((_ptFBufInfo[u4InstID][i].i4TFldLongTermPicNum == u4PicNumX) && (_ptFBufInfo[u4InstID][i].ucTFldRefType == LREF_PIC))
            {
              vVerifyClrPicRefInfo(u4InstID, TOP_FIELD, i);            
            }
            if((_ptFBufInfo[u4InstID][i].i4BFldLongTermPicNum == u4PicNumX) && (_ptFBufInfo[u4InstID][i].ucBFldRefType == LREF_PIC))
            {
              vVerifyClrPicRefInfo(u4InstID, BOTTOM_FIELD, i);            
            }
            i = tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.ucMaxFBufNum; // break
          }
        }
        break;
      case 3:
        tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4DifferencOfPicNumsMinus1 = (tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4MemoryManagementControlOperation[u4Cnt] >> 8) & 0xff;     
        tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4LongTermFrameIdx = tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4MemoryManagementControlOperation[u4Cnt] >> 16;
        u4PicNumX = tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4LongTermFrameIdx;
        for(i=0; i < tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.ucMaxFBufNum; i++)
        {
          if(i != tVerMpvDecPrm->ucDecFBufIdx)
          {
            if((_ptFBufInfo[u4InstID][i].ucFBufStatus == FRAME) && (_ptFBufInfo[u4InstID][i].u4LongTermFrameIdx == u4PicNumX) && (_ptFBufInfo[u4InstID][i].ucFBufRefType == LREF_PIC))
            {
              vVerifyClrPicRefInfo(u4InstID, FRAME, i);            
              i = tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.ucMaxFBufNum; // break
            }
            else if((_ptFBufInfo[u4InstID][i].ucFBufStatus != FRAME) && 
                       (((_ptFBufInfo[u4InstID][i].u4TFldLongTermFrameIdx == u4PicNumX) && (_ptFBufInfo[u4InstID][i].ucTFldRefType == LREF_PIC))
                       || ((_ptFBufInfo[u4InstID][i].u4BFldLongTermFrameIdx == u4PicNumX) && (_ptFBufInfo[u4InstID][i].ucBFldRefType == LREF_PIC))))
            {
              if((_ptFBufInfo[u4InstID][i].u4TFldLongTermFrameIdx == u4PicNumX) && (_ptFBufInfo[u4InstID][i].ucTFldRefType == LREF_PIC))
              {
                vVerifyClrPicRefInfo(u4InstID, TOP_FIELD, i);            
              }
              if((_ptFBufInfo[u4InstID][i].u4BFldLongTermFrameIdx == u4PicNumX) && (_ptFBufInfo[u4InstID][i].ucBFldRefType == LREF_PIC))
              {
                vVerifyClrPicRefInfo(u4InstID, BOTTOM_FIELD, i);            
              }
              i = tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.ucMaxFBufNum; // break
            }
          }
        }
        
        // picNumX
        //if(fgIsFrmPic(_u4VDecID))
        {
          u4PicNumX = _ptCurrFBufInfo[u4InstID]->i4PicNum - tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4DifferencOfPicNumsMinus1 - 1;
        }
#if 0        
        else if(tVerMpvDecPrm->ucPicStruct & TOP_FIELD)
        {
          u4PicNumX = _ptCurrFBufInfo[u4InstID]->i4TFldPicNum - tVerMpvDecPrm->prSliceHdr->u4DifferencOfPicNumsMinus1 - 1;
        }
        else if(tVerMpvDecPrm->ucPicStruct & BOTTOM_FIELD)
        {
          u4PicNumX = _ptCurrFBufInfo[u4InstID]->i4BFldPicNum - tVerMpvDecPrm->prSliceHdr->u4DifferencOfPicNumsMinus1 - 1;
        }
#endif        
        for(i=0; i < tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.ucMaxFBufNum; i++)
        {
          if(fgIsFrmPic(u4InstID) && (_ptFBufInfo[u4InstID][i].i4PicNum == u4PicNumX) 
              && (_ptFBufInfo[u4InstID][i].ucFBufRefType == SREF_PIC) && (!_ptFBufInfo[u4InstID][i].fgNonExisting))
          {
            _ptFBufInfo[u4InstID][i].ucFBufRefType = LREF_PIC;
            _ptFBufInfo[u4InstID][i].ucTFldRefType = LREF_PIC;
            _ptFBufInfo[u4InstID][i].ucBFldRefType = LREF_PIC;          
            _ptFBufInfo[u4InstID][i].u4LongTermFrameIdx = tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4LongTermFrameIdx;            
            _ptFBufInfo[u4InstID][i].u4TFldLongTermFrameIdx = tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4LongTermFrameIdx;            
            _ptFBufInfo[u4InstID][i].u4BFldLongTermFrameIdx = tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4LongTermFrameIdx;            
            _ptFBufInfo[u4InstID][i].i4LongTermPicNum = tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4LongTermFrameIdx;            
            _ptFBufInfo[u4InstID][i].i4TFldLongTermPicNum = tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4LongTermFrameIdx;            
            _ptFBufInfo[u4InstID][i].i4BFldLongTermPicNum = tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4LongTermFrameIdx;            
            i = tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.ucMaxFBufNum; // break
          }
          else if((!fgIsFrmPic(u4InstID)) && 
                     (((_ptFBufInfo[u4InstID][i].i4TFldPicNum == u4PicNumX) && (_ptFBufInfo[u4InstID][i].ucTFldRefType == SREF_PIC)  && (!_ptFBufInfo[u4InstID][i].fgNonExisting))
                     || ((_ptFBufInfo[u4InstID][i].i4BFldPicNum == u4PicNumX) && (_ptFBufInfo[u4InstID][i].ucBFldRefType == SREF_PIC)  && (!_ptFBufInfo[u4InstID][i].fgNonExisting))))
          {
            if((_ptFBufInfo[u4InstID][i].i4TFldPicNum == u4PicNumX) && (_ptFBufInfo[u4InstID][i].ucTFldRefType == SREF_PIC)  && (!_ptFBufInfo[u4InstID][i].fgNonExisting))
            {
              _ptFBufInfo[u4InstID][i].ucTFldRefType = LREF_PIC;
              _ptFBufInfo[u4InstID][i].u4TFldLongTermFrameIdx = tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4LongTermFrameIdx;                      
              _ptFBufInfo[u4InstID][i].i4TFldLongTermPicNum = (tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4LongTermFrameIdx<<1) + ((_tVerMpvDecPrm[u4InstID].ucPicStruct == TOP_FIELD)? 1: 0);
              if(_ptFBufInfo[u4InstID][i].ucBFldRefType == LREF_PIC)
              {
                _ptFBufInfo[u4InstID][i].ucFBufRefType = LREF_PIC;
                _ptFBufInfo[u4InstID][i].u4LongTermFrameIdx  = tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4LongTermFrameIdx;    
                _ptFBufInfo[u4InstID][i].i4LongTermPicNum = _ptFBufInfo[u4InstID][i].u4LongTermFrameIdx;
              }
            }
            if((_ptFBufInfo[u4InstID][i].i4BFldPicNum == u4PicNumX) && (_ptFBufInfo[u4InstID][i].ucBFldRefType == SREF_PIC)  && (!_ptFBufInfo[u4InstID][i].fgNonExisting))
            {
              _ptFBufInfo[u4InstID][i].ucBFldRefType = LREF_PIC;
              _ptFBufInfo[u4InstID][i].u4BFldLongTermFrameIdx = tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4LongTermFrameIdx;                      
              _ptFBufInfo[u4InstID][i].i4BFldLongTermPicNum = (tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4LongTermFrameIdx<<1) + ((_tVerMpvDecPrm[u4InstID].ucPicStruct == BOTTOM_FIELD)? 1: 0);
              if(_ptFBufInfo[u4InstID][i].ucTFldRefType == LREF_PIC)
              {
                _ptFBufInfo[u4InstID][i].ucFBufRefType = LREF_PIC;
                _ptFBufInfo[u4InstID][i].u4LongTermFrameIdx = tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4LongTermFrameIdx;    
                _ptFBufInfo[u4InstID][i].i4LongTermPicNum = _ptFBufInfo[u4InstID][i].u4LongTermFrameIdx;
              }
            }            
                              
            i = tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.ucMaxFBufNum; // break
          }          
        }
        break;      
      case 4:
        tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4MaxLongTermFrameIdxPlus1 = tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4MemoryManagementControlOperation[u4Cnt] >> 8;   
        if(tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4MaxLongTermFrameIdxPlus1 == 0)
        {
           _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecH264DecPrm.u4MaxLongTermFrameIdx = 0xffffffff;
           u4PicNumX = 0;
        }      
        else
        {
          u4PicNumX =(tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4MaxLongTermFrameIdxPlus1);
        }
        for(i=0; i < tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.ucMaxFBufNum; i++)
        {
          if((_ptFBufInfo[u4InstID][i].u4LongTermFrameIdx  >= u4PicNumX) && 
              ((_ptFBufInfo[u4InstID][i].ucTFldRefType == LREF_PIC) || (_ptFBufInfo[u4InstID][i].ucBFldRefType == LREF_PIC)))
          {
            _ptFBufInfo[u4InstID][i].ucTFldRefType = NREF_PIC;
            _ptFBufInfo[u4InstID][i].ucBFldRefType = NREF_PIC;
            _ptFBufInfo[u4InstID][i].ucFBufRefType = NREF_PIC;
          }
        }
        break;      
      case 5:
        _ptCurrFBufInfo[u4InstID]->u4FrameNum = 0;
        _ptCurrFBufInfo[u4InstID]->i4PicNum = 0;        
        _ptCurrFBufInfo[u4InstID]->i4TFldPicNum = 0;        
        _ptCurrFBufInfo[u4InstID]->i4BFldPicNum = 0;         
        if(tVerMpvDecPrm->ucPicStruct == TOP_FIELD)
        {
          _ptCurrFBufInfo[u4InstID]->i4TFldPOC = 0;                        
        }
        else if(tVerMpvDecPrm->ucPicStruct == BOTTOM_FIELD)
        {
          _ptCurrFBufInfo[u4InstID]->i4BFldPOC = 0;                        
        }
        else if(tVerMpvDecPrm->ucPicStruct == FRAME)
        {
          _ptCurrFBufInfo[u4InstID]->i4TFldPOC -= _ptCurrFBufInfo[u4InstID]->i4POC;                        
          _ptCurrFBufInfo[u4InstID]->i4BFldPOC -= _ptCurrFBufInfo[u4InstID]->i4POC;                        
          _ptCurrFBufInfo[u4InstID]->i4POC = (_ptCurrFBufInfo[u4InstID]->i4TFldPOC < _ptCurrFBufInfo[u4InstID]->i4BFldPOC)?
                                                       _ptCurrFBufInfo[u4InstID]->i4TFldPOC: _ptCurrFBufInfo[u4InstID]->i4BFldPOC;
        }
        
        vVerifyFlushBufRefInfo(u4InstID);
        break;      
      case 6:
        tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4LongTermFrameIdx = tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4MemoryManagementControlOperation[u4Cnt] >> 8;                
        u4PicNumX = tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4LongTermFrameIdx;
        for(i=0; i < tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.ucMaxFBufNum; i++)
        {
          if(i != tVerMpvDecPrm->ucDecFBufIdx)
          {
            if((_ptFBufInfo[u4InstID][i].ucFBufStatus == FRAME) && (_ptFBufInfo[u4InstID][i].u4LongTermFrameIdx == u4PicNumX) && (_ptFBufInfo[u4InstID][i].ucFBufRefType == LREF_PIC))
            {
              vVerifyClrPicRefInfo(u4InstID, FRAME, i);            
              i = tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.ucMaxFBufNum; // break
            }
            else if((_ptFBufInfo[u4InstID][i].ucFBufStatus != FRAME) && 
                       (((_ptFBufInfo[u4InstID][i].u4TFldLongTermFrameIdx == u4PicNumX) && (_ptFBufInfo[u4InstID][i].ucTFldRefType == LREF_PIC))
                       || ((_ptFBufInfo[u4InstID][i].u4BFldLongTermFrameIdx == u4PicNumX) && (_ptFBufInfo[u4InstID][i].ucBFldRefType == LREF_PIC))))
            {
              if((_ptFBufInfo[u4InstID][i].u4TFldLongTermFrameIdx == u4PicNumX) && (_ptFBufInfo[u4InstID][i].ucTFldRefType == LREF_PIC))
              {
                vVerifyClrPicRefInfo(u4InstID, TOP_FIELD, i);            
              }
              if((_ptFBufInfo[u4InstID][i].u4BFldLongTermFrameIdx == u4PicNumX) && (_ptFBufInfo[u4InstID][i].ucBFldRefType == LREF_PIC))
              {
                vVerifyClrPicRefInfo(u4InstID, BOTTOM_FIELD, i);            
              }
              i = tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.ucMaxFBufNum; // break
            }
          }
        }

        if(fgIsFrmPic(u4InstID)) // 2 flds decoded
        {
          _ptCurrFBufInfo[u4InstID]->ucFBufRefType = LREF_PIC;          
          _ptCurrFBufInfo[u4InstID]->ucTFldRefType = LREF_PIC;  
          _ptCurrFBufInfo[u4InstID]->ucBFldRefType = LREF_PIC;        
          _ptCurrFBufInfo[u4InstID]->u4LongTermFrameIdx = _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4LongTermFrameIdx;        
          _ptCurrFBufInfo[u4InstID]->u4TFldLongTermFrameIdx = tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4LongTermFrameIdx;            
          _ptCurrFBufInfo[u4InstID]->u4BFldLongTermFrameIdx = tVerMpvDecPrm->SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4LongTermFrameIdx;            
        }
        else if(tVerMpvDecPrm->ucPicStruct & TOP_FIELD) // 1 fld decoded
        {
          _ptCurrFBufInfo[u4InstID]->ucTFldRefType = LREF_PIC;
          _ptCurrFBufInfo[u4InstID]->u4LongTermFrameIdx = _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4LongTermFrameIdx;        
          _ptCurrFBufInfo[u4InstID]->u4TFldLongTermFrameIdx = _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4LongTermFrameIdx;          
          if(_ptCurrFBufInfo[u4InstID]->ucBFldRefType == LREF_PIC)
          {
            _ptCurrFBufInfo[u4InstID]->ucFBufRefType = LREF_PIC;
          }
        }
        else if(tVerMpvDecPrm->ucPicStruct & BOTTOM_FIELD) // 1 fld decoded
        {
          _ptCurrFBufInfo[u4InstID]->ucBFldRefType = LREF_PIC;
          _ptCurrFBufInfo[u4InstID]->u4LongTermFrameIdx = _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4LongTermFrameIdx;        
          _ptCurrFBufInfo[u4InstID]->u4BFldLongTermFrameIdx = _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecH264DecPrm.prSliceHdr->u4LongTermFrameIdx;
          if(_ptCurrFBufInfo[u4InstID]->ucTFldRefType == LREF_PIC)
          {
            _ptCurrFBufInfo[u4InstID]->ucFBufRefType = LREF_PIC;
          }
        }
        break;      
    }
    u4Cnt ++;
  }
}

// *********************************************************************
// Function    : UCHAR bGetPicRefType(UINT32 u4InstID, UCHAR ucPicStruct)
// Description : get pic ref type
// Parameter   : UCHAR ucPicType: pic struct : FRAME, TOP_FIELD, BOTTOM_FIELD
// Return      : UCHAR ucRefType: pic ref type: NREF_PIC, SREF_PIC, LREF_PIC
// *********************************************************************
UCHAR bGetPicRefType(UINT32 u4InstID, UCHAR ucPicStruct)
{
  if(ucPicStruct == TOP_FIELD)
  {
    return _ptCurrFBufInfo[u4InstID]->ucTFldRefType;
  }
  else if(ucPicStruct == BOTTOM_FIELD)
  {
    return _ptCurrFBufInfo[u4InstID]->ucBFldRefType;      
  }
  else//if(ucPicStruct == FRAME)
  {
    return _ptCurrFBufInfo[u4InstID]->ucFBufRefType;
  }
}





// *********************************************************************
// Function    : void vChkOutputFBuf(UINT32 u4InstID)
// Description : Output 1 frm buff in DPB when DPB full
// Parameter   : 
// Return      : None
// *********************************************************************
void vChkOutputFBuf(UINT32 u4InstID)
{
  UINT32 u4MinPOCFBufIdx;
  VDEC_INFO_DEC_PRM_T *tVerMpvDecPrm;

  tVerMpvDecPrm = &_tVerMpvDecPrm[u4InstID];

    // needs to output
    do
    {
        u4MinPOCFBufIdx = ucVDecGetMinPOCFBuf(u4InstID, tVerMpvDecPrm, TRUE);
    
        if((u4MinPOCFBufIdx != 0xFF)
            && (_ptFBufInfo[u4InstID][u4MinPOCFBufIdx].eH264DpbStatus != H264_DPB_STATUS_EMPTY))
        {
            _ptFBufInfo[u4InstID][u4MinPOCFBufIdx].eH264DpbStatus = H264_DPB_STATUS_OUTPUTTED;
            vOutputPOCData(_ptFBufInfo[u4InstID][u4MinPOCFBufIdx].u4DecOrder);
#if 0
            u4MinPOCFBufIdx = ucVDecGetMinPOCFBuf(u4InstID, tMpvDecPrm, TRUE);
            if((u4MinPOCFBufIdx != 0xff)
                && (_ptFBufInfo[u4MinPOCFBufIdx].eH264DpbStatus != H264_DPB_STATUS_EMPTY))
            {
                //prH264DrvInfo->ucH264DpbOutputFbId = u4MinPOCFBufIdx;
            }
            else
            {
                u4MinPOCFBufIdx = 0xFF;
            }
#endif            
            // check in next entry
        }
        else if((u4MinPOCFBufIdx != 0xFF)
            && (_ptFBufInfo[u4InstID][u4MinPOCFBufIdx].eH264DpbStatus == H264_DPB_STATUS_EMPTY))
        {
            u4MinPOCFBufIdx = 0xff;
        }
        
    }while(u4MinPOCFBufIdx != 0xff);


#if 0
  // Check if DPB full
  iMinPOC = 0x7fffffff;
  for(i=0; i<_tVerMpvDecPrm.SpecDecPrm.rVDecH264DecPrm.ucMaxFBufNum; i++)
  {
    if(_ptFBufInfo[i].ucFBufStatus == NO_PIC)
    {
      iMinPOC = 0x7fffffff;
      u4MinPOCFBufIdx = i;
      break;
    }
    // miew: need to take care of field empty
    else if((iMinPOC > _ptFBufInfo[i].i4POC) && fgIsNonRefFBuf(i))
    {
      iMinPOC = _ptFBufInfo[i].i4POC;
      u4MinPOCFBufIdx = i;
    }
  }  
  
  // No free FBuf, output 1 fbuf is needed
  if(_ptFBufInfo[u4MinPOCFBufIdx].ucFBufStatus != NO_PIC)
  {
    vVerifyClrFBufInfo(u4MinPOCFBufIdx);
  }
#endif
}



// *********************************************************************
// Function    : void vAdd2RefPicList(UINT32 u4InstID)
// Description : Add the current pic info to Ref Pic List
// Parameter   : None
// Return      : None
// *********************************************************************
void vAdd2RefPicList(UINT32 u4InstID)
{
  
}

// *********************************************************************
// Function    : void vVerifyClrPicRefInfo(UINT32 u4InstID, UCHAR ucPicType, UCHAR ucFBufIdx)
// Description : Clear picture info in frame buffer
// Parameter   : None
// Return      : None
// *********************************************************************
void vVerifyClrPicRefInfo(UINT32 u4InstID, UCHAR ucPicType, UCHAR ucFBufIdx)
{
  if(ucPicType & TOP_FIELD)
  {
    _ptFBufInfo[u4InstID][ucFBufIdx].ucTFldRefType = NREF_PIC;
  }
  if(ucPicType & BOTTOM_FIELD)
  {
    _ptFBufInfo[u4InstID][ucFBufIdx].ucBFldRefType = NREF_PIC;
  }
  _ptFBufInfo[u4InstID][ucFBufIdx].ucFBufRefType = NREF_PIC;
}

// *********************************************************************
// Function    : BOOL fgIsH264VDecComplete(UINT32 u4InstID)
// Description : Check if VDec complete with interrupt
// Parameter   : None
// Return      : None
// *********************************************************************
BOOL fgIsH264VDecComplete(UINT32 u4InstID)
{
  UINT32 u4MbX;
  UINT32 u4MbY;  
  

  if(_fgVDecComplete[u4InstID]  || (_ucMVCType[u4InstID] == 2 && _fgVDecComplete[0]))
  {
    vVDEC_HAL_H264_GetMbxMby(u4InstID,&u4MbX, &u4MbY);
    if(fgIsFrmPic(u4InstID))
    {      
      if(u4MbX < ((_ptCurrFBufInfo[u4InstID]->u4W >> 4) -1) || (u4MbY < ((_ptCurrFBufInfo[u4InstID]->u4H >> 4) -1)))
      {
        return FALSE;
      }
      else
      {
        return TRUE;
      }
    }
    else
    {
      if(u4MbX < ((_ptCurrFBufInfo[u4InstID]->u4W >> 4) -1) || u4MbY < ((_ptCurrFBufInfo[u4InstID]->u4H >> 5) -1))
      {
        return FALSE;
      }
      else
      {
        return TRUE;
      }
    }
  }
  return FALSE;
}

// *********************************************************************
// Function    : void vReadH264ChkSumGolden(UINT32 u4InstID)
// Description : write check sum in rec file
// Parameter   : None
// Return      : None
// *********************************************************************
void vReadH264ChkSumGolden(UINT32 u4InstID)
{
  vVDEC_HAL_H264_VDec_ReadCheckSum(u4InstID, &_u4DumpChksum[u4InstID][0]);
}

// *********************************************************************
// Function    : void vReadWMVChkSumGolden(UINT32 u4InstID)
// Description : write check sum in rec file
// Parameter   : None
// Return      : None
// *********************************************************************
void vReadWMVChkSumGolden(UINT32 u4InstID)
{
  vVDEC_HAL_WMV_VDec_ReadCheckSum(u4InstID, &_u4DumpChksum[u4InstID][0]);
}

// *********************************************************************
// Function    : void vReadMPEGChkSumGolden(UINT32 u4InstID)
// Description : write check sum in rec file
// Parameter   : None
// Return      : None
// *********************************************************************
void vReadMPEGChkSumGolden(UINT32 u4InstID)
{
  vVDEC_HAL_MPEG_VDec_ReadCheckSum(u4InstID, &_u4DumpChksum[u4InstID][0]);
}

void vReadDvChkSumGolden(UINT32 u4InstID)
{
  UINT32  u4Temp,u4Cnt;
  UINT32 u4VDecID;
  UINT32 *pu4CheckSum;
  
  u4VDecID = u4InstID;
  pu4CheckSum = &_u4DumpChksum[u4InstID][0];
  	
  u4Temp = 0;
  *pu4CheckSum = u4VDecReadMC(u4VDecID, 0x5f4);
  pu4CheckSum ++;
  u4Temp ++;
  *pu4CheckSum = u4VDecReadMC(u4VDecID, 0x5f8);    
  pu4CheckSum ++;
  u4Temp ++;
  *pu4CheckSum = u4VDecReadMC(u4VDecID, 0x608);    
  pu4CheckSum ++;
  u4Temp ++;
  *pu4CheckSum = u4VDecReadMC(u4VDecID, 0x60c);        
  pu4CheckSum ++;
  u4Temp ++;

  //MC  378~397  
  for(u4Cnt=378; u4Cnt<=397; u4Cnt++)
  {
    *pu4CheckSum = u4VDecReadMC(u4VDecID, (u4Cnt<<2));
    pu4CheckSum ++;   
    u4Temp ++;
  }

  //AVC VLD  165~179
  for(u4Cnt=165; u4Cnt<=179; u4Cnt++)
  {
    *pu4CheckSum = u4VDecReadAVCVLD(u4VDecID, (u4Cnt<<2));            
      pu4CheckSum ++;  
      u4Temp ++;
  }

  //MV  147~151
  for(u4Cnt=147; u4Cnt<=151; u4Cnt++)
  {
    *pu4CheckSum = u4VDecReadAVCMV(u4VDecID, (u4Cnt<<2));            
    pu4CheckSum ++;       
    u4Temp ++;
  }

  //IP  212    
  *pu4CheckSum = u4VDecReadAVCMV(u4VDecID, (212 << 2));
  pu4CheckSum ++;  
  u4Temp ++;
   
  //IQ  235~239
  for(u4Cnt=241; u4Cnt<=245; u4Cnt++)
  {
    *pu4CheckSum = u4VDecReadAVCMV(u4VDecID, (u4Cnt<<2));            
    pu4CheckSum ++;     
    u4Temp ++;
  }    

  //IS  241~245
  for(u4Cnt=241; u4Cnt<=245; u4Cnt++)
  {
    *pu4CheckSum = u4VDecReadAVCMV(u4VDecID, (u4Cnt<<2));            
    pu4CheckSum ++;     
    u4Temp ++;
  }

  while(u4Temp < MAX_CHKSUM_NUM)
  {
    *pu4CheckSum = 0;            
    pu4CheckSum ++;   
    u4Temp ++;
  }  
}

void vDvCompare(UINT32 u4InstID)
{
   vDvWrData2PC(u4InstID, _pucDumpYBuf[u4InstID]);
}

#ifdef VPMODE
INT32 i4VPModeDecStart(UINT32 u4VDecID,VDEC_INFO_DEC_PRM_T *prDecPrm)
{

#if ((CONFIG_DRV_VERIFY_SUPPORT) ||(CONFIG_DRV_FPGA_BOARD) && (!VDEC_DRV_PARSER))
    VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *) &(prDecPrm->SpecDecPrm.rVDecMPEGDecPrm);
#else
     VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *)prDecPrm->prVDecCodecHalPrm;
#endif

//VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *)prDecPrm->prVDecCodecHalPrm;
#if VDEC_DDR3_SUPPORT
    UINT32 u4DDR3_PicWdith = 0;
#endif
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
	UINT32 u4PicW,u4PicH,u4WidthMB,u4HeightMB;
#endif

#if (CONFIG_DRV_LINUX_DATA_CONSISTENCY)
//    HalFlushInvalidateDCache();
#endif

	if (prDecPrm->ucDecFBufIdx == 2)
	{
		if(prMpegDecPrm->u4FRefBufIdx == 0)
       	{
       	    // Cheng-Jung 20120322 [
           	prMpegDecPrm->rMpegFrameBufSa.u4Pic1YSa = prMpegDecPrm->rMpegFrameBufSa.u4Pic2YSa;
           	prMpegDecPrm->rMpegFrameBufSa.u4Pic1CSa = prMpegDecPrm->rMpegFrameBufSa.u4Pic2CSa;
           	//prMpegDecPrm->rMpegFrameBufSa.u4Pic0YSa = prMpegDecPrm->rMpegFrameBufSa.u4Pic2YSa;
           	//prMpegDecPrm->rMpegFrameBufSa.u4Pic0CSa = prMpegDecPrm->rMpegFrameBufSa.u4Pic2CSa;            
            // ]
		prMpegDecPrm->u4FRefBufIdx = 1;
		prDecPrm->ucDecFBufIdx = 0;
		_u4DecBufIdx[u4VDecID] = 0;
		_u4FRefBufIdx[u4VDecID] = 1;
       	}
       	else
       	{
       	    // Cheng-Jung 20120322 [
          	prMpegDecPrm->rMpegFrameBufSa.u4Pic0YSa = prMpegDecPrm->rMpegFrameBufSa.u4Pic2YSa;
          	prMpegDecPrm->rMpegFrameBufSa.u4Pic0CSa = prMpegDecPrm->rMpegFrameBufSa.u4Pic2CSa;
          	//prMpegDecPrm->rMpegFrameBufSa.u4Pic1YSa = prMpegDecPrm->rMpegFrameBufSa.u4Pic2YSa;
          	//prMpegDecPrm->rMpegFrameBufSa.u4Pic1CSa = prMpegDecPrm->rMpegFrameBufSa.u4Pic2CSa;
            // ]
          	prMpegDecPrm->u4FRefBufIdx = 0;
		prDecPrm->ucDecFBufIdx = 1;
		_u4DecBufIdx[u4VDecID] = 1;
		_u4FRefBufIdx[u4VDecID] = 0;
       	}
	       
	}
	
//    vVDECSetDownScalerPrm(u4VDecID, &prDecPrm->rDownScalerPrm);

    vVDecWriteMC(u4VDecID, RW_MC_R1Y, (u4AbsDramANc(prMpegDecPrm->rMpegFrameBufSa.u4Pic0YSa)) >> 9); // div 512
    vVDecWriteMC(u4VDecID, RW_MC_R1C, (u4AbsDramANc(prMpegDecPrm->rMpegFrameBufSa.u4Pic0CSa)) >> 8); // div 256
    vVDecWriteMC(u4VDecID, RW_MC_R2Y, (u4AbsDramANc(prMpegDecPrm->rMpegFrameBufSa.u4Pic1YSa)) >> 9); // div 512
    vVDecWriteMC(u4VDecID, RW_MC_R2C, (u4AbsDramANc(prMpegDecPrm->rMpegFrameBufSa.u4Pic1CSa)) >> 8); // div 256
    vVDecWriteMC(u4VDecID, RW_MC_BY,  (u4AbsDramANc(prMpegDecPrm->rMpegFrameBufSa.u4Pic2YSa)) >> 8); // div 256
    vVDecWriteMC(u4VDecID, RW_MC_BC,  (u4AbsDramANc(prMpegDecPrm->rMpegFrameBufSa.u4Pic2CSa)) >> 7); // div 128
    vVDecWriteMC(u4VDecID, RW_MC_BY1,  (u4AbsDramANc(prMpegDecPrm->rMpegFrameBufSa.u4Pic2YSa)) >> 9); // div 256
    vVDecWriteMC(u4VDecID, RW_MC_BC1,  (u4AbsDramANc(prMpegDecPrm->rMpegFrameBufSa.u4Pic2CSa)) >> 8); // div 128
    vVDecWriteMC(u4VDecID, RW_MC_DIGY, (u4AbsDramANc(prMpegDecPrm->rMpegFrameBufSa.u4Pic0YSa)) >> 9); // div 512
    vVDecWriteMC(u4VDecID, RW_MC_DIGC, (u4AbsDramANc(prMpegDecPrm->rMpegFrameBufSa.u4Pic0CSa)) >> 8); // div 256  
    vMCSetOutputBuf(u4VDecID, (UINT32)prDecPrm->ucDecFBufIdx, prMpegDecPrm->u4FRefBufIdx);

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
      #if VDEC_DDR3_SUPPORT
          u4DDR3_PicWdith = (((prDecPrm->u4PicBW + 63) >> 6) << 2);
          #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
          vVDecWriteMC(u4VDecID, RW_MC_PIC_W_MB, u4DDR3_PicWdith);  
          #else
          vVDecWriteVLD(u4VDecID, RW_VLD_PIC_W_MB, u4DDR3_PicWdith);  
          #endif
      #else
          #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
          vVDecWriteMC(u4VDecID, RW_MC_PIC_W_MB, ((prDecPrm->u4PicBW + 15)>> 4));  
          vVDecWriteMC(u4VDecID, RW_MC_DDR3_EN, (u4VDecReadMC(u4VDecID, RW_MC_DDR3_EN)  & 0xFFFFFFFE));
      #else
          vVDecWriteVLD(u4VDecID, RW_VLD_PIC_W_MB, ((prDecPrm->u4PicBW + 15)>> 4));  
      #endif
#endif
#endif

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
    vVDecWriteMC(0, 0x5E4, (u4VDecReadMC(0, 0x5E4) |(0x1 <<12)) );
    //vVDecWriteMC(0, 0x660, (u4VDecReadMC(0, 0x660) |(0x80000000)) );    
    #ifndef VDEC_PIP_WITH_ONE_HW
    vVDecWriteMC(1, 0x5E4, (u4VDecReadMC(1, 0x5E4) |(0x1 <<12)) );
    //vVDecWriteMC(1, 0x660, (u4VDecReadMC(1, 0x660) |(0x80000000)) );    
    #endif
#endif

    if (prMpegDecPrm->rMpegPpInfo.fgPpEnable)
    {
        UINT32 u4MBqp = 0;
        
        vVDecWriteMC(u4VDecID, RW_MC_PP_ENABLE, 1);
        vVDecWriteMC(u4VDecID, RW_MC_PP_Y_ADDR, u4AbsDramANc(prMpegDecPrm->rMpegPpInfo.u4PpYBufSa) >> 9);
        vVDecWriteMC(u4VDecID, RW_MC_PP_C_ADDR, u4AbsDramANc(prMpegDecPrm->rMpegPpInfo.u4PpCBufSa) >> 8);
        vVDecWriteMC(u4VDecID, RW_MC_PP_MB_WIDTH, (prDecPrm->u4PicW + 15) >> 4);
        
        u4MBqp = (prMpegDecPrm->rMpegPpInfo.au1MBqp[0] & 0x1F) | ((UINT32)(prMpegDecPrm->rMpegPpInfo.au1MBqp[1] & 0x1F) << 8) \
                       | ((UINT32)(prMpegDecPrm->rMpegPpInfo.au1MBqp[2] & 0x1F) << 16) | ((UINT32)(prMpegDecPrm->rMpegPpInfo.au1MBqp[3] & 0x1F) << 24);        
        vVDecWriteMC(u4VDecID, RW_MC_PP_QP_TYPE, u4MBqp);  
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8550)
        //vVDecWriteMC(u4VDecID, RW_MC_PP_DBLK_MODE, DBLK_Y+DBLK_C);
        vVDecWriteMC(u4VDecID, RW_MC_PP_DBLK_MODE, 0);
        vVDecWriteMC(u4VDecID, RW_MC_PP_WB_BY_POST, 0); // wirte MC out and PP out
        if (prMpegDecPrm->rMpegPpInfo.fgPpDemoEn)
        {
            vVDecWriteMC(u4VDecID, 0x658, ((u4VDecReadMC(u4VDecID, 0x658)&0xFFFFFFFE)|0x1)); // partial deblocking
            vVDecWriteMC(u4VDecID, 0x65C, ((((prDecPrm->u4PicH + 15) >> 4) - 1) << 24) | ((((prDecPrm->u4PicW + 15) >> 5) - 1) << 8)); // XY end MB
        }
        else
        {
            vVDecWriteMC(u4VDecID, 0x658, (u4VDecReadMC(u4VDecID, 0x658)&0xFFFFFFFE));
        }
#else
        vVDecWriteMC(u4VDecID, RW_MC_PP_DBLK_MODE, DBLK_Y+DBLK_C);
        //vVDecWriteMC(u4VDecID, RW_MC_PP_WB_BY_POST, 0); // wirte MC out and PP out
#endif        
        //vVDecWriteMC(u4VDecID, RW_MC_PP_QP_TYPE, 0x00000114);
        //vVDecWriteMC(u4VDecID, RW_MC_PP_WB_BY_POST, 0); // wirte MC out and PP out
        vVDecWriteMC(u4VDecID, RW_MC_PP_X_RANGE, ((prDecPrm->u4PicW + 15) >> 4) - 1);
        vVDecWriteMC(u4VDecID, RW_MC_PP_Y_RANGE, (((prDecPrm->u4PicH + 15) >> 4) >> (prDecPrm->ucPicStruct != 3)) - 1);
        //vVDecWriteAVCVLD(u4VDecID, RW_AVLD_SHDR_2, 0x6E00);
        //vVDecWriteMC(u4VDecID, RW_MC_PP_MODE, H264_MODE);
    }
    else
    {
        vVDecWriteMC(u4VDecID, RW_MC_PP_ENABLE, 0);
    }

    #if !MPEG4_6582_SUPPORT
    // MT6582 no longer need [    
    vVDecWriteVLD(u4VDecID, RW_VLD_PSUPCTR, ((prDecPrm->u4PicW * prDecPrm->u4PicH) >> 8) + 1);
    vVDecWriteVLD(u4VDecID, RW_VLD_PARA, 0xC0500000); //Frame Picture + VP ???
    // ]
    #endif
    
    #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
//    vVDecWriteVLD(u4VDecID, RW_VLD_PICSZ, ((prDecPrm->u4PicH) << 16) + (prDecPrm->u4PicW >> 4));
	
	u4PicW = ((prDecPrm->u4PicW +15)>>4)<<4;
	u4PicH = ((prDecPrm->u4PicH + 15)>>4)<<4;
	u4WidthMB = ((prDecPrm->u4PicW +15)>>4);
	u4HeightMB = ((prDecPrm->u4PicH +15)>>4);
	//vVDecWriteTopVLD(u4VDecID,RW_TOPVLD_WMV_PICSIZE,u4PicH<<16|u4PicW);
	vVDecWriteVLDTOP(u4VDecID,RW_TOPVLD_WMV_PICSIZE,u4PicH<<16|u4PicW);
	#if MPEG4_6582_SUPPORT
	vVDecWriteVLDTOP(u4VDecID,RW_TOPVLD_WMV_PICSIZE_MB,(u4HeightMB-1)<<16|(u4WidthMB-1));
	#else
	vVDecWriteVLD(u4VDecID, RW_VLD_PICSZ, (prDecPrm->u4PicH)<<16);
	vVDecWriteVLD(u4VDecID, RW_VLD_DIGMBSA, u4WidthMB);
//	vVDecWriteTopVLD(u4VDecID,RW_TOPVLD_WMV_PICSIZE_MB,(((prDecPrm->u4PicW+ 15)>>4) -1) | ((((prDecPrm->u4PicH + 15)>>4) - 1)<<16));
	vVDecWriteVLD(u4VDecID, RW_VLD_MBROWPRM,  0x1ff );
	#endif // MPEG4_6582_SUPPORT
    #else
    vVDecWriteVLD(u4VDecID, RW_VLD_PICSZ, ((prDecPrm->u4PicH + 15) << 16) + (prDecPrm->u4PicW >> 4));
    vVDecWriteVLD(u4VDecID, RW_VLD_MBROWPRM,  ( ((prDecPrm->u4PicH + 15) >> 4 ) - 1) << 16);
    #endif
    
    // addr swap mode
    vVDecWriteMC(u4VDecID, RW_MC_ADDRSWAP, prDecPrm->ucAddrSwapMode);
  
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8550)
     vVDecWriteMC(u4VDecID, RW_MC_NBM_CTRL,  
                 ((u4VDecReadMC(u4VDecID, RW_MC_NBM_CTRL)  & 0xFFFFFFF8) |prDecPrm->ucAddrSwapMode));
#endif 

/*  
    vVDecWriteMC(u4VDecID, RW_MC_HREFP, 0);
    vVDecWriteMC(u4VDecID, RW_MC_DIGWD, ((prDecPrm->u4PicW + 15) >> 4));
    vVDecWriteVLD(u4VDecID, RW_VLD_DIGMBSA, 0);
    vVDecWriteVLD(u4VDecID, RW_VLD_SCALE, 0);//(random(3)<<24) |(random(3)<<16));
    vVDecWriteVLD(u4VDecID, RW_VLD_DIGMBYOFF, 0);
*/
    vVDecWriteMC(u4VDecID, RW_MC_UMV_PIC_WIDTH, prDecPrm->u4PicW);
    vVDecWriteMC(u4VDecID, RW_MC_UMV_PIC_HEIGHT, prDecPrm->u4PicH);
  

    #if MPEG4_6582_SUPPORT
    vVDecWriteVLDTOP(u4VDecID, 0x90, 2);
    vVDecWriteVLDTOP(u4VDecID, 0x90, 3);
    #else
    vVDecWriteVLD(u4VDecID, RW_VLD_PROC, VLD_RTERR + VLD_PDHW + VLD_PSUP +
              (prDecPrm->u4PicW >> 4));
    #endif
    return HAL_HANDLE_OK;	
}
#endif

void vAVCDumpChkSum(void)
{
	UINT32 i,u4Val,u4VDecID = 0;
	printk("read AVCVLD \n");
	for(i = 165; i<180; i++)
	{
		u4Val = u4VDecReadAVCVLD(u4VDecID, i<<2);
		printk("%d (0x%x)  = 0x%4x\n",i, (i<<2),u4Val);
	}
	printk("read AVC MC \n");	
	for(i = 147; i<152; i++)
	{
		u4Val = u4VDecReadAVCMV(u4VDecID, i<<2);
		printk("%d (0x%x)  = 0x%4x\n",i, (i<<2),u4Val);
	}

	printk("read AVC MC \n");	
	for(i = 378; i<397; i++)
	{
		u4Val = u4VDecReadMC(u4VDecID, i<<2);
		printk("%d (0x%x)  = 0x%4x\n",i, (i<<2),u4Val);
	}


	
}


void vPrintDumpReg(UINT32 u4InstID,UINT32 fgTAB)
{
	
    UINT32 u4Val,u4Cnt;
//    UINT32 u4InstID = 0;
//    printk("Before Decode!\n");
    #ifndef REG_LOG_NEW
	if(fgTAB)
	{
		u4InstID = 1;
		printk("After Decode!\n");
	}
	else
	{
		u4InstID = 0;
		printk("Before Decode!\n");
	}
    for(u4Cnt = 33; u4Cnt < 40; u4Cnt++)
    {
    	u4Val = ((UINT32 *)(_pucRegister[u4InstID]))[u4Cnt] ;
       printk("%d (0x%x)  = 0x%4x\n",u4Cnt, (u4Cnt<<2),u4Val);
    }
    for(u4Cnt = 42; u4Cnt < 71; u4Cnt++)
    {
    	u4Val  = ((UINT32 *)(_pucRegister[u4InstID]))[u4Cnt] ;
       printk("%d (0x%x)  = 0x%4x\n",u4Cnt, (u4Cnt<<2),u4Val);
    }
    for(u4Cnt = 112; u4Cnt < 131; u4Cnt++)
    {
    	u4Val  = ((UINT32 *)(_pucRegister[u4InstID]))[u4Cnt] ;
       printk("%d (0x%x)  = 0x%4x\n",u4Cnt, (u4Cnt<<2),u4Val);
    }
	for(u4Cnt = 131; u4Cnt < 192; u4Cnt++)
    {
    	u4Val  = ((UINT32 *)(_pucRegister[u4InstID]))[u4Cnt] ;
       printk("%d (0x%x)  = 0x%4x\n",u4Cnt, (u4Cnt<<2),u4Val);
    }
    for(u4Cnt = 192; u4Cnt < 256; u4Cnt++)
    {
    	u4Val = ((UINT32 *)(_pucRegister[u4InstID]))[u4Cnt] ;
       printk("%d (0x%x)  = 0x%4x\n",u4Cnt, (u4Cnt<<2),u4Val);
    }
	
    printk("MC register data \n");
    for(u4Cnt = 0; u4Cnt < 700; u4Cnt++)
    {
    	u4Val = ((UINT32 *)(_pucRegister[u4InstID]))[u4Cnt + 0x100] ;
       printk("%d (0x%x)  = 0x%4x\n",u4Cnt, (u4Cnt<<2),u4Val);
    }

	 printk("IS Settings\n");
     for(u4Cnt = 128; u4Cnt < 192; u4Cnt++)
     {
         u4Val = ((UINT32 *)(_pucRegister[u4InstID]))[u4Cnt + 1000] ;
         printk("%d (0x%x)  = 0x%4x\n",u4Cnt, (u4Cnt<<2),u4Val);
     }
	 printk("IQ Settings\n");
     for(u4Cnt = 320; u4Cnt < 384; u4Cnt++)
 	 {
 	 	u4Val = ((UINT32 *)(_pucRegister[u4InstID]))[u4Cnt + 1100] ;
         printk("%d (0x%x)  = 0x%4x\n",u4Cnt, (u4Cnt<<2),u4Val);
 	 }

	 printk("IT Settings\n");
     for(u4Cnt = 576; u4Cnt < 640; u4Cnt++)
     {
        u4Val = ((UINT32 *)(_pucRegister[u4InstID]))[u4Cnt + 1200] ;
        printk("%d (0x%x)  = 0x%4x\n",u4Cnt, (u4Cnt<<2),u4Val);
     }

	 for(u4Cnt = 0; u4Cnt < 65; u4Cnt++)
     {
        u4Val = ((UINT32 *)(_pucRegister[u4InstID]))[u4Cnt + 2000] ;
        printk("%d (0x%x)  = 0x%4x\n",u4Cnt, (u4Cnt<<2),u4Val);
     }
	 
    printk("Dump end!\n");
    #endif
}

#ifdef	VDEC_SRAM
#define SRAMSZ	52*1024
#define SRAMWRTCMD (1 << 16)
void vWriteSram(UINT32 u4InstID,UINT32 u4SramAddr,UINT32 u4SramValue)
{
	UINT32 u4Temp;
	#if 0
	vVDecWriteMC(u4InstID,0x93C,u4VDecReadMC(u4InstID,0x93C)|(1<<0));//enable of sram and cs of sram
	vVDecWriteMC(u4InstID,0x93C,u4VDecReadMC(u4InstID,0x93C)&(~(0x3fff<<12)));//set sram addr
	vVDecWriteMC(u4InstID,0x93C,u4VDecReadMC(u4InstID,0x93C) | (u4SramAddr<<12));//set sram addr
	vVDecWriteMC(u4InstID,0x940,u4SramValue);//set sram data
	u4Temp = (1<<4)|(1<<8);
	vVDecWriteMC(u4InstID,0x93C,u4VDecReadMC(u4InstID,0x93C) | u4Temp);//enable write
	vVDecWriteMC(u4InstID,0x93C,0);//clear all
	#endif
	u4Temp = (u4VDecReadMC(u4InstID,0x93C) & 0xffffc000)|SRAMWRTCMD;
	u4Temp |= u4SramAddr;
	printk("<vdec>write addr = %d,value = 0x%x\n",u4Temp,u4SramValue);
	vVDecWriteMC(u4InstID,0x93C,u4Temp);
	vVDecWriteMC(u4InstID,0x940,u4SramValue);
}

UINT32 u4ReadSram(UINT32 u4InstID,UINT32 u4SramAddr)
{
	UINT32 u4RegVal,u4Temp;
	#if 0
	vVDecWriteMC(u4InstID,0x93C,u4VDecReadMC(u4InstID,0x93C)|(1<<0)|(1<<4));//enable of sram and cs of sram
       vVDecWriteMC(u4InstID,0x93C,u4VDecReadMC(u4InstID,0x93C)&(~(0x3fff<<12)));//set sram addr
	vVDecWriteMC(u4InstID,0x93C,u4VDecReadMC(u4InstID,0x93C) | u4SramAddr<<12);//set sram addr
	u4RegVal = u4VDecReadMC(u4InstID,0x944);
//	vVDecWriteMC(u4InstID,0x93C,u4VDecReadMC(u4InstID,0x93C)&(~(1<<0))&(~(1<<4)));//disable sram read 
	vVDecWriteMC(u4InstID,0x93C,0);
	#endif
	u4Temp = (u4VDecReadMC(u4InstID,0x93C) & (~(SRAMWRTCMD))) & 0xffffc000;
	u4Temp |= u4SramAddr;
	vVDecWriteMC(u4InstID,0x93C,u4Temp);
	u4RegVal = u4VDecReadMC(u4InstID,0x940);
	printk("<vdec>sram addr = %d,data is 0x%x\n",u4SramAddr,u4RegVal);
	return u4RegVal;
}

void vDumpSram(UINT32 u4InstID)
{
	UINT32 u4Mcstart,u4SramAddr,u4RegVal,u4ReadSize,u4temp;
	UCHAR fpDumpFile[100] = "d:\\ChkFolder\\sram";
//	UCHAR ucTempBuff[30];
	UCHAR *fpRear = ".bin";
	FILE *pFile = NULL;

	u4temp = strlen(fpDumpFile);
	u4temp += sprintf(fpDumpFile+u4temp,"%d",_u4FileCnt[u4InstID]);
	u4temp += sprintf(fpDumpFile+u4temp,"%s",fpRear);
//	vVDecWriteMC(u4InstID,0x93C,u4VDecReadMC(u4InstID,0x93C)|(1<<0)|(1<<4));//enable of sram and cs of sram
       vVDecWriteMC(u4InstID,0x93C,u4VDecReadMC(u4InstID,0x93C)|(1<<0));//enable of sram and cs of sram
       vVDecWriteMC(u4InstID,0x93C,u4VDecReadMC(u4InstID,0x93C)|(1<<4));//enable of sram and cs of sram
       printk("<vdec>Before read SRAM MC 0x93C = 0x%x \n",u4VDecReadMC(u4InstID,0x93C));
	for(u4Mcstart = 0;  u4Mcstart <13312; u4Mcstart++)
	{
		u4SramAddr = (u4Mcstart) << 12;
		vVDecWriteMC(u4InstID,0x93C,u4VDecReadMC(u4InstID,0x93C)&(~(0x3fff<<12)));//set sram addr
		vVDecWriteMC(u4InstID,0x93C,u4VDecReadMC(u4InstID,0x93C) | u4SramAddr);//set sram addr
//		x_thread_delay(2);
		u4RegVal = u4VDecReadMC(u4InstID,0x944);
		((UINT32*)(_pucDumpSRAMBuf[u4InstID]))[u4Mcstart] = u4RegVal;
	}
	
//	vVDecWriteMC(u4InstID,0x93C,u4VDecReadMC(u4InstID,0x93C)&(~(1<<0))&(~(1<<4)));//disable sram read 
	vVDecWriteMC(u4InstID,0x93C,u4VDecReadMC(u4InstID,0x93C)&(~(1<<4)));//disable sram read 
	vVDecWriteMC(u4InstID,0x93C,u4VDecReadMC(u4InstID,0x93C)&(~(1<<0)));//disable sram read 
	printk("<vdec>After read SRAM MC 0x93C = 0x%x \n",u4VDecReadMC(u4InstID,0x93C));
	
	pFile = fopen(fpDumpFile,"w+");
	if(pFile == NULL)
	{
		printk("Create file error !\n");
	}
	#if 0
	for(u4Mcstart = 0;  u4Mcstart <13312; u4Mcstart++)
	{
		ucLen = 0;
		ucLen = sprintf(ucTempBuff,"%x ",(_pucDumpSRAMBuf[u4InstID])[u4Mcstart*4 + 3]);
		ucLen += sprintf(ucTempBuff+ucLen,"%x ",(_pucDumpSRAMBuf[u4InstID])[u4Mcstart*4 + 2]);
		ucLen += sprintf(ucTempBuff+ucLen,"%x ",(_pucDumpSRAMBuf[u4InstID])[u4Mcstart*4 + 1]);
		ucLen += sprintf(ucTempBuff+ucLen,"%x",(_pucDumpSRAMBuf[u4InstID])[u4Mcstart*4 ]);
		fseek(pFile,SEEK_CUR,0);
		fwrite((char*)(_pucDumpSRAMBuf[u4InstID] + u4Mcstart*4),1,strlen(ucTempBuff),pFile);
	}
	#endif
	
	u4ReadSize = fwrite ((char* )(_pucDumpSRAMBuf[u4InstID]), 1, SRAMSZ, pFile);
	
	printk("read file len = %d \n",u4ReadSize);
	fclose(pFile);
}
#endif
