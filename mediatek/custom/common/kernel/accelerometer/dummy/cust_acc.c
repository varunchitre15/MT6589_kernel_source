#include <cust_acc.h>
#include <mach/mt6516_pll.h>
static struct acc_hw cust_acc_hw = {
    .i2c_num = 2,
    .direction = 0,
    .power_id = MT6516_POWER_NONE,  /*!< LDO is not used */
    .power_vol= VOL_DEFAULT,        /*!< LDO is not used */
    .firlen   = 0,                  /*!< don't enable low pass filter */
};
struct acc_hw* get_cust_acc_hw(void) 
{
    return &cust_acc_hw;
}
