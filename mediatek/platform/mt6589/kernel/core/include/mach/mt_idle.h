#ifndef _MT_IDLE_H
#define _MT_IDLE_H

#include <mach/mt_spm_idle.h>

#ifdef SPM_MCDI_FUNC
extern void enable_mcidle_by_bit(int id);
extern void disable_mcidle_by_bit(int id);
#else
static inline void enable_mcidle_by_bit(int id) { }
static inline void disable_mcidle_by_bit(int id) { }
#endif

extern void enable_dpidle_by_bit(int id);
extern void disable_dpidle_by_bit(int id);


#endif
