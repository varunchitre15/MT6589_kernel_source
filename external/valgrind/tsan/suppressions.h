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

// This file contains the parser and matcher for valgrind-compatible
// suppressions. It supports extended suppression syntax, see details at
// http://code.google.com/p/data-race-test/wiki/ThreadSanitizerSuppressions

#ifndef TSAN_SUPPRESSIONS_H_
#define TSAN_SUPPRESSIONS_H_

#include "common_util.h"

class Suppressions {
 public:
  Suppressions();
  ~Suppressions();

  // Read suppressions file from string. May be called several times.
  // Return the number of parsed suppressions or -1 if an error occured.
  int ReadFromString(const string &str);

  // Returns the string describing the last error. Undefined if there was no
  // error.
  string GetErrorString();

  // Returns the line number of the last error. Undefined if there was no error.
  int GetErrorLineNo();

  // Checks if a given stack trace is suppressed.
  bool StackTraceSuppressed(string tool_name, string warning_name,
      const vector<string>& function_names_mangled,
      const vector<string>& function_names_demangled,
      const vector<string>& object_names,
      string *name_of_suppression);

 private:
  struct SuppressionsRep;
  SuppressionsRep* rep_;
};

#endif  // TSAN_SUPPRESSIONS_H_
