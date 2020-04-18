// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/web/external_apps_launch_policy_decider.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const GURL kSourceUrl1("http://www.google.com");
const GURL kSourceUrl2("http://www.goog.com");
const GURL kSourceUrl3("http://www.goog.ab");
const GURL kSourceUrl4("http://www.foo.com");
const GURL kAppUrl1("facetime://+1354");
const GURL kAppUrl2("facetime-audio://+1234");
const GURL kAppUrl3("abc://abc");
const GURL kAppUrl4("chrome://www.google.com");

}  // namespace

using ExternalAppsLaunchPolicyDeciderTest = PlatformTest;

// Tests cases when the same app is launched repeatedly from same source.
TEST_F(ExternalAppsLaunchPolicyDeciderTest,
       TestRepeatedAppLaunches_SameAppSameSource) {
  ExternalAppsLaunchPolicyDecider* policyDecider =
      [[ExternalAppsLaunchPolicyDecider alloc] init];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow,
            [policyDecider launchPolicyForURL:GURL("facetime://+154")
                            fromSourcePageURL:kSourceUrl1]);

  [policyDecider didRequestLaunchExternalAppURL:GURL("facetime://+1354")
                              fromSourcePageURL:kSourceUrl1];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow,
            [policyDecider launchPolicyForURL:GURL("facetime://+12354")
                            fromSourcePageURL:kSourceUrl1]);

  [policyDecider didRequestLaunchExternalAppURL:GURL("facetime://+154")
                              fromSourcePageURL:kSourceUrl1];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow,
            [policyDecider launchPolicyForURL:GURL("facetime://+13454")
                            fromSourcePageURL:kSourceUrl1]);

  [policyDecider didRequestLaunchExternalAppURL:GURL("facetime://+14")
                              fromSourcePageURL:kSourceUrl1];
  // App was launched more than the max allowed times, the policy should change
  // to Prompt.
  EXPECT_EQ(ExternalAppLaunchPolicyPrompt,
            [policyDecider launchPolicyForURL:GURL("facetime://+14")
                            fromSourcePageURL:kSourceUrl1]);
}

// Tests cases when same app is launched repeatedly from different sources.
TEST_F(ExternalAppsLaunchPolicyDeciderTest,
       TestRepeatedAppLaunches_SameAppDifferentSources) {
  ExternalAppsLaunchPolicyDecider* policyDecider =
      [[ExternalAppsLaunchPolicyDecider alloc] init];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow,
            [policyDecider launchPolicyForURL:kAppUrl1
                            fromSourcePageURL:kSourceUrl1]);
  [policyDecider didRequestLaunchExternalAppURL:kAppUrl1
                              fromSourcePageURL:kSourceUrl1];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow,
            [policyDecider launchPolicyForURL:kAppUrl1
                            fromSourcePageURL:kSourceUrl1]);

  [policyDecider didRequestLaunchExternalAppURL:kAppUrl1
                              fromSourcePageURL:kSourceUrl2];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow,
            [policyDecider launchPolicyForURL:kAppUrl1
                            fromSourcePageURL:kSourceUrl2]);
  [policyDecider didRequestLaunchExternalAppURL:kAppUrl1
                              fromSourcePageURL:kSourceUrl3];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow,
            [policyDecider launchPolicyForURL:kAppUrl1
                            fromSourcePageURL:kSourceUrl3]);
  [policyDecider didRequestLaunchExternalAppURL:kAppUrl1
                              fromSourcePageURL:kSourceUrl4];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow,
            [policyDecider launchPolicyForURL:kAppUrl1
                            fromSourcePageURL:kSourceUrl4]);
}

// Tests cases when different apps are launched from different sources.
TEST_F(ExternalAppsLaunchPolicyDeciderTest,
       TestRepeatedAppLaunches_DifferentAppsDifferentSources) {
  ExternalAppsLaunchPolicyDecider* policyDecider =
      [[ExternalAppsLaunchPolicyDecider alloc] init];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow,
            [policyDecider launchPolicyForURL:kAppUrl1
                            fromSourcePageURL:kSourceUrl1]);
  [policyDecider didRequestLaunchExternalAppURL:kAppUrl1
                              fromSourcePageURL:kSourceUrl1];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow,
            [policyDecider launchPolicyForURL:kAppUrl1
                            fromSourcePageURL:kSourceUrl1]);

  [policyDecider didRequestLaunchExternalAppURL:kAppUrl2
                              fromSourcePageURL:kSourceUrl2];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow,
            [policyDecider launchPolicyForURL:kAppUrl2
                            fromSourcePageURL:kSourceUrl2]);
  [policyDecider didRequestLaunchExternalAppURL:kAppUrl3
                              fromSourcePageURL:kSourceUrl3];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow,
            [policyDecider launchPolicyForURL:kAppUrl3
                            fromSourcePageURL:kSourceUrl3]);
  [policyDecider didRequestLaunchExternalAppURL:kAppUrl4
                              fromSourcePageURL:kSourceUrl4];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow,
            [policyDecider launchPolicyForURL:kAppUrl4
                            fromSourcePageURL:kSourceUrl4]);
}

// Tests blocking App launch only when the app have been launched through the
// policy decider before.
TEST_F(ExternalAppsLaunchPolicyDeciderTest, TestBlockLaunchingApp) {
  ExternalAppsLaunchPolicyDecider* policyDecider =
      [[ExternalAppsLaunchPolicyDecider alloc] init];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow,
            [policyDecider launchPolicyForURL:kAppUrl1
                            fromSourcePageURL:kSourceUrl1]);
  // Don't block for apps that have not been registered.
  [policyDecider blockLaunchingAppURL:kAppUrl1 fromSourcePageURL:kSourceUrl1];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow,
            [policyDecider launchPolicyForURL:kAppUrl1
                            fromSourcePageURL:kSourceUrl1]);

  // Block for apps that have been registered
  EXPECT_EQ(ExternalAppLaunchPolicyAllow,
            [policyDecider launchPolicyForURL:kAppUrl2
                            fromSourcePageURL:kSourceUrl2]);
  [policyDecider didRequestLaunchExternalAppURL:kAppUrl2
                              fromSourcePageURL:kSourceUrl2];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow,
            [policyDecider launchPolicyForURL:kAppUrl2
                            fromSourcePageURL:kSourceUrl2]);
  [policyDecider blockLaunchingAppURL:kAppUrl2 fromSourcePageURL:kSourceUrl2];
  EXPECT_EQ(ExternalAppLaunchPolicyBlock,
            [policyDecider launchPolicyForURL:kAppUrl2
                            fromSourcePageURL:kSourceUrl2]);
}
