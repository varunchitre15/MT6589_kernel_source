#ifndef __CUST_GYRO_H__
#define __CUST_GYRO_H__

struct gyro_hw {
    unsigned short addr;
    int i2c_num;    /*!< the i2c bus used by the chip */
    int direction;  /*!< the direction of the chip */
    int power_id;   /*!< the LDO ID of the chip, MT6516_POWER_NONE means the power is always on*/
    int power_vol;  /*!< the Power Voltage used by the chip */
    int firlen;     /*!< the length of low pass filter */
    int (*power)(struct gyro_hw *hw, unsigned int on, char *devname);
};

extern struct gyro_hw* get_cust_gyro_hw(void);
#endif 
