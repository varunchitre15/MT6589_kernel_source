
 
/** @file usb_io.h
 */


#ifndef __USB_IO_H__
#define __USB_IO_H__

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
enum FILE_CMD
{
    FILE_OPEN = 0,
    FILE_CLOSE,
    FILE_SEEK,
    FILE_READ,
    FILE_WRITE,
    FILE_CMD_LAST
};

enum ACK_STATUS
{
    ACK_ERROR_ABORT = 0,
    ACK_GET_PARAM_NUM,
    ACK_GET_PARAM,
    ACK_CMD_CONTINUE,
    ACK_CMD_COMPLETE,
    ACK_LAST
};


//-----------------------------------------------------------------------------
// Macro definitions
//-----------------------------------------------------------------------------
#ifndef SEEK_SET
#define	SEEK_SET	0	/* set file offset to offset */
#endif
#ifndef SEEK_CUR
#define	SEEK_CUR	1	/* set file offset to current plus offset */
#endif
#ifndef SEEK_END
#define	SEEK_END	2	/* set file offset to EOF plus offset */
#endif

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


#endif // __USB_IO_H__
