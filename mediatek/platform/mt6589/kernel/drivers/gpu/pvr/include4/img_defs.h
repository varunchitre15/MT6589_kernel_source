/*************************************************************************/ /*!
@Title          Common header containing type definitions for portability
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Contains variable and structure definitions. Any platform
                specific types should be defined in this file.
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/
#if !defined (__IMG_DEFS_H__)
#define __IMG_DEFS_H__

#include "img_types.h"

typedef		enum	img_tag_TriStateSwitch
{
	IMG_ON		=	0x00,
	IMG_OFF,
	IMG_IGNORE

} img_TriStateSwitch, * img_pTriStateSwitch;

#define		IMG_SUCCESS				0

#define		IMG_NO_REG				1

#if defined (NO_INLINE_FUNCS)
	#define	INLINE
	#define	FORCE_INLINE
#else
#if defined (__cplusplus)
	#define INLINE					inline
	#define	FORCE_INLINE			inline
#else
#if	!defined(INLINE)
	#define	INLINE					__inline
#endif
	#define	FORCE_INLINE			static __inline
#endif
#endif


/* Use this in any file, or use attributes under GCC - see below */
#ifndef PVR_UNREFERENCED_PARAMETER
#define	PVR_UNREFERENCED_PARAMETER(param) (param) = (param)
#endif

/* The best way to supress unused parameter warnings using GCC is to use a
 * variable attribute.  Place the unref__ between the type and name of an
 * unused parameter in a function parameter list, eg `int unref__ var'. This
 * should only be used in GCC build environments, for example, in files that
 * compile only on Linux. Other files should use UNREFERENCED_PARAMETER */
#ifdef __GNUC__
#define unref__ __attribute__ ((unused))
#else
#define unref__
#endif

/*
	Wide character definitions
*/
#ifndef _TCHAR_DEFINED
#if defined(UNICODE)
typedef unsigned short		TCHAR, *PTCHAR, *PTSTR;
#else	/* #if defined(UNICODE) */
typedef char				TCHAR, *PTCHAR, *PTSTR;
#endif	/* #if defined(UNICODE) */
#define _TCHAR_DEFINED
#endif /* #ifndef _TCHAR_DEFINED */


			#if defined(__linux__) || defined(__QNXNTO__) || defined(__METAG)

				#define IMG_CALLCONV
				#define IMG_INTERNAL	__attribute__((visibility("hidden")))
				#define IMG_EXPORT		__attribute__((visibility("default")))
				#define IMG_IMPORT
				#define IMG_RESTRICT	__restrict__

			#else
					#error("define an OS")
			#endif

// Use default definition if not overridden
#ifndef IMG_ABORT
	#define IMG_ABORT()	abort()
#endif

#ifndef IMG_MALLOC
	#define IMG_MALLOC(A)		malloc	(A)
#endif

#ifndef IMG_FREE
	#define IMG_FREE(A)			free	(A)
#endif

#define IMG_CONST const

#if defined(__GNUC__)
#define IMG_FORMAT_PRINTF(x,y)		__attribute__((format(printf,x,y)))
#else
#define IMG_FORMAT_PRINTF(x,y)
#endif

/*
 * Cleanup request defines
  */
#define  CLEANUP_WITH_POLL		IMG_FALSE
#define  FORCE_CLEANUP			IMG_TRUE

#if defined (_WIN64)
#define IMG_UNDEF	(~0ULL)
#else
#define IMG_UNDEF	(~0UL)
#endif

#endif /* #if !defined (__IMG_DEFS_H__) */
/*****************************************************************************
 End of file (IMG_DEFS.H)
*****************************************************************************/
