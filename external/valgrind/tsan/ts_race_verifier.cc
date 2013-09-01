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
// Author: Evgeniy Stepanov.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <set>
#include <iterator>
#include <algorithm>

#include "ts_lock.h"
#include "ts_util.h"
#include "ts_race_verifier.h"
#include "thread_sanitizer.h"

struct PossibleRace {
  PossibleRace() : pc(0), reported(false) {}
  // racy instruction
  uintptr_t pc;
  // concurrent traces
  vector<uintptr_t> traces;
  // report text
  string report;
  // whether this race has already been reported
  bool reported;
};

// pc -> race
static map<uintptr_t, PossibleRace*>* races_map;

// Data about a call site.
struct CallSite {
  int thread_id;
  uintptr_t pc;
};

struct TypedCallSites {
  vector<CallSite> reads;
  vector<CallSite> writes;
};

// data address -> ([write callsites], [read callsites])
typedef map<uintptr_t, TypedCallSites> AddressMap;

static TSLock racecheck_lock;
static AddressMap* racecheck_map;
// data addresses that are ignored (they have already been reported)
static set<uintptr_t>* ignore_addresses;

// starting pc of the trace -> visit count
// used to reduce the sleep time for hot traces
typedef map<uintptr_t, int> VisitCountMap;
static VisitCountMap* visit_count_map;

static int n_reports;

/**
 * Given max and min pc of a trace (both inclusive!), returns whether this trace
 * is interesting to us at all (via the return value), and whether it should be
 * instrumented fully (*instrument_pc=0), or 1 instruction only. In the latter
 * case, *instrument_pc contains the address of the said instruction.
 */
bool RaceVerifierGetAddresses(uintptr_t min_pc, uintptr_t max_pc,
    uintptr_t* instrument_pc) {
  uintptr_t pc = 0;
  for (map<uintptr_t, PossibleRace*>::iterator it = races_map->begin();
       it != races_map->end(); ++it) {
    PossibleRace* race = it->second;
    if (race->reported)
      continue;
    if (race->pc >= min_pc && race->pc <= max_pc) {
      if (pc) {
        // Two race candidates in one trace. Just instrument it fully.
        *instrument_pc = 0;
        return true;
      }
      pc = race->pc;
    }
    for (vector<uintptr_t>::iterator it2 = race->traces.begin();
         it2 != race->traces.end(); ++it2) {
      if (*it2 >= min_pc && *it2 <= max_pc) {
        *instrument_pc = 0;
        return true;
      }
    }
  }
  *instrument_pc = pc;
  return !!pc;
}

static void UpdateSummary() {
  if (!G_flags->summary_file.empty()) {
    char buff[100];
    snprintf(buff, sizeof(buff),
	     "RaceVerifier: %d report(s) verified\n", n_reports);
    // We overwrite the contents of this file with the new summary.
    // We don't do that at the end because even if we crash later
    // we will already have the summary.
    OpenFileWriteStringAndClose(G_flags->summary_file, buff);
  }
}

/* Build and print a race report for a data address. Does not print stack traces
   and symbols and all the fancy stuff - we don't have that info. Used when we
   don't have a ready report - for unexpected races and for
   --race-verifier-extra races.

   racecheck_lock must be held by the current thread.
*/
static void PrintRaceReportEmpty(uintptr_t addr) {
  TypedCallSites* typedCallSites = &(*racecheck_map)[addr];
  vector<CallSite>& writes = typedCallSites->writes;
  vector<CallSite>& reads = typedCallSites->reads;
  for (vector<CallSite>::const_iterator it = writes.begin();
       it != writes.end(); ++ it) {
    Printf("  write at %p\n", it->pc);
  }
  for (vector<CallSite>::const_iterator it = reads.begin();
       it != reads.end(); ++ it) {
    Printf("  read at %p\n", it->pc);
  }
}

/* Find a PossibleRace that matches current accesses (racecheck_map) to the
   given data address.

   racecheck_lock must be held by the current thread.
 */
static PossibleRace* FindRaceForAddr(uintptr_t addr) {
  TypedCallSites* typedCallSites = &(*racecheck_map)[addr];
  vector<CallSite>& writes = typedCallSites->writes;
  vector<CallSite>& reads = typedCallSites->reads;
  for (vector<CallSite>::const_iterator it = writes.begin();
       it != writes.end(); ++ it) {
    map<uintptr_t, PossibleRace*>::iterator it2 = races_map->find(it->pc);
    if (it2 != races_map->end())
      return it2->second;
  }
  for (vector<CallSite>::const_iterator it = reads.begin();
       it != reads.end(); ++ it) {
    map<uintptr_t, PossibleRace*>::iterator it2 = races_map->find(it->pc);
    if (it2 != races_map->end())
      return it2->second;
  }
  return NULL;
}

