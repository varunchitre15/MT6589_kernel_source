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
//
// Some libc functions are implemented in a way unfriendly to race detectors
// and memcheck-like tools.
// E.g. strlen() may read up to 7 bytes past the allocated buffer.
// To avoid false positives in these functions, the tool needs to replace these
// funcions with simpler implementation.
//
// The includer must define these macros:
// REPORT_WRITE_RANGE, REPORT_READ_RANGE, EXTRA_REPLACE_PARAMS,
// EXTRA_REPLACE_ARGS, NOINLINE
// See ts_valgrind_intercepts.c and ts_pin.cc.

#ifndef TS_REPLACE_H_
#define TS_REPLACE_H_

static NOINLINE char *Replace_memchr(EXTRA_REPLACE_PARAMS const char *s,
                                     int c, size_t n) {
  size_t i;
  char *ret = 0;
  for (i = 0; i < n; i++) {
    if (s[i] == (char)c) {
      ret = (char*)(&s[i]);
      break;
    }
  }
  REPORT_READ_RANGE(s, ret ? i + 1 : n);
  return ret;
}

static NOINLINE char *Replace_strchr(EXTRA_REPLACE_PARAMS const char *s,
                                     int c) {
  size_t i;
  char *ret = 0;
  for (i = 0; ; i++) {
    if (s[i] == (char)c) {
      ret = (char*)(&s[i]);
      break;
    }
    if (s[i] == 0) break;
  }
  REPORT_READ_RANGE(s, i + 1);
  return ret;
}

static NOINLINE char *Replace_strchrnul(EXTRA_REPLACE_PARAMS const char *s,
                                        int c) {
  size_t i;
  char *ret;
  for (i = 0; ; i++) {
    if (s[i] == (char)c || s[i] == 0) {
      ret = (char*)(&s[i]);
      break;
    }
  }
  REPORT_READ_RANGE(s, i + 1);
  return ret;
}

static NOINLINE char *Replace_strrchr(EXTRA_REPLACE_PARAMS const char *s,
                                      int c) {
  char* ret = 0;
  size_t i;
  for (i = 0; ; i++) {
    if (s[i] == (char)c) {
      ret = (char*)&s[i];
    }
    if (s[i] == 0) break;
  }
  REPORT_READ_RANGE(s, i + 1);
  return ret;
}

static NOINLINE size_t Replace_strlen(EXTRA_REPLACE_PARAMS const char *s) {
  size_t i = 0;
  for (i = 0; s[i]; i++) {
  }
  REPORT_READ_RANGE(s, i + 1);
  return i;
}

static NOINLINE char *Replace_memcpy(EXTRA_REPLACE_PARAMS char *dst,
                                     const char *src, size_t len) {
  size_t i;
  for (i = 0; i < len; i++) {
    dst[i] = src[i];
  }
  REPORT_READ_RANGE(src, i);
  REPORT_WRITE_RANGE(dst, i);
  return dst;
}

static NOINLINE char *Replace_memmove(EXTRA_REPLACE_PARAMS char *dst,
                                     const char *src, size_t len) {

  size_t i;
  if (dst < src) {
    for (i = 0; i < len; i++) {
      dst[i] = src[i];
    }
  } else {
    for (i = 0; i < len; i++) {
      dst[len - i - 1] = src[len - i - 1];
    }
  }
  REPORT_READ_RANGE(src, i);
  REPORT_WRITE_RANGE(dst, i);
  return dst;
}

static NOINLINE int Replace_memcmp(EXTRA_REPLACE_PARAMS const unsigned char *s1,
                                     const unsigned char *s2, size_t len) {
  size_t i;
  int res = 0;
  for (i = 0; i < len; i++) {
    if (s1[i] != s2[i]) {
      res = (int)s1[i] - (int)s2[i];
      break;
    }
  }
  REPORT_READ_RANGE(s1, min(i + 1, len));
  REPORT_READ_RANGE(s2, min(i + 1, len));
  return res;
}

static NOINLINE char *Replace_strcpy(EXTRA_REPLACE_PARAMS char *dst,
                                     const char *src) {
  size_t i;
  for (i = 0; src[i]; i++) {
    dst[i] = src[i];
  }
  dst[i] = 0;
  REPORT_READ_RANGE(src, i + 1);
  REPORT_WRITE_RANGE(dst, i + 1);
  return dst;
}

static NOINLINE char *Replace_stpcpy(EXTRA_REPLACE_PARAMS char *dst,
                                     const char *src) {
  size_t i;
  for (i = 0; src[i]; i++) {
    dst[i] = src[i];
  }
  dst[i] = 0;
  REPORT_READ_RANGE(src, i + 1);
  REPORT_WRITE_RANGE(dst, i + 1);
  return dst + i;
}

static NOINLINE char *Replace_strncpy(EXTRA_REPLACE_PARAMS char *dst,
                                     const char *src, size_t n) {
  size_t i;
  for (i = 0; i < n; i++) {
    dst[i] = src[i];
    if (src[i] == 0) break;
  }
  REPORT_READ_RANGE(src, min(i + 1, n));
  while (i < n) {
    dst[i] = 0;
    i++;
  }
  REPORT_WRITE_RANGE(dst, n);
  return dst;
}


static NOINLINE int Replace_strcmp(EXTRA_REPLACE_PARAMS const char *s1,
                                   const char *s2) {
  unsigned char c1;
  unsigned char c2;
  size_t i;
  for (i = 0; ; i++) {
    c1 = (unsigned char)s1[i];
    c2 = (unsigned char)s2[i];
    if (c1 != c2) break;
    if (c1 == 0) break;
  }
  REPORT_READ_RANGE(s1, i+1);
  REPORT_READ_RANGE(s2, i+1);
  if (c1 < c2) return -1;
  if (c1 > c2) return 1;
  return 0;
}

static NOINLINE int Replace_strncmp(EXTRA_REPLACE_PARAMS const char *s1,
                                    const char *s2, size_t n) {
  unsigned char c1 = 0;
  unsigned char c2 = 0;
  size_t i;
  for (i = 0; i < n; i++) {
    c1 = (unsigned char)s1[i];
    c2 = (unsigned char)s2[i];
    if (c1 != c2) break;
    if (c1 == 0) break;
  }
  REPORT_READ_RANGE(s1, min(i + 1, n));
  REPORT_READ_RANGE(s2, min(i + 1, n));
  if (c1 < c2) return -1;
  if (c1 > c2) return 1;
  return 0;
}

static NOINLINE char *Replace_strcat(EXTRA_REPLACE_PARAMS char *dest,
                                     const char *src) {
  size_t dest_len = Replace_strlen(EXTRA_REPLACE_ARGS dest);
  Replace_strcpy(EXTRA_REPLACE_ARGS dest + dest_len, src);
  return dest;
}

#if defined(TS_VALGRIND)
// Read every byte in the memory range.
static NOINLINE void ReadMemory(const void* p, size_t size) {
  const volatile char* start = (const volatile char*)p;
  const volatile char* end = start + size;
  volatile char tmp = 0;
  for (; start < end; ++start) {
    // If we just read the bytes, Valgrind will optimize it out.
    tmp ^= *start;
  }
}

// Read every byte in the null-terminated string.
static NOINLINE void ReadString(const char* s) {
  const volatile char* p = (const volatile char*)s;
  volatile char tmp = 0;
  char c;
  for (; (c = *p); ++p) {
    tmp ^= c;
  }
}
#endif   // TS_VALGRIND

#endif  // TS_REPLACE_H_
