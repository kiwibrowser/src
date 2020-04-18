// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_NTP_NEW_TAB_PAGE_HEADER_CONSTANTS_H_
#define IOS_CHROME_BROWSER_UI_NTP_NEW_TAB_PAGE_HEADER_CONSTANTS_H_

#import <CoreGraphics/CoreGraphics.h>

namespace ntp_header {

// The minimum height of the new tab page header view when the new tab page is
// scrolled up.
extern const CGFloat kMinHeaderHeight;

// The scroll distance within which to animate the search field from its
// initial frame to its final full bleed frame.
extern const CGFloat kAnimationDistance;

CGFloat ToolbarHeight();

extern const CGFloat kScrolledToTopOmniboxBottomMargin;

extern const CGFloat kHintLabelSidePadding;
extern const CGFloat kHintLabelSidePaddingLegacy;

// The margin of the subviews of the fake omnibox when it is pinned to top.
extern const CGFloat kMaxHorizontalMarginDiff;

// The margin added to the fake omnibox to have at the right position.
extern const CGFloat kMaxTopMarginDiff;

}  // namespace ntp_header

#endif  // IOS_CHROME_BROWSER_UI_NTP_NEW_TAB_PAGE_HEADER_CONSTANTS_H_
