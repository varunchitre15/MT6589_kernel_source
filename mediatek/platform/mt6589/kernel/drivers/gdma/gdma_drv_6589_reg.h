
#ifndef __GDMA_DRV_6589_REG_H__
#define __GDMA_DRV_6589_REG_H__


#include <mach/mt_reg_base.h>

#include <mach/sync_write.h>

//#define CAM_REG_BASE    0xF2080000
//#define ISP_REG_BASE    0xF2084000
//#define GDMA_REG_BASE   0xF2084E00

#define CAM_REG_BASE    0xF5000000
#define ISP_REG_BASE    0xF5004000
//#define ISPDMA_REG_BASE 0xF5004200
#define GDMA_REG_BASE   0xF5004E00

#define GDMA_EARLY_MM_BASE    0xF5000000

#define REG_GDMA_MM_REG_MASK                           *(volatile kal_uint32 *)(GDMA_EARLY_MM_BASE + 0x000)

#define IMG_REG_WRITE(v,a) mt65xx_reg_sync_writel(v,a)

//////////////////////////////////////////////////////////////////////////////////////////////////////////
/*     ISP register setting                                                                             */
//////////////////////////////////////////////////////////////////////////////////////////////////////////


#define REG_ISP_STA_CTL                         *(volatile unsigned int *)(ISP_REG_BASE + 0x000)
    #define REG_ISP_STR_CTL_GDMA_SHIFT      3
    #define REG_ISP_STR_CTL_ISP_SHIFT       0
    #define REG_ISP_STR_CTL_GDMA_MASK       0x8
    #define REG_ISP_STR_CTL_ISP_MASK        0x1


#define REG_ISP_CTL_EN2                         *(volatile unsigned int *)(ISP_REG_BASE + 0x008)
    #define REG_ISP_CTL_EN2_FMT_EN_SHIFT        26
    #define REG_ISP_CTL_EN2_FMT_EN_MASK         0x04000000
    #define REG_ISP_CTL_EN2_GDMA_EN_SHIFT       25
    #define REG_ISP_CTL_EN2_GDMA_EN_MASK        0x02000000

#define REG_ISP_CTL_DMA_EN                      *(volatile unsigned int *)(ISP_REG_BASE + 0x00C)
   #define REG_ISP_CTL_DMA_EN_IMGI_EN         0x00000080
   #define REG_ISP_CTL_DMA_EN_VIPI_EN         0x00004000
   #define REG_ISP_CTL_DMA_EN_VIP2I_EN        0x00002000
   #define REG_ISP_CTL_DMA_EN_DISPO_EN        0x00200000
   #define REG_ISP_CTL_DMA_EN_VIDO_EN         0x00080000

    
#define REG_ISP_CTL_SEL                             *(volatile unsigned int *)(ISP_REG_BASE + 0x018)
    #define REG_ISP_CTL_SEL_GDMA_LNK_SHIFT      1
    #define REG_ISP_CTL_SEL_GDMA_LNK_MASK       0x2

#define REG_ISP_CTL_SW_CTL                      *(volatile unsigned int *)(ISP_REG_BASE + 0x05C)
    #define REG_ISP_CTL_HW_RESET            0x4
    #define REG_ISP_CTL_SW_RESET            0x2
    
#define REG_ISP_CTL_INT_STS                     *(volatile unsigned int *)(ISP_REG_BASE + 0x024)    
    #define REG_ISP_PASS2_DON_ST            (1 << 14)



#define REG_ISP_DMA_IMGI_BASE                  *(volatile unsigned int *)(ISP_REG_BASE + 0x230) 
#define REG_ISP_DMA_IMGI_OFST                  *(volatile unsigned int *)(ISP_REG_BASE + 0x234) 
#define REG_ISP_DMA_IMGI_X_SIZE                *(volatile unsigned int *)(ISP_REG_BASE + 0x238) 
#define REG_ISP_DMA_IMGI_Y_SIZE                *(volatile unsigned int *)(ISP_REG_BASE + 0x23C) 
#define REG_ISP_DMA_IMGI_STRIDE                *(volatile unsigned int *)(ISP_REG_BASE + 0x240) 


