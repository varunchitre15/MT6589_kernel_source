

#ifndef __DPI_REG_H__
#define __DPI_REG_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    unsigned EN         : 1;
	unsigned rsv_1      : 31;
}DPI_REG_EN, *PDPI_REG_EN;

typedef struct
{
    unsigned DPI_OEN_OFF    : 1;
    unsigned DPI_BG_EN      : 1;
    unsigned INTL_EN        : 1;
    unsigned TDFP_EN        : 1;
    unsigned CLPF_EN        : 1;
    unsigned YUV422_EN      : 1;
    unsigned YCMUX_EN       : 1;
    unsigned CSC_EN         : 1;
    unsigned EMBSYNC_EN     : 1;
    unsigned rsv_9          : 7;
    unsigned IN_RB_SWAP     : 1;
    unsigned rsv_17         : 15;
} DPI_REG_CNTL, *PDPI_REG_CNTL;


typedef struct
{
    unsigned VSYNC          : 1;
    unsigned rsv_1          : 31;
} DPI_REG_INTERRUPT, *PDPI_REG_INTERRUPT;


typedef struct
{
    UINT16 WIDTH;
    UINT16 HEIGHT;
} DPI_REG_SIZE, *PDPI_REG_SIZE;


typedef struct
{
    unsigned DIVISOR  : 5;
    unsigned rsv_5    : 3;
    unsigned DUTY     : 5;
    unsigned rsv_13   : 18;
    unsigned POLARITY : 1;
} DPI_REG_CLKCNTL, *PDPI_REG_CLKCNTL;

typedef struct
{
    unsigned V_CNT         : 13;
    unsigned rsv_13        : 3;
	unsigned FIELD         : 1;
    unsigned rsv_17        : 3;
	unsigned TDLR          : 1;
    unsigned rsv_21        : 3;
    unsigned DPI_START     : 1;
    unsigned TGEN_START    : 1;
    unsigned OUT_EN        : 1;
    unsigned rsv_27        : 5;
} DPI_REG_STATUS, *PDPI_REG_STATUS;


typedef struct
{
	unsigned OFIX_EN         : 1;
    unsigned rsv_1           : 30;
	unsigned DPI_OEN_ON      : 1;
} DPI_REG_TMODE, *PDPI_REG_TMODE;

typedef struct
{
    unsigned HPW       : 8;
    unsigned HBP       : 8;
    unsigned HFP       : 8;
    unsigned HSYNC_POL : 1;
    unsigned DE_POL    : 1;
    unsigned rsv_26    : 6;
} DPI_REG_TGEN_HCNTL, *PDPI_REG_TGEN_HCNTL;

typedef struct
{
    unsigned HBP       : 12;
	unsigned rsv_12    : 4;
    unsigned HFP       : 12;
    unsigned rsv_28    : 4;
} DPI_REG_TGEN_HPORCH, *PDPI_REG_TGEN_HPORCH;

typedef struct
{
    unsigned VPW_LODD  : 8;
	unsigned VPW_HALF_LODD  : 1;
    unsigned rsv_9    : 23;
} DPI_REG_TGEN_VWIDTH_LODD, *PDPI_REG_TGEN_VWIDTH_LODD;

typedef struct
{
    unsigned VBP_LODD  : 8;
	unsigned VBP_HALF_LODD  : 1;
    unsigned rsv_9    : 7;
    unsigned VFP_LODD  : 8;
	unsigned VFP_HALF_LODD  : 1;
    unsigned rsv_25    : 7;
} DPI_REG_TGEN_VPORCH_LODD, *PDPI_REG_TGEN_VPORCH_LODD;

typedef struct
{
    unsigned VPW_LEVEN  : 7;
	unsigned VPW_HALF_LEVEN  : 1;
    unsigned rsv_8    : 24;
} DPI_REG_TGEN_VWIDTH_LEVEN, *PDPI_REG_TGEN_VWIDTH_LEVEN;

typedef struct
{
    unsigned VBP_LEVEN  : 8;
	unsigned VBP_HALF_LEVEN  : 1;
    unsigned rsv_9    : 7;
    unsigned VFP_LEVEN  : 8;
	unsigned VFP_HALF_LEVEN  : 1;
    unsigned rsv_25    : 7;
} DPI_REG_TGEN_VPORCH_LEVEN, *PDPI_REG_TGEN_VPORCH_LEVEN;

