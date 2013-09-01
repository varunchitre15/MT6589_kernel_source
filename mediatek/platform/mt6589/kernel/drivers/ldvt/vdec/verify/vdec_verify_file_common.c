#include "vdec_verify_file_common.h"
//#include "vdec_ide.h"
#include "vdec_verify_mpv_prov.h"
#ifdef SATA_HDD_FS_SUPPORT
#include "sata_fs_io.h"
//#include "usb_io.h"
#endif

#include <linux/string.h>
#include <mach/mt_typedefs.h>
#if (!CONFIG_DRV_LINUX)
#include <stdio.h>
#include <string.h>
#include "x_printf.h"
#else

#ifndef SEEK_SET
#define	SEEK_SET	0	/* set file offset to offset */
#endif
#ifndef SEEK_CUR
#define	SEEK_CUR	1	/* set file offset to current plus offset */
#endif
#ifndef SEEK_END
#define	SEEK_END	2	/* set file offset to EOF plus offset */
#endif

#if 0//ndef VDEC_EMUVER_FILEIO

#include "x_os.h"                       //For <string.h>
#include <mach/mt6575_typedefs.h>
#include "x_printf.h"
#include "x_debug.h"

#else //VDEC_EMUVER_FILEIO

FILE*   linux_fopen(const char *name, const char *mode)
{
    return 0;
}

size_t  linux_fread(void *buff, size_t length, size_t cnt, FILE *pfile)
{
    return 0;
}

size_t  linux_fwrite(const void *buff, size_t length, size_t cnt, FILE *pfile)
{
   return 0;
}

UINT32   linux_fclose(FILE *pfile)
{
   return 0;
}


UINT32 linux_ftell (FILE *pfile)
{
   return 0;
}

UINT32  linux_fseek(FILE *pfile, long offset, int cnt)
{
   return 0;
}
#endif //VDEC_EMUVER_FILEIO
#endif


void vVDecOutputDebugString(const CHAR * format, ...)
{
#ifdef IDE_WRITE_SUPPORT
  return;
#else

   #if (!CONFIG_DRV_LINUX)
       CHAR szTmpStr[300] = {0};
       va_list argptr;
       va_start(argptr, format);
       vsprintf(szTmpStr, format, argptr);
       szTmpStr[299] = '\0';
       va_end(argptr);
       printk("%s", szTmpStr);
    #endif
    
#endif
}

UINT32 u4fsize(FILE* pFile)
{
    UINT32 u4Result = 0;
    UINT32 u4Offset = 0;

    if(pFile == NULL)
        return u4Result;

    u4Offset = linux_ftell(pFile);
    linux_fseek(pFile, 0, SEEK_END);
    u4Result = linux_ftell(pFile);
    linux_fseek(pFile, u4Offset, SEEK_SET);
    return u4Result;
}

// *********************************************************************
// Function    : void vErrMessage(UINT32 u4InstID, UCHAR *pbDramAddr)
// Description : print error message in record file
// Parameter   : None
// Return      : None
// *********************************************************************
void vErrMessage(UINT32 u4InstID, CHAR *pbDramAddr)
{

}

