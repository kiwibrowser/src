// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/fullscreen_window.h"

#import "chrome/browser/ui/cocoa/themed_window.h"

@implementation FullscreenWindow

// Make sure our designated initializer gets called.
- (id)init {
  return [self initForScreen:[NSScreen mainScreen]];
}

- (id)initForScreen:(NSScreen*)screen {
  NSRect contentRect;
  contentRect.origin = NSZeroPoint;
  contentRect.size = [screen frame].size;

  if ((self = [super initWithContentRect:contentRect
                               styleMask:NSBorderlessWindowMask
                                 backing:NSBackingStoreBuffered
                                   defer:NO
                                  screen:screen])) {
    [self setReleasedWhenClosed:NO];
    // Borderless windows don't usually show up in the Windows menu so whine at
    // Cocoa until it complies. See -dealloc and -setTitle: as well.
    [NSApp addWindowsItem:self title:@"" filename:NO];
    [[self contentView] setWantsLayer:YES];
  }
  return self;
}

- (void)dealloc {
  // Paranoia; doesn't seem to be necessary but it doesn't hurt.
  [NSApp removeWindowsItem:self];

  [super dealloc];
}

- (void)setTitle:(NSString *)title {
  [NSApp changeWindowsItem:self title:title filename:NO];
  [super setTitle:title];
}

// According to
// http://www.cocoabuilder.com/archive/message/cocoa/2006/6/19/165953 ,
// NSBorderlessWindowMask windows cannot become key or main.
// In our case, however, we don't want that behavior, so we override
// canBecomeKeyWindow and canBecomeMainWindow.

- (BOOL)canBecomeKeyWindow {
  return YES;
}

- (BOOL)canBecomeMainWindow {
  return YES;
}

// When becoming/resigning main status, explicitly set the background color,
// which is required by |TabView|.
- (void)becomeMainWindow {
  [super becomeMainWindow];
  [self setBackgroundColor:[NSColor windowFrameColor]];
}

- (void)resignMainWindow {
  [super resignMainWindow];
  [self setBackgroundColor:[NSColor windowBackgroundColor]];
}

// We need our own version, since the default one wants to flash the close
// button (and possibly other things), which results in nothing happening.
- (void)performClose:(id)sender {
  BOOL shouldClose = YES;

  // If applicable, check if this window should close.
  id delegate = [self delegate];
  if ([delegate respondsToSelector:@selector(windowShouldClose:)])
    shouldClose = [delegate windowShouldClose:self];

  if (shouldClose) {
    [self close];
  }
}

- (BOOL)validateUserInterfaceItem:(id<NSValidatedUserInterfaceItem>)item {
  SEL action = [item action];

  // Explicitly enable |-performClose:| (see above); otherwise the fact that
  // this window does not have a close button results in it being disabled.
  if (action == @selector(performClose:))
    return YES;

  return [super validateUserInterfaceItem:item];
}

@end
