// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/privacy_collection_view_controller.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "base/strings/sys_string_conversions.h"
#include "components/handoff/pref_names_ios.h"
#include "components/payments/core/payment_prefs.h"
#include "components/prefs/pref_service.h"
#include "components/strings/grit/components_strings.h"
#include "components/sync_preferences/pref_service_mock_factory.h"
#include "components/sync_preferences/pref_service_syncable.h"
#include "ios/chrome/browser/application_context.h"
#import "ios/chrome/browser/autofill/autofill_controller.h"
#include "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#include "ios/chrome/browser/experimental_flags.h"
#include "ios/chrome/browser/pref_names.h"
#include "ios/chrome/browser/prefs/browser_prefs.h"
#import "ios/chrome/browser/ui/collection_view/collection_view_controller_test.h"
#include "ios/chrome/grit/ios_chromium_strings.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ios/chrome/test/ios_chrome_scoped_testing_local_state.h"
#include "ios/web/public/test/test_web_thread_bundle.h"
#include "ios/web/public/web_capabilities.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

NSString* const kSpdyProxyEnabled = @"SpdyProxyEnabled";

class PrivacyCollectionViewControllerTest
    : public CollectionViewControllerTest {
 protected:
  void SetUp() override {
    CollectionViewControllerTest::SetUp();
    TestChromeBrowserState::Builder test_cbs_builder;
    test_cbs_builder.SetPrefService(CreatePrefService());
    chrome_browser_state_ = test_cbs_builder.Build();

    // Toggle off payments::kCanMakePaymentEnabled.
    chrome_browser_state_->GetPrefs()->SetBoolean(
        payments::kCanMakePaymentEnabled, false);

    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    initialValueForSpdyProxyEnabled_ =
        [[defaults valueForKey:kSpdyProxyEnabled] copy];
    [defaults setValue:@"Disabled" forKey:kSpdyProxyEnabled];
    CreateController();
  }

  void TearDown() override {
    if (initialValueForSpdyProxyEnabled_) {
      [[NSUserDefaults standardUserDefaults]
          setObject:initialValueForSpdyProxyEnabled_
             forKey:kSpdyProxyEnabled];
    } else {
      [[NSUserDefaults standardUserDefaults]
          removeObjectForKey:kSpdyProxyEnabled];
    }
    CollectionViewControllerTest::TearDown();
  }

  // Makes a PrefService to be used by the test.
  std::unique_ptr<sync_preferences::PrefServiceSyncable> CreatePrefService() {
    scoped_refptr<user_prefs::PrefRegistrySyncable> registry(
        new user_prefs::PrefRegistrySyncable);
    RegisterBrowserStatePrefs(registry.get());
    sync_preferences::PrefServiceMockFactory factory;
    return factory.CreateSyncable(registry.get());
  }

  CollectionViewController* InstantiateController() override {
    return [[PrivacyCollectionViewController alloc]
        initWithBrowserState:chrome_browser_state_.get()];
  }

  web::TestWebThreadBundle thread_bundle_;
  IOSChromeScopedTestingLocalState local_state_;
  std::unique_ptr<TestChromeBrowserState> chrome_browser_state_;
  NSString* initialValueForSpdyProxyEnabled_;
};

// Tests PrivacyCollectionViewController is set up with all appropriate items
// and sections.
TEST_F(PrivacyCollectionViewControllerTest, TestModel) {
  CheckController();
  EXPECT_EQ(5, NumberOfSections());

  int sectionIndex = 0;
  EXPECT_EQ(1, NumberOfItemsInSection(sectionIndex));
  CheckSectionHeaderWithId(IDS_IOS_OPTIONS_CONTINUITY_LABEL, sectionIndex);
  NSString* handoffSubtitle = chrome_browser_state_->GetPrefs()->GetBoolean(
                                  prefs::kIosHandoffToOtherDevices)
                                  ? l10n_util::GetNSString(IDS_IOS_SETTING_ON)
                                  : l10n_util::GetNSString(IDS_IOS_SETTING_OFF);
  CheckTextCellTitleAndSubtitle(
      l10n_util::GetNSString(IDS_IOS_OPTIONS_ENABLE_HANDOFF_TO_OTHER_DEVICES),
      handoffSubtitle, sectionIndex, 0);

  ++sectionIndex;
  NSInteger expectedRows = 2;

  if (web::IsDoNotTrackSupported())
    expectedRows++;
  EXPECT_EQ(expectedRows, NumberOfItemsInSection(sectionIndex));

  CheckSectionHeaderWithId(IDS_IOS_OPTIONS_WEB_SERVICES_LABEL, sectionIndex);
  int row = 0;

  CheckSwitchCellStateAndTitleWithId(
      YES, IDS_IOS_OPTIONS_SEARCH_URL_SUGGESTIONS, sectionIndex, row++);

  CheckDetailItemTextWithIds(IDS_IOS_OPTIONS_SEND_USAGE_DATA,
                             IDS_IOS_OPTIONS_DATA_USAGE_NEVER, sectionIndex,
                             row++);

  if (web::IsDoNotTrackSupported()) {
    NSString* doNotTrackSubtitle =
        chrome_browser_state_->GetPrefs()->GetBoolean(prefs::kEnableDoNotTrack)
            ? l10n_util::GetNSString(IDS_IOS_SETTING_ON)
            : l10n_util::GetNSString(IDS_IOS_SETTING_OFF);
    CheckTextCellTitleAndSubtitle(
        l10n_util::GetNSString(IDS_IOS_OPTIONS_DO_NOT_TRACK_MOBILE),
        doNotTrackSubtitle, sectionIndex, row++);
  }

  sectionIndex++;
  EXPECT_EQ(1, NumberOfItemsInSection(sectionIndex));
  CheckSectionFooterWithId(IDS_IOS_OPTIONS_PRIVACY_FOOTER, sectionIndex);

  sectionIndex++;
  EXPECT_EQ(1, NumberOfItemsInSection(sectionIndex));
  CheckSwitchCellStateAndTitle(
      NO, l10n_util::GetNSString(IDS_SETTINGS_CAN_MAKE_PAYMENT_TOGGLE_LABEL),
      sectionIndex, 0);

  sectionIndex++;
  EXPECT_EQ(1, NumberOfItemsInSection(sectionIndex));
  CheckTextCellTitle(l10n_util::GetNSString(IDS_IOS_CLEAR_BROWSING_DATA_TITLE),
                     sectionIndex, 0);
}

}  // namespace
