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
//--------- Head ------------------- {{{1
#ifndef THREAD_SANITIZER_H_
#define THREAD_SANITIZER_H_

#include "ts_util.h"
#include "ts_atomic.h"

//--------- Utils ------------------- {{{1

void Report(const char *format, ...);
void PcToStrings(uintptr_t pc, bool demangle,
                string *img_name, string *rtn_name,
                string *file_name, int *line_no);
string PcToRtnNameAndFilePos(uintptr_t pc);
string PcToRtnName(uintptr_t pc, bool demangle);
string Demangle(const char *str);


//--------- FLAGS ---------------------------------- {{{1
struct FLAGS {
  string           input_type; // for ts_offline.
                               // Possible values: str, bin, decode.
  bool             ignore_stack;
  intptr_t         verbosity;
  intptr_t         show_stats;  // 0 -- no stats; 1 -- some stats; 2 more stats.
  bool             trace_profile;
  bool             show_expected_races;
  uintptr_t        trace_addr;
  uintptr_t        segment_set_recycle_queue_size;
  uintptr_t        recent_segments_cache_size;
  vector<string>   file_prefix_to_cut;
  vector<string>   ignore;
  vector<string>   whitelist;
  bool             ignore_unknown_pcs;  // Ignore PCs with no debug info.
  vector<string>   cut_stack_below;
  string           summary_file;
  string           log_file;
  bool             offline;
  intptr_t         max_n_threads;
  bool             compress_cache_lines;
  bool             unlock_on_mutex_destroy;

  intptr_t         sample_events;
  intptr_t         sample_events_depth;

  intptr_t         num_callers;

  intptr_t    keep_history;
  bool        pure_happens_before;
  bool        free_is_write;
  bool        exit_after_main;
  bool        demangle;
  bool        announce_threads;
  bool        full_output;
  bool        show_states;
  bool        show_proc_self_status;
  bool        show_valgrind_context;  // debug-only
  bool        suggest_happens_before_arcs;
  bool        show_pc;
  bool        full_stack_frames;
  bool        color;  // Colorify terminal output.
  bool        html;  // Output in html format.
  bool        show_pid;

  intptr_t  debug_level;
  bool        save_ignore_context;  // print stack if ignore_end was forgotten.
  vector<string> debug_phase;
  intptr_t  trace_level;

  intptr_t     dry_run;
  intptr_t     max_sid;
  intptr_t     max_sid_before_flush;
  intptr_t     max_mem_in_mb;
  intptr_t     num_callers_in_history;
  intptr_t     flush_period;

  intptr_t     literace_sampling;
  bool         start_with_global_ignore_on;

  intptr_t     locking_scheme;  // Used for internal experiments with locking.

  bool         report_races;
  bool         thread_coverage;
  bool         atomicity;
  bool         call_coverage;
  string       dump_events;  // The name of log file. Debug mode only.
  bool         symbolize;
  bool         attach_mode;

  string       tsan_program_name;
  string       tsan_url;

  vector<string> suppressions;
  bool           generate_suppressions;

  intptr_t     error_exitcode;
  bool         trace_children;

  vector<string> race_verifier;
  vector<string> race_verifier_extra;
  intptr_t       race_verifier_sleep_ms;

  bool nacl_untrusted;

  bool threaded_analysis;

  bool sched_shake;
  bool api_ambush;

  bool enable_atomic;
};

extern FLAGS *G_flags;

extern bool g_race_verifier_active;

extern bool debug_expected_races;
extern bool debug_malloc;
extern bool debug_free;
extern bool debug_thread;
extern bool debug_rtn;
extern bool debug_wrap;
extern bool debug_ins;
extern bool debug_shadow_stack;
extern bool debug_race_verifier;

// -------- CallStack ------------- {{{1
const size_t kMaxCallStackSize = 1 << 12;

struct CallStackPod {
  uintptr_t *end_;
  uintptr_t pcs_[kMaxCallStackSize];
};

struct CallStack: public CallStackPod {

  CallStack() { Clear(); }

  size_t size() { return (size_t)(end_ - pcs_); }
  uintptr_t *pcs() { return pcs_; }

  bool empty() { return end_ == pcs_; }

  uintptr_t &back() {
    DCHECK(!empty());
    return *(end_ - 1);
  }

  void pop_back() {
    DCHECK(!empty());
    end_--;
  }

  void push_back(uintptr_t pc) {
    DCHECK(size() < kMaxCallStackSize);
    *end_ = pc;
    end_++;
  }

  void Clear() {
    end_ = pcs_;
  }

