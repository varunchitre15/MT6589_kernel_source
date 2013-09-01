
#include <linux/types.h>
#include <mach/mt_pm_ldo.h>
#include <cust_mag.h>


static struct mag_hw cust_mag_hw = {
    .i2c_num = 3,
	//< 2013/4/9-23681-liulanqing- [5860][msensor] Fix msensor mmc3416x direction wrong issue
    .direction = 4,//6,
	//> 2013/4/9-23681-liulanqing
    .power_id = MT65XX_POWER_NONE,  /*!< LDO is not used */
    .power_vol= VOL_DEFAULT,        /*!< LDO is not used */
};
struct mag_hw* get_cust_mag_hw(void) 
{
    return &cust_mag_hw;
}
