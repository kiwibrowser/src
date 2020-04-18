// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/printing/external_printers.h"

#include <string>
#include <vector>

#include "base/bind.h"
#include "base/strings/stringprintf.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/scoped_task_environment.h"
#include "chrome/common/chrome_features.h"
#include "chromeos/printing/printer_configuration.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {
namespace {

// The number of printers in BulkPolicyContentsJson.
constexpr size_t kNumPrinters = 3;

// An example bulk printer configuration file.
constexpr char kBulkPolicyContentsJson[] = R"json(
[
  {
    "id": "First",
    "display_name": "LexaPrint",
    "description": "Laser on the test shelf",
    "manufacturer": "LexaPrint, Inc.",
    "model": "MS610de",
    "uri": "ipp://192.168.1.5",
    "ppd_resource": {
      "effective_model": "MS610de"
    }
  }, {
    "id": "Second",
    "display_name": "Color Laser",
    "description": "The printer next to the water cooler.",
    "manufacturer": "Printer Manufacturer",
    "model":"Color Laser 2004",
    "uri":"ipps://print-server.intranet.example.com:443/ipp/cl2k4",
    "uuid":"1c395fdb-5d93-4904-b246-b2c046e79d12",
    "ppd_resource":{
      "effective_manufacturer": "MakesPrinters",
      "effective_model": "ColorLaser2k4"
    }
  }, {
    "id": "Third",
    "display_name": "YaLP",
    "description": "Fancy Fancy Fancy",
    "manufacturer": "LexaPrint, Inc.",
    "model": "MS610de",
    "uri": "ipp://192.168.1.8",
    "ppd_resource": {
      "effective_manufacturer": "LexaPrint",
      "effective_model": "MS610de"
    }
  }
])json";

// A different bulk printer configuration file.
constexpr char kMoreContentsJson[] = R"json(
[
  {
    "id": "ThirdPrime",
    "display_name": "Printy McPrinter",
    "description": "Laser on the test shelf",
    "manufacturer": "CrosInc.",
    "model": "MS610de",
    "uri": "ipp://192.168.1.5",
    "ppd_resource": {
      "effective_model": "MS610de"
    }
  }
])json";

// Observer that counts the number of times it has been called.
class TestObserver : public ExternalPrinters::Observer {
 public:
  void OnPrintersChanged(
      bool valid,
      const std::map<const std::string, const Printer>& /* printers */)
      override {
    last_valid = valid;
    called++;
  }

  // Counts the number of times the observer is invoked.
  int called = 0;
  // Holds the most recent value of valid.
  bool last_valid = false;
};

class ExternalPrintersTest : public testing::Test {
 public:
  ExternalPrintersTest() : scoped_task_environment_() {
    scoped_feature_list_.InitAndEnableFeature(
        base::Feature(features::kBulkPrinters));
    external_printers_ = ExternalPrinters::Create();
  }
  ~ExternalPrintersTest() override {
    // Delete the printer before the task environment.
    external_printers_.reset();
  }

