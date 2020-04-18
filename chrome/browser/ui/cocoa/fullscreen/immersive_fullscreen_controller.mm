// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/fullscreen/immersive_fullscreen_controller.h"

#import "base/mac/mac_util.h"
#include "base/mac/sdk_forward_declarations.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "ui/base/cocoa/tracking_area.h"

namespace {

// The height from the top of the screen that will show the menubar.
const CGFloat kMenubarShowZoneHeight = 4;

// The height from the top of the screen that will hide the menubar.
// The value must be greater than the menubar's height of 22px.
const CGFloat kMenubarHideZoneHeight = 28;

}  // namespace

@interface ImmersiveFullscreenController () {
  BrowserWindowController* browserController_;  // weak

  // Used to track the mouse movements to show/hide the menu.
  base::scoped_nsobject<CrTrackingArea> trackingArea_;

  // The content view for the window.
  NSView* contentView_;  // weak

  // Tracks the currently requested system fullscreen mode, used to show or
  // hide the menubar. Its value is as follows:
  // + |kFullScreenModeNormal| - when the window is not main or not fullscreen,
  // + |kFullScreenModeHideDock| - when the user interacts with the top of the
  // screen
  // + |kFullScreenModeHideAll| - when the conditions don't meet the first two
  // modes.
  base::mac::FullScreenMode systemFullscreenMode_;

  // True if the menubar should be shown on the screen.
  BOOL isMenubarVisible_;
}

// Whether the current screen is expected to have a menubar, regardless of
// current visibility of the menubar.
- (BOOL)doesScreenHaveMenubar;

// Adjusts the AppKit Fullscreen options of the application.
- (void)setSystemFullscreenModeTo:(base::mac::FullScreenMode)mode;

// Sets |isMenubarVisible_|. If the value has changed, update the tracking
// area, the dock, and the menubar
- (void)setMenubarVisibility:(BOOL)visible;

// Methods that update and remove the tracking area.
- (void)updateTrackingArea;
- (void)removeTrackingArea;

@end

@implementation ImmersiveFullscreenController

- (instancetype)initWithBrowserController:(BrowserWindowController*)bwc {
  if ((self = [super init])) {
    browserController_ = bwc;
    systemFullscreenMode_ = base::mac::kFullScreenModeNormal;

    contentView_ = [[bwc window] contentView];
    DCHECK(contentView_);

    isMenubarVisible_ = NO;

    NSNotificationCenter* nc = [NSNotificationCenter defaultCenter];
    NSWindow* window = [browserController_ window];

    [nc addObserver:self
           selector:@selector(windowDidBecomeMain:)
               name:NSWindowDidBecomeMainNotification
             object:window];

    [nc addObserver:self
           selector:@selector(windowDidResignMain:)
               name:NSWindowDidResignMainNotification
             object:window];

    [self updateTrackingArea];
  }

  return self;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];

  [self removeTrackingArea];
  [self setSystemFullscreenModeTo:base::mac::kFullScreenModeNormal];

  [super dealloc];
}

- (void)updateMenuBarAndDockVisibility {
  BOOL isMouseOnScreen =
      NSMouseInRect([NSEvent mouseLocation],
                    [[browserController_ window] screen].frame, false);

  if (!isMouseOnScreen || ![browserController_ isInImmersiveFullscreen])
    [self setSystemFullscreenModeTo:base::mac::kFullScreenModeNormal];
  else if ([self shouldShowMenubar])
    [self setSystemFullscreenModeTo:base::mac::kFullScreenModeHideDock];
  else
    [self setSystemFullscreenModeTo:base::mac::kFullScreenModeHideAll];
}

- (BOOL)shouldShowMenubar {
  return [self doesScreenHaveMenubar] && isMenubarVisible_;
}

- (void)windowDidBecomeMain:(NSNotification*)notification {
  [self updateMenuBarAndDockVisibility];
}

- (void)windowDidResignMain:(NSNotification*)notification {
  [self updateMenuBarAndDockVisibility];
}

- (BOOL)doesScreenHaveMenubar {
  NSScreen* screen = [[browserController_ window] screen];
  NSScreen* primaryScreen = [[NSScreen screens] firstObject];
  BOOL isWindowOnPrimaryScreen = [screen isEqual:primaryScreen];

  BOOL eachScreenShouldHaveMenuBar = [NSScreen screensHaveSeparateSpaces];
  return eachScreenShouldHaveMenuBar ?: isWindowOnPrimaryScreen;
}

- (void)setSystemFullscreenModeTo:(base::mac::FullScreenMode)mode {
  if (mode == systemFullscreenMode_)
    return;

  if (systemFullscreenMode_ == base::mac::kFullScreenModeNormal)
    base::mac::RequestFullScreen(mode);
  else if (mode == base::mac::kFullScreenModeNormal)
    base::mac::ReleaseFullScreen(systemFullscreenMode_);
  else
    base::mac::SwitchFullScreenModes(systemFullscreenMode_, mode);

  systemFullscreenMode_ = mode;
}

- (void)setMenubarVisibility:(BOOL)visible {
  if (isMenubarVisible_ == visible)
    return;

  isMenubarVisible_ = visible;
  [self updateTrackingArea];
  [self updateMenuBarAndDockVisibility];
}

- (void)updateTrackingArea {
  [self removeTrackingArea];

  CGFloat trackingHeight =
      isMenubarVisible_ ? kMenubarHideZoneHeight : kMenubarShowZoneHeight;
  NSRect trackingFrame = [contentView_ bounds];
  trackingFrame.origin.y = NSMaxY(trackingFrame) - trackingHeight;
  trackingFrame.size.height = trackingHeight;

  // If we replace the tracking area with a new one under the cursor, the new
  // tracking area might not receive a |-mouseEntered:| or |-mouseExited| call.
  // As a result, we should also track the mouse's movements so that the
  // so the menubar won't get stuck.
  NSTrackingAreaOptions options =
      NSTrackingMouseEnteredAndExited | NSTrackingActiveInKeyWindow;
  if (isMenubarVisible_)
    options |= NSTrackingMouseMoved;

  // Create and add a new tracking area for |frame|.
  trackingArea_.reset([[CrTrackingArea alloc] initWithRect:trackingFrame
                                                   options:options
                                                     owner:self
                                                  userInfo:nil]);
  [contentView_ addTrackingArea:trackingArea_];
}

- (void)removeTrackingArea {
  if (trackingArea_) {
    [contentView_ removeTrackingArea:trackingArea_];
    trackingArea_.reset();
  }
}

- (void)mouseEntered:(NSEvent*)event {
  [self setMenubarVisibility:YES];
}

- (void)mouseExited:(NSEvent*)event {
  [self setMenubarVisibility:NO];
}

- (void)mouseMoved:(NSEvent*)event {
  [self setMenubarVisibility:[trackingArea_
                                 mouseInsideTrackingAreaForView:contentView_]];
}

@end
