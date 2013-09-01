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

// This file contains tests for suppressions implementation.

#include <gtest/gtest.h>

#include "suppressions.h"

#define VEC(arr) *(new vector<string>(arr, arr + sizeof(arr) / sizeof(*arr)))

class BaseSuppressionsTest : public ::testing::Test {
 protected:
  bool IsSuppressed(string tool, string warning_type, const vector<string>& f_m,
      const vector<string>& f_d, const vector<string>& o) {
    string result;
    return supp_.StackTraceSuppressed(
        tool, warning_type, f_m, f_d, o, &result);
  }

  bool IsSuppressed(const vector<string>& f_m, const vector<string>& f_d,
      const vector<string>& o) {
    return IsSuppressed("test_tool", "test_warning_type", f_m, f_d, o);
  }

  Suppressions supp_;
};

class SuppressionsTest : public BaseSuppressionsTest {
 protected:
  virtual void SetUp() {
    const string data =
        "{\n"
        "  name\n"
        "  test_tool,tool2:test_warning_type\n"
        "  fun:function1\n"
        "  obj:object1\n"
        "  fun:function2\n"
        "}";
    supp_.ReadFromString(data);
  }
};


TEST_F(SuppressionsTest, Simple) {
  string m[] = {"aa", "bb", "cc"};
  string d[] = {"aaa", "bbb", "ccc"};
  string o[] = {"object1", "object2", "object3"};
  ASSERT_FALSE(IsSuppressed(VEC(m), VEC(d), VEC(o)));
}

TEST_F(SuppressionsTest, Simple2) {
  string m[] = {"function1", "bb", "function2"};
  string d[] = {"aaa", "bbb", "ccc"};
  string o[] = {"object2", "object1", "object3"};
  ASSERT_TRUE(IsSuppressed(VEC(m), VEC(d), VEC(o)));
}

// A long stack trace is ok.
TEST_F(SuppressionsTest, LongTrace) {
  string m[] = {"function1", "bb", "function2", "zz"};
  string d[] = {"aaa", "bbb", "ccc", "zzz"};
  string o[] = {"object2", "object1", "object3", "o4"};
  ASSERT_TRUE(IsSuppressed(VEC(m), VEC(d), VEC(o)));
}

// A stack trace template only matches at the top of the stack.
TEST_F(SuppressionsTest, OnlyMatchesAtTheTop) {
  string m[] = {"zz", "function1", "bb", "function2"};
  string d[] = {"zzz", "aaa", "bbb", "ccc"};
  string o[] = {"o0", "object2", "object1", "object3"};
  ASSERT_FALSE(IsSuppressed(VEC(m), VEC(d), VEC(o)));
}

// A short stack trace is not.
TEST_F(SuppressionsTest, ShortTrace) {
  string m[] = {"function1", "bb"};
  string d[] = {"aaa", "bbb"};
  string o[] = {"object2", "object1"};
  ASSERT_FALSE(IsSuppressed(VEC(m), VEC(d), VEC(o)));
}

class SuppressionsWithWildcardsTest : public BaseSuppressionsTest {
 protected:
  virtual void SetUp() {
    const string data =
        "{\n"
        "  name\n"
        "  test_tool,tool2:test_warning_type\n"
        "  fun:fun*1\n"
        "  obj:obj*t1\n"
        "  ...\n"
        "  fun:f?n*2\n"
        "}";
    supp_.ReadFromString(data);
  }
};

TEST_F(SuppressionsWithWildcardsTest, Wildcards1) {
  string m[] = {"function1", "bb", "function2"};
  string d[] = {"aaa", "bbb", "ccc"};
  string o[] = {"object2", "object1", "object3"};
  ASSERT_TRUE(IsSuppressed(VEC(m), VEC(d), VEC(o)));
}

TEST_F(SuppressionsWithWildcardsTest, Wildcards2) {
  string m[] = {"some_other_function1", "bb", "function2"};
  string d[] = {"aaa", "bbb", "ccc"};
  string o[] = {"object2", "object1", "object3"};
  ASSERT_FALSE(IsSuppressed(VEC(m), VEC(d), VEC(o)));
}

TEST_F(SuppressionsWithWildcardsTest, Wildcards3) {
  string m[] = {"fun1", "bb", "fanction2"};
  string d[] = {"aaa", "bbb", "ccc"};
  string o[] = {"object2", "objt1", "object3"};
  ASSERT_TRUE(IsSuppressed(VEC(m), VEC(d), VEC(o)));
}

