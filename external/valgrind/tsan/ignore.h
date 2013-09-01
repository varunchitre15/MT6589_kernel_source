#ifndef TSAN_IGNORE_H__
#define TSAN_IGNORE_H__

#include "common_util.h"

// A triple of patterns to ignore a function, an object file and a source file
// by their names.
struct IgnoreTriple {
  string fun;
  string obj;
  string file;

  IgnoreTriple(string ifun, string iobj, string ifile) : fun(ifun) {
    obj = ConvertToPlatformIndependentPath(iobj);
    file = ConvertToPlatformIndependentPath(ifile);
    CHECK(!((ifun == "*") && (iobj == "*") && (ifile == "*")));
  }
};

struct IgnoreObj : public IgnoreTriple {
  IgnoreObj(string obj) : IgnoreTriple("*", obj, "*") {}
};

struct IgnoreFun : public IgnoreTriple {
  IgnoreFun(string fun) : IgnoreTriple(fun, "*", "*") {}
};

struct IgnoreFile : public IgnoreTriple {
  IgnoreFile(string file) : IgnoreTriple("*", "*", file) {}
};

struct IgnoreLists {
  vector<IgnoreTriple> ignores;
  vector<IgnoreTriple> ignores_r;
  vector<IgnoreTriple> ignores_hist;
};

extern IgnoreLists *g_ignore_lists;
extern vector<string> *g_ignore_obj;

extern IgnoreLists *g_white_lists;

void ReadIgnoresFromString(const string& ignoreString,
    IgnoreLists* ignoreLists);

bool TripleVectorMatchKnown(const vector<IgnoreTriple>& v,
    const string& fun,
    const string& obj,
    const string& file);

bool StringVectorMatch(const vector<string>& v, const string& obj);

#endif
