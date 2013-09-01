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
// Author: Timur Iskhodzhanov.

// This file contains utility classes and functions used by ThreadSanitizer.
// TODO(kcc): move more utilities from thread_sanitizer.cc to this file.

#ifndef TS_UTIL_H_
#define TS_UTIL_H_

//--------- Head ------------------- {{{1
#if defined(TS_VALGRIND)
# define CHECK tl_assert
#elif defined(TS_PIN)
extern void Printf(const char *format, ...);
extern void ThreadSanitizerDumpAllStacks();
# define CHECK(x) do { if (!(x)) { \
   Printf("Assertion failed: %s (%s:%d) %s\n", \
          __FUNCTION__, __FILE__, __LINE__, #x); \
   ThreadSanitizerDumpAllStacks(); \
   exit(1); }} while ((void)0, 0)
#elif defined(TS_OFFLINE)
extern unsigned long offline_line_n;
# define CHECK(x) do { if (!(x)) { \
    Printf("ASSERT on line %ld\n", offline_line_n); \
     assert(x);}} while ((void)0, 0)
#else
# define CHECK assert
#endif

// support for stlport in stlp_std:: namespace (or other custom ns)
#ifdef TS_STL_NS
# define STD TS_STL_NS 
#else
# define STD std
#endif

#if defined(TS_VALGRIND)
# include "ts_valgrind.h"
# define TS_USE_STLPORT
#if defined(VGP_arm_linux)
// This macro is explicitly undefined in glibc for ARM.
#define _GLIBCXX_USE_C99 1
#endif  // ARM

// __WORDSIZE is GLibC-specific. Get it from Valgrind if needed.
#if !defined(__WORDSIZE)
#if VG_WORDSIZE == 4
#define __WORDSIZE 32
#elif VG_WORDSIZE == 8
#define __WORDSIZE 64
#endif // VG_WORDSIZE
#endif // TS_VALGRIND && !__WORDSIZE

#elif defined(TS_LLVM)
#  define TS_USE_STLPORT
# include <assert.h>
# include <fcntl.h>
# include <time.h>

#elif defined(__GNUC__)
# undef NDEBUG  // Assert is always on.
# include <assert.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
# define TS_USE_GNUC_STL

#elif defined(_MSC_VER)
# undef NDEBUG  // Assert is always on.
# include <assert.h>
# include <stdio.h>
# include <intrin.h>
# define TS_USE_WIN_STL

#else
# error "Unknown configuration"
#endif

//--------- STL ------------------- {{{1
#if defined(TS_USE_GNUC_STL)  // ----------- g++ STL -----------
#include <string.h>
#include <limits.h>
#include <set>
#include <map>
#include <vector>
#include <deque>
#include <stack>
#include <algorithm>
#include <string>
#include <bitset>
#include <new>
#include <ext/algorithm>

#ifdef __APPLE__
// Apple's unordered_map in gcc 4.0 does not support -fno-exceptions.
#include "ext/hash_map"
#include "ext/hash_set"
#define unordered_map __gnu_cxx::hash_map
#define unordered_set __gnu_cxx::hash_set
#else
#include "tr1/unordered_map"
#include "tr1/unordered_set"
using STD::tr1::unordered_map;
using STD::tr1::unordered_set;
#endif

#elif defined(TS_USE_STLPORT)  // ------------- STLport ----------
#include "set"
#include "map"
#include "hash_map"
#include "hash_set"
#include "vector"
#include "deque"
#include "stack"
#include "algorithm"
#include "string"
#include "bitset"
#include "algorithm"
#include "new"

#include "unordered_map"
#include "unordered_set"
using STD::tr1::unordered_map;
using STD::tr1::unordered_set;

#elif defined(TS_USE_WIN_STL)  // ------------- MSVC STL ---------
#include <string.h>
#include <limits.h>
#include <set>
#include <map>
#include <vector>
#include <deque>
#include <stack>
#include <algorithm>
#include <string>
#include <bitset>
#include <new>

// No such thing in VC 2005
//#include <unordered_map>
//#include <unordered_set>
//using std::tr1::unordered_map;
//using std::tr1::unordered_set;
#include <hash_map>
#include <hash_set>
#define unordered_map stdext::hash_map
#define unordered_set stdext::hash_set

