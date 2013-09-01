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

#ifndef TS_EVENTS_H_
#define TS_EVENTS_H_

// Each event contains tid (the id of the current thread).
// Most events contain pc (the program counter).
// Some events contain:
//  * addr, a memory address, a lock address, etc
//  * size of a memory range
// Few events contain a string (e.g. SET_THREAD_NAME).

enum EventType {
  NOOP,               // Should not appear.
  READ,               // {tid, pc, addr, size}
  WRITE,              // {tid, pc, addr, size}
  READER_LOCK,        // {tid, pc, lock, 0}
  WRITER_LOCK,        // {tid, pc, lock, 0}
  UNLOCK,             // {tid, pc, lock, 0}
  UNLOCK_OR_INIT,     // {tid, pc, lock, 0}
  LOCK_CREATE,        // {tid, pc, lock, 0}
  LOCK_DESTROY,       // {tid, pc, lock, 0}
  THR_CREATE_BEFORE,  // Parent thread's event. {tid, pc, 0, 0}
  THR_CREATE_AFTER,   // Parent thread's event. {tid, 0, 0, child_tid}
  THR_START,          // Child thread's event {tid, CallStack, 0, parent_tid}
  THR_FIRST_INSN,     // Used only by valgrind.
  THR_END,            // {tid, 0, 0, 0}
  THR_JOIN_AFTER,     // {tid, pc, joined_tid}
  THR_STACK_TOP,      // {tid, pc, stack_top, stack_size_if_known}
  RTN_EXIT,           // {tid, 0, 0, 0}
  RTN_CALL,           // {tid, pc, 0, 0}
  SBLOCK_ENTER,       // {tid, pc, 0, 0}
  SIGNAL,             // {tid, pc, obj, 0}
  WAIT,               // {tid, pc, obj, 0}
  CYCLIC_BARRIER_INIT,         // {tid, pc, obj, n}
  CYCLIC_BARRIER_WAIT_BEFORE,  // {tid, pc, obj, 0}
  CYCLIC_BARRIER_WAIT_AFTER,   // {tid, pc, obj, 0}
  PCQ_CREATE,         // {tid, pc, pcq_addr, 0}
  PCQ_DESTROY,        // {tid, pc, pcq_addr, 0}
  PCQ_PUT,            // {tid, pc, pcq_addr, 0}
  PCQ_GET,            // {tid, pc, pcq_addr, 0}
  STACK_MEM_DIE,      // deprecated.
  MALLOC,             // {tid, pc, addr, size}
  FREE,               // {tid, pc, addr, 0}
  MMAP,               // {tid, pc, addr, size}
  MUNMAP,             // {tid, pc, addr, size}
  PUBLISH_RANGE,      // may be deprecated later.
  UNPUBLISH_RANGE,    // deprecated. TODO(kcc): get rid of this.
  HB_LOCK,            // {tid, pc, addr, 0}
  NON_HB_LOCK,        // {tid, pc, addr, 0}
  IGNORE_READS_BEG,   // {tid, pc, 0, 0}
  IGNORE_READS_END,   // {tid, pc, 0, 0}
  IGNORE_WRITES_BEG,  // {tid, pc, 0, 0}
  IGNORE_WRITES_END,  // {tid, pc, 0, 0}
  SET_THREAD_NAME,    // {tid, pc, name_str, 0}
  SET_LOCK_NAME,      // {tid, pc, lock, lock_name_str}
  TRACE_MEM,          // {tid, pc, addr, 0}
  EXPECT_RACE,        // {tid, descr_str, ptr, size}
  BENIGN_RACE,        // {tid, descr_str, ptr, size}
  EXPECT_RACE_BEGIN,  // {tid, pc, 0, 0}
  EXPECT_RACE_END,    // {tid, pc, 0, 0}
  VERBOSITY,          // Used for debugging.
  STACK_TRACE,        // {tid, pc, 0, 0}, for debugging.
  FLUSH_STATE,        // {tid, pc, 0, 0}
  PC_DESCRIPTION,     // {0, pc, descr_str, 0}, for ts_offline.
  PRINT_MESSAGE,      // {tid, pc, message_str, 0}, for ts_offline.
  FLUSH_EXPECTED_RACES,  // {0, 0, 0, 0}
  LAST_EVENT          // Should not appear.
};

#include "ts_event_names.h"  // generated from this file by sed.

class Event {
 public:
  Event(EventType type, int32_t tid, uintptr_t pc, uintptr_t a, uintptr_t info)
      : type_(type),
      tid_(tid),
      pc_(pc),
      a_(a),
      info_(info) {
      }
  Event() {}  // Not initialized.

  void Init(EventType type, int32_t tid, uintptr_t pc, uintptr_t a, uintptr_t info) {
    type_ = type;
    tid_  = tid;
    pc_   = pc;
    a_    = a;
    info_ = info;
  }


  EventType type()  const { return type_; }
  int32_t   tid()   const { return tid_; }
  uintptr_t a()     const { return a_; }
  uintptr_t pc()    const { return pc_; }
  uintptr_t info()  const { return info_; }
  void      Print() const {
    Printf("T%d: %s [pc=%p; a=%p; i=%p]\n",
           tid(), TypeString(type()), pc(), a(), info());

  }
  static const char *TypeString(EventType type) {
    return kEventNames[type];
  }
 private:
  EventType      type_;
  int32_t   tid_;
  uintptr_t pc_;
  uintptr_t a_;
  uintptr_t info_;
};


// end. {{{1
#endif  // TS_EVENTS_H_
// vim:shiftwidth=2:softtabstop=2:expandtab:tw=80
