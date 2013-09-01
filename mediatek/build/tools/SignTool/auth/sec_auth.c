#include <sign_img.h>

/**************************************************************************
 * EXPORTED FUNCTIONS
 **************************************************************************/
extern void lib_init_key(CUSTOMER_SEC_INTER  cust_inter);
extern int lib_sign(unsigned char* data_buf,  unsigned int data_len, unsigned char* sig_buf, unsigned int sig_len);
extern int lib_hash(unsigned char* data_buf,  unsigned int data_len, unsigned char* hash_buf, unsigned int hash_len);
extern int lib_verify(unsigned char* data_buf,  unsigned int data_len, unsigned char* sig_buf, unsigned int sig_len);

void cust_init_key (unsigned char *key_rsa_n, unsigned int nKey_len,
                        unsigned char *key_rsa_d, unsigned int dKey_len,
                        unsigned char *key_rsa_e, unsigned int eKey_len)
{
    CUSTOMER_SEC_INTER cust;

    mcpy(cust.key_rsa_n,key_rsa_n,sizeof(cust.key_rsa_n));
    mcpy(cust.key_rsa_d,key_rsa_d,sizeof(cust.key_rsa_d));
    mcpy(cust.key_rsa_e,key_rsa_e,sizeof(cust.key_rsa_e));

    return lib_init_key(cust);
}

int cust_sign(unsigned char* data_buf,  unsigned int data_len, unsigned char* sig_buf, unsigned int sig_len)
{
    return lib_sign(data_buf, data_len, sig_buf, sig_len);
}

int cust_hash (unsigned char* data_buf,  unsigned int data_len, unsigned char* hash_buf, unsigned int hash_len)
{
    return lib_hash(data_buf, data_len, hash_buf, hash_len);
}

int cust_verify (unsigned char* data_buf,  unsigned int data_len, unsigned char* sig_buf, unsigned int sig_len)
{
    return lib_verify(data_buf, data_len, sig_buf, sig_len);
}


