// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/color_chooser_mac.h"

#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "skia/ext/skia_utils_mac.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

@interface NSColorPanel (Private)
// Private method returning the NSColorPanel's target.
- (id)__target;
@end

namespace {

class ColorPanelCocoaTest : public CocoaTest {
  void SetUp() override {
    // Create the color panel and call Init() again to update its initial
    // window list to include it. The NSColorPanel cannot be dealloced, so
    // without this step the tests will fail complaining that not all windows
    // were closed.
    [[NSColorPanel sharedColorPanel] makeKeyAndOrderFront:nil];
    Init();
  }
};

TEST_F(ColorPanelCocoaTest, ClearTargetAndDelegateOnEnd) {
  NSColorPanel* nscolor_panel = [NSColorPanel sharedColorPanel];
  @autoreleasepool {
    EXPECT_TRUE([nscolor_panel respondsToSelector:@selector(__target)]);

    // Create a ColorPanelCocoa.
    ColorChooserMac* color_chooser_mac =
        ColorChooserMac::Open(nullptr, SK_ColorBLACK);

    // Confirm the NSColorPanel's configuration by the ColorChooserMac's
    // ColorPanelCocoa.
    EXPECT_TRUE([nscolor_panel delegate]);
    EXPECT_TRUE([nscolor_panel __target]);

    // Release the ColorPanelCocoa and confirm it's no longer the NSColorPanel's
    // target or delegate.
    color_chooser_mac->End();
  }
  EXPECT_EQ([nscolor_panel delegate], nil);
  EXPECT_EQ([nscolor_panel __target], nil);
}

TEST_F(ColorPanelCocoaTest, ClearTargetOnEnd) {
  NSColorPanel* nscolor_panel = [NSColorPanel sharedColorPanel];
  @autoreleasepool {
    EXPECT_TRUE([nscolor_panel respondsToSelector:@selector(__target)]);

    // Create a ColorPanelCocoa.
    ColorChooserMac* color_chooser_mac =
        ColorChooserMac::Open(nullptr, SK_ColorBLACK);

    // Confirm the NSColorPanel's configuration by the ColorChooserMac's
    // ColorPanelCocoa.
    EXPECT_TRUE([nscolor_panel delegate]);
    EXPECT_TRUE([nscolor_panel __target]);

    // Clear the delegate and release the ColorPanelCocoa.
    [nscolor_panel setDelegate:nil];

    // Release the ColorPanelCocoa.
    color_chooser_mac->End();
  }
  // Confirm the ColorPanelCocoa is no longer the NSColorPanel's target or
  // delegate. Previously the ColorPanelCocoa would not clear the target if
  // the delegate had already been cleared.
  EXPECT_EQ([nscolor_panel delegate], nil);
  EXPECT_EQ([nscolor_panel __target], nil);
}

TEST_F(ColorPanelCocoaTest, SetColor) {
  // Set the NSColor panel up with an intial color.
  NSColor* blue_color = [NSColor blueColor];
  NSColorPanel* nscolor_panel = [NSColorPanel sharedColorPanel];
  [nscolor_panel setColor:blue_color];
  EXPECT_TRUE([[nscolor_panel color] isEqual:blue_color]);

  // Create a ColorChooserMac and confirm the NSColorPanel gets its initial
  // color.
  SkColor initial_color = SK_ColorBLACK;
  ColorChooserMac* color_chooser_mac =
      ColorChooserMac::Open(nullptr, SK_ColorBLACK);

  EXPECT_NSEQ([nscolor_panel color],
              skia::SkColorToDeviceNSColor(initial_color));

  // Confirm that -[ColorPanelCocoa setColor:] sets the NSColorPanel's color.
  SkColor test_color = SK_ColorRED;
  color_chooser_mac->SetSelectedColor(test_color);

  EXPECT_NSEQ([nscolor_panel color], skia::SkColorToDeviceNSColor(test_color));

  // Clean up.
  color_chooser_mac->End();
}

}  // namespace
