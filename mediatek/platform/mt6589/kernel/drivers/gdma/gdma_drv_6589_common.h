

#ifndef __GDMA_DRV_6589_COMMON_H__
#define __GDMA_DRV_6589_COMMON_H__

#include <mach/mt_typedefs.h>


#include "gdma_drv.h"


void gdma_config_ctl(GDMA_DRV_CTL_IN *cfgCTL) ;
void gdma_rw_reg(void);
void gdma_dump_reg(void);
void gdma_drv_reset(void);





#endif


