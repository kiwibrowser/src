// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/debug/debugger.h"
#include "chrome/app/chrome_command_ids.h"
#import "chrome/browser/ui/cocoa/chrome_browser_window.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"

class ChromeBrowserWindowTest : public CocoaTest {
 public:
  void SetUp() override {
    CocoaTest::SetUp();
    // Create a window.
    const NSUInteger mask = NSTitledWindowMask | NSClosableWindowMask |
        NSMiniaturizableWindowMask | NSResizableWindowMask;
    window_ = [[ChromeBrowserWindow alloc]
               initWithContentRect:NSMakeRect(0, 0, 800, 600)
               styleMask:mask
               backing:NSBackingStoreBuffered
               defer:NO];
    if (base::debug::BeingDebugged()) {
      [window_ orderFront:nil];
    } else {
      [window_ orderBack:nil];
    }
  }

  void TearDown() override {
    [window_ close];
    CocoaTest::TearDown();
  }

  ChromeBrowserWindow* window_;
};

// Baseline test that the window creates, displays, closes, and
// releases.
TEST_F(ChromeBrowserWindowTest, ShowAndClose) {
  [window_ display];
}

