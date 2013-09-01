#ifndef _DDP_REG_H_
#define _DDP_REG_H_

#include <mach/mt_reg_base.h>
#include <mach/sync_write.h>

//TDODO: get base reg addr from system header
#define DISPSYS_CONFIG_BASE     DISPSYS_BASE		
#define DISPSYS_ROT_BASE        ROT_BASE				
#define DISPSYS_SCL_BASE        SCL_BASE				
#define DISPSYS_OVL_BASE        OVL_BASE				
#define DISPSYS_WDMA0_BASE      WDMA0_BASE			
#define DISPSYS_WDMA1_BASE      WDMA1_BASE			
#define DISPSYS_RDMA0_BASE      RDMA0_BASE			
#define DISPSYS_RDMA1_BASE      RDMA1_BASE			
#define DISPSYS_BLS_BASE        BLS_BASE				
#define DISPSYS_GAMMA_BASE      GAMMA_BASE			
#define DISPSYS_COLOR_BASE      COLOR_BASE			
#define DISPSYS_TDSHP_BASE      TDSHP_BASE			
#define DISPSYS_DBI_BASE        LCD_BASE				
#define DISPSYS_DSI_BASE        DSI_BASE				
#define DISPSYS_DPI0_BASE       DPI0_BASE				
#define DISPSYS_DPI1_BASE       DPI1_BASE		
#define DISPSYS_SMI_LARB2_BASE  SMILARB2_BASE	
#define DISPSYS_G2D_BASE  		G2D_BASE
#define DISPSYS_MUTEX_BASE      DISP_MUTEX_BASE	
#define DISPSYS_CMDQ_BASE       DISP_CMDQ_BASE

#define DISPSYS_REG_ADDR_MIN    DISPSYS_BASE
#define DISPSYS_REG_ADDR_MAX    (DISP_CMDQ_BASE+0x5000)
// ---------------------------------------------------------------------------
//  Register Field Access
// ---------------------------------------------------------------------------

#define REG_FLD(width, shift) \
    ((unsigned int)((((width) & 0xFF) << 16) | ((shift) & 0xFF)))

#define REG_FLD_WIDTH(field) \
    ((unsigned int)(((field) >> 16) & 0xFF))

#define REG_FLD_SHIFT(field) \
    ((unsigned int)((field) & 0xFF))

#define REG_FLD_MASK(field) \
    (((unsigned int)(1 << REG_FLD_WIDTH(field)) - 1) << REG_FLD_SHIFT(field))

#define REG_FLD_VAL(field, val) \
    (((val) << REG_FLD_SHIFT(field)) & REG_FLD_MASK(field))

#define REG_FLD_GET(field, reg32) \
    (((reg32) & REG_FLD_MASK(field)) >> REG_FLD_SHIFT(field))

#define REG_FLD_SET(field, reg32, val)                  \
    do {                                                \
        (reg32) = (((reg32) & ~REG_FLD_MASK(field)) |   \
                   REG_FLD_VAL((field), (val)));        \
    } while (0)
    

#define DISP_REG_GET(reg32) (*(volatile unsigned int*)(reg32))
#define DISP_REG_GET_FIELD(field, reg32) \
    do{                           \
          ((*(volatile unsigned int*)(reg32) & REG_FLD_MASK(field)) >> REG_FLD_SHIFT(field)) \
      } while(0)
      
#if 0
#define DISP_REG_SET(reg32, val) (*(volatile unsigned int*)(reg32) = val)
#define DISP_REG_SET_FIELD(field, reg32, val)  \
    do {                                \
           *(volatile unsigned int*)(reg32) = ((*(volatile unsigned int*)(reg32) & ~REG_FLD_MASK(field)) |  REG_FLD_VAL((field), (val)));  \
    } while (0)
#else  // use write_sync instead of write directly
#define DISP_REG_SET(reg32, val) mt65xx_reg_sync_writel(val, reg32)
/*
#define DISP_REG_SET_FIELD(field, reg32, val)  \
    do {                                \
           mt65xx_reg_sync_writel( (*(volatile unsigned int*)(reg32) & ~REG_FLD_MASK(field))|REG_FLD_VAL((field), (val)), reg32);  \
    } while (0)
*/
void DISP_REG_SET_FIELD(unsigned long field, unsigned long reg32, unsigned long val);
#endif

//-----------------------------------------------------------------
// CMDQ
#define CMDQ_THREAD_NUM 7
#define DISP_REG_CMDQ_IRQ_FLAG                         (DISPSYS_CMDQ_BASE + 0x10)
#define DISP_REG_CMDQ_LOADED_THR                       (DISPSYS_CMDQ_BASE + 0x20)
#define DISP_REG_CMDQ_THR_SLOT_CYCLES                  (DISPSYS_CMDQ_BASE + 0x30)
#define DISP_REG_CMDQ_BUS_CTRL                         (DISPSYS_CMDQ_BASE + 0x40)
#define DISP_REG_CMDQ_ABORT                            (DISPSYS_CMDQ_BASE + 0x50)
#define DISP_REG_CMDQ_THRx_RESET(idx)                  (DISPSYS_CMDQ_BASE + 0x100 + 0x80*idx)   
#define DISP_REG_CMDQ_THRx_EN(idx)                     (DISPSYS_CMDQ_BASE + 0x104 + 0x80*idx)   
#define DISP_REG_CMDQ_THRx_SUSPEND(idx)                (DISPSYS_CMDQ_BASE + 0x108 + 0x80*idx)   
#define DISP_REG_CMDQ_THRx_STATUS(idx)                 (DISPSYS_CMDQ_BASE + 0x10c + 0x80*idx)   
#define DISP_REG_CMDQ_THRx_IRQ_FLAG(idx)               (DISPSYS_CMDQ_BASE + 0x110 + 0x80*idx)   
#define DISP_REG_CMDQ_THRx_IRQ_FLAG_EN(idx)            (DISPSYS_CMDQ_BASE + 0x114 + 0x80*idx)   
#define DISP_REG_CMDQ_THRx_SECURITY(idx)               (DISPSYS_CMDQ_BASE + 0x118 + 0x80*idx)   
#define DISP_REG_CMDQ_THRx_PC(idx)                     (DISPSYS_CMDQ_BASE + 0x120 + 0x80*idx)   
#define DISP_REG_CMDQ_THRx_END_ADDR(idx)               (DISPSYS_CMDQ_BASE + 0x124 + 0x80*idx)   
#define DISP_REG_CMDQ_THRx_EXEC_CMDS_CNT(idx)          (DISPSYS_CMDQ_BASE + 0x128 + 0x80*idx)   
#define DISP_REG_CMDQ_THRx_WAIT_EVENTS0(idx)           (DISPSYS_CMDQ_BASE + 0x130 + 0x80*idx)   
#define DISP_REG_CMDQ_THRx_WAIT_EVENTS1(idx)           (DISPSYS_CMDQ_BASE + 0x134 + 0x80*idx)   
#define DISP_REG_CMDQ_THRx_OBSERVED_EVENTS0(idx)       (DISPSYS_CMDQ_BASE + 0x140 + 0x80*idx)   
#define DISP_REG_CMDQ_THRx_OBSERVED_EVENTS1(idx)       (DISPSYS_CMDQ_BASE + 0x144 + 0x80*idx)   
#define DISP_REG_CMDQ_THRx_OBSERVED_EVENTS0_CLR(idx)   (DISPSYS_CMDQ_BASE + 0x148 + 0x80*idx)   
#define DISP_REG_CMDQ_THRx_OBSERVED_EVENTS1_CLR(idx)   (DISPSYS_CMDQ_BASE + 0x14c + 0x80*idx)   
#define DISP_REG_CMDQ_THRx_INSTN_TIMEOUT_CYCLES(idx)   (DISPSYS_CMDQ_BASE + 0x150 + 0x80*idx) 

