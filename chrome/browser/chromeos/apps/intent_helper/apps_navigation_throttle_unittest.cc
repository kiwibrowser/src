// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/apps/intent_helper/apps_navigation_throttle.h"
#include "components/arc/intent_helper/arc_intent_helper_bridge.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace chromeos {

TEST(AppsNavigationThrottleTest, TestShouldOverrideUrlLoading) {
  // A navigation within the same domain shouldn't be overridden except if the
  // domain is google.com.
  EXPECT_FALSE(AppsNavigationThrottle::ShouldOverrideUrlLoadingForTesting(
      GURL("http://google.com"), GURL("http://google.com/")));
  EXPECT_TRUE(AppsNavigationThrottle::ShouldOverrideUrlLoadingForTesting(
      GURL("http://google.com"), GURL("http://a.google.com/")));
  EXPECT_TRUE(AppsNavigationThrottle::ShouldOverrideUrlLoadingForTesting(
      GURL("http://a.google.com"), GURL("http://google.com/")));
  EXPECT_TRUE(AppsNavigationThrottle::ShouldOverrideUrlLoadingForTesting(
      GURL("http://a.google.com"), GURL("http://b.google.com/")));
  EXPECT_TRUE(AppsNavigationThrottle::ShouldOverrideUrlLoadingForTesting(
      GURL("http://a.google.com"), GURL("http://b.c.google.com/")));
  EXPECT_TRUE(AppsNavigationThrottle::ShouldOverrideUrlLoadingForTesting(
      GURL("http://a.b.google.com"), GURL("http://c.google.com/")));
  EXPECT_TRUE(AppsNavigationThrottle::ShouldOverrideUrlLoadingForTesting(
      GURL("http://a.b.google.com"), GURL("http://b.google.com")));
  EXPECT_TRUE(AppsNavigationThrottle::ShouldOverrideUrlLoadingForTesting(
      GURL("http://b.google.com"), GURL("http://a.b.google.com")));
  EXPECT_FALSE(AppsNavigationThrottle::ShouldOverrideUrlLoadingForTesting(
      GURL("http://notg.com"), GURL("http://notg.com")));
  EXPECT_FALSE(AppsNavigationThrottle::ShouldOverrideUrlLoadingForTesting(
      GURL("http://a.notg.com"), GURL("http://notg.com")));
  EXPECT_FALSE(AppsNavigationThrottle::ShouldOverrideUrlLoadingForTesting(
      GURL("http://notg.com"), GURL("http://a.notg.com")));
  EXPECT_FALSE(AppsNavigationThrottle::ShouldOverrideUrlLoadingForTesting(
      GURL("http://a.notg.com"), GURL("http://b.notg.com")));
  EXPECT_FALSE(AppsNavigationThrottle::ShouldOverrideUrlLoadingForTesting(
      GURL("http://a.notg.com"), GURL("http://a.b.notg.com")));
  EXPECT_FALSE(AppsNavigationThrottle::ShouldOverrideUrlLoadingForTesting(
      GURL("http://a.b.notg.com"), GURL("http://c.notg.com")));

  // Same as last tests, except for "play.google.com".
  EXPECT_TRUE(AppsNavigationThrottle::ShouldOverrideUrlLoadingForTesting(
      GURL("http://google.com"), GURL("http://play.google.com/fake_app")));
  EXPECT_TRUE(AppsNavigationThrottle::ShouldOverrideUrlLoadingForTesting(
      GURL("https://www.google.com.mx"),
      GURL("https://play.google.com/fake_app")));
  EXPECT_TRUE(AppsNavigationThrottle::ShouldOverrideUrlLoadingForTesting(
      GURL("https://mail.google.com"),
      GURL("https://play.google.com/fake_app")));
  EXPECT_FALSE(AppsNavigationThrottle::ShouldOverrideUrlLoadingForTesting(
      GURL("https://play.google.com/search"),
      GURL("https://play.google.com/fake_app")));
  EXPECT_TRUE(AppsNavigationThrottle::ShouldOverrideUrlLoadingForTesting(
      GURL("http://not_google.com"), GURL("http://play.google.com/fake_app")));

  // If either of two paramters is empty, the function should return false.
  EXPECT_FALSE(AppsNavigationThrottle::ShouldOverrideUrlLoadingForTesting(
      GURL(), GURL("http://a.google.com/")));
  EXPECT_FALSE(AppsNavigationThrottle::ShouldOverrideUrlLoadingForTesting(
      GURL("http://a.google.com/"), GURL()));
  EXPECT_FALSE(AppsNavigationThrottle::ShouldOverrideUrlLoadingForTesting(
      GURL(), GURL()));

  // A navigation not within the same domain can be overridden.
  EXPECT_TRUE(AppsNavigationThrottle::ShouldOverrideUrlLoadingForTesting(
      GURL("http://www.google.com"), GURL("http://www.not-google.com/")));
  EXPECT_TRUE(AppsNavigationThrottle::ShouldOverrideUrlLoadingForTesting(
      GURL("http://www.not-google.com"), GURL("http://www.google.com/")));

  // A navigation with neither an http nor https scheme cannot be overriden.
  EXPECT_FALSE(AppsNavigationThrottle::ShouldOverrideUrlLoadingForTesting(
      GURL("chrome-extension://fake_document"), GURL("http://www.a.com")));
  EXPECT_FALSE(AppsNavigationThrottle::ShouldOverrideUrlLoadingForTesting(
      GURL("http://www.a.com"), GURL("chrome-extension://fake_document")));
  EXPECT_FALSE(AppsNavigationThrottle::ShouldOverrideUrlLoadingForTesting(
      GURL("chrome-extension://fake_document"), GURL("https://www.a.com")));
  EXPECT_FALSE(AppsNavigationThrottle::ShouldOverrideUrlLoadingForTesting(
      GURL("https://www.a.com"), GURL("chrome-extension://fake_document")));
  EXPECT_FALSE(AppsNavigationThrottle::ShouldOverrideUrlLoadingForTesting(
      GURL("chrome-extension://fake_a"), GURL("chrome-extension://fake_b")));
}

