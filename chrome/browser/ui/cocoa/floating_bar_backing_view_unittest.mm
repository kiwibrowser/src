// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/floating_bar_backing_view.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"

namespace {

class FloatingBarBackingViewTest : public CocoaTest {
 public:
  FloatingBarBackingViewTest() {
    NSRect content_frame = [[test_window() contentView] frame];
    base::scoped_nsobject<FloatingBarBackingView> view(
        [[FloatingBarBackingView alloc] initWithFrame:content_frame]);
    view_ = view.get();
    [[test_window() contentView] addSubview:view_];
  }

  FloatingBarBackingView* view_;  // Weak. Owned by the view hierarchy.
};

// Tests display, add/remove.
TEST_VIEW(FloatingBarBackingViewTest, view_);

}  // namespace
