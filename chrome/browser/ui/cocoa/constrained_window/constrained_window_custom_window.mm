// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_custom_window.h"

#include "base/command_line.h"
#import "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/chrome_style.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_sheet_controller.h"
#include "content/public/common/content_switches.h"
#include "skia/ext/skia_utils_mac.h"
#include "ui/gfx/scoped_ns_graphics_context_save_gstate_mac.h"

@implementation ConstrainedWindowCustomWindow

- (id)initWithContentRect:(NSRect)contentRect {
  return [self initWithContentRect:contentRect
                         styleMask:NSBorderlessWindowMask];
}

- (id)initWithContentRect:(NSRect)contentRect
                styleMask:(NSUInteger)windowStyle {
  if ((self = [self initWithContentRect:contentRect
                              styleMask:windowStyle
                                backing:NSBackingStoreBuffered
                                  defer:NO])) {
    base::scoped_nsobject<NSView> contentView(
        [[ConstrainedWindowCustomWindowContentView alloc]
            initWithFrame:NSZeroRect]);
    [self setContentView:contentView];
  }
  return self;
}

- (id)initWithContentRect:(NSRect)contentRect
                styleMask:(NSUInteger)windowStyle
                  backing:(NSBackingStoreType)bufferingType
                    defer:(BOOL)deferCreation {
  if ((self = [super initWithContentRect:contentRect
                               styleMask:windowStyle
                                 backing:bufferingType
                                   defer:NO])) {
    // Don't draw shadows in tests, as that causes Window Server crashes on VMs.
    // https://crbug.com/515627
    base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
    if (!command_line->HasSwitch(switches::kTestType))
      [self setHasShadow:YES];
    [self setBackgroundColor:skia::SkColorToCalibratedNSColor(
        chrome_style::GetBackgroundColor())];
    [self setOpaque:NO];
    [self setReleasedWhenClosed:NO];
  }
  return self;
}

- (BOOL)canBecomeKeyWindow {
  return YES;
}

- (NSRect)frameRectForContentRect:(NSRect)windowContent {
  id<ConstrainedWindowSheet> sheet = [ConstrainedWindowSheetController
      sheetForOverlayWindow:[self parentWindow]];
  ConstrainedWindowSheetController* sheetController =
      [ConstrainedWindowSheetController controllerForSheet:sheet];

  // Sheet controller may be nil if this window hasn't been shown yet.
  if (!sheetController)
    return windowContent;

  NSRect frame;
  frame.origin = [sheetController originForSheet:sheet
                                  withWindowSize:windowContent.size];
  frame.size = windowContent.size;
  return frame;
}

@end

@implementation ConstrainedWindowCustomWindowContentView

- (void)drawRect:(NSRect)rect {
  gfx::ScopedNSGraphicsContextSaveGState state;

  // Draw symmetric difference between rect path and oval path as "clear".
  NSBezierPath* ovalPath = [NSBezierPath
      bezierPathWithRoundedRect:[self bounds]
                        xRadius:chrome_style::kBorderRadius
                        yRadius:chrome_style::kBorderRadius];
  NSBezierPath* path = [NSBezierPath bezierPathWithRect:[self bounds]];
  [path appendBezierPath:ovalPath];
  [path setWindingRule:NSEvenOddWindingRule];
  [[NSGraphicsContext currentContext] setCompositingOperation:
      NSCompositeCopy];
  [[NSColor clearColor] set];
  [path fill];

  [[self window] invalidateShadow];
}

@end
