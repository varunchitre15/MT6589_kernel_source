#include <cust_jogball.h>
#include <mach/mt6575_gpt.h>

struct jogball_hw cust_jb_hw = {
    .report_cls = JBD_CLASS_TRACKBALL,
        
    /*trackball class*/    
    .gain_x     = 1,   /*adjust by screen resolution*/
    .gain_y     = 1,   /*adjust by screen resolution*/
    .detect     = 1,    /*eint mode*/
#if 0
    /*keyboard class*/
    .detect     = 1,    /*eint mode*/
    .delay      = 20,   
    .gpt_num    = XGPT6,
    .gpt_period = 33,   /* GPT ticks --> 33 ticks is about 1ms */
#endif
    .acc_cnt    = 300,
    .inact_cnt  = 100,
    .act_cnt    = 5,
    .step       = 3,
};
struct jogball_hw *get_cust_jogball_hw(void)
{
    return &cust_jb_hw;
}