 protected:
  std::unique_ptr<ExternalPrinters> external_printers_;
  base::test::ScopedTaskEnvironment scoped_task_environment_;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

// Verify that we're initiall unset and empty.
TEST_F(ExternalPrintersTest, InitialConditions) {
  EXPECT_FALSE(external_printers_->IsPolicySet());
  EXPECT_TRUE(external_printers_->GetPrinters().empty());
}

// Verify that the object can be destroyed while parsing is in progress.
TEST_F(ExternalPrintersTest, DestructionIsSafe) {
  {
    std::unique_ptr<ExternalPrinters> printers = ExternalPrinters::Create();
    printers->SetAccessMode(ExternalPrinters::BLACKLIST_ONLY);
    printers->SetBlacklist({"Third"});
    printers->SetData(std::make_unique<std::string>(kBulkPolicyContentsJson));
    // Data is valid.  Computation is proceeding.
  }
  // printers is out of scope.  Destructor has run.  Pump the message queue to
  // see if anything strange happens.
  scoped_task_environment_.RunUntilIdle();
}

// Verifies that all IsPolicySet returns false until all necessary data is set.
TEST_F(ExternalPrintersTest, PolicyUnsetWithMissingData) {
  auto data = std::make_unique<std::string>(kBulkPolicyContentsJson);
  external_printers_->ClearData();
  external_printers_->SetData(std::move(data));

  // Waiting for AccessMode.
  scoped_task_environment_.RunUntilIdle();
  EXPECT_FALSE(external_printers_->IsPolicySet());

  external_printers_->SetAccessMode(ExternalPrinters::AccessMode::ALL_ACCESS);
  scoped_task_environment_.RunUntilIdle();
  EXPECT_TRUE(external_printers_->IsPolicySet());

  external_printers_->SetAccessMode(
      ExternalPrinters::AccessMode::WHITELIST_ONLY);
  scoped_task_environment_.RunUntilIdle();
  EXPECT_FALSE(external_printers_->IsPolicySet());  // Waiting for Whitelist.

  std::vector<std::string> whitelist = {"First", "Third"};
  external_printers_->SetWhitelist(whitelist);
  scoped_task_environment_.RunUntilIdle();
  EXPECT_TRUE(external_printers_->IsPolicySet());  // Everything is set.

  external_printers_->SetAccessMode(
      ExternalPrinters::AccessMode::BLACKLIST_ONLY);
  scoped_task_environment_.RunUntilIdle();
  EXPECT_FALSE(external_printers_->IsPolicySet());  // Blacklist needed now.

  std::vector<std::string> blacklist = {"Second"};
  external_printers_->SetBlacklist(blacklist);
  scoped_task_environment_.RunUntilIdle();
  EXPECT_TRUE(
      external_printers_->IsPolicySet());  // Blacklist was set.  Ready again.
}

// Verify printer list after all attributes have been set.
TEST_F(ExternalPrintersTest, AllPoliciesResultInPrinters) {
  auto data = std::make_unique<std::string>(kBulkPolicyContentsJson);
  external_printers_->SetAccessMode(ExternalPrinters::AccessMode::ALL_ACCESS);
  external_printers_->SetData(std::move(data));

  scoped_task_environment_.RunUntilIdle();
  const auto& printers = external_printers_->GetPrinters();
  EXPECT_TRUE(external_printers_->IsPolicySet());
  EXPECT_EQ(kNumPrinters, printers.size());
  EXPECT_EQ("LexaPrint", printers.at("First").display_name());
  EXPECT_EQ("Color Laser", printers.at("Second").display_name());
  EXPECT_EQ("YaLP", printers.at("Third").display_name());
}

// The external policy was cleared, results should be invalidated.
TEST_F(ExternalPrintersTest, PolicyClearedNowUnset) {
  auto data = std::make_unique<std::string>(kBulkPolicyContentsJson);
  external_printers_->SetAccessMode(ExternalPrinters::AccessMode::ALL_ACCESS);
  external_printers_->ClearData();
  external_printers_->SetData(std::move(data));

  scoped_task_environment_.RunUntilIdle();
  ASSERT_TRUE(external_printers_->IsPolicySet());

  external_printers_->ClearData();
  scoped_task_environment_.RunUntilIdle();
  EXPECT_FALSE(external_printers_->IsPolicySet());
  EXPECT_TRUE(external_printers_->GetPrinters().empty());
}

// Verify that the blacklist policy is applied correctly.  Printers in the
// blacklist policy should not be available.  Printers not in the blackslist
// should be available.
TEST_F(ExternalPrintersTest, BlacklistPolicySet) {
  auto data = std::make_unique<std::string>(kBulkPolicyContentsJson);
  external_printers_->ClearData();
  external_printers_->SetData(std::move(data));
  external_printers_->SetAccessMode(ExternalPrinters::BLACKLIST_ONLY);
  scoped_task_environment_.RunUntilIdle();
  EXPECT_FALSE(external_printers_->IsPolicySet());
  external_printers_->SetBlacklist({"Second", "Third"});
  scoped_task_environment_.RunUntilIdle();
  EXPECT_TRUE(external_printers_->IsPolicySet());

  scoped_task_environment_.RunUntilIdle();
  const auto& printers = external_printers_->GetPrinters();
  EXPECT_EQ(1U, printers.size());
  EXPECT_EQ("LexaPrint", printers.at("First").display_name());
}

// Verify that the whitelist policy is correctly applied.  Only printers
// available in the whitelist are available.
TEST_F(ExternalPrintersTest, WhitelistPolicySet) {
  auto data = std::make_unique<std::string>(kBulkPolicyContentsJson);
  external_printers_->ClearData();
  external_printers_->SetData(std::move(data));
  external_printers_->SetAccessMode(ExternalPrinters::WHITELIST_ONLY);
  scoped_task_environment_.RunUntilIdle();
  EXPECT_FALSE(external_printers_->IsPolicySet());
  external_printers_->SetWhitelist({"First"});

  scoped_task_environment_.RunUntilIdle();
  EXPECT_TRUE(external_printers_->IsPolicySet());
  const auto& printers = external_printers_->GetPrinters();
  EXPECT_EQ(1U, printers.size());
  EXPECT_EQ("LexaPrint", printers.at("First").display_name());
}

// Verify that switching from whitelist to blacklist behaves correctly.
TEST_F(ExternalPrintersTest, BlacklistToWhitelistSwap) {
  auto data = std::make_unique<std::string>(kBulkPolicyContentsJson);
  external_printers_->ClearData();
  external_printers_->SetData(std::move(data));
  external_printers_->SetAccessMode(ExternalPrinters::BLACKLIST_ONLY);
  external_printers_->SetWhitelist({"First"});
  external_printers_->SetBlacklist({"First"});

  // This should result in 2 printers.  But we're switching the mode anyway.

  external_printers_->SetAccessMode(ExternalPrinters::WHITELIST_ONLY);
  scoped_task_environment_.RunUntilIdle();
  EXPECT_TRUE(external_printers_->IsPolicySet());
  const auto& printers = external_printers_->GetPrinters();
  EXPECT_EQ(1U, printers.size());
  EXPECT_EQ("LexaPrint", printers.at("First").display_name());
}

// Verify that updated configurations are handled properly.
TEST_F(ExternalPrintersTest, MultipleUpdates) {
  auto data = std::make_unique<std::string>(kBulkPolicyContentsJson);
  external_printers_->ClearData();
  external_printers_->SetData(std::move(data));
  external_printers_->SetAccessMode(ExternalPrinters::ALL_ACCESS);
  // There will be 3 printers here.  But we don't want to wait for compuation to
  // complete to verify the final value gets used.

  auto new_data = std::make_unique<std::string>(kMoreContentsJson);
  external_printers_->SetData(std::move(new_data));
  scoped_task_environment_.RunUntilIdle();
  const auto& printers = external_printers_->GetPrinters();
  ASSERT_EQ(1U, printers.size());
  EXPECT_EQ("ThirdPrime", printers.at("ThirdPrime").id());
}

// Verifies that the observer is called at the expected times.
TEST_F(ExternalPrintersTest, ObserverTest) {
  TestObserver obs;
  external_printers_->AddObserver(&obs);

  external_printers_->SetAccessMode(ExternalPrinters::ALL_ACCESS);
  external_printers_->SetWhitelist(std::vector<std::string>());
  external_printers_->SetBlacklist(std::vector<std::string>());
  external_printers_->ClearData();
  scoped_task_environment_.RunUntilIdle();
  EXPECT_EQ(0, obs.called);

  external_printers_->SetData(
      std::make_unique<std::string>(kBulkPolicyContentsJson));

  scoped_task_environment_.RunUntilIdle();
  EXPECT_TRUE(external_printers_->IsPolicySet());
  EXPECT_EQ(1, obs.called);
  EXPECT_TRUE(obs.last_valid);  // ready now
  // Printer list is correct after notification.
  EXPECT_EQ(kNumPrinters, external_printers_->GetPrinters().size());

  external_printers_->SetAccessMode(ExternalPrinters::WHITELIST_ONLY);
  scoped_task_environment_.RunUntilIdle();
  EXPECT_EQ(2, obs.called);  // effective list changed.  Notified.
  EXPECT_TRUE(obs.last_valid);

  external_printers_->SetAccessMode(ExternalPrinters::BLACKLIST_ONLY);
  scoped_task_environment_.RunUntilIdle();
  EXPECT_EQ(3, obs.called);  // effective list changed.  Notified.
  EXPECT_TRUE(obs.last_valid);

  external_printers_->ClearData();
  scoped_task_environment_.RunUntilIdle();
  EXPECT_EQ(4, obs.called);  // Called for transition to invalid policy.
  EXPECT_FALSE(obs.last_valid);
  EXPECT_TRUE(external_printers_->GetPrinters().empty());

  // cleanup
  external_printers_->RemoveObserver(&obs);
}

}  // namespace
}  // namespace chromeos
