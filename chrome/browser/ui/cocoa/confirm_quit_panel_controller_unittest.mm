// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/confirm_quit_panel_controller.h"

#include "chrome/browser/ui/cocoa/confirm_quit.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "testing/gtest_mac.h"
#include "ui/base/accelerators/platform_accelerator_cocoa.h"

namespace {

class ConfirmQuitPanelControllerTest : public CocoaTest {
 public:
  NSString* TestString(NSString* str) {
    str = [str stringByReplacingOccurrencesOfString:@"{Cmd}"
                                         withString:@"\u2318"];
    str = [str stringByReplacingOccurrencesOfString:@"{Ctrl}"
                                         withString:@"\u2303"];
    str = [str stringByReplacingOccurrencesOfString:@"{Opt}"
                                         withString:@"\u2325"];
    str = [str stringByReplacingOccurrencesOfString:@"{Shift}"
                                         withString:@"\u21E7"];
    return str;
  }
};


TEST_F(ConfirmQuitPanelControllerTest, ShowAndDismiss) {
  ConfirmQuitPanelController* controller =
      [ConfirmQuitPanelController sharedController];
  // Test singleton.
  EXPECT_EQ(controller, [ConfirmQuitPanelController sharedController]);
  [controller showWindow:nil];
  [controller dismissPanel];  // Releases self.
  // The controller should still be the singleton instance until after the
  // animation runs and the window closes. That will happen after this test body
  // finishes executing.
  EXPECT_EQ(controller, [ConfirmQuitPanelController sharedController]);
}

TEST_F(ConfirmQuitPanelControllerTest, KeyCombinationForAccelerator) {
  Class controller = [ConfirmQuitPanelController class];

  ui::PlatformAcceleratorCocoa item(@"q", NSCommandKeyMask);
  EXPECT_NSEQ(TestString(@"{Cmd}Q"),
              [controller keyCombinationForAccelerator:item]);

  ui::PlatformAcceleratorCocoa item2(@"c", NSCommandKeyMask | NSShiftKeyMask);
  EXPECT_NSEQ(TestString(@"{Cmd}{Shift}C"),
              [controller keyCombinationForAccelerator:item2]);

  ui::PlatformAcceleratorCocoa item3(@"h",
      NSCommandKeyMask | NSShiftKeyMask | NSAlternateKeyMask);
  EXPECT_NSEQ(TestString(@"{Cmd}{Opt}{Shift}H"),
              [controller keyCombinationForAccelerator:item3]);

  ui::PlatformAcceleratorCocoa item4(@"r",
      NSCommandKeyMask | NSShiftKeyMask | NSAlternateKeyMask |
      NSControlKeyMask);
  EXPECT_NSEQ(TestString(@"{Cmd}{Ctrl}{Opt}{Shift}R"),
              [controller keyCombinationForAccelerator:item4]);

  ui::PlatformAcceleratorCocoa item5(@"o", NSControlKeyMask);
  EXPECT_NSEQ(TestString(@"{Ctrl}O"),
              [controller keyCombinationForAccelerator:item5]);

  ui::PlatformAcceleratorCocoa item6(@"m", NSShiftKeyMask | NSControlKeyMask);
  EXPECT_NSEQ(TestString(@"{Ctrl}{Shift}M"),
              [controller keyCombinationForAccelerator:item6]);

  ui::PlatformAcceleratorCocoa item7(
      @"e", NSCommandKeyMask | NSAlternateKeyMask);
  EXPECT_NSEQ(TestString(@"{Cmd}{Opt}E"),
              [controller keyCombinationForAccelerator:item7]);
}

}  // namespace
