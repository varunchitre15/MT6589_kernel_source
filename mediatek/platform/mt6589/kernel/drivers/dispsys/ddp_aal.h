#ifndef __DDP_AAL_H__
#define __DDP_AAL_H__


#define PWM_DUTY_LEVEL      256
#define PWM_DUTY_STEP       1
#define PWM_DUTY_MAX        255

#define BLS_HIST_BIN        32

#define LUMA_HIST_BIN       33
#define LUMA_HIST_STEP      32
#define LUMA_HIST_MAX       1023

#define LUMA_CURVE_POINT    33
#define LUMA_CURVE_STEP     32
#define LUMA_CURVE_MAX      1023


enum
{
    ENUM_FUNC_NONE = 0,
    ENUM_FUNC_GAMMA = 0x1,
    ENUM_FUNC_AAL = 0x2,
    ENUM_FUNC_BLS = 0x4,
};

typedef struct {
    unsigned long histogram[LUMA_HIST_BIN];
    unsigned long BLSHist[BLS_HIST_BIN];
    unsigned long ChromHist;
} DISP_AAL_STATISTICS;

typedef struct {
    unsigned long lumaCurve[LUMA_CURVE_POINT];
    unsigned long pwmDuty;
    
    // for BLS
    unsigned long setting;
    unsigned long maxClrLimit;
    unsigned long maxClrDistThd;    
    unsigned long preDistThd;
    unsigned long scDiffThd;
    unsigned long scBinThd;    
} DISP_AAL_PARAM;

//IOCTL , for AAL service to wait vsync and get latest histogram
int disp_wait_hist_update(unsigned long u4TimeOut_ms);

//IOCTL , for AAL service to enable vsync notification
void disp_set_aal_alarm(unsigned int u4En);

//Called by interrupt to check if aal need to be notified
void on_disp_aal_alarm_set(void);
unsigned int is_disp_aal_alarm_on(void);

//Called by interrupt to wake up aal
int disp_needWakeUp(void);

//Called by interrupt to wake up aal
void disp_wakeup_aal(void);

//IOCTL , for AAL service to config AAL
DISP_AAL_PARAM * get_aal_config(void);

//Called by tasklet to config registers
void disp_onConfig_aal(int i4FrameUpdate);

//Called by BLS backlight function
int disp_is_aal_config(void);

#endif
