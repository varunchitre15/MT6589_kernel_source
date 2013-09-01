/*	$OpenBSD: bcopy.c,v 1.5 2005/08/08 08:05:37 espie Exp $ */
/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
   
#include <string.h>

/*
 * sizeof(word) MUST BE A POWER OF TWO
 * SO THAT wmask BELOW IS ALL ONES
 */
typedef	long word;		/* "word" used for optimal copy speed */

#define	wsize	sizeof(word)
#define	wmask	(wsize - 1)

/*
 * Copy a block of memory, handling overlap.
 * This is the routine that actually implements
 * (the portable versions of) bcopy, memcpy, and memmove.
 */
#ifdef MEMCOPY
void *
memcpy(void *dst0, const void *src0, size_t length)
#else
#ifdef MEMMOVE
void *
memmove(void *dst0, const void *src0, size_t length)
#else

#include <machine/cpu-features.h>
#define VFP_COPY_LENGTH_THRESHOLD (1024)
extern void vfp_copy_forward_not_align(void *dst, void *src, size_t length);
extern void vfp_copy_backward_not_align(void *dst, void *src, size_t length,int flags);
 
void
bcopy(const void *src0, void *dst0, size_t length)
#endif
#endif
{
	char *dst = dst0;
	const char *src = src0;
	size_t t;

	if (length == 0 || dst == src)		/* nothing to do */
		goto done;

	/*
	 * Macros: loop-t-times; and loop-t-times, t>0
	 */
#define	TLOOP(s) if (t) TLOOP1(s)
#define	TLOOP1(s) do { s; } while (--t)
	if ((unsigned long)dst < (unsigned long)src) {
		/*
		 * Copy forward.
		 */
		t = (long)src;	/* only need low bits */
		if ((t | (long)dst) & wmask) {
			/*
			 * Try to align operands.  This cannot be done
			 * unless the low bits match.
			 */
                        if (length < wsize) {
                                t = length;
                                TLOOP1(*dst++ = *src++);
                                goto done;
                        }

                        /*
                         * Src and dst not at same byte offset 
                         */
			if ((t ^ (long)dst) & wmask) {
				t = length;
#if !defined (MEMCOPY)
#if defined(__ARM_NEON__)
                                memcpy(dst0, src0, length);
                                goto done;
#else /* BCOPY without NEON */
                                memcpy(dst0, src0, length);
                                goto done;
#endif /* __ARM_NEON__ */
#endif /* !MEMCOPY */
			} else
				t = wsize - (t & wmask);
			length -= t;
			TLOOP1(*dst++ = *src++);
		}
		/*
		 * Copy whole words, then mop up any trailing bytes.
		 */
		t = length / wsize;
		TLOOP(*(word *)dst = *(word *)src; src += wsize; dst += wsize);
		t = length & wmask;
		TLOOP(*dst++ = *src++);  
	} else {
		/*
		 * Copy backwards.  Otherwise essentially the same.
		 * Alignment works as before, except that it takes
		 * (t&wmask) bytes to align, not wsize-(t&wmask).
		 */
		src += length;
		dst += length;
		t = (long)src; 
		if ((t | (long)dst) & wmask) {
                        if (length < wsize) {
                                t = length;
                                TLOOP1(*--dst = *--src);
                                goto done;
                        }

                        /** Src and dst not at same byte offset*/

			if ((t ^ (long)dst) & wmask) {
				t = length;  
#if !defined (MEMCOPY)  
#if defined(__ARM_NEON__)
                                if (length >= VFP_COPY_LENGTH_THRESHOLD) {
                                        unsigned long bytes, i, len;
                                        /* Copy trailing bytes, align dst to 16 bytes */
                                        /* Copy trailing bytes, align src to 32 bytes */
                                        bytes = ((unsigned long)dst) & 15;  
                                        if (bytes > 0) {
                                                for (i = 0; i < bytes; i++) 
                                                        *--dst = *--src;
                                        }     
  
                                        /* Utilize VFP to copy multiple of 32 bytes */
                                        /* Utilize VFP to copy multiple of 64 bytes */ 
                                        len = (length - bytes) & ~31;//~63;//~31;  
                                        vfp_copy_backward_not_align((void *) dst, (void *) src, len,(int)len&0x20/*Check If src 64bytes alignment only*/);

                                        /* Copy remaining bytes */   
                                        src -= len; 
                                        dst -= len;
                                        bytes = ((unsigned long) src) - ((unsigned long) src0);
                                        if (bytes > 0) {
                                                for (i = 0; i < bytes; i++)
                                                        *--dst = *--src;
                                        } 
                                        goto done;
                                }
#endif /* __ARM_NEON__ */
#endif /* !MEMCOPY */
			} else
				t &= wmask;
			length -= t;
			TLOOP1(*--dst = *--src);
		}
		t = length / wsize;
		TLOOP(src -= wsize; dst -= wsize; *(word *)dst = *(word *)src);
		t = length & wmask;
		TLOOP(*--dst = *--src);
	}
done:
#if defined(MEMCOPY) || defined(MEMMOVE)
	return (dst0);
#else
	return;
#endif
}
