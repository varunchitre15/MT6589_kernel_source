//For e1kv2
#include "cust_matv.h"

int cust_matv_power_on(void)
{  
    if(TRUE != hwPowerOn(MT6516_POWER_VCAM_A,VOL_2800,"MT5192"))
    {
        MATV_LOGE("[MATV] Fail to enable analog gain\n");
        return -EIO;
    } 
    return 0;
}


int cust_matv_power_off(void)
{  
    if(TRUE != hwPowerDown(MT6516_POWER_VCAM_A,"MT5192"))
    {
        MATV_LOGE("[MATV] Fail to disable analog gain VCAMA\n");
        return -EIO;
    }  
    return 0;
}

