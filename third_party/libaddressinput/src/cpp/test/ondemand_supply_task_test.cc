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

#include "ondemand_supply_task.h"

#include <libaddressinput/callback.h>
#include <libaddressinput/null_storage.h>
#include <libaddressinput/supplier.h>

#include <cstddef>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include <gtest/gtest.h>

#include "lookup_key.h"
#include "mock_source.h"
#include "retriever.h"
#include "rule.h"
#include "util/size.h"

namespace {

using i18n::addressinput::BuildCallback;
using i18n::addressinput::LookupKey;
using i18n::addressinput::MockSource;
using i18n::addressinput::NullStorage;
using i18n::addressinput::OndemandSupplyTask;
using i18n::addressinput::Retriever;
using i18n::addressinput::Rule;
using i18n::addressinput::Supplier;

class OndemandSupplyTaskTest : public testing::Test {
 public:
  OndemandSupplyTaskTest(const OndemandSupplyTaskTest&) = delete;
  OndemandSupplyTaskTest& operator=(const OndemandSupplyTaskTest&) = delete;

 protected:
  OndemandSupplyTaskTest()
      : success_(true),
        lookup_key_(),
        rule_(),
        called_(false),
        source_(new MockSource),
        rule_cache_(),
        retriever_(new Retriever(source_, new NullStorage)),
        supplied_(BuildCallback(this, &OndemandSupplyTaskTest::Supplied)),
        task_(new OndemandSupplyTask(lookup_key_, &rule_cache_, *supplied_)) {}

  ~OndemandSupplyTaskTest() override {
    for (std::map<std::string, const Rule*>::const_iterator
         it = rule_cache_.begin(); it != rule_cache_.end(); ++it) {
      delete it->second;
    }
  }

  void Queue(const std::string& key) { task_->Queue(key); }

  void Retrieve() { task_->Retrieve(*retriever_); }

  bool success_;  // Expected status from MockSource.
  LookupKey lookup_key_;  // Stub.
  const Rule* rule_[size(LookupKey::kHierarchy)];
  bool called_;
  MockSource* const source_;

 private:
  void Supplied(bool success,
                const LookupKey& lookup_key,
                const Supplier::RuleHierarchy& hierarchy) {
    ASSERT_EQ(success_, success);
    ASSERT_EQ(&lookup_key_, &lookup_key);
    ASSERT_EQ(&task_->hierarchy_, &hierarchy);
    std::memcpy(rule_, hierarchy.rule, sizeof rule_);
    called_ = true;
  }

