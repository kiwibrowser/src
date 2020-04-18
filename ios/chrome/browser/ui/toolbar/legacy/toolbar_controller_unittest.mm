// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Foundation/Foundation.h>

#include "base/format_macros.h"
#import "ios/chrome/browser/ui/toolbar/legacy/toolbar_controller+protected.h"
#import "ios/chrome/browser/ui/toolbar/legacy/toolbar_controller.h"
#import "ios/chrome/browser/ui/ui_util.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// A constant holding some number of tabs that will trigger an easter egg.
const NSInteger kStackButtonEasterEggTabCount = kStackButtonMaxTabCount + 1;

// The easter egg string.
NSString* kStackButtonEasterEggString = @":)";

// Strings containing the max tab count and easter egg tab counts.
NSString* kStackButtonMaxTabCountString =
    [NSString stringWithFormat:@"%" PRIdNS, kStackButtonMaxTabCount];
NSString* kStackButtonEasterEggTabCountString =
    [NSString stringWithFormat:@"%" PRIdNS, kStackButtonEasterEggTabCount];

#pragma mark -

namespace {

class ToolbarControllerTest : public PlatformTest {
 protected:
  void SetUp() override {
    PlatformTest::SetUp();
    toolbarController_ =
        [[ToolbarController alloc] initWithStyle:ToolbarControllerStyleLightMode
                                      dispatcher:nil];
  }

  ToolbarController* toolbarController_;
};

// Verify that if tab count is set to zero, the title is blank, but the a11y
// value is 0.
//
// Note that the iPad doesn't have a |stackButton|, but setTabCount may still
// be invoked so this test covers that code path. The rest of the tab count
// tests don't do anything if run on an iPad.
TEST_F(ToolbarControllerTest, TestTabCountZero) {
  // On iPad, there is no |stackButton|, so the title should be NULL.
  NSString* expectedTitle = IsIPadIdiom() ? NULL : @"";

  [toolbarController_ setTabCount:0];
  EXPECT_NSEQ(expectedTitle, [toolbarController_ stackButton].currentTitle);
}

// Verify that when subsequent calls to tab count cross the max tab count
// threshold (increasing), the title is blank but the a11y value is set to the
// second value.
//
// Doesn't do anything when run on an iPad.
TEST_F(ToolbarControllerTest, TestTabCountBecomesEasterEgg_iPhoneOnly) {
  if (IsIPadIdiom())
    return;

  [toolbarController_ setTabCount:kStackButtonMaxTabCount];
  EXPECT_NSEQ(kStackButtonMaxTabCountString,
              [toolbarController_ stackButton].currentTitle);

  [toolbarController_ setTabCount:kStackButtonEasterEggTabCount];
  EXPECT_NSEQ(kStackButtonEasterEggString,
              [toolbarController_ stackButton].currentTitle);
}

// Verify that when subsequent calls to tab count cross the max tab count
// threshold (decreasing), title and a11y value are both set to the second
// value.
//
// Doesn't do anything when run on an iPad.
TEST_F(ToolbarControllerTest, TestTabCountStopsBeingEasterEgg_iPhoneOnly) {
  if (IsIPadIdiom())
    return;

  [toolbarController_ setTabCount:kStackButtonEasterEggTabCount];
  EXPECT_NSEQ(kStackButtonEasterEggString,
              [toolbarController_ stackButton].currentTitle);

  [toolbarController_ setTabCount:kStackButtonMaxTabCount];
  EXPECT_NSEQ(kStackButtonMaxTabCountString,
              [toolbarController_ stackButton].currentTitle);
}

}  // namespace
