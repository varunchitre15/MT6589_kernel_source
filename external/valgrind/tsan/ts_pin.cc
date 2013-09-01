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

#define __STDC_LIMIT_MACROS
#include "pin.H"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <assert.h>

#include "thread_sanitizer.h"
#include "ts_lock.h"
#include "ts_trace_info.h"
#include "ts_race_verifier.h"
#include "common_util.h"


#if defined(__GNUC__)
# include <cxxabi.h>  // __cxa_demangle
# define ATOMIC_READ(a) __sync_add_and_fetch(a, 0)

#elif defined(_MSC_VER)
namespace WINDOWS
{
// This is the way of including winows.h recommended by PIN docs.
#include<Windows.h>
}

#include <intrin.h>
# define popen(x,y) (NULL)
# define ATOMIC_READ(a)         _InterlockedCompareExchange(a, 0, 0)
# define usleep(x) WINDOWS::Sleep((x)/1000)
# define UINTPTR_MAX ((uintptr_t)-1)
#endif

#ifdef NDEBUG
# error "Please don't define NDEBUG"
#endif

static void DumpEvent(CONTEXT *ctx, EventType type, int32_t tid, uintptr_t pc,
                      uintptr_t a, uintptr_t info);
//------ Global PIN lock ------- {{{1
class ScopedReentrantClientLock {
 public:
  ScopedReentrantClientLock(int line)
    : line_(line) {
    // if (line && G_flags->debug_level >= 5)  Printf("??Try  at line %d\n", line);
    PIN_LockClient();
    if (line && G_flags->debug_level >= 5)  Printf("++Lock at line %d\n", line);
  }
  ~ScopedReentrantClientLock() {
    if (line_ && G_flags->debug_level >= 5) Printf("--Unlock at line %d\n", line_);
    PIN_UnlockClient();
  }
 private:
  int line_;
};

//--------------- Globals ----------------- {{{1
extern FILE *G_out;

// Number of threads created by pthread_create (i.e. not counting main thread).
static int n_created_threads = 0;
// Number of started threads, i.e. the number of CallbackForThreadStart calls.
static int n_started_threads = 0;

const uint32_t kMaxThreads = PIN_MAX_THREADS;

// Serializes the ThreadSanitizer callbacks if TS_SERIALIZED==1
static TSLock g_main_ts_lock;

// Serializes calls to pthread_create and CreateThread.
static TSLock g_thread_create_lock;
// Under g_thread_create_lock.
static THREADID g_tid_of_thread_which_called_create_thread = -1;

#ifdef _MSC_VER
// On Windows, we need to create a h-b arc between
// RtlQueueWorkItem(callback, x, y) and the call to callback.
// Same for RegisterWaitForSingleObject.
static unordered_set<uintptr_t> *g_windows_thread_pool_calback_set;
// Similarly, we need h-b arcs between the returns from callbacks and
// thre related UnregisterWaitEx. Damn, what a stupid interface!
static unordered_map<uintptr_t, uintptr_t> *g_windows_thread_pool_wait_object_map;
#endif

//--------------- StackFrame ----------------- {{{1
struct StackFrame {
  uintptr_t pc;
  uintptr_t sp;
  StackFrame(uintptr_t p, uintptr_t s) : pc(p), sp(s) { }
};
//--------------- InstrumentedCallFrame ----- {{{1
// Machinery to implement the fast interceptors in PIN
// (i.e. the ones that don't use PIN_CallApplicationFunction).
// We instrument the entry of the interesting function (e.g. malloc)
// and all RET instructions in this function's module (e.g. libc).
// At entry, we push an InstrumentedCallFrame object onto InstrumentedCallStack.
// At every RET instruction we check if the stack is not empty (fast path)
// and if the top contains the current SP. If yes -- this is the function return
// and we pop the stack.
struct InstrumentedCallFrame {
  typedef void (*callback_t)(THREADID tid, InstrumentedCallFrame &frame,
                             ADDRINT ret);
  callback_t callback;
  uintptr_t pc;
  uintptr_t sp;
  uintptr_t arg[4];
};

struct InstrumentedCallStack {
 public:
  InstrumentedCallStack() : size_(0) { }

  size_t size() { return size_; }

  void Push(InstrumentedCallFrame::callback_t callback,
            uintptr_t pc,
            uintptr_t sp,
            uintptr_t a0, uintptr_t a1) {
    CHECK(size() < TS_ARRAY_SIZE(frames_));
    size_++;
    Top()->callback = callback;
    Top()->pc = pc;
    Top()->sp = sp;
    Top()->arg[0] = a0;
    Top()->arg[1] = a1;
  }

  void Pop() {
    CHECK(size() > 0);
    size_--;
  }

  InstrumentedCallFrame *Top() {
    CHECK(size() > 0);
    return &frames_[size_-1];
  }

  void Print() {
    for (size_t i = 0; i < size(); i++) {
      Printf( " %p\n", frames_[i].sp);
      if (i > 0) CHECK(frames_[i].sp <= frames_[i-1].sp);
    }
  }

 private:
  InstrumentedCallFrame frames_[20];
  size_t size_;
};

//--------------- PinThread ----------------- {{{1
const size_t kThreadLocalEventBufferSize = 2048 - 2;
// The number of mops should be at least 2 less than the size of TLEB
// so that we have space to put SBLOCK_ENTER token and the trace_info ptr.
const size_t kMaxMopsPerTrace = kThreadLocalEventBufferSize - 2;

REG tls_reg;

struct PinThread;

struct ThreadLocalEventBuffer {
  PinThread *t;
  size_t size;
  uintptr_t events[kThreadLocalEventBufferSize];
};

struct PinThread {
  ThreadLocalEventBuffer tleb;
  int          uniq_tid;
  uint32_t     literace_sampling;  // cache of a flag.
  volatile long last_child_tid;
  InstrumentedCallStack ic_stack;
  THREADID     tid;
  THREADID     parent_tid;
  pthread_t    my_ptid;
  size_t       thread_stack_size_if_known;
  size_t       last_child_stack_size_if_known;
  vector<StackFrame> shadow_stack;
  TraceInfo    *trace_info;
  int ignore_accesses;  // if > 0, ignore all memory accesses.
  int ignore_accesses_depth;
  int ignore_sync;      // if > 0, ignore all sync events.
  int spin_lock_recursion_depth;
  bool         thread_finished;
  bool         thread_done;
  bool         holding_lock;
  int          n_consumed_events;
#ifdef _MSC_VER
  enum StartupState {
    STARTING,
    CHILD_READY,
    MAY_CONTINUE,
  };
  volatile long startup_state;  // used to handle the CREATE_SUSPENDED flag.
#endif
  char         padding[64];  // avoid any chance of ping-pong.
};

// Array of pin threads, indexed by pin's THREADID.
static PinThread *g_pin_threads;

// If true, ignore all accesses in all threads.
extern bool global_ignore;

#ifdef _MSC_VER
static unordered_set<pthread_t> *g_win_handles_which_are_threads;
#endif

//-------------------- ts_replace ------------------- {{{1
static void ReportAccesRange(THREADID tid, uintptr_t pc, EventType type, uintptr_t x, size_t size) {
  if (size && !g_pin_threads[tid].ignore_accesses) {
    uintptr_t end = x + size;
    for(uintptr_t a = x; a < end; a += 8) {
      size_t cur_size = min((uintptr_t)8, end - a);
      DumpEvent(0, type, tid, pc, a, cur_size);
    }
  }
}

#define REPORT_READ_RANGE(x, size) ReportAccesRange(tid, pc, READ, (uintptr_t)x, size)
#define REPORT_WRITE_RANGE(x, size) ReportAccesRange(tid, pc, WRITE, (uintptr_t)x, size)

#define EXTRA_REPLACE_PARAMS THREADID tid, uintptr_t pc,
#define EXTRA_REPLACE_ARGS tid, pc,
#include "ts_replace.h"

//------------- ThreadSanitizer exports ------------ {{{1
string Demangle(const char *str) {
#if defined(__GNUC__)
  int status;
  char *demangled = __cxxabiv1::__cxa_demangle(str, 0, 0, &status);
  if (demangled) {
    string res = demangled;
    free(demangled);
    return res;
  }
#endif
  return str;
}

void PcToStrings(uintptr_t pc, bool demangle,
                string *img_name, string *rtn_name,
                string *file_name, int *line_no) {
  if (G_flags->symbolize) {
    RTN rtn;
    ScopedReentrantClientLock lock(__LINE__);
    // ClientLock must be held.
    PIN_GetSourceLocation(pc, NULL, line_no, file_name);
    *file_name = ConvertToPlatformIndependentPath(*file_name);
    rtn = RTN_FindByAddress(pc);
    string name;
    if (RTN_Valid(rtn)) {
      *rtn_name = demangle
          ? Demangle(RTN_Name(rtn).c_str())
          : RTN_Name(rtn);
      *img_name = IMG_Name(SEC_Img(RTN_Sec(rtn)));
    }
  }
}

string PcToRtnName(uintptr_t pc, bool demangle) {
  string res;
  if (G_flags->symbolize) {
    {
      ScopedReentrantClientLock lock(__LINE__);
      RTN rtn = RTN_FindByAddress(pc);
      if (RTN_Valid(rtn)) {
        res = demangle
            ? Demangle(RTN_Name(rtn).c_str())
            : RTN_Name(rtn);
      }
    }
  }
  return res;
}

//--------------- ThreadLocalEventBuffer ----------------- {{{1
// thread local event buffer is an array of uintptr_t.
// The events are encoded like this:
// { RTN_CALL, call_pc, target_pc }
// { RTN_EXIT }
// { SBLOCK_ENTER, trace_info_of_size_n, addr1, addr2, ... addr_n}

enum TLEBSpecificEvents {
  TLEB_IGNORE_ALL_BEGIN = LAST_EVENT + 1,
  TLEB_IGNORE_ALL_END,
  TLEB_IGNORE_SYNC_BEGIN,
  TLEB_IGNORE_SYNC_END,
  TLEB_GLOBAL_IGNORE_ON,
  TLEB_GLOBAL_IGNORE_OFF,
};

static bool DumpEventPlainText(EventType type, int32_t tid, uintptr_t pc,
                        uintptr_t a, uintptr_t info) {
#if DEBUG == 0 || defined(_MSC_VER)
  return false;
#else
  if (G_flags->dump_events.empty()) return false;

  static unordered_set<uintptr_t> *pc_set;
  if (pc_set == NULL) {
    pc_set = new unordered_set<uintptr_t>;
  }
  static FILE *log_file = NULL;
  if (log_file == NULL) {
    log_file = popen(("gzip > " + G_flags->dump_events).c_str(), "w");
  }
  if (G_flags->symbolize && pc_set->insert(pc).second) {
    string img_name, rtn_name, file_name;
    int line = 0;
    PcToStrings(pc, false, &img_name, &rtn_name, &file_name, &line);
    if (file_name.empty()) file_name = "unknown";
    if (img_name.empty()) img_name = "unknown";
    if (rtn_name.empty()) rtn_name = "unknown";
    if (line == 0) line = 1;
    fprintf(log_file, "#PC %lx %s %s %s %d\n",
            (long)pc, img_name.c_str(), rtn_name.c_str(),
            file_name.c_str(), line);
  }
  fprintf(log_file, "%s %x %lx %lx %lx\n", kEventNames[type], tid,
          (long)pc, (long)a, (long)info);
  return true;
#endif
}

static void DumpEventInternal(EventType type, int32_t uniq_tid, uintptr_t pc,
                              uintptr_t a, uintptr_t info) {
  if (DumpEventPlainText(type, uniq_tid, pc, a, info)) return;
  // PIN wraps the tid (after 2048), but we need a uniq tid.
  Event event(type, uniq_tid, pc, a, info);
  ThreadSanitizerHandleOneEvent(&event);
}

void ComputeIgnoreAccesses(PinThread &t) {
  t.ignore_accesses = (t.ignore_accesses_depth != 0) || (global_ignore != 0);
}

static void HandleInnerEvent(PinThread &t, uintptr_t event) {
  DCHECK(event > LAST_EVENT);
  if (event == TLEB_IGNORE_ALL_BEGIN){
    t.ignore_accesses_depth++;
    ComputeIgnoreAccesses(t);
  } else if (event == TLEB_IGNORE_ALL_END){
    t.ignore_accesses_depth--;
    CHECK(t.ignore_accesses_depth >= 0);
    ComputeIgnoreAccesses(t);
  } else if (event == TLEB_IGNORE_SYNC_BEGIN){
    t.ignore_sync++;
  } else if (event == TLEB_IGNORE_SYNC_END){
    t.ignore_sync--;
    CHECK(t.ignore_sync >= 0);
  } else if (event == TLEB_GLOBAL_IGNORE_ON){
    Report("INFO: GLOBAL IGNORE ON\n");
    global_ignore = true;
    ComputeIgnoreAccesses(t);
  } else if (event == TLEB_GLOBAL_IGNORE_OFF){
    Report("INFO: GLOBAL IGNORE OFF\n");
    global_ignore = false;
    ComputeIgnoreAccesses(t);
  } else {
    Printf("Event: %ld (last: %ld)\n", event, LAST_EVENT);
    CHECK(0);
  }
}

static INLINE bool WantToIgnoreEvent(PinThread &t, uintptr_t event) {
  if (t.ignore_sync &&
      (event == WRITER_LOCK || event == READER_LOCK || event == UNLOCK ||
       event == SIGNAL || event == WAIT)) {
    // do nothing, we are ignoring locks.
    return true;
  } else if (t.ignore_accesses && (event == READ || event == WRITE)) {
    // do nothing, we are ignoring mops.
    return true;
  }
  return false;
}

static INLINE void TLEBFlushUnlocked(ThreadLocalEventBuffer &tleb) {
  if (tleb.size == 0) return;
  PinThread &t = *tleb.t;
  // global_ignore should be always on with race verifier
  DCHECK(!g_race_verifier_active || global_ignore);
  DCHECK(tleb.size <= kThreadLocalEventBufferSize);
  if (DEBUG_MODE && t.thread_done) {
    Printf("ACHTUNG!!! an event from a dead thread T%d\n", t.tid);
  }
  DCHECK(!t.thread_done);

  if (TS_SERIALIZED == 1 || DEBUG_MODE) {
    size_t max_idx = TS_ARRAY_SIZE(G_stats->tleb_flush);
    size_t idx = min((size_t)u32_log2(tleb.size), max_idx - 1);
    CHECK(idx < max_idx);
    G_stats->tleb_flush[idx]++;
  }

  if (TS_SERIALIZED == 1 && G_flags->offline) {
    fwrite(tleb.events, sizeof(uintptr_t), tleb.size, G_out);
    tleb.size = 0;
    return;
  }

  size_t i;
  for (i = 0; i < tleb.size; ) {
    uintptr_t event = tleb.events[i++];
    DCHECK(!g_race_verifier_active ||
        event == SBLOCK_ENTER || event == EXPECT_RACE || event == THR_START);
    if (event == RTN_EXIT) {
      if (DumpEventPlainText(RTN_EXIT, t.uniq_tid, 0, 0, 0)) continue;
      ThreadSanitizerHandleRtnExit(t.uniq_tid);
    } else if (event == RTN_CALL) {
      uintptr_t call_pc = tleb.events[i++];
      uintptr_t target_pc = tleb.events[i++];
      IGNORE_BELOW_RTN ignore_below = (IGNORE_BELOW_RTN)tleb.events[i++];
      if (DumpEventPlainText(RTN_CALL, t.uniq_tid, call_pc,
                             target_pc, ignore_below)) continue;
      ThreadSanitizerHandleRtnCall(t.uniq_tid, call_pc, target_pc,
                                   ignore_below);
    } else if (event == SBLOCK_ENTER){
      TraceInfo *trace_info = (TraceInfo*) tleb.events[i++];
      DCHECK(trace_info);
      bool do_this_trace = true;
      if (t.ignore_accesses) {
        do_this_trace = false;
      } else if (t.literace_sampling) {
        do_this_trace = !trace_info->LiteRaceSkipTraceRealTid(
            t.uniq_tid, t.literace_sampling);
      }

      size_t n = trace_info->n_mops();
      if (do_this_trace) {
        if (DEBUG_MODE && !G_flags->dump_events.empty()) {
          DumpEventPlainText(SBLOCK_ENTER, t.uniq_tid, trace_info->pc(), 0, 0);
          for (size_t j = 0; j < n; j++) {
            MopInfo *mop = trace_info->GetMop(j);
            DCHECK(mop->size());
            DCHECK(mop);
            uintptr_t addr = tleb.events[i + j];
            if (addr) {
              DumpEventPlainText(mop->is_write() ? WRITE : READ, t.uniq_tid,
                                     mop->pc(), addr, mop->size());
            }
          }
        } else {
          ThreadSanitizerHandleTrace(t.uniq_tid, trace_info, tleb.events+i);
        }
      }
      i += n;
    } else if (event == THR_START) {
      uintptr_t parent = -1;
      if (t.parent_tid != (THREADID)-1) {
        parent = g_pin_threads[t.parent_tid].uniq_tid;
      }
      DumpEventInternal(THR_START, t.uniq_tid, 0, 0, parent);
    } else if (event == THR_END) {
      DumpEventInternal(THR_END, t.uniq_tid, 0, 0, 0);
      DCHECK(t.thread_finished == true);
      DCHECK(t.thread_done == false);
      t.thread_done = true;
      i += 3;  // consume the unneeded data.
      DCHECK(i == tleb.size);  // should be last event in this tleb.
    } else if (event > LAST_EVENT) {
      HandleInnerEvent(t, event);
    } else {
      // all other events.
      CHECK(event > NOOP && event < LAST_EVENT);
      uintptr_t pc    = tleb.events[i++];
      uintptr_t a     = tleb.events[i++];
      uintptr_t info  = tleb.events[i++];
      if (!WantToIgnoreEvent(t, event)) {
        DumpEventInternal((EventType)event, t.uniq_tid, pc, a, info);
      }
    }
  }
  DCHECK(i == tleb.size);
  tleb.size = 0;
  if (DEBUG_MODE) { // for sanity checking.
    memset(tleb.events, 0xf0, sizeof(tleb.events));
  }
}

static INLINE void TLEBFlushLocked(PinThread &t) {
#if TS_SERIALIZED==1
  if (G_flags->dry_run) {
    t.tleb.size = 0;
    return;
  }
  CHECK(t.tleb.size <= kThreadLocalEventBufferSize);
  G_stats->lock_sites[0]++;
  ScopedLock lock(&g_main_ts_lock);
  TLEBFlushUnlocked(t.tleb);
#else
  TLEBFlushUnlocked(t.tleb);
#endif
}

static void TLEBAddRtnCall(PinThread &t, uintptr_t call_pc,
                           uintptr_t target_pc, IGNORE_BELOW_RTN ignore_below) {
  if (TS_SERIALIZED == 0) {
    TLEBFlushLocked(t);
    ThreadSanitizerHandleRtnCall(t.uniq_tid, call_pc, target_pc,
                                 ignore_below);
    return;
  }
  DCHECK(t.tleb.size <= kThreadLocalEventBufferSize);
  if (t.tleb.size + 4 > kThreadLocalEventBufferSize) {
    TLEBFlushLocked(t);
    DCHECK(t.tleb.size == 0);
  }
  t.tleb.events[t.tleb.size++] = RTN_CALL;
  t.tleb.events[t.tleb.size++] = call_pc;
  t.tleb.events[t.tleb.size++] = target_pc;
  t.tleb.events[t.tleb.size++] = ignore_below;
  DCHECK(t.tleb.size <= kThreadLocalEventBufferSize);
}

