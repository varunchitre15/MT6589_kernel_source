#include "vdec_verify_mpv_prov.h"
#include <mach/mt_typedefs.h>

#ifdef SATA_HDD_FS_SUPPORT

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

BOOL fgHDDFsMount(UINT32 u4InstID)
{
    UINT32 dwDriveNo   = 0;
    UINT32 u4DrvFSTag  = 0;
    INT32 i4_ret;    
    
  ///TODO:  i4_ret = DrvFSMount(dwDriveNo, &u4DrvFSTag);

    if (i4_ret < 0)
    {
      printk("Fs mount fail %d\n", i4_ret);
      ASSERT(0);
      return 0;
    }
    
    return TRUE;
}

BOOL fgHDDFsUnMount (UINT32 u4InstID)
{
///TODO:    DrvFSUnMount();    
    return TRUE;
}


 
struct file *vdecopenFile(char *path,int flag,int mode) 
{ 
struct file *fp; 
 
fp=filp_open(path, flag, 0); 
if (fp) return fp; 
else return NULL; 
} 
 
int vdecreadFile(struct file *fp,char *buf,int readlen) 
{ 
if (fp->f_op && fp->f_op->read) 
return fp->f_op->read(fp,buf,readlen, &fp->f_pos); 
else 
return -1; 
} 
 
int vdeccloseFile(struct file *fp) 
{ 
filp_close(fp,NULL); 
return 0; 
} 
int vdecwriteFile(struct file *fp,char *buf,int writelen)
{
    if (fp->f_op && fp->f_op->write) 
        return fp->f_op->write(fp,buf,writelen,&fp->f_pos); 
    else 
        return -1; 
}

mm_segment_t vdecoldfs; 
void vdecinitKernelEnv(void) 
{ 
vdecoldfs = get_fs(); 
set_fs(KERNEL_DS); 
} 

BOOL fgHDDFsOpenFile(  UINT32 u4InstID,
    CHAR *strFileName,
    INT32 *pi4FileId)
{

    INT32 i4_ret;
    struct file *fp; 

     fp=vdecopenFile((char*)strFileName,O_RDONLY,0); 
     if (fp == 0xfffffffe)
     {
         printk("Fs open file fail %d\n", fp);
         return FALSE;
     }
     *pi4FileId = fp;
     
     //vdeccloseFile(fp); 
     return TRUE;
}

BOOL fgHDDFsCloseFile(  UINT32 i4FileId)
{
///TODO:   DrvFSCloseFile(i4FileId);
   vdeccloseFile((struct file *)i4FileId); 

   return TRUE;
}

BOOL fgHDDFsWriteFile(CHAR *strFileName,
	void* pvAddr,
	UINT32 u4Length)
{

    INT32 i4_ret;
    struct file *fp; 
    UINT32 u4FileSize = 0;

     fp=vdecopenFile((char*)strFileName,O_CREAT|O_WRONLY,0); 
     if (fp == 0xfffffffe)
     {
         printk("Fs open file fail %d\n", fp);
         return FALSE;
     }

     u4FileSize = (UINT32)fp->f_op->llseek(fp, 0, SEEK_END);
      if (u4FileSize < 0)
      {
           vdeccloseFile(fp); 
        printk("Fs get file size fail %d\n", u4FileSize);
         return FALSE;
      }
     
        if(fp->f_op->llseek(fp, u4FileSize, SEEK_SET) != u4FileSize)
      {
          vdeccloseFile(fp); 
         printk(" seek fail: %s\n", strFileName);
         return FALSE;
      }
     
    vdecinitKernelEnv(); 
    
    i4_ret = fp->f_op->write(fp, pvAddr, u4Length, &fp->f_pos);
   //  printk("u4Length(%d), write size(%d), filesize(%d)\n ", u4Length, i4_ret, u4FileSize);
   set_fs(vdecoldfs); 
    if(i4_ret < 0)
    {
   vdeccloseFile(fp); 
          printk("write file Fail!\n");
          return FALSE;
    }

   vdeccloseFile(fp); 
    return TRUE;
    
}



BOOL fgHDDFsReadFile(UINT32 u4InstID,
    CHAR *strFileName,
    void* pvAddr,
    UINT32 u4Offset,
    UINT32 u4Length,
    UINT32 *pu4RealReadSize,
    UINT32 *pu4TotalFileLength,
    INT32 *pi4FileId)
{

    INT32 i4_ret;
    struct file *fp; 
    UINT32 u4FileSize = 0;
    UINT32 u4ReadSize = 0;

  //   printk("%s", strFileName);
     fp=vdecopenFile((char*)strFileName,O_RDONLY,0); 
     if (fp == 0xfffffffe)
     {
         printk("Fs open file fail %d\n", fp);
         return FALSE;
     }

     *pi4FileId = fp;
    
  //        printk(" fp(%x)\n", fp);
        u4FileSize = (UINT32)fp->f_op->llseek(fp, 0, SEEK_END);
//         printk("Fs get file size %d\n", u4FileSize);

      if (u4FileSize <= 0)
      {
         printk("Fs get file size fail %d\n", u4FileSize);
         return FALSE;
      }
      
      *pu4TotalFileLength = u4FileSize;             

      if (u4Offset >= u4FileSize)
      {
          vdeccloseFile(fp); 
          printk(" read offset(%d) > filesize(%d)\n", u4Offset, u4FileSize);
          return FALSE;
      }

        if(fp->f_op->llseek(fp, u4Offset, SEEK_SET) != u4Offset)
      {
          vdeccloseFile(fp); 
         printk(" seek fail: %s\n", strFileName);
         return FALSE;
      }

    //          printk("u4Length(%d), read offset(%d), filesize(%d)\n ", u4Length, u4Offset, u4FileSize);
    if (u4Length >= (u4FileSize - u4Offset))
        u4ReadSize = u4FileSize - u4Offset;
	else
	{
		printk("Warning =====>File size is larger than VFIFO size! 0x%x Byte will be read\n",V_FIFO_SZ);
        u4ReadSize = u4Length;//V_FIFO_SZ;
        *pu4TotalFileLength = u4Length;//V_FIFO_SZ;
	}
   //           printk(" u4ReadSize(%d), filesize(%d), pvAddr(0x%x)\n", u4ReadSize, u4FileSize, pvAddr);

//    memset(pvAddr ,0, u4Length);
    vdecinitKernelEnv(); 
    i4_ret=vdecreadFile(fp, pvAddr , u4ReadSize);
// i4_ret = kernel_read(fp, 0, pvAddr, u4ReadSize);
   set_fs(vdecoldfs); 

    if(i4_ret < 0)
    {
          vdeccloseFile(fp); 
          printk("read file Fail!\n");
          return FALSE;
    }

   *pu4RealReadSize = i4_ret;
   vdeccloseFile(fp); 

    if (u4ReadSize != *pu4RealReadSize)
    {
        printk("\n read fail: %s", strFileName);
        return FALSE;
    }

    return TRUE;
    
}

UINT32 u4HDDFsGetFileSize(INT32 *pi4FileId)
{
    struct file *fp = (struct file *)(*pi4FileId);

    return ((UINT32)fp->f_op->llseek(fp, 0, SEEK_END));
}

#endif


