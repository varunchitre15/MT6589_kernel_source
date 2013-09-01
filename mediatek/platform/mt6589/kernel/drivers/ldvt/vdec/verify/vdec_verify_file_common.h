#ifndef _VDEC_VERIFY_FILE_COMMON_H_
#define _VDEC_VERIFY_FILE_COMMON_H_

#include "../include/vdec_info_common.h"
#include "vdec_verify_general.h"
#include "vdec_info_verify.h"
#define RM_SOURCEPATH_INDEX        2
#define RM_GOLDENPATH_INDEX        3
#define RM_AUFIFO_INDEX                 4
#define RM_FRMINFO_INDEX               6
#define RM_SUMINFO_INDEX               7
#define RM_CRCINFO_INDEX                11

void vVDecOutputDebugString(const CHAR * format, ...);

void vErrMessage(UINT32 u4InstID, CHAR *pbDramAddr);
//void vDataFromFile(UINT32 u4InstID, UINT32 *pu4W, UINT32 *pu4H);

/// ICE Semihosting
BOOL fgOpenPCFile(UINT32 u4InstID, char *strFileName , char *strMode, VDEC_INFO_VERIFY_FILE_INFO_T *ptFileInfo);

BOOL fgReadPCFile(void* pvAddr, UINT32 u4Size, char *strFileName, UINT32 u4OffSet, UINT32 *pu4RealReadSize, UINT32 *pu4FileLength);

BOOL fgWrMsg2PC(void* pvAddr, UINT32 u4Size, UINT32 u4Mode, VDEC_INFO_VERIFY_FILE_INFO_T *pFILE_INFO);

BOOL fgOverWrData2PC(void* pvAddr, UINT32 u4Size, UINT32 u4Mode, char *strFileName);
///

/// IDE HDD
BOOL fgOpenHDDFileEx(UINT32 u4InstID, char *strFileName , char *strMode, VDEC_INFO_VERIFY_FILE_INFO_T *ptFileInfo, const char *szFile, INT32 i4Line);
#define fgOpenHDDFile(u4InstID, strFileName, strMode, ptFileInfo) fgOpenHDDFileEx(u4InstID, strFileName, strMode, ptFileInfo, __FUNCTION__, __LINE__)

    
BOOL fgReadHDDFile(UINT32 u4InstID, BOOL fgReadInfo, void* pvAddr, UINT32 u4Size, char *strFileName, UINT32 u4OffSet, UINT32 *pu4RealReadSize, UINT32 *pu4FileLength, INT32* pi4FileId);
///


/// File IC General Interface
BOOL fgWrData2PC(void* pvAddr, UINT32 u4Size, UINT32 u4Mode, char *strFileName);

BOOL fgOpenFileEx(UINT32 u4InstID, char *strFileName , char *strMode, VDEC_INFO_VERIFY_FILE_INFO_T *ptFileInfo, const char *szFunction, INT32 i4Line);

#define fgOpenFile(u4InstID, strFileName, strMode, ptFileInfo) fgOpenFileEx(u4InstID, strFileName, strMode, ptFileInfo, __FUNCTION__, __LINE__)
///

#ifdef IDE_WRITE_SUPPORT
BOOL fgIdeWrData2PC(void* pvAddr, UINT32 u4Size, UINT32 u4Mode, char *strFileName);
#endif

#ifdef IDE_READ_SUPPORT
BOOL fgReadIdeFile(void* pvAddr, UINT32 u4Size, char *strFileName, UINT32 u4OffSet, UINT32 *pu4RealReadSize, UINT32 *pu4FileLength);
BOOL fgPureOpenIdeFile(char *strFileName , char *strMode, VDEC_INFO_VERIFY_FILE_INFO_T *ptFileInfo);
BOOL fgOpenIdeFile(char *strFileName , char *strMode, VDEC_INFO_VERIFY_FILE_INFO_T *ptFileInfo);
#endif


#if (CONFIG_DRV_LINUX)
//#ifdef VDEC_EMUVER_FILEIO
UINT32 linux_ftell (FILE *pfile);
UINT32  linux_fseek(FILE *pfile, long offset, int cnt);
UINT32   linux_fclose(FILE *pfile);
FILE*   linux_fopen(const char *name, const char *mode);
size_t  linux_fread(void *buff, size_t length, size_t cnt, FILE *pfile);
size_t  linux_fwrite(const void *buff, size_t length, size_t cnt, FILE *pfile);

#define   fopen  linux_fopen
#define   fread  linux_fread
#define   fwrite  linux_fwrite
#define   fclose linux_fclose
#define   ftell    linux_ftell 
#define   fseek  linux_fseek

//#define   SEEK_SET   0
//#define   SEEK_END   100000
//#endif //VDEC_EMUVER_FILEIO

#endif

#endif

