#ifndef __PMIC_WRAP_REGS_H__
#define __PMIC_WRAP_REGS_H__
#include <mach/mt_reg_base.h>
#include "mach/sync_write.h"

//#define PMIC_WRAP_BASE            (0xF000F000)
//#define PERI_PWRAP_BRIDGE_BASE    (0xF1017000)
#define DEW_BASE                  (0xBC00)

//#define TOPRGU_BASE             0xF0000000
//#define INFRACFG_BASE           0xF0001000
//#define PERICFG_BASE            0xF0003000

//-------macro for pmic register--------------------------------
#define PMIC_BASE (0x0000)
#define PMIC_WRP_CKPDN            (PMIC_BASE+0x011A) //0x0056
#define PMIC_WRP_RST_CON          (PMIC_BASE+0x0120)//0x005C
#define PMIC_TOP_CKCON2           (PMIC_BASE+0x012A)
//-------macro for timeout setting--------------------------------
#define TIMEOUT_RESET	    0xFF //us
#define TIMEOUT_READ	    0xFF //us
#define TIMEOUT_WAIT_IDLE	0xFF //us
//-------macro for clock gating--------------------------------
//INFRACFG_AO_BASE = 0x10001000;
#define INFRA_GLOBALCON_PDN0	(INFRACFG_BASE+0x40)
#define INFRA_GLOBALCON_PDN1    (INFRACFG_BASE+0X44)
//PERICFG_BASE = 0X10003000;
#define PERI_GLOBALCON_PDN1_SET (PERICFG_BASE + 0x0c)
#define PERI_GLOBALCON_PDN1_CLR (PERICFG_BASE + 0x14)
#define PERI_GLOBALCON_PDN1_STA (PERICFG_BASE + 0x1c)

//TOPRGU_BASE = 0X10000000;
#define CLK_PDN_SET            (TOPRGU_BASE + 0x170)
#define CLK_PDN_CLR            (TOPRGU_BASE + 0x174)
#define CLK_PDN_STA            (TOPRGU_BASE + 0x178)

struct pmic_wrap_obj {
    atomic_t        ref;
    dev_t           devno;
    struct class    *cls;
    struct device   *dev;
    struct cdev     chrdev;
    spinlock_t      spin_lock;
    spinlock_t      spin_lock_isr;
    struct miscdevice *misc;
#ifdef CONFIG_MTK_LDVT_PMIC_WRAP
        spinlock_t      spin_lock_dvt;
    spinlock_t      spin_lock_wacs0;
    spinlock_t      spin_lock_wacs1;
    spinlock_t      spin_lock_wacs3;
    spinlock_t      spin_lock_wacs4;
   	void			(*complete)(void *context);
	void			*context;
#endif
};

/*-----start-- global variable-------------------------------------------------*/
static struct pmic_wrap_obj pmic_wrap_dat = {
    .ref  = ATOMIC_INIT(0),
    .cls  = NULL,
    .dev  = NULL,
    .spin_lock = __SPIN_LOCK_UNLOCKED(lock),
    .spin_lock_isr = __SPIN_LOCK_UNLOCKED(lock),
    .misc = NULL,
#ifdef CONFIG_MTK_LDVT_PMIC_WRAP
    .spin_lock_dvt= __SPIN_LOCK_UNLOCKED(lock),
    .spin_lock_wacs0= __SPIN_LOCK_UNLOCKED(lock),
    .spin_lock_wacs1= __SPIN_LOCK_UNLOCKED(lock),
    .spin_lock_wacs3= __SPIN_LOCK_UNLOCKED(lock),
    .spin_lock_wacs4= __SPIN_LOCK_UNLOCKED(lock),
#endif
};
 static struct pmic_wrap_obj *g_pmic_wrap_obj = &pmic_wrap_dat;
/*------end-- global variable-------------------------------------------------*/


//-------macro for spi clock config--------------------------------
#define CLK_CFG_8                       (TOPRGU_BASE+0x164) //6589
#define WDT_SWSYSRST                    (TOPRGU_BASE+0x18) //6589


