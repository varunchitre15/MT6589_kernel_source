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
#include <linux/termios.h>
#include "usb_io.h"

//#define USB_ACM_DMA1
#ifdef USB_ACM_DMA1
#define SERIAL_BUFFER_SZ    40*1024
#else
#define SERIAL_BUFFER_SZ    2*1024
#endif

static struct termios settings; 
static struct file *fd = 0;

mm_segment_t vdecoldfs;
void vdecinitKernelEnv(void) 
{ 
    vdecoldfs = get_fs(); 
    set_fs(KERNEL_DS); 
} 


/**
 * Open a file in host pc through USB, similar to fopen()
 * @param path local file path on host pc
 * @param flag open file flag used in fopen()
 * @param mode 0
 * @return -1 if open failed
 *         0-15 as the file descriptor number
 */
int vdecopenFile(char *path,int flag,int mode) 
{ 
    int num = 0;
    int cmd = 0;
    int ack = 0;
    int fdnum = 0;
    char fname[512];
    char fmode[8];

    //printk("[USBIO] =====> Open file %s <=====\n", path);
    
    if (NULL == fd)
        return FALSE;

    // strFileName size is 300, _bFileStr1 or _bTempStr1
    memcpy(fname, path, 300);
    fname[300] = '\0';

    if ((flag & 0x11) == O_RDONLY) {
        memcpy(fmode, "rb", 3);
    } else if ((flag & 0x11) == O_WRONLY) {
        memcpy(fmode, "ab", 3);
    } else if ((flag & 0x11) == O_RDWR) {
        memcpy(fmode, "a+b", 4);
    } else {
        memcpy(fmode, "rb", 3);
    }

    //vdecinitKernelEnv();        
    cmd = FILE_OPEN;
    num = fd->f_op->write(fd,(const void *)&cmd, sizeof(int), &fd->f_pos);
    if (num != sizeof(int)) printk("[USBIO][OPEN] E1\n");

    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_GET_PARAM_NUM) printk("[USBIO][OPEN] E1-A\n");

    cmd = 2;
    num = fd->f_op->write(fd,(const void *)&cmd, sizeof(int), &fd->f_pos);
    if (num != sizeof(int)) printk("[USBIO][OPEN] E2\n");
    
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_GET_PARAM) printk("[USBIO][OPEN] E2-A\n");

    num = fd->f_op->write(fd,(const void *)fname, 511, &fd->f_pos);
    if (num != 511) printk("[USBIO][OPEN] E3\n");
    
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_GET_PARAM) printk("[USBIO][OPEN] E3-A\n");

    num = fd->f_op->write(fd,(const void *)fmode, 7, &fd->f_pos);
    if (num != 7) printk("[USBIO][OPEN] E4\n");

    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_CMD_COMPLETE) printk("[USBIO][OPEN] E4-A\n");
    
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int)) printk("[USBIO][OPEN] E4-B\n");
    if (ack == -1)
    {    
        num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
        //set_fs(vdecoldfs);
        return -1;
    }
    
    ack = ACK_GET_PARAM_NUM;
    num = fd->f_op->write(fd,(const void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int)) printk("[USBIO][OPEN] E5\n");
    num = fd->f_op->read(fd,(void *)&cmd, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || cmd != 1) printk("[USBIO][OPEN] E5-A\n");

    ack = ACK_GET_PARAM;
    num = fd->f_op->write(fd,(const void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int)) printk("[USBIO][OPEN] E6\n");
    num = fd->f_op->read(fd,(void *)&fdnum, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || fdnum < 0) printk("[USBIO][OPEN] E6-A\n");    
    
    ack = ACK_CMD_COMPLETE;
    num = fd->f_op->write(fd,(const void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int)) printk("[USBIO][OPEN] E7\n");

    //set_fs(vdecoldfs);
    return fdnum;
} 