  uintptr_t &operator[] (size_t i) {
    DCHECK(i < size());
    return pcs_[i];
  }

};

//--------- TS Exports ----------------- {{{1
#include "ts_events.h"
#include "ts_trace_info.h"

struct TSanThread;
void ThreadSanitizerInit();
void ThreadSanitizerFini();
// TODO(glider): this is a temporary solution to avoid deadlocks after fork().
#ifdef TS_LLVM
void ThreadSanitizerLockAcquire();
void ThreadSanitizerLockRelease();
#endif
void ThreadSanitizerHandleOneEvent(Event *event);
TSanThread *ThreadSanitizerGetThreadByTid(int32_t tid);
void ThreadSanitizerHandleTrace(int32_t tid, TraceInfo *trace_info,
                                       uintptr_t *tleb);
void ThreadSanitizerHandleTrace(TSanThread *thr, TraceInfo *trace_info,
                                       uintptr_t *tleb);
void ThreadSanitizerHandleOneMemoryAccess(TSanThread *thr, MopInfo mop,
                                                 uintptr_t addr);
void ThreadSanitizerParseFlags(vector<string>* args);
bool ThreadSanitizerWantToInstrumentSblock(uintptr_t pc);
bool ThreadSanitizerWantToCreateSegmentsOnSblockEntry(uintptr_t pc);
bool ThreadSanitizerIgnoreAccessesBelowFunction(uintptr_t pc);

typedef int (*ThreadSanitizerUnwindCallback)(uintptr_t* stack, int size, uintptr_t pc);
void ThreadSanitizerSetUnwindCallback(ThreadSanitizerUnwindCallback cb);

/** Atomic operation handler.
 *  @param tid ID of a thread that issues the operation.
 *  @param pc Program counter that should be associated with the operation.
 *  @param op Type of the operation (load, store, etc).
 *  @param mo Memory ordering associated with the operation
 *      (relaxed, acquire, release, etc). NB there are some restrictions on
 *      what memory orderings can be used with what types of operations.
 *      E.g. a store can't have an acquire semantics
 *      (see C++0x standard draft for details).
 *  @param fail_mo Memory ordering the operation has if it fails,
 *      applicable only to compare_exchange oprations.
 *  @param size Size of the memory access in bytes (1, 2, 4 or 8).
 *  @param a Address of the memory access.
 *  @param v Operand for the operation (e.g. a value to store).
 *  @param cmp Comparand for compare_exchange oprations.
 *  @return Result of the operation (e.g. loaded value).
 */
uint64_t ThreadSanitizerHandleAtomicOp(int32_t tid,
                                       uintptr_t pc,
                                       tsan_atomic_op op,
                                       tsan_memory_order mo,
                                       tsan_memory_order fail_mo,
                                       size_t size,
                                       void volatile* a,
                                       uint64_t v,
                                       uint64_t cmp);

enum IGNORE_BELOW_RTN {
  IGNORE_BELOW_RTN_UNKNOWN,
  IGNORE_BELOW_RTN_NO,
  IGNORE_BELOW_RTN_YES
};

void ThreadSanitizerHandleRtnCall(int32_t tid, uintptr_t call_pc,
                                         uintptr_t target_pc,
                                         IGNORE_BELOW_RTN ignore_below);

void ThreadSanitizerHandleRtnExit(int32_t tid);

void ThreadSanitizerPrintUsage();
extern "C" const char *ThreadSanitizerQuery(const char *query);
bool PhaseDebugIsOn(const char *phase_name);

extern bool g_has_entered_main;
extern bool g_has_exited_main;

// -------- Stats ------------------- {{{1
#include "ts_stats.h"
extern Stats *G_stats;

// -------- Expected Race ---------------------- {{{1
// Information about expected races.
struct ExpectedRace {
  uintptr_t   ptr;
  uintptr_t   size;
  bool        is_verifiable;
  bool        is_nacl_untrusted;
  int         count;
  const char *description;
  uintptr_t   pc;
};

ExpectedRace* ThreadSanitizerFindExpectedRace(uintptr_t addr);

// Tell ThreadSanitizer about the location of NaCl untrusted region.
void ThreadSanitizerNaclUntrustedRegion(uintptr_t mem_start, uintptr_t mem_end);

// Returns true if accesses and locks at the given address should be ignored
// according to the current NaCl flags (--nacl-untrusted). Always false if not a
// NaCl program.
bool ThreadSanitizerIgnoreForNacl(uintptr_t addr);

// end. {{{1
#endif  //  THREAD_SANITIZER_H_

// vim:shiftwidth=2:softtabstop=2:expandtab
