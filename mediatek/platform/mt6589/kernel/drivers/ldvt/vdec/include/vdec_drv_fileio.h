
#ifndef _VDEC_DRV_FILEIO_H_
#define _VDEC_DRV_FILEIO_H_

typedef struct _FilterInfo_T
{
  UINT32 u4WrIdx;                       ///< [OUT] interface handle
  UINT32 u4Handle;                       ///< [OUT] interface handle
  Filter_OpIf* pprFilterOpIf;
}Filter_Info;

typedef struct TScriptData
{
  UCHAR *pbTargetAddr;
  UCHAR *pbRecAddr;
  UCHAR *pbFileName;
  UINT32 dwFileLength;
  UINT32 dwRealGetBytes;
}TScriptData;


#if defined(MPV_NO_PARSER) || defined(MPV_READ_FILE)
#ifndef VDEC_SR_SUPPORT
#define V_FIFO_SZ 0x00400000 //80*1024*1024
#else
#define V_FIFO_SZ 0x00320000 //80*1024*1024  //only for SR
#endif
#endif

UINT32 u4BreakPoint4Read(UCHAR* pucFileName, UINT32 u4StartAddr);
void vBreakPoint4Write(UCHAR* pucFileName, UINT32 u4StartAddr, UINT32 u4FileSize);

#if defined(MPV_NO_PARSER) || defined(MPV_READ_FILE)
void VDec_Load_Data(UCHAR ucEsId);
void VDec_Load_MPEG4_Data(UCHAR ucEsId);
void VDec_Load_H264_Data(UCHAR ucEsId, BOOL fgDepView);
#endif

#if defined(MPV_NO_PARSER) || defined(MPV_DUMP_FBUF)
void VDec_Dump_Data(UINT32 u4StartAddr, UINT32 u4FileSize, UINT32 u4FileCnt, UCHAR* pucAddStr);
#endif

#endif
