

#ifndef __GFMT_DRV_6589_COMMON_H__
#define __GFMT_DRV_6589_COMMON_H__

#include <mach/mt_typedefs.h>


#include "gfmt_drv.h"





void gfmt_drv_reset(void);

int gfmt_config_fmt(GDMA_DRV_FMT_IN param);
void gfmt_drv_dump_reg(void);


int gfmt_isr_lisr(void);
kal_uint32 gfmt_drv_get_result(void);




#endif


