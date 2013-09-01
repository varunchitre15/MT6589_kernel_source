

#ifndef SEC_HACC_HK_H
#define SEC_HACC_HK_H


/******************************************************************************
 *  CHIP SELECTION
 ******************************************************************************/
#include <mach/mt_typedefs.h>
#include "hacc_mach.h"

/******************************************************************************
 *  EXPORT FUNCTION
 ******************************************************************************/
extern void HACC_V3_Init(bool encode, const unsigned int g_AC_CFG[]);
extern void HACC_V3_Run(volatile U32 *p_src, U32 src_len, volatile U32 *p_dst);
extern void HACC_V3_Terminate(void);

#endif 

