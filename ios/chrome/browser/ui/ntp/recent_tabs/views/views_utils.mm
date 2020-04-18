// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/ntp/recent_tabs/views/views_utils.h"

#include "base/logging.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Text color.
const int kTextColorBlue = 0x4285f4;
const int kTextColorGray = 0x333333;

// Subtitle text color.
const int kSubtitleColorBlue = 0x7daeff;
const int kSubtitleColorGray = 0x969696;

// Colors for the icons.
const int kIconColorBlue = 0x4285f4;
const int kIconColorGray = 0x5a5a5a;

}  //  namespace

namespace recent_tabs {

UILabel* CreateMultilineLabel(NSString* text) {
  UILabel* label = [[UILabel alloc] initWithFrame:CGRectZero];
  [label setTranslatesAutoresizingMaskIntoConstraints:NO];
  [label setText:text];
  [label setLineBreakMode:NSLineBreakByWordWrapping];
  [label setNumberOfLines:0];
  [label setFont:[MDCTypography body1Font]];
  [label setTextColor:UIColorFromRGB(kTextColorGray)];
  [label setTextAlignment:NSTextAlignmentNatural];
  [label setBackgroundColor:[UIColor whiteColor]];
  return label;
}

UIColor* GetTextColorBlue() {
  return UIColorFromRGB(kTextColorBlue);
}

UIColor* GetTextColorGray() {
  return UIColorFromRGB(kTextColorGray);
}

UIColor* GetSubtitleColorBlue() {
  return UIColorFromRGB(kSubtitleColorBlue);
}

UIColor* GetSubtitleColorGray() {
  return UIColorFromRGB(kSubtitleColorGray);
}

UIColor* GetIconColorBlue() {
  return UIColorFromRGB(kIconColorBlue);
}

UIColor* GetIconColorGray() {
  return UIColorFromRGB(kIconColorGray);
}

}  // namespace recent_tabs
