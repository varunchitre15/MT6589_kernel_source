#ifndef __MTK_FM_H__
#define __MTK_FM_H__

#include <linux/types.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

//#define HiSideTableSize 1
#define FM_TX_PWR_CTRL_FREQ_THR 890
#define FM_TX_PWR_CTRL_TMP_THR_UP 45
#define FM_TX_PWR_CTRL_TMP_THR_DOWN 0

#define FM_TX_TRACKING_TIME_MAX 10000 //TX VCO tracking time, default 100ms

enum fm_priv_state{
    UNINITED,
    INITED     
};

enum fm_adpll_state{
    FM_ADPLL_ON,
    FM_ADPLL_OFF
};

enum fm_adpll_clk{
    FM_ADPLL_16M,
    FM_ADPLL_15M
};

enum fm_mcu_desense{
    FM_MCU_DESENSE_ENABLE,
    FM_MCU_DESENSE_DISABLE
};

struct fm_priv_cb {
	//Basic functions.
	int (*hl_side)(uint16_t freq, int *hl);
	int (*adpll_freq_avoid)(uint16_t freq, int *freqavoid);
	int (*mcu_freq_avoid)(uint16_t freq, int *freqavoid);
    int (*tx_pwr_ctrl)(uint16_t freq, int *ctr);
    int (*rtc_drift_ctrl)(uint16_t freq, int *ctr);
    int (*tx_desense_wifi)(uint16_t freq, int *ctr);
    int (*is_dese_chan)(fm_u16 freq);             // check if this is a de-sense channel
};

struct fm_priv{
    int state;
    void *data;
    struct fm_priv_cb priv_tbl;
};

struct fm_op_cb {
	//Basic functions.
	int (*read)(uint8_t addr, uint16_t *val);
	int (*write)(uint8_t addr, uint16_t val);
	int (*setbits)(uint8_t addr, uint16_t bits, uint16_t mask);
    int (*rampdown)(void);
};

struct fm_op{
    int state;
    void *data;
    struct fm_op_cb op_tbl;
};

#define FMR_ASSERT(a) { \
			if ((a) == NULL) { \
				printk("%s,invalid pointer\n", __func__);\
				return -ERR_INVALID_BUF; \
			} \
		}

int fm_priv_register(struct fm_priv *pri, struct fm_op *op);
int fm_priv_unregister(struct fm_priv *pri, struct fm_op *op);

#endif //__MTK_FM_H__
