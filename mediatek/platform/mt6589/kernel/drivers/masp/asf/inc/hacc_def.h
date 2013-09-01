

#ifndef SEC_HACC_H
#define SEC_HACC_H

#include "sec_osal_light.h"

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
extern void SST_Secure_Algo(unsigned char Direction, unsigned int ContentAddr, unsigned int ContentLen, unsigned char *CustomSeed, unsigned char *ResText);

/* return the result of hwEnableClock ( )
   - TRUE  (1) means crypto engine init success
   - FALSE (0) means crypto engine init fail    */
extern bool SST_Secure_Init(void);

/* return the result of hwDisableClock ( )
   - TRUE  (1) means crypto engine de-init success
   - FALSE (0) means crypto engine de-init fail    */
extern bool SST_Secure_DeInit(void);

/* for testing */
extern void crypto_engine_test (void);

/******************************************************************************
 * CONSTANT DEFINITIONS                                                       
 ******************************************************************************/
#define HACC_AES_MAX_KEY_SZ          (32)
#define AES_CFG_SZ                   (16)
#define AES_BLK_SZ                  (16)
#define HACC_HW_KEY_SZ               (16)
#define _CRYPTO_SEED_LEN            (16)

/* In order to support NAND writer and keep MTK secret,
   use MTK HACC seed and custom crypto seed to generate SW key
   to encrypt SEC_CFG */
#define MTK_HACC_SEED                (0x1)

/******************************************************************************
 * TYPE DEFINITIONS                                                           
 ******************************************************************************/
typedef enum
{   
    AES_ECB_MODE,
    AES_CBC_MODE
} AES_MODE;

typedef enum
{   
    AES_DEC,
    AES_ENC
} AES_OPS;

typedef enum
{
    AES_KEY_128 = 16,
    AES_KEY_192 = 24,
    AES_KEY_256 = 32
} AES_KEY;

typedef enum
{   
    AES_SW_KEY,
    AES_HW_KEY,
    AES_HW_WRAP_KEY
} AES_KEY_ID;

typedef struct {
    U8 config[AES_CFG_SZ];
} AES_CFG;

typedef struct {
    U32 size;
    U8  seed[HACC_AES_MAX_KEY_SZ];
} AES_KEY_SEED;

struct hacc_context {
    AES_CFG cfg;
    U32    blk_sz;
    U8     sw_key[HACC_AES_MAX_KEY_SZ];
    U8     hw_key[HACC_AES_MAX_KEY_SZ];
};

#endif 

