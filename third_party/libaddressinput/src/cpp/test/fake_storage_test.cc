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

#include "fake_storage.h"

#include <libaddressinput/callback.h>
#include <libaddressinput/storage.h>

#include <cstddef>
#include <memory>
#include <string>

#include <gtest/gtest.h>

namespace {

using i18n::addressinput::BuildCallback;
using i18n::addressinput::FakeStorage;
using i18n::addressinput::Storage;

// Tests for FakeStorage object.
class FakeStorageTest : public testing::Test {
 public:
  FakeStorageTest(const FakeStorageTest&) = delete;
  FakeStorageTest& operator=(const FakeStorageTest&) = delete;

 protected:
  FakeStorageTest()
      : storage_(),
        success_(false),
        key_(),
        data_(),
        data_ready_(BuildCallback(this, &FakeStorageTest::OnDataReady)) {}

  FakeStorage storage_;
  bool success_;
  std::string key_;
  std::string data_;
  const std::unique_ptr<const Storage::Callback> data_ready_;

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

TEST_F(FakeStorageTest, GetWithoutPutReturnsEmptyData) {
  storage_.Get("key", *data_ready_);

  EXPECT_FALSE(success_);
  EXPECT_EQ("key", key_);
  EXPECT_TRUE(data_.empty());
}

TEST_F(FakeStorageTest, GetReturnsWhatWasPut) {
  storage_.Put("key", new std::string("value"));
  storage_.Get("key", *data_ready_);

  EXPECT_TRUE(success_);
  EXPECT_EQ("key", key_);
  EXPECT_EQ("value", data_);
}

TEST_F(FakeStorageTest, SecondPutOverwritesData) {
  storage_.Put("key", new std::string("bad-value"));
  storage_.Put("key", new std::string("good-value"));
  storage_.Get("key", *data_ready_);

  EXPECT_TRUE(success_);
  EXPECT_EQ("key", key_);
  EXPECT_EQ("good-value", data_);
}

}  // namespace
