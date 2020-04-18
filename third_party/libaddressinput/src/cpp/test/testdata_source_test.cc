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

#include "testdata_source.h"

#include <libaddressinput/callback.h>
#include <libaddressinput/source.h>

#include <cstddef>
#include <memory>
#include <string>

#include <gtest/gtest.h>

#include "region_data_constants.h"

namespace {

using i18n::addressinput::BuildCallback;
using i18n::addressinput::RegionDataConstants;
using i18n::addressinput::Source;
using i18n::addressinput::TestdataSource;
using i18n::addressinput::kDataFileName;

// Tests for TestdataSource object.
class TestdataSourceTest : public testing::TestWithParam<std::string> {
 public:
  TestdataSourceTest(const TestdataSourceTest&) = delete;
  TestdataSourceTest& operator=(const TestdataSourceTest&) = delete;

 protected:
  TestdataSourceTest()
      : source_(false),
        source_with_path_(false, kDataFileName),
        aggregate_source_(true),
        aggregate_source_with_path_(true, kDataFileName),
        success_(false),
        key_(),
        data_(),
        data_ready_(BuildCallback(this, &TestdataSourceTest::OnDataReady)) {}

  TestdataSource source_;
  TestdataSource source_with_path_;
  TestdataSource aggregate_source_;
  TestdataSource aggregate_source_with_path_;
  bool success_;
  std::string key_;
  std::string data_;
  const std::unique_ptr<const Source::Callback> data_ready_;

 private:
  void OnDataReady(bool success, const std::string& key, std::string* data) {
    ASSERT_FALSE(success && data == nullptr);
    success_ = success;
    key_ = key;
    if (data != nullptr) {
      data_ = *data;
      delete data;
    }
  }
};

// Returns testing::AssertionSuccess if |data| is valid callback data for
// |key|.
testing::AssertionResult DataIsValid(const std::string& data,
                                     const std::string& key) {
  if (data.empty()) {
    return testing::AssertionFailure() << "empty data";
  }

  std::string expected_data_begin = R"({"id":")" + key + R"(")";
  if (data.compare(0, expected_data_begin.length(), expected_data_begin) != 0) {
    return testing::AssertionFailure() << data << " does not begin with "
                                       << expected_data_begin;
  }

  // Verify that the data ends on "}.
  static const char kDataEnd[] = "\"}";
  static const size_t kDataEndLength = sizeof kDataEnd - 1;
  if (data.compare(data.length() - kDataEndLength,
                   kDataEndLength,
                   kDataEnd,
                   kDataEndLength) != 0) {
    return testing::AssertionFailure() << data << " does not end with "
                                       << kDataEnd;
  }

  return testing::AssertionSuccess();
}

// Verifies that TestdataSource gets valid data for a region code.
TEST_P(TestdataSourceTest, TestdataSourceHasValidDataForRegion) {
  std::string key = "data/" + GetParam();
  source_.Get(key, *data_ready_);

  EXPECT_TRUE(success_);
  EXPECT_EQ(key, key_);
  EXPECT_TRUE(DataIsValid(data_, key));
};

// Verifies that TestdataSource gets valid data for a region code.
TEST_P(TestdataSourceTest, TestdataSourceWithPathHasValidDataForRegion) {
  std::string key = "data/" + GetParam();
  source_with_path_.Get(key, *data_ready_);

  EXPECT_TRUE(success_);
  EXPECT_EQ(key, key_);
  EXPECT_TRUE(DataIsValid(data_, key));
};

// Returns testing::AssertionSuccess if |data| is valid aggregated callback
// data for |key|.
testing::AssertionResult AggregateDataIsValid(const std::string& data,
                                              const std::string& key) {
  if (data.empty()) {
    return testing::AssertionFailure() << "empty data";
  }

  std::string expected_data_begin = "{\"" + key;
  if (data.compare(0, expected_data_begin.length(), expected_data_begin) != 0) {
    return testing::AssertionFailure() << data << " does not begin with "
                                       << expected_data_begin;
  }

  // Verify that the data ends on "}}.
  static const char kDataEnd[] = "\"}}";
  static const size_t kDataEndLength = sizeof kDataEnd - 1;
  if (data.compare(data.length() - kDataEndLength,
                   kDataEndLength,
                   kDataEnd,
                   kDataEndLength) != 0) {
    return testing::AssertionFailure() << data << " does not end with "
                                       << kDataEnd;
  }

  return testing::AssertionSuccess();
}

// Verifies that TestdataSource gets valid aggregated data for a region code.
TEST_P(TestdataSourceTest, TestdataSourceHasValidAggregatedDataForRegion) {
  std::string key = "data/" + GetParam();
  aggregate_source_.Get(key, *data_ready_);

  EXPECT_TRUE(success_);
  EXPECT_EQ(key, key_);
  EXPECT_TRUE(AggregateDataIsValid(data_, key));
};

// Verifies that TestdataSource gets valid aggregated data for a region code.
TEST_P(TestdataSourceTest,
    TestdataSourceWithPathHasValidAggregatedDataForRegion) {

  std::string key = "data/" + GetParam();
  aggregate_source_with_path_.Get(key, *data_ready_);

  EXPECT_TRUE(success_);
  EXPECT_EQ(key, key_);
  EXPECT_TRUE(AggregateDataIsValid(data_, key));
};

// Test all region codes.
INSTANTIATE_TEST_CASE_P(
    AllRegions, TestdataSourceTest,
    testing::ValuesIn(RegionDataConstants::GetRegionCodes()));

// Verifies that the key "data" also contains valid data.
TEST_F(TestdataSourceTest, GetExistingData) {
  static const std::string kKey = "data";
  source_.Get(kKey, *data_ready_);

  EXPECT_TRUE(success_);
  EXPECT_EQ(kKey, key_);
  EXPECT_TRUE(DataIsValid(data_, kKey));
}

// Verifies that requesting a missing key will return "{}".
TEST_F(TestdataSourceTest, GetMissingKeyReturnsEmptyDictionary) {
  static const std::string kJunkKey = "junk";
  source_.Get(kJunkKey, *data_ready_);

  EXPECT_TRUE(success_);
  EXPECT_EQ(kJunkKey, key_);
  EXPECT_EQ("{}", data_);
}

// Verifies that aggregate requesting of a missing key will also return "{}".
TEST_F(TestdataSourceTest, AggregateGetMissingKeyReturnsEmptyDictionary) {
  static const std::string kJunkKey = "junk";
  aggregate_source_.Get(kJunkKey, *data_ready_);

  EXPECT_TRUE(success_);
  EXPECT_EQ(kJunkKey, key_);
  EXPECT_EQ("{}", data_);
}

// Verifies that requesting an empty key will return "{}".
TEST_F(TestdataSourceTest, GetEmptyKeyReturnsEmptyDictionary) {
  static const std::string kEmptyKey;
  source_.Get(kEmptyKey, *data_ready_);

  EXPECT_TRUE(success_);
  EXPECT_EQ(kEmptyKey, key_);
  EXPECT_EQ("{}", data_);
}

}  // namespace
