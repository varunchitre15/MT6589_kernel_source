#include <mach/mt6516_pll.h>
#include <mach/mt6516_typedefs.h> 
#include <mach/mt6516_reg_base.h>
#include <mach/mt6516_gpio.h>
#include <cust_gpio_usage.h>


#define MATV_I2C_DEVNAME "MT6516_I2C_MATV"
#define MATV_I2C_CHANNEL     (0)        //I2C Channel 1

extern int cust_matv_power_on(void);
extern int cust_matv_power_off(void);


#define GPIO_MATV_I2S_DATA   GPIO27

#ifndef GPIO_MATV_PWR_ENABLE
#define GPIO_MATV_PWR_ENABLE GPIO126
#endif
#ifndef GPIO_MATV_N_RST
#define GPIO_MATV_N_RST      GPIO127
#endif

#if 1
#define MATV_LOGD printk
#else
#define MATV_LOGD(...)
#endif
#if 1
#define MATV_LOGE printk
#else
#define MATV_LOGE(...)
#endif