typedef struct
{
    unsigned VPW_RODD  : 8;
	unsigned VPW_HALF_RODD  : 1;
    unsigned rsv_9    : 23;
} DPI_REG_TGEN_VWIDTH_RODD, *PDPI_REG_TGEN_VWIDTH_RODD;

typedef struct
{
    unsigned VBP_RODD  : 8;
	unsigned VBP_HALF_RODD  : 1;
    unsigned rsv_9    : 7;
    unsigned VFP_RODD  : 8;
	unsigned VFP_HALF_RODD  : 1;
    unsigned rsv_25    : 7;
} DPI_REG_TGEN_VPORCH_RODD, *PDPI_REG_TGEN_VPORCH_RODD;

typedef struct
{
    unsigned VPW_REVEN  : 7;
	unsigned VPW_HALF_REVEN  : 1;
    unsigned rsv_8    : 24;
} DPI_REG_TGEN_VWIDTH_REVEN, *PDPI_REG_TGEN_VWIDTH_REVEN;

typedef struct
{
    unsigned VBP_REVEN  : 8;
	unsigned VBP_HALF_REVEN  : 1;
    unsigned rsv_9    : 7;
    unsigned VFP_REVEN  : 8;
	unsigned VFP_HALF_REVEN  : 1;
    unsigned rsv_25    : 7;
} DPI_REG_TGEN_VPORCH_REVEN, *PDPI_REG_TGEN_VPORCH_REVEN;

typedef struct
{
    unsigned ESAV_VOFST_LODD  : 8;
	unsigned ESAV_VWID_LODD   : 8;
    unsigned ESAV_VOFST_LEVEN  : 8;
	unsigned ESAV_VWID_LEVEN   : 8;
} DPI_REG_ESAV_VTIML, *PDPI_REG_ESAV_VTIML;

typedef struct
{
    unsigned ESAV_VOFST_RODD  : 8;
	unsigned ESAV_VWID_RODD   : 8;
    unsigned ESAV_VOFST_REVEN  : 8;
	unsigned ESAV_VWID_REVEN   : 8;
} DPI_REG_ESAV_VTIMR, *PDPI_REG_ESAV_VTIMR;

typedef struct
{
    unsigned ESAV_FOFST_ODD  : 8;
    unsigned ESAV_FOFST_EVEN  : 8;
    unsigned rsv_16    : 16;
} DPI_REG_ESAV_FTIM, *PDPI_REG_ESAV_FTIM;

typedef struct
{
    unsigned VPW       : 8;
    unsigned VBP       : 8;
    unsigned VFP       : 8;
    unsigned VSYNC_POL : 1;
    unsigned rsv_25    : 7;
} DPI_REG_TGEN_VCNTL, *PDPI_REG_TGEN_VCNTL;

typedef struct
{
    unsigned BG_RIGHT  : 11;
	unsigned rsv_11    : 5;
    unsigned BG_LEFT   : 11;
    unsigned rsv_27    : 5;
} DPI_REG_BG_HCNTL, *PDPI_REG_BG_HCNTL;


typedef struct
{
    unsigned BG_BOT   : 10;
	unsigned rsv_10   : 6;
    unsigned BG_TOP   : 10;
    unsigned rsv_26   : 6;
} DPI_REG_BG_VCNTL, *PDPI_REG_BG_VCNTL;


typedef struct
{
    unsigned BG_B     : 8;
	unsigned BG_G     : 8;
    unsigned BG_R     : 8;
    unsigned rsv_24   : 8;
} DPI_REG_BG_COLOR, *PDPI_REG_BG_COLOR;

typedef struct
{
    unsigned VSYNC_POL     : 1;
	unsigned HSYNC_POL     : 1;
    unsigned DE_POL        : 1;
    unsigned rsv_3         : 5;
	unsigned TDLR_INV      : 1;
	unsigned FIELD_INV     : 1;
    unsigned rsv_10        : 22;
} DPI_REG_TGEN_POL, *PDPI_REG_TGEN_POL;

typedef struct
{
	unsigned CLPF_TYPE     : 2;
	unsigned rsv2          : 2;
	unsigned ROUND_EN      : 1;
	unsigned rsv5          : 27;
}DPI_REG_CLPF_SETTING, *PDPI_REG_CLPF_SETTING;

typedef struct
{
	unsigned Y_LIMIT_BOT   : 12;
	unsigned rsv12         : 4;
	unsigned Y_LIMIT_TOP   : 12;
	unsigned rsv28         : 4;
}DPI_REG_Y_LIMIT, *PDPI_REG_Y_LIMIT;

