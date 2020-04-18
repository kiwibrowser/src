// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/forced_extensions/installation_tracker.h"

#include "base/test/histogram_tester.h"
#include "base/timer/mock_timer.h"
#include "base/values.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/pref_names.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
constexpr char kExtensionId1[] = "id1";
constexpr char kExtensionId2[] = "id2";
constexpr char kExtensionName1[] = "name1";
constexpr char kExtensionName2[] = "name2";
constexpr char kExtensionUrl1[] = "url1";
constexpr char kExtensionUrl2[] = "url2";

constexpr char kLoadTimeStats[] = "Extensions.ForceInstalledLoadTime";
constexpr char kTimedOutStats[] = "Extensions.ForceInstalledTimedOutCount";
}  // namespace

namespace extensions {

class ForcedExtensionsInstallationTrackerTest : public testing::Test {
 public:
  ForcedExtensionsInstallationTrackerTest()
      : prefs_(profile_.GetTestingPrefService()),
        registry_(ExtensionRegistry::Get(&profile_)) {
    auto fake_timer = std::make_unique<base::MockTimer>(true, false);
    fake_timer_ = fake_timer.get();
    tracker_ = std::make_unique<InstallationTracker>(registry_, prefs_,
                                                     std::move(fake_timer));
  }

  void SetupForceList() {
    base::Value dict(base::Value::Type::DICTIONARY);
    dict.SetKey(kExtensionId1, base::Value(kExtensionUrl1));
    dict.SetKey(kExtensionId2, base::Value(kExtensionUrl2));
    prefs_->SetManagedPref(pref_names::kInstallForceList,
                           base::Value::ToUniquePtrValue(std::move(dict)));
  }

 protected:
  content::TestBrowserThreadBundle thread_bundle_;
  TestingProfile profile_;
  sync_preferences::TestingPrefServiceSyncable* prefs_;
  ExtensionRegistry* registry_;
  base::HistogramTester histogram_tester_;

  base::MockTimer* fake_timer_;
  std::unique_ptr<InstallationTracker> tracker_;

  DISALLOW_COPY_AND_ASSIGN(ForcedExtensionsInstallationTrackerTest);
};

TEST_F(ForcedExtensionsInstallationTrackerTest, ExtensionsInstalled) {
  SetupForceList();
  auto ext1 = ExtensionBuilder(kExtensionName1).SetID(kExtensionId1).Build();
  auto ext2 = ExtensionBuilder(kExtensionName2).SetID(kExtensionId2).Build();

  histogram_tester_.ExpectTotalCount(kLoadTimeStats, 0);
  tracker_->OnExtensionLoaded(&profile_, ext1.get());
  histogram_tester_.ExpectTotalCount(kLoadTimeStats, 0);
  tracker_->OnExtensionLoaded(&profile_, ext2.get());
  histogram_tester_.ExpectTotalCount(kLoadTimeStats, 1);
  histogram_tester_.ExpectTotalCount(kTimedOutStats, 0);
}

TEST_F(ForcedExtensionsInstallationTrackerTest,
       ExtensionsInstallationTimedOut) {
  SetupForceList();
  EXPECT_TRUE(fake_timer_->IsRunning());
  fake_timer_->Fire();
  histogram_tester_.ExpectTotalCount(kLoadTimeStats, 0);
  histogram_tester_.ExpectUniqueSample(kTimedOutStats, 2, 1);
}

TEST_F(ForcedExtensionsInstallationTrackerTest, NoExtensionsConfigured) {
  EXPECT_TRUE(fake_timer_->IsRunning());
  fake_timer_->Fire();
  histogram_tester_.ExpectTotalCount(kLoadTimeStats, 0);
  histogram_tester_.ExpectTotalCount(kTimedOutStats, 0);
}

}  // namespace extensions
