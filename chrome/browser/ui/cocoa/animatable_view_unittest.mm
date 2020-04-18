// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/animatable_view.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#import "chrome/browser/ui/cocoa/view_resizer_pong.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

namespace {

class AnimatableViewTest : public CocoaTest {
 public:
  AnimatableViewTest() {
    NSRect frame = NSMakeRect(0, 0, 100, 100);
    view_.reset([[AnimatableView alloc] initWithFrame:frame]);
    [[test_window() contentView] addSubview:view_.get()];

    resizeDelegate_.reset([[ViewResizerPong alloc] init]);
    [view_ setResizeDelegate:resizeDelegate_.get()];
  }

  base::scoped_nsobject<ViewResizerPong> resizeDelegate_;
  base::scoped_nsobject<AnimatableView> view_;
};

// Basic view tests (AddRemove, Display).
TEST_VIEW(AnimatableViewTest, view_);

TEST_F(AnimatableViewTest, GetAndSetHeight) {
  // Make sure the view's height starts out at 100.
  NSRect initialFrame = [view_ frame];
  ASSERT_EQ(100, initialFrame.size.height);
  EXPECT_EQ(initialFrame.size.height, [view_ height]);

  // Set it directly to 50 and make sure it takes effect.
  [resizeDelegate_ setHeight:-1];
  [view_ setHeight:50];
  EXPECT_EQ(50, [resizeDelegate_ height]);
}

// TODO(rohitrao): Find a way to unittest the animations and delegate messages.

}  // namespace
