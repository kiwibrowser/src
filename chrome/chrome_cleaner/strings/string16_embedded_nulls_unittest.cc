// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/chrome_cleaner/strings/string16_embedded_nulls.h"

#include <vector>

#include "base/strings/string_piece.h"
#include "chrome/chrome_cleaner/strings/string_test_helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chrome_cleaner {

namespace {

constexpr wchar_t kStringWithNulls[] = L"string0with0nulls";

base::string16 FormatStringPiece16(base::StringPiece16 sp) {
  return FormatVectorWithNulls(std::vector<wchar_t>(sp.begin(), sp.end()));
}

class String16EmbeddedNullsTest : public ::testing::Test {
 protected:
  String16EmbeddedNullsTest() : vec_(CreateVectorWithNulls(kStringWithNulls)) {}

  const std::vector<wchar_t> vec_;
};

void ExpectEmpty(const String16EmbeddedNulls& str) {
  EXPECT_EQ(base::StringPiece16(), str.CastAsStringPiece16());
  EXPECT_EQ(0u, str.size());
  EXPECT_FALSE(str.CastAsUInt16Array());
}

TEST_F(String16EmbeddedNullsTest, Empty) {
  ExpectEmpty(String16EmbeddedNulls());
  ExpectEmpty(String16EmbeddedNulls(nullptr));
  ExpectEmpty(String16EmbeddedNulls(nullptr, 0));
  ExpectEmpty(String16EmbeddedNulls(nullptr, 1));
  ExpectEmpty(String16EmbeddedNulls(L"", 0));
  ExpectEmpty(String16EmbeddedNulls(std::vector<wchar_t>{}));
  ExpectEmpty(String16EmbeddedNulls(base::string16()));
  ExpectEmpty(String16EmbeddedNulls(base::string16(L"")));
  ExpectEmpty(String16EmbeddedNulls(base::StringPiece16()));
  ExpectEmpty(String16EmbeddedNulls(base::StringPiece16(L"")));
}

TEST_F(String16EmbeddedNullsTest, FromWCharArray) {
  String16EmbeddedNulls str(vec_.data(), vec_.size());
  base::StringPiece16 sp = str.CastAsStringPiece16();
  EXPECT_EQ(FormatVectorWithNulls(vec_), FormatStringPiece16(sp));
}

TEST_F(String16EmbeddedNullsTest, FromVector) {
  String16EmbeddedNulls str(vec_);
  base::StringPiece16 sp = str.CastAsStringPiece16();
  EXPECT_EQ(FormatVectorWithNulls(vec_), FormatStringPiece16(sp));
}

TEST_F(String16EmbeddedNullsTest, FromString16) {
  String16EmbeddedNulls str(base::string16(vec_.data(), vec_.size()));
  base::StringPiece16 sp = str.CastAsStringPiece16();
  EXPECT_EQ(FormatVectorWithNulls(vec_), FormatStringPiece16(sp));
}

TEST_F(String16EmbeddedNullsTest, FromStringPiece16) {
  String16EmbeddedNulls str(base::StringPiece16(vec_.data(), vec_.size()));
  base::StringPiece16 sp = str.CastAsStringPiece16();
  EXPECT_EQ(FormatVectorWithNulls(vec_), FormatStringPiece16(sp));
}

TEST_F(String16EmbeddedNullsTest, CopyConstructor) {
  String16EmbeddedNulls str(vec_.data(), vec_.size());
  String16EmbeddedNulls copied_by_constructor(str);
  EXPECT_EQ(str.CastAsStringPiece16(),
            copied_by_constructor.CastAsStringPiece16());
}

TEST_F(String16EmbeddedNullsTest, AssigmentOperator) {
  String16EmbeddedNulls str(vec_.data(), vec_.size());

  String16EmbeddedNulls other;
  EXPECT_EQ(base::StringPiece16(), other.CastAsStringPiece16());
  EXPECT_NE(str.CastAsStringPiece16(), other.CastAsStringPiece16());

  other = str;
  EXPECT_EQ(str.CastAsStringPiece16(), other.CastAsStringPiece16());
}

}  // namespace

}  // namespace chrome_cleaner
