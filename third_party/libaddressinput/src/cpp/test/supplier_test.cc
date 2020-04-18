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

#include <libaddressinput/supplier.h>

#include <libaddressinput/address_data.h>
#include <libaddressinput/callback.h>
#include <libaddressinput/null_storage.h>
#include <libaddressinput/ondemand_supplier.h>
#include <libaddressinput/preload_supplier.h>

#include <cstddef>
#include <cstring>
#include <memory>
#include <string>

#include <gtest/gtest.h>

#include "lookup_key.h"
#include "rule.h"
#include "testdata_source.h"
#include "util/size.h"

namespace {

using i18n::addressinput::AddressData;
using i18n::addressinput::BuildCallback;
using i18n::addressinput::LookupKey;
using i18n::addressinput::NullStorage;
using i18n::addressinput::OndemandSupplier;
using i18n::addressinput::PreloadSupplier;
using i18n::addressinput::Rule;
using i18n::addressinput::Supplier;
using i18n::addressinput::TestdataSource;

class SupplierWrapper {
 public:
  virtual ~SupplierWrapper() {}
  virtual void Supply(const LookupKey& lookup_key,
                      const Supplier::Callback& supplied) = 0;
};

class OndemandSupplierWrapper : public SupplierWrapper {
 public:
  OndemandSupplierWrapper(const OndemandSupplierWrapper&) = delete;
  OndemandSupplierWrapper& operator=(const OndemandSupplierWrapper&) = delete;

  static SupplierWrapper* Build() { return new OndemandSupplierWrapper; }

  void Supply(const LookupKey& lookup_key,
              const Supplier::Callback& supplied) override {
    ondemand_supplier_.Supply(lookup_key, supplied);
  }

 private:
  OndemandSupplierWrapper()
      : ondemand_supplier_(new TestdataSource(false), new NullStorage) {}

  OndemandSupplier ondemand_supplier_;
};

class PreloadSupplierWrapper : public SupplierWrapper {
 public:
  PreloadSupplierWrapper(const PreloadSupplierWrapper&) = delete;
  PreloadSupplierWrapper& operator=(const PreloadSupplierWrapper&) = delete;

  static SupplierWrapper* Build() { return new PreloadSupplierWrapper; }

  void Supply(const LookupKey& lookup_key,
              const Supplier::Callback& supplied) override {
    const std::string& region_code = lookup_key.GetRegionCode();
    if (!region_code.empty() && !preload_supplier_.IsLoaded(region_code)) {
      preload_supplier_.LoadRules(region_code, *loaded_);
    }
    preload_supplier_.Supply(lookup_key, supplied);
  }

 private:
  PreloadSupplierWrapper()
      : preload_supplier_(new TestdataSource(true), new NullStorage),
        loaded_(BuildCallback(this, &PreloadSupplierWrapper::Loaded)) {}

  void Loaded(bool success, const std::string&, int) { ASSERT_TRUE(success); }

  PreloadSupplier preload_supplier_;
  const std::unique_ptr<const PreloadSupplier::Callback> loaded_;
};

class SupplierTest : public testing::TestWithParam<SupplierWrapper* (*)()> {
 public:
  SupplierTest(const SupplierTest&) = delete;
  SupplierTest& operator=(const SupplierTest&) = delete;

 protected:
  SupplierTest()
      : address_(),
        rule_(),
        called_(false),
        lookup_key_(),
        supplier_wrapper_((*GetParam())()),
        supplied_(BuildCallback(this, &SupplierTest::Supplied)) {}

  void Supply() {
    lookup_key_.FromAddress(address_);
    supplier_wrapper_->Supply(lookup_key_, *supplied_);
  }

  AddressData address_;
  const Rule* rule_[size(LookupKey::kHierarchy)];
  bool called_;

 private:
  void Supplied(bool success,
                const LookupKey& lookup_key,
                const Supplier::RuleHierarchy& hierarchy) {
    ASSERT_TRUE(success);
    ASSERT_EQ(&lookup_key_, &lookup_key);
    std::memcpy(rule_, hierarchy.rule, sizeof rule_);
    called_ = true;
  }

