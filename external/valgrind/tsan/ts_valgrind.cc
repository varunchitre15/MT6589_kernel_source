/*
  This file is part of ThreadSanitizer, a dynamic data race detector
  based on Valgrind.

  Copyright (C) 2008-2010 Google Inc
     opensource@google.com
  Copyright (C) 2007-2008 OpenWorks LLP
      info@open-works.co.uk

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
  02111-1307, USA.

  The GNU General Public License is contained in the file COPYING.
*/

// Author: Konstantin Serebryany.
// Parts of the code in this file are derived from Helgrind,
// a data race detector written by Julian Seward.
// Note that the rest of ThreadSanitizer code is not derived from Helgrind
// and is published under the BSD license.

#include "ts_valgrind.h"
#include "valgrind.h"
#include "ts_valgrind_client_requests.h"
#include "thread_sanitizer.h"
#include "ts_trace_info.h"
#include "ts_race_verifier.h"
#include "common_util.h"

#include "coregrind/pub_core_basics.h"
#include "coregrind/pub_core_machine.h"
#include "coregrind/pub_core_clreq.h"
#include "pub_tool_libcsetjmp.h"
#include "coregrind/pub_core_threadstate.h"
#include "pub_tool_libcproc.h"


//---------------------- C++ malloc support -------------- {{{1
void *operator new (size_t size) {
  return VG_(malloc)((HChar*)g_malloc_stack.Top(), size);
}
void *operator new [](size_t size) {
  return VG_(malloc)((HChar*)g_malloc_stack.Top(), size);
}
void operator delete (void *p) {
  VG_(free)(p);
}
void operator delete [](void *p) {
  VG_(free)(p);
}

extern "C" void *malloc(size_t size) {
  return VG_(malloc)((HChar*)g_malloc_stack.Top(), size);
}

extern "C" void free(void *ptr) {
  VG_(free)(ptr);
}

extern "C" void* realloc(void *ptr, size_t size) {
  return VG_(realloc)((HChar*)g_malloc_stack.Top(), ptr, size);
}


//---------------------- Utils ------------------- {{{1

extern "C" int puts(const char *s) {
  Printf("%s", s);
  return 1;
}

extern "C" void exit(int e) { VG_(exit)(e); }

#ifdef VGO_darwin
extern "C" void abort() { CHECK(0); }
#endif


// TODO: make this rtn public
extern "C" {
  Bool VG_(get_fnname_no_cxx_demangle) ( Addr a, Char* buf, Int nbuf );
}


const int kBuffSize = 1024 * 10 - 1;
// not thread-safe.
static char g_buff1[kBuffSize+1];
static char g_buff2[kBuffSize+1];

string PcToRtnName(uintptr_t pc, bool demangle) {
  if (demangle) {
    if(VG_(get_fnname)(pc, (Char*)g_buff1, kBuffSize)) {
      return g_buff1;
    }
  } else {
    if(VG_(get_fnname_no_cxx_demangle)(pc, (Char*)g_buff1, kBuffSize)) {
      return g_buff1;
    }
  }
  return "(no symbols)";
}

void PcToStrings(uintptr_t pc, bool demangle,
                string *img_name, string *rtn_name,
                string *file_name, int *line_no) {
  const int kBuffSize = 1024 * 10 - 1;
  Bool has_dirname = False;

  if (VG_(get_filename_linenum)
      (pc, (Char*)g_buff1, kBuffSize, (Char*)g_buff2, kBuffSize,
       &has_dirname, (UInt*)line_no) &&
      has_dirname) {
    *file_name = string(g_buff2) + "/" + g_buff1;
  } else {
    VG_(get_linenum)(pc, (UInt *)line_no);
    if (VG_(get_filename)(pc, (Char*)g_buff1, kBuffSize)) {
      *file_name = g_buff1;
    }
  }
  *file_name = ConvertToPlatformIndependentPath(*file_name);

  *rtn_name = PcToRtnName(pc, demangle);

  if (VG_(get_objname)(pc, (Char*)g_buff1, kBuffSize)) {
    *img_name = g_buff1;
  }
}



string Demangle(const char *str) {
  return str;
}

extern "C"
size_t strlen(const char *s) {
  return VG_(strlen)((const Char*)s);
}

static inline ThreadId GetVgTid() {
  extern ThreadId VG_(running_tid); // HACK: avoid calling get_running_tid()
  ThreadId res = VG_(running_tid);
  //DCHECK(res == VG_(get_running_tid)());
  return res;
}

static inline uintptr_t GetVgPc(ThreadId vg_tid) {
  Addr pc = VG_(threads)[vg_tid].arch.vex.VG_INSTR_PTR;
  DCHECK(pc == VG_(get_IP)(vg_tid));
  return pc;
  //return (uintptr_t)VG_(get_IP)(vg_tid);
}

static inline uintptr_t GetVgSp(ThreadId vg_tid) {
  Addr sp = VG_(threads)[vg_tid].arch.vex.VG_STACK_PTR;
  DCHECK(sp == VG_(get_SP)(vg_tid));
  return sp;
}

#ifdef VGP_arm_linux
static inline uintptr_t GetVgLr(ThreadId vg_tid) {
  return (uintptr_t)VG_(threads)[vg_tid].arch.vex.guest_R14;
}
#endif

static uintptr_t g_current_pc;

uintptr_t GetPcOfCurrentThread() {
  return g_current_pc;
}

void GetThreadStack(int tid, uintptr_t *min_addr, uintptr_t *max_addr) {
  // tid is not used because we call it from the current thread anyway.
  uintptr_t stack_max  = VG_(thread_get_stack_max)(GetVgTid());
  uintptr_t stack_size = VG_(thread_get_stack_size)(GetVgTid());
  uintptr_t stack_min  = stack_max - stack_size;
  *min_addr = stack_min;
  *max_addr = stack_max;
}

struct CallStackRecord {
  Addr pc;
  Addr sp;
#ifdef VGP_arm_linux
  // We need to store LR in order to keep the shadow stack consistent.
  Addr lr;
#endif
};

const size_t kMaxMopsPerTrace = 2048;

struct ValgrindThread {
  int32_t zero_based_uniq_tid;
  TSanThread *ts_thread;
  uint32_t literace_sampling;
  vector<CallStackRecord> call_stack;

  int ignore_accesses;
  int ignore_sync;
  int in_signal_handler;

  // thread-local event buffer (tleb).
  uintptr_t tleb[kMaxMopsPerTrace];
  TraceInfo *trace_info;

  // PC (as in trace_info->pc()) of the trace currently being verified.
  // 0 if outside of the verification sleep loop.
  // -1 in the last iteration of the loop.
  uintptr_t verifier_current_pc;

  // End time of the current verification loop.
  unsigned verifier_wakeup_time_ms;

  ValgrindThread() {
    Clear();
  }

  void Clear() {
    ts_thread = NULL;
    zero_based_uniq_tid = -1;
    literace_sampling = G_flags->literace_sampling;  // cache it.
    ignore_accesses = 0;
    ignore_sync = 0;
    in_signal_handler = 0;
    call_stack.clear();
    trace_info = NULL;
    verifier_current_pc = 0;
    verifier_wakeup_time_ms = 0;
  }
};

// If true, ignore all accesses in all threads.
extern bool global_ignore;