#define REG_ISP_DMA_VIPI_BASE                  *(volatile unsigned int *)(ISP_REG_BASE + 0x2C0) 
#define REG_ISP_DMA_VIPI_OFST                  *(volatile unsigned int *)(ISP_REG_BASE + 0x2C4) 
#define REG_ISP_DMA_VIPI_X_SIZE                *(volatile unsigned int *)(ISP_REG_BASE + 0x2C8) 
#define REG_ISP_DMA_VIPI_Y_SIZE                *(volatile unsigned int *)(ISP_REG_BASE + 0x2CC) 
#define REG_ISP_DMA_VIPI_STRIDE                *(volatile unsigned int *)(ISP_REG_BASE + 0x2D0) 

#define REG_ISP_DMA_VIP2I_BASE                 *(volatile unsigned int *)(ISP_REG_BASE + 0x2E0) 
#define REG_ISP_DMA_VIP2I_OFST                 *(volatile unsigned int *)(ISP_REG_BASE + 0x2E4) 
#define REG_ISP_DMA_VIP2I_X_SIZE               *(volatile unsigned int *)(ISP_REG_BASE + 0x2E8) 
#define REG_ISP_DMA_VIP2I_Y_SIZE               *(volatile unsigned int *)(ISP_REG_BASE + 0x2EC) 
#define REG_ISP_DMA_VIP2I_STRIDE               *(volatile unsigned int *)(ISP_REG_BASE + 0x2F0) 












//////////////////////////////////////////////////////////////////////////////////////////////////////////
/*    GDMA CTL register setting                                                                             */
//////////////////////////////////////////////////////////////////////////////////////////////////////////



#define REG_GDMA_CTL_IOTYPE                     *(volatile unsigned int *)(GDMA_REG_BASE + 0x050)
#define REG_GDMA_CTL_MCUMODE                    *(volatile unsigned int *)(GDMA_REG_BASE + 0x058)
    #define REG_GDMA_MCUVFAC_Y_SHIFT        28
    #define REG_GDMA_MCUVFAC_C_SHIFT        24
    #define REG_GDMA_LSTMCUVFAC_Y_SHIFT     17
    #define REG_GDMA_LSTMCUVFAC_C_SHIFT     12
    #define REG_GDMA_FSTMCUVFAC_Y_SHIFT     5
    #define REG_GDMA_FSTMCUVFAC_C_SHIFT     0

#define REG_GDMA_CTL_SRCOFST_Y                  *(volatile unsigned int *)(GDMA_REG_BASE + 0x060)
    #define REG_GDMA_SRCOFST_Y_MASK         0xffff
    #define REG_GDMA_SRCOFST_Y_SHIFT        16
    
#define REG_GDMA_CTL_SRCOFST_C                  *(volatile unsigned int *)(GDMA_REG_BASE + 0x064)
    #define REG_GDMA_SRCOFST_C_MASK         0xffff
    #define REG_GDMA_SRCOFST_C_SHIFT        16

// Y Bank 
#define REG_GDMA_CTL_YSRCB1                     *(volatile unsigned int *)(GDMA_REG_BASE + 0x068)
#define REG_GDMA_CTL_YSRCB2                     *(volatile unsigned int *)(GDMA_REG_BASE + 0x06C)

// CB Bank
#define REG_GDMA_CTL_CBSRCB1                    *(volatile unsigned int *)(GDMA_REG_BASE + 0x070)
#define REG_GDMA_CTL_CBSRCB2                    *(volatile unsigned int *)(GDMA_REG_BASE + 0x074)

// CR Bank
#define REG_GDMA_CTL_CRSRCB1                    *(volatile unsigned int *)(GDMA_REG_BASE + 0x078)
#define REG_GDMA_CTL_CRSRCB2                    *(volatile unsigned int *)(GDMA_REG_BASE + 0x07C)

#define REG_GDMA_CTL_YSRCSZ                     *(volatile unsigned int *)(GDMA_REG_BASE + 0x080)
#define REG_GDMA_CTL_SPARE                      *(volatile unsigned int *)(GDMA_REG_BASE + 0x084)




//////////////////////////////////////////////////////////////////////////////////////////////////////////
/*    GDMA FMT register setting                                                                             */
//////////////////////////////////////////////////////////////////////////////////////////////////////////


