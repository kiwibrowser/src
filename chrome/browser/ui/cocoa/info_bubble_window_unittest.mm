// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <Carbon/Carbon.h>

#include "base/debug/debugger.h"
#include "base/mac/scoped_nsobject.h"
#include "base/run_loop.h"
#import "chrome/browser/ui/cocoa/base_bubble_controller.h"
#include "chrome/browser/ui/cocoa/info_bubble_window.h"
#include "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "chrome/browser/ui/cocoa/test/run_loop_testing.h"
#include "ui/events/test/cocoa_test_event_utils.h"

// Mock BaseBubbleController to pick up -cancel:, but don't call the designated
// initializer in order to test just InfoBubbleWindow.
@interface InfoBubbleWindowController : BaseBubbleController
@end

@implementation InfoBubbleWindowController
- (IBAction)cancel:(id)sender {
  [self close];
}
@end

class InfoBubbleWindowTest : public CocoaTest {
 public:
  void SetUp() override {
    CocoaTest::SetUp();
    window_.reset(
        [[InfoBubbleWindow alloc] initWithContentRect:NSMakeRect(0, 0, 10, 10)
                                            styleMask:NSBorderlessWindowMask
                                              backing:NSBackingStoreBuffered
                                                defer:NO]);
    [window_ setAllowedAnimations:info_bubble::kAnimateNone];
    controller_.reset(
        [[InfoBubbleWindowController alloc] initWithWindow:window_]);
  }

  void TearDown() override {
    // Both controller and window need to be closed & released before TearDown,
    // or CocoaTest will consider the window still open and spinwait for it to
    // close.
    [controller_ close];
    chrome::testing::NSRunLoopRunAllPending();
    controller_.reset();
    window_.reset();

    CocoaTest::TearDown();
  }

  base::scoped_nsobject<InfoBubbleWindow> window_;
  base::scoped_nsobject<NSWindowController> controller_;
};

TEST_F(InfoBubbleWindowTest, Basics2) {
  EXPECT_TRUE([window_ canBecomeKeyWindow]);
  EXPECT_FALSE([window_ canBecomeMainWindow]);

  EXPECT_TRUE([window_ isExcludedFromWindowsMenu]);
}

TEST_F(InfoBubbleWindowTest, EscapeCloses) {
  [controller_ showWindow:nil];
  EXPECT_TRUE([window_ isVisible]);

  [window_ performKeyEquivalent:cocoa_test_event_utils::KeyEventWithKeyCode(
                                    kVK_Escape, '\e', NSKeyDown, 0)];
  EXPECT_FALSE([window_ isVisible]);
}

TEST_F(InfoBubbleWindowTest, CommandPeriodCloses) {
  [controller_ showWindow:nil];
  EXPECT_TRUE([window_ isVisible]);

  [window_ performKeyEquivalent:cocoa_test_event_utils::KeyEventWithKeyCode(
                                    kVK_ANSI_Period, '.', NSKeyDown,
                                    NSCommandKeyMask)];
  EXPECT_FALSE([window_ isVisible]);
}
