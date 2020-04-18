// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_FULLSCREEN_WINDOW_H_
#define CHROME_BROWSER_UI_COCOA_FULLSCREEN_WINDOW_H_

#include <Cocoa/Cocoa.h>
#import "chrome/browser/ui/cocoa/chrome_browser_window.h"

// A FullscreenWindow is a borderless window suitable for going fullscreen.  The
// returned window is NOT release when closed and is not initially visible.
// FullscreenWindow derives from ChromeBrowserWindow to inherit hole punching,
// theming methods, and special event handling
// (e.g. handleExtraKeyboardShortcut).
@interface FullscreenWindow : ChromeBrowserWindow

// Initialize a FullscreenWindow for the given screen.
// Designated initializer.
- (id)initForScreen:(NSScreen*)screen;

@end

#endif  // CHROME_BROWSER_UI_COCOA_FULLSCREEN_WINDOW_H_