  std::map<std::string, const Rule*> rule_cache_;
  const std::unique_ptr<Retriever> retriever_;
  const std::unique_ptr<const Supplier::Callback> supplied_;
  OndemandSupplyTask* const task_;
};

TEST_F(OndemandSupplyTaskTest, Empty) {
  ASSERT_NO_FATAL_FAILURE(Retrieve());
  ASSERT_TRUE(called_);
  EXPECT_TRUE(rule_[0] == nullptr);
  EXPECT_TRUE(rule_[1] == nullptr);
  EXPECT_TRUE(rule_[2] == nullptr);
  EXPECT_TRUE(rule_[3] == nullptr);
}

TEST_F(OndemandSupplyTaskTest, Invalid) {
  Queue("data/XA");

  success_ = false;

  ASSERT_NO_FATAL_FAILURE(Retrieve());
  ASSERT_TRUE(called_);
}

TEST_F(OndemandSupplyTaskTest, Valid) {
  source_->data_.insert(std::make_pair("data/XA", R"({"id":"data/XA"})"));

  Queue("data/XA");

  ASSERT_NO_FATAL_FAILURE(Retrieve());
  ASSERT_TRUE(called_);
  EXPECT_TRUE(rule_[0] != nullptr);
  EXPECT_TRUE(rule_[1] == nullptr);
  EXPECT_TRUE(rule_[2] == nullptr);
  EXPECT_TRUE(rule_[3] == nullptr);

  EXPECT_EQ("data/XA", rule_[0]->GetId());

  // All rules on the COUNTRY level inherit from the default rule.
  EXPECT_FALSE(rule_[0]->GetFormat().empty());
  EXPECT_FALSE(rule_[0]->GetRequired().empty());
  EXPECT_TRUE(rule_[0]->GetPostalCodeMatcher() == nullptr);
}

TEST_F(OndemandSupplyTaskTest, ValidHierarchy) {
  source_->data_.insert(
      std::make_pair("data/XA", R"({"id":"data/XA"})"));
  source_->data_.insert(
      std::make_pair("data/XA/aa", R"({"id":"data/XA/aa"})"));
  source_->data_.insert(
      std::make_pair("data/XA/aa/bb", R"({"id":"data/XA/aa/bb"})"));
  source_->data_.insert(
      std::make_pair("data/XA/aa/bb/cc", R"({"id":"data/XA/aa/bb/cc"})"));

  Queue("data/XA");
  Queue("data/XA/aa");
  Queue("data/XA/aa/bb");
  Queue("data/XA/aa/bb/cc");

  ASSERT_NO_FATAL_FAILURE(Retrieve());
  ASSERT_TRUE(called_);
  EXPECT_TRUE(rule_[0] != nullptr);
  EXPECT_TRUE(rule_[1] != nullptr);
  EXPECT_TRUE(rule_[2] != nullptr);
  EXPECT_TRUE(rule_[3] != nullptr);

  EXPECT_EQ("data/XA", rule_[0]->GetId());
  EXPECT_EQ("data/XA/aa", rule_[1]->GetId());
  EXPECT_EQ("data/XA/aa/bb", rule_[2]->GetId());
  EXPECT_EQ("data/XA/aa/bb/cc", rule_[3]->GetId());

  // All rules on the COUNTRY level inherit from the default rule.
  EXPECT_FALSE(rule_[0]->GetFormat().empty());
  EXPECT_FALSE(rule_[0]->GetRequired().empty());

  // Only rules on the COUNTRY level inherit from the default rule.
  EXPECT_TRUE(rule_[1]->GetFormat().empty());
  EXPECT_TRUE(rule_[1]->GetRequired().empty());
  EXPECT_TRUE(rule_[2]->GetFormat().empty());
  EXPECT_TRUE(rule_[2]->GetRequired().empty());
  EXPECT_TRUE(rule_[3]->GetFormat().empty());
  EXPECT_TRUE(rule_[3]->GetRequired().empty());
}

TEST_F(OndemandSupplyTaskTest, InvalidJson1) {
  source_->data_.insert(std::make_pair("data/XA", ":"));

  success_ = false;

  Queue("data/XA");

  ASSERT_NO_FATAL_FAILURE(Retrieve());
  ASSERT_TRUE(called_);
}

TEST_F(OndemandSupplyTaskTest, InvalidJson2) {
  source_->data_.insert(std::make_pair("data/XA", R"({"id":"data/XA"})"));
  source_->data_.insert(std::make_pair("data/XA/aa", ":"));

  success_ = false;

  Queue("data/XA");
  Queue("data/XA/aa");

  ASSERT_NO_FATAL_FAILURE(Retrieve());
  ASSERT_TRUE(called_);
}

TEST_F(OndemandSupplyTaskTest, EmptyJsonJustMeansServerKnowsNothingAboutKey) {
  source_->data_.insert(std::make_pair("data/XA", R"({"id":"data/XA"})"));
  source_->data_.insert(std::make_pair("data/XA/aa", "{}"));

  Queue("data/XA");
  Queue("data/XA/aa");

  ASSERT_NO_FATAL_FAILURE(Retrieve());
  ASSERT_TRUE(called_);
  EXPECT_TRUE(rule_[0] != nullptr);
  EXPECT_TRUE(rule_[1] == nullptr);
  EXPECT_TRUE(rule_[2] == nullptr);
  EXPECT_TRUE(rule_[3] == nullptr);

  EXPECT_EQ("data/XA", rule_[0]->GetId());
}

TEST_F(OndemandSupplyTaskTest, IfCountryFailsAllFails) {
  source_->data_.insert(std::make_pair("data/XA/aa", R"({"id":"data/XA/aa"})"));

  success_ = false;

  Queue("data/XA");
  Queue("data/XA/aa");

  ASSERT_NO_FATAL_FAILURE(Retrieve());
  ASSERT_TRUE(called_);
}

}  // namespace
