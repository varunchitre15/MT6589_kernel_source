#ifndef _VDEC_INFO_VERIFY_H_
#define _VDEC_INFO_VERIFY_H_

//#include "drv_config.h"
#include "../include/type.h"
#include <mach/mt_typedefs.h>

#define  FILE       UINT32
#if (!CONFIG_DRV_LINUX)
//#include <stdio.h>
#else
//#include "x_os.h"

#ifndef VDEC_EMUVER_FILEIO
//#include "stdio.h"
#else //VDEC_EMUVER_FILEIO
#define  FILE       UINT32
#endif //VDEC_EMUVER_FILEIO
//#define strcpy     x_strcpy
//#define sprintf     x_sprintf
//#define strlen      x_strlen
#endif


typedef struct _VDEC_INFO_VERIFY_FILE_INFO_T_
{
  BOOL     fgGetFileInfo;
  UINT32  u4FileLength;
  UINT32  u4FileOffset;
  UCHAR  *pucTargetAddr;
  UINT32  u4TargetSz;
  UINT32  u4RealGetBytes;
  CHAR     bFileName[300];
  FILE *pFile;
  INT32 i4FileId;
}VDEC_INFO_VERIFY_FILE_INFO_T;


typedef struct _VDEC_INFO_VERIFY_DEC_T_
{
  UCHAR   ucState;
  UINT32  u4DecFlag;
}VDEC_INFO_VERIFY_DEC_T;


typedef struct _VDEC_INFO_VERIFY_PIC_T_
{
  UCHAR ucMpegVer;
  UINT32 u4W;
  UINT32 u4H;
}VDEC_INFO_VERIFY_PIC_T;


#endif

