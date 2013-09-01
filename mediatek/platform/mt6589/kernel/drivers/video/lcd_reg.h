

#ifndef __LCD_REG_H__
#define __LCD_REG_H__

#include <stddef.h>
#include "disp_drv_platform.h"
#ifdef __cplusplus
extern "C" {
#endif


typedef struct
{
    unsigned RUN        : 1;
    unsigned WAIT_CMDQ  : 1;
    unsigned WAIT_HTT   : 1;
    unsigned WAIT_SYNC  : 1;
    unsigned BUSY       : 1;
    unsigned RDMA       : 1;
	unsigned SMI        : 1;
    unsigned rsv_7      : 25;
} LCD_REG_STATUS, *PLCD_REG_STATUS;


typedef struct
{
    unsigned COMPLETED      : 1;
    unsigned CMDQ_COMPLETED : 1;
    unsigned HTT            : 1;
    unsigned SYNC           : 1;
    unsigned rsv_4          : 28;
} LCD_REG_INTERRUPT, *PLCD_REG_INTERRUPT;

 
typedef struct
{
	unsigned RESET     : 1;
	unsigned RDMA_STOP : 1;
    unsigned rsv_2     : 13;
    unsigned START     : 1;
	unsigned rsv_16    : 16;
} LCD_REG_START, *PLCD_REG_START;

typedef struct 
{
	unsigned WR_2ND		: 4;
	unsigned WR_1ST		: 4;
	unsigned RD_2ND		: 4;
	unsigned RD_1ST		: 4;
	unsigned CSH		: 4;
	unsigned CSS		: 4;
	unsigned rsv_24		: 8;
} LCD_REG_SIF_TIMING, *PLCD_REG_SIF_TIMING;

typedef struct
{
    unsigned SIZE_0       : 3;
    unsigned THREE_WIRE_0 : 1;
    unsigned SDI_0        : 1;
    unsigned FIRST_POL_0    : 1;
    unsigned SCK_DEF_0    : 1;
    unsigned DIV2_0       : 1;
	unsigned SIZE_1       : 3;
    unsigned THREE_WIRE_1 : 1;
    unsigned SDI_1        : 1;
    unsigned FIRST_POL_1    : 1;
    unsigned SCK_DEF_1    : 1;
    unsigned DIV2_1       : 1;
    unsigned rsv_16       : 8;
	unsigned HW_CS        : 1;
	unsigned rsv_25       : 7;
} LCD_REG_SCNF, *PLCD_REG_SCNF;

typedef struct
{
    unsigned WST        : 6;
    unsigned rsv_6      : 2;
    unsigned C2WS       : 4;
    unsigned C2WH       : 4;
    unsigned RLT        : 6;
    unsigned rsv_22     : 2;
    unsigned C2RS       : 4;
    unsigned C2RH       : 4;
} LCD_REG_PCNF, *PLCD_REG_PCNF;

typedef struct
{
    unsigned PCNF0_DW   : 3;
    unsigned rsv_3      : 1;
    unsigned PCNF1_DW   : 3;
    unsigned rsv_7      : 1;
    unsigned PCNF2_DW   : 3;
    unsigned rsv_11     : 5;
	unsigned PCNF0_CHW  : 4;
	unsigned PCNF1_CHW  : 4;
	unsigned PCNF2_CHW  : 4;
	unsigned rsv_28     : 4;
} LCD_REG_PCNFDW, *PLCD_REG_PCNFDW;

typedef struct
{
    unsigned ENABLE     : 1;
    unsigned EDGE_SEL   : 1;
    unsigned MODE       : 1;
	unsigned rsv_3      : 12;
    unsigned SW_TE      : 1;
    unsigned rsv_16     : 16;
} LCD_REG_TECON, *PLCD_REG_TECON;

typedef struct
{
    UINT16 WIDTH;
    UINT16 HEIGHT;
} LCD_REG_SIZE, *PLCD_REG_SIZE;

typedef struct
{
	unsigned rsv_0   :4;
	unsigned addr    :4;
	unsigned rsv_8   :24;
} LCD_REG_CMD_ADDR, *PLCD_REG_CMD_ADDR;

typedef struct
{
	unsigned rsv_0   :4;
	unsigned addr    :4;
	unsigned rsv_8   :24;
} LCD_REG_DAT_ADDR, *PLCD_REG_DAT_ADDR;


typedef struct
{
    unsigned RGB_ORDER      : 1;
    unsigned BYTE_ORDER     : 1;
    unsigned PADDING        : 1;
    unsigned DATA_FMT       : 3;
    unsigned IF_FMT         : 2;
    unsigned COMMAND        : 5;
    unsigned rsv_13         : 2;
    unsigned ENC            : 1;
    unsigned rsv_16         : 8;
    unsigned SEND_RES_MODE  : 1;
    unsigned IF_24          : 1;
	unsigned rsv_6          : 6;
}LCD_REG_WROI_CON, *PLCD_REG_WROI_CON;

typedef struct {
    unsigned MAX_BURST          : 3;
    unsigned rsv_3              : 1;
    unsigned THROTTLE_EN        : 1;
    unsigned rsv_5              : 11;
    unsigned THROTTLE_PERIOD    : 16;
} LCD_REG_SMICON;
/*
typedef struct
{
    unsigned DB_B        : 2;
    unsigned rsv_2       : 2;
    unsigned DB_G        : 2;
    unsigned rsv_6       : 2;
    unsigned DB_R        : 2;
    unsigned rsv_10      : 2;
    unsigned LFSR_B_SEED : 4;
    unsigned LFSR_G_SEED : 4;
    unsigned LFSR_R_SEED : 4;
    unsigned rsv_48      : 8;
} LCD_REG_DITHER_CON, *PLCD_REG_DITHER_CON;
*/
typedef struct
{
	unsigned CS0		: 1;
	unsigned CS1		: 1;
	unsigned rsv30		:30;
} LCD_REG_SIF_CS, *PLCD_REG_SIF_CS;

typedef struct 
{
	unsigned TIME_OUT	: 12;
	unsigned rsv_12		: 4;
	unsigned COUNT		: 12;
	unsigned rsv_28		: 4;
} LCD_REG_CALC_HTT, *PLCD_REG_CALC_HTT;

typedef struct 
{
	unsigned HTT		: 10;
	unsigned rsv_10		: 6;
	unsigned VTT		: 12;
	unsigned rsv_28		: 4;
} LCD_REG_SYNC_LCM_SIZE, *PLCD_REG_SYNC_LCM_SIZE;


typedef struct 
{
	unsigned WAITLINE	: 12;
	unsigned rsv_12		: 4;
	unsigned SCANLINE	: 12;
	unsigned rsv_28		: 4;
} LCD_REG_SYNC_CNT, *PLCD_REG_SYNC_CNT;

typedef struct 
{
	unsigned rsv_0      : 16;
	unsigned SWAP	 	: 1;
	unsigned ERR	    : 1;
	unsigned DITHER		: 1;
	unsigned RDMA_EN	: 1;
	unsigned CFMT	    : 3;
	unsigned rsv_23     : 3;
	unsigned CSWAP		: 1;
	unsigned rsv_27     : 5;
} LCD_SRC_CON, *PLCD_SRC_CON;

typedef struct
{
	unsigned DBI_ULTRA 	: 1;
	unsigned GMC_ULTRA  : 1;
	unsigned rsv_2      : 30;
}LCD_REG_ULTRA_CON;

typedef struct
{
	unsigned DBI_TH_LOW   : 16;
	unsigned DBI_TH_HIGH  : 16;
}LCD_REG_DBI_ULTRA_TH;

typedef struct
{
	unsigned GMC_TH_LOW   : 16;
	unsigned GMC_TH_HIGH  : 16;
}LCD_REG_GMC_ULTRA_TH;

typedef struct
{
    LCD_REG_STATUS				STATUS;				// C000
    LCD_REG_INTERRUPT         	INT_ENABLE;         // C004
    LCD_REG_INTERRUPT         	INT_STATUS;         // C008
    LCD_REG_START             	START;              // C00C
    UINT32                    	RESET;              // C010
    UINT32                    	rsv_0014[2];        // C014..C018
	LCD_REG_SIF_TIMING		  	SIF_TIMING[2];	  	// C01C..C020
	UINT32                    	rsv_0024;           // C024
	LCD_REG_SCNF			  	SERIAL_CFG;		  	// C028
	LCD_REG_SIF_CS			  	SIF_CS;			  	// C02C
    LCD_REG_PCNF              	PARALLEL_CFG[3];    // C030..C038
    LCD_REG_PCNFDW            	PARALLEL_DW;        // C03C
    LCD_REG_TECON             	TEARING_CFG;        // C040
	LCD_REG_CALC_HTT			CALC_HTT;			// C044
	LCD_REG_SYNC_LCM_SIZE		SYNC_LCM_SIZE;		// C048
	LCD_REG_SYNC_CNT			SYNC_CNT;			// C04C
    LCD_REG_SMICON            	SMI_CON;            // C050
    UINT32                    	rsv_0054[3];        // C054..C05C
    LCD_REG_WROI_CON          	WROI_CONTROL;       // C060
    LCD_REG_CMD_ADDR           	WROI_CMD_ADDR;      // C064
    LCD_REG_DAT_ADDR           	WROI_DATA_ADDR;     // C068
    LCD_REG_SIZE              	WROI_SIZE;          // C06C
    LCD_SRC_CON                 SRC_CON;            // C070
    UINT32                      SRC_ADD;            // C074
    UINT32                      SRC_PITCH;          // C078
    UINT32                    	rsv_007C[5];           // C07C..C08C
//    LCD_REG_DITHER_CON        	DITHER_CON;         // C080
//    UINT32                    	rsv_0084[3];        // C084..C08C
    LCD_REG_ULTRA_CON          	ULTRA_CON;          // C090
    UINT32                    	CONSUME_RATE;       // C094
    LCD_REG_DBI_ULTRA_TH       	DBI_ULTRA_TH;       // C098
    LCD_REG_GMC_ULTRA_TH       	GMC_ULTRA_TH;       // 109C
    UINT32         	            rsv_00A0[728];      // C0A0..CBFC
	UINT32						CMDQ[32];		    // CC00..CC7F
	UINT32						rsv_1D00[160];		// CC80..CEFC
    UINT32                    	PCMD0;              // CF00
    UINT32                    	rsv_1F04[7];	    // CF04..CF1C
    UINT32                    	PCMD1;              // CF20 
    UINT32                    	rsv_1F24[7];  		// CF24..1C3C
    UINT32                    	PCMD2;              // CF40 
    UINT32                    	rsv_1F44[15];	   	// CF44..CF7C
    UINT32                    	SCMD0;              // CF80
    UINT32                    	rsv_1F84[7];   		// CF84..CF9C
    UINT32                    	SCMD1;              // CFA0
    UINT32                    	rsv_1FA4[7];    	// CFA4..CFBC
} volatile LCD_REGS, *PLCD_REGS;

#ifndef BUILD_UBOOT
STATIC_ASSERT(0x0000 == offsetof(LCD_REGS, STATUS));
STATIC_ASSERT(0x0004 == offsetof(LCD_REGS, INT_ENABLE));
STATIC_ASSERT(0x0028 == offsetof(LCD_REGS, SERIAL_CFG));
STATIC_ASSERT(0x0030 == offsetof(LCD_REGS, PARALLEL_CFG));
STATIC_ASSERT(0x0040 == offsetof(LCD_REGS, TEARING_CFG));

STATIC_ASSERT(0x0C00 == offsetof(LCD_REGS, CMDQ));

STATIC_ASSERT((0xF00) == offsetof(LCD_REGS, PCMD0));
STATIC_ASSERT((0xF80) == offsetof(LCD_REGS, SCMD0));

STATIC_ASSERT(0xFC0 == sizeof(LCD_REGS));
#endif
#define LCD_A0_LOW_OFFSET  (0x0)
#define LCD_A0_HIGH_OFFSET (0x10)

#ifdef __cplusplus
}
#endif

#endif // __LCD_REG_H__
