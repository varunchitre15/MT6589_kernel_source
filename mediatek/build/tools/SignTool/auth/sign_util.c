#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <type_defs.h>
#include <sign_extension.h>
#include <sign_header.h>
#include <sign_img.h>

/**************************************************************************
 *  DEFINITIONS
 **************************************************************************/
#define MOD                         "SignUtil"

/**************************************************************************
 *  MCARO
 **************************************************************************/
#define DBG                         printf

/**************************************************************************
  *  GLOBAL VARIABLES
  **************************************************************************/
static SEC_IMG_HEADER_VER g_hdr_ver = SEC_HDR_V1_V2;
static SEC_EXTENTION_CFG g_ext_cfg = {0};

unsigned int sec_crypto_hash_size[] =
{
    CRYPTO_SIZE_UNKNOWN,
    SEC_SIZE_HASH_MD5,
    SEC_SIZE_HASH_SHA1,
    SEC_SIZE_HASH_SHA256,
    SEC_SIZE_HASH_SHA512
};

unsigned int sec_crypto_sig_size[] =
{
    CRYPTO_SIZE_UNKNOWN,
    SEC_SIZE_SIG_RSA512,
    SEC_SIZE_SIG_RSA1024,
    SEC_SIZE_SIG_RSA2048
};


/**************************************************************************
  *  EXTERNAL VARIABLES
  **************************************************************************/
extern SEC_CRYPTO_HASH_TYPE g_hash_type;
extern SEC_CRYPTO_SIGNATURE_TYPE g_sig_type;

/**************************************************************************
 *  UTILITY FUNCTIONS
 **************************************************************************/
void set_hdr_version(SEC_IMG_HEADER_VER ver)
{
    g_hdr_ver = ver;
}

bool is_hdr_version3()
{
    return (g_hdr_ver == SEC_HDR_V3);
}

SEC_EXTENTION_CFG* get_ext_cfg()
{
    return &g_ext_cfg;
}

unsigned int get_hash_size(SEC_CRYPTO_HASH_TYPE hash)
{
    return sec_crypto_hash_size[hash];
}

unsigned int get_sigature_size(SEC_CRYPTO_SIGNATURE_TYPE sig)
{
    return sec_crypto_sig_size[sig];
}

SEC_EXTENSTION_CRYPTO* allocate_ext_crypto()
{
    SEC_EXTENSTION_CRYPTO *ext;

    ext = (SEC_EXTENSTION_CRYPTO *)malloc(sizeof(*ext));
    memset(ext, 0, sizeof(*ext));

    ext->magic = SEC_EXTENSION_HEADER_MAGIC;
    ext->ext_type = SEC_EXT_HDR_CRYPTO;
    ext->hash_type = SEC_CRYPTO_HASH_UNKNOWN;
    ext->sig_type = SEC_CRYPTO_SIG_UNKNOWN;
    ext->enc_type = SEC_CRYPTO_ENC_UNKNOWN;
    ext->reserved = 0;

    return ext;
}

SEC_FRAGMENT_CFG* allocate_ext_frag()
{
    SEC_FRAGMENT_CFG *ext;

    ext = (SEC_FRAGMENT_CFG *)malloc(sizeof(*ext));
    memset(ext, 0, sizeof(*ext));

    ext->magic = SEC_EXTENSION_HEADER_MAGIC;
    ext->ext_type = SEC_EXT_HDR_FRAG_CFG;
    ext->chunk_size = SEC_CHUNK_SIZE_UNKNOWN;
    ext->frag_count = 0;

    return ext;
}

unsigned int get_ext_hash_only_struct_size(SEC_CRYPTO_HASH_TYPE hash)
{
    return get_hash_size(hash) + sizeof(SEC_EXTENSTION_HASH_ONLY);
}

SEC_EXTENSTION_HASH_ONLY* allocate_ext_hash_only(SEC_CRYPTO_HASH_TYPE hash)
{
    SEC_EXTENSTION_HASH_ONLY *ext;
    unsigned int total_size = get_ext_hash_only_struct_size(hash);
    
    ext = (SEC_EXTENSTION_HASH_ONLY *)malloc(total_size);
    memset(ext, 0, total_size);

    ext->magic = SEC_EXTENSION_HEADER_MAGIC;
    ext->ext_type = SEC_EXT_HDR_HASH_ONLY;
    ext->sub_type = hash;
    ext->hash_offset = 0;
    ext->hash_len = 0;

    return ext;
}


