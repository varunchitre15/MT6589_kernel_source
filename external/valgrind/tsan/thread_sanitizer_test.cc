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

// This file contains tests for various parts of ThreadSanitizer.

#include <gtest/gtest.h>

#include "ts_heap_info.h"
#include "ts_simple_cache.h"
#include "dense_multimap.h"

// Testing the HeapMap.
struct TestHeapInfo {
  uintptr_t ptr;
  uintptr_t size;
  int       val;
  TestHeapInfo() : ptr(0), size(0), val(0) { }
  TestHeapInfo(uintptr_t p, uintptr_t s, uintptr_t v) :
    ptr(p), size(s), val(v) { }
};

TEST(ThreadSanitizer, HeapInfoTest) {
  HeapMap<TestHeapInfo> map;
  TestHeapInfo *info;
  EXPECT_EQ(0U, map.size());
  EXPECT_EQ(NULL, map.GetInfo(12345));

  // Insert range [1000, 1000+100) with value 1.
  map.InsertInfo(1000, TestHeapInfo(1000, 100, 1));
  EXPECT_EQ(1U, map.size());
  info = map.GetInfo(1000);
  EXPECT_TRUE(info);
  EXPECT_EQ(1000U, info->ptr);
  EXPECT_EQ(100U, info->size);
  EXPECT_EQ(1, info->val);

  EXPECT_TRUE(map.GetInfo(1000));
  EXPECT_EQ(1, info->val);
  EXPECT_TRUE(map.GetInfo(1050));
  EXPECT_EQ(1, info->val);
  EXPECT_TRUE(map.GetInfo(1099));
  EXPECT_EQ(1, info->val);
  EXPECT_FALSE(map.GetInfo(1100));
  EXPECT_FALSE(map.GetInfo(2000));

  EXPECT_EQ(NULL, map.GetInfo(2000));
  EXPECT_EQ(NULL, map.GetInfo(3000));

  // Insert range [2000, 2000+200) with value 2.
  map.InsertInfo(2000, TestHeapInfo(2000, 200, 2));
  EXPECT_EQ(2U, map.size());

  info = map.GetInfo(1000);
  EXPECT_TRUE(info);
  EXPECT_EQ(1, info->val);

  info = map.GetInfo(2000);
  EXPECT_TRUE(info);
  EXPECT_EQ(2, info->val);

  info = map.GetInfo(1000);
  EXPECT_TRUE(info);
  EXPECT_EQ(1, info->val);
  EXPECT_TRUE((info = map.GetInfo(1050)));
  EXPECT_EQ(1, info->val);
  EXPECT_TRUE((info = map.GetInfo(1099)));
  EXPECT_EQ(1, info->val);
  EXPECT_FALSE(map.GetInfo(1100));

  EXPECT_TRUE((info = map.GetInfo(2000)));
  EXPECT_EQ(2, info->val);
  EXPECT_TRUE((info = map.GetInfo(2199)));
  EXPECT_EQ(2, info->val);

  EXPECT_FALSE(map.GetInfo(2200));
  EXPECT_FALSE(map.GetInfo(3000));

  // Insert range [3000, 3000+300) with value 3.
  map.InsertInfo(3000, TestHeapInfo(3000, 300, 3));
  EXPECT_EQ(3U, map.size());

  EXPECT_TRUE((info = map.GetInfo(1000)));
  EXPECT_EQ(1, info->val);

  EXPECT_TRUE((info = map.GetInfo(2000)));
  EXPECT_EQ(2, info->val);

  EXPECT_TRUE((info = map.GetInfo(3000)));
  EXPECT_EQ(3, info->val);

  EXPECT_TRUE((info = map.GetInfo(1050)));
  EXPECT_EQ(1, info->val);

  EXPECT_TRUE((info = map.GetInfo(2100)));
  EXPECT_EQ(2, info->val);

  EXPECT_TRUE((info = map.GetInfo(3200)));
  EXPECT_EQ(3, info->val);

  // Remove range [2000,2000+200)
  map.EraseInfo(2000);
  EXPECT_EQ(2U, map.size());

  EXPECT_TRUE((info = map.GetInfo(1050)));
  EXPECT_EQ(1, info->val);

  EXPECT_FALSE(map.GetInfo(2100));

  EXPECT_TRUE((info = map.GetInfo(3200)));
  EXPECT_EQ(3, info->val);

}

