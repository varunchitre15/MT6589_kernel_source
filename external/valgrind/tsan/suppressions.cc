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

// This file contains the parser for valgrind-compatible suppressions.

#include "suppressions.h"

// TODO(eugenis): convert checks to warning messages.
// TODO(eugenis): write tests for incorrect syntax.

enum LocationType {
  LT_STAR,  // ...
  LT_OBJ,  // obj:
  LT_FUN,  // fun:
};

struct Location {
  LocationType type;
  string name;
};

struct StackTraceTemplate {
  vector<Location> locations;
};

struct Suppression {
  string name;
  set<string> tools;
  string warning_name;
  // Extra information available for some suppression types.
  // ex.: Memcheck:Param
  string extra;
  vector<StackTraceTemplate> templates;
};

class Parser {
 public:
  explicit Parser(const string &str)
      : buffer_(str), next_(buffer_.c_str()),
        end_(buffer_.c_str() + buffer_.size()), line_no_(0), error_(false) {}

  bool NextSuppression(Suppression* suppression);
  bool GetError();
  string GetErrorString();
  int GetLineNo();

 private:
  bool Eof() { return next_ >= end_; }
  string NextLine();
  string NextLineSkipComments();
  void PutBackSkipComments(string line);
  bool ParseSuppressionToolsLine(Suppression* supp, string line);
  bool IsExtraLine(string line);
  bool ParseStackTraceLine(StackTraceTemplate* trace, string line);
  bool NextStackTraceTemplate(StackTraceTemplate* trace, bool* last);

  void SetError(string desc);

  const string& buffer_;
  const char* next_;
  const char* end_;
  stack<string> put_back_stack_;

  int line_no_;
  bool error_;
  string error_string_;
};

#define PARSER_CHECK(cond, desc) do {\
    if (!(cond)) {\
      SetError(desc);\
      return false;\
    }} while ((void)0, 0)

void Parser::SetError(string desc) {
  error_ = true;
  error_string_ = desc;
}

bool Parser::GetError() {
  return error_;
}

string Parser::GetErrorString() {
  return error_string_;
}

int Parser::GetLineNo() {
  return line_no_;
}

string Parser::NextLine() {
  const char* first = next_;
  while (!Eof() && *next_ != '\n') {
    ++next_;
  }
  string line(first, next_ - first);
  if (*next_ == '\n') {
    ++next_;
  }
  ++line_no_;
  return line;
}

string Parser::NextLineSkipComments() {
  string line;
  if (!put_back_stack_.empty()) {
    line = put_back_stack_.top();
    put_back_stack_.pop();
    return line;
  }
  while (!Eof()) {
    line = NextLine();
    // Skip empty lines.
    if (line.empty())
      continue;
    // Skip comments.
    if (line[0] == '#')
      continue;
    const char* p = line.c_str();
    const char* e = p + line.size();
    // Strip whitespace.
    while (p < e && (*p == ' ' || *p == '\t'))
      ++p;
    if (p >= e)
      continue;
    const char* last = e - 1;
    while (last > p && (*last == ' ' || *last == '\t'))
      --last;
    return string(p, last - p + 1);
  }
  return "";
}

void Parser::PutBackSkipComments(string line) {
  put_back_stack_.push(line);
}

bool Parser::ParseSuppressionToolsLine(Suppression* supp, string line) {
  size_t idx = line.find(':');
  PARSER_CHECK(idx != string::npos, "expected ':' in tools line");
  string s1 = line.substr(0, idx);
  string s2 = line.substr(idx + 1);
  PARSER_CHECK(!s1.empty(), "expected non-empty tool(s) name");
  PARSER_CHECK(!s2.empty(), "expected non-empty warning name");
  size_t idx2;
  while ((idx2 = s1.find(',')) != string::npos) {
    supp->tools.insert(s1.substr(0, idx2));
    s1.erase(0, idx2 + 1);
  }
  supp->tools.insert(s1);
  supp->warning_name = s2;
  return true;
}

