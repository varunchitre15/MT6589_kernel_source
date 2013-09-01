

#ifndef __DSI_REG_H__
#define __DSI_REG_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct
{
	unsigned PLL_EN                	: 1;
	unsigned PLL_CLKR_EN           	: 1;
	unsigned RG_LNT_LPTX_BIAS_EN   	: 1;
	unsigned RG_LNT_HSTX_EDGE_SEL  	: 1;
	unsigned RG_DSI_PHY_CK_PSEL    	: 1;
	unsigned RG_DPI_EN             	: 1;
	unsigned RG_LNT_HSTX_BIAS_EN   	: 1;
	unsigned RG_LPTX_SWBYPASS      	: 1;
	unsigned RG_PRBS_EN            	: 1;
    unsigned RG_DPI_CKSEL			: 2;
    unsigned RG_CLK_SEL				: 1;
    unsigned rsv_12					: 20;    	    	
} MIPITX_CFG0_REG, *PMIPITX_CFG0_REG;


typedef struct
{
    unsigned RG_PLL_DIV1     		: 7;
    unsigned rsv_7           		: 1;
    unsigned RG_PLL_DIV2			: 4;
    unsigned RG_DSI_CK_SEL			: 2;
    unsigned RG_DSI_CK_DSEL			: 2;
	unsigned rsv_16					: 16;    	    	    	
} MIPITX_CFG1_REG, *PMIPITX_CFG1_REG;


typedef struct
{
    unsigned RG_PLL_BP      		: 1;
    unsigned RG_PLL_BR       		: 1;
    unsigned RG_PLL_POSDIV      	: 2;
    unsigned RG_PLLLBS_RST			: 1;
    unsigned RG_PLL_FMDEN      		: 1;
    unsigned rsv_6          		: 26;
} MIPITX_CFG2_REG, *PMIPITX_CFG2_REG;


typedef struct
{
	unsigned rsv_0					: 2;
	unsigned PG_PLL_PRDIV			: 2;
	unsigned RG_PLL_VCOCAL_CKCTL	: 2;
	unsigned RG_PLL_FBSEL			: 2;
	unsigned RG_PLL_CDIV			: 3;
	unsigned RG_PLL_ACCEN			: 1;
	unsigned RG_PLL_AUTOK_LOAD		: 1;
	unsigned RG_PLL_LOAD_RSTB		: 1;
	unsigned RG_PLL_AUTOK_EN		: 1;
	unsigned RG_PLL_PODIV2			: 1;
	unsigned rsv_16					: 16;
} MIPITX_CFG3_REG, *PMIPITX_CFG3_REG;


typedef struct
{
	unsigned RG_VCOBAND				: 4;
	unsigned rsv_4					: 1;
	unsigned RG_CLK_DLY_EN			: 1;
	unsigned RG_CLK_DLY_SWC			: 2;
	unsigned RG_CLK_DLY_SEL			: 4;
	unsigned RG_CLKB_DLY_SEL		: 4;
	unsigned rsv_16					: 16;	
} MIPITX_CFG4_REG, *PMIPITX_CFG4_REG;


typedef struct
{
	unsigned RG_VCOCAL_CPLT			: 1;
	unsigned RG_CAL_FAIL			: 1;
	unsigned rsv_2					: 2;
	unsigned RG_VCOBANDO			: 4;
	unsigned rsv_8					: 24;
} MIPITX_CFG5_REG, *PMIPITX_CFG5_REG;


typedef struct
{
	unsigned RG_LNT_BGR_EN			: 1;
	unsigned RG_LNT_BGR_CHPEN		: 1;
	unsigned RG_LNT_BGR_SELPH		: 1;
	unsigned rsv_3					: 1;
	unsigned RG_LNT_BGR_DIV			: 2;
	unsigned RG_LNT_BGR_DOUT1_SEL	: 2;
	unsigned RG_LNT_BGR_DOUT2_SEL	: 2;
	unsigned RG_LNT_AIO_SEL			: 4;
	unsigned RG_LNT_AIO_EN			: 1;
	unsigned RG_HSTX_EXTR_EN		: 1;
	unsigned rsv_16					: 16;		
} MIPITX_CFG6_REG, *PMIPITX_CFG6_REG;