static void TLEBAddRtnExit(PinThread &t) {
  if (TS_SERIALIZED == 0) {
    TLEBFlushLocked(t);
    ThreadSanitizerHandleRtnExit(t.uniq_tid);
    return;
  }
  if (t.tleb.size + 1 > kThreadLocalEventBufferSize) {
    TLEBFlushLocked(t);
  }
  t.tleb.events[t.tleb.size++] = RTN_EXIT;
  DCHECK(t.tleb.size <= kThreadLocalEventBufferSize);
}

static INLINE uintptr_t *TLEBAddTrace(PinThread &t) {
  size_t n = t.trace_info->n_mops();
  DCHECK(n > 0);
  if (TS_SERIALIZED == 0) {
    TLEBFlushLocked(t);
  } else if (t.tleb.size + 2 + n > kThreadLocalEventBufferSize) {
    TLEBFlushLocked(t);
  }
  if (TS_SERIALIZED == 1) {
    t.tleb.events[t.tleb.size++] = SBLOCK_ENTER;
    t.tleb.events[t.tleb.size++] = (uintptr_t)t.trace_info;
  } else {
    DCHECK(t.tleb.size == 0);
    t.tleb.events[0] = SBLOCK_ENTER;
    t.tleb.events[1] = (uintptr_t)t.trace_info;
    t.tleb.size += 2;
  }
  uintptr_t *mop_addresses = &t.tleb.events[t.tleb.size];
  // not every address will be written to. so they will stay 0.
  for (size_t i = 0; i < n; i++) {
    mop_addresses[i] = 0;
  }
  t.tleb.size += n;
  DCHECK(t.tleb.size <= kThreadLocalEventBufferSize);
  return mop_addresses;
}

static void TLEBStartThread(PinThread &t) {
  CHECK(t.tleb.size == 0);
  t.tleb.events[t.tleb.size++] = THR_START;
}

static void TLEBSimpleEvent(PinThread &t, uintptr_t event) {
  if (g_race_verifier_active)
    return;
  if (TS_SERIALIZED == 0) {
    TLEBFlushLocked(t);
    if (event < LAST_EVENT) {
      Event e((EventType)event, t.uniq_tid, 0, 0, 0);
      ThreadSanitizerHandleOneEvent(&e);
    } else {
      HandleInnerEvent(t, event);
    }
    return;
  }
  if (t.tleb.size + 1 > kThreadLocalEventBufferSize) {
    TLEBFlushLocked(t);
  }
  t.tleb.events[t.tleb.size++] = event;
  DCHECK(t.tleb.size <= kThreadLocalEventBufferSize);
}

static void TLEBAddGenericEventAndFlush(PinThread &t,
                                        EventType type, uintptr_t pc,
                                        uintptr_t a, uintptr_t info) {
  if (TS_SERIALIZED == 0) {
    if (WantToIgnoreEvent(t, type)) return;
    TLEBFlushLocked(t);
    Event e(type, t.uniq_tid, pc, a, info);
    ThreadSanitizerHandleOneEvent(&e);
    return;
  }
  if (t.tleb.size + 4 > kThreadLocalEventBufferSize) {
    TLEBFlushLocked(t);
  }
  DCHECK(type > NOOP && type < LAST_EVENT);
  t.tleb.events[t.tleb.size++] = type;
  t.tleb.events[t.tleb.size++] = pc;
  t.tleb.events[t.tleb.size++] = a;
  t.tleb.events[t.tleb.size++] = info;
  TLEBFlushLocked(t);
  DCHECK(t.tleb.size <= kThreadLocalEventBufferSize);
}

static void UpdateCallStack(PinThread &t, ADDRINT sp);

// Must be called from its thread (except for THR_END case)!
static void DumpEventWithSp(uintptr_t sp, EventType type, int32_t tid, uintptr_t pc,
                            uintptr_t a, uintptr_t info) {
  if (!g_race_verifier_active || type == EXPECT_RACE) {
    PinThread &t = g_pin_threads[tid];
    if (sp) {
      UpdateCallStack(t, sp);
    }
    TLEBAddGenericEventAndFlush(t, type, pc, a, info);
  }
}
static void DumpEvent(CONTEXT *ctx, EventType type, int32_t tid, uintptr_t pc,
                      uintptr_t a, uintptr_t info) {
  DumpEventWithSp(ctx ? PIN_GetContextReg(ctx, REG_STACK_PTR) : 0,
            type, tid, pc, a, info);
}

//--------- Wraping and relacing --------------- {{{1
static set<string> g_wrapped_functions;
static void InformAboutFunctionWrap(RTN rtn, string name) {
  g_wrapped_functions.insert(name);
  if (!debug_wrap) return;
  Printf("Function wrapped: %s (%s %s)\n", name.c_str(),
         RTN_Name(rtn).c_str(), IMG_Name(SEC_Img(RTN_Sec(rtn))).c_str());
}

static bool RtnMatchesName(const string &rtn_name, const string &name) {
  CHECK(name.size() > 0);
  size_t pos = rtn_name.find(name);
  if (pos == string::npos) {
    return false;
  }
  if (pos == 0 && name.size() == rtn_name.size()) {
  //  Printf("Full match: %s %s\n", rtn_name.c_str(), name.c_str());
    return true;
  }
  // match MyFuncName@123
  if (pos == 0 && name.size() < rtn_name.size()
      && rtn_name[name.size()] == '@') {
  //  Printf("Versioned match: %s %s\n", rtn_name.c_str(), name.c_str());
    return true;
  }
  // match _MyFuncName@123
  if (pos == 1 && rtn_name[0] == '_' && name.size() < rtn_name.size()
      && rtn_name[name.size() + 1] == '@') {
    // Printf("Versioned match: %s %s\n", rtn_name.c_str(), name.c_str());
    return true;
  }

  return false;
}

#define FAST_WRAP_PARAM0 THREADID tid, ADDRINT pc, ADDRINT sp
#define FAST_WRAP_PARAM1 FAST_WRAP_PARAM0, ADDRINT arg0
#define FAST_WRAP_PARAM2 FAST_WRAP_PARAM1, ADDRINT arg1
#define FAST_WRAP_PARAM3 FAST_WRAP_PARAM2, ADDRINT arg2

#define FAST_WRAP_PARAM_AFTER \
  THREADID tid, InstrumentedCallFrame &frame, ADDRINT ret


#define DEBUG_FAST_INTERCEPTORS 0
//#define DEBUG_FAST_INTERCEPTORS (tid == 1)

#define PUSH_AFTER_CALLBACK1(callback, a0) \
  g_pin_threads[tid].ic_stack.Push(callback, pc, sp, a0, 0); \
  if (DEBUG_FAST_INTERCEPTORS) \
    Printf("T%d %s pc=%p sp=%p *sp=(%p) arg0=%p stack_size=%ld\n",\
         tid, __FUNCTION__, pc, sp,\
         ((void**)sp)[0],\
         arg0,\
         g_pin_threads[tid].ic_stack.size()\
         );\


#define WRAP_NAME(name) Wrap_##name
#define WRAP4(name) WrapFunc4(img, rtn, #name, (AFUNPTR)Wrap_##name)
#define WRAPSTD1(name) WrapStdCallFunc1(rtn, #name, (AFUNPTR)Wrap_##name)
#define WRAPSTD2(name) WrapStdCallFunc2(rtn, #name, (AFUNPTR)Wrap_##name)
#define WRAPSTD3(name) WrapStdCallFunc3(rtn, #name, (AFUNPTR)Wrap_##name)
#define WRAPSTD4(name) WrapStdCallFunc4(rtn, #name, (AFUNPTR)Wrap_##name)
#define WRAPSTD5(name) WrapStdCallFunc5(rtn, #name, (AFUNPTR)Wrap_##name)
#define WRAPSTD6(name) WrapStdCallFunc6(rtn, #name, (AFUNPTR)Wrap_##name)
#define WRAPSTD7(name) WrapStdCallFunc7(rtn, #name, (AFUNPTR)Wrap_##name)
#define WRAPSTD8(name) WrapStdCallFunc8(rtn, #name, (AFUNPTR)Wrap_##name)
#define WRAPSTD10(name) WrapStdCallFunc10(rtn, #name, (AFUNPTR)Wrap_##name)
#define WRAPSTD11(name) WrapStdCallFunc11(rtn, #name, (AFUNPTR)Wrap_##name)
#define WRAP_PARAM4  THREADID tid, ADDRINT pc, CONTEXT *ctx, \
                                AFUNPTR f,\
                                uintptr_t arg0, uintptr_t arg1, \
                                uintptr_t arg2, uintptr_t arg3

#define WRAP_PARAM6 WRAP_PARAM4, uintptr_t arg4, uintptr_t arg5
#define WRAP_PARAM8 WRAP_PARAM6, uintptr_t arg6, uintptr_t arg7
#define WRAP_PARAM10 WRAP_PARAM8, uintptr_t arg8, uintptr_t arg9
#define WRAP_PARAM12 WRAP_PARAM10, uintptr_t arg10, uintptr_t arg11

static uintptr_t CallFun4(CONTEXT *ctx, THREADID tid,
                         AFUNPTR f, uintptr_t arg0, uintptr_t arg1,
                         uintptr_t arg2, uintptr_t arg3) {
  uintptr_t ret = 0xdeadbee1;
  PIN_CallApplicationFunction(ctx, tid,
                              CALLINGSTD_DEFAULT, (AFUNPTR)(f),
                              PIN_PARG(uintptr_t), &ret,
                              PIN_PARG(uintptr_t), arg0,
                              PIN_PARG(uintptr_t), arg1,
                              PIN_PARG(uintptr_t), arg2,
                              PIN_PARG(uintptr_t), arg3,
                              PIN_PARG_END());
  return ret;
}

static uintptr_t CallFun6(CONTEXT *ctx, THREADID tid,
                         AFUNPTR f, uintptr_t arg0, uintptr_t arg1,
                         uintptr_t arg2, uintptr_t arg3,
                         uintptr_t arg4, uintptr_t arg5) {
  uintptr_t ret = 0xdeadbee1;
  PIN_CallApplicationFunction(ctx, tid,
                              CALLINGSTD_DEFAULT, (AFUNPTR)(f),
                              PIN_PARG(uintptr_t), &ret,
                              PIN_PARG(uintptr_t), arg0,
                              PIN_PARG(uintptr_t), arg1,
                              PIN_PARG(uintptr_t), arg2,
                              PIN_PARG(uintptr_t), arg3,
                              PIN_PARG(uintptr_t), arg4,
                              PIN_PARG(uintptr_t), arg5,
                              PIN_PARG_END());
  return ret;
}

#define CALL_ME_INSIDE_WRAPPER_4() CallFun4(ctx, tid, f, arg0, arg1, arg2, arg3)
#define CALL_ME_INSIDE_WRAPPER_6() CallFun6(ctx, tid, f, arg0, arg1, arg2, arg3, arg4, arg5)

// Completely replace (i.e. not wrap) a function with 3 (or less) parameters.
// The original function will not be called.
void ReplaceFunc3(IMG img, RTN rtn, const char *name, AFUNPTR replacement_func) {
  if (RTN_Valid(rtn) && RtnMatchesName(RTN_Name(rtn), name)) {
    InformAboutFunctionWrap(rtn, name);
    PROTO proto = PROTO_Allocate(PIN_PARG(uintptr_t),
                                 CALLINGSTD_DEFAULT,
                                 "proto",
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG_END());
    RTN_ReplaceSignature(rtn,
                         AFUNPTR(replacement_func),
                         IARG_PROTOTYPE, proto,
                         IARG_THREAD_ID,
                         IARG_INST_PTR,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
                         IARG_END);
    PROTO_Free(proto);
  }
}

// Wrap a function with up to 4 parameters.
void WrapFunc4(IMG img, RTN rtn, const char *name, AFUNPTR replacement_func) {
  if (RTN_Valid(rtn) && RtnMatchesName(RTN_Name(rtn), name)) {
    InformAboutFunctionWrap(rtn, name);
    PROTO proto = PROTO_Allocate(PIN_PARG(uintptr_t),
                                 CALLINGSTD_DEFAULT,
                                 "proto",
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG_END());
    RTN_ReplaceSignature(rtn,
                         AFUNPTR(replacement_func),
                         IARG_PROTOTYPE, proto,
                         IARG_THREAD_ID,
                         IARG_INST_PTR,
                         IARG_CONTEXT,
                         IARG_ORIG_FUNCPTR,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 3,
                         IARG_END);
    PROTO_Free(proto);
  }
}

// Wrap a function with up to 6 parameters.
void WrapFunc6(IMG img, RTN rtn, const char *name, AFUNPTR replacement_func) {
  if (RTN_Valid(rtn) && RtnMatchesName(RTN_Name(rtn), name)) {
    InformAboutFunctionWrap(rtn, name);
    PROTO proto = PROTO_Allocate(PIN_PARG(uintptr_t),
                                 CALLINGSTD_DEFAULT,
                                 "proto",
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG_END());
    RTN_ReplaceSignature(rtn,
                         AFUNPTR(replacement_func),
                         IARG_PROTOTYPE, proto,
                         IARG_THREAD_ID,
                         IARG_INST_PTR,
                         IARG_CONTEXT,
                         IARG_ORIG_FUNCPTR,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 3,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 4,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 5,
                         IARG_END);
    PROTO_Free(proto);
  }
}


//--------- Instrumentation callbacks --------------- {{{1
//---------- Debug -----------------------------------{{{2
#define DEB_PR (0)

static void ShowPcAndSp(const char *where, THREADID tid,
                        ADDRINT pc, ADDRINT sp) {
    Printf("%s T%d sp=%ld pc=%p %s\n", where, tid, sp, pc,
           PcToRtnName(pc, true).c_str());
}

static void PrintShadowStack(PinThread &t) {
  Printf("T%d Shadow stack (%d)\n", t.tid, (int)t.shadow_stack.size());
  for (int i = t.shadow_stack.size() - 1; i >= 0; i--) {
    uintptr_t pc = t.shadow_stack[i].pc;
    uintptr_t sp = t.shadow_stack[i].sp;
    Printf("  sp=%ld pc=%lx %s\n", sp, pc, PcToRtnName(pc, true).c_str());
  }
}

static void DebugOnlyShowPcAndSp(const char *where, THREADID tid,
                                 ADDRINT pc, ADDRINT sp) {
  if (DEB_PR) {
    ShowPcAndSp(where, tid, pc, sp);
  }
}

static uintptr_t WRAP_NAME(ThreadSanitizerQuery)(WRAP_PARAM4) {
  const char *query = (const char*)arg0;
  return (uintptr_t)ThreadSanitizerQuery(query);
}

//--------- Ignores -------------------------------- {{{2
static void IgnoreMopsBegin(THREADID tid) {
  // if (tid != 0) Printf("T%d IgnoreMops++\n", tid);
  TLEBSimpleEvent(g_pin_threads[tid], TLEB_IGNORE_ALL_BEGIN);
}
static void IgnoreMopsEnd(THREADID tid) {
  // if (tid != 0) Printf("T%d IgnoreMops--\n", tid);
  TLEBSimpleEvent(g_pin_threads[tid], TLEB_IGNORE_ALL_END);
}

static void IgnoreSyncAndMopsBegin(THREADID tid) {
  // if (tid != 0) Printf("T%d IgnoreSync++\n", tid);
  IgnoreMopsBegin(tid);
  TLEBSimpleEvent(g_pin_threads[tid], TLEB_IGNORE_SYNC_BEGIN);
}
static void IgnoreSyncAndMopsEnd(THREADID tid) {
  // if (tid != 0) Printf("T%d IgnoreSync--\n", tid);
  IgnoreMopsEnd(tid);
  TLEBSimpleEvent(g_pin_threads[tid], TLEB_IGNORE_SYNC_END);
}

//--------- __cxa_guard_* -------------------------- {{{2
// From gcc/cp/decl.c:
// --------------------------------------------------------------
//      Emit code to perform this initialization but once.  This code
//      looks like:
//
//      static <type> guard;
//      if (!guard.first_byte) {
//        if (__cxa_guard_acquire (&guard)) {
//          bool flag = false;
//          try {
//            // Do initialization.
//            flag = true; __cxa_guard_release (&guard);
//            // Register variable for destruction at end of program.
//           } catch {
//          if (!flag) __cxa_guard_abort (&guard);
//         }
//      }
// --------------------------------------------------------------
// So, when __cxa_guard_acquire returns true, we start ignoring all accesses
// and in __cxa_guard_release we stop ignoring them.
// We also need to ignore all accesses inside these two functions.

static void Before_cxa_guard_acquire(THREADID tid, ADDRINT pc, ADDRINT guard) {
  IgnoreMopsBegin(tid);
}

static void After_cxa_guard_acquire(THREADID tid, ADDRINT pc, ADDRINT ret) {
  if (ret) {
    // Continue ignoring, it will end in __cxa_guard_release.
  } else {
    // Stop ignoring, there will be no matching call to __cxa_guard_release.
    IgnoreMopsEnd(tid);
  }
}

static void After_cxa_guard_release(THREADID tid, ADDRINT pc) {
  IgnoreMopsEnd(tid);
}

static uintptr_t WRAP_NAME(pthread_once)(WRAP_PARAM4) {
  uintptr_t ret;
  IgnoreMopsBegin(tid);
  ret = CALL_ME_INSIDE_WRAPPER_4();
  IgnoreMopsEnd(tid);
  return ret;
}

void TmpCallback1(THREADID tid, ADDRINT pc) {
  Printf("%s T%d %lx\n", __FUNCTION__, tid, pc);
}
void TmpCallback2(THREADID tid, ADDRINT pc) {
  Printf("%s T%d %lx\n", __FUNCTION__, tid, pc);
}

//--------- Threads --------------------------------- {{{2
static void HandleThreadCreateBefore(THREADID tid, ADDRINT pc) {
  DumpEvent(0, THR_CREATE_BEFORE, tid, pc, 0, 0);
  g_thread_create_lock.Lock();
  IgnoreMopsBegin(tid);
  CHECK(g_tid_of_thread_which_called_create_thread == (THREADID)-1);
  g_tid_of_thread_which_called_create_thread = tid;
  n_created_threads++;
}

static void HandleThreadCreateAbort(THREADID tid) {
  CHECK(g_tid_of_thread_which_called_create_thread == tid);
  g_tid_of_thread_which_called_create_thread = (THREADID)-1;
  n_created_threads--;
  IgnoreMopsEnd(tid);
  g_thread_create_lock.Unlock();
}