unsigned int get_ext_hash_sig_struct_size(SEC_CRYPTO_HASH_TYPE hash,
    SEC_CRYPTO_SIGNATURE_TYPE sig)
{
    return get_sigature_size(sig) + get_hash_size(hash) + sizeof(SEC_EXTENSTION_HASH_SIG);
}

SEC_EXTENSTION_HASH_SIG* allocate_ext_hash_sig(SEC_CRYPTO_HASH_TYPE hash,
    SEC_CRYPTO_SIGNATURE_TYPE sig)
{
    SEC_EXTENSTION_HASH_SIG *ext;
    unsigned int total_size = get_ext_hash_sig_struct_size(hash,sig);
    
    ext = (SEC_EXTENSTION_HASH_SIG *)malloc(total_size);
    memset(ext, 0, total_size);
    
    ext->magic = SEC_EXTENSION_HEADER_MAGIC;
    ext->ext_type = SEC_EXT_HDR_HASH_SIG;
    ext->sig_type = sig;
    ext->hash_type = hash;
    ext->auth_offset = 0;
    ext->auth_len = 0;

    return ext;
}

unsigned int get_ext_sparse_struct_size(unsigned int sparse_data_len)
{
    return sizeof(SEC_EXTENSTION_SPARSE) + sparse_data_len;
}

SEC_EXTENSTION_SPARSE* allocate_ext_sparse(unsigned int sparse_data_len)
{
    SEC_EXTENSTION_SPARSE *ext;
    unsigned int total_size = get_ext_sparse_struct_size(sparse_data_len);
    
    ext = (SEC_EXTENSTION_SPARSE *)malloc(total_size);
    memset(ext, 0, total_size);
    
    ext->magic = SEC_EXTENSION_HEADER_MAGIC;
    ext->ext_type = SEC_EXT_HDR_SPARSE;

    return ext;
}


SEC_EXTENSTION_END_MARK* allocate_ext_end()
{
    SEC_EXTENSTION_END_MARK *ext;

    ext = (SEC_EXTENSTION_END_MARK *)malloc(sizeof(*ext));
    memset(ext, 0, sizeof(*ext));

    ext->magic = SEC_EXTENSION_HEADER_MAGIC;
    ext->ext_type = SEC_EXT_HDR_END_MARK;

    return ext;
}


int config_header_v1_v2_chk(SEC_IMG_HEADER *sec_hdr)
{
    sec_hdr->sig_len = get_sigature_size(g_sig_type)+get_hash_size(g_hash_type);

    /* ------------------------------------- */
    /* check sign length                     */
    /* ------------------------------------- */
    /* check if the whole image should be signed */
    if(0 == sec_hdr->s_len)
    {
        sec_hdr->s_len = sec_hdr->img_len - sec_hdr->s_off;
    }

    /* check if sign offset is greater than image length */
    if(sec_hdr->img_len <= sec_hdr->s_off)
    {
        MSG("[%s] IMG len (%d) <= sign off (%d)\n",MOD, sec_hdr->img_len, sec_hdr->s_off);                                                    
        MSG("[%s] Invalid sign off\n",MOD);
        return -1;
    }

    /* check if sign length is greater than image length */
    if(sec_hdr->img_len < (sec_hdr->s_len + sec_hdr->s_off))
    {
        MSG("[%s] IMG len (%d) < sign len (%d) + sign off (%d)\n",MOD, sec_hdr->img_len, sec_hdr->s_len, sec_hdr->s_off);                                                    
        sec_hdr->s_len = sec_hdr->img_len - sec_hdr->s_off;
        MSG("[%s] adjust sign len to (%d)\n",MOD,sec_hdr->s_len);        
    }

    return 0;
}