//-----------------------------------------------------------------
// Config
#define DISP_REG_CONFIG_SCL_MOUT_EN      (DISPSYS_CONFIG_BASE + 0x020)
#define DISP_REG_CONFIG_OVL_MOUT_EN      (DISPSYS_CONFIG_BASE + 0x024)
#define DISP_REG_CONFIG_COLOR_MOUT_EN    (DISPSYS_CONFIG_BASE + 0x028)
#define DISP_REG_CONFIG_TDSHP_MOUT_EN    (DISPSYS_CONFIG_BASE + 0x02C)
#define DISP_REG_CONFIG_MOUT_RST         (DISPSYS_CONFIG_BASE + 0x030)
#define DISP_REG_CONFIG_RDMA0_OUT_SEL    (DISPSYS_CONFIG_BASE + 0x034)
#define DISP_REG_CONFIG_RDMA1_OUT_SEL    (DISPSYS_CONFIG_BASE + 0x038)
#define DISP_REG_CONFIG_OVL_PQ_OUT_SEL   (DISPSYS_CONFIG_BASE + 0x03C)
#define DISP_REG_CONFIG_WDMA0_SEL        (DISPSYS_CONFIG_BASE + 0x040)
#define DISP_REG_CONFIG_OVL_SEL          (DISPSYS_CONFIG_BASE + 0x044)
#define DISP_REG_CONFIG_OVL_PQ_IN_SEL    (DISPSYS_CONFIG_BASE + 0x048)
#define DISP_REG_CONFIG_COLOR_SEL        (DISPSYS_CONFIG_BASE + 0x04C)
#define DISP_REG_CONFIG_TDSHP_SEL        (DISPSYS_CONFIG_BASE + 0x050)
#define DISP_REG_CONFIG_BLS_SEL          (DISPSYS_CONFIG_BASE + 0x054)
#define DISP_REG_CONFIG_DBI_SEL          (DISPSYS_CONFIG_BASE + 0x058)
#define DISP_REG_CONFIG_DPI0_SEL         (DISPSYS_CONFIG_BASE + 0x05C)
#define DISP_REG_CONFIG_MISC             (DISPSYS_CONFIG_BASE + 0x60)
#define DISP_REG_CONFIG_PATH_DEBUG0      (DISPSYS_CONFIG_BASE + 0x70)
#define DISP_REG_CONFIG_PATH_DEBUG1      (DISPSYS_CONFIG_BASE + 0x74)
#define DISP_REG_CONFIG_PATH_DEBUG2      (DISPSYS_CONFIG_BASE + 0x78)
#define DISP_REG_CONFIG_PATH_DEBUG3      (DISPSYS_CONFIG_BASE + 0x7C)
#define DISP_REG_CONFIG_PATH_DEBUG4      (DISPSYS_CONFIG_BASE + 0x80)
#define DISP_REG_CONFIG_CG_CON0          (DISPSYS_CONFIG_BASE + 0x100)
#define DISP_REG_CONFIG_CG_SET0          (DISPSYS_CONFIG_BASE + 0x104)
#define DISP_REG_CONFIG_CG_CLR0          (DISPSYS_CONFIG_BASE + 0x108)
#define DISP_REG_CONFIG_CG_CON1          (DISPSYS_CONFIG_BASE + 0x110)
#define DISP_REG_CONFIG_CG_SET1          (DISPSYS_CONFIG_BASE + 0x114)
#define DISP_REG_CONFIG_CG_CLR1          (DISPSYS_CONFIG_BASE + 0x118)
#define DISP_REG_CONFIG_HW_DCM_EN0       (DISPSYS_CONFIG_BASE + 0x120)
#define DISP_REG_CONFIG_HW_DCM_EN_SET0   (DISPSYS_CONFIG_BASE + 0x124)
#define DISP_REG_CONFIG_HW_DCM_EN_CLR0   (DISPSYS_CONFIG_BASE + 0x128)
#define DISP_REG_CONFIG_HW_DCM_EN1       (DISPSYS_CONFIG_BASE + 0x130)
#define DISP_REG_CONFIG_HW_DCM_EN_SET1   (DISPSYS_CONFIG_BASE + 0x134)
#define DISP_REG_CONFIG_HW_DCM_EN_CLR1   (DISPSYS_CONFIG_BASE + 0x138)
#define DISP_REG_CONFIG_MBIST_DONE0      (DISPSYS_CONFIG_BASE + 0x800)
#define DISP_REG_CONFIG_MBIST_DONE1      (DISPSYS_CONFIG_BASE + 0x804)
#define DISP_REG_CONFIG_MBIST_FAIL0      (DISPSYS_CONFIG_BASE + 0x808)
#define DISP_REG_CONFIG_MBIST_FAIL1      (DISPSYS_CONFIG_BASE + 0x80C)
#define DISP_REG_CONFIG_MBIST_FAIL2      (DISPSYS_CONFIG_BASE + 0x810)
#define DISP_REG_CONFIG_MBIST_HOLDB0     (DISPSYS_CONFIG_BASE + 0x814)
#define DISP_REG_CONFIG_MBIST_HOLDB1     (DISPSYS_CONFIG_BASE + 0x818)
#define DISP_REG_CONFIG_MBIST_MODE0      (DISPSYS_CONFIG_BASE + 0x81C)
#define DISP_REG_CONFIG_MBIST_MODE1      (DISPSYS_CONFIG_BASE + 0x820)
#define DISP_REG_CONFIG_MBIST_BSEL0      (DISPSYS_CONFIG_BASE + 0x824)
#define DISP_REG_CONFIG_MBIST_BSEL1      (DISPSYS_CONFIG_BASE + 0x828)
#define DISP_REG_CONFIG_MBIST_BSEL2      (DISPSYS_CONFIG_BASE + 0x82C)
#define DISP_REG_CONFIG_MBIST_BSEL3      (DISPSYS_CONFIG_BASE + 0x830)
#define DISP_REG_CONFIG_MBIST_CON        (DISPSYS_CONFIG_BASE + 0x834)
#define DISP_REG_CONFIG_DEBUG_OUT_SEL    (DISPSYS_CONFIG_BASE + 0x870)
#define DISP_REG_CONFIG_TEST_CLK_SEL     (DISPSYS_CONFIG_BASE + 0x874)
#define DISP_REG_CONFIG_DUMMY            (DISPSYS_CONFIG_BASE + 0x880)
#define DDP_MUTEX_INTR_ENABLE_BIT         0x3cf
#define DISP_REG_CONFIG_MUTEX_INTEN      (DISPSYS_MUTEX_BASE + 0x0)
#define DISP_REG_CONFIG_MUTEX_INTSTA     (DISPSYS_MUTEX_BASE + 0x4)
#define DISP_REG_CONFIG_REG_UPD_TIMEOUT  (DISPSYS_MUTEX_BASE + 0x8)
#define DISP_REG_CONFIG_REG_COMMIT       (DISPSYS_MUTEX_BASE + 0xC)
#define DISP_REG_CONFIG_MUTEX_EN(n)      (DISPSYS_MUTEX_BASE + 0x20 + (0x20 * n))
#define DISP_REG_CONFIG_MUTEX(n)         (DISPSYS_MUTEX_BASE + 0x24 + (0x20 * n))
#define DISP_REG_CONFIG_MUTEX_RST(n)     (DISPSYS_MUTEX_BASE + 0x28 + (0x20 * n))
#define DISP_REG_CONFIG_MUTEX_MOD(n)     (DISPSYS_MUTEX_BASE + 0x2C + (0x20 * n))
#define DISP_REG_CONFIG_MUTEX_SOF(n)     (DISPSYS_MUTEX_BASE + 0x30 + (0x20 * n))                                        
#define DISP_REG_CONFIG_MUTEX0_EN        (DISPSYS_MUTEX_BASE + 0x20)    
#define DISP_REG_CONFIG_MUTEX0           (DISPSYS_MUTEX_BASE + 0x24)
#define DISP_REG_CONFIG_MUTEX0_RST       (DISPSYS_MUTEX_BASE + 0x28)
#define DISP_REG_CONFIG_MUTEX0_MOD       (DISPSYS_MUTEX_BASE + 0x2C)
#define DISP_REG_CONFIG_MUTEX0_SOF       (DISPSYS_MUTEX_BASE + 0x30)
#define DISP_REG_CONFIG_MUTEX1_EN        (DISPSYS_MUTEX_BASE + 0x40)
#define DISP_REG_CONFIG_MUTEX1           (DISPSYS_MUTEX_BASE + 0x44)
#define DISP_REG_CONFIG_MUTEX1_RST       (DISPSYS_MUTEX_BASE + 0x48)
#define DISP_REG_CONFIG_MUTEX1_MOD       (DISPSYS_MUTEX_BASE + 0x4C)
#define DISP_REG_CONFIG_MUTEX1_SOF       (DISPSYS_MUTEX_BASE + 0x50)
#define DISP_REG_CONFIG_MUTEX2_EN        (DISPSYS_MUTEX_BASE + 0x60)
#define DISP_REG_CONFIG_MUTEX2           (DISPSYS_MUTEX_BASE + 0x64)
#define DISP_REG_CONFIG_MUTEX2_RST       (DISPSYS_MUTEX_BASE + 0x68)
#define DISP_REG_CONFIG_MUTEX2_MOD       (DISPSYS_MUTEX_BASE + 0x6C)
#define DISP_REG_CONFIG_MUTEX2_SOF       (DISPSYS_MUTEX_BASE + 0x70)
#define DISP_REG_CONFIG_MUTEX3_EN        (DISPSYS_MUTEX_BASE + 0x80)
#define DISP_REG_CONFIG_MUTEX3           (DISPSYS_MUTEX_BASE + 0x84)
#define DISP_REG_CONFIG_MUTEX3_RST       (DISPSYS_MUTEX_BASE + 0x88)
#define DISP_REG_CONFIG_MUTEX3_MOD       (DISPSYS_MUTEX_BASE + 0x8C)
#define DISP_REG_CONFIG_MUTEX3_SOF       (DISPSYS_MUTEX_BASE + 0x90)
#define DISP_REG_CONFIG_MUTEX4_EN        (DISPSYS_MUTEX_BASE + 0xA0)
#define DISP_REG_CONFIG_MUTEX4           (DISPSYS_MUTEX_BASE + 0xA4)
#define DISP_REG_CONFIG_MUTEX4_RST       (DISPSYS_MUTEX_BASE + 0xA8)
#define DISP_REG_CONFIG_MUTEX4_MOD       (DISPSYS_MUTEX_BASE + 0xAC)
#define DISP_REG_CONFIG_MUTEX4_SOF       (DISPSYS_MUTEX_BASE + 0xB0)
#define DISP_REG_CONFIG_MUTEX5_EN        (DISPSYS_MUTEX_BASE + 0xC0)
#define DISP_REG_CONFIG_MUTEX5           (DISPSYS_MUTEX_BASE + 0xC4)
#define DISP_REG_CONFIG_MUTEX5_RST       (DISPSYS_MUTEX_BASE + 0xC8)
#define DISP_REG_CONFIG_MUTEX5_MOD       (DISPSYS_MUTEX_BASE + 0xCC)
#define DISP_REG_CONFIG_MUTEX5_SOF       (DISPSYS_MUTEX_BASE + 0xD0)
#define DISP_REG_CONFIG_MUTEX_DEBUG_OUT_SEL   (DISPSYS_MUTEX_BASE + 0x100)  

