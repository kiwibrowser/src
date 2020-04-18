// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/main_content/main_content_coordinator.h"

#import "base/mac/foundation_util.h"
#import "ios/chrome/browser/ui/browser_list/browser.h"
#import "ios/chrome/browser/ui/coordinators/browser_coordinator+internal.h"
#import "ios/chrome/browser/ui/coordinators/browser_coordinator_test.h"
#import "ios/chrome/browser/ui/coordinators/test_browser_coordinator.h"
#import "ios/chrome/browser/ui/main_content/main_content_ui_state.h"
#import "ios/chrome/browser/ui/main_content/test/main_content_broadcast_test_util.h"
#import "ios/chrome/browser/ui/main_content/test/test_main_content_ui_observer.h"
#import "ios/chrome/browser/ui/main_content/test/test_main_content_ui_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#pragma mark - TestMainContentViewController

// Test implementation of MainContentViewController.
@interface TestMainContentViewController : UIViewController<MainContentUI>
@property(nonatomic, readonly) TestMainContentUIState* mainContentUIState;
@end

@implementation TestMainContentViewController
@synthesize mainContentUIState = _mainContentUIState;

- (instancetype)init {
  if (self = [super init])
    _mainContentUIState = [[TestMainContentUIState alloc] init];
  return self;
}

@end

#pragma mark - TestMainContentCoordinator

// Test version of MainContentCoordinator.
@interface TestMainContentCoordinator : MainContentCoordinator
@property(nonatomic, readonly) TestMainContentViewController* viewController;
@end

@implementation TestMainContentCoordinator
@synthesize viewController = _viewController;

- (instancetype)init {
  if (self = [super init])
    _viewController = [[TestMainContentViewController alloc] init];
  return self;
}

@end

#pragma mark - MainContentCoordinatorTest

// Test fixture for MainContentCoordinator.
class MainContentCoordinatorTest : public BrowserCoordinatorTest {
 protected:
  MainContentCoordinatorTest()
      : BrowserCoordinatorTest(),
        parent_([[TestBrowserCoordinator alloc] init]) {
    parent_.browser = GetBrowser();
  }

  // The coordinator to use as the parent for MainContentCoordinators.
  BrowserCoordinator* parent() { return parent_; }

 private:
  __strong BrowserCoordinator* parent_;
  DISALLOW_COPY_AND_ASSIGN(MainContentCoordinatorTest);
};

// Tests that a MainContentCoordinator's ui forwader starts broadcasting
// when started.
TEST_F(MainContentCoordinatorTest, BroadcastAfterStarting) {
  // Start a MainContentCoordinator and verify that its UI is being broadcasted.
  TestMainContentCoordinator* coordinator =
      [[TestMainContentCoordinator alloc] init];
  [parent() addChildCoordinator:coordinator];
  [coordinator start];
  VerifyMainContentUIBroadcast(coordinator.viewController.mainContentUIState,
                               GetBrowser()->broadcaster(),
                               true /*should_broadcast*/);
  // Stop the coordinator and verify that its UI is no longer being broadcasted.
  [coordinator stop];
  [parent() removeChildCoordinator:coordinator];
  VerifyMainContentUIBroadcast(coordinator.viewController.mainContentUIState,
                               GetBrowser()->broadcaster(),
                               false /*should_broadcast*/);
}

// Tests that a MainContentCoordinator's ui forwader stops broadcasting when
// a child MainContentCoordinator is started.
TEST_F(MainContentCoordinatorTest, StopBroadcastingForChildren) {
  // Start a MainContentCoordinator and start a child MainContentCoordinator,
  // then verify that the child's UI is being broadcast, not the parent's.
  TestMainContentCoordinator* parentMainCoordinator =
      [[TestMainContentCoordinator alloc] init];
  [parent() addChildCoordinator:parentMainCoordinator];
  [parentMainCoordinator start];
  TestMainContentCoordinator* childMainCoordinator =
      [[TestMainContentCoordinator alloc] init];
  [parentMainCoordinator addChildCoordinator:childMainCoordinator];
  [childMainCoordinator start];
  VerifyMainContentUIBroadcast(
      childMainCoordinator.viewController.mainContentUIState,
      GetBrowser()->broadcaster(), true /*should_broadcast*/);
  VerifyMainContentUIBroadcast(
      parentMainCoordinator.viewController.mainContentUIState,
      GetBrowser()->broadcaster(), false /*should_broadcast*/);
  // Stop the child coordinator and verify that the parent's UI is being
  // broadcast instead.
  [childMainCoordinator stop];
  [parentMainCoordinator removeChildCoordinator:childMainCoordinator];
  VerifyMainContentUIBroadcast(
      childMainCoordinator.viewController.mainContentUIState,
      GetBrowser()->broadcaster(), false /*should_broadcast*/);
  VerifyMainContentUIBroadcast(
      parentMainCoordinator.viewController.mainContentUIState,
      GetBrowser()->broadcaster(), true /*should_broadcast*/);
  [parentMainCoordinator stop];
  [parent() removeChildCoordinator:parentMainCoordinator];
}
