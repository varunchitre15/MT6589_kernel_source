

#ifndef __TVE_REG_H__
#define __TVE_REG_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct
{
    unsigned ENCON      : 1;
    unsigned CBON       : 1;
    unsigned SETUP      : 1;
    unsigned rsv_3      : 1;
    unsigned CLPSEL     : 1;
    unsigned CLPON      : 1;
    unsigned YLPON      : 1;
    unsigned CUPOF      : 1;
    unsigned YDEL       : 2;
    unsigned SYDEL      : 1;
    unsigned rsv_11     : 2;
    unsigned SLPOFF     : 1;
    unsigned BLKER      : 1;
    unsigned UVSWP      : 1;
    unsigned TVTYPE     : 2;
    unsigned rsv_18     : 2;
    unsigned CMP        : 1;
    unsigned rsv_21     : 11;
} TVE_REG_MODE, *PTVE_REG_MODE;


typedef struct
{
    unsigned USCALE     : 8;
    unsigned VSCALE     : 8;
    unsigned BLANK      : 4;
    unsigned rsv_20     : 12;
} TVE_REG_SCALE_CON, *PTVE_REG_SCALE_CON;


typedef struct
{
    unsigned rsv_0        : 6;
    unsigned ABIST        : 1;
    unsigned ABIST_12BIT  : 1;
    unsigned rsv_8        : 4;
    unsigned TEN_BIT      : 1;
    unsigned rsv_13       : 7;
    unsigned CLK_DIR      : 1;
    unsigned rsv_21       : 11;
} TVE_REG_DAC_CON, *PTVE_REG_DAC_CON;


typedef struct
{
    unsigned BRSTLVL    : 8;
    unsigned rsv_8      : 8;
    unsigned UPQINI     : 8;
    unsigned rsv_24     : 8;
} TVE_REG_BURST, *PTVE_REG_BURST;


typedef struct
{
    unsigned BFP1       : 9;
    unsigned rsv_9      : 7;
    unsigned BFP2       : 13;
    unsigned rsv_29     : 3;
} TVE_REG_FREQ, *PTVE_REG_FREQ;


typedef struct
{
    unsigned DOWN       : 11;
    unsigned rsv_11     : 5;
    unsigned UP         : 10;
    unsigned rsv_25     : 6;
} TVE_REG_SLEW, *PTVE_REG_SLEW;


typedef struct
{
    unsigned rsv_0      : 16;
    unsigned YLPF10     : 6;
    unsigned rsv_22     : 2;
    unsigned YLPF11     : 6;
    unsigned rsv_30     : 2;
} TVE_REG_YLPFC, *PTVE_REG_YLPFC;


typedef struct
{
    unsigned YLPF12     : 6;
    unsigned rsv_6      : 2;
    unsigned YLPF13     : 6;
    unsigned rsv_14     : 2;
    unsigned YLPF14     : 7;
    unsigned rsv_23     : 1;
    unsigned YLPF15     : 7;
    unsigned rsv_31     : 1;
} TVE_REG_YLPFD, *PTVE_REG_YLPFD;


typedef struct
{
    unsigned YLPF16     : 8;
    unsigned YLPF17     : 8;
    unsigned YLPF18     : 8;
    unsigned YLPF19     : 8;
} TVE_REG_YLPFE, *PTVE_REG_YLPFE;


typedef struct
{
    unsigned CLPF0      : 6;
    unsigned rsv_6      : 2;
    unsigned CLPF1      : 6;
    unsigned rsv_14     : 2;
    unsigned CLPF2      : 6;
    unsigned rsv_22     : 2;
    unsigned CLPF3      : 6;
    unsigned rsv_30     : 2;
} TVE_REG_CLPFA, *PTVE_REG_CLPFA;


typedef struct
{
    unsigned CLPF4      : 6;
    unsigned rsv_6      : 2;
    unsigned CLPF5      : 6;
    unsigned rsv_14     : 2;
    unsigned CLPF6      : 6;
    unsigned rsv_22     : 2;
    unsigned CLPF7      : 6;
    unsigned rsv_30     : 2;
} TVE_REG_CLPFB, *PTVE_REG_CLPFB;


