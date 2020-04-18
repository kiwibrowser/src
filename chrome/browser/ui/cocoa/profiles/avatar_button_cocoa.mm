// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/profiles/avatar_button.h"

@interface AvatarButton (Private)

- (void)rightMouseDown:(NSEvent*)event;
- (void)performRightClick;

@end

@implementation AvatarButton

@synthesize isActive = isActive_;

// Overrides -rightMouseDown and implements a custom mouse tracking loop.
- (void)rightMouseDown:(NSEvent*)event {
  NSEvent* nextEvent = event;
  BOOL mouseInBounds = NO;
  hoverState_ = kHoverStateMouseDown;

  do {
    nextEvent = [[self window]
        nextEventMatchingMask:NSRightMouseDraggedMask |
                              NSRightMouseUpMask];

    mouseInBounds = NSPointInRect(
        [self convertPoint:[nextEvent locationInWindow] fromView:nil],
        [self convertRect:[self frame] fromView:nil]);
  } while (NSRightMouseUp != [nextEvent type]);

  hoverState_ = kHoverStateNone;

  if (mouseInBounds) {
    hoverState_ = kHoverStateMouseOver;
    [self performRightClick];
  }
}

- (void)performRightClick {
  [[super target] performSelector:rightAction_ withObject:self];
}

- (void)setRightAction:(SEL)selector {
  rightAction_ = selector;
}

- (void)setIsActive:(BOOL)isActive {
  BOOL activeChanged = isActive_ != isActive;
  isActive_ = isActive;
  [self setNeedsDisplay:activeChanged];
}

@end
