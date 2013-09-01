
 
#ifndef _MT6589_M4U_REG_H__
#define _MT6589_M4U_REG_H__

#include <asm/io.h>

#define M4U_BASE0                0xf0205200
#define M4U_BASE1                0xf0205800 //0x16010000
#define M4U_BASEg                0xf0205000

#define LARB0_BASE 	0xf7001000
#define LARB1_BASE 	0xf6010000
#define LARB2_BASE 	0xf4010000
#define LARB3_BASE 	0xf5001000
#define LARB4_BASE 	0xf5002000

#define SMI_COMMON_EXT_BASE 0xf0202000
//#define SMI_COMMON_EXT_BASE 0x15000000

// SMI ALWAYS ON        
#define SMI_COMMON_AO_BASE		0xf000E000

#define M4U_L2_BASE             0xf0205100
#define M4U_L2_SRAM_PA          0xf2020000



//=================================================
//common macro definitions
#define F_VAL(val,msb,lsb) (((val)&((1<<(msb-lsb+1))-1))<<lsb)
#define F_MSK(msb, lsb)     F_VAL(0xffffffff, msb, lsb)
#define F_BIT_SET(bit)          (1<<(bit))
#define F_BIT_VAL(val,bit)  ((!!(val))<<(bit))
#define F_MSK_SHIFT(regval,msb,lsb) (((regval)&F_MSK(msb,lsb))>>lsb)


//=====================================================
//M4U register definition
//=====================================================

#define REG_MMU_PROG_EN                 0x10
    #define F_MMU_PROG_EN               1
#define REG_MMU_PROG_VA                 0x14
    #define F_PROG_VA_LOCK_BIT             (1<<11)
    #define F_PROG_VA_SECURE_BIT           (1<<10)
    #define F_PROG_VA_MASK            0xfffff000

#define REG_MMU_PROG_DSC                0x18

#define REG_MMU_SQ_START_0_7(x)          (0x20+((x)<<3))
#define REG_MMU_SQ_END_0_7(x)           (0x24+((x)<<3))
#define REG_MMU_SQ_START_8_15(x)        (0x500+(((x)-8)<<3))
#define REG_MMU_SQ_END_8_15(x)          (0x504+(((x)-8)<<3))

#define REG_MMU_SQ_START(x)             (((x)<8) ? REG_MMU_SQ_START_0_7(x): REG_MMU_SQ_START_8_15(x))
    #define F_SQ_VA_MASK                0xfffc0000
    #define F_SQ_EN_BIT                 (1<<17)
    #define F_SQ_MULTI_ENTRY_VAL(x)     (((x)&0xf)<<13)
#define REG_MMU_SQ_END(x)               (((x)<8) ? REG_MMU_SQ_END_0_7(x): REG_MMU_SQ_END_8_15(x))

#define REG_MMU_PFH_DIST0           0x80
#define REG_MMU_PFH_DIST1           0x84
#define REG_MMU_PFH_DIST2           0x88
#define REG_MMU_PFH_DIST3           0x8c
#define REG_MMU_PFH_DIST4           0x90
#define REG_MMU_PFH_DIST5           0x94
#define REG_MMU_PFH_DIST6           0x98

#define REG_MMU_PFH_DIST(port)      (0x80+(((port)>>3)<<2))
    #define F_MMU_PFH_DIST_VAL(port,val) ((val&0xf)<<(((port)&0x7)<<2))
    #define F_MMU_PFH_DIST_MASK(port)     F_MMU_PFH_DIST_VAL((port), 0xf)

#define REG_MMU_PFH_DIST16_0       0xC0
#define REG_MMU_PFH_DISTS16_1       0xC4

#define REG_MMU_PFH_DIR0         0xF0
#define REG_MMU_PFH_DIR1         0xF4
#define REG_MMU_PFH_DIR(port)   (((port)<32) ? REG_MMU_PFH_DIR0: REG_MMU_PFH_DIR1)
#define F_MMU_PFH_DIR(port,val) ((!!(val))<<((port)&0x1f))

#define REG_MMU_MAIN_TAG_0_31(x)       (0x100+((x)<<2))
#define REG_MMU_MAIN_TAG_32_63(x)      (0x400+(((x)-32)<<2))
#define REG_MMU_MAIN_TAG(x) (((x)<32) ? REG_MMU_MAIN_TAG_0_31(x): REG_MMU_MAIN_TAG_32_63(x))
    #define F_MAIN_TLB_LOCK_BIT     (1<<11)
    #define F_MAIN_TLB_VALID_BIT    (1<<10)
    #define F_MAIN_TLB_SQ_EN_BIT    (1<<9)
    #define F_MAIN_TLB_SQ_INDEX_MSK (0xf<<5)
    #define F_MAIN_TLB_INV_DES_BIT      (1<<4)
    #define F_MAIN_TLB_VA_MSK           F_MSK(31, 12)


