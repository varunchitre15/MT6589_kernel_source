#ifndef __CUST_MAG_H__
#define __CUST_MAG_H__

struct mag_hw {
    int i2c_num;
    int direction;
    int power_id;
    int power_vol;
};

extern struct mag_hw* get_cust_mag_hw(void);
#endif 
