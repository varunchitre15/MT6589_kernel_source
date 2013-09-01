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
#ifndef TS_DENSE_MULTIMAP_
#define TS_DENSE_MULTIMAP_

#include "ts_util.h"

// DenseMultimap is imilar to STL multimap, but optimized for memory.
// DenseMultimap objects are immutable after creation.
// All CTORs have linear complexity.
template<class T, int kPreallocatedElements>
class DenseMultimap {
 public:
  typedef const T *const_iterator;

  enum RemoveEnum {REMOVE};

  // Create multimap {t1, t2}
  DenseMultimap(const T &t1, const T &t2) {
    Allocate(2);
    if (t1 < t2) {
      ptr_[0] = t1;
      ptr_[1] = t2;
    } else {
      ptr_[0] = t2;
      ptr_[1] = t1;
    }
    Validate();
  }

  // Create a copy of m.
  DenseMultimap(const DenseMultimap &m) {
    Allocate(m.size());
    copy(m.begin(), m.end(), ptr_);
    Validate();
  }

  // Create multimap m+{t}
  DenseMultimap(const DenseMultimap &m, const T &t) {
    Allocate(m.size() + 1);
    const_iterator it = lower_bound(m.begin(), m.end(), t);
    copy(m.begin(), it, ptr_);
    ptr_[it - m.begin()] = t;
    copy(it, m.end(), ptr_ + (it - m.begin()) + 1);
    Validate();
  }

  // Create multimap m-{t}
  DenseMultimap(const DenseMultimap &m, RemoveEnum remove, const T &t) {
    const_iterator it = lower_bound(m.begin(), m.end(), t);
    CHECK(it < m.end() && it >= m.begin());
    Allocate(m.size() - 1);
    copy(m.begin(), it, ptr_);
    copy(it + 1, m.end(), ptr_ + (it - m.begin()));
    Validate();
  }

  ~DenseMultimap() {
    if (size_ > kPreallocatedElements) {
      CHECK(ptr_ != (T*)&array_);
      delete [] ptr_;
    } else {
      CHECK(ptr_ == (T*)&array_);
    }
  }

  size_t size() const { return size_; }

  const T &operator [] (size_t i) const {
    CHECK(i < size());
    return ptr_[i];
  }

  const_iterator begin() const { return ptr_; }
  const_iterator end()   const { return ptr_ + size(); }

  bool has(const T&t) const {
    return binary_search(begin(), end(), t);
  }

  bool operator < (const DenseMultimap &m) const {
    if (size() != m.size()) return size() < m.size();
    for (size_t i = 0; i < size(); i++) {
      if (ptr_[i] != m.ptr_[i])
        return ptr_[i] < m.ptr_[i];
    }
    return false;
  }

 private:

  void Allocate(int required_size) {
    size_ = required_size;
    if (size_ <= kPreallocatedElements) {
      ptr_ = (T*)&array_;
    } else {
      ptr_ = new T[size_];
    }
  }

  void Validate() {
    for (size_t i = 1; i < size(); i++) {
      CHECK(ptr_[i-1] <= ptr_[i]);
    }
  }

  T *ptr_;
  int size_;
  T array_[kPreallocatedElements];
};

#endif  // TS_DENSE_MULTIMAP_
