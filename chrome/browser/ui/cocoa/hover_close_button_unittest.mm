// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/hover_close_button.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"

class HoverCloseButtonTest : public CocoaTest {
 public:
  HoverCloseButtonTest() {
    NSRect content_frame = [[test_window() contentView] frame];
    button_.reset([[HoverCloseButton alloc] initWithFrame:content_frame]);
    [[test_window() contentView] addSubview:button_];
  }

 protected:
  base::scoped_nsobject<HoverCloseButton> button_;
};

class WebUIHoverCloseButtonTest : public CocoaTest {
 public:
  WebUIHoverCloseButtonTest() {
    NSRect content_frame = [[test_window() contentView] frame];
    button_.reset([[WebUIHoverCloseButton alloc] initWithFrame:content_frame]);
    [[test_window() contentView] addSubview:button_];
  }

 protected:
  base::scoped_nsobject<WebUIHoverCloseButton> button_;
};

TEST_VIEW(HoverCloseButtonTest, button_)

TEST_VIEW(WebUIHoverCloseButtonTest, button_)
