// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_THEMED_WINDOW_H_
#define CHROME_BROWSER_UI_COCOA_THEMED_WINDOW_H_

#import <Cocoa/Cocoa.h>

namespace ui {
class ThemeProvider;
}
using ui::ThemeProvider;

// Bit flags; mix-and-match as necessary.
enum {
  THEMED_NORMAL    = 0,
  THEMED_INCOGNITO = 1 << 0,
  THEMED_POPUP     = 1 << 1,
  THEMED_DEVTOOLS  = 1 << 2
};
typedef NSUInteger ThemedWindowStyle;

// Indicates how the theme image should be aligned.
enum ThemeImageAlignment {
  // Aligns the top of the theme image with the top of the frame. Use this
  // for IDR_THEME_THEME_FRAME.*
  THEME_IMAGE_ALIGN_WITH_FRAME,
  // Aligns the top of the theme image with the top of the tabs.
  // Use this for IDR_THEME_TAB_BACKGROUND and IDR_THEME_TOOLBAR.
  THEME_IMAGE_ALIGN_WITH_TAB_STRIP
};

// Implemented by windows that support theming.
@interface NSWindow (ThemeProvider)
- (const ThemeProvider*)themeProvider;
- (ThemedWindowStyle)themedWindowStyle;
- (BOOL)inIncognitoMode;
// Return YES if using the system (i.e. non-custom) theme and Incognito mode.
- (BOOL)inIncognitoModeWithSystemTheme;
// Return YES if Incongnito, or a custom theme with a dark toolbar color or
// light tab text.
- (BOOL)hasDarkTheme;

// Returns the position in window coordinates that the top left of a theme
// image with |alignment| should be painted at. The result of this method can
// be used in conjunction with [NSGraphicsContext cr_setPatternPhase:] to set
// the offset of pattern colors.
- (NSPoint)themeImagePositionForAlignment:(ThemeImageAlignment)alignment;
@end

// Adopted by views that want to redraw when the theme changed, or when the
// window's active status changed.
@protocol ThemedWindowDrawing

// Called by the window controller when the theme changed.
- (void)windowDidChangeTheme;

// Called by the window controller when the window gained or lost main window
// status.
- (void)windowDidChangeActive;

@end

#endif  // CHROME_BROWSER_UI_COCOA_THEMED_WINDOW_H_