typedef struct
{
    unsigned CLPF8      : 6;
    unsigned rsv_6      : 2;
    unsigned CLPF9      : 6;
    unsigned rsv_14     : 18;
} TVE_REG_CLPFC, *PTVE_REG_CLPFC;


typedef struct
{
    unsigned rsv_0      : 16;
    unsigned GAMMA0     : 12;
    unsigned rsv_28     : 4;
} TVE_REG_GAMMAA, *PTVE_REG_GAMMAA;


typedef struct
{
    unsigned GAMMA1     : 12;
    unsigned rsv_12     : 4;
    unsigned GAMMA2     : 12;
    unsigned rsv_28     : 4;
} TVE_REG_GAMMAB, *PTVE_REG_GAMMAB;


typedef struct
{
    unsigned GAMMA3     : 12;
    unsigned rsv_12     : 4;
    unsigned GAMMA4     : 12;
    unsigned rsv_28     : 4;
} TVE_REG_GAMMAC, *PTVE_REG_GAMMAC;


typedef struct
{
    unsigned GAMMA5     : 12;
    unsigned rsv_12     : 4;
    unsigned GAMMA6     : 12;
    unsigned rsv_28     : 4;
} TVE_REG_GAMMAD, *PTVE_REG_GAMMAD;


typedef struct
{
    unsigned GAMMA7     : 12;
    unsigned rsv_12     : 4;
    unsigned GAMMA8     : 12;
    unsigned rsv_28     : 4;
} TVE_REG_GAMMAE, *PTVE_REG_GAMMAE;


typedef struct
{
    unsigned INT          : 1;
    unsigned rsv_1        : 23;
    unsigned CMP_EN       : 1;
    unsigned PLUG_ACT     : 1;
    unsigned rsv_26       : 5;
    unsigned COMPRESS     : 1;
} TVE_REG_PLUG, *PTVE_REG_PLUG;


typedef struct
{
    TVE_REG_MODE        MODE;           // 0000
    TVE_REG_SCALE_CON   SCALE_CON;      // 0004
    TVE_REG_DAC_CON     DAC_CON;        // 0008
    TVE_REG_BURST       BURST;          // 000C
    TVE_REG_FREQ        FREQ;           // 0010
    TVE_REG_SLEW        SLEW;           // 0014
    UINT32              rsv_0018[4];    // 0018..0024

    TVE_REG_YLPFC       YLPFC;          // 0028
    TVE_REG_YLPFD       YLPFD;          // 002C
    TVE_REG_YLPFE       YLPFE;          // 0030
    TVE_REG_CLPFA       CLPFA;          // 0034
    TVE_REG_CLPFB       CLPFB;          // 0038
    TVE_REG_CLPFC       CLPFC;          // 003C

    TVE_REG_GAMMAA      GAMMAA;         // 0040
    TVE_REG_GAMMAB      GAMMAB;         // 0044
    TVE_REG_GAMMAC      GAMMAC;         // 0048
    TVE_REG_GAMMAD      GAMMAD;         // 004C
    TVE_REG_GAMMAE      GAMMAE;         // 0050

    UINT32              CMPCODE;        // 0054
    UINT32              rsv_0054[2];    // 0058..005C

    UINT32              RESET;          // 0060
    UINT32              rsv_0064[7];    // 0064..007C

    TVE_REG_PLUG        PLUG;           // 0080
} volatile TVE_REGS, *PTVE_REGS;

STATIC_ASSERT(0x0028 == offsetof(TVE_REGS, YLPFC));
STATIC_ASSERT(0x0050 == offsetof(TVE_REGS, GAMMAE));
STATIC_ASSERT(0x0080 == offsetof(TVE_REGS, PLUG));
STATIC_ASSERT(0x0084 == sizeof(TVE_REGS));

extern PTVE_REGS const TVE_REG;

#ifdef __cplusplus
}
#endif

#endif // __TVE_REG_H__