bool Parser::ParseStackTraceLine(StackTraceTemplate* trace, string line) {
  if (line == "...") {
    Location location = {LT_STAR, ""};
    trace->locations.push_back(location);
    return true;
  } else {
    size_t idx = line.find(':');
    PARSER_CHECK(idx != string::npos, "expected ':' in stack trace line");
    string s1 = line.substr(0, idx);
    string s2 = line.substr(idx + 1);
    if (s1 == "obj") {
      Location location = {LT_OBJ, s2};
      trace->locations.push_back(location);
      return true;
    } else if (s1 == "fun") {
      Location location = {LT_FUN, s2};
      // A suppression frame can only have ( or ) if it comes from Objective-C,
      // i.e. starts with +[ or -[ or =[
      PARSER_CHECK(s2.find_first_of("()") == string::npos ||
                   (s2[1] == '[' && strchr("+-=", s2[0]) != NULL),
                   "'fun:' lines can't contain '()'");

      // Check that we don't have template arguments in the suppression.
      {
        // Caveat: don't be confused by "operator>>" and similar...
        size_t checked_till = 0;
        // List of possible >>-like operators, sorted by the operation length.
        const char *OP[] = {">>=", "<<=",
                            ">>", "<<",
                            ">=", "<=",
                            "->", "->*",
                            "<", ">"};
        bool check_failed = false;
        while (!check_failed && checked_till < s2.size()) {
          size_t next = s2.find_first_of("<>", checked_till);
          if (next == string::npos)
            break;

          if (next < 8) {
            // operatorX won't fit
            check_failed = true;
            break;
          }

          for (size_t i = 0; i < TS_ARRAY_SIZE(OP); i++) {
            size_t op_offset = ((string)OP[i]).find(s2[next]);
            if (op_offset == string::npos)
              continue;
            if (next >= 8 + op_offset &&
                "operator" == s2.substr(next- (8 + op_offset), 8) &&
                OP[i] == s2.substr(next- op_offset, strlen(OP[i]))) {
              checked_till = next + strlen(OP[i] + op_offset);
              break;
            }
          }
        }

        PARSER_CHECK(!check_failed, "'fun:' lines can't contain '<' or '>' "
                     "except for operators");
      }

      trace->locations.push_back(location);
      return true;
    } else {
      SetError("bad stack trace line");
      return false;
    }
  }
}

// Checks if this line can not be parsed by Parser::NextStackTraceTemplate
// and, therefore, is an extra information for the suppression.
bool Parser::IsExtraLine(string line) {
  if (line == "..." || line == "{" || line == "}")
    return false;
  if (line.size() < 4)
    return true;
  string prefix = line.substr(0, 4);
  return !(prefix == "obj:" || prefix == "fun:");
}

bool Parser::NextStackTraceTemplate(StackTraceTemplate* trace,
    bool* last_stack_trace) {
  string line = NextLineSkipComments();
  if (line == "}") {  // No more stack traces in multi-trace syntax
    *last_stack_trace = true;
    return false;
  }

  if (line == "{") {  // A multi-trace syntax
    line = NextLineSkipComments();
  } else {
    *last_stack_trace = true;
  }

  while (true) {
    if (!ParseStackTraceLine(trace, line))
      return false;
    line = NextLineSkipComments();
    if (line == "}")
      break;
  }
  return true;
}

bool Parser::NextSuppression(Suppression* supp) {
  string line;
  line = NextLineSkipComments();
  if (line.empty())
    return false;
  // Opening {
  PARSER_CHECK(line == "{", "expected '{'");
  // Suppression name.
  line = NextLineSkipComments();
  PARSER_CHECK(!line.empty(), "expected suppression name");
  supp->name = line;
  // tool[,tool]:warning_name.
  line = NextLineSkipComments();
  PARSER_CHECK(!line.empty(), "expected tool[, tool]:warning_name line");
  if (!ParseSuppressionToolsLine(supp, line))
    return false;
  if (0) {  // Not used currently. May still be needed later.
    // A possible extra line.
    line = NextLineSkipComments();
    if (IsExtraLine(line))
      supp->extra = line;
    else
      PutBackSkipComments(line);
  }
  // Everything else.
  bool done = false;
  while (!done) {
    StackTraceTemplate trace;
    if (NextStackTraceTemplate(&trace, &done))
      supp->templates.push_back(trace);
    if (error_)
      return false;
  }
  // TODO(eugenis): Do we need to check for empty traces?
  return true;
}