TEST(ThreadSanitizer, PtrToBoolCacheTest) {
  PtrToBoolCache<256> c;
  bool val = false;
  EXPECT_FALSE(c.Lookup(123, &val));

  c.Insert(0, false);
  c.Insert(1, true);
  c.Insert(2, false);
  c.Insert(3, true);

  EXPECT_TRUE(c.Lookup(0, &val));
  EXPECT_EQ(false, val);
  EXPECT_TRUE(c.Lookup(1, &val));
  EXPECT_EQ(true, val);
  EXPECT_TRUE(c.Lookup(2, &val));
  EXPECT_EQ(false, val);
  EXPECT_TRUE(c.Lookup(3, &val));
  EXPECT_EQ(true, val);

  EXPECT_FALSE(c.Lookup(256, &val));
  EXPECT_FALSE(c.Lookup(257, &val));
  EXPECT_FALSE(c.Lookup(258, &val));
  EXPECT_FALSE(c.Lookup(259, &val));

  c.Insert(0, true);
  c.Insert(1, false);

  EXPECT_TRUE(c.Lookup(0, &val));
  EXPECT_EQ(true, val);
  EXPECT_TRUE(c.Lookup(1, &val));
  EXPECT_EQ(false, val);
  EXPECT_TRUE(c.Lookup(2, &val));
  EXPECT_EQ(false, val);
  EXPECT_TRUE(c.Lookup(3, &val));
  EXPECT_EQ(true, val);

  c.Insert(256, false);
  c.Insert(257, false);
  EXPECT_FALSE(c.Lookup(0, &val));
  EXPECT_FALSE(c.Lookup(1, &val));
  EXPECT_TRUE(c.Lookup(2, &val));
  EXPECT_EQ(false, val);
  EXPECT_TRUE(c.Lookup(3, &val));
  EXPECT_EQ(true, val);
  EXPECT_TRUE(c.Lookup(256, &val));
  EXPECT_EQ(false, val);
  EXPECT_TRUE(c.Lookup(257, &val));
  EXPECT_EQ(false, val);
}

TEST(ThreadSanitizer, IntPairToBoolCacheTest) {
  IntPairToBoolCache<257> c;
  bool val = false;
  map<pair<int,int>, bool> m;

  for (int i = 0; i < 1000000; i++) {
    int a = (rand() % 1024) + 1;
    int b = (rand() % 1024) + 1;

    if (c.Lookup(a, b, &val)) {
      EXPECT_EQ(1U, m.count(make_pair(a,b)));
      EXPECT_EQ(val, m[make_pair(a,b)]);
    }

    val = (rand() % 2) == 1;
    c.Insert(a, b, val);
    m[make_pair(a,b)] = val;
  }
}

TEST(ThreadSanitizer, DenseMultimapTest) {
  typedef DenseMultimap<int, 3> Map;

  Map m1(1, 2);
  EXPECT_EQ(m1[0], 1);
  EXPECT_EQ(m1[1], 2);
  EXPECT_EQ(m1.size(), 2U);

  Map m2(3, 2);
  EXPECT_EQ(m2[0], 2);
  EXPECT_EQ(m2[1], 3);
  EXPECT_EQ(m2.size(), 2U);

  Map m3(m1, 0);
  EXPECT_EQ(m3.size(), 3U);
  EXPECT_EQ(m3[0], 0);
  EXPECT_EQ(m3[1], 1);
  EXPECT_EQ(m3[2], 2);

  Map m4(m3, 1);
  EXPECT_EQ(m4.size(), 4U);
  EXPECT_EQ(m4[0], 0);
  EXPECT_EQ(m4[1], 1);
  EXPECT_EQ(m4[2], 1);
  EXPECT_EQ(m4[3], 2);

  Map m5(m4, 5);
  Map m6(m5, -2);
  Map m7(m6, 2);
  EXPECT_EQ(m7.size(), 7U);

  EXPECT_TRUE(m7.has(-2));
  EXPECT_TRUE(m7.has(0));
  EXPECT_TRUE(m7.has(1));
  EXPECT_TRUE(m7.has(2));
  EXPECT_TRUE(m7.has(5));
  EXPECT_FALSE(m7.has(3));
  EXPECT_FALSE(m7.has(-1));
  EXPECT_FALSE(m7.has(4));

  Map m8(m7, Map::REMOVE, 1);
  EXPECT_EQ(m8.size(), 6U);
  EXPECT_TRUE(m8.has(1));

  Map m9(m8, Map::REMOVE, 1);
  EXPECT_EQ(m9.size(), 5U);
  EXPECT_FALSE(m9.has(1));
}

