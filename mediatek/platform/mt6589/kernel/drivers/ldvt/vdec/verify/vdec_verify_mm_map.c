#include "vdec_verify_mpv_prov.h"
#include "vdec_verify_mm_map.h"
#include "../hal/vdec_hw_common.h"

#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/sched.h>

extern char gpfH264LogFileBuffer[4096];
extern int gfpH264log;
extern unsigned int gH264logbufferOffset;
int vdecwriteFile(int fp,char *buf,int writelen);

#define DBG_H264_PRINTF
/*
#define DBG_H264_PRINTF(format,...)  \
    do { \
        if (-1 != gfpH264log) {\
            { gH264logbufferOffset += sprintf((char *)(gpfH264LogFileBuffer+gH264logbufferOffset),format, ##__VA_ARGS__);} \
            printk("gH264logbufferOffset %d\n", gH264logbufferOffset); \
            if (gH264logbufferOffset >= 3840 ) { \
                vdecwriteFile(gfpH264log, gpfH264LogFileBuffer, gH264logbufferOffset); \
                gH264logbufferOffset = 0; \
            } \
        } \
    } while (0)
*/



#define x_alloc_aligned_verify_mem(u4Size, u4Align, fgChannelA) x_alloc_aligned_verify_mem_ex(u4Size, u4Align, fgChannelA, __FUNCTION__, __LINE__)

#if VMMU_SUPPORT
static const UCHAR *g_pu1AllocSA = VMMU_SA; // need 64k align
#else
static const UCHAR *g_pu1AllocSA = FILELIST_SA;
#endif
UINT32 g_u4AllocSize = 0;

void * x_alloc_aligned_verify_mem_ex(UINT32 u4Size, UINT32 u4Align, BOOL fgChannelA, const char *szFunction, INT32 i4Line)
{
    UCHAR *p = NULL;
    UINT32 u4Mem, u4PhyAddr;
	UINT32 u4MB = u4Size / (1024 * 1024);
	UINT32 u4KB = (u4Size - u4MB * 1024 * 1024) / 1024;
	UINT32 u4B  = u4Size - u4MB * 1024 * 1024 - u4KB * 1024;

#if MEM_ALLOCATE_IOREMAP
    if (u4Align < 1024)
    {
        u4Align = 1024;
    }
    u4Size = (u4Size + u4Align-1) & (~(u4Align - 1));
    p = g_pu1AllocSA + g_u4AllocSize;
    p = ((UINT32)p + u4Align-1) & (~(u4Align - 1));
    if (g_u4AllocSize + u4Size < 0x06000000)
    //if (g_u4AllocSize + u4Size < 0x12000000)
	{
	    g_u4AllocSize += u4Size;
	}
	else
	{
	    printk("<vdec> out of memory!!!!!!\n");
		return NULL;
	}

    u4PhyAddr = (UINT32)p;
	//p = ioremap_wc(p, u4Size + 1024); // extra 1K to avoid m4u_user_v2p failed.
	p = ioremap_nocache(p, u4Size + 1024); // extra 1K to avoid m4u_user_v2p failed.
	memset(p, 0, u4Size+1024);

    printk("<vdec> ioremap %uM %uK %uB return 0x%08X (0x%08X) (%s, %d)\n", u4MB, u4KB, u4B, p, u4PhyAddr, szFunction, i4Line);
    DBG_H264_PRINTF("<vdec> ioremap %uM %uK %uB return 0x%08X (0x%08X) (%s, %d)\n", u4MB, u4KB, u4B, p, u4PhyAddr, szFunction, i4Line);

	return p;
#else

   if (u4Size > 4 * 1024 * 1024)
   {
   	 u4Size = 4* 1024 * 1024;
   }

    if(fgChannelA)
    {
#if (!CONFIG_DRV_LINUX)
        p = x_alloc_aligned_nc_mem(u4Size, u4Align);
#else
        if (fgChannelA)
           p = kmalloc(u4Size, GFP_KERNEL);
//           return vmalloc(u4Size);
        else
           p = kmalloc(u4Size, GFP_KERNEL);
//           return vmalloc(u4Size);
        //return x_alloc_aligned_dma_mem(u4Size, u4Align);

        printk("<vdec> alloc(A) %uM %uK %uB return 0x%08X (%s, %d)\n", u4MB, u4KB, u4B, p, szFunction, i4Line);

		return p;
#endif
    }
    else
    {
#if (!CONFIG_DRV_LINUX)
        u4Mem =  NONCACHE((UINT32)(x_alloc_aligned_ch2_mem(u4Size, u4Align)));
#else
        u4Mem =  kmalloc(u4Size, GFP_KERNEL);
//        u4Mem =  vmalloc(u4Size);
#endif
        return (void *)(u4Mem);
    }
#endif
}

void x_free_aligned_verify_mem(void *pvAddr, BOOL fgChannelA)
{
  if (pvAddr)
  {
#if MEM_ALLOCATE_IOREMAP
     iounmap(pvAddr);
     printk("<vdec> iounmap addr=0x%08x\n", pvAddr);
#else
#if (!CONFIG_DRV_LINUX)
        x_free_aligned_nc_mem(pvAddr);
#else

        if (fgChannelA)
           kfree(pvAddr);
        else
           kfree(pvAddr);

        //return x_free_aligned_dma_mem(pvAddr);
#endif
#endif
  }
}