#define REG_GDMA_FMT_IOTYPE                     *(volatile unsigned int *)(GDMA_REG_BASE + 0x090)
#define REG_GDMA_FMT_MCUMODE                    *(volatile unsigned int *)(GDMA_REG_BASE + 0x094)
    #define REG_GDMA_MCUMODE_MCUVFAC_MASK      0x3
   
#define REG_GDMA_FMT_WEBPMODE                   *(volatile unsigned int *)(GDMA_REG_BASE + 0x098)
    #define REG_GDMA_FMT_WEBPMODE_MCUY_SHIFT    0x8
    #define REG_GDMA_FMT_WEBPMODE_MCUC_SHIFT    0x4


#define REG_GDMA_FMT_ULTRA_MODE                 *(volatile unsigned int *)(GDMA_REG_BASE + 0x09C)
    

    
#define REG_GDMA_FMT_MIFMODE                    *(volatile unsigned int *)(GDMA_REG_BASE + 0x0A0)
    #define REG_GDMA_BST_LIM_SHIFT              8
    
#define REG_GDMA_FMT_SRCBUFL                    *(volatile unsigned int *)(GDMA_REG_BASE + 0x0A4)
    #define REG_GDMA_SRCBUFL_MASK               0x3ff
    #define REG_GDMA_SRCBUFL_Y_SHIFT            0
    #define REG_GDMA_SRCBUFL_C_SHIFT            12
    
#define REG_GDMA_FMT_WINTFON                    *(volatile unsigned int *)(GDMA_REG_BASE + 0x0A8)
#define REG_GDMA_FMT_TGBUFL                     *(volatile unsigned int *)(GDMA_REG_BASE + 0x0AC)
    #define REG_GDMA_TGTBUFL_MASK               0x3ff
    #define REG_GDMA_TGTBUFL_Y_SHIFT            0
    #define REG_GDMA_TGTBUFL_C_SHIFT            12

/* Src Buffer */
#define REG_GDMA_FMT_YSRCB1                     *(volatile unsigned int *)(GDMA_REG_BASE + 0x0B0)
#define REG_GDMA_FMT_CBSRCB1                    *(volatile unsigned int *)(GDMA_REG_BASE + 0x0B4)
#define REG_GDMA_FMT_CRSRCB1                    *(volatile unsigned int *)(GDMA_REG_BASE + 0x0B8)

/* Dst Buffer Bank1 */
#define REG_GDMA_FMT_YTGB1                      *(volatile unsigned int *)(GDMA_REG_BASE + 0x0BC)
#define REG_GDMA_FMT_CBTGB1                     *(volatile unsigned int *)(GDMA_REG_BASE + 0x0C0)
#define REG_GDMA_FMT_CRTGB1                     *(volatile unsigned int *)(GDMA_REG_BASE + 0x0C4)

/* Dst Buffer Bank2 */
#define REG_GDMA_FMT_YTGB2                      *(volatile unsigned int *)(GDMA_REG_BASE + 0x0C8)
#define REG_GDMA_FMT_CBTGB2                     *(volatile unsigned int *)(GDMA_REG_BASE + 0x0CC)
#define REG_GDMA_FMT_CRTGB2                     *(volatile unsigned int *)(GDMA_REG_BASE + 0x0D0)        

#define REG_GDMA_FMT_YSRCSZ                     *(volatile unsigned int *)(GDMA_REG_BASE + 0x0D4)
    #define YSRCSZ_LO_MASK          0x0000FFFF
    #define YSRCSZ_HI_SHIFT         16
    
#define REG_GDMA_FMT_RANGE                      *(volatile unsigned int *)(GDMA_REG_BASE + 0x0D8)

#define REG_GDMA_FMT_INTEN                      *(volatile unsigned int *)(GDMA_REG_BASE + 0x0DC)    
    #define INT_EN_SHIFT            4
    #define INT_EN_MASK             0x10
    #define CHK_DONE_MASK           0x1
    
#define REG_GDMA_RANGE                      0x0E0


//===========================================================================================================
// REGISTER ADDRESS



//////////////////////////////////////////////////////////////////////////////////////////////////////////
/*    GDMA CTL register address                                                                             */
//////////////////////////////////////////////////////////////////////////////////////////////////////////