// *********************************************************************
// Function    : void vDataFromFile(UINT32 u4InstID, UINT32 *pu4W, UINT32 *pu4H)
// Description : print error message in record file
// Parameter   : None
// Return      : None
// *********************************************************************
void vDataFromFile(UINT32 u4InstID, UINT32 *pu4W, UINT32 *pu4H)
{
    FILE* pFile = NULL;
    UINT32 u4Temp;
    UINT32 u4FileSize;

    u4Temp = sprintf(_bTempStr1[u4InstID], "%s", _bFileStr1[u4InstID][0]);
    u4Temp += sprintf(_bTempStr1[u4InstID] + u4Temp, "%s", "width.txt");
    pFile = linux_fopen(_bTempStr1[u4InstID], "rb");
    u4FileSize = u4fsize(pFile);
    if(pFile)
    {
        //fgfileread(u4FILE, (UINT32)pu4W, u4FileSize);
        linux_fread(pu4W, 1, u4FileSize, pFile);
        linux_fclose(pFile);
    }
    else
    {
        sprintf(_bTempStr1[u4InstID], "%s", "No W\\n\\0");
        vErrMessage(u4InstID, (CHAR *)_bTempStr1[u4InstID]);
    }


    u4Temp = sprintf(_bTempStr1[u4InstID], "%s", _bFileStr1[u4InstID][0]);
    u4Temp += sprintf(_bTempStr1[u4InstID] + u4Temp, "%s", "height.txt");  
    pFile = linux_fopen(_bTempStr1[u4InstID], "rb");
    u4FileSize = u4fsize(pFile);
    if(pFile)
    {
        //fgfileread(u4FILE, (UINT32)pu4H, u4FileSize);
        linux_fread(pu4H, 1, u4FileSize, pFile);
        linux_fclose(pFile);
    }
    else
    {
        sprintf(_bTempStr1[u4InstID], "%s", "No W\\n\\0");
        vErrMessage(u4InstID, (CHAR *)_bTempStr1[u4InstID]);
    }
}

// *********************************************************************
// Function    : BOOL fgReadPCFile(void* pvAddr, UINT32 u4Size, char *strFileName, UINT32 u4OffSet, UINT32 *pu4RealReadSize)
// Description : read bitstream from pc to memory
// Parameter   : None
// Return      : TRUE/FALSE
// *********************************************************************
BOOL fgReadPCFile(void* pvAddr, UINT32 u4Size, char *strFileName, UINT32 u4OffSet, UINT32 *pu4RealReadSize, UINT32 *pu4FileLength)
{
    FILE* pFile = NULL;
    UINT32 u4FileSize = 0;
    UINT32 u4ReadSize = u4Size;
    if ((NULL == pvAddr) || (0 == u4Size) || (NULL == strFileName) || (NULL == pu4RealReadSize))
    return FALSE;

    *pu4RealReadSize = 0;

    if((pFile = linux_fopen(strFileName, "rb")) == NULL)
        return FALSE;
    
    u4FileSize = u4fsize(pFile);
    *pu4FileLength = u4FileSize;

    if (u4OffSet >= u4FileSize)
    {
        linux_fclose(pFile);
        vVDecOutputDebugString("\n read offset(%d) > filesize(%d)", u4OffSet, u4FileSize);
        return FALSE;
    }

    if(linux_fseek (pFile, u4OffSet, SEEK_SET))
    {
        linux_fclose(pFile);
        vVDecOutputDebugString("\n seek fail: %s", strFileName);
        return FALSE;
    }

    if (u4Size > (u4FileSize - u4OffSet))
        u4ReadSize = u4FileSize - u4OffSet;

    *pu4RealReadSize = linux_fread((void *)pvAddr, 1, u4ReadSize, pFile);
    linux_fclose(pFile);

    if (u4ReadSize != *pu4RealReadSize)
    {
        vVDecOutputDebugString("\n read fail: %s", strFileName);
        return FALSE;
    }

    return TRUE;
}



