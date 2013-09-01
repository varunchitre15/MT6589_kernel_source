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

/*
  RaceVerifier is a tool for verifying race reports produced by ThreadSanitizer.
  It works by adding time delays after potentially racey instructions and making
  sure that they are executed simultaneously.

  To use RaceVerifier, save the stderr log of a ThreadSanitizer run to a file
  and run tsan again with --race-verifier=<log file name> option.
 */
#ifndef TS_RACE_VERIFIER_H_
#define TS_RACE_VERIFIER_H_

bool RaceVerifierGetAddresses(uintptr_t min_pc, uintptr_t max_pc,
    uintptr_t* instrument_pc);
bool RaceVerifierStartAccess(int thread_id, uintptr_t addr, uintptr_t pc,
    bool is_w);
void RaceVerifierEndAccess(int thread_id, uintptr_t addr, uintptr_t pc,
    bool is_w);
int RaceVerifierGetSleepTime(uintptr_t trace_pc);

void RaceVerifierInit(const std::vector<std::string>& fileNames,
    const std::vector<std::string>& raceInfos);
void RaceVerifierFini();

#endif // TS_RACE_VERIFIER_H_
