// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/tabs/tab_view.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

namespace {

const float kTabWidth = 50;
const float kTabHeight = 30;

class TabViewTest : public CocoaTest {
 public:
  TabViewTest() {
    NSRect frame = NSMakeRect(0, 0, kTabWidth, kTabHeight);
    base::scoped_nsobject<TabView> view(
        [[TabView alloc] initWithFrame:frame controller:nil closeButton:nil]);
    view_ = view.get();
    [[test_window() contentView] addSubview:view_];
  }

  TabView* view_;
};

TEST_VIEW(TabViewTest, view_)

// Test drawing, mostly to ensure nothing leaks or crashes.
TEST_F(TabViewTest, Display) {
  for (int i = 0; i < 5; i++) {
    [view_ setHoverAlpha:i * 0.2];
    [view_ display];
  }
}

// Test it doesn't crash when asked for its menu with no TabController set.
TEST_F(TabViewTest, Menu) {
  EXPECT_FALSE([view_ menu]);
}

// Test that clicks outside of the visible boundaries are ignored.
TEST_F(TabViewTest, ClickOnlyInVisibleBounds) {
  NSPoint bottomLeftCorner = NSMakePoint(5, 0);
  EXPECT_TRUE([view_ hitTest:bottomLeftCorner]);

  NSPoint topLeftCorner = NSMakePoint(0, kTabHeight);
  EXPECT_FALSE([view_ hitTest:topLeftCorner]);

  NSPoint middle = NSMakePoint(kTabWidth / 2, kTabHeight / 2);
  EXPECT_TRUE([view_ hitTest:middle]);
}

}  // namespace
