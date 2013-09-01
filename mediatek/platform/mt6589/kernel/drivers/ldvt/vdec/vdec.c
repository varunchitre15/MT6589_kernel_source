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
#include <linux/delay.h>

#include <asm/uaccess.h>
#include "verify/vdec_verify_vdec.h"
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/file.h> 
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
#include <linux/termios.h>
#include <linux/vmalloc.h>
#include <linux/time.h>

#include "verify/vdec_verify_general.h"
#include "verify/vdec_verify_mm_map.h"
#include "verify/vdec_verify_mpv_prov.h"
#include "verify/vdec_verify_file_common.h"
#include "include/vdec_info_common.h"
#include "mach/mt_reg_base.h"
#include "mach/sync_write.h"

#include <linux/io.h>
#include <asm/io.h>

 
#define vdecname	"uvvp_vdec"
#define UVVP_VDEC_TEST_VERSION                  0x00440000
#define UVVP_VDEC_TEST_VERSION_1                0x00440001
#define UVVP_VDEC_TEST_VERSION_2                0x00440002
#define UVVP_VDEC_TEST_VERSION_3                0x00440003


struct task_struct *MPV_thread = NULL;
struct sched_param param;
//---------------------------------------------------------------------------
// IOCTL
//---------------------------------------------------------------------------

#define PRIORITY_CLASS_HIGH             99   ///< for streaming
#define PRIORITY_LAYER_TIME_CRITICAL    0
#define PRIORITY(CLASS, LAYER, OFFSET)  ((CLASS) + (LAYER) + (OFFSET))
#define MPV_VPRS_THREAD_PRIORITY	PRIORITY(PRIORITY_CLASS_HIGH, PRIORITY_LAYER_TIME_CRITICAL, 0)


#define MC_BASE_OFFSET  0x2000

#define OFFSET_R1Y_ADD  0x0
#define OFFSET_R1C_ADD  0x4
#define OFFSET_R2Y_ADD  0x8
#define OFFSET_R2C_ADD  0xC
#define Y_MASK          0x007FFFFF
#define C_MASK          0x00FFFFFF


#define MFV_HW_WRITE(ptr,data)     mt65xx_reg_sync_writel(data,ptr)
#define MFV_HW_READ(ptr)           (*((volatile unsigned int * const)ptr))

//#define USB_ACM_DMA1
#ifdef USB_ACM_DMA1
#define SERIAL_BUFFER_SZ    40*1024
#else
#define SERIAL_BUFFER_SZ    2*1024
#endif
#define TESTDATA_BUFFER_SZ  20*1024*1024

#ifdef SEEK_SET
#undef SEEK_SET
#define SEEK_SET    0
#endif
#ifdef SEEK_CUR
#undef SEEK_CUR
#define SEEK_CUR    1
#endif
#ifdef SEEK_END
#undef SEEK_END
#define SEEK_END    2
#endif

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

#if VDEC_VP8_WEBP_SUPPORT_ME2_INTEGRATION
extern void vVerInitVDec(UINT32 u4InstID);
extern void vVParserProc(UINT32 u4InstID);
extern void vVDecProc(UINT32 u4InstID);
extern void vChkVDec(UINT32 u4InstID);
extern void vVerifyVDecIsrStop(UINT32 u4InstID);
extern BOOL fgHDDFsUnMount(UINT32 u4InstID);

#if VP8_MB_ROW_MODE_SUPPORT_ME2_INTEGRATION
extern void vChkVDec_Webp_Row_Mode(UINT32 u4InstID);
extern UINT32 vVerVP8DecEndProc_MB_ROW_START(UINT32 u4InstID);
#endif
#endif

#define BASE 65521L /* largest prime smaller than 65536 */

#define DO1(buf)  {s1 += *buf++; s2 += s1;}
#define DO4(buf)  DO1(buf); DO1(buf); DO1(buf); DO1(buf);
#define DO16(buf) DO4(buf); DO4(buf); DO4(buf); DO4(buf);