#define PMIC_WRAP_MUX_SEL               (PMIC_WRAP_BASE+0x0)
#define PMIC_WRAP_WRAP_EN               (PMIC_WRAP_BASE+0x4)
#define PMIC_WRAP_DIO_EN                (PMIC_WRAP_BASE+0x8)
#define PMIC_WRAP_SIDLY                 (PMIC_WRAP_BASE+0xC)
#define PMIC_WRAP_CSHEXT                (PMIC_WRAP_BASE+0x10)
#define PMIC_WRAP_CSHEXT_WRITE          (PMIC_WRAP_BASE+0x14)
#define PMIC_WRAP_CSHEXT_READ           (PMIC_WRAP_BASE+0x18)
#define PMIC_WRAP_CSLEXT_START          (PMIC_WRAP_BASE+0x1C)
#define PMIC_WRAP_CSLEXT_END            (PMIC_WRAP_BASE+0x20)
#define PMIC_WRAP_STAUPD_PRD            (PMIC_WRAP_BASE+0x24)
#define PMIC_WRAP_STAUPD_GRPEN          (PMIC_WRAP_BASE+0x28)
#define PMIC_WRAP_STAUPD_MAN_TRIG       (PMIC_WRAP_BASE+0x2C)
#define PMIC_WRAP_STAUPD_STA            (PMIC_WRAP_BASE+0x30)
#define PMIC_WRAP_EVENT_IN_EN           (PMIC_WRAP_BASE+0x34)
#define PMIC_WRAP_EVENT_DST_EN          (PMIC_WRAP_BASE+0x38)
#define PMIC_WRAP_WRAP_STA              (PMIC_WRAP_BASE+0x3C)
#define PMIC_WRAP_RRARB_INIT            (PMIC_WRAP_BASE+0x40)
#define PMIC_WRAP_RRARB_EN              (PMIC_WRAP_BASE+0x44)
#define PMIC_WRAP_RRARB_STA0            (PMIC_WRAP_BASE+0x48)
#define PMIC_WRAP_RRARB_STA1            (PMIC_WRAP_BASE+0x4C)
#define PMIC_WRAP_HARB_INIT             (PMIC_WRAP_BASE+0x50)
#define PMIC_WRAP_HARB_HPRIO            (PMIC_WRAP_BASE+0x54)
#define PMIC_WRAP_HIPRIO_ARB_EN         (PMIC_WRAP_BASE+0x58)
#define PMIC_WRAP_HARB_STA0             (PMIC_WRAP_BASE+0x5C)
#define PMIC_WRAP_HARB_STA1             (PMIC_WRAP_BASE+0x60)
#define PMIC_WRAP_MAN_EN                (PMIC_WRAP_BASE+0x64)
#define PMIC_WRAP_MAN_CMD               (PMIC_WRAP_BASE+0x68)
#define PMIC_WRAP_MAN_RDATA             (PMIC_WRAP_BASE+0x6C)
#define PMIC_WRAP_MAN_VLDCLR            (PMIC_WRAP_BASE+0x70)
#define PMIC_WRAP_WACS0_EN              (PMIC_WRAP_BASE+0x74)
#define PMIC_WRAP_INIT_DONE0            (PMIC_WRAP_BASE+0x78)
#define PMIC_WRAP_WACS0_CMD             (PMIC_WRAP_BASE+0x7C)
#define PMIC_WRAP_WACS0_RDATA           (PMIC_WRAP_BASE+0x80)
#define PMIC_WRAP_WACS0_VLDCLR          (PMIC_WRAP_BASE+0x84)
#define PMIC_WRAP_WACS1_EN              (PMIC_WRAP_BASE+0x88)
#define PMIC_WRAP_INIT_DONE1            (PMIC_WRAP_BASE+0x8C)
#define PMIC_WRAP_WACS1_CMD             (PMIC_WRAP_BASE+0x90)
#define PMIC_WRAP_WACS1_RDATA           (PMIC_WRAP_BASE+0x94)
#define PMIC_WRAP_WACS1_VLDCLR          (PMIC_WRAP_BASE+0x98)
#define PMIC_WRAP_WACS2_EN              (PMIC_WRAP_BASE+0x9C)
#define PMIC_WRAP_INIT_DONE2            (PMIC_WRAP_BASE+0xA0)
#define PMIC_WRAP_WACS2_CMD             (PMIC_WRAP_BASE+0xA4)
#define PMIC_WRAP_WACS2_RDATA           (PMIC_WRAP_BASE+0xA8)
#define PMIC_WRAP_WACS2_VLDCLR          (PMIC_WRAP_BASE+0xAC)
#define PMIC_WRAP_INT_EN                (PMIC_WRAP_BASE+0xB0)
#define PMIC_WRAP_INT_FLG_RAW           (PMIC_WRAP_BASE+0xB4)
#define PMIC_WRAP_INT_FLG               (PMIC_WRAP_BASE+0xB8)
#define PMIC_WRAP_INT_CLR               (PMIC_WRAP_BASE+0xBC)
#define PMIC_WRAP_SIG_ADR               (PMIC_WRAP_BASE+0xC0)
#define PMIC_WRAP_SIG_MODE              (PMIC_WRAP_BASE+0xC4)
#define PMIC_WRAP_SIG_VALUE             (PMIC_WRAP_BASE+0xC8)
#define PMIC_WRAP_SIG_ERRVAL            (PMIC_WRAP_BASE+0xCC)
#define PMIC_WRAP_CRC_EN                (PMIC_WRAP_BASE+0xD0)
#define PMIC_WRAP_EVENT_STA             (PMIC_WRAP_BASE+0xD4)
#define PMIC_WRAP_EVENT_STACLR          (PMIC_WRAP_BASE+0xD8)
#define PMIC_WRAP_TIMER_EN              (PMIC_WRAP_BASE+0xDC)
#define PMIC_WRAP_TIMER_STA             (PMIC_WRAP_BASE+0xE0)
#define PMIC_WRAP_WDT_UNIT              (PMIC_WRAP_BASE+0xE4)
#define PMIC_WRAP_WDT_SRC_EN            (PMIC_WRAP_BASE+0xE8)
#define PMIC_WRAP_WDT_FLG               (PMIC_WRAP_BASE+0xEC)
#define PMIC_WRAP_DEBUG_INT_SEL         (PMIC_WRAP_BASE+0xF0)
#define PMIC_WRAP_DVFS_ADR0             (PMIC_WRAP_BASE+0xF4)
#define PMIC_WRAP_DVFS_WDATA0           (PMIC_WRAP_BASE+0xF8)
#define PMIC_WRAP_DVFS_ADR1             (PMIC_WRAP_BASE+0xFC)
#define PMIC_WRAP_DVFS_WDATA1           (PMIC_WRAP_BASE+0x100)
#define PMIC_WRAP_DVFS_ADR2             (PMIC_WRAP_BASE+0x104)
#define PMIC_WRAP_DVFS_WDATA2           (PMIC_WRAP_BASE+0x108)
#define PMIC_WRAP_DVFS_ADR3             (PMIC_WRAP_BASE+0x10C)
#define PMIC_WRAP_DVFS_WDATA3           (PMIC_WRAP_BASE+0x110)
#define PMIC_WRAP_DVFS_ADR4             (PMIC_WRAP_BASE+0x114)
#define PMIC_WRAP_DVFS_WDATA4           (PMIC_WRAP_BASE+0x118)
#define PMIC_WRAP_DVFS_ADR5             (PMIC_WRAP_BASE+0x11C)
#define PMIC_WRAP_DVFS_WDATA5           (PMIC_WRAP_BASE+0x120)
#define PMIC_WRAP_DVFS_ADR6             (PMIC_WRAP_BASE+0x124)
#define PMIC_WRAP_DVFS_WDATA6           (PMIC_WRAP_BASE+0x128)
#define PMIC_WRAP_DVFS_ADR7             (PMIC_WRAP_BASE+0x12C)
#define PMIC_WRAP_DVFS_WDATA7           (PMIC_WRAP_BASE+0x130)
#define PMIC_WRAP_CIPHER_KEY_SEL        (PMIC_WRAP_BASE+0x134)
#define PMIC_WRAP_CIPHER_IV_SEL         (PMIC_WRAP_BASE+0x138)
#define PMIC_WRAP_CIPHER_LOAD           (PMIC_WRAP_BASE+0x13C)
#define PMIC_WRAP_CIPHER_START          (PMIC_WRAP_BASE+0x140)
#define PMIC_WRAP_CIPHER_RDY            (PMIC_WRAP_BASE+0x144)
#define PMIC_WRAP_CIPHER_MODE           (PMIC_WRAP_BASE+0x148)
#define PMIC_WRAP_CIPHER_SWRST          (PMIC_WRAP_BASE+0x14C)
#define PMIC_WRAP_CIPHER_IV0            (PMIC_WRAP_BASE+0x150)
#define PMIC_WRAP_CIPHER_IV1            (PMIC_WRAP_BASE+0x154)
#define PMIC_WRAP_CIPHER_IV2            (PMIC_WRAP_BASE+0x158)
#define PMIC_WRAP_DCM_EN                (PMIC_WRAP_BASE+0x15C)
#define PMIC_WRAP_DCM_DBC_PRD           (PMIC_WRAP_BASE+0x160)


