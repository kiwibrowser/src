// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#import "ios/chrome/browser/ui/side_swipe/side_swipe_controller.h"
#include "ios/web/public/test/test_web_thread_bundle.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#import "third_party/ocmock/gtest_support.h"
#include "third_party/ocmock/ocmock_extensions.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

class SideSwipeControllerTest : public PlatformTest {
 public:
  void SetUp() override {
    // Create a mock for the TabModel that owns the object under test.
    tab_model_ = [OCMockObject niceMockForClass:[TabModel class]];

    TestChromeBrowserState::Builder builder;
    browser_state_ = builder.Build();
    // Create the object to test.
    side_swipe_controller_ =
        [[SideSwipeController alloc] initWithTabModel:tab_model_
                                         browserState:browser_state_.get()];

    view_ = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 320, 240)];

    [side_swipe_controller_ addHorizontalGesturesToView:view_];
  };

  web::TestWebThreadBundle thread_bundle_;
  std::unique_ptr<TestChromeBrowserState> browser_state_;
  UIView* view_;
  TabModel* tab_model_;
  SideSwipeController* side_swipe_controller_;
};

TEST_F(SideSwipeControllerTest, TestConstructor) {
  EXPECT_TRUE(side_swipe_controller_);
}

TEST_F(SideSwipeControllerTest, TestSwipeRecognizers) {
  NSSet* recognizers = [side_swipe_controller_ swipeRecognizers];
  BOOL hasRecognizer = NO;
  for (UISwipeGestureRecognizer* swipeRecognizer in recognizers) {
    hasRecognizer = YES;
    EXPECT_TRUE(swipeRecognizer);
  }
  EXPECT_TRUE(hasRecognizer);
}

}  // anonymous namespace
