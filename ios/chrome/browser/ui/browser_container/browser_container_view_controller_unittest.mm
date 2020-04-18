// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/browser_container/browser_container_view_controller.h"

#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Fixture for BrowserContainerViewController testing.
class BrowserContainerViewControllerTest : public PlatformTest {
 protected:
  void SetUp() override {
    PlatformTest::SetUp();

    view_controller_ = [[BrowserContainerViewController alloc] init];
    ASSERT_TRUE(view_controller_);
    content_view_ = [[UIView alloc] init];
    ASSERT_TRUE(content_view_);
  }
  BrowserContainerViewController* view_controller_;
  UIView* content_view_;
};

// Tests adding a new content view when BrowserContainerViewController does not
// currently have a content view.
TEST_F(BrowserContainerViewControllerTest, AddingContentView) {
  ASSERT_FALSE([content_view_ superview]);

  [view_controller_ displayContentView:content_view_];
  EXPECT_EQ(view_controller_.view, content_view_.superview);
}

// Tests removing previously added content view.
TEST_F(BrowserContainerViewControllerTest, RemovingContentView) {
  [view_controller_ displayContentView:content_view_];
  ASSERT_EQ(view_controller_.view, content_view_.superview);

  [view_controller_ displayContentView:nil];
  EXPECT_FALSE([content_view_ superview]);
}

// Tests adding a new content view when BrowserContainerViewController already
// has a content view.
TEST_F(BrowserContainerViewControllerTest, ReplacingContentView) {
  [view_controller_ displayContentView:content_view_];
  ASSERT_EQ(view_controller_.view, content_view_.superview);

  UIView* content_view2 = [[UIView alloc] init];
  [view_controller_ displayContentView:content_view2];
  EXPECT_FALSE([content_view_ superview]);
  EXPECT_EQ(view_controller_.view, content_view2.superview);
}
