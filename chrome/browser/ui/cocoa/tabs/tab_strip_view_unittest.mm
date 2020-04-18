// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/tabs/tab_strip_view.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

namespace {

class TabStripViewTest : public CocoaTest {
 public:
  TabStripViewTest() {
    NSRect frame = NSMakeRect(0, 0, 100, 30);
    base::scoped_nsobject<TabStripView> view(
        [[TabStripView alloc] initWithFrame:frame]);
    view_ = view.get();
    [[test_window() contentView] addSubview:view_];
  }

  TabStripView* view_;
};

TEST_VIEW(TabStripViewTest, view_)

}  // namespace
