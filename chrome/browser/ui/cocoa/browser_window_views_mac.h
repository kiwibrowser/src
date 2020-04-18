// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_BROWSER_WINDOW_VIEWS_MAC_H_
#define CHROME_BROWSER_UI_COCOA_BROWSER_WINDOW_VIEWS_MAC_H_

// This file contains functions to support browser window code on Mac which may
// need to deal with either a views browser window or a Cocoa browser window.

// TODO(tapted): BrowserWindowController and TabWindowController shouldn't be
// visible here (or in any files that import this header). Declare a protocol
// instead that describes the dependencies needed outside of Cocoa-specific
// code.
@class BrowserWindowController;
@class NSWindow;
@class TabWindowController;

// Returns the BrowserWindowController backing a Cocoa browser window. Always
// returns nil if |window| is a views browser window.
BrowserWindowController* BrowserWindowControllerForWindow(NSWindow* window);

// Returns the TabWindowController backing a Cocoa browser window. Always
// returns nil if |window| is a views browser window.
TabWindowController* TabWindowControllerForWindow(NSWindow* window);

#endif  // CHROME_BROWSER_UI_COCOA_BROWSER_WINDOW_VIEWS_MAC_H_
