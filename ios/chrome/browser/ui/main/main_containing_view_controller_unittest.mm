// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/main/main_containing_view_controller.h"

#import <UIKit/UIKit.h>

#import "base/test/ios/wait_util.h"
#include "base/test/scoped_feature_list.h"
#import "ios/chrome/browser/ui/main/main_view_controller_test.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher.h"
#import "ios/chrome/test/block_cleanup_test.h"
#include "testing/gtest_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface HiddenStatusBarViewController : UIViewController
@end

@implementation HiddenStatusBarViewController
- (BOOL)prefersStatusBarHidden {
  return YES;
}
@end

@interface MockViewController : UIViewController
@property(nonatomic, readonly) NSInteger didMoveToParentViewControllerCount;
@property(nonatomic, weak, readonly) id didMoveToParentViewControllerArgument;
@property(nonatomic, readonly) NSInteger willMoveToParentViewControllerCount;
@property(nonatomic, weak, readonly) id willMoveToParentViewControllerArgument;
@end

@implementation MockViewController
@synthesize didMoveToParentViewControllerCount =
    _didMoveToParentViewControllerCount;
@synthesize didMoveToParentViewControllerArgument =
    _didMoveToParentViewControllerArgument;
@synthesize willMoveToParentViewControllerCount =
    _willMoveToParentViewControllerCount;
@synthesize willMoveToParentViewControllerArgument =
    _willMoveToParentViewControllerArgument;

- (void)didMoveToParentViewController:(UIViewController*)parent {
  _didMoveToParentViewControllerCount++;
  _didMoveToParentViewControllerArgument = parent;
}

- (void)willMoveToParentViewController:(UIViewController*)parent {
  _willMoveToParentViewControllerCount++;
  _willMoveToParentViewControllerArgument = parent;
}

@end

