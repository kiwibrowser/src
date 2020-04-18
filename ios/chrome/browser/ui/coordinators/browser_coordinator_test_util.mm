// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/coordinators/browser_coordinator_test_util.h"

#import "ios/chrome/browser/ui/coordinators/browser_coordinator+internal.h"
#import "ios/testing/wait_util.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

void WaitForBrowserCoordinatorActivation(BrowserCoordinator* coordinator) {
  ASSERT_TRUE(coordinator);
  UIViewController* view_controller = coordinator.viewController;
  ASSERT_TRUE(view_controller);
  EXPECT_TRUE(testing::WaitUntilConditionOrTimeout(
      testing::kWaitForUIElementTimeout, ^bool {
        return view_controller.presentingViewController &&
               !view_controller.beingPresented;
      }));
}

void WaitForBrowserCoordinatorDeactivation(BrowserCoordinator* coordinator) {
  ASSERT_TRUE(coordinator);
  UIViewController* view_controller = coordinator.viewController;
  EXPECT_TRUE(testing::WaitUntilConditionOrTimeout(
      testing::kWaitForUIElementTimeout, ^bool {
        return !view_controller.presentingViewController &&
               !view_controller.beingDismissed;
      }));
}
