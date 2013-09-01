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

// Some parts of the code in this file are taken from the examples
// in DynamoRIO distribution, which have the following copyright.
/* **********************************************************
 * Copyright (c) 2003-2008 VMware, Inc.  All rights reserved.
 * **********************************************************/

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of VMware, Inc. nor the names of its contributors may be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL VMWARE, INC. OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

// Author: Konstantin Serebryany.
// Author: Timur Iskhodzhanov.
//
// ******* WARNING ********
// This code is experimental. Do not expect anything here to work.
// ***** END WARNING ******

#include "dr_api.h"

#include "ts_util.h"

#define EXTRA_REPLACE_PARAMS
#define REPORT_READ_RANGE(a,b)
#define REPORT_WRITE_RANGE(a,b)
#include "ts_replace.h"

#define Printf dr_printf

static void *g_lock;
static int   g_n_created_threads;

typedef unordered_map<intptr_t, string> SymbolsTable;
static SymbolsTable *sym_tab;

string *g_main_module_path;

//--------------- StackFrame ----------------- {{{1
struct StackFrame {
  uintptr_t pc;
  uintptr_t sp;
  StackFrame(uintptr_t p, uintptr_t s) : pc(p), sp(s) { }
};


//--------------- DrThread ----------------- {{{1
struct DrThread {
  int tid;  // A unique 0-based thread id.
  vector<StackFrame> shadow_stack;
};

static DrThread &GetCurrentThread(void *drcontext) {
  return *(DrThread*)dr_get_tls_field(drcontext);
}

//--------------- ShadowStack ----------------- {{{1
#define DEB_PR (0 && t.tid == 1)

static void PrintShadowStack(DrThread &t) {
  Printf("T%d Shadow stack (%d)\n", t.tid, (int)t.shadow_stack.size());
  for (int i = t.shadow_stack.size() - 1; i >= 0; i--) {
    uintptr_t pc = t.shadow_stack[i].pc;
    Printf("%s[%p]\n", g_main_module_path->c_str(), pc);
  }
  for (int i = t.shadow_stack.size() - 1; i >= 0; i--) {
    uintptr_t pc = t.shadow_stack[i].pc;
    uintptr_t sp = t.shadow_stack[i].sp;
    Printf("  sp=%p pc=%p\n", sp, pc);
  }
}

static void UpdateShadowStack(DrThread &t, uintptr_t sp) {
  while (t.shadow_stack.size() > 0 && sp >= t.shadow_stack.back().sp) {
    t.shadow_stack.pop_back();
    if (DEB_PR) {
      dr_mutex_lock(g_lock);
      Printf("T%d PopShadowStack\n", t.tid);
      PrintShadowStack(t);
      dr_mutex_unlock(g_lock);
    }
  }
}

static void PushShadowStack(DrThread &t, uintptr_t pc, uintptr_t target_pc, uintptr_t sp) {
  if (t.shadow_stack.size() > 0) {
    t.shadow_stack.back().pc = pc;
  }
  t.shadow_stack.push_back(StackFrame(target_pc, sp));
  if (DEB_PR) {
    dr_mutex_lock(g_lock);
    Printf("T%d PushShadowStack %p %p %d\n", t.tid, pc, target_pc, sp);
    PrintShadowStack(t);
    dr_mutex_unlock(g_lock);
  }
}

//--------------- callbacks ----------------- {{{1
static void OnEvent_ThreadInit(void *drcontext) {
  DrThread *t_ptr = new DrThread;
  DrThread &t = *t_ptr;

  dr_mutex_lock(g_lock);
  t.tid = g_n_created_threads++;
  dr_mutex_unlock(g_lock);

  dr_set_tls_field(drcontext, t_ptr);

  dr_printf("T%d %s\n", t.tid, (char*)__FUNCTION__+8);
}

static void OnEvent_ThreadExit(void *drcontext) {
  DrThread &t = GetCurrentThread(drcontext);
  dr_printf("T%d %s\n", t.tid, (char*)__FUNCTION__+8);
}

void OnEvent_ModuleLoaded(void *drcontext, const module_data_t *info,
                          bool loaded) {
  // if this assertion fails, your DynamoRIO is too old. You need rev261 with some patches...
  CHECK(info->full_path);

  dr_printf("%s: %s (%s)\n", __FUNCTION__,
            dr_module_preferred_name(info), info->full_path);
  if (g_main_module_path == NULL) {
    g_main_module_path = new string(info->full_path);
  }
}

static void OnEvent_Exit(void) {
  dr_printf("ThreadSanitizerDynamoRio: done\n");
  dr_mutex_destroy(g_lock);
}