// Array of VG_N_THREADS
static ValgrindThread *g_valgrind_threads = 0;
static map<uintptr_t, int> *g_ptid_to_ts_tid;

// maintains a uniq thread id (first thread will have id=0)
static int32_t g_uniq_thread_id_counter = 0;

static int32_t VgTidToTsTid(ThreadId vg_tid) {
  DCHECK(vg_tid < VG_N_THREADS);
  DCHECK(vg_tid >= 1);
  DCHECK(g_valgrind_threads);
  DCHECK(g_valgrind_threads[vg_tid].zero_based_uniq_tid >= 0);
  return g_valgrind_threads[vg_tid].zero_based_uniq_tid;
}

static vector<string> *g_command_line_options = 0;
static void InitCommandLineOptions() {
  if(G_flags == NULL) {
    G_flags = new FLAGS;
  }
  if (g_command_line_options == NULL) {
    g_command_line_options = new vector<string>;
  }
}

Bool ts_process_cmd_line_option (Char* arg) {
  InitCommandLineOptions();
  g_command_line_options->push_back((char*)arg);
  return True;
}

void ts_print_usage (void) {
  InitCommandLineOptions();
  ThreadSanitizerParseFlags(g_command_line_options);

  ThreadSanitizerPrintUsage();
}

void ts_print_debug_usage(void) {
  ThreadSanitizerPrintUsage();
}

extern int VG_(clo_error_exitcode);

void ts_post_clo_init(void) {
  ScopedMallocCostCenter malloc_cc(__FUNCTION__);
  InitCommandLineOptions();
  ThreadSanitizerParseFlags(g_command_line_options);

  // we get num-callers from valgrind flags.
  G_flags->num_callers = VG_(clo_backtrace_size);
  if (!G_flags->error_exitcode)
    G_flags->error_exitcode = VG_(clo_error_exitcode);

  extern Int   VG_(clo_n_suppressions);
  extern Int   VG_(clo_gen_suppressions);
  extern Char* VG_(clo_suppressions)[];
  extern Int   VG_(clo_n_fullpath_after);
  extern Char* VG_(clo_fullpath_after)[];
  // get the suppressions from Valgrind
  for (int i = 0; i < VG_(clo_n_suppressions); i++) {
    G_flags->suppressions.push_back((char*)VG_(clo_suppressions)[i]);
  }
  // get the --fullpath-after prefixes from Valgrind and treat them as
  // --file-prefix-to-cut arguments.
  for (int i = 0; i < VG_(clo_n_fullpath_after); i++) {
    G_flags->file_prefix_to_cut.push_back((char*)VG_(clo_fullpath_after)[i]);
  }
  G_flags->generate_suppressions |= VG_(clo_gen_suppressions) >= 1;

  if (G_flags->html) {
    Report("<pre>\n"
           "<br id=race0>"
           "<a href=\"#race1\">Go to first race report</a>\n");
  }
  Report("ThreadSanitizerValgrind r%s: %s\n",
         TS_VERSION,
         G_flags->pure_happens_before ? "hybrid=no" : "hybrid=yes");
  if (DEBUG_MODE) {
    Report("INFO: Debug build\n");
  }
  if (G_flags->max_mem_in_mb) {
    Report("INFO: ThreadSanitizer memory limit: %dMB\n",
           (int)G_flags->max_mem_in_mb);
  }
  ThreadSanitizerInit();

  g_valgrind_threads = new ValgrindThread[VG_N_THREADS];
  g_ptid_to_ts_tid = new map<uintptr_t, int>;

  if (g_race_verifier_active) {
    RaceVerifierInit(G_flags->race_verifier, G_flags->race_verifier_extra);
    global_ignore = true;
  }
}

// Remember, valgrind is essentially single-threaded.
// Each time we switch to another thread, we set the global g_cur_tleb
// to the tleb of the current thread. This allows to load the tleb in one
// instruction.
static uintptr_t *g_cur_tleb;
static void OnStartClientCode(ThreadId vg_tid, ULong nDisp) {
  ValgrindThread *thr = &g_valgrind_threads[vg_tid];
  g_cur_tleb = thr->tleb;
}

INLINE void FlushMops(ValgrindThread *thr, bool keep_trace_info = false) {
  DCHECK(!g_race_verifier_active || global_ignore);
  TraceInfo *t = thr->trace_info;
  if (!t) return;
  if (!keep_trace_info) {
    thr->trace_info = NULL;
  }

  if (global_ignore || thr->ignore_accesses ||
       (thr->literace_sampling &&
        t->LiteRaceSkipTraceRealTid(thr->zero_based_uniq_tid, thr->literace_sampling))) {
    thr->trace_info = NULL;
    return;
  }

  size_t n = t->n_mops();
  DCHECK(n > 0);
  uintptr_t *tleb = thr->tleb;
  DCHECK(thr->ts_thread);
  ThreadSanitizerHandleTrace(thr->ts_thread, t, tleb);
}

static void ShowCallStack(ValgrindThread *thr) {
  size_t n = thr->call_stack.size();
  Printf("        ");
  for (size_t i = n - 1; i > n - 10 && i >= 0; i--) {
    Printf("{pc=%p sp=%p}, ", thr->call_stack[i].pc, thr->call_stack[i].sp);
  }
  Printf("\n");
}

static INLINE void UpdateCallStack(ValgrindThread *thr, uintptr_t sp) {
  DCHECK(!g_race_verifier_active);
  if (thr->trace_info) FlushMops(thr, true /* keep_trace_info */);
  vector<CallStackRecord> &call_stack = thr->call_stack;
  while (!call_stack.empty()) {
    CallStackRecord &record = call_stack.back();
    Addr cur_top = record.sp;
    if (sp < cur_top) break;
    call_stack.pop_back();
    int32_t ts_tid = thr->zero_based_uniq_tid;
    ThreadSanitizerHandleRtnExit(ts_tid);
    if (debug_rtn) {
      Printf("T%d: [%ld]<< pc=%p sp=%p cur_sp=%p %s\n",
             ts_tid, thr->call_stack.size(), record.pc,
             record.sp, sp,
             PcToRtnNameAndFilePos(record.pc).c_str());
      ShowCallStack(thr);
    }
  }
}

VG_REGPARM(1)
static void OnTrace(TraceInfo *trace_info) {
  DCHECK(!g_race_verifier_active);
  //trace_info->counter()++;
  if (global_ignore) return;
  ThreadId vg_tid = GetVgTid();
  ValgrindThread *thr = &g_valgrind_threads[vg_tid];

  // First, flush the old trace_info.
  if (thr->trace_info) {
    FlushMops(thr);
  }

  UpdateCallStack(thr, GetVgSp(vg_tid));

  // Start the new trace, zero the contents of tleb.
  size_t n = trace_info->n_mops();
  uintptr_t *tleb = thr->tleb;
  for (size_t i = 0; i < n; i++)
    tleb[i] = 0;
  thr->trace_info = trace_info;
  DCHECK(thr->trace_info);
  DCHECK(thr->trace_info->n_mops() <= kMaxMopsPerTrace);
}

static inline void Put(EventType type, int32_t tid, uintptr_t pc,
                       uintptr_t a, uintptr_t info) {
  if (DEBUG_MODE && G_flags->dry_run >= 1) return;
  Event event(type, tid, pc, a, info);
  ThreadSanitizerHandleOneEvent(&event);
}