static THREADID HandleThreadCreateAfter(THREADID tid, pthread_t child_ptid,
                                        bool suspend_child) {
  // Spin, waiting for last_child_tid to appear (i.e. wait for the thread to
  // actually start) so that we know the child's tid. No locks.
  while (!ATOMIC_READ(&g_pin_threads[tid].last_child_tid)) {
    YIELD();
  }

  CHECK(g_tid_of_thread_which_called_create_thread == tid);
  g_tid_of_thread_which_called_create_thread = -1;

  THREADID last_child_tid = g_pin_threads[tid].last_child_tid;
  CHECK(last_child_tid);

  PinThread &child_t = g_pin_threads[last_child_tid];
  child_t.my_ptid = child_ptid;

#ifdef _MSC_VER
  if (suspend_child) {
    while (ATOMIC_READ(&child_t.startup_state) != PinThread::CHILD_READY) {
      YIELD();
    }
    // Strictly speaking, PIN forbids calling system functions like this.
    // This may violate application library isolation but
    // a) YIELD == WINDOWS::Sleep, so we violate it anyways
    // b) SuspendThread probably calls NtSuspendThread right away
    WINDOWS::DWORD old_count = WINDOWS::SuspendThread((WINDOWS::HANDLE)child_ptid);  // TODO handle?
    CHECK(old_count == 0);
  }
  child_t.startup_state = PinThread::MAY_CONTINUE;
#else
  CHECK(!suspend_child);  // Not implemented - do we need to?
#endif

  int uniq_tid_of_child = child_t.uniq_tid;
  g_pin_threads[tid].last_child_tid = 0;

  IgnoreMopsEnd(tid);
  g_thread_create_lock.Unlock();

  DumpEvent(0, THR_CREATE_AFTER, tid, 0, 0, uniq_tid_of_child);
  return last_child_tid;
}

static uintptr_t WRAP_NAME(pthread_create)(WRAP_PARAM4) {
  HandleThreadCreateBefore(tid, pc);

  uintptr_t ret = CALL_ME_INSIDE_WRAPPER_4();
  if (ret != 0) {
    HandleThreadCreateAbort(tid);
    return ret;
  }

  pthread_t child_ptid = *(pthread_t*)arg0;
  HandleThreadCreateAfter(tid, child_ptid, false);

  return ret;
}

void CallbackForThreadStart(THREADID tid, CONTEXT *ctxt,
                            INT32 flags, void *v) {
  // We can not rely on PIN_GetParentTid() since it is broken on Windows.

  if (g_pin_threads == NULL) {
    g_pin_threads = new PinThread[kMaxThreads];
  }

  bool has_parent = true;
  if (tid == 0) {
    // Main thread or we have attached to a running process.
    has_parent = false;
  } else {
    CHECK(tid > 0);
  }

  CHECK(tid < kMaxThreads);
  PinThread &t = g_pin_threads[tid];
  memset(&t, 0, sizeof(PinThread));
  t.uniq_tid = n_started_threads++;
  t.literace_sampling = G_flags->literace_sampling;
  t.tid = tid;
  t.tleb.t = &t;
#if defined(_MSC_VER)
  t.startup_state = PinThread::STARTING;
#endif
  ComputeIgnoreAccesses(t);


  PIN_SetContextReg(ctxt, tls_reg, (ADDRINT)&t.tleb.events[2]);

  t.parent_tid = -1;
  if (has_parent) {
    t.parent_tid = g_tid_of_thread_which_called_create_thread;
#if !defined(_MSC_VER)  // On Windows, threads may appear out of thin air.
    CHECK(t.parent_tid != (THREADID)-1);
#endif  // _MSC_VER
  }

  if (debug_thread) {
    Printf("T%d ThreadStart parent=%d child=%d\n", tid, t.parent_tid, tid);
  }

  if (has_parent && t.parent_tid != (THREADID)-1) {
    g_pin_threads[t.parent_tid].last_child_tid = tid;
    t.thread_stack_size_if_known =
        g_pin_threads[t.parent_tid].last_child_stack_size_if_known;
  } else {
#if defined(_MSC_VER)
    t.startup_state = PinThread::MAY_CONTINUE;
#endif
  }

  // This is a lock-free (thread local) operation.
  TLEBStartThread(t);
/* TODO(timurrrr): investigate and un-comment
#ifdef _MSC_VER
  // Ignore all mops & sync before the real thread code.
  // See the corresponding IgnoreSyncAndMopsEnd in Before_BaseThreadInitThunk.
  IgnoreSyncAndMopsBegin(tid);
#endif
*/
}

static void Before_start_thread(THREADID tid, ADDRINT pc, ADDRINT sp) {
  PinThread &t = g_pin_threads[tid];
  if (debug_thread) {
    Printf("T%d Before_start_thread: sp=%p my_ptid=%p diff=%p\n",
         tid, sp, t.my_ptid, t.my_ptid - sp);
  }
  // This is a rather scary hack, but I see no easy way to avoid it.
  // On linux NPTL, the pthread_t structure is the same block of memory
  // as the stack (and the tls?). Somewhere inside the pthread_t
  // object lives the address of stackblock followed by its size
  // (see nptl/descr.h).
  // At the current point we may not know the value of pthread_t (my_ptid),
  // but we do know the current sp, which is a bit less than my_ptid.
  //
  // address                        value
  // ------------------------------------------------
  // 0xffffffffffffffff:
  //
  // stackblock + stackblock_size:
  // my_ptid:
  //
  //                                stackblock_size
  //                                stackblock
  //
  // current_sp:
  //
  //
  // stackblock:
  //
  // 0x0000000000000000:
  // -------------------------------------------------
  //
  // So, we itrate from sp to the higher addresses (but just in case, not more
  // than a few pages) trying to find a pair of values which looks like
  // stackblock and stackblock_size. Oh well.
  // Note that in valgrind we are able to get this info from
  //  pthread_getattr_np (linux) or pthread_get_stackaddr_np (mac),
  // but in PIN we can't call those (can we?).
  uintptr_t prev = 0;
  for (uintptr_t sp1 = sp; sp1 - sp < 0x2000;
       sp1 += sizeof(uintptr_t)) {
    uintptr_t val = *(uintptr_t*)sp1;
    if (val == 0) continue;
    if (prev &&
        (prev & 0xfff) == 0 && // stack is page aligned
        prev < sp &&           // min stack is < sp
        prev + val > sp &&     // max stack is > sp
        val >= (1 << 15) &&    // stack size is >= 32k
        val <= 128 * (1 << 20) // stack size is hardly > 128M
        ) {
      if (debug_thread) {
        Printf("T%d found stack: %p size=%p\n", tid, prev, val);
      }
      DumpEvent(0, THR_STACK_TOP, tid, pc, prev + val, val);
      return;
    }
    prev = val;
  }
  // The hack above does not always works. (TODO(kcc)). Do something.
  Printf("WARNING: ThreadSanitizerPin is guessing stack size for T%d\n", tid);
  DumpEvent(0, THR_STACK_TOP, tid, pc, sp, t.thread_stack_size_if_known);
}

#ifdef _MSC_VER
static uintptr_t WRAP_NAME(CreateThread)(WRAP_PARAM6) {
  PinThread &t = g_pin_threads[tid];
  t.last_child_stack_size_if_known = arg1 ? arg1 : 1024 * 1024;

  HandleThreadCreateBefore(tid, pc);

  // We can't start the thread suspended because we want to get its
  // PIN thread ID before leaving CreateThread.
  // So, we reset the CREATE_SUSPENDED flag and SuspendThread before any client
  // code is executed in the HandleThreadCreateAfter if needed.
  bool should_be_suspended = arg4 & CREATE_SUSPENDED;
  arg4 &= ~CREATE_SUSPENDED;

  uintptr_t ret = CALL_ME_INSIDE_WRAPPER_6();
  if (ret == NULL) {
    HandleThreadCreateAbort(tid);
    return ret;
  }
  pthread_t child_ptid = ret;
  THREADID child_tid = HandleThreadCreateAfter(tid, child_ptid,
                                               should_be_suspended);
  {
    ScopedReentrantClientLock lock(__LINE__);
    if (g_win_handles_which_are_threads == NULL) {
      g_win_handles_which_are_threads = new unordered_set<pthread_t>;
    }
    g_win_handles_which_are_threads->insert(child_ptid);
  }
  return ret;
}

static void Before_BaseThreadInitThunk(THREADID tid, ADDRINT pc, ADDRINT sp) {
  PinThread &t = g_pin_threads[tid];
  size_t stack_size = t.thread_stack_size_if_known;
  // Printf("T%d %s %p %p\n", tid, __FUNCTION__, sp, stack_size);
  /* TODO(timurrrr): investigate and uncomment
  if (tid != 0) {
    // Ignore all mops & sync before the real thread code.
    // See the corresponding IgnoreSyncAndMopsBegin in CallbackForThreadStart.
    IgnoreSyncAndMopsEnd(tid);
    TLEBFlushLocked(t);
    CHECK(t.ignore_sync == 0);
    CHECK(t.ignore_accesses == 0);
  }
  */
  DumpEvent(0, THR_STACK_TOP, tid, pc, sp, stack_size);

#ifdef _MSC_VER
  if (t.startup_state != PinThread::MAY_CONTINUE) {
    CHECK(t.startup_state == PinThread::STARTING);
    t.startup_state = PinThread::CHILD_READY;
    while (ATOMIC_READ(&t.startup_state) != PinThread::MAY_CONTINUE) {
      YIELD();
    }
    // Corresponds to SIGNAL from ResumeThread if the thread was suspended on
    // start.
    DumpEvent(0, WAIT, tid, pc, t.my_ptid, 0);
  }
#endif
}

static void Before_RtlExitUserThread(THREADID tid, ADDRINT pc) {
  PinThread &t = g_pin_threads[tid];
  if (t.tid != 0) {
    // Once we started exiting the thread, ignore the locking events.
    // This way we will avoid h-b arcs between unrelated threads.
    // We also start ignoring all mops, otherwise we will get tons of race
    // reports from the windows guts.
    IgnoreSyncAndMopsBegin(tid);
  }
}
#endif  // _MSC_VER

void CallbackForThreadFini(THREADID tid, const CONTEXT *ctxt,
                          INT32 code, void *v) {
  PinThread &t = g_pin_threads[tid];
  t.thread_finished = true;
  // We can not DumpEvent here,
  // due to possible deadlock with PIN's internal lock.
  if (debug_thread) {
    Printf("T%d Thread finished (ptid=%d)\n", tid, t.my_ptid);
  }
}

static bool HandleThreadJoinAfter(THREADID tid, pthread_t joined_ptid) {
  THREADID joined_tid = kMaxThreads;
  int max_uniq_tid_found = -1;

  // TODO(timurrrr): walking through g_pin_threads may be slow.
  // Do we need to/Can we optimize it?
  for (THREADID j = 1; j < kMaxThreads; j++) {
    if (g_pin_threads[j].thread_finished == false)
      continue;
    if (g_pin_threads[j].my_ptid == joined_ptid) {
      // We search for the thread with the maximum uniq_tid to work around
      // thread HANDLE reuse issues.
      if (max_uniq_tid_found < g_pin_threads[j].uniq_tid) {
        max_uniq_tid_found = g_pin_threads[j].uniq_tid;
        joined_tid = j;
      }
    }
  }
  if (joined_tid == kMaxThreads) {
    // This may happen in the following case:
    //  - A non-joinable thread is created and a handle is assigned to it.
    //  - Since the thread is non-joinable, the handle is then reused
    //  for some other purpose, e.g. for a WaitableEvent.
    //  - We did not yet register the thread fini event.
    //  - We observe WaitForSingleObjectEx(ptid) and think that this is thread
    //  join event, while it is not.
    if (debug_thread)
      Printf("T%d JoinAfter returns false! ptid=%d\n", tid, joined_ptid);
    return false;
  }
  CHECK(joined_tid < kMaxThreads);
  CHECK(joined_tid > 0);
  g_pin_threads[joined_tid].my_ptid = 0;
  int joined_uniq_tid = g_pin_threads[joined_tid].uniq_tid;

  if (debug_thread) {
    Printf("T%d JoinAfter   parent=%d child=%d (uniq=%d)\n", tid, tid,
           joined_tid, joined_uniq_tid);
  }

  // Here we send an event for a different thread (joined_tid), which is already
  // dead.
  DumpEvent(0, THR_END, joined_tid, 0, 0, 0);


  DumpEvent(0, THR_JOIN_AFTER, tid, 0, joined_uniq_tid, 0);
  return true;
}

static uintptr_t WRAP_NAME(pthread_join)(WRAP_PARAM4) {
  if (G_flags->debug_level >= 2)
    Printf("T%d in  pthread_join %p\n", tid, arg0);
  pthread_t joined_ptid = (pthread_t)arg0;
  uintptr_t ret = CALL_ME_INSIDE_WRAPPER_4();
  HandleThreadJoinAfter(tid, joined_ptid);
  if (G_flags->debug_level >= 2)
    Printf("T%d out pthread_join %p\n", tid, arg0);
  return ret;
}

static size_t WRAP_NAME(fwrite)(WRAP_PARAM4) {
  void* p = (void*)arg0;
  size_t size = (size_t)arg1 * (size_t)arg2;
  REPORT_READ_RANGE(p, size);
  IgnoreMopsBegin(tid);
  size_t ret = CALL_ME_INSIDE_WRAPPER_4();
  IgnoreMopsEnd(tid);
  return ret;
}

#ifdef _MSC_VER


uintptr_t CallStdCallFun1(CONTEXT *ctx, THREADID tid,
                         AFUNPTR f, uintptr_t arg0) {
  uintptr_t ret = 0xdeadbee1;
  PIN_CallApplicationFunction(ctx, tid,
                              CALLINGSTD_STDCALL, (AFUNPTR)(f),
                              PIN_PARG(uintptr_t), &ret,
                              PIN_PARG(uintptr_t), arg0,
                              PIN_PARG_END());
  return ret;
}

uintptr_t CallStdCallFun2(CONTEXT *ctx, THREADID tid,
                         AFUNPTR f, uintptr_t arg0, uintptr_t arg1) {
  uintptr_t ret = 0xdeadbee2;
  PIN_CallApplicationFunction(ctx, tid,
                              CALLINGSTD_STDCALL, (AFUNPTR)(f),
                              PIN_PARG(uintptr_t), &ret,
                              PIN_PARG(uintptr_t), arg0,
                              PIN_PARG(uintptr_t), arg1,
                              PIN_PARG_END());
  return ret;
}

uintptr_t CallStdCallFun3(CONTEXT *ctx, THREADID tid,
                         AFUNPTR f, uintptr_t arg0, uintptr_t arg1,
                         uintptr_t arg2) {
  uintptr_t ret = 0xdeadbee3;
  PIN_CallApplicationFunction(ctx, tid,
                              CALLINGSTD_STDCALL, (AFUNPTR)(f),
                              PIN_PARG(uintptr_t), &ret,
                              PIN_PARG(uintptr_t), arg0,
                              PIN_PARG(uintptr_t), arg1,
                              PIN_PARG(uintptr_t), arg2,
                              PIN_PARG_END());
  return ret;
}

uintptr_t CallStdCallFun4(CONTEXT *ctx, THREADID tid,
                         AFUNPTR f, uintptr_t arg0, uintptr_t arg1,
                         uintptr_t arg2, uintptr_t arg3) {
  uintptr_t ret = 0xdeadbee4;
  PIN_CallApplicationFunction(ctx, tid,
                              CALLINGSTD_STDCALL, (AFUNPTR)(f),
                              PIN_PARG(uintptr_t), &ret,
                              PIN_PARG(uintptr_t), arg0,
                              PIN_PARG(uintptr_t), arg1,
                              PIN_PARG(uintptr_t), arg2,
                              PIN_PARG(uintptr_t), arg3,
                              PIN_PARG_END());
  return ret;
}

uintptr_t CallStdCallFun5(CONTEXT *ctx, THREADID tid,
                         AFUNPTR f, uintptr_t arg0, uintptr_t arg1,
                         uintptr_t arg2, uintptr_t arg3,
                         uintptr_t arg4) {
  uintptr_t ret = 0xdeadbee5;
  PIN_CallApplicationFunction(ctx, tid,
                              CALLINGSTD_STDCALL, (AFUNPTR)(f),
                              PIN_PARG(uintptr_t), &ret,
                              PIN_PARG(uintptr_t), arg0,
                              PIN_PARG(uintptr_t), arg1,
                              PIN_PARG(uintptr_t), arg2,
                              PIN_PARG(uintptr_t), arg3,
                              PIN_PARG(uintptr_t), arg4,
                              PIN_PARG_END());
  return ret;
}

uintptr_t CallStdCallFun6(CONTEXT *ctx, THREADID tid,
                         AFUNPTR f, uintptr_t arg0, uintptr_t arg1,
                         uintptr_t arg2, uintptr_t arg3,
                         uintptr_t arg4, uintptr_t arg5) {
  uintptr_t ret = 0xdeadbee6;
  PIN_CallApplicationFunction(ctx, tid,
                              CALLINGSTD_STDCALL, (AFUNPTR)(f),
                              PIN_PARG(uintptr_t), &ret,
                              PIN_PARG(uintptr_t), arg0,
                              PIN_PARG(uintptr_t), arg1,
                              PIN_PARG(uintptr_t), arg2,
                              PIN_PARG(uintptr_t), arg3,
                              PIN_PARG(uintptr_t), arg4,
                              PIN_PARG(uintptr_t), arg5,
                              PIN_PARG_END());
  return ret;
}

uintptr_t CallStdCallFun7(CONTEXT *ctx, THREADID tid,
                         AFUNPTR f, uintptr_t arg0, uintptr_t arg1,
                         uintptr_t arg2, uintptr_t arg3,
                         uintptr_t arg4, uintptr_t arg5,
                         uintptr_t arg6) {
  uintptr_t ret = 0xdeadbee7;
  PIN_CallApplicationFunction(ctx, tid,
                              CALLINGSTD_STDCALL, (AFUNPTR)(f),
                              PIN_PARG(uintptr_t), &ret,
                              PIN_PARG(uintptr_t), arg0,
                              PIN_PARG(uintptr_t), arg1,
                              PIN_PARG(uintptr_t), arg2,
                              PIN_PARG(uintptr_t), arg3,
                              PIN_PARG(uintptr_t), arg4,
                              PIN_PARG(uintptr_t), arg5,
                              PIN_PARG(uintptr_t), arg6,
                              PIN_PARG_END());
  return ret;
}