//-----------------------------------------------------------------
// OVL
#define DISP_REG_OVL_STA                         (DISPSYS_OVL_BASE + 0x0000)
#define DISP_REG_OVL_INTEN                       (DISPSYS_OVL_BASE + 0x0004)
#define DISP_REG_OVL_INTSTA                      (DISPSYS_OVL_BASE + 0x0008)
#define DISP_REG_OVL_EN                          (DISPSYS_OVL_BASE + 0x000C)
#define DISP_REG_OVL_TRIG                        (DISPSYS_OVL_BASE + 0x0010)
#define DISP_REG_OVL_RST                         (DISPSYS_OVL_BASE + 0x0014)
#define DISP_REG_OVL_ROI_SIZE                    (DISPSYS_OVL_BASE + 0x0020)
#define DISP_REG_OVL_DATAPATH_CON                (DISPSYS_OVL_BASE + 0x0024)
#define DISP_REG_OVL_ROI_BGCLR                   (DISPSYS_OVL_BASE + 0x0028)
#define DISP_REG_OVL_SRC_CON                     (DISPSYS_OVL_BASE + 0x002C)
#define DISP_REG_OVL_L0_CON                      (DISPSYS_OVL_BASE + 0x0030)
#define DISP_REG_OVL_L0_SRCKEY                   (DISPSYS_OVL_BASE + 0x0034)
#define DISP_REG_OVL_L0_SRC_SIZE                 (DISPSYS_OVL_BASE + 0x0038)
#define DISP_REG_OVL_L0_OFFSET                   (DISPSYS_OVL_BASE + 0x003C)
#define DISP_REG_OVL_L0_ADDR                     (DISPSYS_OVL_BASE + 0x0040)
#define DISP_REG_OVL_L0_PITCH                    (DISPSYS_OVL_BASE + 0x0044)
#define DISP_REG_OVL_L1_CON                      (DISPSYS_OVL_BASE + 0x0050)
#define DISP_REG_OVL_L1_SRCKEY                   (DISPSYS_OVL_BASE + 0x0054)
#define DISP_REG_OVL_L1_SRC_SIZE                 (DISPSYS_OVL_BASE + 0x0058)
#define DISP_REG_OVL_L1_OFFSET                   (DISPSYS_OVL_BASE + 0x005C)
#define DISP_REG_OVL_L1_ADDR                     (DISPSYS_OVL_BASE + 0x0060)
#define DISP_REG_OVL_L1_PITCH                    (DISPSYS_OVL_BASE + 0x0064)
#define DISP_REG_OVL_L2_CON                      (DISPSYS_OVL_BASE + 0x0070)
#define DISP_REG_OVL_L2_SRCKEY                   (DISPSYS_OVL_BASE + 0x0074)
#define DISP_REG_OVL_L2_SRC_SIZE                 (DISPSYS_OVL_BASE + 0x0078)
#define DISP_REG_OVL_L2_OFFSET                   (DISPSYS_OVL_BASE + 0x007C)
#define DISP_REG_OVL_L2_ADDR                     (DISPSYS_OVL_BASE + 0x0080)
#define DISP_REG_OVL_L2_PITCH                    (DISPSYS_OVL_BASE + 0x0084)
#define DISP_REG_OVL_L3_CON                      (DISPSYS_OVL_BASE + 0x0090)
#define DISP_REG_OVL_L3_SRCKEY                   (DISPSYS_OVL_BASE + 0x0094)
#define DISP_REG_OVL_L3_SRC_SIZE                 (DISPSYS_OVL_BASE + 0x0098)
#define DISP_REG_OVL_L3_OFFSET                   (DISPSYS_OVL_BASE + 0x009C)
#define DISP_REG_OVL_L3_ADDR                     (DISPSYS_OVL_BASE + 0x00A0)
#define DISP_REG_OVL_L3_PITCH                    (DISPSYS_OVL_BASE + 0x00A4)
#define DISP_REG_OVL_RDMA0_CTRL                  (DISPSYS_OVL_BASE + 0x00C0)
#define DISP_REG_OVL_RDMA0_MEM_START_TRIG        (DISPSYS_OVL_BASE + 0x00C4)
#define DISP_REG_OVL_RDMA0_MEM_GMC_SETTING       (DISPSYS_OVL_BASE + 0x00C8)
#define DISP_REG_OVL_RDMA0_MEM_SLOW_CON          (DISPSYS_OVL_BASE + 0x00CC)
#define DISP_REG_OVL_RDMA0_FIFO_CTRL             (DISPSYS_OVL_BASE + 0x00D0)
#define DISP_REG_OVL_RDMA1_CTRL                  (DISPSYS_OVL_BASE + 0x00E0)
#define DISP_REG_OVL_RDMA1_MEM_START_TRIG        (DISPSYS_OVL_BASE + 0x00E4)
#define DISP_REG_OVL_RDMA1_MEM_GMC_SETTING       (DISPSYS_OVL_BASE + 0x00E8)
#define DISP_REG_OVL_RDMA1_MEM_SLOW_CON          (DISPSYS_OVL_BASE + 0x00EC)
#define DISP_REG_OVL_RDMA1_FIFO_CTRL             (DISPSYS_OVL_BASE + 0x00F0)
#define DISP_REG_OVL_RDMA2_CTRL                  (DISPSYS_OVL_BASE + 0x0100)
#define DISP_REG_OVL_RDMA2_MEM_START_TRIG        (DISPSYS_OVL_BASE + 0x0104)
#define DISP_REG_OVL_RDMA2_MEM_GMC_SETTING       (DISPSYS_OVL_BASE + 0x0108)
#define DISP_REG_OVL_RDMA2_MEM_SLOW_CON          (DISPSYS_OVL_BASE + 0x010C)
#define DISP_REG_OVL_RDMA2_FIFO_CTRL             (DISPSYS_OVL_BASE + 0x0110)
#define DISP_REG_OVL_RDMA3_CTRL                  (DISPSYS_OVL_BASE + 0x0120)
#define DISP_REG_OVL_RDMA3_MEM_START_TRIG        (DISPSYS_OVL_BASE + 0x0124)
#define DISP_REG_OVL_RDMA3_MEM_GMC_SETTING       (DISPSYS_OVL_BASE + 0x0128)
#define DISP_REG_OVL_RDMA3_MEM_SLOW_CON          (DISPSYS_OVL_BASE + 0x012C)
#define DISP_REG_OVL_RDMA3_FIFO_CTRL             (DISPSYS_OVL_BASE + 0x0130)
#define DISP_REG_OVL_L0_Y2R_PARA_R0              (DISPSYS_OVL_BASE + 0x0134)
#define DISP_REG_OVL_L0_Y2R_PARA_R1              (DISPSYS_OVL_BASE + 0x0138)
#define DISP_REG_OVL_L0_Y2R_PARA_G0              (DISPSYS_OVL_BASE + 0x013C)
#define DISP_REG_OVL_L0_Y2R_PARA_G1              (DISPSYS_OVL_BASE + 0x0140)
#define DISP_REG_OVL_L0_Y2R_PARA_B0              (DISPSYS_OVL_BASE + 0x0144)
#define DISP_REG_OVL_L0_Y2R_PARA_B1              (DISPSYS_OVL_BASE + 0x0148)
#define DISP_REG_OVL_L0_Y2R_PARA_YUV_A_0         (DISPSYS_OVL_BASE + 0x014C)
#define DISP_REG_OVL_L0_Y2R_PARA_YUV_A_1         (DISPSYS_OVL_BASE + 0x0150)
#define DISP_REG_OVL_L0_Y2R_PARA_RGB_A_0         (DISPSYS_OVL_BASE + 0x0154)
#define DISP_REG_OVL_L0_Y2R_PARA_RGB_A_1         (DISPSYS_OVL_BASE + 0x0158)
#define DISP_REG_OVL_L1_Y2R_PARA_R0              (DISPSYS_OVL_BASE + 0x015C)
#define DISP_REG_OVL_L1_Y2R_PARA_R1              (DISPSYS_OVL_BASE + 0x0160)
#define DISP_REG_OVL_L1_Y2R_PARA_G0              (DISPSYS_OVL_BASE + 0x0164)
#define DISP_REG_OVL_L1_Y2R_PARA_G1              (DISPSYS_OVL_BASE + 0x0168)
#define DISP_REG_OVL_L1_Y2R_PARA_B0              (DISPSYS_OVL_BASE + 0x016C)
#define DISP_REG_OVL_L1_Y2R_PARA_B1              (DISPSYS_OVL_BASE + 0x0170)
#define DISP_REG_OVL_L1_Y2R_PARA_YUV_A_0         (DISPSYS_OVL_BASE + 0x0174)
#define DISP_REG_OVL_L1_Y2R_PARA_YUV_A_1         (DISPSYS_OVL_BASE + 0x0178)
#define DISP_REG_OVL_L1_Y2R_PARA_RGB_A_0         (DISPSYS_OVL_BASE + 0x017C)
#define DISP_REG_OVL_L1_Y2R_PARA_RGB_A_1         (DISPSYS_OVL_BASE + 0x0180)
#define DISP_REG_OVL_L2_Y2R_PARA_R0              (DISPSYS_OVL_BASE + 0x0184)
#define DISP_REG_OVL_L2_Y2R_PARA_R1              (DISPSYS_OVL_BASE + 0x0188)
#define DISP_REG_OVL_L2_Y2R_PARA_G0              (DISPSYS_OVL_BASE + 0x018C)
#define DISP_REG_OVL_L2_Y2R_PARA_G1              (DISPSYS_OVL_BASE + 0x0190)
#define DISP_REG_OVL_L2_Y2R_PARA_B0              (DISPSYS_OVL_BASE + 0x0194)
#define DISP_REG_OVL_L2_Y2R_PARA_B1              (DISPSYS_OVL_BASE + 0x0198)
#define DISP_REG_OVL_L2_Y2R_PARA_YUV_A_0         (DISPSYS_OVL_BASE + 0x019C)
#define DISP_REG_OVL_L2_Y2R_PARA_YUV_A_1         (DISPSYS_OVL_BASE + 0x01A0)
#define DISP_REG_OVL_L2_Y2R_PARA_RGB_A_0         (DISPSYS_OVL_BASE + 0x01A4)
#define DISP_REG_OVL_L2_Y2R_PARA_RGB_A_1         (DISPSYS_OVL_BASE + 0x01A8)
#define DISP_REG_OVL_L3_Y2R_PARA_R0              (DISPSYS_OVL_BASE + 0x01AC)
#define DISP_REG_OVL_L3_Y2R_PARA_R1              (DISPSYS_OVL_BASE + 0x01B0)
#define DISP_REG_OVL_L3_Y2R_PARA_G0              (DISPSYS_OVL_BASE + 0x01B4)
#define DISP_REG_OVL_L3_Y2R_PARA_G1              (DISPSYS_OVL_BASE + 0x01B8)
#define DISP_REG_OVL_L3_Y2R_PARA_B0              (DISPSYS_OVL_BASE + 0x01BC)
#define DISP_REG_OVL_L3_Y2R_PARA_B1              (DISPSYS_OVL_BASE + 0x01C0)
#define DISP_REG_OVL_L3_Y2R_PARA_YUV_A_0         (DISPSYS_OVL_BASE + 0x01C4)
#define DISP_REG_OVL_L3_Y2R_PARA_YUV_A_1         (DISPSYS_OVL_BASE + 0x01C8)
#define DISP_REG_OVL_L3_Y2R_PARA_RGB_A_0         (DISPSYS_OVL_BASE + 0x01CC)
#define DISP_REG_OVL_L3_Y2R_PARA_RGB_A_1         (DISPSYS_OVL_BASE + 0x01D0)
#define DISP_REG_OVL_DEBUG_MON_SEL               (DISPSYS_OVL_BASE + 0x01D4)
#define DISP_REG_OVL_RDMA0_MEM_GMC_SETTING2      (DISPSYS_OVL_BASE + 0x01E0)
#define DISP_REG_OVL_RDMA1_MEM_GMC_SETTING2      (DISPSYS_OVL_BASE + 0x01E4)
#define DISP_REG_OVL_RDMA2_MEM_GMC_SETTING2      (DISPSYS_OVL_BASE + 0x01E8)
#define DISP_REG_OVL_RDMA3_MEM_GMC_SETTING2      (DISPSYS_OVL_BASE + 0x01EC)
#define DISP_REG_OVL_DUMMY                       (DISPSYS_OVL_BASE + 0x0200)
#define DISP_REG_OVL_FLOW_CTRL_DBG               (DISPSYS_OVL_BASE + 0x0240)
#define DISP_REG_OVL_ADDCON_DBG                  (DISPSYS_OVL_BASE + 0x0244)
#define DISP_REG_OVL_OUTMUX_DBG                  (DISPSYS_OVL_BASE + 0x0248)
#define DISP_REG_OVL_RDMA0_DBG                   (DISPSYS_OVL_BASE + 0x024C)
#define DISP_REG_OVL_RDMA1_DBG                   (DISPSYS_OVL_BASE + 0x0250)
#define DISP_REG_OVL_RDMA2_DBG                   (DISPSYS_OVL_BASE + 0x0254)
#define DISP_REG_OVL_RDMA3_DBG                   (DISPSYS_OVL_BASE + 0x0258)


