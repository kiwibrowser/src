// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/main/main_coordinator.h"

#import <UIKit/UIKit.h>

#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using MainCoordinatorTest = PlatformTest;

TEST_F(MainCoordinatorTest, SizeViewController) {
  CGRect rect = [UIScreen mainScreen].bounds;
  UIWindow* window = [UIApplication sharedApplication].keyWindow;
  MainCoordinator* coordinator =
      [[MainCoordinator alloc] initWithWindow:window];
  [coordinator start];
  EXPECT_TRUE(
      CGRectEqualToRect(rect, coordinator.mainViewController.view.frame));
}