uintptr_t WRAP_NAME(ResumeThread)(WRAP_PARAM4) {
//  Printf("T%d %s arg0=%p\n", tid, __FUNCTION__, arg0);
  DumpEvent(ctx, SIGNAL, tid, pc, arg0, 0);
  uintptr_t ret = CallStdCallFun1(ctx, tid, f, arg0);
  return ret;
}
uintptr_t WRAP_NAME(RtlInitializeCriticalSection)(WRAP_PARAM4) {
//  Printf("T%d pc=%p %s: %p\n", tid, pc, __FUNCTION__+8, arg0);
  DumpEvent(ctx, LOCK_CREATE, tid, pc, arg0, 0);
  IgnoreSyncAndMopsBegin(tid);
  uintptr_t ret = CallStdCallFun1(ctx, tid, f, arg0);
  IgnoreSyncAndMopsEnd(tid);
  return ret;
}
uintptr_t WRAP_NAME(RtlInitializeCriticalSectionAndSpinCount)(WRAP_PARAM4) {
//  Printf("T%d pc=%p %s: %p\n", tid, pc, __FUNCTION__+8, arg0);
  DumpEvent(ctx, LOCK_CREATE, tid, pc, arg0, 0);
  IgnoreSyncAndMopsBegin(tid);
  uintptr_t ret = CallStdCallFun2(ctx, tid, f, arg0, arg1);
  IgnoreSyncAndMopsEnd(tid);
  return ret;
}
uintptr_t WRAP_NAME(RtlInitializeCriticalSectionEx)(WRAP_PARAM4) {
//  Printf("T%d pc=%p %s: %p\n", tid, pc, __FUNCTION__+8, arg0);
  DumpEvent(ctx, LOCK_CREATE, tid, pc, arg0, 0);
  IgnoreSyncAndMopsBegin(tid);
  uintptr_t ret = CallStdCallFun3(ctx, tid, f, arg0, arg1, arg2);
  IgnoreSyncAndMopsEnd(tid);
  return ret;
}
uintptr_t WRAP_NAME(RtlDeleteCriticalSection)(WRAP_PARAM4) {
//  Printf("T%d pc=%p %s: %p\n", tid, pc, __FUNCTION__+8, arg0);
  DumpEvent(ctx, LOCK_DESTROY, tid, pc, arg0, 0);
  IgnoreSyncAndMopsBegin(tid);
  uintptr_t ret = CallStdCallFun1(ctx, tid, f, arg0);
  IgnoreSyncAndMopsEnd(tid);
  return ret;
}
uintptr_t WRAP_NAME(RtlEnterCriticalSection)(WRAP_PARAM4) {
//  Printf("T%d pc=%p %s: %p\n", tid, pc, __FUNCTION__+8, arg0);
  uintptr_t ret = CallStdCallFun1(ctx, tid, f, arg0);
  DumpEvent(ctx, WRITER_LOCK, tid, pc, arg0, 0);
  return ret;
}
uintptr_t WRAP_NAME(RtlTryEnterCriticalSection)(WRAP_PARAM4) {
  // Printf("T%d pc=%p %s: %p\n", tid, pc, __FUNCTION__+5, arg0);
  uintptr_t ret = CallStdCallFun1(ctx, tid, f, arg0);
  if (ret) {
    DumpEvent(ctx, WRITER_LOCK, tid, pc, arg0, 0);
  }
  return ret;
}
uintptr_t WRAP_NAME(RtlLeaveCriticalSection)(WRAP_PARAM4) {
//  Printf("T%d pc=%p %s: %p\n", tid, pc, __FUNCTION__+8, arg0);
  DumpEvent(ctx, UNLOCK, tid, pc, arg0, 0);
  return CallStdCallFun1(ctx, tid, f, arg0);
}

uintptr_t WRAP_NAME(DuplicateHandle)(WRAP_PARAM8) {
  Printf("WARNING: DuplicateHandle called for handle 0x%X.\n", arg1);
  Printf("Future events on this handle may be processed incorrectly.\n");
  return CallStdCallFun7(ctx, tid, f, arg0, arg1, arg2, arg3, arg4, arg5, arg6);
}

uintptr_t WRAP_NAME(SetEvent)(WRAP_PARAM4) {
  //Printf("T%d before pc=%p %s: %p\n", tid, pc, __FUNCTION__+8, arg0);
  DumpEvent(ctx, SIGNAL, tid, pc, arg0, 0);
  uintptr_t ret = CallStdCallFun1(ctx, tid, f, arg0);
  //Printf("T%d after pc=%p %s: %p\n", tid, pc, __FUNCTION__+8, arg0);
  return ret;
}

uintptr_t InternalWrapCreateSemaphore(WRAP_PARAM4) {
  if (arg3 != NULL) {
    Printf("WARNING: CreateSemaphore called with lpName='%s'.\n", arg3);
    Printf("Future events on this semaphore may be processed incorrectly "
           "if it is reused.\n");
  }
  return CallStdCallFun4(ctx, tid, f, arg0, arg1, arg2, arg3);
}

uintptr_t WRAP_NAME(CreateSemaphoreA)(WRAP_PARAM4) {
  return InternalWrapCreateSemaphore(tid, pc, ctx, f, arg0, arg1, arg2, arg3);
}

uintptr_t WRAP_NAME(CreateSemaphoreW)(WRAP_PARAM4) {
  return InternalWrapCreateSemaphore(tid, pc, ctx, f, arg0, arg1, arg2, arg3);
}

uintptr_t WRAP_NAME(ReleaseSemaphore)(WRAP_PARAM4) {
  DumpEvent(ctx, SIGNAL, tid, pc, arg0, 0);
  return CallStdCallFun3(ctx, tid, f, arg0, arg1, arg2);
}

uintptr_t WRAP_NAME(RtlInterlockedPushEntrySList)(WRAP_PARAM4) {
  DumpEvent(ctx, SIGNAL, tid, pc, arg1, 0);
  uintptr_t ret = CallStdCallFun2(ctx, tid, f, arg0, arg1);
  // Printf("T%d %s list=%p item=%p\n", tid, __FUNCTION__, arg0, arg1);
  return ret;
}

uintptr_t WRAP_NAME(RtlInterlockedPopEntrySList)(WRAP_PARAM4) {
  uintptr_t ret = CallStdCallFun1(ctx, tid, f, arg0);
  // Printf("T%d %s list=%p item=%p\n", tid, __FUNCTION__, arg0, ret);
  if (ret) {
    DumpEvent(ctx, WAIT, tid, pc, ret, 0);
  }
  return ret;
}

uintptr_t WRAP_NAME(RtlAcquireSRWLockExclusive)(WRAP_PARAM4) {
  uintptr_t ret = CallStdCallFun1(ctx, tid, f, arg0);
  DumpEvent(ctx, WRITER_LOCK, tid, pc, arg0, 0);
  return ret;
}
uintptr_t WRAP_NAME(RtlAcquireSRWLockShared)(WRAP_PARAM4) {
  uintptr_t ret = CallStdCallFun1(ctx, tid, f, arg0);
  DumpEvent(ctx, READER_LOCK, tid, pc, arg0, 0);
  return ret;
}
uintptr_t WRAP_NAME(RtlTryAcquireSRWLockExclusive)(WRAP_PARAM4) {
  // Printf("T%d %s %p\n", tid, __FUNCTION__, arg0);
  uintptr_t ret = CallStdCallFun1(ctx, tid, f, arg0);
  if (ret & 0xFF) {  // Looks like this syscall return value is just 1 byte.
    DumpEvent(ctx, WRITER_LOCK, tid, pc, arg0, 0);
  }
  return ret;
}
uintptr_t WRAP_NAME(RtlTryAcquireSRWLockShared)(WRAP_PARAM4) {
  // Printf("T%d %s %p\n", tid, __FUNCTION__, arg0);
  uintptr_t ret = CallStdCallFun1(ctx, tid, f, arg0);
  if (ret & 0xFF) {  // Looks like this syscall return value is just 1 byte.
    DumpEvent(ctx, READER_LOCK, tid, pc, arg0, 0);
  }
  return ret;
}
uintptr_t WRAP_NAME(RtlReleaseSRWLockExclusive)(WRAP_PARAM4) {
  // Printf("T%d %s %p\n", tid, __FUNCTION__, arg0);
  DumpEvent(ctx, UNLOCK, tid, pc, arg0, 0);
  uintptr_t ret = CallStdCallFun1(ctx, tid, f, arg0);
  return ret;
}
uintptr_t WRAP_NAME(RtlReleaseSRWLockShared)(WRAP_PARAM4) {
  // Printf("T%d %s %p\n", tid, __FUNCTION__, arg0);
  DumpEvent(ctx, UNLOCK, tid, pc, arg0, 0);
  uintptr_t ret = CallStdCallFun1(ctx, tid, f, arg0);
  return ret;
}
uintptr_t WRAP_NAME(RtlInitializeSRWLock)(WRAP_PARAM4) {
  // Printf("T%d %s %p\n", tid, __FUNCTION__, arg0);
  DumpEvent(ctx, LOCK_CREATE, tid, pc, arg0, 0);
  uintptr_t ret = CallStdCallFun1(ctx, tid, f, arg0);
  return ret;
}

uintptr_t WRAP_NAME(RtlWakeConditionVariable)(WRAP_PARAM4) {
  // Printf("T%d %s arg0=%p\n", tid, __FUNCTION__, arg0);
  DumpEvent(ctx, SIGNAL, tid, pc, arg0, 0);
  uintptr_t ret = CallStdCallFun1(ctx, tid, f, arg0);
  return ret;
}
uintptr_t WRAP_NAME(RtlWakeAllConditionVariable)(WRAP_PARAM4) {
  // Printf("T%d %s arg0=%p\n", tid, __FUNCTION__, arg0);
  DumpEvent(ctx, SIGNAL, tid, pc, arg0, 0);
  uintptr_t ret = CallStdCallFun1(ctx, tid, f, arg0);
  return ret;
}
uintptr_t WRAP_NAME(RtlSleepConditionVariableSRW)(WRAP_PARAM4) {
  // No need to unlock/lock - looks like RtlSleepConditionVariableSRW performs
  // Rtl{Acquire,Release}SRW... calls itself!
  uintptr_t ret = CallStdCallFun4(ctx, tid, f, arg0, arg1, arg2, arg3);
  if ((ret & 0xFF) == 0)
    DumpEvent(ctx, WAIT, tid, pc, arg0, 0);
  // Printf("T%d %s arg0=%p arg1=%p; ret=%d\n", tid, __FUNCTION__, arg0, arg1, ret);
  return ret;
}
uintptr_t WRAP_NAME(RtlSleepConditionVariableCS)(WRAP_PARAM4) {
  // TODO(timurrrr): do we need unlock/lock?
  uintptr_t ret = CallStdCallFun3(ctx, tid, f, arg0, arg1, arg2);
  if ((ret & 0xFF) == 0)
    DumpEvent(ctx, WAIT, tid, pc, arg0, 0);
  // Printf("T%d %s arg0=%p arg1=%p; ret=%d\n", tid, __FUNCTION__, arg0, arg1, ret);
  return ret;
}

uintptr_t WRAP_NAME(RtlQueueWorkItem)(WRAP_PARAM4) {
  // Printf("T%d %s arg0=%p arg1=%p; arg2=%d\n", tid, __FUNCTION__, arg0, arg1, arg2);
  g_windows_thread_pool_calback_set->insert(arg0);
  DumpEvent(ctx, SIGNAL, tid, pc, arg0, 0);
  uintptr_t ret = CallStdCallFun3(ctx, tid, f, arg0, arg1, arg2);
  return ret;
}

uintptr_t WRAP_NAME(RegisterWaitForSingleObject)(WRAP_PARAM6) {
  // Printf("T%d %s arg0=%p arg2=%p\n", tid, __FUNCTION__, arg0, arg2);
  g_windows_thread_pool_calback_set->insert(arg2);
  DumpEvent(ctx, SIGNAL, tid, pc, arg2, 0);
  uintptr_t ret = CallStdCallFun6(ctx, tid, f, arg0, arg1, arg2, arg3, arg4, arg5);
  if (ret) {
    uintptr_t wait_object = *(uintptr_t*)arg0;
    (*g_windows_thread_pool_wait_object_map)[wait_object] = arg2;
    // Printf("T%d %s *arg0=%p\n", tid, __FUNCTION__, wait_object);
  }
  return ret;
}

uintptr_t WRAP_NAME(UnregisterWaitEx)(WRAP_PARAM4) {
  CHECK(g_windows_thread_pool_wait_object_map);
  uintptr_t obj = (*g_windows_thread_pool_wait_object_map)[arg0];
  // Printf("T%d %s arg0=%p obj=%p\n", tid, __FUNCTION__, arg0, obj);
  uintptr_t ret = CallStdCallFun2(ctx, tid, f, arg0, arg1);
  if (ret) {
    DumpEvent(ctx, WAIT, tid, pc, obj, 0);
  }
  return ret;
}

uintptr_t WRAP_NAME(VirtualAlloc)(WRAP_PARAM4) {
  // Printf("T%d VirtualAlloc: %p %p %p %p\n", tid, arg0, arg1, arg2, arg3);
  uintptr_t ret = CallStdCallFun4(ctx, tid, f, arg0, arg1, arg2, arg3);
  return ret;
}

uintptr_t WRAP_NAME(GlobalAlloc)(WRAP_PARAM4) {
  uintptr_t ret = CallStdCallFun2(ctx, tid, f, arg0, arg1);
  // Printf("T%d %s(%p %p)=%p\n", tid, __FUNCTION__, arg0, arg1, ret);
  if (ret != 0) {
    DumpEvent(ctx, MALLOC, tid, pc, ret, arg1);
  }
  return ret;
}

uintptr_t WRAP_NAME(ZwAllocateVirtualMemory)(WRAP_PARAM6) {
  // Printf("T%d >>%s(%p %p %p %p %p %p)\n", tid, __FUNCTION__, arg0, arg1, arg2, arg3, arg4, arg5);
  uintptr_t ret = CallStdCallFun6(ctx, tid, f, arg0, arg1, arg2, arg3, arg4, arg5);
  // Printf("T%d <<%s(%p %p) = %p\n", tid, __FUNCTION__, *(void**)arg1, *(void**)arg3, ret);
  if (ret == 0) {
    DumpEvent(ctx, MALLOC, tid, pc, *(uintptr_t*)arg1, *(uintptr_t*)arg3);
  }
  return ret;
}

uintptr_t WRAP_NAME(AllocateHeap)(WRAP_PARAM4) {
  uintptr_t ret = CallStdCallFun3(ctx, tid, f, arg0, arg1, arg2);
  // Printf("T%d RtlAllocateHeap(%p %p %p)=%p\n", tid, arg0, arg1, arg2, ret);
  if (ret != 0) {
    DumpEvent(ctx, MALLOC, tid, pc, ret, arg3);
  }
  return ret;
}

uintptr_t WRAP_NAME(HeapCreate)(WRAP_PARAM4) {
  uintptr_t ret = CallStdCallFun3(ctx, tid, f, arg0, arg1, arg2);
  Printf("T%d %s(%p %p %p)=%p\n", tid, __FUNCTION__, arg0, arg1, arg2, ret);
  return ret;
}

// We don't use the definition of WAIT_OBJECT_0 from winbase.h because
// it can't be compiled here for some reason.
#define WAIT_OBJECT_0_ 0

uintptr_t WRAP_NAME(WaitForSingleObjectEx)(WRAP_PARAM4) {
  if (G_flags->verbosity >= 1) {
    ShowPcAndSp(__FUNCTION__, tid, pc, 0);
    Printf("arg0=%lx arg1=%lx\n", arg0, arg1);
  }

  //Printf("T%d before pc=%p %s: %p\n", tid, pc, __FUNCTION__+8, arg0, arg1);
  uintptr_t ret = CallStdCallFun3(ctx, tid, f, arg0, arg1, arg2);
  //Printf("T%d after pc=%p %s: %p\n", tid, pc, __FUNCTION__+8, arg0, arg1);

  if (ret == WAIT_OBJECT_0_) {
    bool is_thread_handle = false;
    {
      ScopedReentrantClientLock lock(__LINE__);
      if (g_win_handles_which_are_threads) {
        is_thread_handle = g_win_handles_which_are_threads->count(arg0) > 0;
        g_win_handles_which_are_threads->erase(arg0);
      }
    }
    if (is_thread_handle)
      HandleThreadJoinAfter(tid, arg0);
    DumpEvent(ctx, WAIT, tid, pc, arg0, 0);
  }

  return ret;
}

uintptr_t WRAP_NAME(WaitForMultipleObjectsEx)(WRAP_PARAM6) {
  if (G_flags->verbosity >= 1) {
    ShowPcAndSp(__FUNCTION__, tid, pc, 0);
    Printf("arg0=%lx arg1=%lx arg2=%lx arg3=%lx\n", arg0, arg1, arg2, arg3);
  }

  //Printf("T%d before pc=%p %s: %p\n", tid, pc, __FUNCTION__+8, arg0, arg1);
  uintptr_t ret = CallStdCallFun5(ctx, tid, f, arg0, arg1, arg2, arg3, arg4);
  //Printf("T%d after pc=%p %s: %p\n", tid, pc, __FUNCTION__+8, arg0, arg1);

  if (ret >= WAIT_OBJECT_0_ && ret < WAIT_OBJECT_0_ + arg0) {
    // TODO(timurrrr): add support for WAIT_ABANDONED_0

    int start_id, count;
    if (arg2 /* wait_for_all */ == 1) {
      start_id = 0;
      count = arg0;
    } else {
      start_id = ret - WAIT_OBJECT_0_;
      count = 1;
    }

    for (int i = start_id; i < start_id + count; i++) {
      uintptr_t handle = ((uintptr_t*)arg1)[i];
      bool is_thread_handle = false;
      {
        ScopedReentrantClientLock lock(__LINE__);
        if (g_win_handles_which_are_threads) {
          is_thread_handle = g_win_handles_which_are_threads->count(handle) > 0;
          g_win_handles_which_are_threads->erase(handle);
        }
      }
      if (is_thread_handle)
        HandleThreadJoinAfter(tid, handle);
      DumpEvent(ctx, WAIT, tid, pc, handle, 0);
    }
  }

  return ret;
}

#endif  // _MSC_VER

//--------- memory allocation ---------------------- {{{2
uintptr_t WRAP_NAME(mmap)(WRAP_PARAM6) {
  uintptr_t ret = CALL_ME_INSIDE_WRAPPER_6();

  if (ret != (ADDRINT)-1L) {
    DumpEvent(ctx, MMAP, tid, pc, ret, arg1);
  }

  return ret;
}

uintptr_t WRAP_NAME(munmap)(WRAP_PARAM4) {
  PinThread &t = g_pin_threads[tid];
  TLEBFlushLocked(t);
  uintptr_t ret = CALL_ME_INSIDE_WRAPPER_4();
  if (ret != (uintptr_t)-1L) {
    DumpEvent(ctx, MUNMAP, tid, pc, arg0, arg1);
  }
  return ret;
}


void After_malloc(FAST_WRAP_PARAM_AFTER) {
  size_t size = frame.arg[0];
  if (DEBUG_FAST_INTERCEPTORS)
    Printf("T%d %s %ld %p\n", tid, __FUNCTION__, size, ret);
  IgnoreSyncAndMopsEnd(tid);
  DumpEventWithSp(frame.sp, MALLOC, tid, frame.pc, ret, size);
}

void Before_malloc(FAST_WRAP_PARAM1) {
  IgnoreSyncAndMopsBegin(tid);
  PUSH_AFTER_CALLBACK1(After_malloc, arg0);
}

void After_free(FAST_WRAP_PARAM_AFTER) {
  if (DEBUG_FAST_INTERCEPTORS)
    Printf("T%d %s %p\n", tid, __FUNCTION__, frame.arg[0]);
  IgnoreSyncAndMopsEnd(tid);
}

void Before_free(FAST_WRAP_PARAM1) {
  PinThread &t = g_pin_threads[tid];
  TLEBFlushLocked(t);
  DumpEvent(0, FREE, tid, pc, arg0, 0);
  IgnoreSyncAndMopsBegin(tid);
  PUSH_AFTER_CALLBACK1(After_free, arg0);
}

void Before_calloc(FAST_WRAP_PARAM2) {
  IgnoreSyncAndMopsBegin(tid);
  PUSH_AFTER_CALLBACK1(After_malloc, arg0 * arg1);
}

