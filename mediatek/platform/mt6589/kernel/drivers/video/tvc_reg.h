

#ifndef __TVC_REG_H__
#define __TVC_REG_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct
{
    unsigned DATA_FMT      : 2;
    unsigned rsv_2         : 1;
    unsigned FAST_MODE     : 1;
    unsigned DMAIF_GULTRA  : 1;
    unsigned BURST_TYPE    : 2;    
    unsigned PFH           : 1;
    unsigned RESZ_GULTRA   : 1;
    unsigned PFH_GULTRA    : 1;
    unsigned BYPASS_RESZ   : 1;
    unsigned R2Y_RND       : 1;
    unsigned DMAIF_GPROT   : 1;
    unsigned CHECK_IRQ     : 1;
    unsigned FIELD_REQ     : 1;
    unsigned OVRUN_IRQ     : 1;
    unsigned rsv_16        : 16;
} TVC_REG_CON, *PTVC_REG_CON;


typedef struct
{
    unsigned LINE_OFFSET    : 12;
    unsigned rsv_12         : 4;
    unsigned LINE_BUF_LIMIT : 6;
    unsigned rsv_22         : 10;
} TVC_REG_LINE_OFFSET, *PTVC_REG_LINE_OFFSET;


typedef struct
{
    unsigned H_RESIDUAL : 12;
    unsigned rsv_12     : 4;
    unsigned V_RESIDUAL : 12;
    unsigned rsv_28     : 4;
} TVC_REG_RESIDUAL, *PTVC_REG_RESIDUAL;


typedef struct
{
    unsigned VSS    : 1;
    unsigned SAEN   : 1;
    unsigned rsv_2  : 14;
    unsigned WMSZ   : 10;
    unsigned rsv_26 : 6;
} TVC_REG_FRCFG, *PTVC_REG_FRCFG;


typedef struct
{
    unsigned LINE   : 10;
    unsigned rsv_10 : 6;
    unsigned PIXEL  : 10;
    unsigned rsv_26 : 6;
} TVC_REG_POINT, *PTVC_REG_POINT;


typedef struct
{
    unsigned IRQ0  : 1;
    unsigned IRQ1  : 1;
    unsigned IRQ2  : 1;
    unsigned rsv_3 : 29;
} TVC_REG_IRQ_STS, *PTVC_REG_IRQ_STS;


typedef struct
{
    unsigned REG_RDY : 1;
    unsigned ACT_RD  : 1;
    unsigned ACT_WR  : 1;
    unsigned rsv_3   : 29;
} TVC_REG_UPDATE, *PTVC_REG_UPDATE;


typedef struct
{
    unsigned Y      : 8;
    unsigned U      : 8;
    unsigned V      : 8;
    unsigned rsv_24 : 8;
} TVC_REG_BG_COLOR, *PTVC_REG_BG_COLOR;


typedef struct
{
    unsigned BCLK_DELAY_NUM  : 4;
    unsigned TVCLK_DELAY_NUM : 4;
    unsigned rsv_8           : 24;
} TVC_REG_ASYNC_CTRL, *PTVC_REG_ASYNC_CTRL;


typedef struct
{
    unsigned TV_LB_CNT         : 2;
    unsigned TVC_OVR           : 1;
    unsigned rsv_3             : 1;
    unsigned DMAIF_BUF_CNT     : 7;
    unsigned rsv_11            : 5;
    unsigned TV_LINE           : 10;
    unsigned FIELD             : 1;
    unsigned TVE_REQ           : 1;
    unsigned H_PRI             : 1;
    unsigned TVC_GREQ          : 1;
    unsigned PFH_REQ           : 1;
    unsigned RESZ_GREQ         : 1;
} TVC_REG_STATUS0, *PTVC_REG_STATUS0;


typedef struct
{
    unsigned TVDAC_IN     : 12;
    unsigned rsv_12       : 4;
    unsigned DMAIF_GRDATA : 16;
} TVC_REG_STATUS1, *PTVC_REG_STATUS1;


typedef struct
{
    UINT32      ENABLE;                 // 0000
    UINT32      RESET;                  // 0004
    TVC_REG_CON CONTROL;                // 0008

    UINT32 SRC_Y_ADDR;                  // 000C
    UINT32 SRC_U_ADDR;                  // 0010
    UINT32 SRC_V_ADDR;                  // 0014

    TVC_REG_LINE_OFFSET LINE_OFFSET;    // 0018

    UINT32 PFH_DMA_ADDR;                // 001C
    UINT32 PFH_DMA_FIFO_LEN;            // 0020

    UINT32 SRC_WIDTH;                   // 0024
    UINT32 SRC_HEIGHT;                  // 0028
    UINT32 TAR_WIDTH;                   // 002C
    UINT32 TAR_HEIGHT;                  // 0030

    UINT32 HRATIO;                      // 0034
    UINT32 VRATIO;                      // 0038
    TVC_REG_RESIDUAL RESIDUAL;          // 003C

    UINT32        RESZ_ADDR;            // 0040
    TVC_REG_FRCFG FINE_RSZ_CFG;         // 0044

    TVC_REG_POINT START_POINT;          // 0048
    TVC_REG_POINT STOP_POINT;           // 004C

    TVC_REG_IRQ_STS IRQ_STATUS;         // 0050
    TVC_REG_UPDATE  REG_UPDATE;         // 0054

    TVC_REG_BG_COLOR BG_COLOR;          // 0058

    UINT32 CHECK_LINE;                  // 005C
    
    TVC_REG_ASYNC_CTRL ASYNC_CTRL;      // 0060
    UINT32 rsv_0064[3];                 // 0064..006C
    
    TVC_REG_STATUS0 STATUS0;            // 0070
    TVC_REG_STATUS1 STATUS1;            // 0074
} volatile TVC_REGS, *PTVC_REGS;


STATIC_ASSERT(0x0020 == offsetof(TVC_REGS, PFH_DMA_FIFO_LEN));
STATIC_ASSERT(0x0044 == offsetof(TVC_REGS, FINE_RSZ_CFG));
STATIC_ASSERT(0x0060 == offsetof(TVC_REGS, ASYNC_CTRL));
STATIC_ASSERT(0x0078 == sizeof(TVC_REGS));

extern PTVC_REGS const TVC_REG;

#ifdef __cplusplus
}
#endif

#endif // __TVC_REG_H__

