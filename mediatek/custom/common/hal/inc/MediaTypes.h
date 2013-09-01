#ifndef _MEDIA_TYPES_H
#define _MEDIA_TYPES_H

/*******************************************************************************
*
*   !!! Remove Me If Possible
*   !!! Do Not Include Me If Possible
*   !!! Do Not Modify Me If Possbile
*   !!! Make sure the compatibility with the original one.
*   !!! Do Not Rename "_MEDIA_TYPES_H" Unless No Side Effect.
*
********************************************************************************/

/*******************************************************************************
*
********************************************************************************/
//
typedef unsigned char   u8;
typedef unsigned short  u16;
typedef unsigned int    u32;
//
typedef void            MHAL_VOID;
typedef char            MHAL_BOOL;
typedef char            MHAL_CHAR;
typedef signed char     MHAL_INT8;
typedef signed short    MHAL_INT16;
typedef signed int      MHAL_INT32;
typedef unsigned char   MHAL_UCHAR;
typedef unsigned char   MHAL_UINT8;
typedef unsigned short  MHAL_UINT16;
typedef unsigned int    MHAL_UINT32;
//
typedef MHAL_VOID       MVOID;
typedef MHAL_UINT8      MUINT8;
typedef MHAL_UINT16     MUINT16;
typedef MHAL_UINT32     MUINT32;
typedef MHAL_INT32      MINT32;

/*******************************************************************************
*
********************************************************************************/
#define READ32(addr)        *(MUINT32 *) (addr)
#define WRITE32(addr, val)  *(MUINT32 *) (addr) = (val)

#define MHAL_TRUE     1
#define MHAL_FALSE    0

/*******************************************************************************
*
********************************************************************************/
//typedef signed char         CHAR;
//typedef char                UCHAR;
#define CHAR                signed char
#define UCHAR               char
typedef signed char         INT8;
typedef unsigned char       UINT8;
typedef unsigned short      UINT16;
typedef signed short        INT16;
//typedef signed int          BOOL;
#define BOOL                signed int
//typedef signed int          INT32;
#define INT32               signed int
typedef unsigned int        UINT32;
typedef long long           INT64;
typedef unsigned long long  UINT64;
typedef float               FLOAT;
typedef double              DOUBLE;
typedef void                VOID;

typedef INT32 MRESULT;

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef NULL
#define NULL 0
#endif

/*******************************************************************************
*
********************************************************************************/

/**
 * @par Enumeration
 *   MHAL_ERROR_ENUM
 * @par Description
 *   This is the return status of each MHAL function
 */
typedef enum
{
    MHAL_NO_ERROR = 0,                  ///< The function work successfully
    MHAL_INVALID_DRIVER,                ///< Error due to invalid driver
    MHAL_INVALID_CTRL_CODE,             ///< Error due to invalid control code
    MHAL_INVALID_PARA,                  ///< Error due to invalid parameter
    MHAL_INVALID_MEMORY,                ///< Error due to invalid memory
    MHAL_INVALID_FORMAT,                ///< Error due to invalid file format
    MHAL_INVALID_RESOURCE,              ///< Error due to invalid resource, like IDP

    MHAL_UNKNOWN_ERROR = 0x80000000,    ///< Unknown error
    MHAL_ALL = 0xFFFFFFFF
} MHAL_ERROR_ENUM;


#endif // _MEDIA_TYPES_H