// (1, frame, height(include padding), width(include padding) )
static unsigned long av_adler32_update(unsigned long adler, const unsigned char *buf, int ylen, int xlen)
{
    unsigned long s1 = adler & 0xffff;
    unsigned long s2 = adler >> 16;
	unsigned char *pbuf;
	int xlenorg = xlen;


	while(ylen>0){
		pbuf = ( unsigned char * )buf;
		while (xlen>0) {
	#ifdef CONFIG_SMALL
			while(xlen>4 && s2 < (1U<<31)){
				DO4(buf); xlen-=4;
	#else
			while(xlen>0 && s2 < (1U<<31)  && (xlen>16) ){
				DO16(buf); xlen-=16;
	#endif

			}
			DO1(buf); xlen--;
			s1 %= BASE;
			s2 %= BASE;
		}
		ylen--;
		xlen = xlenorg;
		//buf = pbuf;
	}
    return (s2 << 16) | s1;
}


static int to_sched_priority(UINT8 ui1_priority)
{
    int sched_priority;
    sched_priority = 100 - (int)ui1_priority * 100 / 256;
    if (sched_priority < 1) sched_priority = 1;
    if (sched_priority > 99) sched_priority = 99;
    return sched_priority;
}
static UINT8 from_sched_priority(int sched_priority)
{
    return (UINT8)((100 - sched_priority) * 256 / 100);
}

mm_segment_t oldfs; 
 
struct file *openFile(char *path,int flag,int mode) 
{ 
struct file *fp; 
 
fp=filp_open(path, flag, 0); 
if (fp) return fp; 
else return NULL; 
} 
 
int readFile(struct file *fp,char *buf,int readlen) 
{ 
if (fp->f_op && fp->f_op->read) 
return fp->f_op->read(fp,buf,readlen, &fp->f_pos); 
else 
return -1; 
} 
 
int closeFile(struct file *fp) 
{ 
filp_close(fp,NULL); 
return 0; 
} 
 
void initKernelEnv(void) 
{ 
oldfs = get_fs(); 
set_fs(KERNEL_DS); 
} 
 
static int __init readfile_init(void) 
{ 
char * buf; 
struct file *fp; 
int ret; 
 
initKernelEnv(); 
buf = (char *)kmalloc(1024, GFP_KERNEL);
printk("buf:%x\n",buf); 
fp=openFile("/mnt/udiska/VP8_bitstream/gp/webm_youtube/zilker park WebM format test/post_0.y.dat",O_RDONLY,0); 
if (fp!=NULL) 
{ 
memset(buf,0,1024); 
if ((ret=readFile(fp,buf,1024))>0) 
printk("buf:%s\n",buf); 
else printk("read file error %d\n",ret); 
closeFile(fp); 
} 
set_fs(oldfs); 
return 0; 
} 


int m4u_v2p_new(unsigned int va)
{
   unsigned int pmdOffset = (va & (PMD_SIZE - 1));
   unsigned int pageOffset = (va & (PAGE_SIZE - 1));
   pgd_t *pgd;
   pmd_t *pmd;
   pte_t *pte;
   unsigned int pa;
   printk("Enter m4u_user_v2p()! 0x%x\n", va);
   
   pgd = pgd_offset(current->mm, va); /* what is tsk->mm */
   printk("m4u_user_v2p(), pgd 0x%x\n", pgd);    
   printk("pgd_none=%d, pgd_bad=%d\n", pgd_none(*pgd), pgd_bad(*pgd));
   
   if(pgd_none(*pgd)||pgd_bad(*pgd))
   {
      printk("Error: m4u_user_v2p(), virtual addr 0x%x, pgd invalid! \n", va);
      return 0;
   }
   
   pmd = pmd_offset(pgd, va);
   printk("m4u_user_v2p(), pmd 0x%x\n", pmd);
   printk("pmd_none=%d, pmd_bad=%d, pmd_val=0x%x\n", pmd_none(*pmd), pmd_bad(*pmd), pmd_val(*pmd));
   
   
   /* If this is a page table entry, keep on walking to the next level */ 
   if (( (unsigned int)pmd_val(*pmd) & PMD_TYPE_MASK) == PMD_TYPE_TABLE)
   {
      if(pmd_none(*pmd)||pmd_bad(*pmd))
      {
         printk("Error: m4u_user_v2p(), virtual addr 0x%x, pmd invalid! \n", va);
         return 0;
      }
      
      pte = pte_offset_map(pmd, va);
      printk("m4u_user_v2p(), pte 0x%x\n", pte);
      if(pte_present(*pte)) 
      { 
         pa=(pte_val(*pte) & (PAGE_MASK)) | pageOffset; 
         printk("PA = 0x%8x\n", pa);
         return pa; 
      }
   }
   else /* Only 1 level page table */
   {
      if(pmd_none(*pmd))
      {
         printk("Error: m4u_user_v2p(), virtual addr 0x%x, pmd invalid! \n", va);
         return 0;
      }
      pa=(pte_val(*pmd) & (PMD_MASK)) | pmdOffset; 
      printk("PA = 0x%8x\n", pa);
      return pa;

   }
   
   return 0;   
}

static int remote_open(struct file *fd, const char *filename, int filename_sz, const char *mode, int mode_sz)
{
    int num = 0;
    int cmd = 0;
    int ack = 0;
    int fdnum = 0;
    char fname[512];
    char fmode[8];
    if (NULL == fd)
        return -1;

    if (filename_sz < 512)
    {
        memcpy(fname, filename, filename_sz);
        fname[filename_sz] = '\0';        
    }
    else
    {
        printk("[OPEN] Filename too long\n");
        return -1;
    }

    if (mode_sz < 8)
    {
        memcpy(fmode, mode, mode_sz);
        fmode[mode_sz] = '\0';        
    }
    else
    {
        printk("[OPEN] Filemode too long\n");
        return -1;
    }
    
    printk("[OPEN] + Entering...\n");
    cmd = FILE_OPEN;
    num = fd->f_op->write(fd,(const void *)&cmd, sizeof(int), &fd->f_pos);
    if (num != sizeof(int))
        printk("[OPEN] E1\n");
    printk("[OPEN] - Entering...\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_GET_PARAM_NUM)
        printk("[OPEN] E1-A\n");
    printk("[OPEN] A...\n");

    cmd = 2;
    num = fd->f_op->write(fd,(const void *)&cmd, sizeof(int), &fd->f_pos);
    if (num != sizeof(int))
        printk("[OPEN] E2\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_GET_PARAM)
        printk("[OPEN] E2-A\n");
    printk("[OPEN] B...\n");
    num = fd->f_op->write(fd,(const void *)&fname, 511, &fd->f_pos);
    if (num != 511)
        printk("[OPEN] E3\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_GET_PARAM)
        printk("[OPEN] E3-A\n");
    printk("[OPEN] C...\n");
    num = fd->f_op->write(fd,(const void *)&fmode, 7, &fd->f_pos);
    if (num != 7)
        printk("[OPEN] E4\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_CMD_COMPLETE)
        printk("[OPEN] E4-A\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int))
        printk("[OPEN] E4-B\n");
    if (ack == -1)
    {    
        num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
        return -1;
    }
    
    ack = ACK_GET_PARAM_NUM;
    num = fd->f_op->write(fd,(const void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int))
        printk("[OPEN] E5\n");
    num = fd->f_op->read(fd,(void *)&cmd, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || cmd != 1)
        printk("[OPEN] E5-A\n");

    ack = ACK_GET_PARAM;
    num = fd->f_op->write(fd,(const void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int))
        printk("[OPEN] E6\n");
    num = fd->f_op->read(fd,(void *)&fdnum, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || fdnum < 0)
        printk("[OPEN] E6-A\n");
    
    
    ack = ACK_CMD_COMPLETE;
    num = fd->f_op->write(fd,(const void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int))
        printk("[OPEN] E7\n");

    return fdnum;
}


static int remote_close(struct file *fd, int file_idx)
{
    int num = 0;
    int cmd = 0;
    int ack = 0;
    if (NULL == fd)
        return -1;
    
    cmd = FILE_CLOSE;
    num = fd->f_op->write(fd,(const void *)&cmd, sizeof(int), &fd->f_pos);
    if (num != sizeof(int))
        printk("[CLOSE] E1\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_GET_PARAM_NUM)
        printk("[CLOSE] E1-A\n");

    cmd = 1;
    num = fd->f_op->write(fd,(const void *)&cmd, sizeof(int), &fd->f_pos);
    if (num != sizeof(int))
        printk("[CLOSE] E2\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_GET_PARAM)
        printk("[CLOSE] E2-A\n");

    num = fd->f_op->write(fd,(const void *)&file_idx, sizeof(int), &fd->f_pos);
    if (num != sizeof(int))
        printk("[CLOSE] E3\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_CMD_COMPLETE)
        printk("[CLOSE] E3-A\n");

    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int))
        printk("[CLOSE] E3-B\n");
    if (ack == -1)
    {    
        num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
        return -1;
    }    

    ack = ACK_CMD_COMPLETE;
    num = fd->f_op->write(fd,(const void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int))
        printk("[CLOSE] E4\n");

    return 0;
}

//#define NO_ACK_TRANSFER
static int remote_read(struct file *fd, int file_idx, unsigned char *buf, int len)
{
#ifdef NO_ACK_TRANSFER
    int op_res = 0;
    int cmd = 0;
    int resp = 0;
    int len_remain = len;
    int len_recv = 0;
    struct timeval tv1;
    struct timeval tv2;
    long usec = 0;
    int tmp = 0;

    
    // Send command
    cmd = FILE_READ;
    do_gettimeofday(&tv1);    
    op_res = fd->f_op->write(fd,(const void *)&cmd, sizeof(int), &fd->f_pos);
    do_gettimeofday(&tv2);    
    if (op_res != sizeof(int)) printk("[READ] Command FILE_READ send fail %d\n", op_res);
    tv2.tv_sec -= tv1.tv_sec;
    if (tv2.tv_usec < tv1.tv_usec)
    {
        if (tv2.tv_sec < 1)
            printk("TIME ERROR\n");
        tv2.tv_usec = tv2.tv_usec + 1000000 - tv1.tv_usec;
        tv2.tv_sec -= 1;
    }
    else
    {
        tv2.tv_usec -= tv1.tv_usec;
    }
    printk("[READ] Send command takes %d ms\n", (tv2.tv_sec*1000000 + tv2.tv_usec)/1000);

    // Send file descriptor index
    do_gettimeofday(&tv1);    
    op_res = fd->f_op->write(fd,(void *)&file_idx, sizeof(int), &fd->f_pos);
    do_gettimeofday(&tv2);
    if (op_res != sizeof(int)) printk("[READ] File descriptor index send fail %d\n", op_res);
    tv2.tv_sec -= tv1.tv_sec;
    if (tv2.tv_usec < tv1.tv_usec)
    {
        if (tv2.tv_sec < 1)
            printk("TIME ERROR\n");
        tv2.tv_usec = tv2.tv_usec + 1000000 - tv1.tv_usec;
        tv2.tv_sec -= 1;
    }
    else
    {
        tv2.tv_usec -= tv1.tv_usec;
    }
    printk("[READ] Send fd takes %d ms\n", (tv2.tv_sec*1000000 + tv2.tv_usec)/1000);

    // Send read data length
    do_gettimeofday(&tv1);    
    op_res = fd->f_op->write(fd,(void *)&len, sizeof(int), &fd->f_pos);
    do_gettimeofday(&tv2);
    if (op_res != sizeof(int)) printk("[READ] Read length send fail %d\n", op_res);
    tv2.tv_sec -= tv1.tv_sec;
    if (tv2.tv_usec < tv1.tv_usec)
    {
        if (tv2.tv_sec < 1)
            printk("TIME ERROR\n");
        tv2.tv_usec = tv2.tv_usec + 1000000 - tv1.tv_usec;
        tv2.tv_sec -= 1;
    }
    else
    {
        tv2.tv_usec -= tv1.tv_usec;
    }
    printk("[READ] Send data length takes %d ms\n", (tv2.tv_sec*1000000 + tv2.tv_usec)/1000);

    // Get response
    do_gettimeofday(&tv1);        
    op_res = fd->f_op->read(fd,(void *)&resp, sizeof(int), &fd->f_pos);
    do_gettimeofday(&tv2);
    if (op_res != sizeof(int)) printk("[READ] Get command response fail %d\n", op_res);
    tv2.tv_sec -= tv1.tv_sec;
    if (tv2.tv_usec < tv1.tv_usec)
    {
        if (tv2.tv_sec < 1)
            printk("TIME ERROR\n");
        tv2.tv_usec = tv2.tv_usec + 1000000 - tv1.tv_usec;
        tv2.tv_sec -= 1;
    }
    else
    {
        tv2.tv_usec -= tv1.tv_usec;
    }
    printk("[READ] Get response takes %d ms\n", (tv2.tv_sec*1000000 + tv2.tv_usec)/1000);

    if (0 != resp)
    {
        printk("[READ] Read operation failed at remote %d\n", resp);
        return -1;
    }

    // Get data if no error
    while (len_remain > 0)
    {
        len_recv = (len_remain > SERIAL_BUFFER_SZ)?SERIAL_BUFFER_SZ:len_remain;
        do_gettimeofday(&tv1);
        op_res = fd->f_op->read(fd,(void *)buf, len_recv, &fd->f_pos);
        do_gettimeofday(&tv2);
        tv2.tv_sec -= tv1.tv_sec;
        if (tv2.tv_usec < tv1.tv_usec)
        {
            if (tv2.tv_sec < 1)
                printk("TIME ERROR\n");
            tv2.tv_usec = tv2.tv_usec + 1000000 - tv1.tv_usec;
            tv2.tv_sec -= 1;
        }
        else
        {
            tv2.tv_usec -= tv1.tv_usec;
        }
        //printk("[Read] takes %d ms\n", (tv2.tv_sec*1000000 + tv2.tv_usec)/1000);
        usec += (tv2.tv_sec*1000000 + tv2.tv_usec);
        
        if (op_res != len_recv)
        {
            printk("[READ] Read data %d-%d bytes failed, only read %d\n", len-len_remain, len-len_remain+len_recv, op_res);
            break; //return (len-len_remain+op_res);
        }

        //tmp = fd->f_op->write(fd,(void *)&tmp, sizeof(int), &fd->f_pos);
        buf += op_res;
        len_remain -= op_res;
    }
    printk("[READ] Total transfer time %d ms\n", usec/1000);

    return (op_res != len_recv)?(len-len_remain+op_res):(len-len_remain);
#else
    int num = 0;
    int cmd = 0;
    int ack = 0;
    int fpos = 0;
    int bytesReadTotal = 0;
    int bytesToRead = 0;
    int bytesRead = 0;
    char *bufCur = buf;

    // Send CMD FILE_READ
    cmd = FILE_READ;
    num = fd->f_op->write(fd,(const void *)&cmd, sizeof(int), &fd->f_pos);
    if (num != sizeof(int)) printk("[READ] E1\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_GET_PARAM_NUM) printk("[READ] E1-A\n");

    // Send int 2
    cmd = 2;
    num = fd->f_op->write(fd,(const void *)&cmd, sizeof(int), &fd->f_pos);
    if (num != sizeof(int)) printk("[READ] E2\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_GET_PARAM) printk("[READ] E2-A\n");

    // Send int fp
    num = fd->f_op->write(fd,(const void *)&file_idx, sizeof(int), &fd->f_pos);
    if (num != sizeof(int)) printk("[READ] E3\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_GET_PARAM) printk("[READ] E3-A\n");

    // Send int readlen
    num = fd->f_op->write(fd,(const void *)&len, sizeof(int), &fd->f_pos);
    if (num != sizeof(int)) printk("[READ] E4\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);    
    if (num != sizeof(int) || ack != ACK_CMD_COMPLETE) printk("[READ] E4-A\n");

    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);    
    if (num != sizeof(int)) printk("[READ] E4-B\n");
    if (ack == -1)
    {    
        num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
        return FALSE;
    }

    // Get int 2
    ack = ACK_GET_PARAM_NUM;
    num = fd->f_op->write(fd,(const void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int)) printk("[READ] E5\n");
    num = fd->f_op->read(fd,(void *)&cmd, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || cmd != 2) printk("[READ] E5-A\n");

    ack = ACK_GET_PARAM;
    num = fd->f_op->write(fd,(const void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int)) printk("[READ] E6\n");

    printk("[READ] Entering read loop\n");
    while (bytesRead < len)
    {
        // Get bytes to read
        num = fd->f_op->read(fd,(void *)&bytesToRead, sizeof(int), &fd->f_pos);
        if (num != sizeof(int) || bytesToRead < 0) printk("[READ] E6-A\n");
        if (0 == bytesToRead)
        {
            ack = ACK_CMD_COMPLETE;
            num = fd->f_op->write(fd,(const void *)&ack, sizeof(int), &fd->f_pos);
            if (num != sizeof(int)) printk("[READ] E6-B\n");
            break;
        }
            
        // Get data        
        ack = ACK_GET_PARAM;
        num = fd->f_op->write(fd,(const void *)&ack, sizeof(int), &fd->f_pos);
        if (num != sizeof(int)) printk("[READ] E7\n");        
        num = fd->f_op->read(fd,(void *)bufCur, bytesToRead, &fd->f_pos);
        if (num != bytesToRead) printk("[READ] E7-A\n");

        //printk("[READ] Bytes to read %d bytesRead %d\n", bytesToRead, bytesRead);

        bufCur += num;
        bytesRead += num;
        if (bytesRead < len)
        {
            ack = ACK_CMD_CONTINUE;
            num = fd->f_op->write(fd,(const void *)&ack, sizeof(int), &fd->f_pos);
            if (num != sizeof(int)) printk("[READ] E8-A\n");
        }
        else
        {
            ack = ACK_CMD_COMPLETE;
            num = fd->f_op->write(fd,(const void *)&ack, sizeof(int), &fd->f_pos);
            if (num != sizeof(int)) printk("[READ] E8-B\n");
        }
    }

    return bytesRead;

#endif
}


static int remote_seek(struct file *fd, int file_idx, int offset, int origin)
{
    int num = 0;
    int cmd = 0;
    int ack = 0;
    int fpos = 0;
    if (NULL == fd)
        return -1;
    
    cmd = FILE_SEEK;
    num = fd->f_op->write(fd,(const void *)&cmd, sizeof(int), &fd->f_pos);
    if (num != sizeof(int))
        printk("[SEEK] E1\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_GET_PARAM_NUM)
        printk("[SEEK] E1-A\n");

    cmd = 3;
    num = fd->f_op->write(fd,(const void *)&cmd, sizeof(int), &fd->f_pos);
    if (num != sizeof(int))
        printk("[SEEK] E2\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_GET_PARAM)
        printk("[SEEK] E2-A\n");

    num = fd->f_op->write(fd,(const void *)&file_idx, sizeof(int), &fd->f_pos);
    if (num != sizeof(int))
        printk("[SEEK] E3\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_GET_PARAM)
        printk("[SEEK] E3-A %d %d\n", num, ack);

    num = fd->f_op->write(fd,(const void *)&offset, sizeof(int), &fd->f_pos);
    if (num != sizeof(int))
        printk("[SEEK] E4\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_GET_PARAM)
        printk("[SEEK] E4-A\n");

    num = fd->f_op->write(fd,(const void *)&origin, sizeof(int), &fd->f_pos);
    if (num != sizeof(int))
        printk("[SEEK] E5\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_CMD_COMPLETE)
        printk("[SEEK] E5-A\n");

    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int))
        printk("[SEEK] E5-B\n");
    if (ack == -1)
    {    
        num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
        return -1;
    }

    ack = ACK_GET_PARAM_NUM;
    num = fd->f_op->write(fd,(const void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int))
        printk("[SEEK] E6\n");
    num = fd->f_op->read(fd,(void *)&cmd, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || cmd != 1)
        printk("[SEEK] E6-A\n");

    ack = ACK_GET_PARAM;
    num = fd->f_op->write(fd,(const void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int))
        printk("[SEEK] E7\n");
    num = fd->f_op->read(fd,(void *)&fpos, sizeof(int), &fd->f_pos);
    if (num != sizeof(int))
        printk("[SEEK] E7-A\n");
    
    ack = ACK_CMD_COMPLETE;
    num = fd->f_op->write(fd,(const void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int))
        printk("[SEEK] E8\n");

    return fpos;
}


int remote_write(struct file *fd, int fp, unsigned char *buf, int writelen)
{
    int num = 0;
    int cmd = 0;
    int ack = 0;
    int fpos = 0;
    int bytesToWriteTotal = 0;
    int bytesToWrite = 0;
    int bytesWrite = 0;
    unsigned char *bufCur = buf;
    int i = 0;
    int *numbers = bufCur;

    int tst = 100;
    
    // Send CMD FILE_READ
    cmd = FILE_WRITE;
    num = fd->f_op->write(fd,(const void *)&cmd, sizeof(int), &fd->f_pos);
    if (num != sizeof(int)) printk("[WRITE] E1\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_GET_PARAM_NUM) printk("[WRITE] E1-A\n");

    // Send int 4
    cmd = 4;
    num = fd->f_op->write(fd,(const void *)&cmd, sizeof(int), &fd->f_pos);
    if (num != sizeof(int)) printk("[WRITE] E2\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_GET_PARAM) printk("[WRITE] E2-A\n");

    // Send int fp
    num = fd->f_op->write(fd,(const void *)&fp, sizeof(int), &fd->f_pos);
    if (num != sizeof(int)) printk("[WRITE] E3\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_GET_PARAM) printk("[WRITE] E3-A\n");

    // Send int writelen
    num = fd->f_op->write(fd,(const void *)&writelen, sizeof(int), &fd->f_pos);
    if (num != sizeof(int)) printk("[WRITE] E4\n");
    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int) || ack != ACK_GET_PARAM) printk("[WRITE] E4-A\n");


    bytesToWriteTotal = writelen;
    while (bytesToWriteTotal > 0)
    {
        bytesToWrite = (bytesToWriteTotal > SERIAL_BUFFER_SZ) ? SERIAL_BUFFER_SZ:bytesToWriteTotal;

        //printk("[USBIO][WRITE] bytesToWrite %d, bytesToWriteTotal %d\n", bytesToWrite, bytesToWriteTotal);
        
        // Send int bytesToWrite
        num = fd->f_op->write(fd,(const void *)&bytesToWrite, sizeof(int), &fd->f_pos);
        if (num != sizeof(int)) printk("[WRITE] E5\n");
        num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
        if (num != sizeof(int) || ack != ACK_GET_PARAM) printk("[WRITE] E5-A\n");
        if (ack == ACK_CMD_COMPLETE) 
        { 
            printk("[WRITE] E5-B\n");
            break;
        }

        // Send data
        bytesWrite = fd->f_op->write(fd,(const void *)bufCur, bytesToWrite, &fd->f_pos);
        if (bytesWrite != bytesToWrite) printk("[WRITE] E6\n");
        num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
        if (num != sizeof(int)) printk("[WRITE] E6-A\n");

        bufCur += bytesWrite;
        bytesToWriteTotal -= bytesWrite;

        if (bytesToWriteTotal > 0)
        {
            if (ack != ACK_CMD_CONTINUE) printk("[WRITE] E6-B\n");
            if (ack == ACK_CMD_COMPLETE) break;
        }
        else
        {
            if (ack != ACK_CMD_COMPLETE) printk("[WRITE] E6-C\n");
        }
    }

    num = fd->f_op->read(fd,(void *)&ack, sizeof(int), &fd->f_pos);
    if (num != sizeof(int)) printk("[WRITE] E7\n");
    if (ack != 0)
        return -1;

    ack = ACK_CMD_COMPLETE;
    num = fd->f_op->write(fd,(const void *)&ack, sizeof(int), &fd->f_pos);

    return 0;

}

void calc_time_diff(struct timeval *tv1, struct timeval *tv2)
{
    tv2->tv_sec -= tv1->tv_sec;
    if (tv2->tv_usec < tv1->tv_usec)
    {
        tv2->tv_sec --;
        tv2->tv_usec = tv2->tv_usec + 1000000 - tv1->tv_usec;
    }
    else
    {
        tv2->tv_usec -= tv1->tv_usec;
    }
}

static const UCHAR *g_pu1AllocSA = 0x88000000; //0x05000000;

static int uvvp_vdec_ioctl(struct file *file,
							unsigned int cmd, unsigned long arg)
{
int ret=0;
    int i, j;
    //int recv_buf[256];
    //int send_buf[256];
    //int num = 0;
    
    // For UVVP_VDEC_TEST_VERSION_1
    int reg_in = 0;
    int reg_out = 0;

    // For UVVP_VDEC_TEST_VERSION_2
    int mem_sz = 1;
    UCHAR *p = NULL;
    unsigned int u4Align = 1024;
    unsigned int u4Size = 16384;
    unsigned char *buf_data;
    unsigned char *buf_data2;
    UINT32 u4RetValue;

    // For UVVP_VDEC_TEST_VERSION_3
    struct termios settings; 
    struct file *fd = 0;
    mm_segment_t oldfs;
    unsigned long crc = 0;
    unsigned long own_crc = 0;
    int file_num = 0;
    int file_len = 0;
    int read_len = 0;
    int tmp_buf[256];

    // For speed measurement
    struct timeval tv1;
    struct timeval tv2;

//struct file * pfilename;
  VDEC_PARAM_T *param;
#if (VDEC_MVC_SUPPORT)
  VDEC_PARAM_T *param1;
#endif
printk("\r\n******** uvvp_vdec_ioctl cmd[%d]********\r\n",cmd);
param = kmalloc(sizeof(VDEC_PARAM_T), GFP_KERNEL);
#if (VDEC_MVC_SUPPORT)
  param1 = kmalloc(sizeof(VDEC_PARAM_T), GFP_KERNEL); 
#endif

	switch (cmd) {
		//  General TEST CASE
		case UVVP_VDEC_TEST_VERSION:
			param->u4InstanceId = 0;
			param->u4Mode = 0;
                        #if (VDEC_MVC_SUPPORT)
                        param->fgMVCType = TRUE;
                        #else
                        param->fgMVCType = FALSE;
                        #endif
			printk("\r\n******** uvvp_vdec_ioctl UVVP_VDEC_TEST_VERSION ********\r\n");	
#if VDEC_VP8_WEBP_SUPPORT_ME2_INTEGRATION
            vVDecVerifyThread(param);
            vVerInitVDec(0);
            vVParserProc(0);
            vVDecProc(0);
#if VP8_MB_ROW_MODE_SUPPORT_ME2_INTEGRATION
            while(1)
            {
                u4RetValue = vVerVP8DecEndProc_MB_ROW_START(0);
                if (u4RetValue == vVerResult_MB_ROW_DONE)
                {
                    printk("\n\n======== MB ROW DONE!! ========\n\n");
                }
                else if(u4RetValue == vVerResult_FRAME_DONE)
                {
                    printk("\n\n======== FRAME DONE!! ========\n\n");
                    break;
                }
                else
                {
                    printk("\n\n[ERROR]======== decode timeout!! ========\n\n");
                    break;
                }
            }
            vChkVDec_Webp_Row_Mode(0);
#else            
            vChkVDec(0);
#endif
#ifdef PCFILE_WRITE  
            if(_tInFileInfo[param->u4InstanceId].pFile)
            {
              fclose(_tInFileInfo[param->u4InstanceId].pFile);
            }
#endif
            vVerifyVDecIsrStop(param->u4InstanceId);
            vMemoryFree(param->u4InstanceId);
#ifdef SATA_HDD_READ_SUPPORT
#ifdef SATA_HDD_FS_SUPPORT
            if (_tFileListInfo[param->u4InstanceId].i4FileId != 0xFFFFFFFF)
            {
                // fgHDDFsCloseFile(u4InstID); // temp avoid system crash
                _tFileListInfo[param->u4InstanceId].i4FileId = 0xFFFFFFFF;
            }
            //FS mount
            fgHDDFsUnMount(0);
#endif
#endif
#else
			MPV_thread = kernel_thread(vVDecVerifyThread, param, CLONE_FS | CLONE_FILES | CLONE_SIGHAND | SIGCHLD);
			if (IS_ERR(MPV_thread)) { 
				printk("[%s]: failed to create MPV thread\n", __FUNCTION__);
				return 0;	
				}
                        else
                        {
                            printk("Creat mpv0 thread ok!\n");
                        }  
//			break;					
//		case UVVP_VDEC_TEST_VERSION_1:
                        #if (VDEC_MVC_SUPPORT)
			param1->u4InstanceId = 1;
			param1->u4Mode = 0;
                        param1->fgMVCType = TRUE;
                        msleep(2);
			MPV_thread = kernel_thread(vVDecVerifyThread, param1, CLONE_FS | CLONE_FILES | CLONE_SIGHAND | SIGCHLD);
			if (IS_ERR(MPV_thread)) { 
				printk("[%s]: failed to create MPV thread\n", __FUNCTION__);
				return 0;	
				}
                        else
                        {
                            printk("Creat mpv1 thread ok!\n");
                        }
                        #endif
#endif                        
			break;
        case UVVP_VDEC_TEST_VERSION_1:
            printk("UVVP_VDEC_TEST_VERSION_1\n");

            reg_in = 0x1;
            reg_out = 0;
            MFV_HW_WRITE(VDEC_GCON_BASE, reg_in);
            reg_out = MFV_HW_READ(VDEC_GCON_BASE);
            printk("GCON write 0x%x, read 0x%x, %s\n", reg_in, reg_out, reg_in == reg_out ? "Pass":"Fail");
            
            reg_in = 0xFFFFFFFF;
            reg_out = 0;
            MFV_HW_WRITE((VDEC_BASE + MC_BASE_OFFSET + OFFSET_R1Y_ADD), reg_in);
            reg_out = MFV_HW_READ((VDEC_BASE + MC_BASE_OFFSET + OFFSET_R1Y_ADD));
            printk("OFFSET_R1Y_ADD write 0x%x, read 0x%x, mask 0x%x, %s\n", reg_in, reg_out, Y_MASK, (reg_in & Y_MASK) == reg_out ? "Pass":"Fail");
            
            reg_in = 0xFFFFFFFF;
            reg_out = 0;
            MFV_HW_WRITE((VDEC_BASE + MC_BASE_OFFSET + OFFSET_R1C_ADD), reg_in);
            reg_out = MFV_HW_READ((VDEC_BASE + MC_BASE_OFFSET + OFFSET_R1C_ADD));
            printk("OFFSET_R1C_ADD write 0x%x, read 0x%x, mask 0x%x, %s\n", reg_in, reg_out, C_MASK, (reg_in & C_MASK) == reg_out ? "Pass":"Fail");
            
            reg_in = 0xFFFFFFFF;
            reg_out = 0;
            MFV_HW_WRITE((VDEC_BASE + MC_BASE_OFFSET + OFFSET_R2Y_ADD), reg_in);
            reg_out = MFV_HW_READ((VDEC_BASE + MC_BASE_OFFSET + OFFSET_R2Y_ADD));
            printk("OFFSET_R2Y_ADD write 0x%x, read 0x%x, mask 0x%x, %s\n", reg_in, reg_out, Y_MASK, (reg_in & Y_MASK) == reg_out ? "Pass":"Fail");

            reg_in = 0xFFFFFFFF;
            reg_out = 0;
            MFV_HW_WRITE((VDEC_BASE + MC_BASE_OFFSET + OFFSET_R2C_ADD), reg_in);
            reg_out = MFV_HW_READ((VDEC_BASE + MC_BASE_OFFSET + OFFSET_R2C_ADD));
            printk("OFFSET_R2C_ADD write 0x%x, read 0x%x, mask 0x%x, %s\n", reg_in, reg_out, C_MASK, (reg_in & C_MASK) == reg_out ? "Pass":"Fail");

            break;
		case UVVP_VDEC_TEST_VERSION_2:
		    printk("UVVP_VDEC_TEST_VERSION_2\n");

            printk("=== vmalloc memory speed test ===\n");
            
            do_gettimeofday(&tv1);
            buf_data = (unsigned char *)vmalloc(TESTDATA_BUFFER_SZ);
            buf_data2 = (unsigned char *)vmalloc(TESTDATA_BUFFER_SZ); 
            do_gettimeofday(&tv2);
            calc_time_diff(&tv1, &tv2);
            printk("Allocated %d + %dMB in %d sec %d usec\n", TESTDATA_BUFFER_SZ/1024/1024, TESTDATA_BUFFER_SZ/1024/1024, tv2.tv_sec, tv2.tv_usec);

            do_gettimeofday(&tv1);
            memset(buf_data, 0, TESTDATA_BUFFER_SZ);
            memset(buf_data2, 1, TESTDATA_BUFFER_SZ);
            do_gettimeofday(&tv2);
            calc_time_diff(&tv1, &tv2);
            printk("memset %d + %dMB in %d sec %d usec\n", TESTDATA_BUFFER_SZ/1024/1024, TESTDATA_BUFFER_SZ/1024/1024, tv2.tv_sec, tv2.tv_usec);

            do_gettimeofday(&tv1);
            memcpy(buf_data, buf_data2, TESTDATA_BUFFER_SZ);
            do_gettimeofday(&tv2);
            calc_time_diff(&tv1, &tv2);
            printk("memcpy %dMB in %d sec %d usec\n", TESTDATA_BUFFER_SZ/1024/1024, tv2.tv_sec, tv2.tv_usec);

            vfree(buf_data);
            vfree(buf_data2);

            printk("=== ioremap_nocache memory speed test ===\n");
            do_gettimeofday(&tv1);
            buf_data = g_pu1AllocSA;
            buf_data2 = g_pu1AllocSA + TESTDATA_BUFFER_SZ;
            buf_data = ioremap_nocache(buf_data, TESTDATA_BUFFER_SZ);
            buf_data2 = ioremap_nocache(buf_data2, TESTDATA_BUFFER_SZ);
            do_gettimeofday(&tv2);
            calc_time_diff(&tv1, &tv2);
            printk("ioremap %d + %dMB in %d sec %d usec\n", TESTDATA_BUFFER_SZ/1024/1024, TESTDATA_BUFFER_SZ/1024/1024, tv2.tv_sec, tv2.tv_usec);

            do_gettimeofday(&tv1);
            memset(buf_data, 0, TESTDATA_BUFFER_SZ);
            memset(buf_data2, 1, TESTDATA_BUFFER_SZ);
            do_gettimeofday(&tv2);
            calc_time_diff(&tv1, &tv2);
            printk("memset %d + %dMB in %d sec %d usec\n", TESTDATA_BUFFER_SZ/1024/1024, TESTDATA_BUFFER_SZ/1024/1024, tv2.tv_sec, tv2.tv_usec);

            do_gettimeofday(&tv1);
            memcpy(buf_data, buf_data2, TESTDATA_BUFFER_SZ);
            do_gettimeofday(&tv2);
            calc_time_diff(&tv1, &tv2);
            printk("memcpy %dMB in %d sec %d usec\n", TESTDATA_BUFFER_SZ/1024/1024, tv2.tv_sec, tv2.tv_usec);
            
            iounmap(buf_data);
            iounmap(buf_data2);            

            printk("=== ioremap write test ===\n");
            p = g_pu1AllocSA;
            p = ((UINT32)p + u4Align-1) & (~(u4Align - 1));
            printk("p physical addr 0x%x\n", p);
            p = ioremap_nocache(p, u4Size);
            printk("p virtual addr 0x%x\n", p);
            *p = 'S';
            *(p+u4Size-1) = 'E';
            printk("p %c %c\n", *p, *(p+u4Size-1));
            
            iounmap(p);
            
/*

            while (1)
            {
                buf_data = (unsigned char *)vmalloc(mem_sz*1024*1024);
                if (buf_data != NULL)
                {
                    vfree(buf_data);
                    mem_sz = mem_sz + 1;
                    printk("Memory size %dMB allocated!\n", mem_sz);
                }
                else
                {
                    printk("Memory size %dMB failed to allocate\n", mem_sz);
                    break;
                }
            }
*/
            break;
        case UVVP_VDEC_TEST_VERSION_3:
            printk("UVVP_VDEC_TEST_VERSION_3\n");
            
            buf_data = (unsigned char *)vmalloc(TESTDATA_BUFFER_SZ);
            if (0 == buf_data)
            {
                printk("Allocate %d bytes failed\n", TESTDATA_BUFFER_SZ);
                return 0;
            }

            oldfs = get_fs();
            set_fs(KERNEL_DS); 
#ifdef USB_ACM_DMA1
            printk("UVVP_VDEC_TEST_VERSION_3 Open /dev/usbacm\n");
            fd = filp_open("/dev/usbacm", O_RDWR|O_NOCTTY|O_NDELAY, 0);
            if (fd == 0) 
            {   printk("Open /dev/usbacm failed\n");
                return 0;
            }
#else
            printk("UVVP_VDEC_TEST_VERSION_3 Open /dev/ttyGS0\n");
            fd = filp_open("/dev/ttyGS0", O_RDWR, 0);
            if (fd == 0) 
            {   printk("Open /dev/ttyGS0 failed\n");
                return 0;
            }
            
            fd->f_op->unlocked_ioctl(fd, TCGETS, (unsigned long)&settings);
        
            settings.c_cflag &= ~CBAUD;
            settings.c_cflag |= B921600;

            settings.c_cflag &= ~PARENB;
            settings.c_cflag &= ~CSTOPB;
            settings.c_cflag &= ~CSIZE;
            settings.c_cflag |= CS8 | CLOCAL | CREAD;
            
            settings.c_iflag &= ~(INLCR | ICRNL | IXON | IXOFF | IXANY);
            settings.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);  /*raw input*/
            settings.c_oflag &= ~OPOST;  /*raw output*/

            settings.c_cc[VMIN] = 0;
            settings.c_cc[VTIME] = 100;
            
            fd->f_op->unlocked_ioctl(fd, TCSETS, (unsigned long)&settings); 
#endif
            // Read CRC
            printk("Read CRC file\n");
            file_num = remote_open(fd, "Z:\\1280x720_30_2000.ivf.crc", 27, "rb", 2);
            if (file_num < 0)
            {
                printk("Open CRC failed\n");
                return 0;
            }
            file_len = remote_seek(fd, file_num, 0, SEEK_END);
            if (file_len < 0 || file_len > TESTDATA_BUFFER_SZ)
            {
                printk("CRC file too big %d\n", file_len);
                return 0;
            }
            remote_seek(fd, file_num, 0, SEEK_SET);
            read_len = remote_read(fd, file_num, &crc, sizeof(int));
            if (read_len < file_len)
            {
                printk("Fail to read CRC file\n");
                return 0;
            }
            remote_close(fd, file_num);

            // Read real file
            printk("Read Data file\n");
            file_num = remote_open(fd, "Z:\\1280x720_30_2000.ivf", 23, "rb", 2);
            if (file_num < 0)
            {
                printk("Open data file failed\n");
                return 0;
            }
            
            file_len = remote_seek(fd, file_num, 0, SEEK_END);
            if (file_len < 0 || file_len > TESTDATA_BUFFER_SZ)
            {
                printk("Data file too big %d\n", file_len);
                return 0;
            }            
            remote_seek(fd, file_num, 0, SEEK_SET);
            memset(&tv1, 0, sizeof(struct timeval));
            memset(&tv2, 0, sizeof(struct timeval));
            printk("Start reading data\n");
            do_gettimeofday(&tv1);
            read_len = remote_read(fd, file_num, buf_data, file_len);
            do_gettimeofday(&tv2);
            printk("End reading data\n");  
            printk("tv1 %d.%d, tv2 %d.%d \n", tv1.tv_sec, tv1.tv_usec, tv2.tv_sec, tv2.tv_usec);
            tv2.tv_sec -= tv1.tv_sec;
            if (tv2.tv_usec < tv1.tv_usec)
            {
                if (tv2.tv_sec < 1)
                    printk("TIME ERROR\n");
                tv2.tv_usec = tv2.tv_usec + 1000000 - tv1.tv_usec;
                tv2.tv_sec -= 1;
            }
            else
            {
                tv2.tv_usec -= tv1.tv_usec;
            }
            printk("Data len %d, elapsed time %d ms, avg speed %d bytes/ms\n", read_len, (tv2.tv_sec * 1000000 + tv2.tv_usec)/1000, read_len/((tv2.tv_sec * 1000000 + tv2.tv_usec)/1000));
            
            if (read_len < file_len)
            {
                printk("Fail read data file exp. %d, read %d\n", file_len, read_len);
                return 0;
            }
            remote_close(fd, file_num);

            // CRC check
            own_crc = av_adler32_update(1, buf_data, 1, read_len);
            printk("own_crc 0x%08x, crc 0x%08x, %s\n", own_crc, crc, (own_crc == crc) ? "Pass":"Fail");

            // Write file
            file_num = remote_open(fd, "D:\\Test.txt", 11, "wb", 2);
            if (file_num < 0)
            {
                printk("Open data file failed\n");
                return 0;
            }

            for (i = 0; i < 256; i++)
            {
                tmp_buf[i] = i;
            }
            
            for (i = 0; i < 1024; i++)
            {
                memcpy(buf_data+(1024*i), (unsigned char *)&tmp_buf[0], 1024);
            }

            remote_write(fd, file_num, buf_data, 1024*1024);
            
            remote_close(fd, file_num);            

            filp_close(fd, NULL);
            
            vfree(buf_data);        

	/*
	    _pucVFifo[0] = ioremap(0x8000000, 0x100000);
           memset(_pucVFifo[0] ,5,0x100000);
           printk("_pucVFifo = 0x%x\n", _pucVFifo[0]);
           printk("_pucVFifo m4u_v2p = 0x%x\n", m4u_v2p_new((unsigned int)_pucVFifo[0]));
           iounmap(_pucVFifo[0]);
	    _pucVFifo[0] = ioremap(0x8000000, 0x100000);
           memset(_pucVFifo[0] ,5,0x100000);
           printk("_pucVFifo = 0x%x\n", _pucVFifo[0]);
           printk("_pucVFifo m4u_v2p = 0x%x\n", m4u_v2p_new((unsigned int)_pucVFifo[0]));
           iounmap(_pucVFifo[0]);
	    _pucVFifo[0] = ioremap(0x8000000, 0x100000);
           memset(_pucVFifo[0] ,5,0x100000);
           printk("_pucVFifo = 0x%x\n", _pucVFifo[0]);
           printk("_pucVFifo m4u_v2p = 0x%x\n", m4u_v2p_new((unsigned int)_pucVFifo[0]));
           iounmap(_pucVFifo[0]);
*/
			break;
		default:
			break;

	}
	printk("\r\n******** uvvp_vdec_ioctl done********\r\n");
	return 0;	
}

static int uvvp_vdec_open(struct inode *inode, struct file *file)
{
	printk("\r\n******** uvvp_vdec_open********\r\n");
	return 0;
}

static int uuvp_vdec_release(struct inode *inode, struct file *file)
{
	printk("\r\n******** uuvp_vdec_release********\r\n");
	return 0;
}
static int uvvp_vdec_read(struct inode *inode, struct file *file)
{
	printk("\r\n******** uvvp_vdec_read********\r\n");
	return 0;
}
static int iuvvp_vdec_write(struct inode *inode, struct file *file)
{
	printk("\r\n******** iuvvp_vdec_write********\r\n");
	return 0;
}


static struct file_operations uvvp_vdec_fops = {
	.owner	= THIS_MODULE,		
	.open	= uvvp_vdec_open,
	.unlocked_ioctl		= uvvp_vdec_ioctl,
	.compat_ioctl = uvvp_vdec_ioctl,
	.read		= uvvp_vdec_read,
	.write		= iuvvp_vdec_write,
	.release  = uuvp_vdec_release,
};

static struct miscdevice uvvp_vdec_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = vdecname,
	.fops = &uvvp_vdec_fops,
};

static int __init uvvp_vdec_init(void)
{
	int ret;
	printk("\r\n******** uvvp_vdec_init  ********\r\n");
	ret = misc_register(&uvvp_vdec_dev);
	if(ret){
		printk("register driver failed\n");
	}
	return 0;
}

static void __exit uvvp_vdec_exit(void)
{
	int ret;
	printk("\r\n******** uvvp_vdec_exit  ********\r\n");
	ret = misc_deregister(&uvvp_vdec_dev);
	if(ret){
		printk("unregister driver failed\n");
	}

}

module_init(uvvp_vdec_init);
module_exit(uvvp_vdec_exit);


