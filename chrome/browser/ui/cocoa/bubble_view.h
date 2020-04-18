// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_BUBBLE_VIEW_H_
#define CHROME_BROWSER_UI_COCOA_BUBBLE_VIEW_H_

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"

// A view class that looks like a "bubble" with rounded corners and displays
// text inside. Can be themed. To put flush against the sides of a window, the
// corner flags can be adjusted.

// Constants that define where the bubble will have a rounded corner. If
// not set, the corner will be square.
enum {
  kRoundedTopLeftCorner = 1,
  kRoundedTopRightCorner = 1 << 1,
  kRoundedBottomLeftCorner = 1 << 2,
  kRoundedBottomRightCorner = 1 << 3,
  kRoundedAllCorners = kRoundedTopLeftCorner |
                       kRoundedTopRightCorner |
                       kRoundedBottomLeftCorner |
                       kRoundedBottomRightCorner
};

// Constants that affect where the text is positioned within the view. They
// are exposed in case anyone needs to use the padding to set the content string
// length appropriately based on available space (such as eliding a URL).
enum {
  kBubbleViewTextPositionX = 4,
  kBubbleViewTextPositionY = 2
};

@interface BubbleView : NSView {
 @private
  base::scoped_nsobject<NSString> content_;
  unsigned long cornerFlags_;
  // The window from which we get the theme used to draw. In some cases,
  // it might not be the window we're in. As a result, this may or may not
  // directly own us, so it needs to be weak to prevent a cycle.
  NSWindow* themeProvider_;
}

// Sets the string displayed in the bubble. A copy of the string is made.
- (void)setContent:(NSString*)content;

// Sets which corners will be rounded.
- (void)setCornerFlags:(unsigned long)flags;

// Sets the window whose theme is used to draw.
- (void)setThemeProvider:(NSWindow*)provider;

// The font used to display the content string.
- (NSFont*)font;

@end

// APIs exposed only for testing.
@interface BubbleView(TestingOnly)
- (NSString*)content;
- (unsigned long)cornerFlags;
@end

#endif  // CHROME_BROWSER_UI_COCOA_BUBBLE_VIEW_H_
