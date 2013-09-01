

#include "cust_msdc.h"

struct msdc_cust msdc_cap = {
    MSDC_CLKSRC_197MHZ, /* host clock source             */
    MSDC_SMPL_RISING,   /* command latch edge            */
    MSDC_SMPL_RISING,   /* data latch edge               */
    4,                  /* clock pad driving             */
    4,                  /* command pad driving           */
    4,                  /* data pad driving              */
    4,                  /* data pins                     */
    0,                  /* data address offset           */

    /* hardware capability flags     */
    MSDC_HIGHSPEED, 
};