void Before_realloc(FAST_WRAP_PARAM2) {
  PinThread &t = g_pin_threads[tid];
  TLEBFlushLocked(t);
  IgnoreSyncAndMopsBegin(tid);
  // TODO: handle FREE? We don't do it in Valgrind right now.
  PUSH_AFTER_CALLBACK1(After_malloc, arg1);
}

// Fast path for INS_InsertIfCall.
ADDRINT Before_RET_IF(THREADID tid, ADDRINT pc, ADDRINT sp, ADDRINT ret) {
  PinThread &t = g_pin_threads[tid];
  return t.ic_stack.size();
}

void Before_RET_THEN(THREADID tid, ADDRINT pc, ADDRINT sp, ADDRINT ret) {
  PinThread &t = g_pin_threads[tid];
  if (t.ic_stack.size() == 0) return;
  DCHECK(t.ic_stack.size());
  InstrumentedCallFrame *frame = t.ic_stack.Top();
  if (DEBUG_FAST_INTERCEPTORS) {
    Printf("T%d RET  pc=%p sp=%p *sp=%p frame.sp=%p stack_size %ld\n",
           tid, pc, sp, *(uintptr_t*)sp, frame->sp, t.ic_stack.size());
    t.ic_stack.Print();
  }
  while (frame->sp <= sp) {
    if (DEBUG_FAST_INTERCEPTORS)
      Printf("pop\n");
    frame->callback(tid, *frame, ret);
    t.ic_stack.Pop();
    if (t.ic_stack.size()) {
      frame = t.ic_stack.Top();
    } else {
      break;
    }
  }
}

// These are no longer used in favor of "fast" wrappers (e.g. Before_malloc)
// TODO(timurrrr): Check on the buildbot and remove.
uintptr_t WRAP_NAME(malloc)(WRAP_PARAM4) { CHECK(0); }
uintptr_t WRAP_NAME(realloc)(WRAP_PARAM4) { CHECK(0); }
uintptr_t WRAP_NAME(calloc)(WRAP_PARAM4) { CHECK(0); }
uintptr_t WRAP_NAME(free)(WRAP_PARAM4) { CHECK(0); }


//-------- Routines and stack ---------------------- {{{2
static INLINE void UpdateCallStack(PinThread &t, ADDRINT sp) {
  while (t.shadow_stack.size() > 0 && sp >= t.shadow_stack.back().sp) {
    TLEBAddRtnExit(t);
    size_t size = t.shadow_stack.size();
    CHECK(size < 1000000);  // stay sane.
    uintptr_t popped_pc = t.shadow_stack.back().pc;
#ifdef _MSC_VER
    // h-b edge from here to UnregisterWaitEx.
    CHECK(g_windows_thread_pool_calback_set);
    if (g_windows_thread_pool_calback_set->count(popped_pc)) {
      DumpEvent(0, SIGNAL, t.tid, 0, popped_pc, 0);
      // Printf("T%d ret %p\n", t.tid, popped_pc);
    }
#endif

    if (debug_rtn) {
      ShowPcAndSp("RET : ", t.tid, popped_pc, sp);
    }
    t.shadow_stack.pop_back();
    CHECK(size - 1 == t.shadow_stack.size());
    if (DEB_PR) {
      Printf("POP SHADOW STACK\n");
      PrintShadowStack(t);
    }
  }
}

void InsertBeforeEvent_SysCall(THREADID tid, ADDRINT sp) {
  PinThread &t = g_pin_threads[tid];
  UpdateCallStack(t, sp);
  TLEBFlushLocked(t);
}

void InsertBeforeEvent_Call(THREADID tid, ADDRINT pc, ADDRINT target,
                            ADDRINT sp, IGNORE_BELOW_RTN ignore_below) {
  PinThread &t = g_pin_threads[tid];
  DebugOnlyShowPcAndSp(__FUNCTION__, t.tid, pc, sp);
  UpdateCallStack(t, sp);
  TLEBAddRtnCall(t, pc, target, ignore_below);
  t.shadow_stack.push_back(StackFrame(target, sp));
  if (DEB_PR) {
    PrintShadowStack(t);
  }
  if (DEBUG_MODE && debug_rtn) {
    ShowPcAndSp("CALL: ", t.tid, target, sp);
  }

#ifdef _MSC_VER
  // h-b edge from RtlQueueWorkItem to here.
  CHECK(g_windows_thread_pool_calback_set);
  if (g_windows_thread_pool_calback_set->count(target)) {
    DumpEvent(0, WAIT, tid, pc, target, 0);
  }
#endif
}

static void OnTraceSerial(THREADID tid, ADDRINT sp, TraceInfo *trace_info,
    uintptr_t **tls_reg_p) {
  PinThread &t = g_pin_threads[tid];

  DCHECK(trace_info);
  DCHECK(trace_info->n_mops() > 0);
  DebugOnlyShowPcAndSp(__FUNCTION__, t.tid, trace_info->pc(), sp);

  UpdateCallStack(t, sp);

  t.trace_info = trace_info;
  trace_info->counter()++;
  *tls_reg_p = TLEBAddTrace(t);
}

static void OnTraceParallel(uintptr_t *tls_reg, ADDRINT sp, TraceInfo *trace_info) {
  // Get the thread handler directly from tls_reg.
  PinThread &t = *(PinThread*)(tls_reg - 4);
  t.trace_info = trace_info;
  if (t.ignore_accesses) return;

  DCHECK(trace_info);
  DCHECK(trace_info->n_mops() > 0);
  DebugOnlyShowPcAndSp(__FUNCTION__, t.tid, trace_info->pc(), sp);

  UpdateCallStack(t, sp);


  if (DEBUG_MODE && G_flags->show_stats)  // this stat may be racey; avoid ping-pong.
    trace_info->counter()++;
  TLEBAddTrace(t);
}

/* Verify all mop accesses in the last trace of the given thread by registering
   them with RaceVerifier and sleeping a bit. */
static void OnTraceVerifyInternal(PinThread &t, uintptr_t **tls_reg_p) {
  DCHECK(g_race_verifier_active);
  if (t.trace_info) {
    int need_sleep = 0;
    for (unsigned i = 0; i < t.trace_info->n_mops(); ++i) {
      uintptr_t addr = (*tls_reg_p)[i];
      if (addr) {
        MopInfo *mop = t.trace_info->GetMop(i);
        need_sleep += RaceVerifierStartAccess(t.uniq_tid, addr, mop->pc(),
            mop->is_write());
      }
    }

    if (!need_sleep)
      return;

    usleep(G_flags->race_verifier_sleep_ms * 1000);

    for (unsigned i = 0; i < t.trace_info->n_mops(); ++i) {
      uintptr_t addr = (*tls_reg_p)[i];
      if (addr) {
        MopInfo *mop = t.trace_info->GetMop(i);
        RaceVerifierEndAccess(t.uniq_tid, addr, mop->pc(), mop->is_write());
      }
    }
  }
}

static void OnTraceNoMopsVerify(THREADID tid, ADDRINT sp,
    uintptr_t **tls_reg_p) {
  PinThread &t = g_pin_threads[tid];
  DCHECK(g_race_verifier_active);
  OnTraceVerifyInternal(t, tls_reg_p);
  t.trace_info = NULL;
}

static void OnTraceVerify(THREADID tid, ADDRINT sp, TraceInfo *trace_info,
    uintptr_t **tls_reg_p) {
  DCHECK(g_race_verifier_active);
  PinThread &t = g_pin_threads[tid];
  OnTraceVerifyInternal(t, tls_reg_p);

  DCHECK(trace_info->n_mops() > 0);

  t.trace_info = trace_info;
  trace_info->counter()++;
  *tls_reg_p = TLEBAddTrace(t);
}


//---------- Memory accesses -------------------------- {{{2
// 'addr' is the section of t.tleb.events which is set in OnTrace.
// 'idx' is the number of this mop in its trace.
// 'a' is the actuall address.
// 'tid' is thread ID, used only in debug mode.
//
// In opt mode this is just one instruction! Something like this:
// mov %rcx,(%rdi,%rdx,8)
static void OnMop(uintptr_t *addr, THREADID tid, ADDRINT idx, ADDRINT a) {
  if (DEBUG_MODE) {
    PinThread &t= g_pin_threads[tid];
    CHECK(idx < kMaxMopsPerTrace);
    CHECK(idx < t.trace_info->n_mops());
    uintptr_t *ptr = addr + idx;
    CHECK(ptr >= t.tleb.events);
    CHECK(ptr < t.tleb.events + kThreadLocalEventBufferSize);
    if (a == G_flags->trace_addr) {
      Printf("T%d %s %lx\n", t.tid, __FUNCTION__, a);
    }
  }
  addr[idx] = a;
}

static void On_PredicatedMop(BOOL is_running, uintptr_t *addr,
                             THREADID tid, ADDRINT idx, ADDRINT a) {
  if (is_running) {
    OnMop(addr, tid, idx, a);
  }
}

static void OnMopCheckIdentStoreBefore(uintptr_t *addr, THREADID tid, ADDRINT idx, ADDRINT a) {
  // Write the value of *a to tleb.
  addr[idx] = *(uintptr_t*)a;
}
static void OnMopCheckIdentStoreAfter(uintptr_t *addr, THREADID tid, ADDRINT idx, ADDRINT a) {
  // Check if the previous value of *a is equal to the new one.
  // If not, we have a regular memory access. If yes, we have an ident operation,
  // which we want to ignore.
  uintptr_t previous_value_of_a = addr[idx];
  uintptr_t new_value_of_a = *(uintptr_t*)a;
  // 111...111 if the values are different, 0 otherwise.
  uintptr_t ne_mask = -(uintptr_t)(new_value_of_a != previous_value_of_a);
  addr[idx] = ne_mask & a;
}

//---------- I/O; exit------------------------------- {{{2
static const uintptr_t kIOMagic = 0x1234c678;

static void Before_SignallingIOCall(THREADID tid, ADDRINT pc) {
  DumpEvent(0, SIGNAL, tid, pc, kIOMagic, 0);
}

static void After_WaitingIOCall(THREADID tid, ADDRINT pc) {
  DumpEvent(0, WAIT, tid, pc, kIOMagic, 0);
}

static const uintptr_t kAtexitMagic = 0x9876f432;

static void On_atexit(THREADID tid, ADDRINT pc) {
  DumpEvent(0, SIGNAL, tid, pc, kAtexitMagic, 0);
}

static void On_exit(THREADID tid, ADDRINT pc) {
  DumpEvent(0, WAIT, tid, pc, kAtexitMagic, 0);
}

//---------- Synchronization -------------------------- {{{2
// locks
static void Before_pthread_unlock(THREADID tid, ADDRINT pc, ADDRINT mu) {
  DumpEvent(0, UNLOCK, tid, pc, mu, 0);
}

static void After_pthread_mutex_lock(FAST_WRAP_PARAM_AFTER) {
  DumpEventWithSp(frame.sp, WRITER_LOCK, tid, frame.pc, frame.arg[0], 0);
}

static void Before_pthread_mutex_lock(FAST_WRAP_PARAM1) {
  PUSH_AFTER_CALLBACK1(After_pthread_mutex_lock, arg0);
}

// In some versions of libpthread, pthread_spin_lock is effectively
// a recursive function. It jumps to its first insn:
//    beb0:       f0 ff 0f                lock decl (%rdi)
//    beb3:       75 0b                   jne    bec0 <pthread_spin_lock+0x10>
//    beb5:       31 c0                   xor    %eax,%eax
//    beb7:       c3                      retq
//    beb8:       0f 1f 84 00 00 00 00    nopl   0x0(%rax,%rax,1)
//    bebf:       00
//    bec0:       f3 90                   pause
//    bec2:       83 3f 00                cmpl   $0x0,(%rdi)
//    bec5:       7f e9  >>>>>>>>>>>>>    jg     beb0 <pthread_spin_lock> 
//    bec7:       eb f7                   jmp    bec0 <pthread_spin_lock+0x10>
//
// So, we need to act only when we return from the last (depth=0) invocation.
static uintptr_t WRAP_NAME(pthread_spin_lock)(WRAP_PARAM4) {
  PinThread &t= g_pin_threads[tid];
  t.spin_lock_recursion_depth++;
  uintptr_t ret = CALL_ME_INSIDE_WRAPPER_4();
  t.spin_lock_recursion_depth--;
  if (t.spin_lock_recursion_depth == 0) {
    DumpEvent(ctx, WRITER_LOCK, tid, pc, arg0, 0);
  }
  return ret;
}

static uintptr_t WRAP_NAME(pthread_rwlock_wrlock)(WRAP_PARAM4) {
  uintptr_t ret = CALL_ME_INSIDE_WRAPPER_4();
  DumpEvent(ctx, WRITER_LOCK, tid, pc, arg0, 0);
  return ret;
}

static uintptr_t WRAP_NAME(pthread_rwlock_rdlock)(WRAP_PARAM4) {
  uintptr_t ret = CALL_ME_INSIDE_WRAPPER_4();
  DumpEvent(ctx, READER_LOCK, tid, pc, arg0, 0);
  return ret;
}

static uintptr_t WRAP_NAME(pthread_mutex_trylock)(WRAP_PARAM4) {
  uintptr_t ret = CALL_ME_INSIDE_WRAPPER_4();
  if (ret == 0)
    DumpEvent(ctx, WRITER_LOCK, tid, pc, arg0, 0);
  return ret;
}

static uintptr_t WRAP_NAME(pthread_spin_trylock)(WRAP_PARAM4) {
  uintptr_t ret = CALL_ME_INSIDE_WRAPPER_4();
  if (ret == 0)
    DumpEvent(ctx, WRITER_LOCK, tid, pc, arg0, 0);
  return ret;
}

static uintptr_t WRAP_NAME(pthread_spin_init)(WRAP_PARAM4) {
  DumpEvent(ctx, UNLOCK_OR_INIT, tid, pc, arg0, 0);
  uintptr_t ret = CALL_ME_INSIDE_WRAPPER_4();
  return ret;
}
static uintptr_t WRAP_NAME(pthread_spin_destroy)(WRAP_PARAM4) {
  DumpEvent(ctx, LOCK_DESTROY, tid, pc, arg0, 0);
  uintptr_t ret = CALL_ME_INSIDE_WRAPPER_4();
  return ret;
}
static uintptr_t WRAP_NAME(pthread_spin_unlock)(WRAP_PARAM4) {
  DumpEvent(ctx, UNLOCK_OR_INIT, tid, pc, arg0, 0);
  uintptr_t ret = CALL_ME_INSIDE_WRAPPER_4();
  return ret;
}

static uintptr_t WRAP_NAME(pthread_rwlock_trywrlock)(WRAP_PARAM4) {
  uintptr_t ret = CALL_ME_INSIDE_WRAPPER_4();
  if (ret == 0)
    DumpEvent(ctx, WRITER_LOCK, tid, pc, arg0, 0);
  return ret;
}

static uintptr_t WRAP_NAME(pthread_rwlock_tryrdlock)(WRAP_PARAM4) {
  uintptr_t ret = CALL_ME_INSIDE_WRAPPER_4();
  if (ret == 0)
    DumpEvent(ctx, READER_LOCK, tid, pc, arg0, 0);
  return ret;
}


static void Before_pthread_mutex_init(THREADID tid, ADDRINT pc, ADDRINT mu) {
  DumpEvent(0, LOCK_CREATE, tid, pc, mu, 0);
}
static void Before_pthread_rwlock_init(THREADID tid, ADDRINT pc, ADDRINT mu) {
  DumpEvent(0, LOCK_CREATE, tid, pc, mu, 0);
}

static void Before_pthread_mutex_destroy(THREADID tid, ADDRINT pc, ADDRINT mu) {
  DumpEvent(0, LOCK_DESTROY, tid, pc, mu, 0);
}
static void Before_pthread_rwlock_destroy(THREADID tid, ADDRINT pc, ADDRINT mu) {
  DumpEvent(0, LOCK_DESTROY, tid, pc, mu, 0);
}

// barrier
static uintptr_t WRAP_NAME(pthread_barrier_init)(WRAP_PARAM4) {
  DumpEvent(ctx, CYCLIC_BARRIER_INIT, tid, pc, arg0, arg2);
  uintptr_t ret = CALL_ME_INSIDE_WRAPPER_4();
  return ret;
}
static uintptr_t WRAP_NAME(pthread_barrier_wait)(WRAP_PARAM4) {
  DumpEvent(ctx, CYCLIC_BARRIER_WAIT_BEFORE, tid, pc, arg0, 0);
  uintptr_t ret = CALL_ME_INSIDE_WRAPPER_4();
  DumpEvent(ctx, CYCLIC_BARRIER_WAIT_AFTER, tid, pc, arg0, 0);
  return ret;
}


// condvar
static void Before_pthread_cond_signal(THREADID tid, ADDRINT pc, ADDRINT cv) {
  DumpEvent(0, SIGNAL, tid, pc, cv, 0);
}

static uintptr_t WRAP_NAME(pthread_cond_wait)(WRAP_PARAM4) {
  DumpEvent(ctx, UNLOCK, tid, pc, arg1, 0);
  uintptr_t ret = CALL_ME_INSIDE_WRAPPER_4();
  DumpEvent(ctx, WAIT, tid, pc, arg0, 0);
  DumpEvent(ctx, WRITER_LOCK, tid, pc, arg1, 0);
  return ret;
}
static uintptr_t WRAP_NAME(pthread_cond_timedwait)(WRAP_PARAM4) {
  DumpEvent(ctx, UNLOCK, tid, pc, arg1, 0);
  uintptr_t ret = CALL_ME_INSIDE_WRAPPER_4();
  if (ret == 0) {
    DumpEvent(ctx, WAIT, tid, pc, arg0, 0);
  }
  DumpEvent(ctx, WRITER_LOCK, tid, pc, arg1, 0);
  return ret;
}

// epoll
static const uintptr_t kSocketMagic = 0xDEADFBAD;

static void Before_epoll_ctl(THREADID tid, ADDRINT pc) {
  DumpEvent(0, SIGNAL, tid, pc, kSocketMagic, 0);
}

static void After_epoll_wait(THREADID tid, ADDRINT pc) {
  DumpEvent(0, WAIT, tid, pc, kSocketMagic, 0);
}

// sem
static void After_sem_open(THREADID tid, ADDRINT pc, ADDRINT ret) {
  // TODO(kcc): need to handle it more precise?
  DumpEvent(0, SIGNAL, tid, pc, ret, 0);
}
static void Before_sem_post(THREADID tid, ADDRINT pc, ADDRINT sem) {
  DumpEvent(0, SIGNAL, tid, pc, sem, 0);
}

static uintptr_t WRAP_NAME(sem_wait)(WRAP_PARAM4) {
  uintptr_t ret = CALL_ME_INSIDE_WRAPPER_4();
  DumpEvent(ctx, WAIT, tid, pc, arg0, 0);
  return ret;
}
static uintptr_t WRAP_NAME(sem_trywait)(WRAP_PARAM4) {
  uintptr_t ret = CALL_ME_INSIDE_WRAPPER_4();
  if (ret == 0) {
    DumpEvent(ctx, WAIT, tid, pc, arg0, 0);
  }
  return ret;
}

