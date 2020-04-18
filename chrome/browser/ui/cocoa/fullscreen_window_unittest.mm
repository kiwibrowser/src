// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/mac/scoped_nsobject.h"
#include "chrome/browser/ui/cocoa/fullscreen_window.h"
#include "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"

@interface PerformCloseUIItem : NSObject<NSValidatedUserInterfaceItem>
@end

@implementation PerformCloseUIItem
- (SEL)action {
  return @selector(performClose:);
}

- (NSInteger)tag {
  return 0;
}
@end

class FullscreenWindowTest : public CocoaTest {
};

TEST_F(FullscreenWindowTest, Basics) {
  base::scoped_nsobject<FullscreenWindow> window;
  window.reset([[FullscreenWindow alloc] init]);

  EXPECT_EQ([NSScreen mainScreen], [window screen]);
  EXPECT_TRUE([window canBecomeKeyWindow]);
  EXPECT_TRUE([window canBecomeMainWindow]);
  EXPECT_EQ(NSBorderlessWindowMask, [window styleMask]);
  EXPECT_NSEQ([[NSScreen mainScreen] frame], [window frame]);
  EXPECT_FALSE([window isReleasedWhenClosed]);
}

TEST_F(FullscreenWindowTest, CanPerformClose) {
  base::scoped_nsobject<FullscreenWindow> window;
  window.reset([[FullscreenWindow alloc] init]);

  base::scoped_nsobject<PerformCloseUIItem> item;
  item.reset([[PerformCloseUIItem alloc] init]);

  EXPECT_TRUE([window validateUserInterfaceItem:item.get()]);
}
