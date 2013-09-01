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

// Experimental off-line race detector.
// Reads program events from a file and detects races.
// See http://code.google.com/p/data-race-test

// ------------- Includes ------------- {{{1
#include "thread_sanitizer.h"
#include "ts_events.h"

#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>

// ------------- Globals ------------- {{{1
static map<string, int> *g_event_type_map;
struct PcInfo {
  string img_name;
  string file_name;
  string rtn_name;
  int line;
};

static map<uintptr_t, PcInfo> *g_pc_info_map;

unsigned long offline_line_n;
//------------- Read binary file Utils ------------ {{{1
static const int kBufSize = 65536;

template<typename T>
static bool Read(FILE *fp, T *res) {
  unsigned char buf[16];
  int size = fread(buf, sizeof(T), 1, fp);
  *res = 0;
  for (unsigned int i=0; i<sizeof(T); i++) {
    *res <<= 8;
    *res += buf[i];
  }
  return size == 1;
}


static bool ReadANSI(FILE *file, string *res) {
  char buf[kBufSize];
  unsigned short length;
  if (!Read<unsigned short>(file, &length)) {
    return false;
  }
  int size = fread(buf, 1, (int)length, file);
  buf[length] = 0;
  *res = (char *)buf;
  return size == length;
}
//------------- Utils ------------------- {{{1
static EventType EventNameToEventType(const char *name) {
  map<string, int>::iterator it = g_event_type_map->find(name);
  if (it == g_event_type_map->end()) {
    Printf("Unknown event type: %s\n", name);
  }
  CHECK(it != g_event_type_map->end());
  return (EventType)it->second;
}

static void InitEventTypeMap() {
  g_event_type_map = new map<string, int>;
  for (int i = 0; i < LAST_EVENT; i++) {
    (*g_event_type_map)[kEventNames[i]] = i;
  }
}

static void SkipCommentText(FILE *file) {
  char buff[kBufSize];
  int i = 0;
  while (true) {
    int c = fgetc(file);
    if (c == EOF) break;
    if (c == '\n') {
      offline_line_n++;
      break;
    }
    if (i < kBufSize - 1)
      buff[i++] = c;
  }
  buff[i] = 0;
  if (buff[0] == 'P' && buff[1] == 'C') {
    char img[kBufSize];
    char rtn[kBufSize];
    char file[kBufSize];
    int line = 0;
    unsigned long pc = 0;
    if (sscanf(buff, "PC %lx %s %s %s %d", (unsigned long*)&pc,
               img, rtn, file, &line) == 5 &&
        pc != 0) {
      CHECK(g_pc_info_map);
      PcInfo pc_info;
      pc_info.img_name = img;
      pc_info.rtn_name = rtn;
      pc_info.file_name = file;
      pc_info.line = line;
      (*g_pc_info_map)[pc] = pc_info;
      // Printf("***** PC %lx %s\n", pc, rtn);
    }
  }
  if (buff[0] == '>') {
    // Just print the rest of comment.
    Printf("%s\n", buff + 2);
  }
}

static void SkipWhiteSpaceAndComments(FILE *file) {
  int c = 0;
  while (true) {
    c = fgetc(file);
    if (c == EOF) return;
    if (c == '#' || c == '=') {
      SkipCommentText(file);
      continue;
    }
    if (isspace(c)) continue;
    break;
  }
  ungetc(c, file);
}

typedef bool (*EventReader)(FILE *, Event *);

bool ReadOneStrEventFromFile(FILE *file, Event *event) {
  CHECK(event);
  char name[1024];
  uint32_t tid;
  unsigned long pc, a, info;
  SkipWhiteSpaceAndComments(file);
  offline_line_n++;
  if (5 == fscanf(file, "%s%x%lx%lx%lx", name, &tid, &pc, &a, &info)) {
    event->Init(EventNameToEventType(name), tid, pc, a, info);
    return true;
  }
  return false;
}

bool ProcessCodePosition(FILE *input, int *pc, string *str) {
  bool ok = Read<int>(input, pc);
  ok &= ReadANSI(input, str);
  return ok;
}

bool ProcessMessage(FILE *input, string *str) {
  return ReadANSI(input, str);
}

// Read information about event in format: [[[info] address] pc] tid.
bool ProcessEvent(FILE *input, EventType type, Event *event) {
  bool ok = true;
  unsigned short tid = 0;
  int pc = 0;
  int64_t address = 0;
  unsigned short extra = 0;
  // It's tricky switch without breaks.
  switch (type) {
    case THR_START:
      ok &= Read<unsigned short>(input, &extra);
      // fallthrough.
    case READ:
    case READER_LOCK:
    case SIGNAL:
    case THR_JOIN_AFTER:
    case UNLOCK:
    case WAIT:
    case WRITE:
    case WRITER_LOCK:
      ok &= Read<int64_t>(input, &address);
      // fallthrough.
    case EXPECT_RACE_BEGIN:
    case EXPECT_RACE_END:
    case RTN_EXIT:
    case SBLOCK_ENTER:
    case STACK_TRACE:
    case THR_END:
    case THR_FIRST_INSN:
      ok &= Read<int>(input, &pc);
      // fallthrough.
    case RTN_CALL:
      ok &= Read<unsigned short>(input, &tid);
      break;
    default:
      // read unsupported EventType.
      Printf("Unsupported EventType %s %d\n", type, (int)type);
      CHECK(false);
  }
  if (type == READ || type == WRITE) {
    extra = 1;
  }
  event->Init(type, (int)tid, pc, address, (int)extra);
  return ok;
}

