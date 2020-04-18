// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/main/main_presenting_view_controller.h"

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

namespace {

class MainPresentingViewControllerTest : public MainViewControllerTest {
 public:
  MainPresentingViewControllerTest() {
    main_view_controller_ = [[MainPresentingViewController alloc] init];
    main_view_controller_.animationsDisabledForTesting = YES;

    SetRootViewController(main_view_controller_);
    CGRect windowRect = CGRectMake(0, 0, 200, 200);
    main_view_controller_.view.frame = windowRect;

    tab_switcher_ = CreateTestTabSwitcher();
    [tab_switcher_ viewController].view.frame = CGRectMake(0, 0, 10, 10);

    normal_tab_view_controller_ = [[UIViewController alloc] init];
    normal_tab_view_controller_.view.frame = CGRectMake(20, 20, 10, 10);

    incognito_tab_view_controller_ = [[UIViewController alloc] init];
    incognito_tab_view_controller_.view.frame = CGRectMake(40, 40, 10, 10);
  }

  ~MainPresentingViewControllerTest() override {}

 protected:
  // The MainPresentingViewController that is under test.  The test fixture sets
  // this VC as the root VC for the window.
  MainPresentingViewController* main_view_controller_;

  // A tab switcher is created by the text fixture and is available for use in
  // tests.
  id<TabSwitcher> tab_switcher_;

  // The following view controllers are created by the test fixture and are
  // available for use in tests.
  UIViewController* normal_tab_view_controller_;
  UIViewController* incognito_tab_view_controller_;
};

// Tests that there is no activeViewController for a newly constructed instance.
TEST_F(MainPresentingViewControllerTest, NoActiveViewController) {
  EXPECT_EQ(nil, main_view_controller_.activeViewController);
}

// Tests that it is possible to set a TabViewController without first setting a
// TabSwitcher.
TEST_F(MainPresentingViewControllerTest, TabViewControllerBeforeTabSwitcher) {
  [main_view_controller_ showTabViewController:normal_tab_view_controller_
                                    completion:nil];
  EXPECT_EQ(normal_tab_view_controller_,
            main_view_controller_.activeViewController);

  // Now setting a TabSwitcher will make the switcher active.
  [main_view_controller_ showTabSwitcher:tab_switcher_ completion:nil];
  EXPECT_EQ([tab_switcher_ viewController],
            main_view_controller_.activeViewController);
}

// Tests that it is possible to set a TabViewController after setting a
// TabSwitcher.
TEST_F(MainPresentingViewControllerTest, TabViewControllerAfterTabSwitcher) {
  [main_view_controller_ showTabSwitcher:tab_switcher_ completion:nil];
  EXPECT_EQ([tab_switcher_ viewController],
            main_view_controller_.activeViewController);

  [main_view_controller_ showTabViewController:normal_tab_view_controller_
                                    completion:nil];
  EXPECT_EQ(normal_tab_view_controller_,
            main_view_controller_.activeViewController);

  // Showing the TabSwitcher again will make it active.
  [main_view_controller_ showTabSwitcher:tab_switcher_ completion:nil];
  EXPECT_EQ([tab_switcher_ viewController],
            main_view_controller_.activeViewController);
}

// Tests swapping between two TabViewControllers.
TEST_F(MainPresentingViewControllerTest, SwapTabViewControllers) {
  [main_view_controller_ showTabViewController:normal_tab_view_controller_
                                    completion:nil];
  EXPECT_EQ(normal_tab_view_controller_,
            main_view_controller_.activeViewController);

  [main_view_controller_ showTabViewController:incognito_tab_view_controller_
                                    completion:nil];
  EXPECT_EQ(incognito_tab_view_controller_,
            main_view_controller_.activeViewController);
}

// Tests calling showTabSwitcher twice in a row with the same VC.
TEST_F(MainPresentingViewControllerTest, ShowTabSwitcherTwice) {
  [main_view_controller_ showTabSwitcher:tab_switcher_ completion:nil];
  EXPECT_EQ([tab_switcher_ viewController],
            main_view_controller_.activeViewController);

  [main_view_controller_ showTabSwitcher:tab_switcher_ completion:nil];
  EXPECT_EQ([tab_switcher_ viewController],
            main_view_controller_.activeViewController);
}

// Tests calling showTabViewController twice in a row with the same VC.
TEST_F(MainPresentingViewControllerTest, ShowTabViewControllerTwice) {
  [main_view_controller_ showTabViewController:normal_tab_view_controller_
                                    completion:nil];
  EXPECT_EQ(normal_tab_view_controller_,
            main_view_controller_.activeViewController);

  [main_view_controller_ showTabViewController:normal_tab_view_controller_
                                    completion:nil];
  EXPECT_EQ(normal_tab_view_controller_,
            main_view_controller_.activeViewController);
}

// Tests that setting the active view controller work and that completion
// handlers are called properly after the new view controller is made active.
TEST_F(MainPresentingViewControllerTest, CompletionHandlers) {
  // Tests that the completion handler is called when showing the switcher.
  __block BOOL completion_handler_was_called = false;
  [main_view_controller_ showTabSwitcher:tab_switcher_
                              completion:^{
                                completion_handler_was_called = YES;
                              }];
  base::test::ios::WaitUntilCondition(^bool() {
    return completion_handler_was_called;
  });
  ASSERT_TRUE(completion_handler_was_called);

  // Tests that the completion handler is called when showing a tab view
  // controller.
  completion_handler_was_called = NO;
  [main_view_controller_ showTabViewController:normal_tab_view_controller_
                                    completion:^{
                                      completion_handler_was_called = YES;
                                    }];
  base::test::ios::WaitUntilCondition(^bool() {
    return completion_handler_was_called;
  });
  ASSERT_TRUE(completion_handler_was_called);

  // Tests that the completion handler is called when replacing an existing tab
  // view controller.
  completion_handler_was_called = NO;
  [main_view_controller_ showTabViewController:incognito_tab_view_controller_
                                    completion:^{
                                      completion_handler_was_called = YES;
                                    }];
  base::test::ios::WaitUntilCondition(^bool() {
    return completion_handler_was_called;
  });
  ASSERT_TRUE(completion_handler_was_called);
}

}  // namespace