static void rtn_call(Addr sp_post_call_insn, Addr pc_post_call_insn,
                     IGNORE_BELOW_RTN ignore_below) {
  DCHECK(!g_race_verifier_active);
  if (global_ignore) return;
  ThreadId vg_tid = GetVgTid();
  ValgrindThread *thr = &g_valgrind_threads[vg_tid];
  int ts_tid = thr->zero_based_uniq_tid;
  CallStackRecord record;
  record.pc = pc_post_call_insn;
  record.sp = sp_post_call_insn + 4;  // sp before call.
  UpdateCallStack(thr, record.sp);
#ifdef VGP_arm_linux
  record.lr = GetVgLr(vg_tid);
#endif
  thr->call_stack.push_back(record);
  // If the shadow stack grows too high this usually means it is not cleaned
  // properly. Or this may be a very deep recursion.
  DCHECK(thr->call_stack.size() < 10000);
  uintptr_t call_pc = GetVgPc(vg_tid);
  if (thr->trace_info) FlushMops(thr);
  ThreadSanitizerHandleRtnCall(ts_tid, call_pc, record.pc,
                               ignore_below);

  if (debug_rtn) {
    Printf("T%d: [%ld]>> pc=%p sp=%p %s\n",
           ts_tid, thr->call_stack.size(), (void*)record.pc,
           (void*)record.sp,
           PcToRtnNameAndFilePos(record.pc).c_str());
    ShowCallStack(thr);
  }
}

VG_REGPARM(2) void evh__rtn_call_ignore_unknown ( Addr sp, Addr pc) {
  rtn_call(sp, pc, IGNORE_BELOW_RTN_UNKNOWN);
}
VG_REGPARM(2) void evh__rtn_call_ignore_yes ( Addr sp, Addr pc) {
  rtn_call(sp, pc, IGNORE_BELOW_RTN_YES);
}
VG_REGPARM(2) void evh__rtn_call_ignore_no ( Addr sp, Addr pc) {
  rtn_call(sp, pc, IGNORE_BELOW_RTN_NO);
}

#ifdef VGP_arm_linux
// Handle shadow stack frame deletion on ARM.
// Instrumented code calls this function for each non-call jump out of
// a superblock. If the |sp_post_call_insn| (the jump target address) is equal
// to a link register value of one or more frames on top of the shadow stack,
// those frames are popped out.
// TODO(glider): there may be problems with optimized recursive functions that
// don't change PC, SP and LR.
VG_REGPARM(2)
void evh__delete_frame ( Addr sp_post_call_insn,
                         Addr pc_post_call_insn) {
  DCHECK(!g_race_verifier_active);
  ThreadId vg_tid = GetVgTid();
  ValgrindThread *thr = &g_valgrind_threads[vg_tid];
  if (thr->trace_info) FlushMops(thr);
  vector<CallStackRecord> &call_stack = thr->call_stack;
  int32_t ts_tid = VgTidToTsTid(vg_tid);
  while (!call_stack.empty()) {
    CallStackRecord &record = call_stack.back();
    if (record.lr != pc_post_call_insn) break;
    call_stack.pop_back();
    ThreadSanitizerHandleRtnExit(ts_tid);
  }
}
#endif

void ts_fini(Int exitcode) {
  ThreadSanitizerFini();
  if (g_race_verifier_active) {
    RaceVerifierFini();
  }
  if (G_flags->error_exitcode && GetNumberOfFoundErrors() > 0) {
    exit(G_flags->error_exitcode);
  }
}


void evh__pre_thread_ll_create ( ThreadId parent, ThreadId child ) {
  tl_assert(parent != child);
  ValgrindThread *thr = &g_valgrind_threads[child];
  //  Printf("thread_create: %d->%d\n", parent, child);
  if (thr->zero_based_uniq_tid != -1) {
    Printf("ThreadSanitizer WARNING: reusing TID %d w/o exiting thread\n",
           child);
  }
  thr->Clear();
  thr->zero_based_uniq_tid = g_uniq_thread_id_counter++;
  // Printf("VG: T%d: VG_THR_START: parent=%d\n", VgTidToTsTid(child), VgTidToTsTid(parent));
  Put(THR_START, VgTidToTsTid(child), 0, 0,
      parent > 0 ? VgTidToTsTid(parent) : 0);
  thr->ts_thread = ThreadSanitizerGetThreadByTid(thr->zero_based_uniq_tid);
  CHECK(thr->ts_thread);
}

void evh__pre_workq_task_start(ThreadId vg_tid, Addr workitem) {
  uintptr_t pc = GetVgPc(vg_tid);
  int32_t ts_tid = VgTidToTsTid(vg_tid);
  ValgrindThread *thr = &g_valgrind_threads[vg_tid];
  FlushMops(thr);
  Put(WAIT, ts_tid, pc, workitem, 0);
}

void evh__pre_thread_first_insn(const ThreadId vg_tid) {
  ValgrindThread *thr = &g_valgrind_threads[vg_tid];
  FlushMops(thr);
  Put(THR_FIRST_INSN, VgTidToTsTid(vg_tid), GetVgPc(vg_tid), 0, 0);
}


void evh__pre_thread_ll_exit ( ThreadId quit_tid ) {
//  Printf("thread_exit: %d\n", quit_tid);
//  Printf("T%d quiting thread; stack size=%ld\n",
//         VgTidToTsTid(quit_tid),
//         (int)g_valgrind_threads[quit_tid].call_stack.size());
  ValgrindThread *thr = &g_valgrind_threads[quit_tid];
  FlushMops(thr);
  Put(THR_END, VgTidToTsTid(quit_tid), 0, 0, 0);
  g_valgrind_threads[quit_tid].zero_based_uniq_tid = -1;
}

  extern "C" void VG_(show_all_errors)();

// Whether we are currently ignoring sync events for the given thread at the
// given address.
static inline Bool ignoring_sync(ThreadId vg_tid, uintptr_t addr) {
  // We ignore locking events if ignore_sync != 0 and if we are not
  // inside a signal handler.
  return (g_valgrind_threads[vg_tid].ignore_sync &&
          !g_valgrind_threads[vg_tid].in_signal_handler) ||
      ThreadSanitizerIgnoreForNacl(addr);
}