static void On_Mop(uintptr_t pc, size_t size, void *a, bool is_w) {
  void *drcontext = dr_get_current_drcontext();
  DrThread &t = GetCurrentThread(drcontext);
  if (t.tid == 777) {
    dr_fprintf(STDERR, "T%d pc=%p a=%p size=%ld %s\n", t.tid, pc, a, size, is_w ? "WRITE" : "READ");
  }
}

static void On_Read(uintptr_t pc, size_t size, void *a) {
  On_Mop(pc, size, a, false);
}

static void On_Write(uintptr_t pc, size_t size, void *a) {
  On_Mop(pc, size, a, true);
}

static void On_AnyCall(uintptr_t pc, uintptr_t target_pc, uintptr_t sp, bool is_direct) {
  void *drcontext = dr_get_current_drcontext();
  DrThread &t = GetCurrentThread(drcontext);
  // dr_fprintf(STDOUT, "T%d CALL %p => %p; sp=%p\n", t.tid, pc, target_pc, sp);
  PushShadowStack(t, pc, target_pc, sp);
}

static void On_DirectCall(uintptr_t pc, uintptr_t target_pc, uintptr_t sp) {
  On_AnyCall(pc, target_pc, sp, true);
}

static void On_IndirectCall(uintptr_t pc, uintptr_t target_pc, uintptr_t sp) {
  On_AnyCall(pc, target_pc, sp, false);
}

static void On_TraceEnter(uintptr_t pc, uintptr_t sp) {
  void *drcontext = dr_get_current_drcontext();
  DrThread &t = GetCurrentThread(drcontext);
  // dr_fprintf(STDOUT, "T%d TRACE:\n%p\n%p\n", t.tid, pc, sp);
  UpdateShadowStack(t, sp);
}

//--------------- instrumentation ----------------- {{{1
opnd_t opnd_create_base_disp_from_dst(opnd_t dst) {
  return opnd_create_base_disp(opnd_get_base(dst),
                               opnd_get_index(dst),
                               opnd_get_scale(dst),
                               opnd_get_disp(dst),
                               OPSZ_lea);
}

static void InstrumentOneMop(void* drcontext, instrlist_t *bb,
                             instr_t *instr, opnd_t opnd, bool is_w) {
  //   opnd_disassemble(drcontext, opnd, 1);
  //   dr_printf("  -- (%s opnd)\n", is_w ? "write" : "read");
  void *callback = (void*)(is_w ? On_Write : On_Read);
  int size = opnd_size_in_bytes(opnd_get_size(opnd));

  instr_t *tmp_instr = NULL;
  reg_id_t reg = REG_XAX;

  /* save %xax */
  dr_save_reg(drcontext, bb, instr, reg, SPILL_SLOT_2);

  if (opnd_is_base_disp(opnd)) {
    /* lea opnd => %xax */
    opnd_set_size(&opnd, OPSZ_lea);
    tmp_instr = INSTR_CREATE_lea(drcontext,
                                 opnd_create_reg(reg),
                                 opnd);
  } else if(
#ifdef X86_64
      opnd_is_rel_addr(opnd) ||
#endif
      opnd_is_abs_addr(opnd)) {
    tmp_instr = INSTR_CREATE_mov_imm(drcontext,
                                     opnd_create_reg(reg),
                                     OPND_CREATE_INTPTR(opnd_get_addr(opnd)));
  }
  if (tmp_instr) {
    // CHECK(tmp_instr);
    instrlist_meta_preinsert(bb, instr, tmp_instr);

    /* clean call */
    dr_insert_clean_call(drcontext, bb, instr, callback, false,
                         3,
                         OPND_CREATE_INTPTR(instr_get_app_pc(instr)),
                         OPND_CREATE_INT32(size),
                         opnd_create_reg(reg));
    /* restore %xax */
    dr_restore_reg(drcontext, bb, instr, REG_XAX, SPILL_SLOT_2);
  } else {
    dr_printf("%s ????????????????????\n", __FUNCTION__);
  }
}

static void InstrumentMopInstruction(void *drcontext,
                                     instrlist_t *bb, instr_t *instr) {
  // reads:
  for (int a = 0; a < instr_num_srcs(instr); a++) {
    opnd_t curop = instr_get_src(instr, a);
    if (opnd_is_memory_reference(curop)) {
      InstrumentOneMop(drcontext, bb, instr, curop, false);
    }
  }
  // writes:
  for (int a = 0; a < instr_num_dsts(instr); a++) {
    opnd_t curop = instr_get_dst(instr, a);
    if (opnd_is_memory_reference(curop)) {
      InstrumentOneMop(drcontext, bb, instr, curop, true);
    }
  }
  //dr_printf("reads: %d writes: %d\n", n_reads, n_writes);
}

