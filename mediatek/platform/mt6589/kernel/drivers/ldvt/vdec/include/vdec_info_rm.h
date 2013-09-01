
#ifndef _VDEC_INFO_RM_H_
#define _VDEC_INFO_RM_H_

//#include "drv_config.h"

#if(!CONFIG_DRV_VERIFY_SUPPORT)
#include "vdec_usage.h"
#include "vdec_info_common.h"
#else
#include "x_stl_lib.h"
//#include "x_assert.h"
//#include "u_pbinf.h"
#endif


#define RM_DRV_IMAGERESIZER_ENABLE

#define VDECRM_DRV_EV_SCALE_DONE           (1<<0)
#define VDECRM_DRV_EV_SCALE_ABORT         (1<<1)
#define VDECRM_DRV_EV_SCALE_ERR              (1<<2)
#define VDECRM_DRV_EV_SCALE_READY         (1<<3)
#define VDECRM_DRV_EV_SCALE_TIMEOUT         (1<<4)



#define RM_MVHWBUF_SZ    0x1FE00
#define RM_VLDPRED_SZ      0xC000

#define RPR_WORK_Y_SZ          (1920*1088)
#define RPR_WORK_C_SZ          (1920*1088 / 2)
//#define PIC_Y_SZ          (1920*1088)
//#define PIC_C_SZ          (1920*1088 / 2)

#define RM_RSZWORKBUF_SZ      0x1800

typedef enum
{
  RM_INTRAPIC,        /* 0 (00) */
  RM_FORCED_INTRAPIC, /* 1 (01) */
  RM_INTERPIC,        /* 2 (10) */
  RM_TRUEBPIC         /* 3 (11) */
} EnumRMPicCodType;

typedef struct _VDEC_INFO_RM_VFIFO_PRM_T_
{
    UINT32 u4VFifoSa;                 ///< Video Fifo memory start address    
    UINT32 u4VFifoEa;                 ///< Video Fifo memory end address 
}VDEC_INFO_RM_VFIFO_PRM_T;

typedef struct _VDEC_HAL_DEC_RM_ERR_INFO_T_
{    
    UINT32 u4RMErrCnt;                                ///< Video decode error count
    UINT32 u4RMErrRow;                                ///< Video decode error mb row 
    UINT32 u4RMErrType;                               ///< Video decode error type
    UINT16 u2RMMBErrCnt; 
}VDEC_INFO_RM_ERR_INFO_T;


typedef struct _VDEC_INFO_RM_BS_INIT_PRM_T_
{
    UINT32 u4ReadPointer;              
    UINT32 u4WritePointer;
    UINT32 u4VFifoSa;                 ///< Video Fifo memory start address    
    UINT32 u4VFifoEa;                 ///< Video Fifo memory end address 
}VDEC_INFO_RM_BS_INIT_PRM_T;



//Init HW Work Buf when Init Process
typedef struct _VDEC_INFO_RM_WORK_BUF_SA_T_
{
    UINT32  u4RMMvWorkBuf;
    UINT32  u4RMVldWorkBuf;
    UINT32  u4RMRPRRefBufY;
    UINT32  u4RMRPRRefBufC;
    UINT32  u4RPRHWWorkBuf;
}VDEC_INFO_RM_WORK_BUF_SA_T;