Bool ts_handle_client_request(ThreadId vg_tid, UWord* args, UWord* ret) {
  if (args[0] == VG_USERREQ__NACL_MEM_START) {
    // This will get truncated on x86-32, but we don't support it with NaCl
    // anyway.
    const uintptr_t kFourGig = (uintptr_t)0x100000000ULL;
    uintptr_t mem_start = args[1];
    uintptr_t mem_end = mem_start + kFourGig;
    ThreadSanitizerNaclUntrustedRegion(mem_start, mem_end);
    return True;
  }
  if (!VG_IS_TOOL_USERREQ('T', 'S', args[0]))
    return False;
  int32_t ts_tid = VgTidToTsTid(vg_tid);
  // Ignore almost everything in race verifier mode.
  if (g_race_verifier_active) {
    if (args[0] == TSREQ_EXPECT_RACE) {
      Put(EXPECT_RACE, ts_tid, /*descr=*/args[2],
          /*p=*/args[1], 0);
    }
    *ret = 0;
    return True;
  }
  ValgrindThread *thr = &g_valgrind_threads[vg_tid];
  if (thr->trace_info) FlushMops(thr);
  UpdateCallStack(thr, GetVgSp(vg_tid));
  *ret = 0;
  uintptr_t pc = GetVgPc(vg_tid);
  switch (args[0]) {
    case TSREQ_SET_MY_PTHREAD_T:
      (*g_ptid_to_ts_tid)[args[1]] = ts_tid;
      break;
    case TSREQ_THR_STACK_TOP:
      Put(THR_STACK_TOP, ts_tid, pc, args[1], 0);
      break;
    case TSREQ_PTHREAD_JOIN_POST:
      Put(THR_JOIN_AFTER, ts_tid, pc, (*g_ptid_to_ts_tid)[args[1]], 0);
      break;
    case TSREQ_CLEAN_MEMORY:
      Put(MALLOC, ts_tid, pc, /*ptr=*/args[1], /*size=*/args[2]);
      break;
    case TSREQ_MAIN_IN:
      g_has_entered_main = true;
      // Report("INFO: Entred main(); argc=%d\n", (int)args[1]);
      break;
    case TSREQ_MAIN_OUT:
      g_has_exited_main = true;
      if (G_flags->exit_after_main) {
        Report("INFO: Exited main(); ret=%d\n", (int)args[1]);
        VG_(show_all_errors)();
        ThreadSanitizerFini();
        if (g_race_verifier_active) {
          RaceVerifierFini();
        }
        exit((int)args[1]);
      }
      break;
    case TSREQ_MALLOC:
      // Printf("Malloc: %p %ld\n", args[1], args[2]);
      Put(MALLOC, ts_tid, pc, /*ptr=*/args[1], /*size=*/args[2]);
      break;
    case TSREQ_FREE:
      // Printf("Free: %p\n", args[1]);
      Put(FREE, ts_tid, pc, /*ptr=*/args[1], 0);
      break;
    case TSREQ_MMAP:
      Put(MMAP, ts_tid, pc, /*ptr=*/args[1], /*size=*/args[2]);
      break;
    case TSREQ_MUNMAP:
      Put(MUNMAP, ts_tid, pc, /*ptr=*/args[1], /*size=*/args[2]);
      break;
    case TSREQ_BENIGN_RACE:
      Put(BENIGN_RACE, ts_tid, /*descr=*/args[3],
          /*p=*/args[1], /*size=*/args[2]);
      break;
    case TSREQ_EXPECT_RACE:
      Put(EXPECT_RACE, ts_tid, /*descr=*/args[2], /*p=*/args[1], 0);
      break;
    case TSREQ_FLUSH_EXPECTED_RACES:
      Put(FLUSH_EXPECTED_RACES, ts_tid, 0, 0, 0);
      break;
    case TSREQ_PCQ_CREATE:
      Put(PCQ_CREATE, ts_tid, pc, /*pcq=*/args[1], 0);
      break;
    case TSREQ_PCQ_DESTROY:
      Put(PCQ_DESTROY, ts_tid, pc, /*pcq=*/args[1], 0);
      break;
    case TSREQ_PCQ_PUT:
      Put(PCQ_PUT, ts_tid, pc, /*pcq=*/args[1], 0);
      break;
    case TSREQ_PCQ_GET:
      Put(PCQ_GET, ts_tid, pc, /*pcq=*/args[1], 0);
      break;
    case TSREQ_TRACE_MEM:
      Put(TRACE_MEM, ts_tid, pc, /*mem=*/args[1], 0);
      break;
    case TSREQ_MUTEX_IS_USED_AS_CONDVAR:
      Put(HB_LOCK, ts_tid, pc, /*lock=*/args[1], 0);
      break;
    case TSREQ_MUTEX_IS_NOT_PHB:
      Put(NON_HB_LOCK, ts_tid, pc, /*lock=*/args[1], 0);
      break;
    case TSREQ_GLOBAL_IGNORE_ON:
      Report("INFO: GLOBAL IGNORE ON\n");
      global_ignore = true;
      break;
    case TSREQ_GLOBAL_IGNORE_OFF:
      Report("INFO: GLOBAL IGNORE OFF\n");
      global_ignore = false;
      break;
    case TSREQ_IGNORE_READS_BEGIN:
      Put(IGNORE_READS_BEG, ts_tid, pc, 0, 0);
      break;
    case TSREQ_IGNORE_READS_END:
      Put(IGNORE_READS_END, ts_tid, pc, 0, 0);
      break;
    case TSREQ_IGNORE_WRITES_BEGIN:
      Put(IGNORE_WRITES_BEG, ts_tid, pc, 0, 0);
      break;
    case TSREQ_IGNORE_WRITES_END:
      Put(IGNORE_WRITES_END, ts_tid, pc, 0, 0);
      break;
    case TSREQ_SET_THREAD_NAME:
      Put(SET_THREAD_NAME, ts_tid, pc, /*name=*/args[1], 0);
      break;
    case TSREQ_SET_STACKTOP_STACKSIZE:
      Put(THR_STACK_TOP, ts_tid, pc, /*addr=*/args[1], /*size=*/args[2]);
      break;
    case TSREQ_IGNORE_ALL_ACCESSES_BEGIN:
      g_valgrind_threads[vg_tid].ignore_accesses++;
      break;
    case TSREQ_IGNORE_ALL_ACCESSES_END:
      g_valgrind_threads[vg_tid].ignore_accesses--;
      CHECK(g_valgrind_threads[vg_tid].ignore_accesses >= 0);
      break;
    case TSREQ_IGNORE_ALL_SYNC_BEGIN:
      g_valgrind_threads[vg_tid].ignore_sync++;
      break;
    case TSREQ_IGNORE_ALL_SYNC_END:
      g_valgrind_threads[vg_tid].ignore_sync--;
      CHECK(g_valgrind_threads[vg_tid].ignore_sync >= 0);
      break;
    case TSREQ_PUBLISH_MEMORY_RANGE:
      Put(PUBLISH_RANGE, ts_tid, pc, /*mem=*/args[1], /*size=*/args[2]);
      break;
    case TSREQ_UNPUBLISH_MEMORY_RANGE:
      Put(UNPUBLISH_RANGE, ts_tid, pc, /*mem=*/args[1], /*size=*/args[2]);
      break;
    case TSREQ_PRINT_MEMORY_USAGE:
    case TSREQ_PRINT_STATS:
    case TSREQ_RESET_STATS:
    case TSREQ_PTH_API_ERROR:
      break;
    case TSREQ_PTHREAD_RWLOCK_CREATE_POST:
      if (ignoring_sync(vg_tid, args[1]))
        break;
      Put(LOCK_CREATE, ts_tid, pc, /*lock=*/args[1], 0);
      break;
    case TSREQ_PTHREAD_RWLOCK_DESTROY_PRE:
      if (ignoring_sync(vg_tid, args[1]))
        break;
      Put(LOCK_DESTROY, ts_tid, pc, /*lock=*/args[1], 0);
      break;
    case TSREQ_PTHREAD_RWLOCK_LOCK_POST:
      if (ignoring_sync(vg_tid, args[1]))
        break;
      Put(args[2] ? WRITER_LOCK : READER_LOCK, ts_tid, pc, /*lock=*/args[1], 0);
      break;
    case TSREQ_PTHREAD_RWLOCK_UNLOCK_PRE:
      if (ignoring_sync(vg_tid, args[1]))
        break;
      Put(UNLOCK, ts_tid, pc, /*lock=*/args[1], 0);
      break;
    case TSREQ_PTHREAD_SPIN_LOCK_INIT_OR_UNLOCK:
      Put(UNLOCK_OR_INIT, ts_tid, pc, /*lock=*/args[1], 0);
      break;
    case TSREQ_POSIX_SEM_INIT_POST:
    case TSREQ_POSIX_SEM_DESTROY_PRE:
      break;
    case TSREQ_SIGNAL:
      if (ignoring_sync(vg_tid, args[1]))
        break;
      Put(SIGNAL, ts_tid, pc, args[1], 0);
      break;
    case TSREQ_WAIT:
      if (ignoring_sync(vg_tid, args[1]))
        break;
      Put(WAIT, ts_tid, pc, args[1], 0);
      break;
    case TSREQ_CYCLIC_BARRIER_INIT:
      Put(CYCLIC_BARRIER_INIT, ts_tid, pc, args[1], args[2]);
      break;
    case TSREQ_CYCLIC_BARRIER_WAIT_BEFORE:
      Put(CYCLIC_BARRIER_WAIT_BEFORE, ts_tid, pc, args[1], 0);
      break;
    case TSREQ_CYCLIC_BARRIER_WAIT_AFTER:
      Put(CYCLIC_BARRIER_WAIT_AFTER, ts_tid, pc, args[1], 0);
      break;
    case TSREQ_GET_MY_SEGMENT:
      break;
    case TSREQ_GET_THREAD_ID:
      *ret = ts_tid;
      break;
    case TSREQ_GET_VG_THREAD_ID:
      *ret = vg_tid;
      break;
    case TSREQ_GET_SEGMENT_ID:
      break;
    case TSREQ_THREAD_SANITIZER_QUERY:
      *ret = (UWord)ThreadSanitizerQuery((const char *)args[1]);
      break;
    case TSREQ_FLUSH_STATE:
      Put(FLUSH_STATE, ts_tid, pc, 0, 0);
      break;
    default: CHECK(0);
  }
  return True;
}

