// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/app_launcher/app_launcher_tab_helper.h"

#include <memory>

#import "ios/chrome/browser/app_launcher/app_launcher_tab_helper_delegate.h"
#import "ios/chrome/browser/web/external_apps_launch_policy_decider.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// An object that conforms to AppLauncherTabHelperDelegate for testing.
@interface FakeAppLauncherTabHelperDelegate
    : NSObject<AppLauncherTabHelperDelegate>
// URL of the last launched application.
@property(nonatomic, assign) GURL lastLaunchedAppURL;
// Number of times an app was launched.
@property(nonatomic, assign) NSUInteger countOfAppsLaunched;
// Number of times the repeated launches alert has been shown.
@property(nonatomic, assign) NSUInteger countOfAlertsShown;
// Simulates the user tapping the accept button when prompted via
// |-appLauncherTabHelper:showAlertOfRepeatedLaunchesWithCompletionHandler|.
@property(nonatomic, assign) BOOL simulateUserAcceptingPrompt;
@end

@implementation FakeAppLauncherTabHelperDelegate
@synthesize lastLaunchedAppURL = _lastLaunchedAppURL;
@synthesize countOfAppsLaunched = _countOfAppsLaunched;
@synthesize countOfAlertsShown = _countOfAlertsShown;
@synthesize simulateUserAcceptingPrompt = _simulateUserAcceptingPrompt;
- (BOOL)appLauncherTabHelper:(AppLauncherTabHelper*)tabHelper
            launchAppWithURL:(const GURL&)URL
                  linkTapped:(BOOL)linkTapped {
  self.countOfAppsLaunched++;
  self.lastLaunchedAppURL = URL;
  return YES;
}
- (void)appLauncherTabHelper:(AppLauncherTabHelper*)tabHelper
    showAlertOfRepeatedLaunchesWithCompletionHandler:
        (ProceduralBlockWithBool)completionHandler {
  self.countOfAlertsShown++;
  completionHandler(self.simulateUserAcceptingPrompt);
}
@end

// An ExternalAppsLaunchPolicyDecider for testing.
@interface FakeExternalAppsLaunchPolicyDecider : ExternalAppsLaunchPolicyDecider
@property(nonatomic, assign) ExternalAppLaunchPolicy policy;
@end

@implementation FakeExternalAppsLaunchPolicyDecider
@synthesize policy = _policy;
- (ExternalAppLaunchPolicy)launchPolicyForURL:(const GURL&)URL
                            fromSourcePageURL:(const GURL&)sourcePageURL {
  return self.policy;
}
@end

// Test fixture for AppLauncherTabHelper class.
class AppLauncherTabHelperTest : public PlatformTest {
 protected:
  AppLauncherTabHelperTest()
      : policy_decider_([[FakeExternalAppsLaunchPolicyDecider alloc] init]),
        delegate_([[FakeAppLauncherTabHelperDelegate alloc] init]) {
    AppLauncherTabHelper::CreateForWebState(&web_state_, policy_decider_,
                                            delegate_);
    tab_helper_ = AppLauncherTabHelper::FromWebState(&web_state_);
  }

  web::TestWebState web_state_;
  FakeExternalAppsLaunchPolicyDecider* policy_decider_ = nil;
  FakeAppLauncherTabHelperDelegate* delegate_ = nil;
  AppLauncherTabHelper* tab_helper_;
};

// Tests that an empty URL does not show alert or launch app.
TEST_F(AppLauncherTabHelperTest, EmptyUrl) {
  tab_helper_->RequestToLaunchApp(GURL::EmptyGURL(), GURL::EmptyGURL(), false);
  EXPECT_EQ(0U, delegate_.countOfAlertsShown);
  EXPECT_EQ(0U, delegate_.countOfAppsLaunched);
}

// Tests that an invalid URL does not show alert or launch app.
TEST_F(AppLauncherTabHelperTest, InvalidUrl) {
  tab_helper_->RequestToLaunchApp(GURL("invalid"), GURL::EmptyGURL(), false);
  EXPECT_EQ(0U, delegate_.countOfAlertsShown);
  EXPECT_EQ(0U, delegate_.countOfAppsLaunched);
}

// Tests that a valid URL does launch app.
TEST_F(AppLauncherTabHelperTest, ValidUrl) {
  policy_decider_.policy = ExternalAppLaunchPolicyAllow;
  tab_helper_->RequestToLaunchApp(GURL("valid://1234"), GURL::EmptyGURL(),
                                  false);
  EXPECT_EQ(1U, delegate_.countOfAppsLaunched);
  EXPECT_EQ(GURL("valid://1234"), delegate_.lastLaunchedAppURL);
}

// Tests that a valid URL does not launch app when launch policy is to block.
TEST_F(AppLauncherTabHelperTest, ValidUrlBlocked) {
  policy_decider_.policy = ExternalAppLaunchPolicyBlock;
  tab_helper_->RequestToLaunchApp(GURL("valid://1234"), GURL::EmptyGURL(),
                                  false);
  EXPECT_EQ(0U, delegate_.countOfAlertsShown);
  EXPECT_EQ(0U, delegate_.countOfAppsLaunched);
}

// Tests that a valid URL shows an alert and launches app when launch policy is
// to prompt and user accepts.
TEST_F(AppLauncherTabHelperTest, ValidUrlPromptUserAccepts) {
  policy_decider_.policy = ExternalAppLaunchPolicyPrompt;
  delegate_.simulateUserAcceptingPrompt = YES;
  tab_helper_->RequestToLaunchApp(GURL("valid://1234"), GURL::EmptyGURL(),
                                  false);
  EXPECT_EQ(1U, delegate_.countOfAlertsShown);
  EXPECT_EQ(1U, delegate_.countOfAppsLaunched);
  EXPECT_EQ(GURL("valid://1234"), delegate_.lastLaunchedAppURL);
}

// Tests that a valid URL does not launch app when launch policy is to prompt
// and user rejects.
TEST_F(AppLauncherTabHelperTest, ValidUrlPromptUserRejects) {
  policy_decider_.policy = ExternalAppLaunchPolicyPrompt;
  delegate_.simulateUserAcceptingPrompt = NO;
  tab_helper_->RequestToLaunchApp(GURL("valid://1234"), GURL::EmptyGURL(),
                                  false);
  EXPECT_EQ(0U, delegate_.countOfAppsLaunched);
}