// Tests "..." wildcard.
TEST_F(SuppressionsWithWildcardsTest, VerticalWildcards1) {
  string m[] = {"fun1", "bb", "qq", "fanction2"};
  string d[] = {"aaa", "bbb", "ddd", "ccc"};
  string o[] = {"object2", "objt1", "object3", "object4"};
  ASSERT_TRUE(IsSuppressed(VEC(m), VEC(d), VEC(o)));
}


class MultipleStackTraceTest : public BaseSuppressionsTest {
 protected:
  virtual void SetUp() {
    const string data =
        "{\n"
        "  name\n"
        "  test_tool,tool2:test_warning_type\n"
        "  {\n"
        "    fun:fun*1\n"
        "  }\n"
        "  {\n"
        "    fun:fun*2\n"
        "    fun:fun*3\n"
        "  }\n"
        "  {\n"
        "    ...\n"
        "    fun:fun*4\n"
        "    obj:obj*5\n"
        "  }\n"
        "}";
    supp_.ReadFromString(data);
  }
};

TEST_F(MultipleStackTraceTest, Simple1) {
  string m[] = {"fun1", "bb", "qq", "fun2"};
  string d[] = {"aaa", "bbb", "ddd", "ccc"};
  string o[] = {"object1", "object2", "object3", "object4"};
  ASSERT_TRUE(IsSuppressed(VEC(m), VEC(d), VEC(o)));
}

TEST_F(MultipleStackTraceTest, SecondTemplateMatches) {
  string m[] = {"fun2", "fun3", "qq", "fun2"};
  string d[] = {"aaa", "bbb", "ddd", "ccc"};
  string o[] = {"object1", "object2", "object3", "object4"};
  ASSERT_TRUE(IsSuppressed(VEC(m), VEC(d), VEC(o)));
}

TEST_F(MultipleStackTraceTest, ThirdTemplateMatches) {
  string m[] = {"fun4", "bb", "qq", "fun2"};
  string d[] = {"aaa", "bbb", "ddd", "ccc"};
  string o[] = {"object1", "object5", "object3", "object4"};
  ASSERT_TRUE(IsSuppressed(VEC(m), VEC(d), VEC(o)));
}

TEST_F(MultipleStackTraceTest, NothingMatches) {
  string m[] = {"_fun1", "bb", "qq", "fun2"};
  string d[] = {"aaa", "bbb", "ddd", "ccc"};
  string o[] = {"object1", "object2", "object3", "object4"};
  ASSERT_FALSE(IsSuppressed(VEC(m), VEC(d), VEC(o)));
}

TEST_F(MultipleStackTraceTest, TwoTemplatesMatch) {
  string m[] = {"fun1", "bb", "fun4", "fun2"};
  string d[] = {"aaa", "bbb", "ddd", "ccc"};
  string o[] = {"object1", "object2", "object3", "object5"};
  ASSERT_TRUE(IsSuppressed(VEC(m), VEC(d), VEC(o)));
}


TEST_F(BaseSuppressionsTest, StartsWithVerticalWildcard) {
  const string data =
      "{\n"
      "  name\n"
      "  test_tool:test_warning_type\n"
      "  ...\n"
      "  fun:qq\n"
      "}";
  ASSERT_GT(supp_.ReadFromString(data), 0);
  string m[] = {"fun1", "bb", "qq", "function2"};
  string d[] = {"aaa", "bbb", "ddd", "ccc"};
  string o[] = {"object2", "objt1", "object3", "object4"};
  ASSERT_TRUE(IsSuppressed(VEC(m), VEC(d), VEC(o)));
}

TEST_F(BaseSuppressionsTest, StartsWithVerticalWildcard2) {
  const string data =
      "{\n"
      "  name\n"
      "  test_tool:test_warning_type\n"
      "  ...\n"
      "  fun:fun1\n"
      "}";
  ASSERT_GT(supp_.ReadFromString(data), 0);
  string m[] = {"fun1", "bb", "qq", "function2"};
  string d[] = {"aaa", "bbb", "ddd", "ccc"};
  string o[] = {"object2", "objt1", "object3", "object4"};
  ASSERT_TRUE(IsSuppressed(VEC(m), VEC(d), VEC(o)));
}

TEST_F(BaseSuppressionsTest, EndsWithVerticalWildcard) {
  const string data =
      "{\n"
      "  name\n"
      "  test_tool:test_warning_type\n"
      "  fun:fun1\n"
      "  ...\n"
      "}";
  ASSERT_GT(supp_.ReadFromString(data), 0);
  string m[] = {"fun1", "bb", "qq", "function2"};
  string d[] = {"aaa", "bbb", "ddd", "ccc"};
  string o[] = {"object2", "objt1", "object3", "object4"};
  ASSERT_TRUE(IsSuppressed(VEC(m), VEC(d), VEC(o)));
}