static void InstrumentInstruction(void *drcontext, instrlist_t *bb,
                                  instr_t *instr) {
  // instr_disassemble(drcontext, instr, 1);
  // dr_printf("  -- \n");
  if (instr_is_call_direct(instr)) {
    dr_insert_call_instrumentation(drcontext, bb, instr,
                                   (app_pc)On_DirectCall);
  } else if (instr_is_call_indirect(instr)) {
    dr_insert_mbr_instrumentation(drcontext, bb, instr,
                                  (app_pc)On_IndirectCall, SPILL_SLOT_1);

  } else if (instr_reads_memory(instr) || instr_writes_memory(instr)) {
    InstrumentMopInstruction(drcontext, bb, instr);
  }
}

static dr_emit_flags_t OnEvent_Trace(void *drcontext, void *tag,
                                     instrlist_t *trace, bool translating) {
  instr_t *first_instr = NULL;
  for (instr_t *instr = instrlist_first(trace); instr != NULL;
       instr = instr_get_next(instr)) {
    if (instr_get_app_pc(instr)) {
      first_instr = instr;
      break;
    }
  }
  if (first_instr) {
    // instr_disassemble(drcontext, first_instr, 1);
    // dr_printf("  -- in_trace %p\n", instr_get_app_pc(first_instr));
    dr_insert_clean_call(drcontext, trace, first_instr,
                         (void*)On_TraceEnter, false,
                         2,
                         OPND_CREATE_INTPTR(instr_get_app_pc(first_instr)),
                         opnd_create_reg(REG_XSP)
                         );
  }
  return DR_EMIT_DEFAULT;
}

int replace_foo(int i, int j, int k) {
  dr_printf(" dy 'foo_replace'(%i, %i, %i)\n", i, j, k);
  return 1;
}

typedef unordered_map<intptr_t, void*> FunctionsReplaceMap;
static FunctionsReplaceMap *fun_replace_map;

namespace wrap {

int (*orig_foo)(int,int,int) = NULL;
int in_wrapper = 0;  // TODO: Make it thread-local

static int wrapped_foo(int i, int j, int k) {
  in_wrapper = 1;

  dr_printf(" dy 'foo_wrap'(%i, %i, %i)\n", i, j, k);
  dr_printf("orig_foo = %p\n", orig_foo);
  int ret = 13;
  if (orig_foo != NULL)
    ret = orig_foo(i, j, k) + 4200;
  else
    dr_printf("ERROR! orig_foo is not set!\n");/**/

  in_wrapper = 0;
  return ret;
}

int is_in_wrapper(int arg) {
  // TODO: this may not work well with recursive functions
  return in_wrapper;
}
}

void print_bb(void* drcontext, instrlist_t *bb, const char * desc) {
  dr_printf("==================\n");
  dr_printf("%s:\n", desc);
  for (instr_t *i = instrlist_first(bb); i != NULL; i = instr_get_next(i)) {
    instr_disassemble(drcontext, i, 1);
    dr_printf("\n");
  }
  dr_printf("==================\n");
}

static dr_emit_flags_t OnEvent_BB(void* drcontext, void *tag, instrlist_t *bb,
                                  bool for_trace, bool translating) {
  instr_t *first_instr = instrlist_first(bb);
  app_pc pc = instr_get_app_pc(first_instr);
  string symbol_name = "UNKNOWN";
  if (sym_tab->find((intptr_t)pc) != sym_tab->end()) {
    symbol_name = (*sym_tab)[(intptr_t)pc];
    //dr_printf("Symbol = %s\n", symbol_name.c_str());
  }

  if (fun_replace_map->count((intptr_t)pc) > 0) {
    // Replace client function with the function supplied by the tool.
    // The logic is inspired by drmemory/replace.c
    app_pc target_fun = (app_pc)(*fun_replace_map)[(intptr_t)pc];
    const module_data_t *info = dr_lookup_module(pc);
    dr_printf("REDIR: %s (from %s) redirected to %p\n",
              symbol_name.c_str(), info->full_path, target_fun);

    instrlist_clear(drcontext, bb);
    instrlist_append(bb, INSTR_XL8(INSTR_CREATE_jmp(drcontext, opnd_create_pc(target_fun)), pc));
  } else {
    if (StringMatch("*foo_to_wrap*", symbol_name)) {
      const module_data_t *info = dr_lookup_module(pc);
      dr_printf(" 'foo_to_wrap' entry point: bb %p, %s / %s\n", pc, dr_module_preferred_name(info), info->full_path);
      wrap::orig_foo = (int (*)(int,int,int))(void*)pc;

      //print_bb(drcontext, bb, "BEFORE");
      // TODO: Use something more optimized than clean_call
      dr_insert_clean_call(drcontext, bb, first_instr, (void*)wrap::is_in_wrapper,
                           false, 1, OPND_CREATE_INTPTR(pc));
      instr_t *opr_instr = INSTR_CREATE_test(drcontext, opnd_create_reg(REG_XAX),
                                                        opnd_create_reg(REG_XAX));
      instr_t *jne_instr = INSTR_CREATE_jcc(drcontext, OP_jz,
                                            opnd_create_pc((app_pc)wrap::wrapped_foo));
      instrlist_meta_preinsert(bb, first_instr, opr_instr);
      instrlist_meta_preinsert(bb, first_instr, jne_instr);

      //print_bb(drcontext, bb, "AFTER");
    }

    instr_t *instr, *next_instr;
    for (instr = instrlist_first(bb); instr != NULL; instr = next_instr) {
      next_instr = instr_get_next(instr);
      if (instr_get_app_pc(instr))  // don't instrument non-app code
        InstrumentInstruction(drcontext, bb, instr);
    }


    OnEvent_Trace(drcontext, tag, bb, translating);
  }

  return DR_EMIT_DEFAULT;
}