TEST(ThreadSanitizer, NormalizeFunctionNameNotChangingTest) {
  const char *samples[] = {
    // These functions should not be changed by NormalizeFunctionName():
    // C functions
    "main",
    "pthread_mutex_unlock",
    "pthread_create@@GLIBC_2.2.5",
    "pthread_create@*"

    // Valgrind can give us this, we should keep it.
    "(below main)",

    // C++ operators
    "operator new[]",
    "operator delete[]",

    // PIN on Windows handles non-templated C++ code well
    "my_namespace::ClassName::Method",
    "PositiveTests_HarmfulRaceInDtor::A::~A",
    "PositiveTests_HarmfulRaceInDtor::B::`scalar deleting destructor'",

    // Objective-C on Mac
    "+[NSNavFBENode _virtualNodeOfType:]",
    "-[NSObject(NSObject) autorelease]",
    "-[NSObject(NSKeyValueCoding) setValue:forKeyPath:]",
    "-[NSCell(NSPrivate_CellMouseTracking) _setMouseTrackingInRect:ofView:]",
    // TODO(glider): other interesting cases from Objective-C?
    // Should we "s/:.*\]/\]/" ?
  };

  for (size_t i = 0; i < sizeof(samples) / sizeof(samples[0]); i += 2) {
    EXPECT_STREQ(samples[i], NormalizeFunctionName(samples[i]).c_str());
  }
}

