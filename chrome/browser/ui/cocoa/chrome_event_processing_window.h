// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_CHROME_EVENT_PROCESSING_WINDOW_H_
#define CHROME_BROWSER_UI_COCOA_CHROME_EVENT_PROCESSING_WINDOW_H_

#import <Cocoa/Cocoa.h>

#import "base/mac/scoped_nsobject.h"
#import "ui/base/cocoa/command_dispatcher.h"
#import "ui/base/cocoa/underlay_opengl_hosting_window.h"

@class ChromeCommandDispatcherDelegate;

// Override NSWindow to access unhandled keyboard events (for command
// processing); subclassing NSWindow is the only method to do
// this.
@interface ChromeEventProcessingWindow
    : UnderlayOpenGLHostingWindow<CommandDispatchingWindow>

// Checks if |event| is a window, delayed window, or browser keyboard shortcut.
// (See global_keyboard_shortcuts_mac.h for details). If so, execute the
// associated command. Returns YES if the event was handled.
- (BOOL)handleExtraKeyboardShortcut:(NSEvent*)event;

@end

#endif  // CHROME_BROWSER_UI_COCOA_CHROME_EVENT_PROCESSING_WINDOW_H_
