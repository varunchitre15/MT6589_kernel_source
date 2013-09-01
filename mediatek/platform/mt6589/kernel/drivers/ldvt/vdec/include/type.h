
#ifndef _TYPE_H
#define _TYPE_H

#define ULONG unsigned long
#define UINT  unsigned int
#define UCHAR unsigned char

#define LONG long
#define INT  int
#define CHAR char

#define UINT64 unsigned long long
#define UINT_32 unsigned long
#define UINT_16 unsigned short
#define UINT_8 unsigned char

#define INT64  long long

#define DWRD UINT32
#define WORD UINT16

#ifndef NULL
#define NULL  0
#endif

#define HANDLE_T UINT32

#if !defined(TRUE)
    #define TRUE true
#endif

#if !defined(FALSE)
    #define FALSE false
#endif

#define IN
#define OUT

#define TCHAR char

#define CString char*

#define LPSTR   char*
#define LPCTSTR char*

#define DLL_FUNC

#define TEXT

#define BIT(n)                          ((UINT_32) 1 << (n))
#define BITS(m,n)                       (~(BIT(m)-1) & ((BIT(n) - 1) | BIT(n)))

#endif
