
#ifndef __GFMT_DRV_H__
#define __GFMT_DRV_H__




#define GFMT_IOCTL_MAGIC        'f'
#define GFMT_DEV_MAJOR_NUMBER   250





typedef struct
{    
    
    unsigned char   gfLinkGDMA;           // 0: 420 3p video format,  1: 420 3p mcu row
	  unsigned char   webpEn    ;
    unsigned char   fieldCompactOuput;      // 0: frame out, 1: field compact 
    unsigned char   isTopField;                 // 0: bottom field, 1: top field
    unsigned char   burst           ;                  // 1,2,4,8 bytes 

    unsigned int    gfDstBufHeight[2]    ;    //0: Y, 1:C
    //unsigned char   gfDstBufHeight[0];      // 0: 4 lines 1: 8 lines 2: 16 lines 3: 32 lines
    //unsigned char   gfDstBufHeight[1];      // 0: 2 lines 1: 4 lines 2: 8 lines 3: 16 lines


    unsigned char   webpMcuHeight_Y  ;            // 0: 4 lines 1: 8 lines 2: 16 lines 3: 32 lines
    unsigned char   webpMcuHeight_C  ;            // 0: 2 lines 1: 4 lines 2: 8 lines 3: 16 lines
    unsigned int    gfSrcBufStride_Y ;          // 16-bytes align
    unsigned int    gfSrcBufStride_C ;
    unsigned int    gfDstBufStride_Y ;          // 16-bytes align
    unsigned int    gfDstBufStride_C ;    

    unsigned int    gfSrcBufAddr_Y   ;    
    unsigned int    gfSrcBufAddr_C   ;
  
  
    unsigned int    gfDstBufBank0[3] ;    
    unsigned int    gfDstBufBank1[3] ;
    
    unsigned int    gfSrcWidth ;     //yWidth; 
    unsigned int    gfSrcHeight;    //yHeight;

    unsigned int    rangeMapEn   ;
    unsigned int    rangeReduceEn;
    unsigned int    rangeMapY    ;
    unsigned int    rangeMapC    ;
    
    
    unsigned int    colorBufferSize[3] ;     //kernel NOUSE
    unsigned char*  tgtBufferVA[3];          //kernel NOUSE

    //for webp row mode
    unsigned int    isRowModeFlag;           //kernel NOUSE
    unsigned int    webp_RingBuf_row_num ;   //kernel NOUSE
    unsigned int    webp_total_row_num ;     //kernel NOUSE
    unsigned int    webp_row_last_height ;   //kernel NOUSE
    unsigned int    webp_row_cnt ;           //kernel NOUSE
    
    unsigned int    webp_rowModeTgtBase[3];  //kernel NOUSE
    unsigned int    webp_rowModeSrcBase[2];  //kernel NOUSE
    unsigned int    webp_srcOneRowSize[2];   //kernel NOUSE
    unsigned int    webp_tgtOneRowSize[2];   //kernel NOUSE
    unsigned int    isRowModeFlag_OpMode ;   //kernel NOUSE
    //0: VDEC+FMT row mode to frame, 
    //1: VDEC RowByRow do whole frame first, then FMT do whole frame 
    //2: VDEC + FMT do RowByRow to ISP tile mode
    
    unsigned int    isRowModeFlag_tdri_state ; //kernel NOUSE
    //0: none
    //1: init
    //2: trigger first row
    //3: trigger next row
    //4: finish VDEC done
    unsigned int    isRowModeFlag_run ;      //kernel NOUSE
    unsigned int    isRowModeOutRowBuf;      //kernel NOUSE
    unsigned int  go_flag;       //only use in DEBUG MODE
    unsigned int  io_fix_value;       //
    
    
    

} GDMA_DRV_FMT_IN;


typedef struct 
{
    long timeout;
    unsigned int *result; 

} GFMT_DRV_OUT; 





//#define GFMT_IOCTL_CONFIG_CTL       _IOW (GFMT_IOCTL_MAGIC, 1, GDMA_DRV_CTL_IN)
#define GFMT_IOCTL_CONFIG_FMT       _IOW (GFMT_IOCTL_MAGIC, 2, GDMA_DRV_FMT_IN)
#define GFMT_IOCTL_START            _IO  (GFMT_IOCTL_MAGIC, 3)
#define GFMT_IOCTL_WAIT             _IO  (GFMT_IOCTL_MAGIC, 4)
#define GFMT_IOCTL_RESET            _IO  (GFMT_IOCTL_MAGIC, 5)
#define GFMT_IOCTL_INIT             _IO  (GFMT_IOCTL_MAGIC, 6)
#define GFMT_IOCTL_CONFIG_ISP       _IOW (GFMT_IOCTL_MAGIC, 7, CONFIG_DRV_CDP_IN)
#define GFMT_IOCTL_DUMP_REG         _IO  (GFMT_IOCTL_MAGIC, 8)
#define GFMT_IOCTL_RW_REG           _IO  (GFMT_IOCTL_MAGIC, 9)
#define GFMT_IOCTL_DEINIT           _IO  (GFMT_IOCTL_MAGIC, 10)
#define GFMT_IOCTL_VDEC             _IOW  (GFMT_IOCTL_MAGIC, 15, GDMA_DRV_FMT_IN)



#define CHK_GFMT_DONE_MASK 0x001


#endif


