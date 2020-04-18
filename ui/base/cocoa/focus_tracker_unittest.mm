// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#import "ui/base/cocoa/focus_tracker.h"
#import "ui/base/test/cocoa_helper.h"

namespace {

class FocusTrackerTest : public ui::CocoaTest {
 public:
  void SetUp() override {
    ui::CocoaTest::SetUp();
    base::scoped_nsobject<NSView> view(
        [[NSView alloc] initWithFrame:NSZeroRect]);
    viewA_ = view.get();
    [[test_window() contentView] addSubview:viewA_];

    view.reset([[NSView alloc] initWithFrame:NSZeroRect]);
    viewB_ = view.get();
    [[test_window() contentView] addSubview:viewB_];
  }

 protected:
  NSView* viewA_;
  NSView* viewB_;
};

TEST_F(FocusTrackerTest, SaveRestore) {
  NSWindow* window = test_window();
  ASSERT_TRUE([window makeFirstResponder:viewA_]);
  base::scoped_nsobject<FocusTracker> tracker(
      [[FocusTracker alloc] initWithWindow:window]);
  // Give focus to |viewB_|, then try and restore it to view1.
  ASSERT_TRUE([window makeFirstResponder:viewB_]);
  EXPECT_TRUE([tracker restoreFocusInWindow:window]);
  EXPECT_EQ(viewA_, [window firstResponder]);
}

TEST_F(FocusTrackerTest, SaveRestoreWithTextView) {
  // Valgrind will complain if the text field has zero size.
  NSRect frame = NSMakeRect(0, 0, 100, 20);
  NSWindow* window = test_window();
  base::scoped_nsobject<NSTextField> text(
      [[NSTextField alloc] initWithFrame:frame]);
  [[window contentView] addSubview:text];

  ASSERT_TRUE([window makeFirstResponder:text]);
  base::scoped_nsobject<FocusTracker> tracker(
      [[FocusTracker alloc] initWithWindow:window]);
  // Give focus to |viewB_|, then try and restore it to the text field.
  ASSERT_TRUE([window makeFirstResponder:viewB_]);
  EXPECT_TRUE([tracker restoreFocusInWindow:window]);
  EXPECT_TRUE([[window firstResponder] isKindOfClass:[NSTextView class]]);
}

TEST_F(FocusTrackerTest, DontRestoreToViewNotInWindow) {
  NSWindow* window = test_window();
  base::scoped_nsobject<NSView> viewC(
      [[NSView alloc] initWithFrame:NSZeroRect]);
  [[window contentView] addSubview:viewC];

  ASSERT_TRUE([window makeFirstResponder:viewC]);
  base::scoped_nsobject<FocusTracker> tracker(
      [[FocusTracker alloc] initWithWindow:window]);

  // Give focus to |viewB_|, then remove viewC from the hierarchy and try
  // to restore focus.  The restore should fail.
  ASSERT_TRUE([window makeFirstResponder:viewB_]);
  [viewC removeFromSuperview];
  EXPECT_FALSE([tracker restoreFocusInWindow:window]);
}

TEST_F(FocusTrackerTest, DontRestoreFocusToViewInDifferentWindow) {
  NSWindow* window = test_window();
  ASSERT_TRUE([window makeFirstResponder:viewA_]);
  base::scoped_nsobject<FocusTracker> tracker(
      [[FocusTracker alloc] initWithWindow:window]);

  // Give focus to |viewB_|, then try and restore focus in a different
  // window.  It is ok to pass a nil NSWindow here because we only use
  // it for direct comparison.
  ASSERT_TRUE([window makeFirstResponder:viewB_]);
  EXPECT_FALSE([tracker restoreFocusInWindow:nil]);
}


}  // namespace
