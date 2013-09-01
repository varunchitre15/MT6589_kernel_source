
 
/** @file sata_fs_io.h
 */


#ifndef SATA_FS_IO_H
#define SATA_FS_IO_H


//#include "x_common.h"
//-----------------------------------------------------------------------------
// Include files
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Configurations
//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------
// Constant definitions
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Type definitions
//-----------------------------------------------------------------------------




//-----------------------------------------------------------------------------
// Macro definitions
//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------
// Prototype  of inter-file functions
//-----------------------------------------------------------------------------
BOOL fgHDDFsMount (UINT32 u4InstID);
BOOL fgHDDFsUnMount (UINT32 u4InstID);

BOOL fgHDDFsOpenFile(  UINT32 u4InstID,
    CHAR *strFileName,
    INT32* pi4FileId);

BOOL fgHDDFsCloseFile(  UINT32 i4FileId);
BOOL fgHDDFsReadFile(  UINT32 u4InstID,
    CHAR *strFileName,
    void* pvAddr,
    UINT32 u4Offset,
    UINT32 u4Length,
    UINT32 *pu4RealReadSize,
    UINT32 *pu4TotalFileLength,
    INT32* pi4FileId);
BOOL fgHDDFsWriteFile(CHAR *strFileName,
	void* pvAddr,
	UINT32 u4Length);

UINT32 u4HDDFsGetFileSize(INT32 *pi4FileId);
int vdecopenFile(char *path,int flag,int mode);
int vdecwriteFile(int fp,char *buf,int writelen);
int vdeccloseFile(int fp);


#endif  // SATA_FS_IO_H