//-----------------------------------------------------------------
// RDMA
#define DISP_REG_RDMA_INT_ENABLE          (DISPSYS_RDMA0_BASE + 0x0000)
#define DISP_REG_RDMA_INT_STATUS          (DISPSYS_RDMA0_BASE + 0x0004)
#define DISP_REG_RDMA_GLOBAL_CON          (DISPSYS_RDMA0_BASE + 0x0010)
#define DISP_REG_RDMA_SIZE_CON_0          (DISPSYS_RDMA0_BASE + 0x0014)
#define DISP_REG_RDMA_SIZE_CON_1          (DISPSYS_RDMA0_BASE + 0x0018)
#define DISP_REG_RDMA_TARGET_LINE         (DISPSYS_RDMA0_BASE + 0x001C)
#define DISP_REG_RDMA_MEM_CON             (DISPSYS_RDMA0_BASE + 0x0024)
#define DISP_REG_RDMA_MEM_START_ADDR      (DISPSYS_RDMA0_BASE + 0x0028)
#define DISP_REG_RDMA_MEM_SRC_PITCH       (DISPSYS_RDMA0_BASE + 0x002C)
#define DISP_REG_RDMA_MEM_GMC_SETTING_0   (DISPSYS_RDMA0_BASE + 0x0030)
#define DISP_REG_RDMA_MEM_SLOW_CON        (DISPSYS_RDMA0_BASE + 0x0034)
#define DISP_REG_RDMA_MEM_GMC_SETTING_1   (DISPSYS_RDMA0_BASE + 0x0038)
#define DISP_REG_RDMA_FIFO_CON            (DISPSYS_RDMA0_BASE + 0x0040)
#define DISP_REG_RDMA_CF_00               (DISPSYS_RDMA0_BASE + 0x0054)
#define DISP_REG_RDMA_CF_01               (DISPSYS_RDMA0_BASE + 0x0058)
#define DISP_REG_RDMA_CF_02               (DISPSYS_RDMA0_BASE + 0x005C)
#define DISP_REG_RDMA_CF_10               (DISPSYS_RDMA0_BASE + 0x0060)
#define DISP_REG_RDMA_CF_11               (DISPSYS_RDMA0_BASE + 0x0064)
#define DISP_REG_RDMA_CF_12               (DISPSYS_RDMA0_BASE + 0x0068)
#define DISP_REG_RDMA_CF_20               (DISPSYS_RDMA0_BASE + 0x006C)
#define DISP_REG_RDMA_CF_21               (DISPSYS_RDMA0_BASE + 0x0070)
#define DISP_REG_RDMA_CF_22               (DISPSYS_RDMA0_BASE + 0x0074)
#define DISP_REG_RDMA_CF_PRE_ADD0         (DISPSYS_RDMA0_BASE + 0x0078)
#define DISP_REG_RDMA_CF_PRE_ADD1         (DISPSYS_RDMA0_BASE + 0x007C)
#define DISP_REG_RDMA_CF_PRE_ADD2         (DISPSYS_RDMA0_BASE + 0x0080)
#define DISP_REG_RDMA_CF_POST_ADD0        (DISPSYS_RDMA0_BASE + 0x0084)
#define DISP_REG_RDMA_CF_POST_ADD1        (DISPSYS_RDMA0_BASE + 0x0088)
#define DISP_REG_RDMA_CF_POST_ADD2        (DISPSYS_RDMA0_BASE + 0x008C)
#define DISP_REG_RDMA_DUMMY               (DISPSYS_RDMA0_BASE + 0x0090)
#define DISP_REG_RDMA_DEBUG_OUT_SEL       (DISPSYS_RDMA0_BASE + 0x0094)