//-----macro for wrapper  regsister--------------------------------------------------------
#define GET_STAUPD_DLE_CNT(x)        ((x>>0)  & 0x00000007)
#define GET_STAUPD_ALE_CNT(x)        ((x>>3)  & 0x00000007)
#define GET_STAUPD_FSM(x)            ((x>>6)  & 0x00000007)
#define GET_WRAP_CH_DLE_RESTCNT(x)   ((x>>0)  & 0x00000007)
#define GET_WRAP_CH_ALE_RESTCNT(x)   ((x>>3)  & 0x00000003)
#define GET_WRAP_AG_DLE_RESTCNT(x)   ((x>>5)  & 0x00000003)
#define GET_WRAP_CH_W(x)             ((x>>7)  & 0x00000001)
#define GET_WRAP_CH_REQ(x)           ((x>>8)  & 0x00000001)
#define GET_AG_WRAP_W(x)             ((x>>9)  & 0x00000001)
#define GET_AG_WRAP_REQ(x)           ((x>>10) & 0x00000001)
#define GET_WRAP_FSM(x)              ((x>>11) & 0x0000000f)
#define GET_HARB_WRAP_WDATA(x)       ((x>>0)  & 0x0000ffff)
#define GET_HARB_WRAP_ADR(x)         ((x>>16) & 0x00007fff)
#define GET_HARB_WRAP_REQ(x)         ((x>>31) & 0x00000001)
#define GET_HARB_DLE_EMPTY(x)        ((x>>0)  & 0x00000001)
#define GET_HARB_DLE_FULL(x)         ((x>>1)  & 0x00000001)
#define GET_HARB_VLD(x)              ((x>>2)  & 0x00000001)
#define GET_HARB_DLE_OWN(x)          ((x>>3)  & 0x0000000f)
#define GET_HARB_OWN(x)              ((x>>7)  & 0x0000000f)
#define GET_HARB_DLE_RESTCNT(x)      ((x>>11) & 0x0000000f)
#define GET_AG_HARB_REQ(x)           ((x>>15) & 0x000001ff)
#define GET_HARB_WRAP_W(x)           ((x>>24) & 0x00000001)
#define GET_HARB_WRAP_REQ0(x)        ((x>>25) & 0x00000001)
#define GET_SPI_WDATA(x)             ((x>>0)  & 0x000000ff)
#define GET_SPI_OP(x)                ((x>>8)  & 0x0000001f)
#define GET_SPI_W(x)                 ((x>>13) & 0x00000001)
#define GET_MAN_RDATA(x)             ((x>>0)  & 0x000000ff)
#define GET_MAN_FSM(x)               ((x>>8)  & 0x00000007)
#define GET_MAN_REQ(x)               ((x>>11) & 0x00000001)