BOOL fgReadHDDFile(UINT32 u4InstID, BOOL fgReadInfo,  void* pvAddr, UINT32 u4Size, char *strFileName, UINT32 u4OffSet, UINT32 *pu4RealReadSize, UINT32 *pu4FileLength, INT32* pi4FileId)
{
#ifdef  SATA_HDD_READ_SUPPORT
#ifndef  SATA_HDD_FS_SUPPORT

    FILE* pFile = NULL;
    UINT32 u4FILE = 0;
    UINT32 u4FileSize = 0;

    if ((NULL == pvAddr) || (0 == u4Size) || (NULL == strFileName) || (NULL == pu4RealReadSize))
        return FALSE;

    *pu4RealReadSize = 0;

    if(0)// '\0' != _bFileStr1[_u4VDecID][9][0])
    {
        //u4FILE = u4fOpenFile(_bFileStr1[_u4VDecID][9], 1/*"rb"*/, &u4FileSize);
    }
    else
    {
        //u4FILE = u4fOpenFile(strFileName, 1/*"rb"*/, &u4FileSize);
    }

     
   if (fgReadInfo)
   {
    u4FILE = i4ReadFileFormHDD(
        u4InstID,
        strFileName,
        pvAddr,
        u4OffSet,
        u4Size,
        pu4RealReadSize,
        pu4FileLength
    );
    }
    else
    {
        if((pFile = linux_fopen(strFileName, "rb")) == NULL)
        {
            return FALSE;
        }
        else
        {
            linux_fclose(pFile);
            return TRUE;
        }
    }

    if (0 != u4FILE)
        return FALSE;

    if (!((u4Size == *pu4RealReadSize) || ((*pu4FileLength - u4OffSet) == *pu4RealReadSize)))
    {
        vVDecOutputDebugString("\n read fail: %s", strFileName);
        return FALSE;
    }
#else  
    BOOL fgRet;

    if ((NULL == pvAddr) || (0 == u4Size) || (NULL == strFileName) || (NULL == pu4RealReadSize))
        return FALSE;

    *pu4RealReadSize = 0;

    if (fgReadInfo)
    {

      fgRet = fgHDDFsReadFile(
           u4InstID,
           strFileName,
           pvAddr,
           u4OffSet,
           u4Size,
           pu4RealReadSize,
           pu4FileLength,
           pi4FileId
       );

       if (!fgRet)
         return FALSE;

        if (!((u4Size == *pu4RealReadSize) || ((*pu4FileLength - u4OffSet) == *pu4RealReadSize)))
        {
           vVDecOutputDebugString("\n read fail: %s", strFileName);
           return FALSE;
        }

    }
    else
    {
       fgRet = fgHDDFsOpenFile(
           u4InstID,
           strFileName,
           pi4FileId
       );

       if (fgRet)
       {
          if (pu4FileLength)
          {
              *pu4FileLength = u4HDDFsGetFileSize(pi4FileId);
          }
          fgHDDFsCloseFile(*pi4FileId);
       }
       else
       {
         return FALSE;
       }
        
    }
    
    return TRUE;
#endif

#else
   return FALSE;
#endif   
}

BOOL fgOpenHDDFileEx(UINT32 u4InstID, char *strFileName , char *strMode, VDEC_INFO_VERIFY_FILE_INFO_T *ptFileInfo, const char *szFile, INT32 i4Line)
{ 
    UINT32 u4Offset;

    #ifdef RING_VFIFO_SUPPORT
    u4Offset = ptFileInfo->u4FileOffset;
    #else
    u4Offset = 0;
    #endif
    
        printk("<vdec> OpenFile: %s @(%s, %d)\n", strFileName, szFile, i4Line);
        strncpy(ptFileInfo->bFileName,strFileName, sizeof(strFileName)); 
        if(fgReadHDDFile(u4InstID, ptFileInfo->fgGetFileInfo, ptFileInfo->pucTargetAddr, ptFileInfo->u4TargetSz, strFileName, u4Offset, &(ptFileInfo->u4RealGetBytes), &(ptFileInfo->u4FileLength), &(ptFileInfo->i4FileId)))
        {
            //vVDecOutputDebugString("The file's size is 0x%.8x bytes\n", ptFileInfo->u4FileLength);
            return TRUE;
        }
        else
        {
            //vVDecOutputDebugString("\n NULL \n");
            printk("<vdec> %s(%s) return FALSE!!!!! @( %s, %d)\n", __FUNCTION__, strFileName, szFile, i4Line);
            return FALSE;
        }    
}

