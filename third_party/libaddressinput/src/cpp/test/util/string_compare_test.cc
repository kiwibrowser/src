// Copyright (C) 2013 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "util/string_compare.h"

#include <string>

#include <gtest/gtest.h>

namespace {

using i18n::addressinput::StringCompare;

struct TestCase {
  TestCase(const std::string& left,
           const std::string& right,
           bool should_be_equal,
           bool should_be_less)
      : left(left),
        right(right),
        should_be_equal(should_be_equal),
        should_be_less(should_be_less) {}

  ~TestCase() {}

  std::string left;
  std::string right;
  bool should_be_equal;
  bool should_be_less;
};

class StringCompareTest : public testing::TestWithParam<TestCase> {
 public:
  StringCompareTest(const StringCompareTest&) = delete;
  StringCompareTest& operator=(const StringCompareTest&) = delete;

 protected:
  StringCompareTest() {}
  StringCompare compare_;
};

TEST_P(StringCompareTest, CorrectComparison) {
  if (GetParam().should_be_equal) {
    EXPECT_TRUE(compare_.NaturalEquals(GetParam().left, GetParam().right));
  } else {
    EXPECT_FALSE(compare_.NaturalEquals(GetParam().left, GetParam().right));
  }
}

TEST_P(StringCompareTest, CorrectLess) {
  if (GetParam().should_be_less) {
    EXPECT_TRUE(compare_.NaturalLess(GetParam().left, GetParam().right));
  } else {
    EXPECT_FALSE(compare_.NaturalLess(GetParam().left, GetParam().right));
  }
}

INSTANTIATE_TEST_CASE_P(
    Comparisons, StringCompareTest,
    testing::Values(
        TestCase("foo", "foo", true, false),
        TestCase("foo", "FOO", true, false),
        TestCase("bar", "foo", false, true),
        TestCase(u8"강원도", u8"강원도", true, false),
        TestCase(u8"강원도", u8"대구광역시", false, true),
        TestCase(u8"ZÜRICH", u8"zürich", true, false),
        TestCase(u8"абв", u8"где", false, true),
        TestCase(u8"абв", u8"ГДЕ", false, true),
        TestCase(u8"где", u8"абв", false, false),
        TestCase(u8"где", u8"АБВ", false, false)));

}  // namespace
