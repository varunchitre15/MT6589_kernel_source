

#ifndef SEC_HACC_SK_H
#define SEC_HACC_SK_H

#include "hacc_def.h"

/******************************************************************************
 *  CHIP SELECTION
 ******************************************************************************/
#include <mach/mt_typedefs.h>
#include "hacc_mach.h"

/******************************************************************************
 *  EXPORT FUNCTION
 ******************************************************************************/
extern unsigned int hacc_set_key(AES_KEY_ID id, AES_KEY key);
extern unsigned int hacc_do_aes(AES_OPS ops, unsigned char *src, unsigned char *dst, unsigned int size);
extern unsigned int hacc_init(AES_KEY_SEED *keyseed);
extern unsigned int hacc_deinit(void);

#endif 