/* Prints a race report for the given data address, either finding one in a
   matching PossibleRace, or just printing pc's of the mops.

   racecheck_lock must be held by the current thread.
*/
static void PrintRaceReport(uintptr_t addr) {
  PossibleRace* race = FindRaceForAddr(addr);
  if (race) {
    ExpectedRace* expected_race = ThreadSanitizerFindExpectedRace(addr);
    if (expected_race)
      expected_race->count++;
    bool is_expected = !!expected_race;
    bool is_unverifiable = is_expected && !expected_race->is_verifiable;

    if (is_expected && !is_unverifiable && !G_flags->show_expected_races)
      return;

    if (is_unverifiable)
      Printf("WARNING: Confirmed a race that was marked as UNVERIFIABLE:\n");
    else
      Printf("WARNING: Confirmed a race:\n");
    const string& report = race->report;
    if (report.empty()) {
      PrintRaceReportEmpty(addr);
    } else {
      Printf("%s", report.c_str());
    }
    // Suppress future reports for this race.
    race->reported = true;
    ignore_addresses->insert(addr);

    n_reports++;
  } else {
    Printf("Warning: unexpected race found!\n");
    PrintRaceReportEmpty(addr);

    n_reports ++;
  }
  UpdateSummary();
}

/**
 * This function is called before the mop delay.
 * @param thread_id Thread id.
 * @param addr Data address.
 * @param pc Instruction pc.
 * @param is_w Whether this is a write (true) or a read (false).
 * @return True if this access is interesting to us at all. If true, the caller
 *     should delay and then call RaceVerifierEndAccess. If false, it should do
 *     nothing more for this mop.
 */
bool RaceVerifierStartAccess(int thread_id, uintptr_t addr, uintptr_t pc,
    bool is_w) {
  CallSite callSite;
  callSite.thread_id = thread_id;
  callSite.pc = pc;
  racecheck_lock.Lock();

  if (debug_race_verifier)
    Printf("[%d] pc %p %s addr %p start\n", thread_id, pc,
        is_w ? "write" : "read", addr);

  if (ignore_addresses->count(addr)) {
    racecheck_lock.Unlock();
    return false;
  }

  TypedCallSites* typedCallSites = &(*racecheck_map)[addr];
  vector<CallSite>& writes = typedCallSites->writes;
  vector<CallSite>& reads = typedCallSites->reads;
  (is_w ? writes : reads).push_back(callSite);
  if (writes.size() > 0 && writes.size() + reads.size() > 1) {
    bool is_race = false;
    for (size_t i = 0; !is_race && i < writes.size(); ++i) {
      for (size_t j = 0; !is_race && j < writes.size(); ++j)
        if (writes[i].thread_id != writes[j].thread_id)
          is_race = true;
      for (size_t j = 0; !is_race && j < reads.size(); ++j)
        if (writes[i].thread_id != reads[j].thread_id)
          is_race = true;
    }
    if (is_race)
      PrintRaceReport(addr);
  }
  racecheck_lock.Unlock();
  return true;
}

/* This function is called after the mop delay, only if RaceVerifierStartAccess
   returned true. The arguments are exactly the same. */
void RaceVerifierEndAccess(int thread_id, uintptr_t addr, uintptr_t pc,
    bool is_w) {
  racecheck_lock.Lock();

  if (debug_race_verifier)
    Printf("[%d] pc %p %s addr %p end\n", thread_id, pc,
        is_w ? "write" : "read", addr);
  if (ignore_addresses->count(addr)) {
    racecheck_lock.Unlock();
    return;
  }

  TypedCallSites* typedCallSites = &(*racecheck_map)[addr];
  vector<CallSite>& vec =
      is_w ? typedCallSites->writes : typedCallSites->reads;
  for (int i = vec.size() - 1; i >= 0; --i) {
    if (vec[i].thread_id == thread_id) {
      vec.erase(vec.begin() + i);
      break;
    }
  }
  racecheck_lock.Unlock();
}

/* Parse a race description that appears in TSan logs after the words
   "Race verifier data: ", not including the said words. It looks like
   "pc,trace[,trace]...", without spaces. */
