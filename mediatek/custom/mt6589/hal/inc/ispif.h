

#ifndef _ISPIF_
#define _ISPIF_

namespace NSIspTuning
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ISP Enable
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CAM_CTL_EN1 CAM+0004H: ISP Enable 1
typedef struct {
    MUINT32 TG1_EN                    : 1;
    MUINT32 TG2_EN                    : 1;
    MUINT32 BIN_EN                    : 1;
    MUINT32 OB_EN                     : 1;
    MUINT32 rsv_4                     : 1;
    MUINT32 LSC_EN                    : 1;
    MUINT32 rsv_6                     : 1;
    MUINT32 BNR_EN                    : 1;
    MUINT32 rsv_8                     : 1;
    MUINT32 HRZ_EN                    : 1;
    MUINT32 rsv_10                    : 1;
    MUINT32 PGN_EN                    : 1;
    MUINT32 PAK_EN                    : 1;
    MUINT32 PAK2_EN                   : 1;
    MUINT32 rsv_14                    : 1;
    MUINT32 SGG_EN                    : 1;
    MUINT32 AF_EN                     : 1;
    MUINT32 FLK_EN                    : 1;
    MUINT32 AA_EN                     : 1;
    MUINT32 LCS_EN                    : 1;
    MUINT32 UNP_EN                    : 1;
    MUINT32 CFA_EN                    : 1;
    MUINT32 CCL_EN                    : 1;
    MUINT32 G2G_EN                    : 1;
    MUINT32 DGM_EN                    : 1;
    MUINT32 LCE_EN                    : 1;
    MUINT32 GGM_EN                    : 1;
    MUINT32 C02_EN                    : 1;
    MUINT32 MFB_EN                    : 1;
    MUINT32 C24_EN                    : 1;
    MUINT32 CAM_EN                    : 1;
    MUINT32 BIN2_EN                   : 1;
} ISP_CAM_CTL_EN1_T;