#define GET_WACS0_WDATA(x)           ((x>>0)  & 0x0000ffff)
#define GET_WACS0_ADR(x)             ((x>>16) & 0x00007fff)
#define GET_WACS0_WRITE(x)           ((x>>31) & 0x00000001)
#define GET_WACS0_RDATA(x)           ((x>>0)  & 0x0000ffff)
#define GET_WACS0_FSM(x)             ((x>>16) & 0x00000007)
#define GET_WACS0_REQ(x)             ((x>>19) & 0x00000001)
#define GET_SYNC_IDLE0(x)            ((x>>20) & 0x00000001)
#define GET_INIT_DONE0(x)            ((x>>21) & 0x00000001)

//macro for staupd sta fsm
#define STAUPD_FSM_IDLE               (0x00)
#define STAUPD_FSM_REQ                (0x02)
#define STAUPD_FSM_WFDLE              (0x04) //wait for dle,wait for read data done,

//macro for WRAP_STA  FSM
//#define WRAP_STA_FSM_IDLE               (0x00)
//#define WRAP_STA_IDLE               (0x00)

//macro for MAN_RDATA  FSM
#define MAN_FSM_NO_REQ             (0x00)
#define MAN_FSM_IDLE               (0x00)
#define MAN_FSM_REQ                (0x02)
#define MAN_FSM_WFDLE              (0x04) //wait for dle,wait for read data done,
#define MAN_FSM_WFVLDCLR           (0x06)

