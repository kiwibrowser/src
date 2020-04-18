// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/page_info/page_info_utils_cocoa.h"

namespace {
// The amount of horizontal space between the button's title and its arrow icon.
const CGFloat kButtonTitleRightPadding = 4.0f;
}

// Determine the size of a popup button with the given title.
NSSize SizeForPageInfoButtonTitle(NSPopUpButton* button, NSString* title) {
  NSDictionary* textAttributes =
      [[button attributedTitle] attributesAtIndex:0 effectiveRange:NULL];
  NSSize titleSize = [title sizeWithAttributes:textAttributes];

  NSRect frame = [button frame];
  NSRect titleRect = [[button cell] titleRectForBounds:frame];
  CGFloat width = titleSize.width + NSWidth(frame) - NSWidth(titleRect);

  return NSMakeSize(width + kButtonTitleRightPadding, NSHeight(frame));
}