//-----------------------------------------------------------------
// ROTDMA
#define DISP_REG_ROT_EN                           (DISPSYS_ROT_BASE + 0x0)
#define DISP_REG_ROT_RESET                        (DISPSYS_ROT_BASE + 0x0008)
#define DISP_REG_ROT_INTERRUPT_ENABLE             (DISPSYS_ROT_BASE + 0x0010)
#define DISP_REG_ROT_INTERRUPT_STATUS             (DISPSYS_ROT_BASE + 0x0018)
#define DISP_REG_ROT_CON                          (DISPSYS_ROT_BASE + 0x0020)
#define DISP_REG_ROT_GMCIF_CON                    (DISPSYS_ROT_BASE + 0x0028)
#define DISP_REG_ROT_SRC_CON                      (DISPSYS_ROT_BASE + 0x0030)
#define DISP_REG_ROT_SRC_BASE_0                   (DISPSYS_ROT_BASE + 0x0040)
#define DISP_REG_ROT_SRC_BASE_1                   (DISPSYS_ROT_BASE + 0x0048)
#define DISP_REG_ROT_SRC_BASE_2                   (DISPSYS_ROT_BASE + 0x0050)
#define DISP_REG_ROT_MF_BKGD_SIZE_IN_BYTE         (DISPSYS_ROT_BASE + 0x0060)
#define DISP_REG_ROT_MF_SRC_SIZE                  (DISPSYS_ROT_BASE + 0x0070)
#define DISP_REG_ROT_MF_CLIP_SIZE                 (DISPSYS_ROT_BASE + 0x0078)
#define DISP_REG_ROT_MF_OFFSET_1                  (DISPSYS_ROT_BASE + 0x0080)
#define DISP_REG_ROT_MF_PAR                       (DISPSYS_ROT_BASE + 0x0088)
#define DISP_REG_ROT_SF_BKGD_SIZE_IN_BYTE         (DISPSYS_ROT_BASE + 0x0090)
#define DISP_REG_ROT_SF_PAR                       (DISPSYS_ROT_BASE + 0x00B8)
#define DISP_REG_ROT_MB_DEPTH                     (DISPSYS_ROT_BASE + 0x00C0)
#define DISP_REG_ROT_MB_BASE                      (DISPSYS_ROT_BASE + 0x00C8)
#define DISP_REG_ROT_MB_CON                       (DISPSYS_ROT_BASE + 0x00D0)
#define DISP_REG_ROT_SB_DEPTH                     (DISPSYS_ROT_BASE + 0x00D8)
#define DISP_REG_ROT_SB_BASE                      (DISPSYS_ROT_BASE + 0x00E0)
#define DISP_REG_ROT_SB_CON                       (DISPSYS_ROT_BASE + 0x00E8)
#define DISP_REG_ROT_VC1_RANGE                    (DISPSYS_ROT_BASE + 0x00F0)
#define DISP_REG_ROT_TRANSFORM_0                  (DISPSYS_ROT_BASE + 0x0200)
#define DISP_REG_ROT_TRANSFORM_1                  (DISPSYS_ROT_BASE + 0x0208)
#define DISP_REG_ROT_TRANSFORM_2                  (DISPSYS_ROT_BASE + 0x0210)
#define DISP_REG_ROT_TRANSFORM_3                  (DISPSYS_ROT_BASE + 0x0218)
#define DISP_REG_ROT_TRANSFORM_4                  (DISPSYS_ROT_BASE + 0x0220)
#define DISP_REG_ROT_TRANSFORM_5                  (DISPSYS_ROT_BASE + 0x0228)
#define DISP_REG_ROT_TRANSFORM_6                  (DISPSYS_ROT_BASE + 0x0230)
#define DISP_REG_ROT_TRANSFORM_7                  (DISPSYS_ROT_BASE + 0x0238)
#define DISP_REG_ROT_RESV_DUMMY_0                 (DISPSYS_ROT_BASE + 0x0240)
#define DISP_REG_ROT_CHKS_EXTR                    (DISPSYS_ROT_BASE + 0x0300)
#define DISP_REG_ROT_CHKS_INTW                    (DISPSYS_ROT_BASE + 0x0308)
#define DISP_REG_ROT_CHKS_INTR                    (DISPSYS_ROT_BASE + 0x0310)
#define DISP_REG_ROT_CHKS_ROTO                    (DISPSYS_ROT_BASE + 0x0318)
#define DISP_REG_ROT_CHKS_SRIY                    (DISPSYS_ROT_BASE + 0x0320)
#define DISP_REG_ROT_CHKS_SRIU                    (DISPSYS_ROT_BASE + 0x0328)
#define DISP_REG_ROT_CHKS_SRIV                    (DISPSYS_ROT_BASE + 0x0330)
#define DISP_REG_ROT_CHKS_SROY                    (DISPSYS_ROT_BASE + 0x0338)
#define DISP_REG_ROT_CHKS_SROU                    (DISPSYS_ROT_BASE + 0x0340)
#define DISP_REG_ROT_CHKS_SROV                    (DISPSYS_ROT_BASE + 0x0348)
#define DISP_REG_ROT_CHKS_VUPI                    (DISPSYS_ROT_BASE + 0x0350)
#define DISP_REG_ROT_CHKS_VUPO                    (DISPSYS_ROT_BASE + 0x0358)
#define DISP_REG_ROT_DEBUG_CON                    (DISPSYS_ROT_BASE + 0x0380)
#define DISP_REG_ROT_MON_STA_0                    (DISPSYS_ROT_BASE + 0x0400)
#define DISP_REG_ROT_MON_STA_1                    (DISPSYS_ROT_BASE + 0x0408)
#define DISP_REG_ROT_MON_STA_2                    (DISPSYS_ROT_BASE + 0x0410)
#define DISP_REG_ROT_MON_STA_3                    (DISPSYS_ROT_BASE + 0x0418)
#define DISP_REG_ROT_MON_STA_4                    (DISPSYS_ROT_BASE + 0x0420)
#define DISP_REG_ROT_MON_STA_5                    (DISPSYS_ROT_BASE + 0x0428)
#define DISP_REG_ROT_MON_STA_6                    (DISPSYS_ROT_BASE + 0x0430)
#define DISP_REG_ROT_MON_STA_7                    (DISPSYS_ROT_BASE + 0x0438)
#define DISP_REG_ROT_MON_STA_8                    (DISPSYS_ROT_BASE + 0x0440)
#define DISP_REG_ROT_MON_STA_9                    (DISPSYS_ROT_BASE + 0x0448)
#define DISP_REG_ROT_MON_STA_10                   (DISPSYS_ROT_BASE + 0x0450)
#define DISP_REG_ROT_MON_STA_11                   (DISPSYS_ROT_BASE + 0x0458)
#define DISP_REG_ROT_MON_STA_12                   (DISPSYS_ROT_BASE + 0x0460)
#define DISP_REG_ROT_MON_STA_13                   (DISPSYS_ROT_BASE + 0x0468)
#define DISP_REG_ROT_MON_STA_14                   (DISPSYS_ROT_BASE + 0x0470)
#define DISP_REG_ROT_MON_STA_15                   (DISPSYS_ROT_BASE + 0x0478)
#define DISP_REG_ROT_MON_STA_16                   (DISPSYS_ROT_BASE + 0x0480)
#define DISP_REG_ROT_MON_STA_17                   (DISPSYS_ROT_BASE + 0x0488)
#define DISP_REG_ROT_MON_STA_18                   (DISPSYS_ROT_BASE + 0x0490)
#define DISP_REG_ROT_MON_STA_19                   (DISPSYS_ROT_BASE + 0x0498)
#define DISP_REG_ROT_MON_STA_20                   (DISPSYS_ROT_BASE + 0x04A0)
#define DISP_REG_ROT_MON_STA_21                   (DISPSYS_ROT_BASE + 0x04A8)
#define DISP_REG_ROT_MON_STA_22                   (DISPSYS_ROT_BASE + 0x04B0)