typedef struct
{
	unsigned RG_LNT_BGR_PRDIV		: 4;
	unsigned RG_LNT_LPRX_CALI		: 3;
	unsigned rsv_1					: 1;
	unsigned RG_LNT_LPTX_CALI		: 3;
	unsigned RG_CLMP_ENB			: 1;
	unsigned RG_LNT_LPCD_CALI		: 3;
	unsigned rsv_15					: 17;

} MIPITX_CFG7_REG, *PMIPITX_CFG7_REG;


typedef struct
{
	unsigned RG_LNTC_HS_CZ			: 4;
	unsigned RG_LNT0_HS_CZ			: 4;
	unsigned RG_LNT1_HS_CZ			: 4;
	unsigned RG_LNT_CIRE			: 4;	
	unsigned rsv_16					: 16;
} MIPITX_CFG8_REG, *PMIPITX_CFG8_REG;


typedef struct
{
	unsigned RG_TXLDO_FBCAL			: 3;
	unsigned RG_TXLDO_EN			: 1;
	unsigned RG_TXLDO_IBCAL			: 4;
	unsigned RG_TXLDO_OCCAL			: 2;
	unsigned RG_TXLDO_OCP_EN		: 1;
	unsigned RG_TXLDO_PDDOS_EN		: 1;
	unsigned RG_TXLDO_VOCAL			: 4;
	unsigned rsv_16					: 16;
} MIPITX_CFG9_REG, *PMIPITX_CFG9_REG;


typedef struct
{
	unsigned PHY_BIST_MODE			: 1;
	unsigned DSI_BIST_EN			: 1;
	unsigned DSI_FIX_PAT			: 1;
	unsigned DSI_SPE_PAT			: 1;
	unsigned RG_LNT_LOOPBACK		: 3;
	unsigned RG_TXLDO_FB_EN			: 1;
	unsigned RG_TXLDO_TBST_EN		: 1;
	unsigned BIST_LANE_NUM			: 1;
	unsigned MIPI_SW_CTL			: 1;
	unsigned BIST_HS_FREE			: 1;
	unsigned rsv_12					: 20;
} MIPITX_CFGA_REG, *PMIPITX_CFGA_REG;


typedef struct
{
	unsigned BIST_PATTERN0_15		: 16;
	unsigned rsv_16					: 16;		
} MIPITX_CFGB_REG, *PMIPITX_CFGB_REG;


typedef struct
{
	unsigned BIST_PATTERN16_31		: 16;
	unsigned rsv_16 				: 16;
} MIPITX_CFGC_REG, *PMIPITX_CFGC_REG;


typedef struct
{
    MIPITX_CFG0_REG		MIPITX_CON0;		// 0000
    MIPITX_CFG1_REG		MIPITX_CON1;		// 0004
    MIPITX_CFG2_REG		MIPITX_CON2;		// 0008
    MIPITX_CFG3_REG		MIPITX_CON3;		// 000C
    MIPITX_CFG4_REG		MIPITX_CON4;		// 0010    
    MIPITX_CFG5_REG		MIPITX_CON5;		// 0014
    MIPITX_CFG6_REG		MIPITX_CON6;		// 0018
    MIPITX_CFG7_REG		MIPITX_CON7;		// 001C
    MIPITX_CFG8_REG		MIPITX_CON8;		// 0020
    MIPITX_CFG9_REG		MIPITX_CON9;		// 0024        
    MIPITX_CFGA_REG		MIPITX_CONA;		// 0028        
    MIPITX_CFGB_REG		MIPITX_CONB;		// 002C        
    MIPITX_CFGC_REG		MIPITX_CONC;		// 0030        
} volatile DSI_PHY_REGS, *PDSI_PHY_REGS;


typedef struct
{
	unsigned DSI_START	: 1;
	unsigned rsv_1		: 15;
	unsigned VM_CMD_START:1;
	unsigned rsv_17		: 15;
} DSI_START_REG, *PDSI_START_REG;


typedef struct
{
	unsigned rsv_0		   : 1;
	unsigned BUF_UNDERRUN  : 1;
	unsigned rsv_2		   : 2;
	unsigned ESC_ENTRY_ERR : 1;
	unsigned LPDT_SYNC_ERR : 1;
	unsigned CTRL_ERR      : 1;
	unsigned CONTENT_ERR   : 1;
} DSI_STATUS_REG, *PDSI_STATUS_REG;


