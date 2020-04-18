// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/background_gradient_view.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

// Since BackgroundGradientView doesn't do any drawing by default, we
// create a subclass to call its draw method for us.
@interface BackgroundGradientSubClassTest : BackgroundGradientView
@end

@implementation BackgroundGradientSubClassTest

- (void)drawRect:(NSRect)dirtyRect {
  [self drawBackground:dirtyRect];
}

@end

namespace {

class BackgroundGradientViewTest : public CocoaTest {
 public:
  BackgroundGradientViewTest() {
    NSRect frame = NSMakeRect(0, 0, 100, 30);
    base::scoped_nsobject<BackgroundGradientSubClassTest> view(
        [[BackgroundGradientSubClassTest alloc] initWithFrame:frame]);
    view_ = view.get();
    [[test_window() contentView] addSubview:view_];
  }

  BackgroundGradientSubClassTest* view_;
};

TEST_VIEW(BackgroundGradientViewTest, view_)

// Test drawing, mostly to ensure nothing leaks or crashes.
TEST_F(BackgroundGradientViewTest, DisplayWithDivider) {
  [view_ setShowsDivider:YES];
  [view_ display];
}

}  // namespace
