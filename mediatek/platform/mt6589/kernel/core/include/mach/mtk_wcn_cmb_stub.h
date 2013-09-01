

#ifndef _MTK_WCN_CMB_STUB_H_
#define _MTK_WCN_CMB_STUB_H_

#include <mach/mt_combo.h>
/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/


/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/



/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/



/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/
typedef enum {
    CMB_STUB_AIF_0 = 0, /* 0000: BT_PCM_OFF & FM analog (line in/out) */
    CMB_STUB_AIF_1 = 1, /* 0001: BT_PCM_ON & FM analog (in/out) */
    CMB_STUB_AIF_2 = 2, /* 0010: BT_PCM_OFF & FM digital (I2S) */
    CMB_STUB_AIF_3 = 3, /* 0011: BT_PCM_ON & FM digital (I2S) (invalid in 73evb & 1.2 phone configuration) */
    CMB_STUB_AIF_MAX = 4,
} CMB_STUB_AIF_X;

/*COMBO_CHIP_AUDIO_PIN_CTRL*/
typedef enum {
    CMB_STUB_AIF_CTRL_DIS = 0,
    CMB_STUB_AIF_CTRL_EN = 1,
    CMB_STUB_AIF_CTRL_MAX = 2,
} CMB_STUB_AIF_CTRL;

typedef void (*wmt_bgf_eirq_cb)(void);
typedef int (*wmt_aif_ctrl_cb)(CMB_STUB_AIF_X, CMB_STUB_AIF_CTRL);
typedef void (*wmt_func_ctrl_cb)(unsigned int, unsigned int);

typedef struct _CMB_STUB_CB_ {
    unsigned int size; //structure size
    /*wmt_bgf_eirq_cb bgf_eirq_cb;*//* remove bgf_eirq_cb from stub. handle it in platform */
    wmt_aif_ctrl_cb aif_ctrl_cb;
    wmt_func_ctrl_cb func_ctrl_cb;
} CMB_STUB_CB, *P_CMB_STUB_CB;

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/



/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/





/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

extern int mtk_wcn_cmb_stub_reg (P_CMB_STUB_CB p_stub_cb);
extern int mtk_wcn_cmb_stub_unreg (void);

extern int mtk_wcn_cmb_stub_aif_ctrl (CMB_STUB_AIF_X state, CMB_STUB_AIF_CTRL ctrl);

// TODO: [FixMe][GeorgeKuo]: put prototypes into mt_combo.h for board.c temporarily for non-finished porting
// TODO: old: rfkill->board.c->mt_combo->wmt_lib_plat
// TODO: new: rfkill->mtk_wcn_cmb_stub_alps->wmt_plat_alps
#if 0
extern int mtk_wcn_cmb_stub_func_ctrl(unsigned int type, unsigned int on);
#endif

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

#endif /* _MTK_WCN_CMB_STUB_H_ */






