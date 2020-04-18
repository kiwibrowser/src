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

#include "retriever.h"

#include <libaddressinput/callback.h>
#include <libaddressinput/null_storage.h>
#include <libaddressinput/storage.h>

#include <cstddef>
#include <memory>
#include <string>

#include <gtest/gtest.h>

#include "mock_source.h"
#include "testdata_source.h"

#define CHECKSUM "dd63dafcbd4d5b28badfcaf86fb6fcdb"
#define DATA "{'foo': 'bar'}"
#define OLD_TIMESTAMP "0"

namespace {

using i18n::addressinput::BuildCallback;
using i18n::addressinput::MockSource;
using i18n::addressinput::NullStorage;
using i18n::addressinput::Retriever;
using i18n::addressinput::Storage;
using i18n::addressinput::TestdataSource;

const char kKey[] = "data/CA/AB--fr";

// Empty data that the source can return.
const char kEmptyData[] = "{}";

// The value of the data that the stale storage returns.
const char kStaleData[] = DATA;

// The actual data that the stale storage returns.
const char kStaleWrappedData[] = "timestamp=" OLD_TIMESTAMP "\n"
                                 "checksum=" CHECKSUM "\n"
                                 DATA;

// Tests for Retriever object.
class RetrieverTest : public testing::Test {
 public:
  RetrieverTest(const RetrieverTest&) = delete;
  RetrieverTest& operator=(const RetrieverTest&) = delete;

 protected:
  RetrieverTest()
      : retriever_(new TestdataSource(false), new NullStorage),
        success_(false),
        key_(),
        data_(),
        data_ready_(BuildCallback(this, &RetrieverTest::OnDataReady)) {}

  Retriever retriever_;
  bool success_;
  std::string key_;
  std::string data_;
  const std::unique_ptr<const Retriever::Callback> data_ready_;

 private:
  void OnDataReady(bool success,
                   const std::string& key,
                   const std::string& data) {
    success_ = success;
    key_ = key;
    data_ = data;
  }
};

TEST_F(RetrieverTest, RetrieveData) {
  retriever_.Retrieve(kKey, *data_ready_);

  EXPECT_TRUE(success_);
  EXPECT_EQ(kKey, key_);
  EXPECT_FALSE(data_.empty());
  EXPECT_NE(kEmptyData, data_);
}

TEST_F(RetrieverTest, ReadDataFromStorage) {
  retriever_.Retrieve(kKey, *data_ready_);
  retriever_.Retrieve(kKey, *data_ready_);

  EXPECT_TRUE(success_);
  EXPECT_EQ(kKey, key_);
  EXPECT_FALSE(data_.empty());
  EXPECT_NE(kEmptyData, data_);
}

TEST_F(RetrieverTest, MissingKeyReturnsEmptyData) {
  static const char kMissingKey[] = "junk";

  retriever_.Retrieve(kMissingKey, *data_ready_);

  EXPECT_TRUE(success_);
  EXPECT_EQ(kMissingKey, key_);
  EXPECT_EQ(kEmptyData, data_);
}

TEST_F(RetrieverTest, FaultySource) {
  // An empty MockSource will fail for any request.
  Retriever bad_retriever(new MockSource, new NullStorage);

  bad_retriever.Retrieve(kKey, *data_ready_);

  EXPECT_FALSE(success_);
  EXPECT_EQ(kKey, key_);
  EXPECT_TRUE(data_.empty());
}

// The storage that always returns stale data.
class StaleStorage : public Storage {
 public:
  StaleStorage(const StaleStorage&) = delete;
  StaleStorage& operator=(const StaleStorage&) = delete;

  StaleStorage() : data_updated_(false) {}
  ~StaleStorage() override {}

  // Storage implementation.
  void Get(const std::string& key, const Callback& data_ready) const override {
    data_ready(true, key, new std::string(kStaleWrappedData));
  }

  void Put(const std::string& key, std::string* value) override {
    ASSERT_TRUE(value != nullptr);
    data_updated_ = true;
    delete value;
  }

  bool data_updated_;
};

TEST_F(RetrieverTest, UseStaleDataWhenSourceFails) {
  // Owned by |resilient_retriever|.
  StaleStorage* stale_storage = new StaleStorage;
  // An empty MockSource will fail for any request.
  Retriever resilient_retriever(new MockSource, stale_storage);

  resilient_retriever.Retrieve(kKey, *data_ready_);

  EXPECT_TRUE(success_);
  EXPECT_EQ(kKey, key_);
  EXPECT_EQ(kStaleData, data_);
  EXPECT_FALSE(stale_storage->data_updated_);
}

TEST_F(RetrieverTest, DoNotUseStaleDataWhenSourceSucceeds) {
  // Owned by |resilient_retriever|.
  StaleStorage* stale_storage = new StaleStorage;
  Retriever resilient_retriever(new TestdataSource(false), stale_storage);

  resilient_retriever.Retrieve(kKey, *data_ready_);

  EXPECT_TRUE(success_);
  EXPECT_EQ(kKey, key_);
  EXPECT_FALSE(data_.empty());
  EXPECT_NE(kEmptyData, data_);
  EXPECT_NE(kStaleData, data_);
  EXPECT_TRUE(stale_storage->data_updated_);
}

}  // namespace
