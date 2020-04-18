// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/alert_coordinator/alert_coordinator.h"

#import <UIKit/UIKit.h>

#import "base/mac/foundation_util.h"
#import "ios/chrome/test/scoped_key_window.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#include "third_party/ocmock/gtest_support.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/strings/grit/ui_strings.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#pragma mark - Fixture.

// Fixture to test AlertCoordinator.
class AlertCoordinatorTest : public PlatformTest {
 protected:
  AlertCoordinatorTest() {
    view_controller_ = [[UIViewController alloc] init];
    [scoped_key_window_.Get() setRootViewController:view_controller_];
  }

  void startAlertCoordinator() { [alert_coordinator_ start]; }

  UIViewController* getViewController() { return view_controller_; }

  AlertCoordinator* getAlertCoordinator(UIViewController* viewController) {
    return getAlertCoordinator(viewController, @"Test title", nil);
  }

  AlertCoordinator* getAlertCoordinator(UIViewController* viewController,
                                        NSString* title,
                                        NSString* message) {
    alert_coordinator_ =
        [[AlertCoordinator alloc] initWithBaseViewController:viewController
                                                       title:title
                                                     message:message];
    return alert_coordinator_;
  }

 private:
  AlertCoordinator* alert_coordinator_;
  ScopedKeyWindow scoped_key_window_;
  UIViewController* view_controller_;
};

#pragma mark - Tests.

// Tests the alert coordinator reports as visible after presenting and at least
// on button is created.
TEST_F(AlertCoordinatorTest, ValidateIsVisible) {
  // Setup.
  UIViewController* viewController = getViewController();
  AlertCoordinator* alertCoordinator = getAlertCoordinator(viewController);

  ASSERT_FALSE(alertCoordinator.isVisible);
  ASSERT_EQ(nil, viewController.presentedViewController);

  // Action.
  startAlertCoordinator();

  // Test.
  EXPECT_TRUE(alertCoordinator.isVisible);
  EXPECT_TRUE([viewController.presentedViewController
      isKindOfClass:[UIAlertController class]]);
  UIAlertController* alertController =
      base::mac::ObjCCastStrict<UIAlertController>(
          viewController.presentedViewController);
  EXPECT_EQ(1LU, alertController.actions.count);
}