struct Suppressions::SuppressionsRep {
  vector<Suppression> suppressions;
  string error_string_;
  int error_line_no_;
};

Suppressions::Suppressions() : rep_(new SuppressionsRep) {}

Suppressions::~Suppressions() {
  delete rep_;
}

int Suppressions::ReadFromString(const string &str) {
  int sizeBefore = rep_->suppressions.size();
  Parser parser(str);
  Suppression supp;
  while (parser.NextSuppression(&supp)) {
    rep_->suppressions.push_back(supp);
  }
  if (parser.GetError()) {
    rep_->error_string_ = parser.GetErrorString();
    rep_->error_line_no_ = parser.GetLineNo();
    return -1;
  }
  return rep_->suppressions.size() - sizeBefore;
}

string Suppressions::GetErrorString() {
  return rep_->error_string_;
}

int Suppressions::GetErrorLineNo() {
  return rep_->error_line_no_;
}

struct MatcherContext {
  MatcherContext(
      const vector<string>& function_names_mangled_,
      const vector<string>& function_names_demangled_,
      const vector<string>& object_names_) :
      function_names_mangled(function_names_mangled_),
      function_names_demangled(function_names_demangled_),
      object_names(object_names_),
      tmpl(NULL)
  {}

  const vector<string>& function_names_mangled;
  const vector<string>& function_names_demangled;
  const vector<string>& object_names;
  StackTraceTemplate* tmpl;
};

static bool MatchStackTraceRecursive(MatcherContext ctx, int trace_index,
    int tmpl_index) {
  const int trace_size = ctx.function_names_mangled.size();
  const int tmpl_size = ctx.tmpl->locations.size();
  while (trace_index < trace_size && tmpl_index < tmpl_size) {
    Location& location = ctx.tmpl->locations[tmpl_index];
    if (location.type == LT_STAR) {
      ++tmpl_index;
      while (trace_index < trace_size) {
        if (MatchStackTraceRecursive(ctx, trace_index++, tmpl_index))
          return true;
      }
      return false;
    } else {
      bool match = false;
      if (location.type == LT_OBJ) {
        match = StringMatch(location.name, ctx.object_names[trace_index]);
      } else {
        CHECK(location.type == LT_FUN);
        match =
          StringMatch(location.name, ctx.function_names_mangled[trace_index]) ||
          StringMatch(location.name, ctx.function_names_demangled[trace_index]);
      }
      if (match) {
        ++trace_index;
        ++tmpl_index;
      } else {
        return false;
      }
    }
  }
  return tmpl_index == tmpl_size;
}

bool Suppressions::StackTraceSuppressed(string tool_name, string warning_name,
    const vector<string>& function_names_mangled,
    const vector<string>& function_names_demangled,
    const vector<string>& object_names,
    string *name_of_suppression) {
  MatcherContext ctx(function_names_mangled, function_names_demangled,
      object_names);
  for (vector<Suppression>::iterator it = rep_->suppressions.begin();
       it != rep_->suppressions.end(); ++it) {
    if (it->warning_name != warning_name ||
        it->tools.find(tool_name) == it->tools.end())
      continue;
    for (vector<StackTraceTemplate>::iterator it2 = it->templates.begin();
         it2 != it->templates.end(); ++it2) {
      ctx.tmpl = &*it2;
      bool result = MatchStackTraceRecursive(ctx, 0, 0);
      if (result) {
        *name_of_suppression = it->name;
        return true;
      }
    }
  }
  return false;
}
