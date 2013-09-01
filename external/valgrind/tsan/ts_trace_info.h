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
// Information about one TRACE (single-entry-multiple-exit region of code).
#ifndef TS_TRACE_INFO_
#define TS_TRACE_INFO_

#include "ts_util.h"
// Information about one Memory Operation.
//
// A memory access is represented by mop[idx] = {pc,size,is_write}
// which is computed at instrumentation time and {actual_address} computed
// at run-time. The instrumentation insn looks like
//  tleb[idx] = actual_address
// The create_sblock field tells if we want to remember the stack trace
// which corresponds to this Mop (i.e. create an SBLOCK).
struct MopInfo {
 public:
  MopInfo(uintptr_t pc, size_t size, bool is_write, bool create_sblock) {
    DCHECK(sizeof(*this) == 8);
    pc_ = pc;
    if (size > 16) size = 16; // some instructions access more than 16 bytes.
    size_minus1_ = size - 1;
    is_write_ = is_write;
    create_sblock_ = create_sblock;

    DCHECK(size != 0);
    DCHECK(this->size() == size);
    DCHECK(this->is_write() == is_write);
    DCHECK(this->create_sblock() == create_sblock);
  }

  MopInfo() {
    DCHECK(sizeof(*this) == 8);
    memset(this, 0, sizeof(*this));
  }

  uintptr_t pc()            { return pc_; };
  size_t    size()          { return size_minus1_ + 1; }
  bool      is_write()      { return is_write_; }
  bool      create_sblock() { return create_sblock_; }

 private:
  uint64_t  pc_           :58;  // 48 bits is enough for pc, even on x86-64.
  uint64_t  create_sblock_ :1;
  uint64_t  is_write_      :1;
  uint64_t  size_minus1_   :4;  // 0..15
};

// ---------------- Lite Race ------------------
// Experimental!
//
// The idea was first introduced in LiteRace:
// http://www.cs.ucla.edu/~dlmarino/pubs/pldi09.pdf
// Instead of analyzing all memory accesses, we do sampling.
// For each trace (single-enry muliple-exit region) we maintain a counter of
// executions. If a trace has been executed more than a certain threshold, we
// start skipping this trace sometimes.
// The LiteRace paper suggests several strategies for sampling, including
// thread-local counters. Having thread local counters for all threads is too
// expensive, so we have kLiteRaceNumTids arrays of counters and use
// the array (tid % 8).
//
// sampling_rate indicates the level of sampling.
// 0 means no sampling.
// 1 means handle *almost* all accesses.
// ...
// 31 means very aggressive sampling (skip a lot of accesses).

//
// Note: ANNOTATE_PUBLISH_MEMORY() does not work with sampling... :(

struct LiteRaceCounters {
  uint32_t counter;
  int32_t num_to_skip;
};

struct TraceInfoPOD {
  enum { kLiteRaceNumTids = 8 };
  enum { kLiteRaceStorageSize = 8 };
  typedef LiteRaceCounters LiteRaceStorage[kLiteRaceNumTids][kLiteRaceStorageSize];

  size_t n_mops_;
  size_t pc_;
  size_t counter_;
  LiteRaceStorage *literace_storage;
  int32_t storage_index;
  MopInfo mops_[1];
};

// An instance of this class is created for each TRACE (SEME region)
// during instrumentation.
class TraceInfo : public TraceInfoPOD {
 public:
  static TraceInfo *NewTraceInfo(size_t n_mops, uintptr_t pc);
  void DeleteTraceInfo(TraceInfo *trace_info) {
    delete [] (uintptr_t*)trace_info;
  }
  MopInfo *GetMop(size_t i) {
    DCHECK(i < n_mops_);
    return &mops_[i];
  }

  size_t n_mops() const { return n_mops_; }
  size_t pc()     const { return pc_; }
  size_t &counter()     { return counter_; }
  MopInfo *mops()       { return mops_; }

  static void PrintTraceProfile();

  INLINE bool LiteRaceSkipTraceQuickCheck(uintptr_t tid_modulo_num) {
    DCHECK(tid_modulo_num < kLiteRaceNumTids);
    // Check how may accesses are left to skip. Racey, but ok.
    LiteRaceCounters *counters =
        &((*literace_storage)[tid_modulo_num][storage_index]);
    int32_t num_to_skip = --counters->num_to_skip;
    if (num_to_skip > 0) {
      return true;
    }
    return false;
  }

  INLINE void LiteRaceUpdate(uintptr_t tid_modulo_num, uint32_t sampling_rate) {
    DCHECK(sampling_rate < 32);
    DCHECK(sampling_rate > 0);
    LiteRaceCounters *counters =
        &((*literace_storage)[tid_modulo_num][storage_index]);
    uint32_t cur_counter = counters->counter;
    // The bigger the counter the bigger the number of skipped accesses.
    int32_t next_num_to_skip = (cur_counter >> (32 - sampling_rate)) + 1;
    counters->num_to_skip = next_num_to_skip;
    counters->counter = cur_counter + next_num_to_skip;

  }

  // TODO(glider): get rid of this.
  INLINE void LLVMLiteRaceUpdate(uintptr_t tid_modulo_num,
                                 uint32_t sampling_rate) {
    LiteRaceUpdate(tid_modulo_num, sampling_rate);
  }

  // This is all racey, but ok.
  INLINE bool LiteRaceSkipTrace(uint32_t tid_modulo_num,
                                uint32_t sampling_rate) {
    if (LiteRaceSkipTraceQuickCheck(tid_modulo_num)) return true;
    LiteRaceUpdate(tid_modulo_num, sampling_rate);
    return false;
  }

  INLINE bool LiteRaceSkipTraceRealTid(uint32_t tid, uint32_t sampling_rate) {
    return LiteRaceSkipTrace(tid % kLiteRaceNumTids, sampling_rate);
  }

 private:
  static size_t id_counter_;
  static vector<TraceInfo*> *g_all_traces;

  TraceInfo() : TraceInfoPOD() { }
};

// end. {{{1
#endif  // TS_TRACE_INFO_
// vim:shiftwidth=2:softtabstop=2:expandtab:tw=80
