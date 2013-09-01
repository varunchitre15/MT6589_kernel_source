

#ifndef FTM_CUST_FONT_H
#define FTM_CUST_FONT_H

#include "cust.h"

#if defined(FEATURE_FTM_FONT_36x64)
#define CHAR_WIDTH      36
#define CHAR_HEIGHT     64
#elif defined(FEATURE_FTM_FONT_32x60)
#define CHAR_WIDTH      32
#define CHAR_HEIGHT     60
#elif defined(FEATURE_FTM_FONT_24x44)
#define CHAR_WIDTH      24
#define CHAR_HEIGHT     44
#elif defined(FEATURE_FTM_FONT_16x30)
#define CHAR_WIDTH      16
#define CHAR_HEIGHT     30
#elif defined(FEATURE_FTM_FONT_16x28)
#define CHAR_WIDTH      16
#define CHAR_HEIGHT     28
#elif defined(FEATURE_FTM_FONT_12x22)
#define CHAR_WIDTH      12
#define CHAR_HEIGHT     22
#elif defined(FEATURE_FTM_FONT_10x18)
#define CHAR_WIDTH      10
#define CHAR_HEIGHT     18
#elif defined(FEATURE_FTM_FONT_8x14)
#define CHAR_WIDTH      8
#define CHAR_HEIGHT     14
#elif defined(FEATURE_FTM_FONT_6x10)
#define CHAR_WIDTH      6
#define CHAR_HEIGHT     10

#elif defined(FEATURE_FTM_FONT_72x72)
#define CHAR_WIDTH      72
#define CHAR_HEIGHT     72
#elif defined(FEATURE_FTM_FONT_64x64)
#define CHAR_WIDTH      64
#define CHAR_HEIGHT     64
#elif defined(FEATURE_FTM_FONT_48x48)
#define CHAR_WIDTH      48
#define CHAR_HEIGHT     48
#elif defined(FEATURE_FTM_FONT_32x32)
#define CHAR_WIDTH      32
#define CHAR_HEIGHT     32
#elif defined(FEATURE_FTM_FONT_28x28)
#define CHAR_WIDTH      28
#define CHAR_HEIGHT     28
#elif defined(FEATURE_FTM_FONT_26x26)
#define CHAR_WIDTH      26
#define CHAR_HEIGHT     26
#elif defined(FEATURE_FTM_FONT_24x24)
#define CHAR_WIDTH      24
#define CHAR_HEIGHT     24
#elif defined(FEATURE_FTM_FONT_20x20)
#define CHAR_WIDTH      20
#define CHAR_HEIGHT     20
#elif defined(FEATURE_FTM_FONT_18x18)
#define CHAR_WIDTH      18
#define CHAR_HEIGHT     18
#elif defined(FEATURE_FTM_FONT_16x16)
#define CHAR_WIDTH      16
#define CHAR_HEIGHT     16
#elif defined(FEATURE_FTM_FONT_12x12)
#define CHAR_WIDTH      12
#define CHAR_HEIGHT     12

#else
//#error "font size is not defined"
#if defined(SUPPORT_GB2312)
#define CHAR_WIDTH      26
#define CHAR_HEIGHT     26
#warning "font size is not defined, use default (26x26)"
#else
#define CHAR_WIDTH      12
#define CHAR_HEIGHT     22
#warning "font size is not defined, use default (12x22)"
#endif


#endif

#endif /* FTM_CUST_H */