//-----------------------------------------------------------------
// SCL
#define DISP_REG_SCL_CTRL              (DISPSYS_SCL_BASE + 0x0000)
#define DISP_REG_SCL_INTEN             (DISPSYS_SCL_BASE + 0x0004)
#define DISP_REG_SCL_INTSTA            (DISPSYS_SCL_BASE + 0x0008)
#define DISP_REG_SCL_STATUS            (DISPSYS_SCL_BASE + 0x000C)
#define DISP_REG_SCL_CFG               (DISPSYS_SCL_BASE + 0x0010)
#define DISP_REG_SCL_INP_CHKSUM        (DISPSYS_SCL_BASE + 0x0018)
#define DISP_REG_SCL_OUTP_CHKSUM       (DISPSYS_SCL_BASE + 0x001C)
#define DISP_REG_SCL_HRZ_CFG           (DISPSYS_SCL_BASE + 0x0020)
#define DISP_REG_SCL_HRZ_SIZE          (DISPSYS_SCL_BASE + 0x0024)
#define DISP_REG_SCL_HRZ_FACTOR        (DISPSYS_SCL_BASE + 0x0028)
#define DISP_REG_SCL_HRZ_OFFSET        (DISPSYS_SCL_BASE + 0x002C)
#define DISP_REG_SCL_VRZ_CFG           (DISPSYS_SCL_BASE + 0x0040)
#define DISP_REG_SCL_VRZ_SIZE          (DISPSYS_SCL_BASE + 0x0044)
#define DISP_REG_SCL_VRZ_FACTOR        (DISPSYS_SCL_BASE + 0x0048)
#define DISP_REG_SCL_VRZ_OFFSET        (DISPSYS_SCL_BASE + 0x004C)
#define DISP_REG_SCL_EXT_COEF          (DISPSYS_SCL_BASE + 0x0060)
#define DISP_REG_SCL_PEAK_CFG          (DISPSYS_SCL_BASE + 0x0064)

