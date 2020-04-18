// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/payments/payment_request_error_coordinator.h"

#include "base/mac/foundation_util.h"
#include "base/memory/ptr_util.h"
#include "base/test/ios/wait_util.h"
#import "ios/chrome/browser/ui/payments/payment_request_error_view_controller.h"
#import "ios/chrome/test/scoped_key_window.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#include "third_party/ocmock/OCMock/OCMock.h"
#include "third_party/ocmock/gtest_support.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using PaymentRequestErrorCoordinatorTest = PlatformTest;

// Tests that invoking start and stop on the coordinator presents and dismisses
// the payment request error view controller, respectively.
TEST_F(PaymentRequestErrorCoordinatorTest, StartAndStop) {
  UIViewController* base_view_controller = [[UIViewController alloc] init];
  ScopedKeyWindow scoped_key_window_;
  [scoped_key_window_.Get() setRootViewController:base_view_controller];

  PaymentRequestErrorCoordinator* coordinator =
      [[PaymentRequestErrorCoordinator alloc]
          initWithBaseViewController:base_view_controller];

  [coordinator start];
  // Spin the run loop to trigger the animation.
  base::test::ios::SpinRunLoopWithMaxDelay(base::TimeDelta::FromSecondsD(1.0));
  EXPECT_TRUE([base_view_controller.presentedViewController
      isMemberOfClass:[PaymentRequestErrorViewController class]]);

  [coordinator stop];
  // Wait until the animation completes and the presented view controller is
  // dismissed.
  base::test::ios::WaitUntilCondition(^bool() {
    return !base_view_controller.presentedViewController;
  });
  EXPECT_EQ(nil, base_view_controller.presentedViewController);
}

// Tests that calling the view controller delegate method which notifies the
// coordinator that the user has dismissed the error invokes the corresponding
// coordinator delegate method.
TEST_F(PaymentRequestErrorCoordinatorTest, DidDismiss) {
  UIViewController* base_view_controller = [[UIViewController alloc] init];
  ScopedKeyWindow scoped_key_window_;
  [scoped_key_window_.Get() setRootViewController:base_view_controller];

  PaymentRequestErrorCoordinator* coordinator =
      [[PaymentRequestErrorCoordinator alloc]
          initWithBaseViewController:base_view_controller];

  // Mock the coordinator delegate.
  id delegate = [OCMockObject
      mockForProtocol:@protocol(PaymentRequestErrorCoordinatorDelegate)];
  [[delegate expect] paymentRequestErrorCoordinatorDidDismiss:coordinator];
  [coordinator setDelegate:delegate];

  [coordinator start];
  // Spin the run loop to trigger the animation.
  base::test::ios::SpinRunLoopWithMaxDelay(base::TimeDelta::FromSecondsD(1.0));

  // Call the controller delegate method.
  PaymentRequestErrorViewController* view_controller =
      base::mac::ObjCCastStrict<PaymentRequestErrorViewController>(
          base_view_controller.presentedViewController);
  [coordinator paymentRequestErrorViewControllerDidDismiss:view_controller];

  EXPECT_OCMOCK_VERIFY(delegate);
}