#define REG_MMU_PFH_TAG_0_31(x)       (0x180+((x)<<2))
#define REG_MMU_PFH_TAG_32_63(x)      (0x480+(((x)-32)<<2))
#define REG_MMU_PFH_TAG(x) (((x)<32) ? REG_MMU_PFH_TAG_0_31(x): REG_MMU_PFH_TAG_32_63(x))
    #define F_PFH_TAG_VA_MSK            F_MSK(31, 14)
    #define F_PFH_TAG_VALID_MSK         F_MSK(13, 10)
    #define F_PFH_TAG_VALID(regval)     F_MSK_SHIFT((regval),13,10)
    #define F_PFH_TAG_VALID_MSK_OF_MVA(mva)  ((F_BIT_SET(10))<<(F_MSK_SHIFT((mva),13,10)))
    #define F_PFH_TAG_DESC_VALID_MSK         F_MSK(9, 6)
    #define F_PFH_TAG_DESC_VALID_MSK_OF_MVA(mva)  ((F_BIT_SET(6))<<(F_MSK_SHIFT(mva,13,10)))
    #define F_PFH_TAG_DESC_VALID(regval)     F_MSK_SHIFT((regval),9,6)
    #define F_PFH_TAG_REQUEST_BY_PFH    F_BIT_SET(5)
    
    

#define REG_MMU_READ_ENTRY       0x200
    #define F_READ_ENTRY_TLB_SEL_PFH        F_VAL(1,0,0)
    #define F_READ_ENTRY_TLB_SEL_MAIN       F_VAL(0,0,0)
    #define F_READ_ENTRY_TLB_SEL_MSK        F_VAL(1,0,0)
    #define F_READ_ENTRY_INDEX_VAL(idx)     F_VAL(idx,10,5)
    #define F_READ_ENTRY_PFH_IDX(idx)       F_VAL(idx,4,3)
    #define F_READ_ENTRY_READ_EN_BIT        F_BIT_SET(0)
    
#define REG_MMU_DES_RDATA        0x204


#define REG_MMU_CTRL_REG         0x210
    #define F_MMU_CTRL_PFH_DIS(dis)         F_BIT_VAL(dis, 0)
    #define F_MMU_CTRL_TLB_WALK_DIS(dis)    F_BIT_VAL(dis, 1)
    #define F_MMU_CTRL_MONITOR_EN(en)       F_BIT_VAL(en, 2)
    #define F_MMU_CTRL_MONITOR_CLR(clr)       F_BIT_VAL(clr, 3)
    #define F_MMU_CTRL_PFH_RT_RPL_MODE(mod)   F_BIT_VAL(mod, 4)
    #define F_MMU_CTRL_TF_PROT_VAL(prot)    F_VAL(prot, 6, 5)
    #define F_MMU_CTRL_TF_PROT_MSK           F_MSK(6,5)
    #define F_MMU_CTRL_INT_HANG_en(en)       F_BIT_VAL(en, 7)
    #define F_MMU_CTRL_COHERE_EN(en)        F_BIT_VAL(en, 8)





#define REG_MMU_IVRP_PADDR       0x214
#define REG_MMU_INT_CONTROL      0x220
    #define F_INT_CLR_BIT (1<<12)
#define REG_MMU_FAULT_ST         0x224
    #define F_INT_TRANSLATION_FAULT                 F_BIT_SET(0)
    #define F_INT_TLB_MULTI_HIT_FAULT               F_BIT_SET(1)
    #define F_INT_INVALID_PHYSICAL_ADDRESS_FAULT    F_BIT_SET(2)
    #define F_INT_ENTRY_REPLACEMENT_FAULT           F_BIT_SET(3)
    #define F_INT_TABLE_WALK_FAULT                  F_BIT_SET(4)
    #define F_INT_TLB_MISS_FAULT                    F_BIT_SET(5)
    #define F_INT_PFH_DMA_FIFO_OVERFLOW             F_BIT_SET(6)
    #define F_INT_MISS_DMA_FIFO_OVERFLOW            F_BIT_SET(7)
    #define F_INT_PFH_FIFO_ERR                      F_BIT_SET(8)
    #define F_INT_MISS_FIFO_ERR                     F_BIT_SET(9)
    