// etc
#if defined(__GNUC__)
uintptr_t WRAP_NAME(lockf)(WRAP_PARAM4) {
  if (arg1 == F_ULOCK) {
    DumpEvent(0, SIGNAL, tid, pc, kSocketMagic, 0);
  }

  uintptr_t ret = CALL_ME_INSIDE_WRAPPER_4();

  if (arg1 == F_LOCK && ret == 0) {
    DumpEvent(0, WAIT, tid, pc, kSocketMagic, 0);
  }

  return ret;
}
#endif

//--------- Annotations -------------------------- {{{2
static void On_AnnotateBenignRace(THREADID tid, ADDRINT pc,
                                  ADDRINT file, ADDRINT line,
                                  ADDRINT a, ADDRINT descr) {
  DumpEvent(0, BENIGN_RACE, tid, descr, a, 1);
}

static void On_AnnotateBenignRaceSized(THREADID tid, ADDRINT pc,
                                       ADDRINT file, ADDRINT line,
                                       ADDRINT a, ADDRINT size, ADDRINT descr) {
  DumpEvent(0, BENIGN_RACE, tid, descr, a, size);
}

static void On_AnnotateExpectRace(THREADID tid, ADDRINT pc,
                                  ADDRINT file, ADDRINT line,
                                  ADDRINT a, ADDRINT descr) {
  DumpEvent(0, EXPECT_RACE, tid, descr, a, 0);
}

static void On_AnnotateFlushExpectedRaces(THREADID tid, ADDRINT pc,
                                  ADDRINT file, ADDRINT line) {
  DumpEvent(0, FLUSH_EXPECTED_RACES, 0, 0, 0, 0);
}


static void On_AnnotateTraceMemory(THREADID tid, ADDRINT pc,
                                   ADDRINT file, ADDRINT line,
                                   ADDRINT a) {
  DumpEvent(0, TRACE_MEM, tid, pc, a, 0);
}

static void On_AnnotateNewMemory(THREADID tid, ADDRINT pc,
                                   ADDRINT file, ADDRINT line,
                                   ADDRINT a, ADDRINT size) {
  DumpEvent(0, MALLOC, tid, pc, a, size);
}

static void On_AnnotateNoOp(THREADID tid, ADDRINT pc,
                            ADDRINT file, ADDRINT line, ADDRINT a) {
  Printf("%s T%d: %s:%d %p\n", __FUNCTION__, tid, (char*)file, (int)line, a);
  //DumpEvent(0, STACK_TRACE, tid, pc, 0, 0);
//  PrintShadowStack(tid);
}

static void On_AnnotateFlushState(THREADID tid, ADDRINT pc,
                                  ADDRINT file, ADDRINT line) {
  DumpEvent(0, FLUSH_STATE, tid, pc, 0, 0);
}

static void On_AnnotateCondVarSignal(THREADID tid, ADDRINT pc,
                                     ADDRINT file, ADDRINT line, ADDRINT obj) {
  DumpEvent(0, SIGNAL, tid, pc, obj, 0);
}

static void On_AnnotateCondVarWait(THREADID tid, ADDRINT pc,
                                   ADDRINT file, ADDRINT line, ADDRINT obj) {
  DumpEvent(0, WAIT, tid, pc, obj, 0);
}

static void On_AnnotateHappensBefore(THREADID tid, ADDRINT pc,
                                     ADDRINT file, ADDRINT line, ADDRINT obj) {
  DumpEvent(0, SIGNAL, tid, pc, obj, 0);
}

static void On_AnnotateHappensAfter(THREADID tid, ADDRINT pc,
                                    ADDRINT file, ADDRINT line, ADDRINT obj) {
  DumpEvent(0, WAIT, tid, pc, obj, 0);
}

static void On_AnnotateEnableRaceDetection(THREADID tid, ADDRINT pc,
                                        ADDRINT file, ADDRINT line,
                                        ADDRINT enable) {
  if (!g_race_verifier_active)
    TLEBSimpleEvent(g_pin_threads[tid],
        enable ? TLEB_GLOBAL_IGNORE_OFF : TLEB_GLOBAL_IGNORE_ON);
}

static void On_AnnotateIgnoreReadsBegin(THREADID tid, ADDRINT pc,
                                        ADDRINT file, ADDRINT line) {
  DumpEvent(0, IGNORE_READS_BEG, tid, pc, 0, 0);
}
static void On_AnnotateIgnoreReadsEnd(THREADID tid, ADDRINT pc,
                                      ADDRINT file, ADDRINT line) {
  DumpEvent(0, IGNORE_READS_END, tid, pc, 0, 0);
}
static void On_AnnotateIgnoreWritesBegin(THREADID tid, ADDRINT pc,
                                         ADDRINT file, ADDRINT line) {
  DumpEvent(0, IGNORE_WRITES_BEG, tid, pc, 0, 0);
}
static void On_AnnotateIgnoreWritesEnd(THREADID tid, ADDRINT pc,
                                       ADDRINT file, ADDRINT line) {
  DumpEvent(0, IGNORE_WRITES_END, tid, pc, 0, 0);
}
static void On_AnnotateThreadName(THREADID tid, ADDRINT pc,
                                  ADDRINT file, ADDRINT line,
                                  ADDRINT name) {
  DumpEvent(0, SET_THREAD_NAME, tid, pc, name, 0);
}
static void On_AnnotatePublishMemoryRange(THREADID tid, ADDRINT pc,
                                          ADDRINT file, ADDRINT line,
                                          ADDRINT a, ADDRINT size) {
  DumpEvent(0, PUBLISH_RANGE, tid, pc, a, size);
}

static void On_AnnotateUnpublishMemoryRange(THREADID tid, ADDRINT pc,
                                          ADDRINT file, ADDRINT line,
                                          ADDRINT a, ADDRINT size) {
//  Printf("T%d %s %lx %lx\n", tid, __FUNCTION__, a, size);
  DumpEvent(0, UNPUBLISH_RANGE, tid, pc, a, size);
}


static void On_AnnotateMutexIsUsedAsCondVar(THREADID tid, ADDRINT pc,
                                            ADDRINT file, ADDRINT line,
                                            ADDRINT mu) {
  DumpEvent(0, HB_LOCK, tid, pc, mu, 0);
}

static void On_AnnotateMutexIsNotPhb(THREADID tid, ADDRINT pc,
                                     ADDRINT file, ADDRINT line,
                                     ADDRINT mu) {
  DumpEvent(0, NON_HB_LOCK, tid, pc, mu, 0);
}

static void On_AnnotatePCQCreate(THREADID tid, ADDRINT pc,
                                 ADDRINT file, ADDRINT line,
                                 ADDRINT pcq) {
  DumpEvent(0, PCQ_CREATE, tid, pc, pcq, 0);
}

static void On_AnnotatePCQDestroy(THREADID tid, ADDRINT pc,
                                  ADDRINT file, ADDRINT line,
                                  ADDRINT pcq) {
  DumpEvent(0, PCQ_DESTROY, tid, pc, pcq, 0);
}

static void On_AnnotatePCQPut(THREADID tid, ADDRINT pc,
                              ADDRINT file, ADDRINT line,
                              ADDRINT pcq) {
  DumpEvent(0, PCQ_PUT, tid, pc, pcq, 0);
}

static void On_AnnotatePCQGet(THREADID tid, ADDRINT pc,
                              ADDRINT file, ADDRINT line,
                              ADDRINT pcq) {
  DumpEvent(0, PCQ_GET, tid, pc, pcq, 0);
}

static void On_AnnotateRWLockCreate(THREADID tid, ADDRINT pc,
                                    ADDRINT file, ADDRINT line,
                                    ADDRINT lock) {
  DumpEvent(0, LOCK_CREATE, tid, pc, lock, 0);
}

static void On_AnnotateRWLockDestroy(THREADID tid, ADDRINT pc,
                                    ADDRINT file, ADDRINT line,
                                    ADDRINT lock) {
  DumpEvent(0, LOCK_DESTROY, tid, pc, lock, 0);
}

static void On_AnnotateRWLockAcquired(THREADID tid, ADDRINT pc,
                                     ADDRINT file, ADDRINT line,
                                     ADDRINT lock, ADDRINT is_w) {
  DumpEvent(0, is_w ? WRITER_LOCK : READER_LOCK, tid, pc, lock, 0);
}

static void On_AnnotateRWLockReleased(THREADID tid, ADDRINT pc,
                                     ADDRINT file, ADDRINT line,
                                     ADDRINT lock, ADDRINT is_w) {
  DumpEvent(0, UNLOCK, tid, pc, lock, 0);
}


int WRAP_NAME(RunningOnValgrind)(WRAP_PARAM4) {
  return 1;
}

//--------- Instrumentation ----------------------- {{{1
static bool IgnoreImage(IMG img) {
  string name = IMG_Name(img);
  if (name.find("/ld-") != string::npos)
    return true;
  return false;
}

static bool IgnoreRtn(RTN rtn) {
  CHECK(rtn != RTN_Invalid());
  ADDRINT rtn_address = RTN_Address(rtn);
  if (ThreadSanitizerWantToInstrumentSblock(rtn_address) == false)
    return true;
  return false;
}

static bool InstrumentCall(INS ins) {
  // Call.
  if (INS_IsProcedureCall(ins) && !INS_IsSyscall(ins)) {
    IGNORE_BELOW_RTN ignore_below = IGNORE_BELOW_RTN_UNKNOWN;
    if (INS_IsDirectBranchOrCall(ins)) {
      ADDRINT target = INS_DirectBranchOrCallTargetAddress(ins);
      bool ignore = ThreadSanitizerIgnoreAccessesBelowFunction(target);
      ignore_below = ignore ? IGNORE_BELOW_RTN_YES : IGNORE_BELOW_RTN_NO;
    }
    INS_InsertCall(ins, IPOINT_BEFORE,
                   (AFUNPTR)InsertBeforeEvent_Call,
                   IARG_THREAD_ID,
                   IARG_INST_PTR,
                   IARG_BRANCH_TARGET_ADDR,
                   IARG_REG_VALUE, REG_STACK_PTR,
                   IARG_ADDRINT, ignore_below,
                   IARG_END);
    return true;
  }
  if (INS_IsSyscall(ins)) {
    INS_InsertCall(ins, IPOINT_BEFORE,
                   (AFUNPTR)InsertBeforeEvent_SysCall,
                   IARG_THREAD_ID,
                   IARG_REG_VALUE, REG_STACK_PTR,
                   IARG_END);
  }
  return false;
}


// return the number of inserted instrumentations.
static void InstrumentMopsInBBl(BBL bbl, RTN rtn, TraceInfo *trace_info, uintptr_t instrument_pc, size_t *mop_idx) {
  // compute 'dtor_head', see
  // http://code.google.com/p/data-race-test/wiki/PopularDataRaces#Data_race_on_vptr
  // On x86_64 only the first BB of DTOR is treated as dtor_head.
  // On x86, we have to treat more BBs as dtor_head due to -fPIC.
  // See http://code.google.com/p/chromium/issues/detail?id=61199
  bool dtor_head = false;
#ifdef TARGET_IA32
  size_t max_offset_for_dtor_head = 32;
#else
  size_t max_offset_for_dtor_head = 0;
#endif

  if (BBL_Address(bbl) - RTN_Address(rtn) <= max_offset_for_dtor_head) {
    string demangled_rtn_name = Demangle(RTN_Name(rtn).c_str());
    if (demangled_rtn_name.find("::~") != string::npos)
      dtor_head = true;
  }

  INS tail = BBL_InsTail(bbl);
  // All memory reads/writes
  for( INS ins = BBL_InsHead(bbl);
       INS_Valid(ins);
       ins = INS_Next(ins) ) {
    if (ins != tail) {
      CHECK(!INS_IsRet(ins));
      CHECK(!INS_IsProcedureCall(ins));
    }
    // bool is_stack = INS_IsStackRead(ins) || INS_IsStackWrite(ins);
    if (INS_IsAtomicUpdate(ins)) continue;

    int n_mops = INS_MemoryOperandCount(ins);
    if (n_mops == 0) continue;

    string opcode_str = OPCODE_StringShort(INS_Opcode(ins));
    if (trace_info && debug_ins) {
      Printf("  INS: opcode=%s n_mops=%d dis=\"%s\"\n",
             opcode_str.c_str(),  n_mops,
             INS_Disassemble(ins).c_str());
    }

    bool ins_ignore_writes = false;
    bool ins_ignore_reads = false;

    // CALL writes to stack and (if the call is indirect) reads the target
    // address. We don't want to handle the stack write.
    if (INS_IsCall(ins)) {
      CHECK(n_mops == 1 || n_mops == 2);
      ins_ignore_writes = true;
    }

    // PUSH: we ignore the write to stack but we don't ignore the read (if any).
    if (opcode_str == "PUSH") {
      CHECK(n_mops == 1 || n_mops == 2);
      ins_ignore_writes = true;
    }

    // POP: we are reading from stack, Ignore it.
    if (opcode_str == "POP") {
      CHECK(n_mops == 1 || n_mops == 2);
      ins_ignore_reads = true;
      continue;
    }

    // RET/LEAVE -- ignore it, it just reads the return address and stack.
    if (INS_IsRet(ins) || opcode_str == "LEAVE") {
      CHECK(n_mops == 1);
      continue;
    }

    bool is_predicated = INS_IsPredicated(ins);
    for (int i = 0; i < n_mops; i++) {
      if (*mop_idx >= kMaxMopsPerTrace) {
        Report("INFO: too many mops in trace: %d %s\n",
            INS_Address(ins), PcToRtnName(INS_Address(ins), true).c_str());
        return;
      }
      size_t size = INS_MemoryOperandSize(ins, i);
      CHECK(size);
      bool is_write = INS_MemoryOperandIsWritten(ins, i);

      if (ins_ignore_writes && is_write) continue;
      if (ins_ignore_reads && !is_write) continue;
      if (instrument_pc && instrument_pc != INS_Address(ins)) continue;

      bool check_ident_store = false;
      if (dtor_head && is_write && INS_IsMov(ins) && size == sizeof(void*)) {
        // This is a special case for '*addr = value', where we want to ignore the
        // access if *addr == value before the store.
        CHECK(!is_predicated);
        check_ident_store = true;
      }

      if (trace_info) {
        if (debug_ins) {
          Printf("    size=%ld is_w=%d\n", size, (int)is_write);
        }
        IPOINT point = IPOINT_BEFORE;
        AFUNPTR on_mop_callback = (AFUNPTR)OnMop;
        if (check_ident_store) {
          INS_InsertCall(ins, IPOINT_BEFORE,
            (AFUNPTR)OnMopCheckIdentStoreBefore,
            IARG_REG_VALUE, tls_reg,
            IARG_THREAD_ID,
            IARG_ADDRINT, *mop_idx,
            IARG_MEMORYOP_EA, i,
            IARG_END);
          // This is just a MOV, so we can insert the instrumentation code
          // after the insn.
          point = IPOINT_AFTER;
          on_mop_callback = (AFUNPTR)OnMopCheckIdentStoreAfter;
        }

        MopInfo *mop = trace_info->GetMop(*mop_idx);
        new (mop) MopInfo(INS_Address(ins), size, is_write, false);
        if (is_predicated) {
          INS_InsertPredicatedCall(ins, point,
              (AFUNPTR)On_PredicatedMop,
              IARG_EXECUTING,
              IARG_REG_VALUE, tls_reg,
              IARG_THREAD_ID,
              IARG_ADDRINT, *mop_idx,
              IARG_MEMORYOP_EA, i,
              IARG_END);
        } else {
          INS_InsertCall(ins, point,
              on_mop_callback,
              IARG_REG_VALUE, tls_reg,
              IARG_THREAD_ID,
              IARG_ADDRINT, *mop_idx,
              IARG_MEMORYOP_EA, i,
              IARG_END);
        }
      }
      (*mop_idx)++;
    }
  }
}

