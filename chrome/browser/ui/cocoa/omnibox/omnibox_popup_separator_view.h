// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_OMNIBOX_OMNIBOX_POPUP_SEPARATOR_VIEW_H_
#define CHROME_BROWSER_UI_COCOA_OMNIBOX_OMNIBOX_POPUP_SEPARATOR_VIEW_H_

#import <Cocoa/Cocoa.h>

#include "chrome/browser/ui/cocoa/background_gradient_view.h"

// A view used to draw a separator above omnibox popup.
@interface OmniboxPopupTopSeparatorView : BackgroundGradientView {
}

+ (CGFloat)preferredHeight;

@end

// A view used to draw a drop shadow beneath the omnibox popup.
@interface OmniboxPopupBottomSeparatorView : NSView {
 @private
  BOOL isDarkTheme_;
}

+ (CGFloat)preferredHeight;
- (instancetype)initWithFrame:(NSRect)frame forDarkTheme:(BOOL)isDarkTheme;

@end

#endif  // CHROME_BROWSER_UI_COCOA_OMNIBOX_OMNIBOX_POPUP_SEPARATOR_VIEW_H_
