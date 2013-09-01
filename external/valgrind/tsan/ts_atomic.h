/* ThreadSanitizer
 * Copyright (c) 2011, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
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

#ifndef TS_ATOMIC_H_INCLUDED
#define TS_ATOMIC_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif


// These constants mostly mimic ones from C++0x standard draft.
// The most recent version of the draft (as of now) can be found here:
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2011/n3242.pdf
// Check out fresh versions here:
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/
// Refer to sections 1.10 and 29.
//
// tsan_memory_order_invalid has no meaning other than invalid enum value.
// tsan_memory_order_natomic stands for "non atomic" and expresses
// as if plain memory access that is not intended to race
// with other accesses.
typedef enum tsan_memory_order {
  tsan_memory_order_invalid = 0,
  tsan_memory_order_natomic = 1 << 0,
  tsan_memory_order_relaxed = 1 << 1,
  tsan_memory_order_consume = 1 << 2,
  tsan_memory_order_acquire = 1 << 3,
  tsan_memory_order_release = 1 << 4,
  tsan_memory_order_acq_rel = 1 << 5,
  tsan_memory_order_seq_cst = 1 << 6
} tsan_memory_order;


// These constants express types of atomic memory operations
// as defined by C++0x standard draft (section 29).
//
// tsan_atomic_op_invalid has no meaning other than invalid enum value.
// compare_exchange_weak differs from compare_exchange_strong in that
// it can fail spuriously.
typedef enum tsan_atomic_op {
  tsan_atomic_op_invalid = 0,
  tsan_atomic_op_fence = 1 << 0,
  tsan_atomic_op_load = 1 << 1,
  tsan_atomic_op_store = 1 << 2,
  tsan_atomic_op_exchange = 1 << 3,
  tsan_atomic_op_fetch_add = 1 << 4,
  tsan_atomic_op_fetch_sub = 1 << 5,
  tsan_atomic_op_fetch_and  = 1 << 6,
  tsan_atomic_op_fetch_xor = 1 << 7,
  tsan_atomic_op_fetch_or = 1 << 8,
  tsan_atomic_op_compare_exchange_weak = 1 << 9,
  tsan_atomic_op_compare_exchange_strong = 1 << 10
} tsan_atomic_op;


#ifdef __cplusplus
}
#endif

#endif // #ifndef TS_ATOMIC_H_INCLUDED


