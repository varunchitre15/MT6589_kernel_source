

#ifndef __GDMA_DRV_H__
#define __GDMA_DRV_H__

#define GDMA_IOCTL_MAGIC        'g'
#define GDMA_DEV_MAJOR_NUMBER   250

#define ISP_REG_LIST_SIZE       20
#define ISP_REG_BASE_OFFSET     0x4000
#define MAX_ISP_REG             0x8518


#define WEBP_VDEC_STATE_INIT 1
#define WEBP_VDEC_STATE_START 2
#define WEBP_VDEC_STATE_DECODE_ROW 3
#define WEBP_VDEC_STATE_FINISH 4



/* MODIFY Offset */

#define ISP_OADDR_TILE_CFG_BASE_ADDR  (0x04204)  

#define ISP_OADDR_DMA_IMGI_BASE       (0x04230)  
#define ISP_OADDR_DMA_IMGI_OFST       (0x04234) 
#define ISP_OADDR_DMA_IMGI_X_SIZE     (0x04238) 
#define ISP_OADDR_DMA_IMGI_Y_SIZE     (0x0423C) 
#define ISP_OADDR_DMA_IMGI_STRIDE     (0x04240) 
#define ISP_OADDR_DMA_IMGI_RING_BUF   (0x04244) 
                                                
#define ISP_OADDR_DMA_VIPI_BASE       (0x042C0) 
#define ISP_OADDR_DMA_VIPI_OFST       (0x042C4) 
#define ISP_OADDR_DMA_VIPI_X_SIZE     (0x042C8) 
#define ISP_OADDR_DMA_VIPI_Y_SIZE     (0x042CC) 
#define ISP_OADDR_DMA_VIPI_STRIDE     (0x042D0) 
#define ISP_OADDR_DMA_VIPI_RING_BUF   (0x042D4) 
                                                
#define ISP_OADDR_DMA_VIP2I_BASE      (0x042E0) 
#define ISP_OADDR_DMA_VIP2I_OFST      (0x042E4) 
#define ISP_OADDR_DMA_VIP2I_X_SIZE    (0x042E8) 
#define ISP_OADDR_DMA_VIP2I_Y_SIZE    (0x042EC) 
#define ISP_OADDR_DMA_VIP2I_STRIDE    (0x042F0) 
#define ISP_OADDR_DMA_VIP2I_RING_BUF  (0x042F4) 


#define ISP_OADDR_DISPO_TARGET_SIZE     (0x04D64)

#define ISP_OADDR_DISPO_LUMA_BASE       (0x04D68) 
#define ISP_OADDR_DISPO_LUMA_OFFSET     (0x04D6C) 
#define ISP_OADDR_DISPO_LUMA_STRIDE     (0x04D70) 

#define ISP_OADDR_DISPO_CHROMA_BASE     (0x04D74) 
#define ISP_OADDR_DISPO_CHROMA_OFFSET   (0x04D78) 
#define ISP_OADDR_DISPO_CHROMA_STRIDE   (0x04D7C) 






typedef struct
{
    unsigned int    modifyOffset[ISP_REG_LIST_SIZE];
    unsigned int    modifyValue[ISP_REG_LIST_SIZE];    
    unsigned int    numModify;
    unsigned int    dispoAddr;
    unsigned int    dispoCAddr;
    unsigned char   startISP;
    unsigned char   linkGDMA;
    unsigned char   fmtEn;    
    unsigned char   gdmaEn;        
    unsigned char   isp_TDRIen;
    unsigned char   isp_TDRI_resume;
} CONFIG_DRV_CDP_IN;


typedef struct
{
    
    unsigned int    gtSrcIs422  ;
    unsigned int    cbcrConstant;
    unsigned int    isSrcGray   ;          // 0: yuv      1: y only    
    unsigned int    isSrcJpeg   ;          // 0: video   1: jpeg    
    
    unsigned int    gtSrcWidth ;           // width : 0~65535
    unsigned int    gtSrcHeight;           // height: 0~65535
    
       
    
    unsigned int  gtSrcBank0[3];
    unsigned int  gtSrcBank1[3];
    
    
    unsigned int    cropY_initOff ;
    unsigned int    cropY_Off     ;
    unsigned int    cropC_initOff ;
    unsigned int    cropC_Off     ; 
    
    unsigned int    gtSrcBufHeight[2]    ;  //Y: 0~5, C: 0~5
      // 0:   1 lines per block 
      // 1:   2 lines per block 
      // 2:   4 lines per block 
      // 3:   8 lines per block 
      // 4:  16 lines per block 
      // 5:  32 lines per block 
    
    unsigned int    gtSrcBufLastHeight[2];  //Y: 0~32, C: 0~16
    unsigned int    gtSrcBufFstHeight[2] ;  //Y: 0~32, C: 0~16
    
    
    

    //unsigned int    mcuYVRatio;
    //unsigned int    mcuCVRatio;     // 0:  1 line per block   1:  2 lines per block
    //unsigned int    lastMcuYCnt;
    //unsigned int    lastMcuCCnt;    
    //unsigned int    fstMcuYCnt;
    //unsigned int    fstMcuCCnt;
    
} GDMA_DRV_CTL_IN;


#define GDMA_IOCTL_CONFIG_CTL       _IOW (GDMA_IOCTL_MAGIC, 1, GDMA_DRV_CTL_IN)
#define GDMA_IOCTL_CONFIG_FMT       _IOW (GDMA_IOCTL_MAGIC, 2, GDMA_DRV_FMT_IN)
#define GDMA_IOCTL_START            _IO  (GDMA_IOCTL_MAGIC, 3)
#define GDMA_IOCTL_WAIT             _IO  (GDMA_IOCTL_MAGIC, 4)
#define GDMA_IOCTL_RESET            _IO  (GDMA_IOCTL_MAGIC, 5)
#define GDMA_IOCTL_INIT             _IO  (GDMA_IOCTL_MAGIC, 6)
#define GDMA_IOCTL_CONFIG_ISP       _IOW (GDMA_IOCTL_MAGIC, 7, CONFIG_DRV_CDP_IN)
#define GDMA_IOCTL_DUMP_REG         _IO  (GDMA_IOCTL_MAGIC, 8)
#define GDMA_IOCTL_RW_REG           _IO  (GDMA_IOCTL_MAGIC, 9)
#define GDMA_IOCTL_DEINIT           _IO  (GDMA_IOCTL_MAGIC, 10)

#define GDMA_IOCTL_VDEC             _IOW  (GDMA_IOCTL_MAGIC, 15, GDMA_DRV_FMT_IN)







#endif


