

#include <linux/types.h>
#include <cust_alsps.h>
#include <mach/mt_pm_ldo.h>

static struct alsps_hw cust_alsps_hw = {
    .i2c_num    = 3,
     //.polling_mode =1,
    .polling_mode_ps =1,
    .polling_mode_als =1,
    .power_id   = MT65XX_POWER_LDO_VGP4,    /*LDO is not used*/
    .power_vol  = VOL_2800,          /*LDO is not used*/
    .i2c_addr   = {0x92, 0x00, 0x00, 0x00},	/*EPL6881*/
    .als_level  = {5,  9, 36, 59, 82, 132, 205, 273, 500, 845, 1136, 1545, 2364, 4655, 6982},	/* als_code */
    .als_value  = {0, 10, 40, 65, 90, 145, 225, 300, 550, 930, 1250, 1700, 2600, 5120, 7680, 10240},    /* lux */
    //.ps_threshold = 0xFE,
    //.als_cmd_val = 0x49,	/*ALS_GAIN=1, IT_ALS=400ms*/
    //.ps_cmd_val = 0x21,	/*SLP=30ms, IT_PS=0.2ms*/
    //.ps_gain_setting = 0x08, /*PS_GAIN=4X */
    //.ps_high_thd_val = 0x78,
    //.ps_low_thd_val = 0x6E,
};
struct alsps_hw *get_cust_alsps_hw(void) {	
    return &cust_alsps_hw;
}