static PossibleRace* ParseRaceInfo(const string& raceInfo) {
  PossibleRace* race = new PossibleRace();
  const char* p = raceInfo.c_str();
  while (true) {
    char* end;
    uintptr_t addr = my_strtol(p, &end, 16);
    if (p == end) {
      Printf("Parse error: %s\n", p);
      exit(1);
    }
    if (!race->pc)
      race->pc = addr;
    else
      race->traces.push_back(addr);
    while (*end == '\n' || *end == '\r')
      ++end;
    if (*end == '\0') {
      // raceInfo already ends with \n
      Printf("Possible race: %s", raceInfo.c_str());
      return race;
    }
    if (*end != ',') {
      Printf("Parse error: comma expected: %s\n", end);
      delete race;
      return NULL;
    }
    p = end + 1;
  }
}

/* Parse a race description and add it to races_map. */
static void RaceVerifierParseRaceInfo(const string& raceInfo) {
  PossibleRace* race = ParseRaceInfo(raceInfo);
  if (race)
    (*races_map)[race->pc] = race;
  else
    Printf("Bad raceInfo: %s\n", raceInfo.c_str());
}


class StringStream {
 public:
  StringStream(const string &s) : s_(s), data_(s.c_str()), p_(data_) {}

  bool Eof() {
    return !*p_;
  }

  string NextLine() {
    const char* first = p_;
    while (*p_ && *p_ != '\n') {
      ++p_;
    }
    if (*p_)
      ++p_;
    return string(first, p_ - first);
  }

 private:
  const string& s_;
  const char* data_;
  const char* p_;
};

/* Parse a TSan log and add all race verifier info's from it to our storage of
   possible races. */
static void RaceVerifierParseFile(const string& fileName) {
  Printf("Reading race data from %s\n", fileName.c_str());
  const string RACEINFO_MARKER = "Race verifier data: ";
  string log = ReadFileToString(fileName, true /* die_if_failed */);
  StringStream ss(log);
  string* desc = NULL;
  int count = 0;
  while (!ss.Eof()) {
    string line = ss.NextLine();
    size_t pos;
    if ((line.find("WARNING: Possible data race during") !=
            string::npos) ||
        (line.find("WARNING: Expected data race during") !=
            string::npos)) {
      desc = new string();
      (*desc) += line;
    } else if ((pos = line.find(RACEINFO_MARKER)) != string::npos) {
      pos += RACEINFO_MARKER.size();
      string raceInfo = line.substr(pos);
      PossibleRace* race = ParseRaceInfo(raceInfo);
      (*desc) += "}}}\n";
      race->report = *desc;
      (*races_map)[race->pc] = race;
      count ++;
      delete desc;
      desc = NULL;
    } else if (desc) {
      (*desc) += line;
    }
  }
  Printf("Got %d possible races\n", count);
}

/**
 * Return the time to sleep for the given trace.
 * @param trace_pc The starting pc of the trace.
 * @return Time to sleep in ms, or 0 if this trace should be ignored.
 */
int RaceVerifierGetSleepTime(uintptr_t trace_pc) {
  racecheck_lock.Lock();
  int visit_count = ++(*visit_count_map)[trace_pc];
  int tm;
  if (visit_count < 20) {
    tm = G_flags->race_verifier_sleep_ms;
  } else if (visit_count < 200) {
    tm = G_flags->race_verifier_sleep_ms / 10;
  } else {
    tm = 0;
  }
  if (debug_race_verifier) {
    if (visit_count == 20) {
      Printf("RaceVerifier: Trace %x: sleep time reduced.\n", trace_pc);
    } else if (visit_count == 200) {
      Printf("RaceVerifier: Trace %x: ignored.\n", trace_pc);
    }
  }
  racecheck_lock.Unlock();
  return tm;
}

/**
 * Init the race verifier. Should be called exactly once before any other
 * functions in this file.
 * @param fileNames Names of TSan log to parse.
 * @param raceInfos Additional race description strings.
 */
void RaceVerifierInit(const vector<string>& fileNames,
    const vector<string>& raceInfos) {
  races_map = new map<uintptr_t, PossibleRace*>();
  racecheck_map = new AddressMap();
  visit_count_map = new VisitCountMap();
  ignore_addresses = new set<uintptr_t>();

  for (vector<string>::const_iterator it = fileNames.begin();
       it != fileNames.end(); ++it) {
    RaceVerifierParseFile(*it);
  }
  for (vector<string>::const_iterator it = raceInfos.begin();
       it != raceInfos.end(); ++it) {
    RaceVerifierParseRaceInfo(*it);
  }
}

void RaceVerifierFini() {
  Report("RaceVerifier summary: verified %d race(s)\n", n_reports);
  int n_errors = GetNumberOfFoundErrors();
  SetNumberOfFoundErrors(n_errors + n_reports);
}