TEST_F(BaseSuppressionsTest, EndsWithVerticalWildcard2) {
  const string data =
      "{\n"
      "  name\n"
      "  test_tool:test_warning_type\n"
      "  fun:qq\n"
      "  ...\n"
      "}";
  ASSERT_GT(supp_.ReadFromString(data), 0);
  string m[] = {"fun1", "bb", "qq", "function2"};
  string d[] = {"aaa", "bbb", "ddd", "ccc"};
  string o[] = {"object2", "objt1", "object3", "object4"};
  ASSERT_FALSE(IsSuppressed(VEC(m), VEC(d), VEC(o)));
}

TEST_F(BaseSuppressionsTest, Complex) {
  const string data =
      "{\n"
      "  name\n"
      "  test_tool:test_warning_type\n"
      "  fun:qq\n"
      "  ...\n"
      "  obj:obj*3\n"
      "  ...\n"
      "  fun:function?\n"
      "}";
  ASSERT_GT(supp_.ReadFromString(data), 0);
  string m[] = {"fun1", "bb", "qq", "function2"};
  string d[] = {"aaa", "bbb", "ddd", "ccc"};
  string o[] = {"object2", "objt1", "object3", "object4"};
  ASSERT_FALSE(IsSuppressed(VEC(m), VEC(d), VEC(o)));
}

TEST_F(BaseSuppressionsTest, DemangledNames) {
  const string data =
      "{\n"
      "  name\n"
      "  test_tool:test_warning_type\n"
      "  fun:bb*w?\n"
      "}";
  ASSERT_GT(supp_.ReadFromString(data), 0);
  string m[] = {"fun1", "bb", "qq", "function2"};
  string d[] = {"bbbxxwz", "aaa", "ddd", "ccc"};
  string o[] = {"object2", "objt1", "object3", "object4"};
  ASSERT_TRUE(IsSuppressed(VEC(m), VEC(d), VEC(o)));
}

TEST_F(BaseSuppressionsTest, TrailingWhitespace) {
  const string data =
      "{\n"
      "  name\n"
      "  test_tool:test_warning_type\n"
      "  fun:bb*w? \n"
      "}";
  ASSERT_GT(supp_.ReadFromString(data), 0);
  string m[] = {"fun1", "bb", "qq", "function2"};
  string d[] = {"bbbxxwz", "aaa", "ddd", "ccc"};
  string o[] = {"object2", "objt1", "object3", "object4"};
  ASSERT_TRUE(IsSuppressed(VEC(m), VEC(d), VEC(o)));
}

TEST_F(BaseSuppressionsTest, ObjectiveC) {
  const string data =
      "{\n"
      "  name\n"
      "  test_tool:test_warning_type\n"
      "  fun:-[NSObject(NSKeyValueCoding) setValue:forKeyPath:]\n"
      "}";
  ASSERT_GT(supp_.ReadFromString(data), 0);
  string m[] = {"-[NSObject(NSKeyValueCoding) setValue:forKeyPath:]", "function2"};
  string d[] = {"bbbxxwz", "aaa", "ddd", "ccc"};
  string o[] = {"object2", "objt1", "object3", "object4"};
  ASSERT_TRUE(IsSuppressed(VEC(m), VEC(d), VEC(o)));
}

TEST_F(BaseSuppressionsTest, ComparisonAndShiftOperators) {
  const string data =
      "{\n"
      "  name\n"
      "  test_tool:test_warning_type\n"
      "  fun:operator<\n"
      "  fun:operator>\n"
      "  fun:operator<=\n"
      "  fun:operator>=\n"
      "  fun:operator<<\n"
      "  fun:operator>>\n"
      "  fun:operator<<=\n"
      "  fun:operator>>=\n"
      "  fun:operator->\n"
      "  fun:operator->*\n"
      "}";
  ASSERT_GT(supp_.ReadFromString(data), 0);
  string m[] = {"operator<", "operator>", "operator<=", "operator>=",
                "operator<<", "operator>>", "operator<<=", "operator>>=",
                "operator->", "operator->*"};
  string d[] = {"bbbxxwz", "aaa", "ddd", "ccc"};
  string o[] = {"object2", "objt1", "object3", "object4"};
  ASSERT_TRUE(IsSuppressed(VEC(m), VEC(d), VEC(o)));
}