typedef struct
{
	unsigned RD_RDY			: 1;
	unsigned CMD_DONE		: 1;
	unsigned TE_RDY         : 1;
	unsigned VM_DONE        : 1;
	unsigned EXT_TE         : 1;
	unsigned VM_CMD_DONE    : 1;
	unsigned rsv_6			: 25;
	unsigned BUSY           : 1;
} DSI_INT_ENABLE_REG, *PDSI_INT_ENABLE_REG;


typedef struct
{
	unsigned RD_RDY			: 1;
	unsigned CMD_DONE		: 1;
	unsigned TE_RDY         : 1;
	unsigned VM_DONE        : 1;
	unsigned EXT_TE         : 1;
	unsigned VM_CMD_DONE    : 1;
	unsigned rsv_6			: 25;
	unsigned BUSY           : 1;	
} DSI_INT_STATUS_REG, *PDSI_INT_STATUS_REG;


typedef struct
{
	unsigned DSI_RESET		: 1;
	unsigned rsv_1			: 31;	
} DSI_COM_CTRL_REG, *PDSI_COM_CTRL_REG;


typedef enum
{
	DSI_CMD_MODE 			= 0,
	DSI_SYNC_PULSE_VDO_MODE = 1,
	DSI_SYNC_EVENT_VDO_MODE = 2,
	DSI_BURST_VDO_MODE 		= 3	
} DSI_MODE_CTRL;


typedef struct
{
	unsigned MODE		: 2;
	unsigned rsv_2	    : 14;
	unsigned FRM_MODE   : 1;
	unsigned MIX_MODE   : 1;
	unsigned rsv_18     : 14;
} DSI_MODE_CTRL_REG, *PDSI_MODE_CTRL_REG;


typedef enum
{
	ONE_LANE = 1,
	TWO_LANE = 2
} DSI_LANE_NUM;


typedef struct
{
	unsigned 		VC_NUM			: 2;
	unsigned		LANE_NUM		: 4;
	unsigned		DIS_EOT			: 1;
	unsigned		NULL_EN			: 1;
	unsigned        TE_FREERUN      : 1;
	unsigned		EXT_TE_EN   	: 1;
	unsigned        EXT_TE_EDGE     : 1;
	unsigned        rsv_11          : 1;
	unsigned		MAX_RTN_SIZE	: 4;
	unsigned        HSTX_CKLP_EN    : 1;
	unsigned		rsv_17			: 15;
} DSI_TXRX_CTRL_REG, *PDSI_TXRX_CTRL_REG;


typedef enum
{
	PACKED_PS_16BIT_RGB565=0,
	LOOSELY_PS_18BIT_RGB666=1,	
	PACKED_PS_24BIT_RGB888=2,
	PACKED_PS_18BIT_RGB666=3		
} DSI_PS_TYPE;


typedef struct
{
	unsigned 	DSI_PS_WC	: 14;
	unsigned	rsv_14		: 2;
	unsigned	DSI_PS_SEL	: 2;
	unsigned	rsv_18		: 14;
} DSI_PSCTRL_REG, *PDSI_PSCTRL_REG;


typedef struct
{
	unsigned 	VSA_NL		: 7;
	unsigned 	rsv_7		: 25;
} DSI_VSA_NL_REG, *PDSI_VSA_NL_REG;


typedef struct
{
	unsigned 	VBP_NL		: 7;
	unsigned 	rsv_7		: 25;	
} DSI_VBP_NL_REG, *PDSI_VBP_NL_REG;


typedef struct
{
	unsigned 	VFP_NL		: 7;
	unsigned 	rsv_7		: 25;	
} DSI_VFP_NL_REG, *PDSI_VFP_NL_REG;


typedef struct
{
	unsigned 	VACT_NL		: 12;
	unsigned 	rsv_12		: 20;	
} DSI_VACT_NL_REG, *PDSI_VACT_NL_REG;


typedef struct
{
	unsigned 	HSA_WC		: 12;
	unsigned 	rsv_12		: 20;		
} DSI_HSA_WC_REG, *PDSI_HSA_WC_REG;


typedef struct
{
	unsigned 	HBP_WC		: 12;
	unsigned 	rsv_12		: 20;		
} DSI_HBP_WC_REG, *PDSI_HBP_WC_REG;


typedef struct
{
	unsigned 	HFP_WC		: 12;
	unsigned 	rsv_12		: 20;		
} DSI_HFP_WC_REG, *PDSI_HFP_WC_REG;

