#ifndef _SECIMGHEADER_H
#define _SECIMGHEADER_H

/**************************************************************************
 *  SEC IMAGE HEADER FORMAT
 **************************************************************************/

#define CUSTOM_NAME                  "CUSTOM_NAME"
#define IMAGE_VERSION                "IMAGE_VERSION"
#define SEC_IMG_MAGIC                (0x53535353)
#define FB_IMG_MAGIC                 (0x46424642)
#define SEC_IMG_HDR_SZ               (64)

/* in order to speedup verification, you can customize the image size 
   which should be signed and checked at boot time */
#define VERIFY_OFFSET                "VERIFY_OFFSET"
#define VERIFY_LENGTH                "VERIFY_LENGTH"


typedef struct _SEC_FB_HEADER
{
    unsigned int magic_num;
    unsigned int hdr_ver;
    unsigned int hash_count;
    unsigned int chunk_size;
    unsigned char part_name[32];
    unsigned int orig_img_size;
    unsigned char reserved[12];
} SEC_FB_HEADER;

 
typedef struct _SEC_IMG_HEADER
{
    unsigned int magic_num;

    /* After WK1151, the size of customer name will be changed from 16 
       bytes to 32 bytes due to customer's request. To distinguish between the 
       old version and new version, the simplest way is to check the value of 
       signature_length. 
    
       Flash tool downloads images by using new format 
       => Tool can find the image is old because signature_length is all 0x00.
          Therefore, Flash tool will automatically apply old image format */

    /* After WK1151 */
    unsigned char cust_name [32];
    unsigned int img_ver;
    unsigned int img_len;  
    unsigned int img_off;
    
    unsigned int s_off;
    unsigned int s_len;
    
    unsigned int sig_off;
    unsigned int sig_len;

    /* Before WK1151 */
#if 0    
    unsigned char cust_name [16];
    unsigned int img_ver;
    unsigned int img_len;  
    unsigned int img_off;
    
    unsigned int s_off;
    unsigned int s_len;
    
    unsigned int sig_off;
    unsigned int sig_len;
    unsigned char dummy [16];
#endif    
    

} SEC_IMG_HEADER;

typedef enum
{
    SEC_HDR_V1 = 1,
    SEC_HDR_V2 = 2,
    SEC_HDR_V1_V2 = 3,
    SEC_HDR_V3 = 4,
    
    UNSET
} SEC_IMG_HEADER_VER;


#endif   /*_SECIMGHEADER_H*/