// Tests the alert coordinator reports as not visible after presenting on a non
// visible view.
TEST_F(AlertCoordinatorTest, ValidateIsNotVisible) {
  // Setup.
  UIWindow* window =
      [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
  UIViewController* viewController = [[UIViewController alloc] init];
  [window setRootViewController:viewController];

  AlertCoordinator* alertCoordinator = getAlertCoordinator(viewController);

  // Action.
  startAlertCoordinator();

  // Test.
  EXPECT_FALSE(alertCoordinator.isVisible);
  EXPECT_EQ(nil, [viewController presentedViewController]);
}

// Tests the alert coordinator has a correct title and message.
TEST_F(AlertCoordinatorTest, TitleAndMessage) {
  // Setup.
  UIViewController* viewController = getViewController();
  NSString* title = @"Foo test title!";
  NSString* message = @"Foo bar message.";

  getAlertCoordinator(viewController, title, message);

  // Action.
  startAlertCoordinator();

  // Test.
  // Get the alert.
  EXPECT_TRUE([viewController.presentedViewController
      isKindOfClass:[UIAlertController class]]);
  UIAlertController* alertController =
      base::mac::ObjCCastStrict<UIAlertController>(
          viewController.presentedViewController);

  // Test the results.
  EXPECT_EQ(title, alertController.title);
  EXPECT_EQ(message, alertController.message);
}

// Tests the alert coordinator dismissal.
TEST_F(AlertCoordinatorTest, ValidateDismissalOnStop) {
  // Setup.
  UIViewController* viewController = getViewController();
  AlertCoordinator* alertCoordinator = getAlertCoordinator(viewController);

  startAlertCoordinator();

  ASSERT_TRUE(alertCoordinator.isVisible);
  ASSERT_NE(nil, viewController.presentedViewController);
  ASSERT_TRUE([viewController.presentedViewController
      isKindOfClass:[UIAlertController class]]);

  id viewControllerMock = [OCMockObject partialMockForObject:viewController];
  [[[viewControllerMock expect] andForwardToRealObject]
      dismissViewControllerAnimated:NO
                         completion:nil];

  // Action.
  [alertCoordinator stop];

  // Test.
  EXPECT_FALSE(alertCoordinator.isVisible);
  EXPECT_OCMOCK_VERIFY(viewControllerMock);
}

// Tests that only the expected actions are present on the alert.
TEST_F(AlertCoordinatorTest, ValidateActions) {
  // Setup.
  UIViewController* viewController = getViewController();
  AlertCoordinator* alertCoordinator = getAlertCoordinator(viewController);

  NSDictionary* actions = @{
    @"foo" : @(UIAlertActionStyleDestructive),
    @"foo2" : @(UIAlertActionStyleDestructive),
    @"bar" : @(UIAlertActionStyleDefault),
    @"bar2" : @(UIAlertActionStyleDefault),
    @"testCancel" : @(UIAlertActionStyleCancel),
  };

  NSMutableDictionary* remainingActions = [actions mutableCopy];

  // Action.
  for (id key in actions) {
    UIAlertActionStyle style =
        static_cast<UIAlertActionStyle>([actions[key] integerValue]);
    [alertCoordinator addItemWithTitle:key action:nil style:style];
  }

  // Test.
  // Present the alert.
  startAlertCoordinator();

  // Get the alert.
  EXPECT_TRUE([viewController.presentedViewController
      isKindOfClass:[UIAlertController class]]);
  UIAlertController* alertController =
      base::mac::ObjCCastStrict<UIAlertController>(
          viewController.presentedViewController);

  // Test the results.
  for (UIAlertAction* action in alertController.actions) {
    EXPECT_TRUE([remainingActions objectForKey:action.title]);
    UIAlertActionStyle style =
        static_cast<UIAlertActionStyle>([actions[action.title] integerValue]);
    EXPECT_EQ(style, action.style);
    [remainingActions removeObjectForKey:action.title];
  }

  EXPECT_EQ(0LU, [remainingActions count]);
}

// Tests that only the first cancel action is added.
TEST_F(AlertCoordinatorTest, OnlyOneCancelAction) {
  // Setup.
  UIViewController* viewController = getViewController();
  AlertCoordinator* alertCoordinator = getAlertCoordinator(viewController);

  NSString* firstButtonTitle = @"Cancel1";

  // Action.
  [alertCoordinator addItemWithTitle:firstButtonTitle
                              action:nil
                               style:UIAlertActionStyleCancel];
  [alertCoordinator addItemWithTitle:@"Cancel2"
                              action:nil
                               style:UIAlertActionStyleCancel];

  // Test.
  // Present the alert.
  startAlertCoordinator();

  // Get the alert.
  EXPECT_TRUE([viewController.presentedViewController
      isKindOfClass:[UIAlertController class]]);
  UIAlertController* alertController =
      base::mac::ObjCCastStrict<UIAlertController>(
          viewController.presentedViewController);

  // Test the results.
  EXPECT_EQ(1LU, alertController.actions.count);

  UIAlertAction* action = [alertController.actions objectAtIndex:0];
  EXPECT_EQ(firstButtonTitle, action.title);
  EXPECT_EQ(UIAlertActionStyleCancel, action.style);
}

TEST_F(AlertCoordinatorTest, NoInteractionActionTest) {
  // Setup.
  UIViewController* viewController = getViewController();
  AlertCoordinator* alertCoordinator = getAlertCoordinator(viewController);

  __block BOOL blockCalled = NO;

  alertCoordinator.noInteractionAction = ^{
    blockCalled = YES;
  };

  startAlertCoordinator();

  // Action.
  [alertCoordinator stop];

  // Test.
  EXPECT_TRUE(blockCalled);
}

TEST_F(AlertCoordinatorTest, NoInteractionActionWithCancelTest) {
  // Setup.
  UIViewController* viewController = getViewController();
  AlertCoordinator* alertCoordinator = getAlertCoordinator(viewController);

  __block BOOL blockCalled = NO;

  alertCoordinator.noInteractionAction = ^{
    blockCalled = YES;
  };

  startAlertCoordinator();

  // Action.
  [alertCoordinator executeCancelHandler];
  [alertCoordinator stop];

  // Test.
  EXPECT_FALSE(blockCalled);
}
