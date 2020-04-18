// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/alert_coordinator/action_sheet_coordinator.h"

#import <UIKit/UIKit.h>

#import "base/mac/foundation_util.h"
#import "ios/chrome/test/scoped_key_window.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using ActionSheetCoordinatorTest = PlatformTest;

// Tests that if there is a popover, it uses the CGRect passed in init.
TEST_F(ActionSheetCoordinatorTest, CGRectUsage) {
  ScopedKeyWindow scoped_key_window;
  UIViewController* viewController = [[UIViewController alloc] init];
  [scoped_key_window.Get() setRootViewController:viewController];

  UIView* view = [[UIView alloc] initWithFrame:viewController.view.bounds];

  [viewController.view addSubview:view];
  CGRect rect = CGRectMake(124, 432, 126, 63);
  AlertCoordinator* alertCoordinator =
      [[ActionSheetCoordinator alloc] initWithBaseViewController:viewController
                                                           title:@"title"
                                                         message:nil
                                                            rect:rect
                                                            view:view];

  // Action.
  [alertCoordinator start];

  // Test.
  // Get the alert.
  EXPECT_TRUE([viewController.presentedViewController
      isKindOfClass:[UIAlertController class]]);
  UIAlertController* alertController =
      base::mac::ObjCCastStrict<UIAlertController>(
          viewController.presentedViewController);

  // Test the results.
  EXPECT_EQ(UIAlertControllerStyleActionSheet, alertController.preferredStyle);

  if (alertController.popoverPresentationController) {
    UIPopoverPresentationController* popover =
        alertController.popoverPresentationController;
    EXPECT_TRUE(CGRectEqualToRect(rect, popover.sourceRect));
    EXPECT_EQ(view, popover.sourceView);
  }

  [alertCoordinator stop];
}