typedef struct _VDEC_INFO_RM_PICINFO_T_
{
  BOOL   fgRV9;  // TRUE -> RV9  FALSE -> RV8
  EnumRMPicCodType ePtype;
  UINT32 u4OrgWidth;
  UINT32 u4OrgHeight;
  UINT32 u4Width;
  UINT32 u4Height;
  UINT32 u4PctszSize;
  UINT32 u4Pctsz;
  UINT32 u4Pquant;
  UINT32 u4Oquant;
  UINT32 u4DFP;
  UINT32 u4Tr;
  UINT32 u4MbaSize;
  UINT32 u4Mba;
  UINT32 u4Rtype;
  UINT32 u4Iratio;
  UINT32 u4HdrSkip;
  UINT32 u4NumSlice;
  UINT32 au4SliceSize[128];
  UINT32 u4BstLength;



  BOOL fgBwdIsI;  
  //RV9, RV10
  BOOL fgECC;    // 1 Bit
  UINT32 u4PSQuant;    // 5 Bit
  BOOL fgBit_Ver; 
  BOOL fgInterlace;
  UINT32 u4OsvQuant;
  BOOL fgDeblockPassThru;
  UINT32 u4RvTr;
  BOOL fgUserPrevWidth;

  //RV8
  UINT32 u4RvBistreamVersion;    // 3 Bit
  BOOL fgRType;    // 1 Bit
  UINT32 u4PicSz;    

  //HW Status
  UINT32 u4RefQpMb0;

  //Buf Inof
  UINT32 u4OutBufY;
  UINT32 u4OutBufC;
  UINT32 u4FwdBufY;
  UINT32 u4FwdBufC;
  UINT32 u4BwdBufY;
  UINT32 u4BwdBufC;

  //Resize Pic Size Info
  BOOL fgCurIsRPRPic;
  UINT32 u4PrevDispWidth;
  UINT32 u4PrevDispHeight;
  UINT32 u4PrevDecWidth;
  UINT32 u4PrevDecHeight;

  //Emulation Info
  UINT32 u4DecodedPicCnt;

  UINT32 u4MCOutBufY;
  UINT32 u4MCOutBufC;

  //For HW Setting
  UINT32 u4AddrSwapMode;
  UINT32 u4InitInputWindow;

  BOOL fgEnableCRC;
  BOOL fgEnableMCOutput;
  BOOL fgEnableDDR3;
  BOOL fgEnableAddrSwap;
  BOOL fgEnablePPOut;
  UINT8 uRPRMode;    //0: Disable RPR Mode, 1: Normal Mode, 2: Racing Mode
  UINT32 u4RPRState;

  //Iratio Calc
  UINT32 u4TrWarp;
  UINT32 u4FRefTr;
  UINT32 u4BRefTr;
  UINT32 u4IratioTr;
  INT32 i4Trd;
  INT32 i4Trb;
} VDEC_INFO_RM_PICINFO_T;

typedef struct _VDEC_INFO_RM_DEC_PRM_T_
{
    // MPEG version: 1, 2, 3, 4 (3: DivX 3.11)
    UCHAR  ucMpegVer;
    UCHAR  ucDecFld;
    UINT32  u4FRefBufIdx;
    
    // Decode picture setting
    BOOL    fgB21Mode;
    BOOL    fgRetErr;
    BOOL    fgIgnoreVdo;
    BOOL    fgDec2ndFld;
    UINT32 u4DecXOff;
    UINT32 u4DecYOff;
    UINT32 u4DecW;
    UINT32 u4DecH;
    UINT32 u4MaxMbl;
    UINT32 u4BBufStart;
    //VDEC_INFO_MPEG_FRAME_BUF_SA_T rMpegFrameBufSa;
    //VDEC_INFO_MPEG_PP_INFO_T rMpegPpInfo;
    //VDEC_INFO_MPEG2_PIC_PRM_T rMp2PicPrm;

    UINT32 u4MvHwWorkBuf;    //
    UINT32 u4VldPredWorkBuf;    //
    VDEC_INFO_RM_PICINFO_T rRMPicInfo;    //Pic info from frminfo.bin    //only for emulation
    VDEC_INFO_RM_PICINFO_T rRMPicInfo_Preparsing;    //Pic info from frminfo.bin    //only for emulation and RPR Racing Mode
    VDEC_INFO_RM_PICINFO_T rRMParsPicInfo;    //Parsing Real RM Syntax
    UINT32 u4RMFrmInfoBuf;    //
    UINT32 u4RMFrmInfoRPtr;                       //only for emulation

    UINT32 u4RMGoldenDataBuf;                   //Golden Data
    UINT32 u4RMGoldenDataBufRPtr;            //Golden Data RPtr

    UINT32 u4RMRingFlowTempFIFO;            //Temp FIFO for Ring Case
    UINT32 u4RMRingFlowTempFIFOSize;     //Temp FIFO Size for Ring Case

    UINT32 u4RMVFIFOWPtr;                          //FIFO Write Pointer
    UINT32 u4RMFIFORPtr;                             //FIFO Read Pointer
    UINT32 u4RMVFIFOSa;                              //VFIFO Sa
    UINT32 u4RMVFIFOSz;                              //VFIFO Sz

    UINT32 u4HWInputWindow;

    //RPR Racing Mode Flag
    BOOL fgRPRRacingModeEnable;  //TRUE => Enable

    UINT32 u4RMAddrSwapMode;


    UINT32 u4ClipTotalFrameCnt;        //Frame Number
    UINT32 u4FrameInfoSize;
    UINT32 u4FramePayloadSize;

    UINT32 u4GlobalChecksumBuf;
    UINT32 u4GlobalChecksumBufSize;

    UINT32 u4CRCResultBufAddr;
    UINT32 u4CRCCheckFrameNumber;
    BOOL fgCRCLoadGoldenFlag;
} VDEC_INFO_RM_DEC_PRM_T;



#endif //#ifndef _VDEC_INFO_RM_H_



