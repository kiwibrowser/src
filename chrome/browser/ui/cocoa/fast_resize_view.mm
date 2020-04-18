// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/fast_resize_view.h"

#import <Cocoa/Cocoa.h>

#include "base/logging.h"
#include "base/mac/scoped_nsobject.h"
#include "ui/base/cocoa/animation_utils.h"

@implementation FastResizeView

- (id)initWithFrame:(NSRect)frameRect {
  if ((self = [super initWithFrame:frameRect])) {
    ScopedCAActionDisabler disabler;
    base::scoped_nsobject<CALayer> layer([[CALayer alloc] init]);
    [self setLayer:layer];
    [self setWantsLayer:YES];
  }
  return self;
}

- (BOOL)isOpaque {
  return YES;
}

// Override -[NSView hitTest:] to prevent mouse events reaching subviews while
// the window is displaying a modal sheet. Without this, context menus can be
// shown on a right-click and trigger all kinds of things (e.g. Print).
- (NSView*)hitTest:(NSPoint)aPoint {
  if ([[self window] attachedSheet])
    return self;
  return [super hitTest:aPoint];
}

@end

