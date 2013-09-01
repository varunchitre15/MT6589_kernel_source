#define SHA1_HASH_LEN       (20)
#define SHA256_HASH_LEN     (32)
#define RSA1024_SIG_LEN     (128)
#define RSA2048_SIG_LEN     (256)


/**************************************************************************
 *  AUTH KEY INIT
 **************************************************************************/
int sec_init_key (unsigned char *nKey, unsigned int nKey_len, 
    unsigned char *eKey, unsigned int eKey_len)
{
    /* TODO : the key will pass in, need to judge if the key is for RSA1024 or RSA2048 and save it for later use */    
    /* customer needs to customized this function */
        
    return 0;
}

/**************************************************************************
 *  HASH (should support SHA1 and SHA256)
 **************************************************************************/
int sec_hash(unsigned char *data_buf,  unsigned int data_len,
    unsigned char *hash_buf, unsigned int hash_len)
{
    if(SHA1_HASH_LEN == hash_len)
    {
        /* =============== */
        /* SHA1            */
        /* =============== */
        
        /* TODO : use sec_hash to generate hash value */    
        /* customer needs to customized this function */
        
        return -1;        
    }
    else if(SHA256_HASH_LEN == hash_len)
    {
        /* =============== */
        /* SHA256          */
        /* =============== */
        
        /* TODO : use sec_hash to generate hash value */    
        /* customer needs to customized this function */
        
        return -1;        
    }
    else
    {
        /* unsupported, just return hash failed */
        return -1;
    }
}

/**************************************************************************
 *  RSA (should support RSA1024 and RSA2048)
 **************************************************************************/
int sec_verify (unsigned char *data_buf,  unsigned int data_len, 
    unsigned char *sig_buf, unsigned int sig_len)
{    
    if(RSA1024_SIG_LEN == sig_len)
    {
        unsigned char sha1sum[SHA1_HASH_LEN] = {0};

        /* SHA1 */        
        if( sec_hash(data_buf, data_len, sha1sum, SHA1_HASH_LEN) != 0 )
        {
            /* return verify failed */
            return -1;
        }
    
        /* TODO : use sec_verify to verify data buffer with RSA1024 */    
        /* customer needs to customized this function */
        
        return -1;        
    }
    else if(RSA2048_SIG_LEN == sig_len)
    {
        unsigned char sha256sum[SHA256_HASH_LEN] = {0};

        /* SHA256 */      
        if( sec_hash(data_buf, data_len, sha256sum, SHA256_HASH_LEN) != 0 )
        {
            /* return verify failed */
            return -1;
        }
        
        /* TODO : use sec_verify to verify data buffer with RSA2048*/    
        /* customer needs to customized this function */
        
        return -1;        
    }
    else
    {
        /* unsupported, just return verify failed */
        return -1;
    }
}

