// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/autofill/autofill_bubble_controller.h"

#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "chrome/browser/ui/cocoa/test/run_loop_testing.h"

class AutofillBubbleControllerTest : public CocoaTest {
};

TEST_F(AutofillBubbleControllerTest, ShowAndClose) {
  AutofillBubbleController* controller =
      [[AutofillBubbleController alloc] initWithParentWindow:test_window()
                                                          message:@"test msg"];
  EXPECT_FALSE([[controller window] isVisible]);

  [controller showWindow:nil];
  EXPECT_TRUE([[controller window] isVisible]);

  // Close will self-delete, but all pending messages must be processed so the
  // deallocation happens.
  [controller close];
  chrome::testing::NSRunLoopRunAllPending();
}
