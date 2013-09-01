#ifndef __MT6575_CA9_POWER_H__
#define __MT6575_CA9_POWER_H__

typedef enum {
    SCU_IC_STANDBY_ON,
    SCU_IC_STANDBY_OFF,
    SCU_STANDBY_ON,
    SCU_STANDBY_OFF
} SCU_CONTROL;

typedef enum {
    SCU_CPU_PWR_NORMAL = 0x0,
    SCU_CPU_PWR_DORMANT = 0x2,
    SCU_CPU_PWR_OFF = 0x3
} SCU_CPU_PWR_STATE;

typedef enum {
    CA9_DYNA_CLK_GATING_DISALBE,
    CA9_DYNA_CLK_GATING_ENABLE
} CA9_CLK_GATING_CTRL;

typedef enum {
    L2C_DYNA_CLK_GATING_DISALBE = 0,
    L2C_STANDBY_DISABLE = 0,
    L2C_STANDBY_ENABLE = 1,
    L2C_DYNA_CLK_GATING_ENABLE = 2
} L2C_POWER_CTRL;

extern int scu_control(const SCU_CONTROL ctrl);
extern int scu_set_cpu_pwr_status(const SCU_CPU_PWR_STATE state);
extern int mt6575_ca9_power_ctrl(const CA9_CLK_GATING_CTRL ctrl);
extern int mt6575_l2c_power_ctrl(const L2C_POWER_CTRL ctrl);

#endif  /* !__MT6575_CA9_POWER_H__ */