typedef struct
{
	unsigned C_LIMIT_BOT   : 12;
	unsigned rsv12         : 4;
	unsigned C_LIMIT_TOP   : 12;
	unsigned rsv28         : 4;
}DPI_REG_C_LIMIT, *PDPI_REG_C_LIMIT;

typedef struct
{
	unsigned UV_SWAP       : 1;
	unsigned rsv1          : 3;
	unsigned CR_DELSEL     : 1;
	unsigned CB_DELSEL     : 1;
	unsigned Y_DELSEL      : 1;
	unsigned DE_DELSEL     : 1;
}DPI_REG_YUV422_SETTING, *PDPI_REG_YUV422_SETTING;

typedef struct
{
	unsigned EMBVSYNC_R_CR     : 1;
	unsigned EMBVSYNC_G_Y      : 1;
    unsigned EMBVSYNC_B_CB     : 1;
    unsigned rsv_3         	   : 1;
	unsigned ESAV_F_INV        : 1;
	unsigned ESAV_V_INV        : 1;
	unsigned ESAV_H_INV        : 1;
    unsigned rsv_7             : 1;
	unsigned ESAV_CODE_MAN     : 1;
    unsigned rsv_9             : 23;
}DPI_REG_EMBSYNC_SETTING;

typedef struct
{
	unsigned OUT_BIT        :2;
	unsigned OUT_BIT_SWAP   :1;
	unsigned rsv_3          :1;
	unsigned OUT_CH_SWAP    :3;
	unsigned rsv_7          :1;
	unsigned OUT_YC_MAP     :3;
	unsigned rsv_11         :21;
}DPI_REG_OUTPUT_SETTING;

typedef struct
{
	unsigned PAT_EN         :1;
	unsigned rsv_1          :3;
	unsigned PAT_SEL        :3;
	unsigned rsv_6          :1;
	unsigned PAT_B_MAN      :8;
	unsigned PAT_G_MAN      :8;
	unsigned PAT_R_MAN      :8;
}DPI_REG_PATTERN;

typedef struct
{
	unsigned MATRIX_C00     :13;
	unsigned rsv_13         :3;
	unsigned MATRIX_C01     :13;
	unsigned rsv_29         :3;
}DPI_REG_MATRIX_COEFF_SET0;

typedef struct
{
	unsigned MATRIX_C02     :13;
	unsigned rsv_13         :3;
	unsigned MATRIX_C10     :13;
	unsigned rsv_29         :3;
}DPI_REG_MATRIX_COEFF_SET1;

typedef struct
{
	unsigned MATRIX_C11     :13;
	unsigned rsv_13         :3;
	unsigned MATRIX_C12     :13;
	unsigned rsv_29         :3;
}DPI_REG_MATRIX_COEFF_SET2;

typedef struct
{
	unsigned MATRIX_C20     :13;
	unsigned rsv_13         :3;
	unsigned MATRIX_C21     :13;
	unsigned rsv_29         :3;
}DPI_REG_MATRIX_COEFF_SET3;

typedef struct
{
	unsigned MATRIX_C22     :13;
	unsigned rsv_13         :19;
}DPI_REG_MATRIX_COEFF_SET4;

typedef struct
{
	unsigned MATRIX_PRE_ADD_0  :9;
	unsigned rsv_9             :7;
	unsigned MATRIX_PRE_ADD_1  :9;
	unsigned rsv_24            :7;
}DPI_REG_MATRIX_PREADD_SET0;

typedef struct
{
	unsigned MATRIX_PRE_ADD_2  :9;
	unsigned rsv_9             :23;
}DPI_REG_MATRIX_PREADD_SET1;

typedef struct
{
	unsigned MATRIX_POST_ADD_0  :13;
	unsigned rsv_13             :3;
	unsigned MATRIX_POST_ADD_1  :13;
	unsigned rsv_24             :3;
}DPI_REG_MATRIX_POSTADD_SET0;

typedef struct
{
	unsigned MATRIX_POST_ADD_2  :13;
	unsigned rsv_13             :19;
}DPI_REG_MATRIX_POSTADD_SET1;