//macro for WACS_FSM
#define WACS_FSM_IDLE               (0x00)
#define WACS_FSM_REQ                (0x02)
#define WACS_FSM_WFDLE              (0x04) //wait for dle,wait for read data done,
#define WACS_FSM_WFVLDCLR           (0x06) //finish read data , wait for valid flag clearing
#define WACS_INIT_DONE              (0x01)
#define WACS_SYNC_IDLE              (0x01)
#define WACS_SYNC_BUSY              (0x00)

#if 0  //the same to wacs0
#define GET_WACS1_WDATA(x)           ((x>>0)  & 0x0000ffff)
#define GET_WACS1_ADR(x)             ((x>>16) & 0x00007fff)
#define GET_WACS1_WRITE(x)           ((x>>31) & 0x00000001)
#define GET_WACS1_RDATA(x)           ((x>>0)  & 0x0000ffff)
#define GET_WACS1_FSM(x)             ((x>>16) & 0x00000007)
#define GET_WACS1_REQ(x)             ((x>>19) & 0x00000001)
#define GET_SYNC_IDLE1(x)            ((x>>20) & 0x00000001)
#define GET_INIT_DONE1(x)            ((x>>21) & 0x00000001)

#define GET_WACS2_WDATA(x)           ((x>>0)  & 0x0000ffff)
#define GET_WACS2_ADR(x)             ((x>>16) & 0x00007fff)
#define GET_WACS2_WRITE(x)           ((x>>31) & 0x00000001)
#define GET_WACS2_RDATA(x)           ((x>>0)  & 0x0000ffff)
#define GET_WACS2_FSM(x)             ((x>>16) & 0x00000007)
#define GET_WACS2_REQ(x)             ((x>>19) & 0x00000001)


#define GET_SYNC_IDLE2(x)            ((x>>20) & 0x00000001)
#define GET_INIT_DONE2(x)            ((x>>21) & 0x00000001)
#endif

//-----macro for wrapper bridge regsister--------------------------------------------------------
// APB Module peri_pwrap_bridge