/**
 * Move position pointer in file, similar to fseek()
 * @param fp file descriptor number
 * @param offset seeking offset
 * @param mode seeking origin SEEK_SET/SEEK_CUR/SEEK_END
 * @return offset from beginning of file
 */
int vdecseekFile(int fp, int offset, int origin)
{
    int num = 0;
    int cmd = 0;
    int ack = 0;
    int fpos = 0;
    
    //printk("[USBIO] =====> Seek file fp %d, offset %d, origin %d <=====\n", fp, offset, origin);
    
    if (NULL == fd)
        return -1;

    //vdecinitKernelEnv();
    // Send seek command
    cmd = FILE_SEEK;
    num = fd->f_op->write(fd,(const void *)&cmd, sizeof(int), &fd->f_pos);
    if (num != sizeof(int))
        printk("[USBIO][SEEK] E1\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_GET_PARAM_NUM)
        printk("[USBIO][SEEK] E1-A\n");

    // Send number of parameters: 3
    cmd = 3;
    num = fd->f_op->write(fd,(const void *)&cmd, sizeof(int), &fd->f_pos);
    if (num != sizeof(int))
        printk("[USBIO][SEEK] E2\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_GET_PARAM)
        printk("[USBIO][SEEK] E2-A\n");

    // Send parameter 1: file id
    num = fd->f_op->write(fd,(const void *)&fp, sizeof(int), &fd->f_pos);
    if (num != sizeof(int))
        printk("[USBIO][SEEK] E3\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_GET_PARAM)
        printk("[USBIO][SEEK] E3-A\n");

    // Send parameter 2: offset
    num = fd->f_op->write(fd,(const void *)&offset, sizeof(int), &fd->f_pos);
    if (num != sizeof(int))
        printk("[USBIO][SEEK] E4\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_GET_PARAM)
        printk("[USBIO][SEEK] E4-A\n");

    // Send parameter 3: origin
    num = fd->f_op->write(fd,(const void *)&origin, sizeof(int), &fd->f_pos);
    if (num != sizeof(int))
        printk("[USBIO][SEEK] E5\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_CMD_COMPLETE)
        printk("[USBIO][SEEK] E5-A\n");

    // Get response, 0 = success, -1 = fail
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int))
        printk("[USBIO][SEEK] E5-B\n");
    if (ack == -1)
    {    
        num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
        //set_fs(vdecoldfs);
        return -1;
    }

    // Get number of parameters, should get 1
    ack = ACK_GET_PARAM_NUM;
    num = fd->f_op->write(fd,(const void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int))
        printk("[USBIO][SEEK] E6\n");
    num = fd->f_op->read(fd,(void *)&cmd, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || cmd != 1)
        printk("[USBIO][SEEK] E6-A\n");

    // Get parameter 1: current offset from beginning of file
    ack = ACK_GET_PARAM;
    num = fd->f_op->write(fd,(const void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int))
        printk("[USBIO][SEEK] E7\n");
    num = fd->f_op->read(fd,(void *)&fpos, sizeof(int), &fd->f_pos);
    if (num != sizeof(int))
        printk("[USBIO][SEEK] E7-A\n");

    // Send complete command
    ack = ACK_CMD_COMPLETE;
    num = fd->f_op->write(fd,(const void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int))
        printk("[USBIO][SEEK] E8\n");
    
    //set_fs(vdecoldfs);
    return fpos;
}

 /**
  * Read from a file in host pc through USB, similar to fread()
  * @param fp file descriptor number
  * @param buf buffer to store the file content
  * @param readlen number of bytes to read
  * @return actual bytes read
  */
int vdecreadFile(int fp,char *buf,int readlen)
{
#ifdef NO_ACK_TRANSFER
    int opRes = 0;
    int cmd = 0;
    int resp = 0;
    int lenRemain = readlen;
    int lenRecv = 0;
    long usec = 0;
    int tmp = 0;

    //printk("[USBIO] =====> Read file fp %d, buf 0x%x, readlen %d <=====\n", fp, buf, readlen);

    //vdecinitKernelEnv();     
     // Send command
    cmd = FILE_READ;
    opRes = fd->f_op->write(fd,(const void *)&cmd, sizeof(int), &fd->f_pos);
    if (op_res != sizeof(int)) printk("[READ] Command FILE_READ send fail %d\n", op_res);

    // Send file descriptor index
    opRes = fd->f_op->write(fd,(void *)&fp, sizeof(int), &fd->f_pos);
    if (op_res != sizeof(int)) printk("[READ] File descriptor index send fail %d\n", op_res);

    // Send read data length
    opRes = fd->f_op->write(fd,(void *)&readlen, sizeof(int), &fd->f_pos);

    // Get response
    opRes = fd->f_op->read(fd,(void *)&resp, sizeof(int), &fd->f_pos);
     
    if (0 != resp)
    {
        printk("[READ] Read operation failed at remote %d\n", resp);
        //set_fs(vdecoldfs);
        return -1;
    }
     
     // Get data if no error
    while (lenRemain > 0)
    {
        lenRecv = (lenRemain > SERIAL_BUFFER_SZ)?SERIAL_BUFFER_SZ:lenRemain;
        opRes = fd->f_op->read(fd,(void *)buf, lenRecv, &fd->f_pos);
        if (opRes != lenRecv)
        {
            printk("[READ] Read data %d-%d bytes failed, only read %d\n", len-len_remain, len-len_remain+len_recv, op_res);
            break; //return (len-len_remain+op_res);
        }

        //tmp = fd->f_op->write(fd,(void *)&tmp, sizeof(int), &fd->f_pos);
        buf += opRes;
        lenRemain -= opRes;
    }

    //set_fs(vdecoldfs);
    return (opRes != lenRecv)?(readlen-lenRemain+opRes):(readlen-lenRemain);
#else
    int num = 0;
    int cmd = 0;
    int ack = 0;
    int fpos = 0;
    int bytesReadTotal = 0;
    int bytesToRead = 0;
    int bytesRead = 0;
    char *bufCur = buf;

    printk("[USBIO] =====> Read file fp %d, buf 0x%x, readlen %d <=====\n", fp, buf, readlen);

    //vdecinitKernelEnv();
    // Send CMD FILE_READ
    cmd = FILE_READ;
    num = fd->f_op->write(fd,(const void *)&cmd, sizeof(int), &fd->f_pos);
    if (num != sizeof(int)) printk("[USBIO][READ] E1\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_GET_PARAM_NUM) printk("[USBIO][READ] E1-A\n");

    // Send int 2
    cmd = 2;
    num = fd->f_op->write(fd,(const void *)&cmd, sizeof(int), &fd->f_pos);
    if (num != sizeof(int)) printk("[USBIO][READ] E2\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_GET_PARAM) printk("[USBIO][READ] E2-A\n");

    // Send int fp
    num = fd->f_op->write(fd,(const void *)&fp, sizeof(int), &fd->f_pos);
    if (num != sizeof(int)) printk("[USBIO][READ] E3\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_GET_PARAM) printk("[USBIO][READ] E3-A\n");

    // Send int readlen
    num = fd->f_op->write(fd,(const void *)&readlen, sizeof(int), &fd->f_pos);
    if (num != sizeof(int)) printk("[USBIO][READ] E3\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);    
    if (num != sizeof(int) || ack != ACK_CMD_COMPLETE) printk("[USBIO][READ] E3-A\n");
    
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);    
    if (num != sizeof(int)) printk("[USBIO][READ] E3-B\n");
    if (ack == -1)
    {    
        num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
        //set_fs(vdecoldfs);
        return FALSE;
    }

    // Get int 2
    ack = ACK_GET_PARAM_NUM;
    num = fd->f_op->write(fd,(const void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int)) printk("[USBIO][READ] E5\n");
    num = fd->f_op->read(fd,(void *)&cmd, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || cmd != 2) printk("[USBIO][READ] E5-A\n");

    ack = ACK_GET_PARAM;
    num = fd->f_op->write(fd,(const void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int)) printk("[USBIO][READ] E6\n");

    while (bytesRead < readlen)
    {
        // Get bytes to read
        num = fd->f_op->read(fd,(void *)&bytesToRead, sizeof(int), &fd->f_pos);
        if (num != sizeof(int) || bytesToRead < 0) printk("[USBIO][READ] E6-A\n");
        if (0 == bytesToRead)
        {
            ack = ACK_CMD_COMPLETE;
            num = fd->f_op->write(fd,(const void *)&ack, sizeof(int), &fd->f_pos);
            if (num != sizeof(int)) printk("[USBIO][READ] E6-B\n");
            break;
        }

        // Get data        
        ack = ACK_GET_PARAM;
        num = fd->f_op->write(fd,(const void *)&ack, sizeof(int), &fd->f_pos);
        if (num != sizeof(int)) printk("[USBIO][READ] E7\n");        
        num = fd->f_op->read(fd,(void *)bufCur, bytesToRead, &fd->f_pos);
        if (num != bytesToRead) printk("[USBIO][READ] E7-A\n");

        bufCur += num;
        bytesRead += num;
        //printk("bytes read %d\n", bytesRead);
        if (bytesRead < readlen)
        {
            ack = ACK_CMD_CONTINUE;
            num = fd->f_op->write(fd,(const void *)&ack, sizeof(int), &fd->f_pos);
            if (num != sizeof(int)) printk("[USBIO][READ] E8-A\n");
        }
        else
        {
            ack = ACK_CMD_COMPLETE;
            num = fd->f_op->write(fd,(const void *)&ack, sizeof(int), &fd->f_pos);
            if (num != sizeof(int)) printk("[USBIO][READ] E8-B\n");
        }
    }

    //set_fs(vdecoldfs);
    return bytesRead;
#endif
} 

/**
  * Write to a file in host pc through USB, similar to fwrite()
  * @param fp file descriptor number
  * @param buf buffer to store the file content
  * @param writelen number of bytes to write
  * @return -1 if write failed, 0 if success
  */
int vdecwriteFile(int fp,char *buf,int writelen)
{
    int num = 0;
    int cmd = 0;
    int ack = 0;
    int fpos = 0;
    int bytesToWriteTotal = 0;
    int bytesToWrite = 0;
    int bytesWrite = 0;
    char *bufCur = buf;


    //printk("[USBIO] =====> Write file fp %d, buf 0x%x, writelen %d <=====\n", fp, buf, writelen);

    //vdecinitKernelEnv();
    // Send CMD FILE_READ
    cmd = FILE_WRITE;
    num = fd->f_op->write(fd,(const void *)&cmd, sizeof(int), &fd->f_pos);
    if (num != sizeof(int)) printk("[USBIO][WRITE] E1\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_GET_PARAM_NUM) printk("[USBIO][WRITE] E1-A\n");

    // Send int 4
    cmd = 4;
    num = fd->f_op->write(fd,(const void *)&cmd, sizeof(int), &fd->f_pos);
    if (num != sizeof(int)) printk("[USBIO][WRITE] E2\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_GET_PARAM) printk("[USBIO][WRITE] E2-A\n");

    // Send int fp
    num = fd->f_op->write(fd,(const void *)&fp, sizeof(int), &fd->f_pos);
    if (num != sizeof(int)) printk("[USBIO][WRITE] E3\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_GET_PARAM) printk("[USBIO][WRITE] E3-A\n");

    // Send int writelen
    num = fd->f_op->write(fd,(const void *)&writelen, sizeof(int), &fd->f_pos);
    if (num != sizeof(int)) printk("[USBIO][WRITE] E4\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_GET_PARAM) printk("[USBIO][WRITE] E4-A\n");


    bytesToWriteTotal = writelen;
    while (bytesToWriteTotal > 0)
    {
        bytesToWrite = (bytesToWriteTotal > SERIAL_BUFFER_SZ) ? SERIAL_BUFFER_SZ:bytesToWriteTotal;
        
        // Send int bytesToWrite
        num = fd->f_op->write(fd,(const void *)&bytesToWrite, sizeof(int), &fd->f_pos);
        if (num != sizeof(int)) printk("[USBIO][WRITE] E5\n");
        num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
        if (num != sizeof(int) || ack != ACK_GET_PARAM) printk("[USBIO][WRITE] E5-A\n");
        if (ack == ACK_CMD_COMPLETE) 
        { 
            printk("[USBIO][WRITE] E5-B\n");
            break;
        }

        // Send data
        bytesWrite = fd->f_op->write(fd,(const void *)bufCur, bytesToWrite, &fd->f_pos);
        if (bytesWrite != bytesToWrite) printk("[USBIO][WRITE] E6\n");
        num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
        if (num != sizeof(int)) printk("[USBIO][WRITE] E6-A\n");

        bufCur += bytesWrite;
        bytesToWriteTotal -= bytesWrite;

        if (bytesToWriteTotal > 0)
        {
            if (ack != ACK_CMD_CONTINUE) printk("[USBIO][WRITE] E6-B\n");
            if (ack == ACK_CMD_COMPLETE) break;
        }
        else
        {
            if (ack != ACK_CMD_COMPLETE) printk("[USBIO][WRITE] E6-C\n");
        }
    }

    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int)) printk("[USBIO][WRITE] E7\n");
    if (ack != 0)
    {
        //set_fs(vdecoldfs);
        return -1;
    }

    // Send complete command
    ack = ACK_CMD_COMPLETE;
    num = fd->f_op->write(fd,(const void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int))
        printk("[USBIO][WRITE] E8\n");

    //set_fs(vdecoldfs);
    return 0;
}


/**
 * Close file on host pc
 * @param fp file descriptor number
 * @return 0
 */
int vdeccloseFile(int fp) 
{
    int num = 0;
    int cmd = 0;
    int ack = 0;

    //printk("[USBIO] =====> Close file %d <=====\n", fp);

    //vdecinitKernelEnv();
    cmd = FILE_CLOSE;
    num = fd->f_op->write(fd,(const void *)&cmd, sizeof(int), &fd->f_pos);
    if (num != sizeof(int)) printk("[USBIO][CLOSE] E1\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_GET_PARAM_NUM) printk("[USBIO][CLOSE] E1-A\n");

    cmd = 1;
    num = fd->f_op->write(fd,(const void *)&cmd, sizeof(int), &fd->f_pos);
    if (num != sizeof(int)) printk("[USBIO][CLOSE] E2\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_GET_PARAM) printk("[USBIO][CLOSE] E2-A\n");

    num = fd->f_op->write(fd,(const void *)&fp, sizeof(int), &fd->f_pos);
    if (num != sizeof(int)) printk("[USBIO][CLOSE] E3\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_CMD_COMPLETE) printk("[USBIO][CLOSE] E3-A\n");

    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int)) printk("[USBIO][CLOSE] E3-B\n");
    if (ack == -1)
    {    
        num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
        //set_fs(vdecoldfs);
        return -1;
    }    

    ack = ACK_CMD_COMPLETE;
    num = fd->f_op->write(fd,(const void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int)) printk("[USBIO][CLOSE] E4\n");

    //set_fs(vdecoldfs);
    return 0; 
} 


/**
 * Initialize COM port connection
 */
BOOL fgHDDFsMount (UINT32 u4InstID)
{
    if (fd != 0)
    {
        fgHDDFsUnMount(u4InstID);
    }
#ifdef USB_ACM_DMA1
    printk("[USBIO] =====> Open /dev/usbacm <=====\n");
#else
    printk("[USBIO] =====> Open /dev/ttyGS0 <=====\n");
#endif

    vdecinitKernelEnv();
#ifdef USB_ACM_DMA1
    fd = filp_open("/dev/usbacm", O_RDWR|O_NOCTTY|O_NDELAY, 0);
    if (fd == 0) 
    {   printk("[USBIO] Open /dev/usbacm failed\n");
        return 0;
    }
#else
    fd = filp_open("/dev/ttyGS0", O_RDWR, 0);
    if (fd == 0) 
    {   printk("[USBIO] Open /dev/ttyGS0 failed\n");
        return 0;
    }
    
    fd->f_op->unlocked_ioctl(fd, TCGETS, (unsigned long)&settings);
    // TODO: Error handling

    settings.c_cflag &= ~CBAUD;
    settings.c_cflag |= B921600;
    settings.c_cflag &= ~PARENB;
    settings.c_cflag &= ~CSTOPB;
    settings.c_cflag &= ~CSIZE;
    settings.c_cflag |= CS8 | CLOCAL | CREAD;
    
    settings.c_iflag &= ~(INLCR | ICRNL | IXON | IXOFF | IXANY);
    settings.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);  // raw input
    settings.c_oflag &= ~OPOST;  // raw output

    settings.c_cc[VMIN] = 0;
    settings.c_cc[VTIME] = 100;
    
    fd->f_op->unlocked_ioctl(fd, TCSETS, (unsigned long)&settings); 
#endif    
    // TODO: Error handling


    return TRUE;
}


/**
 * Close COM port
 */
BOOL fgHDDFsUnMount (UINT32 u4InstID)
{
    printk("[USBIO] =====> Close /dev/ttyGS0 <=====\n");

    if (fd)
    {
        filp_close(fd, NULL);
        set_fs(vdecoldfs);
        fd = 0;
    }

    return TRUE;
}

/**
 * Open HDD file
 * @param u4InstID decoder instance id, not used
 * @param strFileName file name
 * @param pi4FileId file descriptor id
 * @return FALSE if error,
 *         TRUE otherwise
 */
BOOL fgHDDFsOpenFile(  UINT32 u4InstID,
    CHAR *strFileName,
    INT32* pi4FileId)
{
    INT32 i4_ret;
    INT32 fp = -1; 
    printk(" ===> fgHDDFsOpenFile %s <===\n", strFileName);

    fp = vdecopenFile((char*)strFileName,O_RDONLY,0); 
    if (fp == -1)
    {
        printk("Fs open file fail %d\n", fp);
        return FALSE;
    }
    *pi4FileId = fp;

    return TRUE;
}

/**
 * Close HDD file
 * @param pi4FileId file descriptor id
 * @return FALSE if error
 *         TRUE otherwise
 */
BOOL fgHDDFsCloseFile(  UINT32 i4FileId)
{
    printk(" ===> fgHDDFsCloseFile %d <===\n", i4FileId);

    vdeccloseFile(i4FileId);
    return TRUE;

}


/**
 * Read file content into buffer
 * @param u4InstID decoder instance id
 * @param strFileName file name
 * @param pvAddr buffer pointer
 * @param u4Offset offset from beginning of file
 * @param u4Length data length to read
 * @param pu4RealReadSize real data length read
 * @param pu4TotalFileLength total file length
 * @param pi4FileId file id
 */
BOOL fgHDDFsReadFile(  UINT32 u4InstID,
    CHAR *strFileName,
    void* pvAddr,
    UINT32 u4Offset,
    UINT32 u4Length,
    UINT32 *pu4RealReadSize,
    UINT32 *pu4TotalFileLength,
    INT32* pi4FileId)
{
    INT32 i4_ret;
    INT32 i4Fp;
    INT32 i4FileSize = 0;
    UINT32 u4FileSize = 0;
    UINT32 u4ReadSize = 0;

    printk(" ===> fgHDDFsReadFile %s, buffer 0x%08x<===\n", strFileName, pvAddr);

    i4Fp=vdecopenFile((char*)strFileName,O_RDONLY,0); 
    if (i4Fp == -1)
    {
        printk("Fs open file fail %d\n", i4Fp);
        return FALSE;
    }

    *pi4FileId = i4Fp;

    i4FileSize = (UINT32)vdecseekFile(i4Fp, 0, SEEK_END);

    if (i4FileSize <= 0)
    {
        printk("Fs get file size fail %d\n", i4FileSize);
        return FALSE;
    }

    u4FileSize = (UINT32)i4FileSize;
    *pu4TotalFileLength = u4FileSize;             

    if (u4Offset >= u4FileSize)
    {
        vdeccloseFile(i4Fp); 
        printk(" read offset(%d) > filesize(%d)\n", u4Offset, u4FileSize);
        return FALSE;
    }

    if(vdecseekFile(i4Fp, u4Offset, SEEK_SET) != u4Offset)
    {
        vdeccloseFile(i4Fp); 
        printk(" seek fail: %s\n", strFileName);
        return FALSE;
    }

    if (u4Length >= (u4FileSize - u4Offset))
    {
        u4ReadSize = u4FileSize - u4Offset;
    }
    else
    {
        printk("Warning =====>File size is larger than VFIFO size! 0x%x Byte will be read\n",V_FIFO_SZ);
        u4ReadSize = u4Length;//V_FIFO_SZ;
        *pu4TotalFileLength = u4Length;//V_FIFO_SZ;
    }

    //vdecinitKernelEnv(); 
    i4_ret=vdecreadFile(i4Fp, pvAddr, u4ReadSize);
    //set_fs(vdecoldfs); 

    if(i4_ret < 0)
    {
          vdeccloseFile(i4Fp); 
          printk("read file Fail!\n");
          return FALSE;
    }

    *pu4RealReadSize = i4_ret;
    vdeccloseFile(i4Fp); 

    if (u4ReadSize != *pu4RealReadSize)
    {
        printk("\n read fail: %s", strFileName);
        return FALSE;
    }

    return TRUE;
}


BOOL fgHDDFsWriteFile(CHAR *strFileName,
	void* pvAddr,
	UINT32 u4Length)
{
    INT32 i4_ret;
    INT32 i4Fp;
    INT32 i4FileSize = 0;
    UINT32 u4FileSize = 0;

    printk(" ===> fgHDDFsWriteFile %s <===\n", strFileName);

    i4Fp=vdecopenFile((char*)strFileName,O_CREAT|O_WRONLY,0); 
    if (i4Fp == -1)
    {
        printk("Fs open file fail %d\n", i4Fp);
        return FALSE;
    }
    
    i4FileSize = (UINT32)vdecseekFile(i4Fp, 0, SEEK_END);
    if (i4FileSize < 0)
    {
        vdeccloseFile(i4Fp); 
        printk("Fs get file size fail %d\n", u4FileSize);
        return FALSE;
    }

    if(vdecseekFile(i4Fp, i4FileSize, SEEK_SET) != i4FileSize)
    {
        vdeccloseFile(i4Fp); 
        printk(" seek fail: %s\n", strFileName);
        return FALSE;
    }

    //vdecinitKernelEnv();
    i4_ret = vdecwriteFile(i4Fp, pvAddr, u4Length);
    //set_fs(vdecoldfs); 
    if(i4_ret < 0)
    {
        vdeccloseFile(i4Fp); 
        printk("write file Fail!\n");
        return FALSE;
    }    
    vdeccloseFile(i4Fp); 
    return TRUE;
}


UINT32 u4HDDFsGetFileSize(INT32 *pi4FileId)
{
    printk(" ===> fgHDDFsGetFile %d <===\n", *pi4FileId);

    return ((UINT32)vdecseekFile(*pi4FileId, 0, SEEK_END));
}

#endif
