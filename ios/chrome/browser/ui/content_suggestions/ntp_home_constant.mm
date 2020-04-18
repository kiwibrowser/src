// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/content_suggestions/ntp_home_constant.h"
#include "ios/chrome/browser/ui/ui_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace ntp_home {
NSString* FakeOmniboxAccessibilityID() {
  return @"NTPHomeFakeOmniboxAccessibilityID";
}

const CGFloat kMostVisitedBottomMarginIPad = 80;
const CGFloat kMostVisitedBottomMarginIPhone = 60;
const CGFloat kSuggestionPeekingHeight = 60;

UIColor* kNTPBackgroundColor() {
  if (IsUIRefreshPhase1Enabled()) {
    return [UIColor colorWithWhite:0.98 alpha:1.0];
  } else {
    return [UIColor whiteColor];
  }
}

}  // namespace ntp_home
