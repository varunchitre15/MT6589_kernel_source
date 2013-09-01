/* ThreadSanitizer
 * Copyright (c) 2011, Google Inc. All rights reserved.
 * Author: Dmitry Vyukov (dvyukov)
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

#ifndef TS_ATOMIC_INT_H_INCLUDED
#define TS_ATOMIC_INT_H_INCLUDED

#include "ts_atomic.h"
#include "ts_util.h"
#include <stddef.h>

// Helper functions for atomic support

char const* tsan_atomic_to_str(tsan_memory_order mo);
char const* tsan_atomic_to_str(tsan_atomic_op op);
bool tsan_atomic_is_acquire(tsan_memory_order mo);
bool tsan_atomic_is_release(tsan_memory_order mo);
bool tsan_atomic_is_rmw(tsan_atomic_op op);
void tsan_atomic_verify(tsan_atomic_op op,
                        tsan_memory_order mo,
                        tsan_memory_order fail_mo,
                        size_t size,
                        void volatile* a);
uint64_t tsan_atomic_do_op(tsan_atomic_op op,
                           tsan_memory_order mo,
                           tsan_memory_order fail_mo,
                           size_t size,
                           void volatile* a,
                           uint64_t v,
                           uint64_t cmp,
                           uint64_t* newv,
                           uint64_t* prev);

#endif // #ifndef TS_ATOMIC_INT_H_INCLUDED

