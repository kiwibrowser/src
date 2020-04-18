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

#include "region_data_constants.h"

#include <string>

#include <gtest/gtest.h>

namespace {

using i18n::addressinput::RegionDataConstants;

// Tests for region codes, for example "ZA".
class RegionCodeTest : public testing::TestWithParam<std::string> {
 public:
  RegionCodeTest(const RegionCodeTest&) = delete;
  RegionCodeTest& operator=(const RegionCodeTest&) = delete;

 protected:
  RegionCodeTest() {}
};

// Verifies that a region code consists of two characters, for example "ZA".
TEST_P(RegionCodeTest, RegionCodeHasTwoCharacters) {
  EXPECT_EQ(2, GetParam().length());
}

// Test all region codes.
INSTANTIATE_TEST_CASE_P(
    AllRegionCodes, RegionCodeTest,
    testing::ValuesIn(RegionDataConstants::GetRegionCodes()));

// Returns AssertionSuccess if |data| begins with '{' and ends with '}'.
testing::AssertionResult HasCurlyBraces(const std::string& data) {
  if (data.empty()) {
    return testing::AssertionFailure() << "data is empty";
  }
  if (data[0] != '{') {
    return testing::AssertionFailure() << data << " does not start with '{'";
  }
  if (data[data.length() - 1] != '}') {
    return testing::AssertionFailure() << data << " does not end with '}'";
  }
  return testing::AssertionSuccess();
}

// Verifies that the default region data begins with '{' and ends with '}'.
TEST(DefaultRegionDataTest, DefaultRegionHasCurlyBraces) {
  EXPECT_TRUE(HasCurlyBraces(RegionDataConstants::GetDefaultRegionData()));
}

// Tests for region data, for example "{\"fmt\":\"%C%S\"}".
class RegionDataTest : public testing::TestWithParam<std::string> {
 public:
  RegionDataTest(const RegionDataTest&) = delete;
  RegionDataTest& operator=(const RegionDataTest&) = delete;

 protected:
  RegionDataTest() {}

  const std::string& GetData() const {
    return RegionDataConstants::GetRegionData(GetParam());
  }
};

// Verifies that a region data value begins with '{' and end with '}', for
// example "{\"fmt\":\"%C%S\"}".
TEST_P(RegionDataTest, RegionDataHasCurlyBraces) {
  EXPECT_TRUE(HasCurlyBraces(GetData()));
}

// Test all region data.
INSTANTIATE_TEST_CASE_P(
    AllRegionData, RegionDataTest,
    testing::ValuesIn(RegionDataConstants::GetRegionCodes()));

TEST(RegionDataConstantsTest, GetMaxLookupKeyDepth) {
  EXPECT_EQ(0, RegionDataConstants::GetMaxLookupKeyDepth("NZ"));
  EXPECT_EQ(1, RegionDataConstants::GetMaxLookupKeyDepth("KY"));
  EXPECT_EQ(2, RegionDataConstants::GetMaxLookupKeyDepth("US"));
  EXPECT_EQ(3, RegionDataConstants::GetMaxLookupKeyDepth("CN"));
}

}  // namespace
