// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/autofill/layout_view.h"

#include "base/mac/scoped_nsobject.h"
#include "chrome/browser/ui/cocoa/autofill/simple_grid_layout.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#import "ui/base/test/cocoa_helper.h"

namespace {

class LayoutViewTest : public ui::CocoaTest {
 public:
  LayoutViewTest() {
    CocoaTest::SetUp();
    view_.reset([[LayoutView alloc] initWithFrame:NSZeroRect]);
    [view_ setLayoutManager:std::unique_ptr<SimpleGridLayout>(
                                new SimpleGridLayout(view_))];
    layout_ = [view_ layoutManager];
    [[test_window() contentView] addSubview:view_];
  }

 protected:
  base::scoped_nsobject<LayoutView> view_;
  SimpleGridLayout* layout_;  // weak, owned by view_.
};

}  // namespace

TEST_VIEW(LayoutViewTest, view_)

TEST_F(LayoutViewTest, setFrameInvokesLayout) {
  EXPECT_FLOAT_EQ(0, NSWidth([view_ frame]));
  EXPECT_FLOAT_EQ(0, NSHeight([view_ frame]));

  ColumnSet* cs = layout_->AddColumnSet(0);
  cs->AddColumn(0.4);
  cs->AddColumn(0.6);
  layout_->StartRow(0, 0);
  base::scoped_nsobject<NSView> childView1(
      [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 0, 45)]);
  base::scoped_nsobject<NSView> childView2(
      [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 20, 55)]);
  layout_->AddView(childView1);
  layout_->AddView(childView2);

  ASSERT_EQ(2U, [[view_ subviews] count]);
  EXPECT_FLOAT_EQ(0, NSWidth([view_ frame]));
  EXPECT_FLOAT_EQ(0, NSHeight([view_ frame]));

  [view_ setFrame:NSMakeRect(0, 0, 40, 60)];
  EXPECT_FLOAT_EQ(40, NSWidth([view_ frame]));
  EXPECT_FLOAT_EQ(60, NSHeight([view_ frame]));

  NSView* subview = [[view_ subviews] objectAtIndex:0];
  EXPECT_FLOAT_EQ(16, NSWidth([subview frame]));
  EXPECT_FLOAT_EQ(55, NSHeight([subview frame]));

  subview = [[view_ subviews] objectAtIndex:1];
  EXPECT_FLOAT_EQ(24, NSWidth([subview frame]));
  EXPECT_FLOAT_EQ(55, NSHeight([subview frame]));
}
