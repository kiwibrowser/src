// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web/external_app_launching_state.h"

#include "base/test/ios/wait_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using ExternalAppLaunchingStateTest = PlatformTest;

// Tests that updateWithLaunchRequest counts the number of consecutive launches
// correctly and also reset when the time between launches is more than the
// predefined max allowed time between consecutive launches.
TEST_F(ExternalAppLaunchingStateTest, TestUpdateWithLaunchRequest) {
  ExternalAppLaunchingState* state = [[ExternalAppLaunchingState alloc] init];
  EXPECT_EQ(kDefaultMaxSecondsBetweenConsecutiveExternalAppLaunches,
            [ExternalAppLaunchingState maxSecondsBetweenConsecutiveLaunches]);
  double maxSecondsBetweenLaunches = 0.25;
  [ExternalAppLaunchingState
      setMaxSecondsBetweenConsecutiveLaunches:maxSecondsBetweenLaunches];

  EXPECT_EQ(0, state.consecutiveLaunchesCount);
  [state updateWithLaunchRequest];
  EXPECT_EQ(1, state.consecutiveLaunchesCount);
  [state updateWithLaunchRequest];
  EXPECT_EQ(2, state.consecutiveLaunchesCount);
  [state updateWithLaunchRequest];
  EXPECT_EQ(3, state.consecutiveLaunchesCount);
  // Wait for more than |maxSecondsBetweenLaunches|.
  base::test::ios::SpinRunLoopWithMinDelay(
      base::TimeDelta::FromSecondsD(maxSecondsBetweenLaunches + 0.1));
  // consecutiveLaunchesCount should reset.
  [state updateWithLaunchRequest];
  EXPECT_EQ(1, state.consecutiveLaunchesCount);
  [state updateWithLaunchRequest];
  EXPECT_EQ(2, state.consecutiveLaunchesCount);
  // reset back to the default value.
  [ExternalAppLaunchingState
      setMaxSecondsBetweenConsecutiveLaunches:
          kDefaultMaxSecondsBetweenConsecutiveExternalAppLaunches];
}
