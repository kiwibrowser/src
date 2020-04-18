// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_FULLSCREEN_IMMERSIVE_FULLSCREEN_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_FULLSCREEN_IMMERSIVE_FULLSCREEN_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

@class BrowserWindowController;

// This class manages the menubar and dock visibility for Immersive Fullscreen.
// It uses a tracking area to show/hide the menubar if the user interacts with
// the top of the screen.
@interface ImmersiveFullscreenController : NSObject

// Designated initializer.
- (instancetype)initWithBrowserController:(BrowserWindowController*)bwc;

// Updates the menubar and dock visibility according the state of the
// immersive fullscreen.
- (void)updateMenuBarAndDockVisibility;

// Returns YES if the menubar should be shown in immersive fullscreen for the
// screen that contains the window.
- (BOOL)shouldShowMenubar;

@end

#endif  // CHROME_BROWSER_UI_COCOA_FULLSCREEN_IMMERSIVE_FULLSCREEN_CONTROLLER_H_