TEST(ThreadSanitizer, NormalizeFunctionNameChangingTest) {
  const char *samples[] = {
    // These functions should be changed by removing <.*> and (.*) while
    // correctly handling the "function returns a [template] function pointer"
    // case.
    // This is a list of (full demangled name, short name) pairs.
    "SuppressionTests::Foo(int*)", "SuppressionTests::Foo",
    "logging::LogMessage::Init(char const*, int)", "logging::LogMessage::Init",
    "void DispatchToMethod<net::SpdySession, void (net::SpdySession::*)(int), int>(net::SpdySession*, void (net::SpdySession::*)(int), Tuple1<int> const&)",
        "DispatchToMethod",
    "MessageLoop::DeferOrRunPendingTask(MessageLoop::PendingTask const&)",
        "MessageLoop::DeferOrRunPendingTask",
    "spdy::SpdyFramer::ProcessInput(char const*, unsigned long)",
        "spdy::SpdyFramer::ProcessInput",
    "base::RefCountedThreadSafe<history::HistoryBackend, base::DefaultRefCountedThreadSafeTraits<history::HistoryBackend> >::Release() const",
        "base::RefCountedThreadSafe::Release",
    "net::X509Certificate::Verify(std::string const&, int, net::CertVerifyResult*) const",
        "net::X509Certificate::Verify",

    "(anonymous namespace)::ExtentToStringSet(ExtensionExtent const&, std::set<std::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::less<std::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::basic_string<char, std::char_traits<char>, std::allocator<char> > > >*)",
        "::ExtentToStringSet",

    "scoped_ptr<(anonymous namespace)::ImportEndedObserver>::operator->() const",
        "scoped_ptr::operator->",

    "int (anonymous namespace)::ValueCompare<long>(long, long)",
        "::ValueCompare",

    "std::basic_string<unsigned short, base::string16_char_traits, std::allocator<unsigned short> > const& std::__median<std::basic_string<unsigned short, base::string16_char_traits, std::allocator<unsigned short> >, (anonymous namespace)::CompareQuality>(std::basic_string<unsigned short, base::string16_char_traits, std::allocator<unsigned short> > const&, std::basic_string<unsigned short, base::string16_char_traits, std::allocator<unsigned short> > const&, std::basic_string<unsigned short, base::string16_char_traits, std::allocator<unsigned short> > const&, (anonymous namespace)::CompareQuality)",
        "std::__median",

    "std::basic_ostream<char, std::char_traits<char> >& std::operator<< <char, std::char_traits<char> >(std::basic_ostream<char, std::char_traits<char> >&, std::_Setprecision)",
        "std::operator<<",

    "net::(anonymous namespace)::CookieSignature::operator<(net::(anonymous namespace)::CookieSignature const&) const",
        "net::::CookieSignature::operator<",

    "v8::Handle<v8::Value> (*v8::ToCData<v8::Handle<v8::Value> (*)(v8::Arguments const&)>(v8::internal::Object*))(v8::Arguments const&)",
        "v8::ToCData",

    "v8::internal::Handle<v8::internal::Object> v8::FromCData<v8::Handle<v8::Value> (*)(v8::Local<v8::String>, v8::AccessorInfo const&)>(v8::Handle<v8::Value> (*)(v8::Local<v8::String>, v8::AccessorInfo const&))",
        "v8::FromCData",

    "WebCore::operator<<(WebCore::TextStream&, WebCore::LineCap)",
        "WebCore::operator<<",

    "__gnu_cxx::__normal_iterator<void (**)(), std::vector<void (*)(), std::allocator<void (*)()> > >::base() const",
        "__gnu_cxx::__normal_iterator::base",

    "__gnu_cxx::__normal_iterator<device_orientation::DataFetcher* (* const*)(), std::vector<device_orientation::DataFetcher* (*)(), std::allocator<device_orientation::DataFetcher* (*)()> > >::operator++()",
        "__gnu_cxx::__normal_iterator::operator++",

    "__gnu_cxx::__normal_iterator<std::pair<int, std::basic_string<unsigned short, base::string16_char_traits, std::allocator<unsigned short> > >*, std::vector<std::pair<int, std::basic_string<unsigned short, base::string16_char_traits, std::allocator<unsigned short> > >, std::allocator<std::pair<int, std::basic_string<unsigned short, base::string16_char_traits, std::allocator<unsigned short> > > > > >::operator->() const",
        "__gnu_cxx::__normal_iterator::operator->",

    "std::less<CancelableRequestConsumerTSimple<PageUsageData*>::PendingRequest>::operator()(CancelableRequestConsumerTSimple<PageUsageData*>::PendingRequest const&, CancelableRequestConsumerTSimple<PageUsageData*>::PendingRequest const&) const",
        "std::less::operator()",

    "SuppressionTests::MyClass<int>::Fooz(int*) const",
        "SuppressionTests::MyClass::Fooz",

    // Templates and functions returning function pointers
    "void (*SuppressionTests::TemplateFunction1<void (*)(int*)>(void (*)(int*)))(int*)",
        "SuppressionTests::TemplateFunction1",  // Valgrind, Linux
    "void SuppressionTests::TemplateFunction2<void>()",
        "SuppressionTests::TemplateFunction2",  // OMG, return type in template
    "void (**&SuppressionTests::TemplateFunction3<void (*)(int)>())",
        "SuppressionTests::TemplateFunction3",  // Valgrind, Linux

    "SuppressionTests::TemplateFunction1<void (__cdecl*)(int *)>",
        "SuppressionTests::TemplateFunction1",  // PIN, Windows
    "SuppressionTests::MyClass<int>::Fooz",
        "SuppressionTests::MyClass::Fooz",
    "std::operator<<char,std::char_traits<char>,std::allocator<char> >",
        "(malformed frame)",  // Should be "std::operator<"? Really?
    "std::_Ranit<stdext::_Hash<stdext::_Hmap_traits<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,int,stdext::hash_compare<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,int> >,0> >::_List_position,int,stdext::_Hash<stdext::_Hmap_traits<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,int,stdext::hash_compare<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,int> >,0> >::_List_position const *,stdext::_Hash<stdext::_Hmap_traits<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,int,stdext::hash_compare<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,int> >,0> >::_List_position const &>::_Ranit<stdext::_Hash<stdext::_Hmap_traits<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,int,stdext::hash_compare<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,int> >,0> >::_List_position,int,stdext::_Hash<stdext::_Hmap_traits<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,int,stdext::hash_compare<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >,std::allocator<std::pair<std::basic",
        "(malformed frame)",
    "std::_Tree_val<std::_Tmap_traits<net::`anonymous namespace'::CookieSignature,std::set<std::_Tree<std::_Tmap_traits<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,net::CookieMonster::CanonicalCookie *,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,net::CookieMonster::CanonicalCookie *> >,1> >::iterator,net::`anonymous namespace'::OrderByCreationTimeDesc,std::allocator<std::_Tree<std::_Tmap_traits<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,net::CookieMonster::CanonicalCookie *,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,net::CookieMonster::CanonicalCookie *> >,1> >::iterator> >,std::less<net::`anonymous namespace'::CookieSignature>,std::allocator<std::pair<net::`anonymous namespace'::CookieSignature const ,std::set<std::_Tree<std::_Tmap_traits<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,net::CookieMonster::CanonicalCookie *,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,net::CookieMonster::CanonicalCookie *> >,1> >::iterator,net::`anonymous namespace'::OrderByCreationTimeDesc,std::allocator<std::_Tree<std::_Tmap_traits<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,net::CookieMonster::CanonicalCookie *,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,net::CookieMonster::CanonicalCookie *> >,1> >::iterator> > > >,0> >::~_Tree_val<std::_Tmap_traits<net::`anonymous namespace'::CookieSignature,std::set<std::_Tree<std::_Tmap_traits<std::basic_string<",
        "(malformed frame)",

    "__gnu_cxx::new_allocator<char>::allocate(unsigned long, void const*)",
        "__gnu_cxx::new_allocator::allocate",

    "PositiveTests_HarmfulRaceInDtor::A::~A()",  // Valgrind, Linux
        "PositiveTests_HarmfulRaceInDtor::A::~A",

    "X::foo(int*) const()",   // GCC, Linux
        "X::foo",
    "X::foo(int*) const volatile",
        "X::foo",

    "base::(anonymous namespace)::ThreadFunc(void*)",
      "base::::ThreadFunc",  // TODO(timurrrr): keep "anonymous namespace"?

    "operator new[](unsigned long)", "operator new[]",  // Valgrind, Linux
  };

  for (size_t i = 0; i < sizeof(samples) / sizeof(samples[0]); i += 2) {
    EXPECT_STREQ(samples[i+1], NormalizeFunctionName(samples[i]).c_str());
  }
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