class FailingSuppressionsTest : public ::testing::Test {
 protected:
  int ErrorLineNo(string data) {
    int result = supp_.ReadFromString(data);
    if (result >= 0)
      return -1;
    else
      return supp_.GetErrorLineNo();
  }

  Suppressions supp_;
};

TEST_F(FailingSuppressionsTest, NoOpeningBrace) {
  const string data =
      "  name\n"
      "  test_tool:test_warning_type\n"
      "  fun:bb*w? \n"
      "}";
  ASSERT_EQ(1, ErrorLineNo(data));
}

TEST_F(FailingSuppressionsTest, Bad1) {
  const string data =
      "{\n"
      "  name\n"
      "  something_else\n"
      "  test_tool:test_warning_type\n"
      "  fun:bb*w? \n"
      "}";
  ASSERT_EQ(3, ErrorLineNo(data));
}

TEST_F(FailingSuppressionsTest, Bad2) {
  const string data =
      "{\n"
      "  name\n"
      "  test_tool:test_warning_type\n"
      "  extra\n"
      "  fun:bb*w? \n"
      "}";
  ASSERT_EQ(4, ErrorLineNo(data));
}

TEST_F(FailingSuppressionsTest, Bad3) {
  const string data =
      "{\n"
      "  name\n"
      "  test_tool:test_warning_type\n"
      "  fun:bb*w? \n"
      "  extra\n"
      "}";
  ASSERT_EQ(5, ErrorLineNo(data));
}

TEST_F(FailingSuppressionsTest, SomeWeirdTextAfterASuppression) {
  const string data =
      "{\n"
      "  name\n"
      "  test_tool:test_warning_type\n"
      "  fun:bb*w? \n"
      "}\n"
      "some_weird_text\n"
      "after_a_suppression\n";
  ASSERT_EQ(6, ErrorLineNo(data));
}

TEST_F(FailingSuppressionsTest, NoToolsLineInMultitraceSuppression) {
  const string data =
      "{\n"
      "  name\n"
      "  {\n"
      "    fun:fun*2\n"
      "    fun:fun*3\n"
      "  }\n"
      "  {\n"
      "    ...\n"
      "    fun:fun*4\n"
      "    obj:obj*5\n"
      "  }\n"
      "}";
  ASSERT_EQ(3, ErrorLineNo(data));
}

TEST_F(FailingSuppressionsTest, BadStacktrace1) {
  const string data =
      "{\n"
      "  name\n"
      "  test_tool:test_warning_type\n"
      "  {\n"
      "    fun:fun*2\n"
      "    fun:fun*3\n"
      "  }\n"
      "  {\n"
      "    zzz\n"
      "    fun:fun*4\n"
      "    obj:obj*5\n"
      "  }\n"
      "}";
  ASSERT_EQ(9, ErrorLineNo(data));
}

TEST_F(FailingSuppressionsTest, BadStacktrace2) {
  const string data =
      "{\n"
      "  name\n"
      "  test_tool:test_warning_type\n"
      "  {\n"
      "    fun:fun*2\n"
      "    fun:fun*3\n"
      "  }\n"
      "  {\n"
      "    {\n"
      "    fun:fun*4\n"
      "    obj:obj*5\n"
      "  }\n"
      "}";
  ASSERT_EQ(9, ErrorLineNo(data));
}

TEST_F(FailingSuppressionsTest, BadStacktrace3) {
  const string data =
      "{\n"
      "  name\n"
      "  test_tool:test_warning_type\n"
      "  {\n"
      "    fun:fun*2\n"
      "    fun:fun*3\n"
      "  }\n"
      "  {\n"
      "    fun:fun*4\n"
      "    obj:obj*5\n"
      "  }\n"
      "  zzz\n"
      "}";
  ASSERT_EQ(12, ErrorLineNo(data));
}

TEST_F(FailingSuppressionsTest, StacktraceWithParenthesis) {
  const string data =
      "{\n"
      "  name\n"
      "  test_tool:test_warning_type\n"
      "  {\n"
      "    fun:fun*2\n"
      "    fun:fun*3\n"
      "  }\n"
      "  {\n"
      "    fun:fun*4()\n"
      "    obj:obj*5\n"
      "  }\n"
      "}";
  ASSERT_EQ(9, ErrorLineNo(data));
}