TEST(AppsNavigationThrottleTest, TestGetPickerAction) {
  // Expect PickerAction::ERROR if the close_reason is ERROR.
  EXPECT_EQ(AppsNavigationThrottle::PickerAction::ERROR,
            AppsNavigationThrottle::GetPickerAction(
                AppType::INVALID, IntentPickerCloseReason::ERROR,
                /*should_persist=*/true));

  EXPECT_EQ(AppsNavigationThrottle::PickerAction::ERROR,
            AppsNavigationThrottle::GetPickerAction(
                AppType::ARC, IntentPickerCloseReason::ERROR,
                /*should_persist=*/true));

  EXPECT_EQ(AppsNavigationThrottle::PickerAction::ERROR,
            AppsNavigationThrottle::GetPickerAction(
                AppType::INVALID, IntentPickerCloseReason::ERROR,
                /*should_persist=*/false));

  EXPECT_EQ(AppsNavigationThrottle::PickerAction::ERROR,
            AppsNavigationThrottle::GetPickerAction(
                AppType::ARC, IntentPickerCloseReason::ERROR,
                /*should_persist=*/false));

  // Expect PickerAction::DIALOG_DEACTIVATED if the close_reason is
  // DIALOG_DEACTIVATED.
  EXPECT_EQ(AppsNavigationThrottle::PickerAction::DIALOG_DEACTIVATED,
            AppsNavigationThrottle::GetPickerAction(
                AppType::INVALID, IntentPickerCloseReason::DIALOG_DEACTIVATED,
                /*should_persist=*/true));

  EXPECT_EQ(AppsNavigationThrottle::PickerAction::DIALOG_DEACTIVATED,
            AppsNavigationThrottle::GetPickerAction(
                AppType::ARC, IntentPickerCloseReason::DIALOG_DEACTIVATED,
                /*should_persist=*/true));

  EXPECT_EQ(AppsNavigationThrottle::PickerAction::DIALOG_DEACTIVATED,
            AppsNavigationThrottle::GetPickerAction(
                AppType::INVALID, IntentPickerCloseReason::DIALOG_DEACTIVATED,
                /*should_persist=*/false));

  EXPECT_EQ(AppsNavigationThrottle::PickerAction::DIALOG_DEACTIVATED,
            AppsNavigationThrottle::GetPickerAction(
                AppType::ARC, IntentPickerCloseReason::DIALOG_DEACTIVATED,
                /*should_persist=*/false));

  // Expect PickerAction::PREFERRED_ACTIVITY_FOUND if the close_reason is
  // PREFERRED_APP_FOUND.
  EXPECT_EQ(AppsNavigationThrottle::PickerAction::PREFERRED_ACTIVITY_FOUND,
            AppsNavigationThrottle::GetPickerAction(
                AppType::INVALID, IntentPickerCloseReason::PREFERRED_APP_FOUND,
                /*should_persist=*/true));

  EXPECT_EQ(AppsNavigationThrottle::PickerAction::PREFERRED_ACTIVITY_FOUND,
            AppsNavigationThrottle::GetPickerAction(
                AppType::ARC, IntentPickerCloseReason::PREFERRED_APP_FOUND,
                /*should_persist=*/true));

  EXPECT_EQ(AppsNavigationThrottle::PickerAction::PREFERRED_ACTIVITY_FOUND,
            AppsNavigationThrottle::GetPickerAction(
                AppType::INVALID, IntentPickerCloseReason::PREFERRED_APP_FOUND,
                /*should_persist=*/false));

  EXPECT_EQ(AppsNavigationThrottle::PickerAction::PREFERRED_ACTIVITY_FOUND,
            AppsNavigationThrottle::GetPickerAction(
                AppType::ARC, IntentPickerCloseReason::PREFERRED_APP_FOUND,
                /*should_persist=*/false));

  // Expect PREFERRED depending on the value of |should_persist|, and |app_type|
  // to be ignored if reason is STAY_IN_CHROME.
  EXPECT_EQ(AppsNavigationThrottle::PickerAction::CHROME_PREFERRED_PRESSED,
            AppsNavigationThrottle::GetPickerAction(
                AppType::INVALID, IntentPickerCloseReason::STAY_IN_CHROME,
                /*should_persist=*/true));

  EXPECT_EQ(AppsNavigationThrottle::PickerAction::CHROME_PREFERRED_PRESSED,
            AppsNavigationThrottle::GetPickerAction(
                AppType::ARC, IntentPickerCloseReason::STAY_IN_CHROME,
                /*should_persist=*/true));

  EXPECT_EQ(AppsNavigationThrottle::PickerAction::CHROME_PRESSED,
            AppsNavigationThrottle::GetPickerAction(
                AppType::INVALID, IntentPickerCloseReason::STAY_IN_CHROME,
                /*should_persist=*/false));

  EXPECT_EQ(AppsNavigationThrottle::PickerAction::CHROME_PRESSED,
            AppsNavigationThrottle::GetPickerAction(
                AppType::ARC, IntentPickerCloseReason::STAY_IN_CHROME,
                /*should_persist=*/false));

  // Expect PREFERRED depending on the value of |should_persist|, and
  // INVALID/ARC to be chosen if reason is OPEN_APP.
  EXPECT_EQ(AppsNavigationThrottle::PickerAction::INVALID,
            AppsNavigationThrottle::GetPickerAction(
                AppType::INVALID, IntentPickerCloseReason::OPEN_APP,
                /*should_persist=*/true));

  EXPECT_EQ(AppsNavigationThrottle::PickerAction::ARC_APP_PREFERRED_PRESSED,
            AppsNavigationThrottle::GetPickerAction(
                AppType::ARC, IntentPickerCloseReason::OPEN_APP,
                /*should_persist=*/true));

  EXPECT_EQ(AppsNavigationThrottle::PickerAction::INVALID,
            AppsNavigationThrottle::GetPickerAction(
                AppType::INVALID, IntentPickerCloseReason::OPEN_APP,
                /*should_persist=*/false));

  EXPECT_EQ(AppsNavigationThrottle::PickerAction::ARC_APP_PRESSED,
            AppsNavigationThrottle::GetPickerAction(
                AppType::ARC, IntentPickerCloseReason::OPEN_APP,
                /*should_persist=*/false));
}