//-----------------------------------------------------------------
// WDMA
#define DISP_REG_WDMA_INTEN                   (DISPSYS_WDMA0_BASE + 0x0000)
#define DISP_REG_WDMA_INTSTA                  (DISPSYS_WDMA0_BASE + 0x0004)
#define DISP_REG_WDMA_EN                      (DISPSYS_WDMA0_BASE + 0x0008)
#define DISP_REG_WDMA_RST                     (DISPSYS_WDMA0_BASE + 0x000C)
#define DISP_REG_WDMA_SMI_CON                 (DISPSYS_WDMA0_BASE + 0x0010)
#define DISP_REG_WDMA_CFG                     (DISPSYS_WDMA0_BASE + 0x0014)
#define DISP_REG_WDMA_SRC_SIZE                (DISPSYS_WDMA0_BASE + 0x0018)
#define DISP_REG_WDMA_CLIP_SIZE               (DISPSYS_WDMA0_BASE + 0x001C)
#define DISP_REG_WDMA_CLIP_COORD              (DISPSYS_WDMA0_BASE + 0x0020)
#define DISP_REG_WDMA_DST_ADDR                (DISPSYS_WDMA0_BASE + 0x0024)
#define DISP_REG_WDMA_DST_W_IN_BYTE           (DISPSYS_WDMA0_BASE + 0x0028)
#define DISP_REG_WDMA_ALPHA                   (DISPSYS_WDMA0_BASE + 0x002C)
#define DISP_REG_WDMA_BUF_ADDR                (DISPSYS_WDMA0_BASE + 0x0030)
#define DISP_REG_WDMA_STA                     (DISPSYS_WDMA0_BASE + 0x0034)
#define DISP_REG_WDMA_BUF_CON1                (DISPSYS_WDMA0_BASE + 0x0038)
#define DISP_REG_WDMA_BUF_CON2                (DISPSYS_WDMA0_BASE + 0x003C)
#define DISP_REG_WDMA_C00                     (DISPSYS_WDMA0_BASE + 0x0040)
#define DISP_REG_WDMA_C02                     (DISPSYS_WDMA0_BASE + 0x0044)
#define DISP_REG_WDMA_C10                     (DISPSYS_WDMA0_BASE + 0x0048)
#define DISP_REG_WDMA_C12                     (DISPSYS_WDMA0_BASE + 0x004C)
#define DISP_REG_WDMA_C20                     (DISPSYS_WDMA0_BASE + 0x0050)
#define DISP_REG_WDMA_C22                     (DISPSYS_WDMA0_BASE + 0x0054)
#define DISP_REG_WDMA_PRE_ADD0                (DISPSYS_WDMA0_BASE + 0x0058)
#define DISP_REG_WDMA_PRE_ADD2                (DISPSYS_WDMA0_BASE + 0x005C)
#define DISP_REG_WDMA_POST_ADD0               (DISPSYS_WDMA0_BASE + 0x0060)
#define DISP_REG_WDMA_POST_ADD2               (DISPSYS_WDMA0_BASE + 0x0064)
#define DISP_REG_WDMA_DST_U_ADDR              (DISPSYS_WDMA0_BASE + 0x0070)
#define DISP_REG_WDMA_DST_V_ADDR              (DISPSYS_WDMA0_BASE + 0x0074)
#define DISP_REG_WDMA_DST_UV_PITCH            (DISPSYS_WDMA0_BASE + 0x0078)
#define DISP_REG_WDMA_DITHER_CON              (DISPSYS_WDMA0_BASE + 0x0090)
#define DISP_REG_WDMA_FLOW_CTRL_DBG           (DISPSYS_WDMA0_BASE + 0x00A0)
#define DISP_REG_WDMA_EXEC_DBG                (DISPSYS_WDMA0_BASE + 0x00A4)
#define DISP_REG_WDMA_CLIP_DBG                (DISPSYS_WDMA0_BASE + 0x00A8)