  LookupKey lookup_key_;
  const std::unique_ptr<SupplierWrapper> supplier_wrapper_;
  const std::unique_ptr<const Supplier::Callback> supplied_;
};

INSTANTIATE_TEST_CASE_P(OndemandSupplier,
                        SupplierTest,
                        testing::Values(&OndemandSupplierWrapper::Build));

INSTANTIATE_TEST_CASE_P(PreloadSupplier,
                        SupplierTest,
                        testing::Values(&PreloadSupplierWrapper::Build));

TEST_P(SupplierTest, Invalid) {
  address_.region_code = "QZ";

  ASSERT_NO_FATAL_FAILURE(Supply());
  ASSERT_TRUE(called_);
  EXPECT_TRUE(rule_[0] == nullptr);
  EXPECT_TRUE(rule_[1] == nullptr);
  EXPECT_TRUE(rule_[2] == nullptr);
  EXPECT_TRUE(rule_[3] == nullptr);
}

TEST_P(SupplierTest, Valid) {
  address_.region_code = "SE";

  ASSERT_NO_FATAL_FAILURE(Supply());
  ASSERT_TRUE(called_);
  EXPECT_TRUE(rule_[0] != nullptr);
  EXPECT_TRUE(rule_[1] == nullptr);
  EXPECT_TRUE(rule_[2] == nullptr);
  EXPECT_TRUE(rule_[3] == nullptr);
  EXPECT_EQ("data/SE", rule_[0]->GetId());
  EXPECT_FALSE(rule_[0]->GetRequired().empty());
  EXPECT_FALSE(rule_[0]->GetFormat().empty());
  EXPECT_TRUE(rule_[0]->GetPostalCodeMatcher() != nullptr);
}

TEST_P(SupplierTest, KeyDepthEqualsMaxDepth) {
  address_.region_code = "HK";
  address_.administrative_area = u8"九龍";

  ASSERT_NO_FATAL_FAILURE(Supply());
  ASSERT_TRUE(called_);
  EXPECT_TRUE(rule_[0] != nullptr);
  EXPECT_TRUE(rule_[1] != nullptr);
  EXPECT_TRUE(rule_[2] == nullptr);
  EXPECT_TRUE(rule_[3] == nullptr);
}

TEST_P(SupplierTest, KeyDepthLargerThanMaxDepth) {
  address_.region_code = "HK";
  address_.administrative_area = u8"九龍";
  address_.locality = "bbb";  // Ignored!

  ASSERT_NO_FATAL_FAILURE(Supply());
  ASSERT_TRUE(called_);
  EXPECT_TRUE(rule_[0] != nullptr);
  EXPECT_TRUE(rule_[1] != nullptr);
  EXPECT_TRUE(rule_[2] == nullptr);
  EXPECT_TRUE(rule_[3] == nullptr);
}

TEST_P(SupplierTest, KeyDepthSmallerThanMaxDepth) {
  address_.region_code = "HK";

  ASSERT_NO_FATAL_FAILURE(Supply());
  ASSERT_TRUE(called_);
  EXPECT_TRUE(rule_[0] != nullptr);
  EXPECT_TRUE(rule_[1] == nullptr);
  EXPECT_TRUE(rule_[2] == nullptr);
  EXPECT_TRUE(rule_[3] == nullptr);
}

TEST_P(SupplierTest, KeyDepth0) {
  address_.region_code = "CN";

  ASSERT_NO_FATAL_FAILURE(Supply());
  ASSERT_TRUE(called_);
  EXPECT_TRUE(rule_[0] != nullptr);
  EXPECT_TRUE(rule_[1] == nullptr);
  EXPECT_TRUE(rule_[2] == nullptr);
  EXPECT_TRUE(rule_[3] == nullptr);
}

TEST_P(SupplierTest, KeyDepth1) {
  address_.region_code = "CN";
  address_.administrative_area = u8"新疆维吾尔自治区";

  ASSERT_NO_FATAL_FAILURE(Supply());
  ASSERT_TRUE(called_);
  EXPECT_TRUE(rule_[0] != nullptr);
  EXPECT_TRUE(rule_[1] != nullptr);
  EXPECT_TRUE(rule_[2] == nullptr);
  EXPECT_TRUE(rule_[3] == nullptr);
}

TEST_P(SupplierTest, KeyDepth2) {
  address_.region_code = "CN";
  address_.administrative_area = u8"新疆维吾尔自治区";
  address_.locality = u8"喀什地区";

  ASSERT_NO_FATAL_FAILURE(Supply());
  ASSERT_TRUE(called_);
  EXPECT_TRUE(rule_[0] != nullptr);
  EXPECT_TRUE(rule_[1] != nullptr);
  EXPECT_TRUE(rule_[2] != nullptr);
  EXPECT_TRUE(rule_[3] == nullptr);
}

TEST_P(SupplierTest, KeyDepth3) {
  address_.region_code = "CN";
  address_.administrative_area = u8"新疆维吾尔自治区";
  address_.locality = u8"喀什地区";
  address_.dependent_locality = u8"喀什市";

  ASSERT_NO_FATAL_FAILURE(Supply());
  ASSERT_TRUE(called_);
  EXPECT_TRUE(rule_[0] != nullptr);
  EXPECT_TRUE(rule_[1] != nullptr);
  EXPECT_TRUE(rule_[2] != nullptr);
  EXPECT_TRUE(rule_[3] != nullptr);
}

TEST_P(SupplierTest, RuleCache) {
  address_.region_code = "US";
  address_.administrative_area = "CA";

  ASSERT_NO_FATAL_FAILURE(Supply());
  ASSERT_TRUE(called_);
  EXPECT_TRUE(rule_[0] != nullptr);
  EXPECT_TRUE(rule_[1] != nullptr);
  EXPECT_TRUE(rule_[2] == nullptr);
  EXPECT_TRUE(rule_[3] == nullptr);

  // Make a copy of the currently returned pointers to the Rule objects (stored
  // in the OndemandSupplier cache) and verify that calling Supply() again with
  // the same LookupKey returns the same pointers again (and doesn't create any
  // new Rule objects instead).

  const Rule* rule[size(LookupKey::kHierarchy)];
  std::memcpy(rule, rule_, sizeof rule);

  called_ = false;
  ASSERT_NO_FATAL_FAILURE(Supply());
  ASSERT_TRUE(called_);
  EXPECT_EQ(rule[0], rule_[0]);
  EXPECT_EQ(rule[1], rule_[1]);
  EXPECT_EQ(rule[2], rule_[2]);
  EXPECT_EQ(rule[3], rule_[3]);
}

}  // namespace
