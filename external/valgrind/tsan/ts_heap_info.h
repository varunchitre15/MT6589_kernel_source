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
#ifndef TS_HEAP_INFO_
#define TS_HEAP_INFO_

#include "ts_util.h"

// Information about heap memory.
// For each heap allocation we create a struct HeapInfo.
// This struct should have fields 'uintptr_t ptr' and 'uintptr_t size',
// a default CTOR and a copy CTOR.

template<class HeapInfo>
class HeapMap {
 public:
  typedef map<uintptr_t, HeapInfo> map_t;
  typedef typename map_t::iterator iterator;

  HeapMap() { Reset(); }

  iterator begin() { return ++map_.begin(); }
  iterator end() { return --map_.end(); }

  size_t size() { return map_.size() - 2; }

  void InsertInfo(uintptr_t a, HeapInfo info) {
    CHECK(IsValidPtr(a));
    CHECK(info.ptr == a);
    map_[a] = info;
  }

  void EraseInfo(uintptr_t a) {
    CHECK(IsValidPtr(a));
    map_.erase(a);
  }

  void EraseRange(uintptr_t start, uintptr_t end) {
    CHECK(IsValidPtr(start));
    CHECK(IsValidPtr(end));
    // TODO(glider): the [start, end) range may cover several map_ records.
    EraseInfo(start);
  }

  HeapInfo *GetInfo(uintptr_t a) {
    CHECK(this);
    CHECK(IsValidPtr(a));
    typename map_t::iterator it = map_.lower_bound(a);
    CHECK(it != map_.end());
    if (it->second.ptr == a) {
      // Exact match. 'a' is the beginning of a heap-allocated address.
      return &it->second;
    }
    CHECK(a < it->second.ptr);
    CHECK(it != map_.begin());
    // not an exact match, try the previous iterator.
    --it;
    HeapInfo *info = &it->second;
    CHECK(info->ptr < a);
    if (info->ptr + info->size > a) {
      // within the range.
      return info;
    }
    return NULL;
  }

  void Clear() {
    map_.clear();
    Reset();
  }

 private:
  bool IsValidPtr(uintptr_t a) {
    return a != 0 && a != (uintptr_t) -1;
  }
  void Reset() {
    // Insert a maximal and minimal possible values to make GetInfo simpler.
    HeapInfo max_info;
    memset(&max_info, 0, sizeof(HeapInfo));
    max_info.ptr = (uintptr_t)-1;
    map_[max_info.ptr] = max_info;

    HeapInfo min_info;
    memset(&min_info, 0, sizeof(HeapInfo));
    map_[min_info.ptr] = min_info;
  }
  map_t map_;
};

#endif  // TS_HEAP_INFO_