//-----------------------------------------------------------------
// CMDQ
#define DISP_INT_MUTEX_BIT_MASK     0x00000002

//-----------------------------------------------------------------
// BLS
#define DISP_REG_BLS_EN                     (DISPSYS_BLS_BASE + 0x0000)
#define DISP_REG_BLS_RST                    (DISPSYS_BLS_BASE + 0x0004)
#define DISP_REG_BLS_BLS_SETTING            (DISPSYS_BLS_BASE + 0x0008)
#define DISP_REG_BLS_INTEN                  (DISPSYS_BLS_BASE + 0x0010)
#define DISP_REG_BLS_INTSTA                 (DISPSYS_BLS_BASE + 0x0014)
#define DISP_REG_BLS_SRC_SIZE               (DISPSYS_BLS_BASE + 0x0018)
#define DISP_REG_BLS_PWM_LUT_SEL            (DISPSYS_BLS_BASE + 0x003C)
#define DISP_REG_BLS_PWM_DUTY               (DISPSYS_BLS_BASE + 0x0080)
#define DISP_REG_BLS_PWM_DUTY_GAIN          (DISPSYS_BLS_BASE + 0x0084)
#define DISP_REG_BLS_PWM_CON                (DISPSYS_BLS_BASE + 0x0090)
#define DISP_REG_PWM_H_DURATION             (DISPSYS_BLS_BASE + 0x0094)
#define DISP_REG_PWM_L_DURATION             (DISPSYS_BLS_BASE + 0x0098)
#define DISP_REG_PWM_G_DURATION             (DISPSYS_BLS_BASE + 0x009C)
#define DISP_REG_PWM_SEND_DATA0             (DISPSYS_BLS_BASE + 0x00A0)
#define DISP_REG_PWM_SEND_DATA1             (DISPSYS_BLS_BASE + 0x00A4)
#define DISP_REG_PWM_WAVE_NUM               (DISPSYS_BLS_BASE + 0x00A8)
#define DISP_REG_PWM_DATA_WIDTH             (DISPSYS_BLS_BASE + 0x00AC)
#define DISP_REG_PWM_THRESH                 (DISPSYS_BLS_BASE + 0x00B0)
#define DISP_REG_PWM_SEND_WAVENUM           (DISPSYS_BLS_BASE + 0x00B4)
#define DISP_REG_BLS_GAMMA_SETTING          (DISPSYS_BLS_BASE + 0x0030)
#define DISP_REG_BLS_GAMMA_BOUNDARY         (DISPSYS_BLS_BASE + 0x0034)
#define DISP_REG_BLS_LUT_UPDATE             (DISPSYS_BLS_BASE + 0x0038)
#define DISP_REG_BLS_GAMMA_LUT(X)           (DISPSYS_BLS_BASE + 0x400+(4*(X)))
#define DISP_REG_BLS_IGAMMA_LUT(X)          (DISPSYS_BLS_BASE + 0x800+(4*(X)))
#define DISP_REG_BLS_PWM_LUT(X)             (DISPSYS_BLS_BASE + 0xC00+(4*(X)))
#define DISP_REG_BLS_DITHER(X)              (DISPSYS_BLS_BASE + 0xE00+(4*(X)))

//-----------------------------------------------------------------
// BLS

#define DISP_REG_G2D_START                (DISPSYS_G2D_BASE + 0x00)
#define DISP_REG_G2D_MODE_CON             (DISPSYS_G2D_BASE + 0x04)
#define DISP_REG_G2D_RESET                (DISPSYS_G2D_BASE + 0x08)
#define DISP_REG_G2D_STATUS               (DISPSYS_G2D_BASE + 0x0C)
#define DISP_REG_G2D_IRQ                  (DISPSYS_G2D_BASE + 0x10)

#define DISP_REG_G2D_ALP_CON              (DISPSYS_G2D_BASE + 0x18)

#define DISP_REG_G2D_W2M_CON              (DISPSYS_G2D_BASE + 0x40)
#define DISP_REG_G2D_W2M_ADDR             (DISPSYS_G2D_BASE + 0x44)
#define DISP_REG_G2D_W2M_PITCH            (DISPSYS_G2D_BASE + 0x48)
#define DISP_REG_G2D_W2M_SIZE             (DISPSYS_G2D_BASE + 0x50)

#define DISP_REG_G2D_DST_CON              (DISPSYS_G2D_BASE + 0x80)
#define DISP_REG_G2D_DST_ADDR             (DISPSYS_G2D_BASE + 0x84)
#define DISP_REG_G2D_DST_PITCH            (DISPSYS_G2D_BASE + 0x88)
#define DISP_REG_G2D_DST_COLOR            (DISPSYS_G2D_BASE + 0x94)

#define DISP_REG_G2D_SRC_CON              (DISPSYS_G2D_BASE + 0xC0)
#define DISP_REG_G2D_SRC_ADDR             (DISPSYS_G2D_BASE + 0xC4)
#define DISP_REG_G2D_SRC_PITCH            (DISPSYS_G2D_BASE + 0xC8)
#define DISP_REG_G2D_SRC_COLOR            (DISPSYS_G2D_BASE + 0xD4)

#define DISP_REG_G2D_DI_MAT_0             (DISPSYS_G2D_BASE + 0xD8)
#define DISP_REG_G2D_DI_MAT_1             (DISPSYS_G2D_BASE + 0xDC)

#define DISP_REG_G2D_PMC                  (DISPSYS_G2D_BASE + 0xE0)

#define G2D_RESET_APB_RESET_BIT                 0x0004
#define G2D_RESET_HARD_RESET_BIT                0x0002
#define G2D_RESET_WARM_RESET_BIT                0x0001
#define G2D_STATUS_BUSY_BIT                     0x0001
#define G2D_IRQ_STA_BIT			                0x0100
#define G2D_IRQ_ENABLE_BIT                      0x0001

#endif