void vMemoryAllocate_RM(UINT32 u4InstID)
{
#ifdef DYNAMIC_MEMORY_ALLOCATE
#ifdef DOWN_SCALE_SUPPORT
      _pucVDSCLBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(VDSCL_BUF_SZ,1024, VDSCL_CHANEL_A);
      _pucVDSCLWorkBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(VDSCL_WORK_SZ,1024, VDSCL_CHANEL_A);
      _pucVDSCLWork1Sa[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(VDSCL_SW_WORK1_SZ,1024, VDSCL_CHANEL_A);
      _pucVDSCLWork2Sa[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(VDSCL_SW_WORK2_SZ,1024, VDSCL_CHANEL_A);
      _pucVDSCLWork3Sa[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(VDSCL_SW_WORK3_SZ,1024, VDSCL_CHANEL_A);
      _pucVDSCLWork4Sa[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(VDSCL_SW_WORK4_SZ,1024, VDSCL_CHANEL_A);
#endif

#ifdef  SATA_HDD_READ_SUPPORT
  #ifndef  SATA_HDD_FS_SUPPORT
  _pucGoldenFileInfoSa[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(GOLDEN_FILE_INFO_SZ,1024, 1);
  #endif
#endif

#if VMMU_SUPPORT
  _pucVMMUTable[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(VMMU_SZ, 1024, 1);
  printk("_pucVMMUTable[u4InstID] = 0x%x\n", _pucVMMUTable[u4InstID]);
#endif

  _pucFileListSa[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(FILE_LIST_SZ,1024, 1);
  printk("_pucFileListSa[u4InstID] Address :%x!!!\n",_pucFileListSa[u4InstID]);
  DBG_H264_PRINTF("_pucFileListSa[u4InstID] Address :%x!!!\n",_pucFileListSa[u4InstID]);


  _pucVFifo[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(V_FIFO_SZ,4096, 1);
  printk("_pucVFifo[u4InstID] Address :%x!!!\n",_pucVFifo[u4InstID]);
  DBG_H264_PRINTF("_pucVFifo[u4InstID] Address :%x!!!\n",_pucVFifo[u4InstID]);
  _pucDumpYBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(GOLD_Y_SZ,1024, GOLD_CHANEL_A);
  printk("_pucDumpYBuf[u4InstID] Address :%x!!!\n",_pucDumpYBuf[u4InstID]);
  DBG_H264_PRINTF("_pucDumpYBuf[u4InstID] Address :%x!!!\n",_pucDumpYBuf[u4InstID]);
  _pucDumpCBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(GOLD_C_SZ,1024, GOLD_CHANEL_A);
  printk("_pucDumpCBuf[u4InstID] Address :%x!!!\n",_pucDumpCBuf[u4InstID]);
  DBG_H264_PRINTF("_pucDumpCBuf[u4InstID] Address :%x!!!\n",_pucDumpCBuf[u4InstID]);
  _pucPic0Y[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_Y_SZ, 2048, WORKING_AREA_CHANEL_A); //UCHAR address
  printk("_pucPic0Y[u4InstID] Address :%x!!!\n",_pucPic0Y[u4InstID]);
  DBG_H264_PRINTF("_pucPic0Y[u4InstID] Address :%x!!!\n",_pucPic0Y[u4InstID]);
  _pucPic0C[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_C_SZ,2048, WORKING_AREA_CHANEL_A);
  printk("_pucPic0C[u4InstID] Address :%x!!!\n",_pucPic0C[u4InstID]);
  DBG_H264_PRINTF("_pucPic0C[u4InstID] Address :%x!!!\n",_pucPic0C[u4InstID]);

  _pucPic1Y[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_Y_SZ, 2048, WORKING_AREA_CHANEL_A);
  printk("_pucPic1Y[u4InstID] Address :%x!!!\n",_pucPic1Y[u4InstID]);
  DBG_H264_PRINTF("_pucPic1Y[u4InstID] Address :%x!!!\n",_pucPic1Y[u4InstID]);
  _pucPic1C[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_C_SZ,2048, WORKING_AREA_CHANEL_A);
  printk("_pucPic1C[u4InstID] Address :%x!!!\n",_pucPic1C[u4InstID]);
  DBG_H264_PRINTF("_pucPic1C[u4InstID] Address :%x!!!\n",_pucPic1C[u4InstID]);

  _pucPic2Y[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_Y_SZ, 2048, WORKING_AREA_CHANEL_A);
  printk("_pucPic2Y[u4InstID] Address :%x!!!\n",_pucPic2Y[u4InstID]);
  DBG_H264_PRINTF("_pucPic2Y[u4InstID] Address :%x!!!\n",_pucPic2Y[u4InstID]);
  _pucPic2C[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_C_SZ,2048, WORKING_AREA_CHANEL_A);
  printk("_pucPic2C[u4InstID] Address :%x!!!\n",_pucPic2C[u4InstID]);
  DBG_H264_PRINTF("_pucPic2C[u4InstID] Address :%x!!!\n",_pucPic2C[u4InstID]);

  _pucPic3Y[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_Y_SZ, 2048, WORKING_AREA_CHANEL_A);
  printk("_pucPic3Y[u4InstID] Address :%x!!!\n",_pucPic3Y[u4InstID]);
  DBG_H264_PRINTF("_pucPic3Y[u4InstID] Address :%x!!!\n",_pucPic3Y[u4InstID]);
  _pucPic3C[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_C_SZ, 2048, WORKING_AREA_CHANEL_A);
  printk("_pucPic3C[u4InstID] Address :%x!!!\n",_pucPic3C[u4InstID]);
  DBG_H264_PRINTF("_pucPic3C[u4InstID] Address :%x!!!\n",_pucPic3C[u4InstID]);

  _pucRMFrmInfoBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(RM_FRMINFO_SZ, 1024, WORKING_AREA_CHANEL_A);
  printk("_pucRMFrmInfoBuf[u4InstID] Address :%x!!!\n",_pucRMFrmInfoBuf[u4InstID]);
  DBG_H264_PRINTF("_pucRMFrmInfoBuf[u4InstID] Address :%x!!!\n",_pucRMFrmInfoBuf[u4InstID]);

  _pucRMMvHwWorkBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(RM_MVHWBUF_SZ, 4096, WORKING_AREA_CHANEL_A);
  printk("_pucRMMvHwWorkBuf[u4InstID] Address :%x!!!\n",_pucRMMvHwWorkBuf[u4InstID]);
  DBG_H264_PRINTF("_pucRMMvHwWorkBuf[u4InstID] Address :%x!!!\n",_pucRMMvHwWorkBuf[u4InstID]);
  _pucRmVldPredWorkBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(RM_VLDPRED_SZ, 4096, WORKING_AREA_CHANEL_A);
  printk("_pucRmVldPredWorkBuf[u4InstID] Address :%x!!!\n",_pucRmVldPredWorkBuf[u4InstID]);
  DBG_H264_PRINTF("_pucRmVldPredWorkBuf[u4InstID] Address :%x!!!\n",_pucRmVldPredWorkBuf[u4InstID]);

  memset(_pucRMMvHwWorkBuf[u4InstID], 0, RM_MVHWBUF_SZ);
  memset(_pucRmVldPredWorkBuf[u4InstID], 0, RM_VLDPRED_SZ);

  _pucRMGoldenDataBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(RM_GOLDENDATA_SZ, 1024, WORKING_AREA_CHANEL_A);
  printk("_pucRMGoldenDataBuf[u4InstID] Address :%x!!!\n",_pucRMGoldenDataBuf[u4InstID]);
  DBG_H264_PRINTF("_pucRMGoldenDataBuf[u4InstID] Address :%x!!!\n",_pucRMGoldenDataBuf[u4InstID]);

  _pucDumpYBuf_1[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(GOLD_Y_SZ_1, 2048, WORKING_AREA_CHANEL_A);
  printk("_pucDumpYBuf_1[u4InstID] Address :%x!!!\n",_pucDumpYBuf_1[u4InstID]);
  DBG_H264_PRINTF("_pucDumpYBuf_1[u4InstID] Address :%x!!!\n",_pucDumpYBuf_1[u4InstID]);
  _pucDumpCBuf_1[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(GOLD_C_SZ_1, 2048, WORKING_AREA_CHANEL_A);
  printk("_pucDumpCBuf_1[u4InstID] Address :%x!!!\n",_pucDumpCBuf_1[u4InstID]);
  DBG_H264_PRINTF("_pucDumpCBuf_1[u4InstID] Address :%x!!!\n",_pucDumpCBuf_1[u4InstID]);

  #ifdef DOWN_SCALE_SUPPORT
  _pucVDSCLBuf_1[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(VDSCL_BUF_SZ_1, 1024, WORKING_AREA_CHANEL_A);

  //Only for RV8 Debug
  //_pucRMMCOutputBufY[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(RM_MCOUT_Y_SZ, 1024, WORKING_AREA_CHANEL_A);
  //_pucRMMCOutputBufC[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(RM_MCOUT_C_SZ, 1024, WORKING_AREA_CHANEL_A);
  #endif //DOWN_SCALE_SUPPORT

  _pucRMReszWorkBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(RM_RSZWORKBUF_SZ, 1024, WORKING_AREA_CHANEL_A);
  _pucRMRingWorkBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(RM_RINGFLOW_TEMPFIFO_SZ, 1024, WORKING_AREA_CHANEL_A);

  _pucRMAULikeBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(RM_AULIKEBUF_SZ, 1024, WORKING_AREA_CHANEL_A);

  _pucRMChecksumBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(RM_CHECKSUM_BUFFER_SZ, 1024, WORKING_AREA_CHANEL_A);

  #ifdef RM_CRCCHECKFLOW_SUPPORT
  _pucRMCRCResultBuf[u4InstID] =  (UCHAR *)x_alloc_aligned_verify_mem(RM_CRCRESULT_SZ, 8, WORKING_AREA_CHANEL_A);
  #endif //RM_CRCCHECKFLOW_SUPPORT

  _pucVP6VLDWrapperWorkspace[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(15*4096, 1024, WORKING_AREA_CHANEL_A);
  _pucVP6PPWrapperWorkspace[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(30*4096, 1024, WORKING_AREA_CHANEL_A);
  printk("_pucVP6VLDWrapperWorkspace[u4InstID] = 0x%x\n", _pucVP6VLDWrapperWorkspace[u4InstID]);
  DBG_H264_PRINTF("_pucVP6VLDWrapperWorkspace[u4InstID] = 0x%x\n", _pucVP6VLDWrapperWorkspace[u4InstID]);
  printk("_pucVP6PPWrapperWorkspace[u4InstID] = 0x%x\n", _pucVP6PPWrapperWorkspace[u4InstID]);
  DBG_H264_PRINTF("_pucVP6PPWrapperWorkspace[u4InstID] = 0x%x\n", _pucVP6PPWrapperWorkspace[u4InstID]);

#ifdef CAPTURE_ESA_LOG
       _pucESALog[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(ESALOG_SZ, 1024, 1);
      printk("_pucESALog[u4InstID] = 0x%x\n", _pucESALog[u4InstID]);
      _pucESALog[1] = (UCHAR *)x_alloc_aligned_verify_mem(ESALOG_SZ, 1024, 1);
      printk("_pucESALog[u4InstID] = 0x%x\n", _pucESALog[1]);
      _pucESATotalBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(ESALOGTOTAL_SZ, 1024, 1);
      printk("_pucESATotalBufpu4InstId] = 0x%x\n", _pucESATotalBuf[u4InstID]);
#endif

  #else //DYNAMIC_MEMORY_ALLOCATE
  //Don't Support in RM Decoder Emulation/Verification Flow
  ASSERT(0);
  #endif //DYNAMIC_MEMORY_ALLOCATE
}


int m4u_v2p(unsigned int va)
{
   unsigned int pmdOffset = (va & (PMD_SIZE - 1));
   unsigned int pageOffset = (va & (PAGE_SIZE - 1));
   pgd_t *pgd;
   pmd_t *pmd;
   pte_t *pte;
   unsigned int pa;
//   printk("Enter m4u_user_v2p()! 0x%x\n", va);
   
   pgd = pgd_offset(current->mm, va); /* what is tsk->mm */
 //  printk("m4u_user_v2p(), pgd 0x%x\n", pgd);    
 //  printk("pgd_none=%d, pgd_bad=%d\n", pgd_none(*pgd), pgd_bad(*pgd));
   
   if(pgd_none(*pgd)||pgd_bad(*pgd))
   {
      printk("Error: m4u_user_v2p(), virtual addr 0x%x, pgd invalid! \n", va);
      return 0;
   }
   pmd = pmd_offset(pgd, va);
 //  printk("m4u_user_v2p(), pmd 0x%x\n", pmd);
 //  printk("pmd_none=%d, pmd_bad=%d, pmd_val=0x%x\n", pmd_none(*pmd), pmd_bad(*pmd), pmd_val(*pmd));
   
   
   /* If this is a page table entry, keep on walking to the next level */ 
   if (( (unsigned int)pmd_val(*pmd) & PMD_TYPE_MASK) == PMD_TYPE_TABLE)
   {
      if(pmd_none(*pmd)||pmd_bad(*pmd))
      {
         printk("Error: m4u_user_v2p(), virtual addr 0x%x, pmd invalid! \n", va);
         return 0;
      }
      
      pte = pte_offset_map(pmd, va);
 //     printk("m4u_user_v2p(), pte 0x%x\n", pte);
      if(pte_present(*pte)) 
      { 
         pa=(pte_val(*pte) & (PAGE_MASK)) | pageOffset; 
  //       printk("PA = 0x%8x\n", pa);
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
    //  printk("PA = 0x%8x\n", pa);
      return pa;

   }
   return 0;   
}



void* vMemoryAllocateLoop(UINT32 u4Size)
{
    int j;
    UCHAR *pucmem = NULL; 
    UINT32 u4Count;
    UINT32 u4Res;
    if (u4Size > KMALLOC_SZ)
    	{
    	u4Res = u4Size%KMALLOC_SZ;
    	u4Count = u4Size/KMALLOC_SZ;
    	printk("allocate : Res = 0x%x, u4Count = 0x%x\n", u4Res, u4Count);
    	for (j=0; j<u4Count; j++)
    		{
    		pucmem = (UCHAR *)x_alloc_aligned_verify_mem(KMALLOC_SZ,1024, 1); // 4M
    		printk("allocate memory SA = 0x%x\n", pucmem);
    		}
    	if (u4Res)
    		{
      		pucmem = (UCHAR *)x_alloc_aligned_verify_mem(u4Res,1024, 1); // 4M
    		printk("allocate Res memory SA = 0x%x\n", pucmem);
  		}
    	}
    else
    	{
    	pucmem = (UCHAR *)x_alloc_aligned_verify_mem(u4Size,1024, 1); // 4M
    	}
    return pucmem;
}
void* vMemoryFreeLoop(void *pvAddr, UINT32 u4Size)
{
    int j;
    UCHAR *pucmem; 
    UINT32 u4Count;
    UINT32 u4Res;
    if (u4Size > KMALLOC_SZ)
    	{
    	u4Res = u4Size%KMALLOC_SZ;
    	u4Count = u4Size/KMALLOC_SZ;
    	printk("Free 1 : Res = 0x%x, u4Count = 0x%x\n", u4Res, u4Count);
    	if (u4Res)
    		{
    		pucmem = pvAddr;
    		printk("Free Res memory SA = 0x%x\n", pucmem);
              x_free_aligned_verify_mem(pucmem, 1);
    		printk("Free Res memory ok\n");
  		}
    	for (j=0; j<u4Count; j++)
    		{
    		pucmem = pvAddr+u4Res+(KMALLOC_SZ*j);
    		printk("Free memory SA = 0x%x\n", pucmem);
              x_free_aligned_verify_mem(pucmem, 1);
    		printk("Free memory ok\n");
    		}
    	}
    else
    	{
    		pucmem = pvAddr;
    		printk("Free 2 memory SA = 0x%x\n", pucmem);
              x_free_aligned_verify_mem(pucmem, 1);
    	}
    return pucmem;
}

// *********************************************************************
// Function    : void vMemoryAllocate(UINT32 u4InstID)
// Description : allocate memory
// Parameter   : None
// Return      : None
// *********************************************************************
void vMemoryAllocate(UINT32 u4InstID)
{
#ifdef VDEC_VIDEOCODEC_RM
  vMemoryAllocate_RM(u4InstID);    //RM Decoder Memory Allocation Path
  #if VDEC_DRAM_BUSY_TEST
  _pucDramBusy[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(0x10000,2048, WORKING_AREA_CHANEL_A);
  printk("_pucDramBusy[u4InstID] = 0x%x\n", _pucDramBusy[u4InstID]);
  #endif
#else //VDEC_VIDEOCODEC_RM

#ifdef DYNAMIC_MEMORY_ALLOCATE

#ifdef  SATA_HDD_READ_SUPPORT
   #ifndef  SATA_HDD_FS_SUPPORT
  //if(_pucGoldenFileInfoSa[u4InstID] == NULL)
  {
    _pucGoldenFileInfoSa[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(GOLDEN_FILE_INFO_SZ,1024, 1);
    printk("// <buffer> _pucGoldenFileInfoSa[u4InstID] = 32'h%x\n", PHYSICAL(_pucGoldenFileInfoSa[u4InstID]));
    DBG_H264_PRINTF("// <buffer> _pucGoldenFileInfoSa[u4InstID] = 32'h%x\n", PHYSICAL(_pucGoldenFileInfoSa[u4InstID]));
  }
#endif
#endif

#ifdef LETTERBOX_SUPPORT
  {
    _pucSettingFileSa[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(FILE_LIST_SZ,1024, 1);
    printk("_pucSettingFileSa[u4InstID] = 0x%x\n", _pucSettingFileSa[u4InstID]);
    _pucGoldenFileSa[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(FILE_LIST_SZ,1024, 1);
    printk("_pucGoldenFileSa[u4InstID] = 0x%x\n", _pucGoldenFileSa[u4InstID]);
  }
#endif

  //#if MEM_ALLOCATE_IOREMAP
  #if 0
#if VMMU_SUPPORT
       _pucVMMUTable[u4InstID] = ioremap_nocache(VMMU_SA, VMMU_SZ);
       memset(_pucVMMUTable[u4InstID] , 6,VMMU_SZ);
      printk("_pucVMMUTable[u4InstID] = 0x%x\n", _pucVMMUTable[u4InstID]);
#endif

  //if(_pucFileListSa[u4InstID] == NULL)
  {
 //   printk("FILELIST_SA = 0x%x\n", FILELIST_SA);
    _pucFileListSa[u4InstID] = ioremap_nocache(FILELIST_SA, FILE_LIST_SZ);
    memset(_pucFileListSa[u4InstID] ,0,FILE_LIST_SZ);
    printk("_pucFileListSa[u4InstID] = 0x%x\n", _pucFileListSa[u4InstID]);
//    printk("_pucVFifo m4u_v2p = 0x%x\n", m4u_v2p((unsigned int)_pucFileListSa[u4InstID]));
  }
  
 // if(_pucVFifo[u4InstID] == NULL)
  if(u4InstID == 0 && _ucMVCType[1] && _pucVFifo[1])
  {
      _pucVFifo[0] = _pucVFifo[1];
  }
  else
  {
//    printk("VFIFO_SA = 0x%x\n", VFIFO_SA);
    _pucVFifo[u4InstID] = ioremap_nocache(VFIFO_SA, V_FIFO_SZ);
    memset(_pucVFifo[u4InstID] ,5,V_FIFO_SZ);
    printk("_pucVFifo[u4InstID] = 0x%x 0x%x 0x%x\n", _pucVFifo[u4InstID], VFIFO_SA, V_FIFO_SZ);
//    printk("_pucVFifo m4u_v2p = 0x%x\n", m4u_v2p((unsigned int)_pucVFifo[u4InstID]));
  }
  
 // if(_pucDumpYBuf[u4InstID] == NULL)
  {
 //   printk("GOLDY_SA = 0x%x\n", GOLDY_SA);
    _pucDumpYBuf[u4InstID] = ioremap_nocache(GOLDY_SA, GOLD_Y_SZ);
    memset(_pucDumpYBuf[u4InstID],0,GOLD_Y_SZ);
        printk("_pucDumpYBuf[u4InstID] = 0x%x 0x%x 0x%x\n", _pucDumpYBuf[u4InstID], GOLDY_SA, GOLD_Y_SZ);
  }
  //if(_pucDumpCBuf[u4InstID] == NULL)
  {
 //   printk("GOLDC_SA = 0x%x\n", GOLDC_SA);
        _pucDumpCBuf[u4InstID] = ioremap_nocache(GOLDC_SA, GOLD_C_SZ);
    memset(_pucDumpCBuf[u4InstID],0,GOLD_C_SZ);
        printk("_pucDumpCBuf[u4InstID] = 0x%x 0x%x 0x%x\n", _pucDumpCBuf[u4InstID], GOLDC_SA, GOLD_C_SZ);
  }

//    printk("CRCBUF_SA = 0x%x\n", CRCBUF_SA);
  _pucCRCBuf[u4InstID]= ioremap_nocache(CRCBUF_SA, CRC_SZ);
    memset(_pucCRCBuf[u4InstID],0,CRC_SZ);
  printk("_pucCRCBuf[u4InstID] = 0x%x 0x%x 0x%x\n", _pucCRCBuf[u4InstID], CRCBUF_SA, CRC_SZ);

 #ifdef CAPTURE_ESA_LOG
       _pucESALog[u4InstID] = ioremap_nocache(ESALOG_SA, ESALOG_SZ);
       memset(_pucESALog[u4InstID] ,1,ESALOG_SZ);
      printk("_pucESALog[u4InstID] = 0x%x 0x%x 0x%x\n", _pucESALog[u4InstID], ESALOG_SA, ESALOG_SZ);
#endif

#elif 1
#if VMMU_SUPPORT
       _pucVMMUTable[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(VMMU_SZ, 1024, 1);
      printk("_pucVMMUTable[u4InstID] = 0x%x\n", _pucVMMUTable[u4InstID]);
#endif
  {
    _pucFileListSa[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(FILE_LIST_SZ, 1024, 1);
    printk("// <buffer> _pucFileListSa[u4InstID] = 32'h%x\n", PHYSICAL(_pucFileListSa[u4InstID]));
    DBG_H264_PRINTF("// <buffer> _pucFileListSa[u4InstID] = 32'h%x\n", PHYSICAL(_pucFileListSa[u4InstID]));
    printk("_pucFileListSa[u4InstID] = 0x%x\n", _pucFileListSa[u4InstID]);    
    DBG_H264_PRINTF("_pucFileListSa[u4InstID] = 0x%x\n", _pucFileListSa[u4InstID]);    
  }

  if(u4InstID == 0 && _ucMVCType[1] && _pucVFifo[1])
  {
      _pucVFifo[0] = _pucVFifo[1];
  }
  else
  {
    _pucVFifo[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(V_FIFO_SZ, 1024, 1);
    printk("// <buffer> bitstream dram physical address: 32'h%x\n", PHYSICAL(_pucVFifo[u4InstID]));
    DBG_H264_PRINTF("// <buffer> bitstream dram physical address: 32'h%x\n", PHYSICAL(_pucVFifo[u4InstID]));
    printk("_pucVFifo[u4InstID] = 0x%x\n", _pucVFifo[u4InstID]);
    DBG_H264_PRINTF("_pucVFifo[u4InstID] = 0x%x\n", _pucVFifo[u4InstID]);
  }
  
  {
        _pucDumpYBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(GOLD_Y_SZ, 1024, 1);
        printk("// <buffer> _pucDumpYBuf[u4InstID] = 32'h%x\n", PHYSICAL(_pucDumpYBuf[u4InstID]));
        DBG_H264_PRINTF("// <buffer> _pucDumpYBuf[u4InstID] = 32'h%x\n", PHYSICAL(_pucDumpYBuf[u4InstID]));
        printk("_pucDumpYBuf[u4InstID] = 0x%x\n", _pucDumpYBuf[u4InstID]);
        DBG_H264_PRINTF("_pucDumpYBuf[u4InstID] = 0x%x\n", _pucDumpYBuf[u4InstID]);
  }

  {
        _pucDumpCBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(GOLD_C_SZ, 1024, 1);
        printk("// <buffer> _pucDumpCBuf[u4InstID] = 32'h%x\n", PHYSICAL(_pucDumpCBuf[u4InstID]));
        DBG_H264_PRINTF("// <buffer> _pucDumpCBuf[u4InstID] = 32'h%x\n", PHYSICAL(_pucDumpCBuf[u4InstID]));
        printk("_pucDumpCBuf[u4InstID] = 0x%x\n", _pucDumpCBuf[u4InstID]);
        DBG_H264_PRINTF("_pucDumpCBuf[u4InstID] = 0x%x\n", _pucDumpCBuf[u4InstID]);
  }

  _pucCRCBuf[u4InstID]= (UCHAR *)x_alloc_aligned_verify_mem(32, 1024, 1);
  printk("// <buffer> _pucCRCBuf[u4InstID] = 32'h%x\n", PHYSICAL(_pucCRCBuf[u4InstID]));
  DBG_H264_PRINTF("// <buffer> _pucCRCBuf[u4InstID] = 32'h%x\n", PHYSICAL(_pucCRCBuf[u4InstID]));
  printk("_pucCRCBuf[u4InstID] = 0x%x\n", _pucCRCBuf[u4InstID]);
  DBG_H264_PRINTF("_pucCRCBuf[u4InstID] = 0x%x\n", _pucCRCBuf[u4InstID]);
  _pucRegister[0]= (UCHAR *)x_alloc_aligned_verify_mem(32*1024, 1024, 1);
  printk("// <buffer> _pucRegister[0] = 32'h%x\n", PHYSICAL(_pucRegister[0]));
  DBG_H264_PRINTF("// <buffer> _pucRegister[0] = 32'h%x\n", PHYSICAL(_pucRegister[0]));
  printk("_pucRegister[0] = 0x%x\n", _pucRegister[0]);
  DBG_H264_PRINTF("_pucRegister[0] = 0x%x\n", _pucRegister[0]);
  _pucRegister[1]= (UCHAR *)x_alloc_aligned_verify_mem(32*1024, 1024, 1);
  printk("// <buffer> _pucRegister[1] = 32'h%x\n", PHYSICAL(_pucRegister[1]));
  DBG_H264_PRINTF("// <buffer> _pucRegister[1] = 32'h%x\n", PHYSICAL(_pucRegister[1]));
  printk("_pucRegister[0] = 0x%x\n", _pucRegister[1]);
  DBG_H264_PRINTF("_pucRegister[0] = 0x%x\n", _pucRegister[1]);

#if AVC_NEW_CRC_COMPARE
  _pucH264CRCYBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(400*1024, 1024, 1);
  printk("_pucH264CRCYBuf[u4InstID] = 0x%x\n", _pucH264CRCYBuf[u4InstID]);
  DBG_H264_PRINTF("_pucH264CRCYBuf[u4InstID] = 0x%x\n", _pucH264CRCYBuf[u4InstID]);
#endif
#ifdef MPEG4_CRC_CMP //////need to check start address mtk40343
  _pucCRCYBuf[u4InstID] = x_alloc_aligned_verify_mem(500*1024, 1024,1);
  _pucCRCCBCRBuf[u4InstID] = x_alloc_aligned_verify_mem(500*1024, 1024,1);
  printk("// <buffer> _pucCRCYBuf[u4InstID] = 32'h%x\n",PHYSICAL(_pucCRCYBuf[u4InstID]));
  DBG_H264_PRINTF("// <buffer> _pucCRCYBuf[u4InstID] = 32'h%x\n",PHYSICAL(_pucCRCYBuf[u4InstID]));
  printk("// <buffer> _pucCRCCBCRBuf[u4InstID] = 32'h%x\n",PHYSICAL(_pucCRCCBCRBuf[u4InstID]));
  DBG_H264_PRINTF("// <buffer> _pucCRCCBCRBuf[u4InstID] = 32'h%x\n",PHYSICAL(_pucCRCCBCRBuf[u4InstID]));
  printk("_pucCRCYBuf[u4InstID] = 0x%x\n",_pucCRCYBuf[u4InstID]);
  DBG_H264_PRINTF("_pucCRCYBuf[u4InstID] = 0x%x\n",_pucCRCYBuf[u4InstID]);
  printk("_pucCRCCBCRBuf[u4InstID] = 0x%x\n",_pucCRCCBCRBuf[u4InstID]);
  DBG_H264_PRINTF("_pucCRCCBCRBuf[u4InstID] = 0x%x\n",_pucCRCCBCRBuf[u4InstID]);
  #endif
#ifdef CAPTURE_ESA_LOG
       _pucESALog[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(ESALOG_SZ, 1024, 1);
      printk("_pucESALog[u4InstID] = 0x%x\n", _pucESALog[u4InstID]);
      _pucESALog[1] = (UCHAR *)x_alloc_aligned_verify_mem(ESALOG_SZ, 1024, 1);
      printk("_pucESALog[u4InstID] = 0x%x\n", _pucESALog[1]);
      _pucESATotalBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(ESALOGTOTAL_SZ, 1024, 1);
      printk("_pucESATotalBufpu4InstId] = 0x%x\n", _pucESATotalBuf[u4InstID]);
#endif

#ifdef REG_LOG_NEW
  _pucRegisterLog[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(2*1024*1024, 1024, 1);
  printk("_pucRegisterLog[u4InstID] = 0x%x\n", _pucRegisterLog[u4InstID]);
  DBG_H264_PRINTF("_pucRegisterLog[u4InstID] = 0x%x\n", _pucRegisterLog[u4InstID]);
#endif

#else
  {
    _pucFileListSa[u4InstID] = (UCHAR *)vMemoryAllocateLoop(FILE_LIST_SZ);
    printk("_pucFileListSa[u4InstID] = 0x%x\n", _pucFileListSa[u4InstID]);
  }

  if(u4InstID == 0 && _ucMVCType[1] && _pucVFifo[1])
  {
      _pucVFifo[0] = _pucVFifo[1];
  }
  else
  {
    _pucVFifo[u4InstID] = (UCHAR *)vMemoryAllocateLoop(V_FIFO_SZ);
    printk("_pucVFifo[u4InstID] = 0x%x\n", _pucVFifo[u4InstID]);
  }
  
  {
        _pucDumpYBuf[u4InstID] = (UCHAR *)vMemoryAllocateLoop(GOLD_Y_SZ);
        printk("_pucDumpYBuf[u4InstID] = 0x%x, phy:0x%x\n", _pucDumpYBuf[u4InstID], PHYSICAL(_pucDumpYBuf[u4InstID]));
  }

  {
        _pucDumpCBuf[u4InstID] = (UCHAR *)vMemoryAllocateLoop(GOLD_C_SZ);
        printk("_pucDumpCBuf[u4InstID] = 0x%x, phy:0x%x\n", _pucDumpCBuf[u4InstID], PHYSICAL(_pucDumpCBuf[u4InstID]));
  }

  _pucCRCBuf[u4InstID]= (UCHAR *)vMemoryAllocateLoop(32);
  printk("_pucCRCBuf[u4InstID] = 0x%x, phy:0x%x\n", _pucCRCBuf[u4InstID], PHYSICAL(_pucCRCBuf[u4InstID]));

#ifdef CAPTURE_ESA_LOG
       _pucESALog[u4InstID] = (UCHAR *)vMemoryAllocateLoop(ESALOG_SZ);
      printk("_pucESALog[u4InstID] = 0x%x\n", _pucESALog[u4InstID]);
#endif

#endif

     _pucDPB[u4InstID] = 0;
     _pucPredSa[u4InstID] = 0;
     #ifdef VERIFICATION_FGT
       _pucFGSeedbase[u4InstID] = 0;
       _pucFGDatabase[u4InstID] = 0;
       _pucFGSEISa[u4InstID] = 0;
      _pucFGTBuf[u4InstID] = 0;
     #endif

      _pucAddressSwapBuf[u4InstID] = 0;

#ifdef DOWN_SCALE_SUPPORT
       _pucVDSCLBuf[u4InstID] = 0;
      _pucVDSCLWorkBuf[u4InstID] = 0;
      _pucVDSCLWork1Sa[u4InstID] = 0;
      _pucVDSCLWork2Sa[u4InstID] = 0;
      _pucVDSCLWork3Sa[u4InstID] = 0;
      _pucVDSCLWork4Sa[u4InstID] = 0;
#endif

#if VDEC_AVS_EMU
{
    #if MEM_ALLOCATE_IOREMAP
    _pucPic0Y[u4InstID] = ioremap_nocache(AVS_PIC_Y0, PIC_Y_SZ+PIC_C_SZ);
    memset(_pucPic0Y[u4InstID] ,0,PIC_Y_SZ+PIC_C_SZ);
    _pucPic1Y[u4InstID] = ioremap_nocache(AVS_PIC_Y1, PIC_Y_SZ+PIC_C_SZ);
    memset(_pucPic1Y[u4InstID] ,0,PIC_Y_SZ+PIC_C_SZ);
    _pucPic2Y[u4InstID] = ioremap_nocache(AVS_PIC_Y2, PIC_Y_SZ+PIC_C_SZ);
    memset(_pucPic2Y[u4InstID] ,0,PIC_Y_SZ+PIC_C_SZ);
    _pucPic3Y[u4InstID] = ioremap_nocache(AVS_PIC_Y3, PIC_Y_SZ+PIC_C_SZ);
    memset(_pucPic3Y[u4InstID] ,0,PIC_Y_SZ+PIC_C_SZ);
    _pucAvsPred[u4InstID] = ioremap_nocache(AVS_VLD_PRED_BUF,AVS_VLD_PRED_SZ);
    memset(_pucAvsPred[u4InstID] ,0,AVS_VLD_PRED_SZ);
    _pucAvsMv1[u4InstID] = ioremap_nocache(AVS_VLD_MV1_BUF,AVS_VLD_MV_SZ);
    memset(_pucAvsMv1[u4InstID] ,0,AVS_VLD_MV_SZ);
    _pucAvsMv2[u4InstID] = ioremap_nocache(AVS_VLD_MV2_BUF, AVS_VLD_MV_SZ);
    memset(_pucAvsMv2[u4InstID] ,0,AVS_VLD_MV_SZ);
    _pucPic0C[u4InstID] = _pucPic0Y[u4InstID]  + PIC_Y_SZ;
    _pucPic1C[u4InstID] = _pucPic1Y[u4InstID]  + PIC_Y_SZ;
    _pucPic2C[u4InstID] = _pucPic2Y[u4InstID]  + PIC_Y_SZ;
    _pucPic3C[u4InstID] = _pucPic3Y[u4InstID]  + PIC_Y_SZ;

    _pucVLDWrapperWrok[u4InstID] = ioremap_nocache(AVS_VLDWRAPWORK_SA, VLD_WRAP_SZ);
    memset(_pucVLDWrapperWrok[u4InstID] ,0,VLD_WRAP_SZ);
    _pucPPWrapperWork[u4InstID] = ioremap_nocache(AVS_PPWRAPWORK_SA, PP_WRAP_SZ);
    memset(_pucPPWrapperWork[u4InstID] ,0,PP_WRAP_SZ);
    #else
    _pucPic0Y[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_Y_SZ,2048, WORKING_AREA_CHANEL_A); //UCHAR address
    _pucPic0C[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_C_SZ,2048, WORKING_AREA_CHANEL_A);
    _pucPic1Y[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_Y_SZ,2048, WORKING_AREA_CHANEL_A);
    _pucPic1C[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_C_SZ,2048, WORKING_AREA_CHANEL_A);
    _pucPic2Y[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_Y_SZ,2048, WORKING_AREA_CHANEL_A);
    _pucPic2C[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_C_SZ,2048, WORKING_AREA_CHANEL_A);
    _pucPic3Y[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_Y_SZ,2048, WORKING_AREA_CHANEL_A);
    _pucPic3C[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_C_SZ,2048, WORKING_AREA_CHANEL_A);
    _pucAvsPred[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(AVS_VLD_PRED_SZ,1024, WORKING_AREA_CHANEL_A);
    _pucAvsMv1[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(AVS_VLD_MV_SZ,1024, WORKING_AREA_CHANEL_A);
    _pucAvsMv2[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(AVS_VLD_MV_SZ,1024, WORKING_AREA_CHANEL_A);
    #endif
    printk("_pucPic0Y[u4InstID] = 0x%x, 0x%x, size 0x%x\n", (UINT32)_pucPic0Y[u4InstID], AVS_PIC_Y0, PIC_Y_SZ);
    printk("_pucPic0C[u4InstID] = 0x%x\n", (UINT32)_pucPic0C[u4InstID]);
    printk("_pucPic1Y[u4InstID] = 0x%x, 0x%x, size 0x%x\n", (UINT32)_pucPic1Y[u4InstID], AVS_PIC_Y1, PIC_Y_SZ);
    printk("_pucPic1C[u4InstID] = 0x%x\n", (UINT32)_pucPic1C[u4InstID]);
    printk("_pucPic2Y[u4InstID] = 0x%x, 0x%x, size 0x%x\n", (UINT32)_pucPic2Y [u4InstID], AVS_PIC_Y2, PIC_Y_SZ);
    printk("_pucPic2C[u4InstID] = 0x%x\n", (UINT32)_pucPic2C[u4InstID]);
    printk("_pucPic3Y[u4InstID] = 0x%x, 0x%x, size 0x%x\n", (UINT32)_pucPic3Y [u4InstID], AVS_PIC_Y3, PIC_Y_SZ);
    printk("_pucPic3C[u4InstID] = 0x%x\n", (UINT32)_pucPic3C[u4InstID]);

    printk("_pucDcac[u4InstID] = 0x%x 0x%x 0x%x\n", (UINT32)_pucAvsPred[u4InstID], AVS_VLD_PRED_BUF, AVS_VLD_PRED_SZ);
    printk("_pucMv_1[u4InstID] = 0x%x 0x%x 0x%x\n", (UINT32)_pucAvsMv1[u4InstID], AVS_VLD_MV1_BUF, AVS_VLD_MV_SZ);
    printk("_pucMv_2[u4InstID] = 0x%x 0x%x 0x%x\n", (UINT32)_pucAvsMv2[u4InstID], AVS_VLD_MV2_BUF, AVS_VLD_MV_SZ);
    printk("_pucVLDWrapperWrok[u4InstID] = 0x%x 0x%x 0x%x\n", (UINT32)_pucVLDWrapperWrok[u4InstID], AVS_VLDWRAPWORK_SA, VLD_WRAP_SZ);
    printk("_pucPPWrapperWork[u4InstID] = 0x%x 0x%x 0x%x\n", (UINT32)_pucPPWrapperWork[u4InstID], AVS_PPWRAPWORK_SA, PP_WRAP_SZ);

    if (_u2AddressSwapMode[u4InstID] != ADDRSWAP_OFF)
    {
        #if MEM_ALLOCATE_IOREMAP
        _pucAddressSwapBuf[u4InstID] = ioremap_nocache(AVS_ADDSWAP_BUF, ADDSWAP_BUF_SZ);
        memset(_pucAddressSwapBuf[u4InstID] ,0,ADDSWAP_BUF_SZ);
        #else
        _pucAddressSwapBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(ADDSWAP_BUF_SZ, 2048, WORKING_AREA_CHANEL_A);
        #endif
        printk("_pucAddressSwapBuf[u4InstID] = 0x%x\n", (UINT32)_pucAddressSwapBuf[u4InstID]);
    }
}
#endif

#if VDEC_DRAM_BUSY_TEST
     _pucDramBusy[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(0x10000,2048, WORKING_AREA_CHANEL_A);
#endif

#else

#ifdef VERIFICATION_FGT
  _pucFGSeedbase[u4InstID] = (UCHAR *)FGT_SEED_SA;
  _pucFGDatabase[u4InstID] = (UCHAR *)FGT_DATABASE_SA;
  _pucFGSEISa[u4InstID] = (UCHAR *)FGT_SEI_SA;
  _pucFGTBuf[u4InstID] = (UCHAR *)FGT_SA;
#endif

#if 1//def DOWN_SCALE_SUPPORT
  _pucVDSCLBuf[u4InstID] = (UCHAR *)VDSCL_BUF_SA;
  _pucVDSCLWorkBuf[u4InstID] = (UCHAR *)VDSCL_WORK_SA;
  _pucVDSCLWork1Sa[u4InstID] = (UCHAR *)VDSCL_SW_WORK1_SA;
  _pucVDSCLWork2Sa[u4InstID] = (UCHAR *)VDSCL_SW_WORK2_SA;
  _pucVDSCLWork3Sa[u4InstID] = (UCHAR *)VDSCL_SW_WORK3_SA;
  _pucVDSCLWork4Sa[u4InstID] = (UCHAR *)VDSCL_SW_WORK4_SA;
#endif
  _pucFileListSa[u4InstID] = (UCHAR*)FILE_LIST_SA;
  _pucVFifo[u4InstID] = (UCHAR *)V_FIFO_SA;
  _pucDPB[u4InstID] = (UCHAR *)DPB_SA;
  _pucPredSa[u4InstID] = (UCHAR *)AVC_PRED_SA;
  _pucDumpYBuf[u4InstID] = (UCHAR *)GOLD_Y_SA;
  _pucDumpCBuf[u4InstID] = (UCHAR *)GOLD_C_SA;
  // WMV Part
  _pucPic0Y[u4InstID] = (UCHAR *)PIC0Y_SA; //UCHAR address
  _pucPic0C[u4InstID] = (UCHAR *)PIC0C_SA;
  _pucPic1Y[u4InstID] = (UCHAR *)PIC1Y_SA;
  _pucPic1C[u4InstID] = (UCHAR *)PIC1C_SA;
  _pucPic2Y[u4InstID] = (UCHAR *)PIC2Y_SA;
  _pucPic2C[u4InstID] = (UCHAR *)PIC2C_SA;
  _pucDcac[u4InstID] = (UCHAR *)DCAC_SA;
  _pucMv_1[u4InstID] = (UCHAR *)Mv_1;
  _pucMv_2[u4InstID] = (UCHAR *)Mv_2;
  _pucBp_1[u4InstID] = (UCHAR *)Bp_1;
  _pucBp_2[u4InstID] = (UCHAR *)Bp_2;
  _pucBp_3[u4InstID] = (UCHAR *)Bp_3;
  _pucBp_4[u4InstID] = (UCHAR *)Bp_4;
  _pucMv_3[u4InstID] = (UCHAR *)Mv_3;
  _pucMv_1_2[u4InstID] = (UCHAR *)Mv_1_2;
  _pucDcac_2[u4InstID] = (UCHAR *)DCAC_2;
  _pucPp_1[u4InstID] = (UCHAR *)DEC_PP_1;
  _pucPp_2[u4InstID] = (UCHAR *)DEC_PP_2;
  // MPEG part
  _pucPpYSa[u4InstID] = (UCHAR *)DEC_PP_Y_SA;
  _pucPpCSa[u4InstID] = (UCHAR *)DEC_PP_C_SA;
  _pucMp4Dcac[u4InstID] = (UCHAR *)MP4_DCAC_SA;
  _pucMp4Mvec[u4InstID] = (UCHAR *)VER_MVEC_SA;
  _pucMp4Bmb1[u4InstID] = (UCHAR *)VER_BMB1_SA;
  _pucMp4Bmb2[u4InstID] = (UCHAR *)VER_BMB2_SA;
  _pucMp4Bcode[u4InstID] = (UCHAR *)BCODE_SA;

    _pucDcacNew[u4InstID] = (UCHAR *)DCAC_NEW_SA;
    _pucMvNew[u4InstID] = (UCHAR *)MV_NEW_SA;
    _pucBp0New[u4InstID] = (UCHAR *)BP_0_NEW_SA;
    _pucBp1New[u4InstID] = (UCHAR *)BP_1_NEW_SA;
    _pucBp2New[u4InstID] = (UCHAR *)BP_2_NEW_SA;
#endif
#endif //VDEC_VIDEOCODEC_RM
}


// *********************************************************************
// Function    : void vMemoryFree(UINT32 u4InstID)
// Description : Free memory
// Parameter   : None
// Return      : None
// *********************************************************************
void vMemoryFree(UINT32 u4InstID)
{
#ifdef DYNAMIC_MEMORY_ALLOCATE
  #if (MEM_ALLOCATE_IOREMAP)
  if(_ucMVCType[u4InstID] != 1)
       iounmap(_pucVFifo[u4InstID]);
  
       iounmap(_pucFileListSa[u4InstID]);
       iounmap(_pucDumpYBuf[u4InstID]);
       iounmap(_pucDumpCBuf[u4InstID]);
       iounmap(_pucCRCBuf[u4InstID]);

       #if AVC_NEW_CRC_COMPARE
       iounmap(_pucH264CRCYBuf[u4InstID]);
       #endif
       
       #ifdef CAPTURE_ESA_LOG
       iounmap(_pucESALog[u4InstID]);
       iounmap(_pucESALog[1]);
       iounmap(_pucESATotalBuf[u4InstID]);
       #endif
       #if VMMU_SUPPORT
       iounmap(_pucVMMUTable[u4InstID]);
       #endif
       #ifdef REG_LOG_NEW
       iounmap(_pucRegisterLog[u4InstID]);
       #endif       

       
  #else
  
  if(_ucMVCType[u4InstID] != 1)
     vMemoryFreeLoop(_pucVFifo[u4InstID], V_FIFO_SZ);
  
  vMemoryFreeLoop(_pucFileListSa[u4InstID], FILE_LIST_SZ);
  vMemoryFreeLoop(_pucDumpYBuf[u4InstID], GOLD_Y_SZ);
  vMemoryFreeLoop(_pucDumpCBuf[u4InstID], GOLD_C_SZ);
  vMemoryFreeLoop(_pucCRCBuf[u4InstID], CRC_SZ);
  #ifdef CAPTURE_ESA_LOG
  vMemoryFreeLoop(_pucESALog[u4InstID], ESALOG_SZ);
  #endif
  #if VMMU_SUPPORT
  vMemoryFreeLoop(_pucVMMUTable[u4InstID], VMMU_SZ);
  #endif

#endif
  #ifdef LETTERBOX_SUPPORT
  x_free_aligned_verify_mem(_pucSettingFileSa[u4InstID], 1);
  x_free_aligned_verify_mem(_pucGoldenFileSa[u4InstID], 1);
  #endif

  #if 0//VDEC_AVS_EMU
  {

    #if MEM_ALLOCATE_IOREMAP
    iounmap(_pucPic0Y[u4InstID]);
    iounmap(_pucPic0C[u4InstID]);
    iounmap(_pucPic1Y[u4InstID]);
    iounmap(_pucPic1C[u4InstID]);
    iounmap(_pucPic2Y[u4InstID]);
    iounmap(_pucPic2C[u4InstID]);
    iounmap(_pucPic3Y[u4InstID]);
    iounmap(_pucPic3C[u4InstID]);
    iounmap(_pucAvsPred[u4InstID]);
    iounmap(_pucAvsMv1[u4InstID]);
    iounmap(_pucAvsMv2[u4InstID]);
    #else
    x_free_aligned_verify_mem(_pucPic0Y[u4InstID], WORKING_AREA_CHANEL_A);
    x_free_aligned_verify_mem(_pucPic0C[u4InstID], WORKING_AREA_CHANEL_A);
    x_free_aligned_verify_mem(_pucPic1Y[u4InstID], WORKING_AREA_CHANEL_A);
    x_free_aligned_verify_mem(_pucPic1C[u4InstID], WORKING_AREA_CHANEL_A);
    x_free_aligned_verify_mem(_pucPic2Y[u4InstID], WORKING_AREA_CHANEL_A);
    x_free_aligned_verify_mem(_pucPic2C[u4InstID], WORKING_AREA_CHANEL_A);
    x_free_aligned_verify_mem(_pucPic3C[u4InstID], WORKING_AREA_CHANEL_A);
    x_free_aligned_verify_mem(_pucPic3C[u4InstID], WORKING_AREA_CHANEL_A);
    x_free_aligned_verify_mem(_pucAvsPred[u4InstID], WORKING_AREA_CHANEL_A);
    x_free_aligned_verify_mem(_pucAvsMv1[u4InstID], WORKING_AREA_CHANEL_A);
    x_free_aligned_verify_mem(_pucAvsMv2[u4InstID], WORKING_AREA_CHANEL_A);
    #endif

    if (_u2AddressSwapMode[u4InstID] != ADDRSWAP_OFF)
    {
        x_free_aligned_verify_mem(_pucAddressSwapBuf[u4InstID], WORKING_AREA_CHANEL_A);
    }
  }
  #endif

  #if VDEC_DRAM_BUSY_TEST
     x_free_aligned_verify_mem(_pucDramBusy[u4InstID], WORKING_AREA_CHANEL_A);
  #endif
#endif
}


volatile BOOL g_fgAllocate[2] = {FALSE, FALSE};

BOOL fgVDecAllocWorkBuffer(UINT32 u4InstID)
{
  #ifdef CAPTURE_ESA_LOG
  _u4ESATotalLen[u4InstID] = 0;
  #endif

  if (g_fgAllocate[u4InstID] == FALSE)
  {
	g_fgAllocate[u4InstID] = TRUE;
  }
  else
  {
  #if VDEC_MVC_SUPPORT
    if(_fgMVCType && ((_fgMVCResReady[0] == FALSE)|| (_fgMVCResReady[1] == FALSE)))
    {
      //
    }
    else
  #endif
    {
  	return TRUE;
    }
  }


  _fgMVCResReady[u4InstID] = TRUE;

#ifdef DYNAMIC_MEMORY_ALLOCATE

#if VDEC_H264_REDUCE_MV_BUFF
   INT32 i;
#endif

#if VDEC_AVS_EMU
  if(_u4CodecVer[u4InstID] != VDEC_AVS && _pucPic0Y[u4InstID] != 0)
  {
       x_free_aligned_verify_mem(_pucPic0Y[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucPic0C[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucPic1Y[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucPic1C[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucPic2Y[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucPic2C[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucPic3C[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucPic3C[u4InstID], WORKING_AREA_CHANEL_A);

       x_free_aligned_verify_mem(_pucAvsPred[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucAvsMv1[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucAvsMv2[u4InstID], WORKING_AREA_CHANEL_A);

       _pucPic0Y[u4InstID] = 0;
       _pucPic0C[u4InstID] = 0;
       _pucPic1Y[u4InstID] = 0;
       _pucPic1C[u4InstID] = 0;
       _pucPic2Y[u4InstID] = 0;
       _pucPic2C[u4InstID] = 0;
       _pucPic3C[u4InstID] = 0;
       _pucPic3C[u4InstID] = 0;

       _pucAvsPred[u4InstID] = 0;
       _pucAvsMv1[u4InstID] = 0;
       _pucAvsMv2[u4InstID] = 0;

       if (_u2AddressSwapMode[u4InstID] != ADDRSWAP_OFF)
       {
          _pucAddressSwapBuf[u4InstID] = 0;
       }
  }
#endif

  if(_u4CodecVer[u4InstID] == VDEC_H264)
  {
      #if (AVC_8320_SUPPORT)
      _pucVLDWrapperWrok[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(VLD_WRAP_SZ,2048, WORKING_AREA_CHANEL_A); 
      _pucPPWrapperWork[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PP_WRAP_SZ,2048, WORKING_AREA_CHANEL_A); 

      printk("_pucVLDWrapperWrok[u4InstID] = 0x%x\n", _pucVLDWrapperWrok[u4InstID]);
      DBG_H264_PRINTF("_pucVLDWrapperWrok[u4InstID] = 0x%x\n", _pucVLDWrapperWrok[u4InstID]);
      printk("_pucPPWrapperWork[u4InstID] = 0x%x\n", _pucPPWrapperWork[u4InstID]);
      DBG_H264_PRINTF("_pucPPWrapperWork[u4InstID] = 0x%x\n", _pucPPWrapperWork[u4InstID]);
      #endif
      
     //_pucDPB[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(DPB_SZ,1024, 1);
     _pucDPB[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(DPB_SZ,4096, WORKING_AREA_CHANEL_A);
     printk("_pucDPB[u4InstID] = 0x%x\n", _pucDPB[u4InstID]);
     DBG_H264_PRINTF("_pucDPB[u4InstID] = 0x%x\n", _pucDPB[u4InstID]);

     _pucPredSa[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(AVC_PRED_SZ,1024, WORKING_AREA_CHANEL_A);
     printk("_pucPredSa[u4InstID] = 0x%x\n", _pucPredSa[u4InstID]);
     DBG_H264_PRINTF("_pucPredSa[u4InstID] = 0x%x\n", _pucPredSa[u4InstID]);


     #ifdef VERIFICATION_FGT
       _pucFGSeedbase[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(FGT_SEED_SZ,1024,WORKING_AREA_CHANEL_A);
       _pucFGDatabase[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(FGT_DATABASE_SZ,1024, WORKING_AREA_CHANEL_A);
       _pucFGSEISa[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(FGT_SEI_SZ,1024, WORKING_AREA_CHANEL_A);
      _pucFGTBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(FGT_SZ,1024, WORKING_AREA_CHANEL_A);
     #endif

     if (_u2AddressSwapMode[u4InstID] != ADDRSWAP_OFF)
     {
          _pucAddressSwapBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(ADDSWAP_BUF_SZ, 2048,  WORKING_AREA_CHANEL_A);
          printk("_pucAddressSwapBuf[u4InstID] = 0x%x\n", _pucAddressSwapBuf[u4InstID]);
          DBG_H264_PRINTF("_pucAddressSwapBuf[u4InstID] = 0x%x\n", _pucAddressSwapBuf[u4InstID]);
     }

#if VDEC_H264_REDUCE_MV_BUFF
     if (u4InstID == 0)
     {
     #if 0
       _pucAVCMVBuff_Main[0] = (UCHAR *)x_alloc_aligned_verify_mem(1920*1088,4096, WORKING_AREA_CHANEL_A);
       for (i=1; i<16; i++)
       {
          _pucAVCMVBuff_Main[i] = _pucAVCMVBuff_Main[i-1] + ((1920*1088) / 16);
	   printk("_pucAVCMVBuff_Main[%d] = 0x0%x !\n",i,_pucAVCMVBuff_Main[i]);
       }
     #endif
	 //15/9/2010 mtk40343
      _pucAVCMVBuff_Main[0] = (UCHAR *)x_alloc_aligned_verify_mem(MVBUF_SZ,4096, WORKING_AREA_CHANEL_A);
      printk("_pucAVCMVBuff_Main[%d] = 0x%x !\n",i,_pucAVCMVBuff_Main[0]);
      DBG_H264_PRINTF("_pucAVCMVBuff_Main[%d] = 0x%x !\n",i,_pucAVCMVBuff_Main[0]);
       for (i=1; i<17; i++)
       {
          //_pucAVCMVBuff_Main[i] = _pucAVCMVBuff_Main[i-1] + ((4096*2048) / 8);
          _pucAVCMVBuff_Main[i] = _pucAVCMVBuff_Main[i-1] + ((1920*1088) / 8);
	   printk("_pucAVCMVBuff_Main[%d] = 0x%x !\n",i,_pucAVCMVBuff_Main[i]);
       DBG_H264_PRINTF("_pucAVCMVBuff_Main[%d] = 0x%x !\n",i,_pucAVCMVBuff_Main[i]);
       }
     }
     else
     {
     #if 0
         _pucAVCMVBuff_Sub[0] = (UCHAR *)x_alloc_aligned_verify_mem(1920*1088,4096, WORKING_AREA_CHANEL_A);
         for (i=1; i<16; i++)
         {
            _pucAVCMVBuff_Sub[i] = _pucAVCMVBuff_Sub[i-1] + ((1920*1088) / 16);
         }
     #endif
         _pucAVCMVBuff_Sub[0] = (UCHAR *)x_alloc_aligned_verify_mem(1920*1088*5,4096, WORKING_AREA_CHANEL_A);
         for (i=1; i<17; i++)
         {
            _pucAVCMVBuff_Sub[i] = _pucAVCMVBuff_Sub[i-1] + ((1920*1088) / 4);
	   printk("_pucAVCMVBuff_Sub[%d] = 0x0%x !\n",i,_pucAVCMVBuff_Sub[i]);
       DBG_H264_PRINTF("_pucAVCMVBuff_Sub[%d] = 0x0%x !\n",i,_pucAVCMVBuff_Sub[i]);
         }
     }
#endif

#ifdef DOWN_SCALE_SUPPORT
       _pucVDSCLBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(VDSCL_BUF_SZ, 2048, VDSCL_CHANEL_A);
      _pucVDSCLWorkBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(VDSCL_WORK_SZ, 1024, VDSCL_CHANEL_A);
      _pucVDSCLWork1Sa[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(VDSCL_SW_WORK1_SZ, 1024, VDSCL_CHANEL_A);
      _pucVDSCLWork2Sa[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(VDSCL_SW_WORK2_SZ, 1024, VDSCL_CHANEL_A);
      _pucVDSCLWork3Sa[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(VDSCL_SW_WORK3_SZ, 1024, VDSCL_CHANEL_A);
      _pucVDSCLWork4Sa[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(VDSCL_SW_WORK4_SZ, 1024, VDSCL_CHANEL_A);
      printk("_pucVDSCLBuf[u4InstID] = 0x%x\n", _pucVDSCLBuf[u4InstID]);
      printk("_pucVDSCLWork1Sa[u4InstID] = 0x%x\n", _pucVDSCLWork1Sa[u4InstID]);
      printk("_pucVDSCLWork2Sa[u4InstID] = 0x%x\n", _pucVDSCLWork2Sa[u4InstID]);
      printk("_pucVDSCLWork3Sa[u4InstID] = 0x%x\n", _pucVDSCLWork3Sa[u4InstID]);
      printk("_pucVDSCLWork4Sa[u4InstID] = 0x%x\n",_pucVDSCLWork4Sa [u4InstID]);
#endif

       if (( _pucDPB[u4InstID] == 0)
            || ( _pucPredSa[u4InstID] == 0)
            || ( _u2AddressSwapMode[u4InstID] != ADDRSWAP_OFF && _pucAddressSwapBuf[u4InstID] == 0)
     #ifdef VERIFICATION_FGT
            || ( _pucFGSeedbase[u4InstID] == 0)
            || ( _pucFGDatabase[u4InstID] == 0)
            || ( _pucFGSEISa[u4InstID] == 0)
            || ( _pucFGTBuf[u4InstID] == 0)
     #endif
#ifdef DOWN_SCALE_SUPPORT
            || ( _pucVDSCLBuf[u4InstID] == 0)
            || ( _pucVDSCLWorkBuf[u4InstID] == 0)
            || ( _pucVDSCLWork1Sa[u4InstID] == 0)
            || ( _pucVDSCLWork2Sa[u4InstID] == 0)
            || ( _pucVDSCLWork3Sa[u4InstID] == 0)
            || ( _pucVDSCLWork4Sa[u4InstID] == 0)
#endif
      )
     {
         return FALSE;
     }

  }
//~AVC Working Buffer
  else
  if (     _u4CodecVer[u4InstID] == VDEC_WMV
  	|| _u4CodecVer[u4InstID] == VDEC_MPEG1
  	|| _u4CodecVer[u4InstID] == VDEC_MPEG2
  	|| _u4CodecVer[u4InstID] == VDEC_MPEG4
  	|| _u4CodecVer[u4InstID] == VDEC_H263
  	|| _u4CodecVer[u4InstID] == VDEC_DIVX3)
  {
#if 1//def DOWN_SCALE_SUPPORT if turn on DDR3, VDSCLBuf used for address swap buffer
      _pucVDSCLBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(VDSCL_BUF_SZ,2048, VDSCL_CHANEL_A);
      printk("_pucVDSCLBuf[u4InstID] = 0x%x\n", _pucVDSCLBuf[u4InstID]);
#endif

      #if (VDEC_8320_SUPPORT)
      _pucVLDWrapperWrok[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(VLD_WRAP_SZ,2048, WORKING_AREA_CHANEL_A); 
      _pucPPWrapperWork[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PP_WRAP_SZ,2048, WORKING_AREA_CHANEL_A); 

      printk("// <buffer> _pucVLDWrapperWrok[u4InstID] = 32'h%x\n", PHYSICAL(_pucVLDWrapperWrok[u4InstID]));
      printk("// <buffer> _pucPPWrapperWork[u4InstID] = 32'h%x\n", PHYSICAL(_pucPPWrapperWork[u4InstID]));
      printk("_pucVLDWrapperWrok[u4InstID] = 0x%x\n", _pucVLDWrapperWrok[u4InstID]);
      printk("_pucPPWrapperWork[u4InstID] = 0x%x\n", _pucPPWrapperWork[u4InstID]);
      #endif

#ifdef DOWN_SCALE_SUPPORT
      _pucVDSCLWork1Sa[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(VDSCL_SW_WORK1_SZ,1024, VDSCL_CHANEL_A);
      _pucVDSCLWork2Sa[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(VDSCL_SW_WORK2_SZ,1024, VDSCL_CHANEL_A);
      _pucVDSCLWork3Sa[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(VDSCL_SW_WORK3_SZ,1024, VDSCL_CHANEL_A);
      _pucVDSCLWork4Sa[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(VDSCL_SW_WORK4_SZ,1024, VDSCL_CHANEL_A);
      printk("_pucVDSCLWork1Sa[u4InstID] = 0x%x\n", _pucVDSCLWork1Sa[u4InstID]);
      printk("_pucVDSCLWork2Sa[u4InstID] = 0x%x\n", _pucVDSCLWork2Sa[u4InstID]);
      printk("_pucVDSCLWork3Sa[u4InstID] = 0x%x\n", _pucVDSCLWork3Sa[u4InstID]);
      printk("_pucVDSCLWork4Sa[u4InstID] = 0x%x\n",_pucVDSCLWork4Sa [u4InstID]);
#endif


  // WMV Part
      _pucPic0Y[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_Y_SZ,2048, WORKING_AREA_CHANEL_A); //UCHAR address
      _pucPic0C[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_C_SZ,2048, WORKING_AREA_CHANEL_A);
      _pucPic1Y[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_Y_SZ,2048, WORKING_AREA_CHANEL_A);
      _pucPic1C[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_C_SZ,2048, WORKING_AREA_CHANEL_A);
      _pucPic2Y[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_Y_SZ,2048, WORKING_AREA_CHANEL_A);
      _pucPic2C[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_C_SZ,2048, WORKING_AREA_CHANEL_A);
      printk("_pucPic0Y[u4InstID] = 0x%x\n", _pucPic0Y[u4InstID]);
      printk("_pucPic0C[u4InstID] = 0x%x\n", _pucPic0C[u4InstID]);
      printk("_pucPic1Y[u4InstID] = 0x%x\n", _pucPic1Y[u4InstID]);
      printk("_pucPic1C[u4InstID] = 0x%x\n", _pucPic1C[u4InstID]);
      printk("_pucPic2Y[u4InstID] = 0x%x\n",_pucPic2Y [u4InstID]);
      printk("_pucPic2C[u4InstID] = 0x%x\n", _pucPic2C[u4InstID]);



     if (_u4WmvMode[u4InstID] == 0)
     {
      _pucDcac[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(DCAC_SZ,1024, WORKING_AREA_CHANEL_A);
      _pucMv_1[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(Mv_1_SZ,1024, WORKING_AREA_CHANEL_A);
      _pucMv_2[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(Mv_2_SZ,1024, WORKING_AREA_CHANEL_A);
      printk("_pucDcac[u4InstID] = 0x%x\n", _pucDcac[u4InstID]);
      printk("_pucMv_1[u4InstID] = 0x%x\n", _pucMv_1[u4InstID]);
      printk("_pucMv_2[u4InstID] = 0x%x\n", _pucMv_2[u4InstID]);
     }

      _pucBp_1[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(Bp_1_SZ,1024, WORKING_AREA_CHANEL_A);
      _pucBp_2[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(Bp_2_SZ,1024, WORKING_AREA_CHANEL_A);
      _pucBp_3[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(Bp_3_SZ,1024, WORKING_AREA_CHANEL_A);
      _pucBp_4[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(Bp_4_SZ,1024, WORKING_AREA_CHANEL_A);
      printk("_pucBp_1[u4InstID] = 0x%x\n", _pucBp_1[u4InstID]);
      printk("_pucBp_2[u4InstID] = 0x%x\n", _pucBp_2[u4InstID]);
      printk("_pucBp_3[u4InstID] = 0x%x\n", _pucBp_3[u4InstID]);
      printk("_pucBp_4[u4InstID] = 0x%x\n", _pucBp_4[u4InstID]);


   if (_u4WmvMode[u4InstID] == 0)
   {
      _pucMv_3[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(Mv_3_SZ,1024, WORKING_AREA_CHANEL_A);
      _pucMv_1_2[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(Mv_1_2_SZ,1024, WORKING_AREA_CHANEL_A);
      _pucDcac_2[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(DCAC_2_SZ,1024, WORKING_AREA_CHANEL_A);
      
      printk("_pucMv_3[u4InstID] = 0x%x\n", _pucMv_3[u4InstID]);
      printk("_pucMv_1_2[u4InstID] = 0x%x\n", _pucMv_1_2[u4InstID]);
      printk("_pucDcac_2[u4InstID] = 0x%x\n", _pucDcac_2[u4InstID]);
   }
   else
   {
      _pucDcacNew[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(DCAC_NEW_SZ,1024, WORKING_AREA_CHANEL_A);
      _pucMvNew[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(MV_NEW_SZ,1024, WORKING_AREA_CHANEL_A);
      _pucBp0New[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(BP_0_NEW_SZ,1024, WORKING_AREA_CHANEL_A);
      _pucBp1New[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(BP_1_NEW_SZ,1024, WORKING_AREA_CHANEL_A);
      _pucBp2New[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(BP_2_NEW_SZ,1024, WORKING_AREA_CHANEL_A);
      printk("_pucBp0New[u4InstID] = 0x%x\n", _pucBp0New[u4InstID]); //sun for temp
      printk("_pucBp1New[u4InstID] = 0x%x\n", _pucBp1New[u4InstID]);
      printk("_pucBp2New[u4InstID] = 0x%x\n", _pucBp2New[u4InstID]);
    }

      _pucPp_1[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(DEC_PP_SZ,1024, WORKING_AREA_CHANEL_A);
      _pucPp_2[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(DEC_PP_SZ,1024, WORKING_AREA_CHANEL_A);
      printk("// <buffer> _pucPp_1[u4InstID] = 32'h%x\n", PHYSICAL(_pucPp_1[u4InstID]));
      printk("// <buffer> _pucPp_2[u4InstID] = 32'h%x\n", PHYSICAL(_pucPp_2[u4InstID]));
  // MPEG part
  #ifdef VDEC_PP_ENABLE
      _pucPpYSa[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(DEC_PP_Y_SZ,1024, WORKING_AREA_CHANEL_A);
      _pucPpCSa[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(DEC_PP_C_SZ,1024, WORKING_AREA_CHANEL_A);
      printk("// <buffer> _pucPpYSa[u4InstID] = 32'h%x\n", PHYSICAL(_pucPpYSa[u4InstID]));
      printk("// <buffer> _pucPpCSa[u4InstID] = 32'h%x\n", PHYSICAL(_pucPpCSa[u4InstID]));
  #endif
      _pucMp4Dcac[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(DCAC_SZ,1024, WORKING_AREA_CHANEL_A);
      printk("// <buffer> dcac + mv pred address: 32'h%x\n", PHYSICAL(_pucMp4Dcac[u4InstID]));
      _pucMp4Mvec[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(VER_MVEC_SZ,1024, WORKING_AREA_CHANEL_A);
      _pucMp4Bmb1[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(VER_BMB1_SZ,1024, WORKING_AREA_CHANEL_A);
      _pucMp4Bmb2[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(VER_BMB2_SZ,1024, WORKING_AREA_CHANEL_A);
      _pucMp4Bcode[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(BCODE_SZ,1024, WORKING_AREA_CHANEL_A);
      printk("// <buffer> _pucMp4Mvec[u4InstID] = 32'h%x\n", PHYSICAL(_pucMp4Mvec[u4InstID]));
      printk("// <buffer> _pucMp4Bcode[u4InstID] = 32'h%x\n", PHYSICAL(_pucMp4Bcode[u4InstID]));
      //6589NEW 2.4, 2.5, 4.1
  #if (MPEG4_6589_SUPPORT)
      _pucMp4DataPartition[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(DATA_PARTITION_SZ,1024, WORKING_AREA_CHANEL_A);
      _pucMp4NotCoded[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(NOT_CODED_SZ,1024, WORKING_AREA_CHANEL_A);
      _pucMp4MvDirect[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(MV_DIRECT_SZ,1024, WORKING_AREA_CHANEL_A);
      printk("// <buffer> data partition physical address: 32'h%x\n", PHYSICAL(_pucMp4DataPartition[u4InstID]));
      printk("// <buffer> not coded physical address: 32'h%x\n", PHYSICAL(_pucMp4NotCoded[u4InstID]));
      printk("// <buffer> mv direct physical address: 32'h%x\n", PHYSICAL(_pucMp4MvDirect[u4InstID]));
  #endif
      _pucDumpSRAMBuf[u4InstID] = (UCHAR*)x_alloc_aligned_verify_mem(54*1024,1024,1);
      printk("// <buffer> _pucDumpSRAMBuf[u4InstID]  = 32'h%x\n", PHYSICAL(_pucDumpSRAMBuf[u4InstID]));

      printk("_pucPp_1[u4InstID] = 0x%x\n", _pucPp_1[u4InstID]);
      printk("_pucPp_2[u4InstID] = 0x%x\n", _pucPp_2[u4InstID]);
      printk("_pucPpYSa[u4InstID] = 0x%x\n", _pucPpYSa[u4InstID]);
      printk("_pucPpCSa[u4InstID] = 0x%x\n", _pucPpCSa[u4InstID]);
      printk("_pucMp4Dcac[u4InstID] = 0x%x\n", _pucMp4Dcac[u4InstID]);
      printk("_pucMp4Mvec[u4InstID] = 0x%x\n", _pucMp4Mvec[u4InstID]);
      printk("_pucMp4Bmb1[u4InstID] = 0x%x\n", _pucMp4Bmb1[u4InstID]);
      printk("_pucMp4Bmb2[u4InstID] = 0x%x\n", _pucMp4Bmb2[u4InstID]);
      printk("_pucMp4Bcode[u4InstID] = 0x%x\n", _pucMp4Bcode[u4InstID]);
      printk("_pucDumpSRAMBuf[u4InstID]  = 0x%x\n", _pucDumpSRAMBuf[u4InstID]);
      printk("_pucMp4DataPartition[u4InstID]  = 0x%x\n", _pucMp4DataPartition[u4InstID]);
      printk("_pucMp4NotCoded[u4InstID]  = 0x%x\n", _pucMp4NotCoded[u4InstID]);
      printk("_pucMp4MvDirect[u4InstID]  = 0x%x\n", _pucMp4MvDirect[u4InstID]);
  }
  else
  if (_u4CodecVer[u4InstID] == VDEC_VP6)
  {
       _pucSizeFileBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(1024*400,2048, WORKING_AREA_CHANEL_A);
      _pucPic0Y[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_Y_SZ,2048, WORKING_AREA_CHANEL_A); //UCHAR address
      _pucPic0C[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_C_SZ,2048, WORKING_AREA_CHANEL_A);
      _pucPic1Y[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_Y_SZ,2048, WORKING_AREA_CHANEL_A);
      _pucPic1C[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_C_SZ,2048, WORKING_AREA_CHANEL_A);
      _pucPic2Y[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_Y_SZ,2048, WORKING_AREA_CHANEL_A);
      _pucPic2C[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_C_SZ,2048, WORKING_AREA_CHANEL_A);

      _pucPpYSa[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(DEC_PP_Y_SZ,2048, WORKING_AREA_CHANEL_A);
      _pucPpCSa[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(DEC_PP_C_SZ,2048, WORKING_AREA_CHANEL_A);

      _pucVP6VLDWrapperWorkspace[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(15*4096, 1024, WORKING_AREA_CHANEL_A);
      _pucVP6PPWrapperWorkspace[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(30*4096, 1024, WORKING_AREA_CHANEL_A);

      
      printk("_pucSizeFileBuf[%d] = 0x%x\n", u4InstID, _pucSizeFileBuf[u4InstID]);
      printk("_pucPic0Y[%d] = 0x%x\n", u4InstID, _pucPic0Y[u4InstID]);
      printk("_pucPic0C[%d] = 0x%x\n", u4InstID, _pucPic0C[u4InstID]);
      printk("_pucPic1Y[%d] = 0x%x\n", u4InstID, _pucPic1Y[u4InstID]);
      printk("_pucPic1C[%d] = 0x%x\n", u4InstID, _pucPic1C[u4InstID]);
      printk("_pucPic2Y[%d] = 0x%x\n", u4InstID, _pucPic2Y [u4InstID]);
      printk("_pucPic2C[%d] = 0x%x\n", u4InstID, _pucPic2C[u4InstID]);
      printk("_pucPpYSa[%d] = 0x%x\n", u4InstID, _pucPpYSa[u4InstID]);
      printk("_pucPpCSa[%d] = 0x%x\n", u4InstID, _pucPpCSa[u4InstID]);

      if (_u1AlphaBitstream[u4InstID])
      {
          u4InstID = 1;
          _pucPic0Y[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_Y_SZ,2048, WORKING_AREA_CHANEL_A); //UCHAR address
          _pucPic0C[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_C_SZ,2048, WORKING_AREA_CHANEL_A);
          _pucPic1Y[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_Y_SZ,2048, WORKING_AREA_CHANEL_A);
          _pucPic1C[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_C_SZ,2048, WORKING_AREA_CHANEL_A);
          _pucPic2Y[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_Y_SZ,2048, WORKING_AREA_CHANEL_A);
          _pucPic2C[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_C_SZ,2048, WORKING_AREA_CHANEL_A);

          _pucPpYSa[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(DEC_PP_Y_SZ,2048, WORKING_AREA_CHANEL_A);
          _pucPpCSa[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(DEC_PP_C_SZ,2048, WORKING_AREA_CHANEL_A);

          printk("_pucPic0Y[%d] = 0x%x\n", u4InstID, _pucPic0Y[u4InstID]);
          printk("_pucPic0C[%d] = 0x%x\n", u4InstID, _pucPic0C[u4InstID]);
          printk("_pucPic1Y[%d] = 0x%x\n", u4InstID, _pucPic1Y[u4InstID]);
          printk("_pucPic1C[%d] = 0x%x\n", u4InstID, _pucPic1C[u4InstID]);
          printk("_pucPic2Y[%d] = 0x%x\n", u4InstID, _pucPic2Y [u4InstID]);
          printk("_pucPic2C[%d] = 0x%x\n", u4InstID, _pucPic2C[u4InstID]);
          printk("_pucPpYSa[%d] = 0x%x\n", u4InstID, _pucPpYSa[u4InstID]);
          printk("_pucPpCSa[%d] = 0x%x\n", u4InstID, _pucPpCSa[u4InstID]);

          u4InstID = 0;
      }
      printk("_pucVP6VLDWrapperWorkspace[u4InstID] = 0x%x\n", _pucVP6VLDWrapperWorkspace[u4InstID]);
      printk("_pucVP6PPWrapperWorkspace[u4InstID] = 0x%x\n", _pucVP6PPWrapperWorkspace[u4InstID]);

      if (_u2AddressSwapMode[u4InstID] != ADDRSWAP_OFF)
      {
          _pucAddressSwapBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(ADDSWAP_BUF_SZ, 2048, WORKING_AREA_CHANEL_A);
          printk("_pucAddressSwapBuf[u4InstID] = 0x%x\n", _pucAddressSwapBuf[u4InstID]);
      }
  }
  else
  if (_u4CodecVer[u4InstID] == VDEC_VP8)
{
  #if (MEM_ALLOCATE_IOREMAP)
#if 0   //Jackal Chen 20120202
       _pucSizeFileBuf[u4InstID] = ioremap_nocache(FILEBUF_SA, FILEBUF_SZ);
#else
       _pucSizeFileBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(FILEBUF_SZ, 2048, WORKING_AREA_CHANEL_A);
#endif
       memset(_pucSizeFileBuf[u4InstID] ,0,FILEBUF_SZ);

#if 0   //Jackal Chen 20120202
       _pucWorkYBuf[u4InstID] = ioremap_nocache(WORKBUF_SA, PIC_Y_SZ+PIC_C_SZ);
#else
       _pucWorkYBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_Y_SZ+PIC_C_SZ, 2048, WORKING_AREA_CHANEL_A);
#endif
       memset(_pucWorkYBuf[u4InstID] ,0,PIC_Y_SZ+PIC_C_SZ);

#if VDEC_VP8_WRAPPER_OFF
#if 0   //Jackal Chen 20120202
       _pucDumpArfYBuf[u4InstID] = ioremap_nocache(ARFBUF_SA, DEC_PP_Y_SZ+DEC_PP_C_SZ);
#else
       _pucDumpArfYBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(DEC_PP_Y_SZ+DEC_PP_C_SZ, 2048, WORKING_AREA_CHANEL_A);
#endif
       memset(_pucDumpArfYBuf[u4InstID] ,0,DEC_PP_Y_SZ+DEC_PP_C_SZ);
#if 0   //Jackal Chen 20120202
       _pucDumpGldYBuf[u4InstID] = ioremap_nocache(GLDBUF_SA, DEC_PP_Y_SZ+DEC_PP_C_SZ);
#else
       _pucDumpGldYBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(DEC_PP_Y_SZ+DEC_PP_C_SZ, 2048, WORKING_AREA_CHANEL_A);
#endif
       memset(_pucDumpGldYBuf[u4InstID] ,0,DEC_PP_Y_SZ+DEC_PP_C_SZ);
#if 0   //Jackal Chen 20120202
       _pucDumpLstYBuf[u4InstID] = ioremap_nocache(LSTBUF_SA, DEC_PP_Y_SZ+DEC_PP_C_SZ);
#else
       _pucDumpLstYBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(DEC_PP_Y_SZ+DEC_PP_C_SZ, 2048, WORKING_AREA_CHANEL_A);
#endif
       memset(_pucDumpLstYBuf[u4InstID] ,0,DEC_PP_Y_SZ+DEC_PP_C_SZ);

#if 0   //Jackal Chen 20120202       
       _pucVLDWrapper[u4InstID] = ioremap_nocache(VLDWRAP_SA, VLD_PRED_SZ);
#else
       _pucVLDWrapper[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(VLD_PRED_SZ, 2048, WORKING_AREA_CHANEL_A);
#endif
       memset(_pucVLDWrapper[u4InstID] ,0,VLD_PRED_SZ);
#if 0   //Jackal Chen 20120202
       _pucPPWrapperY[u4InstID] = ioremap_nocache(PPWRAPY_SA, PP_WRAPY_SZ);
#else
       _pucPPWrapperY[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PP_WRAPY_SZ, 2048, WORKING_AREA_CHANEL_A);
#endif
       memset(_pucPPWrapperY[u4InstID] ,0,PP_WRAPY_SZ);
#if 0   //Jackal Chen 20120202
       _pucPPWrapperC[u4InstID] = ioremap_nocache(PPWRAPC_SA, PP_WRAPC_SZ);
#else
       _pucPPWrapperC[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PP_WRAPC_SZ, 2048, WORKING_AREA_CHANEL_A);
#endif
       memset(_pucPPWrapperC[u4InstID] ,0,PP_WRAPC_SZ);
#if 0   //Jackal Chen 20120202
       _pucSegIdWrapper[u4InstID] = ioremap_nocache(SEGIDWRAP_SA, SEG_ID_SZ);
#else
       _pucSegIdWrapper[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(SEG_ID_SZ, 2048, WORKING_AREA_CHANEL_A);
#endif
       memset(_pucSegIdWrapper[u4InstID] ,0,SEG_ID_SZ);
#else
#if 0   //Jackal Chen 20120202
       _pucDumpArfYBuf[u4InstID] = ioremap_nocache(ARFBUF_SA, PIC_Y_SZ+PIC_C_SZ);
#else
       _pucDumpArfYBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_Y_SZ+PIC_C_SZ, 2048, WORKING_AREA_CHANEL_A);
#endif
       memset(_pucDumpArfYBuf[u4InstID] ,0,PIC_Y_SZ+PIC_C_SZ);
#if 0   //Jackal Chen 20120202
       _pucDumpGldYBuf[u4InstID] = ioremap_nocache(GLDBUF_SA, PIC_Y_SZ+PIC_C_SZ);
#else
       _pucDumpGldYBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_Y_SZ+PIC_C_SZ, 2048, WORKING_AREA_CHANEL_A);
#endif
       memset(_pucDumpGldYBuf[u4InstID] ,0,PIC_Y_SZ+PIC_C_SZ);
#if 0   //Jackal Chen 20120202
       _pucDumpLstYBuf[u4InstID] = ioremap_nocache(LSTBUF_SA, PIC_Y_SZ+PIC_C_SZ);
#else
       _pucDumpLstYBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_Y_SZ+PIC_C_SZ, 2048, WORKING_AREA_CHANEL_A);
#endif
       memset(_pucDumpLstYBuf[u4InstID] ,0,PIC_Y_SZ+PIC_C_SZ);
#endif
#if 0   //Jackal Chen 20120202
       _pucPpYSa[u4InstID] = ioremap_nocache(PPYBUF_SA, DEC_PP_Y_SZ);
#else
       _pucPpYSa[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(DEC_PP_Y_SZ, 2048, WORKING_AREA_CHANEL_A);
#endif
       memset(_pucPpYSa[u4InstID] ,0,DEC_PP_Y_SZ);
#if 0   //Jackal Chen 20120202
       _pucPpCSa[u4InstID] = ioremap_nocache(PPCBUF_SA, DEC_PP_C_SZ);
#else
       _pucPpCSa[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(DEC_PP_C_SZ, 2048, WORKING_AREA_CHANEL_A);
#endif
       memset(_pucPpCSa[u4InstID] ,0,DEC_PP_C_SZ);
#if 0   //Jackal Chen 20120202
       _pucVLDWrapperWrok[u4InstID] = ioremap_nocache(VLDWRAPWORK_SA, VLD_WRAP_SZ);
#else
       _pucVLDWrapperWrok[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(VLD_WRAP_SZ, 2048, WORKING_AREA_CHANEL_A);
#endif
       memset(_pucVLDWrapperWrok[u4InstID] ,0,VLD_WRAP_SZ);
#if 0   //Jackal Chen 20120202
       _pucPPWrapperWork[u4InstID] = ioremap_nocache(PPWRAPWORK_SA, PP_WRAP_SZ);
#else
       _pucPPWrapperWork[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PP_WRAP_SZ, 2048, WORKING_AREA_CHANEL_A);
#endif
       memset(_pucPPWrapperWork[u4InstID] ,0,PP_WRAP_SZ);
#else
       _pucSizeFileBuf[u4InstID] = (UCHAR *)vMemoryAllocateLoop(FILEBUF_SZ);
       _pucWorkYBuf[u4InstID] = (UCHAR *)vMemoryAllocateLoop(8388608); //UCHAR address
 //      _pucWorkCBuf[u4InstID] = (UCHAR *)vMemoryAllocateLoop(GOLD_C_SZ); //UCHAR address

#if VDEC_VP8_WRAPPER_OFF
      _pucDumpArfYBuf[u4InstID] = (UCHAR *)vMemoryAllocateLoop(8388608); //UCHAR address
      _pucDumpGldYBuf[u4InstID] = (UCHAR *)vMemoryAllocateLoop(8388608); //UCHAR address
      _pucDumpLstYBuf[u4InstID] = (UCHAR *)vMemoryAllocateLoop(8388608); //UCHAR address
     _pucVLDWrapper[u4InstID] = (UCHAR *)vMemoryAllocateLoop(VLD_PRED_SZ);
     _pucPPWrapperY[u4InstID] = (UCHAR *)vMemoryAllocateLoop(PP_WRAPY_SZ);
     _pucPPWrapperC[u4InstID] = (UCHAR *)vMemoryAllocateLoop(PP_WRAPC_SZ);
     _pucSegIdWrapper[u4InstID] = (UCHAR *)vMemoryAllocateLoop(SEG_ID_SZ);
      _pucVLDWrapperWrok[u4InstID] = (UCHAR *)vMemoryAllocateLoop(VLD_WRAP_SZ);
      _pucPPWrapperWork[u4InstID] = (UCHAR *)vMemoryAllocateLoop(PP_WRAP_SZ);
#else
      _pucDumpArfYBuf[u4InstID] = (UCHAR *)vMemoryAllocateLoop(8388608); //UCHAR address
      _pucDumpGldYBuf[u4InstID] = (UCHAR *)vMemoryAllocateLoop(8388608); //UCHAR address
      _pucDumpLstYBuf[u4InstID] = (UCHAR *)vMemoryAllocateLoop(8388608); //UCHAR address
      _pucVLDWrapperWrok[u4InstID] = (UCHAR *)vMemoryAllocateLoop(VLD_WRAP_SZ);
      _pucPPWrapperWork[u4InstID] = (UCHAR *)vMemoryAllocateLoop(PP_WRAP_SZ);
      #endif
      _pucPpYSa[u4InstID] = (UCHAR *)vMemoryAllocateLoop(GOLD_Y_SZ);
      _pucPpCSa[u4InstID] = (UCHAR *)vMemoryAllocateLoop(GOLD_C_SZ);
#endif
/*
       _pucSizeFileBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(1024*400,2048, WORKING_AREA_CHANEL_A);

      _pucWorkYBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_Y_SZ+PIC_C_SZ,2048, WORKING_AREA_CHANEL_A); //UCHAR address
//      _pucWorkCBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_C_SZ,2048, WORKING_AREA_CHANEL_A);

#if VDEC_VP8_WEBP_SUPPORT
      _pucDumpArfYBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(DEC_PP_Y_SZ+DEC_PP_C_SZ,2048, WORKING_AREA_CHANEL_A); //UCHAR address
      _pucDumpGldYBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(DEC_PP_Y_SZ+DEC_PP_C_SZ,2048, WORKING_AREA_CHANEL_A); //UCHAR address
      _pucDumpLstYBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(DEC_PP_Y_SZ+DEC_PP_C_SZ,2048, WORKING_AREA_CHANEL_A); //UCHAR address
     _pucVLDWrapper[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(VLD_PRED_SZ,1024, WORKING_AREA_CHANEL_A);
     _pucPPWrapperY[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PP_WRAPY_SZ,1024, WORKING_AREA_CHANEL_A);
     _pucPPWrapperC[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PP_WRAPC_SZ,1024, WORKING_AREA_CHANEL_A);
     _pucSegIdWrapper[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(SEG_ID_SZ,1024, WORKING_AREA_CHANEL_A);
#else
      _pucDumpArfYBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_Y_SZ+PIC_C_SZ,2048, WORKING_AREA_CHANEL_A); //UCHAR address
      _pucDumpGldYBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_Y_SZ+PIC_C_SZ,2048, WORKING_AREA_CHANEL_A); //UCHAR address
      _pucDumpLstYBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_Y_SZ+PIC_C_SZ,2048, WORKING_AREA_CHANEL_A); //UCHAR address
#endif
      _pucPpYSa[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(DEC_PP_Y_SZ,2048, WORKING_AREA_CHANEL_A);
      _pucPpCSa[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(DEC_PP_C_SZ,2048, WORKING_AREA_CHANEL_A);
*/

      printk("_pucSizeFileBuf[u4InstID] = 0x%x\n", _pucSizeFileBuf[u4InstID]);
      printk("_pucWorkYBuf[u4InstID] = 0x%x\n", _pucWorkYBuf[u4InstID]);
    //  printk("_pucWorkCBuf[u4InstID] = 0x%x\n", _pucWorkCBuf[u4InstID]);
      printk("_pucDumpArfYBuf[u4InstID] = 0x%x\n", _pucDumpArfYBuf[u4InstID]);
      printk("_pucDumpGldYBuf[u4InstID] = 0x%x\n", _pucDumpGldYBuf[u4InstID]);
      printk("_pucDumpLstYBuf[u4InstID] = 0x%x\n", _pucDumpLstYBuf[u4InstID]);
    //  printk("_pucDumpArfCBuf[u4InstID] = 0x%x\n", _pucDumpArfCBuf[u4InstID]);
   //   printk("_pucDumpGldCBuf[u4InstID] = 0x%x\n", _pucDumpGldCBuf[u4InstID]);
  //    printk("_pucDumpLstCBuf[u4InstID] = 0x%x\n", _pucDumpLstCBuf[u4InstID]);
      printk("_pucPpYSa[u4InstID] = 0x%x\n", _pucPpYSa[u4InstID]);
      printk("_pucPpCSa[u4InstID] = 0x%x\n", _pucPpCSa[u4InstID]);
      printk("_pucVLDWrapperWrok[u4InstID] = 0x%x\n", _pucVLDWrapperWrok[u4InstID]);
      printk("_pucPPWrapperWork[u4InstID] = 0x%x\n", _pucPPWrapperWork[u4InstID]);
#if VDEC_VP8_WRAPPER_OFF
      printk("_pucVLDWrapper[u4InstID] = 0x%x\n", _pucVLDWrapper[u4InstID]);
      printk("_pucPPWrapperY[u4InstID] = 0x%x\n", _pucPPWrapperY[u4InstID]);
      printk("_pucPPWrapperC[u4InstID] = 0x%x\n", _pucPPWrapperC[u4InstID]);
      printk("_pucSegIdWrapper[u4InstID] = 0x%x\n", _pucSegIdWrapper[u4InstID]);
#endif

      if (_u2AddressSwapMode[u4InstID] != ADDRSWAP_OFF)
      {
          _pucAddressSwapBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(ADDSWAP_BUF_SZ, 2048, WORKING_AREA_CHANEL_A);
          printk("_pucAddressSwapBuf[u4InstID] = 0x%x\n", _pucAddressSwapBuf[u4InstID]);
      }
  }
#if 0
  else
  if (_u4CodecVer[u4InstID] == VDEC_AVS)
  {
      _pucPic0Y[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_Y_SZ,2048, WORKING_AREA_CHANEL_A); //UCHAR address
      _pucPic0C[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_C_SZ,2048, WORKING_AREA_CHANEL_A);
      _pucPic1Y[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_Y_SZ,2048, WORKING_AREA_CHANEL_A);
      _pucPic1C[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_C_SZ,2048, WORKING_AREA_CHANEL_A);
      _pucPic2Y[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_Y_SZ,2048, WORKING_AREA_CHANEL_A);
      _pucPic2C[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_C_SZ,2048, WORKING_AREA_CHANEL_A);
      _pucPic3Y[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_Y_SZ,2048, WORKING_AREA_CHANEL_A);
      _pucPic3C[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(PIC_C_SZ,2048, WORKING_AREA_CHANEL_A);
      printk("_pucPic0Y[u4InstID] = 0x%x\n", _pucPic0Y[u4InstID]);
      printk("_pucPic0C[u4InstID] = 0x%x\n", _pucPic0C[u4InstID]);
      printk("_pucPic1Y[u4InstID] = 0x%x\n", _pucPic1Y[u4InstID]);
      printk("_pucPic1C[u4InstID] = 0x%x\n", _pucPic1C[u4InstID]);
      printk("_pucPic2Y[u4InstID] = 0x%x\n", _pucPic2Y [u4InstID]);
      printk("_pucPic2C[u4InstID] = 0x%x\n", _pucPic2C[u4InstID]);
      printk("_pucPic3Y[u4InstID] = 0x%x\n", _pucPic3Y [u4InstID]);
      printk("_pucPic3C[u4InstID] = 0x%x\n", _pucPic3C[u4InstID]);

      _pucAvsPred[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(AVS_VLD_PRED_SZ,1024, WORKING_AREA_CHANEL_A);
      _pucAvsMv1[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(AVS_VLD_MV_SZ,1024, WORKING_AREA_CHANEL_A);
      _pucAvsMv2[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(AVS_VLD_MV_SZ,1024, WORKING_AREA_CHANEL_A);
      printk("_pucDcac[u4InstID] = 0x%x\n", _pucAvsPred[u4InstID]);
      printk("_pucMv_1[u4InstID] = 0x%x\n", _pucAvsMv1[u4InstID]);
      printk("_pucMv_2[u4InstID] = 0x%x\n", _pucAvsMv2[u4InstID]);

      if (_u2AddressSwapMode[u4InstID] != ADDRSWAP_OFF)
      {
          _pucAddressSwapBuf[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(ADDSWAP_BUF_SZ, 2048, WORKING_AREA_CHANEL_A);
          printk("_pucAddressSwapBuf[u4InstID] = 0x%x\n", _pucAddressSwapBuf[u4InstID]);
      }
  }
#endif

#endif
  return TRUE;
}


void vVDecFreeWorkBuffer(UINT32 u4InstID)
{
#ifdef DYNAMIC_MEMORY_ALLOCATE
  printk("vVDecFreeWorkBuffer begin\n");

  if(_u4CodecVer[u4InstID] == VDEC_H264)
  {
    #if (AVC_8320_SUPPORT)
    x_free_aligned_verify_mem(_pucVLDWrapperWrok[u4InstID], WORKING_AREA_CHANEL_A);
    x_free_aligned_verify_mem(_pucPPWrapperWork[u4InstID], WORKING_AREA_CHANEL_A);
    #endif
    
    x_free_aligned_verify_mem(_pucDPB[u4InstID], WORKING_AREA_CHANEL_A);
    x_free_aligned_verify_mem(_pucPredSa[u4InstID], WORKING_AREA_CHANEL_A);

#ifdef VERIFICATION_FGT
  x_free_aligned_verify_mem(_pucFGSeedbase[u4InstID], WORKING_AREA_CHANEL_A);
  x_free_aligned_verify_mem(_pucFGDatabase[u4InstID], WORKING_AREA_CHANEL_A);
  x_free_aligned_verify_mem(_pucFGSEISa[u4InstID], WORKING_AREA_CHANEL_A);
  x_free_aligned_verify_mem(_pucFGTBuf[u4InstID], WORKING_AREA_CHANEL_A);
#endif

     if (_u2AddressSwapMode[u4InstID] != ADDRSWAP_OFF)
     {
          x_free_aligned_verify_mem(_pucAddressSwapBuf[u4InstID], WORKING_AREA_CHANEL_A);
     }

#if VDEC_H264_REDUCE_MV_BUFF
    if (u4InstID == 0)
       x_free_aligned_verify_mem(_pucAVCMVBuff_Main[0], WORKING_AREA_CHANEL_A);
    else
    	x_free_aligned_verify_mem(_pucAVCMVBuff_Sub[0], WORKING_AREA_CHANEL_A);
#endif

  #ifdef DOWN_SCALE_SUPPORT
     x_free_aligned_verify_mem(_pucVDSCLBuf[u4InstID], WORKING_AREA_CHANEL_A);
     x_free_aligned_verify_mem(_pucVDSCLWorkBuf[u4InstID], WORKING_AREA_CHANEL_A);
     x_free_aligned_verify_mem(_pucVDSCLWork1Sa[u4InstID], WORKING_AREA_CHANEL_A);
     x_free_aligned_verify_mem(_pucVDSCLWork2Sa[u4InstID], WORKING_AREA_CHANEL_A);
     x_free_aligned_verify_mem(_pucVDSCLWork3Sa[u4InstID], WORKING_AREA_CHANEL_A);
     x_free_aligned_verify_mem(_pucVDSCLWork4Sa[u4InstID], WORKING_AREA_CHANEL_A);
   #endif
  }
  else
  if (_u4CodecVer[u4InstID] == VDEC_WMV
  	|| _u4CodecVer[u4InstID] == VDEC_MPEG1
  	|| _u4CodecVer[u4InstID] == VDEC_MPEG2
  	|| _u4CodecVer[u4InstID] == VDEC_MPEG4
  	|| _u4CodecVer[u4InstID] == VDEC_H263
  	|| _u4CodecVer[u4InstID] == VDEC_DIVX3)
  {  
    #if 1//def DOWN_SCALE_SUPPORT
    x_free_aligned_verify_mem(_pucVDSCLBuf[u4InstID], WORKING_AREA_CHANEL_A);
    #endif

    #ifdef DOWN_SCALE_SUPPORT
    x_free_aligned_verify_mem(_pucVDSCLWorkBuf[u4InstID], WORKING_AREA_CHANEL_A);
    x_free_aligned_verify_mem(_pucVDSCLWork1Sa[u4InstID], WORKING_AREA_CHANEL_A);
    x_free_aligned_verify_mem(_pucVDSCLWork2Sa[u4InstID], WORKING_AREA_CHANEL_A);
    x_free_aligned_verify_mem(_pucVDSCLWork3Sa[u4InstID], WORKING_AREA_CHANEL_A);
    x_free_aligned_verify_mem(_pucVDSCLWork4Sa[u4InstID], WORKING_AREA_CHANEL_A);
    #endif
    
    #if (VDEC_8320_SUPPORT)
    x_free_aligned_verify_mem(_pucVLDWrapperWrok[u4InstID], WORKING_AREA_CHANEL_A);
    x_free_aligned_verify_mem(_pucPPWrapperWork[u4InstID], WORKING_AREA_CHANEL_A);
    #endif

    // WMV Part
    x_free_aligned_verify_mem(_pucPic0Y[u4InstID], WORKING_AREA_CHANEL_A);
    x_free_aligned_verify_mem(_pucPic0C[u4InstID], WORKING_AREA_CHANEL_A);
    x_free_aligned_verify_mem(_pucPic1Y[u4InstID], WORKING_AREA_CHANEL_A);
    x_free_aligned_verify_mem(_pucPic1C[u4InstID], WORKING_AREA_CHANEL_A);
    x_free_aligned_verify_mem(_pucPic2Y[u4InstID], WORKING_AREA_CHANEL_A);
    x_free_aligned_verify_mem(_pucPic2C[u4InstID], WORKING_AREA_CHANEL_A);
    _pucPic0Y[u4InstID] = 0;
    _pucPic0C[u4InstID] = 0;
    _pucPic1Y[u4InstID] = 0;
    _pucPic1C[u4InstID] = 0;
    _pucPic2Y[u4InstID] = 0;
    _pucPic2C[u4InstID] = 0;


    if (_u4WmvMode[u4InstID] == 0)
    {
      x_free_aligned_verify_mem(_pucDcac[u4InstID], WORKING_AREA_CHANEL_A);
      x_free_aligned_verify_mem(_pucMv_1[u4InstID], WORKING_AREA_CHANEL_A);
      x_free_aligned_verify_mem(_pucMv_2[u4InstID], WORKING_AREA_CHANEL_A);
      _pucDcac[u4InstID] = 0;
      _pucMv_1[u4InstID] = 0;
      _pucMv_2[u4InstID] = 0;
    }
    
    x_free_aligned_verify_mem(_pucBp_1[u4InstID], WORKING_AREA_CHANEL_A);
    x_free_aligned_verify_mem(_pucBp_2[u4InstID], WORKING_AREA_CHANEL_A);
    x_free_aligned_verify_mem(_pucBp_3[u4InstID], WORKING_AREA_CHANEL_A);
    x_free_aligned_verify_mem(_pucBp_4[u4InstID], WORKING_AREA_CHANEL_A);
    _pucBp_1[u4InstID] = 0;
    _pucBp_2[u4InstID] = 0;
    _pucBp_3[u4InstID] = 0;
    _pucBp_4[u4InstID] = 0;

    if (_u4WmvMode[u4InstID] == 0)
    {
      x_free_aligned_verify_mem(_pucMv_3[u4InstID], WORKING_AREA_CHANEL_A);
      x_free_aligned_verify_mem(_pucMv_1_2[u4InstID], WORKING_AREA_CHANEL_A);
      x_free_aligned_verify_mem(_pucDcac_2[u4InstID], WORKING_AREA_CHANEL_A);
      _pucMv_3[u4InstID] = 0;
      _pucMv_1_2[u4InstID] = 0;
      _pucDcac_2[u4InstID] = 0;
    }
    else
    {
      x_free_aligned_verify_mem(_pucDcacNew[u4InstID], WORKING_AREA_CHANEL_A);
      x_free_aligned_verify_mem(_pucMvNew[u4InstID], WORKING_AREA_CHANEL_A);
      x_free_aligned_verify_mem(_pucBp0New[u4InstID], WORKING_AREA_CHANEL_A);
      x_free_aligned_verify_mem(_pucBp1New[u4InstID], WORKING_AREA_CHANEL_A);
      x_free_aligned_verify_mem(_pucBp2New[u4InstID], WORKING_AREA_CHANEL_A);
      _pucDcacNew[u4InstID] = 0;
      _pucMvNew[u4InstID] = 0;
      _pucBp0New[u4InstID] = 0;
      _pucBp1New[u4InstID] = 0;
      _pucBp2New[u4InstID] = 0;
    }

    x_free_aligned_verify_mem(_pucPp_1[u4InstID], WORKING_AREA_CHANEL_A);
    x_free_aligned_verify_mem(_pucPp_2[u4InstID], WORKING_AREA_CHANEL_A);
    _pucPp_1[u4InstID] = 0;
    _pucPp_2[u4InstID] = 0;

    // MPEG part
    x_free_aligned_verify_mem(_pucMp4Dcac[u4InstID], WORKING_AREA_CHANEL_A);
    x_free_aligned_verify_mem(_pucMp4Mvec[u4InstID], WORKING_AREA_CHANEL_A);
    x_free_aligned_verify_mem(_pucMp4Bmb1[u4InstID], WORKING_AREA_CHANEL_A);
    x_free_aligned_verify_mem(_pucMp4Bmb2[u4InstID], WORKING_AREA_CHANEL_A);
    x_free_aligned_verify_mem(_pucMp4Bcode[u4InstID], WORKING_AREA_CHANEL_A);
#if (MPEG4_6589_SUPPORT)
    x_free_aligned_verify_mem(_pucMp4DataPartition[u4InstID], WORKING_AREA_CHANEL_A);
    x_free_aligned_verify_mem(_pucMp4NotCoded[u4InstID], WORKING_AREA_CHANEL_A);
    x_free_aligned_verify_mem(_pucMp4MvDirect[u4InstID], WORKING_AREA_CHANEL_A);
#endif
    x_free_aligned_verify_mem(_pucDumpSRAMBuf[u4InstID],WORKING_AREA_CHANEL_A);
    _pucMp4Dcac[u4InstID] = 0;
    _pucMp4Mvec[u4InstID] = 0;
    _pucMp4Bmb1[u4InstID] = 0;
    _pucMp4Bmb2[u4InstID] = 0;
    _pucMp4Bcode[u4InstID] = 0;
#if (MPEG4_6589_SUPPORT)
    _pucMp4DataPartition[u4InstID] = 0;
    _pucMp4NotCoded[u4InstID] = 0;
    _pucMp4MvDirect[u4InstID] = 0;
#endif
    _pucDumpSRAMBuf[u4InstID] = 0;
  }
  else
  if (_u4CodecVer[u4InstID] == VDEC_VP6)
  {
       x_free_aligned_verify_mem(_pucSizeFileBuf[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucPic0Y[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucPic0C[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucPic1Y[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucPic1C[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucPic2Y[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucPic2C[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucPpYSa[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucPpCSa[u4InstID], WORKING_AREA_CHANEL_A);
       _pucPic0Y[u4InstID] = 0;
        _pucPic0C[u4InstID] = 0;
       _pucPic1Y[u4InstID] = 0;
        _pucPic1C[u4InstID] = 0;
        _pucPic2Y[u4InstID] = 0;
        _pucPic2C[u4InstID] = 0;
        _pucPpYSa[u4InstID] = 0;
        _pucPpCSa[u4InstID] = 0;

       if (_u2AddressSwapMode[u4InstID] != ADDRSWAP_OFF)
      {
          x_free_aligned_verify_mem(_pucAddressSwapBuf[u4InstID], WORKING_AREA_CHANEL_A);
          _pucAddressSwapBuf[u4InstID] = 0;
      }
  }
  else
  if (_u4CodecVer[u4InstID] == VDEC_VP8)
  {
  #if (MEM_ALLOCATE_IOREMAP)
#if 0   //Jackal Chen 20120202
       iounmap(_pucSizeFileBuf[u4InstID]);
       iounmap(_pucWorkYBuf[u4InstID]);
       iounmap(_pucWorkCBuf[u4InstID]);
       iounmap(_pucDumpArfYBuf[u4InstID]);
       iounmap(_pucDumpArfCBuf[u4InstID]);
       iounmap(_pucDumpGldYBuf[u4InstID]);
       iounmap(_pucDumpGldCBuf[u4InstID]);
       iounmap(_pucDumpLstYBuf[u4InstID]);
       iounmap(_pucDumpLstCBuf[u4InstID]);
       iounmap(_pucPpYSa[u4InstID]);
       iounmap(_pucPpCSa[u4InstID]);
#if VDEC_VP8_WRAPPER_OFF
       iounmap(_pucVLDWrapper[u4InstID]);
       iounmap(_pucPPWrapperY[u4InstID]);
       iounmap(_pucPPWrapperC[u4InstID]);
       iounmap(_pucSegIdWrapper[u4InstID]);
#endif
       iounmap(_pucVLDWrapperWrok[u4InstID]);
       iounmap(_pucPPWrapperWork[u4InstID]);
#else

       x_free_aligned_verify_mem(_pucSizeFileBuf[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucWorkYBuf[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucDumpArfYBuf[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucDumpArfCBuf[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucDumpGldYBuf[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucDumpGldCBuf[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucDumpLstYBuf[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucDumpLstCBuf[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucPpYSa[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucPpCSa[u4InstID], WORKING_AREA_CHANEL_A);
#if VDEC_VP8_WRAPPER_OFF
       x_free_aligned_verify_mem(_pucVLDWrapper[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucPPWrapperY[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucPPWrapperC[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucSegIdWrapper[u4InstID], WORKING_AREA_CHANEL_A);
#endif
       x_free_aligned_verify_mem(_pucVLDWrapperWrok[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucPPWrapperWork[u4InstID], WORKING_AREA_CHANEL_A);
#endif
#else
       vMemoryFreeLoop(_pucSizeFileBuf[u4InstID], 1024*400);
       vMemoryFreeLoop(_pucWorkYBuf[u4InstID], GOLD_Y_SZ);
       vMemoryFreeLoop(_pucWorkCBuf[u4InstID], GOLD_C_SZ);
       vMemoryFreeLoop(_pucDumpArfYBuf[u4InstID], GOLD_Y_SZ);
       vMemoryFreeLoop(_pucDumpArfCBuf[u4InstID], GOLD_C_SZ);
       vMemoryFreeLoop(_pucDumpGldYBuf[u4InstID], GOLD_Y_SZ);
       vMemoryFreeLoop(_pucDumpGldCBuf[u4InstID], GOLD_C_SZ);
       vMemoryFreeLoop(_pucDumpLstYBuf[u4InstID], GOLD_Y_SZ);
        vMemoryFreeLoop(_pucDumpLstCBuf[u4InstID], GOLD_C_SZ);
      vMemoryFreeLoop(_pucPpYSa[u4InstID], GOLD_Y_SZ);
       vMemoryFreeLoop(_pucPpCSa[u4InstID], GOLD_C_SZ);
#if VDEC_VP8_WRAPPER_OFF
       vMemoryFreeLoop(_pucVLDWrapper[u4InstID], VLD_PRED_SZ);
       vMemoryFreeLoop(_pucPPWrapperY[u4InstID], PP_WRAPY_SZ);
       vMemoryFreeLoop(_pucPPWrapperC[u4InstID], PP_WRAPC_SZ);
       vMemoryFreeLoop(_pucSegIdWrapper[u4InstID], SEG_ID_SZ);
#endif
       vMemoryFreeLoop(_pucVLDWrapperWrok[u4InstID], VLD_WRAP_SZ);
       vMemoryFreeLoop(_pucPPWrapperWork[u4InstID], PP_WRAP_SZ);
#endif
        _pucWorkYBuf[u4InstID] = 0;
//        _pucWorkCBuf[u4InstID] = 0;
        _pucDumpArfYBuf[u4InstID] = 0;
        _pucDumpGldYBuf[u4InstID] = 0;
        _pucDumpLstYBuf[u4InstID] = 0;
        _pucPpYSa[u4InstID] = 0;
        _pucPpCSa[u4InstID] = 0;
#if VDEC_VP8_WRAPPER_OFF
     _pucVLDWrapper[u4InstID] = 0;
     _pucSegIdWrapper[u4InstID] = 0;
     _pucPPWrapperY[u4InstID] = 0;
     _pucPPWrapperC[u4InstID] = 0;
#endif
       if (_u2AddressSwapMode[u4InstID] != ADDRSWAP_OFF)
      {
          x_free_aligned_verify_mem(_pucAddressSwapBuf[u4InstID], WORKING_AREA_CHANEL_A);
          _pucAddressSwapBuf[u4InstID] = 0;
      }
  }
#if 0
  else
  if (_u4CodecVer[u4InstID] == VDEC_AVS)
  {
       x_free_aligned_verify_mem(_pucPic0Y[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucPic0C[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucPic1Y[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucPic1C[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucPic2Y[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucPic2C[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucPic3C[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucPic3C[u4InstID], WORKING_AREA_CHANEL_A);

       x_free_aligned_verify_mem(_pucAvsPred[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucAvsMv1[u4InstID], WORKING_AREA_CHANEL_A);
       x_free_aligned_verify_mem(_pucAvsMv2[u4InstID], WORKING_AREA_CHANEL_A);

       if (_u2AddressSwapMode[u4InstID] != ADDRSWAP_OFF)
      {
          x_free_aligned_verify_mem(_pucAddressSwapBuf[u4InstID], WORKING_AREA_CHANEL_A);
      }
  }
  #endif
  #endif
  return;
}

void vVDec_FlushDCacheRange(UINT32 u4Start, UINT32 u4Len)
{
    //BSP_dma_map_vaddr(u4Start, u4Len, BIDIRECTIONAL);
    //BSP_dma_unmap_vaddr(u4Start, u4Len, BIDIRECTIONAL);
}

void vVDec_CleanDCacheRange(UINT32 u4Start, UINT32 u4Len)
{
    //BSP_dma_map_vaddr(u4Start, u4Len, TO_DEVICE);
    //BSP_dma_unmap_vaddr(u4Start, u4Len, TO_DEVICE);
}

void vVDec_InvDCacheRange(UINT32 u4Start, UINT32 u4Len)
{
    //BSP_dma_map_vaddr(u4Start, u4Len, FROM_DEVICE);
    //BSP_dma_unmap_vaddr(u4Start, u4Len, FROM_DEVICE);
}