// *********************************************************************
// Function    : BOOL fgWrMsg2PC(void* pvAddr, UINT32 u4Size, UINT32 u4Mode, VDEC_INFO_VERIFY_FILE_INFO_T *pFILE_INFO)
// Description : Write the decoded data to PC for compare
// Parameter   : None
// Return      : None
// *********************************************************************
BOOL fgWrMsg2PC(void* pvAddr, UINT32 u4Size, UINT32 u4Mode, VDEC_INFO_VERIFY_FILE_INFO_T *pFILE_INFO)
{
#ifdef IDE_WRITE_SUPPORT
  #if (!CONFIG_DRV_LINUX)
    printk("%s\n",pvAddr);
  #endif
  return TRUE;
#else

    BOOL fgResult = FALSE;

    #ifdef PCFILE_WRITE
    FILE* pFile = NULL;

    if ((NULL == pvAddr) || (0 == u4Size))
        return FALSE;

    if(pFILE_INFO->pFile == NULL)
    {
        if(pFILE_INFO->bFileName == NULL)
            return FALSE;
        else
            pFILE_INFO->pFile = linux_fopen(pFILE_INFO->bFileName, "wb");
    }
    pFile = pFILE_INFO->pFile;
    
    if(pFile == NULL)
        return FALSE;

    fgResult = (linux_fwrite ((char* )pvAddr, 1, u4Size, pFile) == u4Size);
    if (FALSE == fgResult)
    {
        vVDecOutputDebugString("\n write fail: %s", pFILE_INFO->bFileName);
        linux_fclose(pFILE_INFO->pFile);
    }
    //fclose(pFile);

    #else
      #if (!CONFIG_DRV_LINUX)
         printk("%s", (char* )pvAddr);
      #endif
         printk("%s", (char* )pvAddr);
    fgResult = TRUE;
    
    #endif

    return fgResult;
#endif
}

BOOL fgOverWrData2PC(void* pvAddr, UINT32 u4Size, UINT32 u4Mode, char *strFileName)
{
#ifdef IDE_WRITE_SUPPORT
  #if (!CONFIG_DRV_LINUX)
     printk("%s",pvAddr);
  #endif
  return TRUE;
#else
    //printk("file name:%s\n",strFileName);
    //printk("addr:%s, size:0x%x\n",pvAddr, u4Size);

    fgHDDFsWriteFile(strFileName, pvAddr, u4Size);
    return TRUE;
#endif
}

BOOL fgWrData2PC(void* pvAddr, UINT32 u4Size, UINT32 u4Mode, char *strFileName)
{
    #ifdef IDE_WRITE_SUPPORT
    fgIdeWrData2PC(pvAddr,u4Size, u4Mode,strFileName);
    #else
    fgOverWrData2PC(pvAddr,u4Size, u4Mode,strFileName);
    #endif

    return TRUE;
}

BOOL fgOpenPCFile(UINT32 u4InstID, char *strFileName , char *strMode, VDEC_INFO_VERIFY_FILE_INFO_T *ptFileInfo)
{
    FILE *pFile = NULL;
    UINT32 u4Offset;

    #ifdef RING_VFIFO_SUPPORT
    u4Offset = ptFileInfo->u4FileOffset;
    #else
     u4Offset = 0;
    #endif

    if(ptFileInfo->fgGetFileInfo)
    {
        strcpy(ptFileInfo->bFileName,strFileName);   
        if(fgReadPCFile(ptFileInfo->pucTargetAddr, ptFileInfo->u4TargetSz, strFileName, u4Offset, &(ptFileInfo->u4RealGetBytes), &(ptFileInfo->u4FileLength)))
        {
            //vVDecOutputDebugString("The file's size is 0x%.8x bytes\n", ptFileInfo->u4FileLength);
            if(ptFileInfo->u4FileLength > ptFileInfo->u4RealGetBytes)
            {
                vVDecOutputDebugString("\n Over scan\n");
            }
            return TRUE;
        }
        else
        {
            vVDecOutputDebugString("\n NULL \n");
            return FALSE;
        }
    }
    else
    {
        if((pFile = linux_fopen(strFileName, "rb")) == NULL)
        {
            return FALSE;
        }
        else
        {
            linux_fclose(pFile);
            return TRUE;
        }
    }

}