#define REG_MMU_FAULT_VA         0x228
#define REG_MMU_INVLD_PA         0x22C
#define REG_MMU_ACC_CNT          0x230
#define REG_MMU_MAIN_MSCNT       0x234
#define REG_MMU_PF_MSCNT         0x238
#define REG_MMU_PF_CNT           0x23C

#define REG_MMU_WRAP_SA(x)       (0x300+((x)<<3))
#define REG_MMU_WRAP_EA(x)       (0x304+((x)<<3))

#define REG_MMU_WRAP_EN0    0x340
#define REG_MMU_WRAP_EN1    0x344
#define REG_MMU_WRAP_EN2    0x348
#define REG_MMU_WRAP_EN3    0x34c
#define REG_MMU_WRAP_EN4    0x350
#define REG_MMU_WRAP_EN5    0x354
#define REG_MMU_WRAP_EN6    0x358
#define REG_MMU_WRAP_EN(port)         (0x340+(((port)>>3)<<2))
    #define F_MMU_WRAP_SEL_VAL(port, val)  (((val)&0xf)<<(((port)&0x7)<<2))



#define REG_MMU_PFQ_BROADCAST_EN  0x364
#define REG_MMU_NON_BLOCKING_DIS    0x380
    #define F_MMU_NON_BLOCK_DISABLE_BIT 1
#define REG_MMU_RS_PERF_CNT         0x384

#define REG_MMU_INT_ID              0x388
    #define F_INT_ID_TF_PORT_ID(regval)     F_MSK_SHIFT(regval,11, 8)
    #define F_INT_ID_TF_LARB_ID(regval)     F_MSK_SHIFT(regval,14, 12)
    #define F_INT_ID_MH_PORT_ID(regval)     F_MSK_SHIFT(regval,27, 24)
    #define F_INT_ID_MH_LARB_ID(regval)     F_MSK_SHIFT(regval,30, 28)



#define MMU_TOTAL_RS_NR         8
#define REG_MMU_RSx_VA(x)      (0x550+((x)<<2))
    #define F_MMU_RSx_VA_GET(regval)    ((regval)&&F_MSK(31, 12))
    #define F_MMU_RSx_VA_VALID(regval)  F_MSK_SHIFT(regval, 8, 8)
    #define F_MMU_RSx_VA_PID(regval)    F_MSK_SHIFT(regval, 6, 0)
    
#define REG_MMU_RSx_PA(x)      (0x570+((x)<<2))
    #define F_MMU_RSx_PA_GET(regval)    ((regval)&&F_MSK(31, 12))
    #define F_MMU_RSx_PA_VALID(regval)  F_MSK_SHIFT(regval, 1, 0)

#define REG_MMU_RSx_ST(x)      (0x5A0+((x)<<2))
    #define F_MMU_RSx_ST_LID(regval)    F_MSK_SHIFT(regval, 21, 20)
    #define F_MMU_RSx_ST_WRT(regval)    F_MSK_SHIFT(regval, 12, 12)
    #define F_MMU_RSx_ST_OTHER(regval)  F_MSK_SHIFT(regval, 8, 0)


//================================================================
// SMI larb
//================================================================

#define SMI_LARB_STAT            (0x0  )
#define SMI_LARB_CON             (0x10 ) 
    #define F_SMI_LARB_CON_MAU_IRQ_EN(en)   F_BIT_VAL(en, 14)
#define SMI_LARB_CON_SET         (0x14 ) 
#define SMI_LARB_CON_CLR         (0x18 ) 
#define SMI_ARB_CON              (0x20 ) 
#define SMI_ARB_CON_SET          (0x24 ) 
#define SMI_ARB_CON_CLR          (0x28 ) 
#define SMI_LARB_BWFILTER_EN     (0x2c ) 
#define SMI_BW_EXT_CON0          (0x30 ) 
#define SMI_STARV_CON0           (0x40 ) 
#define SMI_STARV_CON1           (0x44 ) 
#define SMI_STARV_CON2           (0x48 ) 
#define SMI_INT_PATH_SEL         (0x54 ) 
#define SMI_BW_VC0               (0x80 ) 
#define SMI_BW_VC1               (0x84 ) 
#define SMI_BW_VC2               (0x88 ) 
#define SMI_EBW_PORT             (0x100)
#define SMI_MAX_GNT              (0x180)

#define SMI_SHARE_EN         (0x210)
    #define F_SMI_SHARE_EN(port)     F_BIT_SET(m4u_port_2_larb_port(port))
#define SMI_ROUTE_SEL     (0x220)
    #define F_SMI_ROUTE_SEL_EMI(port)    F_BIT_SET(m4u_port_2_larb_port(port))
#define SMI_MMULOCK_EN      (0x230)

#define SMI_SUB_PINFO            (0x280)