TEST(AppsNavigationThrottleTest, TestGetDestinationPlatform) {
  const std::string chrome_app =
      arc::ArcIntentHelperBridge::kArcIntentHelperPackageName;
  const std::string non_chrome_app = "fake_package";

  // When the PickerAction is either ERROR or DIALOG_DEACTIVATED we MUST stay in
  // Chrome not taking into account the selected_app_package.
  EXPECT_EQ(AppsNavigationThrottle::Platform::CHROME,
            AppsNavigationThrottle::GetDestinationPlatform(
                chrome_app, AppsNavigationThrottle::PickerAction::ERROR));
  EXPECT_EQ(AppsNavigationThrottle::Platform::CHROME,
            AppsNavigationThrottle::GetDestinationPlatform(
                non_chrome_app, AppsNavigationThrottle::PickerAction::ERROR));
  EXPECT_EQ(AppsNavigationThrottle::Platform::CHROME,
            AppsNavigationThrottle::GetDestinationPlatform(
                chrome_app,
                AppsNavigationThrottle::PickerAction::DIALOG_DEACTIVATED));
  EXPECT_EQ(AppsNavigationThrottle::Platform::CHROME,
            AppsNavigationThrottle::GetDestinationPlatform(
                non_chrome_app,
                AppsNavigationThrottle::PickerAction::DIALOG_DEACTIVATED));

  // When the PickerAction is PWA_APP_PRESSED, always expect the platform to be
  // PWA.
  EXPECT_EQ(
      AppsNavigationThrottle::Platform::PWA,
      AppsNavigationThrottle::GetDestinationPlatform(
          chrome_app, AppsNavigationThrottle::PickerAction::PWA_APP_PRESSED));

  EXPECT_EQ(AppsNavigationThrottle::Platform::PWA,
            AppsNavigationThrottle::GetDestinationPlatform(
                non_chrome_app,
                AppsNavigationThrottle::PickerAction::PWA_APP_PRESSED));

  // Under any other PickerAction, stay in Chrome only if the package is Chrome.
  // Otherwise redirect to ARC.
  EXPECT_EQ(
      AppsNavigationThrottle::Platform::CHROME,
      AppsNavigationThrottle::GetDestinationPlatform(
          chrome_app,
          AppsNavigationThrottle::PickerAction::PREFERRED_ACTIVITY_FOUND));
  EXPECT_EQ(
      AppsNavigationThrottle::Platform::ARC,
      AppsNavigationThrottle::GetDestinationPlatform(
          non_chrome_app,
          AppsNavigationThrottle::PickerAction::PREFERRED_ACTIVITY_FOUND));
}

}  // namespace chromeos