void CallbackForTRACE(TRACE trace, void *v) {
  CHECK(n_started_threads > 0);

  RTN rtn = TRACE_Rtn(trace);
  bool ignore_memory = false;
  string img_name = "<>";
  string rtn_name = "<>";
  if (RTN_Valid(rtn)) {
    SEC sec = RTN_Sec(rtn);
    IMG img = SEC_Img(sec);
    rtn_name = RTN_Name(rtn);
    img_name = IMG_Name(img);

    if (IgnoreImage(img)) {
      // Printf("Ignoring memory accesses in %s\n", IMG_Name(img).c_str());
      ignore_memory = true;
    } else if (IgnoreRtn(rtn)) {
      ignore_memory = true;
    }
  }

  uintptr_t instrument_pc = 0;
  if (g_race_verifier_active) {
    // Check if this trace looks like part of a possible race report.
    uintptr_t min_pc = UINTPTR_MAX;
    uintptr_t max_pc = 0;
    for(BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {
      min_pc = MIN(min_pc, INS_Address(BBL_InsHead(bbl)));
      max_pc = MAX(max_pc, INS_Address(BBL_InsTail(bbl)));
    }

    bool verify_trace = RaceVerifierGetAddresses(min_pc, max_pc, &instrument_pc);
    if (!verify_trace)
      ignore_memory = true;
  }

  size_t n_mops = 0;
  // count the mops.
  for(BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {
    if (!ignore_memory) {
      InstrumentMopsInBBl(bbl, rtn, NULL, instrument_pc, &n_mops);
    }
    INS tail = BBL_InsTail(bbl);
    if (INS_IsRet(tail)) {
#if 0
      INS_InsertIfCall(tail, IPOINT_BEFORE,
                       (AFUNPTR)Before_RET_IF,
                       IARG_THREAD_ID,
                       IARG_END);

      INS_InsertThenCall(
#else
        INS_InsertCall(
#endif
          tail, IPOINT_BEFORE,
          (AFUNPTR)Before_RET_THEN,
          IARG_THREAD_ID,
          IARG_INST_PTR,
          IARG_REG_VALUE, REG_STACK_PTR,
          IARG_FUNCRET_EXITPOINT_VALUE,
          IARG_END);
    }
  }

  // Handle the head of the trace
  INS head = BBL_InsHead(TRACE_BblHead(trace));
  CHECK(n_mops <= kMaxMopsPerTrace);

  TraceInfo *trace_info = NULL;
  if (n_mops) {
    trace_info = TraceInfo::NewTraceInfo(n_mops, INS_Address(head));
    if (TS_SERIALIZED == 0) {
      // TODO(kcc): implement race verifier here.
      INS_InsertCall(head, IPOINT_BEFORE,
                     (AFUNPTR)OnTraceParallel,
                     IARG_REG_VALUE, tls_reg,
                     IARG_REG_VALUE, REG_STACK_PTR,
                     IARG_PTR, trace_info,
                     IARG_END);
    } else {
      AFUNPTR handler = (AFUNPTR)(g_race_verifier_active ?
                                  OnTraceVerify : OnTraceSerial);
      INS_InsertCall(head, IPOINT_BEFORE,
                     handler,
                     IARG_THREAD_ID,
                     IARG_REG_VALUE, REG_STACK_PTR,
                     IARG_PTR, trace_info,
                     IARG_REG_REFERENCE, tls_reg,
                     IARG_END);
    }
  } else {
    if (g_race_verifier_active) {
      INS_InsertCall(head, IPOINT_BEFORE,
                     (AFUNPTR)OnTraceNoMopsVerify,
                     IARG_THREAD_ID,
                     IARG_REG_VALUE, REG_STACK_PTR,
                     IARG_REG_REFERENCE, tls_reg,
                     IARG_END);
    }
  }

  // instrument the mops. We want to do it after we instrumented the head
  // to maintain the right order of instrumentation callbacks (head first, then
  // mops).
  size_t i = 0;
  if (n_mops) {
    if (debug_ins) {
      Printf("TRACE %p (%p); n_mops=%ld %s\n", trace_info,
             TRACE_Address(trace),
             trace_info->n_mops(),
             PcToRtnName(trace_info->pc(), false).c_str());
    }
    for(BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {
      InstrumentMopsInBBl(bbl, rtn, trace_info, instrument_pc, &i);
    }
  }

  // instrument the calls, do it after all other instrumentation.
  if (!g_race_verifier_active) {
    for(BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {
      InstrumentCall(BBL_InsTail(bbl));
    }
  }

  CHECK(n_mops == i);
}


#define INSERT_FN_HELPER(point, name, rtn, to_insert, ...) \
    RTN_Open(rtn); \
    if (G_flags->verbosity >= 2) Printf("RTN: Inserting %-50s (%s) %s (%s) img: %s\n", \
    #to_insert, #point, RTN_Name(rtn).c_str(), name, IMG_Name(img).c_str());\
    RTN_InsertCall(rtn, point, (AFUNPTR)to_insert, IARG_THREAD_ID, \
                   IARG_INST_PTR, __VA_ARGS__, IARG_END);\
    RTN_Close(rtn); \

#define INSERT_FN(point, name, to_insert, ...) \
  while (RtnMatchesName(rtn_name, name)) {\
    INSERT_FN_HELPER(point, name, rtn, to_insert, __VA_ARGS__); \
    break;\
  }\


#define INSERT_BEFORE_FN(name, to_insert, ...) \
    INSERT_FN(IPOINT_BEFORE, name, to_insert, __VA_ARGS__)

#define INSERT_BEFORE_1_SP(name, to_insert) \
    INSERT_BEFORE_FN(name, to_insert, \
                     IARG_REG_VALUE, REG_STACK_PTR, \
                     IARG_FUNCARG_ENTRYPOINT_VALUE, 0)

#define INSERT_BEFORE_2_SP(name, to_insert) \
    INSERT_BEFORE_FN(name, to_insert, \
                     IARG_REG_VALUE, REG_STACK_PTR, \
                     IARG_FUNCARG_ENTRYPOINT_VALUE, 0, \
                     IARG_FUNCARG_ENTRYPOINT_VALUE, 1)

#define INSERT_BEFORE_0(name, to_insert) \
    INSERT_BEFORE_FN(name, to_insert, IARG_END);

#define INSERT_BEFORE_1(name, to_insert) \
    INSERT_BEFORE_FN(name, to_insert, \
                     IARG_FUNCARG_ENTRYPOINT_VALUE, 0)

#define INSERT_BEFORE_2(name, to_insert) \
    INSERT_BEFORE_FN(name, to_insert, \
                     IARG_FUNCARG_ENTRYPOINT_VALUE, 0, \
                     IARG_FUNCARG_ENTRYPOINT_VALUE, 1)

#define INSERT_BEFORE_3(name, to_insert) \
    INSERT_BEFORE_FN(name, to_insert, \
                     IARG_FUNCARG_ENTRYPOINT_VALUE, 0, \
                     IARG_FUNCARG_ENTRYPOINT_VALUE, 1, \
                     IARG_FUNCARG_ENTRYPOINT_VALUE, 2)

#define INSERT_BEFORE_4(name, to_insert) \
    INSERT_BEFORE_FN(name, to_insert, \
                     IARG_FUNCARG_ENTRYPOINT_VALUE, 0, \
                     IARG_FUNCARG_ENTRYPOINT_VALUE, 1, \
                     IARG_FUNCARG_ENTRYPOINT_VALUE, 2, \
                     IARG_FUNCARG_ENTRYPOINT_VALUE, 3)

#define INSERT_BEFORE_5(name, to_insert) \
    INSERT_BEFORE_FN(name, to_insert, \
                     IARG_FUNCARG_ENTRYPOINT_VALUE, 0, \
                     IARG_FUNCARG_ENTRYPOINT_VALUE, 1, \
                     IARG_FUNCARG_ENTRYPOINT_VALUE, 2, \
                     IARG_FUNCARG_ENTRYPOINT_VALUE, 3, \
                     IARG_FUNCARG_ENTRYPOINT_VALUE, 4)

#define INSERT_BEFORE_6(name, to_insert) \
    INSERT_BEFORE_FN(name, to_insert, \
                     IARG_FUNCARG_ENTRYPOINT_VALUE, 0, \
                     IARG_FUNCARG_ENTRYPOINT_VALUE, 1, \
                     IARG_FUNCARG_ENTRYPOINT_VALUE, 2, \
                     IARG_FUNCARG_ENTRYPOINT_VALUE, 3, \
                     IARG_FUNCARG_ENTRYPOINT_VALUE, 4, \
                     IARG_FUNCARG_ENTRYPOINT_VALUE, 5)

#define INSERT_AFTER_FN(name, to_insert, ...) \
    INSERT_FN(IPOINT_AFTER, name, to_insert, __VA_ARGS__)

#define INSERT_AFTER_0(name, to_insert) \
    INSERT_AFTER_FN(name, to_insert, IARG_END)

#define INSERT_AFTER_1(name, to_insert) \
    INSERT_AFTER_FN(name, to_insert, IARG_FUNCRET_EXITPOINT_VALUE)


#ifdef _MSC_VER
void WrapStdCallFunc1(RTN rtn, char *name, AFUNPTR replacement_func) {
  if (RTN_Valid(rtn) && RtnMatchesName(RTN_Name(rtn), name)) {
    InformAboutFunctionWrap(rtn, name);
    PROTO proto = PROTO_Allocate(PIN_PARG(uintptr_t),
                                 CALLINGSTD_STDCALL,
                                 "proto",
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG_END());
    RTN_ReplaceSignature(rtn,
                         AFUNPTR(replacement_func),
                         IARG_PROTOTYPE, proto,
                         IARG_THREAD_ID,
                         IARG_INST_PTR,
                         IARG_CONTEXT,
                         IARG_ORIG_FUNCPTR,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                         IARG_END);
    PROTO_Free(proto);
  }
}

void WrapStdCallFunc2(RTN rtn, char *name, AFUNPTR replacement_func) {
  if (RTN_Valid(rtn) && RtnMatchesName(RTN_Name(rtn), name)) {
    InformAboutFunctionWrap(rtn, name);
    PROTO proto = PROTO_Allocate(PIN_PARG(uintptr_t),
                                 CALLINGSTD_STDCALL,
                                 "proto",
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG_END());
    RTN_ReplaceSignature(rtn,
                         AFUNPTR(replacement_func),
                         IARG_PROTOTYPE, proto,
                         IARG_THREAD_ID,
                         IARG_INST_PTR,
                         IARG_CONTEXT,
                         IARG_ORIG_FUNCPTR,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                         IARG_END);
    PROTO_Free(proto);
  }
}

void WrapStdCallFunc3(RTN rtn, char *name, AFUNPTR replacement_func) {
  if (RTN_Valid(rtn) && RtnMatchesName(RTN_Name(rtn), name)) {
    InformAboutFunctionWrap(rtn, name);
    PROTO proto = PROTO_Allocate(PIN_PARG(uintptr_t),
                                 CALLINGSTD_STDCALL,
                                 "proto",
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG_END());
    RTN_ReplaceSignature(rtn,
                         AFUNPTR(replacement_func),
                         IARG_PROTOTYPE, proto,
                         IARG_THREAD_ID,
                         IARG_INST_PTR,
                         IARG_CONTEXT,
                         IARG_ORIG_FUNCPTR,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
                         IARG_END);
    PROTO_Free(proto);
  }
}

void WrapStdCallFunc4(RTN rtn, char *name, AFUNPTR replacement_func) {
  if (RTN_Valid(rtn) && RtnMatchesName(RTN_Name(rtn), name)) {
    InformAboutFunctionWrap(rtn, name);
    PROTO proto = PROTO_Allocate(PIN_PARG(uintptr_t),
                                 CALLINGSTD_STDCALL,
                                 "proto",
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG_END());
    RTN_ReplaceSignature(rtn,
                         AFUNPTR(replacement_func),
                         IARG_PROTOTYPE, proto,
                         IARG_THREAD_ID,
                         IARG_INST_PTR,
                         IARG_CONTEXT,
                         IARG_ORIG_FUNCPTR,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 3,
                         IARG_END);
    PROTO_Free(proto);
  }
}

void WrapStdCallFunc5(RTN rtn, char *name, AFUNPTR replacement_func) {
  if (RTN_Valid(rtn) && RtnMatchesName(RTN_Name(rtn), name)) {
    InformAboutFunctionWrap(rtn, name);
    PROTO proto = PROTO_Allocate(PIN_PARG(uintptr_t),
                                 CALLINGSTD_STDCALL,
                                 "proto",
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG_END());
    RTN_ReplaceSignature(rtn,
                         AFUNPTR(replacement_func),
                         IARG_PROTOTYPE, proto,
                         IARG_THREAD_ID,
                         IARG_INST_PTR,
                         IARG_CONTEXT,
                         IARG_ORIG_FUNCPTR,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 3,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 4,
                         IARG_END);
    PROTO_Free(proto);
  }
}

void WrapStdCallFunc6(RTN rtn, char *name, AFUNPTR replacement_func) {
  if (RTN_Valid(rtn) && RtnMatchesName(RTN_Name(rtn), name)) {
    InformAboutFunctionWrap(rtn, name);
    PROTO proto = PROTO_Allocate(PIN_PARG(uintptr_t),
                                 CALLINGSTD_STDCALL,
                                 "proto",
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG_END());
    RTN_ReplaceSignature(rtn,
                         AFUNPTR(replacement_func),
                         IARG_PROTOTYPE, proto,
                         IARG_THREAD_ID,
                         IARG_INST_PTR,
                         IARG_CONTEXT,
                         IARG_ORIG_FUNCPTR,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 3,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 4,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 5,
                         IARG_END);
    PROTO_Free(proto);
  }
}

void WrapStdCallFunc7(RTN rtn, char *name, AFUNPTR replacement_func) {
  if (RTN_Valid(rtn) && RtnMatchesName(RTN_Name(rtn), name)) {
    InformAboutFunctionWrap(rtn, name);
    PROTO proto = PROTO_Allocate(PIN_PARG(uintptr_t),
                                 CALLINGSTD_STDCALL,
                                 "proto",
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG_END());
    RTN_ReplaceSignature(rtn,
                         AFUNPTR(replacement_func),
                         IARG_PROTOTYPE, proto,
                         IARG_THREAD_ID,
                         IARG_INST_PTR,
                         IARG_CONTEXT,
                         IARG_ORIG_FUNCPTR,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 3,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 4,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 5,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 6,
                         IARG_END);
    PROTO_Free(proto);
  }
}

void WrapStdCallFunc8(RTN rtn, char *name, AFUNPTR replacement_func) {
  if (RTN_Valid(rtn) && RtnMatchesName(RTN_Name(rtn), name)) {
    InformAboutFunctionWrap(rtn, name);
    PROTO proto = PROTO_Allocate(PIN_PARG(uintptr_t),
                                 CALLINGSTD_STDCALL,
                                 "proto",
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG_END());
    RTN_ReplaceSignature(rtn,
                         AFUNPTR(replacement_func),
                         IARG_PROTOTYPE, proto,
                         IARG_THREAD_ID,
                         IARG_INST_PTR,
                         IARG_CONTEXT,
                         IARG_ORIG_FUNCPTR,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 3,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 4,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 5,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 6,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 7,
                         IARG_END);
    PROTO_Free(proto);
  }
}

void WrapStdCallFunc10(RTN rtn, char *name, AFUNPTR replacement_func) {
  if (RTN_Valid(rtn) && RtnMatchesName(RTN_Name(rtn), name)) {
    InformAboutFunctionWrap(rtn, name);
    PROTO proto = PROTO_Allocate(PIN_PARG(uintptr_t),
                                 CALLINGSTD_STDCALL,
                                 "proto",
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG_END());
    RTN_ReplaceSignature(rtn,
                         AFUNPTR(replacement_func),
                         IARG_PROTOTYPE, proto,
                         IARG_THREAD_ID,
                         IARG_INST_PTR,
                         IARG_CONTEXT,
                         IARG_ORIG_FUNCPTR,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 3,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 4,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 5,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 6,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 7,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 8,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 9,
                         IARG_END);
    PROTO_Free(proto);
  }
}

void WrapStdCallFunc11(RTN rtn, char *name, AFUNPTR replacement_func) {
  if (RTN_Valid(rtn) && RtnMatchesName(RTN_Name(rtn), name)) {
    InformAboutFunctionWrap(rtn, name);
    PROTO proto = PROTO_Allocate(PIN_PARG(uintptr_t),
                                 CALLINGSTD_STDCALL,
                                 "proto",
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG(uintptr_t),
                                 PIN_PARG_END());
    RTN_ReplaceSignature(rtn,
                         AFUNPTR(replacement_func),
                         IARG_PROTOTYPE, proto,
                         IARG_THREAD_ID,
                         IARG_INST_PTR,
                         IARG_CONTEXT,
                         IARG_ORIG_FUNCPTR,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 3,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 4,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 5,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 6,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 7,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 8,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 9,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 10,
                         IARG_END);
    PROTO_Free(proto);
  }
}

#endif

static void MaybeInstrumentOneRoutine(IMG img, RTN rtn) {
  if (IgnoreImage(img)) {
    return;
  }
  string rtn_name = RTN_Name(rtn);
  string img_name = IMG_Name(img);
  if (debug_wrap) {
    Printf("%s: %s %s pc=%p\n", __FUNCTION__, rtn_name.c_str(),
           img_name.c_str(), RTN_Address(rtn));
  }

  // malloc/free/etc
  const char *malloc_names[] = {
    "malloc",
#if defined(__GNUC__)
    "_Znwm",
    "_Znam",
    "_Znwj",
    "_Znaj",
    "_ZnwmRKSt9nothrow_t",
    "_ZnamRKSt9nothrow_t",
    "_ZnwjRKSt9nothrow_t",
    "_ZnajRKSt9nothrow_t",
#endif
#if defined(_MSC_VER)
    "operator new",
    "operator new[]",
#endif  // _MSC_VER
  };

  const char *free_names[] = {
    "free",
#if defined(__GNUC__)
    "_ZdaPv",
    "_ZdlPv",
    "_ZdlPvRKSt9nothrow_t",
    "_ZdaPvRKSt9nothrow_t",
#endif  // __GNUC__
#if defined(_MSC_VER)
    "operator delete",
    "operator delete[]",
#endif  // _MSC_VER
  };

  for (size_t i = 0; i < TS_ARRAY_SIZE(malloc_names); i++) {
    const char *name = malloc_names[i];
    INSERT_BEFORE_1_SP(name, Before_malloc);
  }

  for (size_t i = 0; i < TS_ARRAY_SIZE(free_names); i++) {
    const char *name = free_names[i];
    INSERT_BEFORE_1_SP(name, Before_free);
  }

  INSERT_BEFORE_2_SP("calloc", Before_calloc);
  INSERT_BEFORE_2_SP("realloc", Before_realloc);

#if defined(__GNUC__)
  WrapFunc6(img, rtn, "mmap", (AFUNPTR)WRAP_NAME(mmap));
  WrapFunc4(img, rtn, "munmap", (AFUNPTR)WRAP_NAME(munmap));

  WrapFunc4(img, rtn, "lockf", (AFUNPTR)WRAP_NAME(lockf));
  // pthread create/join
  WrapFunc4(img, rtn, "pthread_create", (AFUNPTR)WRAP_NAME(pthread_create));
  WrapFunc4(img, rtn, "pthread_join", (AFUNPTR)WRAP_NAME(pthread_join));
  WrapFunc4(img, rtn, "fwrite", (AFUNPTR)WRAP_NAME(fwrite));

  INSERT_FN(IPOINT_BEFORE, "start_thread",
            Before_start_thread,
            IARG_REG_VALUE, REG_STACK_PTR, IARG_END);

   // pthread_cond_*
  INSERT_BEFORE_1("pthread_cond_signal", Before_pthread_cond_signal);
  WRAP4(pthread_cond_wait);
  WRAP4(pthread_cond_timedwait);

  // pthread_mutex_*
  INSERT_BEFORE_1("pthread_mutex_init", Before_pthread_mutex_init);
  INSERT_BEFORE_1("pthread_mutex_destroy", Before_pthread_mutex_destroy);
  INSERT_BEFORE_1("pthread_mutex_unlock", Before_pthread_unlock);


  INSERT_BEFORE_1_SP("pthread_mutex_lock", Before_pthread_mutex_lock);
  WRAP4(pthread_mutex_trylock);
  WRAP4(pthread_spin_lock);
  WRAP4(pthread_spin_trylock);
  WRAP4(pthread_spin_init);
  WRAP4(pthread_spin_destroy);
  WRAP4(pthread_spin_unlock);
  WRAP4(pthread_rwlock_wrlock);
  WRAP4(pthread_rwlock_rdlock);
  WRAP4(pthread_rwlock_trywrlock);
  WRAP4(pthread_rwlock_tryrdlock);

  // pthread_rwlock_*
  INSERT_BEFORE_1("pthread_rwlock_init", Before_pthread_rwlock_init);
  INSERT_BEFORE_1("pthread_rwlock_destroy", Before_pthread_rwlock_destroy);
  INSERT_BEFORE_1("pthread_rwlock_unlock", Before_pthread_unlock);

  // pthread_barrier_*
  WrapFunc4(img, rtn, "pthread_barrier_init",
            (AFUNPTR)WRAP_NAME(pthread_barrier_init));
  WrapFunc4(img, rtn, "pthread_barrier_wait",
            (AFUNPTR)WRAP_NAME(pthread_barrier_wait));

  // pthread_once
  WrapFunc4(img, rtn, "pthread_once", (AFUNPTR)WRAP_NAME(pthread_once));

  // sem_*
  INSERT_AFTER_1("sem_open", After_sem_open);
  INSERT_BEFORE_1("sem_post", Before_sem_post);
  WRAP4(sem_wait);
  WRAP4(sem_trywait);

  INSERT_BEFORE_0("epoll_ctl", Before_epoll_ctl);
  INSERT_AFTER_0("epoll_wait", After_epoll_wait);
#endif  // __GNUC__

#ifdef _MSC_VER
  WrapStdCallFunc6(rtn, "CreateThread", (AFUNPTR)WRAP_NAME(CreateThread));
  WRAPSTD1(ResumeThread);

  INSERT_FN(IPOINT_BEFORE, "BaseThreadInitThunk",
            Before_BaseThreadInitThunk,
            IARG_REG_VALUE, REG_STACK_PTR, IARG_END);

  INSERT_BEFORE_0("RtlExitUserThread", Before_RtlExitUserThread);
  INSERT_BEFORE_0("ExitThread", Before_RtlExitUserThread);

  WRAPSTD1(RtlInitializeCriticalSection);
  WRAPSTD2(RtlInitializeCriticalSectionAndSpinCount);
  WRAPSTD3(RtlInitializeCriticalSectionEx);
  WRAPSTD1(RtlDeleteCriticalSection);
  WRAPSTD1(RtlEnterCriticalSection);
  WRAPSTD1(RtlTryEnterCriticalSection);
  WRAPSTD1(RtlLeaveCriticalSection);
  WRAPSTD7(DuplicateHandle);
  WRAPSTD1(SetEvent);
  WRAPSTD4(CreateSemaphoreA);
  WRAPSTD4(CreateSemaphoreW);
  WRAPSTD3(ReleaseSemaphore);

  WRAPSTD1(RtlInterlockedPopEntrySList);
  WRAPSTD2(RtlInterlockedPushEntrySList);

#if 1
  WRAPSTD1(RtlAcquireSRWLockExclusive);
  WRAPSTD1(RtlAcquireSRWLockShared);
  WRAPSTD1(RtlTryAcquireSRWLockExclusive);
  WRAPSTD1(RtlTryAcquireSRWLockShared);
  WRAPSTD1(RtlReleaseSRWLockExclusive);
  WRAPSTD1(RtlReleaseSRWLockShared);
  WRAPSTD1(RtlInitializeSRWLock);
  // For some reason, RtlInitializeSRWLock is aliased to RtlInitializeSRWLock..
  WrapStdCallFunc1(rtn, "RtlRunOnceInitialize",
                   (AFUNPTR)Wrap_RtlInitializeSRWLock);

  /* We haven't seen these syscalls used in the wild yet.
  WRAPSTD2(RtlUpdateClonedSRWLock);
  WRAPSTD1(RtlAcquireReleaseSRWLockExclusive);
  WRAPSTD1(RtlUpdateClonedCriticalSection);
  */

  WRAPSTD1(RtlWakeConditionVariable);
  WRAPSTD1(RtlWakeAllConditionVariable);
  WRAPSTD4(RtlSleepConditionVariableSRW);
  WRAPSTD3(RtlSleepConditionVariableCS);
#endif  // if 1

  WRAPSTD3(RtlQueueWorkItem);
  WRAPSTD6(RegisterWaitForSingleObject);
  WRAPSTD2(UnregisterWaitEx);

  WRAPSTD3(WaitForSingleObjectEx);
  WRAPSTD5(WaitForMultipleObjectsEx);

  WrapStdCallFunc4(rtn, "VirtualAlloc", (AFUNPTR)(WRAP_NAME(VirtualAlloc)));
  WrapStdCallFunc6(rtn, "ZwAllocateVirtualMemory", (AFUNPTR)(WRAP_NAME(ZwAllocateVirtualMemory)));
  WrapStdCallFunc2(rtn, "GlobalAlloc", (AFUNPTR)WRAP_NAME(GlobalAlloc));
//  WrapStdCallFunc3(rtn, "RtlAllocateHeap", (AFUNPTR) WRAP_NAME(AllocateHeap));
//  WrapStdCallFunc3(rtn, "HeapCreate", (AFUNPTR) WRAP_NAME(HeapCreate));
#endif  // _MSC_VER

  // Annotations.
  INSERT_BEFORE_4("AnnotateBenignRace", On_AnnotateBenignRace);
  INSERT_BEFORE_5("AnnotateBenignRaceSized", On_AnnotateBenignRaceSized);
  INSERT_BEFORE_5("WTFAnnotateBenignRaceSized", On_AnnotateBenignRaceSized);
  INSERT_BEFORE_4("AnnotateExpectRace", On_AnnotateExpectRace);
  INSERT_BEFORE_2("AnnotateFlushExpectedRaces", On_AnnotateFlushExpectedRaces);
  INSERT_BEFORE_3("AnnotateTraceMemory", On_AnnotateTraceMemory);
  INSERT_BEFORE_4("AnnotateNewMemory", On_AnnotateNewMemory);
  INSERT_BEFORE_3("AnnotateNoOp", On_AnnotateNoOp);
  INSERT_BEFORE_2("AnnotateFlushState", On_AnnotateFlushState);

  INSERT_BEFORE_3("AnnotateCondVarWait", On_AnnotateCondVarWait);
  INSERT_BEFORE_3("AnnotateCondVarSignal", On_AnnotateCondVarSignal);
  INSERT_BEFORE_3("AnnotateCondVarSignalAll", On_AnnotateCondVarSignal);
  INSERT_BEFORE_3("AnnotateHappensBefore", On_AnnotateHappensBefore);
  INSERT_BEFORE_3("WTFAnnotateHappensBefore", On_AnnotateHappensBefore);
  INSERT_BEFORE_3("AnnotateHappensAfter", On_AnnotateHappensAfter);
  INSERT_BEFORE_3("WTFAnnotateHappensAfter", On_AnnotateHappensAfter);

  INSERT_BEFORE_3("AnnotateEnableRaceDetection", On_AnnotateEnableRaceDetection);
  INSERT_BEFORE_0("AnnotateIgnoreReadsBegin", On_AnnotateIgnoreReadsBegin);
  INSERT_BEFORE_0("AnnotateIgnoreReadsEnd", On_AnnotateIgnoreReadsEnd);
  INSERT_BEFORE_0("AnnotateIgnoreWritesBegin", On_AnnotateIgnoreWritesBegin);
  INSERT_BEFORE_0("AnnotateIgnoreWritesEnd", On_AnnotateIgnoreWritesEnd);
  INSERT_BEFORE_3("AnnotateThreadName", On_AnnotateThreadName);
  INSERT_BEFORE_4("AnnotatePublishMemoryRange", On_AnnotatePublishMemoryRange);
  INSERT_BEFORE_4("AnnotateUnpublishMemoryRange", On_AnnotateUnpublishMemoryRange);
  INSERT_BEFORE_3("AnnotateMutexIsUsedAsCondVar", On_AnnotateMutexIsUsedAsCondVar);
  INSERT_BEFORE_3("AnnotateMutexIsNotPHB", On_AnnotateMutexIsNotPhb);

  INSERT_BEFORE_3("AnnotatePCQCreate", On_AnnotatePCQCreate);
  INSERT_BEFORE_3("AnnotatePCQDestroy", On_AnnotatePCQDestroy);
  INSERT_BEFORE_3("AnnotatePCQPut", On_AnnotatePCQPut);
  INSERT_BEFORE_3("AnnotatePCQGet", On_AnnotatePCQGet);

  INSERT_BEFORE_3("AnnotateRWLockCreate", On_AnnotateRWLockCreate);
  INSERT_BEFORE_3("AnnotateRWLockDestroy", On_AnnotateRWLockDestroy);
  INSERT_BEFORE_4("AnnotateRWLockAcquired", On_AnnotateRWLockAcquired);
  INSERT_BEFORE_4("AnnotateRWLockReleased", On_AnnotateRWLockReleased);

  // ThreadSanitizerQuery
  WrapFunc4(img, rtn, "ThreadSanitizerQuery",
            (AFUNPTR)WRAP_NAME(ThreadSanitizerQuery));
  WrapFunc4(img, rtn, "RunningOnValgrind",
            (AFUNPTR)WRAP_NAME(RunningOnValgrind));

  // I/O
  INSERT_BEFORE_0("write", Before_SignallingIOCall);
  INSERT_BEFORE_0("unlink", Before_SignallingIOCall);
  INSERT_BEFORE_0("rmdir", Before_SignallingIOCall);
//  INSERT_BEFORE_0("send", Before_SignallingIOCall);
  INSERT_AFTER_0("__read_nocancel", After_WaitingIOCall);
  INSERT_AFTER_0("fopen", After_WaitingIOCall);
  INSERT_AFTER_0("__fopen_internal", After_WaitingIOCall);
  INSERT_AFTER_0("open", After_WaitingIOCall);
  INSERT_AFTER_0("opendir", After_WaitingIOCall);
//  INSERT_AFTER_0("recv", After_WaitingIOCall);

  // strlen and friends.
  // These wrappers will generate memory access events.
  // So, if we don't want to get those events (e.g. memcpy inside
  // ld.so or ntdll.dll) we don't wrap them and the regular
  // ignore machinery will make sure we don't get the events.
  if (ThreadSanitizerWantToInstrumentSblock(RTN_Address(rtn))) {
    ReplaceFunc3(img, rtn, "memchr", (AFUNPTR)Replace_memchr);
    ReplaceFunc3(img, rtn, "strchr", (AFUNPTR)Replace_strchr);
    ReplaceFunc3(img, rtn, "index", (AFUNPTR)Replace_strchr);
    ReplaceFunc3(img, rtn, "strchrnul", (AFUNPTR)Replace_strchrnul);
    ReplaceFunc3(img, rtn, "strrchr", (AFUNPTR)Replace_strrchr);
    ReplaceFunc3(img, rtn, "rindex", (AFUNPTR)Replace_strrchr);
    ReplaceFunc3(img, rtn, "strlen", (AFUNPTR)Replace_strlen);
    ReplaceFunc3(img, rtn, "strcmp", (AFUNPTR)Replace_strcmp);
    ReplaceFunc3(img, rtn, "strncmp", (AFUNPTR)Replace_strncmp);
    ReplaceFunc3(img, rtn, "memcpy", (AFUNPTR)Replace_memcpy);
    ReplaceFunc3(img, rtn, "memcmp", (AFUNPTR)Replace_memcmp);
    ReplaceFunc3(img, rtn, "memmove", (AFUNPTR)Replace_memmove);
    ReplaceFunc3(img, rtn, "strcpy", (AFUNPTR)Replace_strcpy);
    ReplaceFunc3(img, rtn, "strncpy", (AFUNPTR)Replace_strncpy);
    ReplaceFunc3(img, rtn, "strcat", (AFUNPTR)Replace_strcat);
    ReplaceFunc3(img, rtn, "stpcpy", (AFUNPTR)Replace_stpcpy);
  }

  // __cxa_guard_acquire / __cxa_guard_release
  INSERT_BEFORE_1("__cxa_guard_acquire", Before_cxa_guard_acquire);
  INSERT_AFTER_1("__cxa_guard_acquire", After_cxa_guard_acquire);
  INSERT_AFTER_0("__cxa_guard_release", After_cxa_guard_release);

  INSERT_BEFORE_0("atexit", On_atexit);
  INSERT_BEFORE_0("exit", On_exit);
}

// Pin calls this function every time a new img is loaded.
static void CallbackForIMG(IMG img, void *v) {
  if (debug_wrap) {
    Printf("Started CallbackForIMG %s\n", IMG_Name(img).c_str());
  }

  string img_name = IMG_Name(img);
  for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec)) {
    for (RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn)) {
      MaybeInstrumentOneRoutine(img, rtn);
    }
  }
  // In DEBUG_MODE check that we have the debug symbols in the Windows guts.
  // We should work w/o them too.
  // TODO(timurrrr): I doubt the problem is the missing symbols.
  // I have a strong gut feeling that this syscall was added
  // in Vista but only used since Windows 7. We had its wrapper wrong
  // (found on W7) but the Vista build was fine for months.
  // Also, we wrap RtlReleaseSRWLock*, so our TSan assertions would have been
  // broken if RtlTryAcquireSRWLock* wasn't wrapped - and we haven't see this.
  if (DEBUG_MODE && img_name.find("ntdll.dll") != string::npos) {
    if (g_wrapped_functions.count("RtlTryAcquireSRWLockExclusive") == 0) {
      Printf("WARNING: Debug symbols for ntdll.dll not found.\n");
    }
  }
}

// Returns:
// TRUE
// If user is interested to inject Pin (and tool) into child/exec-ed process
// FALSE
// If user is not interested to inject Pin (and tool) into child/exec-ed process
static BOOL CallbackForExec(CHILD_PROCESS childProcess, VOID *val) {
  int argc = 0;
  const CHAR *const * argv = NULL;
  CHILD_PROCESS_GetCommandLine(childProcess, &argc, &argv);
  CHECK(argc > 0);
  CHECK(argv);
  bool follow = G_flags->trace_children;
  if (DEBUG_MODE) {
    Printf("CallbackForExec: follow=%d: ", follow);
    for (int i = 0; i < argc; i++) {
      Printf("%s ", argv[i]);
    }
  }
  Printf("\n");
  return follow;
}

//--------- Fini ---------- {{{1
static void CallbackForFini(INT32 code, void *v) {
  DumpEvent(0, THR_END, 0, 0, 0, 0);
  ThreadSanitizerFini();
  if (g_race_verifier_active) {
    RaceVerifierFini();
  }
  if (G_flags->show_stats) {
    TraceInfo::PrintTraceProfile();
  }
  if (G_flags->error_exitcode && GetNumberOfFoundErrors() > 0) {
    exit(G_flags->error_exitcode);
  }
}

//--------- Call Coverage ----------------- {{{1
// A simplistic call coverage tool.
// Outputs all pairs <call_site,call_target>.

typedef set<pair<uintptr_t, uintptr_t> > CallCoverageSet;
static CallCoverageSet *call_coverage_set;

static map<uintptr_t, string> *function_names_map;
static uintptr_t symbolized_functions_cache[1023];
static pair<uintptr_t, uintptr_t> registered_pairs_cache[1023];

static void symbolize_pc(uintptr_t pc) {
  // Check a simple cache if we already symbolized this pc (racey).
  size_t idx = pc % TS_ARRAY_SIZE(symbolized_functions_cache);
  if (symbolized_functions_cache[idx] == pc) return;

  ScopedReentrantClientLock lock(__LINE__);
  CHECK(function_names_map);
  if (function_names_map->count(pc) == 0) {
    (*function_names_map)[pc] = PcToRtnName(pc, false);
  }
  symbolized_functions_cache[idx] = pc;
}

static void CallCoverageRegisterCall(uintptr_t from, uintptr_t to) {
  symbolize_pc(from);
  symbolize_pc(to);

  // Check if we already registered this pair (racey).
  size_t idx = (from ^ to) % TS_ARRAY_SIZE(registered_pairs_cache);
  if (registered_pairs_cache[idx] == make_pair(from,to)) return;

  ScopedReentrantClientLock lock(__LINE__);
  call_coverage_set->insert(make_pair(from, to));
  registered_pairs_cache[idx] = make_pair(from,to);
}

static void CallCoverageCallbackForTRACE(TRACE trace, void *v) {
  RTN rtn = TRACE_Rtn(trace);
  if (RTN_Valid(rtn)) {
    SEC sec = RTN_Sec(rtn);
    IMG img = SEC_Img(sec);
    string img_name = IMG_Name(img);
    // Don't instrument system libraries.
    if (img_name.find("/usr/") == 0) return;
  }

  if (call_coverage_set == NULL) {
    call_coverage_set = new CallCoverageSet;
    function_names_map = new map<uintptr_t, string>;
  }
  for(BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {
    INS ins = BBL_InsTail(bbl);
    if (!INS_IsProcedureCall(ins) || INS_IsSyscall(ins)) continue;
    if (INS_IsDirectBranchOrCall(ins)) {
      // If <from, to> is know at instrumentation time, don't instrument.
      ADDRINT to = INS_DirectBranchOrCallTargetAddress(ins);
      ADDRINT from = INS_Address(ins);
      CallCoverageRegisterCall(from, to);
    } else {
      // target is dynamic. Need to instrument.
      INS_InsertCall(ins, IPOINT_BEFORE,
                     (AFUNPTR)CallCoverageRegisterCall,
                     IARG_INST_PTR,
                     IARG_BRANCH_TARGET_ADDR,
                     IARG_END);
    }
  }
}

// Output all <from,to> pairs.
static void CallCoverageCallbackForFini(INT32 code, void *v) {
  CHECK(call_coverage_set);
  CHECK(function_names_map);
  for (CallCoverageSet::iterator it = call_coverage_set->begin();
       it != call_coverage_set->end(); ++it) {
    string from_name = (*function_names_map)[it->first];
    string to_name   = (*function_names_map)[it->second];
    if (to_name == ".plt" || to_name == "") continue;
    Printf("CallCoverage: %s => %s\n", from_name.c_str(), to_name.c_str());
  }
}

//--------- Main -------------------------- {{{1
int main(INT32 argc, CHAR **argv) {
  PIN_Init(argc, argv);
  PIN_InitSymbols();
  G_out = stderr;

  // Init ThreadSanitizer.
  int first_param = 1;
  // skip until '-t something.so'.
  for (; first_param < argc && argv[first_param] != string("-t");
       first_param++) {
  }
  first_param += 2;
  vector<string> args;
  for (; first_param < argc; first_param++) {
    string param = argv[first_param];
    if (param == "--") break;
    if (param == "-short_name") continue;
    if (param == "-slow_asserts") continue;
    if (param == "1") continue;
    args.push_back(param);
  }

  G_flags = new FLAGS;
  ThreadSanitizerParseFlags(&args);

  if (G_flags->dry_run >= 2) {
    PIN_StartProgram();
    return 0;
  }

  FILE *socket_output = OpenSocketForWriting(G_flags->log_file);
  if (socket_output) {
    G_out = socket_output;
  } else if (!G_flags->log_file.empty()) {
    // Replace %p with tool PID
    string fname = G_flags->log_file;
    char pid_str[100] = "";
    sprintf(pid_str, "%u", getpid());
    while (fname.find("%p") != fname.npos)
      fname.replace(fname.find("%p"), 2, pid_str);

    G_out = fopen(fname.c_str(), "w");
    CHECK(G_out);
  }

  ThreadSanitizerInit();

  if (G_flags->call_coverage) {
    PIN_AddFiniFunction(CallCoverageCallbackForFini, 0);
    TRACE_AddInstrumentFunction(CallCoverageCallbackForTRACE, 0);
    PIN_StartProgram();
    return 0;
  }

  tls_reg = PIN_ClaimToolRegister();
  CHECK(REG_valid(tls_reg));
#if _MSC_VER
  g_windows_thread_pool_calback_set = new unordered_set<uintptr_t>;
  g_windows_thread_pool_wait_object_map = new unordered_map<uintptr_t, uintptr_t>;
#endif

  // Set up PIN callbacks.
  PIN_AddThreadStartFunction(CallbackForThreadStart, 0);
  PIN_AddThreadFiniFunction(CallbackForThreadFini, 0);
  PIN_AddFiniFunction(CallbackForFini, 0);
  IMG_AddInstrumentFunction(CallbackForIMG, 0);
  TRACE_AddInstrumentFunction(CallbackForTRACE, 0);
  PIN_AddFollowChildProcessFunction(CallbackForExec, NULL);

  Report("ThreadSanitizerPin r%s pin %d: %s\n",
         TS_VERSION, PIN_BUILD_NUMBER,
         G_flags->pure_happens_before ? "hybrid=no" : "hybrid=yes");
  if (DEBUG_MODE) {
    Report("INFO: Debug build\n");
  }

  if (g_race_verifier_active) {
    RaceVerifierInit(G_flags->race_verifier, G_flags->race_verifier_extra);
    global_ignore = true;
  }

  // Fire!
  PIN_StartProgram();
  return 0;
}

//--------- Questions about PIN -------------------------- {{{1
/* Questions about PIN:

  - Names (e.g. pthread_create@... __pthread_mutex_unlock)
  - How to get name of a global var by it's address?
  - How to get stack pointer at thread creation?
  - How to get a stack trace (other than intercepting calls, entries, exits)
  - assert with full stack trace?
  */
// end. {{{1
// vim:shiftwidth=2:softtabstop=2:expandtab
