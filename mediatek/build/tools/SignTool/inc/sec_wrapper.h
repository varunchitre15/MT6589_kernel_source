#ifndef _SEC_WRAPPER_HEADER_H
#define _SEC_WRAPPER_HEADER_H

void cust_init_key (unsigned char *key_rsa_n, unsigned int nKey_len,
                        unsigned char *key_rsa_d, unsigned int dKey_len,
                        unsigned char *key_rsa_e, unsigned int eKey_len);
int cust_sign(unsigned char* data_buf,  unsigned int data_len, unsigned char* sig_buf, unsigned int sig_len);
int cust_hash (unsigned char* data_buf,  unsigned int data_len, unsigned char* hash_buf, unsigned int hash_len);
int cust_verify (unsigned char* data_buf,  unsigned int data_len, unsigned char* sig_buf, unsigned int sig_len);

#endif