#define REG_ADDR_GDMA_CTL_IOTYPE                     (GDMA_REG_BASE + 0x050)
#define REG_ADDR_GDMA_CTL_MCUMODE                    (GDMA_REG_BASE + 0x058)
#define REG_ADDR_GDMA_CTL_SRCOFST_Y                  (GDMA_REG_BASE + 0x060)
#define REG_ADDR_GDMA_CTL_SRCOFST_C                  (GDMA_REG_BASE + 0x064)
// Y Bank    
#define REG_ADDR_GDMA_CTL_YSRCB1                     (GDMA_REG_BASE + 0x068)
#define REG_ADDR_GDMA_CTL_YSRCB2                     (GDMA_REG_BASE + 0x06C)
             
// CB Bank   
#define REG_ADDR_GDMA_CTL_CBSRCB1                    (GDMA_REG_BASE + 0x070)
#define REG_ADDR_GDMA_CTL_CBSRCB2                    (GDMA_REG_BASE + 0x074)
             
// CR Bank   
#define REG_ADDR_GDMA_CTL_CRSRCB1                    (GDMA_REG_BASE + 0x078)
#define REG_ADDR_GDMA_CTL_CRSRCB2                    (GDMA_REG_BASE + 0x07C)
             
#define REG_ADDR_GDMA_CTL_YSRCSZ                     (GDMA_REG_BASE + 0x080)
#define REG_ADDR_GDMA_CTL_SPARE                      (GDMA_REG_BASE + 0x084)




//////////////////////////////////////////////////////////////////////////////////////////////////////////
/*    GDMA FMT register address                                                                             */
//////////////////////////////////////////////////////////////////////////////////////////////////////////


#define REG_ADDR_GDMA_FMT_IOTYPE                     (GDMA_REG_BASE + 0x090)
#define REG_ADDR_GDMA_FMT_MCUMODE                    (GDMA_REG_BASE + 0x094)
#define REG_ADDR_GDMA_FMT_WEBPMODE                   (GDMA_REG_BASE + 0x098)
             
#define REG_ADDR_GDMA_FMT_ULTRA_MODE                 (GDMA_REG_BASE + 0x09C)
             
             
             
#define REG_ADDR_GDMA_FMT_MIFMODE                    (GDMA_REG_BASE + 0x0A0)
             
#define REG_ADDR_GDMA_FMT_SRCBUFL                    (GDMA_REG_BASE + 0x0A4)
             
#define REG_ADDR_GDMA_FMT_WINTFON                    (GDMA_REG_BASE + 0x0A8)
#define REG_ADDR_GDMA_FMT_TGBUFL                     (GDMA_REG_BASE + 0x0AC)

/* Src Buffer */
#define REG_ADDR_GDMA_FMT_YSRCB1                     (GDMA_REG_BASE + 0x0B0)
#define REG_ADDR_GDMA_FMT_CBSRCB1                    (GDMA_REG_BASE + 0x0B4)
#define REG_ADDR_GDMA_FMT_CRSRCB1                    (GDMA_REG_BASE + 0x0B8)

/* Dst Buffer Bank1 */
#define REG_ADDR_GDMA_FMT_YTGB1                      (GDMA_REG_BASE + 0x0BC)
#define REG_ADDR_GDMA_FMT_CBTGB1                     (GDMA_REG_BASE + 0x0C0)
#define REG_ADDR_GDMA_FMT_CRTGB1                     (GDMA_REG_BASE + 0x0C4)

/* Dst Buffer Bank2 */
#define REG_ADDR_GDMA_FMT_YTGB2                      (GDMA_REG_BASE + 0x0C8)
#define REG_ADDR_GDMA_FMT_CBTGB2                     (GDMA_REG_BASE + 0x0CC)
#define REG_ADDR_GDMA_FMT_CRTGB2                     (GDMA_REG_BASE + 0x0D0)        
             
#define REG_ADDR_GDMA_FMT_YSRCSZ                     (GDMA_REG_BASE + 0x0D4)
             
#define REG_ADDR_GDMA_FMT_RANGE                      (GDMA_REG_BASE + 0x0D8)
             
#define REG_ADDR_GDMA_FMT_INTEN                      (GDMA_REG_BASE + 0x0DC)










#endif