int config_header_v3_chk(SEC_IMG_HEADER *sec_hdr)
{
    int i;
    SEC_EXTENTION_CFG *p_ext_cfg = (SEC_EXTENTION_CFG *)get_ext_cfg();
    unsigned min_pos = 0;
    unsigned max_pos = sec_hdr->img_len;
    unsigned region_end = 0;

    DBG("[%s] sec_hdr->img_len = %d\n", MOD, sec_hdr->img_len);

    sec_hdr->sig_len = get_sigature_size(g_sig_type)+get_hash_size(g_hash_type);
    
    sec_hdr->s_off = SEC_EXTENSION_MAGIC;
    sec_hdr->s_len = SEC_EXTENSION_MAGIC;

    if(p_ext_cfg->verify_count == 0)
    {
        MSG("[%s] Sign region count is zero, please check config file(v3)\n",MOD);
        return -1;
    }

    /* check for chunk size 0 */
    if(p_ext_cfg->chunk_size == 0)
    {
        MSG("[%s] Chunk size is 0, truncate verify count to 1(v3)\n",MOD);
        p_ext_cfg->verify_count = 1;
    }

    /* remove exceed region */
    for(i=p_ext_cfg->verify_count-1; i>=0; i--)
    {
        if(p_ext_cfg->verify_count == 1)
        {
            /* do not remove the only one */
            break;
        }
    
        //region_end = p_ext_cfg->verify_offset[i] + p_ext_cfg->verify_length[i];
        if(p_ext_cfg->verify_offset[i] > sec_hdr->img_len)
        {
            MSG("[%s] Remove config of offset %d (0x%x)(v3)\n",MOD,
                p_ext_cfg->verify_offset[i],p_ext_cfg->verify_offset[i]);
            p_ext_cfg->verify_count = p_ext_cfg->verify_count - 1;
        }
        else
        {
            break;
        }        
    }

    /* adjust the last region to be aligned with image length */
    i = p_ext_cfg->verify_count-1;
    region_end = p_ext_cfg->verify_offset[i] + p_ext_cfg->verify_length[i];
    if( max_pos < region_end )
    {
        MSG("[%s] The last region's original length is %d (0x%x)\n", MOD,
            p_ext_cfg->verify_length[i], p_ext_cfg->verify_length[i]);
        p_ext_cfg->verify_length[i] = max_pos - p_ext_cfg->verify_offset[i];
        MSG("[%s] Adjust last region's original length to %d (0x%x)\n", MOD,
            p_ext_cfg->verify_length[i], p_ext_cfg->verify_length[i]);
    }

    /* check if need to sign for whole image */
    if(p_ext_cfg->verify_count == 1 && p_ext_cfg->verify_length[0]==0 )
    {        
        MSG("[%s] The sign length is zero, and sign offset is : %d (0x%x)\n",MOD,p_ext_cfg->verify_offset[0],p_ext_cfg->verify_offset[0]);
        p_ext_cfg->verify_length[0] = sec_hdr->img_len - p_ext_cfg->verify_offset[0] ;
        MSG("[%s] Set the sign length to rest whole image length : %d (0x%x)\n",MOD,p_ext_cfg->verify_length[0],p_ext_cfg->verify_length[0]);
    }

    /* check if sign region is inside the image length */
    p_ext_cfg->verify_offset[p_ext_cfg->verify_count] = max_pos;
    for(i=0;i<p_ext_cfg->verify_count;i++)
    {    
        if(sec_hdr->img_len < p_ext_cfg->verify_length[i])
        {
            MSG("[%s] Sign length exceeds image length(v3)\n",MOD);
            return -1;
        }
        
        if(p_ext_cfg->verify_length[i] == 0)
        {
            MSG("[%s] Sign length can't be zero(v3)\n",MOD);
            return -1;
        }
        
        region_end = p_ext_cfg->verify_offset[i] + p_ext_cfg->verify_length[i];

        if(sec_hdr->img_len < region_end)
        {
            MSG("[%s] Sign region exceeds image length(v3)\n",MOD);
            return -1;
        }

        DBG("[%d] min_pos is %d\n", i, min_pos);
        DBG("p_ext_cfg->verify_offset[%d] is %d\n", i, p_ext_cfg->verify_offset[i]);
        DBG("[%d] region_end is %d\n", i, region_end);
        DBG("[%d] next region start is %d\n", i, p_ext_cfg->verify_offset[i+1]);
        /* check if sign region is overlay */
        if((min_pos<=p_ext_cfg->verify_offset[i])&&
            (region_end<=p_ext_cfg->verify_offset[i+1]))
        {
            DBG("[%s] Sign region (%d->%d) ok(v3)\n",MOD,
                p_ext_cfg->verify_offset[i],
                region_end-1);
        }
        else
        {
            MSG("[%s] Sign region is overlap(v3)\n",MOD);
            return -1;
        }

        min_pos = region_end;
    }
    p_ext_cfg->verify_offset[p_ext_cfg->verify_count] = 0;

    if(p_ext_cfg->verify_count == 0)
    {
        MSG("[%s] Sign region count is zero, please check program(v3)\n",MOD);
        return -1;
    }

    return 0;
}


