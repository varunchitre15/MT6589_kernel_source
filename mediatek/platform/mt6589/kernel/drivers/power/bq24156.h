/*****************************************************************************
*
* Filename:
* ---------
*   bq24156.h
*
* Project:
* --------
*   Android
*
* Description:
* ------------
*   bq24156 header file
*
* Author:
* -------
*
****************************************************************************/

/****************************************************************************
  * Reference Datasheet bq24156a.pdf
  * Fully Integrated Switch-Mode One-Cell Li-Ion Charger With Full USB Compliance and USB-OTG Support
  * Check for Samples: bq24153A, bq24156A, bq24158
  * SLUSAB0-OCTOBER 2010
  ***************************************************************************/
  
#ifndef _bq24156_SW_H_
#define _bq24156_SW_H_

#define bq24156_CON0      0x00
#define bq24156_CON1      0x01
#define bq24156_CON2      0x02
#define bq24156_CON3      0x03
#define bq24156_CON4      0x04
#define bq24156_CON5      0x05
#define bq24156_CON6      0x06

/**********************************************************
  *
  *   [MASK/SHIFT] 
  *
  *********************************************************/
//CON0
#define CON0_TMR_RST_MASK   0x01
#define CON0_TMR_RST_SHIFT  7

#define CON0_SLRST_MASK       0x01
#define CON0_SLRST_SHIFT      7

#define CON0_EN_STAT_MASK   0x01
#define CON0_EN_STAT_SHIFT  6

#define CON0_STAT_MASK      0x03
#define CON0_STAT_SHIFT     4

#if 0 /* NA for bq24156A, p.28 */
#define CON0_BOOST_MASK     0x01
#define CON0_BOOST_SHIFT    3
#endif

#define CON0_FAULT_MASK     0x07
#define CON0_FAULT_SHIFT    0

//CON1
#define CON1_LIN_LIMIT_MASK     0x03
#define CON1_LIN_LIMIT_SHIFT    6

#define CON1_LOW_V_MASK     0x03
#define CON1_LOW_V_SHIFT    4

#define CON1_TE_MASK        0x01
#define CON1_TE_SHIFT       3

#define CON1_CE_MASK        0x01
#define CON1_CE_SHIFT       2

#define CON1_HZ_MODE_MASK   0x01
#define CON1_HZ_MODE_SHIFT  1

#if 0 /* NA for bq24156A, p.28 */
#define CON1_OPA_MODE_MASK  0x01
#define CON1_OPA_MODE_SHIFT 0
#endif

//CON2
#define CON2_OREG_MASK    0x3F
#define CON2_OREG_SHIFT   2

#if 0 /* NA for bq24156A, p.28 */
#define CON2_OTG_PL_MASK    0x01
#define CON2_OTG_PL_SHIFT   1

#define CON2_OTG_EN_MASK    0x01
#define CON2_OTG_EN_SHIFT   0
#endif

//CON3
#define CON3_VENDER_CODE_MASK   0x07
#define CON3_VENDER_CODE_SHIFT  5

#define CON3_PIN_MASK           0x03
#define CON3_PIN_SHIFT          3

#define CON3_REVISION_MASK      0x07
#define CON3_REVISION_SHIFT     0

//CON4
#define CON4_RESET_MASK     0x01
#define CON4_RESET_SHIFT    7

#define CON4_I_CHR_MASK     0x0F
#define CON4_I_CHR_SHIFT    3

#define CON4_I_TERM_MASK    0x07
#define CON4_I_TERM_SHIFT   0

//CON5
#define CON5_LOW_CHG_MASK      0x01
#define CON5_LOW_CHG_SHIFT     5

#define CON5_DPM_STATUS_MASK     0x01
#define CON5_DPM_STATUS_SHIFT    4

#define CON5_CD_STATUS_MASK      0x01
#define CON5_CD_STATUS_SHIFT     3

#define CON5_VSREG_MASK           0x07
#define CON5_VSREG_SHIFT          0

//CON6
#define CON6_ISAFE_MASK     0x0F /* VMCHRG, p.30, Table 9*/
#define CON6_ISAFE_SHIFT    4

#define CON6_VSAFE_MASK     0x0F /* VMREG, p.30, Table 9*/
#define CON6_VSAFE_SHIFT    0

/**********************************************************
  *
  *   [Extern Function] 
  *
  *********************************************************/
//CON0----------------------------------------------------
extern void bq24156_set_tmr_rst(kal_uint32 val);
extern kal_uint32 bq24156_get_slrst_status(void);
extern void bq24156_set_en_stat(kal_uint32 val);
extern kal_uint32 bq24156_get_chip_status(void);
extern kal_uint32 bq24156_get_fault_status(void);
//CON1----------------------------------------------------
extern void bq24156_set_input_charging_current(kal_uint32 val);
extern kal_uint32 bq24156_get_input_charging_current(void);
extern void bq24156_set_v_low(kal_uint32 val);
extern void bq24156_set_te(kal_uint32 val);
extern void bq24156_set_ce(kal_uint32 val);
extern void bq24156_set_hz_mode(kal_uint32 val);
//CON2----------------------------------------------------
extern void bq24156_set_oreg(kal_uint32 val);
//CON3----------------------------------------------------
extern kal_uint32 bq24156_get_vender_code(void);
extern kal_uint32 bq24156_get_pn(void);
extern kal_uint32 bq24156_get_revision(void);
//CON4----------------------------------------------------
extern void bq24156_set_reset(kal_uint32 val);
extern void bq24156_set_icharge(kal_uint32 val);
extern kal_uint32 bq24156_get_icharge(void);
extern void bq24156_set_iterm(kal_uint32 val);
//CON5----------------------------------------------------
extern void bq24156_set_low_chg(kal_uint32 val);
extern kal_uint32 bq24156_get_low_chg(void);
extern kal_uint32 bq24156_get_dpm_status(void);
extern kal_uint32 bq24156_get_cd_status(void);
extern void bq24156_set_vsreg(kal_uint32 val);
//CON6----------------------------------------------------
extern void bq24156_set_i_safe(kal_uint32 val);
extern void bq24156_set_v_safe(kal_uint32 val);
//---------------------------------------------------------
extern void bq24156_dump_register(void);
extern void bq24156_hw_init(void);

#endif // _bq24156_SW_H_

