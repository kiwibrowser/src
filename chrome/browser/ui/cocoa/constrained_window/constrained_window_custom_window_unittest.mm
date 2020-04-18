// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_custom_window.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"

class ConstrainedWindowCustomWindowTest : public CocoaTest {
};

// Simply test creating and drawing the window.
TEST_F(ConstrainedWindowCustomWindowTest, Basic) {
  base::scoped_nsobject<ConstrainedWindowCustomWindow> window(
      [[ConstrainedWindowCustomWindow alloc]
          initWithContentRect:NSMakeRect(0, 0, 10, 10)]);
  EXPECT_TRUE([window canBecomeKeyWindow]);
  EXPECT_FALSE([window canBecomeMainWindow]);

  [window makeKeyAndOrderFront:nil];
  [window display];
  [window close];
}