//====== mau registers ========

#define SMI_MAU_ENTR_START(x)      (0x300+(x)*0x10)
    #define F_MAU_START_WR(en)      F_BIT_VAL(en, 31)
    #define F_MAU_START_RD(en)      F_BIT_VAL(en, 30)
    #define F_MAU_START_ADD_MSK     F_MSK(29, 0)
    #define F_MAU_START_ADD(addr)    F_MSK_SHIFT(addr, 31, 2)
    #define F_MAU_START_IS_WR(regval)   F_MSK_SHIFT(regval, 31, 31)
    #define F_MAU_START_IS_RD(regval)   F_MSK_SHIFT(regval, 30, 30)
    #define F_MAU_START_ADDR_VAL(regval)  (F_MSK_SHIFT(regval, 29, 0)<<2)
#define SMI_MAU_ENTR_END(x)        (0x304+(x)*0x10)
    #define F_MAU_END_VIR(en)      F_BIT_VAL(en, 30)
    #define F_MAU_END_ADD(addr)    F_MSK_SHIFT(addr, 31, 2)
    #define F_MAU_END_IS_VIR(regval) F_MSK_SHIFT(regval, 30, 30)
    #define F_MAU_END_ADDR_VAL(regval) (F_MSK_SHIFT(regval, 29, 0)<<2)
    
#define SMI_MAU_ENTR_GID(x)      (0x308+(x)*0x10)
#define SMI_MAU_ENTR_STAT(x)       (0x500+(x)*0x4)
    #define F_MAU_STAT_ASSERT(regval)   F_MSK_SHIFT(regval, 4, 4)
    #define F_MAU_STAT_ID(regval)       F_MSK_SHIFT(regval, 3, 0)


#define SMI_LARB_MON_ENA         (0x400)
#define SMI_LARB_MON_CLR         (0x404)
#define SMI_LARB_MON_PORT        (0x408)
#define SMI_LARB_MON_TYPE        (0x40c)
#define SMI_LARB_MON_CON         (0x410)
#define SMI_LARB_MON_ACT_CNT     (0x420)
#define SMI_LARB_MON_REQ_CNT     (0x424)
#define SMI_LARB_MON_IDL_CNT     (0x428)
#define SMI_LARB_MON_BEA_CNT     (0x42c)
#define SMI_LARB_MON_BYT_CNT     (0x430)
#define SMI_LARB_MON_CP_CNT      (0x434)
#define SMI_LARB_MON_DP_CNT      (0x438)
#define SMI_LARB_MON_CDP_MAX     (0x43c)
#define SMI_LARB_MON_COS_MAX     (0x440)
#define SMI_LARB_MON_BUS_REQ0    (0x450)
#define SMI_LARB_MON_BUS_REQ1    (0x454)
#define SMI_LARB_MON_WDT_CNT     (0x460)
#define SMI_LARB_MON_RDT_CNT     (0x464)
#define SMI_LARB_MON_OST_CNT     (0x468)

#define SMI_IRQ_STATUS           (0x520)
#define SMI_LARB_FIFO_STAT0      (0x600)
#define SMI_LARB_FIFO_STAT1      (0x604)
#define SMI_LARB_BUS_STAT0       (0x610)
#define SMI_LARB_BUS_STAT1       (0x614)
#define SMI_LARB_DBG_MODE0       (0x700)
#define SMI_LARB_DBG_MODE1       (0x704)
#define SMI_LARB_TST_MODE0       (0x780)


/* ===============================================================
 * 					  SMI COMMON
 * =============================================================== */


#define REG_SMI_MON_AXI_ENA             (0x1a0+SMI_COMMON_EXT_BASE)	
#define REG_SMI_MON_AXI_CLR             (0x1a4+SMI_COMMON_EXT_BASE)	
#define REG_SMI_MON_AXI_TYPE            (0x1ac+SMI_COMMON_EXT_BASE)	
#define REG_SMI_MON_AXI_CON	            (0x1b0+SMI_COMMON_EXT_BASE)	
#define REG_SMI_MON_AXI_ACT_CNT         (0x1c0+SMI_COMMON_EXT_BASE)	
#define REG_SMI_MON_AXI_REQ_CNT         (0x1c4+SMI_COMMON_EXT_BASE)	
#define REG_SMI_MON_AXI_IDL_CNT         (0x1c8+SMI_COMMON_EXT_BASE)	
#define REG_SMI_MON_AXI_BEA_CNT         (0x1cc+SMI_COMMON_EXT_BASE)	
#define REG_SMI_MON_AXI_BYT_CNT         (0x1d0+SMI_COMMON_EXT_BASE)	
#define REG_SMI_MON_AXI_CP_CNT          (0x1d4+SMI_COMMON_EXT_BASE)	
#define REG_SMI_MON_AXI_DP_CNT          (0x1d8+SMI_COMMON_EXT_BASE)	
#define REG_SMI_MON_AXI_CP_MAX          (0x1dc+SMI_COMMON_EXT_BASE)	
#define REG_SMI_MON_AXI_COS_MAX         (0x1e0+SMI_COMMON_EXT_BASE)	
#define REG_SMI_L1LEN	                  (0x200+SMI_COMMON_EXT_BASE)	
    #define F_SMI_L1LEN_AXROUTE_G3D_EMI(en)  F_BIT_VAL(en, 2)
    #define F_SMI_L1LEN_AXROUTE_AUDIO_EMI(en)  F_BIT_VAL(en, 5)
