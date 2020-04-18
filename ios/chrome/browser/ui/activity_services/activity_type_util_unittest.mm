// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/activity_services/activity_type_util.h"

#include "ios/chrome/browser/ui/activity_services/appex_constants.h"
#include "ios/chrome/browser/ui/activity_services/print_activity.h"
#include "ios/chrome/grit/ios_strings.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

void StringToTypeTestHelper(NSString* activityString,
                            activity_type_util::ActivityType expectedType) {
  EXPECT_EQ(activity_type_util::TypeFromString(activityString), expectedType);
}

using ActivityTypeUtilTest = PlatformTest;

TEST_F(ActivityTypeUtilTest, StringToTypeTest) {
  StringToTypeTestHelper(@"", activity_type_util::UNKNOWN);
  StringToTypeTestHelper(@"foo", activity_type_util::UNKNOWN);
  StringToTypeTestHelper(@"com.google", activity_type_util::UNKNOWN);
  StringToTypeTestHelper(@"com.google.", activity_type_util::GOOGLE_UNKNOWN);
  StringToTypeTestHelper(@"com.google.Gmail",
                         activity_type_util::GOOGLE_UNKNOWN);
  StringToTypeTestHelper(@"com.google.Gmail.Bar",
                         activity_type_util::GOOGLE_GMAIL);
  StringToTypeTestHelper(@"com.apple.UIKit.activity.Mail",
                         activity_type_util::NATIVE_MAIL);
  StringToTypeTestHelper(@"com.apple.UIKit.activity.Mail.Qux",
                         activity_type_util::UNKNOWN);
  StringToTypeTestHelper([PrintActivity activityIdentifier],
                         activity_type_util::PRINT);
}

void TypeToMessageTestHelper(activity_type_util::ActivityType type,
                             NSString* expectedMessage) {
  EXPECT_NSEQ(activity_type_util::CompletionMessageForActivity(type),
              expectedMessage);
}

TEST_F(ActivityTypeUtilTest, TypeToMessageTest) {
  TypeToMessageTestHelper(activity_type_util::UNKNOWN, nil);
  TypeToMessageTestHelper(activity_type_util::PRINT, nil);
  TypeToMessageTestHelper(
      activity_type_util::NATIVE_CLIPBOARD,
      l10n_util::GetNSString(IDS_IOS_SHARE_TO_CLIPBOARD_SUCCESS));
  TypeToMessageTestHelper(
      activity_type_util::APPEX_PASSWORD_MANAGEMENT,
      l10n_util::GetNSString(IDS_IOS_APPEX_PASSWORD_FORM_FILLED_SUCCESS));
}

TEST_F(ActivityTypeUtilTest, IsPasswordAppExtensionTest) {
  // Verifies that known Bundle ID for 1Password requires exact match.
  EXPECT_EQ(activity_type_util::APPEX_PASSWORD_MANAGEMENT,
            activity_type_util::TypeFromString(
                @"com.agilebits.onepassword-ios.extension"));
  EXPECT_NE(activity_type_util::APPEX_PASSWORD_MANAGEMENT,
            activity_type_util::TypeFromString(
                @"com.agilebits.onepassword-ios.extension.otherstuff"));
  // Verifies that known Bundle ID for LastPass requires exact match.
  EXPECT_EQ(activity_type_util::APPEX_PASSWORD_MANAGEMENT,
            activity_type_util::TypeFromString(
                @"com.lastpass.ilastpass.LastPassExt"));
  EXPECT_NE(activity_type_util::APPEX_PASSWORD_MANAGEMENT,
            activity_type_util::TypeFromString(
                @"com.lastpass.ilastpass.LastPassExt.otherstuff"));
  // Verifies that both variants of Dashlane Bundle IDs are recognized.
  EXPECT_EQ(activity_type_util::APPEX_PASSWORD_MANAGEMENT,
            activity_type_util::TypeFromString(
                @"com.dashlane.dashlanephonefinal.SafariExtension"));
  EXPECT_EQ(activity_type_util::APPEX_PASSWORD_MANAGEMENT,
            activity_type_util::TypeFromString(
                @"com.dashlane.dashlanephonefinal.appextension"));
  // Verifies that any Bundle ID with @"find-login-action" is recognized.
  EXPECT_EQ(activity_type_util::APPEX_PASSWORD_MANAGEMENT,
            activity_type_util::TypeFromString(
                @"com.some-company.find-login-action.an-extension"));
  EXPECT_EQ(activity_type_util::APPEX_PASSWORD_MANAGEMENT,
            activity_type_util::TypeFromString(
                @"com.some-company.compatible-find-login-action-an-extension"));
  EXPECT_EQ(activity_type_util::APPEX_PASSWORD_MANAGEMENT,
            activity_type_util::TypeFromString(
                @"com.some-company.find-login-action-as-prefix"));
  EXPECT_EQ(activity_type_util::APPEX_PASSWORD_MANAGEMENT,
            activity_type_util::TypeFromString(
                @"com.some-company.with-suffix-of-find-login-action"));
  EXPECT_EQ(activity_type_util::APPEX_PASSWORD_MANAGEMENT,
            activity_type_util::TypeFromString(
                @"com.google.find-login-action.extension"));
  // Verifies non-matching Bundle IDs.
  EXPECT_NE(activity_type_util::APPEX_PASSWORD_MANAGEMENT,
            activity_type_util::TypeFromString(@"com.google.chrome.ios"));
  EXPECT_NE(activity_type_util::APPEX_PASSWORD_MANAGEMENT,
            activity_type_util::TypeFromString(
                @"com.apple.UIKit.activity.PostToFacebook"));
}

}  // namespace