static void SignalIn(ThreadId vg_tid, Int sigNo, Bool alt_stack) {
  g_valgrind_threads[vg_tid].in_signal_handler++;
  DCHECK(g_valgrind_threads[vg_tid].in_signal_handler == 1);
//  int32_t ts_tid = VgTidToTsTid(vg_tid);
//  Printf("T%d %s\n", ts_tid, __FUNCTION__);
}

static void SignalOut(ThreadId vg_tid, Int sigNo) {
  g_valgrind_threads[vg_tid].in_signal_handler--;
  CHECK(g_valgrind_threads[vg_tid].in_signal_handler >= 0);
  DCHECK(g_valgrind_threads[vg_tid].in_signal_handler == 0);
//  int32_t ts_tid = VgTidToTsTid(vg_tid);
//  Printf("T%d %s\n", ts_tid, __FUNCTION__);
}


// ---------------------------- RaceVerifier    ---------------------------{{{1

/**
 * In race verifier mode _every_ IRSB is instrumented with a sleep loop at the
 * beginning (but, of course, in most cases it is not executed).
 * Its code logically looks like
 *  irsb_start:
 *   bool need_sleep = OnTraceVerify1();
 *   if (need_sleep) {
 *     sched_yield();
 *     goto irsb_start;
 *   }
 *   OnTraceVerify2(trace_info);
 *
 * This loop verifies mops from the _previous_ trace_info and sets up the new
 * trace info in OnTraceVerify2. Only IRSBs with "interesting" mops have
 * non-zero trace_info.
 */

/**
 * Race verification loop.
 * On the first pass (for a trace_info), if there are mops to be verified,
 * register them with RaceVerifier and calculate the wake up time.
 * On the following passes, check the wake up time against the clock.
 * The loop state is kept in ValgrindThread.
 * Returns true if need to sleep more, false if the loop must be ended.
 */
VG_REGPARM(1)
static uint32_t OnTraceVerify1() {
  DCHECK(g_race_verifier_active);
  ThreadId vg_tid = GetVgTid();

  // First, flush the old trace_info.
  ValgrindThread *thr = &g_valgrind_threads[vg_tid];

  // thr->trace_info is the trace info for the previous superblock.
  if (!thr->trace_info)
    // Nothing to do here.
    return 0;

  if (!thr->verifier_current_pc) {
    // This is the first iteration of the sleep loop.
    // Register memory accesses.
    int sleep_time_ms = RaceVerifierGetSleepTime(thr->trace_info->pc());
    if (!sleep_time_ms) {
      thr->trace_info = NULL;
      return 0;
    }
    size_t n = thr->trace_info->n_mops();
    uintptr_t* tleb = thr->tleb;
    int need_sleep = 0;
    for (size_t i = 0; i < n; ++i) {
      uintptr_t addr = tleb[i];
      if (addr) {
        MopInfo *mop = thr->trace_info->GetMop(i);
        need_sleep += RaceVerifierStartAccess(thr->zero_based_uniq_tid, addr,
            mop->pc(), mop->is_write());
      }
    }
    // Setup the sleep timer.
    thr->verifier_current_pc = thr->trace_info->pc();
    if (need_sleep) {
      unsigned now = VG_(read_millisecond_timer)();
      thr->verifier_wakeup_time_ms = now + sleep_time_ms;
      return 1;
    } else {
      thr->verifier_current_pc = (unsigned)-1;
      return 0;
    }
  } else {
    // Continuation of the sleep loop.
    DCHECK(thr->verifier_current_pc == thr->trace_info->pc());
    unsigned now = VG_(read_millisecond_timer)();
    if (now < thr->verifier_wakeup_time_ms) {
      // sleep more
      return 1;
    } else {
      // done, go straight to OnTraceVerify2
      thr->verifier_current_pc = (unsigned)-1;
      return 0;
    }
  }
}

/**
 * Race verification loop exit.
 * Unregisters mops with the RaceVerifier.
 * Sets up the new trace_info.
 */
VG_REGPARM(1)
static void OnTraceVerify2(TraceInfo *trace_info) {
  DCHECK(g_race_verifier_active);
  ThreadId vg_tid = GetVgTid();
  ValgrindThread *thr = &g_valgrind_threads[vg_tid];

  DCHECK(!thr->trace_info || thr->verifier_current_pc == (unsigned)-1);
  thr->verifier_current_pc = 0;
  thr->verifier_wakeup_time_ms = 0;

  if (thr->trace_info) {
    // Unregister accesses from the old trace_info.
    size_t n = thr->trace_info->n_mops();
    uintptr_t* tleb = thr->tleb;
    for (size_t i = 0; i < n; ++i) {
      uintptr_t addr = tleb[i];
      if (addr) {
        MopInfo *mop = thr->trace_info->GetMop(i);
        RaceVerifierEndAccess(thr->zero_based_uniq_tid, addr,
            mop->pc(), mop->is_write());
      }
    }
  }

  // Start the new trace, zero the contents of tleb.
  thr->trace_info = trace_info;
  if (trace_info) {
    size_t n = trace_info->n_mops();
    uintptr_t *tleb = thr->tleb;
    for (size_t i = 0; i < n; i++)
      tleb[i] = 0;
    DCHECK(thr->trace_info->n_mops() <= kMaxMopsPerTrace);
  }
}