#define REG_SMI_L1ARB0	                (0x204+SMI_COMMON_EXT_BASE)	
#define REG_SMI_L1ARB1	                (0x208+SMI_COMMON_EXT_BASE)	
#define REG_SMI_L1ARB2	                (0x20C+SMI_COMMON_EXT_BASE)	
#define REG_SMI_L1ARB3	                (0x210+SMI_COMMON_EXT_BASE)	
#define REG_SMI_L1ARB4	                (0x214+SMI_COMMON_EXT_BASE)	
#define REG_SMI_L1ARB5	                (0x218+SMI_COMMON_EXT_BASE)	
#define REG_SMI_L1ARB6	                (0x21C+SMI_COMMON_EXT_BASE)	
#define REG_SMI_BUS_SEL	                (0x220+SMI_COMMON_EXT_BASE)
    #define F_SMI_BUS_SEL_larb0(mmu_idx)     F_VAL(mmu_idx, 1, 0)
    #define F_SMI_BUS_SEL_larb1(mmu_idx)     F_VAL(mmu_idx, 3, 2)
    #define F_SMI_BUS_SEL_larb2(mmu_idx)     F_VAL(mmu_idx, 5, 4)
    #define F_SMI_BUS_SEL_larb3(mmu_idx)     F_VAL(mmu_idx, 7, 6)
    #define F_SMI_BUS_SEL_larb4(mmu_idx)     F_VAL(mmu_idx, 9, 8)
    #define F_SMI_BUS_SEL_larb5(mmu_idx)     F_VAL(mmu_idx, 11, 10)
    #define F_SMI_BUS_SEL_larb6(mmu_idx)     F_VAL(mmu_idx, 13, 12)
#define REG_SMI_WRR_REG0                (0x228+SMI_COMMON_EXT_BASE)	
#define REG_SMI_WRR_REG1	              (0x22C+SMI_COMMON_EXT_BASE)	
#define REG_SMI_READ_FIFO_TH            (0x230+SMI_COMMON_EXT_BASE)	
#define REG_SMI_DCM                     (0x300+SMI_COMMON_EXT_BASE)	
#define REG_SMI_DEBUG0                  (0x400+SMI_COMMON_EXT_BASE)	
#define REG_SMI_DEBUG1                  (0x404+SMI_COMMON_EXT_BASE)	
#define REG_SMI_DEBUG2	                (0x408+SMI_COMMON_EXT_BASE)	
#define REG_SMI_DEBUG3	                (0x40C+SMI_COMMON_EXT_BASE)	
#define REG_SMI_DEBUG4                  (0x410+SMI_COMMON_EXT_BASE)	
#define REG_SMI_DEBUG5                  (0x414+SMI_COMMON_EXT_BASE)	
#define REG_SMI_DUMMY                   (0x418+SMI_COMMON_EXT_BASE)	
                                	
        	
#define REG_SMI_CON             	  (0x0010+SMI_COMMON_AO_BASE)	
#define REG_SMI_CON_SET         	  (0x0014+SMI_COMMON_AO_BASE)	
#define REG_SMI_CON_CLR         	  (0x0018+SMI_COMMON_AO_BASE)	
#define REG_SMI_SEN             	  (0x0500+SMI_COMMON_AO_BASE)	
#define REG_D_VIO_CON(x)           	(0x0550+SMI_COMMON_AO_BASE+((x)<<2))
#define REG_D_VIO_STA(x)           	  (0x0560+SMI_COMMON_AO_BASE+((x)<<2))


