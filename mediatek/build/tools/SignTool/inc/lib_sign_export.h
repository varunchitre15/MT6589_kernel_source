
#ifndef _LIB_SIGN_EXPORT_HEADER_H
#define _LIB_SIGN_EXPORT_HEADER_H

extern int chk_img (char *img_name);
extern int imp_key (char *kf, char *kp, char gen_hdr, FILE *gen_fd);
extern int gen_hdr (char *cfg_name, char *hdr_name, char* img_name, char *hs_name);
extern int pro_img(char *hs_name, char *img_name,char *hdr_name,char sparse_header,char *ext_sparse_name);
extern int pro_fb_sig(char *input_img, char *cfg_name, char *output_hdr, char *original_img_name);

#endif

