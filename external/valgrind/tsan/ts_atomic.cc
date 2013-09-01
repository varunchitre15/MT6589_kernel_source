/* ThreadSanitizer
 * Copyright (c) 2011, Google Inc. All rights reserved.
 * Author: Dmitry Vyukov (dvyukov)
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

#include "ts_util.h"
#include "ts_atomic_int.h"


char const* tsan_atomic_to_str(tsan_memory_order mo) {
  switch (mo) {
    case tsan_memory_order_invalid: return "invalid";
    case tsan_memory_order_natomic: return "natomic";
    case tsan_memory_order_relaxed: return "relaxed";
    case tsan_memory_order_consume: return "consume";
    case tsan_memory_order_acquire: return "acquire";
    case tsan_memory_order_release: return "release";
    case tsan_memory_order_acq_rel: return "acq_rel";
    case tsan_memory_order_seq_cst: return "seq_cst";
    default: return "-------";
  }
}


char const* tsan_atomic_to_str(tsan_atomic_op op) {
  switch (op) {
    case tsan_atomic_op_invalid: return "invalid";
    case tsan_atomic_op_fence: return "fence";
    case tsan_atomic_op_load: return "load";
    case tsan_atomic_op_store: return "store";
    case tsan_atomic_op_exchange: return "exchange";
    case tsan_atomic_op_fetch_add: return "fetch_add";
    case tsan_atomic_op_fetch_sub: return "fetch_sub";
    case tsan_atomic_op_fetch_and: return "fetch_and";
    case tsan_atomic_op_fetch_xor: return "fetch_xor";
    case tsan_atomic_op_fetch_or: return "fetch_or";
    case tsan_atomic_op_compare_exchange_weak: return "compare_exchange_weak";
    case tsan_atomic_op_compare_exchange_strong:
      return "compare_exchange_strong";
    default: return "---";
  }
}


bool tsan_atomic_is_acquire(tsan_memory_order mo) {
  return !!(mo & (tsan_memory_order_consume
      | tsan_memory_order_acquire
      | tsan_memory_order_acq_rel
      | tsan_memory_order_seq_cst));
}


bool tsan_atomic_is_release(tsan_memory_order mo) {
  return !!(mo & (tsan_memory_order_release
      | tsan_memory_order_acq_rel
      | tsan_memory_order_seq_cst));
}


bool tsan_atomic_is_rmw(tsan_atomic_op op) {
  return !!(op & (tsan_atomic_op_exchange
      | tsan_atomic_op_fetch_add
      | tsan_atomic_op_fetch_sub
      | tsan_atomic_op_fetch_and
      | tsan_atomic_op_fetch_xor
      | tsan_atomic_op_fetch_or
      | tsan_atomic_op_compare_exchange_weak
      | tsan_atomic_op_compare_exchange_strong));
}


void tsan_atomic_verify(tsan_atomic_op op,
                        tsan_memory_order mo,
                        tsan_memory_order fail_mo,
                        size_t size,
                        void volatile* a) {
  CHECK(size == 1 || size == 2 || size == 4 || size == 8);
  CHECK((((uintptr_t)a) % size) == 0);

  if (op == tsan_atomic_op_load) {
    CHECK(mo & (tsan_memory_order_natomic
        | tsan_memory_order_relaxed
        | tsan_memory_order_consume
        | tsan_memory_order_acquire
        | tsan_memory_order_seq_cst));
  } else if (op == tsan_atomic_op_store) {
    CHECK(mo & (tsan_memory_order_natomic
        | tsan_memory_order_relaxed
        | tsan_memory_order_release
        | tsan_memory_order_seq_cst));
  } else if (op == tsan_atomic_op_fence) {
    CHECK(mo & (tsan_memory_order_consume
        | tsan_memory_order_acquire
        | tsan_memory_order_release
        | tsan_memory_order_acq_rel
        | tsan_memory_order_seq_cst));
  } else if (op & (tsan_atomic_op_exchange
        | tsan_atomic_op_fetch_add
        | tsan_atomic_op_fetch_sub
        | tsan_atomic_op_fetch_and
        | tsan_atomic_op_fetch_xor
        | tsan_atomic_op_fetch_or
        | tsan_atomic_op_compare_exchange_weak
        | tsan_atomic_op_compare_exchange_strong)) {
    CHECK(mo & (tsan_memory_order_relaxed
        | tsan_memory_order_consume
        | tsan_memory_order_acquire
        | tsan_memory_order_release
        | tsan_memory_order_acq_rel
        | tsan_memory_order_seq_cst));
  } else {
    CHECK("unknown tsan_atomic_op" == 0);
  }
}


#if defined(__i386__)
# define __x86__
#elif defined(__x86_64__)
# define __x86__
#endif

#if defined(__GNUC__) && defined(__x86_64__)
uint64_t tsan_atomic_do_op(tsan_atomic_op op,
                           tsan_memory_order mo,
                           tsan_memory_order fail_mo,
                           size_t size,
                           void volatile* a,
                           uint64_t v,
                           uint64_t cmp,
                           uint64_t* newv,
                           uint64_t* prev) {
  *newv = v;
  if (op != tsan_atomic_op_fence) {
    if (size == 1) {
      *prev = *(uint8_t volatile*)a;
    } else if (size == 2) {
      *prev =  *(uint16_t volatile*)a;
    } else if (size == 4) {
      *prev =  *(uint32_t volatile*)a;
    } else if (size == 8) {
      *prev =  *(uint64_t volatile*)a;
    }
  }

  if (op == tsan_atomic_op_load) {
    return *prev;

  } else if (op == tsan_atomic_op_store) {
    if (mo == tsan_memory_order_seq_cst) {
      if (size == 1) {
        uint8_t vv = (uint8_t)v;
        __asm__ __volatile__ ("xchgb %1, %0"
            : "=r" (vv) : "m" (*(uint8_t volatile*)a), "0" (vv));
        *prev = vv;
      } else if (size == 2) {
        uint16_t vv = (uint16_t)v;
        __asm__ __volatile__ ("xchgw %1, %0"
            : "=r" (vv) : "m" (*(uint16_t volatile*)a), "0" (vv));
        *prev = vv;
      } else if (size == 4) {
        uint32_t vv = (uint32_t)v;
        __asm__ __volatile__ ("xchgl %1, %0"
            : "=r" (vv) : "m" (*(uint32_t volatile*)a), "0" (vv));
        *prev = vv;
      } else if (size == 8) {
#ifdef __x86_64__
        uint64_t vv = (uint64_t)v;
        __asm__ __volatile__ ("xchgq %1, %0"
            : "=r" (vv) : "m" (*(uint64_t volatile*)a), "0" (vv));
        *prev = vv;
#else
#error "IMPLEMENT ME, PLZ"
        //uint64_t cmp = *a;
        //!!!while (!tsan_atomic64_compare_exchange_strong(a, &cmp, v, mo, mo))
        //!!! {}
#endif
      }
    } else {
      if (size == 1) {
        *(uint8_t volatile*)a = v;
      } else if (size == 2) {
        *(uint16_t volatile*)a = v;
      } else if (size == 4) {
        *(uint32_t volatile*)a = v;
      } else if (size == 8) {
        *(uint64_t volatile*)a = v;
      }
    }
    return 0;

  } else if (op == tsan_atomic_op_exchange) {
    if (size == 1) {
      uint8_t vv = (uint8_t)v;
      __asm__ __volatile__ ("xchgb %1, %0"
          : "=r" (vv) : "m" (*(uint8_t volatile*)a), "0" (vv));
      *prev = vv;
      return vv;
    } else if (size == 2) {
      uint16_t vv = (uint16_t)v;
      __asm__ __volatile__ ("xchgw %1, %0"
          : "=r" (vv) : "m" (*(uint16_t volatile*)a), "0" (vv));
      *prev = vv;
      return vv;
    } else if (size == 4) {
      uint32_t vv = (uint32_t)v;
      __asm__ __volatile__ ("xchgl %1, %0"
          : "=r" (vv) : "m" (*(uint32_t volatile*)a), "0" (vv));
      *prev = vv;
      return vv;
    } else if (size == 8) {
# ifdef __x86_64__
      uint64_t vv = (uint64_t)v;
      __asm__ __volatile__ ("xchgq %1, %0"
          : "=r" (vv) : "m" (*(uint64_t volatile*)a), "0" (vv));
      *prev = vv;
      return vv;
#else
#error "IMPLEMENT ME, PLZ"
      //uint64_t cmp = *a;
      //while (!tsan_atomic64_compare_exchange_strong(a, &cmp, v, mo, mo))
      // {}
      //return cmp;
#endif
    }

  } else if (op == tsan_atomic_op_fetch_add) {
    if (size == 1) {
      uint8_t prevv = __sync_fetch_and_add((uint8_t volatile*)a, (uint8_t)v);
      *prev = prevv;
      *newv = prevv + (uint8_t)v;
      return prevv;
    } else if (size == 2) {
      uint16_t prevv = __sync_fetch_and_add(
          (uint16_t volatile*)a, (uint16_t)v);
      *prev = prevv;
      *newv = prevv + (uint16_t)v;
      return prevv;
    } else if (size == 4) {
      uint32_t prevv = __sync_fetch_and_add(
          (uint32_t volatile*)a, (uint32_t)v);
      *prev = prevv;
      *newv = prevv + (uint32_t)v;
      return prevv;
    } else if (size == 8) {
      uint64_t prevv = __sync_fetch_and_add(
          (uint64_t volatile*)a, (uint64_t)v);
      *prev = prevv;
      *newv = prevv + v;
      return prevv;
    }

  } else if (op == tsan_atomic_op_fetch_sub) {
    if (size == 1) {
      uint8_t prevv = __sync_fetch_and_sub(
          (uint8_t volatile*)a, (uint8_t)v);
      *prev = prevv;
      *newv = prevv - (uint8_t)v;
      return prevv;
    } else if (size == 2) {
      uint16_t prevv = __sync_fetch_and_sub(
          (uint16_t volatile*)a, (uint16_t)v);
      *prev = prevv;
      *newv = prevv - (uint16_t)v;
      return prevv;
    } else if (size == 4) {
      uint32_t prevv = __sync_fetch_and_sub(
          (uint32_t volatile*)a, (uint32_t)v);
      *prev = prevv;
      *newv = prevv - (uint32_t)v;
      return prevv;
    } else if (size == 8) {
      uint64_t prevv = __sync_fetch_and_sub(
          (uint64_t volatile*)a, (uint64_t)v);
      *prev = prevv;
      *newv = prevv - v;
      return prevv;
    }

  } else if (op == tsan_atomic_op_fetch_and) {
    if (size == 1) {
      uint8_t prevv = __sync_fetch_and_and(
          (uint8_t volatile*)a, (uint8_t)v);
      *prev = prevv;
      *newv = prevv & (uint8_t)v;
      return prevv;
    } else if (size == 2) {
      uint16_t prevv = __sync_fetch_and_and(
          (uint16_t volatile*)a, (uint16_t)v);
      *prev = prevv;
      *newv = prevv & (uint16_t)v;
      return prevv;
    } else if (size == 4) {
      uint32_t prevv = __sync_fetch_and_and(
          (uint32_t volatile*)a, (uint32_t)v);
      *prev = prevv;
      *newv = prevv & (uint32_t)v;
      return prevv;
    } else if (size == 8) {
      uint64_t prevv = __sync_fetch_and_and(
          (uint64_t volatile*)a, (uint64_t)v);
      *prev = prevv;
      *newv = prevv & v;
      return prevv;
    }

  } else if (op == tsan_atomic_op_fetch_xor) {
    if (size == 1) {
      uint8_t prevv = __sync_fetch_and_xor(
          (uint8_t volatile*)a, (uint8_t)v);
      *prev = prevv;
      *newv = prevv ^ (uint8_t)v;
      return prevv;
    } else if (size == 2) {
      uint16_t prevv = __sync_fetch_and_xor(
          (uint16_t volatile*)a, (uint16_t)v);
      *prev = prevv;
      *newv = prevv ^ (uint16_t)v;
      return prevv;
    } else if (size == 4) {
      uint32_t prevv = __sync_fetch_and_xor(
          (uint32_t volatile*)a, (uint32_t)v);
      *prev = prevv;
      *newv = prevv ^ (uint32_t)v;
      return prevv;
    } else if (size == 8) {
      uint64_t prevv = __sync_fetch_and_xor(
          (uint64_t volatile*)a, (uint64_t)v);
      *prev = prevv;
      *newv = prevv ^ v;
      return prevv;
    }

  } else if (op == tsan_atomic_op_fetch_or) {
    if (size == 1) {
      uint8_t prevv = __sync_fetch_and_or(
          (uint8_t volatile*)a, (uint8_t)v);
      *prev = prevv;
      *newv = prevv | (uint8_t)v;
      return prevv;
    } else if (size == 2) {
      uint16_t prevv = __sync_fetch_and_or(
          (uint16_t volatile*)a, (uint16_t)v);
      *prev = prevv;
      *newv = prevv | (uint16_t)v;
      return prevv;
    } else if (size == 4) {
      uint32_t prevv = __sync_fetch_and_or(
          (uint32_t volatile*)a, (uint32_t)v);
      *prev = prevv;
      *newv = prevv | (uint32_t)v;
      return prevv;
    } else if (size == 8) {
      uint64_t prevv = __sync_fetch_and_or(
          (uint64_t volatile*)a, (uint64_t)v);
      *prev = prevv;
      *newv = prevv | v;
      return prevv;
    }

  } else if (op == tsan_atomic_op_compare_exchange_strong
          || op == tsan_atomic_op_compare_exchange_weak) {
    uint64_t prevv = 0;
    if (size == 1) {
      prevv = __sync_val_compare_and_swap((uint8_t volatile*)a, cmp, v);
    } else if (size == 2) {
      prevv = __sync_val_compare_and_swap((uint16_t volatile*)a, cmp, v);
    } else if (size == 4) {
      prevv = __sync_val_compare_and_swap((uint32_t volatile*)a, cmp, v);
    } else if (size == 8) {
      prevv = __sync_val_compare_and_swap((uint64_t volatile*)a, cmp, v);
    }
    *prev = prevv;
    return prevv;

  } else if (op == tsan_atomic_op_fence) {
    if (mo == tsan_memory_order_seq_cst)
      __sync_synchronize();
    return 0;
  }

  CHECK("unknown atomic operation" == 0);
  return 0;
}

#else

uint64_t tsan_atomic_do_op(tsan_atomic_op op,
                           tsan_memory_order mo,
                           tsan_memory_order fail_mo,
                           size_t size,
                           void volatile* a,
                           uint64_t v,
                           uint64_t cmp,
                           uint64_t* newv,
                           uint64_t* prev) {
  CHECK(!"IMPLEMENTED" == 0);
  return 0;
}

#endif









