#ifndef __CUST_BARO_H__
#define __CUST_BARO_H__

struct baro_hw {
    int i2c_num;    /*!< the i2c bus used by the chip */
    int direction;  /*!< the direction of the chip */
    int power_id;   /*!< the LDO ID of the chip, MT6516_POWER_NONE means the power is always on*/
    int power_vol;  /*!< the Power Voltage used by the chip */
    int firlen;     /*!< the length of low pass filter */
    int (*power)(struct baro_hw *hw, unsigned int on, char *devname);
};

extern struct baro_hw* get_cust_baro_hw(void);
#endif 