#else
# error "Unknown STL"
#endif  // TS_USE_STANDARD_STL

using STD::string;
using STD::set;
using STD::multiset;
using STD::multimap;
using STD::map;
using STD::deque;
using STD::stack;
using STD::vector;
using STD::bitset;
using STD::nothrow_t;
using STD::nothrow;

using STD::min;
using STD::max;
using STD::sort;
using STD::pair;
using STD::make_pair;
using STD::unique_copy;
using STD::count;
using STD::set_intersection;
using STD::lower_bound;
using STD::copy;
using STD::binary_search;

#ifdef TS_LLVM
# include "tsan_rtl_wrap.h"
#endif

//--------- defines ------------------- {{{1
#ifdef TS_VALGRIND
// TODO(kcc) get rid of these macros.
#define sprintf(arg1, arg2...) VG_(sprintf)((Char*)arg1, (HChar*)arg2)
#define vsnprintf(a1, a2, a3, a4) VG_(vsnprintf)((Char*)a1, a2, a3, a4)
#define getpid VG_(getpid)
#define strchr(a,b)    VG_(strchr)((Char*)a,b)
#define strdup(a) (char*)VG_(strdup)((HChar*)"strdup", (const Char*)a)
#define snprintf(a,b,c...)     VG_(snprintf)((Char*)a,b,c)
#define read VG_(read)
#define getenv(x) VG_(getenv)((Char*)x)
#define close VG_(close)
#define write VG_(write)
#define usleep(a) /*nothing. TODO.*/

#elif defined(__GNUC__)
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>

#define UNLIKELY(x) __builtin_expect((x), 0)
#define LIKELY(x)   __builtin_expect(!!(x), 1)

#elif defined(_MSC_VER)
typedef __int8 int8_t;
typedef __int16 int16_t;
typedef __int32 int32_t;
typedef __int64 int64_t;
typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;

typedef int pthread_t;
int getpid();
#define snprintf _snprintf
#define strtoll strtol  // TODO(kcc): _MSC_VER hmm...
#define UNLIKELY(x) (x)  // TODO(kcc): how to say this in MSVC?
#define LIKELY(x)   (x)

#else
# error "Unknown configuration"
#endif // TS_VALGRIND

#define CHECK_GT(X, Y) CHECK((X) >  (Y))
#define CHECK_LT(X, Y) CHECK((X) < (Y))
#define CHECK_GE(X, Y) CHECK((X) >= (Y))
#define CHECK_LE(X, Y) CHECK((X) <= (Y))
#define CHECK_NE(X, Y) CHECK((X) != (Y))
#define CHECK_EQ(X, Y) CHECK((X) == (Y))

#if defined(DEBUG) && DEBUG >= 1
  #define DCHECK(a) CHECK(a)
  #define DEBUG_MODE (1)
#else
  #define DCHECK(a) do { if (0) { if (a) {} } } while((void)0, 0)
  #define DEBUG_MODE (0)
#endif

#ifndef ALWAYS_INLINE
  #if defined (__GNUC__)
    #define ALWAYS_INLINE  inline __attribute__ ((always_inline))
  #elif defined(_MSC_VER)
    #define ALWAYS_INLINE __forceinline
  #else
    #error "Unknown Configuration"
  #endif
#endif

#if defined(DEBUG) && DEBUG >= 1
  #define INLINE
  #define NOINLINE
#elif defined (__GNUC__)
  #define INLINE  ALWAYS_INLINE
  #define NOINLINE __attribute__ ((noinline))
#elif defined(_MSC_VER)
  #define INLINE ALWAYS_INLINE
  #define NOINLINE __declspec(noinline)
#else
  #error "Unknown Configuration"
#endif

// When TS_SERIALIZED==1, all calls to ThreadSanitizer* functions
// should be serialized somehow. For example:
//  - Valgrind serializes threads by using a pipe-based semaphore.
//  - ThreadSanitizerOffline is single-threaded by nature.
//  - A Multi-threaded environment (e.g. PIN) can use a single global Mutex.
// When TS_SERIALIZED==0, ThreadSanitizer takes care of synchronization itself.

#if defined(TS_SERIALIZED)
 // someone defined this already, leave it as is.
