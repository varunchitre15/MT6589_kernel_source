#ifndef __CUST_JOGBALL_H__
#define __CUST_JOGBALL_H__
/*----------------------------------------------------------------------------*/
typedef enum {
    JBD_DETECT_POLL     = 0,
    JBD_DETECT_EINT     = 1,            
} JBD_DETECT;
/*----------------------------------------------------------------------------*/
typedef enum {
    JBD_CLASS_KEYBOARD,
    JBD_CLASS_TRACKBALL, 
} JBD_CLASS;
/*----------------------------------------------------------------------------*/
#define C_JBD_RESOLUTION    (22)    /*11 pulses/360 => 22 changes/360*/
/*----------------------------------------------------------------------------*/
struct jogball_hw {
    int report_cls; /*refer to JBD_CLASS*/

    /*trackball class*/
    int gain_x;     /*x-axis gain*/
    int gain_y;     /*y-axis gain*/
    
    /*keyboard class*/
    int detect;     /*refer to JBD_DETECT*/
    int delay;      /*the period of scan timer for reporting key event*/
    int gpt_num;    /*the gpt number used for checking jogball status periodically*/
    int gpt_period; /*the sample period for checking jogball status*/
    
    int acc_cnt;    /*the maximum period of triggered event in one direction*/
    int inact_cnt;  /*the maximum period of two consecutive motion event in one direction*/
    int act_cnt;    /*the minimum period of two consecutive motion event in one direction*/
    int step;       /*the minimum count of triggering key event*/
};
/*----------------------------------------------------------------------------*/
extern struct jogball_hw *get_cust_jogball_hw(void);
/*----------------------------------------------------------------------------*/
#endif 
