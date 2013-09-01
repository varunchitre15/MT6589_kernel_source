
#ifndef _AAA_TYPES_H_
#define _AAA_TYPES_H_

/*******************************************************************************
*
*******************************************************************************/
typedef unsigned char       MUINT8;
typedef unsigned short      MUINT16;
typedef unsigned int        MUINT32;
typedef unsigned long long  MUINT64;
//
typedef signed char         MINT8;
typedef signed short        MINT16;
typedef signed int          MINT32;
typedef signed long long    MINT64;
//
typedef float		    MFLOAT;
typedef double		    MDOUBLE;
//
typedef void                MVOID;
typedef int                 MBOOL;
#ifndef MTRUE
    #define MTRUE           1
#endif
#ifndef MFALSE
    #define MFALSE          0
#endif
#ifndef MNULL
    #define MNULL           0
#endif
/*******************************************************************************
*
*******************************************************************************/
#endif // _AAA_TYPES_H_

