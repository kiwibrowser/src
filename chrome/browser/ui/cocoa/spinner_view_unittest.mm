// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/spinner_view.h"

#include "base/mac/mac_util.h"
#import "ui/base/test/cocoa_helper.h"

namespace {

class SpinnerViewTest : public ui::CocoaTest {
 public:
  SpinnerViewTest() {
    CGRect frame = NSMakeRect(0.0, 0.0, 16.0, 16.0);
    view_.reset([[SpinnerView alloc] initWithFrame:frame]);
    [[test_window() contentView] addSubview:view_];
  }

  base::scoped_nsobject<SpinnerView> view_;
};

TEST_VIEW(SpinnerViewTest, view_)

TEST_F(SpinnerViewTest, StopAnimationOnMiniaturize) {
  if (base::mac::IsOS10_10())
    return;  // Fails when swarmed. http://crbug.com/660582
  EXPECT_TRUE([view_ isAnimating]);

  [test_window() miniaturize:nil];
  EXPECT_FALSE([view_ isAnimating]);

  [test_window() deminiaturize:nil];
  EXPECT_TRUE([view_ isAnimating]);
}

TEST_F(SpinnerViewTest, StopAnimationOnRemoveFromSuperview) {
  EXPECT_TRUE([view_ isAnimating]);

  [view_ removeFromSuperview];
  EXPECT_FALSE([view_ isAnimating]);

  [[test_window() contentView] addSubview:view_];
  EXPECT_TRUE([view_ isAnimating]);
}

TEST_F(SpinnerViewTest, StopAnimationOnHidden) {
  EXPECT_TRUE([view_ isAnimating]);

  [view_ setHidden:YES];
  EXPECT_FALSE([view_ isAnimating]);

  [view_ setHidden:NO];
  EXPECT_TRUE([view_ isAnimating]);
}

} // namespace
