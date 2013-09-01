#ifndef _MT_SPM_SLEEP_
#define _MT_SPM_SLEEP_

#include <linux/kernel.h>

typedef enum {
    WR_NONE         = 0,
    WR_UART_BUSY    = 1,
    WR_PCM_ASSERT   = 2,
    WR_PCM_TIMER    = 3,
    WR_WAKE_SRC     = 4,
    WR_SW_ABORT     = 5,
} wake_reason_t;

/*
 * for suspend
 */
extern int spm_set_sleep_wakesrc(u32 wakesrc, bool enable, bool replace);
extern wake_reason_t spm_go_to_sleep(bool cpu_pdn, bool infra_pdn);
extern wake_reason_t spm_go_to_sleep_dpidle(bool cpu_pdn, u8 pwrlevel);


/*
 * for deep idle
 */
extern bool spm_dpidle_can_enter(void);         /* can be redefined */
extern void spm_dpidle_before_wfi(void);        /* can be redefined */
extern void spm_dpidle_after_wfi(void);         /* can be redefined */
extern wake_reason_t spm_go_to_dpidle(bool cpu_pdn, u8 pwrlevel);


extern bool spm_is_md1_sleep(void);
extern bool spm_is_md2_sleep(void);

extern void spm_output_sleep_option(void);

#endif
