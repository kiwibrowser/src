// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/harmony_button.h"

#import "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"

namespace {

class HarmonyButtonTest : public ui::CocoaTest {
 public:
  HarmonyButtonTest() {
    base::scoped_nsobject<HarmonyButton> button(
        [[HarmonyButton alloc] initWithFrame:NSMakeRect(0, 0, 20, 20)]);
    button_ = button;
    [[test_window() contentView] addSubview:button_];
  }

 protected:
  HarmonyButton* button_;  // Weak, owned by test_window().
};

TEST_VIEW(HarmonyButtonTest, button_)

// Verify that displaying a button that's smaller than double the corner radius
// doesn't crash. (CGPathCreateWithRoundedRect throws an exception if you pass
// it a radius that's too big).
TEST_F(HarmonyButtonTest, TooSmall) {
  CGFloat cornerRadius = button_.layer.cornerRadius;
  EXPECT_LT(0, cornerRadius);
  [button_ setFrameSize:NSMakeSize(cornerRadius, cornerRadius)];
  [button_ layoutSubtreeIfNeeded];
  [button_ displayIfNeeded];
}

TEST(HarmonyButtonTestMore, NSViewRespondsToClipsToBounds) {
  // HarmonyButton implements an undocumented method to avoid having its shadow
  // clipped, so verify that it hasn't disappeared.
  EXPECT_TRUE([NSView instancesRespondToSelector:@selector(clipsToBounds)]);
}

}  // namespace
