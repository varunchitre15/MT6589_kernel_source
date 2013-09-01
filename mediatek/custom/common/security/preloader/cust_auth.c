
#include "sec_platform.h"
#include "sec_auth.h"
#include "sec_error.h"

#define SHA1_HASH_LEN       (20)
#define SHA256_HASH_LEN     (32)
#define RSA1024_SIG_LEN     (128)
#define RSA2048_SIG_LEN     (256)

/**************************************************************************
 *  HASH (should support SHA1 and SHA256)
 **************************************************************************/
int sec_hash (U8* data_buf, U32 data_len, U8* hash_buf, U32 hash_len)
{
    if(SHA1_HASH_LEN == hash_len)
    {
        /* =============== */
        /* SHA1            */
        /* =============== */
        
        /* TODO : use sec_hash to generate hash value */    
        /* customer needs to customized this function */
        return ERR_HASH_IMAGE_FAIL;        
    }
    else if(SHA256_HASH_LEN == hash_len)
    {
        /* =============== */
        /* SHA256          */
        /* =============== */
        
        /* TODO : use sec_hash to generate hash value */    
        /* customer needs to customized this function */
        return ERR_HASH_IMAGE_FAIL;        
    }
    else
    {
        ASSERT(0);
    }
}

/**************************************************************************
 *  RSA (should support RSA1024 and RSA2048)
 **************************************************************************/
int sec_auth (U8* data_buf, U32 data_len, U8* sig_buf, U32 sig_len)
{
    if(RSA1024_SIG_LEN == sig_len)
    {
        U8 sha1sum[SHA1_HASH_LEN] = {0};    
        
        /* =============== */
        /* RSA1024         */
        /* =============== */        

        /* SHA1 */        
        if( sec_hash(data_buf, data_len, sha1sum, SHA1_HASH_LEN) != 0 )
        {
            /* return verify failed */
            return -1;
        }        
        
        /* TODO : use sec_hash to generate hash value */    
        /* customer needs to customized this function */
        return ERR_AUTH_IMAGE_VERIFY_FAIL;        
    }
    else if(RSA2048_SIG_LEN == sig_len)
    {
        U8 sha256sum[SHA256_HASH_LEN] = {0};
    
        /* =============== */
        /* RSA2048         */
        /* =============== */        

        /* SHA256 */      
        if( sec_hash(data_buf, data_len, sha256sum, SHA256_HASH_LEN) != 0 )
        {
            /* return verify failed */
            return -1;
        }        
        
        /* TODO : use sec_hash to generate hash value */    
        /* customer needs to customized this function */
        return ERR_AUTH_IMAGE_VERIFY_FAIL;        
    }
    else
    {
        ASSERT(0);
    }
}

/**************************************************************************
 *  DA AUTH INIT
 **************************************************************************/
U32 da_auth_init (void)
{
    /* customer needs to customized this function */
    return SEC_OK;
}

/**************************************************************************
 *  IMAGE AUTH INIT
 **************************************************************************/
U32 img_auth_init (void)
{       
    /* customer needs to customized this function */
    return SEC_OK;   
}
