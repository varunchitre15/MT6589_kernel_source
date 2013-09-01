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

#ifndef TS_LOCK_H_
#define TS_LOCK_H_

#include "ts_util.h"

#if (DEBUG > 0) && (TS_SERIALIZED == 0) && defined (TS_LLVM) && !defined(DYNAMIC_ANNOTATIONS_ENABLED)
# define DYNAMIC_ANNOTATIONS_ENABLED 1
#endif
#include "dynamic_annotations.h"

//--------- Simple Lock ------------------ {{{1
#if defined(TS_VALGRIND) || defined(TS_OFFLINE)
class TSLock {
 public:
  void Lock() {};
  void Unlock() {};
  void AssertHeld() {};
};
#else
class TSLock {
 public:
  TSLock();
  ~TSLock();
  void Lock();
  void Unlock();
  void AssertHeld();
 private:
  struct Rep;
  Rep *rep_;
};
#endif

class ScopedLock {
 public:
  ScopedLock(TSLock *lock)
    : lock_(lock) {
    lock_->Lock();
  }
  ~ScopedLock() { lock_->Unlock(); }
 private:
  TSLock *lock_;
};

//--------- Atomic operations {{{1
#if TS_SERIALIZED == 1
// No need for atomics when all ThreadSanitizer logic is serialized.
ALWAYS_INLINE uintptr_t AtomicExchange(uintptr_t *ptr, uintptr_t new_value) {
  uintptr_t old_value = *ptr;
  *ptr = new_value;
  return old_value;
}

ALWAYS_INLINE void ReleaseStore(uintptr_t *ptr, uintptr_t value) {
  *ptr = value;
}

ALWAYS_INLINE int32_t NoBarrier_AtomicIncrement(int32_t* ptr) {
  return *ptr += 1;
}

ALWAYS_INLINE int32_t NoBarrier_AtomicDecrement(int32_t* ptr) {
  return *ptr -= 1;
}

#elif defined(__GNUC__)

ALWAYS_INLINE uintptr_t AtomicExchange(uintptr_t *ptr, uintptr_t new_value) {
  return __sync_lock_test_and_set(ptr, new_value);
}

ALWAYS_INLINE void ReleaseStore(uintptr_t *ptr, uintptr_t value) {
  __asm__ __volatile__("" : : : "memory");
  *(volatile uintptr_t*)ptr = value;
}

ALWAYS_INLINE int32_t NoBarrier_AtomicIncrement(int32_t* ptr) {
  return __sync_add_and_fetch(ptr, 1);
}

ALWAYS_INLINE int32_t NoBarrier_AtomicDecrement(int32_t* ptr) {
  return __sync_sub_and_fetch(ptr, 1);
}

#elif defined(_MSC_VER)
uintptr_t AtomicExchange(uintptr_t *ptr, uintptr_t new_value);
void ReleaseStore(uintptr_t *ptr, uintptr_t value);
int32_t NoBarrier_AtomicIncrement(int32_t* ptr);
int32_t NoBarrier_AtomicDecrement(int32_t* ptr);

#else
# error "unsupported configuration"
#endif


ALWAYS_INLINE int32_t AtomicIncrementRefcount(int32_t *refcount) {
  return NoBarrier_AtomicIncrement(refcount);
}

ALWAYS_INLINE int32_t AtomicDecrementRefcount(int32_t *refcount) {
  ANNOTATE_HAPPENS_BEFORE(refcount);
  int32_t res = NoBarrier_AtomicDecrement(refcount);
  if (res == 0) {
    ANNOTATE_HAPPENS_AFTER(refcount);
  }
  return res;
}



// end. {{{1
#endif  // TS_LOCK_H_
// vim:shiftwidth=2:softtabstop=2:expandtab:tw=80
