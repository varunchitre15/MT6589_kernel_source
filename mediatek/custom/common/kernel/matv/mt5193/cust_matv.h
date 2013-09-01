///#include <mach/mt6575_pll.h>
///#include <mach/mt6575_typedefs.h> 
///#include <mach/mt6575_gpio.h>
#include <kd_camera_hw.h>
#include <cust_gpio_usage.h>



#define CAMERA_IO_DRV_1800
//6573_EVB
#define MATV_I2C_CHANNEL     (1)        //I2C Channel 1
//zte73v1
//#define MATV_I2C_CHANNEL     (1)        //I2C Channel 1
extern int cust_matv_power_on(void);
extern int cust_matv_power_off(void);
//customize matv i2s gpio and close fm i2s mode.
extern int cust_matv_gpio_on(void);
extern int cust_matv_gpio_off(void);


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