#define PERI_PWRAP_BRIDGE_IARB_INIT         (PERI_PWRAP_BRIDGE_BASE+0x0)
#define PERI_PWRAP_BRIDGE_IORD_ARB_EN       (PERI_PWRAP_BRIDGE_BASE+0x4)
#define PERI_PWRAP_BRIDGE_IARB_STA0         (PERI_PWRAP_BRIDGE_BASE+0x8)
#define PERI_PWRAP_BRIDGE_IARB_STA1         (PERI_PWRAP_BRIDGE_BASE+0xC)
#define PERI_PWRAP_BRIDGE_WACS3_EN          (PERI_PWRAP_BRIDGE_BASE+0x10)
#define PERI_PWRAP_BRIDGE_INIT_DONE3        (PERI_PWRAP_BRIDGE_BASE+0x14)
#define PERI_PWRAP_BRIDGE_WACS3_CMD         (PERI_PWRAP_BRIDGE_BASE+0x18)
#define PERI_PWRAP_BRIDGE_WACS3_RDATA       (PERI_PWRAP_BRIDGE_BASE+0x1C)
#define PERI_PWRAP_BRIDGE_WACS3_VLDCLR      (PERI_PWRAP_BRIDGE_BASE+0x20)
#define PERI_PWRAP_BRIDGE_WACS4_EN          (PERI_PWRAP_BRIDGE_BASE+0x24)
#define PERI_PWRAP_BRIDGE_INIT_DONE4        (PERI_PWRAP_BRIDGE_BASE+0x28)
#define PERI_PWRAP_BRIDGE_WACS4_CMD         (PERI_PWRAP_BRIDGE_BASE+0x2C)
#define PERI_PWRAP_BRIDGE_WACS4_RDATA       (PERI_PWRAP_BRIDGE_BASE+0x30)
#define PERI_PWRAP_BRIDGE_WACS4_VLDCLR      (PERI_PWRAP_BRIDGE_BASE+0x34)
#define PERI_PWRAP_BRIDGE_INT_EN            (PERI_PWRAP_BRIDGE_BASE+0x38)
#define PERI_PWRAP_BRIDGE_INT_FLG_RAW       (PERI_PWRAP_BRIDGE_BASE+0x3C)
#define PERI_PWRAP_BRIDGE_INT_FLG           (PERI_PWRAP_BRIDGE_BASE+0x40)
#define PERI_PWRAP_BRIDGE_INT_CLR           (PERI_PWRAP_BRIDGE_BASE+0x44)
#define PERI_PWRAP_BRIDGE_TIMER_EN          (PERI_PWRAP_BRIDGE_BASE+0x48)
#define PERI_PWRAP_BRIDGE_TIMER_STA         (PERI_PWRAP_BRIDGE_BASE+0x4C)
#define PERI_PWRAP_BRIDGE_WDT_UNIT          (PERI_PWRAP_BRIDGE_BASE+0x50)
#define PERI_PWRAP_BRIDGE_WDT_SRC_EN        (PERI_PWRAP_BRIDGE_BASE+0x54)
#define PERI_PWRAP_BRIDGE_WDT_FLG           (PERI_PWRAP_BRIDGE_BASE+0x58)
#define PERI_PWRAP_BRIDGE_DEBUG_INT_SEL     (PERI_PWRAP_BRIDGE_BASE+0x5C)

//-----macro for wrapper bridge regsister--------------------------------------------------------

#define GET_WACS3_RDATA(x)           ((x>>0)  & 0x0000ffff)
#define GET_WACS3_FSM(x)             ((x>>16) & 0x00000007)
#define GET_WACS3_REQ(x)             ((x>>19) & 0x00000001)
#define GET_SYNC_IDLE3(x)            ((x>>20) & 0x00000001)
#define GET_INIT_DONE3(x)            ((x>>21) & 0x00000001)
#define GET_WACS4_RDATA(x)           ((x>>0)  & 0x0000ffff)
#define GET_WACS4_FSM(x)             ((x>>16) & 0x00000007)
#define GET_WACS4_REQ(x)             ((x>>19) & 0x00000001)
#define GET_SYNC_IDLE4(x)            ((x>>20) & 0x00000001)
#define GET_INIT_DONE4(x)            ((x>>21) & 0x00000001)


