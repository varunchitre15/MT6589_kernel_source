

#ifndef SEC_HACC_EXPORT_H
#define SEC_HACC_EXPORT_H

// ========================================================
// HACC USER
// ========================================================
typedef enum{
    HACC_USER1 = 0,
    HACC_USER2,
    HACC_USER3
} HACC_USER;

// ========================================================
// SECURE DRIVER INTERNAL HACC FUNCTION
// ========================================================
extern unsigned int sp_hacc_blk_sz (void);
extern unsigned int sp_hacc_init (unsigned char*sec_seed, unsigned int size);
extern unsigned char* sp_hacc_dec(unsigned char *buf, unsigned int size, unsigned char b_anti_clone,HACC_USER user, unsigned char bDoLock);
extern unsigned char* sp_hacc_enc(unsigned char *buf, unsigned int size, unsigned char b_anti_clone,HACC_USER user, unsigned char bDoLock);

#endif 