typedef struct
{
	unsigned 	BLLP_WC		: 12;
	unsigned 	rsv_12		: 20;		
} DSI_BLLP_WC_REG, *PDSI_BLLP_WC_REG;

typedef struct
{
	unsigned 	CMDQ_SIZE	: 6;
	unsigned 	rsv_6		: 26;		
} DSI_CMDQ_CTRL_REG, *PDSI_CMDQ_CTRL_REG;

typedef struct
{
	unsigned char byte0;
	unsigned char byte1;
	unsigned char byte2;
	unsigned char byte3;	
} DSI_RX_DATA_REG, *PDSI_RX_DATA_REG;


typedef struct
{
	unsigned DSI_RACK	        : 1;
	unsigned DSI_RACK_BYPASS	: 1;
	unsigned rsv2		: 30;	
} DSI_RACK_REG, PDSI_RACK_REG;


typedef struct
{
	unsigned TRIG0		: 1;//remote rst
	unsigned TRIG1		: 1;//TE
	unsigned TRIG2		: 1;//ack	
	unsigned TRIG3		: 1;//rsv
	unsigned RX_ULPS    : 1;
	unsigned DIRECTION  : 1;
	unsigned rsv6		: 26;
} DSI_TRIG_STA_REG, *PDSI_TRIG_STA_REG;


typedef struct
{
	unsigned RWMEM_CONTI	: 16;
	unsigned rsv16          : 16;
} DSI_MEM_CONTI_REG, *PDSI_MEM_CONTI_REG;


typedef struct
{
	unsigned FRM_BC		: 21;
	unsigned rsv21		: 11;
} DSI_FRM_BC_REG, *PDSI_FRM_BC_REG;


typedef struct
{
	unsigned PHY_RST	: 1;
	unsigned rsv1		: 4;
	unsigned HTXTO_RST	: 1;
	unsigned LRXTO_RST	: 1;
	unsigned BTATO_RST	: 1;	
	unsigned rsv8		: 24;	
} DSI_PHY_CON_REG, *PDSI_PHY_CON_REG;


typedef struct
{
	unsigned LC_HS_TX_EN	: 1;
	unsigned LC_ULPM_EN		: 1;
	unsigned LC_WAKEUP_EN	: 1;	
	unsigned rsv3			: 29;
} DSI_PHY_LCCON_REG, *PDSI_PHY_LCCON_REG;


typedef struct
{
	unsigned L0_HS_TX_EN	: 1;
	unsigned L0_ULPM_EN		: 1;
	unsigned L0_WAKEUP_EN	: 1;	
	unsigned rsv3			: 29;
} DSI_PHY_LD0CON_REG, *PDSI_PHY_LD0CON_REG;


typedef struct
{
	unsigned char LPX;
	unsigned char HS_PRPR;
	unsigned char HS_ZERO;
	unsigned char HS_TRAIL;
} DSI_PHY_TIMCON0_REG, *PDSI_PHY_TIMCON0_REG;


typedef struct
{
	unsigned char TA_GO;
	unsigned char TA_SURE;
	unsigned char TA_GET;
	unsigned char DA_HS_EXIT;
} DSI_PHY_TIMCON1_REG, *PDSI_PHY_TIMCON1_REG;


typedef struct
{
	unsigned char CONT_DET;
	unsigned char RSV8;
	unsigned char CLK_ZERO;
	unsigned char CLK_TRAIL;
} DSI_PHY_TIMCON2_REG, *PDSI_PHY_TIMCON2_REG;


typedef struct
{
	unsigned char CLK_HS_PRPR;
	unsigned char CLK_HS_POST;
	unsigned char CLK_HS_EXIT;
	unsigned 	  rsv24		: 8;
} DSI_PHY_TIMCON3_REG, *PDSI_PHY_TIMCON3_REG;


typedef struct
{
	DSI_PHY_TIMCON0_REG	CTRL0;
	DSI_PHY_TIMCON1_REG	CTRL1;
	DSI_PHY_TIMCON2_REG	CTRL2;
	DSI_PHY_TIMCON3_REG	CTRL3;
} DSI_PHY_TIMCON_REG, *PDSI_PHY_TIMCON_REG;


typedef struct
{
	unsigned int	CHECK_SUM;
} DSI_CKSM_OUT_REG, *PDSI_CKSM_OUT_REG;


