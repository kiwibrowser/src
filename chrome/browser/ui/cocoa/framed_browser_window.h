// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_FRAMED_BROWSER_WINDOW_H_
#define CHROME_BROWSER_UI_COCOA_FRAMED_BROWSER_WINDOW_H_

#import <Cocoa/Cocoa.h>

#include "chrome/browser/ui/cocoa/chrome_browser_window.h"

// Cocoa class representing a framed browser window.
// We need to override NSWindow with our own class since we need access to all
// unhandled keyboard events and subclassing NSWindow is the only method to do
// this. We also handle our own window controls and custom window frame drawing.
@interface FramedBrowserWindow : ChromeBrowserWindow {
 @private
  BOOL shouldHideTitle_;

  // Locks the window's frame and style mask. If it's set to YES, then the
  // frame and the style mask cannot be changed.
  BOOL styleMaskLock_;
}

// The amount of window background image that is painted at the top of the
// window, so that it shows behind the tap strip area.
+ (CGFloat)browserFrameViewPaintHeight;

// The style mask which -initWithContentRect: will use to create the window.
// May be overridden by subclasses.
+ (NSUInteger)defaultStyleMask;

- (id)initWithContentRect:(NSRect)contentRect NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithContentRect:(NSRect)contentRect
                          styleMask:(NSWindowStyleMask)style
                            backing:(NSBackingStoreType)bufferingType
                              defer:(BOOL)flag NS_UNAVAILABLE;

// Tells the window to suppress title drawing.
- (void)setShouldHideTitle:(BOOL)flag;

// When the lock is set to YES, the frame and style mask of the Window cannot be
// changed. This is used to prevent AppKit from making these unwanted changes
// to the window during exit fullscreen transition. It is very important to
// release this lock after the transition is completed.
- (void)setStyleMaskLock:(BOOL)lock;

// This method is overridden to prevent AppKit from  setting the style mask
// when frameAndStyleMaskLock_ is set to true.
- (void)setStyleMask:(NSUInteger)styleMask;

// Draws the window theme into the specified rect. Returns whether a theme was
// drawn (whether incognito or full pattern theme; an overlay image doesn't
// count).
+ (BOOL)drawWindowThemeInDirtyRect:(NSRect)dirtyRect
                           forView:(NSView*)view
                            bounds:(NSRect)bounds
              forceBlackBackground:(BOOL)forceBlackBackground;

// Gets the color to draw title text.
- (NSColor*)titleColor;

@end

@interface NSWindow (UndocumentedAPI)

// Undocumented Cocoa API to suppress drawing of the window's title.
// -setTitle: still works, but the title set only applies to the
// miniwindow and menus (and, importantly, Expose).  Overridden to
// return |shouldHideTitle_|.
-(BOOL)_isTitleHidden;

@end

#endif  // CHROME_BROWSER_UI_COCOA_FRAMED_BROWSER_WINDOW_H_