// *********************************************************************
// Function    : void fgOpenFile(void)
// Description : Set related files pointer
// Parameter   : None
// Return      : None
// *********************************************************************
BOOL fgOpenFileEx(UINT32 u4InstID, char *strFileName , char *strMode, VDEC_INFO_VERIFY_FILE_INFO_T *ptFileInfo, const char *szFunction, INT32 i4Line)
{
    BOOL fgOpen = TRUE;
    
    #ifdef  SATA_HDD_READ_SUPPORT
          fgOpen = fgOpenHDDFileEx(u4InstID, strFileName, strMode, ptFileInfo, szFunction, i4Line);
      #ifndef  SATA_HDD_FS_SUPPORT
          if(fgOpen == FALSE)
          {
                fgOpen = fgOpenPCFile(u4InstID, strFileName, strMode, ptFileInfo);
          }
      #endif
    #elif defined(IDE_READ_SUPPORT)
          fgOpen = fgOpenIdeFile(strFileName, strMode, ptFileInfo);
          if(fgOpen == FALSE)
          {
              fgOpen = fgOpenPCFile(u4InstID, strFileName, strMode, ptFileInfo);
          }
          
     #else
          fgOpen = fgOpenPCFile(u4InstID, strFileName, strMode, ptFileInfo);
     #endif

     return fgOpen;
}


#ifdef IDE_WRITE_SUPPORT
BOOL fgIdeWrData2PC(void* pvAddr, UINT32 u4Size, UINT32 u4Mode, char *strFileName)
{
    _DmxIdeSendName(strFileName);
    _DmxIdeSendData((UINT32)pvAddr, u4Mode, u4Size);
    return TRUE;
}
#endif


#ifdef IDE_READ_SUPPORT
BOOL fgReadIdeFile(void* pvAddr, UINT32 u4Size, char *strFileName, UINT32 u4OffSet, UINT32 *pu4RealReadSize, UINT32 *pu4FileLength)
{
    if ((NULL == pvAddr) || (0 == u4Size) || (NULL == strFileName) || (NULL == pu4RealReadSize))
        return FALSE;

    *pu4RealReadSize = 0;

    _DmxIdeSendName(strFileName);
    
#if !CONFIG_DRV_LINUX
    if(!_DmxIdeGetFile((UINT32)pvAddr, 0, pu4RealReadSize))
    {
        vVDecOutputDebugString("\n read fail: %s", strFileName);
        return FALSE;
    }
#else
    if(!_DmxIdeGetFile( PHYSICAL((UINT32)pvAddr), 0, pu4RealReadSize, u4Size))
    {
        vVDecOutputDebugString("\n read fail: %s", strFileName);
        return FALSE;
    }
#endif

    *pu4FileLength = *pu4RealReadSize;

    return TRUE;
}

BOOL fgPureOpenIdeFile(char *strFileName , char *strMode, VDEC_INFO_VERIFY_FILE_INFO_T *ptFileInfo)
{
    _DmxIdeSendName(strFileName);
    return(_DmxIdeOpenFile());
}

BOOL fgOpenIdeFile(char *strFileName , char *strMode, VDEC_INFO_VERIFY_FILE_INFO_T *ptFileInfo)
{
    FILE* pFile = NULL;
    UINT32 u4Offset;

    #ifdef RING_VFIFO_SUPPORT
    u4Offset = ptFileInfo->u4FileOffset;
    #else
    u4Offset = 0;
    #endif

    if(ptFileInfo->fgGetFileInfo)
    {
        strcpy(ptFileInfo->bFileName,strFileName); 
        if(fgReadIdeFile(ptFileInfo->pucTargetAddr, ptFileInfo->u4TargetSz, strFileName, u4Offset, &(ptFileInfo->u4RealGetBytes), &(ptFileInfo->u4FileLength)))
        { 
            return TRUE;
        }
        else
        {
            vVDecOutputDebugString("\n NULL \n");
            return FALSE;
        }
    }
    else
    {
        if((pFile = linux_fopen(strFileName, "rb")) == NULL)
        {
            return FALSE;
        }
        else
        {
            linux_fclose(pFile);
            return TRUE;
        }
    }
}
#endif