typedef struct
{
	unsigned			CTL_STATE_C	: 9;
	unsigned            rsv9        : 3;
	unsigned			HX_TX_STATE_C	: 4;
	unsigned 	  	rsv16				: 16;	
} DSI_STATE_DBG0_REG, *PDSI_STATE_DBG0_REG;


typedef struct
{
	unsigned			CTL_STATE_C	: 15;
	unsigned 	  	rsv15					: 1;				
	unsigned			HS_TX_STATE_0	: 5;
	unsigned			rsv21       	: 3;
	unsigned			ESC_STATE_0			: 8;							
} DSI_STATE_DBG1_REG, *PDSI_STATE_DBG1_REG;


typedef struct
{
	unsigned			RX_ESC_STATE			: 10;
	unsigned 	  	rsv10					: 6;
	unsigned			TA_T2R_STATE	: 5;
	unsigned 	  	rsv21					: 3;					
	unsigned			TA_R2T_STATE	: 5;
	unsigned 	  	rsv29					: 3;					
} DSI_STATE_DBG2_REG, *PDSI_STATE_DBG2_REG;


typedef struct
{
	unsigned			CTL_STATE_1			: 5;
	unsigned 	  	rsv5						: 3;	
	unsigned			HS_TX_STATE_1				: 5;	
	unsigned 	  	rsv13						: 3;
	unsigned			CTL_STATE_2 : 5;		
	unsigned 	  	rsv21						: 3;
	unsigned			HS_TX_STATE_2			: 5;
	unsigned 	  	rsv29						: 3;
} DSI_STATE_DBG3_REG, *PDSI_STATE_DBG3_REG;


typedef struct
{
	unsigned			CTL_STATE_3    	: 5;
	unsigned			rsv5						: 3;
	unsigned			HS_TX_STATE_3	: 5;
	unsigned			rsv13						: 3;
	unsigned			rsv16						: 16;
} DSI_STATE_DBG4_REG, *PDSI_STATE_DBG4_REG;


typedef struct
{
	unsigned            BIST_MODE           : 1;
	unsigned            BIST_ENABLE         : 1;
	unsigned            BIST_FIX_PATTERN    : 1;
	unsigned            BIST_SPC_PATTERN    : 1;
	unsigned            BIST_HS_FREE        : 1;
	unsigned            SW_CTL_EN           : 1;
	unsigned            PLL_CK_MON          : 1;
	unsigned            rsv7                : 1;
	unsigned            BIST_LANE_NUM       : 4;
	unsigned            rsv12               : 4;
	unsigned            BIST_TIMING         : 8;
	unsigned            SW_CTL_MODE         : 8;
}DSI_BIST_CON, *PDSI_BIST_CON;

typedef struct
{
	unsigned			VM_CMD_EN			:1;
	unsigned			LONG_PKT			:1;
	unsigned			TIME_SEL			:1;
	unsigned			TS_VSA_EN			:1;
	unsigned			TS_VBP_EN			:1;
	unsigned			TS_VFP_EN			:1;
	unsigned			rsv6				:2;
	unsigned			CM_DATA_ID			:8;
	unsigned			CM_DATA_0			:8;
	unsigned			CM_DATA_1			:8;
}DSI_VM_CMD_CON_REG, *PDSI_VM_CMD_CON_REG;