namespace {

using MainContainingViewControllerTest = MainViewControllerTest;

TEST_F(MainContainingViewControllerTest, ActiveVC) {
  MainContainingViewController* main_view_controller =
      [[MainContainingViewController alloc] init];
  id<TabSwitcher> child_tab_switcher_1 = CreateTestTabSwitcher();
  id<TabSwitcher> child_tab_switcher_2 = CreateTestTabSwitcher();

  // Test that the active view controller is always the first child view
  // controller.
  EXPECT_EQ(nil, [main_view_controller activeViewController]);
  [main_view_controller
      addChildViewController:[child_tab_switcher_1 viewController]];
  EXPECT_EQ([child_tab_switcher_1 viewController],
            [main_view_controller activeViewController]);
  [main_view_controller
      addChildViewController:[child_tab_switcher_2 viewController]];
  EXPECT_EQ([child_tab_switcher_1 viewController],
            [main_view_controller activeViewController]);
  [[child_tab_switcher_1 viewController] removeFromParentViewController];
  EXPECT_EQ([child_tab_switcher_2 viewController],
            [main_view_controller activeViewController]);
}

TEST_F(MainContainingViewControllerTest, Setters) {
  MainContainingViewController* main_view_controller =
      [[MainContainingViewController alloc] init];
  CGRect windowRect = CGRectMake(0, 0, 200, 200);
  [main_view_controller view].frame = windowRect;

  MockViewController* child_view_controller = [[MockViewController alloc] init];
  child_view_controller.view.frame = CGRectMake(0, 0, 10, 10);

  [main_view_controller showTabViewController:child_view_controller
                                   completion:nil];
  EXPECT_EQ(child_view_controller,
            [[main_view_controller childViewControllers] firstObject]);
  EXPECT_EQ(child_view_controller.view,
            [[main_view_controller view].subviews firstObject]);
  EXPECT_NSEQ(NSStringFromCGRect(windowRect),
              NSStringFromCGRect(child_view_controller.view.frame));
  EXPECT_EQ(1, [child_view_controller didMoveToParentViewControllerCount]);
  EXPECT_EQ(main_view_controller,
            [child_view_controller didMoveToParentViewControllerArgument]);
  // Expect there to have also been an automatic call to
  // -willMoveToParentViewController.
  EXPECT_EQ(1, [child_view_controller willMoveToParentViewControllerCount]);
  EXPECT_EQ(main_view_controller,
            [child_view_controller willMoveToParentViewControllerArgument]);

  id<TabSwitcher> child_tab_switcher = CreateTestTabSwitcher();
  [main_view_controller showTabSwitcher:child_tab_switcher completion:nil];
  EXPECT_EQ([child_tab_switcher viewController],
            [[main_view_controller childViewControllers] firstObject]);
  EXPECT_EQ(1U, [[main_view_controller childViewControllers] count]);
  EXPECT_EQ(nil, [child_view_controller.view superview]);
  EXPECT_EQ(2, [child_view_controller willMoveToParentViewControllerCount]);
  EXPECT_EQ(nil,
            [child_view_controller willMoveToParentViewControllerArgument]);
  // Expect there to have also been an automatic call to
  // -didMoveToParentViewController.
  EXPECT_EQ(2, [child_view_controller willMoveToParentViewControllerCount]);
  EXPECT_EQ(nil, [child_view_controller didMoveToParentViewControllerArgument]);
}

TEST_F(MainContainingViewControllerTest, StatusBar) {
  MainContainingViewController* main_view_controller =
      [[MainContainingViewController alloc] init];
  SetRootViewController(main_view_controller);

  id<TabSwitcher> status_bar_visible_tab_switcher = CreateTestTabSwitcher();
  [main_view_controller showTabSwitcher:status_bar_visible_tab_switcher
                             completion:nil];
  // Check if the status bar is hidden by testing if the status bar rect is
  // CGRectZero.
  EXPECT_FALSE(CGRectEqualToRect(
      [UIApplication sharedApplication].statusBarFrame, CGRectZero));
  HiddenStatusBarViewController* status_bar_hidden_view_controller =
      [[HiddenStatusBarViewController alloc] init];
  [main_view_controller showTabViewController:status_bar_hidden_view_controller
                                   completion:nil];
  EXPECT_EQ([main_view_controller childViewControllerForStatusBarHidden],
            status_bar_hidden_view_controller);
  EXPECT_TRUE(CGRectEqualToRect(
      [UIApplication sharedApplication].statusBarFrame, CGRectZero));
}

// Tests that completion handlers are called properly after the new view
// controller is made active
TEST_F(MainContainingViewControllerTest, CompletionHandlers) {
  MainContainingViewController* main_view_controller =
      [[MainContainingViewController alloc] init];
  CGRect windowRect = CGRectMake(0, 0, 200, 200);
  [main_view_controller view].frame = windowRect;

  id<TabSwitcher> child_tab_switcher_1 = CreateTestTabSwitcher();
  [[child_tab_switcher_1 viewController] view].frame = CGRectMake(0, 0, 10, 10);

  id<TabSwitcher> child_tab_switcher_2 = CreateTestTabSwitcher();
  [[child_tab_switcher_2 viewController] view].frame =
      CGRectMake(20, 20, 10, 10);

  // Tests that the completion handler is called when there is no preexisting
  // active view controller.
  __block BOOL completion_handler_was_called = false;
  [main_view_controller showTabSwitcher:child_tab_switcher_1
                             completion:^{
                               completion_handler_was_called = YES;
                             }];
  base::test::ios::WaitUntilCondition(^bool() {
    return completion_handler_was_called;
  });
  ASSERT_TRUE(completion_handler_was_called);

  // Tests that the completion handler is called when replacing an existing
  // active view controller.
  completion_handler_was_called = NO;
  [main_view_controller showTabSwitcher:child_tab_switcher_2
                             completion:^{
                               completion_handler_was_called = YES;
                             }];
  base::test::ios::WaitUntilCondition(^bool() {
    return completion_handler_was_called;
  });
  ASSERT_TRUE(completion_handler_was_called);
}

}  // namespace
