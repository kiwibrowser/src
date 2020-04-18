// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/framed_browser_window.h"

#include <math.h>
#include <objc/runtime.h>
#include <stddef.h>

#include "base/logging.h"
#include "base/mac/foundation_util.h"
#include "base/mac/sdk_forward_declarations.h"
#include "chrome/browser/global_keyboard_shortcuts_mac.h"
#include "chrome/browser/profiles/profile_avatar_icon_util.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/themes/theme_service.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/browser_window_layout.h"
#import "chrome/browser/ui/cocoa/browser_window_touch_bar.h"
#import "chrome/browser/ui/cocoa/browser_window_utils.h"
#include "chrome/browser/ui/cocoa/l10n_util.h"
#import "chrome/browser/ui/cocoa/themed_window.h"
#include "chrome/grit/theme_resources.h"
#include "ui/base/cocoa/cocoa_base_utils.h"
#include "ui/base/cocoa/nsgraphics_context_additions.h"
#import "ui/base/cocoa/nsview_additions.h"
#import "ui/base/cocoa/touch_bar_forward_declarations.h"

@implementation FramedBrowserWindow

+ (CGFloat)browserFrameViewPaintHeight {
  return chrome::kTabStripHeight;
}

+ (NSUInteger)defaultStyleMask {
  return NSTitledWindowMask | NSClosableWindowMask |
         NSMiniaturizableWindowMask | NSResizableWindowMask |
         NSTexturedBackgroundWindowMask;
}

- (void)setStyleMask:(NSUInteger)styleMask {
  if (styleMaskLock_)
    return;
  [super setStyleMask:styleMask];
}

- (id)initWithContentRect:(NSRect)contentRect {
  if ((self = [super initWithContentRect:contentRect
                               styleMask:[[self class] defaultStyleMask]
                                 backing:NSBackingStoreBuffered
                                   defer:YES])) {
    // The 10.6 fullscreen code copies the title to a different window, which
    // will assert if it's nil.
    [self setTitle:@""];
  }

  return self;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [super dealloc];
}

- (NSUserInterfaceLayoutDirection)windowTitlebarLayoutDirection
    NS_AVAILABLE_MAC(10_12) {
  if (!cocoa_l10n_util::ShouldFlipWindowControlsInRTL())
    return NSUserInterfaceLayoutDirectionLeftToRight;
  return [super windowTitlebarLayoutDirection];
}

- (void)setShouldHideTitle:(BOOL)flag {
  shouldHideTitle_ = flag;
}

- (void)setStyleMaskLock:(BOOL)lock {
  styleMaskLock_ = lock;
}

- (BOOL)_isTitleHidden {
  return shouldHideTitle_;
}

- (BOOL)makeFirstResponder:(NSResponder*)responder {
  BrowserWindowController* bwc =
      [BrowserWindowController browserWindowControllerForWindow:self];
  [bwc firstResponderUpdated:responder];
  return [super makeFirstResponder:responder];
}

// This method is called whenever a window is moved in order to ensure it fits
// on the screen.  We cannot always handle resizes without breaking, so we
// prevent frame constraining in those cases.
- (NSRect)constrainFrameRect:(NSRect)frame toScreen:(NSScreen*)screen {
  // Do not constrain the frame rect if our delegate says no.  In this case,
  // return the original (unconstrained) frame.
  id delegate = [self delegate];
  if ([delegate respondsToSelector:@selector(shouldConstrainFrameRect)] &&
      ![delegate shouldConstrainFrameRect])
    return frame;

  return [super constrainFrameRect:frame toScreen:screen];
}

+ (BOOL)drawWindowThemeInDirtyRect:(NSRect)dirtyRect
                           forView:(NSView*)view
                            bounds:(NSRect)bounds
              forceBlackBackground:(BOOL)forceBlackBackground {
  const ui::ThemeProvider* themeProvider = [[view window] themeProvider];
  if (!themeProvider)
    return NO;

  ThemedWindowStyle windowStyle = [[view window] themedWindowStyle];

  // Devtools windows don't get themed.
  if (windowStyle & THEMED_DEVTOOLS)
    return NO;

  BOOL active = [[view window] isMainWindow];
  BOOL incognito = windowStyle & THEMED_INCOGNITO;
  BOOL popup = windowStyle & THEMED_POPUP;

  // Find a theme image.
  NSColor* themeImageColor = nil;
  if (!popup) {
    int themeImageID;
    if (active && incognito)
      themeImageID = IDR_THEME_FRAME_INCOGNITO;
    else if (active && !incognito)
      themeImageID = IDR_THEME_FRAME;
    else if (!active && incognito)
      themeImageID = IDR_THEME_FRAME_INCOGNITO_INACTIVE;
    else
      themeImageID = IDR_THEME_FRAME_INACTIVE;
    if (themeProvider->HasCustomImage(IDR_THEME_FRAME))
      themeImageColor = themeProvider->GetNSImageColorNamed(themeImageID);
  }

  BOOL themed = NO;
  if (themeImageColor) {
    // Default to replacing any existing pixels with the theme image, but if
    // asked paint black first and blend the theme with black.
    NSCompositingOperation operation = NSCompositeCopy;
    if (forceBlackBackground) {
      [[NSColor blackColor] set];
      NSRectFill(dirtyRect);
      operation = NSCompositeSourceOver;
    }

    NSPoint position = [[view window] themeImagePositionForAlignment:
        THEME_IMAGE_ALIGN_WITH_FRAME];
    [[NSGraphicsContext currentContext] cr_setPatternPhase:position
                                                   forView:view];

    [themeImageColor set];
    NSRectFillUsingOperation(dirtyRect, operation);
    themed = YES;
  }

  // Check to see if we have an overlay image.
  NSImage* overlayImage = nil;
  if (themeProvider->HasCustomImage(IDR_THEME_FRAME_OVERLAY) && !incognito &&
      !popup) {
    overlayImage = themeProvider->
        GetNSImageNamed(active ? IDR_THEME_FRAME_OVERLAY :
                                 IDR_THEME_FRAME_OVERLAY_INACTIVE);
  }

  if (overlayImage) {
    // Anchor to top-left and don't scale.
    NSPoint position = [[view window] themeImagePositionForAlignment:
        THEME_IMAGE_ALIGN_WITH_FRAME];
    position = [view convertPoint:position fromView:nil];
    NSSize overlaySize = [overlayImage size];
    NSRect imageFrame = NSMakeRect(0, 0, overlaySize.width, overlaySize.height);
    [overlayImage drawAtPoint:NSMakePoint(position.x,
                                          position.y - overlaySize.height)
                     fromRect:imageFrame
                    operation:NSCompositeSourceOver
                     fraction:1.0];
  }

  return themed;
}

- (NSTouchBar*)makeTouchBar {
  if (@available(macOS 10.12.2, *)) {
    BrowserWindowController* bwc =
        [BrowserWindowController browserWindowControllerForWindow:self];
    return [[bwc browserWindowTouchBar] makeTouchBar];
  } else {
    return nil;
  }
}

- (NSColor*)titleColor {
  const ui::ThemeProvider* themeProvider = [self themeProvider];
  if (!themeProvider)
    return [NSColor windowFrameTextColor];

  ThemedWindowStyle windowStyle = [self themedWindowStyle];
  BOOL incognito = windowStyle & THEMED_INCOGNITO;

  if (incognito)
    return [NSColor whiteColor];
  else
    return [NSColor windowFrameTextColor];
}

@end
