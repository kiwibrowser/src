/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/platform/wtf/text/cstring.h"

#include "testing/gtest/include/gtest/gtest.h"
#include <sstream>

namespace WTF {

namespace {

CString PrintedString(const CString& string) {
  std::ostringstream output;
  output << string;
  const std::string& result = output.str();
  return CString(result.data(), result.length());
}

}  // anonymous namespace

TEST(CStringTest, NullStringConstructor) {
  CString string;
  EXPECT_TRUE(string.IsNull());
  EXPECT_EQ(static_cast<const char*>(nullptr), string.data());
  EXPECT_EQ(static_cast<size_t>(0), string.length());

  CString string_from_char_pointer(static_cast<const char*>(nullptr));
  EXPECT_TRUE(string_from_char_pointer.IsNull());
  EXPECT_EQ(static_cast<const char*>(nullptr), string_from_char_pointer.data());
  EXPECT_EQ(static_cast<size_t>(0), string_from_char_pointer.length());

  CString string_from_char_and_length(static_cast<const char*>(nullptr), 0);
  EXPECT_TRUE(string_from_char_and_length.IsNull());
  EXPECT_EQ(static_cast<const char*>(nullptr),
            string_from_char_and_length.data());
  EXPECT_EQ(static_cast<size_t>(0), string_from_char_and_length.length());
}

TEST(CStringTest, EmptyEmptyConstructor) {
  const char* empty_string = "";
  CString string(empty_string);
  EXPECT_FALSE(string.IsNull());
  EXPECT_EQ(static_cast<size_t>(0), string.length());
  EXPECT_EQ(0, string.data()[0]);

  CString string_with_length(empty_string, 0);
  EXPECT_FALSE(string_with_length.IsNull());
  EXPECT_EQ(static_cast<size_t>(0), string_with_length.length());
  EXPECT_EQ(0, string_with_length.data()[0]);
}

TEST(CStringTest, EmptyRegularConstructor) {
  const char* reference_string = "WebKit";

  CString string(reference_string);
  EXPECT_FALSE(string.IsNull());
  EXPECT_EQ(strlen(reference_string), string.length());
  EXPECT_STREQ(reference_string, string.data());

  CString string_with_length(reference_string, 6);
  EXPECT_FALSE(string_with_length.IsNull());
  EXPECT_EQ(strlen(reference_string), string_with_length.length());
  EXPECT_STREQ(reference_string, string_with_length.data());
}

TEST(CStringTest, UninitializedConstructor) {
  char* buffer;
  CString empty_string = CString::CreateUninitialized(0, buffer);
  EXPECT_FALSE(empty_string.IsNull());
  EXPECT_EQ(buffer, empty_string.data());
  EXPECT_EQ(0, buffer[0]);

  const size_t kLength = 25;
  CString uninitialized_string = CString::CreateUninitialized(kLength, buffer);
  EXPECT_FALSE(uninitialized_string.IsNull());
  EXPECT_EQ(buffer, uninitialized_string.data());
  EXPECT_EQ(0, uninitialized_string.data()[kLength]);
}

TEST(CStringTest, ZeroTerminated) {
  const char* reference_string = "WebKit";
  CString string_with_length(reference_string, 3);
  EXPECT_EQ(0, string_with_length.data()[3]);
}

TEST(CStringTest, Comparison) {
  // Comparison with another CString.
  CString a;
  CString b;
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);
  a = "a";
  b = CString();
  EXPECT_FALSE(a == b);
  EXPECT_TRUE(a != b);
  a = "a";
  b = "b";
  EXPECT_FALSE(a == b);
  EXPECT_TRUE(a != b);
  a = "a";
  b = "a";
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);
  a = "a";
  b = "aa";
  EXPECT_FALSE(a == b);
  EXPECT_TRUE(a != b);
  a = "";
  b = "";
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);
  a = "";
  b = CString();
  EXPECT_FALSE(a == b);
  EXPECT_TRUE(a != b);
  a = "a";
  b = "";
  EXPECT_FALSE(a == b);
  EXPECT_TRUE(a != b);

  // Comparison with a const char*.
  CString c;
  const char* d = nullptr;
  EXPECT_TRUE(c == d);
  EXPECT_FALSE(c != d);
  c = "c";
  d = nullptr;
  EXPECT_FALSE(c == d);
  EXPECT_TRUE(c != d);
  c = CString();
  d = "d";
  EXPECT_FALSE(c == d);
  EXPECT_TRUE(c != d);
  c = "c";
  d = "d";
  EXPECT_FALSE(c == d);
  EXPECT_TRUE(c != d);
  c = "c";
  d = "c";
  EXPECT_TRUE(c == d);
  EXPECT_FALSE(c != d);
  c = "c";
  d = "cc";
  EXPECT_FALSE(c == d);
  EXPECT_TRUE(c != d);
  c = "cc";
  d = "c";
  EXPECT_FALSE(c == d);
  EXPECT_TRUE(c != d);
  c = "";
  d = "";
  EXPECT_TRUE(c == d);
  EXPECT_FALSE(c != d);
  c = "";
  d = nullptr;
  EXPECT_FALSE(c == d);
  EXPECT_TRUE(c != d);
  c = CString();
  d = "";
  EXPECT_FALSE(c == d);
  EXPECT_TRUE(c != d);
  c = "a";
  d = "";
  EXPECT_FALSE(c == d);
  EXPECT_TRUE(c != d);
  c = "";
  d = "b";
  EXPECT_FALSE(c == d);
  EXPECT_TRUE(c != d);
}

TEST(CStringTest, Printer) {
  EXPECT_STREQ("<null>", PrintedString(CString()).data());
  EXPECT_STREQ("\"abc\"", PrintedString("abc").data());
  EXPECT_STREQ("\"\\t\\n\\r\\\"\\\\\"", PrintedString("\t\n\r\"\\").data());
  EXPECT_STREQ("\"\\xFF\\x00\\x01xyz\"",
               PrintedString(CString("\xff\0\x01xyz", 6)).data());
}

}  // namespace WTF