#define REG_SMI_SECUR_CON(x)       	  (0x05C0+SMI_COMMON_AO_BASE+((x)<<2))
#define REG_SMI_SECUR_CON_OF_PORT(port)     REG_SMI_SECUR_CON(((m4u_port_2_smi_port(port))>>3))
    #define F_SMI_SECUR_CON_SECURE(port)        ((1)<<(((m4u_port_2_smi_port(port))&0x7)<<2))
    #define F_SMI_SECUR_CON_DOMAIN(port, val)   (((val)&0x3)<<((((m4u_port_2_smi_port(port))&0x7)<<2)+1))
    #define F_SMI_SECUR_CON_VIRTUAL(port)       ((1)<<((((m4u_port_2_smi_port(port))&0x7)<<2)+3))
    #define F_SMI_SECUR_CON_MASK(port)          ((0xf)<<((((m4u_port_2_smi_port(port))&0x7)<<2)))

#define REG_SMI_SECUR_CON_G3D	  	  (0x05E8+SMI_COMMON_AO_BASE)	
#define REG_SMI_ISPSYS_SEN      	  (0x0600+SMI_COMMON_AO_BASE)	
#define REG_SMI_ISPSYS_SRAM_RANG(x)		(0x0620+SMI_COMMON_AO_BASE+((x)<<2))
    #define F_SYSRAM_RANG_SEC(en)       F_BIT_VAL(en, 30)
    #define F_SYSRAM_RANG_ADDR_SET(addr)    F_MSK_SHIFT(addr, 16, 4)
    #define F_SYSRAM_RANG_ADDR_GET(regval)    F_MSK_SHIFT(regval, 12, 0)
#define REG_SMI_ISPSYS_SRNG_ACTL(x)	  (0x0630+SMI_COMMON_AO_BASE+((x)<<2))	
#define REG_ISPSYS_D_VIO_CON(x)	  	  (0x0650+SMI_COMMON_AO_BASE+((x)<<2))	
#define REG_ISPSYS_D_VIO_STA(x)    	  (0x0660+SMI_COMMON_AO_BASE+((x)<<2))	
#define REG_ISPSYS_VIO_DBG0     	  (0x0670+SMI_COMMON_AO_BASE)
    #define F_SYSRAM_VIO_DBG0_CLR           F_BIT_SET(31)
    #define F_SYSRAM_VIO_DBG0_RD(regval)    F_MSK_SHIFT(regval, 29, 29)
    #define F_SYSRAM_VIO_DBG0_WR(regval)    F_MSK_SHIFT(regval, 28, 28)
    #define F_SYSRAM_VIO_DBG0_APB(regval)   F_MSK_SHIFT(regval, 24, 24)
    #define F_SYSRAM_VIO_DBG0_DOMAIN(regval) F_MSK_SHIFT(regval, 13, 12)
    #define F_SYSRAM_VIO_DBG0_PORT(regval)  F_MSK_SHIFT(regval, 6, 0)


#define REG_ISPSYS_VIO_DBG1     	  (0x0674+SMI_COMMON_AO_BASE)	
                                	
/* =============================	==================================
 * 					  M4U global        	
 * =============================	================================== */
#define REG_MMUg_CTRL           	 (0x0+M4U_BASEg)
 #define F_MMUg_CTRL_INV_EN0    	 (1<<0)
 #define F_MMUg_CTRL_INV_EN1    	 (1<<1)
 #define F_MMUg_CTRL_INV_EN2    	 (1<<2)
 #define F_MMUg_CTRL_PRE_LOCK(en)    F_BIT_VAL(en, 3)
 #define F_MMUg_CTRL_PRE_EN     	 (1<<4)
                                	
#define REG_MMUg_INVLD          	 (0x4  + M4U_BASEg)    
 #define F_MMUg_INV_ALL         	 0x2   
 #define F_MMUg_INV_RANGE       	 0x1   
                                	    
#define REG_MMUg_INVLD_SA        	 (0x8  + M4U_BASEg)     
#define REG_MMUg_INVLD_EA          (0xC  + M4U_BASEg)    
#define REG_MMUg_PT_BASE           (0x10 + M4U_BASEg)
 #define F_MMUg_PT_VA_MSK        0xffff0000

#define REG_MMUg_L2_SEL            (0x18 + M4U_BASEg)
    #define F_MMUg_L2_SEL_FLUSH_EN(en)          F_BIT_VAL(en, 3)
    #define F_MMUg_L2_SEL_L2_ULTRA(en)          F_BIT_VAL(en, 2)
    #define F_MMUg_L2_SEL_L2_SHARE(en)          F_BIT_VAL(en, 1)
    #define F_MMUg_L2_SEL_L2_BUS_SEL(go_emi)    F_BIT_VAL(go_emi, 0)

    
#define REG_MMUg_DCM               (0x1C + M4U_BASEg)
    #define F_MMUg_DCM_ON(on)       F_BIT_VAL(on, 0)

