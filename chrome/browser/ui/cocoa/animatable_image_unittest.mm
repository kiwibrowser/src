// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#import "chrome/browser/ui/cocoa/animatable_image.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

namespace {

class AnimatableImageTest : public CocoaTest {
 public:
  AnimatableImageTest() {
    NSRect frame = NSMakeRect(0, 0, 500, 500);
    NSImage* image = [NSImage imageNamed:NSImageNameComputer];
    animation_ = [[AnimatableImage alloc] initWithImage:image
                                         animationFrame:frame];
  }

  AnimatableImage* animation_;
};

TEST_F(AnimatableImageTest, BasicAnimation) {
  [animation_ setStartFrame:CGRectMake(0, 0, 10, 10)];
  [animation_ setEndFrame:CGRectMake(500, 500, 100, 100)];
  [animation_ setStartOpacity:0.1];
  [animation_ setEndOpacity:1.0];
  [animation_ setDuration:0.5];
  [animation_ startAnimation];
}

TEST_F(AnimatableImageTest, CancelAnimation) {
  [animation_ setStartFrame:CGRectMake(0, 0, 10, 10)];
  [animation_ setEndFrame:CGRectMake(500, 500, 100, 100)];
  [animation_ setStartOpacity:0.1];
  [animation_ setEndOpacity:1.0];
  [animation_ setDuration:5.0];  // Long enough to be able to test cancelling.
  [animation_ startAnimation];
  [animation_ close];
}

}  // namespace