/**
 * Add a race verification preamble to the IRSB.
 */
static void ts_instrument_trace_entry_verify(IRSB *bbOut,
    VexGuestLayout* layout, TraceInfo *trace_info, uintptr_t cur_pc) {
   HChar*   hName = (HChar*)"OnTraceVerify1";
   void *callback = (void*)OnTraceVerify1;
   IRExpr **args = mkIRExprVec_0();
   IRTemp need_sleep = newIRTemp(bbOut->tyenv, Ity_I32);
   IRDirty* di = unsafeIRDirty_1_N(need_sleep, 0, hName,
       VG_(fnptr_to_fnentry)(callback), args);
   addStmtToIRSB( bbOut, IRStmt_Dirty(di));

   IRTemp need_sleep_i1 = newIRTemp(bbOut->tyenv, Ity_I1);
   IRStmt* cmp_stmt = IRStmt_WrTmp(need_sleep_i1,
       IRExpr_Binop(Iop_CmpNE32,
           IRExpr_RdTmp(need_sleep),
           IRExpr_Const(IRConst_U32(0))));
   addStmtToIRSB(bbOut, cmp_stmt);

   IRConst* exit_dst = layout->sizeof_IP == 8 ?
       IRConst_U64(cur_pc) : IRConst_U32(cur_pc);
   IRStmt* exit_stmt = IRStmt_Exit(IRExpr_RdTmp(need_sleep_i1),
       Ijk_YieldNoRedir, exit_dst);
   addStmtToIRSB(bbOut, exit_stmt);

   hName = (HChar*)"OnTraceVerify2";
   callback = (void*)OnTraceVerify2;
   args = mkIRExprVec_1(mkIRExpr_HWord((HWord)trace_info));
   di = unsafeIRDirty_0_N(1, hName, VG_(fnptr_to_fnentry)(callback), args);
   addStmtToIRSB( bbOut, IRStmt_Dirty(di));
}


// ---------------------------- Instrumentation ---------------------------{{{1

static IRTemp gen_Get_SP ( IRSB*           bbOut,
                           VexGuestLayout* layout,
                           Int             hWordTy_szB )
{
  IRExpr* sp_expr;
  IRTemp  sp_temp;
  IRType  sp_type;
  /* This in effect forces the host and guest word sizes to be the
     same. */
  tl_assert(hWordTy_szB == layout->sizeof_SP);
  sp_type = layout->sizeof_SP == 8 ? Ity_I64 : Ity_I32;
  sp_expr = IRExpr_Get( layout->offset_SP, sp_type );
  sp_temp = newIRTemp( bbOut->tyenv, sp_type );
  addStmtToIRSB( bbOut, IRStmt_WrTmp( sp_temp, sp_expr ) );
  return sp_temp;
}

static void ts_instrument_trace_entry(IRSB *bbOut, TraceInfo *trace_info) {
   CHECK(trace_info);
   HChar*   hName = (HChar*)"OnTrace";
   void *callback = (void*)OnTrace;
   IRExpr **args = mkIRExprVec_1(mkIRExpr_HWord((HWord)trace_info));
   IRDirty* di = unsafeIRDirty_0_N( 1,
                           hName,
                           VG_(fnptr_to_fnentry)(callback),
                           args);
   addStmtToIRSB( bbOut, IRStmt_Dirty(di));
}

static void ts_instrument_final_jump (
                                /*MOD*/IRSB* sbOut,
                                IRExpr* next,
                                IRJumpKind jumpkind,
                                VexGuestLayout* layout,
                                IRType gWordTy, IRType hWordTy ) {

#ifndef VGP_arm_linux
  // On non-ARM systems we instrument only function calls.
  if (jumpkind != Ijk_Call) return;
#else
  if (jumpkind != Ijk_Call) {
    // On an ARM system a non-call jump may possibly exit a function.
    IRTemp sp_post_call_insn
        = gen_Get_SP( sbOut, layout, sizeofIRType(hWordTy) );
    IRExpr **args = mkIRExprVec_2(
        IRExpr_RdTmp(sp_post_call_insn),
        next
        );
    IRDirty* di = unsafeIRDirty_0_N(
        2/*regparms*/,
        (char*)"evh__delete_frame",
        VG_(fnptr_to_fnentry)((void*) &evh__delete_frame ),
        args );
    addStmtToIRSB( sbOut, IRStmt_Dirty(di) );
    return;  // do not fall through
  }
#endif
  {
    const char *fn_name = "evh__rtn_call_ignore_unknown";
    void *fn = (void*)&evh__rtn_call_ignore_unknown;
    // Instrument the call instruction to keep the shadow stack consistent.
    IRTemp sp_post_call_insn
        = gen_Get_SP( sbOut, layout, sizeofIRType(hWordTy) );
    IRExpr **args = mkIRExprVec_2(
        IRExpr_RdTmp(sp_post_call_insn),
        next
        );
    if (next->tag == Iex_Const) {
      IRConst *con = next->Iex.Const.con;
      uintptr_t target = 0;
      if (con->tag == Ico_U32 || con->tag == Ico_U64) {
        target = con->tag == Ico_U32 ? con->Ico.U32 : con->Ico.U64;
        bool ignore = ThreadSanitizerIgnoreAccessesBelowFunction(target);
        if (ignore) {
          fn_name = "evh__rtn_call_ignore_yes";
          fn = (void*)&evh__rtn_call_ignore_yes;
        } else {
          fn_name = "evh__rtn_call_ignore_no";
          fn = (void*)&evh__rtn_call_ignore_no;
        }
      }
    }
    IRDirty* di = unsafeIRDirty_0_N(
        2/*regparms*/,
        (char*)fn_name,
        VG_(fnptr_to_fnentry)(fn),
        args );
    addStmtToIRSB( sbOut, IRStmt_Dirty(di) );
  }
}

// Generate exprs/stmts that make g_cur_tleb[idx] = x.
static void gen_store_to_tleb(IRSB *bbOut, IRTemp tleb_temp,
                              uintptr_t idx, IRExpr *x, IRType tyAddr) {
  CHECK(tleb_temp != IRTemp_INVALID);
  IRExpr *idx_expr  = mkIRExpr_HWord(idx * sizeof(uintptr_t));
  IRExpr *tleb_plus_idx_expr = IRExpr_Binop(
      sizeof(uintptr_t) == 8 ? Iop_Add64 : Iop_Add32,
      IRExpr_RdTmp(tleb_temp), idx_expr);
  IRTemp temp = newIRTemp(bbOut->tyenv, tyAddr);
  IRStmt *temp_stmt = IRStmt_WrTmp(temp, tleb_plus_idx_expr);
  IRStmt *store_stmt = IRStmt_Store(Iend_LE, IRExpr_RdTmp(temp), x);

  addStmtToIRSB(bbOut, temp_stmt);
  addStmtToIRSB(bbOut, store_stmt);
}

