

#include "cust_msdc.h"

struct msdc_cust msdc_cap = {
    MSDC_CLKSRC_200MHZ, /* host clock source             */
    MSDC_SMPL_RISING,   /* command latch edge            */
    MSDC_SMPL_RISING,   /* data latch edge               */
    2,                  /* clock pad driving             */
    2,                  /* command pad driving           */
    2,                  /* data pad driving              */
    8,                  /* data pins                     */
    0,                  /* data address offset           */

    /* hardware capability flags     */
    MSDC_HIGHSPEED|MSDC_DDR, 
};


