#ifndef _SIGNIMG_H
#define _SIGNIMG_H


/**************************************************************************
 *  RSA
 **************************************************************************/
#define RSA1024_KEY_SIZE                (128) /* bytes */
#define CUSTOM_RSA_N                "CUSTOM_RSA_N" /* public */
#define CUSTOM_RSA_D                "CUSTOM_RSA_D" /* private */
#define CUSTOM_RSA_E                "CUSTOM_RSA_E"
#define PREFIX_SIZE                 (2)
 
typedef struct _CUSTOMER_SEC_INTER
{
    /* key to sign image patch */
    unsigned char                   key_rsa_n[RSA1024_KEY_SIZE*2]; /* string. number base = 16 */
    unsigned char                   key_rsa_d[RSA1024_KEY_SIZE*2]; /* string. number base = 16 */
    unsigned char                   key_rsa_e[5]; /* string. number base = 16 */
}CUSTOMER_SEC_INTER;

#endif   /*_SIGNIMG_H*/



