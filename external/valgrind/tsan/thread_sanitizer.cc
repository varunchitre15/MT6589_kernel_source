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

// You can find the details on this tool at
// http://code.google.com/p/data-race-test

#include "thread_sanitizer.h"
#include "common_util.h"
#include "suppressions.h"
#include "ignore.h"
#include "ts_lock.h"
#include "ts_atomic_int.h"
#include "dense_multimap.h"
#include <stdarg.h>
// -------- Constants --------------- {{{1
// Segment ID (SID)      is in range [1, kMaxSID-1]
// Segment Set ID (SSID) is in range [-kMaxSID+1, -1]
// This is not a compile-time constant, but it can only be changed at startup.
int kMaxSID = (1 << 23);
// Flush state after so many SIDs have been allocated. Set by command line flag.
int kMaxSIDBeforeFlush;

// Lock ID (LID)      is in range [1, kMaxLID-1]
// Lock Set ID (LSID) is in range [-kMaxLID+1, -1]
const int kMaxLID = (1 << 23);

// This is not a compile-time constant, but it can be changed only at startup.
int kSizeOfHistoryStackTrace = 10;

// Maximal number of segments in a SegmentSet.
// If you change this constant, you also need to change several places
// in SegmentSet code.
const int kMaxSegmentSetSize = 4;

// -------- Globals --------------- {{{1

// If true, ignore all accesses in all threads.
bool global_ignore;

bool g_so_far_only_one_thread = false;
bool g_has_entered_main = false;
bool g_has_exited_main = false;

size_t g_last_flush_time;

// Incremented on each Lock and Unlock. Used by LockHistory.
uint32_t g_lock_era = 0;

uintptr_t g_nacl_mem_start = (uintptr_t)-1;
uintptr_t g_nacl_mem_end = (uintptr_t)-1;

bool g_race_verifier_active = false;

bool debug_expected_races = false;
bool debug_benign_races = false;
bool debug_malloc = false;
bool debug_free = false;
bool debug_thread = false;
bool debug_ignore = false;
bool debug_rtn = false;
bool debug_lock = false;
bool debug_wrap = false;
bool debug_ins = false;
bool debug_shadow_stack = false;
bool debug_happens_before = false;
bool debug_cache = false;
bool debug_race_verifier = false;
bool debug_atomic = false;

#define PrintfIf(flag, ...) \
  do { if ((flag)) Printf(__VA_ARGS__); } while ((void)0, 0)

// -------- TIL --------------- {{{1
// ThreadSanitizer Internal lock (scoped).
class TIL {
 public:
  TIL(TSLock *lock, int lock_site, bool need_locking = true) :
    lock_(lock),
    need_locking_(need_locking) {
    DCHECK(lock_);
    if (need_locking_ && (TS_SERIALIZED == 0)) {
      lock_->Lock();
      G_stats->lock_sites[lock_site]++;
    }
  }
  ~TIL() {
    if (need_locking_ && (TS_SERIALIZED == 0))
      lock_->Unlock();
  }
 private:
  TSLock *lock_;
  bool need_locking_;
};

static TSLock *ts_lock;
static TSLock *ts_ignore_below_lock;

#ifdef TS_LLVM
void ThreadSanitizerLockAcquire() {
  ts_lock->Lock();
}

void ThreadSanitizerLockRelease() {
  ts_lock->Unlock();
}
#endif

static INLINE void AssertTILHeld() {
  if (TS_SERIALIZED == 0 && DEBUG_MODE) {
    ts_lock->AssertHeld();
  }
}

// -------- Util ----------------------------- {{{1

// Can't use ANNOTATE_UNPROTECTED_READ, it may get instrumented.
template <class T>
inline T INTERNAL_ANNOTATE_UNPROTECTED_READ(const volatile T &x) {
  ANNOTATE_IGNORE_READS_BEGIN();
  T res = x;
  ANNOTATE_IGNORE_READS_END();
  return res;
}

static string RemoveFilePrefix(string str) {
  for (size_t i = 0; i < G_flags->file_prefix_to_cut.size(); i++) {
    string prefix_to_cut = G_flags->file_prefix_to_cut[i];
    size_t pos = str.find(prefix_to_cut);
    if (pos != string::npos) {
      str = str.substr(pos + prefix_to_cut.size());
    }
  }
  if (str.find("./") == 0) {  // remove leading ./
    str = str.substr(2);
  }
  return str;
}

string PcToRtnNameAndFilePos(uintptr_t pc) {
  G_stats->pc_to_strings++;
  string img_name;
  string file_name;
  string rtn_name;
  int line_no = -1;
  PcToStrings(pc, G_flags->demangle, &img_name, &rtn_name,
              &file_name, &line_no);
  if (G_flags->demangle && !G_flags->full_stack_frames)
    rtn_name = NormalizeFunctionName(rtn_name);
  file_name = RemoveFilePrefix(file_name);
  if (file_name == "") {
    return rtn_name + " " + RemoveFilePrefix(img_name);
  }
  char buff[10];
  snprintf(buff, sizeof(buff), "%d", line_no);
  return rtn_name + " " + file_name + ":" + buff;
}

// -------- ID ---------------------- {{{1
// We wrap int32_t into ID class and then inherit various ID type from ID.
// This is done in an attempt to implement type safety of IDs, i.e.
// to make it impossible to make implicit cast from one ID type to another.
class ID {
 public:
  typedef int32_t T;
  explicit ID(T id) : id_(id) {}
  ID(const ID &id) : id_(id.id_) {}
  INLINE bool operator ==  (const ID &id) const { return id_ == id.id_; }
  bool operator !=  (const ID &id) const { return id_ != id.id_; }
  bool operator <  (const ID &id) const { return id_ < id.id_; }
  bool operator >  (const ID &id) const { return id_ > id.id_; }
  bool operator >=  (const ID &id) const { return id_ >= id.id_; }
  bool operator <=  (const ID &id) const { return id_ <= id.id_; }

  bool IsValid() const { return id_ >= 0; }

  const ID &operator = (const ID &id) {
    this->id_ = id.id_;
    return *this;
  }
  T raw() const { return id_; }

 private:
  T id_;
};

// Thread ID.
// id >= 0
class TID: public ID {
 public:
  static const int32_t kInvalidTID;

  explicit TID(T id) : ID(id) {}
  TID() : ID(kInvalidTID) {}
  bool valid() const { return raw() >= 0; }
};

const int32_t TID::kInvalidTID = -1;

// Segment ID.
// id > 0 && id < kMaxSID
class SID: public ID {
 public:
  explicit SID(T id) : ID(id) {}
  SID() : ID(0) {}
  bool valid() const { return raw() > 0 && raw() < kMaxSID; }
};

// Lock ID.
// id > 0 && id < kMaxLID
class LID: public ID {
 public:
  explicit LID(T id) : ID(id) {}
  LID() : ID(0) {}
  bool valid() const { return raw() > 0 && raw() < kMaxLID; }
};

// LockSet ID.
// Empty lockset: id == 0
// Singleton:     id > 0 (id == Lock's id)
// Tuple:         id < 0
class LSID: public ID {
 public:
  explicit LSID(T id) : ID(id) {}
  LSID() : ID(INT_MAX) {}
  bool valid() const {
    return raw() < kMaxLID && raw() > -(kMaxLID);
  }
  bool IsEmpty() const { return raw() == 0; }
  bool IsSingleton() const { return raw() > 0; }
  LID GetSingleton() const { return LID(raw()); }
};

// SegmentSet ID.
// Empty SegmentSet: id == 0
// Singleton:        id > 0 (id == Segment's id)
// Tuple:            id < 0
class SSID: public ID {
 public:
  explicit SSID(T id) : ID(id) {}
  explicit SSID(SID sid) : ID(sid.raw()) {}
  SSID(): ID(INT_MAX) {}
  bool valid() const {
    return raw() != 0 && raw() < kMaxSID && raw() > -kMaxSID;
  }
  bool IsValidOrEmpty() { return raw() < kMaxSID && raw() > -kMaxSID; }
  bool IsEmpty() const { return raw() == 0; }
  bool IsSingleton() const {return raw() > 0; }
  bool IsTuple() const {return raw() < 0; }
  SID  GetSingleton() const {
    DCHECK(IsSingleton());
    return SID(raw());
  }
  // TODO(timurrrr): need to start SegmentSetArray indices from 1
  // to avoid "int ???() { return -raw() - 1; }"
};

// -------- Colors ----------------------------- {{{1
// Colors for ansi terminals and for html.
const char *c_bold    = "";
const char *c_red     = "";
const char *c_green   = "";
const char *c_magenta = "";
const char *c_cyan    = "";
const char *c_blue    = "";
const char *c_yellow  = "";
const char *c_default = "";


// -------- Forward decls ------ {{{1
static void ForgetAllStateAndStartOver(TSanThread *thr, const char *reason);
static void FlushStateIfOutOfSegments(TSanThread *thr);
static int32_t raw_tid(TSanThread *t);
// -------- Simple Cache ------ {{{1
#include "ts_simple_cache.h"
// -------- PairCache & IntPairToIntCache ------ {{{1
template <typename A, typename B, typename Ret,
         int kHtableSize, int kArraySize = 8>
class PairCache {
 public:
  PairCache() {
    CHECK(kHtableSize >= 0);
    CHECK(sizeof(Entry) == sizeof(A) + sizeof(B) + sizeof(Ret));
    Flush();
  }

  void Flush() {
    memset(this, 0, sizeof(*this));

    // Change the first hashtable entry so it doesn't match (0,0) on Lookup.
    if (kHtableSize != 0)
      memset(&htable_[0], 1, sizeof(Entry));

    // Any Lookup should fail now.
    for (int i = 0; i < kHtableSize; i++) {
      Ret tmp;
      DCHECK(!Lookup(htable_[i].a, htable_[i].b, &tmp));
    }
    CHECK(array_pos_    == 0);
    CHECK(array_filled_ == false);
  }

  void Insert(A a, B b, Ret v) {
    // fill the hash table
    if (kHtableSize != 0) {
      uint32_t idx  = compute_idx(a, b);
      htable_[idx].Fill(a, b, v);
    }

    // fill the array
    Ret dummy;
    if (kArraySize != 0 && !ArrayLookup(a, b, &dummy)) {
      array_[array_pos_ % kArraySize].Fill(a, b, v);
      array_pos_ = (array_pos_ + 1) % kArraySize;
      if (array_pos_ > kArraySize)
        array_filled_ = true;
    }
  }

  INLINE bool Lookup(A a, B b, Ret *v) {
    // check the array
    if (kArraySize != 0 && ArrayLookup(a, b, v)) {
      G_stats->ls_cache_fast++;
      return true;
    }
    // check the hash table.
    if (kHtableSize != 0) {
      uint32_t idx  = compute_idx(a, b);
      Entry & prev_e = htable_[idx];
      if (prev_e.Match(a, b)) {
        *v = prev_e.v;
        return true;
      }
    }
    return false;
  }

 private:
  struct Entry {
    A a;
    B b;
    Ret v;
    void Fill(A a, B b, Ret v) {
      this->a = a;
      this->b = b;
      this->v = v;
    }
    bool Match(A a, B b) const {
      return this->a == a && this->b == b;
    }
  };

  INLINE bool ArrayLookup(A a, B b, Ret *v) {
    for (int i = 0; i < (array_filled_ ? kArraySize : array_pos_); i++) {
      Entry & entry = array_[i];
      if (entry.Match(a, b)) {
        *v = entry.v;
        return true;
      }
    }
    return false;
  }

  uint32_t compute_idx(A a, B b) {
    if (kHtableSize == 0)
      return 0;
    else
      return combine2(a, b) % kHtableSize;
  }

  static uint32_t combine2(int a, int b) {
    return (a << 16) ^ b;
  }

  static uint32_t combine2(SSID a, SID b) {
    return combine2(a.raw(), b.raw());
  }

  Entry htable_[kHtableSize];

  Entry array_[kArraySize];

  // array_pos_    - next element to write to the array_ (mod kArraySize)
  // array_filled_ - set to true once we write the last element of the array
  int array_pos_;
  bool array_filled_;
};

template<int kHtableSize, int kArraySize = 8>
class IntPairToIntCache
  : public PairCache<int, int, int, kHtableSize, kArraySize> {};



// -------- FreeList --------------- {{{1
class FreeList {
 public:
  FreeList(int obj_size, int chunk_size)
    : list_(0),
      obj_size_(obj_size),
      chunk_size_(chunk_size) {
    CHECK_GE(obj_size_, static_cast<int>(sizeof(NULL)));
    CHECK((obj_size_ % sizeof(NULL)) == 0);
    CHECK_GE(chunk_size_, 1);
  }

  void *Allocate() {
    if (!list_)
      AllocateNewChunk();
    CHECK(list_);
    List *head = list_;
    list_ = list_->next;
    return reinterpret_cast<void*>(head);
  }

  void Deallocate(void *ptr) {
    if (DEBUG_MODE) {
      memset(ptr, 0xac, obj_size_);
    }
    List *new_head = reinterpret_cast<List*>(ptr);
    new_head->next = list_;
    list_ = new_head;
  }

 private:
  void AllocateNewChunk() {
    CHECK(list_ == NULL);
    uint8_t *new_mem = new uint8_t[obj_size_ * chunk_size_];
    if (DEBUG_MODE) {
      memset(new_mem, 0xab, obj_size_ * chunk_size_);
    }
    for (int i = 0; i < chunk_size_; i++) {
      List *new_head = reinterpret_cast<List*>(new_mem + obj_size_ * i);
      new_head->next = list_;
      list_ = new_head;
    }
  }
  struct List {
    struct List *next;
  };
  List *list_;


  const int obj_size_;
  const int chunk_size_;
};
// -------- StackTrace -------------- {{{1
class StackTraceFreeList {
 public:
  uintptr_t *GetNewMemForStackTrace(size_t capacity) {
    DCHECK(capacity <= (size_t)G_flags->num_callers);
    return reinterpret_cast<uintptr_t*>(free_lists_[capacity]->Allocate());
  }

  void TakeStackTraceBack(uintptr_t *mem, size_t capacity) {
    DCHECK(capacity <= (size_t)G_flags->num_callers);
    free_lists_[capacity]->Deallocate(mem);
  }

  StackTraceFreeList() {
    size_t n = G_flags->num_callers + 1;
    free_lists_ = new FreeList *[n];
    free_lists_[0] = NULL;
    for (size_t i = 1; i < n; i++) {
      free_lists_[i] = new FreeList((i+2) * sizeof(uintptr_t), 1024);
    }
  }

 private:
  FreeList **free_lists_;  // Array of G_flags->num_callers lists.
};

static StackTraceFreeList *g_stack_trace_free_list;

class StackTrace {
 public:
  static StackTrace *CreateNewEmptyStackTrace(size_t size,
                                              size_t capacity = 0) {
    ScopedMallocCostCenter cc("StackTrace::CreateNewEmptyStackTrace()");
    DCHECK(g_stack_trace_free_list);
    DCHECK(size != 0);
    if (capacity == 0)
      capacity = size;
    uintptr_t *mem = g_stack_trace_free_list->GetNewMemForStackTrace(capacity);
    DCHECK(mem);
    StackTrace *res = new(mem) StackTrace(size, capacity);
    return res;
  }

  static void Delete(StackTrace *trace) {
    if (!trace) return;
    DCHECK(g_stack_trace_free_list);
    g_stack_trace_free_list->TakeStackTraceBack(
        reinterpret_cast<uintptr_t*>(trace), trace->capacity());
  }

  size_t size() const { return size_; }
  size_t capacity() const { return capacity_; }

  void set_size(size_t size) {
    CHECK(size <= capacity());
    size_ = size;
  }


  void Set(size_t i, uintptr_t pc) {
    arr_[i] = pc;
  }

  uintptr_t Get(size_t i) const {
    return arr_[i];
  }

  static bool CutStackBelowFunc(const string func_name) {
    for (size_t i = 0; i < G_flags->cut_stack_below.size(); i++) {
      if (StringMatch(G_flags->cut_stack_below[i], func_name)) {
        return true;
      }
    }
    return false;
  }

  static string EmbeddedStackTraceToString(const uintptr_t *emb_trace, size_t n,
                                           const char *indent = "    ") {
    string res = "";
    const int kBuffSize = 10000;
    char *buff = new char [kBuffSize];
    for (size_t i = 0; i < n; i++) {
      if (!emb_trace[i]) break;
      string rtn_and_file = PcToRtnNameAndFilePos(emb_trace[i]);
      if (rtn_and_file.find("(below main) ") == 0 ||
          rtn_and_file.find("ThreadSanitizerStartThread ") == 0)
        break;

      if (i == 0) res += c_bold;
      if (G_flags->show_pc) {
        snprintf(buff, kBuffSize, "%s#%-2d %p: ",
                 indent, static_cast<int>(i),
                 reinterpret_cast<void*>(emb_trace[i]));
      } else {
        snprintf(buff, kBuffSize, "%s#%-2d ", indent, static_cast<int>(i));
      }
      res += buff;

      res += rtn_and_file;
      if (i == 0) res += c_default;
      res += "\n";

      // don't print after main ...
      if (rtn_and_file.find("main ") == 0)
        break;
      // ... and after some default functions (see ThreadSanitizerParseFlags())
      // and some more functions specified via command line flag.
      string rtn = NormalizeFunctionName(PcToRtnName(emb_trace[i], true));
      if (CutStackBelowFunc(rtn))
        break;
    }
    delete [] buff;
    return res;
  }

  string ToString(const char *indent = "    ") const {
    if (!this) return "NO STACK TRACE\n";
    if (size() == 0) return "EMPTY STACK TRACE\n";
    return EmbeddedStackTraceToString(arr_, size(), indent);
  }

  void PrintRaw() const {
    for (size_t i = 0; i < size(); i++) {
      Printf("%p ", arr_[i]);
    }
    Printf("\n");
  }

  static bool Equals(const StackTrace *t1, const StackTrace *t2) {
    if (t1->size_ != t2->size_) return false;
    for (size_t i = 0; i < t1->size_; i++) {
      if (t1->arr_[i] != t2->arr_[i]) return false;
    }
    return true;
  }

  struct Less {
    bool operator() (const StackTrace *t1, const StackTrace *t2) const {
      size_t size = min(t1->size_, t2->size_);
      for (size_t i = 0; i < size; i++) {
        if (t1->arr_[i] != t2->arr_[i]) {
          return (t1->arr_[i] < t2->arr_[i]);
        }
      }
      return t1->size_ < t2->size_;
    }
  };

 private:
  StackTrace(size_t size, size_t capacity)
    : size_(size),
      capacity_(capacity) {
  }

  ~StackTrace() {}

  size_t size_;
  size_t capacity_;
  uintptr_t arr_[];
};



// -------- Lock -------------------- {{{1
const char *kLockAllocCC = "kLockAllocCC";
class Lock {
 public:

  static Lock *Create(uintptr_t lock_addr) {
    ScopedMallocCostCenter cc("LockLookup");
//    Printf("Lock::Create: %p\n", lock_addr);
    // Destroy(lock_addr);

    // CHECK(Lookup(lock_addr) == NULL);
    Lock *res = LookupOrCreate(lock_addr);
    res->rd_held_ = 0;
    res->wr_held_ = 0;
    res->is_pure_happens_before_ = G_flags->pure_happens_before;
    res->last_lock_site_ = NULL;
    return res;
  }

  static void Destroy(uintptr_t lock_addr) {
//    Printf("Lock::Destroy: %p\n", lock_addr);
  //  map_.erase(lock_addr);
  }

  static NOINLINE Lock *LookupOrCreate(uintptr_t lock_addr) {
    ScopedMallocCostCenter cc("LockLookup");
    Lock **lock = &(*map_)[lock_addr];
    if (*lock == NULL) {
//      Printf("Lock::LookupOrCreate: %p\n", lock_addr);
      ScopedMallocCostCenter cc_lock("new Lock");
      *lock = new Lock(lock_addr, map_->size());
    }
    return *lock;
  }

  static NOINLINE Lock *Lookup(uintptr_t lock_addr) {
    ScopedMallocCostCenter cc("LockLookup");
    Map::iterator it = map_->find(lock_addr);
    if (it == map_->end()) return NULL;
    return it->second;
  }

  int       rd_held()   const { return rd_held_; }
  int       wr_held()   const { return wr_held_; }
  uintptr_t lock_addr() const { return lock_addr_; }
  LID       lid()       const { return lid_; }
  bool is_pure_happens_before() const { return is_pure_happens_before_; }

  // When a lock is pure happens-before, we need to create hb arcs
  // between all Unlock/Lock pairs except RdUnlock/RdLock.
  // For that purpose have two IDs on which we signal/wait.
  // One id is the lock_addr itself, the second id is derived
  // from lock_addr.
  uintptr_t wr_signal_addr() const { return lock_addr(); }
  uintptr_t rd_signal_addr() const { return lock_addr() + 1; }


  void set_is_pure_happens_before(bool x) { is_pure_happens_before_ = x; }

  void WrLock(TID tid, StackTrace *lock_site) {
    CHECK(!rd_held_);
    if (wr_held_ == 0) {
      thread_holding_me_in_write_mode_ = tid;
    } else {
      CHECK(thread_holding_me_in_write_mode_ == tid);
    }
    wr_held_++;
    StackTrace::Delete(last_lock_site_);
    last_lock_site_ = lock_site;
  }

  void WrUnlock() {
    CHECK(!rd_held_);
    CHECK(wr_held_ > 0);
    wr_held_--;
  }

  void RdLock(StackTrace *lock_site) {
    CHECK(!wr_held_);
    rd_held_++;
    StackTrace::Delete(last_lock_site_);
    last_lock_site_ = lock_site;
  }

  void RdUnlock() {
    CHECK(!wr_held_);
    CHECK(rd_held_);
    rd_held_--;
  }

  void set_name(const char *name) { name_ = name; }
  const char *name() const { return name_; }

  string ToString() const {
    string res;
    char buff[100];
    snprintf(buff, sizeof(buff), "L%d", lid_.raw());
    // do we need to print the address?
    // reinterpret_cast<void*>(lock_addr()));
    res = buff;
    if (name()) {
      res += string(" ") + name();
    }
    return res;
  }

  static Lock *LIDtoLock(LID lid) {
    // slow, but needed only for reports.
    for (Map::iterator it = map_->begin(); it != map_->end(); ++it) {
      Lock *l = it->second;
      if (l->lid_ == lid) {
        return l;
      }
    }
    return NULL;
  }

  static string ToString(LID lid) {
    Lock *lock = LIDtoLock(lid);
    CHECK(lock);
    return lock->ToString();
  }

  static void ReportLockWithOrWithoutContext(LID lid, bool with_context) {
    if (!with_context) {
      Report("   L%d\n", lid.raw());
      return;
    }
    Lock *lock = LIDtoLock(lid);
    CHECK(lock);
    if (lock->last_lock_site_) {
      Report("   %s (%p)\n%s",
             lock->ToString().c_str(),
             lock->lock_addr_,
             lock->last_lock_site_->ToString().c_str());
    } else {
      Report("   %s. This lock was probably destroyed"
                 " w/o calling Unlock()\n", lock->ToString().c_str());
    }
  }

  static void InitClassMembers() {
    map_ = new Lock::Map;
  }

 private:
  Lock(uintptr_t lock_addr, int32_t lid)
    : lock_addr_(lock_addr),
      lid_(lid),
      rd_held_(0),
      wr_held_(0),
      is_pure_happens_before_(G_flags->pure_happens_before),
      last_lock_site_(0),
      name_(NULL) {
  }

  // Data members
  uintptr_t lock_addr_;
  LID       lid_;
  int       rd_held_;
  int       wr_held_;
  bool      is_pure_happens_before_;
  StackTrace *last_lock_site_;
  const char *name_;
  TID       thread_holding_me_in_write_mode_;

  // Static members
  typedef map<uintptr_t, Lock*> Map;
  static Map *map_;
};


Lock::Map *Lock::map_;

// Returns a string like "L123,L234".
static string SetOfLocksToString(const set<LID> &locks) {
  string res;
  for (set<LID>::const_iterator it = locks.begin();
       it != locks.end(); ++it) {
    LID lid = *it;
    char buff[100];
    snprintf(buff, sizeof(buff), "L%d", lid.raw());
    if (it != locks.begin())
      res += ", ";
    res += buff;
  }
  return res;
}

// -------- FixedArray--------------- {{{1
template <typename T, size_t SizeLimit = 1024>
class FixedArray {
 public:
  explicit INLINE FixedArray(size_t array_size)
      : size_(array_size),
        array_((array_size <= SizeLimit
                ? alloc_space_
                : new T[array_size])) { }

  ~FixedArray() {
    if (array_ != alloc_space_) {
      delete[] array_;
    }
  }

  T* begin() { return array_; }
  T& operator[](int i)             { return array_[i]; }

 private:
  const size_t size_;
  T* array_;
  T alloc_space_[SizeLimit];
};

// -------- LockSet ----------------- {{{1
class LockSet {
 public:
  NOINLINE static LSID Add(LSID lsid, Lock *lock) {
    ScopedMallocCostCenter cc("LockSetAdd");
    LID lid = lock->lid();
    if (lsid.IsEmpty()) {
      // adding to an empty lock set
      G_stats->ls_add_to_empty++;
      return LSID(lid.raw());
    }
    int cache_res;
    if (ls_add_cache_->Lookup(lsid.raw(), lid.raw(), &cache_res)) {
      G_stats->ls_add_cache_hit++;
      return LSID(cache_res);
    }
    LSID res;
    if (lsid.IsSingleton()) {
      LSSet set(lsid.GetSingleton(), lid);
      G_stats->ls_add_to_singleton++;
      res = ComputeId(set);
    } else {
      LSSet set(Get(lsid), lid);
      G_stats->ls_add_to_multi++;
      res = ComputeId(set);
    }
    ls_add_cache_->Insert(lsid.raw(), lid.raw(), res.raw());
    return res;
  }

  // If lock is present in lsid, set new_lsid to (lsid \ lock) and return true.
  // Otherwise set new_lsid to lsid and return false.
  NOINLINE static bool Remove(LSID lsid, Lock *lock, LSID *new_lsid) {
    *new_lsid = lsid;
    if (lsid.IsEmpty()) return false;
    LID lid = lock->lid();

    if (lsid.IsSingleton()) {
      // removing the only lock -> LSID(0)
      if (lsid.GetSingleton() != lid) return false;
      G_stats->ls_remove_from_singleton++;
      *new_lsid = LSID(0);
      return true;
    }

    int cache_res;
    if (ls_rem_cache_->Lookup(lsid.raw(), lid.raw(), &cache_res)) {
      G_stats->ls_rem_cache_hit++;
      *new_lsid = LSID(cache_res);
      return true;
    }

    LSSet &prev_set = Get(lsid);
    if (!prev_set.has(lid)) return false;
    LSSet set(prev_set, LSSet::REMOVE, lid);
    CHECK(set.size() == prev_set.size() - 1);
    G_stats->ls_remove_from_multi++;
    LSID res = ComputeId(set);
    ls_rem_cache_->Insert(lsid.raw(), lid.raw(), res.raw());
    *new_lsid = res;
    return true;
  }

  NOINLINE static bool IntersectionIsEmpty(LSID lsid1, LSID lsid2) {
    // at least one empty
    if (lsid1.IsEmpty() || lsid2.IsEmpty())
      return true;  // empty

    // both singletons
    if (lsid1.IsSingleton() && lsid2.IsSingleton()) {
      return lsid1 != lsid2;
    }

    // first is singleton, second is not
    if (lsid1.IsSingleton()) {
      const LSSet &set2 = Get(lsid2);
      return set2.has(LID(lsid1.raw())) == false;
    }

    // second is singleton, first is not
    if (lsid2.IsSingleton()) {
      const LSSet &set1 = Get(lsid1);
      return set1.has(LID(lsid2.raw())) == false;
    }

    // LockSets are equal and not empty
    if (lsid1 == lsid2)
      return false;

    // both are not singletons - slow path.
    bool ret = true,
         cache_hit = false;
    DCHECK(lsid2.raw() < 0);
    if (ls_intersection_cache_->Lookup(lsid1.raw(), -lsid2.raw(), &ret)) {
      if (!DEBUG_MODE)
        return ret;
      cache_hit = true;
    }
    const LSSet &set1 = Get(lsid1);
    const LSSet &set2 = Get(lsid2);

    FixedArray<LID> intersection(min(set1.size(), set2.size()));
    LID *end = set_intersection(set1.begin(), set1.end(),
                            set2.begin(), set2.end(),
                            intersection.begin());
    DCHECK(!cache_hit || (ret == (end == intersection.begin())));
    ret = (end == intersection.begin());
    ls_intersection_cache_->Insert(lsid1.raw(), -lsid2.raw(), ret);
    return ret;
  }

  static bool HasNonPhbLocks(LSID lsid) {
    if (lsid.IsEmpty())
      return false;
    if (lsid.IsSingleton())
      return !Lock::LIDtoLock(LID(lsid.raw()))->is_pure_happens_before();

    LSSet &set = Get(lsid);
    for (LSSet::const_iterator it = set.begin(); it != set.end(); ++it)
      if (!Lock::LIDtoLock(*it)->is_pure_happens_before())
        return true;
    return false;
  }

  static string ToString(LSID lsid) {
    if (lsid.IsEmpty()) {
      return "{}";
    } else if (lsid.IsSingleton()) {
      return "{" + Lock::ToString(lsid.GetSingleton()) + "}";
    }
    const LSSet &set = Get(lsid);
    string res = "{";
    for (LSSet::const_iterator it = set.begin(); it != set.end(); ++it) {
      if (it != set.begin()) res += ", ";
      res += Lock::ToString(*it);
    }
    res += "}";
    return res;
  }

  static void ReportLockSetWithContexts(LSID lsid,
                                        set<LID> *locks_reported,
                                        const char *descr) {
    if (lsid.IsEmpty()) return;
    Report("%s%s%s\n", c_green, descr, c_default);
    if (lsid.IsSingleton()) {
      LID lid = lsid.GetSingleton();
      Lock::ReportLockWithOrWithoutContext(lid,
                                           locks_reported->count(lid) == 0);
      locks_reported->insert(lid);
    } else {
      const LSSet &set = Get(lsid);
      for (LSSet::const_iterator it = set.begin(); it != set.end(); ++it) {
        LID lid = *it;
        Lock::ReportLockWithOrWithoutContext(lid,
                                     locks_reported->count(lid) == 0);
        locks_reported->insert(lid);
      }
    }
  }

  static void AddLocksToSet(LSID lsid, set<LID> *locks) {
    if (lsid.IsEmpty()) return;
    if (lsid.IsSingleton()) {
      locks->insert(lsid.GetSingleton());
    } else {
      const LSSet &set = Get(lsid);
      for (LSSet::const_iterator it = set.begin(); it != set.end(); ++it) {
        locks->insert(*it);
      }
    }
  }


  static void InitClassMembers() {
    map_ = new LockSet::Map;
    vec_ = new LockSet::Vec;
    ls_add_cache_ = new LSCache;
    ls_rem_cache_ = new LSCache;
    ls_rem_cache_ = new LSCache;
    ls_intersection_cache_ = new LSIntersectionCache;
  }

 private:
  // No instances are allowed.
  LockSet() { }

  typedef DenseMultimap<LID, 3> LSSet;

  static LSSet &Get(LSID lsid) {
    ScopedMallocCostCenter cc(__FUNCTION__);
    int idx = -lsid.raw() - 1;
    DCHECK(idx >= 0);
    DCHECK(idx < static_cast<int>(vec_->size()));
    return (*vec_)[idx];
  }

  static LSID ComputeId(const LSSet &set) {
    CHECK(set.size() > 0);
    if (set.size() == 1) {
      // signleton lock set has lsid == lid.
      return LSID(set.begin()->raw());
    }
    DCHECK(map_);
    DCHECK(vec_);
    // multiple locks.
    ScopedMallocCostCenter cc("LockSet::ComputeId");
    int32_t *id = &(*map_)[set];
    if (*id == 0) {
      vec_->push_back(set);
      *id = map_->size();
      if      (set.size() == 2) G_stats->ls_size_2++;
      else if (set.size() == 3) G_stats->ls_size_3++;
      else if (set.size() == 4) G_stats->ls_size_4++;
      else if (set.size() == 5) G_stats->ls_size_5++;
      else                      G_stats->ls_size_other++;
      if (*id >= 4096 && ((*id & (*id - 1)) == 0)) {
        Report("INFO: %d LockSet IDs have been allocated "
               "(2: %ld 3: %ld 4: %ld 5: %ld o: %ld)\n",
               *id,
               G_stats->ls_size_2, G_stats->ls_size_3,
               G_stats->ls_size_4, G_stats->ls_size_5,
               G_stats->ls_size_other
               );
      }
    }
    return LSID(-*id);
  }

  typedef map<LSSet, int32_t> Map;
  static Map *map_;

  static const char *kLockSetVecAllocCC;
  typedef vector<LSSet> Vec;
  static Vec *vec_;

//  static const int kPrimeSizeOfLsCache = 307;
//  static const int kPrimeSizeOfLsCache = 499;
  static const int kPrimeSizeOfLsCache = 1021;
  typedef IntPairToIntCache<kPrimeSizeOfLsCache> LSCache;
  static LSCache *ls_add_cache_;
  static LSCache *ls_rem_cache_;
  static LSCache *ls_int_cache_;
  typedef IntPairToBoolCache<kPrimeSizeOfLsCache> LSIntersectionCache;
  static LSIntersectionCache *ls_intersection_cache_;
};

LockSet::Map *LockSet::map_;
LockSet::Vec *LockSet::vec_;
const char *LockSet::kLockSetVecAllocCC = "kLockSetVecAllocCC";
LockSet::LSCache *LockSet::ls_add_cache_;
LockSet::LSCache *LockSet::ls_rem_cache_;
LockSet::LSCache *LockSet::ls_int_cache_;
LockSet::LSIntersectionCache *LockSet::ls_intersection_cache_;


static string TwoLockSetsToString(LSID rd_lockset, LSID wr_lockset) {
  string res;
  if (rd_lockset == wr_lockset) {
    res = "L";
    res += LockSet::ToString(wr_lockset);
  } else {
    res = "WR-L";
    res += LockSet::ToString(wr_lockset);
    res += "/RD-L";
    res += LockSet::ToString(rd_lockset);
  }
  return res;
}




// -------- VTS ------------------ {{{1
class VTS {
 public:
  static size_t MemoryRequiredForOneVts(size_t size) {
    return sizeof(VTS) + size * sizeof(TS);
  }

  static size_t RoundUpSizeForEfficientUseOfFreeList(size_t size) {
    if (size < 32) return size;
    if (size < 64) return (size + 7) & ~7;
    if (size < 128) return (size + 15) & ~15;
    return (size + 31) & ~31;
  }

  static VTS *Create(size_t size) {
    DCHECK(size > 0);
    void *mem;
    size_t rounded_size = RoundUpSizeForEfficientUseOfFreeList(size);
    DCHECK(size <= rounded_size);
    if (rounded_size <= kNumberOfFreeLists) {
      // Small chunk, use FreeList.
      ScopedMallocCostCenter cc("VTS::Create (from free list)");
      mem = free_lists_[rounded_size]->Allocate();
      G_stats->vts_create_small++;
    } else {
      // Large chunk, use new/delete instead of FreeList.
      ScopedMallocCostCenter cc("VTS::Create (from new[])");
      mem = new int8_t[MemoryRequiredForOneVts(size)];
      G_stats->vts_create_big++;
    }
    VTS *res = new(mem) VTS(size);
    G_stats->vts_total_create += size;
    return res;
  }

  static void Unref(VTS *vts) {
    if (!vts) return;
    CHECK_GT(vts->ref_count_, 0);
    if (AtomicDecrementRefcount(&vts->ref_count_) == 0) {
      size_t size = vts->size_;  // can't use vts->size().
      size_t rounded_size = RoundUpSizeForEfficientUseOfFreeList(size);
      if (rounded_size <= kNumberOfFreeLists) {
        free_lists_[rounded_size]->Deallocate(vts);
        G_stats->vts_delete_small++;
      } else {
        G_stats->vts_delete_big++;
        delete vts;
      }
      G_stats->vts_total_delete += rounded_size;
    }
  }

  static VTS *CreateSingleton(TID tid, int32_t clk = 1) {
    VTS *res = Create(1);
    res->arr_[0].tid = tid.raw();
    res->arr_[0].clk = clk;
    return res;
  }

  VTS *Clone() {
    G_stats->vts_clone++;
    AtomicIncrementRefcount(&ref_count_);
    return this;
  }

  static VTS *CopyAndTick(const VTS *vts, TID id_to_tick) {
    CHECK(vts->ref_count_);
    VTS *res = Create(vts->size());
    bool found = false;
    for (size_t i = 0; i < res->size(); i++) {
      res->arr_[i] = vts->arr_[i];
      if (res->arr_[i].tid == id_to_tick.raw()) {
        res->arr_[i].clk++;
        found = true;
      }
    }
    CHECK(found);
    return res;
  }

  static VTS *Join(const VTS *vts_a, const VTS *vts_b) {
    CHECK(vts_a->ref_count_);
    CHECK(vts_b->ref_count_);
    FixedArray<TS> result_ts(vts_a->size() + vts_b->size());
    TS *t = result_ts.begin();
    const TS *a = &vts_a->arr_[0];
    const TS *b = &vts_b->arr_[0];
    const TS *a_max = a + vts_a->size();
    const TS *b_max = b + vts_b->size();
    while (a < a_max && b < b_max) {
      if (a->tid < b->tid) {
        *t = *a;
        a++;
        t++;
      } else if (a->tid > b->tid) {
        *t = *b;
        b++;
        t++;
      } else {
        if (a->clk >= b->clk) {
          *t = *a;
        } else {
          *t = *b;
        }
        a++;
        b++;
        t++;
      }
    }
    while (a < a_max) {
      *t = *a;
      a++;
      t++;
    }
    while (b < b_max) {
      *t = *b;
      b++;
      t++;
    }

    VTS *res = VTS::Create(t - result_ts.begin());
    for (size_t i = 0; i < res->size(); i++) {
      res->arr_[i] = result_ts[i];
    }
    return res;
  }

  int32_t clk(TID tid) const {
    // TODO(dvyukov): this function is sub-optimal,
    // we only need thread's own clock.
    for (size_t i = 0; i < size_; i++) {
      if (arr_[i].tid == tid.raw()) {
        return arr_[i].clk;
      }
    }
    return 0;
  }

  static INLINE void FlushHBCache() {
    hb_cache_->Flush();
  }

  static INLINE bool HappensBeforeCached(const VTS *vts_a, const VTS *vts_b) {
    bool res = false;
    if (hb_cache_->Lookup(vts_a->uniq_id_, vts_b->uniq_id_, &res)) {
      G_stats->n_vts_hb_cached++;
      DCHECK(res == HappensBefore(vts_a, vts_b));
      return res;
    }
    res = HappensBefore(vts_a, vts_b);
    hb_cache_->Insert(vts_a->uniq_id_, vts_b->uniq_id_, res);
    return res;
  }

  // return true if vts_a happens-before vts_b.
  static NOINLINE bool HappensBefore(const VTS *vts_a, const VTS *vts_b) {
    CHECK(vts_a->ref_count_);
    CHECK(vts_b->ref_count_);
    G_stats->n_vts_hb++;
    const TS *a = &vts_a->arr_[0];
    const TS *b = &vts_b->arr_[0];
    const TS *a_max = a + vts_a->size();
    const TS *b_max = b + vts_b->size();
    bool a_less_than_b = false;
    while (a < a_max && b < b_max) {
      if (a->tid < b->tid) {
        // a->tid is not present in b.
        return false;
      } else if (a->tid > b->tid) {
        // b->tid is not present in a.
        a_less_than_b = true;
        b++;
      } else {
        // this tid is present in both VTSs. Compare clocks.
        if (a->clk > b->clk) return false;
        if (a->clk < b->clk) a_less_than_b = true;
        a++;
        b++;
      }
    }
    if (a < a_max) {
      // Some tids are present in a and not in b
      return false;
    }
    if (b < b_max) {
      return true;
    }
    return a_less_than_b;
  }

  size_t size() const {
    DCHECK(ref_count_);
    return size_;
  }

  string ToString() const {
    DCHECK(ref_count_);
    string res = "[";
    for (size_t i = 0; i < size(); i++) {
      char buff[100];
      snprintf(buff, sizeof(buff), "%d:%d;", arr_[i].tid, arr_[i].clk);
      if (i) res += " ";
      res += buff;
    }
    return res + "]";
  }

  void print(const char *name) const {
    string str = ToString();
    Printf("%s: %s\n", name, str.c_str());
  }

  static void TestHappensBefore() {
    // TODO(kcc): need more tests here...
    const char *test_vts[] = {
      "[0:1;]",
      "[0:4; 2:1;]",
      "[0:4; 2:2; 4:1;]",
      "[0:4; 3:2; 4:1;]",
      "[0:4; 3:2; 4:2;]",
      "[0:4; 3:3; 4:1;]",
      NULL
    };

    for (int i = 0; test_vts[i]; i++) {
      const VTS *vts1 = Parse(test_vts[i]);
      for (int j = 0; test_vts[j]; j++) {
        const VTS *vts2 = Parse(test_vts[j]);
        bool hb  = HappensBefore(vts1, vts2);
        Printf("HB = %d\n   %s\n   %s\n", static_cast<int>(hb),
               vts1->ToString().c_str(),
               vts2->ToString().c_str());
        delete vts2;
      }
      delete vts1;
    }
  }

  static void Test() {
    Printf("VTS::test();\n");
    VTS *v1 = CreateSingleton(TID(0));
    VTS *v2 = CreateSingleton(TID(1));
    VTS *v3 = CreateSingleton(TID(2));
    VTS *v4 = CreateSingleton(TID(3));

    VTS *v12 = Join(v1, v2);
    v12->print("v12");
    VTS *v34 = Join(v3, v4);
    v34->print("v34");

    VTS *x1 = Parse("[0:4; 3:6; 4:2;]");
    CHECK(x1);
    x1->print("x1");
    TestHappensBefore();
  }

  // Parse VTS string in the form "[0:4; 3:6; 4:2;]".
  static VTS *Parse(const char *str) {
#if 1  // TODO(kcc): need sscanf in valgrind
    return NULL;
#else
    vector<TS> vec;
    if (!str) return NULL;
    if (str[0] != '[') return NULL;
    str++;
    int tid = 0, clk = 0;
    int consumed = 0;
    while (sscanf(str, "%d:%d;%n", &tid, &clk, &consumed) > 0) {
      TS ts;
      ts.tid = TID(tid);
      ts.clk = clk;
      vec.push_back(ts);
      str += consumed;
      // Printf("%d:%d\n", tid, clk);
    }
    if (*str != ']') return NULL;
    VTS *res = Create(vec.size());
    for (size_t i = 0; i < vec.size(); i++) {
      res->arr_[i] = vec[i];
    }
    return res;
#endif
  }

  static void InitClassMembers() {
    hb_cache_ = new HBCache;
    free_lists_ = new FreeList *[kNumberOfFreeLists+1];
    free_lists_[0] = 0;
    for (size_t  i = 1; i <= kNumberOfFreeLists; i++) {
      free_lists_[i] = new FreeList(MemoryRequiredForOneVts(i),
                                    (kNumberOfFreeLists * 4) / i);
    }
  }

  int32_t uniq_id() const { return uniq_id_; }

 private:
  explicit VTS(size_t size)
    : ref_count_(1),
      size_(size) {
    uniq_id_counter_++;
    // If we've got overflow, we are in trouble, need to have 64-bits...
    CHECK_GT(uniq_id_counter_, 0);
    uniq_id_ = uniq_id_counter_;
  }
  ~VTS() {}

  struct TS {
    int32_t tid;
    int32_t clk;
  };


  // data members
  int32_t ref_count_;
  int32_t uniq_id_;
  size_t size_;
  TS     arr_[];  // array of size_ elements.


  // static data members
  static int32_t uniq_id_counter_;
  static const int kCacheSize = 4999;  // Has to be prime.
  typedef IntPairToBoolCache<kCacheSize> HBCache;
  static HBCache *hb_cache_;

  static const size_t kNumberOfFreeLists = 512;  // Must be power of two.
//  static const size_t kNumberOfFreeLists = 64; // Must be power of two.
  static FreeList **free_lists_;  // Array of kNumberOfFreeLists elements.
};

int32_t VTS::uniq_id_counter_;
VTS::HBCache *VTS::hb_cache_;
FreeList **VTS::free_lists_;


// This class is somewhat similar to VTS,
// but it's mutable, not reference counted and not sorted.
class VectorClock {
 public:
  VectorClock()
      : size_(),
        clock_()
  {
  }

  void reset() {
    free(clock_);
    size_ = 0;
    clock_ = NULL;
  }

  int32_t clock(TID tid) const {
    for (size_t i = 0; i != size_; i += 1) {
      if (clock_[i].tid == tid.raw()) {
        return clock_[i].clk;
      }
    }
    return 0;
  }

  void update(TID tid, int32_t clk) {
    for (size_t i = 0; i != size_; i += 1) {
      if (clock_[i].tid == tid.raw()) {
        clock_[i].clk = clk;
        return;
      }
    }
    size_ += 1;
    clock_ = (TS*)realloc(clock_, size_ * sizeof(TS));
    clock_[size_ - 1].tid = tid.raw();
    clock_[size_ - 1].clk = clk;
  }

 private:
  struct TS {
    int32_t tid;
    int32_t clk;
  };

  size_t    size_;
  TS*       clock_;
};


// -------- Mask -------------------- {{{1
// A bit mask (32-bits on 32-bit arch and 64-bits on 64-bit arch).
class Mask {
 public:
  static const uintptr_t kOne = 1;
  static const uintptr_t kNBits = sizeof(uintptr_t) * 8;
  static const uintptr_t kNBitsLog = kNBits == 32 ? 5 : 6;

  Mask() : m_(0) {}
  Mask(const Mask &m) : m_(m.m_) { }
  explicit Mask(uintptr_t m) : m_(m) { }
  INLINE bool Get(uintptr_t idx) const   { return m_ & (kOne << idx); }
  INLINE void Set(uintptr_t idx)   { m_ |= kOne << idx; }
  INLINE void Clear(uintptr_t idx) { m_ &= ~(kOne << idx); }
  INLINE bool Empty() const {return m_ == 0; }

  // Clear bits in range [a,b) and return old [a,b) range.
  INLINE Mask ClearRangeAndReturnOld(uintptr_t a, uintptr_t b) {
    DCHECK(a < b);
    DCHECK(b <= kNBits);
    uintptr_t res;
    uintptr_t n_bits_in_mask = (b - a);
    if (n_bits_in_mask == kNBits) {
      res = m_;
      m_ = 0;
    } else {
      uintptr_t t = (kOne << n_bits_in_mask);
      uintptr_t mask = (t - 1) << a;
      res = m_ & mask;
      m_ &= ~mask;
    }
    return Mask(res);
  }

  INLINE void ClearRange(uintptr_t a, uintptr_t b) {
    ClearRangeAndReturnOld(a, b);
  }

  INLINE void SetRange(uintptr_t a, uintptr_t b) {
    DCHECK(a < b);
    DCHECK(b <= kNBits);
    uintptr_t n_bits_in_mask = (b - a);
    if (n_bits_in_mask == kNBits) {
      m_ = ~0;
    } else {
      uintptr_t t = (kOne << n_bits_in_mask);
      uintptr_t mask = (t - 1) << a;
      m_ |= mask;
    }
  }

  INLINE uintptr_t GetRange(uintptr_t a, uintptr_t b) const {
    // a bug was fixed here
    DCHECK(a < b);
    DCHECK(b <= kNBits);
    uintptr_t n_bits_in_mask = (b - a);
    if (n_bits_in_mask == kNBits) {
      return m_;
    } else {
      uintptr_t t = (kOne << n_bits_in_mask);
      uintptr_t mask = (t - 1) << a;
      return m_ & mask;
    }
  }

  // Get index of some set bit (asumes mask is non zero).
  size_t GetSomeSetBit() {
    DCHECK(m_);
    size_t ret;
#ifdef __GNUC__
    ret =  __builtin_ctzl(m_);
#elif defined(_MSC_VER)
    unsigned long index;
    DCHECK(sizeof(uintptr_t) == 4);
    _BitScanReverse(&index, m_);
    ret = index;
#else
# error "Unsupported"
#endif
    DCHECK(this->Get(ret));
    return ret;
  }

  size_t PopCount() {
#ifdef VGO_linux
    return __builtin_popcountl(m_);
#else
    CHECK(0);
    return 0;
#endif
  }

  void Subtract(Mask m) { m_ &= ~m.m_; }
  void Union(Mask m) { m_ |= m.m_; }

  static Mask Intersection(Mask m1, Mask m2) { return Mask(m1.m_ & m2.m_); }


  void Clear() { m_ = 0; }


  string ToString() const {
    char buff[kNBits+1];
    for (uintptr_t i = 0; i < kNBits; i++) {
      buff[i] = Get(i) ? '1' : '0';
    }
    buff[kNBits] = 0;
    return buff;
  }

  static void Test() {
    Mask m;
    m.Set(2);
    Printf("%s\n", m.ToString().c_str());
    m.ClearRange(0, kNBits);
    Printf("%s\n", m.ToString().c_str());
  }

 private:
  uintptr_t m_;
};

// -------- BitSet -------------------{{{1
// Poor man's sparse bit set.
class BitSet {
 public:
  // Add range [a,b). The range should be within one line (kNBitsLog).
  void Add(uintptr_t a, uintptr_t b) {
    uintptr_t line = a & ~(Mask::kNBits - 1);
    DCHECK(a < b);
    DCHECK(a - line < Mask::kNBits);
    if (!(b - line <= Mask::kNBits)) {
      Printf("XXXXX %p %p %p b-line=%ld size=%ld a-line=%ld\n", a, b, line,
             b - line, b - a, a - line);
      return;
    }
    DCHECK(b - line <= Mask::kNBits);
    DCHECK(line == ((b - 1) & ~(Mask::kNBits - 1)));
    Mask &mask= map_[line];
    mask.SetRange(a - line, b - line);
  }

  bool empty() { return map_.empty(); }

  size_t size() {
    size_t res = 0;
    for (Map::iterator it = map_.begin(); it != map_.end(); ++it) {
      res += it->second.PopCount();
    }
    return res;
  }

  string ToString() {
    char buff[100];
    string res;
    int lines = 0;
    snprintf(buff, sizeof(buff), " %ld lines %ld bits:",
             (long)map_.size(), (long)size());
    res += buff;
    for (Map::iterator it = map_.begin(); it != map_.end(); ++it) {
      Mask mask = it->second;
      snprintf(buff, sizeof(buff), " l%d (%ld):", lines++, (long)mask.PopCount());
      res += buff;
      uintptr_t line = it->first;
      bool is_in = false;
      for (size_t i = 0; i < Mask::kNBits; i++) {
        uintptr_t addr = line + i;
        if (mask.Get(i)) {
          if (!is_in) {
            snprintf(buff, sizeof(buff), " [%lx,", (long)addr);
            res += buff;
            is_in = true;
          }
        } else {
          if (is_in) {
            snprintf(buff, sizeof(buff), "%lx);", (long)addr);
            res += buff;
            is_in = false;
          }
        }
      }
      if (is_in) {
        snprintf(buff, sizeof(buff), "%lx);", (long)(line + Mask::kNBits));
        res += buff;
      }
    }
    return res;
  }

  void Clear() { map_.clear(); }
 private:
  typedef map<uintptr_t, Mask> Map;
  Map map_;
};

// -------- Segment -------------------{{{1
class Segment {
 public:
  // for debugging...
  static bool ProfileSeg(SID sid) {
    // return (sid.raw() % (1 << 14)) == 0;
    return false;
  }

  // non-static methods

  VTS *vts() const { return vts_; }
  TID tid() const { return TID(tid_); }
  LSID  lsid(bool is_w) const { return lsid_[is_w]; }
  uint32_t lock_era() const { return lock_era_; }

  // static methods

  static INLINE uintptr_t *embedded_stack_trace(SID sid) {
    DCHECK(sid.valid());
    DCHECK(kSizeOfHistoryStackTrace > 0);
    size_t chunk_idx = (unsigned)sid.raw() / kChunkSizeForStacks;
    size_t idx       = (unsigned)sid.raw() % kChunkSizeForStacks;
    DCHECK(chunk_idx < n_stack_chunks_);
    DCHECK(all_stacks_[chunk_idx] != NULL);
    return &all_stacks_[chunk_idx][idx * kSizeOfHistoryStackTrace];
  }

  static void ensure_space_for_stack_trace(SID sid) {
    ScopedMallocCostCenter malloc_cc(__FUNCTION__);
    DCHECK(sid.valid());
    DCHECK(kSizeOfHistoryStackTrace > 0);
    size_t chunk_idx = (unsigned)sid.raw() / kChunkSizeForStacks;
    DCHECK(chunk_idx < n_stack_chunks_);
    if (all_stacks_[chunk_idx])
      return;
    for (size_t i = 0; i <= chunk_idx; i++) {
      if (all_stacks_[i]) continue;
      all_stacks_[i] = new uintptr_t[
          kChunkSizeForStacks * kSizeOfHistoryStackTrace];
      // we don't clear this memory, it will be clreared later lazily.
      // We also never delete it because it will be used until the very end.
    }
  }

  static string StackTraceString(SID sid) {
    DCHECK(kSizeOfHistoryStackTrace > 0);
    return StackTrace::EmbeddedStackTraceToString(
        embedded_stack_trace(sid), kSizeOfHistoryStackTrace);
  }

  // Allocate `n` fresh segments, put SIDs into `fresh_sids`.
  static INLINE void AllocateFreshSegments(size_t n, SID *fresh_sids) {
    ScopedMallocCostCenter malloc_cc(__FUNCTION__);
    size_t i = 0;
    size_t n_reusable = min(n, reusable_sids_->size());
    // First, allocate from reusable_sids_.
    for (; i < n_reusable; i++) {
      G_stats->seg_reuse++;
      DCHECK(!reusable_sids_->empty());
      SID sid = reusable_sids_->back();
      reusable_sids_->pop_back();
      Segment *seg = GetInternal(sid);
      DCHECK(!seg->seg_ref_count_);
      DCHECK(!seg->vts());
      DCHECK(!seg->tid().valid());
      CHECK(sid.valid());
      if (ProfileSeg(sid)) {
       Printf("Segment: reused SID %d\n", sid.raw());
      }
      fresh_sids[i] = sid;
    }
    // allocate the rest from new sids.
    for (; i < n; i++) {
      G_stats->seg_create++;
      CHECK(n_segments_ < kMaxSID);
      Segment *seg = GetSegmentByIndex(n_segments_);

      // This VTS may not be empty due to ForgetAllState().
      VTS::Unref(seg->vts_);
      seg->vts_ = 0;
      seg->seg_ref_count_ = 0;

      if (ProfileSeg(SID(n_segments_))) {
       Printf("Segment: allocated SID %d\n", n_segments_);
      }

      SID sid = fresh_sids[i] = SID(n_segments_);
      if (kSizeOfHistoryStackTrace > 0) {
        ensure_space_for_stack_trace(sid);
      }
      n_segments_++;
    }
  }

  // Initialize the contents of the given segment.
  static INLINE void SetupFreshSid(SID sid, TID tid, VTS *vts,
                                   LSID rd_lockset, LSID wr_lockset) {
    DCHECK(vts);
    DCHECK(tid.valid());
    DCHECK(sid.valid());
    Segment *seg = GetInternal(sid);
    DCHECK(seg);
    DCHECK(seg->seg_ref_count_ == 0);
    seg->seg_ref_count_ = 0;
    seg->tid_ = tid;
    seg->lsid_[0] = rd_lockset;
    seg->lsid_[1] = wr_lockset;
    seg->vts_ = vts;
    seg->lock_era_ = g_lock_era;
    if (kSizeOfHistoryStackTrace) {
      embedded_stack_trace(sid)[0] = 0;
    }
  }

  static INLINE SID AddNewSegment(TID tid, VTS *vts,
                           LSID rd_lockset, LSID wr_lockset) {
    ScopedMallocCostCenter malloc_cc("Segment::AddNewSegment()");
    SID sid;
    AllocateFreshSegments(1, &sid);
    SetupFreshSid(sid, tid, vts, rd_lockset, wr_lockset);
    return sid;
  }

  static bool Alive(SID sid) {
    Segment *seg = GetInternal(sid);
    return seg->vts() != NULL;
  }

  static void AssertLive(SID sid, int line) {
    if (DEBUG_MODE) {
      if (!(sid.raw() < INTERNAL_ANNOTATE_UNPROTECTED_READ(n_segments_))) {
        Printf("Segment::AssertLive: failed on sid=%d n_segments = %dline=%d\n",
               sid.raw(), n_segments_, line);
      }
      Segment *seg = GetInternal(sid);
      if (!seg->vts()) {
        Printf("Segment::AssertLive: failed on sid=%d line=%d\n",
               sid.raw(), line);
      }
      DCHECK(seg->vts());
      DCHECK(seg->tid().valid());
    }
  }

  static INLINE Segment *Get(SID sid) {
    AssertLive(sid, __LINE__);
    Segment *res = GetInternal(sid);
    DCHECK(res->vts());
    DCHECK(res->tid().valid());
    return res;
  }

  static INLINE void RecycleOneFreshSid(SID sid) {
    Segment *seg = GetInternal(sid);
    seg->tid_ = TID();
    seg->vts_ = NULL;
    reusable_sids_->push_back(sid);
    if (ProfileSeg(sid)) {
      Printf("Segment: recycled SID %d\n", sid.raw());
    }
  }

  static bool RecycleOneSid(SID sid) {
    ScopedMallocCostCenter malloc_cc("Segment::RecycleOneSid()");
    Segment *seg = GetInternal(sid);
    DCHECK(seg->seg_ref_count_ == 0);
    DCHECK(sid.raw() < n_segments_);
    if (!seg->vts()) return false;  // Already recycled.
    VTS::Unref(seg->vts_);
    RecycleOneFreshSid(sid);
    return true;
  }

  int32_t ref_count() const {
    return INTERNAL_ANNOTATE_UNPROTECTED_READ(seg_ref_count_);
  }

  static void INLINE Ref(SID sid, const char *where) {
    Segment *seg = GetInternal(sid);
    if (ProfileSeg(sid)) {
      Printf("SegRef   : %d ref=%d %s; tid=%d\n", sid.raw(),
             seg->seg_ref_count_, where, seg->tid().raw());
    }
    DCHECK(seg->seg_ref_count_ >= 0);
    AtomicIncrementRefcount(&seg->seg_ref_count_);
  }

  static INLINE intptr_t UnrefNoRecycle(SID sid, const char *where) {
    Segment *seg = GetInternal(sid);
    if (ProfileSeg(sid)) {
      Printf("SegUnref : %d ref=%d %s\n", sid.raw(), seg->seg_ref_count_, where);
    }
    DCHECK(seg->seg_ref_count_ > 0);
    return AtomicDecrementRefcount(&seg->seg_ref_count_);
  }

  static void INLINE Unref(SID sid, const char *where) {
    if (UnrefNoRecycle(sid, where) == 0) {
      RecycleOneSid(sid);
    }
  }


  static void ForgetAllState() {
    n_segments_ = 1;
    reusable_sids_->clear();
    // vts_'es will be freed in AddNewSegment.
  }

  static string ToString(SID sid) {
    char buff[100];
    snprintf(buff, sizeof(buff), "T%d/S%d", Get(sid)->tid().raw(), sid.raw());
    return buff;
  }

  static string ToStringTidOnly(SID sid) {
    char buff[100];
    snprintf(buff, sizeof(buff), "T%d", Get(sid)->tid().raw());
    return buff;
  }

  static string ToStringWithLocks(SID sid) {
    char buff[100];
    Segment *seg = Get(sid);
    snprintf(buff, sizeof(buff), "T%d/S%d ", seg->tid().raw(), sid.raw());
    string res = buff;
    res += TwoLockSetsToString(seg->lsid(false), seg->lsid(true));
    return res;
  }

  static bool INLINE HappensBeforeOrSameThread(SID a, SID b) {
    if (a == b) return true;
    if (Get(a)->tid() == Get(b)->tid()) return true;
    return HappensBefore(a, b);
  }

  static bool INLINE HappensBefore(SID a, SID b) {
    DCHECK(a != b);
    G_stats->n_seg_hb++;
    bool res = false;
    const Segment *seg_a = Get(a);
    const Segment *seg_b = Get(b);
    DCHECK(seg_a->tid() != seg_b->tid());
    const VTS *vts_a = seg_a->vts();
    const VTS *vts_b = seg_b->vts();
    res = VTS::HappensBeforeCached(vts_a, vts_b);
#if 0
    if (DEBUG_MODE) {
      Printf("HB = %d\n  %s\n  %s\n", res,
           vts_a->ToString().c_str(), vts_b->ToString().c_str());
    }
#endif
    return res;
  }

  static int32_t NumberOfSegments() { return n_segments_; }

  static void ShowSegmentStats() {
    Printf("Segment::ShowSegmentStats:\n");
    Printf("n_segments_: %d\n", n_segments_);
    Printf("reusable_sids_: %ld\n", reusable_sids_->size());
    map<int, int> ref_to_freq_map;
    for (int i = 1; i < n_segments_; i++) {
      Segment *seg = GetInternal(SID(i));
      int32_t refcount = seg->seg_ref_count_;
      if (refcount > 10) refcount = 10;
      ref_to_freq_map[refcount]++;
    }
    for (map<int, int>::iterator it = ref_to_freq_map.begin();
         it != ref_to_freq_map.end(); ++it) {
      Printf("ref %d => freq %d\n", it->first, it->second);
    }
  }

  static void InitClassMembers() {
    if (G_flags->keep_history == 0)
      kSizeOfHistoryStackTrace = 0;
    Report("INFO: Allocating %ldMb (%ld * %ldM) for Segments.\n",
           (sizeof(Segment) * kMaxSID) >> 20,
           sizeof(Segment), kMaxSID >> 20);
    if (kSizeOfHistoryStackTrace) {
      Report("INFO: Will allocate up to %ldMb for 'previous' stack traces.\n",
             (kSizeOfHistoryStackTrace * sizeof(uintptr_t) * kMaxSID) >> 20);
    }

    all_segments_  = new Segment[kMaxSID];
    // initialization all segments to 0.
    memset(all_segments_, 0, kMaxSID * sizeof(Segment));
    // initialize all_segments_[0] with garbage
    memset(all_segments_, -1, sizeof(Segment));

    if (kSizeOfHistoryStackTrace > 0) {
      n_stack_chunks_ = kMaxSID / kChunkSizeForStacks;
      if (n_stack_chunks_ * kChunkSizeForStacks < (size_t)kMaxSID)
        n_stack_chunks_++;
      all_stacks_ = new uintptr_t*[n_stack_chunks_];
      memset(all_stacks_, 0, sizeof(uintptr_t*) * n_stack_chunks_);
    }
    n_segments_    = 1;
    reusable_sids_ = new vector<SID>;
  }

 private:
  static INLINE Segment *GetSegmentByIndex(int32_t index) {
    return &all_segments_[index];
  }
  static INLINE Segment *GetInternal(SID sid) {
    DCHECK(sid.valid());
    DCHECK(sid.raw() < INTERNAL_ANNOTATE_UNPROTECTED_READ(n_segments_));
    Segment *res = GetSegmentByIndex(sid.raw());
    return res;
  }

  // Data members.
  int32_t seg_ref_count_;
  LSID     lsid_[2];
  TID      tid_;
  uint32_t lock_era_;
  VTS *vts_;

  // static class members.

  // One large array of segments. The size is set by a command line (--max-sid)
  // and never changes. Once we are out of vacant segments, we flush the state.
  static Segment *all_segments_;
  // We store stack traces separately because their size is unknown
  // at compile time and because they are needed less often.
  // The stacks are stored as an array of chunks, instead of one array, 
  // so that for small tests we do not require too much RAM.
  // We don't use vector<> or another resizable array to avoid expensive 
  // resizing.
  enum { kChunkSizeForStacks = DEBUG_MODE ? 512 : 1 * 1024 * 1024 };
  static uintptr_t **all_stacks_;
  static size_t      n_stack_chunks_;

  static int32_t n_segments_;
  static vector<SID> *reusable_sids_;
};

Segment          *Segment::all_segments_;
uintptr_t       **Segment::all_stacks_;
size_t            Segment::n_stack_chunks_;
int32_t           Segment::n_segments_;
vector<SID>      *Segment::reusable_sids_;

// -------- SegmentSet -------------- {{{1
class SegmentSet {
 public:
  static NOINLINE SSID AddSegmentToSS(SSID old_ssid, SID new_sid);
  static NOINLINE SSID RemoveSegmentFromSS(SSID old_ssid, SID sid_to_remove);

  static INLINE SSID AddSegmentToTupleSS(SSID ssid, SID new_sid);
  static INLINE SSID RemoveSegmentFromTupleSS(SSID old_ssid, SID sid_to_remove);

  SSID ComputeSSID() {
    SSID res = map_->GetIdOrZero(this);
    CHECK_NE(res.raw(), 0);
    return res;
  }

  int ref_count() const { return ref_count_; }

  static void AssertLive(SSID ssid, int line) {
    DCHECK(ssid.valid());
    if (DEBUG_MODE) {
      if (ssid.IsSingleton()) {
        Segment::AssertLive(ssid.GetSingleton(), line);
      } else {
        DCHECK(ssid.IsTuple());
        int idx = -ssid.raw()-1;
        DCHECK(idx < static_cast<int>(vec_->size()));
        DCHECK(idx >= 0);
        SegmentSet *res = (*vec_)[idx];
        DCHECK(res);
        DCHECK(res->ref_count_ >= 0);
        res->Validate(line);

        if (!res) {
          Printf("SegmentSet::AssertLive failed at line %d (ssid=%d)\n",
                 line, ssid.raw());
          DCHECK(0);
        }
      }
    }
  }

  static SegmentSet *Get(SSID ssid) {
    DCHECK(ssid.valid());
    DCHECK(!ssid.IsSingleton());
    int idx = -ssid.raw()-1;
    ANNOTATE_IGNORE_READS_BEGIN();
    DCHECK(idx < static_cast<int>(vec_->size()) && idx >= 0);
    ANNOTATE_IGNORE_READS_END();
    SegmentSet *res = (*vec_)[idx];
    DCHECK(res);
    DCHECK(res->size() >= 2);
    return res;
  }

  void RecycleOneSegmentSet(SSID ssid) {
    DCHECK(ref_count_ == 0);
    DCHECK(ssid.valid());
    DCHECK(!ssid.IsSingleton());
    int idx = -ssid.raw()-1;
    DCHECK(idx < static_cast<int>(vec_->size()) && idx >= 0);
    CHECK((*vec_)[idx] == this);
    // Printf("SegmentSet::RecycleOneSegmentSet: %d\n", ssid.raw());
    //
    // Recycle segments
    for (int i = 0; i < kMaxSegmentSetSize; i++) {
      SID sid = this->GetSID(i);
      if (sid.raw() == 0) break;
      Segment::Unref(sid, "SegmentSet::Recycle");
    }
    ref_count_ = -1;

    map_->Erase(this);
    ready_to_be_reused_->push_back(ssid);
    G_stats->ss_recycle++;
  }

  static void INLINE Ref(SSID ssid, const char *where) {
    AssertTILHeld(); // The reference counting logic below is not thread-safe
    DCHECK(ssid.valid());
    if (ssid.IsSingleton()) {
      Segment::Ref(ssid.GetSingleton(), where);
    } else {
      SegmentSet *sset = Get(ssid);
      // Printf("SSRef   : %d ref=%d %s\n", ssid.raw(), sset->ref_count_, where);
      DCHECK(sset->ref_count_ >= 0);
      sset->ref_count_++;
    }
  }

  static void INLINE Unref(SSID ssid, const char *where) {
    AssertTILHeld(); // The reference counting logic below is not thread-safe
    DCHECK(ssid.valid());
    if (ssid.IsSingleton()) {
      Segment::Unref(ssid.GetSingleton(), where);
    } else {
      SegmentSet *sset = Get(ssid);
      // Printf("SSUnref : %d ref=%d %s\n", ssid.raw(), sset->ref_count_, where);
      DCHECK(sset->ref_count_ > 0);
      sset->ref_count_--;
      if (sset->ref_count_ == 0) {
        // We don't delete unused SSID straightaway due to performance reasons
        // (to avoid flushing caches too often and because SSID may be reused
        // again soon)
        //
        // Instead, we use two queues (deques):
        //    ready_to_be_recycled_ and ready_to_be_reused_.
        // The algorithm is following:
        // 1) When refcount_ becomes zero, we push the SSID into
        //    ready_to_be_recycled_.
        // 2) When ready_to_be_recycled_ becomes too large, we call
        //    FlushRecycleQueue().
        //    In FlushRecycleQueue(), we pop the first half of
        //    ready_to_be_recycled_ and for each popped SSID we do
        //     * if "refcount_ > 0", do nothing (this SSID is in use again)
        //     * otherwise, we recycle this SSID (delete its VTS, etc) and push
        //       it into ready_to_be_reused_
        // 3) When a new SegmentSet is about to be created, we re-use SSID from
        //    ready_to_be_reused_ (if available)
        ready_to_be_recycled_->push_back(ssid);
        if (UNLIKELY(ready_to_be_recycled_->size() >
                     2 * G_flags->segment_set_recycle_queue_size)) {
          FlushRecycleQueue();
        }
      }
    }
  }

  static void FlushRecycleQueue() {
    while (ready_to_be_recycled_->size() >
        G_flags->segment_set_recycle_queue_size) {
      SSID rec_ssid = ready_to_be_recycled_->front();
      ready_to_be_recycled_->pop_front();
      int idx = -rec_ssid.raw()-1;
      SegmentSet *rec_ss = (*vec_)[idx];
      DCHECK(rec_ss);
      DCHECK(rec_ss == Get(rec_ssid));
      // We should check that this SSID haven't been referenced again.
      if (rec_ss->ref_count_ == 0) {
        rec_ss->RecycleOneSegmentSet(rec_ssid);
      }
    }

    // SSIDs will be reused soon - need to flush some caches.
    FlushCaches();
  }

  string ToString() const;
  void Print() {
    Printf("SS%d:%s\n", -ComputeSSID().raw(), ToString().c_str());
  }

  static string ToString(SSID ssid) {
    CHECK(ssid.IsValidOrEmpty());
    if (ssid.IsSingleton()) {
      return "{" +  Segment::ToStringTidOnly(SID(ssid.raw())) + "}";
    } else if (ssid.IsEmpty()) {
      return "{}";
    } else {
      AssertLive(ssid, __LINE__);
      return Get(ssid)->ToString();
    }
  }


  static string ToStringWithLocks(SSID ssid);

  static void FlushCaches() {
    add_segment_cache_->Flush();
    remove_segment_cache_->Flush();
  }

  static void ForgetAllState() {
    for (size_t i = 0; i < vec_->size(); i++) {
      delete (*vec_)[i];
    }
    map_->Clear();
    vec_->clear();
    ready_to_be_reused_->clear();
    ready_to_be_recycled_->clear();
    FlushCaches();
  }


  static void Test();

  static int32_t Size(SSID ssid) {
    if (ssid.IsEmpty()) return 0;
    if (ssid.IsSingleton()) return 1;
    return Get(ssid)->size();
  }

  SID GetSID(int32_t i) const {
    DCHECK(i >= 0 && i < kMaxSegmentSetSize);
    DCHECK(i == 0 || sids_[i-1].raw() != 0);
    return sids_[i];
  }

  void SetSID(int32_t i, SID sid) {
    DCHECK(i >= 0 && i < kMaxSegmentSetSize);
    DCHECK(i == 0 || sids_[i-1].raw() != 0);
    sids_[i] = sid;
  }

  static SID GetSID(SSID ssid, int32_t i, int line) {
    DCHECK(ssid.valid());
    if (ssid.IsSingleton()) {
      DCHECK(i == 0);
      Segment::AssertLive(ssid.GetSingleton(), line);
      return ssid.GetSingleton();
    } else {
      AssertLive(ssid, __LINE__);
      SID sid = Get(ssid)->GetSID(i);
      Segment::AssertLive(sid, line);
      return sid;
    }
  }

  static bool INLINE Contains(SSID ssid, SID seg) {
    if (LIKELY(ssid.IsSingleton())) {
      return ssid.GetSingleton() == seg;
    } else if (LIKELY(ssid.IsEmpty())) {
      return false;
    }

    SegmentSet *ss = Get(ssid);
    for (int i = 0; i < kMaxSegmentSetSize; i++) {
      SID sid = ss->GetSID(i);
      if (sid.raw() == 0) break;
      if (sid == seg)
        return true;
    }
    return false;
  }

  static Segment *GetSegmentForNonSingleton(SSID ssid, int32_t i, int line) {
    return Segment::Get(GetSID(ssid, i, line));
  }

  void NOINLINE Validate(int line) const;

  static size_t NumberOfSegmentSets() { return vec_->size(); }


  static void InitClassMembers() {
    map_    = new Map;
    vec_    = new vector<SegmentSet *>;
    ready_to_be_recycled_ = new deque<SSID>;
    ready_to_be_reused_ = new deque<SSID>;
    add_segment_cache_ = new SsidSidToSidCache;
    remove_segment_cache_ = new SsidSidToSidCache;
  }

 private:
  SegmentSet()  // Private CTOR
    : ref_count_(0) {
    // sids_ are filled with zeroes due to SID default CTOR.
    if (DEBUG_MODE) {
      for (int i = 0; i < kMaxSegmentSetSize; i++)
        CHECK_EQ(sids_[i].raw(), 0);
    }
  }

  int size() const {
    for (int i = 0; i < kMaxSegmentSetSize; i++) {
      if (sids_[i].raw() == 0) {
        CHECK_GE(i, 2);
        return i;
      }
    }
    return kMaxSegmentSetSize;
  }

  static INLINE SSID AllocateAndCopy(SegmentSet *ss) {
    DCHECK(ss->ref_count_ == 0);
    DCHECK(sizeof(int32_t) == sizeof(SID));
    SSID res_ssid;
    SegmentSet *res_ss = 0;

    if (!ready_to_be_reused_->empty()) {
      res_ssid = ready_to_be_reused_->front();
      ready_to_be_reused_->pop_front();
      int idx = -res_ssid.raw()-1;
      res_ss = (*vec_)[idx];
      DCHECK(res_ss);
      DCHECK(res_ss->ref_count_ == -1);
      G_stats->ss_reuse++;
      for (int i = 0; i < kMaxSegmentSetSize; i++) {
        res_ss->sids_[i] = SID(0);
      }
    } else {
      // create a new one
      ScopedMallocCostCenter cc("SegmentSet::CreateNewSegmentSet");
      G_stats->ss_create++;
      res_ss = new SegmentSet;
      vec_->push_back(res_ss);
      res_ssid = SSID(-((int32_t)vec_->size()));
      CHECK(res_ssid.valid());
    }
    DCHECK(res_ss);
    res_ss->ref_count_ = 0;
    for (int i = 0; i < kMaxSegmentSetSize; i++) {
      SID sid = ss->GetSID(i);
      if (sid.raw() == 0) break;
      Segment::Ref(sid, "SegmentSet::FindExistingOrAlocateAndCopy");
      res_ss->SetSID(i, sid);
    }
    DCHECK(res_ss == Get(res_ssid));
    map_->Insert(res_ss, res_ssid);
    return res_ssid;
  }

  static NOINLINE SSID FindExistingOrAlocateAndCopy(SegmentSet *ss) {
    if (DEBUG_MODE) {
      int size = ss->size();
      if (size == 2) G_stats->ss_size_2++;
      if (size == 3) G_stats->ss_size_3++;
      if (size == 4) G_stats->ss_size_4++;
      if (size > 4) G_stats->ss_size_other++;
    }

    // First, check if there is such set already.
    SSID ssid = map_->GetIdOrZero(ss);
    if (ssid.raw() != 0) {  // Found.
      AssertLive(ssid, __LINE__);
      G_stats->ss_find++;
      return ssid;
    }
    // If no such set, create one.
    return AllocateAndCopy(ss);
  }

  static INLINE SSID DoubletonSSID(SID sid1, SID sid2) {
    SegmentSet tmp;
    tmp.SetSID(0, sid1);
    tmp.SetSID(1, sid2);
    return FindExistingOrAlocateAndCopy(&tmp);
  }

  // testing only
  static SegmentSet *AddSegmentToTupleSS(SegmentSet *ss, SID new_sid) {
    SSID ssid = AddSegmentToTupleSS(ss->ComputeSSID(), new_sid);
    AssertLive(ssid, __LINE__);
    return Get(ssid);
  }

  static SegmentSet *Doubleton(SID sid1, SID sid2) {
    SSID ssid = DoubletonSSID(sid1, sid2);
    AssertLive(ssid, __LINE__);
    return Get(ssid);
  }

  // static data members
  struct Less {
    INLINE bool operator() (const SegmentSet *ss1,
                            const SegmentSet *ss2) const {
      for (int i = 0; i < kMaxSegmentSetSize; i++) {
        SID sid1 = ss1->sids_[i],
            sid2 = ss2->sids_[i];
        if (sid1 != sid2) return sid1 < sid2;
      }
      return false;
    }
  };

  struct SSEq {
    INLINE bool operator() (const SegmentSet *ss1,
                            const SegmentSet *ss2) const {
      G_stats->sseq_calls++;

      for (int i = 0; i < kMaxSegmentSetSize; i++) {
        SID sid1 = ss1->sids_[i],
            sid2 = ss2->sids_[i];
        if (sid1 != sid2) return false;
      }
      return true;
    }
  };

  struct SSHash {
    INLINE size_t operator() (const SegmentSet *ss) const {
      uintptr_t res = 0;
      uint32_t* sids_array = (uint32_t*)ss->sids_;
      // We must have even number of SIDs.
      DCHECK((kMaxSegmentSetSize % 2) == 0);

      G_stats->sshash_calls++;
      // xor all SIDs together, half of them bswap-ed.
      for (int i = 0; i < kMaxSegmentSetSize; i += 2) {
        uintptr_t t1 = sids_array[i];
        uintptr_t t2 = sids_array[i+1];
        if (t2) t2 = tsan_bswap(t2);
        res = res ^ t1 ^ t2;
      }
      return res;
    }
  };

  struct SSTraits {
    enum {
      // These values are taken from the hash_compare defaults.
      bucket_size = 4,  // Must be greater than zero.
      min_buckets = 8,  // Must be power of 2.
    };

    INLINE size_t operator()(const SegmentSet *ss) const {
      SSHash sshash;
      return sshash(ss);
    }

    INLINE bool operator()(const SegmentSet *ss1, const SegmentSet *ss2) const {
      Less less;
      return less(ss1, ss2);
    }
  };

  template <class MapType>
  static SSID GetIdOrZeroFromMap(MapType *map, SegmentSet *ss) {
    typename MapType::iterator it = map->find(ss);
    if (it == map->end())
      return SSID(0);
    return it->second;
  }

  class Map {
   public:
    SSID GetIdOrZero(SegmentSet *ss) {
      return GetIdOrZeroFromMap(&map_, ss);
    }

    void Insert(SegmentSet *ss, SSID id) {
      map_[ss] = id;
    }

    void Erase(SegmentSet *ss) {
      CHECK(map_.erase(ss));
    }

    void Clear() {
      map_.clear();
    }

   private:
    // TODO(timurrrr): consider making a custom hash_table.
#if defined(_MSC_VER)
    typedef stdext::hash_map<SegmentSet*, SSID, SSTraits > MapType__;
#elif 1
    typedef unordered_map<SegmentSet*, SSID, SSHash, SSEq > MapType__;
#else
    // Old code, may be useful for debugging.
    typedef map<SegmentSet*, SSID, Less > MapType__;
#endif
    MapType__ map_;
  };

//  typedef map<SegmentSet*, SSID, Less> Map;

  static Map                  *map_;
  // TODO(kcc): use vector<SegmentSet> instead.
  static vector<SegmentSet *> *vec_;
  static deque<SSID>         *ready_to_be_reused_;
  static deque<SSID>         *ready_to_be_recycled_;

  typedef PairCache<SSID, SID, SSID, 1009, 1> SsidSidToSidCache;
  static SsidSidToSidCache    *add_segment_cache_;
  static SsidSidToSidCache    *remove_segment_cache_;

  // sids_ contains up to kMaxSegmentSetSize SIDs.
  // Contains zeros at the end if size < kMaxSegmentSetSize.
  SID     sids_[kMaxSegmentSetSize];
  int32_t ref_count_;
};

SegmentSet::Map      *SegmentSet::map_;
vector<SegmentSet *> *SegmentSet::vec_;
deque<SSID>         *SegmentSet::ready_to_be_reused_;
deque<SSID>         *SegmentSet::ready_to_be_recycled_;
SegmentSet::SsidSidToSidCache    *SegmentSet::add_segment_cache_;
SegmentSet::SsidSidToSidCache    *SegmentSet::remove_segment_cache_;




SSID SegmentSet::RemoveSegmentFromSS(SSID old_ssid, SID sid_to_remove) {
  DCHECK(old_ssid.IsValidOrEmpty());
  DCHECK(sid_to_remove.valid());
  SSID res;
  if (remove_segment_cache_->Lookup(old_ssid, sid_to_remove, &res)) {
    return res;
  }

  if (old_ssid.IsEmpty()) {
    res = old_ssid;  // Nothing to remove.
  } else if (LIKELY(old_ssid.IsSingleton())) {
    SID sid = old_ssid.GetSingleton();
    if (Segment::HappensBeforeOrSameThread(sid, sid_to_remove))
      res = SSID(0);  // Empty.
    else
      res = old_ssid;
  } else {
    res = RemoveSegmentFromTupleSS(old_ssid, sid_to_remove);
  }
  remove_segment_cache_->Insert(old_ssid, sid_to_remove, res);
  return res;
}


// static
//
// This method returns a SSID of a SegmentSet containing "new_sid" and all those
// segments from "old_ssid" which do not happen-before "new_sid".
//
// For details, see
// http://code.google.com/p/data-race-test/wiki/ThreadSanitizerAlgorithm#State_machine
SSID SegmentSet::AddSegmentToSS(SSID old_ssid, SID new_sid) {
  DCHECK(old_ssid.raw() == 0 || old_ssid.valid());
  DCHECK(new_sid.valid());
  Segment::AssertLive(new_sid, __LINE__);
  SSID res;

  // These two TIDs will only be used if old_ssid.IsSingleton() == true.
  TID old_tid;
  TID new_tid;

  if (LIKELY(old_ssid.IsSingleton())) {
    SID old_sid(old_ssid.raw());
    DCHECK(old_sid.valid());
    Segment::AssertLive(old_sid, __LINE__);

    if (UNLIKELY(old_sid == new_sid)) {
      // The new segment equals the old one - nothing has changed.
      return old_ssid;
    }

    old_tid = Segment::Get(old_sid)->tid();
    new_tid = Segment::Get(new_sid)->tid();
    if (LIKELY(old_tid == new_tid)) {
      // The new segment is in the same thread - just replace the SID.
      return SSID(new_sid);
    }

    if (Segment::HappensBefore(old_sid, new_sid)) {
      // The new segment is in another thread, but old segment
      // happens before the new one - just replace the SID.
      return SSID(new_sid);
    }

    DCHECK(!Segment::HappensBefore(new_sid, old_sid));
    // The only other case is Signleton->Doubleton transition, see below.
  } else if (LIKELY(old_ssid.IsEmpty())) {
    return SSID(new_sid);
  }

  // Lookup the cache.
  if (add_segment_cache_->Lookup(old_ssid, new_sid, &res)) {
    SegmentSet::AssertLive(res, __LINE__);
    return res;
  }

  if (LIKELY(old_ssid.IsSingleton())) {
    // Signleton->Doubleton transition.
    // These two TIDs were initialized before cache lookup (see above).
    DCHECK(old_tid.valid());
    DCHECK(new_tid.valid());

    SID old_sid(old_ssid.raw());
    DCHECK(old_sid.valid());

    DCHECK(!Segment::HappensBefore(new_sid, old_sid));
    DCHECK(!Segment::HappensBefore(old_sid, new_sid));
    res = (old_tid < new_tid
      ? DoubletonSSID(old_sid, new_sid)
      : DoubletonSSID(new_sid, old_sid));
    SegmentSet::AssertLive(res, __LINE__);
  } else {
    res = AddSegmentToTupleSS(old_ssid, new_sid);
    SegmentSet::AssertLive(res, __LINE__);
  }

  // Put the result into cache.
  add_segment_cache_->Insert(old_ssid, new_sid, res);

  return res;
}

SSID SegmentSet::RemoveSegmentFromTupleSS(SSID ssid, SID sid_to_remove) {
  DCHECK(ssid.IsTuple());
  DCHECK(ssid.valid());
  AssertLive(ssid, __LINE__);
  SegmentSet *ss = Get(ssid);

  int32_t old_size = 0, new_size = 0;
  SegmentSet tmp;
  SID * tmp_sids = tmp.sids_;
  CHECK(sizeof(int32_t) == sizeof(SID));

  for (int i = 0; i < kMaxSegmentSetSize; i++, old_size++) {
    SID sid = ss->GetSID(i);
    if (sid.raw() == 0) break;
    DCHECK(sid.valid());
    Segment::AssertLive(sid, __LINE__);
    if (Segment::HappensBeforeOrSameThread(sid, sid_to_remove))
      continue;  // Skip this segment from the result.
    tmp_sids[new_size++] = sid;
  }

  if (new_size == old_size) return ssid;
  if (new_size == 0) return SSID(0);
  if (new_size == 1) return SSID(tmp_sids[0]);

  if (DEBUG_MODE) tmp.Validate(__LINE__);

  SSID res = FindExistingOrAlocateAndCopy(&tmp);
  if (DEBUG_MODE) Get(res)->Validate(__LINE__);
  return res;
}

//  static
SSID SegmentSet::AddSegmentToTupleSS(SSID ssid, SID new_sid) {
  DCHECK(ssid.IsTuple());
  DCHECK(ssid.valid());
  AssertLive(ssid, __LINE__);
  SegmentSet *ss = Get(ssid);

  Segment::AssertLive(new_sid, __LINE__);
  const Segment *new_seg = Segment::Get(new_sid);
  TID            new_tid = new_seg->tid();

  int32_t old_size = 0, new_size = 0;
  SID tmp_sids[kMaxSegmentSetSize + 1];
  CHECK(sizeof(int32_t) == sizeof(SID));
  bool inserted_new_sid = false;
  // traverse all SID in current ss. tids are ordered.
  for (int i = 0; i < kMaxSegmentSetSize; i++, old_size++) {
    SID sid = ss->GetSID(i);
    if (sid.raw() == 0) break;
    DCHECK(sid.valid());
    Segment::AssertLive(sid, __LINE__);
    const Segment *seg = Segment::Get(sid);
    TID            tid = seg->tid();

    if (sid == new_sid) {
      // we are trying to insert a sid which is already there.
      // SS will not change.
      return ssid;
    }

    if (tid == new_tid) {
      if (seg->vts() == new_seg->vts() &&
          seg->lsid(true) == new_seg->lsid(true) &&
          seg->lsid(false) == new_seg->lsid(false)) {
        // Optimization: if a segment with the same VTS and LS
        // as in the current is already inside SS, don't modify the SS.
        // Improves performance with --keep-history >= 1.
        return ssid;
      }
      // we have another segment from the same thread => replace it.
      tmp_sids[new_size++] = new_sid;
      inserted_new_sid = true;
      continue;
    }

    if (tid > new_tid && !inserted_new_sid) {
      // there was no segment with this tid, put it now.
      tmp_sids[new_size++] = new_sid;
      inserted_new_sid = true;
    }

    if (!Segment::HappensBefore(sid, new_sid)) {
      DCHECK(!Segment::HappensBefore(new_sid, sid));
      tmp_sids[new_size++] = sid;
    }
  }

  if (!inserted_new_sid) {
    tmp_sids[new_size++] = new_sid;
  }

  CHECK_GT(new_size, 0);
  if (new_size == 1) {
    return SSID(new_sid.raw());  // Singleton.
  }

  if (new_size > kMaxSegmentSetSize) {
    CHECK(new_size == kMaxSegmentSetSize + 1);
    // we need to forget one segment. Which? The oldest one.
    int seg_to_forget = 0;
    Segment *oldest_segment = NULL;
    for (int i = 0; i < new_size; i++) {
      SID sid = tmp_sids[i];
      if (sid == new_sid) continue;
      Segment *s = Segment::Get(tmp_sids[i]);
      if (oldest_segment == NULL ||
          oldest_segment->vts()->uniq_id() > s->vts()->uniq_id()) {
        oldest_segment = s;
        seg_to_forget = i;
      }
    }
    DCHECK(oldest_segment);

    // Printf("seg_to_forget: %d T%d\n", tmp_sids[seg_to_forget].raw(),
    //        oldest_segment->tid().raw());
    for (int i = seg_to_forget; i < new_size - 1; i++) {
      tmp_sids[i] = tmp_sids[i+1];
    }
    new_size--;
  }

  CHECK(new_size <= kMaxSegmentSetSize);
  SegmentSet tmp;
  for (int i = 0; i < new_size; i++)
    tmp.sids_[i] = tmp_sids[i];  // TODO(timurrrr): avoid copying?
  if (DEBUG_MODE) tmp.Validate(__LINE__);

  SSID res = FindExistingOrAlocateAndCopy(&tmp);
  if (DEBUG_MODE) Get(res)->Validate(__LINE__);
  return res;
}



void NOINLINE SegmentSet::Validate(int line) const {
  // This is expensive!
  int my_size = size();
  for (int i = 0; i < my_size; i++) {
    SID sid1 = GetSID(i);
    CHECK(sid1.valid());
    Segment::AssertLive(sid1, __LINE__);

    for (int j = i + 1; j < my_size; j++) {
      SID sid2 = GetSID(j);
      CHECK(sid2.valid());
      Segment::AssertLive(sid2, __LINE__);

      bool hb1 = Segment::HappensBefore(sid1, sid2);
      bool hb2 = Segment::HappensBefore(sid2, sid1);
      if (hb1 || hb2) {
        Printf("BAD at line %d: %d %d %s %s\n   %s\n   %s\n",
               line, static_cast<int>(hb1), static_cast<int>(hb2),
               Segment::ToString(sid1).c_str(),
               Segment::ToString(sid2).c_str(),
               Segment::Get(sid1)->vts()->ToString().c_str(),
               Segment::Get(sid2)->vts()->ToString().c_str());
      }
      CHECK(!Segment::HappensBefore(GetSID(i), GetSID(j)));
      CHECK(!Segment::HappensBefore(GetSID(j), GetSID(i)));
      CHECK(Segment::Get(sid1)->tid() < Segment::Get(sid2)->tid());
    }
  }

  for (int i = my_size; i < kMaxSegmentSetSize; i++) {
    CHECK_EQ(sids_[i].raw(), 0);
  }
}

string SegmentSet::ToStringWithLocks(SSID ssid) {
  if (ssid.IsEmpty()) return "";
  string res = "";
  for (int i = 0; i < Size(ssid); i++) {
    SID sid = GetSID(ssid, i, __LINE__);
    if (i) res += ", ";
    res += Segment::ToStringWithLocks(sid);
  }
  return res;
}

string SegmentSet::ToString() const {
  Validate(__LINE__);
  string res = "{";
  for (int i = 0; i < size(); i++) {
    SID sid = GetSID(i);
    if (i) res += ", ";
    CHECK(sid.valid());
    Segment::AssertLive(sid, __LINE__);
    res += Segment::ToStringTidOnly(sid).c_str();
  }
  res += "}";
  return res;
}

// static
void SegmentSet::Test() {
  LSID ls(0);  // dummy
  SID sid1 = Segment::AddNewSegment(TID(0), VTS::Parse("[0:2;]"), ls, ls);
  SID sid2 = Segment::AddNewSegment(TID(1), VTS::Parse("[0:1; 1:1]"), ls, ls);
  SID sid3 = Segment::AddNewSegment(TID(2), VTS::Parse("[0:1; 2:1]"), ls, ls);
  SID sid4 = Segment::AddNewSegment(TID(3), VTS::Parse("[0:1; 3:1]"), ls, ls);
  SID sid5 = Segment::AddNewSegment(TID(4), VTS::Parse("[0:3; 2:2; 3:2;]"),
                                    ls, ls);
  SID sid6 = Segment::AddNewSegment(TID(4), VTS::Parse("[0:3; 1:2; 2:2; 3:2;]"),
                                    ls, ls);


  // SS1:{T0/S1, T2/S3}
  SegmentSet *d1 = SegmentSet::Doubleton(sid1, sid3);
  d1->Print();
  CHECK(SegmentSet::Doubleton(sid1, sid3) == d1);
  // SS2:{T0/S1, T1/S2, T2/S3}
  SegmentSet *d2 = SegmentSet::AddSegmentToTupleSS(d1, sid2);
  CHECK(SegmentSet::AddSegmentToTupleSS(d1, sid2) == d2);
  d2->Print();

  // SS3:{T0/S1, T2/S3, T3/S4}
  SegmentSet *d3 = SegmentSet::AddSegmentToTupleSS(d1, sid4);
  CHECK(SegmentSet::AddSegmentToTupleSS(d1, sid4) == d3);
  d3->Print();

  // SS4:{T0/S1, T1/S2, T2/S3, T3/S4}
  SegmentSet *d4 = SegmentSet::AddSegmentToTupleSS(d2, sid4);
  CHECK(SegmentSet::AddSegmentToTupleSS(d2, sid4) == d4);
  CHECK(SegmentSet::AddSegmentToTupleSS(d3, sid2) == d4);
  d4->Print();

  // SS5:{T1/S2, T4/S5}
  SegmentSet *d5 = SegmentSet::AddSegmentToTupleSS(d4, sid5);
  d5->Print();

  SSID ssid6 = SegmentSet::AddSegmentToTupleSS(d4->ComputeSSID(), sid6);
  CHECK(ssid6.IsSingleton());
  Printf("%s\n", ToString(ssid6).c_str());
  CHECK_EQ(sid6.raw(), 6);
  CHECK_EQ(ssid6.raw(), 6);
}

// -------- Shadow Value ------------ {{{1
class ShadowValue {
 public:
  ShadowValue() {
    if (DEBUG_MODE) {
      rd_ssid_ = 0xDEADBEEF;
      wr_ssid_ = 0xDEADBEEF;
    }
  }

  void Clear() {
    rd_ssid_ = 0;
    wr_ssid_ = 0;
  }

  INLINE bool IsNew() const { return rd_ssid_ == 0 && wr_ssid_ == 0; }
  // new experimental state machine.
  SSID rd_ssid() const { return SSID(rd_ssid_); }
  SSID wr_ssid() const { return SSID(wr_ssid_); }
  INLINE void set(SSID rd_ssid, SSID wr_ssid) {
    rd_ssid_ = rd_ssid.raw();
    wr_ssid_ = wr_ssid.raw();
  }

  // comparison
  INLINE bool operator == (const ShadowValue &sval) const {
    return rd_ssid_ == sval.rd_ssid_ &&
        wr_ssid_ == sval.wr_ssid_;
  }
  bool operator != (const ShadowValue &sval) const {
    return !(*this == sval);
  }
  bool operator <  (const ShadowValue &sval) const {
    if (rd_ssid_ < sval.rd_ssid_) return true;
    if (rd_ssid_ == sval.rd_ssid_ && wr_ssid_ < sval.wr_ssid_) return true;
    return false;
  }

  void Ref(const char *where) {
    if (!rd_ssid().IsEmpty()) {
      DCHECK(rd_ssid().valid());
      SegmentSet::Ref(rd_ssid(), where);
    }
    if (!wr_ssid().IsEmpty()) {
      DCHECK(wr_ssid().valid());
      SegmentSet::Ref(wr_ssid(), where);
    }
  }

  void Unref(const char *where) {
    if (!rd_ssid().IsEmpty()) {
      DCHECK(rd_ssid().valid());
      SegmentSet::Unref(rd_ssid(), where);
    }
    if (!wr_ssid().IsEmpty()) {
      DCHECK(wr_ssid().valid());
      SegmentSet::Unref(wr_ssid(), where);
    }
  }

  string ToString() const {
    char buff[1000];
    if (IsNew()) {
      return "{New}";
    }
    snprintf(buff, sizeof(buff), "R: %s; W: %s",
            SegmentSet::ToStringWithLocks(rd_ssid()).c_str(),
            SegmentSet::ToStringWithLocks(wr_ssid()).c_str());
    return buff;
  }

 private:
  int32_t rd_ssid_;
  int32_t wr_ssid_;
};

// -------- CacheLine --------------- {{{1
// The CacheLine is a set of Mask::kNBits (32 or 64) Shadow Values.
// The shadow values in a cache line are grouped in subsets of 8 values.
// If a particular address of memory is always accessed by aligned 8-byte
// read/write instructions, only the shadow value correspoding to the
// first byte is set, the rest shadow values are not used.
// Ditto to aligned 4- and 2-byte accesses.
// If a memory was accessed as 8 bytes and then it was accesed as 4 bytes,
// (e.g. someone used a C union) we need to split the shadow value into two.
// If the memory was accessed as 4 bytes and is now accessed as 8 bytes,
// we need to try joining the shadow values.
//
// Hence the concept of granularity_mask (which is a string of 16 bits).
// 0000000000000000 -- no accesses were observed to these 8 bytes.
// 0000000000000001 -- all accesses were 8 bytes (aligned).
// 0000000000000110 -- all accesses were 4 bytes (aligned).
// 0000000001111000 -- all accesses were 2 bytes (aligned).
// 0111111110000000 -- all accesses were 1 byte.
// 0110000000100010 -- First 4 bytes were accessed by 4 byte insns,
//   next 2 bytes by 2 byte insns, last 2 bytes by 1 byte insns.


INLINE bool GranularityIs8(uintptr_t off, uint16_t gr) {
  return gr & 1;
}

INLINE bool GranularityIs4(uintptr_t off, uint16_t gr) {
  uintptr_t off_within_8_bytes = (off >> 2) & 1;  // 0 or 1.
  return ((gr >> (1 + off_within_8_bytes)) & 1);
}

INLINE bool GranularityIs2(uintptr_t off, uint16_t gr) {
  uintptr_t off_within_8_bytes = (off >> 1) & 3;  // 0, 1, 2, or 3
  return ((gr >> (3 + off_within_8_bytes)) & 1);
}

INLINE bool GranularityIs1(uintptr_t off, uint16_t gr) {
  uintptr_t off_within_8_bytes = (off) & 7;       // 0, ..., 7
  return ((gr >> (7 + off_within_8_bytes)) & 1);
}

class CacheLine {
 public:
  static const uintptr_t kLineSizeBits = Mask::kNBitsLog;  // Don't change this.
  static const uintptr_t kLineSize = Mask::kNBits;

  static CacheLine *CreateNewCacheLine(uintptr_t tag) {
    ScopedMallocCostCenter cc("CreateNewCacheLine");
    void *mem = free_list_->Allocate();
    DCHECK(mem);
    return new (mem) CacheLine(tag);
  }

  static void Delete(CacheLine *line) {
    free_list_->Deallocate(line);
  }

  const Mask &has_shadow_value() const { return has_shadow_value_;  }
  Mask &traced() { return traced_; }
  Mask &published() { return published_; }
  Mask &racey()  { return racey_; }
  uintptr_t tag() { return tag_; }

  void DebugTrace(uintptr_t off, const char *where_str, int where_int) {
    (void)off;
    (void)where_str;
    (void)where_int;
#if 0
    if (DEBUG_MODE && tag() == G_flags->trace_addr) {
      uintptr_t off8 = off & ~7;
      Printf("CacheLine %p, off=%ld off8=%ld gr=%d "
             "has_sval: %d%d%d%d%d%d%d%d (%s:%d)\n",
             tag(), off, off8,
             granularity_[off/8],
             has_shadow_value_.Get(off8 + 0),
             has_shadow_value_.Get(off8 + 1),
             has_shadow_value_.Get(off8 + 2),
             has_shadow_value_.Get(off8 + 3),
             has_shadow_value_.Get(off8 + 4),
             has_shadow_value_.Get(off8 + 5),
             has_shadow_value_.Get(off8 + 6),
             has_shadow_value_.Get(off8 + 7),
             where_str, where_int
             );
    }
#endif
  }

  // Add a new shadow value to a place where there was no shadow value before.
  ShadowValue *AddNewSvalAtOffset(uintptr_t off) {
    DebugTrace(off, __FUNCTION__, __LINE__);
    CHECK(!has_shadow_value().Get(off));
    has_shadow_value_.Set(off);
    published_.Clear(off);
    ShadowValue *res = GetValuePointer(off);
    res->Clear();
    DebugTrace(off, __FUNCTION__, __LINE__);
    return res;
  }

  // Return true if this line has no useful information in it.
  bool Empty() {
    // The line has shadow values.
    if (!has_shadow_value().Empty()) return false;
    // If the line is traced, racey or published, we want to keep it.
    if (!traced().Empty()) return false;
    if (!racey().Empty()) return false;
    if (!published().Empty()) return false;
    return true;
  }

  INLINE Mask ClearRangeAndReturnOldUsed(uintptr_t from, uintptr_t to) {
    traced_.ClearRange(from, to);
    published_.ClearRange(from, to);
    racey_.ClearRange(from, to);
    for (uintptr_t x = (from + 7) / 8; x < to / 8; x++) {
      granularity_[x] = 0;
    }
    return has_shadow_value_.ClearRangeAndReturnOld(from, to);
  }

  void Clear() {
    has_shadow_value_.Clear();
    traced_.Clear();
    published_.Clear();
    racey_.Clear();
    for (size_t i = 0; i < TS_ARRAY_SIZE(granularity_); i++)
      granularity_[i] = 0;
  }

  ShadowValue *GetValuePointer(uintptr_t offset) {
    DCHECK(offset < kLineSize);
    return  &vals_[offset];
  }
  ShadowValue  GetValue(uintptr_t offset) { return *GetValuePointer(offset); }

  static uintptr_t ComputeOffset(uintptr_t a) {
    return a & (kLineSize - 1);
  }
  static uintptr_t ComputeTag(uintptr_t a) {
    return a & ~(kLineSize - 1);
  }
  static uintptr_t ComputeNextTag(uintptr_t a) {
    return ComputeTag(a) + kLineSize;
  }

  uint16_t *granularity_mask(uintptr_t off) {
    DCHECK(off < kLineSize);
    return &granularity_[off / 8];
  }

  void Split_8_to_4(uintptr_t off) {
    DebugTrace(off, __FUNCTION__, __LINE__);
    uint16_t gr = *granularity_mask(off);
    if (GranularityIs8(off, gr)) {
      DCHECK(!GranularityIs4(off, gr));
      DCHECK(!GranularityIs2(off, gr));
      DCHECK(!GranularityIs1(off, gr));
      uintptr_t off_8_aligned = off & ~7;
      if (has_shadow_value_.Get(off_8_aligned)) {
        ShadowValue sval = GetValue(off_8_aligned);
        sval.Ref("Split_8_to_4");
        DCHECK(!has_shadow_value_.Get(off_8_aligned + 4));
        *AddNewSvalAtOffset(off_8_aligned + 4) = sval;
      }
      *granularity_mask(off) = gr = 3 << 1;
      DCHECK(GranularityIs4(off, gr));
      DebugTrace(off, __FUNCTION__, __LINE__);
    }
  }

  void Split_4_to_2(uintptr_t off) {
    DebugTrace(off, __FUNCTION__, __LINE__);
    uint16_t gr = *granularity_mask(off);
    if (GranularityIs4(off, gr)) {
      DCHECK(!GranularityIs8(off, gr));
      DCHECK(!GranularityIs2(off, gr));
      DCHECK(!GranularityIs1(off, gr));
      uint16_t off_4_aligned = off & ~3;
      if (has_shadow_value_.Get(off_4_aligned)) {
        ShadowValue sval = GetValue(off_4_aligned);
        sval.Ref("Split_4_to_2");
        DCHECK(!has_shadow_value_.Get(off_4_aligned + 2));
        *AddNewSvalAtOffset(off_4_aligned + 2) = sval;
      }
      // Clear this 4-granularity bit.
      uintptr_t off_within_8_bytes = (off >> 2) & 1;  // 0 or 1.
      gr &= ~(1 << (1 + off_within_8_bytes));
      // Set two 2-granularity bits.
      gr |= 3 << (3 + 2 * off_within_8_bytes);
      *granularity_mask(off) = gr;
      DebugTrace(off, __FUNCTION__, __LINE__);
    }
  }

  void Split_2_to_1(uintptr_t off) {
    DebugTrace(off, __FUNCTION__, __LINE__);
    uint16_t gr = *granularity_mask(off);
    if (GranularityIs2(off, gr)) {
      DCHECK(!GranularityIs8(off, gr));
      DCHECK(!GranularityIs4(off, gr));
      DCHECK(!GranularityIs1(off, gr));
      uint16_t off_2_aligned = off & ~1;
      if (has_shadow_value_.Get(off_2_aligned)) {
        ShadowValue sval = GetValue(off_2_aligned);
        sval.Ref("Split_2_to_1");
        DCHECK(!has_shadow_value_.Get(off_2_aligned + 1));
        *AddNewSvalAtOffset(off_2_aligned + 1) = sval;
      }
      // Clear this 2-granularity bit.
      uintptr_t off_within_8_bytes = (off >> 1) & 3;  // 0, 1, 2, or 3
      gr &= ~(1 << (3 + off_within_8_bytes));
      // Set two 1-granularity bits.
      gr |= 3 << (7 + 2 * off_within_8_bytes);
      *granularity_mask(off) = gr;
      DebugTrace(off, __FUNCTION__, __LINE__);
    }
  }

  void Join_1_to_2(uintptr_t off) {
    DebugTrace(off, __FUNCTION__, __LINE__);
    DCHECK((off & 1) == 0);
    uint16_t gr = *granularity_mask(off);
    if (GranularityIs1(off, gr)) {
      DCHECK(GranularityIs1(off + 1, gr));
      if (has_shadow_value_.Get(off) && has_shadow_value_.Get(off + 1)) {
        if (GetValue(off) == GetValue(off + 1)) {
          ShadowValue *sval_p = GetValuePointer(off + 1);
          sval_p->Unref("Join_1_to_2");
          sval_p->Clear();
          has_shadow_value_.Clear(off + 1);
          uintptr_t off_within_8_bytes = (off >> 1) & 3;  // 0, 1, 2, or 3
          // Clear two 1-granularity bits.
          gr &= ~(3 << (7 + 2 * off_within_8_bytes));
          // Set one 2-granularity bit.
          gr |= 1 << (3 + off_within_8_bytes);
          *granularity_mask(off) = gr;
          DebugTrace(off, __FUNCTION__, __LINE__);
        }
      }
    }
  }

  void Join_2_to_4(uintptr_t off) {
    DebugTrace(off, __FUNCTION__, __LINE__);
    DCHECK((off & 3) == 0);
    uint16_t gr = *granularity_mask(off);
    if (GranularityIs2(off, gr) && GranularityIs2(off + 2, gr)) {
      if (has_shadow_value_.Get(off) && has_shadow_value_.Get(off + 2)) {
        if (GetValue(off) == GetValue(off + 2)) {
          ShadowValue *sval_p = GetValuePointer(off + 2);
          sval_p->Unref("Join_2_to_4");
          sval_p->Clear();
          has_shadow_value_.Clear(off + 2);
          uintptr_t off_within_8_bytes = (off >> 2) & 1;  // 0 or 1.
          // Clear two 2-granularity bits.
          gr &= ~(3 << (3 + 2 * off_within_8_bytes));
          // Set one 4-granularity bit.
          gr |= 1 << (1 + off_within_8_bytes);
          *granularity_mask(off) = gr;
          DebugTrace(off, __FUNCTION__, __LINE__);
        }
      }
    }
  }

  void Join_4_to_8(uintptr_t off) {
    DebugTrace(off, __FUNCTION__, __LINE__);
    DCHECK((off & 7) == 0);
    uint16_t gr = *granularity_mask(off);
    if (GranularityIs4(off, gr) && GranularityIs4(off + 4, gr)) {
      if (has_shadow_value_.Get(off) && has_shadow_value_.Get(off + 4)) {
        if (GetValue(off) == GetValue(off + 4)) {
          ShadowValue *sval_p = GetValuePointer(off + 4);
          sval_p->Unref("Join_4_to_8");
          sval_p->Clear();
          has_shadow_value_.Clear(off + 4);
          *granularity_mask(off) = 1;
          DebugTrace(off, __FUNCTION__, __LINE__);
        }
      }
    }
  }

  static void InitClassMembers() {
    if (DEBUG_MODE) {
      Printf("sizeof(CacheLine) = %ld\n", sizeof(CacheLine));
    }
    free_list_ = new FreeList(sizeof(CacheLine), 1024);
  }

 private:
  explicit CacheLine(uintptr_t tag) {
    tag_ = tag;
    Clear();
  }
  ~CacheLine() { }

  uintptr_t tag_;

  // data members
  Mask has_shadow_value_;
  Mask traced_;
  Mask racey_;
  Mask published_;
  uint16_t granularity_[kLineSize / 8];
  ShadowValue vals_[kLineSize];

  // static data members.
  static FreeList *free_list_;
};

FreeList *CacheLine::free_list_;

// If range [a,b) fits into one line, return that line's tag.
// Else range [a,b) is broken into these ranges:
//   [a, line1_tag)
//   [line1_tag, line2_tag)
//   [line2_tag, b)
// and 0 is returned.
uintptr_t GetCacheLinesForRange(uintptr_t a, uintptr_t b,
                                uintptr_t *line1_tag, uintptr_t *line2_tag) {
  uintptr_t a_tag = CacheLine::ComputeTag(a);
  uintptr_t next_tag = CacheLine::ComputeNextTag(a);
  if (b < next_tag) {
    return a_tag;
  }
  *line1_tag = next_tag;
  *line2_tag = CacheLine::ComputeTag(b);
  return 0;
}


// -------- Cache ------------------ {{{1
class Cache {
 public:
  Cache() {
    memset(lines_, 0, sizeof(lines_));
    ANNOTATE_BENIGN_RACE_SIZED(lines_, sizeof(lines_),
                               "Cache::lines_ accessed without a lock");
  }

  INLINE static CacheLine *kLineIsLocked() {
    return (CacheLine*)1;
  }

  INLINE static bool LineIsNullOrLocked(CacheLine *line) {
    return (uintptr_t)line <= 1;
  }

  INLINE CacheLine *TidMagic(int32_t tid) {
    return kLineIsLocked();
  }

  // Try to get a CacheLine for exclusive use.
  // May return NULL or kLineIsLocked.
  INLINE CacheLine *TryAcquireLine(TSanThread *thr, uintptr_t a, int call_site) {
    uintptr_t cli = ComputeCacheLineIndexInCache(a);
    CacheLine **addr = &lines_[cli];
    CacheLine *res = (CacheLine*)AtomicExchange(
           (uintptr_t*)addr, (uintptr_t)kLineIsLocked());
    if (DEBUG_MODE && debug_cache) {
      uintptr_t tag = CacheLine::ComputeTag(a);
      if (res && res != kLineIsLocked())
        Printf("TryAcquire %p empty=%d tag=%lx cli=%lx site=%d\n",
               res, res->Empty(), res->tag(), cli, call_site);
      else
        Printf("TryAcquire tag=%lx cli=%d site=%d\n", tag, cli, call_site);
    }
    if (res) {
      ANNOTATE_HAPPENS_AFTER((void*)cli);
    }
    return res;
  }

  INLINE CacheLine *AcquireLine(TSanThread *thr, uintptr_t a, int call_site) {
    CacheLine *line = NULL;
    int iter = 0;
    const int max_iter = 1 << 30;
    for (;;) {
      line = TryAcquireLine(thr, a, call_site);
      if (line != kLineIsLocked())
        break;
      iter++;
      if ((iter % (1 << 6)) == 0) {
        YIELD();
        G_stats->try_acquire_line_spin++;
        if (DEBUG_MODE && debug_cache && ((iter & (iter - 1)) == 0)) {
          Printf("T%d %s a=%p iter=%d\n", raw_tid(thr), __FUNCTION__, a, iter);
        }
      } else {
        for (int active_spin = 0; active_spin != 10; active_spin += 1) {
          PROCESSOR_YIELD();
        }
      }
      if (DEBUG_MODE && debug_cache && iter == max_iter) {
        Printf("Failed to acquire a cache line: T%d a=%p site=%d\n",
               raw_tid(thr), a, call_site);
        CHECK(iter < max_iter);
      }
    }
    DCHECK(lines_[ComputeCacheLineIndexInCache(a)] == TidMagic(raw_tid(thr)));
    return line;
  }

  // Release a CacheLine from exclusive use.
  INLINE void ReleaseLine(TSanThread *thr, uintptr_t a, CacheLine *line, int call_site) {
    if (TS_SERIALIZED) return;
    DCHECK(line != kLineIsLocked());
    uintptr_t cli = ComputeCacheLineIndexInCache(a);
    DCHECK(line == NULL ||
           cli == ComputeCacheLineIndexInCache(line->tag()));
    CacheLine **addr = &lines_[cli];
    DCHECK(*addr == TidMagic(raw_tid(thr)));
    ReleaseStore((uintptr_t*)addr, (uintptr_t)line);
    ANNOTATE_HAPPENS_BEFORE((void*)cli);
    if (DEBUG_MODE && debug_cache) {
      uintptr_t tag = CacheLine::ComputeTag(a);
      if (line)
        Printf("Release %p empty=%d tag=%lx cli=%lx site=%d\n",
               line, line->Empty(), line->tag(), cli, call_site);
      else
        Printf("Release tag=%lx cli=%d site=%d\n", tag, cli, call_site);
    }
  }

  void AcquireAllLines(TSanThread *thr) {
    CHECK(TS_SERIALIZED == 0);
    for (size_t i = 0; i < (size_t)kNumLines; i++) {
      uintptr_t tag = i << CacheLine::kLineSizeBits;
      AcquireLine(thr, tag, __LINE__);
      CHECK(lines_[i] == kLineIsLocked());
    }
  }

  // Get a CacheLine. This operation should be performed under a lock
  // (whatever that is), but other threads may be acquiring the same line
  // concurrently w/o a lock.
  // Every call to GetLine() which returns non-null line
  // should be followed by a call to ReleaseLine().
  INLINE CacheLine *GetLine(TSanThread *thr, uintptr_t a, bool create_new_if_need, int call_site) {
    uintptr_t tag = CacheLine::ComputeTag(a);
    DCHECK(tag <= a);
    DCHECK(tag + CacheLine::kLineSize > a);
    uintptr_t cli = ComputeCacheLineIndexInCache(a);
    CacheLine *res = NULL;
    CacheLine *line = NULL;

    if (create_new_if_need == false && lines_[cli] == 0) {
      // There is no such line in the cache, nor should it be in the storage.
      // Check that the storage indeed does not have this line.
      // Such DCHECK is racey if tsan is multi-threaded.
      DCHECK(TS_SERIALIZED == 0 || storage_.count(tag) == 0);
      return NULL;
    }

    if (TS_SERIALIZED) {
      line = lines_[cli];
    } else {
      line = AcquireLine(thr, tag, call_site);
    }


    if (LIKELY(line && line->tag() == tag)) {
      res = line;
    } else {
      res = WriteBackAndFetch(thr, line, tag, cli, create_new_if_need);
      if (!res) {
        ReleaseLine(thr, a, line, call_site);
      }
    }
    if (DEBUG_MODE && debug_cache) {
      if (res)
        Printf("GetLine %p empty=%d tag=%lx\n", res, res->Empty(), res->tag());
      else
        Printf("GetLine res=NULL, line=%p tag=%lx cli=%lx\n", line, tag, cli);
    }
    return res;
  }

  INLINE CacheLine *GetLineOrCreateNew(TSanThread *thr, uintptr_t a, int call_site) {
    return GetLine(thr, a, true, call_site);
  }
  INLINE CacheLine *GetLineIfExists(TSanThread *thr, uintptr_t a, int call_site) {
    return GetLine(thr, a, false, call_site);
  }

  void ForgetAllState(TSanThread *thr) {
    for (int i = 0; i < kNumLines; i++) {
      if (TS_SERIALIZED == 0) CHECK(LineIsNullOrLocked(lines_[i]));
      lines_[i] = NULL;
    }
    map<uintptr_t, Mask> racey_masks;
    for (Map::iterator i = storage_.begin(); i != storage_.end(); ++i) {
      CacheLine *line = i->second;
      if (!line->racey().Empty()) {
        racey_masks[line->tag()] = line->racey();
      }
      CacheLine::Delete(line);
    }
    storage_.clear();
    // Restore the racey masks.
    for (map<uintptr_t, Mask>::iterator it = racey_masks.begin();
         it != racey_masks.end(); it++) {
      CacheLine *line = GetLineOrCreateNew(thr, it->first, __LINE__);
      line->racey() = it->second;
      DCHECK(!line->racey().Empty());
      ReleaseLine(thr, line->tag(), line, __LINE__);
    }
  }

  void PrintStorageStats() {
    if (!G_flags->show_stats) return;
    set<ShadowValue> all_svals;
    map<size_t, int> sizes;
    for (Map::iterator it = storage_.begin(); it != storage_.end(); ++it) {
      CacheLine *line = it->second;
      // uintptr_t cli = ComputeCacheLineIndexInCache(line->tag());
      //if (lines_[cli] == line) {
        // this line is in cache -- ignore it.
      //  continue;
      //}
      set<ShadowValue> s;
      for (uintptr_t i = 0; i < CacheLine::kLineSize; i++) {
        if (line->has_shadow_value().Get(i)) {
          ShadowValue sval = *(line->GetValuePointer(i));
          s.insert(sval);
          all_svals.insert(sval);
        }
      }
      size_t size = s.size();
      if (size > 10) size = 10;
      sizes[size]++;
    }
    Printf("Storage sizes: %ld\n", storage_.size());
    for (size_t size = 0; size <= CacheLine::kLineSize; size++) {
      if (sizes[size]) {
        Printf("  %ld => %d\n", size, sizes[size]);
      }
    }
    Printf("Different svals: %ld\n", all_svals.size());
    set <SSID> all_ssids;
    for (set<ShadowValue>::iterator it = all_svals.begin(); it != all_svals.end(); ++it) {
      ShadowValue sval = *it;
      for (int i = 0; i < 2; i++) {
        SSID ssid = i ? sval.rd_ssid() : sval.wr_ssid();
        all_ssids.insert(ssid);
      }
    }
    Printf("Different ssids: %ld\n", all_ssids.size());
    set <SID> all_sids;
    for (set<SSID>::iterator it = all_ssids.begin(); it != all_ssids.end(); ++it) {
      int size = SegmentSet::Size(*it);
      for (int i = 0; i < size; i++) {
        SID sid = SegmentSet::GetSID(*it, i, __LINE__);
        all_sids.insert(sid);
      }
    }
    Printf("Different sids: %ld\n", all_sids.size());
    for (int i = 1; i < Segment::NumberOfSegments(); i++) {
      if (Segment::ProfileSeg(SID(i)) && all_sids.count(SID(i)) == 0) {
        // Printf("Segment SID %d: missing in storage; ref=%d\n", i,
        // Segment::Get(SID(i))->ref_count());
      }
    }
  }

 private:
  INLINE uintptr_t ComputeCacheLineIndexInCache(uintptr_t addr) {
    return (addr >> CacheLine::kLineSizeBits) & (kNumLines - 1);
  }

  NOINLINE CacheLine *WriteBackAndFetch(TSanThread *thr, CacheLine *old_line,
                                        uintptr_t tag, uintptr_t cli,
                                        bool create_new_if_need) {
    ScopedMallocCostCenter cc("Cache::WriteBackAndFetch");
    CacheLine *res;
    size_t old_storage_size = storage_.size();
    (void)old_storage_size;
    CacheLine **line_for_this_tag = NULL;
    if (create_new_if_need) {
      line_for_this_tag = &storage_[tag];
    } else {
      Map::iterator it = storage_.find(tag);
      if (it == storage_.end()) {
        if (DEBUG_MODE && debug_cache) {
          Printf("WriteBackAndFetch: old_line=%ld tag=%lx cli=%ld\n",
                 old_line, tag, cli);
        }
        return NULL;
      }
      line_for_this_tag = &(it->second);
    }
    CHECK(line_for_this_tag);
    DCHECK(old_line != kLineIsLocked());
    if (*line_for_this_tag == NULL) {
      // creating a new cache line
      CHECK(storage_.size() == old_storage_size + 1);
      res = CacheLine::CreateNewCacheLine(tag);
      if (DEBUG_MODE && debug_cache) {
        Printf("%s %d new line %p cli=%lx\n", __FUNCTION__, __LINE__, res, cli);
      }
      *line_for_this_tag = res;
      G_stats->cache_new_line++;
    } else {
      // taking an existing cache line from storage.
      res = *line_for_this_tag;
      if (DEBUG_MODE && debug_cache) {
        Printf("%s %d exi line %p tag=%lx old=%p empty=%d cli=%lx\n",
             __FUNCTION__, __LINE__, res, res->tag(), old_line,
             res->Empty(), cli);
      }
      DCHECK(!res->Empty());
      G_stats->cache_fetch++;
    }

    if (TS_SERIALIZED) {
      lines_[cli] = res;
    } else {
      DCHECK(lines_[cli] == TidMagic(raw_tid(thr)));
    }

    if (old_line) {
      if (DEBUG_MODE && debug_cache) {
        Printf("%s %d old line %p empty=%d\n", __FUNCTION__, __LINE__,
               old_line, old_line->Empty());
      }
      if (old_line->Empty()) {
        storage_.erase(old_line->tag());
        CacheLine::Delete(old_line);
        G_stats->cache_delete_empty_line++;
      } else {
        if (debug_cache) {
          DebugOnlyCheckCacheLineWhichWeReplace(old_line, res);
        }
      }
    }
    DCHECK(res->tag() == tag);

    if (G_stats->cache_max_storage_size < storage_.size()) {
      G_stats->cache_max_storage_size = storage_.size();
    }

    return res;
  }

  void DebugOnlyCheckCacheLineWhichWeReplace(CacheLine *old_line,
                                             CacheLine *new_line) {
    static int c = 0;
    c++;
    if ((c % 1024) == 1) {
      set<int64_t> s;
      for (uintptr_t i = 0; i < CacheLine::kLineSize; i++) {
        if (old_line->has_shadow_value().Get(i)) {
          int64_t sval = *reinterpret_cast<int64_t*>(
                            old_line->GetValuePointer(i));
          s.insert(sval);
        }
      }
      Printf("\n[%d] Cache Size=%ld %s different values: %ld\n", c,
             storage_.size(), old_line->has_shadow_value().ToString().c_str(),
             s.size());

      Printf("new line: %p %p\n", new_line->tag(), new_line->tag()
             + CacheLine::kLineSize);
      G_stats->PrintStatsForCache();
    }
  }

  static const int kNumLines = 1 << (DEBUG_MODE ? 14 : 21);
  CacheLine *lines_[kNumLines];

  // tag => CacheLine
  typedef unordered_map<uintptr_t, CacheLine*> Map;
  Map storage_;
};

static  Cache *G_cache;

// -------- Published range -------------------- {{{1
struct PublishInfo {
  uintptr_t tag;   // Tag of the cache line where the mem is published.
  Mask      mask;  // The bits that are actually published.
  VTS      *vts;   // The point where this range has been published.
};


typedef multimap<uintptr_t, PublishInfo> PublishInfoMap;

// Maps 'mem+size' to the PublishInfoMap{mem, size, vts}.
static PublishInfoMap *g_publish_info_map;

const int kDebugPublish = 0;

// Get a VTS where 'a' has been published,
// return NULL if 'a' was not published.
static const VTS *GetPublisherVTS(uintptr_t a) {
  uintptr_t tag = CacheLine::ComputeTag(a);
  uintptr_t off = CacheLine::ComputeOffset(a);
  typedef PublishInfoMap::iterator Iter;

  pair<Iter, Iter> eq_range = g_publish_info_map->equal_range(tag);
  for (Iter it = eq_range.first; it != eq_range.second; ++it) {
    PublishInfo &info = it->second;
    DCHECK(info.tag == tag);
    if (info.mask.Get(off)) {
      G_stats->publish_get++;
      // Printf("GetPublisherVTS: a=%p vts=%p\n", a, info.vts);
      return info.vts;
    }
  }
  Printf("GetPublisherVTS returned NULL: a=%p\n", a);
  return NULL;
}

static bool CheckSanityOfPublishedMemory(uintptr_t tag, int line) {
  if (!DEBUG_MODE) return true;
  if (kDebugPublish)
    Printf("CheckSanityOfPublishedMemory: line=%d\n", line);
  typedef PublishInfoMap::iterator Iter;
  pair<Iter, Iter> eq_range = g_publish_info_map->equal_range(tag);
  Mask union_of_masks(0);
  // iterate over all entries for this tag
  for (Iter it = eq_range.first; it != eq_range.second; ++it) {
    PublishInfo &info = it->second;
    CHECK(info.tag  == tag);
    CHECK(it->first == tag);
    CHECK(info.vts);
    Mask mask(info.mask);
    CHECK(!mask.Empty());  // Mask should not be empty..
    // And should not intersect with other masks.
    CHECK(Mask::Intersection(union_of_masks, mask).Empty());
    union_of_masks.Union(mask);
  }
  return true;
}

// Clear the publish attribute for the bytes from 'line' that are set in 'mask'
static void ClearPublishedAttribute(CacheLine *line, Mask mask) {
  CHECK(CheckSanityOfPublishedMemory(line->tag(), __LINE__));
  typedef PublishInfoMap::iterator Iter;
  bool deleted_some = true;
  if (kDebugPublish)
    Printf(" ClearPublishedAttribute: %p %s\n",
           line->tag(), mask.ToString().c_str());
  while (deleted_some) {
    deleted_some = false;
    pair<Iter, Iter> eq_range = g_publish_info_map->equal_range(line->tag());
    for (Iter it = eq_range.first; it != eq_range.second; ++it) {
      PublishInfo &info = it->second;
      DCHECK(info.tag == line->tag());
      if (kDebugPublish)
        Printf("?ClearPublishedAttribute: %p %s\n", line->tag(),
               info.mask.ToString().c_str());
      info.mask.Subtract(mask);
      if (kDebugPublish)
        Printf("+ClearPublishedAttribute: %p %s\n", line->tag(),
               info.mask.ToString().c_str());
      G_stats->publish_clear++;
      if (info.mask.Empty()) {
        VTS::Unref(info.vts);
        g_publish_info_map->erase(it);
        deleted_some = true;
        break;
      }
    }
  }
  CHECK(CheckSanityOfPublishedMemory(line->tag(), __LINE__));
}

// Publish range [a, b) in addr's CacheLine with vts.
static void PublishRangeInOneLine(TSanThread *thr, uintptr_t addr, uintptr_t a,
                                  uintptr_t b, VTS *vts) {
  ScopedMallocCostCenter cc("PublishRangeInOneLine");
  DCHECK(b <= CacheLine::kLineSize);
  DCHECK(a < b);
  uintptr_t tag = CacheLine::ComputeTag(addr);
  CHECK(CheckSanityOfPublishedMemory(tag, __LINE__));
  CacheLine *line = G_cache->GetLineOrCreateNew(thr, tag, __LINE__);

  if (1 || line->published().GetRange(a, b)) {
    Mask mask(0);
    mask.SetRange(a, b);
    // TODO(timurrrr): add warning for re-publishing.
    ClearPublishedAttribute(line, mask);
  }

  line->published().SetRange(a, b);
  G_cache->ReleaseLine(thr, tag, line, __LINE__);

  PublishInfo pub_info;
  pub_info.tag  = tag;
  pub_info.mask.SetRange(a, b);
  pub_info.vts  = vts->Clone();
  g_publish_info_map->insert(make_pair(tag, pub_info));
  G_stats->publish_set++;
  if (kDebugPublish)
    Printf("PublishRange   : [%p,%p) %p %s vts=%p\n",
           a, b, tag, pub_info.mask.ToString().c_str(), vts);
  CHECK(CheckSanityOfPublishedMemory(tag, __LINE__));
}

// Publish memory range [a, b).
static void PublishRange(TSanThread *thr, uintptr_t a, uintptr_t b, VTS *vts) {
  CHECK(a);
  CHECK(a < b);
  if (kDebugPublish)
    Printf("PublishRange   : [%p,%p), size=%d, tag=%p\n",
           a, b, (int)(b - a), CacheLine::ComputeTag(a));
  uintptr_t line1_tag = 0, line2_tag = 0;
  uintptr_t tag = GetCacheLinesForRange(a, b, &line1_tag, &line2_tag);
  if (tag) {
    PublishRangeInOneLine(thr, tag, a - tag, b - tag, vts);
    return;
  }
  uintptr_t a_tag = CacheLine::ComputeTag(a);
  PublishRangeInOneLine(thr, a, a - a_tag, CacheLine::kLineSize, vts);
  for (uintptr_t tag_i = line1_tag; tag_i < line2_tag;
       tag_i += CacheLine::kLineSize) {
    PublishRangeInOneLine(thr, tag_i, 0, CacheLine::kLineSize, vts);
  }
  if (b > line2_tag) {
    PublishRangeInOneLine(thr, line2_tag, 0, b - line2_tag, vts);
  }
}

// -------- ThreadSanitizerReport -------------- {{{1
struct ThreadSanitizerReport {
  // Types of reports.
  enum ReportType {
    DATA_RACE,
    UNLOCK_FOREIGN,
    UNLOCK_NONLOCKED,
    INVALID_LOCK,
    ATOMICITY_VIOLATION,
  };

  // Common fields.
  ReportType  type;
  TID         tid;
  StackTrace *stack_trace;

  const char *ReportName() const {
    switch (type) {
      case DATA_RACE:        return "Race";
      case UNLOCK_FOREIGN:   return "UnlockForeign";
      case UNLOCK_NONLOCKED: return "UnlockNonLocked";
      case INVALID_LOCK:     return "InvalidLock";
      case ATOMICITY_VIOLATION: return "AtomicityViolation";
    }
    CHECK(0);
    return NULL;
  }

  virtual ~ThreadSanitizerReport() {
    StackTrace::Delete(stack_trace);
  }
};

static bool ThreadSanitizerPrintReport(ThreadSanitizerReport *report);

// DATA_RACE.
struct ThreadSanitizerDataRaceReport : public ThreadSanitizerReport {
  uintptr_t   racey_addr;
  string      racey_addr_description;
  uintptr_t   last_access_size;
  TID         last_access_tid;
  SID         last_access_sid;
  bool        last_access_is_w;
  LSID        last_acces_lsid[2];

  ShadowValue new_sval;
  ShadowValue old_sval;

  bool        is_expected;
  bool        racey_addr_was_published;
};

// Report for bad unlock (UNLOCK_FOREIGN, UNLOCK_NONLOCKED).
struct ThreadSanitizerBadUnlockReport : public ThreadSanitizerReport {
  LID lid;
};

// Report for invalid lock addresses (INVALID_LOCK).
struct ThreadSanitizerInvalidLockReport : public ThreadSanitizerReport {
  uintptr_t lock_addr;
};

class AtomicityRegion;

struct ThreadSanitizerAtomicityViolationReport : public ThreadSanitizerReport {
  AtomicityRegion *r1, *r2, *r3;
};


// -------- LockHistory ------------- {{{1
// For each thread we store a limited amount of history of locks and unlocks.
// If there is a race report (in hybrid mode) we try to guess a lock
// which might have been used to pass the ownership of the object between
// threads.
//
// Thread1:                    Thread2:
// obj->UpdateMe();
// mu.Lock();
// flag = true;
// mu.Unlock(); // (*)
//                             mu.Lock();  // (**)
//                             bool f = flag;
//                             mu.Unlock();
//                             if (f)
//                                obj->UpdateMeAgain();
//
// For this code a hybrid detector may report a false race.
// LockHistory will find the lock mu and report it.

struct LockHistory {
 public:
  // LockHistory which will track no more than `size` recent locks
  // and the same amount of unlocks.
  LockHistory(size_t size): size_(size) { }

  // Record a Lock event.
  void OnLock(LID lid) {
    g_lock_era++;
    Push(LockHistoryElement(lid, g_lock_era), &locks_);
  }

  // Record an Unlock event.
  void OnUnlock(LID lid) {
    g_lock_era++;
    Push(LockHistoryElement(lid, g_lock_era), &unlocks_);
  }

  // Find locks such that:
  // - A Lock happend in `l`.
  // - An Unlock happened in `u`.
  // - Lock's era is greater than Unlock's era.
  // - Both eras are greater or equal than min_lock_era.
  static bool Intersect(const LockHistory &l, const LockHistory &u,
                        int32_t min_lock_era, set<LID> *locks) {
    const Queue &lq = l.locks_;
    const Queue &uq = u.unlocks_;
    for (size_t i = 0; i < lq.size(); i++) {
      int32_t l_era = lq[i].lock_era;
      if (l_era < min_lock_era) continue;
      LID lid = lq[i].lid;
      // We don't want to report pure happens-before locks since
      // they already create h-b arcs.
      if (Lock::LIDtoLock(lid)->is_pure_happens_before()) continue;
      for (size_t j = 0; j < uq.size(); j++) {
        int32_t u_era = uq[j].lock_era;
        if (lid != uq[j].lid) continue;
        // Report("LockHistory::Intersect: L%d %d %d %d\n", lid.raw(), min_lock_era, u_era, l_era);
        if (u_era < min_lock_era)  continue;
        if (u_era > l_era) continue;
        locks->insert(lid);
      }
    }
    return !locks->empty();
  }

  void PrintLocks() const { Print(&locks_); }
  void PrintUnlocks() const { Print(&unlocks_); }

 private:
  struct LockHistoryElement {
    LID lid;
    uint32_t lock_era;
    LockHistoryElement(LID l, uint32_t era)
        : lid(l),
        lock_era(era) {
        }
  };

  typedef deque<LockHistoryElement> Queue;

  void Push(LockHistoryElement e, Queue *q) {
    CHECK(q->size() <= size_);
    if (q->size() == size_)
      q->pop_front();
    q->push_back(e);
  }

  void Print(const Queue *q) const {
    set<LID> printed;
    for (size_t i = 0; i < q->size(); i++) {
      const LockHistoryElement &e = (*q)[i];
      if (printed.count(e.lid)) continue;
      Report("era %d: \n", e.lock_era);
      Lock::ReportLockWithOrWithoutContext(e.lid, true);
      printed.insert(e.lid);
    }
  }

  Queue locks_;
  Queue unlocks_;
  size_t size_;
};

// -------- RecentSegmentsCache ------------- {{{1
// For each thread we store a limited amount of recent segments with
// the same VTS and LS as the current segment.
// When a thread enters a new basic block, we can sometimes reuse a
// recent segment if it is the same or not used anymore (see Search()).
//
// We need to flush the cache when current lockset changes or the current
// VTS changes or we do ForgetAllState.
// TODO(timurrrr): probably we can cache segments with different LSes and
// compare their LS with the current LS.
struct RecentSegmentsCache {
 public:
  RecentSegmentsCache(int cache_size) : cache_size_(cache_size) {}
  ~RecentSegmentsCache() { Clear(); }

  void Clear() {
    ShortenQueue(0);
  }

  void Push(SID sid) {
    queue_.push_front(sid);
    Segment::Ref(sid, "RecentSegmentsCache::ShortenQueue");
    ShortenQueue(cache_size_);
  }

  void ForgetAllState() {
    queue_.clear();  // Don't unref - the segments are already dead.
  }

  INLINE SID Search(CallStack *curr_stack,
                    SID curr_sid, /*OUT*/ bool *needs_refill) {
    // TODO(timurrrr): we can probably move the matched segment to the head
    // of the queue.

    deque<SID>::iterator it = queue_.begin();
    for (; it != queue_.end(); it++) {
      SID sid = *it;
      Segment::AssertLive(sid, __LINE__);
      Segment *seg = Segment::Get(sid);

      if (seg->ref_count() == 1 + (sid == curr_sid)) {
        // The current segment is not used anywhere else,
        // so just replace the stack trace in it.
        // The refcount of an unused segment is equal to
        // *) 1 if it is stored only in the cache,
        // *) 2 if it is the current segment of the Thread.
        *needs_refill = true;
        return sid;
      }

      // Check three top entries of the call stack of the recent segment.
      // If they match the current segment stack, don't create a new segment.
      // This can probably lead to a little bit wrong stack traces in rare
      // occasions but we don't really care that much.
      if (kSizeOfHistoryStackTrace > 0) {
        size_t n = curr_stack->size();
        uintptr_t *emb_trace = Segment::embedded_stack_trace(sid);
        if(*emb_trace &&  // This stack trace was filled
           curr_stack->size() >= 3 &&
           emb_trace[0] == (*curr_stack)[n-1] &&
           emb_trace[1] == (*curr_stack)[n-2] &&
           emb_trace[2] == (*curr_stack)[n-3]) {
          *needs_refill = false;
          return sid;
        }
      }
    }

    return SID();
  }

 private:
  void ShortenQueue(size_t flush_to_length) {
    while (queue_.size() > flush_to_length) {
      SID sid = queue_.back();
      Segment::Unref(sid, "RecentSegmentsCache::ShortenQueue");
      queue_.pop_back();
    }
  }

  deque<SID> queue_;
  size_t cache_size_;
};

// -------- TraceInfo ------------------ {{{1
vector<TraceInfo*> *TraceInfo::g_all_traces;

TraceInfo *TraceInfo::NewTraceInfo(size_t n_mops, uintptr_t pc) {
  ScopedMallocCostCenter cc("TraceInfo::NewTraceInfo");
  size_t mem_size = (sizeof(TraceInfo) + (n_mops - 1) * sizeof(MopInfo));
  uint8_t *mem = new uint8_t[mem_size];
  memset(mem, 0xab, mem_size);
  TraceInfo *res = new (mem) TraceInfo;
  res->n_mops_ = n_mops;
  res->pc_ = ThreadSanitizerWantToCreateSegmentsOnSblockEntry(pc) ? pc : 0;
  res->counter_ = 0;
  if (g_all_traces == NULL) {
    g_all_traces = new vector<TraceInfo*>;
  }
  res->literace_storage = NULL;
  if (G_flags->literace_sampling != 0) {
    ScopedMallocCostCenter cc("TraceInfo::NewTraceInfo::LiteRaceStorage");
    size_t index_of_this_trace = g_all_traces->size();
    if ((index_of_this_trace % kLiteRaceStorageSize) == 0) {
      res->literace_storage = (LiteRaceStorage*)
          new LiteRaceCounters [kLiteRaceStorageSize * kLiteRaceNumTids];
      memset(res->literace_storage, 0, sizeof(LiteRaceStorage));
    } else {
      CHECK(index_of_this_trace > 0);
      res->literace_storage = (*g_all_traces)[index_of_this_trace - 1]->literace_storage;
      CHECK(res->literace_storage);
    }
    res->storage_index = index_of_this_trace % kLiteRaceStorageSize;
  }
  g_all_traces->push_back(res);
  return res;
}

void TraceInfo::PrintTraceProfile() {
  if (!G_flags->trace_profile) return;
  if (!g_all_traces) return;
  int64_t total_counter = 0;
  multimap<size_t, TraceInfo*> traces;
  for (size_t i = 0; i < g_all_traces->size(); i++) {
    TraceInfo *trace = (*g_all_traces)[i];
    traces.insert(make_pair(trace->counter(), trace));
    total_counter += trace->counter();
  }
  if (total_counter == 0) return;
  Printf("TraceProfile: %ld traces, %lld hits\n",
         g_all_traces->size(), total_counter);
  int i = 0;
  for (multimap<size_t, TraceInfo*>::reverse_iterator it = traces.rbegin();
       it != traces.rend(); ++it, i++) {
    TraceInfo *trace = it->second;
    int64_t c = it->first;
    int64_t permile = (c * 1000) / total_counter;
    CHECK(trace->n_mops() > 0);
    uintptr_t pc = trace->GetMop(0)->pc();
    CHECK(pc);
    if (permile == 0 || i >= 20) break;
    Printf("TR=%p pc: %p %p c=%lld (%lld/1000) n_mops=%ld %s\n",
           trace, trace->pc(), pc, c,
           permile, trace->n_mops(),
           PcToRtnNameAndFilePos(pc).c_str());
  }
}

// -------- Atomicity --------------- {{{1
// An attempt to detect atomicity violations (aka high level races).
// Here we try to find a very restrictive pattern:
// Thread1                    Thread2
//   r1: {
//     mu.Lock();
//     code_r1();
//     mu.Unlock();
//   }
//   r2: {
//     mu.Lock();
//     code_r2();
//     mu.Unlock();
//   }
//                           r3: {
//                             mu.Lock();
//                             code_r3();
//                             mu.Unlock();
//                           }
// We have 3 regions of code such that
// - two of them are in one thread and 3-rd in another thread.
// - all 3 regions have the same lockset,
// - the distance between r1 and r2 is small,
// - there is no h-b arc between r2 and r3,
// - r1 and r2 have different stack traces,
//
// In this situation we report a 'Suspected atomicity violation'.
//
// Current status:
// this code detects atomicity violations on our two motivating examples
// (--gtest_filter=*Atomicity*  --gtest_also_run_disabled_tests) and does
// not overwhelm with false reports.
// However, this functionality is still raw and not tuned for performance.

// TS_ATOMICITY is on in debug mode or if we enabled it at the build time.
#ifndef TS_ATOMICITY
# define TS_ATOMICITY DEBUG_MODE
#endif


struct AtomicityRegion {
  int lock_era;
  TID tid;
  VTS *vts;
  StackTrace *stack_trace;
  LSID lsid[2];
  BitSet access_set[2];
  bool used;
  int n_mops_since_start;

  void Print() {
    Report("T%d era=%d nmss=%ld AtomicityRegion:\n  rd: %s\n  wr: %s\n  %s\n%s",
           tid.raw(),
           lock_era,
           n_mops_since_start,
           access_set[0].ToString().c_str(),
           access_set[1].ToString().c_str(),
           TwoLockSetsToString(lsid[false], lsid[true]).c_str(),
           stack_trace->ToString().c_str()
          );
  }
};

bool SimilarLockSetForAtomicity(AtomicityRegion *r1, AtomicityRegion *r2) {
  // Compare only reader locksets (in case one region took reader locks)
  return ((r1->lsid[0] == r2->lsid[0]));
}

static deque<AtomicityRegion *> *g_atomicity_regions;
static map<StackTrace *, int, StackTrace::Less> *reported_atomicity_stacks_;
const size_t kMaxAtomicityRegions = 8;

static void HandleAtomicityRegion(AtomicityRegion *atomicity_region) {
  if (!g_atomicity_regions) {
    g_atomicity_regions = new deque<AtomicityRegion*>;
    reported_atomicity_stacks_ = new map<StackTrace *, int, StackTrace::Less>;
  }

  if (g_atomicity_regions->size() >= kMaxAtomicityRegions) {
    AtomicityRegion *to_delete = g_atomicity_regions->back();
    g_atomicity_regions->pop_back();
    if (!to_delete->used) {
      VTS::Unref(to_delete->vts);
      StackTrace::Delete(to_delete->stack_trace);
      delete to_delete;
    }
  }
  g_atomicity_regions->push_front(atomicity_region);
  size_t n = g_atomicity_regions->size();

  if (0) {
    for (size_t i = 0; i < n; i++) {
      AtomicityRegion *r = (*g_atomicity_regions)[i];
      r->Print();
    }
  }

  AtomicityRegion *r3 = (*g_atomicity_regions)[0];
  for (size_t i = 1; i < n; i++) {
    AtomicityRegion *r2 = (*g_atomicity_regions)[i];
    if (r2->tid     != r3->tid &&
        SimilarLockSetForAtomicity(r2, r3) &&
        !VTS::HappensBeforeCached(r2->vts, r3->vts)) {
      for (size_t j = i + 1; j < n; j++) {
        AtomicityRegion *r1 = (*g_atomicity_regions)[j];
        if (r1->tid != r2->tid) continue;
        CHECK(r2->lock_era > r1->lock_era);
        if (r2->lock_era - r1->lock_era > 2) break;
        if (!SimilarLockSetForAtomicity(r1, r2)) continue;
        if (StackTrace::Equals(r1->stack_trace, r2->stack_trace)) continue;
        if (!(r1->access_set[1].empty() &&
              !r2->access_set[1].empty() &&
              !r3->access_set[1].empty())) continue;
        CHECK(r1->n_mops_since_start <= r2->n_mops_since_start);
        if (r2->n_mops_since_start - r1->n_mops_since_start > 5) continue;
        if ((*reported_atomicity_stacks_)[r1->stack_trace] > 0) continue;

        (*reported_atomicity_stacks_)[r1->stack_trace]++;
        (*reported_atomicity_stacks_)[r2->stack_trace]++;
        (*reported_atomicity_stacks_)[r3->stack_trace]++;
        r1->used = r2->used = r3->used = true;
        ThreadSanitizerAtomicityViolationReport *report =
            new ThreadSanitizerAtomicityViolationReport;
        report->type = ThreadSanitizerReport::ATOMICITY_VIOLATION;
        report->tid = TID(0);
        report->stack_trace = r1->stack_trace;
        report->r1 = r1;
        report->r2 = r2;
        report->r3 = r3;
        ThreadSanitizerPrintReport(report);
        break;
      }
    }
  }
}

// -------- TSanThread ------------------ {{{1
struct TSanThread {
 public:
  ThreadLocalStats stats;

  TSanThread(TID tid, TID parent_tid, VTS *vts, StackTrace *creation_context,
         CallStack *call_stack)
    : is_running_(true),
      tid_(tid),
      sid_(0),
      parent_tid_(parent_tid),
      max_sp_(0),
      min_sp_(0),
      stack_size_for_ignore_(0),
      fun_r_ignore_(0),
      min_sp_for_ignore_(0),
      n_mops_since_start_(0),
      creation_context_(creation_context),
      announced_(false),
      rd_lockset_(0),
      wr_lockset_(0),
      expensive_bits_(0),
      vts_at_exit_(NULL),
      call_stack_(call_stack),
      lock_history_(128),
      recent_segments_cache_(G_flags->recent_segments_cache_size),
      inside_atomic_op_(),
      rand_state_((unsigned)(tid.raw() + (uintptr_t)vts
                      + (uintptr_t)creation_context
                      + (uintptr_t)call_stack)) {

    NewSegmentWithoutUnrefingOld("TSanThread Creation", vts);
    ignore_depth_[0] = ignore_depth_[1] = 0;

    HandleRtnCall(0, 0, IGNORE_BELOW_RTN_UNKNOWN);
    ignore_context_[0] = NULL;
    ignore_context_[1] = NULL;
    if (tid != TID(0) && parent_tid.valid()) {
      CHECK(creation_context_);
    }

    // Add myself to the array of threads.
    CHECK(tid.raw() < G_flags->max_n_threads);
    CHECK(all_threads_[tid.raw()] == NULL);
    n_threads_ = max(n_threads_, tid.raw() + 1);
    all_threads_[tid.raw()] = this;
    dead_sids_.reserve(kMaxNumDeadSids);
    fresh_sids_.reserve(kMaxNumFreshSids);
    ComputeExpensiveBits();
  }

  TID tid() const { return tid_; }
  TID parent_tid() const { return parent_tid_; }

  void increment_n_mops_since_start() {
    n_mops_since_start_++;
  }

  // STACK
  uintptr_t max_sp() const { return max_sp_; }
  uintptr_t min_sp() const { return min_sp_; }

  unsigned random() {
    return tsan_prng(&rand_state_);
  }

  bool ShouldReportRaces() const {
    return (inside_atomic_op_ == 0);
  }

  void SetStack(uintptr_t stack_min, uintptr_t stack_max) {
    CHECK(stack_min < stack_max);
    // Stay sane. Expect stack less than 64M.
    CHECK(stack_max - stack_min <= 64 * 1024 * 1024);
    min_sp_ = stack_min;
    max_sp_ = stack_max;
    if (G_flags->ignore_stack) {
      min_sp_for_ignore_ = min_sp_;
      stack_size_for_ignore_ = max_sp_ - min_sp_;
    } else {
      CHECK(min_sp_for_ignore_ == 0 &&
            stack_size_for_ignore_ == 0);
    }
  }

  bool MemoryIsInStack(uintptr_t a) {
    return a >= min_sp_ && a <= max_sp_;
  }

  bool IgnoreMemoryIfInStack(uintptr_t a) {
    return (a - min_sp_for_ignore_) < stack_size_for_ignore_;
  }


  bool Announce() {
    if (announced_) return false;
    announced_ = true;
    if (tid_ == TID(0)) {
      Report("INFO: T0 is program's main thread\n");
    } else {
      if (G_flags->announce_threads) {
        Report("INFO: T%d has been created by T%d at this point: {{{\n%s}}}\n",
               tid_.raw(), parent_tid_.raw(),
               creation_context_->ToString().c_str());
        TSanThread * parent = GetIfExists(parent_tid_);
        CHECK(parent);
        parent->Announce();
      } else {
        Report("INFO: T%d has been created by T%d. "
               "Use --announce-threads to see the creation stack.\n",
               tid_.raw(), parent_tid_.raw());
      }
    }
    return true;
  }

  string ThreadName() const {
    char buff[100];
    snprintf(buff, sizeof(buff), "T%d", tid().raw());
    string res = buff;
    if (thread_name_.length() > 0) {
      res += " (";
      res += thread_name_;
      res += ")";
    }
    return res;
  }

  bool is_running() const { return is_running_; }

  INLINE void ComputeExpensiveBits() {
    bool has_expensive_flags = G_flags->trace_level > 0 ||
        G_flags->show_stats > 1                      ||
        G_flags->sample_events > 0;

    expensive_bits_ =
        (ignore_depth_[0] != 0) |
        ((ignore_depth_[1] != 0) << 1) |
        ((has_expensive_flags == true) << 2);
  }

  int expensive_bits() { return expensive_bits_; }
  int ignore_reads() { return expensive_bits() & 1; }
  int ignore_writes() { return (expensive_bits() >> 1) & 1; }

  // ignore
  INLINE void set_ignore_accesses(bool is_w, bool on) {
    ignore_depth_[is_w] += on ? 1 : -1;
    CHECK(ignore_depth_[is_w] >= 0);
    ComputeExpensiveBits();
    if (on && G_flags->save_ignore_context) {
      StackTrace::Delete(ignore_context_[is_w]);
      ignore_context_[is_w] = CreateStackTrace(0, 3);
    }
  }
  INLINE void set_ignore_all_accesses(bool on) {
    set_ignore_accesses(false, on);
    set_ignore_accesses(true, on);
  }

  StackTrace *GetLastIgnoreContext(bool is_w) {
    return ignore_context_[is_w];
  }

  SID sid() const {
    return sid_;
  }

  Segment *segment() const {
    CHECK(sid().valid());
    Segment::AssertLive(sid(), __LINE__);
    return Segment::Get(sid());
  }

  VTS *vts() const {
    return segment()->vts();
  }

  void set_thread_name(const char *name) {
    thread_name_ = string(name);
  }

  void HandleThreadEnd() {
    CHECK(is_running_);
    is_running_ = false;
    CHECK(!vts_at_exit_);
    vts_at_exit_ = vts()->Clone();
    CHECK(vts_at_exit_);
    FlushDeadSids();
    ReleaseFreshSids();
    call_stack_ = NULL;
  }

  // Return the TID of the joined child and it's vts
  TID HandleThreadJoinAfter(VTS **vts_at_exit, TID joined_tid) {
    CHECK(joined_tid.raw() > 0);
    CHECK(GetIfExists(joined_tid) != NULL);
    TSanThread* joined_thread  = TSanThread::Get(joined_tid);
    // Sometimes the joined thread is not truly dead yet.
    // In that case we just take the current vts.
    if (joined_thread->is_running_)
      *vts_at_exit = joined_thread->vts()->Clone();
    else
      *vts_at_exit = joined_thread->vts_at_exit_;

    if (*vts_at_exit == NULL) {
      Printf("vts_at_exit==NULL; parent=%d, child=%d\n",
             tid().raw(), joined_tid.raw());
    }
    CHECK(*vts_at_exit);
    if (0)
    Printf("T%d: vts_at_exit_: %s\n", joined_tid.raw(),
           (*vts_at_exit)->ToString().c_str());
    return joined_tid;
  }

  static int NumberOfThreads() {
    return INTERNAL_ANNOTATE_UNPROTECTED_READ(n_threads_);
  }

  static TSanThread *GetIfExists(TID tid) {
    if (tid.raw() < NumberOfThreads())
      return Get(tid);
    return NULL;
  }

  static TSanThread *Get(TID tid) {
    DCHECK(tid.raw() < NumberOfThreads());
    return all_threads_[tid.raw()];
  }

  void HandleAccessSet() {
    BitSet *rd_set = lock_era_access_set(false);
    BitSet *wr_set = lock_era_access_set(true);
    if (rd_set->empty() && wr_set->empty()) return;
    CHECK(G_flags->atomicity && !G_flags->pure_happens_before);
    AtomicityRegion *atomicity_region = new AtomicityRegion;
    atomicity_region->lock_era = g_lock_era;
    atomicity_region->tid = tid();
    atomicity_region->vts = vts()->Clone();
    atomicity_region->lsid[0] = lsid(0);
    atomicity_region->lsid[1] = lsid(1);
    atomicity_region->access_set[0] = *rd_set;
    atomicity_region->access_set[1] = *wr_set;
    atomicity_region->stack_trace = CreateStackTrace();
    atomicity_region->used = false;
    atomicity_region->n_mops_since_start = this->n_mops_since_start_;
    // atomicity_region->Print();
    // Printf("----------- %s\n", __FUNCTION__);
    // ReportStackTrace(0, 7);
    HandleAtomicityRegion(atomicity_region);
  }

  // Locks
  void HandleLock(uintptr_t lock_addr, bool is_w_lock) {
    Lock *lock = Lock::LookupOrCreate(lock_addr);

    if (debug_lock) {
      Printf("T%d lid=%d %sLock   %p; %s\n",
           tid_.raw(), lock->lid().raw(),
           is_w_lock ? "Wr" : "Rd",
           lock_addr,
           LockSet::ToString(lsid(is_w_lock)).c_str());

      ReportStackTrace(0, 7);
    }

    // NOTE: we assume that all locks can be acquired recurively.
    // No warning about recursive locking will be issued.
    if (is_w_lock) {
      // Recursive locks are properly handled because LockSet is in fact a
      // multiset.
      wr_lockset_ = LockSet::Add(wr_lockset_, lock);
      rd_lockset_ = LockSet::Add(rd_lockset_, lock);
      lock->WrLock(tid_, CreateStackTrace());
    } else {
      if (lock->wr_held()) {
        ReportStackTrace();
      }
      rd_lockset_ = LockSet::Add(rd_lockset_, lock);
      lock->RdLock(CreateStackTrace());
    }

    if (lock->is_pure_happens_before()) {
      if (is_w_lock) {
        HandleWait(lock->wr_signal_addr());
      } else {
        HandleWait(lock->rd_signal_addr());
      }
    }

    if (G_flags->suggest_happens_before_arcs) {
      lock_history_.OnLock(lock->lid());
    }
    NewSegmentForLockingEvent();
    lock_era_access_set_[0].Clear();
    lock_era_access_set_[1].Clear();
  }

  void HandleUnlock(uintptr_t lock_addr) {
    HandleAccessSet();

    Lock *lock = Lock::Lookup(lock_addr);
    // If the lock is not found, report an error.
    if (lock == NULL) {
      ThreadSanitizerInvalidLockReport *report =
          new ThreadSanitizerInvalidLockReport;
      report->type = ThreadSanitizerReport::INVALID_LOCK;
      report->tid = tid();
      report->lock_addr = lock_addr;
      report->stack_trace = CreateStackTrace();
      ThreadSanitizerPrintReport(report);
      return;
    }
    bool is_w_lock = lock->wr_held();

    if (debug_lock) {
      Printf("T%d lid=%d %sUnlock %p; %s\n",
             tid_.raw(), lock->lid().raw(),
             is_w_lock ? "Wr" : "Rd",
             lock_addr,
             LockSet::ToString(lsid(is_w_lock)).c_str());
      ReportStackTrace(0, 7);
    }

    if (lock->is_pure_happens_before()) {
      // reader unlock signals only to writer lock,
      // writer unlock signals to both.
      if (is_w_lock) {
        HandleSignal(lock->rd_signal_addr());
      }
      HandleSignal(lock->wr_signal_addr());
    }

    if (!lock->wr_held() && !lock->rd_held()) {
      ThreadSanitizerBadUnlockReport *report =
          new ThreadSanitizerBadUnlockReport;
      report->type = ThreadSanitizerReport::UNLOCK_NONLOCKED;
      report->tid = tid();
      report->lid = lock->lid();
      report->stack_trace = CreateStackTrace();
      ThreadSanitizerPrintReport(report);
      return;
    }

    bool removed = false;
    if (is_w_lock) {
      lock->WrUnlock();
      removed =  LockSet::Remove(wr_lockset_, lock, &wr_lockset_)
              && LockSet::Remove(rd_lockset_, lock, &rd_lockset_);
    } else {
      lock->RdUnlock();
      removed = LockSet::Remove(rd_lockset_, lock, &rd_lockset_);
    }

    if (!removed) {
      ThreadSanitizerBadUnlockReport *report =
          new ThreadSanitizerBadUnlockReport;
      report->type = ThreadSanitizerReport::UNLOCK_FOREIGN;
      report->tid = tid();
      report->lid = lock->lid();
      report->stack_trace = CreateStackTrace();
      ThreadSanitizerPrintReport(report);
    }

    if (G_flags->suggest_happens_before_arcs) {
      lock_history_.OnUnlock(lock->lid());
    }

    NewSegmentForLockingEvent();
    lock_era_access_set_[0].Clear();
    lock_era_access_set_[1].Clear();
  }

  // Handles memory access with race reports suppressed.
  void HandleAtomicMop(uintptr_t a,
                       uintptr_t pc,
                       tsan_atomic_op op,
                       tsan_memory_order mo,
                       size_t size);

  void HandleForgetSignaller(uintptr_t cv) {
    SignallerMap::iterator it = signaller_map_->find(cv);
    if (it != signaller_map_->end()) {
      if (debug_happens_before) {
        Printf("T%d: ForgetSignaller: %p:\n    %s\n", tid_.raw(), cv,
            (it->second.vts)->ToString().c_str());
        if (G_flags->debug_level >= 1) {
          ReportStackTrace();
        }
      }
      VTS::Unref(it->second.vts);
      signaller_map_->erase(it);
    }
  }

  LSID lsid(bool is_w) {
    return is_w ? wr_lockset_ : rd_lockset_;
  }

  const LockHistory &lock_history() { return lock_history_; }

  // SIGNAL/WAIT events.
  void HandleWait(uintptr_t cv) {

    SignallerMap::iterator it = signaller_map_->find(cv);
    if (it != signaller_map_->end()) {
      const VTS *signaller_vts = it->second.vts;
      NewSegmentForWait(signaller_vts);
    }

    if (debug_happens_before) {
      Printf("T%d: Wait: %p:\n    %s %s\n", tid_.raw(),
             cv,
             vts()->ToString().c_str(),
             Segment::ToString(sid()).c_str());
      if (G_flags->debug_level >= 1) {
        ReportStackTrace();
      }
    }
  }

  void HandleSignal(uintptr_t cv) {
    Signaller *signaller = &(*signaller_map_)[cv];
    if (!signaller->vts) {
      signaller->vts = vts()->Clone();
    } else {
      VTS *new_vts = VTS::Join(signaller->vts, vts());
      VTS::Unref(signaller->vts);
      signaller->vts = new_vts;
    }
    NewSegmentForSignal();
    if (debug_happens_before) {
      Printf("T%d: Signal: %p:\n    %s %s\n    %s\n", tid_.raw(), cv,
             vts()->ToString().c_str(), Segment::ToString(sid()).c_str(),
             (signaller->vts)->ToString().c_str());
      if (G_flags->debug_level >= 1) {
        ReportStackTrace();
      }
    }
  }

  void INLINE NewSegmentWithoutUnrefingOld(const char *call_site,
                                           VTS *new_vts) {
    DCHECK(new_vts);
    SID new_sid = Segment::AddNewSegment(tid(), new_vts,
                                         rd_lockset_, wr_lockset_);
    SID old_sid = sid();
    if (old_sid.raw() != 0 && new_vts != vts()) {
      // Flush the cache if VTS changed - the VTS won't repeat.
      recent_segments_cache_.Clear();
    }
    sid_ = new_sid;
    Segment::Ref(new_sid, "TSanThread::NewSegmentWithoutUnrefingOld");

    if (kSizeOfHistoryStackTrace > 0) {
      FillEmbeddedStackTrace(Segment::embedded_stack_trace(sid()));
    }
    if (0)
    Printf("2: %s T%d/S%d old_sid=%d NewSegment: %s\n", call_site,
           tid().raw(), sid().raw(), old_sid.raw(),
         vts()->ToString().c_str());
  }

  void INLINE NewSegment(const char *call_site, VTS *new_vts) {
    SID old_sid = sid();
    NewSegmentWithoutUnrefingOld(call_site, new_vts);
    Segment::Unref(old_sid, "TSanThread::NewSegment");
  }

  void NewSegmentForLockingEvent() {
    // Flush the cache since we can't reuse segments with different lockset.
    recent_segments_cache_.Clear();
    NewSegment(__FUNCTION__, vts()->Clone());
  }

  void NewSegmentForMallocEvent() {
    // Flush the cache since we can't reuse segments with different lockset.
    recent_segments_cache_.Clear();
    NewSegment(__FUNCTION__, vts()->Clone());
  }


  void SetTopPc(uintptr_t pc) {
    if (pc) {
      DCHECK(!call_stack_->empty());
      call_stack_->back() = pc;
    }
  }

  void NOINLINE HandleSblockEnterSlowLocked() {
    AssertTILHeld();
    FlushStateIfOutOfSegments(this);
    this->stats.history_creates_new_segment++;
    VTS *new_vts = vts()->Clone();
    NewSegment("HandleSblockEnter", new_vts);
    recent_segments_cache_.Push(sid());
    GetSomeFreshSids();  // fill the thread-local SID cache.
  }

  INLINE bool HandleSblockEnter(uintptr_t pc, bool allow_slow_path) {
    DCHECK(G_flags->keep_history);
    if (!pc) return true;

    this->stats.events[SBLOCK_ENTER]++;

    SetTopPc(pc);

    bool refill_stack = false;
    SID match = recent_segments_cache_.Search(call_stack_, sid(),
                                              /*OUT*/&refill_stack);
    DCHECK(kSizeOfHistoryStackTrace > 0);

    if (match.valid()) {
      // This part is 100% thread-local, no need for locking.
      if (sid_ != match) {
        Segment::Ref(match, "TSanThread::HandleSblockEnter");
        this->AddDeadSid(sid_, "TSanThread::HandleSblockEnter");
        sid_ = match;
      }
      if (refill_stack) {
        this->stats.history_reuses_segment++;
        FillEmbeddedStackTrace(Segment::embedded_stack_trace(sid()));
      } else {
        this->stats.history_uses_same_segment++;
      }
    } else if (fresh_sids_.size() > 0) {
      // We have a fresh ready-to-use segment in thread local cache.
      SID fresh_sid = fresh_sids_.back();
      fresh_sids_.pop_back();
      Segment::SetupFreshSid(fresh_sid, tid(), vts()->Clone(),
                             rd_lockset_, wr_lockset_);
      this->AddDeadSid(sid_, "TSanThread::HandleSblockEnter-1");
      Segment::Ref(fresh_sid, "TSanThread::HandleSblockEnter-1");
      sid_ = fresh_sid;
      recent_segments_cache_.Push(sid());
      FillEmbeddedStackTrace(Segment::embedded_stack_trace(sid()));
      this->stats.history_uses_preallocated_segment++;
    } else {
      if (!allow_slow_path) return false;
      AssertTILHeld();
      // No fresh SIDs available, have to grab a lock and get few.
      HandleSblockEnterSlowLocked();
    }
    return true;
  }

  void NewSegmentForWait(const VTS *signaller_vts) {
    const VTS *current_vts   = vts();
    if (0)
    Printf("T%d NewSegmentForWait: \n  %s\n  %s\n", tid().raw(),
           current_vts->ToString().c_str(),
           signaller_vts->ToString().c_str());
    // We don't want to create a happens-before arc if it will be redundant.
    if (!VTS::HappensBeforeCached(signaller_vts, current_vts)) {
      VTS *new_vts = VTS::Join(current_vts, signaller_vts);
      NewSegment("NewSegmentForWait", new_vts);
    }
    DCHECK(VTS::HappensBeforeCached(signaller_vts, vts()));
  }

  void NewSegmentForSignal() {
    VTS *cur_vts = vts();
    VTS *new_vts = VTS::CopyAndTick(cur_vts, tid());
    NewSegment("NewSegmentForSignal", new_vts);
  }

  // When creating a child thread, we need to know
  // 1. where the thread was created (ctx)
  // 2. What was the vector clock of the parent thread (vts).

  struct ThreadCreateInfo {
    StackTrace *ctx;
    VTS        *vts;
  };

  static void StopIgnoringAccessesInT0BecauseNewThreadStarted() {
    AssertTILHeld();
    if (g_so_far_only_one_thread) {
      g_so_far_only_one_thread = false;
      Get(TID(0))->set_ignore_all_accesses(false);
    }
  }

  // This event comes before the child is created (e.g. just
  // as we entered pthread_create).
  void HandleThreadCreateBefore(TID parent_tid, uintptr_t pc) {
    CHECK(parent_tid == tid());
    StopIgnoringAccessesInT0BecauseNewThreadStarted();
    // Store ctx and vts under TID(0).
    ThreadCreateInfo info;
    info.ctx = CreateStackTrace(pc);
    info.vts = vts()->Clone();
    CHECK(info.ctx && info.vts);
    child_tid_to_create_info_[TID(0)] = info;
    // Tick vts.
    this->NewSegmentForSignal();

    if (debug_thread) {
      Printf("T%d: THR_CREATE_BEFORE\n", parent_tid.raw());
    }
  }

  // This event comes when we are exiting the thread creation routine.
  // It may appear before *or* after THR_START event, at least with PIN.
  void HandleThreadCreateAfter(TID parent_tid, TID child_tid) {
    CHECK(parent_tid == tid());
    // Place the info under child_tid if we did not use it yet.
    if (child_tid_to_create_info_.count(TID(0))){
      child_tid_to_create_info_[child_tid] = child_tid_to_create_info_[TID(0)];
      child_tid_to_create_info_.erase(TID(0));
    }

    if (debug_thread) {
      Printf("T%d: THR_CREATE_AFTER %d\n", parent_tid.raw(), child_tid.raw());
    }
  }

  void HandleChildThreadStart(TID child_tid, VTS **vts, StackTrace **ctx) {
    TSanThread *parent = this;
    ThreadCreateInfo info;
    if (child_tid_to_create_info_.count(child_tid)) {
      // We already seen THR_CREATE_AFTER, so the info is under child_tid.
      info = child_tid_to_create_info_[child_tid];
      child_tid_to_create_info_.erase(child_tid);
      CHECK(info.ctx && info.vts);
    } else if (child_tid_to_create_info_.count(TID(0))){
      // We have not seen THR_CREATE_AFTER, but already seen THR_CREATE_BEFORE.
      info = child_tid_to_create_info_[TID(0)];
      child_tid_to_create_info_.erase(TID(0));
      CHECK(info.ctx && info.vts);
    } else {
      // We have not seen THR_CREATE_BEFORE/THR_CREATE_AFTER.
      // If the tool is single-threaded (valgrind) these events are redundant.
      info.ctx = parent->CreateStackTrace();
      info.vts = parent->vts()->Clone();
      parent->NewSegmentForSignal();
    }
    *ctx = info.ctx;
    VTS *singleton = VTS::CreateSingleton(child_tid);
    *vts = VTS::Join(singleton, info.vts);
    VTS::Unref(singleton);
    VTS::Unref(info.vts);


    if (debug_thread) {
      Printf("T%d: THR_START parent: T%d : %s %s\n", child_tid.raw(),
             parent->tid().raw(),
             parent->vts()->ToString().c_str(),
             (*vts)->ToString().c_str());
      if (G_flags->announce_threads) {
        Printf("%s\n", (*ctx)->ToString().c_str());
      }
    }

    // Parent should have ticked its VTS so there should be no h-b.
    DCHECK(!VTS::HappensBefore(parent->vts(), *vts));
  }

  // Support for Cyclic Barrier, e.g. pthread_barrier_t.
  // We need to create (barrier_count-1)^2 h-b arcs between
  // threads blocking on a barrier. We should not create any h-b arcs
  // for two calls to barrier_wait if the barrier was reset between then.
  struct CyclicBarrierInfo {
    // The value given to barrier_init.
    uint32_t barrier_count;
    // How many times we may block on this barrier before resetting.
    int32_t calls_before_reset;
    // How many times we entered the 'wait-before' and 'wait-after' handlers.
    int32_t n_wait_before, n_wait_after;
  };
  // The following situation is possible:
  // - N threads blocked on a barrier.
  // - All N threads reached the barrier and we started getting 'wait-after'
  //   events, but did not yet get all of them.
  // - N threads blocked on the barrier again and we started getting
  //   'wait-before' events from the next barrier epoch.
  // - We continue getting 'wait-after' events from the previous epoch.
  //
  // We don't want to create h-b arcs between barrier events of different
  // epochs, so we use 'barrier + (epoch % 4)' as an object on which we
  // signal and wait (it is unlikely that more than 4 epochs are live at once.
  enum { kNumberOfPossibleBarrierEpochsLiveAtOnce = 4 };
  // Maps the barrier pointer to CyclicBarrierInfo.
  typedef unordered_map<uintptr_t, CyclicBarrierInfo> CyclicBarrierMap;

  CyclicBarrierInfo &GetCyclicBarrierInfo(uintptr_t barrier) {
    if (cyclic_barrier_map_ == NULL) {
      cyclic_barrier_map_ = new CyclicBarrierMap;
    }
    return (*cyclic_barrier_map_)[barrier];
  }

  void HandleBarrierInit(uintptr_t barrier, uint32_t n) {
    CyclicBarrierInfo &info = GetCyclicBarrierInfo(barrier);
    CHECK(n > 0);
    memset(&info, 0, sizeof(CyclicBarrierInfo));
    info.barrier_count = n;
  }

  void HandleBarrierWaitBefore(uintptr_t barrier) {
    CyclicBarrierInfo &info = GetCyclicBarrierInfo(barrier);

    CHECK(info.calls_before_reset >= 0);
    int32_t epoch = info.n_wait_before / info.barrier_count;
    epoch %= kNumberOfPossibleBarrierEpochsLiveAtOnce;
    info.n_wait_before++;
    if (info.calls_before_reset == 0) {
      // We are blocking the first time after reset. Clear the VTS.
      info.calls_before_reset = info.barrier_count;
      Signaller &signaller = (*signaller_map_)[barrier + epoch];
      VTS::Unref(signaller.vts);
      signaller.vts = NULL;
      if (debug_happens_before) {
        Printf("T%d barrier %p (epoch %d) reset\n", tid().raw(),
               barrier, epoch);
      }
    }
    info.calls_before_reset--;
    // Signal to all threads that blocked on this barrier.
    if (debug_happens_before) {
      Printf("T%d barrier %p (epoch %d) wait before\n", tid().raw(),
             barrier, epoch);
    }
    HandleSignal(barrier + epoch);
  }

  void HandleBarrierWaitAfter(uintptr_t barrier) {
    CyclicBarrierInfo &info = GetCyclicBarrierInfo(barrier);
    int32_t epoch = info.n_wait_after / info.barrier_count;
    epoch %= kNumberOfPossibleBarrierEpochsLiveAtOnce;
    info.n_wait_after++;
    if (debug_happens_before) {
      Printf("T%d barrier %p (epoch %d) wait after\n", tid().raw(),
             barrier, epoch);
    }
    HandleWait(barrier + epoch);
  }

  // Call stack  -------------
  void PopCallStack() {
    CHECK(!call_stack_->empty());
    call_stack_->pop_back();
  }

  void HandleRtnCall(uintptr_t call_pc, uintptr_t target_pc,
                     IGNORE_BELOW_RTN ignore_below) {
    this->stats.events[RTN_CALL]++;
    if (!call_stack_->empty() && call_pc) {
      call_stack_->back() = call_pc;
    }
    call_stack_->push_back(target_pc);

    bool ignore = false;
    if (ignore_below == IGNORE_BELOW_RTN_UNKNOWN) {
      if (ignore_below_cache_.Lookup(target_pc, &ignore) == false) {
        ignore = ThreadSanitizerIgnoreAccessesBelowFunction(target_pc);
        ignore_below_cache_.Insert(target_pc, ignore);
        G_stats->ignore_below_cache_miss++;
      } else {
        // Just in case, check the result of caching.
        DCHECK(ignore ==
               ThreadSanitizerIgnoreAccessesBelowFunction(target_pc));
      }
    } else {
      DCHECK(ignore_below == IGNORE_BELOW_RTN_YES ||
             ignore_below == IGNORE_BELOW_RTN_NO);
      ignore = ignore_below == IGNORE_BELOW_RTN_YES;
    }

    if (fun_r_ignore_) {
      fun_r_ignore_++;
    } else if (ignore) {
      fun_r_ignore_ = 1;
      set_ignore_all_accesses(true);
    }
  }

  void HandleRtnExit() {
    this->stats.events[RTN_EXIT]++;
    if (!call_stack_->empty()) {
      call_stack_->pop_back();
      if (fun_r_ignore_) {
        if (--fun_r_ignore_ == 0) {
          set_ignore_all_accesses(false);
        }
      }
    }
  }

  uintptr_t GetCallstackEntry(size_t offset_from_top) {
    if (offset_from_top >= call_stack_->size()) return 0;
    return (*call_stack_)[call_stack_->size() - offset_from_top - 1];
  }

  string CallStackRtnName(size_t offset_from_top = 0) {
    if (call_stack_->size() <= offset_from_top)
      return "";
    uintptr_t pc = (*call_stack_)[call_stack_->size() - offset_from_top - 1];
    return PcToRtnName(pc, false);
  }

  string CallStackToStringRtnOnly(int len) {
    string res;
    for (int i = 0; i < len; i++) {
      if (i)
        res += " ";
      res += CallStackRtnName(i);
    }
    return res;
  }

  uintptr_t CallStackTopPc() {
    if (call_stack_->empty())
      return 0;
    return call_stack_->back();
  }

  INLINE void FillEmbeddedStackTrace(uintptr_t *emb_trace) {
    size_t size = min(call_stack_->size(), (size_t)kSizeOfHistoryStackTrace);
    size_t idx = call_stack_->size() - 1;
    uintptr_t *pcs = call_stack_->pcs();
    for (size_t i = 0; i < size; i++, idx--) {
      emb_trace[i] = pcs[idx];
    }
    if (size < (size_t) kSizeOfHistoryStackTrace) {
      emb_trace[size] = 0;
    }
  }

  INLINE void FillStackTrace(StackTrace *trace, size_t size) {
    size_t idx = call_stack_->size() - 1;
    uintptr_t *pcs = call_stack_->pcs();
    for (size_t i = 0; i < size; i++, idx--) {
      trace->Set(i, pcs[idx]);
    }
  }

  INLINE StackTrace *CreateStackTrace(uintptr_t pc = 0,
                                      int max_len = -1,
                                      int capacity = 0) {
    if (!call_stack_->empty() && pc) {
      call_stack_->back() = pc;
    }
    if (max_len <= 0) {
      max_len = G_flags->num_callers;
    }
    int size = call_stack_->size();
    if (size > max_len)
      size = max_len;
    StackTrace *res = StackTrace::CreateNewEmptyStackTrace(size, capacity);
    FillStackTrace(res, size);
    return res;
  }

  void ReportStackTrace(uintptr_t pc = 0, int max_len = -1) {
    StackTrace *trace = CreateStackTrace(pc, max_len);
    Report("%s", trace->ToString().c_str());
    StackTrace::Delete(trace);
  }

  static void ForgetAllState() {
    // G_flags->debug_level = 2;
    for (int i = 0; i < TSanThread::NumberOfThreads(); i++) {
      TSanThread *thr = Get(TID(i));
      thr->recent_segments_cache_.ForgetAllState();
      thr->sid_ = SID();  // Reset the old SID so we don't try to read its VTS.
      VTS *singleton_vts = VTS::CreateSingleton(TID(i), 2);
      if (thr->is_running()) {
        thr->NewSegmentWithoutUnrefingOld("ForgetAllState", singleton_vts);
      }
      for (map<TID, ThreadCreateInfo>::iterator j =
               thr->child_tid_to_create_info_.begin();
           j != thr->child_tid_to_create_info_.end(); ++j) {
        ThreadCreateInfo &info = j->second;
        VTS::Unref(info.vts);
        // The parent's VTS should neither happen-before nor equal the child's.
        info.vts = VTS::CreateSingleton(TID(i), 1);
      }
      if (thr->vts_at_exit_) {
        VTS::Unref(thr->vts_at_exit_);
        thr->vts_at_exit_ = singleton_vts->Clone();
      }
      thr->dead_sids_.clear();
      thr->fresh_sids_.clear();
    }
    signaller_map_->ClearAndDeleteElements();
  }

  static void InitClassMembers() {
    ScopedMallocCostCenter malloc_cc("InitClassMembers");
    all_threads_        = new TSanThread*[G_flags->max_n_threads];
    memset(all_threads_, 0, sizeof(TSanThread*) * G_flags->max_n_threads);
    n_threads_          = 0;
    signaller_map_      = new SignallerMap;
  }

  BitSet *lock_era_access_set(int is_w) {
    return &lock_era_access_set_[is_w];
  }

  // --------- dead SIDs, fresh SIDs
  // When running fast path w/o a lock we need to recycle SIDs to a thread-local
  // pool. HasRoomForDeadSids and AddDeadSid may be called w/o a lock.
  // FlushDeadSids should be called under a lock.
  // When creating a new segment on SBLOCK_ENTER, we need to get a fresh SID
  // from somewhere. We keep a pile of fresh ready-to-use SIDs in
  // a thread-local array.
  enum { kMaxNumDeadSids = 64,
         kMaxNumFreshSids = 256, };
  INLINE void AddDeadSid(SID sid, const char *where) {
    if (TS_SERIALIZED) {
      Segment::Unref(sid, where);
    } else {
      if (Segment::UnrefNoRecycle(sid, where) == 0) {
        dead_sids_.push_back(sid);
      }
    }
  }

  INLINE void FlushDeadSids() {
    if (TS_SERIALIZED) return;
    size_t n = dead_sids_.size();
    for (size_t i = 0; i < n; i++) {
      SID sid = dead_sids_[i];
      Segment::AssertLive(sid, __LINE__);
      DCHECK(Segment::Get(sid)->ref_count() == 0);
      Segment::RecycleOneSid(sid);
    }
    dead_sids_.clear();
  }

  INLINE bool HasRoomForDeadSids() const {
    return TS_SERIALIZED ? false :
        dead_sids_.size() < kMaxNumDeadSids - 2;
  }

  void GetSomeFreshSids() {
    size_t cur_size = fresh_sids_.size();
    DCHECK(cur_size <= kMaxNumFreshSids);
    if (cur_size > kMaxNumFreshSids / 2) {
      // We already have quite a few fresh SIDs, do nothing.
      return;
    }
    DCHECK(fresh_sids_.capacity() >= kMaxNumFreshSids);
    size_t n_requested_sids = kMaxNumFreshSids - cur_size;
    fresh_sids_.resize(kMaxNumFreshSids);
    Segment::AllocateFreshSegments(n_requested_sids, &fresh_sids_[cur_size]);
  }

  void ReleaseFreshSids() {
    for (size_t i = 0; i < fresh_sids_.size(); i++) {
      Segment::RecycleOneFreshSid(fresh_sids_[i]);
    }
    fresh_sids_.clear();
  }

 private:
  bool is_running_;
  string thread_name_;

  TID    tid_;         // This thread's tid.
  SID    sid_;         // Current segment ID.
  TID    parent_tid_;  // Parent's tid.
  bool   thread_local_copy_of_g_has_expensive_flags_;
  uintptr_t  max_sp_;
  uintptr_t  min_sp_;
  uintptr_t  stack_size_for_ignore_;
  uintptr_t  fun_r_ignore_;  // > 0 if we are inside a fun_r-ed function.
  uintptr_t  min_sp_for_ignore_;
  uintptr_t  n_mops_since_start_;
  StackTrace *creation_context_;
  bool      announced_;

  LSID   rd_lockset_;
  LSID   wr_lockset_;

  // These bits should be read in the hottest loop, so we combine them all
  // together.
  // bit 1 -- ignore reads.
  // bit 2 -- ignore writes.
  // bit 3 -- have expensive flags
  int expensive_bits_;
  int ignore_depth_[2];
  StackTrace *ignore_context_[2];

  VTS *vts_at_exit_;

  CallStack *call_stack_;

  vector<SID> dead_sids_;
  vector<SID> fresh_sids_;

  PtrToBoolCache<251> ignore_below_cache_;

  LockHistory lock_history_;
  BitSet lock_era_access_set_[2];
  RecentSegmentsCache recent_segments_cache_;

  map<TID, ThreadCreateInfo> child_tid_to_create_info_;

  // This var is used to suppress race reports
  // when handling atomic memory accesses.
  // That is, an atomic memory access can't race with other accesses,
  // however plain memory accesses can race with atomic memory accesses.
  int inside_atomic_op_;

  prng_t rand_state_;

  struct Signaller {
    VTS *vts;
  };

  class SignallerMap: public unordered_map<uintptr_t, Signaller> {
    public:
     void ClearAndDeleteElements() {
       for (iterator it = begin(); it != end(); ++it) {
         VTS::Unref(it->second.vts);
       }
       clear();
     }
  };

  // All threads. The main thread has tid 0.
  static TSanThread **all_threads_;
  static int      n_threads_;

  // signaller address -> VTS
  static SignallerMap *signaller_map_;
  static CyclicBarrierMap *cyclic_barrier_map_;
};

INLINE static int32_t raw_tid(TSanThread *t) {
  return t->tid().raw();
}

// TSanThread:: static members
TSanThread                    **TSanThread::all_threads_;
int                         TSanThread::n_threads_;
TSanThread::SignallerMap       *TSanThread::signaller_map_;
TSanThread::CyclicBarrierMap   *TSanThread::cyclic_barrier_map_;


// -------- TsanAtomicCore ------------------ {{{1

// Responsible for handling of atomic memory accesses.
class TsanAtomicCore {
 public:
  TsanAtomicCore();

  void HandleWrite(TSanThread* thr,
                   uintptr_t a,
                   uint64_t v,
                   uint64_t prev,
                   bool is_acquire,
                   bool is_release,
                   bool is_rmw);

  uint64_t HandleRead(TSanThread* thr,
                      uintptr_t a,
                      uint64_t v,
                      bool is_acquire);

  void ClearMemoryState(uintptr_t a, uintptr_t b);

 private:
  // Represents one value in modification history
  // of an atomic variable.
  struct AtomicHistoryEntry {
    // Actual value.
    // (atomics of size more than uint64_t are not supported as of now)
    uint64_t val;
    // ID of a thread that did the modification.
    TID tid;
    // The thread's clock during the modification.
    int32_t clk;
    // Vector clock that is acquired by a thread
    // that loads the value.
    // Similar to Signaller::vts.
    VTS* vts;
  };

  // Descriptor of an atomic variable.
  struct Atomic {
    // Number of stored entries in the modification order of the variable.
    // This represents space-modelling preciseness trade-off.
    // 4 values should be generally enough.
    static int32_t const kHistSize = 4;
    // Current position in the modification order.
    int32_t hist_pos;
    // Modification history organized as a circular buffer.
    // That is, old values are discarded.
    AtomicHistoryEntry hist [kHistSize];
    // It's basically a tid->hist_pos map that tracks what threads
    // had seen what values. It's required to meet the following requirement:
    // even relaxed loads must not be reordered in a single thread.
    VectorClock last_seen;

    Atomic();
    void reset(bool init = false);
  };

  typedef map<uintptr_t, Atomic> AtomicMap;
  AtomicMap atomic_map_;

  void AtomicFixHist(Atomic* atomic,
                     uint64_t prev);

  TsanAtomicCore(TsanAtomicCore const&);
  void operator=(TsanAtomicCore const&);
};


static TsanAtomicCore* g_atomicCore;


// -------- Clear Memory State ------------------ {{{1
static void INLINE UnrefSegmentsInMemoryRange(uintptr_t a, uintptr_t b,
                                                Mask mask, CacheLine *line) {
  while (!mask.Empty()) {
    uintptr_t x = mask.GetSomeSetBit();
    DCHECK(mask.Get(x));
    mask.Clear(x);
    line->GetValuePointer(x)->Unref("Detector::UnrefSegmentsInMemoryRange");
  }
}

void INLINE ClearMemoryStateInOneLine(TSanThread *thr, uintptr_t addr,
                                      uintptr_t beg, uintptr_t end) {
  AssertTILHeld();
  CacheLine *line = G_cache->GetLineIfExists(thr, addr, __LINE__);
  // CacheLine *line = G_cache->GetLineOrCreateNew(addr, __LINE__);
  if (line) {
    DCHECK(beg < CacheLine::kLineSize);
    DCHECK(end <= CacheLine::kLineSize);
    DCHECK(beg < end);
    Mask published = line->published();
    if (UNLIKELY(!published.Empty())) {
      Mask mask(published.GetRange(beg, end));
      ClearPublishedAttribute(line, mask);
    }
    Mask old_used = line->ClearRangeAndReturnOldUsed(beg, end);
    UnrefSegmentsInMemoryRange(beg, end, old_used, line);
    G_cache->ReleaseLine(thr, addr, line, __LINE__);
  }
}

// clear memory state for [a,b)
void NOINLINE ClearMemoryState(TSanThread *thr, uintptr_t a, uintptr_t b) {
  if (a == b) return;
  CHECK(a < b);
  uintptr_t line1_tag = 0, line2_tag = 0;
  uintptr_t single_line_tag = GetCacheLinesForRange(a, b,
                                                    &line1_tag, &line2_tag);
  if (single_line_tag) {
    ClearMemoryStateInOneLine(thr, a, a - single_line_tag,
                              b - single_line_tag);
    return;
  }

  uintptr_t a_tag = CacheLine::ComputeTag(a);
  ClearMemoryStateInOneLine(thr, a, a - a_tag, CacheLine::kLineSize);

  for (uintptr_t tag_i = line1_tag; tag_i < line2_tag;
       tag_i += CacheLine::kLineSize) {
    ClearMemoryStateInOneLine(thr, tag_i, 0, CacheLine::kLineSize);
  }

  if (b > line2_tag) {
    ClearMemoryStateInOneLine(thr, line2_tag, 0, b - line2_tag);
  }

  if (DEBUG_MODE && G_flags->debug_level >= 2) {
    // Check that we've cleared it. Slow!
    for (uintptr_t x = a; x < b; x++) {
      uintptr_t off = CacheLine::ComputeOffset(x);
      (void)off;
      CacheLine *line = G_cache->GetLineOrCreateNew(thr, x, __LINE__);
      CHECK(!line->has_shadow_value().Get(off));
      G_cache->ReleaseLine(thr, x, line, __LINE__);
    }
  }

  g_atomicCore->ClearMemoryState(a, b);
}

// -------- PCQ --------------------- {{{1
struct PCQ {
  uintptr_t pcq_addr;
  deque<VTS*> putters;
};

typedef map<uintptr_t, PCQ> PCQMap;
static PCQMap *g_pcq_map;

// -------- Heap info ---------------------- {{{1
#include "ts_heap_info.h"
// Information about heap memory.

struct HeapInfo {
  uintptr_t   ptr;
  uintptr_t   size;
  SID         sid;
  HeapInfo() : ptr(0), size(0), sid(0) { }

  Segment *seg() { return Segment::Get(sid); }
  TID tid() { return seg()->tid(); }
  string StackTraceString() { return Segment::StackTraceString(sid); }
};

static HeapMap<HeapInfo> *G_heap_map;

struct ThreadStackInfo {
  uintptr_t   ptr;
  uintptr_t   size;
  ThreadStackInfo() : ptr(0), size(0) { }
};

static HeapMap<ThreadStackInfo> *G_thread_stack_map;

// -------- Forget all state -------- {{{1
// We need to forget all state and start over because we've
// run out of some resources (most likely, segment IDs).
static void ForgetAllStateAndStartOver(TSanThread *thr, const char *reason) {
  // This is done under the main lock.
  AssertTILHeld();
  size_t start_time = g_last_flush_time = TimeInMilliSeconds();
  Report("T%d INFO: %s. Flushing state.\n", raw_tid(thr), reason);

  if (TS_SERIALIZED == 0) {
    // We own the lock, but we also must acquire all cache lines
    // so that the fast-path (unlocked) code does not execute while
    // we are flushing.
    G_cache->AcquireAllLines(thr);
  }


  if (0) {
    Report("INFO: Thread Sanitizer will now forget all history.\n");
    Report("INFO: This is experimental, and may fail!\n");
    if (G_flags->keep_history > 0) {
      Report("INFO: Consider re-running with --keep_history=0\n");
    }
    if (G_flags->show_stats) {
        G_stats->PrintStats();
    }
  }

  G_stats->n_forgets++;

  Segment::ForgetAllState();
  SegmentSet::ForgetAllState();
  TSanThread::ForgetAllState();
  VTS::FlushHBCache();

  G_heap_map->Clear();

  g_publish_info_map->clear();

  for (PCQMap::iterator it = g_pcq_map->begin(); it != g_pcq_map->end(); ++it) {
    PCQ &pcq = it->second;
    for (deque<VTS*>::iterator it2 = pcq.putters.begin();
         it2 != pcq.putters.end(); ++it2) {
      VTS::Unref(*it2);
      *it2 = VTS::CreateSingleton(TID(0), 1);
    }
  }

  // Must be the last one to flush as it effectively releases the
  // cach lines and enables fast path code to run in other threads.
  G_cache->ForgetAllState(thr);

  size_t stop_time = TimeInMilliSeconds();
  if (DEBUG_MODE || (stop_time - start_time > 0)) {
    Report("T%d INFO: Flush took %ld ms\n", raw_tid(thr),
           stop_time - start_time);
  }
}

static INLINE void FlushStateIfOutOfSegments(TSanThread *thr) {
  if (Segment::NumberOfSegments() > kMaxSIDBeforeFlush) {
    // too few sids left -- flush state.
    if (DEBUG_MODE) {
      G_cache->PrintStorageStats();
      Segment::ShowSegmentStats();
    }
    ForgetAllStateAndStartOver(thr, "run out of segment IDs");
  }
}

// -------- Expected Race ---------------------- {{{1
typedef  HeapMap<ExpectedRace> ExpectedRacesMap;
static ExpectedRacesMap *G_expected_races_map;
static bool g_expecting_races;
static int g_found_races_since_EXPECT_RACE_BEGIN;

ExpectedRace* ThreadSanitizerFindExpectedRace(uintptr_t addr) {
  return G_expected_races_map->GetInfo(addr);
}

// -------- Suppressions ----------------------- {{{1
static const char default_suppressions[] =
// TODO(kcc): as it gets bigger, move it into a separate object file.
"# We need to have some default suppressions, but we don't want to    \n"
"# keep them in a separate text file, so we keep the in the code.     \n"

#ifdef VGO_darwin
"{                                                                    \n"
"   dyld tries to unlock an invalid mutex when adding/removing image. \n"
"   ThreadSanitizer:InvalidLock                                       \n"
"   fun:pthread_mutex_unlock                                          \n"
"   fun:_dyld_register_func_for_*_image                               \n"
"}                                                                    \n"

"{                                                                      \n"
"  Benign reports in __NSOperationInternal when using workqueue threads \n"
"  ThreadSanitizer:Race                                                 \n"
"  fun:__+[__NSOperationInternal _observeValueForKeyPath:ofObject:changeKind:oldValue:newValue:indexes:context:]_block_invoke_*\n"
"  fun:_dispatch_call_block_and_release                                 \n"
"}                                                                      \n"

"{                                                                    \n"
"  Benign race in GCD when using workqueue threads.                   \n"
"  ThreadSanitizer:Race                                               \n"
"  fun:____startOperations_block_invoke_*                             \n"
"  ...                                                                \n"
"  fun:_dispatch_call_block_and_release                               \n"
"}                                                                    \n"

"{                                                                    \n"
"  Benign race in NSOQSchedule when using workqueue threads.          \n"
"  ThreadSanitizer:Race                                               \n"
"  fun:__doStart*                                                     \n"
"  ...                                                                \n"
"  fun:_dispatch_call_block_and_release                               \n"
"}                                                                    \n"


#endif

#ifndef _MSC_VER
"{                                                                   \n"
"  False reports on std::string internals. See TSan issue #40.       \n"
"  ThreadSanitizer:Race                                              \n"
"  ...                                                               \n"
"  fun:*~basic_string*                                               \n"
"}                                                                   \n"

"{                                                                   \n"
"  False reports on std::string internals. See TSan issue #40.       \n"
"  ThreadSanitizer:Race                                              \n"
"  ...                                                               \n"
"  fun:*basic_string*_M_destroy                                      \n"
"}                                                                   \n"

#else
"{                                                                   \n"
"  False lock report inside ntdll.dll                                \n"
"  ThreadSanitizer:InvalidLock                                       \n"
"  fun:*                                                             \n"
"  obj:*ntdll.dll                                                    \n"
"}                                                                   \n"

"{                                                                   \n"
"  False report due to lack of debug symbols in ntdll.dll  (a)       \n"
"  ThreadSanitizer:InvalidLock                                       \n"
"  fun:*SRWLock*                                                     \n"
"}                                                                   \n"

"{                                                                   \n"
"  False report due to lack of debug symbols in ntdll.dll  (b)       \n"
"  ThreadSanitizer:UnlockForeign                                     \n"
"  fun:*SRWLock*                                                     \n"
"}                                                                   \n"

"{                                                                   \n"
"  False report due to lack of debug symbols in ntdll.dll  (c)       \n"
"  ThreadSanitizer:UnlockNonLocked                                   \n"
"  fun:*SRWLock*                                                     \n"
"}                                                                   \n"

"{                                                                   \n"
"  False reports on std::string internals (2). See TSan issue #40.   \n"
"  ThreadSanitizer:Race                                              \n"
"  ...                                                               \n"
"  fun:*basic_string*scalar deleting destructor*                     \n"
"}                                                                   \n"
#endif

#ifdef TS_PIN
"{                                                                   \n"
"  Suppression for issue 54 (PIN lacks support for IFUNC)            \n"
"  ThreadSanitizer:Race                                              \n"
"  ...                                                               \n"
"  fun:*NegativeTests_Strlen::Worker*                                \n"
"}                                                                   \n"
#endif

;

// -------- Report Storage --------------------- {{{1
class ReportStorage {
 public:

  ReportStorage()
   : n_reports(0),
     n_race_reports(0),
     program_finished_(0),
     unwind_cb_(0) {
    if (G_flags->generate_suppressions) {
      Report("INFO: generate_suppressions = true\n");
    }
    // Read default suppressions
    int n = suppressions_.ReadFromString(default_suppressions);
    if (n == -1) {
      Report("Error reading default suppressions at line %d: %s\n",
          suppressions_.GetErrorLineNo(),
          suppressions_.GetErrorString().c_str());
      exit(1);
    }

    // Read user-supplied suppressions.
    for (size_t i = 0; i < G_flags->suppressions.size(); i++) {
      const string &supp_path = G_flags->suppressions[i];
      Report("INFO: reading suppressions file %s\n", supp_path.c_str());
      int n = suppressions_.ReadFromString(ReadFileToString(supp_path, true));
      if (n == -1) {
        Report("Error at line %d: %s\n",
            suppressions_.GetErrorLineNo(),
            suppressions_.GetErrorString().c_str());
        exit(1);
      }
      Report("INFO: %6d suppression(s) read from file %s\n",
             n, supp_path.c_str());
    }
  }

  bool NOINLINE AddReport(TSanThread *thr, uintptr_t pc, bool is_w, uintptr_t addr,
                          int size,
                          ShadowValue old_sval, ShadowValue new_sval,
                          bool is_published) {
    {
      // Check this isn't a "_ZNSs4_Rep20_S_empty_rep_storageE" report.
      uintptr_t offset;
      string symbol_descr;
      if (GetNameAndOffsetOfGlobalObject(addr, &symbol_descr, &offset)) {
        if (StringMatch("*empty_rep_storage*", symbol_descr))
          return false;
        if (StringMatch("_IO_stdfile_*_lock", symbol_descr))
          return false;
        if (StringMatch("_IO_*_stdout_", symbol_descr))
          return false;
        if (StringMatch("_IO_*_stderr_", symbol_descr))
          return false;
      }
    }

    bool is_expected = false;
    ExpectedRace *expected_race = G_expected_races_map->GetInfo(addr);
    if (debug_expected_races) {
      Printf("Checking expected race for %lx; exp_race=%p\n",
             addr, expected_race);
      if (expected_race) {
        Printf("  FOUND\n");
      }
    }

    if (expected_race) {
      if (G_flags->nacl_untrusted != expected_race->is_nacl_untrusted) {
        Report("WARNING: this race is only expected in NaCl %strusted mode\n",
            expected_race->is_nacl_untrusted ? "un" : "");
      } else {
        is_expected = true;
        expected_race->count++;
      }
    }

    if (g_expecting_races) {
      is_expected = true;
      g_found_races_since_EXPECT_RACE_BEGIN++;
    }

    if (is_expected && !G_flags->show_expected_races) return false;

    StackTrace *stack_trace = thr->CreateStackTrace(pc);
    if (unwind_cb_) {
      int const maxcnt = 256;
      uintptr_t cur_stack [maxcnt];
      int cnt = unwind_cb_(cur_stack, maxcnt, pc);
      if (cnt > 0 && cnt <= maxcnt) {
        cnt = min<int>(cnt, stack_trace->capacity());
        stack_trace->set_size(cnt);
        for (int i = 0; i < cnt; i++)
          stack_trace->Set(i, cur_stack[i]);
      }
    }
    int n_reports_for_this_context = reported_stacks_[stack_trace]++;

    if (n_reports_for_this_context > 0) {
      // we already reported a race here.
      StackTrace::Delete(stack_trace);
      return false;
    }


    ThreadSanitizerDataRaceReport *race_report =
        new ThreadSanitizerDataRaceReport;

    race_report->type = ThreadSanitizerReport::DATA_RACE;
    race_report->new_sval = new_sval;
    race_report->old_sval = old_sval;
    race_report->is_expected = is_expected;
    race_report->last_access_is_w = is_w;
    race_report->racey_addr = addr;
    race_report->racey_addr_description = DescribeMemory(addr);
    race_report->last_access_tid = thr->tid();
    race_report->last_access_sid = thr->sid();
    race_report->last_access_size = size;
    race_report->stack_trace = stack_trace;
    race_report->racey_addr_was_published = is_published;
    race_report->last_acces_lsid[false] = thr->lsid(false);
    race_report->last_acces_lsid[true] = thr->lsid(true);

    Segment *seg = Segment::Get(thr->sid());
    (void)seg;
    CHECK(thr->lsid(false) == seg->lsid(false));
    CHECK(thr->lsid(true) == seg->lsid(true));

    return ThreadSanitizerPrintReport(race_report);
  }

  void AnnounceThreadsInSegmentSet(SSID ssid) {
    if (ssid.IsEmpty()) return;
    for (int s = 0; s < SegmentSet::Size(ssid); s++) {
      Segment *seg = SegmentSet::GetSegmentForNonSingleton(ssid, s, __LINE__);
      TSanThread::Get(seg->tid())->Announce();
    }
  }



  void PrintConcurrentSegmentSet(SSID ssid, TID tid, SID sid,
                                 LSID lsid, bool is_w,
                                 const char *descr, set<LID> *locks,
                                 set<SID>* concurrent_sids) {
    if (ssid.IsEmpty()) return;
    bool printed_header = false;
    TSanThread *thr1 = TSanThread::Get(tid);
    for (int s = 0; s < SegmentSet::Size(ssid); s++) {
      SID concurrent_sid = SegmentSet::GetSID(ssid, s, __LINE__);
      Segment *seg = Segment::Get(concurrent_sid);
      if (Segment::HappensBeforeOrSameThread(concurrent_sid, sid)) continue;
      if (!LockSet::IntersectionIsEmpty(lsid, seg->lsid(is_w))) continue;
      if (concurrent_sids) {
        concurrent_sids->insert(concurrent_sid);
      }
      TSanThread *thr2 = TSanThread::Get(seg->tid());
      if (!printed_header) {
        Report("  %sConcurrent %s happened at (OR AFTER) these points:%s\n",
               c_magenta, descr, c_default);
        printed_header = true;
      }

      Report("   %s (%s):\n",
             thr2->ThreadName().c_str(),
             TwoLockSetsToString(seg->lsid(false),
                                 seg->lsid(true)).c_str());
      if (G_flags->show_states) {
        Report("   S%d\n", concurrent_sid.raw());
      }
      LockSet::AddLocksToSet(seg->lsid(false), locks);
      LockSet::AddLocksToSet(seg->lsid(true), locks);
      Report("%s", Segment::StackTraceString(concurrent_sid).c_str());
      if (!G_flags->pure_happens_before &&
          G_flags->suggest_happens_before_arcs) {
        set<LID> message_locks;
        // Report("Locks in T%d\n", thr1->tid().raw());
        // thr1->lock_history().PrintLocks();
        // Report("Unlocks in T%d\n", thr2->tid().raw());
        // thr2->lock_history().PrintUnlocks();
        if (LockHistory::Intersect(thr1->lock_history(), thr2->lock_history(),
                                   seg->lock_era(), &message_locks)) {
          Report("   Note: these locks were recently released by T%d"
                 " and later acquired by T%d: {%s}\n"
                 "   See http://code.google.com/p/data-race-test/wiki/"
                 "PureHappensBeforeVsHybrid\n",
                 thr2->tid().raw(),
                 thr1->tid().raw(),
                 SetOfLocksToString(message_locks).c_str());
          locks->insert(message_locks.begin(), message_locks.end());
        }
      }
    }
  }

  void SetProgramFinished() {
    CHECK(!program_finished_);
    program_finished_ = true;
  }

  string RaceInfoString(uintptr_t pc, set<SID>& concurrent_sids) {
    string s;
    char buf[100];
    snprintf(buf, 100, "Race verifier data: %p", (void*)pc);
    s += buf;
    for (set<SID>::iterator it = concurrent_sids.begin();
         it != concurrent_sids.end(); ++it) {
      // Take the first pc of the concurrent stack trace.
      uintptr_t concurrent_pc = *Segment::embedded_stack_trace(*it);
      snprintf(buf, 100, ",%p", (void*)concurrent_pc);
      s += buf;
    }
    s += "\n";
    return s;
  }

  void PrintRaceReport(ThreadSanitizerDataRaceReport *race) {
    bool short_report = program_finished_;
    if (!short_report) {
      AnnounceThreadsInSegmentSet(race->new_sval.rd_ssid());
      AnnounceThreadsInSegmentSet(race->new_sval.wr_ssid());
    }
    bool is_w = race->last_access_is_w;
    TID     tid = race->last_access_tid;
    TSanThread *thr = TSanThread::Get(tid);
    SID     sid = race->last_access_sid;
    LSID    lsid = race->last_acces_lsid[is_w];
    set<LID> all_locks;

    n_race_reports++;
    if (G_flags->html) {
      Report("<b id=race%d>Race report #%d; </b>"
             "<a href=\"#race%d\">Next;</a>  "
             "<a href=\"#race%d\">Prev;</a>\n",
             n_race_reports, n_race_reports,
             n_race_reports+1, n_race_reports-1);
    }


    // Note the {{{ and }}}. These are for vim folds.
    Report("%sWARNING: %s data race during %s of size %d at %p: {{{%s\n",
           c_red,
           race->is_expected ? "Expected" : "Possible",
           is_w ? "write" : "read",
           race->last_access_size,
           race->racey_addr,
           c_default);
    if (!short_report) {
      LockSet::AddLocksToSet(race->last_acces_lsid[false], &all_locks);
      LockSet::AddLocksToSet(race->last_acces_lsid[true], &all_locks);
      Report("   %s (%s):\n",
             thr->ThreadName().c_str(),
             TwoLockSetsToString(race->last_acces_lsid[false],
                                 race->last_acces_lsid[true]).c_str());
    }

    CHECK(race->stack_trace);
    Report("%s", race->stack_trace->ToString().c_str());
    if (short_report) {
      Report(" See the full version of this report above.\n");
      Report("}%s\n", "}}");
      return;
    }
    // Report(" sid=%d; vts=%s\n", thr->sid().raw(),
    //       thr->vts()->ToString().c_str());
    if (G_flags->show_states) {
      Report(" old state: %s\n", race->old_sval.ToString().c_str());
      Report(" new state: %s\n", race->new_sval.ToString().c_str());
    }
    set<SID> concurrent_sids;
    if (G_flags->keep_history) {
      PrintConcurrentSegmentSet(race->new_sval.wr_ssid(),
                                tid, sid, lsid, true, "write(s)", &all_locks,
                                &concurrent_sids);
      if (is_w) {
        PrintConcurrentSegmentSet(race->new_sval.rd_ssid(),
                                  tid, sid, lsid, false, "read(s)", &all_locks,
                                  &concurrent_sids);
      }
    } else {
      Report("  %sAccess history is disabled. "
             "Consider running with --keep-history=1 for better reports.%s\n",
             c_cyan, c_default);
    }

    if (race->racey_addr_was_published) {
      Report(" This memory was published\n");
    }
    if (race->racey_addr_description.size() > 0) {
      Report("%s", race->racey_addr_description.c_str());
    }
    if (race->is_expected) {
      ExpectedRace *expected_race =
          G_expected_races_map->GetInfo(race->racey_addr);
      if (expected_race) {
        CHECK(expected_race->description);
        Report(" Description: \"%s\"\n", expected_race->description);
      }
    }
    set<LID>  locks_reported;

    if (!all_locks.empty()) {
      Report("  %sLocks involved in this report "
             "(reporting last lock sites):%s {%s}\n",
             c_green, c_default,
             SetOfLocksToString(all_locks).c_str());

      for (set<LID>::iterator it = all_locks.begin();
           it != all_locks.end(); ++it) {
        LID lid = *it;
        Lock::ReportLockWithOrWithoutContext(lid, true);
      }
    }

    string raceInfoString = RaceInfoString(race->stack_trace->Get(0),
        concurrent_sids);
    Report("   %s", raceInfoString.c_str());
    Report("}}}\n");
  }

  bool PrintReport(ThreadSanitizerReport *report) {
    CHECK(report);
    // Check if we have a suppression.
    vector<string> funcs_mangled;
    vector<string> funcs_demangled;
    vector<string> objects;

    CHECK(!g_race_verifier_active);
    CHECK(report->stack_trace);
    CHECK(report->stack_trace->size());
    for (size_t i = 0; i < report->stack_trace->size(); i++) {
      uintptr_t pc = report->stack_trace->Get(i);
      string img, rtn, file;
      int line;
      PcToStrings(pc, false, &img, &rtn, &file, &line);
      if (rtn == "(below main)" || rtn == "ThreadSanitizerStartThread")
        break;

      funcs_mangled.push_back(rtn);
      funcs_demangled.push_back(NormalizeFunctionName(PcToRtnName(pc, true)));
      objects.push_back(img);

      if (rtn == "main")
        break;
    }
    string suppression_name;
    if (suppressions_.StackTraceSuppressed("ThreadSanitizer",
                                           report->ReportName(),
                                           funcs_mangled,
                                           funcs_demangled,
                                           objects,
                                           &suppression_name)) {
      used_suppressions_[suppression_name]++;
      return false;
    }

    // Actually print it.
    if (report->type == ThreadSanitizerReport::UNLOCK_FOREIGN) {
      ThreadSanitizerBadUnlockReport *bad_unlock =
          reinterpret_cast<ThreadSanitizerBadUnlockReport*>(report);
      Report("WARNING: Lock %s was released by thread T%d"
             " which did not acquire this lock: {{{\n%s}}}\n",
             Lock::ToString(bad_unlock->lid).c_str(),
             bad_unlock->tid.raw(),
             bad_unlock->stack_trace->ToString().c_str());
    } else if (report->type == ThreadSanitizerReport::UNLOCK_NONLOCKED) {
      ThreadSanitizerBadUnlockReport *bad_unlock =
          reinterpret_cast<ThreadSanitizerBadUnlockReport*>(report);
      Report("WARNING: Unlocking a non-locked lock %s in thread T%d: "
             "{{{\n%s}}}\n",
             Lock::ToString(bad_unlock->lid).c_str(),
             bad_unlock->tid.raw(),
             bad_unlock->stack_trace->ToString().c_str());
    } else if (report->type == ThreadSanitizerReport::INVALID_LOCK) {
      ThreadSanitizerInvalidLockReport *invalid_lock =
          reinterpret_cast<ThreadSanitizerInvalidLockReport*>(report);
      Report("WARNING: accessing an invalid lock %p in thread T%d: "
             "{{{\n%s}}}\n",
             invalid_lock->lock_addr,
             invalid_lock->tid.raw(),
             invalid_lock->stack_trace->ToString().c_str());
    } else if (report->type == ThreadSanitizerReport::ATOMICITY_VIOLATION) {
      ThreadSanitizerAtomicityViolationReport *av =
          reinterpret_cast<ThreadSanitizerAtomicityViolationReport*>(report);
      Report("WARNING: Suspected atomicity violation {{{\n");
      av->r1->Print();
      av->r2->Print();
      av->r3->Print();
      Report("}}}\n");

    } else {
      CHECK(report->type == ThreadSanitizerReport::DATA_RACE);
      ThreadSanitizerDataRaceReport *race =
          reinterpret_cast<ThreadSanitizerDataRaceReport*>(report);
      PrintRaceReport(race);
    }

    n_reports++;
    SetNumberOfFoundErrors(n_reports);
    if (!G_flags->summary_file.empty()) {
      char buff[100];
      snprintf(buff, sizeof(buff),
               "ThreadSanitizer: %d warning(s) reported\n", n_reports);
      // We overwrite the contents of this file with the new summary.
      // We don't do that at the end because even if we crash later
      // we will already have the summary.
      OpenFileWriteStringAndClose(G_flags->summary_file, buff);
    }

    // Generate a suppression.
    if (G_flags->generate_suppressions) {
      string supp = "{\n";
      supp += "  <Put your suppression name here>\n";
      supp += string("  ThreadSanitizer:") + report->ReportName() + "\n";
      for (size_t i = 0; i < funcs_mangled.size(); i++) {
        const string &func = funcs_demangled[i];
        if (func.size() == 0 || func == "(no symbols") {
          supp += "  obj:" + objects[i] + "\n";
        } else {
          supp += "  fun:" + funcs_demangled[i] + "\n";
        }
        if (StackTrace::CutStackBelowFunc(funcs_demangled[i])) {
          break;
        }
      }
      supp += "}";
      Printf("------- suppression -------\n%s\n------- end suppression -------\n",
             supp.c_str());
    }

    return true;
  }

  void PrintUsedSuppression() {
    for (map<string, int>::iterator it = used_suppressions_.begin();
         it != used_suppressions_.end(); ++it) {
      Report("used_suppression: %d %s\n", it->second, it->first.c_str());
    }
  }

  void PrintSummary() {
    Report("ThreadSanitizer summary: reported %d warning(s) (%d race(s))\n",
           n_reports, n_race_reports);
  }


  string DescribeMemory(uintptr_t a) {
    const int kBufLen = 1023;
    char buff[kBufLen+1];

    // Is this stack?
    for (int i = 0; i < TSanThread::NumberOfThreads(); i++) {
      TSanThread *t = TSanThread::Get(TID(i));
      if (!t || !t->is_running()) continue;
      if (t->MemoryIsInStack(a)) {
        snprintf(buff, sizeof(buff),
                 "  %sLocation %p is %ld bytes inside T%d's stack [%p,%p]%s\n",
                 c_blue,
                 reinterpret_cast<void*>(a),
                 static_cast<long>(t->max_sp() - a),
                 i,
                 reinterpret_cast<void*>(t->min_sp()),
                 reinterpret_cast<void*>(t->max_sp()),
                 c_default
                );
        return buff;
      }
    }

    HeapInfo *heap_info = G_heap_map->GetInfo(a);
    if (heap_info) {
      snprintf(buff, sizeof(buff),
             "  %sLocation %p is %ld bytes inside a block starting at %p"
             " of size %ld allocated by T%d from heap:%s\n",
             c_blue,
             reinterpret_cast<void*>(a),
             static_cast<long>(a - heap_info->ptr),
             reinterpret_cast<void*>(heap_info->ptr),
             static_cast<long>(heap_info->size),
             heap_info->tid().raw(), c_default);
      return string(buff) + heap_info->StackTraceString().c_str();
    }


    // Is it a global object?
    uintptr_t offset;
    string symbol_descr;
    if (GetNameAndOffsetOfGlobalObject(a, &symbol_descr, &offset)) {
      snprintf(buff, sizeof(buff),
              "  %sAddress %p is %d bytes inside data symbol \"",
              c_blue, reinterpret_cast<void*>(a), static_cast<int>(offset));
      return buff + symbol_descr + "\"" + c_default + "\n";
    }

    if (G_flags->debug_level >= 2) {
      string res;
      // Is this near stack?
      for (int i = 0; i < TSanThread::NumberOfThreads(); i++) {
        TSanThread *t = TSanThread::Get(TID(i));
        const uintptr_t kMaxStackDiff = 1024 * 16;
        uintptr_t diff1 = a - t->max_sp();
        uintptr_t diff2 = t->min_sp() - a;
        if (diff1 < kMaxStackDiff ||
            diff2 < kMaxStackDiff ||
            t->MemoryIsInStack(a)) {
          uintptr_t diff = t->MemoryIsInStack(a) ? 0 :
              (diff1 < kMaxStackDiff ? diff1 : diff2);
          snprintf(buff, sizeof(buff),
                   "  %sLocation %p is within %d bytes outside T%d's stack [%p,%p]%s\n",
                   c_blue,
                   reinterpret_cast<void*>(a),
                   static_cast<int>(diff),
                   i,
                   reinterpret_cast<void*>(t->min_sp()),
                   reinterpret_cast<void*>(t->max_sp()),
                   c_default
                  );
          res += buff;
        }
      }
      if (res.size() > 0) {
        return res +
            "  This report _may_ indicate that valgrind incorrectly "
            "computed the stack boundaries\n";
      }
    }

    return "";
  }

  void SetUnwindCallback(ThreadSanitizerUnwindCallback cb) {
    unwind_cb_ = cb;
  }

 private:
  map<StackTrace *, int, StackTrace::Less> reported_stacks_;
  int n_reports;
  int n_race_reports;
  bool program_finished_;
  Suppressions suppressions_;
  map<string, int> used_suppressions_;
  ThreadSanitizerUnwindCallback unwind_cb_;
};

// -------- Event Sampling ---------------- {{{1
// This class samples (profiles) events.
// Instances of this class should all be static.
class EventSampler {
 public:

  // Sample one event
  void Sample(TSanThread *thr, const char *event_name, bool need_locking) {
    CHECK_NE(G_flags->sample_events, 0);
    (counter_)++;
    if ((counter_ & ((1 << G_flags->sample_events) - 1)) != 0)
      return;

    TIL til(ts_lock, 8, need_locking);
    string pos = thr->CallStackToStringRtnOnly(G_flags->sample_events_depth);
    (*samples_)[event_name][pos]++;
    total_samples_++;
    if (total_samples_ >= print_after_this_number_of_samples_) {
      print_after_this_number_of_samples_ +=
          print_after_this_number_of_samples_ / 2;
      ShowSamples();
    }
  }

  // Show existing samples
  static void ShowSamples() {
    if (G_flags->sample_events == 0) return;
    Printf("ShowSamples: (all samples: %lld)\n", total_samples_);
    for (SampleMapMap::iterator it1 = samples_->begin();
         it1 != samples_->end(); ++it1) {
      string name = it1->first;
      SampleMap &m = it1->second;
      int total = 0;
      for (SampleMap::iterator it2 = m.begin(); it2 != m.end(); it2++) {
        total += it2->second;
      }

      map<int, string> reverted_map;
      for (SampleMap::iterator it2 = m.begin(); it2 != m.end(); it2++) {
        int n_samples = it2->second;
        if (n_samples * 1000 < total) continue;
        reverted_map[n_samples] = it2->first;
      }
      Printf("%s: total samples %'d (~%'lld events)\n", name.c_str(),
             total,
             (int64_t)total << G_flags->sample_events);
      for (map<int, string>::iterator it = reverted_map.begin();
           it != reverted_map.end(); ++it) {
        Printf("%s: %d samples (~%d%%) %s\n", name.c_str(), it->first,
               (it->first * 100) / total, it->second.c_str());
      }
      Printf("\n");
    }
  }

  static void InitClassMembers() {
    samples_ = new SampleMapMap;
    total_samples_ = 0;
    print_after_this_number_of_samples_ = 1000;
  }

 private:
  int counter_;

  typedef map<string, int> SampleMap;
  typedef map<string, SampleMap> SampleMapMap;
  static SampleMapMap *samples_;
  static int64_t total_samples_;
  static int64_t print_after_this_number_of_samples_;
};

EventSampler::SampleMapMap *EventSampler::samples_;
int64_t EventSampler::total_samples_;
int64_t EventSampler::print_after_this_number_of_samples_;

// -------- Detector ---------------------- {{{1
// Collection of event handlers.
class Detector {
 public:
  void INLINE HandleTraceLoop(TSanThread *thr, uintptr_t pc,
                              MopInfo *mops,
                              uintptr_t *tleb, size_t n,
                              int expensive_bits, bool need_locking) {
    bool has_expensive_flags = (expensive_bits & 4) != 0;
    size_t i = 0;
    uintptr_t sblock_pc = pc;
    size_t n_locks = 0;
    do {
      uintptr_t addr = tleb[i];
      if (addr == 0) continue;  // This mop was not executed.
      MopInfo *mop = &mops[i];
      tleb[i] = 0;  // we've consumed this mop, clear it.
      DCHECK(mop->size() != 0);
      DCHECK(mop->pc() != 0);
      if ((expensive_bits & 1) && mop->is_write() == false) continue;
      if ((expensive_bits & 2) && mop->is_write() == true) continue;
      n_locks += HandleMemoryAccessInternal(thr, &sblock_pc, addr, mop,
                                 has_expensive_flags,
                                 need_locking);
    } while (++i < n);
    if (has_expensive_flags) {
      const size_t mop_stat_size = TS_ARRAY_SIZE(thr->stats.mops_per_trace);
      thr->stats.mops_per_trace[min(n, mop_stat_size - 1)]++;
      const size_t stat_size = TS_ARRAY_SIZE(thr->stats.locks_per_trace);
      thr->stats.locks_per_trace[min(n_locks, stat_size - 1)]++;
    }
  }

#ifdef _MSC_VER
  NOINLINE
  // With MSVC, INLINE would cause the compilation to be insanely slow.
#else
  INLINE
#endif
  void HandleTrace(TSanThread *thr, MopInfo *mops, size_t n, uintptr_t pc,
                   uintptr_t *tleb, bool need_locking) {
    DCHECK(n);
    // 0 bit - ignore reads, 1 bit -- ignore writes,
    // 2 bit - has_expensive_flags.
    int expensive_bits = thr->expensive_bits();

    if (expensive_bits == 0) {
      HandleTraceLoop(thr, pc, mops, tleb, n, 0, need_locking);
    } else {
      if ((expensive_bits & 3) == 3) {
        // everything is ignored, just clear the tleb.
        for (size_t i = 0; i < n; i++) tleb[i] = 0;
      } else {
        HandleTraceLoop(thr, pc, mops, tleb, n, expensive_bits, need_locking);
      }
    }
    // At the end, the tleb must be cleared.
    for (size_t i = 0; i < n; i++) DCHECK(tleb[i] == 0);
  }

  // Special case of a trace with just one mop and no sblock.
  void INLINE HandleMemoryAccess(TSanThread *thr, uintptr_t pc,
                                 uintptr_t addr, uintptr_t size,
                                 bool is_w, bool need_locking) {
    CHECK(size);
    MopInfo mop(pc, size, is_w, false);
    HandleTrace(thr, &mop, 1, 0/*no sblock*/, &addr, need_locking);
  }

  void ShowUnfreedHeap() {
    // check if there is not deleted memory
    // (for debugging free() interceptors, not for leak detection)
    if (DEBUG_MODE && G_flags->debug_level >= 1) {
      for (HeapMap<HeapInfo>::iterator it = G_heap_map->begin();
           it != G_heap_map->end(); ++it) {
        HeapInfo &info = it->second;
        Printf("Not free()-ed memory: %p [%p, %p)\n%s\n",
               info.size, info.ptr, info.ptr + info.size,
               info.StackTraceString().c_str());
      }
    }
  }

  void FlushExpectedRaces(bool print_summary) {
    // Report("ThreadSanitizerValgrind: done\n");
    // check if we found all expected races (for unit tests only).
    static int total_missing = 0;
    int this_flush_missing = 0;
    for (ExpectedRacesMap::iterator it = G_expected_races_map->begin();
         it != G_expected_races_map->end(); ++it) {
      ExpectedRace race = it->second;
      if (debug_expected_races) {
        Printf("Checking if expected race fired: %p\n", race.ptr);
      }
      if (race.count == 0 &&
          !(g_race_verifier_active && !race.is_verifiable) &&
          (G_flags->nacl_untrusted == race.is_nacl_untrusted)) {
        ++this_flush_missing;
        Printf("Missing an expected race on %p: %s (annotated at %s)\n",
               it->first,
               race.description,
               PcToRtnNameAndFilePos(race.pc).c_str());
      }
    }

    if (this_flush_missing) {
      int n_errs = GetNumberOfFoundErrors();
      SetNumberOfFoundErrors(n_errs + this_flush_missing);
      total_missing += this_flush_missing;
    }
    G_expected_races_map->Clear();

    if (print_summary && total_missing > 0)
      Report("WARNING: %d expected race(s) NOT detected!\n", total_missing);
  }

  void HandleProgramEnd() {
    FlushExpectedRaces(true);
    // ShowUnfreedHeap();
    EventSampler::ShowSamples();
    ShowStats();
    TraceInfo::PrintTraceProfile();
    ShowProcSelfStatus();
    reports_.PrintUsedSuppression();
    reports_.PrintSummary();
    // Report("ThreadSanitizerValgrind: exiting\n");
  }

  void FlushIfOutOfMem(TSanThread *thr) {
    static int max_vm_size;
    static int soft_limit;
    const int hard_limit = G_flags->max_mem_in_mb;
    const int minimal_soft_limit = (hard_limit * 13) / 16;
    const int print_info_limit   = (hard_limit * 12) / 16;

    CHECK(hard_limit > 0);

    int vm_size_in_mb = GetVmSizeInMb();
    if (max_vm_size < vm_size_in_mb) {
      max_vm_size = vm_size_in_mb;
      if (max_vm_size > print_info_limit) {
        Report("INFO: ThreadSanitizer's VmSize: %dM\n", (int)max_vm_size);
      }
    }

    if (soft_limit == 0) {
      soft_limit = minimal_soft_limit;
    }

    if (vm_size_in_mb > soft_limit) {
      ForgetAllStateAndStartOver(thr,
          "ThreadSanitizer is running close to its memory limit");
      soft_limit = vm_size_in_mb + 1;
    }
  }

  // Force state flushing.
  void FlushState(TID tid) {
    ForgetAllStateAndStartOver(TSanThread::Get(tid), 
                               "State flushing requested by client");
  }

  void FlushIfNeeded(TSanThread *thr) {
    // Are we out of segment IDs?
#ifdef TS_VALGRIND  // GetVmSizeInMb() works only with valgrind any way.
    static int counter;
    counter++;  // ATTENTION: don't do this in multi-threaded code -- too slow.
    CHECK(TS_SERIALIZED == 1);

    // Are we out of memory?
    if (G_flags->max_mem_in_mb > 0) {
      const int kFreq = 1014 * 32;
      if ((counter % kFreq) == 0) {  // Don't do it too often.
        // TODO(kcc): find a way to check memory limit more frequently.
        TIL til(ts_lock, 7);
        AssertTILHeld();
        FlushIfOutOfMem(thr);
      }
    }
#if 0
    if ((counter % (1024 * 1024 * 64)) == 0 ||
        counter == (1024 * 1024)) {
      // ShowStats();
      EventSampler::ShowSamples();
      TraceInfo::PrintTraceProfile();
    }
#endif
#endif

#if 0  // do we still need it? Hope not..
    size_t flush_period = G_flags->flush_period * 1000;  // milliseconds.
    if (flush_period && (counter % (1024 * 4)) == 0) {
      size_t cur_time = TimeInMilliSeconds();
      if (cur_time - g_last_flush_time  > flush_period) {
        TIL til(ts_lock, 7);
        ForgetAllStateAndStartOver(
          "Doing periodic flush (period is set by --flush_period=n_seconds)");
      }
    }
#endif
  }

  void HandleRtnCall(TID tid, uintptr_t call_pc, uintptr_t target_pc,
                     IGNORE_BELOW_RTN ignore_below) {
    TSanThread *thr = TSanThread::Get(tid);
    thr->HandleRtnCall(call_pc, target_pc, ignore_below);
    FlushIfNeeded(thr);
  }

  void INLINE HandleOneEvent(Event *e) {
    ScopedMallocCostCenter malloc_cc("HandleOneEvent");

    DCHECK(e);
    EventType type = e->type();
    DCHECK(type != NOOP);
    TSanThread *thr = NULL;
    if (type != THR_START) {
      thr = TSanThread::Get(TID(e->tid()));
      DCHECK(thr);
      thr->SetTopPc(e->pc());
      thr->stats.events[type]++;
    }

    switch (type) {
      case READ:
        HandleMemoryAccess(thr, e->pc(), e->a(), e->info(), false, true);
        return;
      case WRITE:
        HandleMemoryAccess(thr, e->pc(), e->a(), e->info(), true, true);
        return;
      case RTN_CALL:
        HandleRtnCall(TID(e->tid()), e->pc(), e->a(),
                      IGNORE_BELOW_RTN_UNKNOWN);
        return;
      case RTN_EXIT:
        thr->HandleRtnExit();
        return;
      default: break;
    }

    // Everything else is under a lock.
    TIL til(ts_lock, 0);
    AssertTILHeld();


    if (UNLIKELY(type == THR_START)) {
        HandleThreadStart(TID(e->tid()), TID(e->info()), (CallStack*)e->pc());
        TSanThread::Get(TID(e->tid()))->stats.events[type]++;
        return;
    }

    FlushStateIfOutOfSegments(thr);

    // Since we have the lock, get some fresh SIDs.
    thr->GetSomeFreshSids();

    switch (type) {
      case THR_START   : CHECK(0); break;
        break;
      case SBLOCK_ENTER:
        if (thr->ignore_reads() && thr->ignore_writes()) break;
        thr->HandleSblockEnter(e->pc(), /*allow_slow_path=*/true);
        break;
      case THR_CREATE_BEFORE:
        thr->HandleThreadCreateBefore(TID(e->tid()), e->pc());
        break;
      case THR_CREATE_AFTER:
        thr->HandleThreadCreateAfter(TID(e->tid()), TID(e->info()));
        break;
      case THR_FIRST_INSN:
        HandleThreadFirstInsn(TID(e->tid()));
        break;
      case THR_JOIN_AFTER     : HandleThreadJoinAfter(e);   break;
      case THR_STACK_TOP      : HandleThreadStackTop(e); break;

      case THR_END     : HandleThreadEnd(TID(e->tid()));     break;
      case MALLOC      : HandleMalloc(e, false);     break;
      case FREE        : HandleFree(e);         break;
      case MMAP        : HandleMalloc(e, true);      break;  // same as MALLOC
      case MUNMAP      : HandleMunmap(e);     break;


      case WRITER_LOCK : thr->HandleLock(e->a(), true);     break;
      case READER_LOCK : thr->HandleLock(e->a(), false);    break;
      case UNLOCK      : thr->HandleUnlock(e->a());       break;
      case UNLOCK_OR_INIT : HandleUnlockOrInit(e); break;

      case LOCK_CREATE:
      case LOCK_DESTROY: HandleLockCreateOrDestroy(e); break;

      case SIGNAL      : thr->HandleSignal(e->a());  break;
      case WAIT        : thr->HandleWait(e->a());   break;

      case CYCLIC_BARRIER_INIT:
        thr->HandleBarrierInit(e->a(), e->info());
        break;
      case CYCLIC_BARRIER_WAIT_BEFORE  :
        thr->HandleBarrierWaitBefore(e->a());
        break;
      case CYCLIC_BARRIER_WAIT_AFTER  :
        thr->HandleBarrierWaitAfter(e->a());
        break;

      case PCQ_CREATE   : HandlePcqCreate(e);   break;
      case PCQ_DESTROY  : HandlePcqDestroy(e);  break;
      case PCQ_PUT      : HandlePcqPut(e);      break;
      case PCQ_GET      : HandlePcqGet(e);      break;


      case EXPECT_RACE :
        HandleExpectRace(e->a(), (const char*)e->pc(), TID(e->tid()));
        break;
      case BENIGN_RACE :
        HandleBenignRace(e->a(), e->info(),
                         (const char*)e->pc(), TID(e->tid()));
        break;
      case FLUSH_EXPECTED_RACES:
        FlushExpectedRaces(false);
        break;
      case EXPECT_RACE_BEGIN:
        CHECK(g_expecting_races == false);
        g_expecting_races = true;
        g_found_races_since_EXPECT_RACE_BEGIN = 0;
        break;
      case EXPECT_RACE_END:
        CHECK(g_expecting_races == true);
        g_expecting_races = false;
        if (g_found_races_since_EXPECT_RACE_BEGIN == 0) {
          int n_errs = GetNumberOfFoundErrors();
          SetNumberOfFoundErrors(n_errs + 1);
          Printf("WARNING: expected race not found.\n");
        }
        break;

      case HB_LOCK     : HandleHBLock(e);       break;
      case NON_HB_LOCK : HandleNonHBLock(e);    break;

      case IGNORE_READS_BEG:  HandleIgnore(e, false, true);  break;
      case IGNORE_READS_END:  HandleIgnore(e, false, false); break;
      case IGNORE_WRITES_BEG: HandleIgnore(e, true, true);   break;
      case IGNORE_WRITES_END: HandleIgnore(e, true, false);  break;

      case SET_THREAD_NAME:
        thr->set_thread_name((const char*)e->a());
        break;
      case SET_LOCK_NAME: {
          uintptr_t lock_addr = e->a();
          const char *name = reinterpret_cast<const char *>(e->info());
          Lock *lock = Lock::LookupOrCreate(lock_addr);
          lock->set_name(name);
        }
        break;

      case PUBLISH_RANGE : HandlePublishRange(e); break;
      case UNPUBLISH_RANGE :
        Report("WARNING: ANNOTATE_UNPUBLISH_MEMORY_RANGE is deprecated\n");
        break;

      case TRACE_MEM   : HandleTraceMem(e);   break;
      case STACK_TRACE : HandleStackTrace(e); break;
      case NOOP        : CHECK(0);           break;  // can't happen.
      case VERBOSITY   : e->Print(); G_flags->verbosity = e->info(); break;
      case FLUSH_STATE : FlushState(TID(e->tid()));       break;
      default                 : CHECK(0);    break;
    }
  }

 private:
  void ShowProcSelfStatus() {
    if (G_flags->show_proc_self_status) {
      string str = ReadFileToString("/proc/self/status", false);
      if (!str.empty()) {
        Printf("%s", str.c_str());
      }
    }
  }

  void ShowStats() {
    if (G_flags->show_stats) {
      G_stats->PrintStats();
      G_cache->PrintStorageStats();
    }
  }

  // PCQ_CREATE, PCQ_DESTROY, PCQ_PUT, PCQ_GET
  void HandlePcqCreate(Event *e) {
    if (G_flags->verbosity >= 2) {
      e->Print();
    }
    PCQ pcq;
    pcq.pcq_addr = e->a();
    CHECK(!g_pcq_map->count(e->a()));
    (*g_pcq_map)[e->a()] = pcq;
  }
  void HandlePcqDestroy(Event *e) {
    if (G_flags->verbosity >= 2) {
      e->Print();
    }
    CHECK(g_pcq_map->count(e->a()));
    g_pcq_map->erase(e->a());
  }
  void HandlePcqPut(Event *e) {
    if (G_flags->verbosity >= 2) {
      e->Print();
    }
    PCQ &pcq = (*g_pcq_map)[e->a()];
    CHECK(pcq.pcq_addr == e->a());
    TSanThread *thread = TSanThread::Get(TID(e->tid()));
    VTS *vts = thread->segment()->vts()->Clone();
    pcq.putters.push_back(vts);
    thread->NewSegmentForSignal();
  }
  void HandlePcqGet(Event *e) {
    if (G_flags->verbosity >= 2) {
      e->Print();
    }
    PCQ &pcq = (*g_pcq_map)[e->a()];
    CHECK(pcq.pcq_addr == e->a());
    CHECK(!pcq.putters.empty());
    VTS *putter = pcq.putters.front();
    pcq.putters.pop_front();
    CHECK(putter);
    TSanThread *thread = TSanThread::Get(TID(e->tid()));
    thread->NewSegmentForWait(putter);
    VTS::Unref(putter);
  }

  // PUBLISH_RANGE
  void HandlePublishRange(Event *e) {
    if (G_flags->verbosity >= 2) {
      e->Print();
    }
    static int reported_deprecation;
    reported_deprecation++;
    if (reported_deprecation < 20) {
      Report("WARNING: ANNOTATE_PUBLISH_MEMORY_RANGE is deprecated and will not"
             " be supported in future versions of ThreadSanitizer.\n");
    }

    uintptr_t mem = e->a();
    uintptr_t size = e->info();

    TID tid(e->tid());
    TSanThread *thread = TSanThread::Get(tid);
    VTS *vts = thread->segment()->vts();
    PublishRange(thread, mem, mem + size, vts);

    thread->NewSegmentForSignal();
    // Printf("Publish: [%p, %p)\n", mem, mem+size);
  }

  void HandleIgnore(Event *e, bool is_w, bool on) {
    if (G_flags->verbosity >= 2) {
      e->Print();
    }
    TSanThread *thread = TSanThread::Get(TID(e->tid()));
    thread->set_ignore_accesses(is_w, on);
  }

  // BENIGN_RACE
  void HandleBenignRace(uintptr_t ptr, uintptr_t size,
                        const char *descr, TID tid) {
    TSanThread *thr = TSanThread::Get(tid);
    if (debug_benign_races) {
      Printf("T%d: BENIGN_RACE: ptr=%p size=%ld descr='%s'\n",
             tid.raw(), ptr, size, descr);
    }
    // Simply set all 'racey' bits in the shadow state of [ptr, ptr+size).
    for (uintptr_t p = ptr; p < ptr + size; p++) {
      CacheLine *line = G_cache->GetLineOrCreateNew(thr, p, __LINE__);
      CHECK(line);
      line->racey().Set(CacheLine::ComputeOffset(p));
      G_cache->ReleaseLine(thr, p, line, __LINE__);
    }
  }

  // EXPECT_RACE
  void HandleExpectRace(uintptr_t ptr, const char *descr, TID tid) {
    ExpectedRace expected_race;
    expected_race.ptr = ptr;
    expected_race.size = 1;
    expected_race.count = 0;
    expected_race.is_verifiable = !descr ||
        (string(descr).find("UNVERIFIABLE") == string::npos);
    expected_race.is_nacl_untrusted = !descr ||
        (string(descr).find("NACL_UNTRUSTED") != string::npos);
    // copy descr (may not have strdup)
    CHECK(descr);
    size_t descr_len = strlen(descr);
    char *d = new char [descr_len + 1];
    memcpy(d, descr, descr_len);
    d[descr_len] = 0;
    expected_race.description = d;

    TSanThread *thread = TSanThread::Get(tid);
    expected_race.pc = thread->GetCallstackEntry(1);
    G_expected_races_map->InsertInfo(ptr, expected_race);

    // Flush 'racey' flag for the address
    CacheLine *cache_line = G_cache->GetLineIfExists(thread, ptr, __LINE__);
    if (cache_line != NULL) {
      uintptr_t offset = CacheLine::ComputeOffset(ptr);
      cache_line->racey().ClearRange(offset, offset + 1);
      G_cache->ReleaseLine(thread, ptr, cache_line, __LINE__);
    }

    if (debug_expected_races) {
      Printf("T%d: EXPECT_RACE: ptr=%p descr='%s'\n", tid.raw(), ptr, descr);
      thread->ReportStackTrace(ptr);
      int i = 0;
      for (ExpectedRacesMap::iterator it = G_expected_races_map->begin();
           it != G_expected_races_map->end(); ++it) {
        ExpectedRace &x = it->second;
        Printf("  [%d] %p [0x%lx]\n", i, &x, x.ptr);
        i++;
      }
    }
  }

  void HandleStackTrace(Event *e) {
    TSanThread *thread = TSanThread::Get(TID(e->tid()));
    e->Print();
    thread->ReportStackTrace();
  }

  // HB_LOCK
  void HandleHBLock(Event *e) {
    if (G_flags->verbosity >= 2) {
      e->Print();
    }
    Lock *lock = Lock::LookupOrCreate(e->a());
    CHECK(lock);
    lock->set_is_pure_happens_before(true);
  }

  // NON_HB_LOCK
  void HandleNonHBLock(Event *e) {
    if (G_flags->verbosity >= 2) {
      e->Print();
    }
    Lock *lock = Lock::LookupOrCreate(e->a());
    CHECK(lock);
    lock->set_is_pure_happens_before(false);
  }

  // UNLOCK_OR_INIT
  // This is a hack to handle posix pthread_spin_unlock which is sometimes
  // the same symbol as pthread_spin_init. We need to handle unlock as init
  // if the lock was not seen before or if it is currently unlocked.
  // TODO(kcc): is there a way to distinguish pthread_spin_init
  // and pthread_spin_unlock?
  void HandleUnlockOrInit(Event *e) {
    TSanThread *thread = TSanThread::Get(TID(e->tid()));
    if (G_flags->verbosity >= 2) {
      e->Print();
      thread->ReportStackTrace();
    }
    uintptr_t lock_addr = e->a();
    Lock *lock = Lock::Lookup(lock_addr);
    if (lock && lock->wr_held()) {
      // We know this lock and it is locked. Just unlock it.
      thread->HandleUnlock(lock_addr);
    } else {
      // Never seen this lock or it is currently unlocked. Init it.
      Lock::Create(lock_addr);
    }
  }

  void HandleLockCreateOrDestroy(Event *e) {
    TSanThread *thread = TSanThread::Get(TID(e->tid()));
    uintptr_t lock_addr = e->a();
    if (debug_lock) {
      e->Print();
    }
    if (e->type() == LOCK_CREATE) {
      Lock::Create(lock_addr);
    } else {
      CHECK(e->type() == LOCK_DESTROY);
      // A locked pthread_mutex_t can not be destroyed but other lock types can.
      // When destroying a lock, we must unlock it.
      // If there is a bug in a program when someone attempts to unlock
      // a destoyed lock, we are likely to fail in an assert.
      //
      // We do not unlock-on-destroy after main() has exited.
      // This is because global Mutex objects may be desctructed while threads
      // holding them are still running. Urgh...
      Lock *lock = Lock::Lookup(lock_addr);
      // If the lock is not found, report an error.
      if (lock == NULL) {
        ThreadSanitizerInvalidLockReport *report =
            new ThreadSanitizerInvalidLockReport;
        report->type = ThreadSanitizerReport::INVALID_LOCK;
        report->tid = TID(e->tid());
        report->lock_addr = lock_addr;
        report->stack_trace = thread->CreateStackTrace();
        ThreadSanitizerPrintReport(report);
        return;
      }
      if (lock->wr_held() || lock->rd_held()) {
        if (G_flags->unlock_on_mutex_destroy && !g_has_exited_main) {
          thread->HandleUnlock(lock_addr);
        }
      }
      thread->HandleForgetSignaller(lock_addr);
      Lock::Destroy(lock_addr);
    }
  }

  void HandleTraceMem(Event *e) {
    if (G_flags->trace_level == 0) return;
    TID tid(e->tid());
    TSanThread *thr = TSanThread::Get(TID(e->tid()));
    uintptr_t a = e->a();
    CacheLine *line = G_cache->GetLineOrCreateNew(thr, a, __LINE__);
    uintptr_t offset = CacheLine::ComputeOffset(a);
    line->traced().Set(offset);
    G_cache->ReleaseLine(thr, a, line, __LINE__);
    if (G_flags->verbosity >= 2) e->Print();
  }

  INLINE void RefAndUnrefTwoSegSetPairsIfDifferent(SSID new_ssid1,
                                                   SSID old_ssid1,
                                                   SSID new_ssid2,
                                                   SSID old_ssid2) {
    bool recycle_1 = new_ssid1 != old_ssid1,
         recycle_2 = new_ssid2 != old_ssid2;
    if (recycle_1 && !new_ssid1.IsEmpty()) {
      SegmentSet::Ref(new_ssid1, "RefAndUnrefTwoSegSetPairsIfDifferent");
    }

    if (recycle_2 && !new_ssid2.IsEmpty()) {
      SegmentSet::Ref(new_ssid2, "RefAndUnrefTwoSegSetPairsIfDifferent");
    }

    if (recycle_1 && !old_ssid1.IsEmpty()) {
      SegmentSet::Unref(old_ssid1, "RefAndUnrefTwoSegSetPairsIfDifferent");
    }

    if (recycle_2 && !old_ssid2.IsEmpty()) {
      SegmentSet::Unref(old_ssid2, "RefAndUnrefTwoSegSetPairsIfDifferent");
    }
  }


  // return true if the current pair of read/write segment sets
  // describes a race.
  bool NOINLINE CheckIfRace(SSID rd_ssid, SSID wr_ssid) {
    int wr_ss_size = SegmentSet::Size(wr_ssid);
    int rd_ss_size = SegmentSet::Size(rd_ssid);

    DCHECK(wr_ss_size >= 2 || (wr_ss_size >= 1 && rd_ss_size >= 1));

    // check all write-write pairs
    for (int w1 = 0; w1 < wr_ss_size; w1++) {
      SID w1_sid = SegmentSet::GetSID(wr_ssid, w1, __LINE__);
      Segment *w1_seg = Segment::Get(w1_sid);
      LSID w1_ls = w1_seg->lsid(true);
      for (int w2 = w1 + 1; w2 < wr_ss_size; w2++) {
        DCHECK(wr_ssid.IsTuple());
        SegmentSet *ss = SegmentSet::Get(wr_ssid);
        LSID w2_ls = Segment::Get(ss->GetSID(w2))->lsid(true);
        if (LockSet::IntersectionIsEmpty(w1_ls, w2_ls)) {
          return true;
        } else {
          // May happen only if the locks in the intersection are hybrid locks.
          DCHECK(LockSet::HasNonPhbLocks(w1_ls) &&
                 LockSet::HasNonPhbLocks(w2_ls));
        }
      }
      // check all write-read pairs
      for (int r = 0; r < rd_ss_size; r++) {
        SID r_sid = SegmentSet::GetSID(rd_ssid, r, __LINE__);
        Segment *r_seg = Segment::Get(r_sid);
        LSID r_ls = r_seg->lsid(false);
        if (Segment::HappensBeforeOrSameThread(w1_sid, r_sid))
          continue;
        if (LockSet::IntersectionIsEmpty(w1_ls, r_ls)) {
          return true;
        } else {
          // May happen only if the locks in the intersection are hybrid locks.
          DCHECK(LockSet::HasNonPhbLocks(w1_ls) &&
                 LockSet::HasNonPhbLocks(r_ls));
        }
      }
    }
    return false;
  }

  // New experimental state machine.
  // Set *res to the new state.
  // Return true if the new state is race.
  bool INLINE MemoryStateMachine(ShadowValue old_sval, TSanThread *thr,
                                 bool is_w, ShadowValue *res) {
    ShadowValue new_sval;
    SID cur_sid = thr->sid();
    DCHECK(cur_sid.valid());

    if (UNLIKELY(old_sval.IsNew())) {
      // We see this memory for the first time.
      DCHECK(cur_sid.valid());
      if (is_w) {
        new_sval.set(SSID(0), SSID(cur_sid));
      } else {
        new_sval.set(SSID(cur_sid), SSID(0));
      }
      *res = new_sval;
      return false;
    }

    SSID old_rd_ssid = old_sval.rd_ssid();
    SSID old_wr_ssid = old_sval.wr_ssid();
    SSID new_rd_ssid(0);
    SSID new_wr_ssid(0);
    if (is_w) {
      new_rd_ssid = SegmentSet::RemoveSegmentFromSS(old_rd_ssid, cur_sid);
      new_wr_ssid = SegmentSet::AddSegmentToSS(old_wr_ssid, cur_sid);
    } else {
      if (SegmentSet::Contains(old_wr_ssid, cur_sid)) {
        // cur_sid is already in old_wr_ssid, no change to SSrd is required.
        new_rd_ssid = old_rd_ssid;
      } else {
        new_rd_ssid = SegmentSet::AddSegmentToSS(old_rd_ssid, cur_sid);
      }
      new_wr_ssid = old_wr_ssid;
    }

    if (UNLIKELY(G_flags->sample_events > 0)) {
      if (new_rd_ssid.IsTuple() || new_wr_ssid.IsTuple()) {
        static EventSampler sampler;
        sampler.Sample(thr, "HasTupleSS", false);
      }
    }


    new_sval.set(new_rd_ssid, new_wr_ssid);
    *res = new_sval;
    if (new_sval == old_sval)
      return false;

    if (new_wr_ssid.IsTuple() ||
        (!new_wr_ssid.IsEmpty() && !new_rd_ssid.IsEmpty())) {
      return CheckIfRace(new_rd_ssid, new_wr_ssid);
    }
    return false;
  }


  // Fast path implementation for the case when we stay in the same thread.
  // In this case we don't need to call HappensBefore(), deal with
  // Tuple segment sets and check for race.
  // If this function returns true, the ShadowValue *new_sval is updated
  // in the same way as MemoryStateMachine() would have done it. Just faster.
  INLINE bool MemoryStateMachineSameThread(bool is_w, ShadowValue old_sval,
                                           TSanThread *thr,
                                           ShadowValue *new_sval) {
#define MSM_STAT(i) do { if (DEBUG_MODE) \
  thr->stats.msm_branch_count[i]++; } while ((void)0, 0)
    SSID rd_ssid = old_sval.rd_ssid();
    SSID wr_ssid = old_sval.wr_ssid();
    SID cur_sid = thr->sid();
    TID tid = thr->tid();
    if (rd_ssid.IsEmpty()) {
      if (wr_ssid.IsSingleton()) {
        // *** CASE 01 ***: rd_ssid == 0, wr_ssid == singleton
        SID wr_sid = wr_ssid.GetSingleton();
        if (wr_sid == cur_sid) {  // --- w/r: {0, cur} => {0, cur}
          MSM_STAT(1);
          // no op
          return true;
        }
        if (tid == Segment::Get(wr_sid)->tid()) {
          // same thread, but the segments are different.
          DCHECK(cur_sid != wr_sid);
          if (is_w) {    // -------------- w: {0, wr} => {0, cur}
            MSM_STAT(2);
            new_sval->set(SSID(0), SSID(cur_sid));
            thr->AddDeadSid(wr_sid, "FastPath01");
          } else {       // -------------- r: {0, wr} => {cur, wr}
            MSM_STAT(3);
            new_sval->set(SSID(cur_sid), wr_ssid);
          }
          Segment::Ref(cur_sid, "FastPath01");
          return true;
        }
      } else if (wr_ssid.IsEmpty()) {
        // *** CASE 00 ***: rd_ssid == 0, wr_ssid == 0
        if (is_w) {      // -------------- w: {0, 0} => {0, cur}
          MSM_STAT(4);
          new_sval->set(SSID(0), SSID(cur_sid));
        } else {         // -------------- r: {0, 0} => {cur, 0}
          MSM_STAT(5);
          new_sval->set(SSID(cur_sid), SSID(0));
        }
        Segment::Ref(cur_sid, "FastPath00");
        return true;
      }
    } else if (rd_ssid.IsSingleton()) {
      SID rd_sid = rd_ssid.GetSingleton();
      if (wr_ssid.IsEmpty()) {
        // *** CASE 10 ***: rd_ssid == singleton, wr_ssid == 0
        if (rd_sid == cur_sid) {
          // same segment.
          if (is_w) {    // -------------- w: {cur, 0} => {0, cur}
            MSM_STAT(6);
            new_sval->set(SSID(0), SSID(cur_sid));
          } else {       // -------------- r: {cur, 0} => {cur, 0}
            MSM_STAT(7);
            // no op
          }
          return true;
        }
        if (tid == Segment::Get(rd_sid)->tid()) {
          // same thread, but the segments are different.
          DCHECK(cur_sid != rd_sid);
          if (is_w) {  // -------------- w: {rd, 0} => {0, cur}
            MSM_STAT(8);
            new_sval->set(SSID(0), SSID(cur_sid));
          } else {     // -------------- r: {rd, 0} => {cur, 0}
            MSM_STAT(9);
            new_sval->set(SSID(cur_sid), SSID(0));
          }
          Segment::Ref(cur_sid, "FastPath10");
          thr->AddDeadSid(rd_sid, "FastPath10");
          return true;
        }
      } else if (wr_ssid.IsSingleton()){
        // *** CASE 11 ***: rd_ssid == singleton, wr_ssid == singleton
        DCHECK(rd_ssid.IsSingleton());
        SID wr_sid = wr_ssid.GetSingleton();
        DCHECK(wr_sid != rd_sid);  // By definition of ShadowValue.
        if (cur_sid == rd_sid) {
          if (tid == Segment::Get(wr_sid)->tid()) {
            if (is_w) {  // -------------- w: {cur, wr} => {0, cur}
              MSM_STAT(10);
              new_sval->set(SSID(0), SSID(cur_sid));
              thr->AddDeadSid(wr_sid, "FastPath11");
            } else {     // -------------- r: {cur, wr} => {cur, wr}
              MSM_STAT(11);
              // no op
            }
            return true;
          }
        } else if (cur_sid == wr_sid){
          if (tid == Segment::Get(rd_sid)->tid()) {
            if (is_w) {  // -------------- w: {rd, cur} => {rd, cur}
              MSM_STAT(12);
              // no op
            } else {     // -------------- r: {rd, cur} => {0, cur}
              MSM_STAT(13);
              new_sval->set(SSID(0), SSID(cur_sid));
              thr->AddDeadSid(rd_sid, "FastPath11");
            }
            return true;
          }
        } else if (tid == Segment::Get(rd_sid)->tid() &&
                   tid == Segment::Get(wr_sid)->tid()) {
          if (is_w) {    // -------------- w: {rd, wr} => {0, cur}
            MSM_STAT(14);
            new_sval->set(SSID(0), SSID(cur_sid));
            thr->AddDeadSid(wr_sid, "FastPath11");
          } else {       // -------------- r: {rd, wr} => {cur, wr}
            MSM_STAT(15);
            new_sval->set(SSID(cur_sid), wr_ssid);
          }
          thr->AddDeadSid(rd_sid, "FastPath11");
          Segment::Ref(cur_sid, "FastPath11");
          return true;
        }
      }
    }
    MSM_STAT(0);
    return false;
#undef MSM_STAT
  }

  // return false if we were not able to complete the task (fast_path_only).
  INLINE bool HandleMemoryAccessHelper(bool is_w,
                                       CacheLine *cache_line,
                                       uintptr_t addr,
                                       uintptr_t size,
                                       uintptr_t pc,
                                       TSanThread *thr,
                                       bool fast_path_only) {
    DCHECK((addr & (size - 1)) == 0);  // size-aligned.
    uintptr_t offset = CacheLine::ComputeOffset(addr);

    ShadowValue old_sval;
    ShadowValue *sval_p = NULL;

    if (UNLIKELY(!cache_line->has_shadow_value().Get(offset))) {
      sval_p = cache_line->AddNewSvalAtOffset(offset);
      DCHECK(sval_p->IsNew());
    } else {
      sval_p = cache_line->GetValuePointer(offset);
    }
    old_sval = *sval_p;

    bool res = false;
    bool fast_path_ok = MemoryStateMachineSameThread(
        is_w, old_sval, thr, sval_p);
    if (fast_path_ok) {
      res = true;
    } else if (fast_path_only) {
      res = false;
    } else {
      bool is_published = cache_line->published().Get(offset);
      // We check only the first bit for publishing, oh well.
      if (UNLIKELY(is_published)) {
        const VTS *signaller_vts = GetPublisherVTS(addr);
        CHECK(signaller_vts);
        thr->NewSegmentForWait(signaller_vts);
      }

      bool is_race = MemoryStateMachine(old_sval, thr, is_w, sval_p);

      // Check for race.
      if (UNLIKELY(is_race)) {
        if (thr->ShouldReportRaces()) {
          if (G_flags->report_races && !cache_line->racey().Get(offset)) {
            reports_.AddReport(thr, pc, is_w, addr, size,
                               old_sval, *sval_p, is_published);
          }
          cache_line->racey().SetRange(offset, offset + size);
        }
      }

      // Ref/Unref segments
      RefAndUnrefTwoSegSetPairsIfDifferent(sval_p->rd_ssid(),
                                           old_sval.rd_ssid(),
                                           sval_p->wr_ssid(),
                                           old_sval.wr_ssid());
      res = true;
    }


    if (DEBUG_MODE && !fast_path_only) {
      // check that the SSIDs/SIDs in the new sval have sane ref counters.
      CHECK(!sval_p->wr_ssid().IsEmpty() || !sval_p->rd_ssid().IsEmpty());
      for (int i = 0; i < 2; i++) {
        SSID ssid = i ? sval_p->rd_ssid() : sval_p->wr_ssid();
        if (ssid.IsEmpty()) continue;
        if (ssid.IsSingleton()) {
          // singleton segment should have ref count > 0.
          SID sid = ssid.GetSingleton();
          Segment *seg = Segment::Get(sid);
          (void)seg;
          CHECK(seg->ref_count() > 0);
          if (sid == thr->sid()) {
            // if this is the current seg, ref count should be > 1.
            CHECK(seg->ref_count() > 1);
          }
        } else {
          SegmentSet *sset = SegmentSet::Get(ssid);
          (void)sset;
          CHECK(sset->ref_count() > 0);
        }
      }
    }
    return res;
  }


  // return false if we were not able to complete the task (fast_path_only).
  INLINE bool HandleAccessGranularityAndExecuteHelper(
      CacheLine *cache_line,
      TSanThread *thr, uintptr_t addr, MopInfo *mop,
      bool has_expensive_flags, bool fast_path_only) {
    size_t size = mop->size();
    uintptr_t pc = mop->pc();
    bool is_w = mop->is_write();
    uintptr_t a = addr;
    uintptr_t b = 0;
    uintptr_t off = CacheLine::ComputeOffset(a);

    uint16_t *granularity_mask = cache_line->granularity_mask(off);
    uint16_t gr = *granularity_mask;

    // Can't do split/join on the fast path, bacause it involves segment set
    // reference count manipulation that is not thread-safe.

    if        (size == 8 && (off & 7) == 0) {
      if (!gr) {
        *granularity_mask = gr = 1;  // 0000000000000001
      }
      if (GranularityIs8(off, gr)) {
        if (has_expensive_flags) thr->stats.n_fast_access8++;
        cache_line->DebugTrace(off, __FUNCTION__, __LINE__);
        goto one_call;
      } else {
        if (fast_path_only) return false;
        if (has_expensive_flags) thr->stats.n_slow_access8++;
        cache_line->Join_1_to_2(off);
        cache_line->Join_1_to_2(off + 2);
        cache_line->Join_1_to_2(off + 4);
        cache_line->Join_1_to_2(off + 6);
        cache_line->Join_2_to_4(off);
        cache_line->Join_2_to_4(off + 4);
        cache_line->Join_4_to_8(off);
        goto slow_path;
      }
    } else if (size == 4 && (off & 3) == 0) {
      if (!gr) {
        *granularity_mask = gr = 3 << 1;  // 0000000000000110
      }
      if (GranularityIs4(off, gr)) {
        if (has_expensive_flags) thr->stats.n_fast_access4++;
        cache_line->DebugTrace(off, __FUNCTION__, __LINE__);
        goto one_call;
      } else {
        if (fast_path_only) return false;
        if (has_expensive_flags) thr->stats.n_slow_access4++;
        cache_line->Split_8_to_4(off);
        cache_line->Join_1_to_2(off);
        cache_line->Join_1_to_2(off + 2);
        cache_line->Join_2_to_4(off);
        goto slow_path;
      }
    } else if (size == 2 && (off & 1) == 0) {
      if (!gr) {
        *granularity_mask = gr = 15 << 3;  // 0000000001111000
      }
      if (GranularityIs2(off, gr)) {
        if (has_expensive_flags) thr->stats.n_fast_access2++;
        cache_line->DebugTrace(off, __FUNCTION__, __LINE__);
        goto one_call;
      } else {
        if (fast_path_only) return false;
        if (has_expensive_flags) thr->stats.n_slow_access2++;
        cache_line->Split_8_to_4(off);
        cache_line->Split_4_to_2(off);
        cache_line->Join_1_to_2(off);
        goto slow_path;
      }
    } else if (size == 1) {
      if (!gr) {
        *granularity_mask = gr = 255 << 7;  // 0111111110000000
      }
      if (GranularityIs1(off, gr)) {
        if (has_expensive_flags) thr->stats.n_fast_access1++;
        cache_line->DebugTrace(off, __FUNCTION__, __LINE__);
        goto one_call;
      } else {
        if (fast_path_only) return false;
        if (has_expensive_flags) thr->stats.n_slow_access1++;
        cache_line->Split_8_to_4(off);
        cache_line->Split_4_to_2(off);
        cache_line->Split_2_to_1(off);
        goto slow_path;
      }
    } else {
      if (fast_path_only) return false;
      if (has_expensive_flags) thr->stats.n_very_slow_access++;
      // Very slow: size is not 1,2,4,8 or address is unaligned.
      // Handle this access as a series of 1-byte accesses, but only
      // inside the current cache line.
      // TODO(kcc): do we want to handle the next cache line as well?
      b = a + mop->size();
      uintptr_t max_x = min(b, CacheLine::ComputeNextTag(a));
      for (uintptr_t x = a; x < max_x; x++) {
        off = CacheLine::ComputeOffset(x);
        DCHECK(CacheLine::ComputeTag(x) == cache_line->tag());
        uint16_t *granularity_mask = cache_line->granularity_mask(off);
        if (!*granularity_mask) {
          *granularity_mask = 1;
        }
        cache_line->DebugTrace(off, __FUNCTION__, __LINE__);
        cache_line->Split_8_to_4(off);
        cache_line->Split_4_to_2(off);
        cache_line->Split_2_to_1(off);
        if (!HandleMemoryAccessHelper(is_w, cache_line, x, 1, pc, thr, false))
          return false;
      }
      return true;
    }

slow_path:
    if (fast_path_only) return false;
    DCHECK(cache_line);
    DCHECK(size == 1 || size == 2 || size == 4 || size == 8);
    DCHECK((addr & (size - 1)) == 0);  // size-aligned.
    gr = *granularity_mask;
    CHECK(gr);
    // size is one of 1, 2, 4, 8; address is size-aligned, but the granularity
    // is different.
    b = a + mop->size();
    for (uintptr_t x = a; x < b;) {
      if (has_expensive_flags) thr->stats.n_access_slow_iter++;
      off = CacheLine::ComputeOffset(x);
      cache_line->DebugTrace(off, __FUNCTION__, __LINE__);
      size_t s = 0;
      // How many bytes are we going to access?
      if     (GranularityIs8(off, gr)) s = 8;
      else if(GranularityIs4(off, gr)) s = 4;
      else if(GranularityIs2(off, gr)) s = 2;
      else                             s = 1;
      if (!HandleMemoryAccessHelper(is_w, cache_line, x, s, pc, thr, false))
        return false;
      x += s;
    }
    return true;
one_call:
    return HandleMemoryAccessHelper(is_w, cache_line, addr, size, pc,
                                    thr, fast_path_only);
  }

  INLINE bool IsTraced(CacheLine *cache_line, uintptr_t addr,
                       bool has_expensive_flags) {
    if (!has_expensive_flags) return false;
    if (G_flags->trace_level == 0) return false;
    DCHECK(cache_line);
    uintptr_t off = CacheLine::ComputeOffset(addr);
    if (cache_line->traced().Get(off)) {
      return true;
    } else if (addr == G_flags->trace_addr) {
      return true;
    }
    return false;
  }

  void DoTrace(TSanThread *thr, uintptr_t addr, MopInfo *mop, bool need_locking) {
    size_t size = mop->size();
    uintptr_t pc = mop->pc();
    TIL til(ts_lock, 1, need_locking);
    for (uintptr_t x = addr; x < addr + size; x++) {
      uintptr_t off = CacheLine::ComputeOffset(x);
      CacheLine *cache_line = G_cache->GetLineOrCreateNew(thr,
                                                          x, __LINE__);
      ShadowValue *sval_p = cache_line->GetValuePointer(off);
      if (cache_line->has_shadow_value().Get(off) != 0) {
        bool is_published = cache_line->published().Get(off);
        Printf("TRACE: T%d/S%d %s[%d] addr=%p sval: %s%s; line=%p P=%s\n",
               raw_tid(thr), thr->sid().raw(), mop->is_write() ? "wr" : "rd",
               size, addr, sval_p->ToString().c_str(),
               is_published ? " P" : "",
               cache_line,
               cache_line->published().Empty() ?
               "0" : cache_line->published().ToString().c_str());
        thr->ReportStackTrace(pc);
      }
      G_cache->ReleaseLine(thr, x, cache_line, __LINE__);
    }
  }


#if TS_SERIALIZED == 1
  INLINE  // TODO(kcc): this can also be made NOINLINE later.
#else
  NOINLINE
#endif
  void HandleMemoryAccessSlowLocked(TSanThread *thr,
                                    uintptr_t addr,
                                    MopInfo *mop,
                                    bool has_expensive_flags,
                                    bool need_locking) {
    AssertTILHeld();
    DCHECK(thr->lsid(false) == thr->segment()->lsid(false));
    DCHECK(thr->lsid(true) == thr->segment()->lsid(true));
    thr->FlushDeadSids();
    if (TS_SERIALIZED == 0) {
      // In serialized version this is the hotspot, so grab fresh SIDs
      // only in non-serial variant.
      thr->GetSomeFreshSids();
    }
    CacheLine *cache_line = G_cache->GetLineOrCreateNew(thr, addr, __LINE__);
    HandleAccessGranularityAndExecuteHelper(cache_line, thr, addr,
                                            mop, has_expensive_flags,
                                            /*fast_path_only=*/false);
    bool tracing = IsTraced(cache_line, addr, has_expensive_flags);
    G_cache->ReleaseLine(thr, addr, cache_line, __LINE__);
    cache_line = NULL;  // just in case.

    if (has_expensive_flags) {
      if (tracing) {
        DoTrace(thr, addr, mop, /*need_locking=*/false);
      }
      if (G_flags->sample_events > 0) {
        const char *type = "SampleMemoryAccess";
        static EventSampler sampler;
        sampler.Sample(thr, type, false);
      }
    }
  }

  INLINE bool HandleMemoryAccessInternal(TSanThread *thr,
                                         uintptr_t *sblock_pc,
                                         uintptr_t addr,
                                         MopInfo *mop,
                                         bool has_expensive_flags,
                                         bool need_locking) {
#   define INC_STAT(stat) \
        do { if (has_expensive_flags) (stat)++; } while ((void)0, 0)
    if (TS_ATOMICITY && G_flags->atomicity) {
      HandleMemoryAccessForAtomicityViolationDetector(thr, addr, mop);
      return false;
    }
    DCHECK(mop->size() > 0);
    DCHECK(thr->is_running());
    DCHECK(!thr->ignore_reads() || !thr->ignore_writes());

    // We do not check and ignore stack now.
    // On unoptimized binaries this would give ~10% speedup if ignore_stack==true,
    // but if --ignore_stack==false this would cost few extra insns.
    // On optimized binaries ignoring stack gives nearly nothing.
    // if (thr->IgnoreMemoryIfInStack(addr)) return;

    CacheLine *cache_line = NULL;
    INC_STAT(thr->stats.memory_access_sizes[mop->size() <= 16 ? mop->size() : 17 ]);
    INC_STAT(thr->stats.events[mop->is_write() ? WRITE : READ]);
    if (has_expensive_flags) {
      thr->stats.access_to_first_1g += (addr >> 30) == 0;
      thr->stats.access_to_first_2g += (addr >> 31) == 0;
      thr->stats.access_to_first_4g += ((uint64_t)addr >> 32) == 0;
    }

    int locked_access_case = 0;

    if (need_locking) {
      // The fast (unlocked) path.
      if (thr->HasRoomForDeadSids()) {
        // Acquire a line w/o locks.
        cache_line = G_cache->TryAcquireLine(thr, addr, __LINE__);
        if (!Cache::LineIsNullOrLocked(cache_line)) {
          // The line is not empty or locked -- check the tag.
          if (cache_line->tag() == CacheLine::ComputeTag(addr)) {
            // The line is ours and non-empty -- fire the fast path.
            if (thr->HandleSblockEnter(*sblock_pc, /*allow_slow_path=*/false)) {
              *sblock_pc = 0;  // don't do SblockEnter any more.
              bool res = HandleAccessGranularityAndExecuteHelper(
                  cache_line, thr, addr,
                  mop, has_expensive_flags,
                  /*fast_path_only=*/true);
              bool traced = IsTraced(cache_line, addr, has_expensive_flags);
              // release the line.
              G_cache->ReleaseLine(thr, addr, cache_line, __LINE__);
              if (res && has_expensive_flags && traced) {
                DoTrace(thr, addr, mop, /*need_locking=*/true);
              }
              if (res) {
                INC_STAT(thr->stats.unlocked_access_ok);
                // fast path succeded, we are done.
                return false;
              } else {
                locked_access_case = 1;
              }
            } else {
              // we were not able to handle SblockEnter.
              G_cache->ReleaseLine(thr, addr, cache_line, __LINE__);
              locked_access_case = 2;
            }
          } else {
            locked_access_case = 3;
            // The line has a wrong tag.
            G_cache->ReleaseLine(thr, addr, cache_line, __LINE__);
          }
        } else if (cache_line == NULL) {
          locked_access_case = 4;
          // We grabbed the cache slot but it is empty, release it.
          G_cache->ReleaseLine(thr, addr, cache_line, __LINE__);
        } else {
          locked_access_case = 5;
        }
      } else {
        locked_access_case = 6;
      }
    } else {
      locked_access_case = 7;
    }

    if (need_locking) {
      INC_STAT(thr->stats.locked_access[locked_access_case]);
    }

    // Everything below goes under a lock.
    TIL til(ts_lock, 2, need_locking);
    thr->HandleSblockEnter(*sblock_pc, /*allow_slow_path=*/true);
    *sblock_pc = 0;  // don't do SblockEnter any more.
    HandleMemoryAccessSlowLocked(thr, addr, mop,
                                 has_expensive_flags,
                                 need_locking);
    return true;
#undef INC_STAT
  }


  void HandleMemoryAccessForAtomicityViolationDetector(TSanThread *thr,
                                                       uintptr_t addr,
                                                       MopInfo *mop) {
    CHECK(G_flags->atomicity);
    TID tid = thr->tid();
    if (thr->MemoryIsInStack(addr)) return;

    LSID wr_lsid = thr->lsid(0);
    LSID rd_lsid = thr->lsid(1);
    if (wr_lsid.raw() == 0 && rd_lsid.raw() == 0) {
      thr->increment_n_mops_since_start();
      return;
    }
    // uint64_t combined_lsid = wr_lsid.raw();
    // combined_lsid = (combined_lsid << 32) | rd_lsid.raw();
    // if (combined_lsid == 0) return;

//    Printf("Era=%d T%d %s a=%p pc=%p in_stack=%d %s\n", g_lock_era,
//           tid.raw(), is_w ? "W" : "R", addr, pc, thr->MemoryIsInStack(addr),
//           PcToRtnNameAndFilePos(pc).c_str());

    BitSet *range_set = thr->lock_era_access_set(mop->is_write());
    // Printf("era %d T%d access under lock pc=%p addr=%p size=%p w=%d\n",
    //        g_lock_era, tid.raw(), pc, addr, size, is_w);
    range_set->Add(addr, addr + mop->size());
    // Printf("   %s\n", range_set->ToString().c_str());
  }


  // MALLOC
  void HandleMalloc(Event *e, bool is_mmap) {
    ScopedMallocCostCenter cc("HandleMalloc");
    TID tid(e->tid());
    uintptr_t a = e->a();
    uintptr_t size = e->info();


    if (a == 0)
      return;

    #if defined(__GNUC__) && __WORDSIZE == 64
    // If we are allocating a huge piece of memory,
    // don't handle it because it is too slow.
    // TODO(kcc): this is a workaround for NaCl. May need to fix it cleaner.
    const uint64_t G84 = (1ULL << 32) * 21; // 84G.
    if (size >= G84) {
      return;
    }
    #endif
    TSanThread *thr = TSanThread::Get(tid);
    thr->NewSegmentForMallocEvent();
    uintptr_t b = a + size;
    CHECK(a <= b);
    ClearMemoryState(thr, a, b);
    // update heap_map
    HeapInfo info;
    info.ptr  = a;
    info.size = size;
    info.sid  = thr->sid();
    Segment::Ref(info.sid, __FUNCTION__);
    if (debug_malloc) {
      Printf("T%d MALLOC: %p [%p %p) %s %s\n%s\n",
             tid.raw(), size, a, a+size,
             Segment::ToString(thr->sid()).c_str(),
             thr->segment()->vts()->ToString().c_str(),
             info.StackTraceString().c_str());
    }

    // CHECK(!G_heap_map->count(a));  // we may have two calls
                                      //  to AnnotateNewMemory.
    G_heap_map->InsertInfo(a, info);

    if (is_mmap) {
      // Mmap may be used for thread stack, so we should keep the mmap info
      // when state is flushing.
      ThreadStackInfo ts_info;
      ts_info.ptr = a;
      ts_info.size = size;
      G_thread_stack_map->InsertInfo(a, ts_info);
    }
  }

  void ImitateWriteOnFree(TSanThread *thr, uintptr_t a, uintptr_t size, uintptr_t pc) {
    // Handle the memory deletion as a write, but don't touch all
    // the memory if there is too much of it, limit with the first 1K.
    if (size && G_flags->free_is_write && !global_ignore) {
      const uintptr_t kMaxWriteSizeOnFree = 2048;
      uintptr_t write_size = min(kMaxWriteSizeOnFree, size);
      uintptr_t step = sizeof(uintptr_t);
      // We simulate 4- or 8-byte accesses to make analysis faster.
      for (uintptr_t i = 0; i < write_size; i += step) {
        uintptr_t this_size = write_size - i >= step ? step : write_size - i;
        HandleMemoryAccess(thr, pc, a + i, this_size,
                           /*is_w=*/true, /*need_locking*/false);
      }
    }
  }

  // FREE
  void HandleFree(Event *e) {
    TID tid(e->tid());
    TSanThread *thr = TSanThread::Get(tid);
    uintptr_t a = e->a();
    if (debug_free) {
      e->Print();
      thr->ReportStackTrace(e->pc());
    }
    if (a == 0)
      return;
    HeapInfo *info = G_heap_map->GetInfo(a);
    if (!info || info->ptr != a)
      return;
    uintptr_t size = info->size;
    uintptr_t pc = e->pc();
    ImitateWriteOnFree(thr, a, size, pc);
    // update G_heap_map
    CHECK(info->ptr == a);
    Segment::Unref(info->sid, __FUNCTION__);

    ClearMemoryState(thr, a, a + size);
    G_heap_map->EraseInfo(a);

    // We imitate a Write event again, in case there will be use-after-free.
    // We also need to create a new sblock so that the previous stack trace
    // has free() in it.
    if (G_flags->keep_history && G_flags->free_is_write) {
      thr->HandleSblockEnter(pc, /*allow_slow_path*/true);
    }
    ImitateWriteOnFree(thr, a, size, pc);
  }

  void HandleMunmap(Event *e) {
    // TODO(glider): at the moment we handle only munmap()s of single mmap()ed
    // regions. The correct implementation should handle arbitrary munmap()s
    // that may carve the existing mappings or split them into two parts.
    // It should also be possible to munmap() several mappings at a time.
    uintptr_t a = e->a();
    if (a == 0)
      return;
    HeapInfo *h_info = G_heap_map->GetInfo(a);
    uintptr_t size = e->info();
    if (h_info && h_info->ptr == a && h_info->size == size) {
      // TODO(glider): we may want to handle memory deletion and call
      // Segment::Unref for all the unmapped memory.
      Segment::Unref(h_info->sid, __FUNCTION__);
      G_heap_map->EraseRange(a, a + size);
    }

    ThreadStackInfo *ts_info = G_thread_stack_map->GetInfo(a);
    if (ts_info && ts_info->ptr == a && ts_info->size == size)
      G_thread_stack_map->EraseRange(a, a + size);
  }

  void HandleThreadStart(TID child_tid, TID parent_tid, CallStack *call_stack) {
    // Printf("HandleThreadStart: tid=%d parent_tid=%d pc=%lx pid=%d\n",
    //         child_tid.raw(), parent_tid.raw(), pc, getpid());
    VTS *vts = NULL;
    StackTrace *creation_context = NULL;
    if (child_tid == TID(0)) {
      // main thread, we are done.
      vts = VTS::CreateSingleton(child_tid);
    } else if (!parent_tid.valid()) {
      TSanThread::StopIgnoringAccessesInT0BecauseNewThreadStarted();
      Report("INFO: creating thread T%d w/o a parent\n", child_tid.raw());
      vts = VTS::CreateSingleton(child_tid);
    } else {
      TSanThread::StopIgnoringAccessesInT0BecauseNewThreadStarted();
      TSanThread *parent = TSanThread::Get(parent_tid);
      CHECK(parent);
      parent->HandleChildThreadStart(child_tid, &vts, &creation_context);
    }

    if (!call_stack) {
      call_stack = new CallStack();
    }
    TSanThread *new_thread = new TSanThread(child_tid, parent_tid,
                                    vts, creation_context, call_stack);
    CHECK(new_thread == TSanThread::Get(child_tid));
    if (child_tid == TID(0)) {
      new_thread->set_ignore_all_accesses(true); // until a new thread comes.
    }
  }

  // Executes before the first instruction of the thread but after the thread
  // has been set up (e.g. the stack is in place).
  void HandleThreadFirstInsn(TID tid) {
    // TODO(kcc): get rid of this once we find out how to get the T0's stack.
    if (tid == TID(0)) {
      uintptr_t stack_min(0), stack_max(0);
      GetThreadStack(tid.raw(), &stack_min, &stack_max);
      TSanThread *thr = TSanThread::Get(tid);
      thr->SetStack(stack_min, stack_max);
      ClearMemoryState(thr, thr->min_sp(), thr->max_sp());
    }
  }

  // THR_STACK_TOP
  void HandleThreadStackTop(Event *e) {
    TID tid(e->tid());
    TSanThread *thr = TSanThread::Get(tid);
    // Stack grows from bottom up.
    uintptr_t sp = e->a();
    uintptr_t sp_min = 0, sp_max = 0;
    uintptr_t stack_size_if_known = e->info();
    ThreadStackInfo *stack_info;
    if (stack_size_if_known) {
      sp_min = sp - stack_size_if_known;
      sp_max = sp;
    } else if (NULL != (stack_info = G_thread_stack_map->GetInfo(sp))) {
      if (debug_thread) {
        Printf("T%d %s: %p\n%s\n", e->tid(), __FUNCTION__,  sp,
             reports_.DescribeMemory(sp).c_str());
      }
      sp_min = stack_info->ptr;
      sp_max = stack_info->ptr + stack_info->size;
    }
    if (debug_thread) {
      Printf("T%d SP: %p [%p %p), size=%ldK\n",
             e->tid(), sp, sp_min, sp_max, (sp_max - sp_min) >> 10);
    }
    if (sp_min < sp_max) {
      CHECK((sp_max - sp_min) >= 8 * 1024); // stay sane.
      CHECK((sp_max - sp_min) < 128 * 1024 * 1024); // stay sane.
      ClearMemoryState(thr, sp_min, sp_max);
      thr->SetStack(sp_min, sp_max);
    }
  }

  // THR_END
  void HandleThreadEnd(TID tid) {
    TSanThread *thr = TSanThread::Get(tid);
    // Add the thread-local stats to global stats.
    G_stats->Add(thr->stats);
    thr->stats.Clear();

    // Printf("HandleThreadEnd: %d\n", tid.raw());
    if (tid != TID(0)) {
      TSanThread *child = TSanThread::Get(tid);
      child->HandleThreadEnd();


      if (debug_thread) {
        Printf("T%d:  THR_END     : %s %s\n", tid.raw(),
               Segment::ToString(child->sid()).c_str(),
               child->vts()->ToString().c_str());
      }
      ClearMemoryState(thr, child->min_sp(), child->max_sp());
    } else {
      reports_.SetProgramFinished();
    }


    if (g_so_far_only_one_thread == false
        && (thr->ignore_reads() || thr->ignore_writes())) {
      Report("WARNING: T%d ended while at least one 'ignore' bit is set: "
             "ignore_wr=%d ignore_rd=%d\n", tid.raw(),
             thr->ignore_reads(), thr->ignore_writes());
      for (int i = 0; i < 2; i++) {
        StackTrace *context = thr->GetLastIgnoreContext(i);
        if (context) {
          Report("Last ignore_%s call was here: \n%s\n", i ? "wr" : "rd",
                 context->ToString().c_str());
        }
      }
      if (G_flags->save_ignore_context == false) {
        Report("Rerun with --save_ignore_context to see where "
               "IGNORE_END is missing\n");
      }
    }
    ShowProcSelfStatus();
  }

  // THR_JOIN_AFTER
  void HandleThreadJoinAfter(Event *e) {
    TID tid(e->tid());
    TSanThread *parent_thr = TSanThread::Get(tid);
    VTS *vts_at_exit = NULL;
    TID child_tid = parent_thr->HandleThreadJoinAfter(&vts_at_exit, TID(e->a()));
    CHECK(vts_at_exit);
    CHECK(parent_thr->sid().valid());
    Segment::AssertLive(parent_thr->sid(),  __LINE__);
    parent_thr->NewSegmentForWait(vts_at_exit);
    if (debug_thread) {
      Printf("T%d:  THR_JOIN_AFTER T%d  : %s\n", tid.raw(),
             child_tid.raw(), parent_thr->vts()->ToString().c_str());
    }
  }

 public:
  // TODO(kcc): merge this into Detector class. (?)
  ReportStorage reports_;

  void SetUnwindCallback(ThreadSanitizerUnwindCallback cb) {
    reports_.SetUnwindCallback(cb);
  }
};

static Detector        *G_detector;


void TSanThread::HandleAtomicMop(uintptr_t a,
                             uintptr_t pc,
                             tsan_atomic_op op,
                             tsan_memory_order mo,
                             size_t size) {
  if (op == tsan_atomic_op_fence)
    return;
  bool const is_store = (op != tsan_atomic_op_load);
  CHECK(inside_atomic_op_ >= 0);
  if (mo != tsan_memory_order_natomic)
    inside_atomic_op_ += 1;
  MopInfo mop (pc, size, is_store, true);
  G_detector->HandleTrace(this, &mop, 1, pc, &a, false);
  if (mo != tsan_memory_order_natomic)
    inside_atomic_op_ -= 1;
  CHECK(inside_atomic_op_ >= 0);
}


// -------- Flags ------------------------- {{{1
const char *usage_str =
"Usage:\n"
"  %s [options] program_to_test [program's options]\n"
"See %s for details\n";

void ThreadSanitizerPrintUsage() {
  Printf(usage_str, G_flags->tsan_program_name.c_str(),
         G_flags->tsan_url.c_str());
}

static void ReportUnknownFlagAndExit(const string &str) {
  Printf("Unknown flag or flag value: %s\n", str.c_str());
  ThreadSanitizerPrintUsage();
  exit(1);
}

// if arg and flag match, return true
// and set 'val' to the substring of arg after '='.
static bool FlagNameMatch(const string &arg, const string &flag, string *val) {
  string f = string("--") + flag;
  if (arg.size() < f.size()) return false;
  for (size_t i = 0; i < f.size(); i++) {
    // '-' must match '-'
    // '_' may match '_' or '-'
    if (f[i] == '_') {
      if (arg[i] != '-' && arg[i] != '_') return false;
    } else {
      if (f[i] != arg[i]) return false;
    }
  }
  if (arg.size() == f.size()) {
    *val = "";
    return true;
  }
  if (arg[f.size()] != '=') return false;
  *val = arg.substr(f.size() + 1);
  return true;
}

static int FindBoolFlag(const char *name, bool default_val,
                  vector<string> *args, bool *retval) {
  int res = 0;
  *retval = default_val;
  bool cont = false;
  do {
    cont = false;
    vector<string>::iterator it = args->begin();
    for (; it != args->end(); ++it) {
      string &str = *it;
      string flag_value;
      if (!FlagNameMatch(str, name, &flag_value)) continue;

      if (flag_value == "")            *retval = true;
      else if (flag_value == "1")     *retval = true;
      else if (flag_value == "true")  *retval = true;
      else if (flag_value == "yes")   *retval = true;
      else if (flag_value == "0")     *retval = false;
      else if (flag_value == "false") *retval = false;
      else if (flag_value == "no")    *retval = false;
      else
        ReportUnknownFlagAndExit(str);
      res++;
      if (G_flags->verbosity >= 1) {
        Printf("%40s => %s\n", name, *retval ? "true" : "false");
      }
      break;
    }
    if (it != args->end()) {
      cont = true;
      args->erase(it);
    }
  } while (cont);
  return res;
}

static void FindIntFlag(const char *name, intptr_t default_val,
                 vector<string> *args, intptr_t *retval) {
  *retval = default_val;
  bool cont = false;
  do {
    cont = false;
    vector<string>::iterator it = args->begin();
    for (; it != args->end(); ++it) {
      string &str = *it;
      string flag_value;
      if (!FlagNameMatch(str, name, &flag_value)) continue;
      char *end_ptr;
      const char *beg_ptr = flag_value.c_str();
      intptr_t int_val = my_strtol(beg_ptr, &end_ptr, 0);
      if (flag_value.empty() || beg_ptr + flag_value.size() != end_ptr)
        ReportUnknownFlagAndExit(str);
      *retval = int_val;
      if (G_flags->verbosity >= 1) {
        Printf("%40s => %ld\n", name, *retval);
      }
      break;
    }
    if (it != args->end()) {
      cont = true;
      args->erase(it);
    }
  } while (cont);
}

static void FindUIntFlag(const char *name, intptr_t default_val,
                 vector<string> *args, uintptr_t *retval) {
  intptr_t signed_int;
  FindIntFlag(name, default_val, args, &signed_int);
  CHECK_GE(signed_int, 0);
  *retval = signed_int;
}

void FindStringFlag(const char *name, vector<string> *args,
                    vector<string> *retval) {
  bool cont = false;
  do {
    cont = false;
    vector<string>::iterator it = args->begin();
    for (; it != args->end(); ++it) {
      string &str = *it;
      string flag_value;
      if (!FlagNameMatch(str, name, &flag_value)) continue;
      retval->push_back(flag_value);
      if (G_flags->verbosity >= 1) {
        Printf("%40s => %s\n", name, flag_value.c_str());
      }
      break;
    }
    if (it != args->end()) {
      cont = true;
      args->erase(it);
    }
  } while (cont);
}

void FindStringFlag(const char *name, vector<string> *args,
                    string *retval) {
  vector<string> tmp;
  FindStringFlag(name, args, &tmp);
  if (tmp.size() > 0) {
    *retval = tmp.back();
  }
}

static size_t GetMemoryLimitInMbFromProcSelfLimits() {
#ifdef VGO_linux
  // Parse the memory limit section of /proc/self/limits.
  string proc_self_limits = ReadFileToString("/proc/self/limits", false);
  const char *max_addr_space = "Max address space";
  size_t pos = proc_self_limits.find(max_addr_space);
  if (pos == string::npos) return 0;
  pos += strlen(max_addr_space);
  while (proc_self_limits[pos] == ' ') pos++;
  if (proc_self_limits[pos] == 'u')
    return 0;  // 'unlimited'.
  char *end;
  size_t result = my_strtol(proc_self_limits.c_str() + pos, &end, 0);
  result >>= 20;
  return result;
#else
  return 0;
#endif
}

static size_t GetMemoryLimitInMb() {
  size_t ret = -1;  // Maximum possible value.
#if defined(VGO_linux) && __WORDSIZE == 32
  // Valgrind doesn't support more than 3G per process on 32-bit Linux.
  ret = 3 * 1024;
#endif

  // Try /proc/self/limits.
  size_t from_proc_self = GetMemoryLimitInMbFromProcSelfLimits();
  if (from_proc_self && ret > from_proc_self) {
    ret = from_proc_self;
  }
  // Try env.
  const char *from_env_str =
    (const char*)getenv("VALGRIND_MEMORY_LIMIT_IN_MB");
  if (from_env_str) {
    char *end;
    size_t from_env_value = (size_t)my_strtol(from_env_str, &end, 0);
    if (ret > from_env_value)
      ret = from_env_value;
  }
  if (ret == (size_t)-1)
    return 0;
  return ret;
}

bool PhaseDebugIsOn(const char *phase_name) {
  CHECK(G_flags);
  for (size_t i = 0; i < G_flags->debug_phase.size(); i++) {
    if (G_flags->debug_phase[i] == phase_name)
      return true;
  }
  return false;
}

void ThreadSanitizerParseFlags(vector<string> *args) {
#ifdef TS_OFFLINE
  string input_type_tmp;
  FindStringFlag("input_type", args, &input_type_tmp);
  if (input_type_tmp.size() > 0) {
    G_flags->input_type = input_type_tmp;
  } else {
    G_flags->input_type = "str";
  }
#endif

  // Check this first.
  FindIntFlag("v", 0, args, &G_flags->verbosity);

  FindBoolFlag("ignore_stack", false, args, &G_flags->ignore_stack);
  FindIntFlag("keep_history", 1, args, &G_flags->keep_history);
  FindUIntFlag("segment_set_recycle_queue_size", DEBUG_MODE ? 10 : 10000, args,
               &G_flags->segment_set_recycle_queue_size);
  FindUIntFlag("recent_segments_cache_size", 10, args,
               &G_flags->recent_segments_cache_size);

  bool fast_mode = false;
  FindBoolFlag("fast_mode", false, args, &fast_mode);
  if (fast_mode) {
    Printf("INFO: --fast-mode is deprecated\n");
  }
  bool ignore_in_dtor = false;
  FindBoolFlag("ignore_in_dtor", false, args, &ignore_in_dtor);
  if (ignore_in_dtor) {
    Printf("INFO: --ignore-in-dtor is deprecated\n");
  }

  int has_phb = FindBoolFlag("pure_happens_before", true, args,
                              &G_flags->pure_happens_before);
  bool hybrid = false;
  int has_hyb = FindBoolFlag("hybrid", false, args, &hybrid);
  if (has_hyb && has_phb) {
    Printf("INFO: --hybrid and --pure-happens-before"
           " is mutually exclusive; ignoring the --hybrid switch\n");
  } else if (has_hyb && !has_phb) {
    G_flags->pure_happens_before = !hybrid;
  }

  FindBoolFlag("show_expected_races", false, args,
               &G_flags->show_expected_races);
  FindBoolFlag("demangle", true, args, &G_flags->demangle);

  FindBoolFlag("announce_threads", false, args, &G_flags->announce_threads);
  FindBoolFlag("full_output", false, args, &G_flags->full_output);
  FindBoolFlag("show_states", false, args, &G_flags->show_states);
  FindBoolFlag("show_proc_self_status", false, args,
               &G_flags->show_proc_self_status);
  FindBoolFlag("show_valgrind_context", false, args,
               &G_flags->show_valgrind_context);
  FindBoolFlag("suggest_happens_before_arcs", true, args,
               &G_flags->suggest_happens_before_arcs);
  FindBoolFlag("show_pc", false, args, &G_flags->show_pc);
  FindBoolFlag("full_stack_frames", false, args, &G_flags->full_stack_frames);
  FindBoolFlag("free_is_write", true, args, &G_flags->free_is_write);
  FindBoolFlag("exit_after_main", false, args, &G_flags->exit_after_main);

  FindIntFlag("show_stats", 0, args, &G_flags->show_stats);
  FindBoolFlag("trace_profile", false, args, &G_flags->trace_profile);
  FindBoolFlag("color", false, args, &G_flags->color);
  FindBoolFlag("html", false, args, &G_flags->html);
#ifdef TS_OFFLINE
  bool show_pid_default = false;
#else
  bool show_pid_default = true;
#endif
  FindBoolFlag("show_pid", show_pid_default, args, &G_flags->show_pid);
  FindBoolFlag("save_ignore_context", DEBUG_MODE ? true : false, args,
               &G_flags->save_ignore_context);

  FindIntFlag("dry_run", 0, args, &G_flags->dry_run);
  FindBoolFlag("report_races", true, args, &G_flags->report_races);
  FindIntFlag("locking_scheme", 1, args, &G_flags->locking_scheme);
  FindBoolFlag("unlock_on_mutex_destroy", true, args,
               &G_flags->unlock_on_mutex_destroy);

  FindIntFlag("sample_events", 0, args, &G_flags->sample_events);
  FindIntFlag("sample_events_depth", 2, args, &G_flags->sample_events_depth);

  FindIntFlag("debug_level", 1, args, &G_flags->debug_level);
  FindStringFlag("debug_phase", args, &G_flags->debug_phase);
  FindIntFlag("trace_level", 0, args, &G_flags->trace_level);

  FindIntFlag("literace_sampling", 0, args, &G_flags->literace_sampling);
  FindIntFlag("sampling", 0, args, &G_flags->literace_sampling);
  CHECK(G_flags->literace_sampling < 32);
  CHECK(G_flags->literace_sampling >= 0);
  FindBoolFlag("start_with_global_ignore_on", false, args,
               &G_flags->start_with_global_ignore_on);

  FindStringFlag("fullpath_after", args, &G_flags->file_prefix_to_cut);
  FindStringFlag("file_prefix_to_cut", args, &G_flags->file_prefix_to_cut);
  for (size_t i = 0; i < G_flags->file_prefix_to_cut.size(); i++) {
    G_flags->file_prefix_to_cut[i] =
        ConvertToPlatformIndependentPath(G_flags->file_prefix_to_cut[i]);
  }

  FindStringFlag("ignore", args, &G_flags->ignore);
  FindStringFlag("whitelist", args, &G_flags->whitelist);
  FindBoolFlag("ignore_unknown_pcs", false, args, &G_flags->ignore_unknown_pcs);

  FindBoolFlag("thread_coverage", false, args, &G_flags->thread_coverage);
  
  FindBoolFlag("atomicity", false, args, &G_flags->atomicity);
  if (G_flags->atomicity) {
    // When doing atomicity violation checking we should not 
    // create h-b arcs between Unlocks and Locks.
    G_flags->pure_happens_before = false;
  }

  FindBoolFlag("call_coverage", false, args, &G_flags->call_coverage);
  FindStringFlag("dump_events", args, &G_flags->dump_events);
  FindBoolFlag("symbolize", true, args, &G_flags->symbolize);

  FindIntFlag("trace_addr", 0, args,
              reinterpret_cast<intptr_t*>(&G_flags->trace_addr));

  FindIntFlag("max_mem_in_mb", 0, args, &G_flags->max_mem_in_mb);
  FindBoolFlag("offline", false, args, &G_flags->offline);
  FindBoolFlag("attach_mode", false, args, &G_flags->attach_mode);
  if (G_flags->max_mem_in_mb == 0) {
    G_flags->max_mem_in_mb = GetMemoryLimitInMb();
  }

  vector<string> summary_file_tmp;
  FindStringFlag("summary_file", args, &summary_file_tmp);
  if (summary_file_tmp.size() > 0) {
    G_flags->summary_file = summary_file_tmp.back();
  }

  vector<string> log_file_tmp;
  FindStringFlag("log_file", args, &log_file_tmp);
  if (log_file_tmp.size() > 0) {
    G_flags->log_file = log_file_tmp.back();
  }

  G_flags->tsan_program_name = "valgrind --tool=tsan";
  FindStringFlag("tsan_program_name", args, &G_flags->tsan_program_name);

  G_flags->tsan_url = "http://code.google.com/p/data-race-test";
  FindStringFlag("tsan_url", args, &G_flags->tsan_url);

  FindStringFlag("suppressions", args, &G_flags->suppressions);
  FindBoolFlag("gen_suppressions", false, args,
               &G_flags->generate_suppressions);

  FindIntFlag("error_exitcode", 0, args, &G_flags->error_exitcode);
  FindIntFlag("flush_period", 0, args, &G_flags->flush_period);
  FindBoolFlag("trace_children", false, args, &G_flags->trace_children);

  FindIntFlag("max_sid", kMaxSID, args, &G_flags->max_sid);
  kMaxSID = G_flags->max_sid;
  if (kMaxSID <= 100000) {
    Printf("Error: max-sid should be at least 100000. Exiting\n");
    exit(1);
  }
  FindIntFlag("max_sid_before_flush", (kMaxSID * 15) / 16, args, 
              &G_flags->max_sid_before_flush);
  kMaxSIDBeforeFlush = G_flags->max_sid_before_flush;

  FindIntFlag("num_callers_in_history", kSizeOfHistoryStackTrace, args,
              &G_flags->num_callers_in_history);
  kSizeOfHistoryStackTrace = G_flags->num_callers_in_history;

  // Cut stack under the following default functions.
  G_flags->cut_stack_below.push_back("TSanThread*ThreadBody*");
  G_flags->cut_stack_below.push_back("ThreadSanitizerStartThread");
  G_flags->cut_stack_below.push_back("start_thread");
  G_flags->cut_stack_below.push_back("BaseThreadInitThunk");
  FindStringFlag("cut_stack_below", args, &G_flags->cut_stack_below);

  FindIntFlag("num_callers", 16, args, &G_flags->num_callers);

  G_flags->max_n_threads        = 100000;

  if (G_flags->full_output) {
    G_flags->announce_threads = true;
    G_flags->show_pc = true;
    G_flags->full_stack_frames = true;
    G_flags->show_states = true;
    G_flags->file_prefix_to_cut.clear();
  }

  FindIntFlag("race_verifier_sleep_ms", 100, args,
      &G_flags->race_verifier_sleep_ms);
  FindStringFlag("race_verifier", args, &G_flags->race_verifier);
  FindStringFlag("race_verifier_extra", args, &G_flags->race_verifier_extra);
  g_race_verifier_active =
      !(G_flags->race_verifier.empty() && G_flags->race_verifier_extra.empty());
  if (g_race_verifier_active) {
    Printf("INFO: ThreadSanitizer running in Race Verifier mode.\n");
  }

  FindBoolFlag("nacl_untrusted", false, args, &G_flags->nacl_untrusted);
  FindBoolFlag("threaded_analysis", false, args, &G_flags->threaded_analysis);

  FindBoolFlag("sched_shake", false, args, &G_flags->sched_shake);
  FindBoolFlag("api_ambush", false, args, &G_flags->api_ambush);

  FindBoolFlag("enable_atomic", false, args, &G_flags->enable_atomic);

  if (!args->empty()) {
    ReportUnknownFlagAndExit(args->front());
  }

  debug_expected_races = PhaseDebugIsOn("expected_races");
  debug_benign_races = PhaseDebugIsOn("benign_races");
  debug_malloc = PhaseDebugIsOn("malloc");
  debug_free = PhaseDebugIsOn("free");
  debug_thread = PhaseDebugIsOn("thread");
  debug_ignore = PhaseDebugIsOn("ignore");
  debug_rtn = PhaseDebugIsOn("rtn");
  debug_lock = PhaseDebugIsOn("lock");
  debug_wrap = PhaseDebugIsOn("wrap");
  debug_ins = PhaseDebugIsOn("ins");
  debug_shadow_stack = PhaseDebugIsOn("shadow_stack");
  debug_happens_before = PhaseDebugIsOn("happens_before");
  debug_cache = PhaseDebugIsOn("cache");
  debug_race_verifier = PhaseDebugIsOn("race_verifier");
  debug_atomic = PhaseDebugIsOn("atomic");
}

// -------- ThreadSanitizer ------------------ {{{1

// Setup the list of functions/images/files to ignore.
static void SetupIgnore() {
  g_ignore_lists = new IgnoreLists;
  g_white_lists = new IgnoreLists;

  // Add some major ignore entries so that tsan remains sane
  // even w/o any ignore file. First - for all platforms.
  g_ignore_lists->ignores.push_back(IgnoreFun("ThreadSanitizerStartThread"));
  g_ignore_lists->ignores.push_back(IgnoreFun("exit"));
  g_ignore_lists->ignores.push_back(IgnoreFun("longjmp"));

  // Dangerous: recursively ignoring vfprintf hides races on printf arguments.
  // See PrintfTests in unittest/racecheck_unittest.cc
  // TODO(eugenis): Do something about this.
  // http://code.google.com/p/data-race-test/issues/detail?id=53
  g_ignore_lists->ignores_r.push_back(IgnoreFun("vfprintf"));

  // do not create segments in our Replace_* functions
  g_ignore_lists->ignores_hist.push_back(IgnoreFun("Replace_memcpy"));
  g_ignore_lists->ignores_hist.push_back(IgnoreFun("Replace_memchr"));
  g_ignore_lists->ignores_hist.push_back(IgnoreFun("Replace_strcpy"));
  g_ignore_lists->ignores_hist.push_back(IgnoreFun("Replace_strchr"));
  g_ignore_lists->ignores_hist.push_back(IgnoreFun("Replace_strchrnul"));
  g_ignore_lists->ignores_hist.push_back(IgnoreFun("Replace_strrchr"));
  g_ignore_lists->ignores_hist.push_back(IgnoreFun("Replace_strlen"));
  g_ignore_lists->ignores_hist.push_back(IgnoreFun("Replace_strcmp"));

  // Ignore everything in our own file.
  g_ignore_lists->ignores.push_back(IgnoreFile("*ts_valgrind_intercepts.c"));

#ifndef _MSC_VER
  // POSIX ignores
  g_ignore_lists->ignores.push_back(IgnoreObj("*/libpthread*"));
  g_ignore_lists->ignores.push_back(IgnoreObj("*/ld-2*.so"));
  g_ignore_lists->ignores.push_back(IgnoreFun("pthread_create"));
  g_ignore_lists->ignores.push_back(IgnoreFun("pthread_create@*"));
  g_ignore_lists->ignores.push_back(IgnoreFun("pthread_create_WRK"));
  g_ignore_lists->ignores.push_back(IgnoreFun("__cxa_*"));
  g_ignore_lists->ignores.push_back(
      IgnoreFun("*__gnu_cxx*__exchange_and_add*"));
  g_ignore_lists->ignores.push_back(IgnoreFun("__lll_mutex_*"));
  g_ignore_lists->ignores.push_back(IgnoreFun("__lll_*lock_*"));
  g_ignore_lists->ignores.push_back(IgnoreFun("__fprintf_chk"));
  g_ignore_lists->ignores.push_back(IgnoreFun("_IO_file_xsputn*"));
  // fflush internals
  g_ignore_lists->ignores.push_back(IgnoreFun("_IO_adjust_column"));
  g_ignore_lists->ignores.push_back(IgnoreFun("_IO_flush_all_lockp"));

  g_ignore_lists->ignores.push_back(IgnoreFun("__sigsetjmp"));
  g_ignore_lists->ignores.push_back(IgnoreFun("__sigjmp_save"));
  g_ignore_lists->ignores.push_back(IgnoreFun("_setjmp"));
  g_ignore_lists->ignores.push_back(IgnoreFun("_longjmp_unwind"));

  g_ignore_lists->ignores.push_back(IgnoreFun("__mktime_internal"));

  // http://code.google.com/p/data-race-test/issues/detail?id=40
  g_ignore_lists->ignores_r.push_back(IgnoreFun("_ZNSsD1Ev"));

  g_ignore_lists->ignores_r.push_back(IgnoreFun("gaih_inet"));
  g_ignore_lists->ignores_r.push_back(IgnoreFun("getaddrinfo"));
  g_ignore_lists->ignores_r.push_back(IgnoreFun("gethostbyname2_r"));

  #ifdef VGO_darwin
    // Mac-only ignores
    g_ignore_lists->ignores.push_back(IgnoreObj("/usr/lib/dyld"));
    g_ignore_lists->ignores.push_back(IgnoreObj("/usr/lib/libobjc.A.dylib"));
    g_ignore_lists->ignores.push_back(IgnoreObj("*/libSystem.*.dylib"));
    g_ignore_lists->ignores_r.push_back(IgnoreFun("__CFDoExternRefOperation"));
    g_ignore_lists->ignores_r.push_back(IgnoreFun("_CFAutoreleasePoolPop"));
    g_ignore_lists->ignores_r.push_back(IgnoreFun("_CFAutoreleasePoolPush"));
    g_ignore_lists->ignores_r.push_back(IgnoreFun("OSAtomicAdd32"));
    g_ignore_lists->ignores_r.push_back(IgnoreTriple("_dispatch_Block_copy",
                                            "/usr/lib/libSystem.B.dylib", "*"));

    // pthread_lib_{enter,exit} shouldn't give us any reports since they
    // have IGNORE_ALL_ACCESSES_BEGIN/END but they do give the reports...
    g_ignore_lists->ignores_r.push_back(IgnoreFun("pthread_lib_enter"));
    g_ignore_lists->ignores_r.push_back(IgnoreFun("pthread_lib_exit"));
  #endif
#else
  // Windows-only ignores
  g_ignore_lists->ignores.push_back(IgnoreObj("*ole32.dll"));
  g_ignore_lists->ignores.push_back(IgnoreObj("*OLEAUT32.dll"));
  g_ignore_lists->ignores.push_back(IgnoreObj("*MSCTF.dll"));
  g_ignore_lists->ignores.push_back(IgnoreObj("*ntdll.dll"));
  g_ignore_lists->ignores.push_back(IgnoreObj("*mswsock.dll"));
  g_ignore_lists->ignores.push_back(IgnoreObj("*WS2_32.dll"));
  g_ignore_lists->ignores.push_back(IgnoreObj("*msvcrt.dll"));
  g_ignore_lists->ignores.push_back(IgnoreObj("*kernel32.dll"));
  g_ignore_lists->ignores.push_back(IgnoreObj("*ADVAPI32.DLL"));

  g_ignore_lists->ignores.push_back(IgnoreFun("_EH_epilog3"));
  g_ignore_lists->ignores.push_back(IgnoreFun("_EH_prolog3_catch"));
  g_ignore_lists->ignores.push_back(IgnoreFun("unnamedImageEntryPoint"));
  g_ignore_lists->ignores.push_back(IgnoreFun("_Mtxunlock"));
  g_ignore_lists->ignores.push_back(IgnoreFun("IsNLSDefinedString"));

  g_ignore_lists->ignores_r.push_back(IgnoreFun("RtlDestroyQueryDebugBuffer"));
  g_ignore_lists->ignores_r.push_back(IgnoreFun("BCryptGenerateSymmetricKey"));
  g_ignore_lists->ignores_r.push_back(IgnoreFun("SHGetItemFromDataObject"));

  // http://code.google.com/p/data-race-test/issues/detail?id=53
  g_ignore_lists->ignores_r.push_back(IgnoreFun("_stbuf"));
  g_ignore_lists->ignores_r.push_back(IgnoreFun("_getptd"));

  // TODO(timurrrr): Add support for FLS (fiber-local-storage)
  // http://code.google.com/p/data-race-test/issues/detail?id=55
  g_ignore_lists->ignores_r.push_back(IgnoreFun("_freefls"));
#endif

#ifdef ANDROID
  // Android does not have a libpthread; pthread_* functions live in libc.
  // We have to ignore them one-by-one.
  g_ignore_lists->ignores.push_back(IgnoreFun("pthread_*"));
  g_ignore_lists->ignores.push_back(IgnoreFun("__init_tls"));
#endif

  // Now read the ignore/whitelist files.
  for (size_t i = 0; i < G_flags->ignore.size(); i++) {
    string file_name = G_flags->ignore[i];
    Report("INFO: Reading ignore file: %s\n", file_name.c_str());
    string str = ReadFileToString(file_name, true);
    ReadIgnoresFromString(str, g_ignore_lists);
  }
  for (size_t i = 0; i < G_flags->whitelist.size(); i++) {
    string file_name = G_flags->whitelist[i];
    Report("INFO: Reading whitelist file: %s\n", file_name.c_str());
    string str = ReadFileToString(file_name, true);
    ReadIgnoresFromString(str, g_white_lists);
  }
}

void ThreadSanitizerSetUnwindCallback(ThreadSanitizerUnwindCallback cb) {
  G_detector->SetUnwindCallback(cb);
}

void ThreadSanitizerNaclUntrustedRegion(uintptr_t mem_start, uintptr_t mem_end) {
  g_nacl_mem_start = mem_start;
  g_nacl_mem_end = mem_end;
}

bool AddrIsInNaclUntrustedRegion(uintptr_t addr) {
  return addr >= g_nacl_mem_start && addr < g_nacl_mem_end;
}

bool ThreadSanitizerIgnoreForNacl(uintptr_t addr) {
  // Ignore trusted addresses if tracing untrusted code, and ignore untrusted
  // addresses otherwise.
  return G_flags->nacl_untrusted != AddrIsInNaclUntrustedRegion(addr);
}

bool ThreadSanitizerWantToInstrumentSblock(uintptr_t pc) {
  string img_name, rtn_name, file_name;
  int line_no;
  G_stats->pc_to_strings++;
  PcToStrings(pc, false, &img_name, &rtn_name, &file_name, &line_no);

  if (g_white_lists->ignores.size() > 0) {
    bool in_white_list = TripleVectorMatchKnown(g_white_lists->ignores, 
                                                rtn_name, img_name, file_name);
    if (in_white_list) {
      if (debug_ignore) {
        Report("INFO: Whitelisted rtn: %s\n", rtn_name.c_str());
      }
    } else {
      return false;
    }
  }

  if (G_flags->ignore_unknown_pcs && rtn_name == "(no symbols)") {
    if (debug_ignore) {
      Report("INFO: not instrumenting unknown function at %p\n", pc);
    }
    return false;
  }

  bool ignore = TripleVectorMatchKnown(g_ignore_lists->ignores,
                                       rtn_name, img_name, file_name) ||
                TripleVectorMatchKnown(g_ignore_lists->ignores_r,
                                       rtn_name, img_name, file_name);
  if (debug_ignore) {
    Printf("%s: pc=%p file_name=%s img_name=%s rtn_name=%s ret=%d\n",
           __FUNCTION__, pc, file_name.c_str(), img_name.c_str(),
           rtn_name.c_str(), !ignore);
  }
  bool nacl_ignore = ThreadSanitizerIgnoreForNacl(pc);
  return !(ignore || nacl_ignore);
}

bool ThreadSanitizerWantToCreateSegmentsOnSblockEntry(uintptr_t pc) {
  string rtn_name;
  rtn_name = PcToRtnName(pc, false);
  if (G_flags->keep_history == 0)
    return false;
  return !(TripleVectorMatchKnown(g_ignore_lists->ignores_hist,
                                  rtn_name, "", ""));
}

// Returns true if function at "pc" is marked as "fun_r" in the ignore file.
bool NOINLINE ThreadSanitizerIgnoreAccessesBelowFunction(uintptr_t pc) {
  ScopedMallocCostCenter cc(__FUNCTION__);
  typedef unordered_map<uintptr_t, bool> Cache;
  static Cache *cache = NULL;
  {
    TIL ignore_below_lock(ts_ignore_below_lock, 18);
    if (!cache)
      cache = new Cache;

    // Fast path - check if we already know the answer.
    Cache::iterator i = cache->find(pc);
    if (i != cache->end())
      return i->second;
  }

  string rtn_name = PcToRtnName(pc, false);
  bool ret =
      TripleVectorMatchKnown(g_ignore_lists->ignores_r, rtn_name, "", "");

  if (DEBUG_MODE) {
    // Heavy test for NormalizeFunctionName: test on all possible inputs in
    // debug mode. TODO(timurrrr): Remove when tested.
    NormalizeFunctionName(PcToRtnName(pc, true));
  }

  // Grab the lock again
  TIL ignore_below_lock(ts_ignore_below_lock, 19);
  if (ret && debug_ignore) {
    Report("INFO: ignoring all accesses below the function '%s' (%p)\n",
           PcToRtnNameAndFilePos(pc).c_str(), pc);
  }
  return ((*cache)[pc] = ret);
}

// We intercept a user function with this name
// and answer the user query with a non-NULL string.
extern "C" const char *ThreadSanitizerQuery(const char *query) {
  const char *ret = "0";
  string str(query);
  if (str == "pure_happens_before" && G_flags->pure_happens_before == true) {
    ret = "1";
  }
  if (str == "hybrid_full" &&
      G_flags->pure_happens_before == false) {
    ret = "1";
  }
  if (str == "race_verifier" && g_race_verifier_active == true) {
    ret = "1";
  }
  if (DEBUG_MODE && G_flags->debug_level >= 2) {
    Printf("ThreadSanitizerQuery(\"%s\") = \"%s\"\n", query, ret);
  }
  if (str == "trace-level=0") {
    Report("INFO: trace-level=0\n");
    G_flags->trace_level = 0;
    debug_happens_before = false;
  }
  if (str == "trace-level=1") {
    Report("INFO: trace-level=1\n");
    G_flags->trace_level = 1;
    debug_happens_before = true;
  }
  return ret;
}

extern void ThreadSanitizerInit() {
  ScopedMallocCostCenter cc("ThreadSanitizerInit");
  ts_lock = new TSLock;
  ts_ignore_below_lock = new TSLock;
  g_so_far_only_one_thread = true;
  ANNOTATE_BENIGN_RACE(&g_so_far_only_one_thread, "real benign race");
  CHECK_EQ(sizeof(ShadowValue), 8);
  CHECK(G_flags);
  G_stats        = new Stats;
  SetupIgnore();

  G_detector     = new Detector;
  G_cache        = new Cache;
  G_expected_races_map = new ExpectedRacesMap;
  G_heap_map           = new HeapMap<HeapInfo>;
  G_thread_stack_map   = new HeapMap<ThreadStackInfo>;
  {
    ScopedMallocCostCenter cc1("Segment::InitClassMembers");
    Segment::InitClassMembers();
  }
  SegmentSet::InitClassMembers();
  CacheLine::InitClassMembers();
  TSanThread::InitClassMembers();
  Lock::InitClassMembers();
  LockSet::InitClassMembers();
  EventSampler::InitClassMembers();
  VTS::InitClassMembers();
  // TODO(timurrrr): make sure *::InitClassMembers() are called only once for
  // each class
  g_publish_info_map = new PublishInfoMap;
  g_stack_trace_free_list = new StackTraceFreeList;
  g_pcq_map = new PCQMap;
  g_atomicCore = new TsanAtomicCore();


  if (G_flags->html) {
    c_bold    = "<font ><b>";
    c_red     = "<font color=red><b>";
    c_green   = "<font color=green><b>";
    c_magenta = "<font color=magenta><b>";
    c_cyan    = "<font color=cyan><b>";
    c_blue   = "<font color=blue><b>";
    c_yellow  = "<font color=yellow><b>";
    c_default = "</b></font>";
  } else if (G_flags->color) {
    // Enable ANSI colors.
    c_bold    = "\033[1m";
    c_red     = "\033[31m";
    c_green   = "\033[32m";
    c_yellow  = "\033[33m";
    c_blue    = "\033[34m";
    c_magenta = "\033[35m";
    c_cyan    = "\033[36m";
    c_default = "\033[0m";
  }

  if (G_flags->verbosity >= 1) {
    Report("INFO: Started pid %d\n",  getpid());
  }
  if (G_flags->start_with_global_ignore_on) {
    global_ignore = true;
    Report("INFO: STARTING WITH GLOBAL IGNORE ON\n");
  }
  ANNOTATE_BENIGN_RACE(&g_lock_era,
                       "g_lock_era may be incremented in a racey way");
}

extern void ThreadSanitizerFini() {
  G_detector->HandleProgramEnd();
}

extern void ThreadSanitizerDumpAllStacks() {
  // first, print running threads.
  for (int i = 0; i < TSanThread::NumberOfThreads(); i++) {
    TSanThread *t = TSanThread::Get(TID(i));
    if (!t || !t->is_running()) continue;
    Report("T%d\n", i);
    t->ReportStackTrace();
  }
  // now print all dead threds.
  for (int i = 0; i < TSanThread::NumberOfThreads(); i++) {
    TSanThread *t = TSanThread::Get(TID(i));
    if (!t || t->is_running()) continue;
    Report("T%d (not running)\n", i);
    t->ReportStackTrace();
  }
}


extern void ThreadSanitizerHandleOneEvent(Event *e) {
  // Lock is inside on some paths.
  G_detector->HandleOneEvent(e);
}

TSanThread *ThreadSanitizerGetThreadByTid(int32_t tid) {
  return TSanThread::Get(TID(tid));
}

extern NOINLINE void ThreadSanitizerHandleTrace(int32_t tid, TraceInfo *trace_info,
                                       uintptr_t *tleb) {
  ThreadSanitizerHandleTrace(TSanThread::Get(TID(tid)), trace_info, tleb);
}
extern NOINLINE void ThreadSanitizerHandleTrace(TSanThread *thr, TraceInfo *trace_info,
                                                uintptr_t *tleb) {
  DCHECK(thr);
  // The lock is taken inside on the slow path.
  G_detector->HandleTrace(thr,
                          trace_info->mops(),
                          trace_info->n_mops(),
                          trace_info->pc(),
                          tleb, /*need_locking=*/true);
}

extern NOINLINE void ThreadSanitizerHandleOneMemoryAccess(TSanThread *thr,
                                                          MopInfo mop,
                                                          uintptr_t addr) {
  DCHECK(thr);
  G_detector->HandleTrace(thr,
                          &mop,
                          1,
                          mop.create_sblock() ? mop.pc() : 0,
                          &addr, /*need_locking=*/true);
}

void NOINLINE ThreadSanitizerHandleRtnCall(int32_t tid, uintptr_t call_pc,
                                         uintptr_t target_pc,
                                         IGNORE_BELOW_RTN ignore_below) {
  // This does locking on a cold path. Hot path in thread-local.
  G_detector->HandleRtnCall(TID(tid), call_pc, target_pc, ignore_below);

  if (G_flags->sample_events) {
    static EventSampler sampler;
    TSanThread *thr = TSanThread::Get(TID(tid));
    sampler.Sample(thr, "RTN_CALL", true);
  }
}
void NOINLINE ThreadSanitizerHandleRtnExit(int32_t tid) {
  // This is a thread-local operation, no need for locking.
  TSanThread::Get(TID(tid))->HandleRtnExit();
}

static bool ThreadSanitizerPrintReport(ThreadSanitizerReport *report) {
  return G_detector->reports_.PrintReport(report);
}


// -------- TsanAtomicImplementation ------------------ {{{1

// Atomic operation handler.
// The idea of atomic handling is as simple as follows.
// * First, we handle it as normal memory access,
//     however with race reporting suppressed. That is, we won't produce any
//     race reports during atomic access, but we can produce race reports
//     later during normal memory accesses that race with the access.
// * Then, we do the actual atomic memory access.
//     It's executed in an atomic fashion, because there can be simultaneous
//     atomic accesses from non-instrumented code (FUTEX_OP is a notable
//     example).
// * Finally, we update simulated memory model state according to
//     the access type and associated memory order as follows.
//     For writes and RMWs we create a new entry in the modification order
//     of the variable. For reads we scan the modification order starting
//     from the latest entry and going back in time, during the scan we decide
//     what entry the read returns. A separate VTS (happens-before edges)
//     is associated with each entry in the modification order, so that a load
//     acquires memory visibility from the exact release-sequence associated
//     with the loaded value.
// For details of memory modelling refer to sections 1.10 and 29
//     of C++0x standard:
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2011/n3242.pdf
uint64_t ThreadSanitizerHandleAtomicOp(int32_t tid,
                                       uintptr_t pc,
                                       tsan_atomic_op op,
                                       tsan_memory_order mo,
                                       tsan_memory_order fail_mo,
                                       size_t size,
                                       void volatile* a,
                                       uint64_t v,
                                       uint64_t cmp) {
  if (G_flags->enable_atomic == false) {
    uint64_t newv = 0;
    uint64_t prev = 0;
    return tsan_atomic_do_op(op, mo, fail_mo, size, a, v, cmp, &newv, &prev);
  } else {
    uint64_t rv = 0;
    TSanThread* thr = TSanThread::Get(TID(tid));
    // Just a verification of the parameters.
    tsan_atomic_verify(op, mo, fail_mo, size, a);

    {
      TIL til(ts_lock, 0);
      uint64_t newv = 0;
      uint64_t prev = 0;
      // Handle it as a plain mop. Race reports are temporally suppressed,though.
      thr->HandleAtomicMop((uintptr_t)a, pc, op, mo, size);
      // Do the actual atomic operation. It's executed in an atomic fashion,
      // because there can be simultaneous atomic accesses
      // from non-instrumented code.
      rv = tsan_atomic_do_op(op, mo, fail_mo, size, a, v, cmp, &newv, &prev);

      PrintfIf(debug_atomic, "rv=%llu, newv=%llu, prev=%llu\n",
               (unsigned long long)rv,
               (unsigned long long)newv,
               (unsigned long long)prev);

      if (op != tsan_atomic_op_fence) {
        if (op == tsan_atomic_op_load) {
          // For reads it replaces the return value with a random value
          // from visible sequence of side-effects in the modification order
          // of the variable.
          rv = g_atomicCore->HandleRead(thr, (uintptr_t)a, rv,
                                        tsan_atomic_is_acquire(mo));
        } else if ((op == tsan_atomic_op_compare_exchange_weak
            || op == tsan_atomic_op_compare_exchange_strong)
            && cmp != rv) {
          // Failed compare_exchange is handled as read, because, well,
          // it's indeed just a read (at least logically).
          g_atomicCore->HandleRead(thr, (uintptr_t)a, rv,
                                   tsan_atomic_is_acquire(fail_mo));
        } else {
          // For writes and RMW operations it updates modification order
          // of the atomic variable.
          g_atomicCore->HandleWrite(thr, (uintptr_t)a, newv, prev,
                                    tsan_atomic_is_acquire(mo),
                                    tsan_atomic_is_release(mo),
                                    tsan_atomic_is_rmw(op));
        }
      }
    }

    PrintfIf(debug_atomic, "ATOMIC: %s-%s %p (%llu,%llu)=%llu\n",
             tsan_atomic_to_str(op),
             tsan_atomic_to_str(mo),
             a, (unsigned long long)v, (unsigned long long)cmp,
             (unsigned long long)rv);

    return rv;
  }
}


TsanAtomicCore::TsanAtomicCore() {
}


void TsanAtomicCore::HandleWrite(TSanThread* thr,
                                 uintptr_t a,
                                 uint64_t v,
                                 uint64_t prev,
                                 bool const is_acquire,
                                 bool const is_release,
                                 bool const is_rmw) {
  PrintfIf(debug_atomic, "HIST(%p): store acquire=%u, release=%u, rmw=%u\n",
           (void*)a, is_acquire, is_release, is_rmw);
  Atomic* atomic = &atomic_map_[a];
  // Fix modification history if there were untracked accesses.
  AtomicFixHist(atomic, prev);
  AtomicHistoryEntry& hprv = atomic->hist
      [(atomic->hist_pos - 1) % Atomic::kHistSize];
  AtomicHistoryEntry& hist = atomic->hist
      [atomic->hist_pos % Atomic::kHistSize];
  // Fill in new entry in the modification history.
  hist.val = v;
  hist.tid = thr->tid();
  hist.clk = thr->vts()->clk(thr->tid());
  if (hist.vts != 0) {
    VTS::Unref(hist.vts);
    hist.vts = 0;
  }
  atomic->hist_pos += 1;

  // Update VTS according to memory access type and memory ordering.
  if (is_rmw) {
    if (is_release) {
      if (hprv.vts != 0) {
        hist.vts = VTS::Join(hprv.vts, thr->vts());
      } else {
        hist.vts = thr->vts()->Clone();
      }
    } else if (hprv.vts != 0) {
      hist.vts = hprv.vts->Clone();
    }
    if (is_acquire && hprv.vts != 0) {
      thr->NewSegmentForWait(hprv.vts);
    }
  } else {
    DCHECK(is_acquire == false);
    if (is_release) {
      hist.vts = thr->vts()->Clone();
    }
  }

  // Update the thread's VTS if it's relese memory access.
  if (is_release) {
    thr->NewSegmentForSignal();
    if (debug_happens_before) {
      Printf("T%d: Signal: %p:\n    %s %s\n    %s\n",
             thr->tid().raw(), a,
             thr->vts()->ToString().c_str(),
             Segment::ToString(thr->sid()).c_str(),
             hist.vts->ToString().c_str());
      if (G_flags->debug_level >= 1) {
        thr->ReportStackTrace();
      }
    }
  }
}


uint64_t TsanAtomicCore::HandleRead(TSanThread* thr,
                                    uintptr_t a,
                                    uint64_t v,
                                    bool is_acquire) {
  PrintfIf(debug_atomic, "HIST(%p): {\n", (void*)a);

  Atomic* atomic = &atomic_map_[a];
  // Fix modification history if there were untracked accesses.
  AtomicFixHist(atomic, v);
  AtomicHistoryEntry* hist0 = 0;
  int32_t seen_seq = 0;
  int32_t const seen_seq0 = atomic->last_seen.clock(thr->tid());
  // Scan modification order of the variable from the latest entry
  // back in time. For each side-effect (write) we determine as to
  // whether we have to yield the value or we can go back in time further.
  for (int32_t i = 0; i != Atomic::kHistSize; i += 1) {
    int32_t const idx = (atomic->hist_pos - i - 1);
    CHECK(idx >= 0);
    AtomicHistoryEntry& hist = atomic->hist[idx % Atomic::kHistSize];
    PrintfIf(debug_atomic, "HIST(%p):   #%u (tid=%u, clk=%u,"
           " val=%llu) vts=%u\n",
           (void*)a, (unsigned)i, (unsigned)hist.tid.raw(),
           (unsigned)hist.clk, (unsigned long long)hist.val,
           (unsigned)thr->vts()->clk(hist.tid));
    if (hist.tid.raw() == TID::kInvalidTID) {
      // We hit an uninialized entry, that is, it's an access to an unitialized
      // variable (potentially due to "race").
      // Unfortunately, it should not happen as of now.
      // TODO(dvyukov): how can we detect and report unitialized atomic reads?.
      // .
      hist0 = 0;
      break;
    } else if (i == Atomic::kHistSize - 1) {
      // It's the last entry so we have to return it
      // because we have to return something.
      PrintfIf(debug_atomic, "HIST(%p):   replaced: last\n", (void*)a);
      hist0 = &hist;
      break;
    } else if (seen_seq0 >= idx) {
      // The thread had already seen the entry so we have to return
      // at least it.
      PrintfIf(debug_atomic, "HIST(%p):   replaced: stability\n", (void*)a);
      hist0 = &hist;
      break;
    } else if (thr->vts()->clk(hist.tid) >= hist.clk) {
      // The write happened-before the read, so we have to return it.
      PrintfIf(debug_atomic, "HIST(%p):   replaced: ordering\n", (void*)a);
      hist0 = &hist;
      break;
    } else if (thr->random() % 2) {
      // We are not obliged to return the entry but we can (and decided to do).
      PrintfIf(debug_atomic, "HIST(%p):   replaced: coherence\n", (void*)a);
      seen_seq = idx;
      hist0 = &hist;
      break;
    } else {
      // Move on to the next (older) entry.
      PrintfIf(debug_atomic, "HIST(%p):   can be replaced but not\n", (void*)a);
    }
  }

  if (hist0 != 0) {
    v = hist0->val;
    // Acquire mamory visibility is needed.
    if (is_acquire) {
      if (hist0->vts != 0) {
        thr->NewSegmentForWait(hist0->vts);
      }

      if (debug_happens_before) {
        Printf("T%d: Wait: %p:\n    %s %s\n",
               thr->tid().raw(), a,
               thr->vts()->ToString().c_str(),
               Segment::ToString(thr->sid()).c_str());
        if (G_flags->debug_level >= 1) {
          thr->ReportStackTrace();
        }
      }
    }
    if (seen_seq != 0) {
      // Mark the entry as seen so we won't return any older entry later.
      atomic->last_seen.update(thr->tid(), seen_seq);
    }
  } else {
    CHECK("should never happen as of now" == 0);
    PrintfIf(debug_atomic, "HIST(%p): UNITIALIZED LOAD\n", (void*)a);
    v = thr->random();
  }
  PrintfIf(debug_atomic, "HIST(%p): } -> %llu\n",
      (void*)a, (unsigned long long)v);
  return v;
}


void TsanAtomicCore::ClearMemoryState(uintptr_t a, uintptr_t b) {
  DCHECK(a <= b);
  DCHECK(G_flags->enable_atomic || atomic_map_.empty());
  AtomicMap::iterator begin (atomic_map_.lower_bound(a));
  AtomicMap::iterator pos (begin);
  for (; pos != atomic_map_.end() && pos->first <= b; ++pos) {
    pos->second.reset();
  }
  atomic_map_.erase(begin, pos);
}


void TsanAtomicCore::AtomicFixHist(Atomic* atomic, uint64_t prev) {
  AtomicHistoryEntry& hprv = atomic->hist
      [(atomic->hist_pos - 1) % Atomic::kHistSize];
  // In case we had missed an atomic access (that is, an access from 
  // non-instrumented code), reset whole history and initialize it
  // with a single entry that happened "before world creation".
  if (prev != hprv.val) {
    PrintfIf(debug_atomic, "HIST RESET\n");
    atomic->reset();
    AtomicHistoryEntry& hist = atomic->hist
        [atomic->hist_pos % Atomic::kHistSize];
    hist.val = prev;
    hist.tid = TID(0);
    hist.clk = 0;
    atomic->hist_pos += 1;
  }
}


TsanAtomicCore::Atomic::Atomic() {
  reset(true);
}


void TsanAtomicCore::Atomic::reset(bool init) {
  hist_pos = sizeof(hist)/sizeof(hist[0]) + 1;
  for (size_t i = 0; i != sizeof(hist)/sizeof(hist[0]); i += 1) {
    hist[i].val = 0xBCEBC041;
    hist[i].tid = TID(TID::kInvalidTID);
    hist[i].clk = -1;
    if (init == false && hist[i].vts != 0)
      VTS::Unref(hist[i].vts);
    hist[i].vts = 0;
  }
  last_seen.reset();
}


// -------- TODO -------------------------- {{{1
// - Support configurable aliases for function names (is it doable in valgrind)?
// - Correctly support atomic operations (not just ignore).
// - Handle INC as just one write
//   - same for memset, etc
// - Implement correct handling of memory accesses with different sizes.
// - Do not create HB arcs between RdUnlock and RdLock
// - Compress cache lines
// - Optimize the case where a threads signals twice in a row on the same
//   address.
// - Fix --ignore-in-dtor if --demangle=no.
// - Use cpplint (http://code.google.com/p/google-styleguide)
// - Get rid of annoying casts in printfs.
// - Compress stack traces (64-bit only. may save up to 36 bytes per segment).
// end. {{{1
// vim:shiftwidth=2:softtabstop=2:expandtab:tw=80
