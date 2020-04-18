// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_BROWSER_WINDOW_UTILS_H_
#define CHROME_BROWSER_UI_COCOA_BROWSER_WINDOW_UTILS_H_

#import <Cocoa/Cocoa.h>

#include "chrome/browser/ui/cocoa/themed_window.h"

namespace content {
struct NativeWebKeyboardEvent;
}

@interface BrowserWindowUtils : NSObject

// Returns YES if keyboard event should be handled.
+ (BOOL)shouldHandleKeyboardEvent:(const content::NativeWebKeyboardEvent&)event;

// Returns YES if keyboard event is a text editing command such as Copy or Paste
+ (BOOL)isTextEditingEvent:(const content::NativeWebKeyboardEvent&)event;

// Determines the command associated with the keyboard event.
// Returns -1 if no command found.
+ (int)getCommandId:(const content::NativeWebKeyboardEvent&)event;

// NSWindow must be a ChromeEventProcessingWindow.
+ (BOOL)handleKeyboardEvent:(NSEvent*)event
                   inWindow:(NSWindow*)window;

// Schedule a window title change in the next run loop iteration. This works
// around a Cocoa bug: if a window changes title during the tracking of the
// Window menu it doesn't display well and the constant re-sorting of the list
// makes it difficult for the user to pick the desired window.
// Passing in a non-nil oldTitle will also cancel any pending title changes with
// a matching window and title. This function returns a NSString* that can be
// passed in future calls as oldTitle.
+ (NSString*)scheduleReplaceOldTitle:(NSString*)oldTitle
                        withNewTitle:(NSString*)newTitle
                           forWindow:(NSWindow*)window;

// Returns the position in the coordinates of |windowView| that the top left of
// a theme image should be painted at. See
// [BrowserWindowController themeImagePositionForAlignment:] for more details.
+ (NSPoint)themeImagePositionFor:(NSView*)windowView
                    withTabStrip:(NSView*)tabStripView
                       alignment:(ThemeImageAlignment)alignment;

// Returns the position in the coordinates of |tabStripView| that the top left
// of a theme image should be painted at. This method exists so that the
// position can be queried by the new tab button before the tab strip is layed
// out.
+ (NSPoint)themeImagePositionInTabStripCoords:(NSView*)tabStripView
                                    alignment:(ThemeImageAlignment)alignment;

+ (void)activateWindowForController:(NSWindowController*)controller;
@end

#endif  // CHROME_BROWSER_UI_COCOA_BROWSER_WINDOW_UTILS_H_