#elif defined(TS_PIN)
# define TS_SERIALIZED 1
#elif defined(TS_LLVM)
# define TS_SERIALIZED 0
#elif defined(TS_GO)
# define TS_SERIALIZED 0
#else
# define TS_SERIALIZED 1
#endif


#define TS_ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

//--------- Malloc profiling ------------------- {{{1
class MallocCostCenterStack {
 public:
  void Push(const char *cc) {
    malloc_cost_centers_[size_++] = cc;
  }
  void Pop() {
    size_--;
  }
  const char *Top() {
    return size_ ? malloc_cost_centers_[size_ - 1] : "default_cc";
  }
 private:
  enum { kMaxMallocStackSize = 100 };
  int size_;
  const char *malloc_cost_centers_[kMaxMallocStackSize];
};

// Not thread-safe. Need to make it thread-local if we allow
// malloc to be called concurrently.
extern MallocCostCenterStack g_malloc_stack;

class ScopedMallocCostCenter {
 public:
  ScopedMallocCostCenter(const char *cc) {
#if defined(TS_VALGRIND)
    g_malloc_stack.Push(cc);
#endif
  }
  ~ScopedMallocCostCenter() {
#if defined(TS_VALGRIND)
    g_malloc_stack.Pop();
#endif
  }
};

//--------- Forward decls ------------------- {{{1
class ThreadSanitizerReport;

// Time since some moment before the program start.
extern size_t TimeInMilliSeconds();
extern void YIELD();
extern void PROCESSOR_YIELD();

extern "C" long my_strtol(const char *str, char **end, int base);
extern void Printf(const char *format, ...);

// Strip (.*) and <.*>, also handle "function returns a function pointer" case.
string NormalizeFunctionName(const string &mangled_fname);

string ReadFileToString(const string &file_name, bool die_if_failed);

// Get the current memory footprint of myself (parse /proc/self/status).
size_t GetVmSizeInMb();

// Sets the contents of the file 'file_name' to 'str'.
void OpenFileWriteStringAndClose(const string &file_name, const string &str);

// If host_and_port looks like myhost:12345, open a socket for writing
// and returns a FILE object. Retuns NULL on failure.
FILE *OpenSocketForWriting(const string &host_and_port);

// If addr is inside a global object, returns true and sets 'name' and 'offset'
bool GetNameAndOffsetOfGlobalObject(uintptr_t addr,
                                    string *name, uintptr_t *offset);

extern uintptr_t GetPcOfCurrentThread();

extern void GetThreadStack(int tid, uintptr_t *min_addr, uintptr_t *max_addr);

extern void SetNumberOfFoundErrors(int n_errs);
extern int GetNumberOfFoundErrors();

bool LiteRaceSkipTrace(int tid, uint32_t trace_no, uint32_t sampling_rate);


inline uintptr_t tsan_bswap(uintptr_t x) {
#if defined(VGP_arm_linux) && __WORDSIZE == 64
  return __builtin_bswap64(x);
#elif defined(VGP_arm_linux) && __WORDSIZE == 32
  return __builtin_bswap32(x);
#elif defined(__GNUC__) && __WORDSIZE == 64
  __asm__("bswapq %0" : "=r" (x) : "0" (x));
  return x;
#elif defined(__GNUC__) && __WORDSIZE == 32
  __asm__("bswapl %0" : "=r" (x) : "0" (x));
  return x;
#elif defined(_WIN32)
  return x;  // TODO(kcc)
#else
# error  "Unknown Configuration"
#endif // arch && VG_WORDSIZE
}

#ifdef _MSC_VER
inline unsigned u32_log2(unsigned x) {
  unsigned long y;
  _BitScanReverse(&y, x);
  return y;
}
#endif

#ifdef __GNUC__
inline unsigned u32_log2(unsigned x) {
  return 31 - __builtin_clz(x);
}
#endif

typedef unsigned prng_t;

/// Simple stand-alone pseudorandom number generator.
/// Current algorithm is ANSI C linear congruential PRNG.
inline unsigned tsan_prng(prng_t* state) {
  return (*state = *state * 1103515245 + 12345) >> 16;
}


#endif  // TS_UTIL_H_
// end. {{{1
// vim:shiftwidth=2:softtabstop=2:expandtab:tw=80
