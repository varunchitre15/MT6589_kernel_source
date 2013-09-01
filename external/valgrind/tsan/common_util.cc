/* Copyright (c) 2010-2011, Google Inc.
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

#include "common_util.h"

bool StringMatch(const string& wildcard, const string& text) {
  const char* c_text = text.c_str();
  const char* c_wildcard = wildcard.c_str();
  // Start of the current look-ahead. Everything before these positions is a
  // definite, optimal match.
  const char* c_text_last = NULL;
  const char* c_wildcard_last = NULL;

  char last_wc_char = wildcard[wildcard.size() - 1];

  if (last_wc_char == '*' && wildcard.size() == 1) {
    return true;  // '*' matches everything.
  }

  if (last_wc_char != '*' && last_wc_char != '?'
      && last_wc_char != text[text.size() - 1]) {
    // short cut for the case when the wildcard does not end with '*' or '?'
    // and the last characters of wildcard and text do not match.
    return false;
  }

  while (*c_text) {
    if (*c_wildcard == '*') {
      while (*++c_wildcard == '*') {
        // Skip all '*'.
      }
      if (!*c_wildcard) {
        // Ends with a series of '*'.
        return true;
      }
      c_text_last = c_text;
      c_wildcard_last = c_wildcard;
    } else if ((*c_text == *c_wildcard) || (*c_wildcard == '?')) {
      ++c_text;
      ++c_wildcard;
    } else if (c_text_last) {
      // No match. But we have seen at least one '*', so rollback and try at the
      // next position.
      c_wildcard = c_wildcard_last;
      c_text = c_text_last++;
    } else {
      return false;
    }
  }

  // Skip all '*' at the end of the wildcard.
  while (*c_wildcard == '*') {
    ++c_wildcard;
  }

  return !*c_wildcard;
}

string ConvertToPlatformIndependentPath(const string &s) {
  string ret = s;
#ifdef _MSC_VER
  // TODO(timurrrr): do we need anything apart from s/\\///g?
  size_t it = 0;
  while ((it = ret.find("\\", it)) != string::npos) {
    ret.replace(it, 1, "/");
  }
#endif // _MSC_VER
  return ret;
}

TS_FILE OpenFileReadOnly(const string &file_name, bool die_if_failed) {
  TS_FILE ret = TS_FILE_INVALID;
#ifdef TS_VALGRIND
  SysRes sres = VG_(open)((const Char*)file_name.c_str(), VKI_O_RDONLY, 0);
  if (!sr_isError(sres))
    ret = sr_Res(sres);
#elif defined(_MSC_VER)
  ret = fopen(file_name.c_str(), "r");
#else // no TS_VALGRIND
  ret = open(file_name.c_str(), O_RDONLY);
#endif
  if (ret == TS_FILE_INVALID && die_if_failed) {
    Printf("ERROR: can not open file %s\n", file_name.c_str());
    exit(1);
  }
  return ret;
}

// Read the contents of a file to string. Valgrind version.
string ReadFileToString(const string &file_name, bool die_if_failed) {
  TS_FILE fd = OpenFileReadOnly(file_name, die_if_failed);
  if (fd == TS_FILE_INVALID) {
    return string();
  }
  char buff[257] = {0};
  int n_read;
  string res;
  while ((n_read = read(fd, buff, sizeof(buff) - 1)) > 0) {
    buff[n_read] = 0;
    res.append(buff, n_read);
  }
  close(fd);
  return res;
}
