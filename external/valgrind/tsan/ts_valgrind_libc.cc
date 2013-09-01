/* Copyright (c) 2008-2010, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// This file is part of ThreadSanitizer, a dynamic data race detector.
// Author: Konstantin Serebryany.
// Implement some of the libc functions to link with valgrind.
// Do not include any linux header here to avoid conflicts.
//
extern "C" {
#include "pub_tool_basics.h"
#include "pub_tool_libcbase.h"
}

// can't use VG_(memmove) since it is buggy.
extern "C" void * memmove(void *a, const void *b, unsigned long size) {
  char *A = (char*)a;
  const char *B = (const char*)b;
  if (A < B) {
    for (unsigned long i = 0; i < size; i++) {
      A[i] = B[i];
    }
  } else if(A > B) {
    for (unsigned long i = 0; i < size; i++) {
      A[size - i - 1] = B[size - i - 1];
    }
  }
  return a;
}

extern "C" int memcmp(const void *a, const void *b, unsigned long c) {
  return VG_(memcmp)(a,b,c);
}
#ifndef VGO_darwin
extern "C" void* __memcpy_chk(void *dest, const void *src, unsigned long n) {
   return VG_(memcpy)(dest,src,n);
}
#endif
