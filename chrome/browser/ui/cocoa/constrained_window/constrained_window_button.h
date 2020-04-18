// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_CONSTRAINED_WINDOW_CONSTRAINED_WINDOW_BUTTON_H_
#define CHROME_BROWSER_UI_COCOA_CONSTRAINED_WINDOW_CONSTRAINED_WINDOW_BUTTON_H_

#import <Cocoa/Cocoa.h>

#import "ui/base/cocoa/tracking_area.h"

namespace constrained_window_button {
const CGFloat kButtonMinWidth = 72;
}

@protocol ConstrainedWindowButtonDrawableCell
@property (nonatomic, assign) BOOL isMouseInside;
- (BOOL)isEnabled;
- (BOOL)isHighlighted;
@end

// A push button for use in a constrained window. Specialized constrained
// windows that need a push button should use this class instead of NSButton.
@interface ConstrainedWindowButton : NSButton {
 @private
  ui::ScopedCrTrackingArea trackingArea_;
}

extern const CGFloat buttonMinWidth_;

// Draws the background and shadow inside |path| with the appropriate
// colors for |buttonState|, onto |view|.
+ (void)DrawBackgroundAndShadowForPath:(NSBezierPath*)path
                          withCell:(id<ConstrainedWindowButtonDrawableCell>)cell
                            inView:(NSView*)view;

// Draws the highlight inside |path|, with the color for |buttonState|,
// onto |view|.
+ (void)DrawInnerHighlightForPath:(NSBezierPath*)path
                         withCell:(id<ConstrainedWindowButtonDrawableCell>)cell
                           inView:(NSView*)view;

// Draws the border along |path|, with a color according to |buttonState|,
// onto |view|.
+ (void)DrawBorderForPath:(NSBezierPath*)path
                 withCell:(id<ConstrainedWindowButtonDrawableCell>)cell
                   inView:(NSView*)view;
@end

// A button cell used by ConstrainedWindowButton.
@interface ConstrainedWindowButtonCell :
    NSButtonCell<ConstrainedWindowButtonDrawableCell> {
 @private
  BOOL isMouseInside_;
}

@end

#endif  // CHROME_BROWSER_UI_COCOA_CONSTRAINED_WINDOW_CONSTRAINED_WINDOW_BUTTON_H_