bool ReadOneBinEventFromFile(FILE *input, Event *event) {
  CHECK(event);
  bool ok = true;
  EventType type;
  unsigned char typeOrd;
  int pc;
  int line;
  char rtn[kBufSize];
  char file[kBufSize];
  string str;
  while (ok) {
    offline_line_n++;
    ok &= Read<unsigned char>(input, &typeOrd);
    if (!ok) break;
    type = (EventType)typeOrd;
    switch (type) {
      case PC_DESCRIPTION:
        ok &= ProcessCodePosition(input, &pc, &str);
        if (sscanf(str.c_str(), "%s %s %d", rtn, file, &line) == 3 && pc != 0) {
          CHECK(g_pc_info_map);
          PcInfo pc_info;
          pc_info.img_name = "java";
          pc_info.rtn_name = rtn;
          pc_info.file_name = file;
          pc_info.line = line;
          (*g_pc_info_map)[pc] = pc_info;
        }
        break;
      case PRINT_MESSAGE:
        ok &= ProcessMessage(input, &str);
        // Just print the rest of comment.
        Printf("%s\n", str.c_str());
        break;
      default:
        ok &= ProcessEvent(input, type, event);
        return ok;
    }
  }
  return false;
}

void DecodeEventsFromFile(FILE *input, FILE *output) {
  offline_line_n = 0;
  bool ok = true;
  EventType type;
  unsigned char typeOrd;
  int pc;
  string str;
  Event event;
  while (ok) {
    ok &= Read<unsigned char>(input, &typeOrd);
    if (!ok) break;
    type = (EventType)typeOrd;
    switch (type) {
      case PC_DESCRIPTION:
        ok &= ProcessCodePosition(input, &pc, &str);
        fprintf(output, "#PC %x java %s\n", pc, str.c_str());
        break;
      case PRINT_MESSAGE:
        ok &= ProcessMessage(input, &str);
        fprintf(output, "#> %s\n", str.c_str());
        break;
      default:
        ok &= ProcessEvent(input, type, &event);
        fprintf(output, "%s %x %x %lx %lx\n", kEventNames[event.type()],
            event.tid(), (unsigned int)event.pc(),
            (long unsigned int)event.a(), (long unsigned int)event.info());
        break;
    }
    offline_line_n++;
  }
  Printf("INFO: ThreadSanitizer write %ld lines.\n", offline_line_n);
}

static const uint32_t max_unknown_thread = 10000;

static bool known_threads[max_unknown_thread] = {};

INLINE void ReadEventsFromFile(FILE *file, EventReader event_reader_cb) {
  Event event;
  uint64_t n_events = 0;
  offline_line_n = 0;
  while (event_reader_cb(file, &event)) {
    //event.Print();
    n_events++;
    uint32_t tid = event.tid();
    if (event.type() == THR_START && tid < max_unknown_thread) {
      known_threads[tid] = true;
    }
    if (tid >= max_unknown_thread || known_threads[tid]) {
      ThreadSanitizerHandleOneEvent(&event);
    }
  }
  Printf("INFO: ThreadSanitizerOffline: %ld events read\n", n_events);
}
//------------- ThreadSanitizer exports ------------ {{{1

void PcToStrings(uintptr_t pc, bool demangle,
                string *img_name, string *rtn_name,
                string *file_name, int *line_no) {
  if (g_pc_info_map->count(pc) == 0) {
    *img_name = "";
    *rtn_name = "";
    *file_name = "";
    *line_no = 0;
    return;
  }
  PcInfo &info = (*g_pc_info_map)[pc];
  *img_name = info.img_name;
  *rtn_name = info.rtn_name;
  *file_name = info.file_name;
  *line_no = info.line;
  if (*file_name == "unknown")
    *file_name = "";
}

string PcToRtnName(uintptr_t pc, bool demangle) {
  string img, rtn, file;
  int line;
  PcToStrings(pc, demangle, &img, &rtn, &file, &line);
  return rtn;
}
//------------- main ---------------------------- {{{1
int main(int argc, char *argv[]) {
  Printf("INFO: ThreadSanitizerOffline r%s\n", TS_VERSION);

  InitEventTypeMap();
  g_pc_info_map = new map<uintptr_t, PcInfo>;
  G_flags = new FLAGS;

  vector<string> args(argv + 1, argv + argc);
  ThreadSanitizerParseFlags(&args);
  ThreadSanitizerInit();

  CHECK(G_flags);
  if (G_flags->input_type == "bin") {
    ReadEventsFromFile(stdin, ReadOneBinEventFromFile);
  } else if (G_flags->input_type == "decode") {
    FILE* output;
    if (G_flags->log_file.size() > 0) {
      output = fopen(G_flags->log_file.c_str(), "w");
    } else {
      output = stdout;
    }
    DecodeEventsFromFile(stdin, output);
  } else if (G_flags->input_type == "str") {
    ReadEventsFromFile(stdin, ReadOneStrEventFromFile);
  } else {
    Printf("Error: Unknown input_type value %s\n", G_flags->input_type.c_str());
    exit(5);
  }

  ThreadSanitizerFini();
  if (G_flags->error_exitcode && GetNumberOfFoundErrors() > 0) {
    return G_flags->error_exitcode;
  }
}

// end. {{{1
// vim:shiftwidth=2:softtabstop=2:expandtab:tw=80