//registers for security
#define REG_MMUg_CTRL_SEC          (0x20 + M4U_BASEg)
 #define F_MMUg_CTRL_SEC_INV_EN0     (1<<0)
 #define F_MMUg_CTRL_SEC_INV_EN1     (1<<1)
 #define F_MMUg_CTRL_SEC_INV_EN2     (1<<2)
 #define F_MMUg_CTRL_SEC_PRE_LOCK    (1<<3)
 #define F_MMUg_CTRL_SEC_PRE_EN      (1<<4)
 #define F_MMUg_CTRL_SEC_DBG         (1<<5)

#define REG_MMUg_INVLD_SEC           (0x24+M4U_BASEg)
 #define F_MMUg_INV_SEC_ALL          0x2 
 #define F_MMUg_INV_SEC_RANGE        0x1 
                                     
#define REG_MMUg_INVLD_SA_SEC        (0x28+M4U_BASEg)	    
#define REG_MMUg_INVLD_EA_SEC        (0x2C+M4U_BASEg)
#define REG_MMUg_PT_BASE_SEC         (0x30+M4U_BASEg)
 #define F_MMUg_PT_VA_MSK_SEC        0xffff0000


#define REG_MMUg_SEC_ABORT_INFO      (0x34+M4U_BASEg)
    
                                                                          
#define REG_MMUg_INFA_CTRL           (0x80+M4U_BASEg)
#define REG_MMUg_INFA_ST1            (0x84+M4U_BASEg)
#define REG_MMUg_INFA_ST2            (0x88+M4U_BASEg)
#define REG_MMUg_INFA_ST3            (0x8c+M4U_BASEg)

//=================================================================
// smi mmu L2 cache registers
//=================================================================


#define REG_L2_GDC_STATE        (0x0  + M4U_L2_BASE)
    #define F_L2_GDC_ST_LOCK_ALERT_GET(regval)  F_MSK_SHIFT(regval, 15, 8)
    #define F_L2_GDC_ST_LOCK_ALERT_BIT          F_BIT_SET(15)
    #define F_L2_GDC_ST_LOCK_ALERT_SET_IDX_GET(regval)  F_MSK_SHIFT(regval, 14, 8)
    #define F_L2_GDC_ST_EVENT_GET(regval)       F_MSK_SHIFT(regval, 7, 6)
    #define F_L2_GDC_ST_EVENT_MSK               F_MSK(7,6)
    #define F_L2_GDC_ST_EVENT_VAL(val)          F_VAL(val, 7, 6)
    #define F_L2_GDC_ST_OP_ST_GET(regval)       F_MSK_SHIFT(regval, 5, 1)
    #define F_L2_GDC_ST_BUSY_GET(regval)        F_MSK_SHIFT(regval, 0, 0)

#define REG_L2_GDC_OP           (0x4  + M4U_L2_BASE)
    #define F_L2_GDC_BYPASS(en)             F_BIT_VAL(en, 10)
    #define F_GET_L2_GDC_PERF_MASK(regval)  F_MSK_SHIFT(regval, 9, 7)
    #define F_L2_GDC_PERF_MASK(msk)         F_VAL(msk, 9, 7)
        #define GDC_PERF_MASK_HIT_MISS              0
        #define GDC_PERF_MASK_RI_RO                 1
        #define GDC_PERF_MASK_BUSY_CYCLE            3
        #define GDC_PERF_MASK_READ_OUTSTAND_FIFO    3
    #define F_L2_GDC_LOCK_ALERT_DIS(dis)    F_BIT_VAL(dis, 6)
    #define F_L2_GDC_PERF_EN(en)            F_BIT_VAL(en, 5)
    #define F_L2_GDC_SRAM_MODE(en)          F_BIT_VAL(en, 4)
    #define F_L2_GDC_LOCK_TH(th)            F_VAL(th, 3, 2)
    #define F_L2_GDC_PAUSE_OP(op)           F_VAL(op, 1, 0)
        #define GDC_NO_PAUSE            0
        #define GDC_READ_PAUSE          1
        #define GDC_WRITE_PAUSE         2
        #define GDC_READ_WRITE_PAUSE    3
    
                                
#define REG_L2_GDC_PERF0        (0x8  + M4U_L2_BASE)
#define REG_L2_GDC_PERF1        (0xC  + M4U_L2_BASE)
#define REG_L2_GPE_STATUS       (0x18 + M4U_L2_BASE)
  #define F_L2_GPE_ST_RANGE_INV_DONE  2
  #define F_L2_GPE_ST_PREFETCH_DONE  1

