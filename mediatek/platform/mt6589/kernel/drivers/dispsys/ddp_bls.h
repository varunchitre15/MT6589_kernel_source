#ifndef __DDP_BLS_H__
#define __DDP_BLS_H__

#include "ddp_drv.h"


void disp_bls_init(unsigned int srcWidth, unsigned int srcHeight);
int disp_bls_config(void);
int disp_bls_set_max_backlight(unsigned int level);
int disp_bls_set_backlight(unsigned int level);

DISPLAY_GAMMA_T * get_gamma_index(void);
DISPLAY_PWM_T * get_pwm_lut(void);

//Called by ioctl to config sysram
void disp_bls_update_gamma_lut(void);
void disp_bls_update_pwm_lut(void);

//Called by tasklet to config registers
void disp_onConfig_bls(DISP_AAL_PARAM *param);

#endif