static void instrument_mem_access ( TraceInfo *trace_info,
                                    IRTemp tleb_temp,
                                    uintptr_t pc,
                                    size_t  *trace_idx,
                                    IRSB*   bbOut,
                                    IRStmt* st,
                                    IRExpr* addr,
                                    Int     szB,
                                    Bool    isStore,
                                    Bool    dtor_head,
                                    Int     hWordTy_szB ) {
  IRType   tyAddr   = Ity_INVALID;

  tl_assert(isIRAtom(addr));
  tl_assert(hWordTy_szB == 4 || hWordTy_szB == 8);

  tyAddr = typeOfIRExpr( bbOut->tyenv, addr );
  tl_assert(tyAddr == Ity_I32 || tyAddr == Ity_I64);

  if (szB == 28) {
    // Ignore weird-sized accesses for now.
    // See http://code.google.com/p/data-race-test/issues/detail?id=36
    return;
  }

  bool check_ident_store = false;

  if (st->tag == Ist_Store && dtor_head && 
      typeOfIRExpr(bbOut->tyenv, st->Ist.Store.data) == tyAddr) {
    check_ident_store = true;
  }

  size_t next_trace_idx = *trace_idx + 1;

  if (next_trace_idx > kMaxMopsPerTrace) {
    if (next_trace_idx == kMaxMopsPerTrace) {
      Report("INFO: too many mops in trace: %p %s\n", pc,
             PcToRtnName(pc, true).c_str());
    }
    return;
  }

  if (!trace_info) {
    // not instrumenting yet.
    *trace_idx = next_trace_idx;
    return;
  }

  IRExpr *expr_to_store = NULL;

  if (check_ident_store) {
    int is_64 = (sizeof(void*) == 8);
    // generate expression (*addr == new_value ? 0 : addr):

    // old_value = *addr
    IRExpr *addr_load_expr = IRExpr_Load(Iend_LE, tyAddr, addr);
    IRTemp star_addr = newIRTemp(bbOut->tyenv, tyAddr);
    IRStmt *star_addr_stmt = IRStmt_WrTmp(star_addr, addr_load_expr);
    addStmtToIRSB(bbOut, star_addr_stmt);
    // sub = (old_value - new_value)
    IRTemp sub = newIRTemp(bbOut->tyenv, tyAddr);
    IRExpr *sub_expr = IRExpr_Binop((IROp)(Iop_Sub32 + is_64),
                                    IRExpr_RdTmp(star_addr),
                                    st->Ist.Store.data);
    IRStmt *sub_stmt = IRStmt_WrTmp(sub, sub_expr);
    addStmtToIRSB(bbOut, sub_stmt);
    // mask = (sub==0) ? 0 : -1
    IRTemp mask = newIRTemp(bbOut->tyenv, tyAddr);
    IRExpr *mask_expr = IRExpr_Unop((IROp)(Iop_CmpwNEZ32 + is_64),
                                    IRExpr_RdTmp(sub));
    IRStmt *mask_stmt = IRStmt_WrTmp(mask, mask_expr);
    addStmtToIRSB(bbOut, mask_stmt);

    // res = mask & addr
    IRTemp and_tmp = newIRTemp(bbOut->tyenv, tyAddr);
    IRExpr *and_expr = IRExpr_Binop((IROp)(Iop_And32 + is_64),
                                    IRExpr_RdTmp(mask), addr);
    IRStmt *and_stmt = IRStmt_WrTmp(and_tmp, and_expr);
    addStmtToIRSB(bbOut, and_stmt);

    expr_to_store = IRExpr_RdTmp(and_tmp);
  } else {
    expr_to_store = addr;
  }

  // OnMop: g_cur_tleb[idx] = expr_to_store
  gen_store_to_tleb(bbOut, tleb_temp, *trace_idx, expr_to_store, tyAddr);
  // Create a mop {pc, size, is_write}
  MopInfo *mop = trace_info->GetMop(*trace_idx);
  new (mop) MopInfo(pc, szB, isStore, false);
  (*trace_idx)++;

  CHECK(*trace_idx == next_trace_idx);
}

void instrument_statement (IRStmt* st, IRSB* bbIn, IRSB* bbOut, IRType hWordTy,
                           TraceInfo *trace_info, IRTemp tleb_temp,
                           size_t *idx, uintptr_t *cur_pc, bool dtor_head) {
  switch (st->tag) {
    case Ist_NoOp:
    case Ist_AbiHint:
    case Ist_Put:
    case Ist_PutI:
    case Ist_Exit:
      /* None of these can contain any memory references. */
      break;

    case Ist_IMark:
      *cur_pc = st->Ist.IMark.addr;
      break;

    case Ist_MBE:
      //instrument_memory_bus_event( bbOut, st->Ist.MBE.event );
      switch (st->Ist.MBE.event) {
        case Imbe_Fence:
          break; /* not interesting */
        default:
          ppIRStmt(st);
          tl_assert(0);
      }
      break;

    case Ist_CAS:
      break;

    case Ist_Store:
      instrument_mem_access(trace_info, tleb_temp, *cur_pc, idx,
        bbOut, st,
        st->Ist.Store.addr,
        sizeofIRType(typeOfIRExpr(bbIn->tyenv, st->Ist.Store.data)),
        True/*isStore*/, dtor_head,
        sizeofIRType(hWordTy)
      );
      break;

    case Ist_WrTmp: {
      IRExpr* data = st->Ist.WrTmp.data;
      if (data->tag == Iex_Load) {
        instrument_mem_access(trace_info, tleb_temp, *cur_pc, idx,
            bbOut, st,
            data->Iex.Load.addr,
            sizeofIRType(data->Iex.Load.ty),
            False/*!isStore*/, dtor_head,
            sizeofIRType(hWordTy)
            );
      }
      break;
    }

    case Ist_LLSC: {
      /* Ignore load-linked's and store-conditionals. */
      break;
    }

    case Ist_Dirty: {
      Int      dataSize;
      IRDirty* d = st->Ist.Dirty.details;
      if (d->mFx != Ifx_None) {
        /* This dirty helper accesses memory.  Collect the
           details. */
        tl_assert(d->mAddr != NULL);
        tl_assert(d->mSize != 0);
        dataSize = d->mSize;
        if (d->mFx == Ifx_Read || d->mFx == Ifx_Modify) {
          instrument_mem_access(trace_info, tleb_temp, *cur_pc, idx,
            bbOut, st, d->mAddr, dataSize, False/*!isStore*/, dtor_head,
            sizeofIRType(hWordTy)
          );
        }
        if (d->mFx == Ifx_Write || d->mFx == Ifx_Modify) {
          instrument_mem_access(trace_info, tleb_temp, *cur_pc, idx,
            bbOut, st, d->mAddr, dataSize, True/*isStore*/, dtor_head,
            sizeofIRType(hWordTy)
          );
        }
      } else {
        tl_assert(d->mAddr == NULL);
        tl_assert(d->mSize == 0);
      }
      break;
    }

    default:
      ppIRStmt(st);
      tl_assert(0);
  } /* switch (st->tag) */
}

