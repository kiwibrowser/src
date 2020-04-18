// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/do_not_track_collection_view_controller.h"

#include <memory>

#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/message_loop/message_loop.h"
#include "components/prefs/pref_member.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/pref_service_mock_factory.h"
#include "ios/chrome/browser/pref_names.h"
#import "ios/chrome/browser/ui/collection_view/collection_view_controller_test.h"
#include "ios/chrome/grit/ios_strings.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

class DoNotTrackCollectionViewControllerTest
    : public CollectionViewControllerTest {
 protected:
  void SetUp() override {
    CollectionViewControllerTest::SetUp();
    pref_service_ = CreateLocalState();
  }

  CollectionViewController* InstantiateController() override {
    return [[DoNotTrackCollectionViewController alloc]
        initWithPrefs:pref_service_.get()];
  }

  std::unique_ptr<PrefService> CreateLocalState() {
    scoped_refptr<PrefRegistrySimple> registry(new PrefRegistrySimple());
    registry->RegisterBooleanPref(prefs::kEnableDoNotTrack, false);
    base::FilePath path("DoNotTrackCollectionViewControllerTest.pref");

    sync_preferences::PrefServiceMockFactory factory;
    factory.SetUserPrefsFile(path, message_loop_.task_runner().get());
    return factory.Create(registry.get());
  }

  base::MessageLoopForUI message_loop_;
  std::unique_ptr<PrefService> pref_service_;
};

TEST_F(DoNotTrackCollectionViewControllerTest, TestModelDoNotTrackOff) {
  CreateController();
  CheckController();
  EXPECT_EQ(2, NumberOfSections());
  EXPECT_EQ(1, NumberOfItemsInSection(0));
  CheckSwitchCellStateAndTitleWithId(NO, IDS_IOS_OPTIONS_DO_NOT_TRACK_MOBILE, 0,
                                     0);
}

TEST_F(DoNotTrackCollectionViewControllerTest, TestModelDoNotTrackOn) {
  BooleanPrefMember doNotTrackEnabled;
  doNotTrackEnabled.Init(prefs::kEnableDoNotTrack, pref_service_.get());
  doNotTrackEnabled.SetValue(true);
  CreateController();
  EXPECT_EQ(2, NumberOfSections());
  EXPECT_EQ(1, NumberOfItemsInSection(0));
  CheckSwitchCellStateAndTitleWithId(YES, IDS_IOS_OPTIONS_DO_NOT_TRACK_MOBILE,
                                     0, 0);
}

}  // namespace