TEST_F(FailingSuppressionsTest, StacktraceWithAngleBraces) {
  const string data =
      "{\n"
      "  name\n"
      "  test_tool:test_warning_type\n"
      "  {\n"
      "    fun:fun*2\n"
      "    fun:fun*3\n"
      "  }\n"
      "  {\n"
      "    fun:fun<int>*4\n"
      "    obj:obj*5\n"
      "  }\n"
      "}";
  ASSERT_EQ(9, ErrorLineNo(data));
}


TEST(WildcardTest, Simple) {
  EXPECT_TRUE(StringMatch("abc", "abc"));
  EXPECT_FALSE(StringMatch("abcd", "abc"));
  EXPECT_FALSE(StringMatch("dabc", "abc"));
  EXPECT_FALSE(StringMatch("ab", "abc"));
  EXPECT_FALSE(StringMatch("", "abc"));
  EXPECT_FALSE(StringMatch("abc", ""));
  EXPECT_TRUE(StringMatch("", ""));
}

TEST(WildcardTest, SingleCharacterWildcard) {
  EXPECT_TRUE(StringMatch("a?c", "abc"));
  EXPECT_TRUE(StringMatch("?bc", "abc"));
  EXPECT_TRUE(StringMatch("ab?", "abc"));
  EXPECT_TRUE(StringMatch("a??", "abc"));
  EXPECT_TRUE(StringMatch("???", "abc"));
  EXPECT_TRUE(StringMatch("?", "a"));
  EXPECT_FALSE(StringMatch("?zc", "abc"));
  EXPECT_FALSE(StringMatch("?bz", "abc"));
  EXPECT_FALSE(StringMatch("b?c", "abc"));
  EXPECT_FALSE(StringMatch("az?", "abc"));
  EXPECT_FALSE(StringMatch("abc?", "abc"));
  EXPECT_FALSE(StringMatch("?abc", "abc"));
  EXPECT_FALSE(StringMatch("?", ""));
  EXPECT_FALSE(StringMatch("??", ""));
}

TEST(WildcardTest, MultiCharacterWildcard) {
  EXPECT_TRUE(StringMatch("*x", "x"));
  EXPECT_TRUE(StringMatch("x*", "x"));
  EXPECT_TRUE(StringMatch("*x*", "x"));

  EXPECT_TRUE(StringMatch("a*d", "abcd"));
  EXPECT_TRUE(StringMatch("ab*d", "abcd"));
  EXPECT_TRUE(StringMatch("*cd", "abcd"));
  EXPECT_TRUE(StringMatch("*d", "abcd"));
  EXPECT_TRUE(StringMatch("ab*", "abcd"));
  EXPECT_TRUE(StringMatch("a*", "abcd"));
  EXPECT_TRUE(StringMatch("*", "abcd"));
  EXPECT_TRUE(StringMatch("ab*cd", "abcd"));

  EXPECT_TRUE(StringMatch("ab**", "abcd"));
  EXPECT_TRUE(StringMatch("**", "abcd"));
  EXPECT_TRUE(StringMatch("***", "abcd"));
  EXPECT_TRUE(StringMatch("**d", "abcd"));
  EXPECT_TRUE(StringMatch("*c*", "abcd"));
  EXPECT_TRUE(StringMatch("a*c*d*f", "abcdef"));
  EXPECT_TRUE(StringMatch("a*c*e*", "abcdef"));
  EXPECT_TRUE(StringMatch("*a*b*f", "abcdef"));
  EXPECT_TRUE(StringMatch("*b*d*", "abcdef"));

  EXPECT_FALSE(StringMatch("b*", "abcd"));
  EXPECT_FALSE(StringMatch("*c", "abcd"));
  EXPECT_FALSE(StringMatch("*a", "abcd"));
}

TEST(WildcardTest, WildcardCharactersInText) {
  EXPECT_TRUE(StringMatch("?", "?"));
  EXPECT_FALSE(StringMatch("a", "?"));
  EXPECT_FALSE(StringMatch("ab", "a?"));
  EXPECT_FALSE(StringMatch("ab", "?b"));
  EXPECT_TRUE(StringMatch("a?", "a?"));
  EXPECT_TRUE(StringMatch("?b", "?b"));

  EXPECT_TRUE(StringMatch("*", "*"));
  EXPECT_FALSE(StringMatch("a", "*"));
  EXPECT_FALSE(StringMatch("ab", "a*"));
  EXPECT_FALSE(StringMatch("ab", "*b"));
  EXPECT_TRUE(StringMatch("a*", "a*"));
  EXPECT_TRUE(StringMatch("*b", "*b"));
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
