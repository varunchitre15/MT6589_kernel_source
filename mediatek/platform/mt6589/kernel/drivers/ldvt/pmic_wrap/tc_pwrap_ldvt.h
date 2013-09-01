#ifndef _TC_PWRAP_LDVT_H_
#define _TS_PMIC_WRAP_H_
#include <mach/mt_typedefs.h>

//#include "reg_pmic.h"
//#include "reg_pmic_wrap.h"

extern S32 tc_wrap_init_test(void  );
extern S32 tc_wrap_access_test( void );
extern S32 tc_status_update_test(void  );
extern S32 tc_event_test( void );
extern S32 tc_dual_io_test(void  );
extern S32 tc_reg_rw_test(  void);
extern S32 tc_mux_switch_test( void );
extern S32 tc_reset_pattern_test( void );
extern S32 tc_soft_reset_test( void );
extern S32 tc_high_pri_test( void );
extern S32 tc_in_order_pri_test( void );
extern S32 tc_spi_encryption_test( void );
extern S32 tc_wdt_test(void  );
extern S32 tc_peri_wdt_test(void  );
extern S32 tc_int_test(void  );
extern S32 tc_peri_int_test(void  );
extern S32 pwrap_interrupt_on_ldvt(void);
extern S32 tc_concurrence_test( void );
extern S32 tc_clock_gating_test( void );
extern S32 tc_throughput_test( void );
extern int mtk_pmic_dvfs_wrapper_test(int loop_count);
#endif /* _TC_PWRAP_LDVT_H_ */
