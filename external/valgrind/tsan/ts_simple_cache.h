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
#ifndef TS_SIMPLE_CACHE_
#define TS_SIMPLE_CACHE_

#include "ts_util.h"

// Few simple 'cache' classes.
// -------- PtrToBoolCache ------ {{{1
// Maps a pointer to a boolean.
template <int kSize>
class PtrToBoolCache {
 public:
  PtrToBoolCache() {
    Flush();
  }
  void Flush() {
    memset(this, 0, sizeof(*this));
  }
  void Insert(uintptr_t ptr, bool val) {
    size_t idx  = ptr % kSize;
    arr_[idx] = ptr;
    if (val) {
      bits_[idx / 32] |= 1U << (idx % 32);
    } else {
      bits_[idx / 32] &= ~(1U << (idx % 32));
    }
  }
  bool Lookup(uintptr_t ptr, bool *val) {
    size_t idx  = ptr % kSize;
    if (arr_[idx] == ptr) {
      *val = (bits_[idx / 32] >> (idx % 32)) & 1;
      return true;
    }
    return false;
  }
 private:
  uintptr_t arr_[kSize];
  uint32_t bits_[(kSize + 31) / 32];
};

// -------- IntPairToBoolCache ------ {{{1
// Maps two integers to a boolean.
// The second integer should be less than 1^31.
template <int32_t kSize>
class IntPairToBoolCache {
 public:
  IntPairToBoolCache() {
    Flush();
  }
  void Flush() {
    memset(arr_, 0, sizeof(arr_));
  }
  void Insert(uint32_t a, uint32_t b, bool val) {
    DCHECK((int32_t)b >= 0);
    uint32_t i = idx(a, b);
    if (val) {
      b |= 1U << 31;
    }
    arr_[i * 2 + 0] = a;
    arr_[i * 2 + 1] = b;
  }
  bool Lookup(uint32_t a, uint32_t b, bool *val) {
    DCHECK((int32_t)b >= 0);
    uint32_t i = idx(a, b);
    if (arr_[i * 2] != a) return false;
    uint32_t maybe_b = arr_[i * 2 + 1];
    if (b == (maybe_b & (~(1U << 31)))) {
      *val = (maybe_b & (1U << 31)) != 0;
      return true;
    }
    return false;
  }
 private:
  uint32_t idx(uint32_t a, uint32_t b) {
    return (a ^ ((b >> 16) | (b << 16))) % kSize;
  }
  uint32_t arr_[kSize * 2];
};

// end. {{{1
#endif  // TS_SIMPLE_CACHE_
// vim:shiftwidth=2:softtabstop=2:expandtab:tw=80
