// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <EarlGrey/EarlGrey.h>
#import <UIKit/UIKit.h>
#import <XCTest/XCTest.h>

#include "components/strings/grit/components_strings.h"
#import "ios/chrome/browser/ui/alert_coordinator/alert_coordinator.h"
#import "ios/chrome/browser/ui/util/top_view_controller.h"
#import "ios/chrome/test/earl_grey/chrome_matchers.h"
#import "ios/chrome/test/earl_grey/chrome_test_case.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
NSString* kTitle = @"Foo Title";
}  // namespace

// Integration test for the alert coordinator using Earl Grey.
@interface AlertCoordinatorTestCase : ChromeTestCase

// Whether an alert is presented.
- (BOOL)isPresentingAlert;

@end

@implementation AlertCoordinatorTestCase

// Tests that if the alert coordinator is destroyed, the alert is dismissed.
- (void)testDismissOnDestroy {
  // TODO(crbug.com/754642): Remove TopPresentedViewControllerFrom().
  UIViewController* topViewController =
      top_view_controller::TopPresentedViewController();

  AlertCoordinator* alertCoordinator =
      [[AlertCoordinator alloc] initWithBaseViewController:topViewController
                                                     title:kTitle
                                                   message:nil];

  [alertCoordinator start];

  GREYAssertTrue([self isPresentingAlert], @"An alert should be presented");

  alertCoordinator = nil;

  GREYAssertFalse([self isPresentingAlert], @"The alert should be removed");
}

- (void)testNoInteractionActionAfterTap {
  // TODO(crbug.com/754642): Remove TopPresentedViewControllerFrom().
  UIViewController* topViewController =
      top_view_controller::TopPresentedViewController();

  AlertCoordinator* alertCoordinator =
      [[AlertCoordinator alloc] initWithBaseViewController:topViewController
                                                     title:kTitle
                                                   message:nil];

  __block BOOL blockCalled = NO;

  [alertCoordinator setNoInteractionAction:^{
    blockCalled = YES;
  }];

  [alertCoordinator start];

  GREYAssertTrue([self isPresentingAlert], @"An alert should be presented");

  [[EarlGrey
      selectElementWithMatcher:chrome_test_util::ButtonWithAccessibilityLabelId(
                                   IDS_OK)] performAction:grey_tap()];

  GREYAssertFalse([self isPresentingAlert], @"The alert should be removed");

  GREYAssertFalse(blockCalled,
                  @"The noInteractionBlock should not have been called.");
}

- (BOOL)isPresentingAlert {
  NSError* error = nil;
  [[EarlGrey selectElementWithMatcher:grey_accessibilityLabel(kTitle)]
      assertWithMatcher:grey_sufficientlyVisible()
                  error:&error];

  return (error == nil);
}

@end
