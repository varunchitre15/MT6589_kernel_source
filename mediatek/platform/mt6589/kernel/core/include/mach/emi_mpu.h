#ifndef __MT_EMI_MPU_H
#define __MT_EMI_MPU_H

#define EMI_MPUA (EMI_BASE+0x0160)
#define EMI_MPUB (EMI_BASE+0x0168)
#define EMI_MPUC (EMI_BASE+0x0170)
#define EMI_MPUD (EMI_BASE+0x0178)
#define EMI_MPUE (EMI_BASE+0x0180)
#define EMI_MPUF (EMI_BASE+0x0188)
#define EMI_MPUG (EMI_BASE+0x0190)
#define EMI_MPUH (EMI_BASE+0x0198)
#define EMI_MPUI (EMI_BASE+0x01A0)
#define EMI_MPUJ (EMI_BASE+0x01A8)
#define EMI_MPUK (EMI_BASE+0x01B0)
#define EMI_MPUL (EMI_BASE+0x01B8)
#define EMI_MPUM (EMI_BASE+0x01C0)
#define EMI_MPUN (EMI_BASE+0x01C8)
#define EMI_MPUO (EMI_BASE+0x01D0)
#define EMI_MPUP (EMI_BASE+0x01D8)
#define EMI_MPUQ (EMI_BASE+0x01E0)
#define EMI_MPUR (EMI_BASE+0x01E8)
#define EMI_MPUS (EMI_BASE+0x01F0)
#define EMI_MPUT (EMI_BASE+0x01F8)
#define EMI_MPUU (EMI_BASE+0x0200)
#define EMI_MPUY (EMI_BASE+0x0220)

#define NO_PROTECTION 0
#define SEC_RW 1
#define SEC_RW_NSEC_R 2
#define SEC_RW_NSEC_W 3
#define SEC_R_NSEC_R 4
#define FORBIDDEN 5

#define EN_MPU_STR "ON"
#define DIS_MPU_STR "OFF"


/*EMI memory protection align 64K*/
#define EMI_MPU_ALIGNMENT 0x10000
#define OOR_VIO 0x00000200
#define EMI_PHY_OFFSET 0x80000000

enum
{
    /* apmcu */
    MST_ID_APMCU_0, MST_ID_APMCU_1, MST_ID_APMCU_2, MST_ID_APMCU_3, MST_ID_APMCU_4,
    /* arm9 */
    MST_ID_ARM9_0, MST_ID_ARM9_1, MST_ID_ARM9_2,
    /* Modem */
    MST_ID_MDMCU_0, MST_ID_MDMCU_1, MST_ID_MDMCU_2,
    /* Modem HW (2G/3G) */
    MST_ID_MDHW_0, MST_ID_MDHW_1, MST_ID_MDHW_2, MST_ID_MDHW_3, MST_ID_MDHW_4, MST_ID_MDHW_5, MST_ID_MDHW_6, MST_ID_MDHW_7,
    /* MM0 + Periperal (MCI port) */
    MST_ID_MMPERI_0, MST_ID_MMPERI_1, MST_ID_MMPERI_2, MST_ID_MMPERI_3, MST_ID_MMPERI_4, MST_ID_MMPERI_5, MST_ID_MMPERI_6, MST_ID_MMPERI_7, MST_ID_MMPERI_8,
    MST_ID_MMPERI_9, MST_ID_MMPERI_10, MST_ID_MMPERI_11, MST_ID_MMPERI_12, MST_ID_MMPERI_13, MST_ID_MMPERI_14,
    /* MM1 */
    MST_ID_MM1_0, MST_ID_MM1_1, MST_ID_MM1_2, MST_ID_MM1_3, MST_ID_MM1_4, MST_ID_MM1_5, MST_ID_MM1_6, MST_ID_MM1_7,

    MST_INVALID,
    NR_MST,
};

typedef void (*emi_mpu_notifier)(u32 addr, int wr_vio);

#define SET_ACCESS_PERMISSON(d3, d2, d1, d0) (((d3) << 9) | ((d2) << 6) | ((d1) << 3) | (d0))

extern int emi_mpu_set_region_protection(unsigned int start_addr, unsigned int end_addr, int region, unsigned int access_permission);
extern int emi_mpu_notifier_register(int master, emi_mpu_notifier notifider);

#endif  /* !__MT_EMI_MPU_H */
