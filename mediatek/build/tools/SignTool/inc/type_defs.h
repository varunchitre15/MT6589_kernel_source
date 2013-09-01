#ifndef _TYPE_DEFS_HEADER_H
#define _TYPE_DEFS_HEADER_H

/**************************************************************************
 *  TYPEDEF
 **************************************************************************/
typedef unsigned int uint32;
typedef unsigned char uchar;
typedef unsigned long ulong;
#ifndef __cplusplus
typedef unsigned char bool;
#endif

/**************************************************************************
 *  MCARO
 **************************************************************************/
#define MSG                         printf

/**************************************************************************
 *  DEFINITIONS
 **************************************************************************/
#define TRUE                        (1)
#define FALSE                       (0)

/**************************************************************************
 *  COMPILE ASSERT
 **************************************************************************/
#define COMPILE_ASSERT(condition) ((void)sizeof(char[1 - 2*!!!(condition)]))

#endif