//-----macro for dewrapper regsister--------------------------------------------------------
#define DEW_EVENT_OUT_EN   (DEW_BASE+0x0)
#define DEW_DIO_EN         (DEW_BASE+0x2)
#define DEW_EVENT_SRC_EN   (DEW_BASE+0x4)
#define DEW_EVENT_SRC      (DEW_BASE+0x6)
#define DEW_EVENT_FLAG     (DEW_BASE+0x8)
#define DEW_READ_TEST      (DEW_BASE+0xA)
#define DEW_WRITE_TEST     (DEW_BASE+0xC)
#define DEW_CRC_EN         (DEW_BASE+0xE)
#define DEW_CRC_VAL        (DEW_BASE+0x10)
#define DEW_MON_GRP_SEL    (DEW_BASE+0x12)
#define DEW_MON_FLAG_SEL   (DEW_BASE+0x14)
#define DEW_EVENT_TEST     (DEW_BASE+0x16)
#define DEW_CIPHER_KEY_SEL (DEW_BASE+0x18)
#define DEW_CIPHER_IV_SEL  (DEW_BASE+0x1A)
#define DEW_CIPHER_LOAD    (DEW_BASE+0x1C)
#define DEW_CIPHER_START   (DEW_BASE+0x1E)
#define DEW_CIPHER_RDY     (DEW_BASE+0x20)
#define DEW_CIPHER_MODE    (DEW_BASE+0x22)
#define DEW_CIPHER_SWRST   (DEW_BASE+0x24)
#define DEW_CIPHER_IV0     (DEW_BASE+0x26)
#define DEW_CIPHER_IV1     (DEW_BASE+0x28)
#define DEW_CIPHER_IV2     (DEW_BASE+0x2A)
#define DEW_CIPHER_IV3     (DEW_BASE+0x2C)
#define DEW_CIPHER_IV4     (DEW_BASE+0x2E)
#define DEW_CIPHER_IV5     (DEW_BASE+0x30)
//-----macro for dewrapper defaule value-------------------------------------------------------
#define DEFAULT_VALUE_READ_TEST      0x5aa5
#define WRITE_TEST_VALUE             0x5678

//-----macro for manual commnd --------------------------------------------------------
#define OP_WR    (0x1)
#define OP_CSH   (0x0)
#define OP_CSL   (0x1)
#define OP_OUTS  (0x8)
#define OP_OUTD  (0x9)
#define OP_INS   (0xC)
#define OP_IND   (0xD)

//-----macro for read/write register --------------------------------------------------------

//#define WRAP_RD32(addr)            (*(volatile U32 *)(addr))
//#define WRAP_WR32(addr,data)       ((*(volatile U32 *)(addr)) = (U32)data)

//#define WRAP_SET_BIT(BS,REG)       ((*(volatile U32*)(REG)) |= (U32)(BS))
//#define WRAP_CLR_BIT(BS,REG)       ((*(volatile U32*)(REG)) &= ~((U32)(BS)))

#define WRAP_RD32(addr)            __raw_readl(addr)
#define WRAP_WR32(addr,val)        mt65xx_reg_sync_writel((val), (addr))

#define WRAP_SET_BIT(BS,REG)       mt65xx_reg_sync_writel((__raw_readl(REG) | (U32)(BS)), (REG))
#define WRAP_CLR_BIT(BS,REG)       mt65xx_reg_sync_writel((__raw_readl(REG) & (~(U32)(BS))), (REG))

//-----------------soft reset --------------------------------------------------------
#define INFRA_GLOBALCON_RST0               (INFRACFG_BASE+0x030)

#define PWRAP_SOFT_RESET                   WRAP_SET_BIT(1<<7,INFRA_GLOBALCON_RST0)
#define PWRAP_CLEAR_SOFT_RESET_BIT         WRAP_CLR_BIT(1<<7,INFRA_GLOBALCON_RST0)
#define PERI_GLOBALCON_RST1                (PERICFG_BASE+0x004)

#endif // __PMIC_WRAP_REGS_H__

