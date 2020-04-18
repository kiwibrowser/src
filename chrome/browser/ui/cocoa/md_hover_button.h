// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_MD_HOVER_BUTTON_H_
#define CHROME_BROWSER_UI_COCOA_MD_HOVER_BUTTON_H_

#import <AppKit/AppKit.h>

#import "chrome/browser/ui/cocoa/themed_window.h"
#import "ui/base/cocoa/hover_button.h"
#include "ui/gfx/vector_icon_types.h"

// MDHoverButton has a gray background with rounded corners. The background is
// only visible on hover and gets darker on click. It's friendly to subviews.

@interface MDHoverButton : HoverButton<ThemedWindowDrawing>

// An icon that's displayed in the middle of the button.
@property(nonatomic) const gfx::VectorIcon* icon;
@property(nonatomic) int iconSize;

// If YES, the button doesn't have a hover state. This can be useful if a
// button contains another button, or has an area which shouldn't be sensitive
// to clicks: set hoverSuppressed to YES when the mouse enters that area, and
// NO when the mouse exits.
@property(nonatomic) BOOL hoverSuppressed;
@end

#endif  // CHROME_BROWSER_UI_COCOA_MD_HOVER_BUTTON_H_
