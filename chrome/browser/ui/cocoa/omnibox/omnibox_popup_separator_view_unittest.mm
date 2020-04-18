// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/omnibox/omnibox_popup_separator_view.h"

#include "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"

class OmniboxPopupBottomSeparatorViewTest : public CocoaTest {
 public:
  OmniboxPopupBottomSeparatorViewTest() {
    NSView* contentView = [test_window() contentView];
    bottom_view_.reset([[OmniboxPopupBottomSeparatorView alloc]
        initWithFrame:[contentView bounds]]);
    [contentView addSubview:bottom_view_];
  }

 protected:
  base::scoped_nsobject<OmniboxPopupBottomSeparatorView> bottom_view_;

 private:
  DISALLOW_COPY_AND_ASSIGN(OmniboxPopupBottomSeparatorViewTest);
};

TEST_VIEW(OmniboxPopupBottomSeparatorViewTest, bottom_view_);

TEST_F(OmniboxPopupBottomSeparatorViewTest, PreferredHeight) {
  EXPECT_LT(0, [OmniboxPopupBottomSeparatorView preferredHeight]);
}

class OmniboxPopupTopSeparatorViewTest : public CocoaTest {
 public:
  OmniboxPopupTopSeparatorViewTest() {
    NSView* contentView = [test_window() contentView];
    top_view_.reset([[OmniboxPopupTopSeparatorView alloc]
        initWithFrame:[contentView bounds]]);
    [contentView addSubview:top_view_];
  }

 protected:
  base::scoped_nsobject<OmniboxPopupTopSeparatorView> top_view_;

 private:
  DISALLOW_COPY_AND_ASSIGN(OmniboxPopupTopSeparatorViewTest);
};

TEST_VIEW(OmniboxPopupTopSeparatorViewTest, top_view_);

TEST_F(OmniboxPopupTopSeparatorViewTest, PreferredHeight) {
  EXPECT_LT(0, [OmniboxPopupTopSeparatorView preferredHeight]);
}