#define REG_L2_GPE_STATUS_SEC   (0x20 + M4U_L2_BASE)
  #define F_L2_GPE_ST_RANGE_INV_DONE_SEC  2
  #define F_L2_GPE_ST_PREFETCH_DONE_SEC  1


//================================================================
// L2 parameters
#define MMU_L2_CACHE_SIZE       (24*1024)
#define MMU_L2_SRAM_SIZE        (16*1024)
#define MMU_L2_SET_NR           (128)
#define MMU_L2_WAY_NR           (4)
#define MMU_L2_CACHE_LINE       (48)

/* the pagetable PT transfered by m4u to L2 is:
    [31]        secure bit
    [30:13]     TAG 
    [12:6]      SetIndex
    [5:4]       CacheLineIndex
    [3:0]       BusLineIndex
*/
#define F_L2_PA_SEC_BIT         F_BIT_SET(31)
#define F_L2_PA_TAG_VAL(tag)    F_VAL(tag,30,13)
#define F_L2_PA_TAG_MSK         F_MSK(30, 13)
#define F_L2_PA_SET_INDEX(idx)  F_VAL(idx,12,6)
#define F_L2_PA_SET_MSK         F_MSK(12,6)
#define F_L2_PA_CACHE_LINE(l)   F_VAL(l, 5, 4)

//=================================================================
//other un-register definitions
#define F_DESC_VALID                F_VAL(0x2,1,0)
#define F_DESC_SHARE(en)            F_BIT_VAL(en,2)
#define F_DESC_NONSEC(non_sec)      F_BIT_VAL(non_sec,3)
#define F_DESC_PA_MSK               F_MSK(31,12)


#if 1
static inline unsigned int M4U_ReadReg32(unsigned int M4uBase, unsigned int Offset) 
{
    unsigned int val;
    val = ioread32(M4uBase+Offset);
    
    //printk("read base=0x%x, reg=0x%x, val=0x%x\n",M4uBase,Offset,val );
    return val;
}
static inline void M4U_WriteReg32(unsigned int M4uBase, unsigned int Offset, unsigned int Val) 
{                   
    //unsigned int read;
    iowrite32(Val, M4uBase+Offset);    
    mb();    
    /*
    read = M4U_ReadReg32(M4uBase, Offset);
    if(read != Val)
    {
        printk("error to write base=0x%x, reg=0x%x, val=0x%x, read=0x%x\n",M4uBase,Offset, Val, read );
    }
    else
    {
        printk("write base=0x%x, reg=0x%x, val=0x%x, read=0x%x\n",M4uBase,Offset, Val, read );
    }
*/

}

static inline unsigned int COM_ReadReg32(unsigned int addr) 
{
    return ioread32(addr);
}
static inline void COM_WriteReg32(unsigned int addr, unsigned int Val)
{          
    iowrite32(Val, addr);    
    mb();    
    /*
    if(COM_ReadReg32(addr) != Val)
    {
        printk("error to write add=0x%x, val=0x%x, read=0x%x\n",addr, Val, COM_ReadReg32(addr) );
    }
    else
    {
        printk("write success add=0x%x, val=0x%x, read=0x%x\n",addr, Val, COM_ReadReg32(addr) );
    }
    */
}

static inline unsigned int m4uHw_set_field(unsigned int M4UBase, unsigned int Reg,
                                      unsigned int bit_width, unsigned int shift,
                                      unsigned int value)
{
    unsigned int mask = ((1<<bit_width)-1)<<shift;
    unsigned int old;
    value = (value<<shift)&mask;
    old = M4U_ReadReg32(M4UBase, Reg);
    M4U_WriteReg32(M4UBase, Reg, (old&(~mask))|value);
    return (old&mask)>>shift;
}

#if 0
static inline unsigned int m4uHw_get_field(unsigned int M4UBase, unsigned int Reg,
                                      unsigned int bit_width, unsigned int shift)
{
    unsigned int mask = ((1<<bit_width)-1);
    unsigned int reg = M4U_ReadReg32(M4UBase, Reg);
    return ( (reg>>shift)&mask);
}
#endif

static inline void m4uHw_set_field_by_mask(unsigned int M4UBase, unsigned int reg,
                                      unsigned int mask, unsigned int val)
{
    unsigned int regval;
    regval = M4U_ReadReg32(M4UBase, reg);
    regval = (regval & (~mask))|val;
    M4U_WriteReg32(M4UBase, reg, regval);
}
static inline unsigned int m4uHw_get_field_by_mask(unsigned int M4UBase, unsigned int reg,
                                      unsigned int mask)
{
    return M4U_ReadReg32(M4UBase, reg)&mask;
}

#endif



#endif 