void ReadSymbolsTableFromFile(const char *filename) {
  file_t f = dr_open_file(filename, DR_FILE_READ);
  CHECK(f != INVALID_FILE);

  const int BUFF_SIZE = 1 << 16;  // should be enough for testing
  char buff[BUFF_SIZE];
  dr_read_file(f, buff, BUFF_SIZE);
  char *cur_line = buff;
  while (*cur_line) {
    char *next_line = strstr(cur_line, "\n");
    if (next_line != NULL)
      *next_line = 0;
    char fun_name[1024];
    char dummy;
    void* pc;
    sscanf(cur_line, "%p %c %s", &pc, &dummy, fun_name);
    //dr_printf("%s => %p\n", fun_name, pc);
    (*sym_tab)[(intptr_t)pc] = fun_name;

    if (next_line == NULL) break;
    cur_line = next_line + 1;
  }

}

void ReplaceFunc3(void *img, void *rtn, string filter, void *fun_ptr) {
  for (SymbolsTable::iterator i = sym_tab->begin(); i != sym_tab->end(); i++) {
    if (StringMatch(filter, i->second))
      (*fun_replace_map)[(intptr_t)i->first] = fun_ptr;
  }
}

//--------------- dr_init ----------------- {{{1
DR_EXPORT void dr_init(client_id_t id) {
  sym_tab = new SymbolsTable;

  // HACK doesn't work if multiple options are passed.
  const char *opstr = dr_get_options(id);
  dr_printf("Options: %s\n", opstr);
  const char *fname = strstr(opstr, "--symbols=");
  if (fname) {
    ReadSymbolsTableFromFile(fname + 10);
  }

  // Register events.
  dr_register_exit_event(OnEvent_Exit);
  dr_register_bb_event(OnEvent_BB);
  dr_register_trace_event(OnEvent_Trace);
  dr_register_thread_init_event(OnEvent_ThreadInit);
  dr_register_thread_exit_event(OnEvent_ThreadExit);
  dr_register_module_load_event(OnEvent_ModuleLoaded);
  g_lock = dr_mutex_create();

  fun_replace_map = new FunctionsReplaceMap();
  void *img = NULL, *rtn = NULL;
  #define AFUNPTR void*
  ReplaceFunc3(img, rtn, "memchr", (AFUNPTR)Replace_memchr);
  ReplaceFunc3(img, rtn, "strchr", (AFUNPTR)Replace_strchr);
  ReplaceFunc3(img, rtn, "index", (AFUNPTR)Replace_strchr);
  ReplaceFunc3(img, rtn, "strchrnul", (AFUNPTR)Replace_strchrnul);
  ReplaceFunc3(img, rtn, "strrchr", (AFUNPTR)Replace_strrchr);
  ReplaceFunc3(img, rtn, "rindex", (AFUNPTR)Replace_strrchr);
  ReplaceFunc3(img, rtn, "strlen", (AFUNPTR)Replace_strlen);
  ReplaceFunc3(img, rtn, "memcpy", (AFUNPTR)Replace_memcpy);
  ReplaceFunc3(img, rtn, "memmove", (AFUNPTR)Replace_memmove);
  ReplaceFunc3(img, rtn, "memcmp", (AFUNPTR)Replace_memcmp);
  ReplaceFunc3(img, rtn, "strcpy", (AFUNPTR)Replace_strcpy);
  ReplaceFunc3(img, rtn, "stpcpy", (AFUNPTR)Replace_stpcpy);
  ReplaceFunc3(img, rtn, "strncpy", (AFUNPTR)Replace_strncpy);
  ReplaceFunc3(img, rtn, "strcmp", (AFUNPTR)Replace_strcmp);
  ReplaceFunc3(img, rtn, "strncmp", (AFUNPTR)Replace_strncmp);
  ReplaceFunc3(img, rtn, "strcat", (AFUNPTR)Replace_strcat);
  ReplaceFunc3(img, rtn, "*foo_to_replace*", (AFUNPTR)replace_foo);
}
// end. {{{1
// vim:shiftwidth=2:softtabstop=2:expandtab