typedef struct
{
    DPI_REG_EN        DPI_EN;           // 0000
    UINT32   		  DPI_RST;			// 0004
    DPI_REG_INTERRUPT INT_ENABLE;       // 0008
    DPI_REG_INTERRUPT INT_STATUS;       // 000C
    DPI_REG_CNTL      CNTL;             // 0010

	DPI_REG_CLKCNTL   CLK_CNTL; 		// 0014

    DPI_REG_SIZE      SIZE;             // 0018

	UINT32            TGEN_HWIDTH;      // 001C
    DPI_REG_TGEN_HPORCH TGEN_HPORCH;    // 0020

	DPI_REG_TGEN_VWIDTH_LODD TGEN_VWIDTH_LODD;	// 0024
    DPI_REG_TGEN_VPORCH_LODD TGEN_VPORCH_LODD;	// 0028
	DPI_REG_TGEN_VWIDTH_LEVEN TGEN_VWIDTH_LEVEN; // 002C
    DPI_REG_TGEN_VPORCH_LEVEN TGEN_VPORCH_LEVEN; // 0030

	DPI_REG_TGEN_VWIDTH_RODD TGEN_VWIDTH_RODD;	// 0034
    DPI_REG_TGEN_VPORCH_RODD TGEN_VPORCH_RODD;	// 0038
	DPI_REG_TGEN_VWIDTH_REVEN TGEN_VWIDTH_REVEN; // 003C
    DPI_REG_TGEN_VPORCH_REVEN TGEN_VPORCH_REVEN; // 0040

	DPI_REG_ESAV_VTIML ESAV_VTIML;               // 0044
	DPI_REG_ESAV_VTIMR ESAV_VTIMR;               // 0048
	DPI_REG_ESAV_FTIM  ESAV_FTIM;                // 004C
	
    DPI_REG_BG_HCNTL   BG_HCNTL;        // 0050  
    DPI_REG_BG_VCNTL   BG_VCNTL;        // 0054  
    DPI_REG_BG_COLOR   BG_COLOR;        // 0058  

	DPI_REG_TGEN_POL TGEN_POL;          // 005C

	DPI_REG_STATUS  STATUS;             // 0060

	//UINT32 rsv_64[9];                    //0064..0084
	DPI_REG_MATRIX_COEFF_SET0   MATRIX_COEFF_SET0;   // 0064
	DPI_REG_MATRIX_COEFF_SET1   MATRIX_COEFF_SET1;   // 0068
	DPI_REG_MATRIX_COEFF_SET2   MATRIX_COEFF_SET2;   // 006C
	DPI_REG_MATRIX_COEFF_SET3   MATRIX_COEFF_SET3;   // 0070
	DPI_REG_MATRIX_COEFF_SET4   MATRIX_COEFF_SET4;   // 0074
	DPI_REG_MATRIX_PREADD_SET0  MATRIX_PREADD_SET0;  // 0078
	DPI_REG_MATRIX_PREADD_SET1  MATRIX_PREADD_SET1;  // 007C
	DPI_REG_MATRIX_POSTADD_SET0 MATRIX_POSTADD_SET0; // 0080
	DPI_REG_MATRIX_POSTADD_SET1 MATRIX_POSTADD_SET1; // 0084
	
	DPI_REG_CLPF_SETTING    CLPF_SETTING;    //0088
	DPI_REG_Y_LIMIT         Y_LIMIT;         //008C
	DPI_REG_C_LIMIT         C_LIMIT;         //0090
	DPI_REG_YUV422_SETTING  YUV422_SETTING;  //0094
	UINT32                  YCMUX_SETTING;   //0098
	DPI_REG_EMBSYNC_SETTING EMBSYNC_SETTING; //009C
	UINT32 rsv_a0[2];                        //00A0..00A4
	DPI_REG_OUTPUT_SETTING  OUTPUT_SETTING;  //00A8
	DPI_REG_TMODE    TMODE;                  //00AC
	UINT32           CHKSUM;                 //00B0
	DPI_REG_PATTERN  PATTERN;                //00B4
} volatile DPI_REGS, *PDPI_REGS;

#ifndef BUILD_UBOOT
STATIC_ASSERT(0x0018 == offsetof(DPI_REGS, SIZE));
STATIC_ASSERT(0x0034 == offsetof(DPI_REGS, TGEN_VWIDTH_RODD));
STATIC_ASSERT(0x0058 == offsetof(DPI_REGS, BG_COLOR));
STATIC_ASSERT(0x009C == offsetof(DPI_REGS, EMBSYNC_SETTING));
STATIC_ASSERT(0x00B0 == offsetof(DPI_REGS, CHKSUM));
#endif
#ifdef __cplusplus
}
#endif

#endif // __DPI_REG_H__