static IRSB* ts_instrument ( VgCallbackClosure* closure,
                             IRSB* bbIn,
                             VexGuestLayout* layout,
                             VexGuestExtents* vge,
                             IRType gWordTy, IRType hWordTy) {
  if (G_flags->dry_run >= 2) return bbIn;
  Int   i;
  IRSB* bbOut;
  uintptr_t pc = closure->readdr;

  char objname[kBuffSize];
  if (VG_(get_objname)(pc, (Char*)objname, kBuffSize)) {
    if (StringMatch("*/ld-2*", objname)) {
      // we want to completely ignore ld-so.
      return bbIn;
    }
  }

  bool instrument_memory = ThreadSanitizerWantToInstrumentSblock(pc);

  if (gWordTy != hWordTy) {
    /* We don't currently support this case. */
    VG_(tool_panic)((Char*)"host/guest word size mismatch");
  }

  /* Set up BB */
  bbOut           = emptyIRSB();
  bbOut->tyenv    = deepCopyIRTypeEnv(bbIn->tyenv);
  bbOut->next     = deepCopyIRExpr(bbIn->next);
  bbOut->jumpkind = bbIn->jumpkind;

  // Copy verbatim any IR preamble preceding the first IMark
  i = 0;
  while (i < bbIn->stmts_used && bbIn->stmts[i]->tag != Ist_IMark) {
    addStmtToIRSB( bbOut, bbIn->stmts[i] );
    i++;
  }
  int first = i;
  size_t n_mops = 0;
  uintptr_t cur_pc = pc;

  IRTemp tleb_temp = IRTemp_INVALID;

  bool dtor_head = false;
  char buff[1000];
  // get_fnname_w_offset returns demangled name with optional "+offset" prefix.
  // If we have "::~" and don't have "+", this SB is the first in this dtor.
  // We do all this stuff to avoid benign races on vptr:
  // http://code.google.com/p/data-race-test/wiki/PopularDataRaces#Data_race_on_vptr
  if (VG_(get_fnname_w_offset)(pc, (Char*)buff, sizeof(buff)) &&
      VG_(strstr)((Char*)buff, (Char*)"::~") != NULL) {
    char *offset_str = (char*)VG_(strchr)((Char*)buff, '+');
    if (offset_str == NULL) {
      // we are in the first BB of DTOR.
      dtor_head = true;
    } else {
      // We are not in the first BB.
      // On x86_64 (it seems like) the vfptr is updated only in the first BB.
      // On x86 with -fPIC, the vfptr may be updated in the second BB
      // (because -fPIC adds a call which splits the first BB).
      // See http://code.google.com/p/chromium/issues/detail?id=61199
#ifdef VGA_x86
      char *end;
      size_t offset = my_strtol(offset_str + 1, &end, 10);
      if (offset <= 32) {
        dtor_head = true;
      }
#endif
    }
  }


  uintptr_t instrument_pc = 0; // if != 0, instrument only the instruction at this address
  if (g_race_verifier_active) {
    uintptr_t min_pc = vge->base[0];
    uintptr_t max_pc = min_pc + vge->len[0];
    bool verify_trace = RaceVerifierGetAddresses(min_pc, max_pc, &instrument_pc);
    if (!verify_trace)
      instrument_memory = false;
  }

  // count mops
  if (instrument_memory) {
    for (i = first; i < bbIn->stmts_used; i++) {
      IRStmt* st = bbIn->stmts[i];
      tl_assert(st);
      tl_assert(isFlatIRStmt(st));
      if (st->tag == Ist_IMark)
        cur_pc = st->Ist.IMark.addr;
      if (!instrument_pc || cur_pc == instrument_pc)
        instrument_statement(st, bbIn, bbOut, hWordTy,
            NULL, tleb_temp, &n_mops, &cur_pc, dtor_head);
    } /* iterate over bbIn->stmts */
  }
  TraceInfo *trace_info = NULL;
  if (n_mops > 0) {
    trace_info = TraceInfo::NewTraceInfo(n_mops, pc);
  }
  size_t n_mops_done = 0;
  bool need_to_insert_on_trace = n_mops > 0 || g_race_verifier_active;
  // instrument mops and copy the rest of BB to the new one.
  for (i = first; i < bbIn->stmts_used; i++) {
    IRStmt* st = bbIn->stmts[i];
    tl_assert(st);
    tl_assert(isFlatIRStmt(st));
    if (st->tag != Ist_IMark && need_to_insert_on_trace) {
      if (g_race_verifier_active) {
        ts_instrument_trace_entry_verify(bbOut, layout, trace_info,
            closure->readdr);
      } else {
        ts_instrument_trace_entry(bbOut, trace_info);
      }
      need_to_insert_on_trace = false;
      // Generate temp for *g_cur_tleb.
      IRType   tyAddr = sizeof(uintptr_t) == 8 ?  Ity_I64 : Ity_I32;
      IRExpr *tleb_ptr_expr = mkIRExpr_HWord((HWord)&g_cur_tleb);
      IRExpr *tleb_expr = IRExpr_Load(Iend_LE, tyAddr, tleb_ptr_expr);
      tleb_temp = newIRTemp(bbOut->tyenv, tyAddr);
      IRStmt *stmt = IRStmt_WrTmp(tleb_temp, tleb_expr);
      addStmtToIRSB(bbOut, stmt);
    }
    if (instrument_memory) {
      if (st->tag == Ist_IMark)
        cur_pc = st->Ist.IMark.addr;
      if (!instrument_pc || cur_pc == instrument_pc)
        instrument_statement(st, bbIn, bbOut, hWordTy,
            trace_info, tleb_temp, &n_mops_done, &cur_pc, dtor_head);
    }
    addStmtToIRSB( bbOut, st );
  } /* iterate over bbIn->stmts */
  CHECK(n_mops == n_mops_done);
  if (!g_race_verifier_active)
    ts_instrument_final_jump(bbOut, bbIn->next, bbIn->jumpkind, layout, gWordTy, hWordTy);
  return bbOut;
}

extern "C"
void ts_pre_clo_init(void) {
  VG_(details_name)            ((Char*)"ThreadSanitizer");
  VG_(details_version)         ((Char*)NULL);
  VG_(details_description)     ((Char*)"a data race detector");
  VG_(details_copyright_author)(
      (Char*)"Copyright (C) 2008-2010, and GNU GPL'd, by Google Inc.");
  VG_(details_bug_reports_to)  ((Char*)"data-race-test@googlegroups.com");

  VG_(basic_tool_funcs)        (ts_post_clo_init,
                                ts_instrument,
                                ts_fini);

  VG_(needs_client_requests)     (ts_handle_client_request);

  VG_(needs_command_line_options)(ts_process_cmd_line_option,
                                  ts_print_usage,
                                  ts_print_debug_usage);
   VG_(track_pre_thread_ll_create)( evh__pre_thread_ll_create );
   VG_(track_pre_thread_ll_exit)  ( evh__pre_thread_ll_exit );

   if (!g_race_verifier_active) {
     VG_(track_workq_task_start)( evh__pre_workq_task_start );
     VG_(track_pre_thread_first_insn)( evh__pre_thread_first_insn );
   }

   VG_(clo_vex_control).iropt_unroll_thresh = 0;
   VG_(clo_vex_control).guest_chase_thresh = 0;

   VG_(track_pre_deliver_signal) (&SignalIn);
   VG_(track_post_deliver_signal)(&SignalOut);

   VG_(track_start_client_code)( OnStartClientCode );
}

VG_DETERMINE_INTERFACE_VERSION(ts_pre_clo_init)

// {{{1 end
// vim:shiftwidth=2:softtabstop=2:expandtab
