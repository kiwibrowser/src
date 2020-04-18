// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_BACKGROUND_GRADIENT_VIEW_H_
#define CHROME_BROWSER_UI_COCOA_BACKGROUND_GRADIENT_VIEW_H_

#import <Cocoa/Cocoa.h>

#import "chrome/browser/ui/cocoa/themed_window.h"

// A custom view that draws a 'standard' background gradient.
// Base class for other Chromium views.
@interface BackgroundGradientView : NSView<ThemedWindowDrawing>

// Controls whether the bar draws a dividing line.
@property(nonatomic, assign) BOOL showsDivider;

// Controls where the bar draws a dividing line.
@property(nonatomic, assign) NSRectEdge dividerEdge;

// The color used for the bottom stroke. Public so subclasses can use.
- (NSColor*)strokeColor;

// The pattern phase that will be used by -drawBackground:.
// Defaults to align the top of the theme image with the top of the tabs.
// Views that draw at the bottom of the window (download bar) can override to
// change the pattern phase.
- (NSPoint)patternPhase;

// Draws the background image into the current NSGraphicsContext.
- (void)drawBackground:(NSRect)dirtyRect;

@end

#endif  // CHROME_BROWSER_UI_COCOA_BACKGROUND_GRADIENT_VIEW_H_