typedef union {
    enum { MASK     = 0xFFFFBAAF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_CTL_EN1_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_CTL_EN1_T;

// CAM_CTL_EN2 CAM+0008H: ISP Enable 2
typedef struct {
    MUINT32 G2C_EN                    : 1;
    MUINT32 C42_EN                    : 1;
    MUINT32 NBC_EN                    : 1;
    MUINT32 PCA_EN                    : 1;
    MUINT32 SEEE_EN                   : 1;
    MUINT32 NR3D_EN                   : 1;
    MUINT32 rsv_6                     : 8;
    MUINT32 CQ0C_EN                   : 1;
    MUINT32 CQ0B_EN                   : 1;
    MUINT32 EIS_EN                    : 1;
    MUINT32 CDRZ_EN                   : 1;
    MUINT32 CURZ_EN                   : 1;
    MUINT32 rsv_19                    : 2;
    MUINT32 PRZ_EN                    : 1;
    MUINT32 rsv_22                    : 1;
    MUINT32 UV_CRSA_EN                : 1;
    MUINT32 FE_EN                     : 1;
    MUINT32 GDMA_EN                   : 1;
    MUINT32 FMT_EN                    : 1;
    MUINT32 CQ1_EN                    : 1;
    MUINT32 CQ2_EN                    : 1;
    MUINT32 CQ3_EN                    : 1;
    MUINT32 G2G2_EN                   : 1;
    MUINT32 CQ0_EN                    : 1;
} ISP_CAM_CTL_EN2_T;

typedef union {
    enum { MASK     = 0xFFA7C03F };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_CTL_EN2_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_CTL_EN2_T;

//
typedef union {
    enum { COUNT = 2 };
    MUINT32 set[COUNT];
    struct {
        ISP_NVRAM_CTL_EN1_T  en1;
        ISP_NVRAM_CTL_EN2_T  en2;
    };
} ISP_NVRAM_CTL_EN_T;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// MFB
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CAM_MFB_CON CAM+960H
typedef struct {
        MUINT32 BLD_MODE                  : 2;
        MUINT32 rsv_2                     : 2;
        MUINT32 BLD_DB_EN                 : 1;
        MUINT32 rsv_5                     : 3;
        MUINT32 BLD_XDIST                 : 4;
        MUINT32 BLD_YDIST                 : 4;
        MUINT32 BLD_TH_E                  : 8;
        MUINT32 BLD_MM_TH                 : 8;
} ISP_CAM_MFB_CON_T;

typedef union {
    enum { MASK     = 0xFFFF0013 };
    enum { DEFAULT  = 0x0F0F0010 };
    typedef ISP_CAM_MFB_CON_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_MFB_CON_T;

// CAM_MFB_LL CAM+968H
typedef struct {
        MUINT32 BLD_MAX_WEIGHT            : 3;
        MUINT32 rsv_3                     : 1;
        MUINT32 RESERVED                  : 4;
        MUINT32 BLD_LL_Y_DT               : 8;
        MUINT32 BLD_LL_Y_TH1              : 8;
        MUINT32 BLD_LL_Y_TH2              : 8;
} ISP_CAM_MFB_LL_T;

typedef union {
    enum { MASK     = 0xFFFFFFF7 };
    enum { DEFAULT  = 0x140A0606 };
    typedef ISP_CAM_MFB_LL_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_MFB_LL_T;

//
typedef union{
    enum { COUNT = 2 };
    MUINT32 set[COUNT];
    struct {
        ISP_NVRAM_MFB_CON_T con;
        ISP_NVRAM_MFB_LL_T  ll;
    };
} ISP_NVRAM_MFB_T;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// BPC
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CAM_BPC_CON CAM+0800H
typedef struct {
        MUINT32 BPC_ENABLE                : 1;
        MUINT32 rsv_1                     : 3;
        MUINT32 BPC_TABLE_ENABLE          : 1;
        MUINT32 BPC_TABLE_END_MODE        : 1;
        MUINT32 rsv_6                     : 2;
        MUINT32 BPC_HF_MODE               : 1;
        MUINT32 BPC_THR_MODE              : 1;
        MUINT32 rsv_10                    : 2;
        MUINT32 BPC_DETECTION_MODE        : 2;
        MUINT32 rsv_14                    : 2;
        MUINT32 BPC_CORRECTION_MODE       : 2;
        MUINT32 rsv_18                    : 2;
        MUINT32 BPC_OPT_ENABLE            : 1;
        MUINT32 rsv_21                    : 11;
} ISP_CAM_BPC_CON_T;

typedef union {
    enum { MASK     = 0x00133331 };
    enum { DEFAULT  = 0x00100020 };
    typedef ISP_CAM_BPC_CON_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_BPC_CON_T;

// CAM_BPC_CD1_1 CAM+0804H
typedef struct {
        MUINT32 BPC_CON_DARK_G_THR_LA     : 8;
        MUINT32 BPC_CON_DARK_G_THR_LB     : 8;
        MUINT32 BPC_CON_DARK_G_THR_LC     : 8;
        MUINT32 BPC_CON_DARK_G_LA         : 8;
} ISP_CAM_BPC_CD1_1_T;

typedef union {
    enum { MASK     = 0xFFFFFFFF };
    enum { DEFAULT  = 0x1980301B };
    typedef ISP_CAM_BPC_CD1_1_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_BPC_CD1_1_T;

// CAM_BPC_CD1_2 CAM+0808H
typedef struct {
        MUINT32 BPC_CON_DARK_G_LB         : 8;
        MUINT32 BPC_CON_BRIGHT_G_THR_LA   : 8;
        MUINT32 BPC_CON_BRIGHT_G_THR_LB   : 8;
        MUINT32 BPC_CON_BRIGHT_G_THR_LC   : 8;
} ISP_CAM_BPC_CD1_2_T;

typedef union {
    enum { MASK     = 0xFFFFFFFF };
    enum { DEFAULT  = 0x4023152D };
    typedef ISP_CAM_BPC_CD1_2_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_BPC_CD1_2_T;

// CAM_BPC_CD1_3 CAM+080CH
typedef struct {
        MUINT32 BPC_CON_BRIGHT_G_LA       : 8;
        MUINT32 BPC_CON_BRIGHT_G_LB       : 8;
        MUINT32 BPC_CON_DARK_RB_THR_LA    : 8;
        MUINT32 BPC_CON_DARK_RB_THR_LB    : 8;
} ISP_CAM_BPC_CD1_3_T;

typedef union {
    enum { MASK     = 0xFFFFFFFF };
    enum { DEFAULT  = 0x301B2D19 };
    typedef ISP_CAM_BPC_CD1_3_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_BPC_CD1_3_T;

// CAM_BPC_CD1_4 CAM+0810H
typedef struct {
        MUINT32 BPC_CON_DARK_RB_THR_LC    : 8;
        MUINT32 BPC_CON_DARK_RB_LA        : 8;
        MUINT32 BPC_CON_DARK_RB_LB        : 8;
        MUINT32 BPC_CON_BRIGHT_RB_THR_LA  : 8;
} ISP_CAM_BPC_CD1_4_T;

typedef union {
    enum { MASK     = 0xFFFFFFFF };
    enum { DEFAULT  = 0x152D1980 };
    typedef ISP_CAM_BPC_CD1_4_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_BPC_CD1_4_T;

// CAM_BPC_CD1_5 CAM+0814H
typedef struct {
        MUINT32 BPC_CON_BRIGHT_RB_THR_LB  : 8;
        MUINT32 BPC_CON_BRIGHT_RB_THR_LC  : 8;
        MUINT32 BPC_CON_BRIGHT_RB_LA      : 8;
        MUINT32 BPC_CON_BRIGHT_RB_LB      : 8;
} ISP_CAM_BPC_CD1_5_T;

typedef union {
    enum { MASK     = 0xFFFFFFFF };
    enum { DEFAULT  = 0x2D194035 };
    typedef ISP_CAM_BPC_CD1_5_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_BPC_CD1_5_T;

// CAM_BPC_CD1_6 CAM+0818H
typedef struct {
        MUINT32 BPC_CON_CLAMP_VAR_G_LB    : 6;
        MUINT32 BPC_CON_CLAMP_VAR_G_UB    : 10;
        MUINT32 BPC_CON_CLAMP_VAR_RB_LB   : 6;
        MUINT32 BPC_CON_CLAMP_VAR_RB_UB   : 10;
} ISP_CAM_BPC_CD1_6_T;

typedef union {
    enum { MASK     = 0xFFFFFFFF };
    enum { DEFAULT  = 0x20002000 };
    typedef ISP_CAM_BPC_CD1_6_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_BPC_CD1_6_T;

// CAM_BPC_CD2_1 CAM+081CH
typedef struct {
        MUINT32 BPC_HF_OFFSET_G           : 12;
        MUINT32 BPC_HF_SEG_G              : 12;
        MUINT32 BPC_HF_SLOPE_A_G          : 5;
        MUINT32 rsv_29                    : 3;
} ISP_CAM_BPC_CD2_1_T;

typedef union {
    enum { MASK     = 0x1FFFFFFF };
    enum { DEFAULT  = 0x180C812C };
    typedef ISP_CAM_BPC_CD2_1_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_BPC_CD2_1_T;

// CAM_BPC_CD2_2 CAM+0820H
typedef struct {
        MUINT32 BPC_HF_OFFSET_RB          : 12;
        MUINT32 BPC_HF_SEG_RB             : 12;
        MUINT32 BPC_HF_SLOPE_A_RB         : 5;
        MUINT32 rsv_29                    : 3;
} ISP_CAM_BPC_CD2_2_T;

typedef union {
    enum { MASK     = 0x1FFFFFFF };
    enum { DEFAULT  = 0x180C812C };
    typedef ISP_CAM_BPC_CD2_2_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_BPC_CD2_2_T;

// CAM_BPC_CD2_3 CAM+0824H
typedef struct {
        MUINT32 BPC_HF_VAR_THR            : 10;
        MUINT32 rsv_10                    : 2;
        MUINT32 BPC_HF_DIFF_THR           : 4;
        MUINT32 BPC_CON_AVG_OFFSET        : 5;
        MUINT32 rsv_21                    : 3;
        MUINT32 BPC_HF_OFFSET_ALL         : 8;
} ISP_CAM_BPC_CD2_3_T;

typedef union {
    enum { MASK     = 0xFF1FF3FF };
    enum { DEFAULT  = 0x5000B200 };
    typedef ISP_CAM_BPC_CD2_3_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_BPC_CD2_3_T;

// CAM_BPC_CD0 CAM+0828H
typedef struct {
        MUINT32 BPC_IN_RANGE_NUM          : 4;
        MUINT32 rsv_4                     : 4;
        MUINT32 BPC_HF_SLOPE_B_G          : 6;
        MUINT32 rsv_14                    : 2;
        MUINT32 BPC_HF_SLOPE_B_RB         : 6;
        MUINT32 rsv_22                    : 2;
        MUINT32 BPC_CON_EXTRA_THR         : 4;
        MUINT32 rsv_28                    : 4;
} ISP_CAM_BPC_CD0_T;

typedef union {
    enum { MASK     = 0x0F3F3F0F };
    enum { DEFAULT  = 0x010F0F01 };
    typedef ISP_CAM_BPC_CD0_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_BPC_CD0_T;

// CAM_BPC_DET CAM+082CH
typedef struct {
        MUINT32 BPC_STRONG_BP_D_THR_LA    : 8;
        MUINT32 BPC_STRONG_BP_B_THR_LA    : 8;
        MUINT32 BPC_STRONG_BP_D_THR_LB    : 8;
        MUINT32 BPC_STRONG_BP_B_THR_LB    : 8;
} ISP_CAM_BPC_DET_T;

typedef union {
    enum { MASK     = 0xFFFFFFFF };
    enum { DEFAULT  = 0x26260808 };
    typedef ISP_CAM_BPC_DET_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_BPC_DET_T;

// CAM_BPC_COR CAM+0830H
typedef struct {
        MUINT32 BPC_CR2_MAX               : 8;
        MUINT32 BPC_CR2_THR               : 8;
        MUINT32 BPC_RANKING_INDEX         : 3;
        MUINT32 rsv_19                    : 13;
} ISP_CAM_BPC_COR_T;

typedef union {
    enum { MASK     = 0x0007FFFF };
    enum { DEFAULT  = 0x00010519 };
    typedef ISP_CAM_BPC_COR_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_BPC_COR_T;

// CAM_BPC_TBLI1 CAM+0838H
typedef struct {
        MUINT32 BPC_XOFFSET               : 13;
        MUINT32 rsv_13                    : 3;
        MUINT32 BPC_YOFFSET               : 13;
        MUINT32 rsv_29                    : 3;
} ISP_CAM_BPC_TBLI1_T;

typedef union {
    enum { MASK     = 0x1FFF1FFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_BPC_TBLI1_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_BPC_TBLI1_T;

// CAM_BPC_TBLI2 CAM+083CH
typedef struct {
        MUINT32 BPC_XSIZE                 : 13;
        MUINT32 rsv_13                    : 3;
        MUINT32 BPC_YSIZE                 : 13;
        MUINT32 rsv_29                    : 3;
} ISP_CAM_BPC_TBLI2_T;

typedef union {
    enum { MASK     = 0x1FFF1FFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_BPC_TBLI2_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_BPC_TBLI2_T;

//
typedef union {
    enum { COUNT = 15 };
    MUINT32 set[COUNT];
    struct {
        ISP_NVRAM_BPC_CON_T     con;
        ISP_NVRAM_BPC_CD1_1_T   cd1_1;
        ISP_NVRAM_BPC_CD1_2_T   cd1_2;
        ISP_NVRAM_BPC_CD1_3_T   cd1_3;
        ISP_NVRAM_BPC_CD1_4_T   cd1_4;
        ISP_NVRAM_BPC_CD1_5_T   cd1_5;
        ISP_NVRAM_BPC_CD1_6_T   cd1_6;
        ISP_NVRAM_BPC_CD2_1_T   cd2_1;
        ISP_NVRAM_BPC_CD2_2_T   cd2_2;
        ISP_NVRAM_BPC_CD2_3_T   cd2_3;
        ISP_NVRAM_BPC_CD0_T     cd0;
        ISP_NVRAM_BPC_DET_T     det;
        ISP_NVRAM_BPC_COR_T     cor;
        ISP_NVRAM_BPC_TBLI1_T tbli1;
        ISP_NVRAM_BPC_TBLI2_T tbli2;
    };
} ISP_NVRAM_BPC_T;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// LSC
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
typedef struct {
    MUINT32 rsv_0                       : 1;
    MUINT32 LSCI_EN                     : 1;
    MUINT32 rsv_1                       : 30;
} ISP_CAM_LSCI_EN;

typedef union {
    enum { MASK     = 0x00000002 };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_LSCI_EN reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_LSCI_EN_T;

typedef struct {
    MUINT32 BASE_ADDR               : 32;
} ISP_CAM_LSCI_BA;

typedef union {
    enum { MASK     = 0xffffffff };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_LSCI_BA reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_LSCI_BA_T;

typedef struct {
    MUINT32 XSIZE               : 16;
    MUINT32 rsvd                : 16;
} ISP_CAM_LSCI_XSIZE;

typedef union {
    enum { MASK     = 0x0000ffff };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_LSCI_XSIZE reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_LSCI_XSIZE_T;

typedef struct {
    MUINT32 YSIZE               : 16;
    MUINT32 rsvd                : 16;
} ISP_CAM_LSCI_YSIZE;

typedef union {
    enum { MASK     = 0x0000ffff };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_LSCI_YSIZE reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_LSCI_YSIZE_T;

typedef struct {
    MUINT32 rsv_0                     : 5;
    MUINT32 LSC_EN                    : 1;
    MUINT32 rsv_1                     :26;
} ISP_CAM_LSC_EN;

typedef union {
    enum { MASK     = 0x00000020 };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_LSC_EN reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_LSC_EN_T;

// CAM_LSC_CTL1 CAM+0530H
typedef struct {
        MUINT32 SDBLK_YOFST               : 6;
        MUINT32 rsv_6                     : 10;
        MUINT32 SDBLK_XOFST               : 6;
        MUINT32 rsv_22                    : 2;
        MUINT32 SD_COEFRD_MODE            : 1;
        MUINT32 rsv_25                    : 3;
        MUINT32 SD_ULTRA_MODE             : 1;
        MUINT32 rsv_29                    : 3;
} ISP_CAM_LSC_CTL1_T;

typedef union {
    enum { MASK     = 0x113F003F };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_LSC_CTL1_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_LSC_CTL1_T;

// CAM_LSC_CTL2 CAM+0534H
typedef struct {
        MUINT32 SDBLK_WIDTH               : 12;
        MUINT32 SDBLK_XNUM                : 5;
        MUINT32 LSC_OFFLINE               : 1;
        MUINT32 rsv_18                    : 14;
} ISP_CAM_LSC_CTL2_T;

typedef union {
    enum { MASK     = 0x0003FFFF };
    enum { DEFAULT  = 0x00006000 };
    typedef ISP_CAM_LSC_CTL2_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_LSC_CTL2_T;

// CAM_LSC_CTL3 CAM+0538H
typedef struct {
        MUINT32 SDBLK_HEIGHT              : 12;
        MUINT32 SDBLK_YNUM                : 5;
        MUINT32 LSC_SPARE                 : 15;
} ISP_CAM_LSC_CTL3_T;

typedef union {
    enum { MASK     = 0xFFFFFFFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_LSC_CTL3_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_LSC_CTL3_T;

// CAM_LSC_LBLOCK CAM+053CH
typedef struct {
        MUINT32 SDBLK_lHEIGHT             : 12;
        MUINT32 rsv_12                    : 4;
        MUINT32 SDBLK_lWIDTH              : 12;
        MUINT32 rsv_28                    : 4;
} ISP_CAM_LSC_LBLOCK_T;

typedef union {
    enum { MASK     = 0x0FFF0FFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_LSC_LBLOCK_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_LSC_LBLOCK_T;

// CAM_LSC_RATIO CAM+0540H
typedef struct {
        MUINT32 RATIO11                   : 6;
        MUINT32 rsv_6                     : 2;
        MUINT32 RATIO10                   : 6;
        MUINT32 rsv_14                    : 2;
        MUINT32 RATIO01                   : 6;
        MUINT32 rsv_22                    : 2;
        MUINT32 RATIO00                   : 6;
        MUINT32 rsv_30                    : 2;
} ISP_CAM_LSC_RATIO_T;

typedef union {
    enum { MASK     = 0x3F3F3F3F };
    enum { DEFAULT  = 0x20202020 };
    typedef ISP_CAM_LSC_RATIO_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_LSC_RATIO_T;

// CAM_LSC_GAIN_TH CAM+054CH
typedef struct {
        MUINT32 SDBLK_GAIN_TH2            : 9;
        MUINT32 rsv_9                     : 1;
        MUINT32 SDBLK_GAIN_TH1            : 9;
        MUINT32 rsv_19                    : 1;
        MUINT32 SDBLK_GAIN_TH0            : 9;
        MUINT32 rsv_29                    : 3;
} ISP_CAM_LSC_GAIN_TH_T;

typedef union {
    enum { MASK     = 0x1FF7FDFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_LSC_GAIN_TH_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_LSC_GAIN_TH_T;

//
typedef union {
    enum { COUNT = 10 };
    MUINT32 set[COUNT];
    struct {
        ISP_NVRAM_LSCI_EN_T     lsci_en;
        ISP_NVRAM_LSCI_BA_T     baseaddr;
        ISP_NVRAM_LSCI_XSIZE_T  xsize;
        ISP_NVRAM_LSC_EN_T      lsc_en;
        ISP_NVRAM_LSC_CTL1_T    ctl1;
        ISP_NVRAM_LSC_CTL2_T    ctl2;
        ISP_NVRAM_LSC_CTL3_T    ctl3;
        ISP_NVRAM_LSC_LBLOCK_T  lblock;
        ISP_NVRAM_LSC_RATIO_T   ratio;
        ISP_NVRAM_LSC_GAIN_TH_T gain_th;
    };
} ISP_NVRAM_LSC_T;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Pre-gain
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CAM_PGN_SATU01 CAM+0880H
typedef struct {
        MUINT32 PGN_CH0_SATU              : 12;
        MUINT32 rsv_12                    : 4;
        MUINT32 PGN_CH1_SATU              : 12;
        MUINT32 rsv_28                    : 4;
} ISP_CAM_PGN_SATU01_T;

typedef union {
    enum { MASK     = 0x0FFF0FFF };
    enum { DEFAULT  = 0x0FFF0FFF };
    typedef ISP_CAM_PGN_SATU01_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_PGN_SATU01_T;

// CAM_PGN_SATU23 CAM+0884H
typedef struct {
        MUINT32 PGN_CH2_SATU              : 12;
        MUINT32 rsv_12                    : 4;
        MUINT32 PGN_CH3_SATU              : 12;
        MUINT32 rsv_28                    : 4;
} ISP_CAM_PGN_SATU23_T;

typedef union {
    enum { MASK     = 0x0FFF0FFF };
    enum { DEFAULT  = 0x0FFF0FFF };
    typedef ISP_CAM_PGN_SATU23_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_PGN_SATU23_T;

// CAM_PGN_GAIN01 CAM+0888H
typedef struct {
        MUINT32 PGN_CH0_GAIN              : 13;
        MUINT32 rsv_13                    : 3;
        MUINT32 PGN_CH1_GAIN              : 13;
        MUINT32 rsv_29                    : 3;
} ISP_CAM_PGN_GAIN01_T;

typedef union {
    enum { MASK     = 0x1FFF1FFF };
    enum { DEFAULT  = 0x02000200 };
    typedef ISP_CAM_PGN_GAIN01_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_PGN_GAIN01_T;

// CAM_PGN_GAIN23 CAM+088CH
typedef struct {
        MUINT32 PGN_CH2_GAIN              : 13;
        MUINT32 rsv_13                    : 3;
        MUINT32 PGN_CH3_GAIN              : 13;
        MUINT32 rsv_29                    : 3;
} ISP_CAM_PGN_GAIN23_T;

typedef union {
    enum { MASK     = 0x1FFF1FFF };
    enum { DEFAULT  = 0x02000200 };
    typedef ISP_CAM_PGN_GAIN23_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_PGN_GAIN23_T;

// CAM_PGN_OFFS01 CAM+0890H
typedef struct {
        MUINT32 PGN_CH0_OFFS              : 12;
        MUINT32 rsv_12                    : 4;
        MUINT32 PGN_CH1_OFFS              : 12;
        MUINT32 rsv_28                    : 4;
} ISP_CAM_PGN_OFFS01_T;

typedef union {
    enum { MASK     = 0x0FFF0FFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_PGN_OFFS01_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_PGN_OFFS01_T;

// CAM_PGN_OFFS23 CAM+0894H
typedef struct {
        MUINT32 PGN_CH2_OFFS              : 12;
        MUINT32 rsv_12                    : 4;
        MUINT32 PGN_CH3_OFFS              : 12;
        MUINT32 rsv_28                    : 4;
} ISP_CAM_PGN_OFFS23_T;

typedef union {
    enum { MASK     = 0x0FFF0FFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_PGN_OFFS23_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_PGN_OFFS23_T;

//
typedef union {
    enum { COUNT = 6 };
    MUINT32 set[COUNT];
    struct {
        ISP_NVRAM_PGN_SATU01_T   satu01;
        ISP_NVRAM_PGN_SATU23_T   satu23;
        ISP_NVRAM_PGN_GAIN01_T   gain01;
        ISP_NVRAM_PGN_GAIN23_T   gain23;
        ISP_NVRAM_PGN_OFFS01_T   offs01;
        ISP_NVRAM_PGN_OFFS23_T   offs23;
    };
} ISP_NVRAM_PGN_T;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CFA
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CAM_CFA_BYPASS CAM+08A0H
typedef struct {
        MUINT32 BAYER_BYPASS              : 1;
        MUINT32 BAYER_DEBUG_MODE          : 2;
        MUINT32 rsv_3                     : 29;
} ISP_CAM_CFA_BYPASS_T;

typedef union {
    enum { MASK     = 0x00000007 };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_CFA_BYPASS_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_CFA_BYPASS_T;

// CAM_CFA_ED_F CAM+08A4H
typedef struct {
        MUINT32 BAYER_FLAT_DET_MODE       : 1;
        MUINT32 BAYER_STEP_DET_MODE       : 1;
        MUINT32 rsv_2                     : 6;
        MUINT32 BAYER_FLAT_TH             : 8;
        MUINT32 rsv_16                    : 16;
} ISP_CAM_CFA_ED_F_T;

typedef union {
    enum { MASK     = 0x0000FF03 };
    enum { DEFAULT  = 0x0000FF03 };
    typedef ISP_CAM_CFA_ED_F_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_CFA_ED_F_T;

// CAM_CFA_ED_NYQ CAM+08A8H
typedef struct {
        MUINT32 BAYER_NYQ_TH              : 8;
        MUINT32 BAYER_NYQ_TH2             : 8;
        MUINT32 BAYER_NYQ_TH3             : 8;
        MUINT32 BAYER_HF_NYQ_GAIN         : 2;
        MUINT32 rsv_26                    : 6;
} ISP_CAM_CFA_ED_NYQ_T;

typedef union {
    enum { MASK     = 0x03FFFFFF };
    enum { DEFAULT  = 0x0100FFFF };
    typedef ISP_CAM_CFA_ED_NYQ_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_CFA_ED_NYQ_T;

// CAM_CFA_ED_STEP CAM+08ACH
typedef struct {
        MUINT32 BAYER_STEP_TH             : 8;
        MUINT32 BAYER_STEP2_TH            : 8;
        MUINT32 BAYER_STEP3_TH            : 8;
        MUINT32 rsv_24                    : 7;
        MUINT32 BAYER_RB_MODE             : 1;
} ISP_CAM_CFA_ED_STEP_T;

typedef union {
    enum { MASK     = 0x80FFFFFF };
    enum { DEFAULT  = 0x80FF00FF };
    typedef ISP_CAM_CFA_ED_STEP_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_CFA_ED_STEP_T;

// CAM_CFA_RGB_HF CAM+08B0H
typedef struct {
        MUINT32 BAYER_HF_CORE_G           : 5;
        MUINT32 BAYER_RB_ROUGH_F          : 5;
        MUINT32 BAYER_RB_ROUGH_D          : 5;
        MUINT32 BAYER_G_ROUGH_F           : 5;
        MUINT32 BAYER_G_ROUGH_D           : 5;
        MUINT32 BAYER_RB_MODE_F           : 2;
        MUINT32 BAYER_RB_MODE_D           : 2;
        MUINT32 BAYER_RB_MODE_HV          : 2;
        MUINT32 BAYER_SSG_MODE            : 1;
} ISP_CAM_CFA_RGB_HF_T;

typedef union {
    enum { MASK     = 0xFFFFFFFF };
    enum { DEFAULT  = 0x01084208 };
    typedef ISP_CAM_CFA_RGB_HF_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_CFA_RGB_HF_T;

// CAM_CFA_BW CAM+08B4H
typedef struct {
        MUINT32 BAYER_BD_TH               : 8;
        MUINT32 BAYER_WD_TH               : 8;
        MUINT32 rsv_16                    : 16;
} ISP_CAM_CFA_BW_T;

typedef union {
    enum { MASK     = 0x0000FFFF };
    enum { DEFAULT  = 0x0000FFFF };
    typedef ISP_CAM_CFA_BW_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_CFA_BW_T;

// CAM_CFA_F1_ACT CAM+08B8H
typedef struct {
        MUINT32 BAYER_F1_TH1              : 9;
        MUINT32 BAYER_F1_TH2              : 9;
        MUINT32 BAYER_F1_SLOPE1           : 2;
        MUINT32 BAYER_F1_SLOPE2           : 2;
        MUINT32 rsv_22                    : 10;
} ISP_CAM_CFA_F1_ACT_T;

typedef union {
    enum { MASK     = 0x003FFFFF };
    enum { DEFAULT  = 0x0003FE00 };
    typedef ISP_CAM_CFA_F1_ACT_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_CFA_F1_ACT_T;

// CAM_CFA_F2_ACT CAM+08BCH
typedef struct {
        MUINT32 BAYER_F2_TH1              : 9;
        MUINT32 BAYER_F2_TH2              : 9;
        MUINT32 BAYER_F2_SLOPE1           : 2;
        MUINT32 BAYER_F2_SLOPE2           : 2;
        MUINT32 rsv_22                    : 10;
} ISP_CAM_CFA_F2_ACT_T;

typedef union {
    enum { MASK     = 0x003FFFFF };
    enum { DEFAULT  = 0x0003FE00 };
    typedef ISP_CAM_CFA_F2_ACT_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_CFA_F2_ACT_T;

// CAM_CFA_F3_ACT CAM+08C0H
typedef struct {
        MUINT32 BAYER_F3_TH1              : 9;
        MUINT32 BAYER_F3_TH2              : 9;
        MUINT32 BAYER_F3_SLOPE1           : 2;
        MUINT32 BAYER_F3_SLOPE2           : 2;
        MUINT32 rsv_22                    : 10;
} ISP_CAM_CFA_F3_ACT_T;

typedef union {
    enum { MASK     = 0x003FFFFF };
    enum { DEFAULT  = 0x0003FE00 };
    typedef ISP_CAM_CFA_F3_ACT_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_CFA_F3_ACT_T;

// CAM_CFA_F4_ACT CAM+08C4H
typedef struct {
        MUINT32 BAYER_F4_TH1              : 9;
        MUINT32 BAYER_F4_TH2              : 9;
        MUINT32 BAYER_F4_SLOPE1           : 2;
        MUINT32 BAYER_F4_SLOPE2           : 2;
        MUINT32 rsv_22                    : 10;
} ISP_CAM_CFA_F4_ACT_T;

typedef union {
    enum { MASK     = 0x003FFFFF };
    enum { DEFAULT  = 0x0003FE00 };
    typedef ISP_CAM_CFA_F4_ACT_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_CFA_F4_ACT_T;

// CAM_CFA_F1_L CAM+08C8H
typedef struct {
        MUINT32 BAYER_F1_L_LUT_Y0         : 5;
        MUINT32 BAYER_F1_L_LUT_Y1         : 5;
        MUINT32 BAYER_F1_L_LUT_Y2         : 5;
        MUINT32 BAYER_F1_L_LUT_Y3         : 5;
        MUINT32 BAYER_F1_L_LUT_Y4         : 5;
        MUINT32 rsv_25                    : 7;
} ISP_CAM_CFA_F1_L_T;

typedef union {
    enum { MASK     = 0x01FFFFFF };
    enum { DEFAULT  = 0x01084210 };
    typedef ISP_CAM_CFA_F1_L_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_CFA_F1_L_T;

// CAM_CFA_F2_L CAM+08CCH
typedef struct {
        MUINT32 BAYER_F2_L_LUT_Y0         : 5;
        MUINT32 BAYER_F2_L_LUT_Y1         : 5;
        MUINT32 BAYER_F2_L_LUT_Y2         : 5;
        MUINT32 BAYER_F2_L_LUT_Y3         : 5;
        MUINT32 BAYER_F2_L_LUT_Y4         : 5;
        MUINT32 rsv_25                    : 7;
} ISP_CAM_CFA_F2_L_T;

typedef union {
    enum { MASK     = 0x01FFFFFF };
    enum { DEFAULT  = 0x01084210 };
    typedef ISP_CAM_CFA_F2_L_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_CFA_F2_L_T;

// CAM_CFA_F3_L CAM+08D0H
typedef struct {
        MUINT32 BAYER_F3_L_LUT_Y0         : 5;
        MUINT32 BAYER_F3_L_LUT_Y1         : 5;
        MUINT32 BAYER_F3_L_LUT_Y2         : 5;
        MUINT32 BAYER_F3_L_LUT_Y3         : 5;
        MUINT32 BAYER_F3_L_LUT_Y4         : 5;
        MUINT32 rsv_25                    : 7;
} ISP_CAM_CFA_F3_L_T;

typedef union {
    enum { MASK     = 0x01FFFFFF };
    enum { DEFAULT  = 0x01084210 };
    typedef ISP_CAM_CFA_F3_L_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_CFA_F3_L_T;

// CAM_CFA_F4_L CAM+08D4H
typedef struct {
        MUINT32 BAYER_F4_L_LUT_Y0         : 5;
        MUINT32 BAYER_F4_L_LUT_Y1         : 5;
        MUINT32 BAYER_F4_L_LUT_Y2         : 5;
        MUINT32 BAYER_F4_L_LUT_Y3         : 5;
        MUINT32 BAYER_F4_L_LUT_Y4         : 5;
        MUINT32 rsv_25                    : 7;
} ISP_CAM_CFA_F4_L_T;

typedef union {
    enum { MASK     = 0x01FFFFFF };
    enum { DEFAULT  = 0x01084210 };
    typedef ISP_CAM_CFA_F4_L_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_CFA_F4_L_T;

// CAM_CFA_HF_RB CAM+08D8H
typedef struct {
        MUINT32 BAYER_RB_DIFF_TH          : 10;
        MUINT32 BAYER_HF_CLIP             : 9;
        MUINT32 rsv_19                    : 13;
} ISP_CAM_CFA_HF_RB_T;

typedef union {
    enum { MASK     = 0x0007FFFF };
    enum { DEFAULT  = 0x000403FF };
    typedef ISP_CAM_CFA_HF_RB_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_CFA_HF_RB_T;

// CAM_CFA_HF_GAIN CAM+08DCH
typedef struct {
        MUINT32 HF_GLOBAL_GAIN            : 4;
        MUINT32 rsv_4                     : 28;
} ISP_CAM_CFA_HF_GAIN_T;

typedef union {
    enum { MASK     = 0x0000000F };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_CFA_HF_GAIN_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_CFA_HF_GAIN_T;

// CAM_CFA_HF_COMP CAM+08E0H
typedef struct {
        MUINT32 HF_LSC_GAIN0            : 4;
        MUINT32 HF_LSC_GAIN1            : 4;
        MUINT32 HF_LSC_GAIN2            : 4;
        MUINT32 HF_LSC_GAIN3            : 4;
        MUINT32 HF_UNDER_TH             : 8;
        MUINT32 HF_UNDER_ACT_TH         : 8;
} ISP_CAM_CFA_HF_COMP_T;

typedef union {
    enum { MASK     = 0xFFFFFFFF };
    enum { DEFAULT  = 0xFF048888 };
    typedef ISP_CAM_CFA_HF_COMP_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_CFA_HF_COMP_T;

// CAM_CFA_HF_CORING_TH CAM+08E4H
typedef struct {
        MUINT32 HF_CORING_TH              : 8;
        MUINT32 rsv_8                     : 24;
} ISP_CAM_CFA_HF_CORING_TH_T;

typedef union {
    enum { MASK     = 0x000000FF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_CFA_HF_CORING_TH_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_CFA_HF_CORING_TH_T;

// CAM_CFA_ACT_LUT CAM+08E8H
typedef struct {
        MUINT32 BAYER_ACT_LUT_Y0          : 5;
        MUINT32 BAYER_ACT_LUT_Y1          : 5;
        MUINT32 BAYER_ACT_LUT_Y2          : 5;
        MUINT32 BAYER_ACT_LUT_Y3          : 5;
        MUINT32 BAYER_ACT_LUT_Y4          : 5;
        MUINT32 rsv_25                    : 7;
} ISP_CAM_CFA_ACT_LUT_T;

typedef union {
    enum { MASK     = 0x01FFFFFF };
    enum { DEFAULT  = 0x00842108 };
    typedef ISP_CAM_CFA_ACT_LUT_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_CFA_ACT_LUT_T;

// CAM_CFA_SPARE CAM+08F0H
typedef struct {
        MUINT32 CFA_SPARE                 : 32;
} ISP_CAM_CFA_SPARE_T;

typedef union {
    enum { MASK     = 0xFFFFFFFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_CFA_SPARE_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_CFA_SPARE_T;

// CAM_CFA_BB CAM+08F4H
typedef struct {
        MUINT32 BAYER_BB_TH1              : 9;
        MUINT32 rsv_9                     : 3;
        MUINT32 BAYER_BB_TH2              : 9;
        MUINT32 rsv_21                    : 11;
} ISP_CAM_CFA_BB_T;

typedef union {
    enum { MASK     = 0x001FF1FF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_CFA_BB_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_CFA_BB_T;

//
typedef union {
    enum { COUNT = 21 };
    MUINT32 set[COUNT];
    struct {
        ISP_NVRAM_CFA_BYPASS_T       bypass;
        ISP_NVRAM_CFA_ED_F_T         ed_f;
        ISP_NVRAM_CFA_ED_NYQ_T       ed_nyq;
        ISP_NVRAM_CFA_ED_STEP_T      ed_step;
        ISP_NVRAM_CFA_RGB_HF_T       rgb_hf;
        ISP_NVRAM_CFA_BW_T           bw;
        ISP_NVRAM_CFA_F1_ACT_T       f1_act;
        ISP_NVRAM_CFA_F2_ACT_T       f2_act;
        ISP_NVRAM_CFA_F3_ACT_T       f3_act;
        ISP_NVRAM_CFA_F4_ACT_T       f4_act;
        ISP_NVRAM_CFA_F1_L_T         f1_l;
        ISP_NVRAM_CFA_F2_L_T         f2_l;
        ISP_NVRAM_CFA_F3_L_T         f3_l;
        ISP_NVRAM_CFA_F4_L_T         f4_l;
        ISP_NVRAM_CFA_HF_RB_T        hf_rb;
        ISP_NVRAM_CFA_HF_GAIN_T      hf_gain;
        ISP_NVRAM_CFA_HF_COMP_T      hf_comp;
        ISP_NVRAM_CFA_HF_CORING_TH_T hf_coring_th;
        ISP_NVRAM_CFA_ACT_LUT_T      act_lut;
        ISP_NVRAM_CFA_SPARE_T        spare;
        ISP_NVRAM_CFA_BB_T           bb;
    };
} ISP_NVRAM_CFA_T;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// EE
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CAM_SEEE_SRK_CTRL CAM+AA0H
typedef struct {
        MUINT32 USM_OVER_SHRINK_EN        : 1;
        MUINT32 rsv_1                     : 27;
        MUINT32 RESERVED                  : 4;
} ISP_CAM_SEEE_SRK_CTRL_T;

typedef union {
    enum { MASK     = 0x00000001 };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_SEEE_SRK_CTRL_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_SEEE_SRK_CTRL_T;

// CAM_SEEE_CLIP_CTRL CAM+AA4H
typedef struct {
        MUINT32 USM_OVER_CLIP_EN          : 1;
        MUINT32 USM_CLIP_OVER             : 7;
        MUINT32 USM_CLIP_UNDER            : 7;
        MUINT32 rsv_15                    : 1;
        MUINT32 USM_CLIP                  : 8;
        MUINT32 rsv_24                    : 8;
} ISP_CAM_SEEE_CLIP_CTRL_T;

typedef union {
    enum { MASK     = 0x00FF7FFF };
    enum { DEFAULT  = 0x00401429 };
    typedef ISP_CAM_SEEE_CLIP_CTRL_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_SEEE_CLIP_CTRL_T;

// CAM_SEEE_HP_CTRL1 CAM+AA8H
typedef struct {
        MUINT32 USM_HP_TH                 : 8;
        MUINT32 USM_HP_AMP                : 3;
        MUINT32 rsv_11                    : 21;
} ISP_CAM_SEEE_HP_CTRL1_T;

typedef union {
    enum { MASK     = 0x000007FF };
    enum { DEFAULT  = 0x00000426 };
    typedef ISP_CAM_SEEE_HP_CTRL1_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_SEEE_HP_CTRL1_T;

// CAM_SEEE_HP_CTRL2 CAM+AACH
typedef struct {
        MUINT32 USM_HP_A                  : 8;
        MUINT32 USM_HP_B                  : 6;
        MUINT32 USM_HP_C                  : 5;
        MUINT32 USM_HP_D                  : 4;
        MUINT32 USM_HP_E                  : 4;
        MUINT32 USM_HP_F                  : 3;
        MUINT32 rsv_30                    : 2;
} ISP_CAM_SEEE_HP_CTRL2_T;

typedef union {
    enum { MASK     = 0x3FFFFFFF };
    enum { DEFAULT  = 0x35CE7C78 };
    typedef ISP_CAM_SEEE_HP_CTRL2_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_SEEE_HP_CTRL2_T;

// CAM_SEEE_ED_CTRL1 CAM+AB0H
typedef struct {
        MUINT32 USM_ED_X1                 : 8;
        MUINT32 USM_ED_S1                 : 8;
        MUINT32 USM_ED_Y1                 : 10;
        MUINT32 rsv_26                    : 6;
} ISP_CAM_SEEE_ED_CTRL1_T;

typedef union {
    enum { MASK     = 0x03FFFFFF };
    enum { DEFAULT  = 0x00140123 };
    typedef ISP_CAM_SEEE_ED_CTRL1_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_SEEE_ED_CTRL1_T;

// CAM_SEEE_ED_CTRL2 CAM+AB4H
typedef struct {
        MUINT32 USM_ED_X2                 : 8;
        MUINT32 USM_ED_S2                 : 8;
        MUINT32 USM_ED_Y2                 : 10;
        MUINT32 rsv_26                    : 6;
} ISP_CAM_SEEE_ED_CTRL2_T;

typedef union {
    enum { MASK     = 0x03FFFFFF };
    enum { DEFAULT  = 0x0266183C };
    typedef ISP_CAM_SEEE_ED_CTRL2_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_SEEE_ED_CTRL2_T;

// CAM_SEEE_ED_CTRL3 CAM+AB8H
typedef struct {
        MUINT32 USM_ED_X3                 : 8;
        MUINT32 USM_ED_S3                 : 8;
        MUINT32 USM_ED_Y3                 : 10;
        MUINT32 rsv_26                    : 6;
} ISP_CAM_SEEE_ED_CTRL3_T;

typedef union {
    enum { MASK     = 0x03FFFFFF };
    enum { DEFAULT  = 0x0133F864 };
    typedef ISP_CAM_SEEE_ED_CTRL3_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_SEEE_ED_CTRL3_T;

// CAM_SEEE_ED_CTRL4 CAM+ABCH
typedef struct {
        MUINT32 USM_ED_X4                 : 8;
        MUINT32 USM_ED_S4                 : 8;
        MUINT32 USM_ED_Y4                 : 10;
        MUINT32 rsv_26                    : 6;
} ISP_CAM_SEEE_ED_CTRL4_T;

typedef union {
    enum { MASK     = 0x03FFFFFF };
    enum { DEFAULT  = 0x0029FEDC };
    typedef ISP_CAM_SEEE_ED_CTRL4_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_SEEE_ED_CTRL4_T;

// CAM_SEEE_ED_CTRL5 CAM+AC0H
typedef struct {
        MUINT32 rsv_0                     : 8;
        MUINT32 USM_ED_S5                 : 8;
        MUINT32 rsv_16                    : 16;
} ISP_CAM_SEEE_ED_CTRL5_T;

typedef union {
    enum { MASK     = 0x0000FF00 };
    enum { DEFAULT  = 0x0000FF00 };
    typedef ISP_CAM_SEEE_ED_CTRL5_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_SEEE_ED_CTRL5_T;

// CAM_SEEE_ED_CTRL6 CAM+AC4H
typedef struct {
        MUINT32 USM_ED_TH_OVER            : 8;
        MUINT32 USM_ED_TH_UNDER           : 8;
        MUINT32 USM_ED_TH_MIN             : 8;
        MUINT32 rsv_24                    : 8;
} ISP_CAM_SEEE_ED_CTRL6_T;

typedef union {
    enum { MASK     = 0x00FFFFFF };
    enum { DEFAULT  = 0x0000FFFF };
    typedef ISP_CAM_SEEE_ED_CTRL6_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_SEEE_ED_CTRL6_T;

// CAM_SEEE_ED_CTRL7 CAM+AC8H
typedef struct {
        MUINT32 USM_ED_DIAG_AMP           : 3;
        MUINT32 USM_ED_AMP                : 6;
        MUINT32 USM_ED_LV                 : 3;
        MUINT32 USM_ED_FIL_HP_EN          : 1;
        MUINT32 EE_ED_FIL2_EN             : 1;
        MUINT32 rsv_14                    : 18;
} ISP_CAM_SEEE_ED_CTRL7_T;

typedef union {
    enum { MASK     = 0x00003FFF };
    enum { DEFAULT  = 0x00001643 };
    typedef ISP_CAM_SEEE_ED_CTRL7_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_SEEE_ED_CTRL7_T;

// CAM_SEEE_EE_LINK1 CAM+AECH
typedef struct {
        MUINT32 EE_LCE_X1_1               : 8;
        MUINT32 EE_LCE_S1_1               : 8;
        MUINT32 EE_LCE_S2_1               : 8;
        MUINT32 EE_LCE_LINK               : 1;
        MUINT32 rsv_25                    : 7;
} ISP_CAM_SEEE_EE_LINK1_T;

typedef union {
    enum { MASK     = 0x01FFFFFF };
    enum { DEFAULT  = 0x00180123 };
    typedef ISP_CAM_SEEE_EE_LINK1_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_SEEE_EE_LINK1_T;

// CAM_SEEE_EE_LINK2 CAM+AF0H
typedef struct {
        MUINT32 EE_LCE_X1_2               : 8;
        MUINT32 EE_LCE_S1_2               : 8;
        MUINT32 EE_LCE_S2_2               : 8;
        MUINT32 rsv_24                    : 8;
} ISP_CAM_SEEE_EE_LINK2_T;

typedef union {
    enum { MASK     = 0x00FFFFFF };
    enum { DEFAULT  = 0x00180123 };
    typedef ISP_CAM_SEEE_EE_LINK2_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_SEEE_EE_LINK2_T;

// CAM_SEEE_EE_LINK3 CAM+AF4H
typedef struct {
        MUINT32 EE_LCE_X1_3               : 8;
        MUINT32 EE_LCE_S1_3               : 8;
        MUINT32 EE_LCE_S2_3               : 8;
        MUINT32 rsv_24                    : 8;
} ISP_CAM_SEEE_EE_LINK3_T;

typedef union {
    enum { MASK     = 0x00FFFFFF };
    enum { DEFAULT  = 0x00180123 };
    typedef ISP_CAM_SEEE_EE_LINK3_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_SEEE_EE_LINK3_T;

// CAM_SEEE_EE_LINK4 CAM+AF8H
typedef struct {
        MUINT32 EE_LCE_THO_1              : 8;
        MUINT32 EE_LCE_THU_1              : 8;
        MUINT32 EE_LCE_THO_2              : 8;
        MUINT32 EE_LCE_THU_2              : 8;
} ISP_CAM_SEEE_EE_LINK4_T;

typedef union {
    enum { MASK     = 0xFFFFFFFF };
    enum { DEFAULT  = 0xFFFFFFFF };
    typedef ISP_CAM_SEEE_EE_LINK4_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_SEEE_EE_LINK4_T;

// CAM_SEEE_EE_LINK5 CAM+AFCH
typedef struct {
        MUINT32 EE_LCE_THO_3              : 8;
        MUINT32 EE_LCE_THU_3              : 8;
        MUINT32 rsv_16                    : 16;
} ISP_CAM_SEEE_EE_LINK5_T;

typedef union {
    enum { MASK     = 0x0000FFFF };
    enum { DEFAULT  = 0x0000FFFF };
    typedef ISP_CAM_SEEE_EE_LINK5_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_SEEE_EE_LINK5_T;

//
typedef union{
    enum { COUNT = 16 };
    MUINT32 set[COUNT];
    struct {
        ISP_NVRAM_SEEE_SRK_CTRL_T  srk_ctrl;
        ISP_NVRAM_SEEE_CLIP_CTRL_T clip_ctrl;
        ISP_NVRAM_SEEE_HP_CTRL1_T  hp_ctrl1;
        ISP_NVRAM_SEEE_HP_CTRL2_T  hp_ctrl2;
        ISP_NVRAM_SEEE_ED_CTRL1_T  ed_ctrl1;
        ISP_NVRAM_SEEE_ED_CTRL2_T  ed_ctrl2;
        ISP_NVRAM_SEEE_ED_CTRL3_T  ed_ctrl3;
        ISP_NVRAM_SEEE_ED_CTRL4_T  ed_ctrl4;
        ISP_NVRAM_SEEE_ED_CTRL5_T  ed_ctrl5;
        ISP_NVRAM_SEEE_ED_CTRL6_T  ed_ctrl6;
        ISP_NVRAM_SEEE_ED_CTRL7_T  ed_ctrl7;
        ISP_NVRAM_SEEE_EE_LINK1_T  ee_link1;
        ISP_NVRAM_SEEE_EE_LINK2_T  ee_link2;
        ISP_NVRAM_SEEE_EE_LINK3_T  ee_link3;
        ISP_NVRAM_SEEE_EE_LINK4_T  ee_link4;
        ISP_NVRAM_SEEE_EE_LINK5_T  ee_link5;
    };
} ISP_NVRAM_EE_T;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// LCE
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CAM_LCE_QUA CAM+09C8H
typedef struct {
        MUINT32 LCE_PA                    : 7;
        MUINT32 LCE_PB                    : 9;
        MUINT32 LCE_BA                    : 8;
        MUINT32 rsv_24                    : 8;
} ISP_CAM_LCE_QUA_T;

typedef union {
    enum { MASK     = 0x00FFFFFF };
    enum { DEFAULT  = 0x00000040 };
    typedef ISP_CAM_LCE_QUA_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_LCE_QUA_T;

// CAM_LCE_THR CAM+09CCH
typedef struct {
        MUINT32 LCE_ANR_THR1              : 6;
        MUINT32 rsv_6                     : 2;
        MUINT32 LCE_ANR_THR2              : 6;
        MUINT32 rsv_14                    : 2;
        MUINT32 LCE_ANR_THR3              : 6;
        MUINT32 rsv_22                    : 10;
} ISP_CAM_LCE_THR_T;

typedef union {
    enum { MASK     = 0x003F3F3F };
    enum { DEFAULT  = 0x00100C08 };
    typedef ISP_CAM_LCE_THR_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_LCE_THR_T;


//
typedef union {
    enum { COUNT = 2 };
    MUINT32 set[COUNT];
    struct {
        ISP_NVRAM_LCE_QUA_T qua;
        ISP_NVRAM_LCE_THR_T thr;
    };
} ISP_NVRAM_LCE_T;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// GGM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
enum { GGM_LUT_SIZE = 144 };

// CAM_GGM_CTRL CAM+1600H
typedef struct {
        MUINT32 GAMMA_EN                  : 1;
        MUINT32 rsv_1                     : 31;
} ISP_CAM_GGM_CTRL_T;

typedef union {
    enum { MASK     = 0x00000001 };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_GGM_CTRL_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_GGM_CTRL_T;

// CAM_GGM_RB_GMT CAM+1000H
typedef struct {
        MUINT32 R_GAMMA                   : 16;
        MUINT32 B_GAMMA                   : 16;
} ISP_CAM_GGM_RB_GMT_T;

// CAM_GGM_G_GMT CAM+1300H
typedef struct {
        MUINT32 G_GAMMA                   : 16;
        MUINT32 rsv_16                    : 16;
} ISP_CAM_GGM_G_GMT_T;

//
typedef union {
    enum { COUNT = 1 };
    MUINT32 set[COUNT];
    struct {
        ISP_NVRAM_GGM_CTRL_T ctrl;
    };
} ISP_NVRAM_GGM_CON_T;

//
typedef union {
    enum { COUNT = GGM_LUT_SIZE };
    MUINT32 set[COUNT];
    struct {
        ISP_CAM_GGM_RB_GMT_T  lut[GGM_LUT_SIZE];
    };
} ISP_NVRAM_GGM_RB_GMT_T;

//
typedef union {
    enum { COUNT = GGM_LUT_SIZE };
    MUINT32 set[COUNT];
    struct {
        ISP_CAM_GGM_G_GMT_T   lut[GGM_LUT_SIZE];
    };
} ISP_NVRAM_GGM_G_GMT_T;

//
typedef struct {
    ISP_NVRAM_GGM_RB_GMT_T  rb_gmt;
    ISP_NVRAM_GGM_G_GMT_T   g_gmt;
} ISP_NVRAM_GGM_T;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// G2G (CCM)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CAM_G2G_CONV0A CAM+0920H
typedef struct {
        MUINT32 G2G_CNV_00                : 11;
        MUINT32 rsv_11                    : 5;
        MUINT32 G2G_CNV_01                : 11;
        MUINT32 rsv_27                    : 5;
} ISP_CAM_G2G_CONV0A_T;

typedef union {
    enum { MASK     = 0x07FF07FF };
    enum { DEFAULT  = 0x00000200 };
    typedef ISP_CAM_G2G_CONV0A_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_G2G_CONV0A_T;

// CAM_G2G_CONV0B CAM+0924H
typedef struct {
        MUINT32 G2G_CNV_02                : 11;
        MUINT32 rsv_11                    : 21;
} ISP_CAM_G2G_CONV0B_T;

typedef union {
    enum { MASK     = 0x000007FF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_G2G_CONV0B_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_G2G_CONV0B_T;

// CAM_G2G_CONV1A CAM+0928H
typedef struct {
        MUINT32 G2G_CNV_10                : 11;
        MUINT32 rsv_11                    : 5;
        MUINT32 G2G_CNV_11                : 11;
        MUINT32 rsv_27                    : 5;
} ISP_CAM_G2G_CONV1A_T;

typedef union {
    enum { MASK     = 0x07FF07FF };
    enum { DEFAULT  = 0x02000000 };
    typedef ISP_CAM_G2G_CONV1A_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_G2G_CONV1A_T;

// CAM_G2G_CONV1B CAM+092CH
typedef struct {
        MUINT32 G2G_CNV_12                : 11;
        MUINT32 rsv_11                    : 21;
} ISP_CAM_G2G_CONV1B_T;

typedef union {
    enum { MASK     = 0x000007FF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_G2G_CONV1B_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_G2G_CONV1B_T;

// CAM_G2G_CONV2A CAM+0930H
typedef struct {
        MUINT32 G2G_CNV_20                : 11;
        MUINT32 rsv_11                    : 5;
        MUINT32 G2G_CNV_21                : 11;
        MUINT32 rsv_27                    : 5;
} ISP_CAM_G2G_CONV2A_T;

typedef union {
    enum { MASK     = 0x07FF07FF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_G2G_CONV2A_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_G2G_CONV2A_T;

// CAM_G2G_CONV2B CAM+0934H
typedef struct {
        MUINT32 G2G_CNV_22                : 11;
        MUINT32 rsv_11                    : 21;
} ISP_CAM_G2G_CONV2B_T;

typedef union {
    enum { MASK     = 0x000007FF };
    enum { DEFAULT  = 0x00000200 };
    typedef ISP_CAM_G2G_CONV2B_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_G2G_CONV2B_T;

// CAM_G2G_ACC CAM+0938H
typedef struct {
        MUINT32 G2G_ACC                   : 4;
        MUINT32 G2G_MOFFS_R               : 1;
        MUINT32 G2G_POFFS_R               : 1;
        MUINT32 rsv_6                     : 26;
} ISP_CAM_G2G_ACC_T;

typedef union {
    enum { MASK     = 0x0000002F };
    enum { DEFAULT  = 0x00000009 };
    typedef ISP_CAM_G2G_ACC_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_G2G_ACC_T;

//
typedef union {
    enum { COUNT = 6 };
    MUINT32 set[COUNT];
    struct {
        ISP_NVRAM_G2G_CONV0A_T conv0a;
        ISP_NVRAM_G2G_CONV0B_T conv0b;
        ISP_NVRAM_G2G_CONV1A_T conv1a;
        ISP_NVRAM_G2G_CONV1B_T conv1b;
        ISP_NVRAM_G2G_CONV2A_T conv2a;
        ISP_NVRAM_G2G_CONV2B_T conv2b;
    };
} ISP_NVRAM_CCM_T;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// G2C
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CAM_G2C_CONV_0A CAM+A00H
typedef struct {
        MUINT32 G2C_CNV00                 : 11;
        MUINT32 rsv_11                    : 5;
        MUINT32 G2C_CNV01                 : 11;
        MUINT32 rsv_27                    : 5;
} ISP_CAM_G2C_CONV_0A_T;

typedef union {
    enum { MASK     = 0x07FF07FF };
    enum { DEFAULT  = 0x012D0099 };
    typedef ISP_CAM_G2C_CONV_0A_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_G2C_CONV_0A_T;

// CAM_G2C_CONV_0B CAM+A04H
typedef struct {
        MUINT32 G2C_CNV02                 : 11;
        MUINT32 rsv_11                    : 5;
        MUINT32 G2C_YOFFSET11             : 11;
        MUINT32 rsv_27                    : 5;
} ISP_CAM_G2C_CONV_0B_T;

typedef union {
    enum { MASK     = 0x07FF07FF };
    enum { DEFAULT  = 0x0000003A };
    typedef ISP_CAM_G2C_CONV_0B_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_G2C_CONV_0B_T;

// CAM_G2C_CONV_1A CAM+A08H
typedef struct {
        MUINT32 G2C_CNV10                 : 11;
        MUINT32 rsv_11                    : 5;
        MUINT32 G2C_CNV11                 : 11;
        MUINT32 rsv_27                    : 5;
} ISP_CAM_G2C_CONV_1A_T;

typedef union {
    enum { MASK     = 0x07FF07FF };
    enum { DEFAULT  = 0x075607AA };
    typedef ISP_CAM_G2C_CONV_1A_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_G2C_CONV_1A_T;

// CAM_G2C_CONV_1B CAM+A0CH
typedef struct {
        MUINT32 G2C_CNV12                 : 11;
        MUINT32 rsv_11                    : 5;
        MUINT32 G2C_UOFFSET10             : 10;
        MUINT32 rsv_26                    : 6;
} ISP_CAM_G2C_CONV_1B_T;

typedef union {
    enum { MASK     = 0x03FF07FF };
    enum { DEFAULT  = 0x00000100 };
    typedef ISP_CAM_G2C_CONV_1B_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_G2C_CONV_1B_T;

// CAM_G2C_CONV_2A CAM+A10H
typedef struct {
        MUINT32 G2C_CNV20                 : 11;
        MUINT32 rsv_11                    : 5;
        MUINT32 G2C_CNV21                 : 11;
        MUINT32 rsv_27                    : 5;
} ISP_CAM_G2C_CONV_2A_T;

typedef union {
    enum { MASK     = 0x07FF07FF };
    enum { DEFAULT  = 0x072A0100 };
    typedef ISP_CAM_G2C_CONV_2A_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_G2C_CONV_2A_T;

// CAM_G2C_CONV_2B CAM+A14H
typedef struct {
        MUINT32 G2C_CNV22                 : 11;
        MUINT32 rsv_11                    : 5;
        MUINT32 G2C_VOFFSET10             : 10;
        MUINT32 rsv_26                    : 6;
} ISP_CAM_G2C_CONV_2B_T;

typedef union {
    enum { MASK     = 0x03FF07FF };
    enum { DEFAULT  = 0x000007D6 };
    typedef ISP_CAM_G2C_CONV_2B_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_G2C_CONV_2B_T;

//
typedef union {
    enum { COUNT = 6 };
    MUINT32 set[COUNT];
    struct {
        ISP_NVRAM_G2C_CONV_0A_T conv_0a;
        ISP_NVRAM_G2C_CONV_0B_T conv_0b;
        ISP_NVRAM_G2C_CONV_1A_T conv_1a;
        ISP_NVRAM_G2C_CONV_1B_T conv_1b;
        ISP_NVRAM_G2C_CONV_2A_T conv_2a;
        ISP_NVRAM_G2C_CONV_2B_T conv_2b;
    };
} ISP_NVRAM_G2C_T;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ANR
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CAM_ANR_CON1 CAM+A20H
typedef struct {
        MUINT32 ANR_ENC                   : 1;
        MUINT32 ANR_ENY                   : 1;
        MUINT32 rsv_2                     : 2;
        MUINT32 ANR_SCALE_MODE            : 2;
        MUINT32 rsv_6                     : 2;
        MUINT32 ANR_FLT_MODE              : 3;
        MUINT32 rsv_11                    : 1;
        MUINT32 ANR_MODE                  : 1;
        MUINT32 rsv_13                    : 3;
        MUINT32 ANR_Y_LUMA_SCALE          : 4;
        MUINT32 rsv_20                    : 4;
        MUINT32 ANR_LCE_LINK              : 1;
        MUINT32 rsv_25                    : 7;
} ISP_CAM_ANR_CON1_T;

typedef union {
    enum { MASK     = 0x010F1733 };
    enum { DEFAULT  = 0x01081200 };
    typedef ISP_CAM_ANR_CON1_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_ANR_CON1_T;

// CAM_ANR_CON2 CAM+A24H
typedef struct {
        MUINT32 ANR_GNY                   : 6;
        MUINT32 rsv_6                     : 2;
        MUINT32 ANR_GNC                   : 5;
        MUINT32 rsv_13                    : 19;
} ISP_CAM_ANR_CON2_T;

typedef union {
    enum { MASK     = 0x00001F3F };
    enum { DEFAULT  = 0x00001012 };
    typedef ISP_CAM_ANR_CON2_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_ANR_CON2_T;

// CAM_ANR_CON3 CAM+A28H
typedef struct {
        MUINT32 ANR_IMPL_MODE             : 2;
        MUINT32 rsv_2                     : 2;
        MUINT32 ANR_C_MEDIAN_EN           : 2;
        MUINT32 rsv_6                     : 2;
        MUINT32 ANR_C_SM_EDGE             : 1;
        MUINT32 rsv_9                     : 3;
        MUINT32 ANR_QEC                   : 1;
        MUINT32 rsv_13                    : 3;
        MUINT32 ANR_QEC_VAL               : 8;
        MUINT32 rsv_24                    : 8;
} ISP_CAM_ANR_CON3_T;

typedef union {
    enum { MASK     = 0x00FF1133 };
    enum { DEFAULT  = 0x00020001 };
    typedef ISP_CAM_ANR_CON3_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_ANR_CON3_T;

// CAM_ANR_YAD1 CAM+A2CH
typedef struct {
        MUINT32 ANR_CEN_GAIN_LO_TH        : 5;
        MUINT32 rsv_5                     : 3;
        MUINT32 ANR_CEN_GAIN_HI_TH        : 5;
        MUINT32 rsv_13                    : 3;
        MUINT32 ANR_K_LO_TH               : 4;
        MUINT32 rsv_20                    : 4;
        MUINT32 ANR_K_HI_TH               : 4;
        MUINT32 rsv_28                    : 4;
} ISP_CAM_ANR_YAD1_T;

typedef union {
    enum { MASK     = 0x0F0F1F1F };
    enum { DEFAULT  = 0x09020101 };
    typedef ISP_CAM_ANR_YAD1_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_ANR_YAD1_T;

// CAM_ANR_YAD2 CAM+A30H
typedef struct {
        MUINT32 ANR_PTY_VGAIN             : 4;
        MUINT32 rsv_4                     : 4;
        MUINT32 ANR_PTY_GAIN_TH           : 5;
        MUINT32 rsv_13                    : 19;
} ISP_CAM_ANR_YAD2_T;

typedef union {
    enum { MASK     = 0x00001F0F };
    enum { DEFAULT  = 0x00000206 };
    typedef ISP_CAM_ANR_YAD2_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_ANR_YAD2_T;

// CAM_ANR_4LUT1 CAM+A34H
typedef struct {
        MUINT32 ANR_Y_CPX1                : 8;
        MUINT32 ANR_Y_CPX2                : 8;
        MUINT32 ANR_Y_CPX3                : 8;
        MUINT32 rsv_24                    : 8;
} ISP_CAM_ANR_4LUT1_T;

typedef union {
    enum { MASK     = 0x00FFFFFF };
    enum { DEFAULT  = 0x00A06428 };
    typedef ISP_CAM_ANR_4LUT1_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_ANR_4LUT1_T;

// CAM_ANR_4LUT2 CAM+A38H
typedef struct {
        MUINT32 ANR_Y_SCALE_CPY0          : 5;
        MUINT32 rsv_5                     : 3;
        MUINT32 ANR_Y_SCALE_CPY1          : 5;
        MUINT32 rsv_13                    : 3;
        MUINT32 ANR_Y_SCALE_CPY2          : 5;
        MUINT32 rsv_21                    : 3;
        MUINT32 ANR_Y_SCALE_CPY3          : 5;
        MUINT32 rsv_29                    : 3;
} ISP_CAM_ANR_4LUT2_T;

typedef union {
    enum { MASK     = 0x1F1F1F1F };
    enum { DEFAULT  = 0x080C1010 };
    typedef ISP_CAM_ANR_4LUT2_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_ANR_4LUT2_T;

// CAM_ANR_4LUT3 CAM+A3CH
typedef struct {
        MUINT32 ANR_Y_SCALE_SP0           : 5;
        MUINT32 rsv_5                     : 3;
        MUINT32 ANR_Y_SCALE_SP1           : 5;
        MUINT32 rsv_13                    : 3;
        MUINT32 ANR_Y_SCALE_SP2           : 5;
        MUINT32 rsv_21                    : 3;
        MUINT32 ANR_Y_SCALE_SP3           : 5;
        MUINT32 rsv_29                    : 3;
} ISP_CAM_ANR_4LUT3_T;

typedef union {
    enum { MASK     = 0x1F1F1F1F };
    enum { DEFAULT  = 0x1F1E1E00 };
    typedef ISP_CAM_ANR_4LUT3_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_ANR_4LUT3_T;

// CAM_ANR_PTY CAM+A40H
typedef struct {
        MUINT32 ANR_PTY1                  : 8;
        MUINT32 ANR_PTY2                  : 8;
        MUINT32 ANR_PTY3                  : 8;
        MUINT32 ANR_PTY4                  : 8;
} ISP_CAM_ANR_PTY_T;

typedef union {
    enum { MASK     = 0xFFFFFFFF };
    enum { DEFAULT  = 0x0C080402 };
    typedef ISP_CAM_ANR_PTY_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_ANR_PTY_T;

// CAM_ANR_CAD CAM+A44H
typedef struct {
        MUINT32 ANR_PTC_VGAIN             : 4;
        MUINT32 rsv_4                     : 4;
        MUINT32 ANR_PTC_GAIN_TH           : 5;
        MUINT32 rsv_13                    : 3;
        MUINT32 ANR_C_L_DIFF_TH           : 8;
        MUINT32 rsv_24                    : 8;
} ISP_CAM_ANR_CAD_T;

typedef union {
    enum { MASK     = 0x00FF1F0F };
    enum { DEFAULT  = 0x00460406 };
    typedef ISP_CAM_ANR_CAD_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_ANR_CAD_T;

// CAM_ANR_PTC CAM+A48H
typedef struct {
        MUINT32 ANR_PTC1                  : 8;
        MUINT32 ANR_PTC2                  : 8;
        MUINT32 ANR_PTC3                  : 8;
        MUINT32 ANR_PTC4                  : 8;
} ISP_CAM_ANR_PTC_T;

typedef union {
    enum { MASK     = 0xFFFFFFFF };
    enum { DEFAULT  = 0x06040302 };
    typedef ISP_CAM_ANR_PTC_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_ANR_PTC_T;

// CAM_ANR_LCE1 CAM+A4CH
typedef struct {
        MUINT32 ANR_LCE_C_GAIN            : 4;
        MUINT32 ANR_LCE_SCALE_GAIN        : 3;
        MUINT32 rsv_7                     : 25;
} ISP_CAM_ANR_LCE1_T;

typedef union {
    enum { MASK     = 0x0000007F };
    enum { DEFAULT  = 0x00000026 };
    typedef ISP_CAM_ANR_LCE1_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_ANR_LCE1_T;

// CAM_ANR_LCE2 CAM+A50H
typedef struct {
        MUINT32 ANR_LCE_GAIN0             : 6;
        MUINT32 rsv_6                     : 2;
        MUINT32 ANR_LCE_GAIN1             : 6;
        MUINT32 rsv_14                    : 2;
        MUINT32 ANR_LCE_GAIN2             : 6;
        MUINT32 rsv_22                    : 2;
        MUINT32 ANR_LCE_GAIN3             : 6;
        MUINT32 rsv_30                    : 2;
} ISP_CAM_ANR_LCE2_T;

typedef union {
    enum { MASK     = 0x3F3F3F3F };
    enum { DEFAULT  = 0x100C0A08 };
    typedef ISP_CAM_ANR_LCE2_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_ANR_LCE2_T;

// CAM_ANR_HP1 CAM+A54H
typedef struct {
        MUINT32 ANR_HP_A                  : 8;
        MUINT32 ANR_HP_B                  : 6;
        MUINT32 rsv_14                    : 2;
        MUINT32 ANR_HP_C                  : 5;
        MUINT32 rsv_21                    : 3;
        MUINT32 ANR_HP_D                  : 4;
        MUINT32 ANR_HP_E                  : 4;
} ISP_CAM_ANR_HP1_T;

typedef union {
    enum { MASK     = 0xFF1F3FFF };
    enum { DEFAULT  = 0xB9192C78 };
    typedef ISP_CAM_ANR_HP1_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_ANR_HP1_T;

// CAM_ANR_HP2 CAM+A58H
typedef struct {
        MUINT32 ANR_HP_S1                 : 4;
        MUINT32 ANR_HP_S2                 : 4;
        MUINT32 ANR_HP_X1                 : 7;
        MUINT32 rsv_15                    : 1;
        MUINT32 ANR_HP_F                  : 3;
        MUINT32 rsv_19                    : 13;
} ISP_CAM_ANR_HP2_T;

typedef union {
    enum { MASK     = 0x00077FFF };
    enum { DEFAULT  = 0x00060A32 };
    typedef ISP_CAM_ANR_HP2_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_ANR_HP2_T;

// CAM_ANR_HP3 CAM+A5CH
typedef struct {
        MUINT32 ANR_HP_Y_GAIN_CLIP        : 7;
        MUINT32 rsv_7                     : 1;
        MUINT32 ANR_HP_Y_SP               : 5;
        MUINT32 rsv_13                    : 3;
        MUINT32 ANR_HP_Y_LO               : 8;
        MUINT32 ANR_HP_CLIP               : 8;
} ISP_CAM_ANR_HP3_T;

typedef union {
    enum { MASK     = 0xFFFF1F7F };
    enum { DEFAULT  = 0x80780678 };
    typedef ISP_CAM_ANR_HP3_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_ANR_HP3_T;

// CAM_ANR_ACTY CAM+A60H
typedef struct {
        MUINT32 ANR_ACT_TH_Y              : 8;
        MUINT32 ANR_ACT_BLD_BASE_Y        : 7;
        MUINT32 rsv_15                    : 1;
        MUINT32 ANR_ACT_SLANT_Y           : 5;
        MUINT32 rsv_21                    : 3;
        MUINT32 ANR_ACT_BLD_TH_Y          : 7;
        MUINT32 rsv_31                    : 1;
} ISP_CAM_ANR_ACTY_T;

typedef union {
    enum { MASK     = 0x7F1F7FFF };
    enum { DEFAULT  = 0x30081000 };
    typedef ISP_CAM_ANR_ACTY_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_ANR_ACTY_T;

// CAM_ANR_ACTC CAM+A64H
typedef struct {
        MUINT32 ANR_ACT_TH_C              : 8;
        MUINT32 ANR_ACT_BLD_BASE_C        : 7;
        MUINT32 rsv_15                    : 1;
        MUINT32 ANR_ACT_SLANT_C           : 5;
        MUINT32 rsv_21                    : 3;
        MUINT32 ANR_ACT_BLD_TH_C          : 7;
        MUINT32 rsv_31                    : 1;
} ISP_CAM_ANR_ACTC_T;

typedef union {
    enum { MASK     = 0x7F1F7FFF };
    enum { DEFAULT  = 0x30081000 };
    typedef ISP_CAM_ANR_ACTC_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_ANR_ACTC_T;

//
typedef union {
    enum { COUNT = 18 };
    MUINT32 set[COUNT];
    struct {
        ISP_NVRAM_ANR_CON1_T  con1;
        ISP_NVRAM_ANR_CON2_T  con2;
        ISP_NVRAM_ANR_CON3_T  con3;
        ISP_NVRAM_ANR_YAD1_T  yad1;
        ISP_NVRAM_ANR_YAD2_T  yad2;
        ISP_NVRAM_ANR_4LUT1_T lut1;
        ISP_NVRAM_ANR_4LUT2_T lut2;
        ISP_NVRAM_ANR_4LUT3_T lut3;
        ISP_NVRAM_ANR_PTY_T   pty;
        ISP_NVRAM_ANR_CAD_T   cad;
        ISP_NVRAM_ANR_PTC_T   ptc;
        ISP_NVRAM_ANR_LCE1_T  lce1;
        ISP_NVRAM_ANR_LCE2_T  lce2;
        ISP_NVRAM_ANR_HP1_T   hp1;
        ISP_NVRAM_ANR_HP2_T   hp2;
        ISP_NVRAM_ANR_HP3_T   hp3;
        ISP_NVRAM_ANR_ACTY_T  acty;
        ISP_NVRAM_ANR_ACTC_T  actc;
    };
} ISP_NVRAM_ANR_T;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CCR
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CAM_CCR_CON CAM+A90H
typedef struct {
        MUINT32 CCR_EN                    : 1;
        MUINT32 rsv_1                     : 7;
        MUINT32 CCR_UV_GAIN_MODE          : 1;
        MUINT32 rsv_9                     : 15;
        MUINT32 CCR_Y_CPX3                : 8;
} ISP_CAM_CCR_CON_T;

typedef union {
    enum { MASK     = 0xFF000101 };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_CCR_CON_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_CCR_CON_T;

// CAM_CCR_YLUT CAM+A94H
typedef struct {
        MUINT32 CCR_Y_CPX1                : 8;
        MUINT32 CCR_Y_CPX2                : 8;
        MUINT32 CCR_Y_SP1                 : 7;
        MUINT32 rsv_23                    : 1;
        MUINT32 CCR_Y_CPY1                : 7;
        MUINT32 rsv_31                    : 1;
} ISP_CAM_CCR_YLUT_T;

typedef union {
    enum { MASK     = 0x7F7FFFFF };
    enum { DEFAULT  = 0x13107828 };
    typedef ISP_CAM_CCR_YLUT_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_CCR_YLUT_T;

// CAM_CCR_UVLUT CAM+A98H
typedef struct {
        MUINT32 CCR_UV_X1                 : 8;
        MUINT32 CCR_UV_X2                 : 8;
        MUINT32 CCR_UV_GAIN_SP1           : 7;
        MUINT32 rsv_23                    : 1;
        MUINT32 CCR_UV_GAIN1              : 7;
        MUINT32 rsv_31                    : 1;
} ISP_CAM_CCR_UVLUT_T;

typedef union {
    enum { MASK     = 0x7F7FFFFF };
    enum { DEFAULT  = 0x40273004 };
    typedef ISP_CAM_CCR_UVLUT_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_CCR_UVLUT_T;

// CAM_CCR_YLUT2 CAM+A9CH
typedef struct {
        MUINT32 CCR_Y_SP0                 : 7;
        MUINT32 rsv_7                     : 1;
        MUINT32 CCR_Y_SP2                 : 7;
        MUINT32 rsv_15                    : 1;
        MUINT32 CCR_Y_CPY0                : 7;
        MUINT32 rsv_23                    : 1;
        MUINT32 CCR_Y_CPY2                : 7;
        MUINT32 rsv_31                    : 1;
} ISP_CAM_CCR_YLUT2_T;

typedef union {
    enum { MASK     = 0x7F7F7F7F };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_CCR_YLUT2_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_CCR_YLUT2_T;

//
typedef union {
    enum { COUNT = 4 };
    MUINT32 set[COUNT];
    struct {
        ISP_NVRAM_CCR_CON_T   con;
        ISP_NVRAM_CCR_YLUT_T  ylut;
        ISP_NVRAM_CCR_UVLUT_T uvlut;
        ISP_NVRAM_CCR_YLUT2_T ylut2;
    };
} ISP_NVRAM_CCR_T;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// PCA
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CAM_PCA_CON1 CAM+1E00H
typedef struct {
        MUINT32 PCA_LUT_360               : 1;
        MUINT32 rsv_1                     : 3;
        MUINT32 RSV                       : 1;
        MUINT32 rsv_5                     : 27;
} ISP_CAM_PCA_CON1_T;

typedef union {
    enum { MASK     = 0x00000001 };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_PCA_CON1_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_PCA_CON1_T;

// CAM_PCA_CON2 CAM+1E04H
typedef struct {
        MUINT32 PCA_C_TH                  : 5;
        MUINT32 rsv_5                     : 3;
        MUINT32 RSV                       : 2;
        MUINT32 rsv_10                    : 2;
        MUINT32 PCA_S_TH_EN               : 1;
        MUINT32 rsv_13                    : 3;
        MUINT32 PCA_S_TH                  : 8;
        MUINT32 rsv_24                    : 8;
} ISP_CAM_PCA_CON2_T;

typedef union {
    enum { MASK     = 0x00FF101F };
    enum { DEFAULT  = 0x00FF0200 };
    typedef ISP_CAM_PCA_CON2_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_PCA_CON2_T;

//
typedef union{
    enum { COUNT = 2 };
    MUINT32 set[COUNT];
    struct {
        ISP_NVRAM_PCA_CON1_T  con1;
        ISP_NVRAM_PCA_CON2_T  con2;
    };
} ISP_NVRAM_PCA_T;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// PCA LUT
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
enum { PCA_BIN_NUM = 180 };

// CAM_PCA_TBL CAM+1800H
typedef struct {
        MUINT32 PCA_LUMA_GAIN             : 8;
        MUINT32 PCA_SAT_GAIN              : 8;
        MUINT32 PCA_HUE_SHIFT             : 8;
        MUINT32 rsv_24                    : 8;
} ISP_NVRAM_PCA_BIN_T;

//
typedef struct {
    ISP_NVRAM_PCA_BIN_T lut_lo[PCA_BIN_NUM];
    ISP_NVRAM_PCA_BIN_T lut_md[PCA_BIN_NUM];
    ISP_NVRAM_PCA_BIN_T lut_hi[PCA_BIN_NUM];
} ISP_NVRAM_PCA_LUTS_T;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// SE
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CAM_SEEE_EDGE_CTRL CAM+ACCH
typedef struct {
        MUINT32 SE_EDGE                   : 2;
        MUINT32 rsv_2                     : 30;
} ISP_CAM_SEEE_EDGE_CTRL_T;

typedef union {
    enum { MASK     = 0x00000003 };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_SEEE_EDGE_CTRL_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_SEEE_EDGE_CTRL_T;

// CAM_SEEE_Y_CTRL CAM+AD0H
typedef struct {
        MUINT32 SE_Y                      : 1;
        MUINT32 SE_Y_CONST                : 8;
        MUINT32 rsv_9                     : 7;
        MUINT32 SE_YOUT_QBIT              : 3;
        MUINT32 rsv_19                    : 1;
        MUINT32 SE_COUT_QBIT              : 3;
        MUINT32 rsv_23                    : 9;
} ISP_CAM_SEEE_Y_CTRL_T;

typedef union {
    enum { MASK     = 0x007701FF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_SEEE_Y_CTRL_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_SEEE_Y_CTRL_T;

// CAM_SEEE_EDGE_CTRL1 CAM+AD4H
typedef struct {
        MUINT32 SE_HEDGE_SEL              : 1;
        MUINT32 SE_EGAIN_HA               : 4;
        MUINT32 SE_EGAIN_HB               : 5;
        MUINT32 SE_EGAIN_HC               : 5;
        MUINT32 rsv_15                    : 1;
        MUINT32 SE_VEDGE_SEL              : 1;
        MUINT32 SE_EGAIN_VA               : 4;
        MUINT32 SE_EGAIN_VB               : 5;
        MUINT32 SE_EGAIN_VC               : 5;
        MUINT32 rsv_31                    : 1;
} ISP_CAM_SEEE_EDGE_CTRL1_T;

typedef union {
    enum { MASK     = 0x7FFF7FFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_SEEE_EDGE_CTRL1_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_SEEE_EDGE_CTRL1_T;

// CAM_SEEE_EDGE_CTRL2 CAM+AD8H
typedef struct {
        MUINT32 SE_THRE_RT                : 5;
        MUINT32 SE_EMBOSS1                : 1;
        MUINT32 SE_EMBOSS2                : 1;
        MUINT32 rsv_7                     : 25;
} ISP_CAM_SEEE_EDGE_CTRL2_T;

typedef union {
    enum { MASK     = 0x0000007F };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_SEEE_EDGE_CTRL2_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_SEEE_EDGE_CTRL2_T;

// CAM_SEEE_EDGE_CTRL3 CAM+ADCH
typedef struct {
        MUINT32 SE_ONLYC                  : 1;
        MUINT32 SE_CORE_CON               : 7;
        MUINT32 SE_ETH_CON                : 8;
        MUINT32 SE_TOP_SLOPE              : 1;
        MUINT32 SE_OILEN                  : 1;
        MUINT32 rsv_18                    : 14;
} ISP_CAM_SEEE_EDGE_CTRL3_T;

typedef union {
    enum { MASK     = 0x0003FFFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_SEEE_EDGE_CTRL3_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_SEEE_EDGE_CTRL3_T;

// CAM_SEEE_SPECIAL_CTRL CAM+AE0H
typedef struct {
        MUINT32 SE_SPECIPONLY             : 2;
        MUINT32 SE_SPECIABS               : 1;
        MUINT32 SE_SPECIINV               : 1;
        MUINT32 SE_SPECIGAIN              : 2;
        MUINT32 SE_KNEESEL                : 2;
        MUINT32 rsv_8                     : 24;
} ISP_CAM_SEEE_SPECIAL_CTRL_T;

typedef union {
    enum { MASK     = 0x000000FF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_SEEE_SPECIAL_CTRL_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_SEEE_SPECIAL_CTRL_T;

// CAM_SEEE_CORE_CTRL1 CAM+AE4H
typedef struct {
        MUINT32 SE_COREH                  : 7;
        MUINT32 SE_SUP                    : 2;
        MUINT32 SE_ETH3                   : 8;
        MUINT32 SE_SDN                    : 2;
        MUINT32 SE_COREH2                 : 6;
        MUINT32 rsv_25                    : 7;
} ISP_CAM_SEEE_CORE_CTRL1_T;

typedef union {
    enum { MASK     = 0x01FFFFFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_SEEE_CORE_CTRL1_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_SEEE_CORE_CTRL1_T;

// CAM_SEEE_CORE_CTRL2 CAM+AE8H
typedef struct {
        MUINT32 SE_E_TH1_V                : 7;
        MUINT32 SE_SUP_V                  : 2;
        MUINT32 SE_E_TH3_V                : 8;
        MUINT32 SE_SDN_V                  : 2;
        MUINT32 SE_HALF_V                 : 6;
        MUINT32 rsv_25                    : 7;
} ISP_CAM_SEEE_CORE_CTRL2_T;

typedef union {
    enum { MASK     = 0x01FFFFFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_SEEE_CORE_CTRL2_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_SEEE_CORE_CTRL2_T;

//
typedef union{
    enum { COUNT = 8 };
    MUINT32 set[COUNT];
    struct {
        ISP_NVRAM_SEEE_EDGE_CTRL_T    edge_ctrl;
        ISP_NVRAM_SEEE_Y_CTRL_T       y_ctrl;
        ISP_NVRAM_SEEE_EDGE_CTRL1_T   edge_ctrl1;
        ISP_NVRAM_SEEE_EDGE_CTRL2_T   edge_ctrl2;
        ISP_NVRAM_SEEE_EDGE_CTRL3_T   edge_ctrl3;
        ISP_NVRAM_SEEE_SPECIAL_CTRL_T special_ctrl;
        ISP_NVRAM_SEEE_CORE_CTRL1_T   core_ctrl1;
        ISP_NVRAM_SEEE_CORE_CTRL2_T   core_ctrl2;
    };
} ISP_NVRAM_SE_T;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// NR1 (CT only)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CAM_NR1_CON CAM+0840H
typedef struct {
        MUINT32 rsv_0                     : 4;
        MUINT32 NR1_CT_EN                 : 1;
        MUINT32 rsv_5                     : 27;
} ISP_CAM_NR1_CON_T;

typedef union {
    enum { MASK     = 0x00000010 };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_NR1_CON_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_NR1_CON_T;

// CAM_NR1_CT_CON CAM+0844H
typedef struct {
        MUINT32 NR1_CT_MD                 : 2;
        MUINT32 NR1_CT_MD2                : 2;
        MUINT32 NR1_CT_THRD               : 10;
        MUINT32 rsv_14                    : 2;
        MUINT32 NR1_MBND                  : 10;
        MUINT32 rsv_26                    : 2;
        MUINT32 NR1_CT_SLOPE              : 2;
        MUINT32 NR1_CT_DIV                : 2;
} ISP_CAM_NR1_CT_CON_T;

typedef union {
    enum { MASK     = 0xF3FF3FFF };
    enum { DEFAULT  = 0x50802003 };
    typedef ISP_CAM_NR1_CT_CON_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_NR1_CT_CON_T;

//
typedef union {
    enum { COUNT = 2 };
    MUINT32 set[COUNT];
    struct {
        ISP_NVRAM_NR1_CON_T    con;
        ISP_NVRAM_NR1_CT_CON_T ct_con;
    };
} ISP_NVRAM_NR1_T;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// NR3D
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CAM_NR3D_BLEND CAM+F00H
typedef struct {
        MUINT32 NR3D_GAIN                 : 5;
        MUINT32 rsv_5                     : 3;
        MUINT32 NR3D_ROUND_Y              : 5;
        MUINT32 rsv_13                    : 3;
        MUINT32 NR3D_ROUND_U              : 5;
        MUINT32 rsv_21                    : 3;
        MUINT32 NR3D_ROUND_V              : 5;
        MUINT32 rsv_29                    : 3;
} ISP_CAM_NR3D_BLEND_T;

typedef union {
    enum { MASK     = 0x1F1F1F1F };
    enum { DEFAULT  = 0x10101019 };
    typedef ISP_CAM_NR3D_BLEND_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_NR3D_BLEND_T;

// CAM_NR3D_SKIP_KEY CAM+F04H
typedef struct {
        MUINT32 NR3D_SKIP_KEY_Y           : 8;
        MUINT32 NR3D_SKIP_KEY_U           : 8;
        MUINT32 NR3D_SKIP_KEY_V           : 8;
        MUINT32 rsv_24                    : 8;
} ISP_CAM_NR3D_SKIP_KEY_T;

typedef union {
    enum { MASK     = 0x00FFFFFF };
    enum { DEFAULT  = 0x00FFFFFF };
    typedef ISP_CAM_NR3D_SKIP_KEY_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_NR3D_SKIP_KEY_T;

// CAM_NR3D_FBCNT_OFF CAM+F08H
typedef struct {
        MUINT32 rsv_0                     : 1;
        MUINT32 NR3D_FBCNT_XOFF           : 13;
        MUINT32 rsv_14                    : 2;
        MUINT32 NR3D_FBCNT_YOFF           : 14;
        MUINT32 rsv_30                    : 2;
} ISP_CAM_NR3D_FBCNT_OFF_T;

typedef union {
    enum { MASK     = 0x3FFF3FFE };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_NR3D_FBCNT_OFF_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_NR3D_FBCNT_OFF_T;

// CAM_NR3D_FBCNT_SIZ CAM+F0CH
typedef struct {
        MUINT32 rsv_0                     : 1;
        MUINT32 NR3D_FBCNT_XSIZ           : 13;
        MUINT32 rsv_14                    : 2;
        MUINT32 NR3D_FBCNT_YSIZ           : 14;
        MUINT32 rsv_30                    : 2;
} ISP_CAM_NR3D_FBCNT_SIZ_T;

typedef union {
    enum { MASK     = 0x3FFF3FFE };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_NR3D_FBCNT_SIZ_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_NR3D_FBCNT_SIZ_T;

// CAM_NR3D_FB_COUNT CAM+F10H
typedef struct {
        MUINT32 NR3D_FB_COUNT             : 28;
        MUINT32 NR3D_LIMIT_OUTSIDE_COUNT_TH : 2;
        MUINT32 rsv_30                    : 2;
} ISP_CAM_NR3D_FB_COUNT_T;

typedef union {
    enum { MASK     = 0x3FFFFFFF };
    enum { DEFAULT  = 0x20000000 };
    typedef ISP_CAM_NR3D_FB_COUNT_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_NR3D_FB_COUNT_T;

// CAM_NR3D_LIMIT_CPX CAM+F14H
typedef struct {
        MUINT32 NR3D_LIMIT_CPX1           : 8;
        MUINT32 NR3D_LIMIT_CPX2           : 8;
        MUINT32 NR3D_LIMIT_CPX3           : 8;
        MUINT32 NR3D_LIMIT_CPX4           : 8;
} ISP_CAM_NR3D_LIMIT_CPX_T;

typedef union {
    enum { MASK     = 0xFFFFFFFF };
    enum { DEFAULT  = 0xA0604020 };
    typedef ISP_CAM_NR3D_LIMIT_CPX_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_NR3D_LIMIT_CPX_T;

// CAM_NR3D_LIMIT_Y_CON1 CAM+F18H
typedef struct {
        MUINT32 NR3D_LIMIT_Y0             : 5;
        MUINT32 rsv_5                     : 3;
        MUINT32 NR3D_LIMIT_Y0_TH          : 5;
        MUINT32 rsv_13                    : 3;
        MUINT32 NR3D_LIMIT_Y1             : 5;
        MUINT32 rsv_21                    : 3;
        MUINT32 NR3D_LIMIT_Y1_TH          : 5;
        MUINT32 rsv_29                    : 3;
} ISP_CAM_NR3D_LIMIT_Y_CON1_T;

typedef union {
    enum { MASK     = 0x1F1F1F1F };
    enum { DEFAULT  = 0x140A180C };
    typedef ISP_CAM_NR3D_LIMIT_Y_CON1_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_NR3D_LIMIT_Y_CON1_T;

// CAM_NR3D_LIMIT_Y_CON2 CAM+F1CH
typedef struct {
        MUINT32 NR3D_LIMIT_Y2             : 5;
        MUINT32 rsv_5                     : 3;
        MUINT32 NR3D_LIMIT_Y2_TH          : 5;
        MUINT32 rsv_13                    : 3;
        MUINT32 NR3D_LIMIT_Y3             : 5;
        MUINT32 rsv_21                    : 3;
        MUINT32 NR3D_LIMIT_Y3_TH          : 5;
        MUINT32 rsv_29                    : 3;
} ISP_CAM_NR3D_LIMIT_Y_CON2_T;

typedef union {
    enum { MASK     = 0x1F1F1F1F };
    enum { DEFAULT  = 0x0A051008 };
    typedef ISP_CAM_NR3D_LIMIT_Y_CON2_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_NR3D_LIMIT_Y_CON2_T;

// CAM_NR3D_LIMIT_Y_CON3 CAM+F20H
typedef struct {
        MUINT32 NR3D_LIMIT_Y4             : 5;
        MUINT32 rsv_5                     : 3;
        MUINT32 NR3D_LIMIT_Y4_TH          : 5;
        MUINT32 rsv_13                    : 19;
} ISP_CAM_NR3D_LIMIT_Y_CON3_T;

typedef union {
    enum { MASK     = 0x00001F1F };
    enum { DEFAULT  = 0x00000603 };
    typedef ISP_CAM_NR3D_LIMIT_Y_CON3_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_NR3D_LIMIT_Y_CON3_T;

// CAM_NR3D_LIMIT_U_CON1 CAM+F24H
typedef struct {
        MUINT32 NR3D_LIMIT_U0             : 5;
        MUINT32 rsv_5                     : 3;
        MUINT32 NR3D_LIMIT_U0_TH          : 5;
        MUINT32 rsv_13                    : 3;
        MUINT32 NR3D_LIMIT_U1             : 5;
        MUINT32 rsv_21                    : 3;
        MUINT32 NR3D_LIMIT_U1_TH          : 5;
        MUINT32 rsv_29                    : 3;
} ISP_CAM_NR3D_LIMIT_U_CON1_T;

typedef union {
    enum { MASK     = 0x1F1F1F1F };
    enum { DEFAULT  = 0x05050505 };
    typedef ISP_CAM_NR3D_LIMIT_U_CON1_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_NR3D_LIMIT_U_CON1_T;

// CAM_NR3D_LIMIT_U_CON2 CAM+F28H
typedef struct {
        MUINT32 NR3D_LIMIT_U2             : 5;
        MUINT32 rsv_5                     : 3;
        MUINT32 NR3D_LIMIT_U2_TH          : 5;
        MUINT32 rsv_13                    : 3;
        MUINT32 NR3D_LIMIT_U3             : 5;
        MUINT32 rsv_21                    : 3;
        MUINT32 NR3D_LIMIT_U3_TH          : 5;
        MUINT32 rsv_29                    : 3;
} ISP_CAM_NR3D_LIMIT_U_CON2_T;

typedef union {
    enum { MASK     = 0x1F1F1F1F };
    enum { DEFAULT  = 0x05050505 };
    typedef ISP_CAM_NR3D_LIMIT_U_CON2_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_NR3D_LIMIT_U_CON2_T;

// CAM_NR3D_LIMIT_U_CON3 CAM+F2CH
typedef struct {
        MUINT32 NR3D_LIMIT_U4             : 5;
        MUINT32 rsv_5                     : 3;
        MUINT32 NR3D_LIMIT_U4_TH          : 5;
        MUINT32 rsv_13                    : 19;
} ISP_CAM_NR3D_LIMIT_U_CON3_T;

typedef union {
    enum { MASK     = 0x00001F1F };
    enum { DEFAULT  = 0x00000202 };
    typedef ISP_CAM_NR3D_LIMIT_U_CON3_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_NR3D_LIMIT_U_CON3_T;

// CAM_NR3D_LIMIT_V_CON1 CAM+F30H
typedef struct {
        MUINT32 NR3D_LIMIT_V0             : 5;
        MUINT32 rsv_5                     : 3;
        MUINT32 NR3D_LIMIT_V0_TH          : 5;
        MUINT32 rsv_13                    : 3;
        MUINT32 NR3D_LIMIT_V1             : 5;
        MUINT32 rsv_21                    : 3;
        MUINT32 NR3D_LIMIT_V1_TH          : 5;
        MUINT32 rsv_29                    : 3;
} ISP_CAM_NR3D_LIMIT_V_CON1_T;

typedef union {
    enum { MASK     = 0x1F1F1F1F };
    enum { DEFAULT  = 0x05050505 };
    typedef ISP_CAM_NR3D_LIMIT_V_CON1_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_NR3D_LIMIT_V_CON1_T;

// CAM_NR3D_LIMIT_V_CON2 CAM+F34H
typedef struct {
        MUINT32 NR3D_LIMIT_V2             : 5;
        MUINT32 rsv_5                     : 3;
        MUINT32 NR3D_LIMIT_V2_TH          : 5;
        MUINT32 rsv_13                    : 3;
        MUINT32 NR3D_LIMIT_V3             : 5;
        MUINT32 rsv_21                    : 3;
        MUINT32 NR3D_LIMIT_V3_TH          : 5;
        MUINT32 rsv_29                    : 3;
} ISP_CAM_NR3D_LIMIT_V_CON2_T;

typedef union {
    enum { MASK     = 0x1F1F1F1F };
    enum { DEFAULT  = 0x05050505 };
    typedef ISP_CAM_NR3D_LIMIT_V_CON2_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_NR3D_LIMIT_V_CON2_T;

// CAM_NR3D_LIMIT_V_CON3 CAM+F38H
typedef struct {
        MUINT32 NR3D_LIMIT_V4             : 5;
        MUINT32 rsv_5                     : 3;
        MUINT32 NR3D_LIMIT_V4_TH          : 5;
        MUINT32 rsv_13                    : 19;
} ISP_CAM_NR3D_LIMIT_V_CON3_T;

typedef union {
    enum { MASK     = 0x00001F1F };
    enum { DEFAULT  = 0x00000202 };
    typedef ISP_CAM_NR3D_LIMIT_V_CON3_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_NR3D_LIMIT_V_CON3_T;

//
typedef union{
    enum { COUNT = 15 };
    MUINT32 set[COUNT];
    struct {
        ISP_NVRAM_NR3D_BLEND_T        blend;
        ISP_NVRAM_NR3D_SKIP_KEY_T     skip_key;
        ISP_NVRAM_NR3D_FBCNT_OFF_T    fbcnt_off;
        ISP_NVRAM_NR3D_FBCNT_SIZ_T    fbcnt_siz;
        ISP_NVRAM_NR3D_FB_COUNT_T     fb_count;
        ISP_NVRAM_NR3D_LIMIT_CPX_T    limit_cpx;
        ISP_NVRAM_NR3D_LIMIT_Y_CON1_T limit_y_con1;
        ISP_NVRAM_NR3D_LIMIT_Y_CON2_T limit_y_con2;
        ISP_NVRAM_NR3D_LIMIT_Y_CON3_T limit_y_con3;
        ISP_NVRAM_NR3D_LIMIT_U_CON1_T limit_u_con1;
        ISP_NVRAM_NR3D_LIMIT_U_CON2_T limit_u_con2;
        ISP_NVRAM_NR3D_LIMIT_U_CON3_T limit_u_con3;
        ISP_NVRAM_NR3D_LIMIT_V_CON1_T limit_v_con1;
        ISP_NVRAM_NR3D_LIMIT_V_CON2_T limit_v_con2;
        ISP_NVRAM_NR3D_LIMIT_V_CON3_T limit_v_con3;
    };
} ISP_NVRAM_NR3D_T;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// OBC
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CAM_OBC_OFFST0 CAM+0500H: OB for B channel
typedef struct {
    MUINT32 OBOFFSET0                 : 13;
    MUINT32 rsv_13                    : 19;
} ISP_CAM_OBC_OFFST0_T;

typedef union {
    enum { MASK     = 0x00001FFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_OBC_OFFST0_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_OBC_OFFST0_T;

// CAM_OBC_OFFST1 CAM+0504H: OB for Gr channel
typedef struct {
    MUINT32 OBOFFSET1                 : 13;
    MUINT32 rsv_13                    : 19;
} ISP_CAM_OBC_OFFST1_T;

typedef union {
    enum { MASK     = 0x00001FFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_OBC_OFFST1_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_OBC_OFFST1_T;

// CAM_OBC_OFFST2 CAM+0508H: OB for Gb channel
typedef struct {
    MUINT32 OBOFFSET2                 : 13;
    MUINT32 rsv_13                    : 19;
} ISP_CAM_OBC_OFFST2_T;

typedef union {
    enum { MASK     = 0x00001FFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_OBC_OFFST2_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_OBC_OFFST2_T;

// CAM_OBC_OFFST3 CAM+050CH: OB for R channel
typedef struct {
    MUINT32 OBOFFSET3                 : 13;
    MUINT32 rsv_13                    : 19;
} ISP_CAM_OBC_OFFST3_T;

typedef union {
    enum { MASK     = 0x00001FFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_OBC_OFFST3_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_OBC_OFFST3_T;

// CAM_OBC_GAIN0 CAM+0510H: OB gain for B channel
typedef struct {
    MUINT32 OBGAIN0                   : 13;
    MUINT32 rsv_13                    : 19;
} ISP_CAM_OBC_GAIN0_T;

typedef union {
    enum { MASK     = 0x00001FFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_OBC_GAIN0_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_OBC_GAIN0_T;

// CAM_OBC_GAIN1 CAM+0514H: OB gain for Gr channel
typedef struct {
    MUINT32 OBGAIN1                   : 13;
    MUINT32 rsv_13                    : 19;
} ISP_CAM_OBC_GAIN1_T;

typedef union {
    enum { MASK     = 0x00001FFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_OBC_GAIN1_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_OBC_GAIN1_T;

// CAM_OBC_GAIN2 CAM+0518H: OB gain for Gb channel
typedef struct {
    MUINT32 OBGAIN2                   : 13;
    MUINT32 rsv_13                    : 19;
} ISP_CAM_OBC_GAIN2_T;

typedef union {
    enum { MASK     = 0x00001FFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_OBC_GAIN2_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_OBC_GAIN2_T;

// CAM_OBC_GAIN3 CAM+051CH: OB gain for R channel
typedef struct {
    MUINT32 OBGAIN3                   : 13;
    MUINT32 rsv_13                    : 19;
} ISP_CAM_OBC_GAIN3_T;

typedef union {
    enum { MASK     = 0x00001FFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_OBC_GAIN3_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_OBC_GAIN3_T;

//
typedef union {
    enum { COUNT = 8 };
    MUINT32 set[COUNT];
    struct {
        ISP_NVRAM_OBC_OFFST0_T  offst0; // Q.1.12
        ISP_NVRAM_OBC_OFFST1_T  offst1; // Q.1.12
        ISP_NVRAM_OBC_OFFST2_T  offst2; // Q.1.12
        ISP_NVRAM_OBC_OFFST3_T  offst3; // Q.1.12
        ISP_NVRAM_OBC_GAIN0_T   gain0; // Q.0.4.9
        ISP_NVRAM_OBC_GAIN1_T   gain1; // Q.0.4.9
        ISP_NVRAM_OBC_GAIN2_T   gain2; // Q.0.4.9
        ISP_NVRAM_OBC_GAIN3_T   gain3; // Q.0.4.9
    };
} ISP_NVRAM_OBC_T;



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// AWB Statistics
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CAM_AWB_WIN_ORG CAM+05B0H
typedef struct {
        MUINT32 AWB_W_ORIGIN_X            : 13;
        MUINT32 rsv_13                    : 3;
        MUINT32 AWB_W_ORIGIN_Y            : 13;
        MUINT32 rsv_29                    : 3;
} ISP_CAM_AWB_WIN_ORG_T;

typedef union {
    enum { MASK     = 0x1FFF1FFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_AWB_WIN_ORG_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AWB_WIN_ORG_T;

// CAM_AWB_WIN_SIZE CAM+05B4H
typedef struct {
        MUINT32 AWB_W_SIZE_X              : 13;
        MUINT32 rsv_13                    : 3;
        MUINT32 AWB_W_SIZE_Y              : 13;
        MUINT32 rsv_29                    : 3;
} ISP_CAM_AWB_WIN_SIZE_T;

typedef union {
    enum { MASK     = 0x1FFF1FFF };
    enum { DEFAULT  = 0x00020004 };
    typedef ISP_CAM_AWB_WIN_SIZE_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AWB_WIN_SIZE_T;

// CAM_AWB_WIN_PITCH CAM+05B8H
typedef struct {
        MUINT32 AWB_W_PITCH_X               : 13;
        MUINT32 rsv_13                    : 3;
        MUINT32 AWB_W_PITCH_Y               : 13;
        MUINT32 rsv_29                    : 3;
} ISP_CAM_AWB_WIN_PITCH_T;

typedef union {
    enum { MASK     = 0x1FFF1FFF };
    enum { DEFAULT  = 0x00020004 };
    typedef ISP_CAM_AWB_WIN_PITCH_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AWB_WIN_PITCH_T;

// CAM_AWB_WIN_NUM CAM+05BCH
typedef struct {
        MUINT32 AWB_W_NUM_X               : 8;
        MUINT32 rsv_8                     : 8;
        MUINT32 AWB_W_NUM_Y               : 8;
        MUINT32 rsv_24                    : 8;
} ISP_CAM_AWB_WIN_NUM_T;

typedef union {
    enum { MASK     = 0x00FF00FF };
    enum { DEFAULT  = 0x00010001 };
    typedef ISP_CAM_AWB_WIN_NUM_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AWB_WIN_NUM_T;

// CAM_AWB_RAWPREGAIN1_0 CAM+05COH
typedef struct {
        MUINT32 AWB_GAIN1_R               : 13;
        MUINT32 rsv_13                    : 3;
        MUINT32 AWB_GAIN1_G               : 13;
        MUINT32 rsv_29                    : 3;
} ISP_CAM_AWB_RAWPREGAIN1_0_T;

typedef union {
    enum { MASK     = 0x1FFF1FFF };
    enum { DEFAULT  = 0x02000200 };
    typedef ISP_CAM_AWB_RAWPREGAIN1_0_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AWB_RAWPREGAIN1_0_T;

// CAM_AWB_RAWPREGAIN1_1 CAM+05C4H
typedef struct {
        MUINT32 AWB_GAIN1_B               : 13;
        MUINT32 rsv_13                    : 19;
} ISP_CAM_AWB_RAWPREGAIN1_1_T;

typedef union {
    enum { MASK     = 0x00001FFF };
    enum { DEFAULT  = 0x00000200 };
    typedef ISP_CAM_AWB_RAWPREGAIN1_1_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AWB_RAWPREGAIN1_1_T;

// CAM_AWB_RAWLIMIT1_0 CAM+05C8H
typedef struct {
        MUINT32 AWB_LIMIT1_R              : 12;
        MUINT32 rsv_12                    : 4;
        MUINT32 AWB_LIMIT1_G              : 12;
        MUINT32 rsv_28                    : 4;
} ISP_CAM_AWB_RAWLIMIT1_0_T;

typedef union {
    enum { MASK     = 0x0FFF0FFF };
    enum { DEFAULT  = 0x0FFF0FFF };
    typedef ISP_CAM_AWB_RAWLIMIT1_0_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AWB_RAWLIMIT1_0_T;

// CAM_AWB_RAWLIMIT1_1 CAM+05CCH
typedef struct {
        MUINT32 AWB_LIMIT1_B              : 12;
        MUINT32 rsv_12                    : 20;
} ISP_CAM_AWB_RAWLIMIT1_1_T;

typedef union {
    enum { MASK     = 0x00000FFF };
    enum { DEFAULT  = 0x00000FFF };
    typedef ISP_CAM_AWB_RAWLIMIT1_1_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AWB_RAWLIMIT1_1_T;

// CAM_AWB_LOW_THR CAM+05D0H
typedef struct {
        MUINT32 AWB_LOW_THR0              : 8;
        MUINT32 AWB_LOW_THR1              : 8;
        MUINT32 AWB_LOW_THR2              : 8;
        MUINT32 rsv_24                    : 8;
} ISP_CAM_AWB_LOW_THR_T;

typedef union {
    enum { MASK     = 0x00FFFFFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_AWB_LOW_THR_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AWB_LOW_THR_T;

// CAM_AWB_HI_THR CAM+05D4H
typedef struct {
        MUINT32 AWB_HI_THR0               : 8;
        MUINT32 AWB_HI_THR1               : 8;
        MUINT32 AWB_HI_THR2               : 8;
        MUINT32 rsv_24                    : 8;
} ISP_CAM_AWB_HI_THR_T;

typedef union {
    enum { MASK     = 0x00FFFFFF };
    enum { DEFAULT  = 0x00FFFFFF };
    typedef ISP_CAM_AWB_HI_THR_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AWB_HI_THR_T;

// CAM_AWB_PIXEL_CNT0 CAM+05D8H
typedef struct {
        MUINT32 PIXEL_CNT0                : 24;
        MUINT32 rsv_24                    : 8;
} ISP_CAM_AWB_PIXEL_CNT0_T;

typedef union {
    enum { MASK     = 0x00FFFFFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_AWB_PIXEL_CNT0_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AWB_PIXEL_CNT0_T;

// CAM_AWB_PIXEL_CNT1 CAM+05DCH
typedef struct {
        MUINT32 PIXEL_CNT1                : 24;
        MUINT32 rsv_24                    : 8;
} ISP_CAM_AWB_PIXEL_CNT1_T;

typedef union {
    enum { MASK     = 0x00FFFFFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_AWB_PIXEL_CNT1_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AWB_PIXEL_CNT1_T;

// CAM_AWB_PIXEL_CNT2 CAM+05E0H
typedef struct {
        MUINT32 PIXEL_CNT2                : 24;
        MUINT32 rsv_24                    : 8;
} ISP_CAM_AWB_PIXEL_CNT2_T;

typedef union {
    enum { MASK     = 0x00FFFFFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_AWB_PIXEL_CNT2_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AWB_PIXEL_CNT2_T;

// CAM_AWB_ERR_THR CAM+05E4H
typedef struct {
        MUINT32 AWB_ERR_THR               : 12;
        MUINT32 rsv_12                    : 20;
} ISP_CAM_AWB_ERR_THR_T;

typedef union {
    enum { MASK     = 0x00000FFF };
    enum { DEFAULT  = 0x00000100 };
    typedef ISP_CAM_AWB_ERR_THR_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AWB_ERR_THR_T;

// CAM_AWB_ROT CAM+05E8H
typedef struct {
        MUINT32 AWB_C                     : 9;
        MUINT32 rsv_9                     : 7;
        MUINT32 AWB_S                     : 9;
        MUINT32 rsv_25                    : 7;
} ISP_CAM_AWB_ROT_T;

typedef union {
    enum { MASK     = 0x01FF01FF };
    enum { DEFAULT  = 0x00000100 };
    typedef ISP_CAM_AWB_ROT_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AWB_ROT_T;

// CAM_AWB_L0_X CAM+05ECH
typedef struct {
        MUINT32 AWB_L0_X_LOW              : 14;
        MUINT32 rsv_14                    : 2;
        MUINT32 AWB_L0_X_UP               : 14;
        MUINT32 rsv_30                    : 2;
} ISP_CAM_AWB_L0_X_T;

typedef union {
    enum { MASK     = 0x3FFF3FFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_AWB_L0_X_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AWB_L0_X_T;

// CAM_AWB_L0_Y CAM+05F0H
typedef struct {
        MUINT32 AWB_L0_Y_LOW              : 14;
        MUINT32 rsv_14                    : 2;
        MUINT32 AWB_L0_Y_UP               : 14;
        MUINT32 rsv_30                    : 2;
} ISP_CAM_AWB_L0_Y_T;

typedef union {
    enum { MASK     = 0x3FFF3FFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_AWB_L0_Y_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AWB_L0_Y_T;

// CAM_AWB_L1_X CAM+05F4H
typedef struct {
        MUINT32 AWB_L1_X_LOW              : 14;
        MUINT32 rsv_14                    : 2;
        MUINT32 AWB_L1_X_UP               : 14;
        MUINT32 rsv_30                    : 2;
} ISP_CAM_AWB_L1_X_T;

typedef union {
    enum { MASK     = 0x3FFF3FFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_AWB_L1_X_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AWB_L1_X_T;

// CAM_AWB_L1_Y CAM+05F8H
typedef struct {
        MUINT32 AWB_L1_Y_LOW              : 14;
        MUINT32 rsv_14                    : 2;
        MUINT32 AWB_L1_Y_UP               : 14;
        MUINT32 rsv_30                    : 2;
} ISP_CAM_AWB_L1_Y_T;

typedef union {
    enum { MASK     = 0x3FFF3FFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_AWB_L1_Y_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AWB_L1_Y_T;

// CAM_AWB_L2_X CAM+05FCH
typedef struct {
        MUINT32 AWB_L2_X_LOW              : 14;
        MUINT32 rsv_14                    : 2;
        MUINT32 AWB_L2_X_UP               : 14;
        MUINT32 rsv_30                    : 2;
} ISP_CAM_AWB_L2_X_T;

typedef union {
    enum { MASK     = 0x3FFF3FFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_AWB_L2_X_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AWB_L2_X_T;

// CAM_AWB_L2_Y CAM+0600H
typedef struct {
        MUINT32 AWB_L2_Y_LOW              : 14;
        MUINT32 rsv_14                    : 2;
        MUINT32 AWB_L2_Y_UP               : 14;
        MUINT32 rsv_30                    : 2;
} ISP_CAM_AWB_L2_Y_T;

typedef union {
    enum { MASK     = 0x3FFF3FFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_AWB_L2_Y_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AWB_L2_Y_T;

// CAM_AWB_L3_X CAM+0604H
typedef struct {
        MUINT32 AWB_L3_X_LOW              : 14;
        MUINT32 rsv_14                    : 2;
        MUINT32 AWB_L3_X_UP               : 14;
        MUINT32 rsv_30                    : 2;
} ISP_CAM_AWB_L3_X_T;

typedef union {
    enum { MASK     = 0x3FFF3FFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_AWB_L3_X_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AWB_L3_X_T;

// CAM_AWB_L3_Y CAM+0608H
typedef struct {
        MUINT32 AWB_L3_Y_LOW              : 14;
        MUINT32 rsv_14                    : 2;
        MUINT32 AWB_L3_Y_UP               : 14;
        MUINT32 rsv_30                    : 2;
} ISP_CAM_AWB_L3_Y_T;

typedef union {
    enum { MASK     = 0x3FFF3FFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_AWB_L3_Y_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AWB_L3_Y_T;

// CAM_AWB_L4_X CAM+060CH
typedef struct {
        MUINT32 AWB_L4_X_LOW              : 14;
        MUINT32 rsv_14                    : 2;
        MUINT32 AWB_L4_X_UP               : 14;
        MUINT32 rsv_30                    : 2;
} ISP_CAM_AWB_L4_X_T;

typedef union {
    enum { MASK     = 0x3FFF3FFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_AWB_L4_X_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AWB_L4_X_T;

// CAM_AWB_L4_Y CAM+0610H
typedef struct {
        MUINT32 AWB_L4_Y_LOW              : 14;
        MUINT32 rsv_14                    : 2;
        MUINT32 AWB_L4_Y_UP               : 14;
        MUINT32 rsv_30                    : 2;
} ISP_CAM_AWB_L4_Y_T;

typedef union {
    enum { MASK     = 0x3FFF3FFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_AWB_L4_Y_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AWB_L4_Y_T;

// CAM_AWB_L5_X CAM+0614H
typedef struct {
        MUINT32 AWB_L5_X_LOW              : 14;
        MUINT32 rsv_14                    : 2;
        MUINT32 AWB_L5_X_UP               : 14;
        MUINT32 rsv_30                    : 2;
} ISP_CAM_AWB_L5_X_T;

typedef union {
    enum { MASK     = 0x3FFF3FFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_AWB_L5_X_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AWB_L5_X_T;

// CAM_AWB_L5_Y CAM+0618H
typedef struct {
        MUINT32 AWB_L5_Y_LOW              : 14;
        MUINT32 rsv_14                    : 2;
        MUINT32 AWB_L5_Y_UP               : 14;
        MUINT32 rsv_30                    : 2;
} ISP_CAM_AWB_L5_Y_T;

typedef union {
    enum { MASK     = 0x3FFF3FFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_AWB_L5_Y_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AWB_L5_Y_T;

// CAM_AWB_L6_X CAM+061CH
typedef struct {
        MUINT32 AWB_L6_X_LOW              : 14;
        MUINT32 rsv_14                    : 2;
        MUINT32 AWB_L6_X_UP               : 14;
        MUINT32 rsv_30                    : 2;
} ISP_CAM_AWB_L6_X_T;

typedef union {
    enum { MASK     = 0x3FFF3FFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_AWB_L6_X_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AWB_L6_X_T;

// CAM_AWB_L6_Y CAM+0620H
typedef struct {
        MUINT32 AWB_L6_Y_LOW              : 14;
        MUINT32 rsv_14                    : 2;
        MUINT32 AWB_L6_Y_UP               : 14;
        MUINT32 rsv_30                    : 2;
} ISP_CAM_AWB_L6_Y_T;

typedef union {
    enum { MASK     = 0x3FFF3FFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_AWB_L6_Y_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AWB_L6_Y_T;

// CAM_AWB_L7_X CAM+0624H
typedef struct {
        MUINT32 AWB_L7_X_LOW              : 14;
        MUINT32 rsv_14                    : 2;
        MUINT32 AWB_L7_X_UP               : 14;
        MUINT32 rsv_30                    : 2;
} ISP_CAM_AWB_L7_X_T;

typedef union {
    enum { MASK     = 0x3FFF3FFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_AWB_L7_X_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AWB_L7_X_T;

// CAM_AWB_L7_Y CAM+0628H
typedef struct {
        MUINT32 AWB_L7_Y_LOW              : 14;
        MUINT32 rsv_14                    : 2;
        MUINT32 AWB_L7_Y_UP               : 14;
        MUINT32 rsv_30                    : 2;
} ISP_CAM_AWB_L7_Y_T;

typedef union {
    enum { MASK     = 0x3FFF3FFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_AWB_L7_Y_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AWB_L7_Y_T;

// CAM_AWB_L8_X CAM+062CH
typedef struct {
        MUINT32 AWB_L8_X_LOW              : 14;
        MUINT32 rsv_14                    : 2;
        MUINT32 AWB_L8_X_UP               : 14;
        MUINT32 rsv_30                    : 2;
} ISP_CAM_AWB_L8_X_T;

typedef union {
    enum { MASK     = 0x3FFF3FFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_AWB_L8_X_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AWB_L8_X_T;

// CAM_AWB_L8_Y CAM+0630H
typedef struct {
        MUINT32 AWB_L8_Y_LOW              : 14;
        MUINT32 rsv_14                    : 2;
        MUINT32 AWB_L8_Y_UP               : 14;
        MUINT32 rsv_30                    : 2;
} ISP_CAM_AWB_L8_Y_T;

typedef union {
    enum { MASK     = 0x3FFF3FFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_AWB_L8_Y_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AWB_L8_Y_T;

// CAM_AWB_L9_X CAM+0634H
typedef struct {
        MUINT32 AWB_L9_X_LOW              : 14;
        MUINT32 rsv_14                    : 2;
        MUINT32 AWB_L9_X_UP               : 14;
        MUINT32 rsv_30                    : 2;
} ISP_CAM_AWB_L9_X_T;

typedef union {
    enum { MASK     = 0x3FFF3FFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_AWB_L9_X_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AWB_L9_X_T;

// CAM_AWB_L9_Y CAM+0638H
typedef struct {
        MUINT32 AWB_L9_Y_LOW              : 14;
        MUINT32 rsv_14                    : 2;
        MUINT32 AWB_L9_Y_UP               : 14;
        MUINT32 rsv_30                    : 2;
} ISP_CAM_AWB_L9_Y_T;

typedef union {
    enum { MASK     = 0x3FFF3FFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_AWB_L9_Y_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AWB_L9_Y_T;

//
typedef union {
    enum { COUNT = 35 };
    MUINT32 set[COUNT];
    struct {
        ISP_NVRAM_AWB_WIN_ORG_T         win_org;
        ISP_NVRAM_AWB_WIN_SIZE_T        win_size;
        ISP_NVRAM_AWB_WIN_PITCH_T       win_pitch;
        ISP_NVRAM_AWB_WIN_NUM_T         win_num;
        ISP_NVRAM_AWB_RAWPREGAIN1_0_T   rawpregain1_0;
        ISP_NVRAM_AWB_RAWPREGAIN1_1_T   rawpregain1_1;
        ISP_NVRAM_AWB_RAWLIMIT1_0_T     rawlimit1_0;
        ISP_NVRAM_AWB_RAWLIMIT1_1_T     rawlimit1_1;
        ISP_NVRAM_AWB_LOW_THR_T         low_thr;
        ISP_NVRAM_AWB_HI_THR_T          high_thr;
        ISP_NVRAM_AWB_PIXEL_CNT0_T      pixel_cnt0;
        ISP_NVRAM_AWB_PIXEL_CNT1_T      pixel_cnt1;
        ISP_NVRAM_AWB_PIXEL_CNT2_T      pixel_cnt2;
        ISP_NVRAM_AWB_ERR_THR_T         err_thr;
        ISP_NVRAM_AWB_ROT_T             rot;
        ISP_NVRAM_AWB_L0_X_T            l0_x;
        ISP_NVRAM_AWB_L0_Y_T            l0_y;
        ISP_NVRAM_AWB_L1_X_T            l1_x;
        ISP_NVRAM_AWB_L1_Y_T            l1_y;
        ISP_NVRAM_AWB_L2_X_T            l2_x;
        ISP_NVRAM_AWB_L2_Y_T            l2_y;
        ISP_NVRAM_AWB_L3_X_T            l3_x;
        ISP_NVRAM_AWB_L3_Y_T            l3_y;
        ISP_NVRAM_AWB_L4_X_T            l4_x;
        ISP_NVRAM_AWB_L4_Y_T            l4_y;
        ISP_NVRAM_AWB_L5_X_T            l5_x;
        ISP_NVRAM_AWB_L5_Y_T            l5_y;
        ISP_NVRAM_AWB_L6_X_T            l6_x;
        ISP_NVRAM_AWB_L6_Y_T            l6_y;
        ISP_NVRAM_AWB_L7_X_T            l7_x;
        ISP_NVRAM_AWB_L7_Y_T            l7_y;
        ISP_NVRAM_AWB_L8_X_T            l8_x;
        ISP_NVRAM_AWB_L8_Y_T            l8_y;
        ISP_NVRAM_AWB_L9_X_T            l9_x;
        ISP_NVRAM_AWB_L9_Y_T            l9_y;
    };
} ISP_NVRAM_AWB_STAT_CONFIG_T;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// AE Statistics
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CAM_AE_HST_CTL CAM+0650H
typedef struct {
        MUINT32 AE_HST0_EN                : 1;
        MUINT32 AE_HST1_EN                : 1;
        MUINT32 AE_HST2_EN                : 1;
        MUINT32 AE_HST3_EN                : 1;
        MUINT32 rsv_4                     : 28;
} ISP_CAM_AE_HST_CTL_T;

typedef union {
    enum { MASK     = 0x0000000F };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_AE_HST_CTL_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AE_HST_CTL_T;

// CAM_AE_RAWPREGAIN2_0 CAM+0654H
typedef struct {
        MUINT32 AE_GAIN2_R                : 12;
        MUINT32 rsv_12                    : 4;
        MUINT32 AE_GAIN2_G                : 12;
        MUINT32 rsv_28                    : 4;
} ISP_CAM_AE_RAWPREGAIN2_0_T;

typedef union {
    enum { MASK     = 0x0FFF0FFF };
    enum { DEFAULT  = 0x02000200 };
    typedef ISP_CAM_AE_RAWPREGAIN2_0_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AE_RAWPREGAIN2_0_T;

// CAM_AE_RAWPREGAIN2_1 CAM+0658H
typedef struct {
        MUINT32 AE_GAIN2_B                : 12;
        MUINT32 rsv_12                    : 20;
} ISP_CAM_AE_RAWPREGAIN2_1_T;

typedef union {
    enum { MASK     = 0x00000FFF };
    enum { DEFAULT  = 0x00000200 };
    typedef ISP_CAM_AE_RAWPREGAIN2_1_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AE_RAWPREGAIN2_1_T;

// CAM_AE_RAWLIMIT2_0 CAM+065CH
typedef struct {
        MUINT32 AE_LIMIT2_R               : 12;
        MUINT32 rsv_12                    : 4;
        MUINT32 AE_LIMIT2_G               : 12;
        MUINT32 rsv_28                    : 4;
} ISP_CAM_AE_RAWLIMIT2_0_T;

typedef union {
    enum { MASK     = 0x0FFF0FFF };
    enum { DEFAULT  = 0x0FFF0FFF };
    typedef ISP_CAM_AE_RAWLIMIT2_0_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AE_RAWLIMIT2_0_T;

// CAM_AE_RAWLIMIT2_1 CAM+0660H
typedef struct {
        MUINT32 AE_LIMIT2_B               : 12;
        MUINT32 rsv_12                    : 20;
} ISP_CAM_AE_RAWLIMIT2_1_T;

typedef union {
    enum { MASK     = 0x00000FFF };
    enum { DEFAULT  = 0x00000FFF };
    typedef ISP_CAM_AE_RAWLIMIT2_1_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AE_RAWLIMIT2_1_T;

// CAM_AE_MATRIX_COEF0 CAM+0664H
typedef struct {
        MUINT32 RC_CNV00                  : 11;
        MUINT32 rsv_11                    : 5;
        MUINT32 RC_CNV01                  : 11;
        MUINT32 rsv_27                    : 5;
} ISP_CAM_AE_MATRIX_COEF0_T;

typedef union {
    enum { MASK     = 0x07FF07FF };
    enum { DEFAULT  = 0x00000200 };
    typedef ISP_CAM_AE_MATRIX_COEF0_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AE_MATRIX_COEF0_T;

// CAM_AE_MATRIX_COEF1 CAM+0668H
typedef struct {
        MUINT32 RC_CNV02                  : 11;
        MUINT32 rsv_11                    : 5;
        MUINT32 RC_CNV10                  : 11;
        MUINT32 rsv_27                    : 5;
} ISP_CAM_AE_MATRIX_COEF1_T;

typedef union {
    enum { MASK     = 0x07FF07FF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_AE_MATRIX_COEF1_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AE_MATRIX_COEF1_T;

// CAM_AE_MATRIX_COEF2 CAM+066CH
typedef struct {
        MUINT32 RC_CNV11                  : 11;
        MUINT32 rsv_11                    : 5;
        MUINT32 RC_CNV12                  : 11;
        MUINT32 rsv_27                    : 5;
} ISP_CAM_AE_MATRIX_COEF2_T;

typedef union {
    enum { MASK     = 0x07FF07FF };
    enum { DEFAULT  = 0x00000200 };
    typedef ISP_CAM_AE_MATRIX_COEF2_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AE_MATRIX_COEF2_T;

// CAM_AE_MATRIX_COEF3 CAM+0670H
typedef struct {
        MUINT32 RC_CNV20                  : 11;
        MUINT32 rsv_11                    : 5;
        MUINT32 RC_CNV21                  : 11;
        MUINT32 rsv_27                    : 5;
} ISP_CAM_AE_MATRIX_COEF3_T;

typedef union {
    enum { MASK     = 0x07FF07FF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_AE_MATRIX_COEF3_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AE_MATRIX_COEF3_T;

// CAM_AE_MATRIX_COEF4 CAM+0674H
typedef struct {
        MUINT32 RC_CNV22                  : 11;
        MUINT32 rsv_11                    : 5;
        MUINT32 AE_RC_ACC                 : 4;
        MUINT32 rsv_20                    : 12;
} ISP_CAM_AE_MATRIX_COEF4_T;

typedef union {
    enum { MASK     = 0x000F07FF };
    enum { DEFAULT  = 0x00090200 };
    typedef ISP_CAM_AE_MATRIX_COEF4_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AE_MATRIX_COEF4_T;

// CAM_AE_YGAMMA_0 CAM+0678H
typedef struct {
        MUINT32 Y_GMR1                    : 8;
        MUINT32 Y_GMR2                    : 8;
        MUINT32 Y_GMR3                    : 8;
        MUINT32 Y_GMR4                    : 8;
} ISP_CAM_AE_YGAMMA_0_T;

typedef union {
    enum { MASK     = 0xFFFFFFFF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_AE_YGAMMA_0_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AE_YGAMMA_0_T;

// CAM_AE_YGAMMA_1 CAM+067CH
typedef struct {
        MUINT32 Y_GMR5                    : 8;
        MUINT32 rsv_8                     : 24;
} ISP_CAM_AE_YGAMMA_1_T;

typedef union {
    enum { MASK     = 0x000000FF };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_AE_YGAMMA_1_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AE_YGAMMA_1_T;

// CAM_AE_HST_SET CAM+0680H
typedef struct {
        MUINT32 AE_HST0_BIN               : 2;
        MUINT32 rsv_2                     : 2;
        MUINT32 AE_HST1_BIN               : 2;
        MUINT32 rsv_6                     : 2;
        MUINT32 AE_HST2_BIN               : 2;
        MUINT32 rsv_10                    : 2;
        MUINT32 AE_HST3_BIN               : 2;
        MUINT32 rsv_14                    : 2;
        MUINT32 AE_HST0_COLOR             : 3;
        MUINT32 rsv_19                    : 1;
        MUINT32 AE_HST1_COLOR             : 3;
        MUINT32 rsv_23                    : 1;
        MUINT32 AE_HST2_COLOR             : 3;
        MUINT32 rsv_27                    : 1;
        MUINT32 AE_HST3_COLOR             : 3;
        MUINT32 rsv_31                    : 1;
} ISP_CAM_AE_HST_SET_T;

typedef union {
    enum { MASK     = 0x77773333 };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_AE_HST_SET_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AE_HST_SET_T;

// CAM_AE_HST0_RNG CAM+0684H
typedef struct {
        MUINT32 AE_HST0_X_LOW             : 7;
        MUINT32 rsv_7                     : 1;
        MUINT32 AE_HST0_X_HI              : 7;
        MUINT32 rsv_15                    : 1;
        MUINT32 AE_HST0_Y_LOW             : 7;
        MUINT32 rsv_23                    : 1;
        MUINT32 AE_HST0_Y_HI              : 7;
        MUINT32 rsv_31                    : 1;
} ISP_CAM_AE_HST0_RNG_T;

typedef union {
    enum { MASK     = 0x7F7F7F7F };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_AE_HST0_RNG_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AE_HST0_RNG_T;

// CAM_AE_HST1_RNG CAM+0688H
typedef struct {
        MUINT32 AE_HST1_X_LOW             : 7;
        MUINT32 rsv_7                     : 1;
        MUINT32 AE_HST1_X_HI              : 7;
        MUINT32 rsv_15                    : 1;
        MUINT32 AE_HST1_Y_LOW             : 7;
        MUINT32 rsv_23                    : 1;
        MUINT32 AE_HST1_Y_HI              : 7;
        MUINT32 rsv_31                    : 1;
} ISP_CAM_AE_HST1_RNG_T;

typedef union {
    enum { MASK     = 0x7F7F7F7F };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_AE_HST1_RNG_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AE_HST1_RNG_T;

// CAM_AE_HST2_RNG CAM+068CH
typedef struct {
        MUINT32 AE_HST2_X_LOW             : 7;
        MUINT32 rsv_7                     : 1;
        MUINT32 AE_HST2_X_HI              : 7;
        MUINT32 rsv_15                    : 1;
        MUINT32 AE_HST2_Y_LOW             : 7;
        MUINT32 rsv_23                    : 1;
        MUINT32 AE_HST2_Y_HI              : 7;
        MUINT32 rsv_31                    : 1;
} ISP_CAM_AE_HST2_RNG_T;

typedef union {
    enum { MASK     = 0x7F7F7F7F };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_AE_HST2_RNG_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AE_HST2_RNG_T;

// CAM_AE_HST3_RNG CAM+0690H
typedef struct {
        MUINT32 AE_HST3_X_LOW             : 7;
        MUINT32 rsv_7                     : 1;
        MUINT32 AE_HST3_X_HI              : 7;
        MUINT32 rsv_15                    : 1;
        MUINT32 AE_HST3_Y_LOW             : 7;
        MUINT32 rsv_23                    : 1;
        MUINT32 AE_HST3_Y_HI              : 7;
        MUINT32 rsv_31                    : 1;
} ISP_CAM_AE_HST3_RNG_T;

typedef union {
    enum { MASK     = 0x7F7F7F7F };
    enum { DEFAULT  = 0x00000000 };
    typedef ISP_CAM_AE_HST3_RNG_T reg_t;
    reg_t   bits;
    MUINT32 val;
} ISP_NVRAM_AE_HST3_RNG_T;

//
typedef union {
    enum { COUNT = 2 };
    MUINT32 set[COUNT];
    struct {
        ISP_NVRAM_AE_RAWPREGAIN2_0_T    rawpregain2_0;
        ISP_NVRAM_AE_RAWPREGAIN2_1_T    rawpregain2_1;
    };
} ISP_NVRAM_AE_RAWPREGAIN2_T;

//
typedef union {
    enum { COUNT = 5 };
    MUINT32 set[COUNT];
    struct {
        ISP_CAM_AE_MATRIX_COEF0_T       matrix_coef0;
        ISP_CAM_AE_MATRIX_COEF1_T       matrix_coef1;
        ISP_CAM_AE_MATRIX_COEF2_T       matrix_coef2;
        ISP_CAM_AE_MATRIX_COEF3_T       matrix_coef3;
        ISP_CAM_AE_MATRIX_COEF4_T       matrix_coef4;
    };
} ISP_NVRAM_AE_MATRIX_COEF_T;

//////FLK
typedef struct {
    MUINT32 rsv_0                       : 17;
    MUINT32 FLK_EN                     : 1;
    MUINT32 rsv_1                       : 14;
} ISP_CAM_FLK_EN;
typedef struct {
    MUINT32 rsv_0                       : 17;
    MUINT32 FLK_EN_SET                     : 1;
    MUINT32 rsv_1                       : 14;
} ISP_CAM_CTL_EN1_SET;
typedef struct {
    MUINT32 rsv_0                       : 3;
    MUINT32 ESFKO_EN                     : 1;
    MUINT32 rsv_1                       : 28;
} ISP_CAM_CTL_DMA_EN;
typedef struct {
    MUINT32 rsv_0                       : 3;
    MUINT32 ESFKO_EN_SET                : 1;
    MUINT32 rsv_1                       : 28;
} ISP_CAM_CTL_DMA_EN_SET;
typedef struct {
    MUINT32 FLK_MODE                    : 1;
    MUINT32 rsv_1                       : 31;
} ISP_CAM_FLK_CON;
typedef struct {
    MUINT32 rsv_0                       : 20;
    MUINT32 ESFKO_DONE_EN                : 1;
    MUINT32 rsv_1                       : 11;
} ISP_CAM_CTL_DMA_INT;
typedef struct {
    MUINT32 rsv_0                       : 17;
    MUINT32 FLK_DON_EN                  : 1;
    MUINT32 rsv_1                       : 14;
} ISP_CAM_CTL_INT_EN;


};

#endif  //  _ISPIF_