typedef struct
{
	DSI_START_REG				DSI_START;				// 0000
	DSI_STATUS_REG  			DSI_STA;					// 0004
	DSI_INT_ENABLE_REG	DSI_INTEN;				// 0008
	DSI_INT_STATUS_REG	DSI_INTSTA;				// 000C
	DSI_COM_CTRL_REG		DSI_COM_CTRL;			// 0010
	DSI_MODE_CTRL_REG		DSI_MODE_CTRL;		// 0014
	DSI_TXRX_CTRL_REG		DSI_TXRX_CTRL;		// 0018
	DSI_PSCTRL_REG			DSI_PSCTRL;				// 001C
	DSI_VSA_NL_REG			DSI_VSA_NL;				// 0020
	DSI_VBP_NL_REG			DSI_VBP_NL;				// 0024
	DSI_VFP_NL_REG			DSI_VFP_NL;				// 0028
	DSI_VACT_NL_REG			DSI_VACT_NL;			// 002C
	UINT32                  rsv_30[8];               // 0030..004C
	DSI_HSA_WC_REG			DSI_HSA_WC;				// 0050
	DSI_HBP_WC_REG			DSI_HBP_WC;				// 0054
	DSI_HFP_WC_REG			DSI_HFP_WC;				// 0058
	DSI_BLLP_WC_REG			DSI_BLLP_WC;				// 005C

	DSI_CMDQ_CTRL_REG		DSI_CMDQ_SIZE;		// 0060
	UINT32                  DSI_HSTX_CKL_WC;    // 0064
	UINT32							rsv_0068[3];      // 0068..006C
	DSI_RX_DATA_REG			DSI_RX_DATA0;			// 0074
	DSI_RX_DATA_REG			DSI_RX_DATA1;			// 0078
	DSI_RX_DATA_REG			DSI_RX_DATA2;			// 007c
	DSI_RX_DATA_REG			DSI_RX_DATA3;			// 0080
	DSI_RACK_REG				DSI_RACK;					// 0084
	DSI_TRIG_STA_REG		DSI_TRIG_STA;			// 0088
  UINT32   	        	rsv_008C[1];      // 008C
	DSI_MEM_CONTI_REG		DSI_MEM_CONTI;		// 0090
	DSI_FRM_BC_REG			DSI_FRM_BC;				// 0094
  UINT32   	        	rsv_0098[27];     // 0098..00FC
	DSI_PHY_LCCON_REG		DSI_PHY_LCCON;		// 0104	
	DSI_PHY_LD0CON_REG	DSI_PHY_LD0CON;		// 0108	
  UINT32   	        	rsv_010C[1];      // 010C
	DSI_PHY_TIMCON0_REG	DSI_PHY_TIMECON0;	// 0110	
	DSI_PHY_TIMCON1_REG	DSI_PHY_TIMECON1;	// 0114
	DSI_PHY_TIMCON2_REG	DSI_PHY_TIMECON2;	// 0118
	DSI_PHY_TIMCON3_REG	DSI_PHY_TIMECON3;	// 011C
  UINT32   	        	rsv_0120[4];      // 0120..012c
  	DSI_VM_CMD_CON_REG      DSI_VM_CMD_CON;		//0x130
  	UINT32					rsv_0134[4];			//0x134...0x140
	DSI_CKSM_OUT_REG		DSI_CKSM_OUT;			// 0144
	DSI_STATE_DBG0_REG	DSI_STATE_DBG0;		// 0148
	DSI_STATE_DBG1_REG	DSI_STATE_DBG1;		// 014C
	DSI_STATE_DBG2_REG	DSI_STATE_DBG2;		// 0150
	DSI_STATE_DBG3_REG	DSI_STATE_DBG3;		// 0154
	DSI_STATE_DBG4_REG	DSI_STATE_DBG4;		// 0158
	UINT32    rsv15C[7];                        // 015C..0174
	UINT32    BIST_PATTERN;                         // 0178
	DSI_BIST_CON BIST_CON;                          // 017C
} volatile DSI_REGS, *PDSI_REGS;


typedef struct
{
	unsigned char byte0;
	unsigned char byte1;
	unsigned char byte2;
	unsigned char byte3;
} DSI_CMDQ, *PDSI_CMDQ;

typedef struct
{
	DSI_CMDQ data[32];
} DSI_CMDQ_REGS, *PDSI_CMDQ_REGS;

typedef struct
{
	unsigned char byte0;
	unsigned char byte1;
	unsigned char byte2;
	unsigned char byte3;
} DSI_VM_CMDQ, *PDSI_VM_CMDQ;

typedef struct
{
	DSI_VM_CMDQ data[4];
} DSI_VM_CMDQ_REGS, *PDSI_VM_CMDQ_REGS;

#ifndef BUILD_UBOOT
STATIC_ASSERT(0x002C == offsetof(DSI_REGS, DSI_VACT_NL));
STATIC_ASSERT(0x0104 == offsetof(DSI_REGS, DSI_PHY_LCCON));
STATIC_ASSERT(0x011C == offsetof(DSI_REGS, DSI_PHY_TIMECON3));
STATIC_ASSERT(0x017C == offsetof(DSI_REGS, BIST_CON));
#endif

#ifdef __cplusplus
}
#endif

#endif // __DSI_REG_H__

