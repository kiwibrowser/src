// Copyright (C) 2014 Google Inc.
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

#include <libaddressinput/null_storage.h>

#include <libaddressinput/callback.h>
#include <libaddressinput/storage.h>

#include <cstddef>
#include <memory>
#include <string>

#include <gtest/gtest.h>

namespace {

using i18n::addressinput::BuildCallback;
using i18n::addressinput::NullStorage;
using i18n::addressinput::Storage;

class NullStorageTest : public testing::Test {
 public:
  NullStorageTest(const NullStorageTest&) = delete;
  NullStorageTest& operator=(const NullStorageTest&) = delete;

 protected:
  NullStorageTest()
      : data_ready_(BuildCallback(this, &NullStorageTest::OnDataReady)) {}

  NullStorage storage_;
  bool success_;
  std::string key_;
  std::string data_;
  const std::unique_ptr<const Storage::Callback> data_ready_;

  static const char kKey[];

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

const char NullStorageTest::kKey[] = "foo";

TEST_F(NullStorageTest, Put) {
  // The Put() method should not do anything, so this test only tests that the
  // code compiles and that the call doesn't crash.
  storage_.Put(kKey, new std::string("bar"));
}

TEST_F(NullStorageTest, Get) {
  storage_.Get(kKey, *data_ready_);
  EXPECT_FALSE(success_);
  EXPECT_EQ(kKey, key_);
  EXPECT_TRUE(data_.empty());
}

}  // namespace
