// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/elements/selector_coordinator.h"

#include "base/test/ios/wait_util.h"
#import "ios/chrome/browser/ui/elements/selector_picker_view_controller.h"
#import "ios/chrome/browser/ui/elements/selector_view_controller_delegate.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"
#include "third_party/ocmock/OCMock/OCMock.h"
#include "third_party/ocmock/gtest_support.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface SelectorCoordinator ()<SelectorViewControllerDelegate>
// The view controller for the picker view the coordinator presents. Exposed for
// testing.
@property SelectorPickerViewController* selectorPickerViewController;
@end

using SelectorCoordinatorTest = PlatformTest;

// Tests that invoking start on the coordinator presents the selector view, and
// that invoking stop dismisses the view and invokes the delegate.
TEST_F(SelectorCoordinatorTest, StartAndStop) {
  UIWindow* keyWindow = [[UIApplication sharedApplication] keyWindow];
  UIViewController* rootViewController = keyWindow.rootViewController;
  SelectorCoordinator* coordinator = [[SelectorCoordinator alloc]
      initWithBaseViewController:rootViewController];

  void (^testSteps)(void) = ^{
    [coordinator start];
    EXPECT_NSEQ(coordinator.selectorPickerViewController,
                rootViewController.presentedViewController);

    [coordinator stop];
    base::test::ios::WaitUntilCondition(^{
      return !rootViewController.presentedViewController;
    });
  };
  // Ensure any other presented controllers are dismissed before starting the
  // coordinator.
  [rootViewController dismissViewControllerAnimated:NO completion:testSteps];
}

// Tests that calling the view controller delegate method invokes the
// SelectorCoordinatorDelegate method and stops the coordinator.
TEST_F(SelectorCoordinatorTest, Delegate) {
  UIWindow* keyWindow = [[UIApplication sharedApplication] keyWindow];
  UIViewController* rootViewController = keyWindow.rootViewController;
  SelectorCoordinator* coordinator = [[SelectorCoordinator alloc]
      initWithBaseViewController:rootViewController];
  id delegate =
      [OCMockObject mockForProtocol:@protocol(SelectorCoordinatorDelegate)];
  coordinator.delegate = delegate;

  void (^testSteps)(void) = ^{
    [coordinator start];
    NSString* testOption = @"Test Option";
    [[delegate expect] selectorCoordinator:coordinator
                  didCompleteWithSelection:testOption];
    [coordinator selectorViewController:coordinator.selectorPickerViewController
                        didSelectOption:testOption];
    base::test::ios::WaitUntilCondition(^{
      return !rootViewController.presentedViewController;
    });
  };
  // Ensure any other presented controllers are dismissed before starting the
  // coordinator.
  [rootViewController dismissViewControllerAnimated:NO completion:testSteps];
  EXPECT_OCMOCK_VERIFY(delegate);
}
