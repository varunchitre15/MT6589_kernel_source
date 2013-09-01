#ifndef _CRYPTO_ENGINE_EXPORT_H_
#define _CRYPTO_ENGINE_EXPORT_H_

// ========================================================
// CHIP SELECTION
// ========================================================
#include <mach/mt_typedefs.h>


// ========================================================
// CRYPTO ENGINE EXPORTED API
// ========================================================

/* perform crypto operation
   @ Direction   : TRUE  (1) means encrypt
                   FALSE (0) means decrypt
   @ ContentAddr : input source address
   @ ContentLen  : input source length
   @ CustomSeed  : customization seed for crypto engine
   @ ResText     : output destination address */
extern void SST_Secure_Algo(kal_uint8 Direction, kal_uint32 ContentAddr, kal_uint32 ContentLen, kal_uint8 *CustomSeed, kal_uint8 *ResText);

/* return the result of hwEnableClock ( )
   - TRUE  (1) means crypto engine init success
   - FALSE (0) means crypto engine init fail    */
extern BOOL SST_Secure_Init(void);

/* return the result of hwDisableClock ( )
   - TRUE  (1) means crypto engine de-init success
   - FALSE (0) means crypto engine de-init fail    */
extern BOOL SST_Secure_DeInit(void);

#endif 

