#ifndef __TV_DVT_DATATYPES_H__
#define __TV_DVT_DATATYPES_H__

#include "mt6575_tvrot_drv.h"
/*-----------------------------------------------------------------------------
    DATA STRUCTURE
  -----------------------------------------------------------------------------*/


typedef struct
{
    unsigned long w;
    unsigned long h;
} TvSize;

typedef struct
{
    long    x;
    long    y;
} TvPoint;

typedef struct
{
    long            x;
    long            y;
    unsigned long   w;
    unsigned long   h;
}TvRect;

typedef struct
{
    unsigned long   y;
    unsigned long   u;
    unsigned long   v;
}TvYuvAddr;



typedef struct
{
    unsigned long       src_img_addr;
    TvSize              src_img_size;
    TvRect              src_img_roi;
    unsigned long       dst_img_addr;
    TvSize              dst_img_size;
    TvRect              dst_img_roi;
    unsigned long       dst_img_num;
    TVR_FORMAT          dst_fmt;
    TVR_ROT             bRotate:2;//0:0, 1:90, 2:180, 3:270
    unsigned long       bFlip:1; //0:non-flip ,1:flip
    unsigned long       bAuto:1;
} TVR_DVT_PATH_PARAM;



#endif
