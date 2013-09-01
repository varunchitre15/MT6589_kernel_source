#include <platform/mt_typedefs.h>

#define SHA1_HASH_LEN       (20)
#define SHA256_HASH_LEN     (32)
#define RSA1024_SIG_LEN     (128)
#define RSA2048_SIG_LEN     (256)

/**************************************************************************
 *  HASH (should support SHA1 and SHA256)
 **************************************************************************/
void cust_init_key (unsigned char *key_rsa_n, unsigned int nKey_len,
                    unsigned char *key_rsa_e, unsigned int eKey_len)
{
	 /* customer needs to customized this function */
}

int cust_hash (unsigned char* data_buf,  unsigned int data_len, unsigned char* hash_buf, unsigned int hash_len)
{
 	if(SHA1_HASH_LEN == hash_len)
    {
        /* =============== */
        /* SHA1            */
        /* =============== */
        
        /* TODO : use sec_hash to generate hash value */    
        /* customer needs to customized this function */
    }
    else if(SHA256_HASH_LEN == hash_len)
    {
        /* =============== */
        /* SHA256          */
        /* =============== */
        
        /* TODO : use sec_hash to generate hash value */    
        /* customer needs to customized this function */
    }
    else
    {
		// assert error
    }
}

int cust_verify (unsigned char* data_buf,  unsigned int data_len, unsigned char* sig_buf, unsigned int sig_len)
{
	if(RSA1024_SIG_LEN == sig_len)
	{
        U8 sha1sum[SHA1_HASH_LEN] = {0};    
        
        /* =============== */
        /* RSA1024         */
        /* =============== */        

        /* SHA1 */        
        if( cust_hash(data_buf, data_len, sha1sum, SHA1_HASH_LEN) != 0 )
        {
            /* return verify failed */
            return -1;
        }        
        
        /* TODO : use sec_hash to generate hash value */    
        /* customer needs to customized this function */
         return -1;        
    }
    else if(RSA2048_SIG_LEN == sig_len)
    {
        U8 sha256sum[SHA256_HASH_LEN] = {0};
    
        /* =============== */
        /* RSA2048         */
        /* =============== */        

        /* SHA256 */      
        if( cust_hash(data_buf, data_len, sha256sum, SHA256_HASH_LEN) != 0 )
        {
            /* return verify failed */
            return -1;
        }        
        
        /* TODO : use sec_hash to generate hash value */    
        /* customer needs to customized this function */
        return -1;        
    }
    else
    {
        // assert error
    }
}
